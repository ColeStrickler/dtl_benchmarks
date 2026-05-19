#include "perf.hpp"

PerfManager::PerfManager() 
{
#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    m_CacheMMIOFd = open_fd();
    if (m_CacheMMIOFd == -1)
    {
        printf("PerfManager::PerfManager() Could not open /dev/mem\n");
        exit(-1);
        return;
    }

    auto control_region_size = 0x1000;
    //printf("AGUControlRegionSize: 0x%x\n", control_region_size);
    m_ControlRegionBase = (uint64_t)mmap(NULL, control_region_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_CacheMMIOFd, LLC_PERF_BASE);
#endif


#ifdef INCLUDE_DTU_PERF_COUNTERS
    m_DTUMMIOFd = open_fd();
    if (m_DTUMMIOFd == -1)
    {
        printf("PerfManager::PerfManager() Could not open /dev/mem\n");
        exit(-1);
        return;
    }
    auto dtu_region_size = 0x1000;
    m_DTUControlRegionBase = (uint64_t)mmap(NULL, dtu_region_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_DTUMMIOFd, DTU_PERF_BASE);
#endif

}

PerfManager::~PerfManager()
{
#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    close(m_CacheMMIOFd);
    munmap((void*)m_ControlRegionBase, 0x1000);
#endif
#ifdef INCLUDE_DTU_PERF_COUNTERS
    close(m_DTUMMIOFd);
    munmap((void*)m_DTUControlRegionBase, 0x1000);
    
#endif

}

//#define INCLUDE_INCLUSIVE_LLC_COUNTERS

void PerfManager::ClearCounters() 
{
    m_Counters.m_Cycle = 0;
    m_Counters.m_InstRet = 0;
#ifdef INCLUDE_BOOM_PERF_EXTRA
    m_Counters.m_BrMispredict = 0;
    m_Counters.m_RobEmpty = 0;
    m_Counters.m_RobFull = 0;
    m_Counters.m_L1dMiss = 0;
    m_Counters.m_L1dRelease = 0;
    m_Counters.m_L1iMiss = 0;
    m_Counters.m_dTLBMiss = 0;
    m_Counters.m_iTLBMiss = 0;
    m_Counters.m_L2TLBMiss = 0;
    m_Counters.m_FPIssueStall = 0;
    m_Counters.m_IntIssueStall = 0;
#endif

#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    m_Counters.m_L1Refill = 0;
    m_Counters.m_L1Writeback = 0;
    m_Counters.m_LLCRefill = 0;
    m_Counters.m_LLCWriteback = 0;
#endif 

#ifdef INCLUDE_DTU_PERF_COUNTERS
    m_Counters.RegularDRAMAccess = 0;
    m_Counters.DTUDramAccess = 0;
#endif

}

std::string PerfManager::PrintCountersLabel() {
  std::string ret;
  ret += "cycle,";
  ret += "instret,";
#ifdef INCLUDE_BOOM_PERF_EXTRA
    ret += "br_mispredict,";
    ret += "rob_empty,";
    ret += "rob_full,";
    ret += "l1d_miss,";
    ret += "l1d_release,";
    ret += "l1i_miss,";
    ret += "dTLB_miss,";
    ret += "iTLB_miss,";
    ret += "l2TLB_miss,";
    ret += "fpIssueStall,";
    ret += "intIssueStall,";
#endif


#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    ret += "LLCRefill,";
    ret += "LLCWB,";
    ret += "L1Refill,";
    ret += "L1WB,";
#endif 

#ifdef INCLUDE_DTU_PERF_COUNTERS
    ret += "RegularDRAMAccess,";
    ret += "DTUDramAccess,";
#endif


    return ret;
}

std::string PerfManager::PrintCounters() 
{
    std::string ret;
    ret += std::to_string(m_Counters.m_Cycle) + ",";
    ret += std::to_string(m_Counters.m_InstRet) + ",";
#ifdef INCLUDE_BOOM_PERF_EXTRA
    ret += std::to_string(m_Counters.m_BrMispredict) + ",";
    ret += std::to_string(m_Counters.m_RobEmpty) + ",";
    ret += std::to_string(m_Counters.m_RobFull) + ",";
    ret += std::to_string(m_Counters.m_L1dMiss) + ",";
    ret += std::to_string(m_Counters.m_L1dRelease) + ",";
    ret += std::to_string(m_Counters.m_L1iMiss) + ",";
    ret += std::to_string(m_Counters.m_dTLBMiss) +  ",";
    ret += std::to_string(m_Counters.m_iTLBMiss) +  ",";
    ret += std::to_string(m_Counters.m_L2TLBMiss) + ",";
    ret += std::to_string(m_Counters.m_FPIssueStall) + ",";
    ret += std::to_string(m_Counters.m_IntIssueStall) + ",";
#endif


#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    ret += std::to_string(m_Counters.m_LLCRefill) + ",";
    ret += std::to_string(m_Counters.m_LLCWriteback) + ",";
    ret += std::to_string(m_Counters.m_L1Refill) + ",";
    ret += std::to_string(m_Counters.m_L1Writeback) + ",";
#endif 

#ifdef INCLUDE_DTU_PERF_COUNTERS
    ret += std::to_string(m_Counters.RegularDRAMAccess) + ",";
    ret += std::to_string(m_Counters.DTUDramAccess) +  ",";
#endif


    return ret;
}


void PerfManager::CollectCounters() {
  m_Counters.m_Cycle = read_cycle();
  m_Counters.m_InstRet = read_instret();

#ifdef INCLUDE_BOOM_PERF_EXTRA
    m_Counters.m_BrMispredict = read_csr(CSR_BR_MISPREDICT);
    m_Counters.m_RobEmpty = read_csr(CSR_ROB_EMPTY);
    m_Counters.m_RobFull = read_csr(CSR_ROB_FULL);
    m_Counters.m_L1dMiss = read_csr(CSR_L1D_MISS);
    m_Counters.m_L1dRelease = read_csr(CSR_L1D_RELEASE);
    m_Counters.m_L1iMiss = read_csr(CSR_L1I_MISS);
    m_Counters.m_dTLBMiss = read_csr(CSR_DTLB_MISS);
    m_Counters.m_iTLBMiss = read_csr(CSR_ITLB_MISS);
    m_Counters.m_L2TLBMiss = read_csr(CSR_L2TLB_MISS);


    m_Counters.m_FPIssueStall = read_csr(CSR_FPISSUE_STALL);
    m_Counters.m_IntIssueStall = read_csr(CSR_INTISSUE_STALL);
#endif


    // do MMIO reads last
#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    m_Counters.m_LLCRefill= READ_LLC_REFILL(m_ControlRegionBase);
    m_Counters.m_LLCWriteback =  READ_LLC_WB(m_ControlRegionBase);
    m_Counters.m_L1Refill= READ_L1_REFILL(m_ControlRegionBase);
    m_Counters.m_L1Writeback =  READ_L1_WB(m_ControlRegionBase);
#endif


#ifdef INCLUDE_DTU_PERF_COUNTERS
    m_Counters.DTUDramAccess = READ_DTU_ACCESS(m_DTUControlRegionBase);
    m_Counters.RegularDRAMAccess = READ_NONDTU_ACCESS(m_DTUControlRegionBase);
#endif
   // printf("got counters\n");
}

void PerfManager::CollectDelta()
{
    m_Counters.m_Cycle = read_cycle() - m_Counters.m_Cycle;
    m_Counters.m_InstRet = read_instret() - m_Counters.m_InstRet;



#ifdef INCLUDE_BOOM_PERF_EXTRA
    m_Counters.m_RobEmpty = read_csr(CSR_ROB_EMPTY) -  m_Counters.m_RobEmpty;
    m_Counters.m_RobFull = read_csr(CSR_ROB_FULL) - m_Counters.m_RobFull;
    m_Counters.m_L1dMiss = read_csr(CSR_L1D_MISS) - m_Counters.m_L1dMiss;
    m_Counters.m_dTLBMiss = read_csr(CSR_DTLB_MISS) - m_Counters.m_dTLBMiss;
    m_Counters.m_BrMispredict = read_csr(CSR_BR_MISPREDICT) - m_Counters.m_BrMispredict;
    m_Counters.m_iTLBMiss = read_csr(CSR_ITLB_MISS) - m_Counters.m_iTLBMiss;
    m_Counters.m_L2TLBMiss = read_csr(CSR_L2TLB_MISS) - m_Counters.m_L2TLBMiss;
    m_Counters.m_L1iMiss = read_csr(CSR_L1I_MISS) -  m_Counters.m_L1iMiss;
    m_Counters.m_L1dRelease = read_csr(CSR_L1D_RELEASE) - m_Counters.m_L1dRelease;
    m_Counters.m_FPIssueStall = read_csr(CSR_FPISSUE_STALL) - m_Counters.m_FPIssueStall;
    m_Counters.m_IntIssueStall = read_csr(CSR_INTISSUE_STALL) - m_Counters.m_IntIssueStall;
#endif

        // do MMIO reads last
#ifdef INCLUDE_INCLUSIVE_LLC_COUNTERS
    m_Counters.m_LLCRefill =        READ_LLC_REFILL(m_ControlRegionBase) - m_Counters.m_LLCRefill;
    m_Counters.m_LLCWriteback =     READ_LLC_WB(m_ControlRegionBase) - m_Counters.m_LLCWriteback;
    m_Counters.m_L1Refill =          READ_L1_REFILL(m_ControlRegionBase) - m_Counters.m_L1Refill;
    m_Counters.m_L1Writeback =      READ_L1_WB(m_ControlRegionBase) - m_Counters.m_L1Writeback;
#endif

#ifdef INCLUDE_DTU_PERF_COUNTERS
    m_Counters.DTUDramAccess = READ_DTU_ACCESS(m_DTUControlRegionBase) - m_Counters.DTUDramAccess;
    m_Counters.RegularDRAMAccess = READ_NONDTU_ACCESS(m_DTUControlRegionBase) - m_Counters.RegularDRAMAccess;
#endif

}

RocketChipCounters PerfManager::GetCounters() 
{
    return m_Counters; // return a copy
}
