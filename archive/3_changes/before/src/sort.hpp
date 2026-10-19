#ifndef SORT_HPP
#define SORT_HPP
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <vector>
#include "dtl_api.hpp"
#include "dtl_stencils.hpp"

template <std::size_t PAYLOAD_SIZE>
struct Record {
    int key;
    int payload[PAYLOAD_SIZE];
};


inline std::string replaceKeyword2(std::string str, std::string key, std::string replacement)
{
    auto dtl_parameterized_config = str;

    std::string token = "[" + key + "]";
    int pos;
    while ((pos = dtl_parameterized_config.find(token)) != std::string::npos)
        dtl_parameterized_config.replace(pos, token.size(), replacement);
    
    return dtl_parameterized_config;
}




template <std::size_t PAYLOAD_SIZE>
int run_RecordSort(DTL::API *api)
{
    using Rec = Record<PAYLOAD_SIZE>;

    static_assert(sizeof(Rec) ==
              (PAYLOAD_SIZE + 1) * sizeof(int),
              "Record contains unexpected padding");

    constexpr uint64_t SEED = 12345;
    std::string kernel = dtl_stencil_configs["record_extract"];
    std::string kernel2 = dtl_stencil_configs["recordsort_extract"];
    uint32_t record_size = PAYLOAD_SIZE+1;
    constexpr std::size_t N = 500000;


    kernel = replaceKeyword2(kernel, "RECORDS", std::to_string(N));
    kernel = replaceKeyword2(kernel, "NFIELDS_EXTRACT", std::to_string(1));
    kernel = replaceKeyword2(kernel, "RECORD_SIZE", std::to_string(record_size));
    kernel = "int offsets = {0};\n" + kernel;
    kernel = "DTL<0x0> kernel = {\n" + kernel + "}\nactive kernel;";
    std::cout << kernel << "\n\n";


    kernel2 = replaceKeyword2(kernel2, "RECORDS", std::to_string(N));
    kernel2 = replaceKeyword2(kernel2, "NFIELDS_EXTRACT", std::to_string(1));
    kernel2 = replaceKeyword2(kernel2, "RECORD_SIZE", std::to_string(record_size));
    kernel2 = "int offsets = {0};\n" + kernel2;
    kernel2 = "DTL<0x0> kernel = {\n" + kernel2 + "}\nactive kernel;";
    std::cout << kernel2 << "\n\n";




    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(record_size*sizeof(int)*N);
    if (!api->Compile(kernel)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return -1;
    }
    //printf("Compiled dtl\n");
    api->ProgramHardware(ephemeral, {{"x", 0x178000000ULL}});

    int* records =  (int*)ephemeral->GetHeadlessWriteregion();
    int* field_of_interest =  (int*)ephemeral->GetHeadlessReadRegion();
    
    DTL::EphemeralRegion* ephemeral2 = api->CloneEphemeralRegion(ephemeral);
    if (!api->Compile(kernel2)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return -1;
    }
    //printf("Compiled dtl\n");
    api->ProgramHardware(ephemeral2, {{"x", 0x178000000ULL}});

    std::mt19937_64 rng(SEED);
    std::uniform_int_distribution<int> dist(0, 1000);
    //printf("writing records\n");
    /*
        intialize the records
    */
    for (int i = 0; i < N*record_size; ++i) {
        records[i] = dist(rng);
    }
    //printf("done writing records\n");
            
    uint64_t metadata_size = 0x8000000ULL;
    int fd = open("/dev/mem", O_RDWR);
    assert(fd >= 0);
    int* indices =
        (int*)mmap(
                nullptr,
                metadata_size,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd,
                0x178000000ULL);
    assert(indices != nullptr && indices != MAP_FAILED);


    PerfManager perf;
    perf.CollectCounters();
    for (int i = 0; i < N; ++i) {
        indices[i] = i;
    }
    std::sort(indices, indices + N,
        [&](int a, int b) {
        return field_of_interest[a] < field_of_interest[b];
    });
    int* sorted_field_of_interest =  (int*)ephemeral2->GetHeadlessReadRegion();
    bool is_sorted = std::is_sorted(sorted_field_of_interest, sorted_field_of_interest + N);
    perf.CollectDelta();
    std::cout << "bench," << perf.PrintCountersLabel() << "IsSorted,record_size" << "\n";
    std::cout << "dtu," << perf.PrintCounters() << std::to_string(is_sorted) << "," << std::to_string(record_size) << "\n";
    perf.ClearCounters();



    int* records_raw = new int[record_size*N];
    std::mt19937_64 rng2(SEED);
    std::uniform_int_distribution<int> dist2(0, 1000);
    Rec* rec = (Rec*)records_raw;
    /*
        intialize the records deterministically
    */
    for (int i = 0; i < N*record_size; ++i) {
        records_raw[i] = dist2(rng2); // init 
    }
    int* indices2 = new int[N];
    
    
    perf.CollectCounters();
    for (int i = 0; i < N; ++i) {
        indices2[i] = i;
    }
    std::sort(indices2, indices2 + N,
        [&](int a, int b) {
        return rec[a].key < rec[b].key;
    });

    is_sorted = std::is_sorted(
        indices2,
        indices2 + N,
        [&](int a, int b) {
            return rec[a].key < rec[b].key;
        }
    );
    perf.CollectDelta();
    std::cout << "bench," << perf.PrintCountersLabel() << "IsSorted,record_size" << "\n";
    std::cout << "cpu_permutation," << perf.PrintCounters() << std::to_string(is_sorted) << "," << std::to_string(record_size) << "\n";
    perf.ClearCounters();



    int* records_raw3 = new int[record_size*N];
    std::mt19937_64 rng3(SEED);
    std::uniform_int_distribution<int> dist3(0, 1000);
    for (int i = 0; i < N*record_size; ++i) {
        records_raw3[i] = dist3(rng3); // init 
    }
    Rec* rec3 = (Rec*)records_raw3;
    perf.CollectCounters();

    std::sort(rec3, rec3 + N,
        [&](const Rec& a, const Rec& b) {
        return a.key < b.key;
    });

    is_sorted = std::is_sorted(
        rec3,
        rec3 + N,
        [](const Rec& a, const Rec& b) {
        return a.key < b.key;
    }
    );
    perf.CollectDelta();
    std::cout << "bench," << perf.PrintCountersLabel() << "IsSorted,record_size" << "\n";
    std::cout << "cpu_fullrecord," << perf.PrintCounters() << std::to_string(is_sorted) << "," << std::to_string(record_size) << "\n";



    munmap(indices, metadata_size);
    close(fd);
    delete[] indices2;
    delete[] records_raw;
    delete[] records_raw3;
    api->FreeEphemeralRegion(ephemeral);
    api->FreeEphemeralRegion(ephemeral2);
    return 0;
}






template <std::size_t PAYLOAD_SIZE>
int run_RecordMergeSort(DTL::API *api)
{
    using Rec = Record<PAYLOAD_SIZE>;

    static_assert(sizeof(Rec) ==
              (PAYLOAD_SIZE + 1) * sizeof(int),
              "Record contains unexpected padding");

    constexpr uint64_t SEED = 12345;
    std::string kernel = dtl_stencil_configs["record_extract"];
    std::string kernel2 = dtl_stencil_configs["recordsort_extract"];
    uint32_t record_size = PAYLOAD_SIZE+1;
    constexpr std::size_t N = 500000;


    kernel = replaceKeyword2(kernel, "RECORDS", std::to_string(N));
    kernel = replaceKeyword2(kernel, "NFIELDS_EXTRACT", std::to_string(1));
    kernel = replaceKeyword2(kernel, "RECORD_SIZE", std::to_string(record_size));
    kernel = "int offsets = {0};\n" + kernel;
    std::cout << kernel << "\n\n";


    kernel2 = replaceKeyword2(kernel2, "RECORDS", std::to_string(N));
    kernel2 = replaceKeyword2(kernel2, "NFIELDS_EXTRACT", std::to_string(1));
    kernel2 = replaceKeyword2(kernel2, "RECORD_SIZE", std::to_string(record_size));
    kernel2 = "int offsets = {0};\n" + kernel2;
    std::cout << kernel2 << "\n\n";




    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(record_size*sizeof(int)*N);
    if (!api->Compile(kernel)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return -1;
    }
    //printf("Compiled dtl\n");
    api->ProgramHardware(ephemeral);

    int* records =  (int*)ephemeral->GetHeadlessWriteregion();
    int* field_of_interest =  (int*)ephemeral->GetHeadlessReadRegion();
    
    DTL::EphemeralRegion* ephemeral2 = api->CloneEphemeralRegion(ephemeral);
    if (!api->Compile(kernel2)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return -1;
    }
    //printf("Compiled dtl\n");
    api->ProgramHardware(ephemeral2);

    std::mt19937_64 rng(SEED);
    std::uniform_int_distribution<int> dist(0, 1000);
    //printf("writing records\n");
    /*
        intialize the records
    */
    for (int i = 0; i < N*record_size; ++i) {
        records[i] = dist(rng);
    }
    //printf("done writing records\n");
            
    uint64_t metadata_size = 0x8000000ULL;
    int fd = open("/dev/mem", O_RDWR);
    assert(fd >= 0);
    int* indices =
        (int*)mmap(
                nullptr,
                metadata_size,
                PROT_READ | PROT_WRITE,
                MAP_SHARED,
                fd,
                0x178000000ULL);
    assert(indices != nullptr && indices != MAP_FAILED);


    PerfManager perf;
    perf.CollectCounters();
    for (int i = 0; i < N; ++i) {
        indices[i] = i;
    }
    std::stable_sort(indices, indices + N,
        [&](int a, int b) {
        return field_of_interest[a] < field_of_interest[b];
    });
    int* sorted_field_of_interest =  (int*)ephemeral2->GetHeadlessReadRegion();
    bool is_sorted = std::is_sorted(sorted_field_of_interest, sorted_field_of_interest + N);
    perf.CollectDelta();
    std::cout << "bench," << perf.PrintCountersLabel() << "IsSorted,record_size" << "\n";
    std::cout << "dtu_merge," << perf.PrintCounters() << std::to_string(is_sorted) << "," << std::to_string(record_size) << "\n";
    perf.ClearCounters();



    int* records_raw = new int[record_size*N];
    std::mt19937_64 rng2(SEED);
    std::uniform_int_distribution<int> dist2(0, 1000);
    Rec* rec = (Rec*)records_raw;
    /*
        intialize the records deterministically
    */
    for (int i = 0; i < N*record_size; ++i) {
        records_raw[i] = dist2(rng2); // init 
    }
    int* indices2 = new int[N];
    
    
    perf.CollectCounters();
    for (int i = 0; i < N; ++i) {
        indices2[i] = i;
    }
    std::stable_sort(indices2, indices2 + N,
        [&](int a, int b) {
        return rec[a].key < rec[b].key;
    });

    is_sorted = std::is_sorted(
        indices2,
        indices2 + N,
        [&](int a, int b) {
            return rec[a].key < rec[b].key;
        }
    );
    perf.CollectDelta();
    std::cout << "bench," << perf.PrintCountersLabel() << "IsSorted,record_size" << "\n";
    std::cout << "cpu_permutation_merge," << perf.PrintCounters() << std::to_string(is_sorted) << "," << std::to_string(record_size) << "\n";
    perf.ClearCounters();



    int* records_raw3 = new int[record_size*N];
    std::mt19937_64 rng3(SEED);
    std::uniform_int_distribution<int> dist3(0, 1000);
    for (int i = 0; i < N*record_size; ++i) {
        records_raw3[i] = dist3(rng3); // init 
    }
    Rec* rec3 = (Rec*)records_raw3;
    perf.CollectCounters();

    std::stable_sort(rec3, rec3 + N,
        [&](const Rec& a, const Rec& b) {
        return a.key < b.key;
    });

    is_sorted = std::is_sorted(
        rec3,
        rec3 + N,
        [](const Rec& a, const Rec& b) {
        return a.key < b.key;
    }
    );
    perf.CollectDelta();
    std::cout << "bench," << perf.PrintCountersLabel() << "IsSorted,record_size" << "\n";
    std::cout << "cpu_fullrecord_merge," << perf.PrintCounters() << std::to_string(is_sorted) << "," << std::to_string(record_size) << "\n";



    munmap(indices, metadata_size);
    close(fd);
    delete[] indices2;
    delete[] records_raw;
    delete[] records_raw3;
    api->FreeEphemeralRegion(ephemeral);
    api->FreeEphemeralRegion(ephemeral2);
    return 0;
}






#endif