#include "batch2space.hpp"

void batch_to_space(float *data_im,
                    int batch_size,
                    int channels,
                    int height,
                    int width,
                    float *data_out)
{
    int i = 0;
    // assume chw format
    for (int c = 0; c < channels; c++)
    {
        for (int h = 0; h < height; h++)
        {
            for (int n = 0; n < batch_size; n++)
            {
                for (int w = 0; w < width; w++)
                {
                    data_out[i++] = data_im[w + n*(height*width*channels) + h*width + c*(height*width)]; 
                }
            }
        }
    }

}