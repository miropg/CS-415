#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "monitor.h"

//int pipe_fd is the read end of a UNIX pipe
void monitor_main(int pipe_fd) {
    //allows us to use fgets on the info 
    FILE *pipe_stream = fdopen(pipe_fd, "r");
    if (!pipe_stream) {
        perror("monitor_main: fdopen");
        return;
    }
    char linebuf[512];
    // 2) Read one line at a time until EOF
    while (fgets(linebuf, sizeof(linebuf), pipe_stream) != NULL) {
        // 3) Remove trailing newline (so we don’t end up with double line breaks)
        linebuf[strcspn(linebuf, "\n")] = '\0';

        // 4) Echo the line with a “[Monitor] ” prefix
        printf("[Monitor] %s\n", linebuf);
        fflush(stdout);
    }

    // 5) Once fgets() returns NULL, the parent has closed mon_pipe[1] → EOF
    //    We close our FILE* and exit.
    fclose(pipe_stream);
}
