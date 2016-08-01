#ifndef IPOIRC_H
#define IPOIRC_H

#include "build-libircclient.h"
#include "log.h"
#include "comm.h"
#include <memory>
#include <regex>

#define MAX_IRC_THREADS 20

typedef struct irc_closure {

    int                  xid;
    Comm::Ctx            ctx;

    std::string         netid;
    std::regex          regex      ,
                        regex_final;
    std::string         nick  ,
                        pass  ,
                        server,
                        chan;
    int                  port;
    irc_session_t       *irc_s;

    std::ostream& log(Log::Level l) {
        return Log::gen("irc" + std::to_string(xid), l);
    }
} irc_closure;

#endif
