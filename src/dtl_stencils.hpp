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
                                    out = c_im*(data_size*height*width) + (h + kh)*(data_size*width) + data_size*(w + kw);
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
                                    out = data_size*(c*(size*size) + x*channels + n*c*(size*size));
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
                                            out = (c*(data_size*size*size) + h*(data_size*size) + n*(data_size*channels*size*size) + w*data_size);
                                        }
                                    }
                                }
                            }
                        )"
    },

                    


};









#endif