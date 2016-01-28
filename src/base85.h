#ifndef IPOIRC_BASE85_H
#define IPOIRC_BASE85_H
#include <string>
namespace Base85 {
    std::string encode(const char *, int);
    int decode(const std::string &, char*&);
}
#endif
