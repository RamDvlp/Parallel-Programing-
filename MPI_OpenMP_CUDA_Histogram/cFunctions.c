#include "myProto.h"
#include <stdio.h>

void test(int *histogram, int* data, int N) {
    int tempHist[256];
    

    for (int i = 0;   i < 256;   i++) {
        tempHist[i] =0;
        
    }

    for (int i = 0;   i < N;   i++) {
        tempHist[data[i]]++;
        
    }

    for(int i = 0; i < 256; i++){
        // Uncomment the line to see parallel and linear values of histogram
        //printf(" %d paralel hist %d | linear hist %d\n", i, histogram[i], tempHist[i]);
        if (histogram[i] != tempHist[i]) {
            
           printf("Wrong Calculations - Failure of the test at data[%d]\n", i);
           return;
    	}
    }
    
    printf("The test passed successfully\n"); 
}
