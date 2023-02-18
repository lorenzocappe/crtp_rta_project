/*
 * Author: Lorenzo Cappellotto
 *
 * Task Description: Simulation of dynamic periodic task execution. 
 * A pre-defined set of routines shall be defined with given processor usage, 
 * period and deadline. 
 * Every routine shall be composed of a program loop followed by a nanosleep() 
 * call. 
 * The exact amount of CPU time and consequently of the processor utilization 
 * can be done in advance using the time Linux command. 
 * The execution supervisor shall listen in TCP/IP for requests for task 
 * activation/deactivation and shall start a new thread running the selected 
 * routine. 
 * Before accepting a request for a new task, a response time analysis shall be 
 * carried out in order to assess the schedulability of the system. 
 * Optionally, a multicore simulation can be performed:
 * in this case, the request shall specify the index of the routine to be 
 * executed in the new task and the assigned core, and the supervisor shall 
 * carry out response time analysis for each core.
 * 
 * Impementation and Design Reasoning...
 * Used Data Structures...
 * 
 * Skills to learn:
 * -- how to start and stop a thread
 * -- how to isolate threads on a single cpu
 * -- how to do a wcet analysis on a set of routines  
 * -- how to run a RTA analysis
 * -- how to run a routine with nanosleep given the name as string
 * -- how to communicate via TCP/IP
 * 
 */

/******************************************************************************/
// Library includes

#define _GNU_SOURCE //used for sched.h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <math.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <pthread.h>
#include <sched.h>
//#include <errno.h>

/******************************************************************************/

/******************************************************************************/
// Static variables and MACRO definitions

#define MAX_LENGTH_ROUTINE_NAME 31
#define MAX_NUMBER_ROUTINES 99
#define MAX_NUMBER_CORES 8
#define DEFAULT_CORE_NUMBER 0
#define BILLION 1000000000.0
#define WTA_NUMBER_RUNS 50 // 1K

//#define MAX_THREADS 10

static const char short_options[] = "p:htsm";

static struct option long_options[] = {
    {"port",                    required_argument, 0, 'p'},
    {"help",                    no_argument, 0, 'h'},
    {"time-analysis",           no_argument, 0, 't'},
    {"multicore-scheduling",    no_argument, 0, 'm'},
    {0, 0, 0, 0}
};

static int port = 50000;
static int sd, currSd;
static int reuse = 1;

// mode_multicore == 0 means single-core, mode_multicore == 1 means multi-core.
static int mode_multicore = 0; 

static socklen_t accept_sin_length;
static struct sockaddr_in listen_sin, accept_sin; 

static int number_routines = 0;

static struct routine {
    char routine_name[MAX_LENGTH_ROUTINE_NAME];
    void *(*routine_pointer)(void*);
    double period;
    double deadline;
    double wcet;
    double utilization;
    int active_running_core[MAX_NUMBER_CORES];
} routines_list[MAX_NUMBER_ROUTINES];

static struct routine_command {
    char *action;
    char *routine_name;
    int core_number;
    int routine_index;
} parsed_command;

static char *core_number_string;

static pthread_t threads[MAX_NUMBER_CORES][MAX_NUMBER_ROUTINES];    
static cpu_set_t core_set[MAX_NUMBER_CORES];    
static long number_of_cores;
static pthread_attr_t routine_attr;

static int max_priority;

static char *routines_listing;

/******************************************************************************/

/******************************************************************************/
// Helper functions and Static functions

void *fun1(void *args) 
{ 
    int x = 1;
    //for(int i = 0; i < 500000000; i++)
    for(int i = 0; i < 5000; i++)
        x *= 1.00001;
    printf("Fun1\n"); 
}

void *fun2(void *args) 
{ 
    int x = 1;
    //for(int i = 0; i < 500000000; i++)
    for(int i = 0; i < 500000; i++)
        x *= 1.00001;
    printf("Fun2\n"); 
}

void *fun3(void *args) 
{ 
    int x = 1;
    for(int i = 0; i < 10000000; i++)
        x *= 1.00001;
    printf("Fun3\n"); 
}

void *fun4(void *args) 
{
    printf("fun() starts \n");
    //getchar();
    printf("fun() ends \n");
}

void* routine_wrapper(void *args) 
{
    struct routine *current_arguments = (struct routine*) args;

    struct timespec remain;
    int s;
    
    // An infinite loop, run the routine and wait the rest of the period. 
    for(;;) {
        clock_gettime(CLOCK_REALTIME, &remain);    
        remain.tv_sec += current_arguments->period;
        current_arguments->routine_pointer(NULL);
        s = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &remain, NULL);
    }
}

static void usage(FILE *fp) 
{
    fprintf(fp,
            "Usage: supervisor [-h --help] [-t --time-analysis] "
            "[-m --multicore-scheduling]\n\n"

            "The options are as follows:\n"
            "\t-h, --help\t\t\t give this help list.\n"
            "\t-p, --port [PORT]\t\t change the port used by the supervisor.\n"
            "\t-t, --time-analysis\t\t will run a worst time analysis on "
            "the set of tasks before starting accepting scheduling requests.\n"
            "\t-m, --multicore-scheduling\t will accept scheduling requests"
            " that specify on which core the routine needs to be run.\n");
}

static inline void perror_exit(char *text)
{
    fprintf(stderr, text);
    exit(EXIT_FAILURE);
}

static void list_routines()
{
    int counter = sprintf(routines_listing,"routine_listings:\n");
    for (int i = 0; i < number_routines; i++) {
        counter += sprintf(routines_listing+counter,
                "routine: '%s', pointer: %x, worst_execution_time: %.1f, "
                "period: %.1f, deadline: %.1f, utilization: %.3f\n",
                routines_list[i].routine_name, 
                routines_list[i].routine_pointer, 
                routines_list[i].wcet, 
                routines_list[i].period,
                routines_list[i].deadline,
                routines_list[i].utilization);
        counter += sprintf(routines_listing+counter, "\tactive cores:");
        for (int j = 0; j < number_of_cores; j++)
            counter += sprintf(routines_listing+counter, " %d", 
                    routines_list[i].active_running_core[j]);
        counter += sprintf(routines_listing+counter, "\n");
    }
}

static int receive(int sd, char *retBuf, int size)
{
    int totSize, currSize;
    totSize = 0;
    
    while(totSize < size) {
        currSize = recv(sd, &retBuf[totSize], size - totSize, 0);
        if(currSize <= 0)
            return -1;
        totSize += currSize;
    }
    return 0;
}

static void wait_mfw(time_t seconds, long nanoseconds) 
{
    static struct timespec waitTime;
    clock_gettime(CLOCK_REALTIME, &waitTime);    
    waitTime.tv_sec += seconds;
    waitTime.tv_nsec += nanoseconds;
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &waitTime, NULL);
}

static void populate_routine_info(struct routine* myroutine, 
        char *routine_name, void *(*routine_pointer)(void*), double wcet,
        double period, double deadline) 
{
    myroutine->routine_pointer = routine_pointer;
    myroutine->period = period;
    myroutine->deadline = deadline;
    myroutine->wcet = wcet;
    myroutine->utilization = wcet / period;

    // If the name is too long \0 is not inserted at the end of the string.
    // This prevents a missing \0 at the end. 
    strncpy(myroutine->routine_name, routine_name, MAX_LENGTH_ROUTINE_NAME-1);
    myroutine->routine_name[MAX_LENGTH_ROUTINE_NAME-1] = '\0'; 

    for (int i = 0; i < MAX_NUMBER_CORES; i++)
        myroutine->active_running_core[i] = 0;   
}
static void populate_routine_info_short(struct routine* myroutine, 
        char *routine_name, void *(*routine_pointer)(void*), double wcet,
        double period) 
{
    populate_routine_info(myroutine, routine_name, routine_pointer, 
            wcet, period, period);
}

static void populate_routine_list() 
{
    printf("INFO: Populating routines_list\n");

    /* First set of routines:
    populate_routine_info_short(&routines_list[0], "fun1", &fun1, 2.0, 80.0); 
    populate_routine_info_short(&routines_list[1], "fun2", &fun2, 8.0, 40.0); 
    populate_routine_info_short(&routines_list[2], "fun3", &fun3, 8.0, 40.0);
    populate_routine_info_short(&routines_list[3], "fun3", &fun3, 9.0, 20.0);
    number_routines = 4;
    */

    /* Second set of routines:
    populate_routine_info_short(&routines_list[0], "fun1", &fun1,  2.0, 16.0); 
    populate_routine_info_short(&routines_list[1], "fun2", &fun2,  4.0, 40.0); 
    populate_routine_info_short(&routines_list[2], "fun3", &fun3, 20.0, 50.0);
    number_routines = 3;
    */

    /* Third set of routines:
    populate_routine_info_short(&routines_list[0], "fun1", &fun1, 10.0, 20.0); 
    populate_routine_info_short(&routines_list[1], "fun2", &fun2,  6.0, 30.0); 
    populate_routine_info_short(&routines_list[2], "fun3", &fun3, 10.0, 50.0);
    number_routines = 3;
    */

    /* Forth set of routines:
    populate_routine_info_short(&routines_list[0], "fun1", &fun1, 5.0,  20.0); 
    populate_routine_info_short(&routines_list[1], "fun2", &fun2, 10.0, 40.0); 
    populate_routine_info_short(&routines_list[2], "fun3", &fun3, 40.0, 80.0); 
    number_routines = 3;
    */
    
    /* Fifth set of routines:
    populate_routine_info_short(&routines_list[0], "fun1", &fun1, 3.0,  9.0); 
    populate_routine_info_short(&routines_list[1], "fun2", &fun2, 4.0, 12.0); 
    populate_routine_info_short(&routines_list[2], "fun3", &fun3, 2.0, 18.0); 
    number_routines = 3;
    */

    // Sixth set of routines:
    populate_routine_info_short(&routines_list[0], "fun1", &fun1, 3.0,  8.0); 
    populate_routine_info_short(&routines_list[1], "fun2", &fun2, 4.0, 14.0); 
    populate_routine_info_short(&routines_list[2], "fun3", &fun3, 5.0, 22.0); 
    populate_routine_info_short(&routines_list[3], "fun4", &fun4, 7.0, 30.0); 
    number_routines = 4;
}

// Returns 0 if ordered by period in ascending order, -1 otherwise.
static int check_routine_list() 
{
    if (number_routines == 1)
        return 0;

    for (int i = 0; i < number_routines; i++)
        if (i > 0)
            if (routines_list[i].deadline < routines_list[i-1].deadline)
                return -1;

    return 0;
}

// Returns 0 if the routine is schedulable (on that core), -1 otherwise.
static int run_rta(int routine_index, int core_number) 
{
    // Assume routines_list is ordered from the highest priority task to the 
    // lowest (alternatively by the shortest period to the longest).
    // Assume SCHED_FIFO is used with different priorities depending on the 
    // period.
    double total_utilization = 0.0, interference_time[number_routines]; 
    double response_time = 0.0, current_response_time = 0.0;
    
    // If not in mode multicore, work as if all the routines are positioned in 
    // one core (number 0 for convinience).
    if (mode_multicore == 0)
        core_number = DEFAULT_CORE_NUMBER;

    // Add temporarely the routine to the chosen core in order to add it to the 
    // set of routines that need to be scheduled.
    routines_list[routine_index].active_running_core[core_number] = 1;

    // Compute the total utilization.
    for (int i = 0; i < number_routines; i++)
        if (1 == routines_list[i].active_running_core[core_number]) 
            total_utilization += routines_list[i].utilization;

    if (total_utilization > 1.0) {
        routines_list[routine_index].active_running_core[core_number] = 0;
        printf("ERROR: The set of tasks is not schedulable on core %d because "
                "the total utilization is %.3f\n", 
                core_number, total_utilization);
        return -1;
    }

    for (int i = 0; i < number_routines; i++) {
        // Skip the iteration for the routines not active on the core. 
        if (0 == routines_list[i].active_running_core[core_number])
            continue;

        response_time = -1.0; 
        current_response_time = routines_list[i].wcet;
        printf("routine %d, execution_time: %f\n", i, current_response_time);
        
        while (current_response_time != response_time) {
            response_time = current_response_time;
            current_response_time = routines_list[i].wcet;
            
            for (int j = 0; j < i; j++) {
                // Skip the routines not active on the core for the computation
                // of the interference time.
                if (0 == routines_list[j].active_running_core[core_number])
                    continue;

                current_response_time += 
                        ceil(response_time / routines_list[j].period) * 
                        routines_list[j].wcet;
                printf("\tceil(%f/%f) * %f (= %f)\n", 
                        response_time, 
                        routines_list[j].period, 
                        routines_list[j].wcet, 
                        ceil(response_time / routines_list[j].period) * 
                        routines_list[j].wcet);
            }
            printf("routine %d, current_response_time: %.1f, deadline: %.1f\n", 
                    i, current_response_time, routines_list[i].deadline);
        }
        
        if(response_time > routines_list[i].deadline) {
            routines_list[routine_index].active_running_core[core_number] = 0;
            printf("INFO: The set of tasks on core %d is not schedulable using "
                    "RM with utilization %.3f\n", 
                    core_number, total_utilization);
            return -1;
        }
    }

    routines_list[routine_index].active_running_core[core_number] = 0;
    printf("INFO: The set of tasks on core %d is schedulable using RM with "
            "utilization %.3f\n", core_number, total_utilization);
    return 0;
}

static int set_realtime_attribute(pthread_attr_t *attr, int policy, 
        int priority, cpu_set_t *cpuset) 
{
    struct sched_param param;
    int status;

    pthread_attr_init(attr);
    status = pthread_attr_getschedparam(attr, &param);
    if(status) {
        printf("ERROR: pthread_attr_getschedparam has failed\n");
        return status;
    }

    status = pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
    if(status) {
        printf("ERROR: pthread_attr_setinheritsched has failed\n");
        return status;
    }

    status = pthread_attr_setschedpolicy(attr, policy);
    if(status) {
        printf("ERROR: pthread_attr_setschedpolicy has failed\n");
        return status;
    }

    param.sched_priority = priority;
    status = pthread_attr_setschedparam(attr, &param);
    if(status) {
        printf("ERROR: pthread_attr_setschedparam has failed\n");
        return status;
    }

    if(cpuset != NULL) {
        status = pthread_attr_setaffinity_np(attr, sizeof(cpu_set_t), cpuset);
        if(status) {
            printf("ERROR: pthread_attr_setaffinity_np has failed\n");
            return status;
        }
    }
    
    return status;
}

static int find_routine_pointer(char* routine_name)
{
    for (int i = 0; i < number_routines; i++) {
        if (0 == strncmp(routines_list[i].routine_name, routine_name, 
                strlen(routines_list[i].routine_name))) {
                return i; 
            }
    }
    return -1;
}

// Returns 0 if the routine is started correctly, -1 otherwise.
static int start_routine()
{
    int schedulable;

    // Check if the routine is already active on the core.
    if (1 == routines_list[parsed_command.routine_index].
            active_running_core[parsed_command.core_number]) {
        printf("ERROR: The routine %d is already active on core %d\n", 
                parsed_command.routine_index, parsed_command.core_number);
        return -1;
    }
    printf("INFO: The routine %d is not already active on core %d\n", 
            parsed_command.routine_index, parsed_command.core_number);
    
    // Run RTA before starting the new thread.
    if (-1 == (schedulable = 
            run_rta(parsed_command.routine_index, parsed_command.core_number)))
        return -1;
    
    // If the set of routines is schedulable start the new thread and update
    // the metadata.   
    // The priority is chosen so to make the scheduler be Deadline Monotonic. 
    // (smaller deadlines, higher priorities given how the array is formed) 
    pthread_attr_t routine_attr;
    set_realtime_attribute(
            &routine_attr, 
            SCHED_FIFO, 
            max_priority - parsed_command.routine_index, 
            &core_set[parsed_command.core_number]);
    pthread_create(
            &threads[parsed_command.core_number][parsed_command.routine_index], 
            &routine_attr, 
            routine_wrapper, 
            &routines_list[parsed_command.routine_index]);

    routines_list[parsed_command.routine_index].
            active_running_core[parsed_command.core_number] = 1;
    printf("INFO: The thread %x is activated\n", 
            threads[parsed_command.core_number][parsed_command.routine_index]);

    return 0;
}

// Returns 0 if the routine is stopped correctly, -1 otherwise.
static int stop_routine()
{
    // Check if the routine is active on the core.
    if (0 == routines_list[parsed_command.routine_index].
            active_running_core[parsed_command.core_number]) {
        printf("ERROR: The routine %d is not active on core %d\n", 
                parsed_command.routine_index, parsed_command.core_number);
        return -1;
    }
    printf("INFO: The routine %d in core %d is active\n", 
            parsed_command.routine_index, parsed_command.core_number);

    // Stop the thread and wait for the join.
    pthread_cancel(
            threads[parsed_command.core_number][parsed_command.routine_index]);
    pthread_join(
            threads[parsed_command.core_number][parsed_command.routine_index], 
            NULL);

    routines_list[parsed_command.routine_index].
            active_running_core[parsed_command.core_number] = 0;

    printf("INFO: The thread %x corresponding to routine %d in core %d is "
            "deactivated\n", 
            threads[parsed_command.core_number][parsed_command.routine_index],
            parsed_command.routine_index, parsed_command.core_number);
    
    return 0;
}

// Returns 0 if the command has been parsed correctly, -1 otherwise.
static int parse_command(struct routine_command *parsed_command, char *command, 
        int command_length)
{
    // Try to parse the action that needs to be done.
	parsed_command->action = strtok(command, " ");
    if (parsed_command->action == NULL)
        return -1;
    
    // Check if the action requested actually exists.
    if (0 != strcmp(parsed_command->action, "start") &&
            0 != strcmp(parsed_command->action, "stop")) {
        printf("ERROR: The routine action does not exists\n");
        return -1;
    }
    
    // Try to parse the routine name.
    parsed_command->routine_name = strtok(NULL, " ");
    if (parsed_command->routine_name == NULL) {
        printf("ERROR: The routine name is missing\n");
        return -1;
    }
	
    // Find the correct routine associated to the request (if there is one).
    if (-1 == (parsed_command->routine_index = 
            find_routine_pointer(parsed_command->routine_name))) {
        printf("ERROR: The routine does not exists in the list\n");
        return -1;
    }
    
    // Try to obtain the integer core_number.
    parsed_command->core_number = DEFAULT_CORE_NUMBER;
    core_number_string = strtok(NULL, " ");
    if (mode_multicore == 1 && core_number_string != NULL)
        parsed_command->core_number = atoi(core_number_string);
                
    // Check the processor number is valid.
    if (parsed_command->core_number >= number_of_cores) {
        printf("ERROR: The core number is too high\n");
        return -1;
    }
    
    printf("INFO: The action is '%s', the routine name is '%s', "
            "the routine index is '%d', the core_number is '%d'\n", 
            parsed_command->action, parsed_command->routine_name, 
            parsed_command->routine_index, parsed_command->core_number);

    return 0;
}

static void request_handler(int currSd)
{
    unsigned int network_string_length;
    int pcr, string_length, exit_status = 0;
    char *command, *response;
    
    printf("INFO: Connection accepted from %s, starting to handle the request "
            "now \n", inet_ntoa(accept_sin.sin_addr));

    // Start a loop that runs waiting for commands until quit is chosen. 
    for(;;) {
        // Wait to receive the length of the response.
        if (-1 == receive(currSd, (char *)&network_string_length, 
                sizeof(network_string_length)))
            break;
        string_length = ntohl(network_string_length);

        command = malloc(string_length+1);
        if (-1 == receive(currSd, command, string_length))
            break;
        command[string_length] = 0;
        
         printf("INFO: The command received is '%s'\n", command);
        // Interact with the command.
        if (0 == strcmp(command, "quit")) {
            response = strdup("Closing the connection");
            exit_status = 1;
        } else if (0 == strcmp(command, "list")) {
            routines_listing = malloc(150*number_routines);
            list_routines();
            response = strdup(routines_listing);
            free(routines_listing);
        } else {
            // Try parsing the command and save the result in parsed_command.
            // The command is of type: "start/stop routine_name core_number"
            if (0 == (pcr = parse_command(&parsed_command, command, 
                    string_length))) {
                if (0 == strncmp(parsed_command.action, "stop", 4)) {
                    if (-1 == stop_routine())
                        response = strdup("The routine could not be stopped");
                    else
                        response = strdup("The routine has stopped correctly");
                } else { 
                    if (-1 == start_routine())
                        response = strdup("The routine could not be started");
                    else
                        response = strdup("The routine has started correctly");
                }
            } else {                
                response = strdup("The command given is malformed");
            }
        } 
        printf("INFO: The response is '%s'\n", response);

        string_length = strlen(response);
        network_string_length = htonl(string_length);
        if (-1 == send(currSd, &network_string_length, 
                sizeof(network_string_length), 0))
            break;
        if (-1 == send(currSd, response, string_length, 0))
            break;
        
        free(command);
        free(response);
        if (exit_status) 
            break;
    }

    printf("INFO: Connection terminated with %s\n", 
            inet_ntoa(accept_sin.sin_addr));
    close(currSd);
}

static void connection_handler() 
{
    printf("INFO: Preparing the socket to accept requests\n");
    
    // Create the socket and connect to it.     
    if (-1 == (sd = socket(AF_INET, SOCK_STREAM, 0)))
        perror_exit("ERROR: The system could not create the socket\n");
    
    if (-1 == setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, 
            sizeof(reuse)))
        perror_exit("ERROR: setsockopt(SO_REUSEADDR) failed\n");
    if (-1 == setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, 
            sizeof(reuse)))
        perror_exit("ERROR: setsockopt(SO_REUSEPORT) failed\n");

    // Fill the info on the socket in the correct variable. 
    memset(&listen_sin, 0, sizeof(listen_sin));
    listen_sin.sin_family = AF_INET;
    listen_sin.sin_addr.s_addr = INADDR_ANY;
    listen_sin.sin_port = htons(port);

    // Bind the correct socket.
    if (-1 == bind(sd, (struct sockaddr *)&listen_sin, sizeof(listen_sin)))
        perror_exit("ERROR: The system could not bind the socket\n");

    // Start listening and accept at most 5 connections (queue length).
    if (-1 == listen(sd, 5))
        perror_exit("ERROR: listen() was unsuccessful on the socket.");

    accept_sin_length = sizeof(accept_sin);

    printf("INFO: Ready to accept new connection requests\n");
    for(;;) {
        // Wait and accept the connection request.
        if (-1 == (currSd = accept(sd, (struct sockaddr *) &accept_sin, 
                &accept_sin_length)))
            printf("ERROR: The socket could not accept the connection\n");
        else
            request_handler(currSd);
    }
    close(sd);
}

static void prepare_cpu_sets() 
{
    number_of_cores = sysconf(_SC_NPROCESSORS_ONLN);
    max_priority = sched_get_priority_max(SCHED_FIFO);
    printf("INFO: The maximum priority for SCHED FIFO is %d\n", max_priority);

    if (number_of_cores > MAX_NUMBER_CORES)
        perror_exit("ERROR: the real number of cores is greater than the "
                "maximum number\n");
    
    for (size_t i = 0; i < number_of_cores; i++) {
        CPU_ZERO(&core_set[i]);
        CPU_SET(i, &core_set[i]);
    }
}

static double routine_wcet_analysis(struct routine *myroutine) 
{
    double temp, result = 0.0;
    struct timespec start, end;

    for (int i = 0; i < WTA_NUMBER_RUNS; i++) {
        clock_gettime(CLOCK_REALTIME, &start);    
        myroutine->routine_pointer(NULL);
        clock_gettime(CLOCK_REALTIME, &end);    

        temp = (end.tv_sec - start.tv_sec)  
                + (end.tv_nsec - start.tv_nsec) / BILLION;
        if (temp > result)
            result = temp;
    }

    return result;
}

static void routines_list_wcet_analysis() 
{
    printf("INFO: Disregard the following output\n");
    wait_mfw(3, 0);

    // Doing the worst time analysis on each routine and saving the times.
    for (int i = 0; i < number_routines; i++)
        routines_list[i].wcet = routine_wcet_analysis(&routines_list[i]);    

    for (int i = 0; i < number_routines; i++) {
        routines_list[i].wcet *= 1.5;
        routines_list[i].utilization = 
                routines_list[i].wcet / routines_list[i].period;
    }
}

/******************************************************************************/

/******************************************************************************/
// Main function

int main (int argc, char **argv) 
{ 
    populate_routine_list();
    printf("INFO: The routines_list has been populated\n");

    if(-1 == check_routine_list())
        perror_exit("ERROR: The routines_list is not correctly formatted");
    printf("INFO: The routines_list is correctly formatted\n");
    
    prepare_cpu_sets();
    printf("INFO: The core_sets have been set\n");
    
    printf("INFO: The complete list of routines is:\n");
    routines_listing = malloc(150*number_routines);
    list_routines();
    printf("%s", routines_listing);

    int index_options, c;
    for (;;) {
        c = getopt_long(argc, argv, short_options, long_options, 
                &index_options);
        if (-1 == c)
            break;

        switch (c) {
        case 0: /* getopt_long() flag */
            break;
        case 'h':
            usage(stdout);
            exit(EXIT_SUCCESS);
        case 't':
            printf("INFO: Starting the worst time analysis for the routines "
                    "available\n");
            routines_list_wcet_analysis();
            printf("INFO: Terminating the worst time analysis\n");
            
            printf("INFO: The updated complete list of routines is:\n");
            list_routines();
            printf("%s", routines_listing);
            break;
        case 'm':
            mode_multicore = 1;
            printf("INFO: Multicore option selected\n");
            break;
        case 'p':
            port = atoi(optarg);
            printf("INFO: The port %d will be used to accept requests\n", 
                    port);
            break;
        default:
            usage(stdout);
            exit(EXIT_FAILURE);
        }
    }
    free(routines_listing);


    connection_handler();
    printf("INFO: This point is never reached\n");

    return 0;
}

/******************************************************************************/