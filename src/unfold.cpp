#include "unfold.hpp"

void lower_mat_4d(int *dst, int *src, int d1, int d2, int d3, int d4, int stride_d1, int stride_d2, int stride_d3)
{
    int x = 0;
    for (int k = 0; k < d2; k++) 
    {
        for (int i = 0; i < d3; i++)
        {
            for (int j = 0; j < d4; j++)
            {
                for (int l = 0; l < d1; l++)
                {
                    dst[x++] = src[l + k*stride_d1 + j*stride_d3 + i*stride_d2];
                   
                }
            }
        }
        
    }
}

