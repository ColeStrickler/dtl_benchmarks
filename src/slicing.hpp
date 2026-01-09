#ifndef DTL_BENCH_TENSOR_SLICING_HPP
#define DTL_BENCH_TENSOR_SLICING_HPP



void slice_tensor_int(int* out, int* in, int n1, int h1, int w1, int c1, int stride_n1, int stride_h1, int stride_w1, int stride_c1,\
    int stride_d1, int stride_d2, int stride_d3);

#endif