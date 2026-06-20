#include "Battlesport/CZGameFrame.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstring>

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

struct CFrameWndMessageMapAccess : CFrameWnd {
    static const AFX_MSGMAP *MessageMapAddress() {
        return &CFrameWnd::messageMap;
    }
};

bool MessageEntryMatches(
    const AFX_MSGMAP_ENTRY &entry,
    UINT message,
    UINT code,
    UINT id,
    UINT lastId,
    UINT sig,
    AFX_PMSG pfn
) {
    return entry.nMessage == message &&
           entry.nCode == code &&
           entry.nID == id &&
           entry.nLastID == lastId &&
           entry.nSig == sig &&
           std::memcmp(
               &entry.pfn,
               &pfn,
               sizeof(pfn)
           ) == 0;
}

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
    self->m_hWnd = hwnd;
    return hwnd != 0 ? TRUE : FALSE;
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
    typedef CObject *(PASCAL *MfcCreateObjectProc)();
    typedef CRuntimeClass *(PASCAL *MfcRuntimeClassProc)();
    typedef const AFX_MSGMAP *(PASCAL *MfcMessageMapProc)();
    const UINT updateCode = (UINT)-1;
    const UINT sigIntCreateStruct = 9;
    const UINT sigLongWparamLparam = 10;
    const UINT sigVoid = 12;
    const UINT sigVoidIntInt = 15;
    const UINT sigVoidUIntIntInt = 17;
    const UINT sigVoidUIntCWndBool = 28;
    const UINT sigCmdUi = 44;

    const CRuntimeClass *gameRuntimeClass =
        (const CRuntimeClass *)((unsigned int)CZGameFrame::GetRuntimeClass());
    const CRuntimeClass *gameBaseRuntimeClass =
        (const CRuntimeClass *)((unsigned int)CZGameFrame::GetBaseRuntimeClass());
    const AFX_MSGMAP *gameMessageMap =
        (const AFX_MSGMAP *)((unsigned int)CZGameFrame::GetMessageMap());
    const AFX_MSGMAP *gameBaseMessageMap =
        (const AFX_MSGMAP *)((unsigned int)CZGameFrame::GetBaseMessageMap());
    const CRuntimeClass *recoilRuntimeClass =
        (const CRuntimeClass *)CZRecoilFrame::GetRuntimeClass();
    const AFX_MSGMAP *recoilMessageMap =
        (const AFX_MSGMAP *)CZRecoilFrame::GetMessageMap();

    if (gameRuntimeClass != &CZGameFrame::classCZGameFrame ||
        std::strcmp(
            gameRuntimeClass->m_lpszClassName,
            "CZGameFrame"
        ) != 0 ||
        gameRuntimeClass->m_pfnCreateObject !=
            (MfcCreateObjectProc)&CZGameFrame::CreateObject ||
        gameRuntimeClass->m_pfnGetBaseClass !=
            (MfcRuntimeClassProc)&CZGameFrame::GetBaseRuntimeClass ||
        gameBaseRuntimeClass != &CFrameWnd::classCFrameWnd) {
        return 1;
    }

    if (gameMessageMap != &CZGameFrame::messageMap ||
        gameMessageMap->pfnGetBaseMap !=
            (MfcMessageMapProc)&CZGameFrame::GetBaseMessageMap ||
        gameBaseMessageMap != CFrameWndMessageMapAccess::MessageMapAddress() ||
        gameMessageMap->lpEntries != &CZGameFrame::messageEntries[0]) {
        return 2;
    }

    if (!MessageEntryMatches(
            CZGameFrame::messageEntries[0],
            WM_CLOSE,
            0,
            0,
            0,
            sigVoid,
            (AFX_PMSG)&CZGameFrame::OnClose
        ) ||
        !MessageEntryMatches(
            CZGameFrame::messageEntries[1],
            WM_PAINT,
            0,
            0,
            0,
            sigVoid,
            (AFX_PMSG)&CZGameFrame::OnPaint
        ) ||
        !MessageEntryMatches(
            CZGameFrame::messageEntries[2],
            WM_SIZE,
            0,
            0,
            0,
            sigVoidUIntIntInt,
            (AFX_PMSG)&CZGameFrame::OnSize
        ) ||
        !MessageEntryMatches(
            CZGameFrame::messageEntries[3],
            WM_MOVE,
            0,
            0,
            0,
            sigVoidIntInt,
            (AFX_PMSG)&CZGameFrame::OnMove
        ) ||
        !MessageEntryMatches(
            CZGameFrame::messageEntries[4],
            WM_CREATE,
            0,
            0,
            0,
            sigIntCreateStruct,
            (AFX_PMSG)&CZGameFrame::OnCreate
        ) ||
        !MessageEntryMatches(
            CZGameFrame::messageEntries[5],
            WM_DESTROY,
            0,
            0,
            0,
            sigVoid,
            (AFX_PMSG)&CZGameFrame::OnDestroy
        ) ||
        !MessageEntryMatches(
            CZGameFrame::messageEntries[6],
            0x3b9,
            0,
            0,
            0,
            sigLongWparamLparam,
            (AFX_PMSG)&CZGameFrame::OnAppIdleDispatchMessage
        ) ||
        !MessageEntryMatches(
            CZGameFrame::messageEntries[7],
            WM_ACTIVATE,
            0,
            0,
            0,
            sigVoidUIntCWndBool,
            (AFX_PMSG)&CZGameFrame::OnActivate
        ) ||
        !MessageEntryMatches(
            CZGameFrame::messageEntries[8],
            0,
            0,
            0,
            0,
            0,
            0
        )) {
        return 5;
    }

    if (recoilRuntimeClass != &CZRecoilFrame::classCZRecoilFrame ||
        std::strcmp(
            recoilRuntimeClass->m_lpszClassName,
            "CZRecoilFrame"
        ) != 0 ||
        recoilRuntimeClass->m_pfnCreateObject !=
            (MfcCreateObjectProc)&CZRecoilFrame::CreateObject ||
        recoilRuntimeClass->m_pfnGetBaseClass !=
            (MfcRuntimeClassProc)&CZGameFrame::GetRuntimeClass ||
        recoilRuntimeClass->m_pfnGetBaseClass() != &CZGameFrame::classCZGameFrame) {
        return 3;
    }

    if (recoilMessageMap != &CZRecoilFrame::messageMap ||
        recoilMessageMap->pfnGetBaseMap !=
            (MfcMessageMapProc)&CZGameFrame::GetMessageMap ||
        recoilMessageMap->pfnGetBaseMap() != &CZGameFrame::messageMap ||
        recoilMessageMap->lpEntries != &CZRecoilFrame::messageEntries[0]) {
        return 4;
    }

    if (!MessageEntryMatches(
            CZRecoilFrame::messageEntries[0],
            WM_COMMAND,
            0,
            0x68,
            0x68,
            sigVoid,
            (AFX_PMSG)&CZRecoilFrame::OnMenuStartSinglePlayer
        ) ||
        !MessageEntryMatches(
            CZRecoilFrame::messageEntries[12],
            WM_COMMAND,
            0,
            0x9c53,
            0x9c53,
            sigVoid,
            (AFX_PMSG)&CZRecoilFrame::OnMenuOpenMultiplayerSessionBrowser
        ) ||
        !MessageEntryMatches(
            CZRecoilFrame::messageEntries[23],
            WM_COMMAND,
            updateCode,
            0x210,
            0x210,
            sigCmdUi,
            (AFX_PMSG)&CZRecoilFrame::OnUpdateVideoMode7CmdUI
        ) ||
        !MessageEntryMatches(
            CZRecoilFrame::messageEntries[43],
            WM_COMMAND,
            updateCode,
            0x9c7f,
            0x9c7f,
            sigCmdUi,
            (AFX_PMSG)&CZRecoilFrame::OnUpdateAlwaysEnabledCmdUI
        ) ||
        !MessageEntryMatches(
            CZRecoilFrame::messageEntries[46],
            WM_COMMAND,
            updateCode,
            0x9c7e,
            0x9c7e,
            sigCmdUi,
            (AFX_PMSG)&CZRecoilFrame::OnUpdateNoOpCmdUI
        ) ||
        !MessageEntryMatches(
            CZRecoilFrame::messageEntries[52],
            WM_COMMAND,
            updateCode,
            0x9c53,
            0x9c53,
            sigCmdUi,
            (AFX_PMSG)&CZRecoilFrame::OnUpdateNoOpCmdUI
        ) ||
        !MessageEntryMatches(
            CZRecoilFrame::messageEntries[53],
            WM_SIZE,
            0,
            0,
            0,
            sigVoidUIntIntInt,
            (AFX_PMSG)&CZRecoilFrame::OnSize
        ) ||
        !MessageEntryMatches(
            CZRecoilFrame::messageEntries[54],
            0,
            0,
            0,
            0,
            0,
            0
        )) {
        return 6;
    }

    return 0;
}

extern "C" int czgame_frame_constructor_smoke(void) {
    const int ready = EnsureFrameMfcReady();
    if (ready != 0) {
        return ready;
    }

    CZGameFrame frame = {};
    CZGameFrame *returned = frame.Constructor(0);
    const RecoilPtr32 constructedFrameVtable = *(RecoilPtr32 *)(&frame);
    if (returned == &frame &&
        constructedFrameVtable != 0 &&
        constructedFrameVtable != CZGameFrame::GetRuntimeClass() &&
        frame.m_gameBitmap.m_hObject == 0) {
        frame.Destructor();
        return frame.m_gameBitmap.m_hObject == 0 ? 0 : 4;
    }

    return 3;
}

extern "C" int czgame_frame_create_object_smoke(void) {
    const int ready = EnsureFrameMfcReady();
    if (ready != 0) {
        return ready;
    }

    CZGameFrame *const frame = CZGameFrame::CreateObject();
    if (frame == 0) {
        return 3;
    }

    const RecoilPtr32 constructedFrameVtable = *(RecoilPtr32 *)(frame);
    const bool constructed =
        constructedFrameVtable != 0 &&
        constructedFrameVtable != CZGameFrame::GetRuntimeClass() &&
        frame->m_gameBitmap.m_hObject == 0;

    frame->Destructor();
    ::operator delete(frame);
    return constructed ? 0 : 4;
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

    unsigned long frameStorage
        [(sizeof(CZRecoilFrame) + sizeof(unsigned long) - 1) / sizeof(unsigned long)] = {};
    CZRecoilFrame *const frame = reinterpret_cast<CZRecoilFrame *>(frameStorage);
    CZRecoilFrame *returned = frame->Constructor();

    const RecoilPtr32 constructedFrameVtable = *(RecoilPtr32 *)(frame);
    const bool constructed =
        returned == frame &&
        constructedFrameVtable != 0 &&
        constructedFrameVtable != CZRecoilFrame::GetRuntimeClass();
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

    CZRecoilFrame *const frame = CZRecoilFrame::CreateObject();
    RestoreFunctionPatch(centerWindowPatch);
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreFunctionPatch(createExPatch);

    if (frame == 0) {
        return 4;
    }

    const RecoilPtr32 constructedFrameVtable = *(RecoilPtr32 *)(frame);
    const bool constructed =
        constructedFrameVtable != 0 &&
        constructedFrameVtable != CZRecoilFrame::GetRuntimeClass();
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

    frame->Destructor();
    ::operator delete(frame);
    return !constructed ? 5 : (!fieldsOk ? 6 : (globalsOk ? 0 : 7));
}
