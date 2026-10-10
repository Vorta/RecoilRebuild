#ifndef GAMEZRECOIL_INCLUDE_ZIMAGE_H
#define GAMEZRECOIL_INCLUDE_ZIMAGE_H

#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/zVideo/zvid.h"
#include "recoil/recoil_callconv.h"

typedef struct zArchiveList zArchiveList;
typedef zVidImagePartial*(__fastcall* zImage_CreateFallbackImageProc)(char* path);

typedef struct zImage_TexDirEntryPartial {
    zVidImagePartial* image;
    zVideo_TextureRecordPartial* texture;
    char baseName[0x14];
    int loadState;
    struct zImage_TexDirEntryPartial* nextVariant;
#ifdef __cplusplus
    zVidImagePartial* __fastcall GetVariantImageAtIndex(int variantIndex);
    RECOIL_NO_GS void __fastcall BuildMipChain();
#endif
} zImage_TexDirEntryPartial;
#ifdef __cplusplus
struct zImage_Font {
    zVidImagePartial* image;
    int spaceWidth;
    RECT glyphRects[95];

    static zImage_Font* __fastcall GetByIndexOrDefault(int fontIndex);
    static void __fastcall MeasureString(const char* text, int fontIndex, int* outWidthPx, int* outLineAdvance);
    static void __fastcall BlitStringToActiveTarget(const char* text, int dstX, int dstY, int fontIndex);
    int BuildGlyphRects();
    static int __fastcall IsImageColumnTransparent(zVidImagePartial* image, int columnX);
};
extern "C" {
extern zArchiveList* g_zImage_MissionSearchPathList;
extern zImage_Font* g_zImage_FontTable[20];
extern int g_zImage_TextureMemoryDefault;
extern int* g_zImage_TextureMemoryOption;
extern int g_zImage_FontTransparentColor;
extern int g_zImage_NextFontSlotIndex;
extern zImage_TexDirEntryPartial g_zImage_DefaultTexDirEntry;
extern zImage_CreateFallbackImageProc g_zImage_pfnCreateFallbackImage;
}

namespace zImage {
void __fastcall SetPathExtension(char* path, const char* extension);
void __fastcall TexDirSetBaseNameFromPath(const char* sourcePath, char* destBaseName);
int __fastcall FontsLoadFromPath(const char* path);
zVidImagePartial* __fastcall TexDirFindOrCreateByPath(const char* path);
extern "C" int __fastcall TexDirEntryToIndex(zImage_TexDirEntryPartial* texDirEntry);
extern "C" zImage_TexDirEntryPartial* __fastcall TexIndexToDirEntry(int index);
zImage_TexDirEntryPartial* __fastcall FindTexDirEntryByName(const char* baseName);
extern "C" zImage_TexDirEntryPartial* __cdecl GetDefaultImageRefPtr();
int __cdecl InitTextureDirectory();
zImage_TexDirEntryPartial* __fastcall TexDirFindOrAppendByPath(char* path);
extern "C" int __cdecl TexDirLoadPendingEntries();
extern "C" int __fastcall WriteTextureDirectory(void* stream);
extern "C" int __fastcall ReadTextureDirectory(int entryCount, void* stream);
extern "C" void __fastcall InvalidateLoadedVariantChain(zImage_TexDirEntryPartial* texDirHead);
int __cdecl ShutdownTextureDirectoryRuntime();
int __cdecl Shutdown();
int __cdecl ShutdownSubsystem();
} // namespace zImage

namespace zVid_TexDir {
int __fastcall Shutdown();
}

namespace zImg {
int __cdecl Init();
}

extern "C" {
int __fastcall zImageInitMissionResources(const char* pathText);
int __fastcall zImageInit(const char* fontsPath);
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
#else
/* C units' view of the zImage members they call (zimg_texture.cpp defines them). */
int __fastcall TexDirEntryToIndex(zImage_TexDirEntryPartial* texDirEntry);
zImage_TexDirEntryPartial* __fastcall TexIndexToDirEntry(int index);
zImage_TexDirEntryPartial* __cdecl GetDefaultImageRefPtr();
void __fastcall InvalidateLoadedVariantChain(zImage_TexDirEntryPartial* texDirHead);
#endif

#endif // GAMEZRECOIL_INCLUDE_ZIMAGE_H
