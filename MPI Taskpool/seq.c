#include <stdio.h>
#include <math.h>
#include <mpi.h>

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

// Sequencial code to be parallelized
int main(int argc, char* argv[]) {
	int x, y;
	int size = SIZE;
	double answer = 0, t1 = 0, t2 = 0; 
	MPI_Init(&argc, &argv);

	t1 = MPI_Wtime();
	for (x = 0; x < size; x++)
		for (y = 0; y < size; y++)
			answer += heavy(x, y);

	t2 = MPI_Wtime();
	 
	printf("answer = %e\ntime = %e\n", answer, (t2-t1));
}
