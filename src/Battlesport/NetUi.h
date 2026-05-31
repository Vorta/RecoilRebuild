#pragma once

#include "recoil/recoil_callconv.h"

namespace NetUi {
RECOIL_NOINLINE int RECOIL_FASTCALL
VerifyWinsock2OrPromptContinue(const char *caption, const char *messageFormat);
}
