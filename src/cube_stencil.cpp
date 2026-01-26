#include "cube_stencil.hpp"



void cube_stencil_8corner_cpu(int *tensor, int *out_tensor, int d4, int d3, int d2, int d1, int cube_dim)
{
// w = d4 index, x = d3 index, y = d2 index, z = d1 index
#define TENSOR_COORD4(w, x, y, z) (((w)*d3*d2*d1) + ((x)*d2*d1) + ((y)*d1) + (z))
#define TENSOR_COORD3(x, y, z) (((x)*d2*d1) + ((y)*d1) + (z))
#define CUBE_SIZE (cube_dim*cube_dim*cube_dim)
#define CUBE_2D (cube_dim*cube_dim)
#define CUBE_DIM (cube_dim)


    int stride_d2 = d1;
    int stride_d3 = d2*d1;


    int cube_locs[] = {TENSOR_COORD3(0, 0, 0), TENSOR_COORD3(0, d2-CUBE_DIM, 0), TENSOR_COORD3(0, 0, d1-CUBE_DIM),\
                       TENSOR_COORD3(d3-CUBE_DIM, 0, 0), TENSOR_COORD3(d3-CUBE_DIM, d2-CUBE_DIM, 0), TENSOR_COORD3(d3-CUBE_DIM, 0, d1-CUBE_DIM),\
                       TENSOR_COORD3(0, d2-CUBE_DIM, d1-CUBE_DIM), TENSOR_COORD3(d3-CUBE_DIM, d2-CUBE_DIM, d1-CUBE_DIM)};  

    int write_index = 0;
    for (int i = 0; i < d4; i++)
    {
        for (int a = 0; a < 8; a++)
        {
            for (int x = 0; x < cube_dim; x++)
            {
                for (int y = 0; y < cube_dim; y++)
                {
                    for (int z = 0; z < cube_dim; z++)
                    {
                        out_tensor[i*8*CUBE_SIZE + a*CUBE_SIZE + x*CUBE_2D + y*CUBE_DIM + z] = tensor[cube_locs[a] + TENSOR_COORD4(i, 0, 0, 0) + TENSOR_COORD3(x, 0, 0) + TENSOR_COORD3(0, y, 0) + TENSOR_COORD3(0,0,z)];
                    }
                }
            }
        }
    }


#undef TENSOR_COORD4
#undef TENSOR_COORD3
#undef CUBE_SIZE
#undef CUBE_DIM

}


void algo()
{
//    int d1_max;
//    int d2_max;
//    int d3_max;
//    int d4_max;
//    int stride_d1;
//    int stride_d2;
//    int stride_d3;
//    int stride_d4;
//    int out_size;
//    int item_size;
//#define TYPE int
//
//
//    TYPE out[out_size];
//    TYPE data_item[item_size];
//    int out_index = 0;
//    for (int d4 = 0; d4 < d4_max; d4++)
//        for (int d3 = 0; d3 < d3_max; d3++)
//            for (int d2 = 0; d2 < d2_max; d2++)
//                for (int d1 = 0; d1 < d1_max; d1++)
//                    out[out_index++] = d1*stride_d1 + d2*stride_d2 + d3*stride_d3 + d4*stride_d4;
//
//
//
//    
//    TYPE out[out_size];
//    TYPE data_item[item_size];
//    int out_index = 0;
//
//    /*
//        int c1 =        <any number>
//        int c2 =        <any number>
//        int carray[] =  {parameterizable length array}
//        ......
//        int c_cmax =    <any number>
//    */
//
//    for (int d4 = 0; d4 < d4_max; d4++)
//        for (int d3 = 0; d3 < d3_max; d3++)
//            for (int d2 = 0; d2 < d2_max; d2++)
//                for (int d1 = 0; d1 < d1_max; d1++)
//                    out[out_index++] = d1*stride_d1 + d2*stride_d2 + d3*stride_d3 + d4*stride_d4;
//                    /* 
//                        out[out_index++] = i*j*c1*c2*52944219;
//                        out[out_index++] = d4_max + my_const_array[d1]*d2;
//                        .....
//                        out[out_index++] = <arbitrary combination>
//                    */
//

    int out;


    int row_size = 2560;
    int col_size = 4;
    
    for (int col = 0; col < 640; col++)
    {
        for (int row = 0; row < 640; row++)
        {
            out = row*row_size + col*col_size;
        }
    }
}