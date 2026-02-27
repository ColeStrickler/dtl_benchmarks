#ifndef DTL_BENCHMARK_STENCIL_HPP
#define DTL_BENCHMARK_STENCIL_HPP
#include <unordered_map>



static std::unordered_map<std::string, std::string> dtl_stencil_configs = {
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


    {"matmul_transpose", \
                        R"(
                        for (int col = 0; col < [NCOLS]; col++)
                        {
                            for (int row = 0; row < [NROWS]; row++)
                            {
                                out = row*row_size + col*col_size;
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
                                            out = (c*(size*size) + h*(size) + n*(channels*size*size) + w);
                                        }
                                    }
                                }
                            }
                        )"
    },

    {"tensor_unfold", \
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
                                            out = l*(stride_c1) + j*(stride_d1*stride_w1) + k*(stride_h1*stride_d2) + i*(stride_n1*stride_d3);
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

};









#endif