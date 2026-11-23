CXX := g++
HPX_PREFIX := ./hpx/install
export PKG_CONFIG_PATH := $(HPX_PREFIX)/lib64/pkgconfig:$(PKG_CONFIG_PATH)
export PATH := $(HPX_PREFIX)/bin:$(PATH)
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
	@if command -v hpxcxx >/dev/null 2>&1; then \
		hpxcxx --exe=$(HPX_TARGET) $(CXXFLAGS) $(HPX_SOURCE) $(COMMON_SOURCE); \
	elif pkg-config --exists hpx_application; then \
		$(CXX) $(CXXFLAGS) $(HPX_SOURCE) $(COMMON_SOURCE) -o $(HPX_TARGET) \
			$$(pkg-config --cflags --libs hpx_application); \
	elif pkg-config --exists hpx; then \
		$(CXX) $(CXXFLAGS) $(HPX_SOURCE) $(COMMON_SOURCE) -o $(HPX_TARGET) \
			$$(pkg-config --cflags --libs hpx); \
	else \
		echo "HPX is not installed or is not visible to this Makefile."; \
		echo "Install HPX and make sure hpxcxx or pkg-config can find it."; \
		exit 1; \
	fi

check: sequential
	./$(SEQUENTIAL_TARGET) --width 128 --height 128 --maxiter 100 --save /tmp/mandelbrot_check.ppm

run-seq: sequential $(OUTPUT_DIR)
	LD_LIBRARY_PATH=$(HPX_PREFIX)/lib64:$$LD_LIBRARY_PATH ./$(SEQUENTIAL_TARGET) --width 1920 --height 1080 --maxiter 500 --gradient --save $(OUTPUT_DIR)/mandelbrot_seq.ppm

run-hpx: hpx $(OUTPUT_DIR)
	LD_LIBRARY_PATH=$(HPX_PREFIX)/lib64:$$LD_LIBRARY_PATH ./$(HPX_TARGET) --width 1920 --height 1080 --maxiter 500 --gradient --threads 8 --save $(OUTPUT_DIR)/mandelbrot_hpx.ppm

clean:
	rm -rf $(BUILD_DIR) $(OUTPUT_DIR) /tmp/mandelbrot_check.ppm /tmp/mandelbrot_hpx.ppm
