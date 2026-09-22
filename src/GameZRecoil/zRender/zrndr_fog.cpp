// Inferred current implementation path; original filenames and the
// historical staged-fog/span compiler-input boundary remain unresolved.

#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zRender/zrndr.h"

#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zclass.h"

#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-fogcolor-setrgb01clamped
 * @recoil-artifact defines .text recoil:function:0x49b1e0: zRndr::FogColorSetRgb01Clamped
 *
 * Purpose: Clamp and commit the active fog color, then rebuild its packed 16-bit ramp.
 */
void __fastcall FogColorSetRgb01Clamped(zColorRgb* color)
{
    if (color->red > 1.0f) {
        color->red = 1.0f;
    } else if (!(color->red >= 0.0f)) {
        color->red = 0.0f;
    }

    if (color->green > 1.0f) {
        color->green = 1.0f;
    } else if (!(color->green >= 0.0f)) {
        color->green = 0.0f;
    }

    if (color->blue > 1.0f) {
        color->blue = 1.0f;
    } else if (!(color->blue >= 0.0f)) {
        color->blue = 0.0f;
    }

    if (fabs(g_fogColorParams.colorRgb01[0] - color->red) < 0.01f
        && fabs(g_fogColorParams.colorRgb01[1] - color->green) < 0.01f
        && fabs(g_fogColorParams.colorRgb01[2] - color->blue) < 0.01f) {
        return;
    }

    g_fogColorParams.colorRgb01[0] = color->red;
    g_fogColorParams.colorRgb01[1] = color->green;
    g_fogColorParams.colorRgb01[2] = color->blue;

    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo::SetFogColorFromRgb01((zVideo_ColorRgbFloat*)(color));
    }

    const int red = (int)(color->red * 255.0f + 0.5f);
    const int green = (int)(color->green * 255.0f + 0.5f);
    const unsigned int blue = (unsigned int)(color->blue * 255.0f + 0.5f);
    FogTarget565SetPackedColorAndRamp(
        &g_fogColorParams,
        (red << g_pixelPackRedShift) & (int)(g_pixelPackRedMask),
        (green << g_pixelPackGreenShift) & (int)(g_pixelPackGreenMask),
        blue >> g_pixelPackBlueShift
    );
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-setfogtargetcolorrgb01clamped
 * @recoil-artifact defines .text recoil:function:0x49b350: zRndr::SetFogTargetColorRgb01Clamped
 *
 * Purpose: Clamp and commit the immediate fog target color, then rebuild its packed 16-bit ramp.
 */
void __fastcall SetFogTargetColorRgb01Clamped(zColorRgb* color)
{
    if (color->red > 1.0f) {
        color->red = 1.0f;
    } else if (!(color->red >= 0.0f)) {
        color->red = 0.0f;
    }

    if (color->green > 1.0f) {
        color->green = 1.0f;
    } else if (!(color->green >= 0.0f)) {
        color->green = 0.0f;
    }

    if (color->blue > 1.0f) {
        color->blue = 1.0f;
    } else if (!(color->blue >= 0.0f)) {
        color->blue = 0.0f;
    }

    if (fabs(g_fogTargetParamsDirect.colorRgb01[0] - color->red) < 0.01f
        && fabs(g_fogTargetParamsDirect.colorRgb01[1] - color->green) < 0.01f
        && fabs(g_fogTargetParamsDirect.colorRgb01[2] - color->blue) < 0.01f) {
        return;
    }

    g_fogTargetParamsDirect.colorRgb01[0] = color->red;
    g_fogTargetParamsDirect.colorRgb01[1] = color->green;
    g_fogTargetParamsDirect.colorRgb01[2] = color->blue;

    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo::SetFogTargetColorFromRgb01((zVideo_ColorRgbFloat*)(color));
    }

    const int red = (int)(color->red * 255.0f + 0.5f);
    const int green = (int)(color->green * 255.0f + 0.5f);
    const unsigned int blue = (unsigned int)(color->blue * 255.0f + 0.5f);
    FogTarget565SetPackedColorAndRamp(
        &g_fogTargetParamsDirect,
        (red << zRndr::g_pixelPackRedShift) & (int)(zRndr::g_pixelPackRedMask),
        (green << zRndr::g_pixelPackGreenShift) & (int)(zRndr::g_pixelPackGreenMask),
        blue >> zRndr::g_pixelPackBlueShift
    );
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-commitdirectfogparamsifchanged
 * @recoil-artifact defines .text recoil:function:0x49b4c0: zRndr::CommitDirectFogParamsIfChanged
 *
 * Purpose: Copy direct fog target parameters into the active fog state when they differ.
 */
void __cdecl CommitDirectFogParamsIfChanged()
{
    if (fabs(g_fogParamsActive.colorRgb01[0] - g_fogTargetParamsDirect.colorRgb01[0]) >= 0.01f
        || fabs(g_fogParamsActive.colorRgb01[1] - g_fogTargetParamsDirect.colorRgb01[1]) >= 0.01f
        || fabs(g_fogParamsActive.colorRgb01[2] - g_fogTargetParamsDirect.colorRgb01[2]) >= 0.01f) {
        memcpy(&g_fogParamsActive, &g_fogTargetParamsDirect, sizeof(g_fogParamsActive));
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-commitfogcolorparamsifchanged
 * @recoil-artifact defines .text recoil:function:0x49b530: zRndr::CommitFogColorParamsIfChanged
 *
 * Purpose: Copy fog color parameters into the active fog state when they differ.
 */
void __cdecl CommitFogColorParamsIfChanged()
{
    if (fabs(g_fogParamsActive.colorRgb01[0] - g_fogColorParams.colorRgb01[0]) >= 0.01f
        || fabs(g_fogParamsActive.colorRgb01[1] - g_fogColorParams.colorRgb01[1]) >= 0.01f
        || fabs(g_fogParamsActive.colorRgb01[2] - g_fogColorParams.colorRgb01[2]) >= 0.01f) {
        memcpy(&g_fogParamsActive, &g_fogColorParams, sizeof(g_fogParamsActive));
    }
}
} // namespace zRndr
