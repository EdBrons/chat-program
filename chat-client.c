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

/* reads a message into m from stdin 
 * sets the contents of m to 0, before writing */
int read_message_from_stdin(struct message *m) {
    char buf[BUF_SIZE] = { 0 };
    m->type = MESSAGE_CHAT;

    /* check for ctrl-D */
    if (fgets(buf, BUF_SIZE, stdin) == NULL) {
        return -1;
    }

    /* remove newlines from buf */
    buf[strcspn(buf, "\n")] = '\0';

    /* check if input is a command */
    if (strncmp(buf, "/nick ", 6) == 0 && buf[6] != '\0') {
        m->type = MESSAGE_NICK;
        strncpy(message_get_sender(m), buf+6, NAME_LEN);
    /* otherwise input is just a message */
    } else {
        m->buf[0] = '\0';
        strncpy(message_get_body(m), buf, BODY_LEN);
    }

    return 1;
}

/* we parse the input in read_message, and then send the
 * struct directly over the socket */
void *handle_io(void *arg) {
    int conn_fd = *((int *)arg);
    struct message m;

    while (read_message_from_stdin(&m) > 0) {
        if (send(conn_fd, (char *)(&m), message_get_size(&m), 0) == -1) {
            perror("send");
        }
    }
    printf("Disconnected from server.\n");
    exit(0);

    return NULL;
}

/* we read the bytes from the socket directly into a message struct
 * and then handle it from there */
void *handle_conn(void *arg) {
    int conn_fd = *((int *)arg);
    struct message m;

    while (recv(conn_fd, (char *)(&m), sizeof(struct message), 0) > 0) {
        message_print(&m);
    }
    printf("Connection closed by remote host.\n");
    exit(0);

    return NULL;
}


int main(int argc, char *argv[])
{
    char *dest_hostname, *dest_port;
    struct addrinfo hints, *res;
    int conn_fd;
    int rc;
    int err;

    pthread_t io_thread;
    pthread_t conn_thread;

    dest_hostname = argv[1];
    dest_port     = argv[2];

    /* create a socket */
    if ((conn_fd = socket(PF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
    }

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

    err = pthread_create(&io_thread, NULL, handle_io, &conn_fd);
    if (err) {
        fprintf(stderr, "pthread_create: (%d)%s\n", err, strerror(err));
        exit(EXIT_FAILURE);
    }

    err = pthread_create(&conn_thread, NULL, handle_conn, &conn_fd);
    if (err) {
        fprintf(stderr, "pthread_create: (%d)%s\n", err, strerror(err));
        exit(EXIT_FAILURE);
    }

    err = pthread_join(io_thread, NULL);
    if (err) {
        fprintf(stderr, "pthread_join: (%d)%s\n", err, strerror(err));
        exit(EXIT_FAILURE);
    }

    err = pthread_cancel(conn_thread);
    if (err) {
        fprintf(stderr, "pthread_cancel: (%d)%s\n", err, strerror(err));
        exit(EXIT_FAILURE);
    }

    err = pthread_join(conn_thread, NULL);
    if (err) {
        fprintf(stderr, "pthread_join: (%d)%s\n", err, strerror(err));
        exit(EXIT_FAILURE);
    }

    close(conn_fd);

    return 0;
}
