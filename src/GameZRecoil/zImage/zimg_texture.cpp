#include "GameZRecoil/include/zimage.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
extern char g_zVid_DefaultImageTexturePackReadonlyNameFmt[0x04];
extern char g_zVid_DefaultImageTexturePackName[0x0a];
extern char g_zVid_TextureArchiveNameFmt[0x08];
extern char g_zVid_TextureArchiveSizedNameFmt[0x08];
extern char g_zVid_TextureArchiveMaxName[0x10];
extern char g_zVid_TextureArchiveSize2Fmt[0x08];
extern char g_zVid_TextureArchiveSize4Fmt[0x08];
extern char g_zVid_TextureArchiveSize6Fmt[0x08];
extern char g_zVid_TextureArchiveSize8Fmt[0x08];
extern char g_zVid_TextureArchiveRendererSizedNameFmt[0x0c];
extern char g_zVid_TextureArchiveStem[0x08];
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-nextfontslotindex
 * @recoil-artifact defines .data recoil:data:0x53d790: g_zImage_NextFontSlotIndex.
 * Purpose: hold the next font-slot cursor value initialized by zImage_Init.
 *
 * Evidence: BN xrefs show only zImage::Init writing the value 2 before it
 * clears the 20-entry font table. No current retail code reads this slot.
 */
int g_zImage_NextFontSlotIndex = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-missionsearchpathlist
 * @recoil-artifact defines .data recoil:data:0x53d794: g_zImage_MissionSearchPathList.
 * Purpose: hold the mission resource search-path list shared by image and
 * texture-pack loading.
 *
 * Evidence: BN xrefs show InitMissionResources creating/appending this list,
 * Shutdown freeing and clearing it, and built-in texture-pack loading using it
 * for resolved archive file opens.
 */
zArchiveList *g_zImage_MissionSearchPathList = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-texdirentrycount
 * @recoil-artifact defines .data recoil:data:0x53d798: g_zImage_TexDirEntryCount.
 * Data owner: engine.zimage.texture_directory_state_data.
 * Purpose: track the active prefix of the fixed texture-directory table.
 *
 * Retail 0x53d798: active count for the fixed texture-directory table at
 * 0x53d79c. BN xrefs show this count is reset, serialized, scanned, and
 * appended by the zimg_texture.cpp texture-directory routines and shared with
 * zVid_TexDir shutdown/palette-remap cleanup.
 */
int g_zImage_TexDirEntryCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-texdirentries
 * @recoil-artifact defines .data recoil:data:0x53d79c: g_zImage_TexDirEntries.
 * Data owner: engine.zimage.texture_directory_state_data.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-fonttable
 * @recoil-artifact defines .data recoil:data:0x56179c: g_zImage_FontTable.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-texturememorydefault
 * @recoil-artifact defines .data recoil:data:0x5617ec: g_zImage_TextureMemoryDefault.
 * Purpose: provide the local fallback texture-memory option value when no
 * runtime option record is registered.
 *
 * Evidence: BN zImage::Init zeroes this slot and stores its address into the
 * texture-memory option pointer when zGame::Options_FindOption returns null.
 */
int g_zImage_TextureMemoryDefault = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-texturememoryoption
 * @recoil-artifact defines .data recoil:data:0x5617f0: g_zImage_TextureMemoryOption.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-fonttransparentcolor
 * @recoil-artifact defines .data recoil:data:0x5617f4: g_zImage_FontTransparentColor.
 * Purpose: store the transparent font pixel color used while scanning glyph
 * columns.
 *
 * Evidence: BN zImage_Font::IsImageColumnTransparent loads this value for
 * every non-empty column scan. zImage::Init resets it to zero during startup.
 */
int g_zImage_FontTransparentColor = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-pfncreatefallbackimage
 * @recoil-artifact defines .data recoil:data:0x53d788: g_zImage_pfnCreateFallbackImage.
 * Data owner: engine.zimage.texture_directory_state_data.
 * Purpose: optionally create fallback images when a pending texture-directory
 * load cannot resolve an image pack entry.
 *
 * Retail 0x53d788: optional fastcall fallback-image callback used only by
 * TexDir_LoadPendingEntries when normal texture-pack lookup returns null.
 */
zImage_CreateFallbackImageProc g_zImage_pfnCreateFallbackImage = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-defaulttexdirentry
 * @recoil-artifact defines .data recoil:data:0x4e0718: g_zImage_DefaultTexDirEntry.
 * Data owner: engine.zimage.texture_directory_state_data.
 * Purpose: hold the initialized default texture-directory entry shared by
 * legacy material callers and default texture-record creation.
 *
 * Retail 0x4e0718..0x4e073b is one 0x24-byte zImage_TexDirEntryPartial record:
 * image points at zVid_Image::g_zImage_DefaultImage, texture is null,
 * baseName is "DEFAULT_TEXTURE", loadState is loaded, and nextVariant is null.
 * 0x4e073c starts the separate zVid texture-pack load-state row.
 */
zImage_TexDirEntryPartial g_zImage_DefaultTexDirEntry = {
    &zVid_Image::g_zImage_DefaultImage,
    0,
    "DEFAULT_TEXTURE",
    1,
    0
};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-fontvariantsuffix
 * @recoil-artifact defines .data recoil:data:0x4e0850: g_zImage_FontVariantSuffix.
 * Data owner: engine.zimage.texture_directory_state_data.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-sourcefile-zimgtexturecpp
 * @recoil-artifact defines .data recoil:data:0x4e0740: g_zImage_SourceFile_ZimgTextureCpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-writetexturedirectoryerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e076c: g_zImage_WriteTextureDirectoryErrorMsg.
 * Data owner: engine.zimage.texture_directory_diagnostic_literals_data.
 * Purpose: report texture-directory serialization write failures.
 *
 * Retail 0x4e076c: writable diagnostic literal passed by
 * zImage::WriteTextureDirectory when fwrite fails.
 */
char g_zImage_WriteTextureDirectoryErrorMsg[] =
    "Error writing texture directory.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-readgameztexturedirectorydataerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e0790: g_zImage_ReadGameZTextureDirectoryDataErrorMsg.
 * Data owner: engine.zimage.texture_directory_diagnostic_literals_data.
 * Purpose: report texture-directory binary block read failures.
 *
 * Retail 0x4e0790: writable diagnostic literal passed by
 * zImage::ReadTextureDirectory when fread fails.
 */
char g_zImage_ReadGameZTextureDirectoryDataErrorMsg[] =
    "Error reading GameZ Texture directory data.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-g-zimage-texturearraysizeexceededmsg
 * @recoil-artifact defines .data recoil:data:0x4e07bc: g_zImage_TextureArraySizeExceededMsg.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-texdirentrytoindex
 * @recoil-artifact defines .text recoil:function:0x46d310: zImage::TexDirEntryToIndex.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-texindextodirentry
 * @recoil-artifact defines .text recoil:function:0x46d340: zImage::TexIndexToDirEntry.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-writetexturedirectory
 * @recoil-artifact defines .text recoil:function:0x46d360: zImage::WriteTextureDirectory.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-readtexturedirectory
 * @recoil-artifact defines .text recoil:function:0x46d420: zImage::ReadTextureDirectory.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-getdefaultimagerefptr
 * @recoil-artifact defines .text recoil:function:0x46d4c0: zImage::GetDefaultImageRefPtr.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
 * Source owner: engine.zimage.texture_directory_state.
 * Purpose: expose the default image pointer through the texture-directory
 * entry reference shape expected by legacy callers.
 *
 * Evidence: BN returns the address of the initialized default texture-directory
 * record without touching additional state.
 */
zImage_TexDirEntryPartial *GetDefaultImageRefPtr() {
    return &g_zImage_DefaultTexDirEntry;
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
 * through g_zVideo_pfnCreateTextureRecord and stores the result in the default
 * texture-directory record. BN 0x4a75f0 uses a direct null-name
 * default-image call instead.
 */
zVideo_TextureRecordPartial *CreateDefaultTextureRecord() {
    zVidImagePartial *image = &zVid_Image::g_zImage_DefaultImage;
    int releaseImage = 0;
    if (g_zImage_pfnCreateFallbackImage != 0) {
        zVidImagePartial *fallbackImage = g_zImage_pfnCreateFallbackImage(
            g_zImage_DefaultTexDirEntry.baseName
        );
        if (fallbackImage != 0) {
            image = fallbackImage;
            releaseImage = 1;
        }
    }

    g_zImage_DefaultTexDirEntry.texture = g_zVideo_pfnCreateTextureRecord(
        g_zImage_DefaultTexDirEntry.baseName,
        image,
        image->formatFlagsPacked & 2,
        image->textureAddressFlagsPacked & 1,
        (image->textureAddressFlagsPacked >> 1) & 1
    );
    if (releaseImage != 0) {
        zVid_Image::Destroy(image);
    }
    return g_zImage_DefaultTexDirEntry.texture;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-findtexdirentrybyname
 * @recoil-artifact defines .text recoil:function:0x46d4d0: zImage::FindTexDirEntryByName.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\zimage.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-inittexturedirectory
 * @recoil-artifact defines .text recoil:function:0x46d550: zImage::InitTextureDirectory.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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

namespace {
const int kZVidPaletteColorCount = 256;
const int kZVidPaletteRemapVariantCount = 32;
const int kZVidPaletteRemapColorsPerRecipe =
    kZVidPaletteColorCount * kZVidPaletteRemapVariantCount;
} // namespace

namespace zVid_Image {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-releaseifnotdefault
 * @recoil-artifact defines .text recoil:function:0x46d5a0: zVid_Image::ReleaseIfNotDefault.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: destroy dynamically allocated images while preserving the initialized default image singleton.
 *
 * Evidence: BN compares the incoming image against g_zImage_DefaultImage,
 * calls zVid_Image::Destroy only for non-default images, and returns 0.
 * The VC5-era throw() declaration is retained because callers such as
 * HudUiBackground::~HudUiBackground use it to match retail EH cleanup state numbering.
 */
int __fastcall ReleaseIfNotDefault(
    zVidImagePartial *image
) throw() {
    if (image != &g_zImage_DefaultImage) {
        Destroy(image);
    }

    return 0;
}
} // namespace zVid_Image

namespace zVid {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-settexturepackloadstate
 * @recoil-artifact defines .text recoil:function:0x46d5b0: zVid::SetTexturePackLoadState
 * Purpose: Store the texture-pack loading enable state.
 */
void __fastcall SetTexturePackLoadState(
    int texturePackLoadState
) {
    g_zVid_TexturePackLoadState = texturePackLoadState;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-gettexturepackloadstate
 * @recoil-artifact defines .text recoil:function:0x46d5c0: zVid::GetTexturePackLoadState
 * Purpose: Return the current texture-pack loading enable state.
 */
int GetTexturePackLoadState() {
    return g_zVid_TexturePackLoadState;
}
} // namespace zVid

namespace zVid_TexDir {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-texdir-shutdown
 * @recoil-artifact defines .text recoil:function:0x46d5d0: zVid_TexDir::Shutdown.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: shut down texture-directory entries, palette remap storage, and
 * texture-pack state.
 *
 * Evidence: BN walks g_zImage_TexDirEntries as 0x24-byte records, releases
 * only loadState 1 image/texture pairs, clears loadState 1 and 2 entries,
 * then unconditionally calls the active upload-surface release callback before
 * freeing palette remap tables, recipes, and both texture-pack banks.
 */
int Shutdown() {
    for (int i = 0; i < g_zImage_TexDirEntryCount; ++i) {
        zImage_TexDirEntryPartial &entry = g_zImage_TexDirEntries[i];
        if (entry.loadState == 1) {
            if (entry.image != 0) {
                zVid_Image::ReleaseIfNotDefault(entry.image);
                entry.image = 0;
            }

            if (entry.texture != 0) {
                g_zVideo_pfnTextureRecordDestroy(entry.texture);
                entry.texture = 0;
            }
        }

        if (entry.loadState == 1 || entry.loadState == 2) {
            entry.loadState = 0;
        }
    }

    g_zImage_TexDirEntryCount = 0;

    g_zVideo_pfnTextureRecordReleaseAllUploadSurfaces();

    for (int tableIndex = 0;
         tableIndex < g_zVid_PaletteRemapVariantTableCount;
         ++tableIndex) {
        free(g_zVid_PaletteRemapVariantTables[tableIndex]);
        g_zVid_PaletteRemapVariantTables[tableIndex] = 0;
    }

    unsigned short **variantTables = g_zVid_PaletteRemapVariantTables;
    g_zVid_PaletteRemapVariantTableCount = 0;
    if (variantTables != 0) {
        free(variantTables);
        g_zVid_PaletteRemapVariantTables = 0;
    }

    zVidPaletteRemapRecipe *recipes = g_zVid_PaletteRemapRecipes;
    if (recipes != 0) {
        free(recipes);
        g_zVid_PaletteRemapRecipes = 0;
    }
    g_zVid_PaletteRemapRecipeCount = 0;

    zVid_TexturePack::ShutdownBuiltinPacks();
    zVid_TexturePack::Shutdown();
    return 0;
}
} // namespace zVid_TexDir

namespace zVid_TexturePack {
/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered zVid_TexturePack::ClosePackEntry helper behavior for zVideo callers.
 */
void ClosePackEntry(
    zVidTexturePackEntry &entry
) {
    if (entry.fileHandle != 0) {
        fclose(entry.fileHandle);
        entry.fileHandle = 0;
    }

    if (entry.records != 0) {
        free(entry.records);
        entry.records = 0;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered zVid_TexturePack::FreePackEntryRecords helper behavior for zVideo callers.
 */
void FreePackEntryRecords(
    zVidTexturePackEntry &entry
) {
    if (entry.records != 0) {
        free(entry.records);
        entry.records = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-texturepack-shutdownbuiltinpacks
 * @recoil-artifact defines .text recoil:function:0x46d6b0: zVid_TexturePack::ShutdownBuiltinPacks.
 * Purpose: provide the recovered zVid_TexturePack::ShutdownBuiltinPacks behavior.
 */
void ShutdownBuiltinPacks() {
    zImage::ShutdownTextureDirectoryRuntime();

    for (int i = 0; i < g_zVid_BuiltinTexturePackCount; ++i) {
        FreePackEntryRecords(g_zVid_BuiltinTexturePacks[i]);
    }

    if (g_zVid_BuiltinTexturePacks != 0) {
        free(g_zVid_BuiltinTexturePacks);
        g_zVid_BuiltinTexturePacks = 0;
    }

    g_zVid_BuiltinTexturePackCount = 0;
}
} // namespace zVid_TexturePack

namespace zImage {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-shutdowntexturedirectoryruntime
 * @recoil-artifact defines .text recoil:function:0x46d730: zImage::ShutdownTextureDirectoryRuntime.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zImage\zimg_texture.cpp.
 * Purpose: close open built-in texture-pack file handles and return the
 * current built-in texture-pack count.
 *
 * Evidence: BN reloads g_zVid_BuiltinTexturePackCount after each iteration,
 * walks g_zVid_BuiltinTexturePacks using the zVidTexturePackEntry stride,
 * calls fclose for non-null fileHandle values, clears each closed handle, and
 * returns the last reloaded count.
 */
int ShutdownTextureDirectoryRuntime() {
    int count = g_zVid_BuiltinTexturePackCount;
    for (int i = 0; i < count; ++i) {
        zVidTexturePackEntry &entry = g_zVid_BuiltinTexturePacks[i];
        if (entry.fileHandle != 0) {
            fclose(entry.fileHandle);
            entry.fileHandle = 0;
        }
        count = g_zVid_BuiltinTexturePackCount;
    }

    return count;
}
} // namespace zImage

namespace zVid_TexturePack {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-texturepack-shutdown
 * @recoil-artifact defines .text recoil:function:0x46d780: zVid_TexturePack::Shutdown.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: release the dynamically loaded texture-pack bank.
 */
void Shutdown() {
    for (int i = 0; i < g_zVid_TexturePackCount; ++i) {
        ClosePackEntry(g_zVid_TexturePacks[i]);
    }

    if (g_zVid_TexturePacks != 0) {
        free(g_zVid_TexturePacks);
        g_zVid_TexturePacks = 0;
    }

    g_zVid_TexturePackCount = 0;
}
} // namespace zVid_TexturePack

namespace zImage {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-texdir-findorappendbypath
 * @recoil-artifact defines .text recoil:function:0x46d810: zImage::TexDir_FindOrAppendByPath.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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

} // namespace zImage

namespace zVid_Image {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-clearzeroalphapixelsinplace
 * @recoil-artifact defines .text recoil:function:0x46d870: zVid_Image::ClearZeroAlphaPixelsInPlace.
 * Purpose: provide the recovered zVid_Image::ClearZeroAlphaPixelsInPlace behavior.
 */
void __fastcall ClearZeroAlphaPixelsInPlace(
    zVidImagePartial *image
) {
    if (image->paletteMetaPacked != 0) {
        return;
    }

    const int bytesPerPixel = QueryBytesPerPixel(image);
    if (image->pixelCount <= 0) {
        return;
    }

    unsigned char *alpha = (unsigned char *)(image->alphaMap);
    if (bytesPerPixel == 1) {
        unsigned char *pixels = (unsigned char *)(image->pixels);
        for (int i = 0; i < image->pixelCount; ++i) {
            if (alpha[i] == 0) {
                pixels[i] = 0;
            }
        }
        return;
    }

    if (bytesPerPixel == 2) {
        unsigned short *pixels = (unsigned short *)(image->pixels);
        for (int i = 0; i < image->pixelCount; ++i) {
            if (alpha[i] == 0) {
                pixels[i] = 0;
            }
        }
        return;
    }

    if (bytesPerPixel == 4) {
        unsigned int *pixels = (unsigned int *)(image->pixels);
        for (int i = 0; i < image->pixelCount; ++i) {
            if (alpha[i] == 0) {
                pixels[i] = 0;
            }
        }
    }
}
} // namespace zVid_Image

namespace zImage {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-texdir-findorcreatebypath
 * @recoil-artifact defines .text recoil:function:0x46d900: zImage::TexDir_FindOrCreateByPath.
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

} // namespace zImage

namespace {
/**
 * Recovered helper: LoadTexturePackImageByName.
 * Original-source helper evidence: no standalone retail function is present;
 * recovered from address-backed callers 0x46d940 and 0x46dd30 in this source file.
 * Purpose: find and load an image record from a texture-pack entry array.
 */
zVidImagePartial *LoadTexturePackImageByName(
    zVidTexturePackEntry *entries,
    int count,
    const char *imageName,
    bool builtin
) {
    zVidImagePartial *result = 0;
    for (int i = 0; i < count && result == 0; ++i) {
        zVidTexturePackEntry *entry = &entries[i];
        if (entry->fileHandle == 0) {
            continue;
        }

        for (int recordIndex = 0; recordIndex < entry->header.recordCount && result == 0;
            ++recordIndex) {
            zVidTexturePackRecord *record = &entry->records[recordIndex];
            if (_stricmp(
                record->name,
                imageName
            ) != 0) {
                continue;
            }

            fseek(
                entry->fileHandle,
                record->fileOffset,
                SEEK_SET
            );
            result = zVid_Image::ReadFromFile(entry->fileHandle);
            if (record->paletteIndex != -1) {
                const int tableIndex = entry->paletteTableBaseIndex + record->paletteIndex;
                if (builtin) {
                    if (result->palette != 0) {
                        free(result->palette);
                        result->palette = 0;
                        result->formatFlagsPacked &= (unsigned char)(~0x80);
                    }
                    result->paletteMetaPacked = 0x100;
                }
                result->palette = g_zVid_PaletteRemapVariantTables[tableIndex];
            }
        }
    }

    return result;
}
} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-texturepack-loadimagebyname
 * @recoil-artifact defines .text recoil:function:0x46d940: zVid_TexturePack_LoadImageByName.
 * Purpose: provide the recovered zVid_TexturePack_LoadImageByName behavior.
 */
extern "C" zVidImagePartial *__fastcall zVid_TexturePack_LoadImageByName(
    const char *imageName
) {
    if (g_zVid_TexturePackCount == 0) {
        zVid_TexturePack_EnsureDefaultImagePackLoaded();
    }

    return LoadTexturePackImageByName(
        g_zVid_TexturePacks,
        g_zVid_TexturePackCount,
        imageName,
        false
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-texturepack-ensuredefaultimagepackloaded
 * @recoil-artifact defines .text recoil:function:0x46da40: zVid_TexturePack_EnsureDefaultImagePackLoaded.
 * Purpose: allocate and load the default image texture pack, with retail fallback path.
 */
extern "C" void zVid_TexturePack_EnsureDefaultImagePackLoaded() {
    if (g_zVid_TexturePackCount > 0) {
        return;
    }

    g_zVid_TexturePacks = (zVidTexturePackEntry *)(realloc(
        g_zVid_TexturePacks,
        (size_t)(g_zVid_TexturePackCount + 1) * sizeof(zVidTexturePackEntry)
    ));
    zVidTexturePackEntry *entry = &g_zVid_TexturePacks[g_zVid_TexturePackCount];
    memset(
        entry,
        0,
        sizeof(*entry)
    );
    strcpy(
        entry->filePath,
        g_zVid_DefaultImageTexturePackName
    );

    if (zVid_TexturePackEntry_LoadFromFile(entry) == 0) {
        sprintf(
            entry->filePath,
            g_zVid_DefaultImageTexturePackReadonlyNameFmt,
            g_zVid_DefaultImageTexturePackName
        );
        if (zVid_TexturePackEntry_LoadFromFile(entry) == 0) {
            return;
        }
    }

    ++g_zVid_TexturePackCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-texturepackentry-loadfromfile
 * @recoil-artifact defines .text recoil:function:0x46dae0: zVid_TexturePackEntry_LoadFromFile.
 * Purpose: load one texture-pack ZBD entry table and any palette-remap variant tables.
 */
extern "C" FILE *__fastcall zVid_TexturePackEntry_LoadFromFile(
    zVidTexturePackEntry *entry
) {
    if (g_zVid_TexturePackLoadState == 0) {
        return 0;
    }

    entry->fileHandle = zUtil_ZRDR_OpenFileResolved(
        0,
        entry->filePath,
        "rb"
    );
    if (entry->fileHandle == 0) {
        return 0;
    }

    if (fread(&entry->header, sizeof(entry->header), 1, entry->fileHandle) != 1 ||
        entry->header.fileFormat != 1) {
        fclose(entry->fileHandle);
        entry->fileHandle = 0;
        return 0;
    }

    entry->records = (zVidTexturePackRecord *)(malloc(
        (size_t)(entry->header.recordCount) * sizeof(zVidTexturePackRecord)
    ));
    if (fread(
            entry->records,
            sizeof(zVidTexturePackRecord),
            entry->header.recordCount,
            entry->fileHandle
        ) != (size_t)(entry->header.recordCount)) {
        fclose(entry->fileHandle);
        entry->fileHandle = 0;
        free(entry->records);
        entry->records = 0;
        return 0;
    }

    entry->paletteTableBaseIndex = g_zVid_PaletteRemapVariantTableCount;
    if (entry->header.paletteTableCount <= 0) {
        return entry->fileHandle;
    }

    int tableIndex = g_zVid_PaletteRemapVariantTableCount;
    g_zVid_PaletteRemapVariantTableCount += entry->header.paletteTableCount;
    g_zVid_PaletteRemapVariantTables = (unsigned short **)(realloc(
        g_zVid_PaletteRemapVariantTables,
        (size_t)(g_zVid_PaletteRemapVariantTableCount) * sizeof(unsigned short *)
    ));

    int rBits = 0;
    int gBits = 0;
    int bBits = 0;
    zVideo::PixelPack_GetRgbBits(
        &rBits,
        &gBits,
        &bBits
    );

    while (tableIndex < g_zVid_PaletteRemapVariantTableCount) {
        unsigned short *table =
            (unsigned short *)(malloc((size_t)kZVidPaletteColorCount * sizeof(unsigned short)));
        g_zVid_PaletteRemapVariantTables[tableIndex] = table;
        if (fread(
            table,
            sizeof(unsigned short),
            kZVidPaletteColorCount,
            entry->fileHandle
        ) != (size_t)kZVidPaletteColorCount) {
            fclose(entry->fileHandle);
            entry->fileHandle = 0;
            free(entry->records);
            entry->records = 0;
            return 0;
        }

        if (gBits == 5) {
            {
                for (int colorIndex = 0; colorIndex < kZVidPaletteColorCount; ++colorIndex) {
                    unsigned short *color = &table[colorIndex];
                    const unsigned short value = *color;
                    const unsigned short shifted = (unsigned short)(value >> 1);
                    const unsigned short lowXor =
                        (unsigned char)((unsigned char)(value) ^ (unsigned char)(shifted));
                    *color = (unsigned short)((lowXor & 0x1f) ^ shifted);
                }
            }
        }

        g_zVid_PaletteRemapVariantTables[tableIndex] =
            zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(
                table,
                kZVidPaletteColorCount
            );
        ++tableIndex;
    }

    return entry->fileHandle;
}

extern "C" zVidImagePartial *__fastcall
/**
 * Purpose: provide the recovered zVid_TexturePack_LoadBuiltinImageByName behavior.
 */
zVid_TexturePack_LoadBuiltinImageByName(
    const char *imageName
) {
    return LoadTexturePackImageByName(
        g_zVid_BuiltinTexturePacks,
        g_zVid_BuiltinTexturePackCount,
        imageName,
        true
    );
}

namespace zImage {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-texdir-loadpendingentries
 * @recoil-artifact defines .text recoil:function:0x46de50: zImage::TexDir_LoadPendingEntries.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-texturepack-ensurebuiltintexturepacksloaded
 * @recoil-artifact defines .text recoil:function:0x46df50: zVid_TexturePack_EnsureBuiltinTexturePacksLoaded.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: provide the recovered zVid_TexturePack_EnsureBuiltinTexturePacksLoaded behavior.
 */
extern "C" RECOIL_NO_GS void zVid_TexturePack_EnsureBuiltinTexturePacksLoaded() {
    if (g_zVid_BuiltinTexturePackCount > 0) {
        for (int i = 0; i < g_zVid_BuiltinTexturePackCount; ++i) {
            zVidTexturePackEntry *const entry = &g_zVid_BuiltinTexturePacks[i];
            if (entry->fileHandle == 0) {
                entry->fileHandle = zUtil_ZRDR_OpenFileResolved(
                    g_zImage_MissionSearchPathList,
                    entry->filePath,
                    "rb"
                );
            }
        }
        return;
    }

    char filePath[0x20];
    int probeWasRendererMemory = 0;
    int candidateSize = 8;
    int totalBytes = 0;
    int freeBytes = 0;
    char *const archiveExtension = &g_zVid_TextureArchiveMaxName[11];

    if (g_zVideo_pfnQueryTextureMemoryBytes(-1, &totalBytes, &freeBytes) != 0 &&
        g_zVideo_ActiveRendererPath != 0) {
        candidateSize = (unsigned int)(totalBytes) >> 20;
        sprintf(
            filePath,
            g_zVid_TextureArchiveRendererSizedNameFmt,
            g_zVid_TextureArchiveStem,
            candidateSize,
            archiveExtension
        );
        probeWasRendererMemory = 1;
    } else {
        switch (*g_zImage_TextureMemoryOption) {
        case 1:
            sprintf(
                filePath,
                g_zVid_TextureArchiveSize8Fmt,
                g_zVid_TextureArchiveStem,
                archiveExtension
            );
            candidateSize = 8;
            break;
        case 2:
            sprintf(
                filePath,
                g_zVid_TextureArchiveSize6Fmt,
                g_zVid_TextureArchiveStem,
                archiveExtension
            );
            candidateSize = 6;
            break;
        case 3:
            sprintf(
                filePath,
                g_zVid_TextureArchiveSize4Fmt,
                g_zVid_TextureArchiveStem,
                archiveExtension
            );
            candidateSize = 4;
            break;
        case 4:
            sprintf(
                filePath,
                g_zVid_TextureArchiveSize2Fmt,
                g_zVid_TextureArchiveStem,
                archiveExtension
            );
            candidateSize = 2;
            break;
        default:
            sprintf(
                filePath,
                "%s",
                g_zVid_TextureArchiveMaxName
            );
            candidateSize = 8;
            break;
        }
    }

    g_zVid_BuiltinTexturePacks = (zVidTexturePackEntry *)(realloc(
        g_zVid_BuiltinTexturePacks,
        (size_t)(g_zVid_BuiltinTexturePackCount + 1) * sizeof(zVidTexturePackEntry)
    ));
    zVidTexturePackEntry *const entry = &g_zVid_BuiltinTexturePacks[g_zVid_BuiltinTexturePackCount];
    memset(
        entry,
        0,
        sizeof(*entry)
    );
    strcpy(
        entry->filePath,
        filePath
    );

    if (zVid_TexturePackEntry_LoadFromFile(entry) == 0) {
        {
            for (int size = candidateSize; size >= -1; --size) {
                if (size > 0) {
                    if (probeWasRendererMemory != 0) {
                        sprintf(
                            filePath,
                            g_zVid_TextureArchiveRendererSizedNameFmt,
                            g_zVid_TextureArchiveStem,
                            size,
                            archiveExtension
                        );
                    } else {
                        sprintf(
                            filePath,
                            g_zVid_TextureArchiveSizedNameFmt,
                            g_zVid_TextureArchiveStem,
                            size,
                            archiveExtension
                        );
                    }
                } else if (size == 0) {
                    sprintf(
                        filePath,
                        "%s",
                        g_zVid_TextureArchiveMaxName
                    );
                } else {
                    sprintf(
                        filePath,
                        g_zVid_TextureArchiveNameFmt,
                        g_zVid_TextureArchiveStem,
                        archiveExtension
                    );
                }

                strcpy(
                    entry->filePath,
                    filePath
                );
                if (zVid_TexturePackEntry_LoadFromFile(entry) != 0) {
                    break;
                }
            }
        }
    }

    if (entry->fileHandle != 0) {
        ++g_zVid_BuiltinTexturePackCount;
    }
}

namespace zImage {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-invalidateloadedvariantchain
 * @recoil-artifact defines .text recoil:function:0x46e250: zImage::InvalidateLoadedVariantChain.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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
} // namespace zImage

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-texdirentrypartial-getvariantimageatindex
 * @recoil-artifact defines .text recoil:function:0x46e290: zImage_TexDirEntryPartial::GetVariantImageAtIndex.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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

namespace zImage {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-setpathextension
 * @recoil-artifact defines .text recoil:function:0x46e2c0: zImage::SetPathExtension.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-texdirsetbasenamefrompath
 * @recoil-artifact defines .text recoil:function:0x46e380: zImage::TexDirSetBaseNameFromPath.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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
} // namespace zImage

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-texdirentry-buildmipchain
 * @recoil-artifact defines .text recoil:function:0x46e3e0: zImage_TexDirEntry::BuildMipChain.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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

namespace zVid_PaletteRemap {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-paletteremap-applyrecipetopalettevariant
 * @recoil-artifact defines .text recoil:function:0x46e4e0: zVid_PaletteRemap::ApplyRecipeToPaletteVariant.
 * Purpose: blend one source palette toward a recipe endpoint variant and pack 16-bit colors.
 */
void __fastcall ApplyRecipeToPaletteVariant(
    zVidPaletteRemapRecipe *recipe,
    unsigned short *sourceColors,
    int colorCount,
    int variantIndex,
    unsigned short *destColors
) {
    int rBits;
    int gBits;
    int bBits;
    zVideo::PixelPack_GetRgbBits(
        &rBits,
        &gBits,
        &bBits
    );

    const float variantWeight = (float)(variantIndex) * 0.0322580636f;
    const float inverseVariantWeight = 1.0f - variantWeight;
    float r;
    float g;
    float b;
    zVideo_ColorRgbFloat color;

    while (colorCount > 0) {
        const int packed = *sourceColors;
        r = 0.0f;
        g = 0.0f;
        if (gBits == 5) {
            const int red = packed & 0x7c00;
            const int green = packed & 0x03e0;
            r = (float)(red) * 3.15020152e-05f;
            g = (float)(green) * 0.00100806449f;
        } else {
            const int red = packed & 0xf800;
            const int green = packed & 0x07e0;
            r = (float)(red) * 1.57510076e-05f;
            g = (float)(green) * 0.000496031775f;
        }
        const int blue = packed & 0x001f;
        b = (float)(blue) * 0.0322580636f;

        color.r = ((recipe->color0R - r) * inverseVariantWeight * recipe->color0Strength +
                      (recipe->color1R - r) * variantWeight * recipe->color1Strength + r) *
                  255.0f;
        color.g = ((recipe->color1G - g) * variantWeight * recipe->color1Strength +
                      (recipe->color0G - g) * inverseVariantWeight * recipe->color0Strength + g) *
                  255.0f;
        color.b = ((recipe->color1B - b) * variantWeight * recipe->color1Strength +
                      (recipe->color0B - b) * inverseVariantWeight * recipe->color0Strength + b) *
                  255.0f;

        *destColors = zVid_PackColorRgbFloats(&color);
        ++sourceColors;
        ++destColors;
        --colorCount;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-paletteremap-findrecipeindex
 * @recoil-artifact defines .text recoil:function:0x46e680: zVid_PaletteRemap::FindRecipeIndex.
 * Purpose: find an existing palette-remap recipe with the same endpoint colors and strengths.
 *
 * Evidence: BN scans g_zVid_PaletteRemapRecipes and compares the eight
 * zVidPaletteRemapRecipe float fields in retail field order, including the
 * color0Strength check before the color1 RGB fields.
 */
int __fastcall FindRecipeIndex(
    zVidPaletteRemapRecipe *recipe
) {
    for (int i = 0; i < g_zVid_PaletteRemapRecipeCount; ++i) {
        zVidPaletteRemapRecipe *candidate = &g_zVid_PaletteRemapRecipes[i];
        if (recipe->color0R == candidate->color0R && recipe->color0G == candidate->color0G &&
            recipe->color0B == candidate->color0B &&
            recipe->color0Strength == candidate->color0Strength &&
            recipe->color1R == candidate->color1R && recipe->color1G == candidate->color1G &&
            recipe->color1B == candidate->color1B &&
            recipe->color1Strength == candidate->color1Strength) {
            return i;
        }
    }

    return -1;
}
} // namespace zVid_PaletteRemap

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-paletteremap-buildpalettevariant
 * @recoil-artifact defines .text recoil:function:0x46e720: zVid_PaletteRemap_BuildPaletteVariant.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Source file evidence: Binary Ninja function source comment.
 * Purpose: Add a palette-remap recipe and rebuild existing palette variant tables.
 */
extern "C" int __fastcall zVid_PaletteRemap_BuildPaletteVariant(
    zVidPaletteRemapRecipe *recipe
) {
    const int existingIndex = zVid_PaletteRemap::FindRecipeIndex(recipe);
    if (existingIndex >= 0) {
        return existingIndex;
    }

    ++g_zVid_PaletteRemapRecipeCount;
    g_zVid_PaletteRemapRecipes = (zVidPaletteRemapRecipe *)(realloc(
        g_zVid_PaletteRemapRecipes,
        (size_t)(g_zVid_PaletteRemapRecipeCount) * sizeof(zVidPaletteRemapRecipe)
    ));
    g_zVid_PaletteRemapRecipes[g_zVid_PaletteRemapRecipeCount - 1] = *recipe;

    int i = 0;
    zImage_TexDirEntryPartial *texDirEntry = g_zImage_TexDirEntries;
    for (; i < g_zImage_TexDirEntryCount; ++i, ++texDirEntry) {
        zVidImagePartial *image = texDirEntry->image;
        if (image->paletteMetaPacked == 0 || (image->formatFlagsPacked & 0x10) != 0) {
            continue;
        }

        image->palette = realloc(
            image->palette,
            (size_t)(
                (g_zVid_PaletteRemapRecipeCount * kZVidPaletteRemapColorsPerRecipe) +
                kZVidPaletteColorCount
            ) * sizeof(unsigned short)
        );
        {
            for (int variant = 0; variant < kZVidPaletteRemapVariantCount; ++variant) {
                zVid_PaletteRemap::ApplyRecipeToPaletteVariant(
                    recipe,
                    (unsigned short *)(image->palette),
                    image->paletteMetaPacked,
                    variant,
                    &((unsigned short *)(image->palette))[(
                        ((g_zVid_PaletteRemapRecipeCount - 1) *
                            kZVidPaletteRemapVariantCount) +
                        variant +
                        1
                    ) * kZVidPaletteColorCount]
                );
            }
        }
    }

    for (int tableIndex = 0; tableIndex < g_zVid_PaletteRemapVariantTableCount; ++tableIndex) {
        unsigned short *oldTable = g_zVid_PaletteRemapVariantTables[tableIndex];
        g_zVid_PaletteRemapVariantTables[tableIndex] =
            (unsigned short *)(realloc(
                oldTable,
                (size_t)(
                    (g_zVid_PaletteRemapRecipeCount * kZVidPaletteRemapColorsPerRecipe) +
                kZVidPaletteColorCount
            ) * sizeof(unsigned short)
        ));

        {
            for (int variant = 0; variant < kZVidPaletteRemapVariantCount; ++variant) {
                zVid_PaletteRemap::ApplyRecipeToPaletteVariant(
                    recipe,
                    g_zVid_PaletteRemapVariantTables[tableIndex],
                    kZVidPaletteColorCount,
                    variant,
                    &g_zVid_PaletteRemapVariantTables[tableIndex][(
                        ((g_zVid_PaletteRemapRecipeCount - 1) *
                            kZVidPaletteRemapVariantCount) +
                        variant +
                        1
                    ) * kZVidPaletteColorCount]
                );
            }
        }

        {
            zImage_TexDirEntryPartial *texDirEntry = g_zImage_TexDirEntries;
            for (int i = 0; i < g_zImage_TexDirEntryCount; ++i, ++texDirEntry) {
                zVidImagePartial *image = texDirEntry->image;
                if (image->palette == oldTable) {
                    image->palette = g_zVid_PaletteRemapVariantTables[tableIndex];
                }
            }
        }
    }

    return g_zVid_PaletteRemapRecipeCount - 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-paletteremap-buildallrecipevariantsforpalette
 * @recoil-artifact defines .text recoil:function:0x46e8d0: zVid_PaletteRemap_BuildAllRecipeVariantsForPalette.
 * Purpose: expand a palette with all variants for every active palette-remap recipe.
 */
extern "C" unsigned short *__fastcall zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(
    unsigned short *palette,
    int colorCount
) {
    if (g_zVid_PaletteRemapRecipeCount == 0) {
        return palette;
    }

    unsigned short *result = (unsigned short *)(realloc(
        palette,
        (size_t)(
            (g_zVid_PaletteRemapRecipeCount * kZVidPaletteRemapColorsPerRecipe) +
            kZVidPaletteColorCount
        ) * sizeof(unsigned short)
    ));

    int recipeIndex = 0;
    if (g_zVid_PaletteRemapRecipeCount > 0) {
        unsigned short *dest = &result[kZVidPaletteColorCount];
        do {
            int variant = 0;
            do {
                zVid_PaletteRemap::ApplyRecipeToPaletteVariant(
                    &g_zVid_PaletteRemapRecipes[recipeIndex],
                    result,
                    colorCount,
                    variant,
                    dest
                );
                ++variant;
                dest += kZVidPaletteColorCount;
            } while (variant < kZVidPaletteRemapVariantCount);
            ++recipeIndex;
        } while (recipeIndex < g_zVid_PaletteRemapRecipeCount);
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-paletteremap-findrecipeindexfromrgb
 * @recoil-artifact defines .text recoil:function:0x46e960: zVid_PaletteRemap_FindRecipeIndexFromRgb.
 * Purpose: Build the black-to-RGB palette-remap recipe used by renderer shade lookups and find its existing recipe index.
 *
 * Evidence: BN constructs a stack zVidPaletteRemapRecipe with zero color0
 * endpoint fields, RGB color1 fields copied from the input, color0Strength
 * zero, and color1Strength 1.0f before delegating to FindRecipeIndex.
 */
extern "C" int __fastcall zVid_PaletteRemap_FindRecipeIndexFromRgb(
    zColorRgb *rgb
) {
    zVidPaletteRemapRecipe recipe;
    recipe.color0R = 0.0f;
    recipe.color0G = 0.0f;
    recipe.color0B = 0.0f;
    recipe.color0Strength = 0.0f;
    recipe.color1R = rgb->red;
    recipe.color1G = rgb->green;
    recipe.color1B = rgb->blue;
    recipe.color1Strength = 1.0f;
    return zVid_PaletteRemap::FindRecipeIndex(&recipe);
}

namespace zVid_Image {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-resamplesquare
 * @recoil-artifact defines .text recoil:function:0x46e9b0: zVid_Image::ResampleSquare.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: resamples an owned 16-bit zVid image into a square nearest-source
 * pixel buffer and matching alpha map when present.
 *
 * Evidence: BN assembly allocates sideLength * sideLength pixels and only
 * allocates a replacement alpha map when the old image had one, copies pixels
 * and alpha bytes through _ftol-truncated source coordinates, frees the old
 * buffers, and installs the square dimensions and replacement buffers.
 */
void __fastcall ResampleSquare(
    zVidImagePartial *image,
    int sideLength
) {
    const float inverseSideLength = 1.0f / (float)(sideLength);
    unsigned short *oldPixels = (unsigned short *)(image->pixels);
    char *oldAlphaMap = image->alphaMap;
    const int sourceWidth = image->width;
    const int sourceHeight = image->height;
    const float xScale = (float)(sourceWidth)*inverseSideLength;
    const float yScale = (float)(sourceHeight)*inverseSideLength;

    const unsigned int pixelCount = (unsigned int)(sideLength * sideLength);
    unsigned short *newPixels = (unsigned short *)(malloc(pixelCount * sizeof(unsigned short)));
    char *newAlphaMap = 0;
    if (oldAlphaMap != 0) {
        newAlphaMap = (char *)(malloc(pixelCount));
    }

    {
        for (int dstY = 0; dstY < sideLength; ++dstY) {
            const int srcY = (int)((float)(dstY)*yScale);
            unsigned short *newPixelCursor = &newPixels[dstY * sideLength];
            char *newAlphaCursor = newAlphaMap != 0 ? &newAlphaMap[dstY * sideLength] : 0;

            {
                for (int dstX = 0; dstX < sideLength; ++dstX) {
                    const int srcX = (int)((float)(dstX)*xScale);
                    const int sourceIndex = srcY * sourceWidth + srcX;
                    *newPixelCursor++ = oldPixels[sourceIndex];
                    if (oldAlphaMap != 0) {
                        *newAlphaCursor++ = oldAlphaMap[sourceIndex];
                    }
                }
            }
        }
    }

    free(image->pixels);
    image->height = (short)(sideLength);
    image->width = (short)(sideLength);
    image->pixels = newPixels;
    if (oldAlphaMap != 0) {
        free(oldAlphaMap);
        image->alphaMap = newAlphaMap;
    }
}
} // namespace zVid_Image

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-init
 * @recoil-artifact defines .text recoil:function:0x46eb20: zImage_Init.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-shutdownsubsystem
 * @recoil-artifact defines .text recoil:function:0x46eb90: zImage::ShutdownSubsystem.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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

namespace zImg {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimg-init
 * @recoil-artifact defines .text recoil:function:0x46eba0: zImg::Init.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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

namespace zImage {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-shutdown
 * @recoil-artifact defines .text recoil:function:0x46ebb0: zImage::Shutdown.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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
} // namespace zImage

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-initmissionresources
 * @recoil-artifact defines .text recoil:function:0x46ebd0: zImage_InitMissionResources.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
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

namespace zVid_Image {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-create
 * @recoil-artifact defines .text recoil:function:0x46ec00: zVid_Image::Create.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: allocate and zero-initialize a zVid image record.
 */
zVidImagePartial *Create() {
    zVidImagePartial *image = (zVidImagePartial *)(malloc(sizeof(zVidImagePartial)));
    memset(
        image,
        0,
        sizeof(zVidImagePartial)
    );
    return image;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-querybytesperpixel
 * @recoil-artifact defines .text recoil:function:0x46ec20: zVid_Image::QueryBytesPerPixel.
 * Purpose: provide the recovered zVid_Image::QueryBytesPerPixel behavior.
 */
int __fastcall QueryBytesPerPixel(
    zVidImagePartial *image
) {
    return (image->formatFlagsPacked & 1) != 0 ? 2 : 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-setheaderflagsbyte
 * @recoil-artifact defines .text recoil:function:0x46ec30: zVid_Image::SetHeaderFlagsByte.
 * Purpose: provide the recovered zVid_Image::SetHeaderFlagsByte behavior.
 */
int __fastcall SetHeaderFlagsByte(
    zVidImagePartial *image,
    unsigned char flags
) {
    image->headerFlagsByte = flags;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-querypixeldatabytes
 * @recoil-artifact defines .text recoil:function:0x46ec40: zVid_Image::QueryPixelDataBytes.
 * Purpose: provide the recovered zVid_Image::QueryPixelDataBytes behavior.
 */
int __fastcall QueryPixelDataBytes(
    zVidImagePartial *image
) {
    if (image->paletteMetaPacked != 0) {
        return image->pixelCount;
    }

    return QueryBytesPerPixel(image) * image->pixelCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-setformatcode
 * @recoil-artifact defines .text recoil:function:0x46ec60: zVid_Image::SetFormatCode.
 * Purpose: provide the recovered zVid_Image::SetFormatCode behavior.
 */
int __fastcall SetFormatCode(
    zVidImagePartial *image,
    unsigned char formatCode
) {
    image->formatFlagsPacked = formatCode;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-setpixels
 * @recoil-artifact defines .text recoil:function:0x46ec70: zVid_Image_SetPixels.
 * Purpose: provide the recovered zVid_Image_SetPixels behavior.
 */
extern "C" int __fastcall zVid_Image_SetPixels(
    zVidImagePartial *image,
    void *pixels,
    char *alphaMap
) {
    image->pixels = pixels;
    image->alphaMap = alphaMap;
    if (alphaMap != 0) {
        image->formatFlagsPacked |= 0x02u;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-setsize
 * @recoil-artifact defines .text recoil:function:0x46ec90: zVid_Image::SetSize.
 * Purpose: provide the recovered zVid_Image::SetSize behavior.
 */
int __fastcall SetSize(
    zVidImagePartial *image,
    short width,
    short height
) {
    image->width = width;
    image->height = height;
    image->pixelCount = (int)(width) * (int)(height);
    image->pitchWords = width;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-destroy
 * @recoil-artifact defines .text recoil:function:0x46ecc0: zVid_Image::Destroy.
 * Retail literal-backed physical source block: GameZRecoil/zImage/zimg_texture.cpp.
 * Purpose: release an image object's surface-dependent state, owned buffers,
 * and allocation, returning zero for both null and non-null images.
 *
 * Evidence: BN tests the image pointer, calls the current-device surface
 * provider only when image->surface is non-null, then releases owned buffers
 * and frees the image allocation before returning 0.
 */
int __fastcall Destroy(
    zVidImagePartial *image
) {
    if (image != 0) {
        if (image->surface != 0) {
            g_zVideo_pfnImageEnsureSurfaceForCurrentDevice(image);
        }

        ReleaseOwnedBuffers(image);
        free(image);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-releaseownedbuffers
 * @recoil-artifact defines .text recoil:function:0x46ecf0: zVid_Image::ReleaseOwnedBuffers.
 * Purpose: provide the recovered zVid_Image::ReleaseOwnedBuffers behavior.
 */
void __fastcall ReleaseOwnedBuffers(
    zVidImagePartial *image
) {
    void(__cdecl * freeProc)(void *) = free;

    if (image->pixels != 0 && (image->formatFlagsPacked & 0x20) != 0) {
        freeProc(image->pixels);
        image->pixels = 0;
        image->formatFlagsPacked &= (unsigned char)(~0x20);
    }

    if (image->alphaMap != 0 && (image->formatFlagsPacked & 0x40) != 0) {
        freeProc(image->alphaMap);
        image->alphaMap = 0;
        image->formatFlagsPacked &= (unsigned char)(~0x40);
    }

    if (image->palette != 0 && (image->formatFlagsPacked & 0x80) != 0 &&
        (image->formatFlagsPacked & 0x10) == 0) {
        freeProc(image->palette);
        image->palette = 0;
        image->formatFlagsPacked &= (unsigned char)(~0x80);
    }
}

namespace {
struct zVidImageFileHeader {
    unsigned char formatCode;
    unsigned char unknown_01[3];
    short width;
    short height;
    unsigned char headerFlags;
    unsigned char unknown_09[3];
    short textureAddressFlagsPacked;
    short paletteMeta;
};

RECOIL_STATIC_ASSERT(sizeof(zVidImageFileHeader) == 0x10);
} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-readheader
 * @recoil-artifact defines .text recoil:function:0x46ed70: zVid_Image::ReadHeader.
 * Purpose: provide the recovered zVid_Image::ReadHeader behavior.
 */
int __fastcall ReadHeader(
    FILE *file,
    zVidImagePartial *image
) {
    if (file == 0 || image == 0) {
        return -1;
    }

    zVidImageFileHeader header = {0};
    fread(
        &header,
        0x10,
        1,
        file
    );
    SetSize(
        image,
        header.width,
        header.height
    );
    SetFormatCode(
        image,
        header.formatCode
    );
    SetHeaderFlagsByte(
        image,
        header.headerFlags
    );
    image->paletteMetaPacked = header.paletteMeta;
    image->textureAddressFlagsPacked = header.textureAddressFlagsPacked;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-readdata
 * @recoil-artifact defines .text recoil:function:0x46ede0: zVid_Image::ReadData.
 * Purpose: provide the recovered zVid_Image::ReadData behavior.
 */
int __fastcall ReadData(
    FILE *file,
    zVidImagePartial *image,
    int bytesPerPixel
) {
    if (bytesPerPixel == 0) {
        bytesPerPixel = QueryBytesPerPixel(image);
    }

    if (bytesPerPixel != QueryBytesPerPixel(image)) {
        if (bytesPerPixel <= QueryBytesPerPixel(image)) {
            return -1;
        }
        return 0;
    }

    const int pixelBytes = QueryPixelDataBytes(image);
    if (fread(
        image->pixels,
        1,
        pixelBytes,
        file
    ) != (size_t)(pixelBytes)) {
        return -1;
    }

    if ((image->formatFlagsPacked & 0x08) != 0) {
        image->alphaMap = (char *)(malloc((size_t)(image->pixelCount)));
        if (fread(
            image->alphaMap,
            1,
            image->pixelCount,
            file
        ) != (size_t)(image->pixelCount)) {
            image->formatFlagsPacked |= 0x40;
            return -1;
        }
        image->formatFlagsPacked |= 0x40;
    }

    if ((image->formatFlagsPacked & 0x10) == 0 && image->paletteMetaPacked != 0) {
        const int paletteBytes = bytesPerPixel * image->paletteMetaPacked;
        image->palette = malloc((size_t)(paletteBytes));
        if (fread(
            image->palette,
            1,
            paletteBytes,
            file
        ) != (size_t)(paletteBytes)) {
            image->formatFlagsPacked |= 0x80;
            return -1;
        }
        image->formatFlagsPacked |= 0x80;
    }

    if (bytesPerPixel == 2 && (image->formatFlagsPacked & 0x10) == 0) {
        int rBits = 0;
        int gBits = 0;
        int bBits = 0;
        zVideo::PixelPack_GetRgbBits(
            &rBits,
            &gBits,
            &bBits
        );
        if (gBits == 5) {
            unsigned short *colors = image->paletteMetaPacked == 0
                                         ? (unsigned short *)(image->pixels)
                                         : (unsigned short *)(image->palette);
            int count =
                image->paletteMetaPacked == 0 ? image->pixelCount : image->paletteMetaPacked;
            while (count > 0) {
                const unsigned short value = *colors;
                *colors = (unsigned short)(((value >> 1) & 0x7fe0) | (value & 0x1f));
                ++colors;
                --count;
            }
        }
    }

    if (image->paletteMetaPacked != 0) {
        image->palette = zVid_PaletteRemap_BuildAllRecipeVariantsForPalette(
            (unsigned short *)(image->palette),
            image->paletteMetaPacked
        );
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zvid-image-readfromfile
 * @recoil-artifact defines .text recoil:function:0x46ef70: zVid_Image::ReadFromFile.
 * Purpose: provide the recovered zVid_Image::ReadFromFile behavior.
 */
zVidImagePartial *__fastcall ReadFromFile(
    FILE *file
) {
    zVidImagePartial *image = Create();
    if (ReadHeader(
        file,
        image
    ) != 0) {
        return 0;
    }

    image->pixels = malloc(QueryPixelDataBytes(image));
    ReadData(
        file,
        image,
        0
    );
    image->formatFlagsPacked |= 0x20;
    return image;
}
} // namespace zVid_Image

/* Source-layout blocker: address-backed bodies below do not belong to the assigned contiguous ledger rows.
 * They are preserved here because their proven physical owner is outside this worker scope or still unresolved.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zimage-zimg-texture-zimage-font-blitstringtoactivetarget
 * @recoil-artifact defines .text recoil:function:0x4c7f00: zImage_Font::BlitStringToActiveTarget.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zImage\zimg_fonts.cpp.
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
