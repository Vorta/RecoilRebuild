#include "Battlesport/CZGameFrame.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/NetUi.h"
#include "Battlesport/Recoil.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/WestwoodOnlineUpgradeDialog.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <commdlg.h>
#include <cstring>

extern "C" int g_CZRecoilFrame_HasWolApi;
extern "C" int g_CZRecoilFrame_WestwoodOnlineWinsockChecked;
extern "C" HWND g_RecoilApp_hWndMain;
extern "C" HINSTANCE g_RecoilApp_hInstance;
BOOL __stdcall AfxWinInit(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine,
                               int showCommand);
HINSTANCE __stdcall AfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType);

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

int HandleFrameConstructorException(EXCEPTION_POINTERS *exceptionInfo) {
    (void)exceptionInfo;
    return EXCEPTION_EXECUTE_HANDLER;
}

BOOL __fastcall FakeCWndCreateEx(CWnd *, void *, DWORD, LPCSTR, LPCSTR, DWORD, int, int,
                                      int, int, HWND, HMENU, LPVOID);
void __fastcall FakeCWndSetWindowTextA(CWnd *, void *, LPCSTR);
void __fastcall FakeCWndCenterWindow(CWnd *, void *, CWnd *);
void *CWndCreateExProc();
void *CWndSetWindowTextAProc();
void *CWndCenterWindowProc();
bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch);
void RestoreFunctionPatch(CodeFunctionPatch &patch);
} // namespace

extern "C" int czrecoil_frame_build_window_title_smoke(void) {
    CZRecoilFrame frame{};
    alignas(CString) unsigned char storage[sizeof(CString)];
    auto *title = reinterpret_cast<CString *>(storage);

    g_zVideo_ActiveRendererPath = 0;
    CString *returned = frame.BuildWindowTitle(title);
    bool ok = returned == title && (const char *)(*title) != nullptr &&
              std::strcmp((const char *)(*title), "RECOIL") == 0;
    title->~CString();

    g_zVideo_ActiveRendererPath = 1;
    returned = frame.BuildWindowTitle(title);
    ok = ok && returned == title && (const char *)(*title) != nullptr &&
         std::strcmp((const char *)(*title), "RECOIL (3Dfx)") == 0;
    title->~CString();

    return ok ? 0 : 1;
}

#ifndef RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES
extern "C" int czframe_metadata_accessors_smoke(void) {
    typedef CObject *(PASCAL *MfcCreateObjectProc)();
    typedef CRuntimeClass *(PASCAL *MfcRuntimeClassProc)();
    typedef const AFX_MSGMAP *(PASCAL *MfcMessageMapProc)();

    const auto gameRuntimeClass = reinterpret_cast<CRuntimeClass *>(
        static_cast<std::uintptr_t>(CZGameFrame::GetRuntimeClass()));
    const auto gameBaseRuntimeClass = reinterpret_cast<CRuntimeClass *>(
        static_cast<std::uintptr_t>(CZGameFrame::GetBaseRuntimeClass()));
    const auto gameMessageMap = reinterpret_cast<const AFX_MSGMAP *>(
        static_cast<std::uintptr_t>(CZGameFrame::GetMessageMap()));
    const auto gameBaseMessageMap = reinterpret_cast<const AFX_MSGMAP *>(
        static_cast<std::uintptr_t>(CZGameFrame::GetBaseMessageMap()));
    const auto recoilRuntimeClass = reinterpret_cast<CRuntimeClass *>(
        static_cast<std::uintptr_t>(CZRecoilFrame::GetRuntimeClass()));
    const auto recoilMessageMap = reinterpret_cast<const AFX_MSGMAP *>(
        static_cast<std::uintptr_t>(CZRecoilFrame::GetMessageMap()));

    if (gameRuntimeClass != &CZGameFrame::classCZGameFrame ||
        std::strcmp(gameRuntimeClass->m_lpszClassName, "CZGameFrame") != 0 ||
        gameRuntimeClass->m_pfnCreateObject !=
            (MfcCreateObjectProc)(&CZGameFrame::CreateObject) ||
        gameRuntimeClass->m_pfnGetBaseClass !=
            (MfcRuntimeClassProc)(&CZGameFrame::GetBaseRuntimeClass) ||
        gameRuntimeClass->m_pfnGetBaseClass() != &CFrameWnd::classCFrameWnd) {
        return 1;
    }

    if (gameBaseRuntimeClass != &CFrameWnd::classCFrameWnd ||
        gameBaseMessageMap != &CFrameWnd::messageMap) {
        return 5;
    }

    if (gameMessageMap != &CZGameFrame::messageMap ||
        gameMessageMap->pfnGetBaseMap !=
            (MfcMessageMapProc)(&CZGameFrame::GetBaseMessageMap) ||
        gameMessageMap->pfnGetBaseMap() != &CFrameWnd::messageMap ||
        gameMessageMap->lpEntries != &CZGameFrame::messageEntries[0]) {
        return 2;
    }

    if (recoilRuntimeClass != &CZRecoilFrame::classCZRecoilFrame ||
        std::strcmp(recoilRuntimeClass->m_lpszClassName, "CZRecoilFrame") != 0 ||
        recoilRuntimeClass->m_pfnCreateObject !=
            (MfcCreateObjectProc)(&CZRecoilFrame::CreateObject) ||
        recoilRuntimeClass->m_pfnGetBaseClass !=
            (MfcRuntimeClassProc)(&CZGameFrame::GetRuntimeClass) ||
        recoilRuntimeClass->m_pfnGetBaseClass() != &CZGameFrame::classCZGameFrame) {
        return 3;
    }

    return recoilMessageMap == &CZRecoilFrame::messageMap &&
                   recoilMessageMap->pfnGetBaseMap ==
                       (MfcMessageMapProc)(&CZGameFrame::GetMessageMap) &&
                   recoilMessageMap->pfnGetBaseMap() == &CZGameFrame::messageMap &&
                   recoilMessageMap->lpEntries == &CZRecoilFrame::messageEntries[0]
               ? 0
               : 4;
}
#endif

extern "C" int get_open_file_name_import_provider_smoke(void) {
    OPENFILENAMEA ofn{};
    ofn.lStructSize = 0;
    return GetOpenFileNameA(&ofn) == 0 ? 0 : 1;
}

extern "C" int czrecoil_frame_set_menu_bar_visibility_smoke(void) {
    HINSTANCE instance = GetModuleHandleA(nullptr);
    WNDCLASSA wndClass{};
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "RecoilMenuVisibilityTestClass";
    if (RegisterClassA(&wndClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return 1;
    }

    HWND hwnd = CreateWindowExA(0, wndClass.lpszClassName, "recoil-test", WS_OVERLAPPEDWINDOW, 0,
                                0, 100, 100, nullptr, nullptr, instance, nullptr);
    if (hwnd == nullptr) {
        return 2;
    }

    HMENU menu = CreateMenu();
    if (menu == nullptr) {
        DestroyWindow(hwnd);
        return 3;
    }

    CZRecoilFrame frame{};
    frame.m_hWnd = hwnd;
    frame.m_mainMenu.m_hMenu = menu;

    SetWindowLongA(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
    frame.SetMenuBarVisibility(0);
    const LONG hiddenStyle = GetWindowLongA(hwnd, GWL_STYLE);
    const bool hiddenOk = (hiddenStyle & WS_SYSMENU) == 0 && GetMenu(hwnd) == nullptr;

    frame.SetMenuBarVisibility(1);
    const LONG shownStyle = GetWindowLongA(hwnd, GWL_STYLE);
    const bool shownOk = (shownStyle & WS_SYSMENU) != 0 && GetMenu(hwnd) == menu;

    frame.m_mainMenu.m_hMenu = nullptr;
    frame.m_hWnd = nullptr;
    DestroyMenu(menu);
    DestroyWindow(hwnd);
    return hiddenOk && shownOk ? 0 : 4;
}

#if defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_MENU_SMOKES)
namespace {
struct RecoilFrameMenuImportPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

int g_frameMenuStateEnterCount;
int g_frameMenuStartEngineCount;
int g_frameMenuShutdownEngineCount;
int g_frameMenuExitInstanceCount;
int g_frameMenuStartEngineResult;
HWND g_frameMenuLastStartEngineHwnd;
int g_frameMenuOpenFileNameCalls;
bool g_frameMenuOpenFileNameStructOk;
char g_frameMenuSelectedPath[0x104];

struct RecoilFrameMenuTestAppState : RecoilApp_IState {
    void OnEnter() {
        ++g_frameMenuStateEnterCount;
    }
};

struct RecoilFrameMenuTestRecoilApp : RecoilApp {
    int StartEngine(HWND hwnd) {
        ++g_frameMenuStartEngineCount;
        g_frameMenuLastStartEngineHwnd = hwnd;
        return g_frameMenuStartEngineResult;
    }

    void ShutdownEngine() {
        ++g_frameMenuShutdownEngineCount;
    }

    int ExitInstance() {
        ++g_frameMenuExitInstanceCount;
        return 77;
    }
};

bool PatchFrameMenuImportByName(
    const char *dllName,
    const char *functionName,
    void *replacement,
    RecoilFrameMenuImportPatch &patch
) {
    HMODULE module = GetModuleHandleA(nullptr);
    unsigned char *const base = reinterpret_cast<unsigned char *>(module);
    IMAGE_DOS_HEADER *const dos = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
    IMAGE_NT_HEADERS *const nt = reinterpret_cast<IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
    const IMAGE_DATA_DIRECTORY &directory =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base + directory.VirtualAddress);

    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importDll = reinterpret_cast<const char *>(base + descriptor->Name);
        if (_stricmp(importDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *names = reinterpret_cast<IMAGE_THUNK_DATA *>(
            base + (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                        : descriptor->FirstThunk));
        IMAGE_THUNK_DATA *thunks =
            reinterpret_cast<IMAGE_THUNK_DATA *>(base + descriptor->FirstThunk);
        for (; names->u1.AddressOfData != 0; ++names, ++thunks) {
            if ((names->u1.Ordinal & IMAGE_ORDINAL_FLAG) != 0) {
                continue;
            }

            IMAGE_IMPORT_BY_NAME *const importName =
                reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(base + names->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char *>(importName->Name), functionName) != 0) {
                continue;
            }

            patch.slot = reinterpret_cast<ULONG_PTR *>(&thunks->u1.Function);
            patch.original = *patch.slot;
            DWORD oldProtect = 0;
            if (VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0) {
                patch.slot = nullptr;
                return false;
            }

            *patch.slot = reinterpret_cast<ULONG_PTR>(replacement);
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

    patch.slot = nullptr;
    return false;
}

void RestoreFrameMenuImportPatch(RecoilFrameMenuImportPatch &patch) {
    if (patch.slot == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.slot,
            sizeof(*patch.slot),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        *patch.slot = patch.original;
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
    }
    patch.slot = nullptr;
}

RecoilApp_StateQueueItem *FrameMenuQueueItemAt(RecoilApp_StateQueue &queue, int index) {
    if (index < 0 || index >= queue.m_itemCount || queue.m_readBlock.m_cursor == nullptr) {
        return nullptr;
    }

    return queue.m_readBlock.m_cursor[index];
}

void CleanupFrameMenuQueue(RecoilApp_StateQueue &queue) {
    const int itemCount = queue.m_itemCount;
    for (int index = 0; index < itemCount; ++index) {
        ::operator delete(FrameMenuQueueItemAt(queue, index));
    }

    if (queue.m_chunkBaseList != nullptr) {
        if (queue.m_readBlock.m_chunkBaseSlot != nullptr &&
            queue.m_writeBlock.m_chunkBaseSlot != nullptr) {
            for (RecoilApp_StateQueueItem ***slot = queue.m_readBlock.m_chunkBaseSlot;
                 slot <= queue.m_writeBlock.m_chunkBaseSlot;
                 ++slot) {
                ::operator delete(*slot);
            }
        }
        ::operator delete(queue.m_chunkBaseList);
    }

    std::memset(&queue, 0, sizeof(queue));
}

void InitFrameMenuZbdManager(zZbdManager &manager, zZbdSectionHandlerNode &sentinel) {
    std::memset(&manager, 0, sizeof(manager));
    std::memset(&sentinel, 0, sizeof(sentinel));
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    manager.sectionHandlerListSentinel = &sentinel;
}

void ClearFrameMenuZbdHandlers(zZbdManager &manager, zZbdSectionHandlerNode &sentinel) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *const next = node->next;
        ::operator delete(node);
        node = next;
    }

    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    manager.sectionHandlerCount = 0;
}

void InstallFrameMenuAppHarness(
    RecoilFrameMenuTestRecoilApp &vtableSource,
    RecoilFrameMenuTestAppState &startupState,
    CZRecoilFrame &frame
) {
    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    *reinterpret_cast<void **>(&g_RecoilApp) = *reinterpret_cast<void **>(&vtableSource);
    g_RecoilApp.m_pMainWnd = &frame;
    g_RecoilApp.m_pendingState = &startupState;
    g_RecoilApp.m_currentStateIndex = -1;
    g_frameMenuStartEngineResult = 1;
}

bool CStringEquals(const CString &value, const char *expected) {
    const char *const actual = value;
    return actual != nullptr && std::strcmp(actual, expected) == 0;
}

bool FrameMenuFilterMatches(const char *filter) {
    if (filter == nullptr || std::strcmp(filter, "Game File (*.gs)") != 0) {
        return false;
    }

    filter += std::strlen(filter) + 1;
    if (std::strcmp(filter, "*.gs") != 0) {
        return false;
    }

    filter += std::strlen(filter) + 1;
    if (std::strcmp(filter, "Text File (*.txt)") != 0) {
        return false;
    }

    filter += std::strlen(filter) + 1;
    if (std::strcmp(filter, "*.txt") != 0) {
        return false;
    }

    filter += std::strlen(filter) + 1;
    if (std::strcmp(filter, "All Files (*.*)") != 0) {
        return false;
    }

    filter += std::strlen(filter) + 1;
    if (std::strcmp(filter, "*.*") != 0) {
        return false;
    }

    filter += std::strlen(filter) + 1;
    return *filter == '\0';
}

BOOL WINAPI FakeFrameMenuGetOpenFileNameA(LPOPENFILENAMEA ofn) {
    ++g_frameMenuOpenFileNameCalls;
    g_frameMenuOpenFileNameStructOk =
        ofn != nullptr &&
        ofn->lStructSize == 0x4c &&
        ofn->hwndOwner != nullptr &&
        FrameMenuFilterMatches(ofn->lpstrFilter) &&
        ofn->nFilterIndex == 1 &&
        ofn->lpstrFile != nullptr &&
        ofn->nMaxFile == 0x104 &&
        ofn->lpstrFileTitle != nullptr &&
        ofn->nMaxFileTitle == 0x200 &&
        ofn->Flags == (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST) &&
        ofn->lpstrDefExt != nullptr &&
        std::strcmp(ofn->lpstrDefExt, "gs") == 0;

    strcpy_s(ofn->lpstrFile, ofn->nMaxFile, g_frameMenuSelectedPath);
    return TRUE;
}

int RunFrameMenuOpenCampaignSmoke(bool throughMenuHandler) {
    RecoilFrameMenuImportPatch patch{};
    if (!PatchFrameMenuImportByName(
            "COMDLG32.dll",
            "GetOpenFileNameA",
            reinterpret_cast<void *>(FakeFrameMenuGetOpenFileNameA),
            patch
        )) {
        return 10;
    }

    unsigned char appBackup[sizeof(g_RecoilApp)];
    std::memcpy(appBackup, &g_RecoilApp, sizeof(g_RecoilApp));
    HINSTANCE const oldInstance = g_RecoilApp_hInstance;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    const int oldMissionFlags = g_HudSensorTracker.missionFlags;

    zZbdSectionHandlerNode sentinel;
    zZbdManager manager;
    InitFrameMenuZbdManager(manager, sentinel);

    HWND hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil-open-test",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        100,
        100,
        nullptr,
        nullptr,
        GetModuleHandleA(nullptr),
        nullptr
    );
    if (hwnd == nullptr) {
        RestoreFrameMenuImportPatch(patch);
        return 11;
    }

    g_frameMenuStateEnterCount = 0;
    g_frameMenuStartEngineCount = 0;
    g_frameMenuShutdownEngineCount = 0;
    g_frameMenuExitInstanceCount = 0;
    g_frameMenuLastStartEngineHwnd = nullptr;
    g_frameMenuOpenFileNameCalls = 0;
    g_frameMenuOpenFileNameStructOk = false;
    strcpy_s(g_frameMenuSelectedPath, "selected_campaign.gs");

    g_zUtil_ZbdManager = &manager;
    g_RecoilApp_hInstance = GetModuleHandleA(nullptr);
    g_HudSensorTracker.missionFlags = 0;
    g_HudSensorTracker.zbdPath.Empty();

    alignas(CZRecoilFrame) unsigned char frameStorage[sizeof(CZRecoilFrame)] = {};
    CZRecoilFrame *const frame = reinterpret_cast<CZRecoilFrame *>(frameStorage);
    frame->m_hWnd = hwnd;
    strcpy_s(frame->m_openZbdFilePath, "before.gs");
    RecoilFrameMenuTestRecoilApp vtableSource;
    RecoilFrameMenuTestAppState startupState;
    InstallFrameMenuAppHarness(vtableSource, startupState, *frame);
    g_RecoilApp.m_skipIntroFmv = throughMenuHandler ? 0 : 7;

    if (throughMenuHandler) {
        frame->OnMenuOpenCampaign();
    } else {
        frame->OnOpenFileDialog();
    }

    int result = 0;
    if (g_frameMenuOpenFileNameCalls != 1) {
        result = 20;
    } else if (!g_frameMenuOpenFileNameStructOk) {
        result = 21;
    } else if (std::strcmp(frame->m_openZbdFilePath, g_frameMenuSelectedPath) != 0) {
        result = 22;
    } else if (!CStringEquals(g_HudSensorTracker.zbdPath, g_frameMenuSelectedPath)) {
        result = 23;
    } else if (g_RecoilApp.m_skipIntroFmv != 1) {
        result = 24;
    } else if (g_frameMenuStartEngineCount != 1) {
        result = 25;
    } else if (g_frameMenuLastStartEngineHwnd != hwnd) {
        result = 26;
    } else if (g_frameMenuStateEnterCount != 1) {
        result = 27;
    } else if (g_RecoilApp.m_stateQueue.m_itemCount != 1) {
        result = 28;
    } else if (manager.sectionHandlerCount != 2) {
        result = 29;
    }

    CleanupFrameMenuQueue(g_RecoilApp.m_stateQueue);
    ClearFrameMenuZbdHandlers(manager, sentinel);
    g_HudSensorTracker.zbdPath.Empty();
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_RecoilApp_hInstance = oldInstance;
    g_zUtil_ZbdManager = oldZbdManager;
    std::memcpy(&g_RecoilApp, appBackup, sizeof(g_RecoilApp));
    DestroyWindow(hwnd);
    RestoreFrameMenuImportPatch(patch);
    return result;
}

#if defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES)
UINT_PTR g_frameHelpDocsFindExecutableResult;
int g_frameHelpDocsFindExecutableCalls;
bool g_frameHelpDocsFindExecutableArgsOk;
int g_frameHelpDocsShellExecuteCalls;
bool g_frameHelpDocsShellExecuteArgsOk;
int g_frameHelpDocsMessageBoxCalls;
bool g_frameHelpDocsMessageBoxArgsOk;
CWnd *g_frameHelpDocsExpectedMessageBoxWnd;
const char *g_frameHelpDocsExpectedMessageText;
int g_frameAboutDialogCtorCalls;
int g_frameAboutDialogDoModalCalls;
int g_frameAboutDialogDtorCalls;
bool g_frameAboutDialogCtorArgsOk;
bool g_frameAboutDialogFlowOk;
CDialog *g_frameAboutDialogThis;

constexpr WORD kFrameMfc42CDialogResourceCtorOrdinal = 324;
constexpr WORD kFrameMfc42CDialogDtorOrdinal = 641;
constexpr WORD kFrameMfc42CDialogDoModalOrdinal = 2514;
constexpr WORD kFrameMfc42CWndMessageBoxAOrdinal = 4224;

bool PatchFrameMenuImportByOrdinal(
    const char *dllName,
    WORD ordinal,
    void *replacement,
    RecoilFrameMenuImportPatch &patch
) {
    unsigned char *const imageBase = reinterpret_cast<unsigned char *>(GetModuleHandleA(nullptr));
    IMAGE_DOS_HEADER *const dos = reinterpret_cast<IMAGE_DOS_HEADER *>(imageBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt = reinterpret_cast<IMAGE_NT_HEADERS *>(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &directory =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(imageBase + directory.VirtualAddress);

    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importDll = reinterpret_cast<const char *>(imageBase + descriptor->Name);
        if (_stricmp(importDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *names = reinterpret_cast<IMAGE_THUNK_DATA *>(
            imageBase + (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                             : descriptor->FirstThunk));
        IMAGE_THUNK_DATA *thunks =
            reinterpret_cast<IMAGE_THUNK_DATA *>(imageBase + descriptor->FirstThunk);
        for (; names->u1.AddressOfData != 0; ++names, ++thunks) {
            if (!IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal) ||
                static_cast<WORD>(names->u1.Ordinal & 0xffff) != ordinal) {
                continue;
            }

            patch.slot = reinterpret_cast<ULONG_PTR *>(&thunks->u1.Function);
            patch.original = *patch.slot;
            DWORD oldProtect = 0;
            if (VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0) {
                patch.slot = nullptr;
                return false;
            }

            *patch.slot = reinterpret_cast<ULONG_PTR>(replacement);
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    patch.slot = nullptr;
    return false;
}

bool PatchFrameMenuFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t delta =
        reinterpret_cast<unsigned char *>(replacement) -
        (patch.address + sizeof(patch.original));
    const std::int32_t relative = static_cast<std::int32_t>(delta);
    std::memcpy(patch.address + 1, &relative, sizeof(relative));
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    return true;
}

void RestoreFrameMenuFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    }
    patch.address = nullptr;
}

const char *FrameHelpDocsMessageText(
    unsigned int messageId
) {
    switch (messageId) {
    case 0x19:
        return "Help";
    case 0x20:
        return "No file association";
    case 0x21:
        return "No DDE association";
    case 0x22:
        return "File not found";
    case 0x24:
        return "Association incomplete";
    default:
        return "";
    }
}

DWORD WINAPI FakeFrameHelpDocsFormatMessageA(
    DWORD,
    LPCVOID,
    DWORD messageId,
    DWORD,
    LPSTR buffer,
    DWORD,
    va_list *
) {
    const char *const text = FrameHelpDocsMessageText(messageId);
    *reinterpret_cast<const char **>(buffer) = text;
    return static_cast<DWORD>(std::strlen(text));
}

HLOCAL WINAPI FakeFrameHelpDocsLocalFree(
    HLOCAL
) {
    return nullptr;
}

HINSTANCE WINAPI FakeFrameHelpDocsFindExecutableA(
    LPCSTR file,
    LPCSTR directory,
    LPSTR result
) {
    ++g_frameHelpDocsFindExecutableCalls;
    g_frameHelpDocsFindExecutableArgsOk =
        file != nullptr && std::strcmp(file, "Docs\\Index.html") == 0 &&
        directory == nullptr && result != nullptr;

    return reinterpret_cast<HINSTANCE>(g_frameHelpDocsFindExecutableResult);
}

HINSTANCE WINAPI FakeFrameHelpDocsShellExecuteA(
    HWND hwnd,
    LPCSTR operation,
    LPCSTR file,
    LPCSTR parameters,
    LPCSTR directory,
    INT showCommand
) {
    ++g_frameHelpDocsShellExecuteCalls;
    g_frameHelpDocsShellExecuteArgsOk =
        hwnd == g_RecoilApp_hWndMain && operation != nullptr &&
        std::strcmp(operation, "open") == 0 && file != nullptr &&
        std::strcmp(file, "Docs\\Index.html") == 0 && parameters == nullptr &&
        directory == nullptr && showCommand == SW_HIDE;
    return reinterpret_cast<HINSTANCE>(33);
}

int __fastcall FakeFrameHelpDocsMessageBoxA(
    CWnd *self,
    void *,
    LPCSTR text,
    LPCSTR caption,
    UINT type
) {
    ++g_frameHelpDocsMessageBoxCalls;
    g_frameHelpDocsMessageBoxArgsOk =
        self == g_frameHelpDocsExpectedMessageBoxWnd &&
        text != nullptr && std::strcmp(text, g_frameHelpDocsExpectedMessageText) == 0 &&
        caption != nullptr && std::strcmp(caption, "Help") == 0 && type == 0x30;
    return IDOK;
}

bool RunFrameHelpDocsScenario(
    UINT_PTR findExecutableResult,
    const char *expectedMessageText,
    bool expectShellExecute
) {
    g_frameHelpDocsFindExecutableResult = findExecutableResult;
    g_frameHelpDocsExpectedMessageText = expectedMessageText;
    g_frameHelpDocsFindExecutableCalls = 0;
    g_frameHelpDocsFindExecutableArgsOk = false;
    g_frameHelpDocsShellExecuteCalls = 0;
    g_frameHelpDocsShellExecuteArgsOk = false;
    g_frameHelpDocsMessageBoxCalls = 0;
    g_frameHelpDocsMessageBoxArgsOk = false;
    g_frameHelpDocsExpectedMessageBoxWnd = nullptr;

    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    HWND const hwnd = reinterpret_cast<HWND>(static_cast<std::uintptr_t>(0x12345678));

    g_RecoilApp_hWndMain = hwnd;
    alignas(CZRecoilFrame) unsigned char frameStorage[sizeof(CZRecoilFrame)] = {};
    CZRecoilFrame *const frame = reinterpret_cast<CZRecoilFrame *>(frameStorage);
    frame->m_hWnd = hwnd;
    g_frameHelpDocsExpectedMessageBoxWnd = frame;
    frame->OnMenuOpenHelpDocs();

    const bool ok =
        g_frameHelpDocsFindExecutableCalls == 1 && g_frameHelpDocsFindExecutableArgsOk &&
        (expectShellExecute
             ? (g_frameHelpDocsShellExecuteCalls == 1 && g_frameHelpDocsShellExecuteArgsOk &&
                g_frameHelpDocsMessageBoxCalls == 0)
             : (g_frameHelpDocsShellExecuteCalls == 0 &&
                g_frameHelpDocsMessageBoxCalls == 1 && g_frameHelpDocsMessageBoxArgsOk));

    g_RecoilApp_hWndMain = oldMainHwnd;
    g_frameHelpDocsExpectedMessageBoxWnd = nullptr;
    return ok;
}

void ResetFrameAboutDialogProbe() {
    g_frameAboutDialogCtorCalls = 0;
    g_frameAboutDialogDoModalCalls = 0;
    g_frameAboutDialogDtorCalls = 0;
    g_frameAboutDialogCtorArgsOk = false;
    g_frameAboutDialogFlowOk = true;
    g_frameAboutDialogThis = nullptr;
}

void __fastcall FakeFrameAboutCDialogCtor(
    CDialog *self,
    void *,
    UINT resourceId,
    CWnd *parentWnd
) {
    ++g_frameAboutDialogCtorCalls;
    g_frameAboutDialogThis = self;
    g_frameAboutDialogCtorArgsOk = resourceId == 0x67 && parentWnd == nullptr;
}

int __fastcall FakeFrameAboutCDialogDoModal(
    CDialog *self,
    void *
) {
    ++g_frameAboutDialogDoModalCalls;
    g_frameAboutDialogFlowOk =
        g_frameAboutDialogFlowOk && g_frameAboutDialogCtorCalls == 1 &&
        g_frameAboutDialogDtorCalls == 0 && self == g_frameAboutDialogThis;
    return IDOK;
}

void __fastcall FakeFrameAboutCDialogDtor(
    CDialog *self,
    void *
) {
    ++g_frameAboutDialogDtorCalls;
    g_frameAboutDialogFlowOk =
        g_frameAboutDialogFlowOk && g_frameAboutDialogCtorCalls == 1 &&
        g_frameAboutDialogDoModalCalls == 1 && self == g_frameAboutDialogThis;
}

bool PatchFrameAboutDialogMfcImports(
    RecoilFrameMenuImportPatch &ctorPatch,
    RecoilFrameMenuImportPatch &doModalPatch,
    RecoilFrameMenuImportPatch &dtorPatch
) {
    if (!PatchFrameMenuImportByOrdinal(
            "MFC42.DLL",
            kFrameMfc42CDialogResourceCtorOrdinal,
            reinterpret_cast<void *>(&FakeFrameAboutCDialogCtor),
            ctorPatch
        )) {
        return false;
    }

    if (!PatchFrameMenuImportByOrdinal(
            "MFC42.DLL",
            kFrameMfc42CDialogDoModalOrdinal,
            reinterpret_cast<void *>(&FakeFrameAboutCDialogDoModal),
            doModalPatch
        )) {
        RestoreFrameMenuImportPatch(ctorPatch);
        return false;
    }

    if (!PatchFrameMenuImportByOrdinal(
            "MFC42.DLL",
            kFrameMfc42CDialogDtorOrdinal,
            reinterpret_cast<void *>(&FakeFrameAboutCDialogDtor),
            dtorPatch
        )) {
        RestoreFrameMenuImportPatch(doModalPatch);
        RestoreFrameMenuImportPatch(ctorPatch);
        return false;
    }

    return true;
}

std::int32_t g_frameStartModeLoadCalls;
RecoilApp *g_frameStartModeLoadApp;
std::int32_t g_frameStartModeLoadMissionId;
const char *g_frameStartModeLoadZbdPath;
std::int32_t g_frameStartModeLoadSkipIntro;
std::int32_t g_frameStartModeLoadMissionFlags;

void *FrameStartModeLoadZbdAndSetupSensorTrackerProc() {
    union MemberToFunction {
        int ( RecoilApp::*member)(int, const char *, int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &RecoilApp::LoadZbdAndSetupSensorTracker;
    return thunk.function;
}

int __fastcall FakeFrameStartModeLoadZbdAndSetupSensorTracker(
    RecoilApp *self,
    void *,
    int missionId,
    const char *zbdPath,
    int skipIntroFmvMode,
    int missionFlags
) {
    ++g_frameStartModeLoadCalls;
    g_frameStartModeLoadApp = self;
    g_frameStartModeLoadMissionId = missionId;
    g_frameStartModeLoadZbdPath = zbdPath;
    g_frameStartModeLoadSkipIntro = skipIntroFmvMode;
    g_frameStartModeLoadMissionFlags = missionFlags;
    return 1;
}

void ResetFrameStartModeProbe() {
    g_frameStartModeLoadCalls = 0;
    g_frameStartModeLoadApp = nullptr;
    g_frameStartModeLoadMissionId = -1;
    g_frameStartModeLoadZbdPath = reinterpret_cast<const char *>(static_cast<std::uintptr_t>(1));
    g_frameStartModeLoadSkipIntro = -1;
    g_frameStartModeLoadMissionFlags = -1;
}
#endif
} // namespace

#if defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES)
extern "C" int czrecoil_frame_on_menu_open_help_docs_smoke(void) {
    RecoilFrameMenuImportPatch messageBoxPatch{};
    RecoilFrameMenuImportPatch findExecutablePatch{};
    RecoilFrameMenuImportPatch shellExecutePatch{};
    RecoilFrameMenuImportPatch formatMessagePatch{};
    RecoilFrameMenuImportPatch localFreePatch{};

    if (!PatchFrameMenuImportByName(
            "KERNEL32.dll",
            "FormatMessageA",
            reinterpret_cast<void *>(&FakeFrameHelpDocsFormatMessageA),
            formatMessagePatch
        )) {
        return 10;
    }

    if (!PatchFrameMenuImportByName(
            "KERNEL32.dll",
            "LocalFree",
            reinterpret_cast<void *>(&FakeFrameHelpDocsLocalFree),
            localFreePatch
        )) {
        RestoreFrameMenuImportPatch(formatMessagePatch);
        return 14;
    }

    if (!PatchFrameMenuImportByName(
            "SHELL32.dll",
            "FindExecutableA",
            reinterpret_cast<void *>(&FakeFrameHelpDocsFindExecutableA),
            findExecutablePatch
        )) {
        RestoreFrameMenuImportPatch(localFreePatch);
        RestoreFrameMenuImportPatch(formatMessagePatch);
        return 11;
    }

    if (!PatchFrameMenuImportByName(
            "SHELL32.dll",
            "ShellExecuteA",
            reinterpret_cast<void *>(&FakeFrameHelpDocsShellExecuteA),
            shellExecutePatch
        )) {
        RestoreFrameMenuImportPatch(findExecutablePatch);
        RestoreFrameMenuImportPatch(localFreePatch);
        RestoreFrameMenuImportPatch(formatMessagePatch);
        return 12;
    }

    if (!PatchFrameMenuImportByOrdinal(
            "MFC42.DLL",
            kFrameMfc42CWndMessageBoxAOrdinal,
            reinterpret_cast<void *>(&FakeFrameHelpDocsMessageBoxA),
            messageBoxPatch
        )) {
        RestoreFrameMenuImportPatch(shellExecutePatch);
        RestoreFrameMenuImportPatch(findExecutablePatch);
        RestoreFrameMenuImportPatch(localFreePatch);
        RestoreFrameMenuImportPatch(formatMessagePatch);
        return 13;
    }

    const bool successOk = RunFrameHelpDocsScenario(33, nullptr, true);
    const bool defaultFailureFallsThroughOk = RunFrameHelpDocsScenario(1, nullptr, true);
    const bool noAssociationOk =
        RunFrameHelpDocsScenario(0, FrameHelpDocsMessageText(0x20), false);
    const bool fileNotFoundOk =
        RunFrameHelpDocsScenario(2, FrameHelpDocsMessageText(0x22), false);
    const bool incompleteAssociationOk =
        RunFrameHelpDocsScenario(11, FrameHelpDocsMessageText(0x24), false);
    const bool noDdeAssociationOk =
        RunFrameHelpDocsScenario(31, FrameHelpDocsMessageText(0x21), false);

    RestoreFrameMenuImportPatch(messageBoxPatch);
    RestoreFrameMenuImportPatch(shellExecutePatch);
    RestoreFrameMenuImportPatch(findExecutablePatch);
    RestoreFrameMenuImportPatch(localFreePatch);
    RestoreFrameMenuImportPatch(formatMessagePatch);

    return successOk && defaultFailureFallsThroughOk && noAssociationOk && fileNotFoundOk &&
                   incompleteAssociationOk && noDdeAssociationOk
               ? 0
               : 1;
}

extern "C" int czrecoil_frame_on_menu_about_smoke(void) {
    RecoilFrameMenuImportPatch ctorPatch{};
    RecoilFrameMenuImportPatch doModalPatch{};
    RecoilFrameMenuImportPatch dtorPatch{};
    if (!PatchFrameAboutDialogMfcImports(ctorPatch, doModalPatch, dtorPatch)) {
        return 10;
    }

    ResetFrameAboutDialogProbe();
    CZRecoilFrame frame{};
    frame.OnMenuAbout();

    const bool ok = g_frameAboutDialogCtorCalls == 1 &&
                    g_frameAboutDialogDoModalCalls == 1 &&
                    g_frameAboutDialogDtorCalls == 1 && g_frameAboutDialogCtorArgsOk &&
                    g_frameAboutDialogFlowOk;

    RestoreFrameMenuImportPatch(dtorPatch);
    RestoreFrameMenuImportPatch(doModalPatch);
    RestoreFrameMenuImportPatch(ctorPatch);
    return ok ? 0 : 1;
}

extern "C" int cabout_dlg_constructor_smoke(void) {
    RecoilFrameMenuImportPatch ctorPatch{};
    if (!PatchFrameMenuImportByOrdinal(
            "MFC42.DLL",
            kFrameMfc42CDialogResourceCtorOrdinal,
            reinterpret_cast<void *>(&FakeFrameAboutCDialogCtor),
            ctorPatch
        )) {
        return 10;
    }

    ResetFrameAboutDialogProbe();
    alignas(CAboutDlg) unsigned char storage[sizeof(CAboutDlg)] = {};
    CAboutDlg *const dialog = new (storage) CAboutDlg();

    const bool ok = g_frameAboutDialogCtorCalls == 1 &&
                    g_frameAboutDialogCtorArgsOk &&
                    g_frameAboutDialogThis == static_cast<CDialog *>(dialog) &&
                    *reinterpret_cast<void **>(dialog) != nullptr;

    RestoreFrameMenuImportPatch(ctorPatch);
    return ok ? 0 : 1;
}

extern "C" int czrecoil_frame_start_mode_menu_handlers_smoke(void) {
    CodeFunctionPatch loadPatch{};
    if (!PatchFrameMenuFunctionJump(
            FrameStartModeLoadZbdAndSetupSensorTrackerProc(),
            reinterpret_cast<void *>(&FakeFrameStartModeLoadZbdAndSetupSensorTracker),
            loadPatch
        )) {
        return 1;
    }

    typedef void ( CZRecoilFrame::*StartModeHandler)();
    struct StartModeCase {
        StartModeHandler handler;
        int missionId;
        int archiveBanks;
    };

    const StartModeCase cases[] = {
        {&CZRecoilFrame::OnMenuStartMultiplayer, 1, 0x11},
        {&CZRecoilFrame::OnMenuStartCampaignMode, 2, 0x22},
        {&CZRecoilFrame::OnMenuStartCampaignMode2, 3, 0x33},
        {&CZRecoilFrame::OnMenuStartCampaignMode3, 4, 0x44},
        {&CZRecoilFrame::OnMenuStartCampaignMode4, 5, 0x55},
        {&CZRecoilFrame::OnMenuStartCampaignMode5, 6, 0x66},
    };

    CZRecoilFrame frame{};
    int result = 0;
    for (int index = 0; index < (int)(sizeof(cases) / sizeof(cases[0])); ++index) {
        ResetFrameStartModeProbe();
        frame.m_useArchiveBanks = cases[index].archiveBanks;

        (frame.*cases[index].handler)();

        if (g_frameStartModeLoadCalls != 1 || g_frameStartModeLoadApp != &g_RecoilApp ||
            g_frameStartModeLoadMissionId != cases[index].missionId ||
            g_frameStartModeLoadZbdPath != nullptr ||
            g_frameStartModeLoadSkipIntro != 1 ||
            g_frameStartModeLoadMissionFlags != cases[index].archiveBanks) {
            result = 10 + index;
            break;
        }
    }

    RestoreFrameMenuFunctionPatch(loadPatch);
    ResetFrameStartModeProbe();
    return result;
}
#endif

extern "C" int czrecoil_frame_on_menu_start_single_player_smoke(void) {
    unsigned char appBackup[sizeof(g_RecoilApp)];
    std::memcpy(appBackup, &g_RecoilApp, sizeof(g_RecoilApp));
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    const int oldMissionFlags = g_HudSensorTracker.missionFlags;

    zZbdSectionHandlerNode sentinel;
    zZbdManager manager;
    InitFrameMenuZbdManager(manager, sentinel);
    g_zUtil_ZbdManager = &manager;
    g_HudSensorTracker.missionFlags = 0;

    alignas(CZRecoilFrame) unsigned char frameStorage[sizeof(CZRecoilFrame)] = {};
    CZRecoilFrame *const frame = reinterpret_cast<CZRecoilFrame *>(frameStorage);
    frame->m_hWnd = reinterpret_cast<HWND>(0x22446688);
    RecoilFrameMenuTestRecoilApp vtableSource;
    RecoilFrameMenuTestAppState startupState;
    InstallFrameMenuAppHarness(vtableSource, startupState, *frame);
    g_RecoilApp.m_skipIntroFmv = 7;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 9;

    g_frameMenuStateEnterCount = 0;
    g_frameMenuStartEngineCount = 0;
    g_frameMenuShutdownEngineCount = 0;
    g_frameMenuExitInstanceCount = 0;
    g_frameMenuLastStartEngineHwnd = nullptr;

    frame->OnMenuStartSinglePlayer();

    const bool ok =
        g_RecoilApp.m_skipIntroFmv == 0 &&
        g_RecoilApp.m_missionFmvState.m_skipMissionFmv == 0 &&
        g_frameMenuStartEngineCount == 1 &&
        g_frameMenuLastStartEngineHwnd == reinterpret_cast<HWND>(0x22446688) &&
        g_frameMenuStateEnterCount == 1 &&
        g_RecoilApp.m_stateQueue.m_itemCount == 1 &&
        manager.sectionHandlerCount == 2;

    CleanupFrameMenuQueue(g_RecoilApp.m_stateQueue);
    ClearFrameMenuZbdHandlers(manager, sentinel);
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_zUtil_ZbdManager = oldZbdManager;
    std::memcpy(&g_RecoilApp, appBackup, sizeof(g_RecoilApp));
    return ok ? 0 : 1;
}

extern "C" int czrecoil_frame_on_open_file_dialog_smoke(void) {
    return RunFrameMenuOpenCampaignSmoke(false);
}

extern "C" int czrecoil_frame_on_menu_open_campaign_smoke(void) {
    return RunFrameMenuOpenCampaignSmoke(true);
}

extern "C" int czrecoil_frame_video_mode_menu_handlers_smoke(void) {
    std::int32_t mode = 0;
    std::int32_t acceleration = 0;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    CZRecoilFrame frame{};
    frame.m_vidMemFreeBytes = 0x800000;

    typedef void ( CZRecoilFrame::*Handler)();
    const Handler handlers[] = {
        &CZRecoilFrame::OnMenuSetVideoMode2,
        &CZRecoilFrame::OnMenuSetVideoMode3,
        &CZRecoilFrame::OnMenuSetVideoMode4,
        &CZRecoilFrame::OnMenuSetVideoMode5,
        &CZRecoilFrame::OnMenuSetVideoMode6,
        &CZRecoilFrame::OnMenuSetVideoMode7,
    };

    for (int index = 0; index < 6; ++index) {
        mode = 0;
        for (int stateIndex = 0; stateIndex < 6; ++stateIndex) {
            frame.m_videoModeCmdUiState[stateIndex] = -1;
        }

        (frame.*handlers[index])();
        const int expectedMode = index + 2;
        if (mode != expectedMode) {
            return index + 1;
        }

        for (int stateIndex = 0; stateIndex < 6; ++stateIndex) {
            const int expectedState = stateIndex == index ? 8 : 0;
            if (frame.m_videoModeCmdUiState[stateIndex] != expectedState) {
                return 10 + index;
            }
        }
    }

    return 0;
}

extern "C" int czrecoil_frame_menu_exit_game_smoke(void) {
    HWND hwnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil-exit-test",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        100,
        100,
        nullptr,
        nullptr,
        GetModuleHandleA(nullptr),
        nullptr
    );
    if (hwnd == nullptr) {
        return 1;
    }

    alignas(CZRecoilFrame) unsigned char frameStorage[sizeof(CZRecoilFrame)] = {};
    CZRecoilFrame *const frame = reinterpret_cast<CZRecoilFrame *>(frameStorage);
    frame->m_hWnd = hwnd;
    frame->OnMenuExitGame();

    MSG msg{};
    const BOOL found = PeekMessageA(
        &msg,
        hwnd,
        WM_CLOSE,
        WM_CLOSE,
        PM_REMOVE
    );
    DestroyWindow(hwnd);
    return found != 0 && msg.message == WM_CLOSE ? 0 : 2;
}
#endif

#if defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_MENU_SMOKES) || \
    !defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES)
extern "C" int czrecoil_frame_configure_mode_feature_flags_smoke(void) {
    std::int32_t mode = 6;
    std::int32_t acceleration = 0;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;

    CZRecoilFrame frame{};
    frame.ConfigureModeFeatureFlags();
    if (frame.m_videoModeCmdUiState[0] != 0 || frame.m_videoModeCmdUiState[1] != 0 ||
        frame.m_videoModeCmdUiState[2] != 0 || frame.m_videoModeCmdUiState[3] != 0 ||
        frame.m_videoModeCmdUiState[4] != 8 || frame.m_videoModeCmdUiState[5] != 0) {
        return 1;
    }

    acceleration = 1;
    mode = 7;
    frame.m_vidMemFreeBytes = 0x480000;
    frame.ConfigureModeFeatureFlags();
    if (frame.m_videoModeCmdUiState[0] != 1 || frame.m_videoModeCmdUiState[1] != 1 ||
        frame.m_videoModeCmdUiState[2] != 0 || frame.m_videoModeCmdUiState[3] != 0 ||
        frame.m_videoModeCmdUiState[4] != 0 || frame.m_videoModeCmdUiState[5] != 1) {
        return 2;
    }

    frame.m_vidMemFreeBytes = 0x480001;
    frame.ConfigureModeFeatureFlags();
    return frame.m_videoModeCmdUiState[0] == 1 && frame.m_videoModeCmdUiState[1] == 1 &&
                   frame.m_videoModeCmdUiState[2] == 0 && frame.m_videoModeCmdUiState[3] == 0 &&
                   frame.m_videoModeCmdUiState[4] == 0 && frame.m_videoModeCmdUiState[5] == 8
               ? 0
               : 3;
}

#if !defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_MENU_SMOKES)
extern "C" int czrecoil_frame_video_mode_menu_handlers_smoke(void) {
    std::int32_t mode = 0;
    std::int32_t acceleration = 0;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    CZRecoilFrame frame{};
    frame.m_vidMemFreeBytes = 0x800000;

    typedef void ( CZRecoilFrame::*Handler)();
    const Handler handlers[] = {
        &CZRecoilFrame::OnMenuSetVideoMode2,
        &CZRecoilFrame::OnMenuSetVideoMode3,
        &CZRecoilFrame::OnMenuSetVideoMode4,
        &CZRecoilFrame::OnMenuSetVideoMode5,
        &CZRecoilFrame::OnMenuSetVideoMode6,
        &CZRecoilFrame::OnMenuSetVideoMode7,
    };

    for (int index = 0; index < 6; ++index) {
        mode = 0;
        for (int stateIndex = 0; stateIndex < 6; ++stateIndex) {
            frame.m_videoModeCmdUiState[stateIndex] = -1;
        }

        (frame.*handlers[index])();
        const int expectedMode = index + 2;
        if (mode != expectedMode) {
            return index + 1;
        }

        for (int stateIndex = 0; stateIndex < 6; ++stateIndex) {
            const int expectedState = stateIndex == index ? 8 : 0;
            if (frame.m_videoModeCmdUiState[stateIndex] != expectedState) {
                return 10 + index;
            }
        }
    }

    return 0;
}
#endif

namespace {
HWND g_gameFrameValidityWindow;
int g_hwApiSelectEnsureCalls;
bool g_hwApiSelectEnsureArgsOk;
CZRecoilFrame *g_hwApiSelectExpectedFrame;
int g_hwApiSelectExpectedSelector;

void __fastcall FakeMenuSelectEnsureHwApiInitialized(
    CZRecoilFrame *self,
    void *,
    int selector
) {
    ++g_hwApiSelectEnsureCalls;
    g_hwApiSelectEnsureArgsOk =
        self == g_hwApiSelectExpectedFrame && selector == g_hwApiSelectExpectedSelector;
}

void *EnsureHwApiInitializedProc() {
    union MemberToFunction {
        void ( CZRecoilFrame::*member)(int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CZRecoilFrame::EnsureHwApiInitialized;
    return thunk.function;
}

bool RunHwApiSelectScenario(
    void ( CZRecoilFrame::*handler)(),
    int selector
) {
    CZRecoilFrame frame{};
    g_hwApiSelectExpectedFrame = &frame;
    g_hwApiSelectExpectedSelector = selector;
    g_hwApiSelectEnsureCalls = 0;
    g_hwApiSelectEnsureArgsOk = false;

    (frame.*handler)();

    return g_hwApiSelectEnsureCalls == 1 && g_hwApiSelectEnsureArgsOk;
}
} // namespace

#ifdef RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES
namespace {
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
    const std::intptr_t delta =
        reinterpret_cast<unsigned char *>(replacement) - (patch.address + sizeof(patch.original));
    const std::int32_t relative = static_cast<std::int32_t>(delta);
    std::memcpy(
        patch.address + 1,
        &relative,
        sizeof(relative)
    );

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

void RestoreFunctionPatch(
    CodeFunctionPatch &patch
) {
    if (patch.address == nullptr) {
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
    }
    patch.address = nullptr;
}
} // namespace
#endif

extern "C" int czrecoil_frame_set_hw_api_and_init_mode_smoke(void) {
    std::int32_t mode = 4;
    std::int32_t acceleration = 0;
    std::int32_t hwApi = -1;
    std::int32_t fullscreen = 0;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_zVideo_HwApiDeviceTable[2].m_videoMemTotalBytes = 0x900000;
    g_zVideo_HwApiDeviceTable[2].m_videoMemFreeBytes = 0x700000;
    g_zVideo_HwApiDeviceTable[2].m_textureMemTotalBytes = 0x200000;

    CZRecoilFrame frame{};
    frame.SetHwApiAndInitMode(2);

    if (frame.m_vidMemTotalBytes != 0x900000 || frame.m_vidMemFreeBytes != 0x500000 ||
        frame.m_fullscreenOption != 0 || frame.m_videoModeIndex != 4) {
        return 1;
    }

    if (hwApi != 1 || fullscreen != 1 || acceleration != 1 || g_zOpt_HwMode != 1 || mode != 5 ||
        replicate != 0 || display.width != 0x280 || display.height != 0x1e0) {
        return 2;
    }

    return frame.m_videoModeCmdUiState[0] == 1 && frame.m_videoModeCmdUiState[1] == 1 &&
                   frame.m_videoModeCmdUiState[2] == 0 && frame.m_videoModeCmdUiState[3] == 8 &&
                   frame.m_videoModeCmdUiState[4] == 0 && frame.m_videoModeCmdUiState[5] == 0
               ? 0
               : 3;
}

extern "C" int czrecoil_frame_init_fallback_mode_smoke(void) {
    std::int32_t mode = 5;
    std::int32_t acceleration = 1;
    std::int32_t hwApi = 1;
    std::int32_t fullscreen = 1;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    CZRecoilFrame frame{};
    frame.m_fullscreenOption = 0;
    frame.m_videoModeIndex = 3;
    frame.InitFallbackMode();

    return hwApi == 0 && acceleration == 0 && g_zOpt_HwMode == 0 && fullscreen == 0 && mode == 3 &&
                   replicate == 1 && render.width == 0x140 && render.height == 0xf0 &&
                   frame.m_videoModeCmdUiState[1] == 8
               ? 0
               : 1;
}

extern "C" int czrecoil_frame_ensure_hw_api_initialized_smoke(void) {
    std::int32_t mode = 4;
    std::int32_t acceleration = 0;
    std::int32_t hwApi = -1;
    std::int32_t fullscreen = 0;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_zVideo_HwApiDeviceTable[1].m_videoMemTotalBytes = 0x900000;
    g_zVideo_HwApiDeviceTable[1].m_videoMemFreeBytes = 0x700000;
    g_zVideo_HwApiDeviceTable[1].m_textureMemTotalBytes = 0x200000;

    CZRecoilFrame frame{};
    frame.m_hwApiCmdUiState[0] = 8;
    frame.m_fullscreenOption = 0;
    frame.m_videoModeIndex = 3;
    frame.EnsureHwApiInitialized(0);
    if (hwApi != -1 || acceleration != 0 || frame.m_hwApiCmdUiState[0] != 8) {
        return 1;
    }

    frame.m_hwApiCmdUiState[0] = 8;
    frame.EnsureHwApiInitialized(2);
    return frame.m_hwApiCmdUiState[0] == 0 && frame.m_hwApiCmdUiState[1] == 0 &&
                   frame.m_hwApiCmdUiState[2] == 8 && frame.m_hwApiCmdUiState[3] == 0 &&
                   hwApi == 1 && acceleration == 1 && mode == 5
               ? 0
               : 2;
}

extern "C" int czrecoil_frame_select_hw_api_menu_handlers_smoke(void) {
    CodeFunctionPatch ensurePatch{};
    if (!PatchFunctionJump(
            EnsureHwApiInitializedProc(),
            reinterpret_cast<void *>(&FakeMenuSelectEnsureHwApiInitialized),
            ensurePatch
        )) {
        return 10;
    }

    const bool api0Ok = RunHwApiSelectScenario(&CZRecoilFrame::OnMenuSelectHwApi0, 0);
    const bool api1Ok = RunHwApiSelectScenario(&CZRecoilFrame::OnMenuSelectHwApi1, 1);
    const bool api2Ok = RunHwApiSelectScenario(&CZRecoilFrame::OnMenuSelectHwApi2, 2);
    const bool api3Ok = RunHwApiSelectScenario(&CZRecoilFrame::OnMenuSelectHwApi3, 3);

    RestoreFunctionPatch(ensurePatch);
    return api0Ok && api1Ok && api2Ok && api3Ok ? 0 : 1;
}

#ifndef RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES
extern "C" int czrecoil_frame_init_startup_hw_api_from_options_smoke(void) {
    std::int32_t mode = 4;
    std::int32_t acceleration = 0;
    std::int32_t hwApi = 1;
    std::int32_t fullscreen = 0;
    std::int32_t replicate = -1;
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_MODE = &mode;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_zVideo_NumAcceptedDirectDrawDevices = 2;
    g_zVideo_HwApiDeviceTable[1].m_videoMemTotalBytes = 0x900000;
    g_zVideo_HwApiDeviceTable[1].m_videoMemFreeBytes = 0x700000;
    g_zVideo_HwApiDeviceTable[1].m_textureMemTotalBytes = 0x200000;

    CZRecoilFrame frame{};
    frame.InitStartupHwApiFromOptions();
    if (frame.m_hwApiCmdUiState[0] != 0 || frame.m_hwApiCmdUiState[2] != 8 ||
        frame.m_videoModeIndex != 4 || hwApi != 1 || acceleration != 1 || mode != 5) {
        return 1;
    }

    hwApi = 0;
    acceleration = 1;
    fullscreen = 1;
    mode = 4;
    CZRecoilFrame fallbackFrame{};
    fallbackFrame.InitStartupHwApiFromOptions();
    return fallbackFrame.m_hwApiCmdUiState[0] == 8 && fallbackFrame.m_videoModeIndex == 5 &&
                   fallbackFrame.m_fullscreenOption == 1 && hwApi == 0 && acceleration == 0 &&
                   fullscreen == 1 && mode == 5
               ? 0
               : 2;
}
#endif

namespace {
std::int32_t g_lastCmdUiEnable;
std::int32_t g_lastCmdUiCheck;
char g_lastCmdUiText[0x80];

struct FakeRecoilCmdUI : CCmdUI {
    void Enable(BOOL enable) {
        g_lastCmdUiEnable = enable;
    }

    void SetCheck(int check) {
        g_lastCmdUiCheck = check;
    }

    void SetText(LPCTSTR text) {
        std::strncpy(g_lastCmdUiText, text, sizeof(g_lastCmdUiText) - 1);
        g_lastCmdUiText[sizeof(g_lastCmdUiText) - 1] = '\0';
    }
};

void InitFakeRecoilCmdUI(FakeRecoilCmdUI &cmdUi) {
    cmdUi.m_pMenu = nullptr;
}
} // namespace

extern "C" int czrecoil_frame_menu_toggle_smoke(void) {
    std::int32_t cdAudio = 0;
    std::int32_t joystick = 0;
    std::int32_t fullscreen = 0;
    std::int32_t hudSw = 1;
    std::int32_t hudHw = 0;
    ZOPT_SOUND_CDAUDIO = &cdAudio;
    ZOPT_INPUT_JOYSTICK = &joystick;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_HUD_SW = &hudSw;
    ZOPT_HUD_HW = &hudHw;

    CZRecoilFrame frame{};
    FakeRecoilCmdUI cmdUi{};
    InitFakeRecoilCmdUI(cmdUi);

    frame.OnMenuToggleCDAudio();
    if (cdAudio != 1) {
        return 8;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateCDAudioCmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 9;
    }

    frame.OnMenuToggleJoystick();
    if (joystick != 1) {
        return 10;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateJoystickCmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 11;
    }

    frame.OnMenuToggleFullscreen();
    if (fullscreen != 1) {
        return 1;
    }

    frame.OnMenuToggleFullscreen();
    if (fullscreen != 0) {
        return 2;
    }

    g_zOpt_HwMode = 0;
    frame.OnMenuToggleHud();
    if (hudSw != 0) {
        return 3;
    }

    g_lastCmdUiEnable = 0;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateHudCmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 0) {
        return 4;
    }

    const int oldMissionFlags = g_HudSensorTracker.missionFlags;
    const int oldUseArchiveBanksFlag = g_zSnd_UseArchiveBanksFlag;

    HMENU const menu = CreateMenu();
    if (menu == nullptr) {
        return 5;
    }
    if (AppendMenuA(menu, MF_STRING, 0x9c6b, "Archive banks") == 0) {
        DestroyMenu(menu);
        return 6;
    }

    frame.m_mainMenu.m_hMenu = menu;
    frame.m_useArchiveBanks = 0;
    g_HudSensorTracker.missionFlags = 99;
    g_zSnd_UseArchiveBanksFlag = 99;

    frame.OnMenuToggleArchiveBanks();
    const bool enabledOk = frame.m_useArchiveBanks == 1 && g_HudSensorTracker.missionFlags == 1 &&
                           g_zSnd_UseArchiveBanksFlag == 1 &&
                           (GetMenuState(menu, 0x9c6b, MF_BYCOMMAND) & MF_CHECKED) != 0;

    frame.OnMenuToggleArchiveBanks();
    const bool disabledOk = frame.m_useArchiveBanks == 0 && g_HudSensorTracker.missionFlags == 0 &&
                            g_zSnd_UseArchiveBanksFlag == 0 &&
                            (GetMenuState(menu, 0x9c6b, MF_BYCOMMAND) & MF_CHECKED) == 0;

    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_zSnd_UseArchiveBanksFlag = oldUseArchiveBanksFlag;
    frame.m_mainMenu.m_hMenu = nullptr;
    DestroyMenu(menu);
    return enabledOk && disabledOk ? 0 : 7;
}

extern "C" int czrecoil_frame_toggle_texture_packs_smoke(void) {
    const int oldTexturePackLoadState = g_zVid_TexturePackLoadState;

    HMENU const menu = CreateMenu();
    if (menu == nullptr) {
        return 10;
    }
    if (AppendMenuA(menu, MF_STRING, 0x9c7b, "Texture packs") == 0) {
        DestroyMenu(menu);
        return 11;
    }

    CZRecoilFrame frame{};
    frame.m_mainMenu.m_hMenu = menu;
    g_zVid_TexturePackLoadState = 0;

    frame.OnMenuToggleTexturePacks();
    const bool enabledOk = g_zVid_TexturePackLoadState == 1 &&
                           (GetMenuState(menu, 0x9c7b, MF_BYCOMMAND) & MF_CHECKED) != 0;

    frame.OnMenuToggleTexturePacks();
    const bool disabledOk = g_zVid_TexturePackLoadState == 0 &&
                            (GetMenuState(menu, 0x9c7b, MF_BYCOMMAND) & MF_CHECKED) == 0;

    g_zVid_TexturePackLoadState = oldTexturePackLoadState;
    frame.m_mainMenu.m_hMenu = nullptr;
    DestroyMenu(menu);
    return enabledOk && disabledOk ? 0 : 12;
}
#endif

#if defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_MENU_SMOKES) || \
    !defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES)
extern "C" int czrecoil_frame_update_video_mode_cmd_ui_smoke(void) {
    CZRecoilFrame frame{};
    FakeRecoilCmdUI cmdUi{};
    InitFakeRecoilCmdUI(cmdUi);

    frame.m_videoModeCmdUiState[0] = 1;
    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateVideoMode2CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 0 || g_lastCmdUiCheck != 0) {
        return 1;
    }

    frame.m_videoModeCmdUiState[1] = 8;
    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateVideoMode3CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 2;
    }

    frame.m_videoModeCmdUiState[2] = 0;
    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateVideoMode4CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 0) {
        return 3;
    }

    frame.m_videoModeCmdUiState[3] = 8;
    frame.m_videoModeCmdUiState[4] = 1;
    frame.m_videoModeCmdUiState[5] = 0;
    frame.OnUpdateVideoMode5CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 4;
    }

    frame.OnUpdateVideoMode6CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 0 || g_lastCmdUiCheck != 0) {
        return 5;
    }

    frame.OnUpdateVideoMode7CmdUI(&cmdUi);
    return g_lastCmdUiEnable == 1 && g_lastCmdUiCheck == 0 ? 0 : 6;
}
#endif

#if defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_MENU_SMOKES) || \
    !defined(RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES)
extern "C" int czrecoil_frame_hw_api_menu_cmd_ui_smoke(void) {
    CZRecoilFrame frame{};
    FakeRecoilCmdUI cmdUi{};
    InitFakeRecoilCmdUI(cmdUi);

    frame.m_hwApiCmdUiState[0] = 8;
    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateHwApi0CmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 1;
    }

    strcpy_s(g_zVideo_HwApiDeviceTable[0].m_driverDescription, "Api One");
    strcpy_s(g_zVideo_HwApiDeviceTable[0].m_driverName, "drv1");
    frame.m_acceptedD3DDeviceCount = 2;
    frame.m_hwApiCmdUiState[1] = 0;
    g_lastCmdUiCheck = -1;
    g_lastCmdUiText[0] = '\0';
    frame.OnUpdateHwApi1CmdUI(&cmdUi);
    if (g_lastCmdUiCheck != 0 ||
        std::strcmp(g_lastCmdUiText, "Accelerator - Api One (drv1)") != 0) {
        return 2;
    }

    strcpy_s(g_zVideo_HwApiDeviceTable[1].m_driverDescription, "Test Device");
    strcpy_s(g_zVideo_HwApiDeviceTable[1].m_driverName, "testdrv");
    frame.m_hwApiCmdUiState[2] = 8;
    g_lastCmdUiCheck = -1;
    g_lastCmdUiText[0] = '\0';
    frame.OnUpdateHwApi2CmdUI(&cmdUi);
    if (g_lastCmdUiCheck != 1 ||
        std::strcmp(g_lastCmdUiText, "Accelerator - Test Device (testdrv)") != 0) {
        return 3;
    }

    HMENU menu = CreateMenu();
    if (menu == nullptr) {
        return 4;
    }

    CMenu menuProxy;
    menuProxy.m_hMenu = menu;
    cmdUi.m_pMenu = &menuProxy;
    frame.m_acceptedD3DDeviceCount = 2;
    frame.m_hwApiMenuCommandIds[3] = 0x7003;
    AppendMenuA(menu, MF_STRING, frame.m_hwApiMenuCommandIds[3], "extra");
    frame.OnUpdateHwApi3CmdUI(&cmdUi);
    if (GetMenuState(menu, frame.m_hwApiMenuCommandIds[3], MF_BYCOMMAND) != 0xffffffff) {
        DestroyMenu(menu);
        return 5;
    }

    AppendMenuA(menu, MF_STRING, 0x9c4e, "fullscreen");
    frame.OnUpdateFullscreenCmdUI(&cmdUi);
    const bool fullscreenRemoved = GetMenuState(menu, 0x9c4e, MF_BYCOMMAND) == 0xffffffff;
    DestroyMenu(menu);
    return fullscreenRemoved ? 0 : 6;
}

#endif

#ifndef RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES
extern "C" int czrecoil_frame_audio_input_menu_smoke(void) {
    std::int32_t cdAudio = 0;
    std::int32_t joystick = 0;
    std::int32_t audioApi = 0;
    ZOPT_SOUND_CDAUDIO = &cdAudio;
    ZOPT_INPUT_JOYSTICK = &joystick;
    ZOPT_AUDIO_API = &audioApi;
    g_zSnd_IsInitialized = 0;
    g_zSnd_ActiveBackend = 0;

    CZRecoilFrame frame{};
    FakeRecoilCmdUI cmdUi{};
    InitFakeRecoilCmdUI(cmdUi);

    g_lastCmdUiEnable = -1;
    MfcCmdUI::EnableAlways(&cmdUi);
    if (g_lastCmdUiEnable != 1) {
        return 9;
    }

    frame.OnMenuToggleCDAudio();
    if (cdAudio != 1) {
        return 1;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateCDAudioCmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 2;
    }

    frame.OnMenuToggleJoystick();
    if (joystick != 1) {
        return 3;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateJoystickCmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 4;
    }

    frame.OnMenuSelectA3D();
    if (audioApi != 1 || g_zSnd_ActiveBackend != 1) {
        return 5;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateA3DCmdUI(&cmdUi);
    if (g_lastCmdUiEnable != 1 || g_lastCmdUiCheck != 1) {
        return 6;
    }

    frame.OnMenuSelectDirectSound();
    if (audioApi != 0 || g_zSnd_ActiveBackend != 0) {
        return 7;
    }

    g_lastCmdUiEnable = -1;
    g_lastCmdUiCheck = -1;
    frame.OnUpdateDirectSoundCmdUI(&cmdUi);
    return g_lastCmdUiEnable == 1 && g_lastCmdUiCheck == 1 ? 0 : 8;
}

extern "C" int czrecoil_frame_menu_exit_game_smoke(void) {
    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-exit-test", WS_OVERLAPPEDWINDOW, 0, 0, 100,
                                100, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    CZRecoilFrame frame{};
    frame.m_hWnd = hwnd;
    frame.OnMenuExitGame();

    MSG msg{};
    const BOOL found = PeekMessageA(&msg, hwnd, WM_CLOSE, WM_CLOSE, PM_REMOVE);
    DestroyWindow(hwnd);
    return found != 0 && msg.message == WM_CLOSE ? 0 : 2;
}

extern "C" int czgame_frame_is_window_valid_smoke(void) {
    if (CZGameFrame::IsWindowValid(nullptr) != 0) {
        return 1;
    }

    g_gameFrameValidityWindow = CreateWindowExA(0, "STATIC", "recoil-validity-test",
                                                WS_OVERLAPPEDWINDOW | WS_DISABLED, 0, 0, 10, 10,
                                                nullptr, nullptr, GetModuleHandleA(nullptr),
                                                nullptr);
    alignas(CWnd) unsigned char storage[sizeof(CWnd)] = {};
    CWnd *disabledWnd = reinterpret_cast<CWnd *>(storage);
    disabledWnd->m_hWnd = g_gameFrameValidityWindow;
    if (CZGameFrame::IsWindowValid(disabledWnd) != 1) {
        DestroyWindow(g_gameFrameValidityWindow);
        return 2;
    }

    EnableWindow(g_gameFrameValidityWindow, TRUE);
    const int result = CZGameFrame::IsWindowValid(disabledWnd) == 0 ? 0 : 3;
    DestroyWindow(g_gameFrameValidityWindow);
    return result;
}

extern "C" int czgame_frame_build_window_title_smoke(void) {
    CZGameFrame frame{};
    alignas(CString) unsigned char storage[sizeof(CString)];
    auto *title = reinterpret_cast<CString *>(storage);

    CString *returned = frame.BuildWindowTitle(title);
    const bool ok = returned == title && (const char *)(*title) != nullptr &&
                    std::strcmp((const char *)(*title), "Zipper Interactive") == 0;
    title->~CString();

    return ok ? 0 : 1;
}

#ifndef RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES
extern "C" int czgame_frame_constructor_smoke(void) {
    CZGameFrame frame{};
    CZGameFrame *returned = frame.Constructor(nullptr);
    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(&frame);
    if (returned == &frame && constructedFrameVtable != 0 &&
        constructedFrameVtable != CZGameFrame::GetRuntimeClass() &&
        frame.m_gameBitmap.m_hObject == nullptr) {
        frame.Destructor();
        return frame.m_gameBitmap.m_hObject == nullptr ? 0 : 2;
    }

    return 1;
}

extern "C" int czgame_frame_create_object_smoke(void) {
    CZGameFrame *const frame = CZGameFrame::CreateObject();
    if (frame == nullptr) {
        return 1;
    }

    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(frame);
    const bool ok = constructedFrameVtable != 0 &&
                    constructedFrameVtable != CZGameFrame::GetRuntimeClass() &&
                    frame->m_gameBitmap.m_hObject == nullptr;

    frame->Destructor();
    ::operator delete(frame);
    return ok ? 0 : 2;
}
#endif

extern "C" int czgame_frame_destructor_smoke(void) {
    CZGameFrame frame{};
    frame.Constructor(nullptr);

    frame.Destructor();

    return frame.m_gameBitmap.m_hObject == nullptr ? 0 : 1;
}

#ifndef RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES
extern "C" int czrecoil_frame_constructor_smoke(void) {
    g_RecoilApp.Constructor();

    HINSTANCE instance = GetModuleHandleA(nullptr);
    if (AfxWinInit(instance, nullptr, GetCommandLineA(), SW_HIDE) == 0) {
        return 1;
    }

    WNDCLASSA wndClass{};
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "RecoilClass";
    RegisterClassA(&wndClass);

    CodeFunctionPatch createExPatch{};
    CodeFunctionPatch setWindowTextPatch{};
    CodeFunctionPatch centerWindowPatch{};
    if (!PatchFunctionJump(CWndCreateExProc(), reinterpret_cast<void *>(&FakeCWndCreateEx),
                           createExPatch) ||
        !PatchFunctionJump(CWndSetWindowTextAProc(),
                           reinterpret_cast<void *>(&FakeCWndSetWindowTextA),
                           setWindowTextPatch) ||
        !PatchFunctionJump(CWndCenterWindowProc(), reinterpret_cast<void *>(&FakeCWndCenterWindow),
                           centerWindowPatch)) {
        RestoreFunctionPatch(centerWindowPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreFunctionPatch(createExPatch);
        return 2;
    }

    CZRecoilFrame frame{};
    g_zVid_AcceptedHardwareRendererCount = 5;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    CZRecoilFrame *returned = frame.Constructor();

    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(&frame);
    const bool constructed = returned == &frame && constructedFrameVtable != 0 &&
                             constructedFrameVtable != CZRecoilFrame::GetRuntimeClass();
    const bool fieldsOk =
        frame.m_openZbdFilePath[0] == '\0' && frame.m_useArchiveBanks == 1 &&
        frame.m_cmdlineFlag == 1 && frame.m_campaignsOnlyMode == 0 &&
        frame.m_acceptedD3DDeviceCount == g_zVid_AcceptedHardwareRendererCount &&
        frame.m_hwApiCmdUiState[0] == 0 &&
        frame.m_hwApiCmdUiState[1] == 0 && frame.m_hwApiCmdUiState[2] == 0 &&
        frame.m_hwApiCmdUiState[3] == 0 && frame.m_hwApiMenuCommandIds[0] == 0x9c83 &&
        frame.m_hwApiMenuCommandIds[1] == 0x9c72 &&
        frame.m_hwApiMenuCommandIds[2] == 0x9c75 &&
        frame.m_hwApiMenuCommandIds[3] == 0x9c76;
    const bool globalsOk = g_zSnd_UseArchiveBanksFlag == 1;

    const int failure = !constructed ? 2 : (!fieldsOk ? 3 : (globalsOk ? 0 : 4));
    RestoreFunctionPatch(centerWindowPatch);
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreFunctionPatch(createExPatch);
    return failure;
}

extern "C" int czrecoil_frame_create_object_smoke(void) {
    g_RecoilApp.Constructor();

    HINSTANCE instance = GetModuleHandleA(nullptr);
    if (AfxWinInit(instance, nullptr, GetCommandLineA(), SW_HIDE) == 0) {
        return 1;
    }

    WNDCLASSA wndClass{};
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "RecoilClass";
    RegisterClassA(&wndClass);

    CodeFunctionPatch createExPatch{};
    CodeFunctionPatch setWindowTextPatch{};
    CodeFunctionPatch centerWindowPatch{};
    if (!PatchFunctionJump(CWndCreateExProc(), reinterpret_cast<void *>(&FakeCWndCreateEx),
                           createExPatch) ||
        !PatchFunctionJump(CWndSetWindowTextAProc(),
                           reinterpret_cast<void *>(&FakeCWndSetWindowTextA),
                           setWindowTextPatch) ||
        !PatchFunctionJump(CWndCenterWindowProc(), reinterpret_cast<void *>(&FakeCWndCenterWindow),
                           centerWindowPatch)) {
        RestoreFunctionPatch(centerWindowPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreFunctionPatch(createExPatch);
        return 2;
    }

    g_zVid_AcceptedHardwareRendererCount = 6;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    CZRecoilFrame *const frame = CZRecoilFrame::CreateObject();
    if (frame == nullptr) {
        RestoreFunctionPatch(centerWindowPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreFunctionPatch(createExPatch);
        return 2;
    }

    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(frame);
    const bool ok = constructedFrameVtable != 0 &&
                    constructedFrameVtable != CZRecoilFrame::GetRuntimeClass() &&
                    frame->m_useArchiveBanks == 1 &&
                    frame->m_acceptedD3DDeviceCount == g_zVid_AcceptedHardwareRendererCount &&
                    g_zSnd_UseArchiveBanksFlag == 1;

    frame->Destructor();
    ::operator delete(frame);
    RestoreFunctionPatch(centerWindowPatch);
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreFunctionPatch(createExPatch);
    return ok ? 0 : 3;
}
#endif

extern "C" int recoil_app_create_main_wnd_smoke(void) {
    g_RecoilApp.Constructor();

    HINSTANCE instance = GetModuleHandleA(nullptr);
    if (AfxWinInit(instance, nullptr, GetCommandLineA(), SW_HIDE) == 0) {
        return 1;
    }

    WNDCLASSA wndClass{};
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = instance;
    wndClass.lpszClassName = "RecoilClass";
    RegisterClassA(&wndClass);

    CodeFunctionPatch createExPatch{};
    CodeFunctionPatch setWindowTextPatch{};
    CodeFunctionPatch centerWindowPatch{};
    if (!PatchFunctionJump(CWndCreateExProc(), reinterpret_cast<void *>(&FakeCWndCreateEx),
                           createExPatch) ||
        !PatchFunctionJump(CWndSetWindowTextAProc(),
                           reinterpret_cast<void *>(&FakeCWndSetWindowTextA),
                           setWindowTextPatch) ||
        !PatchFunctionJump(CWndCenterWindowProc(), reinterpret_cast<void *>(&FakeCWndCenterWindow),
                           centerWindowPatch)) {
        RestoreFunctionPatch(centerWindowPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreFunctionPatch(createExPatch);
        return 2;
    }

    g_zVid_AcceptedHardwareRendererCount = 4;
    g_zVid_TexturePackLoadState = 1;
    g_zSnd_UseArchiveBanksFlag = 0;
    g_CZRecoilFrame_HasWolApi = 0;

    CZRecoilFrame *frame = g_RecoilApp.CreateMainWnd();
    if (frame == nullptr) {
        RestoreFunctionPatch(centerWindowPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreFunctionPatch(createExPatch);
        return 2;
    }

    const auto constructedFrameVtable = *reinterpret_cast<RecoilPtr32 *>(frame);
    const int failure = constructedFrameVtable != 0 &&
                                constructedFrameVtable != CZRecoilFrame::GetRuntimeClass() &&
                                frame->m_useArchiveBanks == 1 &&
                                frame->m_acceptedD3DDeviceCount ==
                                    g_zVid_AcceptedHardwareRendererCount &&
                                g_zSnd_UseArchiveBanksFlag == 1
                            ? 0
                            : 3;
    RestoreFunctionPatch(centerWindowPatch);
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreFunctionPatch(createExPatch);
    return failure;
}
#endif

extern "C" int czrecoil_frame_destructor_smoke(void) {
    void *storage = ::operator new(sizeof(CZRecoilFrame));
    CZRecoilFrame *const frame = (CZRecoilFrame *)storage;
    std::memset(frame, 0, sizeof(*frame));
    reinterpret_cast<CZGameFrame *>(frame)->Constructor(nullptr);
    new (&frame->m_mainMenu) CMenu();
    frame->m_mainMenu.m_hMenu = CreateMenu();
    if (frame->m_mainMenu.m_hMenu == nullptr) {
        ::operator delete(storage);
        return 1;
    }

    frame->Destructor();
    const bool ok = frame->m_mainMenu.m_hMenu == nullptr;
    ::operator delete(storage);
    return ok ? 0 : 2;
}

#ifndef RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES
namespace {
std::uint32_t g_lastIdleWParam;
std::uint32_t g_lastIdleLParam;
std::int32_t g_appActivateCalls;
std::int32_t g_appDeactivateCalls;
std::int32_t g_cFrameWndOnCloseCalls;
std::int32_t g_cFrameWndOnCreateCalls;
std::int32_t g_cFrameWndOnCreateResult;
std::int32_t g_cFrameWndOnDestroyCalls;
std::int32_t g_cFrameWndOnActivateCalls;
bool g_cFrameWndOnActivateArgsOk;
std::int32_t g_stateWndActivateCalls;
std::int32_t g_lastStateWndActivateValue;
std::int32_t g_zInputOnAppActivateCalls;
std::int32_t g_zInputOnAppDeactivateCalls;
std::int32_t g_zGameReturnOnlyStubCalls;
std::int32_t g_zVideoRestoreIconicCalls;
std::int32_t g_gdiDeleteObjectCalls;
std::int32_t g_onDestroyCallOrder[5];
std::int32_t g_onDestroyCallCount;
std::int32_t g_onActivateCallOrder[5];
std::int32_t g_onActivateCallCount;
std::int32_t g_cFrameWndOnSizeCalls;
bool g_cFrameWndOnSizeArgsOk;
std::int32_t g_findResourceHandleCalls;
std::int32_t g_loadBitmapCalls;
HINSTANCE g_fakeResourceHandle = reinterpret_cast<HINSTANCE>(static_cast<std::uintptr_t>(0x1357));
HBITMAP g_fakeBitmapHandle = reinterpret_cast<HBITMAP>(static_cast<std::uintptr_t>(0x2468));
LPCSTR g_lastResourceName;
LPCSTR g_lastResourceType;
LPCSTR g_lastBitmapName;
HINSTANCE g_lastBitmapInstance;
std::int32_t g_paintCreateCompatibleDcCalls;
std::int32_t g_paintSelectObjectCalls;
std::int32_t g_paintBitBltCalls;
std::int32_t g_paintStretchBltCalls;
std::int32_t g_paintDeleteDcCalls;
HDC g_paintCompatibleDc = reinterpret_cast<HDC>(static_cast<std::uintptr_t>(0x12345678));
HDC g_paintLastDestDc;
HDC g_paintLastSourceDc;
HGDIOBJ g_paintLastSelectedObject;
RECT g_paintLastDestRect;
RECT g_paintLastSourceRect;
DWORD g_paintLastRasterOp;
std::int32_t g_wolMenuZlocCalls;
std::int32_t g_wolMenuZlocIds[2];
std::int32_t g_wolMenuVerifyCalls;
std::int32_t g_wolMenuVerifyResult;
bool g_wolMenuVerifyArgsOk;
std::int32_t g_wolMenuModalCalls;
std::int32_t g_wolMenuModalResult;
std::int32_t g_wolMenuModalSelectedMissionIndex;
std::int32_t g_wolMenuLoadCalls;
RecoilApp *g_wolMenuLoadApp;
std::int32_t g_wolMenuLoadMissionId;
const char *g_wolMenuLoadZbdPath;
std::int32_t g_wolMenuLoadSkipIntro;
std::int32_t g_wolMenuLoadMissionFlags;
std::int32_t g_mpMenuCoInitializeCalls;
HRESULT g_mpMenuCoInitializeResult;
std::int32_t g_mpMenuInitRuntimeCalls;
GUID *g_mpMenuInitRuntimeGuid;
std::int32_t g_mpMenuShutdownRuntimeCalls;
std::int32_t g_mpMenuDoModalCalls;
std::int32_t g_mpMenuDoModalResult;
std::int32_t g_mpMenuDialogShouldEnterHostSetup;
std::int32_t g_mpMenuDialogSelectedSessionIndex;
const char *g_mpMenuDialogPlayerName;
std::int32_t g_mpMenuSetPlayerNameCalls;
const char *g_mpMenuLastPlayerName;
std::int32_t g_mpMenuSetNetworkEnabledCalls;
std::int32_t g_mpMenuLastNetworkEnabled;
std::int32_t g_mpMenuLoadStartCalls;
RecoilApp *g_mpMenuLoadStartApp;
std::int32_t g_mpMenuQueueEnterCalls;
std::int32_t g_mpMenuQueueEnterFlag;
std::int32_t g_mpMenuOpenSessionCalls;
std::int32_t g_mpMenuOpenSessionResult;
std::int32_t g_mpMenuOpenSelectedSessionIndex;
std::int32_t g_mpMenuOpenEventCode;
std::int32_t g_mpMenuOpenStatusFlags;
std::int32_t g_mpMenuOpenValueOrTime;
std::int32_t g_mpMenuOpenAuxParam;
std::int32_t g_mpMenuCreateLocalPlayerCalls;
const char *g_mpMenuCreateLocalPlayerName;
std::int32_t g_mpMenuRegisterPacketCalls;
std::int32_t g_mpMenuRegisterPacketType;
std::int32_t g_mpMenuRegisterPacketMode;
zNetworkPacketHandler g_mpMenuRegisterPacketHandler;
std::int32_t g_mpMenuSetStatusCalls;
unsigned int g_mpMenuStatusFlags;
std::int32_t g_mpMenuSetRuntimeTimerCalls;
HudSensorTracker *g_mpMenuSetRuntimeTimerThis;
std::int32_t g_mpMenuTimerRaw;
std::int32_t g_mpMenuGoalValue;
std::int32_t g_mpMenuLoadSetupCalls;
RecoilApp *g_mpMenuLoadSetupApp;
std::int32_t g_mpMenuLoadSetupMissionId;
const char *g_mpMenuLoadSetupZbdPath;
std::int32_t g_mpMenuLoadSetupSkipIntro;
std::int32_t g_mpMenuLoadSetupMissionFlags;
std::int32_t g_cWndCreateExCalls;
std::int32_t g_cWndSetWindowTextCalls;
std::int32_t g_cWndCenterWindowCalls;

struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

struct FakeGameFrameApp : CZGameFrameApp {
    void OnActivate() {
        ++g_appActivateCalls;
        if (g_onActivateCallCount < 5) {
            g_onActivateCallOrder[g_onActivateCallCount] = 6;
        }
        ++g_onActivateCallCount;
    }

    std::int32_t OnIdleOrDispatch(std::uint32_t wParam, std::uint32_t lParam) {
        g_lastIdleWParam = wParam;
        g_lastIdleLParam = lParam;
        return 0x1234;
    }

    void OnDeactivate() {
        ++g_appDeactivateCalls;
        if (g_onActivateCallCount < 5) {
            g_onActivateCallOrder[g_onActivateCallCount] = 3;
        }
        ++g_onActivateCallCount;
    }
};

struct FakeActivateState : RecoilApp_IState {
    void OnWndActivate(std::uint32_t nState) {
        ++g_stateWndActivateCalls;
        g_lastStateWndActivateValue = static_cast<std::int32_t>(nState);
        if (g_onActivateCallCount < 5) {
            g_onActivateCallOrder[g_onActivateCallCount] = 2;
        }
        ++g_onActivateCallCount;
    }
};

struct CFrameWndOnActivateAccess : CFrameWnd {
    using CFrameWnd::OnActivate;
};

struct CFrameWndOnSizeAccess : CFrameWnd {
    using CFrameWnd::OnSize;
};

struct CFrameWndOnCloseAccess : CFrameWnd {
    using CFrameWnd::OnClose;
};

struct CFrameWndOnCreateAccess : CFrameWnd {
    using CFrameWnd::OnCreate;
};

struct CFrameWndOnDestroyAccess : CFrameWnd {
    using CFrameWnd::OnDestroy;
};

struct CWndCreateExAccess : CWnd {
    using CWnd::CreateEx;
};

struct CWndSetWindowTextAccess : CWnd {
    using CWnd::SetWindowText;
};

struct CWndCenterWindowAccess : CWnd {
    using CWnd::CenterWindow;
};

struct HudSensorTrackerSetRuntimeTimerAccess : HudSensorTracker {
    using HudSensorTracker::SetRuntimeTimerSecAndGoalValue;
};

void RecordOnDestroyCall(std::int32_t callId) {
    if (g_onDestroyCallCount < 5) {
        g_onDestroyCallOrder[g_onDestroyCallCount] = callId;
    }
    ++g_onDestroyCallCount;
}

void RecordOnActivateCall(std::int32_t callId) {
    if (g_onActivateCallCount < 5) {
        g_onActivateCallOrder[g_onActivateCallCount] = callId;
    }
    ++g_onActivateCallCount;
}

int FakeDestroyCachedLocalPlayer() {
    RecordOnDestroyCall(1);
    return 1;
}

void FakeZInputOnAppActivate() {
    ++g_zInputOnAppActivateCalls;
    RecordOnActivateCall(7);
}

void FakeZInputOnAppDeactivate() {
    ++g_zInputOnAppDeactivateCalls;
    RecordOnActivateCall(4);
}

void FakeZGameReturnOnlyStub() {
    ++g_zGameReturnOnlyStubCalls;
    RecordOnActivateCall(5);
}

void FakeZVideoRestoreIconicFullscreenWindowIfNeeded() {
    ++g_zVideoRestoreIconicCalls;
    RecordOnActivateCall(8);
}

int FakeShutdownVideoSystem() {
    RecordOnDestroyCall(2);
    return 1;
}

int FakeZsndCdStop() {
    RecordOnDestroyCall(3);
    return 1;
}

int __fastcall FakeCFrameWndOnCreate(CFrameWnd *, void *, CREATESTRUCTA *) {
    ++g_cFrameWndOnCreateCalls;
    return g_cFrameWndOnCreateResult;
}

void __fastcall FakeCFrameWndOnClose(CFrameWnd *, void *) {
    ++g_cFrameWndOnCloseCalls;
}

void __fastcall FakeCFrameWndOnActivate(CFrameWnd *, void *, unsigned int nState,
                                             CWnd *pWndOther, BOOL bMinimized) {
    ++g_cFrameWndOnActivateCalls;
    g_cFrameWndOnActivateArgsOk =
        (nState == 0 || nState == 1) && pWndOther == 0 &&
        (bMinimized == TRUE || bMinimized == FALSE);
    RecordOnActivateCall(1);
}

void __fastcall FakeCFrameWndOnDestroy(CFrameWnd *, void *) {
    ++g_cFrameWndOnDestroyCalls;
    RecordOnDestroyCall(4);
}

BOOL __fastcall FakeCGdiObjectDeleteObject(CGdiObject *, void *) {
    ++g_gdiDeleteObjectCalls;
    RecordOnDestroyCall(5);
    return TRUE;
}

HINSTANCE __stdcall FakeAfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType) {
    ++g_findResourceHandleCalls;
    g_lastResourceName = resourceName;
    g_lastResourceType = resourceType;
    return g_fakeResourceHandle;
}

HBITMAP WINAPI FakeLoadBitmapA(HINSTANCE instance, LPCSTR bitmapName) {
    ++g_loadBitmapCalls;
    g_lastBitmapInstance = instance;
    g_lastBitmapName = bitmapName;
    return g_fakeBitmapHandle;
}

HDC WINAPI FakeCreateCompatibleDC(HDC hdc) {
    ++g_paintCreateCompatibleDcCalls;
    g_paintLastDestDc = hdc;
    return g_paintCompatibleDc;
}

HGDIOBJ WINAPI FakeSelectObject(HDC hdc, HGDIOBJ object) {
    if (hdc == g_paintCompatibleDc) {
        ++g_paintSelectObjectCalls;
    }
    g_paintLastSelectedObject = object;
    return object;
}

BOOL WINAPI FakeBitBlt(HDC destDc, int x, int y, int width, int height, HDC sourceDc, int sourceX,
                       int sourceY, DWORD rasterOp) {
    ++g_paintBitBltCalls;
    g_paintLastDestDc = destDc;
    g_paintLastSourceDc = sourceDc;
    g_paintLastDestRect = {x, y, x + width, y + height};
    g_paintLastSourceRect = {sourceX, sourceY, sourceX + width, sourceY + height};
    g_paintLastRasterOp = rasterOp;
    return TRUE;
}

BOOL WINAPI FakeStretchBlt(HDC destDc, int x, int y, int width, int height, HDC sourceDc,
                           int sourceX, int sourceY, int sourceWidth, int sourceHeight,
                           DWORD rasterOp) {
    ++g_paintStretchBltCalls;
    g_paintLastDestDc = destDc;
    g_paintLastSourceDc = sourceDc;
    g_paintLastDestRect = {x, y, x + width, y + height};
    g_paintLastSourceRect = {sourceX, sourceY, sourceX + sourceWidth, sourceY + sourceHeight};
    g_paintLastRasterOp = rasterOp;
    return TRUE;
}

BOOL WINAPI FakeDeleteDC(HDC hdc) {
    if (hdc == g_paintCompatibleDc) {
        ++g_paintDeleteDcCalls;
    }
    return TRUE;
}

void __fastcall FakeCFrameWndOnSize(CFrameWnd *, void *, unsigned int nType, int cx, int cy) {
    ++g_cFrameWndOnSizeCalls;
    g_cFrameWndOnSizeArgsOk = nType == 0 || nType == 1 || nType == 4;
    g_cFrameWndOnSizeArgsOk = g_cFrameWndOnSizeArgsOk && cx == 640 && cy == 480;
}

BOOL __fastcall FakeCWndCreateEx(CWnd *self, void *, DWORD exStyle, LPCSTR className,
                                      LPCSTR windowName, DWORD style, int x, int y, int width,
                                      int height, HWND parent, HMENU menu, LPVOID param) {
    ++g_cWndCreateExCalls;
    HWND hwnd = CreateWindowExA(exStyle, className, windowName, style, x, y, width, height, parent,
                                menu, GetModuleHandleA(nullptr), param);
    self->m_hWnd = hwnd;
    return hwnd != nullptr ? TRUE : FALSE;
}

void __fastcall FakeCWndSetWindowTextA(CWnd *self, void *, LPCSTR text) {
    ++g_cWndSetWindowTextCalls;
    if (self->m_hWnd != nullptr) {
        SetWindowTextA(self->m_hWnd, text);
    }
}

void __fastcall FakeCWndCenterWindow(CWnd *, void *, CWnd *) {
    ++g_cWndCenterWindowCalls;
}

char *__fastcall FakeWolMenuGetMessageString(unsigned int messageId) {
    static char caption[] = "Network Caption";
    static char messageFormat[] = "Need Winsock";

    if (g_wolMenuZlocCalls < 2) {
        g_wolMenuZlocIds[g_wolMenuZlocCalls] = static_cast<std::int32_t>(messageId);
    }
    ++g_wolMenuZlocCalls;
    return messageId == 18 ? caption : messageFormat;
}

int __fastcall FakeWolMenuVerifyWinsock(const char *caption, const char *messageFormat) {
    ++g_wolMenuVerifyCalls;
    g_wolMenuVerifyArgsOk =
        std::strcmp(caption, "Network Caption") == 0 &&
        std::strcmp(messageFormat, "Need Winsock") == 0;
    return g_wolMenuVerifyResult;
}

int __fastcall FakeWolMenuShowModal(int *selectedMissionIndexOut) {
    ++g_wolMenuModalCalls;
    if (g_wolMenuModalResult != 0) {
        *selectedMissionIndexOut = g_wolMenuModalSelectedMissionIndex;
    }
    return g_wolMenuModalResult;
}

int __fastcall FakeWolMenuLoadZbdAndSetupSensorTracker(RecoilApp *self, void *,
                                                            int missionId,
                                                            const char *zbdPath,
                                                            int skipIntroFmvMode,
                                                            int missionFlags) {
    ++g_wolMenuLoadCalls;
    g_wolMenuLoadApp = self;
    g_wolMenuLoadMissionId = missionId;
    g_wolMenuLoadZbdPath = zbdPath;
    g_wolMenuLoadSkipIntro = skipIntroFmvMode;
    g_wolMenuLoadMissionFlags = missionFlags;
    return 1;
}

HRESULT __stdcall FakeMpMenuCoInitialize(LPVOID) {
    ++g_mpMenuCoInitializeCalls;
    return g_mpMenuCoInitializeResult;
}

int __fastcall FakeMpMenuInitSessionRuntime(GUID *appGuid) {
    ++g_mpMenuInitRuntimeCalls;
    g_mpMenuInitRuntimeGuid = appGuid;
    return 0;
}

int FakeMpMenuShutdownSessionRuntime() {
    ++g_mpMenuShutdownRuntimeCalls;
    return 0;
}

int __fastcall FakeMpMenuDoModal(CDialog *self, void *) {
    ++g_mpMenuDoModalCalls;
    NetSessionBrowserDialog *const dialog = (NetSessionBrowserDialog *)self;
    dialog->m_shouldEnterHostSetup = g_mpMenuDialogShouldEnterHostSetup;
    dialog->m_selectedSessionIndex = g_mpMenuDialogSelectedSessionIndex;
    dialog->m_playerName = g_mpMenuDialogPlayerName;
    return g_mpMenuDoModalResult;
}

void __fastcall FakeMpMenuSetPlayerName(const char *name) {
    ++g_mpMenuSetPlayerNameCalls;
    g_mpMenuLastPlayerName = name;
}

void __fastcall FakeMpMenuSetNetworkEnabled(int enabled) {
    ++g_mpMenuSetNetworkEnabledCalls;
    g_mpMenuLastNetworkEnabled = enabled;
}

int __fastcall FakeMpMenuLoadZbdAndStartEngine(RecoilApp *self, void *) {
    ++g_mpMenuLoadStartCalls;
    g_mpMenuLoadStartApp = self;
    return 1;
}

void FakeMpMenuQueueEnterWithReconfigureFlag(int flag) {
    ++g_mpMenuQueueEnterCalls;
    g_mpMenuQueueEnterFlag = flag;
}

int __fastcall
FakeMpMenuOpenSelectedSessionAndReadStatusFields(
    zNetworkSessionDescStatusFields *statusFields
) {
    ++g_mpMenuOpenSessionCalls;
    g_mpMenuOpenSelectedSessionIndex = statusFields->selectedSessionIndex;
    statusFields->eventCode = g_mpMenuOpenEventCode;
    statusFields->statusFlags = g_mpMenuOpenStatusFlags;
    statusFields->valueOrTime = g_mpMenuOpenValueOrTime;
    statusFields->auxParam = g_mpMenuOpenAuxParam;
    return g_mpMenuOpenSessionResult;
}

int __fastcall FakeMpMenuCreateLocalPlayerRecordAndRegister(char *playerName) {
    ++g_mpMenuCreateLocalPlayerCalls;
    g_mpMenuCreateLocalPlayerName = playerName;
    return 1;
}

zNetworkDispatchHandlerRecord *__fastcall FakeMpMenuRegisterPacketHandler(
    int packetType,
    zNetworkPacketHandler handlerProc,
    int mode
) {
    ++g_mpMenuRegisterPacketCalls;
    g_mpMenuRegisterPacketType = packetType;
    g_mpMenuRegisterPacketHandler = handlerProc;
    g_mpMenuRegisterPacketMode = mode;
    return 0;
}

void __fastcall FakeMpMenuSetStatusBitsFromFlags(unsigned int statusFlags) {
    ++g_mpMenuSetStatusCalls;
    g_mpMenuStatusFlags = statusFlags;
}

void __fastcall FakeMpMenuSetRuntimeTimer(
    HudSensorTracker *self,
    void *,
    int timerSecRaw,
    int goalValue
) {
    ++g_mpMenuSetRuntimeTimerCalls;
    g_mpMenuSetRuntimeTimerThis = self;
    g_mpMenuTimerRaw = timerSecRaw;
    g_mpMenuGoalValue = goalValue;
}

int __fastcall FakeMpMenuLoadZbdAndSetupSensorTracker(
    RecoilApp *self,
    void *,
    int missionId,
    const char *zbdPath,
    int skipIntroFmvMode,
    int missionFlags
) {
    ++g_mpMenuLoadSetupCalls;
    g_mpMenuLoadSetupApp = self;
    g_mpMenuLoadSetupMissionId = missionId;
    g_mpMenuLoadSetupZbdPath = zbdPath;
    g_mpMenuLoadSetupSkipIntro = skipIntroFmvMode;
    g_mpMenuLoadSetupMissionFlags = missionFlags;
    return 1;
}

void *CFrameWndOnCloseProc() {
    union MemberToFunction {
        void ( CFrameWndOnCloseAccess::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnCloseAccess::OnClose;
    return thunk.function;
}

void *CFrameWndOnActivateProc() {
    union MemberToFunction {
        void ( CFrameWndOnActivateAccess::*member)(unsigned int, CWnd *, BOOL);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnActivateAccess::OnActivate;
    return thunk.function;
}

void *CFrameWndOnCreateProc() {
    union MemberToFunction {
        int ( CFrameWndOnCreateAccess::*member)(CREATESTRUCTA *);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnCreateAccess::OnCreate;
    return thunk.function;
}

void *CFrameWndOnDestroyProc() {
    union MemberToFunction {
        void ( CFrameWndOnDestroyAccess::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnDestroyAccess::OnDestroy;
    return thunk.function;
}

void *CGdiObjectDeleteObjectProc() {
    union MemberToFunction {
        BOOL ( CGdiObject::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CGdiObject::DeleteObject;
    return thunk.function;
}

void *CFrameWndOnSizeProc() {
    union MemberToFunction {
        void ( CFrameWndOnSizeAccess::*member)(unsigned int, int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CFrameWndOnSizeAccess::OnSize;
    return thunk.function;
}

void *CWndCreateExProc() {
    union MemberToFunction {
        BOOL ( CWndCreateExAccess::*member)(DWORD, LPCSTR, LPCSTR, DWORD, int,
                                                           int, int, int, HWND, HMENU, LPVOID);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CWndCreateExAccess::CreateEx;
    return thunk.function;
}

void *CWndSetWindowTextAProc() {
    union MemberToFunction {
        void ( CWndSetWindowTextAccess::*member)(LPCSTR);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CWndSetWindowTextAccess::SetWindowText;
    return thunk.function;
}

void *CWndCenterWindowProc() {
    union MemberToFunction {
        void ( CWndCenterWindowAccess::*member)(CWnd *);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CWndCenterWindowAccess::CenterWindow;
    return thunk.function;
}

void *RecoilAppLoadZbdAndStartEngineProc() {
    union MemberToFunction {
        int ( RecoilApp::*member)();
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &RecoilApp::LoadZbdAndStartEngine;
    return thunk.function;
}

void *RecoilAppLoadZbdAndSetupSensorTrackerProc() {
    union MemberToFunction {
        int ( RecoilApp::*member)(int, const char *, int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &RecoilApp::LoadZbdAndSetupSensorTracker;
    return thunk.function;
}

void *HudSensorTrackerSetRuntimeTimerProc() {
    union MemberToFunction {
        void ( HudSensorTrackerSetRuntimeTimerAccess::*member)(int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &HudSensorTrackerSetRuntimeTimerAccess::SetRuntimeTimerSecAndGoalValue;
    return thunk.function;
}

void ResetWolMenuProbe() {
    g_wolMenuZlocCalls = 0;
    g_wolMenuZlocIds[0] = -1;
    g_wolMenuZlocIds[1] = -1;
    g_wolMenuVerifyCalls = 0;
    g_wolMenuVerifyResult = 1;
    g_wolMenuVerifyArgsOk = true;
    g_wolMenuModalCalls = 0;
    g_wolMenuModalResult = 0;
    g_wolMenuModalSelectedMissionIndex = -1;
    g_wolMenuLoadCalls = 0;
    g_wolMenuLoadApp = nullptr;
    g_wolMenuLoadMissionId = -1;
    g_wolMenuLoadZbdPath = reinterpret_cast<const char *>(static_cast<std::uintptr_t>(1));
    g_wolMenuLoadSkipIntro = -1;
    g_wolMenuLoadMissionFlags = -1;
}

void ResetMpMenuProbe() {
    g_mpMenuCoInitializeCalls = 0;
    g_mpMenuCoInitializeResult = S_OK;
    g_mpMenuInitRuntimeCalls = 0;
    g_mpMenuInitRuntimeGuid = nullptr;
    g_mpMenuShutdownRuntimeCalls = 0;
    g_mpMenuDoModalCalls = 0;
    g_mpMenuDoModalResult = IDOK;
    g_mpMenuDialogShouldEnterHostSetup = 0;
    g_mpMenuDialogSelectedSessionIndex = 3;
    g_mpMenuDialogPlayerName = "PlayerOne";
    g_mpMenuSetPlayerNameCalls = 0;
    g_mpMenuLastPlayerName = nullptr;
    g_mpMenuSetNetworkEnabledCalls = 0;
    g_mpMenuLastNetworkEnabled = -1;
    g_mpMenuLoadStartCalls = 0;
    g_mpMenuLoadStartApp = nullptr;
    g_mpMenuQueueEnterCalls = 0;
    g_mpMenuQueueEnterFlag = -1;
    g_mpMenuOpenSessionCalls = 0;
    g_mpMenuOpenSessionResult = 1;
    g_mpMenuOpenSelectedSessionIndex = -1;
    g_mpMenuOpenEventCode = 301;
    g_mpMenuOpenStatusFlags = 0x35;
    g_mpMenuOpenValueOrTime = 2;
    g_mpMenuOpenAuxParam = 9;
    g_mpMenuCreateLocalPlayerCalls = 0;
    g_mpMenuCreateLocalPlayerName = nullptr;
    g_mpMenuRegisterPacketCalls = 0;
    g_mpMenuRegisterPacketType = -1;
    g_mpMenuRegisterPacketMode = -1;
    g_mpMenuRegisterPacketHandler = nullptr;
    g_mpMenuSetStatusCalls = 0;
    g_mpMenuStatusFlags = 0;
    g_mpMenuSetRuntimeTimerCalls = 0;
    g_mpMenuSetRuntimeTimerThis = nullptr;
    g_mpMenuTimerRaw = 0;
    g_mpMenuGoalValue = 0;
    g_mpMenuLoadSetupCalls = 0;
    g_mpMenuLoadSetupApp = nullptr;
    g_mpMenuLoadSetupMissionId = -1;
    g_mpMenuLoadSetupZbdPath = reinterpret_cast<const char *>(static_cast<std::uintptr_t>(1));
    g_mpMenuLoadSetupSkipIntro = -1;
    g_mpMenuLoadSetupMissionFlags = -1;
    g_RecoilApp.m_pendingState = 0;
    g_RecoilApp.m_skipIntroFmv = 0;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 0;
    g_zNetwork_FatalDisconnectCallback = nullptr;
}

bool PatchImportByName(const char *dllName, const char *functionName, void *replacement,
                       ImportFunctionPatch &patch) {
    HMODULE module = GetModuleHandleA(nullptr);
    auto *base = reinterpret_cast<unsigned char *>(module);
    auto *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
    const IMAGE_DATA_DIRECTORY &directory =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto *descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base + directory.VirtualAddress);

    for (; descriptor->Name != 0; ++descriptor) {
        const char *importDll = reinterpret_cast<const char *>(base + descriptor->Name);
        if (_stricmp(importDll, dllName) != 0) {
            continue;
        }

        auto *names = reinterpret_cast<IMAGE_THUNK_DATA *>(base + descriptor->OriginalFirstThunk);
        auto *thunks = reinterpret_cast<IMAGE_THUNK_DATA *>(base + descriptor->FirstThunk);
        for (; names->u1.AddressOfData != 0; ++names, ++thunks) {
            if ((names->u1.Ordinal & IMAGE_ORDINAL_FLAG) != 0) {
                continue;
            }

            auto *importName = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(
                base + names->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char *>(importName->Name), functionName) != 0) {
                continue;
            }

            patch.slot = reinterpret_cast<ULONG_PTR *>(&thunks->u1.Function);
            patch.original = *patch.slot;
            DWORD oldProtect = 0;
            if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                               &oldProtect) == 0) {
                patch.slot = nullptr;
                return false;
            }

            *patch.slot = reinterpret_cast<ULONG_PTR>(replacement);
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

bool PatchImportByOrdinal(
    const char *dllName,
    WORD ordinal,
    void *replacement,
    ImportFunctionPatch &patch
) {
    unsigned char *const imageBase = reinterpret_cast<unsigned char *>(GetModuleHandleA(nullptr));
    auto *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(imageBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &directory =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto *descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(
        imageBase + directory.VirtualAddress);

    for (; descriptor->Name != 0; ++descriptor) {
        const char *importDll = reinterpret_cast<const char *>(imageBase + descriptor->Name);
        if (_stricmp(importDll, dllName) != 0) {
            continue;
        }

        auto *names = reinterpret_cast<IMAGE_THUNK_DATA *>(
            imageBase + (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                             : descriptor->FirstThunk));
        auto *thunks = reinterpret_cast<IMAGE_THUNK_DATA *>(imageBase + descriptor->FirstThunk);
        for (; names->u1.AddressOfData != 0; ++names, ++thunks) {
            if (!IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal) ||
                static_cast<WORD>(names->u1.Ordinal & 0xffff) != ordinal) {
                continue;
            }

            patch.slot = reinterpret_cast<ULONG_PTR *>(&thunks->u1.Function);
            patch.original = *patch.slot;
            DWORD oldProtect = 0;
            if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                               &oldProtect) == 0) {
                patch.slot = nullptr;
                return false;
            }

            *patch.slot = reinterpret_cast<ULONG_PTR>(replacement);
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
        return false;
    }

    patch.address[0] = 0xe9;
    const auto delta = static_cast<std::intptr_t>(reinterpret_cast<unsigned char *>(replacement) -
                                                 (patch.address + sizeof(patch.original)));
    const auto relative = static_cast<std::int32_t>(delta);
    std::memcpy(patch.address + 1, &relative, sizeof(relative));
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    return true;
}

void RestoreImportPatch(ImportFunctionPatch &patch) {
    if (patch.slot == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE, &oldProtect) !=
        0) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }

    patch.slot = nullptr;
    patch.original = 0;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    }
    patch.address = nullptr;
}

} // namespace

extern "C" int czrecoil_frame_open_multiplayer_session_browser_smoke(void) {
    ImportFunctionPatch coInitializePatch{};
    ImportFunctionPatch doModalPatch{};
    CodeFunctionPatch initRuntimePatch{};
    CodeFunctionPatch shutdownRuntimePatch{};
    CodeFunctionPatch setPlayerNamePatch{};
    CodeFunctionPatch setNetworkEnabledPatch{};
    CodeFunctionPatch loadStartPatch{};
    CodeFunctionPatch queueEnterPatch{};
    CodeFunctionPatch openSessionPatch{};
    CodeFunctionPatch createPlayerPatch{};
    CodeFunctionPatch registerPacketPatch{};
    CodeFunctionPatch setStatusPatch{};
    CodeFunctionPatch setRuntimeTimerPatch{};
    CodeFunctionPatch loadSetupPatch{};
    const WORD kMfc42CDialogDoModalOrdinal = 2514;

    if (!PatchImportByName("ole32.dll", "CoInitialize",
                           reinterpret_cast<void *>(&FakeMpMenuCoInitialize),
                           coInitializePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zNetwork::InitSessionRuntime),
                           reinterpret_cast<void *>(&FakeMpMenuInitSessionRuntime),
                           initRuntimePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zNetwork::ShutdownSessionRuntime),
                           reinterpret_cast<void *>(&FakeMpMenuShutdownSessionRuntime),
                           shutdownRuntimePatch) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDoModalOrdinal,
                              reinterpret_cast<void *>(&FakeMpMenuDoModal),
                              doModalPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zOpt::SetPlayerName),
                           reinterpret_cast<void *>(&FakeMpMenuSetPlayerName),
                           setPlayerNamePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zOpt::SetNetworkEnabled),
                           reinterpret_cast<void *>(&FakeMpMenuSetNetworkEnabled),
                           setNetworkEnabledPatch) ||
        !PatchFunctionJump(RecoilAppLoadZbdAndStartEngineProc(),
                           reinterpret_cast<void *>(&FakeMpMenuLoadZbdAndStartEngine),
                           loadStartPatch) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&HudUiNetGameSetupOverlayOwner::
                                         QueueEnterWithReconfigureFlag),
            reinterpret_cast<void *>(&FakeMpMenuQueueEnterWithReconfigureFlag),
            queueEnterPatch) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zNetworkDPlay::OpenSelectedSessionAndReadStatusFields),
            reinterpret_cast<void *>(&FakeMpMenuOpenSelectedSessionAndReadStatusFields),
            openSessionPatch) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&zNetwork_DPlay::CreateLocalPlayerRecordAndRegister),
            reinterpret_cast<void *>(&FakeMpMenuCreateLocalPlayerRecordAndRegister),
            createPlayerPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zNetwork::RegisterPacketHandler),
                           reinterpret_cast<void *>(&FakeMpMenuRegisterPacketHandler),
                           registerPacketPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&GameNet::SetStatusBitsFromFlags),
                           reinterpret_cast<void *>(&FakeMpMenuSetStatusBitsFromFlags),
                           setStatusPatch) ||
        !PatchFunctionJump(HudSensorTrackerSetRuntimeTimerProc(),
                           reinterpret_cast<void *>(&FakeMpMenuSetRuntimeTimer),
                           setRuntimeTimerPatch) ||
        !PatchFunctionJump(RecoilAppLoadZbdAndSetupSensorTrackerProc(),
                           reinterpret_cast<void *>(&FakeMpMenuLoadZbdAndSetupSensorTracker),
                           loadSetupPatch)) {
        RestoreFunctionPatch(loadSetupPatch);
        RestoreFunctionPatch(setRuntimeTimerPatch);
        RestoreFunctionPatch(setStatusPatch);
        RestoreFunctionPatch(registerPacketPatch);
        RestoreFunctionPatch(createPlayerPatch);
        RestoreFunctionPatch(openSessionPatch);
        RestoreFunctionPatch(queueEnterPatch);
        RestoreFunctionPatch(loadStartPatch);
        RestoreFunctionPatch(setNetworkEnabledPatch);
        RestoreFunctionPatch(setPlayerNamePatch);
        RestoreImportPatch(doModalPatch);
        RestoreFunctionPatch(shutdownRuntimePatch);
        RestoreFunctionPatch(initRuntimePatch);
        RestoreImportPatch(coInitializePatch);
        return 1;
    }

    int result = 0;
    CZRecoilFrame frame{};
    frame.m_useArchiveBanks = 0x55;

    ResetMpMenuProbe();
    g_mpMenuDoModalResult = IDCANCEL;
    frame.OnMenuOpenMultiplayerSessionBrowser();
    if (g_mpMenuCoInitializeCalls != 1) {
        result = 20;
    } else if (g_mpMenuInitRuntimeCalls != 1) {
        result = 21;
    } else if (g_mpMenuDoModalCalls != 1) {
        result = 22;
    } else if (g_mpMenuShutdownRuntimeCalls != 1) {
        result = 23;
    } else if (g_mpMenuSetNetworkEnabledCalls != 1) {
        result = 24;
    } else if (g_mpMenuLastNetworkEnabled != 0) {
        result = 25;
    } else if (g_mpMenuLoadStartCalls != 0 || g_mpMenuLoadSetupCalls != 0) {
        result = 26;
    } else if (g_zNetwork_FatalDisconnectCallback != &RecoilApp::FatalErrorAndExit) {
        result = 27;
    } else if (g_RecoilApp.m_skipIntroFmv != 1 ||
               g_RecoilApp.m_missionFmvState.m_skipMissionFmv != 1) {
        result = 28;
    }

    ResetMpMenuProbe();
    g_mpMenuDialogShouldEnterHostSetup = 1;
    g_mpMenuDialogPlayerName = "HostPilot";
    frame.OnMenuOpenMultiplayerSessionBrowser();
    if (result == 0 &&
        (g_mpMenuInitRuntimeGuid != &g_zNetwork_RecoilAppGuid ||
         g_mpMenuSetPlayerNameCalls != 1 ||
         std::strcmp(g_mpMenuLastPlayerName, "HostPilot") != 0 ||
         g_mpMenuSetNetworkEnabledCalls != 1 || g_mpMenuLastNetworkEnabled != 1 ||
         g_mpMenuLoadStartCalls != 1 || g_mpMenuLoadStartApp != &g_RecoilApp ||
         g_mpMenuQueueEnterCalls != 1 || g_mpMenuQueueEnterFlag != 0 ||
         g_mpMenuOpenSessionCalls != 0 || g_mpMenuShutdownRuntimeCalls != 0)) {
        result = 3;
    }

    ResetMpMenuProbe();
    g_mpMenuDialogShouldEnterHostSetup = 0;
    g_mpMenuDialogSelectedSessionIndex = 4;
    g_mpMenuDialogPlayerName = "JoinPilot";
    g_mpMenuOpenEventCode = 301;
    g_mpMenuOpenStatusFlags = 0x35;
    g_mpMenuOpenValueOrTime = 2;
    g_mpMenuOpenAuxParam = 9;
    frame.OnMenuOpenMultiplayerSessionBrowser();

    union TimerSecondsBits {
        float seconds;
        std::int32_t raw;
    } expectedTimer = {120.0f};
    const RecoilPtr32 expectedPendingState =
        static_cast<RecoilPtr32>(
            reinterpret_cast<std::uintptr_t>(&g_RecoilApp.m_mpExitDialogState.base)
        );
    if (result == 0 &&
        (g_mpMenuOpenSessionCalls != 1 || g_mpMenuOpenSelectedSessionIndex != 4 ||
         g_mpMenuSetNetworkEnabledCalls != 1 || g_mpMenuLastNetworkEnabled != 1 ||
         g_mpMenuCreateLocalPlayerCalls != 1 ||
         std::strcmp(g_mpMenuCreateLocalPlayerName, "JoinPilot") != 0 ||
         g_mpMenuSetPlayerNameCalls != 2 ||
         std::strcmp(g_mpMenuLastPlayerName, "JoinPilot") != 0 ||
         g_RecoilApp.m_pendingState != expectedPendingState ||
         g_mpMenuRegisterPacketCalls != 1 || g_mpMenuRegisterPacketType != 20 ||
         g_mpMenuRegisterPacketMode != 2 ||
         g_mpMenuRegisterPacketHandler !=
             (zNetworkPacketHandler)&GameNet::HandlePkt14_HudTimerAndFlagsSync ||
         g_mpMenuSetStatusCalls != 1 || g_mpMenuStatusFlags != 0x35 ||
         g_mpMenuSetRuntimeTimerCalls != 1 ||
         g_mpMenuSetRuntimeTimerThis != &g_HudSensorTracker ||
         g_mpMenuTimerRaw != expectedTimer.raw || g_mpMenuGoalValue != 9 ||
         g_mpMenuLoadSetupCalls != 1 || g_mpMenuLoadSetupApp != &g_RecoilApp ||
         g_mpMenuLoadSetupMissionId != 7 || g_mpMenuLoadSetupZbdPath != nullptr ||
         g_mpMenuLoadSetupSkipIntro != 1 || g_mpMenuLoadSetupMissionFlags != 0x55 ||
         g_mpMenuShutdownRuntimeCalls != 0)) {
        result = 4;
    }

    RestoreFunctionPatch(loadSetupPatch);
    RestoreFunctionPatch(setRuntimeTimerPatch);
    RestoreFunctionPatch(setStatusPatch);
    RestoreFunctionPatch(registerPacketPatch);
    RestoreFunctionPatch(createPlayerPatch);
    RestoreFunctionPatch(openSessionPatch);
    RestoreFunctionPatch(queueEnterPatch);
    RestoreFunctionPatch(loadStartPatch);
    RestoreFunctionPatch(setNetworkEnabledPatch);
    RestoreFunctionPatch(setPlayerNamePatch);
    RestoreImportPatch(doModalPatch);
    RestoreFunctionPatch(shutdownRuntimePatch);
    RestoreFunctionPatch(initRuntimePatch);
    RestoreImportPatch(coInitializePatch);
    ResetMpMenuProbe();
    return result;
}

extern "C" int czrecoil_frame_on_menu_westwood_online_upgrade_smoke(void) {
    CodeFunctionPatch zlocPatch{};
    CodeFunctionPatch verifyPatch{};
    CodeFunctionPatch modalPatch{};
    CodeFunctionPatch loadPatch{};
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zLoc::GetMessageString),
                           reinterpret_cast<void *>(&FakeWolMenuGetMessageString), zlocPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&NetUi::VerifyWinsock2OrPromptContinue),
                           reinterpret_cast<void *>(&FakeWolMenuVerifyWinsock), verifyPatch) ||
        !PatchFunctionJump(
            reinterpret_cast<void *>(&WestwoodOnlineUpgradeDialog::
                                         ShowModalAndGetSelectedMissionIndex),
            reinterpret_cast<void *>(&FakeWolMenuShowModal), modalPatch) ||
        !PatchFunctionJump(RecoilAppLoadZbdAndSetupSensorTrackerProc(),
                           reinterpret_cast<void *>(&FakeWolMenuLoadZbdAndSetupSensorTracker),
                           loadPatch)) {
        RestoreFunctionPatch(loadPatch);
        RestoreFunctionPatch(modalPatch);
        RestoreFunctionPatch(verifyPatch);
        RestoreFunctionPatch(zlocPatch);
        return 1;
    }

    const int oldChecked = g_CZRecoilFrame_WestwoodOnlineWinsockChecked;
    const int oldSkipIntro = g_RecoilApp.m_skipIntroFmv;
    const int oldSkipMissionFmv = g_RecoilApp.m_missionFmvState.m_skipMissionFmv;
    const int oldMissionFlags = g_HudSensorTracker.missionFlags;

    CZRecoilFrame frame{};
    int failure = 0;

    ResetWolMenuProbe();
    g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 0;
    g_RecoilApp.m_skipIntroFmv = 7;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 8;
    g_wolMenuVerifyResult = 0;
    frame.OnMenuWestwoodOnlineUpgrade();
    if (g_CZRecoilFrame_WestwoodOnlineWinsockChecked != 1 || g_wolMenuZlocCalls != 2 ||
        g_wolMenuZlocIds[0] != 18 || g_wolMenuZlocIds[1] != 38 ||
        g_wolMenuVerifyCalls != 1 || !g_wolMenuVerifyArgsOk ||
        g_wolMenuModalCalls != 0 || g_wolMenuLoadCalls != 0 ||
        g_RecoilApp.m_skipIntroFmv != 7 ||
        g_RecoilApp.m_missionFmvState.m_skipMissionFmv != 8) {
        failure = 2;
    }

    ResetWolMenuProbe();
    g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 1;
    g_RecoilApp.m_skipIntroFmv = 0;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 0;
    frame.OnMenuWestwoodOnlineUpgrade();
    if (failure == 0 &&
        (g_wolMenuZlocCalls != 0 || g_wolMenuVerifyCalls != 0 ||
         g_wolMenuModalCalls != 1 || g_wolMenuLoadCalls != 0 ||
         g_RecoilApp.m_skipIntroFmv != 1 ||
         g_RecoilApp.m_missionFmvState.m_skipMissionFmv != 1)) {
        failure = 3;
    }

    ResetWolMenuProbe();
    g_CZRecoilFrame_WestwoodOnlineWinsockChecked = 0;
    g_RecoilApp.m_skipIntroFmv = 0;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 0;
    g_HudSensorTracker.missionFlags = 0x55;
    g_wolMenuVerifyResult = 1;
    g_wolMenuModalResult = 1;
    g_wolMenuModalSelectedMissionIndex = 4;
    frame.OnMenuWestwoodOnlineUpgrade();
    if (failure == 0 &&
        (g_CZRecoilFrame_WestwoodOnlineWinsockChecked != 1 || g_wolMenuZlocCalls != 2 ||
         g_wolMenuVerifyCalls != 1 || !g_wolMenuVerifyArgsOk ||
         g_wolMenuModalCalls != 1 || g_wolMenuLoadCalls != 1 ||
         g_wolMenuLoadApp != &g_RecoilApp || g_wolMenuLoadMissionId != 10 ||
         g_wolMenuLoadZbdPath != nullptr || g_wolMenuLoadSkipIntro != 1 ||
         g_wolMenuLoadMissionFlags != 0x55 || g_RecoilApp.m_skipIntroFmv != 1 ||
         g_RecoilApp.m_missionFmvState.m_skipMissionFmv != 1)) {
        failure = 4;
    }

    g_CZRecoilFrame_WestwoodOnlineWinsockChecked = oldChecked;
    g_RecoilApp.m_skipIntroFmv = oldSkipIntro;
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = oldSkipMissionFmv;
    g_HudSensorTracker.missionFlags = oldMissionFlags;

    RestoreFunctionPatch(loadPatch);
    RestoreFunctionPatch(modalPatch);
    RestoreFunctionPatch(verifyPatch);
    RestoreFunctionPatch(zlocPatch);
    return failure;
}

extern "C" int czgame_frame_on_app_idle_dispatch_message_smoke(void) {
    union MemberToFunction {
        std::int32_t ( FakeGameFrameApp::*member)(std::uint32_t, std::uint32_t);
        std::int32_t( *function)(CZGameFrameApp *, std::uint32_t, std::uint32_t);
    };

    MemberToFunction thunk{};
    thunk.member = &FakeGameFrameApp::OnIdleOrDispatch;

    CZGameFrameAppVtable vtable{};
    vtable.OnIdleOrDispatch = thunk.function;
    FakeGameFrameApp app{{&vtable}};
    CZGameFrame frame{};
    frame.m_app = &app;

    const std::int32_t result = frame.OnAppIdleDispatchMessage(0xabcdef01, 0x23456789);

    return result == 0x1234 && g_lastIdleWParam == 0xabcdef01 && g_lastIdleLParam == 0x23456789 ? 0
                                                                                                : 1;
}

extern "C" int czgame_frame_on_close_smoke(void) {
    CodeFunctionPatch frameWndOnClosePatch{};
    if (!PatchFunctionJump(CFrameWndOnCloseProc(), reinterpret_cast<void *>(&FakeCFrameWndOnClose),
                           frameWndOnClosePatch)) {
        return 1;
    }

    CZGameFrame frame{};
    g_cFrameWndOnCloseCalls = 0;
    frame.OnClose();
    const bool ok = g_cFrameWndOnCloseCalls == 1;

    RestoreFunctionPatch(frameWndOnClosePatch);
    return ok ? 0 : 2;
}

extern "C" int czgame_frame_on_create_smoke(void) {
    CodeFunctionPatch frameWndOnCreatePatch{};
    CodeFunctionPatch findResourcePatch{};
    ImportFunctionPatch loadBitmapPatch{};
    if (!PatchFunctionJump(CFrameWndOnCreateProc(),
                           reinterpret_cast<void *>(&FakeCFrameWndOnCreate),
                           frameWndOnCreatePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&AfxFindResourceHandle),
                           reinterpret_cast<void *>(&FakeAfxFindResourceHandle),
                           findResourcePatch) ||
        !PatchImportByName("USER32.dll", "LoadBitmapA", reinterpret_cast<void *>(&FakeLoadBitmapA),
                           loadBitmapPatch)) {
        RestoreImportPatch(loadBitmapPatch);
        RestoreFunctionPatch(findResourcePatch);
        RestoreFunctionPatch(frameWndOnCreatePatch);
        return 1;
    }

    CZGameFrame frame{};
    CREATESTRUCTA createStruct{};
    g_cFrameWndOnCreateCalls = 0;
    g_findResourceHandleCalls = 0;
    g_loadBitmapCalls = 0;
    g_zInput_MouseDevice = nullptr;
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseActive = 1;
    g_cFrameWndOnCreateResult = -1;

    if (frame.OnCreate(&createStruct) != -1 || g_cFrameWndOnCreateCalls != 1 ||
        g_findResourceHandleCalls != 0 || g_loadBitmapCalls != 0 ||
        g_zInput_MouseInitialized != 1 || g_zInput_MouseActive != 1) {
        RestoreImportPatch(loadBitmapPatch);
        RestoreFunctionPatch(findResourcePatch);
        RestoreFunctionPatch(frameWndOnCreatePatch);
        return 2;
    }

    g_cFrameWndOnCreateResult = 0;
    g_cFrameWndOnCreateCalls = 0;
    g_findResourceHandleCalls = 0;
    g_loadBitmapCalls = 0;
    g_lastResourceName = nullptr;
    g_lastResourceType = nullptr;
    g_lastBitmapName = nullptr;
    g_lastBitmapInstance = nullptr;
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseActive = 1;
    frame.m_gameBitmap.m_hObject = nullptr;

    const int result = frame.OnCreate(&createStruct);
    const bool ok = result == 0 && g_cFrameWndOnCreateCalls == 1 &&
                    g_findResourceHandleCalls == 1 && g_loadBitmapCalls == 1 &&
                    std::strcmp(g_lastResourceName, "GAMEBMP") == 0 &&
                    g_lastResourceType == MAKEINTRESOURCEA(2) &&
                    g_lastBitmapInstance == g_fakeResourceHandle &&
                    std::strcmp(g_lastBitmapName, "GAMEBMP") == 0 &&
                    frame.m_gameBitmap.m_hObject == g_fakeBitmapHandle &&
                    g_zInput_MouseInitialized == 0 && g_zInput_MouseActive == 0;

    frame.m_gameBitmap.m_hObject = nullptr;
    RestoreImportPatch(loadBitmapPatch);
    RestoreFunctionPatch(findResourcePatch);
    RestoreFunctionPatch(frameWndOnCreatePatch);
    return ok ? 0 : 3;
}

extern "C" int czgame_frame_on_destroy_smoke(void) {
    CodeFunctionPatch networkPatch{};
    CodeFunctionPatch videoPatch{};
    CodeFunctionPatch cdStopPatch{};
    CodeFunctionPatch frameDestroyPatch{};
    CodeFunctionPatch bitmapDeletePatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zNetwork_DPlay_DestroyCachedLocalPlayer),
                           reinterpret_cast<void *>(&FakeDestroyCachedLocalPlayer),
                           networkPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zVideo::ShutdownVideoSystem),
                           reinterpret_cast<void *>(&FakeShutdownVideoSystem), videoPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zSndCd::Stop),
                           reinterpret_cast<void *>(&FakeZsndCdStop), cdStopPatch) ||
        !PatchFunctionJump(CFrameWndOnDestroyProc(),
                           reinterpret_cast<void *>(&FakeCFrameWndOnDestroy),
                           frameDestroyPatch) ||
        !PatchFunctionJump(CGdiObjectDeleteObjectProc(),
                           reinterpret_cast<void *>(&FakeCGdiObjectDeleteObject),
                           bitmapDeletePatch)) {
        RestoreFunctionPatch(bitmapDeletePatch);
        RestoreFunctionPatch(frameDestroyPatch);
        RestoreFunctionPatch(cdStopPatch);
        RestoreFunctionPatch(videoPatch);
        RestoreFunctionPatch(networkPatch);
        return 1;
    }

    CZGameFrame frame{};
    g_cFrameWndOnDestroyCalls = 0;
    g_gdiDeleteObjectCalls = 0;
    g_onDestroyCallCount = 0;
    for (int i = 0; i < 5; ++i) {
        g_onDestroyCallOrder[i] = 0;
    }

    frame.OnDestroy();
    const bool ok = g_onDestroyCallCount == 5 && g_onDestroyCallOrder[0] == 1 &&
                    g_onDestroyCallOrder[1] == 2 && g_onDestroyCallOrder[2] == 3 &&
                    g_onDestroyCallOrder[3] == 4 && g_onDestroyCallOrder[4] == 5 &&
                    g_cFrameWndOnDestroyCalls == 1 && g_gdiDeleteObjectCalls == 1;

    RestoreFunctionPatch(bitmapDeletePatch);
    RestoreFunctionPatch(frameDestroyPatch);
    RestoreFunctionPatch(cdStopPatch);
    RestoreFunctionPatch(videoPatch);
    RestoreFunctionPatch(networkPatch);
    return ok ? 0 : 2;
}

extern "C" int czgame_frame_on_activate_smoke(void) {
    union ActivateMemberToFunction {
        void ( FakeGameFrameApp::*member)();
        void( *function)(CZGameFrameApp *);
    };

    union StateWndActivateMemberToFn {
        void ( FakeActivateState::*member)(std::uint32_t);
        RecoilFn32 fn;
    };

    ActivateMemberToFunction activateThunk{};
    activateThunk.member = &FakeGameFrameApp::OnActivate;
    ActivateMemberToFunction deactivateThunk{};
    deactivateThunk.member = &FakeGameFrameApp::OnDeactivate;
    StateWndActivateMemberToFn stateThunk{};
    stateThunk.member = &FakeActivateState::OnWndActivate;

    CZGameFrameAppVtable appVtable{};
    appVtable.OnAppActivate = activateThunk.function;
    appVtable.OnAppDeactivate = deactivateThunk.function;

    RecoilApp_IState_Vtbl stateVtable{};
    stateVtable.OnWndActivate = stateThunk.fn;
    FakeActivateState state{};
    state.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&stateVtable));

    RecoilApp app{};
    app.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&appVtable));
    app.m_currentStateIndex = 0;
    app.m_stateStack[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&state));

    CZGameFrame frame{};
    frame.m_app = reinterpret_cast<CZGameFrameApp *>(&app);

    CodeFunctionPatch frameWndOnActivatePatch{};
    CodeFunctionPatch zInputActivatePatch{};
    CodeFunctionPatch zInputDeactivatePatch{};
    CodeFunctionPatch zGameReturnPatch{};
    CodeFunctionPatch zVideoRestorePatch{};

    if (!PatchFunctionJump(CFrameWndOnActivateProc(),
                           reinterpret_cast<void *>(&FakeCFrameWndOnActivate),
                           frameWndOnActivatePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zInput::OnAppActivate),
                           reinterpret_cast<void *>(&FakeZInputOnAppActivate),
                           zInputActivatePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zInput::OnAppDeactivate),
                           reinterpret_cast<void *>(&FakeZInputOnAppDeactivate),
                           zInputDeactivatePatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zGame::ReturnOnlyStub),
                           reinterpret_cast<void *>(&FakeZGameReturnOnlyStub),
                           zGameReturnPatch) ||
        !PatchFunctionJump(reinterpret_cast<void *>(&zVideo_RestoreIconicFullscreenWindowIfNeeded),
                           reinterpret_cast<void *>(
                               &FakeZVideoRestoreIconicFullscreenWindowIfNeeded),
                           zVideoRestorePatch)) {
        RestoreFunctionPatch(zVideoRestorePatch);
        RestoreFunctionPatch(zGameReturnPatch);
        RestoreFunctionPatch(zInputDeactivatePatch);
        RestoreFunctionPatch(zInputActivatePatch);
        RestoreFunctionPatch(frameWndOnActivatePatch);
        return 1;
    }

    g_appActivateCalls = 0;
    g_appDeactivateCalls = 0;
    g_cFrameWndOnActivateCalls = 0;
    g_cFrameWndOnActivateArgsOk = false;
    g_stateWndActivateCalls = 0;
    g_lastStateWndActivateValue = -1;
    g_zInputOnAppActivateCalls = 0;
    g_zInputOnAppDeactivateCalls = 0;
    g_zGameReturnOnlyStubCalls = 0;
    g_zVideoRestoreIconicCalls = 0;
    g_onActivateCallCount = 0;
    for (int i = 0; i < 5; ++i) {
        g_onActivateCallOrder[i] = 0;
    }

    frame.OnActivate(0, nullptr, FALSE);
    bool ok = g_onActivateCallCount == 5 && g_onActivateCallOrder[0] == 1 &&
              g_onActivateCallOrder[1] == 2 && g_onActivateCallOrder[2] == 3 &&
              g_onActivateCallOrder[3] == 4 && g_onActivateCallOrder[4] == 5 &&
              g_cFrameWndOnActivateCalls == 1 && g_cFrameWndOnActivateArgsOk &&
              g_stateWndActivateCalls == 1 && g_lastStateWndActivateValue == 0 &&
              g_appDeactivateCalls == 1 && g_zInputOnAppDeactivateCalls == 1 &&
              g_zGameReturnOnlyStubCalls == 1 && g_appActivateCalls == 0 &&
              g_zInputOnAppActivateCalls == 0 && g_zVideoRestoreIconicCalls == 0;
    if (!ok) {
        RestoreFunctionPatch(zVideoRestorePatch);
        RestoreFunctionPatch(zGameReturnPatch);
        RestoreFunctionPatch(zInputDeactivatePatch);
        RestoreFunctionPatch(zInputActivatePatch);
        RestoreFunctionPatch(frameWndOnActivatePatch);
        return 2;
    }

    g_appActivateCalls = 0;
    g_appDeactivateCalls = 0;
    g_cFrameWndOnActivateCalls = 0;
    g_cFrameWndOnActivateArgsOk = false;
    g_stateWndActivateCalls = 0;
    g_lastStateWndActivateValue = -1;
    g_zInputOnAppActivateCalls = 0;
    g_zInputOnAppDeactivateCalls = 0;
    g_zGameReturnOnlyStubCalls = 0;
    g_zVideoRestoreIconicCalls = 0;
    g_onActivateCallCount = 0;
    for (int i = 0; i < 5; ++i) {
        g_onActivateCallOrder[i] = 0;
    }

    frame.OnActivate(1, nullptr, TRUE);
    ok = g_onActivateCallCount == 5 && g_onActivateCallOrder[0] == 1 &&
         g_onActivateCallOrder[1] == 2 && g_onActivateCallOrder[2] == 6 &&
         g_onActivateCallOrder[3] == 7 && g_onActivateCallOrder[4] == 8 &&
         g_cFrameWndOnActivateCalls == 1 && g_cFrameWndOnActivateArgsOk &&
         g_stateWndActivateCalls == 1 && g_lastStateWndActivateValue == 1 &&
         g_appActivateCalls == 1 && g_zInputOnAppActivateCalls == 1 &&
         g_zVideoRestoreIconicCalls == 1 && g_appDeactivateCalls == 0 &&
         g_zInputOnAppDeactivateCalls == 0 && g_zGameReturnOnlyStubCalls == 0;

    RestoreFunctionPatch(zVideoRestorePatch);
    RestoreFunctionPatch(zGameReturnPatch);
    RestoreFunctionPatch(zInputDeactivatePatch);
    RestoreFunctionPatch(zInputActivatePatch);
    RestoreFunctionPatch(frameWndOnActivatePatch);
    return ok ? 0 : 3;
}

extern "C" int czgame_frame_on_paint_smoke(void) {
    ImportFunctionPatch createDcPatch{};
    ImportFunctionPatch selectObjectPatch{};
    ImportFunctionPatch bitBltPatch{};
    ImportFunctionPatch stretchBltPatch{};
    ImportFunctionPatch deleteDcPatch{};

    if (!PatchImportByName("GDI32.dll", "CreateCompatibleDC",
                           reinterpret_cast<void *>(&FakeCreateCompatibleDC), createDcPatch) ||
        !PatchImportByName("GDI32.dll", "SelectObject",
                           reinterpret_cast<void *>(&FakeSelectObject), selectObjectPatch) ||
        !PatchImportByName("GDI32.dll", "BitBlt", reinterpret_cast<void *>(&FakeBitBlt),
                           bitBltPatch) ||
        !PatchImportByName("GDI32.dll", "StretchBlt",
                           reinterpret_cast<void *>(&FakeStretchBlt), stretchBltPatch) ||
        !PatchImportByName("GDI32.dll", "DeleteDC", reinterpret_cast<void *>(&FakeDeleteDC),
                           deleteDcPatch)) {
        RestoreImportPatch(deleteDcPatch);
        RestoreImportPatch(stretchBltPatch);
        RestoreImportPatch(bitBltPatch);
        RestoreImportPatch(selectObjectPatch);
        RestoreImportPatch(createDcPatch);
        return 1;
    }

    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-paint-test", WS_POPUP, 0, 0, 400, 300,
                                nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        RestoreImportPatch(deleteDcPatch);
        RestoreImportPatch(stretchBltPatch);
        RestoreImportPatch(bitBltPatch);
        RestoreImportPatch(selectObjectPatch);
        RestoreImportPatch(createDcPatch);
        return 2;
    }

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    CZRecoilFrame frame{};
    frame.m_hWnd = hwnd;
    auto *gameFrame = reinterpret_cast<CZGameFrame *>(&frame);
    gameFrame->m_gameBitmap.m_hObject =
        reinterpret_cast<HGDIOBJ>(static_cast<std::uintptr_t>(0x2468));

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldUpdateMask = g_zVid_CachedClientRectUpdateMask;
    g_zVideo_ActiveRendererPath = 0;
    g_zVid_CachedClientRectUpdateMask = 0;

    g_paintCreateCompatibleDcCalls = 0;
    g_paintSelectObjectCalls = 0;
    g_paintBitBltCalls = 0;
    g_paintStretchBltCalls = 0;
    g_paintDeleteDcCalls = 0;
    g_paintLastDestRect = {};
    g_paintLastSourceRect = {};
    g_paintLastRasterOp = 0;

    RECT smallPaint = {0, 0, 400, 300};
    InvalidateRect(hwnd, nullptr, FALSE);
    gameFrame->OnPaint();
    bool ok = g_paintCreateCompatibleDcCalls == 1 && g_paintSelectObjectCalls == 1 &&
              g_paintBitBltCalls == 1 && g_paintStretchBltCalls == 0 &&
              g_paintDeleteDcCalls == 1 && g_paintLastSourceDc == g_paintCompatibleDc &&
              g_paintLastSelectedObject == gameFrame->m_gameBitmap.m_hObject &&
              g_paintLastDestRect.left == smallPaint.left &&
              g_paintLastDestRect.top == smallPaint.top &&
              g_paintLastDestRect.right == smallPaint.right &&
              g_paintLastDestRect.bottom == smallPaint.bottom &&
              g_paintLastSourceRect.left == smallPaint.left &&
              g_paintLastSourceRect.top == smallPaint.top &&
              g_paintLastSourceRect.right == smallPaint.right &&
              g_paintLastSourceRect.bottom == smallPaint.bottom &&
              g_paintLastRasterOp == SRCCOPY;
    if (!ok) {
        int code = 10;
        if (g_paintCreateCompatibleDcCalls != 1) {
            code = 11;
        } else if (g_paintSelectObjectCalls != 1) {
            code = 12;
        } else if (g_paintBitBltCalls != 1) {
            code = 13;
        } else if (g_paintStretchBltCalls != 0) {
            code = 14;
        } else if (g_paintDeleteDcCalls != 1) {
            code = 15;
        } else if (g_paintLastSourceDc != g_paintCompatibleDc) {
            code = 16;
        } else if (g_paintLastSelectedObject != gameFrame->m_gameBitmap.m_hObject) {
            code = 17;
        } else if (g_paintLastDestRect.left != smallPaint.left) {
            code = 18;
        } else if (g_paintLastDestRect.top != smallPaint.top) {
            code = 19;
        } else if (g_paintLastDestRect.right != smallPaint.right) {
            code = 21;
        } else if (g_paintLastDestRect.bottom != smallPaint.bottom) {
            code = 22;
        } else if (g_paintLastSourceRect.left != smallPaint.left) {
            code = 23;
        } else if (g_paintLastSourceRect.top != smallPaint.top) {
            code = 24;
        } else if (g_paintLastSourceRect.right != smallPaint.right) {
            code = 25;
        } else if (g_paintLastSourceRect.bottom != smallPaint.bottom) {
            code = 26;
        } else if (g_paintLastRasterOp != SRCCOPY) {
            code = 27;
        }
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        DestroyWindow(hwnd);
        RestoreImportPatch(deleteDcPatch);
        RestoreImportPatch(stretchBltPatch);
        RestoreImportPatch(bitBltPatch);
        RestoreImportPatch(selectObjectPatch);
        RestoreImportPatch(createDcPatch);
        return code;
    }

    SetWindowPos(hwnd, nullptr, 0, 0, 800, 700, SWP_NOZORDER | SWP_NOACTIVATE);
    RECT tallPaint = {0, 0, 800, 700};
    InvalidateRect(hwnd, nullptr, FALSE);
    gameFrame->OnPaint();
    ok = g_paintCreateCompatibleDcCalls == 2 && g_paintSelectObjectCalls == 2 &&
         g_paintBitBltCalls == 1 && g_paintStretchBltCalls == 1 &&
         g_paintDeleteDcCalls == 2 && g_paintLastSourceDc == g_paintCompatibleDc &&
         g_paintLastDestRect.left == tallPaint.left &&
         g_paintLastDestRect.top == tallPaint.top &&
         g_paintLastDestRect.right == tallPaint.right &&
         g_paintLastDestRect.bottom == tallPaint.bottom &&
         g_paintLastSourceRect.left == tallPaint.left &&
         g_paintLastSourceRect.top == tallPaint.top &&
         g_paintLastSourceRect.right == tallPaint.left + 640 &&
         g_paintLastSourceRect.bottom == tallPaint.top + 480 &&
         g_paintLastRasterOp == SRCCOPY;
    if (!ok) {
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        DestroyWindow(hwnd);
        RestoreImportPatch(deleteDcPatch);
        RestoreImportPatch(stretchBltPatch);
        RestoreImportPatch(bitBltPatch);
        RestoreImportPatch(selectObjectPatch);
        RestoreImportPatch(createDcPatch);
        return 20;
    }

    g_zVideo_ActiveRendererPath = 2;
    g_zVid_CachedClientRectUpdateMask = 1;
    InvalidateRect(hwnd, nullptr, FALSE);
    gameFrame->OnPaint();
    ok = g_paintCreateCompatibleDcCalls == 2 && g_paintSelectObjectCalls == 2 &&
         g_paintBitBltCalls == 1 && g_paintStretchBltCalls == 1 && g_paintDeleteDcCalls == 2;

    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
    DestroyWindow(hwnd);
    RestoreImportPatch(deleteDcPatch);
    RestoreImportPatch(stretchBltPatch);
    RestoreImportPatch(bitBltPatch);
    RestoreImportPatch(selectObjectPatch);
    RestoreImportPatch(createDcPatch);
    return ok ? 0 : 3;
}

extern "C" int czrecoil_frame_on_size_smoke(void) {
    union DeactivateMemberToFunction {
        void ( FakeGameFrameApp::*member)();
        void( *function)(CZGameFrameApp *);
    };

    DeactivateMemberToFunction deactivateThunk{};
    deactivateThunk.member = &FakeGameFrameApp::OnDeactivate;

    CZGameFrameAppVtable vtable{};
    vtable.OnAppDeactivate = deactivateThunk.function;
    FakeGameFrameApp app{{&vtable}};
    CZRecoilFrame frame{};
    reinterpret_cast<CZGameFrame *>(&frame)->m_app = &app;

    CodeFunctionPatch frameWndOnSizePatch{};
    if (!PatchFunctionJump(CFrameWndOnSizeProc(), reinterpret_cast<void *>(&FakeCFrameWndOnSize),
                           frameWndOnSizePatch)) {
        return 6;
    }

    frame.m_hWnd = CreateWindowExA(0, "STATIC", "recoil-size-test", WS_OVERLAPPEDWINDOW, 0, 0, 100,
                                   100, nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (frame.m_hWnd == nullptr) {
        RestoreFunctionPatch(frameWndOnSizePatch);
        return 4;
    }

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldUpdateMask = g_zVid_CachedClientRectUpdateMask;
    const HWND oldVideoHwnd = g_zVideo_hWnd;
    const RECT oldCachedRect = g_zVideo_CachedClientRectScreen;

    RECT client{};
    GetClientRect(frame.m_hWnd, &client);
    g_zVideo_hWnd = frame.m_hWnd;
    g_zVideo_ActiveRendererPath = 2;
    zVid::SetCachedClientRectUpdateMask(1);
    g_zVideo_CachedClientRectScreen = {7, 8, 9, 10};

    g_appDeactivateCalls = 0;
    g_cFrameWndOnSizeCalls = 0;
    g_cFrameWndOnSizeArgsOk = false;
    frame.OnSize(0, 640, 480);
    if (g_appDeactivateCalls != 0 || g_cFrameWndOnSizeCalls != 1 || !g_cFrameWndOnSizeArgsOk) {
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        g_zVideo_hWnd = oldVideoHwnd;
        g_zVideo_CachedClientRectScreen = oldCachedRect;
        DestroyWindow(frame.m_hWnd);
        RestoreFunctionPatch(frameWndOnSizePatch);
        return 1;
    }

    const LONG cachedWidth =
        g_zVideo_CachedClientRectScreen.right - g_zVideo_CachedClientRectScreen.left;
    const LONG cachedHeight =
        g_zVideo_CachedClientRectScreen.bottom - g_zVideo_CachedClientRectScreen.top;
    if (cachedWidth != client.right - client.left ||
        cachedHeight != client.bottom - client.top) {
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        g_zVideo_hWnd = oldVideoHwnd;
        g_zVideo_CachedClientRectScreen = oldCachedRect;
        DestroyWindow(frame.m_hWnd);
        RestoreFunctionPatch(frameWndOnSizePatch);
        return 5;
    }

    frame.OnSize(1, 640, 480);
    if (g_appDeactivateCalls != 1 || g_cFrameWndOnSizeCalls != 2) {
        g_zVideo_ActiveRendererPath = oldRendererPath;
        g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
        g_zVideo_hWnd = oldVideoHwnd;
        g_zVideo_CachedClientRectScreen = oldCachedRect;
        DestroyWindow(frame.m_hWnd);
        RestoreFunctionPatch(frameWndOnSizePatch);
        return 2;
    }

    frame.OnSize(4, 640, 480);
    const bool ok = g_appDeactivateCalls == 2 && g_cFrameWndOnSizeCalls == 3;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
    g_zVideo_hWnd = oldVideoHwnd;
    g_zVideo_CachedClientRectScreen = oldCachedRect;
    DestroyWindow(frame.m_hWnd);
    RestoreFunctionPatch(frameWndOnSizePatch);
    return ok ? 0 : 3;
}
#endif

#ifdef RECOIL_NATIVE_CZRECOIL_FRAME_TESTS_SKIP_SHARED_SMOKES
namespace {
std::int32_t g_onSizeAppDeactivateCalls;
std::int32_t g_onSizeFrameWndCalls;
bool g_onSizeFrameWndArgsOk;
std::int32_t g_onSizeGameFrameCalls;
bool g_onSizeGameFrameArgsOk;

struct OnSizeFakeRecoilApp : RecoilApp {
    void OnAppDeactivate() override {
        ++g_onSizeAppDeactivateCalls;
    }
};

struct OnSizeCFrameWndAccess : CFrameWnd {
    using CFrameWnd::OnSize;
};

void __fastcall OnSizeFakeCFrameWndOnSize(
    CFrameWnd *,
    void *,
    unsigned int nType,
    int cx,
    int cy
) {
    ++g_onSizeFrameWndCalls;
    g_onSizeFrameWndArgsOk =
        (nType == 0 || nType == 1 || nType == 4) && cx == 640 && cy == 480;
}

void *OnSizeCFrameWndOnSizeProc() {
    union MemberToFunction {
        void ( OnSizeCFrameWndAccess::*member)(unsigned int, int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &OnSizeCFrameWndAccess::OnSize;
    return thunk.function;
}

void __fastcall OnSizeFakeCZGameFrameOnSize(
    CZGameFrame *,
    void *,
    unsigned int nType,
    int cx,
    int cy
) {
    ++g_onSizeGameFrameCalls;
    g_onSizeGameFrameArgsOk =
        (nType == 0 || nType == 1 || nType == 4) && cx == 640 && cy == 480;
}

void *OnSizeCZGameFrameOnSizeProc() {
    union MemberToFunction {
        void ( CZGameFrame::*member)(unsigned int, int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CZGameFrame::OnSize;
    return thunk.function;
}
} // namespace

extern "C" int czrecoil_frame_on_size_smoke(void) {
    OnSizeFakeRecoilApp *const app = new OnSizeFakeRecoilApp();
    CZRecoilFrame frame{};
    reinterpret_cast<CZGameFrame *>(&frame)->m_app = app;

    CodeFunctionPatch gameFrameOnSizePatch{};
    if (!PatchFunctionJump(
            OnSizeCZGameFrameOnSizeProc(),
            reinterpret_cast<void *>(&OnSizeFakeCZGameFrameOnSize),
            gameFrameOnSizePatch
        )) {
        return 6;
    }

    g_onSizeAppDeactivateCalls = 0;
    g_onSizeGameFrameCalls = 0;
    g_onSizeGameFrameArgsOk = false;
    frame.OnSize(0, 640, 480);
    if (g_onSizeAppDeactivateCalls != 0 || g_onSizeGameFrameCalls != 1 ||
        !g_onSizeGameFrameArgsOk) {
        RestoreFunctionPatch(gameFrameOnSizePatch);
        return 1;
    }

    frame.OnSize(1, 640, 480);
    if (g_onSizeAppDeactivateCalls != 1 || g_onSizeGameFrameCalls != 2) {
        RestoreFunctionPatch(gameFrameOnSizePatch);
        return 2;
    }

    frame.OnSize(4, 640, 480);
    if (g_onSizeAppDeactivateCalls != 2 || g_onSizeGameFrameCalls != 3) {
        RestoreFunctionPatch(gameFrameOnSizePatch);
        return 3;
    }
    RestoreFunctionPatch(gameFrameOnSizePatch);
    return 0;
}

extern "C" int czgame_frame_on_size_smoke(void) {
    CZGameFrame frame{};
    CodeFunctionPatch frameWndOnSizePatch{};
    if (!PatchFunctionJump(
            OnSizeCFrameWndOnSizeProc(),
            reinterpret_cast<void *>(&OnSizeFakeCFrameWndOnSize),
            frameWndOnSizePatch
        )) {
        return 6;
    }

    frame.m_hWnd = CreateWindowExA(
        0,
        "STATIC",
        "recoil-size-test",
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        100,
        100,
        nullptr,
        nullptr,
        GetModuleHandleA(nullptr),
        nullptr
    );
    if (frame.m_hWnd == nullptr) {
        RestoreFunctionPatch(frameWndOnSizePatch);
        return 4;
    }

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldUpdateMask = g_zVid_CachedClientRectUpdateMask;
    const HWND oldVideoHwnd = g_zVideo_hWnd;
    const RECT oldCachedRect = g_zVideo_CachedClientRectScreen;

    RECT client{};
    GetClientRect(frame.m_hWnd, &client);
    g_zVideo_hWnd = frame.m_hWnd;
    g_zVideo_ActiveRendererPath = 2;
    zVid::SetCachedClientRectUpdateMask(1);
    g_zVideo_CachedClientRectScreen = {7, 8, 9, 10};
    g_onSizeFrameWndCalls = 0;
    g_onSizeFrameWndArgsOk = false;

    frame.OnSize(0, 640, 480);
    const LONG cachedWidth =
        g_zVideo_CachedClientRectScreen.right - g_zVideo_CachedClientRectScreen.left;
    const LONG cachedHeight =
        g_zVideo_CachedClientRectScreen.bottom - g_zVideo_CachedClientRectScreen.top;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVid_CachedClientRectUpdateMask = oldUpdateMask;
    g_zVideo_hWnd = oldVideoHwnd;
    g_zVideo_CachedClientRectScreen = oldCachedRect;
    DestroyWindow(frame.m_hWnd);
    RestoreFunctionPatch(frameWndOnSizePatch);
    if (g_onSizeFrameWndCalls != 1) {
        return 7;
    }
    if (!g_onSizeFrameWndArgsOk) {
        return 8;
    }
    if (cachedWidth != client.right - client.left ||
        cachedHeight != client.bottom - client.top) {
        return 9;
    }
    return 0;
}
#endif
