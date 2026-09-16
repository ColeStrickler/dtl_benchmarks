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
#include "sparse.hpp"
#include "sort.hpp"


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


std::string bench_wrapper_zipStruct(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_ecs(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_imageAugmentation(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_imageAugmentation_nchw(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_imageAugmentation_fuse(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_im2col(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_vol2col(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_db_colproject(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_matmul_transpose(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_nhwc_permutation(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_batch2space(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_tensorunfold_mode1(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_tensorunfold_mode2(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_tensorunfold_mode3(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_tensorunfold_mode4(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_tensorslice(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_highdim_stencil(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_multigrid(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_haar_wavelet(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_cubestencil(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_SoA(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_multithread_db(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_multithread_SoA(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_SpMV(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_wrapper_sparseAttention(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_ShadowPA_overhead(const BenchmarkData& bench_data, DTL::API* api);
std::string bench_recordSort(const BenchmarkData& bench_data, DTL::API* api);




std::string InsertDTLConfigParameters(const BenchmarkData& benchmark_data);
std::string CreateConstants(const std::unordered_map<std::string, BenchParam>& constants);
std::string CreateConstantArray(const std::unordered_map<std::string, BenchParam> &constantArray);
std::string CreateBenchmarkConfig(const BenchmarkData& benchmark_data);
std::string CreateBenchmarkConfig2(BenchmarkData benchmark_data);




static std::vector<BenchmarkData> transposeData = {
    {
        "matmul_transpose",             //  Benchmark
        {                               // CONSTANTS
            {"row_size", {80}},         
            {"col_size", {1}},
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NROWS", 80},
            {"NCOLS", 80}                 
        },
        {                               // OTHER
    
        },
        bench_wrapper_matmul_transpose, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },



    {
        "matmul_transpose",             //  Benchmark
        {                               // CONSTANTS
            {"row_size", {160}},         
            {"col_size", {1}},
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NROWS", 160},
            {"NCOLS", 160}                 
        },
        {                               // OTHER
    
        },
        bench_wrapper_matmul_transpose, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },



    {
        "matmul_transpose",             //  Benchmark
        {                               // CONSTANTS
            {"row_size", {320}},         
            {"col_size", {1}},
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NROWS", 320},
            {"NCOLS", 320}                 
        },
        {                               // OTHER
    
        },
        bench_wrapper_matmul_transpose, // bench function
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



static std::vector<BenchmarkData> sortData = {
        {
        "sort",               //  Benchmark
        {                               // CONSTANTS
  
        },
        {

        },
        {                               // LOOP PARAMETERS
          
        },
        {                               // OTHER

        },
        bench_recordSort,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

};



static std::vector<BenchmarkData> sparseData = {

//    {
//        "sparse_attention",               //  Benchmark
//        {                               // CONSTANTS
//            {"M", {1024}},                  // query sequence length
//            {"N", {1024}},                  // key/value sequence length
//            {"D", {64}},                    // Q/K head dimension
//            {"DV", {64}},                   // V/output head dimension
//            {"nnz_per_row", {32}}           // sparse attention width
//        },
//        {
//
//        },
//        {                               // LOOP PARAMETERS
//          
//        },
//        {                               // OTHER
//
//        },
//        bench_wrapper_sparseAttention,    // bench function
//        "",                             // artifact in
//        ""                              // artifact out
//    },
//
//    {
//        "sparse_attention",               //  Benchmark
//        {                               // CONSTANTS
//            {"M", {256}},                  // query sequence length
//            {"N", {65536}},                  // key/value sequence length
//            {"D", {64}},                    // Q/K head dimension
//            {"DV", {64}},                   // V/output head dimension
//            {"nnz_per_row", {32}}           // sparse attention width
//        },
//        {
//
//        },
//        {                               // LOOP PARAMETERS
//          
//        },
//        {                               // OTHER
//
//        },
//        bench_wrapper_sparseAttention,    // bench function
//        "",                             // artifact in
//        ""                              // artifact out
//    },
//    {
//        "sparse_attention",               //  Benchmark
//        {                               // CONSTANTS
//            {"M", {256}},                  // query sequence length
//            {"N", {65536}},                  // key/value sequence length
//            {"D", {64}},                    // Q/K head dimension
//            {"DV", {64}},                   // V/output head dimension
//            {"nnz_per_row", {512}}           // sparse attention width
//        },
//        {
//
//        },
//        {                               // LOOP PARAMETERS
//          
//        },
//        {                               // OTHER
//
//        },
//        bench_wrapper_sparseAttention,    // bench function
//        "",                             // artifact in
//        ""                              // artifact out
//    },
//    {
//        "sparse_attention",               //  Benchmark
//        {                               // CONSTANTS
//            {"M", {256}},                  // query sequence length
//            {"N", {65536}},                  // key/value sequence length
//            {"D", {64}},                    // Q/K head dimension
//            {"DV", {64}},                   // V/output head dimension
//            {"nnz_per_row", {1024}}           // sparse attention width
//        },
//        {
//
//        },
//        {                               // LOOP PARAMETERS
//          
//        },
//        {                               // OTHER
//
//        },
//        bench_wrapper_sparseAttention,    // bench function
//        "",                             // artifact in
//        ""                              // artifact out
//    },
//    {
//        "sparse_attention",               //  Benchmark
//        {                               // CONSTANTS
//            {"M", {256}},                  // query sequence length
//            {"N", {65536}},                  // key/value sequence length
//            {"D", {64}},                    // Q/K head dimension
//            {"DV", {64}},                   // V/output head dimension
//            {"nnz_per_row", {2048}}           // sparse attention width
//        },
//        {
//
//        },
//        {                               // LOOP PARAMETERS
//          
//        },
//        {                               // OTHER
//
//        },
//        bench_wrapper_sparseAttention,    // bench function
//        "",                             // artifact in
//        ""                              // artifact out
//    },
//    {
//        "sparse_attention",               //  Benchmark
//        {                               // CONSTANTS
//            {"M", {256}},                  // query sequence length
//            {"N", {65536}},                  // key/value sequence length
//            {"D", {64}},                    // Q/K head dimension
//            {"DV", {64}},                   // V/output head dimension
//            {"nnz_per_row", {4096}}           // sparse attention width
//        },
//        {
//
//        },
//        {                               // LOOP PARAMETERS
//          
//        },
//        {                               // OTHER
//
//        },
//        bench_wrapper_sparseAttention,    // bench function
//        "",                             // artifact in
//        ""                              // artifact out
//    },


    {
        "sparse_matrix_vector",               //  Benchmark
        {                               // CONSTANTS
            {"rows", {1024}},
            {"cols", {1024}},         
        },
        {

        },
        {                               // LOOP PARAMETERS
          
        },
        {                               // OTHER
            {"MAX_NNZ", 1024/2},
            {"MIN_NNZ", 1024/2}
        },
        bench_wrapper_SpMV,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },



    {
        "sparse_matrix_vector",               //  Benchmark
        {                               // CONSTANTS
            {"rows", {262144}},
            {"cols", {1048576}},         
        },
        {

        },
        {                               // LOOP PARAMETERS
          
        },
        {                               // OTHER
            {"MAX_NNZ", 16},
            {"MIN_NNZ", 16}
        },
        bench_wrapper_SpMV,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },




    //    {
    //    "sparse_matrix_vector",               //  Benchmark
    //    {                               // CONSTANTS
    //        {"rows", {1024*8}},
    //        {"cols", {1024*8}},         
    //    },
    //    {
//
    //    },
    //    {                               // LOOP PARAMETERS
    //      
    //    },
    //    {                               // OTHER
    //        {"MAX_NNZ", 1024*8/2},
    //        {"MIN_NNZ", 1024*8/2}
    //    },
    //    bench_wrapper_SpMV,    // bench function
    //    "",                             // artifact in
    //    ""                              // artifact out
    //},

    //{
    //    "sparse_matrix_vector",               //  Benchmark
    //    {                               // CONSTANTS
    //        {"rows", {1024}},
    //        {"cols", {1024}},         
    //    },
    //    {
//
    //    },
    //    {                               // LOOP PARAMETERS
    //            
    //    },
    //    {                               // OTHER
    //        {"MAX_NNZ", 1024/16},
    //        {"MIN_NNZ", 1024/16}
    //    },
    //    bench_wrapper_SpMV,    // bench function
    //    "",                             // artifact in
    //    ""                              // artifact out
    //},
//
    //{
    //    "sparse_matrix_vector",             //  Benchmark
    //    {                                   // CONSTANTS
    //        {"rows", {1024}},
    //        {"cols", {1024}},         
    //    },
    //    {
//
    //    },
    //    {                               // LOOP PARAMETERS
    //        
    //    },
    //    {                               // OTHER
    //        {"MAX_NNZ", 1024/32},
    //        {"MIN_NNZ", 1024/32}
    //    },
    //    bench_wrapper_SpMV,    // bench function
    //    "",                             // artifact in
    //    ""                              // artifact out
    //},
};





static std::vector<BenchmarkData> vol2colData = {
    {
        "vol2col",               //  Benchmark
        {                               // CONSTANTS
            {"depth", {16}},
            {"height", {128}},
            {"width", {128}},         
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KT", 2},
            {"KH", 2},
            {"KW", 2},
            {"CDEPTH", 16-2+1},
            {"CHEIGHT", 128-2+1},
            {"CWIDTH", 128-2+1},
        },
        {                               // OTHER
            
        },
        bench_wrapper_vol2col,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },


     {
        "vol2col",               //  Benchmark
        {                               // CONSTANTS
            {"depth", {16}},
            {"height", {128}},
            {"width", {128}},         
        },
        {
  
        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KT", 3},
            {"KH", 3},
            {"KW", 3},
            {"CDEPTH", 16-3+1},
            {"CHEIGHT", 128-3+1},
            {"CWIDTH", 128-3+1},
        },
        {                               // OTHER
            
        },
        bench_wrapper_vol2col,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },


        {
        "vol2col",               //  Benchmark
        {                               // CONSTANTS
            {"depth", {16}},
            {"height", {128}},
            {"width", {128}},         
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KT", 4},
            {"KH", 4},
            {"KW", 4},
            {"CDEPTH", 16-4+1},
            {"CHEIGHT", 128-4+1},
            {"CWIDTH", 128-4+1},
        },
        {                               // OTHER
            
        },
        bench_wrapper_vol2col,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
};





static std::vector<BenchmarkData> colProjData = {
    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {16*8}},         
            
        },
        {
            {"col_offsets", {0}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 1},
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
            {"row_size", {16*8}},         
            
        },
        {
            {"col_offsets", {0,16}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 2},
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
            {"row_size", {16*8}},         
            
        },
        {
            {"col_offsets", {0,16,32}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
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
            {"row_size", {16*8}},         
            
        },
        {
            {"col_offsets", {0,16,32,48}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 4},
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
            {"row_size", {16*8}},         
            
        },
        {
            {"col_offsets", {0,16,32,48,64}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 5},
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
            {"row_size", {16*8}},         
            
        },
        {
            {"col_offsets", {0,16,32,48,64,80}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 6},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        "" 
    },

        {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {16*8}},         
            
        },
        {
            {"col_offsets", {0,16,32,48,64,80,96}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 7},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        "" 
    },



    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {16*8}},         
            
        },
        {
            {"col_offsets", {0,16,32,48,64,80,96,112}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 8},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        "" 
    },

};



static std::vector<BenchmarkData> colProjData2 = {
    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {16*4}},         
            
        },
        {
            {"col_offsets", {0}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 1},
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
            {"row_size", {16*4}},         
            
        },
        {
            {"col_offsets", {0,8}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 2},
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
            {"row_size", {16*4}},         
            
        },
        {
            {"col_offsets", {0,8,16}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
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
            {"row_size", {16*4}},         
            
        },
        {
            {"col_offsets", {0,8,16,24}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 4},
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
            {"row_size", {16*4}},         
            
        },
        {
            {"col_offsets", {0,8,16,24,32}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 5},
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
            {"row_size", {16*4}},         
            
        },
        {
            {"col_offsets", {0,8,16,24,32,40}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 6},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        "" 
    },

        {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {16*4}},         
            
        },
        {
            {"col_offsets", {0,8,16,24,32,40,48}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 7},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        "" 
    },



    {
        "db_col_project",               //  Benchmark
        {                               // CONSTANTS
            {"row_size", {16*4}},         
            
        },
        {
            {"col_offsets", {0,8,16,24,32,40,48,56}},
        },
        {                               // LOOP PARAMETERS
            {"ROWS", 83690},            
            {"COLUMNS", 8},
        },
        {                               // OTHER
  
        },
        bench_wrapper_db_colproject,    // bench function
        "",                             // artifact in
        "" 
    },

};







static std::vector<BenchmarkData> unfoldData = {
    {
        "tensor_unfold1",                //  Benchmark
        {                               // CONSTANTS
            {"stride_d1", {32}},
            {"stride_d2", {32*32}},
            {"stride_d3", {32*32*32}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"D1", 32},            
            {"D2", 32},
            {"D3", 32},
            {"D4", 32}
        },
        {                               // OTHER

        },
        bench_wrapper_tensorunfold_mode1,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },


    {
        "tensor_unfold2",                //  Benchmark
        {                               // CONSTANTS
            {"stride_d1", {32}},
            {"stride_d2", {32*32}},
            {"stride_d3", {32*32*32}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"D1", 32},            
            {"D2", 32},
            {"D3", 32},
            {"D4", 32}
        },
        {                               // OTHER

        },
        bench_wrapper_tensorunfold_mode2,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

        {
        "tensor_unfold3",                //  Benchmark
        {                               // CONSTANTS
            {"stride_d1", {32}},
            {"stride_d2", {32*32}},
            {"stride_d3", {32*32*32}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"D1", 32},            
            {"D2", 32},
            {"D3", 32},
            {"D4", 32}
        },
        {                               // OTHER

        },
        bench_wrapper_tensorunfold_mode3,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },


    {
        "tensor_unfold3",                //  Benchmark
        {                               // CONSTANTS
            {"stride_d1", {32}},
            {"stride_d2", {32*32}},
            {"stride_d3", {32*32*32}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"D1", 32},            
            {"D2", 32},
            {"D3", 32},
            {"D4", 32}
        },
        {                               // OTHER

        },
        bench_wrapper_tensorunfold_mode4,    // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    
  //      {
  //      "tensor_unfold1",                //  Benchmark
  //      {                               // CONSTANTS
  //          {"stride_d1", {8}},
  //          {"stride_d2", {8*64}},
  //          {"stride_d3", {8*64*64}},
  //          {"data_size", {4}}
  //      },
  //      {
//
  //      },
  //      {                               // LOOP PARAMETERS
  //          {"D1", 8},            
  //          {"D2", 64},
  //          {"D3", 64},
  //          {"D4", 128}
  //      },
  //      {                               // OTHER
//
  //      },
  //      bench_wrapper_tensorunfold_mode1,    // bench function
  //      "",                             // artifact in
  //      ""                              // artifact out
  //  },
//
//
  //  {
  //      "tensor_unfold2",                //  Benchmark
  //      {                               // CONSTANTS
  //          {"stride_d1", {8}},
  //          {"stride_d2", {8*64}},
  //          {"stride_d3", {8*64*64}},
  //          {"data_size", {4}}
  //      },
  //      {
//
  //      },
  //      {                               // LOOP PARAMETERS
  //          {"D1", 8},            
  //          {"D2", 64},
  //          {"D3", 64},
  //          {"D4", 128}
  //      },
  //      {                               // OTHER
//
  //      },
  //      bench_wrapper_tensorunfold_mode2,    // bench function
  //      "",                             // artifact in
  //      ""                              // artifact out
  //  },
//
  //      {
  //      "tensor_unfold3",                //  Benchmark
  //      {                               // CONSTANTS
  //          {"stride_d1", {8}},
  //          {"stride_d2", {8*64}},
  //          {"stride_d3", {8*64*64}},
  //          {"data_size", {4}}
  //      },
  //      {
//
  //      },
  //      {                               // LOOP PARAMETERS
  //          {"D1", 8},            
  //          {"D2", 64},
  //          {"D3", 64},
  //          {"D4", 128}
  //      },
  //      {                               // OTHER
//
  //      },
  //      bench_wrapper_tensorunfold_mode3,    // bench function
  //      "",                             // artifact in
  //      ""                              // artifact out
  //  },
//
//
  //  {
  //      "tensor_unfold4",                //  Benchmark
  //      {                               // CONSTANTS
  //          {"stride_d1", {8}},
  //          {"stride_d2", {8*64}},
  //          {"stride_d3", {8*64*64}},
  //          {"data_size", {4}}
  //      },
  //      {
//
  //      },
  //      {                               // LOOP PARAMETERS
  //          {"D1", 8},            
  //          {"D2", 64},
  //          {"D3", 64},
  //          {"D4", 128}
  //      },
  //      {                               // OTHER
//
  //      },
  //      bench_wrapper_tensorunfold_mode4,    // bench function
  //      "",                             // artifact in
  //      ""                              // artifact out
  //  },


    

};








static std::vector<BenchmarkData> slicingData2 = {
    {
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
            {"stride_w1", {2}},
            {"stride_c1", {4}},
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
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
            {"stride_w1", {2}},
            {"stride_c1", {8}},
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
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
            {"stride_w1", {2}},
            {"stride_c1", {16}},
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
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
            {"stride_w1", {2}},
            {"stride_c1", {32}},
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
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
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
};




static std::vector<BenchmarkData> slicingData = {
    {
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
            {"stride_w1", {1}},
            {"stride_c1", {4}},
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
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
            {"stride_w1", {1}},
            {"stride_c1", {8}},
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
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
            {"stride_w1", {1}},
            {"stride_c1", {16}},
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
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
            {"stride_w1", {1}},
            {"stride_c1", {32}},
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
        "tensor_slicing",                //  Benchmark
        {                               // CONSTANTS
            {"stride_n1", {1}},
            {"stride_h1", {1}},
            {"stride_w1", {1}},
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
};




static std::vector<BenchmarkData> im2colData = {
    {
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {720}},         
            {"width", {1080}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KW", 2},
            {"KH", 2},
            {"HMAX", 719},
            {"WMAX", 1079}
        },
        {                               // OTHER
            {"CHANNELS_OUT", 1},        
        },
        bench_wrapper_im2col,           // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {720}},         
            {"width", {1080}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KW", 3},
            {"KH", 3},
            {"HMAX", 718},
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
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {720}},         
            {"width", {1080}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KW", 4},
            {"KH", 4},
            {"HMAX", 717},
            {"WMAX", 1077}
        },
        {                               // OTHER
            {"CHANNELS_OUT", 1},        
        },
        bench_wrapper_im2col,           // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {720}},         
            {"width", {1080}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KW", 5},
            {"KH", 5},
            {"HMAX", 716},
            {"WMAX", 1076}
        },
        {                               // OTHER
            {"CHANNELS_OUT", 1},        
        },
        bench_wrapper_im2col,           // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    /////////////
     {
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {512}},         
            {"width", {512}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KW", 2},
            {"KH", 2},
            {"HMAX", 511},
            {"WMAX", 511}
        },
        {                               // OTHER
            {"CHANNELS_OUT", 1},        
        },
        bench_wrapper_im2col,           // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
     {
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {512}},         
            {"width", {512}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KW", 3},
            {"KH", 3},
            {"HMAX", 510},
            {"WMAX", 510}
        },
        {                               // OTHER
            {"CHANNELS_OUT", 1},        
        },
        bench_wrapper_im2col,           // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

     {
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {512}},         
            {"width", {512}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KW", 4},
            {"KH", 4},
            {"HMAX", 509},
            {"WMAX", 509}
        },
        {                               // OTHER
            {"CHANNELS_OUT", 1},        
        },
        bench_wrapper_im2col,           // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

     {
        "im2col",                       //  Benchmark
        {                               // CONSTANTS
            {"height", {512}},         
            {"width", {512}},
            {"data_size", {4}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"CHANNELS", 3},            
            {"KW", 5},
            {"KH", 5},
            {"HMAX", 508},
            {"WMAX", 508}
        },
        {                               // OTHER
            {"CHANNELS_OUT", 1},        
        },
        bench_wrapper_im2col,           // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
};


static std::vector<BenchmarkData> ImgAugmentationData_nchw = {
    {
        "img_augmentation",             //  Benchmark
        {                               // CONSTANTS
            {"H", {512}},         
            {"W", {512}},            // will assume square image for this one
            {"stride_n", {3*512*512}},
            {"stride_c", {512*512}},
            {"stride_h", {512}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 4},
            {"CMAX", 3},
            {"WMAX", 512},
            {"HMAX", 512}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_imageAugmentation_nchw, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },
    {
        "img_augmentation",             //  Benchmark
        {                               // CONSTANTS
            {"H", {512}},         
            {"W", {512}},            // will assume square image for this one
            {"stride_n", {3*512*512}},
            {"stride_c", {512*512}},
            {"stride_h", {512}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 16},
            {"CMAX", 3},
            {"WMAX", 512},
            {"HMAX", 512}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_imageAugmentation_nchw, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "img_augmentation",             //  Benchmark
        {                               // CONSTANTS
            {"H", {512}},         
            {"W", {512}},            // will assume square image for this one
            {"stride_n", {3*512*512}},
            {"stride_c", {512*512}},
            {"stride_h", {512}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 32},
            {"CMAX", 3},
            {"WMAX", 512},
            {"HMAX", 512}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_imageAugmentation_nchw, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

};







static std::vector<BenchmarkData> ImgAugmentationData = {
    {
        "img_augmentation",             //  Benchmark
        {                               // CONSTANTS
            {"H", {128}},         
            {"W", {128}},            // will assume square image for this one
            {"stride_n", {3*128*128}},
            {"stride_h", {3*128}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 4},
            {"CMAX", 3},
            {"WMAX", 128},
            {"HMAX", 128}            
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
            {"H", {128}},         
            {"W", {128}},            // will assume square image for this one
            {"stride_n", {3*128*128}},
            {"stride_h", {3*128}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 8},
            {"CMAX", 3},
            {"WMAX", 128},
            {"HMAX", 128}            
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
            {"H", {128}},         
            {"W", {128}},            // will assume square image for this one
            {"stride_n", {3*128*128}},
            {"stride_h", {3*128}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 16},
            {"CMAX", 3},
            {"WMAX", 128},
            {"HMAX", 128}            
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
            {"H", {128}},         
            {"W", {128}},            // will assume square image for this one
            {"stride_n", {3*128*128}},
            {"stride_h", {3*128}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 32},
            {"CMAX", 3},
            {"WMAX", 128},
            {"HMAX", 128}            
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
            {"H", {128}},         
            {"W", {128}},            // will assume square image for this one
            {"stride_n", {3*128*128}},
            {"stride_h", {3*128}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 64},
            {"CMAX", 3},
            {"WMAX", 128},
            {"HMAX", 128}            
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
            {"H", {256}},         
            {"W", {256}},            // will assume square image for this one
            {"stride_n", {3*256*256}},
            {"stride_h", {3*256}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 4},
            {"CMAX", 3},
            {"WMAX", 256},
            {"HMAX", 256}            
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
            {"H", {256}},         
            {"W", {256}},            // will assume square image for this one
            {"stride_n", {3*256*256}},
            {"stride_h", {3*256}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 8},
            {"CMAX", 3},
            {"WMAX", 256},
            {"HMAX", 256}            
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
            {"H", {256}},         
            {"W", {256}},            // will assume square image for this one
            {"stride_n", {3*256*256}},
            {"stride_h", {3*256}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 16},
            {"CMAX", 3},
            {"WMAX", 256},
            {"HMAX", 256}            
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
            {"H", {256}},         
            {"W", {256}},            // will assume square image for this one
            {"stride_n", {3*256*256}},
            {"stride_h", {3*256}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 32},
            {"CMAX", 3},
            {"WMAX", 256},
            {"HMAX", 256}            
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
            {"H", {256}},         
            {"W", {256}},            // will assume square image for this one
            {"stride_n", {3*256*256}},
            {"stride_h", {3*256}},
            {"stride_w", {3}}
        },
        {

        },
        {                               // LOOP PARAMETERS
            {"NMAX", 64},
            {"CMAX", 3},
            {"WMAX", 256},
            {"HMAX", 256}            
        },
        {                               // OTHER
            {"use_real_image", 0},
            {"ksize", 2}
        },
        bench_wrapper_imageAugmentation, // bench function
        "",                             // artifact in
        ""                              // artifact out
    },



};





static std::vector<BenchmarkData> TMEComparisonData = {
    {
        "aos_to_soa",                   //  Benchmark
        {                                   // CONSTANTS
            {"struct_size", {6*16}}
        },
        {
            {"var_offsets", {0,16,32,48,64,80}}
        },
        {                               // LOOP PARAMETERS
            {"NSTRUCT", 1000},            
            {"NOFFSETS", 6},
        },
        {                               // OTHER

        },
        bench_wrapper_ecs,              // bench function
        "",                             // artifact in
        ""                              // artifact out
    },

    {
        "aos_to_soa",                   //  Benchmark
        {                                   // CONSTANTS
            {"struct_size", {6*16}}
        },
        {
            {"var_offsets", {0,8,16,24,32,40,48,56,64,72,80}}
        },
        {                               // LOOP PARAMETERS
            {"NSTRUCT", 1000},            
            {"NOFFSETS", 12},
        },
        {                               // OTHER

        },
        bench_wrapper_ecs,              // bench function
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
            {"BATCH_SIZE", 32},
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
        "batch2space",             //  Benchmark
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
            {"HEIGHT", 256},
            {"WIDTH", 256}            
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



    //{
    //    "db_col_project",               //  Benchmark
    //    {                               // CONSTANTS
    //        {"row_size", {512}},         
    //        {"col_offsets", {104, 256, 444}},
    //    },
    //    {
//
    //    },
    //    {                               // LOOP PARAMETERS
    //        {"ROWS", 349520},            
    //        {"COLUMNS", 3},
    //    },
    //    {                               // OTHER
  //
    //    },
    //    bench_wrapper_db_colproject,    // bench function
    //    "",                             // artifact in
    //    ""                              // artifact out
    //},
    //{
    //    "db_col_project",               //  Benchmark
    //    {                               // CONSTANTS
    //        {"row_size", {512}},         
    //        {"col_offsets", {4, 64, 120, 164, 256, 312, 368, 400, 444, 488, 500}},
    //    },
    //    {
//
    //    },
    //    {                               // LOOP PARAMETERS
    //        {"ROWS", 349520},            
    //        {"COLUMNS", 11},
    //    },
    //    {                               // OTHER
//
    //    },
    //    bench_wrapper_db_colproject,    // bench function
    //    "",                             // artifact in
    //    ""                              // artifact out
    //},

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


};




/*
    Maybe later we can set up a data element size. for now it is always assumed element_size=4bytes
*/

static std::vector<BenchmarkData> BenchmarkDispatchData = {
  
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