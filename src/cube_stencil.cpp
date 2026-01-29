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


// New State Registers
/*


Reg Dim1 = UInt(log2ceil(maxDimSize))
Reg Dim2 = UInt(log2ceil(maxDimSize))
Reg Dim3 = UInt(log2ceil(maxDimSize))
Reg Dim4 = UInt(log2ceil(maxDimSize))
Reg Dim5 = UInt(log2ceil(maxDimSize))
Reg Dim6 = UInt(log2ceil(maxDimSize))

Reg Dim1_stride = UInt(log2ceil(maxStride))
Reg Dim2_stride = UInt(log2ceil(maxStride))
Reg Dim3_stride = UInt(log2ceil(maxStride))
Reg Dim4_stride = UInt(log2ceil(maxStride))
Reg Dim5_stride = UInt(log2ceil(maxStride))
Reg Dim6_stride = UInt(log2ceil(maxStride))

Reg vAutoInc = Bool(1bit)
Reg vDim1Index = Vec(vWidth, UInt(log2ceil(maxDimSize)))
Reg vDim2Index = Vec(vWidth, UInt(log2ceil(maxDimSize)))
Reg vDim3Index = Vec(vWidth, UInt(log2ceil(maxDimSize)))
Reg vDim4Index = Vec(vWidth, UInt(log2ceil(maxDimSize)))
Reg vDim5Index = Vec(vWidth, UInt(log2ceil(maxDimSize)))
Reg vDim6Index = Vec(vWidth, UInt(log2ceil(maxDimSize)))



vloadIDim (vreg %0) --> load using Dimensional Index registers
vloadNoDim <addr> (vreg %0) --> load using index reversing
vWriteVDimI (%x), vreg %0 --> write data to a specifc vDimIndex register
vWriteDimStride (%x), <scalar>
vWriteDim (%x), scalar
*/