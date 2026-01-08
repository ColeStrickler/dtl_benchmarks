#ifndef DTL_BENCH_TENSOR_PERMUTATION_HPP
#define DTL_BENCH_TENSOR_PERMUTATION_HPP









void nhwc_to_nchw_cpu(float *data_im, int batch_size, int channels, int height, int width, float *data_out);
void apply_filter_single_channel_nchw(float* filter, float* img, float* img_out, int size, int ksize);




#endif