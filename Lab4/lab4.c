#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/* THIS FILE
creates multiple instances of the iobound.c "worker" program that burns
the CPU.

*/

void script_print (pid_t* pid_ary, int size);

int main(int argc, char*argv[])
{
	if (argc != 2)
	{
		printf ("Wrong number of arguments\n");
		exit (0);
	}
	/*
	*	TODO
	*	#1	declare child process pool
	*	#2 	spawn n new processes
	*		first create the argument needed for the processes
	*		for example "./iobound -seconds 10"
	*	#3	call script_print
	*	#4	wait for children processes to finish
	*	#5	free any dynamic memory
	*/
	int n = atoi(argv[1]);
	pid_t pid;
	//#1	declare child process pool
	int* pids = malloc(sizeof(pid_t) * n);

	//#2 	spawn n new processes
	//	first create the argument needed for the processes
	//	for example "./iobound -seconds 10"
	for(int i = 0; i < n; i++){
		pid = fork();
		if (pid == 0) {
			char* args[] = {"./iobound_1", "-seconds", "10", NULL};
			execvp(args[0], args);
			perror("execvp failed");
			exit(EXIT_FAILURE);
		}
		else if (pid > 0) {
			pids[i] = pid;
		}
		else {
			perror("fork failed");
			exit(EXIT_FAILURE);
		}
	}
	//#3	call script_print
	script_print(pids, n);
	//#4	wait for children processes to finish
	for (int i = 0; i < n; i++){
        // wait for children by pids
        waitpid(pids[i], NULL, 0);
    }
	//#5	free any dynamic memory
	free(pids);
	return 0;
}


void script_print (pid_t* pid_ary, int size)
{
	FILE* fout;
	//top_script.sh helps monitor chidlren processes
	fout = fopen ("top_script.sh", "w");
	fprintf(fout, "#!/bin/bash\ntop");
	for (int i = 0; i < size; i++)
	{
		fprintf(fout, " -p %d", (int)(pid_ary[i]));
	}
	fprintf(fout, "\n");
	fclose (fout);
	//opens a new terminal window
	//inside the window it runs top_script.sh
	//to visually monitor the child processes
	char* top_arg[] = {"gnome-terminal", "--", "bash", "top_script.sh", NULL};
	pid_t top_pid;

	top_pid = fork();
	{
		if (top_pid == 0)
		{
			if(execvp(top_arg[0], top_arg) == -1)
			{
				perror ("top command: ");
			}
			exit(0);
		}
	}
}


