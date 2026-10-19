#ifndef DTL_BENCHMARK_STENCIL_HPP
#define DTL_BENCHMARK_STENCIL_HPP
#include <unordered_map>



static std::unordered_map<std::string, std::string> dtl_stencil_configs = {

    {"vol2col", \
            R"(
            DTL kernel = {
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
            }
            active kernel;
            )"
    },


    {"im2col", \
                R"(
                DTL kernel = {
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
                }
                active kernel;
                )"
    },

    {"db_col_project", \
                    R"(
                    DTL kernel = {
                    for (int i = 0; i < [ROWS]; i++)
                    {
                        for (int j = 0; j < [COLUMNS]; j++)
                        {
                            out = i*row_size + col_offsets[j];
                        }
                    }
                    }
                    active kernel;
                    )"
    },

    {"db_join", \
                R"(
                DTL kernel = {

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
                    }
                    active kernel;
                    )"
    },


    {"aos_to_soa", \
                    R"(
                    DTL kernel = {
                    for (int i = 0; i < [NSTRUCT]; i++)
                    {
                        for (int j = 0; j < [NOFFSETS]; j++)
                        {
                            out = i*struct_size + var_offsets[j];
                        }
                    }
                    }
                    active kernel;
                    )"
    },



    {"matmul_transpose", \
                        R"(
                        DTL kernel = {
                        for (int col = 0; col < [NCOLS]; col++)
                        {
                            for (int row = 0; row < [NROWS]; row++)
                            {
                                out = row*row_size + col;
                            }
                        }
                        }
                        active kernel;
                        )"
    },

    {"nhwc_permutation", \
                        R"(
                        DTL kernel = {
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
                        }
                        active kernel;
                        )"
    },
    {"batch2space", \
                        R"(
                        DTL kernel = {
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
                        }
                        active kernel;
                        )"
    },

    {"tensor_unfold1", \
                        R"(
                        DTL kernel = {
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
                        }
                        active kernel;
                        )"
    },

    {"tensor_unfold2", \
                        R"(
                        DTL kernel = {
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
                        }
                        active kernel;
                        )"
    },

    {"tensor_unfold3", \
                        R"(
                        DTL kernel = {
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
                        }
                        active kernel;
                        )"
    },

    {"tensor_unfold4", \
                        R"(
                        DTL kernel = {
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
                        }
                        active kernel;
                        )"
    },


    
    {"tensor_slicing", \
                        R"(
                        DTL kernel = {
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
                        }
                        active kernel;
                        )"
    },

    {"highdim_7stencil", \
                        R"(
                        DTL kernel = {
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
                        }
                        active kernel;
                        )"
    },

    {"cube_stencil_8corner", \
                    R"(
                    DTL kernel = {
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
                    }
                    active kernel;
                    )"
    },

    {"permute_reflect", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

    {"permute_rot_left", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

        
    {"permute_rot_left_reflect", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

            
    {"permute_flipy", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

    {"permute_flipy_reflect", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },


    {"permute_rot_right", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

    {"permute_rot_right_reflect", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

    {"permute_reflect_nchw", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

    {"permute_rot_left_nchw", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

        
    {"permute_rot_left_reflect_nchw", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

            
    {"permute_flipy_nchw", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

    {"permute_flipy_reflect_nchw", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },


    {"permute_rot_right_nchw", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },

    {"permute_rot_right_reflect_nchw", \
        R"(
        DTL kernel = {
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
        }
        active kernel;
        )"
    },



    {"sparse_matrix_vector", \
        R"(
        DTL kernel = {
            metadata_stream<4, 0x178000000, nnz> x;
            for (int nnz = 0; nnz < [NNZ]; nnz++)
            {
                out = x; 
            }
        }
        active kernel;
        )"
    },


    {"attention_k", \
        R"(
        DTL kernel = {
            metadata_stream<4, 0x178000000, nnz> x;
            for (int nnz = 0; nnz < [NNZ]; nnz++)
            {
                for (int d = 0; d < [D]; d++)
                {
                    out = x*[D2] + d; 
                }
            }
        }
        active kernel;
        )"
    },


    {"attention_v", \
        R"(
        DTL kernel = {
            metadata_stream<4, 0x178000000, nnz> x;

            for (int nnz = 0; nnz < [NNZ]; nnz++)
            {
                for (int d = 0; d < [DV]; d++)
                {
                    out = x*[DV2] + d; 
                }
            }
        }
        active kernel;
        )"
    },


    { // extract entire record in order defined by metadata
        "recordsort", \
        R"(
        DTL kernel = {
            metadata_stream<4, 0x178000000, record> x;
            for (int record = 0; record < [RECORDS]; record++)
            {
                for (int d = 0; d < [PAYLOAD_SIZE]; d++)
                {
                    out = x+d;
                }
            }
        }
        active kernel;
        )"
    },

    {// extract fields in order defined by metadata
        "recordsort_extract", \
        R"(
        DTL kernel = {
            metadata_stream<4, 0x178000000, record> x;
            for (int record = 0; record < [RECORDS]; record++)
            {
                for (int d = 0; d < [NFIELDS_EXTRACT]; d++)
                {
                    out = x*[RECORD_SIZE] + offsets[d]; 
                }
            }
        }
        active kernel;
        )"
    },

    { // extract fileds in order
        "record_extract", \
        {
            R"(
            DTL kernel = {
                for (int record = 0; record < [RECORDS]; record++)
                {
                    for (int d = 0; d < [NFIELDS_EXTRACT]; d++)
                    {
                        out = record*[RECORD_SIZE] + offsets[d]; 
                    }
                }    
            }
            active kernel;
            )"
        }
    },



    {"dummy", \
        R"(
        DTL kernel = {
            for (int i = 0; i < 100000000; i++)
            {
                out = i;
            }
        }
        active kernel;
        )"
    },


};









#endif
