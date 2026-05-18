#include "GameZRecoil/include/zImage.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
zArchiveList *g_zImage_MissionResourcePaths = 0;
int g_zImage_TexDirEntryCount = 0;
zImage_TexDirEntryPartial g_zImage_TexDirEntries[0x1000] = {0};
zImage_Font *g_zImage_FontTable[20] = {0};
int g_zImage_TextureMemoryDefault = 0;
int *g_zImage_TextureMemoryOption = 0;
int g_zImage_FontTransparentColor = 2;
int g_zImage_Unknown5617f4 = 0;
zVidImagePartial *g_zImage_DefaultImagePtr = &zVid_Image::g_zImage_DefaultImage;
zImage_CreateFallbackImageProc g_zImage_pfnCreateFallbackImage = 0;
}

namespace zImage {
// Reimplements 0x46d4c0: zImage::GetDefaultImageRefPtr
// (GameZRecoil/zImage/zimg_texture.cpp)
RECOIL_NOINLINE zImage_TexDirEntryPartial *RECOIL_CDECL GetDefaultImageRefPtr() {
    return reinterpret_cast<zImage_TexDirEntryPartial *>(&g_zImage_DefaultImagePtr);
}
} // namespace zImage

// Reimplements 0x46e290: zImage_TexDirEntryPartial::GetVariantImageAtIndex
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
zImage_TexDirEntryPartial::GetVariantImageAtIndex(int variantIndex) {
    zImage_TexDirEntryPartial *entry = this;
    if (entry == 0) {
        return &zVid_Image::g_zImage_DefaultImage;
    }

    for (int i = 0; i < variantIndex; ++i) {
        if (entry->nextVariant == 0) {
            break;
        }
        entry = entry->nextVariant;
    }

    return entry->image;
}

// Reimplements 0x46e3e0: zImage_TexDirEntry::BuildMipChain
// (GameZRecoil/zImage/zimg_texture.cpp)
RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_FASTCALL zImage_TexDirEntryPartial::BuildMipChain() {
    char variantPath[0x40];
    strcpy(variantPath, baseName);

    zImage_TexDirEntryPartial *const baseEntry = this;
    char *const suffix = strstr(variantPath, "_1");
    if (suffix == 0 || suffix[2] != '\0') {
        return;
    }

    zImage_TexDirEntryPartial *currentEntry = this;
    char *const digit = suffix + 1;
    for (;;) {
        ++*digit;

        zImage_TexDirEntryPartial *variantEntry = zImage::FindTexDirEntryByName(variantPath);
        zVidImagePartial *variantImage = variantEntry != 0 ? variantEntry->image : 0;
        if (variantEntry == 0 || variantEntry->loadState == 2) {
            variantImage = zImage::TexDir_FindOrCreateByPath(variantPath);
            if (variantImage == 0) {
                break;
            }

            if (variantEntry == 0) {
                const int entryIndex = g_zImage_TexDirEntryCount++;
                variantEntry = &g_zImage_TexDirEntries[entryIndex];
                zImage::TexDirSetBaseNameFromPath(variantPath, variantEntry->baseName);
            }

            variantEntry->loadState = 1;
            variantEntry->image = variantImage;
            zVid_Image::CalcPow2ScratchFields(variantImage);
        }

        currentEntry->nextVariant = variantEntry;
        currentEntry = variantEntry;
        variantImage->widthScale =
            static_cast<float>(baseEntry->image->width / variantImage->width);
        variantEntry->nextVariant = 0;
    }
}

namespace {
typedef void (RECOIL_FASTCALL *zVideo_TextureRecordFinalizeUploadProc)(zVideo_TextureRecordPartial *textureRecord, void *reserved, zVidImagePartial *image);
}

namespace zImage {
// Reimplements 0x46de50: zImage::TexDir_LoadPendingEntries
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE int RECOIL_CDECL TexDir_LoadPendingEntries() {
    zVid_TexturePack_EnsureBuiltinTexturePacksLoaded();

    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zImage_TexDirEntryPartial *const entry = &g_zImage_TexDirEntries[i];
        if (entry->loadState != 2 && entry->loadState != 3) {
            continue;
        }

        zVidImagePartial *image = TexDir_FindOrCreateByPath(entry->baseName);
        entry->nextVariant = 0;
        entry->image = image;
        if (image == 0 && g_zImage_pfnCreateFallbackImage != 0) {
            image = g_zImage_pfnCreateFallbackImage(entry->baseName);
            entry->image = image;
        }
        if (entry->image == 0) {
            entry->image = &zVid_Image::g_zImage_DefaultImage;
        }

        entry->BuildMipChain();

        if (g_zVideo_ActiveRendererPath == 0 ||
            OptCatalog_IsDamageMaskSlotPtrRegistered(entry) != 0) {
            zVid_Image::CalcPow2ScratchFields(entry->image);
        } else if (entry->loadState == 3) {
            reinterpret_cast<zVideo_TextureRecordFinalizeUploadProc>(
                g_zVideo_pfnTextureRecordFinalizeUpload)(entry->texture, 0, entry->image);
        } else if (entry->texture == 0) {
            image = entry->image;
            const unsigned short textureAddressFlags =
                static_cast<unsigned short>(image->textureAddressFlagsPacked);
            entry->texture = g_zVideo_pfnCreateTextureRecord(
                entry->baseName, image, image->formatFlagsPacked & 2, textureAddressFlags & 1,
                (textureAddressFlags >> 1) & 1);
            if (g_zVideo_ActiveRendererPath != 2) {
                zVid_Image::ReleaseOwnedBuffers(image);
            }
        }

        entry->loadState = 1;
    }

    ShutdownTextureDirectoryRuntime();
    return 0;
}
} // namespace zImage

// Reimplements 0x46ebd0: zImage_InitMissionResources
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zImage_InitMissionResources(const char *pathText) {
    if (g_zImage_MissionResourcePaths == 0) {
        g_zImage_MissionResourcePaths = zUtil_ZRDR_CreateSearchPathList(pathText);
        return 0;
    }

    zUtil::ZRDR_AddSearchPaths(g_zImage_MissionResourcePaths, pathText);
    return 0;
}

// Reimplements 0x46eb20: zImage_Init
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zImage_Init(const char *fontsPath) {
    memset(g_zImage_FontTable, 0, sizeof(g_zImage_FontTable));
    g_zImage_FontTransparentColor = 2;
    g_zImage_Unknown5617f4 = 0;

    if (fontsPath != 0) {
        zImage::FontsLoadFromPath(fontsPath);
    }

    g_zImage_TextureMemoryDefault = 0;
    const char *optionName =
        g_zVideo_ActiveRendererPath != 0 ? "TextureMemory_HW" : "TextureMemory_SW";
    zOptionEntryPartial *option = zGame::Options_FindOption(optionName);
    g_zImage_TextureMemoryOption =
        option != 0 ? &option->payloadOrBuffer : &g_zImage_TextureMemoryDefault;
    return 0;
}

namespace zImage {
// Reimplements 0x46d310: zImage::TexDirEntryToIndex
// (D:\Proj\GameZRecoil\zImage\zimg_texture.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
TexDirEntryToIndex(zImage_TexDirEntryPartial *texDirEntry) {
    if (texDirEntry == 0) {
        return -1;
    }

    return static_cast<int>(texDirEntry - g_zImage_TexDirEntries);
}

// Reimplements 0x46d340: zImage::TexIndexToDirEntry
// (D:\Proj\GameZRecoil\zImage\zimg_texture.cpp)
RECOIL_NOINLINE zImage_TexDirEntryPartial *RECOIL_FASTCALL TexIndexToDirEntry(int index) {
    if (index == -1) {
        return 0;
    }

    return &g_zImage_TexDirEntries[index];
}

// Reimplements 0x46d4d0: zImage::FindTexDirEntryByName
// (D:\Proj\Battlesport\zimage.cpp)
RECOIL_NOINLINE zImage_TexDirEntryPartial *RECOIL_FASTCALL
FindTexDirEntryByName(const char *baseName) {
    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zImage_TexDirEntryPartial *const entry = &g_zImage_TexDirEntries[i];
        if (entry->loadState != 0 && strcmp(entry->baseName, baseName) == 0) {
            return entry;
        }
    }

    return 0;
}

// Reimplements 0x46d810: zImage::TexDir_FindOrAppendByPath
// (D:\Proj\GameZRecoil\zVideo\zVideo.cpp)
RECOIL_NOINLINE zImage_TexDirEntryPartial *RECOIL_FASTCALL TexDir_FindOrAppendByPath(char *path) {
    char *const extension = strrchr(path, '.');
    if (extension != 0) {
        *extension = '\0';
    }

    zImage_TexDirEntryPartial *entry = FindTexDirEntryByName(path);
    if (extension != 0) {
        *extension = '.';
    }

    if (entry != 0) {
        return entry;
    }

    const int entryIndex = g_zImage_TexDirEntryCount;
    ++g_zImage_TexDirEntryCount;
    entry = &g_zImage_TexDirEntries[entryIndex];
    TexDirSetBaseNameFromPath(path, entry->baseName);
    entry->loadState = 2;
    return entry;
}

// Reimplements 0x46d360: zImage::WriteTextureDirectory
// (D:\Proj\GameZRecoil\zImage\zimg_texture.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL WriteTextureDirectory(void *stream) {
    int count = g_zImage_TexDirEntryCount;
    if (count == 0) {
        return 0;
    }

    const int byteCount =
        count * static_cast<int>(sizeof(zImage_TexDirEntryPartial));
    zImage_TexDirEntryPartial *serializedEntries =
        static_cast<zImage_TexDirEntryPartial *>(malloc(byteCount));
    memcpy(serializedEntries, g_zImage_TexDirEntries, byteCount);

    for (int i = 0; i < count; ++i) {
        serializedEntries[i].nextVariant = reinterpret_cast<zImage_TexDirEntryPartial *>(
            static_cast<int>(TexDirEntryToIndex(serializedEntries[i].nextVariant)));
    }

    if (fwrite(serializedEntries, byteCount, 1, static_cast<FILE *>(stream)) != 1) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zImage\\zimg_texture.cpp", 0x100,
                          "Error writing texture directory.");
        count = 0;
    }

    free(serializedEntries);
    return count;
}

// Reimplements 0x46d420: zImage::ReadTextureDirectory
// (D:\Proj\GameZRecoil\zImage\zimg_texture.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ReadTextureDirectory(int entryCount,
                                                                  void *stream) {
    int count = entryCount;
    if (count == 0) {
        return 0;
    }

    if (count > 0x1000) {
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zImage\\zimg_texture.cpp", 0x11c,
                          "Too many textures for texture array size.");
        return -1;
    }

    const int byteCount =
        count * static_cast<int>(sizeof(zImage_TexDirEntryPartial));
    if (fread(g_zImage_TexDirEntries, byteCount, 1, static_cast<FILE *>(stream)) != 1) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zImage\\zimg_texture.cpp", 0x12a,
                          "Error reading GameZ Texture directory data.");
        return -1;
    }

    g_zImage_TexDirEntryCount = count;
    for (int i = 0; i < count; ++i) {
        g_zImage_TexDirEntries[i].nextVariant = TexIndexToDirEntry(static_cast<int>(
            reinterpret_cast<int>(g_zImage_TexDirEntries[i].nextVariant)));
    }

    return g_zImage_TexDirEntryCount;
}

// Reimplements 0x46e2c0: zImage::SetPathExtension
RECOIL_NOINLINE void RECOIL_FASTCALL SetPathExtension(char *path, const char *extension) {
    char *basePathStart = strchr(path, '\\');
    if (basePathStart == 0) {
        basePathStart = strchr(path, '/');
        if (basePathStart == 0) {
            basePathStart = path;
        }
    }

    char *const dot = strchr(basePathStart, '.');
    if (dot != 0) {
        if (extension == 0) {
            *dot = '\0';
            return;
        }

        strcpy(dot + 1, extension);
        return;
    }

    if (extension != 0) {
        strcat(path, ".");
        strcat(path, extension);
    }
}

// Reimplements 0x46e380: zImage::TexDirSetBaseNameFromPath
RECOIL_NOINLINE void RECOIL_FASTCALL TexDirSetBaseNameFromPath(const char *sourcePath,
                                                               char *destBaseName) {
    const char *baseName = strrchr(sourcePath, '\\');
    if (baseName == 0) {
        baseName = strrchr(sourcePath, '/');
        if (baseName == 0) {
            baseName = sourcePath;
        }
    }

    strcpy(destBaseName, baseName);
    SetPathExtension(destBaseName, 0);
}

// Reimplements 0x46efe0: zImage::FontsLoadFromPath
RECOIL_NOINLINE int RECOIL_FASTCALL FontsLoadFromPath(const char *path) {
    zReader::Node *tree = zReader::LoadNodeFromPath(path, 0, 0);
    if (tree == 0) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zImage\\zimg_fonts.cpp", 0x48,
                          "Failed to read %s", path);
        return -1;
    }

    zImage_InitMissionResources("..\\data\\common\\fonts");
    zReader::Node *fontsNode = zReader_GetNamedNode(tree, "FONTS");
    if (fontsNode == 0) {
        zError::ReportOld(0x800, "D:\\Proj\\GameZRecoil\\zImage\\zimg_fonts.cpp", 0x52,
                          "%s file empty", path);
        return -1;
    }

    zReader::Node *fontArray = fontsNode->value.nodes;
    const int count = fontArray[0].value.i32;
    zImage_Font *font = static_cast<zImage_Font *>(
        malloc(static_cast<size_t>(count - 1) * sizeof(zImage_Font)));

    for (int i = 1; i < count; ++i) {
        zImage_Font **slot = &g_zImage_FontTable[i - 1];
        *slot = font;
        const char *fontImagePath = fontArray[i].value.str;
        font->image = TexDir_FindOrCreateByPath(fontImagePath);
        if (font->image != 0) {
            font->image->formatFlagsPacked |= 0x02;
            const int glyphCount = font->BuildGlyphRects();
            if (glyphCount != 0x5f) {
                zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zImage\\zimg_fonts.cpp", 0x68,
                                  "Only found %d characters in font %s", glyphCount, path);
            }
            ++font;
        }
    }

    zReader::FreeLoadedTree(tree);
    return 0;
}

// Reimplements 0x46d900: zImage::TexDir_FindOrCreateByPath
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL TexDir_FindOrCreateByPath(const char *path) {
    zVidImagePartial *image = zVid_TexturePack_LoadBuiltinImageByName(path);
    if (image == 0) {
        image = zVid_TexturePack_LoadImageByName(path);
    }

    if (image != 0 && (image->formatFlagsPacked & 0x02) != 0 && image->alphaMap != 0) {
        zVid_Image::ClearZeroAlphaPixelsInPlace(image);
    }

    return image;
}

// Reimplements 0x46e250: zImage::InvalidateLoadedVariantChain
RECOIL_NOINLINE void RECOIL_FASTCALL
InvalidateLoadedVariantChain(zImage_TexDirEntryPartial *texDirHead) {
    zImage_TexDirEntryPartial *entry = texDirHead;
    while (entry != 0 && entry->loadState == 1) {
        zVid_Image::ReleaseIfNotDefault(entry->image);
        entry->image = 0;
        entry->loadState = 3;
        entry = entry->nextVariant;
    }
}

// Reimplements 0x46ebb0: zImage::Shutdown
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    zVid_TexDir::Shutdown();
    zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionResourcePaths);
    g_zImage_MissionResourcePaths = 0;
    return 1;
}

// Reimplements 0x46eb90: zImage::ShutdownSubsystem
RECOIL_NOINLINE int RECOIL_CDECL ShutdownSubsystem() {
    Shutdown();
    return 0;
}
} // namespace zImage

// Reimplements 0x46efc0: zImage_Font::GetByIndexOrDefault
RECOIL_NOINLINE zImage_Font *RECOIL_FASTCALL
zImage_Font::GetByIndexOrDefault(int fontIndex) {
    zImage_Font *const font = g_zImage_FontTable[fontIndex];
    if (font != 0) {
        return font;
    }

    return g_zImage_FontTable[0];
}

// Reimplements 0x46f260: zImage_Font::MeasureString
RECOIL_NOINLINE void RECOIL_FASTCALL zImage_Font::MeasureString(const char *text,
                                                                int fontIndex,
                                                                int *outWidthPx,
                                                                int *outLineAdvance) {
    zImage_Font *const font = GetByIndexOrDefault(fontIndex);
    if (font == 0) {
        return;
    }

    const int lineAdvance = font->image->height;
    int currentLineWidth = 0;
    int maxLineWidth = 0;
    int totalLineAdvance = lineAdvance;

    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        const signed char ch = *cursor;
        if (ch == ' ') {
            currentLineWidth += font->spaceWidth;
        } else if (ch == '\r') {
        } else if (ch == '\n') {
            if (currentLineWidth > maxLineWidth) {
                maxLineWidth = currentLineWidth;
            }

            currentLineWidth = 0;
            totalLineAdvance += lineAdvance;
        } else {
            int glyphIndex = static_cast<int>(ch) - 0x21;
            if (glyphIndex < 0 || glyphIndex >= 0x5f) {
                glyphIndex = 0;
            }

            const RECT &glyph = font->glyphRects[glyphIndex];
            currentLineWidth += glyph.right - glyph.left;
        }

        if (currentLineWidth > maxLineWidth) {
            maxLineWidth = currentLineWidth;
        }
    }

    *outWidthPx = maxLineWidth;
    *outLineAdvance = totalLineAdvance;
}

// Reimplements 0x4c7f00: zImage_Font::BlitStringToActiveTarget
// (D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
zImage_Font::BlitStringToActiveTarget(const char *text, int dstX, int dstY,
                                      int fontIndex) {
    int currentX = dstX;
    int currentY = dstY;
    zImage_Font *font = GetByIndexOrDefault(fontIndex);
    if (font == 0) {
        font = GetByIndexOrDefault(0);
        if (font == 0) {
            return;
        }
    }

    zVidImagePartial *const fontImage = font->image;
    if (dstY + fontImage->height >= zRndr::g_activeRegionHeight) {
        return;
    }

    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        const signed char ch = *cursor;
        if (ch == ' ') {
            currentX += font->spaceWidth;
        } else if (ch == '\r') {
        } else if (ch == '\n') {
            currentX = dstX;
            currentY += fontImage->height;
        } else {
            int glyphIndex = static_cast<int>(ch) - 0x21;
            if (glyphIndex < 0 || glyphIndex >= 0x5f) {
                glyphIndex = 0;
            }

            RECT *glyph = &font->glyphRects[glyphIndex];
            zVid_Image::BlitToActiveTarget(font->image, currentX, currentY, 0,
                                           reinterpret_cast<zVidRect32 *>(glyph));
            currentX += glyph->right - glyph->left;
        }
    }
}

// Reimplements 0x46f210: zImage_Font::IsImageColumnTransparent
RECOIL_NOINLINE int RECOIL_FASTCALL
zImage_Font::IsImageColumnTransparent(zVidImagePartial *image, int columnX) {
    const int width = image->width;
    unsigned short *column = static_cast<unsigned short *>(image->pixels) + columnX;
    if (columnX >= width) {
        return 0;
    }

    const int height = image->height;
    if (height <= 0) {
        return 1;
    }

    for (int y = 0; y < height; ++y) {
        if (*column != static_cast<unsigned short>(g_zImage_Unknown5617f4)) {
            return 0;
        }
        column += width;
    }

    return 1;
}

// Reimplements 0x46f130: zImage_Font::BuildGlyphRects
RECOIL_NOINLINE int RECOIL_THISCALL zImage_Font::BuildGlyphRects() {
    zVidImagePartial *image = this->image;
    int x = 0;
    int result = 1;
    this->spaceWidth = image->width / 95 - 1;

    while (x < image->width) {
        RECT *glyph = &this->glyphRects[result - 1];
        glyph->top = 0;
        glyph->bottom = image->height - 1;

        if (IsImageColumnTransparent(image, x) != 0) {
            do {
                ++x;
            } while (IsImageColumnTransparent(image, x) != 0);
        }

        const int left = x;
        while (IsImageColumnTransparent(image, x) == 0) {
            ++x;
        }

        const int right = x;
        if (IsImageColumnTransparent(image, x) != 0) {
            do {
                ++x;
            } while (IsImageColumnTransparent(image, x) != 0);
        }

        x -= (x - right) / 2;
        glyph->left = left;
        glyph->right = right + 1;

        ++result;
        if (result >= 95) {
            break;
        }
    }

    return result;
}
