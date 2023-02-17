/*
 * AUTHOR: Lorenzo Cappellotto
 *
 * OBJECTIVES: 
 * - how to start and stop a thread
 * - how to isolate threads on a single cpu
 * 
 */

/******************************************************************************/
// Library includes

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>

#include <pthread.h>
#include <sched.h>
#include <sys/time.h>
#include <time.h> // for clock_t, clock()
#include <errno.h>

/******************************************************************************/

/******************************************************************************/
// Main function Test thread running and sched affinity

#define MAX_THREADS 10

struct numbers {
  int a;
  int b;
  int sum;
} args[MAX_THREADS]; 

static void wait_mfw() {
    static struct timespec waitTime;
    waitTime.tv_sec = 100;
    waitTime.tv_nsec = 0;
    nanosleep(&waitTime, NULL);
}

static void* thread_function(void *arg)
{
    struct numbers* value = (struct numbers*) arg;
    for (int i = 0; i < 1; i++)
    {
        int a=value->a;
        int b=value->b;
        int result=(a+b)*5;
        value->sum = result;

        printf("value: %d\n", value->sum);
    }
    wait_mfw();

    return NULL;
}

int set_realtime_attribute(pthread_attr_t *attr, int policy, int priority, cpu_set_t *cpuset) {
    // initialize default attributes
    pthread_attr_init(attr);

    // get current thread attributes parameters
    struct sched_param param;
    int status;

    status = pthread_attr_getschedparam(attr, &param);
    if(status) {
        perror("pthread_attr_getschedparam");
        return status;
    }

    // set to not inherit parameter from parent thread
    status = pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
    if(status) {
        perror("pthread_attr_setinheritsched");
        return status;
    }

    // set the real-time scheduler as SHED_FIFO
    status = pthread_attr_setschedpolicy(attr, policy);
    if(status) {
        perror("pthread_attr_setschedpolicy");
        return status;
    }

    // set the real-time priority parameter to 50 and apply it to scheduler attributes
    param.sched_priority = priority;
    status = pthread_attr_setschedparam(attr, &param);
    if(status) {
        perror("pthread_attr_setschedparam");
        return status;
    }

    // CPU AFFINITY
    if(cpuset != NULL) {
        status = pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), cpuset);
        if(status) {
            perror("pthread_attr_setaffinity_np");
            return status;
        }
    }
    
    return status;
}

int main(int argc, char **argv) {
    int nThreads = MAX_THREADS;
    pthread_t threads[nThreads];
    int threads_active[nThreads];
    
    int routineAffinity = 2;
    if(argc != 2)
        printf("Usage: supervisor <producer_CPU_id>\n");
    else
        sscanf(argv[1], "%d", &routineAffinity);

    cpu_set_t routine_set;    
    CPU_ZERO(&routine_set);

    long number_of_processors = sysconf(_SC_NPROCESSORS_ONLN);
    if(routineAffinity >= number_of_processors)
    {
        printf("Producer processor id must be in the range: [0-%d]\n", number_of_processors-1);
        exit(EXIT_FAILURE);
    }
    printf("routineAffinity: %d, number of processors: %d\n", routineAffinity, number_of_processors);

    CPU_SET(routineAffinity, &routine_set);

    pthread_attr_t routine_attr;
    set_realtime_attribute(&routine_attr, SCHED_FIFO, 50, &routine_set);

    for (int i = 0; i < nThreads; i++) {
        threads_active[i] = 0;
        args[i].a = i;
        args[i].b = i+2;
        args[i].sum = 0;
        
        if(i < nThreads-2) {
            //pthread_create(&threads[i], NULL, thread_function, (void*)&args[i]);
            pthread_create(&threads[i], &routine_attr, thread_function, (void*)&args[i]);
            threads_active[i] = 1;
            printf("thread %d with args %d %d %d\n", i, args[i].a, args[i].b, args[i].sum);
        }
    }

    printf("\n");

    for (int i = 0; i < nThreads; i+=3) {
        if(threads_active[i] == 1) {
            printf("thread %d is cancelled\n", i);
            pthread_cancel(threads[i]);
            threads_active[i] = 2;
        }
    }

    for (int i = 0; i < nThreads; i++) {
        if(threads_active[i] == 2) {
            printf("thread %d is joined\n", i);
            pthread_join(threads[i], NULL);
            threads_active[i] = 0;
        }
    }

    printf("\n");

    for (int i = 0; i < nThreads; i++) {
        if(threads_active[i] == 1)
            printf("thread %d remains active %d now, %08x, %d %d %d\n", i, threads_active[i], threads[i], args[i].a, args[i].b, args[i].sum); 
        else
            printf("thread %d is inactive %d, %d %d %d\n", i, threads_active[i], args[i].a, args[i].b, args[i].sum); 
    }
    
    printf("\n");
    
    for (int i = 0; i < nThreads; i++) {
        if(threads_active[i] == 1)
            pthread_join(threads[i], NULL);

        printf("thread %d is active: %d and returns a value of %d %d %d\n", i, threads_active[i], i, args[i].a, args[i].b, args[i].sum);
        
    }
    
    return 0;
}

/******************************************************************************/