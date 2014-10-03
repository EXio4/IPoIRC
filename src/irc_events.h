#ifndef IPOIRC_IRC_EVENTS_H
#define IPOIRC_IRC_EVENTS_H
#include "build-libircclient.h"

void event_connect (irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_join (irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);
void event_message(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count);

#endif
