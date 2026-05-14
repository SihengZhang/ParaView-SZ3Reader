set(SZ3_GIT_TAG "master" CACHE STRING "SZ3 Git tag or branch to fetch")

include(FetchContent)

# Build SZ3 inside the plugin's build directory (not top-level _deps)
set(FETCHCONTENT_BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_deps")

# Force fetching SZ3 instead of using system-installed version
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)

# Configure SZ3 build options - disable tools, use bundled ZSTD
set(BUILD_SZ3_BINARY OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(SZ3_USE_BUNDLED_ZSTD ON CACHE BOOL "" FORCE)

FetchContent_Declare(
  SZ3
  GIT_REPOSITORY https://github.com/szcompressor/SZ3.git
  GIT_TAG ${SZ3_GIT_TAG}
  OVERRIDE_FIND_PACKAGE
)
FetchContent_MakeAvailable(SZ3)

paraview_add_plugin(SZ3Reader
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "0.1"
  MODULES SZ3Reader::vtkSZ3Reader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")
