// sz3_compat_dynamic.cpp - Dynamic dispatcher for SZ3 backward compatibility

#include "sz3_compat/sz3_compat.hpp"
#include <dlfcn.h>
#include <cstring>
#include <unordered_map>
#include <mutex>
#include <stdexcept>

#ifndef SZ3COMPAT_PLUGIN_DIR
#define SZ3COMPAT_PLUGIN_DIR "./plugins"
#endif

namespace {

constexpr uint32_t SZ3_MAGIC = 0xF342F310;
constexpr uint32_t ZSTD_MAGIC = 0xFD2FB528;

struct VersionInfo {
    std::string suffix;     // "302", "317", etc.
    bool hasMagicNumber;
    uint32_t dataVersion;
};

size_t findZstdFrameSize(const char* data, size_t size) {
    if (size < 4) return 0;

    uint8_t frameHeaderDesc = static_cast<uint8_t>(data[4]);
    bool singleSegment = (frameHeaderDesc >> 5) & 1;
    int fcsFieldSize = frameHeaderDesc >> 6;

    static const int fcsFieldSizes[] = {0, 2, 4, 8};
    int dictIdFieldSize = frameHeaderDesc & 3;
    static const int dictIdFieldSizes[] = {0, 1, 2, 4};

    int windowDescSize = singleSegment ? 0 : 1;
    int headerSize = 4 + 1 + windowDescSize + dictIdFieldSizes[dictIdFieldSize] + (fcsFieldSize == 0 && singleSegment ? 1 : fcsFieldSizes[fcsFieldSize]);

    size_t pos = headerSize;
    while (pos < size) {
        if (pos + 3 > size) return 0;
        uint32_t blockHeader;
        std::memcpy(&blockHeader, data + pos, 3);
        blockHeader &= 0xFFFFFF;
        bool lastBlock = blockHeader & 1;
        size_t blockSize = blockHeader >> 3;

        pos += 3 + blockSize;
        if (lastBlock) break;
    }

    bool hasChecksum = (frameHeaderDesc >> 2) & 1;
    if (hasChecksum) pos += 4;

    return pos;
}

VersionInfo detectVersion(const char* cmpData, size_t cmpSize) {
    if (cmpSize < 12) {
        throw std::runtime_error("Compressed data too small");
    }

    uint32_t magic;
    std::memcpy(&magic, cmpData, sizeof(magic));

    if (magic == SZ3_MAGIC) {
        uint32_t version;
        std::memcpy(&version, cmpData + 4, sizeof(version));

        uint32_t major = (version >> 24) & 0xFF;
        uint32_t minor = (version >> 16) & 0xFF;
        uint32_t patch = (version >> 8) & 0xFF;

        std::string suffix = std::to_string(major) + std::to_string(minor) + std::to_string(patch);
        return {suffix, true, version};
    }

    // Pre-v3.2.0 data - detect v302 vs v317 by zstd frame structure
    // v302: single zstd frame uses all remaining bytes after 8-byte header
    // v317+: zstd frame followed by additional data

    uint32_t zstdMagic;
    std::memcpy(&zstdMagic, cmpData + 8, sizeof(zstdMagic));
    if (zstdMagic != ZSTD_MAGIC) {
        return {"317", false, 0};  // Unknown format, try v317
    }

    size_t zstdFrameSize = findZstdFrameSize(cmpData + 8, cmpSize - 8);
    size_t remainingAfterFrame = (cmpSize - 8) - zstdFrameSize;

    // v302: zstd frame uses exactly all remaining bytes
    // v317: has extra data after zstd frame
    if (remainingAfterFrame == 0) {
        return {"302", false, 0};
    } else {
        return {"317", false, 0};
    }
}

std::string getPluginPath(const std::string& suffix) {
    const char* envDir = getenv("SZ3COMPAT_PLUGIN_DIR");
    std::string dir = envDir ? envDir : SZ3COMPAT_PLUGIN_DIR;
    return dir + "/libsz3_v" + suffix + ".so";
}

std::unordered_map<std::string, void*> pluginCache;
std::mutex cacheMutex;

void* loadPlugin(const std::string& suffix) {
    std::lock_guard<std::mutex> lock(cacheMutex);

    auto it = pluginCache.find(suffix);
    if (it != pluginCache.end()) {
        return it->second;
    }

    std::string path = getPluginPath(suffix);
    void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        throw std::runtime_error(std::string("Failed to load plugin: ") + dlerror());
    }

    pluginCache[suffix] = handle;
    return handle;
}

using DecompressFloatFunc = int(*)(const char*, size_t, const size_t*, int, float**, size_t*);
using DecompressDoubleFunc = int(*)(const char*, size_t, const size_t*, int, double**, size_t*);

template<typename T>
T* decompress_dispatch(const char* cmpData, size_t cmpSize,
                       const std::vector<size_t>& dims) {
    if (dims.empty() || dims.size() > 4) {
        throw std::runtime_error("dims must have 1-4 dimensions");
    }

    auto vinfo = detectVersion(cmpData, cmpSize);

    void* plugin = loadPlugin(vinfo.suffix);

    const char* funcName = std::is_same<T, float>::value
        ? "sz3_decompress_float" : "sz3_decompress_double";

    void* funcPtr = dlsym(plugin, funcName);
    if (!funcPtr) {
        throw std::runtime_error("Plugin missing decompress function");
    }

    T* data = nullptr;
    size_t numElements = 0;
    int rc;

    if constexpr (std::is_same<T, float>::value) {
        auto func = reinterpret_cast<DecompressFloatFunc>(funcPtr);
        rc = func(cmpData, cmpSize, dims.data(), static_cast<int>(dims.size()), &data, &numElements);
    } else {
        auto func = reinterpret_cast<DecompressDoubleFunc>(funcPtr);
        rc = func(cmpData, cmpSize, dims.data(), static_cast<int>(dims.size()), &data, &numElements);
    }

    if (rc == 0) {
        return data;
    }

    if (rc == 2) {
        throw std::runtime_error("Dimension mismatch");
    }
    throw std::runtime_error("Decompression failed");
}

}  // namespace

namespace SZ3Compat {

template <class T>
T* SZ3Compat_decompress(const char* cmpData, size_t cmpSize,
                        const std::vector<size_t>& dims) {
    return decompress_dispatch<T>(cmpData, cmpSize, dims);
}

template <class T>
void SZ3Compat_decompress(const char* cmpData, size_t cmpSize,
                          const std::vector<size_t>& dims, T*& decData) {
    decData = decompress_dispatch<T>(cmpData, cmpSize, dims);
}

// Explicit template instantiations
template float* SZ3Compat_decompress<float>(const char*, size_t, const std::vector<size_t>&);
template double* SZ3Compat_decompress<double>(const char*, size_t, const std::vector<size_t>&);
template void SZ3Compat_decompress<float>(const char*, size_t, const std::vector<size_t>&, float*&);
template void SZ3Compat_decompress<double>(const char*, size_t, const std::vector<size_t>&, double*&);

}  // namespace SZ3Compat
