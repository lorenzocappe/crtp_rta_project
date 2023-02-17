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

/******************************************************************************/

/******************************************************************************/
// Helper functions and Static functions

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

/******************************************************************************/
// Main function

int main(int argc, char **argv)
{
    char* hostname = "localhost"; 
    int port = 50000;
    int sd;    
    struct sockaddr_in sin;
    struct hostent* hp = gethostbyname(hostname);
    
    if (0 == hp)
        perror_exit("ERROR: The system could not translate the hostname", 1);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = ((struct in_addr*)(hp->h_addr_list[0]))->s_addr;
    sin.sin_port = htons(port);

    if (-1 == (sd = socket(AF_INET, SOCK_STREAM, 0)))
        perror_exit("ERROR: The system could not create the socket\n", 1);

    if (-1 == connect(sd, (struct sockaddr*)&sin, sizeof(sin)))
        perror_exit("ERROR: Impossible to connect to the server's socket", 1);

    char command[256];
    char *answer;
    //int stopped = FALSE;
    int command_length;
    unsigned int network_command_length;

    while (1) {   
        printf("Enter command (start/stop <task_name>, quit, help): ");
        
        //scanf("%s", command);
        //scanf("%255[^\n]", command);
        //fgets();

        fgets(command, 256, stdin);
        command_length = strlen(command);
        if ((command_length > 0) && (command[command_length-1] == '\n'))
            command[command_length-1] = '\0';

        network_command_length = htonl(command_length);
        printf("command: %s, (length %d)\n", command, command_length);

        if (-1 == send(sd, &network_command_length, sizeof(network_command_length), 0))
            perror_exit("ERROR: Impossible to send the length", 1);
            
        if (-1 == send(sd, command, command_length, 0))
            perror_exit("ERROR: Impossible to send the command", 1);

        if (-1 == receive(sd, (char *)&network_command_length, sizeof(network_command_length)))
            perror_exit("recv", 1);
        command_length = ntohl(network_command_length);

        answer = malloc(command_length + 1);
        if (-1 == receive(sd, answer, command_length)) 
            perror_exit("recv", 1);
        answer[command_length] = 0;
        printf("%s\n", answer);

        free(answer);
        
        if(0 == strcmp(command, "quit"))
            break;
    }
        
    printf("Connection Terminated\n");

    close(sd);
    return 0; 
}

/******************************************************************************/
