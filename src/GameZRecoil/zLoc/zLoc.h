#pragma once

#include "recoil/recoil_types.h"

#include <windows.h>

#ifdef FormatMessage
#undef FormatMessage
#endif

#include "recoil/recoil_callconv.h"

extern "C" {
extern HMODULE g_zLoc_MessagesDllHandle;
extern unsigned int(RECOIL_CDECL *g_zLoc_GetIdProc)(const char *key);
extern char g_zLoc_TempMessageBuffer[0x100];
}

namespace zLoc {
RECOIL_NOINLINE int RECOIL_FASTCALL LoadMessagesDll(const char *dllPath);
RECOIL_NOINLINE void RECOIL_CDECL UnloadMessagesDll();
RECOIL_NOINLINE unsigned int RECOIL_FASTCALL GetMessageId(const char *key);
RECOIL_NOINLINE char *RECOIL_FASTCALL ResolveMessageKeyOrFallback(const char *key);
RECOIL_NOINLINE unsigned int RECOIL_CDECL FormatMessage(char *outBuffer, int maxChars,
                                                         unsigned int messageId, ...);
RECOIL_NOINLINE char *RECOIL_FASTCALL GetMessageString(unsigned int messageId);
} // namespace zLoc
