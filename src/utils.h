#ifndef IPOIRC_UTILS_H
#define IPOIRC_UTILS_H

#include <vector>
#include <stdint.h>

namespace Utils {

    std::vector<uint8_t> from_ptr(void*, int);

}
#endif