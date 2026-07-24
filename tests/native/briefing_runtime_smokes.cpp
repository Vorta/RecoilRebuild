#include "Battlesport/briefing.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

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

void WriteTestU32(
    HANDLE file,
    std::uint32_t value
) {
    DWORD written = 0;
    WriteFile(
        file,
        &value,
        sizeof(value),
        &written,
        0
    );
}

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

unsigned short BriefingSmokeGray565(
    unsigned char value
) {
    const unsigned short level = (unsigned short)(value & 0x1f);
    return (unsigned short)((level << 11) | (level << 6) | level);
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
    state.frameDeltaTimeSec = g_FrameDeltaTimeSec;
    state.timeCurrentSec = g_Time_RuntimeConfig.currentTimeSec;
    state.timeNewSec = g_Time_RuntimeConfig.newTimeSec;
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
    g_FrameDeltaTimeSec = state.frameDeltaTimeSec;
    g_Time_RuntimeConfig.currentTimeSec = state.timeCurrentSec;
    g_Time_RuntimeConfig.newTimeSec = state.timeNewSec;
    g_Time_AccumulatedTimeSec = state.timeAccumulatedSec;
    g_Time_UnscaledDeltaTimeSec = state.timeUnscaledDeltaSec;
    g_Time_UnscaledAccumulatedTimeSec = state.timeUnscaledAccumulatedSec;
    g_Briefing_ProgressEventCode = state.progressEventCode;
}

int CountQueuedActions(
    const Briefing_ActionQueue &queue
) {
    int count = 0;
    std::list<BriefingAction *>::const_iterator action = queue.actions.begin();
    for (; action != queue.actions.end(); ++action) {
        ++count;
        if (count > 128) {
            return -1;
        }
    }

    return count;
}

void DeleteQueuedActions(
    Briefing_ActionQueue &queue
) {
    std::list<BriefingAction *>::iterator action = queue.actions.begin();
    for (; action != queue.actions.end(); ++action) {
        delete *action;
        *action = 0;
    }
}

} // namespace

extern "C" int briefing_runtime_constructor_smoke(void) {
    const int kMissionId = 7;

    ConstructorGlobalState state = {};
    PrepareConstructorGlobals(state);
    CodeFunctionPatch postprocessPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
            reinterpret_cast<void *>(&TestRunPostprocessOnPrimaryBuffer),
            postprocessPatch
        )) {
        RestoreFunctionPatch(postprocessPatch);
        RestoreConstructorGlobals(state);
        return 9;
    }

    HudUiBriefingRuntime *const runtime =
        new HudUiBriefingRuntime(kMissionId);
    HudUiBriefingRuntime *const result = runtime;

    int failure = 0;
    const bool queueOk =
        runtime->actionQueue.actions.empty() &&
        runtime->actionQueue.actions.size() == 0 &&
        runtime->actionQueue.actions.begin() == runtime->actionQueue.actions.end() &&
        runtime->actionQueue.active == 0;
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

    delete runtime;
    RestoreFunctionPatch(postprocessPatch);
    RestoreConstructorGlobals(state);
    return failure;
}

extern "C" int briefing_locator_panel_constructor_smoke(void) {
    HudUiBriefingLocatorPanel locator;

    const unsigned int expectedColor =
        static_cast<unsigned short>(zVid_PackColorRGB(
            0xff,
            0,
            0
        ));

    const bool ok =
        locator.x == 100 &&
        locator.y == 110 &&
        (locator.flags & 0x10u) != 0 &&
        locator.radius == 30 &&
        locator.radiusSquared == 900 &&
        locator.color565 == expectedColor;
    return ok ? 0 : 1;
}

extern "C" int briefing_locator_panel_blit_dirty_rect_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit = g_zVideo_pfnBltSourceToPrimary;
    g_zVideo_pfnBltSourceToPrimary = TestLocatorBltSourceToPrimary;

    HudUiBriefingLocatorPanel locator;

    g_locatorBlitCount = 0;
    g_locatorBlitImage = 0;
    locator.bltSource = 0;
    locator.DrawBase();
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
    locator.DrawBase();

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

    locator.flags = 0x10;
    locator.clipRect.left = 1;
    locator.clipRect.top = 2;
    locator.clipRect.right = 3;
    locator.clipRect.bottom = 4;
    locator.radius = 12;
    locator.radiusSquared = 144;
    g_HudUi_InvalidateMask = 0x80;
    locator.Update(1.0f);
    const bool hiddenSkipped =
        locator.flags == 0x10 &&
        locator.clipRect.left == 1 &&
        locator.clipRect.top == 2 &&
        locator.clipRect.right == 3 &&
        locator.clipRect.bottom == 4 &&
        locator.radius == 12 &&
        locator.radiusSquared == 144;

    locator.flags = 0x02 | 0x08;
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

    locator.flags = 0;
    locator.radius = 4;
    locator.radiusSquared = 16;
    locator.Update(0.01f);
    const bool minStep =
        locator.radius == 3 &&
        locator.radiusSquared == 9 &&
        (locator.flags & 0x80u) != 0;

    locator.flags = 0;
    locator.radius = 2;
    locator.radiusSquared = 4;
    locator.Update(0.01f);
    const bool minClamp =
        locator.radius == 3 &&
        locator.radiusSquared == 9 &&
        (locator.flags & 0x80u) != 0;

    g_HudUi_InvalidateMask = oldInvalidateMask;
    if (!hiddenSkipped) {
        return 2;
    }
    if (!shrunk) {
        return 3;
    }
    if (!minStep) {
        return 4;
    }
    if (!minClamp) {
        return 5;
    }
    return 0;
}

extern "C" int briefing_objective_picture_draw_noise_overlay_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit = g_zVideo_pfnBltSourceToPrimary;
    zVidImagePartial *const oldExclusiveImage = g_HudUiWidget_ExclusiveDrawImage;
    unsigned char *const oldNoiseTable = g_zVid_NoiseByteTable;
    const int oldNoiseTableSize = g_zVid_NoiseByteTableSize;
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;

    HudUiBriefingObjectivePicture picture;
    zVidImagePartial image = {};
    image.width = 3;
    image.height = 2;
    picture.image = &image;
    picture.x = 2;
    picture.y = 1;
    picture.dirtyRectCount = 0;
    picture.bltClipRectOrNull = 0;

    unsigned char noiseTable[32] = {};
    for (int index = 0; index < 32; ++index) {
        noiseTable[index] = (unsigned char)(index);
    }

    unsigned short pixels[64] = {};
    for (int index = 0; index < 64; ++index) {
        pixels[index] = 0xaaaa;
    }

    g_zVideo_pfnBltSourceToPrimary = TestLocatorBltSourceToPrimary;
    g_HudUiWidget_ExclusiveDrawImage = 0;
    g_zVid_NoiseByteTable = noiseTable;
    g_zVid_NoiseByteTableSize = 32;
    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 8;
    g_zVideo_FxSurfaceHeight = 8;
    g_zVideo_FxSurfacePitchBytes = 16;
    g_zVideo_FxSurfacePitchPixels16 = 8;
    zVideo::PixelPack_SetupFromMasks(
        5,
        6,
        5,
        0xf800,
        0x07e0,
        0x001f
    );

    g_locatorBlitCount = 0;
    g_locatorBlitImage = 0;
    picture.noiseAlpha = 0.0f;
    picture.Draw();

    bool lowAlphaOk =
        g_locatorBlitCount == 1 &&
        g_locatorBlitImage == &image &&
        g_locatorBlitX == 2 &&
        g_locatorBlitY == 1 &&
        g_locatorBlitFlags == 0 &&
        g_locatorBlitHasRect == 0;
    for (int index = 0; index < 64; ++index) {
        lowAlphaOk = lowAlphaOk && pixels[index] == 0xaaaa;
    }

    for (int index = 0; index < 64; ++index) {
        pixels[index] = 0xaaaa;
    }

    std::srand(11);
    const int rowWidth = image.width;
    const int firstOffset =
        (std::rand() * (g_zVid_NoiseByteTableSize - rowWidth)) / 0x7fff;
    const int secondOffset =
        (std::rand() * (g_zVid_NoiseByteTableSize - rowWidth)) / 0x7fff;
    std::srand(11);
    g_locatorBlitCount = 0;
    picture.noiseAlpha = 1.0f;
    picture.Draw();

    const bool noiseOk =
        g_locatorBlitCount == 1 &&
        g_locatorBlitImage == &image &&
        pixels[2 + 1 * 8] == BriefingSmokeGray565(noiseTable[firstOffset]) &&
        pixels[3 + 1 * 8] == BriefingSmokeGray565(noiseTable[firstOffset + 1]) &&
        pixels[4 + 1 * 8] == BriefingSmokeGray565(noiseTable[firstOffset + 2]) &&
        pixels[2 + 2 * 8] == BriefingSmokeGray565(noiseTable[secondOffset]) &&
        pixels[3 + 2 * 8] == BriefingSmokeGray565(noiseTable[secondOffset + 1]) &&
        pixels[4 + 2 * 8] == BriefingSmokeGray565(noiseTable[secondOffset + 2]) &&
        pixels[0] == 0xaaaa &&
        pixels[1 + 1 * 8] == 0xaaaa &&
        pixels[5 + 1 * 8] == 0xaaaa &&
        pixels[2 + 3 * 8] == 0xaaaa;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    g_HudUiWidget_ExclusiveDrawImage = oldExclusiveImage;
    g_zVid_NoiseByteTable = oldNoiseTable;
    g_zVid_NoiseByteTableSize = oldNoiseTableSize;
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;

    return lowAlphaOk && noiseOk ? 0 : 1;
}

extern "C" int briefing_runtime_destructor_smoke(void) {
    ConstructorGlobalState state = {};
    PrepareConstructorGlobals(state);
    CodeFunctionPatch postprocessPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
            reinterpret_cast<void *>(&TestRunPostprocessOnPrimaryBuffer),
            postprocessPatch
        )) {
        RestoreConstructorGlobals(state);
        return 1;
    }

    HudUiBriefingRuntime *const runtime =
        new HudUiBriefingRuntime(7);

    runtime->actionQueue.AddDelayUntilProgress(1);
    runtime->actionQueue.AddDelayUntilProgress(2);
    const bool actionQueuePopulated =
        runtime->actionQueue.actions.size() == 2 &&
        runtime->actionQueue.actions.begin() != runtime->actionQueue.actions.end();
    const bool ownedMembersConstructed =
        runtime->messagesPanel.entryVector.size() == 25 &&
        runtime->transportProgress.normalizedValue == 0.0f;
    DeleteQueuedActions(runtime->actionQueue);

    delete runtime;
    RestoreFunctionPatch(postprocessPatch);
    RestoreConstructorGlobals(state);
    return actionQueuePopulated && ownedMembersConstructed ? 0 : 2;
}

extern "C" int briefing_runtime_update_smoke(void) {
    ConstructorGlobalState constructorState = {};
    PrepareConstructorGlobals(constructorState);
    CodeFunctionPatch postprocessPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
            reinterpret_cast<void *>(&TestRunPostprocessOnPrimaryBuffer),
            postprocessPatch
        )) {
        RestoreConstructorGlobals(constructorState);
        return 2;
    }

    HudUiBriefingRuntime *const runtime =
        new HudUiBriefingRuntime(7);

    const unsigned int oldMask = g_HudUi_InvalidateMask;
    const int oldAllowAdvance = g_Briefing_AllowAdvanceFlag;
    const int oldProgressEvent = g_Briefing_ProgressEventCode;
    g_HudUi_InvalidateMask = 0x80;
    runtime->enabled = 0;
    runtime->objectivePicture.flags = 0;
    runtime->transmissionHalted.flags = 0;
    runtime->missionName.flags = 0;
    runtime->transportProgress.flags = 0;
    runtime->objectiveSummary.flags = 0;
    runtime->objectiveDesc.flags = 0;

    runtime->actionQueue.active = 1;
    runtime->actionQueue.current = runtime->actionQueue.actions.end();
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
    runtime->actionQueue.active = 1;
    runtime->actionQueue.current = runtime->actionQueue.actions.begin();
    g_Briefing_ProgressEventCode = 3;
    runtime->HudUiBriefingRuntime::Update(0.125f);
    const bool tickAdvanced =
        runtime->actionQueue.current == runtime->actionQueue.actions.end();

    DeleteQueuedActions(runtime->actionQueue);
    delete runtime;
    g_HudUi_InvalidateMask = oldMask;
    g_Briefing_AllowAdvanceFlag = oldAllowAdvance;
    g_Briefing_ProgressEventCode = oldProgressEvent;
    RestoreFunctionPatch(postprocessPatch);
    RestoreConstructorGlobals(constructorState);
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

extern "C" int zgame_options_load_game_options_minimal_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(
            sizeof(tempDir),
            tempDir
        ) == 0 ||
        GetTempFileNameA(
            tempDir,
            "zgo",
            0,
            tempPath
        ) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(
        tempPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        0,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_TEMPORARY,
        0
    );
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteTestU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteTestU32(file, 1);
    WriteTestU32(file, zReader::ZRDR_NODE_INT);
    WriteTestU32(file, 1);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(
        file,
        0,
        0,
        FILE_CURRENT
    );
    std::strcpy(
        record.name,
        "detail.zrd"
    );

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;

    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zInput_BindMapContext *const oldBindMapCurrent = g_zInput_BindMap_Current;
    g_zArchive_MountedList = &list;

    zGame::Options_InitRegistryContext(
        "RecoilSmoke",
        "CurrentUser",
        "Game"
    );
    zInput::BindMapSystem_Init(0x30);

    const int result = zGame::Options_LoadGameOptions();
    const bool ok =
        result == 1 &&
        g_zGame_Options_PointerCache.videoAcceleration != 0 &&
        zVid::GetAccelerationOption() == 1 &&
        g_zGame_Options_PointerCache.videoStride != 0 &&
        *g_zGame_Options_PointerCache.videoStride == 1 &&
        zInput::BindGroupList_GetCount() == 5 &&
        g_zGame_Options_PointerCache.networkEnabled != 0 &&
        *g_zGame_Options_PointerCache.networkEnabled == 0;

    zInput::BindMapSystem_Shutdown();
    g_zInput_BindMap_Current = oldBindMapCurrent;
    zGame::Options_ShutdownRegistryContext();
    g_zArchive_MountedList = oldMountedList;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 3;
}

extern "C" int zopt_network_enabled_accessor_smoke(void) {
    int networkEnabled = 0;
    int networkModem = 0;
    int networkListen = 0;
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    int *const oldNetworkModem = g_zGame_Options_PointerCache.networkModem;
    int *const oldNetworkListen = g_zGame_Options_PointerCache.networkListen;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
    g_zGame_Options_PointerCache.networkModem = &networkModem;
    g_zGame_Options_PointerCache.networkListen = &networkListen;

    const bool disabled = zOpt::GetNetworkEnabled() == 0;
    zOpt::SetNetworkEnabled(1);
    zOpt::SetNetworkModemEnabled(1);
    zOpt::SetNetworkListenEnabled(1);
    const bool enabled = zOpt::GetNetworkEnabled() == 1;
    const bool modemEnabled = networkModem == 1 && zOpt::GetNetworkModemEnabled() == 1;
    const bool listenEnabled = networkListen == 1;

    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_zGame_Options_PointerCache.networkModem = oldNetworkModem;
    g_zGame_Options_PointerCache.networkListen = oldNetworkListen;
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
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    const int oldMissionId = g_HudSensorTracker.missionId;
    const int oldObjectiveCount = g_HudSensorTracker.objectiveCount;
    HudSensorObjectiveSlot oldSlots[3] = {};
    for (int index = 0; index < 3; ++index) {
        oldSlots[index] = g_HudSensorTracker.objectiveSlots[index];
    }
    HMODULE const oldMessagesDll = g_zLoc_MessagesDllHandle;
    const int oldSequenceActive = g_Briefing_SequenceActiveFlag;

    HMODULE messagesDll = LoadLibraryA("support\\messages.dll");
    if (messagesDll == 0) {
        messagesDll = LoadLibraryA("..\\..\\..\\..\\support\\messages.dll");
    }
    if (messagesDll == 0) {
        return 1;
    }

    int networkEnabled = 0;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
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

    ConstructorGlobalState constructorState = {};
    PrepareConstructorGlobals(constructorState);
    CodeFunctionPatch postprocessPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(&zVideo::RunPostprocessOnPrimaryBuffer),
            reinterpret_cast<void *>(&TestRunPostprocessOnPrimaryBuffer),
            postprocessPatch
        )) {
        RestoreConstructorGlobals(constructorState);
        g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
        g_HudSensorTracker.missionId = oldMissionId;
        g_HudSensorTracker.objectiveCount = oldObjectiveCount;
        for (int index = 0; index < 3; ++index) {
            g_HudSensorTracker.objectiveSlots[index] = oldSlots[index];
        }
        g_zLoc_MessagesDllHandle = oldMessagesDll;
        FreeLibrary(messagesDll);
        return 3;
    }

    HudUiBriefingRuntime *const runtime =
        new HudUiBriefingRuntime(5);

    int failure = 0;
    const int result = runtime->BuildObjectiveActionsFromIndex(1);
    const int count = CountQueuedActions(runtime->actionQueue);
    const bool sequenceStarted =
        result == 1 &&
        count == 25 &&
        runtime->actionQueue.actions.size() == 25 &&
        runtime->actionQueue.active == 1 &&
        runtime->actionQueue.current == runtime->actionQueue.actions.begin() &&
        g_Briefing_SequenceActiveFlag == 1;

    networkEnabled = 1;
    const std::size_t oldCount = runtime->actionQueue.actions.size();
    const int networkResult = runtime->BuildObjectiveActionsFromIndex(1);
    const bool networkSkipped =
        networkResult == 0 &&
        runtime->actionQueue.actions.size() == oldCount;

    failure = sequenceStarted && networkSkipped ? 0 : 2;
    DeleteQueuedActions(runtime->actionQueue);
    delete runtime;
    RestoreFunctionPatch(postprocessPatch);
    RestoreConstructorGlobals(constructorState);

    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_HudSensorTracker.missionId = oldMissionId;
    g_HudSensorTracker.objectiveCount = oldObjectiveCount;
    for (int index = 0; index < 3; ++index) {
        g_HudSensorTracker.objectiveSlots[index] = oldSlots[index];
    }
    g_Briefing_SequenceActiveFlag = oldSequenceActive;
    g_zLoc_MessagesDllHandle = oldMessagesDll;
    FreeLibrary(messagesDll);

    return failure;
}
