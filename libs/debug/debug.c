#include <stdio.h>
#include <stdarg.h>

void DEBUG(char *fmt, ...) {
#ifdef NDEBUG
	va_list args;
	va_start(args, fmt);
   	vfprintf(stderr,fmt,args);
   	va_end(args);
#endif
   	return;
}
