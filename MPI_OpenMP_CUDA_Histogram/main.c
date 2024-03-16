#include <mpi.h>
#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>
#include "myProto.h"

/*
Calculating Histogram of large array in parallel with MPI+OpenMP+CUDA.
Initially the array of size N=300,000 is known for the process 0.
It sends the half of the array to the process 1.
Both processes start calculate the histogram of their parts - quarter with OpenMP, quarter with CUDA
The results is send from the process 1 to the process 0, which perform the test to verify that the integration worked properly
*/
 

int main(int argc, char *argv[]) {
    int size, rank, i, j;
    int *data; // N random elements
    MPI_Status  status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    if (size != 2) {
       printf("Run the example with two processes only\n");
       MPI_Abort(MPI_COMM_WORLD, __LINE__);
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
 
 	int N = 300000;
	int range = 256;
	
	// allocate the memory 
	int *temp = (int *) malloc(CORES * range * sizeof(int)); // 4 CORES times values range; 256 * 4 = 1024
	int *histogram = (int *) malloc(range * sizeof(int)); // 256 histogram
	if (temp == NULL || histogram == NULL) {
		printf("Cannot allocate the memory\n");
		exit(-1);
	}
   // Initialize the temp result for each process
   for (i = 0;  i < CORES * range; i++)
         temp[i] = 0;

    // Divide the tasks between both processes
    if (rank == 0) {
       // Allocate memory for the whole array and send a half of the array to other process
      if ((data = (int *) malloc(N *sizeof(int))) == NULL)
          MPI_Abort(MPI_COMM_WORLD, __LINE__);

         // Initializiation of arrays
      srand(time(NULL));
      for (i = 0;  i < N;  i++){
         data[i] = rand() % range;
       //  printf("%d ",data[i]);
      }
       // Send the second half of array
       MPI_Send(data+ N/2, N/2, MPI_INT, 1, 0, MPI_COMM_WORLD);
       
    } else {
       // Allocate memory and reieve a half of array from other process
       if ((data = (int *) malloc((N/2) *sizeof(int))) == NULL)
          MPI_Abort(MPI_COMM_WORLD, __LINE__);
       MPI_Recv(data, N/2, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
    }
    

////////////
// Run cuda on quarter of data array by each process.


int* cudaOut = (int*)malloc(range*sizeof(int));
computeOnGPU(data, N/4, cudaOut, range);

	// The other quarter handled by openMP
#pragma omp parallel private (i)
{
	int tid = omp_get_thread_num();
	int offset = tid*range;
#pragma omp for 
	for (i = N/4;   i < N/2;  i++)
		temp[offset + data[i]]++;
}

	// Unify the temporary results of omp
#pragma omp parallel private(i, j)
{
#pragma omp for
	for (i = 0;   i < range;   i++) {
		int result = 0;
		for (j = 0;  j < CORES;   j++)
			result += temp[i + range * j];
		histogram[i] = result;
	}
}

// Unify cuda results with omp results
 # pragma omp parallel for private (i)
for (i = 0; i < range; i++){
   histogram[i] += cudaOut[i];
}

free(cudaOut);



      // Collect the result in one of process
      if (rank == 0) {
         int* buff = (int*) malloc(range * sizeof(int));
         if(buff == NULL)
            MPI_Abort(MPI_COMM_WORLD, __LINE__);
         MPI_Recv(buff, range, MPI_INT, 1, 0, MPI_COMM_WORLD, &status);

         // add values from 2 processes together
         # pragma omp parallel for
         for (i = 0; i < range; i++){
            histogram[i] += buff[i];
         }

         test(histogram, data, N);

         free(buff);

      }  else {

         MPI_Send(histogram, range, MPI_INT, 0, 0, MPI_COMM_WORLD);

      }
         

    free(data);
    free(temp);
    free(histogram);

    MPI_Finalize();

    return 0;
}


