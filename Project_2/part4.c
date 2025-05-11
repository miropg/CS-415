#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/wait.h>
#include <fcntl.h>   // Defines O_WRONLY, O_CREAT, O_TRUNC
#include <unistd.h>  // Defines dup2, STDOUT_FILENO, write, close
#include <signal.h>
#include <stdbool.h>
#include <sys/types.h>
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

//Round Robin Global Variables
static pid_t* rr_pids = NULL;         // Global pointer to child PIDs
static int rr_num_procs = 0;          // Total number of children
static int rr_current = 0;            // Current process index
static bool* rr_completed = NULL;     // tracks which children are done
static int rr_alive = 0;              // number of children still alive

static int num_done     = 0;      /* children that have finished */
static bool *rr_started   = NULL;      /* first‑time vs resumed */
static volatile sig_atomic_t should_redraw = 0;   /* ask main loop to repaint */

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
void print_current_process_stats(pid_t pid, int proc_index, int completed, int remaining, const char* mcp_status) {
    char path[64], buffer[1024];

    // === Read /proc/[pid]/stat ===
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *fp_stat = fopen(path, "r");
    if (!fp_stat) return;

    int pid_read, priority, nice;
    char comm[256], state;
    unsigned long utime, stime;
    fscanf(fp_stat, "%d %s %c", &pid_read, comm, &state);

    if (comm[0] == '(') {
        memmove(comm, comm + 1, strlen(comm));               // remove opening '('
        comm[strcspn(comm, ")")] = '\0';                     // remove closing ')'
    }

    for (int i = 0; i < 10; i++) fscanf(fp_stat, "%*s");
    fscanf(fp_stat, "%lu %lu", &utime, &stime);
    for (int i = 0; i < 7; i++) fscanf(fp_stat, "%*s");
    fscanf(fp_stat, "%*s %*s %d %d", &priority, &nice);  // priority and nice are fields 18 and 19
    fclose(fp_stat);

    // === Read /proc/[pid]/status ===
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    FILE *fp_status = fopen(path, "r");
    if (!fp_status) return;

    long vm_size = -1, vm_rss = -1;
    int voluntary_ctxt = -1, nonvoluntary_ctxt = -1, threads = -1;
    while (fgets(buffer, sizeof(buffer), fp_status)) {
        sscanf(buffer, "VmSize: %ld", &vm_size);
        sscanf(buffer, "VmRSS: %ld", &vm_rss);
        sscanf(buffer, "Threads: %d", &threads);
        sscanf(buffer, "voluntary_ctxt_switches: %d", &voluntary_ctxt);
        sscanf(buffer, "nonvoluntary_ctxt_switches: %d", &nonvoluntary_ctxt);
    }
    fclose(fp_status);

    // === Read /proc/[pid]/sched ===
    snprintf(path, sizeof(path), "/proc/%d/sched", pid);
    FILE *fp_sched = fopen(path, "r");
    long cpu_util_ns = -1, cpu_runnable_ns = -1;
    if (fp_sched) {
        while (fgets(buffer, sizeof(buffer), fp_sched)) {
            sscanf(buffer, "se.sum_exec_runtime %ld", &cpu_util_ns);
            sscanf(buffer, "se.statistics.run_delay %ld", &cpu_runnable_ns);
        }
        fclose(fp_sched);
    }

    // === Format ===
    double total_cpu_seconds = (utime + stime) / (double)sysconf(_SC_CLK_TCK);

    printf("\n*********************************************\n");
    printf("* Program name:           %-15s\n", comm);
    printf("* Process ID:             %d\n", pid);
    printf("* Process Number:         %d\n", proc_index);
    printf("* MCP Status:             %s\n", mcp_status);
    printf("* Virtual Memory Usage:   %ld kB\n", vm_size);
    printf("* Physical Memory Usage:  %ld kB\n", vm_rss);
    printf("* Threads:                %d\n", threads);
    printf("* Voluntary Context Switches:     %d\n", voluntary_ctxt);
    printf("* Nonvoluntary Context Switches:  %d\n", nonvoluntary_ctxt);
    printf("* User time:              %.2f\n", utime / (double)sysconf(_SC_CLK_TCK));
    printf("* Kernel time:            %.2f\n", stime / (double)sysconf(_SC_CLK_TCK));
    printf("* Priority:               %d\n", priority);
    printf("* Nice level:             %d\n", nice);
    printf("* Total CPU Seconds:      %.6f\n", total_cpu_seconds);
    printf("* CPU Utilization Nanoseconds:  %ld\n", cpu_util_ns);
    printf("* CPU Runnable Nanoseconds:     %ld\n", cpu_runnable_ns);
    printf("* -------------------------------\n");
    printf("* Processes Complete:     %d\n", completed);
    printf("* Processes Remaining:    %d\n", remaining);
    printf("*********************************************\n\n");
}

void send_signal_to_children(pid_t* pids, int count, int signal, const char* label) {
    for (int i = 0; i < count; i++) {
        kill(pids[i], signal);
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
    kill(rr_pids[rr_current], SIGSTOP); // Pause current process

    // find next alive process
    int next = (rr_current + 1) % rr_num_procs;
    while (rr_completed[next]) next = (next + 1) % rr_num_procs;
    rr_current = next;

    kill(rr_pids[rr_current], SIGCONT);
    alarm(1);

    should_redraw = 1;                // repaint on next loop 
}

/* overwrite the same 22‑line window every time we repaint */
static void redraw_table(int done, int remain)
{
    const int lines = 22;

    /* move cursor to first line of the block */
    printf("\033[%dF", lines);       /* CUU(lines) + CR         */

    /* clear exactly <lines> rows without scrolling */
    for (int i = 0; i < lines; ++i) {
        printf("\033[2K");           /* clear this row          */
        if (i < lines - 1) printf("\033[1B");   /* down one    */
    }
    printf("\033[%dF", lines);       /* jump back to top        */

    const char *status = rr_started[rr_current] ? "RESUMED" : "STARTED";
    rr_started[rr_current] = true;

    print_current_process_stats(rr_pids[rr_current],
                                rr_current, done, remain, status);
    fflush(stdout);
}

/*need an array of bools to track which 
processes have already exited, so the scheduler can skip over them.*/
void round_robin(){
    signal(SIGALRM, signal_alarm);
    //start with 1st process
    rr_current = 0;
    //start 1st child process
    kill(rr_pids[rr_current], SIGCONT);
    alarm(1); // Sets a timer that sends SIGALRM after 1 second
    redraw_table(num_done, rr_alive);
    //loop until all processes finish, while they are alive
    while (rr_alive > 0) {
        int status = 0;       
        pid_t done_pid = waitpid(-1, &status, WNOHANG);
        if (done_pid > 0) {
            //child has exited, marke it complete
            for (int i = 0; i < rr_num_procs; i++) {
                if (rr_pids[i] == done_pid) {
                    rr_completed[i] = true;
                    rr_alive--; //derecement count of live processes
                    num_done++;
                    should_redraw = 1;    /* one more repaint       */
                    break;
                }
            }
        }
        if (should_redraw) {
            redraw_table(num_done, rr_alive);
            should_redraw = 0;
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
    rr_started = malloc(command_ctr * sizeof(bool));
    if (!rr_started) {
        perror("malloc rr_started");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < command_ctr; ++i)
        rr_started[i] = false; 
    // All children are forked and waiting
    // send SIGUSR1 to all children
    send_signal_to_children(pids, command_ctr, SIGUSR1, "SIGUSR1");

    //sleep so that parent does not send SIGSTOP too early, before
    // the child is running the actual command
    sleep(1);

    //pause the children with SIGSTOP
    send_signal_to_children(pids, command_ctr, SIGSTOP, "SIGSTOP");

    // We do not resume all children at once anymore with a for loop
    // printf("\n=== MCP: Sending SIGCONT to all children ===\n");
    // send_signal_to_children(pids, command_ctr, SIGCONT, "SIGCONT");
    // instead SCHEDULING
    round_robin();

    // wait for all children to finish
    for (int i = 0; i < command_ctr; i++){
        waitpid(pids[i], NULL, 0);
    }
    //free boolean array keeping track of which processes have exited CPU
    free(rr_completed);
    free(rr_started);
}

void free_mem(command_line* file_array, int command_ctr, pid_t* pids) {

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
            execvp(file_array[i].command_list[0], file_array[i].command_list);

            const char *err = "execvp failed\n";
            write(2, err, strlen(err)); 
            exit(EXIT_FAILURE);
        } else if(pid > 0) {
            //parent process
            //printf("I am the parent process. The child had PID: %d\n", pid);
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
