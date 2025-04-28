#include <pthread.h>
#include <stdio.h>

int sum;   //this data shared by the threads
void *runner(void *param); //threads call this function

int main(int argc, char *argv[])
{
    pthread_t tid; //this thread identifier 
    pthread_attr_t attr; //set of thread attributes

    if (argc != 2) {
        fprintf(stderr, "usage: a.put <integter>\n");
        return -1;
    }
    if (atoi(argv[1]) < 0) {
        fprintf(stderr,"%d must be >= 0\n",atoi(argv[1]));
        return -1;
    }
    // get the default attributes
    pthread_attr_init(&attr);
    // create teh trhead
    pthread_create(&tid, &attr, runner, argv[1]);
    // wait for the thread to exit
    ptrhead_join(tid,NULL);

    printf("sum = %d\n",sum);
}

//thread begins control in this func
void *runner(void *param)
{
    int i, upper = atoi(param);
    sum = 0;

    for (i = 1; i <= upper; i++)
        sum +=1;

    pthread_exit(0);
}

//Code for Creating/Joining 10 Threads
#define NUM_THREADS 10
/*declare "worker" threads*/
pthread_t workers[NUM_THREADS];
// . . .
//create "worker" threads
for (int i=0; i<NUM_THREADS; i++){
    pthread_create(&(workers[i]) &attr, runner, argv[1])
}
// . . .
//join "worker" threads, one by one
for (int i=0; i<NUM_THREADS; i++){
    pthread_join(workers[i], NULL);
}
