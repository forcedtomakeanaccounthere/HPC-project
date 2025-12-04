// cuda_kernels.cu - CUDA kernel implementations
#include <cuda_runtime.h>
#include <curand_kernel.h>
#include <stdio.h>
#include <math.h>

#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            fprintf(stderr, "CUDA Error: %s:%d, %s\n", __FILE__, __LINE__, \
                    cudaGetErrorString(error)); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)

// ==================== CUDA KERNELS ====================

__global__ void grayscale_kernel(unsigned char* data, int width, int height, int channels) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x < width && y < height && channels >= 3) {
        int idx = (y * width + x) * channels;
        
        float gray = 0.299f * data[idx] + 
                    0.587f * data[idx + 1] + 
                    0.114f * data[idx + 2];
        
        data[idx] = (unsigned char)gray;
        data[idx + 1] = (unsigned char)gray;
        data[idx + 2] = (unsigned char)gray;
    }
}

__global__ void gaussian_blur_kernel(unsigned char* input, unsigned char* output,
                                    int width, int height, int channels,
                                    float* kernel, int kernel_size) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    int kernel_radius = kernel_size / 2;

    for (int c = 0; c < channels; c++) {
        float sum = 0.0f;

        for (int ky = -kernel_radius; ky <= kernel_radius; ky++) {
            for (int kx = -kernel_radius; kx <= kernel_radius; kx++) {
                int py = min(max(y + ky, 0), height - 1);
                int px = min(max(x + kx, 0), width - 1);

                int pixel_idx = (py * width + px) * channels + c;
                int kernel_idx = (ky + kernel_radius) * kernel_size + (kx + kernel_radius);

                sum += input[pixel_idx] * kernel[kernel_idx];
            }
        }

        output[(y * width + x) * channels + c] = (unsigned char)(sum + 0.5f);
    }
}

__global__ void sharpening_kernel(unsigned char* input, unsigned char* output,
                                 int width, int height, int channels) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    float kernel[3][3] = {
        {0, -1, 0},
        {-1, 5, -1},
        {0, -1, 0}
    };

    for (int c = 0; c < channels; c++) {
        float sum = 0.0f;

        for (int ky = -1; ky <= 1; ky++) {
            for (int kx = -1; kx <= 1; kx++) {
                int py = min(max(y + ky, 0), height - 1);
                int px = min(max(x + kx, 0), width - 1);

                int pixel_idx = (py * width + px) * channels + c;
                sum += input[pixel_idx] * kernel[ky + 1][kx + 1];
            }
        }

        sum = fminf(fmaxf(sum, 0.0f), 255.0f);
        output[(y * width + x) * channels + c] = (unsigned char)sum;
    }
}

__global__ void edge_detection_kernel(unsigned char* input, unsigned char* output,
                                     int width, int height, int channels) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    float sobel_x[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };

    float sobel_y[3][3] = {
        {-1, -2, -1},
        {0, 0, 0},
        {1, 2, 1}
    };

    for (int c = 0; c < channels; c++) {
        float gx = 0.0f, gy = 0.0f;

        for (int ky = -1; ky <= 1; ky++) {
            for (int kx = -1; kx <= 1; kx++) {
                int py = min(max(y + ky, 0), height - 1);
                int px = min(max(x + kx, 0), width - 1);

                int pixel_idx = (py * width + px) * channels + c;
                float pixel_value = input[pixel_idx];

                gx += pixel_value * sobel_x[ky + 1][kx + 1];
                gy += pixel_value * sobel_y[ky + 1][kx + 1];
            }
        }

        float magnitude = sqrtf(gx * gx + gy * gy);
        magnitude = fminf(magnitude, 255.0f);

        output[(y * width + x) * channels + c] = (unsigned char)magnitude;
    }
}

__global__ void add_noise_kernel(unsigned char* data, int width, int height, 
                                int channels, float noise_level, unsigned long long seed) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= width || y >= height) return;

    // Initialize random state
    curandState state;
    int idx_base = (y * width + x) * channels;
    curand_init(seed, idx_base, 0, &state);

    for (int c = 0; c < channels; c++) {
        int idx = idx_base + c;
        
        // Generate Gaussian noise using curand
        float noise = curand_normal(&state) * noise_level;
        
        float new_value = (float)data[idx] + noise;
        new_value = fminf(fmaxf(new_value, 0.0f), 255.0f);
        
        data[idx] = (unsigned char)new_value;
    }
}

__global__ void downsample_kernel(unsigned char* input, unsigned char* output,
                                 int width, int height, int new_width, int new_height,
                                 int channels, int scale_factor) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;

    if (x >= new_width || y >= new_height) return;

    for (int c = 0; c < channels; c++) {
        float sum = 0.0f;
        int count = 0;

        for (int dy = 0; dy < scale_factor; dy++) {
            for (int dx = 0; dx < scale_factor; dx++) {
                int src_y = y * scale_factor + dy;
                int src_x = x * scale_factor + dx;

                if (src_y < height && src_x < width) {
                    int src_idx = (src_y * width + src_x) * channels + c;
                    sum += input[src_idx];
                    count++;
                }
            }
        }

        int dst_idx = (y * new_width + x) * channels + c;
        output[dst_idx] = (unsigned char)(sum / count + 0.5f);
    }
}

// ==================== HOST INTERFACE FUNCTIONS ====================

extern "C" {

void cuda_grayscale(unsigned char* data, int width, int height, int channels) {
    size_t size = width * height * channels;
    unsigned char* d_data;

    CUDA_CHECK(cudaMalloc(&d_data, size));
    CUDA_CHECK(cudaMemcpy(d_data, data, size, cudaMemcpyHostToDevice));

    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                  (height + blockSize.y - 1) / blockSize.y);

    grayscale_kernel<<<gridSize, blockSize>>>(d_data, width, height, channels);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(data, d_data, size, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(d_data));
}

void cuda_gaussian_blur(unsigned char* input, unsigned char* output,
                       int width, int height, int channels, float sigma) {
    size_t size = width * height * channels;
    
    // Create Gaussian kernel
    int kernel_size = (int)(6 * sigma + 1);
    if (kernel_size % 2 == 0) kernel_size++;
    int kernel_radius = kernel_size / 2;
    
    float* h_kernel = (float*)malloc(kernel_size * kernel_size * sizeof(float));
    float kernel_sum = 0.0f;
    
    for (int y = -kernel_radius; y <= kernel_radius; y++) {
        for (int x = -kernel_radius; x <= kernel_radius; x++) {
            float value = exp(-(x*x + y*y) / (2 * sigma * sigma));
            h_kernel[(y + kernel_radius) * kernel_size + (x + kernel_radius)] = value;
            kernel_sum += value;
        }
    }
    
    for (int i = 0; i < kernel_size * kernel_size; i++) {
        h_kernel[i] /= kernel_sum;
    }
    
    unsigned char *d_input, *d_output;
    float *d_kernel;

    CUDA_CHECK(cudaMalloc(&d_input, size));
    CUDA_CHECK(cudaMalloc(&d_output, size));
    CUDA_CHECK(cudaMalloc(&d_kernel, kernel_size * kernel_size * sizeof(float)));

    CUDA_CHECK(cudaMemcpy(d_input, input, size, cudaMemcpyHostToDevice));
    CUDA_CHECK(cudaMemcpy(d_kernel, h_kernel, kernel_size * kernel_size * sizeof(float),
                         cudaMemcpyHostToDevice));

    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                  (height + blockSize.y - 1) / blockSize.y);

    gaussian_blur_kernel<<<gridSize, blockSize>>>(d_input, d_output, width, height,
                                                  channels, d_kernel, kernel_size);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(output, d_output, size, cudaMemcpyDeviceToHost));

    CUDA_CHECK(cudaFree(d_input));
    CUDA_CHECK(cudaFree(d_output));
    CUDA_CHECK(cudaFree(d_kernel));
    free(h_kernel);
}

void cuda_sharpening(unsigned char* input, unsigned char* output,
                    int width, int height, int channels) {
    size_t size = width * height * channels;
    unsigned char *d_input, *d_output;

    CUDA_CHECK(cudaMalloc(&d_input, size));
    CUDA_CHECK(cudaMalloc(&d_output, size));
    CUDA_CHECK(cudaMemcpy(d_input, input, size, cudaMemcpyHostToDevice));

    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                  (height + blockSize.y - 1) / blockSize.y);

    sharpening_kernel<<<gridSize, blockSize>>>(d_input, d_output, width, height, channels);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(output, d_output, size, cudaMemcpyDeviceToHost));

    CUDA_CHECK(cudaFree(d_input));
    CUDA_CHECK(cudaFree(d_output));
}

void cuda_edge_detection(unsigned char* input, unsigned char* output,
                        int width, int height, int channels) {
    size_t size = width * height * channels;
    unsigned char *d_input, *d_output;

    CUDA_CHECK(cudaMalloc(&d_input, size));
    CUDA_CHECK(cudaMalloc(&d_output, size));
    CUDA_CHECK(cudaMemcpy(d_input, input, size, cudaMemcpyHostToDevice));

    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                  (height + blockSize.y - 1) / blockSize.y);

    edge_detection_kernel<<<gridSize, blockSize>>>(d_input, d_output, width, height, channels);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(output, d_output, size, cudaMemcpyDeviceToHost));

    CUDA_CHECK(cudaFree(d_input));
    CUDA_CHECK(cudaFree(d_output));
}

void cuda_add_noise(unsigned char* data, int width, int height,
                   int channels, float noise_level) {
    size_t size = width * height * channels;
    unsigned char* d_data;

    CUDA_CHECK(cudaMalloc(&d_data, size));
    CUDA_CHECK(cudaMemcpy(d_data, data, size, cudaMemcpyHostToDevice));

    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                  (height + blockSize.y - 1) / blockSize.y);

    unsigned long long seed = (unsigned long long)time(NULL);
    add_noise_kernel<<<gridSize, blockSize>>>(d_data, width, height, channels, 
                                             noise_level, seed);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(data, d_data, size, cudaMemcpyDeviceToHost));
    CUDA_CHECK(cudaFree(d_data));
}

void cuda_downsample(unsigned char* input, unsigned char* output,
                    int width, int height, int new_width, int new_height, int channels) {
    size_t input_size = width * height * channels;
    size_t output_size = new_width * new_height * channels;
    unsigned char *d_input, *d_output;

    CUDA_CHECK(cudaMalloc(&d_input, input_size));
    CUDA_CHECK(cudaMalloc(&d_output, output_size));
    CUDA_CHECK(cudaMemcpy(d_input, input, input_size, cudaMemcpyHostToDevice));

    int scale_factor = width / new_width;

    dim3 blockSize(16, 16);
    dim3 gridSize((new_width + blockSize.x - 1) / blockSize.x,
                  (new_height + blockSize.y - 1) / blockSize.y);

    downsample_kernel<<<gridSize, blockSize>>>(d_input, d_output, width, height,
                                               new_width, new_height, channels, scale_factor);
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());

    CUDA_CHECK(cudaMemcpy(output, d_output, output_size, cudaMemcpyDeviceToHost));

    CUDA_CHECK(cudaFree(d_input));
    CUDA_CHECK(cudaFree(d_output));
}

} // extern "C"