/*
 * Author: Lorenzo Cappellotto
 */

/******************************************************************************/
// Library includes

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/******************************************************************************/

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
// Main function


int main(int argc, char **argv)
{
    int sd;    
    int port = 50000;
    char* hostname = "localhost"; 
    struct hostent* hp = gethostbyname(hostname);
    struct sockaddr_in sin;
    socklen_t sin_length = sizeof(sin);
    
    if (0 == hp)
        perror_exit("ERROR: The system could not translate the hostname", 1);

    memset(&sin, 0, sin_length);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr_list[0]))->s_addr;
    sin.sin_port = htons(port);

    if (-1 == (sd = socket(AF_INET, SOCK_DGRAM, 0)))
        perror_exit("ERROR: The system could not create the socket\n", 1);


	char buffer[MAXLINE];
	char *hello = "Hello from client";

	int n;

	sendto(sd, (const char *)hello, strlen(hello), MSG_CONFIRM, 
            (const struct sockaddr *) &sin, sin_length);
	printf("Hello message sent.\n");
		
	n = recvfrom(sd, (char *)buffer, MAXLINE, MSG_WAITALL, 
            (struct sockaddr *) &sin, &sin_length);
	buffer[n] = '\0';
	printf("Server : %s\n", buffer);

    printf("Connection Terminated\n");

    close(sd);
    return 0; 
}

/******************************************************************************/
