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
#include "debug_trace.hpp"
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
    const auto trace = [](const char* stage) {
        DTL::DebugTrace("record-sort payload=%zu stage=%s", PAYLOAD_SIZE, stage);
    };
    trace("begin");


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




    trace("allocate-source begin");
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(record_size*sizeof(int)*N);
    trace("allocate-source done; compile-source begin");
    if (!api->Compile(kernel)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return -1;
    }
    //printf("Compiled dtl\n");
    trace("compile-source done; program-source begin");
    api->ProgramHardware(ephemeral, {{"x", 0x178000000ULL}});
    DTL::DebugTrace("record-sort source config=%d shadow_pa=0x%llx",
        ephemeral->GetConfig(),
        static_cast<unsigned long long>(ephemeral->GetCurrentShadowPhysicalAddr()));
    trace("program-source done");

    int* records =  (int*)ephemeral->GetHeadlessWriteregion();
    int* field_of_interest =  (int*)ephemeral->GetHeadlessReadRegion();
    
    trace("clone begin");
    DTL::EphemeralRegion* ephemeral2 = api->CloneEphemeralRegion(ephemeral);
    trace("clone done; compile-gather begin");
    if (!api->Compile(kernel2)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return -1;
    }
    //printf("Compiled dtl\n");
    trace("compile-gather done; program-gather begin");
    api->ProgramHardware(ephemeral2, {{"x", 0x178000000ULL}});
    DTL::DebugTrace("record-sort gather config=%d shadow_pa=0x%llx backing_va=%p source_va=%p gather_va=%p",
        ephemeral2->GetConfig(),
        static_cast<unsigned long long>(ephemeral2->GetCurrentShadowPhysicalAddr()),
        static_cast<void*>(records), static_cast<void*>(field_of_interest),
        ephemeral2->GetHeadlessReadRegion());
    trace("program-gather done; initialize-records begin");

    std::mt19937_64 rng(SEED);
    std::uniform_int_distribution<int> dist(0, 1000);
    //printf("writing records\n");
    /*
        intialize the records
    */
    for (int i = 0; i < N*record_size; ++i) {
        records[i] = dist(rng);
        if ((static_cast<std::size_t>(i) + 1) % (N*record_size/4) == 0)
            DTL::DebugTrace("record-sort initialize-records words=%zu/%zu",
                static_cast<std::size_t>(i) + 1, N*record_size);
    }
    //printf("done writing records\n");
            
    trace("initialize-records done; map-metadata begin");
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
    DTL::DebugTrace("record-sort map-metadata done pa=0x178000000 va=%p bytes=0x%llx",
        static_cast<void*>(indices), static_cast<unsigned long long>(metadata_size));


    trace("performance-manager begin");
    PerfManager perf;
    trace("performance-manager done; collect-counters begin");
    // These diagnostics are included in the measured interval on purpose.
    uint64_t comparisons = 0;
    perf.CollectCounters();
    trace("collect-counters done; initialize-indices begin");
    for (int i = 0; i < N; ++i) {
        indices[i] = i;
        if ((i + 1) % (N/4) == 0)
            DTL::DebugTrace("record-sort initialize-indices elements=%d/%zu", i + 1, N);
    }
    trace("initialize-indices done; sort begin");
    std::sort(indices, indices + N,
        [&](int a, int b) {
        ++comparisons;
        const bool report = comparisons <= 4 || comparisons % (1ULL << 20) == 0;
        if (report)
            DTL::DebugTrace("record-sort compare=%llu before-read-a index=%d index-b=%d",
                static_cast<unsigned long long>(comparisons), a, b);
        const int key_a = field_of_interest[a];
        if (report)
            DTL::DebugTrace("record-sort compare=%llu read-a-done key=%d; before-read-b index=%d",
                static_cast<unsigned long long>(comparisons), key_a, b);
        const int key_b = field_of_interest[b];
        if (report)
            DTL::DebugTrace("record-sort compare=%llu read-b-done key=%d",
                static_cast<unsigned long long>(comparisons), key_b);
        return key_a < key_b;
    });
    DTL::DebugTrace("record-sort sort done comparisons=%llu; validate-gather begin",
        static_cast<unsigned long long>(comparisons));
    int* sorted_field_of_interest =  (int*)ephemeral2->GetHeadlessReadRegion();
    uint64_t validation_comparisons = 0;
    bool is_sorted = std::is_sorted(sorted_field_of_interest, sorted_field_of_interest + N,
        [&](int a, int b) {
            ++validation_comparisons;
            if (validation_comparisons == 1 || validation_comparisons % (1ULL << 16) == 0)
                DTL::DebugTrace("record-sort validate-gather comparisons=%llu a=%d b=%d",
                    static_cast<unsigned long long>(validation_comparisons), a, b);
            return a < b;
        });
    DTL::DebugTrace("record-sort validate-gather done sorted=%d comparisons=%llu; collect-delta begin",
        static_cast<int>(is_sorted), static_cast<unsigned long long>(validation_comparisons));
    perf.CollectDelta();
    trace("collect-delta done");
    std::cout << "bench," << perf.PrintCountersLabel() << "IsSorted,record_size" << "\n";
    std::cout << "dtu," << perf.PrintCounters() << std::to_string(is_sorted) << "," << std::to_string(record_size) << "\n";
    perf.ClearCounters();



    trace("cpu-permutation begin");
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



    trace("cpu-permutation done; cpu-fullrecord begin");
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



    trace("cpu-fullrecord done; cleanup begin");
    munmap(indices, metadata_size);
    close(fd);
    delete[] indices2;
    delete[] records_raw;
    delete[] records_raw3;
    trace("free-source begin");
    api->FreeEphemeralRegion(ephemeral);
    trace("free-source done; free-gather begin");
    api->FreeEphemeralRegion(ephemeral2);
    trace("done");
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