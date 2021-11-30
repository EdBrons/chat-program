#include "defs.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

int read_message_from_stdin(struct message *m) {
    char buf[BUF_SIZE] = { 0 };
    memset(m, 0, sizeof(struct message));
    if (fgets(buf, BUF_SIZE, stdin) == NULL) {
        return -1;
    }
    /* remove newlines from buf */
    buf[strcspn(buf, "\n")] = '\0';
    /* check if input is a command */
    if (strncmp(buf, "/nick ", 6) == 0 && buf[6] != '\0') {
        strncpy(m->sender, buf+6, NAME_LEN);
    /* otherwise input is just a message */
    } else {
        strncpy(m->body, buf, BODY_LEN);
    }
    return 1;
}

void print_message(struct message *m) {
    char time_str[BUF_SIZE] = { 0 };
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(time_str, BUF_SIZE, "%H:%M:%S", tm_info);
    printf("%s: %s: %s\n", time_str, m->sender, m->body);
}
