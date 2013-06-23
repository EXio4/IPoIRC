#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "ipoirc.h"
#include "tun_helpers.h"

void tun_debug(tun_thread_data *self, char * format, ...) {
    time_t curtime = time (NULL);
    struct tm *loctime = localtime(&curtime);
    char timestamp[128];
    strftime(timestamp, 128, "%y-%m-%d %H:%M:%S", loctime);

    char buffer[512];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    printf("%s | [tun#%d] %s\n", timestamp, self->d.id, buffer);
    va_end(args);
}
