#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>

#ifdef DEBUG
#define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

/*Global variables*/
#define SLEEP_TIME 1 // Sleep time will convert in ms
#define EXEC_TIME 60 // Execution time
bool time_flag = true; // Flag to know when the time has passed, true->runnning, false->finished

typedef struct {
    int id;
    int *num_lat; // To know how many latencies have been tested
    long long **latencies;
} ThreadArgs;

// Return the difference between t2 and t1 in ns
long long get_time_diff(const struct timespec *t1, const struct timespec *t2){
    long sec_diff = t2->tv_sec - t1->tv_sec;
    long nsec_diff = t2->tv_nsec - t1->tv_nsec;

    // If the diff of ns is lower than 0 adjust.
    if (nsec_diff < 0) {
        sec_diff -= 1;
        nsec_diff += 1000000000L;
    }

    long long time = (sec_diff * 1000000000LL) + (nsec_diff);
    return time;
}

// Function to treat each client in a thread
void* thread_core(void *arg) {
    // Set prio
    struct sched_param param;
    param.sched_priority = 99;
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

    ThreadArgs *args = (ThreadArgs *)arg; // Cast from void to my struc

    
    long long latency = 0;

    int capacity = 1000;
    long long *lats = malloc(sizeof(long long) * capacity); // some large number or estimate
    if (!lats) {
        perror("malloc");
        pthread_exit(NULL);
    }
    
    struct timespec t1, t2;
    
    int num_iter = 0;
    while (time_flag) {
        // get actual time -> sleep -> get future time and check how much has desviated from the time sleep
        clock_gettime(CLOCK_MONOTONIC_RAW, &t1);
        
        struct timespec ts;
        ts.tv_sec = 0;
        ts.tv_nsec = SLEEP_TIME * 1000000; // SLEEP_TIME in ms
        nanosleep(&ts, NULL);
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &t2);
        
        long long time_diff = get_time_diff(&t1, &t2);
        //DEBUG_PRINTF("Diff ms %lld, id [%d]\n", time_diff, args->id);
        
        latency = time_diff - SLEEP_TIME * 1000000LL;
        //DEBUG_PRINTF("Diff seg %lld\n", latency);
        
        // If the sleep was sorter than the time expected then the latency is perfect = 0
        if (latency <  0)
            latency = 0;

        // Give more memory if there are more latencies than the initial capacity
        if (num_iter >= capacity) {
            capacity *= 2; // duplicar capacidad
            long long *tmp = realloc(lats, sizeof(long long) * capacity);
            if (!tmp) {
                perror("realloc");
                free(lats);
                pthread_exit(NULL);
            }
            lats = tmp;
        }
    
        lats[num_iter] = latency;
        num_iter ++;
        }

    // Add latencies in the array
    args->latencies[args->id] = lats;
    args->num_lat[args->id] = num_iter;
    return 0;
}


int main (int argc, char* argv[]) {
    int N = (int) sysconf(_SC_NPROCESSORS_ONLN); // Number of cores
    DEBUG_PRINTF("Num of cores %d\n",N);

    long long **latencies = malloc(sizeof(long long *) * N);
    for (int i = 0; i < N; i++)
        latencies[i] = NULL;

    static int32_t latency_target_value = 0;
    int latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
    if (latency_target_fd == -1) {
        perror("Error opening /dev/cpu_dma_latency");
        free(latencies);
        return -1;
    }
    write(latency_target_fd, &latency_target_value, 4);
    
    int num_lat[N];
    pthread_t threads[N]; // Array to store the threads id
    ThreadArgs* args_array[N]; // Array to store the args to free later

    // Make threads
    for (int i = 0; i < N; i++) {
        pthread_t thread;
        ThreadArgs* args = malloc(sizeof(ThreadArgs));

        args->id = i;
        args->num_lat = num_lat;
        args->latencies = latencies;

        int thread_status = pthread_create(&thread, NULL, thread_core, args);
        if (thread_status != 0) {
            perror("Error createing thread");
            free(args);
            return -1;
        }
        // Set afinity
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
 
        args_array[i] = args;
        threads[i] = thread;
    }

    // Let the cycletest run the time we want
    sleep(EXEC_TIME);
    time_flag = false;

    // Wait for all the threads and free the memory stored
    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
        free(args_array[i]);
    }

    long long av_latencies[N], max_latencies[N];

    for (int id = 0; id < N; id++) {
        long long sum = 0, max_latency = 0; // Declare the total sum of latencies and max latency for each core
        
        for (int j = 0; j < num_lat[id]; j++ ) {
            long long latency = latencies[id][j];

            // Check if the latency is the highest until now
            if (latency > max_latency)
                max_latency = latency;
        
            sum += latency; // Add the actual value to the sum of the previous latencies
        
            av_latencies[id] = sum / (j+1);
        }
        max_latencies[id] = max_latency;
    }

    // Print results
    long long total_av = 0;
    long long total_max = 0;
    for(int x = 0; x < N; x++) {
        printf("[%d] latencia media = %lld ns. | max = %lld ns\n", x, av_latencies[x], max_latencies[x]);
        
        total_av += av_latencies[x];
        if (total_max < max_latencies[x])
            total_max = max_latencies[x];
    }
    printf("Total latencia media = %lld ns. | max = %lld ns\n", total_av / N, total_max);


    FILE *csv = fopen("cyclictestURJC.csv", "w");
    if (!csv) {
        perror("Error opening CSV file");
        exit(EXIT_FAILURE);
    }

    // header for csv
    fprintf(csv, "CPU,NUMERO_ITERACION,LATENCIA\n");

    for (int cpu = 0; cpu < N; cpu++) {
        for (int iter = 0; iter < num_lat[cpu]; iter++) {
            fprintf(csv, "%d,%d,%lld\n", cpu, iter, latencies[cpu][iter]);
        }
    }

    fclose(csv);

    // Close cpu_dma_latency
    if (close(latency_target_fd) == -1) {
        perror("Error closing file\n");
        for (int i = 0; i < N; i++) {
            free(latencies[i]);
        }
        free(latencies);
        return -1;
    }

    // Free al memory
    for (int i = 0; i < N; i++) {
        free(latencies[i]);
    }
    free(latencies);

    return 0;
}
