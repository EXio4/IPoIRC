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

    irc_debug(ctx->self, "(%s) connected to irc", ctx->nick);
    irc_cmd_join(session, ctx->channel, 0);
}

void event_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    (void) session; (void) event; (void) origin; (void) params; (void) count;
}


void event_message(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {

    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);

    // shutup compiler complaining about unused variables
    (void) event; (void) count;

    char *nick   = (char*)malloc(sizeof(char)*256);

    char *st     = NULL;

    dbuf* buf = NULL;

    irc_target_get_nick(origin, nick, 255);

    if (ctx->self.xid == 0 && params[1]) {

        std::string lin = params[1];
        std::string netid;
        std::string data;

        bool final_match = false;
        std::smatch p_match;

        if (std::regex_match(lin, p_match, ctx->self.regex_final) && p_match.size() == 2+1) {
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

        HASH_FIND_STR((ctx->ds), nick, buf);

        if (!buf) {
            irc_debug(ctx->self, "allocating a new buffer structure for %s", nick);
            buf = (dbuf*)malloc(sizeof(dbuf));
            buf->dt = (char*)malloc(sizeof(char)*MTU*4);
            memset(buf->dt, 0, MTU*4);
            strncpy(buf->nick, nick, 127);
            HASH_ADD_STR((ctx->ds), nick, buf);
        }

        // ugly workaround to a bug that comes from nonwhere, try to know why it doesn't work somewhen
        {
            char buffer[MTU*4] = {0};
            snprintf(buffer, MTU*4-1, "%s%s", buf->dt, data.c_str());
            snprintf(buf->dt, MTU*4-1, "%s", buffer);
        }


        if (final_match) {
            int z, len = 0;
            len = debase64(buf->dt, &st);
            if (len < 1) {
                irc_debug(ctx->self, "error when decoding base64 buffer (%d)", len);
            } else if ((z = zmq_send(ctx->data, st, len, 0)) < 0) {
                    irc_debug(ctx->self, "error when trying to send a message to the tun (warning, this WILL result in missing packets!)", zmq_strerror(errno));
            }
            memset(buf->dt, 0, strlen(buf->dt));
        };


    }

    if (nick) free(nick);
}


#undef parse
