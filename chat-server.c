/*
 * echo-server.c
 */

#include <stdlib.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "defs.h"

#define BACKLOG 10

#define INITIAL_THREAD_MAX 16

struct thread_info {
    int conn_fd;
    int new_message_flag;
    int in_use_flag;
    pthread_t thread;
};

static size_t thread_max = INITIAL_THREAD_MAX;
static size_t thread_count = 0;
static struct thread_info *thread_info_arr = NULL;
static struct message *new_message = NULL;

void init_thread_info_arr() {
    munmap(thread_info_arr, sizeof(struct thread_info) * thread_max);
    thread_info_arr = mmap(NULL, sizeof(struct thread_info) * thread_max, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
}

void grow_thread_info_arr() {
    thread_max *= 2;
    init_thread_info_arr();
}

struct thread_info *find_empty_thread_info() {
    for (int i = 0; i < thread_max; i++) {
        if (!thread_info_arr[i].in_use_flag) {
            return &thread_info_arr[i];
        }
    }
    return NULL;
}

void *handle_client(void *arg) {
    struct thread_info *t = (struct thread_info *)arg;
    char buf[BUF_SIZE];
    int bytes_received;
    while((bytes_received = recv(t->conn_fd, buf, BUF_SIZE, 0)) > 0) {
        printf(".");
        fflush(stdout);
        send(t->conn_fd, buf, bytes_received, 0);
    }
    printf("\n");
    close(t->conn_fd);
    return NULL;
}

int main(int argc, char *argv[])
{
    char *listen_port;
    int listen_fd, conn_fd;
    struct addrinfo hints, *res;
    int rc;
    struct sockaddr_in remote_sa;
    uint16_t remote_port;
    socklen_t addrlen;
    char *remote_ip;

    listen_port = argv[1];

    /* initialize shared memory */
    init_thread_info_arr();
    new_message = (struct message *)mmap(NULL, sizeof(struct message), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);

    /* create a socket */
    listen_fd = socket(PF_INET, SOCK_STREAM, 0);

    /* bind it to a port */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if((rc = getaddrinfo(NULL, listen_port, &hints, &res)) != 0) {
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    bind(listen_fd, res->ai_addr, res->ai_addrlen);

    /* start listening */
    listen(listen_fd, BACKLOG);

    /* infinite loop of accepting new connections and handling them */
    while(1) {
        /* accept a new connection (will block until one appears) */
        addrlen = sizeof(remote_sa);
        conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen);

        /* announce our communication partner */
        remote_ip = inet_ntoa(remote_sa.sin_addr);
        remote_port = ntohs(remote_sa.sin_port);
        printf("new connection from %s:%d\n", remote_ip, remote_port);

        /* make sure we have enough space in thread arr */
        if (thread_count >= thread_max) {
            grow_thread_info_arr();
        }

        /* create a new thread_info struct */
        struct thread_info *t = find_empty_thread_info();
        memset(t, 0, sizeof(struct thread_info));
        t->conn_fd = conn_fd;
        t->new_message_flag = 0;
        t->in_use_flag = 1;
        thread_count++;
        pthread_create(&t->thread, NULL, handle_client, t);
    }

    /* join threads */
    for (int i = 0; i < thread_max; i++) {
        if (!thread_info_arr[i].in_use_flag) {
            pthread_join(thread_info_arr[i].thread, NULL);
        }
    }
}


