#include "kmp.h"

int* compute_prefix_function(char* pattern) {
    int m = strlen(pattern);
    int* pi = (int*)malloc(m * sizeof(int));
    pi[0] = 0;
    int k = 0;
    for (int q = 1; q < m; q++) {
        while (k > 0 && pattern[k] != pattern[q]) {
            k = pi[k - 1];
        }
        if (pattern[k] == pattern[q]) {
            k++;
        }
        pi[q] = k;
    }
    return pi;
}

int kmp_search(char* text, char* pattern) {
    int n = strlen(text);
    int m = strlen(pattern);
    int* pi = compute_prefix_function(pattern);
    int q = 0;
    for (int i = 0; i < n; i++) {
        while (q > 0 && pattern[q] != text[i]) {
            q = pi[q - 1];
        }
        if (pattern[q] == text[i]) {
            q++;
        }
        if (q == m) {
            free(pi);
            return i - m + 1;
        }
    }
    free(pi);
    return -1;
}

