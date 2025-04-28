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