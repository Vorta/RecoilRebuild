#ifndef GAMEZRECOIL_INCLUDE_ZIMAGE_H
#define GAMEZRECOIL_INCLUDE_ZIMAGE_H

#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/zVideo/zVideo.h"
#include "recoil/recoil_callconv.h"

struct zArchiveList;
typedef zVidImagePartial *(__fastcall *zImage_CreateFallbackImageProc)(char *path);

struct zImage_TexDirEntryPartial {
    zVidImagePartial *image;
    zVideo_TextureRecordPartial *texture;
    char baseName[0x14];
    int loadState;
    zImage_TexDirEntryPartial *nextVariant;

    zVidImagePartial *__fastcall GetVariantImageAtIndex(int variantIndex);
    RECOIL_NO_GS void __fastcall BuildMipChain();
};

struct zImage_Font {
    zVidImagePartial *image;
    int spaceWidth;
    RECT glyphRects[95];

    static zImage_Font *__fastcall GetByIndexOrDefault(int fontIndex);
    static void __fastcall MeasureString(
        const char *text,
        int fontIndex,
        int *outWidthPx,
        int *outLineAdvance
    );
    static void __fastcall BlitStringToActiveTarget(
        const char *text,
        int dstX,
        int dstY,
        int fontIndex
    );
    int BuildGlyphRects();
    static int __fastcall IsImageColumnTransparent(
        zVidImagePartial *image,
        int columnX
    );
};

extern "C" {
extern zArchiveList *g_zImage_MissionSearchPathList;
extern int g_zImage_TexDirEntryCount;
extern zImage_TexDirEntryPartial g_zImage_TexDirEntries[0x1000];
extern zImage_Font *g_zImage_FontTable[20];
extern int g_zImage_TextureMemoryDefault;
extern int *g_zImage_TextureMemoryOption;
extern int g_zImage_FontTransparentColor;
extern int g_zImage_NextFontSlotIndex;
extern zVidImagePartial *g_zImage_DefaultImagePtr;
extern zVideo_TextureRecordPartial *g_zImage_DefaultTextureRecord;
extern zImage_CreateFallbackImageProc g_zImage_pfnCreateFallbackImage;
}

namespace zImage {
void __fastcall SetPathExtension(
    char *path,
    const char *extension
);
void __fastcall TexDirSetBaseNameFromPath(
    const char *sourcePath,
    char *destBaseName
);
int __fastcall FontsLoadFromPath(const char *path);
zVidImagePartial *__fastcall TexDir_FindOrCreateByPath(const char *path);
int __fastcall TexDirEntryToIndex(zImage_TexDirEntryPartial *texDirEntry);
zImage_TexDirEntryPartial *__fastcall TexIndexToDirEntry(int index);
zImage_TexDirEntryPartial *__fastcall FindTexDirEntryByName(
    const char *baseName
);
zImage_TexDirEntryPartial *GetDefaultImageRefPtr();
zVideo_TextureRecordPartial *CreateDefaultTextureRecord();
int InitTextureDirectory();
zImage_TexDirEntryPartial *__fastcall TexDir_FindOrAppendByPath(char *path);
int TexDir_LoadPendingEntries();
int __fastcall WriteTextureDirectory(void *stream);
int __fastcall ReadTextureDirectory(
    int entryCount,
    void *stream
);
void __fastcall InvalidateLoadedVariantChain(
    zImage_TexDirEntryPartial *texDirHead
);
int ShutdownTextureDirectoryRuntime();
int Shutdown();
int ShutdownSubsystem();
} // namespace zImage

namespace zVid_TexDir {
int Shutdown();
}

namespace zImg {
int Init();
}

extern "C" {
extern char g_zImage_DefaultTextureName[0x10];
int __fastcall zImage_InitMissionResources(const char *pathText);
int __fastcall zImage_Init(const char *fontsPath);
}

RECOIL_STATIC_ASSERT(
    offsetof(
        zImage_TexDirEntryPartial,
        image
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zImage_TexDirEntryPartial,
        texture
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zImage_TexDirEntryPartial,
        baseName
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zImage_TexDirEntryPartial,
        loadState
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zImage_TexDirEntryPartial,
        nextVariant
    ) == 0x20
);
RECOIL_STATIC_ASSERT(sizeof(zImage_TexDirEntryPartial) == 0x24);
RECOIL_STATIC_ASSERT(
    offsetof(
        zImage_Font,
        image
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zImage_Font,
        spaceWidth
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zImage_Font,
        glyphRects
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zImage_Font) == 0x5f8);

#endif // GAMEZRECOIL_INCLUDE_ZIMAGE_H
