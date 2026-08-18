#ifndef OC_UTIL_TIME_H
#define OC_UTIL_TIME_H

#include <ctime>

namespace oc::util {

bool ClockIsSane(std::time_t minimum_epoch);

} // namespace oc::util

#endif
