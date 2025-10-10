#include "im2col.hpp"
#include <stdio.h>
float im2col_get_pixel(float *im, int height, int width, int channels,
                        int row, int col, int channel, int pad)
{
    row -= pad;
    col -= pad;

    if (row < 0 || col < 0 ||
        row >= height || col >= width) return 0;
    return im[col + width*(row + height*channel)];
}

//From Berkeley Vision's Caffe!
//https://github.com/BVLC/caffe/blob/master/LICENSE
void im2col_cpu(float* data_im,
     int channels,  int height,  int width,
     int ksize,  int stride, int pad, float* data_col) 
{
    int c,h,w;
    int height_col = (height + 2*pad - ksize) / stride + 1;
    int width_col = (width + 2*pad - ksize) / stride + 1;

    int channels_col = channels * ksize * ksize;
    for (c = 0; c < channels_col; ++c) {
        int w_offset = c % ksize;
        int h_offset = (c / ksize) % ksize;
        int c_im = c / ksize / ksize;
        for (h = 0; h < height_col; ++h) {
            for (w = 0; w < width_col; ++w) {
                int im_row = h_offset + h * stride;
                int im_col = w_offset + w * stride;
                int col_index = (c * height_col + h) * width_col + w;
                //printf("%d = %d\n", col_index, im_col + width*(im_row + height*c_im));
                data_col[col_index] = im2col_get_pixel(data_im, height, width, channels,
                        im_row, im_col, c_im, pad);
            }
        }
    }
}


float* GetImageBuf(const std::string& filename)
{
    int H = 1080, W = 1920, C = 3;

    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file\n";
        return nullptr;
    }

    float* buf = new float[H*W*C];
    file.read((char*)buf, H*W*C*sizeof(float));
    if (!file) {
        std::cerr << "Failed to read full file\n";
        return nullptr;
    }

    return buf;
}