//parallel_image_processing.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <omp.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image.h"
#include "stb_image_write.h"

typedef struct {
    unsigned char *data;
    int width;
    int height;
    int channels;
} Image;

// Function prototypes
Image* load_image(const char* filename);
void free_image(Image* img);
void save_image(const char* filename, Image* img);
Image* create_image(int width, int height, int channels);

// Image processing functions
void convert_to_grayscale(Image* img);
void apply_gaussian_blur(Image* img, float sigma);
void apply_sharpening_filter(Image* img);
void add_gaussian_noise(Image* img, float noise_level);
void apply_edge_detection(Image* img);
void apply_gaussian_prefilter(Image* img, float sigma);
Image* downsample_image(Image* img, int scale_factor);
void compress_image_multilevel(Image* img, const char* output_prefix, int levels);

// Utility functions
double get_time();
void print_processing_info(const char* operation, double time_taken);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_image> <output_prefix>\n", argv[0]);
        return 1;
    }

    // user input for thread no.
    int num_threads;
    printf("Enter the number of threads to use (1-%d): ", omp_get_max_threads());
    scanf("%d", &num_threads);
    
    if (num_threads < 1 || num_threads > omp_get_max_threads()) {
        printf("Invalid number of threads. Using default: %d\n", omp_get_max_threads());
        num_threads = omp_get_max_threads();
    }

    // set number of threads
    omp_set_num_threads(num_threads);

    const char* input_filename = argv[1];
    const char* output_prefix = argv[2];
    
    // Create output directory
    #ifdef _WIN32
        system("mkdir \"parallel output images\" 2>nul");
    #else
        system("mkdir -p \"parallel output images\"");
    #endif

    printf("=== Parallel Image Processing with OpenMP ===\n");
    printf("Loading image: %s\n", input_filename);

    Image* original = load_image(input_filename);
    if (!original) {
        printf("Error: Could not load image %s\n", input_filename);
        return 1;
    }

    printf("Image loaded: %dx%d pixels, %d channels\n", 
           original->width, original->height, original->channels);

    double total_start_time = get_time();

    #pragma omp parallel
    {
        #pragma omp single
        printf("Running with %d threads\n", omp_get_num_threads());
    }

    printf("\n--- Processing Operations ---\n");

    // Grayscale
    Image* grayscale_img = create_image(original->width, original->height, original->channels);
    memcpy(grayscale_img->data, original->data, original->width * original->height * original->channels);

    double start_time = get_time();
    convert_to_grayscale(grayscale_img);
    double end_time = get_time();
    print_processing_info("Grayscale Conversion", end_time - start_time);

    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "parallel output images/%s_grayscale.png", output_prefix);
    save_image(output_filename, grayscale_img);

    // Gaussian Blur
    Image* blur_img = create_image(original->width, original->height, original->channels);
    memcpy(blur_img->data, original->data, original->width * original->height * original->channels);

    start_time = get_time();
    apply_gaussian_blur(blur_img, 2.0f);
    end_time = get_time();
    print_processing_info("Gaussian Blur", end_time - start_time);

    snprintf(output_filename, sizeof(output_filename), "parallel output images/%s_blur.png", output_prefix);
    save_image(output_filename, blur_img);

    // Sharpen
    Image* sharp_img = create_image(original->width, original->height, original->channels);
    memcpy(sharp_img->data, original->data, original->width * original->height * original->channels);

    start_time = get_time();
    apply_sharpening_filter(sharp_img);
    end_time = get_time();
    print_processing_info("Sharpening Filter", end_time - start_time);

    snprintf(output_filename, sizeof(output_filename), "parallel output images/%s_sharp.png", output_prefix);
    save_image(output_filename, sharp_img);

    // Add Noise
    Image* noise_img = create_image(original->width, original->height, original->channels);
    memcpy(noise_img->data, original->data, original->width * original->height * original->channels);

    start_time = get_time();
    add_gaussian_noise(noise_img, 25.0f);
    end_time = get_time();
    print_processing_info("Noise Addition", end_time - start_time);

    snprintf(output_filename, sizeof(output_filename), "parallel output images/%s_noise.png", output_prefix);
    save_image(output_filename, noise_img);

    // Edge Detection
    Image* edge_img = create_image(original->width, original->height, original->channels);
    memcpy(edge_img->data, original->data, original->width * original->height * original->channels);

    start_time = get_time();
    apply_edge_detection(edge_img);
    end_time = get_time();
    print_processing_info("Edge Detection", end_time - start_time);

    snprintf(output_filename, sizeof(output_filename), "parallel output images/%s_edges.png", output_prefix);
    save_image(output_filename, edge_img);

    // Multi-level Image Compression
    Image* compress_img = create_image(original->width, original->height, original->channels);
    memcpy(compress_img->data, original->data, 
           original->width * original->height * original->channels);
    
    printf("\n--- Multi-level Image Compression ---\n");
    start_time = get_time();
    compress_image_multilevel(compress_img, output_prefix, 3); // 3 compression levels
    end_time = get_time();
    print_processing_info("Multi-level Compression", end_time - start_time);

    double total_end_time = get_time();
    double total_time = total_end_time - total_start_time;

    printf("\n=== Performance Summary ===\n");
    printf("Total processing time: %.4f seconds\n", total_time);
    printf("Image dimensions: %dx%d = %d pixels\n", 
           original->width, original->height, original->width * original->height);

    // *** Standard result line for parsing in Python/matplotlib ***
    // format: RESULT,version,label,threads,time
    printf("RESULT,par,%s,%d,%.6f\n", output_prefix, num_threads, total_time);

    free_image(original);
    free_image(grayscale_img);
    free_image(blur_img);
    free_image(sharp_img);
    free_image(noise_img);
    free_image(edge_img);
    free_image(compress_img);

    printf("Processing complete. Output files saved with prefix: %s\n", output_prefix);
    return 0;
}

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

void convert_to_grayscale(Image* img) {
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int idx = (y * img->width + x) * img->channels;
            if (img->channels >= 3) {
                float gray = 0.299f * img->data[idx] + 0.587f * img->data[idx + 1] + 0.114f * img->data[idx + 2];
                img->data[idx] = img->data[idx + 1] = img->data[idx + 2] = (unsigned char)gray;
            }
        }
    }
}

void apply_gaussian_blur(Image* img, float sigma) {
    int kernel_size = (int)(6 * sigma + 1);
    if (kernel_size % 2 == 0) kernel_size++;
    int kernel_radius = kernel_size / 2;

    float* kernel = malloc(kernel_size * kernel_size * sizeof(float));
    float kernel_sum = 0.0f;

    #pragma omp parallel for collapse(2) reduction(+:kernel_sum)
    for (int y = -kernel_radius; y <= kernel_radius; y++) {
        for (int x = -kernel_radius; x <= kernel_radius; x++) {
            float value = exp(-(x * x + y * y) / (2 * sigma * sigma));
            kernel[(y + kernel_radius) * kernel_size + (x + kernel_radius)] = value;
            kernel_sum += value;
        }
    }

    #pragma omp parallel for
    for (int i = 0; i < kernel_size * kernel_size; i++) {
        kernel[i] /= kernel_sum;
    }

    unsigned char* temp_data = malloc(img->width * img->height * img->channels);

    #pragma omp parallel for collapse(2)
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

void apply_sharpening_filter(Image* img) {
    float kernel[3][3] = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };

    unsigned char* temp_data = malloc(img->width * img->height * img->channels);

    #pragma omp parallel for collapse(2)
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

void add_gaussian_noise(Image* img, float noise_level) {
    #pragma omp parallel
    {
        unsigned int seed = (unsigned int)(time(NULL) + omp_get_thread_num() * 1337);

        #pragma omp for collapse(2)
        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) {
                for (int c = 0; c < img->channels; c++) {
                    int idx = (y * img->width + x) * img->channels + c;

                    seed = seed * 1103515245 + 12345;
                    float u = ((float)(seed & 0x7FFFFFFF) + 1.0f) / 2147483648.0f;

                    seed = seed * 1103515245 + 12345;
                    float v = ((float)(seed & 0x7FFFFFFF) + 1.0f) / 2147483648.0f;

                    float mag = noise_level * sqrtf(-2.0f * logf(u));
                    float noise = mag * cosf(2.0f * M_PI * v);

                    float new_value = img->data[idx] + noise;
                    img->data[idx] = (unsigned char)(new_value < 0 ? 0 : (new_value > 255 ? 255 : new_value));
                }
            }
        }
    }
}

void apply_edge_detection(Image* img) {
    float sobel_x[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    float sobel_y[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    unsigned char* temp_data = malloc(img->width * img->height * img->channels);

    #pragma omp parallel for collapse(2)
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
                float magnitude = sqrtf(gx * gx + gy * gy);
                magnitude = magnitude > 255 ? 255 : magnitude;
                temp_data[(y * img->width + x) * img->channels + c] = (unsigned char)magnitude;
            }
        }
    }

    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(temp_data);
}

void apply_gaussian_prefilter(Image* img, float sigma) {
    apply_gaussian_blur(img, sigma);
}

Image* downsample_image(Image* img, int scale_factor) {
    int new_width = img->width / scale_factor;
    int new_height = img->height / scale_factor;
    
    if (new_width < 1) new_width = 1;
    if (new_height < 1) new_height = 1;
    
    Image* downsampled = create_image(new_width, new_height, img->channels);
    if (!downsampled) return NULL;
    
    #pragma omp parallel
    {
        #pragma omp single
        printf("  Downsampling from %dx%d to %dx%d (factor: %d)\n", 
               img->width, img->height, new_width, new_height, scale_factor);
    }
    
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

void compress_image_multilevel(Image* img, const char* output_prefix, int levels) {
    printf("Starting %d-level image compression...\n", levels);
    
    Image* current_img = create_image(img->width, img->height, img->channels);
    memcpy(current_img->data, img->data, img->width * img->height * img->channels);
    
    char filename[256];
    
    for (int level = 1; level <= levels; level++) {
        printf("Processing compression level %d/%d:\n", level, levels);
        
        double level_start = get_time();
        
        double prefilter_start = get_time();
        float sigma = 0.8f * level;
        apply_gaussian_prefilter(current_img, sigma);
        double prefilter_end = get_time();
        printf("  Pre-filter (σ=%.1f): %.4f seconds\n", sigma, prefilter_end - prefilter_start);
        
        double downsample_start = get_time();
        Image* downsampled = downsample_image(current_img, 2);
        double downsample_end = get_time();
        printf("  Downsampling: %.4f seconds\n", downsample_end - downsample_start);
        
        if (!downsampled) {
            printf("  Error: Failed to downsample at level %d\n", level);
            break;
        }
        
        snprintf(filename, sizeof(filename), "parallel output images/%s_compressed_level_%d.png", output_prefix, level);
        save_image(filename, downsampled);
        
        free_image(current_img);
        current_img = downsampled;
        
        double level_end = get_time();
        printf("  Level %d completed: %.4f seconds (Size: %dx%d)\n", 
               level, level_end - level_start, current_img->width, current_img->height);
        
        if (current_img->width < 16 || current_img->height < 16) {
            printf("  Stopping compression - image too small\n");
            break;
        }
    }
    
    snprintf(filename, sizeof(filename), "parallel output images/%s_final_compressed.png", output_prefix);
    save_image(filename, current_img);
    
    printf("Final compressed size: %dx%d pixels\n", current_img->width, current_img->height);
    free_image(current_img);
}

double get_time() {
    // Use OpenMP wall-clock timer (better for parallel)
    return omp_get_wtime();
}

void print_processing_info(const char* operation, double time_taken) {
    #pragma omp critical
    printf("%s: %.4f seconds\n", operation, time_taken);
}
