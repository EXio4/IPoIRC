#ifndef IPOIRC_LTUN_H
#define IPOIRC_LTUN_H

#include "build-dnet.h"

typedef struct ltun_t {
    int fd;
    intf_t *intf;
    char *name;
} ltun_t;

ltun_t* ltun_alloc(const char *dev, int mtu, const char *local, const char *remote);
int ltun_read(ltun_t *self, char *buf, int len);
int ltun_write(ltun_t *self, const char *buf, int len);
int ltun_close(ltun_t *self);

#endif
