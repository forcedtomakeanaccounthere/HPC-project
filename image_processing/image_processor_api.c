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

// Image I/O functions
Image* load_image(const char* filename);
void free_image(Image* img);
void save_image(const char* filename, Image* img);
Image* create_image(int width, int height, int channels);

// Existing image processing functions
void convert_to_grayscale(Image* img);
void apply_gaussian_blur(Image* img, float sigma);
void apply_sharpening_filter(Image* img, float intensity);
void add_gaussian_noise(Image* img, float noise_level);
void apply_edge_detection(Image* img);
void compress_image_multilevel(Image* img, const char* output_prefix, int levels);

// NEW image processing functions
void adjust_brightness(Image* img, float brightness);
void adjust_saturation(Image* img, float saturation);
void flip_horizontal(Image* img);
void flip_vertical(Image* img);
void rotate_image_90(Image* img, int times);
void rotate_image_angle(Image* img, float angle);

double get_time() {
    return omp_get_wtime();
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
    stbi_write_png(filename, img->width, img->height, img->channels, img->data, img->width * img->channels);
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
    
    memset(img->data, 0, width * height * channels);
    return img;
}

// ==================== EXISTING PROCESSING FUNCTIONS ====================

void convert_to_grayscale(Image* img) {
    #pragma omp parallel for collapse(2)
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

void apply_gaussian_blur(Image* img, float sigma) {
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

void apply_sharpening_filter(Image* img, float intensity) {
    float center = 1.0f + 4.0f * intensity;
    float edge = -intensity;
    
    float kernel[3][3] = {
        { 0, edge,  0},
        {edge, center, edge},
        { 0, edge,  0}
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
    srand(time(NULL));
    
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                int idx = (y * img->width + x) * img->channels + c;
                
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
                
                float magnitude = sqrt(gx * gx + gy * gy);
                magnitude = magnitude > 255 ? 255 : magnitude;
                
                temp_data[(y * img->width + x) * img->channels + c] = (unsigned char)magnitude;
            }
        }
    }
    
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(temp_data);
}

Image* downsample_image(Image* img, int scale_factor) {
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

void compress_image_multilevel(Image* img, const char* output_prefix, int levels) {
    Image* current = create_image(img->width, img->height, img->channels);
    memcpy(current->data, img->data, img->width * img->height * img->channels);
    
    for (int level = 0; level < levels; level++) {
        apply_gaussian_blur(current, 0.8f);
        Image* downsampled = downsample_image(current, 2);
        free_image(current);
        current = downsampled;
    }
    
    // Copy final compressed result back
    if (current->width <= img->width && current->height <= img->height) {
        memset(img->data, 0, img->width * img->height * img->channels);
        for (int y = 0; y < current->height && y < img->height; y++) {
            for (int x = 0; x < current->width && x < img->width; x++) {
                for (int c = 0; c < img->channels; c++) {
                    int src_idx = (y * current->width + x) * img->channels + c;
                    int dst_idx = (y * img->width + x) * img->channels + c;
                    img->data[dst_idx] = current->data[src_idx];
                }
            }
        }
    }
    
    free_image(current);
}

// ==================== NEW PROCESSING FUNCTIONS ====================

void adjust_brightness(Image* img, float brightness) {
    // brightness: -100 to +100
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                int idx = (y * img->width + x) * img->channels + c;
                float new_value = img->data[idx] + brightness;
                img->data[idx] = (unsigned char)(new_value < 0 ? 0 : (new_value > 255 ? 255 : new_value));
            }
        }
    }
}

void adjust_saturation(Image* img, float saturation) {
    // saturation: 0.0 (grayscale) to 2.0 (highly saturated)
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            if (img->channels >= 3) {
                int idx = (y * img->width + x) * img->channels;
                
                float r = img->data[idx];
                float g = img->data[idx + 1];
                float b = img->data[idx + 2];
                
                float gray = 0.299f * r + 0.587f * g + 0.114f * b;
                
                float new_r = gray + saturation * (r - gray);
                float new_g = gray + saturation * (g - gray);
                float new_b = gray + saturation * (b - gray);
                
                img->data[idx] = (unsigned char)(new_r < 0 ? 0 : (new_r > 255 ? 255 : new_r));
                img->data[idx + 1] = (unsigned char)(new_g < 0 ? 0 : (new_g > 255 ? 255 : new_g));
                img->data[idx + 2] = (unsigned char)(new_b < 0 ? 0 : (new_b > 255 ? 255 : new_b));
            }
        }
    }
}

void flip_horizontal(Image* img) {
    unsigned char* temp_data = malloc(img->width * img->height * img->channels);
    
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                int src_idx = (y * img->width + x) * img->channels + c;
                int dst_idx = (y * img->width + (img->width - 1 - x)) * img->channels + c;
                temp_data[dst_idx] = img->data[src_idx];
            }
        }
    }
    
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(temp_data);
}

void flip_vertical(Image* img) {
    unsigned char* temp_data = malloc(img->width * img->height * img->channels);
    
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            for (int c = 0; c < img->channels; c++) {
                int src_idx = (y * img->width + x) * img->channels + c;
                int dst_idx = ((img->height - 1 - y) * img->width + x) * img->channels + c;
                temp_data[dst_idx] = img->data[src_idx];
            }
        }
    }
    
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(temp_data);
}

void rotate_image_90(Image* img, int times) {
    // Rotate 90 degrees clockwise 'times' number of times
    times = times % 4;
    if (times < 0) times += 4;
    
    for (int t = 0; t < times; t++) {
        Image* rotated = create_image(img->height, img->width, img->channels);
        
        #pragma omp parallel for collapse(2)
        for (int y = 0; y < img->height; y++) {
            for (int x = 0; x < img->width; x++) {
                for (int c = 0; c < img->channels; c++) {
                    int src_idx = (y * img->width + x) * img->channels + c;
                    int new_x = img->height - 1 - y;
                    int new_y = x;
                    int dst_idx = (new_y * rotated->width + new_x) * img->channels + c;
                    rotated->data[dst_idx] = img->data[src_idx];
                }
            }
        }
        
        // Swap dimensions and data
        int temp_dim = img->width;
        img->width = img->height;
        img->height = temp_dim;
        
        free(img->data);
        img->data = rotated->data;
        rotated->data = NULL;
        free_image(rotated);
    }
}

void rotate_image_angle(Image* img, float angle) {
    // Convert to radians
    float rad = angle * M_PI / 180.0f;
    float cos_a = cos(rad);
    float sin_a = sin(rad);
    
    int cx = img->width / 2;
    int cy = img->height / 2;
    
    unsigned char* temp_data = malloc(img->width * img->height * img->channels);
    memset(temp_data, 0, img->width * img->height * img->channels);
    
    #pragma omp parallel for collapse(2)
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int dx = x - cx;
            int dy = y - cy;
            
            int src_x = (int)(dx * cos_a - dy * sin_a + cx);
            int src_y = (int)(dx * sin_a + dy * cos_a + cy);
            
            if (src_x >= 0 && src_x < img->width && src_y >= 0 && src_y < img->height) {
                for (int c = 0; c < img->channels; c++) {
                    int src_idx = (src_y * img->width + src_x) * img->channels + c;
                    int dst_idx = (y * img->width + x) * img->channels + c;
                    temp_data[dst_idx] = img->data[src_idx];
                }
            }
        }
    }
    
    memcpy(img->data, temp_data, img->width * img->height * img->channels);
    free(temp_data);
}

// ==================== MAIN API ====================

int main(int argc, char* argv[]) {
    if (argc < 4) {
        printf("Usage: %s <input> <output> <filter> [params...]\n", argv[0]);
        printf("Filters:\n");
        printf("  grayscale\n");
        printf("  blur <sigma>\n");
        printf("  sharpen <intensity>\n");
        printf("  noise <level>\n");
        printf("  edges\n");
        printf("  compress <levels>\n");
        printf("  brightness <value>\n");
        printf("  saturation <value>\n");
        printf("  flip-h\n");
        printf("  flip-v\n");
        printf("  rotate90 <times>\n");
        printf("  rotate <angle>\n");
        return 1;
    }
    
    const char* input_file = argv[1];
    const char* output_file = argv[2];
    const char* filter = argv[3];
    
    printf("Loading image: %s\n", input_file);
    Image* img = load_image(input_file);
    if (!img) {
        printf("Error: Could not load image\n");
        return 1;
    }
    
    printf("Image: %dx%d, %d channels\n", img->width, img->height, img->channels);
    
    double start_time = get_time();
    
    // Apply filter
    if (strcmp(filter, "grayscale") == 0) {
        printf("Applying grayscale filter\n");
        convert_to_grayscale(img);
    }
    else if (strcmp(filter, "blur") == 0 && argc >= 5) {
        float sigma = atof(argv[4]);
        printf("Applying blur (sigma=%.2f)\n", sigma);
        apply_gaussian_blur(img, sigma);
    }
    else if (strcmp(filter, "sharpen") == 0 && argc >= 5) {
        float intensity = atof(argv[4]);
        printf("Applying sharpen (intensity=%.2f)\n", intensity);
        apply_sharpening_filter(img, intensity);
    }
    else if (strcmp(filter, "noise") == 0 && argc >= 5) {
        float level = atof(argv[4]);
        printf("Applying noise (level=%.2f)\n", level);
        add_gaussian_noise(img, level);
    }
    else if (strcmp(filter, "edges") == 0) {
        printf("Applying edge detection\n");
        apply_edge_detection(img);
    }
    else if (strcmp(filter, "compress") == 0 && argc >= 5) {
        int levels = atoi(argv[4]);
        printf("Applying compression (levels=%d)\n", levels);
        compress_image_multilevel(img, "temp", levels);
    }
    else if (strcmp(filter, "brightness") == 0 && argc >= 5) {
        float value = atof(argv[4]);
        printf("Adjusting brightness (%.2f)\n", value);
        adjust_brightness(img, value);
    }
    else if (strcmp(filter, "saturation") == 0 && argc >= 5) {
        float value = atof(argv[4]);
        printf("Adjusting saturation (%.2f)\n", value);
        adjust_saturation(img, value);
    }
    else if (strcmp(filter, "flip-h") == 0) {
        printf("Flipping horizontally\n");
        flip_horizontal(img);
    }
    else if (strcmp(filter, "flip-v") == 0) {
        printf("Flipping vertically\n");
        flip_vertical(img);
    }
    else if (strcmp(filter, "rotate90") == 0 && argc >= 5) {
        int times = atoi(argv[4]);
        printf("Rotating 90° x %d\n", times);
        rotate_image_90(img, times);
    }
    else if (strcmp(filter, "rotate") == 0 && argc >= 5) {
        float angle = atof(argv[4]);
        printf("Rotating %.2f degrees\n", angle);
        rotate_image_angle(img, angle);
    }
    else {
        printf("Unknown filter or missing parameters\n");
        free_image(img);
        return 1;
    }
    
    double end_time = get_time();
    printf("Processing time: %.4f seconds\n", end_time - start_time);
    
    printf("Saving to: %s\n", output_file);
    save_image(output_file, img);
    
    free_image(img);
    printf("Done!\n");
    
    return 0;
}
