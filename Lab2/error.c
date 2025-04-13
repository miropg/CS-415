#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "error.h"

/*
(File descriptor (where to write data), ptr to buf, num bytes to write);
FD	            Meaning
0	    stdin (standard input)
1	    stdout (standard output) writes in the file
2	    stderr (standard error)
*/
//error for an unrecognized command
void err_unrecognized(const char *cmd){
    write(1, "Error! Unrecognized command: ", strlen("Error! Unrecognized command: "));
    write(1, cmd, strlen(cmd));
    write(1, "\n", 1);
    //fprintf(stderr, "Error! Unrecognized command: %s\n", cmd);
}

//error for incorrect parameter count
void err_params(const char *cmd){
    write(1, "Error! Unsupported parameters for command: ", strlen("Error! Unsupported parameters for command: "));
    write(1, cmd, strlen(cmd));
    write(1, "\n", 1);
    //fprintf(stderr, "Error! Unsupported parameters for command: %s\n", cmd);
}

//checks if the command is one of the known commands
int valid_command(const char *cmd){
    const char *valid_cmds[] = {
        "ls", "pwd", "mkdir", "cd", "cp", "mv", "rm", "cat"
    };
    int num_cmds = sizeof(valid_cmds) / sizeof(valid_cmds[0]);

    for (int i = 0; i < num_cmds; i++) {
        if (strcmp(cmd, valid_cmds[i]) == 0) {
            return 1; //valid cmd
        }
    }
    return 0; // not a valid cmd from out options
}
//boolean functions, will return True if the specific command the user entered has
// correct number of parameters
int param_count_valid(const char *cmd, int arg_count){
    if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "pwd") == 0  
                                || strcmp(cmd, "lfcat")) {
        return arg_count == 1; // this is just command, no parameters
    } else if (strcmp(cmd, "cd") == 0 || strcmp(cmd, "mkdir") == 0 ||
            strcmp(cmd, "rm") == 0 || strcmp(cmd, "cat") == 0) {
        return arg_count == 2; // commands that need one additonal param
    } else if (strcmp(cmd, "cp") == 0 || strcmp(cmd, "mv") == 0) {
        return arg_count == 3; // commands that have 2 additional param
    }
    return 0;
}
