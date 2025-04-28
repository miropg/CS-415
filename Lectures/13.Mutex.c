acquire() {
    while (!available);
    //busy wait
    available = false;
}
release() {
    available = true;
}
while (TRUE) {
    // acquire lock
    // crit
    // release lock
}