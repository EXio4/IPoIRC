#ifndef _IRC_EVENTS_H
#define _IRC_EVENTS_H
#include <libircclient/libircclient.h>

void event_connect (irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_join (irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_message(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);

#endif
