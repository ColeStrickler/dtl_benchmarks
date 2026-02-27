// System Headers
#include <sys/mman.h>

// Standard Library
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <pthread.h>
#include <sched.h>

// User Headers

#include "benchmarks.hpp"

#include "matmul.hpp"
#include "util.hpp"
#include "im2col.hpp"
#include "dtl_api.hpp"



#define DIMENSION 640
#define RME_SHIFT 0x8000000UL
#define TO_RME(addr) (((uint64_t)addr) + RME_SHIFT)


void toFile(const std::string &file, float *res, int dimension) {
  std::ofstream ofs(file);
  if (!ofs.is_open()) {
    // Handle error opening file
    return;
  }

  for (int i = 0; i < dimension; i++) {
    for (int j = 0; j < dimension; j++) {
      ofs << std::to_string(i) << "," << std::to_string(j) << ": "
          << res[i * dimension + j] << "\n";
    }
  }
  ofs.close();
}





void toFile_int(const std::string &file, int *res, int dimension) {
  std::ofstream ofs(file);
  if (!ofs.is_open()) {
    // Handle error opening file
    return;
  }

  for (int i = 0; i < dimension; i++) {
    for (int j = 0; j < dimension; j++) {
      ofs << std::to_string(i) << "," << std::to_string(j) << ": "
          << res[i * dimension + j] << "\n";
    }
  }
  ofs.close();
}


void matrix_benchmark(int benchmark, DTL::EphemeralRegion* ephemeral, DTL::API* api)
{
  int alloc_size = DIMENSION * DIMENSION * sizeof(float);
  float *A = (float *)malloc(alloc_size);
  float *B = (float *)malloc(alloc_size);
  float *C = (float *)malloc(alloc_size);
  float* ew = (float*)ephemeral->GetHeadlessWriteregion();
  float* er = (float*)ephemeral->GetHeadlessReadRegion();

  if (!api->Compile(FileToString("./configs/transpose_640x640.dtl"))) {
    printf("Failed to compile dtl program or map onto agu\n");
    return;
  }
  api->ProgramHardware(ephemeral);
  printf("Successfully programmed agu\n");


  switch (benchmark)
  {
    case 0:
    {
      BENCH(matmult_opt0_naive(A, B, C, DIMENSION)); // no dtl equivalent
      break;
    }
    case 1:
    {
      BENCH(matmult_opt1_jk(A, B, C, DIMENSION));
      break;
    }
    case 2:
    {
      BENCH(matmult_opt2_jk_tiling(A, B, C, DIMENSION)); // 1 deep tiling
      
      break;
    }
    case 3:
    {
      printf("matmul benchmark: matmult_opt3_transposed\n");
      float* bt;
      BENCH(matmult_opt3_transposed(A, B, C, &bt, DIMENSION));
      init_data(A, ew, C, DIMENSION);
      auto start = std::chrono::high_resolution_clock::now();
      matmult_dtl_transposed(A, er, C, DIMENSION);
      auto end = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(end - start).count();
      double checksum = print_checksum(C, DIMENSION);
      printf("%.12s  secs: %.6f  chsum: %.6f\n", "matmult_dtl_transposed", elapsed, checksum);
      break;
    }
    case 4:
    {
      init_data(A, B, C, DIMENSION);
      auto start = std::chrono::high_resolution_clock::now();
      matmul_opt4_recursive(A, B, C, DIMENSION, 64);
      auto end = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(end - start).count();
      double checksum = print_checksum(C, DIMENSION);
      printf("%.12s  secs: %.6f  chsum: %.6f\n", "matmul_opt4_recursive", elapsed, checksum);
      break;
    }
    case 5:
    {
      init_data(A, B, C, DIMENSION);

      auto bT = (float*)malloc(alloc_size);
      transpose_naive(B, bT, DIMENSION, DIMENSION);
      auto start = std::chrono::high_resolution_clock::now();
      matmul_opt5_recursive_pretranspose(A, bT, C, DIMENSION, 64);
      auto end = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(end - start).count();
      double checksum = print_checksum(C, DIMENSION);
      printf("%.12s  secs: %.6f  chsum: %.6f\n", "matmul_opt5_recursive_pretransposed", elapsed, checksum);

      delete A;
      delete B;
      delete C;
      A = (float *)malloc(alloc_size);
      C = (float *)malloc(alloc_size);

      init_data(A, ew, C, DIMENSION);
      start = std::chrono::high_resolution_clock::now();
      matmul_opt5_recursive_pretranspose(A, er, C, DIMENSION, 64);
      end = std::chrono::high_resolution_clock::now();
      elapsed = std::chrono::duration<double>(end - start).count();
      checksum = print_checksum(C, DIMENSION);
      printf("%.12s  secs: %.6f  chsum: %.6f\n", "matmul_opt5_recursive_pretransposed_dtl", elapsed, checksum);


      break;
    }
    case 6:
    {
      init_data(A, B, C, DIMENSION);

      auto bT = (float*)malloc(alloc_size);
      transpose_naive(B, bT, DIMENSION, DIMENSION);
      auto start = std::chrono::high_resolution_clock::now();
      matmult_dtl_transposed_tile(DIMENSION, 512, 32, A, bT, C);
      auto end = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(end - start).count();
      double checksum = print_checksum(C, DIMENSION);
      printf("%.12s  secs: %.6f  chsum: %.6f\n", "matmul_opt6_transposed_tile", elapsed, checksum);


      init_data(A, ew, C, DIMENSION);
      start = std::chrono::high_resolution_clock::now();
      // 512x512 float matrix is exactly 1MB, 32x32 float matrix is exactly 16KB
      matmult_dtl_transposed_tile(DIMENSION, 512, 32, A, er, C);
      end = std::chrono::high_resolution_clock::now();
      elapsed = std::chrono::duration<double>(end - start).count();
      checksum = print_checksum(C, DIMENSION);
      printf("%.12s  secs: %.6f  chsum: %.6f\n", "matmul_opt6_transposed_tile_tme", elapsed, checksum);
      break;
    }
    case 999:
    {
      init_data(A, ew, C, DIMENSION);
      auto start = std::chrono::high_resolution_clock::now();
      matmult_dtl_transposed(A, er, C, DIMENSION);
      auto end = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(end - start).count();
      double checksum = print_checksum(C, DIMENSION);
      //printf("%.12s  secs: %.6f  chsum: %.6f\n", "matmult_dtl_transposed", elapsed, checksum);
      break;
    }
    default:
    {
      printf("matrix_benchmark %d not implemented\n", benchmark);
    }
  }
}


void im2col_benchmark(int benchmark, DTL::EphemeralRegion* ephemeral, DTL::API* api)
{
  printf("im2col_benchmark %d\n", benchmark);
  float* ew = (float*)ephemeral->GetHeadlessWriteregion();
  float* er = (float*)ephemeral->GetHeadlessReadRegion();
  switch(benchmark)
  {
    case 0: // 1920x1080
    {
      int H = 1080, W = 1920, C = 3;
      auto chw_img_buf = GetImageBuf("./tools/image_chw_1920x1080.raw");
      assert(chw_img_buf != nullptr);
      memcpy(ew, chw_img_buf, H*W*C*sizeof(float));
      unsigned int buf_size = 3*1078*3*3*1918;
      float* outbuf = new float[buf_size];
      auto start = std::chrono::high_resolution_clock::now();
      im2col_cpu(chw_img_buf, 3, 1080, 1920, 3, 1, 0, outbuf);
      auto end = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(end - start).count();
      printf("%.12s  secs: %.6f\n", "im2col_cpu_1920x1080", elapsed);
      dump_buffer_to_disk("im2col_cpu.out", (unsigned char*)outbuf, buf_size*sizeof(float));
      delete outbuf;
      if (!api->Compile(FileToString("./configs/im2col_chw_1920x1080.dtl"))) {
        printf("Failed to compile dtl program or map onto agu\n");
        return;
      }
      api->ProgramHardware(ephemeral);
      printf("Successfully programmed agu\n");

      printf("%.12s  secs: %.6f\n", "im2col_dtu_1920x1080", 0.0f);
      float* outbuf2 = new float[buf_size];
      for (int i = 0; i < buf_size; i++)
        outbuf2[i] = er[i];
    // memcpy(outbuf2, er, buf_size*sizeof(float));
      dump_buffer_to_disk("im2col_dtu.out", (unsigned char*)outbuf2, buf_size*sizeof(float));



      break;
    }
    case 1:
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
      //naive_conv(chw_img_buf, C_in, insize, filter_matrix, C_out, ksize, outbuf, out_height, out_width);
      auto end = std::chrono::high_resolution_clock::now();
      double elapsed = std::chrono::duration<double>(end - start).count();
      printf("naive_conv chsum=%.6f, elapsed=%.6f\n", print_checksum_l(outbuf, out_height*out_width*C_out), elapsed);


      // run im2col_cpu
      // im2col_cpu_matrix: (C_in * ksize * ksize) × (out_height * out_width)
      int M = C_out;             // 256
      int K = C_in * ksize*ksize; // 128*4*4 = 2048
      int N = out_height * out_width; // 61*61 = 3721
      start = std::chrono::high_resolution_clock::now();
      //im2col_cpu(chw_img_buf, C_in, insize, insize, ksize, 1, 0, im2col_cpu_matrix);
      //matmult_conv_blocked(filter_matrix, im2col_cpu_matrix, outbuf2, M, K, N);
      end = std::chrono::high_resolution_clock::now();
      elapsed = std::chrono::duration<double>(end - start).count();
      printf("im2col chsum=%.6f, elapsed=%.6f\n", print_checksum_l(outbuf2, out_height*out_width*C_out), elapsed);
      // run im2col_dtu


      if (!api->Compile(FileToString("./configs/im2col_chw_1.dtl"))) {
        printf("Failed to compile dtl program or map onto agu\n");
        return;
      }
      api->ProgramHardware(ephemeral);
      printf("Successfully programmed agu\n");


      float* Aw = (float*)ephemeral->GetHeadlessWriteregion();
      float* Ar = (float*)ephemeral->GetHeadlessReadRegion();
      memcpy(Aw, chw_img_buf, insize*insize*C_in*sizeof(float));


      start = std::chrono::high_resolution_clock::now();
      matmult_conv_blocked(filter_matrix, Ar, outbuf3, M, K, N);
      end = std::chrono::high_resolution_clock::now();
      elapsed = std::chrono::duration<double>(end - start).count();
      printf("im2col_dtu chsum=%.6f, elapsed=%.6f\n", print_checksum_l(outbuf3, out_height*out_width*C_out), elapsed);



    }
    default:
      printf("im2col_benchmark %d not implemented\n", benchmark);
  }
}


#define DEFAULT_ROW_COUNT 43690
#define DEFAULT_COL_SIZE 4
void db_benchmark(int row_size, int col_count, DTL::EphemeralRegion* ephemeral, DTL::API* api, bool log)
{
  std::string dtl_bench_config = "database_row" + std::to_string(row_size) + "_" + std::to_string(col_count) + "col.dtl";
  std::string config_file = "./configs/" + dtl_bench_config;
  if (log)
    printf("Running Database Benchmark for row_size=%d, col_count=%d\n", row_size, col_count);
  uint32_t* db = new uint32_t[(row_size/DEFAULT_COL_SIZE)*DEFAULT_ROW_COUNT];
  
  uint32_t* ew = (uint32_t*)ephemeral->GetHeadlessWriteregion();
  uint32_t* er = (uint32_t*)ephemeral->GetHeadlessReadRegion();
  if (!api->Compile(FileToString(config_file))) {
      printf("Failed to compile dtl program or map onto agu\n");
      return;
  }
  api->ProgramHardware(ephemeral);

  /*
    Initialize data
  */
  std::mt19937 rng(292); // fixed seed for reproducibility
  std::uniform_int_distribution<int> dist(-50,50);
  for (int i = 0; i < (row_size/DEFAULT_COL_SIZE)*DEFAULT_ROW_COUNT; i++)
  {
    auto x = dist(rng);
    db[i] = x;
    ew[i] = x;
  }
  ephemeral->Sync();

  int* col_offsets;



  switch (row_size)
  {
      case 64:
      {
        switch(col_count)
        {
          case 3:
          {
            col_offsets = new int[3];
            int col_offsetsx[] = {4, 32, 48};
            memcpy(col_offsets, col_offsetsx, sizeof(int)*3);
            break;           
          }
          case 11:
          {
            col_offsets = new int[11];
            int col_offsetsx[] = {4, 8, 16, 24, 28, 32, 40, 48, 52, 56, 60};
            memcpy(col_offsets, col_offsetsx, sizeof(int)*11);
            break;    
          }
          default:
          {
            printf("Invalid col_count. valids=(3, 11)\n");
           return;
          }
          
        }
        break;
      }
      case 512:
      {
        switch(col_count)
        {
          case 3:
          {
            col_offsets = new int[3];
            int col_offsetsx[] = {104, 256, 444};
            memcpy(col_offsets, col_offsetsx, sizeof(int)*3);
            break;
          }
          case 11:
          {
            col_offsets = new int[11];
            int col_offsetsx[] = {4, 64, 120, 164, 256, 312, 368, 400, 444, 488, 500};
            memcpy(col_offsets, col_offsetsx, sizeof(int)*11);
            break;
          }
          default:
          {
            printf("Invalid col_count. valids=(3, 11)\n");
            return;
          }
        }
        break;
      }
      default:
      {
        printf("Invalid row size. valids=(64, 512)\n");
        return;
      }
  }

  for (int i = 0; i < col_count; i++)
  {
    if (log)
      printf("col_offset[%d] %d\n", i, col_offsets[i]);
  }
  uint32_t checksum = 0;
  uint32_t* out_array = new uint32_t[DEFAULT_ROW_COUNT*col_count];
  uint32_t out_write_index = 0;
  memset(out_array, 0x00, DEFAULT_ROW_COUNT*col_count*sizeof(uint32_t));
  auto start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < DEFAULT_ROW_COUNT; i++)
  {
    for (int j = 0; j < col_count; j++)
    {
        out_array[out_write_index++] = READ_UINT32(((uint64_t)db) + i*row_size + col_offsets[j]);
    }
  }
  auto end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();
  // get checksum
  for (int x = 0; x < col_count*DEFAULT_ROW_COUNT; x++)
    checksum += out_array[x];
  if (log)
    printf("%.12s  secs: %.6f, chsum: %lu\n", dtl_bench_config.c_str(), elapsed, checksum);
  //delete out_array;

  /*
    Now through DTU
  */
  checksum = 0;
  uint32_t* out_array2 = new uint32_t[DEFAULT_ROW_COUNT*col_count];
  uint32_t out_write_index2 = 0;
  memset(out_array2, 0x00, DEFAULT_ROW_COUNT*col_count*sizeof(uint32_t));
  start = std::chrono::high_resolution_clock::now();
  uint32_t row_span_dtu = col_count*DEFAULT_COL_SIZE;
  for (int i = 0; i < DEFAULT_ROW_COUNT; i++)
  {
    for (int j = 0; j < col_count; j++)
    {
        out_array2[out_write_index2++] = READ_UINT32(((uint64_t)er) + i*row_span_dtu + j*DEFAULT_COL_SIZE);
    }
  }
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration<double>(end - start).count();
  // get checksum
  for (int x = 0; x < col_count*DEFAULT_ROW_COUNT; x++)
    checksum += out_array2[x];
  if (log)
    printf("dtu_%.12s  secs: %.6f, chsum: %lu\n", dtl_bench_config.c_str(), elapsed, checksum);
  delete col_offsets;
}



void set_thread_affinity(int core_id)
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    pthread_t current_thread = pthread_self();
    int rc = pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
    if (rc != 0)
    {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
}


void multi_benchmark(int thread_count, DTL::API* api)
{
  assert(thread_count <= api->GetHWStat()->nMaxConfigs);

  unsigned int num_cores = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;
  threads.reserve(thread_count);
  std::vector<DTL::EphemeralRegion*> ephemeral_vars;
  std::vector<float*> Ax;
  std::vector<float*> Cx;

  for (int i = 0; i < thread_count; i++)
  {
    auto ephemeral = api->AllocEphemeralRegion(0x1000000);
    ephemeral_vars.push_back(ephemeral);
    float* a = new float[640*640];
    float* c = new float[640*640];
    Ax.push_back(a);
    Cx.push_back(c);
  }
    

  if (!api->Compile(FileToString("./configs/transpose_640x640.dtl"))) {
      printf("Failed to compile dtl program or map onto agu\n");
      return;
  }
  


  for (int x = 0; x < thread_count; ++x)
  {
      threads.emplace_back([=]() {
          set_thread_affinity(x % num_cores);
          // Each thread gets its own ephemeral region
          auto ephemeral = ephemeral_vars[x];
          api->ProgramHardware(ephemeral);
          float* ew = (float*)ephemeral->GetHeadlessWriteregion();
          float* er = (float*)ephemeral->GetHeadlessReadRegion();
          auto A = Ax[x];
          auto C = Cx[x];
          init_data(A, ew, C, DIMENSION);
          auto start = std::chrono::high_resolution_clock::now();
          matmult_dtl_transposed(A, er, C, DIMENSION);
          auto end = std::chrono::high_resolution_clock::now();
          double elapsed = std::chrono::duration<double>(end - start).count();
          double checksum = print_checksum(C, DIMENSION);
          printf("%d chsum: %.6f\n", x, checksum);
          if (x == thread_count-1)
            printf("elapsed %d: %.6f\n", x, elapsed);
      });
  }


  // Wait for all threads to finish
  for (auto& t : threads)
  {
      t.join();
  }
}




void hpc_multigrid_benchmark()
{
  
}




int main(int argc, char* argv[]) {







  //auto hwStat = new DTL::AGUHardwareStat(4, 4, 5, 6, 5, 4, 8, 1, 2);
  auto hwStat = new DTL::AGUHardwareStat(4, 4, 5, 6, 5, 4, 1, 1, 2);
  hwStat->nMaxConfigs = 8;
  





  auto api = new DTL::API(hwStat);
  if (api->GetError() != 0)
  {
    printf("DTL::API::HASERROR\n");
    return 0;
  }
  PerfManager perf;
  std::cout << "benchmark,type," << perf.PrintCountersLabel() << "hash" <<"\n";


  //for (auto& bench : benchmark::BenchmarkDispatchDataSplitLayouts)
  //{
  //  std::cout << bench.benchmark(bench, api);
  //}



  for (auto& bench : benchmark::TMEComparisonData)
  {
   // printf("calling bench\n");
    std::cout << bench.benchmark(bench, api);
    
    //api->DebugPrintAllocator();
  }

  return 0;



  if (strcmp(argv[1], "--matrix") == 0)
  {
    auto ephemeral = api->AllocEphemeralRegion(0x1000000ULL);
      assert(argc == 3);
      int benchmark = std::stoi(argv[2]);
      matrix_benchmark(benchmark, ephemeral, api);
  }
  else if (strcmp(argv[1], "--im2col") == 0)
  {
    printf("[benchmark=im2col]\n");
    auto ephemeral = api->AllocEphemeralRegion(0x4000000ULL);
    assert(argc == 3);
    int benchmark = std::stoi(argv[2]);
    im2col_benchmark(benchmark, ephemeral, api);
  }
  else if (strcmp(argv[1], "--db") == 0)
  {
    auto ephemeral = api->AllocEphemeralRegion(0x1000000ULL);
    assert(argc == 4);
    int row_size = std::stoi(argv[2]);
    int col_count = std::stoi(argv[3]);
    db_benchmark(row_size, col_count, ephemeral, api, true);
  }
  else if (strcmp(argv[1], "--multi") == 0) // multithreaded
  {
    int threads = std::stoi(argv[2]);
    multi_benchmark(threads, api);

    // want to be able to run different number of threads on different configs or same config
    //auto ephemeral = api->AllocEphemeralRegion(0x1000000ULL);
    //auto ephemeral2 = api->AllocEphemeralRegion(0x1000000ULL);


  }
  else
  {
    printf("Usage: ./dtlbench --[matrix, im2col, db] <benchmark>\n");
    return 0;
  }

  



  //auto ephemeral = api->AllocEphemeralRegion(0x8000000ULL);
  //auto ephemeral2 = api->AllocEphemeralRegion(0x8000000ULL);





  //auto start = std::chrono::high_resolution_clock::now();
  //matmult_dtl_transposed(A, Br, C, DIMENSION);
  //auto end = std::chrono::high_resolution_clock::now();
  //double elapsed = std::chrono::duration<double>(end - start).count();
  //double checksum = print_checksum(C, DIMENSION);
  //printf("%.12s  secs: %.6f  chsum: %.6f\n", "matmult_dtl_transposed", elapsed, checksum);

  //init_data(A, B, C, DIMENSION);
  //ephemeral->Sync();
  //start = std::chrono::high_resolution_clock::now();
  //matmult_dtl_transposed(A, Br, C, DIMENSION);
  //end = std::chrono::high_resolution_clock::now();
  //elapsed = std::chrono::duration<double>(end - start).count();
  //checksum = print_checksum(C, DIMENSION);
  //printf("%.12s  secs: %.6f  chsum: %.6f\n", "matmult_dtl_transposed", elapsed, checksum);
  //BENCH(matmult_opt1_jk(A, B, C, DIMENSION));
  //BENCH(matmult_opt2_jk_tiling(A, B, C, DIMENSION));
  //BENCH(matmult_opt0_naive(A, B, C, DIMENSION));







  

  // also 

/*
  unsigned long *config = (unsigned long *)mmap(NULL,
                                                RME_CONFIG_SIZE,
                                                PROT_READ | PROT_WRITE,
                                                MAP_SHARED,
                                                hpm_fd,
                                                RME_CONFIG);
  void *agu_config_base =
      mmap(NULL, 0xfff, PROT_READ | PROT_WRITE, MAP_SHARED, hpm_fd, 0x4000000);
  assert(agu_config_base != nullptr);
  unsigned char *plim = (unsigned char *)mmap((void *)0,
                                              RELCACHE_SIZE,
                                              PROT_READ | PROT_WRITE,
                                              MAP_SHARED,
                                              hpm_fd,
                                              RELCACHE_ADDR);

  // we first will allocate matrix B on the rme region
  
  DTL::API api(hwStat);

  int alloc_size = DIMENSION * DIMENSION * sizeof(float);
  float *A = (float *)malloc(alloc_size);
  float *B = (float *)plim;[detached from 767305.fsim0]
c674s876@avalugg:~/FIRESIM_RUNS_DIR/sim_slot_0$ cat uartlog |less
c674s876@avalugg:~/FIRESIM_RUNS_DIR/sim_slot_0$ cat sy

  float *C = (float *)malloc(alloc_size);
  float *Bt;

  // BENCH(matmult_opt0_naive(A, B, C, DIMENSION));
  // BENCH(matmult_opt1_jk(A, B, C, DIMENSION));
  // toFile("opt1.out", C, DIMENSION);
  // BENCH(matmult_opt2_jk_tiling(A, B, C, DIMENSION));
  //  toFile("opt2.out", C, DIMENSION);
  BENCH(matmult_opt3_transposed(A, B, C, &Bt, DIMENSION));
  toFile("opt3A.out", A, DIMENSION);
  toFile("opt3.out", C, DIMENSION);
  toFile("opt3B.out", Bt, DIMENSION);
  */
  // chsum: 1440.415743
  // chsum: 1474.542120
  // chsum: 1452.471111
  // chsum: 1444.091102
  //init_data(A, B, C, DIMENSION);


  
/*

  if (configure_relcache() == -1) {
    printf("Unable to configure relcache\n");
    return -1;
  }

  // flush_cache();

  
  flush_cache();
  EnableRelCache(hpm_fd);

  toFile("optdtlB.out", B, DIMENSION);
  auto start = std::chrono::high_resolution_clock::now();
  matmult_dtl_transposed(A, (float*)TO_RME(B), C, DIMENSION);
  auto end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();
  double checksum = print_checksum(C, DIMENSION);
  toFile("optdtlA.out", A, DIMENSION);
  
  toFile("optdtl.out", C, DIMENSION);
  printf("%.12s  secs: %.6f  chsum: %.6f\n",
         "matmult_dtl_transposed",
         elapsed,
         checksum);
  FlushAndDisable(hpm_fd);
  close(hpm_fd);
  free(A);
  // free(B);
  free(C);
  */

  return 0;
}