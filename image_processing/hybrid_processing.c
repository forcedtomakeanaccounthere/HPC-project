// hybrid_processing.c - Host code with OpenMP task-level parallelism
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

typedef struct {
    unsigned char *data;
    int width;
    int height;
    int channels;
} Image;

// Forward declarations
Image* load_image(const char* filename);
void free_image(Image* img);
void save_image(const char* filename, Image* img);
Image* create_image(int width, int height, int channels);

// CUDA interface functions (implemented in cuda_kernels.cu)
extern void cuda_grayscale(unsigned char* data, int width, int height, int channels);
extern void cuda_gaussian_blur(unsigned char* input, unsigned char* output, 
                               int width, int height, int channels, float sigma);
extern void cuda_sharpening(unsigned char* input, unsigned char* output,
                           int width, int height, int channels);
extern void cuda_edge_detection(unsigned char* input, unsigned char* output,
                                int width, int height, int channels);
extern void cuda_add_noise(unsigned char* data, int width, int height, 
                          int channels, float noise_level);
extern void cuda_downsample(unsigned char* input, unsigned char* output,
                           int width, int height, int new_width, int new_height, int channels);

// OpenMP CPU fallback functions
void convert_to_grayscale_omp(Image* img);
void apply_gaussian_blur_omp(Image* img, float sigma);
void apply_sharpening_filter_omp(Image* img);
void add_gaussian_noise_omp(Image* img, float noise_level);
void apply_edge_detection_omp(Image* img);
Image* downsample_image_omp(Image* img, int scale_factor);

// Hybrid processing functions
void process_image_hybrid(Image* img, const char* output_prefix, int use_gpu);

// Utility functions
double get_time();
void print_processing_info(const char* operation, double time_taken, int use_gpu);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage: %s <input_image> <output_prefix> [gpu]\n", argv[0]);
        printf("  gpu: 1 for GPU processing, 0 for CPU only (default: 1)\n");
        printf("\nExamples:\n");
        printf("  %s input.jpg output 1    # Use GPU\n", argv[0]);
        printf("  %s input.jpg output 0    # CPU only\n", argv[0]);
        return 1;
    }

    // Create output directory
    #ifdef _WIN32
        system("mkdir \"hybrid_output\" 2>nul");
    #else
        system("mkdir -p \"hybrid_output\"");
    #endif

    const char* input_filename = argv[1];
    const char* output_prefix = argv[2];
    int use_gpu = (argc > 3) ? atoi(argv[3]) : 1;
    
    printf("=== Hybrid OpenMP + CUDA Image Processing ===\n");
    printf("Based on: Enhancing Heterogeneous Computing (Yu et al., ICPP 2024)\n");
    printf("OpenMP Threads Available: %d\n", omp_get_max_threads());
    printf("Processing Mode: %s\n", use_gpu ? "GPU (CUDA)" : "CPU (OpenMP)");
    
    printf("\n--- Loading Image ---\n");
    printf("Input: %s\n", input_filename);
    
    Image* img = load_image(input_filename);
    if (!img) {
        printf("Error: Could not load image %s\n", input_filename);
        return 1;
    }
    
    printf("Image loaded: %dx%d pixels, %d channels\n", 
           img->width, img->height, img->channels);
    
    // Process with hybrid approach
    double total_start = get_time();
    process_image_hybrid(img, output_prefix, use_gpu);
    double total_end = get_time();
    
    printf("\n=== Performance Summary ===\n");
    printf("Total processing time: %.4f seconds\n", total_end - total_start);
    printf("Image dimensions: %dx%d = %d pixels\n", 
           img->width, img->height, img->width * img->height);
    printf("Throughput: %.2f Mpixels/sec\n", 
           (img->width * img->height / 1e6) / (total_end - total_start));
    
    free_image(img);
    
    printf("\nProcessing complete! Output files saved in 'hybrid_output/' directory.\n");
    return 0;
}

// ==================== IMAGE LOADING/SAVING ====================

Image* load_image(const char* filename) {
    Image* img = malloc(sizeof(Image));
    if (!img) return NULL;
    
    img->data = stbi_load(filename, &img->width, &img->height, &img->channels, 0);
    if (!img->data) {
        free(img);
        return NULL;
    }
    
    return img;
}

void free_image(Image* img) {
    if (img) {
        if (img->data) stbi_image_free(img->data);
        free(img);
    }
}

void save_image(const char* filename, Image* img) {
    stbi_write_png(filename, img->width, img->height, img->channels, img->data, 0);
}

Image* create_image(int width, int height, int channels) {
    Image* img = malloc(sizeof(Image));
    if (!img) return NULL;
    
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = malloc(width * height * channels);
    
    if (!img->data) {
        free(img);
        return NULL;
    }
    
    return img;
}

// ==================== HYBRID PROCESSING ====================

void process_image_hybrid(Image* img, const char* output_prefix, int use_gpu) {
    char filename[512];
    double start, end;
    
    printf("\n=== Starting Hybrid Processing ===\n");
    printf("OpenMP manages task distribution\n");
    printf("%s handles individual operations\n\n", use_gpu ? "CUDA" : "OpenMP");
    
    // OpenMP task-level parallelism for independent operations
    #pragma omp parallel
    {
        #pragma omp single
        {
            // Task 1: Grayscale conversion
            #pragma omp task
            {
                Image* gray_img = create_image(img->width, img->height, img->channels);
                memcpy(gray_img->data, img->data, img->width * img->height * img->channels);
                
                double task_start = get_time();
                if (use_gpu) {
                    cuda_grayscale(gray_img->data, gray_img->width, gray_img->height, gray_img->channels);
                } else {
                    convert_to_grayscale_omp(gray_img);
                }
                double task_end = get_time();
                
                print_processing_info("Grayscale", task_end - task_start, use_gpu);
                snprintf(filename, sizeof(filename), "hybrid_output/%s_grayscale.png", output_prefix);
                save_image(filename, gray_img);
                free_image(gray_img);
            }
            
            // Task 2: Gaussian Blur
            #pragma omp task
            {
                Image* blur_img = create_image(img->width, img->height, img->channels);
                memcpy(blur_img->data, img->data, img->width * img->height * img->channels);
                
                unsigned char* temp_data = malloc(img->width * img->height * img->channels);
                
                double task_start = get_time();
                if (use_gpu) {
                    cuda_gaussian_blur(blur_img->data, temp_data, blur_img->width, 
                                     blur_img->height, blur_img->channels, 2.0f);
                    memcpy(blur_img->data, temp_data, img->width * img->height * img->channels);
                } else {
                    apply_gaussian_blur_omp(blur_img, 2.0f);
                }
                double task_end = get_time();
                
                print_processing_info("Gaussian Blur", task_end - task_start, use_gpu);
                snprintf(filename, sizeof(filename), "hybrid_output/%s_blur.png", output_prefix);
                save_image(filename, blur_img);
                free(temp_data);
                free_image(blur_img);
            }
            
            // Task 3: Sharpening
            #pragma omp task
            {
                Image* sharp_img = create_image(img->width, img->height, img->channels);
                memcpy(sharp_img->data, img->data, img->width * img->height * img->channels);
                
                unsigned char* temp_data = malloc(img->width * img->height * img->channels);
                
                double task_start = get_time();
                if (use_gpu) {
                    cuda_sharpening(sharp_img->data, temp_data, sharp_img->width, 
                                  sharp_img->height, sharp_img->channels);
                    memcpy(sharp_img->data, temp_data, img->width * img->height * img->channels);
                } else {
                    apply_sharpening_filter_omp(sharp_img);
                }
                double task_end = get_time();
                
                print_processing_info("Sharpening", task_end - task_start, use_gpu);
                snprintf(filename, sizeof(filename), "hybrid_output/%s_sharp.png", output_prefix);
                save_image(filename, sharp_img);
                free(temp_data);
                free_image(sharp_img);
            }
            
            // Task 4: Edge Detection
            #pragma omp task
            {
                Image* edge_img = create_image(img->width, img->height, img->channels);
                memcpy(edge_img->data, img->data, img->width * img->height * img->channels);
                
                unsigned char* temp_data = malloc(img->width * img->height * img->channels);
                
                double task_start = get_time();
                if (use_gpu) {
                    cuda_edge_detection(edge_img->data, temp_data, edge_img->width, 
                                      edge_img->height, edge_img->channels);
                    memcpy(edge_img->data, temp_data, img->width * img->height * img->channels);
                } else {
                    apply_edge_detection_omp(edge_img);
                }
                double task_end = get_time();
                
                print_processing_info("Edge Detection", task_end - task_start, use_gpu);
                snprintf(filename, sizeof(filename), "hybrid_output/%s_edges.png", output_prefix);
                save_image(filename, edge_img);
                free(temp_data);
                free_image(edge_img);
            }
            
            // Task 5: Noise Addition
            #pragma omp task
            {
                Image* noise_img = create_image(img->width, img->height, img->channels);
                memcpy(noise_img->data, img->data, img->width * img->height * img->channels);
                
                double task_start = get_time();
                if (use_gpu) {
                    cuda_add_noise(noise_img->data, noise_img->width, noise_img->height, 
                                 noise_img->channels, 25.0f);
                } else {
                    add_gaussian_noise_omp(noise_img, 25.0f);
                }
                double task_end = get_time();
                
                print_processing_info("Noise Addition", task_end - task_start, use_gpu);
                snprintf(filename, sizeof(filename), "hybrid_output/%s_noise.png", output_prefix);
                save_image(filename, noise_img);
                free_image(noise_img);
            }
            
            // Task 6: Multi-level Downsampling (dependent task chain)
            #pragma omp task
            {
                printf("\n--- Multi-level Compression Pipeline ---\n");
                Image* current = create_image(img->width, img->height, img->channels);
                memcpy(current->data, img->data, img->width * img->height * img->channels);
                
                for (int level = 1; level <= 3; level++) {
                    int new_width = current->width / 2;
                    int new_height = current->height / 2;
                    
                    if (new_width < 16 || new_height < 16) break;
                    
                    Image* downsampled = create_image(new_width, new_height, current->channels);
                    
                    double task_start = get_time();
                    if (use_gpu) {
                        cuda_downsample(current->data, downsampled->data, 
                                      current->width, current->height,
                                      new_width, new_height, current->channels);
                    } else {
                        Image* temp = downsample_image_omp(current, 2);
                        memcpy(downsampled->data, temp->data, 
                              new_width * new_height * current->channels);
                        free_image(temp);
                    }
                    double task_end = get_time();
                    
                    char level_name[64];
                    snprintf(level_name, sizeof(level_name), "Downsample Level %d", level);
                    print_processing_info(level_name, task_end - task_start, use_gpu);
                    
                    snprintf(filename, sizeof(filename), 
                            "hybrid_output/%s_downsample_level%d.png", output_prefix, level);
                    save_image(filename, downsampled);
                    
                    free_image(current);
                    current = downsampled;
                }
                
                free_image(current);
            }
            
            // Wait for all tasks to complete
            #pragma omp taskwait
        }
    }
}

// ==================== OPENMP CPU FALLBACK FUNCTIONS ====================

void convert_to_grayscale_omp(Image* img) {
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int idx = (y * img->width + x) * img->channels;
            if (img->channels >= 3) {
                float gray = 0.299f * img->data[idx] + 
                            0.587f * img->data[idx + 1] + 
                            0.114f * img->data[idx + 2];
                img->data[idx] = img->data[idx + 1] = img->data[idx + 2] = (unsigned char)gray;
            }
        }
    }
}

void apply_gaussian_blur_omp(Image* img, float sigma) {
    int kernel_size = (int)(6 * sigma + 1);
    if (kernel_size % 2 == 0) kernel_size++;
    int kernel_radius = kernel_size / 2;
    
    float* kernel = malloc(kernel_size * kernel_size * sizeof(float));
    float kernel_sum = 0.0f;
    
    for (int y = -kernel_radius; y <= kernel_radius; y++) {
        for (int x = -kernel_radius; x <= kernel_radius; x++) {
            float value = exp(-(x*x + y*y) / (2 * sigma * sigma));
            kernel[(y + kernel_radius) * kernel_size + (x + kernel_radius)] = value;
            kernel_sum += value;
        }
    }
    
    for (int i = 0; i < kernel_size * kernel_size; i++) {
        kernel[i] /= kernel_sum;
    }
    
    unsigned char* temp_data = malloc(img->width * img->height * img->channels);
    
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                float sum = 0.0f;
                for (int ky = -kernel_radius; ky <= kernel_radius; ky++) {
                    for (int kx = -kernel_radius; kx <= kernel_radius; kx++) {
                        int py = y + ky;
                        int px = x + kx;
                        py = py < 0 ? 0 : (py >= img->height ? img->height - 1 : py);
                        px = px < 0 ? 0 : (px >= img->width ? img->width - 1 : px);
                        int pixel_idx = (py * img->width + px) * img->channels + c;
                        int kernel_idx = (ky + kernel_radius) * kernel_size + (kx + kernel_radius);
                        sum += img->data[pixel_idx] * kernel[kernel_idx];
                    }
                }
                temp_data[(y * img->width + x) * img->channels + c] = (unsigned char)(sum + 0.5f);
            }
        }
    }
    
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(kernel);
    free(temp_data);
}

void apply_sharpening_filter_omp(Image* img) {
    float kernel[3][3] = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };
    
    unsigned char* temp_data = malloc(img->width * img->height * img->channels);
    
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                float sum = 0.0f;
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int py = y + ky;
                        int px = x + kx;
                        py = py < 0 ? 0 : (py >= img->height ? img->height - 1 : py);
                        px = px < 0 ? 0 : (px >= img->width ? img->width - 1 : px);
                        int pixel_idx = (py * img->width + px) * img->channels + c;
                        sum += img->data[pixel_idx] * kernel[ky + 1][kx + 1];
                    }
                }
                sum = sum < 0 ? 0 : (sum > 255 ? 255 : sum);
                temp_data[(y * img->width + x) * img->channels + c] = (unsigned char)sum;
            }
        }
    }
    
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(temp_data);
}

void add_gaussian_noise_omp(Image* img, float noise_level) {
    #pragma omp parallel
    {
        unsigned int seed = (unsigned int)(time(NULL) + omp_get_thread_num() * 1337);
        
        #pragma omp for collapse(2) schedule(dynamic)
        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) {
                for (int c = 0; c < img->channels; c++) {
                    int idx = (y * img->width + x) * img->channels + c;
                    
                    seed = seed * 1103515245 + 12345;
                    float u = ((float)(seed & 0x7FFFFFFF) + 1.0f) / 2147483648.0f;
                    seed = seed * 1103515245 + 12345;
                    float v = ((float)(seed & 0x7FFFFFFF) + 1.0f) / 2147483648.0f;
                    
                    float mag = noise_level * sqrt(-2.0f * log(u));
                    float noise = mag * cos(2.0f * M_PI * v);
                    
                    float new_value = img->data[idx] + noise;
                    img->data[idx] = (unsigned char)(new_value < 0 ? 0 : 
                                                    (new_value > 255 ? 255 : new_value));
                }
            }
        }
    }
}

void apply_edge_detection_omp(Image* img) {
    float sobel_x[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    float sobel_y[3][3] = {{-1, -2, -1}, {0, 0, 0}, {1, 2, 1}};
    
    unsigned char* temp_data = malloc(img->width * img->height * img->channels);
    
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                float gx = 0.0f, gy = 0.0f;
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int py = y + ky;
                        int px = x + kx;
                        py = py < 0 ? 0 : (py >= img->height ? img->height - 1 : py);
                        px = px < 0 ? 0 : (px >= img->width ? img->width - 1 : px);
                        int pixel_idx = (py * img->width + px) * img->channels + c;
                        float pixel_value = img->data[pixel_idx];
                        gx += pixel_value * sobel_x[ky + 1][kx + 1];
                        gy += pixel_value * sobel_y[ky + 1][kx + 1];
                    }
                }
                float magnitude = sqrt(gx * gx + gy * gy);
                magnitude = magnitude > 255 ? 255 : magnitude;
                temp_data[(y * img->width + x) * img->channels + c] = (unsigned char)magnitude;
            }
        }
    }
    
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(temp_data);
}

Image* downsample_image_omp(Image* img, int scale_factor) {
    int new_width = img->width / scale_factor;
    int new_height = img->height / scale_factor;
    
    if (new_width < 1) new_width = 1;
    if (new_height < 1) new_height = 1;
    
    Image* downsampled = create_image(new_width, new_height, img->channels);
    if (!downsampled) return NULL;
    
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < new_height; y++) {
        for (int x = 0; x < new_width; x++) {
            for (int c = 0; c < img->channels; c++) {
                float sum = 0.0f;
                int count = 0;
                for (int dy = 0; dy < scale_factor; dy++) {
                    for (int dx = 0; dx < scale_factor; dx++) {
                        int src_y = y * scale_factor + dy;
                        int src_x = x * scale_factor + dx;
                        if (src_y < img->height && src_x < img->width) {
                            int src_idx = (src_y * img->width + src_x) * img->channels + c;
                            sum += img->data[src_idx];
                            count++;
                        }
                    }
                }
                int dst_idx = (y * new_width + x) * img->channels + c;
                downsampled->data[dst_idx] = (unsigned char)(sum / count + 0.5f);
            }
        }
    }
    
    return downsampled;
}

// ==================== UTILITY FUNCTIONS ====================

double get_time() {
    return omp_get_wtime();
}

void print_processing_info(const char* operation, double time_taken, int use_gpu) {
    #pragma omp critical
    {
        printf("[%s] %s: %.4f seconds (Thread %d)\n", 
               use_gpu ? "GPU" : "CPU", operation, time_taken, omp_get_thread_num());
    }
}