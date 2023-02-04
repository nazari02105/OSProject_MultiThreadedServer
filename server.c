#include <stdio.h>
#include <stdlib.h>

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <limits.h>
#include <arpa/inet.h>


#define QUEUE_SIZE 5
#define MAX_THREADS 10

struct thread_data {
    int thread_id;
    int newsockfd;
    sem_t is_data_ready;
};
typedef struct thread_data thread_data_t;

// semaphore for thread pool
sem_t thread_pool_sem;
// mutex for each thread in thread pool
pthread_mutex_t thread_pool_mutex[MAX_THREADS];

// thread pool data and ids
pthread_t worker_thread_id[MAX_THREADS];
thread_data_t *data[MAX_THREADS];

void *worker_thread(void *data) {
    thread_data_t *thread_data = (thread_data_t *)data;
    int thread_id = thread_data->thread_id;
    char buffer[256];
    int n;
    int newsockfd;

    while (1) {
        // wait until data of a new client is assigned to this thread
        sem_wait(&thread_data->is_data_ready);
        // new client is assigned to this thread!

        // extract data of new client (socket id)
        newsockfd = thread_data->newsockfd;

        // read from client
        bzero(buffer,256);
        n = read(newsockfd, buffer, 255);

        if (n < 0){
            perror("ERROR in reading from socket");
            exit(1);
        }

        printf("client said: whats the result of %s? \n", buffer);

        int a, b, portno, ip1, ip2, ip3, ip4;
        char operator;
        sscanf(buffer,"%d.%d.%d.%d - %d - %d %c %d", &ip1, &ip2, &ip3, &ip4, &portno, &a, &operator, &b);
        int result;
        switch (operator) {
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
                if (b!=0) {
                    result = a / b;
                } else { // infinity
                    result = INT_MAX;
                }
                break;
            case '%':
                result = a%b;
                break;
            default:
                break;
        }

        // store the result of calculation in buffer
        sprintf(buffer,"result is: %d", result);

        /*
         * The way of communication with remote (in this case, client) is adopted from the link:
         * https://stackoverflow.com/questions/15673846/how-to-give-to-a-client-specific-ip-address-in-c
         * the part 'Connect to remote server' of Adam Rosenfield's answer is adopted
        */
        int sockfd;
        struct sockaddr_in serv_addr;
        char buffer[256];

        // create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        // ip of client waiting for response
        char * ip_str = malloc(16);
        sprintf(ip_str, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
        in_addr_t ip = inet_addr(ip_str);

        // data of server address
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = ip;
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR while connecting");
            exit(1);
        }

        n = write(newsockfd, buffer, strlen(buffer));
        if (n < 0){
            perror("ERROR in writing to socket");
            exit(1);
        }

        // unlock mutex and increment semaphore (thread is free)
        pthread_mutex_unlock(&thread_pool_mutex[thread_id]);
        sem_post(&thread_pool_sem);
    }
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

    // create socket and get file descriptor
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    portno = 5003;
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
        sem_init(&data[i]->is_data_ready, 0, 0);
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
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0){
            perror("ERROR on accept\n");
            continue;
        }

        // waiting until a thread is available
        sem_wait(&thread_pool_sem);
        // find the available thread...
        for (int i = 0; i < MAX_THREADS; i++) {
            // lock the mutex of the first free thread
            if (pthread_mutex_trylock(&thread_pool_mutex[i])) {
                // allocate data to the thread
                data[i]->newsockfd = newsockfd;
                // signal the thread that data is ready
                sem_post(&data[i]->is_data_ready);
                break; // this client is handled; break and serve the next client...
            }
        }
    }

    return 0;
}
