#include <thread>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include <zmq.h>
#include "build-libircclient.h"
#include <pcre.h>
#include "config.h"
#include "irc.h"
#include "irc_events.h"
#include "ipoirc.h"
#include "irc_helpers.h"
#include "helpers.h"
#include "base64.h"

void irc_thread_zmq(irc_closure self) {
    char *sbuffer = (char*)malloc(sizeof(char)*MTU);
    char *final_line = (char*)malloc(sizeof(char)*MTU*2); // worst thing that can happen (I hope)
    char *b64 = NULL;
    char **b64s = (char**)malloc(sizeof(char)*MTU);
    int i = 0;
    int lines = 0;

    void *socket = NULL;

    if (!sbuffer) goto exit;
    if (!final_line) goto exit;

    socket = zmq_socket(self.context, ZMQ_PULL); // client of the tun_socket

    if (zmq_connect(socket, "inproc://#tun_to_#irc")) {
        irc_debug(self, "(irc_thread_zmq) error when connecting to IPC socket - %s", zmq_strerror(errno));
        goto exit;
    }

    while (1) {
        memset(sbuffer, 0, MTU);
        int nbytes = zmq_recv(socket, sbuffer, MTU, 0);

        if (nbytes < 0) {
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
            char *format = (char*)malloc(sizeof(char)*16);
            if (i == (lines-1)) {
                snprintf(format, 15, "%s", FORMAT_FINAL);
            } else {
                snprintf(format, 15, "%s", FORMAT);
            }

            snprintf(final_line, (MTU*2)-1, format, self.netid, b64s[i]);
            irc_cmd_msg(self.irc_s, self.chan, final_line);

            free(format);
        }

        free(b64);
    }

    exit:

    if (sbuffer)
        free(sbuffer);
    if (final_line)
        free(final_line);
    if (socket)
        zmq_close(socket);

}

void irc_thread_net(irc_closure self) {
    irc_callbacks_t callbacks;
    irc_ctx_t ctx;

    void *socket = zmq_socket(self.context, ZMQ_PUSH); // "server" from irc -> tun

    if (zmq_connect(socket, "inproc://#irc_to_#tun")) {
        irc_debug(self, "(irc_thread_net) error when creating IPC socket - %s", zmq_strerror(errno));
        goto exit;
    }

    memset (&callbacks, 0, sizeof(callbacks));

    callbacks.event_connect = event_connect;
    callbacks.event_join    = event_join;
    callbacks.event_privmsg = event_message;
    callbacks.event_channel = event_message;

    ctx.channel = strdup(self.chan);
    ctx.nick    = (char*)malloc(sizeof(char)*512);
    snprintf(ctx.nick, 511, self.nick, rand()%2048+1);
    ctx.self    = self;
    ctx.ds      = NULL;
    ctx.data    = socket; // WE ARE PASSING A THREAD-UNSAFE SOCKET HERE!

    self.irc_s = irc_create_session(&callbacks);
    if (!self.irc_s) {
        irc_debug(self, "error when creating irc_session");
        goto exit;
    }

    irc_set_ctx(self.irc_s, &ctx);

    irc_debug(self, "created irc_session, connecting to %s:6667 as %s!", self.server, ctx.nick);

    if (irc_connect (self.irc_s, self.server, self.port, self.pass, ctx.nick, "ipoirc", "IP over IRC - http://github.com/EXio4/IPoIRC")) {
        irc_debug(self, "error when connecting to irc (%s)", irc_strerror(irc_errno(self.irc_s)));
        goto exit;
    }

    sleep(1); // wait for the network to answer THIS SHOULD BE DONE IN A RIGHT WAY!

    (void) irc_run(self.irc_s);

    exit:
    if (self.irc_s) {
        irc_destroy_session(self.irc_s);
    }
    if (socket) {
        zmq_close(socket);
    }
}

void irc_thread(irc_closure self) {

    {
        // should be compiled in main thread, not here
        const char *error;
        int erroroffset;

        self.regex = (void*)pcre_compile(REGEX, 0, &error, &erroroffset, NULL);

        if (!self.regex) {
            irc_debug(self, "(irc_thread) error compiling regex (%s)", REGEX);
            return;
        }

        self.regex_final = (void*)pcre_compile(REGEX_FINAL, 0, &error, &erroroffset, NULL);
        if (!self.regex_final) {
            irc_debug(self, "(irc_thread) error compiling regex (%s)", REGEX);
            return;
        }

    }

    std::thread zm([self]() {
        return irc_thread_zmq(self);
    });

    while (1) {
        std::thread th([self]() {
            return irc_thread_net(self);
        });
        th.join();
        irc_debug(self, "irc thread died, reconnecting in 2s..");
        sleep(2);
    }


}
