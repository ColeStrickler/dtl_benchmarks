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
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(in_data_size);

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

    return results;
}
