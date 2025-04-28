// Shared Memory - Producer
int nextProduced;
                    // Circular buffer
while (1) {
    while (((in + 1) % BUFFER_SIZE) == out);
    //do nothing ... buffer is full
    buffer[in] = nextProduced;
    in = (in + 1) % BUFFER_SIZE;
}

//assume that the producer is modifying the out variable.
//do you see any problems here?



// Shared Memory - Consumer
item nextConsumed;
                    // Circular buffer
while (1) {
    while (in == out);
    //do nothing ... buffer is empty 
    nextConsumed = buffer[out];
    out = (out + 1) % BUFFER_SIZE;
}

//assume that the producer is modifying the in variable.
//do you see any problems here?
