#ifndef __DEFS_H
#define __DEFS_H

#define CHAT 0
#define NICK 1
#define DISC 2

#define NAME_LEN 32
#define BODY_LEN 1024

struct thread_args {
    int conn_fd;
};

struct message {
    int mode;
    char from[NAME_LEN];
    char body[BODY_LEN];
};

void parse_line(char *from, struct message *to);

#endif
