#ifndef IPOIRC_LOG_H
#define IPOIRC_LOG_H
#include <iostream>
namespace Log {

    enum Level {
        Debug   ,
        Info    ,
        Warning ,
        Error   ,
        Fatal   ,
    };

    void set_log_level(Level);

    std::ostream& gen(const std::string&, Level);
}

#endif
