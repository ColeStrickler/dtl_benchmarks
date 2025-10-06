// System Headers
#include <sys/mman.h>

// User Headers
#include "matmul.hpp"
#include "util.hpp"

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


int main() {
  auto hwStat = new DTL::AGUHardwareStat(4, 4, 5, 5, 6, 4, 3, 1);
  auto api = new DTL::API(hwStat);
  if (api->GetError() != 0)
  {
    printf("DTL::API::HASERROR\n");
    return 0;
  }



  auto ephemeral = api->AllocEphemeralRegion(64*0x100000);
  int alloc_size = DIMENSION * DIMENSION * sizeof(float);
  float *A = (float *)malloc(alloc_size);
  float* Bw = (float*)ephemeral->GetHeadlessWriteregion();
  float* Br = (float*)ephemeral->GetHeadlessReadRegion();
  float* Bt;
  float* B = (float*)malloc(alloc_size);
  float* C = (float*)malloc(alloc_size);

  if (A == nullptr || B == nullptr || C == nullptr)
  {
    printf("0x%x, 0x%x, 0x%x", A, B, C);
  }


  BENCH(matmult_opt3_transposed(A, B, C, &Bt, DIMENSION));


  init_data(A, Bw, C, DIMENSION);
    double checksum = print_checksum(C, DIMENSION);

  if (!api->Compile(FileToString("./transpose_640x640.dtl"))) {
    printf("Failed to compile dtl program or map onto agu\n");
    return 0;
  }
  api->ProgramHardware(ephemeral);
  printf("Successfully programmed agu\n");
  matmult_dtl_transposed(A, Br, C, DIMENSION);


for (int j = 0; j < 32; j++)
{
  printf("\n\nJ=%d\n\n", j);


  ephemeral->GuardedWrite_8(0, 0x1);
   printf("did write\n");
  // Access with bounds checking

  for (int i = 0; i < 10000; i++)
  {
    uint8_t a = ephemeral->GuardedRead_8(0);
    
//
  //// move to new physical address so writes propagate
  //ephemeral->Sync();
//

  // we shhould now get new data
  uint8_t b = ephemeral->GuardedRead_8(0);
  printf("Check a 0x%x, 0x%x\n", a, b);
  }
  

  // also allow headless acess if needed
  void* ephemeral_raw = ephemeral->GetHeadlessReadRegion();


  

  float p = 0.0f;
  for (int i = 0; i < 100000; i++)
  {
      float x = 5423.0f / (i*i*i*i);
      p += x / 3.0f;
  }
  printf("%.3f\n", p);

 
    ephemeral->GuardedWrite_8(0, 0x2);
    printf("did write\n");
     ephemeral->Sync();
  // Access with bounds checking

  for (int i = 0; i < 10000; i++)
  {
    uint8_t a = ephemeral->GuardedRead_8(0);
    
//
  //// move to new physical address so writes propagate
  //ephemeral->Sync();
//

  // we shhould now get new data
  uint8_t b = ephemeral->GuardedRead_8(0);
  printf("Check a 0x%x, 0x%x\n", a, b);
  }
  api->FreeEphemeralRegion(ephemeral);
}



  

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