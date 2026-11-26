CXX := g++
HPX_PREFIX ?= /usr
HPX_LIBDIR ?= $(HPX_PREFIX)/lib64
HPX_FLAGS := -I$(HPX_PREFIX)/include -L$(HPX_LIBDIR) -Xlinker -rpath -Xlinker $(HPX_LIBDIR)
HPX_LIBS := -lhpx_init -lhpx -lhpx_core -lhwloc -ltcmalloc_minimal -ldl -lrt
HPX_LIBS_NVCC := -lhpx_init -lhpx -lhpx_core -lhwloc -ltcmalloc_minimal -Xcompiler -pthread -ldl -lrt
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -pedantic
NVCC := nvcc
CUDA_ARCH ?= native
CUDA_INC := /usr/local/cuda/include
CUDA_LIB := /usr/local/cuda/lib64

BUILD_DIR := build
OUTPUT_DIR := output
COMMON_SOURCE := mandelbrot_common.cpp
SEQUENTIAL_SOURCE := CPU_single_thread.cpp
HPX_SOURCE := CPU_hpx.cpp
CUDA_SOURCE := GPU_cuda.cu
HPX_CUDA_SOURCE := GPU_hpx_cuda.cpp
HPX_CUDA_KERNEL := GPU_hpx_cuda_kernel.cu
SEQUENTIAL_TARGET := $(BUILD_DIR)/mandelbrot_sequential
HPX_TARGET := $(BUILD_DIR)/mandelbrot_hpx
CUDA_TARGET := $(BUILD_DIR)/mandelbrot_cuda
HPX_CUDA_TARGET := $(BUILD_DIR)/mandelbrot_hpx_cuda
HPX_CUDA_KERNEL_OBJ := $(BUILD_DIR)/GPU_hpx_cuda_kernel.o

.PHONY: all sequential hpx cuda hpx-cuda clean check run-hpx run-seq run-cuda run-hpx-cuda benchmark

all: sequential hpx hpx-cuda

$(BUILD_DIR) $(OUTPUT_DIR):
	mkdir -p $@

sequential: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SEQUENTIAL_SOURCE) $(COMMON_SOURCE) -o $(SEQUENTIAL_TARGET)

hpx: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(HPX_FLAGS) $(HPX_SOURCE) $(COMMON_SOURCE) -o $(HPX_TARGET) $(HPX_LIBS)

hpx-cuda: $(BUILD_DIR) $(HPX_CUDA_KERNEL_OBJ)
	$(CXX) $(CXXFLAGS) -I$(CUDA_INC) -L$(CUDA_LIB) $(HPX_CUDA_SOURCE) $(COMMON_SOURCE) $(HPX_CUDA_KERNEL_OBJ) -o $(HPX_CUDA_TARGET) -lcudart -lpthread

$(HPX_CUDA_KERNEL_OBJ): $(HPX_CUDA_KERNEL) $(BUILD_DIR)
	$(NVCC) -std=c++17 -O3 --fmad=false -arch=$(CUDA_ARCH) -c $< -o $@

cuda: $(BUILD_DIR)
	$(NVCC) -std=c++17 -O3 --fmad=false -arch=$(CUDA_ARCH) $(CUDA_SOURCE) $(COMMON_SOURCE) -o $(CUDA_TARGET)

check: sequential
	./$(SEQUENTIAL_TARGET) --width 128 --height 128 --maxiter 100 --save /tmp/mandelbrot_check.ppm

run-seq: sequential $(OUTPUT_DIR)
	./$(SEQUENTIAL_TARGET) --width 1920 --height 1080 --maxiter 500 --gradient --save $(OUTPUT_DIR)/mandelbrot_seq.ppm

run-hpx: hpx $(OUTPUT_DIR)
	./$(HPX_TARGET) --width 1920 --height 1080 --maxiter 500 --gradient --threads 8 --save $(OUTPUT_DIR)/mandelbrot_hpx.ppm

run-hpx-cuda: hpx-cuda $(OUTPUT_DIR)
	./$(HPX_CUDA_TARGET) --width 1920 --height 1080 --maxiter 500 --gradient --threads 8 --save $(OUTPUT_DIR)/mandelbrot_hpx_cuda.ppm

run-cuda: cuda $(OUTPUT_DIR)
	./$(CUDA_TARGET) --width 1920 --height 1080 --maxiter 500 --gradient --save $(OUTPUT_DIR)/mandelbrot_cuda.ppm

benchmark: all hpx-cuda
	python3 benchmark/run_benchmark.py
	python3 benchmark/make_tables.py > benchmark/results.md

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR) /tmp/mandelbrot_check.ppm /tmp/mandelbrot_hpx.ppm
