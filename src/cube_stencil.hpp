#ifndef DTL_BENCH_CUBE_STENCIL_HPP
#define DTL_BENCH_CUBE_STENCIL_HPP


#include <stdio.h>

void cube_stencil_8corner_cpu(int* tensor, int* out_tensor, int d4, int d3, int d2, int d1, int cube_dim);


#endif