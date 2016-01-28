#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "ipoirc.h"
#include "irc_helpers.h"

std::ostream&  irc_debug(const irc_closure& self) {
    return debug_gen("irc" + std::to_string(self.xid));
}