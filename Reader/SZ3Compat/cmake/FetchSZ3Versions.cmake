# FetchSZ3Versions.cmake
# Downloads and configures multiple SZ3 versions for backward-compatible decompression

include(FetchContent)

# CMP0135: Set download timestamp behavior
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()

# SZ3 repository URL
set(SZ3_REPO "https://github.com/szcompressor/SZ3.git")

# Convert version string to suffix (e.g., "3.1.7" -> "317")
function(sz3_version_to_suffix version out_var)
    string(REPLACE "." "" suffix "${version}")
    set(${out_var} "${suffix}" PARENT_SCOPE)
endfunction()

# Fetch a single SZ3 version without running its CMakeLists.txt
function(fetch_sz3_version version)
    sz3_version_to_suffix(${version} ver_suffix)
    set(fetch_name "sz3_v${ver_suffix}")

    message(STATUS "Fetching SZ3 v${version}...")

    FetchContent_Declare(
        ${fetch_name}
        GIT_REPOSITORY ${SZ3_REPO}
        GIT_TAG        "v${version}"
        SOURCE_SUBDIR  ""
    )

    FetchContent_GetProperties(${fetch_name})
    if(NOT ${fetch_name}_POPULATED)
        FetchContent_Populate(${fetch_name})
    endif()

    set(src_dir "${${fetch_name}_SOURCE_DIR}")
    set(bin_dir "${${fetch_name}_BINARY_DIR}")

    # Export to parent scope
    set(SZ3_V${ver_suffix}_SOURCE_DIR "${src_dir}" PARENT_SCOPE)
    set(SZ3_V${ver_suffix}_BINARY_DIR "${bin_dir}" PARENT_SCOPE)
endfunction()

# Generate version.hpp for a specific SZ3 version
function(generate_sz3_version_header version source_dir binary_dir)
    sz3_version_to_suffix(${version} ver_suffix)

    # Parse version components
    string(REPLACE "." ";" ver_components "${version}")
    list(GET ver_components 0 ver_major)
    list(GET ver_components 1 ver_minor)
    list(GET ver_components 2 ver_patch)

    # Set variables for version.hpp.in
    set(PROJECT_NAME "SZ3")
    set(PROJECT_VERSION "${version}")
    set(PROJECT_VERSION_MAJOR "${ver_major}")
    set(PROJECT_VERSION_MINOR "${ver_minor}")
    set(PROJECT_VERSION_PATCH "${ver_patch}")
    set(PROJECT_VERSION_TWEAK "")
    set(SZ3_DATA_VERSION "${version}")

    # Check if version.hpp.in exists at expected location
    set(version_in_path "${source_dir}/include/SZ3/version.hpp.in")
    if(NOT EXISTS "${version_in_path}")
        # v3.0.x has different structure - no version.hpp.in
        message(STATUS "No version.hpp.in for SZ3 v${version} (expected for v3.0.x)")
        return()
    endif()

    # Create output directory
    file(MAKE_DIRECTORY "${binary_dir}/include/SZ3")

    configure_file(
        "${version_in_path}"
        "${binary_dir}/include/SZ3/version.hpp"
        @ONLY
    )

    message(STATUS "Generated version.hpp for SZ3 v${version}")
endfunction()

# Create INTERFACE target for a version's include paths
function(create_sz3_include_target version source_dir binary_dir)
    sz3_version_to_suffix(${version} ver_suffix)
    set(target_name "sz3_v${ver_suffix}_includes")

    add_library(${target_name} INTERFACE)

    # Determine include path based on version structure
    # v3.0.x uses include/, v3.1.0+ uses include/SZ3/
    if(EXISTS "${source_dir}/include/SZ3")
        target_include_directories(${target_name} INTERFACE
            "${source_dir}/include"
            "${binary_dir}/include"
        )
    else()
        # v3.0.x structure
        target_include_directories(${target_name} INTERFACE
            "${source_dir}/include"
        )
    endif()

    target_compile_features(${target_name} INTERFACE cxx_std_17)

    set(SZ3_V${ver_suffix}_INCLUDE_TARGET ${target_name} PARENT_SCOPE)
endfunction()

# Master function: set up all required SZ3 versions
function(setup_sz3_multi_versions)
    set(SZ3_VERSIONS "3.0.2" "3.1.7" "3.2.0" "3.2.1" "3.3.0" "3.3.2")

    foreach(version IN LISTS SZ3_VERSIONS)
        sz3_version_to_suffix(${version} ver_suffix)

        # Fetch source
        fetch_sz3_version(${version})

        set(src_dir "${SZ3_V${ver_suffix}_SOURCE_DIR}")
        set(bin_dir "${SZ3_V${ver_suffix}_BINARY_DIR}")

        # Generate version header (if applicable)
        generate_sz3_version_header(${version} "${src_dir}" "${bin_dir}")

        # Create include target
        create_sz3_include_target(${version} "${src_dir}" "${bin_dir}")

        # Export variables
        set(SZ3_V${ver_suffix}_SOURCE_DIR "${src_dir}" PARENT_SCOPE)
        set(SZ3_V${ver_suffix}_BINARY_DIR "${bin_dir}" PARENT_SCOPE)
        set(SZ3_V${ver_suffix}_INCLUDE_TARGET "sz3_v${ver_suffix}_includes" PARENT_SCOPE)
    endforeach()
endfunction()
