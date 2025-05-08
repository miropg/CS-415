#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

/*
 * iobound.c
 *
 * This program simulates an I/O-bound workload. It repeatedly writes output 
 * to a file (in this case, /dev/null) for a specified duration. I/O-bound 
 * tasks spend most of their time waiting on input/output operations such as 
 * file access or network communication rather than performing heavy computation.
 *
 * In contrast, a CPU-bound program (like cpubound) uses most of its time 
 * performing calculations or logic with minimal waiting on external resources.
 *
 * This program is used to test how an operating system schedules and handles 
 * tasks that are limited by I/O operations rather than raw processing power.
 *
 * Usage:
 *   ./iobound -seconds 3
 *   (Runs the I/O-bound simulation for 3 seconds.)
 */

int main(int argc, char **argv) {
    int i, j, now, start, condition = 1, seconds = 5;
    double duration;
    FILE *outfile = fopen("/dev/null", "w");

/*
 * process environment variable and command line arguments
 */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-seconds") == 0) {
            seconds = atoi(argv[i + 1]);
            break;
        } else {
            fprintf(stderr, "Illegal flag: `%s'\n", argv[i]);
            exit(1);
        }
    }

    printf("Process: %d - Begining to write to file.\n", getpid());

    start = clock();
    while(condition) {
      //write a line to the file
      for(j = 0; j<100; j++) {
        fprintf(outfile, "A string! ");
      }
      fprintf(outfile, "\n");
      //check if done
      now = clock();
      duration =  (now - start)/(double) CLOCKS_PER_SEC;
      //printf("Duration: %f - Seconds: %d - condition: %d\n", duration, seconds, duration>=seconds);
      if(duration >= seconds) {condition = 0;}
    }
    fclose(outfile);

    printf("Process: %d - Finished.\n", getpid());
    return 0;
}
