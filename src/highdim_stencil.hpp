#ifndef DTL_BENCH_HIGHDIM_STENCIL_HPP
#define DTL_BENCH_HIGHDIM_STENCIL_HPP


#include <stdio.h>

void highdim_7stencil_cpu(float* u, float* u_new, int nx, int ny, int nz);
void highdim_7stencil_dtu(float* array, float* u_new, int nx, int ny, int nz);




#endif