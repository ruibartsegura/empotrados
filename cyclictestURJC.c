#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>

#ifdef DEBUG
#define DEBUG_PRINTF(...) printf("DEBUG: "__VA_ARGS__)
#else
#define DEBUG_PRINTF(...)
#endif

/*Global variables*/
#define SLEEP_TIME 100 // Sleep time
#define EXEC_TIME 5 // Execution time

typedef struct {
    int id;
    double *av_latencies;
    double *max_latencies;
} ThreadArgs;

double get_time_diif(const struct timespec *t1, const struct timespec *t2){
    long sec_diff = t2->tv_sec - t1->tv_sec;
    long nsec_diff = t2->tv_nsec - t1->tv_nsec;

    return (sec_diff * 1000.0) + (nsec_diff / 1e6);
}

// Function to treat each client in a thread
void* thread_core(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg; // Cast from void to my struc

    double latency = 0, sum = 0, max_latency = 0;
    struct timespec t1, t2;

    int num_iter = 0;
    while (1) {
        // get actual time -> sleep -> get future time and check how much has desviated from the sleep
        clock_gettime(CLOCK_MONOTONIC, &t1);
        usleep(SLEEP_TIME);
        clock_gettime(CLOCK_MONOTONIC, &t2);

        double time_diff = get_time_diif(&t1, &t2);
        latency = time_diff - SLEEP_TIME;

        // I the sleep was sorter than the time expected then the latency is perfect = 0
        if (latency <  0)
            latency = 0;

        // Check if the latency is the highest until now
        if (latency > max_latency)
            args->max_latencies[args->id] = latency;

        sum += latency; // Add the actual value to the sum of the previous latencies
        num_iter++; // Increase the iteration number to make the average

        args->av_latencies[args->id] = sum / num_iter;
    }

}


int main (int argc, char* argv[]) {
    int N = (int) sysconf(_SC_NPROCESSORS_ONLN); // Number of cores
    DEBUG_PRINTF("%d",N);

    double av_latencies[N]; // Array to store the average latencie of each core
    double max_latencies[N]; // Array to store the maximun latencie of each core
    pthread_t threads[N]; // Array to store the threads id

    static int32_t latency_target_value = 0;
    int latency_target_fd = open("/dev/cpu_dma_latency", O_RDWR);
    write(latency_target_fd, &latency_target_value, 4);


    // Make threads
    for (int i = 0; i < N; i++) {
        pthread_t thread;
        ThreadArgs* args = malloc(sizeof(ThreadArgs));

        args->id = i;
        args->av_latencies = av_latencies;
        args->max_latencies = max_latencies;

        pthread_create(&thread, NULL, thread_core, args);
        threads[i] = thread;
    }
    sleep(EXEC_TIME);
    for (int i = 0; i < N; i++) {
        pthread_join(threads[i], NULL);
    }


    
    
    // Print results
    double total_av = 0;
    double total_max = 0;
    for(int x = 0; x < N; x++) {
        printf("[%d] latencia media = %f ns. | max = %f ns", x, av_latencies[x], max_latencies[x]);
        
        total_av += av_latencies[x];
        if (total_max < av_latencies[x])
            total_max = av_latencies[x];
    }
    printf("Total latencia media = %f ns. | max = %f ns", total_av, total_max);
    
    // Close cpu_dma_latency
    if (close(latency_target_fd) == -1) {
        perror("Error closing file");
        return -1;
    }
    return 0;
}