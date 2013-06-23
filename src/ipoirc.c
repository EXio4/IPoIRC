#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <zmq.h>

#include "irc.h"
#include "tun.h"
#include "helpers.h"
#include "ipoirc.h"

void usage(char *h) {
    printf("%s usage:\n%s <netid> <local_ip> <remote_ip>\n", h, h);
    exit(1);
}

int main(int argc, char **argv) {

    srand(time(NULL));

    void* context = zmq_ctx_new();

    irc_thread_data irc_data[IRC_THREADS];
    tun_thread_data tun_data[TUN_THREADS];

    pthread_t irc_threads[IRC_THREADS];
    pthread_t tun_threads[TUN_THREADS];

    if (argc < 3) usage(argv[0]);
    int netid = atoi(argv[1]);
    char *h1 = strdup(argv[2]);
    char *h2 = strdup(argv[3]);

    int i=0;

    // define shared data (context and thread id)
    for (i=0; i<IRC_THREADS; i++) {
        irc_data[i].d.id = i;
        irc_data[i].d.context = context;
    }
    for (i=0; i<TUN_THREADS; i++) {
        tun_data[i].d.id = i;
        tun_data[i].d.context = context;
    }

    for (i=0; i<IRC_THREADS; i++) {
        irc_data[i].session_id = netid; // and "socket id" (used in irc <-> irc communication)
        irc_data[i].irc_s = NULL;
    }


    for (i=0; i<TUN_THREADS; i++) {
        tun_data[i].h1 = h1;
        tun_data[i].h2 = h2;
    }

    int rc = 0;

    for (i=0; i<IRC_THREADS; i++) {
        rc = pthread_create(&irc_threads[i], NULL, irc_thread, &irc_data[i]);
        if (rc) {
            // something went wrong!
        }
    }
    for (i=0; i<TUN_THREADS; i++) {
        rc = pthread_create(&tun_threads[i], NULL, tun_thread, &tun_data[i]);
        if (rc) {
            // something went wrong!
        }
    }

    // wait for the threads
    for (i=0; i<IRC_THREADS; i++) {
        pthread_join(irc_threads[i], NULL);
    }
    for (i=0; i<TUN_THREADS; i++) {
        pthread_join(tun_threads[i], NULL);
    }

    return 0;
}
