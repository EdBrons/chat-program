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
    int conn_fd; /* socket of client */
    char client_name[NAME_LEN];
    char remote_ip[NAME_LEN];
    uint16_t remote_port;
    pthread_t thread;
    struct thread_info *next; /* next thread_info in linked list */
};

static struct thread_info *threads_head = NULL;
static struct thread_info *threads_tail = NULL;

struct thread_info *get_new_thread_info() {
    struct thread_info *t;

    if ((t = malloc(sizeof(struct thread_info))) == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&mutex); /* mutex when accessing linked list */
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
    struct thread_info *tp;
    pthread_mutex_lock(&mutex); /* mutex when accessing linked list */
    tp = threads_head;

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
    pthread_mutex_lock(&mutex); /* mutex when accessing linked list */
    struct thread_info *tp = threads_head;

    while (tp != NULL) {
        if (tp != t) {
            if (send(tp->conn_fd, (char *)m, message_get_size(m), 0) == -1) {
                perror("send");
                continue;
            }
        }
        tp = tp->next;
    }
    pthread_mutex_unlock(&mutex);
}

/* creates a message saying the client has changed their name */
void create_nick_message(struct thread_info *t, struct message *m, char *name) {
    strncpy(message_get_sender(m), "Server", NAME_LEN);
    printf("%s has changed their name to %s.\n", t->client_name, name);
    snprintf(message_get_body(m), BODY_LEN, "%s has changed their name to %s.", t->client_name, name);
}

/* creates a message saying a client has connected with some ip and some port */
void create_connect_message(struct thread_info *t, struct message *m) {
    strncpy(message_get_sender(m), "Server", NAME_LEN);
    snprintf(message_get_body(m), BODY_LEN, "%s connected.", t->client_name);
}

/* creates a message saying the client has disconnected */
void create_disconnect_message(struct thread_info *t, struct message *m) {
    strncpy(message_get_sender(m), "Server", NAME_LEN);
    snprintf(message_get_body(m), BODY_LEN, "%s disconnected.", t->client_name);
}


void *handle_client(void *arg) {
    char buf[BUF_SIZE] = { 0 };
    struct thread_info *t = (struct thread_info *)arg;
    struct message m;

    /* send message to other clients saying we connected */
    create_connect_message(t, &m);
    share_message(t, &m);

    while(recv(t->conn_fd, &m, sizeof(struct message), 0) > 0) {
        switch (m.type) {
            case MESSAGE_NICK: /* we are changing our name */
                strncpy(buf, message_get_sender(&m), NAME_LEN);
                create_nick_message(t, &m, buf);
                share_message(t, &m);
                strncpy(t->client_name, buf, NAME_LEN);
                break;
            case MESSAGE_CHAT: /* we are sending a chat */
                strncpy(buf, message_get_body(&m), BUF_SIZE);
                strncpy(message_get_sender(&m), t->client_name, NAME_LEN);
                strncpy(message_get_body(&m), buf, BODY_LEN);
                share_message(t, &m);
                break;
            default: /* something bad happend */
                break;
        }
    }

    /* log disconnection on server */
    printf("client %s disconnected.\n", t->client_name);
    /* tell other clients */
    create_disconnect_message(t, &m);
    share_message(t, &m);

    if (close(t->conn_fd) == -1) {
        perror("close");
        return NULL;
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
    struct thread_info *tp;

    listen_port = argv[1];

    /* create a socket */
    if ((listen_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        return 1;
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
        return 1;
    }

    /* start listening */
    if (listen(listen_fd, BACKLOG) != 0) {
        perror("listen");
        return 1;
    }

    /* infinite loop of accepting new connections and handling them */
    while(1) {
        /* accept a new connection (will block until one appears) */
        addrlen = sizeof(remote_sa);
        if ((conn_fd = accept(listen_fd, (struct sockaddr *) &remote_sa, &addrlen)) == -1) {
            perror("accept");
            continue;
        }

        /* announce our communication partner */
        remote_ip = inet_ntoa(remote_sa.sin_addr);
        remote_port = ntohs(remote_sa.sin_port);
        printf("New connection from %s:%d\n", remote_ip, remote_port);

        /* create a new thread_info struct */
        tp = get_new_thread_info();

        tp->conn_fd = conn_fd;
        snprintf(tp->client_name, NAME_LEN, "Guest");
        strncpy(tp->remote_ip, remote_ip, NAME_LEN);
        tp->remote_port = remote_port;

        if (pthread_create(&tp->thread, NULL, handle_client, tp) != 0) {
            remove_thread_info(tp);
            perror("pthread_create");
            break;
        }
    }

    /* join threads */
    pthread_mutex_lock(&mutex); /* mutex when accessing linked list */
    tp = threads_head;
    while (tp != NULL) {
        if (pthread_join(tp->thread, NULL) != 0) {
            perror("pthread_join");
            break;
        }
        tp = tp->next;
    }
    pthread_mutex_unlock(&mutex); /* mutex when accessing linked list */
    return 0;
}


