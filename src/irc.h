#ifndef IPOIRC_IRC_H
#define IPOIRC_IRC_H
#include "ipoirc.h"
#include <map>

typedef struct irc_ctx_t {
    char                *channel;
    char                *nick;
    irc_closure          self;
    std::map<std::string,std::string> dt;
    void                *data;
} irc_ctx_t;


void irc_thread(irc_closure self);
#endif
