// matrix_ops.c (updated)
// Author: HPC course project — updated for safe interactive profiling
// Compile: gcc -pg -g -Wall -O0 matrix_ops.c -o matrix_ops_profile -lm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <getopt.h>
#include <stdbool.h>

#define DEFAULT_MATRIX_SIZE 512      // Much smaller default for interactive runs
#define DEFAULT_VECTOR_SIZE 512
#define DEFAULT_NUM_ITERATIONS 3     // Small default so runs finish quickly
#define DEFAULT_BLOCK_SIZE 64
#define SAFE_OPS_THRESHOLD 1e10      // warn if estimated ops exceed this (approx)

/* Structure to hold computational data */
typedef struct {
    double **matrix_a;
    double **matrix_b;
    double **result_matrix;
    double *vector_x;
    double *vector_y;
    double *result_vector;   // used for matrix_vector result
    double *temp_vector;     // used for vector_operations output (so we don't overwrite result_vector)
    int size;
} ComputeData;

/* Function prototypes */
void initialize_data(ComputeData *data, int use_heavy_init);
void cleanup_data(ComputeData *data);
double** allocate_matrix(int size);
void free_matrix(double **matrix, int size);
void matrix_multiply(double **a, double **b, double **result, int size);
void blocked_matrix_multiply(double **a, double **b, double **result, int size, int block_size);
void matrix_vector_multiply(double **matrix, double *vector, double *result, int size);
void vector_operations(double *a, double *b, double *out, int size);
void data_preprocessing(double *data, int size);
void data_postprocessing(double *data, int size);
void compute_pipeline(ComputeData *data, int num_iterations, int use_blocked, int block_size);
double calculate_checksum(double **matrix, int size);

/* Utilities */
double** allocate_matrix(int size) {
    double **matrix = (double**)malloc(size * sizeof(double*));
    if (!matrix) {
        fprintf(stderr, "Memory allocation failed for matrix rows\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < size; i++) {
        matrix[i] = (double*)calloc(size, sizeof(double));
        if (!matrix[i]) {
            fprintf(stderr, "Memory allocation failed for matrix row %d\n", i);
            exit(EXIT_FAILURE);
        }
    }
    return matrix;
}

void free_matrix(double **matrix, int size) {
    if (matrix) {
        for (int i = 0; i < size; i++) {
            free(matrix[i]);
        }
        free(matrix);
    }
}

/* Data initialization - heavy initialization optional */
void initialize_data(ComputeData *data, int use_heavy_init) {
    printf("Initializing data structures (size=%d)...\n", data->size);
    fflush(stdout);

    data->matrix_a = allocate_matrix(data->size);
    data->matrix_b = allocate_matrix(data->size);
    data->result_matrix = allocate_matrix(data->size);

    data->vector_x = (double*)malloc(data->size * sizeof(double));
    data->vector_y = (double*)malloc(data->size * sizeof(double));
    data->result_vector = (double*)malloc(data->size * sizeof(double));
    data->temp_vector = (double*)malloc(data->size * sizeof(double));

    if (!data->vector_x || !data->vector_y || !data->result_vector || !data->temp_vector) {
        fprintf(stderr, "Memory allocation failed for vectors\n");
        exit(EXIT_FAILURE);
    }

    if (use_heavy_init) {
        // Computationally expensive trig-based pattern
        for (int i = 0; i < data->size; i++) {
            for (int j = 0; j < data->size; j++) {
                data->matrix_a[i][j] = sin(i * 0.01) * cos(j * 0.01) + (i + j) * 0.001;
                data->matrix_b[i][j] = cos(i * 0.01) * sin(j * 0.01) + (i - j) * 0.001;
            }
            data->vector_x[i] = sin(i * 0.02) + i * 0.001;
            data->vector_y[i] = cos(i * 0.02) + i * 0.001;
        }
    } else {
        // Fast deterministic initialization (useful for quick testing)
        for (int i = 0; i < data->size; i++) {
            for (int j = 0; j < data->size; j++) {
                data->matrix_a[i][j] = (double)(i + j + 1) / (double)(data->size);
                data->matrix_b[i][j] = (double)(i - j + 1) / (double)(data->size);
            }
            data->vector_x[i] = (double)(i + 1) / (double)(data->size);
            data->vector_y[i] = (double)(data->size - i) / (double)(data->size);
        }
    }

    printf("Data initialization completed.\n");
    fflush(stdout);
}

/* Data preprocessing */
void data_preprocessing(double *data, int size) {
    for (int i = 0; i < size; i++) {
        data[i] = sqrt(fabs(data[i])) + log(fabs(data[i]) + 1.0);
        for (int j = 0; j < 10; j++) {
            data[i] += sin(data[i] * 0.1) * 0.01;
        }
    }
}

/* Matrix multiplication (naive) */
void matrix_multiply(double **a, double **b, double **result, int size) {
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            double sum = 0.0;
            for (int k = 0; k < size; k++) {
                sum += a[i][k] * b[k][j];
            }
            result[i][j] = sum;
        }
    }
}

/* Blocked matrix multiply (cache-friendly) */
void blocked_matrix_multiply(double **a, double **b, double **result, int size, int block_size) {
    // Zero out result
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) result[i][j] = 0.0;
    }

    for (int bi = 0; bi < size; bi += block_size) {
        for (int bj = 0; bj < size; bj += block_size) {
            for (int bk = 0; bk < size; bk += block_size) {
                int max_i = (bi + block_size < size) ? bi + block_size : size;
                int max_j = (bj + block_size < size) ? bj + block_size : size;
                int max_k = (bk + block_size < size) ? bk + block_size : size;
                for (int i = bi; i < max_i; i++) {
                    for (int k = bk; k < max_k; k++) {
                        double a_ik = a[i][k];
                        for (int j = bj; j < max_j; j++) {
                            result[i][j] += a_ik * b[k][j];
                        }
                    }
                }
            }
        }
    }
}

/* Matrix-vector multiplication */
void matrix_vector_multiply(double **matrix, double *vector, double *result, int size) {
    for (int i = 0; i < size; i++) {
        double sum = 0.0;
        for (int j = 0; j < size; j++) sum += matrix[i][j] * vector[j];
        result[i] = sum;
    }
}

/* Vector operations (writes to out) */
void vector_operations(double *a, double *b, double *out, int size) {
    for (int i = 0; i < size; i++) {
        out[i] = sqrt(a[i] * a[i] + b[i] * b[i]) +
                 sin(a[i]) * cos(b[i]) +
                 log(fabs(a[i]) + 1.0);
    }
}

/* Data postprocessing */
void data_postprocessing(double *data, int size) {
    for (int i = 0; i < size; i++) {
        data[i] = pow(fabs(data[i]), 0.7) + tanh(data[i] * 0.1);
        if (i > 0) data[i] += data[i-1] * 0.05;
    }
}

/* Checksum */
double calculate_checksum(double **matrix, int size) {
    double sum = 0.0;
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) sum += matrix[i][j];
    }
    return sum;
}

/* Main compute pipeline */
void compute_pipeline(ComputeData *data, int num_iterations, int use_blocked, int block_size) {
    printf("Starting computational pipeline (iterations=%d, use_blocked=%d)...\n", num_iterations, use_blocked);
    fflush(stdout);

    clock_t t0 = clock();
    for (int iter = 0; iter < num_iterations; iter++) {
        if (iter % 1 == 0) {
            printf("Iteration %d/%d\n", iter + 1, num_iterations);
            fflush(stdout);
        }

        clock_t s = clock();
        data_preprocessing(data->vector_x, data->size);
        data_preprocessing(data->vector_y, data->size);
        clock_t e = clock();
        printf("  Preprocessing time: %f s\n", (double)(e - s) / CLOCKS_PER_SEC);
        fflush(stdout);

        s = clock();
        if (use_blocked) blocked_matrix_multiply(data->matrix_a, data->matrix_b, data->result_matrix, data->size, block_size);
        else matrix_multiply(data->matrix_a, data->matrix_b, data->result_matrix, data->size);
        e = clock();
        printf("  Matrix multiplication time: %f s\n", (double)(e - s) / CLOCKS_PER_SEC);
        fflush(stdout);

        s = clock();
        matrix_vector_multiply(data->result_matrix, data->vector_x, data->result_vector, data->size);
        e = clock();
        printf("  Matrix-vector time: %f s\n", (double)(e - s) / CLOCKS_PER_SEC);
        fflush(stdout);

        s = clock();
        // Use temp_vector so we don't overwrite result_vector
        vector_operations(data->vector_x, data->vector_y, data->temp_vector, data->size);
        e = clock();
        printf("  Vector operations time: %f s\n", (double)(e - s) / CLOCKS_PER_SEC);
        fflush(stdout);

        s = clock();
        // Postprocess the vector, write into result_vector (or combine as needed)
        // For demonstration, copy temp_vector into result_vector first then postprocess
        memcpy(data->result_vector, data->temp_vector, sizeof(double) * data->size);
        data_postprocessing(data->result_vector, data->size);
        e = clock();
        printf("  Postprocessing time: %f s\n", (double)(e - s) / CLOCKS_PER_SEC);
        fflush(stdout);
    }
    clock_t t1 = clock();
    printf("Pipeline total time: %f s\n", (double)(t1 - t0) / CLOCKS_PER_SEC);
    fflush(stdout);
}

/* Cleanup */
void cleanup_data(ComputeData *data) {
    free_matrix(data->matrix_a, data->size);
    free_matrix(data->matrix_b, data->size);
    free_matrix(data->result_matrix, data->size);
    free(data->vector_x);
    free(data->vector_y);
    free(data->result_vector);
    free(data->temp_vector);
}

/* Helper to estimate operations: ~2 * n^3 per matrix multiply */
double estimate_total_ops(long n, int iterations) {
    return 2.0 * (double)n * (double)n * (double)n * (double)iterations;
}

/* Print usage */
void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -s, --size N           Matrix/Vector size (default %d)\n", DEFAULT_MATRIX_SIZE);
    printf("  -n, --iterations N     Number of iterations (default %d)\n", DEFAULT_NUM_ITERATIONS);
    printf("  -b, --block-size N     Block size for blocked multiply (default %d)\n", DEFAULT_BLOCK_SIZE);
    printf("  -B, --use-blocked      Use blocked matrix multiply (default off)\n");
    printf("  -f, --fast-init        Use fast (cheap) initialization (default false)\n");
    printf("  -F, --force            Force run even if estimated ops are large\n");
    printf("  -h, --help             Show this help\n");
}

/* Main */
int main(int argc, char **argv) {
    int size = DEFAULT_MATRIX_SIZE;
    int iterations = DEFAULT_NUM_ITERATIONS;
    int block_size = DEFAULT_BLOCK_SIZE;
    int use_blocked = 0;
    int use_fast_init = 0;
    int force = 0;

    static struct option long_options[] = {
        {"size", required_argument, 0, 's'},
        {"iterations", required_argument, 0, 'n'},
        {"block-size", required_argument, 0, 'b'},
        {"use-blocked", no_argument, 0, 'B'},
        {"fast-init", no_argument, 0, 'f'},
        {"force", no_argument, 0, 'F'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "s:n:b:BFfh", long_options, NULL)) != -1) {
        switch (opt) {
            case 's': size = atoi(optarg); break;
            case 'n': iterations = atoi(optarg); break;
            case 'b': block_size = atoi(optarg); break;
            case 'B': use_blocked = 1; break;
            case 'f': use_fast_init = 1; break;
            case 'F': force = 1; break;
            case 'h':
            default:
                print_usage(argv[0]);
                return 0;
        }
    }

    if (size <= 0 || iterations <= 0 || block_size <= 0) {
        fprintf(stderr, "Invalid size/iterations/block_size. All must be > 0.\n");
        return EXIT_FAILURE;
    }

    printf("=== HPC Matrix-Vector Operations Benchmark ===\n");
    printf("Matrix Size: %d x %d\n", size, size);
    printf("Vector Size: %d\n", size);
    printf("Iterations: %d\n", iterations);
    printf("Use blocked multiply: %s\n", use_blocked ? "YES" : "NO");
    printf("Fast init: %s\n", use_fast_init ? "YES" : "NO");
    printf("\n");
    fflush(stdout);

    double estimated_ops = estimate_total_ops(size, iterations);
    printf("Estimated floating-point ops for matrix multiplies: %.3e\n", estimated_ops);
    if (!force && estimated_ops > SAFE_OPS_THRESHOLD) {
        fprintf(stderr, "Estimated ops > %.3e — this will take a very long time. Use --force to override.\n", SAFE_OPS_THRESHOLD);
        return EXIT_FAILURE;
    }
    fflush(stdout);

    ComputeData data;
    data.size = size;

    clock_t t0 = clock();
    initialize_data(&data, !use_fast_init);
    clock_t t1 = clock();
    double init_time = (double)(t1 - t0) / CLOCKS_PER_SEC;
    printf("Initialization time: %f seconds\n\n", init_time);
    fflush(stdout);

    t0 = clock();
    compute_pipeline(&data, iterations, use_blocked, block_size);
    t1 = clock();
    double compute_time = (double)(t1 - t0) / CLOCKS_PER_SEC;

    double checksum = calculate_checksum(data.result_matrix, data.size);

    // Memory usage estimate
    unsigned long long bytes = (3ULL * (unsigned long long)size * (unsigned long long)size + 3ULL * (unsigned long long)size) * sizeof(double);
    double mem_mb = (double)bytes / (1024.0 * 1024.0);

    printf("\n=== Performance Results ===\n");
    printf("Total computation time: %f seconds\n", compute_time);
    printf("Time per iteration: %f seconds\n", compute_time / iterations);
    printf("Result checksum: %f\n", checksum);
    printf("Estimated Memory usage: ~%.2f MB\n", mem_mb);
    printf("\nProgram completed successfully.\n");

    cleanup_data(&data);
    return 0;
}
