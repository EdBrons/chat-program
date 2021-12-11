#include "defs.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

void print_message(struct message *m) {
    char time_str[BUF_SIZE] = { 0 };
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(time_str, BUF_SIZE, "%H:%M:%S", tm_info);
    printf("%s: %s: %s\n", time_str, m->sender, m->body);
}
