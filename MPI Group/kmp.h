#ifndef KMP_H
#define KMP_H

/* KMP algorithm to find a substring "pattern" in a string "text" */

#include <stdlib.h>
#include <string.h>

int* compute_prefix_function(char* pattern);
int kmp_search(char* text, char* pattern);

#endif
