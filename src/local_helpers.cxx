#include "ipoirc.h"
#include "local_helpers.h"

std::ostream& loc_debug() {
    return debug_gen("local");
}