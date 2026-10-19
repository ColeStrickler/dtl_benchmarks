#include "resource_alloc_pass.hpp"
#include "ast.hpp"

DTL::ByteWrite::ByteWrite(uint64_t offset, uint64_t value, BYTEW_SIZE size)
    : m_Offset(offset), m_Value(value), m_SizeWrite(size), m_RequiresArg(false), m_RequiredArgID("")
{
}

void DTL::ByteWrite::DoWrite(uint64_t base_addr)
{
    switch(m_SizeWrite)
    {
        case BYTEW_SIZE::UINT8:     WRITE_UINT8(base_addr + m_Offset, m_Value); break;
        case BYTEW_SIZE::UINT16:    WRITE_UINT16(base_addr + m_Offset, m_Value); break;
        case BYTEW_SIZE::UINT32:    WRITE_UINT32(base_addr + m_Offset, m_Value); break;
        case BYTEW_SIZE::UINT64:    WRITE_UINT64(base_addr + m_Offset, m_Value); break;
        default:
            assert(false);
    }
}

std::string DTL::ByteWrite::PrintString(uint64_t base_addr)
{
    switch(m_SizeWrite)
    {
        case BYTEW_SIZE::UINT8:     return "WRITE_UINT8( " + to_hex(base_addr + m_Offset) + "," + to_hex(m_Value) + ");\n"; break;
        case BYTEW_SIZE::UINT16:    return "WRITE_UINT16(" + to_hex(base_addr + m_Offset) + "," + to_hex(m_Value) + ");\n"; break;
        case BYTEW_SIZE::UINT32:    return "WRITE_UINT32(" + to_hex(base_addr + m_Offset) + "," + to_hex(m_Value) + ");\n"; break;
        case BYTEW_SIZE::UINT64:    return "WRITE_UINT64(" + to_hex(base_addr + m_Offset) + "," + to_hex(m_Value) + ");\n"; break;
        default:
            assert(false);
    }
}

void DTL::AGUHardwareStat::AppendControlWrite(std::vector<ByteWrite>& writes,
                                            int numOutStatement, int layer,
                                            int inRegNumber, int outRegNumber)
{
    if (outRegNumber == 255 || inRegNumber == 255)
        return;
    assert(outRegNumber < __UINT8_MAX__);

    unsigned int layerByteOffset = layer * bytesLayer;
    unsigned int cellByteOffset = inRegNumber * bytesCell;
    unsigned int outStatementOffset = numOutStatement * bytesOutStatement;
    auto hash_str = std::to_string(0) + "_" + std::to_string(layerByteOffset) + "_" +
                    std::to_string(cellByteOffset) + "_" + std::to_string(outStatementOffset);
    auto idx_str = std::to_string(layer) + "_" + std::to_string(outRegNumber);

    int cell_index = VarOutMap[hash_str]++;
    int routingIdx = IdxOutMap[idx_str]++;
    assert(VarOutMap[hash_str] <= bytesCell);
    assert(IdxOutMap[idx_str] <= bytesCell);

    uint8_t write_val = static_cast<uint8_t>(outRegNumber);
    write_val |= (routingIdx << idxBit);
    writes.emplace_back(layerByteOffset + cellByteOffset + outStatementOffset + cell_index,
                        write_val, BYTEW_SIZE::UINT8);
}

void DTL::AGULayer::AppendControlWrites(std::vector<ByteWrite>& writes, int numOutStmt,
                                       int layer, AGUHardwareStat* hwstat)
{
    for (auto& unit : inputRouting)
    {
        hwstat->AppendControlWrite(writes, numOutStmt, layer, unit->InputA, unit->RegAssignment);
        hwstat->AppendControlWrite(writes, numOutStmt, layer, unit->InputB, unit->RegAssignment);
    }
}

void DTL::OutStmtRouting::AppendControlWrites(std::vector<ByteWrite>& writes,
                                             int numoutstatement) const
{
    for (auto& e : LayerRouting)
        e.second->AppendControlWrites(writes, numoutstatement, e.first, hwStat);
}

std::vector<DTL::ByteWrite> DTL::ResourceAllocation::GetByteWrites()
{
    std::vector<ByteWrite> writes;
    const uint32_t byte_width = 4;

    for (auto& reg : loopRegisters)
    {
        writes.emplace_back(hwStat->GetLoopRegsOffset() + reg.reg_num * byte_width,
                            static_cast<uint32_t>(reg.init_value), BYTEW_SIZE::UINT32);
        writes.emplace_back(hwStat->GetLoopIncRegsOffset(byte_width) + reg.reg_num * byte_width,
                            static_cast<uint32_t>(reg.increment_condition), BYTEW_SIZE::UINT32);

        uint64_t offset = hwStat->GetMagicRegsOffset(byte_width) +
                         (hwStat->nForLoopRegisters - 1 - reg.reg_num) * hwStat->bytesMagic;
        writes.emplace_back(offset, reg.hwDivMagic.M, BYTEW_SIZE::UINT64);
        writes.emplace_back(offset + 0x8, reg.hwDivMagic.s, BYTEW_SIZE::UINT32);
        writes.emplace_back(offset + 0xc, static_cast<uint8_t>(reg.hwDivMagic.add_indicator),
                            BYTEW_SIZE::UINT8);
    }

    for (auto& c : rsrcAnalysis->ConstValueMap)
        writes.emplace_back(hwStat->GetConstantRegsOffset(byte_width) + c.first * byte_width,
                            static_cast<uint32_t>(c.second), BYTEW_SIZE::UINT32);

    for (auto& ca : constArrayRegisters)
    {
        uint64_t offset = hwStat->GetConstArrayOffset(byte_width);
        for (size_t i = 0; i < ca.values.size(); i++)
        {
            assert(i < hwStat->nConstArraySize);
            writes.emplace_back(offset + i * byte_width, ca.values[i], BYTEW_SIZE::UINT32);
        }
        writes.emplace_back(offset + byte_width * hwStat->nConstArraySize,
                            static_cast<uint8_t>(ca.index_reg_num), BYTEW_SIZE::UINT8);
    }

    ByteWrite using_metadata_stream(USING_METADATATASTREAM_REG,
                                    !MetadataStreamMMIOInfo.empty(),
                                    BYTEW_SIZE::UINT8);
    writes.push_back(using_metadata_stream);

    for (size_t stream_num = 0; stream_num < MetadataStreamMMIOInfo.size(); stream_num++)
    {
        auto& ms = MetadataStreamMMIOInfo[stream_num];
        uint64_t offset = hwStat->GetMetaDataStreamRegOffset(byte_width);

        ByteWrite stream_address(offset + 8 * stream_num,
                                 ms.streamPhysStart,
                                 BYTEW_SIZE::UINT64);
        stream_address.m_RequiresArg = ms.useArgID;
        stream_address.m_RequiredArgID = ms.argID;
        writes.push_back(stream_address);

        ByteWrite stream_loop_register(
            offset + 8 * hwStat->nMetadataStreams + stream_num,
            static_cast<uint8_t>(ForLoopIDToMapping(ms.id_index) - GetForLoopRegOffset()),
            BYTEW_SIZE::UINT8);
        writes.push_back(stream_loop_register);
    }

    writes.emplace_back(USED_OUTSTMT_REG, static_cast<uint8_t>(OutStatementRouting.size()), BYTEW_SIZE::UINT8);
    writes.emplace_back(USED_FORLOOP_REG, static_cast<uint8_t>(loopRegisters.size()), BYTEW_SIZE::UINT8);
    writes.emplace_back(USED_OUT_PERCOND_REG,
                        static_cast<uint8_t>(CondInfo.conditionalCode != CondCode::DISABLE ?
                            CondInfo.outStatementsPerCond : rsrcAnalysis->GetResources()->nOutStatements),
                        BYTEW_SIZE::UINT8);
    writes.emplace_back(USE_CONDCODE_REG, static_cast<uint8_t>(CondInfo.conditionalCode), BYTEW_SIZE::UINT8);
    for (size_t i = 0; i < CondInfo.condIndices.size(); i++)
        writes.emplace_back(USE_CONDITIONAL_IDX_REG + i,
                            static_cast<uint8_t>(ForLoopIDToMapping(CondInfo.condIndices[i]->getName()) -
                                hwStat->nConstRegisters - hwStat->nConstArray),
                            BYTEW_SIZE::UINT8);

    for (size_t i = 0; i < OutStatementRouting.size(); i++)
    {
        OutStatementRouting[i]->AppendControlWrites(writes, i);
        hwStat->VarOutMap.clear();
        hwStat->IdxOutMap.clear();
    }
    return writes;
}



void DTL::ProgramNode::resourceAllocation(ResourceAllocation *ralloc, int depth)
{
    //printf("ProgramNode::resourceAllocation()\n");
    for (auto& stmt: myStatements)
        stmt->resourceAllocation(ralloc, false);
}



void DTL::ConstDeclNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{

    /*
        We have already mapped the ID of the constant declared here to a register
        in ResourceAnalysis
    */
    return; 

}




void DTL::ConstArrayDeclNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{

    /*
        We have already mapped the ID of the ConstArray declared here to a register
        in ResourceAnalysis
    */

    std::vector<uint32_t> values;

    for (auto& val: myVals)
    {
        values.push_back(static_cast<uint32_t>(val->GetVal()));
    }


    
    ralloc->AllocConstArray(myID->getName(), ralloc->rsrcAnalysis->GetConstArrayRegMapping(myID->getName()), values);

    return; 

}


void DTL::PostIncStmtNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{
    return;
}



void DTL::ForStmtNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{
    
    int x = Collapse(ralloc);
    if (x != 0)
    {
        std::cerr << "Error in ForStmtNode::Collapse() during resourceAllocation()\n";
        exit(-1);
    }

    /*
        Must do this here to successfully map onto loop register indexing scheme
    */
    ralloc->MapForLoopReg(GetInitVar());
    ralloc->AllocLoopRegister(RegInitValue, RegMaxValue);
    

    for (auto& stmt: myStatements)
        stmt->resourceAllocation(ralloc, false);

    
    
}

void DTL::OutStmtNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{
    int usedLayers = myExp->GetMaxDepth();
    ralloc->NewOutStatement(usedLayers);
    myExp->resourceAllocation(ralloc, 0);
    ralloc->SetAnswer(myExp);
}


void DTL::IntTypeNode::resourceAllocation(ResourceAllocation *ralloc, int depth)
{
    return;
}

void DTL::MetadataStreamTypeNode::resourceAllocation(ResourceAllocation *ralloc, int depth)
{
    return;
}



void DTL::ArrayIndexNode::resourceAllocation(ResourceAllocation *ralloc, int depth)
{

    auto out = ralloc->GetCurrentOutStatement();
    auto& ra = ralloc->rsrcAnalysis;

    if (isMetaStreamIndex) // we perform the mapping in the decl because we have all the bindings there
    {
        int unit = ralloc->MetadataStreamIDToMapping(myID->getName());
        out->MapNodeFuncUnit(this, unit, depth);
        return;
    }


    



    //int index_id = ra->GetConstRegMapping(myIndexVar->getName());
    //assert(index_id == -1); // We should never be indexing with a non loop
    int index_id = ralloc->ForLoopIDToMapping(myIndexVar->getName());
    assert(index_id != -1);

    int array_id = ra->GetConstArrayRegRoutingIndex(myID->getName());
    assert(array_id != -1);

    printf("array_id %d index_id %d\n", array_id, index_id);

    auto hwStat = ralloc->hwStat;
    // Ensure we do not already have a binding
    assert(ralloc->BindArrayIndex(array_id - hwStat->nConstRegisters, index_id - hwStat->nConstRegisters - hwStat->nConstArray));

   // printf("ArrayIndexNode depth %d\n");
    out->MapNodeFuncUnit(this, array_id, depth);


}




void DTL::IDNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{
    auto out = ralloc->GetCurrentOutStatement();
    auto& ra = ralloc->rsrcAnalysis;

    int id = ra->GetConstRegMapping(getName());
   // 
   // printf("ID %d %s\n", id, name.c_str());
    if (id == -1) // if a for loop register or metadatatype
    {   
        auto type  = ralloc->rsrcAnalysis->GetNodeType(this);
        if (type->isMetadataStream())
            id = ralloc->MetadataStreamIDToMapping(getName());
        else
            id = ralloc->ForLoopIDToMapping(getName());
    }

    out->MapNodeFuncUnit(this, id, depth); // we just shift the mapping over
}

void DTL::IntLitNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{
    auto out = ralloc->GetCurrentOutStatement();
    auto& ra = ralloc->rsrcAnalysis;
    out->MapNodeFuncUnit(this, ra->GetConstRegMapping(std::to_string(myNum)), depth); // we just shift the mapping over
}

void DTL::PlusNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{
    auto outstmt = ralloc->GetCurrentOutStatement();
    assert(outstmt != nullptr);
    myExp1->resourceAllocation(ralloc, depth+1);
    myExp2->resourceAllocation(ralloc, depth+1);

    if (isPassThrough())
    {
        ralloc->PassThru(depth, this, myExp1);
    }
    else
    {
        ralloc->AddUnit(depth, this, myExp1, myExp2);
    }

}


void DTL::MinusNode::resourceAllocation(ResourceAllocation *ralloc, int depth) 
{
    auto outstmt = ralloc->GetCurrentOutStatement();
    assert(outstmt != nullptr);

    myExp1->resourceAllocation(ralloc, depth+1);
    myExp2->resourceAllocation(ralloc, depth+1);
    ralloc->SubUnit(depth, this, myExp1, myExp2);

}


void DTL::TimesNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{
    auto outstmt = ralloc->GetCurrentOutStatement();
    assert(outstmt != nullptr);
    myExp1->resourceAllocation(ralloc, depth+1);
    myExp2->resourceAllocation(ralloc, depth+1);

    ralloc->MultUnit(depth, this, myExp1, myExp2);
    
}

void DTL::LessNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{
    return;
}

void DTL::LessEqNode::resourceAllocation(ResourceAllocation* ralloc, int depth)
{
    return;
}


/*
    We offer these methods to collapse for loop into a structure
    suitable for resource allocation. 
*/
int DTL::ForStmtNode::Collapse(ResourceAllocation* ralloc)
{
    int m = myCondExp->Collapse(ralloc);
    int init = myInit->Collapse(ralloc);

    RegMaxValue = m;
    RegInitValue = init;
    return 0;
}

int DTL::MinusNode::Collapse(ResourceAllocation *ralloc)
{
    assert(false);
    return 0;
}

void DTL::IfStmtNode::resourceAllocation(ResourceAllocation *ralloc, int depth) 
{
    assert(false);  // should already be collapsed in transform pass
}

void DTL::SwitchStmtNode::resourceAllocation(ResourceAllocation *ralloc, int depth)
{
    assert(false);  // should already be collapsed in transform pass
}


int DTL::ConstDeclNode::Collapse(ResourceAllocation* ralloc)
{
    myID->Collapse(ralloc);
    int val = myVal->Collapse(ralloc);
    return val;
}

int DTL::ConstArrayDeclNode::Collapse(ResourceAllocation *ralloc)
{
    assert(false); // should never hit
    return 0;
}


int DTL::IntLitNode::Collapse(ResourceAllocation* ralloc)
{
    return myNum;
}


int DTL::OutStmtNode::Collapse(ResourceAllocation* ralloc)
{
    assert(false); // should never hit
    return 0;
}

int DTL::IfStmtNode::Collapse(ResourceAllocation *ralloc)
{
    assert(false); // should never hit
    return 0;
}


int DTL::SwitchStmtNode::Collapse(ResourceAllocation *ralloc)
{
    assert(false); // should never hit
    return 0;
}

int DTL::PlusNode::Collapse(ResourceAllocation* ralloc)
{
    assert(false); // should never hit
    return 0;
}

int DTL::TimesNode::Collapse(ResourceAllocation* ralloc)
{
    assert(false); // should never hit
    return 0;
}


int DTL::ArrayIndexNode::Collapse(ResourceAllocation *ralloc)
{
    assert(false);
    return 0;
}



int DTL::LessNode::Collapse(ResourceAllocation* ralloc)
{
    
    return myExp2->Collapse(ralloc);
}

int DTL::LessEqNode::Collapse(ResourceAllocation* ralloc)
{
    assert(false); // possibly implement later
    return 0;
}

int DTL::PostIncStmtNode::Collapse(ResourceAllocation* ralloc)
{
    assert(false); // should never hit
    return 0;
}

int DTL::IDNode::Collapse(ResourceAllocation* ralloc)
{
    //ralloc->MapForLoopReg(getName());
    return 0;
}

int DTL::IntTypeNode::Collapse(ResourceAllocation* ralloc)
{
    assert(false); // should never hit
    return 0;
}

int DTL::MetadataStreamTypeNode::Collapse(ResourceAllocation *ralloc)
{
    assert(false);
    return 0;
}

bool DTL::OutStmtRouting::PrintDigraph(const std::string &file)
{
	std::ofstream outfile(file);  // open for writing
	if (!outfile.is_open()) {
		std::cerr << "Failed to open file.\n";
		return false;
	}

	outfile << "digraph G {\n";
    /*
        We will map the constants in layer0
    */
    //for (int i = 0; i < LayerRouting.size(); i++)
    //{
    //    outfile << LayerRouting[i]->PrintDigraph(i);
    //}

    for (auto& e: LayerRouting)
    {
        int i = e.first;
        //printf("i %d\n", i);
        outfile << e.second->PrintDigraph(i+1);
    }


	outfile << "}\n";

    outfile.close();

    return true;
}

std::string DTL::OutStmtRouting::PrintControlWrites(uint64_t baseaddr, int numoutstatement) const
{
    std::string ret;
   // printf("outstmt num %d\n", numoutstatement);
    for (auto& e: LayerRouting)
    {
         int i = e.first;
         ret += e.second->PrintControlWrites(baseaddr, numoutstatement, i, hwStat);
         
    }
    return ret;
}

void DTL::OutStmtRouting::DoControlWrites(uint64_t baseaddr, int numoutstatement) const
{
    for (auto& e: LayerRouting)
    {
         int i = e.first;
         e.second->DoControlWrites(baseaddr, numoutstatement, i, hwStat);     
    }
}

std::string DTL::AGULayer::PrintControlWrites(uint64_t baseaddr, int numOutStmt, int layer, AGUHardwareStat *hwstat)
{
    std::string ret;
    for (auto& unit : inputRouting)
    {
        /*
            I think this is correct, we map inputs to the units assignment, but I am not completely sure this will map correctly

            We will want to check and confirm here first when we debugs
        */
        ret += hwstat->PrintControlWrite(baseaddr, numOutStmt, layer, unit->InputA, unit->RegAssignment);
        ret += hwstat->PrintControlWrite(baseaddr, numOutStmt, layer, unit->InputB, unit->RegAssignment);
    }
    return ret;
}

void DTL::AGULayer::DoControlWrites(uint64_t baseaddr, int numOutStmt, int layer, AGUHardwareStat *hwstat)
{
    for (auto& unit : inputRouting)
    {
        /*
            I think this is correct, we map inputs to the units assignment, but I am not completely sure this will map correctly

            We will want to check and confirm here first when we debugs
        */
        hwstat->DoControlWrite(baseaddr, numOutStmt, layer, unit->InputA, unit->RegAssignment);
        hwstat->DoControlWrite(baseaddr, numOutStmt, layer, unit->InputB, unit->RegAssignment);
    }
}

std::string DTL::AGULayer::PrintDigraph(int layer) const
{
    std::string ret;
   // printf("inputRouting.size() %d\n", inputRouting.size());
    for (auto& unit: inputRouting)
        ret += unit->toString(layer);


    return ret;
}

std::string DTL::MultUnit::toString(int layer)
{
    std::string node_name = "\"" + std::to_string(layer) + "_" + std::to_string(RegAssignment) + "\"";
    std::string label = node_name + "[label=\"Mult_" +  std::to_string(layer) + "_" + std::to_string(RegAssignment)+ "\"];";


    std::string inA = "\"" + std::to_string(layer-1) + "_" + std::to_string(InputA) + "\"" + " -> " + node_name + ";";
    std::string inB = "\"" + std::to_string(layer-1) + "_" + std::to_string(InputB) + "\"" + " -> " + node_name + ";";

    return label + "\n" + inA + "\n" + inB + "\n";
}

std::string DTL::PassThrough::toString(int layer)
{
  //  printf("pass thru\n");
    std::string node_name = "\"" + std::to_string(layer) + "_" + std::to_string(RegAssignment) + "\"";
    std::string label = node_name + "[label=\"PassThru" +  std::to_string(layer) + "_" + std::to_string(RegAssignment)+ "\"];";


    std::string inA = "\"" + std::to_string(layer-1) + "_" + std::to_string(InputA) + "\"" + " -> " + node_name + ";";

    return label + "\n" + inA + "\n";
}

std::string DTL::AddUnit::toString(int layer)
{
    std::string node_name = "\"" + std::to_string(layer) + "_" + std::to_string(RegAssignment) + "\"";
    std::string label = node_name  + "[label=\"Plus" +  std::to_string(layer) + "_" + std::to_string(RegAssignment)+ "\"];";


    std::string inA = "\"" + std::to_string(layer-1) + "_" + std::to_string(InputA) + "\"" + " -> " + node_name + ";";
    std::string inB = "\"" + std::to_string(layer-1) + "_" + std::to_string(InputB) + "\"" + " -> " + node_name + ";";

    return label + "\n" + inA + "\n" + inB + "\n";
}


std::string DTL::SubUnit::toString(int layer) {
    std::string node_name = "\"" + std::to_string(layer) + "_" + std::to_string(RegAssignment) + "\"";
    std::string label = node_name  + "[label=\"Sub" +  std::to_string(layer) + "_" + std::to_string(RegAssignment)+ "\"];";


    std::string inA = "\"" + std::to_string(layer-1) + "_" + std::to_string(InputA) + "\"" + " -> " + node_name + ";";
    std::string inB = "\"" + std::to_string(layer-1) + "_" + std::to_string(InputB) + "\"" + " -> " + node_name + ";";

    return label + "\n" + inA + "\n" + inB + "\n";
}


void DTL::ResourceAllocation::PrintInitStateRegisters(const std::string &file, uint64_t baseaddr)
{
        std::ofstream outfile(file, std::ios::app);  // open for writing
        if (!outfile.is_open()) {
            std::cerr << "Failed to open file.\n";
            return;
        }

        std::string write;
        for (auto& f : loopRegisters)
        {
            write += hwStat->PrintForLoopWrite(baseaddr, f, 4);
            // We will place the for loop registers immediately after the routing registers
        }


        for (auto& c : rsrcAnalysis->ConstValueMap)
        {
            write += hwStat->PrintConstRegWrite(baseaddr, c.first, c.second, 4);
        }
       // printf("const array\n");
        for (auto& c: constArrayRegisters)
        {
            write += hwStat->PrintConstArrayWrite(baseaddr, c, 4);
        }

        if (MetadataStreamMMIOInfo.size() != 0)
        {
             write += "WRITE_BOOL(" + to_hex(baseaddr+USING_METADATATASTREAM_REG) + ",1);";
             std::cout << "HEX: " << to_hex(baseaddr+USING_METADATATASTREAM_REG) << std::endl;
        }
        else
            write += "WRITE_BOOL(" + to_hex(baseaddr+USING_METADATATASTREAM_REG) + ",0);";

  
        for (int stream_num = 0; stream_num < MetadataStreamMMIOInfo.size(); stream_num++)
        {
            auto& ms = MetadataStreamMMIOInfo[stream_num];
            write += hwStat->PrintMetadataStreamWrite(baseaddr, ms, stream_num, ForLoopIDToMapping(ms.id_index)-GetForLoopRegOffset(),4);
        }

        write += "\nWRITE_UINT8(" + to_hex(baseaddr+USED_OUTSTMT_REG) + "," + to_hex(static_cast<uint8_t>(OutStatementRouting.size())) +   ");\n";
        write += "\nWRITE_UINT8(" + to_hex(baseaddr+USED_FORLOOP_REG) + "," + to_hex(static_cast<uint8_t>(loopRegisters.size())) +   ");\n";
                
        write  += "\nWRITE_UINT8(" + to_hex(baseaddr+USED_OUT_PERCOND_REG) + ","  +         \
            to_hex(static_cast<uint8_t>(CondInfo.conditionalCode != CondCode::DISABLE ? CondInfo.outStatementsPerCond : rsrcAnalysis->GetResources()->nOutStatements))+");\n";;
        write  += "\nWRITE_UINT8(" + to_hex(baseaddr+USE_CONDCODE_REG) + "," +               \
            to_hex(static_cast<uint8_t>(CondInfo.conditionalCode)) + ");\n";




        for (int i = 0; i < CondInfo.condIndices.size(); i++)
        {
            //printf("cond %s --> %d\n", CondInfo.condIndices[i]->getName().c_str(), ForLoopIDToMapping(CondInfo.condIndices[i]->getName()) - hwStat->nConstRegisters - hwStat->nConstArray);
             write  += "\nWRITE_UINT8(" + to_hex(baseaddr+USE_CONDITIONAL_IDX_REG+i) + "," +        \
                to_hex(static_cast<uint8_t>(ForLoopIDToMapping(CondInfo.condIndices[i]->getName()) - hwStat->nConstRegisters - hwStat->nConstArray)) + ");\n";
        }
        outfile << write;
        outfile.close();
}

void DTL::ResourceAllocation::DoInitStateRegisters(uint64_t baseAddr)
{
    for (auto& f : loopRegisters)
    {
        hwStat->DoForLoopWrite(baseAddr, f, 4);
        // We will place the for loop registers immediately after the routing registers
    }
    for (auto& c : rsrcAnalysis->ConstValueMap)
    {
       hwStat->DoConstRegWrite(baseAddr, c.first, c.second, 4);
    }
    
    for (auto& ca : constArrayRegisters)
    {
        hwStat->DoConstArrayWrite(baseAddr, ca, 4);
    }

    if (MetadataStreamMMIOInfo.size() != 0)
        WRITE_BOOL(baseAddr+USING_METADATATASTREAM_REG, 1);
    else
        WRITE_BOOL(baseAddr+USING_METADATATASTREAM_REG, 0);

    
    for (int stream_num = 0; stream_num < MetadataStreamMMIOInfo.size(); stream_num++)
    {
        auto& ms = MetadataStreamMMIOInfo[stream_num];
        hwStat->DoMetadataStreamWrite(baseAddr, ms, stream_num, ForLoopIDToMapping(ms.id_index)-GetForLoopRegOffset(), 4);
    }


    WRITE_UINT8(baseAddr+USED_OUTSTMT_REG, static_cast<uint8_t>(OutStatementRouting.size()));
    WRITE_UINT8(baseAddr+USED_FORLOOP_REG, static_cast<uint8_t>(loopRegisters.size()));



    WRITE_UINT8(baseAddr+USED_OUT_PERCOND_REG, static_cast<uint8_t>(CondInfo.conditionalCode != CondCode::DISABLE ? CondInfo.outStatementsPerCond : rsrcAnalysis->GetResources()->nOutStatements));
    WRITE_UINT8(baseAddr+USE_CONDCODE_REG, static_cast<uint8_t>(CondInfo.conditionalCode));
        for (int i = 0; i < CondInfo.condIndices.size(); i++)
        {
            
             WRITE_UINT8(baseAddr+USE_CONDITIONAL_IDX_REG+i, (ForLoopIDToMapping(CondInfo.condIndices[i]->getName()) - hwStat->nConstRegisters - hwStat->nConstArray));
        }
        
}

void DTL::ResourceAllocation::DoControlWrites(uint64_t baseaddr)
{

    for (int i = 0; i < OutStatementRouting.size(); i++)
    {
        auto& outstmt = OutStatementRouting[i];
        outstmt->DoControlWrites(baseaddr, i);
        hwStat->VarOutMap.clear(); // I believe we can clear this since each out statement will use different resources
        hwStat->IdxOutMap.clear();
       // printf("her!!e\n");
    }
}

void DTL::ResourceAllocation::PrintControlWrites(const std::string &file,uint64_t baseaddr)
{
    std::ofstream outfile(file);  // open for writing
    if (!outfile.is_open()) {
        std::cerr << "Failed to open file.\n";
        return;
    }
    std::string out;
    for (int i = 0; i < OutStatementRouting.size(); i++)
    {
        auto& outstmt = OutStatementRouting[i];
       // printf("here\n");
        out += outstmt->PrintControlWrites(baseaddr, i);
        hwStat->VarOutMap.clear(); // I believe we can clear this since each out statement will use different resources
        hwStat->IdxOutMap.clear();
        //printf("her!!e\n");
    }
    outfile << out;
    outfile.close();
    /*
        We still need to set up a way to write for loop and constant registers
    */
}

bool DTL::ResourceAllocation::BindArrayIndex(int arrayReg, int ForLoopReg) {
        auto it = ArrayIndexBinding.find(arrayReg);
        if (it != ArrayIndexBinding.end() && it->second != ForLoopReg)
        {
            printf("Error. Array is already bound to index.\n");
            return false;
        }


        // we set the value here. We should be fine with for loop here
        // as the number of constArrayRegisters will be 1-2
        for (auto& constArray: constArrayRegisters)
        {
            //printf("ConstArray %d attempt bind %d to %d\n",constArray.reg_num, arrayReg, ForLoopReg);
            if (constArray.reg_num == arrayReg)
            {
                constArray.index_reg_num = ForLoopReg;

                
            }
        }

        ArrayIndexBinding[arrayReg] = ForLoopReg;
        return true;
    }

void DTL::ResourceAllocation::UpdateMetadataStreamInfo(const std::unordered_map<std::string, uint64_t> &metadata_args)
{
    for (auto& mminfo : MetadataStreamMMIOInfo)
    {
        if (metadata_args.find(mminfo.varID) != metadata_args.end())
        {
            mminfo.streamPhysStart = metadata_args.at(mminfo.varID);
        }
    }
}

void DTL::AGUHardwareStat::DoForLoopWrite(uint64_t baseAddress, LoopReg &reg, uint32_t byte_width) 
{
    uint64_t addr = baseAddress + GetLoopRegsOffset() + reg.reg_num*byte_width;
    //printf("LoopRegsOffset 0x%x\n", GetLoopRegsOffset());
    //printf("DoForLoopWrite()1 0x%x, 0x%x\n", baseAddress, GetLoopRegsOffset() + reg.reg_num*byte_width);
    uint32_t write_value_ = static_cast<uint32_t>(reg.init_value);
    WRITE_UINT32(addr, write_value_);

    addr = baseAddress + GetLoopIncRegsOffset(byte_width) + reg.reg_num*byte_width; // these should align
    write_value_ = static_cast<uint32_t>(reg.increment_condition);
   //  printf("DoForLoopWrite()1 0x%x, 0x%x\n", baseAddress, GetLoopIncRegsOffset(byte_width) + reg.reg_num*byte_width);
    WRITE_UINT32(addr, write_value_);

    /*
        We now can write the magic values
        These are implemented backwards in the hardware to facilitate the unroll unit
    */
    addr = baseAddress + GetMagicRegsOffset(byte_width) + ((nForLoopRegisters-1-reg.reg_num)*bytesMagic);
   // printf("Start MagicReg 0x%x, 0x%x\n", GetMagicRegsOffset(byte_width), GetMagicRegsOffset(byte_width) + ((nForLoopRegisters-1-reg.reg_num)*bytesMagic));
    auto write_value64 = static_cast<uint64_t>(reg.hwDivMagic.M);
    WRITE_UINT64(addr, write_value64);
    addr += 0x8;
    write_value_ = static_cast<uint32_t>(reg.hwDivMagic.s);
    WRITE_UINT32(addr, write_value_);
    addr += 0x4;
    uint8_t write_val8 = static_cast<uint8_t>(reg.hwDivMagic.add_indicator); // this will be interpreted as boolean value in hardware
    WRITE_UINT8(addr, write_val8);
}


std::string DTL::AGUHardwareStat::PrintForLoopWrite(uint64_t baseAddress,
                                                    LoopReg &reg,
                                                    uint32_t byte_width) {
  uint64_t addr_ = baseAddress + GetLoopRegsOffset() + reg.reg_num * byte_width;
  std::string addr = to_hex(addr_);
  uint64_t write_value_ = reg.init_value;
  std::string write_value = to_hex(write_value_);

  // ret to loop register
  std::string ret = "\nWRITE_UINT32(" + addr + "," + write_value + ");\n";
  addr_ = baseAddress + GetLoopIncRegsOffset(byte_width) +
          reg.reg_num * byte_width; // these should align
  addr = to_hex(addr_);
  write_value = to_hex(static_cast<uint64_t>(reg.increment_condition));
  ret += "\nWRITE_UINT32(" + addr + "," + write_value + ");\n";
  //  printf("LoopRegsOffset 0x%x\n", GetLoopRegsOffset());
  /*
      We now can write the magic values
      These are implemented backwards in the hardware to facilitate the unroll
     unit
  */
  addr_ = baseAddress + GetMagicRegsOffset(byte_width) +
          ((nForLoopRegisters - 1 - reg.reg_num) * bytesMagic);
  addr = to_hex(addr_);
  write_value = to_hex(static_cast<uint64_t>(reg.hwDivMagic.M));
  ret += "\nWRITE_UINT64(" + addr + "," + write_value + ");\n";

  addr_ += 0x8;
  addr = to_hex(addr_);
  write_value = to_hex(static_cast<uint64_t>(reg.hwDivMagic.s));
  ret += "\nWRITE_UINT32(" + addr + "," + write_value + ");\n";

  addr_ += 0x4;
  addr = to_hex(addr_);
  uint8_t write_val8 = static_cast<uint8_t>(
      reg.hwDivMagic.add_indicator); // this will be interpreted as boolean
                                     // value in hardware
  write_value = to_hex(write_val8);
  ret += "\nWRITE_UINT8(" + addr + "," + write_value + ");\n";
  return ret;
}



void DTL::MetadataStreamDeclNode::resourceAllocation(ResourceAllocation *ralloc, int depth)
{
   // printf("HERE\n");
    ralloc->MapMetadataStream(myID->getName(), this);
}

