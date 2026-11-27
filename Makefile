CXX := g++
HPX_PREFIX ?= /usr
HPX_LIBDIR ?= $(HPX_PREFIX)/lib64
HPX_FLAGS := -I$(HPX_PREFIX)/include -L$(HPX_LIBDIR) -Wl,-rpath,$(HPX_LIBDIR)
HPX_LIBS := -lhpx_init -lhpx -lhpx_core -lhwloc -ltcmalloc_minimal -pthread -ldl -lrt
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -pedantic
NVCC := nvcc
CUDA_ARCH ?= native

BUILD_DIR := build
COMMON_SOURCE := mandelbrot_common.cpp
SEQUENTIAL_SOURCE := CPU_single_thread.cpp
HPX_SOURCE := CPU_hpx.cpp
CUDA_SOURCE := GPU_cuda.cu
SEQUENTIAL_TARGET := $(BUILD_DIR)/mandelbrot_sequential
HPX_TARGET := $(BUILD_DIR)/mandelbrot_hpx
CUDA_TARGET := $(BUILD_DIR)/mandelbrot_cuda

.PHONY: all sequential hpx cuda clean

all: sequential hpx

$(BUILD_DIR):
	mkdir -p $@

sequential: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SEQUENTIAL_SOURCE) $(COMMON_SOURCE) -o $(SEQUENTIAL_TARGET)

hpx: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(HPX_FLAGS) $(HPX_SOURCE) $(COMMON_SOURCE) -o $(HPX_TARGET) $(HPX_LIBS)

cuda: $(BUILD_DIR)
	$(NVCC) -std=c++17 -O3 --fmad=false -arch=$(CUDA_ARCH) $(CUDA_SOURCE) $(COMMON_SOURCE) -o $(CUDA_TARGET)

clean:
	rm -rf $(BUILD_DIR)
