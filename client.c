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

#define MAX_COMMAND_SIZE 64

static int sd;    
static char *hostname = "localhost"; 
static int port = 50000;

static struct sockaddr_in sin;
static struct hostent* hp;

static char command[MAX_COMMAND_SIZE], *response;
static int string_length;
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
            "\t'quit'\t\t\t\t\t communicate to the supervisor the communication" 
            " will be closed.\n"
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
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr_list[0]))->s_addr;
    sin.sin_port = htons(port);

    // Create the socket and connect to it.     
    if (-1 == (sd = socket(AF_INET, SOCK_STREAM, 0)))
        perror_exit("ERROR: The system could not create the socket\n");
    if (-1 == connect(sd, (struct sockaddr*)&sin, sizeof(sin)))
        perror_exit("ERROR: Impossible to connect to the server's socket");

    printf("INFO: Connection to the supervisor on port %d successful\n", port);

    // A loop that runs until the command "quit" is chosen.
    for(;;) {
        // Enters the command keeping into account that there might be spaces.
        printf("Enter command (start/stop <task_name>, list, help , quit): ");
        fgets(command, MAX_COMMAND_SIZE, stdin);
        string_length = strlen(command);
        
        // Add the \0 at the end of the string.
        if ((string_length > 0) && (command[string_length-1] == '\n'))
            command[string_length-1] = '\0';

        printf("INFO: The command is '%s', (length %d)\n", command, 
                string_length);

        // If the command chosen is "help" than return the supervisor usage and 
        //skip the communication.
        if(0 == strcmp(command, "help")){
            usage(stdout);
            continue;
        } 

        // Else send the command to the supervisor. 
        network_string_length = htonl(string_length);
        // Send first the length of the command and then send it.    
        if (-1 == send(sd, &network_string_length, 
                sizeof(network_string_length), 0))
            perror_exit("ERROR: Impossible to send the length");
            
        if (-1 == send(sd, command, string_length, 0))
            perror_exit("ERROR: Impossible to send the command");

        // Wait to receive the length of the response and the response itself.
        if (-1 == receive(sd, (char *)&network_string_length, 
                sizeof(network_string_length)))
            perror_exit("ERROR: Impossible to receive the length");

        string_length = ntohl(network_string_length);
        response = malloc(string_length + 1);
        
        if (-1 == receive(sd, response, string_length)) 
            perror_exit("ERROR: Impossible to receive the response");
        
        response[string_length] = 0;
        printf("INFO: The response is '%s'\n", response);

        free(response);

        // If the command chosen is "quit" than exit the loop.
        if(0 == strcmp(command, "quit"))
            break;
    }

    // Close the socket and terminate.
    close(sd);
    printf("INFO: Connection Terminated\n");

    return 0; 
}

/******************************************************************************/
