#pragma once

#include "recoil/recoil_types.h"

#include <windows.h>

#ifdef FormatMessage
#undef FormatMessage
#endif

#include "recoil/recoil_callconv.h"

extern "C" {
extern HMODULE g_zLoc_MessagesDllHandle;
extern unsigned int(__cdecl *g_zLoc_GetIdProc)(const char *key);
extern char g_zLoc_TempMessageBuffer[0x100];
}

namespace zLoc {
int __fastcall LoadMessagesDll(const char *dllPath);
void __cdecl UnloadMessagesDll();
unsigned int __fastcall GetMessageId(const char *key);
char *__fastcall ResolveMessageKeyOrFallback(const char *key);
unsigned int __cdecl FormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    ...
);
char *__fastcall GetMessageString(unsigned int messageId);
} // namespace zLoc
