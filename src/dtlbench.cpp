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




int main(int argc, char* argv[]) {


  //auto hwStat = new DTL::AGUHardwareStat(4, 4, 5, 6, 5, 4, 8, 1, 2);
	auto hwStat = new DTL::AGUHardwareStat({ // (nAdd, nMult, nSUb, nPassThru) 
		{2,4,2,4},
		{2,2,1,4},
		{2,2,1,2},
		{1,1,1,2},
		{1,1,1,1},
		{0,0,0,1},
	}, 5, 6, 5, 2, 1, 2);
  hwStat->nMaxConfigs = 8;
  

  auto api = new DTL::API(hwStat);
  if (api->GetError() != 0)
  {
    printf("DTL::API::HASERROR\n");
    return 0;
  }
  PerfManager perf;
  std::cout << "benchmark,type,transform_cost," << perf.PrintCountersLabel() << "hash" <<"\n";


  //for (auto& bench : benchmark::BenchmarkDispatchDataSplitLayouts)
  //{
  //  std::cout << bench.benchmark(bench, api);
  //}


  int x = 1000;

  std::string res = "";



    for (int i = 0; i < 10; i++)
  {
      for (auto& bench: benchmark::ImgAugmentationData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        //std::cout << bench_res << "\n";
    }
    
  }
  std::cout << res << "\n";
  res = "";

  for (int i = 0; i < 10; i++)
  {
      for (auto& bench: benchmark::im2colData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        //std::cout << bench_res << "\n";
    }
    
  }
  std::cout << res << "\n";
  res = "";
    for (int i = 0; i < 10; i++)
  {
      for (auto& bench: benchmark::unfoldData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        //std::cout << bench_res << "\n";
    }
  }

  std::cout << res << "\n";
  res = "";

  for (int i = 0; i < 10; i++)
  {
      for (auto& bench: benchmark::slicingData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        //std::cout << bench_res << "\n";
    }
  }
  std::cout << res << "\n";
  res = "";

  for (int i = 0; i < 10; i++)
  {
      for (auto& bench: benchmark::slicingData2)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        //std::cout << bench_res << "\n";
    }
  }

  std::cout << res << "\n";
  res = "";
  
  return 0;

  
  for (auto& bench : benchmark::TMEComparisonData)
  {
   // printf("calling bench\n");


    auto bench_res = bench.benchmark(bench, api);
    res += bench_res;
    std::cout << bench_res << "\n";

    //api->DebugPrintAllocator();
  }
  std::cout << res << "\n";



  return 0;



  return 0;
}