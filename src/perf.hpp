#ifndef DTL_PERF_H
#define DTL_PERF_H
#include <stdint.h>
#include <string>

#include <fcntl.h> 
#include <sys/mman.h>
#include <unistd.h>
#include <sys/ioctl.h>


#include "util.hpp"

#ifndef ACCESS_HELPERS
#define ACCESS_HELPERS
#define WRITE_BOOL(addr, value)(*(volatile bool*)(addr) = value)
#define WRITE_UINT8(addr, value)(*(volatile uint8_t*)(addr) = value)
#define WRITE_UINT16(addr, value)(*(volatile uint16_t*)(addr) = value)
#define WRITE_UINT32(addr, value)(*(volatile uint32_t*)(addr) = value)
#define WRITE_UINT64(addr, value)(*(volatile uint64_t*)(addr) = value)

#define READ_BOOL(addr)(*(volatile const bool*)(addr))
#define READ_UINT8(addr)(*(volatile const uint8_t*)(addr))
#define READ_UINT16(addr)(*(volatile const uint16_t*)(addr))
#define READ_UINT32(addr)(*(volatile const uint32_t*)(addr))
#define READ_UINT64(addr)(*(volatile const uint64_t*)(addr))
#endif




//#define INCLUDE_INCLUSIVE_LLC_COUNTERS
//#define INCLUDE_BOOM_PERF_EXTRA
//#define INCLUDE_INCLUSIVE_LLC_COUNTERS
#define INCLUDE_DTU_PERF_COUNTERS
//#define INCLUDE_BOOM_PERF_EXTRA
/*
    Custom intra-core counters for the boom core
*/
#ifdef INCLUDE_BOOM_PERF_EXTRA
#define CSR_ROB_FULL        0x020
#define CSR_ROB_EMPTY       0x021
#define CSR_BR_MISPREDICT   0x022
#define CSR_L1I_MISS        0x023
#define CSR_L1D_MISS        0x024
#define CSR_L1D_RELEASE     0x025
#define CSR_ITLB_MISS       0x026
#define CSR_DTLB_MISS       0x027
#define CSR_L2TLB_MISS      0x028
#define CSR_FPISSUE_STALL   0x029
#define CSR_INTISSUE_STALL  0x02a
#endif

#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
#define LLC_PERF_BASE 0x2110000
#define LLC_REFILL 0x610
#define LLC_WB 0x618
#define L1_REFILL 0x600
#define L1_WB 0x608


#define READ_LLC_REFILL(base)  (READ_UINT64(base + LLC_REFILL))
#define READ_LLC_WB(base)  (READ_UINT64(base + LLC_WB))
#define READ_L1_REFILL(base) (READ_UINT64(base + L1_REFILL))
#define READ_L1_WB(base) (READ_UINT64(base + L1_WB))
 
#define READ_LLC_ACCESS_COUNT(base) (READ_UINT64(base + LLC_ACCESS_COUNT))
#define READ_LLC_MISS_COUNT(base) (READ_UINT64(base + LLC_MISS_COUNT))
#endif

#ifdef INCLUDE_DTU_PERF_COUNTERS
#define DTU_PERF_BASE 0x3000000ULL

#define DTU_ACCESS 0xf00
#define NON_DTU_ACCESS 0xf08


#define READ_DTU_ACCESS(base) (READ_UINT64(base + DTU_ACCESS))
#define READ_NONDTU_ACCESS(base) (READ_UINT64(base + NON_DTU_ACCESS))

#endif



struct RocketChipCounters
{

    uint64_t m_Cycle;
    uint64_t m_InstRet;

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
    uint64_t m_FPIssueStall;
    uint64_t m_IntIssueStall;
#endif

#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    uint64_t m_L1Refill;
    uint64_t m_L1Writeback;
    uint64_t m_LLCRefill;
    uint64_t m_LLCWriteback;
#endif




#ifdef INCLUDE_DTU_PERF_COUNTERS
    uint64_t RegularDRAMAccess;
    uint64_t DTUDramAccess;
#endif
};


static inline uint64_t read_csr(uint32_t csr)
{
    uint64_t value;
    asm volatile ("csrr %0, %1"
                  : "=r"(value)
                  : "i"(csr));
    return value;
}

inline uint64_t read_instret() {
    uint64_t instret;
    asm volatile ("csrr %0, instret" : "=r"(instret));
    return instret;
}

inline uint64_t read_cycle() {
    uint64_t cycle_count;
    asm volatile ("csrr %0, cycle" : "=r"(cycle_count));
    return cycle_count;
}



class PerfManager
{
public:
    PerfManager();
    ~PerfManager();

    void ClearCounters();
    static std::string PrintCountersLabel();
    std::string PrintCounters();
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
    uint64_t m_ControlRegionBase;
    RocketChipCounters m_Counters;
    int m_CacheMMIOFd;
    int m_DTUMMIOFd;
    uint64_t m_DTUControlRegionBase;
    
};

#endif