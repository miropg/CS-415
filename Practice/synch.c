/*Two office workers, Alice and Bob, want to print documents. 
There's only one printer, and they both try to use it at the 
same time. The printer needs to initialize, then print, then 
shut down.
What problem could happen if both threads run this code?
Where would you add synchronization and what kind?*/
semaphore mutex = 1; // declared outside so all threads share
void* print_job(void* arg) {
    char* name = (char*) arg;
   
    wait(mutex);
    printf("%s is preparing the printer\n", name);
    sleep(1);  // Simulate printer warm-up

    printf("%s is printing a document\n", name);
    sleep(2);  // Simulate printing time
    
    printf("%s is shutting down the printer\n", name);
    signal(mutex);
    return NULL;
}

/*Five friends are eating at a table. There are 3 forks total, 
and each friend needs 1 fork to eat. A friend only eats if they
can grab a fork. Once they finish, they return the fork.*/
void* eat_meal(void* arg) {
    int id = *((int*) arg);

    printf("Friend %d is hungry\n", id);

    // wait for fork here?

    printf("Friend %d got a fork and is eating\n", id);
    sleep(1); //simulate eating time

    printf("Friend %d finished eating and returned the fork\n", id);

    // signal returned fork here?
    return NULL;
}
/*
6.8
Race conditions are possible in many computer systems. Consider an
online auction system where the current highest bid for each item
must be maintained. A person who wishes to bid on an item calls the
bid(amount) function, which compares the amount being bid to the
current highest bid. If the amount exceeds the current highest bid, the
highest bid is set to the new amount. This is illustrated below:
*/
//Describe how a race condition is possible in this situation and what
//might be done to prevent the race condition from occurring.
void bid(double amount) {
    if (amount > highestBid)
        highestBid = amount;
}
/*A: Well a race condition could happen if two bids that are both higher
than the current highest bid. The first may be the actual highest, changes
amount and then the 2nd might be inbetween the current highestBid and the 
highest, and it managed to change it to an incorrect value just because it
changed amount after.
I believe a simple mutex lock could fix this
pthread_mutex_t lock;
void bid(double amount) {
    pthread_mutex_lock(&lock);
    if (amount > highestBid)
        highestBid = amount;
    pthread_mutex_unlock(&lock); 
}
*/


push(item) {
    if (top < SIZE) {
        stack[top] = item;
        top++;
    }
    else
        ERROR
}
pop() {
    if (!is empty()) {
        top--;
        return stack[top];
    }
    else
        ERROR
}
is empty() {
    if (top == 0)
        return true;
    else
        return false;
}
    
/* 
6.9
The following program example can be used to sum the array values
of size N elements in parallel on a system containing N computing cores
(there is a separate processor for each array element):

This has the effect of summing the elements in the array as a series
of partial sums, as shown in Figure 6.16. After the code has executed,
the sum of all elements in the array is stored in the last array location.
Are there any race conditions in the above code example? If so, identify
where they occur and illustrate with an example. If not, demonstrate
why this algorithm is free from race conditions.
*/
for j = 1 to log_2(N) {
    for k = 1 to N {
        if ((k + 1) % pow(2,j) == 0) {
            values[k] += values[k - pow(2,(j-1))]
        }
        // wait for all threads before next round
        pthread_barrier_wait(&barrier);
        //ADDED
    }     
}
/* The race condition here is it's possible a thread in round j+1 starts early
and tries to read from a value that's still possible written in round j

Barrier ensures all threads finish current step before any proceeds
prevents race conditions in multi-phase parallel algorithms like
reduction or prefix cum
*/


//6.11
//One approach for using compare and swap() for implementing a spinlock is as follows:
void lock spinlock(int *lock) {
    while (compare and swap(lock, 0, 1) != 0)
    ; //spin
    }
//A suggested alternative approach is to use the “compare and 
//compareand-swap” idiom, which checks the status of the lock before invoking the

/*compare and swap() operation. (The rationale behind this approach is
to invoke compare and swap()only if the lock is currently available.)
This strategy is shown below:
*/
void lock spinlock(int *lock)
{
    while (true) {
        if (*lock == 0) { // non-atomic check
        /* lock appears to be available */
            if (!compare and swap(lock, 0, 1))
                break; // we successfully got the lock
        }
    }
}
/*Does this “compare and compare-and-swap” idiom work appropriately
for implementing spinlocks? If so, explain. If not, illustrate how the
integrity of the lock is compromised.
*/   
// it doesn't add anything, because two threads can run *lock == 0 at the
// same time necause it is not atomic, and therefore a pointless check


/*Some semaphore implementations provide a function getValue() that
returns the current value of a semaphore. This function may, for instance,
be invoked prior to calling wait() so that a process will only call wait()
if the value of the semaphore is > 0, thereby preventing blocking while
waiting for the semaphore. For example:

if (getValue(&sem) > 0)
    wait(&sem);

Many developers argue against such a function and discourage its use.
Describe a potential problem that could occur when using the function
getValue() in this scenario.
*/

//I suppose it is posisble the value will change by another thread inbetween
//the 2 lines of code? so it will inapropriately wait because the value of
//the semaphore is actually 0 instead.
// RULE: never split the atomic "check-and-act" into two lines


//6.13
/*The first known correct software solution to the critical-section problem
for two processes was developed by Dekker. The two processes, P0 and
P1, share the following variables: */
    boolean flag[2]; //initially false 
    int turn;
/*
The structure of process Pi (i == 0 or 1) is shown in Figure 6.18. The
other process is Pj (j == 1 or 0). Prove that the algorithm satisfies all
three requirements for the critical-section problem.*/
flag[0] = false, flag[1] = false //initially
turn = 0;
//process Pi / P0 (i == 0 or 1) -> i = 0
while (true) {
    flag[i] = true;
    
    while (flag[j]) { //both go in while loops
        if (turn == j) { // if turn == 1, false, spins
            flag[i] = false;
            while (turn == j);
                /* do nothing */
            flag[i] = true;
        }
    }
        /* critical section (not visible)*/
    turn = j;
    flag[i] = false;
        /* remainder section */
}
//process Pj / P1 (j == 1 or 0) -> j = 1
while (true) {
    flag[j] = true;// both set to true

    while (flag[i]) { //both go in while loops
        if (turn == i) { //THIS GOES FIRST, i = 0
            flag[j] = false; //flag[1] = false
            while (turn == i); //
                // do nothing
            flag[j] = true
        }
    }
        /* critical section (not visible)*/
    turn = i;
    flag[j] = false;
}
//3 requirements: progress, mutual exclusion, bounded waiting
/* im assuming that which ever thread gets to flag[j]/flag[i] = true
is the humble thread, giving the turn to the other process. 
lets say while(flag[i]) gets to go
progres: yes one thread, P1 gets to use the critical section first
mutual exclusion: only 1 thread gets to use the critical section at a time
bounded waiting: P0 eventually gets its turn so it doesn't wait indefinitely 
*/