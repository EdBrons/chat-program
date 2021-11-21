/*
 * echo-client.c
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>

#include "defs.h"

#define BUF_SIZE 4096

struct shared_info {
    int conn_fd;
};

void *handle_client(void *arg) {
    struct shared_info *s = arg;
    char buf[BUF_SIZE];
    char *message = malloc(sizeof(struct message));
    memset(buf, 0, BUF_SIZE);
    memset(message, 0, sizeof(struct message));
    while (fgets(buf, BUF_SIZE, stdin) != NULL) {
        strncpy(((struct message *)message)->text, buf, sizeof(struct message));
        ((struct message *)message)->mode = CHAT;
        send(s->conn_fd, message, sizeof(struct message), 0);
        memset(buf, 0, BUF_SIZE);
        memset(message, 0, sizeof(struct message));
    }
    return NULL;
}

// TODO: kill this process after handle_client ends
void *listen_server(void *arg) {
    struct shared_info *s = arg;
    char buf[BUF_SIZE];
    while (1) {
        recv(s->conn_fd, buf, BUF_SIZE, 0);
        struct message m;
        memcpy(&m, buf, TEXT_LEN);
        puts(m.text);
    }
    return NULL;
}

int main(int argc, char *argv[])
{
    char *dest_hostname, *dest_port;
    struct addrinfo hints, *res;
    int conn_fd;
    int rc;

    dest_hostname = argv[1];
    dest_port     = argv[2];

    /* create a socket */
    conn_fd = socket(PF_INET, SOCK_STREAM, 0);

    /* client usually doesn't bind, which lets kernel pick a port number */

    /* but we do need to find the IP address of the server */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if((rc = getaddrinfo(dest_hostname, dest_port, &hints, &res)) != 0) {
        printf("getaddrinfo failed: %s\n", gai_strerror(rc));
        exit(1);
    }

    /* connect to the server */
    if(connect(conn_fd, res->ai_addr, res->ai_addrlen) < 0) {
        perror("connect");
        exit(2);
    }

    printf("Connected\n");

    struct shared_info s;
    s.conn_fd = conn_fd;

    pthread_t client_thread;
    pthread_create(&client_thread, NULL, handle_client, &s);

    pthread_t listen_thread;
    pthread_create(&listen_thread, NULL, listen_server, &s);

    pthread_join(client_thread, NULL);
    pthread_join(listen_thread, NULL);

    close(conn_fd);
}


