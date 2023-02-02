#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>
#include<sys/types.h>
#include <string.h>
#include<unistd.h>

int main(int argc, char *argv[]) {
    char buffer[256];
    // inside this while loop, implement communicating with read/write or send/recv function
    while (1) {
        printf("What Operation do you want to Ask or -1 to exit? seperate by space (a,+,b)\n");
        bzero(buffer,256);
        scanf("%s", buffer);

        if (!bcmp(buffer, "quit", 4))
            break;
        else{
            int fd = fork();
            if(fd <0){
                printf("Error creating process");
                return 1;
            }
            else if(fd == 0){
                int sockfd, portno, n;
                struct sockaddr_in serv_addr;

                struct hostent *server;

                portno = 5002;

                // create socket and get file descriptor
                sockfd = socket(AF_INET, SOCK_STREAM, 0);

                server = gethostbyname("127.0.0.1");

                if (server == NULL) {
                    fprintf(stderr,"ERROR, no such host\n");
                    exit(0);
                }

                bzero((char *) &serv_addr, sizeof(serv_addr));
                serv_addr.sin_family = AF_INET;
                bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
                serv_addr.sin_port = htons(portno);
                printf("child - sending request\n");
                // connect to server with server address which is set above (serv_addr)
                if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                    perror("ERROR while connecting\n");
                    perror("Try reconnect after 5 seconds\n");
                    exit(1);
                }
                n = write(sockfd,buffer,strlen(buffer));

                if (n < 0){
                    perror("ERROR while writing to socket");
                    exit(1);
                }

                bzero(buffer,256);
                n = read(sockfd, buffer, 255);

                if (n < 0){
                    perror("ERROR while reading from socket");
                    exit(1);
                }
                printf("server replied: %s \n", buffer);

                
                // just first time send a request
                if (shutdown(sockfd,SHUT_WR) < 0) {
                    perror("ERROR while Shutting down");
                    exit(1);
                } 
                return 0;     
            }
            else if(fd>0){
                // wait until child done with its job
                int status = 0;
                pid_t wpid;
                while ((wpid = wait(&status)) > 0);
                printf("First task done!\n");
            }
        }
       

    }
    return 0;
}
