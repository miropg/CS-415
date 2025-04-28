    //ADDED IN LAB
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <number_of_processes>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    pid_t pid; //my pid
    
    //make array to store proess id's
    int* pids = malloc(sizeof(pid_t) * n);

    for(int i = 0; i < n; i++){
        pid = fork();
        if(pid == 0) {
            //child process
            printf("I am the child process. My PID: %d\n", pid);
            printf("I am the child process. My PID: %d\n", getpid());
            char* args[] = {"./hi", NULL};
            execvp(args[0], args);
            
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
        else if(pid > 0) {
            //parent process
            printf("I am the parent process. The child had PID: %d\n", pid);
            pids[i] = pid; 
        } else {
            perror("fork fail");
        }
    }
    for (int i = 0; i < n; i++){
        // wait for children by pids
        waitpid(pids[i], NULL, 0);
    }
    free(pids);
    return 0;
}