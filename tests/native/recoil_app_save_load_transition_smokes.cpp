// Checked-in focused native smoke translation unit, formerly extracted from recoil_app_message_map.cpp.
// Emits focused RecoilApp state smokes needed by functional manifests.

#include "Battlesport/recoil_app.h"

#include "Battlesport/briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/player.h"
#include "Battlesport/hud.h"
#include "Battlesport/about.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

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
extern "C" unsigned int g_HudUi_InvalidateMask;
BOOL __stdcall AfxWinInit(HINSTANCE instance, HINSTANCE previousInstance, LPSTR commandLine,
                               int showCommand);

struct RecoilStateCredits {
    RecoilPtr32 vftable;
    RecoilPtr32 dialog;

    RecoilStateCredits * Constructor();
    static void StaticInitAndRegisterAtExit();
    static void StaticInit();
    static void RegisterAtExit();
    void OnWndActivate(int activateCode);
    int OnTryBecomeCurrent();
    void OnDeactivate();
    ~RecoilStateCredits();
    static void QueuePush();
};

extern RecoilStateCredits g_RecoilStateCredits;

namespace {
int g_saveLoadDeactivateDestroyCalls;
bool g_saveLoadDeactivateDestroyNameOk;
int g_saveLoadDeactivateMuteCalls;
int g_saveLoadDeactivateMuteState;
int g_confirmQuitPostprocessCalls;
int g_confirmQuitBlitCalls;
int g_confirmQuitUnlockCalls;
int g_playStateLayoutActivatedCount;
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
int g_saveLoadUpdatePollCalls;
int g_saveLoadUpdatePollDispatch;
int g_saveLoadUpdateTimeCalls;
int g_saveLoadUpdatePostprocessCalls;
int g_saveLoadUpdateUnlockCalls;
int g_saveLoadUpdateAdjustCalls;
int g_saveLoadUpdateAdjustWait;
int g_saveLoadUpdateAdjustBlit;
zVidRect32 *g_saveLoadUpdateAdjustSrc;
zVidRect32 *g_saveLoadUpdateAdjustDst;
int g_saveGameInitLoadCalls;
bool g_saveGameInitLoadArgsOk;
const char *g_saveLoadInitExpectedSectionName;

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

struct TestSaveLoadTransitionDialog {
    virtual void Update(float) {}
    virtual void SetEnabled(int enabled) {
        ++setEnabledCount;
        lastEnabled = enabled;
    }
    virtual TestSaveLoadTransitionDialog * ScalarDeletingDestructor(unsigned int flags) {
        ++scalarDeletingCount;
        lastScalarDeletingFlags = flags;
        return this;
    }

    int setEnabledCount = 0;
    int lastEnabled = -1;
    int scalarDeletingCount = 0;
    unsigned int lastScalarDeletingFlags = 0;
};

struct TestSaveLoadPlayStateLayout : HudLayoutBase {
    void OnActivated() {
        ++g_playStateLayoutActivatedCount;
    }
};

struct TestAppState : RecoilApp_IState {
    void OnEnter() {
        ++g_stateEnterCount;
    }

    void OnExit() {
        ++g_stateExitCount;
    }

    std::int32_t OnTryBecomeCurrent() {
        ++g_stateTryBecomeCurrentCount;
        return g_stateTryBecomeCurrentResult;
    }

    std::int32_t OnUpdateShouldQuit() {
        ++g_stateUpdateShouldQuitCount;
        return g_stateUpdateShouldQuitResult;
    }

    void OnDeactivate() {
        ++g_stateDeactivateCount;
    }

    void OnSuspend(int param) {
        ++g_stateSuspendCount;
        g_stateSuspendParam = param;
    }

    void OnResume(int param) {
        ++g_stateResumeCount;
        g_stateResumeParam = param;
    }

    std::int32_t OnIdleOrDispatch(std::uint32_t wParam, std::uint32_t lParam) {
        ++g_stateIdleCount;
        g_stateIdleWParam = wParam;
        g_stateIdleLParam = lParam;
        return 123;
    }
};

void *ReadStateVtable(const RecoilApp_IState &state) {
    return *reinterpret_cast<void *const *>(
        const_cast<RecoilApp_IState *>(&state));
}

void WriteStateVtable(RecoilApp_IState &state, std::uintptr_t value) {
    *reinterpret_cast<std::uintptr_t *>(&state) = value;
}

void *RecoilAppIStateVtable() {
    RecoilApp_IState state{};
    return ReadStateVtable(state);
}

RecoilStateSaveLoadTransition *CallStateScalarDeletingDestructor(
    RecoilStateSaveLoadTransition *state, unsigned int flags) {
    using ScalarDeletingDestructor =
        RecoilStateSaveLoadTransition *(__thiscall *)(RecoilStateSaveLoadTransition *,
                                                      unsigned int);
    void **const vtable = *reinterpret_cast<void ***>(state);
    return reinterpret_cast<ScalarDeletingDestructor>(vtable[0])(state, flags);
}

struct FakeSaveGameInitLoadThunk {
    zReader::Node * LoadFromZrd(const char *zrdPath, const char *sectionName,
                                int capturePrimary);
};

zReader::Node *FakeSaveGameInitLoadThunk::LoadFromZrd(
    const char *zrdPath, const char *sectionName, int capturePrimary) {
    ++g_saveGameInitLoadCalls;
    g_saveGameInitLoadArgsOk =
        this != nullptr && zrdPath != nullptr && std::strcmp(zrdPath, "dialog.zrd") == 0 &&
        sectionName != nullptr && g_saveLoadInitExpectedSectionName != nullptr &&
        std::strcmp(sectionName, g_saveLoadInitExpectedSectionName) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *FakeSaveGameInitLoadFromZrdProc() {
    union MemberToFunction {
        zReader::Node *( FakeSaveGameInitLoadThunk::*member)(const char *, const char *, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &FakeSaveGameInitLoadThunk::LoadFromZrd;
    return thunk.function;
}

void *HudUiBackgroundLoadFromZrdProc() {
    union MemberToFunction {
        zReader::Node *( HudUiBackground::*member)(const char *, const char *, int);
        void *function;
    };

    MemberToFunction thunk{};
    thunk.member = &HudUiBackground::LoadFromZrd;
    return thunk.function;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    patch.address = static_cast<unsigned char *>(target);
    memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
        patch.address = 0;
        return false;
    }

    patch.address[0] = 0xe9;
    *reinterpret_cast<int *>(patch.address + 1) =
        static_cast<int>(static_cast<unsigned char *>(replacement) -
                         (patch.address + sizeof(patch.original)));

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }
    patch.address = 0;
}

int __fastcall FakeSaveLoadDeactivateDestroySampleSetByName(const char *setName) {
    ++g_saveLoadDeactivateDestroyCalls;
    g_saveLoadDeactivateDestroyNameOk =
        setName != nullptr && std::strcmp(setName, "DIALOG") == 0;
    return 1;
}

int __fastcall FakeSaveLoadDeactivateApplyMuteStateToActiveVoices(int enableMute) {
    ++g_saveLoadDeactivateMuteCalls;
    g_saveLoadDeactivateMuteState = enableMute;
    return 1;
}

int FakeConfirmQuitRunPostprocessOnPrimaryBuffer() {
    ++g_confirmQuitPostprocessCalls;
    return 0;
}

int FakeConfirmQuitUnlockPrimarySurfaceState() {
    ++g_confirmQuitUnlockCalls;
    return 0;
}

void __fastcall FakeSaveLoadUpdatePollActiveDevices(unsigned char dispatchCallbacks) {
    ++g_saveLoadUpdatePollCalls;
    g_saveLoadUpdatePollDispatch = dispatchCallbacks;
}

void FakeSaveLoadUpdateTimeTick() {
    ++g_saveLoadUpdateTimeCalls;
    g_FrameDeltaTimeSec = 0.25f;
}

int FakeSaveLoadUpdateRunPostprocessOnPrimaryBuffer() {
    ++g_saveLoadUpdatePostprocessCalls;
    return 0;
}

int FakeSaveLoadUpdateUnlockPrimarySurfaceState() {
    ++g_saveLoadUpdateUnlockCalls;
    return 0;
}

int __fastcall FakeSaveLoadUpdateAdjustSurfaces(zVidRect32 *srcRect, zVidRect32 *dstRect,
                                                int waitForPresent,
                                                int blitPrimaryToSwFirst) {
    ++g_saveLoadUpdateAdjustCalls;
    g_saveLoadUpdateAdjustSrc = srcRect;
    g_saveLoadUpdateAdjustDst = dstRect;
    g_saveLoadUpdateAdjustWait = waitForPresent;
    g_saveLoadUpdateAdjustBlit = blitPrimaryToSwFirst;
    return 0;
}

struct FakeConfirmQuitBlitThunk {
    void BlitOwnedSurfaceToPrimary() {
        ++g_confirmQuitBlitCalls;
    }
};

void __fastcall TestPlayStateLayoutOnActivated(HudLayoutBase *) {
    ++g_playStateLayoutActivatedCount;
}

void CleanupQueuedItems(RecoilApp_StateQueue &queue) {
    if (queue.m_chunkBaseList == 0) {
        queue = RecoilApp_StateQueue();
        return;
    }

    const int itemCount = queue.m_itemCount;
    RecoilApp_StateQueueItem **const firstSlot = queue.m_writeBlock.m_cursor - itemCount;
    for (int i = 0; i < itemCount; ++i) {
        ::operator delete(firstSlot[i]);
    }

    RecoilApp_StateQueueItem ***slot = queue.m_readBlock.m_chunkBaseSlot;
    while (slot <= queue.m_writeBlock.m_chunkBaseSlot) {
        ::operator delete(*slot);
        ++slot;
    }
    ::operator delete(queue.m_chunkBaseList);
    queue = RecoilApp_StateQueue();
}

bool IsSinglePushStateQueueItem(RecoilApp_StateQueue &queue, RecoilApp_IState *state,
                                int param) {
    if (queue.m_itemCount != 1 || queue.m_writeBlock.m_cursor == 0) {
        return false;
    }

    RecoilApp_StateQueueItem *const item = *(queue.m_writeBlock.m_cursor - 1);
    return item->m_type == 0 && item->m_kind == RecoilApp_StateQueueKind_PushState &&
           item->m_stateObj == state && item->m_param == param;
}
} // namespace

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
                        saveDialog->enabled == 1 &&
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
                        loadDialog->enabled == 1 &&
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

    void Update(float deltaSeconds) {
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
    zOpt_ViewRectSection **const oldWindowOption = g_zGame_Options_PointerCache.windowSection;
    const float oldFrameDelta = g_FrameDeltaTimeSec;

    CodeFunctionPatch pollPatch{};
    CodeFunctionPatch timePatch{};
    CodeFunctionPatch postprocessPatch{};
    CodeFunctionPatch unlockPatch{};
    CodeFunctionPatch adjustPatch{};
    CodeFunctionPatch dialogUpdatePatch{};

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

    void ( HudUiContainer::*updateMember)(float) = &HudUiContainer::UpdateAll;
    void ( TestSaveLoadUpdateDialog::*fakeUpdateMember)(float) =
        &TestSaveLoadUpdateDialog::Update;
    if (!PatchFunctionJump(reinterpret_cast<void *>(TestSaveLoadMethodAddress(updateMember)),
                           reinterpret_cast<void *>(TestSaveLoadMethodAddress(fakeUpdateMember)),
                           dialogUpdatePatch)) {
        RestoreFunctionPatch(dialogUpdatePatch);
    RestoreFunctionPatch(adjustPatch);
        RestoreFunctionPatch(unlockPatch);
        RestoreFunctionPatch(postprocessPatch);
        RestoreFunctionPatch(timePatch);
        RestoreFunctionPatch(pollPatch);
        return 7;
    }

    zOpt_ViewRectSection windowSection = {};
    zOpt_ViewRectSection *windowPtr = &windowSection;
    g_zGame_Options_PointerCache.windowSection = &windowPtr;
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

    RestoreFunctionPatch(dialogUpdatePatch);
    RestoreFunctionPatch(adjustPatch);
    RestoreFunctionPatch(unlockPatch);
    RestoreFunctionPatch(postprocessPatch);
    RestoreFunctionPatch(timePatch);
    RestoreFunctionPatch(pollPatch);
    g_zGame_Options_PointerCache.windowSection = oldWindowOption;
    g_FrameDeltaTimeSec = oldFrameDelta;

    return emptyOk && dialogOk ? 0 : 6;
}

extern "C" int recoil_state_save_load_transition_on_deactivate_smoke(void) {
    CodeFunctionPatch postprocessPatch{};
    CodeFunctionPatch blitPatch{};
    CodeFunctionPatch unlockPatch{};
    CodeFunctionPatch destroySampleSetPatch{};
    CodeFunctionPatch applyMutePatch{};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
                           reinterpret_cast<void *>(
                               &FakeConfirmQuitRunPostprocessOnPrimaryBuffer),
                           postprocessPatch)) {
        return 1;
    }

    void ( HudUiDialogController::*blitMember)() =
        &HudUiDialogController::BlitOwnedSurfaceToPrimary;
    void ( FakeConfirmQuitBlitThunk::*fakeBlitMember)() =
        &FakeConfirmQuitBlitThunk::BlitOwnedSurfaceToPrimary;
    if (!PatchFunctionJump(reinterpret_cast<void *>(TestSaveLoadMethodAddress(blitMember)),
                           reinterpret_cast<void *>(TestSaveLoadMethodAddress(fakeBlitMember)),
                           blitPatch)) {
        RestoreFunctionPatch(postprocessPatch);
        return 2;
    }

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
                           reinterpret_cast<void *>(&FakeConfirmQuitUnlockPrimarySurfaceState),
                           unlockPatch)) {
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(postprocessPatch);
        return 3;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zSndSampleSet_DestroyByName),
                           reinterpret_cast<void *>(
                               &FakeSaveLoadDeactivateDestroySampleSetByName),
                           destroySampleSetPatch)) {
        RestoreFunctionPatch(unlockPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(postprocessPatch);
        return 4;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zSnd::ApplyMuteStateToActiveVoices),
                           reinterpret_cast<void *>(
                               &FakeSaveLoadDeactivateApplyMuteStateToActiveVoices),
                           applyMutePatch)) {
        RestoreFunctionPatch(destroySampleSetPatch);
        RestoreFunctionPatch(unlockPatch);
        RestoreFunctionPatch(blitPatch);
        RestoreFunctionPatch(postprocessPatch);
        return 5;
    }

    const int oldHalfResMode = g_zVideo_HalfResAdjustMode;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    const zSndSampleSetRegistry oldSampleSetRegistry = g_zSnd_SampleSetRegistry;
    void *const oldGlobalVolumeScale = g_zSnd_GlobalVolumeScalePtr;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldSndPreInitialized = g_zSnd_PreInitialized;
    HudLayoutBase *const oldLayout = g_HudUiMgrCurrentLayout;

    float globalVolumeScale = 1.0f;
    TestSaveLoadPlayStateLayout layout{};

    g_zVideo_HalfResAdjustMode = ZVIDEO_HALFRES_ADJUST_DISABLED;
    g_HudUi_InvalidateMask = 0x04;
    g_zSnd_GlobalVolumeScalePtr = &globalVolumeScale;
    g_zSnd_SampleSetRegistry.clear();
    g_zSnd_ActiveBackend = 0;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_HudUiMgrCurrentLayout = &layout;

    RecoilStateSaveLoadTransition disabledTransition = {};
    disabledTransition.m_dialog = 0;
    disabledTransition.m_capturePresentationMode = RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED;

    g_confirmQuitPostprocessCalls = 0;
    g_confirmQuitBlitCalls = 0;
    g_confirmQuitUnlockCalls = 0;
    g_playStateLayoutActivatedCount = 0;
    g_saveLoadDeactivateDestroyCalls = 0;
    g_saveLoadDeactivateDestroyNameOk = false;
    g_saveLoadDeactivateMuteCalls = 0;
    g_saveLoadDeactivateMuteState = -1;
    disabledTransition.OnDeactivate();

    bool disabledOk =
        disabledTransition.m_dialog == 0 && g_confirmQuitPostprocessCalls == 0 &&
        g_confirmQuitBlitCalls == 0 && g_confirmQuitUnlockCalls == 0 &&
        g_saveLoadDeactivateDestroyCalls == 0 && g_saveLoadDeactivateMuteCalls == 0 &&
        g_playStateLayoutActivatedCount == 0;

    RecoilStateSaveLoadTransition capturedTransition = {};
    capturedTransition.m_capturePresentationMode = RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED;
    capturedTransition.m_savedHalfResAdjustMode = ZVIDEO_HALFRES_ADJUST_ENABLED;
    capturedTransition.m_pausedAudioSnapshot = 0;

    g_confirmQuitPostprocessCalls = 0;
    g_confirmQuitBlitCalls = 0;
    g_confirmQuitUnlockCalls = 0;
    g_playStateLayoutActivatedCount = 0;
    g_saveLoadDeactivateDestroyCalls = 0;
    g_saveLoadDeactivateDestroyNameOk = false;
    g_saveLoadDeactivateMuteCalls = 0;
    g_saveLoadDeactivateMuteState = -1;
    capturedTransition.OnDeactivate();

    bool capturedOk =
        capturedTransition.m_dialog == 0 && g_saveLoadDeactivateDestroyCalls == 1 &&
        g_saveLoadDeactivateDestroyNameOk && g_saveLoadDeactivateMuteCalls == 1 &&
        g_saveLoadDeactivateMuteState == 0 &&
        g_zVideo_HalfResAdjustMode == ZVIDEO_HALFRES_ADJUST_ENABLED &&
        g_HudUi_InvalidateMask == 0x0c && g_playStateLayoutActivatedCount == 1 &&
        g_confirmQuitPostprocessCalls == 0 && g_confirmQuitBlitCalls == 0 &&
        g_confirmQuitUnlockCalls == 0;

    g_zVideo_HalfResAdjustMode = oldHalfResMode;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_zSnd_SampleSetRegistry = oldSampleSetRegistry;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScale;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_zSnd_PreInitialized = oldSndPreInitialized;
    g_HudUiMgrCurrentLayout = oldLayout;

    RestoreFunctionPatch(applyMutePatch);
    RestoreFunctionPatch(destroySampleSetPatch);
    RestoreFunctionPatch(unlockPatch);
    RestoreFunctionPatch(blitPatch);
    RestoreFunctionPatch(postprocessPatch);

    return disabledOk && capturedOk ? 0 : 6;
}

extern "C" int recoil_state_save_load_transition_lifecycle_smoke(void) {
    unsigned char oldTransition[sizeof(g_RecoilStateSaveLoadTransition)] = {};
    std::memcpy(oldTransition, &g_RecoilStateSaveLoadTransition,
                sizeof(g_RecoilStateSaveLoadTransition));

    alignas(RecoilStateSaveLoadTransition) unsigned char constructedStorage[
        sizeof(RecoilStateSaveLoadTransition)] = {};
    RecoilStateSaveLoadTransition *const constructed =
        new (constructedStorage) RecoilStateSaveLoadTransition{};
    constructed->m_dialog = 0x22222222;
    constructed->m_dialogKind = RECOIL_SAVELOAD_DIALOG_LOAD;
    RecoilStateSaveLoadTransition *const constructedReturned = constructed->Constructor();
    if (constructedReturned != constructed || constructed->m_dialog != 0 ||
        constructed->m_dialogKind != RECOIL_SAVELOAD_DIALOG_SAVE) {
        return 1;
    }

    alignas(RecoilStateSaveLoadTransition) unsigned char nullDialogStorage[
        sizeof(RecoilStateSaveLoadTransition)] = {};
    RecoilStateSaveLoadTransition *const nullDialogState =
        new (nullDialogStorage) RecoilStateSaveLoadTransition{};
    nullDialogState->m_dialog = 0;
    nullDialogState->Destructor();
    if (nullDialogState->m_dialog != 0) {
        return 2;
    }

    alignas(RecoilStateSaveLoadTransition) unsigned char scalarStateStorage[
        sizeof(RecoilStateSaveLoadTransition)] = {};
    RecoilStateSaveLoadTransition *const scalarState =
        new (scalarStateStorage) RecoilStateSaveLoadTransition{};
    RecoilStateSaveLoadTransition *const scalarReturned =
        CallStateScalarDeletingDestructor(scalarState, 0);
    if (scalarReturned != scalarState) {
        return 3;
    }

    RecoilStateSaveLoadTransition *const deletingState =
        new RecoilStateSaveLoadTransition{};
    RecoilStateSaveLoadTransition *const deletingReturned =
        CallStateScalarDeletingDestructor(deletingState, 1);
    if (deletingReturned != deletingState) {
        return 4;
    }

    std::memset(&g_RecoilStateSaveLoadTransition, 0,
                sizeof(g_RecoilStateSaveLoadTransition));
    RecoilStateSaveLoadTransition *const staticInitReturned =
        RecoilStateSaveLoadTransition::StaticInit();
    if (staticInitReturned != &g_RecoilStateSaveLoadTransition ||
        ReadStateVtable(g_RecoilStateSaveLoadTransition) != nullptr ||
        g_RecoilStateSaveLoadTransition.m_dialog != 0 ||
        g_RecoilStateSaveLoadTransition.m_dialogKind != RECOIL_SAVELOAD_DIALOG_SAVE) {
        std::memcpy(&g_RecoilStateSaveLoadTransition, oldTransition,
                    sizeof(g_RecoilStateSaveLoadTransition));
        return 5;
    }

    RecoilStateSaveLoadTransition::AtExitDestructor();
    if (ReadStateVtable(g_RecoilStateSaveLoadTransition) != nullptr ||
        g_RecoilStateSaveLoadTransition.m_dialog != 0) {
        std::memcpy(&g_RecoilStateSaveLoadTransition, oldTransition,
                    sizeof(g_RecoilStateSaveLoadTransition));
        return 6;
    }

    std::memset(&g_RecoilStateSaveLoadTransition, 0,
                sizeof(g_RecoilStateSaveLoadTransition));
    RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit();
    const bool staticRegisterOk =
        ReadStateVtable(g_RecoilStateSaveLoadTransition) == nullptr &&
        g_RecoilStateSaveLoadTransition.m_dialog == 0 &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE;

    std::memcpy(&g_RecoilStateSaveLoadTransition, oldTransition,
                sizeof(g_RecoilStateSaveLoadTransition));
    g_RecoilStateSaveLoadTransition.m_dialog = 0;
    g_saveLoadInitExpectedSectionName = nullptr;
    return staticRegisterOk ? 0 : 7;
}

extern "C" int recoil_state_save_load_transition_queue_dialogs_smoke(void) {
    unsigned char oldApp[sizeof(g_RecoilApp)] = {};
    std::memcpy(oldApp, &g_RecoilApp, sizeof(g_RecoilApp));
    unsigned char oldTransition[sizeof(g_RecoilStateSaveLoadTransition)] = {};
    std::memcpy(oldTransition, &g_RecoilStateSaveLoadTransition,
                sizeof(g_RecoilStateSaveLoadTransition));
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;

    zUtil_PlayerStateStorage playerState = {};
    zInput_GameStateOrMapTablePartial gameState = {};
    gameState.playerState = reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    RecoilApp_IState *const transitionState = &g_RecoilStateSaveLoadTransition;

    auto resetHarness = [&]() {
        if (g_RecoilApp.m_stateQueue.m_itemCount != 0) {
            CleanupQueuedItems(g_RecoilApp.m_stateQueue);
        }
        std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
        g_RecoilApp.m_currentStateIndex = -1;
        std::memset(&g_RecoilStateSaveLoadTransition, 0,
                    sizeof(g_RecoilStateSaveLoadTransition));
        TestAppState stateVtableSource{};
        std::memcpy(&g_RecoilStateSaveLoadTransition, &stateVtableSource,
                    sizeof(void *));
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
        IsSinglePushStateQueueItem(g_RecoilApp.m_stateQueue, transitionState, 0);

    resetHarness();
    g_RecoilStateSaveLoadTransition.m_capturePresentationMode =
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED;
    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_LOAD;
    playerState.environmentAttachmentActive = 1;
    RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED);
    const bool saveBlockedOk =
        g_stateEnterCount == 0 && g_RecoilApp.m_stateQueue.m_itemCount == 0 &&
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
        IsSinglePushStateQueueItem(g_RecoilApp.m_stateQueue, transitionState, 0);

    resetHarness();
    RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_QUICKLOAD);
    const bool loadQuickOk =
        g_stateEnterCount == 1 &&
        g_RecoilStateSaveLoadTransition.m_transitionMode == RECOIL_SAVELOAD_MODE_QUICKLOAD &&
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode ==
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_LOAD &&
        IsSinglePushStateQueueItem(g_RecoilApp.m_stateQueue, transitionState, 0);

    resetHarness();
    g_RecoilStateSaveLoadTransition.m_transitionMode = RECOIL_SAVELOAD_MODE_FADE;
    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    playerState.environmentAttachmentActive = 1;
    RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_QUICKLOAD);
    const bool loadBlockedOk =
        g_stateEnterCount == 0 && g_RecoilApp.m_stateQueue.m_itemCount == 0 &&
        g_RecoilStateSaveLoadTransition.m_transitionMode == RECOIL_SAVELOAD_MODE_FADE &&
        g_RecoilStateSaveLoadTransition.m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE;

    if (g_RecoilApp.m_stateQueue.m_itemCount != 0) {
        CleanupQueuedItems(g_RecoilApp.m_stateQueue);
    }
    std::memcpy(&g_RecoilApp, oldApp, sizeof(g_RecoilApp));
    std::memcpy(&g_RecoilStateSaveLoadTransition, oldTransition,
                sizeof(g_RecoilStateSaveLoadTransition));
    g_GameStateOrMapTable = oldGameState;

    return saveOk && saveBlockedOk && loadStandardOk && loadQuickOk && loadBlockedOk ? 0 : 1;
}

extern "C" int recoil_app_play_state_constructor_smoke(void) {
    alignas(RecoilApp_PlayState) unsigned char storage[sizeof(RecoilApp_PlayState)];
    std::memset(storage, 0xcc, sizeof(storage));
    RecoilApp_PlayState *const playState = reinterpret_cast<RecoilApp_PlayState *>(storage);
    zOpt_ViewRectSection windowSection = {};
    zOpt_ViewRectSection displaySection = {};
    zOpt_ViewRectSection renderSection = {};
    playState->pWindowSection = &windowSection;
    playState->pDisplaySection = &displaySection;
    playState->pRenderSection = &renderSection;
    playState->m_transitionScratch = 0x55555555;
    playState->pPendingLoadGameStartPath = reinterpret_cast<char *>(0x66666666);

    RecoilApp_PlayState *returned = new (playState) RecoilApp_PlayState();
    if (returned != playState) {
        return 1;
    }

    if (*reinterpret_cast<void **>(playState) == nullptr ||
        playState->m_transitionScratch != 0 || playState->pPendingLoadGameStartPath != nullptr) {
        return 2;
    }

    return playState->pWindowSection == &windowSection &&
                   playState->pDisplaySection == &displaySection &&
                   playState->pRenderSection == &renderSection
               ? 0
               : 3;
}

extern "C" int recoil_app_play_state_on_wnd_activate_smoke(void) {
    HudLayoutBase *const oldLayout = g_HudUiMgrCurrentLayout;
    TestSaveLoadPlayStateLayout layout{};
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
    zOpt_ViewRectSection **const oldRenderOption = g_zGame_Options_PointerCache.renderSection;
    zOpt_ViewRectSection **const oldDisplayOption = g_zGame_Options_PointerCache.displaySection;
    zOpt_ViewRectSection **const oldWindowOption = g_zGame_Options_PointerCache.windowSection;
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
    g_zGame_Options_PointerCache.renderSection = &renderPtr;
    g_zGame_Options_PointerCache.displaySection = &displayPtr;
    g_zGame_Options_PointerCache.windowSection = &windowPtr;
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
        playState.pRenderSection == &renderSection &&
        playState.pDisplaySection == &displaySection &&
        playState.pWindowSection == &windowSection;

    g_Player_ActiveDebugScriptAsyncEntry = oldDebugEntry;
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    g_zGame_Options_PointerCache.renderSection = oldRenderOption;
    g_zGame_Options_PointerCache.displaySection = oldDisplayOption;
    g_zGame_Options_PointerCache.windowSection = oldWindowOption;
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
    unsigned char oldApp[sizeof(g_RecoilApp)] = {};
    std::memcpy(oldApp, &g_RecoilApp, sizeof(g_RecoilApp));
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const int oldQuitAfterCredits = g_RecoilApp_QuitAfterCredits;
    zEffectAnimEntry *const oldDebugEntry = g_Player_ActiveDebugScriptAsyncEntry;
    int *const oldMuteOption = g_zGame_Options_PointerCache.muteSound;
    zOpt_ViewRectSection **const oldRenderOption = g_zGame_Options_PointerCache.renderSection;
    zOpt_ViewRectSection **const oldDisplayOption = g_zGame_Options_PointerCache.displaySection;
    zOpt_ViewRectSection **const oldWindowOption = g_zGame_Options_PointerCache.windowSection;
    const unsigned char oldInputRegistry = g_zInput_DeviceRegistry;
    const unsigned char oldMouseFlags = g_zInputMouseFlags;
    const unsigned char oldJoystickFlags = g_zInputJoystickFlags;
    const short oldKeyboardPollRefCount = g_zInputKeyboardPollRefCount;
    const short oldMousePollRefCount = g_zInputMousePollRefCount;
    const short oldJoystickPollRefCount = g_zInputJoystickPollRefCount;
    const int oldHotkeyEnabled = g_zVideo_SoftwareModeHotkeyEnabled;
    const int oldAdjustDisableGate = g_zVideo_AdjustSurfacesDisableGate;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const TimeRuntimeConfig oldTimeRuntimeConfig = g_Time_RuntimeConfig;
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
    TestSaveLoadPlayStateLayout layout{};
    int muteOption = 1;

    saveState.playerState = &playerState;
    g_HudUiMgrCurrentLayout = &layout;
    g_playStateLayoutActivatedCount = 0;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_zGame_Options_PointerCache.muteSound = &muteOption;
    g_zGame_Options_PointerCache.renderSection = &renderPtr;
    g_zGame_Options_PointerCache.displaySection = &displayPtr;
    g_zGame_Options_PointerCache.windowSection = &windowPtr;
    g_Player_ActiveDebugScriptAsyncEntry = nullptr;
    g_RecoilApp_QuitAfterCredits = 1;
    g_RecoilApp.m_transitionFadeTimer = 0.05f;
    g_zInput_DeviceRegistry = 0;
    g_zInputMouseFlags = 0;
    g_zInputJoystickFlags = 0;
    g_zInputKeyboardPollRefCount = 0;
    g_zInputMousePollRefCount = 0;
    g_zInputJoystickPollRefCount = 0;
    g_zVideo_SoftwareModeHotkeyEnabled = 1;
    g_zVideo_AdjustSurfacesDisableGate = 1;
    g_zVideo_ActiveRendererPath = 0;
    g_Time_RuntimeConfig.maximumDeltaTimeSec = 0.125f;
    g_Time_RuntimeConfig.deltaTimeClampEnabled = 1;
    g_Time_RuntimeConfig.currentTimeSec = 0.0f;
    g_Time_RuntimeConfig.timeScaleFactor = 1.0f;
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
        g_RecoilApp.m_transitionFadeTimer <= 0.0f &&
        playState.pWindowSection == &windowSection &&
        zRndr::g_overlayBlendEnabled == 1 && zRndr::g_overlayBlendPackedColor16 == 0 &&
        overlayAlphaOk;

    std::memcpy(&g_RecoilApp, oldApp, sizeof(g_RecoilApp));
    g_GameStateOrMapTable = oldGameState;
    g_RecoilApp_QuitAfterCredits = oldQuitAfterCredits;
    g_Player_ActiveDebugScriptAsyncEntry = oldDebugEntry;
    g_zGame_Options_PointerCache.muteSound = oldMuteOption;
    g_zGame_Options_PointerCache.renderSection = oldRenderOption;
    g_zGame_Options_PointerCache.displaySection = oldDisplayOption;
    g_zGame_Options_PointerCache.windowSection = oldWindowOption;
    g_zInput_DeviceRegistry = oldInputRegistry;
    g_zInputMouseFlags = oldMouseFlags;
    g_zInputJoystickFlags = oldJoystickFlags;
    g_zInputKeyboardPollRefCount = oldKeyboardPollRefCount;
    g_zInputMousePollRefCount = oldMousePollRefCount;
    g_zInputJoystickPollRefCount = oldJoystickPollRefCount;
    g_zVideo_SoftwareModeHotkeyEnabled = oldHotkeyEnabled;
    g_zVideo_AdjustSurfacesDisableGate = oldAdjustDisableGate;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_Time_RuntimeConfig = oldTimeRuntimeConfig;
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
    int *const oldCdAudioOption = g_zGame_Options_PointerCache.cdAudio;
    const std::int32_t oldCdFlags = g_zSndCdFlags;
    int cdAudioOption = 0;

    g_zGame_Options_PointerCache.cdAudio = &cdAudioOption;
    g_zSndCdFlags = 0x12345678;

    RecoilApp_PlayState playState{};
    playState.OnResume(0x55);

    const bool ok = cdAudioOption == 0 && g_zSndCdFlags == 0x12345678;

    g_zGame_Options_PointerCache.cdAudio = oldCdAudioOption;
    g_zSndCdFlags = oldCdFlags;
    return ok ? 0 : 1;
}

extern "C" int recoil_app_play_state_on_deactivate_skip_gameplay_smoke(void) {
    unsigned char oldApp[sizeof(g_RecoilApp)] = {};
    std::memcpy(oldApp, &g_RecoilApp, sizeof(g_RecoilApp));
    auto *const oldMountedList = g_zArchive_MountedList;
    auto *const oldCurrentArchive = g_zArchive_Current;
    int *const oldAccelerationOption = g_zGame_Options_PointerCache.videoAcceleration;
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    const std::int32_t oldCdFlags = g_zSndCdFlags;
    const int oldHalfResMode = g_zVideo_HalfResAdjustMode;
    const int oldUseHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;

    int acceleration = 0;
    int networkEnabled = 1;
    zArchiveList mountedList = {};

    g_zGame_Options_PointerCache.videoAcceleration = &acceleration;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
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

    std::memcpy(&g_RecoilApp, oldApp, sizeof(g_RecoilApp));
    g_zArchive_MountedList = oldMountedList;
    g_zArchive_Current = oldCurrentArchive;
    g_zGame_Options_PointerCache.videoAcceleration = oldAccelerationOption;
    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_zSndCdFlags = oldCdFlags;
    g_zVideo_HalfResAdjustMode = oldHalfResMode;
    g_zVideo_UseHalfResBackbuffer = oldUseHalfResBackbuffer;
    return ok ? 0 : 1;
}
