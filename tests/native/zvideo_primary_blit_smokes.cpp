#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zRender/zrndr.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <stdint.h>

namespace {
struct FakeDirectDrawSurface3Object {
    void **vtable;
};

void *g_fakeDirectDrawSurface3VTable[40];
int g_fakeDirectDrawSurface3BltCalls;
int g_fakeDirectDrawSurface3LockCalls;
int g_fakeDirectDrawSurface3UnlockCalls;
int g_fakeDirectDrawSurface3RestoreCalls;
RECT g_fakeDirectDrawSurface3LastBltDstRect;
RECT g_fakeDirectDrawSurface3LastBltSrcRect;
IDirectDrawSurface3 *g_fakeDirectDrawSurface3LastBltSource;
DWORD g_fakeDirectDrawSurface3LastBltFlags;
LPDDBLTFX g_fakeDirectDrawSurface3LastBltFx;
unsigned char g_fakeDirectDrawSurface3LockPixels[64];
int g_captureBltSourceToPrimaryCalls;
zVidImagePartial *g_captureBltSourceToPrimaryImage;
int g_captureBltSourceToPrimaryDstX;
int g_captureBltSourceToPrimaryDstY;
int g_captureBltSourceToPrimaryClipFlags;
zVidRect32 *g_captureBltSourceToPrimarySrcRect;

HRESULT __stdcall FakeDirectDrawSurface3_Blt(
    IDirectDrawSurface3 *,
    LPRECT dstRect,
    LPDIRECTDRAWSURFACE3 sourceSurface,
    LPRECT srcRect,
    DWORD flags,
    LPDDBLTFX bltFx
) {
    ++g_fakeDirectDrawSurface3BltCalls;
    g_fakeDirectDrawSurface3LastBltDstRect =
        dstRect != 0 ? *dstRect : RECT();
    g_fakeDirectDrawSurface3LastBltSrcRect =
        srcRect != 0 ? *srcRect : RECT();
    g_fakeDirectDrawSurface3LastBltSource = sourceSurface;
    g_fakeDirectDrawSurface3LastBltFlags = flags;
    g_fakeDirectDrawSurface3LastBltFx = bltFx;
    return DD_OK;
}

HRESULT __stdcall FakeDirectDrawSurface3_Lock(
    IDirectDrawSurface3 *,
    LPRECT,
    LPDDSURFACEDESC surfaceDesc,
    DWORD,
    HANDLE
) {
    ++g_fakeDirectDrawSurface3LockCalls;
    surfaceDesc->dwWidth = 100;
    surfaceDesc->dwHeight = 80;
    surfaceDesc->lPitch = 200;
    surfaceDesc->lpSurface = g_fakeDirectDrawSurface3LockPixels;
    return DD_OK;
}

HRESULT __stdcall FakeDirectDrawSurface3_Unlock(
    IDirectDrawSurface3 *,
    LPVOID
) {
    ++g_fakeDirectDrawSurface3UnlockCalls;
    return DD_OK;
}

HRESULT __stdcall FakeDirectDrawSurface3_Restore(
    IDirectDrawSurface3 *
) {
    ++g_fakeDirectDrawSurface3RestoreCalls;
    return DD_OK;
}

void InstallFakeDirectDrawSurface3(
    FakeDirectDrawSurface3Object &surface
) {
    std::memset(g_fakeDirectDrawSurface3VTable, 0, sizeof(g_fakeDirectDrawSurface3VTable));
    g_fakeDirectDrawSurface3VTable[5] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Blt);
    g_fakeDirectDrawSurface3VTable[25] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Lock);
    g_fakeDirectDrawSurface3VTable[27] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Restore);
    g_fakeDirectDrawSurface3VTable[32] =
        reinterpret_cast<void *>(FakeDirectDrawSurface3_Unlock);
    g_fakeDirectDrawSurface3BltCalls = 0;
    g_fakeDirectDrawSurface3LockCalls = 0;
    g_fakeDirectDrawSurface3UnlockCalls = 0;
    g_fakeDirectDrawSurface3RestoreCalls = 0;
    g_fakeDirectDrawSurface3LastBltDstRect = RECT();
    g_fakeDirectDrawSurface3LastBltSrcRect = RECT();
    g_fakeDirectDrawSurface3LastBltSource = 0;
    g_fakeDirectDrawSurface3LastBltFlags = 0;
    g_fakeDirectDrawSurface3LastBltFx = 0;
    surface.vtable = g_fakeDirectDrawSurface3VTable;
}

void __fastcall CaptureBltSourceToPrimary(
    zVidImagePartial *image,
    int dstX,
    int dstY,
    int clipFlags,
    zVidRect32 *srcRect
) {
    ++g_captureBltSourceToPrimaryCalls;
    g_captureBltSourceToPrimaryImage = image;
    g_captureBltSourceToPrimaryDstX = dstX;
    g_captureBltSourceToPrimaryDstY = dstY;
    g_captureBltSourceToPrimaryClipFlags = clipFlags;
    g_captureBltSourceToPrimarySrcRect = srcRect;
}

unsigned short ExpectedFramebufferBlend565(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
) {
    const int greenDelta = (((srcPixel & 0x07e0) - (dstPixel & 0x07e0)) * alpha) >> 8;
    const int redDelta = (((srcPixel & 0xf800) - (dstPixel & 0xf800)) * alpha) >> 8;
    int blended = (short)(dstPixel) + (redDelta & 0xfffff800);
    const int blueDelta = (((srcPixel & 0x001f) - (blended & 0x001f)) * alpha) >> 8;
    blended += (greenDelta & 0xffffffe0) + blueDelta;
    return (unsigned short)(blended);
}
} // namespace

extern "C" int zvideo_buff_blt_source_to_primary_clipped_smoke(void) {
    const zVideo_SurfaceStatePartial savedPrimarySurface =
        g_zVideo_PrimarySurfaceState;
    FakeDirectDrawSurface3Object primarySurface = {};
    FakeDirectDrawSurface3Object sourceSurface = {};
    InstallFakeDirectDrawSurface3(primarySurface);
    sourceSurface.vtable = g_fakeDirectDrawSurface3VTable;

    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_PrimarySurfaceState.width = 100;
    g_zVideo_PrimarySurfaceState.height = 80;
    g_zVideo_PrimarySurfaceState.locked = 1;
    g_zVideo_PrimarySurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);

    zVidImagePartial image = {};
    image.width = 20;
    image.height = 10;
    image.formatFlagsPacked = 2;
    image.surface = reinterpret_cast<IDirectDrawSurface3 *>(&sourceSurface);

    zVidRect32 srcRect = {2, 3, 12, 9};
    zVideo_buff::BltSourceToPrimaryClipped(&image, -1, 4, 0, &srcRect);

    const int ok =
        g_fakeDirectDrawSurface3UnlockCalls == 1 &&
        g_fakeDirectDrawSurface3BltCalls == 1 &&
        g_fakeDirectDrawSurface3LockCalls == 1 &&
        g_fakeDirectDrawSurface3RestoreCalls == 0 &&
        g_zVideo_PrimarySurfaceState.locked == 1 &&
        g_fakeDirectDrawSurface3LastBltSource == image.surface &&
        g_fakeDirectDrawSurface3LastBltFx == 0 &&
        g_fakeDirectDrawSurface3LastBltFlags ==
            (DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE | DDBLT_KEYSRC) &&
        g_fakeDirectDrawSurface3LastBltDstRect.left == 0 &&
        g_fakeDirectDrawSurface3LastBltDstRect.top == 4 &&
        g_fakeDirectDrawSurface3LastBltDstRect.right == 9 &&
        g_fakeDirectDrawSurface3LastBltDstRect.bottom == 10 &&
        g_fakeDirectDrawSurface3LastBltSrcRect.left == 3 &&
        g_fakeDirectDrawSurface3LastBltSrcRect.top == 3 &&
        g_fakeDirectDrawSurface3LastBltSrcRect.right == 12 &&
        g_fakeDirectDrawSurface3LastBltSrcRect.bottom == 9;

    g_zVideo_PrimarySurfaceState = savedPrimarySurface;
    return ok != 0 ? 0 : 1;
}

extern "C" int zvid_image_blit_to_active_target_smoke(void) {
    const zVideo_SurfaceStatePartial savedPrimarySurface =
        g_zVideo_PrimarySurfaceState;
    void *const savedFrameBuffer = zRndr::g_frameBuffer;
    zVideo_BltSourceToPrimaryProc const savedDispatch =
        g_zVideo_pfnBltSourceToPrimary;

    FakeDirectDrawSurface3Object primarySurface = {};
    FakeDirectDrawSurface3Object sourceSurface = {};
    InstallFakeDirectDrawSurface3(primarySurface);
    sourceSurface.vtable = g_fakeDirectDrawSurface3VTable;

    unsigned short primaryPixels[16] = {};
    g_zVideo_PrimarySurfaceState = zVideo_SurfaceStatePartial();
    g_zVideo_PrimarySurfaceState.width = 16;
    g_zVideo_PrimarySurfaceState.height = 16;
    g_zVideo_PrimarySurfaceState.pixels = primaryPixels;
    g_zVideo_PrimarySurfaceState.surf =
        reinterpret_cast<IDirectDrawSurface3 *>(&primarySurface);
    zRndr::g_frameBuffer = primaryPixels;

    zVidImagePartial image = {};
    image.width = 4;
    image.height = 4;
    image.formatFlagsPacked = 2;
    image.surface = reinterpret_cast<IDirectDrawSurface3 *>(&sourceSurface);
    zVidRect32 srcRect = {1, 1, 4, 4};

    g_captureBltSourceToPrimaryCalls = 0;
    g_zVideo_pfnBltSourceToPrimary = CaptureBltSourceToPrimary;
    zVid_Image::BlitToActiveTarget(&image, 2, 3, 0x10001, &srcRect);

    int status = 0;
    if (g_captureBltSourceToPrimaryCalls != 0 ||
        g_fakeDirectDrawSurface3BltCalls != 1 ||
        g_fakeDirectDrawSurface3LastBltSource != image.surface ||
        g_fakeDirectDrawSurface3LastBltDstRect.left != 2 ||
        g_fakeDirectDrawSurface3LastBltDstRect.top != 3 ||
        g_fakeDirectDrawSurface3LastBltDstRect.right != 5 ||
        g_fakeDirectDrawSurface3LastBltDstRect.bottom != 6 ||
        g_fakeDirectDrawSurface3LastBltSrcRect.left != 1 ||
        g_fakeDirectDrawSurface3LastBltSrcRect.top != 1 ||
        g_fakeDirectDrawSurface3LastBltSrcRect.right != 4 ||
        g_fakeDirectDrawSurface3LastBltSrcRect.bottom != 4 ||
        g_fakeDirectDrawSurface3LastBltFlags !=
            (DDBLT_WAIT | DDBLT_KEYSRCOVERRIDE | DDBLT_KEYSRC)) {
        status = 1;
    }

    if (status == 0) {
        g_captureBltSourceToPrimaryCalls = 0;
        g_captureBltSourceToPrimaryImage = 0;
        g_captureBltSourceToPrimaryDstX = 0;
        g_captureBltSourceToPrimaryDstY = 0;
        g_captureBltSourceToPrimaryClipFlags = 0;
        g_captureBltSourceToPrimarySrcRect = 0;
        g_fakeDirectDrawSurface3BltCalls = 0;
        zRndr::g_frameBuffer = reinterpret_cast<void *>(0x12345678);

        zVid_Image::BlitToActiveTarget(&image, -4, 7, 0x10002, &srcRect);

        if (g_fakeDirectDrawSurface3BltCalls != 0 ||
            g_captureBltSourceToPrimaryCalls != 1 ||
            g_captureBltSourceToPrimaryImage != &image ||
            g_captureBltSourceToPrimaryDstX != -4 ||
            g_captureBltSourceToPrimaryDstY != 7 ||
            g_captureBltSourceToPrimaryClipFlags != 0x10002 ||
            g_captureBltSourceToPrimarySrcRect != &srcRect) {
            status = 2;
        }
    }

    g_zVideo_PrimarySurfaceState = savedPrimarySurface;
    zRndr::g_frameBuffer = savedFrameBuffer;
    g_zVideo_pfnBltSourceToPrimary = savedDispatch;
    return status;
}

extern "C" int zvid_image_blit_to_framebuffer_clipped_smoke(void) {
    void *const savedFrameBuffer = zRndr::g_frameBuffer;
    const int savedActiveRegionWidth = zRndr::g_activeRegionWidth;
    const int savedActiveRegionHeight = zRndr::g_activeRegionHeight;
    const int savedPitchBytes = zRndr::g_pitchBytes;
    const int savedPixelPackGreenBits = zRndr::g_pixelPackGreenBits;

    uint16_t frame[16];
    for (int i = 0; i < 16; ++i) {
        frame[i] = 0xaaaa;
    }

    uint16_t pixels16[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    zVidImagePartial image = {};
    image.width = 4;
    image.height = 3;
    image.pitchWords = 4;
    image.pixels = pixels16;

    zRndr::g_frameBuffer = frame;
    zRndr::g_activeRegionWidth = 4;
    zRndr::g_activeRegionHeight = 4;
    zRndr::g_pitchBytes = 8;
    zRndr::g_pixelPackGreenBits = 6;

    zVid_Image::BlitToFramebufferClipped(&image, -1, 1, 0, 0);
    int status = 0;
    if (frame[4] != 2 || frame[5] != 3 || frame[6] != 4 || frame[7] != 0xaaaa ||
        frame[8] != 6 || frame[9] != 7 || frame[10] != 8 || frame[12] != 10 ||
        frame[13] != 11 || frame[14] != 12) {
        status = 1;
    }

    uint16_t maskedPixels[2] = {0x1234, 0x00ff};
    if (status == 0) {
        frame[0] = 0x1111;
        frame[1] = 0x2222;
        image.width = 2;
        image.height = 1;
        image.pitchWords = 2;
        image.pixels = maskedPixels;
        image.alphaMap = 0;
        image.palette = 0;
        image.formatFlagsPacked = 2;
        zVid_Image::BlitToFramebufferClipped(&image, 0, 0, 0x00ff, 0);
        if (frame[0] != 0x1234 || frame[1] != 0x2222) {
            status = 2;
        }
    }

    uint8_t alphaMap[2] = {0x80, 3};
    uint16_t alphaPixels[2] = {0xffff, 0xf800};
    if (status == 0) {
        frame[0] = 0x001f;
        frame[1] = 0x3333;
        image.pixels = alphaPixels;
        image.alphaMap = (char *)(alphaMap);
        image.formatFlagsPacked = 0;
        zVid_Image::BlitToFramebufferClipped(&image, 0, 0, 0, 0);
        if (frame[0] != ExpectedFramebufferBlend565(0x001f, 0xffff, 0x80) ||
            frame[1] != 0x3333) {
            status = 3;
        }
    }

    uint8_t palPixels[3] = {1, 2, 3};
    uint16_t palette[4] = {0, 0x0101, 0x0202, 0x0303};
    if (status == 0) {
        frame[0] = 0xaaaa;
        frame[1] = 0xbbbb;
        frame[2] = 0xcccc;
        image.width = 3;
        image.height = 1;
        image.pitchWords = 3;
        image.pixels = palPixels;
        image.alphaMap = 0;
        image.palette = palette;
        image.formatFlagsPacked = 2;
        zVid_Image::BlitToFramebufferClipped(&image, 0, 0, 2, 0);
        if (frame[0] != 0x0101 || frame[1] != 0xbbbb || frame[2] != 0x0303) {
            status = 4;
        }
    }

    zRndr::g_frameBuffer = savedFrameBuffer;
    zRndr::g_activeRegionWidth = savedActiveRegionWidth;
    zRndr::g_activeRegionHeight = savedActiveRegionHeight;
    zRndr::g_pitchBytes = savedPitchBytes;
    zRndr::g_pixelPackGreenBits = savedPixelPackGreenBits;
    return status;
}

extern "C" int zvideo_image_file_read_helpers_smoke(void) {
    unsigned char header[0x10] = {};
    header[0] = 1;
    *reinterpret_cast<std::int16_t *>(&header[4]) = 2;
    *reinterpret_cast<std::int16_t *>(&header[6]) = 1;
    header[8] = 0x12;
    *reinterpret_cast<std::int16_t *>(&header[0x0c]) = 0x3456;
    *reinterpret_cast<std::int16_t *>(&header[0x0e]) = 0;
    std::uint16_t pixels[2] = {0x7fff, 0x001f};

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::fwrite(header, 1, sizeof(header), file);
    std::fwrite(pixels, 1, sizeof(pixels), file);
    std::rewind(file);

    zVidImagePartial directReadImage = {};
    const bool directHeaderRead =
        zVid_Image::ReadHeader(nullptr, &directReadImage) == -1 &&
        zVid_Image::ReadHeader(file, nullptr) == -1 &&
        zVid_Image::ReadHeader(file, &directReadImage) == 0 &&
        directReadImage.width == 2 && directReadImage.height == 1 &&
        directReadImage.pixelCount == 2 && directReadImage.pitchWords == 2 &&
        directReadImage.formatFlagsPacked == 1 && directReadImage.headerFlagsByte == 0x12 &&
        directReadImage.textureAddressFlagsPacked == 0x3456 &&
        directReadImage.paletteMetaPacked == 0;

    std::rewind(file);
    g_zVideo_PixelPack.gBits = 5;
    zVidImagePartial *image = zVid_Image::ReadFromFile(file);
    std::fclose(file);

    if (image == nullptr) {
        return 2;
    }

    std::uint16_t *readPixels = static_cast<std::uint16_t *>(image->pixels);
    const bool ok = directHeaderRead && image->width == 2 && image->height == 1 &&
                    image->pixelCount == 2 && image->pitchWords == 2 &&
                    image->headerFlagsByte == 0x12 &&
                    image->textureAddressFlagsPacked == 0x3456 &&
                    image->paletteMetaPacked == 0 && (image->formatFlagsPacked & 0x21) == 0x21 &&
                    readPixels[0] == 0x3fff && readPixels[1] == 0x001f;

    zVid_Image::Destroy(image);
    return ok ? 0 : 3;
}

extern "C" int zvideo_image_read_data_smoke(void) {
    unsigned char sourcePixels[4] = {0x10, 0x20, 0x30, 0x40};
    unsigned char sourceAlpha[4] = {1, 2, 3, 4};
    unsigned char sourcePalette[2] = {0xa0, 0xb0};

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::fwrite(sourcePixels, 1, sizeof(sourcePixels), file);
    std::fwrite(sourceAlpha, 1, sizeof(sourceAlpha), file);
    std::fwrite(sourcePalette, 1, sizeof(sourcePalette), file);
    std::rewind(file);

    unsigned char pixels[4] = {};
    zVidImagePartial image = {};
    image.pixelCount = 4;
    image.width = 2;
    image.height = 2;
    image.formatFlagsPacked = 0x08;
    image.paletteMetaPacked = 2;
    image.pixels = pixels;

    const int readResult = zVid_Image::ReadData(file, &image, 0);
    std::fclose(file);

    const bool dataOk =
        readResult == 0 && image.alphaMap != nullptr && image.palette != nullptr &&
        std::memcmp(pixels, sourcePixels, sizeof(sourcePixels)) == 0 &&
        std::memcmp(image.alphaMap, sourceAlpha, sizeof(sourceAlpha)) == 0 &&
        std::memcmp(image.palette, sourcePalette, sizeof(sourcePalette)) == 0 &&
        (image.formatFlagsPacked & 0xc8) == 0xc8;
    std::free(image.alphaMap);
    std::free(image.palette);
    if (!dataOk) {
        return 2;
    }

    unsigned char largerHintPixel = 0xaa;
    zVidImagePartial largerHintImage = {};
    largerHintImage.pixelCount = 1;
    largerHintImage.formatFlagsPacked = 0;
    largerHintImage.pixels = &largerHintPixel;

    file = std::tmpfile();
    if (file == nullptr) {
        return 3;
    }
    const bool largerHintOk =
        zVid_Image::ReadData(file, &largerHintImage, 2) == 0 && largerHintPixel == 0xaa;
    std::fclose(file);
    if (!largerHintOk) {
        return 4;
    }

    unsigned char shortPixels[4] = {};
    zVidImagePartial shortReadImage = {};
    shortReadImage.pixelCount = 4;
    shortReadImage.formatFlagsPacked = 0;
    shortReadImage.pixels = shortPixels;

    file = std::tmpfile();
    if (file == nullptr) {
        return 5;
    }
    std::fputc(0x7f, file);
    std::rewind(file);
    const bool shortReadOk = zVid_Image::ReadData(file, &shortReadImage, 0) == -1;
    std::fclose(file);

    return shortReadOk ? 0 : 6;
}
