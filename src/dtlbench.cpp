// System Headers
#include <sys/mman.h>

// Standard Library
#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <pthread.h>
#include <sched.h>
#include <exception>

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
  const bool run_gradzatz = argc == 2 && std::string(argv[1]) == "--gradzatz-invert";
  const bool run_gradzatp = argc == 2 && std::string(argv[1]) == "--gradzatp-invert";
  const bool run_bfs = argc == 2 && std::string(argv[1]) == "--bfs";
  const bool run_bfs_file = argc == 3 && std::string(argv[1]) == "--bfs-file";
  const bool run_nas_cg = argc == 2 && std::string(argv[1]) == "--nas-cg";
  const bool run_single_indirect = argc == 2 && std::string(argv[1]) == "--single-indirect";
  const bool run_double_indirect = argc == 2 && std::string(argv[1]) == "--double-indirect";
  const bool run_triple_indirect = argc == 2 && std::string(argv[1]) == "--triple-indirect";
  const bool run_double_metadata = argc == 2 && std::string(argv[1]) == "--double-metadata";
  const bool run_small_embedding = argc == 2 && std::string(argv[1]) == "--small-embedding";
  const bool run_medium_embedding = argc == 2 && std::string(argv[1]) == "--medium-embedding";
  const bool run_large_embedding = argc == 2 && std::string(argv[1]) == "--large-embedding";
  const bool run_small_double_indirect_embedding = argc == 2 && std::string(argv[1]) == "--small-double-indirect-embedding";
  const bool run_medium_double_indirect_embedding = argc == 2 && std::string(argv[1]) == "--medium-double-indirect-embedding";
  const bool run_large_double_indirect_embedding = argc == 2 && std::string(argv[1]) == "--large-double-indirect-embedding";
  const bool run_microbenchmark = run_single_indirect || run_double_indirect || run_triple_indirect ||
      run_double_metadata || run_small_embedding || run_medium_embedding || run_large_embedding ||
      run_small_double_indirect_embedding || run_medium_double_indirect_embedding || run_large_double_indirect_embedding;
  if (argc > 1 && !run_gradzatz && !run_gradzatp && !run_bfs && !run_bfs_file && !run_nas_cg && !run_microbenchmark) {
    std::cerr << "Usage: " << argv[0]
              << " [--gradzatz-invert|--gradzatp-invert|--bfs|--bfs-file PATH|--nas-cg|"
              << "--single-indirect|--double-indirect|--triple-indirect|--double-metadata|"
              << "--small-embedding|--medium-embedding|--large-embedding|"
              << "--small-double-indirect-embedding|--medium-double-indirect-embedding|--large-double-indirect-embedding]\n";

    return 1;
  }
  auto bfs_data = benchmark::bfsData;
  if (run_bfs_file) {
    bfs_data.resize(1);
    bfs_data[0].input_artifact = argv[2];
  }

  const auto* selected_data = &benchmark::doubleIndirectData;
  if (run_single_indirect)
    selected_data = &benchmark::singleIndirectData;
  else if (run_double_indirect)
    selected_data = &benchmark::doubleIndirectData;
  else if (run_triple_indirect)
    selected_data = &benchmark::tripleIndirectData;
  else if (run_double_metadata)
    selected_data = &benchmark::doubleMetadataData;
  else if (run_small_embedding)
    selected_data = &benchmark::smallEmbeddingData;
  else if (run_medium_embedding)
    selected_data = &benchmark::mediumEmbeddingData;
  else if (run_large_embedding)
    selected_data = &benchmark::largeEmbeddingData;
  else if (run_small_double_indirect_embedding)
    selected_data = &benchmark::smallDoubleIndirectEmbeddingData;
  else if (run_medium_double_indirect_embedding)
    selected_data = &benchmark::mediumDoubleIndirectEmbeddingData;
  else if (run_large_double_indirect_embedding)
    selected_data = &benchmark::largeDoubleIndirectEmbeddingData;
  else if (run_nas_cg)
    selected_data = &benchmark::nasCgData;
  else if (run_bfs || run_bfs_file)
    selected_data = &bfs_data;
  else if (run_gradzatp)
    selected_data = &benchmark::gradzatpInvertData;
  else if (run_gradzatz)
    selected_data = &benchmark::gradzatzInvertData;


  //auto hwStat = new DTL::AGUHardwareStat(4, 4, 5, 6, 5, 4, 8, 1, 2);
	//auto hwStat = new DTL::AGUHardwareStat({ // (nAdd, nMult, nSUb, nPassThru) 
	//	{2,4,2,4},
	//	{2,2,1,4},
	//	{2,2,1,2},
	//	{1,1,1,2},
	//	{1,1,1,1},
	//	{0,0,0,1},
	//}, 5, 6, 5, 2, 1, 2);


  auto hwStat = new DTL::AGUHardwareStat({ // (nAdd, nMult, nSUb, nPassThru) 
		{2,4,2,4},
		{2,2,1,4},
		{2,2,1,2},
		{1,1,1,2},
		{1,1,1,1},
		{0,0,0,1},
	}, 5, 6, 7, 2, 1, 2);
  hwStat->nMaxConfigs = 4;
  

  auto api = new DTL::API(hwStat);
  if (api->GetError() != 0)
  {
    printf("DTL::API::HASERROR\n");
    return 0;
  }
  PerfManager perf;
  std::cout << "benchmark,type,transform_cost," << perf.PrintCountersLabel() << "hash" <<"\n";


    std::string res = "";



    std::cout << res << "\n";


        for (int i = 0; i < 1; i++)
    {
      for (auto& bench: *selected_data)
      {
        try {
          auto bench_res = bench.benchmark(bench, api);
          res += bench_res;
          std::cout << bench_res << "\n";
        } catch (const std::exception& error) {
          std::cerr << bench.name << ": " << error.what() << "\n";
          return 1;
        }
      }
      
    }


    if (run_bfs || run_bfs_file || run_nas_cg || run_microbenchmark)

      return 0;

          for (int i = 0; i < 1; i++)
    {
        for (auto& bench: benchmark::im2colData)
      {
          auto bench_res = bench.benchmark(bench, api);
          res += bench_res;
          std::cout << bench_res << "\n";
      }
      
    }

    return 0;
    std::cout << res << "\n";
    res = "";

    

      for (int i = 0; i < 1; i++)
    {
        for (auto& bench: benchmark::im2colData)
      {
          auto bench_res = bench.benchmark(bench, api);
          res += bench_res;
          std::cout << bench_res << "\n";
      }
      
    }
    std::cout << res << "\n";
    res = "";


    for (int i = 0; i < 1; i++)
  {
      for (auto& bench: benchmark::transposeData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        std::cout << bench_res << "\n";
    }
    
  }

  
  return 0;
  std::cout << res << "\n";




  for (int i = 0; i < 1; i++)
  {
      for (auto& bench: benchmark::vol2colData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        std::cout << bench_res << "\n";
    }
    
  }
  std::cout << res << "\n";
  res = "";



      for (int i = 0; i < 1; i++)
  {
      for (auto& bench: benchmark::ImgAugmentationData_nchw)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        //std::cout << bench_res << "\n";
    }
    
  }
  std::cout << res << "\n";
  res = "";
    return  0;


  //for (auto& bench : benchmark::BenchmarkDispatchDataSplitLayouts)
  //{
  //  std::cout << bench.benchmark(bench, api);
  //}








    for (int i = 0; i < 1; i++)
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



      for (int i = 0; i < 1; i++)
  {
      for (auto& bench: benchmark::colProjData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        //std::cout << bench_res << "\n";
    }
  }

    std::cout << res << "\n";
  res = "";
  return 0;


  for (int i = 0; i < 1; i++)
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
      for (auto& bench: benchmark::colProjData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        std::cout << bench_res << "\n";
    }
    
  }
  std::cout << res << "\n";
  res = "";



  return 0;


  int x = 1000;



      for (int i = 0; i < 10; i++)
  {
      for (auto& bench: benchmark::unfoldData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        std::cout << bench_res << "\n";
    }
  }

  std::cout << res << "\n";
 // res = "";
//
 //   for (int i = 0; i < 10; i++)
 // {
 //     for (auto& bench: benchmark::vol2colData)
 //   {
 //       auto bench_res = bench.benchmark(bench, api);
 //       res += bench_res;
 //       std::cout << bench_res << "\n";
 //   }
 //   
 // }
 // std::cout << res << "\n";
 // res = "";
//
  return 0;


      for (int i = 0; i < 10; i++)
  {
      for (auto& bench: benchmark::colProjData)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        std::cout << bench_res << "\n";
    }
    
  }
  std::cout << res << "\n";
  res = "";



      for (int i = 0; i < 10; i++)
  {
      for (auto& bench: benchmark::colProjData2)
    {
        auto bench_res = bench.benchmark(bench, api);
        res += bench_res;
        std::cout << bench_res << "\n";
    }
    
  }
  std::cout << res << "\n";
  res = "";


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