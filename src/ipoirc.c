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
    printf("%s usage:\n%s <netid> <irc_nick> <irc_network> <irc_pass> <irc_channel> <local_ip> <remote_ip>\n", h, h);
    exit(1);
}

int main(int argc, char **argv) {

    srand(time(NULL));

    void* context = zmq_ctx_new();

    irc_thread_data irc_data[IRC_THREADS];
    tun_thread_data tun_data;

    pthread_t irc_threads[IRC_THREADS];
    pthread_t tun_threadS;

    if (argc < 7) usage(argv[0]);

    char *netid = argv[1];
    char *nick  = argv[2];
    char *net   = argv[3];
    char *pass  = argv[4];
    char *chan  = argv[5];
    char *h1    = argv[6];
    char *h2    = argv[7];

    int i       = 0;


    // define shared data (context and thread id)
    for (i=0; i<IRC_THREADS; i++) {
        irc_data[i].d.id = i;
        irc_data[i].d.context = context;
    }

    tun_data.d.id = -1;
    tun_data.d.context = context;

    for (i=0; i<IRC_THREADS; i++) {
        irc_data[i].netid = netid; // and "socket id" (used in irc <-> irc communication)
        irc_data[i].nick = nick;
        irc_data[i].pass = pass;
        irc_data[i].server = net;
        irc_data[i].chan = chan;
        irc_data[i].irc_s = NULL;
    }


    tun_data.h1 = h1;
    tun_data.h2 = h2;

    int rc = 0;

    rc = pthread_create(&tun_threadS, NULL, tun_thread, &tun_data);
    if (rc) {
        // something went wrong!
    }

    sleep(1);

    // we could drop privs here?

    for (i=0; i<IRC_THREADS; i++) {
        rc = pthread_create(&irc_threads[i], NULL, irc_thread, &irc_data[i]);
        if (rc) {
            // something went wrong!
        }
    }


    pthread_join(tun_threadS, NULL);

    return 0;
}
