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

__global__ void count_pixels(int* counts, View view) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < view.width && y < view.height) {
        counts[y * view.width + x] = pixel_count(view, x, y);
    }
}

static void render(const Options& options, std::vector<int>& counts) {
    const View& view = options.view;
    std::size_t bytes = counts.size() * sizeof(int);

    int* device_counts = nullptr;
    cuda_check(cudaMalloc(&device_counts, bytes));

    dim3 block(16, 16);
    dim3 grid((view.width + block.x - 1) / block.x,
              (view.height + block.y - 1) / block.y);

    count_pixels<<<grid, block>>>(device_counts, view);
    cuda_check(cudaGetLastError());

    cuda_check(cudaMemcpy(counts.data(), device_counts, bytes, cudaMemcpyDeviceToHost));
    cuda_check(cudaFree(device_counts));
}

int main(int argc, char* argv[]) {
    Options options = parse_options(argc, argv);

    // Starts the CUDA runtime so that it is not timed.
    if (options.load_file.empty()) {
        cuda_check(cudaFree(nullptr));
    }

    return run(options, "CUDA", render);
}
