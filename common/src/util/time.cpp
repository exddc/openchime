#include "oc/util/time.h"

namespace oc::util {

bool ClockIsSane(std::time_t minimum_epoch) {
    return std::time(nullptr) >= minimum_epoch;
}

} // namespace oc::util
