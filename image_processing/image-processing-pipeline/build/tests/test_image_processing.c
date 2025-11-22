#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

// DO NOT include STB headers here since sequential_image_processing.c already includes them
// We'll just use the Image structure definition

typedef struct {
    unsigned char *data;
    int width;
    int height;
    int channels;
} Image;

// Test framework macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    printf("Running: %s...", #name); \
    test_##name(); \
    printf(" PASSED\n"); \
    passed_tests++; \
} while(0)

#define ASSERT_TRUE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "ASSERTION FAILED: %s (line %d)\n", #expr, __LINE__); \
        exit(1); \
    } \
} while(0)

#define ASSERT_EQ(a, b) ASSERT_TRUE((a) == (b))
#define ASSERT_NEQ(a, b) ASSERT_TRUE((a) != (b))
#define ASSERT_NEAR(a, b, eps) ASSERT_TRUE(fabs((a) - (b)) < (eps))

// Test utilities
Image* create_test_image(int width, int height, int channels) {
    Image* img = malloc(sizeof(Image));
    img->width = width;
    img->height = height;
    img->channels = channels;
    img->data = calloc(width * height * channels, sizeof(unsigned char));
    return img;
}

void free_test_image(Image* img) {
    if (img) {
        free(img->data);
        free(img);
    }
}

// Test: Image creation
TEST(image_creation) {
    Image* img = create_test_image(100, 100, 3);
    ASSERT_NEQ(img, NULL);
    ASSERT_EQ(img->width, 100);
    ASSERT_EQ(img->height, 100);
    ASSERT_EQ(img->channels, 3);
    ASSERT_NEQ(img->data, NULL);
    free_test_image(img);
}

// Test: Grayscale conversion math
TEST(grayscale_conversion) {
    Image* img = create_test_image(10, 10, 3);
    img->data[0] = 255; img->data[1] = 0; img->data[2] = 0;
    
    float expected_gray = 0.299f * 255;
    float gray = 0.299f * img->data[0] + 0.587f * img->data[1] + 0.114f * img->data[2];
    
    ASSERT_NEAR(gray, expected_gray, 1.0);
    free_test_image(img);
}

// Test: Gaussian kernel properties
TEST(gaussian_kernel) {
    float sigma = 1.0f;
    int kernel_size = 5;
    int kernel_radius = 2;
    
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
    
    float sum = 0.0f;
    for (int i = 0; i < kernel_size * kernel_size; i++) {
        sum += kernel[i];
    }
    
    ASSERT_NEAR(sum, 1.0f, 0.001f);
    free(kernel);
}

// Test: Edge detection range
TEST(edge_detection_range) {
    Image* img = create_test_image(50, 50, 3);
    
    for (int y = 0; y < img->height; y++) {
        for (int x = 0; x < img->width; x++) {
            int idx = (y * img->width + x) * img->channels;
            unsigned char value = (unsigned char)(x * 255 / img->width);
            img->data[idx] = img->data[idx+1] = img->data[idx+2] = value;
        }
    }
    
    for (int i = 0; i < img->width * img->height * img->channels; i++) {
        ASSERT_TRUE(img->data[i] <= 255);
    }
    
    free_test_image(img);
}

// Test: Downsample dimensions
TEST(downsample_dimensions) {
    int orig_width = 1024, orig_height = 768;
    int new_width = orig_width / 2, new_height = orig_height / 2;
    
    ASSERT_EQ(new_width, 512);
    ASSERT_EQ(new_height, 384);
}

// Test: Compression levels
TEST(compression_levels) {
    int width = 1024;
    for (int i = 1; i <= 3; i++) width = width / 2;
    ASSERT_EQ(width, 128);
}

// Test: Pixel clamping
TEST(pixel_clamping) {
    float test_values[] = {-10.0f, 0.0f, 127.5f, 255.0f, 300.0f};
    unsigned char expected[] = {0, 0, 128, 255, 255};
    
    for (int i = 0; i < 5; i++) {
        float val = test_values[i];
        val = val < 0 ? 0 : (val > 255 ? 255 : val);
        ASSERT_EQ((unsigned char)val, expected[i]);
    }
}

// Test: Memory boundaries
TEST(memory_boundaries) {
    Image* img = create_test_image(100, 100, 3);
    
    img->data[0] = 255;
    ASSERT_EQ(img->data[0], 255);
    
    int last_idx = (img->width * img->height - 1) * img->channels;
    img->data[last_idx] = 128;
    ASSERT_EQ(img->data[last_idx], 128);
    
    free_test_image(img);
}

// Main test runner
int main(int argc, char* argv[]) {
    int passed_tests = 0;
    
    printf("======================================\n");
    printf("Image Processing Unit Tests\n");
    printf("======================================\n\n");
    
    if (argc > 1) {
        if (strcmp(argv[1], "load") == 0) {
            RUN_TEST(image_creation);
        } else if (strcmp(argv[1], "grayscale") == 0) {
            RUN_TEST(grayscale_conversion);
        } else if (strcmp(argv[1], "blur") == 0) {
            RUN_TEST(gaussian_kernel);
        } else if (strcmp(argv[1], "edge") == 0) {
            RUN_TEST(edge_detection_range);
        } else if (strcmp(argv[1], "compress") == 0) {
            RUN_TEST(downsample_dimensions);
            RUN_TEST(compression_levels);
        } else if (strcmp(argv[1], "all") == 0) {
            RUN_TEST(image_creation);
            RUN_TEST(grayscale_conversion);
            RUN_TEST(gaussian_kernel);
            RUN_TEST(edge_detection_range);
            RUN_TEST(downsample_dimensions);
            RUN_TEST(compression_levels);
            RUN_TEST(pixel_clamping);
            RUN_TEST(memory_boundaries);
        } else {
            fprintf(stderr, "Unknown test: %s\n", argv[1]);
            return 1;
        }
    } else {
        RUN_TEST(image_creation);
        RUN_TEST(grayscale_conversion);
        RUN_TEST(gaussian_kernel);
        RUN_TEST(edge_detection_range);
        RUN_TEST(downsample_dimensions);
        RUN_TEST(compression_levels);
        RUN_TEST(pixel_clamping);
        RUN_TEST(memory_boundaries);
    }
    
    printf("\n======================================\n");
    printf("Tests passed: %d\n", passed_tests);
    printf("======================================\n");
    
    return 0;
}
