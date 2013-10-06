#ifndef _IPOIRC_H
#define _IPOIRC_H

#include <pthread.h>
#include <libircclient/libircclient.h>

#define MAX_IRC_THREADS 20

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
    int                 port;
    irc_session_t       *irc_s;
} irc_thread_data;

typedef struct tun_thread_data {
    shared_thread_data  d;
    void                *tun;
    pthread_mutex_t     mutex;
} tun_thread_data;

#endif
