// Checked-in focused native smoke translation unit, formerly extracted from zgame_tests.cpp.
// Emits only the zClass camera render-pass smokes needed by functional manifests.

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

extern "C" int zclass_camera_build_frustum_grid_tiles_from_params_smoke() {
    auto setIdentity = [](float *matrix) {
        const zMat4x3 identity{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        std::memcpy(matrix, &identity, sizeof(identity));
    };

    zVec3 polygonVertices[8] = {};
    zVec3 polygonNormals[8] = {};
    zVec3 *const savedPolygonVertices = g_zModel_PointInPolygonVertices;
    zVec3 *const savedPolygonNormals = g_zModel_PointInPolygonEdgeNormals;
    const int savedPolygonVertexCount = g_zModel_PointInPolygonVertexCount;
    zClass_CameraDataPartial *const savedViewContext = g_zVideo_pActiveViewContext;
    zClass_CameraDataPartial *const savedProjectionViewContext =
        g_zVideo_pActiveProjectionViewContext;
    int *const savedMatrixIdentitySlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const savedMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    int matrixIdentityFlags[16] = {};
    float *matrixSlots[16] = {};
    zMat4x3 matrixStorage[16] = {};
    setIdentity(reinterpret_cast<float *>(&matrixStorage[0]));
    matrixIdentityFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&matrixStorage[0]);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    g_zModel_PointInPolygonVertices = polygonVertices;
    g_zModel_PointInPolygonEdgeNormals = polygonNormals;
    g_zModel_PointInPolygonVertexCount = 0;

    zWorldAreaPartial row0[2] = {};
    zWorldAreaPartial row1[2] = {};
    zWorldAreaPartial *rows[2] = {row0, row1};
    for (int rowIndex = 0; rowIndex < 2; ++rowIndex) {
        for (int colIndex = 0; colIndex < 2; ++colIndex) {
            rows[rowIndex][colIndex].areaIndex = 1;
            rows[rowIndex][colIndex].cellMinX = static_cast<float>(colIndex * 10);
            rows[rowIndex][colIndex].cellMinZ = static_cast<float>(rowIndex - 1);
        }
    }

    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.worldMaxX = 20.0f;
    worldData.worldMaxZ = 1.0f;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaHalfSizeX = 5.0f;
    worldData.areaHalfSizeZ = 0.5f;
    worldData.areaCellRadiusBias = -1000.0f;
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 2;
    worldData.areaGridRows = rows;

    zClass_NodePartial world{};
    world.classData = &worldData;

    zClass_CameraDataPartial cameraData{};
    cameraData.cameraPos = {0.5f, 0.0f, 0.5f};
    cameraData.frustumOrigin = {-5.0f, 0.0f, -5.0f};
    cameraData.frustumCorners[0] = {5.0f, 0.0f, -5.0f};
    cameraData.frustumCorners[1] = {-5.0f, 0.0f, 5.0f};
    g_zVideo_pActiveViewContext = &cameraData;
    g_zVideo_pActiveProjectionViewContext = &cameraData;

    int result = zClass_Camera::BuildFrustumGridTilesFromParams(&world, &worldData, &cameraData);
    int status = 0;
    if (result != 0 || g_zCamera_FrustumFootprintPointCount != 3) {
        status = 1;
    } else if (g_zCamera_FrustumGridTileRings[0].count != 12) {
        status = 20 + g_zCamera_FrustumGridTileRings[0].count;
    } else {
        auto hasTile = [](int col, int row, int hasPosOffset, float posOffsetX,
                          float posOffsetZ) {
            const zCamera_FrustumGridTileRingPartial &ring = g_zCamera_FrustumGridTileRings[0];
            for (int i = 0; i < ring.count; ++i) {
                const zCamera_FrustumGridTilePartial &tile = ring.tiles[i];
                if (tile.col == col && tile.row == row && tile.hasPosOffset == hasPosOffset &&
                    tile.posOffsetX == posOffsetX && tile.posOffsetZ == posOffsetZ &&
                    tile.clipMask == 0x3f) {
                    return true;
                }
            }
            return false;
        };
        if (!hasTile(0, 0, 1, -10.0f, -5.0f)) {
            status = 3;
        } else if (!hasTile(0, 0, 1, -10.0f, 0.0f)) {
            status = 4;
        } else if (!hasTile(0, 0, 1, 0.0f, -5.0f)) {
            status = 5;
        } else if (!hasTile(0, 0, 0, 0.0f, 0.0f)) {
            status = 6;
        }
    }

    g_zVideo_pActiveViewContext = savedViewContext;
    g_zVideo_pActiveProjectionViewContext = savedProjectionViewContext;
    g_zModel_PointInPolygonVertices = savedPolygonVertices;
    g_zModel_PointInPolygonEdgeNormals = savedPolygonNormals;
    g_zModel_PointInPolygonVertexCount = savedPolygonVertexCount;
    zMath::g_currentMatrixIdentityFlagSlot = savedMatrixIdentitySlot;
    zMath::g_currentMatrixPtrSlot = savedMatrixPtrSlot;
    return status;
}

extern "C" int zclass_camera_build_frustum_grid_tiles_smoke() {
    auto setIdentity = [](float *matrix) {
        const zMat4x3 identity{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        std::memcpy(matrix, &identity, sizeof(identity));
    };

    zVec3 polygonVertices[8] = {};
    zVec3 polygonNormals[8] = {};
    zVec3 *const savedPolygonVertices = g_zModel_PointInPolygonVertices;
    zVec3 *const savedPolygonNormals = g_zModel_PointInPolygonEdgeNormals;
    const int savedPolygonVertexCount = g_zModel_PointInPolygonVertexCount;
    zClass_CameraDataPartial *const savedViewContext = g_zVideo_pActiveViewContext;
    zClass_CameraDataPartial *const savedProjectionViewContext =
        g_zVideo_pActiveProjectionViewContext;
    int *const savedMatrixIdentitySlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const savedMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    int matrixIdentityFlags[16] = {};
    float *matrixSlots[16] = {};
    zMat4x3 matrixStorage[16] = {};
    setIdentity(reinterpret_cast<float *>(&matrixStorage[0]));
    matrixIdentityFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&matrixStorage[0]);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    g_zModel_PointInPolygonVertices = polygonVertices;
    g_zModel_PointInPolygonEdgeNormals = polygonNormals;
    g_zModel_PointInPolygonVertexCount = 0;

    zWorldAreaPartial row0[2] = {};
    zWorldAreaPartial row1[2] = {};
    zWorldAreaPartial *rows[2] = {row0, row1};
    for (int rowIndex = 0; rowIndex < 2; ++rowIndex) {
        for (int colIndex = 0; colIndex < 2; ++colIndex) {
            rows[rowIndex][colIndex].areaIndex = 1;
            rows[rowIndex][colIndex].cellMinX = static_cast<float>(colIndex * 10);
            rows[rowIndex][colIndex].cellMinZ = static_cast<float>(rowIndex * 10);
        }
    }

    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.worldMaxX = 20.0f;
    worldData.worldMaxZ = 20.0f;
    worldData.areaCellSizeX = 10.0f;
    worldData.areaCellSizeZ = 10.0f;
    worldData.areaInvSizeX = 0.1f;
    worldData.areaInvSizeZ = 0.1f;
    worldData.areaHalfSizeX = 5.0f;
    worldData.areaHalfSizeZ = 5.0f;
    worldData.areaCellRadiusBias = -1000.0f;
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 2;
    worldData.areaGridRows = rows;

    zClass_NodePartial world{};
    world.classData = &worldData;

    zClass_CameraDataPartial cameraData{};
    cameraData.cameraPos = {5.0f, 0.0f, -1.0f};
    cameraData.frustumOrigin = {0.0f, 0.0f, 0.0f};
    cameraData.frustumCorners[0] = {10.0f, 0.0f, 0.0f};
    cameraData.frustumCorners[1] = {0.0f, 0.0f, 12.0f};
    g_zVideo_pActiveViewContext = &cameraData;
    g_zVideo_pActiveProjectionViewContext = &cameraData;

    int result = zClass_Camera::BuildFrustumGridTiles(&world, &worldData, &cameraData);
    int status = 0;
    if (result != 0) {
        status = result;
    } else if (g_zCamera_FrustumFootprintPointCount != 3) {
        status = 10 + g_zCamera_FrustumFootprintPointCount;
    } else if (g_zCamera_FrustumGridTileRings[0].count != 1 ||
               g_zCamera_FrustumGridTileRings[1].count != 2 ||
               g_zCamera_FrustumGridTileRings[2].count != 1) {
        status = 20 + g_zCamera_FrustumGridTileRings[0].count +
                 g_zCamera_FrustumGridTileRings[1].count * 4 +
                 g_zCamera_FrustumGridTileRings[2].count * 16;
    } else {
        const zCamera_FrustumGridTilePartial &originTile =
            g_zCamera_FrustumGridTileRings[0].tiles[0];
        const zCamera_FrustumGridTilePartial &rowTile = g_zCamera_FrustumGridTileRings[1].tiles[0];
        const zCamera_FrustumGridTilePartial &colTile = g_zCamera_FrustumGridTileRings[1].tiles[1];
        const zCamera_FrustumGridTilePartial &diagonalTile =
            g_zCamera_FrustumGridTileRings[2].tiles[0];
        if (originTile.col != 0 || originTile.row != 0 || originTile.hasPosOffset != 0 ||
            originTile.clipMask != 0x3f) {
            status = 3;
        } else if (rowTile.col != 0 || rowTile.row != 1 || rowTile.hasPosOffset != 0 ||
                   rowTile.clipMask != 0x3f) {
            status = 4;
        } else if (colTile.col != 1 || colTile.row != 0 || colTile.hasPosOffset != 0 ||
                   colTile.clipMask != 0x3f) {
            status = 5;
        } else if (diagonalTile.col != 1 || diagonalTile.row != 1 ||
                   diagonalTile.hasPosOffset != 0 || diagonalTile.clipMask != 0x3f) {
            status = 6;
        }
    }

    g_zVideo_pActiveViewContext = savedViewContext;
    g_zVideo_pActiveProjectionViewContext = savedProjectionViewContext;
    g_zModel_PointInPolygonVertices = savedPolygonVertices;
    g_zModel_PointInPolygonEdgeNormals = savedPolygonNormals;
    g_zModel_PointInPolygonVertexCount = savedPolygonVertexCount;
    zMath::g_currentMatrixIdentityFlagSlot = savedMatrixIdentitySlot;
    zMath::g_currentMatrixPtrSlot = savedMatrixPtrSlot;
    return status;
}

extern "C" int zclass_camera_render_overlay_nodes_smoke() {
    auto setIdentity = [](float *matrix) {
        const zMat4x3 identity{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
        std::memcpy(matrix, &identity, sizeof(identity));
    };
    auto resetRenderLog = []() {
        g_zclassRenderTraverseCallCount = 0;
        for (int i = 0; i < 16; ++i) {
            g_zclassRenderTraverseNodes[i] = nullptr;
            g_zclassRenderTraverseClipMasks[i] = -1;
            g_zclassRenderTraverseAlphaScales[i] = -1.0f;
            g_zclassRenderTraverseVertexAlphaEnabled[i] = -1;
        }
    };

    zClass_RenderFn const savedRenderFn = gModel_RenderFn;
    int *const savedClipStackTop = gModel_ClipMaskStackTop;
    const int savedClipStack0 = gModel_ClipMaskStack[0];
    zClass_CameraDataPartial *const savedViewContext = g_zVideo_pActiveViewContext;
    zClass_CameraDataPartial *const savedProjectionViewContext =
        g_zVideo_pActiveProjectionViewContext;
    const int savedBoundsContext = g_zClass_RenderBoundsContextActive;
    const int savedObjectHseTestEnabled = g_zClass_ObjectHseTestEnabled;
    const int savedFrustumGridTileIndex = g_zClass_RenderFrustumGridTileIndex;
    const int savedVertexAlphaOverride = g_zClass_RenderVertexAlphaOverrideActive;
    const int savedAlphaScaleStackTop = g_zClass_RenderAlphaScaleStackTop;
    const int savedSoftwarePathStateStackTop = g_zClass_SoftwarePathStateStackTop;
    const int savedVariantFilterEnabled = g_Variant_FilterEnabled;
    const int savedModelVertexAlpha = gModel_RenderVertexAlphaEnabled;
    const float savedModelAlphaScale = gModel_RenderAlphaScaleCurrent;
    int *const savedMatrixIdentitySlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const savedMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    int matrixIdentityFlags[16] = {};
    float *matrixSlots[16] = {};
    zMat4x3 matrixStorage[16] = {};
    setIdentity(reinterpret_cast<float *>(&matrixStorage[0]));
    matrixIdentityFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&matrixStorage[0]);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zClass_CameraDataPartial cameraData{};
    cameraData.worldFrustumNormals[0] = {1.0f, 0.0f, 0.0f};
    cameraData.worldFrustumNormals[1] = {1.0f, 0.0f, 0.0f};
    cameraData.worldFrustumNormals[2] = {0.0f, 1.0f, 0.0f};
    cameraData.worldFrustumNormals[3] = {0.0f, 1.0f, 0.0f};
    cameraData.worldFrustumNormals[4] = {0.0f, 0.0f, 1.0f};
    cameraData.worldFrustumNormals[5] = {0.0f, 0.0f, 1.0f};
    g_zVideo_pActiveViewContext = &cameraData;
    g_zVideo_pActiveProjectionViewContext = &cameraData;
    gModel_RenderFn = TestZClassRenderTraverseCallback;
    gModel_ClipMaskStackTop = gModel_ClipMaskStack;
    gModel_ClipMaskStack[0] = 0;
    g_zClass_RenderBoundsContextActive = 0;
    g_zClass_ObjectHseTestEnabled = 0;
    g_zClass_RenderFrustumGridTileIndex = 0;
    g_zClass_RenderVertexAlphaOverrideActive = 0;
    g_zClass_RenderAlphaScaleStackTop = -1;
    g_zClass_SoftwarePathStateStackTop = -1;
    g_Variant_FilterEnabled = 0;
    gModel_RenderVertexAlphaEnabled = 0;
    gModel_RenderAlphaScaleCurrent = 1.0f;
    resetRenderLog();

    zClass_Object3DDataPartial objectData[2] = {};
    objectData[0].flags = 0x08;
    objectData[1].flags = 0x08;
    zClass_NodeFreeListSlot overlaySlots[2] = {};
    zClass_NodePartial &overlay0 = overlaySlots[0].node;
    zClass_NodePartial &overlay1 = overlaySlots[1].node;
    overlay0.flags = 0x02080004;
    overlay0.nodeType = 0xff;
    overlay0.classId = 5;
    overlay0.classData = &objectData[0];
    *zClass_NodeViewSphereCenter(&overlay0) = {1000000.0f, 1000000.0f, 1000000.0f};
    *zClass_NodeViewSphereRadius(&overlay0) = 1.0f;
    overlay1.flags = 0x02080004;
    overlay1.nodeType = 0xff;
    overlay1.classId = 5;
    overlay1.classData = &objectData[1];
    *zClass_NodeViewSphereCenter(&overlay1) = {1000000.0f, 1000000.0f, 1000000.0f};
    *zClass_NodeViewSphereRadius(&overlay1) = 1.0f;
    zClass_NodePartial *overlays[2] = {&overlay0, &overlay1};
    zClass_NodePartial world{};
    world.listCountB = 2;
    world.listB = overlays;

    zClass_Camera::RenderOverlayNodes(&world);
    int status = 0;
    if (gModel_ClipMaskStack[0] != 0x3f) {
        status = 1;
    } else if (g_zclassRenderTraverseCallCount != 2 ||
               g_zclassRenderTraverseNodes[0] != &overlay0 ||
               g_zclassRenderTraverseNodes[1] != &overlay1) {
        status = 10 + g_zclassRenderTraverseCallCount;
    }

    gModel_RenderFn = savedRenderFn;
    gModel_ClipMaskStackTop = savedClipStackTop;
    gModel_ClipMaskStack[0] = savedClipStack0;
    g_zVideo_pActiveViewContext = savedViewContext;
    g_zVideo_pActiveProjectionViewContext = savedProjectionViewContext;
    g_zClass_RenderBoundsContextActive = savedBoundsContext;
    g_zClass_ObjectHseTestEnabled = savedObjectHseTestEnabled;
    g_zClass_RenderFrustumGridTileIndex = savedFrustumGridTileIndex;
    g_zClass_RenderVertexAlphaOverrideActive = savedVertexAlphaOverride;
    g_zClass_RenderAlphaScaleStackTop = savedAlphaScaleStackTop;
    g_zClass_SoftwarePathStateStackTop = savedSoftwarePathStateStackTop;
    g_Variant_FilterEnabled = savedVariantFilterEnabled;
    gModel_RenderVertexAlphaEnabled = savedModelVertexAlpha;
    gModel_RenderAlphaScaleCurrent = savedModelAlphaScale;
    zMath::g_currentMatrixIdentityFlagSlot = savedMatrixIdentitySlot;
    zMath::g_currentMatrixPtrSlot = savedMatrixPtrSlot;
    return status;
}
