#include "oc/util/platform.h"

namespace oc::util {

bool IsLinux() {
#ifdef __linux__
  return true;
#else
  return false;
#endif
}

}  // namespace oc::util
