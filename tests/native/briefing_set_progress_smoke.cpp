// Checked-in focused native smoke translation unit, formerly extracted from briefing_tests.cpp.
// Emits focused Briefing smokes without broad briefing test duplicates.

#include "Battlesport/briefing.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

extern zSndPlayHandle *g_Briefing_CurrentSndHandle;
extern char g_Briefing_SndSetName[0x40];
extern "C" unsigned int g_HudUi_InvalidateMask;

namespace {
int g_deleteCount;
std::uint32_t g_deleteFlags;
HudUiBriefingRuntime *g_deletedRuntime;
int g_setProgressCount;
float g_setProgressValue;
void *g_setProgressThis;
int g_invalidateCount;
void *g_invalidateThis[8];
unsigned short g_constructorSurfacePixel;
int g_briefingSetVisibleCount;
void *g_briefingSetVisibleThis[16];
int g_briefingSetVisibleValue[16];
int g_briefingAdjustSurfaceCalls;
int g_briefingStopAfterAdjustCalls = 1;
int g_briefingStopRequested;
int g_briefingBlitCount;
zVidImagePartial *g_briefingBlitImage;
int g_briefingBlitX;
int g_briefingBlitY;
int g_briefingBlitFlags;
int g_briefingBlitHasRect;
zVidRect32 g_briefingBlitRect;
int g_briefingMessageEntryDtorCount;
void *g_briefingMessageEntryDtorThis[4];
unsigned int g_briefingMessageEntryDtorFlags[4];
int g_briefingTickDrawBaseCount;
int g_briefingTickInvalidateCount;
int g_briefingTickRebuildCount;
int g_briefingTickUpdateBoundsCount;
int g_briefingTickSetVisibleCount;
int g_briefingTickSetVisibleValue[8];
void *g_briefingTickSetVisibleThis[8];
void *g_briefingTickNoArgThis[16];
void *g_briefingTickPanelSetTextThis;
char g_briefingTickPanelSetText[0x100];

int __fastcall TestVideoSurfaceDispatch(zVideo_SurfaceStatePartial *) {
    return 0;
}

int __fastcall TestVideoSurfaceDispatchDisableBriefingRuntime(
    zVideo_SurfaceStatePartial *) {
    if (g_Briefing_Runtime != nullptr) {
        *reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(g_Briefing_Runtime) + 4) =
            0;
    }

    return 0;
}

int __fastcall TestAdjustSurfaces(zVidRect32 *, zVidRect32 *, int, int) {
    return 0;
}

int __fastcall TestAdjustSurfacesStopBriefingThread(zVidRect32 *, zVidRect32 *, int,
                                                         int) {
    ++g_briefingAdjustSurfaceCalls;
    if (g_briefingStopRequested != 0 ||
        (g_briefingStopAfterAdjustCalls > 0 &&
         g_briefingAdjustSurfaceCalls >= g_briefingStopAfterAdjustCalls)) {
        g_Briefing_ThreadRunFlag = 0;
    }
    return 0;
}

void __fastcall TestBriefingBltSourceToPrimary(zVidImagePartial *self, int dstX, int dstY,
                                                    int clipFlags, zVidRect32 *srcRect) {
    ++g_briefingBlitCount;
    g_briefingBlitImage = static_cast<zVidImagePartial *>(self);
    g_briefingBlitX = dstX;
    g_briefingBlitY = dstY;
    g_briefingBlitFlags = clipFlags;
    g_briefingBlitHasRect = srcRect != nullptr ? 1 : 0;
    if (srcRect != nullptr) {
        g_briefingBlitRect = *static_cast<zVidRect32 *>(srcRect);
    }
}

struct TestBriefingTickTarget {
    void DrawBase() {
        if (g_briefingTickDrawBaseCount < 16) {
            g_briefingTickNoArgThis[g_briefingTickDrawBaseCount] = this;
        }

        ++g_briefingTickDrawBaseCount;
    }

    void Invalidate() {
        if (g_briefingTickInvalidateCount < 16) {
            g_briefingTickNoArgThis[g_briefingTickInvalidateCount] = this;
        }

        ++g_briefingTickInvalidateCount;
    }

    void Rebuild() {
        ++g_briefingTickRebuildCount;
        g_briefingTickNoArgThis[0] = this;
    }

    void UpdateBounds() {
        ++g_briefingTickUpdateBoundsCount;
        g_briefingTickNoArgThis[1] = this;
    }

    void SetVisible(int visible) {
        if (g_briefingTickSetVisibleCount < 8) {
            g_briefingTickSetVisibleThis[g_briefingTickSetVisibleCount] = this;
            g_briefingTickSetVisibleValue[g_briefingTickSetVisibleCount] = visible;
        }

        ++g_briefingTickSetVisibleCount;
    }
};

void TestBriefingTickPanelSetText(void *self, const char *format, ...) {
    g_briefingTickPanelSetTextThis = self;
    std::strncpy(g_briefingTickPanelSetText, format, sizeof(g_briefingTickPanelSetText) - 1);
    g_briefingTickPanelSetText[sizeof(g_briefingTickPanelSetText) - 1] = '\0';
}

unsigned int MakeBriefingTickDrawBaseThunk() {
    union MemberToFunction {
        void ( TestBriefingTickTarget::*member)();
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingTickTarget::DrawBase;
    return thunk.fn;
}

unsigned int MakeBriefingTickInvalidateThunk() {
    union MemberToFunction {
        void ( TestBriefingTickTarget::*member)();
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingTickTarget::Invalidate;
    return thunk.fn;
}

unsigned int MakeBriefingTickRebuildThunk() {
    union MemberToFunction {
        void ( TestBriefingTickTarget::*member)();
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingTickTarget::Rebuild;
    return thunk.fn;
}

unsigned int MakeBriefingTickUpdateBoundsThunk() {
    union MemberToFunction {
        void ( TestBriefingTickTarget::*member)();
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingTickTarget::UpdateBounds;
    return thunk.fn;
}

unsigned int MakeBriefingTickSetVisibleThunk() {
    union MemberToFunction {
        void ( TestBriefingTickTarget::*member)(int);
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingTickTarget::SetVisible;
    return thunk.fn;
}

void ResetBriefingTickDispatchLog() {
    g_briefingTickDrawBaseCount = 0;
    g_briefingTickInvalidateCount = 0;
    g_briefingTickRebuildCount = 0;
    g_briefingTickUpdateBoundsCount = 0;
    g_briefingTickSetVisibleCount = 0;
    std::memset(g_briefingTickSetVisibleValue, 0, sizeof(g_briefingTickSetVisibleValue));
    std::memset(g_briefingTickSetVisibleThis, 0, sizeof(g_briefingTickSetVisibleThis));
    std::memset(g_briefingTickNoArgThis, 0, sizeof(g_briefingTickNoArgThis));
    g_briefingTickPanelSetTextThis = nullptr;
    std::memset(g_briefingTickPanelSetText, 0, sizeof(g_briefingTickPanelSetText));
}

int CallBriefingActionTick(void *action, float deltaSec) {
    typedef int( *TickFn)(void *self, float deltaSec);
    const unsigned int *const vtable = *reinterpret_cast<unsigned int *const *>(action);
    return ((TickFn)(vtable[0]))(action, deltaSec);
}

struct TestBriefingTransportProgress : HudUiBriefingTransportProgress {
    void SetNormalizedValue(float value) {
        ++g_setProgressCount;
        g_setProgressValue = value;
        g_setProgressThis = this;
    }
};

struct TestBriefingInvalidatedElement {
    unsigned int *vptr;

    void Invalidate() {
        if (g_invalidateCount < 8) {
            g_invalidateThis[g_invalidateCount] = this;
        }

        ++g_invalidateCount;
    }
};

struct TestBriefingVisibleElement {
    unsigned int *vptr;

    void SetVisible(int visible) {
        if (g_briefingSetVisibleCount < 16) {
            g_briefingSetVisibleThis[g_briefingSetVisibleCount] = this;
            g_briefingSetVisibleValue[g_briefingSetVisibleCount] = visible;
        }

        ++g_briefingSetVisibleCount;
    }
};

struct TestBriefingCompositePanelEntry : HudUiCompositePanelEntry {
    HudUiCompositePanelEntry * ScalarDeletingDtor(unsigned int flags) {
        if (g_briefingMessageEntryDtorCount < 4) {
            g_briefingMessageEntryDtorThis[g_briefingMessageEntryDtorCount] = this;
            g_briefingMessageEntryDtorFlags[g_briefingMessageEntryDtorCount] = flags;
        }

        ++g_briefingMessageEntryDtorCount;
        return this;
    }
};

struct TestBriefingRuntime : HudUiBriefingRuntime {
    HudUiBriefingRuntime * ScalarDeletingDtor(std::uint32_t flags) {
        ++g_deleteCount;
        g_deleteFlags = flags;
        g_deletedRuntime = this;
        return this;
    }
};

typedef HudUiBriefingRuntime *( *HudUiBriefingRuntimeScalarDeletingDestructor)(
    HudUiBriefingRuntime *self,
    unsigned int flags
);

HudUiBriefingRuntimeScalarDeletingDestructor MakeScalarDeletingDtorThunk() {
    union MemberToFunction {
        HudUiBriefingRuntime *( TestBriefingRuntime::*member)(std::uint32_t);
        HudUiBriefingRuntimeScalarDeletingDestructor fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingRuntime::ScalarDeletingDtor;
    return thunk.fn;
}

unsigned int MakeSetNormalizedValueThunk() {
    union MemberToFunction {
        void ( TestBriefingTransportProgress::*member)(float);
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingTransportProgress::SetNormalizedValue;
    return thunk.fn;
}

unsigned int MakeBriefingInvalidateThunk() {
    union MemberToFunction {
        void ( TestBriefingInvalidatedElement::*member)();
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingInvalidatedElement::Invalidate;
    return thunk.fn;
}

unsigned int MakeBriefingSetVisibleThunk() {
    union MemberToFunction {
        void ( TestBriefingVisibleElement::*member)(int);
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingVisibleElement::SetVisible;
    return thunk.fn;
}

unsigned int MakeBriefingCompositeEntryDtorThunk() {
    union MemberToFunction {
        HudUiCompositePanelEntry *( TestBriefingCompositePanelEntry::*member)(
            unsigned int);
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingCompositePanelEntry::ScalarDeletingDtor;
    return thunk.fn;
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

void PrepareConstructorGlobals(ConstructorGlobalState &state) {
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
    testSurface.pitch = static_cast<int>(sizeof(g_constructorSurfacePixel));
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

void RestoreConstructorGlobals(const ConstructorGlobalState &state) {
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
} // namespace

extern "C" int briefing_start_for_mission_smoke(void) {
    HudUiBriefingRuntime *const oldRuntime = g_Briefing_Runtime;
    zSndPlayHandle *const oldHandle = g_Briefing_CurrentSndHandle;
    const int oldThreadRunFlag = g_Briefing_ThreadRunFlag;
    const int oldThreadExitedFlag = g_Briefing_ThreadExitedFlag;
    const int oldAllowAdvanceFlag = g_Briefing_AllowAdvanceFlag;
    const int oldSequenceActiveFlag = g_Briefing_SequenceActiveFlag;
    const int oldSystemActiveFlag = g_Briefing_SystemActiveFlag;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    const zSndSampleSetRegistry oldSampleSetRegistry = g_zSnd_SampleSetRegistry;
    const int oldUseArchiveBanksFlag = g_zSnd_UseArchiveBanksFlag;
    char oldSndSetName[sizeof(g_Briefing_SndSetName)];
    std::memcpy(oldSndSetName, g_Briefing_SndSetName, sizeof(oldSndSetName));

    ConstructorGlobalState constructorState = {};
    PrepareConstructorGlobals(constructorState);

    char setName[] = "BRIEFING7";
    zSndSampleSet sampleSet = {};
    sampleSet.setName = setName;
    sampleSet.resourcesLoaded = 0;
    int networkEnabled = 1;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
    g_zSnd_SampleSetRegistry.clear();
    g_zSnd_SampleSetRegistry.push_back(&sampleSet);
    g_zSnd_UseArchiveBanksFlag = 0;
    g_zVideo_RendererType = 1;
    g_zVideo_pfnLockSurfaceState = TestVideoSurfaceDispatchDisableBriefingRuntime;
    g_zVideo_pfnAdjustSurfaces = TestAdjustSurfacesStopBriefingThread;
    g_Briefing_Runtime = nullptr;
    g_Briefing_CurrentSndHandle = nullptr;
    g_Briefing_ThreadRunFlag = 0;
    g_Briefing_ThreadExitedFlag = 1;
    g_Briefing_AllowAdvanceFlag = 0;
    g_Briefing_SequenceActiveFlag = 0;
    g_Briefing_SystemActiveFlag = 0;
    g_HudUi_InvalidateMask = 0;
    g_briefingAdjustSurfaceCalls = 0;
    g_briefingStopAfterAdjustCalls = 0;
    g_briefingStopRequested = 0;
    std::memset(g_Briefing_SndSetName, 0, sizeof(g_Briefing_SndSetName));

    const int result = Briefing::StartForMission(7);
    g_briefingStopRequested = 1;
    for (int attempt = 0; attempt < 100 && g_Briefing_ThreadExitedFlag == 0; ++attempt) {
        Sleep(10);
    }

    const bool started =
        result == 1 && std::strcmp(g_Briefing_SndSetName, setName) == 0 &&
        g_Briefing_Runtime != nullptr && g_Briefing_ThreadRunFlag == 0 &&
        g_Briefing_ThreadExitedFlag == 1 && g_Briefing_AllowAdvanceFlag == 1 &&
        g_briefingAdjustSurfaceCalls >= 1 && sampleSet.resourcesLoaded == 0 &&
        g_HudUi_InvalidateMask == 0x04u;

    HudUiBriefingRuntime *const allocatedRuntime = g_Briefing_Runtime;
    if (allocatedRuntime != nullptr && g_Briefing_ThreadExitedFlag != 0) {
        ::operator delete(allocatedRuntime);
        g_Briefing_Runtime = nullptr;
        g_Briefing_SystemActiveFlag = 0;
    }

    const bool stopped = g_Briefing_Runtime == nullptr && g_Briefing_SystemActiveFlag == 0;

    RestoreConstructorGlobals(constructorState);
    g_Briefing_Runtime = oldRuntime;
    g_Briefing_CurrentSndHandle = oldHandle;
    g_Briefing_ThreadRunFlag = oldThreadRunFlag;
    g_Briefing_ThreadExitedFlag = oldThreadExitedFlag;
    g_Briefing_AllowAdvanceFlag = oldAllowAdvanceFlag;
    g_Briefing_SequenceActiveFlag = oldSequenceActiveFlag;
    g_Briefing_SystemActiveFlag = oldSystemActiveFlag;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_zSnd_SampleSetRegistry = oldSampleSetRegistry;
    g_zSnd_UseArchiveBanksFlag = oldUseArchiveBanksFlag;
    std::memcpy(g_Briefing_SndSetName, oldSndSetName, sizeof(g_Briefing_SndSetName));

    return started && stopped ? 0 : 1;
}

extern "C" int briefing_thread_main_one_iteration_smoke(void) {
    HudUiBriefingRuntime *const oldRuntime = g_Briefing_Runtime;
    zSndPlayHandle *const oldHandle = g_Briefing_CurrentSndHandle;
    const int oldThreadRunFlag = g_Briefing_ThreadRunFlag;
    const int oldThreadExitedFlag = g_Briefing_ThreadExitedFlag;
    const int oldAllowAdvanceFlag = g_Briefing_AllowAdvanceFlag;
    const int oldSequenceActiveFlag = g_Briefing_SequenceActiveFlag;
    const int oldSystemActiveFlag = g_Briefing_SystemActiveFlag;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    int *const oldNetworkEnabled = g_zGame_Options_PointerCache.networkEnabled;
    const zSndSampleSetRegistry oldSampleSetRegistry = g_zSnd_SampleSetRegistry;
    char oldSndSetName[sizeof(g_Briefing_SndSetName)];
    std::memcpy(oldSndSetName, g_Briefing_SndSetName, sizeof(oldSndSetName));

    ConstructorGlobalState constructorState = {};
    PrepareConstructorGlobals(constructorState);

    alignas(4) static unsigned char runtimeStorage[0xba70];
    std::memset(runtimeStorage, 0, sizeof(runtimeStorage));
    HudUiBriefingRuntime *const runtime =
        reinterpret_cast<HudUiBriefingRuntime *>(runtimeStorage);

    char setName[] = "BRIEFING7";
    zSndSampleSet sampleSet = {};
    sampleSet.setName = setName;
    sampleSet.resourcesLoaded = 1;
    int networkEnabled = 1;
    g_zGame_Options_PointerCache.networkEnabled = &networkEnabled;
    g_zSnd_SampleSetRegistry.clear();
    g_zSnd_SampleSetRegistry.push_back(&sampleSet);
    std::strcpy(g_Briefing_SndSetName, setName);
    g_Briefing_Runtime = runtime;
    g_Briefing_CurrentSndHandle = nullptr;
    g_Briefing_ThreadRunFlag = 0;
    g_Briefing_ThreadExitedFlag = 0;
    g_Briefing_AllowAdvanceFlag = 0;
    g_Briefing_SequenceActiveFlag = 1;
    g_Briefing_SystemActiveFlag = 1;
    g_HudUi_InvalidateMask = 0;
    g_briefingAdjustSurfaceCalls = 0;
    g_briefingStopAfterAdjustCalls = 1;
    g_briefingStopRequested = 0;
    g_zVideo_pfnAdjustSurfaces = TestAdjustSurfacesStopBriefingThread;

    Briefing::ThreadMain(nullptr);

    const bool ok = g_Briefing_ThreadRunFlag == 0 && g_Briefing_ThreadExitedFlag == 1 &&
                    g_Briefing_AllowAdvanceFlag == 1 && g_Briefing_SequenceActiveFlag == 1 &&
                    g_briefingAdjustSurfaceCalls == 1 && sampleSet.resourcesLoaded == 0 &&
                    g_HudUi_InvalidateMask == 0x04u && g_zVideo_FrameTick == 1;

    RestoreConstructorGlobals(constructorState);
    g_Briefing_Runtime = oldRuntime;
    g_Briefing_CurrentSndHandle = oldHandle;
    g_Briefing_ThreadRunFlag = oldThreadRunFlag;
    g_Briefing_ThreadExitedFlag = oldThreadExitedFlag;
    g_Briefing_AllowAdvanceFlag = oldAllowAdvanceFlag;
    g_Briefing_SequenceActiveFlag = oldSequenceActiveFlag;
    g_Briefing_SystemActiveFlag = oldSystemActiveFlag;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_zGame_Options_PointerCache.networkEnabled = oldNetworkEnabled;
    g_zSnd_SampleSetRegistry = oldSampleSetRegistry;
    std::memcpy(g_Briefing_SndSetName, oldSndSetName, sizeof(g_Briefing_SndSetName));

    return ok ? 0 : 1;
}

extern "C" int briefing_set_progress_and_sleep_smoke(void) {
    HudUiBriefingRuntime *const oldRuntime = g_Briefing_Runtime;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    alignas(4) static unsigned char runtimeStorage[sizeof(HudUiBriefingRuntime)];
    HudUiBriefingRuntime *const runtime =
        reinterpret_cast<HudUiBriefingRuntime *>(runtimeStorage);
    static zVidImagePartial fillImage;

    std::memset(runtime, 0, sizeof(*runtime));
    std::memset(&fillImage, 0, sizeof(fillImage));
    fillImage.width = 120;
    fillImage.height = 16;
    runtime->transportProgress.fillImage = &fillImage;
    runtime->transportProgress.flags = 0;
    g_HudUi_InvalidateMask = 0x04;

    g_Briefing_Runtime = runtime;
    Briefing::SetProgressAndSleep(0.375f);
    const bool runtimeOk =
        runtime->transportProgress.normalizedValue == 0.375f &&
        runtime->transportProgress.fillRect.left == 0 &&
        runtime->transportProgress.fillRect.top == 0 &&
        runtime->transportProgress.fillRect.right == 45 &&
        runtime->transportProgress.fillRect.bottom == 16 &&
        (runtime->transportProgress.flags & 0x04u) != 0;

    g_Briefing_Runtime = nullptr;
    Briefing::SetProgressAndSleep(0.875f);
    const bool nullOk =
        runtime->transportProgress.normalizedValue == 0.375f &&
        runtime->transportProgress.fillRect.right == 45;

    g_Briefing_Runtime = oldRuntime;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    return runtimeOk && nullOk ? 0 : 1;
}
