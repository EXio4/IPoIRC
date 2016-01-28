#include "ipoirc.h"
#include "tun_helpers.h"

std::ostream& tun_debug() {
    return debug_gen("tun");
}