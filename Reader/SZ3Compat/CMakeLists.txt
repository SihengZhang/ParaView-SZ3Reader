cmake_minimum_required(VERSION 3.18)

# SZ3Compat - Multi-version SZ3 decompression with dynamic plugins
# Integrated for ParaView SZ3Reader plugin

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Plugin output directory - place plugins inside the SZ3Reader plugin folder
set(SZ3COMPAT_PLUGIN_DIR "${CMAKE_BINARY_DIR}/lib/paraview-6.1/plugins/SZ3Reader"
    CACHE PATH "Directory for SZ3 version plugins")
file(MAKE_DIRECTORY ${SZ3COMPAT_PLUGIN_DIR})

# Fetch all SZ3 versions
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FetchSZ3Versions.cmake)
setup_sz3_multi_versions()

# Set up Zstd (shared across all plugins)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_search_module(ZSTD IMPORTED_TARGET libzstd)
endif()

if(NOT ZSTD_FOUND)
    set(ZSTD_SRC_DIR "${SZ3_V332_SOURCE_DIR}/tools/zstd/zstd/lib")
    if(NOT EXISTS "${ZSTD_SRC_DIR}")
        include(FetchContent)
        FetchContent_Declare(
            zstd_lib
            URL "https://github.com/facebook/zstd/releases/download/v1.5.5/zstd-1.5.5.tar.gz"
        )
        FetchContent_MakeAvailable(zstd_lib)
        set(ZSTD_SRC_DIR "${zstd_lib_SOURCE_DIR}/lib")
    endif()

    file(GLOB ZSTD_COMMON_SOURCES "${ZSTD_SRC_DIR}/common/*.c")
    file(GLOB ZSTD_COMPRESS_SOURCES "${ZSTD_SRC_DIR}/compress/*.c")
    file(GLOB ZSTD_DECOMPRESS_SOURCES "${ZSTD_SRC_DIR}/decompress/*.c")

    add_library(sz3compat_zstd STATIC
        ${ZSTD_COMMON_SOURCES}
        ${ZSTD_COMPRESS_SOURCES}
        ${ZSTD_DECOMPRESS_SOURCES}
    )
    target_include_directories(sz3compat_zstd
        PUBLIC $<BUILD_INTERFACE:${ZSTD_SRC_DIR}>
        PRIVATE "${ZSTD_SRC_DIR}/common"
    )
    target_compile_definitions(sz3compat_zstd PRIVATE ZSTD_DISABLE_ASM)
    set_target_properties(sz3compat_zstd PROPERTIES
        C_VISIBILITY_PRESET hidden
        POSITION_INDEPENDENT_CODE ON
    )
    set(ZSTD_TARGET sz3compat_zstd)
else()
    message(STATUS "SZ3Compat: Using system ZSTD")
    set(ZSTD_TARGET PkgConfig::ZSTD)
endif()

# Build version plugins as shared libraries
# v3.0.2 plugin (different include structure)
add_library(sz3_v302 SHARED src/plugins/sz3_v302_plugin.cpp)
target_include_directories(sz3_v302 PRIVATE
    ${SZ3_V302_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins
)
target_link_libraries(sz3_v302 PRIVATE ${ZSTD_TARGET})
set_target_properties(sz3_v302 PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${SZ3COMPAT_PLUGIN_DIR}"
    CXX_VISIBILITY_PRESET hidden
)

# v3.1.7+ plugins
foreach(ver IN ITEMS 317 320 321 330 332)
    add_library(sz3_v${ver} SHARED src/plugins/sz3_v${ver}_plugin.cpp)
    target_link_libraries(sz3_v${ver} PRIVATE sz3_v${ver}_includes ${ZSTD_TARGET})
    target_include_directories(sz3_v${ver} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/plugins)
    set_target_properties(sz3_v${ver} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${SZ3COMPAT_PLUGIN_DIR}"
        CXX_VISIBILITY_PRESET hidden
    )
endforeach()

# Main dispatcher library (STATIC for linking into ParaView module)
add_library(SZ3Compat STATIC src/sz3_compat_dynamic.cpp)
target_include_directories(SZ3Compat
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(SZ3Compat PRIVATE ${CMAKE_DL_LIBS})
target_compile_definitions(SZ3Compat PRIVATE
    SZ3COMPAT_PLUGIN_DIR="${SZ3COMPAT_PLUGIN_DIR}"
)
target_compile_features(SZ3Compat PUBLIC cxx_std_17)

# Ensure plugins are built before the dispatcher library
add_dependencies(SZ3Compat sz3_v302 sz3_v317 sz3_v320 sz3_v321 sz3_v330 sz3_v332)
