int main()
{
pid_t pid;
    // fork another process
    pid = fork();
    if (pid < 0) { //error occured
        fprintf(stderr, "Fork Failed");
        exit(-1);
    }
    else if (pid == 0) { //child process
        execlp("/bin/ls", "ls", NULL); //exec a file
    }
    else { //parent process
        //parent will wait for the child to complete
        wait(NULL);
        printf ("Child complete");
    exit(0);
    }
}

/*execl, execlp, execle, execv, execvp, execvpe
all execute a file and are frontends to execve
*/