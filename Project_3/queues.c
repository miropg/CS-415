#include <stdio.h>      
#include <stdlib.h>     
#include <stdbool.h>     
#include <pthread.h>
#include "queues.h"

// === SYNC PRIMITIVES ===
pthread_cond_t car_available = PTHREAD_COND_INITIALIZER;
pthread_mutex_t car_queue_lock = PTHREAD_MUTEX_INITIALIZER;
// PASSENGER QUEUE

// === GLOBAL QUEUES ===
CarQueue car_queue; // the queue of cars on roller coaster to keep them in line
PassengerQueue coaster_queue; //for roller coaster line
PassengerQueue ticket_queue; //for ticket queue line

// TICKET QUEUE FUNCTIONS

void init_passenger_queue(PassengerQueue* q, pthread_mutex_t* lock) {
    q->front = NULL;
    q->rear = NULL;
    q->size = 0;
    q->lock = lock;
}

bool is_passenger_queue_empty(PassengerQueue* q) {
    return q->front == NULL;
}

void enqueue_passenger(PassengerQueue* q, Passenger* p) {
    pthread_mutex_lock(q->lock);
    p->next = NULL;

    if (q->rear == NULL) {
        q->front = q->rear = p;
    } else {
        q->rear->next = p;
        q->rear = p;
    }
    q->size++;
    if (q == &coaster_queue) {
        pthread_cond_signal(&passengers_waiting);
    }
    pthread_mutex_unlock(q->lock);
}

Passenger* dequeue_passenger(PassengerQueue* q) {
    pthread_mutex_lock(q->lock);

    if (q->front == NULL) {
        pthread_mutex_unlock(q->lock);
        return NULL;
    }

    Passenger* temp = q->front;
    q->front = q->front->next;

    if (q->front == NULL) {
        q->rear = NULL;
    }

    q->size--;
    temp->next = NULL; // ??
    pthread_mutex_unlock(q->lock);
    return temp;  // caller is responsible for freeing it
}

void print_passenger_queue(PassengerQueue* q) {
    printf("Passenger Queue: ");
    Passenger* curr = q->front;
    while (curr != NULL) {
        printf("%d ", curr->pass_id);
        curr = curr->next;
    }
    printf("\n");
}


// CAR QUEUE FUNCTIONS

void init_car_queue(CarQueue* q, int max_size) {
    q->cars = malloc(sizeof(Car*) * max_size);
    if (!q->cars) {
        perror("malloc failed for car queue");
        exit(EXIT_FAILURE);
    }
    q->front = 0;
    q->rear = 0;
    q->curr_size = 0;
    q->queue_size = max_size;
}

bool is_empty(CarQueue* q) {
    return q->curr_size == 0;
}

bool is_full(CarQueue* q) {
    return q->curr_size == q->queue_size;
}

void enqueue(CarQueue* q, Car* car) {
    pthread_mutex_lock(&car_queue_lock);

    if (is_full(q)) {
        pthread_mutex_unlock(&car_queue_lock);
        return;
    }
    q->cars[q->rear] = car;
    q->rear = (q->rear + 1) % q->queue_size;
    q->curr_size++;
     // Signal that a car is now available
    //pthread_cond_signal(&car_available); OLD, instead of cond_broadcast below
    pthread_cond_broadcast(&car_available);
    pthread_mutex_unlock(&car_queue_lock);
}

Car* dequeue(CarQueue* q) {
    pthread_mutex_lock(&car_queue_lock);

    // wait until a car is available
    while (is_empty(q) && simulation_running) {
        pthread_cond_wait(&car_available, &car_queue_lock);
    }
    if (!simulation_running && is_empty(q)) {
        pthread_mutex_unlock(&car_queue_lock);
        return NULL;
    }
    Car* car = q->cars[q->front];
    q->front = (q->front + 1) % q->queue_size;
    q->curr_size--;

    pthread_mutex_unlock(&car_queue_lock);
    return car;
}

Car* peek_car(CarQueue* q) {
    if (is_empty(q)) return NULL;
    return q->cars[q->front];
}

void print_queue(CarQueue* q) {
    if (is_empty(q)) {
        printf("Queue is empty.\n");
        return;
    }

    printf("Car Queue [front to rear]: ");
    int count = q->curr_size;
    int index = q->front;
    while (count > 0) {
        printf("Car%d ", q->cars[index]->car_id);
        index = (index + 1) % q->queue_size;
        count--;
    }
    printf("\n");
}

