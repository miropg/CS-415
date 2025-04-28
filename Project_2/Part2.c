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
3	Once all workload programs are launched, MCP sends SIGSTOP to each child ➔
    this suspends them.
4	Then MCP sends SIGCONT to each child ➔ this resumes them.
5	After resuming, MCP waits for all children to finish with wait()/waitpid(),
    then exits cleanly and frees memory.
*/