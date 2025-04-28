#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

pid_t pid;
int status;

int main(int argc, char *argv[])
{
    int i; numprocesses;

    if(argc != 2) {
        fprintf(stderr, "USAGE: processes <INT>\n");
        exit(1);
    }
    numprocesses = atoi(argv[1]);   //# child processes to create

    for (i = 0; i < numprocesses; i++){
        pid = fork();
        if (pid == 0)               // child process
            exit(0);                // * immediate exits
        else                        // parent process
            waitpid(pid, &status, 0);//* waits for child process to exit
    };
    return 0;
}