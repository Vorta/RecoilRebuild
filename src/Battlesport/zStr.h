#pragma once

#include "recoil/recoil_callconv.h"

namespace zStr {
RECOIL_NOINLINE int RECOIL_FASTCALL ContainsCaseInsensitive(const char *haystack,
                                                           const char *needle);
}
