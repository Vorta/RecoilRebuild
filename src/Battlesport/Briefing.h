#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "recoil/recoil_callconv.h"

struct HudUiBriefingRuntime;

typedef void( *HudUiBriefingRuntimeUpdate)(
    HudUiBriefingRuntime *self,
    float deltaSec
);
typedef void( *HudUiBriefingRuntimeSetEnabled)(
    HudUiBriefingRuntime *self,
    int enabled
);
typedef HudUiBriefingRuntime *( *HudUiBriefingRuntimeScalarDeletingDestructor)(
    HudUiBriefingRuntime *self,
    unsigned int flags
);

struct HudUiBriefingRuntimeVtable {
    HudUiBriefingRuntimeUpdate Update;
    HudUiBriefingRuntimeSetEnabled SetEnabled;
    HudUiBriefingRuntimeScalarDeletingDestructor ScalarDeletingDtor;
};
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBriefingRuntimeVtable,
        ScalarDeletingDtor
    ) == 0x08
);

struct HudUiBriefingTransportProgress {
    const void *vptr;
};

struct HudUiBriefingRuntime {
    HudUiBriefingRuntimeVtable *vptr;
    unsigned char unknown_0004[0xa95c];
    HudUiBriefingTransportProgress transportProgress;

    HudUiBriefingRuntime * Constructor(int missionId);
    void Destructor();
    HudUiBriefingRuntime * ScalarDeletingDestructor(unsigned int flags);
    int BuildObjectiveActionsFromIndex(int objectiveIndex);
    void Update(float deltaSec);
};
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBriefingRuntime,
        transportProgress
    ) == 0xa960
);

namespace Briefing {
void __fastcall BuildObjectiveActionsGlobal(int objectiveIndex);
void __fastcall SampleEventCallback(int progressEventCode);
int __fastcall StartForMission(int missionId);
void ThreadMain(void *threadParameter);
void __fastcall StopAndShutdownThread(int waitForInput);
void __stdcall SetProgressAndSleep(float progressValue);
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
