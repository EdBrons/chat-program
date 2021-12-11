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

pthread_mutex_t mutex;

struct thread_info {
    int conn_fd;
    int in_use;
    char client_name[NAME_LEN];
    char remote_ip[NAME_LEN];
    uint16_t remote_port;
    pthread_t thread;
    struct thread_info *next;
};

static struct thread_info *threads_head = NULL;
static struct thread_info *threads_tail = NULL;

struct thread_info *get_new_thread_info() {
    struct thread_info *t;
    if ((t = malloc(sizeof(struct thread_info))) == NULL) {
        return NULL;
    }
    pthread_mutex_lock(&mutex);
    if (threads_head == NULL) {
        threads_head = t;
        threads_tail = t;
    } else {
        threads_tail->next = t;
        threads_tail = t;
    }
    t->next = NULL;
    pthread_mutex_unlock(&mutex);
    return t;
}

void remove_thread_info(struct thread_info *t) {
    pthread_mutex_lock(&mutex);
    struct thread_info *tp = threads_head;
    while (tp != NULL) {
        if (tp->next == t) {
            tp->next = t->next;
            if (t == threads_tail) {
                threads_tail = tp;
            }
            free(t);
            break;
        }
        tp = tp->next;
    }
    if (t == threads_head) {
        threads_head = t->next;
    }
    pthread_mutex_unlock(&mutex);
}

void share_message(struct thread_info *t, struct message *m) {
    struct thread_info *tp = threads_head;
    while (tp != NULL) {
        if (tp != t) {
            printf("sending a message to %d.\n", tp->conn_fd);
            if (send(tp->conn_fd, (char *)m, sizeof(struct message), 0) == -1) {
                perror("send");
            }
        }
        tp = tp->next;
    }
}

void create_nick_message(struct thread_info *t, struct message *m, char *name) {
    strncpy(m->sender, "Server", NAME_LEN);
    printf("%s has changed their name to %s.\n", t->client_name, name);
    snprintf(m->body, BODY_LEN, "%s has changed their name to %s.", t->client_name, name);
}

void create_connect_message(struct thread_info *t, struct message *m) {
    strncpy(m->sender, "Server", NAME_LEN);
    snprintf(m->body, BODY_LEN, "%s(%s:%d) connected.", t->client_name, t->remote_ip, t->remote_port);
}

void create_disconnect_message(struct thread_info *t, struct message *m) {
    strncpy(m->sender, "Server", NAME_LEN);
    snprintf(m->body, BODY_LEN, "%s disconnected.", t->client_name);
}


void *handle_client(void *arg) {
    struct thread_info *t = (struct thread_info *)arg;
    struct message m;
    memset(&m, 0, sizeof(struct message));
    create_connect_message(t, &m);
    share_message(t, &m);
    memset(&m, 0, sizeof(struct message));
    while(recv(t->conn_fd, (char *)(&m), sizeof(struct message), 0) > 0) {
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
    printf("client %s disconnected.\n", t->client_name);
    create_disconnect_message(t, &m);
    share_message(t, &m);
    if (close(t->conn_fd) == -1) {
        perror("close");
    }
    remove_thread_info(t);
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
    struct thread_info *t;

    listen_port = argv[1];

    /* create a socket */
    if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
    }

    /* bind it to a port */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    if((rc = getaddrinfo(NULL, listen_port, &hints, &res)) != 0) {
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    if (bind(listen_fd, res->ai_addr, res->ai_addrlen) != 0) {
        perror("bind");
    }

    /* start listening */
    if (listen(listen_fd, BACKLOG) != 0) {
        perror("listen");
    }

    /* infinite loop of accepting new connections and handling them */
    while(1) {
        /* accept a new connection (will block until one appears) */
        addrlen = sizeof(remote_sa);
        if ((conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen)) == -1) {
            perror("accept");
        }

        /* announce our communication partner */
        remote_ip = inet_ntoa(remote_sa.sin_addr);
        remote_port = ntohs(remote_sa.sin_port);
        printf("new connection from %s:%d\n", remote_ip, remote_port);

        /* create a new thread_info struct */
        t = get_new_thread_info();
        t->conn_fd = conn_fd;
        t->in_use = 1;
        snprintf(t->client_name, NAME_LEN, "Guest");
        strncpy(t->remote_ip, remote_ip, NAME_LEN);
        t->remote_port = remote_port;
        if (pthread_create(&t->thread, NULL, handle_client, t) != 0) {
            perror("pthread_create");
            break;
        }
    }

    /* join threads */
    struct thread_info *tp = threads_head;
    while (tp != NULL) {
        if (pthread_join(tp->thread, NULL) != 0) {
            perror("pthread_join");
            break;
        }
        tp = tp->next;
    }
}


