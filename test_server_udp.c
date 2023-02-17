/*
 * AUTHOR: Lorenzo Cappellotto
 *
 * OBJECTIVES: 
 * - how to communicate via UDP
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

/******************************************************************************/
// Static variables and MACRO definitions

#define MAXLINE 1024

/******************************************************************************/

/******************************************************************************/
// Helper functions and Static functions

static inline void perror_exit(char* text, int code) {
    perror(text);
    exit(0);
}

/******************************************************************************/

/******************************************************************************/
// Main function


int main(int argc, char **argv) {
    
    int sd; //, currSd;
    int port = 50000;
    
    struct sockaddr_in listen_sin, accept_sin; 
    socklen_t accept_sin_length = sizeof(accept_sin);

    char buffer[MAXLINE+1];
    char *hello = "Hello from server";
       
    // Creating socket file descriptor
    if ( (sd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 )
        perror_exit("ERROR: The system could not create the socket\n", 1);
       
    memset(&listen_sin, 0, sizeof(listen_sin));
    memset(&accept_sin, 0, accept_sin_length);
    listen_sin.sin_family    = AF_INET; // IPv4
    listen_sin.sin_addr.s_addr = INADDR_ANY;
    listen_sin.sin_port = htons(port);

    // Bind the socket with the server address
    if (-1 == bind(sd, (struct sockaddr *)&listen_sin, sizeof(listen_sin)))
        perror_exit("ERROR: The system could not bind the socket\n", 1);

    int n = recvfrom(sd, (char *)buffer, MAXLINE, MSG_WAITALL, 
            (struct sockaddr *) &accept_sin, &accept_sin_length);
    buffer[n] = '\0';
    printf("Client : %s\n", buffer);
    
    sendto(sd, (const char *)hello, strlen(hello), MSG_CONFIRM, 
            (const struct sockaddr *) &accept_sin, accept_sin_length);
    printf("Hello message sent.\n"); 

    printf("SERVER: hello world\n");
    return 0; 

}

/******************************************************************************/