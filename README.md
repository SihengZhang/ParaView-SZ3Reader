# SZ3Reader Plugin

ParaView/VTK reader for direct visualization of data files compressed using the SZ3 compressor.

Version: 0.3

## Features

- Read `.sz3` compressed files directly in ParaView
- Multi-version SZ3 support with dynamic plugin loading
- Automatic version detection from compressed file header
- Supports both single (float32) and double (float64) precision
- Automatically fetches and builds SZ3 dependencies

## Supported SZ3 Versions

Supported: 3.0.2, 3.1.7, 3.2.0, 3.2.1, 3.3.0, 3.3.2

## Build Instructions

### Prerequisites

- ParaView source code
- CMake 3.18+
- C++17 compatible compiler
- Git

### Build Steps

```bash
# 1. Clone or copy plugin to ParaView Plugins directory
cp -r /path/to/ParaView-SZ3Reader /path/to/ParaView/source/Plugins/SZ3Reader

# 2. Create build directory (if not exists)
mkdir -p /path/to/paraview_build
cd /path/to/paraview_build

# 3. Configure ParaView with plugin enabled
cmake /path/to/ParaView/source -DPARAVIEW_PLUGIN_ENABLE_SZ3Reader=ON

# 4. Build the plugin
cmake --build . --target SZ3Reader
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `PARAVIEW_PLUGIN_ENABLE_SZ3Reader` | OFF | Enable building the SZ3Reader plugin |

### Rebuild After Changes

If you modify the plugin or pull updates:

```bash
cd /path/to/paraview_build

# Clean plugin build (optional, for fresh rebuild)
rm -rf Plugins/SZ3Reader

# Reconfigure and rebuild
cmake .
cmake --build . --target SZ3Reader
```

## Usage in ParaView

1. Launch ParaView
2. File → Open → Select a `.sz3` file
3. Set the domain dimensions (X, Y, Z)
4. Check "Double Precision" if the data was compressed as float64
5. Click Apply

## Architecture

The plugin uses dynamic plugin loading (dlopen) to support multiple SZ3 versions:

- **SZ3Compat dispatcher**: Detects version from compressed file header and loads the appropriate plugin
- **Version plugins**: Separate shared libraries for each SZ3 version (libsz3_v302.so, libsz3_v317.so, etc.)
- **Automatic version detection**: Files compressed with SZ3 v3.2.0+ contain version info in header; older versions are detected by analyzing the data structure

## References

- SZ3 Compressor: https://szcompressor.org/ | https://github.com/szcompressor/SZ3
- SZ3-multi-version: https://github.com/SihengZhang/SZ3-multi-version
- ParaView: https://www.paraview.org/

## Contact

Please report bugs and suggestions in the Issues section.
