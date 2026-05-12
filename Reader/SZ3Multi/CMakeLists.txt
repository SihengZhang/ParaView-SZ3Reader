cmake_minimum_required(VERSION 3.18)

# List of SZ3 versions to support (hardcoded - only tested versions)
set(SZ3_SUPPORTED_VERSIONS "3.3.2;3.3.0")

message(STATUS "SZ3Multi: Building support for versions: ${SZ3_SUPPORTED_VERSIONS}")

include(FetchContent)

# Find or fetch ZSTD
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_search_module(ZSTD IMPORTED_TARGET libzstd)
endif()

if(NOT ZSTD_FOUND)
    message(STATUS "SZ3Multi: System ZSTD not found, fetching bundled version")
    FetchContent_Declare(
        zstd_fetched
        URL "https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-1.5.5.tar.gz"
        SOURCE_SUBDIR build/cmake
    )
    set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(zstd_fetched)
    set(ZSTD_INCLUDE_DIR "${zstd_fetched_SOURCE_DIR}/lib")
    set(ZSTD_LIBRARY libzstd_static)
else()
    message(STATUS "SZ3Multi: Found system ZSTD: ${ZSTD_INCLUDE_DIRS}")
    set(ZSTD_INCLUDE_DIR "${ZSTD_INCLUDE_DIRS}")
    set(ZSTD_LIBRARY PkgConfig::ZSTD)
endif()

# Force all SZ3 versions to be built inside this directory
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps")
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)

# Variables for generating dispatcher
set(SZ3_WRAPPER_INCLUDES "")
set(SZ3_SUPPORTED_VERSIONS_LIST "")
set(SZ3_FLOAT_DISPATCHER_ENTRIES "")
set(SZ3_DOUBLE_DISPATCHER_ENTRIES "")
set(SZ3_VERSION_LIBS "")

foreach(VERSION ${SZ3_SUPPORTED_VERSIONS})
    string(REPLACE "." "_" VERSION_UNDERSCORE ${VERSION})
    set(SZ3_VERSION ${VERSION})
    set(SZ3_VERSION_UNDERSCORE ${VERSION_UNDERSCORE})

    message(STATUS "SZ3Multi: Configuring SZ3 v${VERSION}")

    # Fetch this version
    FetchContent_Declare(
        sz3_${VERSION_UNDERSCORE}
        GIT_REPOSITORY https://github.com/szcompressor/SZ3.git
        GIT_TAG v${VERSION}
    )

    # Populate without adding to build (we'll handle include dirs manually)
    FetchContent_GetProperties(sz3_${VERSION_UNDERSCORE})
    if(NOT sz3_${VERSION_UNDERSCORE}_POPULATED)
        FetchContent_Populate(sz3_${VERSION_UNDERSCORE})
    endif()

    set(SZ3_${VERSION_UNDERSCORE}_SOURCE_INCLUDE "${sz3_${VERSION_UNDERSCORE}_SOURCE_DIR}/include")
    set(SZ3_${VERSION_UNDERSCORE}_BINARY_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/sz3_${VERSION_UNDERSCORE}_include")

    # Generate version.hpp for this SZ3 version
    # Read version from CMakeLists.txt or use the tag version
    set(PROJECT_VERSION ${VERSION})
    string(REPLACE "." ";" VERSION_PARTS ${VERSION})
    list(GET VERSION_PARTS 0 PROJECT_VERSION_MAJOR)
    list(GET VERSION_PARTS 1 PROJECT_VERSION_MINOR)
    list(GET VERSION_PARTS 2 PROJECT_VERSION_PATCH)
    set(PROJECT_VERSION_TWEAK 0)
    set(PROJECT_NAME "SZ3")
    set(SZ3_DATA_VERSION ${VERSION})

    file(MAKE_DIRECTORY "${SZ3_${VERSION_UNDERSCORE}_BINARY_INCLUDE}/SZ3")
    configure_file(
        "${sz3_${VERSION_UNDERSCORE}_SOURCE_DIR}/include/SZ3/version.hpp.in"
        "${SZ3_${VERSION_UNDERSCORE}_BINARY_INCLUDE}/SZ3/version.hpp"
        @ONLY
    )

    # Generate wrapper header for this version
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/templates/sz3_wrapper.hpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/sz3_v${VERSION_UNDERSCORE}_wrapper.hpp
        @ONLY
    )

    # Generate implementation file for this version
    configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/templates/sz3_impl.cpp.in
        ${CMAKE_CURRENT_BINARY_DIR}/sz3_v${VERSION_UNDERSCORE}.cpp
        @ONLY
    )

    # Create an OBJECT library for this version to isolate symbols
    add_library(sz3_impl_${VERSION_UNDERSCORE} OBJECT
        ${CMAKE_CURRENT_BINARY_DIR}/sz3_v${VERSION_UNDERSCORE}.cpp
    )

    target_include_directories(sz3_impl_${VERSION_UNDERSCORE}
        PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}
            ${SZ3_${VERSION_UNDERSCORE}_BINARY_INCLUDE}
            ${SZ3_${VERSION_UNDERSCORE}_SOURCE_INCLUDE}
            ${ZSTD_INCLUDE_DIR}
    )

    target_compile_features(sz3_impl_${VERSION_UNDERSCORE} PRIVATE cxx_std_17)

    # Link ZSTD
    target_link_libraries(sz3_impl_${VERSION_UNDERSCORE} PRIVATE ${ZSTD_LIBRARY})

    # Hide symbols to prevent conflicts between versions
    set_target_properties(sz3_impl_${VERSION_UNDERSCORE} PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )

    # Find OpenMP if available
    find_package(OpenMP QUIET)
    if(OpenMP_FOUND)
        target_link_libraries(sz3_impl_${VERSION_UNDERSCORE} PRIVATE OpenMP::OpenMP_CXX)
    endif()

    list(APPEND SZ3_VERSION_LIBS sz3_impl_${VERSION_UNDERSCORE})

    # Build dispatcher entries
    string(APPEND SZ3_WRAPPER_INCLUDES "#include \"sz3_v${VERSION_UNDERSCORE}_wrapper.hpp\"\n")
    string(APPEND SZ3_SUPPORTED_VERSIONS_LIST "    \"${VERSION}\",\n")
    string(APPEND SZ3_FLOAT_DISPATCHER_ENTRIES
        "    {\"${VERSION}\", sz3_v${VERSION_UNDERSCORE}_decompress_float},\n")
    string(APPEND SZ3_DOUBLE_DISPATCHER_ENTRIES
        "    {\"${VERSION}\", sz3_v${VERSION_UNDERSCORE}_decompress_double},\n")
endforeach()

# Create supported versions string for messages
string(REPLACE ";" ", " SZ3_SUPPORTED_VERSIONS_STRING "${SZ3_SUPPORTED_VERSIONS}")

# Generate dispatcher implementation
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/templates/SZ3VersionDispatcher.cpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/SZ3VersionDispatcher.cpp
    @ONLY
)

# Create the multi-version library combining all object libraries
add_library(SZ3Multi STATIC
    ${CMAKE_CURRENT_BINARY_DIR}/SZ3VersionDispatcher.cpp
)

# Link object libraries
foreach(LIB ${SZ3_VERSION_LIBS})
    target_sources(SZ3Multi PRIVATE $<TARGET_OBJECTS:${LIB}>)
endforeach()

target_include_directories(SZ3Multi
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_compile_features(SZ3Multi PUBLIC cxx_std_17)

# Link ZSTD to the final library
target_link_libraries(SZ3Multi PRIVATE ${ZSTD_LIBRARY})
