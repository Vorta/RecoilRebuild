#include "Battlesport/Briefing.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdint>
#include <cstring>

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

int RECOIL_FASTCALL TestVideoSurfaceDispatch(zVideo_SurfaceStatePartial *) {
    return 0;
}

int RECOIL_FASTCALL TestAdjustSurfaces(zVidRect32 *, zVidRect32 *, int, int) {
    return 0;
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
