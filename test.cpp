
#include "src/matmul.hpp"
#include "src/im2col.hpp"

int main()
{
  

  // chw -> hwc
for (int h = 0; h < height; h++)
{
    for (int w = 0; w < width; w++)
    {
        for (int c = 0; c < channels; c++)
        {
            out2[x] = out[c*height*width + w + h*width];
            x++;
        }
    }
}

  // chw -> hwc reordered_loop
  for (int c = 0; c < channels; c++)
    for (int h = 0; h < height; h++)
      for (int w = 0; w < width; w++)
      {
          out3[x] = out[h * (width * channels) + w * channels + c];
          x++;  // now safe — x is correct
      }







  return 0;
}