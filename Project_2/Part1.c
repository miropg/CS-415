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
        const char *err = "failier opening input file\n";
        write(2, err, strlen(err)); //does this need write?
        return;
    }
    // Allocate a buffer for reading lines
    // allocate memory on the heap
    size_t len = 0;
    char *line_buf = NULL;
    pid_t pids[100]; //max 100 processes 
    int count = 0;
    //Token Type   Split By	     Represents	             Example
    //Small Token	 " "	  Command + each argument	 "cd", "folder"
    command_line s_tok_buf;

    //getline reads a line from batch file, stores the line in a buffer
    //pointed to by line_buf and updates len w/ current allocated size of buffer
    while (getline(&line_buf, &len, in_fd) != -1) {
        trim(line_buf); //trim any white space away

        command_line s_tok_buf = str_filler(line_buf, " ");
        
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            //execvp(const char *file, char *const argv[])
            execvp(s_tok_buf.command_list[0], s_tok_buf.command_list);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        } else {
            pids[count++] = pid;
        }
        free_command_line(&s_tok_buf);
    }
    for (int i = 0; i < count; i++) {
        waitpid(pids[i], NULL, 0);
    }
    free(line_buf);
    fclose(in_fd);
}

int main(int argc, char const *argv[]){
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <workload_file>\n", argv[0]);
        return 1;
    }
    launch_workload(argv[1]);
}