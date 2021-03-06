#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>
#include <pcre.h>
#include "config.h"
#include "b2t.h"
#include "ipoirc.h"
#include "irc.h"

void event_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);

    // shutup compiler complaining about unused variables
    (void) event; (void) origin; (void) params; (void) count;

    ctx->self.log(Log::Info) << "(" << ctx->nick << ") connected to irc" << std::endl;
    irc_cmd_join(session, ctx->channel.c_str(), 0);
}

void event_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    (void) session; (void) event; (void) origin; (void) params; (void) count;
}


void event_message(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {

    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);

    // shutup compiler complaining about unused variables
    (void) event; (void) count;

    std::string nick;
    {
        char *c_nick   = (char*)malloc(sizeof(char)*256);
        if (!c_nick) return;
        irc_target_get_nick(origin, c_nick, 255);
        nick = c_nick;
    }


    if (ctx->self.xid == 0 && params[1]) {

        std::string lin = params[1];
        std::string netid;
        std::string data;

        bool final_match = false;
        std::smatch p_match;

        if  (std::regex_match(lin, p_match, ctx->self.regex_final) && p_match.size() == 2+1) {
            final_match = true;
            netid = p_match[1];
            data  = p_match[2];
        } else if (std::regex_match(lin, p_match, ctx->self.regex) && p_match.size() == 2+1) {
            final_match = false;
            netid = p_match[1];
            data  = p_match[2];
        } else {
            return;
        }
        if (netid == ctx->self.netid) return;

        std::string& buf = ctx->dt[nick];

        buf += data;

        if (final_match) {
            boost::optional<std::vector<uint8_t>> st = B2T::decode(buf);
            if (!st) {
                ctx->self.log(Log::Error) << "error when decoding base64 buffer (" << buf << ")" << std::endl;
            } else if (ctx->socket->send(*st) < 0) {
                    ctx->self.log(Log::Error) << "error trying to send a message to the tun (warning, this WILL result in missing packets!): " << zmq_strerror(errno) << std::endl;
            }
            buf = "";
        };


    }
}


#undef parse
