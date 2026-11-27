CXX := g++
HPX_PREFIX ?= /usr
HPX_LIBDIR ?= $(HPX_PREFIX)/lib64
HPX_FLAGS := -I$(HPX_PREFIX)/include -L$(HPX_LIBDIR) -Wl,-rpath,$(HPX_LIBDIR)
HPX_LIBS := -lhpx_init -lhpx -lhpx_core -lhwloc -ltcmalloc_minimal -pthread -ldl -lrt
CXXFLAGS := -std=c++17 -O3 -ffp-contract=off -Wall -Wextra -pedantic
NVCC := nvcc
NVCCFLAGS := -std=c++17 -O3 --fmad=false
SDL_FLAGS = $(shell pkg-config --cflags --libs sdl2)
CUDA_ARCH ?= native

BUILD_DIR := build
COMMON_SOURCE := src/app.cpp
SEQUENTIAL_SOURCE := src/sequential.cpp
HPX_SOURCE := src/hpx.cpp
CUDA_SOURCE := src/cuda.cu
EXPLORER_SOURCE := src/explorer.cpp
SEQUENTIAL_TARGET := $(BUILD_DIR)/mandelbrot_sequential
HPX_TARGET := $(BUILD_DIR)/mandelbrot_hpx
CUDA_TARGET := $(BUILD_DIR)/mandelbrot_cuda
EXPLORER_TARGET := $(BUILD_DIR)/mandelbrot_explorer

.PHONY: all sequential hpx cuda explorer clean

all: sequential hpx

$(BUILD_DIR):
	mkdir -p $@

sequential: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SEQUENTIAL_SOURCE) $(COMMON_SOURCE) -o $(SEQUENTIAL_TARGET)

hpx: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(HPX_FLAGS) $(HPX_SOURCE) $(COMMON_SOURCE) -o $(HPX_TARGET) $(HPX_LIBS)

explorer: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(HPX_FLAGS) $(EXPLORER_SOURCE) $(COMMON_SOURCE) -o $(EXPLORER_TARGET) $(HPX_LIBS) $(SDL_FLAGS)

cuda: $(BUILD_DIR)
	$(NVCC) $(NVCCFLAGS) -arch=$(CUDA_ARCH) $(CUDA_SOURCE) $(COMMON_SOURCE) -o $(CUDA_TARGET)

clean:
	rm -rf $(BUILD_DIR)
