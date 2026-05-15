# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ParaView plugin for reading SZ3-compressed data files (`.sz3`). Enables direct visualization of lossy-compressed scientific data in ParaView without pre-decompression.

## Build Instructions

This plugin must be built as part of ParaView:

1. Place this directory under `${PARAVIEW_SOURCE}/Plugins/`
2. Build ParaView with `-DPARAVIEW_PLUGIN_ENABLE_SZ3Reader=ON`
3. Requires SZ3 library pre-installed (https://github.com/szcompressor/SZ3)

Dependencies: SZ3, ZSTD (transitive via SZ3)

## Architecture

**Reader Implementation** ([Reader/vtkSZ3Reader.cxx](Reader/vtkSZ3Reader.cxx))
- Inherits from `vtkImageAlgorithm` - outputs `vtkImageData`
- `RequestData()`: reads compressed file, calls SZ3 decompression API, populates VTK point data
- Templated `Decompress<T>()` handles float/double based on `UseDoublePrecision` flag

**ParaView Integration** ([Reader/SZ3Reader.xml](Reader/SZ3Reader.xml))
- Server manager XML defines UI properties exposed in ParaView
- Properties: FileName, DomainDimensions (X,Y,Z), DoublePrecision checkbox
- Registered as reader for `.sz3` extension via `<ReaderFactory>`

**CMake** ([cmake/FindSZ3.cmake](cmake/FindSZ3.cmake))
- Custom find module creates `SZ3::SZ3` imported target
- Handles ZSTD dependency attachment automatically
