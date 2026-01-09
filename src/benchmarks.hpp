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
#include "tensor_permutation.hpp"
#include "batch2space.hpp"
#include "unfold.hpp"
#include "slicing.hpp"

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
std::string bench_wrapper_tensorunfold(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_tensorslice(const BenchmarkData& bench_data, DTL::API* api);
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




/*
    Maybe later we can set up a data element size. for now it is always assumed element_size=4bytes
*/

static std::vector<BenchmarkData> BenchmarkDispatchData = {
        {
        "tensor_unfold",                //  Benchmark
        {                               // CONSTANTS
            {"stride_d1", {8}},
            {"stride_d2", {8*64}},
            {"stride_d3", {8*64*64}},
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"D1", 8},            
            {"D2", 64},
            {"D3", 64},
            {"D4", 128}
        },
        {                               // OTHER

        },
        bench_wrapper_tensorunfold,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {2}},
            {"stride_h1", {4}},
            {"stride_w1", {2}},
            {"stride_c1", {64}},
            {"stride_d1", {512}},
            {"stride_d2", {512*64}},
            {"stride_d3", {512*64*64}},
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"D1", 512},            
            {"D2", 64},
            {"D3", 64},
            {"D4", 8}
        },
        {                               // OTHER

        },
        bench_wrapper_tensorslice,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
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

    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {64}},         
            {"col_offsets", {4, 8, 16, 24, 28, 32, 40, 48, 52, 56, 60}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 43690},            
            {"COLUMNS", 11},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {64}},         
            {"col_offsets", {4, 32, 48}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 43690},            
            {"COLUMNS", 3},
        },
        {                               // OTHER
    
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {512}},         
            {"col_offsets", {104, 256, 444}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 43690},            
            {"COLUMNS", 3},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {512}},         
            {"col_offsets", {4, 64, 120, 164, 256, 312, 368, 400, 444, 488, 500}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 43690},            
            {"COLUMNS", 11},
        },
        {                               // OTHER

        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "matmul_transpose",             //  Benchmark
        {                               // CONSTANTS
            {"row_size", {2560}},         
            {"col_size", {4}},
        },
        {                               // LOOP PARAMETERS
            {"NROWS", 640},
            {"NCOLS", 640}                 
        },
        {                               // OTHER
    
        },
        bench_wrapper_matmul_transpose, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "nhwc_permutation",             //  Benchmark
        {                               // CONSTANTS
            {"channels", {3}},         
            {"size", {128}},            // will assume square image for this one
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"BATCH_SIZE", 16},
            {"CHANNELS", 3},
            {"SIZE_SQUARED", 128*128}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_nhwc_permutation, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "nhwc_permutation",             //  Benchmark
        {                               // CONSTANTS
            {"channels", {3}},         
            {"size", {64}},            // will assume square image for this one
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"BATCH_SIZE", 16},
            {"CHANNELS", 3},
            {"SIZE_SQUARED", 64*64}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_nhwc_permutation, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "nhwc_permutation",             //  Benchmark
        {                               // CONSTANTS
            {"channels", {3}},         
            {"size", {256}},            // will assume square image for this one
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"BATCH_SIZE", 16},
            {"CHANNELS", 3},
            {"SIZE_SQUARED", 256*256}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_nhwc_permutation, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "nhwc_permutation",             //  Benchmark
        {                               // CONSTANTS
            {"channels", {3}},         
            {"size", {256}},            // will assume square image for this one
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"BATCH_SIZE", 8},
            {"CHANNELS", 3},
            {"SIZE_SQUARED", 256*256}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_nhwc_permutation, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "batch2space",             //  Benchmark
        {                               // CONSTANTS
            {"channels", {3}},         
            {"size", {512}},            // will assume square image for this one
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"BATCH_SIZE", 8},
            {"CHANNELS", 3},
            {"HEIGHT", 512},
            {"WIDTH", 512}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_batch2space, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "batch2space",             //  Benchmark
        {                               // CONSTANTS
            {"channels", {3}},         
            {"size", {64}},            // will assume square image for this one
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"BATCH_SIZE", 16},
            {"CHANNELS", 3},
            {"HEIGHT", 64},
            {"WIDTH", 64}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_batch2space, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
        {
        "batch2space",             //  Benchmark
        {                               // CONSTANTS
            {"channels", {3}},         
            {"size", {64}},            // will assume square image for this one
            {"data_size", {4}}
        },
        {                               // LOOP PARAMETERS
            {"BATCH_SIZE", 128},
            {"CHANNELS", 3},
            {"HEIGHT", 64},
            {"WIDTH", 64}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_batch2space, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

};








static std::vector<BenchmarkData> BenchmarkDispatchDataLong = {
    {
        "matmul_transpose",             //  Benchmark
        {                               // CONSTANTS
            {"row_size", {8192}},         
            {"col_size", {4}},
        },
        {                               // LOOP PARAMETERS
            {"NROWS", 2048},
            {"NCOLS", 2048}            
        },
        {                               // OTHER
    
        },
        bench_wrapper_matmul_transpose, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

       {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {64}},         
            {"col_offsets", {4, 8, 16, 24, 28, 32, 40, 48, 52, 56, 60}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 442368},            
            {"COLUMNS", 11},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {64}},         
            {"col_offsets", {4, 32, 48}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 442368},            
            {"COLUMNS", 3},
        },
        {                               // OTHER
    
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {512}},         
            {"col_offsets", {104, 256, 444}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 442368},            
            {"COLUMNS", 3},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {512}},         
            {"col_offsets", {4, 64, 120, 164, 256, 312, 368, 400, 444, 488, 500}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 442368},            
            {"COLUMNS", 11},
        },
        {                               // OTHER

        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },



};









};




#endif