/*
 * AUTHOR: Lorenzo Cappellotto
 *
 * OBJECTIVES: 
 * - how to do a wcet analysis on a set of routines  
 * 
 */

/******************************************************************************/
// Library includes

#include <stdio.h>
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

#define MAX_NUMBER_ITERATIONS 1000000
#define MAX_LENGTH_FUNCTION_NAME 20
typedef struct tr {
    double times[MAX_NUMBER_ITERATIONS]; //in nanoseconds
    double max_time;
} time_result;

struct routine {
    char routine_name[MAX_LENGTH_FUNCTION_NAME];
    void* (*function_pointer)(void*);
    double period;
    double estimated_max_execution_time;
    time_result tr;
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

static void* measure_time(void* args) {
    //int max_time = -1.0;
    //double times[100];

    struct routine* current_arguments = (struct routine*) args;
    struct timespec start, end;

    //freopen ("output.txt", "a+", stdout);
    for (int i = 0; i < MAX_NUMBER_ITERATIONS; i++) {
        clock_gettime(CLOCK_REALTIME, &start);    
        current_arguments->function_pointer(NULL);
        clock_gettime(CLOCK_REALTIME, &end);
        current_arguments->tr.times[i] = (end.tv_sec - start.tv_sec) * BILLION + (end.tv_nsec - start.tv_nsec);
        
        if (current_arguments->tr.times[i] > current_arguments->tr.max_time) {
            current_arguments->tr.max_time = current_arguments->tr.times[i];
        }
    }
    //freopen ("/dev/tty", "a", stdout);

    current_arguments->estimated_max_execution_time = 1.4 * current_arguments->tr.max_time; // Add 40% to the max time to have an estimate of the maximum execution time. 
    
    return NULL;
}
/******************************************************************************/

/******************************************************************************/
// Main function

int main(int argc, char **argv) {
    printf("hello\n");

    strcpy(tasks[0].routine_name, "fun1");
    tasks[0].period = 8.0;
    tasks[0].function_pointer = &fun1;

    strcpy(tasks[1].routine_name, "fun2");
    tasks[1].period = 6.0;
    tasks[1].function_pointer = &fun2;

    strcpy(tasks[2].routine_name, "fun3");
    tasks[2].period = 5.0;
    tasks[2].function_pointer = &fun3;

    for (int i = 0; i < 2; i++)
        measure_time(&tasks[i]);
    
    for (int i = 0; i < 2; i++) {
        printf("routine_name: %s, period: %f, estimated_max_ex_time: %f, max_time (nanoseconds): %.1f\n",
            tasks[i].routine_name,
            tasks[i].period,
            tasks[i].estimated_max_execution_time,
            tasks[i].tr.max_time);
        /*printf("times: ");
        for (int i = 0; i < MAX_NUMBER_ITERATIONS; i++) {
            printf("%.1f, ", tasks[0].tr.times[i]);
        }
        printf("\n");*/
    }
    

    struct timespec start, end;
    clock_gettime(CLOCK_REALTIME, &start);    
    fun1(NULL);
    clock_gettime(CLOCK_REALTIME, &end);
    printf("Time elpased is %.1f nanoseconds\n", (end.tv_sec - start.tv_sec) * BILLION + (end.tv_nsec - start.tv_nsec));

    clock_gettime(CLOCK_REALTIME, &start);    
    wrapper(&tasks[0]);
    clock_gettime(CLOCK_REALTIME, &end);
    printf("Time elpased is %.1f nanoseconds\n", (end.tv_sec - start.tv_sec) * BILLION + (end.tv_nsec - start.tv_nsec));
    
    return 0;
}

/******************************************************************************/

