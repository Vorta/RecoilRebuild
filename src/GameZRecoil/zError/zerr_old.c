#include "GameZRecoil/zError/zError.h"

extern "C" HWND g_RecoilError_OutputHWnd = 0;
extern "C" int g_RecoilError_OutputMaxBytes = 0;
extern "C" int g_RecoilError_OutputByteCount = 0;
extern "C" char g_zError_DebugMsgBuffer[1024] = {0};

namespace zError {
    /**
     * Reimplements 0x462310: RecoilError::InitOutputContext.
     * Purpose: Resets the legacy error-output counters and stores the target output window.
     */
    int __fastcall InitOutputContext(
        HWND hWnd,
        int maxBytes,
        const char *
    ){
        g_RecoilError_OutputByteCount = 0;
        g_RecoilError_OutputMaxBytes = maxBytes;
        g_RecoilError_OutputHWnd = hWnd;
        return 0;
    }

    /**
     * Reimplements 0x404e80: zError::ReportOldNoOp.
     * Purpose: Preserves the stripped retail legacy-report call ABI without producing output.
     */
    void ReportOld(
        int,
        const char *,
        int,
        const char *,
        ...
    ) {}

    /**
     * Reimplements 0x4622f0: zError::EmitDebugBuffer.
     * Purpose: Forwards the shared debug buffer through the stripped legacy report stub.
     */
    void __fastcall EmitDebugBuffer(int severity) {
        ReportOld(
            severity,
            "D:\\Proj\\GameZRecoil\\zError\\zerr_old.c",
            0x23,
            g_zError_DebugMsgBuffer
        );
    }
}
