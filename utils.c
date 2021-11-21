#include "defs.h"
#include <string.h>

void parse_line(char *from, struct message *to) {
    memset(to, 0, sizeof(struct message));
    if (strncmp(from, "/nick ", 6) == 0 && from[6] != '\0') {
        to->mode = NICK;
        strncpy(to->from, from+6, NAME_LEN);
    } else {
        to->mode = CHAT;
        strncpy(to->body, from, BODY_LEN);
    }
}
