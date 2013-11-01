#ifndef _IRC_HELPERS_H
#define _IRC_HELPERS_H
void irc_debug(irc_thread_data *self, char * format, ...);
int64_t tms_diff (struct timespec *timeA_p, struct timespec *timeB_p);
int lsleep(float time);
int ms_delay(int ms);
#endif
