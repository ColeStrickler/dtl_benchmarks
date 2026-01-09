#include "benchmarks.hpp"




/*
    Function to parameterize the DTL configuration
*/
std::string benchmark::InsertDTLConfigParameters(const BenchmarkData& benchmark_data)
{
    auto dtl_parameterized_config = dtl_stencil_configs[benchmark_data.name];
    auto& params = benchmark_data.params;

    for (auto& [key, value] : params) {
        std::string token = "[" + key + "]";
        size_t pos;
        while ((pos = dtl_parameterized_config.find(token)) != std::string::npos)
            dtl_parameterized_config.replace(pos, token.size(), std::to_string(value));
    }

    return dtl_parameterized_config;
}

std::string benchmark::CreateConstants(const std::unordered_map<std::string, BenchParam> &constants) {
    std::string ret;

    for (auto& e: constants)
    {
        std::string constDecl = "int " + e.first + " = ";
        assert(e.second.size() >= 1);
        if (e.second.size() > 1) // an array
        {
            constDecl += "{";
            for (auto& x: e.second)
                constDecl += std::to_string(x) + ",";
            constDecl.pop_back(); // remove last comma;
            constDecl += "}";
        }
        else
        {
            constDecl += std::to_string(e.second[0]);
        }
        constDecl += ";\n";
        ret += constDecl;
    }

    return ret;
}

std::string benchmark::CreateBenchmarkConfig(const BenchmarkData &benchmark_data)
{
    std::string constants = CreateConstants(benchmark_data.constants);
    std::string config_body = InsertDTLConfigParameters(benchmark_data);

    return constants + "\n"+ config_body;
}

std::string benchmark::bench_wrapper_im2col(const BenchmarkData &bench_data, DTL::API* api)
{
    printf("here\n");
    std::string results;
    
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);
    std::string img_info  = std::to_string(bench_data.constants.at("height")[0]) + "_" + \
        std::to_string(bench_data.constants.at("width")[0]) + "_" + std::to_string(bench_data.params.at("CHANNELS"));

    printf("here2\n");

    assert(bench_data.params.find("CHANNELS") != bench_data.params.end());
    assert(bench_data.other.find("CHANNELS_OUT") != bench_data.other.end());
    assert(bench_data.params.find("KW") != bench_data.params.end());
    assert(bench_data.constants.find("height") != bench_data.constants.end());
    assert(bench_data.constants.find("width") != bench_data.constants.end());



    int channels_in = bench_data.params.at("CHANNELS");
    int channels_out = bench_data.other.at("CHANNELS_OUT");
    int ksize = bench_data.params.at("KW");
    int height = bench_data.constants.at("height")[0];
    int width = bench_data.constants.at("width")[0];
    int stride = 1;
    int pad = 0;
    
    int out_height = (height + 2*pad -  ksize) / stride + 1;
    int out_width = (width  + 2*pad - ksize) / stride + 1;

    int M = channels_out;             
    int K = channels_in * ksize*ksize; 
    int N = out_height * out_width; 

    uint32_t in_data_size = height*width*channels_in;
    uint32_t out_data_size = out_height*out_width*channels_out;
    uint32_t im2col_matsize = channels_in*(height-ksize+1)*(width-ksize+1)*ksize*ksize;
          unsigned int buf_size = 3*1078*3*3*1918;

    float* im2col_cpu_matrix = new float[im2col_matsize];
    float* in_data = new float[in_data_size];
    float* filter_matrix = new float[channels_out*channels_in*ksize*ksize]; // each C_Out applies a ksize*ksize filter to each C_In

    float* outbuf = new float[out_height*out_width*channels_out]; 
    float* outbuf2 = new float[out_height*out_width*channels_out];
    // setup input data
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(in_data), in_data_size*sizeof(float));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(filter_matrix), channels_out*channels_in*ksize*ksize*sizeof(float));


    // CPU benchmark
    printf("startingf im2col_cpu\n");
    perf.CollectCounters();
    im2col_cpu(in_data, channels_in, height, width, ksize, stride, pad, im2col_cpu_matrix);
    printf("im2col done\n");
    matmult_conv_blocked(filter_matrix, im2col_cpu_matrix, outbuf, M, K, N);
    perf.CollectDelta();
    results += "im2col_cpu_" + img_info + "," + perf.PrintCounters() + "\n";
    printf("done cpu\n");
    if (bench_data.output_artifact.size())
        dump_buffer_to_disk("./artifacts/im2col_cpu_" + img_info, reinterpret_cast<unsigned char*>(outbuf), out_height*out_width*channels_out*sizeof(float));
    // CPU benchmark

    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(im2col_matsize*sizeof(float));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    float* Aw = (float*)ephemeral->GetHeadlessWriteregion();
    float* Ar = (float*)ephemeral->GetHeadlessReadRegion();
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), in_data_size*sizeof(float));



    // DTU Benchmark
    perf.CollectCounters();
    matmult_conv_blocked(filter_matrix, Ar, outbuf2, M, K, N);
    perf.CollectDelta();

    results += "im2col_dtu_" + img_info + "," + perf.PrintCounters() + "\n";
    if (bench_data.output_artifact.size())
        dump_buffer_to_disk("./artifacts/im2col_dtu_" + img_info, reinterpret_cast<unsigned char*>(outbuf2), out_height*out_width*channels_out*sizeof(float));

    // DTU Benchmark
    

    // perhaps wrap these in smart pointers
    delete[] in_data;
    delete[] outbuf;
    delete[] outbuf2;
    delete[] im2col_cpu_matrix;
    delete[] filter_matrix;
    api->FreeEphemeralRegion(ephemeral);

    return results;
}

std::string benchmark::bench_wrapper_db_colproject(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);


    assert(bench_data.params.find("ROWS") != bench_data.params.end());
    //assert(bench_data.other.find("CHANNELS_OUT") != bench_data.other.end());
    assert(bench_data.params.find("COLUMNS") != bench_data.params.end());
    assert(bench_data.constants.find("row_size") != bench_data.constants.end());
    assert(bench_data.constants.find("col_offsets") != bench_data.constants.end());



    int row_count = bench_data.params.at("ROWS");
    int col_count = bench_data.params.at("COLUMNS");
    int row_size = bench_data.constants.at("row_size")[0]; // row size in bytes
    std::vector<uint32_t> col_offsets = bench_data.constants.at("col_offsets");
    std::string db_info = std::to_string(row_count) + "_" + std::to_string(col_count) + "_" + std::to_string(row_size);


    uint32_t db_size = row_size*row_count; // db size in bytes
    auto db_cpu_raw = new uint8_t[db_size];
    uint32_t* db_cpu = reinterpret_cast<uint32_t*>(db_cpu_raw);
    uint32_t* write_out1 = new uint32_t[col_count*row_count];
    uint32_t* write_out2 = new uint32_t[col_count*row_count];
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(db_cpu), db_size); // write dummy data to database
    
    perf.CollectCounters();
    int out_write_index1 = 0;
    for (int i = 0; i < row_count; i++)
    {
        for (int j = 0; j < col_count; j++)
        {
            write_out1[out_write_index1++] = READ_UINT32(((uint64_t)db_cpu) + i*row_size + col_offsets[j]);
        }
    }
    perf.CollectDelta();
    results += "dbcolproj_cpu_" + db_info + "," + perf.PrintCounters() + "\n";

    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(db_size);

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    uint32_t* Aw = (uint32_t*)ephemeral->GetHeadlessWriteregion();
    uint32_t* Ar = (uint32_t*)ephemeral->GetHeadlessReadRegion();
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), db_size);

    int sum_col_size = col_count*sizeof(uint32_t);
    perf.CollectCounters();
    int out_write_index2 = 0;
    for (int i = 0; i < row_count; i++)
    {
        for (int j = 0; j < col_count; j++)
        {
            write_out2[out_write_index2++] = READ_UINT32(((uint64_t)Ar) + i*sum_col_size + j*sizeof(uint32_t));
        }
    }
    perf.CollectDelta();
    std::string counters = perf.PrintCounters();
    results += "dbcolproj_dtu_" + db_info + "," + counters + "\n";



    delete[] db_cpu_raw;
    delete[] write_out1;
    delete[] write_out2;
    api->FreeEphemeralRegion(ephemeral);

    return results;
}

std::string benchmark::bench_wrapper_matmul_transpose(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);




    assert(bench_data.params.find("NROWS") != bench_data.params.end());
    //assert(bench_data.other.find("CHANNELS_OUT") != bench_data.other.end());
    assert(bench_data.params.find("NCOLS") != bench_data.params.end());
    assert(bench_data.constants.find("row_size") != bench_data.constants.end());
    assert(bench_data.constants.find("col_size") != bench_data.constants.end());
    
    int nrows = bench_data.params.at("NROWS");
    int ncols = bench_data.params.at("NCOLS");
    assert(nrows == ncols); // assume square matrices
    std::string mat_info = std::to_string(nrows) + "_" + std::to_string(ncols);

    int* A =  new int[nrows*ncols];
    int* B =  new int[nrows*ncols];
    int* Bt = new int[nrows*ncols];
    int* C1 = new int[nrows*ncols];
    int* C2 = new int[nrows*ncols];
    init_data_int(A, B, C1, nrows);

    perf.CollectCounters();
    transpose_naive_int(B, Bt, nrows, ncols);
    matmult_opt3_pretransposed_int(A, Bt, C1, nrows);
    perf.CollectDelta();
    results += "matmul_transpose_cpu_" + mat_info + "," + perf.PrintCounters() + "\n";


    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(nrows*ncols*sizeof(int));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    int* Aw = (int*)ephemeral->GetHeadlessWriteregion();
    int* Ar = (int*)ephemeral->GetHeadlessReadRegion();

    init_data_int(A, Aw, C2, nrows);

    perf.CollectCounters();
    matmult_opt3_pretransposed_int(A, Ar, C2, nrows);
    perf.CollectDelta();
    results += "matmul_transpose_dtu_" + mat_info + "," + perf.PrintCounters() + "\n";




    delete A;
    delete B;
    delete Bt;
    delete C1;
    delete C2;
    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string benchmark::bench_wrapper_nhwc_permutation(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);


    assert(bench_data.params.find("BATCH_SIZE") != bench_data.params.end());
    assert(bench_data.params.find("CHANNELS") != bench_data.params.end());
    assert(bench_data.params.find("SIZE_SQUARED") != bench_data.params.end());
    assert(bench_data.constants.find("size") != bench_data.constants.end());
    assert(bench_data.constants.find("channels") != bench_data.constants.end());
    assert(bench_data.constants.find("data_size") != bench_data.constants.end());
    assert(bench_data.other.find("use_real_image") != bench_data.other.end());
    assert(bench_data.other.find("ksize") != bench_data.other.end());


    int batch_size = bench_data.params.at("BATCH_SIZE");
    int channels = bench_data.constants.at("channels")[0];
    int img_size = bench_data.constants.at("size")[0]; // square this
    int data_size = bench_data.constants.at("data_size")[0];
    int pad = 0;
    bool use_real_image = bench_data.other.at("use_real_image") == 1;
    int ksize = bench_data.other.at("ksize");
    int out_height =    (img_size + 2*pad -  ksize) / 1 + 1;
    int out_width =     (img_size  + 2*pad - ksize) / 1 + 1;

    std::string img_info = std::to_string(img_size) + "x" + std::to_string(img_size)\
         + "_k" + std::to_string(ksize) + "x" + std::to_string(ksize) + "_" + std::to_string(batch_size);

    float* base_img; // nhwc
    float* img_nchw = new float[batch_size*img_size*img_size*channels]; // after permutation
    float* img_out = new float[batch_size*img_size*img_size*channels]; // after convolution
    float* img_out2 = new float[batch_size*img_size*img_size*channels]; // after convolution using dtu
    float* filter = new float[ksize*ksize];
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(filter), ksize*ksize*sizeof(float));

    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        base_img = new float[batch_size*img_size*img_size*channels];
        randomize_region_deterministic(reinterpret_cast<uint8_t*>(base_img), batch_size*img_size*img_size*channels*sizeof(float));
    }
        

    perf.CollectCounters();
    nhwc_to_nchw_cpu(base_img, batch_size, channels, img_size, img_size, img_nchw);
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &img_nchw[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_out[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "nhwc_permutation_cpu_" + img_info + "," + perf.PrintCounters() + "\n";


    
    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(batch_size*img_size*img_size*channels*sizeof(float));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    float* Aw = (float*)ephemeral->GetHeadlessWriteregion();
    float* Ar = (float*)ephemeral->GetHeadlessReadRegion();


    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), batch_size*img_size*img_size*channels*sizeof(float));
    }

    perf.CollectCounters();
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &Ar[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_out2[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "nhwc_permutation_dtu_" + img_info + "," + perf.PrintCounters() + "\n";



    delete[] base_img;
    delete[] img_out;
    delete[] img_out2;
    delete[] img_nchw;
    delete[] filter;
    api->FreeEphemeralRegion(ephemeral);

    return results;
}

std::string benchmark::bench_wrapper_batch2space(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);


    assert(bench_data.params.find("BATCH_SIZE") != bench_data.params.end());
    assert(bench_data.params.find("CHANNELS") != bench_data.params.end());
    assert(bench_data.params.find("HEIGHT") != bench_data.params.end());
    assert(bench_data.params.find("WIDTH") != bench_data.params.end());
    assert(bench_data.constants.find("size") != bench_data.constants.end());
    assert(bench_data.constants.find("channels") != bench_data.constants.end());
    assert(bench_data.constants.find("data_size") != bench_data.constants.end());
    assert(bench_data.other.find("use_real_image") != bench_data.other.end());
    assert(bench_data.other.find("ksize") != bench_data.other.end());


    int batch_size = bench_data.params.at("BATCH_SIZE");
    int channels = bench_data.constants.at("channels")[0];
    int img_size = bench_data.constants.at("size")[0]; // square this
    int data_size = bench_data.constants.at("data_size")[0];
    int pad = 0;
    bool use_real_image = bench_data.other.at("use_real_image") == 1;
    int ksize = bench_data.other.at("ksize");
    int out_height =    (img_size + 2*pad -  ksize) / 1 + 1;
    int out_width =     (img_size  + 2*pad - ksize) / 1 + 1;

    std::string img_info = std::to_string(img_size) + "x" + std::to_string(img_size)\
         + "_k" + std::to_string(ksize) + "x" + std::to_string(ksize) + "_" + std::to_string(batch_size);

    float* base_img; // nhwc
    float* img_batch = new float[batch_size*img_size*img_size*channels]; // after permutation
    float* img_batch_transform = new float[batch_size*img_size*img_size*channels]; // after convolution
    float* img_batch_out = new float[batch_size*img_size*img_size*channels]; // after convolution
    float* img_batch_out2 = new float[batch_size*img_size*img_size*channels]; // after convolution using dtu
    float* filter = new float[ksize*ksize];
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(filter), ksize*ksize*sizeof(float));

    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        base_img = new float[batch_size*img_size*img_size*channels];
        randomize_region_deterministic(reinterpret_cast<uint8_t*>(base_img), batch_size*img_size*img_size*channels*sizeof(float));
    }
        

    perf.CollectCounters();
    //nhwc_to_nchw_cpu(base_img, batch_size, channels, img_size, img_size, img_nchw);
    batch_to_space(base_img, batch_size, channels, img_size, img_size, img_batch_transform);
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &img_batch_transform[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_batch_out[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "batch2space_cpu_" + img_info + "," + perf.PrintCounters() + "\n";


    
    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(batch_size*img_size*img_size*channels*sizeof(float));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    float* Aw = (float*)ephemeral->GetHeadlessWriteregion();
    float* Ar = (float*)ephemeral->GetHeadlessReadRegion();


    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), batch_size*img_size*img_size*channels*sizeof(float));
    }

    perf.CollectCounters();
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &Ar[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_batch_out2[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "batch2space_dtu_" + img_info + "," + perf.PrintCounters() + "\n";



    delete[] base_img;
    delete[] img_batch;
    delete[] img_batch_out;
    delete[] img_batch_out2;
    delete[] img_batch_transform;
    delete[] filter;
    api->FreeEphemeralRegion(ephemeral);

    return results;
}

std::string benchmark::bench_wrapper_tensorunfold(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);

    assert(bench_data.params.find("D1") != bench_data.params.end());
    assert(bench_data.params.find("D2") != bench_data.params.end());
    assert(bench_data.params.find("D3") != bench_data.params.end());
    assert(bench_data.params.find("D4") != bench_data.params.end());
    assert(bench_data.constants.find("stride_d1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d2") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d3") != bench_data.constants.end());
    assert(bench_data.constants.find("data_size") != bench_data.constants.end());

    int d1 = bench_data.params.at("D1");
    int d2 = bench_data.params.at("D2");
    int d3 = bench_data.params.at("D3");
    int d4 = bench_data.params.at("D4");
    int stride_d1 = bench_data.constants.at("stride_d1")[0];
    int stride_d2 = bench_data.constants.at("stride_d2")[0];
    int stride_d3 = bench_data.constants.at("stride_d3")[0];

    int* tensor_in = new int[d1*d2*d3*d4];
    int* lowered_mat = new int[d1*d2*d3*d4];
    int* tensor_out1 = new int[d1*d2*d3*d4];
    int* tensor_out2 = new int[d1*d2*d3*d4];
    int* matrix_b = new int[d1*d2*d3*d4];
    int* matrix_b2 = new int[d1*d2*d3*d4];


    assert(tensor_in != nullptr);
    assert(lowered_mat != nullptr);
    assert(tensor_out1 != nullptr);
    assert(tensor_out2 != nullptr);
    assert(matrix_b != nullptr);
    assert(matrix_b2 != nullptr);


    randomize_region_deterministic(reinterpret_cast<uint8_t*>(tensor_in), d1*d2*d3*d4*sizeof(int));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(matrix_b), d1*d2*d3*d4*sizeof(int));

    std::string tensor_info = std::to_string(d1) + "x" + std::to_string(d2) + "x" + std::to_string(d3) + "x" + std::to_string(d4);


    perf.CollectCounters();
    lower_mat_4d(lowered_mat, tensor_in, d1, d2, d4, d4, stride_d1, stride_d2, stride_d3);
    hadamard(tensor_out1, lowered_mat, matrix_b, d3, d1*d2*d4);
    perf.CollectDelta();
    results += "tensor_unfold_cpu_" + tensor_info + "," + perf.PrintCounters() + "\n";

    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(d1*d2*d3*d4*sizeof(int));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    int* Aw = (int*)ephemeral->GetHeadlessWriteregion();
    int* Ar = (int*)ephemeral->GetHeadlessReadRegion();

    randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), d1*d2*d3*d4*sizeof(int));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(matrix_b2), d1*d2*d3*d4*sizeof(int));


    perf.CollectCounters();
    hadamard(tensor_out2, Ar, matrix_b2, d3, d1*d2*d4);
    perf.CollectDelta();
    results += "tensor_unfold_dtu_" + tensor_info + "," + perf.PrintCounters() + "\n";
    printf("%s\n", results.c_str());
    //delete[] tensor_in;
    printf("a\n");
    delete[] lowered_mat; 
    printf("b\n");
    delete[] tensor_out1;
    printf("c\n");  
    delete[] tensor_out2;
    printf("d\n");
    delete[] matrix_b;
    printf("e\n"); 
    delete[] matrix_b2; 
    printf("f\n");
    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string benchmark::bench_wrapper_tensorslice(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);

    assert(bench_data.params.find("D1") != bench_data.params.end());
    assert(bench_data.params.find("D2") != bench_data.params.end());
    assert(bench_data.params.find("D3") != bench_data.params.end());
    assert(bench_data.params.find("D4") != bench_data.params.end());
    assert(bench_data.constants.find("stride_n1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_h1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_w1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_c1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d2") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d3") != bench_data.constants.end());
    assert(bench_data.constants.find("data_size") != bench_data.constants.end());
    int d1 = bench_data.params.at("D1");
    int d2 = bench_data.params.at("D2");
    int d3 = bench_data.params.at("D3");
    int d4 = bench_data.params.at("D4");
    int stride_d1 = bench_data.constants.at("stride_d1")[0];
    int stride_d2 = bench_data.constants.at("stride_d2")[0];
    int stride_d3 = bench_data.constants.at("stride_d3")[0];

    int stride_n1 = bench_data.constants.at("stride_n1")[0];
    int stride_h1 = bench_data.constants.at("stride_h1")[0];
    int stride_w1 = bench_data.constants.at("stride_w1")[0];
    int stride_c1 = bench_data.constants.at("stride_c1")[0];

    int n1 = d4 / stride_n1;
    int h1 = d3 / stride_h1;
    int w1 = d2 / stride_w1;
    int c1 = d1 / stride_c1;   


    int* tensor_in = new int[d1*d2*d3*d4];
    int* sliced_tensor = new int[n1*h1*w1*c1];
    int* tensor_out1 = new int[n1*h1*w1*c1];
    int* tensor_out2 = new int[n1*h1*w1*c1];
    int* tensor_b = new int[n1*h1*w1*c1];
    int* tensor_b2 = new int[n1*h1*w1*c1];
    assert(tensor_in != nullptr);
    assert(sliced_tensor != nullptr);
    assert(tensor_out1 != nullptr);
    assert(tensor_out2 != nullptr);
    assert(tensor_b != nullptr);
    assert(tensor_b2 != nullptr);


    randomize_region_deterministic(reinterpret_cast<uint8_t*>(tensor_in), d1*d2*d3*d4*sizeof(int));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(tensor_b), n1*h1*w1*c1*sizeof(int));

    std::string tensor_info = std::to_string(d1) + "x" + std::to_string(d2) + "x" + std::to_string(d3) + "x" + std::to_string(d4);
    tensor_info += " _" + std::to_string(stride_n1) + "x" + std::to_string(stride_h1) + "x" + std::to_string(stride_w1) + "x" + std::to_string(stride_c1);

    perf.CollectCounters();
    slice_tensor_int(sliced_tensor, tensor_in, n1, h1, w1, c1, stride_n1, stride_h1, stride_w1, stride_c1, stride_d1, stride_d2, stride_d3);
    hadamard_tensor_4d_int(tensor_out1, sliced_tensor, tensor_b, n1, h1, w1, c1);
    perf.CollectDelta();
    results += "tensor_slicing_cpu_" + tensor_info + "," + perf.PrintCounters() + "\n";

    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(d1*d2*d3*d4*sizeof(int));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    int* Aw = (int*)ephemeral->GetHeadlessWriteregion();
    int* Ar = (int*)ephemeral->GetHeadlessReadRegion();

    randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), d1*d2*d3*d4*sizeof(int));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(tensor_b2), n1*h1*w1*c1*sizeof(int));


    perf.CollectCounters();
    hadamard_tensor_4d_int(tensor_out2, Ar, tensor_b2, n1, h1, w1, c1);
    perf.CollectDelta();
    results += "tensor_slicing_dtu_" + tensor_info + "," + perf.PrintCounters() + "\n";

    delete[] tensor_in;
    delete[] sliced_tensor; 
    delete[] tensor_out1;     
    delete[] tensor_out2;
    delete[] tensor_b; 
    delete[] tensor_b2; 

    api->FreeEphemeralRegion(ephemeral);
    return results;
}