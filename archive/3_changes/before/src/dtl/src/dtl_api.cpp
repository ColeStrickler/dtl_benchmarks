#include "dtl_api.hpp"
#include "util.hpp"
#include <algorithm>
#include <cmath>


#define ERR(msg) (std::cerr << msg << std::endl)


static int open_fd(const std::string& path) {
  int fd = open(path.c_str(), O_RDWR | O_SYNC);
  if (fd == -1) {
    printf("Can't open /dev/mem.\n");
    exit(0);
  }
  return fd;
}
#define PAGE_SHIFT 12
#define PAGE_SIZE (1ULL << PAGE_SHIFT)
#define PFN_MASK ((1ULL << 55) - 1)


static uint64_t va_to_pa(void *va) {
    uint64_t value;
    int pagemap_fd;
    off_t offset = ((uintptr_t)va / PAGE_SIZE) * sizeof(uint64_t);

    pagemap_fd = open("/proc/self/pagemap", O_RDONLY);
    if (pagemap_fd < 0) {
        perror("open");
        return 0;
    }

    if (pread(pagemap_fd, &value, sizeof(value), offset) != sizeof(value)) {
        perror("pread");
        close(pagemap_fd);
        return 0;
    }

    close(pagemap_fd);

    if (!(value & (1ULL << 63))) {
        // page not present
        return 0;
    }

    uint64_t pfn = value & PFN_MASK;
    return (pfn << PAGE_SHIFT) | ((uintptr_t)va & (PAGE_SIZE - 1));
}




DTL::API::API(uint64_t realBackingStart, uint64_t realBackingSize)
    : m_ConfigBitmap(0), m_RealBackingStart(realBackingStart),
      m_RealBackingSize(realBackingSize)
{
    ReadHardwareInfo();


    assert(is_power_of_two(realBackingSize) && realBackingSize%Megabyte == 0);
    assert(realBackingSize > Megabyte);
    m_BuddyAllocator = new BuddyAllocator(realBackingSize);



}

DTL::API::API(AGUHardwareStat *hwStat, uint64_t realBackingStart, uint64_t realBackingSize) 
    :  hwStat(hwStat), \
        m_ConfigBitmap(0), m_RealBackingStart(realBackingStart), m_RealBackingSize(realBackingSize)
{

    SetError(0);

    assert(is_power_of_two(realBackingSize) && realBackingSize%Megabyte == 0);
    assert(realBackingSize > Megabyte);
    m_BuddyAllocator = new BuddyAllocator(realBackingSize);

    m_AGUControlRegionfd = open_fd("/dev/mem");
    if (m_AGUControlRegionfd == -1)
    {
        printf("Could not open /dev/mem\n");
        SetError(-1);
        return;
    }



    auto control_region_size = 16*0x1000;
   // printf("AGUControlRegionSize: 0x%x\n", control_region_size);
    AGUControlRegionBaseAddress = (uint64_t)mmap(NULL, control_region_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_AGUControlRegionfd, AGU_CONFIG_BASE);
    if (AGUControlRegionBaseAddress == (uint64_t)MAP_FAILED)
    {
        printf("DTL::API::API() could not map AGUControlRegionBaseAddress\n");
        SetError(-1);
        return;
    }

}

DTL::API::~API() {
    
}

bool DTL::API::CheckHasAvailableResources(CompiledDTLProgram &dtlProg)
{
    int configs_available = 0;
    for (int i = 0; i < hwStat->nMaxConfigs; i++)
    {
        if (((m_ConfigBitmap >> i) & 0x1ULL) == 0)
            configs_available++;
    }
    return configs_available >= dtlProg.m_NeededConfigs;
}


/*
    We will return only the active handle
*/
DTL::DTUResourceHandle *DTL::API::MapTODTUHardware(CompiledDTLProgram &dtlProg)
{
    std::unordered_map<std::string, DTUResourceHandle*> HandleMap;
    /*
        dtlProg.m_Views is already in topological order
    */
   for (auto& view: dtlProg.m_Views)
   {
        DTL::EphemeralRegion* region = AllocEphemeralRegion(view->m_BaseObjectSize, view->m_ViewSize);
        if (region == nullptr)
        {
            exit(-1);
            // *TODO: ADD CLEANUP*
            return nullptr;
        }
        
        DTUResourceHandle* handle = new DTUResourceHandle();
        handle->m_BaseObjectSize = view->m_BaseObjectSize;
        handle->m_ViewSize = view->m_ViewSize;

        // Will fill these out, then move to the newly created handle
        std::string HandleName = view->m_ViewName;;
        std::unordered_map<std::string, MetadataRegion*> MetaDataRegions; 
        EphemeralRegion* MainRegion = region;
        std::unordered_map<std::string, DTUResourceHandle*> SubHandles; // get kernel by name
        std::vector<KernelDependency> DependencyOf; // Track all kernels that are dependent on this one
        std::unordered_map<std::string, uint64_t> byteWriteArgs;
        for (auto& dep: view->m_Dependencies)
        {
            CompiledView* dependency = dep.m_DownstreamView;

            std::string dependency_name = dependency->m_ViewName;
            assert(HandleMap.find(dependency_name) != HandleMap.end()); // should never happen
            DTUResourceHandle* handleDep = HandleMap.at(dependency_name);

            SubHandles.insert({dependency_name, handleDep});
            handleDep->AddDependencyOf(handle, dep.m_DependencyType, dep.m_DependencyNumber, dep.m_MetadataID);
            if (dep.m_DependencyType == DTL_KERNEL_DEPENDENCY::METADATA)
            {
                MetadataRegion* metadataAccessRegion = new MetadataRegion();
                /*
                    We need the non-dtu metadata regions available here
                */
                metadataAccessRegion->isDTURegion = true;
                metadataAccessRegion->m_ResourceHandle = handleDep;
                metadataAccessRegion->m_WriteRegion = handleDep->GetWriteRegion();
                metadataAccessRegion->m_ReadRegion = handleDep->GetReadRegion();
                metadataAccessRegion->m_RegionSize = handleDep->GetLogicalObjectSize(); // held downstream
                metadataAccessRegion->m_PhysAddress = handleDep->GetShadowPAAddressStart();

                MetaDataRegions.insert({dep.m_MetadataID, metadataAccessRegion});
                byteWriteArgs.insert({dependency_name, reinterpret_cast<uint64_t>(metadataAccessRegion->m_PhysAddress)});
            }
        }

        for (auto& metadatadep: view->m_NonDTURegionMetadata)
        {
            MetadataRegion* metadataAccessRegion = GenMetadataRegion(metadatadep);
            MetaDataRegions.insert({metadatadep.m_MetadataID, metadataAccessRegion});
            byteWriteArgs.insert({metadatadep.m_MetadataID, reinterpret_cast<uint64_t>(metadataAccessRegion->m_PhysAddress)});
        }

        handle->m_HandleName = std::move(HandleName);
        handle->m_MetaDataRegions = std::move(MetaDataRegions);
        handle->m_SubHandles = std::move(SubHandles);
        handle->m_DependencyOf = std::move(DependencyOf);
        handle->m_MainRegion = std::move(MainRegion);

        ProgramHardwareByteWrites(MainRegion, view, byteWriteArgs);
        HandleMap.insert({handle->m_HandleName, handle});
    }

    assert(HandleMap.find(dtlProg.m_ActiveViewName) != HandleMap.end());
    DTUResourceHandle* activeHandle = HandleMap.at(dtlProg.m_ActiveViewName);
    return activeHandle;
}

void DTL::API::ReadHardwareInfo()
{
    std::cerr << "DTL::API::ReadHardwareInfo() not yet implemented" << std::endl;
    assert(false);
}

bool DTL::API::CompileAndProgramHardware(const std::string &dtlProgram, EphemeralRegion* region) 
{
    if (!Compile(dtlProgram))
        return false;
    return ProgramHardware(region);
}

static void writeTokenStream(const char * inPath, const char * outPath){
	std::ifstream inStream(inPath);
	if (!inStream.good()){
		std::string msg = "Bad input stream";
		msg += inPath;
		throw new DTL::InternalError(msg.c_str());
	}
	if (outPath == nullptr){
		std::string msg = "No tokens output file given";
		throw new DTL::InternalError(msg.c_str());
	}

	DTL::Scanner scanner(&inStream);
	if (strcmp(outPath, "--") == 0){
		scanner.outputTokens(std::cout);
	} else {
		std::ofstream outStream(outPath);
		if (!outStream.good()){
			std::string msg = "Bad output file ";
			msg += outPath;
			throw new DTL::InternalError(msg.c_str());
		}
		scanner.outputTokens(outStream);
		outStream.close();
	}
}



/*
    compiled_program = api->CompileProgram(program_string)
    if (compiled_program->HasErrors())
        return compiled_program->DumpError();

    
    if (!api->CheckAvailableResources(compiled_program))
        return RESOURCES_INSUFFICIENT;

    DTUResourceHandle* handle = api->MapToDTUHardware(compiled_program);
    if (!handle)
        return DTU_HANDLE_INVALID;

    void* read = handle->GetMainReadRegion();
    void* write = handle->GetMainWriteRegion();
    void* metadataK = hangle->GetMetadataRegion("k");

    struct ByteWrite
    {
        ByteWrite(enum size, value, offset);
        void DoWrite(base);
    }

    struct CompiledView
    {
        uint64_t base_object_size;
        uint64_t view_size;
        std::vector<ByteWrite> bytewrites;

        std::string m_ViewName;
        std::vector<CompiledView*> m_Dependencies;
    };

    struct KernelDependency
    {
        DTUResourceHandle* UpstreamDependency;
        ENUM DependencyType;
        uint8_t DependencyNumber;
    };

    struct CompiledDTLProgram
    {
        DTL_API_ERROR errors;
        std::vector<CompiledView> views;
    };



    class DTUResourceHandle
    {
        std::unordered_map<std::string, MetadataRegion*> MetaDataRegions;
        EphemeralRegion* MainRegion;
        std::unordered_map<std::string, DTUResourceHandle*> SubHandles; // get kernel by name
        std::vector<> DependencyOf; // Track all kernels that are dependent on this one
        CompiledDTLProgram

        void Sync() {
            // If we sync the base object, we also have to sync the upstream objects as well

            // This may require either updating the metadata object or base object, depending
            // on what kind of dependency the object is
        }
    }

*/


/*
    This function is deprecated, and is only maintained to work with legacy DTL::API code

    Please see `DTL::API::CompileProgram()`
*/
bool DTL::API::Compile(const std::string &dtlProgram)
{
    /*
        May want to handle memory management here differently to prevent leaks!
    */

    std::istringstream input(dtlProgram);
    DTL::ProgramNode * root = nullptr;

    DTL::TransformGraph* graph;
    DTL::Scanner scanner(&input);
	DTL::Parser parser(scanner, &graph);
    //parser.set_debug_level(1);  // Turn on debugging
    int err = parser.parse();
	if (err != 0){ printf("parse() errCode: %d\n", err); return false; }


    root = graph->GetActiveProgram();
    auto na = DTL::NameAnalysis::build(root);
	if (na == nullptr)
	{
		ERR("Failed Name Analysis");
		return false;
	}
	auto ta = DTL::TypeAnalysis::build(na);
	if (ta == nullptr)
	{
        ERR("Failed Type Analysis");
        return false;
    }
    auto condInfo = ta->GetConditionalInfo();

    root = static_cast<DTL::ProgramNode*>(DTL::DTLOptimizer::OptimizeStatic(root, DTL::DTL_OPTLEVEL::OPTMAX));
    root = static_cast<DTL::ProgramNode*>(DTL::ASTTransformPass::Transform(root, DTL_OPT_MAX));
    if (root == nullptr)
    {
        ERR("ASTTransformPass() failed");
        return false;
    }
	root->PreprocessArgs();
    
	auto ra = DTL::ResourceAnalysis::build(root, hwStat, ta);
    if (ra == nullptr)
    {
        ERR("ResourceAnalysis Failed");
        return false;
    }
    ra->GetResources()->GetNeededResourceStats();

    ralloc = DTL::ResourceAllocation::build(ra, hwStat, condInfo);
    if (ralloc == nullptr)
    {
        ERR("ResourceAllocation failed\n");
        return false;
    }

    hwStat->VarOutMap.clear(); // otherwise we we will get subsequent collisions
    hwStat->IdxOutMap.clear();
    return true;
    
}

DTL::CompiledDTLProgram DTL::API::CompileProgram(const std::string &dtlProgram)
{
    CompiledDTLProgram returnedCompiledProgram;

    std::istringstream input(dtlProgram);
    DTL::TransformGraph* graph;
    DTL::Scanner scanner(&input);
	DTL::Parser parser(scanner, &graph);
    int err = parser.parse();
	if (err != 0){ 
        returnedCompiledProgram.m_Error = DTL_API_ERROR::COMPILE_ERROR_PARSING; return returnedCompiledProgram; 
    }

    std::string active_kernel_name = graph->GetActiveKernel()->GetIDString();
    returnedCompiledProgram.m_ActiveViewName = active_kernel_name;
    std::unordered_map<std::string, CompiledView*> ViewMap; // to construct dependencies
   
    for (auto& node: graph->BeginReverseTopologicalOrder())
    {
        auto& kernel = node->m_Kernel;
		std::string kernel_id = kernel->GetIDString();
		std::cout <<  kernel_id << "\n";
		DTL::ProgramNode* prog = kernel->GetProgram();
        
        
        auto na = DTL::NameAnalysis::build(prog);
		if (na == nullptr) {
			returnedCompiledProgram.m_Error = DTL_API_ERROR::COMPILE_ERROR_NAME_ANALYSIS;
            return returnedCompiledProgram;
        }

		auto ta = DTL::TypeAnalysis::build(na);
		if (ta == nullptr) {
            returnedCompiledProgram.m_Error = DTL_API_ERROR::COMPILE_ERROR_TYPE_ANALYSIS;
            return returnedCompiledProgram;
        }
        auto condInfo = ta->GetConditionalInfo();

        prog = static_cast<DTL::ProgramNode*>(DTL::DTLOptimizer::OptimizeStatic(prog, DTL::DTL_OPTLEVEL::OPTMAX));
        prog = static_cast<DTL::ProgramNode*>(DTL::ASTTransformPass::Transform(prog, DTL_OPT_MAX));
		if (prog == nullptr) {     
            returnedCompiledProgram.m_Error = DTL_API_ERROR::COMPILE_ERROR_TRANSFORM_PASS;
            return returnedCompiledProgram;
		}
        prog->PreprocessArgs();

        auto ra = DTL::ResourceAnalysis::build(prog, hwStat, ta);
		if (ra == nullptr)
		{
            returnedCompiledProgram.m_Error = DTL_API_ERROR::COMPILE_ERROR_RESOURCE_ANALYSIS;
            return returnedCompiledProgram;
		}
		ra->GetResources()->GetNeededResourceStats();
        
        auto ralloc = DTL::ResourceAllocation::build(ra, hwStat, condInfo);
		if (ralloc == nullptr)
		{
            returnedCompiledProgram.m_Error = DTL_API_ERROR::COMPILE_ERROR_RESOURCE_ALLOCATION;
            return returnedCompiledProgram;
		}
        std::vector<ByteWrite> byteWrites = ralloc->GetByteWrites();


        CompiledView* view = new CompiledView(kernel_id, kernel->GetBackingPhysSize(), ralloc->GetViewSize(), byteWrites);
        /*
            Set the dependencies up
        */
        for (auto& arg: kernel->GetDTURegionMetadataDependencies())
        {
            std::string argName = arg.m_KernelID;
            uint8_t metadataSlotNum = arg.m_DependencyNumber;

            assert(ViewMap.find(argName) != ViewMap.end()); // Should be guaranteed by compiling in reverse topological order
            CompiledView* dependencyViewPtr  = ViewMap.at(argName);
            view->m_Dependencies.push_back({dependencyViewPtr, DTL_KERNEL_DEPENDENCY::METADATA, metadataSlotNum, arg.m_MetadataID});
        }

        for (auto& arg: kernel->GetNonDTURegionMetadataDependencies())
        {
            std::string depName = std::get<0>(arg);
            uint64_t regionSize = std::get<1>(arg);
            uint8_t metadataSlotNum = std::get<2>(arg);
            view->m_NonDTURegionMetadata.push_back({depName, regionSize, metadataSlotNum});
        }


        ViewMap.insert({kernel_id, view});
        returnedCompiledProgram.m_Views.push_back(view);
        returnedCompiledProgram.m_NeededConfigs++;
    }


    hwStat->VarOutMap.clear(); // otherwise we we will get subsequent collisions
    hwStat->IdxOutMap.clear();
    return returnedCompiledProgram;
}

bool DTL::API::ProgramHardware(EphemeralRegion *region, const std::unordered_map<std::string, uint64_t>& metadata_args)
{
    // because we map each config region in the TLRegisterNode I think we can just calculate a new base offset
    uint64_t config_n_base = AGUControlRegionBaseAddress + AGU_CONFIG_OFFSET(region->GetConfig());
    if (metadata_args.size())
        ralloc->UpdateMetadataStreamInfo(metadata_args);
   // printf("DTU_CONFIG_OFFSET(0x%x)\n", AGU_CONFIG_OFFSET(region->GetConfig()));
    ralloc->DoInitStateRegisters(config_n_base);
   // printf("FinishInitStateRegs\n");
    ralloc->DoControlWrites(config_n_base);
  //  printf("Finished Control Writes\n");
    return true;
}

void DTL::API::ProgramHardwareByteWrites(EphemeralRegion *region, CompiledView *view, std::unordered_map<std::string, uint64_t> &args)
{
    uint64_t config_n_base = AGUControlRegionBaseAddress + AGU_CONFIG_OFFSET(region->GetConfig());
    view->DoByteWrites(args, config_n_base);
}

void DTL::API::ResetConfig(int config)
{
    uint64_t config_n_rst = AGUControlRegionBaseAddress + AGU_CONFIG_OFFSET(config) + AGU_CONFIG_RST;
    WRITE_BOOL(config_n_rst, true);
    while(READ_BOOL(config_n_rst) != true){
        
    };
   // WRITE_BOOL(config_n_rst, false);
}

void DTL::API::DebugPrintAllocator() 
{
    m_BuddyAllocator->DebugPrintTree();
}

DTL::EphemeralRegion *DTL::API::AllocEphemeralRegion(uint64_t size_needed)
{
    /*
        We allocate a region offset from the buddy allocator
    */
    uint64_t region_offset = AllocateRegion(size_needed);
   // printf("DTL::API::AllocEphemeralRegion() region_offset 0x%x\n", region_offset);
    if (region_offset == BUDDY_ALLOC_FAILURE)
    {
        SetError(BUDDY_ALLOC_FAILURE);
        return nullptr;
    }

    int config = AllocateConfig();
    if (config == -1)
    {
        SetError(CONFIG_ALLOC_FAILURE);
        return nullptr;
    }
  // printf("DTL::API::AllocEphemeralRegion() config %d\n", config);
    auto ephemeral = new EphemeralRegion(region_offset, next_power_of_two(size_needed), config, m_RealBackingStart, hwStat, AGUControlRegionBaseAddress+AGU_CONFIG_OFFSET(config), next_power_of_two(size_needed));
    return ephemeral;
}

DTL::EphemeralRegion *DTL::API::AllocEphemeralRegion(uint64_t positional_size_needed, uint64_t logical_size_needed)
{
    uint64_t region_offset = AllocateRegion(positional_size_needed);
    if (region_offset == BUDDY_ALLOC_FAILURE)
    {
        SetError(BUDDY_ALLOC_FAILURE);
        return nullptr;
    }

    int config = AllocateConfig();
    if (config == -1)
    {
        SetError(CONFIG_ALLOC_FAILURE);
        return nullptr;
    }

    auto ephemeral = new EphemeralRegion(region_offset, next_power_of_two(logical_size_needed), config, m_RealBackingStart, hwStat, AGUControlRegionBaseAddress+AGU_CONFIG_OFFSET(config), next_power_of_two(positional_size_needed));
    return ephemeral;

}

DTL::EphemeralRegion *DTL::API::CloneEphemeralRegion(EphemeralRegion *ephemeralRegion) {
    m_BuddyAllocator->FindAndIncNode(ephemeralRegion->GetRegionOffset()); // find node, inc in use
    int config = AllocateConfig(); // we must use a new configuration when cloning to avoid overwriting old one
    if (config == -1)
    {
        SetError(CONFIG_ALLOC_FAILURE);
        return nullptr;
    }

    return ephemeralRegion->Clone(config);
}

void DTL::API::FreeEphemeralRegion(EphemeralRegion *ephemeralRegion)
{
    m_BuddyAllocator->FreeNode(ephemeralRegion->GetRegionOffset());
    FreeConfig(ephemeralRegion->GetConfig());
    delete ephemeralRegion;
}

void DTL::API::FreeDTUResourceHandle(DTUResourceHandle *rsrcHandle)
{
    for (auto& metadata: rsrcHandle->m_MetaDataRegions)
    {
        if (metadata.second->IsDTURegion())
            continue;
            m_BuddyAllocator->FreeNode(metadata.second->m_OffsetPhysAddress);
    }
    FreeEphemeralRegion(rsrcHandle->m_MainRegion);
}

uint64_t DTL::API::GetError()
{
    return m_ErrorCode;
}

DTL::AGUHardwareStat *DTL::API::GetHWStat() { return hwStat; }

uint64_t DTL::API::AllocateRegion(uint64_t size) 
{
   // printf("size 0x%llx vs realBackingSize 0x%llx\n", size, m_RealBackingSize);
  assert(size <= m_RealBackingSize);
  uint64_t received_size = next_power_of_two(size);
 // printf("AllocRegion() receivedSize %lld\n", received_size);
  return m_BuddyAllocator->AllocNode(size);
}

DTL::MetadataRegion *DTL::API::GenMetadataRegion(CompiledNonDTUMetadata &metadataDep)
{
    uint64_t size_needed = metadataDep.m_MetadataRegionSize;
        
    // Allocate the positional memory for the metadata
    uint64_t region_offset = AllocateRegion(size_needed);
    if (region_offset == BUDDY_ALLOC_FAILURE)
    {
        SetError(BUDDY_ALLOC_FAILURE);
        return nullptr;
    }
    assert(region_offset < m_RealBackingSize);


    uint64_t physAddrMetadataRegion = m_RealBackingStart + region_offset;
    void* metadataPtr = mmap(
        nullptr,
        size_needed,
        PROT_READ | PROT_WRITE,
        MAP_SHARED,
        m_AGUControlRegionfd,
        physAddrMetadataRegion
    );

    MetadataRegion* metadataAccessRegion = new MetadataRegion();
    metadataAccessRegion->isDTURegion = false;
    metadataAccessRegion->m_ResourceHandle = nullptr;
    metadataAccessRegion->m_WriteRegion = metadataPtr;
    metadataAccessRegion->m_ReadRegion = metadataPtr;
    metadataAccessRegion->m_RegionSize = next_power_of_two(size_needed);
    metadataAccessRegion->m_PhysAddress = physAddrMetadataRegion;
    metadataAccessRegion->m_OffsetPhysAddress = region_offset;

    return metadataAccessRegion;
}

/*
    Sets a bit in the config bitmap, returns -1 on error
*/
int DTL::API::AllocateConfig() {
    for (int i = 0; i < hwStat->nMaxConfigs; i++)
    {
        if (((m_ConfigBitmap >> i) & 0x1ULL) == 0)
        {
            m_ConfigBitmap |= (1ULL << i);
          //  printf("Allocating config %d\n", i);
            assert(i < hwStat->nMaxConfigs);

            uint64_t config_n_rst = AGUControlRegionBaseAddress + AGU_CONFIG_OFFSET(i) + AGU_CONFIG_RST;
            WRITE_BOOL(config_n_rst, false);

            return i;
        }
    }
    return -1;
}

void DTL::API::FreeConfig(int config)
{
    assert(config >= 0 && config < 64);
    assert(((m_ConfigBitmap >> config) & 0x1ULL) != 0);


    m_ConfigBitmap &= (~(1ULL << config));
    ResetConfig(config);
}

void DTL::API::SetError(uint64_t Error)
{
    m_ErrorCode = Error;
}

void DTL::API::SetAGUControlRegionBaseAddr(uint64_t newControlBaseAddr)
{
    AGUControlRegionBaseAddress = newControlBaseAddr;
}

DTL::BuddyNode::BuddyNode(uint64_t TrackedSize, uint64_t TrackedOffset, BuddyNode* parent) 
    : m_TrackedSize(TrackedSize), m_TrackedOffset(TrackedOffset), m_Parent(parent)
{
    //SetFree();
    m_IsFree = 0;
    m_LeftChild = nullptr;
    m_RightChild = nullptr;
}

DTL::BuddyNode::~BuddyNode() 
{

}

void DTL::BuddyNode::Split()
{
    auto new_size = m_TrackedSize/2;
    m_LeftChild = new BuddyNode(new_size, m_TrackedOffset, this);
    m_RightChild = new BuddyNode(new_size, m_TrackedOffset + new_size, this);
}

DTL::BuddyNode *DTL::BuddyNode::GetLeft() 
{
    return m_LeftChild;
}

DTL::BuddyNode *DTL::BuddyNode::GetRight() 
{
    return m_RightChild;
}

void DTL::BuddyNode::SetInUse() 
{
    assert(m_IsFree >= 0);
    m_IsFree++;
}

int DTL::BuddyNode::SetFree() 
{
    
    //printf("isFree %d\n", m_IsFree);
    m_IsFree = std::max(0, m_IsFree - 1);

    return m_IsFree;
}

bool DTL::BuddyNode::isFree() 
{
    assert(m_IsFree >= 0);
    return m_IsFree == 0;
}

bool DTL::BuddyNode::isRoot() 
{ 
    return m_Parent == nullptr;
}

void DTL::BuddyNode::Coalesce()
{
    //printf("node 0x%llx coalescing!\n", m_TrackedOffset);

    auto left = GetLeft();
    auto right = GetRight();

    bool left_good = left == nullptr || left->isFree();
    bool right_good = right == nullptr || right->isFree();

    if (left_good && right_good)
    {
        if (left != nullptr)
        {
            delete left;
            m_LeftChild = nullptr;
        }
        if (right != nullptr)
        {
             delete right;
             m_RightChild = nullptr;
        }

        //printf("[Coalesce() Free Node] offset 0x%x, size 0x%x, in_use %d\n", m_TrackedOffset, m_TrackedSize, !m_IsFree);
        SetFree();
        if (!isRoot() && isFree())
        {
           // printf("coalesce parent!\n");
            m_Parent->Coalesce();
        }
    }


}

void DTL::BuddyNode::DebugPrint() 
{
   // printf("[BuddyNode] offset 0x%x, size 0x%x, in_use %d\n", m_TrackedOffset, m_TrackedSize, !m_IsFree);
    
    if (m_LeftChild != nullptr)
        m_LeftChild->DebugPrint();

    if(m_RightChild != nullptr)
        m_RightChild->DebugPrint();
}

uint64_t DTL::BuddyNode::GetTrackedOffset() 
{
    return m_TrackedOffset;
}

uint64_t DTL::BuddyNode::GetTrackedSize() 
{
    return m_TrackedSize;
}

DTL::BuddyAllocator::BuddyAllocator(uint64_t region_size) 
{
    auto size = region_size;
    if (!is_power_of_two(region_size))
    {
        size = next_power_of_two(region_size);
    }

   // printf("Root size: 0x%llx\n", size);
    m_RootNode = new BuddyNode(size, 0, nullptr);
}

void DTL::BuddyAllocator::FreeNode(uint64_t offset) 
{
    BuddyNode* node = FindNode(offset, m_RootNode);
    assert(node != nullptr);
    assert(node->GetLeft() == nullptr || node->GetLeft()->isFree());
    assert(node->GetRight() == nullptr || node->GetRight()->isFree());
 //   printf("[FreeNode] offset 0x%x, size 0x%x, in_use %d\n", node->GetTrackedOffset(), node->GetTrackedSize(), !node->isFree());

    int isFree = node->SetFree();
    if (node->isFree()) // We increment for shared regions. We must check even after SetFree()
        node->Coalesce();
}

uint64_t DTL::BuddyAllocator::AllocNode(uint64_t size_needed) {

    auto size  = std::max(size_needed, (uint64_t)(0x100000ULL));
    if (!is_power_of_two(size))
        size = next_power_of_two(size);
  uint64_t AllocatedOffset = FindAndAllocFreeNode(size, m_RootNode);
  return AllocatedOffset;
}

void DTL::BuddyAllocator::FindAndIncNode(uint64_t offset)
{
    BuddyNode* node = FindNode(offset, m_RootNode);
    assert(node != nullptr);
    assert(node->GetLeft() == nullptr || node->GetLeft()->isFree());
    assert(node->GetRight() == nullptr || node->GetRight()->isFree());
    assert(!node->isFree()); // only ahoulds be done on nodes already in use

    node->SetInUse();
}

void DTL::BuddyAllocator::DebugPrintTree() 
{
    m_RootNode->DebugPrint();
}

uint64_t DTL::BuddyAllocator::FindAndAllocFreeNode(uint64_t size_needed, BuddyNode *currentNode) 
{
    if (currentNode == nullptr)
        return BUDDY_ALLOC_FAILURE;

    auto currentNodeSize = currentNode->GetTrackedSize();
    if (currentNodeSize < size_needed)
        return BUDDY_ALLOC_FAILURE;


    if (currentNode->isFree())
    {
        if (next_power_of_two(size_needed) == currentNodeSize)
        {
            currentNode->SetInUse();

            return currentNode->GetTrackedOffset();
        }
        else
        {
            currentNode->Split();
            uint64_t check_left = FindAndAllocFreeNode(size_needed, currentNode->GetLeft());
            if (check_left != BUDDY_ALLOC_FAILURE)
            {
                currentNode->SetInUse();
                return check_left;
            }
            uint64_t check_right = FindAndAllocFreeNode(size_needed, currentNode->GetRight());
            if (check_right != BUDDY_ALLOC_FAILURE)
                currentNode->SetInUse();
            return check_right;
        }
    }
    else
    {
        uint64_t check_left = FindAndAllocFreeNode(size_needed, currentNode->GetLeft());
        if (check_left != BUDDY_ALLOC_FAILURE)
        {
            currentNode->SetInUse();
            return check_left;
        }
        uint64_t check_right = FindAndAllocFreeNode(size_needed, currentNode->GetRight());
        if (check_right != BUDDY_ALLOC_FAILURE)
            currentNode->SetInUse();
        return check_right;
    }
    return BUDDY_ALLOC_FAILURE;
}

DTL::BuddyNode *DTL::BuddyAllocator::FindNode(uint64_t offset, BuddyNode* currNode) 
{
    if (currNode == nullptr)
        return nullptr;

    auto currNodeOffset = currNode->GetTrackedOffset();
    if(offset > currNodeOffset && offset > currNodeOffset + currNode->GetTrackedSize()) // we should never be less than
    {
        return nullptr; 
    }   
   // printf("here\n");
    if (offset == currNodeOffset)
    {
        auto left = currNode->GetLeft();
        auto right = currNode->GetRight();


        if (!currNode->isFree() && left == nullptr && right == nullptr)
            return currNode;
        else if (!currNode->isFree() && left != nullptr && right != nullptr && left->isFree() && right->isFree())
        {
            return currNode;
        }
        else
        {
            // the left should have the same offset based on BuddyNode::Split() semantics
            return FindNode(offset, currNode->GetLeft()); 
        }
    }

    if (offset >= currNodeOffset + currNode->GetTrackedSize()/2)
    {
        // the right should always have a greater offset based on BuddyNode::Split() semantics
        return FindNode(offset, currNode->GetRight());
    }
    else
        return FindNode(offset, currNode->GetLeft());


    assert(false);
    return nullptr;
}

DTL::EphemeralRegion::EphemeralRegion(uint64_t positional_region_offset, uint64_t logical_region_size, int config_num, uint64_t physical_backing_start, AGUHardwareStat* hwStat, uint64_t agu_region_base, uint64_t positional_region_size)
    : m_PositionalRegionOffset(positional_region_offset), m_ConfigNum(config_num), m_PhysBackingStart(physical_backing_start),
      m_LogicalRegionSize(logical_region_size), hwStat(hwStat), m_AGUControlRegionBase(agu_region_base), m_PositionalRegionSize(positional_region_size)
{
    printf("EphemeralRegion()\n");
    m_Regionfd = open_fd("/dev/mem");
    assert(m_Regionfd != -1);
    m_DTURuntimeDriverfd = open_fd(DEVICE_FILE);



    /*
        We need the DTU not the AGU config, because the DTU config is where the address indirection comes from
    */
    m_DTUConfigRegion = mmap(NULL, DTU_CONFIG_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_Regionfd, DTU_CONFIG_BASE);
    assert(m_DTUConfigRegion != nullptr && m_DTUConfigRegion != MAP_FAILED);

    // We want the logical region size since this will be used to trap shadow PA addresses
    UPDATE_CONFIG_SIZE((uint64_t)m_DTUConfigRegion, m_ConfigNum, hwStat->nMaxConfigs, m_LogicalRegionSize);

   // printf("m_PhysBackingStart 0x%llx, region_offset 0x%x\n, region_size 0x%x\n", m_PhysBackingStart, region_offset, region_size);
    UPDATE_CONFIG_START((uint64_t)m_DTUConfigRegion, m_ConfigNum, hwStat->nMaxConfigs, m_PositionalRegionOffset + m_PhysBackingStart);
    /*
        We just allocate a region of adequate size and then remap it

        we do not use mmap callback through driver fd because we need to write out the config to the driver,
        if we add this functionality to the driver we can use the mmap callback instead of the remap ioctl
     */
    m_LogicalRegionAccess = mmap(NULL,
                                    m_LogicalRegionSize,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS,
                                    -1,
                                    0);
    assert(m_LogicalRegionAccess != nullptr && m_LogicalRegionAccess != MAP_FAILED);
    if(Sync() == -1)
    {
        printf("Failed to initialize EphemeralRegion()\n");
        return;
    }

    




    assert(m_LogicalRegionAccess != nullptr && m_LogicalRegionAccess != MAP_FAILED);

    //m_CurrentShadowPhysicalAddr = va_to_pa(m_LogicalRegionAccess);
    //printf("Got m_LogicalRegionAccess PA = 0x%llx\n", m_CurrentShadowPhysicalAddr);
    //UPDATE_CONFIG_PHYSMAP(m_DTUConfigRegion, m_ConfigNum, hwStat->nMaxConfigs, m_CurrentShadowPhysicalAddr);


    //int syncret = Sync();
    //if (syncret != 0)
    //{
    //    printf("DTL::EphemeralRegion::EphemeralRegion() %d\n", syncret);
    //}



   m_Positionalfd = open("/dev/mem", O_RDWR);
    m_PositionalRegionAccess =
        mmap(nullptr,
            m_PositionalRegionSize,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            m_Positionalfd,
            m_PhysBackingStart + m_PositionalRegionOffset);
    assert(m_PositionalRegionAccess != nullptr && m_PositionalRegionAccess != MAP_FAILED);
}

DTL::EphemeralRegion::~EphemeralRegion()
{
    munmap(m_LogicalRegionAccess, m_LogicalRegionSize);
    munmap(m_PositionalRegionAccess, m_PositionalRegionSize);
    munmap(m_DTUConfigRegion, DTU_CONFIG_SIZE);
    close(m_Regionfd);
    close(m_DTURuntimeDriverfd);
    close(m_Positionalfd);
}

DTL::EphemeralRegion *DTL::EphemeralRegion::Clone(int new_config)
{
    return new EphemeralRegion(GetRegionOffset(), m_LogicalRegionSize, new_config, m_PhysBackingStart, hwStat, m_AGUControlRegionBase-AGU_CONFIG_OFFSET(m_ConfigNum)+AGU_CONFIG_OFFSET(new_config), m_PositionalRegionSize);
}

void DTL::EphemeralRegion::SetDataSize(uint8_t size)
{
    WRITE_UINT8(m_DTUConfigRegion + DTU_CONFIG_BASE_COLUMNWIDTH, log2ceil(size));
}

int DTL::EphemeralRegion::Sync()
{
    RemapPARequest req;
    req.u_VA = m_LogicalRegionAccess;
    int ret = ioctl(m_DTURuntimeDriverfd, IOCTL_REMAP_PA, &req);
    if (ret == 0) // if successful, update config mapping
    {
        m_CurrentShadowPhysicalAddr = (uint64_t)req.u_NewPA;
        UPDATE_CONFIG_PHYSMAP(m_DTUConfigRegion,
                              m_ConfigNum,
                              hwStat->nMaxConfigs,
                              m_CurrentShadowPhysicalAddr);
        // printf("new PA 0x%llx\n", m_CurrentShadowPhysicalAddr);
    } else
        printf("DTL::EphemeralRegion::Sync() ioctl(IOCTL_REMAP_PA) failed!\n");

  return ret;
}



void DTL::EphemeralRegion::UpdateBackingObjectAddr(uint64_t addr)
{
    UPDATE_CONFIG_START((uint64_t)m_DTUConfigRegion, m_ConfigNum, hwStat->nMaxConfigs, addr);
}

void DTL::EphemeralRegion::UpdateMetadataObjectAddr(uint8_t metadataObjectNumber, uint64_t addr)
{
    uint32_t metadataRegOffset = hwStat->GetMetaDataStreamRegOffset(4) + BYTES_METADATASTREAM*metadataObjectNumber;
    uint64_t metadataObjMMIOAddr = m_AGUControlRegionBase + metadataRegOffset;
    WRITE_UINT64(metadataObjMMIOAddr, addr);
}

void *DTL::EphemeralRegion::GetHeadlessReadRegion() {
  return m_LogicalRegionAccess;
}

void *DTL::EphemeralRegion::GetHeadlessWriteregion()
{
    return m_PositionalRegionAccess;
}

void DTL::EphemeralRegion::GuardedWrite_8(uint64_t offset, uint8_t data)
{
   // assert(offset < m_RegionSize);
    WRITE_UINT8(m_PositionalRegionAccess + offset, data);
}

uint8_t DTL::EphemeralRegion::GuardedRead_8(uint64_t offset) {
 // assert(offset < m_RegionSize);
  return *((uint16_t *)m_LogicalRegionAccess);
}

uint16_t DTL::EphemeralRegion::GuardedRead_16(uint64_t offset) {
  //assert(offset < m_RegionSize);
  return *((uint16_t *)m_LogicalRegionAccess);
}

uint32_t DTL::EphemeralRegion::GuardedRead_32(uint64_t offset) {
 // assert(offset < m_RegionSize);
  return *((uint32_t *)m_LogicalRegionAccess);
}

uint64_t DTL::EphemeralRegion::GuardedRead_64(uint64_t offset) {
//  assert(offset < m_RegionSize);
  return *((uint64_t *)m_LogicalRegionAccess);
}

float DTL::EphemeralRegion::GuardedRead_Float(uint64_t offset) {
  //assert(offset < m_RegionSize);
  return *((float *)m_LogicalRegionAccess);
}




DTL::CompiledView::CompiledView(std::string view_name, uint64_t base_object_size, uint64_t view_size, std::vector<ByteWrite> &bytewrites) 
    : m_BaseObjectSize(base_object_size), m_ViewSize(view_size), m_ByteWrites(bytewrites), m_ViewName(view_name)
{
}

void DTL::CompiledView::DoByteWrites(std::unordered_map<std::string, uint64_t> &args, uint64_t baseAddr)
{
    for (auto& bw: m_ByteWrites)
    {
        if (bw.m_RequiresArg)
        {
            std::string& requiredArgID = bw.m_RequiredArgID;
            assert(args.find(requiredArgID) != args.end());
            bw.m_Value = args.at(requiredArgID);
            bw.DoWrite(baseAddr);
        }
        else
            bw.DoWrite(baseAddr);
    }
}

DTL::DTUResourceHandle::~DTUResourceHandle()
{
     for (auto& metadata: m_MetaDataRegions)
        delete metadata.second;

    for (auto& subhandle: m_SubHandles)
        delete subhandle.second;
}

uint64_t DTL::DTUResourceHandle::GetShadowPAAddressStart() const
{
    return m_MainRegion->GetCurrentShadowPhysicalAddr();
}

/*
    We also need to sync upstream dependents
*/
void DTL::DTUResourceHandle::Sync()
{
    assert(m_MainRegion->Sync() == 0);
    std::tuple<uint64_t, uint64_t, uint64_t> addressTriple = m_MainRegion->GetAddrMappingTriple();
    for (auto& dep: m_DependencyOf)
    {
        //DTL::DTUResourceHandle* dependencyHandle = dep.m_UpstreamDependency;
        SyncDependent(dep);
    }
}

void DTL::DTUResourceHandle::SyncFrom(std::string caller, uint8_t dependency_num, std::tuple<uint64_t, uint64_t, uint64_t>& addressTriple)
{
    #define VA_IDX 0 
    #define SHADOWPA_IDX 1
    #define POSITIONALPA_IDX 2
    assert(m_MetaDataRegions.find(caller) != m_MetaDataRegions.end());
    uint64_t shadow_pa = std::get<SHADOWPA_IDX>(addressTriple);

    m_MetaDataRegions.at(caller)->m_PhysAddress = shadow_pa;

    m_MainRegion->UpdateMetadataObjectAddr(dependency_num, shadow_pa);
    assert(m_MainRegion->Sync() == 0);
    for (auto& dep: m_DependencyOf)
        SyncDependent(dep);
    #undef VA_IDX 0 
    #undef SHADOWPA_IDX 1
    #undef POSITIONALPA_IDX 2
}


/*
    The dependency will call this on the kernel that holds this dependency

    For example if we have a dependency chain of A <--dependent on-- B

    A will call:
        B->SyncDependent(dep, A's new synced address)
*/
void DTL::DTUResourceHandle::SyncDependent(KernelDependency dep)
{
    /*
        So here, we need to update addresses/MMIOs

        We want to avoid recompiling
    */

    std::tuple<uint64_t, uint64_t, uint64_t> addressTriple = m_MainRegion->GetAddrMappingTriple();
    switch (dep.m_DependencyType)
    {
        case DTL_KERNEL_DEPENDENCY::BASE_OBJECT: // we just update the base object addr in MMIO for the config
        {
            assert(false); // NOT YET IMPLEMENTED
            dep.m_UpstreamDependency->SyncFrom(dep.m_MetadataVarID, dep.m_DependencyNumber, addressTriple);
            break;
        }
        case DTL_KERNEL_DEPENDENCY::METADATA: // we just update the metadata object addr in MMIO for the config
        {
            dep.m_UpstreamDependency->SyncFrom(dep.m_MetadataVarID,  dep.m_DependencyNumber, addressTriple);
            break;
        }
        default:
            assert(false); // should never happen
    }


}

void DTL::DTUResourceHandle::AddDependencyOf(DTUResourceHandle *upstreamDependency, DTL_KERNEL_DEPENDENCY dependencyType, uint8_t dependencyNum, std::string metadataVarID)
{
    m_DependencyOf.push_back({upstreamDependency, dependencyType, dependencyNum, metadataVarID});
}

DTL::CompiledDTLProgram::CompiledDTLProgram()
{
}

DTL::CompiledDTLProgram::~CompiledDTLProgram()
{
}

DTL::MetadataRegion::~MetadataRegion()
{
    if (isNormalMemoryRegion())
        munmap(m_ReadRegion, m_RegionSize);  
}
