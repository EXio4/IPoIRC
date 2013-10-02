#ifndef _IPOIRC_H
#define _IPOIRC_H

#include <pthread.h>
#include <libircclient/libircclient.h>

#define IRC_THREADS 1 /* only one irc thread for now; more actually work but
                         need protocol tweaks for not having clients sending
                         data to themselves */

typedef struct shared_thread_data {
    int id;
    void *context;
} shared_thread_data; // internal

typedef struct irc_thread_data {
    shared_thread_data  d;
    char                *netid;
    void                *regex;
    void                *regex_final;
    char                *nick, *pass;
    char                *server, *chan;
    irc_session_t       *irc_s;
} irc_thread_data;

typedef struct tun_thread_data {
    shared_thread_data  d;
    void                *tun;
    char                *h1, *h2;
    pthread_mutex_t     mutex;
} tun_thread_data;

#endif
