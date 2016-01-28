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
#include "base64.h"

std::vector<std::string> split(const std::string& str, int splitLength) {
   int NumSubstrings = str.length() / splitLength;
   std::vector<std::string> ret;

   for (auto i = 0; i < NumSubstrings; i++) {
        ret.push_back(str.substr(i * splitLength, splitLength));
   }

   // If there are leftover characters, create a shorter item at the end.
   if (str.length() % splitLength != 0) {
        ret.push_back(str.substr(splitLength * NumSubstrings));
   }

   return ret;
}

void irc_thread_zmq(irc_closure& self) {
    char *sbuffer = (char*)malloc(sizeof(char)*MTU);
    char *final_line = (char*)malloc(sizeof(char)*MTU*2); // worst thing that can happen (I hope)

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

        std::vector<std::string> lines = split(base64(sbuffer, nbytes), 128);
        for (size_t i=0; i<lines.size()-1; i++) {
            snprintf(final_line, (MTU*2)-1, FORMAT, self.netid, lines[i].c_str());
            irc_cmd_msg(self.irc_s, self.chan, final_line);
        }
        snprintf(final_line, (MTU*2)-1, FORMAT_FINAL, self.netid, lines[lines.size()-1].c_str());
        irc_cmd_msg(self.irc_s, self.chan, final_line);

    }

    exit:

    if (sbuffer)
        free(sbuffer);
    if (final_line)
        free(final_line);
    if (socket)
        zmq_close(socket);

}

void irc_thread_net(irc_closure& self) {
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

    try {
        self.regex = REGEX;
        self.regex_final = REGEX_FINAL;
    } catch (std::regex_error const &e) {
        irc_debug(self, "error loading regex(es)");
        return;
    }

    std::thread zm([&]() {
        return irc_thread_zmq(self);
    });

    while (1) {
        std::thread th([&]() {
            return irc_thread_net(self);
        });
        th.join();
        irc_debug(self, "irc thread died, reconnecting in 2s..");
        sleep(2);
    }


}
