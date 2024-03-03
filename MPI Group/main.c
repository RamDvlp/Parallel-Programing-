#include <mpi.h>
#include <stdio.h>
#include <ctype.h>
#include "kmp.h"

/*
	The code is suitable for K^2 processes.
*/

#define NUM_OF_PARAMS 3
#define WE_DONE 123456


int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Comm comm;
	MPI_Status status;
    int dim[2], period[2], reorder;
    int coord[2], id;
	int data[NUM_OF_PARAMS]; // [0] -> K root of processes num; [1] -> N half the length of line; [2] -> I max num of iterations.
	int K, N, I;
	int source, dest;
	FILE *file;
	char word[100]; // the word to search - aka pattern.
	char *text;
	char *textToLeft, *textToUp;
	int done = 0;

     MPI_Init(&argc, &argv);
     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
     MPI_Comm_size(MPI_COMM_WORLD, &size);
 
	if(rank == 0) {
	//Only process 0 reads from file.
		file = fopen("data.txt", "r"); // The fle name is hardcoded, may also be passed as excecution parameter.
     		if (file == NULL) {
        	perror("Unable to open the file.");
        	return 0;
     		}

     		if (fscanf(file, "%d %d %d\n", &data[0], &data[1], &data[2]) != 3) {
        	perror("Failed to read parameters.");
        	fclose(file);
        	return 0;
     		}
    
	}
	
	 //Send the parameters to all other processes. Packed in array to broadcast once.
	 //Could be done with MPI_pack but considering its same data type of parameters using pack would be more trouble than worth.
	 MPI_Bcast(data, NUM_OF_PARAMS, MPI_INT, 0, MPI_COMM_WORLD);
	 //unpack for each process.
	 K = data[0]; N = data[1]; I = data[2];
     
     	// Make sure there are K*K processes
     	if (size != K*K) {
        	 printf("Please run with 9 processes.\n");fflush(stdout);
        	 MPI_Abort(MPI_COMM_WORLD, 1);
     	}
     
    //printf("Rank = %d K = %d, N = %d , I %d\n", rank, K, N, I); fflush(stdout);

	if( rank == 0 ) {
		// Read the word to search aka pattern
     	 	if (fgets(word, sizeof(word), file) == NULL) {
          	perror("Failed to read the word.");
          	fclose(file);
          	return 0;
      		}
      		// Remove newline character
      		word[strcspn(word, "\n")-1] = '\0';

			//Text reduced to lower cases to make search for pattern more practical.
			for(int i =0; i< strlen(word);i++){
				word[i] = tolower(word[i]);
			}
      	
	}
	
	// Send to all processes the word to search for.
	MPI_Bcast(word, 100, MPI_CHAR, 0, MPI_COMM_WORLD); 	
      	//printf("rank  = %d , Read word: %s, len = %ld\n", rank, word, strlen(word));
  
	
	 // 9 processes in a 3x3 grid //
     dim[0] = K; 
	 dim[1] = K;
     period[0] = 1; 
	 period[1] = 1;
     reorder = 1;
     MPI_Cart_create(MPI_COMM_WORLD, 2, dim, period, reorder, &comm);
      
	 // Each process rank and cartesian coordinates
     MPI_Cart_coords(comm, rank, 2, coord);
     //printf("Rank %d coordinates are %d %d\n", rank, coord[0], coord[1]);fflush(stdout);
   
	int textSize = N*2;
    text = (char*)malloc(textSize+1); 
		if(text == NULL){
			printf("Unsuccsesful memory aloc.\n");
			MPI_Abort(MPI_COMM_WORLD,1);
		}

     if(rank==0) {

		/*
		Since each process need recive text from its neighbors, the work wont progres in anyway untill each process recives its initiall text,
		but some processes may start working between each other, while other waiting for text, hence process 0 will read and send the text line by line.
		*/
    	char *line = (char*)malloc(textSize+2); 
		if(line == NULL){
			printf("Unsuccsesful memory aloc.\n");
			MPI_Abort(MPI_COMM_WORLD,1);
		}

		int address =0;
		while (fgets(line, textSize+2, file) != NULL) {
			// Remove newline character
			line[strcspn(line, "\r\n")] = 0;
			if(strlen(line)==0)
				continue;

			for(int i =0; i< strlen(line);i++){
				line[i] = tolower(line[i]);
			}
			//can still send over WORLD communicator, even thou there are cartesian coords.
			if(address > 0)
				MPI_Send(line, strlen(line)+1, MPI_CHAR, address, 0, MPI_COMM_WORLD);
			else
				strcpy(text, line);
			address++;	
			//printf("Read line: |%s| sizeof line =%ld\n", line,strlen(line));
		}


		free(line);
     	fclose(file);
      }

	//each process gets the initial text
	if(rank != 0 ){
		MPI_Recv(text,N*2+1,MPI_CHAR, 0, 0, MPI_COMM_WORLD, &status);
	}
//    printf("Rank %d coordinates are %d %d, the text is = \n |%s|, text length = %ld \n", rank, coord[0], coord[1], text,strlen(text));fflush(stdout);


	//Initial non-mendatory search
	if(kmp_search(text,word) != -1){
		
	printf("\n I found the WORD %s (in original text)!!\nRank %d coordinates are %d %d, the text is = \n |%s|, text length = %ld \n\n", word,rank, coord[0], coord[1], text,strlen(text));fflush(stdout);
	
	}
	char *buffer = (char*)malloc(N+1); 
		if(buffer == NULL){
			printf("Unsuccsesful memory aloc.\n");
			MPI_Abort(MPI_COMM_WORLD,1);
		}

	
    textToUp = (char*)malloc(N+1); 
		if(textToUp == NULL){
			printf("Unsuccsesful memory aloc.\n");
			MPI_Abort(MPI_COMM_WORLD,1);
		}

    textToLeft = (char*)malloc(N+1); 
		if(textToLeft == NULL){
			printf("Unsuccsesful memory aloc.\n");
			MPI_Abort(MPI_COMM_WORLD,1);
		}


	for(int i=0; i< I; i++){
		done = 0;	

		//build the required string to send
		for(int i = 0, up = 0, left = 0; i< strlen(text); i++){
			if(i%2==0){
				textToUp[up] = text[i];
				up++;
			} else {
				textToLeft[left] = text[i];
				left++;
			}
		}

		//horizontal neighbors
		MPI_Cart_shift(comm, 1, 1, &source, &dest);	
		MPI_Sendrecv(textToLeft, N+1,MPI_CHAR, dest, 0, buffer, N+1, MPI_CHAR, source, 0, comm, &status);

		strcpy(text, buffer);

		//vertical neighbors
		MPI_Cart_shift(comm, 0, 1, &source, &dest);
		//printf("Rank = %d Source = %d  Destination %d\n", rank, source, dest); fflush(stdout);
		MPI_Sendrecv(textToUp, N+1,MPI_CHAR, dest, 0, buffer, N+1, MPI_CHAR, source, 0, comm, &status);

		strcat(text, buffer);

		int found = kmp_search(text, word);

		// Tell each other if you finished or not. If pattern will be found, the index of text will be non negative.
		MPI_Allreduce(&found, &done, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

		if(done >= 0){
			if(rank == 0){
				printf("Rank %d coordinates are %d %d, the text is: \n|%s|, text length = %ld \n", rank, coord[0], coord[1], text,strlen(text));fflush(stdout);
				for(int i = 1; i< size; i++){
					//by requirments, process 0 recives text in order
					MPI_Recv(text,N*2+1,MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
					printf("Rank %d coordinates are %d %d, the text is: \n|%s|, text length = %ld \n", i, coord[0], coord[1], text,strlen(text));fflush(stdout);
				
				}
				
			} else {
				MPI_Send(text, N*2+1, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
			}

			break; //if somebody found, everybody go home.

		}
    


	}

	// The last made search didnt find anything
	if(done < 0){
		if(rank == 0)
			printf("After I = %d Iterations The String Was Not Found\n", I);

	}
	

	free(buffer);
	free(text);
	free(textToLeft);
	free(textToUp);
	MPI_Finalize();
    return 0;
     
     
}
