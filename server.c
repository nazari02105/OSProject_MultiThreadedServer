#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>
#include<time.h>

#define QUEUE_SIZE 5

int main( int argc, char *argv[] ) {
    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int  n;
    time_t t,time1,time2;
    time(&t);
    /* Initialize socket structure */
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 5001;

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

    // start listening for the clients,
    listen(sockfd, QUEUE_SIZE);

    while (1) {

        // accept actual connection from the client
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        printf("\nA client accepted at time (date and time): %s", ctime(&t));


        // inside this while loop, implemented communication with read/write or send/recv function
        printf("Server start");

        bzero(buffer,256);

        //reading time:
        time(time1);
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

        //time resut is ready :
        // time(time2);
        n = write(newsockfd, buffer, strlen(buffer));

        // printf("Difference time from recienve and result ready is %.2f seconds",
        //    difftime(time2, time1));
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
