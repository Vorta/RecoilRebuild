#define DIRECTINPUT_VERSION 0x0500
#if defined(_MSC_VER)
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 0
#endif

#include "GameZRecoil/zSys/zsys.h"

#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <windows.h>

namespace {

/**
 * Reimplements data 0x56b438: g_zSys_DriveTypeSearchPathBuffer.
 * Purpose: stores the candidate drive path returned by FindFileOnDriveType.
 */
char g_zSys_DriveTypeSearchPathBuffer[MAX_PATH];

} // namespace

/**
 * Reimplements 0x4a5980: zSys::ExitProcessWithCleanup.
 * Purpose: Runs shutdown cleanup hooks, closes CRT streams, and terminates the process.
 * Retail keeps VC5's unreachable pop/ret epilogue after the noreturn ExitProcess import.
 */
void __fastcall zSys::ExitProcessWithCleanup(
    int exitCode
) {
    zGame::ReturnOnlyStub();
    _fcloseall();
    ExitProcess((UINT)(exitCode));
#if defined(_MSC_VER) && _MSC_VER >= 1300
    __assume(0);
#endif
}

/**
 * Operational definition of zVid::SetCachedClientRectUpdateMask.
 * Purpose: store the client-rect update mask used by cached rect refresh helpers.
 *
 * Data evidence: BN stores the fastcall mask argument into the zero-initialized
 * g_zVid_CachedClientRectUpdateMask int32 global at 0x56b564.
 */
void __fastcall zVid::SetCachedClientRectUpdateMask(
    int mask
) {
    g_zVid_CachedClientRectUpdateMask = mask;
}

/**
 * Operational definition of zVid::QueryCachedClientRectUpdateMaskIf3dfx.
 * Purpose: return the cached client-rect update mask unless the path-2
 * renderer is active.
 *
 * Data evidence: BN reads g_zVideo_ActiveRendererPath at 0x56bbe8 and
 * g_zVid_CachedClientRectUpdateMask at 0x56b564; the branchless predicate
 * subtracts renderer path 2, negates it, and uses sbb as a nonzero mask.
 */
int zVid::QueryCachedClientRectUpdateMaskIf3dfx() {
    if (g_zVideo_ActiveRendererPath != 2) {
        return g_zVid_CachedClientRectUpdateMask;
    }
    return 0;
}

/**
 * Reimplements 0x4a59e0: zSys::FindFileOnDriveType.
 * Purpose: Scans logical drives of a requested type and returns the first path containing a file.
 */
RECOIL_NO_GS char *__fastcall zSys::FindFileOnDriveType(
    int driveType,
    const char *relativePath,
    int
) {
    enum {
        kLogicalDriveStringsReadLimit = 256,
        kLogicalDriveStringsBufferSize = 300
    };
    char driveStrings[kLogicalDriveStringsBufferSize];
    const char *searchPath;
    struct _stat statBuffer;
    searchPath = relativePath;
    GetLogicalDriveStringsA(
        kLogicalDriveStringsReadLimit,
        driveStrings
    );

    int driveListOffset = 0;
    int found = 0;
    while (1) {
        const char *drive = &driveStrings[driveListOffset];
        sprintf(
            g_zSys_DriveTypeSearchPathBuffer,
            "%s%s",
            drive,
            searchPath
        );
        switch (GetDriveTypeA(drive)) {
        case DRIVE_FIXED:
            if (driveType == DRIVE_FIXED) {
                if (_stat(
                    g_zSys_DriveTypeSearchPathBuffer,
                    &statBuffer
                ) == 0) {
                    found = 1;
                }
            }
            break;

        case DRIVE_CDROM:
            if (driveType == DRIVE_CDROM) {
                if (_stat(
                    g_zSys_DriveTypeSearchPathBuffer,
                    &statBuffer
                ) == 0) {
                    found = 1;
                }
            }
            break;
        }

        if (found != 0) {
            return g_zSys_DriveTypeSearchPathBuffer;
        }

        if (strlen(drive) == 0) {
            break;
        }

        driveListOffset += (int)(strlen(drive) + 1);
    }

    return 0;
}

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
