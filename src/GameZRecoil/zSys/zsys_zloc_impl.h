#include "GameZRecoil/zLoc/zloc.h"

#include <string.h>

extern "C" {
/**
 * Reimplements data 0x56b670: g_zLoc_MessagesDllHandle.
 * Reimplements data 0x56b568: g_zLoc_GetIdProc.
 * Reimplements data 0x56b570: g_zLoc_TempMessageBuffer.
 * Purpose: stores the loaded messages DLL handle, resolved ZLocGetID export,
 * and shared temporary localization message buffer.
 */
HMODULE g_zLoc_MessagesDllHandle = 0;
unsigned int(*g_zLoc_GetIdProc)(const char *key) = 0;
char g_zLoc_TempMessageBuffer[0x100] = {0};
}

namespace zLoc {
/**
 * Reimplements 0x4a5ad0: zLoc::LoadMessagesDll.
 * Purpose: Loads the localization messages DLL and resolves its ZLocGetID export.
 */
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

/**
 * Reimplements 0x4a5b00: zLoc::UnloadMessagesDll.
 * Purpose: Releases the loaded localization messages DLL and clears the cached module handle.
 */
void UnloadMessagesDll() {
    HMODULE const module = g_zLoc_MessagesDllHandle;
    if (module != 0) {
        FreeLibrary(module);
    }

    g_zLoc_MessagesDllHandle = 0;
}

/**
 * Reimplements 0x4a5b20: zLoc::GetMessageId.
 * Purpose: Looks up a localization message id through the loaded ZLocGetID export.
 */
unsigned int __fastcall GetMessageId(
    const char *key
) {
    if (g_zLoc_GetIdProc != 0) {
        return g_zLoc_GetIdProc(key);
    }

    return 0;
}

/**
 * Reimplements 0x4a5b40: zLoc::ResolveMessageKeyOrFallback.
 * Purpose: Resolves a localization key to a message string, or returns the key when lookup fails.
 */
char *__fastcall ResolveMessageKeyOrFallback(
    const char *key
) {
    const unsigned int messageId = GetMessageId(key);
    if (messageId != 0) {
        return GetMessageString(messageId);
    }

    return (char *)(key);
}

/**
 * Reimplements 0x4a5b60: zLoc::FormatMessage.
 * Purpose: Formats a message resource from the loaded DLL into a caller-provided buffer.
 */
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

/**
 * Reimplements 0x4a5bf0: zLoc::GetMessageString.
 * Purpose: Formats a message resource into the shared temporary localization buffer.
 */
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
