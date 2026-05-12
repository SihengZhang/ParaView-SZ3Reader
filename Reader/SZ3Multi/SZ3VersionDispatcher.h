#ifndef SZ3_VERSION_DISPATCHER_H
#define SZ3_VERSION_DISPATCHER_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

// SZ3 Magic Number
#define SZ3_MAGIC_NUMBER 0xF342F310

// Read version from compressed data header (bytes 4-8)
uint32_t SZ3_ReadVersion(const char* cmpData, size_t cmpSize);

// Convert version uint32_t to string (e.g., "3.3.2")
std::string SZ3_VersionToString(uint32_t version);

// Convert version string to uint32_t
uint32_t SZ3_VersionFromString(const std::string& version);

// Check if a version is supported
bool SZ3_IsVersionSupported(uint32_t version);

// Get list of supported version strings
std::vector<std::string> SZ3_GetSupportedVersions();

// Multi-version decompress for float - automatically selects correct version
// Returns 0 on success, non-zero on error
// dims[3] will be filled with the dimensions from the config
int SZ3_DecompressFloat(const char* cmpData, size_t cmpSize,
                        float* decData, size_t num, int dims[3]);

// Multi-version decompress for double - automatically selects correct version
// Returns 0 on success, non-zero on error
// dims[3] will be filled with the dimensions from the config
int SZ3_DecompressDouble(const char* cmpData, size_t cmpSize,
                         double* decData, size_t num, int dims[3]);

// Get error message for last error
const char* SZ3_GetLastError();

#endif // SZ3_VERSION_DISPATCHER_H
