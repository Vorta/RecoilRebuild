#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zReader/zReader.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
int g_textureDestroyCount = 0;
int g_uploadSurfaceReleaseCount = 0;

void RECOIL_FASTCALL TextureRecordDestroyStub(zVideo_TextureRecordPartial *) {
    ++g_textureDestroyCount;
}

void RECOIL_CDECL TextureRecordReleaseAllUploadSurfacesStub() {
    ++g_uploadSurfaceReleaseCount;
}

std::int32_t RECOIL_FASTCALL QueryTextureMemoryBytesStub(std::int32_t, std::int32_t *totalBytes,
                                                         std::int32_t *freeBytes) {
    *totalBytes = 6 << 20;
    *freeBytes = 4 << 20;
    return 1;
}

zVidImagePartial g_fallbackImage = {};

zVidImagePartial *RECOIL_FASTCALL FallbackImageStub(char *) {
    return &g_fallbackImage;
}
} // namespace

extern "C" int zimage_font_glyph_scan_smoke(void) {
    constexpr std::int32_t kWidth = 192;
    std::uint16_t pixels[kWidth] = {};
    for (std::int32_t x = 0; x < kWidth; ++x) {
        pixels[x] = (x & 1) == 0 ? 0x1234 : 0x5678;
    }

    g_zImage_Unknown5617f4 = 0x1234;

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
    g_zImage_TextureMemoryDefault = 99;
    g_zImage_TextureMemoryOption = nullptr;
    g_zImage_FontTransparentColor = 0;
    g_zImage_Unknown5617f4 = 7;
    g_zVideo_ActiveRendererPath = 0;
    g_zGame_Options_OptionListHead = nullptr;

    if (zImage_Init(nullptr) != 0) {
        return 1;
    }

    return g_zImage_FontTable[0] == nullptr && g_zImage_FontTransparentColor == 2 &&
                   g_zImage_Unknown5617f4 == 0 && g_zImage_TextureMemoryDefault == 0 &&
                   g_zImage_TextureMemoryOption == &g_zImage_TextureMemoryDefault
               ? 0
               : 2;
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

    if (g_zImage_MissionResourcePaths != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionResourcePaths);
    }
    g_zImage_MissionResourcePaths = nullptr;

    if (zImage_InitMissionResources(tempPathA) != 0 || g_zImage_MissionResourcePaths == nullptr ||
        g_zImage_MissionResourcePaths->count != 1 ||
        std::strcmp(static_cast<const char *>(g_zImage_MissionResourcePaths->head->payload),
                    tempPathA) != 0) {
        DeleteFileA(tempPathA);
        DeleteFileA(tempPathB);
        return 2;
    }

    zImage_InitMissionResources(tempPathB);
    const bool appendOk =
        g_zImage_MissionResourcePaths->count == 2 &&
        std::strcmp(static_cast<const char *>(g_zImage_MissionResourcePaths->head->payload),
                    tempPathB) == 0 &&
        std::strcmp(static_cast<const char *>(g_zImage_MissionResourcePaths->head->next->payload),
                    tempPathA) == 0;

    zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionResourcePaths);
    g_zImage_MissionResourcePaths = nullptr;
    DeleteFileA(tempPathA);
    DeleteFileA(tempPathB);
    return appendOk ? 0 : 3;
}

extern "C" int zimage_shutdown_texdir_smoke(void) {
    g_textureDestroyCount = 0;
    g_uploadSurfaceReleaseCount = 0;
    g_zVideo_pfnTextureRecordDestroy = reinterpret_cast<std::uint32_t>(&TextureRecordDestroyStub);
    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces =
        reinterpret_cast<std::uint32_t>(&TextureRecordReleaseAllUploadSurfacesStub);

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
