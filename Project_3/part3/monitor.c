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
    int first_line = 1;
    // Read one line at a time until EOF
    while (fgets(linebuf, sizeof(linebuf), pipe_stream) != NULL) {
        // Remove trailing newline (so we don’t end up with double line breaks)
        linebuf[strcspn(linebuf, "\n")] = '\0';

        if (first_line) {
            fprintf(stderr, "[Monitor] %s\n", linebuf);
            first_line = 0;
        } else {
            fprintf(stderr, "%s\n", linebuf);
        }
        fflush(stderr);
    }
    //Once fgets() returns NULL, the parent has closed mon_pipe[1] -> EOF
    //    We close our FILE* and exit.
    fclose(pipe_stream);
}
