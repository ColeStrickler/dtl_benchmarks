#include "perf.hpp"

PerfManager::PerfManager() {}

PerfManager::~PerfManager() {}

#define INCLUDE_INCLUSIVE_LLC_COUNTERS

void PerfManager::CollectCounters() 
{
    m_Counters.m_Cycle = read_cycle();
    m_Counters.m_InstRet = read_instret();

#ifdef INCLUDE_BOOM_PERF_EXTRA
    m_Counters.m_RobEmpty = read_csr(CSR_ROB_EMPTY);
    m_Counters.m_RobFull = read_csr(CSR_ROB_FULL);
    m_Counters.m_L1dMiss = read_csr(CSR_L1D_MISS);
    m_Counters.m_dTLBMiss = read_csr(CSR_DTLB_MISS);
    m_Counters.m_BrMispredict = read_csr(CSR_BR_MISPREDICT);
    m_Counters.m_iTLBMiss = read_csr(CSR_ITLB_MISS);
    m_Counters.m_L2TLBMiss = read_csr(CSR_L2TLB_MISS);
    m_Counters.m_L1iMiss = read_csr(CSR_L1I_MISS);
    m_Counters.m_L1dRelease = read_csr(CSR_L1D_RELEASE);
#endif


    // do MMIO reads last
#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    m_Counters.m_LLCAccessCount = READ_UINT64(LLC_ACCESS_COUNT);
    m_Counters.m_LLCMissCounter = READ_UINT64(LLC_MISS_COUNT);
#endif

}

void PerfManager::CollectDelta()
{
    m_Counters.m_RobEmpty = read_csr(CSR_ROB_EMPTY) -  m_Counters.m_RobEmpty;
    m_Counters.m_RobFull = read_csr(CSR_ROB_FULL) - m_Counters.m_RobFull;
    m_Counters.m_L1dMiss = read_csr(CSR_L1D_MISS) - m_Counters.m_L1dMiss;
    m_Counters.m_dTLBMiss = read_csr(CSR_DTLB_MISS) - m_Counters.m_dTLBMiss;
    m_Counters.m_BrMispredict = read_csr(CSR_BR_MISPREDICT) - m_Counters.m_BrMispredict;
    m_Counters.m_iTLBMiss = read_csr(CSR_ITLB_MISS) - m_Counters.m_iTLBMiss;
    m_Counters.m_L2TLBMiss = read_csr(CSR_L2TLB_MISS) - m_Counters.m_L2TLBMiss;
    m_Counters.m_L1iMiss = read_csr(CSR_L1I_MISS) -  m_Counters.m_L1iMiss;
    m_Counters.m_L1dRelease = read_csr(CSR_L1D_RELEASE) - m_Counters.m_L1dRelease;
}

RocketChipCounters PerfManager::GetCounters() 
{
    return m_Counters; // return a copy
}
