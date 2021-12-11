#ifndef __DEFS_H
#define __DEFS_H

#define LOG 1

#define NAME_LEN 64
#define BODY_LEN 1024
#define BUF_SIZE 4096

struct message {
    char sender[NAME_LEN];
    char body[BODY_LEN];
};

void print_message(struct message *m);

#endif
