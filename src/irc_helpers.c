#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <time.h>
#include "ipoirc.h"
#include "irc_helpers.h"

void irc_debug(irc_thread_data *self, char * format, ...) {
    time_t curtime = time (NULL);
    struct tm *loctime = localtime(&curtime);
    char timestamp[128] = {0};
    strftime(timestamp, 128, "%y-%m-%d %H:%M:%S", loctime);

    char buffer[512] = {0};
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer)-1, format, args);
    printf("%s | [irc#%d] %s\n", timestamp, self->d.id, buffer);
    va_end(args);
}

// http://stackoverflow.com/questions/361363/how-to-measure-time-in-milliseconds-using-ansi-c
int64_t tms_diff (struct timespec *timeA_p, struct timespec *timeB_p) {
  return ((timeA_p->tv_sec * 1000) + (timeA_p->tv_nsec)/ 1.0e6) -
           ((timeB_p->tv_sec * 1000) + (timeB_p->tv_nsec/ 1.0e6));
}


int lsleep(float time) {
    struct timespec tim;
    tim.tv_sec = (int)time;
    tim.tv_nsec = ( (time - tim.tv_sec )* 10)*100000000;
    return nanosleep(&tim, NULL);
}

int ms_delay(int ms) {
    return lsleep(ms / 1000);
}
