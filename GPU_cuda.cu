#include "mandelbrot_kernel.cuh"

#include <chrono>
#include <iostream>
#include <string>

static Image generate_mandelbrot(const AppOptions& options) {
    Image image(options.width, options.height);
    std::size_t bytes = image.pixels.size() * sizeof(Pixel);

    Pixel* device_pixels = nullptr;
    cudaEvent_t start;
    cudaEvent_t stop;
    cuda_check(cudaMalloc(&device_pixels, bytes));
    cuda_check(cudaEventCreate(&start));
    cuda_check(cudaEventCreate(&stop));

    dim3 block(16, 16);
    dim3 grid((options.width + block.x - 1) / block.x,
              (options.height + block.y - 1) / block.y);

    cuda_check(cudaEventRecord(start));
    mandelbrot_rows<<<grid, block>>>(device_pixels, make_view(options), 0, options.height);
    cuda_check(cudaGetLastError());
    cuda_check(cudaEventRecord(stop));

    cuda_check(cudaMemcpy(image.pixels.data(), device_pixels, bytes, cudaMemcpyDeviceToHost));

    float kernel_ms = 0.0f;
    cuda_check(cudaEventElapsedTime(&kernel_ms, start, stop));
    std::cout << "CUDA kernel time: " << kernel_ms / 1000.0 << " seconds\n";

    cuda_check(cudaEventDestroy(start));
    cuda_check(cudaEventDestroy(stop));
    cuda_check(cudaFree(device_pixels));
    return image;
}

int main(int argc, char* argv[]) {
    AppOptions options;
    std::string error;

    if (!parse_arguments(argc, argv, options, error)) {
        if (error.empty()) {
            return 0;
        }
        std::cerr << "Error: " << error << '\n';
        print_usage(argv[0]);
        return 1;
    }

    if (!validate_arguments(options, error)) {
        std::cerr << "Error: " << error << '\n';
        print_usage(argv[0]);
        return 1;
    }

    Image image(0, 0);

    if (!options.load_file.empty()) {
        bool loaded = false;
        image = load_ppm(options.load_file, loaded);
        if (!loaded) {
            return 1;
        }
    } else {
        cuda_check(cudaFree(nullptr));

        auto start = std::chrono::steady_clock::now();
        image = generate_mandelbrot(options);
        auto end = std::chrono::steady_clock::now();

        std::chrono::duration<double> elapsed = end - start;
        std::cout << "CUDA generation time: " << elapsed.count() << " seconds\n";
        std::cout << "Generated Mandelbrot set (" << options.width << "x"
                  << options.height << ", max_iter=" << options.max_iter
                  << ")\n";
    }

    if (!options.save_file.empty() && !save_ppm(image, options.save_file)) {
        return 1;
    }

    if (options.display) {
        display_image(image, options);
    }

    return 0;
}
