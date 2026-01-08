#include "matmul.hpp"



void init_bank_aware_transpose_int(uint64_t base, int* write_out, int dimension)
{
    int bank_stride = (1 << 13);
    int bank_count = 8;

    int o_row = 0;
    int o_col = 0;

    int row_size = 2560;
    int col_size = 4;


    for (int stride = 0; stride < 25; stride++)
    {
        for (int bank_element_count = 0; bank_element_count < 2048; bank_element_count++)
        {
            for (int bank = 0; bank < 8; bank++)
            {
                //printf("stride %d, bank_element_count %d, bank %d\n", stride, bank_element_count, bank);
                *(int*)(base + bank_element_count*col_size + bank*bank_stride + stride*(bank_count*bank_stride)) = write_out[dimension*o_row+o_col];
                o_row++;
                if (o_row == dimension)
                    o_col++;
                o_row %= dimension;
            }
        }

    }
}


void init_bank_aware_transpose(uint64_t base, float* write_out, int dimension)
{
    int bank_stride = (1 << 13);
    int bank_count = 8;

    int o_row = 0;
    int o_col = 0;

    int row_size = 2560;
    int col_size = 4;


    for (int stride = 0; stride < 25; stride++)
    {
        for (int bank_element_count = 0; bank_element_count < 2048; bank_element_count++)
        {
            for (int bank = 0; bank < 8; bank++)
            {
                //printf("stride %d, bank_element_count %d, bank %d\n", stride, bank_element_count, bank);
                *(float*)(base + bank_element_count*col_size + bank*bank_stride + stride*(bank_count*bank_stride)) = write_out[dimension*o_row+o_col];
                o_row++;
                if (o_row == dimension)
                    o_col++;
                o_row %= dimension;
            }
        }

    }
}


void zero_matrix(float* matrix, int dimension)
{
    for(int i = 0; i < dimension; i++) {
        for(int j = 0; j < dimension; j++) {
            matrix[dimension*i+j] = 0.0f;
            //printf("%f %f\n", A[dimension*i+j], B[dimension*i+j]);
        }
    }
}

double print_checksum_l(float *C, int length)
{
  double chsum = 0.0f;
  for (int i = 0; i < length; i++)
    chsum += C[i];

  return chsum;
}

void zero_matrix_int(int *matrix, int dimension) {
  for (int i = 0; i < dimension; i++) {
    for (int j = 0; j < dimension; j++) {
      matrix[dimension * i + j] = 0;
      // printf("%f %f\n", A[dimension*i+j], B[dimension*i+j]);
    }
  }
}

void copy_matrix(float* src, float* dst, int dimension)
{
    for(int i = 0; i < dimension; i++) {
        for(int j = 0; j < dimension; j++) {
            dst[dimension*i+j] = src[dimension*i+j];
            //printf("%f %f\n", A[dimension*i+j], B[dimension*i+j]);
        }
    }
}


void copy_matrix_int(int* src, int* dst, int dimension)
{
    for(int i = 0; i < dimension; i++) {
        for(int j = 0; j < dimension; j++) {
            dst[dimension*i+j] = src[dimension*i+j];
            //printf("%f %f\n", A[dimension*i+j], B[dimension*i+j]);
        }
    }
}



void init_data(float *A, float *B, float *C, int dimension)
{
    int i, j, k;
    std::mt19937 rng(292); // fixed seed for reproducibility
    std::uniform_real_distribution<float> dist(-0.5f, 0.5f);
    for(i = 0; i < dimension; i++) {
        //printf("Initializing data %d\n", i);
        for(j = 0; j < dimension; j++) {
            A[dimension*i+j] = dist(rng);
            B[dimension*i+j] = dist(rng);
            C[dimension*i+j] = 0.0f;
            //printf("%f %f\n", A[dimension*i+j], B[dimension*i+j]);
        }
    }
    printf("Data successfully initialized.\n");
}


void init_data_int(int *A, int *B, int *C, int dimension)
{
    int i, j, k;
    std::mt19937 rng(292); // fixed seed for reproducibility
    std::uniform_int_distribution<int> dist(-50,50);
    for(i = 0; i < dimension; i++) {
        for(j = 0; j < dimension; j++) {
            A[dimension*i+j] = dist(rng);
            B[dimension*i+j] = dist(rng);
            C[dimension*i+j] = 0;
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


double print_checksum_int(int* C, int dimention)
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

/*
  M = C_out
  K = C_in*ksize*ksize
  N = out_height*out_width


    // A = M*K
    // B = K*N
    // C = M*N

*/
void matmult_conv_blocked(float* A, float* B, float* C, int M, int K, int N)
{


    int i, j, k, ii, jj, kk;
    int bs = 64; // you can tune this

    for(i = 0; i < M; i += bs)
        for(k = 0; k < K; k += bs)
            for(j = 0; j < N; j += bs)
                for(ii = i; ii < std::min(i + bs, M); ii++)
                    for(kk = k; kk < std::min(k + bs, K); kk++)
                        for(jj = j; jj < std::min(j + bs, N); jj++)
                            C[ii*N + jj] += A[ii*K + kk] * B[kk*N + jj];
}


/*
    This matches benchmark tile_tme
*/
void matmult_dtl_transposed_tile(size_t size, size_t tile_size, size_t inner_tile_size, float* a, float*  b, float*  c) {
  // Perform the matrix multiplication with double nested tiling
  for (size_t ii = 0; ii < size; ii += tile_size) {
    for (size_t jj = 0; jj < size; jj += tile_size) {
      for (size_t kk = 0; kk < size; kk += tile_size) {
        // Handle each block
        for (size_t iii = ii; iii < ii+tile_size && iii < size; iii += inner_tile_size) {
          for (size_t jjj = jj; jjj < jj+tile_size && jjj < size; jjj += inner_tile_size) {
            for (size_t kkk = kk; kkk < kk+tile_size && kkk < size; kkk += inner_tile_size) {
              // Handle each sub-block
              for (size_t i = iii; i < iii+inner_tile_size && i < size; i++) {
                for (size_t j = jjj; j < jjj+inner_tile_size && i < size; j++) {
                  for (size_t k = kkk; k < kkk+inner_tile_size && k < size; k++) {
                    c[i*size+j] += a[i*size+k]*b[j*size+k];
                  }
                }
              }
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

void transpose_naive_int(int *src, int *dst, int src_row, int src_col)
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
    //printf("dimension %d\n", dimension);
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
void matmult_opt3_transposed_int(int *A, int *B, int *C, int dimension)
{
    int i,j,k;
    int alloc_size = dimension*dimension*sizeof(int);
    int* Bt = (int*)malloc(alloc_size);

    int * bt = Bt;
    transpose_naive_int(B, bt, dimension, dimension);

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
void matmult_opt3_pretransposed_int(int *A, int* bt, int *C, int dimension)
{
    int i,j,k;

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
void matmult_opt3_pretransposed(float *A, float* bt, float *C, int dimension)
{
    int i,j,k;
    int alloc_size = dimension*dimension*sizeof(float);
    //*Bt = (float*)malloc(alloc_size);



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



void matmult_dtl_transposed_int(int *A, int *B, int *C, int dimension)
{
    int i,j,k;
    int alloc_size = dimension*dimension*sizeof(int);
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




void mat_add(TYPE* a, TYPE*  b, TYPE* c, size_t size) {
	for (size_t i = 0; i < size*size; i++)
        c[i] = a[i]+b[i];
}

void matmul_opt4_recursive(TYPE*  a, TYPE*  b, TYPE* c, const size_t size, size_t threshold) {
    
  if (size == THRESHOLD) {
	  // matrix C must be zeroed because the tiled multiplication cannot do it inside the loops (unlike the naive approach	
    memset(c, 0, size*size*sizeof(TYPE));
    for (size_t i = 0; i < size; i++) {
      for (size_t j = 0; j < size; j++) {
        for (size_t k = 0; k < size; k++) {
          c[i*size+j] += a[i*size+k]*b[k*size+j];
        }
      }
    }
  }
  else {
    // Shrink the size by a factor of 2
    int new_size = size/2;

    int res = 0;
    // Allocate the buffers to store the submatrices (i.e., a, b, c, d, e, f, g, and h)
    TYPE** submatrices = (TYPE**)malloc(8*sizeof(TYPE*));
    for (size_t i = 0; i < 8; i++) {
      res = posix_memalign((void**)&submatrices[i], NEON_ALIGNMENT, (new_size*new_size*sizeof(TYPE)));
      if (res != 0)
        printf("An error occured when using 'posix_memalign'\n");
    }

    TYPE** intermediate_result = (TYPE**)malloc(2*sizeof(TYPE*));
    for (int i = 0; i < 2; i++) { 
      res = posix_memalign((void**)&intermediate_result[i], NEON_ALIGNMENT, new_size*new_size*sizeof(TYPE));
      if (res != 0)
        printf("An error occured when using 'posix_memalign'\n");
    }

    TYPE** result = (TYPE**)malloc(4*sizeof(TYPE*));
    for (int i = 0; i < 4; i++) { 
      res = posix_memalign((void**)&result[i], NEON_ALIGNMENT, new_size*new_size*sizeof(TYPE));
      if (res != 0)
        printf("An error occured when using 'posix_memalign'\n");
    }

    // Divide the matrices into 4 submatrices and call multiply recursively
    for (size_t i = 0; i < new_size; i++) {
      for (size_t j = 0; j < new_size; j++) {
        // Computing submatrices
        submatrices[0][i*new_size+j] = a[(i*size)+j];                       // A00 (A)
        submatrices[1][i*new_size+j] = a[(i*size)+(j+new_size)];            // A01 (B)
        submatrices[2][i*new_size+j] = a[((i+new_size)*size)+j];            // A10 (C)
        submatrices[3][i*new_size+j] = a[((i+new_size)*size)+(j+new_size)]; // A11 (D)
        submatrices[4][i*new_size+j] = b[(i*size)+j];                       // B00 (E)
        submatrices[5][i*new_size+j] = b[(i*size)+(j+new_size)];            // B01 (F)
        submatrices[6][i*new_size+j] = b[((i+new_size)*size)+j];            // B10 (G)
        submatrices[7][i*new_size+j] = b[((i+new_size)*size)+(j+new_size)]; // B11 (H)
      }
    }

    // Compute each submatrix of matrix c
    // C11
    matmul_opt4_recursive(submatrices[0], submatrices[4], intermediate_result[0], new_size, THRESHOLD); // AE
    matmul_opt4_recursive(submatrices[1], submatrices[6], intermediate_result[1], new_size, THRESHOLD); // BG
    mat_add(intermediate_result[0], intermediate_result[1], result[0], new_size);   // AE+BG
    // C12
    matmul_opt4_recursive(submatrices[0], submatrices[5], intermediate_result[0], new_size, THRESHOLD); // AF
    matmul_opt4_recursive(submatrices[1], submatrices[7], intermediate_result[1], new_size, THRESHOLD); // BH
    mat_add(intermediate_result[0], intermediate_result[1], result[1], new_size);   // AF+BH
    // C21
    matmul_opt4_recursive(submatrices[2], submatrices[4], intermediate_result[0], new_size, THRESHOLD); // CE
    matmul_opt4_recursive(submatrices[3], submatrices[6], intermediate_result[1], new_size, THRESHOLD); // DG
    mat_add(intermediate_result[0], intermediate_result[1], result[2], new_size);   // CE+DG
    // C22
    matmul_opt4_recursive(submatrices[2], submatrices[5], intermediate_result[0], new_size, THRESHOLD); // CF
    matmul_opt4_recursive(submatrices[3], submatrices[7], intermediate_result[1], new_size, THRESHOLD); // DH
    mat_add(intermediate_result[0], intermediate_result[1], result[3], new_size);   // CF+DH

    // Inserts results in matrix c
    for (size_t i = 0; i < new_size; i++) {
      for (size_t j = 0; j < new_size; j++) { 
        c[(i*size)+j]                       = result[0][i*new_size+j]; // C00 (AE+BG)
        c[(i*size)+(j+new_size)]            = result[1][i*new_size+j]; // C01 (AF+BH)
        c[((i+new_size)*size)+j]            = result[2][i*new_size+j]; // C10 (CE+DG)
        c[((i+new_size)*size)+(j+new_size)] = result[3][i*new_size+j]; // C11 (CF+DH)
      }
    }

    // Free temporary matrices
    for (int i = 0; i < 8; i++)
      free(submatrices[i]);
    free(submatrices);
    for (int i = 0; i < 4; i++)
      free(result[i]);
    free(result);
    for (int i = 0; i < 2; i++)
      free(intermediate_result[i]);
    free(intermediate_result);
  }
}



void matmul_opt5_recursive_pretranspose(TYPE* a, TYPE*  b, TYPE* c, size_t size, size_t threshold) {
  if (size == THRESHOLD) {
	  // Matrix C must be zeroed because the tiled multiplication cannot do it inside the loops (unlike the naive approach	
    memset(c, 0, size*size*sizeof(TYPE));
    //
    for (size_t i = 0; i < size; i++) {
      for (size_t j = 0; j < size; j++) {
        for (size_t k = 0; k < size; k++) {
          c[i*size+j] += a[i*size+k]*b[j*size+k];
        }
      }
    }
  }
  else {
    // Shrink the size by a factor of 2
    uint32_t new_size = size/2;

    int res = 0;
    // Allocate the buffers to store the submatrices (i.e., a, b, c, d, e, f, g, and h)
    TYPE** submatrices = (TYPE**)malloc(8*sizeof(TYPE*));
    for (size_t i = 0; i < 8; i++) {
      res = posix_memalign((void**)&submatrices[i], NEON_ALIGNMENT, (new_size*new_size*sizeof(TYPE)));
      if (res != 0)
        printf("An error occured when using 'posix_memalign'\n");
    }

    TYPE** intermediate_result = (TYPE**)malloc(2*sizeof(TYPE*));
    for (int i = 0; i < 2; i++) { 
      res = posix_memalign((void**)&intermediate_result[i], NEON_ALIGNMENT, new_size*new_size*sizeof(TYPE));
      if (res != 0)
        printf("An error occured when using 'posix_memalign'\n");
    }

    TYPE** result = (TYPE**)malloc(4*sizeof(TYPE*));
    for (int i = 0; i < 4; i++) { 
      res = posix_memalign((void**)&result[i], NEON_ALIGNMENT, new_size*new_size*sizeof(TYPE));
      if (res != 0)
        printf("An error occured when using 'posix_memalign'\n");
    }

    // Divide the matrices into 4 submatrices and call multiply recursively
    for (size_t i = 0; i < new_size; i++) {
      for (size_t j = 0; j < new_size; j++) {
        // Computing submatrices
        submatrices[0][i*new_size+j] = a[(i*size)+j];                       // A00 (A)
        submatrices[1][i*new_size+j] = a[(i*size)+(j+new_size)];            // A01 (B)
        submatrices[2][i*new_size+j] = a[((i+new_size)*size)+j];            // A10 (C)
        submatrices[3][i*new_size+j] = a[((i+new_size)*size)+(j+new_size)]; // A11 (D)
        submatrices[4][i*new_size+j] = b[(i*size)+j];                       // B00 (E)
        submatrices[5][i*new_size+j] = b[(i*size)+(j+new_size)];            // B01 (F)
        submatrices[6][i*new_size+j] = b[((i+new_size)*size)+j];            // B10 (G)
        submatrices[7][i*new_size+j] = b[((i+new_size)*size)+(j+new_size)]; // B11 (H)
      }
    }

    // Compute each submatrix of matrix c
    // C11
    matmul_opt5_recursive_pretranspose(submatrices[0], submatrices[4], intermediate_result[0], new_size, THRESHOLD); // AE
    matmul_opt5_recursive_pretranspose(submatrices[1], submatrices[5], intermediate_result[1], new_size, THRESHOLD); // BF
    mat_add(intermediate_result[0], intermediate_result[1], result[0], new_size);   // AE+BF
    // C12
    matmul_opt5_recursive_pretranspose(submatrices[0], submatrices[6], intermediate_result[0], new_size, THRESHOLD); // AG
    matmul_opt5_recursive_pretranspose(submatrices[1], submatrices[7], intermediate_result[1], new_size, THRESHOLD); // BH
    mat_add(intermediate_result[0], intermediate_result[1], result[1], new_size);   // AG+BH
    // C21
    matmul_opt5_recursive_pretranspose(submatrices[2], submatrices[4], intermediate_result[0], new_size, THRESHOLD); // CE
    matmul_opt5_recursive_pretranspose(submatrices[3], submatrices[5], intermediate_result[1], new_size, THRESHOLD); // DF
    mat_add(intermediate_result[0], intermediate_result[1], result[2], new_size);   // CE+DF
    // C22
    matmul_opt5_recursive_pretranspose(submatrices[2], submatrices[6], intermediate_result[0], new_size, THRESHOLD); // CG
    matmul_opt5_recursive_pretranspose(submatrices[3], submatrices[7], intermediate_result[1], new_size, THRESHOLD); // DH
    mat_add(intermediate_result[0], intermediate_result[1], result[3], new_size);   // CG+DH

    // Inserts results in matrix c
    for (size_t i = 0; i < new_size; i++) {
      for (size_t j = 0; j < new_size; j++) { 
        c[(i*size)+j]                       = result[0][i*new_size+j]; // C11 (AE+BF)
        c[(i*size)+(j+new_size)]            = result[1][i*new_size+j]; // C12 (AG+BH)
        c[((i+new_size)*size)+j]            = result[2][i*new_size+j]; // C21 (CE+DF)
        c[((i+new_size)*size)+(j+new_size)] = result[3][i*new_size+j]; // C22 (CG+DH)
      }
    }

    // Free temporary matrices
    for (int i = 0; i < 8; i++)
      free(submatrices[i]);
    free(submatrices);
    for (int i = 0; i < 4; i++)
      free(result[i]);
    free(result);
    for (int i = 0; i < 2; i++)
      free(intermediate_result[i]);
    free(intermediate_result);
  }
}