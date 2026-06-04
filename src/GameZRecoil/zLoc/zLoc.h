#pragma once

#include "recoil/recoil_types.h"

#include <windows.h>

#ifdef FormatMessage
#undef FormatMessage
#endif

#include "recoil/recoil_callconv.h"

extern "C" {
extern HMODULE g_zLoc_MessagesDllHandle;
extern unsigned int(*g_zLoc_GetIdProc)(const char *key);
extern char g_zLoc_TempMessageBuffer[0x100];
}

namespace zLoc {
int __fastcall LoadMessagesDll(const char *dllPath);
void UnloadMessagesDll();
unsigned int __fastcall GetMessageId(const char *key);
char *__fastcall ResolveMessageKeyOrFallback(const char *key);
unsigned int FormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    ...
);
char *__fastcall GetMessageString(unsigned int messageId);
} // namespace zLoc
