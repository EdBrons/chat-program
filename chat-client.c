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

/* writes bytes from a message directly into a socket */
ssize_t send_message_to_server(int conn_fd, struct message *m) {
    return send(conn_fd, (char *)m, sizeof(struct message), 0);
}

/* reads bytes from a socket directly into a message struct */
ssize_t read_message_from_server(int conn_fd, struct message *m) {
    return recv(conn_fd, (char *)m, sizeof(struct message), 0);
}

/* we parse the input in read_message, and then send the
 * struct directly over the socket */
void *handle_io(void *arg) {
    int conn_fd;
    struct message m;
    memcpy(&conn_fd, arg, sizeof(int));
    memset(&m, 0, sizeof(struct message));
    while (read_message_from_stdin(&m) > 0) {
        if (send_message_to_server(conn_fd, &m) == -1) {
            perror("send");
        }
    }
    printf("Disconnected from server.\n");
    return NULL;
}

/* we read the bytes from the socket directly into a message struct
 * and then handle it from there */
void *handle_conn(void *arg) {
    int conn_fd;
    memcpy(&conn_fd, arg, sizeof(int));
    struct message m;
    memset(&m, 0, sizeof(struct message));
    while (read_message_from_server(conn_fd, &m) > 0) {
        print_message(&m);
        // printf("%s: %s\n", m.sender, m.body);
        fflush(stdout);
    }
    printf("Connection closed by remote host.\n");
    return NULL;
}

int main(int argc, char *argv[])
{
    char *dest_hostname, *dest_port;
    struct addrinfo hints, *res;
    int conn_fd;
    int rc;

    pthread_t io_thread;
    pthread_t conn_thread;

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

    pthread_create(&io_thread, NULL, handle_io, &conn_fd);
    pthread_create(&conn_thread, NULL, handle_conn, &conn_fd);

    pthread_join(io_thread, NULL);
    pthread_cancel(conn_thread);
    pthread_join(conn_thread, NULL);

    close(conn_fd);
}
