#include "Battlesport/hud.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <new>
#include <string.h>

extern "C" unsigned int g_HudUi_InvalidateMask;

namespace {
int __fastcall TestNewGamePanelVideoSurfaceStateNoOp(
    zVideo_SurfaceStatePartial *surfaceState
) {
    return surfaceState != 0 ? 1 : 0;
}

int g_cheatCodeBltDirectCalls;

void __fastcall TestCheatCodeBltSwToPrimaryRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    if (srcRect == 0 && dstRect == 0) {
        ++g_cheatCodeBltDirectCalls;
    }
}

int __fastcall TestCheatCodeVideoSurfaceStateNoOp(
    zVideo_SurfaceStatePartial *surfaceState
) {
    return surfaceState != 0 ? 1 : 0;
}

int g_cheatCodeLayoutActivatedCount;
int g_cheatCodeLoadFromZrdCount;
int g_cheatCodePostprocessCount;
int g_cheatCodeBlitOwnedCount;
int g_cheatCodeUnlockCount;
int g_cheatCodeSnapshotCreateCount;
int g_cheatCodeSnapshotStopCount;
int g_cheatCodeSampleSetInitCount;
int g_cheatCodeSampleSetDestroyCount;
const char *g_cheatCodeLoadPath;
const char *g_cheatCodeLoadSection;
int g_cheatCodeLoadCapture;
zSndSampleSet *g_cheatCodeDialogSet;

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

template <typename Method>
unsigned int TestCheatCodeMethodAddress(Method method) {
    union MethodBits {
        Method method;
        unsigned int address;
    } bits;

    bits.method = method;
    return bits.address;
}

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = static_cast<unsigned char *>(target);
    memcpy(patch.original, patch.address, sizeof(patch.original));

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
    *reinterpret_cast<int *>(patch.address + 1) =
        static_cast<int>(
            static_cast<unsigned char *>(replacement) -
            (patch.address + sizeof(patch.original))
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

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
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
        memcpy(patch.address, patch.original, sizeof(patch.original));
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

void __fastcall TestCheatCodeLayoutOnActivated(
    HudLayoutBase *
) {
    ++g_cheatCodeLayoutActivatedCount;
}

struct TestCheatCodeLayout : HudLayoutBase {
    void OnActivated() {
        ++g_cheatCodeLayoutActivatedCount;
    }
};

struct TestCheatCodeBackgroundPatchOps {
    zReader::Node *LoadFromZrd(
        const char *path,
        const char *section,
        int capturePrimary
    ) {
        ++g_cheatCodeLoadFromZrdCount;
        g_cheatCodeLoadPath = path;
        g_cheatCodeLoadSection = section;
        g_cheatCodeLoadCapture = capturePrimary;
        return 0;
    }

    void BlitOwnedSurfaceToPrimary() {
        ++g_cheatCodeBlitOwnedCount;
    }
};

int FakeCheatCodeRunPostprocessOnPrimaryBuffer() {
    ++g_cheatCodePostprocessCount;
    return 1;
}

int FakeCheatCodeDispatchUnlockPrimarySurfaceState() {
    ++g_cheatCodeUnlockCount;
    return 1;
}

zSndPlayHandleSnapshot *FakeCheatCodeCreateFromActiveSamples() {
    ++g_cheatCodeSnapshotCreateCount;
    zSndPlayHandleSnapshot *const snapshot =
        new zSndPlayHandleSnapshot(0);
    zSndPlayHandleSnapshotPayload payload = {};
    memcpy(
        &payload.volumeScaleRaw,
        g_zSnd_GlobalVolumeScalePtr,
        sizeof(payload.volumeScaleRaw)
    );
    snapshot->AppendPayload(payload);
    return snapshot;
}

struct TestCheatCodeSnapshotPatchOps {
    int StopAllIfPlaying() {
        ++g_cheatCodeSnapshotStopCount;
        return 1;
    }
};

extern "C" int __fastcall FakeCheatCodeSampleSetInitByName(
    const char *setName
) {
    ++g_cheatCodeSampleSetInitCount;
    if (strcmp(setName, "DIALOG") == 0 && g_cheatCodeDialogSet != 0) {
        g_cheatCodeDialogSet->resourcesLoaded = 1;
        return 1;
    }
    return 0;
}

extern "C" int __fastcall FakeCheatCodeSampleSetDestroyByName(
    const char *setName
) {
    ++g_cheatCodeSampleSetDestroyCount;
    if (strcmp(setName, "DIALOG") == 0 && g_cheatCodeDialogSet != 0) {
        g_cheatCodeDialogSet->resourcesLoaded = 0;
        return 1;
    }
    return 0;
}

void RestoreCheatCodePatches(
    CodeFunctionPatch *patches,
    int patchCount
) {
    while (patchCount > 0) {
        --patchCount;
        RestoreFunctionPatch(patches[patchCount]);
    }
}

bool InstallCheatCodeLoadPatch(
    CodeFunctionPatch *patches,
    int &patchCount
) {
    zReader::Node * (HudUiBackground::*loadMember)(
        const char *,
        const char *,
        int
    ) = &HudUiBackground::LoadFromZrd;
    zReader::Node * (TestCheatCodeBackgroundPatchOps::*fakeLoadMember)(
        const char *,
        const char *,
        int
    ) = &TestCheatCodeBackgroundPatchOps::LoadFromZrd;

    return PatchFunctionJump(
        reinterpret_cast<void *>(TestCheatCodeMethodAddress(loadMember)),
        reinterpret_cast<void *>(TestCheatCodeMethodAddress(fakeLoadMember)),
        patches[patchCount++]
    );
}

bool InstallCheatCodeDeactivatePatches(
    CodeFunctionPatch *patches,
    int &patchCount
) {
    void (HudUiDialogController::*blitMember)() =
        &HudUiDialogController::BlitOwnedSurfaceToPrimary;
    void (TestCheatCodeBackgroundPatchOps::*fakeBlitMember)() =
        &TestCheatCodeBackgroundPatchOps::BlitOwnedSurfaceToPrimary;

    return
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
            reinterpret_cast<void *>(&FakeCheatCodeRunPostprocessOnPrimaryBuffer),
            patches[patchCount++]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(TestCheatCodeMethodAddress(blitMember)),
            reinterpret_cast<void *>(TestCheatCodeMethodAddress(fakeBlitMember)),
            patches[patchCount++]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::Dispatch_UnlockPrimarySurfaceState),
            reinterpret_cast<void *>(&FakeCheatCodeDispatchUnlockPrimarySurfaceState),
            patches[patchCount++]
        );
}

bool InstallCheatCodeAudioPatches(
    CodeFunctionPatch *patches,
    int &patchCount
) {
    int (zSndPlayHandleSnapshot::*stopMember)() =
        &zSndPlayHandleSnapshot::StopAllIfPlaying;
    int (TestCheatCodeSnapshotPatchOps::*fakeStopMember)() =
        &TestCheatCodeSnapshotPatchOps::StopAllIfPlaying;

    return
        PatchFunctionJump(
            reinterpret_cast<void *>(&zSndPlayHandleSnapshot::CreateFromActiveSamples),
            reinterpret_cast<void *>(&FakeCheatCodeCreateFromActiveSamples),
            patches[patchCount++]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(TestCheatCodeMethodAddress(stopMember)),
            reinterpret_cast<void *>(TestCheatCodeMethodAddress(fakeStopMember)),
            patches[patchCount++]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zSndSampleSet_InitByName),
            reinterpret_cast<void *>(&FakeCheatCodeSampleSetInitByName),
            patches[patchCount++]
        ) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zSndSampleSet_DestroyByName),
            reinterpret_cast<void *>(&FakeCheatCodeSampleSetDestroyByName),
            patches[patchCount++]
        );
}
}

extern "C" int recoil_state_main_menu_transition_constructor_smoke(void) {
    char storage[sizeof(RecoilStateMainMenuTransition)];
    memset(storage, 0xcc, sizeof(storage));

    RecoilStateMainMenuTransition *const state =
        new (storage) RecoilStateMainMenuTransition;

    if (state->m_mainMenuDialog != 0) {
        return 1;
    }

    if (state->m_savedHalfResAdjustMode != 0) {
        return 2;
    }

    if (state->m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        return 3;
    }

    if (state->m_deferredVideoModeIndex != ZVID_MODE_INVALID_COMPLEMENT) {
        return 4;
    }

    if (state->m_pausedAudioSnapshot != 0) {
        return 5;
    }

    return 0;
}

extern "C" int recoil_state_main_menu_transition_static_init_smoke(void) {
    g_RecoilState_MainMenuTransition.m_mainMenuDialog =
        reinterpret_cast<HudUiMainMenuDialog *>(0x22222222);
    g_RecoilState_MainMenuTransition.m_savedHalfResAdjustMode = 0x33333333;
    g_RecoilState_MainMenuTransition.m_entryRoute =
        static_cast<RecoilMainMenuEntryRoute>(7);
    g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex =
        static_cast<zVidModeIndex>(5);
    g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot = 0x44444444;

    RecoilStateMainMenuTransition *const staticInitReturned =
        RecoilStateMainMenuTransition::StaticInit();
    if (staticInitReturned != &g_RecoilState_MainMenuTransition) {
        return 1;
    }

    if (g_RecoilState_MainMenuTransition.m_mainMenuDialog != 0 ||
        g_RecoilState_MainMenuTransition.m_savedHalfResAdjustMode != 0 ||
        g_RecoilState_MainMenuTransition.m_entryRoute !=
            RECOIL_MAINMENU_ROUTE_FRONTEND ||
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex !=
            ZVID_MODE_INVALID_COMPLEMENT ||
        g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot != 0) {
        return 2;
    }

    RecoilStateMainMenuTransition::AtExitDestructor();
    if (g_RecoilState_MainMenuTransition.m_mainMenuDialog != 0) {
        return 3;
    }

    g_RecoilState_MainMenuTransition.m_mainMenuDialog = 0;
    g_RecoilState_MainMenuTransition.m_savedHalfResAdjustMode = 0x55555555;
    g_RecoilState_MainMenuTransition.m_entryRoute =
        static_cast<RecoilMainMenuEntryRoute>(9);
    g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex =
        static_cast<zVidModeIndex>(6);
    g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot = 0x66666666;
    RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit();
    if (g_RecoilState_MainMenuTransition.m_mainMenuDialog != 0 ||
        g_RecoilState_MainMenuTransition.m_savedHalfResAdjustMode != 0 ||
        g_RecoilState_MainMenuTransition.m_entryRoute !=
            RECOIL_MAINMENU_ROUTE_FRONTEND ||
        g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex !=
            ZVID_MODE_INVALID_COMPLEMENT ||
        g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot != 0) {
        return 4;
    }

    return 0;
}

extern "C" int recoil_state_main_menu_transition_set_deferred_video_mode_index_smoke(void) {
    g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex = ZVID_MODE_INVALID_COMPLEMENT;

    RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
        static_cast<zVidModeIndex>(5)
    );

    return g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex ==
                   static_cast<zVidModeIndex>(5)
               ? 0
               : 1;
}

extern "C" int recoil_state_main_menu_transition_queue_enter_smoke(void) {
    const int oldCount = g_RecoilApp.m_stateQueue.m_itemCount;
    g_RecoilState_MainMenuTransition.m_entryRoute = RECOIL_MAINMENU_ROUTE_FRONTEND;

    RecoilStateMainMenuTransition::QueueEnter(
        static_cast<RecoilMainMenuEntryRoute>(7)
    );

    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    RecoilApp_StateQueueItem *const item =
        queue.m_writeBlock.m_cursor != 0 ? *(queue.m_writeBlock.m_cursor - 1) : 0;

    if (g_RecoilState_MainMenuTransition.m_entryRoute !=
        static_cast<RecoilMainMenuEntryRoute>(7)) {
        return 1;
    }

    if (queue.m_itemCount != oldCount + 1) {
        return 2;
    }

    if (item == 0 ||
        item->m_kind != RecoilApp_StateQueueKind_PushState ||
        item->m_stateObj != &g_RecoilState_MainMenuTransition ||
        item->m_param != 0) {
        return 3;
    }

    return 0;
}

extern "C" int hud_ui_main_menu_dialog_constructor_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const zVideo_BltRectDirectProc oldBltDirect =
        g_zVideo_pfnBltSwToPrimaryRectDirect;
    const zVideo_SurfaceStatePartial oldSwSurface = g_zVideo_SwSurfaceState;
    const zVideo_SurfaceStatePartial oldPrimarySurface =
        g_zVideo_PrimarySurfaceState;
    const zVideo_SurfaceStatePartial oldDisplaySurface =
        g_zVideo_DisplayModeSurfaceState;
    zVideo_SurfaceStateProc const oldLockSurfaceState =
        g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const oldUnlockSurfaceState =
        g_zVideo_pfnUnlockSurfaceState;

    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = 0;
    g_zVideo_pfnLockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_SwSurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_DisplayModeSurfaceState = zVideo_SurfaceStatePartial();

    int networkEnabled = 0;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState =
        g_GameStateOrMapTable;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable = 0;

    HudUiMainMenuDialog frontendDialog(RECOIL_MAINMENU_ROUTE_FRONTEND);
    const bool frontendConstructed =
        frontendDialog.enabled == 0 &&
        frontendDialog.captureTransitionMask == 1 &&
        frontendDialog.loadGameButton.modeOrEnabled == 1;

    zUtil_PlayerStateStorage playerState = zUtil_PlayerStateStorage();
    zInput_GameStateOrMapTablePartial gameState = zInput_GameStateOrMapTablePartial();
    gameState.playerState =
        reinterpret_cast<zInput_PlayerStatePartial *>(&playerState);
    g_GameStateOrMapTable = &gameState;

    playerState.lifecycleState = 4;
    HudUiMainMenuDialog resumeDialog(RECOIL_MAINMENU_ROUTE_INGAME);
    const bool resumeConstructed =
        resumeDialog.saveGameButton.modeOrEnabled == 1 &&
        resumeDialog.loadGameButton.modeOrEnabled == 1 &&
        resumeDialog.captureTransitionMask == 1;

    playerState.lifecycleState = 3;
    playerState.environmentAttachmentActive = 1;
    HudUiMainMenuDialog blockedDialog(RECOIL_MAINMENU_ROUTE_INGAME);
    const bool blockedConstructed =
        blockedDialog.saveGameButton.modeOrEnabled == 0 &&
        blockedDialog.loadGameButton.modeOrEnabled == 0;

    g_GameStateOrMapTable = oldGameState;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_SwSurfaceState = oldSwSurface;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_DisplayModeSurfaceState = oldDisplaySurface;
    if (!frontendConstructed) {
        return 1;
    }
    if (!resumeConstructed) {
        return 2;
    }
    if (!blockedConstructed) {
        return 3;
    }
    return 0;
}

extern "C" int hud_ui_new_game_panel_constructor_cluster_smoke(void) {
    zOptionEntryPartial *const oldPlayerNameOption = ZOPT_PLAYER_NAME;
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    zOptionEntryPartial *const oldOptionListHead = g_zGame_Options_OptionListHead;
    void *const oldRawCallback = g_zInput_KbdRawEventCallback;
    void *const oldRawCallbackCtx = g_zInput_KbdRawEventCallbackCtx;
    const int oldRendererType = g_zVideo_RendererType;
    const int oldHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;

    unsigned short pixels[4] = {};
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption = {};
    vmodeOption.payloadOrBuffer = 5;
    vmodeOption.name = vmodeName;
    vmodeOption.next = 0;
    char playerName[32] = "Ace";
    zOptionEntryPartial playerNameOption = {};
    playerNameOption.payloadOrBuffer = (int)(unsigned int)(playerName);
    playerNameOption.dataSize = sizeof(playerName);
    int difficulty = 2;
    g_zGame_Options_OptionListHead = &vmodeOption;
    ZOPT_PLAYER_NAME = &playerNameOption;
    g_zOpt_GameDifficultyOption = &difficulty;
    g_zInput_KbdRawEventCallback = 0;
    g_zInput_KbdRawEventCallbackCtx = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(unsigned short) * 2;

    unsigned char panelStorage[sizeof(HudUiNewGamePanel)];
    memset(panelStorage, 0, sizeof(panelStorage));
    HudUiNewGamePanel *const panel = (HudUiNewGamePanel *)(panelStorage);
    HudUiNewGamePanel *const returned = new (panel) HudUiNewGamePanel;
    const bool constructed =
        returned == panel &&
        panel->inputFocusElement == 0 &&
        panel->backWidget.owner == 0 &&
        panel->startWidget.owner == 0 &&
        panel->nameInput.owner == 0 &&
        panel->nameInput.textInput.owner == &panel->nameInput &&
        panel->nameInput.sliderBorder.inputActive == 1 &&
        panel->nameInput.sliderBorder.sliderVisibleWhenInputActive == 0 &&
        panel->nameInput.sliderBorder.rawKeyFilterEnabled == 0 &&
        panel->intensity.optionCount == 0 &&
        panel->intensity.options[0] == 0 &&
        panel->loadedRoot == 0 && panel->cfgRoot == 0 &&
        panel->uiOriginX == 0 && panel->uiOriginY == 0 &&
        panel->nameInput.textInput.buffer != 0 &&
        panel->nameInput.textInput.capacity == 0x100 &&
        strcmp(panel->nameInput.textInput.buffer, "Ace") == 0 &&
        panel->nameInput.textInput.cursor == 3;

    difficulty = 4;
    panel->SyncIntensityFromDifficulty();
    const bool synced = panel->intensity.selectedIndex == 4;

    strcpy(playerName, "Ranger");
    panel->nameInput.HudUiNewGamePanel_NameInput::OnActivate();
    const bool nameActivated =
        panel->nameInput.textInput.capacity == 21 &&
        strcmp(panel->nameInput.textInput.buffer, "Ranger") == 0 &&
        panel->nameInput.textInput.cursor == 6 &&
        panel->nameInput.sliderBorder.inputActive == 1 &&
        panel->nameInput.sliderBorder.sliderVisibleWhenInputActive == 1 &&
        g_zInput_KbdRawEventCallback ==
            (void *)(&HudUiNumericTextInput::RawKeyboardCallback) &&
        g_zInput_KbdRawEventCallbackCtx == &panel->nameInput;
    int nameActivationFailure = 0;
    if (!nameActivated) {
        if (panel->nameInput.textInput.capacity != 21) {
            if (panel->nameInput.textInput.capacity == 0x100) {
                nameActivationFailure = 127;
            } else if (panel->nameInput.textInput.capacity == 22) {
                nameActivationFailure = 128;
            } else {
                nameActivationFailure = 120;
            }
        } else if (strcmp(panel->nameInput.textInput.buffer, "Ranger") != 0) {
            nameActivationFailure = 121;
        } else if (panel->nameInput.textInput.cursor != 6) {
            nameActivationFailure = 122;
        } else if (panel->nameInput.sliderBorder.inputActive != 1) {
            nameActivationFailure = 123;
        } else if (panel->nameInput.sliderBorder.sliderVisibleWhenInputActive != 1) {
            nameActivationFailure = 124;
        } else if (g_zInput_KbdRawEventCallback !=
                   (void *)(&HudUiNumericTextInput::RawKeyboardCallback)) {
            nameActivationFailure = 125;
        } else if (g_zInput_KbdRawEventCallbackCtx != &panel->nameInput) {
            nameActivationFailure = 126;
        } else {
            nameActivationFailure = 12;
        }
    }
    panel->nameInput.SetRawKeyboardCapture(0);

    HudUiBackground *const noDeleteResult = panel->ScalarDeletingDestructor(0);
    const bool noDeleteScalar = noDeleteResult == panel;

    HudUiNewGamePanel *const heapPanel =
        (HudUiNewGamePanel *)(::operator new(sizeof(HudUiNewGamePanel)));
    new (heapPanel) HudUiNewGamePanel;
    HudUiBackground *const heapScalarResult = heapPanel->ScalarDeletingDestructor(1);
    const bool heapScalar = heapScalarResult == heapPanel;

    HudUiZrdWidget *const zrdWidget =
        (HudUiZrdWidget *)(::operator new(sizeof(HudUiZrdWidget)));
    zrdWidget->Constructor();
    HudUiZrdWidget *const zrdThunkResult = zrdWidget->ScalarDeletingDestructorThunk(1);
    const bool zrdThunk = zrdThunkResult == zrdWidget;

    HudUiZrdWidgetEx17C *const selector =
        (HudUiZrdWidgetEx17C *)(::operator new(sizeof(HudUiZrdWidgetEx17C)));
    selector->Constructor();
    HudUiZrdWidgetEx17C *const selectorThunkResult =
        selector->ScalarDeletingDestructorThunk(1);
    const bool selectorThunk = selectorThunkResult == selector;

    ZOPT_PLAYER_NAME = oldPlayerNameOption;
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    g_zGame_Options_OptionListHead = oldOptionListHead;
    g_zInput_KbdRawEventCallback = oldRawCallback;
    g_zInput_KbdRawEventCallbackCtx = oldRawCallbackCtx;
    g_zVideo_RendererType = oldRendererType;
    g_zVideo_UseHalfResBackbuffer = oldHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;

    if (!constructed) {
        return 10;
    }
    if (!synced) {
        return 11;
    }
    if (nameActivationFailure != 0) {
        return nameActivationFailure;
    }
    if (!noDeleteScalar) {
        return 13;
    }
    if (!heapScalar) {
        return 14;
    }
    if (!zrdThunk) {
        return 15;
    }
    if (!selectorThunk) {
        return 16;
    }

    return 0;
}

extern "C" int hud_ui_new_game_panel_overlay_owner_queue_enter_smoke(void) {
    const int oldCount = g_RecoilApp.m_stateQueue.m_itemCount;
    HudUiNewGamePanelOverlayOwner::QueueEnter();
    return g_RecoilApp.m_stateQueue.m_itemCount == oldCount + 1 ? 0 : 1;
}

extern "C" int hud_ui_new_game_panel_overlay_owner_on_try_become_current_smoke(void) {
    zOptionEntryPartial *const oldPlayerNameOption = ZOPT_PLAYER_NAME;
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    zOptionEntryPartial *const oldOptionListHead = g_zGame_Options_OptionListHead;
    void *const oldRawCallback = g_zInput_KbdRawEventCallback;
    void *const oldRawCallbackCtx = g_zInput_KbdRawEventCallbackCtx;
    const int oldRendererType = g_zVideo_RendererType;
    const int oldHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;

    unsigned short pixels[4] = {};
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption = {};
    vmodeOption.payloadOrBuffer = 5;
    vmodeOption.name = vmodeName;
    vmodeOption.next = 0;
    char playerName[32] = "Ace";
    zOptionEntryPartial playerNameOption = {};
    playerNameOption.payloadOrBuffer = (int)(unsigned int)(playerName);
    playerNameOption.dataSize = sizeof(playerName);
    int difficulty = 3;
    g_zGame_Options_OptionListHead = &vmodeOption;
    ZOPT_PLAYER_NAME = &playerNameOption;
    g_zOpt_GameDifficultyOption = &difficulty;
    g_zInput_KbdRawEventCallback = 0;
    g_zInput_KbdRawEventCallbackCtx = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(unsigned short) * 2;

    HudUiNewGamePanelOverlayOwner state;
    const int accepted = state.OnTryBecomeCurrent();
    HudUiNewGamePanel *const panel = (HudUiNewGamePanel *)state.m_dialog;
    const bool ok =
        accepted == 1 &&
        panel != 0 &&
        panel->enabled == 1 &&
        panel->intensity.selectedIndex == 3 &&
        panel->nameInput.textInput.buffer != 0 &&
        strcmp(panel->nameInput.textInput.buffer, "Ace") == 0;

    if (panel != 0) {
        panel->ScalarDeletingDestructor(1);
        state.m_dialog = 0;
    }

    ZOPT_PLAYER_NAME = oldPlayerNameOption;
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    g_zGame_Options_OptionListHead = oldOptionListHead;
    g_zInput_KbdRawEventCallback = oldRawCallback;
    g_zInput_KbdRawEventCallbackCtx = oldRawCallbackCtx;
    g_zVideo_RendererType = oldRendererType;
    g_zVideo_UseHalfResBackbuffer = oldHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;

    return ok ? 0 : 1;
}

extern "C" int hud_ui_new_game_panel_overlay_owner_lifecycle_smoke(void) {
    zOptionEntryPartial *const oldPlayerNameOption = ZOPT_PLAYER_NAME;
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    zOptionEntryPartial *const oldOptionListHead = g_zGame_Options_OptionListHead;
    void *const oldRawCallback = g_zInput_KbdRawEventCallback;
    void *const oldRawCallbackCtx = g_zInput_KbdRawEventCallbackCtx;
    const int oldRendererType = g_zVideo_RendererType;
    const int oldHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;

    unsigned short pixels[4] = {};
    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption = {};
    vmodeOption.payloadOrBuffer = 5;
    vmodeOption.name = vmodeName;
    vmodeOption.next = 0;
    char playerName[32] = "Ace";
    zOptionEntryPartial playerNameOption = {};
    playerNameOption.payloadOrBuffer = (int)(unsigned int)(playerName);
    playerNameOption.dataSize = sizeof(playerName);
    int difficulty = 1;
    g_zGame_Options_OptionListHead = &vmodeOption;
    ZOPT_PLAYER_NAME = &playerNameOption;
    g_zOpt_GameDifficultyOption = &difficulty;
    g_zInput_KbdRawEventCallback = 0;
    g_zInput_KbdRawEventCallbackCtx = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnLockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestNewGamePanelVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(unsigned short) * 2;

    g_HudUiNewGamePanelOverlayOwner.m_dialog =
        (HudUiContainer *)0x22222222;
    HudUiNewGamePanelOverlayOwner *const staticInitReturned =
        HudUiNewGamePanelOverlayOwner::StaticInit();
    const bool staticInitOk =
        staticInitReturned == &g_HudUiNewGamePanelOverlayOwner &&
        g_HudUiNewGamePanelOverlayOwner.m_dialog == 0;

    HudUiNewGamePanel *const atExitPanel =
        (HudUiNewGamePanel *)(::operator new(sizeof(HudUiNewGamePanel)));
    new (atExitPanel) HudUiNewGamePanel;
    atExitPanel->SetEnabled(1);
    g_HudUiNewGamePanelOverlayOwner.m_dialog = atExitPanel;
    HudUiNewGamePanelOverlayOwner::AtExitDestructor();
    const bool atExitOk = g_HudUiNewGamePanelOverlayOwner.m_dialog == 0;

    HudUiNewGamePanel *const destructorPanel =
        (HudUiNewGamePanel *)(::operator new(sizeof(HudUiNewGamePanel)));
    new (destructorPanel) HudUiNewGamePanel;
    destructorPanel->SetEnabled(1);
    HudUiNewGamePanelOverlayOwner state;
    state.m_dialog = destructorPanel;
    state.~HudUiNewGamePanelOverlayOwner();
    const bool destructorOk = state.m_dialog == 0;

    HudUiNewGamePanelOverlayOwner::RegisterAtExit();

    g_HudUiNewGamePanelOverlayOwner.m_dialog =
        (HudUiContainer *)0x77777777;
    HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit();
    const bool staticInitRegisterOk = g_HudUiNewGamePanelOverlayOwner.m_dialog == 0;

    ZOPT_PLAYER_NAME = oldPlayerNameOption;
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    g_zGame_Options_OptionListHead = oldOptionListHead;
    g_zInput_KbdRawEventCallback = oldRawCallback;
    g_zInput_KbdRawEventCallbackCtx = oldRawCallbackCtx;
    g_zVideo_RendererType = oldRendererType;
    g_zVideo_UseHalfResBackbuffer = oldHalfResBackbuffer;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;

    return staticInitOk && atExitOk && destructorOk && staticInitRegisterOk ? 0 : 1;
}

extern "C" int recoil_state_cheat_code_constructor_smoke(void) {
    char storage[sizeof(RecoilStateCheatCode)];
    memset(storage, 0xcc, sizeof(storage));

    RecoilStateCheatCode *const rawState =
        reinterpret_cast<RecoilStateCheatCode *>(storage);
    rawState->m_prevHalfResAdjustMode = ZVIDEO_HALFRES_ADJUST_ENABLED;
    rawState->m_audioSnapshot = 0x33333333;

    RecoilStateCheatCode *const state = new (storage) RecoilStateCheatCode;

    if (state->m_dialog != 0) {
        return 1;
    }

    if (state->m_prevHalfResAdjustMode != ZVIDEO_HALFRES_ADJUST_ENABLED) {
        return 2;
    }

    if (state->m_audioSnapshot != 0x33333333) {
        return 3;
    }

    return 0;
}

extern "C" int recoil_state_cheat_code_on_try_become_current_smoke(void) {
    CodeFunctionPatch patches[5] = {};
    int patchCount = 0;
    if (!InstallCheatCodeLoadPatch(patches, patchCount) ||
        !InstallCheatCodeAudioPatches(patches, patchCount)) {
        RestoreCheatCodePatches(patches, patchCount);
        return 10;
    }

    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption = {};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    zOptionEntryPartial *const oldOptionsHead = g_zGame_Options_OptionListHead;

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldRendererType = g_zVideo_RendererType;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const int oldHalfResMode = g_zVideo_HalfResAdjustMode;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    const zSndSampleSetRegistry oldSampleSetRegistry = g_zSnd_SampleSetRegistry;
    void *const oldGlobalVolumeScale = g_zSnd_GlobalVolumeScalePtr;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldSndPreInitialized = g_zSnd_PreInitialized;

    float globalVolumeScale = 1.0f;
    char dialogSetName[] = "DIALOG";
    zSndSampleSet dialogSet = {};
    zSndSampleSet *sampleSetSlots[1] = {&dialogSet};
    unsigned short pixels[4] = {};
    dialogSet.setName = dialogSetName;

    g_cheatCodeLoadFromZrdCount = 0;
    g_cheatCodeLoadPath = 0;
    g_cheatCodeLoadSection = 0;
    g_cheatCodeLoadCapture = -1;
    g_cheatCodeSnapshotCreateCount = 0;
    g_cheatCodeSnapshotStopCount = 0;
    g_cheatCodeSampleSetInitCount = 0;
    g_cheatCodeSampleSetDestroyCount = 0;
    g_cheatCodeDialogSet = &dialogSet;
    g_zGame_Options_OptionListHead = &vmodeOption;
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_RendererType = 1;
    g_zVideo_pfnBltSwToPrimaryRectDirect = TestCheatCodeBltSwToPrimaryRectDirect;
    g_zVideo_pfnLockSurfaceState = TestCheatCodeVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestCheatCodeVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(unsigned short) * 2;
    g_zVideo_HalfResAdjustMode = ZVIDEO_HALFRES_ADJUST_ENABLED;
    g_HudUi_InvalidateMask = 0x80;
    g_zSnd_GlobalVolumeScalePtr = &globalVolumeScale;
    g_zSnd_SampleSetRegistry.begin = sampleSetSlots;
    g_zSnd_SampleSetRegistry.end = sampleSetSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSetSlots + 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_cheatCodeBltDirectCalls = 0;

    RecoilStateCheatCode state;
    const int accepted = state.OnTryBecomeCurrent();
    HudUiCheatCodeDialog *const dialog = (HudUiCheatCodeDialog *)state.m_dialog;
    zSndPlayHandleSnapshot *const snapshot =
        reinterpret_cast<zSndPlayHandleSnapshot *>(static_cast<unsigned int>(state.m_audioSnapshot));

    int result = 0;
    if (accepted != 1 || dialog == 0 || snapshot == 0) {
        result = 1;
    } else if (g_cheatCodeBltDirectCalls != 1 ||
               state.m_prevHalfResAdjustMode != ZVIDEO_HALFRES_ADJUST_ENABLED ||
               g_zVideo_HalfResAdjustMode != ZVIDEO_HALFRES_ADJUST_DISABLED ||
               g_HudUi_InvalidateMask != 0x04u || dialog->enabled != 1 ||
               dialogSet.resourcesLoaded != 1 ||
               g_cheatCodeLoadFromZrdCount != 1 ||
               strcmp(g_cheatCodeLoadPath, "dialog.zrd") != 0 ||
               strcmp(g_cheatCodeLoadSection, "CHEAT_CODE_DIALOG") != 0 ||
               g_cheatCodeLoadCapture != 0 ||
               g_cheatCodeSnapshotCreateCount != 1 ||
               g_cheatCodeSnapshotStopCount != 1 ||
               g_cheatCodeSampleSetInitCount != 1 ||
               g_cheatCodeSampleSetDestroyCount != 0) {
        result = 2;
    }

    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
        state.m_dialog = 0;
    }
    if (snapshot != 0) {
        snapshot->Destroy();
        state.m_audioSnapshot = 0;
    }

    g_zGame_Options_OptionListHead = oldOptionsHead;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_RendererType = oldRendererType;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_HalfResAdjustMode = oldHalfResMode;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_zSnd_SampleSetRegistry = oldSampleSetRegistry;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScale;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_zSnd_PreInitialized = oldSndPreInitialized;
    g_cheatCodeDialogSet = 0;
    RestoreCheatCodePatches(patches, patchCount);
    return result;
}

extern "C" int recoil_state_cheat_code_on_deactivate_smoke(void) {
    CodeFunctionPatch patches[8] = {};
    int patchCount = 0;
    if (!InstallCheatCodeLoadPatch(patches, patchCount) ||
        !InstallCheatCodeAudioPatches(patches, patchCount) ||
        !InstallCheatCodeDeactivatePatches(patches, patchCount)) {
        RestoreCheatCodePatches(patches, patchCount);
        return 10;
    }

    char vmodeName[] = "VMode";
    zOptionEntryPartial vmodeOption = {};
    vmodeOption.payloadOrBuffer = 6;
    vmodeOption.name = vmodeName;
    zOptionEntryPartial *const oldOptionsHead = g_zGame_Options_OptionListHead;

    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldRendererType = g_zVideo_RendererType;
    const int oldUseHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    const zVideo_BltRectDirectProc oldBltDirect = g_zVideo_pfnBltSwToPrimaryRectDirect;
    const int oldHalfResMode = g_zVideo_HalfResAdjustMode;
    const zVideo_SurfaceStatePartial oldPrimarySurface = g_zVideo_PrimarySurfaceState;
    zVideo_SurfaceStateProc const oldLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const oldUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    const zSndSampleSetRegistry oldSampleSetRegistry = g_zSnd_SampleSetRegistry;
    void *const oldGlobalVolumeScale = g_zSnd_GlobalVolumeScalePtr;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldSndPreInitialized = g_zSnd_PreInitialized;
    HudLayoutBase *const oldLayout = g_HudUiMgrCurrentLayout;

    float globalVolumeScale = 1.0f;
    char dialogSetName[] = "DIALOG";
    zSndSampleSet dialogSet = {};
    zSndSampleSet *sampleSetSlots[1] = {&dialogSet};
    unsigned short pixels[4] = {};
    TestCheatCodeLayout layout;
    dialogSet.setName = dialogSetName;

    g_cheatCodeLoadFromZrdCount = 0;
    g_cheatCodeLoadPath = 0;
    g_cheatCodeLoadSection = 0;
    g_cheatCodeLoadCapture = -1;
    g_cheatCodePostprocessCount = 0;
    g_cheatCodeBlitOwnedCount = 0;
    g_cheatCodeUnlockCount = 0;
    g_cheatCodeSnapshotCreateCount = 0;
    g_cheatCodeSnapshotStopCount = 0;
    g_cheatCodeSampleSetInitCount = 0;
    g_cheatCodeSampleSetDestroyCount = 0;
    g_cheatCodeDialogSet = &dialogSet;
    g_zGame_Options_OptionListHead = &vmodeOption;
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_RendererType = 1;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_pfnBltSwToPrimaryRectDirect = TestCheatCodeBltSwToPrimaryRectDirect;
    g_zVideo_pfnLockSurfaceState = TestCheatCodeVideoSurfaceStateNoOp;
    g_zVideo_pfnUnlockSurfaceState = TestCheatCodeVideoSurfaceStateNoOp;
    g_zVideo_PrimarySurfaceState = {};
    g_zVideo_PrimarySurfaceState.pixels = pixels;
    g_zVideo_PrimarySurfaceState.width = 2;
    g_zVideo_PrimarySurfaceState.height = 2;
    g_zVideo_PrimarySurfaceState.pitch = sizeof(unsigned short) * 2;
    g_zVideo_HalfResAdjustMode = ZVIDEO_HALFRES_ADJUST_ENABLED;
    g_HudUi_InvalidateMask = 0x80;
    g_zSnd_GlobalVolumeScalePtr = &globalVolumeScale;
    g_zSnd_SampleSetRegistry.begin = sampleSetSlots;
    g_zSnd_SampleSetRegistry.end = sampleSetSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSetSlots + 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_HudUiMgrCurrentLayout = &layout;
    g_cheatCodeBltDirectCalls = 0;
    g_cheatCodeLayoutActivatedCount = 0;

    RecoilStateCheatCode state;
    int result = state.OnTryBecomeCurrent() == 1 ? 0 : 1;
    zSndPlayHandleSnapshot *const snapshot =
        reinterpret_cast<zSndPlayHandleSnapshot *>(static_cast<unsigned int>(state.m_audioSnapshot));

    if (result == 0) {
        state.OnDeactivate();
        if (state.m_dialog != 0) {
            result = 2;
        } else if (g_zVideo_HalfResAdjustMode != ZVIDEO_HALFRES_ADJUST_ENABLED) {
            result = 3;
        } else if (g_HudUi_InvalidateMask != 0x0cu) {
            result = 4;
        } else if (dialogSet.resourcesLoaded != 0) {
            result = 5;
        } else if (g_cheatCodeLayoutActivatedCount != 1) {
            result = 6;
        } else if (g_cheatCodePostprocessCount != 1 ||
                   g_cheatCodeBlitOwnedCount != 1 ||
                   g_cheatCodeUnlockCount != 1) {
            result = 7;
        } else if (g_cheatCodeLoadFromZrdCount != 1 ||
                   strcmp(g_cheatCodeLoadPath, "dialog.zrd") != 0 ||
                   strcmp(g_cheatCodeLoadSection, "CHEAT_CODE_DIALOG") != 0 ||
                   g_cheatCodeLoadCapture != 0) {
            result = 8;
        } else if (g_cheatCodeSnapshotCreateCount != 1 ||
                   g_cheatCodeSnapshotStopCount != 1 ||
                   g_cheatCodeSampleSetInitCount != 1 ||
                   g_cheatCodeSampleSetDestroyCount != 1) {
            result = 9;
        }
    }

    if (snapshot != 0) {
        snapshot->Destroy();
        state.m_audioSnapshot = 0;
    }

    g_zGame_Options_OptionListHead = oldOptionsHead;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_RendererType = oldRendererType;
    g_zVideo_UseHalfResBackbuffer = oldUseHalfResBackbuffer;
    g_zVideo_pfnBltSwToPrimaryRectDirect = oldBltDirect;
    g_zVideo_pfnLockSurfaceState = oldLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = oldUnlockSurfaceState;
    g_zVideo_PrimarySurfaceState = oldPrimarySurface;
    g_zVideo_HalfResAdjustMode = oldHalfResMode;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_zSnd_SampleSetRegistry = oldSampleSetRegistry;
    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScale;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_zSnd_PreInitialized = oldSndPreInitialized;
    g_HudUiMgrCurrentLayout = oldLayout;
    g_cheatCodeDialogSet = 0;
    RestoreCheatCodePatches(patches, patchCount);
    return result;
}

extern "C" int recoil_state_controls_lifecycle_smoke(void) {
    char storage[sizeof(RecoilStateControls)];
    memset(storage, 0xcc, sizeof(storage));

    RecoilStateControls *const state = new (storage) RecoilStateControls;
    if (state->m_dialog != 0) {
        return 1;
    }

    state->~RecoilStateControls();
    return state->m_dialog == 0 ? 0 : 2;
}

extern "C" int recoil_state_controls_activation_smoke(void) {
    RecoilStateControls state;
    return state.m_dialog == 0 ? 0 : 1;
}

extern "C" int recoil_state_controls_on_resume_smoke(void) {
    RecoilStateControls state;
    state.OnResume(1);
    return state.m_dialog == 0 ? 0 : 1;
}

extern "C" int recoil_state_controls_queue_enter_smoke(void) {
    const int oldCount = g_RecoilApp.m_stateQueue.m_itemCount;
    RecoilStateControls::QueueEnter();
    return g_RecoilApp.m_stateQueue.m_itemCount == oldCount + 1 ? 0 : 1;
}

extern "C" int recoil_state_confirm_quit_queue_enter_smoke(void) {
    const int oldCount = g_RecoilApp.m_stateQueue.m_itemCount;
    RecoilStateConfirmQuit::QueueEnter();
    return g_RecoilApp.m_stateQueue.m_itemCount == oldCount + 1 ? 0 : 1;
}

extern "C" int recoil_state_confirm_quit_destructor_smoke(void) {
    char storage[sizeof(RecoilStateConfirmQuit)];
    memset(storage, 0xcc, sizeof(storage));

    RecoilStateConfirmQuit *const state = new (storage) RecoilStateConfirmQuit;
    if (state->m_dialog != 0) {
        return 1;
    }

    state->~RecoilStateConfirmQuit();
    return state->m_dialog == 0 ? 0 : 2;
}
