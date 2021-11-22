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

#define BUF_SIZE 4096

void *handle_io(void *arg) {
    int conn_fd;
    char buf[BUF_SIZE] = { 0 };
    memcpy(&conn_fd, arg, sizeof(int));
    while (fgets(buf, BUF_SIZE, stdin)) {
        /* uncomment this to remove newlines from buf */
        // buf[strcspn(buf, "\n")] = '\0';
        puts(buf);
        memset(buf, 0, BUF_SIZE);
    }
    return NULL;
}

void *handle_conn(void *arg) {
    int conn_fd;
    memcpy(&conn_fd, arg, sizeof(int));
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
    pthread_join(conn_thread, NULL);

    close(conn_fd);

    /* infinite loop of reading from terminal, sending the data, and printing
     * what we get back */
    /*
    while((n = read(0, buf, BUF_SIZE)) > 0) {
        send(conn_fd, buf, n, 0);

        n = recv(conn_fd, buf, BUF_SIZE, 0);
        printf("received: ");
        puts(buf);
    }
    */
}


