#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <zmq.h>
#include <libircclient/libircclient.h>
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

void event_message(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) {
    irc_ctx_t *ctx = (irc_ctx_t*)irc_get_ctx(session);
    irc_thread_data *self = (irc_thread_data*)ctx->self;

    // this is insanely thread-unsafe, so this needs a rework, but lazy as fuck.

    char *tk;
    char *search = ":";
    char *lin = NULL;

    int id, dn = 0;

    if (params[1]) {
        lin = strdup(params[1]);

        if (!strstr(lin, ":"))
            goto clean; // skip messages without :

        tk = strtok(lin, search);
        id = atoi(tk);

        tk = strtok(NULL, search);
        if (tk[strlen(tk)-1] == ']') { dn = 1; tk[strlen(tk)-1]='\0'; }

        strcat(ctx->buffer, tk);

        if (dn == 1) {
        // buffer contains base64 shit now, ready to "decode"
            int z, len = 0;
            char *st = NULL;
            len = debase64(ctx->buffer, &st);
            irc_debug(self, "(decoding)base64 returned %d", len);
            if (id == self->session_id && len > 0) {
                if ((z = zmq_send(ctx->data, st, len, 0)) < 0) {
                    irc_debug(self, "error when trying to send a message to the tun (warning, this WILL result in missing packets!)", zmq_strerror(errno));
                }
            }
            memset(ctx->buffer, 0, strlen(ctx->buffer));
            free(st);
        }
        clean:
        free(lin);
    }

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
