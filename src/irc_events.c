#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>
#include <libircclient/libircclient.h>
#include <pcre.h>
#include "constants.h"
#include "config.h"
#include "base64.h"
#include "ipoirc.h"
#include "irc.h"
#include "irc_helpers.h"

// helper macro for event_message
#define parse   ((__parse(self, lin, netid, data, vc)))

void event_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);
    irc_thread_data *self = (irc_thread_data*)ctx->self;

    // shutup compiler complaining about unused variables
    (void) event; (void) origin; (void) params; (void) count;

    irc_debug(self, "(%s) connected to irc", ctx->nick);
    irc_cmd_join(session, ctx->channel, 0);
}

void event_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);
    irc_thread_data *self = (irc_thread_data*)ctx->self;

    // shutup compiler complaining about unused variables
    (void) event; (void) origin; (void) params; (void) count;
    (void) self;
}


// helper function for event_message
int __parse(irc_thread_data *self, char *lin, char* netid, char *data, int *vc) {

    snprintf(netid, 511, "%.*s", vc[3] - vc[2], lin + vc[2]);

    if (strcmp(netid, self->netid) == 0) {
        return 0; // same id means same client, ignore
    }

    snprintf(data , 511, "%.*s", vc[5] - vc[4], lin + vc[4]);

    return 1;
}

void event_message(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {

    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);
    irc_thread_data *self = (irc_thread_data*)ctx->self;

    // shutup compiler complaining about unused variables
    (void) event; (void) count;

    char *lin    = NULL;
    char *netid  = NULL;
    char *data   = NULL;
    char *nick   = malloc(sizeof(char)*256);

    char *st     = NULL;

    dbuf* buf = NULL;

    irc_target_get_nick(origin, nick, 255);

    if (self->d.id != 0) goto out;

    if (params[1]) {
        lin   = strdup(params[1]);
        netid = malloc(sizeof(char)*512);
        data  = malloc(sizeof(char)*512);

        //if (!netid || !data || !lin) goto clean;

        int vc[9];

        int rc = 0;

        // note: we assume the regex matches *always* the id and the data, for making the code smaller and simpler

        rc = pcre_exec(self->regex_final, NULL, lin, (int)strlen(lin), 0, 0, vc, 9);
        if (rc >= 0 && !parse)
                goto clean; // failed parsing

        if (rc >= 0) rc = DONE;

        if (rc < 0) {
            rc = pcre_exec(self->regex, NULL, lin, (int)strlen(lin), 0, 0, vc, 9);
            if (rc < 0 || !parse)
                goto clean; // no match with the final nor the "normal"
        }

        HASH_FIND_STR((ctx->ds), nick, buf);

        if (!buf) {
            buf = malloc(sizeof(dbuf));
            buf->dt = malloc(sizeof(char)*MTU*4);
            strncpy(buf->nick, nick, 127);
            HASH_ADD_STR((ctx->ds), nick, buf);
        }

        snprintf(buf->dt, MTU*4-1, "%s%s", buf->dt, data);

        if (rc != DONE) goto clean; // data already added, but there is still more data to come

        int z, len = 0;
        len = debase64(buf->dt, &st);
        if (len < 1) {
            irc_debug(self, "error when decoding base64 buffer (%d)", len);
            goto iclean;
        }

        if ((z = zmq_send(ctx->data, st, len, 0)) < 0) {
                irc_debug(self, "error when trying to send a message to the tun (warning, this WILL result in missing packets!)", zmq_strerror(errno));
        }
        iclean:
        memset(buf->dt, 0, strlen(buf->dt));

        clean:
        if (lin) free(lin);
        if (netid) free(netid);
        if (data) free(data);
        if (st) free(st);
    }

    out:
    if (nick) free(nick);
}


#undef parse
