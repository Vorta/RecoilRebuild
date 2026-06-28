#include "GameZRecoil/include/zImage.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char g_HudSensorTracker_ReadFileFailedFmt[18];
extern char g_HudCfgKey_Fonts[6];

extern "C" {
/**
 * Reimplements data 0x53d790: g_zImage_NextFontSlotIndex.
 * Purpose: hold the next font-slot cursor value initialized by zImage_Init.
 *
 * Evidence: BN xrefs show only zImage::Init writing the value 2 before it
 * clears the 20-entry font table. No current retail code reads this slot.
 */
int g_zImage_NextFontSlotIndex = 0;
/**
 * Reimplements data 0x53d794: g_zImage_MissionSearchPathList.
 * Purpose: hold the mission resource search-path list shared by image and
 * texture-pack loading.
 *
 * Evidence: BN xrefs show InitMissionResources creating/appending this list,
 * Shutdown freeing and clearing it, and built-in texture-pack loading using it
 * for resolved archive file opens.
 */
zArchiveList *g_zImage_MissionSearchPathList = 0;
/**
 * Reimplements data 0x53d798: g_zImage_TexDirEntryCount.
 * Data owner: engine.zimage.texture_directory_state.
 * Purpose: track the active prefix of the fixed texture-directory table.
 *
 * Retail 0x53d798: active count for the fixed texture-directory table at
 * 0x53d79c. BN xrefs show this count is reset, serialized, scanned, and
 * appended by the zimg_texture.cpp texture-directory routines and shared with
 * zVid_TexDir shutdown/palette-remap cleanup.
 */
int g_zImage_TexDirEntryCount = 0;
/**
 * Reimplements data 0x53d79c: g_zImage_TexDirEntries.
 * Data owner: engine.zimage.texture_directory_state.
 * Purpose: store the fixed texture-directory records used by image loading,
 * serialization, and mip/variant chaining.
 *
 * Retail 0x53d79c: fixed 0x1000-entry zImage_TexDirEntry table, zero-filled
 * in BSS. Each record is 0x24 bytes and stores the image, texture record,
 * basename, load state, and mip/variant link used by directory serialization
 * and runtime loading.
 */
zImage_TexDirEntryPartial g_zImage_TexDirEntries[0x1000] = {0};
/**
 * Reimplements data 0x56179c: g_zImage_FontTable.
 * Purpose: hold the twenty runtime font slots used by zImage font lookup and
 * text rendering.
 *
 * Evidence: BN zImage::Init clears this contiguous 20-pointer table after
 * initializing the font-slot cursor, FontsLoadFromPath populates slots 1..19,
 * and zImage_Font::GetByIndexOrDefault falls back to slot 0 when an indexed
 * slot is null.
 */
zImage_Font *g_zImage_FontTable[20] = {0};
/**
 * Reimplements data 0x5617ec: g_zImage_TextureMemoryDefault.
 * Purpose: provide the local fallback texture-memory option value when no
 * runtime option record is registered.
 *
 * Evidence: BN zImage::Init zeroes this slot and stores its address into the
 * texture-memory option pointer when zGame::Options_FindOption returns null.
 */
int g_zImage_TextureMemoryDefault = 0;
/**
 * Reimplements data 0x5617f0: g_zImage_TextureMemoryOption.
 * Purpose: point image-loading code at the active texture-memory option
 * storage, or at the local default when no renderer option is registered.
 *
 * Evidence: BN zImage::Init selects the TextureMemory_HW or TextureMemory_SW
 * option name from g_zVideo_ActiveRendererPath, calls zGame::Options_FindOption,
 * and stores either the option payload address or
 * g_zImage_TextureMemoryDefault.
 */
int *g_zImage_TextureMemoryOption = 0;
/**
 * Reimplements data 0x5617f4: g_zImage_FontTransparentColor.
 * Purpose: store the transparent font pixel color used while scanning glyph
 * columns.
 *
 * Evidence: BN zImage_Font::IsImageColumnTransparent loads this value for
 * every non-empty column scan. zImage::Init resets it to zero during startup.
 */
int g_zImage_FontTransparentColor = 0;
/**
 * Reimplements data 0x4e0718: g_zImage_DefaultImagePtr.
 * Data owner: engine.zimage.texture_directory_state.
 * Purpose: hold the default zVid image pointer in the legacy entry-reference
 * storage shape.
 *
 * Retail 0x4e0718: initialized default image pointer. BN initializes it to
 * zVid_Image::g_zImage_DefaultImage at 0x4e06e0; GetDefaultImageRefPtr exposes
 * this storage address as the default texture-directory entry reference shape
 * expected by legacy material callers.
 */
zVidImagePartial *g_zImage_DefaultImagePtr = &zVid_Image::g_zImage_DefaultImage;
/**
 * Reimplements data 0x4e071c: g_zImage_DefaultTextureRecord.
 * Data owner: engine.zimage.texture_directory_state.
 * Purpose: remember the default hardware texture record created for the
 * zImage texture directory.
 *
 * Retail 0x4e071c: initialized null texture-record pointer adjacent to the
 * zImage default image pointer and default texture name. InitTextureDirectory
 * is the zImage writer for this texture-directory storage. The similarly
 * named video-runtime pointer at 0x6333a8 is a separate zVideo owner and is
 * intentionally not folded into this zImage data symbol.
 */
zVideo_TextureRecordPartial *g_zImage_DefaultTextureRecord = 0;
/**
 * Reimplements data 0x53d788: g_zImage_pfnCreateFallbackImage.
 * Data owner: engine.zimage.texture_directory_state.
 * Purpose: optionally create fallback images when a pending texture-directory
 * load cannot resolve an image pack entry.
 *
 * Retail 0x53d788: optional fastcall fallback-image callback used only by
 * TexDir_LoadPendingEntries when normal texture-pack lookup returns null.
 */
zImage_CreateFallbackImageProc g_zImage_pfnCreateFallbackImage = 0;
/**
 * Reimplements data 0x4e0720: g_zImage_DefaultTextureName.
 * Data owner: engine.zimage.texture_directory_state.
 * Purpose: name the hardware default texture record created during texture
 * directory initialization.
 *
 * Retail 0x4e0720: 16-byte initialized default texture name. BN shows the
 * exact bytes "DEFAULT_TEXTURE\0" and InitTextureDirectory passes this storage
 * to the hardware texture-record creation callback.
 */
char g_zImage_DefaultTextureName[0x10] = "DEFAULT_TEXTURE";
/**
 * Reimplements data 0x4e0850: g_zImage_FontVariantSuffix.
 * Data owner: engine.zimage.texture_directory_state.
 * Purpose: provide the writable mip-chain suffix seed used to identify base
 * texture variants.
 *
 * Retail 0x4e0850: three-byte initialized string "_1". BN xrefs show
 * zImage_TexDirEntry::BuildMipChain tests texture-directory base names against
 * this suffix before incrementing the trailing digit in place in the local
 * variant path. The recovered BN name is misleading; the data belongs to the
 * zImage texture-directory mip-chain runtime, not font state.
 */
char g_zImage_FontVariantSuffix[0x3] = "_1";
/**
 * Reimplements data 0x4e0740: g_zImage_SourceFile_ZimgTextureCpp.
 * Data owner: engine.zimage.texture_directory_diagnostic_literals_data.
 * Purpose: source-file literal shared by texture-directory diagnostics.
 *
 * Retail 0x4e0740: writable source-path literal used by
 * zImage::WriteTextureDirectory and zImage::ReadTextureDirectory error
 * reports. BN xrefs at 0x46d3e5, 0x46d43f, and 0x46d47e.
 */
char g_zImage_SourceFile_ZimgTextureCpp[] =
    "D:\\Proj\\GameZRecoil\\zImage\\zimg_texture.cpp";
/**
 * Reimplements data 0x4e076c: g_zImage_WriteTextureDirectoryErrorMsg.
 * Data owner: engine.zimage.texture_directory_diagnostic_literals_data.
 * Purpose: report texture-directory serialization write failures.
 *
 * Retail 0x4e076c: writable diagnostic literal passed by
 * zImage::WriteTextureDirectory when fwrite fails.
 */
char g_zImage_WriteTextureDirectoryErrorMsg[] =
    "Error writing texture directory.";
/**
 * Reimplements data 0x4e0790: g_zImage_ReadGameZTextureDirectoryDataErrorMsg.
 * Data owner: engine.zimage.texture_directory_diagnostic_literals_data.
 * Purpose: report texture-directory binary block read failures.
 *
 * Retail 0x4e0790: writable diagnostic literal passed by
 * zImage::ReadTextureDirectory when fread fails.
 */
char g_zImage_ReadGameZTextureDirectoryDataErrorMsg[] =
    "Error reading GameZ Texture directory data.";
/**
 * Reimplements data 0x4e07bc: g_zImage_TextureArraySizeExceededMsg.
 * Data owner: engine.zimage.texture_directory_diagnostic_literals_data.
 * Purpose: report serialized texture-directory counts beyond table capacity.
 *
 * Retail 0x4e07bc: writable diagnostic literal passed by
 * zImage::ReadTextureDirectory for counts above the 0x1000-entry table.
 */
char g_zImage_TextureArraySizeExceededMsg[] =
    "Too many textures for texture array size.";
}

namespace zImage {
/**
 * Reimplements 0x46d4c0: zImage::GetDefaultImageRefPtr.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: expose the default image pointer through the texture-directory
 * entry reference shape expected by legacy callers.
 *
 * Evidence: BN returns the address of g_zImage_DefaultImagePtr retyped as a
 * texture-directory entry pointer without touching additional state.
 */
zImage_TexDirEntryPartial *GetDefaultImageRefPtr() {
    return (zImage_TexDirEntryPartial *)(&g_zImage_DefaultImagePtr);
}

/**
 * Recovered helper: zImage::CreateDefaultTextureRecord.
 * Original shape: no standalone retail function is currently identified in the
 * inspected BN/plan evidence.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: create and remember the default texture record through the active
 * zVideo texture creation callback.
 *
 * Original helper evidence: source-faithful helper recovered from BN caller
 * body 0x46d550, which routes the DEFAULT_TEXTURE/default-image contract
 * through g_zVideo_pfnCreateTextureRecord and stores the result in
 * g_zImage_DefaultTextureRecord. BN 0x4a75f0 uses a direct null-name
 * default-image call instead.
 */
zVideo_TextureRecordPartial *CreateDefaultTextureRecord() {
    zVidImagePartial *image = &zVid_Image::g_zImage_DefaultImage;
    int releaseImage = 0;
    if (g_zImage_pfnCreateFallbackImage != 0) {
        zVidImagePartial *fallbackImage = g_zImage_pfnCreateFallbackImage(
            g_zImage_DefaultTextureName
        );
        if (fallbackImage != 0) {
            image = fallbackImage;
            releaseImage = 1;
        }
    }

    g_zImage_DefaultTextureRecord = g_zVideo_pfnCreateTextureRecord(
        g_zImage_DefaultTextureName,
        image,
        image->formatFlagsPacked & 2,
        image->textureAddressFlagsPacked & 1,
        (image->textureAddressFlagsPacked >> 1) & 1
    );
    if (releaseImage != 0) {
        zVid_Image::Destroy(image);
    }
    return g_zImage_DefaultTextureRecord;
}

/**
 * Reimplements 0x46d550: zImage::InitTextureDirectory.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: reset the texture-directory table and create the hardware default
 * texture record when a non-software renderer is active.
 *
 * Evidence: BN clears the texture-directory count/table, tests
 * g_zVideo_ActiveRendererPath, and on the hardware path creates the default
 * texture record before returning success.
 */
int InitTextureDirectory() {
    g_zImage_TexDirEntryCount = 0;
    memset(
        g_zImage_TexDirEntries,
        0,
        sizeof(g_zImage_TexDirEntries)
    );

    if (g_zVideo_ActiveRendererPath != 0) {
        CreateDefaultTextureRecord();
    }

    return 1;
}
} // namespace zImage

namespace zImg {
/**
 * Reimplements 0x46eba0: zImg::Init.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: initialize the zImage texture-directory runtime and report success.
 *
 * Evidence: BN assembly is a single call to zImage::InitTextureDirectory
 * followed by return value 1; this is a namespace-level wrapper, not a class
 * method or table-dispatch shape.
 */
int Init() {
    zImage::InitTextureDirectory();
    return 1;
}
} // namespace zImg

/**
 * Reimplements 0x46e290: zImage_TexDirEntryPartial::GetVariantImageAtIndex.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: return the requested image from a texture-directory variant chain.
 *
 * Evidence: BN treats this as a zImage_TexDirEntry member leaf: null self
 * returns g_zImage_DefaultImage, non-positive indexes return this->image,
 * and positive indexes walk nextVariant at offset 0x20 until the index or
 * chain tail is reached.
 */
zVidImagePartial *__fastcall zImage_TexDirEntryPartial::GetVariantImageAtIndex(
    int variantIndex
) {
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

/**
 * Reimplements 0x46e3e0: zImage_TexDirEntry::BuildMipChain.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: link numbered mip texture-directory variants and assign each
 * variant image width scale from the base image width.
 *
 * Evidence: BN assembly copies baseName into a 0x40-byte local path, accepts
 * only names ending in "_1", increments that suffix digit in place, looks up
 * or loads each numbered variant, appends missing records through
 * g_zImage_TexDirEntries/g_zImage_TexDirEntryCount, calls
 * zVid_Image::CalcPow2ScratchFields for newly loaded variants, links
 * nextVariant, and stores the integer width quotient as a float. RECOIL_NO_GS
 * is retained as a host-build annotation for the local buffer; VC5 has no /GS
 * cookie for this retail-era member.
 */
RECOIL_NO_GS void __fastcall zImage_TexDirEntryPartial::BuildMipChain() {
    char variantPath[0x40];
    strcpy(
        variantPath,
        baseName
    );

    zImage_TexDirEntryPartial *const baseEntry = this;
    char *const suffix = strstr(
        variantPath,
        g_zImage_FontVariantSuffix
    );
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
                zImage::TexDirSetBaseNameFromPath(
                    variantPath,
                    variantEntry->baseName
                );
            }

            variantEntry->loadState = 1;
            variantEntry->image = variantImage;
            zVid_Image::CalcPow2ScratchFields(variantImage);
        }

        currentEntry->nextVariant = variantEntry;
        currentEntry = variantEntry;
        variantImage->widthScale = (float)(baseEntry->image->width / variantImage->width);
        variantEntry->nextVariant = 0;
    }
}

namespace zImage {
/**
 * Reimplements 0x46de50: zImage::TexDir_LoadPendingEntries.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: resolve pending texture-directory entries, create renderer texture
 * records when needed, and retire transient texture-pack runtime state.
 *
 * Evidence: BN iterates g_zImage_TexDirEntries up to
 * g_zImage_TexDirEntryCount, handles only load states 2 and 3, loads images by
 * basename with the optional fallback callback, falls back to
 * zVid_Image::g_zImage_DefaultImage, builds mip chains, then either updates
 * image scratch fields or creates/finalizes texture records before setting
 * loadState to 1 and calling ShutdownTextureDirectoryRuntime.
 */
int TexDir_LoadPendingEntries() {
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
            zVideo_TextureRecordFinalizeUploadProc finalizeUpload =
                g_zVideo_pfnTextureRecordFinalizeUpload;
            finalizeUpload(
                entry->texture,
                0,
                entry->image
            );
        } else if (entry->texture == 0) {
            image = entry->image;
            const unsigned short textureAddressFlags =
                (unsigned short)(image->textureAddressFlagsPacked);
            entry->texture = g_zVideo_pfnCreateTextureRecord(
                entry->baseName,
                image,
                image->formatFlagsPacked & 2,
                textureAddressFlags & 1,
                (textureAddressFlags >> 1) & 1
            );
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

/**
 * Reimplements 0x46ebd0: zImage_InitMissionResources.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: create the mission resource search-path list on first use and
 * append later paths to the existing list.
 *
 * Evidence: BN loads g_zImage_MissionSearchPathList, creates a new list when
 * it is null, otherwise passes the existing list and incoming path to
 * zUtil::ZRDR_AddSearchPaths, then returns 0.
 */
extern "C" int __fastcall zImage_InitMissionResources(
    const char *pathText
) {
    if (g_zImage_MissionSearchPathList == 0) {
        g_zImage_MissionSearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        return 0;
    }

    zUtil::ZRDR_AddSearchPaths(
        g_zImage_MissionSearchPathList,
        pathText
    );
    return 0;
}

/**
 * Reimplements 0x46eb20: zImage_Init.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: reset font-table state, optionally load fonts, and bind the
 * texture-memory option value used by image loading.
 *
 * Evidence: BN stores 2 to g_zImage_NextFontSlotIndex at 0x53d790, stores 0
 * to g_zImage_FontTransparentColor at 0x5617f4, clears the 20-entry font
 * table, optionally loads fonts, and binds the texture-memory option pointer
 * to the active renderer's option or the local default.
 */
extern "C" int __fastcall zImage_Init(
    const char *fontsPath
) {
    g_zImage_NextFontSlotIndex = 2;
    g_zImage_FontTransparentColor = 0;
    memset(
        g_zImage_FontTable,
        0,
        sizeof(g_zImage_FontTable)
    );

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
/**
 * Reimplements 0x46d310: zImage::TexDirEntryToIndex.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: convert a texture-directory entry pointer to its serialized table
 * index.
 *
 * Evidence: BN returns -1 for a null entry pointer and otherwise subtracts the
 * fixed g_zImage_TexDirEntries table base from the entry pointer.
 */
int __fastcall TexDirEntryToIndex(
    zImage_TexDirEntryPartial *texDirEntry
) {
    if (texDirEntry == 0) {
        return -1;
    }

    return (int)(texDirEntry - g_zImage_TexDirEntries);
}

/**
 * Reimplements 0x46d340: zImage::TexIndexToDirEntry.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: restore a serialized texture-directory index to a table entry
 * pointer.
 *
 * Evidence: BN maps -1 back to null and otherwise indexes the fixed
 * g_zImage_TexDirEntries table directly.
 */
zImage_TexDirEntryPartial *__fastcall TexIndexToDirEntry(
    int index
) {
    if (index == -1) {
        return 0;
    }

    return &g_zImage_TexDirEntries[index];
}

/**
 * Reimplements 0x46d4d0: zImage::FindTexDirEntryByName.
 * Original file: D:\Proj\Battlesport\zimage.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: find an active texture-directory entry by base texture name.
 *
 * Evidence: BN scans the active g_zImage_TexDirEntries prefix, skips entries
 * with loadState zero, compares baseName with strcmp, and returns null when no
 * matching active entry exists.
 */
zImage_TexDirEntryPartial *__fastcall FindTexDirEntryByName(
    const char *baseName
) {
    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zImage_TexDirEntryPartial *const entry = &g_zImage_TexDirEntries[i];
        if (entry->loadState != 0 && strcmp(
            entry->baseName,
            baseName
        ) == 0) {
            return entry;
        }
    }

    return 0;
}

/**
 * Reimplements 0x46d810: zImage::TexDir_FindOrAppendByPath.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: find a texture-directory entry for a path or append a pending
 * record for later loading.
 *
 * Evidence: BN temporarily strips the extension for lookup, restores the path
 * text before returning, and appends a new g_zImage_TexDirEntries record with
 * loadState 2 when no existing basename entry is present.
 */
zImage_TexDirEntryPartial *__fastcall TexDir_FindOrAppendByPath(
    char *path
) {
    char *const extension = strrchr(
        path,
        '.'
    );
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
    TexDirSetBaseNameFromPath(
        path,
        entry->baseName
    );
    entry->loadState = 2;
    return entry;
}

/**
 * Reimplements 0x46d360: zImage::WriteTextureDirectory.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: serialize the active texture-directory records with variant links
 * converted to table indexes.
 *
 * Evidence: BN returns zero for an empty directory, copies the active fixed
 * table prefix to heap storage, rewrites each nextVariant pointer through
 * TexDirEntryToIndex, writes one binary block, reports the retail write error
 * on fwrite failure, then frees the temporary copy.
 */
int __fastcall WriteTextureDirectory(
    void *stream
) {
    int count = g_zImage_TexDirEntryCount;
    if (count == 0) {
        return 0;
    }

    const int byteCount = count * (int)(sizeof(zImage_TexDirEntryPartial));
    zImage_TexDirEntryPartial *serializedEntries = (zImage_TexDirEntryPartial *)(malloc(byteCount));
    memcpy(
        serializedEntries,
        g_zImage_TexDirEntries,
        byteCount
    );

    for (int i = 0; i < count; ++i) {
        serializedEntries[i].nextVariant = (zImage_TexDirEntryPartial *)((int)(TexDirEntryToIndex(
            serializedEntries[i].nextVariant
        )));
    }

    if (fwrite(
        serializedEntries,
        byteCount,
        1,
        (FILE *)(stream)
    ) != 1) {
        zError::ReportOld(
            0x200,
            g_zImage_SourceFile_ZimgTextureCpp,
            0x100,
            g_zImage_WriteTextureDirectoryErrorMsg
        );
        count = 0;
    }

    free(serializedEntries);
    return count;
}

/**
 * Reimplements 0x46d420: zImage::ReadTextureDirectory.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: read serialized texture-directory records and restore in-memory
 * variant links.
 *
 * Evidence: BN returns zero for an empty stream count, rejects counts above
 * the 0x1000-entry fixed table capacity with the retail error, reads the table
 * prefix directly into g_zImage_TexDirEntries, stores
 * g_zImage_TexDirEntryCount, and converts each serialized nextVariant index
 * back through TexIndexToDirEntry.
 */
int __fastcall ReadTextureDirectory(
    int entryCount,
    void *stream
) {
    int count = entryCount;
    if (count == 0) {
        return 0;
    }

    if (count > 0x1000) {
        zError::ReportOld(
            0x100,
            g_zImage_SourceFile_ZimgTextureCpp,
            0x11c,
            g_zImage_TextureArraySizeExceededMsg
        );
        return -1;
    }

    const int byteCount = count * (int)(sizeof(zImage_TexDirEntryPartial));
    if (fread(
        g_zImage_TexDirEntries,
        byteCount,
        1,
        (FILE *)(stream)
    ) != 1) {
        zError::ReportOld(
            0x200,
            g_zImage_SourceFile_ZimgTextureCpp,
            0x12a,
            g_zImage_ReadGameZTextureDirectoryDataErrorMsg
        );
        return -1;
    }

    g_zImage_TexDirEntryCount = count;
    for (int i = 0; i < count; ++i) {
        g_zImage_TexDirEntries[i].nextVariant =
            TexIndexToDirEntry((int)((int)(g_zImage_TexDirEntries[i].nextVariant)));
    }

    return g_zImage_TexDirEntryCount;
}

/**
 * Reimplements 0x46e2c0: zImage::SetPathExtension.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: replace, remove, or append the extension portion of a mutable path.
 *
 * Evidence: BN searches after the last directory separator candidate used by
 * the retail helper, truncates at the dot when extension is null, copies a
 * replacement extension after an existing dot, or appends "." plus the new
 * extension when no dot exists.
 */
void __fastcall SetPathExtension(
    char *path,
    const char *extension
) {
    char *basePathStart = strchr(
        path,
        '\\'
    );
    if (basePathStart == 0) {
        basePathStart = strchr(
            path,
            '/'
        );
        if (basePathStart == 0) {
            basePathStart = path;
        }
    }

    char *const dot = strchr(
        basePathStart,
        '.'
    );
    if (dot != 0) {
        if (extension == 0) {
            *dot = '\0';
            return;
        }

        strcpy(
            dot + 1,
            extension
        );
        return;
    }

    if (extension != 0) {
        /* Retail literal 0x4e084c is the compiler-emitted "." used when
           appending a missing extension separator. */
        strcat(
            path,
            "."
        );
        strcat(
            path,
            extension
        );
    }
}

/**
 * Reimplements 0x46e380: zImage::TexDirSetBaseNameFromPath.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: copy a path basename into texture-directory storage without its
 * extension.
 *
 * Evidence: BN chooses the substring after the last backslash or slash,
 * copies it to the destination basename buffer, and reuses SetPathExtension
 * with a null extension to strip the suffix in place.
 */
void __fastcall TexDirSetBaseNameFromPath(
    const char *sourcePath,
    char *destBaseName
) {
    const char *baseName = strrchr(
        sourcePath,
        '\\'
    );
    if (baseName == 0) {
        baseName = strrchr(
            sourcePath,
            '/'
        );
        if (baseName == 0) {
            baseName = sourcePath;
        }
    }

    strcpy(
        destBaseName,
        baseName
    );
    SetPathExtension(
        destBaseName,
        0
    );
}

/**
 * Reimplements 0x46efe0: zImage::FontsLoadFromPath.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: load the FONTS node, create font records, load each font image,
 * and build glyph rectangles for the font table.
 *
 * Evidence: BN reports the same error paths, mission resource initialization,
 * FONTS node lookup, per-font image load, alpha flag update, glyph build call,
 * and loaded-tree release.
 */
int __fastcall FontsLoadFromPath(
    const char *path
) {
    zReader::Node *tree = zReader::LoadNodeFromPath(
        path,
        0,
        0
    );
    if (tree == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\GameZRecoil\\zImage\\zimg_fonts.cpp",
            0x48,
            g_HudSensorTracker_ReadFileFailedFmt,
            path
        );
        return -1;
    }

    zImage_InitMissionResources("..\\data\\common\\fonts");
    zReader::Node *fontsNode = zReader_GetNamedNode(
        tree,
        g_HudCfgKey_Fonts
    );
    if (fontsNode == 0) {
        zError::ReportOld(
            0x800,
            "D:\\Proj\\GameZRecoil\\zImage\\zimg_fonts.cpp",
            0x52,
            "%s file empty",
            path
        );
        return -1;
    }

    zReader::Node *fontArray = fontsNode->value.nodes;
    const int count = fontArray[0].value.i32;
    zImage_Font *font = (zImage_Font *)(malloc((size_t)(count - 1) * sizeof(zImage_Font)));

    for (int i = 1; i < count; ++i) {
        zImage_Font **slot = &g_zImage_FontTable[i - 1];
        *slot = font;
        const char *fontImagePath = fontArray[i].value.str;
        font->image = TexDir_FindOrCreateByPath(fontImagePath);
        if (font->image != 0) {
            font->image->formatFlagsPacked |= 0x02;
            const int glyphCount = font->BuildGlyphRects();
            if (glyphCount != 0x5f) {
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zImage\\zimg_fonts.cpp",
                    0x68,
                    "Only found %d characters in font %s",
                    glyphCount,
                    path
                );
            }
            ++font;
        }
    }

    zReader::FreeLoadedTree(tree);
    return 0;
}

/**
 * Reimplements 0x46d900: zImage::TexDir_FindOrCreateByPath.
 * Purpose: load or reuse the texture-directory image for a path.
 * Evidence: BN calls the dynamic texture-pack lookup first, then the builtin
 * lookup, and clears zero-alpha pixels on a loaded image before returning it.
 */
zVidImagePartial *__fastcall TexDir_FindOrCreateByPath(
    const char *path
) {
    zVidImagePartial *image = zVid_TexturePack_LoadBuiltinImageByName(path);
    if (image == 0) {
        image = zVid_TexturePack_LoadImageByName(path);
    }

    if (image != 0 && (image->formatFlagsPacked & 0x02) != 0 && image->alphaMap != 0) {
        zVid_Image::ClearZeroAlphaPixelsInPlace(image);
    }

    return image;
}

/**
 * Reimplements 0x46e250: zImage::InvalidateLoadedVariantChain.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: mark a loaded texture-directory variant chain for reload.
 *
 * Evidence: BN walks nextVariant while entries are loaded, releases each
 * non-default image through zVid_Image::ReleaseIfNotDefault, clears image, sets
 * loadState to 3, and stops at the first null or non-loaded chain entry.
 */
void __fastcall InvalidateLoadedVariantChain(
    zImage_TexDirEntryPartial *texDirHead
) {
    zImage_TexDirEntryPartial *entry = texDirHead;
    while (entry != 0 && entry->loadState == 1) {
        zVid_Image::ReleaseIfNotDefault(entry->image);
        entry->image = 0;
        entry->loadState = 3;
        entry = entry->nextVariant;
    }
}

/**
 * Reimplements 0x46ebb0: zImage::Shutdown.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: shut down texture-directory state and release the mission resource
 * search-path list.
 *
 * Evidence: BN calls zVid_TexDir::Shutdown, frees
 * g_zImage_MissionSearchPathList through zUtil_ZRDR::FreeSearchPathList,
 * stores the returned null pointer, and returns 1.
 */
int Shutdown() {
    zVid_TexDir::Shutdown();
    zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionSearchPathList);
    g_zImage_MissionSearchPathList = 0;
    return 1;
}

/**
 * Reimplements 0x46eb90: zImage::ShutdownSubsystem.
 * Original file: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: provide the subsystem shutdown entry point that delegates to
 * zImage::Shutdown and reports success.
 *
 * Evidence: BN assembly contains only the zImage::Shutdown call followed by
 * zero return value setup.
 */
int ShutdownSubsystem() {
    Shutdown();
    return 0;
}
} // namespace zImage

/**
 * Reimplements 0x46efc0: zImage_Font::GetByIndexOrDefault.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: return a requested font table slot, falling back to slot 0 when
 * the requested slot is empty.
 *
 * Evidence: BN performs one g_zImage_FontTable indexed load, tests it, and
 * loads g_zImage_FontTable[0] only on the null-slot path.
 */
zImage_Font *__fastcall zImage_Font::GetByIndexOrDefault(
    int fontIndex
) {
    zImage_Font *const font = g_zImage_FontTable[fontIndex];
    if (font != 0) {
        return font;
    }

    return g_zImage_FontTable[0];
}

/**
 * Reimplements 0x46f260: zImage_Font::MeasureString.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: measure wrapped font text width and total line advance.
 *
 * Evidence: BN gets the requested font with fallback, uses image height as
 * line advance, treats space, carriage return, and newline specially, clamps
 * printable glyph indexes to the 95-glyph table, and writes both outputs.
 */
void __fastcall zImage_Font::MeasureString(
    const char *text,
    int fontIndex,
    int *outWidthPx,
    int *outLineAdvance
) {
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
            int glyphIndex = (int)(ch)-0x21;
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

/**
 * Reimplements 0x4c7f00: zImage_Font::BlitStringToActiveTarget.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: draw a string to the active target using the selected font image
 * and glyph rectangles.
 *
 * Evidence: BN falls back through font slot 0, clips against active region
 * height, advances spaces and newlines, clamps printable glyph indexes, and
 * blits each glyph through zVid_Image::BlitToActiveTarget.
 */
void __fastcall zImage_Font::BlitStringToActiveTarget(
    const char *text,
    int dstX,
    int dstY,
    int fontIndex
) {
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
            int glyphIndex = (int)(ch)-0x21;
            if (glyphIndex < 0 || glyphIndex >= 0x5f) {
                glyphIndex = 0;
            }

            RECT *glyph = &font->glyphRects[glyphIndex];
            zVid_Image::BlitToActiveTarget(
                font->image,
                currentX,
                currentY,
                0,
                (zVidRect32 *)(glyph)
            );
            currentX += glyph->right - glyph->left;
        }
    }
}

/**
 * Reimplements 0x46f210: zImage_Font::IsImageColumnTransparent.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: test whether a vertical image column contains only the configured
 * transparent font color.
 *
 * Evidence: BN rejects columns beyond image width, treats non-positive height
 * as transparent, then walks one 16-bit pixel per row using image width as the
 * row stride.
 */
int __fastcall zImage_Font::IsImageColumnTransparent(
    zVidImagePartial *image,
    int columnX
) {
    const int width = image->width;
    unsigned short *column = (unsigned short *)(image->pixels) + columnX;
    if (columnX >= width) {
        return 0;
    }

    const int height = image->height;
    if (height <= 0) {
        return 1;
    }

    for (int y = 0; y < height; ++y) {
        if (*column != (unsigned short)(g_zImage_FontTransparentColor)) {
            return 0;
        }
        column += width;
    }

    return 1;
}

/**
 * Reimplements 0x46f130: zImage_Font::BuildGlyphRects.
 * Original file: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
 * Purpose: scan the font image into glyph rectangles and compute the space
 * width used by font text layout.
 *
 * Evidence: BN initializes space width from image width divided by 95 minus
 * one, scans transparent and non-transparent column runs through
 * IsImageColumnTransparent, fills glyph RECT bounds, and returns the glyph
 * count.
 */
int zImage_Font::BuildGlyphRects() {
    zVidImagePartial *image = this->image;
    int x = 0;
    int result = 1;
    this->spaceWidth = image->width / 95 - 1;

    while (x < image->width) {
        RECT *glyph = &this->glyphRects[result - 1];
        glyph->top = 0;
        glyph->bottom = image->height - 1;

        if (IsImageColumnTransparent(
            image,
            x
        ) != 0) {
            do {
                ++x;
            } while (IsImageColumnTransparent(
                image,
                x
            ) != 0);
        }

        const int left = x;
        while (IsImageColumnTransparent(
            image,
            x
        ) == 0) {
            ++x;
        }

        const int right = x;
        if (IsImageColumnTransparent(
            image,
            x
        ) != 0) {
            do {
                ++x;
            } while (IsImageColumnTransparent(
                image,
                x
            ) != 0);
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
