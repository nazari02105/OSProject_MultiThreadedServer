#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <semaphore.h>

pthread_barrier_t threads_index;
int server_port_generated = -1;
sem_t mutex;
time_t send_request_time;

void *thread_function(void *main_array) {
    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = 0;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    clilen = sizeof(cli_addr);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("ERROR on binding\n");
        exit(1);
    }
    socklen_t len = sizeof(serv_addr);
    if (getsockname(sockfd, (struct sockaddr *) &serv_addr, &len) != -1) {
        int random_port = ntohs(serv_addr.sin_port);
        server_port_generated = random_port;
        sem_post(&mutex);
    }
    while (1){
        listen(sockfd, 5);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        printf("server socket started\n");
        bzero(buffer, 256);
        n = read(newsockfd, buffer, 255);
        if (n < 0) {
            perror("ERROR in reading from socket");
            exit(1);
        }
        printf("request was answered in %f seconds\n", difftime(time(NULL), send_request_time));
        printf("the result is: %s \n", buffer);
    }
}

pthread_t all_threads[1];

int main() {
    sem_init(&mutex, 0, 1);
    sem_wait(&mutex);
    // create threads
    pthread_barrier_init(&threads_index, NULL, 1);
    for (int i = 0; i < 1; i++) {
        int (*to_pass) = malloc(4);
        to_pass[0] = i;
        pthread_create(&all_threads[i], NULL, thread_function, (void *) to_pass);
    }
    // main part for client socket
    while (1) {
        int sockfd, portno, n;
        struct sockaddr_in serv_addr;
        struct hostent *server;
        char buffer[256];
        portno = 5002;

        printf("Enter you sentence: ");
        bzero(buffer, 256);
        char math_statement[256];
        gets(math_statement);
        sem_wait(&mutex);
        sprintf(buffer, "127.0.0.1 - %d - %s", server_port_generated, math_statement);
        sem_post(&mutex);

        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        server = gethostbyname("127.0.0.1");
        if (server == NULL) {
            fprintf(stderr, "ERROR, no such host\n");
            exit(0);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *) server->h_addr, (char *) &serv_addr.sin_addr.s_addr, server->h_length);
        serv_addr.sin_port = htons(portno);
        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR while connecting");
            exit(1);
        }
        send_request_time = time(NULL);
        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0) {
            perror("ERROR while writing to socket");
            exit(1);
        }
    }
    return 0;
}
