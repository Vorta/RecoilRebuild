// Checked-in focused native smoke translation unit, formerly extracted from zgame_tests.cpp.
// Emits only the zDi material helper/current variant smokes needed by functional manifests.

#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zUtil/zutil.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "opt_catalog.h"
#include "zclass.h"
#include "zclip_alt.h"
#include "zdi.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <io.h>
#include <limits>


namespace {
int g_damageMaskUploadLockCount = 0;
int g_damageMaskUploadUnlockCount = 0;
int g_damageMaskUploadFinalizeCount = 0;
unsigned short *g_damageMaskUploadPixels = nullptr;
int g_damageMaskUploadPitchBytes = 0;
zVideo_TextureRecordPartial *g_damageMaskLastTextureRecord = nullptr;
int g_zgameFogColorUpdateCount = 0;
int g_modelRefLerpCallbackCount = 0;
void *g_modelRefLerpLastCallbackCtx = nullptr;
int g_zclassUpdateBucketCallbackCount = 0;
zClass_NodePartial *g_zclassUpdateBucketLastNode = nullptr;
int g_zclassUpdateBucketDeferredDuringCallback = -1;
int g_zclassRenderTraverseCallCount = 0;
zClass_NodePartial *g_zclassRenderTraverseNodes[16] = {};
int g_zclassRenderTraverseClipMasks[16] = {};
float g_zclassRenderTraverseAlphaScales[16] = {};
int g_zclassRenderTraverseVertexAlphaEnabled[16] = {};

void TestZGameUpdateFogColor(void) {
    ++g_zgameFogColorUpdateCount;
}

void __fastcall TestModelRefLerpCallback(void *callbackCtx) {
    ++g_modelRefLerpCallbackCount;
    g_modelRefLerpLastCallbackCtx = callbackCtx;
}

int __fastcall TestZClassUpdateBucketCallback(zClass_NodePartial *node) {
    ++g_zclassUpdateBucketCallbackCount;
    g_zclassUpdateBucketLastNode = node;
    g_zclassUpdateBucketDeferredDuringCallback = g_zClass_DeferredProcessingEnabled;
    return 0;
}

void __fastcall TestZClassRenderTraverseCallback(zClass_NodePartial *node, int clipMask) {
    if (g_zclassRenderTraverseCallCount < 16) {
        g_zclassRenderTraverseNodes[g_zclassRenderTraverseCallCount] = node;
        g_zclassRenderTraverseClipMasks[g_zclassRenderTraverseCallCount] = clipMask;
        g_zclassRenderTraverseAlphaScales[g_zclassRenderTraverseCallCount] =
            gModel_RenderAlphaScaleCurrent;
        g_zclassRenderTraverseVertexAlphaEnabled[g_zclassRenderTraverseCallCount] =
            gModel_RenderVertexAlphaEnabled;
    }
    ++g_zclassRenderTraverseCallCount;
}

int g_zmodelReleaseTextureUploadCount = 0;
zVideo_TextureRecordPartial *g_zmodelReleaseTextureUploadLast = nullptr;

void __fastcall TestReleaseTextureUploadSurfaceRef(
    zVideo_TextureRecordPartial *textureRecord) {
    ++g_zmodelReleaseTextureUploadCount;
    g_zmodelReleaseTextureUploadLast = textureRecord;
}

int __fastcall TextureMemoryQueryMissingStub(int, int *, int *) {
    return 0;
}

int __fastcall DamageMaskLockUploadStub(zVideo_TextureRecordPartial *textureRecord,
                                             void **outPixels, int *outPitchBytes) {
    ++g_damageMaskUploadLockCount;
    g_damageMaskLastTextureRecord = textureRecord;
    *outPixels = g_damageMaskUploadPixels;
    *outPitchBytes = g_damageMaskUploadPitchBytes;
    return 1;
}

void __fastcall DamageMaskUnlockUploadStub(zVideo_TextureRecordPartial *textureRecord) {
    ++g_damageMaskUploadUnlockCount;
    g_damageMaskLastTextureRecord = textureRecord;
}

void __fastcall DamageMaskFinalizeUploadStub(zVideo_TextureRecordPartial *textureRecord,
                                                  void *, void *) {
    ++g_damageMaskUploadFinalizeCount;
    g_damageMaskLastTextureRecord = textureRecord;
}

void WriteU32(HANDLE file, std::uint32_t value) {
    DWORD written = 0;
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void WriteBytes(HANDLE file, const char *text, std::uint32_t length) {
    DWORD written = 0;
    WriteFile(file, text, length, &written, nullptr);
}

void WriteZrdStringNode(HANDLE file, const char *text) {
    WriteU32(file, zReader::ZRDR_NODE_STRING);
    WriteU32(file, static_cast<std::uint32_t>(std::strlen(text)));
    WriteBytes(file, text, static_cast<std::uint32_t>(std::strlen(text)));
}

void WriteZrdIntNode(HANDLE file, std::int32_t value) {
    WriteU32(file, zReader::ZRDR_NODE_INT);
    WriteU32(file, static_cast<std::uint32_t>(value));
}

void WriteZrdFloatNode(HANDLE file, float value) {
    DWORD written = 0;
    WriteU32(file, zReader::ZRDR_NODE_FLOAT);
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void WriteZrdNamedStringNode(HANDLE file, const char *name, const char *value) {
    WriteZrdStringNode(file, name);
    WriteZrdStringNode(file, value);
}

void WriteZrdNamedFloatNode(HANDLE file, const char *name, float value) {
    WriteZrdStringNode(file, name);
    WriteZrdFloatNode(file, value);
}

void WriteZrdNamedIntArray(HANDLE file, const char *name, std::int32_t value) {
    WriteZrdStringNode(file, name);
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 2);
    WriteZrdIntNode(file, value);
}

void WriteZrdNamedDirectInt(HANDLE file, const char *name, std::int32_t value) {
    WriteZrdStringNode(file, name);
    WriteZrdIntNode(file, value);
}

void WriteZrdNamedColorArray(HANDLE file, const char *name, std::int32_t red,
                             std::int32_t green, std::int32_t blue) {
    WriteZrdStringNode(file, name);
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 4);
    WriteZrdIntNode(file, red);
    WriteZrdIntNode(file, green);
    WriteZrdIntNode(file, blue);
}

void WriteZrdNamedFloatPairArray(HANDLE file, const char *name, float first, float second) {
    WriteZrdStringNode(file, name);
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 3);
    WriteZrdFloatNode(file, first);
    WriteZrdFloatNode(file, second);
}

void FreeHudWeatherFxForTest(HudUiElement *element) {
    if (element == nullptr) {
        return;
    }

    HudWeatherFx *const weatherFx = static_cast<HudWeatherFx *>(element);
    if (weatherFx->softwareImage != nullptr) {
        char *const alphaMap = weatherFx->softwareImage->alphaMap;
        zVid_Image::Destroy(weatherFx->softwareImage);
        if (alphaMap != nullptr) {
            std::free(alphaMap);
        }
    }
    ::operator delete(weatherFx->particleQuads);
    ::operator delete(weatherFx->particlePositions[0]);
    ::operator delete(weatherFx->particlePositions[1]);
    ::operator delete(weatherFx);
}

bool EnterSupportDirectoryForRetailZbdTest(char *oldDir, DWORD oldDirSize) {
    if (GetCurrentDirectoryA(oldDirSize, oldDir) == 0) {
        return false;
    }

    const char *candidates[] = {
        "support",
        "..\\..\\..\\..\\support",
    };
    for (int i = 0; i < static_cast<int>(sizeof(candidates) / sizeof(candidates[0])); ++i) {
        const DWORD attributes = GetFileAttributesA(candidates[i]);
        if (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            SetCurrentDirectoryA(candidates[i]) != 0) {
            return true;
        }
    }

    return false;
}

bool ExistingNonEmptyFileForTest(const char *path) {
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &data) == 0) {
        return false;
    }
    return (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 &&
        (data.nFileSizeHigh != 0 || data.nFileSizeLow != 0);
}

bool RetailMissionScriptFixturesAvailableForTest(void) {
    const char *const requiredFiles[] = {
        "support\\initm1.gw",
        "support\\initm3.gw",
        "support\\commonm1.gw",
        "support\\loadm1.gw",
        "support\\tex_fxm1.gw",
        "support\\commonm3.gw",
        "support\\loadm3.gw",
        "support\\tex_fxm3.gw",
        "m1.gs",
        "m1_zbd.gs",
        "m3.gs",
        "m3_zbd.gs",
    };
    for (int index = 0;
         index < static_cast<int>(sizeof(requiredFiles) / sizeof(requiredFiles[0]));
         ++index) {
        if (!ExistingNonEmptyFileForTest(requiredFiles[index])) {
            return false;
        }
    }
    return true;
}

bool EnterRetailSupportScriptsDirectoryForTest(char *oldDir, DWORD oldDirSize) {
    if (GetCurrentDirectoryA(oldDirSize, oldDir) == 0) {
        return false;
    }

    const char *candidates[] = {
        "support\\scripts",
        "..\\support\\scripts",
        "..\\..\\support\\scripts",
        "..\\..\\..\\support\\scripts",
        "..\\..\\..\\..\\support\\scripts",
    };
    for (int i = 0; i < static_cast<int>(sizeof(candidates) / sizeof(candidates[0])); ++i) {
        const DWORD attributes = GetFileAttributesA(candidates[i]);
        if (attributes != INVALID_FILE_ATTRIBUTES &&
            (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            SetCurrentDirectoryA(candidates[i]) != 0) {
            if (RetailMissionScriptFixturesAvailableForTest()) {
                return true;
            }
            if (SetCurrentDirectoryA(oldDir) == 0) {
                return false;
            }
        }
    }

    return false;
}

std::int32_t FloatBitsForTest(float value) {
    std::int32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    return raw;
}

std::int32_t __fastcall zclass_test_node_type_0x42(zClass_NodePartial *node) {
    return node->nodeType == 0x42 ? 1 : 0;
}

float __fastcall zclass_damage_timer_stub(void *context, float damageAmount) {
    *static_cast<float *>(context) = damageAmount;
    return damageAmount + 1.0f;
}

zProjectedPoint g_drawPointLastPoint{};
std::uint32_t g_drawPointLastColor = 0;
std::int32_t g_drawPointLastCount = 0;
std::int32_t g_drawPointCallCount = 0;
std::int32_t g_submitFlatCallCount = 0;
std::int32_t g_submitFlatLastVertexCount = 0;
std::int32_t g_submitFlatLastAlpha = 0;
std::uint32_t g_submitFlatLastColor = 0;
std::uint32_t g_submitFlatLastRenderParam = 0;
std::int32_t g_submitFlatLastQueueMode = 0;
zVideo_XyzVertex g_submitFlatLastVerts[0x40]{};

void __fastcall zmodel_draw_point_color16_stub(zVideo_XyzVertex *point,
                                                    std::uint32_t packedColor16,
                                                    std::int32_t pointCount) {
    g_drawPointLastPoint = *(zProjectedPoint *)(point);
    g_drawPointLastColor = packedColor16;
    g_drawPointLastCount = pointCount;
    ++g_drawPointCallCount;
}

void __fastcall zmodel_submit_poly_flat_stub(zVideo_XyzVertex *vertices,
                                                  std::uint32_t packedColor16,
                                                  std::int32_t alpha,
                                                  std::int32_t renderParam,
                                                  std::int32_t vertexCount,
                                                  std::int32_t queueMode) {
    ++g_submitFlatCallCount;
    g_submitFlatLastVertexCount = vertexCount;
    g_submitFlatLastAlpha = alpha;
    g_submitFlatLastColor = packedColor16;
    g_submitFlatLastRenderParam = static_cast<std::uint32_t>(renderParam);
    g_submitFlatLastQueueMode = queueMode;
    for (int i = 0; i < vertexCount && i < 0x40; ++i) {
        g_submitFlatLastVerts[i] = vertices[i];
    }
}

void reset_zclass_type_lists_for_test() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;
}

void free_zclass_type_lists_for_test() {
    for (int i = 0; i < 16; ++i) {
        for (zClass_TypeListLink *link = zClass_TypeList::Head(i); link != nullptr;) {
            zClass_TypeListLink *const next = link->next;
            std::free(link);
            link = next;
        }
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    for (zClass_TypeListLink *link = g_zClass_TypeList_FreeLinkHead; link != nullptr;) {
        zClass_TypeListLink *const next = link->next;
        std::free(link);
        link = next;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
}

bool zclass_bucket_has_pending_node_for_test(int bucket, zClass_NodePartial *node) {
    for (zClass_TypeListLink *link = zClass_TypeList::Head(bucket); link != nullptr;
         link = link->next) {
        if (link->node == node && link->pendingRemove != 0) {
            return true;
        }
    }

    return false;
}
} // namespace

extern "C" int zdi_reset_current_variant_smoke() {
    zDiPartial di{};
    zDiEntryPartial entry{};
    zModel_MaterialPartial material{};
    zModel_MaterialCyclePartial cycle{};
    zImage_TexDirEntryPartial frameA{};
    zImage_TexDirEntryPartial frameB{};
    zImage_TexDirEntryPartial *frames[2] = {&frameA, &frameB};

    di.entries = &entry;
    entry.material = &material;
    material.cycle = &cycle;
    material.currentTextureDirectoryEntry = &frameB;
    cycle.currentFrame = 3.0f;
    cycle.frameTable = frames;

    zDi::ResetCurrentVariant(&di);
    if (cycle.currentFrame != 0.0f || material.currentTextureDirectoryEntry != &frameA) {
        return 1;
    }

    material.cycle = nullptr;
    material.currentTextureDirectoryEntry = &frameB;
    zDi::ResetCurrentVariant(&di);
    return material.currentTextureDirectoryEntry == &frameB ? 0 : 2;
}

extern "C" int zdi_set_current_variant_smoke() {
    zDiPartial di{};
    zDiEntryPartial entry{};
    zModel_MaterialPartial material{};
    zModel_MaterialCyclePartial cycle{};
    zImage_TexDirEntryPartial frameA{};
    zImage_TexDirEntryPartial frameB{};
    zImage_TexDirEntryPartial frameC{};
    zImage_TexDirEntryPartial *frames[3] = {&frameA, &frameB, &frameC};

    di.entries = &entry;
    entry.material = &material;
    material.cycle = &cycle;
    material.currentTextureDirectoryEntry = &frameA;
    cycle.frameCount = 3;
    cycle.frameTable = frames;

    zDi::SetCurrentVariant(&di, 1);
    if (material.currentTextureDirectoryEntry != &frameB || cycle.currentFrame != 1.0f) {
        return 1;
    }

    zDi::SetCurrentVariant(&di, 5);
    if (material.currentTextureDirectoryEntry != &frameC || cycle.currentFrame != 2.0f) {
        return 2;
    }

    zDi::SetCurrentVariant(&di, -4);
    if (material.currentTextureDirectoryEntry != &frameA || cycle.currentFrame != 0.0f) {
        return 3;
    }

    material.cycle = nullptr;
    material.currentTextureDirectoryEntry = &frameC;
    zDi::SetCurrentVariant(&di, 1);
    if (material.currentTextureDirectoryEntry != &frameC) {
        return 4;
    }

    material.flags = 0x0400;
    material.cycle = &cycle;
    cycle.framesPerSecond = 1.0f;
    if (zDi::SetCurrentVariantCycleTextureSpeed(&di, 8.5f) != 1 ||
        cycle.framesPerSecond != 8.5f) {
        return 5;
    }

    material.flags = 0;
    if (zDi::SetCurrentVariantCycleTextureSpeed(&di, 3.0f) != 0 ||
        zDi::SetCurrentVariantCycleTextureSpeed(nullptr, 3.0f) != 0) {
        return 6;
    }

    material.cycle = nullptr;
    if (zDi::SetCurrentVariantCycleTextureCount(&di, 2) != 0 ||
        material.cycle == nullptr || material.cycle->frameCount != 2 ||
        material.cycle->framesPerSecond != 15.0f || material.cycle->frameTable == nullptr ||
        material.cycle->frameTable[0] != zImage::GetDefaultImageRefPtr()) {
        return 7;
    }
    std::free(material.cycle->frameTable);
    std::free(material.cycle);

    g_zError_DebugMsgBuffer[0] = '\0';
    if (zDi::SetCurrentVariantCycleTextureCount(nullptr, 2) != -1 ||
        std::strcmp(g_zError_DebugMsgBuffer,
                    "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c(3903): ERROR setting model "
                    "cycle texture. Model 3D pointer is NULL.\n") != 0) {
        return 8;
    }

    material.flags = 0x0400;
    material.cycle = &cycle;
    cycle.frameTable = frames;
    cycle.loopEnabled = 0;
    if (zModel_Instance::SetCycleTextureLoop(&di, 1) != 1 || cycle.loopEnabled != 1 ||
        zModel_Instance::SetCycleTextureLoop(nullptr, 1) != 0) {
        return 9;
    }

    zImage_TexDirEntryPartial frameD{};
    cycle.frameCount = 3;
    cycle.frameWriteCount = 1;
    cycle.frameTable = frames;
    frames[1] = nullptr;
    if (zModel_Instance::AddCycleTexture(&di, &frameD) != 1 || frames[1] != &frameD ||
        cycle.frameWriteCount != 2 || zModel_Instance::AddCycleTexture(nullptr, &frameD) != 0) {
        return 10;
    }

    return 0;
}

extern "C" int zdi_entry_material_helpers_smoke() {
    zDiEntryPartial entries[3]{};
    zDiPartial di{};
    di.entryCount = 3;
    di.entries = entries;

    entries[0].drawFlags = 1;
    entries[1].drawFlags = 2;
    entries[2].drawFlags = 3;
    zDi::SetEntryValueForAllEntries(&di, 0x89abcdef);
    if (entries[0].drawFlags != 0x89abcdef || entries[1].drawFlags != 0x89abcdef ||
        entries[2].drawFlags != 0x89abcdef) {
        return 1;
    }

    zDi::SetEntryValueForAllEntries(nullptr, 0x11111111);
    di.entryCount = 0;
    entries[0].drawFlags = 0x22222222;
    zDi::SetEntryValueForAllEntries(&di, 0x33333333);
    if (entries[0].drawFlags != 0x22222222) {
        return 2;
    }

    di.entryCount = 3;
    entries[0].flagsAndIndexCount = 0x00000105;
    entries[1].flagsAndIndexCount = 0x00000008;
    entries[2].flagsAndIndexCount = 0x0000ff00;
    zDi::SetShowBackFaceForAllEntries(&di, 0);
    if (entries[0].flagsAndIndexCount != 0x00000005 ||
        entries[1].flagsAndIndexCount != 0x00000008 ||
        entries[2].flagsAndIndexCount != 0x0000fe00) {
        return 3;
    }

    zDi::SetShowBackFaceForAllEntries(&di, 3);
    if (entries[0].flagsAndIndexCount != 0x00000105 ||
        entries[1].flagsAndIndexCount != 0x00000108 ||
        entries[2].flagsAndIndexCount != 0x0000ff00) {
        return 4;
    }

    di.entryCount = 0;
    entries[0].flagsAndIndexCount = 0x00000044;
    zDi::SetShowBackFaceForAllEntries(&di, 1);
    if (entries[0].flagsAndIndexCount != 0x00000044) {
        return 5;
    }

    zModel_MaterialPartial updated{};
    zModel_MaterialPartial skipped{};
    zModel_MaterialPartial updatedAgain{};
    updated.flags = 0;
    updated.packedColor = 0xabcd;
    updated.colorRgb.red = -1.0f;
    updated.colorRgb.green = 9.0f;
    updated.colorRgb.blue = 8.0f;
    updated.colorScalar = 4.0f;
    skipped.flags = 0x0100;
    skipped.packedColor = 0x7777;
    skipped.colorRgb.red = 7.0f;
    skipped.colorRgb.green = 6.0f;
    skipped.colorRgb.blue = 5.0f;
    skipped.colorScalar = 3.0f;
    updatedAgain.flags = 0;
    updatedAgain.packedColor = 0x0055;
    updatedAgain.colorScalar = 2.0f;

    entries[0].material = &updated;
    entries[1].material = &skipped;
    entries[2].material = &updatedAgain;
    di.entryCount = 3;
    zDi::SetObject3DColorModeForMaterials(&di, 0x123);

    if (updated.colorRgb.red != 291.0f || updated.colorRgb.green != 0.0f ||
        updated.colorRgb.blue != 0.0f || updated.packedColor != 0x23cd ||
        updated.colorScalar != 1.0f) {
        return 6;
    }

    if (skipped.colorRgb.red != 7.0f || skipped.colorRgb.green != 6.0f ||
        skipped.colorRgb.blue != 5.0f || skipped.packedColor != 0x7777 ||
        skipped.colorScalar != 3.0f) {
        return 7;
    }

    return updatedAgain.colorRgb.red == 291.0f && updatedAgain.colorRgb.green == 0.0f &&
                   updatedAgain.colorRgb.blue == 0.0f && updatedAgain.packedColor == 0x2355 &&
                   updatedAgain.colorScalar == 1.0f
               ? 0
               : 8;
}
