#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "monitor.h"

//int pipe_fd is the read end of a UNIX pipe
// In monitor_main, replace its loop with something like:
void monitor_main(int pipe_fd) {
    setvbuf(stdout, NULL, _IOLBF, 0);
    FILE *pipe_stream = fdopen(pipe_fd, "r");
    if (!pipe_stream) {
        perror("monitor_main: fdopen");
        return;
    }

    char bigbuf[4096];
    while (fgets(bigbuf, sizeof(bigbuf), pipe_stream)) {
        pthread_mutex_lock(shared_print_mutex);
        fputs(bigbuf, stdout);
        fflush(stdout);
        pthread_mutex_unlock(shared_print_mutex);
    }
    fclose(pipe_stream);
}

