#ifndef DTL_BENCHMARK_STENCIL_HPP
#define DTL_BENCHMARK_STENCIL_HPP
#include <unordered_map>



static std::unordered_map<std::string, std::string> dtl_stencil_configs = {

    {"vol2col", \
            R"(
                for (int c = 0; c < [CHANNELS]; c++) {
                    for (int kt = 0; kt < [KT]; kt++) {
                        for (int kh = 0; kh < [KH]; kh++) {
                            for (int kw = 0; kw < [KW]; kw++) {
                                for (int t = 0; t < [CDEPTH]; t++) {
                                    for (int h = 0; h < [CHEIGHT]; h++) {
                                        for (int w = 0; w < [CWIDTH]; w++) {
                                            out = (c*(depth*height*width) + w + kw)
                                                    + ((t + kt)*(height*width)
                                                    + (h + kh)*width);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            )"
    },


    {"im2col", \
                R"(
                for (int c_im = 0; c_im < [CHANNELS]; c_im++) {
                    for (int kh = 0; kh < [KH]; kh++) {
                        for (int kw = 0; kw < [KW]; kw++) {
                            for (int h = 0; h < [HMAX]; h++) {
                                for (int w = 0; w < [WMAX]; w++) {
                                    out = c_im*(height*width) + (h + kh)*(width) + (w + kw);
                                }
                            }
                        }
                    }
                }
                )"
    },

    {"db_col_project", \
                    R"(
                    for (int i = 0; i < [ROWS]; i++)
                    {
                        for (int j = 0; j < [COLUMNS]; j++)
                        {
                            out = i*row_size + col_offsets[j];
                        }
                    }
                    )"
    },

    {"db_join", \
                R"(

                for (int x = 0; x < [NTABLE_COUNT]; x++)
                {
                    for (int i = 0; i < [NROWS]; i++)
                    {
                        for (int j = 0; j < [NCOLS]; j++)
                        {
                                switch(x)
                                {
                                    case:
                                        out = table1_base + t1_c1 + i*t1row_size;
                                        out = table2_base + t2_c1 + i*t2row_size;
                                    case:
                                        out = table1_base + t1_c2 + i*t2row_size;
                                        out = table2_base + t2_c2 + i*t2row_size;
                                    case:
                                        out = table1_base + t1_c3 + i*t2row_size;
                                        out = table2_base + t2_c3 + i*t2row_size;
                                }
                            }
                            
                        }
                    }
                    )"
    },


    {"aos_to_soa", \
                    R"(
                    for (int i = 0; i < [NSTRUCT]; i++)
                    {
                        for (int j = 0; j < [NOFFSETS]; j++)
                        {
                            out = i*struct_size + var_offsets[j];
                        }
                    }
                    )"
    },



    {"matmul_transpose", \
                        R"(
                        for (int col = 0; col < [NCOLS]; col++)
                        {
                            for (int row = 0; row < [NROWS]; row++)
                            {
                                out = row*row_size + col;
                            }
                        }
                        )"
    },

    {"nhwc_permutation", \
                        R"(
                        for (int n = 0; n < [BATCH_SIZE]; n++)
                        {
                            for (int c = 0; c < [CHANNELS]; c++)
                            {
                                for (int x = 0; x < [SIZE_SQUARED]; x++)
                                {
                                    out = (c + x*channels + n*c*(size*size));
                                }
                            }
                        }
                        )"
    },
    {"batch2space", \
                        R"(
                            for (int c = 0; c < [CHANNELS]; c++)
                            {
                                for (int h = 0; h < [HEIGHT]; h++)
                                {
                                    for (int n = 0; n < [BATCH_SIZE]; n++)
                                    {
                                        for (int w = 0; w < [WIDTH]; w++)
                                        {
                                            out = (c + h*(size*channels) + n*(channels*size*size) + w*channels);
                                        }
                                    }
                                }
                            }
                        )"
    },

    {"tensor_unfold1", \
                        R"(
                            for (int k = 0; k < [D2]; k++)
                            {
                                for (int i = 0; i < [D3]; i++)
                                {
                                    for (int j = 0; j < [D4]; j++)
                                    {
                                        for (int l = 0; l < [D1]; l++)
                                        {
                                            out = l + k*(stride_d1) + j*(stride_d3) + i*(stride_d2);
                                        }
                                    }
                                }
                            }
                        )"
    },

    {"tensor_unfold2", \
                        R"(
                            for (int l = 0; l < [D1]; l++)
                            {
                                for (int i = 0; i < [D3]; i++)
                                {
                                    for (int j = 0; j < [D4]; j++)
                                    {
                                        for (int k = 0; k < [D2]; k++)
                                        {
                                            out = l + k*(stride_d1) + j*(stride_d3) + i*(stride_d2);
                                        }
                                    }
                                }
                            }
                        )"
    },

    {"tensor_unfold3", \
                        R"(
                            for (int l = 0; l < [D1]; l++)
                            {
                                for (int k = 0; k < [D2]; k++)
                                {
                                    for (int j = 0; j < [D4]; j++)
                                    {
                                        for (int i = 0; i < [D3]; i++)
                                        {
                                            out = l + k*(stride_d1) + j*(stride_d3) + i*(stride_d2);
                                        }
                                    }
                                }
                            }
                        )"
    },

    {"tensor_unfold4", \
                        R"(
                            for (int l = 0; l < [D1]; l++)
                            {
                                for (int k = 0; k < [D2]; k++)
                                {
                                    for (int i = 0; i < [D3]; i++)
                                    {
                                        for (int j = 0; j < [D4]; j++)
                                        {
                                            out = l + k*(stride_d1) + j*(stride_d3) + i*(stride_d2);
                                        }
                                    }
                                }
                            }
                        )"
    },


    
    {"tensor_slicing", \
                        R"(
for (int i = 0; i < [D4]; i++)
{
    for (int j = 0; j < [D3]; j++)
    {
        for (int k = 0; k < [D2]; k++)
        {
            for (int l = 0; l < [D1]; l++)
            {
                out = (l*(stride_c1) + j*(stride_d1*stride_w1)) + (k*(stride_h1*stride_d2) + i*(stride_n1*stride_d3));
            }
        }
    }
}
                        )"
    },

    {"highdim_7stencil", \
                        R"(
                            for (int i = 0; i < [NX_MINUS_1]; i++)
                            {
                                for (int j = 0; j < [NY_MINUS_1]; j++)
                                {
                                    for (int k = 0; k < [NZ_MINUS_1]; j++)
                                    {
                                        out = (i+1)*(stride_nx*data_size) + (j+1)*(stride_ny*data_size) + (data_size*k+data_size);
                                        out = (i+(1))*(stride_nx*data_size) + (j+(1+1))*(stride_ny*data_size) + ((k+1)*data_size);
                                        out = (i+(1))*(stride_nx*data_size) + (j)*(stride_ny*data_size) + ((k+1)*data_size);
                                        out = (i+(1))*(stride_nx*data_size) + (j+(1))*(stride_ny*data_size) + (k+(1+1))*data_size;
                                        out = (i+(1))*(stride_nx*data_size) + (j+(1))*(stride_ny*data_size) + ((k)*data_size);
                                        out = (i+(1+1))*(stride_nx*data_size) + (j+(1))*(stride_ny*data_size) + ((k+1)*data_size);
                                        out = (i)*(stride_nx*data_size) + (j+(1))*(stride_ny*data_size) + ((k+1)*data_size);
                                    }
                                }
                            }
                        )"
    },

    {"cube_stencil_8corner", \
                    R"(
                        for (int i = 0; i < [N_3DSTRUCT]; i++)
                        {
                            for (int a = 0; a < [NCUBES]; a++)
                            {
                                for (int x = 0; x < [CUBE_DIM1]; x++)
                                {
                                    for (int y = 0; y < [CUBE_DIM2]; y++)
                                    {
                                        for (int z = 0; z < [CUBE_DIM3]; z++)
                                        {
                                            out = cube_locs[a] + (i*(stride_d3*data_size) + x*(stride_d2*data_size)) + (y*(stride_d1*data_size) + z*data_size);
                                        }
                                    }
                                }
                            }
                        }
                    )"
    },

    {"permute_reflect", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                {
                    for (int h = 0; h < [HMAX]; h++)
                    {
                        for (int w = 0; w < [WMAX]; w++)
                        {
                            out = n*stride_n + c + h*stride_h + (W-1-w)*stride_w;
                        }
                    }
                }
            }
        )"
    },

    {"permute_rot_left", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                {
                    for (int w = 0; w < [WMAX]; w++)
                    {
                        for (int h = 0; h < [HMAX]; h++)
                        {
                            out = n*stride_n + c + h*stride_h + (W-1-w)*stride_w;
                        }
                    }
                }
            }
        )"
    },

        
    {"permute_rot_left_reflect", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                { 
                    for (int w = 0; w < [WMAX]; w++)
                    {
                        for (int h = 0; h < [HMAX]; h++)
                        {
                            out = n*stride_n + c + h*stride_h + w*stride_w;
                        }
                    }
                }
            }
        )"
    },

            
    {"permute_flipy", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                { 
                    for (int h = 0; h < [HMAX]; h++)
                    {
                        for (int w = 0; w < [WMAX]; w++)
                        {
                            out = n*stride_n + c + (H-1-h)*stride_h + w*stride_w;
                        }
                    }
                }
            }
        )"
    },

    {"permute_flipy_reflect", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                { 
                    for (int h = 0; h < [HMAX]; h++)
                    {
                        for (int w = 0; w < [WMAX]; w++)
                        {
                            out = n*stride_n + c + (H-1-h)*stride_h + (W-1-w)*stride_w;
                        }
                    }
                }
            }
        )"
    },


    {"permute_rot_right", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                {
                    for (int w = 0; w < [WMAX]; w++)
                    {
                        for (int h = 0; h < [HMAX]; h++)
                        {
                            out = n*stride_n + c + (H-1-h)*stride_h + w*stride_w;
                        }
                    }
                }
            }
        )"
    },

    {"permute_rot_right_reflect", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                {
                    for (int w = 0; w < [WMAX]; w++)
                    {
                        for (int h = 0; h < [HMAX]; h++)
                        {
                            out = n*stride_n + c + (H-1-h)*stride_h + (W-1-w)*stride_w;
                        }
                    }
                }
            }
        )"
    },

    {"permute_reflect_nchw", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                {
                    for (int h = 0; h < [HMAX]; h++)
                    {
                        for (int w = 0; w < [WMAX]; w++)
                        {
                            out = n*stride_n + c*stride_c + h*stride_h + (W-1-w);
                        }
                    }
                }
            }
        )"
    },

    {"permute_rot_left_nchw", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                {
                    for (int w = 0; w < [WMAX]; w++)
                    {
                        for (int h = 0; h < [HMAX]; h++)
                        {
                            out = n*stride_n + c*stride_c + h*stride_h + (W-1-w);
                        }
                    }
                }
            }
        )"
    },

        
    {"permute_rot_left_reflect_nchw", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                { 
                    for (int w = 0; w < [WMAX]; w++)
                    {
                        for (int h = 0; h < [HMAX]; h++)
                        {
                            out = n*stride_n + c*stride_c + h*stride_h + w;
                        }
                    }
                }
            }
        )"
    },

            
    {"permute_flipy_nchw", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                { 
                    for (int h = 0; h < [HMAX]; h++)
                    {
                        for (int w = 0; w < [WMAX]; w++)
                        {
                            out = n*stride_n + c*stride_c + (H-1-h)*stride_h + w;
                        }
                    }
                }
            }
        )"
    },

    {"permute_flipy_reflect_nchw", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                { 
                    for (int h = 0; h < [HMAX]; h++)
                    {
                        for (int w = 0; w < [WMAX]; w++)
                        {
                            out = n*stride_n + c*stride_c + (H-1-h)*stride_h + (W-1-w);
                        }
                    }
                }
            }
        )"
    },


    {"permute_rot_right_nchw", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                {
                    for (int w = 0; w < [WMAX]; w++)
                    {
                        for (int h = 0; h < [HMAX]; h++)
                        {
                            out = n*stride_n + c*stride_c + (H-1-h)*stride_h + w;
                        }
                    }
                }
            }
        )"
    },

    {"permute_rot_right_reflect_nchw", \
        R"(
            for (int n = 0; n < [NMAX]; n++)
            {
                for (int c = 0; c < [CMAX]; c++)
                {
                    for (int w = 0; w < [WMAX]; w++)
                    {
                        for (int h = 0; h < [HMAX]; h++)
                        {
                            out = n*stride_n + c*stride_c + (H-1-h)*stride_h + (W-1-w);
                        }
                    }
                }
            }
        )"
    },



    {"sparse_matrix_vector", \
        R"(
            metadata_stream<4, 0x0> x;
            for (int nnz = 0; nnz < [NNZ]; nnz++)
            {
                out = x[nnz]; 
            }
        )"
    },


    {"attention_k", \
        R"(
            metadata_stream<4, 0x0> x;
            for (int nnz = 0; nnz < [NNZ]; nnz++)
            {
                for (int d = 0; d < [D]; d++)
                {
                    out = x[nnz]*[D2] + d; 
                }
            }
        )"
    },


    {"attention_v", \
        R"(
            metadata_stream<4, 0x0> x;

            for (int nnz = 0; nnz < [NNZ]; nnz++)
            {
                for (int d = 0; d < [DV]; d++)
                {
                    out = x[nnz]*[DV2] + d; 
                }
            }
        )"
    },


    { // extract entire record in order defined by metadata
        "recordsort", \
        R"(
            metadata_stream<4, 0x0> x;
            for (int record = 0; record < [RECORDS]; record++)
            {
                for (int d = 0; d < [PAYLOAD_SIZE]; d++)
                {
                    out = x[record]+d;
                }
            }
        )"
    },

    {// extract fields in order defined by metadata
        "recordsort_extract", \
        R"(
            metadata_stream<4, 0x0> x;
            for (int record = 0; record < [RECORDS]; record++)
            {
                for (int d = 0; d < [NFIELDS_EXTRACT]; d++)
                {
                    out = x[record]*[RECORD_SIZE] + offsets[d]; 
                }
            }
        )"
    },

    { // extract fileds in order
        "record_extract", \
        {
            R"(
                for (int record = 0; record < [RECORDS]; record++)
                {
                    for (int d = 0; d < [NFIELDS_EXTRACT]; d++)
                    {
                        out = record*[RECORD_SIZE] + offsets[d]; 
                    }
                }    
            )"
        }
    },



    {"dummy", \
        R"(
            for (int i = 0; i < 100000000; i++)
            {
                out = i;
            }
        )"
    },


    {"composedTest", \
        R"(
            DTL<0x4000000> base = {
                metadata_stream<4, 0x4000000> i_stream;
                for (int i = 0; i < 0x1000000; i++)
                {
                    out = i_stream[i];
                }
            }

            DTL<0x4000000> kernel : base = {
                metadata_stream<4, base> indirect_stream;
                for (int i = 0; i < 0x1000000; i++)
                {
                    out = indirect_stream[i];
                }
            }
            active kernel;
        )"
    },


};









#endif
