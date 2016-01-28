#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>
#include <pcre.h>
#include "constants.h"
#include "config.h"
#include "base64.h"
#include "ipoirc.h"
#include "irc.h"
#include "irc_helpers.h"

void event_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);

    // shutup compiler complaining about unused variables
    (void) event; (void) origin; (void) params; (void) count;

    irc_debug(ctx->self) << "(" << ctx->nick << ") connected to irc" << std::endl;
    irc_cmd_join(session, ctx->channel, 0);
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

        std::string &buf = ctx->dt[nick];

        buf += data;

        if (final_match) {
            char *st = NULL;
            int len = debase64(buf , &st);
            if (len < 1) {
                irc_debug(ctx->self) << "error when decoding base64 buffer (" << len << ")" << std::endl;
            } else if (zmq_send(ctx->data, st, len, 0) < 0) {
                    irc_debug(ctx->self) << "error trying to send a message to the tun (warning, this WILL result in missing packets!): " << zmq_strerror(errno) << std::endl;
            }
            buf = "";
        };


    }
}


#undef parse
