// sz3_v317_plugin.cpp - SZ3 v3.1.7 decompressor plugin

// Include standard headers before SZ3 headers (older SZ3 versions miss these)
#include <cstdint>
#include <memory>

#include "SZ3/api/sz.hpp"
#include "plugin_interface.h"

namespace {

template<typename T>
int decompress_impl(const char* cmpData, size_t cmpSize,
                    const size_t* userDims, int numDims,
                    T** decData, size_t* numElements) {
    try {
        SZ::Config conf;
        T* out = nullptr;

        SZ_decompress<T>(conf, const_cast<char*>(cmpData), cmpSize, out);

        // Skip strict dimension validation - SZ3 uses dimensions from compressed header
        // Just verify total element count matches if user provided dimensions
        size_t userTotal = 1;
        for (int i = 0; i < numDims; ++i) {
            userTotal *= userDims[i];
        }

        if (userTotal != conf.num) {
            delete[] out;
            return SZ3COMPAT_ERR_DIM_MISMATCH;
        }

        *decData = out;
        *numElements = conf.num;
        return SZ3COMPAT_OK;

    } catch (...) {
        return SZ3COMPAT_ERR_DECOMPRESS;
    }
}

}  // namespace

extern "C" {

SZ3COMPAT_PLUGIN_EXPORT
int sz3_decompress_float(const char* cmpData, size_t cmpSize,
                         const size_t* dims, int numDims,
                         float** decData, size_t* numElements) {
    return decompress_impl<float>(cmpData, cmpSize, dims, numDims, decData, numElements);
}

SZ3COMPAT_PLUGIN_EXPORT
int sz3_decompress_double(const char* cmpData, size_t cmpSize,
                          const size_t* dims, int numDims,
                          double** decData, size_t* numElements) {
    return decompress_impl<double>(cmpData, cmpSize, dims, numDims, decData, numElements);
}

SZ3COMPAT_PLUGIN_EXPORT
const char* sz3_plugin_version() {
    return "3.1.7";
}

}  // extern "C"
