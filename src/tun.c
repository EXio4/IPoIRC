#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <zmq.h>

#include "ipoirc.h"
#include "config.h"
#include "helpers.h"
#include "ltun.h"
#include "tun.h"
#include "tun_helpers.h"

void* tun_thread_zmq(void *data) {
    tun_thread_data *self = (tun_thread_data*)data;

    char *sbuffer = malloc(sizeof(char)*MTU);
    if (!sbuffer) goto exit;

    void *socket = zmq_socket(self->d.context, ZMQ_PAIR); // client of the tun_socket
    sleep(1); // wait for other threads and shitz, TODO: make a proper way with semaphores and shitz
    int ret = zmq_connect(socket, "inproc://#irc_to_#tun");
    if (ret) {
        tun_debug(self, "(tun_thread_zmq) error when connecting to IPC socket - %s", zmq_strerror(errno));
        goto exit;
    }
    while (1) {
        tun_debug(self, ">> zmq_recv <<");
        memset(sbuffer, 0, MTU);
        int nbytes = zmq_recv(socket, sbuffer, MTU, 0);
        tun_debug(self, "got %d bytes", nbytes);
        if (nbytes < 0 ) {
            tun_debug(self, "error when reading from zeromq socket");
            goto exit; // a cute break here!
        } else if (nbytes == 0) {
            continue;
        } else if (nbytes > MTU) {
            tun_debug(self, "warning: some message got truncated by %d (%d - %d), this means the MTU is too low for you!", nbytes - MTU, nbytes, MTU);
        }
        ltun_write(self->tun, sbuffer, nbytes);
    }
    exit:
    pthread_exit(NULL);
}

void* tun_thread_dt(void *data) {
    tun_thread_data *self = (tun_thread_data*)data;
    void *socket = zmq_socket(self->d.context, ZMQ_PAIR);
    int rc = zmq_bind(socket, "inproc://#tun_to_#irc");

    char *sbuffer = malloc(sizeof(char)*MTU);
    if (!sbuffer) goto exit;

    if (rc) {
        tun_debug(self, "error when creating IPC socket - %s", zmq_strerror(errno));
        goto exit;
    }

    // tell_to_other_threads_the_tun2irc_socket_is_binded
    tun_debug(self, "[data] created tun (data) thread!");
    while (1) {
        int nbytes = ltun_read(self->tun, sbuffer, MTU);
        if (nbytes > 0) {
            tun_debug(self, "got %d from tun", nbytes);
        }
        if (zmq_send(socket, sbuffer, nbytes, 0) < 0) {
            tun_debug(self, "error when trying to send a message to the irc thread (warning, we continue here!)", zmq_strerror(errno));
        }
    }

    exit:

    pthread_exit(NULL);
}


void* tun_thread(void* data) {
    tun_thread_data *self = (tun_thread_data*)data;

    pthread_t data_thread = {0};
    pthread_t zeromq_thread = {0};

    self->tun = (void*)ltun_alloc("irc%d", self->h1, self->h2);

    if (!self->tun) {
        tun_debug(self, "error start the tun interface, are you root?");
        goto exit;
    }

    int d_th = pthread_create(&data_thread, NULL, tun_thread_dt, (void*)self);
    if (d_th) {
        tun_debug(self, "error when creating tun thread (tun interface)");
        goto exit;
    }

    int z_th = pthread_create(&zeromq_thread, NULL, tun_thread_zmq, (void*)self);
    if (z_th) {
        tun_debug(self, "error when creating tun thread (zmq interface)");
        goto exit;
    }

    pthread_join(data_thread, NULL);

    exit:
    pthread_exit(NULL);
}
