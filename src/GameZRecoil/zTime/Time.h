#ifndef GAMEZRECOIL_TIME_TIME_H
#define GAMEZRECOIL_TIME_TIME_H

#pragma once

#include "recoil/recoil_callconv.h"

#include "recoil/recoil_types.h"

/**
 * Retail 0x4e2fa8..0x4e2fb8 is one initialized Time runtime/config record.
 */
typedef struct TimeRuntimeConfig {
    float maximumDeltaTimeSec;
    int deltaTimeClampEnabled;
    float currentTimeSec;
    float newTimeSec;
    float timeScaleFactor;
} TimeRuntimeConfig;

RECOIL_STATIC_ASSERT(sizeof(TimeRuntimeConfig) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(TimeRuntimeConfig, maximumDeltaTimeSec) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(TimeRuntimeConfig, deltaTimeClampEnabled) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(TimeRuntimeConfig, currentTimeSec) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(TimeRuntimeConfig, newTimeSec) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(TimeRuntimeConfig, timeScaleFactor) == 0x10);

#ifdef __cplusplus
namespace Time {
void __cdecl Reset();
void __cdecl Tick();
} // namespace Time

extern "C" {
#endif
extern TimeRuntimeConfig g_Time_RuntimeConfig;
extern float g_FrameDeltaTimeSec;
extern float g_Time_AccumulatedTimeSec;
extern float g_Time_UnscaledDeltaTimeSec;
extern float g_Time_UnscaledAccumulatedTimeSec;
#ifdef __cplusplus
}
#endif

#endif
