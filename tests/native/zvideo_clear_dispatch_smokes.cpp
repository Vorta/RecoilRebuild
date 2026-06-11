#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdint>

namespace {
int g_clearSwCalls;
int g_clearPrimaryCalls;
zVidRect32 *g_lastClearSwSurfaceRect;
zVidRect32 *g_lastClearSwZRect;
zVidRect32 *g_lastClearPrimaryRect;
zVideo_SurfaceStatePartial *g_lastClearPrimaryState;

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
