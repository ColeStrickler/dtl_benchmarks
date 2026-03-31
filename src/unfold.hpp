#ifndef DTL_BENCH_TENSOR_UNFOLD_HPP
#define DTL_BENCH_TENSOR_UNFOLD_HPP


void lower_mat_4d_mode1(int* dst, int* src, int d1, int d2, int d3, int d4, int stride_d1, int stride_d2, int stride_d3);
void lower_mat_4d_mode2(int* dst, int* src, int d1, int d2, int d3, int d4, int stride_d1, int stride_d2, int stride_d3);
void lower_mat_4d_mode3(int* dst, int* src, int d1, int d2, int d3, int d4, int stride_d1, int stride_d2, int stride_d3);
void lower_mat_4d_mode4(int* dst, int* src, int d1, int d2, int d3, int d4, int stride_d1, int stride_d2, int stride_d3);


#endif