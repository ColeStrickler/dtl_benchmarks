#ifndef MATMUL_HPP
#define MATMUL_HPP
#include <random>
#include <chrono>
#include "dtl_api.hpp"



#define BENCH(func) \
do { \
    init_data(A, B, C, DIMENSION); \
    auto start = std::chrono::high_resolution_clock::now(); \
    func; \
    auto end = std::chrono::high_resolution_clock::now(); \
    double elapsed = std::chrono::duration<double>(end - start).count(); \
    double checksum = print_checksum(C, DIMENSION); \
    printf("%.12s  secs: %.6f  chsum: %.6f\n", #func, elapsed, checksum); \
} while (0)

double timestamp();

void init_data(float *A, float *B, float *C, int dimension);

double print_checksum(float *C, int dimention);

void matmult_opt0_naive(float *A, float *B, float *C, int dimension);

void matmult_opt1_jk(float *A, float *B, float *C, int dimension);

void matmult_opt2_jk_tiling(float *A, float *B, float *C, int dimension);

void transpose_naive(float *src, float *dst, int src_row, int src_col);

void matmult_opt3_transposed(float *A, float *B, float *C, float** Bt, int dimension);

void matmult_dtl_transposed(float *A, float *B, float *C, int dimension);

#endif