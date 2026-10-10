#pragma once

#include "recoil/recoil_types.h"

#include <windows.h>

#ifdef FormatMessage
#undef FormatMessage
#endif

#include "recoil/recoil_callconv.h"

#ifdef __cplusplus
extern "C" {
extern HMODULE g_zLoc_MessagesDllHandle;
extern unsigned int(__cdecl* g_zLoc_GetIdProc)(const char* key);
extern char g_zLoc_TempMessageBuffer[0x100];
}

namespace zLoc {
int __fastcall LoadMessagesDll(const char* dllPath);
void __cdecl UnloadMessagesDll();
extern "C" unsigned int __fastcall GetMessageId(const char* key);
extern "C" char* __fastcall ResolveMessageKeyOrFallback(const char* key);
unsigned int __cdecl FormatMessage(char* outBuffer, int maxChars, unsigned int messageId, ...);
extern "C" char* __fastcall GetMessageString(unsigned int messageId);
} // namespace zLoc
#else
/* C units' view of the message lookups they call (zsys.cpp defines them). */
unsigned int __fastcall GetMessageId(const char* key);
char* __fastcall GetMessageString(unsigned int messageId);
#endif
