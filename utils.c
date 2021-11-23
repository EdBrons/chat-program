#include "defs.h"
#include <string.h>
#include <stdio.h>

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
