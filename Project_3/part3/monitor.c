#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "monitor.h"

//int pipe_fd is the read end of a UNIX pipe
// In monitor_main, replace its loop with something like:
void monitor_main(int pipe_fd) {
    FILE *pipe_stream = fdopen(pipe_fd, "r");
    if (!pipe_stream) {
        perror("monitor_main: fdopen");
        return;
    }

    char bigbuf[4096];
    while (fgets(bigbuf, sizeof(bigbuf), pipe_stream)) {
        fprintf(stderr, "%s", bigbuf);
        fflush(stderr);
    }
    fclose(pipe_stream);
}

