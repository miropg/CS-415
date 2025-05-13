#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>   // Defines O_WRONLY, O_CREAT, O_TRUNC
#include <unistd.h>  // Defines dup2, STDOUT_FILENO, write, close
#include <signal.h>
#include <stdbool.h>
#include "string_parser.h"
/*
MCP v3.0 must act like a real CPU scheduler: let each process run for a short
time, then pause it and switch to the next, over and over, until all are done.
You are building a basic multitasking system now!

1	Upgrade MCP v2.0 into MCP v3.0 — add scheduling (time-sharing).
2	Give each process a small time slice to run (e.g., 1 second each).
3	Use alarm() to set a timer that delivers a SIGALRM after the time 
    slice expires.
4	When the MCP receives a SIGALRM, it must:
4a	Send SIGSTOP to suspend the currently running process.
4b	Pick the next ready process (Round Robin or similar policy).
4c	Send SIGCONT to resume the next process.
4d	Set another alarm(1) to repeat the cycle.
5	Keep doing this until all workload processes finish.
*/

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

void signaler(pid_t* pids, int size, int signal) {
    for (int i = 0; i < size; i++) {
        // Sleep 3 seconds before signaling each child
        // This simulates slow, staggered scheduling
        sleep(3); // Lab: delay between signals
        printf("Parent process: %d - Sending signal: %s to child process: %d\n",
               getpid(), strsignal(signal), pids[i]);
        // Send the specified signal to the current child
        // kill(pid, signal) tells the OS to deliver 'signal' to 'pid'
        kill(pids[i], signal);
    }
}

//PART 3 CODE
void coordinate_children(pid_t* pids, int command_ctr) {
    printf("\n=== MCP: Sending SIGUSR1 to all children ===\n");
    signaler(pids, command_ctr, SIGUSR1);  // Signal children to exec

    // Optional pause to give time for exec to start
    printf("\n=== MCP: Sleeping briefly to let children begin workloads ===\n");
    sleep(1);

    printf("\n=== MCP: Sending SIGSTOP to all children ===\n");
    signaler(pids, command_ctr, SIGSTOP);  // Pause the processes

    printf("\n=== MCP: Sending SIGCONT to all children ===\n");
    signaler(pids, command_ctr, SIGCONT);  // Resume processes

    printf("\n=== MCP: Sending SIGINT to all children ===\n");
    signaler(pids, command_ctr, SIGINT);   // Gracefully kill the children

    // Wait for all children to finish
    printf("\n=== MCP: Waiting for all children to complete ===\n");
    for (int i = 0; i < command_ctr; i++) {
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
            printf("Child Process: %d - Waiting for SIGUSR1...\n", getpid());

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
            printf("Child Process: %d - Received signal: SIGUSR1 - Calling exec().\n", getpid());
            //child process
            printf("I am the child process. My PID: %d\n", getpid());
            execvp(file_array[i].command_list[0], file_array[i].command_list);
            
            const char *err = "execvp failed\n";
            write(2, err, strlen(err)); 
            exit(EXIT_FAILURE);
        } else if(pid > 0) {
            //parent process
            //printf("I am the parent process. The child had PID: %d\n", pid);
            printf("MCP: Forked child PID %d for command: %s\n", pid, file_array[i].command_list[0]);
        } else {
            const char *err = "fork fail\n";
            write(2, err, strlen(err)); 
            exit(EXIT_FAILURE);
        }
    }
    //PART 3 CODE
    //rr_alive = command_ctr; //forked child processes still running for RR
    coordinate_children(pids, command_ctr); 
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
/*
//avoids infinite loops by scanning in a circular way and explicitly checks if we
//  looped all the way back to the original process.

//safely handles the case where only one process is still running.

//avoids rescheduling any process that’s already rr_completed[] = true.
void signal_alarm(int signum) {
    // Don't try to stop a process that already exited
    if (!rr_completed[rr_current]) {
        printf("MCP: Time slice expired. Stopping PID %d\n", rr_pids[rr_current]);
        kill(rr_pids[rr_current], SIGSTOP);
    } else {
        printf("MCP: Current PID %d already completed. Skipping stop.\n", rr_pids[rr_current]);
    }

    // Find next non-completed process (full circular scan)
    int next = (rr_current + 1) % rr_num_procs;
    while (next != rr_current && rr_completed[next]) {
        next = (next + 1) % rr_num_procs;
    }

    if (rr_completed[next]) {
        // Only one process was left and now it's done
        printf("MCP: No runnable processes remain to schedule.\n");
        return;
    }

    rr_current = next;
    printf("MCP: Switching to PID %d\n", rr_pids[rr_current]);
    kill(rr_pids[rr_current], SIGCONT);
    alarm(1);
}

//need an array of bools to track which 
//processes have already exited, so the scheduler can skip over them.
void round_robin(){
    printf("MCP: Starting Round Robin scheduling...\n");

    signal(SIGALRM, signal_alarm);
    //start with 1st process
    rr_current = 0;
    //start 1st child process
    kill(rr_pids[rr_current], SIGCONT);
    alarm(1); // Sets a timer that sends SIGALRM after 1 second

    //for sigwait() to work
    sigset_t sigset;
    int sig;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGCHLD);
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);
    //loop until all processes finish, while they are alive
    while (rr_alive > 0) {
        int status;
        // 
        pid_t done_pid = waitpid(-1, &status, WNOHANG);
        //Wifeexited Identifies a child that has exited normally
        //(not stopped or killed by a signal)
        if (done_pid > 0 && WIFEXITED(status)) {
            //child has exited, marke it complete
            for (int i = 0; i < rr_num_procs; i++) {
                if (rr_pids[i] == done_pid) {
                    rr_completed[i] = true;
                    rr_alive--; //derecement count of live processes
                    printf("MCP: Process PID %d finished. Remaining: %d\n", done_pid, rr_alive);
                    break;
                }
            }
        }
        sigwait(&sigset, &sig); //wait for next SIGALRM or SIGCHLD
    }
}