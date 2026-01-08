#include "tensor_permutation.hpp"

void nhwc_to_nchw_cpu(float *data_im, int batch_size, int channels, int height, int width, float *data_out) {

    int i = 0;
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            for (int x = 0; x < height*width; x++)
            {
                data_out[i++] = data_im[n*(channels*width*height) + x*channels + c];
            }
        }
        
    }

}

/*
    Can re-use this for Batch2Space as well
*/
void apply_filter_single_channel_nchw(float* filter, float* img, float* img_out, int size, int ksize) {
    
    for (int h = 0; h < size-ksize+1; h++)
    {
        for (int w = 0; w < size-ksize+1; w++)
        {
            for (int j = 0; j < ksize; j++)
            { 
                for (int k = 0; k < ksize; k++)
                {
                    img_out[w +h*(size-ksize+1)] += filter[(j*ksize)+k] * img[(h+j)*size + w + k];
                }

            }
        }
    }
}