#ifndef DTL_BENCH_BATCH2SPACE_HPP
#define DTL_BENCH_BATCH2SPACE_HPP


void batch_to_space(float* data_im, int batch_size,
     int channels,  int height,  int width, float* data_out);

#endif