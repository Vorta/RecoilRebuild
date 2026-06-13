#include "Battlesport/Briefing.h"
#include "Battlesport/HudSensorTracker.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdint>
#include <cstring>
#include <new>

extern "C" unsigned int g_HudUi_InvalidateMask;

namespace {
unsigned short g_constructorSurfacePixel;
int g_locatorBlitCount;
zVidImagePartial *g_locatorBlitImage;
int g_locatorBlitX;
int g_locatorBlitY;
int g_locatorBlitFlags;
int g_locatorBlitHasRect;
zVidRect32 g_locatorBlitRect;

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

template <typename Method> void *MethodAddress(
    Method method
) {
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
    if (target == 0 || replacement == 0) {
        patch.address = 0;
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
        patch.address = 0;
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

    patch.address = 0;
}

int TestRunPostprocessOnPrimaryBuffer() {
    return 0;
}

struct TestCompositePanelConstructor {
    HudUiCompositePanel *ConstructorWithEntryCount(int) {
        HudUiCompositePanel *const panel =
            reinterpret_cast<HudUiCompositePanel *>(this);
        panel->HudUiPanel::ConstructorDefault(
            0,
            0,
            0
        );
        panel->activeEntryCount = 0;
        panel->entryVector.allocatorProxy.value = 0;
        panel->entryVector.begin = 0;
        panel->entryVector.end = 0;
        panel->entryVector.capacityEnd = 0;
        panel->SetTextFmt("");
        panel->SetVisible(1);
        return panel;
    }
};

int __fastcall TestVideoSurfaceDispatch(
    zVideo_SurfaceStatePartial *
) {
    return 0;
}

int __fastcall TestAdjustSurfaces(
    zVidRect32 *,
    zVidRect32 *,
    int,
    int
) {
    return 0;
}

void __fastcall TestLocatorBltSourceToPrimary(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
) {
    ++g_locatorBlitCount;
    g_locatorBlitImage = image;
    g_locatorBlitX = dstX;
    g_locatorBlitY = dstY;
    g_locatorBlitFlags = clipFlags;
    g_locatorBlitHasRect = srcRect != 0 ? 1 : 0;
    if (srcRect != 0) {
        g_locatorBlitRect = *srcRect;
    }
}

struct ConstructorGlobalState {
    zVideo_SurfaceStatePartial swSurface;
    zVideo_SurfaceStatePartial primarySurface;
    zVideo_SurfaceStatePartial displaySurface;
    zVideo_SurfaceStateProc lockSurface;
    zVideo_SurfaceStateProc unlockSurface;
    zVideo_AdjustSurfacesProc adjustSurfaces;
    int adjustDisableGate;
    int rendererType;
    int useHalfResBackbuffer;
    int halfResAdjustMode;
    int frameTick;
    int sndActiveBackend;
    zSndSample *lastVoice;
    zSndPlayHandle *lastVoiceHandle;
    int lastVoiceMarkerIndex;
    int lastVoiceStopMarkerIndex;
    int fadeActiveCount;
    float frameDeltaTimeSec;
    float timeCurrentSec;
    float timeNewSec;
    float timeAccumulatedSec;
    float timeUnscaledDeltaSec;
    float timeUnscaledAccumulatedSec;
    int progressEventCode;
};

void PrepareConstructorGlobals(
    ConstructorGlobalState &state
) {
    state.swSurface = g_zVideo_SwSurfaceState;
    state.primarySurface = g_zVideo_PrimarySurfaceState;
    state.displaySurface = g_zVideo_DisplayModeSurfaceState;
    state.lockSurface = g_zVideo_pfnLockSurfaceState;
    state.unlockSurface = g_zVideo_pfnUnlockSurfaceState;
    state.adjustSurfaces = g_zVideo_pfnAdjustSurfaces;
    state.adjustDisableGate = g_zVideo_AdjustSurfacesDisableGate;
    state.rendererType = g_zVideo_RendererType;
    state.useHalfResBackbuffer = g_zVideo_UseHalfResBackbuffer;
    state.halfResAdjustMode = g_zVideo_HalfResAdjustMode;
    state.frameTick = g_zVideo_FrameTick;
    state.sndActiveBackend = g_zSnd_ActiveBackend;
    state.lastVoice = g_zSndLastVoice;
    state.lastVoiceHandle = g_zSndLastVoiceHandle;
    state.lastVoiceMarkerIndex = g_zSndLastVoiceMarkerIndex;
    state.lastVoiceStopMarkerIndex = g_zSndLastVoiceStopMarkerIndex;
    state.fadeActiveCount = g_zSndFadeActiveListCount;
    state.frameDeltaTimeSec = g_FrameDeltaTimeSec;
    state.timeCurrentSec = g_Time_CurrentTimeSec;
    state.timeNewSec = g_Time_NewTimeSec;
    state.timeAccumulatedSec = g_Time_AccumulatedTimeSec;
    state.timeUnscaledDeltaSec = g_Time_UnscaledDeltaTimeSec;
    state.timeUnscaledAccumulatedSec = g_Time_UnscaledAccumulatedTimeSec;
    state.progressEventCode = g_Briefing_ProgressEventCode;

    zVideo_SurfaceStatePartial testSurface = {};
    testSurface.width = 1;
    testSurface.height = 1;
    testSurface.pitch = sizeof(g_constructorSurfacePixel);
    testSurface.pixels = &g_constructorSurfacePixel;

    g_zVideo_SwSurfaceState = testSurface;
    g_zVideo_PrimarySurfaceState = testSurface;
    g_zVideo_DisplayModeSurfaceState = testSurface;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceDispatch;
    g_zVideo_pfnUnlockSurfaceState = TestVideoSurfaceDispatch;
    g_zVideo_pfnAdjustSurfaces = TestAdjustSurfaces;
    g_zVideo_AdjustSurfacesDisableGate = 0;
    g_zVideo_RendererType = 0;
    g_zVideo_UseHalfResBackbuffer = 0;
    g_zVideo_HalfResAdjustMode = 0;
    g_zSnd_ActiveBackend = 0;
    g_zSndLastVoice = 0;
    g_zSndLastVoiceHandle = 0;
    g_zSndLastVoiceMarkerIndex = 0;
    g_zSndLastVoiceStopMarkerIndex = 999;
    g_zSndFadeActiveListCount = 0;
    g_FrameDeltaTimeSec = 0.0f;
}

void RestoreConstructorGlobals(
    const ConstructorGlobalState &state
) {
    g_zVideo_SwSurfaceState = state.swSurface;
    g_zVideo_PrimarySurfaceState = state.primarySurface;
    g_zVideo_DisplayModeSurfaceState = state.displaySurface;
    g_zVideo_pfnLockSurfaceState = state.lockSurface;
    g_zVideo_pfnUnlockSurfaceState = state.unlockSurface;
    g_zVideo_pfnAdjustSurfaces = state.adjustSurfaces;
    g_zVideo_AdjustSurfacesDisableGate = state.adjustDisableGate;
    g_zVideo_RendererType = state.rendererType;
    g_zVideo_UseHalfResBackbuffer = state.useHalfResBackbuffer;
    g_zVideo_HalfResAdjustMode = state.halfResAdjustMode;
    g_zVideo_FrameTick = state.frameTick;
    g_zSnd_ActiveBackend = state.sndActiveBackend;
    g_zSndLastVoice = state.lastVoice;
    g_zSndLastVoiceHandle = state.lastVoiceHandle;
    g_zSndLastVoiceMarkerIndex = state.lastVoiceMarkerIndex;
    g_zSndLastVoiceStopMarkerIndex = state.lastVoiceStopMarkerIndex;
    g_zSndFadeActiveListCount = state.fadeActiveCount;
    g_FrameDeltaTimeSec = state.frameDeltaTimeSec;
    g_Time_CurrentTimeSec = state.timeCurrentSec;
    g_Time_NewTimeSec = state.timeNewSec;
    g_Time_AccumulatedTimeSec = state.timeAccumulatedSec;
    g_Time_UnscaledDeltaTimeSec = state.timeUnscaledDeltaSec;
    g_Time_UnscaledAccumulatedTimeSec = state.timeUnscaledAccumulatedSec;
    g_Briefing_ProgressEventCode = state.progressEventCode;
}

int CountActionNodes(
    BriefingActionNode *head
) {
    int count = 0;
    for (BriefingActionNode *node = head->next; node != head; node = node->next) {
        ++count;
        if (count > 128) {
            return -1;
        }
    }

    return count;
}

void DeleteQueuedActions(
    BriefingActionNode *head
) {
    for (BriefingActionNode *node = head->next; node != head; node = node->next) {
        ::operator delete(node->action);
        node->action = 0;
    }
}

void InitActionQueue(
    HudUiBriefingRuntime *runtime,
    BriefingActionNode *sentinel
) {
    sentinel->prev = sentinel;
    sentinel->next = sentinel;
    sentinel->action = 0;
    runtime->actionQueue.headSentinel = sentinel;
    runtime->actionQueue.nodeCount = 0;
    runtime->actionQueue.currentNode = sentinel;
    runtime->actionQueue.sequenceActive = 0;
}

void ConstructBriefingUpdateMembers(
    HudUiBriefingRuntime *runtime
) {
    std::memset(runtime, 0, sizeof(*runtime));
    new (&runtime->transportProgress) HudUiBriefingTransportProgress;
    new (&runtime->missionName) HudUiPanel;
    new (&runtime->objectiveSummary) HudUiPanel;
    new (&runtime->objectiveDesc) HudUiPanel;
    new (&runtime->objectivePicture) HudUiBriefingObjectivePicture;
    new (&runtime->transmissionHalted) HudUiPanel;
    for (int index = 0; index < 6; ++index) {
        new (&runtime->locatorPanels[index]) HudUiBriefingLocatorPanel;
    }
}

} // namespace

extern "C" int briefing_runtime_constructor_smoke(void) {
    const int kMissionId = 7;

    ConstructorGlobalState state = {};
    PrepareConstructorGlobals(state);
    CodeFunctionPatch postprocessPatch = {};
    CodeFunctionPatch compositeConstructorPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
            reinterpret_cast<void *>(&TestRunPostprocessOnPrimaryBuffer),
            postprocessPatch
        ) ||
        !PatchFunctionJump(
            MethodAddress(&HudUiCompositePanel::ConstructorWithEntryCount),
            MethodAddress(&TestCompositePanelConstructor::ConstructorWithEntryCount),
            compositeConstructorPatch
        )) {
        RestoreFunctionPatch(postprocessPatch);
        RestoreFunctionPatch(compositeConstructorPatch);
        RestoreConstructorGlobals(state);
        return 9;
    }

    alignas(4) static unsigned char storage[sizeof(HudUiBriefingRuntime)];
    std::memset(storage, 0, sizeof(storage));
    HudUiBriefingRuntime *const runtime =
        reinterpret_cast<HudUiBriefingRuntime *>(storage);
    HudUiBriefingRuntime *const result = runtime->Constructor(kMissionId);

    int failure = 0;
    BriefingActionNode *const sentinel = runtime->actionQueue.headSentinel;
    const bool queueOk =
        runtime->actionQueue.missionId == (kMissionId & 0xff) &&
        sentinel != 0 &&
        sentinel->prev == sentinel &&
        sentinel->next == sentinel &&
        runtime->actionQueue.nodeCount == 0 &&
        runtime->actionQueue.sequenceActive == 0;
    const bool widgetOk =
        runtime->transportProgress.normalizedValue == 0.0f &&
        runtime->objectivePicture.noiseAlpha == 0.0f &&
        (runtime->missionName.flags & 0x10u) != 0 &&
        (runtime->messagesPanel.flags & 0x10u) == 0 &&
        runtime->enabled != 0;

    if (result != runtime) {
        failure = 1;
    } else if (!queueOk) {
        failure = 2;
    } else if (g_Briefing_ProgressEventCode != -1) {
        failure = 3;
    } else if (!widgetOk) {
        failure = 4;
    }

    if (sentinel != 0) {
        ::operator delete(sentinel);
        runtime->actionQueue.headSentinel = 0;
    }
    RestoreFunctionPatch(compositeConstructorPatch);
    RestoreFunctionPatch(postprocessPatch);
    RestoreConstructorGlobals(state);
    return failure;
}

extern "C" int briefing_locator_panel_constructor_smoke(void) {
    alignas(4) unsigned char storage[sizeof(HudUiBriefingLocatorPanel)] = {};
    HudUiBriefingLocatorPanel *const locator =
        new (storage) HudUiBriefingLocatorPanel;

    const unsigned int expectedColor =
        static_cast<unsigned short>(zVid_PackColorRGB(
            0xff,
            0,
            0
        ));

    const bool ok =
        locator->x == 100 &&
        locator->y == 110 &&
        (locator->flags & 0x10u) != 0 &&
        locator->radius == 30 &&
        locator->radiusSquared == 900 &&
        locator->color565 == expectedColor;
    return ok ? 0 : 1;
}

extern "C" int briefing_locator_panel_blit_dirty_rect_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit = g_zVideo_pfnBltSourceToPrimary;
    g_zVideo_pfnBltSourceToPrimary = TestLocatorBltSourceToPrimary;

    HudUiBriefingLocatorPanel locator;

    g_locatorBlitCount = 0;
    g_locatorBlitImage = 0;
    locator.bltSource = 0;
    locator.BlitDirtyRect();
    const bool nullSkipped =
        g_locatorBlitCount == 0 &&
        g_locatorBlitImage == 0;

    zVidImagePartial image{};
    locator.bltSource = &image;
    locator.clipRect.left = 4;
    locator.clipRect.top = 5;
    locator.clipRect.right = 24;
    locator.clipRect.bottom = 25;
    g_locatorBlitCount = 0;
    g_locatorBlitImage = 0;
    locator.BlitDirtyRect();

    const bool blitted =
        g_locatorBlitCount == 1 &&
        g_locatorBlitImage == &image &&
        g_locatorBlitX == 4 &&
        g_locatorBlitY == 5 &&
        g_locatorBlitFlags == 0 &&
        g_locatorBlitHasRect != 0 &&
        g_locatorBlitRect.left == 4 &&
        g_locatorBlitRect.top == 5 &&
        g_locatorBlitRect.right == 24 &&
        g_locatorBlitRect.bottom == 25;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    return nullSkipped && blitted ? 0 : 1;
}

extern "C" int briefing_locator_panel_update_smoke(void) {
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;

    HudUiBriefingLocatorPanel locator;

    locator.flags = 0;
    locator.clipRect.left = 1;
    locator.clipRect.top = 2;
    locator.clipRect.right = 3;
    locator.clipRect.bottom = 4;
    locator.radius = 12;
    locator.radiusSquared = 144;
    g_HudUi_InvalidateMask = 0x80;
    locator.Update(1.0f);
    const bool visibleSkipped =
        locator.flags == 0 &&
        locator.clipRect.left == 1 &&
        locator.clipRect.top == 2 &&
        locator.clipRect.right == 3 &&
        locator.clipRect.bottom == 4 &&
        locator.radius == 12 &&
        locator.radiusSquared == 144;

    locator.flags = 0x10 | 0x02 | 0x08;
    locator.x = 100;
    locator.y = 110;
    locator.radius = 12;
    locator.radiusSquared = 144;
    locator.Update(0.25f);
    const bool shrunk =
        locator.clipRect.left == 88 &&
        locator.clipRect.top == 98 &&
        locator.clipRect.right == 113 &&
        locator.clipRect.bottom == 123 &&
        locator.radius == 7 &&
        locator.radiusSquared == 49 &&
        (locator.flags & 0x08u) == 0 &&
        (locator.flags & 0x80u) != 0;

    locator.flags = 0x10;
    locator.radius = 4;
    locator.radiusSquared = 16;
    locator.Update(0.01f);
    const bool minStep =
        locator.radius == 3 &&
        locator.radiusSquared == 9 &&
        (locator.flags & 0x80u) != 0;

    locator.flags = 0x10;
    locator.radius = 2;
    locator.radiusSquared = 4;
    locator.Update(0.01f);
    const bool minClamp =
        locator.radius == 3 &&
        locator.radiusSquared == 9 &&
        (locator.flags & 0x80u) != 0;

    g_HudUi_InvalidateMask = oldInvalidateMask;
    if (!visibleSkipped) {
        return 2;
    }
    if (!shrunk) {
        return 3;
    }
    if (!minStep) {
        return 4;
    }
    if (locator.radius != 3) {
        return 5;
    }
    if (locator.radiusSquared != 9) {
        return 6;
    }
    if ((locator.flags & 0x80u) == 0) {
        return 7;
    }
    return 0;
}

extern "C" int briefing_runtime_destructor_smoke(void) {
    alignas(4) unsigned char storage[sizeof(HudUiBriefingRuntime)] = {};
    HudUiBriefingRuntime *const runtime =
        reinterpret_cast<HudUiBriefingRuntime *>(storage);

    new ((HudUiBackground *)runtime) HudUiBackground;
    new (&runtime->transportProgress) HudUiBriefingTransportProgress;
    new (&runtime->missionName) HudUiPanel;
    new (&runtime->objectiveSummary) HudUiPanel;
    new (&runtime->objectiveDesc) HudUiPanel;
    new (&runtime->objectivePicture) HudUiBriefingObjectivePicture;
    new (&runtime->transmissionHalted) HudUiPanel;
    runtime->messagesPanel.ConstructorWithEntryCount(2);

    void *locatorInitialVptr[6] = {};
    for (std::size_t index = 0; index < 6; ++index) {
        new (&runtime->locatorPanels[index]) HudUiBriefingLocatorPanel;
        locatorInitialVptr[index] =
            *reinterpret_cast<void **>(&runtime->locatorPanels[index]);
    }

    BriefingActionNode *const sentinel =
        static_cast<BriefingActionNode *>(::operator new(sizeof(BriefingActionNode)));
    BriefingActionNode *const first =
        static_cast<BriefingActionNode *>(::operator new(sizeof(BriefingActionNode)));
    BriefingActionNode *const second =
        static_cast<BriefingActionNode *>(::operator new(sizeof(BriefingActionNode)));
    sentinel->prev = second;
    sentinel->next = first;
    sentinel->action = 0;
    first->prev = sentinel;
    first->next = second;
    first->action = 0;
    second->prev = first;
    second->next = sentinel;
    second->action = 0;
    runtime->actionQueue.headSentinel = sentinel;
    runtime->actionQueue.nodeCount = 2;

    runtime->Destructor();

    bool locatorsReset = true;
    for (std::size_t index = 0; index < 6; ++index) {
        locatorsReset =
            locatorsReset &&
            *reinterpret_cast<void **>(&runtime->locatorPanels[index]) !=
                locatorInitialVptr[index] &&
            *reinterpret_cast<void **>(&runtime->locatorPanels[index]) != 0;
    }

    const bool messageEntriesDestroyed =
        runtime->messagesPanel.entryVector.begin == 0 &&
        runtime->messagesPanel.entryVector.end == 0 &&
        runtime->messagesPanel.entryVector.capacityEnd == 0;
    const bool actionQueueReset =
        runtime->actionQueue.headSentinel == 0 &&
        runtime->actionQueue.nodeCount == 0;
    const bool transportDestructed =
        runtime->transportProgress.image == 0;
    const bool baseDestructed = runtime->enabled == 0;

    if (!locatorsReset) {
        return 2;
    }
    if (!messageEntriesDestroyed) {
        return 3;
    }
    if (!actionQueueReset) {
        return 4;
    }
    if (!transportDestructed) {
        return 5;
    }
    if (!baseDestructed) {
        return 6;
    }
    return 0;
}

extern "C" int briefing_runtime_update_smoke(void) {
    alignas(4) static unsigned char storage[sizeof(HudUiBriefingRuntime)];
    HudUiBriefingRuntime *const runtime =
        reinterpret_cast<HudUiBriefingRuntime *>(storage);

    ConstructBriefingUpdateMembers(runtime);
    BriefingActionNode sentinel = {};
    InitActionQueue(runtime, &sentinel);

    const unsigned int oldMask = g_HudUi_InvalidateMask;
    g_HudUi_InvalidateMask = 0x80;
    runtime->enabled = 0;
    runtime->objectivePicture.flags = 0;
    runtime->transmissionHalted.flags = 0;
    runtime->missionName.flags = 0;
    runtime->transportProgress.flags = 0;
    runtime->objectiveSummary.flags = 0;
    runtime->objectiveDesc.flags = 0;

    runtime->actionQueue.sequenceActive = 1;
    runtime->actionQueue.currentNode = runtime->actionQueue.headSentinel;
    g_Briefing_AllowAdvanceFlag = 1;
    runtime->HudUiBriefingRuntime::Update(0.125f);

    const bool sentinelComplete =
        g_Briefing_AllowAdvanceFlag == 0 &&
        (runtime->objectivePicture.flags & 0x80u) != 0 &&
        (runtime->transmissionHalted.flags & 0x80u) != 0 &&
        (runtime->missionName.flags & 0x80u) != 0 &&
        (runtime->transportProgress.flags & 0x80u) != 0 &&
        (runtime->objectiveSummary.flags & 0x80u) != 0 &&
        (runtime->objectiveDesc.flags & 0x80u) != 0;

    runtime->actionQueue.AddDelayUntilProgress(3);
    runtime->actionQueue.sequenceActive = 1;
    runtime->actionQueue.currentNode = runtime->actionQueue.headSentinel->next;
    g_Briefing_ProgressEventCode = 3;
    runtime->HudUiBriefingRuntime::Update(0.125f);
    const bool tickAdvanced =
        runtime->actionQueue.currentNode == runtime->actionQueue.headSentinel;

    g_HudUi_InvalidateMask = oldMask;
    DeleteQueuedActions(runtime->actionQueue.headSentinel);
    return sentinelComplete && tickAdvanced ? 0 : 1;
}

extern "C" int hud_sensor_tracker_get_objective_briefing_strings_smoke(void) {
    HudSensorTracker tracker = {};
    zVidImagePartial image = {};

    std::strcpy(tracker.objectiveSlots[2].objectiveTitle, "brief summary");
    std::strcpy(tracker.objectiveSlots[2].objectiveDesc, "brief description");
    tracker.objectiveSlots[2].objectiveImage = &image;

    char *summary = 0;
    char *description = 0;
    zVidImagePartial *imageRef = 0;
    const int result = tracker.GetObjectiveBriefingStringsAndImageRef(
        2,
        &summary,
        &description,
        &imageRef
    );

    return result == 1 && summary == tracker.objectiveSlots[2].objectiveTitle &&
                   description == tracker.objectiveSlots[2].objectiveDesc && imageRef == &image
               ? 0
               : 1;
}

extern "C" int zopt_network_enabled_accessor_smoke(void) {
    int networkEnabled = 0;
    int networkModem = 0;
    int networkListen = 0;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int *const oldNetworkModem = g_zOpt_NetworkModemOption;
    int *const oldNetworkListen = g_zOpt_NetworkListenOption;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zOpt_NetworkModemOption = &networkModem;
    g_zOpt_NetworkListenOption = &networkListen;

    const bool disabled = zOpt::GetNetworkEnabled() == 0;
    zOpt::SetNetworkEnabled(1);
    zOpt::SetNetworkModemEnabled(1);
    zOpt::SetNetworkListenEnabled(1);
    const bool enabled = zOpt::GetNetworkEnabled() == 1;
    const bool modemEnabled = networkModem == 1 && zOpt::GetNetworkModemEnabled() == 1;
    const bool listenEnabled = networkListen == 1;

    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zOpt_NetworkModemOption = oldNetworkModem;
    g_zOpt_NetworkListenOption = oldNetworkListen;
    return disabled && enabled && modemEnabled && listenEnabled ? 0 : 1;
}

extern "C" int hud_sensor_mission_identity_smoke(void) {
    HudSensorTracker tracker = {};
    if (tracker.SetZbdPath("missions\\m01.zbd") != 1 ||
        std::strcmp((const char *)tracker.zbdPath, "missions\\m01.zbd") != 0) {
        return 1;
    }

    const bool initOk =
        tracker.InitMissionIdAndFlags(7, 0x55) == 1 &&
        tracker.missionId == 7 &&
        tracker.GetMissionId() == 7 &&
        tracker.missionFlags == 0x55 &&
        ((const char *)tracker.zbdPath)[0] == '\0';

    const bool clearOk =
        tracker.SetZbdPath("alternate.zbd") == 1 &&
        std::strcmp((const char *)tracker.zbdPath, "alternate.zbd") == 0 &&
        tracker.SetZbdPath(0) == 1 &&
        ((const char *)tracker.zbdPath)[0] == '\0';

    const bool setIdOk =
        tracker.SetZbdPath("pending.zbd") == 1 &&
        tracker.SetMissionId(12) == 1 &&
        tracker.GetMissionId() == 12 &&
        ((const char *)tracker.zbdPath)[0] == '\0';

    return initOk && clearOk && setIdOk ? 0 : 2;
}

extern "C" int briefing_build_objective_actions_smoke(void) {
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const int oldMissionId = g_HudSensorTracker.missionId;
    const int oldObjectiveCount = g_HudSensorTracker.objectiveCount;
    HudSensorObjectiveSlot oldSlots[3] = {};
    for (int index = 0; index < 3; ++index) {
        oldSlots[index] = g_HudSensorTracker.objectiveSlots[index];
    }
    HMODULE const oldMessagesDll = g_zLoc_MessagesDllHandle;

    HMODULE messagesDll = LoadLibraryA("support\\messages.dll");
    if (messagesDll == 0) {
        messagesDll = LoadLibraryA("..\\..\\..\\..\\support\\messages.dll");
    }
    if (messagesDll == 0) {
        return 1;
    }

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zLoc_MessagesDllHandle = messagesDll;
    g_HudSensorTracker.missionId = 5;
    g_HudSensorTracker.objectiveCount = 3;

    zVidImagePartial image1 = {};
    zVidImagePartial image2 = {};
    std::strcpy(g_HudSensorTracker.objectiveSlots[1].objectiveTitle, "summary one");
    std::strcpy(g_HudSensorTracker.objectiveSlots[1].objectiveDesc, "description one");
    g_HudSensorTracker.objectiveSlots[1].objectiveImage = &image1;
    std::strcpy(g_HudSensorTracker.objectiveSlots[2].objectiveTitle, "summary two");
    std::strcpy(g_HudSensorTracker.objectiveSlots[2].objectiveDesc, "description two");
    g_HudSensorTracker.objectiveSlots[2].objectiveImage = &image2;

    alignas(4) static unsigned char storage[sizeof(HudUiBriefingRuntime)];
    HudUiBriefingRuntime *const runtime =
        reinterpret_cast<HudUiBriefingRuntime *>(storage);
    ConstructBriefingUpdateMembers(runtime);
    BriefingActionNode sentinel = {};
    InitActionQueue(runtime, &sentinel);

    int failure = 0;
    const int result = runtime->BuildObjectiveActionsFromIndex(1);
    const int count = CountActionNodes(runtime->actionQueue.headSentinel);
    const bool sequenceStarted =
        result == 1 &&
        count == 25 &&
        runtime->actionQueue.nodeCount == 25 &&
        runtime->actionQueue.sequenceActive == 1 &&
        runtime->actionQueue.currentNode == runtime->actionQueue.headSentinel->prev &&
        g_Briefing_SequenceActiveFlag == 1;

    networkEnabled = 1;
    const int oldCount = runtime->actionQueue.nodeCount;
    const int networkResult = runtime->BuildObjectiveActionsFromIndex(1);
    const bool networkSkipped =
        networkResult == 0 &&
        runtime->actionQueue.nodeCount == oldCount;

    failure = sequenceStarted && networkSkipped ? 0 : 2;
    DeleteQueuedActions(runtime->actionQueue.headSentinel);

    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_HudSensorTracker.missionId = oldMissionId;
    g_HudSensorTracker.objectiveCount = oldObjectiveCount;
    for (int index = 0; index < 3; ++index) {
        g_HudSensorTracker.objectiveSlots[index] = oldSlots[index];
    }
    g_zLoc_MessagesDllHandle = oldMessagesDll;
    FreeLibrary(messagesDll);

    return failure;
}
