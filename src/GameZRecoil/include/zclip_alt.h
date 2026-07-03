#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"
#include "zclip_rect.h"

struct zClass_CameraDataPartial;

struct zClipAltFloatRect {
    float left;
    float top;
    float right;
    float bottom;
};

extern "C" {
extern float g_zClipAlt_SourceLeft;
extern float g_zClipAlt_SourceTop;
extern float g_zClipAlt_SourceRight;
extern float g_zClipAlt_SourceBottom;
extern float g_zClipAlt_SourceWidth;
extern float g_zClipAlt_SourceHeight;
extern zClipRectPartial gClipRect_Alt;
extern float g_zClipAlt_RemapOffsetX;
extern float g_zClipAlt_RemapOffsetY;
extern float g_zClipAlt_RemapScaleX;
extern float g_zClipAlt_RemapScaleY;
extern float g_zClipAlt_RemapBiasX;
extern float g_zClipAlt_RemapBiasY;
extern int g_zClipAlt_BiasIncludesPrimaryOrigin;
extern int gAltClipSourceRectValid;
extern int gAltClipPassEnabled;
}

void __fastcall zClipAlt_BuildFrustumPlanes(
    zClass_CameraDataPartial *cameraData
);

namespace zClipAlt {
void __fastcall SetSourceRect(const zClipAltFloatRect *rect);
void __fastcall SetTargetRect(
    const zClipAltFloatRect *rect,
    int replicate
);
int __fastcall RemapPointXYInPlace(float *point);
} // namespace zClipAlt
