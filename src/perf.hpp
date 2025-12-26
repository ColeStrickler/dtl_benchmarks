#ifndef DTL_PERF_H
#define DTL_PERF_H
#include <stdint.h>

#ifndef ACCESS_HELPERS
#define ACCESS_HELPERS
#define WRITE_BOOL(addr, value)(*(bool*)(addr) = value)
#define WRITE_UINT8(addr, value)(*(uint8_t*)(addr) = value)
#define WRITE_UINT16(addr, value)(*(uint16_t*)(addr) = value)
#define WRITE_UINT32(addr, value)(*(uint32_t*)(addr) = value)
#define WRITE_UINT64(addr, value)(*(uint64_t*)(addr) = value)

#define READ_BOOL(addr)(*(bool*)(addr))
#define READ_UINT8(addr)(*(uint8_t*)(addr))
#define READ_UINT16(addr)(*(uint16_t*)(addr))
#define READ_UINT32(addr)(*(uint32_t*)(addr))
#define READ_UINT64(addr)(*(uint64_t*)(addr))
#endif




#define INCLUDE_INCLUSIVE_LLC_COUNTERS
#define INCLUDE_BOOM_PERF_EXTRA
/*
    Custom intra-core counters for the boom core
*/
#ifdef INCLUDE_BOOM_PERF_EXTRA
#define CSR_ROB_FULL 0x520
#define CSR_ROB_EMPTY 0x521
#define CSR_BR_MISPREDICT 0x522
#define CSR_L1I_MISS 0x523
#define CSR_L1D_MISS 0x524
#define CSR_L1D_RELEASE 0x525
#define CSR_ITLB_MISS 0x526
#define CSR_DTLB_MISS 0x527
#define CSR_L2TLB_MISS 0x528
#endif

#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
#define LLC_ACCESS_COUNT 0x3010000 // SET TO ACTUAL
#define LLC_MISS_COUNT 0x3010008
#endif


struct RocketChipCounters
{
#ifdef  INCLUDE_BOOM_PERF_EXTRA
    uint64_t m_RobFull;
    uint64_t m_RobEmpty;
    uint64_t m_BrMispredict;
    uint64_t m_L1iMiss;
    uint64_t m_L1dMiss;
    uint64_t m_L1dRelease;
    uint64_t m_iTLBMiss;
    uint64_t m_dTLBMiss;
    uint64_t m_L2TLBMiss;
#endif

#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    uint64_t m_LLCAccessCount;
    uint64_t m_LLCMissCounter;
#endif

    uint64_t m_Cycle;
    uint64_t m_InstRet;
};


static inline uint64_t read_csr(uint32_t csr)
{
    uint64_t value;
    asm volatile ("csrr %0, %1"
                  : "=r"(value)
                  : "i"(csr));
    return value;
}

uint64_t read_instret() {
    uint64_t instret;
    asm volatile ("csrr %0, instret" : "=r"(instret));
    return instret;
}

uint64_t read_cycle() {
    uint64_t cycle_count;
    asm volatile ("csrr %0, cycle" : "=r"(cycle_count));
    return cycle_count;
}



class PerfManager
{
public:
    PerfManager();
    ~PerfManager();

    void CollectCounters();
    void CollectDelta();
    RocketChipCounters GetCounters();

    /*
        We want:
        - instructions retired
        - l1 misses (i-cache and dcache)
        - l2 misses
        - l2 accesses
        - l1 accesses
        - ROB counters
        - branch misses
        - mis-speculated memory instructions
        - TLB misses (from inst fetch, data)
    */

private:
    RocketChipCounters m_Counters;



};

#endif