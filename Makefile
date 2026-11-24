CXX := g++
HPX_PREFIX ?= /usr
HPX_LIBDIR ?= $(HPX_PREFIX)/lib64
HPX_FLAGS := -I$(HPX_PREFIX)/include -L$(HPX_LIBDIR) -Wl,-rpath,$(HPX_LIBDIR)
HPX_LIBS := -lhpx_init -lhpx -lhpx_core -lhwloc -ltcmalloc_minimal -pthread -ldl -lrt
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -pedantic

BUILD_DIR := build
OUTPUT_DIR := output
COMMON_SOURCE := mandelbrot_common.cpp
SEQUENTIAL_SOURCE := CPU_single_thread.cpp
HPX_SOURCE := CPU_hpx.cpp
SEQUENTIAL_TARGET := $(BUILD_DIR)/mandelbrot_sequential
HPX_TARGET := $(BUILD_DIR)/mandelbrot_hpx

.PHONY: all sequential hpx clean check run-hpx run-seq

all: sequential hpx

$(BUILD_DIR) $(OUTPUT_DIR):
	mkdir -p $@

sequential: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(SEQUENTIAL_SOURCE) $(COMMON_SOURCE) -o $(SEQUENTIAL_TARGET)

hpx: $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(HPX_FLAGS) $(HPX_SOURCE) $(COMMON_SOURCE) -o $(HPX_TARGET) $(HPX_LIBS)

check: sequential
	./$(SEQUENTIAL_TARGET) --width 128 --height 128 --maxiter 100 --save /tmp/mandelbrot_check.ppm

run-seq: sequential $(OUTPUT_DIR)
	./$(SEQUENTIAL_TARGET) --width 1920 --height 1080 --maxiter 500 --gradient --save $(OUTPUT_DIR)/mandelbrot_seq.ppm

run-hpx: hpx $(OUTPUT_DIR)
	./$(HPX_TARGET) --width 1920 --height 1080 --maxiter 500 --gradient --threads 8 --save $(OUTPUT_DIR)/mandelbrot_hpx.ppm

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR) /tmp/mandelbrot_check.ppm /tmp/mandelbrot_hpx.ppm
