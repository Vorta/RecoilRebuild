#include "CZGameFrame/CZGameFrame.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/recoil_app.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstring>
#include <new>

extern "C" int g_CZRecoilFrame_HasWolApi;
BOOL __stdcall AfxWinInit(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    LPSTR commandLine,
    int showCommand
);

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

int g_cWndCreateExCalls;
int g_cWndSetWindowTextCalls;
int g_cWndCenterWindowCalls;

struct CWndCreateExAccess : CWnd {
    using CWnd::CreateEx;
};

struct CWndSetWindowTextAccess : CWnd {
    using CWnd::SetWindowText;
};

struct CWndCenterWindowAccess : CWnd {
    using CWnd::CenterWindow;
};

BOOL __fastcall FakeCWndCreateEx(
    CWnd *self,
    void *,
    DWORD exStyle,
    LPCSTR className,
    LPCSTR windowName,
    DWORD style,
    int x,
    int y,
    int width,
    int height,
    HWND parent,
    HMENU menu,
    LPVOID param
) {
    ++g_cWndCreateExCalls;
    HWND hwnd = CreateWindowExA(
        exStyle,
        className,
        windowName,
        style,
        x,
        y,
        width,
        height,
        parent,
        menu,
        GetModuleHandleA(0),
        param
    );
    if (hwnd == 0) {
        return FALSE;
    }

    return self->Attach(hwnd);
}

void __fastcall FakeCWndSetWindowTextA(
    CWnd *self,
    void *,
    LPCSTR text
) {
    ++g_cWndSetWindowTextCalls;
    if (self->m_hWnd != 0) {
        SetWindowTextA(
            self->m_hWnd,
            text
        );
    }
}

void __fastcall FakeCWndCenterWindow(
    CWnd *,
    void *,
    CWnd *
) {
    ++g_cWndCenterWindowCalls;
}

void *CWndCreateExProc() {
    union MemberToFunction {
        BOOL ( CWndCreateExAccess::*member)(
            DWORD,
            LPCSTR,
            LPCSTR,
            DWORD,
            int,
            int,
            int,
            int,
            HWND,
            HMENU,
            LPVOID
        );
        void *function;
    };

    MemberToFunction thunk = {};
    thunk.member = &CWndCreateExAccess::CreateEx;
    return thunk.function;
}

void *CWndSetWindowTextAProc() {
    union MemberToFunction {
        void ( CWndSetWindowTextAccess::*member)(LPCSTR);
        void *function;
    };

    MemberToFunction thunk = {};
    thunk.member = &CWndSetWindowTextAccess::SetWindowText;
    return thunk.function;
}

void *CWndCenterWindowProc() {
    union MemberToFunction {
        void ( CWndCenterWindowAccess::*member)(CWnd *);
        void *function;
    };

    MemberToFunction thunk = {};
    thunk.member = &CWndCenterWindowAccess::CenterWindow;
    return thunk.function;
}

int CallGameFrameDestructorWithSeh(
    CZGameFrame *frame
) {
    __try {
        frame->~CZGameFrame();
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 1;
    }

    return 0;
}

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(
        patch.original,
        patch.address,
        sizeof(patch.original)
    );

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        return false;
    }

    patch.address[0] = 0xe9;
    const intptr_t delta =
        reinterpret_cast<unsigned char *>(replacement) -
        (patch.address + sizeof(patch.original));
    const int relative = static_cast<int>(delta);
    std::memcpy(
        patch.address + 1,
        &relative,
        sizeof(relative)
    );
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );

    DWORD ignored = 0;
    VirtualProtect(
        patch.address,
        sizeof(patch.original),
        oldProtect,
        &ignored
    );
    return true;
}

void RestoreFunctionPatch(
    CodeFunctionPatch &patch
) {
    if (patch.address == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(
            patch.address,
            patch.original,
            sizeof(patch.original)
        );
        FlushInstructionCache(
            GetCurrentProcess(),
            patch.address,
            sizeof(patch.original)
        );
        DWORD ignored = 0;
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            oldProtect,
            &ignored
        );
    }

    patch.address = 0;
}

int EnsureFrameMfcReady() {
    HINSTANCE instance = GetModuleHandleA(0);
    if (AfxWinInit(
            instance,
            0,
            GetCommandLineA(),
            SW_HIDE
        ) == 0) {
        return 1;
    }

    WNDCLASSA wndClass = {};
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "RecoilClass";
    if (RegisterClassA(&wndClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return 2;
    }

    return 0;
}

bool PatchFrameCWndMethods(
    CodeFunctionPatch &createExPatch,
    CodeFunctionPatch &setWindowTextPatch,
    CodeFunctionPatch &centerWindowPatch
) {
    if (!PatchFunctionJump(
            CWndCreateExProc(),
            reinterpret_cast<void *>(&FakeCWndCreateEx),
            createExPatch
        ) ||
        !PatchFunctionJump(
            CWndSetWindowTextAProc(),
            reinterpret_cast<void *>(&FakeCWndSetWindowTextA),
            setWindowTextPatch
        ) ||
        !PatchFunctionJump(
            CWndCenterWindowProc(),
            reinterpret_cast<void *>(&FakeCWndCenterWindow),
            centerWindowPatch
        )) {
        RestoreFunctionPatch(centerWindowPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreFunctionPatch(createExPatch);
        return false;
    }

    return true;
}
} // namespace

extern "C" int czframe_metadata_accessors_smoke(void) {
    const CRuntimeClass *const gameRuntimeClass = RUNTIME_CLASS(CZGameFrame);
    const CRuntimeClass *const recoilRuntimeClass = RUNTIME_CLASS(CZRecoilFrame);

    if (gameRuntimeClass == 0 ||
        strcmp(
            gameRuntimeClass->m_lpszClassName,
            "CZGameFrame"
        ) != 0 ||
        !gameRuntimeClass->IsDerivedFrom(RUNTIME_CLASS(CFrameWnd))) {
        return 1;
    }

    if (recoilRuntimeClass == 0 ||
        strcmp(
            recoilRuntimeClass->m_lpszClassName,
            "CZRecoilFrame"
        ) != 0 ||
        !recoilRuntimeClass->IsDerivedFrom(gameRuntimeClass)) {
        return 2;
    }

    return 0;
}

extern "C" int czgame_frame_constructor_smoke(void) {
    const int ready = EnsureFrameMfcReady();
    if (ready != 0) {
        return ready;
    }

    alignas(CZGameFrame) unsigned char frameStorage[sizeof(CZGameFrame)] = {};
    CZGameFrame *const frame = reinterpret_cast<CZGameFrame *>(frameStorage);
    CZGameFrame *returned = new (frame) CZGameFrame(0);
    if (returned == frame &&
        returned->GetRuntimeClass() == RUNTIME_CLASS(CZGameFrame) &&
        frame->m_gameBitmap.m_hObject == 0) {
        frame->~CZGameFrame();
        return frame->m_gameBitmap.m_hObject == 0 ? 0 : 4;
    }

    return 3;
}

extern "C" int czgame_frame_create_object_smoke(void) {
    const int ready = EnsureFrameMfcReady();
    if (ready != 0) {
        return ready;
    }

    CObject *const object = CZGameFrame::CreateObject();
    if (object == 0) {
        return 3;
    }
    CZGameFrame *const frame = static_cast<CZGameFrame *>(object);

    const bool constructed =
        frame->GetRuntimeClass() == RUNTIME_CLASS(CZGameFrame) &&
        frame->m_gameBitmap.m_hObject == 0;

    delete frame;
    return constructed ? 0 : 4;
}

extern "C" int czgame_frame_destructor_smoke(void) {
    const int ready = EnsureFrameMfcReady();
    if (ready != 0) {
        return ready;
    }

    HBITMAP bitmap = CreateBitmap(
        1,
        1,
        1,
        1,
        0
    );
    if (bitmap == 0) {
        return 3;
    }

    alignas(CZGameFrame) unsigned char frameStorage[sizeof(CZGameFrame)] = {};
    CZGameFrame *const frame = reinterpret_cast<CZGameFrame *>(frameStorage);
    new (frame) CZGameFrame();
    frame->m_gameBitmap.Attach(bitmap);
    if (CallGameFrameDestructorWithSeh(frame) != 0) {
        ::DeleteObject(bitmap);
        return 4;
    }

    return frame->m_gameBitmap.m_hObject == 0 ? 0 : 5;
}

extern "C" int czgame_frame_is_window_valid_smoke(void) {
    const int ready = EnsureFrameMfcReady();
    if (ready != 0) {
        return ready;
    }

    CZGameFrame frame = {};
    if (frame.IsWindowValid(0) != 0) {
        return 3;
    }

    HWND hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil-validity-test",
        WS_OVERLAPPEDWINDOW | WS_DISABLED,
        0,
        0,
        10,
        10,
        0,
        0,
        GetModuleHandleA(0),
        0
    );
    if (hwnd == 0) {
        return 4;
    }

    unsigned long wndStorage[(sizeof(CWnd) + sizeof(unsigned long) - 1) / sizeof(unsigned long)] = {};
    CWnd *const disabledWnd = reinterpret_cast<CWnd *>(wndStorage);
    disabledWnd->m_hWnd = hwnd;
    if (frame.IsWindowValid(disabledWnd) != 1) {
        DestroyWindow(hwnd);
        return 5;
    }

    EnableWindow(
        hwnd,
        TRUE
    );
    const int result = frame.IsWindowValid(disabledWnd) == 0 ? 0 : 6;
    DestroyWindow(hwnd);
    return result;
}

extern "C" int czrecoil_frame_constructor_smoke(void) {
    const int ready = EnsureFrameMfcReady();
    if (ready != 0) {
        return ready;
    }

    CodeFunctionPatch createExPatch = {};
    CodeFunctionPatch setWindowTextPatch = {};
    CodeFunctionPatch centerWindowPatch = {};
    if (!PatchFrameCWndMethods(
            createExPatch,
            setWindowTextPatch,
            centerWindowPatch
        )) {
        return 3;
    }

    g_cWndCreateExCalls = 0;
    g_cWndSetWindowTextCalls = 0;
    g_cWndCenterWindowCalls = 0;
    g_zVid_AcceptedHardwareRendererCount = 5;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    alignas(CZRecoilFrame) unsigned char frameStorage[sizeof(CZRecoilFrame)] = {};
    CZRecoilFrame *const frame = reinterpret_cast<CZRecoilFrame *>(frameStorage);
    CZRecoilFrame *returned = new (frame) CZRecoilFrame();

    const bool constructed =
        returned == frame &&
        returned->GetRuntimeClass() == RUNTIME_CLASS(CZRecoilFrame);
    const bool fieldsOk =
        frame->m_openZbdFilePath[0] == '\0' &&
        frame->m_useArchiveBanks == 1 &&
        frame->m_cmdlineFlag == 1 &&
        frame->m_campaignsOnlyMode == 0 &&
        frame->m_acceptedD3DDeviceCount == g_zVid_AcceptedHardwareRendererCount &&
        frame->m_hwApiCmdUiState[0] == 0 &&
        frame->m_hwApiCmdUiState[1] == 0 &&
        frame->m_hwApiCmdUiState[2] == 0 &&
        frame->m_hwApiCmdUiState[3] == 0 &&
        frame->m_hwApiMenuCommandIds[0] == 0x9c83 &&
        frame->m_hwApiMenuCommandIds[1] == 0x9c72 &&
        frame->m_hwApiMenuCommandIds[2] == 0x9c75 &&
        frame->m_hwApiMenuCommandIds[3] == 0x9c76;
    const bool globalsOk =
        g_zSnd_UseArchiveBanksFlag == 1 &&
        g_cWndCreateExCalls == 1 &&
        g_cWndSetWindowTextCalls == 1 &&
        g_cWndCenterWindowCalls == 1;

    frame->~CZRecoilFrame();
    RestoreFunctionPatch(centerWindowPatch);
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreFunctionPatch(createExPatch);

    return !constructed ? 4 : (!fieldsOk ? 5 : (globalsOk ? 0 : 6));
}

extern "C" int czrecoil_frame_create_object_smoke(void) {
    const int ready = EnsureFrameMfcReady();
    if (ready != 0) {
        return ready;
    }

    CodeFunctionPatch createExPatch = {};
    CodeFunctionPatch setWindowTextPatch = {};
    CodeFunctionPatch centerWindowPatch = {};
    if (!PatchFrameCWndMethods(
            createExPatch,
            setWindowTextPatch,
            centerWindowPatch
        )) {
        return 3;
    }

    g_cWndCreateExCalls = 0;
    g_cWndSetWindowTextCalls = 0;
    g_cWndCenterWindowCalls = 0;
    g_zVid_AcceptedHardwareRendererCount = 5;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    CObject *const object = CZRecoilFrame::CreateObject();
    if (object == 0) {
        RestoreFunctionPatch(centerWindowPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreFunctionPatch(createExPatch);
        return 4;
    }
    CZRecoilFrame *const frame = static_cast<CZRecoilFrame *>(object);

    const bool constructed =
        frame->GetRuntimeClass() == RUNTIME_CLASS(CZRecoilFrame);
    const bool fieldsOk =
        frame->m_openZbdFilePath[0] == '\0' &&
        frame->m_useArchiveBanks == 1 &&
        frame->m_cmdlineFlag == 1 &&
        frame->m_acceptedD3DDeviceCount == g_zVid_AcceptedHardwareRendererCount;
    const bool globalsOk =
        g_zSnd_UseArchiveBanksFlag == 1 &&
        g_cWndCreateExCalls == 1 &&
        g_cWndSetWindowTextCalls == 1 &&
        g_cWndCenterWindowCalls == 1;

    delete frame;
    RestoreFunctionPatch(centerWindowPatch);
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreFunctionPatch(createExPatch);
    return !constructed ? 5 : (!fieldsOk ? 6 : (globalsOk ? 0 : 7));
}
