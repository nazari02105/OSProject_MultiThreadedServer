#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

#define QUEUE_SIZE 5

int main( int argc, char *argv[] ) {
    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int  n;

    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5002;

    // create socket and get file descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    clilen = sizeof(cli_addr);

    // bind the host address using bind() call
    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0){
        perror("ERROR on binding\n");
        exit(1);
    }


    while (1) {
        // start listening for the clients,
        listen(sockfd, QUEUE_SIZE);

        // accept actual connection from the client
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);

        // inside this while loop, implemented communication with read/write or send/recv function
        printf("Server start");

        bzero(buffer,256);
        n = read(newsockfd, buffer, 255);

        if (n < 0){
            perror("ERROR in reading from socket");
            exit(1);
        }

        printf("client said: whats the result of %s? \n", buffer);

        char * pch;
        pch = strtok (buffer,",");
        int a = atoi(pch);
        pch = strtok (NULL,",");
        char operator = pch[0];
        pch = strtok (NULL,",");
        int b = atoi(pch);
        int result = -1;
        switch (operator)
        {
        case '+':
            result = a+b;
            break;
        case '-':
            result = a-b;
            break;
        case '*':
            result = a*b;
            break;
        case '/':
            if (b!=0)
                result = a/b;
            break;
        case '%':
            result = a%b;
            break;
        default:
            break;
        }

        sprintf(buffer,"result is:%d (-1 means error or result)",result);

        n = write(newsockfd, buffer, strlen(buffer));

        if (n < 0){
            perror("ERROR in writing to socket");
            exit(1);
        }
        // escape this loop, if the client sends message "quit"
        if (!bcmp(buffer, "quit", 4))
            break;
    }

    return 0;
}
