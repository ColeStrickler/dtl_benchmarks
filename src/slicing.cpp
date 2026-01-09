#include "slicing.hpp"

void slice_tensor_int(int *out,
                      int *in,
                      int n1,
                      int h1,
                      int w1,
                      int c1,
                      int stride_n1,
                      int stride_h1,
                      int stride_w1,
                      int stride_c1,
                      int stride_d1,
                      int stride_d2,
                      int stride_d3)
{
    int x = 0;
    //printf("%d, %d\n", n0, n1);
    for (int n = 0; n < n1; n++) {
        for (int h = 0; h < h1; h++) {
            for (int w = 0; w < w1; w++) {
                for (int c = 0; c < c1; c++) {
                    //printf("%d\n", x);
                    out[x++] = in[(n*stride_n1)*stride_d3 + (h*stride_h1)*stride_d2 + (w*stride_w1)*stride_d1 + stride_c1*c];
                }
            }
        }
    }

}