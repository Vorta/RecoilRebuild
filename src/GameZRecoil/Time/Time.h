#pragma once

#include "recoil/recoil_callconv.h"

#include "recoil/recoil_types.h"

namespace Time {
RECOIL_NOINLINE void RECOIL_CDECL Reset();
RECOIL_NOINLINE void RECOIL_CDECL Tick();
}

extern "C" {
extern float g_Time_MaximumDeltaTimeSec;
extern int g_Time_DeltaTimeClampEnabled;
extern float g_Time_CurrentTimeSec;
extern float g_Time_NewTimeSec;
extern float g_Time_TimeScaleFactor;
extern float g_FrameDeltaTimeSec;
extern float g_Time_AccumulatedTimeSec;
extern float g_Time_UnscaledDeltaTimeSec;
extern float g_Time_UnscaledAccumulatedTimeSec;
}
