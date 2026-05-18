#include "GameZRecoil/zLoc/zLoc.h"

#include <string.h>

extern "C" {
HMODULE g_zLoc_MessagesDllHandle = 0;
unsigned int(RECOIL_CDECL *g_zLoc_GetIdProc)(const char *key) = 0;
char g_zLoc_TempMessageBuffer[0x100] = {0};
}

namespace zLoc {
// Reimplements 0x4a5ad0: zLoc::LoadMessagesDll
RECOIL_NOINLINE int RECOIL_FASTCALL LoadMessagesDll(const char *dllPath) {
    int result = 0;
    HMODULE const module = LoadLibraryA(dllPath);
    g_zLoc_MessagesDllHandle = module;
    if (module != 0) {
        result = 1;
        g_zLoc_GetIdProc =
            (unsigned int(RECOIL_CDECL *)(const char *))GetProcAddress(module, "ZLocGetID");
    }
    return result;
}

// Reimplements 0x4a5b00: zLoc::UnloadMessagesDll
RECOIL_NOINLINE void RECOIL_CDECL UnloadMessagesDll() {
    HMODULE const module = g_zLoc_MessagesDllHandle;
    if (module != 0) {
        FreeLibrary(module);
    }

    g_zLoc_MessagesDllHandle = 0;
}

// Reimplements 0x4a5b20: zLoc::GetMessageId
RECOIL_NOINLINE unsigned int RECOIL_FASTCALL GetMessageId(const char *key) {
    if (g_zLoc_GetIdProc != 0) {
        return g_zLoc_GetIdProc(key);
    }

    return 0;
}

// Reimplements 0x4a5b40: zLoc::ResolveMessageKeyOrFallback
RECOIL_NOINLINE char *RECOIL_FASTCALL ResolveMessageKeyOrFallback(const char *key) {
    const unsigned int messageId = GetMessageId(key);
    if (messageId != 0) {
        return GetMessageString(messageId);
    }

    return const_cast<char *>(key);
}

// Reimplements 0x4a5b60: zLoc::FormatMessage
RECOIL_NOINLINE unsigned int RECOIL_CDECL FormatMessage(char *outBuffer, int maxChars,
                                                         unsigned int messageId, ...) {
    char *arguments = reinterpret_cast<char *>(&messageId + 1);
    HLOCAL sourceHandle = 0;
    const unsigned int result = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE, g_zLoc_MessagesDllHandle,
        messageId, 0, reinterpret_cast<LPSTR>(&sourceHandle), static_cast<DWORD>(maxChars),
        reinterpret_cast<va_list *>(&arguments));

    char *source = static_cast<char *>(sourceHandle);
    if (source != 0) {
        if (static_cast<int>(result) > 2 && source[result - 2] == '\r') {
            *(source + result - 2) = '\0';
        }
    }

    source = static_cast<char *>(sourceHandle);
    if (source != 0) {
        strncpy(outBuffer, source, static_cast<size_t>(maxChars));
        ::LocalFree(sourceHandle);
    }

    return result;
}

// Reimplements 0x4a5bf0: zLoc::GetMessageString
RECOIL_NOINLINE char *RECOIL_FASTCALL GetMessageString(unsigned int messageId) {
    char *message = 0;
    if (FormatMessage(g_zLoc_TempMessageBuffer, sizeof(g_zLoc_TempMessageBuffer), messageId) != 0) {
        message = g_zLoc_TempMessageBuffer;
    }
    return message;
}
} // namespace zLoc
