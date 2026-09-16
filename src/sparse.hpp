#ifndef SPARSE_HPP
#define SPARSE_HPP
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <random>

#include "stdint.h"

void spmv_csr(
    int nrows,
    const int* __restrict row_ptr,
    const int* __restrict col_idx,
    const float* __restrict values,
    const float* __restrict x,
    float* __restrict y) noexcept;

void spmv_dtu(
    int nrows,
    const int* __restrict row_ptr,
    const float* __restrict values,
    const float* __restrict x_dtu,
    float* __restrict y) noexcept;


int generate_csr_random_rows(
    int nrows,
    int ncols,
    int min_nnz_per_row,
    int max_nnz_per_row,
    int* row_ptr,
    int* col_idx,
    float* values,
    uint32_t seed);


void generate_vector(int ncols, float *x, uint32_t seed);

struct SparseAttentionBuffers {
    int M;
    int N;
    int D;
    int DV;
    int nnz;

    float* Q;              // [M * D]
    float* K;              // [N * D]
    float* V;              // [N * DV]

    int* row_ptr;          // [M + 1]
    int* col_idx;          // [nnz]

    float* scores;         // [nnz]
    float* O;              // [M * DV]
};

SparseAttentionBuffers generate_sparse_attention_buffers(
    int M,
    int N,
    int D,
    int DV,
    int nonzeros_per_row,
    bool causal,
    uint32_t seed);
void free_sparse_attention_buffers(
    SparseAttentionBuffers* b);

/*
    The DTU is doing this
*/
// K_dtu[p * D  + d]  = K[col_idx[p] * D  + d]
// V_dtu[p * DV + d]  = V[col_idx[p] * DV + d]
void sparse_attention(
    int M,
    int D,
    int DV,
    const float* Q,
    const float* K,
    const float* V,
    const int* row_ptr,
    const int* col_idx,
    float* scores,
    float* O);

void sparse_attention_dtu(
    int M,
    int D,
    int DV,
    const float* Q,
    const float* K_dtu,
    const int* row_ptr,
    const float* dtu_V,
    float* scores,
    float* O);

#endif