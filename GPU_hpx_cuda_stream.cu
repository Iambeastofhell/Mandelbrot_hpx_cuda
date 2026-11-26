#include "mandelbrot_kernel.h"
#include "mandelbrot_kernel.cuh"

#include <cuda_runtime.h>
#include <vector>

struct StreamContext {
    cudaStream_t stream;
    Pixel* device_pixels;
    size_t chunk_bytes;
};

static std::vector<StreamContext> g_contexts;

extern "C" void stream_init(int num_streams, int width, int max_chunk_height) {
    g_contexts.resize(num_streams);
    size_t chunk_bytes = static_cast<size_t>(width) * max_chunk_height * sizeof(Pixel);
    for (int i = 0; i < num_streams; ++i) {
        cudaStreamCreate(&g_contexts[i].stream);
        cudaMalloc(&g_contexts[i].device_pixels, chunk_bytes);
        g_contexts[i].chunk_bytes = chunk_bytes;
    }
}

extern "C" void stream_cleanup() {
    for (auto& ctx : g_contexts) {
        cudaStreamDestroy(ctx.stream);
        cudaFree(ctx.device_pixels);
    }
    g_contexts.clear();
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
    cudaMemcpyAsync(host_pixels, ctx.device_pixels, bytes, cudaMemcpyDeviceToHost, ctx.stream);
}

extern "C" void stream_synchronize(int stream_idx) {
    cudaStreamSynchronize(g_contexts[stream_idx].stream);
}