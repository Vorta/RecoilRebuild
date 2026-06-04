#pragma once

#include "recoil/recoil_callconv.h"

namespace NetUi {
int __fastcall VerifyWinsock2OrPromptContinue(
    const char *caption,
    const char *messageFormat
);
}
