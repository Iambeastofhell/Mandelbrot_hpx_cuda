#include "app.hpp"

#include <cuda_runtime.h>

#include <cstdlib>
#include <iostream>

static void cuda_check(cudaError_t status) {
    if (status != cudaSuccess) {
        std::cerr << "CUDA error: " << cudaGetErrorString(status) << '\n';
        std::exit(1);
    }
}

// Computes any rectangle of the canvas, one thread per pixel.
__global__ void count_rect(View view, Rect rect, Canvas canvas) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < rect.width && y < rect.height) {
        canvas.at(rect.left + x, rect.top + y) = pixel_count(view, rect.left + x, rect.top + y);
    }
}

static void render(const Options& options, std::vector<int>& counts) {
    const Rect& area = options.view.area;
    std::size_t bytes = counts.size() * sizeof(int);

    int* device_counts = nullptr;
    cuda_check(cudaMalloc(&device_counts, bytes));

    dim3 block(16, 16);
    dim3 grid((area.width + block.x - 1) / block.x,
              (area.height + block.y - 1) / block.y);

    count_rect<<<grid, block>>>(options.view, area, Canvas{device_counts, area});
    cuda_check(cudaGetLastError());

    cuda_check(cudaMemcpy(counts.data(), device_counts, bytes, cudaMemcpyDeviceToHost));
    cuda_check(cudaFree(device_counts));
}

int main(int argc, char* argv[]) {
    Options options = parse_options(argc, argv);

    // Starts the CUDA runtime so that it is not timed.
    cuda_check(cudaFree(nullptr));

    return run(options, "CUDA", render);
}
