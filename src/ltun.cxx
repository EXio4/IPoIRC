#include <iostream>
#include <stdexcept>

#include <string.h>
#include <bsd/string.h>
#include <stdio.h>
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
#include "utils.h"

Tun::Tun(std::string dev, uint16_t mtu, std::string local, std::string remote) {
    intf = intf_open();
    if (!intf)
            throw TunError("Error allocating intf");
    if ((fd = open("/dev/net/tun" , O_RDWR)) < 0)
            throw TunError("Error opening /dev/net/tun");


    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;

    if (dev != "") {
        strncpy(ifr.ifr_name, dev.c_str(), IFNAMSIZ);
    }

    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        close(fd);
        throw TunError("ioctl error");
    }

    name = dev;


    struct addr a, b;
    struct intf_entry ifent;

    addr_pton( local.c_str(), &a);
    addr_pton(remote.c_str(), &b);

    memset(&ifent, 0, sizeof(ifent));
    strlcpy(ifent.intf_name, ifr.ifr_name, sizeof(ifent.intf_name));
    ifent.intf_flags = INTF_FLAG_UP|INTF_FLAG_POINTOPOINT;
    ifent.intf_addr = a;
    ifent.intf_dst_addr = b;
    ifent.intf_mtu = mtu;

    if (intf_set(intf, &ifent) < 0) {
        close(fd);
        throw TunError("intf failed");
    }

}

Tun::~Tun() {
    if (intf) {
        close(fd);
        intf_close(intf);
    }
}


std::vector<uint8_t> Tun::read() const {
    if (!intf) return {};
    char buffer[4096];
    int n = ::read(fd, buffer, 4096);
    if (n >= 0) {
        return Utils::from_ptr(buffer, n);
    } else {
        return {};
    }
}
bool Tun::write(const std::vector<uint8_t>& b) const {
    if (!intf) return false;
    return ::write(fd, b.data(), b.size()) == b.size();
}