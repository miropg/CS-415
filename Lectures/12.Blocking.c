//think of L as a lock, that needs to stop spinning
//stop (wasting the CPU) Lets fix it...
void EnterCS(Lock *L) {
    DisableInterrupts();
    if (L is free) {
        //Set L to being used;
    } else {
        //Move this process to Blocked queue for L;
        //Select another process from Ready queue;
        //Context switch to that process;
    }
    EnableInterrupts();
}

void ExitCS(Lock *L) {
    DisableInterrupts();
    if (Blocked queue for L is empty) {
        //Set L to free;
    } else {
        //Move first process from Blocked queue to Ready queue;
    }
    EnableInterrupts();
}
