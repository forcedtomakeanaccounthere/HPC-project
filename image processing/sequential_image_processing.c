#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

// download these header files:
// stb_image.h and stb_image_write.h from https://github.com/nothings/stb
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

// Utility functions
double get_time();
void print_processing_info(const char* operation, double time_taken);

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("Usage: %s <input_image> <output_prefix>\n", argv[0]);
        return 1;
    }

    const char* input_filename = argv[1];
    const char* output_prefix = argv[2];
    
    printf("=== Sequential Image Processing Implementation ===\n");
    printf("Loading image: %s\n", input_filename);
    
    // Load the input image
    Image* original = load_image(input_filename);
    if (!original) {
        printf("Error: Could not load image %s\n", input_filename);
        return 1;
    }
    
    printf("Image loaded: %dx%d pixels, %d channels\n", 
           original->width, original->height, original->channels);
    
    double total_start_time = get_time();
    
    // 1. Grayscale conversion
    printf("\nProcessing Operations....................\n");
    Image* grayscale_img = create_image(original->width, original->height, original->channels);
    memcpy(grayscale_img->data, original->data, 
           original->width * original->height * original->channels);
    
    double start_time = get_time();
    convert_to_grayscale(grayscale_img);
    double end_time = get_time();
    print_processing_info("Grayscale Conversion", end_time - start_time);
    
    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "%s_grayscale.png", output_prefix);
    save_image(output_filename, grayscale_img);
    
    // 2. Gaussian Blur
    Image* blur_img = create_image(original->width, original->height, original->channels);
    memcpy(blur_img->data, original->data, 
           original->width * original->height * original->channels);
    
    start_time = get_time();
    apply_gaussian_blur(blur_img, 2.0f);
    end_time = get_time();
    print_processing_info("Gaussian Blur", end_time - start_time);
    
    snprintf(output_filename, sizeof(output_filename), "%s_blur.png", output_prefix);
    save_image(output_filename, blur_img);
    
    // 3. Sharpening Filter
    Image* sharp_img = create_image(original->width, original->height, original->channels);
    memcpy(sharp_img->data, original->data, 
           original->width * original->height * original->channels);
    
    start_time = get_time();
    apply_sharpening_filter(sharp_img);
    end_time = get_time();
    print_processing_info("Sharpening Filter", end_time - start_time);
    
    snprintf(output_filename, sizeof(output_filename), "%s_sharp.png", output_prefix);
    save_image(output_filename, sharp_img);
    
    // 4. Add Gaussian Noise
    Image* noise_img = create_image(original->width, original->height, original->channels);
    memcpy(noise_img->data, original->data, 
           original->width * original->height * original->channels);
    
    start_time = get_time();
    add_gaussian_noise(noise_img, 25.0f);
    end_time = get_time();
    print_processing_info("Noise Addition", end_time - start_time);
    
    snprintf(output_filename, sizeof(output_filename), "%s_noise.png", output_prefix);
    save_image(output_filename, noise_img);
    
    // 5. Edge Detection (Sobel operator)
    Image* edge_img = create_image(original->width, original->height, original->channels);
    memcpy(edge_img->data, original->data, 
           original->width * original->height * original->channels);
    
    start_time = get_time();
    apply_edge_detection(edge_img);
    end_time = get_time();
    print_processing_info("Edge Detection", end_time - start_time);
    
    snprintf(output_filename, sizeof(output_filename), "%s_edges.png", output_prefix);
    save_image(output_filename, edge_img);
    
    double total_end_time = get_time();
    
    printf("\n=== Performance Summary ===\n");
    printf("Total processing time: %.4f seconds\n", total_end_time - total_start_time);
    printf("Image dimensions: %dx%d = %d pixels\n", 
           original->width, original->height, original->width * original->height);
    
    // Cleanup
    free_image(original);
    free_image(grayscale_img);
    free_image(blur_img);
    free_image(sharp_img);
    free_image(noise_img);
    free_image(edge_img);
    
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
    // HOTSPOT 1: Pixel-wise operation - highly parallelizable
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int idx = (y * img->width + x) * img->channels;
            
            if (img->channels >= 3) {
                // Standard grayscale conversion weights
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

void apply_gaussian_blur(Image* img, float sigma) {
    // HOTSPOT 2: Convolution operation - computationally intensive
    int kernel_size = (int)(6 * sigma + 1);
    if (kernel_size % 2 == 0) kernel_size++;
    int kernel_radius = kernel_size / 2;
    
    // Create Gaussian kernel
    float* kernel = malloc(kernel_size * kernel_size * sizeof(float));
    float kernel_sum = 0.0f;
    
    for (int y = -kernel_radius; y <= kernel_radius; y++) {
        for (int x = -kernel_radius; x <= kernel_radius; x++) {
            float value = exp(-(x*x + y*y) / (2 * sigma * sigma));
            kernel[(y + kernel_radius) * kernel_size + (x + kernel_radius)] = value;
            kernel_sum += value;
        }
    }
    
    // Normalize kernel
    for (int i = 0; i < kernel_size * kernel_size; i++) {
        kernel[i] /= kernel_sum;
    }
    
    // Create temporary image for output
    unsigned char* temp_data = malloc(img->width * img->height * img->channels);
    
    // Apply convolution
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                float sum = 0.0f;
                
                // Convolve with kernel
                for (int ky = -kernel_radius; ky <= kernel_radius; ky++) {
                    for (int kx = -kernel_radius; kx <= kernel_radius; kx++) {
                        int py = y + ky;
                        int px = x + kx;
                        
                        // Handle boundary conditions (clamp to edge)
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
    
    // Copy result back
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    
    free(kernel);
    free(temp_data);
}

void apply_sharpening_filter(Image* img) {
    // HOTSPOT 3: Convolution with sharpening kernel
    float kernel[3][3] = {
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
    };
    
    unsigned char* temp_data = malloc(img->width * img->height * img->channels);
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                float sum = 0.0f;
                
                for (int ky = -1; ky <= 1; ky++) {
                    for (int kx = -1; kx <= 1; kx++) {
                        int py = y + ky;
                        int px = x + kx;
                        
                        // Clamp to image boundaries
                        py = py < 0 ? 0 : (py >= img->height ? img->height - 1 : py);
                        px = px < 0 ? 0 : (px >= img->width ? img->width - 1 : px);
                        
                        int pixel_idx = (py * img->width + px) * img->channels + c;
                        sum += img->data[pixel_idx] * kernel[ky + 1][kx + 1];
                    }
                }
                
                // Clamp result to valid range
                sum = sum < 0 ? 0 : (sum > 255 ? 255 : sum);
                temp_data[(y * img->width + x) * img->channels + c] = (unsigned char)sum;
            }
        }
    }
    
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(temp_data);
}

void add_gaussian_noise(Image* img, float noise_level) {
    // HOTSPOT 4: Random number generation and pixel operations
    srand(time(NULL));
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                int idx = (y * img->width + x) * img->channels + c;
                
                // Generate Gaussian noise using Box-Muller transform
                static int has_spare = 0;
                static float spare;
                float noise;
                
                if (has_spare) {
                    has_spare = 0;
                    noise = spare;
                } else {
                    has_spare = 1;
                    float u = (rand() + 1.0f) / (RAND_MAX + 2.0f);
                    float v = (rand() + 1.0f) / (RAND_MAX + 2.0f);
                    float mag = noise_level * sqrt(-2.0f * log(u));
                    noise = mag * cos(2.0f * M_PI * v);
                    spare = mag * sin(2.0f * M_PI * v);
                }
                
                float new_value = img->data[idx] + noise;
                img->data[idx] = (unsigned char)(new_value < 0 ? 0 : (new_value > 255 ? 255 : new_value));
            }
        }
    }
}

void apply_edge_detection(Image* img) {
    // HOTSPOT 5: Sobel edge detection - dual convolution operations
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
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                float gx = 0.0f, gy = 0.0f;
                
                // Apply Sobel kernels
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
                
                // Calculate gradient magnitude
                float magnitude = sqrt(gx * gx + gy * gy);
                magnitude = magnitude > 255 ? 255 : magnitude;
                
                temp_data[(y * img->width + x) * img->channels + c] = (unsigned char)magnitude;
            }
        }
    }
    
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(temp_data);
}

double get_time() {
    // Simple cross-platform timing using standard C library
    return (double)clock() / CLOCKS_PER_SEC;
}

void print_processing_info(const char* operation, double time_taken) {
    printf("%s: %.4f seconds\n", operation, time_taken);
}