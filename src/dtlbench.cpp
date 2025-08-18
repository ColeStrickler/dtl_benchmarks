// System Headers
#include <sys/mman.h>

// User Headers
#include "matmul.hpp"
#include "util.hpp"

#define DIMENSION 640

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

int main() {

  int hpm_fd = open_fd();

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
  auto hwStat = new DTL::AGUHardwareStat(4, 4, 5, 6, 6, 4, 3);
  DTL::API api(hwStat);

  int alloc_size = DIMENSION * DIMENSION * sizeof(float);
  float *A = (float *)malloc(alloc_size);
  float *B = (float *)plim;
  float *C = (float *)malloc(alloc_size);
  float *Bt;

  // BENCH(matmult_opt0_naive(A, B, C, DIMENSION));
  // BENCH(matmult_opt1_jk(A, B, C, DIMENSION));
  // toFile("opt1.out", C, DIMENSION);
  // BENCH(matmult_opt2_jk_tiling(A, B, C, DIMENSION));
  //  toFile("opt2.out", C, DIMENSION);
  BENCH(matmult_opt3_transposed(A, B, C, &Bt, DIMENSION));
 // toFile("opt3A.out", A, DIMENSION);
  toFile("opt3.out", C, DIMENSION);
  //toFile("opt3B.out", Bt, DIMENSION);


  //init_data(A, B, C, DIMENSION);
  auto start = std::chrono::high_resolution_clock::now();
  //matmult_opt3_pretransposed(A,Bt,C, DIMENSION);
  auto end = std::chrono::high_resolution_clock::now();
  double elapsed = std::chrono::duration<double>(end - start).count();
  double checksum = print_checksum(C, DIMENSION);
  //toFile("opt3Pre.out", C, DIMENSION);
  //printf("%.12s  secs: %.6f  chsum: %.6f\n",
  //       "matmult_opt3_pretransposed",
  //       elapsed,
  //       checksum);
  
  // chsum: 1440.415743
  // chsum: 1474.542120
  // chsum: 1452.471111
  // chsum: 1444.091102
  init_data(A, B, C, DIMENSION);


  float *bcopy = (float *)malloc(alloc_size);
  copy_matrix(B, bcopy, DIMENSION);
  printf("copied B\n");
  zero_matrix(B, DIMENSION);
  printf("zeroed\n");
  init_bank_aware_transpose((uint64_t)plim, bcopy, DIMENSION);

  api.SetBaseAddr((uint64_t)agu_config_base);
  //if (!api.Compile(FileToString("./transpose_640x640.dtl"))) {
  //  printf("Failed to compile dtl program or map onto agu\n");
  //  return 0;
  //}

  if (!api.Compile(FileToString("./bankaware_640x640.dtl"))) {
     printf("Failed to compile dtl program or map onto agu\n");
     return 0;
  }
  api.ProgramHardware();

  printf("Successfully compiled dtl\n");
  printf("Successfully programmed agu\n");


  


  if (configure_relcache() == -1) {
    printf("Unable to configure relcache\n");
    return -1;
  }

  // flush_cache();

  
  flush_cache();
  EnableRelCache(hpm_fd);


  
  
  
  
  toFile("optdtlB.out", B, DIMENSION);
  
  
  start = std::chrono::high_resolution_clock::now();
  matmult_dtl_transposed(A, B, C, DIMENSION);
  end = std::chrono::high_resolution_clock::now();
  elapsed = std::chrono::duration<double>(end - start).count();
  checksum = print_checksum(C, DIMENSION);
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

  return 0;
}