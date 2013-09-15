#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>
#include <libircclient/libircclient.h>
#include <pcre.h>
#include "config.h"
#include "base64.h"
#include "ipoirc.h"
#include "irc.h"
#include "irc_helpers.h"

void event_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);
    irc_thread_data *self = (irc_thread_data*)ctx->self;

    irc_debug(self, "(%s) connected to irc", ctx->nick);
    irc_cmd_join(session, ctx->channel, 0);
}

void event_join(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
/*    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);
    irc_thread_data *self = (irc_thread_data*)ctx->self;

    irc_cmd_user_mode(session, "+i");
    char *buffer = malloc(sizeof(char) * 512);
    // this is defined on-bot-connect time
    //snprintf(buffer, 511, "IPOIRC +%d", self->session_id);
    //irc_cmd_msg(session, params[0], buffer);
    free(buffer); */
}


// helper function for event_message
int __parse(irc_thread_data *self, char *lin, char* netid, char *data, int *vc) {

    snprintf(netid, 511, "%.*s", vc[3] - vc[2], lin + vc[2]);

    if (strcmp(netid, self->netid)) {
        return 0; // invalid id
    }

    snprintf(data , 511, "%.*s", vc[5] - vc[4], lin + vc[4]);

    return 1;
}

void event_message(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
#define parse   ((__parse(self, lin, netid, data, vc)))
#define DONE    42

    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);
    irc_thread_data *self = (irc_thread_data*)ctx->self;

    char *lin    = NULL;
    char *netid  = NULL;
    char *data   = NULL;

    char *st = NULL;

    if (params[1]) {
        lin   = strdup(params[1]);
        netid = malloc(sizeof(char)*512);
        data  = malloc(sizeof(char)*512);

        //if (!netid || !data || !lin) goto clean;

        int vc[9];

        int rc = 0;

        // note: we assume the regex matches *always* the id and the data, for making the code smaller

        if ((rc = pcre_exec(self->regex_final, NULL, lin, (int)strlen(lin), 0, 0, vc, 9)) >= 0 &&
            !parse)
                goto clean; // failed parsing

        if (rc >= 0) rc = DONE;

        if (rc < 0 &&
            (rc = pcre_exec(self->regex, NULL, lin, (int)strlen(lin), 0, 0, vc, 9)) >= 0 &&
            !parse)
                goto clean; // no match with the final nor the "normal"

        snprintf(ctx->buffer, MTU*4-1, "%s%s", ctx->buffer, data);

        if (rc != DONE) goto clean; // data already added, but there is still more data to come

        int z, len = 0;
        len = debase64(ctx->buffer, &st);
        if (len < 1) {
            irc_debug(self, "error when decoding base64 buffer (%d)", len);
            goto iclean;
        }

        if (strcmp(netid, self->netid) == 0) {
            if ((z = zmq_send(ctx->data, st, len, 0)) < 0) {
                irc_debug(self, "error when trying to send a message to the tun (warning, this WILL result in missing packets!)", zmq_strerror(errno));
            }
        }
        iclean:
        memset(ctx->buffer, 0, strlen(ctx->buffer));

        clean:
        if (lin) free(lin);
        if (netid) free(netid);
        if (data) free(data);
        if (st) free(st);
    }
#undef parse
#undef DONE
}

void dump_event (irc_session_t * session, const char * event, const char * origin, const char ** params, unsigned int count)
{
    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);
    irc_thread_data *self = (irc_thread_data*)ctx->self;

    char buf[512];
    int cnt;

    buf[0] = '\0';

    for ( cnt = 0; cnt < count; cnt++ )
    {
        if ( cnt )
            strcat (buf, "|");

        strcat (buf, params[cnt]);
    }


    irc_debug(self, "Event \"%s\", origin: \"%s\", params: %d [%s]", event, origin ? origin : "NULL", cnt, buf);
}
