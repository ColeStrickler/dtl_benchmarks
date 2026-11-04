# Target output
OUTPUT = dtl_bench

# Cross compiler
#CXX = riscv64-unknown-linux-gnu-g++
CXX = $(HOME)/opt/riscv-gcc14/bin/riscv64-unknown-linux-gnu-g++
# Native Compiler
CXX_X86 = g++
# Directories
INCLUDE_DIR = -I./src -I./src/benchmarks
SRC_DIR = ./src
DTL_DIR     = ./src/dtl/src
GAPBS_DIR ?= ../gapbs
CUSTOM_BOOM_COUNTERS = -DBOOM_PERF_EXTRA


  #-march=rv64gcv \
  #-mabi=lp64d \
  #-ftree-vectorize \
  #-fopt-info-vec-optimized \
  #-fopt-info-vec-missed \



# Compilation flags
VECTOR_FLAGS = -march=rv64gcv \
               -mabi=lp64d \
               -ftree-vectorize \
               -fopt-info-vec-optimized-missed=vectorization.log \
               -DTREE_VECTORIZE_ENABLED

CXXFLAGS = $(INCLUDE_DIR) \
           -I$(DTL_DIR) \
           -O3 \
           -g \
           -std=c++17 \
           -pthread \
           -D__aarch64__ \
           #-DTREE_VECTORIZE_ENABLED
           #$(VECTOR_FLAGS)

# C++ source files
CPP_SOURCES = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/ume/*.cpp) $(wildcard $(SRC_DIR)/gap/*.cpp) $(wildcard $(SRC_DIR)/NAS/*.cpp)

# Object files
OBJS = $(CPP_SOURCES:.cpp=.o)

# Default target
all: $(DTL_DIR)/libdtl.a $(DTL_DIR)/frontend.hh $(OUTPUT)

# Link final binary
$(OUTPUT): $(OBJS) $(DTL_DIR)/libdtl.a
	$(CXX) $(CXXFLAGS) $(OBJS) -L$(DTL_DIR) -ldtl -o $@

# Compile .cpp files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# CompileProgram's optional arguments are resolved in each caller.
$(OBJS): $(DTL_DIR)/dtl_api.hpp $(DTL_DIR)/resource_alloc_pass.hpp

$(SRC_DIR)/ume/gradzatz_invert.o $(SRC_DIR)/ume/gradzatz_benchmark.o: $(SRC_DIR)/ume/gradzatz_invert.hpp
$(SRC_DIR)/ume/gradzatz_benchmark.o $(SRC_DIR)/dtlbench.o: $(SRC_DIR)/benchmarks.hpp
$(SRC_DIR)/ume/gradzatp_invert.o $(SRC_DIR)/ume/gradzatp_benchmark.o: $(SRC_DIR)/ume/gradzatp_invert.hpp
$(SRC_DIR)/ume/gradzatp_benchmark.o: $(SRC_DIR)/benchmarks.hpp

$(SRC_DIR)/gap/bfs.o $(SRC_DIR)/gap/bfs_input.o $(SRC_DIR)/gap/bfs_benchmark.o: $(SRC_DIR)/gap/bfs.hpp
$(SRC_DIR)/gap/bfs_benchmark.o: $(SRC_DIR)/benchmarks.hpp $(SRC_DIR)/gap/bfs_dtu.hpp
$(SRC_DIR)/gap/bfs_input.o: CXXFLAGS += -I$(GAPBS_DIR)/src
$(SRC_DIR)/gap/bfs_input.o: $(wildcard $(GAPBS_DIR)/src/*.h)

$(SRC_DIR)/NAS/cg.o $(SRC_DIR)/NAS/cg_benchmark.o: $(SRC_DIR)/NAS/cg.hpp
$(SRC_DIR)/NAS/cg_benchmark.o: $(SRC_DIR)/benchmarks.hpp $(SRC_DIR)/NAS/cg_dtu.hpp
$(SRC_DIR)/benchmarks.o: $(SRC_DIR)/benchmarks.hpp
# Preserve serial float reduction order in both CG implementations.
$(filter $(SRC_DIR)/NAS/%, $(OBJS)): CXXFLAGS += -fno-tree-vectorize -ffp-contract=off

# Ensure DTL is built first (including frontend.hh and libdtl.a)
$(DTL_DIR)/libdtl.a: $(wildcard $(DTL_DIR)/*.cpp) $(wildcard $(DTL_DIR)/optimizations/*.cpp) $(DTL_DIR)/Makefile $(DTL_DIR)/dtl_api.hpp $(DTL_DIR)/resource_alloc_pass.hpp
	$(MAKE) -C $(DTL_DIR) lib CXX_RISCV="$(CXX)"

$(DTL_DIR)/frontend.hh: $(DTL_DIR)/parser.yy
	$(MAKE) -C $(DTL_DIR)

sort:
	$(CXX) -O3 -std=c++17 sort_compare.cpp -o sort_compare
# Clean everything


clean:
	rm -f $(OUTPUT) $(OBJS)
	$(MAKE) -C $(DTL_DIR) clean