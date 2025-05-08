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
int count_commands_in_file(const char *filename) {
    FILE *in_fd = fopen(filename, "r");
    if (!in_fd) {
        const char *err = "failure opening input file\n";
        write(2, err, strlen(err));
        exit(EXIT_FAILURE);
    }
    int count = 0;
    size_t len = 0;
    char *line_buf = NULL;
    while (getline(&line_buf, &len, in_fd) != -1) {
        trim(line_buf);
        if (strlen(line_buf) > 0) count++;
    }
    free(line_buf);
    fclose(in_fd);
    return count;
}

command_line* read_commands_from_file(const char *filename, int command_ctr) {
    FILE *in_fd = fopen(filename, "r");
    if (!in_fd) {
        const char *err = "failure opening input file\n";
        write(2, err, strlen(err));
        exit(EXIT_FAILURE);
    }
    command_line* file_array = malloc(sizeof(command_line) * command_ctr);
    if (!file_array) {
        const char *err = "malloc failed\n";
        write(2, err, strlen(err));
        exit(EXIT_FAILURE);
    }
    size_t len = 0;
    char *line_buf = NULL;
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
            i--; // retry for blank lines
        }
    }
    free(line_buf);
    fclose(in_fd);
    return file_array;
}

pid_t* allocate_pid_array(int command_ctr) {
    pid_t* pids = malloc(sizeof(pid_t) * command_ctr);
    if (!pids) {
        const char *err = "malloc failed\n";
        write(2, err, strlen(err));
        exit(EXIT_FAILURE);
    }
    return pids;
}

void free_mem(command_line* file_array, int command_ctr, pid_t* pids) {
    printf("\n=== MCP: All child processes completed. Cleaning up. ===\n");

    // Free each parsed command line
    for (int i = 0; i < command_ctr; i++) {
        free_command_line(&file_array[i]);
    }

    // Free the array of command_line structs
    free(file_array);

    // Free the array of PIDs
    free(pids);
}

void launch_workload(const char *filename){
    int command_ctr = count_commands_in_file(filename);
    command_line* file_array = read_commands_from_file(filename, command_ctr);
    pid_t* pids = allocate_pid_array(command_ctr);

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
    free_mem(file_array, command_ctr, pids);
}

int main(int argc, char const *argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <workload_file>\n", argv[0]);
        return 1;
    }
    launch_workload(argv[1]);
    exit(EXIT_SUCCESS);
}

/*
Example run with: ./part1 input.txt

// Parent forked child process 1
I am the parent process. The child had PID: 1043686
// Child 1 starts
I am the child process. My PID: 1043686

// Parent forked child process 2
I am the parent process. The child had PID: 1043687
// Child 2 starts
I am the child process. My PID: 1043687

// Parent forked child process 3 (invalid command will happen here)
I am the parent process. The child had PID: 1043688

// Parent forked child process 4 (iobound)
I am the parent process. The child had PID: 1043689
// Child 3 (invalid command) starts
I am the child process. My PID: 1043688

// Parent forked child process 5 (cpubound)
I am the parent process. The child had PID: 1043690
// Child 4 (iobound) starts
I am the child process. My PID: 1043689
// Child 5 (cpubound) starts
I am the child process. My PID: 1043690

// Child 3 tried to run an invalid command (execvp failed)
execvp failed

// iobound output
Process: 1043689 - Begining to write to file.

// Output from the `ls` command (child 1 or 2, depending on input.txt order)
total 139
 7 string_parser.o   1 main.c      7 cpubound.c   7 Part2.c    1 EX.c
 7 string_parser.h   7 iobound.c  12 cpubound    12 Part1.o   12 ..
 7 string_parser.c  12 iobound     7 Part4.c      7 Part1.c   12 .
12 part1             1 input.txt   7 Part3.c      7 Makefile

// cpubound output
Process: 1043690 - Begining calculation.

// Done messages from iobound and cpubound
Process: 1043690 - Finished.
Process: 1043689 - Finished.
*/
