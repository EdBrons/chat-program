#ifndef __DEFS_H
#define __DEFS_H

#define NAME_LEN 64
#define BODY_LEN 1024
#define BUF_SIZE 4096

struct message {
    char sender[NAME_LEN];
    char body[BODY_LEN];
};

/* reads a message into m from stdin */
int read_message(struct message *m);

#endif
