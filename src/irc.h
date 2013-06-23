#ifndef _IRC_H
#define _IRC_H
#include "ipoirc.h"

typedef struct irc_ctx_t {
    char                *channel;
    char                *nick;
    irc_thread_data     *self;
    char                *buffer;
    void                *data;
} irc_ctx_t;


void* irc_thread(void* data);
#endif
