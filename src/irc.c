#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
#include <zmq.h>
#include <libircclient/libircclient.h>
#include "config.h"
#include "irc.h"
#include "irc_events.h"
#include "ipoirc.h"
#include "irc_helpers.h"
#include "helpers.h"
#include "base64.h"

void* irc_thread_zmq(void *data) {
    irc_thread_data *self = (irc_thread_data*)data;
    char *sbuffer = malloc(sizeof(char)*MTU);
    char *final_line = malloc(sizeof(char)*MTU*2); // worse thing that can happen?
    char *b64 = NULL;
    char **b64s = malloc(sizeof(char)*MTU);
    int i = 0;
    int lines = 0;

    if (!sbuffer) goto exit;
    if (!final_line) goto exit;

    void *socket = zmq_socket(self->d.context, ZMQ_PAIR); // client of the tun_socket
    sleep(1); // wait for other threads and shitz, TODO: make a proper way with semaphores and shitz
    int ret = zmq_connect(socket, "inproc://#tun_to_#irc");
    if (ret) {
        irc_debug(self, "(irc_thread_zmq) error when connecting to IPC socket - %s", zmq_strerror(errno));
        goto exit;
    }

    while (1) {
        irc_debug(self, ">> zmq_recv <<");
        memset(sbuffer, 0, MTU);
        int nbytes = zmq_recv(socket, sbuffer, MTU, 0);

        if (nbytes < 0 ) {
            irc_debug(self, "error when reading from zeromq socket");
            goto exit; // a cute break here!
        } else if (nbytes == 0) {
            continue;
        } else if (nbytes > MTU) {
            irc_debug(self, "warning: some message got truncated by %d (%d - %d), this means the MTU is too low for you!", nbytes - MTU, nbytes, MTU);
            nbytes = MTU;
        }

        base64(sbuffer, nbytes, &b64);
        // split newlines
        lines = split(b64, b64s);
        for (i=0; i<lines; i++) {
            char *format;
            if (i == (lines-1)) {
                format = strdup("%d:%s]");
            } else {
                format = strdup("%d:%s");
            }

            snprintf(final_line, (MTU*2)-1, format, self->session_id, b64s[i]);
            irc_cmd_msg(self->irc_s, IRC_CHANNEL, final_line);

            free(format);
        }
        irc_debug(self, "@ %d", lines);

        free(b64);
    }

    exit:

    if (sbuffer)
        free(sbuffer);
    if (final_line)
        free(final_line);

    pthread_exit(NULL);
}

void* irc_thread_net(void *data) {
    irc_thread_data *self = (irc_thread_data*)data;
    irc_callbacks_t callbacks;
    irc_ctx_t ctx;
    irc_session_t *irc_s;

    void *socket = zmq_socket(self->d.context, ZMQ_PAIR); // "server" from irc -> tun
    int ret = zmq_bind(socket, "inproc://#irc_to_#tun");

    if (ret) {
        irc_debug(self, "(irc_thread_net) error when creating IPC socket - %s", zmq_strerror(errno));
        goto exit;
    }

    memset (&callbacks, 0, sizeof(callbacks));

    callbacks.event_connect = event_connect;
    callbacks.event_join = event_join;
    callbacks.event_privmsg = event_message;
    callbacks.event_channel = event_message;

    callbacks.event_nick = dump_event;
    callbacks.event_quit = dump_event;
    callbacks.event_part = dump_event;
    callbacks.event_mode = dump_event;
    callbacks.event_topic = dump_event;
    callbacks.event_kick = dump_event;
    callbacks.event_notice = dump_event;
    callbacks.event_invite = dump_event;
    callbacks.event_umode = dump_event;
    callbacks.event_ctcp_rep = dump_event;
    callbacks.event_ctcp_action = dump_event;
    callbacks.event_unknown = dump_event;

    ctx.channel = strdup(IRC_CHANNEL);
    ctx.nick = malloc(sizeof(char)*512);
    snprintf(ctx.nick, 511, IRC_NICK, rand()+rand());
    ctx.self = self;
    ctx.buffer = malloc(sizeof(char)*MTU*4);
    ctx.data = socket; // WE ARE PASSING A NON-THREAD-SAFE SOCKET HERE! </redwarning>

    irc_s = irc_create_session(&callbacks);
    if (!irc_s) {
        irc_debug(self, "error when creating irc_session");
        goto exit;
    }
    self->irc_s = irc_s;

    irc_debug(self, "created irc_session!");
    irc_set_ctx(self->irc_s, &ctx);

    if (irc_connect (self->irc_s, IRC_NETWORK, 6667, IRC_PASSWD, ctx.nick, "ipoirc", "IP over IRC - coming soon(tm)")) {
        irc_debug(self, "error when connecting to irc (%s)", irc_strerror(irc_errno(self->irc_s)));
        goto exit;
    }

    sleep(1); // wait for the network to answer THIS SHOULD BE DONE IN A RIGHT WAY!

    irc_debug(self, "irc_run!");
    int rc = irc_run(self->irc_s);

    irc_debug(self, "irc run finished with %d", rc);

    irc_debug(self, "in case of error: %s", irc_strerror(irc_errno(self->irc_s)));

    exit:
    if (self->irc_s) {
        irc_destroy_session(self->irc_s);
    }
    pthread_exit(NULL);
}

void* irc_thread(void* data) {
    irc_thread_data *self = (irc_thread_data*)data;

    pthread_t network_thread;
    pthread_t zeromq_thread;

    int n_th = pthread_create(&network_thread, NULL, irc_thread_net, (void*)self);
    if (n_th) {
        irc_debug(self, "error when creating irc thread");
        goto exit;
    }

    int z_th = pthread_create(&zeromq_thread, NULL, irc_thread_zmq, (void*)self);
    if (z_th) {
        irc_debug(self, "error when creating zmq thread");
        goto exit;
    }

    pthread_join(network_thread, NULL); // we don't care about the "zeromq one"!

    exit:

    pthread_exit(NULL);
}
