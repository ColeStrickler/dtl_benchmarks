
#include <stdint.h>
#include <stdio.h>

#include <unistd.h>
#include <random>
#include <algorithm>    // for std::generate



void randomize_region_deterministic_float(float *data, size_t size) {
    if (!data || size == 0) return;

    std::mt19937 gen(12345);
    std::uniform_real_distribution<float> dist(0.0f, 10.0f);
    
    std::generate(data, data + size, [&]() {
        return dist(gen);
    });
}




template <typename T>
void vol2col(
    const T* data_vol,
    const int64_t channels,
    const int64_t depth,
    const int64_t height,
    const int64_t width,
    const int64_t depth_col,
    const int64_t height_col,
    const int64_t width_col,
    const int64_t kT,
    const int64_t kernel_height,
    const int64_t kernel_width,
    const int64_t pT,
    const int64_t pH,
    const int64_t pW,
    const int64_t dT,
    const int64_t dH,
    const int64_t dW,
    const int64_t dilationT,
    const int64_t dilationH,
    const int64_t dilationW,
    T* data_col) {
        int x =0;
  int64_t c, t, h, w;
  int64_t channels_col = channels * kT * kernel_height * kernel_width;
  for (c = 0; c < channels_col; ++c) {
    int64_t w_offset = c % kernel_width; // kw
    int64_t h_offset = (c / kernel_width) % kernel_height; // kh
    int64_t t_offset = (c / kernel_width / kernel_height) % kT; // kT
    int64_t c_vol = c / kT / kernel_height / kernel_width; // c
    for (t = 0; t < depth_col; ++t) { // 
      int64_t t_pad = t * dT - pT + t_offset * dilationT;
      for (h = 0; h < height_col; ++h) {
        int64_t h_pad = h * dH - pH + h_offset * dilationH;
        for (w = 0; w < width_col; ++w) {
          int64_t w_pad = w * dW - pW + w_offset * dilationW;
          if (t_pad >= 0 && t_pad < depth && h_pad >= 0 && h_pad < height &&
              w_pad >= 0 && w_pad < width)
              {
                // c <- kt <- kh <- kw t <- h <- w
                data_col[((c * depth_col + t) * height_col + h) * width_col + w] =
                data_vol
                    [((c_vol * depth + t_pad) * height + h_pad) * width +
                     w_pad];

              }

          else
            data_col[((c * depth_col + t) * height_col + h) * width_col + w] =
                0;
        }
      }
    }
  }
}

int main()
{
    int i = 0;
    int depth = 16;
    int height = 128;
    int width = 128;
    int channels = 3;
    int kT = 3;          // kernel temporal depth
    int kH = 3;          // kernel height
    int kW = 3;          // kernel width

    int pT = 0;          // padding temporal
    int pH = 0;          // padding height
    int pW = 0;          // padding width

    int dT = 1;          // stride temporal
    int dH = 1;          // stride height
    int dW = 1;          // stride width

    int dilationT = 1;    // dilation temporal
    int dilationH = 1;    // dilation height
    int dilationW = 1;    // dilation width

    int insize = kT*kH*kW*channels*depth*height*width;
    float* in = new float[insize];
    randomize_region_deterministic_float(in, insize);
    float* out1 = new float[kT*kH*kW*channels*depth*height*width];
    float* out2 = new float[kT*kH*kW*channels*depth*height*width];
    
    for (int c = 0; c < channels; c++) {
    for (int kt = 0; kt < kT; kt++) {
    for (int kh = 0; kh < kH; kh++) {
    for (int kw = 0; kw < kW; kw++) {
        
           
                
                    for (int t = 0; t < depth - kT+1; t++) {
                        for (int h = 0; h < height-kH+1; h++) {
                            for (int w = 0; w < width-kW+1; w++) {
                                out1[i++] = in[(c*(depth*height*width) + (w + kw))
                                        + ((t + kt)*(height*width)
                                        + (h + kh)*width)];
                            }
                        }
                    }
                }
            }
        }
    }
    
    int depth_col  = depth  - kT+1;
    int height_col = height - kH+1;
    int width_col  = width - kW+1;


    vol2col<float>(
        in,
        channels,
        depth,
        height,
        width,
        depth_col,
        height_col,
        width_col,
        kT,
        kH,
        kW,
        pT,
        pH,
        pW,
        dT,
        dH,
        dW,
        dilationT,
        dilationH,
        dilationW,
        out2
    );


    for (int i = 0; i < insize; i++) {

        if (out1[i] != out2[i])
            printf("%d\n", i);
       // printf("%.3f vs. %.3f\n", out1[i], out2[i]);
    }




}