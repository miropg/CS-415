#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#include <semaphore.h>
#include <sched.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "queues.h"
// #include "monitor.h"

//ALL MONITOR GLOBAL VARIABLES
// static int mon_pipe[2];
// static pthread_mutex_t shared_mutex_storage = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t *&print_lock = &shared_mutex_storage;

static time_t beginning_time;
void* monitor_timer_thread(void* arg);

// Pointer to the array of all Car structs (allocated in launch_ark)
Car *all_cars;

// Counters for final statistics
int total_passengers_ridden;
int total_rides_completed;

// Accumulators for average‐wait‐time statistics
double sum_wait_ticket_queue;
double sum_wait_ride_queue;
int    count_wait_ticket;
int    count_wait_ride;

volatile int simulation_running = 1; // Parks Hours of Operation
struct timespec start_time; //timestamps
int currently_loading = 0;
//________________________________________________________________________
//locks
pthread_mutex_t car_selection_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ticket_booth_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t coaster_queue_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ride_lock = PTHREAD_MUTEX_INITIALIZER;

static pthread_mutex_t print_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  snapshot_done_cv = PTHREAD_COND_INITIALIZER;
static volatile bool   snapshot_pending = false;

pthread_mutex_t stats_lock = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t load_lock = PTHREAD_MUTEX_INITIALIZER;

//condition variables
pthread_cond_t can_board      = PTHREAD_COND_INITIALIZER;
//pthread_cond_t all_boarded    = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_unboard    = PTHREAD_COND_INITIALIZER;
pthread_cond_t all_unboarded  = PTHREAD_COND_INITIALIZER;
pthread_cond_t passengers_waiting = PTHREAD_COND_INITIALIZER;
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

static void print_barrier_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    pthread_mutex_lock(&print_mutex);
    while (snapshot_pending) {
        // if the monitor has announced “snapshot pending,”
        // block here until it signals “done.”
        pthread_cond_wait(&snapshot_done_cv, &print_mutex);
    }

    // Now it’s our turn to print:
    print_timestamp();  // assume existing function to print “[Time: xx:xx:xx] ”
    vprintf(fmt, ap);
    fflush(stdout);

    pthread_mutex_unlock(&print_mutex);
    va_end(ap);
}

//ALL MONITOR FUNCTIONS
void beginning_stats(int passengers,
                             int cars,
                             int capacity,
                             int wait,
                             int ride,
                             int hours)
{
    char buf[128];
    // First line:
    snprintf(buf, sizeof(buf),
             "Simulation started with parameters:\n");
    write(STDOUT_FILENO, buf, strlen(buf));

    snprintf(buf, sizeof(buf),
             "- Number of passenger threads: %d\n", passengers);
    write(STDOUT_FILENO, buf, strlen(buf));

    snprintf(buf, sizeof(buf),
             "- Number of cars: %d\n", cars);
    write(STDOUT_FILENO, buf, strlen(buf));

    snprintf(buf, sizeof(buf),
             "- Capacity per car: %d\n", capacity);
    write(STDOUT_FILENO, buf, strlen(buf));

    // The “Park exploration time” line was hard‐coded, so send that as well:
    snprintf(buf, sizeof(buf),
             "- Park exploration time: 1-10 seconds\n");
    write(STDOUT_FILENO, buf, strlen(buf));

    snprintf(buf, sizeof(buf),
             "- Car waiting period: %d seconds\n", wait);
    write(STDOUT_FILENO, buf, strlen(buf));

    snprintf(buf, sizeof(buf),
             "- Ride duration: %d seconds\n", ride);
    write(STDOUT_FILENO, buf, strlen(buf));

    snprintf(buf, sizeof(buf),
             "- Total simulation time: %d seconds\n", hours);
    write(STDOUT_FILENO, buf, strlen(buf));

    // A blank line to separate from later snapshots:
    snprintf(buf, sizeof(buf), "\n");
    write(STDOUT_FILENO, buf, strlen(buf));
}

void* monitor_timer_thread(void* arg) {
    int interval = *((int*)arg);
    free(arg);

    // 1) Set up the first “next_wake” = now + interval
    struct timespec next_wake;
    clock_gettime(CLOCK_MONOTONIC, &next_wake);
    next_wake.tv_sec += interval;     // first snapshot at ~interval seconds from now

    while (simulation_running) {
        // 2) Sleep until that absolute time
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next_wake, NULL);

        // It’s possible simulation_running flipped to 0 while sleeping; check:
        if (!simulation_running) break;

        // 3) Build & write exactly one snapshot at “now”
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        time_t elapsed_sec = now.tv_sec - start_time.tv_sec;
        int hh = elapsed_sec / 3600;
        int mm = (elapsed_sec % 3600) / 60;
        int ss = elapsed_sec % 60;

        char block[4096];
        char* p = block;
        int rem = sizeof(block);
        int n;

        // 3a) Header
        n = snprintf(p, rem, "\nSystem State at %02d:%02d:%02d\n", hh, mm, ss);
        p += n; rem -= n;

        // 3b) Ticket Queue
        pthread_mutex_lock(ticket_queue.lock);
        n = snprintf(p, rem, "Ticket Queue: [");
        p += n; rem -= n;

        Passenger* cur = ticket_queue.front;
        bool first = true;
        while (cur && rem > 32) {
            if (!first) {
                n = snprintf(p, rem, ", ");
                p += n; rem -= n;
            }
            n = snprintf(p, rem, "Passenger %d", cur->pass_id);
            p += n; rem -= n;
            first = false;
            cur = cur->next;
        }
        n = snprintf(p, rem, "]\n");
        p += n; rem -= n;
        pthread_mutex_unlock(ticket_queue.lock);

        // 3c) Ride Queue
        pthread_mutex_lock(coaster_queue.lock);
        n = snprintf(p, rem, "Ride Queue: [");
        p += n; rem -= n;

        cur = coaster_queue.front;
        first = true;
        while (cur && rem > 32) {
            if (!first) {
                n = snprintf(p, rem, ", ");
                p += n; rem -= n;
            }
            n = snprintf(p, rem, "Passenger %d", cur->pass_id);
            p += n; rem -= n;
            first = false;
            cur = cur->next;
        }
        n = snprintf(p, rem, "]\n");
        p += n; rem -= n;
        pthread_mutex_unlock(coaster_queue.lock);

        // 3d) Car statuses
        pthread_mutex_lock(&ride_lock);
        for (int i = 0; i < num_cars && rem > 64; i++) {
            Car* car = &all_cars[i];
            const char* state_str =
                (car->state == WAITING) ? "WAITING" :
                (car->state == LOADING) ? "LOADING" :
                (car->state == RUNNING) ? "RUNNING" : "UNKNOWN";

            n = snprintf(p, rem,
                         "Car %d Status: %s (%d/%d passengers)\n",
                         car->car_id,
                         state_str,
                         car->onboard_count,
                         car->capacity);
            p += n; rem -= n;
        }
        pthread_mutex_unlock(&ride_lock);

        // 3e) “Passengers in park” line:
        pthread_mutex_lock(&ride_lock);
        int on_ride = 0;
        for (int i = 0; i < num_cars; i++) {
            on_ride += all_cars[i].onboard_count;
        }
        pthread_mutex_unlock(&ride_lock);

        int in_queues = ticket_queue.size + coaster_queue.size;
        int exploring = tot_passengers - (in_queues + on_ride);
        if (exploring < 0) exploring = 0;

        n = snprintf(p, rem,
                     "Passengers in park: %d (%d exploring, %d in queues, %d on rides)\n\n",
                     tot_passengers, exploring, in_queues, on_ride);
        p += n; rem -= n;

        // 3f) Write all at once
        size_t total_len = p - block;


        snapshot_pending = true;

        // dump the entire block
        write(STDOUT_FILENO, block, total_len);

        // now clear the flag and wake everyone who was waiting
        snapshot_pending = false;
        pthread_cond_broadcast(&snapshot_done_cv);

        // (leave next_wake.tv_nsec unchanged because interval is whole seconds)
    }
    return NULL;
}

static void print_final_statistics(void) {
    pthread_mutex_lock(&stats_lock);
    int served       = total_passengers_ridden;
    int rides_done   = total_rides_completed;
    double sum_ticket = sum_wait_ticket_queue;
    int cnt_ticket    = count_wait_ticket;
    double sum_ride   = sum_wait_ride_queue;
    int cnt_ride      = count_wait_ride;
    int capacity      = car_capacity;
    pthread_mutex_unlock(&stats_lock);

    // Figure out wall‐clock elapsed time
    struct timespec final_now;
    clock_gettime(CLOCK_MONOTONIC, &final_now);
    time_t elapsed_final = final_now.tv_sec - start_time.tv_sec;
    int hh = elapsed_final / 3600;
    int mm = (elapsed_final % 3600) / 60;
    int ss = elapsed_final % 60;

    double avg_ticket_wait   = (cnt_ticket > 0) ? (sum_ticket / cnt_ticket) : 0.0;
    double avg_ride_wait     = (cnt_ride   > 0) ? (sum_ride   / cnt_ride)   : 0.0;
    double avg_pass_per_ride = (rides_done > 0) ? ((double)served / rides_done) : 0.0;
    double utilization_pct   = (capacity > 0 && rides_done > 0)
                              ? (avg_pass_per_ride / capacity) * 100.0
                              : 0.0;

    char final_block[512];
    int m = snprintf(final_block, sizeof(final_block),
                     "\nFINAL STATISTICS:\n"
                     "Total simulation time: %02d:%02d:%02d\n"
                     "Total passengers served: %d\n"
                     "Total rides completed: %d\n"
                     "Average wait time in ticket queue: %.3f seconds\n"
                     "Average wait time in ride queue: %.3f seconds\n"
                     "Average car utilization: %.1f%% (%.1f/%d passengers per ride)\n\n",
                     hh, mm, ss,
                     served,
                     rides_done,
                     avg_ticket_wait,
                     avg_ride_wait,
                     utilization_pct,
                     avg_pass_per_ride,
                     capacity);

    // Write directly to stdout (or to mon_pipe if you prefer)
    write(STDOUT_FILENO, final_block, m);
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

//!!!!!!!!!!!!!!!!!!!!!!!!!!! TEST THIS WHEN DONE WITH PROJECT !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
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
    printf("\nExample: ./park -n 10 -c 4 -p 6 -w 8 -r 10 -t 60\n");
}

struct TimerArgs {
    int duration;
};
//for Timing the Duration of the Park
void* timer_thread(void* arg) {
    struct TimerArgs* t = (struct TimerArgs*) arg;
    sleep(t->duration);
    simulation_running = 0;
    print_barrier_printf("Simulation timer ended. Cleaning up.\n");
    print_final_statistics();
    // Wake up any threads waiting on condition variables
    // like a bell for closing, making sure threads call pthread_exit
    pthread_cond_broadcast(&can_board);
    pthread_cond_broadcast(&all_unboarded);
    pthread_cond_broadcast(&can_unboard);
    //pthread_cond_broadcast(&car_available);
    pthread_cond_broadcast(&passengers_waiting);
    free(t);
    return NULL;
}
// PASSENGER FUNCTIONS
//– Called when a passenger boards the car.
void board(Passenger* p) {
    pthread_mutex_lock(&ride_lock);
    //printf("[DEBUG] Passenger %d waiting for can_board\n", p->pass_id);
    // wait until load() calls pthread_cond_broadcast(&can_board);
    // threads wake up. check struct if they were assigned a car, if so they board
    while ((!can_load_now || p->assigned_car == NULL) && simulation_running) {
        //printf("[DEBUG] Passenger %d waiting — can_load_now=%d, assigned_car=%p\n", p->pass_id, can_load_now, (void*)p->assigned_car);
        pthread_cond_wait(&can_board, &ride_lock);
    }
    Car* my_car = p->assigned_car;
    if (my_car != NULL) {
        // ── record how long we sat in the ride‐queue ──
        {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double waited = (now.tv_sec  - p->ride_queue_enter.tv_sec)
                          + (now.tv_nsec - p->ride_queue_enter.tv_nsec) / 1e9;

            pthread_mutex_lock(&stats_lock);
            sum_wait_ride_queue += waited;
            count_wait_ride    += 1;
            pthread_mutex_unlock(&stats_lock);
        }

        print_barrier_printf("Passenger %d boarded Car %d\n", p->pass_id, my_car->car_id);
        my_car->passenger_ids[my_car->onboard_count++] = p->pass_id;
        pthread_cond_signal(&passengers_waiting);
        // if (my_car->onboard_count == my_car->capacity) {
        //     pthread_cond_signal(&all_boarded);
        // }
    }
    pthread_mutex_unlock(&ride_lock);
    sem_post(&ride_queue_semaphore);
} 

//– Called when a passenger exits the car.
void unboard(Passenger* p) {
    //printf("Passenger %d is entering unboard()\n", p->pass_id);
    pthread_mutex_lock(&ride_lock);
    // wait until car has called unload()
    Car* my_car = p->assigned_car;
    pthread_cond_wait(&my_car->can_unload, &ride_lock);
    
    if (my_car != NULL){
        pthread_mutex_lock(&stats_lock);
        total_passengers_ridden += 1;
        pthread_mutex_unlock(&stats_lock);

        print_barrier_printf("Passenger %d unboarded Car %d\n", p->pass_id, my_car->car_id);
        my_car->unboard_count++;

        int expected = (tot_passengers == 1) ? 1 : my_car->onboard_count;
        if (my_car->unboard_count == expected) {
            pthread_cond_signal(&all_unboarded);
        }
    }
    pthread_mutex_unlock(&ride_lock);
    p->assigned_car = NULL;
} 

int attempt_load_available_passenger(Car* car){ 
    int passenger_assigned = 0;
    while (!is_passenger_queue_empty(&coaster_queue) && car->assigned_count <
            car->capacity && simulation_running) {
        Passenger* p = dequeue_passenger(&coaster_queue);
        if (p != NULL) { 
            //print_timestamp();
            //printf(" DEBUG Car %d dequeued passenger %d\n", car->car_id, p->pass_id);
            p->assigned_car = car;
            car->assigned_count++;
            pthread_cond_broadcast(&can_board);
            passenger_assigned = 1;
        }
    }
    return passenger_assigned;
}
// ROLLER COASTER FUNCTIONS
// Signals passengers to call board
void load(Car* car){
    bool solo_early_departure = false;
    pthread_mutex_lock(&ride_lock);
    car->assigned_count = 0;
    car->onboard_count = 0;
    car->unboard_count = 0;
    
    car->state = LOADING;

    can_load_now = 1; // signal board() to board a passenger
    pthread_cond_broadcast(&can_board); //signal waiting passengers to board
    // Try to dequeue one passenger immediately (to catch the solo case early)
    //printf("Car %d entering load(), coaster_queue size: %d\n", car->car_id, coaster_queue.size);
    int passenger_assigned = attempt_load_available_passenger(car);
    if (passenger_assigned) {
        //pthread_cond_broadcast(&can_board);

        print_barrier_printf("Car %d invoked load()\n", car->car_id);
    }
    // Special case: If there is only one total passenger and they've boarded, leave immediately
    // if (tot_passengers == 1 && car->onboard_count == 1) {
    //     print_timestamp();
    //     printf("Only one passenger — departing immediately\n");
    //     can_load_now = 0;
    //     pthread_mutex_unlock(&ride_lock);
    //     return;
    // }
    struct timespec deadline;  // OR TIMER goes off
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += ride_wait;

    int result = 0;
    while (car->onboard_count < car_capacity && simulation_running){
        attempt_load_available_passenger(car);
        if (car->onboard_count == car_capacity) break;
        result = pthread_cond_timedwait(&passengers_waiting, &ride_lock, &deadline);
        if (tot_passengers == 1 && car->onboard_count == 1){
    

            print_barrier_printf("Only one passenger — Car %d departing immediately\n", car->car_id);
            solo_early_departure = true;
            break;
        }
        if (result == ETIMEDOUT) break;
    }
    if (car->onboard_count == 0) {
        can_load_now = 0;
        pthread_mutex_unlock(&ride_lock);
        return;
    }
    if (solo_early_departure) {
        can_load_now = 0;
        pthread_mutex_unlock(&ride_lock);
        return;
    }
    if (result == ETIMEDOUT) {


        print_barrier_printf("Car %d done waiting, departing with %d / %d\n",
                car->car_id, car->onboard_count, car->capacity);
    } else if (car->onboard_count == car->capacity){

        print_barrier_printf("Car %d is full with %d passengers\n", car->car_id, 
            car->capacity);
    } else {

        print_barrier_printf("Car %d boarded all %d assigned passengers, but is not full (capacity %d)\n",
            car->car_id, car->onboard_count, car->capacity);
    }
    can_load_now = 0;
    pthread_mutex_unlock(&ride_lock);
}
// – Simulates the ride duration.
void run(Car* car){  //int car?

    car->state = RUNNING;

    print_barrier_printf("Car %d departed for its run\n", car->car_id);
    sleep(ride_duration);
    print_barrier_printf("Car %d finished its run\n", car->car_id);
}
//– Signals passengers to call unboard()
void unload(Car* car){
    pthread_mutex_lock(&ride_lock);
    print_barrier_printf("Car %d invoked unload()\n", car->car_id);

    car->can_unload_now = true;
    pthread_cond_broadcast(&car->can_unload); //changed to car specific value
    //wait until all passengers who boarded have unboarded

    while (car->unboard_count < car->onboard_count && simulation_running) {
        //wait until all passengers unboarded
        pthread_cond_wait(&all_unboarded, &ride_lock);
    }
    pthread_mutex_lock(&stats_lock);
    total_rides_completed += 1;
    pthread_mutex_unlock(&stats_lock);

    car->unboard_count = 0; //reset after all passengers exited
    car->onboard_count = 0;
    car->can_unload_now = false;

    car->state = WAITING;

    pthread_mutex_unlock(&ride_lock);
    
    //print_timestamp();
    //printf("Car %d completed unload and will rejoin the queue.\n", car->car_id);
    
}

//roller coaster gets called by each car thread in launch_park
void* roller_coaster(void* arg){
    Car* car = (Car*)arg;
    enqueue(&car_queue, car);
    /
    // print_queue(&car_queue);  //debug
    /
    while (simulation_running) {
        pthread_mutex_lock(&car_selection_lock);
        bool can_load = !is_passenger_queue_empty(&coaster_queue) &&
                car_queue.cars[car_queue.front] == car && currently_loading == 0;
        if (can_load) {
            currently_loading = 1;
            dequeue(&car_queue);
            pthread_mutex_unlock(&car_selection_lock);
            load(car);
            currently_loading = 0;
                // Step 3: Only ride and unload if passengers boarded
            if (car->onboard_count > 0) {
                run(car);
                unload(car);
            }
            enqueue(&car_queue, car);
        } else {
            //just have cars wait if loading or car_queue line empty
            pthread_mutex_unlock(&car_selection_lock);
            usleep(1000); // small sleep to prevent tight CPU loop
        }
        // Loop repeats: car will re-enqueue itself on next cycle
    }
    pthread_exit(NULL);
}

int embark_coaster(Passenger* p){
    board(p);
    //printf("[DEBUG] Passenger %d finished boarding\n", p->pass_id);
    if (p->assigned_car != NULL) {
        unboard(p);
    }
    return 0;
}

//semaphore count starts at global_max_coaster_line, then decrements
//decrements count if >0, if it's 0, the thread blocks
void* park_experience(void* arg){
    //need to pass as pointer for load to communicate to board
    Passenger* p = (Passenger*)arg;
    print_barrier_printf("Passenger %d entered the park\n", p->pass_id);

    while (simulation_running) {
        // Exploring the Park
        int explore_time = (rand() % 10) + 1; //2-5 seconds

        print_barrier_printf("Passenger %d is exploring the park...\n", p->pass_id);
        sleep(explore_time);

        print_barrier_printf("Passenger %d finished exploring, heading to ticket booth\n", p->pass_id);
        
        // record the moment we joined the ticket line:
        clock_gettime(CLOCK_MONOTONIC, &p->ticket_queue_enter);
        // Ticket booth
        enqueue_passenger(&ticket_queue, p);
    

        print_barrier_printf("Passenger %d waiting in ticket queue\n", p->pass_id);
        pthread_mutex_lock(&ticket_booth_lock);
        //increments ride queue total, and blocks if line at max
        sem_wait(&ride_queue_semaphore); 
        // now compute how long we waited in ticket queue:
        {
            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double waited = (now.tv_sec  - p->ticket_queue_enter.tv_sec)
                        + (now.tv_nsec - p->ticket_queue_enter.tv_nsec) / 1e9;

            pthread_mutex_lock(&stats_lock);
            sum_wait_ticket_queue += waited;
            count_wait_ticket    += 1;
            pthread_mutex_unlock(&stats_lock);
        }

        print_barrier_printf("Passenger %d acquired a ticket\n", p->pass_id);
        usleep(1000000);
        pthread_mutex_unlock(&ticket_booth_lock);
        dequeue_passenger(&ticket_queue);

        // record “enter ride queue” timestamp:
        clock_gettime(CLOCK_MONOTONIC, &p->ride_queue_enter);
        enqueue_passenger(&coaster_queue, p);
        //pthread_cond_signal(&all_boarded); // wake cars waiting in load()
        pthread_cond_signal(&passengers_waiting);

        print_barrier_printf("Passenger %d joined the ride queue\n", p->pass_id);
        fflush(stdout); 

        
        embark_coaster(p);
    }
    //free(p); //free specific passenger after park hours
    pthread_exit(NULL); //all threads are done after the park hours are over
}

void launch_park(int passengers, int cars, int capacity, int wait, int ride, int park_hours,
    int max_coaster_line)
{
    // for both timer threads
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // 2) immediately spawn the 5 s–interval monitor:
    pthread_t monitor_tid;
    int *interval_arg = malloc(sizeof(int));
    *interval_arg = 5;
    pthread_create(&monitor_tid, NULL, monitor_timer_thread, interval_arg);

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
    //separate queue structure for cars
    init_car_queue(&car_queue, cars);
    //REUSEing same queue structure for ticket_queue/ride_queue
    init_passenger_queue(&ticket_queue, &ticket_booth_lock);
    init_passenger_queue(&coaster_queue, &coaster_queue_lock);

    //beginning_stats(tot_passengers, num_cars, car_capacity, ride_wait, ride_duration, park_hours);

    pthread_t* car_thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * num_cars); //threads for cars too
    pthread_t* thread_ids = (pthread_t *)malloc(sizeof(pthread_t) * tot_passengers);//array of threads
    
    all_cars = malloc(sizeof(Car) * num_cars);
    for (int i = 0; i < num_cars; i++) {
        all_cars[i].car_id = i + 1;
        all_cars[i].capacity = car_capacity;
        all_cars[i].onboard_count = 0;
        all_cars[i].unboard_count = 0;
        all_cars[i].assigned_count = 0;
        all_cars[i].passenger_ids = malloc(sizeof(int) * car_capacity);
        pthread_cond_init(&all_cars[i].can_unload, NULL);
        all_cars[i].can_unload_now = false;
        all_cars[i].state = WAITING;
        pthread_create(&car_thread_ids[i], NULL, roller_coaster, (void*)&all_cars[i]);
    }
    // START THE PARK (40 second default timer)
    struct TimerArgs* timer_args = malloc(sizeof(struct TimerArgs));
    timer_args->duration = park_hours;  // user-defined or default duration

    pthread_t timer;
    pthread_create(&timer, NULL, timer_thread, (void*) timer_args);
    //create passenger amount of threads in addition to the main thread
    //id numbers for passengser, 1, 2, 3...
    Passenger** passenger_objects = malloc(sizeof(Passenger*) * tot_passengers);
    //do not need to allocate mem for threads themselves, just the ids
    //pthread_create(store thread id, attr, function that runs in new thread, arg)
	for(int i = 0; i < passengers; i++){
        passenger_objects[i] = malloc(sizeof(Passenger));
        passenger_objects[i]->pass_id = i + 1; //so pasengers start from 1, not 0
        passenger_objects[i]->assigned_car = NULL;
        passenger_objects[i]->next = NULL;
        // threads enter the park when you call simulate_work on them
		pthread_create(&thread_ids[i], NULL, park_experience, (void*)passenger_objects[i]);
        //stagger passengers entering the park(can make random instead?)
    }
	for (int j = 0; j < passengers; ++j){
		pthread_join(thread_ids[j], NULL); // wait on our threads to rejoin main thread
	}
    pthread_join(timer, NULL);
    pthread_join(monitor_tid, NULL);
    for (int j = 0; j < num_cars; ++j){
		pthread_join(car_thread_ids[j], NULL); // wait on our threads to rejoin main thread
	}
    for (int i = 0; i < passengers; ++i) {
        free(passenger_objects[i]);  
    }
    free(thread_ids);
    free(car_thread_ids);
    free(passenger_objects);

    for (int i = 0; i < num_cars; i++){
        free(all_cars[i].passenger_ids);
    }
    free(all_cars);
    //destroy locks
    //pthread_mutex_destroy(&print_lock);
    pthread_mutex_destroy(&car_queue_lock);
    pthread_mutex_destroy(&ticket_booth_lock);
    pthread_mutex_destroy(&coaster_queue_lock);
    pthread_mutex_destroy(&ride_lock);
    pthread_mutex_destroy(&car_selection_lock);
    pthread_mutex_destroy(&stats_lock);
    //pthread_mutex_destroy(&load_lock);
    //destroy variables & booleans
    pthread_cond_destroy(&can_board);
    //pthread_cond_destroy(&all_boarded);
    pthread_cond_destroy(&can_unboard);
    pthread_cond_destroy(&all_unboarded);
    //pthread_cond_destroy(&car_available);
    pthread_cond_destroy(&passengers_waiting);
    //destroy semaphore used in size of ride_queue
    sem_destroy(&ride_queue_semaphore);
}
//int main: 1 process, 1 main thread inside it
int main(int argc, char *argv[]){
    srand(time(NULL));
    beginning_time = time(NULL);
    init_print_lock();
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
        passengers = 3;
        cars = 2;
        capacity = 2;
        wait = 20;
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
    // Create Pipe for Monitor
    // if (pipe(mon_pipe) < 0) {
    //     perror("pipe");
    //     exit(1);
    // }
    // &print_lock = mmap(NULL,
    //                           sizeof(pthread_mutex_t),
    //                           PROT_READ | PROT_WRITE,
    //                           MAP_ANONYMOUS | MAP_SHARED,
    //                           -1,
    //                           0);
    // if (&print_lock == MAP_FAILED) {
    //     perror("mmap");
    //     exit(1);
    // }

    // pthread_mutexattr_t attr;
    // pthread_mutexattr_init(&attr);
    // pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    // pthread_mutex_init(&print_lock, &attr);
    // pthread_mutexattr_destroy(&attr);

    // // 2) Create the pipe that the parent will write snapshots into:
    // if (pipe(mon_pipe) < 0) {
    //     perror("pipe");
    //     exit(1);
    // }

    // pid_t pid = fork();
    // if (pid < 0) {
    //     perror("fork");
    //     exit(1);
    // }
    // //child process is the monitor
    // if (pid == 0) {
    //     close(STDOUT_FILENO);
    //     monitor_main(mon_pipe[0]);
    //     close(mon_pipe[0]);
    //     _exit(0);
    // }
    // close(mon_pipe[0]);
    printf("===== DUCK PARK SIMULATION =====\n");
    beginning_stats(passengers, cars, capacity, wait, ride, park_hours);
    launch_park(passengers, cars, capacity, wait, ride, park_hours, max_coaster_line);
    //close(STDOUT_FILENO);
    // Wait for timer thread to finish (it will exit once simulation_running == false):
    print_barrier_printf("Closing the park.\n");
    return 0;
}

