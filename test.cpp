
#include "src/matmul.hpp"
#include "src/im2col.hpp"

int main()
{
      int insize = 64; // 64x64
      int C_in = 128; // channels in
      int ksize = 4;
      int pad = 0;
      int stride = 1;
      int out_height = (insize + 2*pad -  ksize) / stride + 1;
      int out_width = (insize  + 2*pad - ksize) / stride + 1;
      int C_out = 256;

      float* chw_img_buf = new float[insize*insize*C_in]; // C_in insize*insize input filters
      float* filter_matrix = new float[C_out*C_in*ksize*ksize]; // each C_Out applies a ksize*ksize filter to each C_In

      std::mt19937 rng(1234);  // fixed seed
      std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
      for (int i = 0; i < C_out*C_in*ksize*ksize; i++)
        filter_matrix[i] = dist(rng);

      for (int i = 0; i < insize*insize*C_in; i++)
        chw_img_buf[i] = dist(rng);



      float* im2col_cpu_matrix = new float[out_height*out_width*C_in*ksize*ksize]; // transformed im2col






      float* outbuf = new float[out_height*out_width*C_out]; 
      float* outbuf2 = new float[out_height*out_width*C_out];
      float* outbuf3 = new float[out_height*out_width*C_out];
      

      

      // run default conv
      auto start = std::chrono::high_resolution_clock::now();
      naive_conv(chw_img_buf, C_in, insize, filter_matrix, C_out, ksize, outbuf, out_height, out_width);
      auto end = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(end - start).count();
      printf("naive_conv chsum=%.6f, elapsed=%.6f\n", print_checksum_l(outbuf, out_height*out_width*C_out), elapsed);





      // run im2col_cpu
      // im2col_cpu_matrix: (C_in * ksize * ksize) × (out_height * out_width)
      int M = C_out;             // 256
      int K = C_in * ksize*ksize; // 128*4*4 = 2048
      int N = out_height * out_width; // 61*61 = 3721
      start = std::chrono::high_resolution_clock::now();
      im2col_cpu(chw_img_buf, C_in, insize, insize, ksize, 1, 0, im2col_cpu_matrix);
      matmult_conv_blocked(filter_matrix, im2col_cpu_matrix, outbuf2, M, K, N);
      end = std::chrono::high_resolution_clock::now();
      elapsed = std::chrono::duration<double>(end - start).count();
      printf("im2col chsum=%.6f, elapsed=%.6f\n", print_checksum_l(outbuf2, out_height*out_width*C_out), elapsed);
      // run im2col_dtu

    start = std::chrono::high_resolution_clock::now();
    




  return 0;
}