#include "benchmarks.hpp"




/*
    Function to parameterize the DTL configuration
*/
std::string benchmark::InsertDTLConfigParameters(const BenchmarkData& benchmark_data)
{
    auto dtl_parameterized_config = dtl_stencil_configs[benchmark_data.name];
    auto& params = benchmark_data.params;

    for (auto& [key, value] : params) {
       // printf("%s,%d\n", key.c_str(), value);
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
       // printf("const %s\n", vec2String(e.second));
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

std::string benchmark::CreateConstantArray(const std::unordered_map<std::string, BenchParam> &constantArray) {
    std::string ret;

    for (auto& e: constantArray)
    {
       // printf("const %s\n", vec2String(e.second));
        std::string constDecl = "int " + e.first + " = ";
        assert(e.second.size() >= 1);
        constDecl += "{";
        for (auto& x: e.second)
            constDecl += std::to_string(x) + ",";
        constDecl.pop_back(); // remove last comma;
        constDecl += "}";

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
    std::string constArr = CreateConstantArray(benchmark_data.constArray);
    std::string config_body = InsertDTLConfigParameters(benchmark_data);

    return constants +"\n" +constArr + "\n"+ config_body;
}

std::string benchmark::bench_wrapper_zipStruct(const BenchmarkData &bench_data, DTL::API *api) {
  



}





std::string benchmark::bench_wrapper_ecs(const BenchmarkData &bench_data,
                                         DTL::API *api) {
  std::string results = "";
  PerfManager perf;

  assert(bench_data.constants.find("struct_size") !=
         bench_data.constants.end());
  assert(bench_data.constArray.find("var_offsets") !=
         bench_data.constArray.end());
  assert(bench_data.params.find("NSTRUCT") != bench_data.params.end());
  assert(bench_data.params.find("NOFFSETS") != bench_data.params.end());

  auto struct_size = bench_data.constants.at("struct_size")[0];
  auto var_offsets = bench_data.constArray.at("var_offsets");
  auto nstructs = bench_data.params.at("NSTRUCT");
  auto noffsets = bench_data.params.at("NOFFSETS");
  std::string soa_config = CreateBenchmarkConfig2(bench_data);

  std::cout << soa_config << "\n\n\n" << std::endl;

  /*
      We assume structures of struct_size*sizeof(float)
  */
  std::string ecs_info =
      std::to_string(struct_size) + "_" + vec2String(var_offsets) + "_" +
      std::to_string(nstructs) + "_" + std::to_string(noffsets);

  float *entities_aos = new float[struct_size * nstructs];
  float *entities_soa = new float[struct_size * nstructs];
  randomize_region_deterministic_float(entities_aos, struct_size * nstructs);

  /*
      We initialize first with aos, transform into soa. DTU will access an aos
     base
  */
  for (int n = 0; n < nstructs; n++) {
    for (int j = 0; j < struct_size; j++) // struct members
    {
      entities_soa[j * nstructs + n] = entities_aos[n * struct_size + j];
    }
  }

  printf("converted to soa\n");

  float out_threshold = 0.0f;
  float out_threshold2 = 0.0f;

  /*
      We are comparing to an already locality optimized arrangement
  */
  perf.CollectCounters();
  for (int i = 0; i < nstructs; i++) {
    for (int j = 0; j < noffsets; j++) {
      out_threshold += entities_soa[j * nstructs + i];
    }
  }
  out_threshold /= (nstructs * noffsets);
  perf.CollectDelta();

  results += "ecs_cpu," + std::to_string(0) + "," + perf.PrintCounters() + "," +
             std::to_string(out_threshold) + "\n";

  printf("done CPU\n");
  perf.ClearCounters();

  DTL::EphemeralRegion *base_ephemeral =
      api->AllocEphemeralRegion(struct_size * nstructs * sizeof(float));
  float *base_write = (float *)base_ephemeral->GetHeadlessWriteregion();
  float *base_read = (float *)base_ephemeral->GetHeadlessReadRegion();
  randomize_region_deterministic_float(base_write, struct_size * nstructs);

  if (!api->Compile(soa_config)) {
    printf("Failed to compile dtl program or map onto agu\n");
    return "Failed to compile dtl program or map onto agu\n";
  }

  perf.CollectCounters();
  int x = 0;
  for (int i = 0; i < nstructs * noffsets; i++) {
    out_threshold2 += base_read[x++];
  }

  out_threshold2 /= (nstructs * noffsets);
  perf.CollectDelta();
  results += "ecs_dtu," + std::to_string(0) + "," + perf.PrintCounters() + "," +
             std::to_string(out_threshold2) + "\n";

  printf("done DTU\n");

  perf.ClearCounters();

  delete[] entities_soa;
  delete[] entities_aos;
  api->FreeEphemeralRegion(base_ephemeral);

  return results;
}

std::string
benchmark::bench_wrapper_imageAugmentation(const BenchmarkData &bench_data,
                                           DTL::API *api) {
  std::string results = "";
  PerfManager perf;

  assert(bench_data.params.find("NMAX") != bench_data.params.end());
  assert(bench_data.params.find("CMAX") != bench_data.params.end());
  assert(bench_data.params.find("HMAX") != bench_data.params.end());
  assert(bench_data.params.find("WMAX") != bench_data.params.end());

  assert(bench_data.constants.find("H") != bench_data.constants.end());
  assert(bench_data.constants.find("W") != bench_data.constants.end());
  assert(bench_data.constants.find("stride_n") != bench_data.constants.end());
  assert(bench_data.constants.find("stride_h") != bench_data.constants.end());
  assert(bench_data.constants.find("stride_w") != bench_data.constants.end());

  assert(bench_data.other.find("ksize") != bench_data.other.end());
  assert(bench_data.other.find("use_real_image") != bench_data.other.end());

  auto n = static_cast<int>(bench_data.params.at("NMAX"));
  auto c = static_cast<int>(bench_data.params.at("CMAX"));
  auto h = static_cast<int>(bench_data.params.at("HMAX"));
  auto w = static_cast<int>(bench_data.params.at("WMAX"));
  std::string img_info = std::to_string(n) + "_" + std::to_string(c) + "_" +
                         std::to_string(h) + "_" + std::to_string(w);
  /*
      We instantiate the seven augmentations,
  */

  printf("Got params\n");
  BenchmarkData permute_reflectX_data = bench_data;
  BenchmarkData permute_reflectY_data = bench_data;
  BenchmarkData permute_reflectX_reflectY_data = bench_data;
  BenchmarkData permute_rotateLeft_data = bench_data;
  BenchmarkData permute_rotateLeft_reflectX_data = bench_data;
  BenchmarkData permute_rotateRight_data = bench_data;
  BenchmarkData permute_rotateRight_reflextX_data = bench_data;
  permute_reflectX_data.name = "permute_reflect";
  permute_reflectY_data.name = "permute_flipy";
  permute_reflectX_reflectY_data.name = "permute_flipy_reflect";
  permute_rotateLeft_data.name = "permute_rot_left";
  permute_rotateLeft_reflectX_data.name = "permute_rot_left_reflect";
  permute_rotateRight_data.name = "permute_rot_right";
  permute_rotateRight_reflextX_data.name = "permute_rot_right_reflect";

  std::string permute_reflectX_Conf =
      CreateBenchmarkConfig(permute_reflectX_data);
  std::string permute_reflectY_Conf =
      CreateBenchmarkConfig(permute_reflectY_data);
  std::string permute_reflectX_reflectY_Conf =
      CreateBenchmarkConfig(permute_reflectX_reflectY_data);
  std::string permute_rotateLeft_Conf =
      CreateBenchmarkConfig(permute_rotateLeft_data);
  std::string permute_rotateLeft_reflectX_Conf =
      CreateBenchmarkConfig(permute_rotateLeft_reflectX_data);
  std::string permute_rotateRight_Conf =
      CreateBenchmarkConfig(permute_rotateRight_data);
  std::string permute_rotateRight_reflectX_Conf =
      CreateBenchmarkConfig(permute_rotateRight_reflextX_data);
  printf("Created config\n");

  // Now we assume all of these are compiled ahead of time, and we either can
  // use 7 configs, or swap them mid run
  float *base_img = new float[n * c * h * w];
  float *out_perm_reflectX = new float[n * c * h * w];
  float *out_perm_reflectY = new float[n * c * h * w];
  float *out_perm_reflectX_reflectY = new float[n * c * h * w];
  float *out_perm_rotLeft = new float[n * c * h * w];
  float *out_perm_rotLeft_reflectX = new float[n * c * h * w];
  float *out_perm_rotRight = new float[n * c * h * w];
  float *out_perm_rotRight_reflectX = new float[n * c * h * w];
  float *out2_perm_reflectX = new float[n * c * h * w];
  float *out2_perm_reflectY = new float[n * c * h * w];
  float *out2_perm_reflectX_reflectY = new float[n * c * h * w];
  float *out2_perm_rotLeft = new float[n * c * h * w];
  float *out2_perm_rotLeft_reflectX = new float[n * c * h * w];
  float *out2_perm_rotRight = new float[n * c * h * w];
  float *out2_perm_rotRight_reflectX = new float[n * c * h * w];

  randomize_region_deterministic_float(base_img, n * c * h * w);
  printf("allocated regions\n");

  uint64_t transform_cycles = 0;
  uint64_t start;
  uint64_t end;
  perf.CollectCounters();
  Shape shape_in({n, h, w, c});
  Shape shape_augment({n, c, h, w});
  start = read_cycle();
  permute_reflect_x2D(base_img, out_perm_reflectX, shape_in);
  end = read_cycle();
  transform_cycles += end - start;
  IntensityScaleBatch(
      0.1f, out_perm_reflectX, out2_perm_reflectX, shape_augment);

  start = read_cycle();
  permute_reflect_y2D(base_img, out_perm_reflectY, shape_in);
  end = read_cycle();
  transform_cycles += end - start;
  IntensityScaleBatch(
      0.1f, out_perm_reflectY, out2_perm_reflectY, shape_augment);

  start = read_cycle();
  permute_reflect_y_reflect_x2D(base_img, out_perm_reflectX_reflectY, shape_in);
  end = read_cycle();
  transform_cycles += end - start;
  IntensityScaleBatch(0.1f,
                      out_perm_reflectX_reflectY,
                      out2_perm_reflectX_reflectY,
                      shape_augment);

  start = read_cycle();
  permute_rot_left2D(base_img, out_perm_rotLeft, shape_in);
  end = read_cycle();
  transform_cycles += end - start;
  IntensityScaleBatch(0.1f, out_perm_rotLeft, out2_perm_rotLeft, shape_augment);

  start = read_cycle();
  permute_reflectx_rot_left2D(base_img, out_perm_rotLeft_reflectX, shape_in);
  end = read_cycle();
  transform_cycles += end - start;
  IntensityScaleBatch(0.1f,
                      out_perm_rotLeft_reflectX,
                      out2_perm_rotLeft_reflectX,
                      shape_augment);

  start = read_cycle();
  permute_rot_right2D(base_img, out_perm_rotRight, shape_in);
  end = read_cycle();
  transform_cycles += end - start;
  IntensityScaleBatch(
      0.1f, out_perm_rotRight, out2_perm_rotRight, shape_augment);

  start = read_cycle();
  permute_reflectx_rot_right2D(base_img, out_perm_rotRight_reflectX, shape_in);
  end = read_cycle();
  transform_cycles += end - start;
  IntensityScaleBatch(0.1f,
                      out_perm_rotRight_reflectX,
                      out2_perm_rotRight_reflectX,
                      shape_augment);
  // we can do the conv later

  perf.CollectDelta();
  results += "img_augmentation_" + std::to_string(h) + "_" + std::to_string(n) + ",cpu," + std::to_string(transform_cycles) + "," +
             perf.PrintCounters() + ",0\n";

  delete[] out2_perm_reflectX;
  delete[] out2_perm_reflectY;
  delete[] out2_perm_reflectX_reflectY;
  delete[] out2_perm_rotLeft;
  delete[] out2_perm_rotLeft_reflectX;
  delete[] out2_perm_rotRight;
  delete[] out2_perm_rotRight_reflectX;

  float *out3_perm_reflectX = new float[n * c * h * w];
  float *out3_perm_reflectY = new float[n * c * h * w];
  float *out3_perm_reflectX_reflectY = new float[n * c * h * w];
  float *out3_perm_rotLeft = new float[n * c * h * w];
  float *out3_perm_rotLeft_reflectX = new float[n * c * h * w];
  float *out3_perm_rotRight = new float[n * c * h * w];
  float *out3_perm_rotRight_reflectX = new float[n * c * h * w];

  DTL::EphemeralRegion *base_ephemeral =
      api->AllocEphemeralRegion(n * h * w * c * sizeof(float));
  float *base_write = (float *)base_ephemeral->GetHeadlessWriteregion();
  randomize_region_deterministic_float(base_write, n * c * h * w);
  DTL::EphemeralRegion *ephemeral_reflectX =
      api->CloneEphemeralRegion(base_ephemeral);
  DTL::EphemeralRegion *ephemeral_reflectY =
      api->CloneEphemeralRegion(base_ephemeral);
  DTL::EphemeralRegion *ephemeral_reflectX_reflectY =
      api->CloneEphemeralRegion(base_ephemeral);
  DTL::EphemeralRegion *ephemeral_rotLeft =
      api->CloneEphemeralRegion(base_ephemeral);
  DTL::EphemeralRegion *ephemeral_reflectX_rotLeft =
      api->CloneEphemeralRegion(base_ephemeral);
  DTL::EphemeralRegion *ephemeral_rotRight =
      api->CloneEphemeralRegion(base_ephemeral);
  DTL::EphemeralRegion *ephemeral_reflectX_rotRight =
      api->CloneEphemeralRegion(base_ephemeral);

  if (!api->Compile(permute_reflectX_Conf)) {
    printf("Failed to compile dtl program or map onto agu1\n");
    return "Failed to compile dtl program or map onto agu\n";
  }
  api->ProgramHardware(ephemeral_reflectX);
  float *reflectX_read = (float *)ephemeral_reflectX->GetHeadlessReadRegion();

  if (!api->Compile(permute_reflectY_Conf)) {
    printf("Failed to compile dtl program or map onto agu2\n");
    return "Failed to compile dtl program or map onto agu\n";
  }
  api->ProgramHardware(ephemeral_reflectY);
  float *reflectY_read = (float *)ephemeral_reflectY->GetHeadlessReadRegion();

  if (!api->Compile(permute_reflectX_reflectY_Conf)) {
    printf("Failed to compile dtl program or map onto agu3\n");
    return "Failed to compile dtl program or map onto agu\n";
  }
  api->ProgramHardware(ephemeral_reflectX_reflectY);
  float *reflectX_reflectY_read =
      (float *)ephemeral_reflectX_reflectY->GetHeadlessReadRegion();

  if (!api->Compile(permute_rotateLeft_Conf)) {
    printf("Failed to compile dtl program or map onto agu4\n");
    return "Failed to compile dtl program or map onto agu\n";
  }
  api->ProgramHardware(ephemeral_rotLeft);
  float *rotLeft_read = (float *)ephemeral_rotLeft->GetHeadlessReadRegion();

  if (!api->Compile(permute_rotateLeft_reflectX_Conf)) {
    printf("Failed to compile dtl program or map onto agu5\n");
    return "Failed to compile dtl program or map onto agu\n";
  }
  api->ProgramHardware(ephemeral_reflectX_rotLeft);
  float *reflectX_rotLeft_read =
      (float *)ephemeral_reflectX_rotLeft->GetHeadlessReadRegion();

  if (!api->Compile(permute_rotateRight_Conf)) {
    printf("Failed to compile dtl program or map onto agu6\n");
    return "Failed to compile dtl program or map onto agu\n";
  }
  api->ProgramHardware(ephemeral_rotRight);
  float *rotRight_read = (float *)ephemeral_rotRight->GetHeadlessReadRegion();

  if (!api->Compile(permute_rotateRight_reflectX_Conf)) {
    printf("Failed to compile dtl program or map onto agu7\n");
    return "Failed to compile dtl program or map onto agu\n";
  }
  api->ProgramHardware(ephemeral_reflectX_rotRight);
  float *reflectX_rotRight_read =
      (float *)ephemeral_reflectX_rotRight->GetHeadlessReadRegion();

  perf.CollectCounters();
  IntensityScaleBatch(0.1f, reflectX_read, out3_perm_reflectX, shape_augment);

  IntensityScaleBatch(0.1f, reflectY_read, out3_perm_reflectY, shape_augment);

  IntensityScaleBatch(
      0.1f, reflectX_reflectY_read, out3_perm_reflectX_reflectY, shape_augment);

  IntensityScaleBatch(0.1f, rotLeft_read, out3_perm_rotLeft, shape_augment);

  IntensityScaleBatch(
      0.1f, reflectX_rotLeft_read, out3_perm_rotLeft_reflectX, shape_augment);

  IntensityScaleBatch(0.1f, rotRight_read, out3_perm_rotRight, shape_augment);

  IntensityScaleBatch(
      0.1f, reflectX_rotRight_read, out3_perm_rotRight_reflectX, shape_augment);
  // we can do the conv later

  perf.CollectDelta();

  results += "img_augmentation_" + std::to_string(h) + "_" + std::to_string(n) + ",dtu," + std::string("0,") + perf.PrintCounters() + ",0\n";
 // std::cout << results << "\n";

  printf("reflectX cpu %s -- dtu %s\n",
         std::to_string(print_checksum_float(out_perm_reflectX, n * c * h * w))
             .c_str(),
         std::to_string(print_checksum_float(reflectX_read, n * c * h * w))
             .c_str());
  printf("reflectY cpu %s -- dtu %s\n",
         std::to_string(print_checksum_float(out_perm_reflectY, n * c * h * w))
             .c_str(),
         std::to_string(print_checksum_float(reflectY_read, n * c * h * w))
             .c_str());
  printf("reflectX_reflectY cpu %s -- dtu %s\n",
         std::to_string(
             print_checksum_float(out_perm_reflectX_reflectY, n * c * h * w))
             .c_str(),
         std::to_string(
             print_checksum_float(reflectX_reflectY_read, n * c * h * w))
             .c_str());
  printf("rotLeft cpu %s -- dtu %s\n",
         std::to_string(print_checksum_float(out_perm_rotLeft, n * c * h * w))
             .c_str(),
         std::to_string(print_checksum_float(rotLeft_read, n * c * h * w))
             .c_str());
  printf(
      "reflectX_rotLeft cpu %s -- dtu %s\n",
      std::to_string(
          print_checksum_float(out_perm_rotLeft_reflectX, n * c * h * w))
          .c_str(),
      std::to_string(print_checksum_float(reflectX_rotLeft_read, n * c * h * w))
          .c_str());
  printf("rotRight cpu %s -- dtu %s\n",
         std::to_string(print_checksum_float(out_perm_rotRight, n * c * h * w))
             .c_str(),
         std::to_string(print_checksum_float(rotRight_read, n * c * h * w))
             .c_str());
  printf("reflectX_rotRight cpu %s -- dtu %s\n",
         std::to_string(
             print_checksum_float(out_perm_rotRight_reflectX, n * c * h * w))
             .c_str(),
         std::to_string(
             print_checksum_float(reflectX_rotRight_read, n * c * h * w))
             .c_str());

  api->FreeEphemeralRegion(base_ephemeral);
  api->FreeEphemeralRegion(ephemeral_reflectX);
  api->FreeEphemeralRegion(ephemeral_reflectY);
  api->FreeEphemeralRegion(ephemeral_reflectX_reflectY);
  api->FreeEphemeralRegion(ephemeral_rotLeft);
  api->FreeEphemeralRegion(ephemeral_rotRight);
  api->FreeEphemeralRegion(ephemeral_reflectX_rotLeft);
  api->FreeEphemeralRegion(ephemeral_reflectX_rotRight);

  delete[] out3_perm_reflectX;
  delete[] out3_perm_reflectY;
  delete[] out3_perm_reflectX_reflectY;
  delete[] out3_perm_rotLeft;
  delete[] out3_perm_rotLeft_reflectX;
  delete[] out3_perm_rotRight;
  delete[] out3_perm_rotRight_reflectX;

  delete[] base_img; // wait so we can hash
  delete[] out_perm_reflectX;
  delete[] out_perm_reflectY;
  delete[] out_perm_reflectX_reflectY;
  delete[] out_perm_rotLeft;
  delete[] out_perm_rotLeft_reflectX;
  delete[] out_perm_rotRight;
  delete[] out_perm_rotRight_reflectX;

  return results;
}

std::string benchmark::bench_wrapper_imageAugmentation_fuse(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results = "";
    PerfManager perf;


    assert(bench_data.params.find("NMAX") != bench_data.params.end());
    assert(bench_data.params.find("CMAX") != bench_data.params.end());
    assert(bench_data.params.find("HMAX") != bench_data.params.end());
    assert(bench_data.params.find("WMAX") != bench_data.params.end());

    assert(bench_data.constants.find("H") != bench_data.constants.end());
    assert(bench_data.constants.find("W") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_n") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_h") != bench_data.constants.end());
    assert(bench_data.constants.find("stride_w") != bench_data.constants.end());

    assert(bench_data.other.find("ksize") != bench_data.other.end());
    assert(bench_data.other.find("use_real_image") != bench_data.other.end());



    auto n = static_cast<int>(bench_data.params.at("NMAX"));
    auto c = static_cast<int>(bench_data.params.at("CMAX"));
    auto h = static_cast<int>(bench_data.params.at("HMAX"));
    auto w = static_cast<int>(bench_data.params.at("WMAX"));
    std::string img_info =  std::to_string(n) + "_" + std::to_string(c) + "_" + std::to_string(h) + "_" + std::to_string(w);
    /*
        We instantiate the seven augmentations,
    */

    printf("Got params\n");
    BenchmarkData permute_reflectX_data = bench_data;
    BenchmarkData permute_reflectY_data = bench_data;
    BenchmarkData permute_reflectX_reflectY_data = bench_data;
    BenchmarkData permute_rotateLeft_data = bench_data;
    BenchmarkData permute_rotateLeft_reflectX_data = bench_data;
    BenchmarkData permute_rotateRight_data = bench_data;
    BenchmarkData permute_rotateRight_reflextX_data = bench_data;
    permute_reflectX_data.name  = "permute_reflect";
    permute_reflectY_data.name  = "permute_flipy";
    permute_reflectX_reflectY_data.name  = "permute_flipy_reflect";
    permute_rotateLeft_data.name  = "permute_rot_left";
    permute_rotateLeft_reflectX_data.name  = "permute_rot_left_reflect";
    permute_rotateRight_data.name  = "permute_rot_right";
    permute_rotateRight_reflextX_data.name  = "permute_rot_right_reflect";

    std::string permute_reflectX_Conf = CreateBenchmarkConfig(permute_reflectX_data);
    std::string permute_reflectY_Conf = CreateBenchmarkConfig(permute_reflectY_data);
    std::string permute_reflectX_reflectY_Conf = CreateBenchmarkConfig(permute_reflectX_reflectY_data);
    std::string permute_rotateLeft_Conf = CreateBenchmarkConfig(permute_rotateLeft_data);
    std::string permute_rotateLeft_reflectX_Conf = CreateBenchmarkConfig(permute_rotateLeft_reflectX_data);
    std::string permute_rotateRight_Conf = CreateBenchmarkConfig(permute_rotateRight_data);
    std::string permute_rotateRight_reflectX_Conf = CreateBenchmarkConfig(permute_rotateRight_reflextX_data);
     printf("Created config\n");


    // Now we assume all of these are compiled ahead of time, and we either can use 7 configs, or swap them mid run
    float* base_img = new float[n*c*h*w];
    float* out_perm_reflectX = new float[n*c*h*w];
    float* out_perm_reflectY = new float[n*c*h*w];
    float* out_perm_reflectX_reflectY = new float[n*c*h*w];
    float* out_perm_rotLeft = new float[n*c*h*w];
    float* out_perm_rotLeft_reflectX = new float[n*c*h*w];
    float* out_perm_rotRight = new float[n*c*h*w];
    float* out_perm_rotRight_reflectX = new float[n*c*h*w];  
    float* out2_perm_reflectX = new float[n*c*h*w];
    float* out2_perm_reflectY = new float[n*c*h*w];
    float* out2_perm_reflectX_reflectY = new float[n*c*h*w];
    float* out2_perm_rotLeft = new float[n*c*h*w];
    float* out2_perm_rotLeft_reflectX = new float[n*c*h*w];
    float* out2_perm_rotRight = new float[n*c*h*w];
    float* out2_perm_rotRight_reflectX = new float[n*c*h*w];  




    randomize_region_deterministic_float(base_img, n*c*h*w);
    printf("allocated regions\n");



    perf.CollectCounters();
    Shape shape_in({n,h,w,c});
    Shape shape_augment({n,c,h,w});
    fuse_permute_reflect_x2D(base_img, out_perm_reflectX,out2_perm_reflectX, shape_in);
    //IntensityScaleBatch(0.1f, out_perm_reflectX,out2_perm_reflectX, shape_augment);

    fuse_permute_reflect_y2D(base_img, out_perm_reflectY,out2_perm_reflectY, shape_in);
    //IntensityScaleBatch(0.1f, out_perm_reflectY,out2_perm_reflectY, shape_augment);

    fuse_permute_reflect_y_reflect_x2D(base_img, out_perm_reflectX_reflectY,out2_perm_reflectX_reflectY, shape_in);
   // IntensityScaleBatch(0.1f, out_perm_reflectX_reflectY, out2_perm_reflectX_reflectY, shape_augment);



    fuse_permute_rot_left2D(base_img, out_perm_rotLeft,out2_perm_rotLeft, shape_in);
    //IntensityScaleBatch(0.1f, out_perm_rotLeft,  out2_perm_rotLeft, shape_augment);



    fuse_permute_reflectx_rot_left2D(base_img, out_perm_rotLeft_reflectX, out2_perm_rotLeft_reflectX, shape_in);
    //IntensityScaleBatch(0.1f, out_perm_rotLeft_reflectX,out2_perm_rotLeft_reflectX, shape_augment);



    fuse_permute_rot_right2D(base_img, out_perm_rotRight,out2_perm_rotRight, shape_in);
   // IntensityScaleBatch(0.1f, out_perm_rotRight, out2_perm_rotRight, shape_augment);



    fuse_permute_reflectx_rot_right2D(base_img, out_perm_rotRight_reflectX,out2_perm_rotRight_reflectX, shape_in);
   // IntensityScaleBatch(0.1f, out_perm_rotRight_reflectX, out2_perm_rotRight_reflectX, shape_augment);
    // we can do the conv later


    perf.CollectDelta();
        results += "img_augmentation_fuse_cpu," + perf.PrintCounters() + ",0\n";

    delete[] base_img;
    delete[] out_perm_reflectX;
    delete[] out_perm_reflectY;
    delete[] out_perm_reflectX_reflectY;
    delete[] out_perm_rotLeft;
    delete[] out_perm_rotLeft_reflectX;
    delete[] out_perm_rotRight;
    delete[] out_perm_rotRight_reflectX;

    delete[] out2_perm_reflectX;
    delete[] out2_perm_reflectY;
    delete[] out2_perm_reflectX_reflectY;
    delete[] out2_perm_rotLeft;
    delete[] out2_perm_rotLeft_reflectX;
    delete[] out2_perm_rotRight;
    delete[] out2_perm_rotRight_reflectX;




    return results;
}

std::string benchmark::bench_wrapper_im2col(const BenchmarkData &bench_data,
                                            DTL::API *api) {
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
    printf("matsize 0x%x\n", im2col_matsize);
  float *im2col_cpu_matrix = new float[im2col_matsize];
  float *in_data = new float[in_data_size];
  float *filter_matrix = new float[channels_out * channels_in * ksize * ksize]; // each C_Out applies a ksize*ksize filter to each C_In

  float *outbuf = new float[out_height * out_width * channels_out];
  float *outbuf2 = new float[out_height * out_width * channels_out];
  // setup input data
  randomize_region_deterministic_float(in_data, in_data_size);
  randomize_region_deterministic_float(filter_matrix, channels_out * channels_in * ksize * ksize);

  // CPU benchmark

    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
  perf.CollectCounters();
  start = read_cycle();
  im2col_cpu(in_data,
             channels_in,
             height,
             width,
             ksize,
             stride,
             pad,
             im2col_cpu_matrix);
    end = read_cycle();
    transform_cycles = end - start;
  matmult_conv_blocked(filter_matrix, im2col_cpu_matrix, outbuf, M, K, N);
  perf.CollectDelta();
  results +=
      "im2col_" + img_info + "," + "cpu," + std::to_string(transform_cycles) + ","  + perf.PrintCounters() + "," +
      std::to_string(print_checksum_float(im2col_cpu_matrix, im2col_matsize)) +
      "\n";
  printf("done cpu\n");
  if (bench_data.output_artifact.size())
    dump_buffer_to_disk("./artifacts/im2col_cpu_" + img_info,
                        reinterpret_cast<unsigned char *>(outbuf),
                        out_height * out_width * channels_out * sizeof(float));
  // CPU benchmark

  perf.ClearCounters();


  printf("matsize 0x%x\n", im2col_matsize*sizeof(float));
  DTL::EphemeralRegion *ephemeral = api->AllocEphemeralRegion(im2col_matsize * sizeof(float));

  if (!api->Compile(conf)) {
    printf("Failed to compile dtl program or map onto agu\n");
    return "Failed to compile dtl program or map onto agu\n";
  }
  api->ProgramHardware(ephemeral);
  float *Aw = (float *)ephemeral->GetHeadlessWriteregion();
  float *Ar = (float *)ephemeral->GetHeadlessReadRegion();
  randomize_region_deterministic_float(Aw, in_data_size);

  // DTU Benchmark
  perf.CollectCounters();
  matmult_conv_blocked(filter_matrix, Ar, outbuf2, M, K, N);
  perf.CollectDelta();

  results += "im2col_" + img_info + "," + "dtu," + "0," + perf.PrintCounters() + "," +
             std::to_string(print_checksum_float(Ar, im2col_matsize)) + "\n";
  if (bench_data.output_artifact.size())
    dump_buffer_to_disk("./artifacts/im2col_dtu_" + img_info,
                        reinterpret_cast<unsigned char *>(outbuf2),
                        out_height * out_width * channels_out * sizeof(float));

  // DTU Benchmark

    for(int i=0; i<100; i++)
    {
        printf("%d: CPU=%f, DTU=%f\n", i, im2col_cpu_matrix[i], Ar[i]);
    }



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
    

    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
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
    results += "dbcolproj_" + db_info + "," + "cpu," + std::to_string(0) + ","  + perf.PrintCounters() + "," + print_checksum_ul(write_out1, col_count*row_count) + "\n";

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
    results += "dbcolproj_" + db_info + "," + "dtu," + "0,"  + perf.PrintCounters()  + "," + print_checksum_ul(write_out2, col_count*row_count) + "\n";



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



    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
    init_data_int(A, B, C1, nrows);
    printf("A %s\n", print_checksum_i32(A, nrows*ncols).c_str());
    perf.CollectCounters();
    start = read_cycle();
    transpose_naive_int(B, Bt, nrows, ncols);
    end = read_cycle();
    transform_cycles = end - start;
    matmult_opt3_pretransposed_int(A, Bt, C1, nrows);
    perf.CollectDelta();
    results += "matmul_transpose_" + mat_info + "," + "cpu," + std::to_string(transform_cycles) + ","  + perf.PrintCounters()  + "," + print_checksum_i32(Bt, nrows*ncols) + "\n";


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


    printf("checksum A %s\n", print_checksum_i32(A, nrows*ncols).c_str());
    results += "matmul_transpose_" + mat_info + "," + "dtu," + "0,"  + perf.PrintCounters()   + "," + print_checksum_i32(Ar, nrows*ncols) + "\n";


        for(int i=0; i<100; i++)
    {
        printf("matmul %d: CPU=%d, DTU=%d\n",i, Bt[i], Ar[i]);
    }

    delete[] A;
    delete[] B;
    delete[] Bt;
    delete[] C1;
    delete[] C2;
    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string benchmark::bench_wrapper_nhwc_permutation(const BenchmarkData &bench_data, DTL::API *api) 
{
    std::string results;
    PerfManager perf;
    std::string conf = CreateBenchmarkConfig(bench_data);
    std::cout << conf << "\n";

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
   // printf("alloc buffers\n");
    float* base_img; // nhwc
    float* img_nchw = new float[batch_size*img_size*img_size*channels]; // after permutation
    float* img_out = new float[batch_size*img_size*img_size*channels]; // after convolution
    float* img_out2 = new float[batch_size*img_size*img_size*channels]; // after convolution using dtu
    float* filter = new float[ksize*ksize];
    randomize_region_deterministic_float(filter, ksize*ksize);

    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        base_img = new float[batch_size*img_size*img_size*channels];
        randomize_region_deterministic_float(base_img, batch_size*img_size*img_size*channels);
    }
     printf("starting bench\n");

    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
    perf.CollectCounters();
    start = read_cycle();
    nhwc_to_nchw_cpu(base_img, batch_size, channels, img_size, img_size, img_nchw);
    end = read_cycle();
    transform_cycles = end - start;
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &img_nchw[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_out[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "nhwc_permutation_" + img_info + "," + "cpu," + std::to_string(transform_cycles) + ","  + perf.PrintCounters() + "," + std::to_string(print_checksum_float(img_nchw, batch_size*img_size*img_size*channels)) + "\n";
     

    printf("here\n");
    perf.ClearCounters();
    DTL::EphemeralRegion* ephemeral = api->AllocEphemeralRegion(batch_size*img_size*img_size*channels*sizeof(float));

    if (!api->Compile(conf)) {
        printf("Failed to compile dtl program or map onto agu\n");
        return "Failed to compile dtl program or map onto agu\n";
    }

    printf("here2\n");
    api->ProgramHardware(ephemeral);
    float* Aw = (float*)ephemeral->GetHeadlessWriteregion();
    float* Ar = (float*)ephemeral->GetHeadlessReadRegion();


    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        randomize_region_deterministic_float(Aw, batch_size*img_size*img_size*channels);
    }
     printf("starting bench\n");
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
    results += "nhwc_permutation_" + img_info + "," + "dtu," + "0," + perf.PrintCounters() + "," + std::to_string(print_checksum_float(Ar, batch_size*img_size*img_size*channels)) + "\n";
    printf("finished bench\n");


        for(int i=0; i<100; i++)
    {
        printf("permute %d: CPU=%f, DTU=%f\n", i, img_nchw[i], Ar[i]);
    }


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
    randomize_region_deterministic_float(filter, ksize*ksize);

    if (use_real_image)
        assert(false); // can implement this later. will copy single image batch_size times to make batch
    else
    {
        base_img = new float[batch_size*img_size*img_size*channels];
        randomize_region_deterministic_float(base_img, batch_size*img_size*img_size*channels);
    }
        

    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
    perf.CollectCounters();
    //nhwc_to_nchw_cpu(base_img, batch_size, channels, img_size, img_size, img_nchw);
    start = read_cycle();
    batch_to_space(base_img, batch_size, channels, img_size, img_size, img_batch_transform);
    end = read_cycle();
    transform_cycles = end - start;
    for (int n = 0; n < batch_size; n++)
    {
        for (int c = 0; c < channels; c++)
        {
            apply_filter_single_channel_nchw(filter, &img_batch_transform[n*channels*img_size*img_size + c*img_size*img_size],\
                 &img_batch_out[n*channels*out_height*out_width + c*out_height*out_width], img_size, ksize);
        }
    }  
    perf.CollectDelta();
    results += "batch2space_" + img_info + "," + "cpu," + std::to_string(transform_cycles) + ","  + perf.PrintCounters()  + "," + std::to_string(print_checksum_float(img_batch_transform, batch_size*img_size*img_size*channels)) + "\n";


    
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
        randomize_region_deterministic_float(Aw, batch_size*img_size*img_size*channels);
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
    results += "batch2space_" + img_info + "," + "dtu," + "0,"  + perf.PrintCounters() + "," + std::to_string(print_checksum_float(Ar, batch_size*img_size*img_size*channels)) + "\n";


    delete[] base_img;
    delete[] img_batch;
    delete[] img_batch_out;
    delete[] img_batch_out2;
    delete[] img_batch_transform;
    delete[] filter;
    api->FreeEphemeralRegion(ephemeral);

    return results;
}

std::string
benchmark::bench_wrapper_tensorunfold_mode1(const BenchmarkData &bench_data,
                                            DTL::API *api) {
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


    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
    perf.CollectCounters();
    start = read_cycle();
    lower_mat_4d_mode1(lowered_mat, tensor_in, d1, d2, d3, d4, stride_d1, stride_d2, stride_d3);
    end = read_cycle();
    transform_cycles = end - start;
    hadamard(tensor_out1, lowered_mat, matrix_b, d3, d1*d2*d4);
    perf.CollectDelta();
    results += "tensor_unfold_1_" + tensor_info + "," + "cpu," + std::to_string(transform_cycles) + ","  + perf.PrintCounters()  + "," + print_checksum_i32(lowered_mat, d1*d2*d3*d4) + "\n";

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
    results += "tensor_unfold_1_" + tensor_info + "," + "dtu," + "0,"  + perf.PrintCounters()   + "," + print_checksum_i32(Ar, d1*d2*d3*d4) + "\n";
    //delete[] tensor_in;
    delete[] lowered_mat; 
    delete[] tensor_out1;
    delete[] tensor_out2;
    delete[] matrix_b;
    delete[] matrix_b2; 
    delete[] tensor_in;

    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string
benchmark::bench_wrapper_tensorunfold_mode2(const BenchmarkData &bench_data,
                                            DTL::API *api) {
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


    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
    perf.CollectCounters();
    start = read_cycle();
    lower_mat_4d_mode2(lowered_mat, tensor_in, d1, d2, d3, d4, stride_d1, stride_d2, stride_d3);
    end = read_cycle();
    transform_cycles = end - start;
    hadamard(tensor_out1, lowered_mat, matrix_b, d3, d1*d2*d4);
    perf.CollectDelta();
    results += "tensor_unfold_2_" + tensor_info + "," + "cpu," + std::to_string(transform_cycles) + ","  + perf.PrintCounters()  + "," + print_checksum_i32(lowered_mat, d1*d2*d3*d4) + "\n";

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
    results += "tensor_unfold_2_" + tensor_info + "," + "dtu," + "0,"  + perf.PrintCounters()   + "," + print_checksum_i32(Ar, d1*d2*d3*d4) + "\n";
    //delete[] tensor_in;
    delete[] lowered_mat; 
    delete[] tensor_out1;
    delete[] tensor_out2;
    delete[] matrix_b;
    delete[] matrix_b2; 
    delete[] tensor_in;

    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string
benchmark::bench_wrapper_tensorunfold_mode3(const BenchmarkData &bench_data,
                                            DTL::API *api) {
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


    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
    perf.CollectCounters();
    start = read_cycle();
    lower_mat_4d_mode3(lowered_mat, tensor_in, d1, d2, d3, d4, stride_d1, stride_d2, stride_d3);
    end = read_cycle();
    transform_cycles = end - start;
    hadamard(tensor_out1, lowered_mat, matrix_b, d3, d1*d2*d4);
    perf.CollectDelta();
    results += "tensor_unfold_3_" + tensor_info + "," + "cpu," + std::to_string(transform_cycles) + ","  + perf.PrintCounters()  + "," + print_checksum_i32(lowered_mat, d1*d2*d3*d4) + "\n";

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
    results += "tensor_unfold_3_" + tensor_info + "," + "dtu," + "0,"  + perf.PrintCounters()   + "," + print_checksum_i32(Ar, d1*d2*d3*d4) + "\n";
    //delete[] tensor_in;
    delete[] lowered_mat; 
    delete[] tensor_out1;
    delete[] tensor_out2;
    delete[] matrix_b;
    delete[] matrix_b2; 
    delete[] tensor_in;

    api->FreeEphemeralRegion(ephemeral);
    return results;
}

std::string
benchmark::bench_wrapper_tensorunfold_mode4(const BenchmarkData &bench_data,
                                            DTL::API *api) {
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


    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
    perf.CollectCounters();
    start = read_cycle();
    lower_mat_4d_mode4(lowered_mat, tensor_in, d1, d2, d3, d4, stride_d1, stride_d2, stride_d3);
    end = read_cycle();
    transform_cycles = end - start;
    hadamard(tensor_out1, lowered_mat, matrix_b, d3, d1*d2*d4);
    perf.CollectDelta();
    results += "tensor_unfold_4_" + tensor_info + "," + "cpu," + std::to_string(transform_cycles) + ","  + perf.PrintCounters()  + "," + print_checksum_i32(lowered_mat, d1*d2*d3*d4) + "\n";

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
    results += "tensor_unfold_4_" + tensor_info + "," + "dtu," + "0,"  + perf.PrintCounters()   + "," + print_checksum_i32(Ar, d1*d2*d3*d4) + "\n";
    //delete[] tensor_in;
    delete[] lowered_mat; 
    delete[] tensor_out1;
    delete[] tensor_out2;
    delete[] matrix_b;
    delete[] matrix_b2; 
    delete[] tensor_in;

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
    int* tensor_out_inplace = new int[n1*h1*w1*c1];
    int* tensor_b = new int[n1*h1*w1*c1];
    int* tensor_b2 = new int[n1*h1*w1*c1];
    int* tensor_b_inplace = new int[n1*h1*w1*c1];
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



    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
    perf.CollectCounters();
    start = read_cycle();
    slice_tensor_int(sliced_tensor, tensor_in, n1, h1, w1, c1, stride_n1, stride_h1, stride_w1, stride_c1, stride_d1, stride_d2, stride_d3);
    end = read_cycle();
    transform_cycles  = end - start;
    hadamard_tensor_4d_int(tensor_out1, sliced_tensor, tensor_b, n1, h1, w1, c1);
    perf.CollectDelta();
    results += "tensor_slicing_" + tensor_info + "," + "cpu," + std::to_string(transform_cycles) + ","  + perf.PrintCounters() + "," + print_checksum_i32(sliced_tensor, n1*h1*w1*c1) + "\n";

    perf.ClearCounters();
    
    
    
    
    randomize_region_deterministic(reinterpret_cast<uint8_t*>(tensor_b_inplace), n1*h1*w1*c1*sizeof(int));
    perf.CollectCounters();
    hadamard_inPlace_int(tensor_out_inplace, tensor_in, tensor_b_inplace, n1, h1, w1, c1, stride_n1, stride_h1, stride_w1, stride_c1, stride_d1, stride_d2, stride_d3);
    perf.CollectDelta();
    results += "tensor_slicing_" + tensor_info + "," + "inplace," + "0,"  + perf.PrintCounters() + "," + print_checksum_i32(sliced_tensor, n1*h1*w1*c1) + "\n";
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
    results += "tensor_slicing_" + tensor_info + "," + "0," + "dtu," + perf.PrintCounters() + "," + print_checksum_i32(Ar, n1*h1*w1*c1) + "\n";

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


    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
    perf.CollectCounters();
    highdim_7stencil_cpu(base_tensor, new_tensor, nx, ny, nz);
    perf.CollectDelta();
    results += "highdim_7stencil_" + tensor_info + "," + "cpu," + std::to_string(0) + ","  + perf.PrintCounters() + "," + std::to_string(print_checksum_float(new_tensor, ny*nx*nz)) + "\n";



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
    results += "highdim_7stencil_" + tensor_info + "," + "dtu," + "0,"  + perf.PrintCounters() + "," + std::to_string(print_checksum_float(new_tensor2, ny*nx*nz)) + "\n";
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




    uint64_t transform_cycles = 0;
    uint64_t start;
    uint64_t end;
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
    results += "cubestencil_8corner_" + tensor_info + "," + "cpu," + std::to_string(0) + ","  + perf.PrintCounters() + "," + print_checksum_i32(filter_tensor1, d4*ncubes*CUBE_SIZE) + "\n";



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
    results += "cubestencil_8corner_" + tensor_info + "," + "dtu," + "0,"  + "," + print_checksum_i32(Ar, d4*ncubes*CUBE_SIZE) + "\n";

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
