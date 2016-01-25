#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "ipoirc.h"
#include "irc_helpers.h"

void irc_debug(const irc_closure& self, const char * format, ...) {
    time_t curtime = time (NULL);
    struct tm *loctime = localtime(&curtime);
    char timestamp[128] = {0};
    strftime(timestamp, 128, "%y-%m-%d %H:%M:%S", loctime);

    char buffer[512] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer)-1, format, args);
    printf("%s | [irc#%d] %s\n", timestamp, self.xid, buffer);
    va_end(args);
}
