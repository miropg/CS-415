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

void send_signal_to_children(pid_t* pids, int count, int signal, const char* label) {
    for (int i = 0; i < count; i++) {
        kill(pids[i], signal);
        printf("MCP sent %s to PID: %d\n", label, pids[i]);
    }
}

void coordinate_children(pid_t* pids, int command_ctr) {
    // All children are forked and waiting
    // send SIGUSR1 to all children
    printf("\n=== MCP: Sending SIGUSR1 to all children ===\n");
    send_signal_to_children(pids, command_ctr, SIGUSR1, "SIGUSR1");

    //sleep so that parent does not send SIGSTOP too early, before
    // the child is running the actual command
    printf("\n=== MCP: Sleeping briefly to let children begin workloads ===\n");
    sleep(1);

    //pause the children with SIGSTOP
    printf("\n=== MCP: Sending SIGSTOP to all children ===\n");
    send_signal_to_children(pids, command_ctr, SIGSTOP, "SIGSTOP");

    //let children continue work
    printf("\n=== MCP: Sending SIGCONT to all children ===\n");
    send_signal_to_children(pids, command_ctr, SIGCONT, "SIGCONT");

    // wait for all children to finish
    printf("\n=== MCP: Waiting for all children to complete ===\n");
    for (int i = 0; i < command_ctr; i++){
        waitpid(pids[i], NULL, 0);
    }
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
    coordinate_children(pids, command_ctr); //PART 2 CODE HERE
    free_mem(file_array, command_ctr, pids);
}
//MCP: Master Controller Process
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