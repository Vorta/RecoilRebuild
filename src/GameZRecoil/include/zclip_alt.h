#ifndef GAMEZRECOIL_INCLUDE_ZCLIP_ALT_H
#define GAMEZRECOIL_INCLUDE_ZCLIP_ALT_H

#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"
#include "zclip_rect.h"

typedef struct CZCameraDataPartial CZCameraDataPartial;

typedef struct zClipAltFloatRect {
    float left;
    float top;
    float right;
    float bottom;
} zClipAltFloatRect;

#ifdef __cplusplus
extern "C" {
#endif
extern int g_zClipAlt_BiasIncludesPrimaryOrigin;

void __fastcall zClipAltBuildFrustumPlanes(CZCameraDataPartial* cameraData);
#ifdef __cplusplus
}

namespace zClipAlt {
extern "C" {
#endif
void __fastcall SetSourceRect(const zClipAltFloatRect* rect);
void __fastcall SetTargetRect(const zClipAltFloatRect* rect, int replicate);
int __fastcall RemapPointXYInPlace(float* point);
#ifdef __cplusplus
}
} // namespace zClipAlt
#endif

#endif // GAMEZRECOIL_INCLUDE_ZCLIP_ALT_H
