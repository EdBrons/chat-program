#ifndef __DEFS_H
#define __DEFS_H

#define CHAT 0
#define NICK 1
#define DISC 2

#define TEXT_LEN 1024

struct message {
    char mode;
    char text[TEXT_LEN];
};

#endif
