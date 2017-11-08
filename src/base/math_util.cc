#include "base/math_util.h"
#include <cassert>

namespace base {

int GetDigits(int number) {
  assert(number >= 0);

  int digits = 0;

  do {
    ++digits;
    number /= 10;
  } while (number > 0);

  return digits;
}

}  // namespace base
