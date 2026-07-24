// Checked-in focused native smoke translation unit, formerly extracted from zgame_tests.cpp.
// Emits only the zModel shutdown smokes needed by functional manifests.

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

extern "C" int zmodel_display_shutdown_smoke() {
    g_zModel_DiPoolCapacity = 1;
    g_zModel_DiPoolInUseCount = 1;
    g_zModel_DiPoolFreeHeadIndex = 7;
    g_zModel_DiPoolBase = static_cast<zDiPartial *>(std::calloc(1, sizeof(zDiPartial)));
    g_zModel_DiPoolBase[0].entries =
        static_cast<zDiEntryPartial *>(std::malloc(sizeof(zDiEntryPartial)));
    g_zModel_MatlPool = static_cast<zModel_MaterialSlot *>(std::malloc(4));
    g_zModel_MatlPoolCapacity = 2;
    g_zModel_MatlPoolInUseCount = 1;
    g_zModel_MatlFreeHeadIndex = 3;
    g_zModel_MatlActiveHeadIndex = 4;
    g_zModel_MatlReuseCache = &g_zModel_DefaultMaterial;

    if (zModel_Display::ShutdownThunk() != 0) {
        return 1;
    }

    return g_zModel_DiPoolBase == nullptr && g_zModel_DiPoolCapacity == 0 &&
                   g_zModel_DiPoolInUseCount == 0 && g_zModel_DiPoolFreeHeadIndex == -1 &&
                   g_zModel_MatlPool == nullptr && g_zModel_MatlPoolCapacity == 0 &&
                   g_zModel_MatlPoolInUseCount == 0 && g_zModel_MatlFreeHeadIndex == -1 &&
                   g_zModel_MatlActiveHeadIndex == -1 && g_zModel_MatlReuseCache == nullptr
               ? 0
               : 2;
}

extern "C" int zmodel_matlslot_release_smoke() {
    zModel_MaterialSlot *const savedPool = g_zModel_MatlPool;
    const int savedCapacity = g_zModel_MatlPoolCapacity;
    const int savedInUse = g_zModel_MatlPoolInUseCount;
    const int savedFreeHead = g_zModel_MatlFreeHeadIndex;
    const int savedActiveHead = g_zModel_MatlActiveHeadIndex;

    zModel_MaterialSlot slots[4] = {};
    zModel_MaterialCyclePartial *const cycle =
        static_cast<zModel_MaterialCyclePartial *>(std::malloc(sizeof(zModel_MaterialCyclePartial)));
    zImage_TexDirEntryPartial **const frameTable = static_cast<zImage_TexDirEntryPartial **>(
        std::malloc(2 * sizeof(zImage_TexDirEntryPartial *)));
    if (cycle == nullptr || frameTable == nullptr) {
        std::free(frameTable);
        std::free(cycle);
        return 1;
    }

    std::memset(cycle, 0, sizeof(*cycle));
    cycle->frameTable = frameTable;
    slots[0].material.flags = 0x0001;
    slots[0].prevPoolIndex = -1;
    slots[0].nextPoolIndex = 1;
    slots[1].material.flags = 0x0400;
    slots[1].material.packedColor = 0x1234;
    slots[1].material.cycle = cycle;
    slots[1].prevPoolIndex = 0;
    slots[1].nextPoolIndex = 2;
    slots[2].material.flags = 0x0002;
    slots[2].prevPoolIndex = 1;
    slots[2].nextPoolIndex = -1;
    slots[3].prevPoolIndex = -1;
    slots[3].nextPoolIndex = -1;

    g_zModel_MatlPool = slots;
    g_zModel_MatlPoolCapacity = 4;
    g_zModel_MatlPoolInUseCount = 3;
    g_zModel_MatlFreeHeadIndex = 3;
    g_zModel_MatlActiveHeadIndex = 0;

    zModel_MatlSlot::Release(nullptr);
    if (g_zModel_MatlPoolInUseCount != 3 || g_zModel_MatlFreeHeadIndex != 3 ||
        g_zModel_MatlActiveHeadIndex != 0) {
        std::free(frameTable);
        std::free(cycle);
        g_zModel_MatlPool = savedPool;
        g_zModel_MatlPoolCapacity = savedCapacity;
        g_zModel_MatlPoolInUseCount = savedInUse;
        g_zModel_MatlFreeHeadIndex = savedFreeHead;
        g_zModel_MatlActiveHeadIndex = savedActiveHead;
        return 2;
    }

    zModel_MatlSlot::Release(&slots[1]);
    const bool middleReleaseOk =
        slots[0].nextPoolIndex == 2 && slots[2].prevPoolIndex == 0 &&
        slots[1].prevPoolIndex == -1 && slots[1].nextPoolIndex == 3 &&
        slots[3].prevPoolIndex == 1 && g_zModel_MatlActiveHeadIndex == 0 &&
        g_zModel_MatlFreeHeadIndex == 1 && g_zModel_MatlPoolInUseCount == 2 &&
        slots[1].material.flags == 0 && slots[1].material.packedColor == 0 &&
        slots[1].material.cycle == nullptr;

    zModel_MatlSlot::Release(&slots[0]);
    const bool headReleaseOk =
        slots[2].prevPoolIndex == -1 && slots[0].prevPoolIndex == -1 &&
        slots[0].nextPoolIndex == 1 && slots[1].prevPoolIndex == 0 &&
        g_zModel_MatlActiveHeadIndex == 2 && g_zModel_MatlFreeHeadIndex == 0 &&
        g_zModel_MatlPoolInUseCount == 1 && slots[0].material.flags == 0;

    g_zModel_MatlPool = savedPool;
    g_zModel_MatlPoolCapacity = savedCapacity;
    g_zModel_MatlPoolInUseCount = savedInUse;
    g_zModel_MatlFreeHeadIndex = savedFreeHead;
    g_zModel_MatlActiveHeadIndex = savedActiveHead;

    if (!middleReleaseOk) {
        return 3;
    }
    return headReleaseOk ? 0 : 4;
}

extern "C" int zmodel_matlbuffer_release_all_active_smoke() {
    zModel_MaterialSlot *const savedPool = g_zModel_MatlPool;
    const int savedCapacity = g_zModel_MatlPoolCapacity;
    const int savedInUse = g_zModel_MatlPoolInUseCount;
    const int savedFreeHead = g_zModel_MatlFreeHeadIndex;
    const int savedActiveHead = g_zModel_MatlActiveHeadIndex;
    zModel_MaterialPartial *const savedReuseCache = g_zModel_MatlReuseCache;

    zModel_MaterialSlot slots[4] = {};
    slots[0].material.flags = 0x0001;
    slots[0].prevPoolIndex = -1;
    slots[0].nextPoolIndex = 2;
    slots[1].material.flags = 0x0002;
    slots[1].prevPoolIndex = 2;
    slots[1].nextPoolIndex = -1;
    slots[2].material.flags = 0x0004;
    slots[2].prevPoolIndex = 0;
    slots[2].nextPoolIndex = 1;
    slots[3].prevPoolIndex = -1;
    slots[3].nextPoolIndex = -1;

    g_zModel_MatlPool = slots;
    g_zModel_MatlPoolCapacity = 4;
    g_zModel_MatlPoolInUseCount = 3;
    g_zModel_MatlFreeHeadIndex = 3;
    g_zModel_MatlActiveHeadIndex = 0;
    g_zModel_MatlReuseCache = &slots[2].material;

    const int result = zModel_MatlBuffer::ReleaseAllActive();
    const bool releaseOk =
        result == 0 && g_zModel_MatlActiveHeadIndex == -1 &&
        g_zModel_MatlFreeHeadIndex == 1 && g_zModel_MatlPoolInUseCount == 0 &&
        g_zModel_MatlReuseCache == nullptr && slots[1].nextPoolIndex == 2 &&
        slots[2].prevPoolIndex == 1 && slots[2].nextPoolIndex == 0 &&
        slots[0].prevPoolIndex == 2 && slots[0].nextPoolIndex == 3 &&
        slots[3].prevPoolIndex == 0 && slots[0].material.flags == 0 &&
        slots[1].material.flags == 0 && slots[2].material.flags == 0;

    g_zModel_MatlPool = savedPool;
    g_zModel_MatlPoolCapacity = savedCapacity;
    g_zModel_MatlPoolInUseCount = savedInUse;
    g_zModel_MatlFreeHeadIndex = savedFreeHead;
    g_zModel_MatlActiveHeadIndex = savedActiveHead;
    g_zModel_MatlReuseCache = savedReuseCache;

    return releaseOk ? 0 : 1;
}
