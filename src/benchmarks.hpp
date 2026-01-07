#ifndef BENCH_DTL_HPP
#define BENCH_DTL_HPP


#include <string>
#include <vector>
#include <unordered_map>
#include <stdint.h>
#include <cassert>
#include <variant>


#include "util.hpp"
#include "perf.hpp"
#include "dtl_stencils.hpp"
#include "im2col.hpp"
#include "dtl_api.hpp"
#include "matmul.hpp"


namespace benchmark
{

typedef std::vector<uint32_t> BenchParam;

// Declare function pointer type first
using BenchFn = std::string(*)(const struct BenchmarkData&, DTL::API*);

struct BenchmarkData
{
    std::string name;
    std::unordered_map<std::string, BenchParam> constants;
    std::unordered_map<std::string, uint32_t> params;
    std::unordered_map<std::string, uint32_t> other;
    BenchFn benchmark;            // Now this is valid
    std::string input_artifact;
    std::string output_artifact;
};


std::string bench_wrapper_im2col(const BenchmarkData&, DTL::API* api);
std::string bench_wrapper_db_colproject(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_matmul_transpose(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_nhwc_permutation(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_batch2space(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_tensorslice(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_tensorunfold(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_highdim_stencil(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_multigrid(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_haar_wavelet(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_cubestencil(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_SoA(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_multithread_db(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_multithread_SoA(const BenchmarkData& bench_data, DTL::API* api);





std::string InsertDTLConfigParameters(const BenchmarkData& benchmark_data);
std::string CreateConstants(const std::unordered_map<std::string, BenchParam>& constants);
std::string CreateBenchmarkConfig(const BenchmarkData& benchmark_data);





static std::vector<BenchmarkData> BenchmarkDispatchData = {
    {
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {1920}},         
            {"width", {1080}},
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KW", 3},
            {"KH", 3},
            {"HMAX", 1918},
            {"WMAX", 1078}
        },
        {                               // OTHER
            {"CHANNELS_OUT", 1},        
        },
        bench_wrapper_im2col,           // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

};




};














#endif