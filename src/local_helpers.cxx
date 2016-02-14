#include "log.h"
#include "local_helpers.h"

std::ostream& loc_log(Log::Level l) {
    return Log::gen("local", l);
}