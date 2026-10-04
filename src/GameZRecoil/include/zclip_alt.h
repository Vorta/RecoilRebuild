#ifndef GAMEZRECOIL_INCLUDE_ZCLIP_ALT_H
#define GAMEZRECOIL_INCLUDE_ZCLIP_ALT_H

#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"
#include "zclip_rect.h"

struct CZCameraDataPartial;

struct zClipAltFloatRect {
    float left;
    float top;
    float right;
    float bottom;
};

extern "C" {
extern int g_zClipAlt_BiasIncludesPrimaryOrigin;
}

void __fastcall zClipAltBuildFrustumPlanes(CZCameraDataPartial* cameraData);

namespace zClipAlt {
void __fastcall SetSourceRect(const zClipAltFloatRect* rect);
void __fastcall SetTargetRect(const zClipAltFloatRect* rect, int replicate);
int __fastcall RemapPointXYInPlace(float* point);
} // namespace zClipAlt

#endif // GAMEZRECOIL_INCLUDE_ZCLIP_ALT_H
