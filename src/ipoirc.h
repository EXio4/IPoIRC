#ifndef IPOIRC_H
#define IPOIRC_H

#include "build-libircclient.h"

#define MAX_IRC_THREADS 20

typedef struct irc_closure {

    int                  xid;
    void                *context;

    char                *netid;
    void                *regex;
    void                *regex_final;
    char                *nick  ,
                        *pass;
    char                *server,
                        *chan;
    int                  port;
    irc_session_t       *irc_s;
} irc_closure;


#endif
