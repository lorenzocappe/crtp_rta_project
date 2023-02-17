/*
 * AUTHOR: Lorenzo Cappellotto
 *
 * OBJECTIVES: 
 * - ?
 *
 */

/******************************************************************************/
// Library includes

#include <stdio.h>
//#include <math.h>
//#include <stdlib.h>
//#include <sys/types.h>
//#include <string.h>
//#include <pthread.h>

//#include <time.h> 
//#include <errno.h>

#include <sys/resource.h>
#include <unistd.h>

/******************************************************************************/

/******************************************************************************/
// Static variables and MACRO definitions

/******************************************************************************/

/******************************************************************************/
// Main function

int main(int argc, char **argv) {
    int prio;
    
    prio = getpriority(PRIO_PROCESS, 0);
    printf("priority: %d\n", prio);
    sleep(20);

    if (-1 == setpriority(PRIO_PROCESS, 0, 0)) {
        printf("errore\n");
        return -1;
    }

    prio = getpriority(PRIO_PROCESS, 0);
    printf("priority: %d\n", prio);
    sleep(2);

    if (-1 == setpriority(PRIO_PROCESS, 0, 19)) {
        printf("errore\n");
        return -1;
    }

    prio = getpriority(PRIO_PROCESS, 0);
    printf("priority: %d\n", prio);
    sleep(2);

    if (-1 == setpriority(PRIO_PROCESS, 0, -20)) {
        printf("errore\n");
        return -1;
    }

    prio = getpriority(PRIO_PROCESS, 0);
    printf("priority: %d\n", prio);
    sleep(2);

    nice(20);
    prio = getpriority(PRIO_PROCESS, 0);
    printf("priority: %d\n", prio);
    sleep(2);

    nice(19);
    prio = getpriority(PRIO_PROCESS, 0);
    printf("priority: %d\n", prio);
    sleep(2);
    
    nice(-39);
    prio = getpriority(PRIO_PROCESS, 0);
    printf("priority: %d\n", prio);
    sleep(2);

    return 0;
}

/******************************************************************************/

