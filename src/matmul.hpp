#ifndef MATMUL_HPP
#define MATMUL_HPP
#include <random>
#include <chrono>
#include <cstring>

#define TYPE float
#define NEON_ALIGNMENT 16
#define THRESHOLD threshold

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


#define BENCH_INT(func) \
do { \
    init_data_int(A, B, C, DIMENSION); \
    auto start = std::chrono::high_resolution_clock::now(); \
    func; \
    auto end = std::chrono::high_resolution_clock::now(); \
    double elapsed = std::chrono::duration<double>(end - start).count(); \
    double checksum = print_checksum((float*)C, DIMENSION); \
    printf("%.12s  secs: %.6f  chsum: %.6f\n", #func, elapsed, checksum); \
} while (0)



double print_checksum_l(float* C, int length);
double print_checksum_float(float *C, int n);
void zero_matrix_int(int* matrix, int dimension);
void copy_matrix_int(int* src, int* dst, int dimension);
void init_data_int(int *A, int *B, int *C, int dimension);
double print_checksum_int(int* C, int dimention);
void transpose_naive_int(int *src, int *dst, int src_row, int src_col);
void matmult_opt3_transposed_int(int *A, int *B, int *C, int dimension);
void matmult_dtl_transposed_int(int *A, int *B, int *C, int dimension);
void init_bank_aware_transpose_int(uint64_t base, int* write_out, int dimension);


void matmult_conv_blocked(float* A, float* B, float* C, int M, int K, int N);


void zero_matrix(float* matrix, int dimension);

void init_bank_aware_transpose(uint64_t base, float* write_out, int dimension);

void copy_matrix(float* src, float* dst, int dimension);

double timestamp();

void init_data(float *A, float *B, float *C, int dimension);

double print_checksum(float *C, int dimention);

void matmult_opt0_naive(float *A, float *B, float *C, int dimension);

void matmult_opt1_jk(float *A, float *B, float *C, int dimension);

void matmult_opt2_jk_tiling(float *A, float *B, float *C, int dimension);

void transpose_naive(float *src, float *dst, int src_row, int src_col);

void matmult_opt3_transposed(float *A, float *B, float *C, float** Bt, int dimension);
void matmult_opt3_pretransposed_int(int *A, int* bt, int *C, int dimension);

void matmult_dtl_transposed(float *A, float *B, float *C, int dimension);

void matmult_opt3_pretransposed(float *A, float* bt, float *C, int dimension);

void matmult_dtl_transposed_tile(size_t size, size_t tile_size, size_t inner_tile_size, float* a, float*  b, float*  c);
void matmul_opt4_recursive(TYPE*  a, TYPE*  b, TYPE* c, const size_t size, size_t threshold);
void matmul_opt5_recursive_pretranspose(TYPE* a, TYPE*  b, TYPE* c, size_t size, size_t threshold);

void hadamard(int* out, int* a, int* b, int height, int width);

void hadamard_tensor_4d_int(int* out, int* a, int* b, int d4, int d3, int d2, int d1);
#endif