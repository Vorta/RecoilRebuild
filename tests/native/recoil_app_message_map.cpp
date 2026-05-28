#include "Battlesport/RecoilApp.h"

#include "Battlesport/Briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/player.h"
#include "Battlesport/hud.h"
#include "Battlesport/Recoil.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <commdlg.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" std::int32_t g_zSndCdFlags;
extern "C" HWND g_RecoilApp_hWndMain;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" const char *g_RecoilApp_WndClassNamePtr;
extern "C" int g_RecoilApp_AttractFmvReloadMode;
BOOL RECOIL_STDCALL AfxWinInit(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine,
                               int showCommand);

struct RecoilStateCredits {
    RecoilPtr32 vftable;
    RecoilPtr32 dialog;

    RecoilStateCredits *RECOIL_THISCALL Constructor();
    ~RecoilStateCredits();
    static void RECOIL_CDECL QueuePush();
};

namespace {
void AtexitProviderNoOp() {}

int g_stateEnterCount;
int g_stateExitCount;
int g_stateIdleCount;
int g_stateTryBecomeCurrentCount;
int g_stateTryBecomeCurrentResult;
int g_stateUpdateShouldQuitCount;
int g_stateUpdateShouldQuitResult;
int g_stateDeactivateCount;
int g_stateSuspendCount;
int g_stateSuspendParam;
int g_stateResumeCount;
int g_stateResumeParam;
std::uint32_t g_stateIdleWParam;
std::uint32_t g_stateIdleLParam;
int g_startEngineCount;
int g_shutdownEngineCount;
int g_exitInstanceCount;
int g_startEngineResult;
RecoilPtr32 g_lastStartEngineHwnd;
CZRecoilFrame *g_createMainWndResult;
int g_playStateLayoutActivatedCount;
int g_runPumpMessageCount;
int g_runPumpMessageResult;
int g_runOnAppDeactivateCount;
int g_runPeekMessageCount;
int g_runPostQuitCount;
int g_runPostQuitCode;
int g_runWaitMessageCount;
int g_runSetThreadPriorityCount;
int g_runSetThreadPriorityValue;
int g_runReceivePendingMessagesCount;
int g_runReceivePendingMessagesBudget;
int g_runQuitPosted;
int g_runPeekMessageTrueAfterReceive;
int g_saveLoadUpdatePollCalls;
int g_saveLoadUpdatePollDispatch;
int g_saveLoadUpdateTimeCalls;
int g_saveLoadUpdatePostprocessCalls;
int g_saveLoadUpdateUnlockCalls;
int g_saveLoadUpdateAdjustCalls;
int g_saveLoadUpdateAdjustWait;
int g_saveLoadUpdateAdjustBlit;
float g_saveLoadUpdateDelta;
zVidRect32 *g_saveLoadUpdateAdjustSrc;
zVidRect32 *g_saveLoadUpdateAdjustDst;
int g_confirmQuitPostprocessCalls;
int g_confirmQuitBlitCalls;
int g_confirmQuitUnlockCalls;
int g_confirmQuitSleepCalls;
DWORD g_confirmQuitSleepMilliseconds;
int g_openFileNameCalls;
bool g_openFileNameStructOk;
char g_openFileNameSelectedPath[MAX_PATH];

struct TestCreditsPanel {
    virtual void RECOIL_THISCALL Update(float) {}
    virtual void RECOIL_THISCALL SetEnabled(int enabled) {
        ++setEnabledCount;
        lastEnabled = enabled;
    }
    virtual TestCreditsPanel *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags) {
        ++scalarDeletingCount;
        lastScalarDeletingFlags = flags;
        return this;
    }

    int setEnabledCount = 0;
    int lastEnabled = -1;
    int scalarDeletingCount = 0;
    unsigned int lastScalarDeletingFlags = 0;
};

struct TestConfirmQuitDialog {
    virtual void RECOIL_THISCALL Update(float) {}
    virtual void RECOIL_THISCALL SetEnabled(int enabled) {
        ++setEnabledCount;
        lastEnabled = enabled;
    }
    virtual TestConfirmQuitDialog *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags) {
        ++scalarDeletingCount;
        lastScalarDeletingFlags = flags;
        return this;
    }

    int setEnabledCount = 0;
    int lastEnabled = -1;
    int scalarDeletingCount = 0;
    unsigned int lastScalarDeletingFlags = 0;
};

struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

zZbdManager MakeTestZbdManager(zZbdSectionHandlerNode &sentinel) {
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    sentinel.sectionHandler = {};

    zZbdManager manager = {};
    manager.sectionHandlerListSentinel = &sentinel;
    return manager;
}

void ClearTestRegisteredHandlers(zZbdSectionHandlerNode &sentinel) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *next = node->next;
        delete node;
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

BOOL WINAPI FakeRunPeekMessageA(LPMSG, HWND, UINT, UINT, UINT) {
    ++g_runPeekMessageCount;
    return g_runQuitPosted != 0 ||
                   (g_runPeekMessageTrueAfterReceive != 0 &&
                    g_runReceivePendingMessagesCount > 0)
               ? TRUE
               : FALSE;
}

void WINAPI FakeRunPostQuitMessage(int exitCode) {
    ++g_runPostQuitCount;
    g_runPostQuitCode = exitCode;
    g_runQuitPosted = 1;
}

BOOL WINAPI FakeRunWaitMessage() {
    ++g_runWaitMessageCount;
    return TRUE;
}

BOOL WINAPI FakeRunSetThreadPriority(HANDLE, int priority) {
    ++g_runSetThreadPriorityCount;
    g_runSetThreadPriorityValue = priority;
    return TRUE;
}

int RECOIL_FASTCALL FakeRunReceivePendingMessages(int messageBudget) {
    ++g_runReceivePendingMessagesCount;
    g_runReceivePendingMessagesBudget = messageBudget;
    return 0;
}

void RECOIL_FASTCALL FakeSaveLoadUpdatePollActiveDevices(int dispatchCallbacks) {
    ++g_saveLoadUpdatePollCalls;
    g_saveLoadUpdatePollDispatch = dispatchCallbacks;
}

void RECOIL_CDECL FakeSaveLoadUpdateTimeTick() {
    ++g_saveLoadUpdateTimeCalls;
    g_FrameDeltaTimeSec = 0.25f;
}

int RECOIL_CDECL FakeSaveLoadUpdateRunPostprocessOnPrimaryBuffer() {
    ++g_saveLoadUpdatePostprocessCalls;
    return 0;
}

int RECOIL_CDECL FakeSaveLoadUpdateUnlockPrimarySurfaceState() {
    ++g_saveLoadUpdateUnlockCalls;
    return 0;
}

int RECOIL_FASTCALL FakeSaveLoadUpdateAdjustSurfaces(zVidRect32 *srcRect, zVidRect32 *dstRect,
                                                     int waitForPresent,
                                                     int blitPrimaryToSwFirst) {
    ++g_saveLoadUpdateAdjustCalls;
    g_saveLoadUpdateAdjustSrc = srcRect;
    g_saveLoadUpdateAdjustDst = dstRect;
    g_saveLoadUpdateAdjustWait = waitForPresent;
    g_saveLoadUpdateAdjustBlit = blitPrimaryToSwFirst;
    return 0;
}

int RECOIL_CDECL FakeConfirmQuitRunPostprocessOnPrimaryBuffer() {
    ++g_confirmQuitPostprocessCalls;
    return 0;
}

int RECOIL_CDECL FakeConfirmQuitUnlockPrimarySurfaceState() {
    ++g_confirmQuitUnlockCalls;
    return 0;
}

struct FakeConfirmQuitBlitThunk {
    void RECOIL_THISCALL BlitOwnedSurfaceToPrimary();
};

void RECOIL_THISCALL FakeConfirmQuitBlitThunk::BlitOwnedSurfaceToPrimary() {
    ++g_confirmQuitBlitCalls;
}

void WINAPI FakeConfirmQuitSleep(DWORD milliseconds) {
    ++g_confirmQuitSleepCalls;
    g_confirmQuitSleepMilliseconds = milliseconds;
}

bool PatchImportByName(const char *dllName, const char *functionName, void *replacement,
                       ImportFunctionPatch &patch) {
    unsigned char *const imageBase = reinterpret_cast<unsigned char *>(GetModuleHandleA(nullptr));
    auto *const dos = reinterpret_cast<IMAGE_DOS_HEADER *>(imageBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    auto *const nt = reinterpret_cast<IMAGE_NT_HEADERS *>(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    auto *descriptor =
        reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll =
            reinterpret_cast<const char *>(imageBase + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0) {
            continue;
        }

        auto *nameThunk = reinterpret_cast<IMAGE_THUNK_DATA *>(
            imageBase + (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                             : descriptor->FirstThunk));
        auto *addressThunk =
            reinterpret_cast<IMAGE_THUNK_DATA *>(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)) {
                continue;
            }

            auto *const importName = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(
                imageBase + nameThunk->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char *>(importName->Name), functionName) != 0) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                               &oldProtect) == 0) {
                return false;
            }

            *patch.slot = static_cast<ULONG_PTR>(reinterpret_cast<std::uintptr_t>(replacement));
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

bool PatchImportByOrdinal(const char *dllName, WORD ordinal, void *replacement,
                          ImportFunctionPatch &patch) {
    unsigned char *const imageBase = reinterpret_cast<unsigned char *>(GetModuleHandleA(nullptr));
    auto *const dos = reinterpret_cast<IMAGE_DOS_HEADER *>(imageBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    auto *const nt = reinterpret_cast<IMAGE_NT_HEADERS *>(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    auto *descriptor =
        reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll =
            reinterpret_cast<const char *>(imageBase + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0) {
            continue;
        }

        auto *nameThunk = reinterpret_cast<IMAGE_THUNK_DATA *>(
            imageBase + (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                             : descriptor->FirstThunk));
        auto *addressThunk =
            reinterpret_cast<IMAGE_THUNK_DATA *>(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (!IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal) ||
                static_cast<WORD>(nameThunk->u1.Ordinal & 0xffff) != ordinal) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                               &oldProtect) == 0) {
                return false;
            }

            *patch.slot = static_cast<ULONG_PTR>(reinterpret_cast<std::uintptr_t>(replacement));
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

void RestoreImportPatch(ImportFunctionPatch &patch) {
    if (patch.slot == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE, &oldProtect) != 0) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }

    patch.slot = nullptr;
    patch.original = 0;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
        patch.address = nullptr;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(patch.address + sizeof(patch.original));
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }

    patch.address = nullptr;
}

struct RunPatchSet {
    ImportFunctionPatch peek;
    ImportFunctionPatch postQuit;
    ImportFunctionPatch wait;
    ImportFunctionPatch priority;
    CodeFunctionPatch receive;
};

void RestoreRunPatches(RunPatchSet &patches) {
    RestoreFunctionPatch(patches.receive);
    RestoreImportPatch(patches.priority);
    RestoreImportPatch(patches.wait);
    RestoreImportPatch(patches.postQuit);
    RestoreImportPatch(patches.peek);
}

int InstallRunPatches(RunPatchSet &patches) {
    if (!PatchImportByName("USER32.dll", "PeekMessageA",
                           reinterpret_cast<void *>(&FakeRunPeekMessageA), patches.peek)) {
        return 1;
    }
    if (!PatchImportByName("USER32.dll", "PostQuitMessage",
                           reinterpret_cast<void *>(&FakeRunPostQuitMessage), patches.postQuit)) {
        RestoreRunPatches(patches);
        return 2;
    }
    if (!PatchImportByName("USER32.dll", "WaitMessage",
                           reinterpret_cast<void *>(&FakeRunWaitMessage), patches.wait)) {
        RestoreRunPatches(patches);
        return 3;
    }
    if (!PatchImportByName("KERNEL32.dll", "SetThreadPriority",
                           reinterpret_cast<void *>(&FakeRunSetThreadPriority),
                           patches.priority)) {
        RestoreRunPatches(patches);
        return 4;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zNetworkDPlay::ReceivePendingMessages),
                           reinterpret_cast<void *>(&FakeRunReceivePendingMessages),
                           patches.receive)) {
        RestoreRunPatches(patches);
        return 5;
    }

    return 0;
}

void ResetRunHarnessCounters() {
    g_stateEnterCount = 0;
    g_stateExitCount = 0;
    g_stateTryBecomeCurrentCount = 0;
    g_stateTryBecomeCurrentResult = 0;
    g_stateUpdateShouldQuitCount = 0;
    g_stateUpdateShouldQuitResult = 0;
    g_stateDeactivateCount = 0;
    g_stateSuspendCount = 0;
    g_stateSuspendParam = 0;
    g_stateResumeCount = 0;
    g_stateResumeParam = 0;
    g_runPumpMessageCount = 0;
    g_runPumpMessageResult = 0;
    g_runOnAppDeactivateCount = 0;
    g_runPeekMessageCount = 0;
    g_runPostQuitCount = 0;
    g_runPostQuitCode = -1;
    g_runWaitMessageCount = 0;
    g_runSetThreadPriorityCount = 0;
    g_runSetThreadPriorityValue = 0;
    g_runReceivePendingMessagesCount = 0;
    g_runReceivePendingMessagesBudget = 0;
    g_runQuitPosted = 0;
    g_runPeekMessageTrueAfterReceive = 0;
    g_exitInstanceCount = 0;
}

bool FilterStringMatchesOpenFileDialogContract(const char *filter) {
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

BOOL WINAPI FakeGetOpenFileNameSuccessA(LPOPENFILENAMEA ofn) {
    ++g_openFileNameCalls;
    g_openFileNameStructOk =
        ofn != nullptr && ofn->lStructSize == 0x4c && ofn->hwndOwner != nullptr &&
        FilterStringMatchesOpenFileDialogContract(ofn->lpstrFilter) &&
        ofn->nFilterIndex == 1 && ofn->lpstrFile != nullptr && ofn->nMaxFile == 0x104 &&
        ofn->lpstrFileTitle != nullptr && ofn->nMaxFileTitle == 0x200 &&
        ofn->Flags == (OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST) &&
        ofn->lpstrDefExt != nullptr && std::strcmp(ofn->lpstrDefExt, "gs") == 0;

    strcpy_s(ofn->lpstrFile, ofn->nMaxFile, g_openFileNameSelectedPath);
    return TRUE;
}

UINT_PTR g_findExecutableResult;
int g_findExecutableCalls;
bool g_findExecutableArgsOk;
int g_shellExecuteCalls;
bool g_shellExecuteArgsOk;
int g_helpDocsMessageBoxCalls;
bool g_helpDocsMessageBoxArgsOk;
const char *g_expectedHelpDocsMessageText;
int g_aboutDialogCtorCalls;
int g_aboutDialogDoModalCalls;
int g_aboutDialogDtorCalls;
bool g_aboutDialogCtorArgsOk;
bool g_aboutDialogFlowOk;
CDialog *g_aboutDialogThis;
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
int g_startModeSetupCalls;
bool g_startModeSetupArgsOk;
RecoilApp *g_startModeSetupThis;
int g_startModeExpectedMissionId;
int g_startModeExpectedMissionFlags;
int g_hwApiSelectEnsureCalls;
bool g_hwApiSelectEnsureArgsOk;
CZRecoilFrame *g_hwApiSelectExpectedFrame;
int g_hwApiSelectExpectedSelector;
int g_saveGameInitLoadCalls;
bool g_saveGameInitLoadArgsOk;
const char *g_saveLoadInitExpectedSectionName;

constexpr WORD kMfc42CDialogResourceCtorOrdinal = 324;
constexpr WORD kMfc42CDialogDtorOrdinal = 641;
constexpr WORD kMfc42CDialogDoModalOrdinal = 2514;

char *RECOIL_FASTCALL FakeHelpDocsGetMessageString(unsigned int messageId) {
    switch (messageId) {
    case 0x12:
        ++g_fatalLocCaptionCalls;
        return const_cast<char *>("Recoil");
    case 0x19:
        return const_cast<char *>("Help");
    case 0x20:
        return const_cast<char *>("No file association");
    case 0x21:
        return const_cast<char *>("No DDE association");
    case 0x22:
        return const_cast<char *>("File not found");
    case 0x24:
        return const_cast<char *>("Association incomplete");
    case 0x30:
        ++g_fatalLocTextCalls;
        return const_cast<char *>("Fatal error");
    default:
        return const_cast<char *>("");
    }
}

HINSTANCE WINAPI FakeHelpDocsFindExecutableA(LPCSTR file, LPCSTR directory, LPSTR result) {
    ++g_findExecutableCalls;
    g_findExecutableArgsOk =
        file != nullptr && std::strcmp(file, "Docs\\Index.html") == 0 && directory == nullptr &&
        result != nullptr;
    if (result != nullptr) {
        strcpy_s(result, 0x100, "fake-browser.exe");
    }

    return reinterpret_cast<HINSTANCE>(g_findExecutableResult);
}

HINSTANCE WINAPI FakeHelpDocsShellExecuteA(HWND hwnd, LPCSTR operation, LPCSTR file,
                                           LPCSTR parameters, LPCSTR directory, INT showCommand) {
    ++g_shellExecuteCalls;
    g_shellExecuteArgsOk =
        hwnd == g_RecoilApp_hWndMain && operation != nullptr &&
        std::strcmp(operation, "open") == 0 && file != nullptr &&
        std::strcmp(file, "Docs\\Index.html") == 0 && parameters == nullptr &&
        directory == nullptr && showCommand == SW_HIDE;
    return reinterpret_cast<HINSTANCE>(33);
}

int WINAPI FakeHelpDocsMessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type) {
    ++g_helpDocsMessageBoxCalls;
    g_helpDocsMessageBoxArgsOk =
        hwnd == g_RecoilApp_hWndMain && text == g_expectedHelpDocsMessageText &&
        caption != nullptr && std::strcmp(caption, "Help") == 0 && type == 0x30;
    return IDOK;
}

void ResetAboutDialogProbe() {
    g_aboutDialogCtorCalls = 0;
    g_aboutDialogDoModalCalls = 0;
    g_aboutDialogDtorCalls = 0;
    g_aboutDialogCtorArgsOk = false;
    g_aboutDialogFlowOk = true;
    g_aboutDialogThis = nullptr;
}

void RECOIL_FASTCALL FakeAboutCDialogCtor(CDialog *self, void *, UINT resourceId,
                                          CWnd *parentWnd) {
    ++g_aboutDialogCtorCalls;
    g_aboutDialogThis = self;
    g_aboutDialogCtorArgsOk = resourceId == 0x67 && parentWnd == nullptr;
}

int RECOIL_FASTCALL FakeAboutCDialogDoModal(CDialog *self, void *) {
    ++g_aboutDialogDoModalCalls;
    g_aboutDialogFlowOk = g_aboutDialogFlowOk && g_aboutDialogCtorCalls == 1 &&
                          g_aboutDialogDtorCalls == 0 && self == g_aboutDialogThis;
    return IDOK;
}

void RECOIL_FASTCALL FakeAboutCDialogDtor(CDialog *self, void *) {
    ++g_aboutDialogDtorCalls;
    g_aboutDialogFlowOk = g_aboutDialogFlowOk && g_aboutDialogCtorCalls == 1 &&
                          g_aboutDialogDoModalCalls == 1 && self == g_aboutDialogThis;
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

void ExpectFatalStep(int expected) {
    ++g_fatalSequence;
    g_fatalOrderOk = g_fatalOrderOk && g_fatalSequence == expected;
}

void RECOIL_FASTCALL FakeFatalBriefingStop(int waitForInput) {
    ++g_fatalBriefingCalls;
    ExpectFatalStep(1);
    g_fatalArgsOk = g_fatalArgsOk && waitForInput == 0;
}

void RECOIL_CDECL FakeFatalFlipToGDI() {
    ++g_fatalFlipCalls;
    ExpectFatalStep(2);
}

int RECOIL_CDECL FakeFatalSndShutdown() {
    ++g_fatalSndCalls;
    ExpectFatalStep(3);
    return 1;
}

int RECOIL_CDECL FakeFatalNetworkShutdown() {
    ++g_fatalNetworkCalls;
    ExpectFatalStep(4);
    return 1;
}

int RECOIL_CDECL FakeFatalVideoShutdown() {
    ++g_fatalVideoCalls;
    ExpectFatalStep(5);
    return 1;
}

void WINAPI FakeFatalSleep(DWORD milliseconds) {
    ++g_fatalSleepCalls;
    ExpectFatalStep(6);
    g_fatalArgsOk = g_fatalArgsOk && milliseconds == 1000;
}

BOOL WINAPI FakeFatalMessageBeep(UINT type) {
    ++g_fatalBeepCalls;
    ExpectFatalStep(7);
    g_fatalArgsOk = g_fatalArgsOk && type == MB_ICONHAND;
    return TRUE;
}

int WINAPI FakeFatalMessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type) {
    ++g_fatalMessageBoxCalls;
    ExpectFatalStep(8);
    g_fatalArgsOk = g_fatalArgsOk && hwnd == g_RecoilApp_hWndMain && text != nullptr &&
                    std::strcmp(text, "Fatal error") == 0 && caption != nullptr &&
                    std::strcmp(caption, "Recoil") == 0 && type == MB_ICONHAND;
    return IDOK;
}

void RECOIL_FASTCALL FakeFatalExitProcessWithCleanup(int exitCode) {
    ++g_fatalExitCalls;
    ExpectFatalStep(9);
    g_fatalArgsOk = g_fatalArgsOk && exitCode == 0;
}

int RECOIL_FASTCALL FakeStartModeLoadZbdAndSetup(RecoilApp *self, void *, int missionId,
                                                 const char *zbdPath, int skipIntroFmvMode,
                                                 int missionFlags) {
    ++g_startModeSetupCalls;
    g_startModeSetupThis = self;
    g_startModeSetupArgsOk =
        missionId == g_startModeExpectedMissionId && zbdPath == nullptr &&
        skipIntroFmvMode == 1 && missionFlags == g_startModeExpectedMissionFlags;
    return 1;
}

void *RecoilAppLoadSetupProc() {
    union MemberToFunction {
        int(RECOIL_THISCALL RecoilApp::*member)(int, const char *, int, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &RecoilApp::LoadZbdAndSetupSensorTracker;
    return thunk.function;
}

bool RunStartModeScenario(void (RECOIL_THISCALL CZRecoilFrame::*handler)(), int missionId,
                          int missionFlags) {
    g_startModeSetupCalls = 0;
    g_startModeSetupArgsOk = false;
    g_startModeSetupThis = nullptr;
    g_startModeExpectedMissionId = missionId;
    g_startModeExpectedMissionFlags = missionFlags;

    CZRecoilFrame frame{};
    frame.m_useArchiveBanks = missionFlags;
    (frame.*handler)();

    return g_startModeSetupCalls == 1 && g_startModeSetupArgsOk &&
           g_startModeSetupThis == &g_RecoilApp;
}

void RECOIL_FASTCALL FakeMenuSelectEnsureHwApiInitialized(CZRecoilFrame *self, void *,
                                                          int selector) {
    ++g_hwApiSelectEnsureCalls;
    g_hwApiSelectEnsureArgsOk =
        self == g_hwApiSelectExpectedFrame && selector == g_hwApiSelectExpectedSelector;
}

void *EnsureHwApiInitializedProc() {
    union MemberToFunction {
        void (RECOIL_THISCALL CZRecoilFrame::*member)(int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &CZRecoilFrame::EnsureHwApiInitialized;
    return thunk.function;
}

bool RunHwApiSelectScenario(void (RECOIL_THISCALL CZRecoilFrame::*handler)(), int selector) {
    CZRecoilFrame frame{};
    g_hwApiSelectExpectedFrame = &frame;
    g_hwApiSelectExpectedSelector = selector;
    g_hwApiSelectEnsureCalls = 0;
    g_hwApiSelectEnsureArgsOk = false;

    (frame.*handler)();

    return g_hwApiSelectEnsureCalls == 1 && g_hwApiSelectEnsureArgsOk;
}

struct FakeSaveGameInitLoadThunk {
    zReader::Node *RECOIL_THISCALL LoadFromZrd(const char *zrdPath, const char *sectionName,
                                               int capturePrimary);
};

zReader::Node *RECOIL_THISCALL FakeSaveGameInitLoadThunk::LoadFromZrd(
    const char *zrdPath, const char *sectionName, int capturePrimary) {
    ++g_saveGameInitLoadCalls;
    g_saveGameInitLoadArgsOk =
        this != nullptr && zrdPath != nullptr && std::strcmp(zrdPath, "dialog.zrd") == 0 &&
        sectionName != nullptr &&
        g_saveLoadInitExpectedSectionName != nullptr &&
        std::strcmp(sectionName, g_saveLoadInitExpectedSectionName) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *FakeSaveGameInitLoadFromZrdProc() {
    union MemberToFunction {
        zReader::Node *(RECOIL_THISCALL FakeSaveGameInitLoadThunk::*member)(const char *,
                                                                            const char *, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &FakeSaveGameInitLoadThunk::LoadFromZrd;
    return thunk.function;
}

void *HudUiBackgroundLoadFromZrdProc() {
    union MemberToFunction {
        zReader::Node *(RECOIL_THISCALL HudUiBackground::*member)(const char *, const char *,
                                                                  int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &HudUiBackground::LoadFromZrd;
    return thunk.function;
}

bool CStringIsEmpty(const CString &value) {
    return value.m_pchData != nullptr && value.m_pchData[0] == '\0';
}

bool CStringEquals(const CString &value, const char *text) {
    return value.m_pchData != nullptr && std::strcmp(value.m_pchData, text) == 0;
}

bool ReadFilePrefix(const char *path, char *buffer, DWORD bufferSize) {
    HANDLE file = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesRead = 0;
    const BOOL ok = ReadFile(file, buffer, bufferSize - 1, &bytesRead, nullptr);
    CloseHandle(file);
    if (ok == 0) {
        return false;
    }

    buffer[bytesRead] = '\0';
    return true;
}

FILETIME MakeSaveLoadSmokeFileTime(DWORD lowPart) {
    FILETIME time{};
    time.dwLowDateTime = lowPart;
    time.dwHighDateTime = 30000000;
    return time;
}

bool WriteSaveLoadSmokeFile(const char *path, DWORD timeLowPart) {
    HANDLE file = CreateFileA(
        path,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesWritten = 0;
    const char payload[] = "save";
    const BOOL writeOk =
        WriteFile(file, payload, static_cast<DWORD>(sizeof(payload) - 1), &bytesWritten, nullptr);
    const FILETIME writeTime = MakeSaveLoadSmokeFileTime(timeLowPart);
    const BOOL timeOk = SetFileTime(file, nullptr, nullptr, &writeTime);
    CloseHandle(file);

    return writeOk != 0 && timeOk != 0 && bytesWritten == sizeof(payload) - 1;
}

void CleanupSaveLoadSmokeFiles(bool removeDirectory) {
    DeleteFileA("SavedGames\\recoil_refresh_smoke_old.sav");
    DeleteFileA("SavedGames\\recoil_refresh_smoke_new.sav");
    DeleteFileA("SavedGames\\recoil_refresh_smoke_middle.sav");
    DeleteFileA("SavedGames\\recoil_refresh_smoke_oldest.sav");
    DeleteFileA("SavedGames\\recoil_process_smoke.sav");
    RemoveDirectoryA("SavedGames\\recoil_refresh_smoke_dir");
    if (removeDirectory) {
        RemoveDirectoryA("SavedGames");
    }
}

HudUiSaveLoadDialog *g_saveLoadInitDialog = nullptr;
int g_saveLoadInitTextCount = 0;
int g_saveLoadInitVisibleCount = 0;
int g_saveLoadInitInvalidateCount = 0;
bool g_saveLoadInitFmtOk = false;
bool g_saveLoadInitVisibleOk = false;
char g_saveLoadInitTexts[9][MAX_PATH]{};
int g_saveLoadInitTextIndices[9]{};
int g_saveLoadInitVisibleIndices[9]{};
int g_saveLoadInitVisibleValues[9]{};
int g_saveLoadInitInvalidateIndices[9]{};

int SaveLoadInitListItemIndex(HudUiSaveLoadListItem *item) {
    return static_cast<int>(item - g_saveLoadInitDialog->entryWidgets);
}

void ResetSaveLoadInitListItemCapture(HudUiSaveLoadDialog *dialog) {
    g_saveLoadInitDialog = dialog;
    g_saveLoadInitTextCount = 0;
    g_saveLoadInitVisibleCount = 0;
    g_saveLoadInitInvalidateCount = 0;
    g_saveLoadInitFmtOk = true;
    g_saveLoadInitVisibleOk = true;
    std::memset(g_saveLoadInitTexts, 0, sizeof(g_saveLoadInitTexts));
    std::memset(g_saveLoadInitTextIndices, 0, sizeof(g_saveLoadInitTextIndices));
    std::memset(g_saveLoadInitVisibleIndices, 0, sizeof(g_saveLoadInitVisibleIndices));
    std::memset(g_saveLoadInitVisibleValues, 0, sizeof(g_saveLoadInitVisibleValues));
    std::memset(g_saveLoadInitInvalidateIndices, 0, sizeof(g_saveLoadInitInvalidateIndices));
}

void RECOIL_CDECL FakeSaveLoadListItemSetTextFmt(HudUiSaveLoadListItem *self,
                                                 const char *format,
                                                 const char *text) {
    const int index = SaveLoadInitListItemIndex(self);
    if (g_saveLoadInitTextCount >= 9 || index < 0 || index >= 9 ||
        std::strcmp(format, "%s") != 0) {
        g_saveLoadInitFmtOk = false;
        return;
    }

    g_saveLoadInitTextIndices[g_saveLoadInitTextCount] = index;
    std::strncpy(g_saveLoadInitTexts[g_saveLoadInitTextCount],
                 text,
                 sizeof(g_saveLoadInitTexts[g_saveLoadInitTextCount]) - 1);
    ++g_saveLoadInitTextCount;
}

struct FakeSaveLoadListItemVisibleThunk {
    void RECOIL_THISCALL SetVisible(int visible);
};

void RECOIL_THISCALL FakeSaveLoadListItemVisibleThunk::SetVisible(int visible) {
    HudUiSaveLoadListItem *self = (HudUiSaveLoadListItem *)(this);
    const int index = SaveLoadInitListItemIndex(self);
    if (g_saveLoadInitVisibleCount >= 9 || index < 0 || index >= 9) {
        g_saveLoadInitVisibleOk = false;
        return;
    }

    g_saveLoadInitVisibleIndices[g_saveLoadInitVisibleCount] = index;
    g_saveLoadInitVisibleValues[g_saveLoadInitVisibleCount] = visible;
    ++g_saveLoadInitVisibleCount;
}

HudUiSaveLoadSetVisibleFn SaveLoadListItemSetVisibleThunk() {
    union MemberToFunction {
        void (RECOIL_THISCALL FakeSaveLoadListItemVisibleThunk::*member)(int);
        HudUiSaveLoadSetVisibleFn function;
    };

    MemberToFunction thunk{};
    thunk.member = &FakeSaveLoadListItemVisibleThunk::SetVisible;
    return thunk.function;
}

struct FakeSaveLoadListItemInvalidateThunk {
    void RECOIL_THISCALL Invalidate();
};

void RECOIL_THISCALL FakeSaveLoadListItemInvalidateThunk::Invalidate() {
    HudUiSaveLoadListItem *self = (HudUiSaveLoadListItem *)(this);
    const int index = SaveLoadInitListItemIndex(self);
    if (g_saveLoadInitInvalidateCount >= 9 || index < 0 || index >= 9) {
        g_saveLoadInitVisibleOk = false;
        return;
    }

    g_saveLoadInitInvalidateIndices[g_saveLoadInitInvalidateCount] = index;
    ++g_saveLoadInitInvalidateCount;
}

HudUiSaveLoadInvalidateFn SaveLoadListItemInvalidateThunk() {
    union MemberToFunction {
        void (RECOIL_THISCALL FakeSaveLoadListItemInvalidateThunk::*member)();
        HudUiSaveLoadInvalidateFn function;
    };

    MemberToFunction thunk{};
    thunk.member = &FakeSaveLoadListItemInvalidateThunk::Invalidate;
    return thunk.function;
}

struct TestAppState : RecoilApp_IState {
    void RECOIL_THISCALL OnEnter() {
        ++g_stateEnterCount;
    }

    void RECOIL_THISCALL OnExit() {
        ++g_stateExitCount;
    }

    std::int32_t RECOIL_THISCALL OnTryBecomeCurrent() {
        ++g_stateTryBecomeCurrentCount;
        return g_stateTryBecomeCurrentResult;
    }

    std::int32_t RECOIL_THISCALL OnUpdateShouldQuit() {
        ++g_stateUpdateShouldQuitCount;
        return g_stateUpdateShouldQuitResult;
    }

    void RECOIL_THISCALL OnDeactivate() {
        ++g_stateDeactivateCount;
    }

    void RECOIL_THISCALL OnSuspend(int param) {
        ++g_stateSuspendCount;
        g_stateSuspendParam = param;
    }

    void RECOIL_THISCALL OnResume(int param) {
        ++g_stateResumeCount;
        g_stateResumeParam = param;
    }

    std::int32_t RECOIL_THISCALL OnIdleOrDispatch(std::uint32_t wParam, std::uint32_t lParam) {
        ++g_stateIdleCount;
        g_stateIdleWParam = wParam;
        g_stateIdleLParam = lParam;
        return 123;
    }
};

RecoilFn32 StateMethodToFn(void (RECOIL_THISCALL TestAppState::*method)()) {
    union MemberToFunction {
        void (RECOIL_THISCALL TestAppState::*member)();
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 StateIntMethodToFn(std::int32_t (RECOIL_THISCALL TestAppState::*method)()) {
    union MemberToFunction {
        std::int32_t (RECOIL_THISCALL TestAppState::*member)();
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 StateParamMethodToFn(void (RECOIL_THISCALL TestAppState::*method)(int)) {
    union MemberToFunction {
        void (RECOIL_THISCALL TestAppState::*member)(int);
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 StateIdleMethodToFn(
    std::int32_t (RECOIL_THISCALL TestAppState::*method)(std::uint32_t, std::uint32_t)) {
    union MemberToFunction {
        std::int32_t (RECOIL_THISCALL TestAppState::*member)(std::uint32_t, std::uint32_t);
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilApp_IState_Vtbl MakeTestAppStateVtable() {
    RecoilApp_IState_Vtbl vtable{};
    vtable.OnEnter = StateMethodToFn(&TestAppState::OnEnter);
    vtable.OnCanBecomeCurrent = StateIntMethodToFn(&TestAppState::OnTryBecomeCurrent);
    vtable.OnUpdateShouldQuit = StateIntMethodToFn(&TestAppState::OnUpdateShouldQuit);
    vtable.OnExit = StateMethodToFn(&TestAppState::OnExit);
    vtable.OnDeactivate = StateMethodToFn(&TestAppState::OnDeactivate);
    vtable.OnSuspend = StateParamMethodToFn(&TestAppState::OnSuspend);
    vtable.OnResume = StateParamMethodToFn(&TestAppState::OnResume);
    vtable.OnIdleOrDispatch = StateIdleMethodToFn(&TestAppState::OnIdleOrDispatch);
    return vtable;
}

RecoilApp_IState_Vtbl g_testAppStateVtable = MakeTestAppStateVtable();

void RECOIL_FASTCALL TestPlayStateLayoutOnActivated(HudLayoutBase *) {
    ++g_playStateLayoutActivatedCount;
}

struct TestRecoilApp : RecoilApp {
    CZRecoilFrame *RECOIL_THISCALL CreateMainWnd() {
        return g_createMainWndResult;
    }

    std::int32_t RECOIL_THISCALL StartEngine(RecoilPtr32 hwnd) {
        ++g_startEngineCount;
        g_lastStartEngineHwnd = hwnd;
        return g_startEngineResult;
    }

    std::int32_t RECOIL_THISCALL PumpMessage() {
        ++g_runPumpMessageCount;
        return g_runPumpMessageResult;
    }

    void RECOIL_THISCALL ShutdownEngine() {
        ++g_shutdownEngineCount;
    }

    void RECOIL_THISCALL OnAppDeactivate() {
        ++g_runOnAppDeactivateCount;
    }

    std::int32_t RECOIL_THISCALL ExitInstance() {
        ++g_exitInstanceCount;
        return 77;
    }
};

RecoilFn32 AppStartMethodToFn(std::int32_t (RECOIL_THISCALL TestRecoilApp::*method)(RecoilPtr32)) {
    union MemberToFunction {
        std::int32_t (RECOIL_THISCALL TestRecoilApp::*member)(RecoilPtr32);
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 AppFrameMethodToFn(CZRecoilFrame *(RECOIL_THISCALL TestRecoilApp::*method)()) {
    union MemberToFunction {
        CZRecoilFrame *(RECOIL_THISCALL TestRecoilApp::*member)();
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 AppVoidMethodToFn(void (RECOIL_THISCALL TestRecoilApp::*method)()) {
    union MemberToFunction {
        void (RECOIL_THISCALL TestRecoilApp::*member)();
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 AppIntMethodToFn(std::int32_t (RECOIL_THISCALL TestRecoilApp::*method)()) {
    union MemberToFunction {
        std::int32_t (RECOIL_THISCALL TestRecoilApp::*member)();
        RecoilFn32 fn;
    };

    MemberToFunction thunk{};
    thunk.member = method;
    return thunk.fn;
}

RecoilFn32 g_testRecoilAppVtable[0x30];

void InitTestRecoilAppVtable() {
    g_testRecoilAppVtable[0x64 / 4] = AppIntMethodToFn(&TestRecoilApp::PumpMessage);
    g_testRecoilAppVtable[0x70 / 4] = AppIntMethodToFn(&TestRecoilApp::ExitInstance);
    g_testRecoilAppVtable[0xa8 / 4] = AppVoidMethodToFn(&TestRecoilApp::OnAppDeactivate);
    g_testRecoilAppVtable[0xac / 4] = AppStartMethodToFn(&TestRecoilApp::StartEngine);
    g_testRecoilAppVtable[0xb0 / 4] = AppVoidMethodToFn(&TestRecoilApp::ShutdownEngine);
    g_testRecoilAppVtable[0xb8 / 4] = AppFrameMethodToFn(&TestRecoilApp::CreateMainWnd);
}

void CleanupSingleQueuedItem(RecoilApp_StateQueue &queue) {
    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    auto *const chunkList =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(queue.m_chunkPtrList));
    auto *const chunk = reinterpret_cast<void *>(static_cast<std::uintptr_t>(chunkList[1]));
    ::operator delete(item);
    ::operator delete(chunk);
    ::operator delete(chunkList);
}

void CleanupQueuedItems(RecoilApp_StateQueue &queue) {
    if (queue.m_itemCount == 0 || queue.m_chunkPtrList == 0) {
        return;
    }

    const int itemCount = queue.m_itemCount;
    const RecoilPtr32 firstSlotValue = queue.m_writeBlock.m_cursor -
                                       static_cast<RecoilPtr32>(itemCount * 4);
    for (int i = 0; i < itemCount; ++i) {
        auto *const slot = reinterpret_cast<RecoilPtr32 *>(
            static_cast<std::uintptr_t>(firstSlotValue + static_cast<RecoilPtr32>(i * 4)));
        auto *const item = reinterpret_cast<RecoilApp_StateQueueItem *>(
            static_cast<std::uintptr_t>(*slot));
        ::operator delete(item);
    }

    auto *const chunkList =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(queue.m_chunkPtrList));
    auto *const chunk = reinterpret_cast<void *>(static_cast<std::uintptr_t>(chunkList[1]));
    ::operator delete(chunk);
    ::operator delete(chunkList);
    queue = RecoilApp_StateQueue{};
}

bool IsSingleExitCurrentQueueItem(RecoilApp_StateQueue &queue, int param) {
    if (queue.m_itemCount != 1 || queue.m_chunkPtrList == 0) {
        return false;
    }

    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    return item->m_kind == RecoilApp_StateQueueKind_ExitCurrent && item->m_param == param;
}

bool IsSinglePushStateQueueItem(RecoilApp_StateQueue &queue, RecoilPtr32 state, int param) {
    if (queue.m_itemCount != 1 || queue.m_chunkPtrList == 0) {
        return false;
    }

    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    return item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_PushState &&
           item->m_stateObj == state && item->m_param == param;
}

int RunOpenCampaignDialogHarness(bool throughMenuHandler) {
    ImportFunctionPatch patch{};
    if (!PatchImportByName("COMDLG32.dll", "GetOpenFileNameA",
                           reinterpret_cast<void *>(&FakeGetOpenFileNameSuccessA), patch)) {
        return 10;
    }

    InitTestRecoilAppVtable();
    g_startEngineCount = 0;
    g_shutdownEngineCount = 0;
    g_exitInstanceCount = 0;
    g_stateEnterCount = 0;
    g_stateExitCount = 0;
    g_lastStartEngineHwnd = 0;
    g_openFileNameCalls = 0;
    g_openFileNameStructOk = false;
    strcpy_s(g_openFileNameSelectedPath, "selected_campaign.gs");

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeTestZbdManager(sentinel);
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    HINSTANCE const oldInstance = g_RecoilApp_hInstance;
    const int oldMissionFlags = g_HudSensorTracker.missionFlags;
    RecoilApp const oldApp = g_RecoilApp;

    HWND const hwnd = CreateWindowExA(0, "STATIC", "recoil-open-test", WS_OVERLAPPEDWINDOW, 0, 0,
                                      100, 100, nullptr, nullptr, GetModuleHandleA(nullptr),
                                      nullptr);
    if (hwnd == nullptr) {
        RestoreImportPatch(patch);
        return 11;
    }

    g_zUtil_ZbdManager = &manager;
    g_RecoilApp_hInstance = GetModuleHandleA(nullptr);
    g_HudSensorTracker.missionFlags = 0;

    RecoilPtr32 frameWords[9]{};
    frameWords[8] = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(hwnd));

    TestAppState startupState{};
    startupState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    g_RecoilApp.m_pMainWnd = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(frameWords));
    g_RecoilApp.m_pendingState_0c4 =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&startupState));
    g_RecoilApp.m_currentStateIndex_0c8 = -1;
    g_RecoilApp.m_skipIntroFmv = throughMenuHandler ? 0 : 7;
    g_startEngineResult = 1;

    CZRecoilFrame frame{};
    frame.m_hWnd = hwnd;
    strcpy_s(frame.m_openZbdFilePath, "before.gs");

    if (throughMenuHandler) {
        frame.OnMenuOpenCampaign();
    } else {
        frame.OnOpenFileDialog();
    }

    const bool ok = g_openFileNameCalls == 1 && g_openFileNameStructOk &&
                    std::strcmp(frame.m_openZbdFilePath, g_openFileNameSelectedPath) == 0 &&
                    CStringEquals(g_HudSensorTracker.zbdPath, g_openFileNameSelectedPath) &&
                    g_RecoilApp.m_skipIntroFmv == 1 && g_startEngineCount == 1 &&
                    g_lastStartEngineHwnd ==
                        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(hwnd)) &&
                    g_stateEnterCount == 1 && g_RecoilApp.m_stateQueue_118.m_itemCount == 1 &&
                    manager.sectionHandlerCount == 2;

    if (g_RecoilApp.m_stateQueue_118.m_itemCount != 0) {
        CleanupSingleQueuedItem(g_RecoilApp.m_stateQueue_118);
    }
    ClearTestRegisteredHandlers(sentinel);
    g_HudSensorTracker.zbdPath.Empty();
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_RecoilApp = oldApp;
    g_RecoilApp_hInstance = oldInstance;
    g_zUtil_ZbdManager = oldZbdManager;
    DestroyWindow(hwnd);
    RestoreImportPatch(patch);

    return ok ? 0 : 1;
}

bool RunHelpDocsScenario(UINT_PTR findExecutableResult, const char *expectedMessageText,
                         bool expectShellExecute) {
    g_findExecutableResult = findExecutableResult;
    g_expectedHelpDocsMessageText = expectedMessageText;
    g_findExecutableCalls = 0;
    g_findExecutableArgsOk = false;
    g_shellExecuteCalls = 0;
    g_shellExecuteArgsOk = false;
    g_helpDocsMessageBoxCalls = 0;
    g_helpDocsMessageBoxArgsOk = false;

    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    HWND const hwnd = CreateWindowExA(0, "STATIC", "recoil-help-test", WS_OVERLAPPEDWINDOW, 0,
                                      0, 100, 100, nullptr, nullptr, GetModuleHandleA(nullptr),
                                      nullptr);
    if (hwnd == nullptr) {
        return false;
    }

    g_RecoilApp_hWndMain = hwnd;
    CZRecoilFrame frame{};
    frame.m_hWnd = hwnd;
    frame.OnMenuOpenHelpDocs();

    const bool ok = g_findExecutableCalls == 1 && g_findExecutableArgsOk &&
                    (expectShellExecute ? (g_shellExecuteCalls == 1 && g_shellExecuteArgsOk &&
                                           g_helpDocsMessageBoxCalls == 0)
                                        : (g_shellExecuteCalls == 0 &&
                                           g_helpDocsMessageBoxCalls == 1 &&
                                           g_helpDocsMessageBoxArgsOk));

    g_RecoilApp_hWndMain = oldMainHwnd;
    DestroyWindow(hwnd);
    return ok;
}

bool PatchAboutDialogMfcImports(ImportFunctionPatch &ctorPatch, ImportFunctionPatch &doModalPatch,
                                ImportFunctionPatch &dtorPatch) {
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogResourceCtorOrdinal,
                              reinterpret_cast<void *>(&FakeAboutCDialogCtor), ctorPatch)) {
        return false;
    }

    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDoModalOrdinal,
                              reinterpret_cast<void *>(&FakeAboutCDialogDoModal), doModalPatch)) {
        RestoreImportPatch(ctorPatch);
        return false;
    }

    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDtorOrdinal,
                              reinterpret_cast<void *>(&FakeAboutCDialogDtor), dtorPatch)) {
        RestoreImportPatch(doModalPatch);
        RestoreImportPatch(ctorPatch);
        return false;
    }

    return true;
}
} // namespace

extern "C" int crt_atexit_import_provider_smoke(void) {
    return std::atexit(AtexitProviderNoOp) == 0 ? 0 : 1;
}

extern "C" int recoil_app_get_message_map_smoke(void) {
    const MfcMsgMap *messageMap = g_RecoilApp.GetMessageMap();
    if (messageMap != &g_RecoilApp_MessageMap) {
        return 1;
    }

    if (messageMap->getBaseMap != 0x004428a0 || messageMap->entries != 0x004d0998) {
        return 2;
    }

    const zMfcMsgMapEntry &sentinel = g_RecoilApp_MessageEntries[0];
    return sentinel.nMessage == 0 && sentinel.nCode == 0 && sentinel.nID == 0 &&
                   sentinel.nLastID == 0 && sentinel.nSig == 0 && sentinel.pfn == 0
               ? 0
               : 3;
}

extern "C" int recoil_app_mfc_ole_module_constructor_smoke(void) {
    RecoilApp app{};
    app.m_stateQueue_118.m_ctorTag_00 = 0x11223344;
    app.m_pendingState_0c4 = 0x12345678;
    app.m_currentStateIndex_0c8 = 7;
    app.m_skipWait_0d0 = 9;
    for (RecoilPtr32 &state : app.m_stateStack_0d8) {
        state = 0x87654321;
    }

    RecoilApp *returned = app.MfcOleModuleConstructor();
    int result = 0;
    if (returned != &app || app.vftable != kRecoilApp_MfcOleModule_VtblAddress) {
        result = 1;
    }

    if (result == 0 && (app.m_pendingState_0c4 != 0 || app.m_currentStateIndex_0c8 != -1 ||
                        app.m_skipWait_0d0 != 0)) {
        result = 2;
    }

    const unsigned char expectedCtorByte =
        static_cast<unsigned char>((reinterpret_cast<std::uintptr_t>(&app) >> 24) & 0xff);
    const auto *const ctorTagBytes =
        reinterpret_cast<const unsigned char *>(&app.m_stateQueue_118.m_ctorTag_00);
    if (result == 0 && (ctorTagBytes[0] != expectedCtorByte || ctorTagBytes[1] != 0x33 ||
                        ctorTagBytes[2] != 0x22 || ctorTagBytes[3] != 0x11)) {
        result = 3;
    }

    const auto *const queueTail =
        reinterpret_cast<const unsigned char *>(&app.m_stateQueue_118.m_readBlock);
    for (std::size_t i = 0; result == 0 && i < 0x2c; ++i) {
        if (queueTail[i] != 0) {
            result = 4;
        }
    }

    for (RecoilPtr32 state : app.m_stateStack_0d8) {
        if (result == 0 && state != 0) {
            result = 5;
        }
    }

    app.MfcOleModuleDestructor();
    return result;
}

extern "C" int recoil_app_accessor_and_skip_wait_smoke(void) {
    RecoilApp app{};
    app.m_pMainWnd = 0x12345678;
    if (app.GetMainWnd() != 0x12345678) {
        return 1;
    }

    app.m_skipWait_0d0 = 7;
    if (app.TakeSkipWaitMessage() != 7 || app.m_skipWait_0d0 != 0) {
        return 2;
    }

    if (app.MarkSkipWaitMessage() != 0 || app.m_skipWait_0d0 != 1) {
        return 3;
    }

    return app.MarkSkipWaitMessage() == 1 && app.m_skipWait_0d0 == 1 ? 0 : 4;
}

extern "C" int recoil_app_activate_existing_instance_absent_smoke(void) {
    const char *const oldClassName = g_RecoilApp_WndClassNamePtr;
    g_RecoilApp_WndClassNamePtr = "RecoilNativeAbsentWindowClass";
    const std::int32_t result = g_RecoilApp.ActivateExistingInstance();
    g_RecoilApp_WndClassNamePtr = oldClassName;
    return result == 1 ? 0 : 1;
}

extern "C" int recoil_app_pre_translate_message_smoke(void) {
    static std::int32_t acceleration = 0;
    ZOPT_VIDEO_ACCELERATION = &acceleration;

    MSG message = {};
    message.message = WM_SYSKEYDOWN;
    if (g_RecoilApp.PreTranslateMessage(&message) != 0) {
        return 1;
    }

    acceleration = 1;
    if (g_RecoilApp.PreTranslateMessage(&message) != 1) {
        return 2;
    }

    message.message = WM_SYSKEYUP;
    if (g_RecoilApp.PreTranslateMessage(&message) != 1) {
        return 3;
    }

    message.message = WM_KEYDOWN;
    if (g_RecoilApp.PreTranslateMessage(&message) != 0) {
        return 4;
    }

    message.message = WM_SYSKEYUP + 1;
    return g_RecoilApp.PreTranslateMessage(&message) == 0 ? 0 : 5;
}

extern "C" int recoil_app_init_std_log_files_smoke(void) {
    g_RecoilApp_hWndMain = reinterpret_cast<HWND>(0x12345678);
    RecoilApp::InitStdLogFiles(nullptr);
    if (g_RecoilApp_hWndMain != nullptr) {
        return 1;
    }

    char tempPath[MAX_PATH];
    if (GetTempPathA(sizeof(tempPath), tempPath) == 0) {
        return 2;
    }

    char stem[MAX_PATH];
    if (GetTempFileNameA(tempPath, "rcl", 0, stem) == 0) {
        return 3;
    }
    DeleteFileA(stem);

    char errPath[MAX_PATH];
    char outPath[MAX_PATH];
    lstrcpyA(errPath, stem);
    lstrcatA(errPath, ".err");
    lstrcpyA(outPath, stem);
    lstrcatA(outPath, ".out");
    DeleteFileA(errPath);
    DeleteFileA(outPath);

    RecoilApp::InitStdLogFiles(stem);
    fflush(stderr);
    fflush(stdout);

    char errHeader[32];
    char outHeader[32];
    const bool errOk = ReadFilePrefix(errPath, errHeader, sizeof(errHeader));
    const bool outOk = ReadFilePrefix(outPath, outHeader, sizeof(outHeader));
    if (!errOk || !outOk) {
        return 4;
    }

    return errHeader[0] == 'F' && errHeader[5] == 's' && outHeader[0] == 'F' &&
                   outHeader[5] == 's'
               ? 0
               : 5;
}

extern "C" int recoil_app_get_current_state_smoke(void) {
    RecoilApp app{};
    app.m_stateStack_0d8[0] = 0x11111111;
    app.m_stateStack_0d8[15] = 0x22222222;

    app.m_currentStateIndex_0c8 = -1;
    if (app.GetCurrentState() != 0) {
        return 1;
    }

    app.m_currentStateIndex_0c8 = 0;
    if (app.GetCurrentState() != 0x11111111) {
        return 2;
    }

    app.m_currentStateIndex_0c8 = 15;
    if (app.GetCurrentState() != 0x22222222) {
        return 3;
    }

    app.m_currentStateIndex_0c8 = 16;
    return app.GetCurrentState() == 0 ? 0 : 4;
}

extern "C" int recoil_app_state_queue_block_init_from_cursor_smoke(void) {
    RecoilPtr32 chunkBaseSlot = 0x12340000;
    RecoilApp_StateQueueBlock block{};
    RecoilApp_StateQueueBlock *returned = block.InitFromCursor(
        0x12340800, static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&chunkBaseSlot)));

    if (returned != &block) {
        return 1;
    }

    return block.m_chunkBegin == 0x12340000 && block.m_chunkEnd == 0x12341000 &&
                   block.m_cursor == 0x12340800 &&
                   block.m_chunkPtrSlot ==
                       static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&chunkBaseSlot))
               ? 0
               : 2;
}

extern "C" int recoil_app_state_queue_grow_chunk_base_list_smoke(void) {
    RecoilApp_StateQueue queue{};
    auto *oldList = static_cast<RecoilPtr32 *>(::operator new(3 * sizeof(RecoilPtr32)));
    oldList[0] = 0x11111111;
    oldList[1] = 0x22222222;
    oldList[2] = 0x33333333;

    queue.m_readBlock.m_chunkPtrSlot =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldList[0]));
    queue.m_writeBlock.m_chunkPtrSlot =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldList[2]));
    queue.m_chunkPtrList = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(oldList));
    queue.m_chunkPtrCapacity = 3;

    const RecoilPtr32 centerSlotValue = queue.GrowAndCenterChunkBaseList(8);
    auto *const newList =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(queue.m_chunkPtrList));
    auto *const centerSlot =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(centerSlotValue));

    const bool ok = queue.m_chunkPtrCapacity == 8 && centerSlot == &newList[2] &&
                    centerSlot[0] == 0x11111111 && centerSlot[1] == 0x22222222 &&
                    centerSlot[2] == 0x33333333;
    ::operator delete(newList);

    return ok ? 0 : 1;
}

extern "C" int recoil_app_queue_switch_current_state_smoke(void) {
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    RecoilApp app{};
    TestAppState oldState{};
    TestAppState newState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    newState.vftable = oldState.vftable;
    app.m_currentStateIndex_0c8 = 0;
    app.m_stateStack_0d8[0] = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    const RecoilPtr32 returned = app.QueueSwitchCurrentState(&newState, 42);
    if (returned != app.m_stateStack_0d8[0] || g_stateExitCount != 1 || g_stateEnterCount != 1) {
        return 1;
    }

    RecoilApp_StateQueue &queue = app.m_stateQueue_118;
    if (queue.m_itemCount != 1 || queue.m_chunkPtrCapacity != 2) {
        return 2;
    }

    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    const bool itemOk =
        item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
        item->m_stateObj == static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&newState)) &&
        item->m_param == 42;

    ::operator delete(item);
    auto *const chunkList =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(queue.m_chunkPtrList));
    auto *const chunk = reinterpret_cast<void *>(static_cast<std::uintptr_t>(chunkList[1]));
    ::operator delete(chunk);
    ::operator delete(chunkList);

    return itemOk ? 0 : 3;
}

extern "C" int hud_sensor_queue_mission_fmv_state_for_mission_id_smoke(void) {
    InitTestRecoilAppVtable();
    const RecoilApp oldApp = g_RecoilApp;
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    TestAppState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));
    g_RecoilApp.m_missionFmvState_1d8.base.vftable = oldState.vftable;
    g_RecoilApp.m_missionFmvState_1d8.m_skipMissionFmv = 7;

    HudSensorTracker tracker{};
    const int returned = tracker.QueueMissionFmvStateForMissionId(14);
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;

    int result = 0;
    if (returned != 1 || g_RecoilApp.m_missionFmvState_1d8.m_missionId != 14 ||
        g_RecoilApp.m_missionFmvState_1d8.m_skipMissionFmv != 0 ||
        g_stateExitCount != 1 || g_stateEnterCount != 1) {
        result = 1;
    } else if (queue.m_itemCount != 1 || queue.m_chunkPtrCapacity != 2) {
        result = 2;
    } else {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot = reinterpret_cast<RecoilPtr32 *>(
            static_cast<std::uintptr_t>(slotValue));
        auto *const item = reinterpret_cast<RecoilApp_StateQueueItem *>(
            static_cast<std::uintptr_t>(*slot));
        if (item->m_type != 0 || item->m_kind != RecoilApp_StateQueueKind_SwitchCurrent ||
            item->m_stateObj != static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                                    &g_RecoilApp.m_missionFmvState_1d8.base)) ||
            item->m_param != 0) {
            result = 3;
        }
    }

    if (queue.m_itemCount == 1) {
        CleanupSingleQueuedItem(queue);
    }
    g_RecoilApp = oldApp;
    return result;
}

extern "C" int hud_sensor_save_and_queue_mission_state_smoke(void) {
    InitTestRecoilAppVtable();
    const RecoilApp oldApp = g_RecoilApp;
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const int oldHudCounterValue = g_Player_HudCounterValue;
    const int oldQuitAfterCredits = g_RecoilApp_QuitAfterCredits;

    HudSensorTracker finalTracker{};
    finalTracker.finalMissionFlag = 1;
    g_RecoilApp_QuitAfterCredits = 0;
    finalTracker.SaveAndQueueMissionState();
    const bool finalOk = g_RecoilApp_QuitAfterCredits == 1;

    TestAppState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));
    g_RecoilApp.m_missionFmvState_1d8.base.vftable = oldState.vftable;

    zUtil_SaveGameState saveState{};
    zUtil_PlayerStateStorage playerState{};
    PlayerModalState modalState{};
    PlayerMasterModalData modalData{};
    zClass_NodePartial cameraNode{};
    zClass_CameraDataPartial cameraData{};

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState.altWeaponBanks[bankIndex];
        bank.selectedSide = bankIndex & 1;
        bank.controllerA.flags = 4;
        bank.controllerA.ammoOrCharge = static_cast<float>(bankIndex);
        bank.controllerA.weaponBankIndex = bankIndex;
        bank.controllerA.weaponSideIndex = 0;
        bank.controllerB.flags = 0;
        bank.controllerB.ammoOrCharge = static_cast<float>(bankIndex + 20);
        bank.controllerB.weaponBankIndex = bankIndex;
        bank.controllerB.weaponSideIndex = 1;
    }

    playerState.activeAltGunController = &playerState.altWeaponBanks[2].controllerA;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[3].controllerB;
    playerState.nanitePanelLevel = 77;
    playerState.amphibUnlocked = 1;
    playerState.hoverUnlocked = 1;
    playerState.subUnlocked = 1;
    playerState.aiMode = 4;
    playerState.motionInput = 5;
    playerState.autoTurnSign = -1;
    playerState.bankInput = 6;
    modalData.masterType = 9;
    modalState.masterModalData = &modalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.worldTarget = {1.0f, 2.0f, 3.0f};
    cameraData.posOffset = {4.0f, 5.0f, 6.0f};

    g_LocalPlayerSaveState = &saveState;
    g_MainCamera = &cameraNode;
    g_PlayerStatusMeterRatio = 0.5f;
    g_Player_HudCounterValue = 123;
    g_RecoilApp_QuitAfterCredits = 0;

    HudSensorTracker tracker{};
    tracker.missionId = 5;
    tracker.SaveAndQueueMissionState();

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool normalOk = tracker.hasPendingPlayerSave == 1 &&
                    tracker.pendingPlayerSave.savedNanitePanelLevel == 77 &&
                    tracker.pendingPlayerSave.playerSaveData.size ==
                        sizeof(PlayerMissionSaveData) &&
                    tracker.pendingPlayerSave.playerSaveData.altWeaponBankIndex == 2 &&
                    tracker.pendingPlayerSave.playerSaveData.primaryWeaponSideIndex == 1 &&
                    g_RecoilApp.m_missionFmvState_1d8.m_missionId == 6 &&
                    g_RecoilApp.m_missionFmvState_1d8.m_skipMissionFmv == 0 &&
                    queue.m_itemCount == 1;
    if (normalOk) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot = reinterpret_cast<RecoilPtr32 *>(
            static_cast<std::uintptr_t>(slotValue));
        auto *const item = reinterpret_cast<RecoilApp_StateQueueItem *>(
            static_cast<std::uintptr_t>(*slot));
        normalOk = item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
                   item->m_stateObj == static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                                           &g_RecoilApp.m_missionFmvState_1d8.base)) &&
                   item->m_param == 0;
    }

    if (queue.m_itemCount == 1) {
        CleanupSingleQueuedItem(queue);
    }
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    g_Player_HudCounterValue = oldHudCounterValue;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_MainCamera = oldMainCamera;
    g_LocalPlayerSaveState = oldLocalSaveState;
    g_RecoilApp = oldApp;
    return finalOk && normalOk ? 0 : 1;
}

extern "C" int recoil_app_queue_push_state_smoke(void) {
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    RecoilApp app{};
    TestAppState oldState{};
    TestAppState newState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    newState.vftable = oldState.vftable;
    app.m_currentStateIndex_0c8 = 0;
    app.m_stateStack_0d8[0] = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    const RecoilPtr32 returned = app.QueuePushState(&newState, 23);
    if (returned != app.m_stateStack_0d8[0] || g_stateExitCount != 0 || g_stateEnterCount != 1) {
        return 1;
    }

    RecoilApp_StateQueue &queue = app.m_stateQueue_118;
    if (queue.m_itemCount != 1 || queue.m_chunkPtrCapacity != 2) {
        return 2;
    }

    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    const bool itemOk =
        item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_PushState &&
        item->m_stateObj == static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&newState)) &&
        item->m_param == 23;
    CleanupSingleQueuedItem(queue);

    return itemOk ? 0 : 3;
}

extern "C" int hud_ui_callback_queue_cheat_code_state_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    const RecoilStateCheatCode oldCheatState = g_RecoilStateCheatCode;
    g_RecoilApp = RecoilApp{};
    g_RecoilStateCheatCode = RecoilStateCheatCode{};
    g_RecoilStateCheatCode.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    const int returned = HudUiCallback::QueueCheatCodeState();
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    int result = 0;
    if (returned != 1 || g_stateEnterCount != 1 || g_stateExitCount != 0) {
        result = 1;
    } else if (queue.m_itemCount != 1 || queue.m_chunkPtrCapacity != 2) {
        result = 2;
    } else {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        if (item->m_type != 0 || item->m_kind != RecoilApp_StateQueueKind_PushState ||
            item->m_stateObj !=
                static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                    &g_RecoilStateCheatCode)) ||
            item->m_param != 0) {
            result = 3;
        }
    }

    if (queue.m_itemCount == 1) {
        CleanupSingleQueuedItem(queue);
    }
    g_RecoilStateCheatCode = oldCheatState;
    g_RecoilApp = oldApp;
    return result;
}

extern "C" int recoil_state_controls_queue_enter_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    const RecoilStateControls oldControlsState = g_RecoilStateControls;
    g_RecoilApp = RecoilApp{};
    g_RecoilStateControls = RecoilStateControls{};
    g_RecoilStateControls.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    RecoilStateControls::QueueEnter();
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    int result = 0;
    if (g_stateEnterCount != 1 || g_stateExitCount != 0) {
        result = 1;
    } else if (queue.m_itemCount != 1 || queue.m_chunkPtrCapacity != 2) {
        result = 2;
    } else {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        if (item->m_type != 0 || item->m_kind != RecoilApp_StateQueueKind_PushState ||
            item->m_stateObj !=
                static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                    &g_RecoilStateControls)) ||
            item->m_param != 0) {
            result = 3;
        }
    }

    if (queue.m_itemCount == 1) {
        CleanupSingleQueuedItem(queue);
    }
    g_RecoilStateControls = oldControlsState;
    g_RecoilApp = oldApp;
    return result;
}

extern "C" int hud_ui_options_panel_overlay_owner_queue_enter_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    const HudUiOptionsPanelOverlayOwner oldOptionsState = g_HudUiOptionsPanelOverlayOwner;
    g_RecoilApp = RecoilApp{};
    g_HudUiOptionsPanelOverlayOwner = HudUiOptionsPanelOverlayOwner{};
    g_HudUiOptionsPanelOverlayOwner.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    HudUiOptionsPanelOverlayOwner::QueueEnter();
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    int result = 0;
    if (g_stateEnterCount != 1 || g_stateExitCount != 0) {
        result = 1;
    } else if (queue.m_itemCount != 1 || queue.m_chunkPtrCapacity != 2) {
        result = 2;
    } else {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        if (item->m_type != 0 || item->m_kind != RecoilApp_StateQueueKind_PushState ||
            item->m_stateObj !=
                static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                    &g_HudUiOptionsPanelOverlayOwner)) ||
            item->m_param != 0) {
            result = 3;
        }
    }

    if (queue.m_itemCount == 1) {
        CleanupSingleQueuedItem(queue);
    }
    g_HudUiOptionsPanelOverlayOwner = oldOptionsState;
    g_RecoilApp = oldApp;
    return result;
}

extern "C" int recoil_app_queue_exit_current_state_smoke(void) {
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    RecoilApp app{};
    TestAppState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    app.m_currentStateIndex_0c8 = 0;
    app.m_stateStack_0d8[0] = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));

    const RecoilPtr32 returned = app.QueueExitCurrentState(17);
    if (returned != app.m_stateStack_0d8[0] || g_stateExitCount != 1 || g_stateEnterCount != 0) {
        return 1;
    }

    RecoilApp_StateQueue &queue = app.m_stateQueue_118;
    if (queue.m_itemCount != 1 || queue.m_chunkPtrCapacity != 2) {
        return 2;
    }

    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    auto *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    auto *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    const bool itemOk = item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
                        item->m_stateObj == 0 && item->m_param == 17;
    CleanupSingleQueuedItem(queue);

    return itemOk ? 0 : 3;
}

extern "C" int recoil_app_run_current_state_quit_smoke(void) {
    InitTestRecoilAppVtable();

    ImportFunctionPatch peekPatch{};
    ImportFunctionPatch postQuitPatch{};
    ImportFunctionPatch waitPatch{};
    ImportFunctionPatch priorityPatch{};
    CodeFunctionPatch receivePatch{};

    if (!PatchImportByName("USER32.dll", "PeekMessageA",
                           reinterpret_cast<void *>(&FakeRunPeekMessageA), peekPatch)) {
        return 1;
    }
    if (!PatchImportByName("USER32.dll", "PostQuitMessage",
                           reinterpret_cast<void *>(&FakeRunPostQuitMessage), postQuitPatch)) {
        RestoreImportPatch(peekPatch);
        return 2;
    }
    if (!PatchImportByName("USER32.dll", "WaitMessage",
                           reinterpret_cast<void *>(&FakeRunWaitMessage), waitPatch)) {
        RestoreImportPatch(postQuitPatch);
        RestoreImportPatch(peekPatch);
        return 3;
    }
    if (!PatchImportByName("KERNEL32.dll", "SetThreadPriority",
                           reinterpret_cast<void *>(&FakeRunSetThreadPriority), priorityPatch)) {
        RestoreImportPatch(waitPatch);
        RestoreImportPatch(postQuitPatch);
        RestoreImportPatch(peekPatch);
        return 4;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zNetworkDPlay::ReceivePendingMessages),
                           reinterpret_cast<void *>(&FakeRunReceivePendingMessages),
                           receivePatch)) {
        RestoreImportPatch(priorityPatch);
        RestoreImportPatch(waitPatch);
        RestoreImportPatch(postQuitPatch);
        RestoreImportPatch(peekPatch);
        return 5;
    }

    g_stateUpdateShouldQuitCount = 0;
    g_stateUpdateShouldQuitResult = 1;
    g_stateDeactivateCount = 0;
    g_runPumpMessageCount = 0;
    g_runPumpMessageResult = 0;
    g_runOnAppDeactivateCount = 0;
    g_runPeekMessageCount = 0;
    g_runPostQuitCount = 0;
    g_runPostQuitCode = -1;
    g_runWaitMessageCount = 0;
    g_runSetThreadPriorityCount = 0;
    g_runSetThreadPriorityValue = 0;
    g_runReceivePendingMessagesCount = 0;
    g_runReceivePendingMessagesBudget = 0;
    g_runQuitPosted = 0;
    g_runPeekMessageTrueAfterReceive = 0;
    g_exitInstanceCount = 0;

    TestAppState state{};
    state.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));

    TestRecoilApp app{};
    app.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    app.m_currentStateIndex_0c8 = 0;
    app.m_skipWait_0d0 = 1;
    app.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&state));

    const int result = app.Run();

    RestoreFunctionPatch(receivePatch);
    RestoreImportPatch(priorityPatch);
    RestoreImportPatch(waitPatch);
    RestoreImportPatch(postQuitPatch);
    RestoreImportPatch(peekPatch);

    const bool ok = result == 77 && g_runSetThreadPriorityCount == 1 &&
                    g_runSetThreadPriorityValue == THREAD_PRIORITY_HIGHEST &&
                    g_runReceivePendingMessagesCount == 1 &&
                    g_runReceivePendingMessagesBudget == -1 &&
                    g_stateUpdateShouldQuitCount == 1 && g_runOnAppDeactivateCount == 1 &&
                    g_runPostQuitCount == 1 && g_runPostQuitCode == 0 &&
                    g_runPumpMessageCount == 1 && g_exitInstanceCount == 1 &&
                    g_runWaitMessageCount == 0;
    return ok ? 0 : 6;
}

extern "C" int recoil_app_run_queue_transitions_smoke(void) {
    InitTestRecoilAppVtable();

    RunPatchSet patches{};
    const int patchResult = InstallRunPatches(patches);
    if (patchResult != 0) {
        return patchResult;
    }

    int result = 0;

    TestAppState switchOldState{};
    TestAppState switchNewState{};
    switchOldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    switchNewState.vftable = switchOldState.vftable;

    TestRecoilApp switchApp{};
    switchApp.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    switchApp.m_currentStateIndex_0c8 = 0;
    switchApp.m_skipWait_0d0 = 1;
    switchApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&switchOldState));
    switchApp.QueueSwitchCurrentState(&switchNewState, 42);

    ResetRunHarnessCounters();
    g_stateTryBecomeCurrentResult = 1;
    g_runPeekMessageTrueAfterReceive = 1;
    const int switchRunResult = switchApp.Run();
    if (switchRunResult != 77) {
        result = 10;
    } else if (g_stateDeactivateCount != 1) {
        result = 11;
    } else if (g_stateTryBecomeCurrentCount != 1) {
        result = 12;
    } else if (g_stateSuspendCount != 0 || g_stateResumeCount != 0) {
        result = 13;
    } else if (switchApp.m_currentStateIndex_0c8 != 0) {
        result = 14;
    } else if (switchApp.m_stateStack_0d8[0] !=
               static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&switchNewState))) {
        result = 15;
    } else if (switchApp.m_stateQueue_118.m_itemCount != 0) {
        result = 16;
    } else if (g_runPumpMessageCount != 1 || g_exitInstanceCount != 1) {
        result = 17;
    } else if (g_runWaitMessageCount != 0) {
        result = 18;
    }

    if (result == 0) {
        TestAppState pushOldState{};
        TestAppState pushNewState{};
        pushOldState.vftable =
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
        pushNewState.vftable = pushOldState.vftable;

        TestRecoilApp pushApp{};
        pushApp.vftable =
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
        pushApp.m_currentStateIndex_0c8 = 0;
        pushApp.m_skipWait_0d0 = 1;
        pushApp.m_stateStack_0d8[0] =
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&pushOldState));
        pushApp.QueuePushState(&pushNewState, 23);

        ResetRunHarnessCounters();
        g_stateTryBecomeCurrentResult = 1;
        g_runPeekMessageTrueAfterReceive = 1;
        const int pushRunResult = pushApp.Run();
        if (pushRunResult != 77 || g_stateSuspendCount != 1 || g_stateSuspendParam != 23 ||
            g_stateTryBecomeCurrentCount != 1 || g_stateDeactivateCount != 0 ||
            g_stateResumeCount != 0 || pushApp.m_currentStateIndex_0c8 != 1 ||
            pushApp.m_stateStack_0d8[1] !=
                static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&pushNewState)) ||
            pushApp.m_stateQueue_118.m_itemCount != 0 || g_runPumpMessageCount != 1 ||
            g_exitInstanceCount != 1 || g_runWaitMessageCount != 0) {
            result = 20;
        }
    }

    if (result == 0) {
        TestAppState resumeState{};
        TestAppState exitState{};
        resumeState.vftable =
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
        exitState.vftable = resumeState.vftable;

        TestRecoilApp exitApp{};
        exitApp.vftable =
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
        exitApp.m_currentStateIndex_0c8 = 1;
        exitApp.m_skipWait_0d0 = 1;
        exitApp.m_stateStack_0d8[0] =
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&resumeState));
        exitApp.m_stateStack_0d8[1] =
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&exitState));
        exitApp.QueueExitCurrentState(17);

        ResetRunHarnessCounters();
        g_runPeekMessageTrueAfterReceive = 1;
        const int exitRunResult = exitApp.Run();
        if (exitRunResult != 77 || g_stateDeactivateCount != 1 || g_stateResumeCount != 1 ||
            g_stateResumeParam != 17 || g_stateTryBecomeCurrentCount != 0 ||
            g_stateSuspendCount != 0 || exitApp.m_currentStateIndex_0c8 != 0 ||
            exitApp.m_stateStack_0d8[0] !=
                static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&resumeState)) ||
            exitApp.m_stateStack_0d8[1] != 0 || exitApp.m_stateQueue_118.m_itemCount != 0 ||
            g_runPumpMessageCount != 1 || g_exitInstanceCount != 1 ||
            g_runWaitMessageCount != 0) {
            result = 30;
        }
    }

    RestoreRunPatches(patches);
    return result;
}

extern "C" int recoil_app_start_engine_and_queue_startup_state_smoke(void) {
    InitTestRecoilAppVtable();
    g_startEngineCount = 0;
    g_shutdownEngineCount = 0;
    g_exitInstanceCount = 0;
    g_stateEnterCount = 0;
    g_stateExitCount = 0;
    g_lastStartEngineHwnd = 0;

    RecoilPtr32 frameWords[9]{};
    frameWords[8] = 0x44556677;

    TestRecoilApp app{};
    TestAppState startupState{};
    startupState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    app.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    app.m_pMainWnd = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(frameWords));
    app.m_pendingState_0c4 =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&startupState));
    app.m_currentStateIndex_0c8 = -1;
    app.m_skipWait_0d0 = 0;
    app.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
    g_startEngineResult = 1;

    if (app.StartEngineAndQueueStartupState() != 1 || g_startEngineCount != 1 ||
        g_lastStartEngineHwnd != 0x44556677 || g_shutdownEngineCount != 0 ||
        g_exitInstanceCount != 0 || g_stateEnterCount != 1) {
        return 1;
    }

    if (app.m_skipWait_0d0 != 1 ||
        app.m_missionShutdownMode != RECOILAPP_MISSION_SHUTDOWN_ON_EXIT ||
        app.m_stateQueue_118.m_itemCount != 1) {
        return 2;
    }
    CleanupSingleQueuedItem(app.m_stateQueue_118);

    TestRecoilApp failApp{};
    failApp.vftable = app.vftable;
    failApp.m_pMainWnd = app.m_pMainWnd;
    g_startEngineResult = 0;
    if (failApp.StartEngineAndQueueStartupState() != 77 || g_startEngineCount != 2 ||
        g_shutdownEngineCount != 1 || g_exitInstanceCount != 1) {
        return 3;
    }

    return failApp.m_stateQueue_118.m_itemCount == 0 ? 0 : 4;
}

extern "C" int recoil_app_init_main_window_smoke(void) {
    InitTestRecoilAppVtable();

    HINSTANCE instance = GetModuleHandleA(nullptr);
    if (AfxWinInit(instance, nullptr, GetCommandLineA(), SW_HIDE) == 0) {
        return 1;
    }

    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil-init-main-window-smoke",
                                WS_OVERLAPPEDWINDOW, 0, 0, 64, 64, nullptr, nullptr,
                                instance, nullptr);
    if (hwnd == nullptr) {
        return 2;
    }

    CZRecoilFrame frame{};
    frame.m_hWnd = hwnd;
    g_createMainWndResult = &frame;

    TestRecoilApp app{};
    app.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    const int result = app.InitMainWindow();
    const bool ok = result == 1 && app.m_pMainWnd == reinterpret_cast<std::uintptr_t>(&frame) &&
                    frame.m_app == reinterpret_cast<std::uintptr_t>(&app) &&
                    IsWindowVisible(hwnd) != 0;

    DestroyWindow(hwnd);
    g_createMainWndResult = nullptr;
    return ok ? 0 : 3;
}

extern "C" int recoil_app_load_zbd_and_start_engine_smoke(void) {
    InitTestRecoilAppVtable();
    g_startEngineCount = 0;
    g_shutdownEngineCount = 0;
    g_exitInstanceCount = 0;
    g_stateEnterCount = 0;
    g_stateExitCount = 0;
    g_lastStartEngineHwnd = 0;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeTestZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;

    const int oldMissionFlags = g_HudSensorTracker.missionFlags;
    g_HudSensorTracker.missionFlags = 0;

    RecoilPtr32 frameWords[9]{};
    frameWords[8] = 0x13572468;

    TestRecoilApp app{};
    TestAppState startupState{};
    startupState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    app.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    app.m_pMainWnd = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(frameWords));
    app.m_pendingState_0c4 =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&startupState));
    app.m_currentStateIndex_0c8 = -1;
    g_startEngineResult = 1;

    const int result = app.LoadZbdAndStartEngine();
    const bool ok = result == 1 && g_startEngineCount == 1 &&
                    g_lastStartEngineHwnd == 0x13572468 && g_stateEnterCount == 1 &&
                    app.m_skipWait_0d0 == 1 &&
                    app.m_missionShutdownMode == RECOILAPP_MISSION_SHUTDOWN_ON_EXIT &&
                    app.m_stateQueue_118.m_itemCount == 1 &&
                    manager.sectionHandlerCount == 2 &&
                    std::strcmp(sentinel.next->sectionHandler.sectionName, "Mission") == 0 &&
                    std::strcmp(sentinel.prev->sectionHandler.sectionName, "MissionLate") == 0;

    CleanupSingleQueuedItem(app.m_stateQueue_118);
    ClearTestRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = nullptr;
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    return ok ? 0 : 1;
}

extern "C" int czrecoil_frame_on_menu_start_single_player_smoke(void) {
    InitTestRecoilAppVtable();
    g_startEngineCount = 0;
    g_shutdownEngineCount = 0;
    g_exitInstanceCount = 0;
    g_stateEnterCount = 0;
    g_stateExitCount = 0;
    g_lastStartEngineHwnd = 0;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeTestZbdManager(sentinel);
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    g_zUtil_ZbdManager = &manager;

    const int oldMissionFlags = g_HudSensorTracker.missionFlags;
    g_HudSensorTracker.missionFlags = 0;
    RecoilApp const oldApp = g_RecoilApp;

    RecoilPtr32 frameWords[9]{};
    frameWords[8] = 0x22446688;

    TestAppState startupState{};
    startupState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    g_RecoilApp.m_pMainWnd = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(frameWords));
    g_RecoilApp.m_pendingState_0c4 =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&startupState));
    g_RecoilApp.m_currentStateIndex_0c8 = -1;
    g_RecoilApp.m_skipIntroFmv = 7;
    g_RecoilApp.m_missionFmvState_1d8.m_skipMissionFmv = 9;
    g_startEngineResult = 1;

    CZRecoilFrame frame{};
    frame.OnMenuStartSinglePlayer();

    const bool ok = g_RecoilApp.m_skipIntroFmv == 0 &&
                    g_RecoilApp.m_missionFmvState_1d8.m_skipMissionFmv == 0 &&
                    g_startEngineCount == 1 && g_lastStartEngineHwnd == 0x22446688 &&
                    g_stateEnterCount == 1 && g_RecoilApp.m_stateQueue_118.m_itemCount == 1 &&
                    manager.sectionHandlerCount == 2;

    CleanupSingleQueuedItem(g_RecoilApp.m_stateQueue_118);
    ClearTestRegisteredHandlers(sentinel);
    g_RecoilApp = oldApp;
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_zUtil_ZbdManager = oldZbdManager;
    return ok ? 0 : 1;
}

extern "C" int czrecoil_frame_on_open_file_dialog_smoke(void) {
    return RunOpenCampaignDialogHarness(false);
}

extern "C" int czrecoil_frame_on_menu_open_campaign_smoke(void) {
    return RunOpenCampaignDialogHarness(true);
}

extern "C" int czrecoil_frame_on_menu_open_help_docs_smoke(void) {
    CodeFunctionPatch zlocPatch{};
    CodeFunctionPatch messageBoxPatch{};
    ImportFunctionPatch findExecutablePatch{};
    ImportFunctionPatch shellExecutePatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zLoc::GetMessageString),
                           reinterpret_cast<void *>(&FakeHelpDocsGetMessageString), zlocPatch)) {
        return 10;
    }

    if (!PatchImportByName("SHELL32.dll", "FindExecutableA",
                           reinterpret_cast<void *>(&FakeHelpDocsFindExecutableA),
                           findExecutablePatch)) {
        RestoreFunctionPatch(zlocPatch);
        return 11;
    }

    if (!PatchImportByName("SHELL32.dll", "ShellExecuteA",
                           reinterpret_cast<void *>(&FakeHelpDocsShellExecuteA),
                           shellExecutePatch)) {
        RestoreImportPatch(findExecutablePatch);
        RestoreFunctionPatch(zlocPatch);
        return 12;
    }

    HMODULE user32 = GetModuleHandleA("USER32.dll");
    void *const messageBoxProc =
        user32 != nullptr ? GetProcAddress(user32, "MessageBoxA") : nullptr;
    if (messageBoxProc == nullptr ||
        !PatchFunctionJump(messageBoxProc, reinterpret_cast<void *>(&FakeHelpDocsMessageBoxA),
                           messageBoxPatch)) {
        RestoreImportPatch(shellExecutePatch);
        RestoreImportPatch(findExecutablePatch);
        RestoreFunctionPatch(zlocPatch);
        return 13;
    }

    const bool successOk = RunHelpDocsScenario(33, nullptr, true);
    const bool defaultFailureFallsThroughOk = RunHelpDocsScenario(1, nullptr, true);
    const bool noAssociationOk =
        RunHelpDocsScenario(0, FakeHelpDocsGetMessageString(0x20), false);
    const bool fileNotFoundOk =
        RunHelpDocsScenario(2, FakeHelpDocsGetMessageString(0x22), false);
    const bool incompleteAssociationOk =
        RunHelpDocsScenario(11, FakeHelpDocsGetMessageString(0x24), false);
    const bool noDdeAssociationOk =
        RunHelpDocsScenario(31, FakeHelpDocsGetMessageString(0x21), false);

    RestoreFunctionPatch(messageBoxPatch);
    RestoreImportPatch(shellExecutePatch);
    RestoreImportPatch(findExecutablePatch);
    RestoreFunctionPatch(zlocPatch);

    return successOk && defaultFailureFallsThroughOk && noAssociationOk && fileNotFoundOk &&
                   incompleteAssociationOk && noDdeAssociationOk
               ? 0
               : 1;
}

extern "C" int cabout_dlg_constructor_smoke(void) {
    ImportFunctionPatch ctorPatch{};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogResourceCtorOrdinal,
                              reinterpret_cast<void *>(&FakeAboutCDialogCtor), ctorPatch)) {
        return 10;
    }

    ResetAboutDialogProbe();
    alignas(CAboutDlg) unsigned char storage[sizeof(CAboutDlg)]{};
    CAboutDlg *const dlg = new (storage) CAboutDlg();

    const bool ok = g_aboutDialogCtorCalls == 1 && g_aboutDialogCtorArgsOk &&
                    g_aboutDialogThis == static_cast<CDialog *>(dlg) &&
                    *reinterpret_cast<void **>(dlg) != nullptr;

    RestoreImportPatch(ctorPatch);
    return ok ? 0 : 1;
}

extern "C" int czrecoil_frame_on_menu_about_smoke(void) {
    ImportFunctionPatch ctorPatch{};
    ImportFunctionPatch doModalPatch{};
    ImportFunctionPatch dtorPatch{};
    if (!PatchAboutDialogMfcImports(ctorPatch, doModalPatch, dtorPatch)) {
        return 10;
    }

    ResetAboutDialogProbe();
    CZRecoilFrame frame{};
    frame.OnMenuAbout();

    const bool ok = g_aboutDialogCtorCalls == 1 && g_aboutDialogDoModalCalls == 1 &&
                    g_aboutDialogDtorCalls == 1 && g_aboutDialogCtorArgsOk &&
                    g_aboutDialogFlowOk;

    RestoreImportPatch(dtorPatch);
    RestoreImportPatch(doModalPatch);
    RestoreImportPatch(ctorPatch);
    return ok ? 0 : 1;
}

extern "C" int recoil_app_fatal_error_and_exit_smoke(void) {
    CodeFunctionPatch zlocPatch{};
    CodeFunctionPatch briefingPatch{};
    CodeFunctionPatch flipPatch{};
    CodeFunctionPatch sndPatch{};
    CodeFunctionPatch networkPatch{};
    CodeFunctionPatch videoPatch{};
    CodeFunctionPatch exitPatch{};
    ImportFunctionPatch sleepPatch{};
    ImportFunctionPatch beepPatch{};
    ImportFunctionPatch messageBoxPatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zLoc::GetMessageString),
                           reinterpret_cast<void *>(&FakeHelpDocsGetMessageString), zlocPatch)) {
        return 10;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&Briefing::StopAndShutdownThread),
                           reinterpret_cast<void *>(&FakeFatalBriefingStop), briefingPatch)) {
        RestoreFunctionPatch(zlocPatch);
        return 11;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo_dd::FlipToGDIIfAttached),
                           reinterpret_cast<void *>(&FakeFatalFlipToGDI), flipPatch)) {
        RestoreFunctionPatch(briefingPatch);
        RestoreFunctionPatch(zlocPatch);
        return 12;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zSndSystem::Shutdown),
                           reinterpret_cast<void *>(&FakeFatalSndShutdown), sndPatch)) {
        RestoreFunctionPatch(flipPatch);
        RestoreFunctionPatch(briefingPatch);
        RestoreFunctionPatch(zlocPatch);
        return 13;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zNetwork::ShutdownSessionRuntime),
                           reinterpret_cast<void *>(&FakeFatalNetworkShutdown), networkPatch)) {
        RestoreFunctionPatch(sndPatch);
        RestoreFunctionPatch(flipPatch);
        RestoreFunctionPatch(briefingPatch);
        RestoreFunctionPatch(zlocPatch);
        return 14;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::ShutdownVideoSystem),
                           reinterpret_cast<void *>(&FakeFatalVideoShutdown), videoPatch)) {
        RestoreFunctionPatch(networkPatch);
        RestoreFunctionPatch(sndPatch);
        RestoreFunctionPatch(flipPatch);
        RestoreFunctionPatch(briefingPatch);
        RestoreFunctionPatch(zlocPatch);
        return 15;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zSys::ExitProcessWithCleanup),
                           reinterpret_cast<void *>(&FakeFatalExitProcessWithCleanup), exitPatch)) {
        RestoreFunctionPatch(videoPatch);
        RestoreFunctionPatch(networkPatch);
        RestoreFunctionPatch(sndPatch);
        RestoreFunctionPatch(flipPatch);
        RestoreFunctionPatch(briefingPatch);
        RestoreFunctionPatch(zlocPatch);
        return 16;
    }
    if (!PatchImportByName("KERNEL32.dll", "Sleep", reinterpret_cast<void *>(&FakeFatalSleep),
                           sleepPatch)) {
        RestoreFunctionPatch(exitPatch);
        RestoreFunctionPatch(videoPatch);
        RestoreFunctionPatch(networkPatch);
        RestoreFunctionPatch(sndPatch);
        RestoreFunctionPatch(flipPatch);
        RestoreFunctionPatch(briefingPatch);
        RestoreFunctionPatch(zlocPatch);
        return 17;
    }
    if (!PatchImportByName("USER32.dll", "MessageBeep",
                           reinterpret_cast<void *>(&FakeFatalMessageBeep), beepPatch)) {
        RestoreImportPatch(sleepPatch);
        RestoreFunctionPatch(exitPatch);
        RestoreFunctionPatch(videoPatch);
        RestoreFunctionPatch(networkPatch);
        RestoreFunctionPatch(sndPatch);
        RestoreFunctionPatch(flipPatch);
        RestoreFunctionPatch(briefingPatch);
        RestoreFunctionPatch(zlocPatch);
        return 18;
    }
    if (!PatchImportByName("USER32.dll", "MessageBoxA",
                           reinterpret_cast<void *>(&FakeFatalMessageBoxA), messageBoxPatch)) {
        RestoreImportPatch(beepPatch);
        RestoreImportPatch(sleepPatch);
        RestoreFunctionPatch(exitPatch);
        RestoreFunctionPatch(videoPatch);
        RestoreFunctionPatch(networkPatch);
        RestoreFunctionPatch(sndPatch);
        RestoreFunctionPatch(flipPatch);
        RestoreFunctionPatch(briefingPatch);
        RestoreFunctionPatch(zlocPatch);
        return 19;
    }

    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    g_RecoilApp_hWndMain = reinterpret_cast<HWND>(0x12345678);

    RecoilApp app{};
    ResetFatalErrorProbe();
    app.FatalErrorAndExit(7);
    const bool nonFatalOk = g_fatalSequence == 0 && g_fatalLocCaptionCalls == 0 &&
                            g_fatalLocTextCalls == 0;

    ResetFatalErrorProbe();
    app.FatalErrorAndExit(-1);
    const bool fatalOk = g_fatalLocCaptionCalls == 1 && g_fatalLocTextCalls == 1 &&
                         g_fatalBriefingCalls == 1 && g_fatalFlipCalls == 1 &&
                         g_fatalSndCalls == 1 && g_fatalNetworkCalls == 1 &&
                         g_fatalVideoCalls == 1 && g_fatalSleepCalls == 1 &&
                         g_fatalBeepCalls == 1 && g_fatalMessageBoxCalls == 1 &&
                         g_fatalExitCalls == 1 && g_fatalSequence == 9 &&
                         g_fatalOrderOk && g_fatalArgsOk;

    g_RecoilApp_hWndMain = oldMainHwnd;
    RestoreImportPatch(messageBoxPatch);
    RestoreImportPatch(beepPatch);
    RestoreImportPatch(sleepPatch);
    RestoreFunctionPatch(exitPatch);
    RestoreFunctionPatch(videoPatch);
    RestoreFunctionPatch(networkPatch);
    RestoreFunctionPatch(sndPatch);
    RestoreFunctionPatch(flipPatch);
    RestoreFunctionPatch(briefingPatch);
    RestoreFunctionPatch(zlocPatch);

    return nonFatalOk && fatalOk ? 0 : 1;
}

extern "C" int czrecoil_frame_start_mode_menu_handlers_smoke(void) {
    CodeFunctionPatch setupPatch{};
    if (!PatchFunctionJump(RecoilAppLoadSetupProc(),
                           reinterpret_cast<void *>(&FakeStartModeLoadZbdAndSetup),
                           setupPatch)) {
        return 10;
    }

    const bool multiplayerOk =
        RunStartModeScenario(&CZRecoilFrame::OnMenuStartMultiplayer, 1, 0);
    const bool campaign1Ok =
        RunStartModeScenario(&CZRecoilFrame::OnMenuStartCampaignMode, 2, 1);
    const bool campaign2Ok =
        RunStartModeScenario(&CZRecoilFrame::OnMenuStartCampaignMode2, 3, 0x22);
    const bool campaign3Ok =
        RunStartModeScenario(&CZRecoilFrame::OnMenuStartCampaignMode3, 4, 0x33);
    const bool campaign4Ok =
        RunStartModeScenario(&CZRecoilFrame::OnMenuStartCampaignMode4, 5, 0x44);
    const bool campaign5Ok =
        RunStartModeScenario(&CZRecoilFrame::OnMenuStartCampaignMode5, 6, 0x55);

    RestoreFunctionPatch(setupPatch);
    return multiplayerOk && campaign1Ok && campaign2Ok && campaign3Ok && campaign4Ok &&
                   campaign5Ok
               ? 0
               : 1;
}

extern "C" int czrecoil_frame_select_hw_api_menu_handlers_smoke(void) {
    CodeFunctionPatch ensurePatch{};
    if (!PatchFunctionJump(EnsureHwApiInitializedProc(),
                           reinterpret_cast<void *>(&FakeMenuSelectEnsureHwApiInitialized),
                           ensurePatch)) {
        return 10;
    }

    const bool api0Ok = RunHwApiSelectScenario(&CZRecoilFrame::OnMenuSelectHwApi0, 0);
    const bool api1Ok = RunHwApiSelectScenario(&CZRecoilFrame::OnMenuSelectHwApi1, 1);
    const bool api2Ok = RunHwApiSelectScenario(&CZRecoilFrame::OnMenuSelectHwApi2, 2);
    const bool api3Ok = RunHwApiSelectScenario(&CZRecoilFrame::OnMenuSelectHwApi3, 3);

    RestoreFunctionPatch(ensurePatch);
    return api0Ok && api1Ok && api2Ok && api3Ok ? 0 : 1;
}

extern "C" int czrecoil_frame_toggle_archive_banks_smoke(void) {
    const int oldMissionFlags = g_HudSensorTracker.missionFlags;
    const int oldUseArchiveBanksFlag = g_zSnd_UseArchiveBanksFlag;

    HMENU const menu = CreateMenu();
    if (menu == nullptr) {
        return 10;
    }
    if (AppendMenuA(menu, MF_STRING, 0x9c6b, "Archive banks") == 0) {
        DestroyMenu(menu);
        return 11;
    }

    CZRecoilFrame frame{};
    frame.m_mainMenuHandle = menu;
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
    DestroyMenu(menu);
    return enabledOk && disabledOk ? 0 : 1;
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
    frame.m_mainMenuHandle = menu;
    g_zVid_TexturePackLoadState = 0;

    frame.OnMenuToggleTexturePacks();
    const bool enabledOk = g_zVid_TexturePackLoadState == 1 &&
                           (GetMenuState(menu, 0x9c7b, MF_BYCOMMAND) & MF_CHECKED) != 0;

    frame.OnMenuToggleTexturePacks();
    const bool disabledOk = g_zVid_TexturePackLoadState == 0 &&
                            (GetMenuState(menu, 0x9c7b, MF_BYCOMMAND) & MF_CHECKED) == 0;

    g_zVid_TexturePackLoadState = oldTexturePackLoadState;
    DestroyMenu(menu);
    return enabledOk && disabledOk ? 0 : 1;
}

extern "C" int hud_ui_save_load_entry_is_newer_than_smoke(void) {
    HudUiSaveLoadEntry older{};
    HudUiSaveLoadEntry same{};
    HudUiSaveLoadEntry newer{};

    older.ftLastWriteTime.dwLowDateTime = 100;
    older.ftLastWriteTime.dwHighDateTime = 0;
    same.ftLastWriteTime = older.ftLastWriteTime;
    newer.ftLastWriteTime.dwLowDateTime = 101;
    newer.ftLastWriteTime.dwHighDateTime = 0;

    return newer.IsNewerThan(&older) == 1 && older.IsNewerThan(&newer) == 0 &&
                   older.IsNewerThan(&same) == 0
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_list_item_constructor_smoke(void) {
    HudUiSaveLoadListItem item{};

    HudUiSaveLoadListItem *const result = item.Constructor();

    return result == &item && item.vftable == &g_HudUiSaveLoadListItem_Vtbl &&
                   item.vftable->OnActivate != nullptr && item.layoutY == 32767 &&
                   item.layoutX == -1
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_list_item_on_activate_smoke(void) {
    static HudUiSaveLoadDialog dialog;
    static HudUiSaveLoadEntry entries[4];
    static HudUiSaveLoadListItemVtable itemVtable;
    char gameNameBuffer[32]{};

    std::memset(&dialog, 0, sizeof(dialog));
    std::memset(entries, 0, sizeof(entries));
    std::memset(&itemVtable, 0, sizeof(itemVtable));
    itemVtable.Invalidate = SaveLoadListItemInvalidateThunk();
    itemVtable.SetTextFmt = FakeSaveLoadListItemSetTextFmt;
    itemVtable.SetVisible = SaveLoadListItemSetVisibleThunk();
    for (int i = 0; i < 9; ++i) {
        dialog.entryWidgets[i].vftable = &itemVtable;
    }
    for (int i = 0; i < 4; ++i) {
        std::sprintf(entries[i].cFileName, "row%d.sav", i);
    }

    dialog.fileEntries.begin = entries;
    dialog.fileEntries.end = entries + 4;
    dialog.fileEntries.capacityEnd = entries + 4;
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);
    ResetSaveLoadInitListItemCapture(&dialog);

    HudUiSaveLoadListItem item{};
    item.parent = &dialog;
    item.layoutX = 2;
    item.OnActivate();
    const bool selected =
        dialog.selectedEntryIndex == 2 && std::strcmp(gameNameBuffer, "row2.sav") == 0;

    item.parent = nullptr;
    item.layoutX = 1;
    item.OnActivate();
    const bool nullParentSkipped = dialog.selectedEntryIndex == 2;

    return selected && nullParentSkipped ? 0 : 1;
}

extern "C" int hud_ui_save_load_insert_entry_sorted_prefix_smoke(void) {
    HudUiSaveLoadEntry entries[4]{};
    entries[0].ftLastWriteTime.dwLowDateTime = 400;
    entries[1].ftLastWriteTime.dwLowDateTime = 100;

    HudUiSaveLoadEntry middle{};
    middle.ftLastWriteTime.dwLowDateTime = 250;
    HudUiSaveLoadDialog::InsertEntryIntoSortedPrefix(&entries[2], middle);

    const bool insertedMiddle = entries[0].ftLastWriteTime.dwLowDateTime == 400 &&
                                entries[1].ftLastWriteTime.dwLowDateTime == 250 &&
                                entries[2].ftLastWriteTime.dwLowDateTime == 100;

    HudUiSaveLoadEntry oldest{};
    oldest.ftLastWriteTime.dwLowDateTime = 50;
    HudUiSaveLoadDialog::InsertEntryIntoSortedPrefix(&entries[3], oldest);

    return insertedMiddle && entries[0].ftLastWriteTime.dwLowDateTime == 400 &&
                   entries[1].ftLastWriteTime.dwLowDateTime == 250 &&
                   entries[2].ftLastWriteTime.dwLowDateTime == 100 &&
                   entries[3].ftLastWriteTime.dwLowDateTime == 50
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_partition_entries_by_pivot_smoke(void) {
    HudUiSaveLoadEntry entries[5]{};
    entries[0].ftLastWriteTime.dwLowDateTime = 300;
    entries[1].ftLastWriteTime.dwLowDateTime = 100;
    entries[2].ftLastWriteTime.dwLowDateTime = 500;
    entries[3].ftLastWriteTime.dwLowDateTime = 200;
    entries[4].ftLastWriteTime.dwLowDateTime = 400;

    HudUiSaveLoadEntry pivot{};
    pivot.ftLastWriteTime.dwLowDateTime = 300;

    HudUiSaveLoadEntry *split =
        HudUiSaveLoadDialog::PartitionEntriesByPivot(entries, entries + 5, pivot);
    if (split < entries || split > entries + 5) {
        return 1;
    }

    for (HudUiSaveLoadEntry *entry = entries; entry != split; ++entry) {
        if (pivot.IsNewerThan(entry) != 0) {
            return 1;
        }
    }

    for (HudUiSaveLoadEntry *entry = split; entry != entries + 5; ++entry) {
        if (entry->IsNewerThan(&pivot) != 0) {
            return 1;
        }
    }

    return 0;
}

extern "C" int hud_ui_save_load_sort_entry_range_smoke(void) {
    HudUiSaveLoadEntry smallEntries[16]{};
    for (int i = 0; i < 16; ++i) {
        smallEntries[i].ftLastWriteTime.dwLowDateTime = static_cast<DWORD>(i + 1);
    }

    HudUiSaveLoadDialog::SortEntryRange(smallEntries, smallEntries + 16, 0);
    for (int i = 0; i < 16; ++i) {
        if (smallEntries[i].ftLastWriteTime.dwLowDateTime != static_cast<DWORD>(i + 1)) {
            return 1;
        }
    }

    HudUiSaveLoadEntry largeEntries[17]{};
    for (int i = 0; i < 17; ++i) {
        largeEntries[i].ftLastWriteTime.dwLowDateTime = static_cast<DWORD>((i + 1) * 100);
    }

    HudUiSaveLoadDialog::SortEntryRange(largeEntries, largeEntries + 17, 0);
    for (int i = 0; i < 17; ++i) {
        const DWORD expectedTime = static_cast<DWORD>((17 - i) * 100);
        if (largeEntries[i].ftLastWriteTime.dwLowDateTime != expectedTime) {
            return 1;
        }
    }

    return 0;
}

extern "C" int hud_ui_save_load_refresh_file_list_smoke(void) {
    DWORD savedGamesAttrs = GetFileAttributesA("SavedGames");
    bool createdDirectory = false;
    if (savedGamesAttrs == INVALID_FILE_ATTRIBUTES) {
        if (CreateDirectoryA("SavedGames", nullptr) == 0) {
            return 1;
        }
        createdDirectory = true;
    } else if ((savedGamesAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return 1;
    }

    CleanupSaveLoadSmokeFiles(false);
    if (CreateDirectoryA("SavedGames\\recoil_refresh_smoke_dir", nullptr) == 0) {
        CleanupSaveLoadSmokeFiles(createdDirectory);
        return 1;
    }

    const bool filesWritten =
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_old.sav", 100) &&
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_new.sav", 400) &&
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_middle.sav", 250) &&
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_oldest.sav", 50);
    if (!filesWritten) {
        CleanupSaveLoadSmokeFiles(createdDirectory);
        return 1;
    }

    static HudUiSaveLoadDialog dialog;
    static HudUiSaveLoadEntry storage[8];
    std::memset(&dialog, 0, sizeof(dialog));
    std::memset(storage, 0, sizeof(storage));
    dialog.fileEntries.begin = storage;
    dialog.fileEntries.end = storage;
    dialog.fileEntries.capacityEnd = storage + 8;

    dialog.RefreshSaveFileList();

    int result = 0;
    const int entryCount = static_cast<int>(dialog.fileEntries.end - dialog.fileEntries.begin);
    const DWORD expectedTimes[4] = {400, 250, 100, 50};
    if (entryCount != 4) {
        result = 1;
    } else {
        for (int i = 0; i < 4; ++i) {
            if (dialog.fileEntries.begin[i].ftLastWriteTime.dwLowDateTime != expectedTimes[i]) {
                result = 1;
            }
        }
    }

    CleanupSaveLoadSmokeFiles(createdDirectory);
    return result;
}

extern "C" int hud_ui_save_load_initialize_file_entries_smoke(void) {
    DWORD savedGamesAttrs = GetFileAttributesA("SavedGames");
    bool createdDirectory = false;
    if (savedGamesAttrs == INVALID_FILE_ATTRIBUTES) {
        if (CreateDirectoryA("SavedGames", nullptr) == 0) {
            return 1;
        }
        createdDirectory = true;
    } else if ((savedGamesAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return 1;
    }

    CleanupSaveLoadSmokeFiles(false);
    const bool filesWritten =
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_old.sav", 100) &&
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_new.sav", 400) &&
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_middle.sav", 250) &&
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_oldest.sav", 50);
    if (!filesWritten) {
        CleanupSaveLoadSmokeFiles(createdDirectory);
        return 1;
    }

    static HudUiSaveLoadDialog dialog;
    static HudUiSaveLoadEntry storage[8];
    static HudUiSaveLoadListItemVtable itemVtable;
    std::memset(&dialog, 0, sizeof(dialog));
    std::memset(storage, 0, sizeof(storage));
    std::memset(&itemVtable, 0, sizeof(itemVtable));
    itemVtable.Invalidate = SaveLoadListItemInvalidateThunk();
    itemVtable.SetTextFmt = FakeSaveLoadListItemSetTextFmt;
    itemVtable.SetVisible = SaveLoadListItemSetVisibleThunk();
    for (int i = 0; i < 9; ++i) {
        dialog.entryWidgets[i].vftable = &itemVtable;
    }
    dialog.fileEntries.begin = storage;
    dialog.fileEntries.end = storage;
    dialog.fileEntries.capacityEnd = storage + 8;

    ResetSaveLoadInitListItemCapture(&dialog);
    dialog.InitializeFileEntries();

    const int expectedLayoutY[9] = {9830, 16383, 32767, 32767, 32767,
                                    29490, 22936, 16383, 9830};
    const char *expectedText[4] = {
        "recoil_refresh_smoke_new.sav",
        "recoil_refresh_smoke_middle.sav",
        "recoil_refresh_smoke_old.sav",
        "recoil_refresh_smoke_oldest.sav",
    };

    int result = 0;
    for (int i = 0; i < 9; ++i) {
        if (dialog.entryWidgets[i].layoutY != expectedLayoutY[i]) {
            result = 1;
        }
    }

    if (g_saveLoadInitTextCount != 4 || g_saveLoadInitVisibleCount != 4 ||
        !g_saveLoadInitFmtOk || !g_saveLoadInitVisibleOk) {
        result = 1;
    }

    for (int i = 0; i < 4; ++i) {
        if (dialog.entryWidgets[i].layoutX != i || g_saveLoadInitTextIndices[i] != i ||
            g_saveLoadInitVisibleIndices[i] != i || g_saveLoadInitVisibleValues[i] != 1 ||
            std::strcmp(g_saveLoadInitTexts[i], expectedText[i]) != 0) {
            result = 1;
        }
    }

    CleanupSaveLoadSmokeFiles(createdDirectory);
    return result;
}

extern "C" int hud_ui_save_load_delete_save_file_smoke(void) {
    DWORD savedGamesAttrs = GetFileAttributesA("SavedGames");
    bool createdDirectory = false;
    if (savedGamesAttrs == INVALID_FILE_ATTRIBUTES) {
        if (CreateDirectoryA("SavedGames", nullptr) == 0) {
            return 1;
        }
        createdDirectory = true;
    } else if ((savedGamesAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return 1;
    }

    CleanupSaveLoadSmokeFiles(false);
    const bool filesWritten =
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_old.sav", 100) &&
        WriteSaveLoadSmokeFile("SavedGames\\recoil_refresh_smoke_new.sav", 400);
    if (!filesWritten) {
        CleanupSaveLoadSmokeFiles(createdDirectory);
        return 1;
    }

    static HudUiSaveLoadDialog dialog;
    static HudUiSaveLoadEntry storage[4];
    static HudUiSaveLoadListItemVtable itemVtable;
    char gameNameBuffer[64]{};

    std::memset(&dialog, 0, sizeof(dialog));
    std::memset(storage, 0, sizeof(storage));
    std::memset(&itemVtable, 0, sizeof(itemVtable));
    itemVtable.Invalidate = SaveLoadListItemInvalidateThunk();
    itemVtable.SetTextFmt = FakeSaveLoadListItemSetTextFmt;
    itemVtable.SetVisible = SaveLoadListItemSetVisibleThunk();
    for (int i = 0; i < 9; ++i) {
        dialog.entryWidgets[i].vftable = &itemVtable;
    }

    std::strcpy(gameNameBuffer, "recoil_refresh_smoke_new.sav");
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);
    dialog.fileEntries.begin = storage;
    dialog.fileEntries.end = storage;
    dialog.fileEntries.capacityEnd = storage + 4;
    dialog.selectedEntryIndex = 1;

    ResetSaveLoadInitListItemCapture(&dialog);
    dialog.DeleteSaveFile(0);

    const bool deleted =
        GetFileAttributesA("SavedGames\\recoil_refresh_smoke_new.sav") ==
        INVALID_FILE_ATTRIBUTES;
    const bool kept =
        GetFileAttributesA("SavedGames\\recoil_refresh_smoke_old.sav") !=
        INVALID_FILE_ATTRIBUTES;
    const int entryCount = static_cast<int>(dialog.fileEntries.end - dialog.fileEntries.begin);
    const bool refreshed =
        entryCount == 1 &&
        std::strcmp(dialog.fileEntries.begin[0].cFileName,
                    "recoil_refresh_smoke_old.sav") == 0;
    const bool selection = dialog.selectedEntryIndex == 0 &&
                           std::strcmp(gameNameBuffer,
                                       "recoil_refresh_smoke_old.sav") == 0;

    CleanupSaveLoadSmokeFiles(createdDirectory);
    return deleted && kept && refreshed && selection ? 0 : 1;
}

extern "C" int hud_ui_save_load_set_selected_entry_index_smoke(void) {
    static HudUiSaveLoadDialog dialog;
    static HudUiSaveLoadEntry entries[5];
    static HudUiSaveLoadListItemVtable itemVtable;
    char gameNameBuffer[32]{};

    std::memset(&dialog, 0, sizeof(dialog));
    std::memset(entries, 0, sizeof(entries));
    std::memset(&itemVtable, 0, sizeof(itemVtable));
    itemVtable.Invalidate = SaveLoadListItemInvalidateThunk();
    itemVtable.SetTextFmt = FakeSaveLoadListItemSetTextFmt;
    itemVtable.SetVisible = SaveLoadListItemSetVisibleThunk();

    for (int i = 0; i < 9; ++i) {
        dialog.entryWidgets[i].vftable = &itemVtable;
    }

    for (int i = 0; i < 5; ++i) {
        std::sprintf(entries[i].cFileName, "entry%d.sav", i);
    }

    dialog.fileEntries.begin = entries;
    dialog.fileEntries.end = entries + 5;
    dialog.fileEntries.capacityEnd = entries + 5;
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);

    ResetSaveLoadInitListItemCapture(&dialog);
    dialog.SetSelectedEntryIndex(2);

    const int expectedVisibleValues[9] = {0, 1, 1, 1, 1, 0, 0, 0, 0};
    const int expectedTextIndices[4] = {1, 2, 3, 4};
    const char *expectedText[4] = {"entry0.sav", "entry1.sav", "entry3.sav", "entry4.sav"};

    int result = 0;
    if (dialog.selectedEntryIndex != 2 || std::strcmp(gameNameBuffer, "entry2.sav") != 0 ||
        g_saveLoadInitVisibleCount != 9 || g_saveLoadInitTextCount != 4 ||
        g_saveLoadInitInvalidateCount != 4 || !g_saveLoadInitFmtOk || !g_saveLoadInitVisibleOk) {
        result = 1;
    }

    for (int i = 0; i < 9; ++i) {
        if (g_saveLoadInitVisibleIndices[i] != i ||
            g_saveLoadInitVisibleValues[i] != expectedVisibleValues[i]) {
            result = 1;
        }
    }

    for (int i = 0; i < 4; ++i) {
        if (g_saveLoadInitTextIndices[i] != expectedTextIndices[i] ||
            g_saveLoadInitInvalidateIndices[i] != expectedTextIndices[i] ||
            std::strcmp(g_saveLoadInitTexts[i], expectedText[i]) != 0) {
            result = 1;
        }
    }

    return result;
}

extern "C" int hud_ui_save_load_game_name_input_smoke(void) {
    HudUiSaveLoadGameNameInput input{};
    input.BaseConstructor();
    input.Update("SAVE1");
    input.textInput.cursor = 0;
    input.sliderBorder.inputActive = 0;

    input.OnActivate();
    const bool activated =
        std::strcmp(input.GetBuffer(), "SAVE1") == 0 && input.textInput.cursor == 5 &&
        input.sliderBorder.inputActive == 1;

    input.Update("");
    input.OnRawKeyboardEvent('A');
    input.OnRawKeyboardEvent('!');
    input.OnRawKeyboardEvent('_');
    const bool rawFiltered = std::strcmp(input.GetBuffer(), "A_") == 0;

    input.Destructor();
    return activated && rawFiltered ? 0 : 1;
}

extern "C" int hud_ui_save_load_next_prev_buttons_smoke(void) {
    static HudUiSaveLoadDialog dialog;
    static HudUiSaveLoadEntry entries[3];
    static HudUiSaveLoadListItemVtable itemVtable;
    char gameNameBuffer[32]{};

    std::memset(&dialog, 0, sizeof(dialog));
    std::memset(entries, 0, sizeof(entries));
    std::memset(&itemVtable, 0, sizeof(itemVtable));
    itemVtable.Invalidate = SaveLoadListItemInvalidateThunk();
    itemVtable.SetTextFmt = FakeSaveLoadListItemSetTextFmt;
    itemVtable.SetVisible = SaveLoadListItemSetVisibleThunk();
    for (int i = 0; i < 9; ++i) {
        dialog.entryWidgets[i].vftable = &itemVtable;
    }
    for (int i = 0; i < 3; ++i) {
        std::sprintf(entries[i].cFileName, "nav%d.sav", i);
    }

    dialog.fileEntries.begin = entries;
    dialog.fileEntries.end = entries + 3;
    dialog.fileEntries.capacityEnd = entries + 3;
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);
    ResetSaveLoadInitListItemCapture(&dialog);

    HudUiSaveLoadNextButton nextButton{};
    nextButton.owner = &dialog;
    dialog.selectedEntryIndex = 1;
    nextButton.OnActivate();
    const bool nextAdvanced = dialog.selectedEntryIndex == 2;
    nextButton.OnActivate();
    const bool nextStopped = dialog.selectedEntryIndex == 2;

    HudUiSaveLoadPrevButton prevButton{};
    prevButton.owner = &dialog;
    prevButton.OnActivate();
    const bool prevAdvanced = dialog.selectedEntryIndex == 1;
    dialog.selectedEntryIndex = 0;
    prevButton.OnActivate();
    const bool prevStopped = dialog.selectedEntryIndex == 0;

    return nextAdvanced && nextStopped && prevAdvanced && prevStopped ? 0 : 1;
}

extern "C" int hud_ui_save_load_delete_button_on_activate_smoke(void) {
    static HudUiSaveLoadDialog dialog;
    char gameNameBuffer[32]{};

    std::memset(&dialog, 0, sizeof(dialog));
    dialog.selectedEntryIndex = 7;
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);

    HudUiSaveLoadDeleteButton deleteButton{};
    deleteButton.owner = &dialog;
    deleteButton.OnActivate();

    return dialog.selectedEntryIndex == 7 ? 0 : 1;
}

extern "C" int hud_ui_save_game_primary_action_button_on_activate_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    g_RecoilApp = RecoilApp{};

    static HudUiSaveLoadDialog dialog;
    char gameNameBuffer[1]{};
    std::memset(&dialog, 0, sizeof(dialog));
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);

    HudUiSaveGamePrimaryActionButton button{};
    button.Constructor();
    button.owner = &dialog;
    button.OnActivate();
    const bool ownerOk = g_RecoilApp.m_stateQueue_118.m_itemCount == 0;

    button.owner = nullptr;
    button.OnActivate();
    const bool nullOwnerOk = g_RecoilApp.m_stateQueue_118.m_itemCount == 0;

    g_RecoilApp = oldApp;
    return ownerOk && nullOwnerOk ? 0 : 1;
}

extern "C" int hud_ui_load_game_dialog_on_primary_action_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    g_RecoilApp = RecoilApp{};

    static HudUiLoadGameDialog dialog;
    char gameNameBuffer[1]{};
    std::memset(&dialog, 0, sizeof(dialog));
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);

    dialog.OnPrimaryAction();
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    const bool emptyNameQueuedExit = IsSingleExitCurrentQueueItem(queue, 0);
    CleanupQueuedItems(queue);

    g_RecoilApp = oldApp;
    return emptyNameQueuedExit ? 0 : 1;
}

extern "C" int hud_ui_load_game_dialog_process_dialog_result_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    g_RecoilApp = RecoilApp{};

    static HudUiLoadGameDialog dialog;
    char gameNameBuffer[1]{};
    std::memset(&dialog, 0, sizeof(dialog));
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);

    dialog.ProcessDialogResult();
    const bool emptyNameReturned = g_RecoilApp.m_stateQueue_118.m_itemCount == 0;

    g_RecoilApp = oldApp;
    return emptyNameReturned ? 0 : 1;
}

extern "C" int hud_ui_load_game_primary_action_button_on_activate_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    g_RecoilApp = RecoilApp{};

    static HudUiLoadGameDialog dialog;
    char gameNameBuffer[1]{};
    std::memset(&dialog, 0, sizeof(dialog));
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);

    HudUiLoadGamePrimaryActionButton button{};
    button.Constructor();
    button.owner = &dialog;
    button.OnActivate();
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    const bool ownerOk = IsSingleExitCurrentQueueItem(queue, 0);
    CleanupQueuedItems(queue);

    button.owner = nullptr;
    button.OnActivate();
    const bool nullOwnerOk = g_RecoilApp.m_stateQueue_118.m_itemCount == 0;

    g_RecoilApp = oldApp;
    return ownerOk && nullOwnerOk ? 0 : 1;
}

extern "C" int hud_ui_save_load_process_dialog_result_smoke(void) {
    DWORD savedGamesAttrs = GetFileAttributesA("SavedGames");
    bool createdDirectory = false;
    if (savedGamesAttrs == INVALID_FILE_ATTRIBUTES) {
        if (CreateDirectoryA("SavedGames", nullptr) == 0) {
            return 1;
        }
        createdDirectory = true;
    } else if ((savedGamesAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return 1;
    }

    CleanupSaveLoadSmokeFiles(false);
    const unsigned char payload[4] = {9, 8, 7, 6};
    zIndexArchive writer = {};
    writer.Reset();
    if (writer.OpenCreateWrite("SavedGames\\recoil_process_smoke.sav") == 0 ||
        writer.AddFileRecord("Smoke/Process", payload, sizeof(payload), nullptr, nullptr) == 0 ||
        writer.CloseAndFreeRecords() == 0) {
        writer.CloseAndFreeRecords();
        CleanupSaveLoadSmokeFiles(createdDirectory);
        return 1;
    }

    const RecoilApp oldApp = g_RecoilApp;
    const RecoilStateSaveLoadTransition oldSaveLoadTransition = g_RecoilStateSaveLoadTransition;
    const RecoilStateMainMenuTransition oldMainMenuTransition = g_RecoilState_MainMenuTransition;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int *const oldMuteOption = ZOPT_MUTE_SOUND;
    int joystickOption = 0;
    int gameControlOptions = 0;
    int muteOption = 0;
    ZOPT_INPUT_JOYSTICK = &joystickOption;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    ZOPT_MUTE_SOUND = &muteOption;

    int result = 0;
    char gameNameBuffer[64]{};
    std::strcpy(gameNameBuffer, "recoil_process_smoke.sav");

    static HudUiSaveLoadDialog dialog;
    std::memset(&dialog, 0, sizeof(dialog));
    dialog.gameNameInput.textInput.buffer = gameNameBuffer;
    dialog.gameNameInput.textInput.capacity = sizeof(gameNameBuffer);

    TestAppState oldState{};
    oldState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeTestZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    g_stateEnterCount = 0;
    g_stateExitCount = 0;
    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));
    g_RecoilApp.m_missionFmvState_1d8.base.vftable = oldState.vftable;
    g_RecoilStateSaveLoadTransition = RecoilStateSaveLoadTransition{};
    g_RecoilStateSaveLoadTransition.m_transitionMode = RECOIL_SAVELOAD_MODE_STANDARD;
    g_RecoilState_MainMenuTransition = RecoilStateMainMenuTransition{};

    dialog.ProcessDialogResult();

    RecoilApp_StateQueue &standardQueue = g_RecoilApp.m_stateQueue_118;
    char *const pendingPath = reinterpret_cast<char *>(
        static_cast<std::uintptr_t>(g_RecoilApp.m_playState_208.pPendingLoadGameStartPath));
    bool standardOk =
        pendingPath != nullptr &&
        std::strcmp(pendingPath, "SavedGames\\recoil_process_smoke.sav") == 0 &&
        g_RecoilApp.m_missionFmvState_1d8.m_skipMissionFmv == 1 &&
        g_stateExitCount == 2 && g_stateEnterCount == 1 && standardQueue.m_itemCount == 2;
    if (standardOk) {
        const RecoilPtr32 firstSlotValue = standardQueue.m_writeBlock.m_cursor - 8;
        auto *const firstSlot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(firstSlotValue));
        auto *const secondSlot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(firstSlotValue + 4));
        auto *const exitItem = reinterpret_cast<RecoilApp_StateQueueItem *>(
            static_cast<std::uintptr_t>(*firstSlot));
        auto *const switchItem = reinterpret_cast<RecoilApp_StateQueueItem *>(
            static_cast<std::uintptr_t>(*secondSlot));
        standardOk =
            exitItem->m_kind == RecoilApp_StateQueueKind_ExitCurrent && exitItem->m_param == 1 &&
            switchItem->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
            switchItem->m_stateObj == static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                                          &g_RecoilApp.m_missionFmvState_1d8.base)) &&
            switchItem->m_param == 0;
    }

    free(pendingPath);
    CleanupQueuedItems(standardQueue);
    if (!standardOk) {
        result = 3;
    }

    g_stateEnterCount = 0;
    g_stateExitCount = 0;
    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex_0c8 = 0;
    g_RecoilApp.m_stateStack_0d8[0] =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&oldState));
    g_RecoilApp.m_transitionFadeTimer150 = 0.0f;
    g_RecoilStateSaveLoadTransition = RecoilStateSaveLoadTransition{};
    g_RecoilStateSaveLoadTransition.m_transitionMode = RECOIL_SAVELOAD_MODE_FADE;
    muteOption = 0;

    dialog.ProcessDialogResult();

    RecoilApp_StateQueue &fadeQueue = g_RecoilApp.m_stateQueue_118;
    bool fadeOk = g_RecoilApp.m_transitionFadeTimer150 == 5.0f &&
                  zOpt::GetMuteSoundOption() == 1 && g_stateExitCount == 2 &&
                  g_stateEnterCount == 0 && fadeQueue.m_itemCount == 2;
    if (fadeOk) {
        const RecoilPtr32 firstSlotValue = fadeQueue.m_writeBlock.m_cursor - 8;
        auto *const firstSlot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(firstSlotValue));
        auto *const secondSlot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(firstSlotValue + 4));
        auto *const firstExit = reinterpret_cast<RecoilApp_StateQueueItem *>(
            static_cast<std::uintptr_t>(*firstSlot));
        auto *const secondExit = reinterpret_cast<RecoilApp_StateQueueItem *>(
            static_cast<std::uintptr_t>(*secondSlot));
        fadeOk = firstExit->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
                 firstExit->m_param == 1 &&
                 secondExit->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
                 secondExit->m_param == 1;
    }
    CleanupQueuedItems(fadeQueue);
    if (!fadeOk && result == 0) {
        result = 4;
    }

    g_RecoilApp = oldApp;
    g_RecoilStateSaveLoadTransition = oldSaveLoadTransition;
    g_RecoilState_MainMenuTransition = oldMainMenuTransition;
    g_zUtil_ZbdManager = oldZbdManager;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    ZOPT_MUTE_SOUND = oldMuteOption;
    CleanupSaveLoadSmokeFiles(createdDirectory);
    return result;
}

extern "C" int hud_ui_save_game_dialog_init_layout_smoke(void) {
    static HudUiSaveGameDialog dialog;
    std::memset(&dialog, 0, sizeof(dialog));

    CodeFunctionPatch loadPatch{};
    if (!PatchFunctionJump(HudUiBackgroundLoadFromZrdProc(), FakeSaveGameInitLoadFromZrdProc(),
                           loadPatch)) {
        return 1;
    }

    g_saveGameInitLoadCalls = 0;
    g_saveGameInitLoadArgsOk = false;
    g_saveLoadInitExpectedSectionName = "SAVE_GAME_DIALOG";
    HudUiSaveGameDialog *const result = dialog.InitLayout();
    RestoreFunctionPatch(loadPatch);

    int checkResult = 0;
    if (result != &dialog || g_saveGameInitLoadCalls != 1 || !g_saveGameInitLoadArgsOk ||
        dialog.deleteButton.base.ftable == nullptr ||
        dialog.backButton.base.ftable == nullptr ||
        dialog.nextEntryButton.base.ftable == nullptr ||
        dialog.prevEntryButton.base.ftable == nullptr ||
        dialog.primaryActionButton.base.ftable == nullptr ||
        dialog.gameNameInput.base.base.ftable == nullptr ||
        dialog.gameNameInput.textInput.buffer == nullptr ||
        dialog.gameNameInput.textInput.capacity != 20 ||
        std::strcmp(dialog.gameNameInput.textInput.buffer, "") != 0 ||
        dialog.selectedEntryIndex != -1) {
        checkResult = 1;
    }

    for (int i = 0; i < 9; ++i) {
        if (dialog.entryWidgets[i].vftable != &g_HudUiSaveLoadListItem_Vtbl) {
            checkResult = 1;
        }
    }

    dialog.gameNameInput.Destructor();
    if (dialog.fileEntries.begin != nullptr) {
        ::operator delete(dialog.fileEntries.begin);
        dialog.fileEntries.begin = nullptr;
        dialog.fileEntries.end = nullptr;
        dialog.fileEntries.capacityEnd = nullptr;
    }
    return checkResult;
}

extern "C" int hud_ui_load_game_dialog_constructor_smoke(void) {
    static HudUiLoadGameDialog dialog;
    std::memset(&dialog, 0, sizeof(dialog));

    CodeFunctionPatch loadPatch{};
    if (!PatchFunctionJump(HudUiBackgroundLoadFromZrdProc(), FakeSaveGameInitLoadFromZrdProc(),
                           loadPatch)) {
        return 1;
    }

    g_saveGameInitLoadCalls = 0;
    g_saveGameInitLoadArgsOk = false;
    g_saveLoadInitExpectedSectionName = "LOAD_GAME_DIALOG";
    HudUiLoadGameDialog *const result = dialog.Constructor();
    RestoreFunctionPatch(loadPatch);

    int checkResult = 0;
    if (result != &dialog || g_saveGameInitLoadCalls != 1 || !g_saveGameInitLoadArgsOk ||
        dialog.deleteButton.base.ftable == nullptr ||
        dialog.backButton.base.ftable == nullptr ||
        dialog.nextEntryButton.base.ftable == nullptr ||
        dialog.prevEntryButton.base.ftable == nullptr ||
        dialog.primaryActionButton.base.ftable == nullptr ||
        dialog.gameNameInput.base.base.ftable == nullptr ||
        dialog.gameNameInput.textInput.buffer == nullptr ||
        dialog.gameNameInput.textInput.capacity != 20 ||
        std::strcmp(dialog.gameNameInput.textInput.buffer, "") != 0 ||
        dialog.selectedEntryIndex != 0) {
        checkResult = 1;
    }

    for (int i = 0; i < 9; ++i) {
        if (dialog.entryWidgets[i].vftable != &g_HudUiSaveLoadListItem_Vtbl) {
            checkResult = 1;
        }
    }

    dialog.gameNameInput.Destructor();
    if (dialog.fileEntries.begin != nullptr) {
        ::operator delete(dialog.fileEntries.begin);
        dialog.fileEntries.begin = nullptr;
        dialog.fileEntries.end = nullptr;
        dialog.fileEntries.capacityEnd = nullptr;
    }
    return checkResult;
}

static void InitLoadGameDialogDestructorFixture(HudUiLoadGameDialog *dialog) {
    std::memset(dialog, 0, sizeof(*dialog));
    dialog->base.Constructor();
    dialog->deleteButton.Constructor();
    dialog->backButton.Constructor();
    dialog->nextEntryButton.Constructor();
    dialog->prevEntryButton.Constructor();
    dialog->gameNameInput.BaseConstructor();
    dialog->gameNameInput.textInput.AllocTextBuffer(20);
    dialog->primaryActionButton.Constructor();

    for (int i = 0; i < 9; ++i) {
        dialog->entryWidgets[i].Constructor();
    }

    dialog->fileEntries.begin =
        static_cast<HudUiSaveLoadEntry *>(::operator new(sizeof(HudUiSaveLoadEntry) * 2));
    dialog->fileEntries.end = dialog->fileEntries.begin + 1;
    dialog->fileEntries.capacityEnd = dialog->fileEntries.begin + 2;
}

static int LoadGameDialogDestroyedCheck(const HudUiLoadGameDialog &dialog) {
    if (dialog.fileEntries.begin != nullptr || dialog.fileEntries.end != nullptr ||
        dialog.fileEntries.capacityEnd != nullptr) {
        return 1;
    }

    if (dialog.base.base.base.vptr != &g_HudUiContainer_FTable) {
        return 2;
    }

    if (dialog.primaryActionButton.base.ftable !=
        (const HudUiWidget_FTable *)(&g_HudUiCommon_FTable)) {
        return 3;
    }

    if (dialog.deleteButton.base.ftable !=
        (const HudUiWidget_FTable *)(&g_HudUiCommon_FTable)) {
        return 4;
    }

    if (dialog.backButton.base.ftable !=
        (const HudUiWidget_FTable *)(&g_HudUiCommon_FTable)) {
        return 5;
    }

    if (dialog.nextEntryButton.base.ftable !=
        (const HudUiWidget_FTable *)(&g_HudUiCommon_FTable)) {
        return 6;
    }

    if (dialog.prevEntryButton.base.ftable !=
        (const HudUiWidget_FTable *)(&g_HudUiCommon_FTable)) {
        return 7;
    }

    if (dialog.gameNameInput.base.base.ftable !=
        (const HudUiWidget_FTable *)(&g_HudUiCommon_FTable)) {
        return 8;
    }

    if (dialog.gameNameInput.textInput.ftable != &g_HudUiTextInput_FTable) {
        return 9;
    }

    for (int i = 0; i < 9; ++i) {
        if (((const HudUiPanel *)(&dialog.entryWidgets[i]))->vtbl != &g_HudUiCommon_FTable) {
            return 10;
        }
    }

    return 0;
}

extern "C" int hud_ui_load_game_dialog_destructor_smoke(void) {
    static HudUiLoadGameDialog dialog;
    InitLoadGameDialogDestructorFixture(&dialog);

    dialog.Destructor();

    return LoadGameDialogDestroyedCheck(dialog);
}

extern "C" int hud_ui_load_game_dialog_scalar_deleting_destructor_smoke(void) {
    HudUiLoadGameDialog *dialog =
        static_cast<HudUiLoadGameDialog *>(::operator new(sizeof(HudUiLoadGameDialog)));
    InitLoadGameDialogDestructorFixture(dialog);

    HudUiLoadGameDialog *const returned = dialog->ScalarDeletingDestructor(0);
    int result = returned == dialog ? LoadGameDialogDestroyedCheck(*dialog) : 11;
    ::operator delete(dialog);

    HudUiLoadGameDialog *deleteDialog =
        static_cast<HudUiLoadGameDialog *>(::operator new(sizeof(HudUiLoadGameDialog)));
    InitLoadGameDialogDestructorFixture(deleteDialog);
    deleteDialog->ScalarDeletingDestructor(1);

    return result;
}

extern "C" int recoil_state_save_load_transition_on_try_become_current_smoke(void) {
    CodeFunctionPatch loadPatch{};
    if (!PatchFunctionJump(HudUiBackgroundLoadFromZrdProc(), FakeSaveGameInitLoadFromZrdProc(),
                           loadPatch)) {
        return 1;
    }

    RecoilStateSaveLoadTransition saveTransition = {};
    saveTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    saveTransition.m_capturePresentationMode = RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED;
    g_saveGameInitLoadCalls = 0;
    g_saveGameInitLoadArgsOk = false;
    g_saveLoadInitExpectedSectionName = "SAVE_GAME_DIALOG";
    const int saveResult = saveTransition.OnTryBecomeCurrent();
    HudUiSaveGameDialog *const saveDialog =
        (HudUiSaveGameDialog *)(static_cast<std::uintptr_t>(saveTransition.m_dialog));
    const bool saveOk = saveResult == 1 && saveDialog != nullptr &&
                        saveDialog->base.base.base.enabled == 1 &&
                        saveDialog->selectedEntryIndex == -1 &&
                        g_saveGameInitLoadCalls == 1 && g_saveGameInitLoadArgsOk;

    if (saveDialog != nullptr) {
        saveDialog->gameNameInput.Destructor();
        if (saveDialog->fileEntries.begin != nullptr) {
            ::operator delete(saveDialog->fileEntries.begin);
        }
        ::operator delete(saveDialog);
    }

    RecoilStateSaveLoadTransition loadTransition = {};
    loadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_LOAD;
    loadTransition.m_capturePresentationMode = RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED;
    g_saveGameInitLoadCalls = 0;
    g_saveGameInitLoadArgsOk = false;
    g_saveLoadInitExpectedSectionName = "LOAD_GAME_DIALOG";
    const int loadResult = loadTransition.OnTryBecomeCurrent();
    HudUiLoadGameDialog *const loadDialog =
        (HudUiLoadGameDialog *)(static_cast<std::uintptr_t>(loadTransition.m_dialog));
    const bool loadOk = loadResult == 1 && loadDialog != nullptr &&
                        loadDialog->base.base.base.enabled == 1 &&
                        loadDialog->selectedEntryIndex == 0 &&
                        g_saveGameInitLoadCalls == 1 && g_saveGameInitLoadArgsOk;

    if (loadDialog != nullptr) {
        loadDialog->gameNameInput.Destructor();
        if (loadDialog->fileEntries.begin != nullptr) {
            ::operator delete(loadDialog->fileEntries.begin);
        }
        ::operator delete(loadDialog);
    }

    RestoreFunctionPatch(loadPatch);
    return saveOk && loadOk ? 0 : 2;
}

struct TestSaveLoadUpdateDialog;

struct TestSaveLoadUpdateDialogVtable {
    std::uintptr_t Update;
};

struct TestSaveLoadUpdateDialog {
    TestSaveLoadUpdateDialogVtable *vftable;
    int updateCalls;
    float lastDeltaSeconds;

    void RECOIL_THISCALL Update(float deltaSeconds) {
        ++updateCalls;
        lastDeltaSeconds = deltaSeconds;
    }
};

template <typename Method> std::uintptr_t TestSaveLoadMethodAddress(Method method) {
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(method));
    return address;
}

extern "C" int recoil_state_save_load_transition_on_update_should_quit_smoke(void) {
    zOpt_ViewRectSection **const oldWindowOption = g_zOpt_WindowSectionOption;
    const float oldFrameDelta = g_FrameDeltaTimeSec;

    CodeFunctionPatch pollPatch{};
    CodeFunctionPatch timePatch{};
    CodeFunctionPatch postprocessPatch{};
    CodeFunctionPatch unlockPatch{};
    CodeFunctionPatch adjustPatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zInput::PollActiveDevices),
                           reinterpret_cast<void *>(&FakeSaveLoadUpdatePollActiveDevices),
                           pollPatch)) {
        return 1;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&Time::Tick),
                           reinterpret_cast<void *>(&FakeSaveLoadUpdateTimeTick),
                           timePatch)) {
        RestoreFunctionPatch(pollPatch);
        return 2;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
                           reinterpret_cast<void *>(
                               &FakeSaveLoadUpdateRunPostprocessOnPrimaryBuffer),
                           postprocessPatch)) {
        RestoreFunctionPatch(timePatch);
        RestoreFunctionPatch(pollPatch);
        return 3;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
                           reinterpret_cast<void *>(
                               &FakeSaveLoadUpdateUnlockPrimarySurfaceState),
                           unlockPatch)) {
        RestoreFunctionPatch(postprocessPatch);
        RestoreFunctionPatch(timePatch);
        RestoreFunctionPatch(pollPatch);
        return 4;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::AdjustSurfacesIfEnabled),
                           reinterpret_cast<void *>(&FakeSaveLoadUpdateAdjustSurfaces),
                           adjustPatch)) {
        RestoreFunctionPatch(unlockPatch);
        RestoreFunctionPatch(postprocessPatch);
        RestoreFunctionPatch(timePatch);
        RestoreFunctionPatch(pollPatch);
        return 5;
    }

    zOpt_ViewRectSection windowSection = {};
    zOpt_ViewRectSection *windowPtr = &windowSection;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_FrameDeltaTimeSec = 0.0f;
    g_saveLoadUpdatePollCalls = 0;
    g_saveLoadUpdatePollDispatch = -1;
    g_saveLoadUpdateTimeCalls = 0;
    g_saveLoadUpdatePostprocessCalls = 0;
    g_saveLoadUpdateUnlockCalls = 0;
    g_saveLoadUpdateAdjustCalls = 0;
    g_saveLoadUpdateAdjustWait = 0;
    g_saveLoadUpdateAdjustBlit = 0;
    g_saveLoadUpdateAdjustSrc = nullptr;
    g_saveLoadUpdateAdjustDst = nullptr;

    RecoilStateSaveLoadTransition emptyTransition = {};
    const int emptyResult = emptyTransition.OnUpdateShouldQuit();
    const bool emptyOk = emptyResult == 0 && g_saveLoadUpdatePollCalls == 1 &&
                         g_saveLoadUpdatePollDispatch == 0 && g_saveLoadUpdateTimeCalls == 0 &&
                         g_saveLoadUpdatePostprocessCalls == 0 &&
                         g_saveLoadUpdateUnlockCalls == 0 && g_saveLoadUpdateAdjustCalls == 1 &&
                         g_saveLoadUpdateAdjustSrc == (zVidRect32 *)(&windowSection) &&
                         g_saveLoadUpdateAdjustDst == (zVidRect32 *)(&windowSection) &&
                         g_saveLoadUpdateAdjustWait == 1 && g_saveLoadUpdateAdjustBlit == 1;

    TestSaveLoadUpdateDialogVtable dialogVtable = {
        TestSaveLoadMethodAddress(&TestSaveLoadUpdateDialog::Update)};
    TestSaveLoadUpdateDialog dialog = {&dialogVtable, 0, 0.0f};
    g_saveLoadUpdatePollCalls = 0;
    g_saveLoadUpdateTimeCalls = 0;
    g_saveLoadUpdatePostprocessCalls = 0;
    g_saveLoadUpdateUnlockCalls = 0;
    g_saveLoadUpdateAdjustCalls = 0;
    g_saveLoadUpdateAdjustSrc = nullptr;
    g_saveLoadUpdateAdjustDst = nullptr;

    RecoilStateSaveLoadTransition dialogTransition = {};
    dialogTransition.m_dialog =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&dialog));
    const int dialogResult = dialogTransition.OnUpdateShouldQuit();
    const bool dialogOk = dialogResult == 0 && g_saveLoadUpdatePollCalls == 1 &&
                          g_saveLoadUpdateTimeCalls == 1 &&
                          g_saveLoadUpdatePostprocessCalls == 1 &&
                          g_saveLoadUpdateUnlockCalls == 1 &&
                          g_saveLoadUpdateAdjustCalls == 1 && dialog.updateCalls == 1 &&
                          dialog.lastDeltaSeconds > 0.249f &&
                          dialog.lastDeltaSeconds < 0.251f &&
                          g_saveLoadUpdateAdjustSrc == (zVidRect32 *)(&windowSection) &&
                          g_saveLoadUpdateAdjustDst == (zVidRect32 *)(&windowSection);

    RestoreFunctionPatch(adjustPatch);
    RestoreFunctionPatch(unlockPatch);
    RestoreFunctionPatch(postprocessPatch);
    RestoreFunctionPatch(timePatch);
    RestoreFunctionPatch(pollPatch);
    g_zOpt_WindowSectionOption = oldWindowOption;
    g_FrameDeltaTimeSec = oldFrameDelta;

    return emptyOk && dialogOk ? 0 : 6;
}

extern "C" int hud_ui_main_menu_dialog_save_load_checks_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;

    g_GameStateOrMapTable = nullptr;
    const bool noGameOk =
        HudUiMainMenuDialog::CanLoadGame() == 1 && HudUiMainMenuDialog::CanSaveGame() == 0;

    zInput_GameStateOrMapTablePartial gameState = {};
    g_GameStateOrMapTable = &gameState;
    const bool noPlayerOk =
        HudUiMainMenuDialog::CanLoadGame() == 1 && HudUiMainMenuDialog::CanSaveGame() == 1;

    zUtil_PlayerStateStorage playerState = {};
    gameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    playerState.environmentAttachmentActive = 0;
    const bool unblockedOk =
        HudUiMainMenuDialog::CanLoadGame() == 1 && HudUiMainMenuDialog::CanSaveGame() == 1;

    playerState.environmentAttachmentActive = 1;
    const bool blockedOk =
        HudUiMainMenuDialog::CanLoadGame() == 0 && HudUiMainMenuDialog::CanSaveGame() == 0;

    g_GameStateOrMapTable = oldGameState;
    return noGameOk && noPlayerOk && unblockedOk && blockedOk ? 0 : 1;
}

extern "C" int recoil_state_save_load_transition_queue_dialogs_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    const RecoilStateSaveLoadTransition oldTransition = g_RecoilStateSaveLoadTransition;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;

    zUtil_PlayerStateStorage playerState = {};
    zInput_GameStateOrMapTablePartial gameState = {};
    gameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    const RecoilPtr32 transitionState =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
            &g_RecoilStateSaveLoadTransition));

    auto resetHarness = [&]() {
        if (g_RecoilApp.m_stateQueue_118.m_itemCount != 0) {
            CleanupQueuedItems(g_RecoilApp.m_stateQueue_118);
        }
        g_RecoilApp = RecoilApp{};
        g_RecoilApp.m_currentStateIndex_0c8 = -1;
        g_RecoilStateSaveLoadTransition = RecoilStateSaveLoadTransition{};
        g_RecoilStateSaveLoadTransition.vftable =
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
        g_stateEnterCount = 0;
        playerState.environmentAttachmentActive = 0;
    };

    resetHarness();
    RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED);
    const bool saveOk =
        g_stateEnterCount == 1 &&
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode ==
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE &&
        IsSinglePushStateQueueItem(g_RecoilApp.m_stateQueue_118, transitionState, 0);

    resetHarness();
    g_RecoilStateSaveLoadTransition.m_capturePresentationMode =
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED;
    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_LOAD;
    playerState.environmentAttachmentActive = 1;
    RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED);
    const bool saveBlockedOk =
        g_stateEnterCount == 0 && g_RecoilApp.m_stateQueue_118.m_itemCount == 0 &&
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode ==
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_LOAD;

    resetHarness();
    g_RecoilStateSaveLoadTransition.m_capturePresentationMode =
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED;
    RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_STANDARD);
    const bool loadStandardOk =
        g_stateEnterCount == 1 &&
        g_RecoilStateSaveLoadTransition.m_transitionMode == RECOIL_SAVELOAD_MODE_STANDARD &&
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode ==
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_LOAD &&
        IsSinglePushStateQueueItem(g_RecoilApp.m_stateQueue_118, transitionState, 0);

    resetHarness();
    RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_QUICKLOAD);
    const bool loadQuickOk =
        g_stateEnterCount == 1 &&
        g_RecoilStateSaveLoadTransition.m_transitionMode == RECOIL_SAVELOAD_MODE_QUICKLOAD &&
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode ==
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_LOAD &&
        IsSinglePushStateQueueItem(g_RecoilApp.m_stateQueue_118, transitionState, 0);

    resetHarness();
    g_RecoilStateSaveLoadTransition.m_transitionMode = RECOIL_SAVELOAD_MODE_FADE;
    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    playerState.environmentAttachmentActive = 1;
    RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_QUICKLOAD);
    const bool loadBlockedOk =
        g_stateEnterCount == 0 && g_RecoilApp.m_stateQueue_118.m_itemCount == 0 &&
        g_RecoilStateSaveLoadTransition.m_transitionMode == RECOIL_SAVELOAD_MODE_FADE &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE;

    if (g_RecoilApp.m_stateQueue_118.m_itemCount != 0) {
        CleanupQueuedItems(g_RecoilApp.m_stateQueue_118);
    }
    g_RecoilApp = oldApp;
    g_RecoilStateSaveLoadTransition = oldTransition;
    g_GameStateOrMapTable = oldGameState;

    return saveOk && saveBlockedOk && loadStandardOk && loadQuickOk && loadBlockedOk ? 0 : 1;
}

extern "C" int recoil_app_load_zbd_and_setup_sensor_tracker_smoke(void) {
    InitTestRecoilAppVtable();
    g_startEngineCount = 0;
    g_shutdownEngineCount = 0;
    g_exitInstanceCount = 0;
    g_stateEnterCount = 0;
    g_stateExitCount = 0;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeTestZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;

    const int oldMissionId = g_HudSensorTracker.missionId;
    const int oldMissionFlags = g_HudSensorTracker.missionFlags;
    const int oldSkipIntroFmv = g_RecoilApp.m_skipIntroFmv;
    g_HudSensorTracker.missionFlags = 0;

    RecoilPtr32 frameWords[9]{};
    frameWords[8] = 0x24681357;

    TestRecoilApp app{};
    TestAppState startupState{};
    startupState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    app.vftable = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    app.m_pMainWnd = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(frameWords));
    app.m_pendingState_0c4 =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&startupState));
    app.m_currentStateIndex_0c8 = -1;
    g_startEngineResult = 1;

    const bool zbdPathOk =
        app.LoadZbdAndSetupSensorTracker(0, "custom.zbd", 3, 0x44) == 1 &&
        app.m_skipIntroFmv == 3 && CStringEquals(g_HudSensorTracker.zbdPath, "custom.zbd");
    CleanupSingleQueuedItem(app.m_stateQueue_118);
    const bool zbdRegisterOk = manager.sectionHandlerCount == 2;
    ClearTestRegisteredHandlers(sentinel);
    manager.sectionHandlerCount = 0;

    g_HudSensorTracker.missionFlags = 0;
    TestRecoilApp missionApp{};
    TestAppState missionStartupState{};
    missionStartupState.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    missionApp.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(g_testRecoilAppVtable));
    missionApp.m_pMainWnd = app.m_pMainWnd;
    missionApp.m_pendingState_0c4 =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&missionStartupState));
    missionApp.m_currentStateIndex_0c8 = -1;

    const bool missionOk =
        missionApp.LoadZbdAndSetupSensorTracker(9, nullptr, 4, 0x66) == 1 &&
        missionApp.m_skipIntroFmv == 4 && g_HudSensorTracker.missionId == 9 &&
        g_HudSensorTracker.missionFlags == 0x66 && CStringIsEmpty(g_HudSensorTracker.zbdPath);
    CleanupSingleQueuedItem(missionApp.m_stateQueue_118);
    const bool missionRegisterOk = manager.sectionHandlerCount == 2;

    ClearTestRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = nullptr;
    g_HudSensorTracker.missionId = oldMissionId;
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_RecoilApp.m_skipIntroFmv = oldSkipIntroFmv;
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
    if (g_startEngineCount != 2) {
        return 5;
    }
    return 0;
}

extern "C" int recoil_app_initialize_display_failure_smoke(void) {
    static std::int32_t modeIndex = 3;
    static std::int32_t fullscreen = 1;
    static std::int32_t hwApi = 0;
    static std::int32_t acceleration = 1;

    ZOPT_VIDEO_MODE = &modeIndex;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    g_zVideo_IsInitialized = 1;

    const std::int32_t result = RecoilApp::InitializeDisplay(0x12345678);
    g_zVideo_IsInitialized = 0;
    return result == 0 ? 0 : 1;
}

extern "C" int recoil_app_shutdown_subsystems_smoke(void) {
    g_zEffectAnim_EnableZarRegistration = 0;
    g_zInput_hWnd = nullptr;

    return RecoilApp::ShutdownSubsystems() == 0 ? 0 : 1;
}

extern "C" int recoil_app_on_idle_or_dispatch_smoke(void) {
    g_zSndCdFlags = 0;
    g_stateIdleCount = 0;
    g_stateIdleWParam = 0;
    g_stateIdleLParam = 0;

    RecoilApp app{};
    app.m_currentStateIndex_0c8 = -1;
    if (app.OnIdleOrDispatch(0x11, 0x22) != 0 || g_stateIdleCount != 0) {
        return 1;
    }

    TestAppState state{};
    state.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    app.m_currentStateIndex_0c8 = 0;
    app.m_stateStack_0d8[0] = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&state));

    if (app.OnIdleOrDispatch(0x33, 0x44) != 123) {
        return 2;
    }

    return g_stateIdleCount == 1 && g_stateIdleWParam == 0x33 && g_stateIdleLParam == 0x44 ? 0 : 3;
}

extern "C" int recoil_app_mfc_ole_module_destructor_smoke(void) {
    RecoilApp app{};
    app.MfcOleModuleConstructor();
    void *chunk = ::operator new(0x1000);
    auto *chunkList = static_cast<RecoilPtr32 *>(::operator new(sizeof(RecoilPtr32)));
    *chunkList = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(chunk));

    const RecoilPtr32 chunkValue =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(chunk));
    const RecoilPtr32 chunkListValue =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(chunkList));
    app.m_stateQueue_118.m_readBlock.m_chunkBegin = chunkValue;
    app.m_stateQueue_118.m_readBlock.m_chunkEnd = chunkValue + 0x1000;
    app.m_stateQueue_118.m_readBlock.m_cursor = chunkValue;
    app.m_stateQueue_118.m_readBlock.m_chunkPtrSlot = chunkListValue;
    app.m_stateQueue_118.m_writeBlock.m_chunkBegin = 0x22222222;
    app.m_stateQueue_118.m_writeBlock.m_chunkEnd = 0x33333333;
    app.m_stateQueue_118.m_writeBlock.m_cursor = 0x44444444;
    app.m_stateQueue_118.m_writeBlock.m_chunkPtrSlot = 0x55555555;
    app.m_stateQueue_118.m_chunkPtrList = chunkListValue;
    app.m_stateQueue_118.m_itemCount = 1;

    app.MfcOleModuleDestructor();
    if (app.m_stateQueue_118.m_itemCount != 0) {
        return 1;
    }

    const auto *const readBytes =
        reinterpret_cast<const unsigned char *>(&app.m_stateQueue_118.m_readBlock);
    const auto *const writeBytes =
        reinterpret_cast<const unsigned char *>(&app.m_stateQueue_118.m_writeBlock);
    for (std::size_t i = 0; i < sizeof(app.m_stateQueue_118.m_readBlock); ++i) {
        if (readBytes[i] != 0 || writeBytes[i] != 0) {
            return 2;
        }
    }

    return 0;
}

extern "C" int recoil_app_mfc_ole_module_scalar_deleting_destructor_smoke(void) {
    auto *callerOwnedApp = new RecoilApp{};
    callerOwnedApp->MfcOleModuleConstructor();
    RecoilApp *callerOwnedReturned =
        callerOwnedApp->MfcOleModuleScalarDeletingDestructor(0);
    const bool callerOwnedOk = callerOwnedReturned == callerOwnedApp;
    ::operator delete(callerOwnedApp);

    auto *deletingApp = new RecoilApp{};
    deletingApp->MfcOleModuleConstructor();
    RecoilApp *deletingReturned = deletingApp->MfcOleModuleScalarDeletingDestructor(1);
    return callerOwnedOk && deletingReturned == deletingApp ? 0 : 1;
}

extern "C" int recoil_app_play_state_constructor_smoke(void) {
    RecoilApp_PlayState playState{};
    playState.base.vftable = 0x11111111;
    playState.pWindowSection = 0x22222222;
    playState.pDisplaySection = 0x33333333;
    playState.pRenderSection = 0x44444444;
    playState.m_transitionScratch_10 = 0x55555555;
    playState.pPendingLoadGameStartPath = 0x66666666;

    RecoilApp_PlayState *returned = playState.Constructor();
    if (returned != &playState) {
        return 1;
    }

    if (playState.base.vftable != kRecoilApp_PlayState_VtblAddress ||
        playState.m_transitionScratch_10 != 0 || playState.pPendingLoadGameStartPath != 0) {
        return 2;
    }

    return playState.pWindowSection == 0x22222222 && playState.pDisplaySection == 0x33333333 &&
                   playState.pRenderSection == 0x44444444
               ? 0
               : 3;
}

extern "C" int recoil_app_play_state_on_wnd_activate_smoke(void) {
    HudLayoutBase *const oldLayout = g_HudUiMgrCurrentLayout;
    HudLayoutBase_FTable layoutTable{};
    layoutTable.OnActivated = TestPlayStateLayoutOnActivated;
    HudLayoutBase layout{&layoutTable};
    g_HudUiMgrCurrentLayout = &layout;
    g_playStateLayoutActivatedCount = 0;

    RecoilApp_PlayState playState{};
    playState.OnWndActivate(0);
    const bool inactiveOk = g_playStateLayoutActivatedCount == 0;

    playState.OnWndActivate(1);
    const bool activeOk = g_playStateLayoutActivatedCount == 1;

    g_HudUiMgrCurrentLayout = oldLayout;
    return inactiveOk && activeOk ? 0 : 1;
}

extern "C" int recoil_app_play_state_tick_and_render_frame_quit_smoke(void) {
    zEffectAnimEntry *const oldDebugEntry = g_Player_ActiveDebugScriptAsyncEntry;
    const int oldQuitAfterCredits = g_RecoilApp_QuitAfterCredits;
    zOpt_ViewRectSection **const oldRenderOption = g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection **const oldDisplayOption = g_zOpt_DisplaySectionOption;
    zOpt_ViewRectSection **const oldWindowOption = g_zOpt_WindowSectionOption;
    const unsigned char oldInputRegistry = g_zInput_DeviceRegistry;
    const unsigned char oldMouseFlags = g_zInputMouseFlags;
    const unsigned char oldJoystickFlags = g_zInputJoystickFlags;
    const short oldKeyboardPollRefCount = g_zInputKeyboardPollRefCount;
    const short oldMousePollRefCount = g_zInputMousePollRefCount;
    const short oldJoystickPollRefCount = g_zInputJoystickPollRefCount;
    zClass_TypeListLink *oldBucketHeads[6] = {};

    for (int i = 0; i < 6; ++i) {
        oldBucketHeads[i] = *g_zClassCallbackPriorityHeadSlotPtrs[i];
        *g_zClassCallbackPriorityHeadSlotPtrs[i] = nullptr;
    }

    zOpt_ViewRectSection renderSection = {};
    zOpt_ViewRectSection displaySection = {};
    zOpt_ViewRectSection windowSection = {};
    zOpt_ViewRectSection *renderPtr = &renderSection;
    zOpt_ViewRectSection *displayPtr = &displaySection;
    zOpt_ViewRectSection *windowPtr = &windowSection;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_Player_ActiveDebugScriptAsyncEntry = nullptr;
    g_RecoilApp_QuitAfterCredits = 1;
    g_zInput_DeviceRegistry = 0;
    g_zInputMouseFlags = 0;
    g_zInputJoystickFlags = 0;
    g_zInputKeyboardPollRefCount = 0;
    g_zInputMousePollRefCount = 0;
    g_zInputJoystickPollRefCount = 0;

    RecoilApp_PlayState playState{};
    const int result = playState.TickAndRenderFrame(1);
    const bool ok =
        result == 1 &&
        playState.pRenderSection ==
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&renderSection)) &&
        playState.pDisplaySection ==
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&displaySection)) &&
        playState.pWindowSection ==
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&windowSection));

    g_Player_ActiveDebugScriptAsyncEntry = oldDebugEntry;
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    g_zOpt_RenderSectionOption = oldRenderOption;
    g_zOpt_DisplaySectionOption = oldDisplayOption;
    g_zOpt_WindowSectionOption = oldWindowOption;
    g_zInput_DeviceRegistry = oldInputRegistry;
    g_zInputMouseFlags = oldMouseFlags;
    g_zInputJoystickFlags = oldJoystickFlags;
    g_zInputKeyboardPollRefCount = oldKeyboardPollRefCount;
    g_zInputMousePollRefCount = oldMousePollRefCount;
    g_zInputJoystickPollRefCount = oldJoystickPollRefCount;
    for (int i = 0; i < 6; ++i) {
        *g_zClassCallbackPriorityHeadSlotPtrs[i] = oldBucketHeads[i];
    }

    return ok ? 0 : 1;
}

extern "C" int recoil_app_play_state_on_update_should_quit_transition_smoke(void) {
    RecoilApp oldApp = g_RecoilApp;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const int oldQuitAfterCredits = g_RecoilApp_QuitAfterCredits;
    zEffectAnimEntry *const oldDebugEntry = g_Player_ActiveDebugScriptAsyncEntry;
    int *const oldMuteOption = ZOPT_MUTE_SOUND;
    zOpt_ViewRectSection **const oldRenderOption = g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection **const oldDisplayOption = g_zOpt_DisplaySectionOption;
    zOpt_ViewRectSection **const oldWindowOption = g_zOpt_WindowSectionOption;
    const unsigned char oldInputRegistry = g_zInput_DeviceRegistry;
    const unsigned char oldMouseFlags = g_zInputMouseFlags;
    const unsigned char oldJoystickFlags = g_zInputJoystickFlags;
    const short oldKeyboardPollRefCount = g_zInputKeyboardPollRefCount;
    const short oldMousePollRefCount = g_zInputMousePollRefCount;
    const short oldJoystickPollRefCount = g_zInputJoystickPollRefCount;
    const int oldHotkeyEnabled = g_zVideo_SoftwareModeHotkeyEnabled;
    const int oldAdjustDisableGate = g_zVideo_AdjustSurfacesDisableGate;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const float oldMaximumDelta = g_Time_MaximumDeltaTimeSec;
    const int oldDeltaClamp = g_Time_DeltaTimeClampEnabled;
    const float oldCurrentTime = g_Time_CurrentTimeSec;
    const float oldNewTime = g_Time_NewTimeSec;
    const float oldTimeScale = g_Time_TimeScaleFactor;
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const float oldUnscaledDelta = g_Time_UnscaledDeltaTimeSec;
    const float oldUnscaledAccumulated = g_Time_UnscaledAccumulatedTimeSec;
    HudLayoutBase *const oldLayout = g_HudUiMgrCurrentLayout;
    const int oldOverlayEnabled = zRndr::g_overlayBlendEnabled;
    const unsigned int oldOverlayColor = zRndr::g_overlayBlendPackedColor16;
    const double oldOverlayAlpha = zRndr::g_overlayBlendAlpha;
    zClass_TypeListLink *oldBucketHeads[6] = {};

    for (int i = 0; i < 6; ++i) {
        oldBucketHeads[i] = *g_zClassCallbackPriorityHeadSlotPtrs[i];
        *g_zClassCallbackPriorityHeadSlotPtrs[i] = nullptr;
    }

    zOpt_ViewRectSection renderSection = {};
    zOpt_ViewRectSection displaySection = {};
    zOpt_ViewRectSection windowSection = {};
    zOpt_ViewRectSection *renderPtr = &renderSection;
    zOpt_ViewRectSection *displayPtr = &displaySection;
    zOpt_ViewRectSection *windowPtr = &windowSection;
    zUtil_SaveGameState saveState{};
    zUtil_PlayerStateStorage playerState{};
    HudLayoutBase_FTable layoutTable{};
    HudLayoutBase layout{&layoutTable};
    int muteOption = 1;

    saveState.playerState = &playerState;
    layoutTable.OnActivated = TestPlayStateLayoutOnActivated;
    g_HudUiMgrCurrentLayout = &layout;
    g_playStateLayoutActivatedCount = 0;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    ZOPT_MUTE_SOUND = &muteOption;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    g_Player_ActiveDebugScriptAsyncEntry = nullptr;
    g_RecoilApp_QuitAfterCredits = 1;
    g_RecoilApp.m_transitionFadeTimer150 = 0.05f;
    g_zInput_DeviceRegistry = 0;
    g_zInputMouseFlags = 0;
    g_zInputJoystickFlags = 0;
    g_zInputKeyboardPollRefCount = 0;
    g_zInputMousePollRefCount = 0;
    g_zInputJoystickPollRefCount = 0;
    g_zVideo_SoftwareModeHotkeyEnabled = 1;
    g_zVideo_AdjustSurfacesDisableGate = 1;
    g_zVideo_ActiveRendererPath = 0;
    g_Time_MaximumDeltaTimeSec = 0.125f;
    g_Time_DeltaTimeClampEnabled = 1;
    g_Time_CurrentTimeSec = 0.0f;
    g_Time_TimeScaleFactor = 1.0f;
    g_FrameDeltaTimeSec = 0.0f;
    zRndr::g_overlayBlendEnabled = 0;
    zRndr::g_overlayBlendPackedColor16 = 0xffffffffU;
    zRndr::g_overlayBlendAlpha = 0.0;

    RecoilApp_PlayState playState{};
    const int result = playState.OnUpdateShouldQuit();
    const bool overlayAlphaOk =
        zRndr::g_overlayBlendAlpha > 0.049 && zRndr::g_overlayBlendAlpha < 0.051;
    const bool ok =
        result == 0 && g_zVideo_SoftwareModeHotkeyEnabled == 0 && muteOption == 0 &&
        g_playStateLayoutActivatedCount == 1 && playerState.transitionDamageSuppressed == 0 &&
        g_RecoilApp.m_transitionFadeTimer150 <= 0.0f &&
        playState.pWindowSection ==
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&windowSection)) &&
        zRndr::g_overlayBlendEnabled == 1 && zRndr::g_overlayBlendPackedColor16 == 0 &&
        overlayAlphaOk;

    g_RecoilApp = oldApp;
    g_GameStateOrMapTable = oldGameState;
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    g_Player_ActiveDebugScriptAsyncEntry = oldDebugEntry;
    ZOPT_MUTE_SOUND = oldMuteOption;
    g_zOpt_RenderSectionOption = oldRenderOption;
    g_zOpt_DisplaySectionOption = oldDisplayOption;
    g_zOpt_WindowSectionOption = oldWindowOption;
    g_zInput_DeviceRegistry = oldInputRegistry;
    g_zInputMouseFlags = oldMouseFlags;
    g_zInputJoystickFlags = oldJoystickFlags;
    g_zInputKeyboardPollRefCount = oldKeyboardPollRefCount;
    g_zInputMousePollRefCount = oldMousePollRefCount;
    g_zInputJoystickPollRefCount = oldJoystickPollRefCount;
    g_zVideo_SoftwareModeHotkeyEnabled = oldHotkeyEnabled;
    g_zVideo_AdjustSurfacesDisableGate = oldAdjustDisableGate;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_Time_MaximumDeltaTimeSec = oldMaximumDelta;
    g_Time_DeltaTimeClampEnabled = oldDeltaClamp;
    g_Time_CurrentTimeSec = oldCurrentTime;
    g_Time_NewTimeSec = oldNewTime;
    g_Time_TimeScaleFactor = oldTimeScale;
    g_FrameDeltaTimeSec = oldFrameDelta;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_Time_UnscaledDeltaTimeSec = oldUnscaledDelta;
    g_Time_UnscaledAccumulatedTimeSec = oldUnscaledAccumulated;
    g_HudUiMgrCurrentLayout = oldLayout;
    zRndr::g_overlayBlendEnabled = oldOverlayEnabled;
    zRndr::g_overlayBlendPackedColor16 = oldOverlayColor;
    zRndr::g_overlayBlendAlpha = oldOverlayAlpha;
    for (int i = 0; i < 6; ++i) {
        *g_zClassCallbackPriorityHeadSlotPtrs[i] = oldBucketHeads[i];
    }

    return ok ? 0 : 1;
}

extern "C" int recoil_app_play_state_on_resume_cd_disabled_smoke(void) {
    int *const oldCdAudioOption = ZOPT_SOUND_CDAUDIO;
    const std::int32_t oldCdFlags = g_zSndCdFlags;
    int cdAudioOption = 0;

    ZOPT_SOUND_CDAUDIO = &cdAudioOption;
    g_zSndCdFlags = 0x12345678;

    RecoilApp_PlayState playState{};
    playState.OnResume(0x55);

    const bool ok = cdAudioOption == 0 && g_zSndCdFlags == 0x12345678;

    ZOPT_SOUND_CDAUDIO = oldCdAudioOption;
    g_zSndCdFlags = oldCdFlags;
    return ok ? 0 : 1;
}

extern "C" int recoil_app_play_state_on_deactivate_skip_gameplay_smoke(void) {
    RecoilApp oldApp = g_RecoilApp;
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zArchiveList *const oldCurrentArchive = g_zArchive_Current;
    int *const oldAccelerationOption = ZOPT_VIDEO_ACCELERATION;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const std::int32_t oldCdFlags = g_zSndCdFlags;
    const int oldHalfResMode = g_zVideo_HalfResAdjustMode;
    const int oldUseHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;

    int acceleration = 0;
    int networkEnabled = 1;
    zArchiveList mountedList = {};

    ZOPT_VIDEO_ACCELERATION = &acceleration;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zSndCdFlags = 0;
    g_zVideo_HalfResAdjustMode = ZVIDEO_HALFRES_ADJUST_DISABLED;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
    g_zArchive_MountedList = &mountedList;
    g_zArchive_Current = nullptr;

    RecoilApp_PlayState playState{};
    playState.OnDeactivate();

    const bool ok =
        g_RecoilApp.m_missionShutdownMode == RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY &&
        g_zVideo_HalfResAdjustMode == ZVIDEO_HALFRES_ADJUST_DISABLED &&
        mountedList.count == 0;

    g_RecoilApp = oldApp;
    g_zArchive_MountedList = oldMountedList;
    g_zArchive_Current = oldCurrentArchive;
    ZOPT_VIDEO_ACCELERATION = oldAccelerationOption;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zSndCdFlags = oldCdFlags;
    g_zVideo_HalfResAdjustMode = oldHalfResMode;
    g_zVideo_UseHalfResBackbuffer = oldUseHalfResBackbuffer;
    return ok ? 0 : 1;
}

extern "C" int recoil_app_fmv_state_constructor_smoke(void) {
    RecoilApp_AttractFmvState attract{};
    auto *returnedAttract = attract.Constructor();
    auto *attractScript = reinterpret_cast<zFMV_Script *>(attract.m_fmv_10);
    if (returnedAttract != &attract ||
        attract.base.vftable != kRecoilApp_AttractFmvState_VtblAddress ||
        attractScript->m_abortOnKey != 1 || attractScript->m_head != nullptr) {
        return 1;
    }

    RecoilApp_IntroFmvState intro{};
    auto *returnedIntro = intro.Constructor();
    auto *introScript = reinterpret_cast<zFMV_Script *>(intro.m_fmv_08);
    if (returnedIntro != &intro || intro.base.vftable != kRecoilApp_IntroFmvState_VtblAddress ||
        introScript->m_abortOnKey != 1 || introScript->m_tail != nullptr) {
        return 2;
    }

    RecoilApp_MissionFmvState mission{};
    auto *returnedMission = mission.Constructor();
    auto *missionScript = reinterpret_cast<zFMV_Script *>(mission.m_fmv_08);
    if (returnedMission != &mission ||
        mission.base.vftable != kRecoilApp_MissionFmvState_VtblAddress ||
        mission.m_missionId != 0 || mission.m_skipMissionFmv != 0 ||
        missionScript->m_abortOnKey != 1 || missionScript->m_cur != nullptr) {
        return 3;
    }

    return 0;
}

extern "C" int recoil_app_fmv_state_on_idle_or_dispatch_smoke(void) {
    RecoilApp_FmvState state{};
    return state.OnIdleOrDispatch(0x11111111, 0x22222222) == 1 ? 0 : 1;
}

extern "C" int recoil_app_intro_fmv_on_try_become_current_smoke(void) {
    const int oldSkipIntro = g_RecoilApp.m_skipIntroFmv;
    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    zOpt_ViewRectSection **const oldDisplayOption = g_zOpt_DisplaySectionOption;
    zOpt_ViewRectSection **const oldWindowOption = g_zOpt_WindowSectionOption;
    int *const oldStrideOption = ZOPT_VIDEO_STRIDE;

    zOpt_ViewRectSection displaySection = {};
    zOpt_ViewRectSection windowSection = {};
    displaySection.bitsPerPixel = 16;
    windowSection.x = 10;
    windowSection.y = 20;
    windowSection.rightExclusive = 330;
    windowSection.bottomExclusive = 220;
    windowSection.width = 320;
    windowSection.height = 200;
    zOpt_ViewRectSection *displayPtr = &displaySection;
    zOpt_ViewRectSection *windowPtr = &windowSection;
    int stride = 640;
    unsigned short pixels[320 * 2] = {};

    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;
    ZOPT_VIDEO_STRIDE = &stride;
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    zRndr::g_frameBuffer = nullptr;
    zRndr::g_activeRegionWidth = 0;
    zRndr::g_activeRegionHeight = 0;
    zRndr::g_bytesPerPixel = 0;
    zRndr::g_pitchBytes = 0;
    zRndr::g_videoStrideMirror0 = 0;
    zRndr::g_videoStrideMirror1 = 0;
    g_zVideo_FxSurfacePixels16 = nullptr;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;
    g_zVid_CachedClientRectUpdateMask = 0;
    g_RecoilApp.m_skipIntroFmv = 1;

    RecoilApp_IntroFmvState intro{};
    const int result = intro.OnTryBecomeCurrent();

    const bool ok =
        result == 1 && zRndr::g_frameBuffer == pixels && zRndr::g_activeRegionWidth == 320 &&
        zRndr::g_activeRegionHeight == 200 && zRndr::g_activeRegionRect.x == 10 &&
        zRndr::g_activeRegionRect.y == 20 && zRndr::g_activeRegionRect.right == 330 &&
        zRndr::g_activeRegionRect.bottom == 220 && zRndr::g_bytesPerPixel == 2 &&
        zRndr::g_pitchBytes == 640 && zRndr::g_videoStrideMirror0 == 640 &&
        zRndr::g_videoStrideMirror1 == 640 && g_zVideo_FxSurfacePixels16 == pixels &&
        g_zVideo_FxSurfaceWidth == 320 && g_zVideo_FxSurfaceHeight == 200 &&
        g_zVideo_FxSurfacePitchBytes == 640 && g_zVideo_FxSurfacePitchPixels16 == 320 &&
        g_zVid_CachedClientRectUpdateMask == 1;

    g_RecoilApp.m_skipIntroFmv = oldSkipIntro;
    g_RecoilApp_hWndMain = oldMainHwnd;
    g_zOpt_DisplaySectionOption = oldDisplayOption;
    g_zOpt_WindowSectionOption = oldWindowOption;
    ZOPT_VIDEO_STRIDE = oldStrideOption;
    return ok ? 0 : 1;
}

extern "C" int recoil_app_intro_fmv_on_update_should_quit_smoke(void) {
    RecoilApp oldApp = g_RecoilApp;

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex_0c8 = -1;
    g_RecoilApp.m_skipIntroFmv = 1;
    g_RecoilApp.m_missionFmvState_1d8.base.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    g_RecoilApp.m_mainMenuPrepState_1c8.base.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));

    RecoilApp_IntroFmvState intro{};
    if (intro.OnUpdateShouldQuit() != 0 || g_RecoilApp.m_stateQueue_118.m_itemCount != 1) {
        g_RecoilApp = oldApp;
        return 1;
    }

    RecoilApp_StateQueueItem *item = reinterpret_cast<RecoilApp_StateQueueItem *>(
        static_cast<std::uintptr_t>(*reinterpret_cast<RecoilPtr32 *>(
            static_cast<std::uintptr_t>(g_RecoilApp.m_stateQueue_118.m_writeBlock.m_cursor - 4))));
    const bool skipOk =
        item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent && item->m_param == 0 &&
        item->m_stateObj ==
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                &g_RecoilApp.m_missionFmvState_1d8.base));
    CleanupSingleQueuedItem(g_RecoilApp.m_stateQueue_118);

    std::memset(&g_RecoilApp.m_stateQueue_118, 0, sizeof(g_RecoilApp.m_stateQueue_118));
    g_RecoilApp.m_skipIntroFmv = 0;
    auto *script = reinterpret_cast<zFMV_Script *>(intro.m_fmv_08);
    script->m_cur = nullptr;

    if (intro.OnUpdateShouldQuit() != 0 || g_RecoilApp.m_stateQueue_118.m_itemCount != 1) {
        g_RecoilApp = oldApp;
        return 2;
    }

    item = reinterpret_cast<RecoilApp_StateQueueItem *>(
        static_cast<std::uintptr_t>(*reinterpret_cast<RecoilPtr32 *>(
            static_cast<std::uintptr_t>(g_RecoilApp.m_stateQueue_118.m_writeBlock.m_cursor - 4))));
    const bool finishedOk =
        item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent && item->m_param == 0 &&
        item->m_stateObj ==
            static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                &g_RecoilApp.m_mainMenuPrepState_1c8.base));
    CleanupSingleQueuedItem(g_RecoilApp.m_stateQueue_118);
    g_RecoilApp = oldApp;

    return skipOk && finishedOk ? 0 : 3;
}

extern "C" int recoil_app_intro_fmv_on_deactivate_smoke(void) {
    RecoilApp_IntroFmvState intro{};
    auto *script = reinterpret_cast<zFMV_Script *>(intro.m_fmv_08);
    auto *action = new zFMV_Action{};
    action->vftable = &g_zFMV_ActionBase_Vtable;
    action->next = nullptr;
    script->m_head = action;
    script->m_tail = action;
    script->m_cur = action;

    intro.OnDeactivate();
    return script->m_head == nullptr && script->m_tail == nullptr && script->m_cur == nullptr ? 0
                                                                                              : 1;
}

extern "C" int recoil_app_main_menu_prep_on_try_become_current_smoke(void) {
    unsigned short pixels[320 * 2] = {};

    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    g_zVideo_FxSurfacePixels16 = nullptr;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;

    RecoilApp_MainMenuPrepState state{};
    state.m_stateData04 = 0x12345678;

    const int result = state.OnTryBecomeCurrent();
    return result == 1 && state.m_stateData04 == 0 && g_zVideo_FxSurfacePixels16 == pixels &&
                   g_zVideo_FxSurfaceWidth == 320 && g_zVideo_FxSurfaceHeight == 200 &&
                   g_zVideo_FxSurfacePitchBytes == 640 && g_zVideo_FxSurfacePitchPixels16 == 320
               ? 0
               : 1;
}

extern "C" int recoil_app_main_menu_prep_on_update_should_quit_smoke(void) {
    RecoilApp oldApp = g_RecoilApp;
    RecoilStateMainMenuTransition oldTransition = g_RecoilState_MainMenuTransition;

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex_0c8 = -1;
    g_stateEnterCount = 0;
    g_RecoilState_MainMenuTransition.m_entryRoute = static_cast<RecoilMainMenuEntryRoute>(7);
    g_RecoilState_MainMenuTransition.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));

    RecoilApp_MainMenuPrepState state{};
    const int result = state.OnUpdateShouldQuit();

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        RecoilApp_StateQueueItem *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(
                *reinterpret_cast<RecoilPtr32 *>(
                    static_cast<std::uintptr_t>(queue.m_writeBlock.m_cursor - 4))));
        itemOk = item->m_kind == RecoilApp_StateQueueKind_PushState && item->m_param == 0 &&
                 item->m_stateObj == static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                                         &g_RecoilState_MainMenuTransition));
        CleanupSingleQueuedItem(queue);
    }

    const bool ok =
        result == 0 &&
        g_RecoilState_MainMenuTransition.m_entryRoute == RECOIL_MAINMENU_ROUTE_FRONTEND &&
        g_stateEnterCount == 1 && itemOk;

    g_RecoilApp = oldApp;
    g_RecoilState_MainMenuTransition = oldTransition;
    return ok ? 0 : 1;
}

extern "C" int recoil_app_attract_fmv_on_try_become_current_smoke(void) {
    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    const int oldReloadMode = g_RecoilApp_AttractFmvReloadMode;
    HINSTANCE const instance = GetModuleHandleA(nullptr);
    HWND const hwnd = CreateWindowExA(0, "STATIC", "recoil-attract-fmv-on-try-smoke",
                                      WS_POPUP, 0, 0, 160, 90, nullptr, nullptr, instance,
                                      nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    unsigned short pixels[320 * 2] = {};
    g_RecoilApp_hWndMain = hwnd;
    g_RecoilApp_AttractFmvReloadMode = 1;
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 320;
    g_zVideo_PrimarySurfaceState.height = 200;
    g_zVideo_PrimarySurfaceState.pitch = 640;
    g_zVideo_FxSurfacePixels16 = nullptr;
    g_zVideo_FxSurfaceWidth = 0;
    g_zVideo_FxSurfaceHeight = 0;
    g_zVideo_FxSurfacePitchBytes = 0;
    g_zVideo_FxSurfacePitchPixels16 = 0;

    RecoilApp_AttractFmvState state{};
    state.Constructor();
    state.m_clientRect_30[0] = -1;
    state.m_clientRect_30[1] = -1;
    state.m_clientRect_30[2] = -1;
    state.m_clientRect_30[3] = -1;

    const int result = state.OnTryBecomeCurrent();
    auto *const script = reinterpret_cast<zFMV_Script *>(state.m_fmv_10);
    const bool ok =
        result == 1 && g_RecoilApp_AttractFmvReloadMode == 0 && script->m_hWnd == hwnd &&
        state.m_clientRect_30[0] == 0 && state.m_clientRect_30[1] == 0 &&
        state.m_clientRect_30[2] > 0 && state.m_clientRect_30[3] > 0 &&
        g_zVideo_FxSurfacePixels16 == pixels && g_zVideo_FxSurfaceWidth == 320 &&
        g_zVideo_FxSurfaceHeight == 200 && g_zVideo_FxSurfacePitchBytes == 640 &&
        g_zVideo_FxSurfacePitchPixels16 == 320;

    state.Destructor();
    DestroyWindow(hwnd);
    g_RecoilApp_hWndMain = oldMainHwnd;
    g_RecoilApp_AttractFmvReloadMode = oldReloadMode;
    return ok ? 0 : 2;
}

extern "C" int recoil_app_attract_fmv_on_update_should_quit_smoke(void) {
    RecoilApp oldApp = g_RecoilApp;

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    g_RecoilApp.m_currentStateIndex_0c8 = -1;
    g_RecoilApp.m_mainMenuPrepState_1c8.base.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_testAppStateVtable));
    g_stateEnterCount = 0;

    RecoilApp_AttractFmvState state{};
    auto *const script = reinterpret_cast<zFMV_Script *>(state.m_fmv_10);
    script->m_cur = nullptr;

    const int result = state.OnUpdateShouldQuit();
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue_118;
    bool itemOk = false;
    if (queue.m_itemCount == 1) {
        auto *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(
                *reinterpret_cast<RecoilPtr32 *>(
                    static_cast<std::uintptr_t>(queue.m_writeBlock.m_cursor - 4))));
        itemOk = item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent && item->m_param == 0 &&
                 item->m_stateObj == static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(
                                         &g_RecoilApp.m_mainMenuPrepState_1c8.base));
        CleanupSingleQueuedItem(queue);
    }

    const bool ok = result == 0 && g_stateEnterCount == 1 && itemOk;
    g_RecoilApp = oldApp;
    return ok ? 0 : 1;
}

extern "C" int recoil_app_attract_fmv_on_deactivate_smoke(void) {
    RecoilApp_AttractFmvState state{};
    auto *const script = reinterpret_cast<zFMV_Script *>(state.m_fmv_10);
    zFMV_Action action1{};
    zFMV_Action action2{};

    action1.next = &action2;
    action2.next = nullptr;
    script->m_head = &action1;
    script->m_tail = &action2;
    script->m_cur = &action2;

    state.OnDeactivate();
    return script->m_head == &action1 && script->m_tail == &action2 && script->m_cur == &action1 &&
                   action1.next == &action2 && action2.next == nullptr
               ? 0
               : 1;
}

extern "C" int recoil_app_constructor_destructor_smoke(void) {
    RecoilApp app{};
    RecoilApp *returned = app.Constructor();
    if (returned != &app || app.vftable != kRecoilApp_VtblAddress ||
        app.m_attractFmvState_160.base.vftable != kRecoilApp_AttractFmvState_VtblAddress ||
        app.m_introFmvState_1a0.base.vftable != kRecoilApp_IntroFmvState_VtblAddress ||
        app.m_mainMenuPrepState_1c8.base.vftable != kRecoilApp_MainMenuPrepState_VtblAddress ||
        app.m_leaveNetworkState_1d0.base.vftable != kRecoilApp_LeaveNetworkState_VtblAddress ||
        app.m_missionFmvState_1d8.base.vftable != kRecoilApp_MissionFmvState_VtblAddress ||
        app.m_playState_208.base.vftable != kRecoilApp_PlayState_VtblAddress ||
        app.m_mpExitDialogState_220.base.vftable != kRecoilApp_MpExitDialogState_VtblAddress ||
        app.m_transitionFadeTimer150 != 0.0f) {
        return 1;
    }

    app.Destructor();
    if (app.m_attractFmvState_160.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_introFmvState_1a0.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_mainMenuPrepState_1c8.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_leaveNetworkState_1d0.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_missionFmvState_1d8.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_playState_208.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_mpExitDialogState_220.base.vftable != kRecoilStateBase_VtblAddress) {
        return 2;
    }

    return 0;
}

extern "C" int recoil_app_istate_destructor_smoke(void) {
    RecoilApp_IState state{0x12345678};
    state.Destructor();

    return state.vftable == kRecoilStateBase_VtblAddress ? 0 : 1;
}

extern "C" int recoil_app_fmv_state_destructor_smoke(void) {
    RecoilApp_AttractFmvState attract{};
    attract.base.vftable = 0x11111111;
    auto *attractScript = reinterpret_cast<zFMV_Script *>(attract.m_fmv_10);
    attractScript->m_fmvPath = static_cast<char *>(std::malloc(4));
    if (attractScript->m_fmvPath == nullptr) {
        return 1;
    }

    attract.Destructor();
    if (attract.base.vftable != kRecoilStateBase_VtblAddress ||
        attractScript->m_fmvPath != nullptr) {
        return 2;
    }

    RecoilApp_IntroFmvState intro{};
    intro.base.vftable = 0x22222222;
    auto *introScript = reinterpret_cast<zFMV_Script *>(intro.m_fmv_08);
    introScript->m_fmvPath = static_cast<char *>(std::malloc(4));
    if (introScript->m_fmvPath == nullptr) {
        return 3;
    }

    intro.Destructor();
    return intro.base.vftable == kRecoilStateBase_VtblAddress && introScript->m_fmvPath == nullptr
               ? 0
               : 4;
}

extern "C" int recoil_state_credits_destructor_smoke(void) {
    RecoilStateCredits state{};
    TestCreditsPanel panel{};

    state.vftable = 0x11111111;
    state.dialog = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&panel));

    state.~RecoilStateCredits();

    if (state.vftable != kRecoilStateBase_VtblAddress || state.dialog != 0) {
        return 1;
    }
    if (panel.setEnabledCount != 1 || panel.lastEnabled != 0) {
        return 2;
    }
    if (panel.scalarDeletingCount != 1 || panel.lastScalarDeletingFlags != 1) {
        return 3;
    }

    state.vftable = 0x22222222;
    state.dialog = 0;
    state.~RecoilStateCredits();

    return state.vftable == kRecoilStateBase_VtblAddress ? 0 : 4;
}

extern "C" int recoil_state_confirm_quit_destructor_smoke(void) {
    RecoilStateConfirmQuit constructed{};
    constructed.vftable = 0x11111111;
    constructed.m_dialog = 0x22222222;
    RecoilStateConfirmQuit *const constructedReturned = constructed.Constructor();
    if (constructedReturned != &constructed || constructed.vftable == 0 ||
        constructed.m_dialog != 0) {
        return 7;
    }

    RecoilStateConfirmQuit state{};
    TestConfirmQuitDialog dialog{};

    state.vftable = 0x11111111;
    state.m_dialog = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&dialog));

    state.~RecoilStateConfirmQuit();

    if (state.vftable != kRecoilStateBase_VtblAddress || state.m_dialog != 0) {
        return 1;
    }
    if (dialog.setEnabledCount != 1 || dialog.lastEnabled != 0) {
        return 2;
    }
    if (dialog.scalarDeletingCount != 1 || dialog.lastScalarDeletingFlags != 1) {
        return 3;
    }

    state.vftable = 0x22222222;
    state.m_dialog = 0;
    state.~RecoilStateConfirmQuit();

    if (state.vftable != kRecoilStateBase_VtblAddress) {
        return 4;
    }

    RecoilStateConfirmQuit scalarState{};
    RecoilStateConfirmQuit *const returned = scalarState.ScalarDeletingDestructor(0);
    if (returned != &scalarState || scalarState.vftable != kRecoilStateBase_VtblAddress) {
        return 5;
    }

    auto *const deletingState = new RecoilStateConfirmQuit{};
    RecoilStateConfirmQuit *const deletingReturned =
        deletingState->ScalarDeletingDestructor(1);
    if (deletingReturned != deletingState) {
        return 6;
    }

    CodeFunctionPatch postprocessPatch{};
    CodeFunctionPatch blitPatch{};
    CodeFunctionPatch unlockPatch{};
    ImportFunctionPatch sleepPatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
                           reinterpret_cast<void *>(
                               &FakeConfirmQuitRunPostprocessOnPrimaryBuffer),
                           postprocessPatch)) {
        return 8;
    }

    void (RECOIL_THISCALL HudUiDialogController::*blitMember)() =
        &HudUiDialogController::BlitOwnedSurfaceToPrimary;
    void (RECOIL_THISCALL FakeConfirmQuitBlitThunk::*fakeBlitMember)() =
        &FakeConfirmQuitBlitThunk::BlitOwnedSurfaceToPrimary;
    if (!PatchFunctionJump(reinterpret_cast<void *>(TestSaveLoadMethodAddress(blitMember)),
                           reinterpret_cast<void *>(TestSaveLoadMethodAddress(fakeBlitMember)),
                           blitPatch)) {
        RestoreFunctionPatch(postprocessPatch);
        return 9;
    }

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
                           reinterpret_cast<void *>(&FakeConfirmQuitUnlockPrimarySurfaceState),
                           unlockPatch)) {
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(postprocessPatch);
        return 10;
    }

    if (!PatchImportByName("KERNEL32.dll", "Sleep",
                           reinterpret_cast<void *>(&FakeConfirmQuitSleep), sleepPatch)) {
        RestoreFunctionPatch(unlockPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(postprocessPatch);
        return 11;
    }

    g_confirmQuitPostprocessCalls = 0;
    g_confirmQuitBlitCalls = 0;
    g_confirmQuitUnlockCalls = 0;
    g_confirmQuitSleepCalls = 0;
    g_confirmQuitSleepMilliseconds = 0;
    dialog.setEnabledCount = 0;
    dialog.lastEnabled = -1;
    dialog.scalarDeletingCount = 0;
    dialog.lastScalarDeletingFlags = 0;

    state.m_dialog = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&dialog));
    state.OnDeactivate();

    const bool deactivateOk =
        state.m_dialog == 0 && dialog.setEnabledCount == 1 && dialog.lastEnabled == 0 &&
        dialog.scalarDeletingCount == 1 && dialog.lastScalarDeletingFlags == 1 &&
        g_confirmQuitPostprocessCalls == 1 && g_confirmQuitBlitCalls == 1 &&
        g_confirmQuitUnlockCalls == 1 && g_confirmQuitSleepCalls == 1 &&
        g_confirmQuitSleepMilliseconds == 1000;

    state.m_dialog = 0;
    g_confirmQuitPostprocessCalls = 0;
    g_confirmQuitBlitCalls = 0;
    g_confirmQuitUnlockCalls = 0;
    g_confirmQuitSleepCalls = 0;
    state.OnDeactivate();
    const bool nullDeactivateOk =
        g_confirmQuitPostprocessCalls == 0 && g_confirmQuitBlitCalls == 0 &&
        g_confirmQuitUnlockCalls == 0 && g_confirmQuitSleepCalls == 0;

    RestoreImportPatch(sleepPatch);
    RestoreFunctionPatch(unlockPatch);
    RestoreFunctionPatch(blitPatch);
    RestoreFunctionPatch(postprocessPatch);

    if (!deactivateOk) {
        return 12;
    }
    return nullDeactivateOk ? 0 : 13;
}

extern "C" int recoil_app_mission_fmv_state_destructor_smoke(void) {
    RecoilApp_MissionFmvState mission{};
    mission.base.vftable = 0x33333333;
    auto *missionScript = reinterpret_cast<zFMV_Script *>(mission.m_fmv_08);
    missionScript->m_fmvPath = static_cast<char *>(std::malloc(4));
    if (missionScript->m_fmvPath == nullptr) {
        return 1;
    }

    mission.Destructor();
    if (mission.base.vftable != kRecoilStateBase_VtblAddress ||
        missionScript->m_fmvPath != nullptr) {
        return 2;
    }

    auto *heapMission = new RecoilApp_MissionFmvState{};
    heapMission->base.vftable = 0x44444444;
    auto *returned = heapMission->ScalarDeletingDestructor(1);

    return returned == heapMission ? 0 : 3;
}

extern "C" int recoil_app_scalar_deleting_destructor_smoke(void) {
    RecoilApp app{};
    app.Constructor();
    RecoilApp *returnedApp = app.ScalarDeletingDestructor(0);
    if (returnedApp != &app || app.m_attractFmvState_160.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_introFmvState_1a0.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_mainMenuPrepState_1c8.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_leaveNetworkState_1d0.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_missionFmvState_1d8.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_playState_208.base.vftable != kRecoilStateBase_VtblAddress ||
        app.m_mpExitDialogState_220.base.vftable != kRecoilStateBase_VtblAddress) {
        return 1;
    }

    auto *deletingApp = new RecoilApp{};
    deletingApp->Constructor();
    RecoilApp *deletingReturnedApp = deletingApp->ScalarDeletingDestructor(1);
    if (deletingReturnedApp != deletingApp) {
        return 2;
    }

    auto *state = new RecoilApp_IState{0x11111111};
    RecoilApp_IState *returnedState = state->ScalarDeletingDestructor(1);
    if (returnedState != state) {
        return 3;
    }

    auto *attract = new RecoilApp_AttractFmvState{};
    attract->base.vftable = 0x22222222;
    auto *returnedAttract = attract->ScalarDeletingDestructor(1);
    if (returnedAttract != attract) {
        return 4;
    }

    auto *intro = new RecoilApp_IntroFmvState{};
    intro->base.vftable = 0x33333333;
    auto *returnedIntro = intro->ScalarDeletingDestructor(1);
    if (returnedIntro != intro) {
        return 5;
    }

    auto *mission = new RecoilApp_MissionFmvState{};
    mission->base.vftable = 0x44444444;
    RecoilApp_MissionFmvState *returnedMission = mission->ScalarDeletingDestructor(1);
    return returnedMission == mission ? 0 : 6;
}
