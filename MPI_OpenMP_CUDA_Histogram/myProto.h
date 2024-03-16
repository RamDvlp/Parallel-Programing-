#pragma once

#define CORES 4
#define NUM_OF_BLOCKS 10
#define NUM_OF_THREADS 20

void test(int *histogram, int* data, int N);
int computeOnGPU(int *data, int n, int* cudaOut, int cudaOutSize);
