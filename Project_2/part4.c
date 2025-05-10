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
Part 4
After each time slice, MCP must act like a mini top, reading /proc, 
picking out meaningful info, and printing it cleanly.
You are adding live system monitoring to your scheduler

1	After each scheduler cycle (each time slice), gather information from 
    /proc for every workload process.
2	Pick useful information like: execution time, memory usage, I/O stats.
3	Analyze and nicely format the output (not just raw dump).
4	Display an updated table of stats every cycle (kind of like Linux top).
5	Continue gathering and showing stats until all processes finish.
*/
//Round Robin Global Variables
static pid_t* rr_pids = NULL;         // Global pointer to child PIDs
static int rr_num_procs = 0;          // Total number of children
static int rr_current = 0;            // Current process index
static bool* rr_completed = NULL;     // tracks which children are done
static int rr_alive = 0;              // number of children still alive

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

//PART 4 Code
void proc_stats_header() {
    printf("%-8s %-20s %-6s %-10s %-10s\n", "PID", "NAME", "STATE", "CPU(s)", "MEM(KB)");
    printf("---------------------------------------------------------------\n");
}

//PART 4 Code
void print_proc_stats(pid_t pid) {
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) {
        perror("fopen");
        return;
    }

    int pid_read;
    char comm[256];
    char state;
    unsigned long utime, stime;
    long rss;

    // Skip unneeded fields, extract only what we want
    fscanf(fp, "%d %s %c", &pid_read, comm, &state);
    for (int i = 0; i < 10; i++) fscanf(fp, "%*s"); // skip 10 fields
    fscanf(fp, "%lu %lu", &utime, &stime);          // fields 14 and 15
    for (int i = 0; i < 7; i++) fscanf(fp, "%*s");   // skip to rss
    fscanf(fp, "%ld", &rss);                        // field 24

    fclose(fp);

    long page_size_kb = sysconf(_SC_PAGESIZE) / 1024;
    double cpu_seconds = (utime + stime) / (double)sysconf(_SC_CLK_TCK);
    long mem_kb = rss * page_size_kb;

    printf("%-8d %-20s %-6c %-10.2f %-10ld\n", pid_read, comm, state, cpu_seconds, mem_kb);
}

void send_signal_to_children(pid_t* pids, int count, int signal, const char* label) {
    for (int i = 0; i < count; i++) {
        kill(pids[i], signal);
        printf("MCP sent %s to PID: %d\n", label, pids[i]);
    }
}

void signal_alarm(int signum) {
    // code to try and let it finish the last one without resending signal
    // did not work, just let it reschedule same process even if it wastes
    // CPU time
    // if (rr_alive == 1) {
    //     // Only one process left, let it finish naturally instead of
    //     // context switching back onto itself over and over
    //     return; // skip everything below
    // }
    
    printf("MCP: Time slice expired. Stopping PID %d\n", rr_pids[rr_current]);
    kill(rr_pids[rr_current], SIGSTOP); // Pause current process

    // find next alive process
    int next = (rr_current + 1) % rr_num_procs;
    while (rr_completed[next]) {
        next = (next + 1) % rr_num_procs;
    }
    rr_current = next;
    printf("MCP: Switching to PID %d\n", rr_pids[rr_current]);
    kill(rr_pids[rr_current], SIGCONT); // Resume next
    alarm(1); // Set up next time quantum
}

/*Your MCP 4.0 must output the analyzed process information 
for every child process each time the scheduler completes a cycle."*/

/*need an array of bools to track which 
processes have already exited, so the scheduler can skip over them.*/
void round_robin(){
    printf("MCP: Starting Round Robin scheduling...\n");
    signal(SIGALRM, signal_alarm);
    //start with 1st process
    rr_current = 0;
    //start 1st child process
    kill(rr_pids[rr_current], SIGCONT);
    alarm(1); // Sets a timer that sends SIGALRM after 1 second

    //loop until all processes finish, while they are alive
    while (rr_alive > 0) {
        int status;
        pid_t done_pid = waitpid(-1, &status, WNOHANG);
        if (done_pid > 0) {
            //child has exited, mark it complete
            for (int i = 0; i < rr_num_procs; i++) {
                if (rr_pids[i] == done_pid) {
                    // print_proc_stats(done_pid);
                    rr_completed[i] = true;
                    rr_alive--; //derecement count of live processes
                    printf("MCP: Process PID %d finished. Remaining: %d\n", done_pid, rr_alive);
                    break;
                }
            }
        }
        // print live stats
        // only print processes that are still alive, otherwise you get a file-not-found error
        printf("\n=== MCP: Resource Usage for Processes ===\n");
        proc_stats_header();
        for (int i = 0; i < rr_num_procs; i++) {
            if (!rr_completed[i]) {
                print_proc_stats(rr_pids[i]);
            }    
        }
        //improve amount of busy waiting
        usleep(100000); // 10ms
    }
}

//PART 3 CODE
//Implementing Round Robin
void coordinate_children(pid_t* pids, int command_ctr) {
    //round robin variables
    rr_pids = pids;
    rr_num_procs = command_ctr;
    rr_completed = malloc(sizeof(bool) * command_ctr);
    if (!rr_completed) {
        const char *err = "malloc failed for rr_completed\n";
        write(2, err, strlen(err));
        exit(EXIT_FAILURE);
    }
    //initialize booleans all to false, True means process is completed
    for (int i = 0; i < command_ctr; i++) {
        rr_completed[i] = false;
    }
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

    // We do not resume all children at once anymore with a for loop
    // printf("\n=== MCP: Sending SIGCONT to all children ===\n");
    // send_signal_to_children(pids, command_ctr, SIGCONT, "SIGCONT");
    // instead SCHEDULING
    round_robin();

    // wait for all children to finish
    printf("\n=== MCP: Waiting for all children to complete ===\n");
    for (int i = 0; i < command_ctr; i++){
        waitpid(pids[i], NULL, 0);
    }
    //free boolean array keeping track of which processes have exited CPU
    free(rr_completed);
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
            //printf("I am the parent process. The child had PID: %d\n", pid);
            printf("MCP: Forked child PID %d for command: %s\n", pid, file_array[i].command_list[0]);
        } else {
            const char *err = "fork fail\n";
            write(2, err, strlen(err)); 
            exit(EXIT_FAILURE);
        }
    }
    //PART 3 CODE
    rr_alive = command_ctr; //forked child processes still running for RR
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
// notes:
//fopen(/proc/pinnumber)