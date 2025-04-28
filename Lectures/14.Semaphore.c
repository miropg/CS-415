typedef struct{
    int value;
    struct process *list;
    semaphore;
}

// Sempahore S is an int variable that can only be
//accessed via 2 indivisible (atomic) operations
wait(s) {  //P = Proberr "Try"
    while (S <= 0);
    S--;
}

signal(S) { //V = increment
    S++;
}

