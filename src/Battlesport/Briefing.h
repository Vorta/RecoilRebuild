#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

struct HudUiBriefingRuntime;

typedef void (RECOIL_THISCALL *HudUiBriefingRuntimeUpdate)(HudUiBriefingRuntime * self, float deltaSec);
typedef void (RECOIL_THISCALL *HudUiBriefingRuntimeSetEnabled)(HudUiBriefingRuntime * self, int enabled);
typedef HudUiBriefingRuntime * (RECOIL_THISCALL *HudUiBriefingRuntimeScalarDeletingDestructor)(HudUiBriefingRuntime * self, unsigned int flags);

struct HudUiBriefingRuntimeVtable {
    HudUiBriefingRuntimeUpdate Update;
    HudUiBriefingRuntimeSetEnabled SetEnabled;
    HudUiBriefingRuntimeScalarDeletingDestructor ScalarDeletingDtor;
};
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntimeVtable, ScalarDeletingDtor) == 0x08);

struct HudUiBriefingTransportProgress {
    const void *vptr;
};

struct HudUiBriefingRuntime {
    HudUiBriefingRuntimeVtable *vptr;
    unsigned char unknown_0004[0xa95c];
    HudUiBriefingTransportProgress transportProgress;

    RECOIL_NOINLINE HudUiBriefingRuntime *RECOIL_THISCALL Constructor(int missionId);
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    HudUiBriefingRuntime *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    RECOIL_NOINLINE int RECOIL_THISCALL BuildObjectiveActionsFromIndex(
        int objectiveIndex);
    RECOIL_NOINLINE void RECOIL_THISCALL Update(float deltaSec);
};
RECOIL_STATIC_ASSERT(offsetof(HudUiBriefingRuntime, transportProgress) == 0xa960);

namespace Briefing {
RECOIL_NOINLINE void RECOIL_FASTCALL BuildObjectiveActionsGlobal(int objectiveIndex);
RECOIL_NOINLINE void RECOIL_FASTCALL SampleEventCallback(int progressEventCode);
RECOIL_NOINLINE int RECOIL_FASTCALL StartForMission(int missionId);
RECOIL_NOINLINE void RECOIL_CDECL ThreadMain(void *threadParameter);
RECOIL_NOINLINE void RECOIL_FASTCALL StopAndShutdownThread(int waitForInput);
RECOIL_NOINLINE void RECOIL_STDCALL SetProgressAndSleep(float progressValue);
} // namespace Briefing

extern HudUiBriefingRuntime *g_Briefing_Runtime;

extern "C" {
extern int g_Briefing_ThreadRunFlag;
extern int g_Briefing_ThreadExitedFlag;
extern int g_Briefing_SequenceActiveFlag;
extern int g_Briefing_AllowAdvanceFlag;
extern int g_Briefing_SystemActiveFlag;
extern int g_Briefing_ProgressEventCode;
}
