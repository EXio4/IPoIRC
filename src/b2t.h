#ifndef IPOIRC_B2T_H
#define IPOIRC_B2T_H
#include <string>
#include <vector>
#include <boost/optional.hpp>
namespace B2T {
    std::string encode(const std::vector<uint8_t>&);
    boost::optional<std::vector<uint8_t>> decode(const std::string &const_msg);
}
#endif
