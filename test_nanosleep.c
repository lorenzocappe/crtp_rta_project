/*
 * AUTHOR: Lorenzo Cappellotto
 *
 * OBJECTIVES: 
 * - how to run a routine with nanosleep given the name as string
 *
 */

/******************************************************************************/
// Library includes

#include <stdio.h>
//#include <math.h>
//#include <stdlib.h>
//#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <time.h> 
#include <errno.h>

#define BILLION 1000000000.0

/******************************************************************************/

/******************************************************************************/
// Static variables and MACRO definitions

void* fun1(void* args) { 
    printf("Fun1\n"); 
}
void* fun2(void* args) { 
    printf("Fun2\n"); 
}
void* fun3(void* args) {
    printf("fun() starts \n");
    while(1) {
        if (getchar())
            break;
    }
    printf("fun() ends \n");
}

/*struct wrapper_arguments {
    void* (*function_name)();
    double period;
};*/

struct routine{
    char routine_name[20];
    double period;
    void* (*function_pointer)(void*);
} tasks[3];
  
void* wrapper(void* args) {
    struct routine* current_arguments = (struct routine*) args;

    struct timespec remain;
    int s;
    
    //while(1) {
        clock_gettime(CLOCK_REALTIME, &remain);    
        remain.tv_sec += current_arguments->period;
        current_arguments->function_pointer(NULL);
        s = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &remain, NULL);
    //}
}

/******************************************************************************/

/******************************************************************************/
// Main function

int main(int argc, char **argv) {
    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);    
    fun3(NULL);
    start.tv_sec += 2;
    int s = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &start, NULL);
    clock_gettime(CLOCK_REALTIME, &end);
    start.tv_sec -= 2;
    printf("Time elpased is %f seconds\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / BILLION);

    strcpy(tasks[0].routine_name, "fun1");
    tasks[0].period = 8.0;
    tasks[0].function_pointer = &fun1;

    strcpy(tasks[1].routine_name, "fun2");
    tasks[1].period = 6.0;
    tasks[1].function_pointer = &fun2;

    strcpy(tasks[2].routine_name, "fun3");
    tasks[2].period = 5.0;
    tasks[2].function_pointer = &fun3;

    clock_gettime(CLOCK_REALTIME, &start);    
    wrapper(&tasks[0]);
    clock_gettime(CLOCK_REALTIME, &end);
    printf("Time elpased is %f seconds\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / BILLION);

    clock_gettime(CLOCK_REALTIME, &start);    
    wrapper(&tasks[1]);
    clock_gettime(CLOCK_REALTIME, &end);
    printf("Time elpased is %f seconds\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / BILLION);

    pthread_t mythread;
    clock_gettime(CLOCK_REALTIME, &start);    
    pthread_create(&mythread, NULL, tasks[2].function_pointer, NULL);
    pthread_join(mythread, NULL);
    clock_gettime(CLOCK_REALTIME, &end);
    printf("Time elpased is %f seconds\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / BILLION);

    clock_gettime(CLOCK_REALTIME, &start);    
    pthread_create(&mythread, NULL, wrapper, &tasks[2]);
    pthread_join(mythread, NULL);
    clock_gettime(CLOCK_REALTIME, &end);
    printf("Time elpased is %f seconds\n", (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / BILLION);
            
    return 0;
}

/******************************************************************************/

