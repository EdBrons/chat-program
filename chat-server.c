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
#include <errno.h>
#include "defs.h"

#define BACKLOG 10

#define INITIAL_THREAD_MAX 16

struct thread_info {
    int conn_fd;
    int in_use_flag;
    char client_name[NAME_LEN];
    pthread_t thread;
};

static size_t thread_max = INITIAL_THREAD_MAX;
static size_t thread_count = 0;
static struct thread_info *thread_info_arr = NULL;

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

/* sends message directly to client */
ssize_t send_message_to_client(struct thread_info *t, struct message *m) {
    if (LOG) printf("LOG: sent a message to %s.\n", t->client_name);
    return send(t->conn_fd, (char *)m, sizeof(struct message), 0);
}

/* reads bytes from the socket into the message struct */
ssize_t read_message_from_client(struct thread_info *t, struct message *m) {
    if (LOG) printf("LOG: received a message from %s.\n\t%s\n", t->client_name, m->body);
    return recv(t->conn_fd, (char *)m, sizeof(struct message), 0);
}

void share_message(struct thread_info *t, struct message *m) {
    for (int i = 0; i < thread_max; i++) {
        if (thread_info_arr[i].in_use_flag && t != &thread_info_arr[i]) {
            send_message_to_client(&thread_info_arr[i], m);
        }
    }
}

void create_nick_message(struct thread_info *t, struct message *m, char *name) {
    strncpy(m->sender, "Server", NAME_LEN);
    snprintf(m->body, BODY_LEN, "%s has changed their name to %s.", t->client_name, name);
}

void create_connect_message(struct thread_info *t, struct message *m) {
    strncpy(m->sender, "Server", NAME_LEN);
    snprintf(m->body, BODY_LEN, "%s connected.", t->client_name);
}

void create_disconnect_message(struct thread_info *t, struct message *m) {
    strncpy(m->sender, "Server", NAME_LEN);
    snprintf(m->body, BODY_LEN, "%s disconnected.", t->client_name);
}


void *handle_client(void *arg) {
    struct thread_info *t = (struct thread_info *)arg;
    struct message m;
    memset(&m, 0, sizeof(struct message));
    while(read_message_from_client(t, &m) > 0) {
        if (m.sender[0] != '\0' && m.body[0] == '\0') {
            char new_name[NAME_LEN];
            strncpy(new_name, m.sender, NAME_LEN);
            create_nick_message(t, &m, new_name);
            share_message(t, &m);
            memcpy(t->client_name, new_name, NAME_LEN);
        } else {
            memcpy(m.sender, t->client_name, NAME_LEN);
            share_message(t, &m);
        }
    }
    if (LOG) printf("LOG: client %s disconnected.\n", t->client_name);
    create_disconnect_message(t, &m);
    share_message(t, &m);
    close(t->conn_fd);
    memset(t, 0, sizeof(struct thread_info));
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
        if (LOG ) printf("LOG: new connection from %s:%d\n", remote_ip, remote_port);

        /* make sure we have enough space in thread arr */
        if (thread_count >= thread_max) {
            grow_thread_info_arr();
        }

        /* create a new thread_info struct */
        struct thread_info *t = find_empty_thread_info();
        memset(t, 0, sizeof(struct thread_info));
        t->conn_fd = conn_fd;
        t->in_use_flag = 1;
        snprintf(t->client_name, NAME_LEN, "user_%ld", thread_count);
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


