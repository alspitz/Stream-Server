/****************************************************************
 * A TCP server that streams data to clients using sockets.
 * Each client connection evokes a child thread that sends the 
 * data from a global circular buffer, which is written to by a 
 * seperate thread. This thread (currently "record") can be adapted
 * to write any data that can be represented as a string of bytes.
 *
 * See Beej's Guide to Network Programming for a great tutorial
 *
 * -- Alex Spitzer 2013
 *
 ****************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.h"
#include "capture.h"

#define PORT "1234"

int sockfd;
struct thread_data *td;

void interrupt_handler(int signum)
{
    printf("Quitting server...\n");

    /* Clean up */
    close(sockfd);
    free(td->DATA);
    free(td->writePtr);
    free(td);
    exit(signum);
}

void *send_data(void *param)  // interprets void *param as a socket file descriptor
{
    int sockfd = *(int *) param;
    int readPtr = *td->writePtr;
    int rv;

    unsigned long bytes_to_send = td->CHUNK_SIZE * sizeof(td->DATA[0]);
    while (1) {
        pthread_mutex_lock(&td->mutex);
        while (readPtr == *td->writePtr) {  // wait for buffer to advance
            pthread_cond_wait(&td->cond, &td->mutex);
        }
        pthread_mutex_unlock(&td->mutex);
        if ((rv = send(sockfd, td->DATA + readPtr, bytes_to_send, MSG_NOSIGNAL)) == -1) {
            perror("send");
            close(sockfd);
            return NULL;
        }
        if (rv != bytes_to_send) {
            fprintf(stderr, "Error: only sent %d out of %lu bytes", rv, bytes_to_send);
        }
        readPtr = (readPtr + td->CHUNK_SIZE) % td->BUFFER_SIZE;
    }
}



int main(int argc, char *argv[])
{
    const int CHUNK_SIZE = 128; 
    const int BUFFER_SIZE = CHUNK_SIZE * 16;

    int new_fd;
    struct addrinfo hints, *servinfo;
    struct sockaddr_storage their_addr;
    socklen_t sin_size = sizeof their_addr;
    int yes = 1;
    char s[INET_ADDRSTRLEN];
    int rv; // return value
    
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    // Fill the servinfo struct
    if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // Open the socket
    if ((sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol)) == -1) {
        perror("server: socket");
        return 1;
    }

    // Avoid address in use errors
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        return 1;
    }
   
    // Bind the socket 
    if (bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen) == -1) {
        close(sockfd);
        perror("server: bind");
        return 1;
    }

    freeaddrinfo(servinfo); // Done with this struct

    if (listen(sockfd, 5) == -1) {  // Up to 5 connections
        perror("listen");
        return 1;
    }

    printf("Server started on port %s.\n", PORT);

    signal(SIGINT, interrupt_handler);  // Register the interrupt handler

    // Start the recording thread
    td = malloc(sizeof(struct thread_data));
    td->DATA = malloc(BUFFER_SIZE * sizeof(short));
    td->writePtr = malloc(sizeof(int));
    *td->writePtr = 0;
    td->CHUNK_SIZE = CHUNK_SIZE;
    td->BUFFER_SIZE = BUFFER_SIZE;

    pthread_mutex_init(&td->mutex, NULL);
    pthread_cond_init(&td->cond, NULL);

    pthread_t record_thread;
    pthread_create(&record_thread, NULL, record, td);

    while (1) {
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);

        inet_ntop(their_addr.ss_family, &(((struct sockaddr_in*)&their_addr)->sin_addr), s, sizeof s);
        printf("Connection from %s.\n", s);

        pthread_t serve_thread;
        pthread_create(&serve_thread, NULL, send_data, &new_fd);
    }

    return 0; // who comes here?
}
