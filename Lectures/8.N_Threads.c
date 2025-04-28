#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

struct threadargs {
    int id;
};
struct threadargs    threadARGS;

pthread_t tid;              //thread ID for publishers
pthread_attr_t attr;        //thread attributes
void **retval;

void *routine(void *param);
void *routine(void *args) {
    int tid;                //thread id
    tid = ((struct threadargs *) args)->id;
}// routine

int main(int argc, char *argv[])
{
    int i, numthreads;
    if(argc != 2) {
        fprintf(stderr, "USAGE: threads <INT>\n");
        exit(1);
    }

    numthreads = atoi(argv[1]);  // # child threads to create

    for(i = 0; i < numthreads; i++) {
        tid = i;
        pthread_create(&tid, &attr, routine, (void *) &threadARGS);
        pthread_join(tid,retval);
    }
} // main()