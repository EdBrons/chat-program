#ifndef __DEFS_H
#define __DEFS_H

#include <stddef.h>

#define NAME_LEN 64
#define BODY_LEN 1024
#define BUF_SIZE 4096
#define MESSAGE_MAX 1024

#define MESSAGE_CHAT 1
#define MESSAGE_NICK 2

struct message {
    char type;
    char buf[MESSAGE_MAX];
};

/* prints a message with the current time prefixing the message */
void message_print(struct message *m);
char *message_get_sender(struct message *m);
char *message_get_body(struct message *m);
size_t message_get_size(struct message *m);

#endif
