#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdint>

namespace {
int g_clearSwCalls;
int g_clearPrimaryCalls;
zVidRect32 *g_lastClearSwSurfaceRect;
zVidRect32 *g_lastClearSwZRect;
zVidRect32 *g_lastClearPrimaryRect;
zVideo_SurfaceStatePartial *g_lastClearPrimaryState;
int g_lockSurfaceCalls;
int g_unlockSurfaceCalls;
zVideo_SurfaceStatePartial *g_lastLockSurfaceState;
zVideo_SurfaceStatePartial *g_lastUnlockSurfaceState;

void __fastcall ClearSwFake(zVidRect32 *surfaceRect, zVidRect32 *zRect) {
    ++g_clearSwCalls;
    g_lastClearSwSurfaceRect = surfaceRect;
    g_lastClearSwZRect = zRect;
}

void __fastcall ClearStateFake(
    zVidRect32 *rect,
    zVideo_SurfaceStatePartial *surfaceState
) {
    ++g_clearPrimaryCalls;
    g_lastClearPrimaryRect = rect;
    g_lastClearPrimaryState = surfaceState;
}

int __fastcall LockSurfaceStateFake(zVideo_SurfaceStatePartial *surfaceState) {
    ++g_lockSurfaceCalls;
    g_lastLockSurfaceState = surfaceState;
    return 0x35a;
}

int __fastcall UnlockSurfaceStateFake(zVideo_SurfaceStatePartial *surfaceState) {
    ++g_unlockSurfaceCalls;
    g_lastUnlockSurfaceState = surfaceState;
    surfaceState->locked = 0;
    return 0x6a5;
}
} // namespace

extern "C" int zvideo_clear_dispatch_and_exchange_smoke(void) {
    zVidRect32 surfaceRect = {1, 2, 3, 4};
    zVidRect32 zRect = {5, 6, 7, 8};
    g_clearSwCalls = 0;
    g_clearPrimaryCalls = 0;
    g_lastClearSwSurfaceRect = 0;
    g_lastClearSwZRect = 0;
    g_lastClearPrimaryRect = 0;
    g_lastClearPrimaryState = 0;
    g_zVideo_pfnClearSwSurfaceAndZBuffer = ClearSwFake;
    g_zVideo_pfnClearStateSurfaceAndZBuffer = ClearStateFake;
    g_zVideo_ClearScreenBufferEnabled = 3;

    zVideo::CallClearSwSurfaceAndZBuffer(&surfaceRect, &zRect);
    zVideo::CallClearPrimarySurfaceAndZBuffer(&surfaceRect);
    if (g_clearSwCalls != 1 || g_lastClearSwSurfaceRect != &surfaceRect ||
        g_lastClearSwZRect != &zRect || g_clearPrimaryCalls != 1 ||
        g_lastClearPrimaryRect != &surfaceRect ||
        g_lastClearPrimaryState != &g_zVideo_PrimarySurfaceState) {
        return 1;
    }

    const std::int32_t previous = zVideo::ExchangeClearScreenBufferEnabled(1);
    return previous == 3 && zVideo::GetClearScreenBufferEnabled() == 1 ? 0 : 2;
}

extern "C" int zvideo_dispatch_wrappers_smoke(void) {
    zVideo_SurfaceStateProc const savedLockSurfaceState = g_zVideo_pfnLockSurfaceState;
    zVideo_SurfaceStateProc const savedUnlockSurfaceState = g_zVideo_pfnUnlockSurfaceState;
    const zVideo_SurfaceStatePartial savedDisplayModeSurfaceState =
        g_zVideo_DisplayModeSurfaceState;
    const zVideo_SurfaceStatePartial savedSwSurfaceState = g_zVideo_SwSurfaceState;

    g_zVideo_DisplayModeSurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_SwSurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_SwSurfaceState.locked = 1;
    g_zVideo_pfnLockSurfaceState = LockSurfaceStateFake;
    g_zVideo_pfnUnlockSurfaceState = UnlockSurfaceStateFake;
    g_lockSurfaceCalls = 0;
    g_unlockSurfaceCalls = 0;
    g_lastLockSurfaceState = 0;
    g_lastUnlockSurfaceState = 0;

    const int displayLockResult = zVideo::Dispatch_LockDisplayModeSurfaceState();
    const int displayUnlockResult = zVideo::Dispatch_UnlockDisplayModeSurfaceState();
    const int swUnlockResult = zVideo::Dispatch_UnlockSwSurfaceState();

    const bool ok =
        displayLockResult == 0x35a &&
        displayUnlockResult == 0x6a5 &&
        swUnlockResult == 0x6a5 &&
        g_lockSurfaceCalls == 1 &&
        g_lastLockSurfaceState == &g_zVideo_DisplayModeSurfaceState &&
        g_unlockSurfaceCalls == 2 &&
        g_lastUnlockSurfaceState == &g_zVideo_SwSurfaceState &&
        g_zVideo_DisplayModeSurfaceState.locked == 0 &&
        g_zVideo_SwSurfaceState.locked == 0;

    g_zVideo_pfnLockSurfaceState = savedLockSurfaceState;
    g_zVideo_pfnUnlockSurfaceState = savedUnlockSurfaceState;
    g_zVideo_DisplayModeSurfaceState = savedDisplayModeSurfaceState;
    g_zVideo_SwSurfaceState = savedSwSurfaceState;

    return ok ? 0 : 1;
}
