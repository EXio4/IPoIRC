#ifndef _LTUN_H
#define _LTUN_H

#include <dumbnet.h>

typedef struct ltun_t {
    int fd;
    intf_t *intf;
    char *name;
} ltun_t;

ltun_t* ltun_alloc(char *dev, int mtu, char *local, char *remote);
int ltun_read(ltun_t *self, char *buf, int len);
int ltun_write(ltun_t *self, char *buf, int len);
int ltun_close(ltun_t *self);

#endif
