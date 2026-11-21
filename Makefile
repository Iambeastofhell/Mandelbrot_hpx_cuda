CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -pedantic

COMMON_SOURCE := mandelbrot_common.cpp
SEQUENTIAL_SOURCE := CPU_single_thread.cpp
HPX_SOURCE := CPU_hpx.cpp
SEQUENTIAL_TARGET := mandelbrot_sequential
HPX_TARGET := mandelbrot_hpx

.PHONY: all sequential hpx clean check

all: sequential hpx

sequential:
	$(CXX) $(CXXFLAGS) $(SEQUENTIAL_SOURCE) $(COMMON_SOURCE) -o $(SEQUENTIAL_TARGET)

hpx:
	@if command -v hpxcxx >/dev/null 2>&1; then \
		hpxcxx $(CXXFLAGS) $(HPX_SOURCE) $(COMMON_SOURCE) -o $(HPX_TARGET); \
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

check:
	./$(SEQUENTIAL_TARGET) --width 128 --height 128 --maxiter 100 --save /tmp/mandelbrot_check.ppm

clean:
	rm -f $(SEQUENTIAL_TARGET) $(HPX_TARGET) /tmp/mandelbrot_check.ppm /tmp/mandelbrot_hpx.ppm
