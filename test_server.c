/*
 * AUTHOR: Lorenzo Cappellotto
 *
 * OBJECTIVES: 
 * - how to communicate via TCP/IP
 * 
 */

/******************************************************************************/
// Library includes

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/******************************************************************************/

/******************************************************************************/
// Static variables and MACRO definitions

static void wait_mfw() {
    static struct timespec waitTime;
    clock_gettime(CLOCK_REALTIME, &waitTime);    
    waitTime.tv_sec = 10;
    waitTime.tv_nsec = 0;
    clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &waitTime, NULL);
}

static inline void perror_exit(char* text, int code) {
    perror(text);
    exit(0);
}

static int receive(int sd, char *retBuf, int size) {
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

static int request_handler(int currSd) {
    unsigned int network_command_length;
    int command_length, exit_status = 0;
    char *command, *answer;
    
    while(1) {
        if (-1 == receive(currSd, (char *)&network_command_length, sizeof(network_command_length)))
            break;

        command_length = ntohl(network_command_length);
        command = malloc(command_length+1);
        
        receive(currSd, command, command_length);
        command[command_length] = 0;

        if(0 == strcmp(command, "help"))
            answer = strdup(
                "server is active.\n"
                "    commands:\n"
                "       help: print this help\n"
                "       quit: stop client connection\n"
                "       stop: force stop server connection\n"
            );
        else if (0 == strcmp(command, "quit")) {
            answer = strdup("Closing server connection");
            exit_status = 1;
        }
        else{
            printf("command: '%s', ", command);
            answer = strdup("trying...");
        } 
        printf("%s\n", answer);

        command_length = strlen(answer);
        network_command_length = htonl(command_length);

        if (-1 == send(currSd, &network_command_length, sizeof(network_command_length), 0))
            break;
        if (-1 == send(currSd, answer, command_length, 0))
            break;
        
        free(command);
        free(answer);
    
        if (exit_status) 
            break;

    }

    printf("Connection terminated\n");
    
    close(currSd);
    return 0;
}

/******************************************************************************/

/******************************************************************************/
// Main function

int main(int argc, char **argv) {
    int port = 50000;
    int sd, currSd;
    int reuse = 1;
    socklen_t accept_sin_length;
    struct sockaddr_in listen_sin, accept_sin; 

    if (argc == 2)
        sscanf(argv[1], "%d", port);

    if (-1 == (sd = socket(AF_INET, SOCK_STREAM, 0)))
        perror_exit("ERROR: The system could not create the socket\n", 1);

    if (-1 == setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char*)&reuse, sizeof(reuse)))
        perror_exit("ERROR: setsockopt(SO_REUSEADDR) failed\n", 1);
    if (-1 == setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, (const char*)&reuse, sizeof(reuse)))
        perror_exit("ERROR: setsockopt(SO_REUSEPORT) failed\n", 1);

    memset(&listen_sin, 0, sizeof(listen_sin));
    listen_sin.sin_family = AF_INET;
    listen_sin.sin_addr.s_addr = INADDR_ANY;
    listen_sin.sin_port = htons(port);

    if (-1 == bind(sd, (struct sockaddr *)&listen_sin, sizeof(listen_sin)))
        perror_exit("ERROR: The system could not bind the socket\n", 1);

    if (-1 == listen(sd, 5))
        perror_exit("ERROR: The system could not prepare the socket to accept connections", 1);

    accept_sin_length = sizeof(accept_sin);
    while (1)
    {
        if (-1 == (currSd = accept(sd, (struct sockaddr *) &accept_sin, &accept_sin_length)))
            perror_exit("ERROR: The socket could not accept the connection", 1);
        printf("SERVER: Connection accepted from %s\n", inet_ntoa(accept_sin.sin_addr));
        request_handler(currSd);
    }

    //never reached, always listening
    printf("SERVER: hello world\n");
    return 0; 

}

/******************************************************************************/
