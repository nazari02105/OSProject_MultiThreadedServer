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


#define QUEUE_SIZE 20
#define MAX_THREADS 3
#define IDLE 0
#define BUSY 1

struct thread_data {
    int thread_id;
    int newsockfd;
    int initial_port;
    sem_t is_data_ready;
    int status;
    time_t initial_connection;
};
typedef struct thread_data thread_data_t;

// semaphore for thread pool
sem_t thread_pool_sem;

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
        // extract time of initial connection
        time_t initial_connection = thread_data->initial_connection;

        // read from client
        bzero(buffer,256);
        n = read(newsockfd, buffer, 255);

        if (n < 0){
            perror("ERROR in reading from socket\n");
            exit(1);
        }

        int a, b, portno, ip1, ip2, ip3, ip4;
        char operator;
        sscanf(buffer,"%d.%d.%d.%d - %d - %d %c %d", &ip1, &ip2, &ip3, &ip4, &portno, &a, &operator, &b);
        // print the client's request
        printf("client %d.%d.%d.%d:%d said: whats the result of %d %c %d?\n", ip1, ip2, ip3, ip4, thread_data->initial_port, a, operator, b);
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
        sleep(20);
        // store the result of calculation in buffer
        bzero(buffer,256);
        sprintf(buffer,"%d", result);

        int sockfd;
        struct sockaddr_in serv_addr;

        // create socket
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        // ip of client waiting for response
        char * ip_str = malloc(16);
        sprintf(ip_str, "%d.%d.%d.%d", ip1, ip2, ip3, ip4);
        in_addr_t ip = inet_addr(ip_str);

        // data of server address
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);

        if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR while connecting\n");
            exit(1);
        }

        // time of writing to socket
        time_t time_of_writing = time(NULL);
        double time_of_response = difftime(time_of_writing, initial_connection);
        printf("client %s:%d -> took %f seconds for server to respond\n", ip_str, thread_data->initial_port, time_of_response);

        n = write(sockfd, buffer, strlen(buffer));
        if (n < 0){
            perror("ERROR in writing to socket\n");
            exit(1);
        }

        // set status and increment semaphore (thread is free)
        thread_data->status = IDLE;
        sem_post(&thread_pool_sem);
    }
}

int main( int argc, char *argv[] ) {
    // initialize semaphore
    sem_init(&thread_pool_sem, 0, MAX_THREADS);

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
    portno = 5002;
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
        data[i]->status = IDLE;
        sem_init(&data[i]->is_data_ready, 0, 0);
        pthread_create(&worker_thread_id[i], NULL, worker_thread, (void *)data[i]);
    }

    // start listening for the clients,
    if (listen(sockfd, QUEUE_SIZE) != 0){
        perror("ERROR on listen\n");
        exit(1);
    }
    printf("Server is listening...\n");

    // accept actual connection from the clients
    while (1) {
        // accept connection of the first client in the queue
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0){
            perror("ERROR on accept\n");
            continue;
        }

        // initial port number of the client
        int initial_portno = ntohs(cli_addr.sin_port);

        // store time of connection
        time_t initial_connection = time(NULL);
        char *initial_connection_str = malloc(26);
        strftime(initial_connection_str, 26, "%Y-%m-%d %H:%M:%S", localtime(&initial_connection));
        printf("client %s:%d -> connected to the server at %s\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), initial_connection_str);

        // waiting until a thread is available
        sem_wait(&thread_pool_sem);
        // find the available thread...
        for (int i = 0; i < MAX_THREADS; i++) {
            // lock the mutex of the first free thread
            if (data[i]->status == IDLE) {
                // store time of thread assignment
                time_t thread_assignment = time(NULL);
                // calculate time difference
                double time_difference = difftime(thread_assignment, initial_connection);
                // print "client <ip>:<port> -> took <diff_time> for server to assign a thread"
                printf("client %s:%d -> took %f seconds for server to assign a thread\n", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port), time_difference);

                // allocate data to the thread
                data[i]->initial_port = initial_portno;
                data[i]->newsockfd = newsockfd;
                data[i]->initial_connection = initial_connection;
                data[i]->status = BUSY;
                // signal the thread that data is ready
                sem_post(&data[i]->is_data_ready);
                break; // this client is handled; break and serve the next client...
            }
        }
    }

    return 0;
}
