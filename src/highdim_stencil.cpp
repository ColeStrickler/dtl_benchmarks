#include "highdim_stencil.hpp"

  

void highdim_7stencil_cpu(float *u, float *u_new, int nx, int ny, int nz)
{
    float alpha = 0.125;

    int istride = ny*nz;
    int jstride = nz;

        // Update interior points only (simple Dirichlet = 0 on boundaries)
        for (int i = 1; i < nx-1; ++i) {
            for (int j = 1; j < ny-1; ++j) {
                for (int k = 1; k < nz-1; ++k) {

                    float center = u[(i)*istride + (j)*jstride + (k)];    
                    float north  = u[(i)*istride + (j+1)*jstride + (k)];
                    float south  = u[(i)*istride + (j-1)*jstride + (k)];
                    float east   = u[(i)*istride + (j)*jstride + (k+1)];
                    float west   = u[(i)*istride + (j)*jstride + (k-1)];
                    float up     = u[(i+1)*istride + (j)*jstride + (k)];
                    float down   = u[(i-1)*istride + (j)*jstride + (k)];

                    u_new[(i)*istride + (j)*jstride + (k)] = center + alpha * (
                        (north + south + east + west + up + down) - 6.0 * center
                    );
                }
            }
        }
   
}

void highdim_7stencil_dtu(float *array, float *u_new, int nx, int ny, int nz)
{

    //printf("here highdim_7stencil\n");
    float alpha = 0.125;
    int x = 0;
    int istride = ny*nz;
    int jstride = nz;
        // Update interior points only (simple Dirichlet = 0 on boundaries)
    for (int i = 1; i < nx-1; ++i) {
        
        for (int j = 1; j < ny-1; ++j) {
            for (int k = 1; k < nz-1; ++k) {
                float center = array[x];
                float north  = array[x+1];
                float south  = array[x+2];
                float east   = array[x+3];
                float west   = array[x+4];
                float up     = array[x+5];
                float down   = array[x+6];
                x += 7;
                u_new[(i)*istride + (j)*jstride + (k)] = center + alpha * (
                    (north + south + east + west + up + down) - 6.0 * center
                );
                }
            }
        }
}
