#include "mandelbrot_kernel.h"
#include "mandelbrot_kernel.cuh"

#include <cuda_runtime.h>
#include <vector>

struct StreamContext {
    cudaStream_t stream;
    Pixel* device_pixels;
};

static std::vector<StreamContext> g_contexts;

extern "C" void cuda_startup() {
    cuda_check(cudaFree(nullptr));
}

// The image is pinned so that the copies into it do not block the host.
extern "C" void stream_init(int num_streams, Image& image, int strip_height) {
    cuda_check(cudaHostRegister(image.pixels.data(), image.pixels.size() * sizeof(Pixel),
                                cudaHostRegisterDefault));
    g_contexts.resize(num_streams);
    size_t chunk_bytes = static_cast<size_t>(image.width) * strip_height * sizeof(Pixel);
    for (auto& ctx : g_contexts) {
        cuda_check(cudaStreamCreate(&ctx.stream));
        cuda_check(cudaMalloc(&ctx.device_pixels, chunk_bytes));
    }
}

extern "C" void stream_cleanup(Image& image) {
    for (auto& ctx : g_contexts) {
        cuda_check(cudaStreamDestroy(ctx.stream));
        cuda_check(cudaFree(ctx.device_pixels));
    }
    g_contexts.clear();
    cuda_check(cudaHostUnregister(image.pixels.data()));
}

extern "C" void launch_mandelbrot_stream(Pixel* host_pixels, const MandelbrotView& view,
                                         int row_begin, int row_end, int stream_idx) {
    StreamContext& ctx = g_contexts[stream_idx];
    int width = view.width;
    int chunk_height = row_end - row_begin;
    size_t bytes = static_cast<size_t>(width) * chunk_height * sizeof(Pixel);

    dim3 block(16, 16);
    dim3 grid((width + block.x - 1) / block.x,
              (chunk_height + block.y - 1) / block.y);

    mandelbrot_rows_chunk<<<grid, block, 0, ctx.stream>>>(ctx.device_pixels, view, row_begin, row_end);
    cuda_check(cudaGetLastError());
    cuda_check(cudaMemcpyAsync(host_pixels, ctx.device_pixels, bytes, cudaMemcpyDeviceToHost, ctx.stream));
}

extern "C" bool stream_done(int stream_idx) {
    cudaError_t status = cudaStreamQuery(g_contexts[stream_idx].stream);
    if (status == cudaErrorNotReady) {
        return false;
    }
    cuda_check(status);
    return true;
}