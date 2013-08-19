// it is dumbnet in debian, but, how could we check if it is called dumbnet or dnet? (a define, when compiling?)
#include <stdio.h>
#include <string.h>
#include <bsd/string.h>
#include <stdlib.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include "ltun.h"

#define __TUN_INTEGRITY_CHECK(tun, leave)  do {\
                                        if (!tun) {\
                                            leave;\
                                        }\
                                    } while(0);

ltun_t* ltun_alloc(char *dev, char *local, char *remote) {

    int ln = strlen(dev);
    ltun_t *sf = malloc(sizeof(ltun_t));
    char *dv = malloc(sizeof(char)*ln+1);

    struct intf_entry ifent;
    struct addr a, b;

    struct ifreq ifr;
    int fd = -1, err;
    char *clonedev = "/dev/net/tun";

    if (!sf) goto exit;
    // check for dv

    sf->intf = intf_open();

    if( ((sf->intf == NULL) || (fd = open(clonedev , O_RDWR)) < 0)) {
        fd = -1;
        goto exit;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    if (*dev) {
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
        close(fd);
        fd = -1;
        goto exit;
    }

    strncpy(dv, ifr.ifr_name, ln);

    sf->fd = fd;
    sf->name = dv;


    addr_pton(local, &a);
    addr_pton(remote, &b);

    memset(&ifent, 0, sizeof(ifent));
    strlcpy(ifent.intf_name, ifr.ifr_name, sizeof(ifent.intf_name));
    ifent.intf_flags = INTF_FLAG_UP|INTF_FLAG_POINTOPOINT;
    ifent.intf_addr = a;
    ifent.intf_dst_addr = b;
    ifent.intf_mtu = 1500;

    if (intf_set(sf->intf, &ifent) < 0)
        fd = -1;

    exit:

    if (fd < 0) {
        free(sf);
        sf = NULL;
    }

    return sf;
}

int ltun_read(ltun_t *self, char *buf, int len) {
    __TUN_INTEGRITY_CHECK(self, return -1)

    return read(self->fd, buf, len);
}

int ltun_write(ltun_t *self, char *buf, int len) {
    __TUN_INTEGRITY_CHECK(self, return -1)

    return write(self->fd, buf, len);
}

int ltun_close(ltun_t *self) {
    __TUN_INTEGRITY_CHECK(self, return -1)

    close(self->fd);
    free(self->name);
    free(self);

    self = NULL;

    return 0;
}

