// cuda_kernels.cu - CUDA Kernels with CUDA Graph API
#include <cuda_runtime.h>
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
        { 0, -1,  0},
        {-1,  5, -1},
        { 0, -1,  0}
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
        { 0,  0,  0},
        { 1,  2,  1}
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

// ==================== CUDA GRAPH WRAPPER ====================

class CUDAImageGraph {
private:
    cudaGraph_t graph;
    cudaGraphExec_t graphExec;
    bool graphCreated;
    
    unsigned char *d_input, *d_output, *d_temp;
    float *d_kernel;
    
    int width, height, channels;
    size_t imageSize;
    
public:
    CUDAImageGraph() : graphCreated(false), graph(nullptr), graphExec(nullptr),
                       d_input(nullptr), d_output(nullptr), d_temp(nullptr), d_kernel(nullptr) {}
    
    ~CUDAImageGraph() {
        cleanup();
    }
    
    void initialize(int w, int h, int c) {
        width = w;
        height = h;
        channels = c;
        imageSize = width * height * channels;
        
        // Allocate device memory
        CUDA_CHECK(cudaMalloc(&d_input, imageSize));
        CUDA_CHECK(cudaMalloc(&d_output, imageSize));
        CUDA_CHECK(cudaMalloc(&d_temp, imageSize));
    }
    
    void createProcessingGraph(unsigned char* h_input) {
        // Copy input to device
        CUDA_CHECK(cudaMemcpy(d_input, h_input, imageSize, cudaMemcpyHostToDevice));
        
        // Start graph capture
        cudaStream_t stream;
        CUDA_CHECK(cudaStreamCreate(&stream));
        CUDA_CHECK(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal));
        
        dim3 blockSize(16, 16);
        dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                     (height + blockSize.y - 1) / blockSize.y);
        
        // Node 1: Grayscale conversion
        grayscale_kernel<<<gridSize, blockSize, 0, stream>>>(d_input, width, height, channels);
        
        // Node 2: Edge detection
        edge_detection_kernel<<<gridSize, blockSize, 0, stream>>>(d_input, d_output, 
                                                                   width, height, channels);
        
        // Node 3: Sharpening (using output as new input)
        sharpening_kernel<<<gridSize, blockSize, 0, stream>>>(d_output, d_temp,
                                                              width, height, channels);
        
        // Copy temp back to output
        CUDA_CHECK(cudaMemcpyAsync(d_output, d_temp, imageSize, 
                                   cudaMemcpyDeviceToDevice, stream));
        
        // End graph capture
        CUDA_CHECK(cudaStreamEndCapture(stream, &graph));
        
        // Instantiate the graph
        CUDA_CHECK(cudaGraphInstantiate(&graphExec, graph, nullptr, nullptr, 0));
        
        CUDA_CHECK(cudaStreamDestroy(stream));
        graphCreated = true;
        
        printf("CUDA Graph created with multiple processing nodes\n");
    }
    
    void executeGraph() {
        if (!graphCreated) {
            fprintf(stderr, "Error: Graph not created\n");
            return;
        }
        
        cudaStream_t stream;
        CUDA_CHECK(cudaStreamCreate(&stream));
        
        // Launch the entire graph
        CUDA_CHECK(cudaGraphLaunch(graphExec, stream));
        CUDA_CHECK(cudaStreamSynchronize(stream));
        
        CUDA_CHECK(cudaStreamDestroy(stream));
    }
    
    void getResult(unsigned char* h_output) {
        CUDA_CHECK(cudaMemcpy(h_output, d_output, imageSize, cudaMemcpyDeviceToHost));
    }
    
    void cleanup() {
        if (graphCreated) {
            cudaGraphExecDestroy(graphExec);
            cudaGraphDestroy(graph);
        }
        
        if (d_input) cudaFree(d_input);
        if (d_output) cudaFree(d_output);
        if (d_temp) cudaFree(d_temp);
        if (d_kernel) cudaFree(d_kernel);
    }
};

// ==================== VIDEO PROCESSING WITH CUDA GRAPH ====================

class CUDAVideoGraph {
private:
    cudaGraph_t graph;
    cudaGraphExec_t graphExec;
    bool graphCreated;
    
    unsigned char **d_frames;
    unsigned char **d_processed;
    int num_frames;
    int width, height, channels;
    size_t frameSize;
    
public:
    CUDAVideoGraph() : graphCreated(false), graph(nullptr), graphExec(nullptr),
                       d_frames(nullptr), d_processed(nullptr) {}
    
    ~CUDAVideoGraph() {
        cleanup();
    }
    
    void initialize(int num_f, int w, int h, int c) {
        num_frames = num_f;
        width = w;
        height = h;
        channels = c;
        frameSize = width * height * channels;
        
        // Allocate arrays of pointers
        d_frames = new unsigned char*[num_frames];
        d_processed = new unsigned char*[num_frames];
        
        // Allocate device memory for each frame
        for (int i = 0; i < num_frames; i++) {
            CUDA_CHECK(cudaMalloc(&d_frames[i], frameSize));
            CUDA_CHECK(cudaMalloc(&d_processed[i], frameSize));
        }
    }
    
    void createVideoGraph(unsigned char** h_frames) {
        cudaStream_t stream;
        CUDA_CHECK(cudaStreamCreate(&stream));
        
        // Start capturing the graph
        CUDA_CHECK(cudaStreamBeginCapture(stream, cudaStreamCaptureModeGlobal));
        
        dim3 blockSize(16, 16);
        dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                     (height + blockSize.y - 1) / blockSize.y);
        
        // Create a graph node for each frame
        for (int i = 0; i < num_frames; i++) {
            // Copy frame to device
            CUDA_CHECK(cudaMemcpyAsync(d_frames[i], h_frames[i], frameSize,
                                       cudaMemcpyHostToDevice, stream));
            
            // Process frame: grayscale + edge detection
            grayscale_kernel<<<gridSize, blockSize, 0, stream>>>(d_frames[i], 
                                                                 width, height, channels);
            
            edge_detection_kernel<<<gridSize, blockSize, 0, stream>>>(d_frames[i], 
                                                                      d_processed[i],
                                                                      width, height, channels);
            
            // Copy result back
            CUDA_CHECK(cudaMemcpyAsync(h_frames[i], d_processed[i], frameSize,
                                       cudaMemcpyDeviceToHost, stream));
        }
        
        // End graph capture
        CUDA_CHECK(cudaStreamEndCapture(stream, &graph));
        
        // Instantiate the graph
        CUDA_CHECK(cudaGraphInstantiate(&graphExec, graph, nullptr, nullptr, 0));
        
        CUDA_CHECK(cudaStreamDestroy(stream));
        graphCreated = true;
        
        printf("CUDA Video Graph created for %d frames\n", num_frames);
    }
    
    void executeGraph() {
        if (!graphCreated) {
            fprintf(stderr, "Error: Video graph not created\n");
            return;
        }
        
        cudaStream_t stream;
        CUDA_CHECK(cudaStreamCreate(&stream));
        
        // Launch the entire video processing graph
        CUDA_CHECK(cudaGraphLaunch(graphExec, stream));
        CUDA_CHECK(cudaStreamSynchronize(stream));
        
        CUDA_CHECK(cudaStreamDestroy(stream));
    }
    
    void cleanup() {
        if (graphCreated) {
            cudaGraphExecDestroy(graphExec);
            cudaGraphDestroy(graph);
        }
        
        if (d_frames) {
            for (int i = 0; i < num_frames; i++) {
                if (d_frames[i]) cudaFree(d_frames[i]);
                if (d_processed[i]) cudaFree(d_processed[i]);
            }
            delete[] d_frames;
            delete[] d_processed;
        }
    }
};

// ==================== C INTERFACE FOR HOST CODE ====================

extern "C" {

void launch_grayscale_kernel(unsigned char* d_data, int width, int height, int channels) {
    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                 (height + blockSize.y - 1) / blockSize.y);
    
    grayscale_kernel<<<gridSize, blockSize>>>(d_data, width, height, channels);
    CUDA_CHECK(cudaDeviceSynchronize());
}

void launch_edge_detection_kernel(unsigned char* d_input, unsigned char* d_output,
                                  int width, int height, int channels) {
    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                 (height + blockSize.y - 1) / blockSize.y);
    
    edge_detection_kernel<<<gridSize, blockSize>>>(d_input, d_output, width, height, channels);
    CUDA_CHECK(cudaDeviceSynchronize());
}

void launch_sharpening_kernel(unsigned char* d_input, unsigned char* d_output,
                              int width, int height, int channels) {
    dim3 blockSize(16, 16);
    dim3 gridSize((width + blockSize.x - 1) / blockSize.x,
                 (height + blockSize.y - 1) / blockSize.y);
    
    sharpening_kernel<<<gridSize, blockSize>>>(d_input, d_output, width, height, channels);
    CUDA_CHECK(cudaDeviceSynchronize());
}

void launch_downsample_kernel(unsigned char* d_input, unsigned char* d_output,
                              int width, int height, int new_width, int new_height,
                              int channels) {
    int scale_factor = width / new_width;
    
    dim3 blockSize(16, 16);
    dim3 gridSize((new_width + blockSize.x - 1) / blockSize.x,
                 (new_height + blockSize.y - 1) / blockSize.y);
    
    downsample_kernel<<<gridSize, blockSize>>>(d_input, d_output, width, height,
                                               new_width, new_height, channels, scale_factor);
    CUDA_CHECK(cudaDeviceSynchronize());
}

// High-level graph-based processing
void* create_cuda_image_graph(int width, int height, int channels) {
    CUDAImageGraph* graph = new CUDAImageGraph();
    graph->initialize(width, height, channels);
    return (void*)graph;
}

void execute_cuda_image_graph(void* graph_ptr, unsigned char* h_input, unsigned char* h_output) {
    CUDAImageGraph* graph = (CUDAImageGraph*)graph_ptr;
    graph->createProcessingGraph(h_input);
    graph->executeGraph();
    graph->getResult(h_output);
}

void destroy_cuda_image_graph(void* graph_ptr) {
    CUDAImageGraph* graph = (CUDAImageGraph*)graph_ptr;
    delete graph;
}

void* create_cuda_video_graph(int num_frames, int width, int height, int channels) {
    CUDAVideoGraph* graph = new CUDAVideoGraph();
    graph->initialize(num_frames, width, height, channels);
    return (void*)graph;
}

void execute_cuda_video_graph(void* graph_ptr, unsigned char** h_frames) {
    CUDAVideoGraph* graph = (CUDAVideoGraph*)graph_ptr;
    graph->createVideoGraph(h_frames);
    graph->executeGraph();
}

void destroy_cuda_video_graph(void* graph_ptr) {
    CUDAVideoGraph* graph = (CUDAVideoGraph*)graph_ptr;
    delete graph;
}

} // extern "C"