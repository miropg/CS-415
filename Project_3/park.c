#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <semaphore.h>
#include "queues.h"

/*Functions you need
pthread_create()
pthread_exit()
pthread_join()
pthread_mutex_lock()
pthread_mutex_unlock()
pthread_cond_wait()
pthread_cond_broadcast() or pthread_cond_signal()*/

volatile int simulation_running = 1; // Parks Hours of Operation
struct timespec start_time; //timestamps

//locks
pthread_mutex_t ticket_queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t coaster_queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ride_lock = PTHREAD_MUTEX_INITIALIZER;

//condition variables
pthread_cond_t can_board      = PTHREAD_COND_INITIALIZER;
pthread_cond_t all_boarded    = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_unboard    = PTHREAD_COND_INITIALIZER;
pthread_cond_t all_unboarded  = PTHREAD_COND_INITIALIZER;
int can_load_now = 0;
int can_unload_now = 0;

// ticket -> ride queue variables
sem_t ride_queue_semaphore;  // replaces semaphore_total
int global_max_coaster_line;

// Global simulation parameters (initialized in launch_park)
int tot_passengers;
int num_cars;
int car_capacity;
int ride_wait;
int ride_duration;
/*
pthread_t *thread_ids;
pthread_mutex_t counter_lock;
pthread_mutex_t pipe_lock;
int counter = 0;
int pipe_fd[2];		// our pipe :)
*/
void beginning_stats(int passengers, int cars, int capacity, int wait, int ride, int hours){
    //printf("[Monitor] Simulation started with parameters:\n");
    printf("- Number of passenger threads: %d\n", passengers);
    printf("- Number of cars: %d\n", cars);
    printf("- Capacity per car: %d\n", capacity);
    printf("- Park exploration time: 2-5 seconds\n");
    printf("- Car waiting period: %d seconds\n", wait);
    printf("- Ride duration: %d seconds\n", ride);
    printf("- Total simulation time: %d seconds\n", hours);
}

void print_timestamp() {
    struct timespec current_time;
    clock_gettime(CLOCK_MONOTONIC, &current_time);

    time_t elapsed_sec = current_time.tv_sec - start_time.tv_sec;
    int hours = elapsed_sec / 3600;
    int minutes = (elapsed_sec % 3600) / 60;
    int seconds = elapsed_sec % 60;

    printf("[Time: %02d:%02d:%02d] ", hours, minutes, seconds);
}

void print_help() {
    printf("Usage: ./park [OPTIONS]\n");
    printf("Options:\n");
    printf("  -n INT   Number of passenger threads\n");
    printf("  -c INT   Number of cars\n");
    printf("  -p INT   Capacity per car\n");
    printf("  -w INT   Car waiting period (seconds)\n");
    printf("  -r INT   Ride duration (seconds)\n");
    printf("  -t INT   Total simulation time in seconds (default: 40)\n");
    printf("  -h       Show this help message\n");
    printf("\nExample: ./park -n 30 -c 4 -p 6 -w 8 -r 10 -t 60\n");
}

struct TimerArgs {
    int duration;
};
//for Timing the Duration of the Park
void* timer_thread(void* arg) {
    struct TimerArgs* t = (struct TimerArgs*) arg;
    sleep(t->duration);
    simulation_running = 0;
    print_timestamp();
    printf("Simulation timer ended. Closing the park.\n");
    // Wake up any threads waiting on condition variables
    // like a bell for closing, making sure threads call pthread_exit
    pthread_cond_broadcast(&can_board);
    pthread_cond_broadcast(&all_unboarded);
    pthread_cond_broadcast(&can_unboard);
    free(t);
    return NULL;
}
// PASSENGER FUNCTIONS
//– Called when a passenger boards the car.
void board(Passenger* p) {
    pthread_mutex_lock(&ride_lock);
    // wait until load() calls pthread_cond_broadcast(&can_board);
    // threads wake up. check struct if they were assigned a car, if so they board
    while (!can_load_now) {
        pthread_cond_wait(&can_board, &ride_lock);
    }
    Car* my_car = p->assigned_car;
    if (my_car != NULL) {
        print_timestamp();
        printf("Passenger %d boarded Car %d\n", p->pass_id, my_car->car_id);

        my_car->passenger_ids[my_car->onboard_count++] = p->pass_id;

        if (my_car->onboard_count == my_car->capacity) {
            pthread_cond_signal(&all_boarded);
        }
    }
    pthread_mutex_unlock(&ride_lock);
    sem_post(&ride_queue_semaphore);
} 

//– Called when a passenger exits the car.
void unboard(Passenger* p) {
    pthread_mutex_lock(&ride_lock);
    // wait until car has called unload()
    while (!can_unload_now) {
        pthread_cond_wait(&can_unboard, &ride_lock);
    }
    Car* my_car = p->assigned_car;
    if (my_car != NULL){
        print_timestamp();
        printf("Passenger %d unboarded Car %d\n", p->pass_id, my_car->car_id);
        my_car->unboard_count++;
        if (my_car->unboard_count == my_car->onboard_count) {
            pthread_cond_signal(&all_unboarded);
        }
    }
    pthread_mutex_unlock(&ride_lock);
    p->assigned_car = NULL;
} 

// ROLLER COASTER FUNCTIONS
// Signals passengers to call board
void load(Car* car){
    int passengers_assigned = 0;
    pthread_mutex_lock(&ride_lock);
    car->onboard_count = 0;
    car->unboard_count = 0;
    print_timestamp();
    printf("Car %d invoked load()\n", car->car_id);
    can_load_now = 1; // signal board() to board a passenger
    //assign passengers
    for (int i = 0; i < car_capacity; i++) {
        Passenger* p = dequeue_passenger(&coaster_queue);
        if (!p) break;
        p->assigned_car = car;
        passengers_assigned++;
    }
    if (passengers_assigned == 0) {
        print_timestamp();
        printf("Car %d found no passengers in queue and is departing empty\n", car->car_id);
        can_load_now = 0;
        pthread_mutex_unlock(&ride_lock);
        return;
    }   
    pthread_cond_broadcast(&can_board); //signal waiting passengers to board
    //Edge case for 1 passenger
    if (tot_passengers == 1) {
        print_timestamp();
        printf("Only one passenger — departing immediately\n");
        can_load_now = 0;
        pthread_mutex_unlock(&ride_lock);
        return;
    }
    struct timespec deadline;  // OR TIMER goes off
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += ride_wait;
    int timed_out = 0;
    while (car->onboard_count < passengers_assigned) {
        int result = pthread_cond_timedwait(&all_boarded, &ride_lock, &deadline);
        if (result == ETIMEDOUT) {
            timed_out = 1;
            break;
        }
    }
    if (timed_out) {
        print_timestamp();
        printf("Car %d done waiting, departing with %d / %d\n",
                car->car_id, car->onboard_count, car->capacity);
    } else if (car->onboard_count == car->capacity){
        print_timestamp();
        printf("Car %d is full with %d passengers\n", car->car_id, 
            car->capacity);
    } else {
        print_timestamp();
        printf("Car %d boarded all %d assigned passengers, but is not full (capacity %d)\n",
           car->car_id, car->onboard_count, car->capacity);
    }
    can_load_now = 0;
    pthread_mutex_unlock(&ride_lock);
}
// – Simulates the ride duration.
void run(Car* car){  //int car?
    print_timestamp();
    printf("Car %d departed for its run\n", car->car_id);
    sleep(ride_duration);
    print_timestamp();
    printf("Car %d finished its run\n", car->car_id);
}
//– Signals passengers to call unboard()
void unload(Car* car){
    pthread_mutex_lock(&ride_lock);
    print_timestamp();
    printf("Car %d invoked unload()\n", car->car_id);

    can_unload_now = 1;
    pthread_cond_broadcast(&can_unboard);
    //wait until all passengers who boarded have unboarded
    while (car->unboard_count < car->onboard_count) {
        //wait until all passengers unboarded
        pthread_cond_wait(&all_unboarded, &ride_lock);
    }
    car->unboard_count = 0; //reset after all passengers exited
    car->onboard_count = 0;
    can_unload_now = 0;
    pthread_mutex_unlock(&ride_lock);
}

//roller coaster gets called by each car thread in launch_park
void* roller_coaster(void* arg){
    while (simulation_running) {
        Car* car = dequeue(&car_queue);
        //avoid starting a new ride during closed Park hours? maybe cut
        if (!car || !simulation_running){
            break;
        }
        load(car);
        run(car);
        unload(car);
        enqueue(&car_queue, car);
    }
    return NULL;
}

int embark_coaster(Passenger* p){
    board(p);
    unboard(p);
    return 0;
}

//semaphore count starts at global_max_coaster_line, then decrements
//decrements count if >0, if it's 0, the thread blocks
void* park_experience(void* arg){
    //need to pass as pointer for load to communicate to board
    Passenger* p = (Passenger*)arg;
    print_timestamp();
    printf("Passenger %d entered the park\n", p->pass_id);

    while (simulation_running) {
        // Exploring the Park
        int explore_time = (rand() % 4) + 2; //2-5 seconds
        print_timestamp();
        printf("Passenger %d is exploring the park...\n", p->pass_id);
        sleep(explore_time);
        print_timestamp();
        printf("Passenger %d finished exploring, heading to ticket booth\n", p->pass_id);
        
        // Ticket booth
        enqueue_passenger(&ticket_queue, p);
        print_timestamp();
        printf("Passenger %d waiting in ticket queue\n", p->pass_id);
        //increments ride queue total, and blocks if line at max
        sem_wait(&ride_queue_semaphore); 

        print_timestamp();
        printf("Passenger %d acquired a ticket\n", p->pass_id);
        dequeue_passenger(&ticket_queue);

        enqueue_passenger(&coaster_queue, p);
        print_timestamp();
        printf("Passenger %d joined the ride queue\n", p->pass_id); 

        embark_coaster(p);
    }
    free(p); //free specific passenger after park hours
    pthread_exit(NULL); //all threads are done after the park hours are over
}

void launch_park(int passengers, int cars, int capacity, int wait, int ride, int park_hours,
    int max_coaster_line)
{
    //assigned_car = malloc(sizeof(Car*) * tot_passengers);
    // Set global variables
    tot_passengers = passengers;
    num_cars = cars;
    car_capacity = capacity;
    ride_wait = wait;
    ride_duration = ride;
    global_max_coaster_line = max_coaster_line; // *2 total seats on all cars
    // use 0 in sem_init because we are using threads, above 0 would be num processes using it
    sem_init(&ride_queue_semaphore, 0, global_max_coaster_line); //SEMAPHORE for ride queue line

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    //separate queue structure for cars
    init_car_queue(&car_queue, cars);
    //REUSEing same queue structure for ticket_queue/ride_queue
    init_passenger_queue(&ticket_queue, &ticket_queue_lock);
    init_passenger_queue(&coaster_queue, &coaster_queue_lock);

    beginning_stats(tot_passengers, num_cars, car_capacity, ride_wait, ride_duration, park_hours);

    pthread_t* car_thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * num_cars); //threads for cars too
    pthread_t* thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * tot_passengers);//array of threads
    
    Car* all_cars = malloc(sizeof(Car) * num_cars);
    for (int i = 0; i < num_cars; i++) {
        all_cars[i].car_id = i + 1;
        all_cars[i].capacity = car_capacity;
        all_cars[i].onboard_count = 0;
        all_cars[i].unboard_count = 0;
        all_cars[i].passenger_ids = malloc(sizeof(int) * car_capacity);
        enqueue(&car_queue, &all_cars[i]);
        pthread_create(&car_thread_ids[i], NULL, roller_coaster, (void*)&all_cars[i]);
    }
    // START THE PARK (40 second default timer)
    struct TimerArgs* timer_args = malloc(sizeof(struct TimerArgs));
    timer_args->duration = park_hours;  // user-defined or default duration

    pthread_t timer;
    pthread_create(&timer, NULL, timer_thread, (void*) timer_args);
    //create passenger amount of threads in addition to the main thread
    //id numbers for passengser, 1, 2, 3...
    Passenger** passenger_objects = malloc(sizeof(Passenger) * tot_passengers);
    //do not need to allocate mem for threads themselves, just the ids
    //pthread_create(store thread id, attr, function that runs in new thread, arg)
	for(int i = 0; i < passengers; i++){
        passenger_objects[i] = malloc(sizeof(Passenger));
        passenger_objects[i]->pass_id = i + 1; //so pasengers start from 1, not 0
        passenger_objects[i]->assigned_car = NULL;
        passenger_objects[i]->next = NULL;
        // threads enter the park when you call simulate_work on them
		pthread_create(&thread_ids[i], NULL, park_experience, (void*)passenger_objects[i]);
        usleep(100000); //stagger passengers entering the park(can make random instead?)
    }
	for (int j = 0; j < passengers; ++j){
		pthread_join(thread_ids[j], NULL); // wait on our threads to rejoin main thread
	}
    pthread_join(timer, NULL);

    for (int j = 0; j < num_cars; ++j){
		pthread_join(car_thread_ids[j], NULL); // wait on our threads to rejoin main thread
	}
    free(thread_ids);
    free(car_thread_ids);
    //not necessary if we free(p) individual threads in park_exp
    // for (int i = 0; i < tot_passengers; i++) {
    //     free(passenger_objects[i]);
    // }
    free(passenger_objects);

    for (int i = 0; i < num_cars; i++){
        free(all_cars[i].passenger_ids);
    }
    free(all_cars);
    //destroy locks
    pthread_mutex_destroy(&car_queue_lock);
    pthread_mutex_destroy(&ticket_queue_lock);
    pthread_mutex_destroy(&coaster_queue_lock);
    pthread_mutex_destroy(&ride_lock);
    //destroy variables & booleans
    pthread_cond_destroy(&can_board);
    pthread_cond_destroy(&all_boarded);
    pthread_cond_destroy(&can_unboard);
    pthread_cond_destroy(&all_unboarded);
    pthread_cond_destroy(&car_available);
    //destroy semaphore used in size of ride_queue
    sem_destroy(&ride_queue_semaphore);
}
//int main: 1 process, 1 main thread inside it
int main(int argc, char *argv[]){
    int opt;
    int passengers = -1, cars = -1, capacity = -1, wait = -1, ride = -1;
    int park_hours = 40; //in sec

    // Handle long flag manually
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            print_help();
            exit(0);
        }
    }
    while ((opt = getopt(argc, argv, "n:c:p:w:r:t:")) != -1) {
        switch (opt) {
            case 'n': passengers = atoi(optarg); break;
            case 'c': cars = atoi(optarg); break;
            case 'p': capacity = atoi(optarg); break;
            case 'w': wait = atoi(optarg); break;
            case 'r': ride = atoi(optarg); break;
            case 't': park_hours = atoi(optarg); break;
            default:
                fprintf(stderr, "Error: Invalid option. Use -h or --help.\n");
                return 1;
        }
    }
    //set default values if no args provided
    if (passengers < 0 && cars < 0 && capacity < 0 && wait < 0 && ride < 0) {
        printf("[Info] No arguments provided. Using default settings.\n");
        passengers = 10;
        cars = 2;
        capacity = 4;
        wait = 5;
        ride = 6;
        park_hours = 40;
    }
    // Check for missing args
    if (passengers < 0 || cars < 0 || capacity < 0 || wait < 0 || ride < 0) {
        fprintf(stderr, "Error: missing or invalid arguments\n");
        print_help();
        exit(EXIT_FAILURE);
    }
    int max_coaster_line = 2 * (cars * capacity);
    printf("===== DUCK PARK SIMULATION =====\n");
    launch_park(passengers, cars, capacity, wait, ride, park_hours, max_coaster_line);
    return 0;
}
/* lab code
	pthread_mutex_lock(&pipe_lock);	// lock the critical section to prevent race condition!
	char message[100];
    snprintf(message, sizeof(message), "Thread %d finished work, counter = %d\n", *numbers, counter);
    write(pipe_fd[1], message, strlen(message)); 	// write to the pipe	
    pthread_mutex_unlock(&pipe_lock);	

    pthread_mutex_init(&counter_lock, NULL);		// initialize the lock
	pthread_mutex_init(&pipe_lock, NULL);		// initialize the lock
    if (pipe(pipe_fd) == -1) {			// init the pipe!
        perror("pipe failed");
        exit(EXIT_FAILURE);
    }

    close(pipe_fd[1]);		// close write end of pipe
	char buffer[100];
    ssize_t bytes_read;
    while ((bytes_read = read(pipe_fd[0], buffer, sizeof(buffer))) > 0)
        fwrite(buffer, 1, bytes_read, stdout);

	pthread_mutex_destroy(&counter_lock);
	pthread_mutex_destroy(&pipe_lock);
	close(pipe_fd[0]);			// close read end
    free(thread_ids);
    */
