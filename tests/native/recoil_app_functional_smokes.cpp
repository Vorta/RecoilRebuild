#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/recoil_app.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" HWND g_RecoilApp_hWndMain;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern const char *g_RecoilApp_WndClassNamePtr;
extern "C" int g_CZRecoilFrame_HasWolApi;

BOOL __stdcall AfxWinInit(
    HINSTANCE instance,
    HINSTANCE previousInstance,
    LPSTR commandLine,
    int showCommand
);

namespace {
void AtexitProviderNoOp() {
}

struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;

    ImportFunctionPatch() : slot(0), original(0) {
    }

    ~ImportFunctionPatch() {
        Restore();
    }

    void Restore() {
        if (slot == 0) {
            return;
        }

        DWORD oldProtect = 0;
        if (VirtualProtect(
                slot,
                sizeof(*slot),
                PAGE_EXECUTE_READWRITE,
                &oldProtect
            ) != 0) {
            *slot = original;
            DWORD ignored = 0;
            VirtualProtect(
                slot,
                sizeof(*slot),
                oldProtect,
                &ignored
            );
            FlushInstructionCache(
                GetCurrentProcess(),
                slot,
                sizeof(*slot)
            );
        }

        slot = 0;
        original = 0;
    }
};

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];

    CodeFunctionPatch() : address(0), original{} {
    }

    ~CodeFunctionPatch() {
        Restore();
    }

    void Restore() {
        if (address == 0) {
            return;
        }

        DWORD oldProtect = 0;
        if (VirtualProtect(
                address,
                sizeof(original),
                PAGE_EXECUTE_READWRITE,
                &oldProtect
            ) != 0) {
            std::memcpy(
                address,
                original,
                sizeof(original)
            );
            DWORD ignored = 0;
            VirtualProtect(
                address,
                sizeof(original),
                oldProtect,
                &ignored
            );
            FlushInstructionCache(
                GetCurrentProcess(),
                address,
                sizeof(original)
            );
        }

        address = 0;
    }
};

bool PatchImportByName(
    const char *dllName,
    const char *functionName,
    void *replacement,
    ImportFunctionPatch &patch
) {
    unsigned char *const imageBase =
        reinterpret_cast<unsigned char *>(GetModuleHandleA(0));
    IMAGE_DOS_HEADER *const dos =
        reinterpret_cast<IMAGE_DOS_HEADER *>(imageBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt =
        reinterpret_cast<IMAGE_NT_HEADERS *>(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(
            imageBase + imports.VirtualAddress
        );
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll =
            reinterpret_cast<const char *>(imageBase + descriptor->Name);
        if (_stricmp(
                importedDll,
                dllName
            ) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *nameThunk =
            reinterpret_cast<IMAGE_THUNK_DATA *>(
                imageBase +
                (descriptor->OriginalFirstThunk != 0
                     ? descriptor->OriginalFirstThunk
                     : descriptor->FirstThunk)
            );
        IMAGE_THUNK_DATA *addressThunk =
            reinterpret_cast<IMAGE_THUNK_DATA *>(
                imageBase + descriptor->FirstThunk
            );
        for (; nameThunk->u1.AddressOfData != 0;
             ++nameThunk, ++addressThunk) {
            if (IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)) {
                continue;
            }

            IMAGE_IMPORT_BY_NAME *const importName =
                reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(
                    imageBase + nameThunk->u1.AddressOfData
                );
            if (std::strcmp(
                    reinterpret_cast<const char *>(importName->Name),
                    functionName
                ) != 0) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0) {
                patch.slot = 0;
                patch.original = 0;
                return false;
            }

            *patch.slot = static_cast<ULONG_PTR>(
                reinterpret_cast<std::uintptr_t>(replacement)
            );
            DWORD ignored = 0;
            VirtualProtect(
                patch.slot,
                sizeof(*patch.slot),
                oldProtect,
                &ignored
            );
            FlushInstructionCache(
                GetCurrentProcess(),
                patch.slot,
                sizeof(*patch.slot)
            );
            return true;
        }
    }

    return false;
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
        patch.address = 0;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(
            patch.address + sizeof(patch.original)
        );
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(
        patch.address,
        sizeof(patch.original),
        oldProtect,
        &ignored
    );
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );
    return true;
}

struct CWndCreateExAccess : CWnd {
    using CWnd::CreateEx;
};

struct CWndSetWindowTextAccess : CWnd {
    using CWnd::SetWindowText;
};

struct CWndCenterWindowAccess : CWnd {
    using CWnd::CenterWindow;
};

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

void *CWndSetWindowTextProc() {
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
    HWND const hwnd = CreateWindowExA(
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

void __fastcall FakeCWndSetWindowText(
    CWnd *self,
    void *,
    LPCSTR text
) {
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
}

bool InitMfcAndRegisterRecoilWindowClass() {
    HINSTANCE const instance = GetModuleHandleA(0);
    if (AfxWinInit(
            instance,
            0,
            GetCommandLineA(),
            SW_HIDE
        ) == 0) {
        return false;
    }

    WNDCLASSA wndClass = {};
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "RecoilClass";
    return RegisterClassA(&wndClass) != 0 ||
           GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
}

struct FrameConstructionPatches {
    CodeFunctionPatch createEx;
    CodeFunctionPatch setWindowText;
    CodeFunctionPatch centerWindow;

    bool Install() {
        return PatchFunctionJump(
                   CWndCreateExProc(),
                   reinterpret_cast<void *>(&FakeCWndCreateEx),
                   createEx
               ) &&
               PatchFunctionJump(
                   CWndSetWindowTextProc(),
                   reinterpret_cast<void *>(&FakeCWndSetWindowText),
                   setWindowText
               ) &&
               PatchFunctionJump(
                   CWndCenterWindowProc(),
                   reinterpret_cast<void *>(&FakeCWndCenterWindow),
                   centerWindow
               );
    }
};

struct FrameGlobalScope {
    int acceptedRendererCount;
    int texturePackLoadState;
    int useArchiveBanks;
    int hasWolApi;
    int missionFlags;
    HWND mainHwnd;
    HINSTANCE appInstance;

    FrameGlobalScope()
        : acceptedRendererCount(g_zVid_AcceptedHardwareRendererCount),
          texturePackLoadState(g_zVid_TexturePackLoadState),
          useArchiveBanks(g_zSnd_UseArchiveBanksFlag),
          hasWolApi(g_CZRecoilFrame_HasWolApi),
          missionFlags(g_HudSensorTracker.missionFlags),
          mainHwnd(g_RecoilApp_hWndMain),
          appInstance(g_RecoilApp_hInstance) {
    }

    ~FrameGlobalScope() {
        g_RecoilApp_hInstance = appInstance;
        g_RecoilApp_hWndMain = mainHwnd;
        g_HudSensorTracker.missionFlags = missionFlags;
        g_CZRecoilFrame_HasWolApi = hasWolApi;
        g_zSnd_UseArchiveBanksFlag = useArchiveBanks;
        g_zVid_TexturePackLoadState = texturePackLoadState;
        g_zVid_AcceptedHardwareRendererCount = acceptedRendererCount;
    }
};

void DestroySmokeFrame(CZRecoilFrame *frame) {
    if (frame == 0) {
        return;
    }

    if (frame->m_hWnd != 0) {
        HWND const hwnd = frame->Detach();
        if (hwnd != 0) {
            DestroyWindow(hwnd);
        }
    }
    delete frame;
}

struct TestAppState : RecoilApp_IState {
    int enterCalls;

    TestAppState() : enterCalls(0) {
    }

    void OnEnter() override {
        ++enterCalls;
    }
};

struct TestRecoilApp : RecoilApp {
    int startEngineCalls;
    int shutdownEngineCalls;
    int exitInstanceCalls;
    int startEngineResult;
    HWND lastStartEngineHwnd;
    CZRecoilFrame *createMainWndResult;

    TestRecoilApp()
        : startEngineCalls(0),
          shutdownEngineCalls(0),
          exitInstanceCalls(0),
          startEngineResult(1),
          lastStartEngineHwnd(0),
          createMainWndResult(0) {
    }

    int StartEngine(HWND hwnd) override {
        ++startEngineCalls;
        lastStartEngineHwnd = hwnd;
        return startEngineResult;
    }

    void ShutdownEngine() override {
        ++shutdownEngineCalls;
    }

    int ExitInstance() override {
        ++exitInstanceCalls;
        return 77;
    }

    CZRecoilFrame * CreateMainWnd() override {
        return createMainWndResult;
    }
};

void CleanupStateQueue(RecoilApp_StateQueue &queue) {
    if (queue.m_itemCount != 0 && queue.m_readBlock.m_cursor != 0) {
        RecoilApp_StateQueueItem **itemSlot = queue.m_readBlock.m_cursor;
        for (int index = 0; index < queue.m_itemCount; ++index) {
            delete itemSlot[index];
        }
    }

    if (queue.m_chunkBaseList != 0 &&
        queue.m_readBlock.m_chunkBaseSlot != 0 &&
        queue.m_writeBlock.m_chunkBaseSlot != 0) {
        RecoilApp_StateQueueItem ***slot =
            queue.m_readBlock.m_chunkBaseSlot;
        while (slot <= queue.m_writeBlock.m_chunkBaseSlot) {
            ::operator delete(*slot);
            ++slot;
        }
        ::operator delete(queue.m_chunkBaseList);
    }

    queue = RecoilApp_StateQueue{};
}

zZbdManager MakeTestZbdManager(
    zZbdSectionHandlerNode &sentinel
) {
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    sentinel.sectionHandler = {};

    zZbdManager manager = {};
    manager.sectionHandlerListSentinel = &sentinel;
    return manager;
}

void ClearTestRegisteredHandlers(
    zZbdSectionHandlerNode &sentinel
) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *const next = node->next;
        delete node;
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

bool CStringIsEmpty(
    const CString &value
) {
    const char *const text = value;
    return text != 0 && text[0] == '\0';
}

bool CStringEquals(
    const CString &value,
    const char *expected
) {
    const char *const text = value;
    return text != 0 &&
           std::strcmp(
               text,
               expected
           ) == 0;
}

int g_fatalLocCaptionCalls;
int g_fatalLocTextCalls;
int g_fatalSequence;
bool g_fatalOrderOk;
bool g_fatalArgsOk;
int g_fatalBriefingCalls;
int g_fatalFlipCalls;
int g_fatalSndCalls;
int g_fatalNetworkCalls;
int g_fatalVideoCalls;
int g_fatalSleepCalls;
int g_fatalBeepCalls;
int g_fatalMessageBoxCalls;
int g_fatalExitCalls;

char *__fastcall FakeFatalGetMessageString(
    unsigned int messageId
) {
    if (messageId == 0x12) {
        ++g_fatalLocCaptionCalls;
        return const_cast<char *>("Recoil");
    }
    if (messageId == 0x30) {
        ++g_fatalLocTextCalls;
        return const_cast<char *>("Fatal error");
    }
    return const_cast<char *>("");
}

void ResetFatalErrorProbe() {
    g_fatalLocCaptionCalls = 0;
    g_fatalLocTextCalls = 0;
    g_fatalSequence = 0;
    g_fatalOrderOk = true;
    g_fatalArgsOk = true;
    g_fatalBriefingCalls = 0;
    g_fatalFlipCalls = 0;
    g_fatalSndCalls = 0;
    g_fatalNetworkCalls = 0;
    g_fatalVideoCalls = 0;
    g_fatalSleepCalls = 0;
    g_fatalBeepCalls = 0;
    g_fatalMessageBoxCalls = 0;
    g_fatalExitCalls = 0;
}

void ExpectFatalStep(
    int expected
) {
    ++g_fatalSequence;
    g_fatalOrderOk =
        g_fatalOrderOk && g_fatalSequence == expected;
}

void __fastcall FakeFatalBriefingStop(
    int waitForInput
) {
    ++g_fatalBriefingCalls;
    ExpectFatalStep(1);
    g_fatalArgsOk =
        g_fatalArgsOk && waitForInput == 0;
}

void FakeFatalFlipToGDI() {
    ++g_fatalFlipCalls;
    ExpectFatalStep(2);
}

int FakeFatalSndShutdown() {
    ++g_fatalSndCalls;
    ExpectFatalStep(3);
    return 1;
}

int FakeFatalNetworkShutdown() {
    ++g_fatalNetworkCalls;
    ExpectFatalStep(4);
    return 1;
}

int FakeFatalVideoShutdown() {
    ++g_fatalVideoCalls;
    ExpectFatalStep(5);
    return 1;
}

void WINAPI FakeFatalSleep(
    DWORD milliseconds
) {
    ++g_fatalSleepCalls;
    ExpectFatalStep(6);
    g_fatalArgsOk =
        g_fatalArgsOk && milliseconds == 1000;
}

BOOL WINAPI FakeFatalMessageBeep(
    UINT type
) {
    ++g_fatalBeepCalls;
    ExpectFatalStep(7);
    g_fatalArgsOk =
        g_fatalArgsOk && type == MB_ICONHAND;
    return TRUE;
}

int WINAPI FakeFatalMessageBox(
    HWND hwnd,
    LPCSTR text,
    LPCSTR caption,
    UINT type
) {
    ++g_fatalMessageBoxCalls;
    ExpectFatalStep(8);
    g_fatalArgsOk =
        g_fatalArgsOk &&
        hwnd == g_RecoilApp_hWndMain &&
        text != 0 &&
        std::strcmp(
            text,
            "Fatal error"
        ) == 0 &&
        caption != 0 &&
        std::strcmp(
            caption,
            "Recoil"
        ) == 0 &&
        type == MB_ICONHAND;
    return IDOK;
}

void __fastcall FakeFatalExitProcessWithCleanup(
    int exitCode
) {
    ++g_fatalExitCalls;
    ExpectFatalStep(9);
    g_fatalArgsOk =
        g_fatalArgsOk && exitCode == 0;
}
} // namespace

extern "C" int crt_atexit_import_provider_smoke(void) {
    return std::atexit(AtexitProviderNoOp) == 0 ? 0 : 1;
}

extern "C" int
hud_ui_options_panel_overlay_owner_scalar_deleting_destructor_smoke(void) {
    {
        HudUiOptionsPanelOverlayOwner state;
        if (state.m_dialog != 0) {
            return 1;
        }
    }

    HudUiOptionsPanelOverlayOwner *const state =
        new HudUiOptionsPanelOverlayOwner;
    if (state == 0 || state->m_dialog != 0) {
        delete state;
        return 2;
    }

    delete state;
    return 0;
}

extern "C" int recoil_app_accessor_and_skip_wait_smoke(void) {
    RecoilApp app;
    CWnd mainWindow;
    app.m_pMainWnd = &mainWindow;
    if (app.GetMainWnd() !=
        reinterpret_cast<CZRecoilFrame *>(&mainWindow)) {
        return 1;
    }

    app.m_skipWait = 7;
    if (app.TakeSkipWaitMessage() != 7 ||
        app.m_skipWait != 0) {
        return 2;
    }

    if (app.MarkSkipWaitMessage() != 0 ||
        app.m_skipWait != 1) {
        return 3;
    }

    return app.MarkSkipWaitMessage() == 1 &&
                   app.m_skipWait == 1
               ? 0
               : 4;
}

extern "C" int recoil_app_activate_existing_instance_absent_smoke(void) {
    char absentClassName[96] = {};
    std::sprintf(
        absentClassName,
        "RecoilNativeAbsentWindowClass_%lu",
        static_cast<unsigned long>(GetCurrentProcessId())
    );

    const char *const oldClassName =
        g_RecoilApp_WndClassNamePtr;
    g_RecoilApp_WndClassNamePtr = absentClassName;
    const int result =
        g_RecoilApp.ActivateExistingInstance();
    g_RecoilApp_WndClassNamePtr = oldClassName;
    return result == 1 ? 0 : 1;
}

extern "C" int recoil_app_create_main_wnd_smoke(void) {
    if (!InitMfcAndRegisterRecoilWindowClass()) {
        return 1;
    }

    FrameGlobalScope globals;
    FrameConstructionPatches patches;
    if (!patches.Install()) {
        return 2;
    }

    g_zVid_AcceptedHardwareRendererCount = 4;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    CZRecoilFrame *const frame =
        g_RecoilApp.CreateMainWnd();
    if (frame == 0) {
        return 3;
    }

    const bool ok =
        frame->GetRuntimeClass() ==
            RUNTIME_CLASS(CZRecoilFrame) &&
        frame->m_useArchiveBanks == 1 &&
        frame->m_acceptedD3DDeviceCount ==
            g_zVid_AcceptedHardwareRendererCount &&
        g_zSnd_UseArchiveBanksFlag == 1;

    DestroySmokeFrame(frame);
    return ok ? 0 : 4;
}

extern "C" int recoil_app_fatal_error_and_exit_smoke(void) {
    CodeFunctionPatch zlocPatch;
    CodeFunctionPatch briefingPatch;
    CodeFunctionPatch flipPatch;
    CodeFunctionPatch sndPatch;
    CodeFunctionPatch networkPatch;
    CodeFunctionPatch videoPatch;
    CodeFunctionPatch exitPatch;
    ImportFunctionPatch sleepPatch;
    ImportFunctionPatch beepPatch;
    ImportFunctionPatch messageBoxPatch;

    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zLoc::GetMessageString),
            reinterpret_cast<void *>(&FakeFatalGetMessageString),
            zlocPatch
        )) {
        return 10;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&Briefing::StopAndShutdownThread),
            reinterpret_cast<void *>(&FakeFatalBriefingStop),
            briefingPatch
        )) {
        return 11;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo_dd::FlipToGDIIfAttached),
            reinterpret_cast<void *>(&FakeFatalFlipToGDI),
            flipPatch
        )) {
        return 12;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zSndSystem::Shutdown),
            reinterpret_cast<void *>(&FakeFatalSndShutdown),
            sndPatch
        )) {
        return 13;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zNetwork::ShutdownSessionRuntime),
            reinterpret_cast<void *>(&FakeFatalNetworkShutdown),
            networkPatch
        )) {
        return 14;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::ShutdownVideoSystem),
            reinterpret_cast<void *>(&FakeFatalVideoShutdown),
            videoPatch
        )) {
        return 15;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zSys::ExitProcessWithCleanup),
            reinterpret_cast<void *>(&FakeFatalExitProcessWithCleanup),
            exitPatch
        )) {
        return 16;
    }
    if (!PatchImportByName(
            "KERNEL32.dll",
            "Sleep",
            reinterpret_cast<void *>(&FakeFatalSleep),
            sleepPatch
        )) {
        return 17;
    }
    if (!PatchImportByName(
            "USER32.dll",
            "MessageBeep",
            reinterpret_cast<void *>(&FakeFatalMessageBeep),
            beepPatch
        )) {
        return 18;
    }
    if (!PatchImportByName(
            "USER32.dll",
            "MessageBoxA",
            reinterpret_cast<void *>(&FakeFatalMessageBox),
            messageBoxPatch
        )) {
        return 19;
    }

    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    g_RecoilApp_hWndMain =
        reinterpret_cast<HWND>(0x12345678);

    ResetFatalErrorProbe();
    RecoilApp::FatalErrorAndExit(7);
    const bool nonFatalOk =
        g_fatalSequence == 0 &&
        g_fatalLocCaptionCalls == 0 &&
        g_fatalLocTextCalls == 0;

    ResetFatalErrorProbe();
    RecoilApp::FatalErrorAndExit(-1);
    const bool fatalOk =
        g_fatalLocCaptionCalls == 1 &&
        g_fatalLocTextCalls == 1 &&
        g_fatalBriefingCalls == 1 &&
        g_fatalFlipCalls == 1 &&
        g_fatalSndCalls == 1 &&
        g_fatalNetworkCalls == 1 &&
        g_fatalVideoCalls == 1 &&
        g_fatalSleepCalls == 1 &&
        g_fatalBeepCalls == 1 &&
        g_fatalMessageBoxCalls == 1 &&
        g_fatalExitCalls == 1 &&
        g_fatalSequence == 9 &&
        g_fatalOrderOk &&
        g_fatalArgsOk;

    g_RecoilApp_hWndMain = oldMainHwnd;
    return nonFatalOk && fatalOk ? 0 : 1;
}

extern "C" int recoil_app_fmv_state_on_idle_or_dispatch_smoke(void) {
    RecoilApp_FmvState state;
    return state.OnIdleOrDispatch(
               0x11111111,
               0x22222222
           ) == 1
               ? 0
               : 1;
}

extern "C" int recoil_app_get_current_state_smoke(void) {
    RecoilApp app;
    TestAppState firstState;
    TestAppState lastState;
    app.m_stateStack[0] = &firstState;
    app.m_stateStack[15] = &lastState;

    app.m_currentStateIndex = -1;
    if (app.GetCurrentState() != 0) {
        return 1;
    }

    app.m_currentStateIndex = 0;
    if (app.GetCurrentState() != &firstState) {
        return 2;
    }

    app.m_currentStateIndex = 15;
    if (app.GetCurrentState() != &lastState) {
        return 3;
    }

    app.m_currentStateIndex = 16;
    return app.GetCurrentState() == 0 ? 0 : 4;
}

extern "C" int recoil_app_init_main_window_smoke(void) {
    if (!InitMfcAndRegisterRecoilWindowClass()) {
        return 1;
    }

    FrameGlobalScope globals;
    FrameConstructionPatches patches;
    if (!patches.Install()) {
        return 2;
    }

    g_zVid_AcceptedHardwareRendererCount = 5;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    CZRecoilFrame *const frame =
        new CZRecoilFrame;
    if (frame == 0) {
        return 3;
    }

    TestRecoilApp app;
    app.createMainWndResult = frame;
    const int result =
        app.RecoilApp_MfcOleModule::InitInstance();
    const bool ok =
        result == 1 &&
        app.GetMainWnd() == frame &&
        frame->m_app == &app &&
        IsWindowVisible(frame->m_hWnd) != 0;

    app.m_pMainWnd = 0;
    DestroySmokeFrame(frame);
    return ok ? 0 : 4;
}

extern "C" int recoil_app_load_zbd_and_setup_sensor_tracker_smoke(void) {
    zZbdManager *const oldManager =
        g_zUtil_ZbdManager;
    const int oldMissionId =
        g_HudSensorTracker.missionId;
    const int oldMissionFlags =
        g_HudSensorTracker.missionFlags;
    const CString oldZbdPath =
        g_HudSensorTracker.zbdPath;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager =
        MakeTestZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    g_HudSensorTracker.missionFlags = 0;

    CWnd mainWindow;
    mainWindow.m_hWnd =
        reinterpret_cast<HWND>(0x24681357);

    TestRecoilApp zbdApp;
    TestAppState zbdStartupState;
    zbdApp.m_pMainWnd = &mainWindow;
    zbdApp.m_pendingState = &zbdStartupState;
    zbdApp.m_currentStateIndex = -1;

    const bool zbdPathOk =
        zbdApp.LoadZbdAndSetupSensorTracker(
            0,
            "custom.zbd",
            3,
            0x44
        ) == 1 &&
        zbdApp.m_skipIntroFmv == 3 &&
        CStringEquals(
            g_HudSensorTracker.zbdPath,
            "custom.zbd"
        );
    CleanupStateQueue(zbdApp.m_stateQueue);
    const bool zbdRegisterOk =
        manager.sectionHandlerCount == 2;
    ClearTestRegisteredHandlers(sentinel);
    manager.sectionHandlerCount = 0;

    g_HudSensorTracker.missionFlags = 0;
    TestRecoilApp missionApp;
    TestAppState missionStartupState;
    missionApp.m_pMainWnd = &mainWindow;
    missionApp.m_pendingState = &missionStartupState;
    missionApp.m_currentStateIndex = -1;

    const bool missionOk =
        missionApp.LoadZbdAndSetupSensorTracker(
            9,
            0,
            4,
            0x66
        ) == 1 &&
        missionApp.m_skipIntroFmv == 4 &&
        g_HudSensorTracker.missionId == 9 &&
        g_HudSensorTracker.missionFlags == 0x66 &&
        CStringIsEmpty(g_HudSensorTracker.zbdPath);
    CleanupStateQueue(missionApp.m_stateQueue);
    const bool missionRegisterOk =
        manager.sectionHandlerCount == 2;
    ClearTestRegisteredHandlers(sentinel);

    mainWindow.m_hWnd = 0;
    g_zUtil_ZbdManager = oldManager;
    g_HudSensorTracker.missionId = oldMissionId;
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_HudSensorTracker.zbdPath = oldZbdPath;

    if (!zbdPathOk) {
        return 1;
    }
    if (!zbdRegisterOk) {
        return 2;
    }
    if (!missionOk) {
        return 3;
    }
    if (!missionRegisterOk) {
        return 4;
    }
    if (zbdApp.startEngineCalls != 1 ||
        missionApp.startEngineCalls != 1) {
        return 5;
    }
    return 0;
}

extern "C" int recoil_app_load_zbd_and_start_engine_smoke(void) {
    zZbdManager *const oldManager =
        g_zUtil_ZbdManager;
    const int oldMissionFlags =
        g_HudSensorTracker.missionFlags;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager =
        MakeTestZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    g_HudSensorTracker.missionFlags = 0;

    CWnd mainWindow;
    mainWindow.m_hWnd =
        reinterpret_cast<HWND>(0x13572468);

    TestRecoilApp app;
    TestAppState startupState;
    app.m_pMainWnd = &mainWindow;
    app.m_pendingState = &startupState;
    app.m_currentStateIndex = -1;
    app.startEngineResult = 1;

    const int result =
        app.LoadZbdAndStartEngine();
    const bool ok =
        result == 1 &&
        app.startEngineCalls == 1 &&
        app.lastStartEngineHwnd ==
            reinterpret_cast<HWND>(0x13572468) &&
        startupState.enterCalls == 1 &&
        app.m_skipWait == 1 &&
        app.m_missionShutdownMode ==
            RECOILAPP_MISSION_SHUTDOWN_ON_EXIT &&
        app.m_stateQueue.m_itemCount == 1 &&
        manager.sectionHandlerCount == 2 &&
        std::strcmp(
            sentinel.next->sectionHandler.sectionName,
            "Mission"
        ) == 0 &&
        std::strcmp(
            sentinel.prev->sectionHandler.sectionName,
            "MissionLate"
        ) == 0;

    CleanupStateQueue(app.m_stateQueue);
    ClearTestRegisteredHandlers(sentinel);
    mainWindow.m_hWnd = 0;
    g_zUtil_ZbdManager = oldManager;
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    return ok ? 0 : 1;
}

extern "C" int recoil_app_pre_translate_message_smoke(void) {
    int *const oldAccelerationOption =
        g_zGame_Options_PointerCache.videoAcceleration;
    int acceleration = 0;
    g_zGame_Options_PointerCache.videoAcceleration = &acceleration;

    MSG message = {};
    message.message = WM_SYSKEYDOWN;
    if (g_RecoilApp.PreTranslateMessage(&message) != 0) {
        g_zGame_Options_PointerCache.videoAcceleration = oldAccelerationOption;
        return 1;
    }

    acceleration = 1;
    if (g_RecoilApp.PreTranslateMessage(&message) != 1) {
        g_zGame_Options_PointerCache.videoAcceleration = oldAccelerationOption;
        return 2;
    }

    message.message = WM_SYSKEYUP;
    if (g_RecoilApp.PreTranslateMessage(&message) != 1) {
        g_zGame_Options_PointerCache.videoAcceleration = oldAccelerationOption;
        return 3;
    }

    message.message = WM_KEYDOWN;
    if (g_RecoilApp.PreTranslateMessage(&message) != 0) {
        g_zGame_Options_PointerCache.videoAcceleration = oldAccelerationOption;
        return 4;
    }

    message.message = WM_SYSKEYUP + 1;
    const bool adjacentMessageRejected =
        g_RecoilApp.PreTranslateMessage(&message) == 0;
    g_zGame_Options_PointerCache.videoAcceleration = oldAccelerationOption;
    return adjacentMessageRejected ? 0 : 5;
}

extern "C" int recoil_app_state_queue_grow_chunk_base_list_smoke(void) {
    RecoilApp_StateQueue queue = {};
    RecoilApp_StateQueueItem ***const oldList =
        static_cast<RecoilApp_StateQueueItem ***>(
            ::operator new(
                3 * sizeof(RecoilApp_StateQueueItem **)
            )
        );
    oldList[0] =
        reinterpret_cast<RecoilApp_StateQueueItem **>(
            static_cast<std::uintptr_t>(0x11111111)
        );
    oldList[1] =
        reinterpret_cast<RecoilApp_StateQueueItem **>(
            static_cast<std::uintptr_t>(0x22222222)
        );
    oldList[2] =
        reinterpret_cast<RecoilApp_StateQueueItem **>(
            static_cast<std::uintptr_t>(0x33333333)
        );

    queue.m_readBlock.m_chunkBaseSlot = &oldList[0];
    queue.m_writeBlock.m_chunkBaseSlot = &oldList[2];
    queue.m_chunkBaseList = oldList;
    queue.m_chunkBaseCapacity = 3;

    RecoilApp_StateQueueItem ***const centerSlot =
        queue.GrowAndCenterChunkBaseList(8);
    RecoilApp_StateQueueItem ***const newList =
        queue.m_chunkBaseList;
    const bool ok =
        queue.m_chunkBaseCapacity == 8 &&
        centerSlot == &newList[2] &&
        centerSlot[0] ==
            reinterpret_cast<RecoilApp_StateQueueItem **>(
                static_cast<std::uintptr_t>(0x11111111)
            ) &&
        centerSlot[1] ==
            reinterpret_cast<RecoilApp_StateQueueItem **>(
                static_cast<std::uintptr_t>(0x22222222)
            ) &&
        centerSlot[2] ==
            reinterpret_cast<RecoilApp_StateQueueItem **>(
                static_cast<std::uintptr_t>(0x33333333)
            );

    ::operator delete(newList);
    return ok ? 0 : 1;
}

extern "C" int
recoil_state_cheat_code_scalar_deleting_destructor_smoke(void) {
    {
        RecoilStateCheatCode state;
        if (state.m_dialog != 0) {
            return 1;
        }
    }

    RecoilStateCheatCode *const state =
        new RecoilStateCheatCode;
    if (state == 0 || state->m_dialog != 0) {
        delete state;
        return 2;
    }

    delete state;
    return 0;
}
