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
namespace {
    /**
     * Recovered inline helper: zRndr fog packed-color rotate
     * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200 and
     * 0x49e300 fog blend callers as the rotate-right term in packed 565/555 ramp blending. Purpose: Rotate packed
     * 32-bit color terms right by a caller-selected bit count.
     */
    static inline unsigned int RotateRight32(unsigned int value, int count)
    {
        return (value >> count) | (value << (32 - count));
    }

    /**
     * Recovered inline helper: zRndr fog saturated-coordinate test
     * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200,
     * 0x49e300, 0x49e400, and 0x49e560 before ramp or solid-fog blending. Purpose: Detect fog coordinates that have
     * reached the fully fogged color.
     */
    static inline bool FogCoordIsFullyFogged(unsigned int fogCoordFixed24)
    {
        return (int)(fogCoordFixed24) >= 0x1000000;
    }

    /**
     * Recovered inline helper: zRndr fog ramp-range test
     * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200,
     * 0x49e300, 0x49e400, and 0x49e560 before ramp lookup. Purpose: Detect fog coordinates that should use the packed
     * color ramp.
     */
    static inline bool FogCoordUsesRamp(unsigned int fogCoordFixed24)
    {
        return (int)(fogCoordFixed24) >= 0x80000;
    }

    /**
     * Recovered inline helper: zRndr fog ramp index
     * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200,
     * 0x49e300, 0x49e400, and 0x49e560 as the fixed-point ramp lookup expression. Purpose: Convert a fixed-point fog
     * coordinate into the 32-entry ramp index.
     */
    static inline unsigned int FogRampIndex(unsigned int fogCoordFixed24)
    {
        return (0x1000000u - fogCoordFixed24) >> 19;
    }

    /**
     * Recovered inline helper: zRndr 565 fog pixel blend
     * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200 and
     * through the 0x49e400 MMX-shaped fog blend tail. Purpose: Blend one 565 pixel against the active packed fog ramp.
     */
    static inline unsigned short FogBlendPixel565(unsigned short pixel, unsigned int fogCoordFixed24)
    {
        if (FogCoordIsFullyFogged(fogCoordFixed24)) {
            return (unsigned short)(g_fogParamsActive.packedColor16);
        }

        if (!FogCoordUsesRamp(fogCoordFixed24)) {
            return pixel;
        }

        const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
        const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
        const unsigned int pixel32 = pixel;
        const unsigned int green = ((((pixel32 & 0x07e0u) >> 5) * rampIndex) + rampValue) & 0x07e0u;
        const unsigned int redBlue = (((pixel32 & 0xf81fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) & 0xf81fu;
        return (unsigned short)(green + redBlue);
    }

    /**
     * Recovered inline helper: zRndr 555 fog pixel blend
     * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e300 and
     * through the 0x49e560 MMX-shaped fog blend tail. Purpose: Blend one 555 pixel against the active packed fog ramp.
     */
    static inline unsigned short FogBlendPixel555(unsigned short pixel, unsigned int fogCoordFixed24)
    {
        if (FogCoordIsFullyFogged(fogCoordFixed24)) {
            return (unsigned short)(g_fogParamsActive.packedColor16);
        }

        if (!FogCoordUsesRamp(fogCoordFixed24)) {
            return pixel;
        }

        const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
        const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
        const unsigned int pixel32 = pixel;
        const unsigned int green = ((((pixel32 & 0x03e0u) >> 5) * rampIndex) + rampValue) & 0x03e0u;
        const unsigned int redBlue = (((pixel32 & 0x7c1fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) & 0x7c1fu;
        return (unsigned short)(green + redBlue);
    }

    /**
     * Recovered inline helper: zRndr 565 fog pair blend
     * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e200
     * paired-pixel fog loops. Purpose: Blend two packed 565 pixels against the active packed fog ramp.
     */
    static inline unsigned int FogBlendPair565(unsigned int packedPixels, unsigned int fogCoordFixed24)
    {
        if (FogCoordIsFullyFogged(fogCoordFixed24)) {
            return (unsigned int)(g_fogParamsActive.packedColor16Dup);
        }

        if (!FogCoordUsesRamp(fogCoordFixed24)) {
            return packedPixels;
        }

        const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
        const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
        const unsigned int green = ((((packedPixels & 0xf81f07e0u) >> 5) * rampIndex) + rampValue) & 0xf81f07e0u;
        const unsigned int redBlue
            = (((packedPixels & 0x07e0f81fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) & 0x07e0f81fu;
        return green + redBlue;
    }

    /**
     * Recovered inline helper: zRndr 555 fog pair blend
     * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49e300
     * paired-pixel fog loops. Purpose: Blend two packed 555 pixels against the active packed fog ramp.
     */
    static inline unsigned int FogBlendPair555(unsigned int packedPixels, unsigned int fogCoordFixed24)
    {
        if (FogCoordIsFullyFogged(fogCoordFixed24)) {
            return (unsigned int)(g_fogParamsActive.packedColor16Dup);
        }

        if (!FogCoordUsesRamp(fogCoordFixed24)) {
            return packedPixels;
        }

        const unsigned int rampIndex = FogRampIndex(fogCoordFixed24);
        const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
        const unsigned int green = ((((packedPixels & 0x7c1f03e0u) >> 5) * rampIndex) + rampValue) & 0x7c1f03e0u;
        const unsigned int redBlue
            = (((packedPixels & 0x03e07c1fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) & 0x03e07c1fu;
        return green + redBlue;
    }

    /**
     * Scalar emulation helper: zRndr signed MMX word subtract
     * BN retail evidence: 0x49e400 and 0x49e560 use MMX signed saturating word
     * subtracts inside the fog blend lanes; this helper is not accepted
     * original-source inline-helper evidence.
     * Purpose: Emulate the saturating signed word subtract used by the MMX fog lane.
     */
    static inline short SaturatingSubWord(unsigned short minuend, unsigned short subtrahend)
    {
        const int result = (short)(minuend) - (short)(subtrahend);
        if (result > 0x7fff) {
            return 0x7fff;
        }
        if (result < -0x8000) {
            return -32768;
        }
        return (short)(result);
    }

    /**
     * Scalar emulation helper: zRndr signed MMX low-word multiply
     * BN retail evidence: 0x49e400 and 0x49e560 use MMX signed low-word
     * multiplies inside the fog blend lanes; this helper is not accepted
     * original-source inline-helper evidence.
     * Purpose: Emulate the low-word signed multiply used by the MMX fog lane.
     */
    static inline unsigned short MultiplyLowWord(short lhs, short rhs)
    {
        return (unsigned short)((int)(lhs) * (int)(rhs));
    }

    /**
     * Scalar emulation helper: zRndr MMX fog lane blend
     * BN retail evidence: 0x49e400 and 0x49e560 repeat this per-lane MMX fog
     * math pattern; this helper is behavior/data-equivalent scalar emulation, not
     * accepted original-source inline-helper evidence.
     * Purpose: Blend one lane of the MMX-shaped fog quad with active mask globals.
     */
    static inline unsigned short
    FogBlendMmxLane(unsigned short pixel, unsigned short fogFactor, int lane, int redShift, int redTermShift)
    {
        const short factor = (short)(fogFactor);
        const short redDelta = SaturatingSubWord(g_mmxBitsRed255[lane], (unsigned short)(pixel >> redShift));
        const short greenDelta
            = SaturatingSubWord(g_mmxBitsGreen255[lane], (unsigned short)((pixel & g_mmxMaskGreenBits[lane]) >> 5));
        const short blueDelta
            = SaturatingSubWord(g_mmxBitsBlue255[lane], (unsigned short)(pixel & g_mmxMaskBlueBits[lane]));

        const unsigned short redProduct = MultiplyLowWord(redDelta, factor);
        const unsigned short greenProduct = MultiplyLowWord(greenDelta, factor);
        const unsigned short blueProduct = MultiplyLowWord(blueDelta, factor);

        const unsigned short redTerm = (unsigned short)(redProduct << redTermShift) & g_mmxMaskRedPacked[lane];
        const unsigned short greenTerm = (unsigned short)((short)(greenProduct) >> 3) & g_mmxMaskGreenPacked[lane];
        const unsigned short blueTerm = (unsigned short)((short)(blueProduct) >> 8);

        return (unsigned short)(pixel + redTerm + greenTerm + blueTerm);
    }

    /**
     * Recovered inline helper: zRndr span texture sample index
     * Original-source inline helper evidence: No standalone retail function is expected; observed in span callers
     * including 0x49e6c0, 0x49b7e0, 0x49edc0, 0x49bbf0, and 0x49f180. Purpose: Combine fixed-point texture U and masked
     * V coordinates into the active texture sample index.
     */
    static inline int SpanTex16SampleIndex(int texU, int texV, int texVShift, int texUMask)
    {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & texUMask;
        return vIndex + uIndex;
    }

    /**
     * Recovered inline helper: zRndr 16-bit texture sample
     * Original-source inline helper evidence: No standalone plan/source-map entry; observed in span callers including
     * 0x49e6c0, 0x49ea80, and 0x49ec20. Purpose: Read a 16-bit texel from the active texture using the recovered
     * fixed-point sample-index helper.
     */
    static inline unsigned short SpanTex16Sample(int texU, int texV, int texVShift, int texUMask)
    {
        const unsigned short* texels = (const unsigned short*)(g_spanActiveTexPixels);
        return texels[SpanTex16SampleIndex(texU, texV, texVShift, texUMask)];
    }

    /**
     * Recovered inline helper: zRndr palettized texture sample expansion
     * Original-source inline helper evidence: No standalone plan/source-map entry; observed in 0x49edc0, 0x49bbf0, and
     * 0x49f180 palettized texture span patterns. Purpose: Expand an 8-bit texture sample through the active span
     * palette.
     */
    static inline unsigned short SpanPal8SampleExpanded(int texU, int texV, int texVShift, int texUMask)
    {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, texUMask);
        return g_spanActiveTexPalette[g_spanActiveTexPixels[sourceIndex]];
    }

    /**
     * Recovered inline helper: zRndr 565 alpha pixel blend
     * Original-source inline helper evidence: No standalone retail function is expected; observed across 0x49c360,
     * 0x49c970, 0x49cbb0, 0x49d1a0, 0x49d810, and 0x49da80 alpha-map span callers. Purpose: Blend one 565 destination
     * pixel toward a source pixel using an 8-bit alpha value.
     */
    static inline unsigned short BlendPixel565Alpha8(unsigned short dstPixel, unsigned short srcPixel, int alpha)
    {
        const int dstColor = (short)(dstPixel);
        const int srcColor = srcPixel;
        const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
        const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
        int blended = dstColor + (redDelta & 0xfffff800);
        const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
        blended += (greenDelta & 0xffffffe0) + blueDelta;
        return (unsigned short)(blended);
    }

    /**
     * Recovered inline helper: zRndr 555 alpha pixel blend
     * Original-source inline helper evidence: No standalone retail function is expected; observed across 0x49c560,
     * 0x49ca90, 0x49cea0, 0x49d3b0, 0x49d950, and 0x49ddb0 alpha-map span callers. Purpose: Blend one 555 destination
     * pixel toward a source pixel using an 8-bit alpha value.
     */
    static inline unsigned short BlendPixel555Alpha8(unsigned short dstPixel, unsigned short srcPixel, int alpha)
    {
        const int dstColor = (short)(dstPixel);
        const int srcColor = srcPixel;
        const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
        int blended = dstColor + (redDelta & 0xfffffc00);
        const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
        const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
        blended += (greenDelta & 0xffffffe0) + blueDelta;
        return (unsigned short)(blended);
    }

    /**
     * Recovered inline helper: zRndr 555 constant-alpha-map pixel blend
     * Original-source inline helper evidence: No standalone retail function is expected; observed in 0x49ca90 and
     * 0x49d950 scaled alpha-map span callers. Purpose: Blend one 555 destination pixel toward a source pixel using a
     * scaled alpha-map value.
     */
    static inline unsigned short BlendPixel555ConstAlphaMap(unsigned short dstPixel, unsigned short srcPixel, int alpha)
    {
        const int dstColor = (short)(dstPixel);
        const int srcColor = srcPixel;
        const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
        const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
        const int blueDelta = (((srcColor & 0x001f) - (dstColor & 0x001f)) * alpha) >> 8;
        return (unsigned short)(dstColor + (redDelta & 0xfffffc00) + (greenDelta & 0xffffffe0) + blueDelta);
    }

}
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-zrndr-fogtargetcolorstaged-setrgb01clamped
 * @recoil-artifact defines .text recoil:function:0x49b5a0: zRndrFogTargetColorStagedSetRgb01Clamped
 *
 * Purpose: Clamp and stage the pending fog target color, then rebuild its packed 16-bit ramp.
 */
void __fastcall zRndrFogTargetColorStagedSetRgb01Clamped(zColorRgb* color)
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

    zRndr::FogParamsPartial* staged = &zRndr::g_fogTargetParamsStaged;
    if (fabs(staged->colorRgb01[0] - color->red) < 0.01f && fabs(staged->colorRgb01[1] - color->green) < 0.01f
        && fabs(staged->colorRgb01[2] - color->blue) < 0.01f) {
        return;
    }

    staged->colorRgb01[0] = color->red;
    staged->colorRgb01[1] = color->green;
    staged->colorRgb01[2] = color->blue;

    if (g_zVideo_ActiveRendererPath != 0) {
        zVideoSetPendingFogTargetColorFromRgb01((zVideo_ColorRgbFloat*)(color));
    }

    const int red = (int)(color->red * 255.0f + 0.5f);
    const int green = (int)(color->green * 255.0f + 0.5f);
    const unsigned int blue = (unsigned int)(color->blue * 255.0f + 0.5f);
    zRndr::FogTarget565SetPackedColorAndRamp(
        staged,
        (red << zRndr::g_pixelPackRedShift) & (int)(zRndr::g_pixelPackRedMask),
        (green << zRndr::g_pixelPackGreenShift) & (int)(zRndr::g_pixelPackGreenMask),
        blue >> zRndr::g_pixelPackBlueShift
    );
}

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-commitstagedfogparamsifchanged
 * @recoil-artifact defines .text recoil:function:0x49b710: zRndr::CommitStagedFogParamsIfChanged
 *
 * Purpose: Copy staged fog target parameters into the active fog state when they differ.
 */
void __cdecl CommitStagedFogParamsIfChanged()
{
    if (fabs(g_fogParamsActive.colorRgb01[0] - g_fogTargetParamsStaged.colorRgb01[0]) >= 0.01f
        || fabs(g_fogParamsActive.colorRgb01[1] - g_fogTargetParamsStaged.colorRgb01[1]) >= 0.01f
        || fabs(g_fogParamsActive.colorRgb01[2] - g_fogTargetParamsStaged.colorRgb01[2]) >= 0.01f) {
        memcpy(&g_fogParamsActive, &g_fogTargetParamsStaged, sizeof(g_fogParamsActive));
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-blendpackedcolor565withfoginplace
 * @recoil-artifact defines .text recoil:function:0x49b780: zRndr::BlendPackedColor565WithFogInPlace
 *
 * Inferred implementation placement: the reviewed renderer fog/span family.
 * The original filename remains unresolved; BN names are navigation labels.
 * Purpose: Blend a packed 565 color in place toward the active fog color.
 */
void __fastcall BlendPackedColor565WithFogInPlace(int* ioPackedColor, int blend255)
{
    const int fogGreen = g_fogParamsActive.packedColorGreen;
    const int packedColor = *ioPackedColor;
    const int greenMask = (int)(g_pixelPackGreenMask);
    const int blueMask = (int)(g_pixelPackBlueMask);

    const int greenDelta = ((fogGreen - (greenMask & packedColor)) * blend255) >> 8;
    const int blueDelta = ((g_fogParamsActive.packedColorBlue - (blueMask & packedColor)) * blend255) >> 8;
    const int redMask = (int)(g_pixelPackRedMask);
    const int redDelta = ((g_fogParamsActive.packedColorRed - (redMask & packedColor)) * blend255) >> 8;

    int blendedColor = redDelta & redMask;
    blendedColor += blueDelta;
    blendedColor += greenDelta & greenMask;
    blendedColor += packedColor;

    *ioPackedColor = blendedColor;
}
} // namespace zRndr

namespace zRndr {
/**
 * Inferred implementation placement: the reviewed renderer fog/span family.
 * Source-shape evidence: BN assembly uses a texVShift 10..17 jump table, saves
 * through gRndr_SavedEspSlot, pivots ESP to gRndr_CurrentSpanBaseAddr + count,
 * samples gRndr_ActiveTexPixels as 16-bit texels, and either pushes a nonzero
 * word or subtracts two bytes so zero texels leave the destination transparent.
 * The guarded VC5 x86 path keeps C++ responsible for dispatch and uses narrow
 * inline asm only for the ESP-pivot masked write/skip loop; the portable
 * fallback below remains behavior-only.
 * Purpose: Write nonzero 16-bit texels into the active span using the texVShift-specialized reverse span loops.
 */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_ESP_PIVOT_RAW_ASM)
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-masked-16-from-tex16-switch-vshift
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-masked-16-from-tex16-switch-vshift recoil:function:0x49b7e0
 *
 * Purpose: Reimplements 0x49b7e0 as a disabled ESP-pivot alternative. Write nonzero 16-bit texels through C++ switch
 * cases with narrow inline asm for the approved zRndr ESP-pivot loop. BN proves the ESP pivot; scoped VC5 C++ forms
 * failed.
 */
void __fastcall SpanMasked16FromTex16SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_mask_tex16_switch_loop10:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0ah
            and eax, 3ffh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            cmp word ptr [ebp+eax*2], 0
            je zRndr_span_mask_tex16_switch_skip10
            push word ptr [ebp+eax*2]
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop10
            jmp zRndr_span_mask_tex16_switch_restore10
        zRndr_span_mask_tex16_switch_skip10:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop10
        zRndr_span_mask_tex16_switch_restore10:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 11:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_mask_tex16_switch_loop11:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0bh
            and eax, 1ffh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            cmp word ptr [ebp+eax*2], 0
            je zRndr_span_mask_tex16_switch_skip11
            push word ptr [ebp+eax*2]
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop11
            jmp zRndr_span_mask_tex16_switch_restore11
        zRndr_span_mask_tex16_switch_skip11:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop11
        zRndr_span_mask_tex16_switch_restore11:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 12:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_mask_tex16_switch_loop12:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0ch
            and eax, 0ffh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            cmp word ptr [ebp+eax*2], 0
            je zRndr_span_mask_tex16_switch_skip12
            push word ptr [ebp+eax*2]
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop12
            jmp zRndr_span_mask_tex16_switch_restore12
        zRndr_span_mask_tex16_switch_skip12:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop12
        zRndr_span_mask_tex16_switch_restore12:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 13:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_mask_tex16_switch_loop13:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0dh
            and eax, 7fh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            cmp word ptr [ebp+eax*2], 0
            je zRndr_span_mask_tex16_switch_skip13
            push word ptr [ebp+eax*2]
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop13
            jmp zRndr_span_mask_tex16_switch_restore13
        zRndr_span_mask_tex16_switch_skip13:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop13
        zRndr_span_mask_tex16_switch_restore13:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 14:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_mask_tex16_switch_loop14:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0eh
            and eax, 3fh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            cmp word ptr [ebp+eax*2], 0
            je zRndr_span_mask_tex16_switch_skip14
            push word ptr [ebp+eax*2]
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop14
            jmp zRndr_span_mask_tex16_switch_restore14
        zRndr_span_mask_tex16_switch_skip14:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop14
        zRndr_span_mask_tex16_switch_restore14:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 15:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_mask_tex16_switch_loop15:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0fh
            and eax, 1fh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            cmp word ptr [ebp+eax*2], 0
            je zRndr_span_mask_tex16_switch_skip15
            push word ptr [ebp+eax*2]
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop15
            jmp zRndr_span_mask_tex16_switch_restore15
        zRndr_span_mask_tex16_switch_skip15:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop15
        zRndr_span_mask_tex16_switch_restore15:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 16:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_mask_tex16_switch_loop16:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 10h
            and eax, 0fh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            cmp word ptr [ebp+eax*2], 0
            je zRndr_span_mask_tex16_switch_skip16
            push word ptr [ebp+eax*2]
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop16
            jmp zRndr_span_mask_tex16_switch_restore16
        zRndr_span_mask_tex16_switch_skip16:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop16
        zRndr_span_mask_tex16_switch_restore16:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 17:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_mask_tex16_switch_loop17:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 11h
            and eax, 7
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            cmp word ptr [ebp+eax*2], 0
            je zRndr_span_mask_tex16_switch_skip17
            push word ptr [ebp+eax*2]
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop17
            jmp zRndr_span_mask_tex16_switch_restore17
        zRndr_span_mask_tex16_switch_skip17:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_tex16_switch_loop17
        zRndr_span_mask_tex16_switch_restore17:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;
    }
}
#else
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zrender.span-masked-16-from-tex16-switch-vshift
 * @recoil-artifact defines .text recoil:function:0x49b7e0: Current C++ definition under the canonical compiler settings.
 *
 * Original function evidence: retail 0x49b7e0 has this portable conditional definition.
 * Purpose: Preserve portable masked tex16 behavior when the ESP-pivot raw-assembly exception is disabled.
 */
void __fastcall SpanMasked16FromTex16SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 11: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 12: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0xff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 13: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x7f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 14: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 15: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 16: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x0f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 17: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x07) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            const unsigned short source = texels16[sourceIndex];
            if (source != 0) {
                *dstEnd = source;
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }
    }
}
#endif
} // namespace zRndr

namespace zRndr {
/**
 * Inferred implementation placement: the reviewed renderer fog/span family.
 * Source-shape evidence: BN assembly/HLIL shows a texVShift 10..17 jump table;
 * each case saves through gRndr_SavedEspSlot, pivots ESP to
 * gRndr_CurrentSpanBaseAddr + count, samples an 8-bit texel from
 * gRndr_ActiveTexPixels, skips zero texels by reserving the destination word,
 * and expands nonzero texels through gRndr_ActiveTexPalette before pushing the
 * 16-bit palette word backward into the span. The guarded VC5 x86 path keeps
 * C++ responsible for dispatch and uses narrow inline asm only for the
 * ESP-pivot masked write/skip loop; the portable fallback below remains
 * behavior-only.
 * Purpose: Write nonzero palettized texels into the active 16-bit span using the variable-texVShift reverse span
 * contract.
 */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_ESP_PIVOT_RAW_ASM)
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-masked-16-from-pal8-switch-vshift
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-masked-16-from-pal8-switch-vshift recoil:function:0x49bbf0
 *
 * Purpose: Reimplements 0x49bbf0 as a disabled ESP-pivot alternative. Write nonzero palettized texels through C++
 * switch cases with narrow inline asm for the approved zRndr ESP-pivot loop. BN proves the ESP pivot; scoped VC5 C++
 * forms failed.
 */
void __fastcall SpanMasked16FromPal8SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_mask_pal8_switch_loop10:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0ah
            and eax, 3ffh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            or eax, eax
            je zRndr_span_mask_pal8_switch_skip10
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_mask_pal8_switch_loop10
            jmp zRndr_span_mask_pal8_switch_restore10
        zRndr_span_mask_pal8_switch_skip10:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_pal8_switch_loop10
        zRndr_span_mask_pal8_switch_restore10:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 11:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_mask_pal8_switch_loop11:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0bh
            and eax, 1ffh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            or eax, eax
            je zRndr_span_mask_pal8_switch_skip11
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_mask_pal8_switch_loop11
            jmp zRndr_span_mask_pal8_switch_restore11
        zRndr_span_mask_pal8_switch_skip11:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_pal8_switch_loop11
        zRndr_span_mask_pal8_switch_restore11:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 12:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_mask_pal8_switch_loop12:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0ch
            and eax, 0ffh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            or eax, eax
            je zRndr_span_mask_pal8_switch_skip12
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_mask_pal8_switch_loop12
            jmp zRndr_span_mask_pal8_switch_restore12
        zRndr_span_mask_pal8_switch_skip12:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_pal8_switch_loop12
        zRndr_span_mask_pal8_switch_restore12:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 13:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_mask_pal8_switch_loop13:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0dh
            and eax, 7fh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            or eax, eax
            je zRndr_span_mask_pal8_switch_skip13
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_mask_pal8_switch_loop13
            jmp zRndr_span_mask_pal8_switch_restore13
        zRndr_span_mask_pal8_switch_skip13:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_pal8_switch_loop13
        zRndr_span_mask_pal8_switch_restore13:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 14:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_mask_pal8_switch_loop14:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0eh
            and eax, 3fh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            or eax, eax
            je zRndr_span_mask_pal8_switch_skip14
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_mask_pal8_switch_loop14
            jmp zRndr_span_mask_pal8_switch_restore14
        zRndr_span_mask_pal8_switch_skip14:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_pal8_switch_loop14
        zRndr_span_mask_pal8_switch_restore14:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 15:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_mask_pal8_switch_loop15:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0fh
            and eax, 1fh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            or eax, eax
            je zRndr_span_mask_pal8_switch_skip15
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_mask_pal8_switch_loop15
            jmp zRndr_span_mask_pal8_switch_restore15
        zRndr_span_mask_pal8_switch_skip15:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_pal8_switch_loop15
        zRndr_span_mask_pal8_switch_restore15:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 16:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_mask_pal8_switch_loop16:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 10h
            and eax, 0fh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            or eax, eax
            je zRndr_span_mask_pal8_switch_skip16
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_mask_pal8_switch_loop16
            jmp zRndr_span_mask_pal8_switch_restore16
        zRndr_span_mask_pal8_switch_skip16:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_pal8_switch_loop16
        zRndr_span_mask_pal8_switch_restore16:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 17:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_mask_pal8_switch_loop17:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 11h
            and eax, 7
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            or eax, eax
            je zRndr_span_mask_pal8_switch_skip17
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_mask_pal8_switch_loop17
            jmp zRndr_span_mask_pal8_switch_restore17
        zRndr_span_mask_pal8_switch_skip17:
            sub esp, 2
            add edi, 2
            jne zRndr_span_mask_pal8_switch_loop17
        zRndr_span_mask_pal8_switch_restore17:
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;
    }
}
#else
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zrender.span-masked-16-from-pal8-switch-vshift
 * @recoil-artifact defines .text recoil:function:0x49bbf0: Current C++ definition under the canonical compiler settings.
 *
 * Original function evidence: retail 0x49bbf0 has this portable conditional definition.
 * Purpose: Preserve portable masked palettized behavior when the ESP-pivot raw-assembly exception is disabled.
 */
void __fastcall SpanMasked16FromPal8SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 11: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 12: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0xff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 13: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x7f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 14: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 15: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 16: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x0f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 17: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x07) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            if (source != 0) {
                *dstEnd = g_spanActiveTexPalette[source];
            }
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }
    }
}
#endif
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanmasked16frompal8to565
 * @recoil-artifact defines .text recoil:function:0x49c020: zRndr::SpanMasked16FromPal8To565
 *
 * Source-shape evidence: BN's retail body owns the same generic V-shift pal8
 * 565 loop as 0x49c230, including the nonzero source gate, alpha > 3 gate,
 * alpha >= 0xfc palette copy, and destination-word palette lookup in the
 * partial-alpha path.
 * Purpose: Write nonzero palettized texture samples into a 565 span using the active constant alpha.
 */
void __fastcall SpanMasked16FromPal8To565(int texU, int texV, int pixelCount, int texVShift)
{
    const unsigned char* texels = g_spanActiveTexPixels;
    int activeAlpha = g_spanActiveConstAlphaBits;
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* palette = g_spanActiveTexPalette;

    do {
        const int vIndex = (int)((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift);
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned short sourceIndex = texels[vIndex + uIndex];
        if (sourceIndex != 0 && (unsigned int)(activeAlpha) > 3) {
            if ((unsigned int)(activeAlpha) >= 0xfc) {
                *dst = palette[(short)(sourceIndex)];
                activeAlpha = g_spanActiveConstAlphaBits;
            } else {
                const int dstColor = (short)(*dst);
                // BN 0x49c0aa intentionally uses the current destination word
                // as the palette index in this partial-alpha path.
                const int srcColor = palette[dstColor];
                const int dstGreen = dstColor & 0x07e0;
                const int srcGreen = srcColor & 0x07e0;
                const int greenDelta = (srcGreen - dstGreen) * activeAlpha;
                const int dstRed = dstColor & 0xf800;
                const int srcRed = srcColor & 0xf800;
                const int redDelta = (srcRed - dstRed) * activeAlpha;
                int blended = dstColor + ((int)((unsigned int)(redDelta) >> 8) & 0xfffff800);
                const int srcBlue = srcColor & 0x001f;
                const int blendedBlue = blended & 0x001f;
                const int blueDelta = (srcBlue - blendedBlue) * activeAlpha;
                blended
                    += ((int)((unsigned int)(greenDelta) >> 8) & 0xffffffe0) + (int)((unsigned int)(blueDelta) >> 8);
                *dst = (unsigned short)(blended);
                activeAlpha = g_spanActiveConstAlphaBits;
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    } while (--pixelCount != 0);
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanmasked16fromtex16to565
 * @recoil-artifact defines .text recoil:function:0x49c150: zRndr::SpanMasked16FromTex16To565
 *
 * Source-shape evidence: BN samples a nonzero tex16 mask and copies it only
 * for alpha >= 0xfc; the partial-alpha branch emits channel math that collapses
 * to preserving the current destination word.
 * Purpose: Copy nonzero 16-bit texture samples into a 565 destination span.
 */
void __fastcall SpanMasked16FromTex16To565(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned short sourceTexel = texels16[vIndex + uIndex];
        if (sourceTexel != 0 && g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = sourceTexel;
            } else {
                // BN 0x49c1f2 reaches the partial-alpha branch but adds zero,
                // preserving the destination after the source/nonzero gate.
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend565constalphafrompal8
 * @recoil-artifact defines .text recoil:function:0x49c230: zRndr::SpanAlphaBlend565ConstAlphaFromPal8
 *
 * Source-shape evidence: BN uses the sampled pal8 texel for the high-alpha
 * palette copy path, but the partial-alpha path reloads the current destination
 * word and uses that word as the palette index before blending 565 channels.
 * Purpose: Blend palettized texture samples into a 565 span using the active constant alpha.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFromPal8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned char sourceIndex = g_spanActiveTexPixels[vIndex + uIndex];
        if (sourceIndex != 0 && g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = g_spanActiveTexPalette[(short)(sourceIndex)];
            } else {
                const int dstColor = (short)(*dst);
                // BN 0x49c2ba intentionally uses the current destination word
                // as the palette index in this partial-alpha path.
                const int srcColor = g_spanActiveTexPalette[dstColor];
                const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend565fromtex16alpha8
 * @recoil-artifact defines .text recoil:function:0x49c360: zRndr::SpanAlphaBlend565FromTex16Alpha8
 *
 * Source-shape evidence: BN inlines the odd tex16 alpha-map scalar path,
 * duplicates one sampled texel into a packed pair, reduces alpha to five bits,
 * and blends the two-pixel 565 lanes with packed masks.
 * Purpose: Alpha-blend 16-bit texture samples into a 565 span using per-texel alpha.
 */
void __fastcall SpanAlphaBlend565FromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);

    if ((pixelCount & 1) != 0) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourceTexel = texels16[sourceIndex];
            if (alpha >= 0xf8) {
                *dst = sourceTexel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourceTexel;
                const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
                const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    {
        for (int pairCount = pixelCount >> 1; pairCount != 0; --pairCount) {
            const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
            const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
            const int sourceIndex = vIndex + uIndex;
            const int alpha = alphaMap[sourceIndex];
            if (alpha >= 8) {
                const unsigned short sourceTexel = texels16[sourceIndex];
                unsigned int packedPixels = 0;
                if (alpha >= 0xf8) {
                    packedPixels = (unsigned int)(sourceTexel) | ((unsigned int)(sourceTexel) << 16);
                } else {
                    memcpy(&packedPixels, dst, sizeof(packedPixels));
                    const unsigned int sourcePair = (unsigned int)(sourceTexel) | ((unsigned int)(sourceTexel) << 16);
                    const unsigned int alpha5 = (unsigned int)(alpha >> 3);
                    const unsigned int inverseAlpha5 = 0x1fu - alpha5;
                    const unsigned int lowTerms
                        = ((((packedPixels & 0x07e0f81fu) * inverseAlpha5) + ((sourcePair & 0x07e0f81fu) * alpha5))
                              >> 5)
                        & 0x07e0f81fu;
                    const unsigned int highTerms = ((((packedPixels >> 5) & 0x07c0f83fu) * inverseAlpha5)
                                                       + (((sourcePair >> 5) & 0x07c0f83fu) * alpha5))
                        & 0xf81f07e0u;
                    packedPixels = lowTerms | highTerms;
                }

                memcpy(dst, &packedPixels, sizeof(packedPixels));
            }

            texU += g_spanActiveTexUStepFixed20 * 2;
            texV += g_spanActiveTexVStepFixed20 * 2;
            dst += 2;
        }
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend555fromtex16alpha8
 * @recoil-artifact defines .text recoil:function:0x49c560: zRndr::SpanAlphaBlend555FromTex16Alpha8
 *
 * Source-shape evidence: BN matches the tex16 alpha-map odd/pair loop with
 * 555-specific red and green masks in the packed two-pixel blend.
 * Purpose: Alpha-blend 16-bit texture samples into a 555 span using per-texel alpha.
 */
void __fastcall SpanAlphaBlend555FromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);

    if ((pixelCount & 1) != 0) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourceTexel = texels16[sourceIndex];
            if (alpha >= 0xf8) {
                *dst = sourceTexel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourceTexel;
                const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    {
        for (int pairCount = pixelCount >> 1; pairCount != 0; --pairCount) {
            const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
            const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
            const int sourceIndex = vIndex + uIndex;
            const int alpha = alphaMap[sourceIndex];
            if (alpha >= 8) {
                const unsigned short sourceTexel = texels16[sourceIndex];
                unsigned int packedPixels = 0;
                if (alpha >= 0xf8) {
                    packedPixels = (unsigned int)(sourceTexel) | ((unsigned int)(sourceTexel) << 16);
                } else {
                    memcpy(&packedPixels, dst, sizeof(packedPixels));
                    const unsigned int sourcePair = (unsigned int)(sourceTexel) | ((unsigned int)(sourceTexel) << 16);
                    const unsigned int alpha5 = (unsigned int)(alpha >> 3);
                    const unsigned int inverseAlpha5 = 0x1fu - alpha5;
                    const unsigned int lowTerms
                        = ((((packedPixels & 0x03e07c1fu) * inverseAlpha5) + ((sourcePair & 0x03e07c1fu) * alpha5))
                              >> 5)
                        & 0x03e07c1fu;
                    const unsigned int highTerms = ((((packedPixels >> 5) & 0x03e0f81fu) * inverseAlpha5)
                                                       + (((sourcePair >> 5) & 0x03e0f81fu) * alpha5))
                        & 0x7c1f03e0u;
                    packedPixels = highTerms | lowTerms;
                }

                memcpy(dst, &packedPixels, sizeof(packedPixels));
            }

            texU += g_spanActiveTexUStepFixed20 * 2;
            texV += g_spanActiveTexVStepFixed20 * 2;
            dst += 2;
        }
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend565constalphafromtex16
 * @recoil-artifact defines .text recoil:function:0x49c760: zRndr::SpanAlphaBlend565ConstAlphaFromTex16
 *
 * Source-shape evidence: BN samples a 16-bit texel through the active U/V
 * masks, skips only when gRndr_ActiveConstAlphaBits <= 3, copies for alpha
 * >= 0xfc, and otherwise blends 565 channels toward the texel.
 * Purpose: Blend 16-bit texture samples into a 565 span using the active constant alpha.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFromTex16(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int srcColor = (short)(texels16[vIndex + uIndex]);
        if (g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = (unsigned short)(srcColor);
            } else {
                const int dstColor = (short)(*dst);
                const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend555constalphafromtex16
 * @recoil-artifact defines .text recoil:function:0x49c860: zRndr::SpanAlphaBlend555ConstAlphaFromTex16
 *
 * Source-shape evidence: BN matches the tex16 constant-alpha loop shape with a
 * stricter alpha > 7 gate and 555 red/green/blue channel masks.
 * Purpose: Blend 16-bit texture samples into a 555 span using the active constant alpha.
 */
void __fastcall SpanAlphaBlend555ConstAlphaFromTex16(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int srcColor = (short)(texels16[vIndex + uIndex]);
        if (g_spanActiveConstAlphaBits > 7) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = (unsigned short)(srcColor);
            } else {
                const int dstColor = (short)(*dst);
                const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend565constalphafromtex16alpha8
 * @recoil-artifact defines .text recoil:function:0x49c970: zRndr::SpanAlphaBlend565ConstAlphaFromTex16Alpha8
 *
 * Source-shape evidence: BN uses the same active U/V index for tex16 and
 * alpha-map reads, scales the alpha byte by the float stored in
 * gRndr_ActiveConstAlphaBits, skips alpha <= 3, copies for alpha >= 0xfc, and
 * otherwise blends 565 channels.
 * Purpose: Blend 16-bit texture samples into a 565 span using scaled alpha-map values.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);
    float alphaScale = 0.0f;
    memcpy(&alphaScale, &g_spanActiveConstAlphaBits, sizeof(alphaScale));

    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const double alphaScaled = (double)(alphaMap[sourceIndex]) * (double)(alphaScale);
        const double alphaFixedBits = alphaScaled - -6755399441055744.0;
        const int alpha = *(const int*)(&alphaFixedBits);
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourceTexel;
                const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
                const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend555constalphafromtex16alpha8
 * @recoil-artifact defines .text recoil:function:0x49ca90: zRndr::SpanAlphaBlend555ConstAlphaFromTex16Alpha8
 *
 * Source-shape evidence: BN matches the tex16 alpha-map scaling loop with a
 * 555-specific alpha > 7 gate and 555 channel masks.
 * Purpose: Blend 16-bit texture samples into a 555 span using scaled alpha-map values.
 */
void __fastcall SpanAlphaBlend555ConstAlphaFromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);
    float alphaScale = 0.0f;
    memcpy(&alphaScale, &g_spanActiveConstAlphaBits, sizeof(alphaScale));

    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const double alphaScaled = (double)(alphaMap[sourceIndex]) * (double)(alphaScale);
        const double alphaFixedBits = alphaScaled - -6755399441055744.0;
        const int alpha = *(const int*)(&alphaFixedBits);
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourceTexel;
                const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
                const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
                const int blueDelta = (((srcColor & 0x001f) - (dstColor & 0x001f)) * alpha) >> 8;
                *dst = (unsigned short)(dstColor + (redDelta & 0xfffffc00) + (greenDelta & 0xffffffe0) + blueDelta);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend565mmxfromtex16alpha8
 * @recoil-artifact defines .text recoil:function:0x49cbb0: zRndr::SpanAlphaBlend565MmxFromTex16Alpha8
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-alpha-blend-565-mmx-from-tex16-alpha8
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-alpha-blend-565-mmx-from-tex16-alpha8
 *
 * BN retail evidence: BN builds paired U/V indices with the MMX mask and
 * step globals, stages sampled tex16 pixels and alpha bytes in a stack scratch
 * area, blends packed groups, then runs a scalar tail with the 565 gates.
 * Source-shape evidence: the VC5 x86 path keeps the retail MMX paired-index
 * gather and quad blend over stack texel/alpha scratch; portable builds keep
 * the behavior/data-equivalent scalar fallback.
 * Purpose: Blend tex16 alpha-map samples into a 565 span using the MMX-selected path shape.
 */
void __fastcall SpanAlphaBlend565MmxFromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM)
    unsigned short texelScratch[1024];
    unsigned short alphaScratch[1024];
    unsigned short* texelScratchBase = texelScratch;
    unsigned short* alphaScratchBase = alphaScratch;
    const int pairCount = pixelCount >> 1;
    const int pairPixels = pairCount << 1;

    g_mmxVPair.hi = texV;
    g_mmxVPair.lo = texV + g_spanActiveTexVStepFixed20;
    g_mmxUPair.hi = texU;
    g_mmxUPair.lo = texU + g_spanActiveTexUStepFixed20;
    g_mmxVStepDup2.lo = g_spanActiveTexVStepFixed20 * 2;
    g_mmxVStepDup2.hi = g_spanActiveTexVStepFixed20 * 2;
    g_mmxUStepDup2.lo = g_spanActiveTexUStepFixed20 * 2;
    g_mmxUStepDup2.hi = g_spanActiveTexUStepFixed20 * 2;

    if (pairCount != 0) {
        unsigned short* alphaScratchEnd = alphaScratchBase + pairPixels;
        __asm {
            mov eax, pairCount
            mov esi, texels16
            mov edi, texelScratchBase
            lea edi, [edi+eax*4]
            neg eax
            movq mm0, qword ptr [g_mmxVPair]
            movq mm1, qword ptr [g_mmxUPair]
            movq mm4, qword ptr [g_mmxVMask]
            movq mm5, qword ptr [g_mmxUMask]
            movq mm6, qword ptr [g_mmxVStepDup2]
            movq mm7, qword ptr [g_mmxUStepDup2]
            xor edx, edx

        zRndr_span_alpha565_tex16_alpha8_gather_loop:
            movq mm2, mm0
            movq mm3, mm1
            pand mm2, mm4
            pand mm3, mm5
            psrld mm2, qword ptr [g_mmxVShiftCounts]
            paddd mm0, mm6
            psrld mm3, 14h
            paddd mm1, mm7
            paddd mm2, mm3
            movd ebx, mm2
            psrlq mm2, 20h
            mov cx, word ptr [esi+ebx*2]
            mov edx, alphaMap
            mov dl, byte ptr [edx+ebx]
            and edx, 0ffh
            movd ebx, mm2
            shl ecx, 10h
            shl edx, 10h
            inc eax
            mov cx, word ptr [esi+ebx*2]
            mov esi, alphaMap
            mov dl, byte ptr [esi+ebx]
            mov esi, alphaScratchEnd
            mov dword ptr [edi+eax*4-4], ecx
            mov dword ptr [esi+eax*4-4], edx
            mov esi, texels16
            jne zRndr_span_alpha565_tex16_alpha8_gather_loop
        }
    }

    if ((pixelCount & 1) != 0) {
        const int tailTexU = texU + pairPixels * g_spanActiveTexUStepFixed20;
        const int tailTexV = texV + pairPixels * g_spanActiveTexVStepFixed20;
        const int vIndex = (tailTexV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (tailTexU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        texelScratch[pairPixels] = texels16[sourceIndex];
        alphaScratch[pairPixels] = (unsigned char)(alphaMap[sourceIndex]);
    }

    const int quadPixels = pixelCount & ~3;
    const int quadCount = pixelCount >> 2;
    if (quadCount != 0) {
        __asm {
            mov eax, quadCount
            mov esi, texelScratchBase
            mov edi, dst
            lea esi, [esi+eax*8]
            lea edi, [edi+eax*8]
            neg eax
            movq mm0, qword ptr [esi+eax*8]
            movq mm1, mm0

        zRndr_span_alpha565_tex16_alpha8_blend_loop:
            movq mm7, qword ptr [edi+eax*8]
            movq mm2, mm0
            pand mm1, qword ptr [g_mmxMaskGreenBits]
            movq mm4, mm7
            pand mm2, qword ptr [g_mmxMaskBlueBits]
            psrlw mm0, 0bh
            movq mm5, mm7
            movq mm6, mm7
            pand mm5, qword ptr [g_mmxMaskGreenBits]
            psrlw mm1, 5
            pand mm6, qword ptr [g_mmxMaskBlueBits]
            psrlw mm4, 0bh
            mov ebx, alphaScratchBase
            movq mm3, qword ptr [ebx+eax*8]
            psrlw mm5, 5
            psubw mm0, mm4
            psubw mm1, mm5
            pmullw mm0, mm3
            psubw mm2, mm6
            pmullw mm1, mm3
            inc eax
            pmullw mm2, mm3
            psllw mm0, 3
            pand mm0, qword ptr [g_mmxMaskRedPacked]
            psraw mm1, 3
            pand mm1, qword ptr [g_mmxMaskGreenPacked]
            paddw mm7, mm0
            psraw mm2, 8
            paddw mm7, mm1
            movq mm0, qword ptr [esi+eax*8]
            paddw mm7, mm2
            movq qword ptr [edi+eax*8-8], mm7
            movq mm1, mm0
            jne zRndr_span_alpha565_tex16_alpha8_blend_loop
        }
        dst += quadPixels;
    }

    for (int i = quadPixels; i < pixelCount; ++i) {
        const int alpha = alphaScratch[i];
        const unsigned short sourceTexel = texelScratch[i];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourceTexel;
                const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
                const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        ++dst;
    }
#else
    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        *dst = BlendPixel565Alpha8(*dst, texels16[sourceIndex], alphaMap[sourceIndex]);

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i_1490 = quadPixels; i_1490 < pixelCount; ++i_1490) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel565Alpha8(*dst, sourceTexel, alpha);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
#endif
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend555mmxfromtex16alpha8
 * @recoil-artifact defines .text recoil:function:0x49cea0: zRndr::SpanAlphaBlend555MmxFromTex16Alpha8
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-alpha-blend-555-mmx-from-tex16-alpha8
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-alpha-blend-555-mmx-from-tex16-alpha8
 *
 * BN retail evidence: BN matches the 565 MMX alpha-map staging loop but
 * uses the 555 red/green masks and an alpha > 7 scalar-tail gate.
 * Source-shape evidence: the VC5 x86 path keeps the retail MMX paired-index
 * gather and quad blend over stack texel/alpha scratch; portable builds keep
 * the behavior/data-equivalent scalar fallback.
 * Purpose: Blend tex16 alpha-map samples into a 555 span using the MMX-selected path shape.
 */
void __fastcall SpanAlphaBlend555MmxFromTex16Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM)
    unsigned short texelScratch[1024];
    unsigned short alphaScratch[1024];
    unsigned short* texelScratchBase = texelScratch;
    unsigned short* alphaScratchBase = alphaScratch;
    const int pairCount = pixelCount >> 1;
    const int pairPixels = pairCount << 1;

    g_mmxVPair.hi = texV;
    g_mmxVPair.lo = texV + g_spanActiveTexVStepFixed20;
    g_mmxUPair.hi = texU;
    g_mmxUPair.lo = texU + g_spanActiveTexUStepFixed20;
    g_mmxVStepDup2.lo = g_spanActiveTexVStepFixed20 * 2;
    g_mmxVStepDup2.hi = g_spanActiveTexVStepFixed20 * 2;
    g_mmxUStepDup2.lo = g_spanActiveTexUStepFixed20 * 2;
    g_mmxUStepDup2.hi = g_spanActiveTexUStepFixed20 * 2;

    if (pairCount != 0) {
        unsigned short* alphaScratchEnd = alphaScratchBase + pairPixels;
        __asm {
            mov eax, pairCount
            mov esi, texels16
            mov edi, texelScratchBase
            lea edi, [edi+eax*4]
            neg eax
            movq mm0, qword ptr [g_mmxVPair]
            movq mm1, qword ptr [g_mmxUPair]
            movq mm4, qword ptr [g_mmxVMask]
            movq mm5, qword ptr [g_mmxUMask]
            movq mm6, qword ptr [g_mmxVStepDup2]
            movq mm7, qword ptr [g_mmxUStepDup2]
            xor edx, edx

        zRndr_span_alpha555_tex16_alpha8_gather_loop:
            movq mm2, mm0
            movq mm3, mm1
            pand mm2, mm4
            pand mm3, mm5
            psrld mm2, qword ptr [g_mmxVShiftCounts]
            paddd mm0, mm6
            psrld mm3, 14h
            paddd mm1, mm7
            paddd mm2, mm3
            movd ebx, mm2
            psrlq mm2, 20h
            mov cx, word ptr [esi+ebx*2]
            mov edx, alphaMap
            mov dl, byte ptr [edx+ebx]
            and edx, 0ffh
            movd ebx, mm2
            shl ecx, 10h
            shl edx, 10h
            inc eax
            mov cx, word ptr [esi+ebx*2]
            mov esi, alphaMap
            mov dl, byte ptr [esi+ebx]
            mov esi, alphaScratchEnd
            mov dword ptr [edi+eax*4-4], ecx
            mov dword ptr [esi+eax*4-4], edx
            mov esi, texels16
            jne zRndr_span_alpha555_tex16_alpha8_gather_loop
        }
    }

    if ((pixelCount & 1) != 0) {
        const int tailTexU = texU + pairPixels * g_spanActiveTexUStepFixed20;
        const int tailTexV = texV + pairPixels * g_spanActiveTexVStepFixed20;
        const int vIndex = (tailTexV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (tailTexU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        texelScratch[pairPixels] = texels16[sourceIndex];
        alphaScratch[pairPixels] = (unsigned char)(alphaMap[sourceIndex]);
    }

    const int quadPixels = pixelCount & ~3;
    const int quadCount = pixelCount >> 2;
    if (quadCount != 0) {
        __asm {
            mov eax, quadCount
            mov esi, texelScratchBase
            mov edi, dst
            lea esi, [esi+eax*8]
            lea edi, [edi+eax*8]
            neg eax
            movq mm0, qword ptr [esi+eax*8]
            movq mm1, mm0

        zRndr_span_alpha555_tex16_alpha8_blend_loop:
            movq mm7, qword ptr [edi+eax*8]
            movq mm2, mm0
            pand mm1, qword ptr [g_mmxMaskGreenBits]
            movq mm4, mm7
            pand mm2, qword ptr [g_mmxMaskBlueBits]
            psrlw mm0, 0ah
            movq mm5, mm7
            movq mm6, mm7
            pand mm5, qword ptr [g_mmxMaskGreenBits]
            psrlw mm1, 5
            pand mm6, qword ptr [g_mmxMaskBlueBits]
            psrlw mm4, 0ah
            mov ebx, alphaScratchBase
            movq mm3, qword ptr [ebx+eax*8]
            psrlw mm5, 5
            psubw mm0, mm4
            psubw mm1, mm5
            pmullw mm0, mm3
            psubw mm2, mm6
            pmullw mm1, mm3
            inc eax
            pmullw mm2, mm3
            psllw mm0, 2
            pand mm0, qword ptr [g_mmxMaskRedPacked]
            psraw mm1, 3
            pand mm1, qword ptr [g_mmxMaskGreenPacked]
            paddw mm7, mm0
            psraw mm2, 8
            paddw mm7, mm1
            movq mm0, qword ptr [esi+eax*8]
            paddw mm7, mm2
            movq qword ptr [edi+eax*8-8], mm7
            movq mm1, mm0
            jne zRndr_span_alpha555_tex16_alpha8_blend_loop
        }
        dst += quadPixels;
    }

    for (int i = quadPixels; i < pixelCount; ++i) {
        const int alpha = alphaScratch[i];
        const unsigned short sourceTexel = texelScratch[i];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourceTexel;
                const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        ++dst;
    }
#else
    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        *dst = BlendPixel555Alpha8(*dst, texels16[sourceIndex], alphaMap[sourceIndex]);

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i_1529 = quadPixels; i_1529 < pixelCount; ++i_1529) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourceTexel = texels16[sourceIndex];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourceTexel;
            } else {
                *dst = BlendPixel555Alpha8(*dst, sourceTexel, alpha);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
#endif
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend565frompal8alpha8
 * @recoil-artifact defines .text recoil:function:0x49d1a0: zRndr::SpanAlphaBlend565FromPal8Alpha8
 *
 * Source-shape evidence: BN expands each sampled pal8 texel through the active
 * palette before the odd scalar and packed two-pixel 565 alpha-map blend.
 * Purpose: Alpha-blend palettized texture samples into a 565 span using per-texel alpha.
 */
void __fastcall SpanAlphaBlend565FromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned char* texels8 = g_spanActiveTexPixels;
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);
    const unsigned short* palette = g_spanActiveTexPalette;

    if ((pixelCount & 1) != 0) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                *dst = sourcePixel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourcePixel;
                const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
                const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i = pixelCount >> 1; i != 0; --i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                dst[0] = sourcePixel;
                dst[1] = sourcePixel;
            } else {
                unsigned int packedPixels = 0;
                memcpy(&packedPixels, dst, sizeof(packedPixels));
                const unsigned int sourcePair = (unsigned int)(sourcePixel) | ((unsigned int)(sourcePixel) << 16);
                const unsigned int alpha5 = (unsigned int)(alpha >> 3);
                const unsigned int inverseAlpha5 = 0x1fu - alpha5;
                const unsigned int lowTerms
                    = ((((packedPixels & 0x07e0f81fu) * inverseAlpha5) + ((sourcePair & 0x07e0f81fu) * alpha5)) >> 5)
                    & 0x07e0f81fu;
                const unsigned int highTerms = ((((packedPixels >> 5) & 0x07c0f83fu) * inverseAlpha5)
                                                   + (((sourcePair >> 5) & 0x07c0f83fu) * alpha5))
                    & 0xf81f07e0u;
                packedPixels = lowTerms | highTerms;
                memcpy(dst, &packedPixels, sizeof(packedPixels));
            }
        }

        texU += 2 * g_spanActiveTexUStepFixed20;
        texV += 2 * g_spanActiveTexVStepFixed20;
        dst += 2;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend555frompal8alpha8
 * @recoil-artifact defines .text recoil:function:0x49d3b0: zRndr::SpanAlphaBlend555FromPal8Alpha8
 *
 * Source-shape evidence: BN matches the pal8 alpha-map odd/pair loop with
 * active-palette expansion and 555-specific packed blend masks.
 * Purpose: Alpha-blend palettized texture samples into a 555 span using per-texel alpha.
 */
void __fastcall SpanAlphaBlend555FromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned char* texels8 = g_spanActiveTexPixels;
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);
    const unsigned short* palette = g_spanActiveTexPalette;

    if ((pixelCount & 1) != 0) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                *dst = sourcePixel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourcePixel;
                const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i = pixelCount >> 1; i != 0; --i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const int alpha = alphaMap[sourceIndex];
        if (alpha >= 8) {
            const unsigned short sourcePixel = palette[texels8[sourceIndex]];
            if (alpha >= 0xf8) {
                dst[0] = sourcePixel;
                dst[1] = sourcePixel;
            } else {
                unsigned int packedPixels = 0;
                memcpy(&packedPixels, dst, sizeof(packedPixels));
                const unsigned int sourcePair = (unsigned int)(sourcePixel) | ((unsigned int)(sourcePixel) << 16);
                const unsigned int alpha5 = (unsigned int)(alpha >> 3);
                const unsigned int inverseAlpha5 = 0x1fu - alpha5;
                const unsigned int lowTerms
                    = ((((packedPixels & 0x03e07c1fu) * inverseAlpha5) + ((sourcePair & 0x03e07c1fu) * alpha5)) >> 5)
                    & 0x03e07c1fu;
                const unsigned int highTerms = ((((packedPixels >> 5) & 0x03e0f81fu) * inverseAlpha5)
                                                   + (((sourcePair >> 5) & 0x03e0f81fu) * alpha5))
                    & 0x7c1f03e0u;
                packedPixels = highTerms | lowTerms;
                memcpy(dst, &packedPixels, sizeof(packedPixels));
            }
        }

        texU += 2 * g_spanActiveTexUStepFixed20;
        texV += 2 * g_spanActiveTexVStepFixed20;
        dst += 2;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend565constalphafastfrompal8
 * @recoil-artifact defines .text recoil:function:0x49d5c0: zRndr::SpanAlphaBlend565ConstAlphaFastFromPal8
 *
 * Source-shape evidence: BN samples an 8-bit texel, expands it through the
 * active palette before the alpha gate, skips only when alpha <= 3, copies for
 * alpha >= 0xfc, and otherwise blends 565 channels toward the palette color.
 * Purpose: Blend palettized texture samples into a 565 span using fast constant alpha.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFastFromPal8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned char sourceIndex = g_spanActiveTexPixels[vIndex + uIndex];
        const int srcColor = (short)(g_spanActiveTexPalette[sourceIndex]);
        if (g_spanActiveConstAlphaBits > 3) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = (unsigned short)(srcColor);
            } else {
                const int dstColor = (short)(*dst);
                const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend555constalphafastfrompal8
 * @recoil-artifact defines .text recoil:function:0x49d6e0: zRndr::SpanAlphaBlend555ConstAlphaFastFromPal8
 *
 * Source-shape evidence: BN matches the fast pal8 constant-alpha loop shape
 * with alpha <= 7 skip behavior and 555 channel masks.
 * Purpose: Blend palettized texture samples into a 555 span using fast constant alpha.
 */
void __fastcall SpanAlphaBlend555ConstAlphaFastFromPal8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const unsigned char sourceIndex = g_spanActiveTexPixels[vIndex + uIndex];
        const int srcColor = (short)(g_spanActiveTexPalette[sourceIndex]);
        if (g_spanActiveConstAlphaBits > 7) {
            if (g_spanActiveConstAlphaBits >= 0xfc) {
                *dst = (unsigned short)(srcColor);
            } else {
                const int dstColor = (short)(*dst);
                const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * g_spanActiveConstAlphaBits) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * g_spanActiveConstAlphaBits) >> 8;
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * g_spanActiveConstAlphaBits) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend565constalphafrompal8alpha8
 * @recoil-artifact defines .text recoil:function:0x49d810: zRndr::SpanAlphaBlend565ConstAlphaFromPal8Alpha8
 *
 * Source-shape evidence: BN samples pal8 texels and the alpha map through the
 * same active U/V index, expands the texel through the active palette, scales
 * alpha by the float constant-alpha value, and applies the 565 alpha gates.
 * Purpose: Blend palettized texture samples into a 565 span using scaled alpha-map values.
 */
void __fastcall SpanAlphaBlend565ConstAlphaFromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned char* texels8 = g_spanActiveTexPixels;
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);
    const unsigned short* palette = g_spanActiveTexPalette;

    float alphaScale = 0.0f;
    memcpy(&alphaScale, &g_spanActiveConstAlphaBits, sizeof(alphaScale));

    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        const double alphaScaled = (double)(alphaMap[sourceIndex]) * (double)(alphaScale);
        const double alphaFixedBits = alphaScaled - -6755399441055744.0;
        const int alpha = *(const int*)(&alphaFixedBits);
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourcePixel;
                const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
                const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend555constalphafrompal8alpha8
 * @recoil-artifact defines .text recoil:function:0x49d950: zRndr::SpanAlphaBlend555ConstAlphaFromPal8Alpha8
 *
 * Source-shape evidence: BN matches the pal8 alpha-map scaling loop with the
 * active palette expansion and 555-specific alpha > 7 gate.
 * Purpose: Blend palettized texture samples into a 555 span using scaled alpha-map values.
 */
void __fastcall SpanAlphaBlend555ConstAlphaFromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned char* texels8 = g_spanActiveTexPixels;
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);
    const unsigned short* palette = g_spanActiveTexPalette;

    float alphaScale = 0.0f;
    memcpy(&alphaScale, &g_spanActiveConstAlphaBits, sizeof(alphaScale));

    for (int i = 0; i < pixelCount; ++i) {
        const int vIndex = (texV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (texU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        const double alphaScaled = (double)(alphaMap[sourceIndex]) * (double)(alphaScale);
        const double alphaFixedBits = alphaScaled - -6755399441055744.0;
        const int alpha = *(const int*)(&alphaFixedBits);
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourcePixel;
                const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
                const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
                const int blueDelta = (((srcColor & 0x001f) - (dstColor & 0x001f)) * alpha) >> 8;
                *dst = (unsigned short)(dstColor + (redDelta & 0xfffffc00) + (greenDelta & 0xffffffe0) + blueDelta);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend565mmxfrompal8alpha8
 * @recoil-artifact defines .text recoil:function:0x49da80: zRndr::SpanAlphaBlend565MmxFromPal8Alpha8
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-alpha-blend-565-mmx-from-pal8-alpha8
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-alpha-blend-565-mmx-from-pal8-alpha8
 *
 * BN retail evidence: BN stages paired pal8 samples through the active
 * palette, alpha bytes through the active alpha map, and packed 565 blends
 * through the same MMX U/V index body before the scalar tail.
 * Source-shape evidence: the VC5 x86 path keeps the retail MMX paired-index
 * gather, pal8 palette expansion, and quad blend over stack texel/alpha
 * scratch; portable builds keep the behavior/data-equivalent scalar fallback.
 * Purpose: Blend pal8 alpha-map samples into a 565 span using the MMX-selected path shape.
 */
void __fastcall SpanAlphaBlend565MmxFromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned char* texels8 = g_spanActiveTexPixels;
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);
    const unsigned short* palette = g_spanActiveTexPalette;

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM)
    unsigned short texelScratch[1024];
    unsigned short alphaScratch[1024];
    unsigned short* texelScratchBase = texelScratch;
    unsigned short* alphaScratchBase = alphaScratch;
    const int pairCount = pixelCount >> 1;
    const int pairPixels = pairCount << 1;

    g_mmxVPair.hi = texV;
    g_mmxVPair.lo = texV + g_spanActiveTexVStepFixed20;
    g_mmxUPair.hi = texU;
    g_mmxUPair.lo = texU + g_spanActiveTexUStepFixed20;
    g_mmxVStepDup2.lo = g_spanActiveTexVStepFixed20 * 2;
    g_mmxVStepDup2.hi = g_spanActiveTexVStepFixed20 * 2;
    g_mmxUStepDup2.lo = g_spanActiveTexUStepFixed20 * 2;
    g_mmxUStepDup2.hi = g_spanActiveTexUStepFixed20 * 2;

    if (pairCount != 0) {
        unsigned short* alphaScratchEnd = alphaScratchBase + pairPixels;
        __asm {
            mov eax, pairCount
            mov esi, texels8
            mov edi, texelScratchBase
            lea edi, [edi+eax*4]
            neg eax
            movq mm0, qword ptr [g_mmxVPair]
            movq mm1, qword ptr [g_mmxUPair]
            movq mm4, qword ptr [g_mmxVMask]
            movq mm5, qword ptr [g_mmxUMask]
            movq mm6, qword ptr [g_mmxVStepDup2]
            movq mm7, qword ptr [g_mmxUStepDup2]
            xor edx, edx

        zRndr_span_alpha565_pal8_alpha8_gather_loop:
            movq mm2, mm0
            movq mm3, mm1
            pand mm2, mm4
            pand mm3, mm5
            psrld mm2, qword ptr [g_mmxVShiftCounts]
            paddd mm0, mm6
            psrld mm3, 14h
            paddd mm1, mm7
            paddd mm2, mm3
            movd ebx, mm2
            psrlq mm2, 20h
            mov cl, byte ptr [esi+ebx]
            and ecx, 0ffh
            mov edx, palette
            mov cx, word ptr [edx+ecx*2]
            mov edx, alphaMap
            mov dl, byte ptr [edx+ebx]
            and edx, 0ffh
            movd ebx, mm2
            shl ecx, 10h
            shl edx, 10h
            mov esi, alphaMap
            mov dl, byte ptr [esi+ebx]
            mov esi, texels8
            mov bl, byte ptr [esi+ebx]
            and ebx, 0ffh
            mov esi, palette
            mov cx, word ptr [esi+ebx*2]
            inc eax
            mov esi, alphaScratchEnd
            mov dword ptr [edi+eax*4-4], ecx
            mov dword ptr [esi+eax*4-4], edx
            mov esi, texels8
            jne zRndr_span_alpha565_pal8_alpha8_gather_loop
        }
    }

    if ((pixelCount & 1) != 0) {
        const int tailTexU = texU + pairPixels * g_spanActiveTexUStepFixed20;
        const int tailTexV = texV + pairPixels * g_spanActiveTexVStepFixed20;
        const int vIndex = (tailTexV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (tailTexU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        texelScratch[pairPixels] = palette[texels8[sourceIndex]];
        alphaScratch[pairPixels] = (unsigned char)(alphaMap[sourceIndex]);
    }

    const int quadPixels = pixelCount & ~3;
    const int quadCount = pixelCount >> 2;
    if (quadCount != 0) {
        __asm {
            mov eax, quadCount
            mov esi, texelScratchBase
            mov edi, dst
            lea esi, [esi+eax*8]
            lea edi, [edi+eax*8]
            neg eax
            movq mm0, qword ptr [esi+eax*8]
            movq mm1, mm0

        zRndr_span_alpha565_pal8_alpha8_blend_loop:
            movq mm7, qword ptr [edi+eax*8]
            movq mm2, mm0
            pand mm1, qword ptr [g_mmxMaskGreenBits]
            movq mm4, mm7
            pand mm2, qword ptr [g_mmxMaskBlueBits]
            psrlw mm0, 0bh
            movq mm5, mm7
            movq mm6, mm7
            pand mm5, qword ptr [g_mmxMaskGreenBits]
            psrlw mm1, 5
            pand mm6, qword ptr [g_mmxMaskBlueBits]
            psrlw mm4, 0bh
            mov ebx, alphaScratchBase
            movq mm3, qword ptr [ebx+eax*8]
            psrlw mm5, 5
            psubw mm0, mm4
            psubw mm1, mm5
            pmullw mm0, mm3
            psubw mm2, mm6
            pmullw mm1, mm3
            inc eax
            pmullw mm2, mm3
            psllw mm0, 3
            pand mm0, qword ptr [g_mmxMaskRedPacked]
            psraw mm1, 3
            pand mm1, qword ptr [g_mmxMaskGreenPacked]
            paddw mm7, mm0
            psraw mm2, 8
            paddw mm7, mm1
            movq mm0, qword ptr [esi+eax*8]
            paddw mm7, mm2
            movq qword ptr [edi+eax*8-8], mm7
            movq mm1, mm0
            jne zRndr_span_alpha565_pal8_alpha8_blend_loop
        }
        dst += quadPixels;
    }

    for (int i = quadPixels; i < pixelCount; ++i) {
        const int alpha = alphaScratch[i];
        const unsigned short sourcePixel = texelScratch[i];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourcePixel;
                const int greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
                const int redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffff800);
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        ++dst;
    }
#else
    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        *dst = BlendPixel565Alpha8(*dst, palette[texels8[sourceIndex]], alphaMap[sourceIndex]);

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i_1733 = quadPixels; i_1733 < pixelCount; ++i_1733) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        if (alpha > 3) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel565Alpha8(*dst, sourcePixel, alpha);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
#endif
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanalphablend555mmxfrompal8alpha8
 * @recoil-artifact defines .text recoil:function:0x49ddb0: zRndr::SpanAlphaBlend555MmxFromPal8Alpha8
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-alpha-blend-555-mmx-from-pal8-alpha8
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-alpha-blend-555-mmx-from-pal8-alpha8
 *
 * BN retail evidence: BN matches the pal8 MMX alpha-map staging loop but
 * uses the 555 red/green masks and an alpha > 7 scalar-tail gate.
 * Source-shape evidence: the VC5 x86 path keeps the retail MMX paired-index
 * gather, pal8 palette expansion, and quad blend over stack texel/alpha
 * scratch; portable builds keep the behavior/data-equivalent scalar fallback.
 * Purpose: Blend pal8 alpha-map samples into a 555 span using the MMX-selected path shape.
 */
void __fastcall SpanAlphaBlend555MmxFromPal8Alpha8(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned char* texels8 = g_spanActiveTexPixels;
    const unsigned char* alphaMap = (const unsigned char*)(g_spanActiveTexAlphaMap);
    const unsigned short* palette = g_spanActiveTexPalette;

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM)
    unsigned short texelScratch[1024];
    unsigned short alphaScratch[1024];
    unsigned short* texelScratchBase = texelScratch;
    unsigned short* alphaScratchBase = alphaScratch;
    const int pairCount = pixelCount >> 1;
    const int pairPixels = pairCount << 1;

    g_mmxVPair.hi = texV;
    g_mmxVPair.lo = texV + g_spanActiveTexVStepFixed20;
    g_mmxUPair.hi = texU;
    g_mmxUPair.lo = texU + g_spanActiveTexUStepFixed20;
    g_mmxVStepDup2.lo = g_spanActiveTexVStepFixed20 * 2;
    g_mmxVStepDup2.hi = g_spanActiveTexVStepFixed20 * 2;
    g_mmxUStepDup2.lo = g_spanActiveTexUStepFixed20 * 2;
    g_mmxUStepDup2.hi = g_spanActiveTexUStepFixed20 * 2;

    if (pairCount != 0) {
        unsigned short* alphaScratchEnd = alphaScratchBase + pairPixels;
        __asm {
            mov eax, pairCount
            mov esi, texels8
            mov edi, texelScratchBase
            lea edi, [edi+eax*4]
            neg eax
            movq mm0, qword ptr [g_mmxVPair]
            movq mm1, qword ptr [g_mmxUPair]
            movq mm4, qword ptr [g_mmxVMask]
            movq mm5, qword ptr [g_mmxUMask]
            movq mm6, qword ptr [g_mmxVStepDup2]
            movq mm7, qword ptr [g_mmxUStepDup2]
            xor edx, edx

        zRndr_span_alpha555_pal8_alpha8_gather_loop:
            movq mm2, mm0
            movq mm3, mm1
            pand mm2, mm4
            pand mm3, mm5
            psrld mm2, qword ptr [g_mmxVShiftCounts]
            paddd mm0, mm6
            psrld mm3, 14h
            paddd mm1, mm7
            paddd mm2, mm3
            movd ebx, mm2
            psrlq mm2, 20h
            mov cl, byte ptr [esi+ebx]
            and ecx, 0ffh
            mov edx, palette
            mov cx, word ptr [edx+ecx*2]
            mov edx, alphaMap
            mov dl, byte ptr [edx+ebx]
            and edx, 0ffh
            movd ebx, mm2
            shl ecx, 10h
            shl edx, 10h
            mov esi, alphaMap
            mov dl, byte ptr [esi+ebx]
            mov esi, texels8
            mov bl, byte ptr [esi+ebx]
            and ebx, 0ffh
            mov esi, palette
            mov cx, word ptr [esi+ebx*2]
            inc eax
            mov esi, alphaScratchEnd
            mov dword ptr [edi+eax*4-4], ecx
            mov dword ptr [esi+eax*4-4], edx
            mov esi, texels8
            jne zRndr_span_alpha555_pal8_alpha8_gather_loop
        }
    }

    if ((pixelCount & 1) != 0) {
        const int tailTexU = texU + pairPixels * g_spanActiveTexUStepFixed20;
        const int tailTexV = texV + pairPixels * g_spanActiveTexVStepFixed20;
        const int vIndex = (tailTexV & g_spanActiveTexVMask) >> texVShift;
        const int uIndex = (tailTexU >> 20) & g_spanActiveTexUMask;
        const int sourceIndex = vIndex + uIndex;
        texelScratch[pairPixels] = palette[texels8[sourceIndex]];
        alphaScratch[pairPixels] = (unsigned char)(alphaMap[sourceIndex]);
    }

    const int quadPixels = pixelCount & ~3;
    const int quadCount = pixelCount >> 2;
    if (quadCount != 0) {
        __asm {
            mov eax, quadCount
            mov esi, texelScratchBase
            mov edi, dst
            lea esi, [esi+eax*8]
            lea edi, [edi+eax*8]
            neg eax
            movq mm0, qword ptr [esi+eax*8]
            movq mm1, mm0

        zRndr_span_alpha555_pal8_alpha8_blend_loop:
            movq mm7, qword ptr [edi+eax*8]
            movq mm2, mm0
            pand mm1, qword ptr [g_mmxMaskGreenBits]
            movq mm4, mm7
            pand mm2, qword ptr [g_mmxMaskBlueBits]
            psrlw mm0, 0ah
            movq mm5, mm7
            movq mm6, mm7
            pand mm5, qword ptr [g_mmxMaskGreenBits]
            psrlw mm1, 5
            pand mm6, qword ptr [g_mmxMaskBlueBits]
            psrlw mm4, 0ah
            mov ebx, alphaScratchBase
            movq mm3, qword ptr [ebx+eax*8]
            psrlw mm5, 5
            psubw mm0, mm4
            psubw mm1, mm5
            pmullw mm0, mm3
            psubw mm2, mm6
            pmullw mm1, mm3
            inc eax
            pmullw mm2, mm3
            psllw mm0, 2
            pand mm0, qword ptr [g_mmxMaskRedPacked]
            psraw mm1, 3
            pand mm1, qword ptr [g_mmxMaskGreenPacked]
            paddw mm7, mm0
            psraw mm2, 8
            paddw mm7, mm1
            movq mm0, qword ptr [esi+eax*8]
            paddw mm7, mm2
            movq qword ptr [edi+eax*8-8], mm7
            movq mm1, mm0
            jne zRndr_span_alpha555_pal8_alpha8_blend_loop
        }
        dst += quadPixels;
    }

    for (int i = quadPixels; i < pixelCount; ++i) {
        const int alpha = alphaScratch[i];
        const unsigned short sourcePixel = texelScratch[i];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                const int dstColor = (short)(*dst);
                const int srcColor = sourcePixel;
                const int redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
                int blended = dstColor + (redDelta & 0xfffffc00);
                const int greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
                const int blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
                blended += (greenDelta & 0xffffffe0) + blueDelta;
                *dst = (unsigned short)(blended);
            }
        }

        ++dst;
    }
#else
    const int quadPixels = pixelCount & ~3;
    for (int i = 0; i < quadPixels; ++i) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        *dst = BlendPixel555Alpha8(*dst, palette[texels8[sourceIndex]], alphaMap[sourceIndex]);

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }

    for (int i_1773 = quadPixels; i_1773 < pixelCount; ++i_1773) {
        const int sourceIndex = SpanTex16SampleIndex(texU, texV, texVShift, g_spanActiveTexUMask);
        const int alpha = alphaMap[sourceIndex];
        const unsigned short sourcePixel = palette[texels8[sourceIndex]];
        if (alpha > 7) {
            if (alpha >= 0xfc) {
                *dst = sourcePixel;
            } else {
                *dst = BlendPixel555Alpha8(*dst, sourcePixel, alpha);
            }
        }

        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        ++dst;
    }
#endif
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-fogtarget565-setpackedcolorandramp
 * @recoil-artifact defines .text recoil:function:0x49e0e0: zRndr::FogTarget565SetPackedColorAndRamp
 *
 * Inferred placement: renderer span family; the original filename is unresolved.
 * Data evidence: stores RGB565 component fields, writes packedColor16 as a
 * 16-bit field, replicates the packed 565 color, and fills packedColorRamp[31..0].
 * Purpose: Build the packed fog color and ramp table used by 16-bit fog blending.
 */
void __fastcall
FogTarget565SetPackedColorAndRamp(FogParamsPartial* params, int packedRed, int packedGreen, int packedBlue)
{
    const unsigned int packedColor16 = (unsigned int)(packedRed | packedGreen | packedBlue);
    params->packedColorRed = packedRed;
    params->packedColorGreen = packedGreen;
    params->packedColorBlue = packedBlue;
    params->packedColor16 = (unsigned short)(packedColor16);
    params->packedColor16Dup = (int)(packedColor16 | (packedColor16 << 16));

    const unsigned int rampStep = ((unsigned int)(packedRed | packedBlue) << 11) | ((unsigned int)(packedGreen) >> 5);
    unsigned int rampValue = 0;
    for (int i = 31; i >= 0; --i) {
        params->packedColorRamp[i] = (int)(rampValue);
        rampValue += rampStep;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanmmxsetpixelformatmasks
 * @recoil-artifact defines .text recoil:function:0x49e140: zRndr::SpanMmxSetPixelFormatMasks
 *
 * Purpose: Replicate the active 555/565 pixel-format masks into the four-lane MMX span-mask globals.
 */
void __fastcall SpanMmxSetPixelFormatMasks(int greenBits)
{
    short redPacked;
    if (greenBits == 5) {
        g_mmxMaskGreenBits[3] = 0x03e0U;
        g_mmxMaskGreenBits[2] = 0x03e0U;
        g_mmxMaskGreenBits[1] = 0x03e0U;
        g_mmxMaskGreenBits[0] = 0x03e0U;
        g_mmxMaskBlueBits[3] = 0x001fU;
        g_mmxMaskBlueBits[2] = 0x001fU;
        g_mmxMaskBlueBits[1] = 0x001fU;
        g_mmxMaskBlueBits[0] = 0x001fU;
        redPacked = (short)(0xfc00U);
    } else {
        g_mmxMaskGreenBits[3] = 0x07e0U;
        g_mmxMaskGreenBits[2] = 0x07e0U;
        g_mmxMaskGreenBits[1] = 0x07e0U;
        g_mmxMaskGreenBits[0] = 0x07e0U;
        g_mmxMaskBlueBits[3] = 0x001fU;
        g_mmxMaskBlueBits[2] = 0x001fU;
        g_mmxMaskBlueBits[1] = 0x001fU;
        g_mmxMaskBlueBits[0] = 0x001fU;
        redPacked = (short)(0xf800U);
    }

    g_mmxMaskRedPacked[3] = (unsigned short)(redPacked);
    g_mmxMaskRedPacked[2] = (unsigned short)(redPacked);
    g_mmxMaskRedPacked[1] = (unsigned short)(redPacked);
    g_mmxMaskRedPacked[0] = (unsigned short)(redPacked);
    int greenPacked = -32;
    g_mmxMaskGreenPacked[3] = greenPacked;
    g_mmxMaskGreenPacked[2] = greenPacked;
    g_mmxMaskGreenPacked[1] = greenPacked;
    g_mmxMaskGreenPacked[0] = greenPacked;
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-fogblendspan565scalar
 * @recoil-artifact defines .text recoil:function:0x49e200: zRndr::FogBlendSpan565Scalar
 *
 * Purpose: Blend a 565 span with the active fog color using scalar pair processing.
 */
void __fastcall
FogBlendSpan565Scalar(unsigned short* pixels, int pixelCount, int fogCoordFixed24, int fogCoordStepFixed24)
{
    unsigned int fogCoord = (unsigned int)(fogCoordFixed24);
    const unsigned int fogStep = (unsigned int)(fogCoordStepFixed24);
    unsigned int pairCount = (unsigned int)(pixelCount) >> 1;

    if ((pixelCount & 1) != 0) {
        if ((int)(fogCoord) >= 0x1000000) {
            *pixels = (unsigned short)(g_fogParamsActive.packedColor16);
        } else if ((int)(fogCoord) >= 0x80000) {
            const unsigned int rampIndex = (0x1000000u - fogCoord) >> 19;
            const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
            const unsigned int pixel = *pixels;
            const unsigned int green = ((((pixel & 0x07e0u) >> 5) * rampIndex) + rampValue) & 0x07e0u;
            const unsigned int rotatedRamp = (rampValue >> 11) | (rampValue << 21);
            const unsigned int redBlue = (((pixel & 0xf81fu) * rampIndex + rotatedRamp) >> 5) & 0xf81fu;
            *pixels = (unsigned short)(green + redBlue);
        }
        ++pixels;
        fogCoord += fogStep;
    }

    const unsigned int pairFogStep = fogStep + fogStep;
    while (pairCount != 0) {
        const unsigned int packedPixels = (unsigned int)(pixels[0]) | ((unsigned int)(pixels[1]) << 16);
        unsigned int blended = packedPixels;
        if ((int)(fogCoord) >= 0x1000000) {
            blended = (unsigned int)(g_fogParamsActive.packedColor16Dup);
        } else if ((int)(fogCoord) >= 0x80000) {
            const unsigned int rampIndex = (0x1000000u - fogCoord) >> 19;
            const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
            const unsigned int green = ((((packedPixels & 0xf81f07e0u) >> 5) * rampIndex) + rampValue) & 0xf81f07e0u;
            const unsigned int rotatedRamp = (rampValue >> 11) | (rampValue << 21);
            const unsigned int redBlue = (((packedPixels & 0x07e0f81fu) * rampIndex + rotatedRamp) >> 5) & 0x07e0f81fu;
            blended = green + redBlue;
        }
        pixels[0] = (unsigned short)(blended);
        pixels[1] = (unsigned short)(blended >> 16);

        pixels += 2;
        fogCoord += pairFogStep;
        --pairCount;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-fogblendspan555scalar
 * @recoil-artifact defines .text recoil:function:0x49e300: zRndr::FogBlendSpan555Scalar
 *
 * Purpose: Blend a 555 span with the active fog color using scalar pair processing.
 */
void __fastcall
FogBlendSpan555Scalar(unsigned short* pixels, int pixelCount, int fogCoordFixed24, int fogCoordStepFixed24)
{
    unsigned int fogCoord = (unsigned int)(fogCoordFixed24);
    const unsigned int fogStep = (unsigned int)(fogCoordStepFixed24);
    unsigned int pairCount = (unsigned int)(pixelCount) >> 1;

    if ((pixelCount & 1) != 0) {
        if ((int)(fogCoord) >= 0x1000000) {
            *pixels = (unsigned short)(g_fogParamsActive.packedColor16);
        } else if ((int)(fogCoord) >= 0x80000) {
            const unsigned int rampIndex = (0x1000000u - fogCoord) >> 19;
            const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
            const unsigned int pixel = *pixels;
            const unsigned int green = ((((pixel & 0x03e0u) >> 5) * rampIndex) + rampValue) & 0x03e0u;
            const unsigned int rotatedRamp = (rampValue >> 11) | (rampValue << 21);
            const unsigned int redBlue = (((pixel & 0x7c1fu) * rampIndex + rotatedRamp) >> 5) & 0x7c1fu;
            *pixels = (unsigned short)(green + redBlue);
        }
        ++pixels;
        fogCoord += fogStep;
    }

    const unsigned int pairFogStep = fogStep + fogStep;
    while (pairCount != 0) {
        const unsigned int packedPixels = (unsigned int)(pixels[0]) | ((unsigned int)(pixels[1]) << 16);
        unsigned int blended = packedPixels;
        if ((int)(fogCoord) >= 0x1000000) {
            blended = (unsigned int)(g_fogParamsActive.packedColor16Dup);
        } else if ((int)(fogCoord) >= 0x80000) {
            const unsigned int rampIndex = (0x1000000u - fogCoord) >> 19;
            const unsigned int rampValue = (unsigned int)(g_fogParamsActive.packedColorRamp[rampIndex]);
            const unsigned int green = ((((packedPixels & 0x7c1f03e0u) >> 5) * rampIndex) + rampValue) & 0x7c1f03e0u;
            const unsigned int rotatedRamp = (rampValue >> 11) | (rampValue << 21);
            const unsigned int redBlue = (((packedPixels & 0x03e07c1fu) * rampIndex + rotatedRamp) >> 5) & 0x03e07c1fu;
            blended = green + redBlue;
        }
        pixels[0] = (unsigned short)(blended);
        pixels[1] = (unsigned short)(blended >> 16);

        pixels += 2;
        fogCoord += pairFogStep;
        --pairCount;
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-fogblendspan565mmx
 * @recoil-artifact defines .text recoil:function:0x49e400: zRndr::FogBlendSpan565Mmx
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.fog-blend-span-565-mmx
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.fog-blend-span-565-mmx
 *
 * Source-shape evidence: BN retail keeps scalar edge calls in C++ call shape
 * and uses a narrow MMX quad body over gRndr_SpanShade16_MmxFogFactors and
 * the accepted channel-mask vectors. The guarded VC5 x86 path preserves that
 * raw MMX block; the portable fallback remains behavior-only scalar emulation.
 * Purpose: Blend a 565 span through scalar edge handling and the MMX-shaped quad body.
 */
void __fastcall FogBlendSpan565Mmx(unsigned short* pixels, int pixelCount, int fogCoordFixed24, int fogCoordStepFixed24)
{
    unsigned short* cursor = pixels;
    int remaining = pixelCount;
    unsigned int fogCoord = (unsigned int)(fogCoordFixed24);
    const unsigned int fogStep = (unsigned int)(fogCoordStepFixed24);

    int headPixels = (int)((unsigned int)(pixels) & 3u);
    if ((unsigned int)(headPixels) >= (unsigned int)(remaining)) {
        headPixels = remaining;
    }

    if (headPixels != 0) {
        FogBlendSpan565Scalar(cursor, headPixels, (int)(fogCoord), fogCoordStepFixed24);
        cursor += headPixels;
        fogCoord += (unsigned int)(headPixels)*fogStep;
        remaining -= headPixels;
    }

    const int tailPixels = remaining & 3;
    unsigned int quadCount = (unsigned int)(remaining) >> 2;
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM)
    if (quadCount != 0) {
        __asm {
            mov ecx, quadCount
            mov edx, cursor
            mov eax, fogCoord
            mov ebx, fogStep
            add eax, ebx
            mov esi, eax
            add eax, ebx
            shr esi, 10h
            mov edi, eax
            and edi, 0ffff0000h

        zRndr_fog565_mmx_loop:
            add eax, ebx
            or edi, esi
            mov esi, eax
            add eax, ebx
            shr esi, 10h
            mov dword ptr [g_mmxFogFactors], edi
            mov edi, eax
            and edi, 0ffff0000h
            or edi, esi
            mov dword ptr [g_mmxFogFactors+4], edi
            movq mm0, qword ptr [edx]
            movq mm1, mm0
            movq mm2, mm0
            movq mm4, qword ptr [g_mmxFogFactors]
            movq mm3, mm0
            pand mm1, qword ptr [g_mmxMaskGreenBits]
            pand mm2, qword ptr [g_mmxMaskBlueBits]
            psrlw mm0, 0bh
            movq mm5, qword ptr [g_mmxBitsRed255]
            psrlw mm1, 5
            movq mm6, qword ptr [g_mmxBitsGreen255]
            psubsw mm5, mm0
            movq mm7, qword ptr [g_mmxBitsBlue255]
            psubsw mm6, mm1
            psubsw mm7, mm2
            pmullw mm5, mm4
            add eax, ebx
            pmullw mm6, mm4
            mov esi, eax
            pmullw mm7, mm4
            add eax, ebx
            psllw mm5, 3
            shr esi, 10h
            psraw mm6, 3
            mov edi, eax
            psraw mm7, 8
            and edi, 0ffff0000h
            pand mm5, qword ptr [g_mmxMaskRedPacked]
            pand mm6, qword ptr [g_mmxMaskGreenPacked]
            paddw mm3, mm5
            paddw mm3, mm6
            add edx, 8
            paddw mm3, mm7
            dec ecx
            movq qword ptr [edx-8], mm3
            jne zRndr_fog565_mmx_loop

            mov dword ptr [cursor], edx
            mov dword ptr [fogCoord], eax
        }
    }
#else
    while (quadCount != 0) {
        fogCoord += fogStep;
        g_mmxFogFactors[0] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[1] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[2] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[3] = (unsigned short)(fogCoord >> 16);

        cursor[0] = FogBlendMmxLane(cursor[0], g_mmxFogFactors[0], 0, 11, 3);
        cursor[1] = FogBlendMmxLane(cursor[1], g_mmxFogFactors[1], 1, 11, 3);
        cursor[2] = FogBlendMmxLane(cursor[2], g_mmxFogFactors[2], 2, 11, 3);
        cursor[3] = FogBlendMmxLane(cursor[3], g_mmxFogFactors[3], 3, 11, 3);

        fogCoord += fogStep;
        fogCoord += fogStep;
        cursor += 4;
        --quadCount;
    }
#endif

    if (tailPixels != 0) {
        FogBlendSpan565Scalar(cursor, tailPixels, (int)(fogCoord), fogCoordStepFixed24);
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-fogblendspan555mmx
 * @recoil-artifact defines .text recoil:function:0x49e560: zRndr::FogBlendSpan555Mmx
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.fog-blend-span-555-mmx
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.fog-blend-span-555-mmx
 *
 * Source-shape evidence: BN retail matches the 565 scalar-edge/MMX-quad shape
 * with 555 red extraction and packed red terms. The guarded VC5 x86 path keeps
 * the raw MMX block; the portable fallback remains behavior-only scalar emulation.
 * Purpose: Blend a 555 span through scalar edge handling and the MMX-shaped quad body.
 */
void __fastcall FogBlendSpan555Mmx(unsigned short* pixels, int pixelCount, int fogCoordFixed24, int fogCoordStepFixed24)
{
    unsigned short* cursor = pixels;
    int remaining = pixelCount;
    unsigned int fogCoord = (unsigned int)(fogCoordFixed24);
    const unsigned int fogStep = (unsigned int)(fogCoordStepFixed24);

    int headPixels = (int)((unsigned int)(pixels) & 3u);
    if ((unsigned int)(headPixels) >= (unsigned int)(remaining)) {
        headPixels = remaining;
    }

    if (headPixels != 0) {
        FogBlendSpan555Scalar(cursor, headPixels, (int)(fogCoord), fogCoordStepFixed24);
        cursor += headPixels;
        fogCoord += (unsigned int)(headPixels)*fogStep;
        remaining -= headPixels;
    }

    const int tailPixels = remaining & 3;
    unsigned int quadCount = (unsigned int)(remaining) >> 2;
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM)
    if (quadCount != 0) {
        __asm {
            mov ecx, quadCount
            mov edx, cursor
            mov eax, fogCoord
            mov ebx, fogStep
            add eax, ebx
            mov esi, eax
            add eax, ebx
            shr esi, 10h
            mov edi, eax
            and edi, 0ffff0000h

        zRndr_fog555_mmx_loop:
            add eax, ebx
            or edi, esi
            mov esi, eax
            add eax, ebx
            shr esi, 10h
            mov dword ptr [g_mmxFogFactors], edi
            mov edi, eax
            and edi, 0ffff0000h
            or edi, esi
            mov dword ptr [g_mmxFogFactors+4], edi
            movq mm0, qword ptr [edx]
            movq mm1, mm0
            movq mm2, mm0
            movq mm4, qword ptr [g_mmxFogFactors]
            movq mm3, mm0
            pand mm1, qword ptr [g_mmxMaskGreenBits]
            pand mm2, qword ptr [g_mmxMaskBlueBits]
            psrlw mm0, 0ah
            movq mm5, qword ptr [g_mmxBitsRed255]
            psrlw mm1, 5
            movq mm6, qword ptr [g_mmxBitsGreen255]
            psubsw mm5, mm0
            movq mm7, qword ptr [g_mmxBitsBlue255]
            psubsw mm6, mm1
            psubsw mm7, mm2
            pmullw mm5, mm4
            add eax, ebx
            pmullw mm6, mm4
            mov esi, eax
            pmullw mm7, mm4
            add eax, ebx
            psllw mm5, 2
            shr esi, 10h
            psraw mm6, 3
            mov edi, eax
            psraw mm7, 8
            and edi, 0ffff0000h
            pand mm5, qword ptr [g_mmxMaskRedPacked]
            pand mm6, qword ptr [g_mmxMaskGreenPacked]
            paddw mm3, mm5
            paddw mm3, mm6
            add edx, 8
            paddw mm3, mm7
            dec ecx
            movq qword ptr [edx-8], mm3
            jne zRndr_fog555_mmx_loop

            mov dword ptr [cursor], edx
            mov dword ptr [fogCoord], eax
        }
    }
#else
    while (quadCount != 0) {
        fogCoord += fogStep;
        g_mmxFogFactors[0] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[1] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[2] = (unsigned short)(fogCoord >> 16);
        fogCoord += fogStep;
        g_mmxFogFactors[3] = (unsigned short)(fogCoord >> 16);

        cursor[0] = FogBlendMmxLane(cursor[0], g_mmxFogFactors[0], 0, 10, 2);
        cursor[1] = FogBlendMmxLane(cursor[1], g_mmxFogFactors[1], 1, 10, 2);
        cursor[2] = FogBlendMmxLane(cursor[2], g_mmxFogFactors[2], 2, 10, 2);
        cursor[3] = FogBlendMmxLane(cursor[3], g_mmxFogFactors[3], 3, 10, 2);

        fogCoord += fogStep;
        fogCoord += fogStep;
        cursor += 4;
        --quadCount;
    }
#endif

    if (tailPixels != 0) {
        FogBlendSpan555Scalar(cursor, tailPixels, (int)(fogCoord), fogCoordStepFixed24);
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * Inferred implementation placement: the reviewed renderer fog/span family.
 * Source-shape evidence: BN assembly uses a texVShift 10..17 jump table, saves
 * real ESP in gRndr_SavedEspSlot, pivots ESP to gRndr_CurrentSpanBaseAddr +
 * count, samples gRndr_ActiveTexPixels as 16-bit texels, pushes every sampled
 * word backward, and restores ESP at case exit. The guarded VC5 x86 path keeps
 * C++ responsible for dispatch and uses narrow inline asm only for the
 * ESP-pivot write loop; the portable fallback below remains behavior-only.
 * Purpose: Copy 16-bit texels into the active span using the variable-texVShift reverse span contract.
 */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_ESP_PIVOT_RAW_ASM)
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-copy-16-from-tex16-switch-vshift
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-copy-16-from-tex16-switch-vshift recoil:function:0x49e6c0
 *
 * Purpose: Reimplements 0x49e6c0 as a disabled ESP-pivot alternative. Copy 16-bit texels through C++ switch cases with
 * narrow inline asm for the approved zRndr ESP-pivot loop. BN proves the ESP pivot; scoped VC5 C++ forms failed.
 */
void __fastcall SpanCopy16FromTex16SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_copy_tex16_switch_loop10:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0ah
            and eax, 3ffh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            add edi, 2
            push word ptr [ebp+eax*2]
            jne zRndr_span_copy_tex16_switch_loop10
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 11:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_copy_tex16_switch_loop11:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0bh
            and eax, 1ffh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            add edi, 2
            push word ptr [ebp+eax*2]
            jne zRndr_span_copy_tex16_switch_loop11
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 12:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_copy_tex16_switch_loop12:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0ch
            and eax, 0ffh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            add edi, 2
            push word ptr [ebp+eax*2]
            jne zRndr_span_copy_tex16_switch_loop12
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 13:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_copy_tex16_switch_loop13:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0dh
            and eax, 7fh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            add edi, 2
            push word ptr [ebp+eax*2]
            jne zRndr_span_copy_tex16_switch_loop13
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 14:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_copy_tex16_switch_loop14:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0eh
            and eax, 3fh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            add edi, 2
            push word ptr [ebp+eax*2]
            jne zRndr_span_copy_tex16_switch_loop14
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 15:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_copy_tex16_switch_loop15:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 0fh
            and eax, 1fh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            add edi, 2
            push word ptr [ebp+eax*2]
            jne zRndr_span_copy_tex16_switch_loop15
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 16:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_copy_tex16_switch_loop16:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 10h
            and eax, 0fh
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            add edi, 2
            push word ptr [ebp+eax*2]
            jne zRndr_span_copy_tex16_switch_loop16
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 17:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexUStepFixed20]
            neg edi
            mov esi, dword ptr [g_spanActiveTexVMask]
        zRndr_span_copy_tex16_switch_loop17:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, ebx
            shr esi, 11h
            and eax, 7
            add eax, esi
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            add edi, 2
            push word ptr [ebp+eax*2]
            jne zRndr_span_copy_tex16_switch_loop17
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;
    }
}
#else
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zrender.span-copy-16-from-tex16-switch-vshift
 * @recoil-artifact defines .text recoil:function:0x49e6c0: Current C++ definition under the canonical compiler settings.
 *
 * Original function evidence: retail 0x49e6c0 has this portable conditional definition.
 * Purpose: Preserve portable tex16 copy behavior when the ESP-pivot raw-assembly exception is disabled.
 */
void __fastcall SpanCopy16FromTex16SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 11: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 12: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0xff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 13: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x7f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 14: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 15: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 16: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x0f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 17: {
        const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x07) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            *dstEnd = texels16[sourceIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }
    }
}
#endif
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spanmmxsettexuvmasksandvshift
 * @recoil-artifact defines .text recoil:function:0x49ea40: zRndr::SpanMmxSetTexUvMasksAndVShift
 *
 * Purpose: Mirror the active texture U/V masks and selected V shift into the two-lane MMX span globals.
 */
void __fastcall SpanMmxSetTexUvMasksAndVShift(int texVShift)
{
    const int texVMask = g_spanActiveTexVMask;
    g_mmxVShiftCounts.hi = 0;
    g_mmxVMask.hi = texVMask;
    g_mmxVMask.lo = texVMask;

    const int texUMask = g_spanActiveTexUMask << 20;
    g_mmxVShiftCounts.lo = texVShift;
    g_mmxUMask.hi = texUMask;
    g_mmxUMask.lo = texUMask;
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spancopy16fromtex16
 * @recoil-artifact defines .text recoil:function:0x49ea80: zRndr::SpanCopy16FromTex16
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-copy-16-from-tex16
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-copy-16-from-tex16
 *
 * Source-shape evidence: BN handles an optional unaligned leading texel, sets
 * paired MMX U/V and doubled-step scratch globals, samples two tex16 indices
 * per packed loop through the active MMX masks, then writes an odd tail texel.
 * This C++ body keeps scalar edge samples and uses the guarded raw MMX block
 * only for the packed two-pixel loop; the portable fallback remains scalar.
 * Purpose: Copy a 16-bit textured span while priming the paired MMX U/V scratch records.
 */
void __fastcall SpanCopy16FromTex16(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    if (((unsigned int)(dst) & 3u) != 0) {
        const int sourceIndex
            = ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) + ((texU >> 20) & g_spanActiveTexUMask);
        *dst = texels16[sourceIndex];
        ++dst;
        --pixelCount;
        if (pixelCount == 0) {
            return;
        }
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
    }

    g_mmxVPair.hi = texV;
    g_mmxVPair.lo = texV + g_spanActiveTexVStepFixed20;
    g_mmxUPair.hi = texU;
    g_mmxUPair.lo = texU + g_spanActiveTexUStepFixed20;
    g_mmxVStepDup2.lo = g_spanActiveTexVStepFixed20 * 2;
    g_mmxVStepDup2.hi = g_spanActiveTexVStepFixed20 * 2;
    g_mmxUStepDup2.lo = g_spanActiveTexUStepFixed20 * 2;
    g_mmxUStepDup2.hi = g_spanActiveTexUStepFixed20 * 2;

    int pairCount = pixelCount >> 1;
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM)
    const int pairPixels = pairCount << 1;
    if (pairCount != 0) {
        __asm {
            mov eax, pairCount
            mov esi, texels16
            mov edi, dst
            movq mm0, qword ptr [g_mmxVPair]
            movq mm1, qword ptr [g_mmxUPair]
            movq mm4, qword ptr [g_mmxVMask]
            movq mm5, qword ptr [g_mmxUMask]
            movq mm6, qword ptr [g_mmxVStepDup2]
            movq mm7, qword ptr [g_mmxUStepDup2]

        zRndr_span_copy_tex16_mmx_loop:
            movq mm2, mm0
            movq mm3, mm1
            pand mm2, mm4
            pand mm3, mm5
            psrld mm2, qword ptr [g_mmxVShiftCounts]
            paddd mm0, mm6
            psrld mm3, 14h
            paddd mm1, mm7
            paddd mm2, mm3
            movd ebx, mm2
            psrlq mm2, 20h
            xor ecx, ecx
            mov cx, word ptr [esi+ebx*2]
            movd ebx, mm2
            shl ecx, 10h
            xor edx, edx
            mov dx, word ptr [esi+ebx*2]
            or ecx, edx
            mov dword ptr [edi], ecx
            add edi, 4
            dec eax
            jne zRndr_span_copy_tex16_mmx_loop

            mov dword ptr [dst], edi
        }
        texU += pairPixels * g_spanActiveTexUStepFixed20;
        texV += pairPixels * g_spanActiveTexVStepFixed20;
    }
#else
    while (pairCount != 0) {
        const int firstIndex
            = ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) + ((texU >> 20) & g_spanActiveTexUMask);
        const unsigned short first = texels16[firstIndex];
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;

        const int secondIndex
            = ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) + ((texU >> 20) & g_spanActiveTexUMask);
        const unsigned short second = texels16[secondIndex];
        *((unsigned int*)(dst)) = ((unsigned int)(second) << 16) | first;
        dst += 2;
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        --pairCount;
    }
#endif

    if ((pixelCount & 1) != 0) {
        const int sourceIndex
            = ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) + ((texU >> 20) & g_spanActiveTexUMask);
        *dst = texels16[sourceIndex];
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zrender-zrndr-draw-spancopy16fromtex16explicitvshift
 * @recoil-artifact defines .text recoil:function:0x49ec20: zRndr::SpanCopy16FromTex16ExplicitVShift
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-copy-16-from-tex16-explicit-vshift
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-copy-16-from-tex16-explicit-vshift
 *
 * Source-shape evidence: BN matches the generic tex16 copy body with the
 * caller-supplied V shift feeding the MMX packed-index loop and odd tail. This
 * C++ body keeps scalar edge samples and uses the guarded raw MMX block only
 * for the packed two-pixel loop; the portable fallback remains scalar.
 * Purpose: Copy a 16-bit textured span with the caller-supplied V shift and MMX U/V scratch records.
 */
void __fastcall SpanCopy16FromTex16ExplicitVShift(int texU, int texV, int pixelCount, int texVShift)
{
    unsigned short* dst = g_spanCurrentSpanBaseAddr;
    const unsigned short* texels16 = (const unsigned short*)(g_spanActiveTexPixels);
    if (((unsigned int)(dst) & 3u) != 0) {
        const int sourceIndex
            = ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) + ((texU >> 20) & g_spanActiveTexUMask);
        *dst = texels16[sourceIndex];
        ++dst;
        --pixelCount;
        if (pixelCount == 0) {
            return;
        }
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
    }

    g_mmxVPair.hi = texV;
    g_mmxVPair.lo = texV + g_spanActiveTexVStepFixed20;
    g_mmxUPair.hi = texU;
    g_mmxUPair.lo = texU + g_spanActiveTexUStepFixed20;
    g_mmxVStepDup2.lo = g_spanActiveTexVStepFixed20 * 2;
    g_mmxVStepDup2.hi = g_spanActiveTexVStepFixed20 * 2;
    g_mmxUStepDup2.lo = g_spanActiveTexUStepFixed20 * 2;
    g_mmxUStepDup2.hi = g_spanActiveTexUStepFixed20 * 2;

    int pairCount = pixelCount >> 1;
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM)
    const int pairPixels = pairCount << 1;
    if (pairCount != 0) {
        __asm {
            mov eax, pairCount
            mov esi, texels16
            mov edi, dst
            movq mm0, qword ptr [g_mmxVPair]
            movq mm1, qword ptr [g_mmxUPair]
            movq mm4, qword ptr [g_mmxVMask]
            movq mm5, qword ptr [g_mmxUMask]
            movq mm6, qword ptr [g_mmxVStepDup2]
            movq mm7, qword ptr [g_mmxUStepDup2]

        zRndr_span_copy_tex16_explicit_mmx_loop:
            movq mm2, mm0
            movq mm3, mm1
            pand mm2, mm4
            pand mm3, mm5
            psrld mm2, qword ptr [g_mmxVShiftCounts]
            paddd mm0, mm6
            psrld mm3, 14h
            paddd mm1, mm7
            paddd mm2, mm3
            movd ebx, mm2
            psrlq mm2, 20h
            xor ecx, ecx
            mov cx, word ptr [esi+ebx*2]
            movd ebx, mm2
            shl ecx, 10h
            xor edx, edx
            mov dx, word ptr [esi+ebx*2]
            or ecx, edx
            mov dword ptr [edi], ecx
            add edi, 4
            dec eax
            jne zRndr_span_copy_tex16_explicit_mmx_loop

            mov dword ptr [dst], edi
        }
        texU += pairPixels * g_spanActiveTexUStepFixed20;
        texV += pairPixels * g_spanActiveTexVStepFixed20;
    }
#else
    while (pairCount != 0) {
        const int firstIndex
            = ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) + ((texU >> 20) & g_spanActiveTexUMask);
        const unsigned short first = texels16[firstIndex];
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;

        const int secondIndex
            = ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) + ((texU >> 20) & g_spanActiveTexUMask);
        const unsigned short second = texels16[secondIndex];
        *((unsigned int*)(dst)) = ((unsigned int)(second) << 16) | first;
        dst += 2;
        texU += g_spanActiveTexUStepFixed20;
        texV += g_spanActiveTexVStepFixed20;
        --pairCount;
    }
#endif

    if ((pixelCount & 1) != 0) {
        const int sourceIndex
            = ((unsigned int)(texV & g_spanActiveTexVMask) >> texVShift) + ((texU >> 20) & g_spanActiveTexUMask);
        *dst = texels16[sourceIndex];
    }
}
} // namespace zRndr

namespace zRndr {
/**
 * Inferred implementation placement: the reviewed renderer fog/span family.
 * Source-shape evidence: BN assembly/HLIL shows a texVShift 10..17 jump table;
 * each case saves through gRndr_SavedEspSlot, pivots ESP to
 * gRndr_CurrentSpanBaseAddr + count, samples an 8-bit texel from
 * gRndr_ActiveTexPixels, expands it through gRndr_ActiveTexPalette, then pushes
 * the 16-bit palette word backward into the span. The guarded VC5 x86 path
 * keeps C++ responsible for dispatch and uses narrow inline asm only for the
 * ESP-pivot write loop; the portable fallback below remains behavior-only.
 * Purpose: Copy palettized texels into the active 16-bit span using the variable-texVShift reverse span contract.
 */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_ESP_PIVOT_RAW_ASM)
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-copy-16-from-pal8-switch-vshift
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-copy-16-from-pal8-switch-vshift recoil:function:0x49edc0
 *
 * Purpose: Reimplements 0x49edc0 as a disabled ESP-pivot alternative. Copy palettized texels through C++ switch cases
 * with narrow inline asm for the approved zRndr ESP-pivot loop. BN proves the ESP pivot; scoped VC5 C++ forms failed.
 */
void __fastcall SpanCopy16FromPal8SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_copy_pal8_switch_loop10:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0ah
            and eax, 3ffh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_copy_pal8_switch_loop10
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 11:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_copy_pal8_switch_loop11:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0bh
            and eax, 1ffh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_copy_pal8_switch_loop11
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 12:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_copy_pal8_switch_loop12:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0ch
            and eax, 0ffh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_copy_pal8_switch_loop12
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 13:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_copy_pal8_switch_loop13:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0dh
            and eax, 7fh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_copy_pal8_switch_loop13
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 14:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_copy_pal8_switch_loop14:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0eh
            and eax, 3fh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_copy_pal8_switch_loop14
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 15:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_copy_pal8_switch_loop15:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 0fh
            and eax, 1fh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_copy_pal8_switch_loop15
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 16:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_copy_pal8_switch_loop16:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 10h
            and eax, 0fh
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_copy_pal8_switch_loop16
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 17:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            add edi, edi
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esp, edi
            mov esi, dword ptr [g_spanActiveTexVMask]
            mov ebx, dword ptr [g_spanActiveTexPalette]
            neg edi
        zRndr_span_copy_pal8_switch_loop17:
            mov eax, ecx
            and esi, edx
            sar eax, 14h
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            shr esi, 11h
            and eax, 7
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add edi, 2
            push word ptr [ebx+eax*2]
            jne zRndr_span_copy_pal8_switch_loop17
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;
    }
}
#else
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zrender.span-copy-16-from-pal8-switch-vshift
 * @recoil-artifact defines .text recoil:function:0x49edc0: Current C++ definition under the canonical compiler settings.
 *
 * Original function evidence: retail 0x49edc0 has this portable conditional definition.
 * Purpose: Preserve portable palettized copy behavior when the ESP-pivot raw-assembly exception is disabled.
 */
void __fastcall SpanCopy16FromPal8SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 11: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 12: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0xff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 13: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x7f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 14: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 15: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 16: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x0f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }

    case 17: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        int remainingBytes = -pixelCount * 2;
        do {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x07) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            const unsigned char source = g_spanActiveTexPixels[sourceIndex];
            *dstEnd = g_spanActiveTexPalette[source];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
            remainingBytes += 2;
        } while (remainingBytes != 0);
        return;
    }
    }
}
#endif
} // namespace zRndr

namespace zRndr {
/**
 * Inferred implementation placement: the reviewed renderer fog/span family.
 * Source-shape evidence: BN assembly/HLIL shows a texVShift 10..17 jump table;
 * each case saves through gRndr_SavedEspSlot, pivots ESP to
 * gRndr_CurrentSpanBaseAddr + count, samples an 8-bit texel from
 * gRndr_ActiveTexPixels, adds the current shade bucket from
 * gRndr_ActiveShadeFixed16, advances by gRndr_ActiveShadeStepFixed16, then
 * pushes the shade-adjusted 16-bit palette word backward into the span. The
 * guarded VC5 x86 path keeps C++ responsible for dispatch and uses narrow
 * inline asm only for the ESP-pivot shade write loop; the portable fallback
 * below remains behavior-only.
 * Purpose: Shade palettized texels through the active palette and write them into the reverse active span.
 */
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZRNDR_ESP_PIVOT_RAW_ASM)
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zrender.span-shade-16-from-pal8-switch-vshift
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zrender.span-shade-16-from-pal8-switch-vshift recoil:function:0x49f180
 *
 * Purpose: Reimplements 0x49f180 as a disabled ESP-pivot alternative. Shade palettized texels through C++ switch cases
 * with narrow inline asm for the approved zRndr ESP-pivot loop. BN proves the ESP pivot; scoped VC5 C++ forms failed.
 */
void __fastcall SpanShade16FromPal8SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexPalette]
            add esp, edi
        zRndr_span_shade_pal8_switch_loop10:
            mov eax, ecx
            mov esi, dword ptr [g_spanActiveTexVMask]
            sar eax, 14h
            and esi, edx
            shr esi, 0ah
            and eax, 3ffh
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov ebp, dword ptr [g_spanActiveShadeFixed16]
            mov esi, ebp
            and ebp, 0f80000h
            shr ebp, 0bh
            add esi, dword ptr [g_spanActiveShadeStepFixed16]
            add eax, ebp
            mov dword ptr [g_spanActiveShadeFixed16], esi
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            dec edi
            push word ptr [ebx+eax*2]
            jne zRndr_span_shade_pal8_switch_loop10
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 11:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexPalette]
            add esp, edi
        zRndr_span_shade_pal8_switch_loop11:
            mov eax, ecx
            mov esi, dword ptr [g_spanActiveTexVMask]
            sar eax, 14h
            and esi, edx
            shr esi, 0bh
            and eax, 1ffh
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov ebp, dword ptr [g_spanActiveShadeFixed16]
            mov esi, ebp
            and ebp, 0f80000h
            shr ebp, 0bh
            add esi, dword ptr [g_spanActiveShadeStepFixed16]
            add eax, ebp
            mov dword ptr [g_spanActiveShadeFixed16], esi
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            dec edi
            push word ptr [ebx+eax*2]
            jne zRndr_span_shade_pal8_switch_loop11
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 12:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexPalette]
            add esp, edi
        zRndr_span_shade_pal8_switch_loop12:
            mov eax, ecx
            mov esi, dword ptr [g_spanActiveTexVMask]
            sar eax, 14h
            and esi, edx
            shr esi, 0ch
            and eax, 0ffh
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov ebp, dword ptr [g_spanActiveShadeFixed16]
            mov esi, ebp
            and ebp, 0f80000h
            shr ebp, 0bh
            add esi, dword ptr [g_spanActiveShadeStepFixed16]
            add eax, ebp
            mov dword ptr [g_spanActiveShadeFixed16], esi
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            dec edi
            push word ptr [ebx+eax*2]
            jne zRndr_span_shade_pal8_switch_loop12
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 13:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexPalette]
            add esp, edi
        zRndr_span_shade_pal8_switch_loop13:
            mov eax, ecx
            mov esi, dword ptr [g_spanActiveTexVMask]
            sar eax, 14h
            and esi, edx
            shr esi, 0dh
            and eax, 7fh
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov ebp, dword ptr [g_spanActiveShadeFixed16]
            mov esi, ebp
            and ebp, 0f80000h
            shr ebp, 0bh
            add esi, dword ptr [g_spanActiveShadeStepFixed16]
            add eax, ebp
            mov dword ptr [g_spanActiveShadeFixed16], esi
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            dec edi
            push word ptr [ebx+eax*2]
            jne zRndr_span_shade_pal8_switch_loop13
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 14:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexPalette]
            add esp, edi
        zRndr_span_shade_pal8_switch_loop14:
            mov eax, ecx
            mov esi, dword ptr [g_spanActiveTexVMask]
            sar eax, 14h
            and esi, edx
            shr esi, 0eh
            and eax, 3fh
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov ebp, dword ptr [g_spanActiveShadeFixed16]
            mov esi, ebp
            and ebp, 0f80000h
            shr ebp, 0bh
            add esi, dword ptr [g_spanActiveShadeStepFixed16]
            add eax, ebp
            mov dword ptr [g_spanActiveShadeFixed16], esi
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            dec edi
            push word ptr [ebx+eax*2]
            jne zRndr_span_shade_pal8_switch_loop14
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 15:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexPalette]
            add esp, edi
        zRndr_span_shade_pal8_switch_loop15:
            mov eax, ecx
            mov esi, dword ptr [g_spanActiveTexVMask]
            sar eax, 14h
            and esi, edx
            shr esi, 0fh
            and eax, 1fh
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov ebp, dword ptr [g_spanActiveShadeFixed16]
            mov esi, ebp
            and ebp, 0f80000h
            shr ebp, 0bh
            add esi, dword ptr [g_spanActiveShadeStepFixed16]
            add eax, ebp
            mov dword ptr [g_spanActiveShadeFixed16], esi
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            dec edi
            push word ptr [ebx+eax*2]
            jne zRndr_span_shade_pal8_switch_loop15
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 16:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexPalette]
            add esp, edi
        zRndr_span_shade_pal8_switch_loop16:
            mov eax, ecx
            mov esi, dword ptr [g_spanActiveTexVMask]
            sar eax, 14h
            and esi, edx
            shr esi, 10h
            and eax, 0fh
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov ebp, dword ptr [g_spanActiveShadeFixed16]
            mov esi, ebp
            and ebp, 0f80000h
            shr ebp, 0bh
            add esi, dword ptr [g_spanActiveShadeStepFixed16]
            add eax, ebp
            mov dword ptr [g_spanActiveShadeFixed16], esi
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            dec edi
            push word ptr [ebx+eax*2]
            jne zRndr_span_shade_pal8_switch_loop16
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;

    case 17:
        __asm {
            push ebp
            mov dword ptr [g_spanSavedEspSlot], esp
            mov ecx, texU
            mov edx, texV
            mov edi, pixelCount
            mov esp, dword ptr [g_spanCurrentSpanBaseAddr]
            mov esi, dword ptr [g_spanActiveTexVMask]
            add esp, edi
            mov ebx, dword ptr [g_spanActiveTexPalette]
            add esp, edi
        zRndr_span_shade_pal8_switch_loop17:
            mov eax, ecx
            mov esi, dword ptr [g_spanActiveTexVMask]
            sar eax, 14h
            and esi, edx
            shr esi, 11h
            and eax, 7
            mov ebp, dword ptr [g_spanActiveTexPixels]
            add esi, eax
            xor eax, eax
            add edx, dword ptr [g_spanActiveTexVStepFixed20]
            mov al, byte ptr [ebp+esi]
            mov ebp, dword ptr [g_spanActiveShadeFixed16]
            mov esi, ebp
            and ebp, 0f80000h
            shr ebp, 0bh
            add esi, dword ptr [g_spanActiveShadeStepFixed16]
            add eax, ebp
            mov dword ptr [g_spanActiveShadeFixed16], esi
            add ecx, dword ptr [g_spanActiveTexUStepFixed20]
            dec edi
            push word ptr [ebx+eax*2]
            jne zRndr_span_shade_pal8_switch_loop17
            mov esp, dword ptr [g_spanSavedEspSlot]
            pop ebp
        }
        return;
    }
}
#else
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zrender.span-shade-16-from-pal8-switch-vshift
 * @recoil-artifact defines .text recoil:function:0x49f180: Current C++ definition under the canonical compiler settings.
 *
 * Original function evidence: retail 0x49f180 has this portable conditional definition.
 * Purpose: Preserve portable palettized shade behavior when the ESP-pivot raw-assembly exception is disabled.
 */
void __fastcall SpanShade16FromPal8SwitchVShift(int texU, int texV, int pixelCount, int texVShift)
{
    switch (texVShift) {
    default:
        return;

    case 10: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 10);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16
                = (int)((unsigned int)(g_spanActiveShadeFixed16) + (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 11: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1ff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 11);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16
                = (int)((unsigned int)(g_spanActiveShadeFixed16) + (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 12: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0xff) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 12);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16
                = (int)((unsigned int)(g_spanActiveShadeFixed16) + (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 13: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x7f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 13);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16
                = (int)((unsigned int)(g_spanActiveShadeFixed16) + (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 14: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x3f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 14);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16
                = (int)((unsigned int)(g_spanActiveShadeFixed16) + (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 15: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x1f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 15);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16
                = (int)((unsigned int)(g_spanActiveShadeFixed16) + (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 16: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x0f) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 16);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16
                = (int)((unsigned int)(g_spanActiveShadeFixed16) + (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }

    case 17: {
        unsigned short* dstEnd = g_spanCurrentSpanBaseAddr + pixelCount;
        for (int i = 0; i < pixelCount; ++i) {
            --dstEnd;
            const int sourceIndex = ((texU >> 20) & 0x07) + ((unsigned int)(texV & g_spanActiveTexVMask) >> 17);
            const int shadeBucket = (g_spanActiveShadeFixed16 & 0x00f80000) >> 11;
            const int paletteIndex = g_spanActiveTexPixels[sourceIndex] + shadeBucket;
            g_spanActiveShadeFixed16
                = (int)((unsigned int)(g_spanActiveShadeFixed16) + (unsigned int)(g_spanActiveShadeStepFixed16));
            *dstEnd = g_spanActiveTexPalette[paletteIndex];
            texU += g_spanActiveTexUStepFixed20;
            texV += g_spanActiveTexVStepFixed20;
        }
        return;
    }
    }
}
#endif
} // namespace zRndr
