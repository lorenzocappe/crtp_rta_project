/*
 * Author: Lorenzo Cappellotto
 */

/******************************************************************************/
// Library includes

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

/******************************************************************************/

/******************************************************************************/
// Static variables and MACRO definitions

#define DEFAULT_PORT 50000
#define MAX_NUMBER_ROUTINES 98
#define MAXLINE 150

static int sd;    
static char *hostname = "localhost"; 
static int port = DEFAULT_PORT;

static struct sockaddr_in sin;
static socklen_t sin_length;
static struct hostent* hp;

static char command[MAXLINE+1], response[MAXLINE*MAX_NUMBER_ROUTINES+1];
static int command_length, response_length;
static unsigned int network_string_length;

/******************************************************************************/

/******************************************************************************/
// Helper functions and Static functions

static void usage(FILE *fp) 
{
    fprintf(fp,
            "The list of commands available is the following:\n"
            "\t'help'\t\t\t\t\t give this help list.\n"
            "\t'list'\t\t\t\t\t list all the routines and where they are "
            "active\n" 
            "\t'quit'\t\t\t\t\t to terminate the client.\n"
            "\t'start routine_name [processor_number]'\t start the routine "
            "given its name and optionally provide which processor to use.\n"
            "\t'stop routine_name [processor_number]'\t stop the routine "
            "given its name and optionally the processor on which it's running."
            "\nThe processor number will be ignored if the supervisor is not "
            "running in multicore mode\n");
}

static inline void perror_exit(char* text) 
{
    perror(text);
    exit(EXIT_FAILURE);
}

/******************************************************************************/
// Main function

int main (int argc, char **argv) 
{
    hp = gethostbyname(hostname);

    if (argc > 1)
        if (0 == strcmp("-p", argv[1]) || 0 == strcmp("--port", argv[1])) {
            if (argc != 3) {
                perror_exit("Usage: client [--port server_port]\n");
            } else {
                port = atoi(argv[2]);
                printf("INFO: The port %d will be used.\n", port);
            }
        }

    // If there is no known host with the required hostname fail.
    if (0 == hp)
        perror_exit("ERROR: The system could not translate the hostname");

    // Fill the info on the server's socket in the correct variable. 
    sin_length = sizeof(sin);
    memset(&sin, 0, sin_length);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr_list[0]))->s_addr;
    sin.sin_port = htons(port);

    // Create the socket.     
    if (-1 == (sd = socket(AF_INET, SOCK_DGRAM, 0)))
        perror_exit("ERROR: The system could not create the socket\n");

    printf("INFO: Ready to send packets to the supervisor on port %d\n", port);

    // A loop that runs until the command "quit" is chosen.
    for(;;) {
        // Enters the command keeping into account that there might be spaces.
        printf("Enter command (start/stop <task_name>, list, help , quit): ");
        fgets(command, MAXLINE, stdin);
        command_length = strlen(command);
        
        // Add the \0 at the end of the string.
        if ((command_length > 0) && (command[command_length-1] == '\n'))
            command[command_length-1] = '\0';

        printf("INFO: The command is '%s', (length %d)\n", command, 
                command_length);

        // If the command chosen is "help" than return the supervisor usage and 
        //skip the communication.
        if(0 == strcmp(command, "help")){
            usage(stdout);
            continue;
        } 
        // If the command chosen is "quit" than exit the loop.
        if(0 == strcmp(command, "quit"))
            break;

        // Else send the command to the supervisor. 
        if (-1 == sendto(sd, (const char *)&command[0], command_length, 
            MSG_CONFIRM, (const struct sockaddr *) &sin, sin_length))
            perror_exit("ERROR: Impossible to send the command");

        // Wait to receive the length of the response and the response itself.
        if (-1 == (response_length = recvfrom(sd, (char *)&response[0], 
                MAXLINE*MAX_NUMBER_ROUTINES, MSG_WAITALL, (struct sockaddr *) &sin, 
                &sin_length))) { }
	    response[response_length] = '\0';

        printf("INFO: The response is '%s'\n", response);
    }

    // Close the socket and terminate.
    close(sd);
    printf("INFO: Connection Terminated\n");

    return 0; 
}

/******************************************************************************/
