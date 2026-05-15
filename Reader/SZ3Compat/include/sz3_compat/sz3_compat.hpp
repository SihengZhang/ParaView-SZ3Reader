#ifndef SZ3_COMPAT_HPP
#define SZ3_COMPAT_HPP

#include <cstddef>
#include <vector>

namespace SZ3Compat {

// Main API - decompress from memory buffer (SZ3 v3.3.2 style)
// @tparam T Data type: float or double
// @param cmpData Pointer to compressed data
// @param cmpSize Size of compressed data in bytes
// @param dims Dimension sizes (e.g., {100, 200, 300}) - REQUIRED
//        - Pre-v3.2.0: dims.size() used for numDims; sizes validated against embedded data
//        - v3.2.0+: Both count and sizes validated against embedded config
// @return T* pointer to decompressed data (caller must delete[])
// @throws std::runtime_error if dims don't match config, or on decompression failure
template <class T>
T* SZ3Compat_decompress(const char* cmpData, size_t cmpSize,
                        const std::vector<size_t>& dims);

// Alternative API with output parameter (SZ3 v3.3.2 style)
template <class T>
void SZ3Compat_decompress(const char* cmpData, size_t cmpSize,
                          const std::vector<size_t>& dims, T*& decData);

}  // namespace SZ3Compat

#endif  // SZ3_COMPAT_HPP
