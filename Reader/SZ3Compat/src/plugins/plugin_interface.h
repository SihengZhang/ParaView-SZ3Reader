// plugin_interface.h - C interface for SZ3 decompressor plugins

#ifndef SZ3COMPAT_PLUGIN_INTERFACE_H
#define SZ3COMPAT_PLUGIN_INTERFACE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Export macro for plugin functions
#if defined(_WIN32) || defined(__CYGWIN__)
  #define SZ3COMPAT_PLUGIN_EXPORT __declspec(dllexport)
#else
  #define SZ3COMPAT_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#define SZ3COMPAT_OK              0
#define SZ3COMPAT_ERR_DECOMPRESS  1
#define SZ3COMPAT_ERR_DIM_MISMATCH 2

// Decompress float data
// Returns: 0 on success, error code on failure
// decData: Allocated by plugin, caller must delete[]
SZ3COMPAT_PLUGIN_EXPORT
int sz3_decompress_float(const char* cmpData, size_t cmpSize,
                         const size_t* dims, int numDims,
                         float** decData, size_t* numElements);

// Decompress double data
SZ3COMPAT_PLUGIN_EXPORT
int sz3_decompress_double(const char* cmpData, size_t cmpSize,
                          const size_t* dims, int numDims,
                          double** decData, size_t* numElements);

// Return plugin version string (e.g., "3.3.2")
SZ3COMPAT_PLUGIN_EXPORT
const char* sz3_plugin_version();

#ifdef __cplusplus
}
#endif

#endif // SZ3COMPAT_PLUGIN_INTERFACE_H
