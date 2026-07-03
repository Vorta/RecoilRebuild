#include "GameZRecoil/zRndr/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstring>

extern "C" int zvideo_fx_surface_alpha_line_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldPitchBytes = zRndr::g_pitchBytes;
    const int oldGreenBits = zRndr::g_pixelPackGreenBits;

    unsigned short pixels[36] = {};
    for (int i = 0; i < 36; ++i) {
        pixels[i] = 0x001f;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 6;
    g_zVideo_FxSurfaceHeight = 6;
    g_zVideo_FxSurfacePitchBytes = 12;
    g_zVideo_FxSurfacePitchPixels16 = 6;
    zRndr::g_pitchBytes = 12;
    zRndr::g_pixelPackGreenBits = 6;

    zVidRect32 clip = {0, 0, 5, 5};
    zVideo_FxSurface::DrawAlphaBlendedLine(&clip, 4, 1, 1, 1, 0xf800, 1.0f, 1.0f, 1);
    const bool horizontalOk = pixels[1 + 1 * 6] == 0xf800 && pixels[2 + 1 * 6] == 0xf800 &&
                              pixels[3 + 1 * 6] == 0xf800 && pixels[4 + 1 * 6] == 0xf800;

    for (int i = 0; i < 36; ++i) {
        pixels[i] = 0x07e0;
    }
    zVideo_FxSurface::DrawAlphaBlendedLine(&clip, 4, 2, 1, 2, 0xf800, 0.0f, 0.0f, 1);
    bool lowAlphaOk = true;
    for (int x = 1; x <= 4; ++x) {
        lowAlphaOk = lowAlphaOk && pixels[x + 2 * 6] == 0x07e0;
    }

    for (int i = 0; i < 36; ++i) {
        pixels[i] = 0x0000;
    }
    zVideo_FxSurface::DrawAlphaBlendedLine(&clip, 3, 3, 1, 3, 0xf800, 0.5f, 0.5f, 1);
    const unsigned short blended565 = pixels[1 + 3 * 6];
    const bool blendOk = blended565 != 0x0000 && blended565 != 0xf800 &&
                         pixels[2 + 3 * 6] == blended565 && pixels[3 + 3 * 6] == blended565;

    for (int i = 0; i < 36; ++i) {
        pixels[i] = 0xaaaa;
    }
    zVidRect32 clipped = {1, 0, 5, 5};
    zVideo_FxSurface::DrawAlphaBlendedLine(&clipped, 5, 2, 0, 2, 0xf800, 1.0f, 1.0f, 1);
    const bool clipOk = pixels[1 + 2 * 6] == 0xaaaa && pixels[2 + 2 * 6] == 0xf800 &&
                        pixels[3 + 2 * 6] == 0xf800 && pixels[4 + 2 * 6] == 0xf800 &&
                        pixels[5 + 2 * 6] == 0xaaaa;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    zRndr::g_pitchBytes = oldPitchBytes;
    zRndr::g_pixelPackGreenBits = oldGreenBits;

    return horizontalOk && lowAlphaOk && blendOk && clipOk ? 0 : 1;
}

extern "C" int zvideo_fx_surface_apply_blue_tint_rect_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldQuadBatchCount = g_zVideo_QuadBatchCount;
    zVideo_QuadBatchItemPartial oldQuadBatchItems[16];
    std::memcpy(oldQuadBatchItems, g_zVideo_QuadBatchItemsBase, sizeof(oldQuadBatchItems));

    unsigned short pixels[24] = {};
    for (int i = 0; i < 24; ++i) {
        pixels[i] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 6;
    g_zVideo_FxSurfaceHeight = 4;
    g_zVideo_FxSurfacePitchBytes = 12;
    g_zVideo_FxSurfacePitchPixels16 = 6;
    g_zVideo_ActiveRendererPath = 0;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zVidRect32 rect = {1, 1, 5, 3};
    zVideo_FxSurface::ApplyBlueTintRect(&rect);

    const bool softwareRegionOk =
        pixels[1 + 1 * 6] == 0x7bff && pixels[2 + 1 * 6] == 0x7bff &&
        pixels[3 + 1 * 6] == 0x7bff && pixels[4 + 1 * 6] == 0x7bff &&
        pixels[1 + 2 * 6] == 0x7bff && pixels[2 + 2 * 6] == 0x7bff &&
        pixels[3 + 2 * 6] == 0x7bff && pixels[4 + 2 * 6] == 0x7bff;
    const bool softwareEdgesOk =
        pixels[0 + 1 * 6] == 0xffff && pixels[5 + 1 * 6] == 0xffff &&
        pixels[0 + 2 * 6] == 0xffff && pixels[5 + 2 * 6] == 0xffff &&
        pixels[1 + 3 * 6] == 0xffff;

    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_ActiveRendererPath = 1;
    zVidRect32 hwRect = {2, 3, 4, 5};
    zVideo_FxSurface::ApplyBlueTintRect(&hwRect);
    const zVideo_QuadBatchItemPartial &quad = g_zVideo_QuadBatchItemsBase[0];
    const bool hardwareOk =
        g_zVideo_QuadBatchCount == 1 &&
        quad.vertices[0].sx == 2.0f && quad.vertices[0].sy == 3.0f &&
        quad.vertices[1].sx == 4.0f && quad.vertices[1].sy == 3.0f &&
        quad.vertices[2].sx == 4.0f && quad.vertices[2].sy == 5.0f &&
        quad.vertices[3].sx == 2.0f && quad.vertices[3].sy == 5.0f &&
        quad.vertices[0].color == 0x4c0000f8 && quad.vertices[3].color == 0x4c0000f8;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_QuadBatchCount = oldQuadBatchCount;
    std::memcpy(g_zVideo_QuadBatchItemsBase, oldQuadBatchItems, sizeof(oldQuadBatchItems));

    if (!softwareRegionOk) {
        return 1;
    }
    if (!softwareEdgesOk) {
        return 2;
    }
    if (!hardwareOk) {
        return 3;
    }
    return 0;
}

extern "C" int zvideo_fx_surface_apply_green_mask_rect_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const int oldQuadBatchCount = g_zVideo_QuadBatchCount;
    zVideo_QuadBatchItemPartial oldQuadBatchItems[16];
    std::memcpy(oldQuadBatchItems, g_zVideo_QuadBatchItemsBase, sizeof(oldQuadBatchItems));

    unsigned short pixels[24] = {};
    for (int i = 0; i < 24; ++i) {
        pixels[i] = 0xffff;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 6;
    g_zVideo_FxSurfaceHeight = 4;
    g_zVideo_FxSurfacePitchBytes = 12;
    g_zVideo_FxSurfacePitchPixels16 = 6;
    g_zVideo_ActiveRendererPath = 0;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zVidRect32 rect = {1, 1, 5, 3};
    zVideo_FxSurface::ApplyGreenMaskRect(&rect);

    const bool softwareRegionOk =
        pixels[1 + 1 * 6] == 0x07e0 && pixels[2 + 1 * 6] == 0x07e0 &&
        pixels[3 + 1 * 6] == 0x07e0 && pixels[4 + 1 * 6] == 0x07e0 &&
        pixels[1 + 2 * 6] == 0x07e0 && pixels[2 + 2 * 6] == 0x07e0 &&
        pixels[3 + 2 * 6] == 0x07e0 && pixels[4 + 2 * 6] == 0x07e0;
    const bool softwareEdgesOk =
        pixels[0 + 1 * 6] == 0xffff && pixels[5 + 1 * 6] == 0xffff &&
        pixels[0 + 2 * 6] == 0xffff && pixels[5 + 2 * 6] == 0xffff &&
        pixels[1 + 3 * 6] == 0xffff;

    std::memset(g_zVideo_QuadBatchItemsBase, 0, sizeof(g_zVideo_QuadBatchItemsBase));
    g_zVideo_QuadBatchCount = 0;
    g_zVideo_ActiveRendererPath = 1;
    zVidRect32 hwRect = {2, 3, 4, 5};
    zVideo_FxSurface::ApplyGreenMaskRect(&hwRect);
    const zVideo_QuadBatchItemPartial &quad = g_zVideo_QuadBatchItemsBase[0];
    const bool hardwareOk =
        g_zVideo_QuadBatchCount == 1 &&
        quad.vertices[0].sx == 2.0f && quad.vertices[0].sy == 3.0f &&
        quad.vertices[1].sx == 4.0f && quad.vertices[1].sy == 3.0f &&
        quad.vertices[2].sx == 4.0f && quad.vertices[2].sy == 5.0f &&
        quad.vertices[3].sx == 2.0f && quad.vertices[3].sy == 5.0f &&
        quad.vertices[0].color == 0x4c00fc00 && quad.vertices[3].color == 0x4c00fc00;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_QuadBatchCount = oldQuadBatchCount;
    std::memcpy(g_zVideo_QuadBatchItemsBase, oldQuadBatchItems, sizeof(oldQuadBatchItems));

    if (!softwareRegionOk) {
        return 1;
    }
    if (!softwareEdgesOk) {
        return 2;
    }
    if (!hardwareOk) {
        return 3;
    }
    return 0;
}

extern "C" int zvideo_fx_surface_colored_lines_batch_smoke(void) {
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldPitchBytes = zRndr::g_pitchBytes;
    const int oldGreenBits = zRndr::g_pixelPackGreenBits;

    unsigned short pixels[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = 0x0000;
    }

    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 5;
    g_zVideo_FxSurfaceHeight = 5;
    g_zVideo_FxSurfacePitchBytes = 10;
    g_zVideo_FxSurfacePitchPixels16 = 5;
    zRndr::g_pitchBytes = 10;
    zRndr::g_pixelPackGreenBits = 6;

    zVideoFxColoredLineRecord lines[2] = {};
    lines[0].x = 1;
    lines[0].y = 1;
    lines[0].width = 2;
    lines[0].height = 0;
    lines[0].color16 = 0xf800;
    lines[0].alphaEnd = 1.0f;
    lines[0].alphaStart = 1.0f;
    lines[0].clipInset = 1;
    lines[1].x = 0;
    lines[1].y = 3;
    lines[1].width = 4;
    lines[1].height = 0;
    lines[1].color16 = 0x07e0;
    lines[1].alphaEnd = 1.0f;
    lines[1].alphaStart = 1.0f;
    lines[1].clipInset = 1;

    zVidRect32 clip = {-2, -1, 5, 6};
    zVideo_FxSurface::DrawColoredLinesBatch(lines, 2, &clip);
    const bool firstLineOk = pixels[1 + 1 * 5] == 0xf800 &&
                             pixels[2 + 1 * 5] == 0xf800 &&
                             pixels[3 + 1 * 5] == 0xf800;
    const bool secondLineClippedOk = pixels[0 + 3 * 5] == 0x0000 &&
                                     pixels[1 + 3 * 5] == 0x07e0 &&
                                     pixels[2 + 3 * 5] == 0x07e0 &&
                                     pixels[3 + 3 * 5] == 0x07e0 &&
                                     pixels[4 + 3 * 5] == 0x0000;

    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    zRndr::g_pitchBytes = oldPitchBytes;
    zRndr::g_pixelPackGreenBits = oldGreenBits;

    return firstLineOk && secondLineClippedOk ? 0 : 1;
}
