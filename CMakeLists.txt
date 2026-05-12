# SZ3Reader ParaView Plugin
# Supports multi-version SZ3 decompression

paraview_add_plugin(SZ3Reader
  REQUIRED_ON_CLIENT
  REQUIRED_ON_SERVER
  VERSION "0.1"
  MODULES SZ3Reader::vtkSZ3Reader
  MODULE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/Reader/vtk.module")
