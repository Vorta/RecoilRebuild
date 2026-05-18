#pragma once

#include "recoil/recoil_callconv.h"

#include "recoil/recoil_types.h"
#include <windows.h>

namespace zError {
int RECOIL_FASTCALL InitOutputContext(HWND hWnd, int maxBytes,
                                               const char *logFileName);
void RECOIL_CDECL ReportOld(int flags, const char *sourceFile, int sourceLine, const char *format,
                            ...);
RECOIL_NOINLINE void RECOIL_FASTCALL EmitDebugBuffer(int severity);
} // namespace zError

extern "C" char g_zError_DebugMsgBuffer[1024];
