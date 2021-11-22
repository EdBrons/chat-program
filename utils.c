#include "defs.h"
#include <string.h>
#include <stdio.h>

int read_message(struct message *m) {
    char buf[BUF_SIZE] = { 0 };
    if (fgets(buf, BUF_SIZE, stdin) == NULL) {
        return -1;
    }
    /* uncomment this to remove newlines from buf */
    buf[strcspn(buf, "\n")] = '\0';
    // strncpy(m->sender, "", NAME_LEN);
    memset(m->sender, 0, NAME_LEN);
    strncpy(m->body, buf, BODY_LEN);
    return 1;
}
