#ifndef IM2COL_H
#define IM2COL_H
#include <fstream>
#include <string>
#include <iostream>


float im2col_get_pixel(float *im, int height, int width, int channels,
                        int row, int col, int channel, int pad);

void im2col_cpu(float* data_im,
     int channels,  int height,  int width,
     int ksize,  int stride, int pad, float* data_col);                    

float* GetImageBuf(const std::string& filename);
#endif