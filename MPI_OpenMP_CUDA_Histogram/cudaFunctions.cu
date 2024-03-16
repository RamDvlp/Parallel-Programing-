#include <cuda_runtime.h>
#include <helper_cuda.h>
#include "myProto.h"


// Initialize the temporary array
__global__ void initTemp(int* arr) {

    int id = threadIdx.x + blockIdx.x * blockDim.x;
    int i;
    int offset = id*256;
  
    for (i = offset;   i < offset+256;   i++)   // Each of 200 threads initialize 256 members in temp
        arr[i] = 0;  
  
}

// Kernel to create the temporary array
__global__ void fillTemp(int* d_A, int *d_temp, int size, int range) {
  int id = threadIdx.x + blockIdx.x * blockDim.x; //0-199
  int chunk = size/200; //75000 / 200 = 375
  int offset_data = id * chunk;  // Start of the part of data for this thread
  int offset_temp = id *range; // Start of the part of temp for this thread
  int i, index;

  // Jump to the place in data and update the proper part of the temp
  for (i = 0;   i < chunk;  i++) {
    index = d_A[offset_data + i];
    d_temp[offset_temp + index]++;
  }
}

// Unify all values in the temp
__global__ void unify(int *d_temp, int *d_out) {

  int tid = threadIdx.x + blockIdx.x * blockDim.x;

    for(int i = tid ;i< 256; i+=200 ) { // Threads 0-55 will make "double shift"
        int result = 0;
        for(int bin = 0; bin < 200; bin++){
            result += d_temp[i + bin*256];
        }
        d_out[i] = result;
    }

}



int computeOnGPU(int *data, int numElements,int* cudaOut, int cudaOutSize) {
    // Error code to check return values for CUDA calls
    cudaError_t err = cudaSuccess;

    size_t size = numElements * sizeof(int); // N/4 data
    size_t outSize = cudaOutSize * sizeof(int); // 256 values
    size_t tempSize = cudaOutSize * NUM_OF_BLOCKS * NUM_OF_THREADS * sizeof(int); //256 * 10 * 20 = 51200 temp array

    // Allocate memory on GPU to copy the data from the host
    int *d_A, *d_out, *d_temp;
    err = cudaMalloc((void **)&d_A, size);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to allocate data device memory - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    err = cudaMalloc((void **)&d_out, outSize);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to allocate result device memory - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    err = cudaMalloc((void **)&d_temp, tempSize);
    if (err != cudaSuccess) {
        fprintf(stderr, "%d Failed to allocate temp device memory - %s\n", __LINE__,cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // Init temp array
    initTemp<<<NUM_OF_BLOCKS, NUM_OF_THREADS>>>(d_temp); // 10 blocks 20 threads; each thread has range of 256
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "%d Failed to launch temp init -  %s\n",__LINE__, cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // Copy data from host to the GPU memory
    err = cudaMemcpy(d_A, data, size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "%d Failed to copy data from host to device - %s\n",__LINE__, cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }


    //Calculate the histogram for each thread on ints segment              75000*int| 51200*int| 75000| 256
    fillTemp<<<NUM_OF_BLOCKS,NUM_OF_THREADS>>>(d_A, d_temp, size/4, cudaOutSize); //data, tempTofill, datasize, value range
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "%d Failed to launch fillTemp kernel -  %s\n",__LINE__, cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // Add values from each thread segment to a single array
    unify<<<NUM_OF_BLOCKS, NUM_OF_THREADS>>>(d_temp, d_out);
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to launch unify kernel -  %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // Copy the  result from GPU to the host memory.
    err = cudaMemcpy(cudaOut, d_out, outSize, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to copy result array from device to host -%s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // Free allocated memory on GPU
    if (cudaFree(d_A) != cudaSuccess) {
        fprintf(stderr, "Failed to free device data - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

        if (cudaFree(d_temp) != cudaSuccess) {
        fprintf(stderr, "Failed to free device temp - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

        if (cudaFree(d_out) != cudaSuccess) {
        fprintf(stderr, "Failed to free device out - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    return 0;
}

