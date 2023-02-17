/*
 * AUTHOR: Lorenzo Cappellotto
 *
 * OBJECTIVES: 
 * - how to run a RTA analysis
 * 
 */

/******************************************************************************/
// Library includes

#include <stdio.h>
#include <math.h>
//#include <stdlib.h>
//#include <sys/types.h>
//#include <string.h>
//#include <unistd.h>

//#include <sys/time.h>
//#include <time.h> // for clock_t, clock()
//#include <errno.h>

/******************************************************************************/

/******************************************************************************/
// Static variables and MACRO definitions

#define MAX_NUMBER_ROUTINES 99

struct task
{
    int id_number;
    double period;
    double deadline;
    double estimated_execution_time;
    double utilization;
};

static void populate_rotine_information(struct task* mytask, int id_number, double time, double period) {
    mytask->id_number = id_number;
    mytask->estimated_execution_time = time;

    mytask->period = period;
    mytask->deadline = period;
    mytask->utilization = time / period;
}

// returns 0 if ordered by period in ascending order. returns -1 otherwise.
static int check_routine_list(struct task* routine_list, int number_of_routines) {
    if (number_of_routines == 1) {
        return 0;
    }

    //printf("for routine %d, %f \n", 0, routine_list[0].period);
    for (int i = 1; i < number_of_routines; i++) {
        //printf("for routine %d, %f > %f?\n", i, routine_list[i].period, routine_list[i-1].period);
        if (routine_list[i].period < routine_list[i-1].period) {
            return -1;
        }
    }

    return 0;
}

/******************************************************************************/

/******************************************************************************/
// Main function


struct task routines_list[MAX_NUMBER_ROUTINES];

int main(int argc, char **argv) {
    double rm_utilization_lub = log(2);
    double total_utilization = 0.0;
    int number_of_routines = 3; //4;

    if (number_of_routines > MAX_NUMBER_ROUTINES) {
        printf("The set of routines is too big\n");
        return -1;
    }
    
    //populate_rotine_information(&routines_list[0], 0, 2.0, 80.0); 
    //populate_rotine_information(&routines_list[1], 1, 8.0, 40.0); 
    //populate_rotine_information(&routines_list[2], 2, 8.0, 40.0); 
    //populate_rotine_information(&routines_list[3], 3, 9.0, 20.0); 

    //populate_rotine_information(&routines_list[0], 0, 2.0, 16.0); 
    //populate_rotine_information(&routines_list[1], 1, 4.0, 40.0); 
    //populate_rotine_information(&routines_list[2], 2, 20.0, 50.0); 

    //populate_rotine_information(&routines_list[0], 0, 10.0, 20.0); 
    //populate_rotine_information(&routines_list[1], 1, 6.0, 30.0); 
    //populate_rotine_information(&routines_list[2], 2, 10.0, 50.0); 

    //populate_rotine_information(&routines_list[0], 0, 5.0, 20.0); 
    //populate_rotine_information(&routines_list[1], 1, 10.0, 40.0); 
    //populate_rotine_information(&routines_list[2], 2, 40.0, 80.0); 

    populate_rotine_information(&routines_list[0], 0, 3.0, 8.0); 
    populate_rotine_information(&routines_list[1], 1, 4.0, 14.0); 
    populate_rotine_information(&routines_list[2], 2, 5.0, 22.0); 

    //populate_rotine_information(&routines_list[0], 0, 3.0, 9.0); 
    //populate_rotine_information(&routines_list[1], 1, 4.0, 12.0); 
    //populate_rotine_information(&routines_list[2], 2, 2.0, 18.0); 

    if (-1 == check_routine_list(&routines_list[0], number_of_routines))
        return -1;

    for (int i = 0; i < number_of_routines; i++) {
        /*printf("Currently looking at routine %d with priority %d\n", i, 99-i);
        printf("period %.1f, deadline %.1f, esitimate execution time %.1f, utilization %.3f\n",  
            routines_list[i].id_number,
            routines_list[i].period,
            routines_list[i].deadline,
            routines_list[i].estimated_execution_time,
            routines_list[i].utilization);
        printf("\n");*/

        total_utilization += routines_list[i].utilization;
        if(total_utilization > 1)
            printf("the total utilization for the routines from 0 to %d is %.3f\n", i, total_utilization);
    }
    //printf("total utilization is %.3f\n", total_utilization);

    // Assume routines_list is ordered from the highest priority task to the lowest,
    // alternatively by the shortest period to the longest.

    // Response Time Analysis (assuming SCHED_FIFO with different priorities given using )

    double response_time = 0.0, interference_time[number_of_routines], current_response_time = 0.0;
    int schedulable = 0;

    for (int i = 0; i < number_of_routines; i++) {
        response_time = -1.0; 
        current_response_time = routines_list[i].estimated_execution_time;
        printf("routine %d, execution_time: %f\n", i, current_response_time);
        
        while (current_response_time != response_time) {
            response_time = current_response_time;
            current_response_time = routines_list[i].estimated_execution_time;
            
            for (int j = 0; j < i; j++) {
                current_response_time += ceil(response_time / routines_list[j].period) * routines_list[j].estimated_execution_time;
                printf("\tceil(%f/%f) * %f (= %f)\n", 
                    response_time, 
                    routines_list[j].period, 
                    routines_list[j].estimated_execution_time, 
                    ceil(response_time / routines_list[j].period) * routines_list[j].estimated_execution_time);
            }
            printf("routine %d, current_response_time: %f\n", i, current_response_time);
        }
        printf("\n");
        
        //response_time = routines_list[i].estimated_execution_time + interference_time;
        //number_of_releases = ceil(response_time / routine_list[i].period);
        //interference_time = interference_time + number_of_releases * routines_list[i].estimated_execution_time;

        if(response_time > routines_list[i].deadline) {
            schedulable = -1;
            break;
        }
    }

    if(total_utilization < rm_utilization_lub || 0 == schedulable)
        printf("the set of tasks is schedulable using RM with utilization %.3f\n", total_utilization);
    else
        printf("the set of tasks is not schedulable using RM with utilization %.3f\n", total_utilization);

    return 0;
}

/******************************************************************************/