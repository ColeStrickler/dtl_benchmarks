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


};









#endif