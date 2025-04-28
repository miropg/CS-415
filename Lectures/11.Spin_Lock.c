int test_and_set (boolean *target) {
//set rv to old value
//if *target, was FALSE, that means you
//successfully acquired the lock
//if rv is TRUE, that means someone else had lock
    boolean rv = *target; // test part
//set target to True, so all other threads know you
//got the lock
    *target = TRUE; // set part
    return rv;
}
//after using lock (thread gets to use the crit section)
//the overall function, must set *target = FALSE, so the
//other threads know they can use it now

//Ex.
while (TRUE) {
    waiting[i] = TRUE;
    key = TRUE;
    while (waiting[i] && key)
        key = test_and_set(&lock);
    waiting[i] = FALSE;

    /* critical section */

    j = (i + 1) % n;
    while ((j != i) && !waiting[j])
        j = (j + 1) % n;
    if (j == i)
        lock = FALSE;
    else
        waiting[j] = FALSE;
}

int compare_and_swap(int *value, int expected, int newvalue){
    int tem = *value;
//if expected is 0 (unlocked) and you successfully swap
//it to 1 (locked), you have acquired the lock
//if not you keep trying
    if (*value == expected)
        *value = newvalue;
    return temp;
}
//after using lock, set lock = 0 for the next thread

//Ex.
while (TRUE) {
    while(compare_and_swap(&lock, 0, 1) != 0);
        //crit
    lock = 0;
}