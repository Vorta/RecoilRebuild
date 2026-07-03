#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRndr/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" int g_zImage_NextFontSlotIndex;

namespace {
int g_textureDestroyCount = 0;
int g_uploadSurfaceReleaseCount = 0;
int g_textureCreateCount = 0;
int g_textureFinalizeUploadCount = 0;
zVideo_TextureRecordPartial g_createdTextureRecord = {};
zVideo_TextureRecordPartial g_existingTextureRecord = {};
zVidImagePartial *g_lastCreatedImage = nullptr;
const char *g_lastCreatedTextureName = nullptr;
zVidImagePartial *g_lastFinalizedImage = nullptr;
int g_fontBlitCount = 0;
void *g_fontBlitImage[8] = {};
std::int32_t g_fontBlitX[8] = {};
std::int32_t g_fontBlitY[8] = {};
std::int32_t g_fontBlitFlags[8] = {};
zVidRect32 g_fontBlitRect[8] = {};

void __fastcall TextureRecordDestroyStub(zVideo_TextureRecordPartial *) {
    ++g_textureDestroyCount;
}

void TextureRecordReleaseAllUploadSurfacesStub() {
    ++g_uploadSurfaceReleaseCount;
}

zVideo_TextureRecordPartial *__fastcall CreateTextureRecordStub(const char *textureName,
                                                                     zVidImagePartial *image,
                                                                     int useAlpha,
                                                                     int clampU,
                                                                     int clampV) {
    ++g_textureCreateCount;
    g_lastCreatedTextureName = textureName;
    g_lastCreatedImage = image;
    g_createdTextureRecord.m_alphaMode = useAlpha;
    g_createdTextureRecord.m_uWrapMode = static_cast<D3DTEXTUREADDRESS>(clampU);
    g_createdTextureRecord.m_vWrapMode = static_cast<D3DTEXTUREADDRESS>(clampV);
    return &g_createdTextureRecord;
}

void __fastcall TextureRecordFinalizeUploadStub(zVideo_TextureRecordPartial *,
                                                     void *,
                                                     zVidImagePartial *image) {
    ++g_textureFinalizeUploadCount;
    g_lastFinalizedImage = image;
}

void __fastcall FontBlitCapture(zVidImagePartial *image, std::int32_t dstX,
                                     std::int32_t dstY, std::int32_t clipFlags,
                                     zVidRect32 *srcRect) {
    const int index = g_fontBlitCount;
    if (index < 8) {
        g_fontBlitImage[index] = image;
        g_fontBlitX[index] = dstX;
        g_fontBlitY[index] = dstY;
        g_fontBlitFlags[index] = clipFlags;
        g_fontBlitRect[index] = *static_cast<zVidRect32 *>(srcRect);
    }

    ++g_fontBlitCount;
}

std::int32_t __fastcall QueryTextureMemoryBytesStub(std::int32_t, std::int32_t *totalBytes,
                                                         std::int32_t *freeBytes) {
    *totalBytes = 6 << 20;
    *freeBytes = 4 << 20;
    return 1;
}

zVidImagePartial g_fallbackImage = {};

zVidImagePartial *__fastcall FallbackImageStub(char *) {
    return &g_fallbackImage;
}

bool WriteMinimalTexturePack(const char *path) {
    zVidTexturePackHeader packHeader{};
    packHeader.fileFormat = 1;
    packHeader.recordCount = 0;

    std::FILE *out = std::fopen(path, "wb");
    if (out == nullptr) {
        return false;
    }

    const bool ok = std::fwrite(&packHeader, sizeof(packHeader), 1, out) == 1;
    std::fclose(out);
    return ok;
}

void ResetDefaultTexturePackState() {
    if (g_zVid_TexturePacks != nullptr) {
        for (int i = 0; i < g_zVid_TexturePackCount; ++i) {
            if (g_zVid_TexturePacks[i].fileHandle != nullptr) {
                std::fclose(g_zVid_TexturePacks[i].fileHandle);
                g_zVid_TexturePacks[i].fileHandle = nullptr;
            }
            std::free(g_zVid_TexturePacks[i].records);
            g_zVid_TexturePacks[i].records = nullptr;
        }
        std::free(g_zVid_TexturePacks);
    }

    g_zVid_TexturePacks = nullptr;
    g_zVid_TexturePackCount = 0;
}
} // namespace

extern "C" int zvid_pack_color_rgb_smoke(void) {
    const int savedRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int savedGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int savedRShift = g_zVideo_PixelPack.packedBase;
    const int savedGShift = g_zVideo_PixelPack.sumMinus8;
    const int savedBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;

    g_zVideo_PixelPack.rMaskShifted = 0xf8;
    g_zVideo_PixelPack.gMaskShifted = 0xfc;
    g_zVideo_PixelPack.packedBase = 8;
    g_zVideo_PixelPack.sumMinus8 = 3;
    g_zVideo_PixelPack.bShiftTo8 = 3;

    const unsigned int packedWhite = zVid_PackColorRGB(0xff, 0xff, 0xff);
    const unsigned int packedMaskedWhite = zVid_PackColorRGB(0xf8, 0xfc, 0xf8);
    const unsigned int packedSample = zVid_PackColorRGB(0x20, 0x60, 0x40);
    const unsigned int expectedSample = ((0xfc & 0x60) << 3) |
                                        ((0xf8 & 0x20) << 8) |
                                        (0x40 >> 3);

    g_zVideo_PixelPack.rMaskShifted = savedRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = savedGMaskShifted;
    g_zVideo_PixelPack.packedBase = savedRShift;
    g_zVideo_PixelPack.sumMinus8 = savedGShift;
    g_zVideo_PixelPack.bShiftTo8 = savedBShiftTo8;

    return packedWhite == 0xffff && packedMaskedWhite == 0xffff &&
                   packedSample == expectedSample
               ? 0
               : 1;
}

extern "C" int zvid_pack_color_rgb_floats_smoke(void) {
    const int savedRMaskShifted = g_zVideo_PixelPack.rMaskShifted;
    const int savedGMaskShifted = g_zVideo_PixelPack.gMaskShifted;
    const int savedRShift = g_zVideo_PixelPack.packedBase;
    const int savedGShift = g_zVideo_PixelPack.sumMinus8;
    const int savedBShiftTo8 = g_zVideo_PixelPack.bShiftTo8;

    g_zVideo_PixelPack.rMaskShifted = 0xf8;
    g_zVideo_PixelPack.gMaskShifted = 0xfc;
    g_zVideo_PixelPack.packedBase = 8;
    g_zVideo_PixelPack.sumMinus8 = 3;
    g_zVideo_PixelPack.bShiftTo8 = 3;

    zVideo_ColorRgbFloat color = {255.0f, 127.6f, 32.4f};
    const std::uint16_t packed = zVid_PackColorRgbFloats(&color);

    g_zVideo_PixelPack.rMaskShifted = savedRMaskShifted;
    g_zVideo_PixelPack.gMaskShifted = savedGMaskShifted;
    g_zVideo_PixelPack.packedBase = savedRShift;
    g_zVideo_PixelPack.sumMinus8 = savedGShift;
    g_zVideo_PixelPack.bShiftTo8 = savedBShiftTo8;

    return packed == 0xfc04 ? 0 : 1;
}

extern "C" int zvideo_palette_remap_no_recipes_smoke(void) {
    std::uint16_t palette[2] = {1, 2};
    g_zVid_PaletteRemapRecipeCount = 0;
    g_zVid_PaletteRemapRecipes = nullptr;
    return zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(palette, 2) == palette ? 0 : 1;
}

extern "C" int zvideo_palette_remap_recipe_variants_smoke(void) {
    g_zVideo_PixelPack.rMaskShifted = 0xf8;
    g_zVideo_PixelPack.gMaskShifted = 0xfc;
    g_zVideo_PixelPack.packedBase = 8;
    g_zVideo_PixelPack.sumMinus8 = 3;
    g_zVideo_PixelPack.bShiftTo8 = 3;
    g_zVideo_PixelPack.rBits = 5;
    g_zVideo_PixelPack.gBits = 6;
    g_zVideo_PixelPack.bBits = 5;

    zVidPaletteRemapRecipe recipe = {};
    std::uint16_t source[2] = {0x0000, 0xffff};
    std::uint16_t directDest[2] = {0x1111, 0x2222};
    zVid_PaletteRemap::ApplyRecipeToPaletteVariant(&recipe, source, 2, 31, directDest);
    const bool directOk = directDest[0] == 0x0000 && directDest[1] == 0xffff;

    std::uint16_t *palette = static_cast<std::uint16_t *>(std::malloc(0x200));
    if (palette == nullptr) {
        return 1;
    }
    std::memset(palette, 0, 0x200);
    palette[0] = 0x0000;
    palette[1] = 0xffff;

    g_zVid_PaletteRemapRecipeCount = 1;
    g_zVid_PaletteRemapRecipes = &recipe;
    std::uint16_t *expanded = zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(palette, 2);
    if (expanded == nullptr) {
        g_zVid_PaletteRemapRecipeCount = 0;
        g_zVid_PaletteRemapRecipes = nullptr;
        return 2;
    }

    const int firstVariant = 0x200 / sizeof(std::uint16_t);
    const int lastVariant = firstVariant + 31 * (0x200 / sizeof(std::uint16_t));
    const bool buildOk = expanded[0] == 0x0000 && expanded[1] == 0xffff &&
                         expanded[firstVariant] == 0x0000 &&
                         expanded[firstVariant + 1] == 0xffff &&
                         expanded[lastVariant] == 0x0000 &&
                         expanded[lastVariant + 1] == 0xffff;

    std::free(expanded);
    g_zVid_PaletteRemapRecipeCount = 0;
    g_zVid_PaletteRemapRecipes = nullptr;
    return directOk && buildOk ? 0 : 3;
}

extern "C" int zvideo_image_set_pixels_smoke(void) {
    zVidImagePartial image{};
    std::uint16_t pixels[2] = {0x1111, 0x2222};
    char alpha[2] = {1, 0};

    image.formatFlagsPacked = 0x20;
    const bool withAlpha = zVid_Image_SetPixels(&image, pixels, alpha) == 0 &&
                           image.pixels == pixels && image.alphaMap == alpha &&
                           (image.formatFlagsPacked & 0x22u) == 0x22u;

    image.formatFlagsPacked = 0x20;
    const bool withoutAlpha = zVid_Image_SetPixels(&image, pixels, nullptr) == 0 &&
                              image.pixels == pixels && image.alphaMap == nullptr &&
                              image.formatFlagsPacked == 0x20u;

    return withAlpha && withoutAlpha ? 0 : 1;
}

extern "C" int zvid_image_create_format_size_pixels_smoke(void) {
    zVidImagePartial *const image = zVid_Image::Create();
    if (image == nullptr) {
        return 1;
    }

    const bool createdZeroed =
        image->pixelCount == 0 && image->width == 0 && image->height == 0 &&
        image->formatFlagsPacked == 0 && image->pixels == nullptr && image->alphaMap == nullptr;

    std::uint16_t pixels[128] = {};
    char alpha[128] = {};
    const bool configured =
        zVid_Image::SetFormatCode(image, 1) == 0 &&
        zVid_Image::SetHeaderFlagsByte(image, 0x7a) == 0 &&
        zVid_Image_SetPixels(image, pixels, alpha) == 0;

    image->widthScale = 3.5f;
    image->uShiftFrom20 = 77;
    image->uMask = 88;
    image->vMaskFixed20 = 99;
    const bool sized =
        zVid_Image::SetSize(image, 16, 8) == 0 &&
        image->headerFlagsByte == 0x7a && image->formatFlagsPacked == 3 &&
        image->pixels == pixels && image->alphaMap == alpha && image->width == 16 &&
        image->height == 8 && image->pixelCount == 128 && image->pitchWords == 16 &&
        image->widthScale == 3.5f &&
        image->uShiftFrom20 == 77 && image->uMask == 88 &&
        image->vMaskFixed20 == 99;

    const bool unpackedPixelDataBytes = zVid_Image::QueryPixelDataBytes(image) == 256;
    image->paletteMetaPacked = 32;
    const bool palettedPixelDataBytes = zVid_Image::QueryPixelDataBytes(image) == 128;
    image->paletteMetaPacked = 0;

    zVid_Image::Destroy(image);
    return createdZeroed && configured && sized && unpackedPixelDataBytes && palettedPixelDataBytes
               ? 0
               : 2;
}

extern "C" int zvideo_capture_surface_to_image_smoke(void) {
    zVideo::BindRendererDispatch(0, 0);
    std::uint16_t pixels[6] = {1, 2, 0xaaaa, 3, 4, 0xbbbb};
    g_zVideo_DisplayModeSurfaceState = {};
    g_zVideo_DisplayModeSurfaceState.width = 2;
    g_zVideo_DisplayModeSurfaceState.height = 2;
    g_zVideo_DisplayModeSurfaceState.pitch = 6;
    g_zVideo_DisplayModeSurfaceState.pixels = pixels;

    zVidImagePartial *image = zVideo_buff_CaptureSurfaceToImage(2);
    if (image == nullptr) {
        return 1;
    }

    std::uint16_t *const captured = static_cast<std::uint16_t *>(image->pixels);
    const bool ok = image->width == 2 && image->height == 2 && image->pixelCount == 4 &&
                    (image->formatFlagsPacked & 0x20u) != 0 && captured != nullptr &&
                    captured[0] == 1 && captured[1] == 2 && captured[2] == 3 &&
                    captured[3] == 4;

    zVid_Image::ReleaseIfNotDefault(image);
    return ok ? 0 : 2;
}

extern "C" int zvideo_fx_set_surface_state_smoke(void) {
    unsigned short *const oldPixels = g_zVideo_FxSurfacePixels16;
    const int oldWidth = g_zVideo_FxSurfaceWidth;
    const int oldHeight = g_zVideo_FxSurfaceHeight;
    const int oldPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldPitchPixels = g_zVideo_FxSurfacePitchPixels16;

    unsigned short pixels[8] = {};
    zVideo::Fx_SetSurfaceState(pixels, 5, 7, 14);

    const bool ok = g_zVideo_FxSurfacePixels16 == pixels &&
                    g_zVideo_FxSurfaceWidth == 5 &&
                    g_zVideo_FxSurfaceHeight == 7 &&
                    g_zVideo_FxSurfacePitchBytes == 14 &&
                    g_zVideo_FxSurfacePitchPixels16 == 7;

    g_zVideo_FxSurfacePixels16 = oldPixels;
    g_zVideo_FxSurfaceWidth = oldWidth;
    g_zVideo_FxSurfaceHeight = oldHeight;
    g_zVideo_FxSurfacePitchBytes = oldPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldPitchPixels;
    return ok ? 0 : 1;
}

extern "C" int zvideo_image_alpha_clear_smoke(void) {
    std::uint16_t pixels[4] = {0x1111, 0x2222, 0x3333, 0x4444};
    char alpha[4] = {1, 0, 1, 0};

    zVidImagePartial image{};
    image.pixelCount = 4;
    image.formatFlagsPacked = 1;
    image.paletteMetaPacked = 0;
    image.pixels = pixels;
    image.alphaMap = alpha;

    if (zVid_Image::QueryBytesPerPixel(&image) != 2) {
        return 1;
    }

    zVid_Image::ClearZeroAlphaPixelsInPlace(&image);
    return pixels[0] == 0x1111 && pixels[1] == 0 && pixels[2] == 0x3333 &&
                   pixels[3] == 0
               ? 0
               : 2;
}

extern "C" int zvideo_texture_pack_load_image_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char packPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "ztp", 0, packPath) == 0) {
        return 1;
    }

    zVidTexturePackHeader packHeader{};
    packHeader.fileFormat = 1;
    packHeader.recordCount = 1;
    zVidTexturePackRecord record{};
    std::strcpy(record.name, "font.tex");
    record.fileOffset = sizeof(packHeader) + sizeof(record);
    record.paletteIndex = -1;

    unsigned char imageHeader[0x10] = {};
    imageHeader[0] = 1;
    *reinterpret_cast<std::int16_t *>(&imageHeader[4]) = 1;
    *reinterpret_cast<std::int16_t *>(&imageHeader[6]) = 1;
    std::uint16_t pixel = 0x1234;

    FILE *out = std::fopen(packPath, "wb");
    if (out == nullptr) {
        DeleteFileA(packPath);
        return 2;
    }
    std::fwrite(&packHeader, sizeof(packHeader), 1, out);
    std::fwrite(&record, sizeof(record), 1, out);
    std::fwrite(imageHeader, 1, sizeof(imageHeader), out);
    std::fwrite(&pixel, sizeof(pixel), 1, out);
    std::fclose(out);

    zVidTexturePackEntry entry{};
    std::strcpy(entry.filePath, packPath);
    g_zVid_TexturePackLoadState = 1;
    g_zVideo_PixelPack.rBits = 0;
    if (zVid_TexturePackEntry_LoadFromFile(&entry) == nullptr) {
        DeleteFileA(packPath);
        return 3;
    }

    g_zVid_TexturePacks = &entry;
    g_zVid_TexturePackCount = 1;
    zVidImagePartial *image = zVid_TexturePack_LoadImageByName("font.tex");
    const bool ok = image != nullptr && image->width == 1 && image->height == 1 &&
                    image->pixelCount == 1 &&
                    static_cast<std::uint16_t *>(image->pixels)[0] == pixel;

    zVid_Image::Destroy(image);
    std::fclose(entry.fileHandle);
    std::free(entry.records);
    g_zVid_TexturePacks = nullptr;
    g_zVid_TexturePackCount = 0;
    g_zVid_TexturePackLoadState = 0;
    DeleteFileA(packPath);
    return ok ? 0 : 4;
}

extern "C" int zimage_font_glyph_scan_smoke(void) {
    constexpr std::int32_t kWidth = 192;
    std::uint16_t pixels[kWidth] = {};
    for (std::int32_t x = 0; x < kWidth; ++x) {
        pixels[x] = (x & 1) == 0 ? 0x1234 : 0x5678;
    }

    g_zImage_FontTransparentColor = 0x1234;

    zVidImagePartial image{};
    image.width = kWidth;
    image.height = 1;
    image.pixels = pixels;

    if (zImage_Font::IsImageColumnTransparent(&image, 0) != 1 ||
        zImage_Font::IsImageColumnTransparent(&image, 1) != 0 ||
        zImage_Font::IsImageColumnTransparent(&image, kWidth) != 0) {
        return 1;
    }

    zImage_Font font{};
    font.image = &image;
    const std::int32_t count = font.BuildGlyphRects();

    return count == 95 && font.spaceWidth == 1 && font.glyphRects[0].left == 1 &&
                   font.glyphRects[0].right == 3 && font.glyphRects[0].top == 0 &&
                   font.glyphRects[0].bottom == 0
               ? 0
               : 2;
}

extern "C" int zimage_font_measure_string_smoke(void) {
    zVidImagePartial image{};
    image.height = 7;

    zImage_Font font{};
    font.image = &image;
    font.spaceWidth = 3;
    font.glyphRects['A' - 0x21].left = 2;
    font.glyphRects['A' - 0x21].right = 7;
    font.glyphRects['B' - 0x21].left = 9;
    font.glyphRects['B' - 0x21].right = 11;
    font.glyphRects[0].left = 0;
    font.glyphRects[0].right = 4;

    g_zImage_FontTable[0] = nullptr;
    g_zImage_FontTable[2] = &font;

    std::int32_t width = -1;
    std::int32_t lineAdvance = -1;
    zImage_Font::MeasureString("A B\n!", 2, &width, &lineAdvance);

    const bool measured = width == 10 && lineAdvance == 14;

    g_zImage_FontTable[0] = &font;
    g_zImage_FontTable[2] = nullptr;
    zImage_Font *const fallback = zImage_Font::GetByIndexOrDefault(2);
    width = -1;
    lineAdvance = -1;
    zImage_Font::MeasureString("B", 2, &width, &lineAdvance);
    const bool fallbackMeasured = fallback == &font && width == 2 && lineAdvance == 7;

    width = 55;
    lineAdvance = 66;
    g_zImage_FontTable[0] = nullptr;
    g_zImage_FontTable[2] = nullptr;
    zImage_Font::MeasureString("A", 2, &width, &lineAdvance);

    g_zImage_FontTable[0] = nullptr;
    return measured && fallbackMeasured && width == 55 && lineAdvance == 66 ? 0 : 1;
}

extern "C" int zimage_font_blit_string_smoke(void) {
    zVideo_BltSourceToPrimaryProc const oldBlit = g_zVideo_pfnBltSourceToPrimary;
    zImage_Font *const oldFont0 = g_zImage_FontTable[0];
    zImage_Font *const oldFont2 = g_zImage_FontTable[2];
    const int oldActiveHeight = zRndr::g_activeRegionHeight;

    zVidImagePartial image{};
    image.height = 7;

    zImage_Font font{};
    font.image = &image;
    font.spaceWidth = 3;
    font.glyphRects[0].left = 20;
    font.glyphRects[0].top = 1;
    font.glyphRects[0].right = 24;
    font.glyphRects[0].bottom = 6;
    font.glyphRects['A' - 0x21].left = 2;
    font.glyphRects['A' - 0x21].top = 3;
    font.glyphRects['A' - 0x21].right = 7;
    font.glyphRects['A' - 0x21].bottom = 9;
    font.glyphRects['B' - 0x21].left = 9;
    font.glyphRects['B' - 0x21].top = 4;
    font.glyphRects['B' - 0x21].right = 11;
    font.glyphRects['B' - 0x21].bottom = 12;

    g_zVideo_pfnBltSourceToPrimary = FontBlitCapture;
    g_zImage_FontTable[0] = nullptr;
    g_zImage_FontTable[2] = &font;
    zRndr::g_activeRegionHeight = 100;

    g_fontBlitCount = 0;
    std::memset(g_fontBlitImage, 0, sizeof(g_fontBlitImage));
    std::memset(g_fontBlitX, 0, sizeof(g_fontBlitX));
    std::memset(g_fontBlitY, 0, sizeof(g_fontBlitY));
    std::memset(g_fontBlitFlags, 0xff, sizeof(g_fontBlitFlags));
    std::memset(g_fontBlitRect, 0, sizeof(g_fontBlitRect));

    const char text[] = {'A', ' ', 'B', '\r', '\n', '!', (char)(0x80), '\0'};
    zImage_Font::BlitStringToActiveTarget(text, 10, 20, 2);

    const bool drawSequence =
        g_fontBlitCount == 4 &&
        g_fontBlitImage[0] == &image && g_fontBlitX[0] == 10 &&
        g_fontBlitY[0] == 20 && g_fontBlitFlags[0] == 0 &&
        g_fontBlitRect[0].left == 2 && g_fontBlitRect[0].top == 3 &&
        g_fontBlitRect[0].right == 7 && g_fontBlitRect[0].bottom == 9 &&
        g_fontBlitImage[1] == &image && g_fontBlitX[1] == 18 &&
        g_fontBlitY[1] == 20 && g_fontBlitFlags[1] == 0 &&
        g_fontBlitRect[1].left == 9 && g_fontBlitRect[1].top == 4 &&
        g_fontBlitRect[1].right == 11 && g_fontBlitRect[1].bottom == 12 &&
        g_fontBlitImage[2] == &image && g_fontBlitX[2] == 10 &&
        g_fontBlitY[2] == 27 && g_fontBlitFlags[2] == 0 &&
        g_fontBlitRect[2].left == 20 && g_fontBlitRect[2].top == 1 &&
        g_fontBlitRect[2].right == 24 && g_fontBlitRect[2].bottom == 6 &&
        g_fontBlitImage[3] == &image && g_fontBlitX[3] == 14 &&
        g_fontBlitY[3] == 27 && g_fontBlitFlags[3] == 0 &&
        g_fontBlitRect[3].left == 20 && g_fontBlitRect[3].top == 1 &&
        g_fontBlitRect[3].right == 24 && g_fontBlitRect[3].bottom == 6;

    g_zImage_FontTable[0] = &font;
    g_zImage_FontTable[2] = nullptr;
    g_fontBlitCount = 0;
    zImage_Font::BlitStringToActiveTarget("B", 5, 6, 2);
    const bool fallbackDraw =
        g_fontBlitCount == 1 && g_fontBlitX[0] == 5 && g_fontBlitY[0] == 6 &&
        g_fontBlitRect[0].left == 9 && g_fontBlitRect[0].right == 11;

    zRndr::g_activeRegionHeight = 13;
    g_fontBlitCount = 0;
    zImage_Font::BlitStringToActiveTarget("A", 5, 6, 0);
    const bool clippedByHeight = g_fontBlitCount == 0;

    g_zImage_FontTable[0] = nullptr;
    g_fontBlitCount = 0;
    zRndr::g_activeRegionHeight = 100;
    zImage_Font::BlitStringToActiveTarget("A", 5, 6, 0);
    const bool noFontSkipped = g_fontBlitCount == 0;

    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    g_zImage_FontTable[0] = oldFont0;
    g_zImage_FontTable[2] = oldFont2;
    zRndr::g_activeRegionHeight = oldActiveHeight;

    return drawSequence && fallbackDraw && clippedByHeight && noFontSkipped ? 0 : 1;
}

extern "C" int zimage_fonts_load_missing_smoke(void) {
    g_zArchive_MountedList = nullptr;
    return zImage::FontsLoadFromPath("missing_fonts.zrd") == -1 ? 0 : 1;
}

extern "C" int zimage_texdir_find_or_create_missing_smoke(void) {
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_TexturePackLoadState = 0;
    g_zVid_TexturePackCount = 0;
    g_zVid_TexturePacks = nullptr;
    const bool ok = zImage::TexDir_FindOrCreateByPath("missing") == nullptr;
    std::free(g_zVid_TexturePacks);
    g_zVid_TexturePacks = nullptr;
    return ok ? 0 : 1;
}

extern "C" int zimage_texdir_build_mip_chain_smoke(void) {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 3;
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_TexturePackLoadState = 0;
    g_zVid_TexturePackCount = 0;
    g_zVid_TexturePacks = nullptr;

    zVidImagePartial baseImage{};
    zVidImagePartial mip2Image{};
    zVidImagePartial mip3Image{};
    baseImage.width = 64;
    mip2Image.width = 32;
    mip3Image.width = 16;

    zImage_TexDirEntryPartial &base = g_zImage_TexDirEntries[0];
    zImage_TexDirEntryPartial &mip2 = g_zImage_TexDirEntries[1];
    zImage_TexDirEntryPartial &mip3 = g_zImage_TexDirEntries[2];
    std::strcpy(base.baseName, "stone_1");
    std::strcpy(mip2.baseName, "stone_2");
    std::strcpy(mip3.baseName, "stone_3");
    base.loadState = 1;
    mip2.loadState = 1;
    mip3.loadState = 1;
    base.image = &baseImage;
    mip2.image = &mip2Image;
    mip3.image = &mip3Image;

    base.BuildMipChain();
    const bool chainOk = base.nextVariant == &mip2 && mip2.nextVariant == &mip3 &&
                         mip3.nextVariant == nullptr && mip2Image.widthScale == 2.0f &&
                         mip3Image.widthScale == 4.0f;

    zImage_TexDirEntryPartial noMip = {};
    std::strcpy(noMip.baseName, "panel");
    noMip.nextVariant = &base;
    noMip.BuildMipChain();
    const bool noMipOk = noMip.nextVariant == &base;

    std::free(g_zVid_TexturePacks);
    g_zVid_TexturePacks = nullptr;
    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));

    return chainOk && noMipOk ? 0 : 1;
}

extern "C" int zvid_texture_pack_ensure_builtin_smoke(void) {
    std::free(g_zVid_BuiltinTexturePacks);
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVid_TexturePackLoadState = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = QueryTextureMemoryBytesStub;
    g_zVideo_ActiveRendererPath = 1;
    std::int32_t textureMemoryOption = 1;
    g_zImage_TextureMemoryOption = &textureMemoryOption;

    zVid_TexturePack_EnsureBuiltinTexturePacksLoaded();
    const bool allocatedFallback =
        g_zVid_BuiltinTexturePackCount == 0 && g_zVid_BuiltinTexturePacks != nullptr &&
        std::strcmp(g_zVid_BuiltinTexturePacks[0].filePath, "texture.zbd") == 0;

    std::free(g_zVid_BuiltinTexturePacks);
    g_zVid_BuiltinTexturePacks = nullptr;

    zVidTexturePackEntry existingEntry = {};
    std::strcpy(existingEntry.filePath, "existing.zbd");
    existingEntry.fileHandle = reinterpret_cast<std::FILE *>(1);
    g_zVid_BuiltinTexturePacks = &existingEntry;
    g_zVid_BuiltinTexturePackCount = 1;
    zVid_TexturePack_EnsureBuiltinTexturePacksLoaded();
    const bool existingPreserved = g_zVid_BuiltinTexturePackCount == 1 &&
                                   existingEntry.fileHandle == reinterpret_cast<std::FILE *>(1);

    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = nullptr;
    g_zVideo_ActiveRendererPath = 0;
    g_zImage_TextureMemoryOption = nullptr;

    return allocatedFallback && existingPreserved ? 0 : 1;
}

extern "C" int zvid_texture_pack_ensure_default_smoke(void) {
    zVidTexturePackEntry *const oldTexturePacks = g_zVid_TexturePacks;
    const int oldTexturePackCount = g_zVid_TexturePackCount;
    const int oldTexturePackLoadState = g_zVid_TexturePackLoadState;

    std::remove("image.zbd");
    std::remove("rimage.zbd");

    zVidTexturePackEntry existingEntry{};
    std::strcpy(existingEntry.filePath, "existing.zbd");
    g_zVid_TexturePacks = &existingEntry;
    g_zVid_TexturePackCount = 1;
    g_zVid_TexturePackLoadState = 1;
    zVid_TexturePack_EnsureDefaultImagePackLoaded();
    const bool existingPreserved =
        g_zVid_TexturePackCount == 1 && g_zVid_TexturePacks == &existingEntry &&
        std::strcmp(existingEntry.filePath, "existing.zbd") == 0;

    g_zVid_TexturePacks = nullptr;
    g_zVid_TexturePackCount = 0;
    zVid_TexturePack_EnsureDefaultImagePackLoaded();
    const bool missingLeavesAllocatedSlot =
        g_zVid_TexturePackCount == 0 && g_zVid_TexturePacks != nullptr &&
        std::strcmp(g_zVid_TexturePacks[0].filePath, "rimage.zbd") == 0;
    ResetDefaultTexturePackState();

    bool fallbackLoaded = false;
    if (WriteMinimalTexturePack("rimage.zbd")) {
        zVid_TexturePack_EnsureDefaultImagePackLoaded();
        fallbackLoaded = g_zVid_TexturePackCount == 1 && g_zVid_TexturePacks != nullptr &&
                         std::strcmp(g_zVid_TexturePacks[0].filePath, "rimage.zbd") == 0 &&
                         g_zVid_TexturePacks[0].fileHandle != nullptr;
    }
    ResetDefaultTexturePackState();
    std::remove("rimage.zbd");

    g_zVid_TexturePacks = oldTexturePacks;
    g_zVid_TexturePackCount = oldTexturePackCount;
    g_zVid_TexturePackLoadState = oldTexturePackLoadState;

    return existingPreserved && missingLeavesAllocatedSlot && fallbackLoaded ? 0 : 1;
}

extern "C" int zimage_texdir_load_pending_entries_smoke(void) {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    std::free(g_zVid_BuiltinTexturePacks);
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVid_TexturePackLoadState = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = QueryTextureMemoryBytesStub;
    g_zVideo_ActiveRendererPath = 0;
    std::int32_t textureMemoryOption = 1;
    g_zImage_TextureMemoryOption = &textureMemoryOption;

    g_fallbackImage = {};
    g_fallbackImage.width = 32;
    g_fallbackImage.height = 8;
    g_zImage_pfnCreateFallbackImage = FallbackImageStub;

    g_zImage_TexDirEntryCount = 1;
    zImage_TexDirEntryPartial &entry = g_zImage_TexDirEntries[0];
    std::strcpy(entry.baseName, "missing_1");
    entry.loadState = 2;
    entry.nextVariant = reinterpret_cast<zImage_TexDirEntryPartial *>(1);

    zImage::TexDir_LoadPendingEntries();
    const bool ok = entry.loadState == 1 && entry.image == &g_fallbackImage &&
                    entry.nextVariant == nullptr && g_fallbackImage.uPow2Shift == 5 &&
                    g_fallbackImage.vPow2Shift == 3 && g_fallbackImage.uShiftFrom20 == 15 &&
                    g_fallbackImage.uMask == 0x1f && g_fallbackImage.vMaskFixed20 == 0x00700000;

    std::free(g_zVid_BuiltinTexturePacks);
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = nullptr;
    g_zVideo_ActiveRendererPath = 0;
    g_zImage_TextureMemoryOption = nullptr;
    g_zImage_pfnCreateFallbackImage = nullptr;
    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));

    return ok ? 0 : 1;
}

extern "C" int zimage_texdir_load_pending_entries_renderer_smoke(void) {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    std::free(g_zVid_BuiltinTexturePacks);
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVid_TexturePackLoadState = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = QueryTextureMemoryBytesStub;
    g_zVideo_ActiveRendererPath = 1;
    std::int32_t textureMemoryOption = 1;
    g_zImage_TextureMemoryOption = &textureMemoryOption;

    g_fallbackImage = {};
    g_fallbackImage.width = 16;
    g_fallbackImage.height = 16;
    g_fallbackImage.formatFlagsPacked = 2;
    g_fallbackImage.textureAddressFlagsPacked = 3;
    g_zImage_pfnCreateFallbackImage = FallbackImageStub;

    g_textureCreateCount = 0;
    g_textureFinalizeUploadCount = 0;
    g_lastCreatedImage = nullptr;
    g_lastFinalizedImage = nullptr;
    g_createdTextureRecord = {};
    g_existingTextureRecord = {};
    g_zVideo_pfnCreateTextureRecord = CreateTextureRecordStub;
    g_zVideo_pfnTextureRecordFinalizeUpload = TextureRecordFinalizeUploadStub;
    g_OptCatalogDamageMaskHandles[0] = nullptr;
    g_OptCatalogDamageMaskHandles[1] = nullptr;
    g_OptCatalogDamageMaskHandles[2] = nullptr;

    g_zImage_TexDirEntryCount = 2;
    zImage_TexDirEntryPartial &createEntry = g_zImage_TexDirEntries[0];
    std::strcpy(createEntry.baseName, "render_create");
    createEntry.loadState = 2;
    zImage_TexDirEntryPartial &finalizeEntry = g_zImage_TexDirEntries[1];
    std::strcpy(finalizeEntry.baseName, "render_finalize");
    finalizeEntry.loadState = 3;
    finalizeEntry.texture = &g_existingTextureRecord;

    zImage::TexDir_LoadPendingEntries();
    const bool ok = createEntry.loadState == 1 && finalizeEntry.loadState == 1 &&
                    createEntry.image == &g_fallbackImage &&
                    finalizeEntry.image == &g_fallbackImage &&
                    createEntry.texture == &g_createdTextureRecord &&
                    finalizeEntry.texture == &g_existingTextureRecord &&
                    g_textureCreateCount == 1 && g_textureFinalizeUploadCount == 1 &&
                    g_lastCreatedImage == &g_fallbackImage &&
                    g_lastFinalizedImage == &g_fallbackImage &&
                    g_createdTextureRecord.m_alphaMode == 2 &&
                    g_createdTextureRecord.m_uWrapMode == 1 &&
                    g_createdTextureRecord.m_vWrapMode == 1;

    std::free(g_zVid_BuiltinTexturePacks);
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = nullptr;
    g_zVideo_pfnCreateTextureRecord = nullptr;
    g_zVideo_pfnTextureRecordFinalizeUpload = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zImage_TextureMemoryOption = nullptr;
    g_zImage_pfnCreateFallbackImage = nullptr;
    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    return ok ? 0 : 1;
}

extern "C" int zclass_node_load_flag_bit8_material_images_and_texture_pack_smoke(void) {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    std::free(g_zVid_BuiltinTexturePacks);
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVid_TexturePackLoadState = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = QueryTextureMemoryBytesStub;
    g_zVideo_ActiveRendererPath = 0;
    std::int32_t textureMemoryOption = 1;
    g_zImage_TextureMemoryOption = &textureMemoryOption;

    g_fallbackImage = {};
    g_fallbackImage.width = 32;
    g_fallbackImage.height = 8;
    g_zImage_pfnCreateFallbackImage = FallbackImageStub;

    zImage_TexDirEntryPartial materialEntry{};
    zImage_TexDirEntryPartial frameEntry{};
    materialEntry.loadState = 1;
    frameEntry.loadState = 1;
    zImage_TexDirEntryPartial *frameTable[] = {&frameEntry};
    zModel_MaterialCyclePartial cycle{};
    cycle.frameCount = 1;
    cycle.frameTable = frameTable;
    zModel_MaterialPartial material{};
    material.flags = 0x0300;
    material.currentTextureDirectoryEntry = &materialEntry;
    material.cycle = &cycle;
    zDiEntryPartial diEntry{};
    diEntry.material = &material;
    zDiPartial di{};
    di.entryCount = 1;
    di.entries = &diEntry;
    zClass_NodePartial root{};
    root.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);

    g_zImage_TexDirEntryCount = 1;
    zImage_TexDirEntryPartial &pending = g_zImage_TexDirEntries[0];
    std::strcpy(pending.baseName, "wrapper_pending");
    pending.loadState = 2;

    zClass_Node::LoadFlagBit8MaterialImagesAndTexturePack(&root);
    const bool loaded = materialEntry.loadState == 3 && frameEntry.loadState == 3 &&
                        pending.loadState == 1 && pending.image == &g_fallbackImage;

    pending.loadState = 2;
    pending.image = nullptr;
    zClass_Node::LoadFlagBit8MaterialImagesAndTexturePack(nullptr);
    const bool nullSkipped = pending.loadState == 2 && pending.image == nullptr;

    std::free(g_zVid_BuiltinTexturePacks);
    g_zVid_BuiltinTexturePacks = nullptr;
    g_zVid_BuiltinTexturePackCount = 0;
    g_zVideo_pfnQueryTextureMemoryBytes = nullptr;
    g_zVideo_ActiveRendererPath = 0;
    g_zImage_TextureMemoryOption = nullptr;
    g_zImage_pfnCreateFallbackImage = nullptr;
    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    return loaded && nullSkipped ? 0 : 1;
}

extern "C" int zimage_texdir_base_name_path_smoke(void) {
    char pathA[64] = "textures\\panel.bmp";
    zImage::SetPathExtension(pathA, "tga");
    if (std::strcmp(pathA, "textures\\panel.tga") != 0) {
        return 1;
    }

    char pathB[64] = "textures/panel.bmp";
    zImage::SetPathExtension(pathB, nullptr);
    if (std::strcmp(pathB, "textures/panel") != 0) {
        return 2;
    }

    char pathC[64] = "panel";
    zImage::SetPathExtension(pathC, "bmp");
    if (std::strcmp(pathC, "panel.bmp") != 0) {
        return 3;
    }

    char baseName[64] = {};
    zImage::TexDirSetBaseNameFromPath("dir\\sub/panel.bmp", baseName);
    if (std::strcmp(baseName, "\\sub/panel") != 0) {
        return 4;
    }

    zImage::TexDirSetBaseNameFromPath("panel.bmp", baseName);
    return std::strcmp(baseName, "panel") == 0 ? 0 : 5;
}

extern "C" int zimage_texdir_variant_image_smoke(void) {
    zVidImagePartial image0{};
    zVidImagePartial image1{};
    zVidImagePartial image2{};
    zImage_TexDirEntryPartial entry0{};
    zImage_TexDirEntryPartial entry1{};
    zImage_TexDirEntryPartial entry2{};

    entry0.image = &image0;
    entry0.nextVariant = &entry1;
    entry1.image = &image1;
    entry1.nextVariant = &entry2;
    entry2.image = &image2;

    if (entry0.GetVariantImageAtIndex(-1) != &image0 ||
        entry0.GetVariantImageAtIndex(0) != &image0 ||
        entry0.GetVariantImageAtIndex(1) != &image1 ||
        entry0.GetVariantImageAtIndex(2) != &image2 ||
        entry0.GetVariantImageAtIndex(99) != &image2) {
        return 1;
    }

    return 0;
}

extern "C" int zimage_texdir_find_by_name_smoke(void) {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 3;

    std::strcpy(g_zImage_TexDirEntries[0].baseName, "panel");
    g_zImage_TexDirEntries[0].loadState = 1;
    std::strcpy(g_zImage_TexDirEntries[1].baseName, "inactive");
    g_zImage_TexDirEntries[1].loadState = 0;
    std::strcpy(g_zImage_TexDirEntries[2].baseName, "hud");
    g_zImage_TexDirEntries[2].loadState = 3;

    const bool ok = zImage::FindTexDirEntryByName("panel") == &g_zImage_TexDirEntries[0] &&
                    zImage::FindTexDirEntryByName("hud") == &g_zImage_TexDirEntries[2] &&
                    zImage::FindTexDirEntryByName("inactive") == nullptr &&
                    zImage::FindTexDirEntryByName("missing") == nullptr;

    char existingPath[32] = "panel.bmp";
    zImage_TexDirEntryPartial *const existing = zImage::TexDir_FindOrAppendByPath(existingPath);
    char appendPath[32] = "dir\\new.tga";
    zImage_TexDirEntryPartial *const appended = zImage::TexDir_FindOrAppendByPath(appendPath);
    const bool appendOk =
        existing == &g_zImage_TexDirEntries[0] && std::strcmp(existingPath, "panel.bmp") == 0 &&
        appended == &g_zImage_TexDirEntries[3] && g_zImage_TexDirEntryCount == 4 &&
        std::strcmp(appended->baseName, "\\new") == 0 && appended->loadState == 2;

    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    return ok && appendOk ? 0 : 1;
}

extern "C" int zimage_texdir_write_smoke(void) {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 3;
    g_zImage_TexDirEntries[0].loadState = 10;
    g_zImage_TexDirEntries[0].nextVariant = &g_zImage_TexDirEntries[2];
    g_zImage_TexDirEntries[1].loadState = 20;
    g_zImage_TexDirEntries[1].nextVariant = nullptr;
    g_zImage_TexDirEntries[2].loadState = 30;
    g_zImage_TexDirEntries[2].nextVariant = &g_zImage_TexDirEntries[1];

    if (zImage::TexDirEntryToIndex(nullptr) != -1 ||
        zImage::TexDirEntryToIndex(&g_zImage_TexDirEntries[2]) != 2 ||
        zImage::TexIndexToDirEntry(-1) != nullptr ||
        zImage::TexIndexToDirEntry(2) != &g_zImage_TexDirEntries[2]) {
        g_zImage_TexDirEntryCount = 0;
        return 1;
    }

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        g_zImage_TexDirEntryCount = 0;
        return 2;
    }

    if (zImage::WriteTextureDirectory(file) != 3) {
        std::fclose(file);
        g_zImage_TexDirEntryCount = 0;
        return 3;
    }

    if (g_zImage_TexDirEntries[0].nextVariant != &g_zImage_TexDirEntries[2]) {
        std::fclose(file);
        g_zImage_TexDirEntryCount = 0;
        return 4;
    }

    zImage_TexDirEntryPartial written[3]{};
    std::rewind(file);
    if (std::fread(written, sizeof(written), 1, file) != 1) {
        std::fclose(file);
        g_zImage_TexDirEntryCount = 0;
        return 5;
    }

    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 99;
    std::rewind(file);
    if (zImage::ReadTextureDirectory(3, file) != 3 || g_zImage_TexDirEntryCount != 3 ||
        g_zImage_TexDirEntries[0].nextVariant != &g_zImage_TexDirEntries[2] ||
        g_zImage_TexDirEntries[1].nextVariant != nullptr ||
        g_zImage_TexDirEntries[2].nextVariant != &g_zImage_TexDirEntries[1] ||
        g_zImage_TexDirEntries[2].loadState != 30) {
        std::fclose(file);
        g_zImage_TexDirEntryCount = 0;
        return 6;
    }

    if (zImage::ReadTextureDirectory(0, file) != 0 || g_zImage_TexDirEntryCount != 3 ||
        zImage::ReadTextureDirectory(0x1001, file) != -1) {
        std::fclose(file);
        g_zImage_TexDirEntryCount = 0;
        return 7;
    }

    std::fclose(file);
    g_zImage_TexDirEntryCount = 0;

    return written[0].loadState == 10 &&
                   reinterpret_cast<std::intptr_t>(written[0].nextVariant) == 2 &&
                   written[1].loadState == 20 &&
                   reinterpret_cast<std::intptr_t>(written[1].nextVariant) == -1 &&
                   written[2].loadState == 30 &&
                   reinterpret_cast<std::intptr_t>(written[2].nextVariant) == 1
               ? 0
               : 8;
}

extern "C" int zimage_init_option_fallback_smoke(void) {
    g_zImage_FontTable[0] = reinterpret_cast<zImage_Font *>(0x1234);
    g_zImage_NextFontSlotIndex = 0;
    g_zImage_TextureMemoryDefault = 99;
    g_zImage_TextureMemoryOption = nullptr;
    g_zImage_FontTransparentColor = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zGame_Options_OptionListHead = nullptr;

    if (zImage_Init(nullptr) != 0) {
        return 1;
    }

    return g_zImage_FontTable[0] == nullptr && g_zImage_NextFontSlotIndex == 2 &&
                   g_zImage_FontTransparentColor == 0 && g_zImage_TextureMemoryDefault == 0 &&
                   g_zImage_TextureMemoryOption == &g_zImage_TextureMemoryDefault
               ? 0
               : 2;
}

extern "C" int zimage_init_texture_directory_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    zVideo_CreateTextureRecordProc const oldCreateTextureRecord = g_zVideo_pfnCreateTextureRecord;
    zVideo_TextureRecordPartial *const oldDefaultTextureRecord =
        g_zImage_DefaultTexDirEntry.texture;

    g_zImage_TexDirEntryCount = 3;
    std::memset(g_zImage_TexDirEntries, 0x5a, sizeof(g_zImage_TexDirEntries));
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnCreateTextureRecord = CreateTextureRecordStub;
    g_textureCreateCount = 0;
    g_lastCreatedTextureName = nullptr;
    g_lastCreatedImage = nullptr;

    const int softwareResult = zImage::InitTextureDirectory();
    const bool softwareOk = softwareResult == 1 && g_zImage_TexDirEntryCount == 0 &&
                            g_zImage_TexDirEntries[0].image == nullptr &&
                            g_zImage_TexDirEntries[0].loadState == 0 &&
                            g_textureCreateCount == 0;

    g_zImage_TexDirEntryCount = 2;
    std::memset(g_zImage_TexDirEntries, 0x7b, sizeof(g_zImage_TexDirEntries));
    g_zVideo_ActiveRendererPath = 1;
    g_zImage_DefaultTexDirEntry.texture = nullptr;
    g_textureCreateCount = 0;
    g_lastCreatedTextureName = nullptr;
    g_lastCreatedImage = nullptr;

    const int hardwareResult = zImage::InitTextureDirectory();
    const bool hardwareOk =
        hardwareResult == 1 && g_zImage_TexDirEntryCount == 0 &&
        g_zImage_TexDirEntries[0].image == nullptr && g_zImage_TexDirEntries[0].loadState == 0 &&
        g_textureCreateCount == 1 &&
        g_lastCreatedTextureName == g_zImage_DefaultTexDirEntry.baseName &&
        g_lastCreatedImage == &zVid_Image::g_zImage_DefaultImage &&
        g_zImage_DefaultTexDirEntry.texture == &g_createdTextureRecord;

    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnCreateTextureRecord = oldCreateTextureRecord;
    g_zImage_DefaultTexDirEntry.texture = oldDefaultTextureRecord;
    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    return softwareOk && hardwareOk ? 0 : 1;
}

extern "C" int zimg_init_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    zVideo_CreateTextureRecordProc const oldCreateTextureRecord = g_zVideo_pfnCreateTextureRecord;
    zVideo_TextureRecordPartial *const oldDefaultTextureRecord =
        g_zImage_DefaultTexDirEntry.texture;

    g_zImage_TexDirEntryCount = 1;
    std::memset(g_zImage_TexDirEntries, 0x33, sizeof(g_zImage_TexDirEntries));
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_pfnCreateTextureRecord = CreateTextureRecordStub;
    g_zImage_DefaultTexDirEntry.texture = nullptr;
    g_textureCreateCount = 0;
    g_lastCreatedTextureName = nullptr;
    g_lastCreatedImage = nullptr;

    const int result = zImg::Init();
    const bool ok = result == 1 && g_zImage_TexDirEntryCount == 0 &&
                    g_zImage_TexDirEntries[0].image == nullptr && g_textureCreateCount == 1 &&
                    g_lastCreatedTextureName == g_zImage_DefaultTexDirEntry.baseName &&
                    g_lastCreatedImage == &zVid_Image::g_zImage_DefaultImage &&
                    g_zImage_DefaultTexDirEntry.texture == &g_createdTextureRecord;

    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnCreateTextureRecord = oldCreateTextureRecord;
    g_zImage_DefaultTexDirEntry.texture = oldDefaultTextureRecord;
    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    return ok ? 0 : 1;
}

extern "C" int zimage_init_mission_resources_smoke(void) {
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }

    g_zRdr_ScratchSearchPathList = nullptr;

    char tempDir[MAX_PATH] = {};
    char tempPathA[MAX_PATH] = {};
    char tempPathB[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zim", 0, tempPathA) == 0 ||
        GetTempFileNameA(tempDir, "zin", 0, tempPathB) == 0) {
        return 1;
    }

    if (g_zImage_MissionSearchPathList != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionSearchPathList);
    }
    g_zImage_MissionSearchPathList = nullptr;

    if (zImage_InitMissionResources(tempPathA) != 0 || g_zImage_MissionSearchPathList == nullptr ||
        g_zImage_MissionSearchPathList->count != 1 ||
        std::strcmp(static_cast<const char *>(g_zImage_MissionSearchPathList->head->payload),
                    tempPathA) != 0) {
        DeleteFileA(tempPathA);
        DeleteFileA(tempPathB);
        return 2;
    }

    zImage_InitMissionResources(tempPathB);
    const bool appendOk =
        g_zImage_MissionSearchPathList->count == 2 &&
        std::strcmp(static_cast<const char *>(g_zImage_MissionSearchPathList->head->payload),
                    tempPathB) == 0 &&
        std::strcmp(static_cast<const char *>(g_zImage_MissionSearchPathList->head->next->payload),
                    tempPathA) == 0;

    zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionSearchPathList);
    g_zImage_MissionSearchPathList = nullptr;
    DeleteFileA(tempPathA);
    DeleteFileA(tempPathB);
    return appendOk ? 0 : 3;
}

extern "C" int zimage_shutdown_texdir_smoke(void) {
    g_textureDestroyCount = 0;
    g_uploadSurfaceReleaseCount = 0;
    g_zVideo_pfnTextureRecordDestroy = TextureRecordDestroyStub;
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces =
        TextureRecordReleaseAllUploadSurfacesStub;

    g_zImage_TexDirEntryCount = 3;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    zVideo_TextureRecordPartial texture{};
    g_zImage_TexDirEntries[0].image = &zVid_Image::g_zImage_DefaultImage;
    g_zImage_TexDirEntries[0].texture = &texture;
    g_zImage_TexDirEntries[0].loadState = 1;
    g_zImage_TexDirEntries[1].loadState = 2;
    g_zImage_TexDirEntries[2].loadState = 3;

    g_zVid_PaletteRemapVariantTableCount = 2;
    g_zVid_PaletteRemapVariantTables =
        static_cast<std::uint16_t **>(std::malloc(2 * sizeof(std::uint16_t *)));
    g_zVid_PaletteRemapVariantTables[0] = static_cast<std::uint16_t *>(std::malloc(2));
    g_zVid_PaletteRemapVariantTables[1] = static_cast<std::uint16_t *>(std::malloc(2));
    g_zVid_PaletteRemapRecipeCount = 1;
    g_zVid_PaletteRemapRecipes =
        static_cast<zVidPaletteRemapRecipe *>(std::malloc(sizeof(zVidPaletteRemapRecipe)));

    g_zVid_BuiltinTexturePackCount = 1;
    g_zVid_BuiltinTexturePacks =
        static_cast<zVidTexturePackEntry *>(std::calloc(1, sizeof(zVidTexturePackEntry)));
    g_zVid_BuiltinTexturePacks[0].records =
        static_cast<zVidTexturePackRecord *>(std::malloc(sizeof(zVidTexturePackRecord)));
    g_zVid_BuiltinTexturePacks[0].fileHandle = std::tmpfile();

    g_zVid_TexturePackCount = 1;
    g_zVid_TexturePacks =
        static_cast<zVidTexturePackEntry *>(std::calloc(1, sizeof(zVidTexturePackEntry)));
    g_zVid_TexturePacks[0].records =
        static_cast<zVidTexturePackRecord *>(std::malloc(sizeof(zVidTexturePackRecord)));
    g_zVid_TexturePacks[0].fileHandle = std::tmpfile();

    const std::int32_t shutdownResult = zImage::ShutdownSubsystem();

    const bool ok =
        shutdownResult == 0 && g_textureDestroyCount == 1 && g_uploadSurfaceReleaseCount == 1 &&
        g_zImage_TexDirEntryCount == 0 && g_zImage_TexDirEntries[0].image == nullptr &&
        g_zImage_TexDirEntries[0].texture == nullptr && g_zImage_TexDirEntries[0].loadState == 0 &&
        g_zImage_TexDirEntries[1].loadState == 0 && g_zImage_TexDirEntries[2].loadState == 3 &&
        g_zVid_PaletteRemapVariantTableCount == 0 && g_zVid_PaletteRemapVariantTables == nullptr &&
        g_zVid_PaletteRemapRecipeCount == 0 && g_zVid_PaletteRemapRecipes == nullptr &&
        g_zVid_BuiltinTexturePackCount == 0 && g_zVid_BuiltinTexturePacks == nullptr &&
        g_zVid_TexturePackCount == 0 && g_zVid_TexturePacks == nullptr;

    g_zVideo_pfnTextureRecordDestroy = 0;
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces = 0;
    return ok ? 0 : 1;
}

extern "C" int zvid_image_release_owned_buffers_smoke(void) {
    zVidImagePartial image{};
    image.pixels = std::malloc(4);
    image.alphaMap = static_cast<char *>(std::malloc(4));
    image.palette = std::malloc(4);
    if (image.pixels == nullptr || image.alphaMap == nullptr || image.palette == nullptr) {
        std::free(image.pixels);
        std::free(image.alphaMap);
        std::free(image.palette);
        return 1;
    }

    image.formatFlagsPacked = 0xe0;
    zVid_Image::ReleaseOwnedBuffers(&image);
    if (image.pixels != nullptr || image.alphaMap != nullptr || image.palette != nullptr ||
        (image.formatFlagsPacked & 0xe0) != 0) {
        return 2;
    }

    void *palette = std::malloc(4);
    if (palette == nullptr) {
        return 3;
    }

    image.palette = palette;
    image.formatFlagsPacked = 0x90;
    zVid_Image::ReleaseOwnedBuffers(&image);
    const bool keptSharedPalette = image.palette == palette && image.formatFlagsPacked == 0x90;
    std::free(palette);
    image.palette = nullptr;
    return keptSharedPalette ? 0 : 4;
}

extern "C" int zvid_image_destroy_smoke(void) {
    zVidImagePartial *image =
        static_cast<zVidImagePartial *>(std::malloc(sizeof(zVidImagePartial)));
    if (image == nullptr) {
        return 1;
    }

    *image = {};
    image->pixels = std::malloc(4);
    if (image->pixels == nullptr) {
        std::free(image);
        return 2;
    }
    image->formatFlagsPacked = 0x20;

    return zVid_Image::Destroy(image) == 0 && zVid_Image::Destroy(nullptr) == 0 ? 0 : 3;
}

extern "C" int zvideo_buff_clip_coord_to_range_smoke(void) {
    std::int32_t coord = 5;
    if (zVideo_buff::ClipCoordToRange(&coord, 2, 8) != 0 || coord != 5) {
        return 1;
    }

    coord = -3;
    if (zVideo_buff::ClipCoordToRange(&coord, 2, 8) != -5 || coord != 2) {
        return 2;
    }

    coord = 12;
    return zVideo_buff::ClipCoordToRange(&coord, 2, 8) == 4 && coord == 8 ? 0 : 3;
}

extern "C" int zvideo_buff_copy_surface_rect_to_image_smoke(void) {
    std::uint16_t pixels[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
    std::uint16_t captured[12] = {};
    zVidImagePartial image{};
    image.pixels = captured;

    const zVideo_SurfaceStatePartial oldSwSurfaceState = g_zVideo_SwSurfaceState;
    g_zVideo_SwSurfaceState = {};
    g_zVideo_SwSurfaceState.width = 4;
    g_zVideo_SwSurfaceState.height = 3;
    g_zVideo_SwSurfaceState.pitch = 8;
    g_zVideo_SwSurfaceState.pixels = pixels;

    zVidRect32 rect{-1, 1, 3, 4};
    zVidImagePartial *result = zVideo_buff::CopySurfaceRectToImage(0, &rect, &image);
    if (result != &image) {
        g_zVideo_SwSurfaceState = oldSwSurfaceState;
        return 1;
    }

    if (rect.left != 0 || rect.top != 1 || rect.right != 3 || rect.bottom != 3) {
        g_zVideo_SwSurfaceState = oldSwSurfaceState;
        return 2;
    }

    const bool ok = captured[0] == 0 && captured[1] == 5 && captured[2] == 6 &&
                    captured[3] == 7 && captured[4] == 0 && captured[5] == 9 &&
                    captured[6] == 10 && captured[7] == 11;
    g_zVideo_SwSurfaceState = oldSwSurfaceState;
    return ok ? 0 : 3;
}
