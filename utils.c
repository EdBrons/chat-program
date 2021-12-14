#include "defs.h"
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <stddef.h>

char *message_get_sender(struct message *m) {
    return m->buf;
}

char *message_get_body(struct message *m) {
    return m->buf + strlen(m->buf) + 1;
}

size_t message_get_size(struct message *m) {
    return sizeof(char) + strlen(message_get_sender(m)) + strlen(message_get_body(m)) + 2;
}

void message_print(struct message *m) {
    char time_str[BUF_SIZE] = { 0 };
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(time_str, BUF_SIZE, "%H:%M:%S", tm_info);

    printf("%s: %s: %s\n", time_str, message_get_sender(m), message_get_body(m));
}
