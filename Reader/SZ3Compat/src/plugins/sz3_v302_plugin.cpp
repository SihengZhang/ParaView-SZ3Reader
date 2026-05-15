// sz3_v302_plugin.cpp - SZ3 v3.0.2 decompressor plugin
// v3.0.2 has template-based Config<T, N> API

// Include standard headers before SZ3 headers (older SZ3 versions miss these)
#include <memory>
#include <cstdint>

#include "compressor/SZGeneralCompressor.hpp"
#include "frontend/SZ3Frontend.hpp"
#include "encoder/HuffmanEncoder.hpp"
#include "lossless/Lossless_zstd.hpp"
#include "predictor/ComposedPredictor.hpp"
#include "predictor/LorenzoPredictor.hpp"
#include "predictor/RegressionPredictor.hpp"
#include "quantizer/IntegerQuantizer.hpp"
#include "plugin_interface.h"
#include <array>

namespace {

template<typename T, SZ::uint N>
int decompress_impl_n(const char* cmpData, size_t cmpSize,
                      const size_t* userDims,
                      T** decData, size_t* numElements) {
    try {
        using namespace SZ;

        std::array<size_t, N> dims;
        for (SZ::uint i = 0; i < N; ++i) {
            dims[i] = userDims[i];
        }

        T eb = static_cast<T>(1e-4);
        Config<T, N> conf(eb, dims);

        std::vector<std::shared_ptr<concepts::PredictorInterface<T, N>>> predictors;
        predictors.push_back(std::make_shared<LorenzoPredictor<T, N, 1>>(eb));
        predictors.push_back(std::make_shared<RegressionPredictor<T, N>>(conf.block_size, eb));
        auto predictor = ComposedPredictor<T, N>(predictors);

        auto quantizer = LinearQuantizer<T>(eb, conf.quant_state_num / 2);

        auto frontend = SZ3Frontend<T, N, decltype(predictor), decltype(quantizer)>(
            conf, predictor, quantizer
        );

        auto encoder = HuffmanEncoder<int>();
        auto lossless = Lossless_zstd();

        auto sz = make_sz_general_compressor(conf, frontend, encoder, lossless);

        T* out = sz.decompress(reinterpret_cast<const uchar*>(cmpData), cmpSize);

        size_t totalElements = 1;
        for (SZ::uint i = 0; i < N; ++i) {
            totalElements *= dims[i];
        }

        *decData = out;
        *numElements = totalElements;
        return SZ3COMPAT_OK;

    } catch (...) {
        return SZ3COMPAT_ERR_DECOMPRESS;
    }
}

template<typename T>
int decompress_impl(const char* cmpData, size_t cmpSize,
                    const size_t* userDims, int numDims,
                    T** decData, size_t* numElements) {
    switch (numDims) {
        case 1:
            return decompress_impl_n<T, 1>(cmpData, cmpSize, userDims, decData, numElements);
        case 2:
            return decompress_impl_n<T, 2>(cmpData, cmpSize, userDims, decData, numElements);
        case 3:
            return decompress_impl_n<T, 3>(cmpData, cmpSize, userDims, decData, numElements);
        case 4:
            return decompress_impl_n<T, 4>(cmpData, cmpSize, userDims, decData, numElements);
        default:
            return SZ3COMPAT_ERR_DIM_MISMATCH;
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
    return "3.0.2";
}

}  // extern "C"
