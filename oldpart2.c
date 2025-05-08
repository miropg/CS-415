#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>   // Defines O_WRONLY, O_CREAT, O_TRUNC
#include <unistd.h>  // Defines dup2, STDOUT_FILENO, write, close
#include "string_parser.h"
/*
Part 2
(reuse part 1 structure but modify it)
Build MCP v2.0 so that it can pause and resume child processes using signals 
(SIGUSR1, SIGSTOP, SIGCONT), instead of immediately launching everything blindly.
Now you are controlling the start, stop, & continue behavior of each child

1	After fork(), do not immediately exec(). Instead, make each child wait for
    a SIGUSR1 signal before calling exec(). (Use sigwait().)
2	After all children are forked and waiting, MCP sends SIGUSR1 to all ➔
    children wake up and exec() their workload commands.
//ensures all children created first
//none of them start running their actual commands execvp() until the parent MCP
//explicitely allows them to, increasing synchronization

3	Once all workload programs are launched, MCP sends SIGSTOP to each child ➔
    this suspends them.
4	Then MCP sends SIGCONT to each child ➔ this resumes them.
5	After resuming, MCP waits for all children to finish with wait()/waitpid(),
    then exits cleanly and frees memory.
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
    pid_t* pids = malloc(sizeof(pid_t) * command_ctr);
    if (!pids) {
        const char *err = "malloc failed\n";
        write(2, err, strlen(err));
        return;
    }

    //fork children, assign commands to child executables
    for(int i = 0; i < command_ctr; i++){
        pid_t pid = fork();
        pids[i] = pid;
        if(pid == 0) {
// PART 2 CODE HERE
            printf("Child PID: %d waiting for SIGUSR1...\n", getpid());

            sigset_t sigset; //a data structure to hold set of signals
            sigemptyset(&sigset); //set signal set to empty
            //only respond to SIGUSR1 signal (only signal in set)
            sigaddset(&sigset, SIGUSR1); 
            int sig; //variable to store signal

            // Block SIGUSR1 and wait for it to be delivered by the parent
            //siprocmask(how, set, oldset)
            //SIG_BLOCK: add these signals to my blocked list
            sigprocmask(SIG_BLOCK, &sigset, NULL);
            if (sigwait(&sigset, &sig) != 0) {
                const char *err = "sigwait failed\n";
                write(2, err, strlen(err));
                exit(EXIT_FAILURE);
            }
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
    // All children are forked and waiting
    // send SIGUSR1 to all children
    printf("\n=== MCP: Sending SIGUSR1 to all children ===\n");
    for (int i = 0; i < command_ctr; i++) {
        //kill(process ID, the signal to send)
        //kill gives the OK for the threads to continue
        printf("MCP sending SIGUSR1 to PID: %d\n", pids[i]);
        kill(pids[i], SIGUSR1);
    }

    //sleep so that parent does not send SIGSTOP too early, before
    // the child is running the actual command
    printf("\n=== MCP: Sleeping briefly to let children begin workloads ===\n");
    sleep(1);

    //pause the children with SIGSTOP
    printf("\n=== MCP: Sending SIGSTOP to all children ===\n");
    for (int i = 0; i < command_ctr; i++){
        kill(pids[i], SIGSTOP);
        printf("MCP sent SIGSTOP to PID: %d\n", pids[i]);
    }

    //let children continue work
    printf("\n=== MCP: Sending SIGCONT to all children ===\n");
    for (int i = 0; i < command_ctr; i++) {
        kill(pids[i], SIGCONT);
        printf("MCP sent SIGCONT to PID: %d\n", pids[i]);
    }

    // wait for all children to finish
    printf("\n=== MCP: Waiting for all children to complete ===\n");
    for (int i = 0; i < command_ctr; i++){
        waitpid(pids[i], NULL, 0);
    }

    //free memory
    printf("\n=== MCP: All child processes completed. Cleaning up. ===\n");
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

//order of operations for processes
/*
Create all child processes before letting any of them start.

kill(pids[i], SIGUSR1);
Synchronize the start — ensure no child runs its command before the 
parent says “Go.”

short pause. (BLOCKED)
sleep(1);
Prevent the parent from sending SIGSTOP too early, which could freeze a child 
before its workload starts.

process gets suspended by the OS, gets no CPU time
kill(pids[i], SIGSTOP);
Simulate pausing all processes — like an OS might for scheduling, 
debugging, or synchronization testing.

MCP sends SIGCONT to all children:
Resumes the execution of each previously paused process.
Children continue from exactly where they were stopped.

MCP waits for all children to finish:
Uses waitpid() on each child.
Ensures proper cleanup and no zombie processes remain.

Cleanup.
Frees all dynamically allocated memory (commands, PID array).
MCP exits cleanly.
*/