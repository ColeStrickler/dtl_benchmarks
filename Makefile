# Target output
OUTPUT = dtl_bench

# Cross compiler
CXX = riscv64-unknown-linux-gnu-g++

# Native Compiler
CXX_X86 = g++
# Directories
INCLUDE_DIR = ./src
SRC_DIR = ./src
DTL_DIR     = ./src/dtl/src
CUSTOM_BOOM_COUNTERS = -DBOOM_PERF_EXTRA


# Compilation flags
CXXFLAGS = -I$(INCLUDE_DIR) -I$(DTL_DIR) -O3 -g -D __aarch64__ -std=c++17 -pthread

# C++ source files
CPP_SOURCES = $(wildcard $(SRC_DIR)/*.cpp) 

# Object files
OBJS = $(CPP_SOURCES:.cpp=.o)

# Default target
all: $(DTL_DIR)/libdtl.a $(DTL_DIR)/frontend.hh $(OUTPUT)

# Link final binary
$(OUTPUT): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -L$(DTL_DIR) -ldtl -o $@

# Compile .cpp files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Ensure DTL is built first (including frontend.hh and libdtl.a)
$(DTL_DIR)/libdtl.a:
	$(MAKE) -C $(DTL_DIR)

$(DTL_DIR)/frontend.hh: $(DTL_DIR)/parser.yy
	$(MAKE) -C $(DTL_DIR)

# Clean everything
clean:
	rm -f $(OUTPUT) $(OBJS)
	$(MAKE) -C $(DTL_DIR) clean
