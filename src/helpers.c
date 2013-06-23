#include <string.h>
#include <unistd.h>

int split(char *str, char **fstr) {
     char *rest;
     char *p;
     int i=0;

     for(p = strtok_r(str,"\n", &rest); p; p = strtok_r(NULL, "\n", &rest)) {
               fstr[i] = p;
               i++;
     }
     return i;
}
