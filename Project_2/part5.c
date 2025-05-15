#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>   // Defines O_WRONLY, O_CREAT, O_TRUNC
#include <unistd.h>  // Defines dup2, STDOUT_FILENO, write, close
#include <signal.h>
#include <stdbool.h>
#include <sys/time.h>  // For gettimeofday
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

//global variables for alarm handler
#define NUM_MAX 100  // Or some safe upper bound if you don't know the real number yet

//Part 5 code
ProcInfo proc_info[NUM_MAX];
int proc_count = 0;

typedef struct {
    pid_t pid; // The process ID
    double total_cpu_seconds;  // Total CPU time used (user + system), converted to seconds
    int assigned_slice; // Time slice given to this process (based on priority)
} ProcInfo;

//queue for Round Robin
typedef struct {
    pid_t data[NUM_MAX];
    int front;
    int rear;
    int size;
    //finished or not
} Queue;

void init_queue(Queue* q) {
    q->front = 0;
    q->rear = 0;
    q->size = 0;
}

bool is_empty(Queue* q) {
    return q->size == 0;
}

bool is_full(Queue* q) {
    return q->size == NUM_MAX;
}

void enqueue(Queue* q, pid_t pid) {
    if (is_full(q)) return;
    q->data[q->rear] = pid;
    q->rear = (q->rear + 1) % NUM_MAX;
    q->size++;
}

pid_t dequeue(Queue* q) {
    if (is_empty(q)) return -1;
    pid_t pid = q->data[q->front];
    q->front = (q->front + 1) % NUM_MAX;
    q->size--;
    return pid;
}

void print_queue(Queue* q) {
    if (is_empty(q)) {
        return;
    }
    int count = q->size;
    int index = q->front;
    while (count > 0) {
        index = (index + 1) % NUM_MAX;
        count--;
    }
}

Queue queue;
pid_t current_process; 
//int current_process = 0 //index of current active child (on CPU)
// int pids[NUM_MAX]; // I don't know if we need this, we have false array already
// bool rr_done[NUM_MAX] = { false };
// int num_children = 0;  // Total number of child processes, set later

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

void update_cpu_stats(){
    for (int i = 0; i < proc_count; i++){
        char path[64];
        sprintf(path, "/proc/%d/stat", proc_info[i].pid);
        FILE* fp = fopen(path, "r");
        if (!fp) continue;
        
        //hold the parsed data
        long utime_ticks = 0, stime_ticks = 0;
        int dummy;
        char comm[256], state;

        fscanf(fp, "%d %s %c", &dummy, comm, &state);
        // Skip to field 14 (utime) and 15 (stime)
        for (int j = 0; j < 11; j++) fscanf(fp, "%*s"); // Skip next 11 fields

        fscanf(fp, "%ld %ld", &utime_ticks, &stime_ticks);
        fclose(fp);

        double total_sec = (utime_ticks + stime_ticks) / (double)sysconf(_SC_CLK_TCK);
        proc_info[i].total_cpu_seconds = total_sec;

        // Optional: Adjust slice — here longer CPU time gives longer slice (simple linear)
        proc_info[i].assigned_slice = (int)(total_sec + 1);  // ensure minimum of 1s
    }
}

void print_table_header() {
    printf("\nPID     | State           | VmSize (kB) | VmRSS (kB) | Vol/NonVol ctxt switches\n");
    printf("--------|-----------------|-------------|------------|--------------------------\n");
}

void print_process_status(pid_t pid, pid_t current_pid) {
    char path[64], line[256];
    sprintf(path, "/proc/%d/status", pid);

    FILE* fp = fopen(path, "r");
    if (!fp) return;

    char state[32] = "";
    long vm_size = -1, vm_rss = -1;
    int voluntary_ctxt = -1, nonvoluntary_ctxt = -1;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "State:", 6) == 0) {
            sscanf(line, "State:\t%31[^\n]", state);
        } else if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize:\t%ld", &vm_size);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS:\t%ld", &vm_rss);
        } else if (strncmp(line, "voluntary_ctxt_switches:", 24) == 0) {
            sscanf(line, "voluntary_ctxt_switches: %d", &voluntary_ctxt);
        } else if (strncmp(line, "nonvoluntary_ctxt_switches:", 27) == 0) {
            sscanf(line, "nonvoluntary_ctxt_switches: %d", &nonvoluntary_ctxt);
        }
    }
    fclose(fp);
    const char *running_marker = (pid == current_pid) ? "<-- RUNNING" : "";
    printf("%-8d| %-16s| %-12ld| %-11ld| %4d / %4d %s\n",
       pid, state, vm_size, vm_rss, voluntary_ctxt, nonvoluntary_ctxt, running_marker);
}

void alarm_handler(int sig) {
    // PREEMPTION PHASE
    // 1. The MCP will suspend the currently running workload process using SIGSTOP.
    //if current proess has not finished yet...
    int finished = kill(current_process, SIGSTOP);
        // send SIGSTOP to pause the currently running child process
        // simulating a "preemption" -stopping process so another can run
    // int status; // check if process exited...
    int pid_running = kill(current_process, 0);
    
    // pid_t result = waitpid(current_process, &status, WNOHANG); // check cont.
    
    if (pid_running != 0 || finished < 0) {
        // Process finished — don't requeue
    } else {
        // Still running — requeue it
        enqueue(&queue, current_process);
    }
    update_cpu_stats();  //PART 5 CODE
    print_table_header();
    for (int i = 0; i < queue.size; i++) {
        int index = (queue.front + i) % NUM_MAX;
        pid_t pid = queue.data[index];
        print_process_status(pid, current_process);
    }
    // 2. Decide on the next process to run
    if (!is_empty(&queue)) {
        current_process = dequeue(&queue);
        kill(current_process, SIGCONT);  // Resume next process
        // 3. Reset the alarm for the next time slice
        int slice = 1;
        for (int i = 0; i < proc_count; i++) {
            if (proc_info[i].pid == current_process) {
                slice = proc_info[i].assigned_slice;
                break;
            }
        }
        alarm(slice);
    }   
}

void run_scheduler(command_line* file_array, int command_ctr){
    // Register the alarm handler to receive SIGALRM after each time slice.
    signal(SIGALRM, alarm_handler);	

    // Step 1: Start all child processes by sending SIGUSR1.
    // This unblocks their sigwait() and allows them to call execvp().
    for (int i = 0; i < command_ctr; i++) {
        kill(queue.data[i], SIGUSR1);  // Trigger child to start
    }	

    for (int i = 0; i < command_ctr; i++) {
        kill(queue.data[i], SIGSTOP);  // Freeze them until we send SIGCONT
    }
    // PUT the PROCESSES ON THE CPU
    // Step 2: Begin scheduling — start the first process by dequeuing it and sending SIGCONT.
    current_process = dequeue(&queue); //get 1st process to run
    kill(current_process, SIGCONT); //resume/start it
    alarm(1); //start 1 second time slice

    // Step 3: Reap exited children so they don't become zombies.
    // We block here using wait(), which pauses the parent MCP until a child exits.
    int status;
    int exited = 0;
    while (exited < command_ctr) {
        pid_t pid = wait(&status);
        if (pid > 0) {
            exited++;
        }
    }
}

void free_mem(command_line* file_array, int command_ctr,  pid_t* pids) {

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

    init_queue(&queue); //need for queue to work?

    //fork children, assign commands to child executables
    for(int i = 0; i < command_ctr; i++){
        sigset_t sigset; //a data structure to hold set of signals
        sigemptyset(&sigset); //set signal set to empty
        sigaddset(&sigset, SIGUSR1); //only respond to SIGUSR1 signal (only signal in set)
        sigprocmask(SIG_BLOCK, &sigset, NULL);
        int sig; //variable to store signal
            
        pid_t pid = fork();
        pids[i] = pid;
        //PART 5 code
        if (pid > 0) {  // Parent process
            // Parent: record info about this child
            proc_info[i].pid = pid;
            proc_info[i].total_cpu_seconds = 0.0;
            proc_info[i].assigned_slice = 1;  // Default time slice
            proc_count++;  // Optional, if you use this to track how many children
        }
        if (pid < 0){
			fprintf(stderr, "fork failed\n");
			exit(-1);
		}
        else if(pid == 0) {
            //printf("Child Process: %d - Waiting for SIGUSR1...\n", getpid());
            
			if (sigwait(&sigset, &sig) != 0) {
                perror("sigwait failed");
                exit(EXIT_FAILURE);
            }
            // === SUPPRESS LS === 
            int null_fd = open("/dev/null", O_WRONLY);
            if (null_fd != -1) {
                dup2(null_fd, STDOUT_FILENO);
                dup2(null_fd, STDERR_FILENO);
                close(null_fd);
            }
			execvp(file_array[i].command_list[0], file_array[i].command_list);
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }        
    } 
    for(int i = 0; i < command_ctr; i++){
        //1st command in input should be 1st command in queue
        enqueue(&queue, pids[i]); 
    }
         
    print_queue(&queue);
    struct timeval start_time, end_time;
    gettimeofday(&start_time, NULL);
    run_scheduler(file_array, command_ctr);
    print_queue(&queue);
    gettimeofday(&end_time, NULL);

    long seconds = end_time.tv_sec - start_time.tv_sec;
    long micros = end_time.tv_usec - start_time.tv_usec;

    if (micros < 0) {
        seconds -= 1;
        micros += 1000000;
    }

    printf("\nTotal runtime: %ld.%06ld seconds\n", seconds, micros);
    free_mem(file_array, command_ctr, pids);
}
//MCP: Master Controller Process
int main(int argc, char const *argv[]){
    if (argc != 2) {
        //fprintf(stderr, "Usage: %s <workload_file>\n", argv[0]);
        return 1;
    }
    launch_workload(argv[1]);
    exit(EXIT_SUCCESS);
}

/*
use priority queue for priority round robin
choose some stat in the stat file to be the priority
CPU time printed out, whichever has higher 
*/
/*
I/O-bound processes:

Spend more time blocked on I/O (e.g., disk, terminal, network).

Appear with lower CPU usage in /proc/[pid]/stat.

→ So their calculated total_cpu_seconds is lower.

→ You assign them shorter slices via assigned_slice = total_cpu_seconds + 1.

CPU-bound processes:

Spend most of their time using the CPU.

Appear with higher CPU usage in /proc/[pid]/stat.

→ So total_cpu_seconds is higher.

→ You assign them longer slices.*/