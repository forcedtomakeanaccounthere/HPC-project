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

// OpenMP parallelized functions (CPU)
void convert_to_grayscale_omp(Image* img);
void apply_gaussian_blur_omp(Image* img, float sigma);
void apply_sharpening_filter_omp(Image* img);
void add_gaussian_noise_omp(Image* img, float noise_level);
void apply_edge_detection_omp(Image* img);

// CUDA kernel declarations (GPU) - Only declare if compiling with CUDA
#ifdef USE_CUDA
void launch_grayscale_kernel(unsigned char* d_data, int width, int height, int channels);
void launch_gaussian_blur_kernel(unsigned char* d_input, unsigned char* d_output, 
                                 int width, int height, int channels, float sigma);
void launch_sharpening_kernel(unsigned char* d_input, unsigned char* d_output,
                              int width, int height, int channels);
void launch_edge_detection_kernel(unsigned char* d_input, unsigned char* d_output,
                                  int width, int height, int channels);
void launch_downsample_kernel(unsigned char* d_input, unsigned char* d_output,
                              int width, int height, int new_width, int new_height, int channels);
#endif

// Hybrid processing functions
void process_image_hybrid(Image* img, const char* output_prefix, int use_gpu);
void process_video_hybrid(const char* input_pattern, int num_frames, const char* output_prefix, int use_gpu);

// Utility functions
double get_time();
void print_processing_info(const char* operation, double time_taken);

// Video frame structure
typedef struct {
    Image** frames;
    int num_frames;
} VideoFrames;

VideoFrames* load_video_frames(const char* pattern, int num_frames);
void free_video_frames(VideoFrames* video);
void save_video_frames(VideoFrames* video, const char* output_prefix);

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Usage:\n");
        printf("  Image processing: %s <input_image> <output_prefix>\n", argv[0]);
        printf("\nExamples:\n");
        printf("  %s iiit.jpg output\n", argv[0]);
        return 1;
    }

    // Create output directory
    #ifdef _WIN32
        system("mkdir \"hybrid output images\" 2>nul");
    #else
        system("mkdir -p \"hybrid output images\"");
    #endif

    const char* input_filename = argv[1];
    const char* output_prefix = argv[2];
    
    printf("=== Hybrid OpenMP + CUDA Image Processing ===\n");
    printf("OpenMP Threads Available: %d\n", omp_get_max_threads());
    
    printf("\n--- IMAGE PROCESSING MODE ---\n");
    printf("Loading image: %s\n", input_filename);
    
    Image* img = load_image(input_filename);
    if (!img) {
        printf("Error: Could not load image %s\n", input_filename);
        return 1;
    }
    
    printf("Image loaded: %dx%d pixels, %d channels\n", 
           img->width, img->height, img->channels);
    
    // Process with CPU (OpenMP)
    printf("\n=== CPU Processing (OpenMP) ===\n");
    Image* cpu_img = create_image(img->width, img->height, img->channels);
    memcpy(cpu_img->data, img->data, img->width * img->height * img->channels);
    
    double cpu_start = get_time();
    process_image_hybrid(cpu_img, output_prefix, 0);
    double cpu_end = get_time();
    printf("\nTotal CPU processing time: %.4f seconds\n", cpu_end - cpu_start);
    
    printf("\n=== Performance Summary ===\n");
    printf("Total processing time: %.4f seconds\n", cpu_end - cpu_start);
    printf("Image dimensions: %dx%d = %d pixels\n", 
           img->width, img->height, img->width * img->height);
    
    free_image(img);
    free_image(cpu_img);
    
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

// ==================== OPENMP CPU FUNCTIONS ====================

void convert_to_grayscale_omp(Image* img) {
    // HOTSPOT 1: OpenMP parallel pixel processing
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int idx = (y * img->width + x) * img->channels;
            
            if (img->channels >= 3) {
                float gray = 0.299f * img->data[idx] + 
                            0.587f * img->data[idx + 1] + 
                            0.114f * img->data[idx + 2];
                
                img->data[idx] = (unsigned char)gray;
                img->data[idx + 1] = (unsigned char)gray;
                img->data[idx + 2] = (unsigned char)gray;
            }
        }
    }
}

void apply_gaussian_blur_omp(Image* img, float sigma) {
    // HOTSPOT 2: OpenMP parallel convolution
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
    // HOTSPOT 3: OpenMP parallel sharpening
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
    // HOTSPOT 4: OpenMP parallel noise addition
    srand(time(NULL));
    
    #pragma omp parallel for collapse(2) schedule(dynamic)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                int idx = (y * img->width + x) * img->channels + c;
                
                // Box-Muller transform for Gaussian noise
                float u1, u2;
                #pragma omp critical
                {
                    u1 = ((float)rand() / RAND_MAX);
                    u2 = ((float)rand() / RAND_MAX);
                }
                float noise = noise_level * sqrt(-2.0f * log(u1 + 0.0001f)) * cos(2.0f * M_PI * u2);
                
                float new_value = img->data[idx] + noise;
                img->data[idx] = (unsigned char)(new_value < 0 ? 0 : (new_value > 255 ? 255 : new_value));
            }
        }
    }
}

void apply_edge_detection_omp(Image* img) {
    // HOTSPOT 5: OpenMP parallel edge detection
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

// ==================== HYBRID PROCESSING ====================

void process_image_hybrid(Image* img, const char* output_prefix, int use_gpu) {
    char filename[512];
    double start, end;
    
    if (use_gpu) {
        printf("Using GPU (CUDA) for processing...\n");
        // GPU processing will use CUDA kernels
        // This requires compiling with nvcc - see separate CUDA file
        printf("Note: GPU processing requires CUDA compilation (see .cu file)\n");
        printf("For now, falling back to CPU with OpenMP\n");
        use_gpu = 0;
    }
    
    // 1. Grayscale
    Image* gray_img = create_image(img->width, img->height, img->channels);
    memcpy(gray_img->data, img->data, img->width * img->height * img->channels);
    start = get_time();
    convert_to_grayscale_omp(gray_img);
    end = get_time();
    print_processing_info("Grayscale", end - start);
    snprintf(filename, sizeof(filename), "hybrid output images/%s_grayscale.png", output_prefix);
    save_image(filename, gray_img);
    free_image(gray_img);
    
    // 2. Gaussian Blur
    Image* blur_img = create_image(img->width, img->height, img->channels);
    memcpy(blur_img->data, img->data, img->width * img->height * img->channels);
    start = get_time();
    apply_gaussian_blur_omp(blur_img, 2.0f);
    end = get_time();
    print_processing_info("Gaussian Blur", end - start);
    snprintf(filename, sizeof(filename), "hybrid output images/%s_blur.png", output_prefix);
    save_image(filename, blur_img);
    free_image(blur_img);
    
    // 3. Sharpening
    Image* sharp_img = create_image(img->width, img->height, img->channels);
    memcpy(sharp_img->data, img->data, img->width * img->height * img->channels);
    start = get_time();
    apply_sharpening_filter_omp(sharp_img);
    end = get_time();
    print_processing_info("Sharpening", end - start);
    snprintf(filename, sizeof(filename), "hybrid output images/%s_sharp.png", output_prefix);
    save_image(filename, sharp_img);
    free_image(sharp_img);
    
    // 4. Noise Addition
    Image* noise_img = create_image(img->width, img->height, img->channels);
    memcpy(noise_img->data, img->data, img->width * img->height * img->channels);
    start = get_time();
    add_gaussian_noise_omp(noise_img, 25.0f);
    end = get_time();
    print_processing_info("Noise Addition", end - start);
    snprintf(filename, sizeof(filename), "hybrid output images/%s_noise.png", output_prefix);
    save_image(filename, noise_img);
    free_image(noise_img);
    
    // 5. Edge Detection
    Image* edge_img = create_image(img->width, img->height, img->channels);
    memcpy(edge_img->data, img->data, img->width * img->height * img->channels);
    start = get_time();
    apply_edge_detection_omp(edge_img);
    end = get_time();
    print_processing_info("Edge Detection", end - start);
    snprintf(filename, sizeof(filename), "hybrid output images/%s_edges.png", output_prefix);
    save_image(filename, edge_img);
    free_image(edge_img);
}

// ==================== VIDEO PROCESSING ====================

VideoFrames* load_video_frames(const char* pattern, int num_frames) {
    VideoFrames* video = malloc(sizeof(VideoFrames));
    video->frames = malloc(sizeof(Image*) * num_frames);
    video->num_frames = num_frames;
    
    char filename[512];
    int loaded = 0;
    
    for (int i = 0; i < num_frames; i++) {
        snprintf(filename, sizeof(filename), pattern, i);
        video->frames[i] = load_image(filename);
        if (video->frames[i]) {
            loaded++;
        } else {
            printf("Warning: Could not load frame %d from %s\n", i, filename);
        }
    }
    
    printf("Loaded %d/%d frames\n", loaded, num_frames);
    return video;
}

void free_video_frames(VideoFrames* video) {
    if (video) {
        for (int i = 0; i < video->num_frames; i++) {
            if (video->frames[i]) free_image(video->frames[i]);
        }
        free(video->frames);
        free(video);
    }
}

void save_video_frames(VideoFrames* video, const char* output_prefix) {
    char filename[512];
    for (int i = 0; i < video->num_frames; i++) {
        if (video->frames[i]) {
            snprintf(filename, sizeof(filename), "hybrid output images/%s_frame_%04d.png", output_prefix, i);
            save_image(filename, video->frames[i]);
        }
    }
}

void process_video_hybrid(const char* input_pattern, int num_frames, const char* output_prefix, int use_gpu) {
    VideoFrames* video = load_video_frames(input_pattern, num_frames);
    
    if (use_gpu) {
        printf("Processing video with GPU (CUDA Graph API)...\n");
        // GPU video processing with CUDA Graph
        // Each frame processed as a node in CUDA Graph
        printf("Note: GPU processing requires CUDA compilation\n");
    } else {
        printf("Processing video with CPU (OpenMP)...\n");
        
        // HOTSPOT 6: Parallel frame processing with OpenMP taskgraph concept
        #pragma omp parallel for schedule(dynamic, 1)
        for (int i = 0; i < video->num_frames; i++) {
            if (video->frames[i]) {
                // Apply grayscale to each frame
                convert_to_grayscale_omp(video->frames[i]);
                
                if (i % 10 == 0) {
                    printf("Processed frame %d/%d\n", i, num_frames);
                }
            }
        }
    }
    
    save_video_frames(video, output_prefix);
    free_video_frames(video);
}

// ==================== UTILITY FUNCTIONS ====================

double get_time() {
    return omp_get_wtime();
}

void print_processing_info(const char* operation, double time_taken) {
    printf("%s: %.4f seconds\n", operation, time_taken);
}