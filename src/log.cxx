
#include <iostream>
#include "log.h"

namespace Log {

    class NullBuffer : public std::streambuf {
        public:
            int overflow(int c) { return c; }
    };

    NullBuffer null_buffer;
    std::ostream null_str(&null_buffer);

    static Level log_level = Debug;

    void set_log_level(Level l) {
        log_level = l;
    }

    std::ostream& gen(const std::string& name, Level l) {
        if (log_level > l) return null_str;
        time_t curtime = time (NULL);
        struct tm *loctime = localtime(&curtime);
        char timestamp[128];
        strftime(timestamp, 127, "%y-%m-%d %H:%M:%S", loctime);
        return std::cout << timestamp << " | [" << name << "] ";
    }

}