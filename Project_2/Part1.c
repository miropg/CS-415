#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>   // Defines O_WRONLY, O_CREAT, O_TRUNC
#include <unistd.h>  // Defines dup2, STDOUT_FILENO, write, close
#include "string_parser.h"
/*
Part 1: MCP Launches the Workload
"Read commands -> Fork child -> Exec command 
-> Wait for all -> Exit."

Build a program (MCP v1.0) that reads a file of commands, 
forks a child for each one, execs each command, waits for 
them all to finish, and then exits cleanly.
Launching multiple programs at once (like a basic batch job launcher),

1   Read a workload file line-by-line (each line is a command + arguments).
2	For each command, fork() a new child process.
3	In the child process, use exec() to run the command.
4	In the parent process, after launching all children, use wait() 
    or waitpid() to wait for them all to finish.
5	After all processes finish, use exit() to terminate your MCP program.
*/

// trimming white space function
void trim(char *str) {
    if (str == NULL){
        return;
    }
    //trim leading space
    while (isspace((unsigned char)*str)){
        str++;
    }
    //if all spaces
    if (*str == 0){
        return;
    }
    //now trim trailing space
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;

    //write new null terminator
    *(end + 1) = '\0';
}

void launch_workload(const char *filename){
    
    FILE *in_fd = fopen(filename, "r");
    if (!in_fd) {
        const char *err = "failure opening input file\n";
        write(2, err, strlen(err)); 
        return;
    }

    size_t len = 0;
    char *line_buf = NULL;
    int command_ctr = 0;

    //count how many commands there are
    while (getline(&line_buf, &len, in_fd) != -1) {
        trim(line_buf);
        if (strlen(line_buf) > 0) {
            command_ctr++;
        }
    }
    free(line_buf);
    fclose(in_fd);

    //reset ptr to beginning of file
    in_fd = fopen(filename, "r");
    if (!in_fd) {
        const char *err = "failure opening input file\n";
        write(2, err, strlen(err)); 
        return;
    }

    //make room for array of commands
    command_line* file_array = malloc(sizeof(command_line) * command_ctr);
    if (!file_array) {
        const char *err = "malloc failed\n";
        write(2, err, strlen(err));
        free(pids);
        fclose(in_fd);
        return;
    }

    //fill file_array with the commands from input.txt
    len = 0;
    line_buf = NULL;
    for (int i = 0; i < command_ctr; i++) {
        if (getline(&line_buf, &len, in_fd) == -1) {
            const char *err = "unexpected end of file\n";
            write(2, err, strlen(err));
            exit(EXIT_FAILURE);
        }
        trim(line_buf);
        if (strlen(line_buf) > 0) {
            file_array[i] = str_filler(line_buf, " ");
        } else {
            i--; // if blank line, retry
        }
    }
    free(line_buf);
    fclose(in_fd);

    //make space for pool of threads
    pid_t pids = malloc(sizeof(pid_t) * command_ctr);
    if (!pids) {
        const char *err = "malloc failed\n";
        write(2, err, strlen(err));
        fclose(in_fd);
        return;
    }

    //fork children, assign commands to child executables
    for(int i = 0; i < command_ctr; i++){
        pid_t pid = fork();
        pids[i] = pid;
        if(pid == 0) {
            //child process
            printf("I am the child process. My PID: %d\n", getpid());
            execvp(file_array[i].command_list[0], file_array[i].command_list);

            const char *err = "execvp failed\n";
            write(2, err, strlen(err)); 
            exit(EXIT_FAILURE);
        } else if(pid > 0) {
            //parent process
            printf("I am the parent process. The child had PID: %d\n", pid);
        } else {
            const char *err = "fork fail\n";
            write(2, err, strlen(err)); 
            exit(EXIT_FAILURE);
        }
    }
    // wait for all children
    for (int i = 0; i < command_ctr; i++){
        // wait for children by pids
        waitpid(pids[i], NULL, 0);
    }

    //free memory
    for (int i = 0; i < command_ctr; i++) {
        free_command_line(&file_array[i]);
    }
    free(file_array);
    free(pids);
}

int main(int argc, char const *argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <workload_file>\n", argv[0]);
        return 1;
    }
    launch_workload(argv[1]);
    exit(EXIT_SUCCESS);
}