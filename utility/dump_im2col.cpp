#include <iostream>
#include <fstream>
#include <vector>


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


float* GetBuf(const std::string& filename)
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
#define READ_FLOAT(addr)(*(float*)(addr))
#include <cassert>
int main() {
    unsigned int buf_size = 3*1078*3*3*1918;
    int H = 1080, W = 1920, C = 3;
    float * hwc = GetBuf("data/image_hwc.raw");
    float * chw = GetBuf("data/image_chw.raw");


    float* outbuf = new float[buf_size];
    im2col_cpu(chw, 3, 1080, 1920, 3, 1, 0, outbuf);
    std::ofstream out2("data/output_im2col_cc2.raw", std::ios::binary);
    if (!out2) {
        std::cerr << "Failed to open file\n";
        return 1;
    }
    out2.write(reinterpret_cast<char*>(outbuf), buf_size*sizeof(float));
    out2.close();



    int row_size = 5760;
    int col_size = 4;
    int nchannel = 3;


    int num_patches = 1077*1917;
    int patch_size = 3*3*3;
    
    float* im2col = new float[buf_size];
    unsigned int index = 0;
    //for (int chan = 0; chan < 3; chan++)
    //{
    //    for (int height = 0; height < 1078; height++)
    //    {
    //        for (int kh = 0; kh < 3; kh++)
    //        {
    //            for (int kw = 0; kw < 3; kw++)
    //            {
    //                for (int width = 0; width < 1918; width++)
    //                {                       // printf("%d = %d\n", index, (width+kw)*nchannel + row_size*(kh+height) + chan);
    //                    im2col[index++] = chw[chan*(1920*1080) + width + kw + (height+kh)*1920];
    //                }
    //            }
    //        }
    //    }
    //}

    for (unsigned int c_im = 0; c_im < 3; ++c_im) {
        for (unsigned int kh = 0; kh < 3; ++kh) {
            for (unsigned int kw = 0; kw < 3; ++kw) {
                for (unsigned int h = 0; h < 1078; ++h) {
                    for (unsigned int w = 0; w < 1918; ++w) {
                        unsigned int im_row = h + kh;
                        unsigned int im_col = w + kw;
                        // Check bounds for padding (pad=0, so ensure valid indices)
                            unsigned int byte_offset =   c_im*(8294400) + (h + kh)*7680 + 4*(w + kw);
                            unsigned int byte_offset2 = (c_im * (1080 * 1920) + im_row * 1920 + im_col) * 4;
                            if (byte_offset != byte_offset2)
                                printf("0x%x, 0x%x\n %d,%d,%d,%d,%d\n", byte_offset, byte_offset2, c_im, kh, kw, h, w);
                            assert(byte_offset == byte_offset2);
                            im2col[index++] =  (float)READ_FLOAT((uint64_t)chw + byte_offset);//chw[c_im*(1080*1920) + im_row*1920 + im_col];
                        
                    }
                }
            }
        }
    }



    std::ofstream out("data/output_im2col_cc.raw", std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open file\n";
        return 1;
    }

    // Write buffer
    out.write(reinterpret_cast<char*>(im2col), buf_size*sizeof(float));
    out.close();

    return 0;
}
