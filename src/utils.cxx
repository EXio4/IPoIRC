#include "utils.h"

namespace Utils {

        std::vector<uint8_t> from_ptr(void* ptr, int n) {
                return std::vector<uint8_t>((uint8_t*)ptr, ((uint8_t*)ptr)+n);
        }
}