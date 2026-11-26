#include "mandelbrot_kernel.h"
#include "mandelbrot_kernel.cuh"

#include <cuda_runtime.h>

__global__ void mandelbrot_rows_chunk(Pixel* pixels, MandelbrotView view, int row_begin, int row_end) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = row_begin + blockIdx.y * blockDim.y + threadIdx.y;

    if (x < view.width && y < row_end) {
        pixels[(y - row_begin) * view.width + x] = mandelbrot_pixel(view, x, y);
    }
}

extern "C" void launch_mandelbrot_kernel(Pixel* host_pixels, const MandelbrotView& view, int row_begin, int row_end) {
    int width = view.width;
    int height = row_end - row_begin;
    std::size_t bytes = static_cast<std::size_t>(width) * height * sizeof(Pixel);

    Pixel* device_pixels = nullptr;
    cudaMalloc(&device_pixels, bytes);

    dim3 block(16, 16);
    dim3 grid((width + block.x - 1) / block.x,
              (height + block.y - 1) / block.y);

    mandelbrot_rows_chunk<<<grid, block>>>(device_pixels, view, row_begin, row_end);
    cudaGetLastError();
    cudaMemcpy(host_pixels, device_pixels, bytes, cudaMemcpyDeviceToHost);
    cudaFree(device_pixels);
}