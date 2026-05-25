#ifndef GAMEZRECOIL_INCLUDE_ZIMAGE_H
#define GAMEZRECOIL_INCLUDE_ZIMAGE_H

#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/zVideo/zVideo.h"
#include "recoil/recoil_callconv.h"

struct zArchiveList;
typedef zVidImagePartial * (RECOIL_FASTCALL *zImage_CreateFallbackImageProc)(char *path);

struct zImage_TexDirEntryPartial {
    zVidImagePartial *image;
    zVideo_TextureRecordPartial *texture;
    char baseName[0x14];
    int loadState;
    zImage_TexDirEntryPartial *nextVariant;

    RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL
    GetVariantImageAtIndex(int variantIndex);
    RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_FASTCALL BuildMipChain();
};

struct zImage_Font {
    zVidImagePartial *image;
    int spaceWidth;
    RECT glyphRects[95];

    RECOIL_NOINLINE static zImage_Font *RECOIL_FASTCALL GetByIndexOrDefault(int fontIndex);
    RECOIL_NOINLINE static void RECOIL_FASTCALL MeasureString(const char *text,
                                                              int fontIndex,
                                                              int *outWidthPx,
                                                              int *outLineAdvance);
    RECOIL_NOINLINE static void RECOIL_FASTCALL BlitStringToActiveTarget(
        const char *text, int dstX, int dstY, int fontIndex);
    RECOIL_NOINLINE int RECOIL_THISCALL BuildGlyphRects();
    RECOIL_NOINLINE static int RECOIL_FASTCALL
    IsImageColumnTransparent(zVidImagePartial *image, int columnX);
};

extern "C" {
extern zArchiveList *g_zImage_MissionResourcePaths;
extern int g_zImage_TexDirEntryCount;
extern zImage_TexDirEntryPartial g_zImage_TexDirEntries[0x1000];
extern zImage_Font *g_zImage_FontTable[20];
extern int g_zImage_TextureMemoryDefault;
extern int *g_zImage_TextureMemoryOption;
extern int g_zImage_FontTransparentColor;
extern int g_zImage_Unknown5617f4;
extern zVidImagePartial *g_zImage_DefaultImagePtr;
extern zImage_CreateFallbackImageProc g_zImage_pfnCreateFallbackImage;
}

namespace zImage {
RECOIL_NOINLINE void RECOIL_FASTCALL SetPathExtension(char *path, const char *extension);
RECOIL_NOINLINE void RECOIL_FASTCALL TexDirSetBaseNameFromPath(const char *sourcePath,
                                                               char *destBaseName);
RECOIL_NOINLINE int RECOIL_FASTCALL FontsLoadFromPath(const char *path);
RECOIL_NOINLINE zVidImagePartial *RECOIL_FASTCALL TexDir_FindOrCreateByPath(const char *path);
RECOIL_NOINLINE int RECOIL_FASTCALL
TexDirEntryToIndex(zImage_TexDirEntryPartial *texDirEntry);
RECOIL_NOINLINE zImage_TexDirEntryPartial *RECOIL_FASTCALL TexIndexToDirEntry(int index);
RECOIL_NOINLINE zImage_TexDirEntryPartial *RECOIL_FASTCALL
FindTexDirEntryByName(const char *baseName);
RECOIL_NOINLINE zImage_TexDirEntryPartial *RECOIL_CDECL GetDefaultImageRefPtr();
RECOIL_NOINLINE int RECOIL_CDECL InitTextureDirectory();
RECOIL_NOINLINE zImage_TexDirEntryPartial *RECOIL_FASTCALL TexDir_FindOrAppendByPath(char *path);
RECOIL_NOINLINE int RECOIL_CDECL TexDir_LoadPendingEntries();
RECOIL_NOINLINE int RECOIL_FASTCALL WriteTextureDirectory(void *stream);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadTextureDirectory(int entryCount,
                                                                  void *stream);
RECOIL_NOINLINE void RECOIL_FASTCALL
InvalidateLoadedVariantChain(zImage_TexDirEntryPartial *texDirHead);
RECOIL_NOINLINE int RECOIL_CDECL ShutdownTextureDirectoryRuntime();
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownSubsystem();
} // namespace zImage

namespace zVid_TexDir {
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
}

namespace zImg {
RECOIL_NOINLINE int RECOIL_CDECL Init();
}

extern "C" {
extern char g_zImage_DefaultTextureName[0x10];
RECOIL_NOINLINE int RECOIL_FASTCALL zImage_InitMissionResources(const char *pathText);
RECOIL_NOINLINE int RECOIL_FASTCALL zImage_Init(const char *fontsPath);
}

RECOIL_STATIC_ASSERT(offsetof(zImage_TexDirEntryPartial, image) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zImage_TexDirEntryPartial, texture) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zImage_TexDirEntryPartial, baseName) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zImage_TexDirEntryPartial, loadState) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zImage_TexDirEntryPartial, nextVariant) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(zImage_TexDirEntryPartial) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zImage_Font, image) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zImage_Font, spaceWidth) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zImage_Font, glyphRects) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zImage_Font) == 0x5f8);

#endif // GAMEZRECOIL_INCLUDE_ZIMAGE_H
