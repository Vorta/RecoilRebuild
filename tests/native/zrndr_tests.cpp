#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zClass.h"
#include "zClipRect.h"

#include <cstdlib>
#include <cstring>

namespace {
int g_line4Count = 0;
int g_line5Count = 0;
void *g_lineFrameBuffer = nullptr;
const void *g_lineClipRect = nullptr;
int g_lineArgs[5] = {};
int g_pointOpCount = 0;
void *g_pointOpFrameBuffer = nullptr;
int g_pointOpArgs[3] = {};
int g_pointOpTrace[64][3] = {};
int g_rasterSpanBuildCount = 0;
int g_rasterSpanOpCount = 0;
int g_rasterLastColumn = 0;
int g_rasterLastContext = 0;
int g_rasterLastPixelCount = 0;
zRndr::SpanNodePartial g_rasterLastNode = {};
std::uint16_t *g_rasterLastDst = nullptr;
int g_flatImmediateSpanOpCount = 0;
int g_flatImmediateLastEcxArg = 0;
int g_flatImmediateLastEdxArg = 0;
int g_texturedQueuedSpanCount = 0;
int g_texturedQueuedLastTexU = 0;
int g_texturedQueuedLastTexV = 0;
int g_texturedQueuedLastPixelCount = 0;
int g_texturedQueuedLastVShift = 0;

void RECOIL_FASTCALL TestImmediateRaster4(void *frameBuffer, int x0, int y0, int x1, int y1,
                                          int color16) {
    ++g_line4Count;
    g_lineFrameBuffer = frameBuffer;
    g_lineArgs[0] = x0;
    g_lineArgs[1] = y0;
    g_lineArgs[2] = x1;
    g_lineArgs[3] = y1;
    g_lineArgs[4] = color16;
}

void RECOIL_FASTCALL TestImmediateRaster5(void *frameBuffer, const void *clipRect, int x0, int y0,
                                          int x1, int y1, int color16) {
    ++g_line5Count;
    g_lineFrameBuffer = frameBuffer;
    g_lineClipRect = clipRect;
    g_lineArgs[0] = x0;
    g_lineArgs[1] = y0;
    g_lineArgs[2] = x1;
    g_lineArgs[3] = y1;
    g_lineArgs[4] = color16;
}

void RECOIL_FASTCALL TestPointOp(void *frameBuffer, std::int32_t y, std::int32_t x,
                                 std::int32_t color16) {
    ++g_pointOpCount;
    g_pointOpFrameBuffer = frameBuffer;
    g_pointOpArgs[0] = y;
    g_pointOpArgs[1] = x;
    g_pointOpArgs[2] = color16;
    if (g_pointOpCount <= 64) {
        g_pointOpTrace[g_pointOpCount - 1][0] = y;
        g_pointOpTrace[g_pointOpCount - 1][1] = x;
        g_pointOpTrace[g_pointOpCount - 1][2] = color16;
    }
}

std::uint32_t RotateRight32(std::uint32_t value, int count) {
    return (value >> count) | (value << (32 - count));
}

std::uint16_t ExpectedFog565Pixel(std::uint16_t pixel, std::uint32_t fogCoordFixed24) {
    if (static_cast<std::int32_t>(fogCoordFixed24) >= 0x1000000) {
        return static_cast<std::uint16_t>(zRndr::g_fogParamsActive.packedColor16);
    }

    if (static_cast<std::int32_t>(fogCoordFixed24) < 0x80000) {
        return pixel;
    }

    const std::uint32_t rampIndex = (0x1000000u - fogCoordFixed24) >> 19;
    const std::uint32_t rampValue =
        static_cast<std::uint32_t>(zRndr::g_fogParamsActive.packedColorRamp[rampIndex]);
    const std::uint32_t pixel32 = pixel;
    return static_cast<std::uint16_t>(
        (((((pixel32 & 0x07e0u) >> 5) * rampIndex) + rampValue) & 0x07e0u) +
        ((((pixel32 & 0xf81fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) & 0xf81fu));
}

std::uint16_t ExpectedFog555Pixel(std::uint16_t pixel, std::uint32_t fogCoordFixed24) {
    if (static_cast<std::int32_t>(fogCoordFixed24) >= 0x1000000) {
        return static_cast<std::uint16_t>(zRndr::g_fogParamsActive.packedColor16);
    }

    if (static_cast<std::int32_t>(fogCoordFixed24) < 0x80000) {
        return pixel;
    }

    const std::uint32_t rampIndex = (0x1000000u - fogCoordFixed24) >> 19;
    const std::uint32_t rampValue =
        static_cast<std::uint32_t>(zRndr::g_fogParamsActive.packedColorRamp[rampIndex]);
    const std::uint32_t pixel32 = pixel;
    return static_cast<std::uint16_t>(
        (((((pixel32 & 0x03e0u) >> 5) * rampIndex) + rampValue) & 0x03e0u) +
        ((((pixel32 & 0x7c1fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) & 0x7c1fu));
}

std::uint32_t ExpectedFog565Pair(std::uint32_t packedPixels, std::uint32_t fogCoordFixed24) {
    if (static_cast<std::int32_t>(fogCoordFixed24) >= 0x1000000) {
        return static_cast<std::uint32_t>(zRndr::g_fogParamsActive.packedColor16Dup);
    }

    if (static_cast<std::int32_t>(fogCoordFixed24) < 0x80000) {
        return packedPixels;
    }

    const std::uint32_t rampIndex = (0x1000000u - fogCoordFixed24) >> 19;
    const std::uint32_t rampValue =
        static_cast<std::uint32_t>(zRndr::g_fogParamsActive.packedColorRamp[rampIndex]);
    return (((((packedPixels & 0xf81f07e0u) >> 5) * rampIndex) + rampValue) & 0xf81f07e0u) +
           ((((packedPixels & 0x07e0f81fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) &
            0x07e0f81fu);
}

std::uint32_t ExpectedFog555Pair(std::uint32_t packedPixels, std::uint32_t fogCoordFixed24) {
    if (static_cast<std::int32_t>(fogCoordFixed24) >= 0x1000000) {
        return static_cast<std::uint32_t>(zRndr::g_fogParamsActive.packedColor16Dup);
    }

    if (static_cast<std::int32_t>(fogCoordFixed24) < 0x80000) {
        return packedPixels;
    }

    const std::uint32_t rampIndex = (0x1000000u - fogCoordFixed24) >> 19;
    const std::uint32_t rampValue =
        static_cast<std::uint32_t>(zRndr::g_fogParamsActive.packedColorRamp[rampIndex]);
    return (((((packedPixels & 0x7c1f03e0u) >> 5) * rampIndex) + rampValue) & 0x7c1f03e0u) +
           ((((packedPixels & 0x03e07c1fu) * rampIndex + RotateRight32(rampValue, 11)) >> 5) &
            0x03e07c1fu);
}

std::uint16_t ExpectedBlend565Alpha8(std::uint16_t dstPixel, std::uint16_t srcPixel, int alpha) {
    const std::int32_t dstColor = static_cast<std::int16_t>(dstPixel);
    const std::int32_t srcColor = srcPixel;
    const std::int32_t greenDelta = (((srcColor & 0x07e0) - (dstColor & 0x07e0)) * alpha) >> 8;
    const std::int32_t redDelta = (((srcColor & 0xf800) - (dstColor & 0xf800)) * alpha) >> 8;
    std::int32_t blended = dstColor + (redDelta & 0xfffff800);
    const std::int32_t blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
    blended += (greenDelta & 0xffffffe0) + blueDelta;
    return static_cast<std::uint16_t>(blended);
}

std::uint32_t ExpectedBlend565PairAlpha5(std::uint32_t packedPixels, std::uint16_t srcPixel,
                                         int alpha) {
    const std::uint32_t srcPair =
        static_cast<std::uint32_t>(srcPixel) | (static_cast<std::uint32_t>(srcPixel) << 16);
    const std::uint32_t alpha5 = static_cast<std::uint32_t>(alpha >> 3);
    const std::uint32_t inverseAlpha5 = 0x1fu - alpha5;
    return (((((packedPixels & 0x07e0f81fu) * inverseAlpha5) +
              ((srcPair & 0x07e0f81fu) * alpha5)) >>
             5) &
            0x07e0f81fu) |
           (((((packedPixels >> 5) & 0x07c0f83fu) * inverseAlpha5) +
             (((srcPair >> 5) & 0x07c0f83fu) * alpha5)) &
            0xf81f07e0u);
}

std::uint16_t ExpectedBlend555Alpha8(std::uint16_t dstPixel, std::uint16_t srcPixel, int alpha) {
    const std::int32_t dstColor = static_cast<std::int16_t>(dstPixel);
    const std::int32_t srcColor = srcPixel;
    const std::int32_t redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
    std::int32_t blended = dstColor + (redDelta & 0xfffffc00);
    const std::int32_t greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
    const std::int32_t blueDelta = (((srcColor & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
    blended += (greenDelta & 0xffffffe0) + blueDelta;
    return static_cast<std::uint16_t>(blended);
}

std::uint32_t ExpectedBlend555PairAlpha5(std::uint32_t packedPixels, std::uint16_t srcPixel,
                                         int alpha) {
    const std::uint32_t srcPair =
        static_cast<std::uint32_t>(srcPixel) | (static_cast<std::uint32_t>(srcPixel) << 16);
    const std::uint32_t alpha5 = static_cast<std::uint32_t>(alpha >> 3);
    const std::uint32_t inverseAlpha5 = 0x1fu - alpha5;
    return (((((packedPixels & 0x03e07c1fu) * inverseAlpha5) +
              ((srcPair & 0x03e07c1fu) * alpha5)) >>
             5) &
            0x03e07c1fu) |
           (((((packedPixels >> 5) & 0x03e0f81fu) * inverseAlpha5) +
             (((srcPair >> 5) & 0x03e0f81fu) * alpha5)) &
            0x7c1f03e0u);
}

std::uint16_t ExpectedBlend555ConstAlphaMap(std::uint16_t dstPixel, std::uint16_t srcPixel,
                                            int alpha) {
    const std::int32_t dstColor = static_cast<std::int16_t>(dstPixel);
    const std::int32_t srcColor = srcPixel;
    const std::int32_t redDelta = (((srcColor & 0x7c00) - (dstColor & 0x7c00)) * alpha) >> 8;
    const std::int32_t greenDelta = (((srcColor & 0x03e0) - (dstColor & 0x03e0)) * alpha) >> 8;
    const std::int32_t blueDelta = (((srcColor & 0x001f) - (dstColor & 0x001f)) * alpha) >> 8;
    return static_cast<std::uint16_t>(dstColor + (redDelta & 0xfffffc00) +
                                      (greenDelta & 0xffffffe0) + blueDelta);
}

std::int16_t SaturatingSubWord(std::uint16_t minuend, std::uint16_t subtrahend) {
    const std::int32_t result =
        static_cast<std::int16_t>(minuend) - static_cast<std::int16_t>(subtrahend);
    if (result > 0x7fff) {
        return 0x7fff;
    }
    if (result < -0x8000) {
        return static_cast<std::int16_t>(0x8000);
    }
    return static_cast<std::int16_t>(result);
}

std::uint16_t MultiplyLowWord(std::int16_t lhs, std::int16_t rhs) {
    return static_cast<std::uint16_t>(static_cast<std::int32_t>(lhs) *
                                      static_cast<std::int32_t>(rhs));
}

std::uint16_t ExpectedFogMmxLane(std::uint16_t pixel, std::uint16_t factor, int lane, int redShift,
                                 int redTermShift) {
    const std::int16_t signedFactor = static_cast<std::int16_t>(factor);
    const std::uint16_t redProduct =
        MultiplyLowWord(SaturatingSubWord(zRndr::g_mmxBitsRed255[lane],
                                          static_cast<std::uint16_t>(pixel >> redShift)),
                        signedFactor);
    const std::uint16_t greenProduct =
        MultiplyLowWord(SaturatingSubWord(zRndr::g_mmxBitsGreen255[lane],
                                          static_cast<std::uint16_t>(
                                              (pixel & zRndr::g_mmxMaskGreenBits[lane]) >> 5)),
                        signedFactor);
    const std::uint16_t blueProduct = MultiplyLowWord(
        SaturatingSubWord(zRndr::g_mmxBitsBlue255[lane],
                          static_cast<std::uint16_t>(pixel & zRndr::g_mmxMaskBlueBits[lane])),
        signedFactor);

    const std::uint16_t redTerm =
        static_cast<std::uint16_t>(redProduct << redTermShift) & zRndr::g_mmxMaskRedPacked[lane];
    const std::uint16_t greenTerm =
        static_cast<std::uint16_t>(static_cast<std::int16_t>(greenProduct) >> 3) &
        zRndr::g_mmxMaskGreenPacked[lane];
    const std::uint16_t blueTerm =
        static_cast<std::uint16_t>(static_cast<std::int16_t>(blueProduct) >> 8);
    return static_cast<std::uint16_t>(pixel + redTerm + greenTerm + blueTerm);
}

void ReplicateMmxTargetBits(std::uint16_t red, std::uint16_t green, std::uint16_t blue) {
    for (int i = 0; i < 4; ++i) {
        zRndr::g_mmxBitsRed255[i] = red;
        zRndr::g_mmxBitsGreen255[i] = green;
        zRndr::g_mmxBitsBlue255[i] = blue;
    }
}

void RECOIL_FASTCALL TestRasterBuildSpanList(zRndr::SpanNodePartial **spanList,
                                             std::int32_t columnIndex, std::int32_t *spanCount) {
    ++g_rasterSpanBuildCount;
    g_rasterLastColumn = columnIndex;
    g_rasterLastNode = *zRndr::g_spanAllocCursor;
    spanList[0] = zRndr::g_spanAllocCursor;
    *spanCount = 1;
    ++zRndr::g_spanAllocCursor;
}

void RECOIL_FASTCALL TestRasterSelectedSpanOp(std::int32_t spanOpContext, std::int32_t pixelCount) {
    ++g_rasterSpanOpCount;
    g_rasterLastContext = spanOpContext;
    g_rasterLastPixelCount = pixelCount;
    g_rasterLastDst = zRndr::g_spanCurrentDst16;
}

void RECOIL_FASTCALL TestTexturedQueuedSpanOp(std::int32_t texU, std::int32_t texV,
                                              std::int32_t pixelCount, std::int32_t texVShift) {
    ++g_texturedQueuedSpanCount;
    g_texturedQueuedLastTexU = texU;
    g_texturedQueuedLastTexV = texV;
    g_texturedQueuedLastPixelCount = pixelCount;
    g_texturedQueuedLastVShift = texVShift;
    g_rasterLastDst = zRndr::g_spanCurrentDst16;
}

void RECOIL_FASTCALL TestFlatImmediateSpanOp(std::int32_t flatSpanOpEcxArg,
                                             std::int32_t flatSpanOpEdxArg,
                                             std::int32_t pixelCount) {
    ++g_flatImmediateSpanOpCount;
    g_flatImmediateLastEcxArg = flatSpanOpEcxArg;
    g_flatImmediateLastEdxArg = flatSpanOpEdxArg;
    g_rasterLastPixelCount = pixelCount;
    g_rasterLastDst = zRndr::g_spanCurrentDst16;
}

bool SameQueuedVec3(const zRndr::QueuedVec3 &actual, const zVec3 &expected) {
    return actual.x == expected.x && actual.y == expected.y && actual.z == expected.z;
}

bool FloatNear(float actual, float expected) {
    const float delta = actual - expected;
    return delta > -0.0001f && delta < 0.0001f;
}
} // namespace

extern "C" int zrndr_get_active_region_state_smoke(void) {
    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x12345678);
    zRndr::g_activeRegionWidth = 320;
    zRndr::g_activeRegionHeight = 200;
    zRndr::g_pitchBytes = 640;
    zRndr::g_bytesPerPixel = 2;

    int width = 0;
    int height = 0;
    int bitsPerPixel = 0;
    int pitch = 0;
    void *framebuffer = zRndr::GetActiveRegionState(&width, &height, &bitsPerPixel, &pitch);

    return framebuffer == reinterpret_cast<void *>(0x12345678) && width == 320 && height == 200 &&
                   bitsPerPixel == 16 && pitch == 640
               ? 0
               : 1;
}

extern "C" int zrndr_framebuffer_and_stride_cache_smoke(void) {
    zOpt_ViewRectSection rect{};
    rect.x = 10;
    rect.y = 20;
    rect.rightExclusive = 330;
    rect.bottomExclusive = 220;
    zRndr::g_bytesPerPixel = 1;
    zRndr::g_perspectiveTextureDeltaXPow2 = 32;

    zRndr::SetFrameBufferRegion(reinterpret_cast<void *>(0x11112222), &rect, 16, 640);
    if (zRndr::g_frameBuffer != reinterpret_cast<void *>(0x11112222) ||
        zRndr::g_activeRegionWidth != 320 || zRndr::g_activeRegionHeight != 200 ||
        zRndr::g_activeRegionRect.x != 10 || zRndr::g_activeRegionRect.y != 20 ||
        zRndr::g_activeRegionRect.right != 330 || zRndr::g_activeRegionRect.bottom != 220 ||
        zRndr::g_bytesPerPixel != 2 || zRndr::g_pitchBytes != 640 ||
        zRndr::g_perspectiveTextureDeltaXBytes != 64) {
        return 1;
    }

    zRndr::SetFrameBufferRegion(reinterpret_cast<void *>(0x33334444), nullptr, 0, 1280);
    if (zRndr::g_frameBuffer != reinterpret_cast<void *>(0x33334444) ||
        zRndr::g_activeRegionWidth != 320 || zRndr::g_bytesPerPixel != 2 ||
        zRndr::g_pitchBytes != 1280 || zRndr::g_perspectiveTextureDeltaXBytes != 64) {
        return 2;
    }

    zRndr::SetVideoStrideMirrors(4096);
    return zRndr::g_videoStrideMirror0 == 4096 && zRndr::g_videoStrideMirror1 == 4096 ? 0 : 3;
}

extern "C" int zrndr_immediate_line_dispatch_smoke(void) {
    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x1234);
    zRndr::g_pfnImmediateRaster4 = reinterpret_cast<zRndr::SpanRoutineProc>(TestImmediateRaster4);
    zRndr::g_pfnImmediateRaster5 = reinterpret_cast<zRndr::SpanRoutineProc>(TestImmediateRaster5);

    g_line4Count = 0;
    g_line5Count = 0;
    g_lineFrameBuffer = nullptr;
    g_lineClipRect = nullptr;
    zRndr_DrawImmediateLine(1, 2, 3, 4, 5);
    if (g_line4Count != 1 || g_line5Count != 0 ||
        g_lineFrameBuffer != reinterpret_cast<void *>(0x1234) || g_lineArgs[0] != 1 ||
        g_lineArgs[1] != 2 || g_lineArgs[2] != 3 || g_lineArgs[3] != 4 || g_lineArgs[4] != 5) {
        return 1;
    }

    zRndr_LinePoint2I points[] = {
        {10, 20},
        {30, 40},
        {50, 60},
    };
    const int clipRect = 0x7788;
    g_line4Count = 0;
    g_line5Count = 0;
    g_lineFrameBuffer = nullptr;
    g_lineClipRect = nullptr;
    zRndr_DrawClippedImmediateLineStrip(points, 2, &clipRect, 0x99);
    if (g_line4Count != 0 || g_line5Count != 2 ||
        g_lineFrameBuffer != reinterpret_cast<void *>(0x1234) || g_lineClipRect != &clipRect ||
        g_lineArgs[0] != 30 || g_lineArgs[1] != 40 || g_lineArgs[2] != 50 || g_lineArgs[3] != 60 ||
        g_lineArgs[4] != 0x99) {
        return 2;
    }

    g_line5Count = 0;
    zRndr_DrawClippedImmediateLineStrip(points, 0, &clipRect, 0x99);
    return g_line5Count == 0 ? 0 : 3;
}

extern "C" int zrndr_lens_flare_queue_projected_sample_smoke(void) {
    zRndr::g_lensFlareSampleQueueCount = 0;
    zRndr::g_inverseDepthBias = 1.0f;
    zRndr::g_inverseDepthScale = 2.0f;
    zRndr::g_lensFlareSampleQueue[0] = {};

    zProjectedPoint point{3.0f, 4.0f, 5.0f};
    zRndr_LensFlare_QueueProjectedSample(&point, 0x1234, 0x5678);
    if (zRndr::g_lensFlareSampleQueueCount != 1 || point.reciprocalZ != 11.0f ||
        zRndr::g_lensFlareSampleQueue[0].x != 3.0f || zRndr::g_lensFlareSampleQueue[0].y != 4.0f ||
        zRndr::g_lensFlareSampleQueue[0].reciprocalZ != 11.0f ||
        zRndr::g_lensFlareSampleQueue[0].packedColor16 != 0x1234 ||
        zRndr::g_lensFlareSampleQueue[0].lensFlareSource != 0x5678) {
        return 1;
    }

    zRndr::g_lensFlareSampleQueueCount = 0x28a;
    point = {7.0f, 8.0f, 9.0f};
    zRndr_LensFlare_QueueProjectedSample(&point, 0xabcd, 0xef01);
    return zRndr::g_lensFlareSampleQueueCount == 0x28a && point.reciprocalZ == 9.0f ? 0 : 2;
}

extern "C" int zrndr_lens_flare_draw_sample_smoke(void) {
    std::uint16_t frame[8 * 8] = {};
    zRndr::LensFlareSamplePartial sample{};
    sample.x = 2.0f;
    sample.y = 3.0f;
    sample.reciprocalZ = 1.0f;
    sample.packedColor16 = 0x7bef;

    zRndr::g_frameBuffer = frame;
    zRndr::g_pitchBytes = 8 * static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_activeRegionWidth = 7;
    zRndr::g_activeRegionHeight = 7;
    zRndr::g_pixelPackGreenBits = 6;
    zRndr::g_overlayBlendEnabled = 0;

    zRndr::LensFlare_DrawQueuedSample16_ClippedFramebuffer(&sample, 1, 1.0f);
    if (frame[4 * 8 + 2] != 0x7bef) {
        return 1;
    }

    zRndr::g_overlayBlendEnabled = 1;
    zRndr::g_overlayBlendPackedColor16 = 0xf800;
    zRndr::g_overlayBlendAlpha = 1.0;
    sample.x = 1.0f;
    sample.y = 1.0f;
    sample.packedColor16 = 0x001f;
    zRndr::LensFlare_DrawQueuedSample16_ClippedFramebuffer(&sample, 0, 1.0f);
    if (frame[1 * 8 + 1] != 0xf800) {
        return 2;
    }

    zRndr_LensFlareSource source{};
    source.depthFadeInvZMin = 0.1f;
    source.depthFadeInvZMax = 1.0f;
    source.depthFadeScale = 128.0f;
    sample.x = 3.0f;
    sample.y = 3.0f;
    sample.reciprocalZ = 2.0f;
    sample.packedColor16 = 0xffff;
    sample.lensFlareSource = reinterpret_cast<std::intptr_t>(&source);
    zRndr::g_overlayBlendEnabled = 0;
    frame[3 * 8 + 3] = 0x0000;
    zRndr::LensFlare_DrawQueuedSample16_ClippedFramebuffer(&sample, 0, 1.0f);
    if (frame[3 * 8 + 3] == 0x0000 || frame[3 * 8 + 3] == 0xffff) {
        return 3;
    }

    sample.x = 20.0f;
    sample.y = 20.0f;
    zRndr::LensFlare_DrawQueuedSample16_ClippedFramebuffer(&sample, 0, 1.0f);

    zRndr::g_frameBuffer = nullptr;
    zRndr::g_overlayBlendEnabled = 0;
    return 0;
}

extern "C" int zrndr_lens_flare_stage_helpers_smoke(void) {
    zRndr::g_lensFlareSampleQueueCount = 17;
    zRndr::LensFlare_ResetSampleQueue();
    if (zRndr_LensFlare_GetQueuedSampleCount() != 0) {
        return 1;
    }

    zImage_TexDirEntryPartial stages[4] = {};
    for (int i = 0; i < 4; ++i) {
        zRndr::g_lensFlareVisibleSampleStages[i] = nullptr;
    }

    zRndr::g_lensFlareVisibilityActive = 1;
    zRndr_LensFlare_SetVisibleSampleStage(-1, &stages[0]);
    if (zRndr::g_lensFlareVisibilityActive != 0 ||
        zRndr::g_lensFlareVisibleSampleStages[0] != nullptr) {
        return 2;
    }

    for (int i = 0; i < 3; ++i) {
        zRndr_LensFlare_SetVisibleSampleStage(i, &stages[i]);
        if (zRndr::g_lensFlareVisibilityActive != 0) {
            return 3 + i;
        }
    }

    zRndr_LensFlare_SetVisibleSampleStage(4, &stages[3]);
    if (zRndr::g_lensFlareVisibilityActive != 0 ||
        zRndr::g_lensFlareVisibleSampleStages[3] != nullptr) {
        return 6;
    }

    zRndr_LensFlare_SetVisibleSampleStage(3, &stages[3]);
    if (zRndr::g_lensFlareVisibilityActive != 1) {
        return 7;
    }

    zRndr_LensFlare_SetVisibleSampleStage(2, nullptr);
    return zRndr::g_lensFlareVisibilityActive == 0 &&
                   zRndr::g_lensFlareVisibleSampleStages[2] == nullptr
               ? 0
               : 8;
}

extern "C" int zrndr_span_occlusion_filter_sample_list_smoke(void) {
    g_zMath_ProjOffsetX = 320.0f;
    g_zMath_ProjOffsetY = 240.0f;
    g_zMath_InvProjScaleX = 0.01f;
    g_zMath_InvProjScaleY = -0.02f;
    zMath::g_zMath_CameraScratchA = {1.0f, 0.0f, 0.0f, 0.0f,  1.0f,  0.0f,
                                     0.0f, 0.0f, 1.0f, 10.0f, 20.0f, 30.0f};

    zRndr_LensFlareVisibleSampleDef sample{};
    sample.sampleCenterX = 340.0f;
    sample.sampleCenterY = 220.0f;
    sample.depthDivisor = 0.1f;
    zRndr::g_lensFlareVisibleSampleDefs[2] = &sample;

    zVec3 outPoint{};
    zRndr_SpanOcclusion_FilterSampleList(2, &outPoint);
    return outPoint.x > 11.999f && outPoint.x < 12.001f && outPoint.y > 23.999f &&
                   outPoint.y < 24.001f && outPoint.z > 39.999f && outPoint.z < 40.001f
               ? 0
               : 1;
}

extern "C" int zrndr_lens_flare_draw_sample_stage_clipped_smoke(void) {
    zVidImagePartial image{};
    image.formatFlagsPacked = 2;
    zImage_TexDirEntryPartial entry{};
    entry.image = &image;
    zVec2 center{5.0f, 5.0f};
    zRndr_LineClipRect2I clipRect{4, 3, 8, 9};

    zRndr::g_transparentQueueCount = 0;
    zRndr::g_transparentQueue[0] = {};
    zRndr::g_overwriteQueueCount = 0;
    zRndr::g_scanConvertMode = 12;
    zRndr::g_inverseDepthBias = 2.0f;
    zRndr::g_inverseDepthScale = 3.0f;
    g_zVideo_ActiveRendererPath = 0;
    g_zRndr_ActivePaletteRemapKey = 0x44;

    zRndr_LensFlare_DrawSampleStageClipped(&center, &entry, 3.0f, &clipRect);

    const zRndr::TransparentQueuedPolyDrawCmd &cmd = zRndr::g_transparentQueue[0];
    const std::uint8_t *raw = reinterpret_cast<const std::uint8_t *>(&cmd);
    const zRndr::QueuedVec3 *clipped = reinterpret_cast<const zRndr::QueuedVec3 *>(raw + 0x308);
    if (zRndr::g_transparentQueueCount != 1 || cmd.vertexCount != 4 || cmd.materialRef != &entry ||
        cmd.hasClippedTriVerts != 1 || cmd.scanConvertMode != 12 || cmd.savedInvDepthBias != 2.0f ||
        cmd.savedInvDepthScale != 3.0f || cmd.texKey != 0x44 ||
        !FloatNear(cmd.polyVerts[0].x, 7.0f) || !FloatNear(cmd.polyVerts[0].y, 8.0f) ||
        !FloatNear(cmd.polyVerts[0].z, 10.0f) || !FloatNear(cmd.polyVerts[2].x, 4.0f) ||
        !FloatNear(cmd.polyVerts[2].y, 3.0f) || !FloatNear(cmd.triVerts[1].x, 7.0f) ||
        !FloatNear(cmd.triVerts[1].y, 3.0f) || !FloatNear(cmd.triUVs[0], 0.8333333f) ||
        !FloatNear(cmd.triUVs[1], 0.0f) || !FloatNear(cmd.triUVs[2], 0.8333333f) ||
        !FloatNear(cmd.triUVs[3], 0.8333333f) || !FloatNear(cmd.triUVs[4], 0.3333333f) ||
        !FloatNear(cmd.triUVs[5], 0.8333333f) || !FloatNear(clipped[0].x, 0.7f) ||
        !FloatNear(clipped[0].y, 0.8f) || !FloatNear(clipped[0].z, 0.1f)) {
        return 1;
    }

    zRndr::g_transparentQueueCount = 0;
    center = {1.0f, 1.0f};
    zRndr_LensFlare_DrawSampleStageClipped(&center, &entry, 0.5f, nullptr);
    if (zRndr::g_transparentQueueCount != 0) {
        return 2;
    }

    zRndr::g_activeRegionWidth = 8;
    zRndr::g_activeRegionHeight = 8;
    center = {20.0f, 20.0f};
    zRndr_LensFlare_DrawSampleStageClipped(&center, &entry, 3.0f, nullptr);
    return zRndr::g_transparentQueueCount == 0 ? 0 : 3;
}

extern "C" int zrndr_lens_flare_draw_visible_sample_stages_smoke(void) {
    zVidImagePartial image{};
    image.formatFlagsPacked = 2;
    zImage_TexDirEntryPartial entries[4] = {};
    for (int i = 0; i < 4; ++i) {
        entries[i].image = &image;
        zRndr::g_lensFlareVisibleSampleStages[i] = &entries[i];
    }

    zRndr_LensFlareVisibleSampleDef sample{};
    sample.sampleCenterX = 200.0f;
    sample.sampleCenterY = 80.0f;

    zRndr::g_activeRegionWidth = 320;
    zRndr::g_activeRegionHeight = 200;
    zRndr::g_activeRegionRect = {0, 0, 320, 200};
    zRndr::g_transparentQueueCount = 0;
    zRndr::g_transparentQueue[0] = {};
    zRndr::g_transparentQueue[1] = {};
    zRndr::g_transparentQueue[2] = {};
    zRndr::g_transparentQueue[3] = {};
    g_zVideo_ActiveRendererPath = 0;

    zRndr_LensFlare_DrawVisibleSampleStages(&sample, 0.5f);
    if (zRndr::g_transparentQueueCount != 4 ||
        zRndr::g_transparentQueue[0].materialRef != &entries[0] ||
        zRndr::g_transparentQueue[1].materialRef != &entries[1] ||
        zRndr::g_transparentQueue[2].materialRef != &entries[2] ||
        zRndr::g_transparentQueue[3].materialRef != &entries[3]) {
        return 1;
    }

    const zRndr::TransparentQueuedPolyDrawCmd &stage0 = zRndr::g_transparentQueue[0];
    const zRndr::TransparentQueuedPolyDrawCmd &stage1 = zRndr::g_transparentQueue[1];
    const zRndr::TransparentQueuedPolyDrawCmd &stage2 = zRndr::g_transparentQueue[2];
    const zRndr::TransparentQueuedPolyDrawCmd &stage3 = zRndr::g_transparentQueue[3];
    if (!FloatNear(stage0.polyVerts[0].x, 210.0f) || !FloatNear(stage0.polyVerts[0].y, 90.0f) ||
        !FloatNear(stage1.polyVerts[0].x, 190.0f) || !FloatNear(stage1.polyVerts[0].y, 100.0f) ||
        !FloatNear(stage2.polyVerts[0].x, 169.0f) || !FloatNear(stage2.polyVerts[0].y, 103.0f) ||
        !FloatNear(stage3.polyVerts[0].x, 135.0f) || !FloatNear(stage3.polyVerts[0].y, 135.0f)) {
        return 2;
    }

    return 0;
}

extern "C" int zrndr_lens_flare_draw_visible_sample_smoke(void) {
    zVidImagePartial image{};
    image.formatFlagsPacked = 2;
    zImage_TexDirEntryPartial entries[4] = {};
    for (int i = 0; i < 4; ++i) {
        entries[i].image = &image;
        zRndr::g_lensFlareVisibleSampleStages[i] = &entries[i];
    }

    zRndr_LensFlareSource source{};
    source.fadeNear = 0.2f;
    source.fadeFar = 0.8f;
    zRndr_LensFlareVisibleSampleDef sample{};
    sample.sampleCenterX = 200.0f;
    sample.sampleCenterY = 80.0f;
    sample.depthDivisor = 2.0f;
    sample.lensFlareSource = &source;
    zRndr::g_lensFlareVisibleSampleDefs[1] = &sample;
    zRndr::g_activeRegionWidth = 320;
    zRndr::g_activeRegionHeight = 200;
    zRndr::g_activeRegionRect = {0, 0, 320, 200};
    zRndr::g_lensFlareVisibilityActive = 1;
    zRndr::g_transparentQueueCount = 0;
    g_zVideo_ActiveRendererPath = 0;

    zRndr_LensFlare_DrawVisibleSample(1);
    if (zRndr::g_transparentQueueCount != 4 ||
        !FloatNear(zRndr::g_transparentQueue[0].polyVerts[0].x, 210.0f) ||
        !FloatNear(zRndr::g_transparentQueue[0].polyVerts[0].y, 90.0f)) {
        return 1;
    }

    zRndr::g_transparentQueueCount = 0;
    sample.depthDivisor = 1.0f;
    zRndr_LensFlare_DrawVisibleSample(1);
    if (zRndr::g_transparentQueueCount != 0) {
        return 2;
    }

    zRndr::g_lensFlareVisibilityActive = 0;
    sample.depthDivisor = 2.0f;
    zRndr_LensFlare_DrawVisibleSample(1);
    return zRndr::g_transparentQueueCount == 0 ? 0 : 3;
}

extern "C" int zrndr_lens_flare_draw_visible_samples_smoke(void) {
    zVidImagePartial image{};
    image.formatFlagsPacked = 2;
    zImage_TexDirEntryPartial entries[4] = {};
    for (int i = 0; i < 4; ++i) {
        entries[i].image = &image;
        zRndr::g_lensFlareVisibleSampleStages[i] = &entries[i];
    }

    zRndr_LensFlareSource source{};
    source.fadeNear = 0.2f;
    source.fadeFar = 0.8f;
    zRndr_LensFlareVisibleSampleDef samples[2] = {};
    samples[0].sampleCenterX = 200.0f;
    samples[0].sampleCenterY = 80.0f;
    samples[0].depthDivisor = 2.0f;
    samples[0].lensFlareSource = &source;
    samples[1].sampleCenterX = 160.0f;
    samples[1].sampleCenterY = 100.0f;
    samples[1].depthDivisor = 2.0f;
    samples[1].lensFlareSource = &source;
    zRndr::g_lensFlareVisibleSampleDefs[0] = &samples[0];
    zRndr::g_lensFlareVisibleSampleDefs[1] = &samples[1];

    zRndr::g_activeRegionWidth = 320;
    zRndr::g_activeRegionHeight = 200;
    zRndr::g_activeRegionRect = {0, 0, 320, 200};
    zRndr::g_lensFlareVisibilityActive = 1;
    zRndr::g_lensFlareVisibleSampleCount = 2;
    zRndr::g_transparentQueueCount = 0;
    g_zVideo_ActiveRendererPath = 0;

    zRndr_LensFlare_DrawVisibleSamples();
    if (zRndr::g_lensFlareVisibleSampleCount != 0 || zRndr::g_transparentQueueCount != 8 ||
        !FloatNear(zRndr::g_transparentQueue[0].polyVerts[0].x, 210.0f) ||
        !FloatNear(zRndr::g_transparentQueue[4].polyVerts[0].x, 170.0f)) {
        return 1;
    }

    zRndr::g_lensFlareVisibilityActive = 0;
    zRndr::g_lensFlareVisibleSampleCount = 3;
    zRndr_LensFlare_DrawVisibleSamples();
    return zRndr::g_lensFlareVisibleSampleCount == 3 ? 0 : 2;
}

extern "C" int zrndr_init_globals_smoke(void) {
    zRndr::g_spanAllocCursor = reinterpret_cast<zRndr::SpanNodePartial *>(0x1111);
    zRndr::g_spanColumnHeadTable = reinterpret_cast<zRndr::SpanNodePartial **>(0x2222);
    zRndr::g_spanPoolBase = reinterpret_cast<zRndr::SpanNodePartial *>(0x3333);
    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x4444);
    zRndr::g_activeRegionWidth = 640;
    zRndr::g_bytesPerPixel = 2;
    zRndr::g_videoStrideMirror0 = 8;
    zRndr::g_videoStrideMirror1 = 8;
    zRndr::g_perspectiveTextureDeltaXBytes = 99;
    g_zVideo_pfnBltSourceToPrimary = nullptr;
    g_zGame_Options_OptionListHead = nullptr;
    g_zVideo_ActiveRendererPath = 0;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_fogColorParams = {};

    if (zRndr::InitGlobals() != 0) {
        return 1;
    }

    if (zRndr::g_spanAllocCursor != nullptr || zRndr::g_spanColumnHeadTable != nullptr ||
        zRndr::g_spanPoolBase != nullptr || zRndr::g_perspectiveTextureDeltaXInput != 0x20 ||
        zRndr::g_perspectiveTextureDeltaXPow2 != 0x20 ||
        zRndr::g_perspectiveTextureDeltaXShift != 5 ||
        zRndr::g_perspectiveTextureDeltaXBytes != 0x20 || zRndr::g_scanConvertMode != 1 ||
        zRndr::g_perspectiveTextureEnabled != 1 || zRndr::g_textureMipSelectionEnabled != 1 ||
        zRndr::g_frameBuffer != nullptr || zRndr::g_activeRegionWidth != 0 ||
        zRndr::g_bytesPerPixel != 1 || zRndr::g_videoStrideMirror0 != 1 ||
        zRndr::g_videoStrideMirror1 != 1 || zRndr::g_renderStateReadyWriteOnlyFlag != 1 ||
        zRndr::g_defaultGraphicsFlags != -1 ||
        zRndr::g_graphicsFlags != &zRndr::g_defaultGraphicsFlags ||
        reinterpret_cast<std::uintptr_t>(g_zVideo_pfnBltSourceToPrimary) != 0x0048f560) {
        return 2;
    }

    if (zRndr::g_fogColorParams.colorRgb01[0] < 0.039f ||
        zRndr::g_fogColorParams.colorRgb01[0] > 0.041f ||
        zRndr::g_fogTargetParamsStaged.colorRgb01[0] != zRndr::g_fogColorParams.colorRgb01[0] ||
        zRndr::g_fogParamsActive.colorRgb01[0] != zRndr::g_fogColorParams.colorRgb01[0]) {
        return 3;
    }

    zRndr::SetPerspectiveAdaptiveCorrection(-1.0f);
    if (zRndr::g_spanDepthBias != -1.0f || zRndr::g_spanDepthBiasPlusOne != 0.0f ||
        zRndr::g_spanDepthBiasPlusOneInv != 0.0f) {
        return 4;
    }

    zOptionEntryPartial option = {};
    char optionName[] = "GfxFlags_HW";
    option.name = optionName;
    option.payloadOrBuffer = 0x12345678;
    g_zGame_Options_OptionListHead = &option;
    g_zVideo_ActiveRendererPath = 1;
    zRndr::InitGlobals();

    return zRndr::g_graphicsFlags == &option.payloadOrBuffer &&
                   *zRndr::g_graphicsFlags == 0x12345678
               ? 0
               : 5;
}

extern "C" int zrndr_span_occlusion_test_depth_order_pair_smoke(void) {
    zRndr::SetPerspectiveAdaptiveCorrection(0.0f);

    zRndr::SpanNodePartial lhs{};
    lhs.sampleXMin = 1;
    lhs.sampleXMax = 3;
    lhs.invDepth = 10.0f;
    lhs.invDepthStep = 12.0f;
    lhs.depthSlope = 1.0f;

    zRndr::SpanNodePartial rhs{};
    rhs.sampleXMin = 3;
    rhs.sampleXMax = 3;
    rhs.invDepth = 11.0f;
    rhs.invDepthStep = 11.0f;
    rhs.depthSlope = 0.0f;
    if (zRndr_SpanOcclusion_TestSpanDepthOrderPair(&lhs, &rhs) == 0) {
        return 1;
    }

    rhs.invDepth = 13.0f;
    rhs.invDepthStep = 13.0f;
    if (zRndr_SpanOcclusion_TestSpanDepthOrderPair(&lhs, &rhs) != 0) {
        return 2;
    }

    lhs.sampleXMin = 2;
    lhs.sampleXMax = 2;
    lhs.invDepth = 9.0f;
    lhs.invDepthStep = 9.0f;
    lhs.depthSlope = 0.0f;
    rhs.sampleXMin = 1;
    rhs.sampleXMax = 3;
    rhs.invDepth = 7.0f;
    rhs.invDepthStep = 11.0f;
    rhs.depthSlope = 2.0f;
    if (zRndr_SpanOcclusion_TestSpanDepthOrderPair(&lhs, &rhs) == 0) {
        return 3;
    }

    lhs.sampleXMin = 1;
    lhs.sampleXMax = 5;
    lhs.invDepth = 6.0f;
    lhs.invDepthStep = 10.0f;
    lhs.depthSlope = 1.0f;
    rhs.sampleXMin = 1;
    rhs.sampleXMax = 5;
    rhs.invDepth = 4.0f;
    rhs.invDepthStep = 8.0f;
    rhs.depthSlope = 1.0f;
    if (zRndr_SpanOcclusion_TestSpanDepthOrderPair(&lhs, &rhs) == 0) {
        return 4;
    }

    lhs.invDepth = 2.0f;
    lhs.invDepthStep = 6.0f;
    if (zRndr_SpanOcclusion_TestSpanDepthOrderPair(&lhs, &rhs) != 0) {
        return 5;
    }

    return 0;
}

extern "C" int zrndr_span_occlusion_insert_no_depth_smoke(void) {
    zRndr::SpanNodePartial *heads[4] = {};
    zRndr::SpanNodePartial pool[8] = {};
    zRndr::SpanNodePartial *visible[4] = {};
    std::int32_t spanCount = -1;

    zRndr::g_spanColumnCountPadded = 4;
    zRndr::g_spanColumnHeadTable = heads;

    zRndr::g_spanAllocCursor = &pool[0];
    pool[0].sampleXMin = 2;
    pool[0].sampleXMax = 4;
    pool[0].invDepth = 1.0f;
    pool[0].invDepthStep = 3.0f;
    pool[0].depthSlope = 1.0f;
    zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest(visible, 1, &spanCount);
    if (spanCount != 1 || heads[1] != &pool[0] || visible[0] != &pool[0] ||
        pool[0].next != nullptr || zRndr::g_spanAllocCursor != &pool[1]) {
        return 1;
    }

    heads[1] = &pool[0];
    pool[0] = {};
    pool[1] = {};
    visible[0] = nullptr;
    spanCount = -1;
    pool[0].sampleXMin = 5;
    pool[0].sampleXMax = 8;
    pool[0].invDepth = 5.0f;
    pool[0].invDepthStep = 8.0f;
    pool[0].depthSlope = 1.0f;
    zRndr::g_spanAllocCursor = &pool[1];
    pool[1].sampleXMin = 1;
    pool[1].sampleXMax = 3;
    pool[1].invDepth = 10.0f;
    pool[1].invDepthStep = 12.0f;
    pool[1].depthSlope = 1.0f;
    zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest(visible, 1, &spanCount);
    if (spanCount != 1 || heads[1] != &pool[1] || pool[1].next != &pool[0] ||
        visible[0] != &pool[1] || zRndr::g_spanAllocCursor != &pool[2]) {
        return 2;
    }

    heads[1] = &pool[0];
    for (int i = 0; i < 8; ++i) {
        pool[i] = {};
    }
    visible[0] = nullptr;
    spanCount = -1;
    pool[0].sampleXMin = 0;
    pool[0].sampleXMax = 10;
    pool[0].invDepth = 1.0f;
    pool[0].invDepthStep = 11.0f;
    pool[0].depthSlope = 1.0f;
    zRndr::g_spanAllocCursor = &pool[1];
    pool[1].sampleXMin = 3;
    pool[1].sampleXMax = 5;
    pool[1].invDepth = 20.0f;
    pool[1].invDepthStep = 22.0f;
    pool[1].depthSlope = 1.0f;
    zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest(visible, 1, &spanCount);
    if (spanCount != 1 || heads[1] != &pool[0] || pool[0].sampleXMax != 2 ||
        pool[0].invDepthStep != 3.0f || pool[0].next != &pool[1] || pool[1].next != &pool[2] ||
        pool[2].sampleXMin != 6 || pool[2].sampleXMax != 10 || pool[2].invDepth != 7.0f ||
        pool[2].invDepthStep != 11.0f || visible[0] != &pool[1] ||
        zRndr::g_spanAllocCursor != &pool[3]) {
        return 3;
    }

    heads[1] = &pool[0];
    for (int i = 0; i < 8; ++i) {
        pool[i] = {};
    }
    visible[0] = nullptr;
    spanCount = -1;
    pool[0].sampleXMin = 3;
    pool[0].sampleXMax = 10;
    pool[0].invDepth = 3.0f;
    pool[0].invDepthStep = 10.0f;
    pool[0].depthSlope = 1.0f;
    zRndr::g_spanAllocCursor = &pool[1];
    pool[1].sampleXMin = 1;
    pool[1].sampleXMax = 5;
    pool[1].invDepth = 20.0f;
    pool[1].invDepthStep = 24.0f;
    pool[1].depthSlope = 1.0f;
    zRndr_SpanOcclusion_InsertSpanNode_NoDepthTest(visible, 1, &spanCount);
    if (spanCount != 1 || heads[1] != &pool[1] || pool[1].next != &pool[0] ||
        pool[0].sampleXMin != 6 || pool[0].invDepth != 6.0f || visible[0] != &pool[1] ||
        zRndr::g_spanAllocCursor != &pool[2]) {
        return 4;
    }

    zRndr::g_spanColumnHeadTable = nullptr;
    zRndr::g_spanAllocCursor = nullptr;
    zRndr::g_spanColumnCountPadded = 0;
    return 0;
}

extern "C" int zrndr_span_occlusion_build_span_list_smoke(void) {
    zRndr::SetPerspectiveAdaptiveCorrection(0.0f);

    zRndr::SpanNodePartial *heads[4] = {};
    zRndr::SpanNodePartial pool[8] = {};
    zRndr::SpanNodePartial *visible[4] = {};
    std::int32_t spanCount = -1;

    zRndr::g_spanColumnCountPadded = 4;
    zRndr::g_spanColumnHeadTable = heads;

    zRndr::g_spanAllocCursor = &pool[0];
    pool[0].sampleXMin = 1;
    pool[0].sampleXMax = 3;
    pool[0].invDepth = 1.0f;
    pool[0].invDepthStep = 3.0f;
    pool[0].depthSlope = 1.0f;
    zRndr_SpanOcclusion_BuildSpanList(visible, 1, &spanCount);
    if (spanCount != 1 || heads[1] != nullptr || visible[0] != &pool[0] ||
        zRndr::g_spanAllocCursor != &pool[1]) {
        return 1;
    }

    heads[1] = &pool[0];
    for (int i = 0; i < 8; ++i) {
        pool[i] = {};
    }
    visible[0] = nullptr;
    visible[1] = nullptr;
    spanCount = -1;
    pool[0].sampleXMin = 1;
    pool[0].sampleXMax = 4;
    pool[0].invDepth = 10.0f;
    pool[0].invDepthStep = 10.0f;
    pool[0].depthSlope = 0.0f;
    zRndr::g_spanAllocCursor = &pool[1];
    pool[1].sampleXMin = 1;
    pool[1].sampleXMax = 4;
    pool[1].invDepth = 1.0f;
    pool[1].invDepthStep = 1.0f;
    pool[1].depthSlope = 0.0f;
    zRndr_SpanOcclusion_BuildSpanList(visible, 1, &spanCount);
    if (spanCount != 0 || heads[1] != &pool[0] || zRndr::g_spanAllocCursor != &pool[1]) {
        return 2;
    }

    heads[1] = &pool[0];
    for (int i = 0; i < 8; ++i) {
        pool[i] = {};
    }
    visible[0] = nullptr;
    visible[1] = nullptr;
    spanCount = -1;
    pool[0].sampleXMin = 3;
    pool[0].sampleXMax = 5;
    pool[0].invDepth = 10.0f;
    pool[0].invDepthStep = 10.0f;
    pool[0].depthSlope = 0.0f;
    zRndr::g_spanAllocCursor = &pool[1];
    pool[1].sampleXMin = 1;
    pool[1].sampleXMax = 8;
    pool[1].invDepth = 1.0f;
    pool[1].invDepthStep = 8.0f;
    pool[1].depthSlope = 1.0f;
    zRndr_SpanOcclusion_BuildSpanList(visible, 1, &spanCount);
    if (spanCount != 2 || heads[1] != &pool[0] || visible[0] != &pool[1] ||
        visible[1] != &pool[2] || pool[1].sampleXMin != 1 || pool[1].sampleXMax != 2 ||
        pool[1].invDepthStep != 2.0f || pool[2].sampleXMin != 6 || pool[2].sampleXMax != 8 ||
        pool[2].invDepth != 6.0f || pool[2].invDepthStep != 8.0f ||
        zRndr::g_spanAllocCursor != &pool[3]) {
        return 3;
    }

    zRndr::g_spanColumnHeadTable = nullptr;
    zRndr::g_spanAllocCursor = nullptr;
    zRndr::g_spanColumnCountPadded = 0;
    return 0;
}

extern "C" int zrndr_span_occlusion_insert_local_smoke(void) {
    zRndr::SetPerspectiveAdaptiveCorrection(0.0f);

    zRndr::SpanNodePartial *heads[4] = {};
    zRndr::SpanNodePartial pool[8] = {};
    zRndr::SpanNodePartial *visible[4] = {};
    std::int32_t spanCount = -1;

    zRndr::g_spanColumnCountPadded = 4;
    zRndr::g_spanColumnHeadTable = heads;

    zRndr::g_spanAllocCursor = &pool[0];
    pool[0].sampleXMin = 1;
    pool[0].sampleXMax = 3;
    pool[0].invDepth = 1.0f;
    pool[0].invDepthStep = 3.0f;
    pool[0].depthSlope = 1.0f;
    zRndr_SpanOcclusion_InsertSpanNode_Local(visible, 1, &spanCount);
    if (spanCount != 1 || heads[1] != &pool[0] || visible[0] != &pool[0] ||
        zRndr::g_spanAllocCursor != &pool[1]) {
        return 1;
    }

    heads[1] = &pool[0];
    for (int i = 0; i < 8; ++i) {
        pool[i] = {};
    }
    visible[0] = nullptr;
    spanCount = -1;
    pool[0].sampleXMin = 3;
    pool[0].sampleXMax = 5;
    pool[0].invDepth = 1.0f;
    pool[0].invDepthStep = 1.0f;
    pool[0].depthSlope = 0.0f;
    zRndr::g_spanAllocCursor = &pool[1];
    pool[1].sampleXMin = 1;
    pool[1].sampleXMax = 8;
    pool[1].invDepth = 10.0f;
    pool[1].invDepthStep = 10.0f;
    pool[1].depthSlope = 0.0f;
    zRndr_SpanOcclusion_InsertSpanNode_Local(visible, 1, &spanCount);
    if (spanCount != 1 || heads[1] != &pool[1] || pool[1].sampleXMin != 1 ||
        pool[1].sampleXMax != 8 || pool[1].next != nullptr || visible[0] != &pool[1] ||
        zRndr::g_spanAllocCursor != &pool[2]) {
        return 2;
    }

    heads[1] = &pool[0];
    for (int i = 0; i < 8; ++i) {
        pool[i] = {};
    }
    visible[0] = nullptr;
    visible[1] = nullptr;
    spanCount = -1;
    pool[0].sampleXMin = 3;
    pool[0].sampleXMax = 5;
    pool[0].invDepth = 10.0f;
    pool[0].invDepthStep = 10.0f;
    pool[0].depthSlope = 0.0f;
    zRndr::g_spanAllocCursor = &pool[1];
    pool[1].sampleXMin = 1;
    pool[1].sampleXMax = 8;
    pool[1].invDepth = 1.0f;
    pool[1].invDepthStep = 8.0f;
    pool[1].depthSlope = 1.0f;
    zRndr_SpanOcclusion_InsertSpanNode_Local(visible, 1, &spanCount);
    if (spanCount != 2 || heads[1] != &pool[1] || pool[1].sampleXMin != 1 ||
        pool[1].sampleXMax != 2 || pool[1].next != &pool[0] || pool[0].next != &pool[2] ||
        pool[2].sampleXMin != 6 || pool[2].sampleXMax != 8 || visible[0] != &pool[1] ||
        visible[1] != &pool[2] || zRndr::g_spanAllocCursor != &pool[3]) {
        return 3;
    }

    heads[1] = &pool[0];
    for (int i = 0; i < 8; ++i) {
        pool[i] = {};
    }
    visible[0] = nullptr;
    spanCount = -1;
    pool[0].sampleXMin = 1;
    pool[0].sampleXMax = 4;
    pool[0].invDepth = 10.0f;
    pool[0].invDepthStep = 10.0f;
    pool[0].depthSlope = 0.0f;
    zRndr::g_spanAllocCursor = &pool[1];
    pool[1].sampleXMin = 1;
    pool[1].sampleXMax = 4;
    pool[1].invDepth = 1.0f;
    pool[1].invDepthStep = 1.0f;
    pool[1].depthSlope = 0.0f;
    zRndr_SpanOcclusion_InsertSpanNode_Local(visible, 1, &spanCount);
    if (spanCount != 0 || heads[1] != &pool[0] || zRndr::g_spanAllocCursor != &pool[1]) {
        return 4;
    }

    zRndr::g_spanColumnHeadTable = nullptr;
    zRndr::g_spanAllocCursor = nullptr;
    zRndr::g_spanColumnCountPadded = 0;
    return 0;
}

extern "C" int zrndr_span_occlusion_build_span_list_fast_smoke(void) {
    zRndr::SpanNodePartial pool[2] = {};
    zRndr::SpanNodePartial *visible[2] = {};
    std::int32_t spanCount = -1;

    zRndr::g_spanAllocCursor = &pool[0];
    pool[0].next = &pool[1];
    pool[0].sampleXMin = 4;
    pool[0].sampleXMax = 7;
    pool[0].invDepth = 2.0f;
    pool[0].invDepthStep = 5.0f;
    pool[0].depthSlope = 1.0f;

    zRndr_SpanOcclusion_BuildSpanListFast(visible, 99, &spanCount);
    if (spanCount != 1 || visible[0] != &pool[0] || pool[0].next != nullptr ||
        zRndr::g_spanAllocCursor != &pool[1]) {
        return 1;
    }

    zRndr::g_spanAllocCursor = nullptr;
    return 0;
}

extern "C" int zrndr_span_occlusion_test_column_visibility_smoke(void) {
    zRndr::SetPerspectiveAdaptiveCorrection(0.0f);

    zRndr::SpanNodePartial *heads[4] = {};
    zRndr::SpanNodePartial pool[4] = {};
    std::int32_t visible = -1;

    zRndr::g_spanColumnCountPadded = 4;
    zRndr::g_spanColumnHeadTable = heads;
    zRndr::g_spanAllocCursor = &pool[0];
    pool[0].sampleXMin = 1;
    pool[0].sampleXMax = 4;
    pool[0].invDepth = 1.0f;
    pool[0].invDepthStep = 1.0f;
    pool[0].depthSlope = 0.0f;
    zRndr_SpanOcclusion_TestColumnVisibility(1, &visible);
    if (visible != 1) {
        return 1;
    }

    heads[1] = &pool[1];
    pool[1].sampleXMin = 5;
    pool[1].sampleXMax = 8;
    visible = -1;
    zRndr_SpanOcclusion_TestColumnVisibility(1, &visible);
    if (visible != 1) {
        return 2;
    }

    pool[1].sampleXMin = 1;
    pool[1].sampleXMax = 4;
    pool[1].invDepth = 10.0f;
    pool[1].invDepthStep = 10.0f;
    pool[1].depthSlope = 0.0f;
    visible = -1;
    zRndr_SpanOcclusion_TestColumnVisibility(1, &visible);
    if (visible != 0) {
        return 3;
    }

    pool[1].sampleXMin = 2;
    pool[1].sampleXMax = 3;
    pool[1].invDepth = 10.0f;
    pool[1].invDepthStep = 10.0f;
    visible = -1;
    zRndr_SpanOcclusion_TestColumnVisibility(1, &visible);
    if (visible != 1) {
        return 4;
    }

    pool[0].invDepth = 20.0f;
    pool[0].invDepthStep = 20.0f;
    pool[1].sampleXMin = 1;
    pool[1].sampleXMax = 4;
    pool[1].invDepth = 1.0f;
    pool[1].invDepthStep = 1.0f;
    visible = -1;
    zRndr_SpanOcclusion_TestColumnVisibility(1, &visible);
    if (visible != 1) {
        return 5;
    }

    zRndr::g_spanColumnHeadTable = nullptr;
    zRndr::g_spanAllocCursor = nullptr;
    zRndr::g_spanColumnCountPadded = 0;
    return 0;
}

extern "C" int zrndr_span_occlusion_test_point_visibility_smoke(void) {
    zRndr::SetPerspectiveAdaptiveCorrection(0.0f);

    zRndr::SpanNodePartial *heads[4] = {};
    zRndr::SpanNodePartial pool[2] = {};
    zVec3 sample{2.75f, 1.25f, 0.5f};

    zRndr::g_spanColumnCountPadded = 4;
    zRndr::g_spanColumnHeadTable = heads;
    zRndr::g_spanAllocCursor = &pool[0];

    if (zRndr_SpanOcclusion_TestPointVisibility(&sample) != 1 || pool[0].sampleXMin != 2 ||
        pool[0].sampleXMax != 2 || pool[0].invDepth != 0.5f || pool[0].invDepthStep != 0.5f ||
        pool[0].depthSlope != 0.0f) {
        zRndr::g_spanColumnHeadTable = nullptr;
        zRndr::g_spanAllocCursor = nullptr;
        zRndr::g_spanColumnCountPadded = 0;
        return 1;
    }

    heads[1] = &pool[1];
    pool[1].sampleXMin = 2;
    pool[1].sampleXMax = 2;
    pool[1].invDepth = 10.0f;
    pool[1].invDepthStep = 10.0f;
    pool[1].depthSlope = 0.0f;

    if (zRndr_SpanOcclusion_TestPointVisibility(&sample) != 0) {
        zRndr::g_spanColumnHeadTable = nullptr;
        zRndr::g_spanAllocCursor = nullptr;
        zRndr::g_spanColumnCountPadded = 0;
        return 2;
    }

    zRndr::g_spanColumnHeadTable = nullptr;
    zRndr::g_spanAllocCursor = nullptr;
    zRndr::g_spanColumnCountPadded = 0;
    return 0;
}

extern "C" int zrndr_span_occlusion_test_sample_smoke(void) {
    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x12345678);
    zRndr::g_pfnPointOpActive = TestPointOp;
    g_pointOpCount = 0;
    g_pointOpFrameBuffer = nullptr;
    g_pointOpArgs[0] = 0;
    g_pointOpArgs[1] = 0;
    g_pointOpArgs[2] = 0;

    zRndr_SpanOcclusion_TestSample(7, 11, 0x5a5a);
    if (g_pointOpCount != 1 || g_pointOpFrameBuffer != reinterpret_cast<void *>(0x12345678) ||
        g_pointOpArgs[0] != 11 || g_pointOpArgs[1] != 7 || g_pointOpArgs[2] != 0x5a5a) {
        return 1;
    }

    zRndr::g_pfnPointOpActive = nullptr;
    zRndr_SpanOcclusion_TestSample(1, 2, 3);
    return g_pointOpCount == 1 ? 0 : 2;
}

extern "C" int zrndr_draw_circle_octants_smoke(void) {
    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x87654321);
    zRndr::g_pfnPointOpActive = TestPointOp;
    g_zRndr_CircleCenterX = 10;
    g_zRndr_CircleCenterY = 20;
    g_pointOpCount = 0;

    zRndr_DrawCircleOctants16_Framebuffer(2, 5, 0x1234);

    const int expected[8][3] = {
        {22, 15, 0x1234}, {22, 5, 0x1234}, {18, 15, 0x1234}, {18, 5, 0x1234},
        {25, 12, 0x1234}, {25, 8, 0x1234}, {15, 8, 0x1234},  {15, 12, 0x1234},
    };
    if (g_pointOpCount != 8 || g_pointOpFrameBuffer != reinterpret_cast<void *>(0x87654321)) {
        return 1;
    }

    for (int index = 0; index < 8; ++index) {
        if (g_pointOpTrace[index][0] != expected[index][0] ||
            g_pointOpTrace[index][1] != expected[index][1] ||
            g_pointOpTrace[index][2] != expected[index][2]) {
            return 2 + index;
        }
    }

    return 0;
}

extern "C" int zrndr_draw_circle_outline_smoke(void) {
    zRndr::g_frameBuffer = reinterpret_cast<void *>(0xabcdef00);
    zRndr::g_pfnPointOpActive = TestPointOp;
    g_pointOpCount = 0;
    g_zRndr_CircleCenterX = 0;
    g_zRndr_CircleCenterY = 0;
    g_zRndr_CircleDrawAuxArg = 0;

    zRndr_DrawCircleOutline16_Framebuffer(30, 40, 3, 0x55aa, 0x77);
    if (g_zRndr_CircleCenterX != 30 || g_zRndr_CircleCenterY != 40 ||
        g_zRndr_CircleDrawAuxArg != 0x77 || g_pointOpCount != 24) {
        return 1;
    }

    const int expectedStarts[3][2] = {
        {40, 33},
        {41, 33},
        {42, 32},
    };
    for (int octantCall = 0; octantCall < 3; ++octantCall) {
        const int traceIndex = octantCall * 8;
        if (g_pointOpTrace[traceIndex][0] != expectedStarts[octantCall][0] ||
            g_pointOpTrace[traceIndex][1] != expectedStarts[octantCall][1] ||
            g_pointOpTrace[traceIndex][2] != 0x55aa) {
            return 2 + octantCall;
        }
    }

    g_pointOpCount = 0;
    zRndr_DrawCircleOutline16_Framebuffer(1, 2, 0, 3, 4);
    return g_pointOpCount == 0 ? 0 : 5;
}

extern "C" int zrndr_plot_pixel16_smoke(void) {
    std::uint16_t frame[8 * 8] = {};
    zRndr::g_pitchBytes = 8 * static_cast<int>(sizeof(std::uint16_t));

    zRndr_PlotPixel16(frame, 2, 3, 0x12345);
    if (frame[2 * 8 + 3] != 0x2345) {
        return 1;
    }

    for (int index = 0; index < 8 * 8; ++index) {
        if (index != 2 * 8 + 3 && frame[index] != 0) {
            return 2;
        }
    }

    return 0;
}

extern "C" int zrndr_draw_line16_smoke(void) {
    std::uint16_t frame[8 * 8] = {};
    zRndr::g_pitchBytes = 8 * static_cast<int>(sizeof(std::uint16_t));

    zRndr_DrawLine16(frame, 1, 1, 5, 3, 0x0f0f);
    const int xMajorExpected[5][2] = {
        {1, 1}, {2, 1}, {3, 2}, {4, 2}, {5, 3},
    };
    for (int index = 0; index < 5; ++index) {
        const int x = xMajorExpected[index][0];
        const int y = xMajorExpected[index][1];
        if (frame[y * 8 + x] != 0x0f0f) {
            return 1 + index;
        }
    }

    for (int index = 0; index < 8 * 8; ++index) {
        frame[index] = 0;
    }

    zRndr_DrawLine16(frame, 6, 5, 4, 1, 0x7777);
    const int yMajorExpected[5][2] = {
        {6, 5}, {6, 4}, {5, 3}, {5, 2}, {4, 1},
    };
    for (int index = 0; index < 5; ++index) {
        const int x = yMajorExpected[index][0];
        const int y = yMajorExpected[index][1];
        if (frame[y * 8 + x] != 0x7777) {
            return 6 + index;
        }
    }

    return 0;
}

extern "C" int zrndr_draw_line16_segmented_smoke(void) {
    std::uint16_t frame[8 * 8] = {};
    zRndr::g_pitchBytes = 8 * static_cast<int>(sizeof(std::uint16_t));

    zRndr_DrawLine16_Segmented(frame, 1, 1, 7, 1, 0xaaaa, 2);
    for (int x = 1; x <= 7; ++x) {
        const bool shouldDraw = x <= 4;
        if ((frame[1 * 8 + x] == 0xaaaa) != shouldDraw) {
            return x;
        }
    }

    for (int index = 0; index < 8 * 8; ++index) {
        frame[index] = 0;
    }

    zRndr_DrawLine16_Segmented(frame, 6, 5, 4, 1, 0xbbbb, 2);
    const int expectedDrawn[3][2] = {
        {6, 5},
        {6, 4},
        {5, 3},
    };
    for (int index = 0; index < 3; ++index) {
        const int x = expectedDrawn[index][0];
        const int y = expectedDrawn[index][1];
        if (frame[y * 8 + x] != 0xbbbb) {
            return 8 + index;
        }
    }
    if (frame[2 * 8 + 5] != 0 || frame[1 * 8 + 4] != 0) {
        return 11;
    }

    return 0;
}

extern "C" int zrndr_draw_line16_clipped_smoke(void) {
    std::uint16_t frame[8 * 8] = {};
    zRndr::g_pitchBytes = 8 * static_cast<int>(sizeof(std::uint16_t));
    const zRndr_LineClipRect2I clipRect = {2, 2, 5, 5};

    zRndr_DrawLine16_Clipped(frame, &clipRect, 0, 3, 7, 3, 0x1111);
    for (int x = 0; x < 8; ++x) {
        const bool shouldDraw = x >= 2 && x <= 5;
        if ((frame[3 * 8 + x] == 0x1111) != shouldDraw) {
            return 1 + x;
        }
    }

    for (int index = 0; index < 8 * 8; ++index) {
        frame[index] = 0;
    }

    zRndr_DrawLine16_Clipped(frame, &clipRect, 0, 0, 7, 7, 0x2222);
    for (int coord = 2; coord <= 5; ++coord) {
        if (frame[coord * 8 + coord] != 0x2222) {
            return 10 + coord;
        }
    }
    if (frame[1 * 8 + 1] != 0 || frame[6 * 8 + 6] != 0) {
        return 16;
    }

    for (int index = 0; index < 8 * 8; ++index) {
        frame[index] = 0;
    }

    zRndr_DrawLine16_Clipped(frame, &clipRect, 3, 0, 3, 7, 0x3333);
    for (int y = 0; y < 8; ++y) {
        const bool shouldDraw = y >= 2 && y <= 5;
        if ((frame[y * 8 + 3] == 0x3333) != shouldDraw) {
            return 20 + y;
        }
    }

    for (int index = 0; index < 8 * 8; ++index) {
        frame[index] = 0;
    }

    zRndr_DrawLine16_Clipped(frame, &clipRect, -3, 3, 1, 4, 0x4444);
    for (int index = 0; index < 8 * 8; ++index) {
        if (frame[index] != 0) {
            return 30;
        }
    }

    return 0;
}

extern "C" int zrndr_span_occlusion_add_polygon_smoke(void) {
    zRndr::g_spanOccluderPolyCount = 0;
    for (int slot = 0; slot < 8; ++slot) {
        zRndr::g_spanOccluderPolys[slot] = {};
    }

    zVec3 vertices[2] = {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}};
    zRndr::SpanOcclusionAddPolygon(vertices, 2);
    if (zRndr::g_spanOccluderPolyCount != 1 || zRndr::g_spanOccluderPolys[0].vertCount != 2 ||
        zRndr::g_spanOccluderPolys[0].vertices[0][0] != 1.0f ||
        zRndr::g_spanOccluderPolys[0].vertices[0][1] != 2.0f ||
        zRndr::g_spanOccluderPolys[0].vertices[0][2] != 3.0f ||
        zRndr::g_spanOccluderPolys[0].vertices[1][0] != 4.0f ||
        zRndr::g_spanOccluderPolys[0].vertices[1][1] != 5.0f ||
        zRndr::g_spanOccluderPolys[0].vertices[1][2] != 6.0f) {
        return 1;
    }

    zRndr::SpanOcclusionAddPolygon(vertices, 0);
    if (zRndr::g_spanOccluderPolyCount != 2 || zRndr::g_spanOccluderPolys[1].vertCount != 0) {
        return 2;
    }

    zRndr::g_spanOccluderPolyCount = 7;
    zRndr::g_spanOccluderPolys[7].vertCount = 123;
    zRndr::SpanOcclusionAddPolygon(vertices, 2);
    return zRndr::g_spanOccluderPolyCount == 7 && zRndr::g_spanOccluderPolys[7].vertCount == 123
               ? 0
               : 3;
}

extern "C" int zcliprect_clip_poly_near_z_smoke(void) {
    zClipUV uvs[0x40] = {};
    g_Clip_PolyUvs = uvs;
    zClipRectPartial clipRect{};
    clipRect.flags = 0x10;
    clipRect.zMin = 1.0f;
    clipRect.zMax = 4.0f;

    g_Clip_PolyVertsScratch[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVertsScratch[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVertsScratch[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVertsScratch[3] = {-1.0f, 1.0f, 0.0f};
    uvs[0] = {0.0f, 0.0f};
    uvs[1] = {1.0f, 0.0f};
    uvs[2] = {1.0f, 1.0f};
    uvs[3] = {0.0f, 1.0f};

    std::int32_t count = 4;
    if (zClipRect::ClipPolyNearZ(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVertsScratch[0].x < -0.001f || g_Clip_PolyVertsScratch[0].x > 0.001f ||
        g_Clip_PolyVertsScratch[0].z != 1.0f || uvs[0].u < 0.499f || uvs[0].u > 0.501f ||
        g_Clip_PolyVertsScratch[3].x < -0.001f || g_Clip_PolyVertsScratch[3].x > 0.001f ||
        g_Clip_PolyVertsScratch[3].y != 1.0f || uvs[3].v != 1.0f) {
        return 1;
    }

    g_Clip_PolyVertsScratch[0] = {0.0f, 0.0f, 2.0f};
    g_Clip_PolyVertsScratch[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVertsScratch[2] = {0.0f, 1.0f, 2.0f};
    count = 3;
    if (zClipRect::ClipPolyNearZ(&clipRect, &count) != 1 || count != 3) {
        return 2;
    }

    g_Clip_PolyVertsScratch[0].z = 0.0f;
    g_Clip_PolyVertsScratch[1].z = 0.0f;
    g_Clip_PolyVertsScratch[2].z = 0.0f;
    count = 3;
    if (zClipRect::ClipPolyNearZ(&clipRect, &count) != 0 || count != 0) {
        return 3;
    }

    clipRect.flags = 0x20;
    clipRect.zMax = 4.0f;
    g_Clip_PolyVertsScratch[0].z = 4.0f;
    g_Clip_PolyVertsScratch[1].z = 5.0f;
    g_Clip_PolyVertsScratch[2].z = 6.0f;
    count = 3;
    if (zClipRect::ClipPolyNearZ(&clipRect, &count) != 0) {
        return 4;
    }

    g_Clip_PolyVertsScratch[1].z = 3.0f;
    return zClipRect::ClipPolyNearZ(&clipRect, &count) == 1 ? 0 : 5;
}

extern "C" int zcliprect_clip_poly_near_z_attr0_smoke(void) {
    zClipUV uvs[0x40] = {};
    g_Clip_PolyUvs = uvs;
    zClipRectPartial clipRect{};
    clipRect.flags = 0x10;
    clipRect.zMin = 1.0f;
    clipRect.zMax = 4.0f;

    g_Clip_PolyVertsScratch[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVertsScratch[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVertsScratch[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVertsScratch[3] = {-1.0f, 1.0f, 0.0f};
    uvs[0] = {0.0f, 0.0f};
    uvs[1] = {1.0f, 0.0f};
    uvs[2] = {1.0f, 1.0f};
    uvs[3] = {0.0f, 1.0f};
    g_Clip_PolyAttr0[0] = 0.0f;
    g_Clip_PolyAttr0[1] = 10.0f;
    g_Clip_PolyAttr0[2] = 20.0f;
    g_Clip_PolyAttr0[3] = 30.0f;

    std::int32_t count = 4;
    if (zClipRect::ClipPolyNearZ_WithAttr0(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVertsScratch[0].x < -0.001f || g_Clip_PolyVertsScratch[0].x > 0.001f ||
        uvs[0].u < 0.499f || uvs[0].u > 0.501f || g_Clip_PolyAttr0[0] < 4.999f ||
        g_Clip_PolyAttr0[0] > 5.001f || g_Clip_PolyAttr0[1] != 10.0f ||
        g_Clip_PolyAttr0[2] != 20.0f || g_Clip_PolyVertsScratch[3].x < -0.001f ||
        g_Clip_PolyVertsScratch[3].x > 0.001f || g_Clip_PolyAttr0[3] < 24.999f ||
        g_Clip_PolyAttr0[3] > 25.001f) {
        return 1;
    }

    clipRect.flags = 0x20;
    g_Clip_PolyVertsScratch[0].z = 4.0f;
    g_Clip_PolyVertsScratch[1].z = 5.0f;
    g_Clip_PolyVertsScratch[2].z = 6.0f;
    count = 3;
    if (zClipRect::ClipPolyNearZ_WithAttr0(&clipRect, &count) != 0) {
        return 2;
    }

    clipRect.flags = 0x00;
    count = 2;
    return zClipRect::ClipPolyNearZ_WithAttr0(&clipRect, &count) == 1 && count == 2 ? 0 : 3;
}

extern "C" int zcliprect_clip_poly_zrange_attr012_smoke(void) {
    zClipUV uvs[0x40] = {};
    g_Clip_PolyUvs = uvs;
    zClipRectPartial clipRect{};
    clipRect.flags = 0x10;
    clipRect.zMin = 1.0f;
    clipRect.zMax = 4.0f;

    g_Clip_PolyVertsScratch[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVertsScratch[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVertsScratch[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVertsScratch[3] = {-1.0f, 1.0f, 0.0f};
    uvs[0] = {0.0f, 0.0f};
    uvs[1] = {1.0f, 0.0f};
    uvs[2] = {1.0f, 1.0f};
    uvs[3] = {0.0f, 1.0f};
    g_Clip_PolyAttr0[0] = 0.0f;
    g_Clip_PolyAttr0[1] = 10.0f;
    g_Clip_PolyAttr0[2] = 20.0f;
    g_Clip_PolyAttr0[3] = 30.0f;
    g_Clip_PolyAttr1[0] = 100.0f;
    g_Clip_PolyAttr1[1] = 110.0f;
    g_Clip_PolyAttr1[2] = 120.0f;
    g_Clip_PolyAttr1[3] = 130.0f;
    g_Clip_PolyAttr2[0] = 200.0f;
    g_Clip_PolyAttr2[1] = 210.0f;
    g_Clip_PolyAttr2[2] = 220.0f;
    g_Clip_PolyAttr2[3] = 230.0f;

    std::int32_t count = 4;
    if (zClipRect::ClipPolyZRange_WithAttr012(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVertsScratch[0].x < -0.001f || g_Clip_PolyVertsScratch[0].x > 0.001f ||
        g_Clip_PolyVertsScratch[0].z != 1.0f || uvs[0].u < 0.499f || uvs[0].u > 0.501f ||
        g_Clip_PolyAttr0[0] < 4.999f || g_Clip_PolyAttr0[0] > 5.001f ||
        g_Clip_PolyAttr1[0] < 104.999f || g_Clip_PolyAttr1[0] > 105.001f ||
        g_Clip_PolyAttr2[0] < 204.999f || g_Clip_PolyAttr2[0] > 205.001f ||
        g_Clip_PolyVertsScratch[3].x < -0.001f || g_Clip_PolyVertsScratch[3].x > 0.001f ||
        g_Clip_PolyAttr0[3] < 24.999f || g_Clip_PolyAttr0[3] > 25.001f ||
        g_Clip_PolyAttr1[3] < 124.999f || g_Clip_PolyAttr1[3] > 125.001f ||
        g_Clip_PolyAttr2[3] < 224.999f || g_Clip_PolyAttr2[3] > 225.001f) {
        return 1;
    }

    clipRect.flags = 0x20;
    g_Clip_PolyVertsScratch[0].z = 4.0f;
    g_Clip_PolyVertsScratch[1].z = 5.0f;
    g_Clip_PolyVertsScratch[2].z = 6.0f;
    count = 3;
    if (zClipRect::ClipPolyZRange_WithAttr012(&clipRect, &count) != 0) {
        return 2;
    }

    clipRect.flags = 0x10;
    g_Clip_PolyVertsScratch[0].z = 0.0f;
    g_Clip_PolyVertsScratch[1].z = 0.0f;
    g_Clip_PolyVertsScratch[2].z = 0.0f;
    count = 3;
    if (zClipRect::ClipPolyZRange_WithAttr012(&clipRect, &count) != 0 || count != 0) {
        return 3;
    }

    clipRect.flags = 0x00;
    count = 2;
    return zClipRect::ClipPolyZRange_WithAttr012(&clipRect, &count) == 1 && count == 2 ? 0 : 4;
}

extern "C" int zcliprect_clip_poly_zrange_no_uv_smoke(void) {
    zClipRectPartial clipRect{};
    clipRect.flags = 0x10;
    clipRect.zMin = 1.0f;
    clipRect.zMax = 4.0f;

    g_Clip_PolyVertsScratch[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVertsScratch[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVertsScratch[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVertsScratch[3] = {-1.0f, 1.0f, 0.0f};

    std::int32_t count = 4;
    if (zClipRect::ClipPolyZRange_NoUV(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVertsScratch[0].x < -0.001f || g_Clip_PolyVertsScratch[0].x > 0.001f ||
        g_Clip_PolyVertsScratch[0].z != 1.0f || g_Clip_PolyVertsScratch[3].x < -0.001f ||
        g_Clip_PolyVertsScratch[3].x > 0.001f || g_Clip_PolyVertsScratch[3].z != 1.0f) {
        return 1;
    }

    clipRect.flags = 0x20;
    g_Clip_PolyVertsScratch[0].z = 4.0f;
    g_Clip_PolyVertsScratch[1].z = 5.0f;
    g_Clip_PolyVertsScratch[2].z = 6.0f;
    count = 3;
    if (zClipRect::ClipPolyZRange_NoUV(&clipRect, &count) != 0) {
        return 2;
    }

    clipRect.flags = 0x00;
    count = 2;
    return zClipRect::ClipPolyZRange_NoUV(&clipRect, &count) == 1 && count == 2 ? 0 : 3;
}

extern "C" int zcliprect_clip_poly_zrange_no_uv_attribs_smoke(void) {
    zClipRectPartial clipRect{};
    clipRect.flags = 0x10;
    clipRect.zMin = 1.0f;
    clipRect.zMax = 4.0f;

    g_Clip_PolyVertsScratch[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVertsScratch[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVertsScratch[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVertsScratch[3] = {-1.0f, 1.0f, 0.0f};
    g_Clip_PolyAttr0[0] = 0.0f;
    g_Clip_PolyAttr0[1] = 10.0f;
    g_Clip_PolyAttr0[2] = 20.0f;
    g_Clip_PolyAttr0[3] = 30.0f;
    g_Clip_PolyAttr1[0] = 100.0f;
    g_Clip_PolyAttr1[1] = 110.0f;
    g_Clip_PolyAttr1[2] = 120.0f;
    g_Clip_PolyAttr1[3] = 130.0f;
    g_Clip_PolyAttr2[0] = 200.0f;
    g_Clip_PolyAttr2[1] = 210.0f;
    g_Clip_PolyAttr2[2] = 220.0f;
    g_Clip_PolyAttr2[3] = 230.0f;

    std::int32_t count = 4;
    if (zClipRect::ClipPolyZRange_NoUV_WithAttribs(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVertsScratch[0].x < -0.001f || g_Clip_PolyVertsScratch[0].x > 0.001f ||
        g_Clip_PolyAttr0[0] < 4.999f || g_Clip_PolyAttr0[0] > 5.001f ||
        g_Clip_PolyAttr1[0] < 104.999f || g_Clip_PolyAttr1[0] > 105.001f ||
        g_Clip_PolyAttr2[0] < 204.999f || g_Clip_PolyAttr2[0] > 205.001f ||
        g_Clip_PolyVertsScratch[3].x < -0.001f || g_Clip_PolyVertsScratch[3].x > 0.001f ||
        g_Clip_PolyAttr0[3] < 24.999f || g_Clip_PolyAttr0[3] > 25.001f ||
        g_Clip_PolyAttr1[3] < 124.999f || g_Clip_PolyAttr1[3] > 125.001f ||
        g_Clip_PolyAttr2[3] < 224.999f || g_Clip_PolyAttr2[3] > 225.001f) {
        return 1;
    }

    clipRect.flags = 0x10;
    g_Clip_PolyVertsScratch[0].z = 0.0f;
    g_Clip_PolyVertsScratch[1].z = 0.0f;
    g_Clip_PolyVertsScratch[2].z = 0.0f;
    count = 3;
    if (zClipRect::ClipPolyZRange_NoUV_WithAttribs(&clipRect, &count) != 0 || count != 0) {
        return 2;
    }

    clipRect.flags = 0x00;
    count = 2;
    return zClipRect::ClipPolyZRange_NoUV_WithAttribs(&clipRect, &count) == 1 && count == 2 ? 0 : 3;
}

extern "C" int zcliprect_clip_poly_no_uv_alt_smoke(void) {
    zClipRectPartial clipRect{};
    clipRect.flags = 0x01;
    clipRect.xMin = 0.0f;
    clipRect.yMin = 0.0f;
    clipRect.xMaxAlt = 1.0f;
    clipRect.yMaxAlt = 1.0f;

    g_Clip_PolyVerts[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVerts[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVerts[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 1.0f, 0.0f};

    std::int32_t count = 4;
    if (zClipRect::ClipPoly_NoUV_Alt(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVerts[0].x != 0.0f || g_Clip_PolyVerts[0].z != 1.0f ||
        g_Clip_PolyVerts[3].x != 0.0f || g_Clip_PolyVerts[3].y != 1.0f ||
        g_Clip_PolyVerts[3].z != 1.0f) {
        return 1;
    }

    clipRect.flags = 0x0f;
    g_Clip_PolyVerts[0] = {-1.0f, -1.0f, 0.0f};
    g_Clip_PolyVerts[1] = {2.0f, -1.0f, 3.0f};
    g_Clip_PolyVerts[2] = {2.0f, 2.0f, 6.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 2.0f, 3.0f};
    count = 4;
    if (zClipRect::ClipPoly_NoUV_Alt(&clipRect, &count) != 1 || count < 3) {
        return 2;
    }

    for (std::int32_t i = 0; i < count; ++i) {
        if (g_Clip_PolyVerts[i].x < -0.001f || g_Clip_PolyVerts[i].x > 1.001f ||
            g_Clip_PolyVerts[i].y < -0.001f || g_Clip_PolyVerts[i].y > 1.001f) {
            return 3;
        }
    }

    clipRect.flags = 0x00;
    count = 4;
    return zClipRect::ClipPoly_NoUV_Alt(&clipRect, &count) == 0 && count == 0 ? 0 : 4;
}

extern "C" int zcliprect_clip_poly_no_uv_smoke(void) {
    zClipRectPartial clipRect{};
    clipRect.flags = 0x0f;
    clipRect.xMin = 0.0f;
    clipRect.yMin = 0.0f;
    clipRect.xMaxAlt = 1.0f;
    clipRect.yMaxAlt = 1.0f;

    g_Clip_PolyVerts[0] = {-1.0f, -1.0f, 0.0f};
    g_Clip_PolyVerts[1] = {2.0f, -1.0f, 3.0f};
    g_Clip_PolyVerts[2] = {2.0f, 2.0f, 6.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 2.0f, 3.0f};

    std::int32_t count = 4;
    if (zClipRect::ClipPoly_NoUV(&clipRect, &count) != 1 || count < 3) {
        return 1;
    }

    for (std::int32_t i = 0; i < count; ++i) {
        if (g_Clip_PolyVerts[i].x < -0.001f || g_Clip_PolyVerts[i].x > 1.001f ||
            g_Clip_PolyVerts[i].y < -0.001f || g_Clip_PolyVerts[i].y > 1.001f) {
            return 2;
        }
    }

    clipRect.flags = 0x02;
    clipRect.xMaxAlt = -2.0f;
    count = 4;
    return zClipRect::ClipPoly_NoUV(&clipRect, &count) == 0 ? 0 : 3;
}

extern "C" int zcliprect_clip_poly_uv_smoke(void) {
    zClipUV uvs[0x40] = {};
    g_Clip_PolyUvs = uvs;
    zClipRectPartial clipRect{};
    clipRect.flags = 0x01;
    clipRect.xMin = 0.0f;
    clipRect.yMin = 0.0f;
    clipRect.xMaxAlt = 1.0f;
    clipRect.yMaxAlt = 1.0f;

    g_Clip_PolyVerts[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVerts[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVerts[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 1.0f, 0.0f};
    uvs[0] = {0.0f, 0.0f};
    uvs[1] = {1.0f, 0.0f};
    uvs[2] = {1.0f, 1.0f};
    uvs[3] = {0.0f, 1.0f};

    std::int32_t count = 4;
    if (zClipRect::ClipPoly(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVerts[0].x != 0.0f || uvs[0].u < 0.499f || uvs[0].u > 0.501f ||
        g_Clip_PolyVerts[3].x != 0.0f || uvs[3].u < 0.499f || uvs[3].u > 0.501f ||
        uvs[3].v != 1.0f) {
        return 1;
    }

    clipRect.flags = 0x0f;
    g_Clip_PolyVerts[0] = {-1.0f, -1.0f, 0.0f};
    g_Clip_PolyVerts[1] = {2.0f, -1.0f, 3.0f};
    g_Clip_PolyVerts[2] = {2.0f, 2.0f, 6.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 2.0f, 3.0f};
    uvs[0] = {0.0f, 0.0f};
    uvs[1] = {1.0f, 0.0f};
    uvs[2] = {1.0f, 1.0f};
    uvs[3] = {0.0f, 1.0f};
    count = 4;
    if (zClipRect::ClipPoly(&clipRect, &count) != 1 || count < 3) {
        return 2;
    }

    for (std::int32_t i = 0; i < count; ++i) {
        if (g_Clip_PolyVerts[i].x < -0.001f || g_Clip_PolyVerts[i].x > 1.001f ||
            g_Clip_PolyVerts[i].y < -0.001f || g_Clip_PolyVerts[i].y > 1.001f ||
            uvs[i].u < -0.001f || uvs[i].u > 1.001f || uvs[i].v < -0.001f || uvs[i].v > 1.001f) {
            return 3;
        }
    }

    clipRect.flags = 0x00;
    count = 4;
    return zClipRect::ClipPoly(&clipRect, &count) == 0 && count == 0 ? 0 : 4;
}

extern "C" int zcliprect_clip_poly_uv_attr012_smoke(void) {
    zClipUV uvs[0x40] = {};
    g_Clip_PolyUvs = uvs;
    zClipRectPartial clipRect{};
    clipRect.flags = 0x01;
    clipRect.xMin = 0.0f;
    clipRect.yMin = 0.0f;
    clipRect.xMaxAlt = 1.0f;
    clipRect.yMaxAlt = 1.0f;

    g_Clip_PolyVerts[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVerts[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVerts[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 1.0f, 0.0f};
    uvs[0] = {0.0f, 0.0f};
    uvs[1] = {1.0f, 0.0f};
    uvs[2] = {1.0f, 1.0f};
    uvs[3] = {0.0f, 1.0f};
    g_Clip_PolyAttr0[0] = 0.0f;
    g_Clip_PolyAttr0[1] = 10.0f;
    g_Clip_PolyAttr0[2] = 20.0f;
    g_Clip_PolyAttr0[3] = 30.0f;
    g_Clip_PolyAttr1[0] = 100.0f;
    g_Clip_PolyAttr1[1] = 110.0f;
    g_Clip_PolyAttr1[2] = 120.0f;
    g_Clip_PolyAttr1[3] = 130.0f;
    g_Clip_PolyAttr2[0] = 200.0f;
    g_Clip_PolyAttr2[1] = 210.0f;
    g_Clip_PolyAttr2[2] = 220.0f;
    g_Clip_PolyAttr2[3] = 230.0f;

    std::int32_t count = 4;
    if (zClipRect::ClipPoly_WithAttr012(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVerts[0].x != 0.0f || uvs[0].u < 0.499f || uvs[0].u > 0.501f ||
        g_Clip_PolyAttr0[0] < 4.999f || g_Clip_PolyAttr0[0] > 5.001f ||
        g_Clip_PolyAttr1[0] < 104.999f || g_Clip_PolyAttr1[0] > 105.001f ||
        g_Clip_PolyAttr2[0] < 204.999f || g_Clip_PolyAttr2[0] > 205.001f ||
        g_Clip_PolyVerts[3].x != 0.0f || uvs[3].u < 0.499f || uvs[3].u > 0.501f ||
        g_Clip_PolyAttr0[3] < 24.999f || g_Clip_PolyAttr0[3] > 25.001f ||
        g_Clip_PolyAttr1[3] < 124.999f || g_Clip_PolyAttr1[3] > 125.001f ||
        g_Clip_PolyAttr2[3] < 224.999f || g_Clip_PolyAttr2[3] > 225.001f) {
        return 1;
    }

    clipRect.flags = 0x0f;
    g_Clip_PolyVerts[0] = {-1.0f, -1.0f, 0.0f};
    g_Clip_PolyVerts[1] = {2.0f, -1.0f, 3.0f};
    g_Clip_PolyVerts[2] = {2.0f, 2.0f, 6.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 2.0f, 3.0f};
    uvs[0] = {0.0f, 0.0f};
    uvs[1] = {1.0f, 0.0f};
    uvs[2] = {1.0f, 1.0f};
    uvs[3] = {0.0f, 1.0f};
    g_Clip_PolyAttr0[0] = 0.0f;
    g_Clip_PolyAttr0[1] = 3.0f;
    g_Clip_PolyAttr0[2] = 6.0f;
    g_Clip_PolyAttr0[3] = 3.0f;
    g_Clip_PolyAttr1[0] = 10.0f;
    g_Clip_PolyAttr1[1] = 13.0f;
    g_Clip_PolyAttr1[2] = 16.0f;
    g_Clip_PolyAttr1[3] = 13.0f;
    g_Clip_PolyAttr2[0] = 20.0f;
    g_Clip_PolyAttr2[1] = 23.0f;
    g_Clip_PolyAttr2[2] = 26.0f;
    g_Clip_PolyAttr2[3] = 23.0f;
    count = 4;
    if (zClipRect::ClipPoly_WithAttr012(&clipRect, &count) != 1 || count < 3) {
        return 2;
    }

    for (std::int32_t i = 0; i < count; ++i) {
        if (g_Clip_PolyVerts[i].x < -0.001f || g_Clip_PolyVerts[i].x > 1.001f ||
            g_Clip_PolyVerts[i].y < -0.001f || g_Clip_PolyVerts[i].y > 1.001f ||
            uvs[i].u < -0.001f || uvs[i].u > 1.001f || uvs[i].v < -0.001f || uvs[i].v > 1.001f ||
            g_Clip_PolyAttr0[i] < -0.001f || g_Clip_PolyAttr0[i] > 6.001f ||
            g_Clip_PolyAttr1[i] < 9.999f || g_Clip_PolyAttr1[i] > 16.001f ||
            g_Clip_PolyAttr2[i] < 19.999f || g_Clip_PolyAttr2[i] > 26.001f) {
            return 3;
        }
    }

    clipRect.flags = 0x00;
    count = 4;
    return zClipRect::ClipPoly_WithAttr012(&clipRect, &count) == 0 && count == 0 ? 0 : 4;
}

extern "C" int zcliprect_clip_poly_no_uv_attr0_alt_smoke(void) {
    zClipRectPartial clipRect{};
    clipRect.flags = 0x01;
    clipRect.xMin = 0.0f;
    clipRect.yMin = 0.0f;
    clipRect.xMaxAlt = 1.0f;
    clipRect.yMaxAlt = 1.0f;

    g_Clip_PolyVerts[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVerts[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVerts[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 1.0f, 0.0f};
    g_Clip_PolyAttr0[0] = 0.0f;
    g_Clip_PolyAttr0[1] = 10.0f;
    g_Clip_PolyAttr0[2] = 20.0f;
    g_Clip_PolyAttr0[3] = 30.0f;

    std::int32_t count = 4;
    if (zClipRect::ClipPoly_NoUV_WithAttr0_Alt(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVerts[0].x != 0.0f || g_Clip_PolyAttr0[0] < 4.999f ||
        g_Clip_PolyAttr0[0] > 5.001f || g_Clip_PolyVerts[3].x != 0.0f ||
        g_Clip_PolyAttr0[3] < 24.999f || g_Clip_PolyAttr0[3] > 25.001f) {
        return 1;
    }

    clipRect.flags = 0x0f;
    g_Clip_PolyVerts[0] = {-1.0f, -1.0f, 0.0f};
    g_Clip_PolyVerts[1] = {2.0f, -1.0f, 3.0f};
    g_Clip_PolyVerts[2] = {2.0f, 2.0f, 6.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 2.0f, 3.0f};
    g_Clip_PolyAttr0[0] = 0.0f;
    g_Clip_PolyAttr0[1] = 3.0f;
    g_Clip_PolyAttr0[2] = 6.0f;
    g_Clip_PolyAttr0[3] = 3.0f;
    count = 4;
    if (zClipRect::ClipPoly_NoUV_WithAttr0_Alt(&clipRect, &count) != 1 || count < 3) {
        return 2;
    }

    for (std::int32_t i = 0; i < count; ++i) {
        if (g_Clip_PolyVerts[i].x < -0.001f || g_Clip_PolyVerts[i].x > 1.001f ||
            g_Clip_PolyVerts[i].y < -0.001f || g_Clip_PolyVerts[i].y > 1.001f ||
            g_Clip_PolyAttr0[i] < -0.001f || g_Clip_PolyAttr0[i] > 6.001f) {
            return 3;
        }
    }

    clipRect.flags = 0x00;
    count = 4;
    return zClipRect::ClipPoly_NoUV_WithAttr0_Alt(&clipRect, &count) == 0 && count == 0 ? 0 : 4;
}

extern "C" int zcliprect_clip_poly_no_uv_attr012_alt_smoke(void) {
    zClipRectPartial clipRect{};
    clipRect.flags = 0x01;
    clipRect.xMin = 0.0f;
    clipRect.yMin = 0.0f;
    clipRect.xMaxAlt = 1.0f;
    clipRect.yMaxAlt = 1.0f;

    g_Clip_PolyVerts[0] = {-1.0f, 0.0f, 0.0f};
    g_Clip_PolyVerts[1] = {1.0f, 0.0f, 2.0f};
    g_Clip_PolyVerts[2] = {1.0f, 1.0f, 2.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 1.0f, 0.0f};
    g_Clip_PolyAttr0[0] = 0.0f;
    g_Clip_PolyAttr0[1] = 10.0f;
    g_Clip_PolyAttr0[2] = 20.0f;
    g_Clip_PolyAttr0[3] = 30.0f;
    g_Clip_PolyAttr1[0] = 100.0f;
    g_Clip_PolyAttr1[1] = 110.0f;
    g_Clip_PolyAttr1[2] = 120.0f;
    g_Clip_PolyAttr1[3] = 130.0f;
    g_Clip_PolyAttr2[0] = 200.0f;
    g_Clip_PolyAttr2[1] = 210.0f;
    g_Clip_PolyAttr2[2] = 220.0f;
    g_Clip_PolyAttr2[3] = 230.0f;

    std::int32_t count = 4;
    if (zClipRect::ClipPoly_NoUV_WithAttr012_Alt(&clipRect, &count) != 1 || count != 4 ||
        g_Clip_PolyVerts[0].x != 0.0f || g_Clip_PolyAttr0[0] < 4.999f ||
        g_Clip_PolyAttr0[0] > 5.001f || g_Clip_PolyAttr1[0] < 104.999f ||
        g_Clip_PolyAttr1[0] > 105.001f || g_Clip_PolyAttr2[0] < 204.999f ||
        g_Clip_PolyAttr2[0] > 205.001f || g_Clip_PolyVerts[3].x != 0.0f ||
        g_Clip_PolyAttr0[3] < 24.999f || g_Clip_PolyAttr0[3] > 25.001f ||
        g_Clip_PolyAttr1[3] < 124.999f || g_Clip_PolyAttr1[3] > 125.001f ||
        g_Clip_PolyAttr2[3] < 224.999f || g_Clip_PolyAttr2[3] > 225.001f) {
        return 1;
    }

    clipRect.flags = 0x0f;
    g_Clip_PolyVerts[0] = {-1.0f, -1.0f, 0.0f};
    g_Clip_PolyVerts[1] = {2.0f, -1.0f, 3.0f};
    g_Clip_PolyVerts[2] = {2.0f, 2.0f, 6.0f};
    g_Clip_PolyVerts[3] = {-1.0f, 2.0f, 3.0f};
    g_Clip_PolyAttr0[0] = 0.0f;
    g_Clip_PolyAttr0[1] = 3.0f;
    g_Clip_PolyAttr0[2] = 6.0f;
    g_Clip_PolyAttr0[3] = 3.0f;
    g_Clip_PolyAttr1[0] = 10.0f;
    g_Clip_PolyAttr1[1] = 13.0f;
    g_Clip_PolyAttr1[2] = 16.0f;
    g_Clip_PolyAttr1[3] = 13.0f;
    g_Clip_PolyAttr2[0] = 20.0f;
    g_Clip_PolyAttr2[1] = 23.0f;
    g_Clip_PolyAttr2[2] = 26.0f;
    g_Clip_PolyAttr2[3] = 23.0f;
    count = 4;
    if (zClipRect::ClipPoly_NoUV_WithAttr012_Alt(&clipRect, &count) != 1 || count < 3) {
        return 2;
    }

    for (std::int32_t i = 0; i < count; ++i) {
        if (g_Clip_PolyVerts[i].x < -0.001f || g_Clip_PolyVerts[i].x > 1.001f ||
            g_Clip_PolyVerts[i].y < -0.001f || g_Clip_PolyVerts[i].y > 1.001f ||
            g_Clip_PolyAttr0[i] < -0.001f || g_Clip_PolyAttr0[i] > 6.001f ||
            g_Clip_PolyAttr1[i] < 9.999f || g_Clip_PolyAttr1[i] > 16.001f ||
            g_Clip_PolyAttr2[i] < 19.999f || g_Clip_PolyAttr2[i] > 26.001f) {
            return 3;
        }
    }

    clipRect.flags = 0x00;
    count = 4;
    return zClipRect::ClipPoly_NoUV_WithAttr012_Alt(&clipRect, &count) == 0 && count == 0 ? 0 : 4;
}

extern "C" int zcliprect_trivial_reject_poly_xy_smoke(void) {
    zClipRectPartial clipRect{};
    clipRect.flags = 0x0f;
    clipRect.xMin = 0.0f;
    clipRect.yMin = 0.0f;
    clipRect.xMax = 10.0f;
    clipRect.yMax = 8.0f;

    g_Clip_PolyVerts[0] = {1.0f, 1.0f, 0.0f};
    g_Clip_PolyVerts[1] = {2.0f, 1.0f, 0.0f};
    g_Clip_PolyVerts[2] = {1.0f, 2.0f, 0.0f};
    if (zClipRect::TrivialRejectPolyXY(&clipRect, 3) != 1) {
        return 1;
    }

    g_Clip_PolyVerts[0].x = -3.0f;
    g_Clip_PolyVerts[1].x = -2.0f;
    g_Clip_PolyVerts[2].x = -1.0f;
    if (zClipRect::TrivialRejectPolyXY(&clipRect, 3) != 0) {
        return 2;
    }

    g_Clip_PolyVerts[0] = {10.0f, 1.0f, 0.0f};
    g_Clip_PolyVerts[1] = {11.0f, 1.0f, 0.0f};
    g_Clip_PolyVerts[2] = {12.0f, 2.0f, 0.0f};
    if (zClipRect::TrivialRejectPolyXY(&clipRect, 3) != 0) {
        return 3;
    }

    g_Clip_PolyVerts[0] = {1.0f, -3.0f, 0.0f};
    g_Clip_PolyVerts[1] = {2.0f, -2.0f, 0.0f};
    g_Clip_PolyVerts[2] = {3.0f, -1.0f, 0.0f};
    if (zClipRect::TrivialRejectPolyXY(&clipRect, 3) != 0) {
        return 4;
    }

    g_Clip_PolyVerts[0] = {1.0f, 8.0f, 0.0f};
    g_Clip_PolyVerts[1] = {2.0f, 9.0f, 0.0f};
    g_Clip_PolyVerts[2] = {3.0f, 10.0f, 0.0f};
    if (zClipRect::TrivialRejectPolyXY(&clipRect, 3) != 0) {
        return 5;
    }

    clipRect.flags = 0;
    return zClipRect::TrivialRejectPolyXY(&clipRect, 0) == 1 ? 0 : 6;
}

extern "C" int zrndr_span_occlusion_reset_shutdown_smoke(void) {
    zRndr::g_spanOccluderPolyCount = 5;
    zRndr::SpanOcclusionResetFrame();
    if (zRndr::g_spanOccluderPolyCount != 0) {
        return 1;
    }

    zRndr::g_spanColumnHeadTable =
        static_cast<zRndr::SpanNodePartial **>(std::malloc(sizeof(zRndr::SpanNodePartial *) * 4));
    zRndr::g_spanPoolBase =
        static_cast<zRndr::SpanNodePartial *>(std::malloc(sizeof(zRndr::SpanNodePartial) * 4));
    if (zRndr::g_spanColumnHeadTable == nullptr || zRndr::g_spanPoolBase == nullptr) {
        std::free(zRndr::g_spanColumnHeadTable);
        std::free(zRndr::g_spanPoolBase);
        zRndr::g_spanColumnHeadTable = nullptr;
        zRndr::g_spanPoolBase = nullptr;
        return 2;
    }

    zRndr::SpanOcclusionShutdown();
    if (zRndr::g_spanColumnHeadTable != nullptr || zRndr::g_spanPoolBase != nullptr) {
        return 3;
    }

    zRndr::SpanOcclusionShutdown();
    return 0;
}

extern "C" int zrndr_span_occlusion_init_build_smoke(void) {
    zRndr::SpanOcclusionShutdown();
    zRndr::g_spanOccluderPolyCount = 0;
    zRndr::g_spanColumnCount = -1;
    zRndr::g_spanColumnCountPadded = -1;
    zRndr::g_pfnBuildSpanList = nullptr;
    zRndr::g_pfnBuildSpanListSecondary = nullptr;

    if (zRndr::SpanOcclusionInit(3) != 0) {
        return 1;
    }

    if (zRndr::g_spanColumnCount != 3 || zRndr::g_spanColumnCountPadded != 0x83 ||
        zRndr::g_spanColumnHeadTable == nullptr || zRndr::g_spanPoolBase == nullptr ||
        zRndr::g_spanAllocCursor != zRndr::g_spanPoolBase || zRndr::g_spanOccluderPolyCount != 0 ||
        zRndr::g_pfnBuildSpanList != zRndr_SpanOcclusion_InsertSpanNode_Local ||
        zRndr::g_pfnBuildSpanListSecondary != zRndr_SpanOcclusion_BuildSpanList) {
        zRndr::SpanOcclusionShutdown();
        return 2;
    }

    for (int i = 0; i < 3; ++i) {
        if (zRndr::g_spanColumnHeadTable[i] != nullptr) {
            zRndr::SpanOcclusionShutdown();
            return 3;
        }
    }

    zRndr::SpanOcclusionShutdown();
    return 0;
}

extern "C" int zrndr_span_occlusion_rasterize_smoke(void) {
    zRndr::SpanOcclusionShutdown();
    if (zRndr::SpanOcclusionInit(8) != 0) {
        return 1;
    }

    zRndr::SpanOccluderPolyPartial poly{};
    poly.vertices[0][0] = 1.0f;
    poly.vertices[0][1] = 1.0f;
    poly.vertices[0][2] = 0.5f;
    poly.vertices[1][0] = 5.0f;
    poly.vertices[1][1] = 1.0f;
    poly.vertices[1][2] = 0.5f;
    poly.vertices[2][0] = 5.0f;
    poly.vertices[2][1] = 5.0f;
    poly.vertices[2][2] = 0.5f;
    poly.vertices[3][0] = 1.0f;
    poly.vertices[3][1] = 5.0f;
    poly.vertices[3][2] = 0.5f;
    poly.vertCount = 4;

    zRndr::SpanOcclusionRasterizeOccluderPoly(&poly, poly.vertCount);
    if (zRndr::g_spanAllocCursor == zRndr::g_spanPoolBase) {
        zRndr::SpanOcclusionShutdown();
        return 2;
    }

    for (int y = 1; y <= 4; ++y) {
        zRndr::SpanNodePartial *head = zRndr::g_spanColumnHeadTable[y];
        if (head == nullptr || head->sampleXMin != 1 || head->sampleXMax != 4 ||
            head->invDepth != 0.5f || head->invDepthStep != 0.5f || head->depthSlope != 0.0f) {
            zRndr::SpanOcclusionShutdown();
            return 3 + y;
        }
    }

    zRndr::SpanOcclusionShutdown();
    if (zRndr::SpanOcclusionInit(8) != 0) {
        return 8;
    }

    zRndr::SpanOccluderPolyPartial triangle{};
    triangle.vertices[0][0] = 1.0f;
    triangle.vertices[0][1] = 1.0f;
    triangle.vertices[0][2] = 0.25f;
    triangle.vertices[1][0] = 5.0f;
    triangle.vertices[1][1] = 1.0f;
    triangle.vertices[1][2] = 0.25f;
    triangle.vertices[2][0] = 5.0f;
    triangle.vertices[2][1] = 1.0f;
    triangle.vertices[2][2] = 0.25f;
    triangle.vertices[3][0] = 1.0f;
    triangle.vertices[3][1] = 5.0f;
    triangle.vertices[3][2] = 0.25f;
    triangle.vertices[4][0] = 1.0f;
    triangle.vertices[4][1] = 1.0f;
    triangle.vertices[4][2] = 0.25f;
    triangle.vertCount = 5;

    zRndr::SpanOcclusionRasterizeOccluderPoly(&triangle, triangle.vertCount);
    const int expectedTriangleMax[3] = {3, 2, 1};
    for (int y = 1; y <= 3; ++y) {
        zRndr::SpanNodePartial *head = zRndr::g_spanColumnHeadTable[y];
        if (head == nullptr || head->sampleXMin != 1 ||
            head->sampleXMax != expectedTriangleMax[y - 1] || head->invDepth != 0.25f ||
            head->invDepthStep != 0.25f || head->depthSlope != 0.0f) {
            zRndr::SpanOcclusionShutdown();
            return 8 + y;
        }
    }

    if (zRndr::g_spanColumnHeadTable[4] != nullptr) {
        zRndr::SpanOcclusionShutdown();
        return 12;
    }

    zRndr::SpanOcclusionShutdown();
    return 0;
}

extern "C" int zrndr_rasterize_poly_with_span_list_smoke(void) {
    std::uint16_t frame[0x100] = {};
    zRndr::SpanNodePartial spanPool[8] = {};
    zVec3 vertices[3] = {
        {1.0f, 1.0f, 2.0f},
        {5.0f, 1.0f, 2.0f},
        {1.0f, 4.0f, 2.0f},
    };
    zVec3 planeVerts[3] = {
        {1.0f, 1.0f, 2.0f},
        {5.0f, 1.0f, 2.0f},
        {1.0f, 4.0f, 2.0f},
    };

    g_rasterSpanBuildCount = 0;
    g_rasterSpanOpCount = 0;
    g_rasterLastColumn = 0;
    g_rasterLastContext = 0;
    g_rasterLastPixelCount = 0;
    g_rasterLastNode = {};
    g_rasterLastDst = nullptr;

    zRndr::g_frameBuffer = frame;
    zRndr::g_pitchBytes = 32;
    zRndr::g_bytesPerPixel = 2;
    zRndr::g_inverseDepthBias = 1.0f;
    zRndr::g_inverseDepthScale = 2.0f;
    zRndr::g_spanAllocCursor = spanPool;
    zRndr::g_pfnBuildSpanList = TestRasterBuildSpanList;
    zRndr::g_pfnSelectedSpanOp = TestRasterSelectedSpanOp;

    zRndr_RasterizePolyWithSpanList(vertices, planeVerts, 3, 0x77);
    zRndr::g_pfnBuildSpanList = nullptr;
    zRndr::g_pfnSelectedSpanOp = nullptr;

    if (g_rasterSpanBuildCount <= 0 || g_rasterSpanOpCount <= 0 || g_rasterLastContext != 0x77) {
        return 1;
    }

    if (g_rasterLastNode.sampleXMin > g_rasterLastNode.sampleXMax ||
        g_rasterLastNode.invDepth < 4.99f || g_rasterLastNode.invDepth > 5.01f ||
        g_rasterLastNode.invDepthStep < 4.99f || g_rasterLastNode.invDepthStep > 5.01f ||
        g_rasterLastNode.depthSlope != 0.0f) {
        return 2;
    }

    const std::uint8_t *expectedDst = reinterpret_cast<std::uint8_t *>(frame) +
                                      g_rasterLastColumn * zRndr::g_pitchBytes +
                                      g_rasterLastNode.sampleXMin * zRndr::g_bytesPerPixel;
    if (reinterpret_cast<const std::uint8_t *>(g_rasterLastDst) != expectedDst ||
        g_rasterLastPixelCount != g_rasterLastNode.sampleXMax - g_rasterLastNode.sampleXMin + 1) {
        return 3;
    }

    return 0;
}

extern "C" int zrndr_draw_flat_immediate_smoke(void) {
    std::uint16_t frame[0x100] = {};
    zRndr::SpanNodePartial spanPool[8] = {};
    zVec3 vertices[3] = {
        {1.0f, 1.0f, 2.0f},
        {5.0f, 1.0f, 2.0f},
        {1.0f, 4.0f, 2.0f},
    };
    zVec3 planeVertices[3] = {
        {1.0f, 1.0f, 2.0f},
        {5.0f, 1.0f, 2.0f},
        {1.0f, 4.0f, 2.0f},
    };

    g_rasterSpanBuildCount = 0;
    g_flatImmediateSpanOpCount = 0;
    g_flatImmediateLastEcxArg = 0;
    g_flatImmediateLastEdxArg = 0;
    g_rasterLastPixelCount = 0;
    g_rasterLastColumn = 0;
    g_rasterLastNode = {};
    g_rasterLastDst = nullptr;

    zRndr::g_frameBuffer = frame;
    zRndr::g_pitchBytes = 32;
    zRndr::g_bytesPerPixel = 2;
    zRndr::g_inverseDepthBias = 1.0f;
    zRndr::g_inverseDepthScale = 2.0f;
    zRndr::g_spanAllocCursor = spanPool;
    zRndr::g_pfnBuildSpanListSecondary = TestRasterBuildSpanList;
    zRndr::g_pfnFlatImmediateSpanOp = TestFlatImmediateSpanOp;

    zRndr_DrawFlatImmediate(vertices, planeVertices, 3, 0x22, 0x11);
    zRndr::g_pfnBuildSpanListSecondary = nullptr;
    zRndr::g_pfnFlatImmediateSpanOp = nullptr;

    if (g_rasterSpanBuildCount <= 0 || g_flatImmediateSpanOpCount <= 0 ||
        g_flatImmediateLastEcxArg != 0x11 || g_flatImmediateLastEdxArg != 0x22) {
        return 1;
    }

    if (g_rasterLastNode.sampleXMin > g_rasterLastNode.sampleXMax ||
        g_rasterLastNode.invDepth < 4.99f || g_rasterLastNode.invDepth > 5.01f ||
        g_rasterLastNode.invDepthStep < 4.99f || g_rasterLastNode.invDepthStep > 5.01f ||
        g_rasterLastNode.depthSlope != 0.0f) {
        return 2;
    }

    const std::uint8_t *expectedDst = reinterpret_cast<std::uint8_t *>(frame) +
                                      g_rasterLastColumn * zRndr::g_pitchBytes +
                                      g_rasterLastNode.sampleXMin * zRndr::g_bytesPerPixel;
    if (reinterpret_cast<const std::uint8_t *>(g_rasterLastDst) != expectedDst ||
        g_rasterLastPixelCount != g_rasterLastNode.sampleXMax - g_rasterLastNode.sampleXMin + 1) {
        return 3;
    }

    return 0;
}

extern "C" int zrndr_rasterize_poly_smoke(void) {
    std::uint16_t frame[8 * 8] = {};
    zVec3 vertices[4] = {
        {1.0f, 1.0f, 0.0f},
        {5.0f, 1.0f, 0.0f},
        {5.0f, 1.0f, 0.0f},
        {1.0f, 5.0f, 0.0f},
    };

    g_rasterSpanOpCount = 0;
    g_rasterLastContext = 0;
    g_rasterLastPixelCount = 0;
    g_rasterLastDst = nullptr;

    zRndr::g_frameBuffer = frame;
    zRndr::g_pitchBytes = 8 * static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_bytesPerPixel = static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_pfnSelectedSpanOp = TestRasterSelectedSpanOp;

    zRndr_RasterizePoly(vertices, 4, 0x3344);
    zRndr::g_pfnSelectedSpanOp = nullptr;

    return g_rasterSpanOpCount > 0 && g_rasterLastContext == 0x3344 && g_rasterLastPixelCount > 0 &&
                   g_rasterLastDst >= frame && g_rasterLastDst < frame + 64
               ? 0
               : 1;
}

extern "C" int zrndr_submit_poly_with_span_list_smoke(void) {
    zVec3 vertices[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zVec3 planeVerts[3] = {
        {1.5f, 2.5f, 3.5f},
        {4.5f, 5.5f, 6.5f},
        {7.5f, 8.5f, 9.5f},
    };

    zRndr::g_overwriteQueueCount = 0;
    zRndr::g_transparentQueueCount = 0;
    zRndr::g_overwriteQueue[0] = {};
    zRndr::g_transparentQueue[0] = {};
    zRndr::g_scanConvertMode = 2;
    zRndr::g_inverseDepthBias = 11.0f;
    zRndr::g_inverseDepthScale = 13.0f;

    zRndr_SubmitPolyWithSpanList(vertices, planeVerts, 0x22, 128, 3, 1);
    if (zRndr::g_overwriteQueueCount != 1 || zRndr::g_transparentQueueCount != 0) {
        return 1;
    }

    const zRndr::OverwriteQueuedPolyDrawCmd &overwrite = zRndr::g_overwriteQueue[0];
    if (overwrite.hasClippedTriVerts != 0 || overwrite.vertexCount != 3 ||
        overwrite.shadeOrSpanMode != 0x22 || overwrite.alphaOrShadeF != 128.0f ||
        overwrite.materialRef != nullptr || overwrite.scanConvertMode != 2 ||
        overwrite.savedInvDepthBias != 11.0f || overwrite.savedInvDepthScale != 13.0f ||
        !SameQueuedVec3(overwrite.polyVerts[1], vertices[1]) ||
        !SameQueuedVec3(overwrite.triVerts[2], planeVerts[2])) {
        return 2;
    }

    zRndr::g_scanConvertMode = 5;
    zRndr::g_inverseDepthBias = 17.0f;
    zRndr::g_inverseDepthScale = 19.0f;
    zRndr_SubmitPolyWithSpanList(vertices, planeVerts, 0x33, 0x80, 3, 0);
    if (zRndr::g_overwriteQueueCount != 1 || zRndr::g_transparentQueueCount != 1) {
        return 3;
    }

    const zRndr::TransparentQueuedPolyDrawCmd &transparent = zRndr::g_transparentQueue[0];
    if (transparent.materialRef != nullptr || transparent.vertexCount != 3 ||
        transparent.shadeOrSpanMode != 0x33 || transparent.alphaOrShadeBits != 0x80 ||
        transparent.scanConvertMode != 5 || transparent.savedInvDepthBias != 17.0f ||
        transparent.savedInvDepthScale != 19.0f ||
        !SameQueuedVec3(transparent.polyVerts[0], vertices[0]) ||
        !SameQueuedVec3(transparent.triVerts[1], planeVerts[1])) {
        return 4;
    }

    zRndr_SubmitPolyWithSpanList(vertices, planeVerts, 0x44, 0xff, 3, 0);
    if (zRndr::g_overwriteQueueCount != 1 || zRndr::g_transparentQueueCount != 1) {
        return 5;
    }

    return 0;
}

extern "C" int zrndr_submit_textured_poly_uniform_smoke(void) {
    zVec3 projected[4] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
        {10.0f, 11.0f, 12.0f},
    };
    zVec3 clipped[3] = {
        {21.0f, 22.0f, 23.0f},
        {24.0f, 25.0f, 26.0f},
        {27.0f, 28.0f, 29.0f},
    };
    zVec3 triData[3] = {
        {31.0f, 32.0f, 33.0f},
        {34.0f, 35.0f, 36.0f},
        {37.0f, 38.0f, 39.0f},
    };
    zVec2 triUVs[3] = {
        {0.25f, 0.5f},
        {0.75f, 1.0f},
        {1.25f, 1.5f},
    };
    zVidImagePartial image{};
    image.formatFlagsPacked = 2;
    zImage_TexDirEntryPartial entry{};
    entry.image = &image;

    zRndr::g_overwriteQueueCount = 0;
    zRndr::g_transparentQueueCount = 0;
    zRndr::g_overwriteQueue[0] = {};
    zRndr::g_transparentQueue[0] = {};
    zRndr::g_scanConvertMode = 6;
    zRndr::g_inverseDepthBias = 2.5f;
    zRndr::g_inverseDepthScale = 4.5f;
    g_zRndr_ActivePaletteRemapKey = 0x2a;

    zRndr_SubmitTexturedPolyUniformAlphaOrShade(projected, clipped, triData, triUVs, 4, &entry,
                                                0.75f, 1);
    const zRndr::OverwriteQueuedPolyDrawCmd &overwrite = zRndr::g_overwriteQueue[0];
    const std::uint8_t *overwriteRaw = reinterpret_cast<const std::uint8_t *>(&overwrite);
    const zRndr::QueuedVec3 *overwriteClipped =
        reinterpret_cast<const zRndr::QueuedVec3 *>(overwriteRaw + 0x304);
    const float *overwriteUvs = reinterpret_cast<const float *>(overwriteRaw + 0x358);
    if (zRndr::g_overwriteQueueCount != 1 ||
        *reinterpret_cast<const std::int32_t *>(overwriteRaw + 0x000) != 1 ||
        overwrite.vertexCount != 4 || overwrite.materialRef != &entry ||
        overwrite.alphaOrShadeF != 0.75f || overwrite.scanConvertMode != 6 ||
        overwrite.savedInvDepthBias != 2.5f || overwrite.savedInvDepthScale != 4.5f ||
        *reinterpret_cast<const std::int32_t *>(overwriteRaw + 0x47c) != 1 ||
        *reinterpret_cast<const std::int32_t *>(overwriteRaw + 0x488) != 0x2a ||
        !SameQueuedVec3(overwrite.polyVerts[3], projected[3]) ||
        !SameQueuedVec3(overwrite.triVerts[1], triData[1]) ||
        !SameQueuedVec3(overwriteClipped[2], clipped[2]) || overwriteUvs[4] != triUVs[2].x ||
        overwriteUvs[5] != triUVs[2].y) {
        return 1;
    }

    zRndr::g_transparentQueueCount = 0;
    zRndr::g_transparentQueue[0] = {};
    zRndr::g_scanConvertMode = 7;
    zRndr::g_inverseDepthBias = 8.5f;
    zRndr::g_inverseDepthScale = 9.5f;
    g_zRndr_ActivePaletteRemapKey = 0x31;

    zRndr_SubmitTexturedPolyUniformAlphaOrShade(projected, nullptr, triData, triUVs, 3, &entry,
                                                0.5f, 0);
    const zRndr::TransparentQueuedPolyDrawCmd &transparent = zRndr::g_transparentQueue[0];
    std::int32_t alphaBits = 0;
    const float halfAlpha = 0.5f;
    std::memcpy(&alphaBits, &halfAlpha, sizeof(float));
    if (zRndr::g_transparentQueueCount != 1 || transparent.materialRef != &entry ||
        transparent.vertexCount != 3 || transparent.alphaOrShadeBits != alphaBits ||
        transparent.hasClippedTriVerts != 0 || transparent.scanConvertMode != 7 ||
        transparent.savedInvDepthBias != 8.5f || transparent.savedInvDepthScale != 9.5f ||
        transparent.texKey != 0x31 || !SameQueuedVec3(transparent.polyVerts[2], projected[2]) ||
        !SameQueuedVec3(transparent.triVerts[0], triData[0]) ||
        transparent.triUVs[4] != triUVs[2].x || transparent.triUVs[5] != triUVs[2].y) {
        return 2;
    }

    return 0;
}

extern "C" int zrndr_submit_textured_poly_per_vertex_smoke(void) {
    zVec3 projected[4] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
        {10.0f, 11.0f, 12.0f},
    };
    zVec3 clipped[3] = {
        {21.0f, 22.0f, 23.0f},
        {24.0f, 25.0f, 26.0f},
        {27.0f, 28.0f, 29.0f},
    };
    zVec3 triData[3] = {
        {31.0f, 32.0f, 33.0f},
        {34.0f, 35.0f, 36.0f},
        {37.0f, 38.0f, 39.0f},
    };
    zVec2 triUVs[3] = {
        {0.25f, 0.5f},
        {0.75f, 1.0f},
        {1.25f, 1.5f},
    };
    float perVertex[4] = {8.0f, 16.0f, 24.0f, 32.0f};
    zVidImagePartial image{};
    image.formatFlagsPacked = 2;
    zImage_TexDirEntryPartial entry{};
    entry.image = &image;

    zRndr::g_overwriteQueueCount = 0;
    zRndr::g_transparentQueueCount = 0;
    zRndr::g_overwriteQueue[0] = {};
    zRndr::g_transparentQueue[0] = {};
    zRndr::g_scanConvertMode = 3;
    zRndr::g_inverseDepthBias = 4.0f;
    zRndr::g_inverseDepthScale = 5.0f;
    g_zRndr_ActivePaletteRemapKey = 0x40;

    zRndr_SubmitTexturedPolyPerVertexAlphaOrShade(projected, clipped, triData, triUVs, perVertex,
                                                  77, 4, &entry, 1, 1);
    const zRndr::OverwriteQueuedPolyDrawCmd &overwrite = zRndr::g_overwriteQueue[0];
    const std::uint8_t *overwriteRaw = reinterpret_cast<const std::uint8_t *>(&overwrite);
    const zRndr::QueuedVec3 *overwriteClipped =
        reinterpret_cast<const zRndr::QueuedVec3 *>(overwriteRaw + 0x304);
    const float *overwritePerVertex = reinterpret_cast<const float *>(overwriteRaw + 0x374);
    if (zRndr::g_overwriteQueueCount != 1 ||
        *reinterpret_cast<const std::int32_t *>(overwriteRaw + 0x000) != 2 ||
        overwrite.vertexCount != 4 || overwrite.materialRef != &entry ||
        overwrite.alphaOrShadeF != 77.0f || overwrite.scanConvertMode != 3 ||
        overwrite.savedInvDepthBias != 4.0f || overwrite.savedInvDepthScale != 5.0f ||
        overwritePerVertex[3] != 32.0f ||
        *reinterpret_cast<const std::int32_t *>(overwriteRaw + 0x47c) != 1 ||
        *reinterpret_cast<const std::int32_t *>(overwriteRaw + 0x488) != 0x40 ||
        !SameQueuedVec3(overwrite.polyVerts[2], projected[2]) ||
        !SameQueuedVec3(overwrite.triVerts[2], triData[2]) ||
        !SameQueuedVec3(overwriteClipped[0], clipped[0])) {
        return 1;
    }

    zRndr::g_transparentQueueCount = 0;
    zRndr::g_transparentQueue[0] = {};
    zRndr::g_scanConvertMode = 4;
    zRndr::g_inverseDepthBias = 6.0f;
    zRndr::g_inverseDepthScale = 7.0f;
    g_zRndr_ActivePaletteRemapKey = 0x55;

    zRndr_SubmitTexturedPolyPerVertexAlphaOrShade(projected, nullptr, triData, triUVs, perVertex,
                                                  88, 3, &entry, 1, 0);
    const zRndr::TransparentQueuedPolyDrawCmd &transparent = zRndr::g_transparentQueue[0];
    if (zRndr::g_transparentQueueCount != 1 || transparent.materialRef != &entry ||
        transparent.vertexCount != 3 || transparent.alphaOrShadeBits != 0xff ||
        transparent.scanConvertMode != 4 || transparent.savedInvDepthBias != 6.0f ||
        transparent.savedInvDepthScale != 7.0f || transparent.texKey != 0x55 ||
        !SameQueuedVec3(transparent.polyVerts[1], projected[1]) ||
        !SameQueuedVec3(transparent.triVerts[1], triData[1]) ||
        transparent.triUVs[2] != triUVs[1].x || transparent.triUVs[3] != triUVs[1].y) {
        return 2;
    }

    return 0;
}

extern "C" int zrndr_flush_transparent_queue_smoke(void) {
    zRndr::g_transparentQueueCount = 2;
    zRndr::g_transparentQueueSortIndices[0] = -1;
    zRndr::g_transparentQueueSortIndices[1] = -1;
    zRndr::g_transparentQueue[0] = {};
    zRndr::g_transparentQueue[1] = {};
    zRndr::g_transparentQueue[0].triVerts[0].z = 4.0f;
    zRndr::g_transparentQueue[1].triVerts[0].z = 2.0f;
    zRndr::g_transparentQueue[0].savedInvDepthBias = 11.0f;
    zRndr::g_transparentQueue[0].savedInvDepthScale = 12.0f;
    zRndr::g_transparentQueue[0].scanConvertMode = 13;
    zRndr::g_transparentQueue[1].savedInvDepthBias = 21.0f;
    zRndr::g_transparentQueue[1].savedInvDepthScale = 22.0f;
    zRndr::g_transparentQueue[1].scanConvertMode = 23;

    zRndr_FlushTransparentQueue();
    if (zRndr::g_transparentQueueCount != 0 || zRndr::g_transparentQueueSortIndices[0] != 1 ||
        zRndr::g_transparentQueueSortIndices[1] != 0) {
        return 1;
    }

    return zRndr::g_inverseDepthBias == 11.0f && zRndr::g_inverseDepthScale == 12.0f &&
                   zRndr::g_scanConvertMode == 13
               ? 0
               : 2;
}

extern "C" int zrndr_flush_overwrite_queue_smoke(void) {
    zRndr::g_overwriteQueueCount = 1;
    zRndr::g_overwriteQueue[0] = {};
    std::uint8_t *raw = reinterpret_cast<std::uint8_t *>(&zRndr::g_overwriteQueue[0]);
    *reinterpret_cast<std::int32_t *>(raw + 0x000) = 0;
    zRndr::g_overwriteQueue[0].vertexCount = 0;
    zRndr::g_overwriteQueue[0].alphaOrShadeF = 128.0f;
    zRndr::g_overwriteQueue[0].shadeOrSpanMode = 3;
    zRndr::g_overwriteQueue[0].savedInvDepthBias = 31.0f;
    zRndr::g_overwriteQueue[0].savedInvDepthScale = 32.0f;
    zRndr::g_overwriteQueue[0].scanConvertMode = 33;
    zRndr::g_inverseDepthBias = 1.0f;
    zRndr::g_inverseDepthScale = 2.0f;
    zRndr::g_scanConvertMode = 3;
    zRndr::g_pfnBuildSpanList = nullptr;
    zRndr::g_pfnBuildSpanListSecondary = nullptr;

    zRndr_FlushOverwriteQueue();
    if (zRndr::g_overwriteQueueCount != 0 ||
        zRndr::g_pfnBuildSpanList != zRndr_SpanOcclusion_InsertSpanNode_Local ||
        zRndr::g_pfnBuildSpanListSecondary != zRndr_SpanOcclusion_BuildSpanList) {
        return 1;
    }

    return zRndr::g_inverseDepthBias == 31.0f && zRndr::g_inverseDepthScale == 32.0f &&
                   zRndr::g_scanConvertMode == 33
               ? 0
               : 2;
}

extern "C" int zrndr_texture_mip_select_variant_smoke(void) {
    zVidImagePartial baseImage{};
    zVidImagePartial variantImage{};
    zImage_TexDirEntryPartial baseEntry{};
    zImage_TexDirEntryPartial variantEntry{};
    baseEntry.image = &baseImage;
    baseEntry.nextVariant = &variantEntry;
    variantEntry.image = &variantImage;

    zVec3 triVerts[3] = {
        {0.0f, 0.0f, 2.0f},
        {1.0f, 0.0f, 4.0f},
        {0.0f, 1.0f, 3.0f},
    };
    zVec2 triUVs[3] = {
        {0.0f, 0.0f},
        {2.0f, 2.0f},
        {1.0f, 1.0f},
    };
    zVec2 mipParamsA{0.0f, 0.0f};
    zVec2 mipParamsB{8.0f, 0.0f};
    zVec2 mipParamsC{0.0f, 0.0f};

    zRndr::g_textureMipSelectionEnabled = 0;
    if (zRndr_TextureMip_SelectVariantImage(&baseEntry, triVerts, 3, triUVs, &mipParamsA,
                                            &mipParamsB, &mipParamsC) != &baseImage ||
        zRndr_TextureMip_SelectVariantImage(nullptr, triVerts, 3, triUVs, &mipParamsA, &mipParamsB,
                                            &mipParamsC) != nullptr) {
        return 1;
    }

    zRndr::g_textureMipSelectionEnabled = 1;
    if (zRndr_TextureMip_SelectVariantImage(&baseEntry, triVerts, 3, triUVs, &mipParamsA,
                                            &mipParamsB, &mipParamsC) != &variantImage) {
        return 2;
    }

    return 0;
}

extern "C" int zrndr_draw_textured_queued_alpha_smoke(void) {
    std::uint16_t frame[16 * 16] = {};
    zRndr::SpanNodePartial spanPool[16] = {};
    std::uint8_t texels[16] = {};
    std::uint16_t palette[0x400] = {};

    zVidImagePartial image{};
    image.width = 4;
    image.height = 4;
    image.pixels = texels;
    image.palette = palette;
    image.widthScale = 4.0f;
    image.uShiftFrom20 = 20;
    image.uMask = 3;
    image.vMaskFixed20 = 3 << 20;

    zImage_TexDirEntryPartial entry{};
    entry.image = &image;

    zVec3 projectedVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec3 triVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec2 triUVs[3] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };

    g_rasterSpanBuildCount = 0;
    g_rasterLastDst = nullptr;
    g_texturedQueuedSpanCount = 0;
    g_texturedQueuedLastTexU = -1;
    g_texturedQueuedLastTexV = -1;
    g_texturedQueuedLastPixelCount = 0;
    g_texturedQueuedLastVShift = 0;

    zRndr::g_frameBuffer = frame;
    zRndr::g_pitchBytes = 16 * static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_bytesPerPixel = static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_spanAllocCursor = spanPool;
    zRndr::g_pfnBuildSpanList = TestRasterBuildSpanList;
    zRndr::g_pfnTexturedQueuedSpanOp_Mode0 = nullptr;
    zRndr::g_pfnTexturedQueuedSpanOp_Mode1 = TestTexturedQueuedSpanOp;
    zRndr::g_inverseDepthBias = 0.0f;
    zRndr::g_inverseDepthScale = 1.0f;
    zRndr::g_perspectiveAdaptiveMinSpan = 0;
    zRndr::g_perspectiveTextureDeltaXPow2 = 4;
    zRndr::g_spanActiveTexels = nullptr;
    zRndr::g_spanActiveTexPalette = nullptr;
    zRndr::g_spanActiveTexUMask = 0;
    zRndr::g_spanActiveTexVMask = 0;

    zRndr_DrawTexturedQueuedAlpha(&entry, projectedVerts, nullptr, triVerts, triUVs, 3, 2);

    if (g_rasterSpanBuildCount <= 0 || g_texturedQueuedSpanCount <= 0 ||
        g_texturedQueuedLastPixelCount <= 0 || g_texturedQueuedLastVShift != 20) {
        return 1;
    }

    if (zRndr::g_spanActiveTexels != texels || zRndr::g_spanActiveTexPalette != &palette[0x300] ||
        zRndr::g_spanActiveTexUMask != 3 || zRndr::g_spanActiveTexVMask != (3 << 20)) {
        return 2;
    }

    const std::uint8_t *frameBase = reinterpret_cast<const std::uint8_t *>(frame);
    const std::uint8_t *frameEnd = frameBase + sizeof(frame);
    const std::uint8_t *dst = reinterpret_cast<const std::uint8_t *>(g_rasterLastDst);
    if (dst < frameBase || dst >= frameEnd) {
        return 3;
    }

    return 0;
}

extern "C" int zrndr_draw_textured_fan_tri_smoke(void) {
    std::uint16_t frame[16 * 16] = {};
    zRndr::SpanNodePartial spanPool[16] = {};
    std::uint8_t texels[16] = {};
    char alphaMap[16] = {};
    std::uint16_t palette[0x400] = {};

    zVidImagePartial image{};
    image.width = 4;
    image.height = 4;
    image.pixels = texels;
    image.palette = palette;
    image.widthScale = 4.0f;
    image.queuedAlphaMap = alphaMap;
    image.uShiftFrom20 = 20;
    image.uMask = 3;
    image.vMaskFixed20 = 3 << 20;

    zImage_TexDirEntryPartial entry{};
    entry.image = &image;

    zVec3 projectedVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec3 triVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec2 triUVs[3] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };

    g_rasterSpanBuildCount = 0;
    g_rasterLastDst = nullptr;
    g_texturedQueuedSpanCount = 0;
    g_texturedQueuedLastPixelCount = 0;
    g_texturedQueuedLastVShift = 0;

    zRndr::g_frameBuffer = frame;
    zRndr::g_pitchBytes = 16 * static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_bytesPerPixel = static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_spanAllocCursor = spanPool;
    zRndr::g_pfnBuildSpanList = nullptr;
    zRndr::g_pfnBuildSpanListSecondary = TestRasterBuildSpanList;
    zRndr::g_pfnTexturedFanTriSpanOp_Mode0 = nullptr;
    zRndr::g_pfnTexturedFanTriSpanOp_Mode1 = TestTexturedQueuedSpanOp;
    zRndr::g_inverseDepthBias = 0.0f;
    zRndr::g_inverseDepthScale = 1.0f;
    zRndr::g_perspectiveAdaptiveMinSpan = 0;
    zRndr::g_perspectiveTextureDeltaXPow2 = 4;
    zRndr::g_spanActiveTexels = nullptr;
    zRndr::g_spanActiveTexAlphaMap = nullptr;
    zRndr::g_spanActiveTexPalette = nullptr;
    zRndr::g_spanActiveConstAlphaBits = 0;

    zRndr_DrawTexturedFanTri(&entry, projectedVerts, nullptr, triVerts, triUVs, 3, 0x80, 1);

    if (g_rasterSpanBuildCount <= 0 || g_texturedQueuedSpanCount <= 0 ||
        g_texturedQueuedLastPixelCount <= 0 || g_texturedQueuedLastVShift != 20) {
        return 1;
    }

    if (zRndr::g_spanActiveTexels != texels || zRndr::g_spanActiveTexAlphaMap != alphaMap ||
        zRndr::g_spanActiveTexPalette != &palette[0x200] ||
        zRndr::g_spanActiveConstAlphaBits != 0x80) {
        return 2;
    }

    const std::uint8_t *frameBase = reinterpret_cast<const std::uint8_t *>(frame);
    const std::uint8_t *frameEnd = frameBase + sizeof(frame);
    const std::uint8_t *dst = reinterpret_cast<const std::uint8_t *>(g_rasterLastDst);
    return dst >= frameBase && dst < frameEnd ? 0 : 3;
}

extern "C" int zrndr_draw_flat_queued_smoke(void) {
    std::uint16_t frame[16 * 16] = {};
    zRndr::SpanNodePartial spanPool[16] = {};
    std::uint8_t texels[16] = {};
    char alphaMap[16] = {};
    std::uint16_t palette[0x400] = {};

    zVidImagePartial image{};
    image.width = 4;
    image.height = 4;
    image.pixels = texels;
    image.alphaMap = alphaMap;
    image.palette = palette;
    image.widthScale = 4.0f;
    image.uShiftFrom20 = 20;
    image.uMask = 3;
    image.vMaskFixed20 = 3 << 20;

    zImage_TexDirEntryPartial entry{};
    entry.image = &image;

    zVec3 polyVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec3 triVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec2 triUVs[3] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };

    g_rasterSpanBuildCount = 0;
    g_rasterLastDst = nullptr;
    g_texturedQueuedSpanCount = 0;
    g_texturedQueuedLastPixelCount = 0;
    g_texturedQueuedLastVShift = 0;

    zRndr::g_frameBuffer = frame;
    zRndr::g_pitchBytes = 16 * static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_bytesPerPixel = static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_spanAllocCursor = spanPool;
    zRndr::g_pfnBuildSpanList = nullptr;
    zRndr::g_pfnBuildSpanListSecondary = TestRasterBuildSpanList;
    zRndr::g_pfnFlatQueuedSpanOp_Mode0 = TestTexturedQueuedSpanOp;
    zRndr::g_pfnFlatQueuedSpanOpAlt_Mode0 = TestTexturedQueuedSpanOp;
    zRndr::g_pfnFlatQueuedSpanOp_Mode1 = nullptr;
    zRndr::g_pfnFlatQueuedSpanOpAlt_Mode1 = nullptr;
    zRndr::g_inverseDepthBias = 0.0f;
    zRndr::g_inverseDepthScale = 1.0f;
    zRndr::g_perspectiveAdaptiveMinSpan = 0;
    zRndr::g_perspectiveTextureDeltaXPow2 = 4;
    zRndr::g_spanActiveTexels = nullptr;
    zRndr::g_spanActiveTexAlphaMap = nullptr;
    zRndr::g_spanActiveTexPalette = nullptr;

    zRndr_DrawFlatQueued(&entry, polyVerts, triVerts, triUVs, 3, 1);

    if (g_rasterSpanBuildCount <= 0 || g_texturedQueuedSpanCount <= 0 ||
        g_texturedQueuedLastPixelCount <= 0 || g_texturedQueuedLastVShift != 20) {
        return 1;
    }

    if (zRndr::g_spanActiveTexels != texels || zRndr::g_spanActiveTexAlphaMap != alphaMap ||
        zRndr::g_spanActiveTexPalette != &palette[0x200] ||
        zRndr::g_pfnSelectedSpanOp_Mode0 != zRndr::g_pfnFlatQueuedSpanOp_Mode0) {
        return 2;
    }

    const std::uint8_t *frameBase = reinterpret_cast<const std::uint8_t *>(frame);
    const std::uint8_t *frameEnd = frameBase + sizeof(frame);
    const std::uint8_t *dst = reinterpret_cast<const std::uint8_t *>(g_rasterLastDst);
    return dst >= frameBase && dst < frameEnd ? 0 : 3;
}

extern "C" int zrndr_renderer_draw_poly_tlv_smoke(void) {
    std::uint16_t frame[16 * 16] = {};
    zRndr::SpanNodePartial spanPool[16] = {};
    std::uint8_t texels[16] = {};
    char alphaMap[16] = {};
    std::uint16_t palette[0x400] = {};

    zVidImagePartial image{};
    image.width = 4;
    image.height = 4;
    image.pixels = texels;
    image.alphaMap = alphaMap;
    image.palette = palette;
    image.widthScale = 4.0f;
    image.uShiftFrom20 = 20;
    image.uMask = 3;
    image.vMaskFixed20 = 3 << 20;

    zImage_TexDirEntryPartial entry{};
    entry.image = &image;

    zVec3 polyVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec3 triVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec2 triUVs[3] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };

    g_rasterSpanBuildCount = 0;
    g_rasterLastDst = nullptr;
    g_texturedQueuedSpanCount = 0;
    g_texturedQueuedLastPixelCount = 0;
    g_texturedQueuedLastVShift = 0;

    zRndr::g_frameBuffer = frame;
    zRndr::g_pitchBytes = 16 * static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_bytesPerPixel = static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_spanAllocCursor = spanPool;
    zRndr::g_pfnBuildSpanList = nullptr;
    zRndr::g_pfnBuildSpanListSecondary = TestRasterBuildSpanList;
    zRndr::g_pfnPolyTlvSpanOp_Mode0 = nullptr;
    zRndr::g_pfnPolyTlvSpanOpAlt_Mode0 = TestTexturedQueuedSpanOp;
    zRndr::g_pfnPolyTlvSpanOp_Mode1 = nullptr;
    zRndr::g_pfnPolyTlvSpanOpAlt_Mode1 = nullptr;
    zRndr::g_inverseDepthBias = 0.0f;
    zRndr::g_inverseDepthScale = 1.0f;
    zRndr::g_spanActiveTexels = nullptr;
    zRndr::g_spanActiveTexAlphaMap = nullptr;
    zRndr::g_spanActiveTexPalette = nullptr;
    zRndr::g_spanActiveConstAlphaBits = 0;

    Renderer_DrawPolyTLV(&entry, polyVerts, triVerts, triUVs, 3, 0.5f, 1);

    if (g_rasterSpanBuildCount <= 0 || g_texturedQueuedSpanCount <= 0 ||
        g_texturedQueuedLastPixelCount <= 0 || g_texturedQueuedLastVShift != 20) {
        return 1;
    }

    if (zRndr::g_spanActiveTexels != texels || zRndr::g_spanActiveTexAlphaMap != alphaMap ||
        zRndr::g_spanActiveTexPalette != &palette[0x200] ||
        zRndr::g_spanActiveConstAlphaBits != 0x3f000000) {
        return 2;
    }

    const std::uint8_t *frameBase = reinterpret_cast<const std::uint8_t *>(frame);
    const std::uint8_t *frameEnd = frameBase + sizeof(frame);
    const std::uint8_t *dst = reinterpret_cast<const std::uint8_t *>(g_rasterLastDst);
    return dst >= frameBase && dst < frameEnd ? 0 : 3;
}

extern "C" int zrndr_draw_textured_queued_smoke(void) {
    std::uint16_t frame[16 * 16] = {};
    zRndr::SpanNodePartial spanPool[16] = {};
    std::uint8_t texels[16] = {};
    std::uint16_t palette[0x400] = {};

    zVidImagePartial image{};
    image.width = 4;
    image.height = 4;
    image.pixels = texels;
    image.palette = palette;
    image.widthScale = 4.0f;
    image.uShiftFrom20 = 20;
    image.uMask = 3;
    image.vMaskFixed20 = 3 << 20;

    zImage_TexDirEntryPartial entry{};
    entry.image = &image;

    zVec3 projectedVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec3 triVerts[3] = {
        {1.0f, 1.0f, 1.0f},
        {5.0f, 1.0f, 1.0f},
        {1.0f, 5.0f, 1.0f},
    };
    zVec2 triUVs[3] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
    };
    zVec3 shadeTriplet{0.0f, 128.0f, 255.0f};

    g_rasterSpanBuildCount = 0;
    g_texturedQueuedSpanCount = 0;
    g_texturedQueuedLastPixelCount = 0;
    g_texturedQueuedLastVShift = 0;

    zRndr::g_frameBuffer = frame;
    zRndr::g_pitchBytes = 16 * static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_bytesPerPixel = static_cast<int>(sizeof(std::uint16_t));
    zRndr::g_spanAllocCursor = spanPool;
    zRndr::g_pfnBuildSpanList = TestRasterBuildSpanList;
    zRndr::g_pfnTexturedQueuedSpanOp_Mode0 = nullptr;
    zRndr::g_pfnTexturedQueuedSpanOp_Mode1 = TestTexturedQueuedSpanOp;
    zRndr::g_inverseDepthBias = 0.0f;
    zRndr::g_inverseDepthScale = 1.0f;
    zRndr::g_perspectiveAdaptiveMinSpan = 0;
    zRndr::g_perspectiveTextureDeltaXPow2 = 4;
    zRndr::g_spanActiveTexels = nullptr;
    zRndr::g_spanActiveTexPalette = nullptr;

    zRndr_DrawTexturedQueued(&entry, projectedVerts, nullptr, triVerts, triUVs, &shadeTriplet, 3, 0,
                             1);

    if (g_rasterSpanBuildCount <= 0 || g_texturedQueuedSpanCount <= 0 ||
        g_texturedQueuedLastPixelCount <= 0 || g_texturedQueuedLastVShift != 20) {
        return 1;
    }

    return zRndr::g_spanActiveTexels == texels && zRndr::g_spanActiveTexPalette == &palette[0x200]
               ? 0
               : 2;
}

extern "C" int zrndr_span_routine_selection_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 5, 5, 0x7c00, 0x03e0, 0x001f);
    zRndr::g_bytesPerPixel = 2;
    zRndr::g_defaultGraphicsFlags = 0x0e;
    zRndr::g_graphicsFlags = &zRndr::g_defaultGraphicsFlags;
    zRndr::g_pfnFlatQueuedSpanOp_Mode0 = nullptr;
    zRndr::g_pfnTexturedQueuedFinalizeAlt = nullptr;

    zRndr::SelectSpanRoutines();

    return zRndr::g_pixelPackRedBits == 5 && zRndr::g_pixelPackGreenBits == 5 &&
                   zRndr::g_pixelPackBlueBits == 5 && zRndr::g_pixelPackRedMask == 0x7c00 &&
                   zRndr::g_pixelPackGreenShift == 2 && zRndr::g_pixelPackBlueShift == 3 &&
                   (zRndr::g_defaultGraphicsFlags & 4) == 0 &&
                   zRndr::g_perspectiveAdaptiveMinSpan == 0x10 &&
                   zRndr::g_perspectiveAdaptiveMaxSpan == 0x40 &&
                   zRndr::g_perspectiveAdaptiveSlope == 0.100000001f &&
                   zRndr::g_pfnFlatQueuedSpanOp_Mode0 != nullptr &&
                   zRndr::g_pfnTexturedQueuedFinalizeAlt == nullptr
               ? 0
               : 1;
}

extern "C" int zrndr_perspective_adaptive_span_params_smoke(void) {
    zRndr::g_perspectiveAdaptiveMinSpan = 0;
    zRndr::g_perspectiveAdaptiveMaxSpan = 0;
    zRndr::g_perspectiveAdaptiveSlope = 0.0f;

    zRndr::SetPerspectiveAdaptiveSpanParams(12, 96, 0.25f);

    return zRndr::g_perspectiveAdaptiveMinSpan == 12 &&
                   zRndr::g_perspectiveAdaptiveMaxSpan == 96 &&
                   zRndr::g_perspectiveAdaptiveSlope == 0.25f
               ? 0
               : 1;
}

extern "C" int zrndr_overlay_and_mmx_masks_smoke(void) {
    zRndr::g_swOverlayDstScale5 = 3;
    zRndr::g_swOverlayPremulPacked = 0x00200100;
    zRndr::g_swOverlayPremulPackedRot16 = 0x00010002;

    std::uint32_t pair = 0x7c1f03e0;
    const std::uint32_t expected =
        (((((pair >> 5) & 0x03e0f81fU) * 3U) + 0x00200100U) & 0x7c1f03e0U) |
        (((((pair & 0x03e07c1fU) * 3U) >> 5) + 0x00010002U) & 0x03e07c1fU);
    zRndr::OverlayBlendRow555_Scalar(reinterpret_cast<std::uint16_t *>(&pair), 2);
    if (pair != expected) {
        return 1;
    }

    zRndr::SpanMmxSetPixelFormatMasks(5);
    if (zRndr::g_mmxMaskGreenBits[0] != 0x03e0 || zRndr::g_mmxMaskRedPacked[3] != 0xfc00 ||
        zRndr::g_mmxMaskGreenPacked[2] != 0xffe0 || zRndr::g_mmxMaskBlueBits[1] != 0x001f) {
        return 2;
    }

    zRndr::SpanMmxSetPixelFormatMasks(6);
    return zRndr::g_mmxMaskGreenBits[0] == 0x07e0 && zRndr::g_mmxMaskRedPacked[3] == 0xf800 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_565_const_alpha_pal8_smoke(void) {
    std::uint8_t texels[0x100] = {};
    std::uint16_t palette[0x10000] = {};
    std::uint16_t dst[4] = {0x0000, 0x001f, 0x0000, 0x0000};
    texels[0] = 0;
    texels[1] = 7;
    texels[2] = 9;
    palette[7] = 0xf800;
    palette[9] = 0x07e0;
    palette[0x001f] = 0xffff;

    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanActiveConstAlphaBits = 0xff;

    zRndr::SpanAlphaBlend565ConstAlphaFromPal8(0, 0, 2, 0);
    if (dst[0] != 0x0000 || dst[1] != 0xf800 || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[1] = 0x001f;
    zRndr::g_spanCurrentDst16 = &dst[1];
    zRndr::g_spanActiveConstAlphaBits = 0x80;
    zRndr::SpanAlphaBlend565ConstAlphaFromPal8(1 << 20, 0, 1, 0);
    if (dst[1] != 0x7bff) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::g_spanActiveConstAlphaBits = 3;
    zRndr::SpanAlphaBlend565ConstAlphaFromPal8(2 << 20, 0, 1, 0);
    if (dst[2] != 0x0000 || zRndr::g_spanCurrentDst16 != &dst[2]) {
        return 3;
    }

    zRndr::g_spanCurrentDst16 = &dst[3];
    zRndr::g_spanActiveConstAlphaBits = 0xff;
    zRndr::SpanMasked16FromPal8To565(2 << 20, 0, 1, 0);
    return dst[3] == 0x07e0 && zRndr::g_spanCurrentDst16 == &dst[3] ? 0 : 4;
}

extern "C" int zrndr_fill_span16_opaque_smoke(void) {
    std::uint16_t dst[7] = {0xaaaa, 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0xbbbb};

    zRndr::g_spanCurrentDst16 = &dst[1];
    zRndr_FillSpan16Opaque(0x12345, 5);
    if (dst[0] != 0xaaaa || dst[6] != 0xbbbb || zRndr::g_spanCurrentDst16 != &dst[1]) {
        return 1;
    }

    for (int index = 1; index <= 5; ++index) {
        if (dst[index] != 0x2345) {
            return 1 + index;
        }
    }

    return 0;
}

extern "C" int zrndr_fill_span555_solid_smoke(void) {
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanCurrentDst16 = dst;
    zRndr_FillSpan555Solid(0x7fff, 7, 1);
    if (dst[0] != 0x001f || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    zRndr::g_spanCurrentDst16 = dst;
    zRndr_FillSpan555Solid(0x7fff, 0x80, 1);
    if (dst[0] != 0x3dff) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[1];
    zRndr_FillSpan555Solid(0x1234, 0xfc, 2);
    return dst[1] == 0x1234 && dst[2] == 0x1234 && dst[3] == 0x4444 ? 0 : 3;
}

extern "C" int zrndr_fill_span565_solid_smoke(void) {
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanCurrentDst16 = dst;
    zRndr_FillSpan565Solid(0xffff, 3, 1);
    if (dst[0] != 0x001f || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    zRndr::g_spanCurrentDst16 = dst;
    zRndr_FillSpan565Solid(0xffff, 0x80, 1);
    if (dst[0] != 0x7bff) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[1];
    zRndr_FillSpan565Solid(0x5678, 0xfc, 2);
    return dst[1] == 0x5678 && dst[2] == 0x5678 && dst[3] == 0x4444 ? 0 : 3;
}

extern "C" int zrndr_span_masked_tex16_to_565_smoke(void) {
    std::uint16_t texels[4] = {0x0000, 0xf800, 0x07e0, 0x001f};
    std::uint16_t dst[4] = {0x1111, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanActiveConstAlphaBits = 0xff;

    zRndr::SpanMasked16FromTex16To565(0, 0, 3, 0);
    if (dst[0] != 0x1111 || dst[1] != 0xf800 || dst[2] != 0x07e0 ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::g_spanActiveConstAlphaBits = 0x80;
    zRndr::SpanMasked16FromTex16To565(3 << 20, 0, 1, 0);
    if (dst[2] != 0x07e0) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[3];
    zRndr::g_spanActiveConstAlphaBits = 3;
    zRndr::SpanMasked16FromTex16To565(3 << 20, 0, 1, 0);
    return dst[3] == 0x4444 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_565_const_alpha_tex16_smoke(void) {
    std::uint16_t texels[4] = {0x0000, 0xf800, 0xffff, 0x001f};
    std::uint16_t dst[4] = {0x1111, 0x2222, 0x001f, 0x4444};

    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanActiveConstAlphaBits = 0xff;

    zRndr::SpanAlphaBlend565ConstAlphaFromTex16(0, 0, 2, 0);
    if (dst[0] != 0x0000 || dst[1] != 0xf800 || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::g_spanActiveConstAlphaBits = 0x80;
    zRndr::SpanAlphaBlend565ConstAlphaFromTex16(2 << 20, 0, 1, 0);
    if (dst[2] != 0x7bff) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[3];
    zRndr::g_spanActiveConstAlphaBits = 3;
    zRndr::SpanAlphaBlend565ConstAlphaFromTex16(3 << 20, 0, 1, 0);
    return dst[3] == 0x4444 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_565_from_tex16_alpha8_smoke(void) {
    std::uint16_t texels[4] = {0xffff, 0xf800, 0x07e0, 0x001f};
    std::uint8_t alphaMap[4] = {0x80, 0xf8, 0x10, 0};
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    zRndr::SpanAlphaBlend565FromTex16Alpha8(0, 0, 3, 0);
    if (dst[0] != ExpectedBlend565Alpha8(0x001f, 0xffff, 0x80) || dst[1] != 0xf800 ||
        dst[2] != 0xf800 || dst[3] != 0x4444 || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x1111;
    dst[1] = 0x2222;
    alphaMap[0] = 7;
    zRndr::SpanAlphaBlend565FromTex16Alpha8(0, 0, 2, 0);
    if (dst[0] != 0x1111 || dst[1] != 0x2222) {
        return 2;
    }

    dst[0] = 0x001f;
    dst[1] = 0x0000;
    alphaMap[0] = 0x80;
    zRndr::SpanAlphaBlend565FromTex16Alpha8(0, 0, 2, 0);

    std::uint32_t packed = 0;
    std::memcpy(&packed, dst, sizeof(packed));
    const std::uint32_t expectedPair = ExpectedBlend565PairAlpha5(0x0000001fu, 0xffff, 0x80);
    return packed == expectedPair ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_565_const_alpha_from_tex16_alpha8_smoke(void) {
    std::uint16_t texels[4] = {0xffff, 0xf800, 0x07e0, 0x001f};
    std::uint8_t alphaMap[4] = {0x80, 0xff, 0x02, 0x40};
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    float alphaScale = 0.5f;
    std::memcpy(&zRndr::g_spanActiveConstAlphaBits, &alphaScale, sizeof(alphaScale));
    zRndr::SpanAlphaBlend565ConstAlphaFromTex16Alpha8(0, 0, 1, 0);
    if (dst[0] != ExpectedBlend565Alpha8(0x001f, 0xffff, 0x40) ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    alphaScale = 1.0f;
    std::memcpy(&zRndr::g_spanActiveConstAlphaBits, &alphaScale, sizeof(alphaScale));
    zRndr::g_spanCurrentDst16 = &dst[1];
    zRndr::SpanAlphaBlend565ConstAlphaFromTex16Alpha8(1 << 20, 0, 1, 0);
    if (dst[1] != 0xf800) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::SpanAlphaBlend565ConstAlphaFromTex16Alpha8(2 << 20, 0, 1, 0);
    return dst[2] == 0x3333 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_565_mmx_from_tex16_alpha8_smoke(void) {
    std::uint16_t texels[5] = {0xffff, 0xf800, 0x07e0, 0x001f, 0x1234};
    std::uint8_t alphaMap[5] = {0x80, 0xff, 0x00, 0x10, 0xfc};
    std::uint16_t dst[5] = {0x001f, 0x2222, 0x3333, 0x4444, 0x5555};

    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    zRndr::SpanAlphaBlend565MmxFromTex16Alpha8(0, 0, 5, 0);
    if (dst[0] != ExpectedBlend565Alpha8(0x001f, 0xffff, 0x80) ||
        dst[1] != ExpectedBlend565Alpha8(0x2222, 0xf800, 0xff) ||
        dst[2] != ExpectedBlend565Alpha8(0x3333, 0x07e0, 0x00) ||
        dst[3] != ExpectedBlend565Alpha8(0x4444, 0x001f, 0x10) || dst[4] != 0x1234 ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x7777;
    alphaMap[0] = 3;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::SpanAlphaBlend565MmxFromTex16Alpha8(0, 0, 1, 0);
    return dst[0] == 0x7777 ? 0 : 2;
}

extern "C" int zrndr_span_alpha_blend_555_const_alpha_from_tex16_alpha8_smoke(void) {
    std::uint16_t texels[4] = {0x7fff, 0x7c00, 0x03e0, 0x001f};
    std::uint8_t alphaMap[4] = {0x80, 0xff, 0x07, 0x40};
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    float alphaScale = 0.5f;
    std::memcpy(&zRndr::g_spanActiveConstAlphaBits, &alphaScale, sizeof(alphaScale));
    zRndr::SpanAlphaBlend555ConstAlphaFromTex16Alpha8(0, 0, 1, 0);
    if (dst[0] != ExpectedBlend555ConstAlphaMap(0x001f, 0x7fff, 0x40) ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    alphaScale = 1.0f;
    std::memcpy(&zRndr::g_spanActiveConstAlphaBits, &alphaScale, sizeof(alphaScale));
    zRndr::g_spanCurrentDst16 = &dst[1];
    zRndr::SpanAlphaBlend555ConstAlphaFromTex16Alpha8(1 << 20, 0, 1, 0);
    if (dst[1] != 0x7c00) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::SpanAlphaBlend555ConstAlphaFromTex16Alpha8(2 << 20, 0, 1, 0);
    return dst[2] == 0x3333 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_555_mmx_from_tex16_alpha8_smoke(void) {
    std::uint16_t texels[5] = {0x7fff, 0x7c00, 0x03e0, 0x001f, 0x1234};
    std::uint8_t alphaMap[5] = {0x80, 0xff, 0x00, 0x10, 0xfc};
    std::uint16_t dst[5] = {0x001f, 0x2222, 0x3333, 0x4444, 0x5555};

    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    zRndr::SpanAlphaBlend555MmxFromTex16Alpha8(0, 0, 5, 0);
    if (dst[0] != ExpectedBlend555Alpha8(0x001f, 0x7fff, 0x80) ||
        dst[1] != ExpectedBlend555Alpha8(0x2222, 0x7c00, 0xff) ||
        dst[2] != ExpectedBlend555Alpha8(0x3333, 0x03e0, 0x00) ||
        dst[3] != ExpectedBlend555Alpha8(0x4444, 0x001f, 0x10) || dst[4] != 0x1234 ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x7777;
    alphaMap[0] = 7;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::SpanAlphaBlend555MmxFromTex16Alpha8(0, 0, 1, 0);
    return dst[0] == 0x7777 ? 0 : 2;
}

extern "C" int zrndr_span_alpha_blend_565_from_pal8_alpha8_smoke(void) {
    std::uint8_t texels[4] = {1, 2, 3, 4};
    std::uint8_t alphaMap[4] = {0x80, 0xf8, 0x80, 0x07};
    std::uint16_t palette[5] = {0x0000, 0xffff, 0xf800, 0x07e0, 0x001f};
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    zRndr::SpanAlphaBlend565FromPal8Alpha8(0, 0, 3, 0);
    if (dst[0] != ExpectedBlend565Alpha8(0x001f, 0xffff, 0x80) || dst[1] != 0xf800 ||
        dst[2] != 0xf800 || dst[3] != 0x4444 || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x0000;
    dst[1] = 0xffff;
    alphaMap[0] = 0x80;
    zRndr::SpanAlphaBlend565FromPal8Alpha8(0, 0, 2, 0);

    std::uint32_t packed = 0;
    std::memcpy(&packed, dst, sizeof(packed));
    const std::uint32_t expectedPair = ExpectedBlend565PairAlpha5(0xffff0000u, 0xffff, 0x80);
    if (packed != expectedPair) {
        return 2;
    }

    dst[0] = 0x7777;
    dst[1] = 0x8888;
    alphaMap[0] = 7;
    zRndr::SpanAlphaBlend565FromPal8Alpha8(0, 0, 2, 0);
    return dst[0] == 0x7777 && dst[1] == 0x8888 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_555_from_pal8_alpha8_smoke(void) {
    std::uint8_t texels[4] = {1, 2, 3, 4};
    std::uint8_t alphaMap[4] = {0x80, 0xf8, 0x80, 0x07};
    std::uint16_t palette[5] = {0x0000, 0x7fff, 0x7c00, 0x03e0, 0x001f};
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    zRndr::SpanAlphaBlend555FromPal8Alpha8(0, 0, 3, 0);
    if (dst[0] != ExpectedBlend555Alpha8(0x001f, 0x7fff, 0x80) || dst[1] != 0x7c00 ||
        dst[2] != 0x7c00 || dst[3] != 0x4444 || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x0000;
    dst[1] = 0x7fff;
    alphaMap[0] = 0x80;
    zRndr::SpanAlphaBlend555FromPal8Alpha8(0, 0, 2, 0);

    std::uint32_t packed = 0;
    std::memcpy(&packed, dst, sizeof(packed));
    const std::uint32_t expectedPair = ExpectedBlend555PairAlpha5(0x7fff0000u, 0x7fff, 0x80);
    if (packed != expectedPair) {
        return 2;
    }

    dst[0] = 0x7777;
    dst[1] = 0x8888;
    alphaMap[0] = 7;
    zRndr::SpanAlphaBlend555FromPal8Alpha8(0, 0, 2, 0);
    return dst[0] == 0x7777 && dst[1] == 0x8888 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_565_const_alpha_from_pal8_alpha8_smoke(void) {
    std::uint8_t texels[4] = {1, 2, 3, 4};
    std::uint8_t alphaMap[4] = {0x80, 0xff, 0x02, 0x40};
    std::uint16_t palette[5] = {0x0000, 0xffff, 0xf800, 0x07e0, 0x001f};
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    float alphaScale = 0.5f;
    std::memcpy(&zRndr::g_spanActiveConstAlphaBits, &alphaScale, sizeof(alphaScale));
    zRndr::SpanAlphaBlend565ConstAlphaFromPal8Alpha8(0, 0, 1, 0);
    if (dst[0] != ExpectedBlend565Alpha8(0x001f, 0xffff, 0x40) ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    alphaScale = 1.0f;
    std::memcpy(&zRndr::g_spanActiveConstAlphaBits, &alphaScale, sizeof(alphaScale));
    zRndr::g_spanCurrentDst16 = &dst[1];
    zRndr::SpanAlphaBlend565ConstAlphaFromPal8Alpha8(1 << 20, 0, 1, 0);
    if (dst[1] != 0xf800) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::SpanAlphaBlend565ConstAlphaFromPal8Alpha8(2 << 20, 0, 1, 0);
    return dst[2] == 0x3333 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_555_const_alpha_from_pal8_alpha8_smoke(void) {
    std::uint8_t texels[4] = {1, 2, 3, 4};
    std::uint8_t alphaMap[4] = {0x80, 0xff, 0x07, 0x40};
    std::uint16_t palette[5] = {0x0000, 0x7fff, 0x7c00, 0x03e0, 0x001f};
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    float alphaScale = 0.5f;
    std::memcpy(&zRndr::g_spanActiveConstAlphaBits, &alphaScale, sizeof(alphaScale));
    zRndr::SpanAlphaBlend555ConstAlphaFromPal8Alpha8(0, 0, 1, 0);
    if (dst[0] != ExpectedBlend555ConstAlphaMap(0x001f, 0x7fff, 0x40) ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    alphaScale = 1.0f;
    std::memcpy(&zRndr::g_spanActiveConstAlphaBits, &alphaScale, sizeof(alphaScale));
    zRndr::g_spanCurrentDst16 = &dst[1];
    zRndr::SpanAlphaBlend555ConstAlphaFromPal8Alpha8(1 << 20, 0, 1, 0);
    if (dst[1] != 0x7c00) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::SpanAlphaBlend555ConstAlphaFromPal8Alpha8(2 << 20, 0, 1, 0);
    return dst[2] == 0x3333 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_565_mmx_from_pal8_alpha8_smoke(void) {
    std::uint8_t texels[5] = {1, 2, 3, 4, 5};
    std::uint8_t alphaMap[5] = {0x80, 0xff, 0x00, 0x10, 0xfc};
    std::uint16_t palette[6] = {0x0000, 0xffff, 0xf800, 0x07e0, 0x001f, 0x1234};
    std::uint16_t dst[5] = {0x001f, 0x2222, 0x3333, 0x4444, 0x5555};

    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    zRndr::SpanAlphaBlend565MmxFromPal8Alpha8(0, 0, 5, 0);
    if (dst[0] != ExpectedBlend565Alpha8(0x001f, 0xffff, 0x80) ||
        dst[1] != ExpectedBlend565Alpha8(0x2222, 0xf800, 0xff) ||
        dst[2] != ExpectedBlend565Alpha8(0x3333, 0x07e0, 0x00) ||
        dst[3] != ExpectedBlend565Alpha8(0x4444, 0x001f, 0x10) || dst[4] != 0x1234 ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x7777;
    alphaMap[0] = 3;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::SpanAlphaBlend565MmxFromPal8Alpha8(0, 0, 1, 0);
    return dst[0] == 0x7777 ? 0 : 2;
}

extern "C" int zrndr_span_alpha_blend_555_mmx_from_pal8_alpha8_smoke(void) {
    std::uint8_t texels[5] = {1, 2, 3, 4, 5};
    std::uint8_t alphaMap[5] = {0x80, 0xff, 0x00, 0x10, 0xfc};
    std::uint16_t palette[6] = {0x0000, 0x7fff, 0x7c00, 0x03e0, 0x001f, 0x1234};
    std::uint16_t dst[5] = {0x001f, 0x2222, 0x3333, 0x4444, 0x5555};

    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    zRndr::SpanAlphaBlend555MmxFromPal8Alpha8(0, 0, 5, 0);
    if (dst[0] != ExpectedBlend555Alpha8(0x001f, 0x7fff, 0x80) ||
        dst[1] != ExpectedBlend555Alpha8(0x2222, 0x7c00, 0xff) ||
        dst[2] != ExpectedBlend555Alpha8(0x3333, 0x03e0, 0x00) ||
        dst[3] != ExpectedBlend555Alpha8(0x4444, 0x001f, 0x10) || dst[4] != 0x1234 ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x7777;
    alphaMap[0] = 7;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::SpanAlphaBlend555MmxFromPal8Alpha8(0, 0, 1, 0);
    return dst[0] == 0x7777 ? 0 : 2;
}

extern "C" int zrndr_span_alpha_blend_555_from_tex16_alpha8_smoke(void) {
    std::uint16_t texels[4] = {0x7fff, 0x7c00, 0x03e0, 0x001f};
    std::uint8_t alphaMap[4] = {0x80, 0xf8, 0x10, 0};
    std::uint16_t dst[4] = {0x001f, 0x2222, 0x3333, 0x4444};

    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanActiveTexAlphaMap = reinterpret_cast<char *>(alphaMap);
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;

    zRndr::SpanAlphaBlend555FromTex16Alpha8(0, 0, 3, 0);
    if (dst[0] != ExpectedBlend555Alpha8(0x001f, 0x7fff, 0x80) || dst[1] != 0x7c00 ||
        dst[2] != 0x7c00 || dst[3] != 0x4444 || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x1111;
    dst[1] = 0x2222;
    alphaMap[0] = 7;
    zRndr::SpanAlphaBlend555FromTex16Alpha8(0, 0, 2, 0);
    if (dst[0] != 0x1111 || dst[1] != 0x2222) {
        return 2;
    }

    dst[0] = 0x001f;
    dst[1] = 0x0000;
    alphaMap[0] = 0x80;
    zRndr::SpanAlphaBlend555FromTex16Alpha8(0, 0, 2, 0);

    std::uint32_t packed = 0;
    std::memcpy(&packed, dst, sizeof(packed));
    const std::uint32_t expectedPair = ExpectedBlend555PairAlpha5(0x0000001fu, 0x7fff, 0x80);
    return packed == expectedPair ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_555_const_alpha_tex16_smoke(void) {
    std::uint16_t texels[4] = {0x0000, 0x7c00, 0x7fff, 0x001f};
    std::uint16_t dst[4] = {0x1111, 0x2222, 0x001f, 0x4444};

    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanActiveConstAlphaBits = 0xff;

    zRndr::SpanAlphaBlend555ConstAlphaFromTex16(0, 0, 2, 0);
    if (dst[0] != 0x0000 || dst[1] != 0x7c00 || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::g_spanActiveConstAlphaBits = 0x80;
    zRndr::SpanAlphaBlend555ConstAlphaFromTex16(2 << 20, 0, 1, 0);
    if (dst[2] != 0x3dff) {
        return 2;
    }

    zRndr::g_spanCurrentDst16 = &dst[3];
    zRndr::g_spanActiveConstAlphaBits = 7;
    zRndr::SpanAlphaBlend555ConstAlphaFromTex16(3 << 20, 0, 1, 0);
    return dst[3] == 0x4444 ? 0 : 3;
}

extern "C" int zrndr_span_alpha_blend_565_const_alpha_fast_pal8_smoke(void) {
    std::uint8_t texels[3] = {0, 1, 2};
    std::uint16_t palette[3] = {0x001f, 0xf800, 0xffff};
    std::uint16_t dst[3] = {0x1111, 0x2222, 0x001f};

    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanActiveConstAlphaBits = 0xff;

    zRndr::SpanAlphaBlend565ConstAlphaFastFromPal8(0, 0, 2, 0);
    if (dst[0] != 0x001f || dst[1] != 0xf800 || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::g_spanActiveConstAlphaBits = 0x80;
    zRndr::SpanAlphaBlend565ConstAlphaFastFromPal8(2 << 20, 0, 1, 0);
    return dst[2] == 0x7bff ? 0 : 2;
}

extern "C" int zrndr_span_alpha_blend_555_const_alpha_fast_pal8_smoke(void) {
    std::uint8_t texels[3] = {0, 1, 2};
    std::uint16_t palette[3] = {0x001f, 0x7c00, 0x7fff};
    std::uint16_t dst[3] = {0x1111, 0x2222, 0x001f};

    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x7f;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanActiveConstAlphaBits = 0xff;

    zRndr::SpanAlphaBlend555ConstAlphaFastFromPal8(0, 0, 2, 0);
    if (dst[0] != 0x001f || dst[1] != 0x7c00 || zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    zRndr::g_spanCurrentDst16 = &dst[2];
    zRndr::g_spanActiveConstAlphaBits = 0x80;
    zRndr::SpanAlphaBlend555ConstAlphaFastFromPal8(2 << 20, 0, 1, 0);
    return dst[2] == 0x3dff ? 0 : 2;
}

extern "C" int zrndr_fog_blend_span_565_scalar_smoke(void) {
    zRndr::g_fogParamsActive = {};
    zRndr::g_fogParamsActive.packedColor16 = 0xaaaa;
    zRndr::g_fogParamsActive.packedColor16Dup = static_cast<std::int32_t>(0xaaaaaaaa);
    zRndr::g_fogParamsActive.packedColorRamp[31] = 0x00010002;
    zRndr::g_fogParamsActive.packedColorRamp[30] = 0x00030004;

    std::uint16_t pixels[5] = {0x001f, 0xf800, 0x07e0, 0x1111, 0x2222};
    const std::uint16_t expected0 = ExpectedFog565Pixel(pixels[0], 0x80000);
    const std::uint32_t expectedPair1 = ExpectedFog565Pair(
        static_cast<std::uint32_t>(pixels[1]) | (static_cast<std::uint32_t>(pixels[2]) << 16),
        0x100000);
    const std::uint32_t expectedPair2 = ExpectedFog565Pair(
        static_cast<std::uint32_t>(pixels[3]) | (static_cast<std::uint32_t>(pixels[4]) << 16),
        0x200000);

    zRndr::FogBlendSpan565Scalar(pixels, 5, 0x80000, 0x80000);
    if (pixels[0] != expected0) {
        return 10;
    }
    if (pixels[1] != static_cast<std::uint16_t>(expectedPair1)) {
        return 11;
    }
    if (pixels[2] != static_cast<std::uint16_t>(expectedPair1 >> 16)) {
        return 12;
    }
    if (pixels[3] != static_cast<std::uint16_t>(expectedPair2)) {
        return 13;
    }
    if (pixels[4] != static_cast<std::uint16_t>(expectedPair2 >> 16)) {
        return 14;
    }

    pixels[0] = 0x1234;
    pixels[1] = 0x5678;
    zRndr::FogBlendSpan565Scalar(pixels, 2, 0x1000000, 0x80000);
    if (pixels[0] != 0xaaaa || pixels[1] != 0xaaaa) {
        return 15;
    }

    pixels[0] = 0x1234;
    pixels[1] = 0x5678;
    zRndr::FogBlendSpan565Scalar(pixels, 2, static_cast<std::int32_t>(0xfff00000u), 0x80000);
    return pixels[0] == 0x1234 && pixels[1] == 0x5678 ? 0 : 2;
}

extern "C" int zrndr_fog_blend_span_555_scalar_smoke(void) {
    zRndr::g_fogParamsActive = {};
    zRndr::g_fogParamsActive.packedColor16 = 0x5555;
    zRndr::g_fogParamsActive.packedColor16Dup = 0x55555555;
    zRndr::g_fogParamsActive.packedColorRamp[31] = 0x00010002;
    zRndr::g_fogParamsActive.packedColorRamp[30] = 0x00030004;

    std::uint16_t pixels[5] = {0x001f, 0x7c00, 0x03e0, 0x1111, 0x2222};
    const std::uint16_t expected0 = ExpectedFog555Pixel(pixels[0], 0x80000);
    const std::uint32_t expectedPair1 = ExpectedFog555Pair(
        static_cast<std::uint32_t>(pixels[1]) | (static_cast<std::uint32_t>(pixels[2]) << 16),
        0x100000);
    const std::uint32_t expectedPair2 = ExpectedFog555Pair(
        static_cast<std::uint32_t>(pixels[3]) | (static_cast<std::uint32_t>(pixels[4]) << 16),
        0x200000);

    zRndr::FogBlendSpan555Scalar(pixels, 5, 0x80000, 0x80000);
    if (pixels[0] != expected0) {
        return 10;
    }
    if (pixels[1] != static_cast<std::uint16_t>(expectedPair1)) {
        return 11;
    }
    if (pixels[2] != static_cast<std::uint16_t>(expectedPair1 >> 16)) {
        return 12;
    }
    if (pixels[3] != static_cast<std::uint16_t>(expectedPair2)) {
        return 13;
    }
    if (pixels[4] != static_cast<std::uint16_t>(expectedPair2 >> 16)) {
        return 14;
    }

    pixels[0] = 0x1234;
    pixels[1] = 0x5678;
    zRndr::FogBlendSpan555Scalar(pixels, 2, 0x1000000, 0x80000);
    if (pixels[0] != 0x5555 || pixels[1] != 0x5555) {
        return 15;
    }

    pixels[0] = 0x1234;
    pixels[1] = 0x5678;
    zRndr::FogBlendSpan555Scalar(pixels, 2, static_cast<std::int32_t>(0xfff00000u), 0x80000);
    return pixels[0] == 0x1234 && pixels[1] == 0x5678 ? 0 : 2;
}

extern "C" int zrndr_fog_blend_span_565_mmx_smoke(void) {
    zRndr::SpanMmxSetPixelFormatMasks(6);
    ReplicateMmxTargetBits(31, 63, 31);
    zRndr::g_fogParamsActive = {};
    zRndr::g_fogParamsActive.packedColorRamp[30] = 0x00030004;

    alignas(4) std::uint16_t pixels[5] = {0x0000, 0x001f, 0x07e0, 0xf800, 0x1234};
    std::uint16_t expected[5] = {};
    for (int i = 0; i < 4; ++i) {
        expected[i] = ExpectedFogMmxLane(
            pixels[i], static_cast<std::uint16_t>((0x20000 * (i + 1)) >> 16), i, 11, 3);
    }
    expected[4] = ExpectedFog565Pixel(pixels[4], 0xc0000);

    zRndr::FogBlendSpan565Mmx(pixels, 5, 0, 0x20000);
    if (pixels[0] != expected[0] || pixels[1] != expected[1] || pixels[2] != expected[2] ||
        pixels[3] != expected[3] || pixels[4] != expected[4]) {
        return 1;
    }

    return zRndr::g_mmxFogFactors[0] == 2 && zRndr::g_mmxFogFactors[3] == 8 ? 0 : 2;
}

extern "C" int zrndr_fog_blend_span_555_mmx_smoke(void) {
    zRndr::SpanMmxSetPixelFormatMasks(5);
    ReplicateMmxTargetBits(31, 31, 31);
    zRndr::g_fogParamsActive = {};
    zRndr::g_fogParamsActive.packedColorRamp[30] = 0x00030004;

    alignas(4) std::uint16_t pixels[5] = {0x0000, 0x001f, 0x03e0, 0x7c00, 0x1234};
    std::uint16_t expected[5] = {};
    for (int i = 0; i < 4; ++i) {
        expected[i] = ExpectedFogMmxLane(
            pixels[i], static_cast<std::uint16_t>((0x20000 * (i + 1)) >> 16), i, 10, 2);
    }
    expected[4] = ExpectedFog555Pixel(pixels[4], 0xc0000);

    zRndr::FogBlendSpan555Mmx(pixels, 5, 0, 0x20000);
    if (pixels[0] != expected[0] || pixels[1] != expected[1] || pixels[2] != expected[2] ||
        pixels[3] != expected[3] || pixels[4] != expected[4]) {
        return 1;
    }

    return zRndr::g_mmxFogFactors[0] == 2 && zRndr::g_mmxFogFactors[3] == 8 ? 0 : 2;
}

extern "C" int zrndr_span_copy_16_from_tex16_smoke(void) {
    std::uint16_t texels[16] = {};
    for (int i = 0; i < 16; ++i) {
        texels[i] = static_cast<std::uint16_t>(0x1000 + i);
    }

    alignas(4) std::uint16_t dst[5] = {};
    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x0f;

    zRndr::SpanMmxSetTexUvMasksAndVShift(12);
    if (zRndr::g_mmxVShiftCounts[0] != 12 || zRndr::g_mmxVShiftCounts[1] != 0 ||
        zRndr::g_mmxVMask[0] != 0 || zRndr::g_mmxUMask[0] != 0x00f00000) {
        return 1;
    }

    zRndr::SpanCopy16FromTex16(0, 0, 3, 12);
    if (dst[0] != 0x1000 || dst[1] != 0x1001 || dst[2] != 0x1002 ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 2;
    }

    dst[0] = dst[1] = dst[2] = 0;
    zRndr::SpanCopy16FromTex16ExplicitVShift(1 << 20, 0, 3, 12);
    return dst[0] == 0x1001 && dst[1] == 0x1002 && dst[2] == 0x1003 ? 0 : 3;
}

extern "C" int zrndr_span_copy_16_from_tex16_switch_vshift_smoke(void) {
    std::uint16_t texels[16] = {};
    for (int i = 0; i < 16; ++i) {
        texels[i] = static_cast<std::uint16_t>(0x2000 + i);
    }

    alignas(4) std::uint16_t dst[4] = {};
    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x0f;

    zRndr::SpanCopy16FromTex16SwitchVShift(0, 0, 4, 16);
    if (dst[0] != 0x2003 || dst[1] != 0x2002 || dst[2] != 0x2001 || dst[3] != 0x2000) {
        return 1;
    }

    dst[0] = 0x7777;
    zRndr::SpanCopy16FromTex16SwitchVShift(0, 0, 1, 9);
    return dst[0] == 0x7777 ? 0 : 2;
}

extern "C" int zrndr_span_masked_16_from_tex16_switch_vshift_smoke(void) {
    std::uint16_t texels[16] = {};
    texels[0] = 0x5000;
    texels[1] = 0;
    texels[2] = 0x5002;
    texels[3] = 0x5003;

    alignas(4) std::uint16_t dst[4] = {0xaaaa, 0xbbbb, 0xcccc, 0xdddd};
    zRndr::g_spanActiveTexels = reinterpret_cast<std::uint8_t *>(texels);
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x0f;

    zRndr::SpanMasked16FromTex16SwitchVShift(0, 0, 4, 16);
    if (dst[0] != 0x5003 || dst[1] != 0x5002 || dst[2] != 0xcccc || dst[3] != 0x5000 ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x7777;
    zRndr::SpanMasked16FromTex16SwitchVShift(0, 0, 1, 9);
    return dst[0] == 0x7777 ? 0 : 2;
}

extern "C" int zrndr_span_copy_16_from_pal8_switch_vshift_smoke(void) {
    std::uint8_t texels[16] = {};
    std::uint16_t palette[256] = {};
    for (int i = 0; i < 16; ++i) {
        texels[i] = static_cast<std::uint8_t>(i);
    }
    for (int i = 0; i < 256; ++i) {
        palette[i] = static_cast<std::uint16_t>(0x3000 + i);
    }

    alignas(4) std::uint16_t dst[4] = {};
    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x0f;

    zRndr::SpanCopy16FromPal8SwitchVShift(0, 0, 4, 16);
    if (dst[0] != 0x3003 || dst[1] != 0x3002 || dst[2] != 0x3001 || dst[3] != 0x3000) {
        return 1;
    }

    dst[0] = 0x7777;
    zRndr::SpanCopy16FromPal8SwitchVShift(0, 0, 1, 18);
    return dst[0] == 0x7777 ? 0 : 2;
}

extern "C" int zrndr_span_masked_16_from_pal8_switch_vshift_smoke(void) {
    std::uint8_t texels[16] = {};
    std::uint16_t palette[256] = {};
    texels[0] = 1;
    texels[1] = 0;
    texels[2] = 3;
    texels[3] = 4;
    for (int i = 0; i < 256; ++i) {
        palette[i] = static_cast<std::uint16_t>(0x6000 + i);
    }

    alignas(4) std::uint16_t dst[4] = {0xaaaa, 0xbbbb, 0xcccc, 0xdddd};
    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x0f;

    zRndr::SpanMasked16FromPal8SwitchVShift(0, 0, 4, 16);
    if (dst[0] != 0x6004 || dst[1] != 0x6003 || dst[2] != 0xcccc || dst[3] != 0x6001 ||
        zRndr::g_spanCurrentDst16 != dst) {
        return 1;
    }

    dst[0] = 0x7777;
    zRndr::SpanMasked16FromPal8SwitchVShift(0, 0, 1, 18);
    return dst[0] == 0x7777 ? 0 : 2;
}

extern "C" int zrndr_span_shade_16_from_pal8_switch_vshift_smoke(void) {
    std::uint8_t texels[16] = {};
    std::uint16_t palette[0x1000] = {};
    for (int i = 0; i < 16; ++i) {
        texels[i] = static_cast<std::uint8_t>(i + 1);
    }
    for (int i = 0; i < 0x1000; ++i) {
        palette[i] = static_cast<std::uint16_t>(0x4000 + i);
    }

    alignas(4) std::uint16_t dst[4] = {};
    zRndr::g_spanActiveTexels = texels;
    zRndr::g_spanActiveTexPalette = palette;
    zRndr::g_spanCurrentDst16 = dst;
    zRndr::g_spanTexUAdvance = 1 << 20;
    zRndr::g_spanTexVAdvance = 0;
    zRndr::g_spanActiveTexVMask = 0;
    zRndr::g_spanActiveTexUMask = 0x0f;
    zRndr::g_spanActiveShadeFixed16 = 0x00100000;
    zRndr::g_spanActiveShadeStepFixed16 = 0x00100000;

    zRndr::SpanShade16FromPal8SwitchVShift(0, 0, 4, 16);
    if (dst[0] != static_cast<std::uint16_t>(0x4000 + 4 + 0x800) ||
        dst[1] != static_cast<std::uint16_t>(0x4000 + 3 + 0x600) ||
        dst[2] != static_cast<std::uint16_t>(0x4000 + 2 + 0x400) ||
        dst[3] != static_cast<std::uint16_t>(0x4000 + 1 + 0x200) ||
        zRndr::g_spanActiveShadeFixed16 != 0x00500000) {
        return 1;
    }

    dst[0] = 0x7777;
    zRndr::g_spanActiveShadeFixed16 = 0x00100000;
    zRndr::SpanShade16FromPal8SwitchVShift(0, 0, 1, 9);
    return dst[0] == 0x7777 && zRndr::g_spanActiveShadeFixed16 == 0x00100000 ? 0 : 2;
}

extern "C" int zrndr_palette_remap_key_smoke(void) {
    std::free(g_zVid_PaletteRemapRecipes);
    g_zVid_PaletteRemapRecipes = nullptr;
    g_zVid_PaletteRemapRecipeCount = 0;
    g_zVid_PaletteRemapVariantTableCount = 0;
    g_zVid_PaletteRemapVariantTables = nullptr;
    g_zImage_TexDirEntryCount = 0;

    zVidPaletteRemapRecipe recipe{};
    recipe.color1R = 0.25f;
    recipe.color1G = 0.5f;
    recipe.color1B = 0.75f;
    recipe.color1Strength = 1.0f;

    zRndr_SetPaletteRemapKey(nullptr, 16.0f);
    if (g_zRndr_ActivePaletteRemapKey != -1) {
        return 1;
    }

    zRndr_SetPaletteRemapKey(&recipe, -8.0f);
    if (g_zVid_PaletteRemapRecipeCount != 1 || g_zRndr_ActivePaletteRemapKey != 0) {
        return 2;
    }

    zRndr_SetPaletteRemapKey(&recipe, 300.0f);
    if (g_zVid_PaletteRemapRecipeCount != 1 || g_zRndr_ActivePaletteRemapKey != 31) {
        return 3;
    }

    zColorRgb rgb{0.25f, 0.5f, 0.75f};
    if (zVid_PaletteRemap_FindRecipeIndexFromRgb(&rgb) != 0) {
        return 4;
    }

    zRndr_SetPaletteRemapKeyFromRgb01(&rgb, 16.0f);
    if (g_zRndr_ActivePaletteRemapKey != 2) {
        return 5;
    }

    zRndr_SetPaletteShadeRecipeIndex(nullptr);
    if (g_zRndr_ActivePaletteShadeRecipeIndex != -1) {
        return 6;
    }

    zRndr_SetPaletteShadeRecipeIndex(&recipe);
    if (g_zRndr_ActivePaletteShadeRecipeIndex != 0) {
        return 7;
    }

    std::free(g_zVid_PaletteRemapRecipes);
    g_zVid_PaletteRemapRecipes = nullptr;
    g_zVid_PaletteRemapRecipeCount = 0;
    return 0;
}

extern "C" int zrndr_fog_target_color_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_fogTargetParamsStaged = {};
    g_zVideo_RendererType = 1;
    g_zVideo_D3DColorNormalizeChannelIndex = -1;

    zColorRgb color{1.2f, -0.5f, 0.5f};
    zRndr_FogTargetColorStaged_SetRgb01Clamped(&color);
    if (color.red != 1.0f || color.green != 0.0f || color.blue != 0.5f ||
        zRndr::g_fogTargetParamsStaged.colorRgb01[0] != 1.0f ||
        zRndr::g_fogTargetParamsStaged.colorRgb01[1] != 0.0f ||
        zRndr::g_fogTargetParamsStaged.colorRgb01[2] != 0.5f ||
        g_zVideo_D3DColorAttrBiasR != 255.0f || g_zVideo_D3DColorAttrBiasG != 0.0f ||
        g_zVideo_D3DColorAttrBiasB != 127.5f || g_zVideo_D3DColorNormalizeChannelIndex != 0) {
        return 1;
    }

    if (zRndr::g_fogTargetParamsStaged.packedColorRed != 0xf800 ||
        zRndr::g_fogTargetParamsStaged.packedColorGreen != 0x0000 ||
        zRndr::g_fogTargetParamsStaged.packedColorBlue != 0x0010 ||
        zRndr::g_fogTargetParamsStaged.packedColor16 != 0xf810 ||
        zRndr::g_fogTargetParamsStaged.packedColor16Dup != static_cast<std::int32_t>(0xf810f810) ||
        zRndr::g_fogTargetParamsStaged.packedColorRamp[31] != 0) {
        return 2;
    }

    const std::int32_t oldPackedColor = zRndr::g_fogTargetParamsStaged.packedColor16;
    zRndr::g_fogTargetParamsStaged.packedColor16 = 0x1234;
    color = {0.995f, 0.004f, 0.504f};
    zRndr_FogTargetColorStaged_SetRgb01Clamped(&color);
    if (zRndr::g_fogTargetParamsStaged.packedColor16 != 0x1234) {
        return 3;
    }

    zRndr::g_fogParamsActive = {};
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_FogTargetColorR255 = 0.0f;
    g_zVideo_FogTargetColorG255 = 0.0f;
    g_zVideo_FogTargetColorB255 = 0.0f;
    color = {1.2f, -0.5f, 0.5f};
    zRndr::SetFogTargetColorRgb01Clamped(&color);
    if (color.red != 1.0f || color.green != 0.0f || color.blue != 0.5f ||
        zRndr::g_fogParamsActive.colorRgb01[0] != 1.0f ||
        zRndr::g_fogParamsActive.colorRgb01[1] != 0.0f ||
        zRndr::g_fogParamsActive.colorRgb01[2] != 0.5f || g_zVideo_FogTargetColorR255 != 255.0f ||
        g_zVideo_FogTargetColorG255 != 0.0f || g_zVideo_FogTargetColorB255 != 127.5f ||
        zRndr::g_fogParamsActive.packedColor16 != 0xf810) {
        return 5;
    }

    zRndr::g_fogParamsActive.packedColor16 = 0x1234;
    color = {0.995f, 0.004f, 0.504f};
    zRndr::SetFogTargetColorRgb01Clamped(&color);
    if (zRndr::g_fogParamsActive.packedColor16 != 0x1234) {
        return 6;
    }

    zRndr::g_fogTargetParamsStaged.packedColor16 = oldPackedColor;
    zRndr::FogParamsPartial params{};
    zRndr::SpanAlphaBlend565_Mmx_FromPal8(&params, 0xf800, 0x0400, 0x001f);
    const std::uint32_t step =
        (static_cast<std::uint32_t>(0xfc00) << 11) | (static_cast<std::uint32_t>(0x001f) >> 5);
    return params.packedColorRed == 0xf800 && params.packedColorGreen == 0x0400 &&
                   params.packedColorBlue == 0x001f && params.packedColor16 == 0xfc1f &&
                   params.packedColorRamp[31] == 0 &&
                   params.packedColorRamp[30] == static_cast<std::int32_t>(step)
               ? 0
               : 4;
}

extern "C" int zrndr_fog_commit_and_blend_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zRndr::g_fogParamsActive = {};
    zRndr::g_fogTargetParamsDirect = {};
    zRndr::g_fogTargetParamsDirect.colorRgb01[0] = 0.02f;
    zRndr::g_fogTargetParamsDirect.colorRgb01[1] = 0.0f;
    zRndr::g_fogTargetParamsDirect.colorRgb01[2] = 0.0f;
    zRndr::g_fogTargetParamsDirect.packedColor16 = 0x1111;
    zRndr::CommitDirectFogParamsIfChanged();
    if (zRndr::g_fogParamsActive.packedColor16 != 0x1111) {
        return 1;
    }

    zRndr::g_fogTargetParamsStaged = zRndr::g_fogParamsActive;
    zRndr::g_fogTargetParamsStaged.colorRgb01[0] += 0.005f;
    zRndr::g_fogTargetParamsStaged.packedColor16 = 0x2222;
    zRndr::CommitStagedFogParamsIfChanged();
    if (zRndr::g_fogParamsActive.packedColor16 != 0x1111) {
        return 2;
    }

    zRndr::g_fogColorParams = zRndr::g_fogParamsActive;
    zRndr::g_fogColorParams.colorRgb01[2] = 0.5f;
    zRndr::g_fogColorParams.packedColor16 = 0x3333;
    zRndr::CommitFogColorParamsIfChanged();
    if (zRndr::g_fogParamsActive.packedColor16 != 0x3333) {
        return 3;
    }

    zRndr::g_fogParamsActive.packedColorRed = 0xf800;
    zRndr::g_fogParamsActive.packedColorGreen = 0x07e0;
    zRndr::g_fogParamsActive.packedColorBlue = 0x001f;
    zRndr::g_pixelPackRedMask = 0xf800;
    zRndr::g_pixelPackGreenMask = 0x07e0;
    zRndr::g_pixelPackBlueMask = 0x001f;
    std::int32_t color = 0;
    zRndr::BlendPackedColor565WithFogInPlace(&color, 128);
    return color == 0x7bef ? 0 : 4;
}

extern "C" int zrndr_and_zmodel_current_fog_color_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_fogColorParams = {};
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_FogColorPendingR255 = 0.0f;
    g_zVideo_FogColorPendingG255 = 0.0f;
    g_zVideo_FogColorPendingB255 = 0.0f;

    zColorRgb color{1.5f, -0.25f, 0.25f};
    zRndr::FogColor_SetRgb01Clamped(&color);
    if (color.red != 1.0f || color.green != 0.0f || color.blue != 0.25f ||
        zRndr::g_fogColorParams.colorRgb01[0] != 1.0f ||
        zRndr::g_fogColorParams.colorRgb01[1] != 0.0f ||
        zRndr::g_fogColorParams.colorRgb01[2] != 0.25f || g_zVideo_FogColorPendingR255 != 255.0f ||
        g_zVideo_FogColorPendingG255 != 0.0f || g_zVideo_FogColorPendingB255 != 63.75f) {
        return 1;
    }

    if (zRndr::g_fogColorParams.packedColorRed != 0xf800 ||
        zRndr::g_fogColorParams.packedColorGreen != 0x0000 ||
        zRndr::g_fogColorParams.packedColorBlue != 0x0008 ||
        zRndr::g_fogColorParams.packedColor16 != 0xf808) {
        return 2;
    }

    gModel_FogDistanceEnd = 20.0f;
    gModel_FogDistanceInvRange = -1.0f;
    zModel_Fog_SetDistanceStart(5.0f);
    if (gModel_FogDistanceStart != 5.0f || gModel_FogDistanceInvRange < 0.066f ||
        gModel_FogDistanceInvRange > 0.067f) {
        return 3;
    }

    zModel_Fog_SetDistanceEnd(25.0f);
    if (gModel_FogDistanceEnd != 25.0f || gModel_FogDistanceInvRange != 0.05f) {
        return 4;
    }

    gModel_FogHeightLow = 2.0f;
    zModel_Fog_SetHeightHigh(12.0f);
    if (gModel_FogHeightHigh != 12.0f || gModel_FogHeightInvRange != 0.1f) {
        return 5;
    }

    zModel_Fog_SetHeightLow(7.0f);
    zModel_Fog_SetDensity(0.75f);
    zModel_Fog_SetEnabled(1);
    zModel_Fog_SetLinearModeEnabled(0);
    if (gModel_FogHeightLow != 7.0f || gModel_FogHeightInvRange != 0.2f ||
        gModel_FogDensity != 0.75f || gModel_FogEnabled != 1 || gModel_FogLinearModeEnabled != 0) {
        return 6;
    }

    zColorRgb modelColor{0.2f, 0.4f, 0.6f};
    zModel_Fog_SetColorRgb01(&modelColor);
    if (gModel_FogColorRgb01.red != 0.2f || gModel_FogColorRgb01.green != 0.4f ||
        gModel_FogColorRgb01.blue != 0.6f || g_zVideo_FogColorPendingR255 != 51.0f ||
        g_zVideo_FogColorPendingG255 != 102.0f || g_zVideo_FogColorPendingB255 != 153.0f) {
        return 7;
    }

    zRndr::g_fogColorParams = {};
    gModel_FogColorRgb01 = {0.5f, 0.25f, 1.0f};
    zModel_Fog_ApplyCurrentColor();
    return zRndr::g_fogColorParams.colorRgb01[0] == 0.5f &&
                   zRndr::g_fogColorParams.colorRgb01[1] == 0.25f &&
                   zRndr::g_fogColorParams.colorRgb01[2] == 1.0f
               ? 0
               : 8;
}
