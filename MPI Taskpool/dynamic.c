#include <stdio.h>
#include "mpi.h"
#include <math.h>
//#include <stdlib.h>

#define HEAVY  1000
#define SIZE   60
#define WORK	0
#define QUIT	1

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
	int x, y = 0; // in dunamic taskpool each process will recive one line to work on every time.
	int  rank; /* rank of process */
	int  numProc;       /* number of processes */
	MPI_Status status ;   /* return status for receive */
	//int chunk, from;
	double result = 0, t1 = 0, t2 = 0;
	
	
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); 
	MPI_Comm_size(MPI_COMM_WORLD, &numProc); 
	
	if(numProc < 2) {
		printf("Run with atleast 2 processes\n");
		MPI_Abort(MPI_COMM_WORLD, __LINE__);
		
	}

	// The assumption is that the master does not participate in tasks.
	//master code
	if (rank == 0){
	
		double answer = 0;
		int i, counter = 0;
		t1 = MPI_Wtime();
		// Initially Distribute a task between the workers.
		// The task is calculate values based on x,y coordinates.
		// each process recives indexe of one array to work on
		for (i = 0;  i < numProc-1;  i++) {
			MPI_Send(&y, 1, MPI_INT, i  + 1, WORK, MPI_COMM_WORLD);	
			y++;
			counter++; //count the working processes.
			//in case we will start with number of processes larger than taskpool (although unlikely)
			if(y == SIZE)
			   break;
			}
		//while anyone from processes still working
		while(counter > 0){
			// Receive the answer from each worker
			MPI_Recv(&result, 1, MPI_DOUBLE, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
			counter--;
			answer += result;
			//printf("%e\n",answer);
			//if there is more work to do
			if(y < SIZE) {
				// Send new task to process which gave answer and waiting for task
				MPI_Send(&y, 1, MPI_INT, status.MPI_SOURCE, WORK, MPI_COMM_WORLD);
				y++;
				counter++;
			// no more tasks - need release slaves
			} else {
				MPI_Send(&y, 1, MPI_INT, status.MPI_SOURCE, QUIT, MPI_COMM_WORLD);
			}
		
		}
		
		t2 = MPI_Wtime();
		printf("answer = %e\ntime = %e\n", answer, (t2-t1));
		
	}
	//slave code
	else{
		// Receive the first task array from the master
		MPI_Recv(&y, 1, MPI_INT, 0, WORK, MPI_COMM_WORLD, &status);
		
		//Work while there is work to do
		while(status.MPI_TAG == WORK){
			result = 0;
			// Perform the computation on the part of the array
			for(x = 0; x < SIZE; x++){
				result += heavy(x, y);
			
			}
			// Send the result to master
			MPI_Send(&result, 1, MPI_DOUBLE, 0, WORK, MPI_COMM_WORLD);
			// Wait to get another task or finish
			MPI_Recv(&y, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
		}
		
	}

	MPI_Finalize(); 
	
	
	return 0;
}
