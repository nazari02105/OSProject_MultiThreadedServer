#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define QUEUE_SIZE 5
#define MAX_THREADS 10
#define IDLE 0
#define RESERVED 1
#define RUNNING 2

struct thread_data {
    int thread_id;
    char buffer[256];
    int n;
    int newsockfd;
};
typedef struct thread_data thread_data_t;

// semaphore for thread pool
sem_t thread_pool_sem;
// mutex for each thread in thread pool
pthread_mutex_t thread_pool_mutex[MAX_THREADS];

// thread pool data and ids
pthread_t worker_thread_id[MAX_THREADS];
int thread_status[MAX_THREADS];
thread_data_t *data[MAX_THREADS];

void *worker_thread(void *data) {
    thread_data_t *thread_data = (thread_data_t *)data;
    int thread_id = thread_data->thread_id;
    char buffer[256];
    int n;
    int newsockfd;

    while (1) {
        // new client

        // extract data of new client
        strcpy(buffer, thread_data->buffer);
        n = thread_data->n;
        newsockfd = thread_data->newsockfd;

        // inside this while loop, implemented communication with read/write or send/recv function
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

        // do work
        printf("Thread %d is doing work", thread_id);
        // unlock mutex and increment semaphore (thread is free)
        pthread_mutex_unlock(&thread_pool_mutex[thread_id]);
        sem_post(&thread_pool_sem);
    }

    free(data);
    pthread_exit(NULL);
}

int main( int argc, char *argv[] ) {
    // initialize semaphore
    sem_init(&thread_pool_sem, 0, MAX_THREADS);
    for (int i = 0; i < MAX_THREADS; i++) {
        pthread_mutex_init(&thread_pool_mutex[i], NULL);
    }

    // create socket
    int sockfd, newsockfd, portno, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    /* initialize socket structure */
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

    // create thread pool
    for (int i = 0; i < MAX_THREADS; i++) {
        data[i] = malloc(sizeof(thread_data_t));
        data[i]->thread_id = i;
        pthread_create(&worker_thread_id[i], NULL, worker_thread, (void *)data[i]);
    }

    // start listening for the clients,
    if (listen(sockfd, QUEUE_SIZE) != 0){
        perror("ERROR on listen\n");
        exit(1);
    }
    printf("Server is listening...");

    // accept actual connection from the clients
    while (1) {
        // accept connection of the first client in the queue
        if (accept(sockfd, (struct sockaddr *)&cli_addr, &clilen) != 0){
            perror("ERROR on accept\n");
            continue;
        }

        // waiting until a thread is available
        sem_wait(&thread_pool_sem);
        int first_free_thread = -1;
        int i = 0;
        while (1) {
            // lock the mutex of the first free thread
            if (pthread_mutex_trylock(&thread_pool_mutex[i])) {
                first_free_thread = i;
                strcpy(data[i]->buffer, buffer);
                break;
            }
            i++;
            i = i % MAX_THREADS;
        }


    }

    return 0;
}
