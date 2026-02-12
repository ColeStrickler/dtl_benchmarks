#include "benchmarks.hpp"




/*
    Function to parameterize the DTL configuration
*/
std::string benchmark::InsertDTLConfigParameters(const BenchmarkData& benchmark_data)
{
    auto dtl_parameterized_config = dtl_stencil_configs[benchmark_data.name];
    auto& params = benchmark_data.params;

    for (auto& [key, value] : params) {
        printf("%s,%d\n", key.c_str(), value);
        std::string token = "[" + key + "]";
        size_t pos;
        while ((pos = dtl_parameterized_config.find(token)) != std::string::npos)
            dtl_parameterized_config.replace(pos, token.size(), std::to_string(value));
    }
    //printf("parametrized config %s\n", dtl_parameterized_config.c_str());
    return dtl_parameterized_config;
}

std::string benchmark::CreateConstants(const std::unordered_map<std::string, BenchParam> &constants) {
    std::string ret;

    for (auto& e: constants)
    {
        printf("const %s\n", vec2String(e.second));
        std::string constDecl = "int " + e.first + " = ";
        assert(e.second.size() >= 1);
        if (e.second.size() > 1) // an array
        {
            constDecl += "{";
            for (auto& x: e.second)
                constDecl += std::to_string(x) + ",";
            constDecl.pop_back(); // remove last comma;
            constDecl += "}";
        }
        else
        {
            constDecl += std::to_string(e.second[0]);
        }
        constDecl += ";\n";
        ret += constDecl;
    }

    return ret;
}

std::string benchmark::CreateBenchmarkConfig(const BenchmarkData &benchmark_data)
{
    std::string constants = CreateConstants(benchmark_data.constants);
    std::string config_body = InsertDTLConfigParameters(benchmark_data);

    return constants + "\n"+ config_body;
}

std::string benchmark::CreateBenchmarkConfig2(BenchmarkData benchmark_data)
{
    std::string constants = CreateConstants(benchmark_data.constants);
    std::string config_body = InsertDTLConfigParameters(benchmark_data);

    return constants + "\n"+ config_body;
}

std::string benchmark::bench_wrapper_db_filterselect(const BenchmarkData & bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;

    std::string oneEphemeralTableConf;
    std::string splitEphemeralTableConf_sel;
    std::string splitEphemeralTableConf_filter;
    assert(bench_data.constants.find("row_size") != bench_data.constants.end());
    assert(bench_data.constants.find("filter_col_offsets") != bench_data.constants.end());
    assert(bench_data.constants.find("selection_col_offsets") != bench_data.constants.end());
    assert(bench_data.params.find("ROWS") != bench_data.params.end());
    assert(bench_data.params.find("COLUMNS") != bench_data.params.end());

    uint32_t rows = bench_data.params.at("ROWS");
    uint32_t cols =  bench_data.params.at("COLUMNS"); // this is currently just the # of selection columns
    uint32_t row_size = bench_data.constants.at("row_size")[0];
    uint32_t col_sum_width_combined = sizeof(uint32_t)*(bench_data.constants.at("selection_col_offsets").size() + bench_data.constants.at("filter_col_offsets").size() - 1);
    uint32_t col_filter_width_split = sizeof(uint32_t)*bench_data.constants.at("filter_col_offsets").size();
    uint32_t col_sel_width_split = sizeof(uint32_t)*bench_data.constants.at("selection_col_offsets").size();
    uint32_t col_width = 4;

    BenchmarkData oneTableData;
    BenchmarkData splitTableData_sel;
    BenchmarkData splitTableData_filter;

    // combine filter and selection columns
    std::vector<uint32_t> combined;
    combined.insert(combined.end(), bench_data.constants.at("filter_col_offsets").begin(), bench_data.constants.at("filter_col_offsets").end());

    combined.insert(combined.end(), bench_data.constants.at("selection_col_offsets").begin(), bench_data.constants.at("selection_col_offsets").end());
    auto rit = std::find(combined.rbegin(), combined.rend(), 36); // no need for duplicate column
    if (rit != combined.rend())
    {
        combined.erase(std::next(rit).base());
    }

    //std::sort(combined.begin(), combined.end()); // not needed if we want them grouped contiguously



    oneTableData.name = "db_col_project";
    oneTableData.params = bench_data.params;
    oneTableData.params["COLUMNS"] = combined.size();
    oneTableData.constants.insert({"row_size", bench_data.constants.at("row_size")});
    oneTableData.constants.insert({"col_offsets", combined});

    splitTableData_filter = bench_data;
    splitTableData_filter.name = "db_col_project";
    splitTableData_filter.constants.erase("selection_col_offsets");
    splitTableData_filter.constants.insert({"col_offsets", splitTableData_filter.constants.at("filter_col_offsets")});
    splitTableData_filter.constants.erase("filter_col_offsets");

    splitTableData_sel = bench_data;
    splitTableData_sel.name = "db_col_project";
    splitTableData_sel.constants.erase("filter_col_offsets");
    splitTableData_sel.constants.insert({"col_offsets", splitTableData_sel.constants.at("selection_col_offsets")});
    splitTableData_sel.constants.erase("selection_col_offsets");

    oneEphemeralTableConf = CreateBenchmarkConfig(oneTableData);
    std::cout << oneEphemeralTableConf << "\n\n\n";
    splitEphemeralTableConf_filter = CreateBenchmarkConfig(splitTableData_filter);
    std::cout << splitEphemeralTableConf_filter << "\n\n\n";
    splitEphemeralTableConf_sel = CreateBenchmarkConfig(splitTableData_sel);
    std::cout << splitEphemeralTableConf_sel << "\n\n\n";
    DTL::EphemeralRegion* ephemeral_splitFilter = api->AllocEphemeralRegion(rows*row_size);
    if (!api->Compile(splitEphemeralTableConf_filter)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral_splitFilter);
    
    


    DTL::EphemeralRegion* ephemeral_splitSelect = api->CloneEphemeralRegion(ephemeral_splitFilter);
    if (!api->Compile(splitEphemeralTableConf_sel)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral_splitSelect);

    DTL::EphemeralRegion* ephemeral_oneTable = api->AllocEphemeralRegion(rows*row_size);
    printf("ephemeral alloced\n");
    if (!api->Compile(oneEphemeralTableConf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral_oneTable);


    printf("Cloning region\n");

    int* split_FilterReadRegion = (int*)ephemeral_splitFilter->GetHeadlessReadRegion();
    int* split_FilterWriteRegion = (int*)ephemeral_splitFilter->GetHeadlessWriteregion();
    int* split_SelectReadRegion = (int*)ephemeral_splitSelect->GetHeadlessReadRegion();
    int* split_SelectWriteRegion = (int*)ephemeral_splitSelect->GetHeadlessWriteregion();
    
    printf("programmed\n");
    int* Aw = (int*)ephemeral_oneTable->GetHeadlessWriteregion();
    int* db1 = (int*)ephemeral_oneTable->GetHeadlessReadRegion();
    int* write_out1 = new int[rows*cols]; // maximum number written out
    int* write_out2 = new int[rows*cols]; // maximum number written out
    printf("writing region\n");
    randomize_region_deterministic_int(Aw, 16*rows);
    randomize_region_deterministic_int(split_FilterWriteRegion, 16*rows);
    printf("done writing region\n");

    std::string bench_info_combined = "db_filterselect_combined_" + vec2String(bench_data.constants.at("filter_col_offsets"))\
        + "_" + vec2String(bench_data.constants.at("selection_col_offsets"));
    std::string bench_info_split = "db_filterselect_split_" + vec2String(bench_data.constants.at("filter_col_offsets"))\
        + "_" + vec2String(bench_data.constants.at("selection_col_offsets"));
    


    /*
        Inside benchmarks.hpp, we assume the column offsets are sorted.
        
        When we combine them, we leave them apart. Thus we have
        <filter1, filter2, ... filterN>, <sel1, sel2, ... selN>
        <+1 col_width, +2 col_width, ...> .... 
    */
    printf("running bench\n");
    perf.CollectCounters();
    int x = 0;
    for (int i = 0; i < rows; i++)
    {
        
        int c5 =    READ_INT32(reinterpret_cast<uint8_t*>(db1) + i*col_sum_width_combined);
        int c6 =    READ_INT32(reinterpret_cast<uint8_t*>(db1) + i*col_sum_width_combined + col_width);
        int c10 =   READ_INT32(reinterpret_cast<uint8_t*>(db1) + i*col_sum_width_combined + 2*col_width);
        int c14 =   READ_INT32(reinterpret_cast<uint8_t*>(db1) + i*col_sum_width_combined + 3*col_width);

        // TEMPORARY

        //int Xc5 =    READ_INT32(reinterpret_cast<uint8_t*>(split_FilterReadRegion) + i*col_filter_width_split);
        //int Xc6 =    READ_INT32(reinterpret_cast<uint8_t*>(split_FilterReadRegion) + i*col_filter_width_split + col_width);
        //int Xc10 =   READ_INT32(reinterpret_cast<uint8_t*>(split_FilterReadRegion) + i*col_filter_width_split + 2*col_width);
        //int Xc14 =   READ_INT32(reinterpret_cast<uint8_t*>(split_FilterReadRegion) + i*col_filter_width_split + 3*col_width);
        //if (Xc5 != c5 || Xc6 != c6 || Xc10 != c10 || Xc14 != c14)
        //{
        //    printf("%d\n", i);
        //}
        //assert(Xc5 == c5);
        //assert(Xc6 == c6);
        //assert(Xc10 == c10);
        //assert(Xc14 == c14);

        
        if (c5 < 256 && c6 > 64 && c10%2 == 0 && c14 > 128)
        {
            write_out1[x++] = c10;
            for (int j = 0; j < cols-1; j++)
            {
                write_out1[x++] = READ_INT32(reinterpret_cast<uint8_t*>(db1) + i*col_sum_width_combined + (j+4)*col_width);
            }
        }   
    }
    perf.CollectDelta();
    printf("finished bench\n");
    results += bench_info_combined + ",dtu," + perf.PrintCounters() + "," + print_checksum_i32(write_out1, x) + "\n";
    





    perf.ClearCounters();
    /*
        We should be able to use either write region, and it should propagate
    */


    //for (int i = 0; i < rows*16; i++)
    //{
    //    assert(split_FilterWriteRegion[i] == Aw[i]);
    //}

    
    perf.CollectCounters();
    int x2 = 0;
    for (int i = 0; i < rows; i++)
    {
        int c5 =    READ_INT32(reinterpret_cast<uint8_t*>(split_FilterReadRegion) + i*col_filter_width_split);
        int c6 =    READ_INT32(reinterpret_cast<uint8_t*>(split_FilterReadRegion) + i*col_filter_width_split + col_width);
        int c10 =   READ_INT32(reinterpret_cast<uint8_t*>(split_FilterReadRegion) + i*col_filter_width_split + 2*col_width);
        int c14 =   READ_INT32(reinterpret_cast<uint8_t*>(split_FilterReadRegion) + i*col_filter_width_split + 3*col_width);

        if (c5 < 256 && c6 > 64 && c10%2 == 0 && c14 > 128)
        {
            for (int j = 0; j < cols; j++)
            {
                if (j == 2)
                    write_out2[x2++] = c10;
                else
                    write_out2[x2++] = READ_INT32(reinterpret_cast<uint8_t*>(split_SelectReadRegion) + i*col_sel_width_split + j*col_width);
            }
        }
    }
    perf.CollectDelta();
    results += bench_info_split + ",dtu," + perf.PrintCounters() + "," + print_checksum_i32(write_out2, x2) + "\n";
    printf("x=%d, x2=%d\n", x, x2);
    assert(x2 == x);
    api->FreeEphemeralRegion(ephemeral_splitFilter);
    api->FreeEphemeralRegion(ephemeral_splitSelect);
    api->FreeEphemeralRegion(ephemeral_oneTable);
    delete[] write_out1;
    delete[] write_out2;
    return results;
}

std::string benchmark::bench_wrapper_db_filterselect5(const BenchmarkData &bench_data, DTL::API *api) 
{
#define C0 0
#define C1 4
#define C2 8
#define C3 12
#define C4 16
#define C5 20
#define C6 24
#define C7 28
#define C8 32
#define C9 36
#define C10 40
#define C11 44
#define C12 48
#define C13 52
#define C14 56
#define C15 60

    std::string results;
    PerfManager perf;



    std::string oneEphemeralTableConf; // c3,c8,c10,c15 | c5,c6,C10,c15
    std::string splitEphemeralTableConf_sel; // c5,c6,C10,c15
    std::string splitEphemeralTableConf_filter1; // c3
    std::string splitEphemeralTableConf_filter2; // c8
    std::string splitEphemeralTableConf_filter3; // c10
    std::string splitEphemeralTableConf_filter4; // c15
    assert(bench_data.constants.find("row_size") != bench_data.constants.end());
    assert(bench_data.params.find("ROWS") != bench_data.params.end());
    assert(bench_data.params.find("COLUMNS") != bench_data.params.end());
    assert(bench_data.constants.find("filter_col_offsets") != bench_data.constants.end());
    assert(bench_data.constants.find("selection_col_offsets") != bench_data.constants.end());

    
    BenchmarkData bench_data_copy =  bench_data;
    bench_data_copy.constants.erase("filter_col_offsets");
    bench_data_copy.constants.erase("selection_col_offsets");
    std::vector<uint32_t> oneTableCols = {C3, C8, C10, C15, C5, C6, C10, C14};
    std::vector<uint32_t> split_selCols = {C5, C6, C10, C14};
    std::vector<uint32_t> split_filterCols1 = {C3};
    std::vector<uint32_t> split_filterCols2 = {C8};
    std::vector<uint32_t> split_filterCols3 = {C10};
    std::vector<uint32_t> split_filterCols4 = {C15};



    std::string bench_info_combined = "db_filterselect_combined_" + vec2String(oneTableCols)\
        + "__" + vec2String(split_selCols);
    std::string bench_info_split = "db_filterselect_split5table_" + vec2String(oneTableCols)\
        + "__" + vec2String(split_selCols);
    






    BenchmarkData oneTableData              = bench_data_copy;
    BenchmarkData splitTableData_sel        = bench_data_copy;
    BenchmarkData splitTableData_filter1    = bench_data_copy;
    BenchmarkData splitTableData_filter2    = bench_data_copy;
    BenchmarkData splitTableData_filter3    = bench_data_copy;
    BenchmarkData splitTableData_filter4    = bench_data_copy;

    oneTableData.constants.insert({"col_offsets", oneTableCols});
    splitTableData_sel.constants.insert({"col_offsets", split_selCols});
    splitTableData_filter1.constants.insert({"col_offsets", split_filterCols1});
    splitTableData_filter2.constants.insert({"col_offsets", split_filterCols2});
    splitTableData_filter3.constants.insert({"col_offsets", split_filterCols3});
    splitTableData_filter4.constants.insert({"col_offsets", split_filterCols4});
    oneTableData.params.insert({"COLUMNS", oneTableData.constants.at("col_offsets").size()});
    splitTableData_sel.params.insert({"COLUMNS", splitTableData_sel.constants.at("col_offsets").size()});
    splitTableData_filter1.params.insert({"COLUMNS", splitTableData_filter1.constants.at("col_offsets").size()});
    splitTableData_filter2.params.insert({"COLUMNS", splitTableData_filter2.constants.at("col_offsets").size()});
    splitTableData_filter3.params.insert({"COLUMNS", splitTableData_filter3.constants.at("col_offsets").size()});
    splitTableData_filter4.params.insert({"COLUMNS", splitTableData_filter4.constants.at("col_offsets").size()});
    uint32_t rows = bench_data_copy.params.at("ROWS");
    uint32_t row_size = bench_data_copy.constants.at("row_size")[0];
    std::string oneTableConf;
    std::string splitSelConf;
    std::string splitFilterConf1;
    std::string splitFilterConf2;
    std::string splitFilterConf3;
    std::string splitFilterConf4;

    oneTableConf = CreateBenchmarkConfig(oneTableData);
    splitSelConf = CreateBenchmarkConfig(splitTableData_sel);
    splitFilterConf1 = CreateBenchmarkConfig(splitTableData_filter1);
    splitFilterConf2 = CreateBenchmarkConfig(splitTableData_filter2);
    splitFilterConf3 = CreateBenchmarkConfig(splitTableData_filter3);
    splitFilterConf4 = CreateBenchmarkConfig(splitTableData_filter4);

    DTL::EphemeralRegion* oneTableRegion = api->AllocEphemeralRegion(rows*64);
    DTL::EphemeralRegion* split_SelRegion = api->CloneEphemeralRegion(oneTableRegion);
    DTL::EphemeralRegion* split_FilterRegion1 = api->CloneEphemeralRegion(oneTableRegion);
    DTL::EphemeralRegion* split_FilterRegion2 = api->CloneEphemeralRegion(oneTableRegion);
    DTL::EphemeralRegion* split_FilterRegion3 = api->CloneEphemeralRegion(oneTableRegion);
    DTL::EphemeralRegion* split_FilterRegion4 = api->CloneEphemeralRegion(oneTableRegion);


    if (!api->Compile(oneTableConf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(oneTableRegion);
    
        if (!api->Compile(splitSelConf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(split_SelRegion);
    

        if (!api->Compile(splitFilterConf1)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(split_FilterRegion1);
    
        if (!api->Compile(splitFilterConf2)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(split_FilterRegion2);
    
        if (!api->Compile(splitFilterConf3)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(split_FilterRegion3);
    
    if (!api->Compile(splitFilterConf4)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(split_FilterRegion4);
    
    
    

    int* oneTableRead   = (int*)oneTableRegion->GetHeadlessReadRegion();
    int* oneTableWrite  = (int*)oneTableRegion->GetHeadlessReadRegion();
    int* split_SelRead = (int*)split_SelRegion->GetHeadlessReadRegion();
    int* split_FilterRead1 = (int*)split_FilterRegion1->GetHeadlessReadRegion();
    int* split_FilterRead2 = (int*)split_FilterRegion2->GetHeadlessReadRegion();
    int* split_FilterRead3 = (int*)split_FilterRegion3->GetHeadlessReadRegion();
    int* split_FilterRead4 = (int*)split_FilterRegion4->GetHeadlessReadRegion();

    // We just need to intiate data at the base region, since they all point to the same table
    randomize_region_deterministic_int(oneTableWrite, 16*rows);


    int* write_out1 = new int[split_selCols.size()*rows];
    int* write_out2 = new int[split_selCols.size()*rows];


    uint32_t col_sum_width_combined1 = oneTableCols.size()*sizeof(int);
    int x = 0;
    int cols = oneTableCols.size();
    int col_width = 4;
    perf.CollectCounters();
    for (int i = 0; i < rows; i++)
    {
        
        int c3 =    READ_INT32(reinterpret_cast<uint8_t*>(oneTableRead) + i*col_sum_width_combined1);
        int c8 =    READ_INT32(reinterpret_cast<uint8_t*>(oneTableRead) + i*col_sum_width_combined1 + col_width);
        int c10 =   READ_INT32(reinterpret_cast<uint8_t*>(oneTableRead) + i*col_sum_width_combined1 + 2*col_width);
        int c15 =   READ_INT32(reinterpret_cast<uint8_t*>(oneTableRead) + i*col_sum_width_combined1 + 3*col_width);


        if (c3 < 256 && c8 > 64 && c10%2 == 0 && c15 > 128)
        {          
            for (int j = 0; j < cols; j++)
            {
                if (j == 2)
                    write_out1[x++] = c10;
                else
                    write_out1[x++] = READ_INT32(reinterpret_cast<uint8_t*>(oneTableRead) + i*col_sum_width_combined1 + (j+4)*col_width);
            }
             
        }   
    }
    perf.CollectDelta();
    results += bench_info_combined + ",dtu," + perf.PrintCounters() + "," + print_checksum_i32(write_out1, x) + "\n";




    
    perf.ClearCounters();
    uint32_t col_sum_width_combined2 = split_selCols.size()*sizeof(int);
    int x2 = 0;
    int cols2 = oneTableCols.size();


    perf.CollectCounters();
    for (int i = 0; i < rows; i++)
    {
        
        int c3 =    READ_INT32(reinterpret_cast<uint8_t*>(split_FilterRegion1) + i*col_width);
        int c8 =    READ_INT32(reinterpret_cast<uint8_t*>(split_FilterRegion2) + i*col_width);
        int c10 =   READ_INT32(reinterpret_cast<uint8_t*>(split_FilterRegion3) + i*col_width);
        int c15 =   READ_INT32(reinterpret_cast<uint8_t*>(split_FilterRegion4) + i*col_width);


        if (c3 < 256 && c8 > 64 && c10%2 == 0 && c15 > 128)
        {          
            for (int j = 0; j < cols; j++)
            {
                if (j == 2)
                    write_out2[x2++] = c10;
                else
                    write_out2[x2++] = READ_INT32(reinterpret_cast<uint8_t*>(split_SelRegion) + i*col_sum_width_combined2 + j*col_width);
            }
             
        }   
    }
    perf.CollectDelta();
    results += bench_info_split + ",dtu," + perf.PrintCounters() + "," + print_checksum_i32(write_out1, x) + "\n";



    delete[] write_out1;
    delete[] write_out2;
    api->FreeEphemeralRegion(oneTableRegion);
    api->FreeEphemeralRegion(split_SelRegion);
    api->FreeEphemeralRegion(split_FilterRegion1);
    api->FreeEphemeralRegion(split_FilterRegion2);
    api->FreeEphemeralRegion(split_FilterRegion3);
    api->FreeEphemeralRegion(split_FilterRegion4);


    return results;
#undef C0
#undef C1
#undef C2
#undef C3
#undef C4
#undef C5
#undef C6
#undef C7
#undef C8
#undef C9
#undef C10
#undef C11
#undef C12
#undef C13
#undef C14
#undef C15
}

std::string benchmark::bench_wrapper_im2col(const BenchmarkData &bench_data,
                                            DTL::API *api) {
  printf("here\n");
  std::string results;

  PerfManager perf;
  std::string conf = CreateBenchmarkConfig(bench_data);
  std::string img_info = std::to_string(bench_data.constants.at("height")[0]) +
                         "_" +
                         std::to_string(bench_data.constants.at("width")[0]) +
                         "_" + std::to_string(bench_data.params.at("CHANNELS"));

  printf("here2\n");

  assert(bench_data.params.find("CHANNELS") != bench_data.params.end());
  assert(bench_data.other.find("CHANNELS_OUT") != bench_data.other.end());
  assert(bench_data.params.find("KW") != bench_data.params.end());
  assert(bench_data.constants.find("height") != bench_data.constants.end());
  assert(bench_data.constants.find("width") != bench_data.constants.end());

  int channels_in = bench_data.params.at("CHANNELS");
  int channels_out = bench_data.other.at("CHANNELS_OUT");
  int ksize = bench_data.params.at("KW");
  int height = bench_data.constants.at("height")[0];
  int width = bench_data.constants.at("width")[0];
  int stride = 1;
  int pad = 0;

  int out_height = (height + 2 * pad - ksize) / stride + 1;
  int out_width = (width + 2 * pad - ksize) / stride + 1;

  int M = channels_out;
  int K = channels_in * ksize * ksize;
  int N = out_height * out_width;

  uint32_t in_data_size = height * width * channels_in;
  uint32_t out_data_size = out_height * out_width * channels_out;
  uint32_t im2col_matsize =
      channels_in * (height - ksize + 1) * (width - ksize + 1) * ksize * ksize;
  unsigned int buf_size = 3 * 1078 * 3 * 3 * 1918;

  float *im2col_cpu_matrix = new float[im2col_matsize];
  float *in_data = new float[in_data_size];
  float *filter_matrix =
      new float[channels_out * channels_in * ksize *
                ksize]; // each C_Out applies a ksize*ksize filter to each C_In

  float *outbuf = new float[out_height * out_width * channels_out];
  float *outbuf2 = new float[out_height * out_width * channels_out];
  // setup input data
  randomize_region_deterministic(reinterpret_cast<uint8_t *>(in_data),
                                 in_data_size * sizeof(float));
  randomize_region_deterministic(reinterpret_cast<uint8_t *>(filter_matrix),
                                 channels_out * channels_in * ksize * ksize *
                                     sizeof(float));

  // CPU benchmark
  perf.CollectCounters();
  im2col_cpu(in_data,
             channels_in,
             height,
             width,
             ksize,
             stride,
             pad,
             im2col_cpu_matrix);
  matmult_conv_blocked(filter_matrix, im2col_cpu_matrix, outbuf, M, K, N);
  perf.CollectDelta();
  results +=
      "im2col_" + img_info + "," + "cpu," + perf.PrintCounters() + "," +
      std::to_string(print_checksum_l(im2col_cpu_matrix, im2col_matsize)) +
      "\n";
  printf("done cpu\n");
  if (bench_data.output_artifact.size())
    dump_buffer_to_disk("./artifacts/im2col_cpu_" + img_info,
                        reinterpret_cast<unsigned char *>(outbuf),
                        out_height * out_width * channels_out * sizeof(float));
  // CPU benchmark

  perf.ClearCounters();
  DTL::EphemeralRegion *ephemeral =
      api->AllocEphemeralRegion(im2col_matsize * sizeof(float));

  if (!api->Compile(conf)) {
    printf("Failed to compile dtl program or map onto agu\n");
    return "Failed to compile dtl program or map onto agu\n";
  }
  api->ProgramHardware(ephemeral);
  float *Aw = (float *)ephemeral->GetHeadlessWriteregion();
  float *Ar = (float *)ephemeral->GetHeadlessReadRegion();
  randomize_region_deterministic(reinterpret_cast<uint8_t *>(Aw),
                                 in_data_size * sizeof(float));

  // DTU Benchmark
  perf.CollectCounters();
  matmult_conv_blocked(filter_matrix, Ar, outbuf2, M, K, N);
  perf.CollectDelta();

  results += "im2col_" + img_info + "," + "dtu," + perf.PrintCounters() + "," +
             std::to_string(print_checksum_l(Ar, im2col_matsize)) + "\n";
  if (bench_data.output_artifact.size())
    dump_buffer_to_disk("./artifacts/im2col_dtu_" + img_info,
                        reinterpret_cast<unsigned char *>(outbuf2),
                        out_height * out_width * channels_out * sizeof(float));

  // DTU Benchmark

  // perhaps wrap these in smart pointers
  delete[] in_data;
  delete[] outbuf;
  delete[] outbuf2;
  delete[] im2col_cpu_matrix;
  delete[] filter_matrix;
  api->FreeEphemeralRegion(ephemeral);

  return results;
}

std::string benchmark::bench_wrapper_db_colproject(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);


    assert(bench_data.params.find("ROWS") != bench_data.params.end());
    //assert(bench_data.other.find("CHANNELS_OUT") != bench_data.other.end());
    assert(bench_data.params.find("COLUMNS") != bench_data.params.end());
    assert(bench_data.constants.find("row_size") != bench_data.constants.end());
    assert(bench_data.constants.find("col_offsets") != bench_data.constants.end());



    int row_count = bench_data.params.at("ROWS");
    int col_count = bench_data.params.at("COLUMNS");
    int row_size = bench_data.constants.at("row_size")[0]; // row size in bytes
    std::vector<uint32_t> col_offsets = bench_data.constants.at("col_offsets");
    std::string db_info = std::to_string(row_count) + "_" + std::to_string(col_count) + "_" + std::to_string(row_size);


    uint32_t db_size = row_size*row_count; // db size in bytes
    auto db_cpu_raw = new uint8_t[db_size];
    uint32_t* db_cpu = reinterpret_cast<uint32_t*>(db_cpu_raw);
    uint32_t* write_out1 = new uint32_t[col_count*row_count];
    uint32_t* write_out2 = new uint32_t[col_count*row_count];
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(db_cpu), db_size); // write dummy data to database
    
    perf.CollectCounters();
    int out_write_index1 = 0;
    for (int i = 0; i < row_count; i++)
    {
        for (int j = 0; j < col_count; j++)
        {
            write_out1[out_write_index1++] = READ_UINT32(((uint64_t)db_cpu) + i*row_size + col_offsets[j]);
        }
    }
    perf.CollectDelta();
    results += "dbcolproj_" + db_info + "," + "cpu," + perf.PrintCounters() + "," + print_checksum_ul(write_out1, col_count*row_count) + "\n";

    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(db_size);

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    uint32_t* Aw = (uint32_t*)ephemeral->GetHeadlessWriteregion();
    uint32_t* Ar = (uint32_t*)ephemeral->GetHeadlessReadRegion();
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), db_size);

    int sum_col_size = col_count*sizeof(uint32_t);
    perf.CollectCounters();
    int out_write_index2 = 0;
    for (int i = 0; i < row_count; i++)
    {
        for (int j = 0; j < col_count; j++)
        {
            write_out2[out_write_index2++] = READ_UINT32(((uint64_t)Ar) + i*sum_col_size + j*sizeof(uint32_t));
        }
    }
    perf.CollectDelta();
    results += "dbcolproj_" + db_info + "," + "dtu," + perf.PrintCounters()  + "," + print_checksum_ul(write_out2, col_count*row_count) + "\n";



    delete[] db_cpu_raw;
    delete[] write_out1;
    delete[] write_out2;
    api->FreeEphemeralRegion(ephemeral);

    return results;
}

std::string benchmark::bench_wrapper_matmul_transpose(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);




    assert(bench_data.params.find("NROWS") != bench_data.params.end());
    //assert(bench_data.other.find("CHANNELS_OUT") != bench_data.other.end());
    assert(bench_data.params.find("NCOLS") != bench_data.params.end());
    assert(bench_data.constants.find("row_size") != bench_data.constants.end());
    assert(bench_data.constants.find("col_size") != bench_data.constants.end());
    
    int nrows = bench_data.params.at("NROWS");
    int ncols = bench_data.params.at("NCOLS");
    assert(nrows == ncols); // assume square matrices
    std::string mat_info = std::to_string(nrows) + "_" + std::to_string(ncols);

    int* A =  new int[nrows*ncols];
    int* B =  new int[nrows*ncols];
    int* Bt = new int[nrows*ncols];
    int* C1 = new int[nrows*ncols];
    int* C2 = new int[nrows*ncols];
    init_data_int(A, B, C1, nrows);

    perf.CollectCounters();
    transpose_naive_int(B, Bt, nrows, ncols);
    matmult_opt3_pretransposed_int(A, Bt, C1, nrows);
    perf.CollectDelta();
    results += "matmul_transpose_" + mat_info + "," + "cpu," + perf.PrintCounters()  + "," + print_checksum_i32(Bt, nrows*ncols) + "\n";


    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(nrows*ncols*sizeof(int));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    int* Aw = (int*)ephemeral->GetHeadlessWriteregion();
    int* Ar = (int*)ephemeral->GetHeadlessReadRegion();

    init_data_int(A, Aw, C2, nrows);
    perf.CollectCounters();
    matmult_opt3_pretransposed_int(A, Ar, C2, nrows);
    perf.CollectDelta();



    results += "matmul_transpose_" + mat_info + "," + "dtu," + perf.PrintCounters()   + "," + print_checksum_i32(Ar, nrows*ncols) + "\n";




    delete A;
    delete B;
    delete Bt;
    delete C1;
    delete C2;
    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string benchmark::bench_wrapper_nhwc_permutation(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);


    assert(bench_data.params.find("BATCH_SIZE") != bench_data.params.end());
    assert(bench_data.params.find("CHANNELS") != bench_data.params.end());
    assert(bench_data.params.find("SIZE_SQUARED") != bench_data.params.end());
    assert(bench_data.constants.find("size") != bench_data.constants.end());
    assert(bench_data.constants.find("channels") != bench_data.constants.end());
    assert(bench_data.constants.find("data_size") != bench_data.constants.end());
    assert(bench_data.other.find("use_real_image") != bench_data.other.end());
    assert(bench_data.other.find("ksize") != bench_data.other.end());


    int batch_size = bench_data.params.at("BATCH_SIZE");
    int channels = bench_data.constants.at("channels")[0];
    int img_size = bench_data.constants.at("size")[0]; // square this
    int data_size = bench_data.constants.at("data_size")[0];
    int pad = 0;
    bool use_real_image = bench_data.other.at("use_real_image") == 1;
    int ksize = bench_data.other.at("ksize");
    int out_height =    (img_size + 2*pad -  ksize) / 1 + 1;
    int out_width =     (img_size  + 2*pad - ksize) / 1 + 1;

    std::string img_info = std::to_string(img_size) + "x" + std::to_string(img_size)\
         + "_k" + std::to_string(ksize) + "x" + std::to_string(ksize) + "_" + std::to_string(batch_size);

    float* base_img; // nhwc
    float* img_nchw = new float[batch_size*img_size*img_size*channels]; // after permutation
    float* img_out = new float[batch_size*img_size*img_size*channels]; // after convolution
    float* img_out2 = new float[batch_size*img_size*img_size*channels]; // after convolution using dtu
    float* filter = new float[ksize*ksize];
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(filter), ksize*ksize*sizeof(float));

    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        base_img = new float[batch_size*img_size*img_size*channels];
        randomize_region_deterministic(reinterpret_cast<uint8_t*>(base_img), batch_size*img_size*img_size*channels*sizeof(float));
    }
        

    perf.CollectCounters();
    nhwc_to_nchw_cpu(base_img, batch_size, channels, img_size, img_size, img_nchw);
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &img_nchw[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_out[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "nhwc_permutation_" + img_info + "," + "cpu," + perf.PrintCounters() + "," + std::to_string(print_checksum(img_nchw, batch_size*img_size*img_size*channels)) + "\n";


    
    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(batch_size*img_size*img_size*channels*sizeof(float));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    float* Aw = (float*)ephemeral->GetHeadlessWriteregion();
    float* Ar = (float*)ephemeral->GetHeadlessReadRegion();


    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), batch_size*img_size*img_size*channels*sizeof(float));
    }

    perf.CollectCounters();
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &Ar[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_out2[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "nhwc_permutation_" + img_info + "," + "dtu," + perf.PrintCounters() + "," + std::to_string(print_checksum(Ar, batch_size*img_size*img_size*channels)) + "\n";



    delete[] base_img;
    delete[] img_out;
    delete[] img_out2;
    delete[] img_nchw;
    delete[] filter;
    api->FreeEphemeralRegion(ephemeral);

    return results;
}

std::string benchmark::bench_wrapper_batch2space(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);


    assert(bench_data.params.find("BATCH_SIZE") != bench_data.params.end());
    assert(bench_data.params.find("CHANNELS") != bench_data.params.end());
    assert(bench_data.params.find("HEIGHT") != bench_data.params.end());
    assert(bench_data.params.find("WIDTH") != bench_data.params.end());
    assert(bench_data.constants.find("size") != bench_data.constants.end());
    assert(bench_data.constants.find("channels") != bench_data.constants.end());
    assert(bench_data.constants.find("data_size") != bench_data.constants.end());
    assert(bench_data.other.find("use_real_image") != bench_data.other.end());
    assert(bench_data.other.find("ksize") != bench_data.other.end());


    int batch_size = bench_data.params.at("BATCH_SIZE");
    int channels = bench_data.constants.at("channels")[0];
    int img_size = bench_data.constants.at("size")[0]; // square this
    int data_size = bench_data.constants.at("data_size")[0];
    int pad = 0;
    bool use_real_image = bench_data.other.at("use_real_image") == 1;
    int ksize = bench_data.other.at("ksize");
    int out_height =    (img_size + 2*pad -  ksize) / 1 + 1;
    int out_width =     (img_size  + 2*pad - ksize) / 1 + 1;

    std::string img_info = std::to_string(img_size) + "x" + std::to_string(img_size)\
         + "_k" + std::to_string(ksize) + "x" + std::to_string(ksize) + "_" + std::to_string(batch_size);

    float* base_img; // nhwc
    float* img_batch = new float[batch_size*img_size*img_size*channels]; // after permutation
    float* img_batch_transform = new float[batch_size*img_size*img_size*channels]; // after convolution
    float* img_batch_out = new float[batch_size*img_size*img_size*channels]; // after convolution
    float* img_batch_out2 = new float[batch_size*img_size*img_size*channels]; // after convolution using dtu
    float* filter = new float[ksize*ksize];
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(filter), ksize*ksize*sizeof(float));

    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        base_img = new float[batch_size*img_size*img_size*channels];
        randomize_region_deterministic(reinterpret_cast<uint8_t*>(base_img), batch_size*img_size*img_size*channels*sizeof(float));
    }
        

    perf.CollectCounters();
    //nhwc_to_nchw_cpu(base_img, batch_size, channels, img_size, img_size, img_nchw);
    batch_to_space(base_img, batch_size, channels, img_size, img_size, img_batch_transform);
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &img_batch_transform[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_batch_out[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "batch2space_" + img_info + "," + "cpu," + perf.PrintCounters()  + "," + std::to_string(print_checksum(img_batch_transform, batch_size*img_size*img_size*channels)) + "\n";


    
    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(batch_size*img_size*img_size*channels*sizeof(float));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    float* Aw = (float*)ephemeral->GetHeadlessWriteregion();
    float* Ar = (float*)ephemeral->GetHeadlessReadRegion();


    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), batch_size*img_size*img_size*channels*sizeof(float));
    }

    perf.CollectCounters();
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &Ar[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_batch_out2[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "batch2space_" + img_info + "," + "dtu," + perf.PrintCounters() + "," + std::to_string(print_checksum(Ar, batch_size*img_size*img_size*channels)) + "\n";



    delete[] base_img;
    delete[] img_batch;
    delete[] img_batch_out;
    delete[] img_batch_out2;
    delete[] img_batch_transform;
    delete[] filter;
    api->FreeEphemeralRegion(ephemeral);

    return results;
}

std::string benchmark::bench_wrapper_tensorunfold(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);

    assert(bench_data.params.find("D1") != bench_data.params.end());
    assert(bench_data.params.find("D2") != bench_data.params.end());
    assert(bench_data.params.find("D3") != bench_data.params.end());
    assert(bench_data.params.find("D4") != bench_data.params.end());
    assert(bench_data.constants.find("stride_d1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d2") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d3") != bench_data.constants.end());
    assert(bench_data.constants.find("data_size") != bench_data.constants.end());

    int d1 = bench_data.params.at("D1");
    int d2 = bench_data.params.at("D2");
    int d3 = bench_data.params.at("D3");
    int d4 = bench_data.params.at("D4");
    int stride_d1 = bench_data.constants.at("stride_d1")[0];
    int stride_d2 = bench_data.constants.at("stride_d2")[0];
    int stride_d3 = bench_data.constants.at("stride_d3")[0];

    int* tensor_in = new int[d1*d2*d3*d4];
    int* lowered_mat = new int[d1*d2*d3*d4];
    int* tensor_out1 = new int[d1*d2*d3*d4];
    int* tensor_out2 = new int[d1*d2*d3*d4];
    int* matrix_b = new int[d1*d2*d3*d4];
    int* matrix_b2 = new int[d1*d2*d3*d4];


    assert(tensor_in != nullptr);
    assert(lowered_mat != nullptr);
    assert(tensor_out1 != nullptr);
    assert(tensor_out2 != nullptr);
    assert(matrix_b != nullptr);
    assert(matrix_b2 != nullptr);


    randomize_region_deterministic(reinterpret_cast<uint8_t*>(tensor_in), d1*d2*d3*d4*sizeof(int));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(matrix_b), d1*d2*d3*d4*sizeof(int));

    std::string tensor_info = std::to_string(d1) + "x" + std::to_string(d2) + "x" + std::to_string(d3) + "x" + std::to_string(d4);


    perf.CollectCounters();
    lower_mat_4d(lowered_mat, tensor_in, d1, d2, d4, d4, stride_d1, stride_d2, stride_d3);
    hadamard(tensor_out1, lowered_mat, matrix_b, d3, d1*d2*d4);
    perf.CollectDelta();
    results += "tensor_unfold_" + tensor_info + "," + "cpu," + perf.PrintCounters()  + "," + print_checksum_i32(lowered_mat, d1*d2*d3*d4) + "\n";

    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(d1*d2*d3*d4*sizeof(int));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    int* Aw = (int*)ephemeral->GetHeadlessWriteregion();
    int* Ar = (int*)ephemeral->GetHeadlessReadRegion();

    randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), d1*d2*d3*d4*sizeof(int));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(matrix_b2), d1*d2*d3*d4*sizeof(int));


    perf.CollectCounters();
    hadamard(tensor_out2, Ar, matrix_b2, d3, d1*d2*d4);
    perf.CollectDelta();
    results += "tensor_unfold_" + tensor_info + "," + "dtu," + perf.PrintCounters()   + "," + print_checksum_i32(Ar, d1*d2*d3*d4) + "\n";
    printf("%s\n", results.c_str());
    //delete[] tensor_in;
    delete[] lowered_mat; 
    delete[] tensor_out1;
    delete[] tensor_out2;
    delete[] matrix_b;
    delete[] matrix_b2; 
    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string benchmark::bench_wrapper_tensorslice(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);

    assert(bench_data.params.find("D1") != bench_data.params.end());
    assert(bench_data.params.find("D2") != bench_data.params.end());
    assert(bench_data.params.find("D3") != bench_data.params.end());
    assert(bench_data.params.find("D4") != bench_data.params.end());
    assert(bench_data.constants.find("stride_n1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_h1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_w1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_c1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d1") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d2") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_d3") != bench_data.constants.end());
    assert(bench_data.constants.find("data_size") != bench_data.constants.end());
    int d1 = bench_data.params.at("D1");
    int d2 = bench_data.params.at("D2");
    int d3 = bench_data.params.at("D3");
    int d4 = bench_data.params.at("D4");
    int stride_d1 = bench_data.constants.at("stride_d1")[0];
    int stride_d2 = bench_data.constants.at("stride_d2")[0];
    int stride_d3 = bench_data.constants.at("stride_d3")[0];

    int stride_n1 = bench_data.constants.at("stride_n1")[0];
    int stride_h1 = bench_data.constants.at("stride_h1")[0];
    int stride_w1 = bench_data.constants.at("stride_w1")[0];
    int stride_c1 = bench_data.constants.at("stride_c1")[0];

    int n1 = d4 / stride_n1;
    int h1 = d3 / stride_h1;
    int w1 = d2 / stride_w1;
    int c1 = d1 / stride_c1;   


    int* tensor_in = new int[d1*d2*d3*d4];
    int* sliced_tensor = new int[n1*h1*w1*c1];
    int* tensor_out1 = new int[n1*h1*w1*c1];
    int* tensor_out2 = new int[n1*h1*w1*c1];
    int* tensor_b = new int[n1*h1*w1*c1];
    int* tensor_b2 = new int[n1*h1*w1*c1];
    assert(tensor_in != nullptr);
    assert(sliced_tensor != nullptr);
    assert(tensor_out1 != nullptr);
    assert(tensor_out2 != nullptr);
    assert(tensor_b != nullptr);
    assert(tensor_b2 != nullptr);


    randomize_region_deterministic(reinterpret_cast<uint8_t*>(tensor_in), d1*d2*d3*d4*sizeof(int));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(tensor_b), n1*h1*w1*c1*sizeof(int));

    std::string tensor_info = std::to_string(d1) + "x" + std::to_string(d2) + "x" + std::to_string(d3) + "x" + std::to_string(d4);
    tensor_info += "_" + std::to_string(stride_n1) + "x" + std::to_string(stride_h1) + "x" + std::to_string(stride_w1) + "x" + std::to_string(stride_c1);

    perf.CollectCounters();
    slice_tensor_int(sliced_tensor, tensor_in, n1, h1, w1, c1, stride_n1, stride_h1, stride_w1, stride_c1, stride_d1, stride_d2, stride_d3);
    hadamard_tensor_4d_int(tensor_out1, sliced_tensor, tensor_b, n1, h1, w1, c1);
    perf.CollectDelta();
    results += "tensor_slicing_" + tensor_info + "," + "cpu," + perf.PrintCounters() + "," + print_checksum_i32(sliced_tensor, n1*h1*w1*c1) + "\n";

    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(d1*d2*d3*d4*sizeof(int));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    int* Aw = (int*)ephemeral->GetHeadlessWriteregion();
    int* Ar = (int*)ephemeral->GetHeadlessReadRegion();

    randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), d1*d2*d3*d4*sizeof(int));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(tensor_b2), n1*h1*w1*c1*sizeof(int));


    perf.CollectCounters();
    hadamard_tensor_4d_int(tensor_out2, Ar, tensor_b2, n1, h1, w1, c1);
    perf.CollectDelta();
    results += "tensor_slicing_" + tensor_info + "," + "dtu," + perf.PrintCounters() + "," + print_checksum_i32(Ar, n1*h1*w1*c1) + "\n";

    delete[] tensor_in;
    delete[] sliced_tensor; 
    delete[] tensor_out1;     
    delete[] tensor_out2;
    delete[] tensor_b; 
    delete[] tensor_b2; 

    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string benchmark::bench_wrapper_highdim_stencil(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);

    assert(bench_data.params.find("NY_MINUS_1") != bench_data.params.end());
    assert(bench_data.params.find("NX_MINUS_1") != bench_data.params.end());
    assert(bench_data.params.find("NZ_MINUS_1") != bench_data.params.end());
    assert(bench_data.constants.find("stride_nx") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_ny") != bench_data.constants.end());
    assert(bench_data.constants.find("data_size") != bench_data.constants.end());

    int ny = 1 + bench_data.params.at("NY_MINUS_1");
    int nx = 1 + bench_data.params.at("NX_MINUS_1");
    int nz = 1 + bench_data.params.at("NZ_MINUS_1");

    std::string tensor_info = std::to_string(nx) + "x" + std::to_string(ny) + "x" + std::to_string(nz);

    float* base_tensor = new float[ny*nx*nz];
    float* new_tensor = new float[ny*nx*nz];
    float* new_tensor2 = new float[ny*nx*nz];

    randomize_region_deterministic(reinterpret_cast<uint8_t*>(base_tensor), ny*nx*nz*sizeof(float));


    perf.CollectCounters();
    highdim_7stencil_cpu(base_tensor, new_tensor, nx, ny, nz);
    perf.CollectDelta();
    results += "highdim_7stencil_" + tensor_info + "," + "cpu," + perf.PrintCounters() + "," + std::to_string(print_checksum(new_tensor, ny*nx*nz)) + "\n";



    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(8*nx*ny*nz*sizeof(float));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    float* Aw = (float*)ephemeral->GetHeadlessWriteregion();
    float* Ar = (float*)ephemeral->GetHeadlessReadRegion();
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), ny*nx*nz*sizeof(float));

    //printf("dtu start\n");
    perf.CollectCounters();
    highdim_7stencil_dtu(Ar, new_tensor2, nx, ny, nz);
    perf.CollectDelta();
    //printf("done\n");
    results += "highdim_7stencil_" + tensor_info + "," + "dtu," + perf.PrintCounters() + "," + std::to_string(print_checksum(new_tensor2, ny*nx*nz)) + "\n";
    //printf("%s\n", results.c_str());


    delete[] base_tensor;
    delete[] new_tensor;
    delete[] new_tensor2;

    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string benchmark::bench_wrapper_multigrid(const BenchmarkData &bench_data, DTL::API *api) {
  return std::string();
}

std::string benchmark::bench_wrapper_cubestencil(const BenchmarkData &bench_data, DTL::API *api) {
#define CUBE_ACCESS(i, a, x, y, z) (i*ncubes*CUBE_SIZE + a*CUBE_SIZE + x*CUBE_2D + y*CUBE_DIM + z)
#define S_CUBE_ACCESS(x, y, z) (x*CUBE_2D + y*CUBE_DIM + z)
#define TENSOR_COORD4(w, x, y, z) (((w)*d3*d2*d1) + ((x)*d2*d1) + ((y)*d1) + (z))
#define TENSOR_COORD3(x, y, z) (((x)*d2*d1) + ((y)*d1) + (z))
#define CUBE_SIZE (cube_dim*cube_dim*cube_dim)
#define CUBE_2D (cube_dim*cube_dim)
#define CUBE_DIM (cube_dim)

    /*
        This benchmark is a bit more complicated, so we create the config on the fly first
    */

    assert(bench_data.other.find("d4") != bench_data.other.end());
    assert(bench_data.other.find("d3") != bench_data.other.end());
    assert(bench_data.other.find("d2") != bench_data.other.end());
    assert(bench_data.other.find("d1") != bench_data.other.end());
    assert(bench_data.other.find("CUBE_DIM") != bench_data.other.end());
    assert(bench_data.constants.find("data_size")   != bench_data.constants.end());
    assert(bench_data.params.find("N_3DSTRUCT")     != bench_data.params.end());
    assert(bench_data.params.find("NCUBES")         != bench_data.params.end());
    assert(bench_data.params.find("CUBE_DIM1")      != bench_data.params.end());
    assert(bench_data.params.find("CUBE_DIM2")      != bench_data.params.end());
    assert(bench_data.params.find("CUBE_DIM3")      != bench_data.params.end());

    int ncubes = bench_data.params.at("NCUBES");
    assert(ncubes == 8); // all we want to support right now
    int cube_dim = bench_data.other.at("CUBE_DIM");
    int d4 = bench_data.other.at("d4");
    int d3 = bench_data.other.at("d3");
    int d2 = bench_data.other.at("d2");
    int d1 = bench_data.other.at("d1");
    int data_size = bench_data.constants.at("data_size")[0];

    BenchmarkData nBenchData = bench_data;
    nBenchData.constants.insert({"stride_d3", BenchParam{static_cast<uint32_t>(d3*d2*d1)}});
    nBenchData.constants.insert({"stride_d2", BenchParam{static_cast<uint32_t>(d2*d1)}});
    nBenchData.constants.insert({"stride_d1", BenchParam{static_cast<uint32_t>(d1)}});
    
    std::vector<int> cube_locs = {TENSOR_COORD3(0, 0, 0), TENSOR_COORD3(0, d2-CUBE_DIM, 0), TENSOR_COORD3(0, 0, d1-CUBE_DIM),\
                        TENSOR_COORD3(d3-CUBE_DIM, 0, 0), TENSOR_COORD3(d3-CUBE_DIM, d2-CUBE_DIM, 0), TENSOR_COORD3(d3-CUBE_DIM, 0, d1-CUBE_DIM),\
                        TENSOR_COORD3(0, d2-CUBE_DIM, d1-CUBE_DIM), TENSOR_COORD3(d3-CUBE_DIM, d2-CUBE_DIM, d1-CUBE_DIM)};  
    BenchParam cube_loc_param;
    for (auto& loc: cube_locs)
        cube_loc_param.push_back(static_cast<uint32_t>(loc*data_size));
    nBenchData.constants["cube_locs"] = cube_loc_param;

    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig2(nBenchData);
    //printf("conf:\n%s\n", conf.c_str());
    
    std::string tensor_info = std::to_string(d4) + "x" + std::to_string(d3) + "x" + std::to_string(d2)  + "x" + std::to_string(d1);
    
    int* in_tensor = new int[d4*d3*d2*d1];
    int* out_tensor1 = new int[d4*ncubes*CUBE_SIZE];
    int* filter_tensor1 = new int[d4*ncubes*CUBE_SIZE];
    int* filter_tensor2 = new int[d4*ncubes*CUBE_SIZE];
    int* filter = new int[CUBE_SIZE];
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(filter), CUBE_SIZE*sizeof(int));
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(in_tensor), d4*d3*d2*d1*sizeof(int));

    perf.CollectCounters();
    cube_stencil_8corner_cpu(in_tensor, out_tensor1, d4, d3, d2, d1, CUBE_DIM);
    // apply filter to cubes, write to result
    for (int i = 0; i < d4; i++) {
        for (int a = 0; a < ncubes; a++) {
            for (int x = 0; x < CUBE_DIM; x++) {
                for (int y = 0; y < CUBE_DIM; y++) {
                    for (int z = 0; z < CUBE_DIM; z++) {
                        filter_tensor1[CUBE_ACCESS(i, a, x, y, z)] = filter[S_CUBE_ACCESS(x,y,z)] * out_tensor1[CUBE_ACCESS(i, a, x, y, z)];
                    }
                }
            }
        }
    }
    perf.CollectDelta();
    results += "cubestencil_8corner_" + tensor_info + "," + "cpu," + perf.PrintCounters() + "," + print_checksum_i32(filter_tensor1, d4*ncubes*CUBE_SIZE) + "\n";



    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(d1*d2*d3*d4*sizeof(int));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }
    api->ProgramHardware(ephemeral);
    int* Aw = (int*)ephemeral->GetHeadlessWriteregion();
    int* Ar = (int*)ephemeral->GetHeadlessReadRegion();
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(Aw), d1*d2*d3*d4*sizeof(int));

    //printf("dtu start\n");
    perf.CollectCounters();
    // DTU already extracts cubes, we just apply filter and write out
    // apply filter to cubes, write to result
    for (int i = 0; i < d4; i++) {
        for (int a = 0; a < ncubes; a++) {
            for (int x = 0; x < CUBE_DIM; x++) {
                for (int y = 0; y < CUBE_DIM; y++) {
                    for (int z = 0; z < CUBE_DIM; z++) {
                        filter_tensor2[CUBE_ACCESS(i, a, x, y, z)] = filter[S_CUBE_ACCESS(x,y,z)] * Ar[CUBE_ACCESS(i, a, x, y, z)];
                    }
                }
            }
        }
    }

    perf.CollectDelta();
    //printf("done\n");
    results += "cubestencil_8corner_" + tensor_info + "," + "dtu," + "," + print_checksum_i32(Ar, d4*ncubes*CUBE_SIZE) + "\n";

    delete[] in_tensor;
    delete[] filter_tensor1;
    delete[] filter_tensor2;
    delete[] out_tensor1;
    delete[] filter;

#undef TENSOR_COORD4
#undef TENSOR_COORD3
#undef CUBE_SIZE
#undef CUBE_DIM
#undef CUBE_ACCESS
#undef S_CUBE_ACCESS

    api->FreeEphemeralRegion(ephemeral);
    return results;

}
