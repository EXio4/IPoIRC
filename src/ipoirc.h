#ifndef IPOIRC_H
#define IPOIRC_H

#include "build-libircclient.h"
#include <iostream>
#include <regex>

#define MAX_IRC_THREADS 20

typedef struct irc_closure {

    int                  xid;
    void                *context;

    std::string netid;
    std::regex regex;
    std::regex regex_final;
    std::string         nick  ,
                        pass;
    std::string server,
                chan;
    int                  port;
    irc_session_t       *irc_s;
} irc_closure;


inline std::ostream& debug_gen(std::string name) {
    time_t curtime = time (NULL);
    struct tm *loctime = localtime(&curtime);
    char timestamp[128];
    strftime(timestamp, 127, "%y-%m-%d %H:%M:%S", loctime);
    return std::cout << timestamp << " | [" << name << "] ";
}

#endif
