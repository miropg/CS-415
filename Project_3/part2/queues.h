#ifndef QUEUES_H
#define QUEUES_H

#include <stdbool.h>
#include <pthread.h>

extern volatile int simulation_running;

// === CAR DEFINITIONS ===
typedef struct {
    int car_id;
    int capacity;
    int unboard_count;
    int onboard_count;
    int assigned_count;
    int* passenger_ids;
    pthread_cond_t can_unload;
    bool can_unload_now;
} Car;

typedef struct {
    Car** cars;
    int front;
    int rear;
    int curr_size;
    int queue_size;
} CarQueue;

void init_car_queue(CarQueue* q, int max_size);
bool is_empty(CarQueue* q);
bool is_full(CarQueue* q);
void enqueue(CarQueue* q, Car* car);
Car* dequeue(CarQueue* q);
Car* peek_car(CarQueue* q);
void print_queue(CarQueue* q);

// === PASSENGER DEFINITIONS ===
typedef struct Passenger {
    int pass_id;
    Car* assigned_car;
    struct Passenger* next;
} Passenger;

typedef struct {
    Passenger* front;
    Passenger* rear;
    int size;
    pthread_mutex_t* lock;
} PassengerQueue;

void init_passenger_queue(PassengerQueue* q, pthread_mutex_t* lock);
bool is_passenger_queue_empty(PassengerQueue* q);
void enqueue_passenger(PassengerQueue* q, Passenger* p);
Passenger* dequeue_passenger(PassengerQueue* q);
void print_passenger_queue(PassengerQueue* q);

// === GLOBAL QUEUES ===
extern CarQueue car_queue;
extern PassengerQueue coaster_queue;
extern PassengerQueue ticket_queue;

// === SYNC PRIMITIVES ===
extern pthread_cond_t car_available;
extern pthread_cond_t passengers_waiting;
extern pthread_mutex_t car_queue_lock;
// extern pthread_mutex_t ticket_queue_lock;

#endif
