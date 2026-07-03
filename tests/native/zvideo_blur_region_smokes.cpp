#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>

namespace {

unsigned short BlurTestAverage(
    unsigned short before,
    unsigned short center,
    unsigned short after
) {
    const unsigned int rbMask = 0xf81f;
    const unsigned int greenMask = 0x07e0;
    const unsigned int rb =
        (before & rbMask) + ((center & rbMask) << 1) + (after & rbMask);
    const unsigned int green =
        (before & greenMask) + ((center & greenMask) << 1) + (after & greenMask);
    return static_cast<unsigned short>(((rb >> 2) & rbMask) | ((green >> 2) & greenMask));
}

void SaveBlurGlobals(
    unsigned short **oldFxPixels,
    unsigned short **oldScratch,
    int *oldWidth,
    int *oldHeight,
    int *oldPitchBytes,
    int *oldPitchPixels
) {
    *oldFxPixels = g_zVideo_FxSurfacePixels16;
    *oldScratch = g_zVideo_FxPass3_ScratchPixels16;
    *oldWidth = g_zVideo_FxSurfaceWidth;
    *oldHeight = g_zVideo_FxSurfaceHeight;
    *oldPitchBytes = g_zVideo_FxSurfacePitchBytes;
    *oldPitchPixels = g_zVideo_FxSurfacePitchPixels16;
}

void RestoreBlurGlobals(
    unsigned short *oldFxPixels,
    unsigned short *oldScratch,
    int oldWidth,
    int oldHeight,
    int oldPitchBytes,
    int oldPitchPixels
) {
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxPass3_ScratchPixels16 = oldScratch;
    g_zVideo_FxSurfaceWidth = oldWidth;
    g_zVideo_FxSurfaceHeight = oldHeight;
    g_zVideo_FxSurfacePitchBytes = oldPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldPitchPixels;
}

void SetupBlurSurface(
    unsigned short *pixels,
    unsigned short *scratch,
    int width,
    int height
) {
    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxPass3_ScratchPixels16 = scratch;
    g_zVideo_FxSurfaceWidth = width;
    g_zVideo_FxSurfaceHeight = height;
    g_zVideo_FxSurfacePitchBytes = width * 2;
    g_zVideo_FxSurfacePitchPixels16 = width;
    zVideo::PixelPack_SetupFromMasks(
        5,
        6,
        5,
        0xf800,
        0x07e0,
        0x001f
    );
}

} // namespace

extern "C" int zvideo_blur_region_horizontal_smoke(void) {
    unsigned short *oldFxPixels;
    unsigned short *oldScratch;
    int oldWidth;
    int oldHeight;
    int oldPitchBytes;
    int oldPitchPixels;
    SaveBlurGlobals(
        &oldFxPixels,
        &oldScratch,
        &oldWidth,
        &oldHeight,
        &oldPitchBytes,
        &oldPitchPixels
    );

    unsigned short pixels[20];
    unsigned short original[20];
    unsigned short scratch[20] = {};
    for (int i = 0; i < 20; ++i) {
        pixels[i] = static_cast<unsigned short>(i);
        original[i] = pixels[i];
    }

    SetupBlurSurface(
        pixels,
        scratch,
        5,
        4
    );
    zVidRect32 rect = {1, 1, 4, 2};
    zVideo::buff_BlurRegionHorizontal(
        &rect,
        1
    );

    bool ok = true;
    for (int i = 0; i < 20; ++i) {
        unsigned short expected = original[i];
        const int x = i % 5;
        const int y = i / 5;
        if (y >= 1 && y <= 2 && x >= 1 && x < 4) {
            expected = BlurTestAverage(
                original[i - 1],
                original[i],
                original[i + 1]
            );
        }
        ok = ok && pixels[i] == expected;
    }

    RestoreBlurGlobals(
        oldFxPixels,
        oldScratch,
        oldWidth,
        oldHeight,
        oldPitchBytes,
        oldPitchPixels
    );
    return ok ? 0 : 1;
}

extern "C" int zvideo_blur_region_vertical_smoke(void) {
    unsigned short *oldFxPixels;
    unsigned short *oldScratch;
    int oldWidth;
    int oldHeight;
    int oldPitchBytes;
    int oldPitchPixels;
    SaveBlurGlobals(
        &oldFxPixels,
        &oldScratch,
        &oldWidth,
        &oldHeight,
        &oldPitchBytes,
        &oldPitchPixels
    );

    unsigned short pixels[25];
    unsigned short original[25];
    unsigned short scratch[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = static_cast<unsigned short>(i);
        original[i] = pixels[i];
    }

    SetupBlurSurface(
        pixels,
        scratch,
        5,
        5
    );
    zVidRect32 rect = {0, 1, 4, 3};
    zVideo::buff_BlurRegionVertical(
        &rect,
        2
    );

    bool ok = true;
    for (int i = 0; i < 25; ++i) {
        unsigned short expected = original[i];
        const int y = i / 5;
        if (y >= 1 && y < 3) {
            expected = BlurTestAverage(
                original[i - 5],
                original[i],
                original[i + 5]
            );
        }
        ok = ok && pixels[i] == expected;
    }

    RestoreBlurGlobals(
        oldFxPixels,
        oldScratch,
        oldWidth,
        oldHeight,
        oldPitchBytes,
        oldPitchPixels
    );
    return ok ? 0 : 1;
}

extern "C" int zvideo_blur_region_combined_smoke(void) {
    unsigned short *oldFxPixels;
    unsigned short *oldScratch;
    int oldWidth;
    int oldHeight;
    int oldPitchBytes;
    int oldPitchPixels;
    SaveBlurGlobals(
        &oldFxPixels,
        &oldScratch,
        &oldWidth,
        &oldHeight,
        &oldPitchBytes,
        &oldPitchPixels
    );

    unsigned short pixels[25];
    unsigned short expectedScratch[25];
    unsigned short expected[25];
    unsigned short scratch[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = static_cast<unsigned short>((i * 3) & 0xffff);
        expected[i] = pixels[i];
        expectedScratch[i] = pixels[i];
    }

    SetupBlurSurface(
        pixels,
        scratch,
        5,
        5
    );
    zVideo::buff_BlurRegionCombined(
        0,
        3
    );

    for (int y = 1; y < 4; ++y) {
        for (int x = 0; x < 5; ++x) {
            const int index = y * 5 + x;
            expectedScratch[index] = BlurTestAverage(
                expected[index - 5],
                expected[index],
                expected[index + 5]
            );
        }
    }
    for (int i = 0; i < 25; ++i) {
        expected[i] = expectedScratch[i];
    }
    for (int y = 0; y < 5; ++y) {
        for (int x = 1; x < 4; ++x) {
            const int index = y * 5 + x;
            expected[index] = BlurTestAverage(
                expectedScratch[index - 1],
                expectedScratch[index],
                expectedScratch[index + 1]
            );
        }
    }

    bool ok = true;
    for (int i = 0; i < 25; ++i) {
        ok = ok && pixels[i] == expected[i];
    }

    RestoreBlurGlobals(
        oldFxPixels,
        oldScratch,
        oldWidth,
        oldHeight,
        oldPitchBytes,
        oldPitchPixels
    );
    return ok ? 0 : 1;
}

extern "C" int zvideo_blur_region_by_mode_smoke(void) {
    unsigned short *oldFxPixels;
    unsigned short *oldScratch;
    int oldWidth;
    int oldHeight;
    int oldPitchBytes;
    int oldPitchPixels;
    SaveBlurGlobals(
        &oldFxPixels,
        &oldScratch,
        &oldWidth,
        &oldHeight,
        &oldPitchBytes,
        &oldPitchPixels
    );

    unsigned short pixelsA[25];
    unsigned short pixelsB[25];
    unsigned short scratch[25] = {};
    bool ok = true;
    for (int mode = 1; mode <= 3; ++mode) {
        for (int i = 0; i < 25; ++i) {
            pixelsA[i] = static_cast<unsigned short>((i * 5 + mode) & 0xffff);
            pixelsB[i] = pixelsA[i];
            scratch[i] = 0;
        }

        SetupBlurSurface(
            pixelsA,
            scratch,
            5,
            5
        );
        zVideo::buff_BlurRegionByMode(
            0,
            mode
        );
        for (int i = 0; i < 25; ++i) {
            scratch[i] = 0;
        }
        SetupBlurSurface(
            pixelsB,
            scratch,
            5,
            5
        );
        if (mode == 1) {
            zVideo::buff_BlurRegionHorizontal(
                0,
                mode
            );
        } else if (mode == 2) {
            zVideo::buff_BlurRegionVertical(
                0,
                mode
            );
        } else {
            zVideo::buff_BlurRegionCombined(
                0,
                mode
            );
        }

        for (int i = 0; i < 25; ++i) {
            ok = ok && pixelsA[i] == pixelsB[i];
        }
    }

    RestoreBlurGlobals(
        oldFxPixels,
        oldScratch,
        oldWidth,
        oldHeight,
        oldPitchBytes,
        oldPitchPixels
    );
    return ok ? 0 : 1;
}
