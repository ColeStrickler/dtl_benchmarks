#include "matmul.hpp"



void init_data(float *A, float *B, float *C, int dimension)
{
    int i, j, k;
    std::mt19937 rng(292); // fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
    for(i = 0; i < dimension; i++) {
        for(j = 0; j < dimension; j++) {
            A[dimension*i+j] = dist(rng);
            B[dimension*i+j] = dist(rng);
            C[dimension*i+j] = 0.0f;
            //printf("%f %f\n", A[dimension*i+j], B[dimension*i+j]);
        }
    }
}

double print_checksum(float *C, int dimention)
{
    double sum = 0.0;
    for(int i = 0; i < dimention; i++) {
        for(int j = 0; j < dimention; j++) {
            sum += C[i*dimention+j];
        }
    }
    return sum;
}



// a naive matrix multiplication implementation. 
void matmult_opt0_naive(float *A, float *B, float *C, int dimension)
{
    for(int i = 0; i < dimension; i++) {
        for(int j = 0; j < dimension; j++) {
            for(int k = 0; k < dimension; k++) {
                C[dimension*i+j] += (A[dimension*i+k] * B[dimension*k+j]);
            }
        }
    }	
}

// matrix multiplication with jk order switch
void matmult_opt1_jk(float *A, float *B, float *C, int dimension)
{
    for(int i = 0; i < dimension; i++) {
        for(int k = 0; k < dimension; k++) {
            for(int j = 0; j < dimension; j++) {
                C[dimension*i+j] += (A[dimension*i+k] * B[dimension*k+j]);
            }
        }
    }	
}

// matrix multiplication with jk order switch and tiling    
void matmult_opt2_jk_tiling(float *A, float *B, float *C, int dimension)
{
    int i,j,k,ii,jj,kk;
    int bs = 64; // block size = 32*32*4 = 4KB

    for(i = 0; i < dimension; i+=bs) {
        for(k = 0; k < dimension; k+=bs) {
            for(j = 0; j < dimension; j+=bs) {
                for(ii = i; ii < i+bs; ii++) {
                    for(kk = k; kk < k+bs; kk++) {
                        for(jj = j; jj < j+bs; jj++) {
                            C[dimension*ii+jj] += (A[dimension*ii+kk] * B[dimension*kk+jj]);
                        }
                    }
                }
            }
        }
    }
}   


// transpose matrix
void transpose_naive(float *src, float *dst, int src_row, int src_col)
// src: m(src_row) x n(src_col)  -> dst: n x m
{
    for (int i = 0; i < src_col; i++) {
        for (int j = 0; j < src_row; j++) {
            dst[i*src_row+j] = src[j*src_col+i];
        }
    }
}

// matrix multiplicaiton after transposed
void matmult_opt3_transposed(float *A, float *B, float *C, float** Bt, int dimension)
{
    int i,j,k;
    int alloc_size = dimension*dimension*sizeof(float);
    *Bt = (float*)malloc(alloc_size);

    float * bt = *Bt;
    transpose_naive(B, bt, dimension, dimension);

    for(i = 0; i < dimension; i++) {
        for(j = 0; j < dimension; j++) {
            for(k = 0; k < dimension; k++) {                            
                C[dimension*i+j] += (A[dimension*i+k] * bt[dimension*j+k]);
            }
        }
    }
    //free(Bt);
}


// matrix multiplicaiton after transposed
void matmult_dtl_transposed(float *A, float *B, float *C, int dimension)
{
    int i,j,k;
    int alloc_size = dimension*dimension*sizeof(float);
    //float *Bt = (float*)malloc(alloc_size);
    //transpose_naive(B, Bt, dimension, dimension);

    for(i = 0; i < dimension; i++) {
        for(j = 0; j < dimension; j++) {
            //printf("B [%d,%d]=%.3f\n", i, j, B[dimension*i+j]); 
            for(k = 0; k < dimension; k++) {                           
                C[dimension*i+j] += (A[dimension*i+k] * B[dimension*j+k]);
            }
        }
    }
   // free(Bt);
}