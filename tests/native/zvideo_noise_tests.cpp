#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdlib>

namespace {
unsigned short PackNoiseGray565(unsigned char value) {
    const unsigned short level = static_cast<unsigned short>(value & 0x1f);
    return static_cast<unsigned short>((level << 11) | (level << 6) | level);
}
}

extern "C" int zvideo_draw_noise_rect_smoke(void) {
    unsigned char *const oldNoiseTable = g_zVid_NoiseByteTable;
    const int oldNoiseTableSize = g_zVid_NoiseByteTableSize;
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;

    unsigned char noiseTable[32] = {};
    for (int i = 0; i < 32; ++i) {
        noiseTable[i] = static_cast<unsigned char>(i);
    }

    unsigned short pixels[25] = {};
    for (int i = 0; i < 25; ++i) {
        pixels[i] = 0xaaaa;
    }

    g_zVid_NoiseByteTable = noiseTable;
    g_zVid_NoiseByteTableSize = 32;
    g_zVideo_FxSurfacePixels16 = pixels;
    g_zVideo_FxSurfaceWidth = 5;
    g_zVideo_FxSurfaceHeight = 5;
    g_zVideo_FxSurfacePitchBytes = 10;
    g_zVideo_FxSurfacePitchPixels16 = 5;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zVidRect32 rect = {1, 1, 4, 3};
    zVid::DrawNoiseRect(&rect, 0.0);
    bool lowIntensityOk = true;
    for (int i = 0; i < 25; ++i) {
        lowIntensityOk = lowIntensityOk && pixels[i] == 0xaaaa;
    }

    std::srand(7);
    const int rowWidth = rect.right - rect.left;
    const int firstOffset = (std::rand() * (g_zVid_NoiseByteTableSize - rowWidth)) / 0x7fff;
    const int secondOffset = (std::rand() * (g_zVid_NoiseByteTableSize - rowWidth)) / 0x7fff;
    std::srand(7);
    zVid::DrawNoiseRect(&rect, 1.0);

    const bool rowOneOk =
        pixels[1 + 1 * 5] == PackNoiseGray565(noiseTable[firstOffset]) &&
        pixels[2 + 1 * 5] == PackNoiseGray565(noiseTable[firstOffset + 1]) &&
        pixels[3 + 1 * 5] == PackNoiseGray565(noiseTable[firstOffset + 2]);
    const bool rowTwoOk =
        pixels[1 + 2 * 5] == PackNoiseGray565(noiseTable[secondOffset]) &&
        pixels[2 + 2 * 5] == PackNoiseGray565(noiseTable[secondOffset + 1]) &&
        pixels[3 + 2 * 5] == PackNoiseGray565(noiseTable[secondOffset + 2]);
    const bool untouchedOk =
        pixels[0] == 0xaaaa &&
        pixels[4] == 0xaaaa &&
        pixels[1 + 3 * 5] == 0xaaaa &&
        pixels[24] == 0xaaaa;

    g_zVid_NoiseByteTable = oldNoiseTable;
    g_zVid_NoiseByteTableSize = oldNoiseTableSize;
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;

    return lowIntensityOk && rowOneOk && rowTwoOk && untouchedOk ? 0 : 1;
}
