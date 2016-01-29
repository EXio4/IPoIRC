#ifndef IPOIRC_B2T_H
#define IPOIRC_B2T_H
#include <string>
namespace B2T {
    std::string encode(const char *, int);
    int decode(const std::string &, char*&);
}
#endif
