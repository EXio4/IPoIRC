#include <thread>
#include <regex>

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/select.h>
#include "build-libircclient.h"
#include <pcre.h>
#include "config.h"
#include "irc.h"
#include "irc_events.h"
#include "ipoirc.h"
#include "b2t.h"

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
    char *final_line = (char*)malloc(sizeof(char)*MTU*2); // worst thing that can happen (I hope)
    Comm::Socket socket = self.ctx->connect(Comm::Net2Chat);

    if (!final_line) goto exit;


    while (1) {

        std::vector<uint8_t> buffer = socket->recv();

        std::regex netid_reg    { "%N" };
        std::regex message_reg  { "%M" };
        std::vector<std::string> lines = split(B2T::encode(buffer), 128);
        for (size_t i=0; i<lines.size()-1; i++) {
            std::string final_line = std::regex_replace(std::regex_replace(FORMAT, netid_reg, self.netid), message_reg, lines[i]);
            irc_cmd_msg(self.irc_s, self.chan.c_str(), final_line.c_str());
        }
        std::string final_line = std::regex_replace(std::regex_replace(FORMAT_FINAL, netid_reg, self.netid), message_reg, lines[lines.size()-1]);
        irc_cmd_msg(self.irc_s, self.chan.c_str(), final_line.c_str());

    }

    exit:
    if (final_line)
        free(final_line);

}

void irc_thread_net(irc_closure& self) {
    irc_callbacks_t callbacks;
    irc_ctx_t ctx;

    Comm::Socket socket = self.ctx->connect(Comm::Chat2Net);

    memset (&callbacks, 0, sizeof(callbacks));

    callbacks.event_connect = event_connect;
    callbacks.event_join    = event_join;
    callbacks.event_privmsg = event_message;
    callbacks.event_channel = event_message;

    ctx.channel = self.chan;
    ctx.nick    = std::regex_replace(self.nick, std::regex("%d"), std::to_string(rand()%2048 + 1));
    ctx.self    = self;
    ctx.socket  = socket;

    self.irc_s = irc_create_session(&callbacks);
    if (!self.irc_s) {
        self.log(Log::Fatal) << "error when creating irc_session" << std::endl;
        goto exit;
    }

    irc_set_ctx(self.irc_s, &ctx);

    self.log(Log::Info) << "created irc_session, connecting to " << self.server << ":6667 as " << ctx.nick << "!" << std::endl;

    if (irc_connect (self.irc_s, self.server.c_str(), self.port, self.pass.c_str(), ctx.nick.c_str(), "ipoirc", "IP over IRC - http://github.com/EXio4/IPoIRC")) {
        self.log(Log::Fatal) << "error when connecting to irc (" << irc_strerror(irc_errno(self.irc_s)) << ")" << std::endl;
        goto exit;
    }

    sleep(2); //I still don't know how to solve this issue, we need to wait for something yet we don't know for how long.

    (void) irc_run(self.irc_s);

    exit:
    if (self.irc_s) {
        irc_destroy_session(self.irc_s);
    }
}

void irc_thread(irc_closure self) {

    try {
        self.regex = REGEX;
        self.regex_final = REGEX_FINAL;
    } catch (std::regex_error const &e) {
        self.log(Log::Fatal) << "error loading regex(es)" << std::endl;
        return;
    }

    std::thread zm([&]() {
        return irc_thread_zmq(self);
    });

    while (1) {
        irc_thread_net(self);
        self.log(Log::Warning) << "irc thread died, reconnecting in 2s.." << std::endl;
        sleep(2);
    }


}
