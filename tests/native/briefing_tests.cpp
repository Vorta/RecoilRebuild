#include "Battlesport/Briefing.h"
#include "Battlesport/HudSensorTracker.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

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

int RECOIL_FASTCALL TestVideoSurfaceDispatch(zVideo_SurfaceStatePartial *) {
    return 0;
}

int RECOIL_FASTCALL TestVideoSurfaceDispatchDisableBriefingRuntime(
    zVideo_SurfaceStatePartial *) {
    if (g_Briefing_Runtime != nullptr) {
        *reinterpret_cast<int *>(reinterpret_cast<unsigned char *>(g_Briefing_Runtime) + 4) =
            0;
    }

    return 0;
}

int RECOIL_FASTCALL TestAdjustSurfaces(zVidRect32 *, zVidRect32 *, int, int) {
    return 0;
}

int RECOIL_FASTCALL TestAdjustSurfacesStopBriefingThread(zVidRect32 *, zVidRect32 *, int,
                                                         int) {
    ++g_briefingAdjustSurfaceCalls;
    if (g_briefingStopRequested != 0 ||
        (g_briefingStopAfterAdjustCalls > 0 &&
         g_briefingAdjustSurfaceCalls >= g_briefingStopAfterAdjustCalls)) {
        g_Briefing_ThreadRunFlag = 0;
    }
    return 0;
}

void RECOIL_FASTCALL TestBriefingBltSourceToPrimary(void *self, int dstX, int dstY,
                                                    int clipFlags, void *srcRect) {
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

struct TestBriefingTransportProgress : HudUiBriefingTransportProgress {
    void RECOIL_THISCALL SetNormalizedValue(float value) {
        ++g_setProgressCount;
        g_setProgressValue = value;
        g_setProgressThis = this;
    }
};

struct TestBriefingInvalidatedElement {
    unsigned int *vptr;

    void RECOIL_THISCALL Invalidate() {
        if (g_invalidateCount < 8) {
            g_invalidateThis[g_invalidateCount] = this;
        }

        ++g_invalidateCount;
    }
};

struct TestBriefingVisibleElement {
    unsigned int *vptr;

    void RECOIL_THISCALL SetVisible(int visible) {
        if (g_briefingSetVisibleCount < 16) {
            g_briefingSetVisibleThis[g_briefingSetVisibleCount] = this;
            g_briefingSetVisibleValue[g_briefingSetVisibleCount] = visible;
        }

        ++g_briefingSetVisibleCount;
    }
};

struct TestBriefingRuntime : HudUiBriefingRuntime {
    HudUiBriefingRuntime *RECOIL_THISCALL ScalarDeletingDtor(std::uint32_t flags) {
        ++g_deleteCount;
        g_deleteFlags = flags;
        g_deletedRuntime = this;
        return this;
    }
};

HudUiBriefingRuntimeScalarDeletingDestructor MakeScalarDeletingDtorThunk() {
    union MemberToFunction {
        HudUiBriefingRuntime *(RECOIL_THISCALL TestBriefingRuntime::*member)(std::uint32_t);
        HudUiBriefingRuntimeScalarDeletingDestructor fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingRuntime::ScalarDeletingDtor;
    return thunk.fn;
}

unsigned int MakeSetNormalizedValueThunk() {
    union MemberToFunction {
        void (RECOIL_THISCALL TestBriefingTransportProgress::*member)(float);
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingTransportProgress::SetNormalizedValue;
    return thunk.fn;
}

unsigned int MakeBriefingInvalidateThunk() {
    union MemberToFunction {
        void (RECOIL_THISCALL TestBriefingInvalidatedElement::*member)();
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingInvalidatedElement::Invalidate;
    return thunk.fn;
}

unsigned int MakeBriefingSetVisibleThunk() {
    union MemberToFunction {
        void (RECOIL_THISCALL TestBriefingVisibleElement::*member)(int);
        unsigned int fn;
    };

    MemberToFunction thunk{};
    thunk.member = &TestBriefingVisibleElement::SetVisible;
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
    int fadeActiveCount;
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
    g_zSndFadeActiveListCount = 0;
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
    g_zSndFadeActiveListCount = state.fadeActiveCount;
    g_FrameDeltaTimeSec = state.frameDeltaTimeSec;
    g_Time_CurrentTimeSec = state.timeCurrentSec;
    g_Time_NewTimeSec = state.timeNewSec;
    g_Time_AccumulatedTimeSec = state.timeAccumulatedSec;
    g_Time_UnscaledDeltaTimeSec = state.timeUnscaledDeltaSec;
    g_Time_UnscaledAccumulatedTimeSec = state.timeUnscaledAccumulatedSec;
    g_Briefing_ProgressEventCode = state.progressEventCode;
}
} // namespace

extern "C" int briefing_stop_and_shutdown_thread_smoke(void) {
    static TestBriefingRuntime runtime;
    static HudUiBriefingRuntimeVtable vtable;

    g_deleteCount = 0;
    g_deleteFlags = 0;
    g_deletedRuntime = nullptr;
    vtable.ScalarDeletingDtor = MakeScalarDeletingDtorThunk();
    runtime.vptr = &vtable;

    g_Briefing_Runtime = &runtime;
    g_Briefing_ThreadRunFlag = 1;
    g_Briefing_ThreadExitedFlag = 1;
    g_Briefing_SequenceActiveFlag = 1;
    g_Briefing_AllowAdvanceFlag = 1;
    g_Briefing_SystemActiveFlag = 1;

    Briefing::StopAndShutdownThread(0);
    if (g_Briefing_ThreadRunFlag != 0 || g_Briefing_SystemActiveFlag != 0 ||
        g_Briefing_Runtime != nullptr || g_deleteCount != 1 || g_deleteFlags != 1 ||
        g_deletedRuntime != &runtime) {
        return 1;
    }

    g_Briefing_ThreadRunFlag = 1;
    g_Briefing_ThreadExitedFlag = 1;
    g_Briefing_SequenceActiveFlag = 0;
    g_Briefing_AllowAdvanceFlag = 0;
    g_Briefing_SystemActiveFlag = 1;
    Briefing::StopAndShutdownThread(1);

    return g_Briefing_ThreadRunFlag == 0 && g_Briefing_SystemActiveFlag == 0 &&
                   g_Briefing_Runtime == nullptr && g_deleteCount == 1
               ? 0
               : 2;
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
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
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
    zSndSampleSet *sampleSetSlots[1] = {&sampleSet};

    int networkEnabled = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zSnd_SampleSetRegistry.begin = sampleSetSlots;
    g_zSnd_SampleSetRegistry.end = sampleSetSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSetSlots + 1;
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
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zSnd_SampleSetRegistry = oldSampleSetRegistry;
    std::memcpy(g_Briefing_SndSetName, oldSndSetName, sizeof(g_Briefing_SndSetName));

    return ok ? 0 : 1;
}

extern "C" int briefing_start_for_mission_smoke(void) {
    HudUiBriefingRuntime *const oldRuntime = g_Briefing_Runtime;
    zSndPlayHandle *const oldHandle = g_Briefing_CurrentSndHandle;
    const int oldThreadRunFlag = g_Briefing_ThreadRunFlag;
    const int oldThreadExitedFlag = g_Briefing_ThreadExitedFlag;
    const int oldAllowAdvanceFlag = g_Briefing_AllowAdvanceFlag;
    const int oldSequenceActiveFlag = g_Briefing_SequenceActiveFlag;
    const int oldSystemActiveFlag = g_Briefing_SystemActiveFlag;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const zSndSampleSetRegistry oldSampleSetRegistry = g_zSnd_SampleSetRegistry;
    char oldSndSetName[sizeof(g_Briefing_SndSetName)];
    std::memcpy(oldSndSetName, g_Briefing_SndSetName, sizeof(oldSndSetName));

    ConstructorGlobalState constructorState = {};
    PrepareConstructorGlobals(constructorState);

    char setName[] = "BRIEFING7";
    zSndSampleSet sampleSet = {};
    sampleSet.setName = setName;
    sampleSet.resourcesLoaded = 0;
    zSndSampleSet *sampleSetSlots[1] = {&sampleSet};

    int networkEnabled = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zSnd_SampleSetRegistry.begin = sampleSetSlots;
    g_zSnd_SampleSetRegistry.end = sampleSetSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSetSlots + 1;
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
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zSnd_SampleSetRegistry = oldSampleSetRegistry;
    std::memcpy(g_Briefing_SndSetName, oldSndSetName, sizeof(g_Briefing_SndSetName));

    return started && stopped ? 0 : 1;
}

extern "C" int briefing_set_progress_and_sleep_smoke(void) {
    HudUiBriefingRuntime *const oldRuntime = g_Briefing_Runtime;
    static TestBriefingRuntime runtime;
    static unsigned int transportProgressFtable[34];

    std::memset(&runtime, 0, sizeof(runtime));
    std::memset(transportProgressFtable, 0, sizeof(transportProgressFtable));
    transportProgressFtable[0x84 / 4] = MakeSetNormalizedValueThunk();
    runtime.transportProgress.vptr = transportProgressFtable;
    g_setProgressCount = 0;
    g_setProgressValue = 0.0f;
    g_setProgressThis = nullptr;

    g_Briefing_Runtime = &runtime;
    Briefing::SetProgressAndSleep(0.375f);
    const bool runtimeOk =
        g_setProgressCount == 1 && g_setProgressValue == 0.375f &&
        g_setProgressThis == &runtime.transportProgress;

    g_Briefing_Runtime = nullptr;
    Briefing::SetProgressAndSleep(0.875f);
    const bool nullOk = g_setProgressCount == 1;

    g_Briefing_Runtime = oldRuntime;
    return runtimeOk && nullOk ? 0 : 1;
}

extern "C" int briefing_runtime_constructor_smoke(void) {
    constexpr std::size_t kRuntimeSize = 0xba70;
    constexpr std::size_t kActionQueueMissionOffset = 0xa94c;
    constexpr std::size_t kActionQueueHeadOffset = 0xa950;
    constexpr std::size_t kActionQueueCountOffset = 0xa954;
    constexpr std::size_t kActionQueueActiveOffset = 0xa95c;
    constexpr std::size_t kTransportProgressOffset = 0xa960;
    constexpr std::size_t kObjectivePictureOffset = 0xb2d4;
    constexpr std::size_t kObjectivePictureNoiseAlphaOffset = kObjectivePictureOffset + 0xbc;
    constexpr int kMissionId = 7;

    struct ActionNode {
        ActionNode *prev;
        ActionNode *next;
        void *action;
    };

    ConstructorGlobalState state = {};
    PrepareConstructorGlobals(state);

    alignas(4) static unsigned char storage[kRuntimeSize];
    std::memset(storage, 0, sizeof(storage));
    HudUiBriefingRuntime *const runtime = reinterpret_cast<HudUiBriefingRuntime *>(storage);
    HudUiBriefingRuntime *const result = runtime->Constructor(kMissionId);

    int failure = 0;
    const ActionNode *const sentinel =
        *reinterpret_cast<ActionNode **>(storage + kActionQueueHeadOffset);
    const float objectiveNoiseAlpha =
        *reinterpret_cast<float *>(storage + kObjectivePictureNoiseAlphaOffset);

    if (result != runtime) {
        failure = result == 0 ? 8 : 9;
    } else if (runtime->vptr == 0) {
        failure = 7;
    }

    if (failure != 0) {
        RestoreConstructorGlobals(state);
        return failure;
    }

    if (*reinterpret_cast<unsigned char *>(storage + kActionQueueMissionOffset) !=
        static_cast<unsigned char>(kMissionId)) {
        failure = 2;
    } else if (sentinel == 0 || sentinel->prev != sentinel || sentinel->next != sentinel) {
        failure = 3;
    } else if (*reinterpret_cast<int *>(storage + kActionQueueCountOffset) != 0 ||
               *reinterpret_cast<int *>(storage + kActionQueueActiveOffset) != 0) {
        failure = 4;
    } else if (g_Briefing_ProgressEventCode != -1) {
        failure = 5;
    } else if (*reinterpret_cast<void **>(storage + kTransportProgressOffset) == 0 ||
               objectiveNoiseAlpha != 0.0f) {
        failure = 6;
    }

    // Destructor coverage is tracked separately; this smoke only exercises the constructor.
    RestoreConstructorGlobals(state);
    return failure;
}

extern "C" int briefing_locator_panel_constructor_smoke(void) {
    constexpr std::size_t kRuntimeSize = 0xba70;
    constexpr std::size_t kLocatorPanelsOffset = 0xb8f0;
    constexpr std::size_t kLocatorPanelStride = 0x40;

    ConstructorGlobalState state = {};
    PrepareConstructorGlobals(state);

    alignas(4) static unsigned char storage[kRuntimeSize];
    std::memset(storage, 0, sizeof(storage));
    HudUiBriefingRuntime *const runtime = reinterpret_cast<HudUiBriefingRuntime *>(storage);
    runtime->Constructor(2);

    const unsigned int expectedColor =
        static_cast<unsigned short>(zVid_PackColorRGB(0xff, 0, 0));
    const HudUiCommon_FTable *locatorFTable = nullptr;
    bool ok = true;
    for (std::size_t index = 0; index < 6; ++index) {
        auto *const locator =
            reinterpret_cast<HudUiCircle *>(storage + kLocatorPanelsOffset +
                                            index * kLocatorPanelStride);
        if (index == 0) {
            locatorFTable = locator->base.ftable;
            ok = ok && locatorFTable != nullptr && locatorFTable != &g_HudUiCircle_FTable;
        }

        ok = ok && locator->base.ftable == locatorFTable && locator->base.x == 100 &&
             locator->base.y == 110 && (locator->base.flags & 0x10u) != 0 &&
             locator->radius == 30 && locator->radiusSquared == 900 &&
             locator->color565 == expectedColor;
    }

    RestoreConstructorGlobals(state);
    return ok ? 0 : 1;
}

extern "C" int briefing_locator_panel_blit_dirty_rect_smoke(void) {
    constexpr std::size_t kRuntimeSize = 0xba70;
    constexpr std::size_t kLocatorPanelsOffset = 0xb8f0;

    zVideo_BltSourceToPrimaryProc oldBlit = g_zVideo_pfnBltSourceToPrimary;
    ConstructorGlobalState state = {};
    PrepareConstructorGlobals(state);

    alignas(4) static unsigned char storage[kRuntimeSize];
    std::memset(storage, 0, sizeof(storage));
    HudUiBriefingRuntime *const runtime = reinterpret_cast<HudUiBriefingRuntime *>(storage);
    runtime->Constructor(3);

    auto *const locator = reinterpret_cast<HudUiCircle *>(storage + kLocatorPanelsOffset);
    typedef void (RECOIL_THISCALL *DrawBaseFn)(HudUiCircle * self);
    DrawBaseFn const drawBase = reinterpret_cast<DrawBaseFn>(locator->base.ftable->slots[2]);

    g_zVideo_pfnBltSourceToPrimary = TestBriefingBltSourceToPrimary;
    g_briefingBlitCount = 0;
    g_briefingBlitImage = nullptr;
    locator->base.bltSource = nullptr;
    drawBase(locator);
    const bool nullSkipped = g_briefingBlitCount == 0 && g_briefingBlitImage == nullptr;

    zVidImagePartial image{};
    locator->base.bltSource = &image;
    locator->base.clipRect.left = 4;
    locator->base.clipRect.top = 5;
    locator->base.clipRect.right = 24;
    locator->base.clipRect.bottom = 25;
    g_briefingBlitCount = 0;
    g_briefingBlitImage = nullptr;
    drawBase(locator);

    const bool blitted = g_briefingBlitCount == 1 && g_briefingBlitImage == &image &&
                         g_briefingBlitX == 4 && g_briefingBlitY == 5 &&
                         g_briefingBlitFlags == 0 && g_briefingBlitHasRect != 0 &&
                         g_briefingBlitRect.left == 4 && g_briefingBlitRect.top == 5 &&
                         g_briefingBlitRect.right == 24 && g_briefingBlitRect.bottom == 25;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    RestoreConstructorGlobals(state);
    return nullSkipped && blitted ? 0 : 1;
}

extern "C" int briefing_locator_panel_update_smoke(void) {
    constexpr std::size_t kRuntimeSize = 0xba70;
    constexpr std::size_t kLocatorPanelsOffset = 0xb8f0;

    ConstructorGlobalState state = {};
    PrepareConstructorGlobals(state);

    alignas(4) static unsigned char storage[kRuntimeSize];
    std::memset(storage, 0, sizeof(storage));
    HudUiBriefingRuntime *const runtime = reinterpret_cast<HudUiBriefingRuntime *>(storage);
    runtime->Constructor(3);

    auto *const locator = reinterpret_cast<HudUiCircle *>(storage + kLocatorPanelsOffset);
    typedef void (RECOIL_THISCALL *UpdateFn)(HudUiCircle * self, float deltaSec);
    UpdateFn const update = reinterpret_cast<UpdateFn>(locator->base.ftable->slots[9]);

    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;

    locator->base.flags = 0;
    locator->base.clipRect.left = 1;
    locator->base.clipRect.top = 2;
    locator->base.clipRect.right = 3;
    locator->base.clipRect.bottom = 4;
    locator->radius = 12;
    locator->radiusSquared = 144;
    g_HudUi_InvalidateMask = 0x80;
    update(locator, 1.0f);
    const bool visibleSkipped =
        locator->base.flags == 0 && locator->base.clipRect.left == 1 &&
        locator->base.clipRect.top == 2 && locator->base.clipRect.right == 3 &&
        locator->base.clipRect.bottom == 4 && locator->radius == 12 &&
        locator->radiusSquared == 144;

    locator->base.flags = 0x10 | 0x02 | 0x08;
    locator->base.x = 100;
    locator->base.y = 110;
    locator->base.bltSource = nullptr;
    locator->radius = 30;
    locator->radiusSquared = 900;
    update(locator, 0.25f);
    const bool clipAndShrink =
        locator->base.clipRect.left == 70 && locator->base.clipRect.top == 80 &&
        locator->base.clipRect.right == 131 && locator->base.clipRect.bottom == 141 &&
        locator->radius == 25 && locator->radiusSquared == 625;
    const bool baseUpdateAndInvalidate =
        (locator->base.flags & 0x08) == 0 && (locator->base.flags & 0x80) != 0;

    locator->base.flags = 0x10;
    locator->base.x = 20;
    locator->base.y = 25;
    locator->radius = 10;
    locator->radiusSquared = 100;
    g_HudUi_InvalidateMask = 0;
    update(locator, 0.01f);
    const bool minimumStep =
        locator->base.clipRect.left == 10 && locator->base.clipRect.top == 15 &&
        locator->base.clipRect.right == 31 && locator->base.clipRect.bottom == 36 &&
        locator->radius == 9 && locator->radiusSquared == 81;

    locator->base.flags = 0x10;
    locator->radius = 4;
    locator->radiusSquared = 16;
    update(locator, 1.0f);
    const bool clamped = locator->radius == 3 && locator->radiusSquared == 9;

    g_HudUi_InvalidateMask = oldInvalidateMask;
    RestoreConstructorGlobals(state);
    return visibleSkipped && clipAndShrink && baseUpdateAndInvalidate && minimumStep &&
                   clamped
               ? 0
               : 1;
}

extern "C" int briefing_runtime_update_smoke(void) {
    constexpr std::size_t kRuntimeSize = 0xba70;
    constexpr std::size_t kActionQueueHeadOffset = 0xa950;
    constexpr std::size_t kActionQueueCurrentOffset = 0xa958;
    constexpr std::size_t kActionQueueActiveOffset = 0xa95c;
    constexpr std::size_t kTransportProgressOffset = 0xa960;
    constexpr std::size_t kMissionNameOffset = 0xaae8;
    constexpr std::size_t kObjectiveSummaryOffset = 0xad8c;
    constexpr std::size_t kObjectiveDescOffset = 0xb030;
    constexpr std::size_t kObjectivePictureOffset = 0xb2d4;
    constexpr std::size_t kTransmissionHaltedOffset = 0xb394;

    struct ActionNode {
        ActionNode *prev;
        ActionNode *next;
        void *action;
    };

    alignas(4) unsigned char storage[kRuntimeSize] = {};
    HudUiBriefingRuntime *const runtime = reinterpret_cast<HudUiBriefingRuntime *>(storage);

    ActionNode sentinel = {};
    sentinel.prev = &sentinel;
    sentinel.next = &sentinel;
    *reinterpret_cast<ActionNode **>(storage + kActionQueueHeadOffset) = &sentinel;
    *reinterpret_cast<ActionNode **>(storage + kActionQueueCurrentOffset) = &sentinel;
    *reinterpret_cast<int *>(storage + kActionQueueActiveOffset) = 1;

    static unsigned int invalidateTable[34];
    std::memset(invalidateTable, 0, sizeof(invalidateTable));
    invalidateTable[0x20 / 4] = MakeBriefingInvalidateThunk();

    const std::size_t invalidateOffsets[6] = {
        kObjectivePictureOffset,       kTransmissionHaltedOffset, kMissionNameOffset,
        kTransportProgressOffset,      kObjectiveSummaryOffset,   kObjectiveDescOffset,
    };
    for (std::size_t index = 0; index < 6; ++index) {
        *reinterpret_cast<unsigned int **>(storage + invalidateOffsets[index]) = invalidateTable;
    }

    g_invalidateCount = 0;
    std::memset(g_invalidateThis, 0, sizeof(g_invalidateThis));
    g_Briefing_AllowAdvanceFlag = 1;

    runtime->Update(0.125f);

    bool invalidatedInOrder = g_invalidateCount == 6;
    for (std::size_t index = 0; index < 6; ++index) {
        invalidatedInOrder =
            invalidatedInOrder && g_invalidateThis[index] == storage + invalidateOffsets[index];
    }

    return g_Briefing_AllowAdvanceFlag == 0 && invalidatedInOrder ? 0 : 1;
}

extern "C" int briefing_objective_picture_draw_noise_overlay_smoke(void) {
    constexpr std::size_t kRuntimeSize = 0xba70;
    constexpr std::size_t kObjectivePictureOffset = 0xb2d4;
    constexpr std::size_t kObjectivePictureNoiseAlphaOffset = kObjectivePictureOffset + 0xbc;

    ConstructorGlobalState state = {};
    PrepareConstructorGlobals(state);

    zVideo_BltSourceToPrimaryProc oldBlit = g_zVideo_pfnBltSourceToPrimary;
    zVidImagePartial *oldExclusiveImage = g_HudUiWidget_ExclusiveDrawImage;
    unsigned char *const oldNoiseTable = g_zVid_NoiseByteTable;
    const int oldNoiseTableSize = g_zVid_NoiseByteTableSize;
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;

    alignas(4) static unsigned char storage[kRuntimeSize];
    std::memset(storage, 0, sizeof(storage));
    HudUiBriefingRuntime *const runtime = reinterpret_cast<HudUiBriefingRuntime *>(storage);
    runtime->Constructor(1);

    HudUiWidget *const picture =
        reinterpret_cast<HudUiWidget *>(storage + kObjectivePictureOffset);
    float *const noiseAlpha =
        reinterpret_cast<float *>(storage + kObjectivePictureNoiseAlphaOffset);
    typedef void (RECOIL_THISCALL *DrawFn)(HudUiWidget * self);
    DrawFn const draw = reinterpret_cast<DrawFn>(picture->ftable->slots[1]);

    zVidImagePartial image{};
    image.width = 3;
    image.height = 2;
    picture->image = &image;
    picture->x = 2;
    picture->y = 1;
    picture->dirtyRectCount = 0;
    picture->bltClipRectOrNull = nullptr;

    unsigned char noiseTable[32] = {};
    for (int i = 0; i < 32; ++i) {
        noiseTable[i] = static_cast<unsigned char>(i);
    }

    unsigned short pixels[64] = {};
    for (int i = 0; i < 64; ++i) {
        pixels[i] = 0xaaaa;
    }

    g_zVideo_pfnBltSourceToPrimary = TestBriefingBltSourceToPrimary;
    g_HudUiWidget_ExclusiveDrawImage = nullptr;
    g_zVid_NoiseByteTable = noiseTable;
    g_zVid_NoiseByteTableSize = 32;
    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 8;
    g_zVideo_FxSurfaceHeight = 8;
    g_zVideo_FxSurfacePitchBytes = 16;
    g_zVideo_FxSurfacePitchPixels16 = 8;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    g_briefingBlitCount = 0;
    g_briefingBlitImage = nullptr;
    *noiseAlpha = 0.0f;
    draw(picture);

    bool lowAlphaOk = g_briefingBlitCount == 1 && g_briefingBlitImage == &image &&
                      g_briefingBlitX == 2 && g_briefingBlitY == 1 &&
                      g_briefingBlitFlags == 0 && g_briefingBlitHasRect == 0;
    for (int i = 0; i < 64; ++i) {
        lowAlphaOk = lowAlphaOk && pixels[i] == 0xaaaa;
    }

    for (int i = 0; i < 64; ++i) {
        pixels[i] = 0xaaaa;
    }

    std::srand(11);
    const int rowWidth = image.width;
    const int firstOffset = (std::rand() * (g_zVid_NoiseByteTableSize - rowWidth)) / 0x7fff;
    const int secondOffset = (std::rand() * (g_zVid_NoiseByteTableSize - rowWidth)) / 0x7fff;
    std::srand(11);
    g_briefingBlitCount = 0;
    *noiseAlpha = 1.0f;
    draw(picture);

    const auto gray565 = [](unsigned char value) -> unsigned short {
        const unsigned short level = static_cast<unsigned short>(value & 0x1f);
        return static_cast<unsigned short>((level << 11) | (level << 6) | level);
    };

    const bool noiseOk =
        g_briefingBlitCount == 1 && g_briefingBlitImage == &image &&
        pixels[2 + 1 * 8] == gray565(noiseTable[firstOffset]) &&
        pixels[3 + 1 * 8] == gray565(noiseTable[firstOffset + 1]) &&
        pixels[4 + 1 * 8] == gray565(noiseTable[firstOffset + 2]) &&
        pixels[2 + 2 * 8] == gray565(noiseTable[secondOffset]) &&
        pixels[3 + 2 * 8] == gray565(noiseTable[secondOffset + 1]) &&
        pixels[4 + 2 * 8] == gray565(noiseTable[secondOffset + 2]) &&
        pixels[0] == 0xaaaa && pixels[1 + 1 * 8] == 0xaaaa &&
        pixels[5 + 1 * 8] == 0xaaaa && pixels[2 + 3 * 8] == 0xaaaa;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    g_HudUiWidget_ExclusiveDrawImage = oldExclusiveImage;
    g_zVid_NoiseByteTable = oldNoiseTable;
    g_zVid_NoiseByteTableSize = oldNoiseTableSize;
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    RestoreConstructorGlobals(state);

    return lowAlphaOk && noiseOk ? 0 : 1;
}

extern "C" int briefing_build_objective_actions_smoke(void) {
    constexpr std::size_t kRuntimeSize = 0xba70;
    constexpr std::size_t kActionQueueHeadOffset = 0xa950;
    constexpr std::size_t kActionQueueCountOffset = 0xa954;
    constexpr std::size_t kActionQueueCurrentOffset = 0xa958;
    constexpr std::size_t kActionQueueActiveOffset = 0xa95c;
    constexpr std::size_t kMissionNameOffset = 0xaae8;
    constexpr std::size_t kObjectiveSummaryOffset = 0xad8c;
    constexpr std::size_t kObjectiveDescOffset = 0xb030;
    constexpr std::size_t kObjectivePictureOffset = 0xb2d4;
    constexpr std::size_t kLocatorPanelsOffset = 0xb8f0;
    constexpr std::size_t kLocatorPanelStride = 0x40;

    struct ActionNode {
        ActionNode *prev;
        ActionNode *next;
        void *action;
    };
    struct PlayAction {
        void *vptr;
        char sampleName[0x50];
        float gain;
        int useVariant;
        int variantIndex;
    };
    struct DelayAction {
        void *vptr;
        float requiredProgress;
    };
    struct PanelTextAction {
        void *vptr;
        char text[0x100];
        void *target;
    };
    struct ImageTimedAction {
        void *vptr;
        zVidImagePartial *imageRef;
        void *target;
        float timer;
    };
    struct ElementAction {
        void *vptr;
        void *target;
    };
    struct FadeAction {
        void *vptr;
        void *target;
        float alpha;
    };

    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const int oldMissionId = g_HudSensorTracker.missionId;
    const int oldObjectiveCount = g_HudSensorTracker.objectiveCount;
    HudSensorObjectiveSlot oldSlots[3] = {};
    for (int index = 0; index < 3; ++index) {
        oldSlots[index] = g_HudSensorTracker.objectiveSlots[index];
    }
    HMODULE const oldMessagesDll = g_zLoc_MessagesDllHandle;

    HMODULE messagesDll = LoadLibraryA("support\\messages.dll");
    if (messagesDll == nullptr) {
        messagesDll = LoadLibraryA("..\\..\\..\\..\\support\\messages.dll");
    }
    if (messagesDll == nullptr) {
        return 3;
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

    char expectedTitle1[0x20] = {};
    char expectedTitle2[0x20] = {};
    if (zLoc::FormatMessage(expectedTitle1, sizeof(expectedTitle1), 0x244, 2) == 0 ||
        zLoc::FormatMessage(expectedTitle2, sizeof(expectedTitle2), 0x244, 3) == 0) {
        ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
        g_HudSensorTracker.missionId = oldMissionId;
        g_HudSensorTracker.objectiveCount = oldObjectiveCount;
        for (int index = 0; index < 3; ++index) {
            g_HudSensorTracker.objectiveSlots[index] = oldSlots[index];
        }
        g_zLoc_MessagesDllHandle = oldMessagesDll;
        FreeLibrary(messagesDll);
        return 4;
    }

    alignas(4) unsigned char storage[kRuntimeSize] = {};
    HudUiBriefingRuntime *const runtime = reinterpret_cast<HudUiBriefingRuntime *>(storage);
    ActionNode sentinel = {};
    sentinel.prev = &sentinel;
    sentinel.next = &sentinel;
    *reinterpret_cast<ActionNode **>(storage + kActionQueueHeadOffset) = &sentinel;
    *reinterpret_cast<int *>(storage + kActionQueueCountOffset) = 0;
    *reinterpret_cast<int *>(storage + kActionQueueActiveOffset) = 0;

    static unsigned int visibleTable[25];
    std::memset(visibleTable, 0, sizeof(visibleTable));
    visibleTable[0x60 / 4] = MakeBriefingSetVisibleThunk();
    *reinterpret_cast<unsigned int **>(storage + kMissionNameOffset) = visibleTable;
    *reinterpret_cast<unsigned int **>(storage + kObjectiveSummaryOffset) = visibleTable;
    *reinterpret_cast<unsigned int **>(storage + kObjectiveDescOffset) = visibleTable;
    *reinterpret_cast<unsigned int **>(storage + kObjectivePictureOffset) = visibleTable;

    g_briefingSetVisibleCount = 0;
    std::memset(g_briefingSetVisibleThis, 0, sizeof(g_briefingSetVisibleThis));
    std::memset(g_briefingSetVisibleValue, 0, sizeof(g_briefingSetVisibleValue));
    g_Briefing_SequenceActiveFlag = 0;

    const int result = runtime->BuildObjectiveActionsFromIndex(1);

    void *actions[25] = {};
    ActionNode *cursor = sentinel.prev;
    int actionCount = 0;
    while (cursor != &sentinel && actionCount < 25) {
        actions[actionCount++] = cursor->action;
        cursor = cursor->prev;
    }

    const unsigned char *const missionName = storage + kMissionNameOffset;
    const unsigned char *const summaryPanel = storage + kObjectiveSummaryOffset;
    const unsigned char *const descPanel = storage + kObjectiveDescOffset;
    const unsigned char *const picture = storage + kObjectivePictureOffset;
    const unsigned char *const locator1 = storage + kLocatorPanelsOffset + kLocatorPanelStride;
    const unsigned char *const locator2 =
        storage + kLocatorPanelsOffset + kLocatorPanelStride * 2;

    bool ok = result == 1 && actionCount == 25 &&
              *reinterpret_cast<int *>(storage + kActionQueueCountOffset) == 25 &&
              *reinterpret_cast<int *>(storage + kActionQueueActiveOffset) == 1 &&
              g_Briefing_SequenceActiveFlag == 1 &&
              *reinterpret_cast<ActionNode **>(storage + kActionQueueCurrentOffset) ==
                  sentinel.prev &&
              g_briefingSetVisibleCount == 8;

    const PlayAction *const play = static_cast<const PlayAction *>(actions[0]);
    ok = ok && std::strcmp(play->sampleName, "snd_briefing_c5") == 0 && play->gain == 1.0f &&
         play->useVariant == 1 && play->variantIndex == 2;

    const DelayAction *const delay0 = static_cast<const DelayAction *>(actions[1]);
    const PanelTextAction *const title1 = static_cast<const PanelTextAction *>(actions[2]);
    const PanelTextAction *const summary1 = static_cast<const PanelTextAction *>(actions[3]);
    const ImageTimedAction *const imageAction1 = static_cast<const ImageTimedAction *>(actions[4]);
    const ElementAction *const show1 = static_cast<const ElementAction *>(actions[5]);
    const PanelTextAction *const desc1 = static_cast<const PanelTextAction *>(actions[6]);
    const DelayAction *const delay1 = static_cast<const DelayAction *>(actions[7]);
    const ElementAction *const hideLocator1 = static_cast<const ElementAction *>(actions[8]);
    const ElementAction *const hideMission1 = static_cast<const ElementAction *>(actions[9]);
    const ElementAction *const hideSummary1 = static_cast<const ElementAction *>(actions[10]);
    const FadeAction *const fade1 = static_cast<const FadeAction *>(actions[11]);
    const ElementAction *const hideDesc1 = static_cast<const ElementAction *>(actions[12]);

    ok = ok && delay0->requiredProgress == 2.0f && title1->target == missionName &&
         std::strcmp(title1->text, expectedTitle1) == 0 && summary1->target == summaryPanel &&
         std::strcmp(summary1->text, "summary one") == 0 &&
         imageAction1->imageRef == &image1 && imageAction1->target == picture &&
         imageAction1->timer == 1.0f && show1->target == locator1 && desc1->target == descPanel &&
         std::strcmp(desc1->text, "description one") == 0 &&
         delay1->requiredProgress == 3.0f && hideLocator1->target == locator1 &&
         hideMission1->target == missionName && hideSummary1->target == summaryPanel &&
         fade1->target == picture && fade1->alpha == 0.0f && hideDesc1->target == descPanel;

    const DelayAction *const delay2 = static_cast<const DelayAction *>(actions[13]);
    const PanelTextAction *const title2 = static_cast<const PanelTextAction *>(actions[14]);
    const PanelTextAction *const summary2 = static_cast<const PanelTextAction *>(actions[15]);
    const ImageTimedAction *const imageAction2 = static_cast<const ImageTimedAction *>(actions[16]);
    const ElementAction *const show2 = static_cast<const ElementAction *>(actions[17]);
    const PanelTextAction *const desc2 = static_cast<const PanelTextAction *>(actions[18]);
    const DelayAction *const delay3 = static_cast<const DelayAction *>(actions[19]);
    const ElementAction *const hideLocator2 = static_cast<const ElementAction *>(actions[20]);
    const ElementAction *const hideMission2 = static_cast<const ElementAction *>(actions[21]);
    const ElementAction *const hideSummary2 = static_cast<const ElementAction *>(actions[22]);
    const FadeAction *const fade2 = static_cast<const FadeAction *>(actions[23]);
    const ElementAction *const hideDesc2 = static_cast<const ElementAction *>(actions[24]);

    ok = ok && delay2->requiredProgress == 4.0f && title2->target == missionName &&
         std::strcmp(title2->text, expectedTitle2) == 0 && summary2->target == summaryPanel &&
         std::strcmp(summary2->text, "summary two") == 0 &&
         imageAction2->imageRef == &image2 && imageAction2->target == picture &&
         imageAction2->timer == 1.0f && show2->target == locator2 && desc2->target == descPanel &&
         std::strcmp(desc2->text, "description two") == 0 &&
         delay3->requiredProgress == 5.0f && hideLocator2->target == locator2 &&
         hideMission2->target == missionName && hideSummary2->target == summaryPanel &&
         fade2->target == picture && fade2->alpha == 0.0f && hideDesc2->target == descPanel;

    networkEnabled = 1;
    alignas(4) unsigned char networkStorage[kRuntimeSize] = {};
    HudUiBriefingRuntime *const networkRuntime =
        reinterpret_cast<HudUiBriefingRuntime *>(networkStorage);
    ActionNode networkSentinel = {};
    networkSentinel.prev = &networkSentinel;
    networkSentinel.next = &networkSentinel;
    *reinterpret_cast<ActionNode **>(networkStorage + kActionQueueHeadOffset) = &networkSentinel;
    ok = ok && networkRuntime->BuildObjectiveActionsFromIndex(1) == 0 &&
         *reinterpret_cast<int *>(networkStorage + kActionQueueCountOffset) == 0;

    cursor = sentinel.next;
    while (cursor != &sentinel) {
        ActionNode *const next = cursor->next;
        ::operator delete(cursor->action);
        ::operator delete(cursor);
        cursor = next;
    }

    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_HudSensorTracker.missionId = oldMissionId;
    g_HudSensorTracker.objectiveCount = oldObjectiveCount;
    for (int index = 0; index < 3; ++index) {
        g_HudSensorTracker.objectiveSlots[index] = oldSlots[index];
    }
    g_zLoc_MessagesDllHandle = oldMessagesDll;
    FreeLibrary(messagesDll);

    return ok ? 0 : 1;
}
