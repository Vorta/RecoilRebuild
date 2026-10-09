#pragma once

#include "recoil/recoil_callconv.h"

#include "recoil/recoil_types.h"
#include <windows.h>

/*
 * zerr_old.c is a C unit, so these functions have C linkage; C++ consumers
 * keep the reconstruction's zError namespace view of them.
 */
#ifdef __cplusplus
namespace zError {
extern "C" {
#endif
int __fastcall InitOutputContext(void* hWnd, int maxBytes, const char* logFileName);
void __cdecl ReportOld(int flags, const char* sourceFile, int sourceLine, const char* format, ...);
void __fastcall EmitDebugBuffer(int severity);
#ifdef __cplusplus
}
} // namespace zError
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern char g_zError_DebugMsgBuffer[1024];
#ifdef __cplusplus
}
#endif
