// System Headers
#include <sys/mman.h>

// Standard Library
#include <iostream>
#include <string>
#include <cstring>

// User Headers
#include "matmul.hpp"
#include "util.hpp"
#include "im2col.hpp"




#define DIMENSION 2048
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

  if (!api->Compile(FileToString("./configs/transpose_2048x2048.dtl"))) {
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
      matmul_opt5_recursive_pretranspose(A, bT, C, DIMENSION, 64);
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
    default:
    {
      printf("matrix_benchmark %d not implemented\n", benchmark);
    }
  }
}


void im2col_benchmark(int benchmark, DTL::EphemeralRegion* ephemeral, DTL::API* api)
{
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
    default:
      printf("im2col_benchmark %d not implemented\n", benchmark);
  }
}




int main(int argc, char* argv[]) {
  auto hwStat = new DTL::AGUHardwareStat(4, 4, 5, 5, 6, 4, 3, 1);
  auto api = new DTL::API(hwStat);
  if (api->GetError() != 0)
  {
    printf("DTL::API::HASERROR\n");
    return 0;
  }
  auto ephemeral = api->AllocEphemeralRegion(0x10000000ULL);
  //int alloc_size = DIMENSION * DIMENSION * sizeof(float);
  //float *A = (float *)malloc(alloc_size);
  
  
  // im2col output = 53MB for 1920x1080
  


  
  //float* Bt;
  //float* B = (float*)malloc(alloc_size);
  //float* C = (float*)malloc(alloc_size);
  //if (A == nullptr || B == nullptr || C == nullptr)
  //{
  //  printf("0x%x, 0x%x, 0x%x", A, B, C);
  //}
  //
  //
  //BENCH(matmult_opt3_transposed(A, B, C, &Bt, DIMENSION));

  
  //init_data(A, Bw, C, DIMENSION);


 




  assert(argc > 1);
  if (strcmp(argv[1], "--matrix") == 0)
  {
      assert(argc == 3);
      int benchmark = std::stoi(argv[2]);
      matrix_benchmark(benchmark, ephemeral, api);
  }
  else if (strcmp(argv[1], "--im2col") == 0)
  {
    assert(argc == 3);
    int benchmark = std::stoi(argv[2]);
    im2col_benchmark(benchmark, ephemeral, api);
  }
  else if (strcmp(argv[1], "--db") == 0)
  {
    printf("no db benchmark implemented yet.\n");
  }
  else
  {
    printf("Usage: ./dtlbench --[matrix, im2col, db] <benchmark>\n");
    return 0;
  }
  



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