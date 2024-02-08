#include <stdio.h>
#include "mpi.h"
#include <math.h>
//#include <stdlib.h>

#define HEAVY  1000
#define SIZE   60

// This function performs heavy computations, 
// its run time depends on x and y values
// DO NOT change the function
double heavy(int x, int y) {
	int i, loop;
	double sum = 0;
	
	if (x > 0.25*SIZE &&  x < 0.5*SIZE && y > 0.4 * SIZE && y < 0.6 * SIZE)
		loop = x * y;
	else
		loop = y + x;

	for (i = 0; i < loop * HEAVY; i++)
		sum += cos(exp(sin((double)i / HEAVY)));

	return  sum;
}


int main(int argc, char* argv[]){
	int x, y;
	int  rank; /* rank of process */
	int  numProc;       /* number of processes */
	MPI_Status status ;   /* return status for receive */
	int chunk, from;
	double result = 0, t1 = 0, t2 = 0;
	
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
	MPI_Comm_size(MPI_COMM_WORLD, &numProc); 
	
	if(numProc < 2) {
		printf("Run with atleast 2 processes\n");
		MPI_Abort(MPI_COMM_WORLD, __LINE__);
		
	}

	// Define a chunk - the portion of the initial matrix to be sent to the workers
	chunk = SIZE / (numProc - 1);
	
	// The assumption is that the master does not participate in tasks.
	//master code
	if (rank == 0){
	
		double answer = 0;
		int i;
		t1 = MPI_Wtime();
		// Distribute the task between the workers.
		// The task is calculate values based on x,y coordinates.
		// This solution assumes that the array can be divided equally between the workers, and it can, 2,4,20 processes devidable by SIZE = 60
		for (i = 0;  i < numProc-1;  i++) {
			y = i * chunk;
			MPI_Send(&y, 1, MPI_INT, i  + 1, 0, MPI_COMM_WORLD);	
			}

		// Receive the answer from each worker
		for (i = 1;  i < numProc;  i++) {
			MPI_Recv(&result, 1, MPI_DOUBLE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			answer += result;
		}
		t2 = MPI_Wtime();
		printf("answer = %e\ntime = %e\n", answer, (t2-t1));
		
	}
	//slave code
	else{
		// Receive the chunk of the array from the master
		MPI_Recv(&from, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

		// Perform the computation on the part of the array
		for(x = 0; x < SIZE; x++){
			//each slave recives several line slices of the matrix to work on.
			for(y = from; y < from + chunk; y++){
				result += heavy(x, y);
			}
		}
		// Send the result to the master
		MPI_Send(&result, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
	}

	MPI_Finalize(); 
	
	
	return 0;
}
