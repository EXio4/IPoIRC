#ifndef IPOIRC_IRC_H
#define IPOIRC_IRC_H
#include "ipoirc.h"
#include "uthash.h"

typedef struct dbuf {
    char             nick[128];
    char            *dt;
    UT_hash_handle   hh;
} dbuf;

typedef struct irc_ctx_t {
    char                *channel;
    char                *nick;
    irc_thread_data     *self;
    dbuf                *ds;
    void                *data;
} irc_ctx_t;


void* irc_thread(void* data);
#endif
