#include "GameZRecoil/zError/zError.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

extern "C" HWND g_RecoilError_OutputHWnd = 0;
extern "C" int g_RecoilError_OutputMaxBytes = 0;
extern "C" int g_RecoilError_OutputByteCount = 0;
extern "C" char g_zError_DebugMsgBuffer[1024] = {0};

namespace zError {
    const int kReportOldMessageBytes = 1024;
    const int kReportOldOutputBytes = 1280;
    const char kReportOldUnknownSource[] = "(unknown source)";
    const char kReportOldNullMessage[] = "(null message)";

    // Reimplements 0x462310: RecoilError::InitOutputContext
    RECOIL_NOINLINE int RECOIL_FASTCALL InitOutputContext(
        HWND hWnd,
        int maxBytes,
        const char *
    ){
        g_RecoilError_OutputByteCount = 0;
        g_RecoilError_OutputMaxBytes = maxBytes;
        g_RecoilError_OutputHWnd = hWnd;
        return 0;
    }

    // Reimplements 0x404e80: zError::ReportOld (GameZRecoil/zError/zerr_old.c)
    RECOIL_NOINLINE void RECOIL_CDECL ReportOld(
        int flags,
        const char *sourceFile,
        int sourceLine,
        const char *format,
        ...
    ) {
        // Retail 0x404e80 is a single retn. This body is deliberate
        // non-retail debug instrumentation for the reconstructed engine.
        char message[kReportOldMessageBytes];
        char output[kReportOldOutputBytes];
        va_list args;

        if (sourceFile == 0) {
            sourceFile = kReportOldUnknownSource;
        }
        if (format == 0) {
            format = kReportOldNullMessage;
        }

        va_start(
            args,
            format
        );
        _vsnprintf(
            message,
            sizeof(message),
            format,
            args
        );
        va_end(args);
        message[sizeof(message) - 1] = '\0';

        _snprintf(
            output,
            sizeof(output),
            "zError::ReportOld flags=0x%08x %s:%d: %s\r\n",
            flags,
            sourceFile,
            sourceLine,
            message
        );
        output[sizeof(output) - 1] = '\0';

        g_RecoilError_OutputByteCount += (int)strlen(output);
        OutputDebugStringA(output);
    }

    // Reimplements 0x4622f0: zError::EmitDebugBuffer
    RECOIL_NOINLINE void RECOIL_FASTCALL EmitDebugBuffer(int severity) {
        ReportOld(
            severity,
            "D:\\Proj\\GameZRecoil\\zError\\zerr_old.c",
            0x23,
            g_zError_DebugMsgBuffer
        );
    }
}
