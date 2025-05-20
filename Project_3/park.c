#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <errno.h>
#define NUM_WORKERS 1 //cut later

volatile int simulation_running = 1; // Parks Hours of Operation
struct timespec start_time; //timestamps
pthread_mutex_t ticket_booth_lock;
pthread_mutex_t ride_lock;
pthread_mutex_t queue_lock;
//use these inside, board, unboard, load, run, & unload
pthread_cond_t can_board, all_boarded, can_unboard, all_unboarded;

//condition variables
pthread_cond_t can_board      = PTHREAD_COND_INITIALIZER;
pthread_cond_t all_boarded    = PTHREAD_COND_INITIALIZER;
pthread_cond_t can_unboard    = PTHREAD_COND_INITIALIZER;
pthread_cond_t all_unboarded  = PTHREAD_COND_INITIALIZER;
int can_load_now = 0;
int can_unload_now = 0;
pthread_cond_t car_available;

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
void beginning_stats(int passengers, int cars, int capacity, int wait, int ride){
    //printf("[Monitor] Simulation started with parameters:\n");
    printf("- Number of passenger threads: %d\n", passengers);
    printf("- Number of cars: %d\n", cars);
    printf("- Capacity per car: %d\n", capacity);
    printf("- Park exploration time: 2-5 seconds\n");
    printf("- Car waiting period: %d seconds\n", wait);
    printf("- Ride duration: %d seconds\n", ride);
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

typedef struct {
    int car_id;
    int capacity;
    int unboard_count;
    int onboard_count;
    int* passenger_ids; //array of passenger IDs
} Car;
Car* car_struct;

typedef struct {
    Car** cars;      // array of car pointers
    int front;
    int rear;
    int curr_size;
    int queue_size;
} CarQueue;

void init_queue(CarQueue* q, int num_cars) {
    q->cars = (Car**)malloc(sizeof(Car*) * num_cars);
    q->front = 0;
    q->rear = 0;
    q->curr_size = 0;
    q->queue_size = num_cars;
}

bool is_empty(CarQueue* q) {
    return q->curr_size == 0;
}

bool is_full(CarQueue* q) {
    return q->curr_size == q->queue_size;
}

void enqueue(CarQueue* q, Car* car) {
    pthread_mutex_lock(&queue_lock);

    if (is_full(q)) {
        pthread_mutex_unlock(&queue_lock);
        return;
    }
    q->cars[q->rear] = car;
    q->rear = (q->rear + 1) % q->queue_size;
    q->curr_size++;
     // Signal that a car is now available
    pthread_cond_signal(&car_available);
    
    pthread_mutex_unlock(&queue_lock);
}

Car* dequeue(CarQueue* q) {
    pthread_mutex_lock(&queue_lock);

    // wait until a car is available
    if (is_empty(q)) {
        pthread_cond_wait(&car_available, &queue_lock);
    }

    Car* car = q->cars[q->front];
    q->front = (q->front + 1) % q->queue_size;
    q->curr_size--;

    pthread_mutex_unlock(&queue_lock);
    return car;
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
CarQueue car_queue;

//for Timing the Duration of the Park
void* timer_thread(void* arg) {
    sleep(40); // duration of simulation
    simulation_running = 0;
    print_timestamp();
    printf("Simulation timer ended. Closing the park.\n");
    return NULL;
}
/*Functions you need
pthread_create()
pthread_exit()
pthread_join()
pthread_mutex_lock()
pthread_mutex_unlock()
pthread_cond_wait()
pthread_cond_broadcast() or pthread_cond_signal()
• pthread_barrier_wait()
• pthread_barrier_init()
• sched_yield() (optional but strongly recommended)*/

// PASSENGER FUNCTIONS
//– Called when a passenger boards the car.
void board(int passenger_id, Car* car) {
    pthread_mutex_lock(&ride_lock);
    // wait until load() calls pthread_cond_broadcast(&can_board);
    while (!can_load_now) {
        pthread_cond_wait(&can_board, &ride_lock);
    }
    print_timestamp();
    printf("Passenger %d boarded Car %d\n", passenger_id, car->car_id);
    car->passenger_ids[car->onboard_count++] = passenger_id;

    if (car->onboard_count == car->capacity) {
        pthread_cond_signal(&all_boarded);
    }
    pthread_mutex_unlock(&ride_lock);
} 

//– Called when a passenger exits the car.
void unboard(int passenger_id, Car* car) {
    pthread_mutex_lock(&ride_lock);
    // wait until car has called unload()
    while (!can_unload_now) {
        pthread_cond_wait(&can_unboard, &ride_lock);
    }
    print_timestamp();
    printf("Passenger %d unboarded Car %d\n", passenger_id, car->car_id);
    car->unboard_count++;

    if (car->unboard_count == car->onboard_count) {
        pthread_cond_signal(&all_unboarded);
    }
    pthread_mutex_unlock(&ride_lock);
    //??? DO PASENGERS RESET OR ARE THEY DONE AFTER THEY RIDE ONCE?
    // After unboarding, simulate roaming the park
    print_timestamp();
    printf("Passenger %d is exploring the park...\n", passenger_id);
} 

// ROLLER COASTER FUNCTIONS
// Signals passengers to call board
void load(Car* car){
    pthread_mutex_lock(&ride_lock);
    car->onboard_count = 0;
    car->unboard_count = 0;
    
    print_timestamp();
    printf("Car %d invoked load()\n", car->car_id);

    can_load_now = 1; // signal board() to board a passenger
    pthread_cond_broadcast(&can_board);

    //Edge case for 1 passenger
    if (tot_passengers == 1) {
        print_timestamp();
        printf("Only one passenger — departing immediately\n");
        can_load_now = 0;
        pthread_mutex_unlock(&ride_lock);
        return;
    }
    // OR TIMER goes off
    struct timespec deadline;
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec += ride_wait;
    int timed_out = 0;
    while (car->onboard_count < car->capacity) {
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
    } else {
        print_timestamp();
        printf("Car %d is full with %d passengers\n", car->car_id, 
            car->capacity);
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

void* roller_coaster(void* arg){
    Car* car = (Car*)arg;
    while (1) {
        sleep(ride_wait);
        load(car);
        run(car);
        unload(car);
        enqueue(&car_queue, car);
    }
    return NULL;
}

int embark_coaster(int id){
    Car* car = dequeue(&car_queue);
    if (!car) {
        fprintf(stderr, "Passenger %d could not find a car!\n", id);
        pthread_exit(NULL);
    }
    board(id, car);
    unboard(id, car);
    return 0;
}

void* park_experience(void* arg){
    //weird pointer stuff
	int id = *(int *)arg;// cast back to the type we are expecting!
	free(arg);

    print_timestamp();
    printf("Passenger %d entered the park\n", id);
    while (simulation_running) {
        // Exploring the Park
        int explore_time = (rand() % 4) + 2; //2-5 seconds
        print_timestamp();
        printf("Passenger %d is exploring the park...\n", id);
        sleep(explore_time);
        print_timestamp();
        printf("Passenger %d finished exploring, heading to ticket booth\n", id);
        
        // Ticket booth
        print_timestamp();
        printf("Passenger %d waiting in ticket queue\n", id);
        pthread_mutex_lock(&ticket_booth_lock);
        print_timestamp();
        printf("Passenger %d acquired a ticket\n", id);
        pthread_mutex_unlock(&ticket_booth_lock);

        //Ride Queue
        print_timestamp();
        printf("Passenger %d joined the ride queue\n", id);
        embark_coaster(id);
        sleep(1);  //
    }
    pthread_exit(NULL); //threads are done after they ride the coaster
}

void launch_park(int passengers, int cars, int capacity, int wait, int ride)
{
    // Set global variables
    tot_passengers = passengers;
    num_cars = cars;
    car_capacity = capacity;
    ride_wait = wait;
    ride_duration = ride;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    init_queue(&car_queue, cars);
    beginning_stats(tot_passengers, num_cars, car_capacity, ride_wait, ride_duration);
    pthread_mutex_init(&ticket_booth_lock, NULL); //initialize mutex for ticket queue
    pthread_mutex_init(&ride_lock, NULL);
    pthread_mutex_init(&queue_lock, NULL);
    pthread_cond_init(&car_available, NULL);
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
    for (int j = 0; j < num_cars; ++j){
		pthread_join(car_thread_ids[j], NULL); // wait on our threads to rejoin main thread
	}
    // START THE PARK (40 second timer)
    pthread_t timer;
    pthread_create(&timer, NULL, timer_thread, NULL);  // start the 40s timer

    //create passenger amount of threads in addition to the main thread
    //id numbers for passengser, 1, 2, 3...
    int* people_id_nums = malloc(sizeof(int) * tot_passengers);
    //do not need to allocate mem for threads themselves, just the ids
    //pthread_create(store thread id, attr, function that runs in new thread, arg)
	for(int i = 0; i < passengers; i++){
        people_id_nums[i] = i + 1; //so pasengers start from 1, not 0
        // threads enter the park when you call simulate_work on them
		pthread_create(&thread_ids[i], NULL, park_experience, (void*)&(people_id_nums[i]));
        usleep(100000); //stagger passengers entering the park(can make random instead?)
    }
	for (int j = 0; j < passengers; ++j){
		pthread_join(thread_ids[j], NULL); // wait on our threads to rejoin main thread
	}
    free(thread_ids);
    free(car_thread_ids);
    free(people_id_nums);
    for (int i = 0; i < num_cars; i++){
        free(all_cars[i].passenger_ids);
    }
    free(all_cars);
    pthread_mutex_destroy(&ticket_booth_lock); //destroy it at the end
    pthread_mutex_destroy(&ride_lock);
}
//int main: 1 process, 1 main thread inside it
int main(int argc, char *argv[]){
    int opt;
    int passengers = -1, cars = -1, capacity = -1, wait = -1, ride = -1;

    while ((opt = getopt(argc, argv, "n:c:p:w:r:")) != -1) {
        switch (opt) {
            case 'n': passengers = atoi(optarg); break;
            case 'c': cars = atoi(optarg); break;
            case 'p': capacity = atoi(optarg); break;
            case 'w': wait = atoi(optarg); break;
            case 'r': ride = atoi(optarg); break;
            default:
                fprintf(stderr, "Usage: ./park -n INT -c INT -p INT -w INT -r INT\n");
                return 1;
        }
    }
    // Check for missing args
    if (passengers < 0 || cars < 0 || capacity < 0 || wait < 0 || ride < 0) {
        fprintf(stderr, "Error: missing or invalid arguments\n");
        exit(EXIT_FAILURE);
    }
    printf("===== DUCK PARK SIMULATION =====\n");
    launch_park(passengers, cars, capacity, wait, ride);
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
