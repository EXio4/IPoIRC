#ifndef IPOIRC_LTUN_H
#define IPOIRC_LTUN_H

#include "build-dnet.h"

#include <string>
#include <cstdint>


struct TunError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Tun {
private:
    int fd = -1;
    intf_t *intf; /* shall never be null */
    std::string name;
public:
    Tun(std::string dev, uint16_t mtu, std::string local, std::string remote);
    ~Tun();
    Tun(Tun const&) = delete;
    void operator=(Tun const&) = delete;
    int read(char *buf, uint16_t len) const;
    int write(const char *buf, uint16_t len) const;
};

#endif
