#pragma once

#include "recoil/recoil_callconv.h"

#include "recoil/recoil_types.h"
#include <windows.h>

namespace zError {
int __fastcall InitOutputContext(
    HWND hWnd,
    int maxBytes,
    const char *logFileName
);
void ReportOld(
    int flags,
    const char *sourceFile,
    int sourceLine,
    const char *format,
    ...
);
void __fastcall EmitDebugBuffer(int severity);
} // namespace zError

extern "C" char g_zError_DebugMsgBuffer[1024];
