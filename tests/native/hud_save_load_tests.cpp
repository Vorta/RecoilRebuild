#include "Battlesport/hud.h"
#include "GameZRecoil/RecoilApp/recoil_state_main_menu_transition.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

void __fastcall InsertEntryIntoSortedPrefix(
    HudUiSaveLoadEntry *entryPosition,
    HudUiSaveLoadEntry entry
);
HudUiSaveLoadEntry *__fastcall PartitionEntriesByPivot(
    HudUiSaveLoadEntry *begin,
    HudUiSaveLoadEntry *end,
    HudUiSaveLoadEntry pivot
);
void __fastcall SortEntryRange(
    HudUiSaveLoadEntry *begin,
    HudUiSaveLoadEntry *end,
    int unused
);

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

template <typename Method> void *MethodAddress(Method method) {
    std::uintptr_t address = 0;
    std::memcpy(
        &address,
        &method,
        sizeof(method)
    );
    return reinterpret_cast<void *>(address);
}

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    if (target == nullptr || replacement == nullptr) {
        patch.address = nullptr;
        return false;
    }

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

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
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

RecoilApp_StateQueueItem *SaveLoadQueueItemAt(
    RecoilApp_StateQueue &queue,
    int index
) {
    if (index < 0 || index >= queue.m_itemCount ||
        queue.m_readBlock.m_cursor == nullptr) {
        return nullptr;
    }

    return queue.m_readBlock.m_cursor[index];
}

bool SaveLoadQueueHasSingleExit(
    RecoilApp_StateQueue &queue,
    int param
) {
    RecoilApp_StateQueueItem *const item = SaveLoadQueueItemAt(
        queue,
        0
    );
    return queue.m_itemCount == 1 && item != nullptr &&
           item->m_kind == RecoilApp_StateQueueKind_ExitCurrent &&
           item->m_param == param;
}

void CleanupSaveLoadQueue(
    RecoilApp_StateQueue &queue
) {
    const int itemCount = queue.m_itemCount;
    for (int index = 0; index < itemCount; ++index) {
        ::operator delete(SaveLoadQueueItemAt(
            queue,
            index
        ));
    }

    if (queue.m_chunkBaseList != nullptr) {
        if (queue.m_readBlock.m_chunkBaseSlot != nullptr &&
            queue.m_writeBlock.m_chunkBaseSlot != nullptr) {
            for (RecoilApp_StateQueueItem ***slot =
                     queue.m_readBlock.m_chunkBaseSlot;
                 slot <= queue.m_writeBlock.m_chunkBaseSlot;
                 ++slot) {
                ::operator delete(*slot);
            }
        }
        ::operator delete(queue.m_chunkBaseList);
    }

    std::memset(
        &queue,
        0,
        sizeof(queue)
    );
}

void SaveLoadSetEmptyGameName(
    HudUiLoadGameDialog *dialog,
    char *buffer,
    int capacity
) {
    dialog->gameNameInput.textInput.buffer = buffer;
    dialog->gameNameInput.textInput.capacity = capacity;
    dialog->gameNameInput.textInput.cursor = 0;
}

int g_saveGameInitLoadCalls;
bool g_saveGameInitLoadArgsOk;
int g_loadGameInitLoadCalls;
bool g_loadGameInitLoadArgsOk;
int g_saveLoadSetSelectedCalls;
HudUiSaveLoadDialog *g_saveLoadSetSelectedThis;
int g_saveLoadSetSelectedIndex;
int g_saveLoadRefreshCalls;
HudUiSaveLoadDialog *g_saveLoadRefreshThis;
int g_saveLoadInitializeCalls;
HudUiSaveLoadDialog *g_saveLoadInitializeThis;
int g_saveLoadDeleteCalls;
HudUiSaveLoadDialog *g_saveLoadDeleteThis;
int g_saveLoadDeleteConfirm;
int g_saveLoadProcessCalls;
HudUiSaveLoadDialog *g_saveLoadProcessThis;
int g_loadGamePrimaryActionCalls;
HudUiLoadGameDialog *g_loadGamePrimaryActionThis;
int g_saveLoadFileExistsCalls;
char g_saveLoadFileExistsPath[MAX_PATH];
int g_saveLoadZarLoadCalls;
char g_saveLoadZarLoadPath[MAX_PATH];
int g_saveLoadQueueExitCalls;
int g_saveLoadQueueExitParams[4];
int g_saveLoadQueueSwitchCalls;
RecoilApp_IState *g_saveLoadQueueSwitchStates[4];
int g_saveLoadQueueSwitchParams[4];
int g_saveLoadStateEnterCount;
int g_saveLoadStateExitCount;
int g_confirmQuitBackgroundLoadCalls;
bool g_confirmQuitBackgroundLoadArgsOk;
int g_confirmQuitBackgroundBindCalls;
bool g_confirmQuitBackgroundBindArgsOk;
int g_confirmQuitBackgroundFreeCalls;
bool g_confirmQuitBackgroundFreeArgsOk;
int g_saveLoadStartEngineCount;
int g_saveLoadShutdownEngineCount;
int g_saveLoadExitInstanceCount;
int g_saveLoadStartEngineResult;
HWND g_saveLoadLastStartEngineHwnd;
int g_saveLoadCallOrder;
int g_saveLoadActionOrder;
int g_saveLoadListItemUpdateBoundsCount;
HudUiSaveLoadListItem *g_saveLoadListItemUpdateBoundsThis;

struct SaveGameInitLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * SaveGameInitLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_saveGameInitLoadCalls;
    g_saveGameInitLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "SAVE_GAME_DIALOG"
        ) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

struct ConfirmQuitBackgroundProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
    int BindWidgetByName(
        zReader::Node *loadedSectionNode,
        HudUiZrdWidget *widget,
        const char *name
    );
    void FreeLoadedTreeRoots(int loadedRoot);
};

zReader::Node g_confirmQuitBackgroundNode;

zReader::Node * ConfirmQuitBackgroundProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_confirmQuitBackgroundLoadCalls;
    g_confirmQuitBackgroundLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "CONFIRM_QUIT"
        ) == 0 &&
        capturePrimary == 0;
    return &g_confirmQuitBackgroundNode;
}

int ConfirmQuitBackgroundProbe::BindWidgetByName(
    zReader::Node *loadedSectionNode,
    HudUiZrdWidget *widget,
    const char *name
) {
    ++g_confirmQuitBackgroundBindCalls;

    HudUiBackgroundConfirmQuit *const dialog =
        static_cast<HudUiBackgroundConfirmQuit *>(static_cast<void *>(this));
    const bool widgetOk =
        (g_confirmQuitBackgroundBindCalls == 1 &&
         widget == &dialog->okButton &&
         name != nullptr &&
         std::strcmp(name, "OK_TO_QUIT") == 0) ||
        (g_confirmQuitBackgroundBindCalls == 2 &&
         widget == &dialog->cancelButton &&
         name != nullptr &&
         std::strcmp(name, "CANCEL_QUIT") == 0);

    g_confirmQuitBackgroundBindArgsOk =
        g_confirmQuitBackgroundBindArgsOk &&
        loadedSectionNode == &g_confirmQuitBackgroundNode &&
        widgetOk;
    return 0;
}

void ConfirmQuitBackgroundProbe::FreeLoadedTreeRoots(
    int loadedRoot
) {
    ++g_confirmQuitBackgroundFreeCalls;
    g_confirmQuitBackgroundFreeArgsOk =
        loadedRoot == (int)reinterpret_cast<std::uintptr_t>(&g_confirmQuitBackgroundNode);
}

void ResetSaveLoadListItemDrawCapture() {
    g_saveLoadListItemUpdateBoundsCount = 0;
    g_saveLoadListItemUpdateBoundsThis = nullptr;
}

struct TestSaveLoadListItem : HudUiSaveLoadListItem {
    void UpdateTextBoundsFromContent();
};

void TestSaveLoadListItem::UpdateTextBoundsFromContent() {
    ++g_saveLoadListItemUpdateBoundsCount;
    g_saveLoadListItemUpdateBoundsThis = this;
}

struct SaveLoadTestAppState : RecoilApp_IState {
    void OnEnter() {
        ++g_saveLoadStateEnterCount;
    }

    void OnExit() {
        ++g_saveLoadStateExitCount;
    }
};

struct SaveLoadTestRecoilApp : RecoilApp {
    int StartEngine(HWND hwnd) {
        ++g_saveLoadStartEngineCount;
        g_saveLoadLastStartEngineHwnd = hwnd;
        return g_saveLoadStartEngineResult;
    }

    void ShutdownEngine() {
        ++g_saveLoadShutdownEngineCount;
    }

    int ExitInstance() {
        ++g_saveLoadExitInstanceCount;
        return 77;
    }
};

void InstallSaveLoadStateVptr(
    RecoilApp_IState &target,
    RecoilApp_IState &source
) {
    *reinterpret_cast<void **>(&target) =
        *reinterpret_cast<void **>(&source);
}

extern "C" int recoil_app_state_queue_block_init_from_cursor_smoke(void) {
    RecoilApp_StateQueueItem *chunk[1024] = {};
    RecoilApp_StateQueueItem **chunkBaseSlot = chunk;
    RecoilApp_StateQueueItem **const cursor = chunk + 512;
    RecoilApp_StateQueueBlock block{};

    RecoilApp_StateQueueBlock *const returned = block.InitFromCursor(
        cursor,
        &chunkBaseSlot
    );

    if (returned != &block) {
        return 1;
    }

    return block.m_chunkBegin == chunk &&
                   block.m_chunkEnd == chunk + 1024 &&
                   block.m_cursor == cursor &&
                   block.m_chunkBaseSlot == &chunkBaseSlot
               ? 0
               : 2;
}

extern "C" int recoil_app_queue_switch_current_state_smoke(void) {
    g_saveLoadStateEnterCount = 0;
    g_saveLoadStateExitCount = 0;

    RecoilApp app;
    SaveLoadTestAppState oldState;
    SaveLoadTestAppState newState;
    app.m_currentStateIndex = 0;
    app.m_stateStack[0] = &oldState;

    RecoilApp_IState *const returned = app.QueueSwitchCurrentState(
        &newState,
        42
    );
    RecoilApp_StateQueue &queue = app.m_stateQueue;
    RecoilApp_StateQueueItem *const item = SaveLoadQueueItemAt(
        queue,
        0
    );

    int result = 0;
    if (returned != &oldState ||
        g_saveLoadStateExitCount != 1 ||
        g_saveLoadStateEnterCount != 1 ||
        queue.m_itemCount != 1) {
        result = 1;
    } else if (item == nullptr ||
               item->m_type != 0 ||
               item->m_kind != RecoilApp_StateQueueKind_SwitchCurrent ||
               item->m_stateObj != &newState ||
               item->m_param != 42) {
        result = 2;
    }

    CleanupSaveLoadQueue(queue);
    return result;
}

extern "C" int recoil_app_queue_push_state_smoke(void) {
    g_saveLoadStateEnterCount = 0;
    g_saveLoadStateExitCount = 0;

    RecoilApp app;
    SaveLoadTestAppState oldState;
    SaveLoadTestAppState newState;
    app.m_currentStateIndex = 0;
    app.m_stateStack[0] = &oldState;

    RecoilApp_IState *const returned = app.QueuePushState(
        &newState,
        23
    );
    RecoilApp_StateQueue &queue = app.m_stateQueue;
    RecoilApp_StateQueueItem *const item = SaveLoadQueueItemAt(
        queue,
        0
    );

    int result = 0;
    if (returned != &oldState ||
        g_saveLoadStateExitCount != 0 ||
        g_saveLoadStateEnterCount != 1 ||
        queue.m_itemCount != 1) {
        result = 1;
    } else if (item == nullptr ||
               item->m_type != 0 ||
               item->m_kind != RecoilApp_StateQueueKind_PushState ||
               item->m_stateObj != &newState ||
               item->m_param != 23) {
        result = 2;
    }

    CleanupSaveLoadQueue(queue);
    return result;
}

extern "C" int recoil_app_queue_exit_current_state_smoke(void) {
    g_saveLoadStateEnterCount = 0;
    g_saveLoadStateExitCount = 0;

    RecoilApp app;
    SaveLoadTestAppState oldState;
    app.m_currentStateIndex = 0;
    app.m_stateStack[0] = &oldState;

    RecoilApp_IState *const returned = app.QueueExitCurrentState(17);
    RecoilApp_StateQueue &queue = app.m_stateQueue;
    RecoilApp_StateQueueItem *const item = SaveLoadQueueItemAt(
        queue,
        0
    );

    int result = 0;
    if (returned != &oldState ||
        g_saveLoadStateExitCount != 1 ||
        g_saveLoadStateEnterCount != 0 ||
        queue.m_itemCount != 1) {
        result = 1;
    } else if (item == nullptr ||
               item->m_type != 0 ||
               item->m_kind != RecoilApp_StateQueueKind_ExitCurrent ||
               item->m_stateObj != nullptr ||
               item->m_param != 17) {
        result = 2;
    }

    CleanupSaveLoadQueue(queue);
    return result;
}

extern "C" int recoil_app_mfc_ole_module_constructor_smoke(void) {
    void *const storage = ::operator new(sizeof(RecoilApp_MfcOleModule));
    std::memset(
        storage,
        0x5a,
        sizeof(RecoilApp_MfcOleModule)
    );

    RecoilApp_MfcOleModule *const app = new (storage) RecoilApp_MfcOleModule;

    int result = 0;
    if (app->m_pendingState != nullptr ||
        app->m_currentStateIndex != -1 ||
        app->m_skipWait != 0) {
        result = 1;
    } else if (app->m_stateQueue.m_itemCount != 0 ||
               app->m_stateQueue.m_chunkBaseList != nullptr) {
        result = 2;
    }

    for (int index = 0; result == 0 && index < 16; ++index) {
        if (app->m_stateStack[index] != nullptr) {
            result = 3;
        }
    }

    app->~RecoilApp_MfcOleModule();
    ::operator delete(storage);
    return result;
}

extern "C" int recoil_app_mfc_ole_module_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(RecoilApp_MfcOleModule));
    RecoilApp_MfcOleModule *const app = new (storage) RecoilApp_MfcOleModule;
    RecoilApp_StateQueueItem *queuedItem = nullptr;
    app->m_stateQueue.PushBack(queuedItem);

    int result = 0;
    if (app->m_stateQueue.m_itemCount != 1 ||
        app->m_stateQueue.m_chunkBaseList == nullptr) {
        result = 1;
    }

    app->~RecoilApp_MfcOleModule();

    if (result == 0 &&
        (app->m_stateQueue.m_itemCount != 0 ||
         app->m_stateQueue.m_chunkBaseList != nullptr ||
         app->m_stateQueue.m_readBlock.m_cursor != nullptr ||
         app->m_stateQueue.m_writeBlock.m_cursor != nullptr)) {
        result = 2;
    }

    ::operator delete(storage);
    return result;
}

extern "C" int recoil_app_constructor_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(RecoilApp));
    std::memset(
        storage,
        0,
        sizeof(RecoilApp)
    );

    RecoilApp *const app = new (storage) RecoilApp;

    int result = 0;
    if (app->m_pendingState != nullptr ||
        app->m_currentStateIndex != -1 ||
        app->m_skipWait != 0 ||
        app->m_skipIntroFmv != 0 ||
        app->m_transitionFadeTimer != 0.0f) {
        result = 1;
    } else if (app->m_stateQueue.m_itemCount != 0 ||
               app->m_stateQueue.m_chunkBaseList != nullptr) {
        result = 2;
    } else if (app->m_attractFmvState.m_fmv.m_fmvPath != nullptr ||
               app->m_introFmvState.m_fmv.m_fmvPath != nullptr ||
               app->m_missionFmvState.m_fmv.m_fmvPath != nullptr) {
        result = 3;
    }

    char *const attractPath = static_cast<char *>(std::malloc(4));
    char *const introPath = static_cast<char *>(std::malloc(4));
    char *const missionPath = static_cast<char *>(std::malloc(4));
    if (attractPath == nullptr || introPath == nullptr || missionPath == nullptr) {
        std::free(attractPath);
        std::free(introPath);
        std::free(missionPath);
        app->~RecoilApp();
        ::operator delete(storage);
        return 4;
    }

    app->m_attractFmvState.m_fmv.m_fmvPath = attractPath;
    app->m_introFmvState.m_fmv.m_fmvPath = introPath;
    app->m_missionFmvState.m_fmv.m_fmvPath = missionPath;

    app->~RecoilApp();

    if (result == 0 &&
        (app->m_attractFmvState.m_fmv.m_fmvPath != nullptr ||
         app->m_introFmvState.m_fmv.m_fmvPath != nullptr ||
         app->m_missionFmvState.m_fmv.m_fmvPath != nullptr)) {
        result = 5;
    }

    ::operator delete(storage);
    return result;
}

extern "C" int recoil_app_fmv_state_destructor_smoke(void) {
    void *const attractStorage = ::operator new(sizeof(RecoilApp_AttractFmvState));
    RecoilApp_AttractFmvState *const attract =
        new (attractStorage) RecoilApp_AttractFmvState;
    char *const attractPath = static_cast<char *>(std::malloc(4));
    if (attractPath == nullptr) {
        attract->~RecoilApp_AttractFmvState();
        ::operator delete(attractStorage);
        return 1;
    }

    attract->m_fmv.m_fmvPath = attractPath;
    attract->~RecoilApp_AttractFmvState();
    const bool attractCleared = attract->m_fmv.m_fmvPath == nullptr;
    ::operator delete(attractStorage);
    if (!attractCleared) {
        return 2;
    }

    void *const introStorage = ::operator new(sizeof(RecoilApp_IntroFmvState));
    RecoilApp_IntroFmvState *const intro =
        new (introStorage) RecoilApp_IntroFmvState;
    char *const introPath = static_cast<char *>(std::malloc(4));
    if (introPath == nullptr) {
        intro->~RecoilApp_IntroFmvState();
        ::operator delete(introStorage);
        return 3;
    }

    intro->m_fmv.m_fmvPath = introPath;
    intro->~RecoilApp_IntroFmvState();
    const bool introCleared = intro->m_fmv.m_fmvPath == nullptr;
    ::operator delete(introStorage);

    return introCleared ? 0 : 4;
}

extern "C" int recoil_app_mission_fmv_state_destructor_smoke(void) {
    void *const storage = ::operator new(sizeof(RecoilApp_MissionFmvState));
    RecoilApp_MissionFmvState *const mission =
        new (storage) RecoilApp_MissionFmvState;
    char *const missionPath = static_cast<char *>(std::malloc(4));
    if (missionPath == nullptr) {
        mission->~RecoilApp_MissionFmvState();
        ::operator delete(storage);
        return 1;
    }

    mission->m_fmv.m_fmvPath = missionPath;
    mission->~RecoilApp_MissionFmvState();
    const bool missionCleared = mission->m_fmv.m_fmvPath == nullptr;
    ::operator delete(storage);

    return missionCleared ? 0 : 2;
}

extern "C" int recoil_app_scalar_deleting_destructor_smoke(void) {
    void *const stateStorage = ::operator new(sizeof(RecoilApp_IState));
    RecoilApp_IState *const state = new (stateStorage) RecoilApp_IState;
    RecoilApp_IState baseProbe;
    SaveLoadTestAppState derivedProbe;
    InstallSaveLoadStateVptr(
        *state,
        derivedProbe
    );

    state->~RecoilApp_IState();
    const bool baseVptrRestored =
        *reinterpret_cast<void **>(state) ==
        *reinterpret_cast<void **>(&baseProbe);
    ::operator delete(stateStorage);
    if (!baseVptrRestored) {
        return 1;
    }

    RecoilApp_IState *const deletingState = new RecoilApp_IState;
    delete deletingState;

    RecoilApp *const deletingApp = new RecoilApp;
    delete deletingApp;

    RecoilApp_AttractFmvState *const deletingAttract =
        new RecoilApp_AttractFmvState;
    delete deletingAttract;

    RecoilApp_IntroFmvState *const deletingIntro =
        new RecoilApp_IntroFmvState;
    delete deletingIntro;

    RecoilApp_MissionFmvState *const deletingMission =
        new RecoilApp_MissionFmvState;
    delete deletingMission;

    return 0;
}

extern "C" int recoil_app_initialize_display_failure_smoke(void) {
    static std::int32_t modeIndex = 3;
    static std::int32_t fullscreen = 1;
    static std::int32_t hwApi = 0;
    static std::int32_t acceleration = 1;

    int *const oldVideoMode = ZOPT_VIDEO_MODE;
    int *const oldFullscreen = ZOPT_VIDEO_FULLSCREEN;
    int *const oldHwApi = ZOPT_HW_API;
    int *const oldAcceleration = ZOPT_VIDEO_ACCELERATION;
    const int oldVideoInitialized = g_zVideo_IsInitialized;

    ZOPT_VIDEO_MODE = &modeIndex;
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_HW_API = &hwApi;
    ZOPT_VIDEO_ACCELERATION = &acceleration;
    g_zVideo_IsInitialized = 1;

    const std::int32_t result = RecoilApp::InitializeDisplay(
        reinterpret_cast<HWND>(0x12345678)
    );

    ZOPT_VIDEO_MODE = oldVideoMode;
    ZOPT_VIDEO_FULLSCREEN = oldFullscreen;
    ZOPT_HW_API = oldHwApi;
    ZOPT_VIDEO_ACCELERATION = oldAcceleration;
    g_zVideo_IsInitialized = oldVideoInitialized;
    return result == 0 ? 0 : 1;
}

extern "C" int recoil_app_start_engine_and_queue_startup_state_smoke(void) {
    g_saveLoadStateEnterCount = 0;
    g_saveLoadStateExitCount = 0;
    g_saveLoadStartEngineCount = 0;
    g_saveLoadShutdownEngineCount = 0;
    g_saveLoadExitInstanceCount = 0;
    g_saveLoadStartEngineResult = 1;
    g_saveLoadLastStartEngineHwnd = 0;

    SaveLoadTestRecoilApp app;
    SaveLoadTestAppState startupState;
    void *frameWords[9] = {};
    frameWords[8] = reinterpret_cast<void *>(0x44556677);

    app.m_pMainWnd = reinterpret_cast<CWnd *>(frameWords);
    app.m_pendingState = &startupState;
    app.m_currentStateIndex = -1;
    app.m_skipWait = 0;
    app.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;

    const int returned = app.StartEngineAndQueueStartupState();
    RecoilApp_StateQueue &queue = app.m_stateQueue;
    RecoilApp_StateQueueItem *const item = SaveLoadQueueItemAt(
        queue,
        0
    );

    int result = 0;
    if (returned != 1 ||
        g_saveLoadStartEngineCount != 1 ||
        g_saveLoadLastStartEngineHwnd !=
            reinterpret_cast<HWND>(0x44556677) ||
        g_saveLoadShutdownEngineCount != 0 ||
        g_saveLoadExitInstanceCount != 0 ||
        g_saveLoadStateEnterCount != 1 ||
        g_saveLoadStateExitCount != 0 ||
        app.m_skipWait != 1 ||
        app.m_missionShutdownMode != RECOILAPP_MISSION_SHUTDOWN_ON_EXIT ||
        queue.m_itemCount != 1) {
        result = 1;
    } else if (item == nullptr ||
               item->m_type != 0 ||
               item->m_kind != RecoilApp_StateQueueKind_SwitchCurrent ||
               item->m_stateObj != &startupState ||
               item->m_param != 0) {
        result = 2;
    }

    CleanupSaveLoadQueue(queue);
    return result;
}

struct LoadGameInitLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * LoadGameInitLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_loadGameInitLoadCalls;
    g_loadGameInitLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "LOAD_GAME_DIALOG"
        ) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *HudUiBackgroundLoadFromZrdAddress() {
    return MethodAddress(&HudUiBackground::LoadFromZrd);
}

void *SaveGameInitLoadProbeAddress() {
    return MethodAddress(&SaveGameInitLoadProbe::LoadFromZrd);
}

void *LoadGameInitLoadProbeAddress() {
    return MethodAddress(&LoadGameInitLoadProbe::LoadFromZrd);
}

void *HudUiBackgroundBindWidgetByNameAddress() {
    return MethodAddress(&HudUiBackground::BindWidgetByName);
}

void *HudUiBackgroundFreeLoadedTreeRootsAddress() {
    return MethodAddress(&HudUiBackground::FreeLoadedTreeRoots);
}

void *ConfirmQuitBackgroundLoadProbeAddress() {
    return MethodAddress(&ConfirmQuitBackgroundProbe::LoadFromZrd);
}

void *ConfirmQuitBackgroundBindProbeAddress() {
    return MethodAddress(&ConfirmQuitBackgroundProbe::BindWidgetByName);
}

void *ConfirmQuitBackgroundFreeProbeAddress() {
    return MethodAddress(&ConfirmQuitBackgroundProbe::FreeLoadedTreeRoots);
}

struct SaveLoadDialogActionProbe {
    void InitializeFileEntries();
    void SetSelectedEntryIndex(int selectedEntryIndex);
    void RefreshSaveFileList();
    void DeleteSaveFile(int confirmDelete);
    void ProcessDialogResult();
};

struct LoadGameDialogActionProbe {
    void OnPrimaryAction();
};

struct RecoilAppQueueProbe {
    RecoilApp_IState * QueueExitCurrentState(int stateParam);
    RecoilApp_IState * QueueSwitchCurrentState(
        RecoilApp_IState *state,
        int stateParam
    );
};

void SaveLoadDialogActionProbe::InitializeFileEntries() {
    ++g_saveLoadInitializeCalls;
    g_saveLoadInitializeThis = reinterpret_cast<HudUiSaveLoadDialog *>(this);
}

void SaveLoadDialogActionProbe::SetSelectedEntryIndex(
    int selectedEntryIndex
) {
    ++g_saveLoadSetSelectedCalls;
    g_saveLoadSetSelectedThis = reinterpret_cast<HudUiSaveLoadDialog *>(this);
    g_saveLoadSetSelectedIndex = selectedEntryIndex;
    if (g_saveLoadActionOrder == 0) {
        g_saveLoadActionOrder = ++g_saveLoadCallOrder;
    }
}

void SaveLoadDialogActionProbe::RefreshSaveFileList() {
    ++g_saveLoadRefreshCalls;
    g_saveLoadRefreshThis = reinterpret_cast<HudUiSaveLoadDialog *>(this);
}

void SaveLoadDialogActionProbe::DeleteSaveFile(
    int confirmDelete
) {
    ++g_saveLoadDeleteCalls;
    g_saveLoadDeleteThis = reinterpret_cast<HudUiSaveLoadDialog *>(this);
    g_saveLoadDeleteConfirm = confirmDelete;
    if (g_saveLoadActionOrder == 0) {
        g_saveLoadActionOrder = ++g_saveLoadCallOrder;
    }
}

void SaveLoadDialogActionProbe::ProcessDialogResult() {
    ++g_saveLoadProcessCalls;
    g_saveLoadProcessThis = reinterpret_cast<HudUiSaveLoadDialog *>(this);
    if (g_saveLoadActionOrder == 0) {
        g_saveLoadActionOrder = ++g_saveLoadCallOrder;
    }
}

void LoadGameDialogActionProbe::OnPrimaryAction() {
    ++g_loadGamePrimaryActionCalls;
    g_loadGamePrimaryActionThis = reinterpret_cast<HudUiLoadGameDialog *>(this);
    if (g_saveLoadActionOrder == 0) {
        g_saveLoadActionOrder = ++g_saveLoadCallOrder;
    }
}

int __fastcall SaveLoadFileExistsProbe(
    const char *path
) {
    ++g_saveLoadFileExistsCalls;
    std::strncpy(
        g_saveLoadFileExistsPath,
        path != nullptr ? path : "",
        sizeof(g_saveLoadFileExistsPath)
    );
    g_saveLoadFileExistsPath[sizeof(g_saveLoadFileExistsPath) - 1] = '\0';
    return 1;
}

int __fastcall SaveLoadZarLoadProbe(
    const char *path
) {
    ++g_saveLoadZarLoadCalls;
    std::strncpy(
        g_saveLoadZarLoadPath,
        path != nullptr ? path : "",
        sizeof(g_saveLoadZarLoadPath)
    );
    g_saveLoadZarLoadPath[sizeof(g_saveLoadZarLoadPath) - 1] = '\0';
    return 1;
}

RecoilApp_IState * RecoilAppQueueProbe::QueueExitCurrentState(
    int stateParam
) {
    const int index = g_saveLoadQueueExitCalls;
    if (index < (int)(sizeof(g_saveLoadQueueExitParams) /
                      sizeof(g_saveLoadQueueExitParams[0]))) {
        g_saveLoadQueueExitParams[index] = stateParam;
    }
    ++g_saveLoadQueueExitCalls;
    return nullptr;
}

RecoilApp_IState * RecoilAppQueueProbe::QueueSwitchCurrentState(
    RecoilApp_IState *state,
    int stateParam
) {
    const int index = g_saveLoadQueueSwitchCalls;
    if (index < (int)(sizeof(g_saveLoadQueueSwitchParams) /
                      sizeof(g_saveLoadQueueSwitchParams[0]))) {
        g_saveLoadQueueSwitchStates[index] = state;
        g_saveLoadQueueSwitchParams[index] = stateParam;
    }
    ++g_saveLoadQueueSwitchCalls;
    return nullptr;
}

void ResetSaveLoadActionCapture() {
    g_saveLoadSetSelectedCalls = 0;
    g_saveLoadSetSelectedThis = nullptr;
    g_saveLoadSetSelectedIndex = -99;
    g_saveLoadInitializeCalls = 0;
    g_saveLoadInitializeThis = nullptr;
    g_saveLoadRefreshCalls = 0;
    g_saveLoadRefreshThis = nullptr;
    g_saveLoadDeleteCalls = 0;
    g_saveLoadDeleteThis = nullptr;
    g_saveLoadDeleteConfirm = -99;
    g_saveLoadProcessCalls = 0;
    g_saveLoadProcessThis = nullptr;
    g_loadGamePrimaryActionCalls = 0;
    g_loadGamePrimaryActionThis = nullptr;
    g_saveLoadFileExistsCalls = 0;
    g_saveLoadFileExistsPath[0] = '\0';
    g_saveLoadZarLoadCalls = 0;
    g_saveLoadZarLoadPath[0] = '\0';
    g_saveLoadQueueExitCalls = 0;
    std::memset(
        g_saveLoadQueueExitParams,
        0,
        sizeof(g_saveLoadQueueExitParams)
    );
    g_saveLoadQueueSwitchCalls = 0;
    std::memset(
        g_saveLoadQueueSwitchStates,
        0,
        sizeof(g_saveLoadQueueSwitchStates)
    );
    std::memset(
        g_saveLoadQueueSwitchParams,
        0,
        sizeof(g_saveLoadQueueSwitchParams)
    );
    g_saveLoadCallOrder = 0;
    g_saveLoadActionOrder = 0;
}

void ResetSaveLoadInitCapture() {
    g_saveGameInitLoadCalls = 0;
    g_saveGameInitLoadArgsOk = false;
    g_loadGameInitLoadCalls = 0;
    g_loadGameInitLoadArgsOk = false;
}

void *SaveLoadDialogInitializeFileEntriesAddress() {
    return MethodAddress(&HudUiSaveLoadDialog::InitializeFileEntries);
}

void *SaveLoadDialogInitializeFileEntriesProbeAddress() {
    return MethodAddress(&SaveLoadDialogActionProbe::InitializeFileEntries);
}

void *SaveLoadDialogSetSelectedAddress() {
    return MethodAddress(&HudUiSaveLoadDialog::SetSelectedEntryIndex);
}

void *SaveLoadDialogSetSelectedProbeAddress() {
    return MethodAddress(&SaveLoadDialogActionProbe::SetSelectedEntryIndex);
}

void *SaveLoadDialogRefreshAddress() {
    return MethodAddress(&HudUiSaveLoadDialog::RefreshSaveFileList);
}

void *SaveLoadDialogRefreshProbeAddress() {
    return MethodAddress(&SaveLoadDialogActionProbe::RefreshSaveFileList);
}

void *SaveLoadDialogDeleteSaveFileAddress() {
    return MethodAddress(&HudUiSaveLoadDialog::DeleteSaveFile);
}

void *SaveLoadDialogDeleteSaveFileProbeAddress() {
    return MethodAddress(&SaveLoadDialogActionProbe::DeleteSaveFile);
}

void *SaveLoadDialogProcessDialogResultAddress() {
    return MethodAddress(&HudUiSaveLoadDialog::ProcessDialogResult);
}

void *SaveLoadDialogProcessDialogResultProbeAddress() {
    return MethodAddress(&SaveLoadDialogActionProbe::ProcessDialogResult);
}

void *LoadGameDialogOnPrimaryActionAddress() {
    return MethodAddress(&HudUiLoadGameDialog::OnPrimaryAction);
}

void *LoadGameDialogOnPrimaryActionProbeAddress() {
    return MethodAddress(&LoadGameDialogActionProbe::OnPrimaryAction);
}

void *RecoilAppQueueExitAddress() {
    return MethodAddress(&RecoilApp::QueueExitCurrentState);
}

void *RecoilAppQueueExitProbeAddress() {
    return MethodAddress(&RecoilAppQueueProbe::QueueExitCurrentState);
}

void *RecoilAppQueueSwitchAddress() {
    return MethodAddress(&RecoilApp::QueueSwitchCurrentState);
}

void *RecoilAppQueueSwitchProbeAddress() {
    return MethodAddress(&RecoilAppQueueProbe::QueueSwitchCurrentState);
}

void SetWriteTime(
    HudUiSaveLoadEntry *entry,
    DWORD lowPart
) {
    std::memset(
        entry,
        0,
        sizeof(*entry)
    );
    entry->ftLastWriteTime.dwLowDateTime = lowPart;
    entry->ftLastWriteTime.dwHighDateTime = 0;
}

FILETIME MakeSaveLoadSmokeFileTime(
    DWORD lowPart
) {
    FILETIME time{};
    time.dwLowDateTime = lowPart;
    time.dwHighDateTime = 30000000;
    return time;
}

bool WriteSaveLoadSmokeFile(
    const char *path,
    DWORD timeLowPart
) {
    HANDLE file = CreateFileA(
        path,
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }

    DWORD bytesWritten = 0;
    const char payload[] = "save";
    const BOOL writeOk = WriteFile(
        file,
        payload,
        static_cast<DWORD>(sizeof(payload) - 1),
        &bytesWritten,
        nullptr
    );
    const FILETIME writeTime = MakeSaveLoadSmokeFileTime(timeLowPart);
    const BOOL timeOk = SetFileTime(
        file,
        nullptr,
        nullptr,
        &writeTime
    );
    CloseHandle(file);

    return writeOk != 0 && timeOk != 0 && bytesWritten == sizeof(payload) - 1;
}

void CleanupSaveLoadSmokeFiles(
    bool removeDirectory
) {
    DeleteFileA("SavedGames\\recoil_refresh_smoke_old.sav");
    DeleteFileA("SavedGames\\recoil_refresh_smoke_new.sav");
    DeleteFileA("SavedGames\\recoil_refresh_smoke_middle.sav");
    DeleteFileA("SavedGames\\recoil_refresh_smoke_oldest.sav");
    RemoveDirectoryA("SavedGames\\recoil_refresh_smoke_dir");
    if (removeDirectory) {
        RemoveDirectoryA("SavedGames");
    }
}

} // namespace

extern "C" int hud_ui_save_load_entry_is_newer_than_smoke(void) {
    HudUiSaveLoadEntry older;
    HudUiSaveLoadEntry same;
    HudUiSaveLoadEntry newer;

    SetWriteTime(
        &older,
        100
    );
    same = older;
    SetWriteTime(
        &newer,
        101
    );

    return (newer < older) == 1 && (older < newer) == 0 &&
                   (older < same) == 0
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_list_item_constructor_smoke(void) {
    HudUiSaveLoadListItem item{};

    return item.layoutY == 32767 && item.layoutX == -1 &&
                   item.parent == 0 && item.next == 0 && item.textPick == 0
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_list_item_draw_smoke(void) {
    TestSaveLoadListItem item{};

    ResetSaveLoadListItemDrawCapture();
    item.Draw();

    return g_saveLoadListItemUpdateBoundsCount == 1 &&
                   g_saveLoadListItemUpdateBoundsThis == &item
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_list_item_on_activate_smoke(void) {
    CodeFunctionPatch setSelectedPatch{};
    if (!PatchFunctionJump(
            SaveLoadDialogSetSelectedAddress(),
            SaveLoadDialogSetSelectedProbeAddress(),
            setSelectedPatch
        )) {
        return 1;
    }

    HudUiSaveLoadDialog dialog{};
    HudUiSaveLoadListItem item{};
    item.parent = &dialog;
    item.layoutX = 4;

    ResetSaveLoadActionCapture();
    item.OnActivate();
    const bool selected = g_saveLoadSetSelectedCalls == 1 &&
                          g_saveLoadSetSelectedThis == &dialog &&
                          g_saveLoadSetSelectedIndex == 4;

    ResetSaveLoadActionCapture();
    item.parent = nullptr;
    item.OnActivate();
    const bool ignoredNullParent = g_saveLoadSetSelectedCalls == 0;

    RestoreFunctionPatch(setSelectedPatch);
    return selected && ignoredNullParent ? 0 : 1;
}

extern "C" int hud_ui_save_load_delete_button_on_activate_smoke(void) {
    CodeFunctionPatch deletePatch{};
    if (!PatchFunctionJump(
            SaveLoadDialogDeleteSaveFileAddress(),
            SaveLoadDialogDeleteSaveFileProbeAddress(),
            deletePatch
        )) {
        RestoreFunctionPatch(deletePatch);
        return 1;
    }

    HudUiSaveLoadDialog dialog{};
    HudUiSaveLoadDeleteButton button{};
    button.owner = &dialog;

    ResetSaveLoadActionCapture();
    button.OnActivate();
    const bool result = g_saveLoadDeleteCalls == 1 &&
                        g_saveLoadDeleteThis == &dialog &&
                        g_saveLoadDeleteConfirm == 1;

    RestoreFunctionPatch(deletePatch);
    return result ? 0 : 1;
}

extern "C" int hud_ui_save_load_delete_save_file_smoke(void) {
    CodeFunctionPatch fileExistsPatch{};
    CodeFunctionPatch refreshPatch{};
    CodeFunctionPatch setSelectedPatch{};
    const bool patched =
        PatchFunctionJump(
            reinterpret_cast<void *>(&zReader::FileExists),
            reinterpret_cast<void *>(&SaveLoadFileExistsProbe),
            fileExistsPatch
        ) &&
        PatchFunctionJump(
            SaveLoadDialogRefreshAddress(),
            SaveLoadDialogRefreshProbeAddress(),
            refreshPatch
        ) &&
        PatchFunctionJump(
            SaveLoadDialogSetSelectedAddress(),
            SaveLoadDialogSetSelectedProbeAddress(),
            setSelectedPatch
        );
    if (!patched) {
        RestoreFunctionPatch(setSelectedPatch);
        RestoreFunctionPatch(refreshPatch);
        RestoreFunctionPatch(fileExistsPatch);
        return 1;
    }

    HudUiSaveLoadEntry entries[2]{};
    void *const storage = ::operator new(sizeof(HudUiSaveLoadDialog));
    HudUiSaveLoadDialog *const dialog =
        static_cast<HudUiSaveLoadDialog *>(storage);
    std::memset(
        dialog,
        0,
        sizeof(*dialog)
    );
    dialog->gameNameInput.BaseConstructor();
    dialog->gameNameInput.AllocTextBuffer(32);
    dialog->gameNameInput.Update("delete_me.sav");
    dialog->fileEntries.begin = entries;
    dialog->fileEntries.end = entries + 2;
    dialog->fileEntries.capacityEnd = entries + 2;
    dialog->selectedEntryIndex = 5;

    ResetSaveLoadActionCapture();
    dialog->DeleteSaveFile(0);

    const bool result =
        g_saveLoadFileExistsCalls == 1 &&
        std::strcmp(
            g_saveLoadFileExistsPath,
            "SavedGames\\delete_me.sav"
        ) == 0 &&
        std::strcmp(
            dialog->gameNameInput.GetBuffer(),
            ""
        ) == 0 &&
        g_saveLoadRefreshCalls == 1 &&
        g_saveLoadRefreshThis == dialog &&
        g_saveLoadSetSelectedCalls == 1 &&
        g_saveLoadSetSelectedThis == dialog &&
        g_saveLoadSetSelectedIndex == 1;

    dialog->gameNameInput.Destructor();
    ::operator delete(storage);
    RestoreFunctionPatch(setSelectedPatch);
    RestoreFunctionPatch(refreshPatch);
    RestoreFunctionPatch(fileExistsPatch);
    return result ? 0 : 1;
}

extern "C" int hud_ui_save_load_next_button_on_activate_smoke(void) {
    CodeFunctionPatch setSelectedPatch{};
    if (!PatchFunctionJump(
            SaveLoadDialogSetSelectedAddress(),
            SaveLoadDialogSetSelectedProbeAddress(),
            setSelectedPatch
        )) {
        RestoreFunctionPatch(setSelectedPatch);
        return 1;
    }

    HudUiSaveLoadEntry entries[3]{};
    HudUiSaveLoadDialog dialog{};
    dialog.fileEntries.begin = entries;
    dialog.fileEntries.end = entries + 3;
    dialog.fileEntries.capacityEnd = entries + 3;
    dialog.selectedEntryIndex = 1;

    HudUiSaveLoadNextButton button{};
    button.owner = &dialog;

    ResetSaveLoadActionCapture();
    button.OnActivate();
    const bool selectedNext = g_saveLoadSetSelectedCalls == 1 &&
                              g_saveLoadSetSelectedThis == &dialog &&
                              g_saveLoadSetSelectedIndex == 2;

    dialog.selectedEntryIndex = 2;
    ResetSaveLoadActionCapture();
    button.OnActivate();
    const bool clampedAtEnd = g_saveLoadSetSelectedCalls == 0;

    RestoreFunctionPatch(setSelectedPatch);
    return selectedNext && clampedAtEnd ? 0 : 1;
}

extern "C" int hud_ui_save_load_prev_button_on_activate_smoke(void) {
    CodeFunctionPatch setSelectedPatch{};
    if (!PatchFunctionJump(
            SaveLoadDialogSetSelectedAddress(),
            SaveLoadDialogSetSelectedProbeAddress(),
            setSelectedPatch
        )) {
        RestoreFunctionPatch(setSelectedPatch);
        return 1;
    }

    HudUiSaveLoadEntry entries[3]{};
    HudUiSaveLoadDialog dialog{};
    dialog.fileEntries.begin = entries;
    dialog.fileEntries.end = entries + 3;
    dialog.fileEntries.capacityEnd = entries + 3;
    dialog.selectedEntryIndex = 1;

    HudUiSaveLoadPrevButton button{};
    button.owner = &dialog;

    ResetSaveLoadActionCapture();
    button.OnActivate();
    const bool selectedPrev = g_saveLoadSetSelectedCalls == 1 &&
                              g_saveLoadSetSelectedThis == &dialog &&
                              g_saveLoadSetSelectedIndex == 0;

    dialog.selectedEntryIndex = 0;
    ResetSaveLoadActionCapture();
    button.OnActivate();
    const bool clampedAtStart = g_saveLoadSetSelectedCalls == 0;

    RestoreFunctionPatch(setSelectedPatch);
    return selectedPrev && clampedAtStart ? 0 : 1;
}

extern "C" int hud_ui_save_game_primary_action_button_on_activate_smoke(void) {
    CodeFunctionPatch processPatch{};
    if (!PatchFunctionJump(
            SaveLoadDialogProcessDialogResultAddress(),
            SaveLoadDialogProcessDialogResultProbeAddress(),
            processPatch
        )) {
        RestoreFunctionPatch(processPatch);
        return 1;
    }

    HudUiSaveLoadDialog dialog{};
    HudUiSaveGamePrimaryActionButton button{};
    button.owner = &dialog;

    ResetSaveLoadActionCapture();
    button.OnActivate();
    const bool processThenActivate = g_saveLoadProcessCalls == 1 &&
                                     g_saveLoadProcessThis == &dialog;

    ResetSaveLoadActionCapture();
    button.owner = nullptr;
    button.OnActivate();
    const bool nullOwnerIgnored = g_saveLoadProcessCalls == 0;

    RestoreFunctionPatch(processPatch);
    return processThenActivate && nullOwnerIgnored ? 0 : 1;
}

extern "C" int hud_ui_load_game_dialog_constructor_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch initializePatch{};
    CodeFunctionPatch setSelectedPatch{};
    const bool patched =
        PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            LoadGameInitLoadProbeAddress(),
            loadPatch
        ) &&
        PatchFunctionJump(
            SaveLoadDialogInitializeFileEntriesAddress(),
            SaveLoadDialogInitializeFileEntriesProbeAddress(),
            initializePatch
        ) &&
        PatchFunctionJump(
            SaveLoadDialogSetSelectedAddress(),
            SaveLoadDialogSetSelectedProbeAddress(),
            setSelectedPatch
        );
    if (!patched) {
        RestoreFunctionPatch(setSelectedPatch);
        RestoreFunctionPatch(initializePatch);
        RestoreFunctionPatch(loadPatch);
        return 1;
    }

    void *const storage = ::operator new(sizeof(HudUiLoadGameDialog));
    HudUiLoadGameDialog *dialog = nullptr;

    ResetSaveLoadInitCapture();
    ResetSaveLoadActionCapture();
    dialog = new (storage) HudUiLoadGameDialog;

    const bool result =
        dialog == static_cast<HudUiLoadGameDialog *>(storage) &&
        g_loadGameInitLoadCalls == 1 &&
        g_loadGameInitLoadArgsOk &&
        dialog->gameNameInput.GetBuffer() != nullptr &&
        std::strcmp(
            dialog->gameNameInput.GetBuffer(),
            ""
        ) == 0 &&
        dialog->gameNameInput.textInput.capacity == 20 &&
        dialog->fileEntries.begin == nullptr &&
        dialog->fileEntries.end == nullptr &&
        dialog->fileEntries.capacityEnd == nullptr &&
        g_saveLoadInitializeCalls == 1 &&
        g_saveLoadInitializeThis == dialog &&
        g_saveLoadSetSelectedCalls == 1 &&
        g_saveLoadSetSelectedThis == dialog &&
        g_saveLoadSetSelectedIndex == 0;

    dialog->Destructor();
    ::operator delete(storage);
    RestoreFunctionPatch(setSelectedPatch);
    RestoreFunctionPatch(initializePatch);
    RestoreFunctionPatch(loadPatch);
    return result ? 0 : 1;
}

extern "C" int hud_ui_load_game_primary_action_button_on_activate_smoke(void) {
    CodeFunctionPatch primaryPatch{};
    if (!PatchFunctionJump(
            LoadGameDialogOnPrimaryActionAddress(),
            LoadGameDialogOnPrimaryActionProbeAddress(),
            primaryPatch
        )) {
        RestoreFunctionPatch(primaryPatch);
        return 1;
    }

    void *const storage = ::operator new(sizeof(HudUiLoadGameDialog));
    HudUiLoadGameDialog *const dialog =
        static_cast<HudUiLoadGameDialog *>(storage);
    std::memset(
        dialog,
        0,
        sizeof(*dialog)
    );
    HudUiLoadGamePrimaryActionButton button{};
    button.owner = dialog;

    ResetSaveLoadActionCapture();
    button.OnActivate();
    const bool primaryThenActivate = g_loadGamePrimaryActionCalls == 1 &&
                                     g_loadGamePrimaryActionThis == dialog;

    ResetSaveLoadActionCapture();
    button.owner = nullptr;
    button.OnActivate();
    const bool nullOwnerIgnored = g_loadGamePrimaryActionCalls == 0;

    RestoreFunctionPatch(primaryPatch);
    ::operator delete(storage);
    return primaryThenActivate && nullOwnerIgnored ? 0 : 1;
}

extern "C" int hud_ui_zrd_widget_on_activate_queue_exit_current_state_smoke(void) {
    unsigned char oldApp[sizeof(g_RecoilApp)];
    std::memcpy(
        oldApp,
        &g_RecoilApp,
        sizeof(oldApp)
    );
    std::memset(
        &g_RecoilApp,
        0,
        sizeof(g_RecoilApp)
    );

    SaveLoadTestAppState oldState;
    g_RecoilApp.m_currentStateIndex = 0;
    g_RecoilApp.m_stateStack[0] = &oldState;
    g_saveLoadStateEnterCount = 0;
    g_saveLoadStateExitCount = 0;

    HudUiCreditsBackButton widget{};
    widget.Constructor();
    widget.OnActivate();

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    int result = 0;
    if (g_saveLoadStateExitCount != 1 ||
        g_saveLoadStateEnterCount != 0) {
        result = 1;
    } else if (!SaveLoadQueueHasSingleExit(
                   queue,
                   0
               )) {
        result = 2;
    }

    CleanupSaveLoadQueue(queue);
    widget.DestructorCore();
    std::memcpy(
        &g_RecoilApp,
        oldApp,
        sizeof(g_RecoilApp)
    );
    return result;
}

extern "C" int hud_ui_credits_quit_button_on_activate_smoke(void) {
    unsigned char oldApp[sizeof(g_RecoilApp)];
    std::memcpy(
        oldApp,
        &g_RecoilApp,
        sizeof(oldApp)
    );
    std::memset(
        &g_RecoilApp,
        0,
        sizeof(g_RecoilApp)
    );

    SaveLoadTestAppState oldState;
    SaveLoadTestAppState leaveState;
    g_RecoilApp.m_currentStateIndex = 0;
    g_RecoilApp.m_stateStack[0] = &oldState;
    InstallSaveLoadStateVptr(
        g_RecoilApp.m_leaveNetworkState,
        leaveState
    );
    g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_ON_EXIT;
    g_saveLoadStateEnterCount = 0;
    g_saveLoadStateExitCount = 0;

    HudUiCreditsQuitButton widget{};
    widget.Constructor();
    widget.OnActivate();

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    RecoilApp_StateQueueItem *const exitItem = SaveLoadQueueItemAt(
        queue,
        0
    );
    RecoilApp_StateQueueItem *const switchItem = SaveLoadQueueItemAt(
        queue,
        1
    );

    int result = 0;
    if (g_saveLoadStateExitCount != 2 ||
        g_saveLoadStateEnterCount != 1 ||
        g_RecoilApp.m_missionShutdownMode !=
            RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY ||
        queue.m_itemCount != 2) {
        result = 1;
    } else if (exitItem == nullptr ||
               exitItem->m_kind != RecoilApp_StateQueueKind_ExitCurrent ||
               exitItem->m_param != 1) {
        result = 2;
    } else if (switchItem == nullptr ||
               switchItem->m_kind != RecoilApp_StateQueueKind_SwitchCurrent ||
               switchItem->m_stateObj != &g_RecoilApp.m_leaveNetworkState ||
               switchItem->m_param != 0) {
        result = 3;
    }

    CleanupSaveLoadQueue(queue);
    widget.DestructorCore();
    std::memcpy(
        &g_RecoilApp,
        oldApp,
        sizeof(g_RecoilApp)
    );
    return result;
}

extern "C" int hud_ui_confirm_quit_ok_button_on_activate_smoke(void) {
    unsigned char oldApp[sizeof(g_RecoilApp)];
    std::memcpy(
        oldApp,
        &g_RecoilApp,
        sizeof(oldApp)
    );
    const int oldSkipExitDelay = g_RecoilState_MainMenuSkipExitDelay;
    std::memset(
        &g_RecoilApp,
        0,
        sizeof(g_RecoilApp)
    );

    SaveLoadTestAppState currentState;
    SaveLoadTestAppState leaveState;
    g_RecoilApp.m_currentStateIndex = 0;
    g_RecoilApp.m_stateStack[0] = &currentState;
    InstallSaveLoadStateVptr(
        g_RecoilApp.m_leaveNetworkState,
        leaveState
    );
    g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_ON_EXIT;
    g_RecoilState_MainMenuSkipExitDelay = 0;
    g_saveLoadStateEnterCount = 0;
    g_saveLoadStateExitCount = 0;

    HudUiConfirmQuitOkButton button{};
    button.OnActivate();

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    RecoilApp_StateQueueItem *const firstItem = SaveLoadQueueItemAt(
        queue,
        0
    );
    RecoilApp_StateQueueItem *const secondItem = SaveLoadQueueItemAt(
        queue,
        1
    );
    RecoilApp_StateQueueItem *const thirdItem = SaveLoadQueueItemAt(
        queue,
        2
    );

    int result = 0;
    if (g_RecoilState_MainMenuSkipExitDelay != 1 ||
        g_RecoilApp.m_missionShutdownMode !=
            RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY ||
        g_saveLoadStateExitCount != 3 ||
        g_saveLoadStateEnterCount != 1 ||
        queue.m_itemCount != 3) {
        result = 1;
    } else if (firstItem == nullptr ||
               firstItem->m_kind != RecoilApp_StateQueueKind_ExitCurrent ||
               firstItem->m_param != 1) {
        result = 2;
    } else if (secondItem == nullptr ||
               secondItem->m_kind != RecoilApp_StateQueueKind_ExitCurrent ||
               secondItem->m_param != 0) {
        result = 3;
    } else if (thirdItem == nullptr ||
               thirdItem->m_kind != RecoilApp_StateQueueKind_SwitchCurrent ||
               thirdItem->m_stateObj != &g_RecoilApp.m_leaveNetworkState ||
               thirdItem->m_param != 0) {
        result = 4;
    }

    CleanupSaveLoadQueue(queue);
    button.DestructorCore();
    g_RecoilState_MainMenuSkipExitDelay = oldSkipExitDelay;
    std::memcpy(
        &g_RecoilApp,
        oldApp,
        sizeof(g_RecoilApp)
    );
    return result;
}

extern "C" int hud_ui_background_confirm_quit_lifecycle_smoke(void) {
    CodeFunctionPatch loadPatch{};
    CodeFunctionPatch bindPatch{};
    CodeFunctionPatch freePatch{};

    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            ConfirmQuitBackgroundLoadProbeAddress(),
            loadPatch
        )) {
        return 1;
    }
    if (!PatchFunctionJump(
            HudUiBackgroundBindWidgetByNameAddress(),
            ConfirmQuitBackgroundBindProbeAddress(),
            bindPatch
        )) {
        RestoreFunctionPatch(loadPatch);
        return 2;
    }
    if (!PatchFunctionJump(
            HudUiBackgroundFreeLoadedTreeRootsAddress(),
            ConfirmQuitBackgroundFreeProbeAddress(),
            freePatch
        )) {
        RestoreFunctionPatch(bindPatch);
        RestoreFunctionPatch(loadPatch);
        return 3;
    }

    g_confirmQuitBackgroundLoadCalls = 0;
    g_confirmQuitBackgroundLoadArgsOk = false;
    g_confirmQuitBackgroundBindCalls = 0;
    g_confirmQuitBackgroundBindArgsOk = true;
    g_confirmQuitBackgroundFreeCalls = 0;
    g_confirmQuitBackgroundFreeArgsOk = false;

    void *const storage = ::operator new(sizeof(HudUiBackgroundConfirmQuit));
    HudUiBackgroundConfirmQuit *const dialog =
        static_cast<HudUiBackgroundConfirmQuit *>(storage);
    HudUiBackgroundConfirmQuit *const returned = dialog->Constructor();

    int result = 0;
    if (returned != dialog ||
        g_confirmQuitBackgroundLoadCalls != 1 ||
        !g_confirmQuitBackgroundLoadArgsOk ||
        g_confirmQuitBackgroundBindCalls != 2 ||
        !g_confirmQuitBackgroundBindArgsOk ||
        g_confirmQuitBackgroundFreeCalls != 1 ||
        !g_confirmQuitBackgroundFreeArgsOk) {
        result = 4;
    }

    HudUiBackground *const deleted = dialog->ScalarDeletingDestructor(0);
    if (result == 0 && deleted != dialog) {
        result = 5;
    }

    ::operator delete(storage);
    RestoreFunctionPatch(freePatch);
    RestoreFunctionPatch(bindPatch);
    RestoreFunctionPatch(loadPatch);
    return result;
}

extern "C" int hud_ui_load_game_dialog_on_primary_action_smoke(void) {
    unsigned char oldApp[sizeof(g_RecoilApp)];
    std::memcpy(
        oldApp,
        &g_RecoilApp,
        sizeof(oldApp)
    );
    std::memset(
        &g_RecoilApp,
        0,
        sizeof(g_RecoilApp)
    );

    void *const storage = ::operator new(sizeof(HudUiLoadGameDialog));
    HudUiLoadGameDialog *const dialog =
        static_cast<HudUiLoadGameDialog *>(storage);
    std::memset(
        dialog,
        0,
        sizeof(*dialog)
    );
    char gameNameBuffer[1] = {};
    SaveLoadSetEmptyGameName(
        dialog,
        gameNameBuffer,
        sizeof(gameNameBuffer)
    );

    dialog->OnPrimaryAction();
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    const bool emptyNameQueuedExit = SaveLoadQueueHasSingleExit(
        queue,
        0
    );

    CleanupSaveLoadQueue(queue);
    ::operator delete(storage);
    std::memcpy(
        &g_RecoilApp,
        oldApp,
        sizeof(g_RecoilApp)
    );

    return emptyNameQueuedExit ? 0 : 1;
}

extern "C" int hud_ui_load_game_dialog_on_primary_action_thunk_smoke(void) {
    unsigned char oldApp[sizeof(g_RecoilApp)];
    std::memcpy(
        oldApp,
        &g_RecoilApp,
        sizeof(oldApp)
    );
    std::memset(
        &g_RecoilApp,
        0,
        sizeof(g_RecoilApp)
    );

    void *const storage = ::operator new(sizeof(HudUiLoadGameDialog));
    HudUiLoadGameDialog *const dialog =
        static_cast<HudUiLoadGameDialog *>(storage);
    std::memset(
        dialog,
        0,
        sizeof(*dialog)
    );
    char gameNameBuffer[1] = {};
    SaveLoadSetEmptyGameName(
        dialog,
        gameNameBuffer,
        sizeof(gameNameBuffer)
    );

    dialog->OnPrimaryActionThunk();
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    const bool emptyNameQueuedExit = SaveLoadQueueHasSingleExit(
        queue,
        0
    );

    CleanupSaveLoadQueue(queue);
    ::operator delete(storage);
    std::memcpy(
        &g_RecoilApp,
        oldApp,
        sizeof(g_RecoilApp)
    );

    return emptyNameQueuedExit ? 0 : 1;
}

extern "C" int hud_ui_load_game_dialog_process_dialog_result_smoke(void) {
    CodeFunctionPatch processPatch{};
    if (!PatchFunctionJump(
            SaveLoadDialogProcessDialogResultAddress(),
            SaveLoadDialogProcessDialogResultProbeAddress(),
            processPatch
        )) {
        RestoreFunctionPatch(processPatch);
        return 1;
    }

    void *const storage = ::operator new(sizeof(HudUiLoadGameDialog));
    HudUiLoadGameDialog *const dialog =
        static_cast<HudUiLoadGameDialog *>(storage);
    std::memset(
        dialog,
        0,
        sizeof(*dialog)
    );

    ResetSaveLoadActionCapture();
    dialog->ProcessDialogResult();
    const bool result = g_saveLoadProcessCalls == 1 &&
                        g_saveLoadProcessThis == dialog;

    ::operator delete(storage);
    RestoreFunctionPatch(processPatch);
    return result ? 0 : 1;
}

extern "C" int hud_ui_save_load_process_dialog_result_smoke(void) {
    CodeFunctionPatch fileExistsPatch{};
    CodeFunctionPatch zarLoadPatch{};
    CodeFunctionPatch queueExitPatch{};
    CodeFunctionPatch queueSwitchPatch{};
    bool patched =
        PatchFunctionJump(
            reinterpret_cast<void *>(&zReader::FileExists),
            reinterpret_cast<void *>(&SaveLoadFileExistsProbe),
            fileExistsPatch
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zUtil::ZAR_LoadFileGlobal),
            reinterpret_cast<void *>(&SaveLoadZarLoadProbe),
            zarLoadPatch
        ) &&
        PatchFunctionJump(
            RecoilAppQueueExitAddress(),
            RecoilAppQueueExitProbeAddress(),
            queueExitPatch
        ) &&
        PatchFunctionJump(
            RecoilAppQueueSwitchAddress(),
            RecoilAppQueueSwitchProbeAddress(),
            queueSwitchPatch
        );
    if (!patched) {
        RestoreFunctionPatch(queueSwitchPatch);
        RestoreFunctionPatch(queueExitPatch);
        RestoreFunctionPatch(zarLoadPatch);
        RestoreFunctionPatch(fileExistsPatch);
        return 1;
    }

    unsigned char oldApp[sizeof(g_RecoilApp)];
    RecoilStateSaveLoadTransition oldSaveLoadTransition =
        g_RecoilStateSaveLoadTransition;
    RecoilStateMainMenuTransition oldMainMenuTransition =
        g_RecoilState_MainMenuTransition;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int *const oldMuteOption = ZOPT_MUTE_SOUND;
    std::memcpy(
        oldApp,
        &g_RecoilApp,
        sizeof(oldApp)
    );

    int joystickOption = 0;
    int gameControlOptions = 0;
    int muteOption = 0;
    ZOPT_INPUT_JOYSTICK = &joystickOption;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    ZOPT_MUTE_SOUND = &muteOption;

    void *const storage = ::operator new(sizeof(HudUiSaveLoadDialog));
    HudUiSaveLoadDialog *const dialog =
        static_cast<HudUiSaveLoadDialog *>(storage);
    std::memset(
        dialog,
        0,
        sizeof(*dialog)
    );
    char gameNameBuffer[64] = "recoil_process_smoke.sav";
    dialog->gameNameInput.textInput.buffer = gameNameBuffer;
    dialog->gameNameInput.textInput.capacity = sizeof(gameNameBuffer);
    dialog->gameNameInput.textInput.cursor = 0;

    std::memset(
        &g_RecoilApp,
        0,
        sizeof(g_RecoilApp)
    );
    g_RecoilStateSaveLoadTransition = RecoilStateSaveLoadTransition{};
    g_RecoilStateSaveLoadTransition.m_transitionMode =
        RECOIL_SAVELOAD_MODE_STANDARD;
    g_RecoilState_MainMenuTransition = RecoilStateMainMenuTransition{};
    ResetSaveLoadActionCapture();

    dialog->ProcessDialogResult();

    char *const pendingPath = g_RecoilApp.m_playState.pPendingLoadGameStartPath;
    const bool standardOk =
        g_saveLoadFileExistsCalls == 1 &&
        std::strcmp(
            g_saveLoadFileExistsPath,
            "SavedGames\\recoil_process_smoke.sav"
        ) == 0 &&
        g_saveLoadZarLoadCalls == 1 &&
        std::strcmp(
            g_saveLoadZarLoadPath,
            "SavedGames\\recoil_process_smoke.sav"
        ) == 0 &&
        pendingPath != nullptr &&
        std::strcmp(
            pendingPath,
            "SavedGames\\recoil_process_smoke.sav"
        ) == 0 &&
        g_RecoilApp.m_missionFmvState.m_skipMissionFmv == 1 &&
        g_saveLoadQueueExitCalls == 1 &&
        g_saveLoadQueueExitParams[0] == 1 &&
        g_saveLoadQueueSwitchCalls == 1 &&
        g_saveLoadQueueSwitchStates[0] == &g_RecoilApp.m_missionFmvState &&
        g_saveLoadQueueSwitchParams[0] == 0;
    std::free(pendingPath);
    g_RecoilApp.m_playState.pPendingLoadGameStartPath = nullptr;

    std::memset(
        &g_RecoilApp,
        0,
        sizeof(g_RecoilApp)
    );
    g_RecoilStateSaveLoadTransition = RecoilStateSaveLoadTransition{};
    g_RecoilStateSaveLoadTransition.m_transitionMode = RECOIL_SAVELOAD_MODE_FADE;
    muteOption = 0;
    ResetSaveLoadActionCapture();

    dialog->ProcessDialogResult();

    const bool fadeOk =
        g_RecoilApp.m_transitionFadeTimer == 5.0f &&
        muteOption == 1 &&
        g_saveLoadQueueExitCalls == 2 &&
        g_saveLoadQueueExitParams[0] == 1 &&
        g_saveLoadQueueExitParams[1] == 1 &&
        g_saveLoadQueueSwitchCalls == 0;

    ::operator delete(storage);
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    ZOPT_MUTE_SOUND = oldMuteOption;
    g_RecoilStateSaveLoadTransition = oldSaveLoadTransition;
    g_RecoilState_MainMenuTransition = oldMainMenuTransition;
    std::memcpy(
        &g_RecoilApp,
        oldApp,
        sizeof(g_RecoilApp)
    );
    RestoreFunctionPatch(queueSwitchPatch);
    RestoreFunctionPatch(queueExitPatch);
    RestoreFunctionPatch(zarLoadPatch);
    RestoreFunctionPatch(fileExistsPatch);

    return standardOk && fadeOk ? 0 : 1;
}

extern "C" int hud_ui_save_load_game_name_input_raw_keyboard_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiSaveLoadGameNameInput));
    HudUiSaveLoadGameNameInput *const input =
        static_cast<HudUiSaveLoadGameNameInput *>(storage);
    input->BaseConstructor();
    input->AllocTextBuffer(16);
    input->Update("");

    const int allowedResult = input->OnRawKeyboardEvent('A');
    const bool allowedInserted = allowedResult == 0 &&
                                 std::strcmp(
                                     input->GetBuffer(),
                                     "A"
                                 ) == 0 &&
                                 input->textInput.cursor == 1;

    const int filteredResult = input->OnRawKeyboardEvent('#');
    const bool filtered = filteredResult == 0 &&
                          std::strcmp(
                              input->GetBuffer(),
                              "A"
                          ) == 0 &&
                          input->textInput.cursor == 1;

    input->OnRawKeyboardEvent('_');
    const bool underscoreInserted = std::strcmp(
                                        input->GetBuffer(),
                                        "A_"
                                    ) == 0 &&
                                    input->textInput.cursor == 2;

    input->Destructor();
    ::operator delete(storage);
    return allowedInserted && filtered && underscoreInserted ? 0 : 1;
}

extern "C" int hud_ui_save_load_game_name_input_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiSaveLoadGameNameInput));
    HudUiSaveLoadGameNameInput *const input =
        static_cast<HudUiSaveLoadGameNameInput *>(storage);
    input->BaseConstructor();
    input->AllocTextBuffer(16);
    input->Update("SAVE1");
    input->textInput.cursor = 0;
    input->sliderBorder.inputActive = 0;

    input->HudUiSaveLoadGameNameInput::OnActivate();
    int result = 0;
    if (std::strcmp(
            input->GetBuffer(),
            "SAVE1"
        ) != 0) {
        result = 2;
    } else if (input->textInput.cursor != 5) {
        result = 3;
    } else if (input->sliderBorder.inputActive != 1) {
        result = 4;
    }

    input->Destructor();
    ::operator delete(storage);
    return result;
}

extern "C" int hud_ui_container_constructor_smoke(void) {
    HudUiContainer container;

    return container.enabled == 0 && container.childHead == nullptr &&
                   container.childTail == nullptr
               ? 0
               : 1;
}

extern "C" int hud_ui_container_set_enabled_smoke(void) {
    HudUiContainer container;

    container.SetEnabled(1);
    const bool enabled = container.enabled == 1;

    container.SetEnabled(0);
    return enabled && container.enabled == 0 ? 0 : 1;
}

extern "C" int hud_ui_background_container_constructor_smoke(void) {
    HudUiBackgroundContainer container(3);

    return container.enabled == 0 && container.childHead == nullptr &&
                   container.childTail == nullptr &&
                   container.inputFocusElement == nullptr &&
                   container.captureTransitionMask == 3
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_insert_entry_sorted_prefix_smoke(void) {
    HudUiSaveLoadEntry entries[4];
    SetWriteTime(
        &entries[0],
        400
    );
    SetWriteTime(
        &entries[1],
        100
    );

    HudUiSaveLoadEntry middle;
    SetWriteTime(
        &middle,
        250
    );
    InsertEntryIntoSortedPrefix(
        &entries[2],
        middle
    );

    const bool insertedMiddle = entries[0].ftLastWriteTime.dwLowDateTime == 400 &&
                                entries[1].ftLastWriteTime.dwLowDateTime == 250 &&
                                entries[2].ftLastWriteTime.dwLowDateTime == 100;

    HudUiSaveLoadEntry oldest;
    SetWriteTime(
        &oldest,
        50
    );
    InsertEntryIntoSortedPrefix(
        &entries[3],
        oldest
    );

    return insertedMiddle && entries[0].ftLastWriteTime.dwLowDateTime == 400 &&
                   entries[1].ftLastWriteTime.dwLowDateTime == 250 &&
                   entries[2].ftLastWriteTime.dwLowDateTime == 100 &&
                   entries[3].ftLastWriteTime.dwLowDateTime == 50
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_partition_entries_by_pivot_smoke(void) {
    HudUiSaveLoadEntry entries[5];
    SetWriteTime(
        &entries[0],
        300
    );
    SetWriteTime(
        &entries[1],
        100
    );
    SetWriteTime(
        &entries[2],
        500
    );
    SetWriteTime(
        &entries[3],
        200
    );
    SetWriteTime(
        &entries[4],
        400
    );

    HudUiSaveLoadEntry pivot;
    SetWriteTime(
        &pivot,
        300
    );

    HudUiSaveLoadEntry *const split = PartitionEntriesByPivot(
        entries,
        entries + 5,
        pivot
    );
    if (split < entries || split > entries + 5) {
        return 1;
    }

    for (HudUiSaveLoadEntry *entry = entries; entry != split; ++entry) {
        if (pivot < *entry) {
            return 1;
        }
    }

    for (HudUiSaveLoadEntry *entry = split; entry != entries + 5; ++entry) {
        if (*entry < pivot) {
            return 1;
        }
    }

    return 0;
}

extern "C" int hud_ui_save_load_sort_entry_range_smoke(void) {
    HudUiSaveLoadEntry smallEntries[16];
    for (int i = 0; i < 16; ++i) {
        SetWriteTime(
            &smallEntries[i],
            (DWORD)(i + 1)
        );
    }

    SortEntryRange(
        smallEntries,
        smallEntries + 16,
        0
    );
    for (int i = 0; i < 16; ++i) {
        if (smallEntries[i].ftLastWriteTime.dwLowDateTime != (DWORD)(i + 1)) {
            return 1;
        }
    }

    HudUiSaveLoadEntry largeEntries[17];
    for (int i = 0; i < 17; ++i) {
        SetWriteTime(
            &largeEntries[i],
            (DWORD)((i + 1) * 100)
        );
    }

    SortEntryRange(
        largeEntries,
        largeEntries + 17,
        0
    );
    for (int i = 0; i < 17; ++i) {
        const DWORD expectedTime = (DWORD)((17 - i) * 100);
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
        if (CreateDirectoryA(
                "SavedGames",
                nullptr
            ) == 0) {
            return 1;
        }
        createdDirectory = true;
    } else if ((savedGamesAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return 1;
    }

    CleanupSaveLoadSmokeFiles(false);
    if (CreateDirectoryA(
            "SavedGames\\recoil_refresh_smoke_dir",
            nullptr
        ) == 0) {
        CleanupSaveLoadSmokeFiles(createdDirectory);
        return 1;
    }

    const bool filesWritten =
        WriteSaveLoadSmokeFile(
            "SavedGames\\recoil_refresh_smoke_old.sav",
            100
        ) &&
        WriteSaveLoadSmokeFile(
            "SavedGames\\recoil_refresh_smoke_new.sav",
            400
        ) &&
        WriteSaveLoadSmokeFile(
            "SavedGames\\recoil_refresh_smoke_middle.sav",
            250
        ) &&
        WriteSaveLoadSmokeFile(
            "SavedGames\\recoil_refresh_smoke_oldest.sav",
            50
        );
    if (!filesWritten) {
        CleanupSaveLoadSmokeFiles(createdDirectory);
        return 1;
    }

    static HudUiSaveLoadDialog dialog;
    static HudUiSaveLoadEntry storage[8];
    std::memset(
        &dialog,
        0,
        sizeof(dialog)
    );
    std::memset(
        storage,
        0,
        sizeof(storage)
    );
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
        if (CreateDirectoryA(
                "SavedGames",
                nullptr
            ) == 0) {
            return 1;
        }
        createdDirectory = true;
    } else if ((savedGamesAttrs & FILE_ATTRIBUTE_DIRECTORY) == 0) {
        return 1;
    }

    CleanupSaveLoadSmokeFiles(false);
    const bool filesWritten =
        WriteSaveLoadSmokeFile(
            "SavedGames\\recoil_refresh_smoke_old.sav",
            100
        ) &&
        WriteSaveLoadSmokeFile(
            "SavedGames\\recoil_refresh_smoke_new.sav",
            400
        ) &&
        WriteSaveLoadSmokeFile(
            "SavedGames\\recoil_refresh_smoke_middle.sav",
            250
        ) &&
        WriteSaveLoadSmokeFile(
            "SavedGames\\recoil_refresh_smoke_oldest.sav",
            50
        );
    if (!filesWritten) {
        CleanupSaveLoadSmokeFiles(createdDirectory);
        return 1;
    }

    static HudUiSaveLoadDialog dialog;
    static HudUiSaveLoadEntry storage[8];
    std::memset(
        &dialog,
        0,
        sizeof(dialog)
    );
    std::memset(
        storage,
        0,
        sizeof(storage)
    );

    for (int index = 0; index < 9; ++index) {
        new (&dialog.entryWidgets[index]) HudUiSaveLoadListItem;
    }
    dialog.fileEntries.begin = storage;
    dialog.fileEntries.end = storage;
    dialog.fileEntries.capacityEnd = storage + 8;

    dialog.InitializeFileEntries();

    int result = 0;
    const int expectedLayoutY[9] = {
        9830,
        16383,
        32767,
        32767,
        32767,
        29490,
        22936,
        16383,
        9830
    };
    const char *const expectedText[4] = {
        "recoil_refresh_smoke_new.sav",
        "recoil_refresh_smoke_middle.sav",
        "recoil_refresh_smoke_old.sav",
        "recoil_refresh_smoke_oldest.sav"
    };

    for (int index = 0; index < 9; ++index) {
        if (dialog.entryWidgets[index].layoutY != expectedLayoutY[index]) {
            result = 1;
        }
    }
    for (int index = 0; index < 4; ++index) {
        HudUiSaveLoadListItem *const item = &dialog.entryWidgets[index];
        if (item->layoutX != index ||
            (item->flags & 0x10u) != 0 ||
            std::strcmp(
                item->GetLastTextPtr(),
                expectedText[index]
            ) != 0) {
            result = 1;
        }
    }

    for (int index = 8; index >= 0; --index) {
        dialog.entryWidgets[index].HudUiPanel::~HudUiPanel();
    }
    CleanupSaveLoadSmokeFiles(createdDirectory);
    return result;
}

extern "C" int hud_ui_save_game_dialog_init_layout_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiSaveGameDialog));
    std::memset(
        storage,
        0,
        sizeof(HudUiSaveGameDialog)
    );
    HudUiSaveGameDialog *const dialog = static_cast<HudUiSaveGameDialog *>(storage);

    CodeFunctionPatch loadPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            SaveGameInitLoadProbeAddress(),
            loadPatch
        )) {
        ::operator delete(storage);
        return 1;
    }

    g_saveGameInitLoadCalls = 0;
    g_saveGameInitLoadArgsOk = false;
    HudUiSaveGameDialog *const result = new (dialog) HudUiSaveGameDialog;
    RestoreFunctionPatch(loadPatch);

    int checkResult = 0;
    if (result != dialog ||
        g_saveGameInitLoadCalls != 1 ||
        !g_saveGameInitLoadArgsOk ||
        dialog->gameNameInput.textInput.buffer == nullptr ||
        dialog->gameNameInput.textInput.capacity != 20 ||
        std::strcmp(
            dialog->gameNameInput.textInput.buffer,
            ""
        ) != 0 ||
        dialog->gameNameInput.sliderBorder.inputActive != 1 ||
        dialog->selectedEntryIndex != -1) {
        checkResult = 2;
    }

    const int expectedLayoutY[9] = {
        9830,
        16383,
        32767,
        32767,
        32767,
        29490,
        22936,
        16383,
        9830
    };
    for (int i = 0; i < 9; ++i) {
        if (dialog->entryWidgets[i].layoutY != expectedLayoutY[i] ||
            (dialog->entryWidgets[i].flags & 0x10u) == 0) {
            checkResult = 3;
        }
    }

    dialog->Destructor();
    ::operator delete(storage);
    return checkResult;
}

extern "C" int hud_ui_save_load_dialog_destructor_smoke(void) {
    void *const panelProbeStorage = ::operator new(sizeof(HudUiSaveLoadListItem));
    HudUiSaveLoadListItem *const panelProbe =
        new (panelProbeStorage) HudUiSaveLoadListItem;
    panelProbe->HudUiPanel::~HudUiPanel();
    void *const panelDestroyedVptr = *reinterpret_cast<void **>(panelProbe);
    ::operator delete(panelProbeStorage);

    void *const zrdProbeStorage = ::operator new(sizeof(HudUiZrdWidget));
    HudUiZrdWidget *const zrdProbe =
        static_cast<HudUiZrdWidget *>(zrdProbeStorage);
    std::memset(
        zrdProbe,
        0,
        sizeof(*zrdProbe)
    );
    zrdProbe->Constructor();
    zrdProbe->DestructorCore();
    void *const zrdDestroyedVptr = *reinterpret_cast<void **>(zrdProbe);
    ::operator delete(zrdProbeStorage);

    void *const gameNameProbeStorage = ::operator new(sizeof(HudUiSaveLoadGameNameInput));
    HudUiSaveLoadGameNameInput *const gameNameProbe =
        static_cast<HudUiSaveLoadGameNameInput *>(gameNameProbeStorage);
    std::memset(
        gameNameProbe,
        0,
        sizeof(*gameNameProbe)
    );
    gameNameProbe->BaseConstructor();
    gameNameProbe->textInput.AllocTextBuffer(8);
    gameNameProbe->Destructor();
    void *const gameNameDestroyedVptr = *reinterpret_cast<void **>(gameNameProbe);
    ::operator delete(gameNameProbeStorage);

    void *const dialogStorage = ::operator new(sizeof(HudUiSaveLoadDialog));
    HudUiSaveLoadDialog *const dialog =
        static_cast<HudUiSaveLoadDialog *>(dialogStorage);
    std::memset(
        dialog,
        0,
        sizeof(*dialog)
    );

    new (static_cast<HudUiBackground *>(dialog)) HudUiBackground;
    dialog->deleteButton.Constructor();
    dialog->backButton.Constructor();
    dialog->nextEntryButton.Constructor();
    dialog->prevEntryButton.Constructor();
    dialog->gameNameInput.BaseConstructor();
    dialog->gameNameInput.textInput.AllocTextBuffer(8);
    for (int index = 0; index < 9; ++index) {
        new (&dialog->entryWidgets[index]) HudUiSaveLoadListItem;
    }

    HudUiSaveLoadEntry *const entries =
        static_cast<HudUiSaveLoadEntry *>(::operator new(2 * sizeof(HudUiSaveLoadEntry)));
    dialog->fileEntries.begin = entries;
    dialog->fileEntries.end = entries + 1;
    dialog->fileEntries.capacityEnd = entries + 2;

    dialog->Destructor();

    bool entryPanelsDestroyed = true;
    for (int index = 0; index < 9; ++index) {
        entryPanelsDestroyed =
            entryPanelsDestroyed &&
            *reinterpret_cast<void **>(&dialog->entryWidgets[index]) == panelDestroyedVptr;
    }

    const bool vectorCleared =
        dialog->fileEntries.begin == nullptr && dialog->fileEntries.end == nullptr &&
        dialog->fileEntries.capacityEnd == nullptr;
    const bool gameNameDestroyed =
        *reinterpret_cast<void **>(&dialog->gameNameInput) == gameNameDestroyedVptr;
    const bool buttonsDestroyed =
        *reinterpret_cast<void **>(&dialog->prevEntryButton) == zrdDestroyedVptr &&
        *reinterpret_cast<void **>(&dialog->nextEntryButton) == zrdDestroyedVptr &&
        *reinterpret_cast<void **>(&dialog->backButton) == zrdDestroyedVptr &&
        *reinterpret_cast<void **>(&dialog->deleteButton) == zrdDestroyedVptr;

    ::operator delete(dialogStorage);

    return vectorCleared && entryPanelsDestroyed && gameNameDestroyed && buttonsDestroyed
               ? 0
               : 1;
}

extern "C" int hud_ui_save_game_dialog_destructor_smoke(void) {
    void *const zrdProbeStorage = ::operator new(sizeof(HudUiZrdWidget));
    HudUiZrdWidget *const zrdProbe =
        static_cast<HudUiZrdWidget *>(zrdProbeStorage);
    std::memset(
        zrdProbe,
        0,
        sizeof(*zrdProbe)
    );
    zrdProbe->Constructor();
    zrdProbe->DestructorCore();
    void *const zrdDestroyedVptr = *reinterpret_cast<void **>(zrdProbe);
    ::operator delete(zrdProbeStorage);

    void *const dialogStorage = ::operator new(sizeof(HudUiSaveGameDialog));
    HudUiSaveGameDialog *const dialog =
        static_cast<HudUiSaveGameDialog *>(dialogStorage);
    std::memset(
        dialog,
        0,
        sizeof(*dialog)
    );

    new (static_cast<HudUiBackground *>(dialog)) HudUiBackground;
    dialog->deleteButton.Constructor();
    dialog->backButton.Constructor();
    dialog->nextEntryButton.Constructor();
    dialog->prevEntryButton.Constructor();
    dialog->gameNameInput.BaseConstructor();
    dialog->gameNameInput.textInput.AllocTextBuffer(8);
    for (int index = 0; index < 9; ++index) {
        new (&dialog->entryWidgets[index]) HudUiSaveLoadListItem;
    }
    dialog->primaryActionButton.Constructor();

    HudUiSaveLoadEntry *const entries =
        static_cast<HudUiSaveLoadEntry *>(::operator new(2 * sizeof(HudUiSaveLoadEntry)));
    dialog->fileEntries.begin = entries;
    dialog->fileEntries.end = entries + 1;
    dialog->fileEntries.capacityEnd = entries + 2;

    dialog->Destructor();

    const bool vectorCleared =
        dialog->fileEntries.begin == nullptr && dialog->fileEntries.end == nullptr &&
        dialog->fileEntries.capacityEnd == nullptr;
    const bool primaryDestroyed =
        *reinterpret_cast<void **>(&dialog->primaryActionButton) == zrdDestroyedVptr;
    const bool buttonsDestroyed =
        *reinterpret_cast<void **>(&dialog->prevEntryButton) == zrdDestroyedVptr &&
        *reinterpret_cast<void **>(&dialog->nextEntryButton) == zrdDestroyedVptr &&
        *reinterpret_cast<void **>(&dialog->backButton) == zrdDestroyedVptr &&
        *reinterpret_cast<void **>(&dialog->deleteButton) == zrdDestroyedVptr;

    ::operator delete(dialogStorage);
    return vectorCleared && primaryDestroyed && buttonsDestroyed ? 0 : 1;
}

extern "C" int hud_ui_load_game_dialog_destructor_smoke(void) {
    void *const zrdProbeStorage = ::operator new(sizeof(HudUiZrdWidget));
    HudUiZrdWidget *const zrdProbe =
        static_cast<HudUiZrdWidget *>(zrdProbeStorage);
    std::memset(
        zrdProbe,
        0,
        sizeof(*zrdProbe)
    );
    zrdProbe->Constructor();
    zrdProbe->DestructorCore();
    void *const zrdDestroyedVptr = *reinterpret_cast<void **>(zrdProbe);
    ::operator delete(zrdProbeStorage);

    void *const dialogStorage = ::operator new(sizeof(HudUiLoadGameDialog));
    HudUiLoadGameDialog *const dialog =
        static_cast<HudUiLoadGameDialog *>(dialogStorage);
    std::memset(
        dialog,
        0,
        sizeof(*dialog)
    );

    new (static_cast<HudUiBackground *>(dialog)) HudUiBackground;
    dialog->deleteButton.Constructor();
    dialog->backButton.Constructor();
    dialog->nextEntryButton.Constructor();
    dialog->prevEntryButton.Constructor();
    dialog->gameNameInput.BaseConstructor();
    dialog->gameNameInput.textInput.AllocTextBuffer(8);
    for (int index = 0; index < 9; ++index) {
        new (&dialog->entryWidgets[index]) HudUiSaveLoadListItem;
    }
    dialog->primaryActionButton.Constructor();

    HudUiSaveLoadEntry *const entries =
        static_cast<HudUiSaveLoadEntry *>(::operator new(2 * sizeof(HudUiSaveLoadEntry)));
    dialog->fileEntries.begin = entries;
    dialog->fileEntries.end = entries + 1;
    dialog->fileEntries.capacityEnd = entries + 2;

    dialog->Destructor();

    const bool vectorCleared =
        dialog->fileEntries.begin == nullptr && dialog->fileEntries.end == nullptr &&
        dialog->fileEntries.capacityEnd == nullptr;
    const bool primaryDestroyed =
        *reinterpret_cast<void **>(&dialog->primaryActionButton) == zrdDestroyedVptr;
    const bool buttonsDestroyed =
        *reinterpret_cast<void **>(&dialog->prevEntryButton) == zrdDestroyedVptr &&
        *reinterpret_cast<void **>(&dialog->nextEntryButton) == zrdDestroyedVptr &&
        *reinterpret_cast<void **>(&dialog->backButton) == zrdDestroyedVptr &&
        *reinterpret_cast<void **>(&dialog->deleteButton) == zrdDestroyedVptr;

    ::operator delete(dialogStorage);
    return vectorCleared && primaryDestroyed && buttonsDestroyed ? 0 : 1;
}

extern "C" int hud_ui_save_load_set_selected_entry_index_smoke(void) {
    void *const dialogStorage = ::operator new(sizeof(HudUiSaveLoadDialog));
    std::memset(
        dialogStorage,
        0,
        sizeof(HudUiSaveLoadDialog)
    );
    HudUiSaveLoadDialog *const dialog = static_cast<HudUiSaveLoadDialog *>(dialogStorage);
    HudUiSaveLoadEntry entries[5];

    std::memset(
        entries,
        0,
        sizeof(entries)
    );

    dialog->gameNameInput.BaseConstructor();
    dialog->gameNameInput.AllocTextBuffer(32);
    for (int i = 0; i < 9; ++i) {
        new (&dialog->entryWidgets[i]) HudUiSaveLoadListItem;
    }

    for (int i = 0; i < 5; ++i) {
        std::sprintf(
            entries[i].cFileName,
            "entry%d.sav",
            i
        );
    }

    dialog->fileEntries.begin = entries;
    dialog->fileEntries.end = entries + 5;
    dialog->fileEntries.capacityEnd = entries + 5;

    dialog->SetSelectedEntryIndex(2);

    int result = 0;
    if (dialog->selectedEntryIndex != 2 ||
        std::strcmp(
            dialog->gameNameInput.GetBuffer(),
            "entry2.sav"
        ) != 0) {
        result = 1;
    }

    const int expectedLayout[9] = {-1, 0, 1, 3, 4, -1, -1, -1, -1};
    const int expectedHidden[9] = {1, 0, 0, 0, 0, 1, 1, 1, 1};
    const char *const expectedText[9] = {
        "",
        "entry0.sav",
        "entry1.sav",
        "entry3.sav",
        "entry4.sav",
        "",
        "",
        "",
        ""
    };

    for (int i = 0; i < 9; ++i) {
        HudUiSaveLoadListItem *const item = &dialog->entryWidgets[i];
        if (item->layoutX != expectedLayout[i]) {
            result = 1;
        }
        if (((item->flags & 0x10u) != 0 ? 1 : 0) != expectedHidden[i]) {
            result = 1;
        }
        if (std::strcmp(
                item->GetLastTextPtr(),
                expectedText[i]
            ) != 0) {
            result = 1;
        }
    }

    dialog->gameNameInput.Destructor();
    for (int i = 8; i >= 0; --i) {
        dialog->entryWidgets[i].HudUiPanel::~HudUiPanel();
    }
    ::operator delete(dialogStorage);

    return result;
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
