#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>


bool choosing[0..n-1];   // All set to FALSE initially
int number[0..n-1];      // All set to 0 initially

void enterCS(int myid) {
    choosing[myid] = true;
    number[myid] = 1 + max(number[0], number[1], ..., number[n-1]);
    choosing[myid] = false;

    for (int j = 0; j < n; j++) {
        while (choosing[j]);  // Wait if process j is choosing its number
        while (number[j] != 0 && 
              (number[j] < number[myid] || 
              (number[j] == number[myid] && j < myid))) {
            // Wait if process j has higher priority
        }
    }
}

void exitCS(int myid) {
    number[myid] = 0;  // Release ticket number
}
