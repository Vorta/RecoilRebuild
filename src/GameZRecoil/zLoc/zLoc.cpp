#include "GameZRecoil/zLoc/zLoc.h"

#include <string.h>

extern "C" {
HMODULE g_zLoc_MessagesDllHandle = 0;
unsigned int(*g_zLoc_GetIdProc)(const char *key) = 0;
char g_zLoc_TempMessageBuffer[0x100] = {0};
}

namespace zLoc {
// Reimplements 0x4a5ad0: zLoc::LoadMessagesDll
int __fastcall LoadMessagesDll(
    const char *dllPath
) {
    int result = 0;
    HMODULE const module = LoadLibraryA(dllPath);
    g_zLoc_MessagesDllHandle = module;
    if (module != 0) {
        result = 1;
        g_zLoc_GetIdProc =
            (unsigned int(*)(const char *))GetProcAddress(
                module,
                "ZLocGetID"
            );
    }
    return result;
}

// Reimplements 0x4a5b00: zLoc::UnloadMessagesDll
void UnloadMessagesDll() {
    HMODULE const module = g_zLoc_MessagesDllHandle;
    if (module != 0) {
        FreeLibrary(module);
    }

    g_zLoc_MessagesDllHandle = 0;
}

// Reimplements 0x4a5b20: zLoc::GetMessageId
unsigned int __fastcall GetMessageId(
    const char *key
) {
    if (g_zLoc_GetIdProc != 0) {
        return g_zLoc_GetIdProc(key);
    }

    return 0;
}

// Reimplements 0x4a5b40: zLoc::ResolveMessageKeyOrFallback
char *__fastcall ResolveMessageKeyOrFallback(
    const char *key
) {
    const unsigned int messageId = GetMessageId(key);
    if (messageId != 0) {
        return GetMessageString(messageId);
    }

    return (char *)(key);
}

// Reimplements 0x4a5b60: zLoc::FormatMessage
unsigned int FormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    ...
) {
    char *arguments = (char *)(&messageId + 1);
    HLOCAL sourceHandle = 0;
    const unsigned int result = ::FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE,
        g_zLoc_MessagesDllHandle,
        messageId,
        0,
        (LPSTR)(&sourceHandle),
        (DWORD)(maxChars),
        (va_list *)(&arguments)
    );

    char *source = (char *)(sourceHandle);
    if (source != 0) {
        if ((int)(result) > 2 && source[result - 2] == '\r') {
            *(source + result - 2) = '\0';
        }
    }

    source = (char *)(sourceHandle);
    if (source != 0) {
        strncpy(
            outBuffer,
            source,
            (size_t)(maxChars)
        );
        ::LocalFree(sourceHandle);
    }

    return result;
}

// Reimplements 0x4a5bf0: zLoc::GetMessageString
char *__fastcall GetMessageString(
    unsigned int messageId
) {
    char *message = 0;
    if (FormatMessage(
        g_zLoc_TempMessageBuffer,
        sizeof(g_zLoc_TempMessageBuffer),
        messageId
    ) != 0) {
        message = g_zLoc_TempMessageBuffer;
    }
    return message;
}
} // namespace zLoc
