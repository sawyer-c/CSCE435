//Calculates the mean and standard deviation of a list
//
//Sawyer Cowan
//226005454
//
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_THREADS     65536
#define MAX_LIST_SIZE   268435456


int num_threads;		// Number of threads to create - user input 

int thread_id[MAX_THREADS];	// User defined id for thread
pthread_t p_threads[MAX_THREADS];// Threads
pthread_attr_t attr;		// Thread attributes 

pthread_mutex_t lock_value;	// Protects mean and std_dev
pthread_cond_t cond_barrier; // Added barrier for when the condition set is met
double mean;    // Global Mean value
double std_dev; //Global STD-Deviation value
int count;			// Count of threads that have updated the stats

int list[MAX_LIST_SIZE];	// List of values
int list_size;			// List size

// Thread routine to compute mean and std_dev of sublist assigned to thread; 
// update global value of stats if necessary
void *find_stats (void *s) {
    int j;
    int my_thread_id = *((int *)s);

    int block_size = list_size/num_threads;
    int my_start = my_thread_id*block_size;
    int my_end = (my_thread_id+1)*block_size-1;
    if (my_thread_id == num_threads-1) my_end = list_size-1;

    double my_sum = 0;
    for (int i = my_start; i < my_end; i++)
    {
        my_sum = my_sum + list[i];
    }

    
    //lock the mutex
    pthread_mutex_lock(&lock_value);

    //If beginning of count....
    if(count==0){
        //initialize
        mean = my_sum;
    }

    //otherwise....
    else
    {
        mean = mean + my_sum;
    }

    //increment counter
    count++;
    
    //unlock mutex
    pthread_mutex_unlock(&lock_value);
    
    //we calculate the mean when all the threads are done adding
    if(count == num_threads){
        mean = mean/(1.00*(list_size));
        
        //reset counter
        count = 0;

        pthread_cond_broadcast(&cond_barrier);
        pthread_mutex_unlock(&lock_value);
    }
    else //we wait for threads to complete
    {
        pthread_cond_wait(&cond_barrier, &lock_value);
        pthread_mutex_unlock(&lock_value);
    }
    //set the local copy of the std_dev
    double my_std_dev = 0.0;

    //find the local std_dev per thread and sum them
    for (int i = my_start; i < my_end; i++)
    {
        my_std_dev = my_std_dev + (list[i]-mean)*(list[i]-mean); //formula for Standard Deviation
    }
    
    //lock once all are added
    pthread_mutex_lock(&lock_value);

    //initialize the std_dev
    if(count==0){
        std_dev = my_std_dev;
    }
    else //otherwise add the local std-dev
    {
        std_dev = std_dev + my_std_dev;
    }
    //increment counter once again
    count++;

    //when we have finished the sum
    if (count == num_threads)
    {
        //find true standard deviation with the Math::sqrt() function
        std_dev = sqrt(std_dev/(1.00*(list_size)));

        pthread_cond_broadcast(&cond_barrier);
        pthread_mutex_unlock(&lock_value);
    }
    else //otherwise, wait for threads to finish
    {
        pthread_cond_wait(&cond_barrier, &lock_value);
        pthread_mutex_unlock(&lock_value);
    }
    
    // Thread exits
    pthread_exit(NULL);
}

// Main program - set up list of randon integers and use threads to find
// the mean and standard deviation of the list
int main(int argc, char *argv[]) {

    struct timespec start, stop;
    double total_time, time_res;
    int i, j; 
    double true_mean = 0.0;
    double true_std_dev = 0.0;

    if (argc != 3) {
	printf("Need two integers as input \n"); 
	printf("Use: <executable_name> <list_size> <num_threads>\n"); 
	exit(0);
    }
    if ((list_size = atoi(argv[argc-2])) > MAX_LIST_SIZE) {
	printf("Maximum list size allowed: %d.\n", MAX_LIST_SIZE);
	exit(0);
    }; 
    if ((num_threads = atoi(argv[argc-1])) > MAX_THREADS) {
	printf("Maximum number of threads allowed: %d.\n", MAX_THREADS);
	exit(0);
    }; 
    if (num_threads > list_size) {
	printf("Number of threads (%d) < list_size (%d) not allowed.\n", num_threads, list_size);
	exit(0);
    }; 

    // Initialize mutex and attribute structures
    pthread_mutex_init(&lock_value, NULL);
    pthread_cond_init(&cond_barrier, NULL); //added
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // Initialize list, compute mean and std_dev to verify result
    srand48(0); 	// seed the random number generator
    list[0] = lrand48(); 
    true_mean = list[0];
    for (j = 1; j < list_size; j++) {
	    list[j] = lrand48(); 
        true_mean = true_mean + list[j];
	
    }
    //Calculate the real mean
    true_mean = true_mean / (1.00*(list_size));

    //Calculate the real standard deviation
    for (int i = 0; i < list_size; i++)
    {
        true_std_dev = true_std_dev + 1.00*((list[i]-true_mean)*(list[i]-true_mean));
    }
    
    // Initialize count
    count = 0;

    // Create threads; each thread executes find_stats
    clock_gettime(CLOCK_REALTIME, &start);
    for (i = 0; i < num_threads; i++) {
	thread_id[i] = i; 
	pthread_create(&p_threads[i], &attr, find_stats, (void *) &thread_id[i]); 
    }
    // Join threads
    for (i = 0; i < num_threads; i++) {
	pthread_join(p_threads[i], NULL);
    }

    // Compute time taken
    clock_gettime(CLOCK_REALTIME, &stop);
    total_time = (stop.tv_sec-start.tv_sec)
	+0.000000001*(stop.tv_nsec-start.tv_nsec);

    // Check answer
    if (true_mean != true_mean) {
	printf("Houston, we have a problem!\n"); 
    }
    // Print time taken
    printf("Threads = %d, mean = %f, standard deviation = %f, time (sec) = %8.4f\n", 
	    num_threads, mean, std_dev, total_time);

    // Destroy mutex and attribute structures
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&lock_value);
}

