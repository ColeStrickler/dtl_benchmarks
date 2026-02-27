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
#include "highdim_stencil.hpp"
#include "cube_stencil.hpp"
#include "bijective.hpp"

namespace benchmark
{

typedef std::vector<uint32_t> BenchParam;

// Declare function pointer type first
using BenchFn = std::string(*)(const struct BenchmarkData&, DTL::API*);

struct BenchmarkData
{
    std::string name;
    std::unordered_map<std::string, BenchParam> constants;
    std::unordered_map<std::string, BenchParam> constArray;
    std::unordered_map<std::string, uint32_t> params;
    std::unordered_map<std::string, uint32_t> other;
    BenchFn benchmark;            // Now this is valid
    std::string input_artifact;
    std::string output_artifact;
};




std::string bench_wrapper_db_filterselect(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_db_filterselect5(const BenchmarkData& bench_data, DTL::API* api);


std::string bench_wrapper_imageAugmentation(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_imageAugmentation_fuse(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_im2col(const BenchmarkData& bench_data, DTL::API* api);
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
std::string CreateConstantArray(const std::unordered_map<std::string, BenchParam> &constantArray);
std::string CreateBenchmarkConfig(const BenchmarkData& benchmark_data);
std::string CreateBenchmarkConfig2(BenchmarkData benchmark_data);


static std::vector<BenchmarkData> BenchmarkDispatchDataSplitLayouts = {

    {
        "db_col_project",             //  Benchmark
        {                               // CONSTANTS
            {"row_size", {16}},
            {"filter_col_offsets", {5, 6, 10, 14}},
            {"selection_col_offsets", {3, 8, 10, 15}},         
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"ROWS", 43690},            
            {"COLUMNS", 4},
        },
        {

        },
        bench_wrapper_db_filterselect5,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },



    {
        "db_col_project",             //  Benchmark
        {                               // CONSTANTS
            {"row_size", {16}},
            {"filter_col_offsets", {5, 6, 10, 14}},
            {"selection_col_offsets", {3, 8, 10, 15}},         
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"ROWS", 43690},            
            {"COLUMNS", 4},
        },
        {

        },
        bench_wrapper_db_filterselect,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    //    {
    //    "im2col",                       //  Benchmark
    //    {                               // CONSTANTS
    //        {"height", {1024}},         
    //        {"width", {1024}},
    //        {"data_size", {4}}
    //    },
    //    {                               // LOOP PARAMETERS
    //        {"CHANNELS", 1},            
    //        {"KW", 2},
    //        {"KH", 2},
    //        {"HMAX", 1919},
    //        {"WMAX", 1079}
    //    },
    //    {                               // OTHER
    //        {"CHANNELS_OUT", 1},        
    //    },
    //    bench_wrapper_im2col,           // bench function
    //    "",                             // artifact in
    //    ""                              // artifact out
    //},
};


static std::vector<BenchmarkData> TMEComparisonData = {
    {
        "img_augmentation",             //  Benchmark
        {                               // CONSTANTS
            {"H", {512}},         
            {"W", {512}},            // will assume square image for this one
            {"stride_n", {3*512*512}},
            {"stride_h", {3*512}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 8},
            {"CMAX", 3},
            {"WMAX", 512},
            {"HMAX", 512}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_imageAugmentation, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

     {
        "img_augmentation",             //  Benchmark
        {                               // CONSTANTS
            {"H", {512}},         
            {"W", {512}},            // will assume square image for this one
            {"stride_n", {3*512*512}},
            {"stride_h", {3*512}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 8},
            {"CMAX", 3},
            {"WMAX", 512},
            {"HMAX", 512}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_imageAugmentation_fuse, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },


    {
        "nhwc_permutation",             //  Benchmark
        {                               // CONSTANTS
            {"channels", {3}},         
            {"size", {512}},            // will assume square image for this one
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"BATCH_SIZE", 8},
            {"CHANNELS", 3},
            {"SIZE_SQUARED", 512*512}            
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
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {1024}},         
            {"width", {1024}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 1},            
            {"KW", 2},
            {"KH", 2},
            {"HMAX", 1023},
            {"WMAX", 1023}
        },
        {                               // OTHER
            {"CHANNELS_OUT", 1},        
        },
        bench_wrapper_im2col,           // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "tensor_unfold",                //  Benchmark
        {                               // CONSTANTS
            {"stride_d1", {8}},
            {"stride_d2", {8*64}},
            {"stride_d3", {8*64*64}},
            {"data_size", {4}}
        },
        {

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
        "batch2space",             //  Benchmark
        {                               // CONSTANTS
            {"channels", {3}},         
            {"size", {64}},            // will assume square image for this one
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"BATCH_SIZE", 8},
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
        "tensor_slicing",                   //  Benchmark
        {                                   // CONSTANTS
            {"stride_n1", {2}},
            {"stride_h1", {4}},
            {"stride_w1", {2}},
            {"stride_c1", {64}},
            {"stride_d1", {512}},
            {"stride_d2", {512*64}},
            {"stride_d3", {512*64*64}},
            {"data_size", {4}}
        },
        {

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
        "matmul_transpose",             //  Benchmark
        {                               // CONSTANTS
            {"row_size", {640}},         
            {"col_size", {1}},
        },
        {

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



};




/*
    Maybe later we can set up a data element size. for now it is always assumed element_size=4bytes
*/

static std::vector<BenchmarkData> BenchmarkDispatchData = {
    {
        "filter_select_db",             //  Benchmark
        {                               // CONSTANTS
            {"row_size", {16}},
            {"filter_col_offsets", {5, 6, 10, 14}},
            {"selection_col_offsets", {3, 8, 10, 15}},         
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"ROWS", 43690},            
            {"COLUMNS", 4},
        },
        {

        },
        bench_wrapper_db_filterselect,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "cube_stencil_8corner",         //  Benchmark
        {                               // CONSTANTS
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"N_3DSTRUCT", 64}, // 4th dimension
            {"NCUBES", 8},         // how many total cubes we get per 4th dim
            {"CUBE_DIM1", 3},
            {"CUBE_DIM2", 3},
            {"CUBE_DIM3", 3}
        },
        {                               // OTHER
            {"d4", 64},
            {"d3", 64},
            {"d2", 64},
            {"d1", 64},
            {"CUBE_DIM", 3},
        },
        bench_wrapper_cubestencil,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "tensor_unfold",                //  Benchmark
        {                               // CONSTANTS
            {"stride_d1", {8}},
            {"stride_d2", {8*64}},
            {"stride_d3", {8*64*64}},
            {"data_size", {4}}
        },
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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
        {

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

    {
        "highdim_7stencil",             //  Benchmark
        {                               // CONSTANTS
            {"stride_nx", {256*128}},
            {"stride_ny", {128}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NX_MINUS_1", 127},            
            {"NY_MINUS_1", 255},
            {"NZ_MINUS_1", 127},
        },
        {                               // OTHER

        },
        bench_wrapper_highdim_stencil,  // bench function
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
        {

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
        {

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
        {

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



};









};




#endif