#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "Battlesport/zUtil/zutil.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zGeometry/zGeometry.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "OptCatalog.h"
#include "zClass.h"
#include "zClipAlt.h"
#include "zDi.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <io.h>
#include <limits>

extern "C" int zutil_store_int32_smoke(void) {
    int value = 0;
    zUtil::StoreInt32(&value, -12345);
    return value == -12345 ? 0 : 1;
}

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

void RECOIL_CDECL TestZGameUpdateFogColor(void) {
    ++g_zgameFogColorUpdateCount;
}

void RECOIL_FASTCALL TestModelRefLerpCallback(void *callbackCtx) {
    ++g_modelRefLerpCallbackCount;
    g_modelRefLerpLastCallbackCtx = callbackCtx;
}

int RECOIL_FASTCALL TestZClassUpdateBucketCallback(zClass_NodePartial *node) {
    ++g_zclassUpdateBucketCallbackCount;
    g_zclassUpdateBucketLastNode = node;
    g_zclassUpdateBucketDeferredDuringCallback = g_zClass_DeferredProcessingEnabled;
    return 0;
}

int g_zmodelReleaseTextureUploadCount = 0;
zVideo_TextureRecordPartial *g_zmodelReleaseTextureUploadLast = nullptr;

void RECOIL_FASTCALL TestReleaseTextureUploadSurfaceRef(
    zVideo_TextureRecordPartial *textureRecord) {
    ++g_zmodelReleaseTextureUploadCount;
    g_zmodelReleaseTextureUploadLast = textureRecord;
}

int RECOIL_FASTCALL TextureMemoryQueryMissingStub(int, int *, int *) {
    return 0;
}

int RECOIL_FASTCALL DamageMaskLockUploadStub(zVideo_TextureRecordPartial *textureRecord,
                                             void **outPixels, int *outPitchBytes) {
    ++g_damageMaskUploadLockCount;
    g_damageMaskLastTextureRecord = textureRecord;
    *outPixels = g_damageMaskUploadPixels;
    *outPitchBytes = g_damageMaskUploadPitchBytes;
    return 1;
}

void RECOIL_FASTCALL DamageMaskUnlockUploadStub(zVideo_TextureRecordPartial *textureRecord) {
    ++g_damageMaskUploadUnlockCount;
    g_damageMaskLastTextureRecord = textureRecord;
}

void RECOIL_FASTCALL DamageMaskFinalizeUploadStub(zVideo_TextureRecordPartial *textureRecord,
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

std::int32_t FloatBitsForTest(float value) {
    std::int32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    return raw;
}

std::int32_t RECOIL_FASTCALL zclass_test_node_type_0x42(zClass_NodePartial *node) {
    return node->nodeType == 0x42 ? 1 : 0;
}

float RECOIL_FASTCALL zclass_damage_timer_stub(void *context, float damageAmount) {
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

void RECOIL_FASTCALL zmodel_draw_point_color16_stub(zProjectedPoint *point,
                                                    std::uint32_t packedColor16,
                                                    std::int32_t pointCount) {
    g_drawPointLastPoint = *point;
    g_drawPointLastColor = packedColor16;
    g_drawPointLastCount = pointCount;
    ++g_drawPointCallCount;
}

void RECOIL_FASTCALL zmodel_submit_poly_flat_stub(zVideo_XyzVertex *vertices,
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

extern "C" int zgame_return_only_stub_smoke() {
    zGame::ReturnOnlyStub();
    return 0;
}

extern "C" int zgame_options_init_registry_context_smoke() {
    std::free(g_zGame_Options_RegKeyRoot);
    std::free(g_zGame_Options_RegKeyCurrentUser);
    std::free(g_zGame_Options_RegKeyGame);
    g_zGame_Options_RegKeyRoot = nullptr;
    g_zGame_Options_RegKeyCurrentUser = nullptr;
    g_zGame_Options_RegKeyGame = nullptr;
    g_zGame_Options_RegContextInitialized = 0;

    zOptionEntryPartial option = {};
    g_zGame_Options_OptionListHead = &option;
    zGame::Options_InitRegistryContext("root", "user", "game");

    const bool ok = g_zGame_Options_RegContextInitialized == 1 &&
                    g_zGame_Options_OptionListHead == nullptr &&
                    g_zGame_Options_RegKeyRoot != nullptr &&
                    std::strcmp(g_zGame_Options_RegKeyRoot, "root") == 0 &&
                    g_zGame_Options_RegKeyCurrentUser != nullptr &&
                    std::strcmp(g_zGame_Options_RegKeyCurrentUser, "user") == 0 &&
                    g_zGame_Options_RegKeyGame != nullptr &&
                    std::strcmp(g_zGame_Options_RegKeyGame, "game") == 0 &&
                    g_zGame_Options_RuntimeConfigDefaults.cpuVendor[0] != '\0' &&
                    g_zGame_Options_RuntimeConfigDefaults.systemRamKb != 0;

    std::free(g_zGame_Options_RegKeyRoot);
    std::free(g_zGame_Options_RegKeyCurrentUser);
    std::free(g_zGame_Options_RegKeyGame);
    g_zGame_Options_RegKeyRoot = nullptr;
    g_zGame_Options_RegKeyCurrentUser = nullptr;
    g_zGame_Options_RegKeyGame = nullptr;
    return ok ? 0 : 1;
}

extern "C" int zmodel_display_init_smoke() {
    g_zVideo_ActiveRendererPath = 0;
    gModel_DefaultGraphicsFlags = 0;
    g_zModel_GraphicsFlagsOption = nullptr;
    g_Variant_CurrentTag.count = 3;
    g_Variant_CurrentTag.tags[0] = 1;

    if (zModel_Display_Init() != 0 || g_zModel_DisplayClipMode != 2 || g_zModel_DisplayClipX != 0 ||
        g_zModel_DisplayClipY != 0 || gModel_FogEnabled != 1 || gModel_FogLinearModeEnabled != 1 ||
        gModel_FogDistanceStart != 500.0f || gModel_FogDistanceEnd != 700.0f ||
        gModel_FogDistanceInvRange != 0.005f || gModel_FogHeightHigh != 300.0f ||
        gModel_FogHeightLow != 200.0f || gModel_FogHeightInvRange != 0.01f ||
        gModel_FogDensity != 2.0f || g_zModel_InverseZTolerance != 0.01f ||
        gModel_DefaultGraphicsFlags != -1 ||
        g_zModel_GraphicsFlagsOption != &gModel_DefaultGraphicsFlags ||
        g_Variant_CurrentTag.count != 0 || g_Variant_CurrentTag.tags[0] != 0xff) {
        return 1;
    }

    char optionName[] = "GfxFlags_HW";
    zOptionEntryPartial option = {};
    option.name = optionName;
    option.next = nullptr;
    g_zGame_Options_OptionListHead = &option;
    g_zVideo_ActiveRendererPath = 1;
    g_zModel_HardwareInverseZTolerance = 0.0f;
    g_zModel_GraphicsFlagsOption = nullptr;

    const bool hardwareOk = zModel_Display_Init() == 0 && g_zModel_InverseZTolerance == 0.02f &&
                            g_zModel_HardwareInverseZTolerance == 0.02f &&
                            g_zModel_GraphicsFlagsOption == &option;

    g_zGame_Options_OptionListHead = nullptr;
    g_zVideo_ActiveRendererPath = 0;
    return hardwareOk ? 0 : 2;
}

extern "C" int zmodel_backface_elimination_tolerance_smoke() {
    g_zModel_BFETolerance = -1.0f;
    zModel::SetBackfaceEliminationToleranceScalar(0.125f);
    if (g_zModel_BFETolerance != 0.125f ||
        zModel::GetBackfaceEliminationToleranceScalar() != 0.125f) {
        return 1;
    }

    zModel::SetBackfaceEliminationToleranceScalar(-3.5f);
    return zModel::GetBackfaceEliminationToleranceScalar() == -3.5f ? 0 : 2;
}

extern "C" int zmodel_small_poly_reject_thresholds_smoke() {
    const float savedArea2x = gModel_SmallPolyRejectArea2x;
    const float savedArea20x = gModel_SmallPolyRejectArea20x;

    zModel::UpdateSmallPolyRejectThresholds(3.25f);
    if (gModel_SmallPolyRejectArea2x != 6.5f || gModel_SmallPolyRejectArea20x != 65.0f) {
        gModel_SmallPolyRejectArea2x = savedArea2x;
        gModel_SmallPolyRejectArea20x = savedArea20x;
        return 1;
    }

    zModel::UpdateSmallPolyRejectThresholds(-0.5f);
    const bool negativeOk =
        gModel_SmallPolyRejectArea2x == -1.0f && gModel_SmallPolyRejectArea20x == -10.0f;

    gModel_SmallPolyRejectArea2x = savedArea2x;
    gModel_SmallPolyRejectArea20x = savedArea20x;
    return negativeOk ? 0 : 2;
}

extern "C" int zmodel_set_vertex_shading_enabled_smoke() {
    const int savedVertexShadingEnabled = g_zModel_VertexShadingEnabled;

    zModel::SetVertexShadingEnabled(1);
    if (g_zModel_VertexShadingEnabled != 1) {
        g_zModel_VertexShadingEnabled = savedVertexShadingEnabled;
        return 1;
    }

    zModel::SetVertexShadingEnabled(-7);
    const bool signedValueOk = g_zModel_VertexShadingEnabled == -7;

    g_zModel_VertexShadingEnabled = savedVertexShadingEnabled;
    return signedValueOk ? 0 : 2;
}

extern "C" int zmodel_const_tolerances_and_cross_smoke() {
    zModel_Const::SetCoplanarTolerance(0.25f);
    zModel_Const::SetColinearTolerance(0.001f);
    if (g_zModel_CoplanarTolerance != 0.25 || g_zModel_ColinearTolerance != 0.001f) {
        return 1;
    }

    zVec3 vertex0 = {1.0f, 0.0f, 0.0f};
    zVec3 vertex1 = {0.0f, 0.0f, 0.0f};
    zVec3 vertex2 = {0.0f, 1.0f, 0.0f};
    zVec3 normal = {};
    zVec3 *const returned =
        zModel_Const::SetNormalizedCrossFromVertexTriplet(&vertex0, &vertex1, &normal, &vertex2);
    if (returned != &normal || std::fabs(normal.x) > 0.00001f ||
        std::fabs(normal.y) > 0.00001f || std::fabs(normal.z + 1.0f) > 0.00001f) {
        return 2;
    }

    zVec3 small0 = {0.00001f, 0.0f, 0.0f};
    zVec3 small1 = {0.0f, 0.0f, 0.0f};
    zVec3 small2 = {0.0f, 0.00001f, 0.0f};
    normal.x = 3.0f;
    normal.y = 4.0f;
    normal.z = 5.0f;
    zModel_Const::SetNormalizedCrossFromVertexTriplet(&small0, &small1, &normal, &small2);
    if (normal.x != 0.0f || normal.y != 0.0f || normal.z != 0.0f) {
        return 3;
    }

    zVec3 noRemoval[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    int noRemovalCount = 3;
    if (zModel_Const::RemoveColinearVerticesInPlace(&noRemovalCount, noRemoval, nullptr,
                                                    nullptr, nullptr) != 0 ||
        noRemovalCount != 3) {
        return 4;
    }

    zVec3 removal[4] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {2.0f, 0.0f, 0.0f},
        {2.0f, 1.0f, 0.0f},
    };
    int removalCount = 4;
    if (zModel_Const::RemoveColinearVerticesInPlace(&removalCount, removal, nullptr,
                                                    nullptr, nullptr) != 1 ||
        removalCount != 3 || removal[1].x != 2.0f || removal[1].y != 0.0f ||
        removal[2].x != 2.0f || removal[2].y != 1.0f) {
        return 5;
    }

    zModel_Const::SetCoplanarTolerance(0.001f);
    zVec3 planePoints[4] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    zGeometry_PlaneEquationPartial plane = {};
    if (zModel_Const::ComputePolygonPlaneEquation(4, planePoints, &plane) != &plane ||
        std::fabs(plane.a) > 0.00001f || std::fabs(plane.b) > 0.00001f ||
        std::fabs(plane.c - 1.0f) > 0.00001f || std::fabs(plane.d) > 0.00001f) {
        return 6;
    }

    if (zModel_Const::IsPolygonCoplanar(4, planePoints) != 1) {
        return 7;
    }

    planePoints[2].z = 1.0f;
    if (zModel_Const::IsPolygonCoplanar(4, planePoints) != 0 ||
        zModel_Const::IsPolygonCoplanar(0, planePoints) != 1) {
        return 8;
    }

    zDiPartial splitDi = {};
    zModel_MaterialPartial splitMaterial = {};
    splitMaterial.flags = 0x0101;
    zVec3 splitPoints[4] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    zClipUV splitUvA[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {1.0f, 1.0f},
        {0.0f, 1.0f},
    };
    const int splitTag = 0x1234;
    zModel_Const::SplitPolygonChunkedByVertexLimit(
        &splitDi, 4, splitPoints, nullptr, splitUvA, nullptr, nullptr, nullptr,
        &splitMaterial, 0x55, 1, &splitTag);
    if (splitDi.entryCount != 2 || splitDi.vertCount != 4 || splitDi.entries == nullptr ||
        splitDi.verts == nullptr) {
        return 9;
    }
    const int *firstIndices = static_cast<const int *>(splitDi.entries[0].vertexIndices);
    const int *secondIndices = static_cast<const int *>(splitDi.entries[1].vertexIndices);
    const zClipUV *firstUvs = static_cast<const zClipUV *>(splitDi.entries[0].uvPairs);
    const zClipUV *secondUvs = static_cast<const zClipUV *>(splitDi.entries[1].uvPairs);
    const bool splitOk =
        firstIndices != nullptr && secondIndices != nullptr && firstUvs != nullptr &&
        secondUvs != nullptr && splitDi.entries[0].drawFlags == 0x55 &&
        splitDi.entries[1].drawFlags == 0x55 &&
        (splitDi.entries[0].flagsAndIndexCount & 0x01ff) == 0x0103 &&
        (splitDi.entries[1].flagsAndIndexCount & 0x01ff) == 0x0103 &&
        splitDi.verts[firstIndices[0]].x == 0.0f && splitDi.verts[firstIndices[1]].x == 1.0f &&
        splitDi.verts[firstIndices[1]].y == 0.0f && splitDi.verts[firstIndices[2]].x == 1.0f &&
        splitDi.verts[firstIndices[2]].y == 1.0f && splitDi.verts[secondIndices[0]].x == 0.0f &&
        splitDi.verts[secondIndices[1]].x == 1.0f && splitDi.verts[secondIndices[1]].y == 1.0f &&
        splitDi.verts[secondIndices[2]].x == 0.0f && splitDi.verts[secondIndices[2]].y == 1.0f &&
        firstUvs[0].u == 0.0f && firstUvs[1].u == 1.0f && firstUvs[2].v == 1.0f &&
        secondUvs[0].u == 0.0f && secondUvs[1].u == 1.0f && secondUvs[1].v == 1.0f &&
        secondUvs[2].u == 0.0f && secondUvs[2].v == 1.0f;
    for (int i = 0; i < splitDi.entryCount; ++i) {
        std::free(splitDi.entries[i].vertexIndices);
        std::free(splitDi.entries[i].normalIndices);
        std::free(splitDi.entries[i].uvPairs);
    }
    std::free(splitDi.entries);
    std::free(splitDi.verts);
    std::free(splitDi.normals);
    std::free(splitDi.blendVerts);
    if (!splitOk) {
        return 9;
    }

    zDiPartial chunkDi = {};
    zModel_MaterialPartial chunkMaterial = {};
    chunkMaterial.flags = 0x0101;
    zVec3 chunkPoints[6] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {2.0f, 1.0f, 0.0f},
        {3.0f, 0.0f, 0.0f},
        {4.0f, 1.0f, 0.0f},
        {5.0f, 0.0f, 0.0f},
    };
    zClipUV chunkUvA[6] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {2.0f, 1.0f},
        {3.0f, 0.0f},
        {4.0f, 1.0f},
        {5.0f, 0.0f},
    };
    zDi::AddPolygonSplitByVertexLimit(&chunkDi, 6, chunkPoints, nullptr, chunkUvA, nullptr,
                                      nullptr, nullptr, &chunkMaterial, 0x66, 1, &splitTag, 4);
    if (chunkDi.entryCount != 2 || chunkDi.vertCount != 6 || chunkDi.entries == nullptr ||
        chunkDi.verts == nullptr) {
        return 10;
    }
    const int *chunkFirst = static_cast<const int *>(chunkDi.entries[0].vertexIndices);
    const int *chunkSecond = static_cast<const int *>(chunkDi.entries[1].vertexIndices);
    const zClipUV *chunkFirstUvs = static_cast<const zClipUV *>(chunkDi.entries[0].uvPairs);
    const zClipUV *chunkSecondUvs = static_cast<const zClipUV *>(chunkDi.entries[1].uvPairs);
    const bool chunkOk =
        chunkFirst != nullptr && chunkSecond != nullptr && chunkFirstUvs != nullptr &&
        chunkSecondUvs != nullptr && (chunkDi.entries[0].flagsAndIndexCount & 0x01ff) == 0x0104 &&
        (chunkDi.entries[1].flagsAndIndexCount & 0x01ff) == 0x0104 &&
        chunkDi.verts[chunkFirst[0]].x == 0.0f && chunkDi.verts[chunkFirst[1]].x == 1.0f &&
        chunkDi.verts[chunkFirst[2]].x == 2.0f && chunkDi.verts[chunkFirst[3]].x == 3.0f &&
        chunkDi.verts[chunkSecond[0]].x == 0.0f && chunkDi.verts[chunkSecond[1]].x == 3.0f &&
        chunkDi.verts[chunkSecond[2]].x == 4.0f && chunkDi.verts[chunkSecond[3]].x == 5.0f &&
        chunkFirstUvs[3].u == 3.0f && chunkSecondUvs[1].u == 3.0f &&
        chunkSecondUvs[2].u == 4.0f && chunkSecondUvs[3].u == 5.0f;
    for (int i = 0; i < chunkDi.entryCount; ++i) {
        std::free(chunkDi.entries[i].vertexIndices);
        std::free(chunkDi.entries[i].normalIndices);
        std::free(chunkDi.entries[i].uvPairs);
    }
    std::free(chunkDi.entries);
    std::free(chunkDi.verts);
    std::free(chunkDi.normals);
    std::free(chunkDi.blendVerts);
    if (!chunkOk) {
        return 10;
    }

    zDiPartial mergeDi = {};
    zVec3 mergePoint = {1.0f, 2.0f, 3.0f};
    zVec3 mergeNormal = {2.0f, 4.0f, 6.0f};
    int mergedIndex = zModel_Const::AddOrMergeVertexAndNormal(&mergeDi, &mergePoint,
                                                              &mergeNormal);
    const bool firstMergeOk =
        mergedIndex == 0 && mergeDi.vertCount == 1 && mergeDi.blendVertCount == 1 &&
        mergeDi.verts != nullptr && mergeDi.blendVerts != nullptr &&
        mergeDi.verts[0].x == 1.0f && mergeDi.verts[0].y == 2.0f &&
        mergeDi.verts[0].z == 3.0f && mergeDi.blendVerts[0].x == 1.0f &&
        mergeDi.blendVerts[0].y == 2.0f && mergeDi.blendVerts[0].z == 3.0f;
    if (!firstMergeOk) {
        std::free(mergeDi.verts);
        std::free(mergeDi.blendVerts);
        return 11;
    }

    mergedIndex = zModel_Const::AddOrMergeVertexAndNormal(&mergeDi, &mergePoint,
                                                          &mergeNormal);
    if (mergedIndex != 0 || mergeDi.vertCount != 1 || mergeDi.blendVertCount != 1) {
        std::free(mergeDi.verts);
        std::free(mergeDi.blendVerts);
        return 12;
    }

    zVec3 secondNormal = {3.0f, 4.0f, 6.0f};
    mergedIndex = zModel_Const::AddOrMergeVertexAndNormal(&mergeDi, &mergePoint,
                                                          &secondNormal);
    const bool secondMergeOk =
        mergedIndex == 1 && mergeDi.vertCount == 2 && mergeDi.blendVertCount == 2 &&
        mergeDi.verts[1].x == 1.0f && mergeDi.verts[1].y == 2.0f &&
        mergeDi.verts[1].z == 3.0f && mergeDi.blendVerts[1].x == 2.0f &&
        mergeDi.blendVerts[1].y == 2.0f && mergeDi.blendVerts[1].z == 3.0f;
    std::free(mergeDi.verts);
    std::free(mergeDi.blendVerts);
    if (!secondMergeOk) {
        return 13;
    }

    zModel_Const::SetVertexMergeEpsilon(0.25f);
    if (zModel_Const::GetVertexMergeEpsilon() != 0.25f) {
        return 99;
    }

    zModel_Const::SetVertexMergeEpsilon(0.001f);
    zDiPartial vertexDi = {};
    zVec3 vertexPoint = {1.0f, 2.0f, 3.0f};
    int vertexIndex = zModel_Const::AddOrMergeVertex(&vertexDi, &vertexPoint);
    const bool firstVertexOk =
        vertexIndex == 0 && vertexDi.vertCount == 1 && vertexDi.verts != nullptr &&
        vertexDi.verts[0].x == 1.0f && vertexDi.verts[0].y == 2.0f &&
        vertexDi.verts[0].z == 3.0f;
    if (!firstVertexOk) {
        std::free(vertexDi.verts);
        return 14;
    }

    zVec3 nearVertexPoint = {1.0005f, 1.9995f, 3.0005f};
    vertexIndex = zModel_Const::AddOrMergeVertex(&vertexDi, &nearVertexPoint);
    if (vertexIndex != 0 || vertexDi.vertCount != 1) {
        std::free(vertexDi.verts);
        return 15;
    }

    zVec3 farVertexPoint = {1.01f, 2.0f, 3.0f};
    vertexIndex = zModel_Const::AddOrMergeVertex(&vertexDi, &farVertexPoint);
    const bool secondVertexOk =
        vertexIndex == 1 && vertexDi.vertCount == 2 && vertexDi.verts[1].x == 1.01f &&
        vertexDi.verts[1].y == 2.0f && vertexDi.verts[1].z == 3.0f;
    std::free(vertexDi.verts);
    if (!secondVertexOk) {
        return 16;
    }

    zDiPartial normalDi = {};
    zVec3 normalPoint = {0.0f, 1.0f, 0.0f};
    int normalIndex = zModel_Const::FindOrAppendNormalIndex(&normalDi, &normalPoint);
    const bool firstNormalOk =
        normalIndex == 0 && normalDi.normalCount == 1 && normalDi.normals != nullptr &&
        normalDi.normals[0].x == 0.0f && normalDi.normals[0].y == 1.0f &&
        normalDi.normals[0].z == 0.0f;
    if (!firstNormalOk) {
        std::free(normalDi.normals);
        return 17;
    }

    zVec3 nearNormal = {0.00005f, 0.99995f, 0.0f};
    normalIndex = zModel_Const::FindOrAppendNormalIndex(&normalDi, &nearNormal);
    if (normalIndex != 0 || normalDi.normalCount != 1) {
        std::free(normalDi.normals);
        return 18;
    }

    zVec3 farNormal = {0.001f, 1.0f, 0.0f};
    normalIndex = zModel_Const::FindOrAppendNormalIndex(&normalDi, &farNormal);
    const bool secondNormalOk =
        normalIndex == 1 && normalDi.normalCount == 2 &&
        normalDi.normals[1].x == 0.001f && normalDi.normals[1].y == 1.0f &&
        normalDi.normals[1].z == 0.0f;
    std::free(normalDi.normals);
    if (!secondNormalOk) {
        return 19;
    }

    zClipUV gradientA = zModel_Const::SolveTriScalarGradient2D(
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
    zClipUV gradientB = zModel_Const::SolveTriScalarGradient2D(
        0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
    zClipUV degenerateGradient = zModel_Const::SolveTriScalarGradient2D(
        0.0f, 0.0f, 1.0f, 0.0f, 2.0f, 0.0f, 0.0f, 1.0f, 2.0f);
    if (std::fabs(gradientA.u - 1.0f) > 0.00001f ||
        std::fabs(gradientA.v) > 0.00001f ||
        std::fabs(gradientB.u) > 0.00001f ||
        std::fabs(gradientB.v - 1.0f) > 0.00001f ||
        degenerateGradient.u != 0.0f || degenerateGradient.v != 0.0f) {
        return 20;
    }

    zDiPartial uvDi = {};
    zModel_MaterialPartial uvMaterial = {};
    uvMaterial.flags = 0x0100;
    zVec3 uvVerts[4] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    int uvIndices[4] = {0, 1, 2, 3};
    zClipUV generatedUvs[4] = {
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {0.0f, 1.0f},
        {99.0f, 99.0f},
    };
    zDiEntryPartial uvEntry = {};
    uvEntry.flagsAndIndexCount = 4;
    uvEntry.vertexIndices = uvIndices;
    uvEntry.uvPairs = generatedUvs;
    uvEntry.material = &uvMaterial;
    uvDi.entries = &uvEntry;
    uvDi.verts = uvVerts;
    zDi::RebuildGeneratedUvPairsForEntry(&uvDi, 0);
    if (std::fabs(generatedUvs[3].u - 1.0f) > 0.00001f ||
        std::fabs(generatedUvs[3].v - 1.0f) > 0.00001f) {
        return 21;
    }

    zClipUV quantizedUvs[3] = {
        {2.001f, 3.001f},
        {3.0f, 4.0f},
        {2.499f, 3.251f},
    };
    zModel_Const::QuantizeAndNormalizeUvPairs(3, quantizedUvs);
    if (std::fabs(quantizedUvs[0].u) > 0.00001f ||
        std::fabs(quantizedUvs[0].v) > 0.00001f ||
        std::fabs(quantizedUvs[1].u - 1.0f) > 0.00001f ||
        std::fabs(quantizedUvs[1].v - 1.0f) > 0.00001f ||
        std::fabs(quantizedUvs[2].u - 0.5f) > 0.00001f ||
        std::fabs(quantizedUvs[2].v - 0.25f) > 0.00001f) {
        return 22;
    }

    zDiPartial blendDi = {};
    blendDi.flags = 0x40;
    blendDi.vertCount = 5;
    int blendEntry0Indices[3] = {0, 1, 2};
    int blendEntry1Indices[3] = {1, 2, 3};
    int blendEntry2Indices[2] = {2, 4};
    zDiEntryPartial blendEntries[3] = {};
    blendEntries[0].flagsAndIndexCount = 3;
    blendEntries[0].vertexIndices = blendEntry0Indices;
    blendEntries[1].flagsAndIndexCount = 3;
    blendEntries[1].vertexIndices = blendEntry1Indices;
    blendEntries[2].flagsAndIndexCount = 2;
    blendEntries[2].vertexIndices = blendEntry2Indices;
    blendDi.entries = blendEntries;
    blendDi.entryCount = 3;
    int excludedBlendVertices[1] = {3};
    zDi::BuildBlendVertsFromConnectivity(&blendDi, excludedBlendVertices, 0.75f, 1, 2);
    const bool blendOk =
        blendDi.blendVerts != nullptr && blendDi.blendVertCount == 5 &&
        blendDi.blendScale == 1.0f && (blendDi.flags & 0x48) == 0x48 &&
        blendDi.blendVerts[0].x == 0.0f && blendDi.blendVerts[0].y == 0.0f &&
        blendDi.blendVerts[0].z == 0.0f &&
        blendDi.blendVerts[1].x == 0.0f && blendDi.blendVerts[1].y == 0.75f &&
        blendDi.blendVerts[1].z == 0.0f &&
        blendDi.blendVerts[2].x == 0.0f && blendDi.blendVerts[2].y == 0.75f &&
        blendDi.blendVerts[2].z == 0.0f &&
        blendDi.blendVerts[3].x == 0.0f && blendDi.blendVerts[3].y == 0.0f &&
        blendDi.blendVerts[3].z == 0.0f &&
        blendDi.blendVerts[4].x == 0.0f && blendDi.blendVerts[4].y == 0.0f &&
        blendDi.blendVerts[4].z == 0.0f;
    std::free(blendDi.blendVerts);
    if (!blendOk) {
        return 23;
    }

    zDiPartial addDi = {};
    zModel_MaterialPartial addMaterial = {};
    addMaterial.flags = 0x0100;
    zVec3 addPoints[4] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    zVec3 addEntryNormals[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f},
    };
    zVec3 addNormalsA[4] = {};
    zVec3 addNormalsB[4] = {
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
    };
    zClipUV addUvs[4] = {
        {2.0f, 3.0f},
        {3.0f, 3.0f},
        {2.0f, 4.0f},
        {99.0f, 99.0f},
    };
    const int addTag = 0x3456;
    const int addResult =
        zDi::AddPolygonEx(&addDi, 4, addPoints, addEntryNormals, addUvs, addNormalsA,
                          addNormalsB, nullptr, &addMaterial, 0x77, 1, &addTag);
    const zDiEntryPartial *addEntry = addDi.entries;
    const int *addVertexIndices =
        addEntry != nullptr ? static_cast<const int *>(addEntry->vertexIndices) : nullptr;
    const int *addNormalIndices =
        addEntry != nullptr ? static_cast<const int *>(addEntry->normalIndices) : nullptr;
    const zClipUV *addEntryUvs =
        addEntry != nullptr ? static_cast<const zClipUV *>(addEntry->uvPairs) : nullptr;
    const bool addOk =
        addResult == 0 && addDi.entryCount == 1 && addDi.vertCount == 4 &&
        addDi.normalCount == 1 && addDi.blendVertCount == 4 && addEntry != nullptr &&
        addVertexIndices != nullptr && addNormalIndices != nullptr && addEntryUvs != nullptr &&
        addEntry->drawFlags == 0x77 &&
        (addEntry->flagsAndIndexCount & 0x03ff) == 0x0304 &&
        addEntry->material == &addMaterial &&
        addEntry->variantTagInitialized == 0x56 && addEntry->variantTag == 0x34 &&
        addVertexIndices[0] == 0 && addVertexIndices[1] == 1 &&
        addVertexIndices[2] == 2 && addVertexIndices[3] == 3 &&
        addNormalIndices[0] == 0 && addNormalIndices[1] == 0 &&
        addNormalIndices[2] == 0 && addNormalIndices[3] == 0 &&
        addDi.blendVerts[0].z == 1.0f &&
        std::fabs(addEntryUvs[0].u) < 0.00001f &&
        std::fabs(addEntryUvs[0].v) < 0.00001f &&
        std::fabs(addEntryUvs[3].u - 1.0f) < 0.00001f &&
        std::fabs(addEntryUvs[3].v - 1.0f) < 0.00001f;
    if (addEntry != nullptr) {
        std::free(addEntry->vertexIndices);
        std::free(addEntry->normalIndices);
        std::free(addEntry->uvPairs);
    }
    std::free(addDi.entries);
    std::free(addDi.verts);
    std::free(addDi.normals);
    std::free(addDi.blendVerts);
    if (!addOk) {
        return 24;
    }

    return 0;
}

extern "C" int zdi_add_polygon_wrapper_smoke() {
    zDiPartial addDi = {};
    zModel_MaterialPartial addMaterial = {};
    addMaterial.flags = 0x0100;
    zVec3 points[4] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 1.0f, 0.0f},
    };
    zVec3 normalsA[4] = {};
    zVec3 normalsB[4] = {
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
        {1.0f, 1.0f, 1.0f},
    };
    zClipUV uvs[4] = {
        {4.0f, 5.0f},
        {5.0f, 5.0f},
        {4.0f, 6.0f},
        {99.0f, 99.0f},
    };
    const int tag = 0x4567;

    const int result = zDi::AddPolygon(&addDi, 4, points, uvs, normalsA, normalsB, nullptr,
                                       &addMaterial, 0x88, 1, &tag);

    const zDiEntryPartial *const entry = addDi.entries;
    const int *const vertexIndices =
        entry != nullptr ? static_cast<const int *>(entry->vertexIndices) : nullptr;
    const zClipUV *const entryUvs =
        entry != nullptr ? static_cast<const zClipUV *>(entry->uvPairs) : nullptr;

    const bool ok = result == 0 && addDi.entryCount == 1 && addDi.vertCount == 4 &&
                    addDi.normalCount == 0 && addDi.blendVertCount == 4 && entry != nullptr &&
                    vertexIndices != nullptr && entry->normalIndices == nullptr &&
                    entryUvs != nullptr && entry->drawFlags == 0x88 &&
                    (entry->flagsAndIndexCount & 0x03ff) == 0x0104 &&
                    entry->material == &addMaterial &&
                    entry->variantTagInitialized == 0x67 && entry->variantTag == 0x45 &&
                    vertexIndices[0] == 0 && vertexIndices[1] == 1 &&
                    vertexIndices[2] == 2 && vertexIndices[3] == 3 &&
                    addDi.blendVerts[0].z == 1.0f &&
                    std::fabs(entryUvs[0].u) < 0.00001f &&
                    std::fabs(entryUvs[0].v) < 0.00001f &&
                    std::fabs(entryUvs[3].u - 1.0f) < 0.00001f &&
                    std::fabs(entryUvs[3].v - 1.0f) < 0.00001f;

    if (entry != nullptr) {
        std::free(entry->vertexIndices);
        std::free(entry->normalIndices);
        std::free(entry->uvPairs);
    }
    std::free(addDi.entries);
    std::free(addDi.verts);
    std::free(addDi.normals);
    std::free(addDi.blendVerts);

    return ok ? 0 : 1;
}

extern "C" int zmodel_damage_mask_uv_smoke() {
    int slotA = 0;
    int slotB = 0;
    int slotMissing = 0;
    g_OptCatalogDamageMaskHandles[0] = &slotA;
    g_OptCatalogDamageMaskHandles[1] = nullptr;
    g_OptCatalogDamageMaskHandles[2] = &slotB;

    const bool registeredOk = OptCatalog_IsDamageMaskSlotPtrRegistered(&slotA) == 1 &&
                              OptCatalog_IsDamageMaskSlotPtrRegistered(&slotB) == 1 &&
                              OptCatalog_IsDamageMaskSlotPtrRegistered(nullptr) == 1 &&
                              OptCatalog_IsDamageMaskSlotPtrRegistered(&slotMissing) == 0;

    g_OptCatalogDamageMaskPhaseU = 0.0f;
    g_OptCatalogDamageMaskPhaseV = 0.0f;
    g_OptCatalogDamageMaskEnabled = 0;

    OptCatalog_SetDamageMaskUv(0.25f, 0.75f);
    OptCatalog_SetDamageMaskEnabled(7);

    g_OptCatalogDamageMaskHandles[0] = nullptr;
    g_OptCatalogDamageMaskHandles[1] = nullptr;
    g_OptCatalogDamageMaskHandles[2] = nullptr;

    return registeredOk && g_OptCatalogDamageMaskPhaseU == 0.25f &&
                   g_OptCatalogDamageMaskPhaseV == 0.75f &&
                   OptCatalog_IsDamageMaskEnabled() == 7
               ? 0
               : 1;
}

extern "C" int zmodel_damage_mask_stamp_smoke() {
    void *const oldSlot0 = g_OptCatalogDamageMaskHandles[0];
    void *const oldSlot1 = g_OptCatalogDamageMaskHandles[1];
    void *const oldSlot2 = g_OptCatalogDamageMaskHandles[2];
    const int oldEnabled = g_OptCatalogDamageMaskEnabled;
    const int oldSlotIndex = g_OptCatalogDamageMaskSlotIndex;
    const float oldPhaseU = g_OptCatalogDamageMaskPhaseU;
    const float oldPhaseV = g_OptCatalogDamageMaskPhaseV;
    const int oldGreenBits = zRndr::g_pixelPackGreenBits;
    const unsigned int oldLock = g_zVideo_pfnTextureRecordLockUploadSurface;
    const unsigned int oldUnlock = g_zVideo_pfnTextureRecordUnlockUploadSurface;
    const unsigned int oldFinalize = g_zVideo_pfnTextureRecordFinalizeUpload;

    unsigned short srcPixels[9] = {0, 0xf800, 0x07e0, 0x001f, 0x7fff,
                                   0xffff, 0x1234, 0x2222, 0x3333};
    OptCatalogDamageMaskSurface srcSurface = {};
    srcSurface.width = 3;
    srcSurface.height = 3;
    srcSurface.pixels = srcPixels;
    OptCatalogSurfaceTextureHandle srcHandle = {};
    srcHandle.surface = &srcSurface;

    unsigned short dstPixels[25] = {};
    for (int i = 0; i < 25; ++i) {
        dstPixels[i] = 0x1111;
    }
    OptCatalogDamageMaskSurface dstSurface = {};
    dstSurface.width = 5;
    dstSurface.height = 5;
    dstSurface.pixels = dstPixels;
    OptCatalogSurfaceTextureHandle dstHandle = {};
    dstHandle.surface = &dstSurface;
    OptCatalogSurfaceMaterialRef material = {};
    material.flags = 0x0300;
    material.textureHandle = &dstHandle;
    OptCatalogHitEventPartial hitEvent = {};
    hitEvent.surfaceRef = &material;

    g_OptCatalogDamageMaskEnabled = 0;
    OptCatalog::SetDamageMaskSlotIndex(1);
    OptCatalog::RegisterDamageMaskSlotPtr(&srcHandle);
    if (g_OptCatalogDamageMaskEnabled != 1 || g_OptCatalogDamageMaskSlotIndex != 1 ||
        g_OptCatalogDamageMaskHandles[1] != &srcHandle) {
        return 1;
    }

    g_OptCatalogDamageMaskPhaseU = 0.5f;
    g_OptCatalogDamageMaskPhaseV = 0.5f;
    OptCatalog::ApplyDamageMaskStampOnHit(&hitEvent);
    if (dstPixels[6] != 0x1111 || dstPixels[7] != 0xf800 || dstPixels[8] != 0x07e0 ||
        dstPixels[11] != 0x001f || dstPixels[12] != 0x7fff || dstPixels[13] != 0xffff ||
        dstPixels[16] != 0x1234 || dstPixels[17] != 0x2222 || dstPixels[18] != 0x3333) {
        return 2;
    }

    unsigned char alphaOne = 0x80;
    unsigned short alphaSrcPixel = 0xf800;
    unsigned short alphaDstPixel = 0x001f;
    srcSurface.width = 1;
    srcSurface.height = 1;
    srcSurface.pixels = &alphaSrcPixel;
    srcSurface.alpha = &alphaOne;
    dstSurface.width = 1;
    dstSurface.height = 1;
    dstSurface.pixels = &alphaDstPixel;
    dstHandle.textureRecord = nullptr;
    g_OptCatalogDamageMaskPhaseU = 0.5f;
    g_OptCatalogDamageMaskPhaseV = 0.5f;
    zRndr::g_pixelPackGreenBits = 6;
    OptCatalog::ApplyDamageMaskStampOnHit(&hitEvent);
    if (alphaDstPixel != 0x780f) {
        return 3;
    }

    alphaSrcPixel = 0x7c00;
    alphaDstPixel = 0x001f;
    zRndr::g_pixelPackGreenBits = 5;
    OptCatalog::ApplyDamageMaskStampOnHit(&hitEvent);
    if (alphaDstPixel != 0x3c0f) {
        return 4;
    }

    unsigned short uploadSrcPixel = 0x4567;
    unsigned short uploadPixels[4] = {0, 0, 0, 0};
    zVideo_TextureRecordPartial textureRecord = {};
    srcSurface.alpha = nullptr;
    srcSurface.pixels = &uploadSrcPixel;
    dstSurface.width = 2;
    dstSurface.height = 2;
    dstSurface.pixels = nullptr;
    dstHandle.textureRecord = &textureRecord;
    g_damageMaskUploadPixels = uploadPixels;
    g_damageMaskUploadPitchBytes = 4;
    g_damageMaskUploadLockCount = 0;
    g_damageMaskUploadUnlockCount = 0;
    g_damageMaskUploadFinalizeCount = 0;
    g_damageMaskLastTextureRecord = nullptr;
    g_zVideo_pfnTextureRecordLockUploadSurface =
        static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&DamageMaskLockUploadStub));
    g_zVideo_pfnTextureRecordUnlockUploadSurface =
        static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&DamageMaskUnlockUploadStub));
    g_zVideo_pfnTextureRecordFinalizeUpload =
        static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&DamageMaskFinalizeUploadStub));
    g_OptCatalogDamageMaskPhaseU = 0.5f;
    g_OptCatalogDamageMaskPhaseV = 0.5f;
    OptCatalog::ApplyDamageMaskStampOnHit(&hitEvent);
    const bool uploadOk = uploadPixels[3] == 0x4567 && g_damageMaskUploadLockCount == 1 &&
                          g_damageMaskUploadUnlockCount == 1 &&
                          g_damageMaskUploadFinalizeCount == 1 &&
                          g_damageMaskLastTextureRecord == &textureRecord;

    g_OptCatalogDamageMaskHandles[0] = oldSlot0;
    g_OptCatalogDamageMaskHandles[1] = oldSlot1;
    g_OptCatalogDamageMaskHandles[2] = oldSlot2;
    g_OptCatalogDamageMaskEnabled = oldEnabled;
    g_OptCatalogDamageMaskSlotIndex = oldSlotIndex;
    g_OptCatalogDamageMaskPhaseU = oldPhaseU;
    g_OptCatalogDamageMaskPhaseV = oldPhaseV;
    zRndr::g_pixelPackGreenBits = oldGreenBits;
    g_zVideo_pfnTextureRecordLockUploadSurface = oldLock;
    g_zVideo_pfnTextureRecordUnlockUploadSurface = oldUnlock;
    g_zVideo_pfnTextureRecordFinalizeUpload = oldFinalize;

    return uploadOk ? 0 : 5;
}

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

extern "C" int zmodel_material_pool_entry_smoke() {
    zModel_MaterialSlot slots[3] = {};
    g_zModel_MatlPool = slots;

    const bool ok = zModel_Matl::GetPoolEntry(-1) == nullptr &&
                    zModel_Matl::GetPoolEntry(0) == &slots[0] &&
                    zModel_Matl::GetPoolEntry(2) == &slots[2] &&
                    zModel_MatlSlot::IndexFromPtrOrMinus1(nullptr) == -1 &&
                    zModel_MatlSlot::IndexFromPtrOrMinus1(&slots[2]) == 2;

    g_zModel_MatlPool = nullptr;
    return ok ? 0 : 1;
}

extern "C" int zmodel_matl_init_globals_smoke() {
    if (g_zModel_MatlPool != nullptr) {
        std::free(g_zModel_MatlPool);
    }
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 3;
    g_zModel_MatlPoolInUseCount = 77;
    g_zModel_MatlFreeHeadIndex = 88;
    g_zModel_MatlActiveHeadIndex = 99;
    g_zModel_DefaultMaterial.flags = 0xffff;
    g_zModel_DefaultMaterial.colorRgb.red = -1.0f;

    if (zModel_Matl::InitGlobals() != 0) {
        return 1;
    }

    const bool poolOk = g_zModel_MatlPool != nullptr && g_zModel_MatlPoolCapacity == 3 &&
                        g_zModel_MatlFreeHeadIndex == 0 &&
                        g_zModel_MatlActiveHeadIndex == -1 &&
                        g_zModel_MatlPoolInUseCount == 0 &&
                        g_zModel_MatlPool[0].prevPoolIndex == -1 &&
                        g_zModel_MatlPool[0].nextPoolIndex == 1 &&
                        g_zModel_MatlPool[1].prevPoolIndex == 0 &&
                        g_zModel_MatlPool[1].nextPoolIndex == 2 &&
                        g_zModel_MatlPool[2].prevPoolIndex == 1 &&
                        g_zModel_MatlPool[2].nextPoolIndex == -1;

    const bool defaultsOk = g_zModel_DefaultMaterial.flags == 0xf8ff &&
                            g_zModel_DefaultMaterial.packedColor == 0x7fff &&
                            g_zModel_DefaultMaterial.colorRgb.red == 255.0f &&
                            g_zModel_DefaultMaterial.colorRgb.green == 255.0f &&
                            g_zModel_DefaultMaterial.colorRgb.blue == 255.0f &&
                            g_zModel_DefaultMaterial.colorScalar == 0.5f &&
                            g_zModel_DefaultMaterial.unknown_1c == 0.5f &&
                            g_zModel_DefaultMaterial.currentTextureDirectoryEntry == nullptr &&
                            g_zModel_DefaultMaterial.cycle == nullptr;

    std::free(g_zModel_MatlPool);
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    return poolOk && defaultsOk ? 0 : 2;
}

extern "C" int zmodel_matlbuffer_set_array_size_smoke() {
    g_zModel_MatlPoolCapacity = 0;
    zModel_MatlBuffer::SetArraySize(12);
    const bool setOk = g_zModel_MatlPoolCapacity == 12;

    zModel_MatlBuffer::SetArraySize(13);
    const bool alreadySetOk = g_zModel_MatlPoolCapacity == 12;

    g_zModel_MatlPoolCapacity = 0;
    zModel_MatlBuffer::SetArraySize(32768);
    const bool limitOk = g_zModel_MatlPoolCapacity == 0;

    zModel_MatlBuffer::SetArraySize(-4);
    const bool signedCompareOk = g_zModel_MatlPoolCapacity == -4;

    g_zModel_MatlPoolCapacity = 0;
    return setOk && alreadySetOk && limitOk && signedCompareOk ? 0 : 1;
}

extern "C" int zmodel_set_display_instance_pool_capacity_smoke() {
    g_zModel_DiPoolCapacity = 0;
    zModel::SetDisplayInstancePoolCapacity(24);
    const bool setOk = g_zModel_DiPoolCapacity == 24;

    zModel::SetDisplayInstancePoolCapacity(31);
    const bool alreadySetOk = g_zModel_DiPoolCapacity == 24;

    g_zModel_DiPoolCapacity = 0;
    zModel::SetDisplayInstancePoolCapacity(-3);
    const bool signedValueOk = g_zModel_DiPoolCapacity == -3;

    g_zModel_DiPoolCapacity = 0;
    return setOk && alreadySetOk && signedValueOk ? 0 : 1;
}

extern "C" int zmodel_set_software_path_active_smoke() {
    const int savedRendererPath = g_zVideo_ActiveRendererPath;
    const int savedSoftwareActive = g_zModel_SoftwarePathActive;

    g_zVideo_ActiveRendererPath = 0;
    g_zModel_SoftwarePathActive = 12;
    zModel::SetSoftwarePathActive(34);
    const bool softwareSetOk = g_zModel_SoftwarePathActive == 34;

    zModel::SetSoftwarePathActive(-5);
    const bool signedValueOk = g_zModel_SoftwarePathActive == -5;

    g_zVideo_ActiveRendererPath = 1;
    zModel::SetSoftwarePathActive(99);
    const bool hardwareSkippedOk = g_zModel_SoftwarePathActive == -5;

    g_zVideo_ActiveRendererPath = savedRendererPath;
    g_zModel_SoftwarePathActive = savedSoftwareActive;
    return softwareSetOk && signedValueOk && hardwareSkippedOk ? 0 : 1;
}

extern "C" int zmodel_matlbuffer_release_texture_surfaces_smoke() {
    zModel_MaterialSlot *const savedPool = g_zModel_MatlPool;
    const int savedActiveHead = g_zModel_MatlActiveHeadIndex;
    const int savedRendererPath = g_zVideo_ActiveRendererPath;
    const unsigned int savedReleaseUploadSurface =
        g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef;

    zVideo_TextureRecordPartial textureRelease{};
    zVideo_TextureRecordPartial texturePinned{};
    zVideo_TextureRecordPartial textureMissing{};
    zImage_TexDirEntryPartial entryRelease{};
    zImage_TexDirEntryPartial entryPinned{};
    zImage_TexDirEntryPartial entryMissing{};
    zModel_MaterialSlot slots[4] = {};

    entryRelease.texture = &textureRelease;
    entryPinned.texture = &texturePinned;
    entryMissing.texture = &textureMissing;

    slots[0].material.flags = 0x0100;
    slots[0].material.currentTextureDirectoryEntry = &entryRelease;
    slots[0].nextPoolIndex = 1;

    slots[1].material.flags = 0x0300;
    slots[1].material.currentTextureDirectoryEntry = &entryPinned;
    slots[1].nextPoolIndex = 2;

    slots[2].material.flags = 0x0100;
    slots[2].material.currentTextureDirectoryEntry = nullptr;
    slots[2].nextPoolIndex = 3;

    slots[3].material.flags = 0x0000;
    slots[3].material.currentTextureDirectoryEntry = &entryMissing;
    slots[3].nextPoolIndex = -1;

    g_zmodelReleaseTextureUploadCount = 0;
    g_zmodelReleaseTextureUploadLast = nullptr;
    g_zModel_MatlPool = slots;
    g_zModel_MatlActiveHeadIndex = 0;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef =
        (unsigned int)TestReleaseTextureUploadSurfaceRef;

    zModel_MatlBuffer::ReleaseTextureSurfaces();

    const bool releaseOk =
        g_zmodelReleaseTextureUploadCount == 1 &&
        g_zmodelReleaseTextureUploadLast == &textureRelease;

    g_zModel_MatlPool = savedPool;
    g_zModel_MatlActiveHeadIndex = savedActiveHead;
    g_zVideo_ActiveRendererPath = savedRendererPath;
    g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef = savedReleaseUploadSurface;
    g_zmodelReleaseTextureUploadCount = 0;
    g_zmodelReleaseTextureUploadLast = nullptr;

    return releaseOk ? 0 : 1;
}

extern "C" int zmodel_material_defaults_and_find_smoke() {
    zImage_TexDirEntryPartial texA{};
    zImage_TexDirEntryPartial texB{};
    zImage_TexDirEntryPartial texMissing{};

    zModel_MaterialCyclePartial cycle{};
    zModel_MaterialPartial material{};
    material.flags = 0xffff;
    material.packedColor = 1;
    material.colorRgb.red = 1.0f;
    material.currentTextureDirectoryEntry = &texA;
    material.unknown_14 = 7.0f;
    material.colorScalar = 8.0f;
    material.unknown_1c = 9.0f;
    material.userTag = 10;
    material.cycle = &cycle;

    zModel_Material::ResetDefaults(&material);
    const bool defaultsOk =
        material.flags == 0xf8ff && material.packedColor == 0x7fff &&
        material.colorRgb.red == 255.0f && material.colorRgb.green == 255.0f &&
        material.colorRgb.blue == 255.0f && material.currentTextureDirectoryEntry == nullptr &&
        material.unknown_14 == 0.0f && material.colorScalar == 0.5f &&
        material.unknown_1c == 0.5f && material.userTag == 0 && material.cycle == nullptr;

    zModel_MaterialSlot slots[3] = {};
    slots[0].material.currentTextureDirectoryEntry = &texA;
    slots[0].nextPoolIndex = 2;
    slots[2].material.currentTextureDirectoryEntry = &texB;
    slots[2].nextPoolIndex = -1;
    g_zModel_MatlPool = slots;
    g_zModel_MatlActiveHeadIndex = 0;

    const bool findOk = zModel_Material::FindByTexDirEntry(nullptr) == nullptr &&
                        zModel_Material::FindByTexDirEntry(&texA) == &slots[0].material &&
                        zModel_Material::FindByTexDirEntry(&texB) == &slots[2].material &&
                        zModel_Material::FindByTexDirEntry(&texMissing) == nullptr;

    zModel_MaterialPartial compareA = material;
    zModel_MaterialPartial compareB = material;
    compareA.currentTextureDirectoryEntry = &texA;
    compareB.currentTextureDirectoryEntry = &texA;
    compareA.userTag = 0;
    compareB.userTag = 77;
    const bool tagPromoteLeft =
        zModel_Material::CompareForReuse(&compareA, &compareB) == 0 && compareA.userTag == 77;
    compareA.userTag = 55;
    compareB.userTag = 0;
    const bool tagPromoteRight =
        zModel_Material::CompareForReuse(&compareA, &compareB) == 0 && compareB.userTag == 55;
    compareB.userTag = 66;
    const bool tagConflict = zModel_Material::CompareForReuse(&compareA, &compareB) != 0;
    compareB.userTag = 55;
    compareB.currentTextureDirectoryEntry = &texB;
    const bool textureMismatch = zModel_Material::CompareForReuse(&compareA, &compareB) == 1;
    compareB.currentTextureDirectoryEntry = &texA;
    compareB.colorRgb.red = 1.0f;
    const bool bytesMismatch = zModel_Material::CompareForReuse(&compareA, &compareB) != 0;

    zModel_MaterialPartial activeRequest = slots[2].material;
    activeRequest.userTag = 99;
    g_zModel_MatlReuseCache = nullptr;
    const bool findOrCloneActive =
        zModel_Material::FindOrClone(&activeRequest) == &slots[2].material &&
        slots[2].material.userTag == 99;

    zModel_MaterialPartial cacheMaterial = slots[0].material;
    zModel_MaterialPartial cacheRequest = cacheMaterial;
    g_zModel_MatlReuseCache = &cacheMaterial;
    const bool findOrCloneCache = zModel_Material::FindOrClone(&cacheRequest) == &cacheMaterial;

    zModel_MaterialSlot cloneSlots[1] = {};
    cloneSlots[0].prevPoolIndex = -1;
    cloneSlots[0].nextPoolIndex = -1;
    zModel_MaterialPartial cloneRequest = material;
    cloneRequest.currentTextureDirectoryEntry = &texMissing;
    g_zModel_MatlPool = cloneSlots;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlReuseCache = nullptr;
    zModel_MaterialPartial *const clonedMaterial = zModel_Material::FindOrClone(&cloneRequest);
    const bool findOrCloneMiss =
        clonedMaterial == &cloneSlots[0].material && g_zModel_MatlReuseCache == clonedMaterial &&
        g_zModel_MatlActiveHeadIndex == 0 && g_zModel_MatlFreeHeadIndex == -1 &&
        g_zModel_MatlPoolInUseCount == 1 &&
        clonedMaterial->currentTextureDirectoryEntry == &texMissing;

    zModel_MaterialSlot debugSlots[1] = {};
    debugSlots[0].prevPoolIndex = -1;
    debugSlots[0].nextPoolIndex = -1;
    g_zModel_MatlPool = debugSlots;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlReuseCache = nullptr;
    std::srand(1);
    zModel_MaterialPartial *const debugMaterial =
        zGeometry_Model::FindOrCreateRandomDebugMaterial();
    const int debugRed = static_cast<int>(debugMaterial->colorRgb.red);
    const int debugGreen = static_cast<int>(debugMaterial->colorRgb.green);
    const int debugBlue = static_cast<int>(debugMaterial->colorRgb.blue);
    const unsigned short debugPackedColor = static_cast<unsigned short>(
        ((debugRed & 0x1f) << 11) | ((debugGreen & 0x3f) << 5) | (debugBlue & 0x1f));
    const bool randomDebugMaterialOk =
        debugMaterial == &debugSlots[0].material &&
        g_zModel_MatlReuseCache == debugMaterial && g_zModel_MatlActiveHeadIndex == 0 &&
        g_zModel_MatlFreeHeadIndex == -1 && g_zModel_MatlPoolInUseCount == 1 &&
        debugMaterial->packedColor == debugPackedColor;

    const bool setTagOk = zModel_Material::SetUserTag(nullptr, 7) == 0 &&
                          zModel_Material::SetUserTag(&compareA, 88) == 1 && compareA.userTag == 88;

    zModel_MaterialPartial cycleMaterial = {};
    cycleMaterial.flags = 0x0001;
    const bool cycleCountDefaultReject =
        zModel_Material::SetCycleTextureCount(&g_zModel_DefaultMaterial, 2) == 0;
    const bool cycleCountOk =
        zModel_Material::SetCycleTextureCount(&cycleMaterial, 3) == 1 &&
        (cycleMaterial.flags & 0x0500) == 0x0500 && cycleMaterial.cycle != nullptr &&
        cycleMaterial.cycle->loopEnabled == 0 && cycleMaterial.cycle->currentFrame == 0.0f &&
        cycleMaterial.cycle->framesPerSecond == 15.0f && cycleMaterial.cycle->frameCount == 3 &&
        cycleMaterial.cycle->frameWriteCount == 0 && cycleMaterial.cycle->frameTable != nullptr &&
        cycleMaterial.cycle->frameTable[0] == zImage::GetDefaultImageRefPtr() &&
        reinterpret_cast<zVidImagePartial **>(zImage::GetDefaultImageRefPtr())[0] ==
            &zVid_Image::g_zImage_DefaultImage;
    const bool cycleNoResize = zModel_Material::SetCycleTextureCount(&cycleMaterial, 2) == 0;
    zImage_TexDirEntryPartial cycleTexture = {};
    const bool cycleAddOk = zModel_Material::AddCycleTexture(&cycleMaterial, &cycleTexture) == 1 &&
                            cycleMaterial.cycle->frameTable[0] == &cycleTexture &&
                            cycleMaterial.cycle->frameWriteCount == 1;
    const bool cycleLoopSpeedOk =
        zModel_Material::SetCycleTextureLoop(&cycleMaterial, 1) == 1 &&
        cycleMaterial.cycle->loopEnabled == 1 &&
        zModel_Material::SetCycleTextureSpeed(&cycleMaterial, 7.5f) == 1 &&
        cycleMaterial.cycle->framesPerSecond == 7.5f;
    zModel_MaterialPartial nonCycleMaterial = {};
    const bool cycleRejects =
        zModel_Material::AddCycleTexture(&nonCycleMaterial, &cycleTexture) == 0 &&
        zModel_Material::SetCycleTextureLoop(nullptr, 1) == 0 &&
        zModel_Material::SetCycleTextureSpeed(nullptr, 1.0f) == 0;

    std::free(cycleMaterial.cycle->frameTable);
    std::free(cycleMaterial.cycle);

    g_zModel_MatlPool = nullptr;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlReuseCache = nullptr;
    return defaultsOk && findOk && tagPromoteLeft && tagPromoteRight && tagConflict &&
                   textureMismatch && bytesMismatch && findOrCloneActive && findOrCloneCache &&
                   findOrCloneMiss && randomDebugMaterialOk && setTagOk &&
                   cycleCountDefaultReject && cycleCountOk && cycleNoResize && cycleAddOk &&
                   cycleLoopSpeedOk && cycleRejects
               ? 0
               : 1;
}

extern "C" int zgeometry_model_add_polygon_to_di_smoke() {
    g_zModel_MaxPolygonVertexCountBeforeSplit = 64;
    g_zModel_CoplanarTolerance = 0.01;

    zDiPartial di{};
    zModel_MaterialPartial material{};
    zVec3 points[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };

    if (zGeometry_Model::AddPolygonToDi(&di, 2, points, &material, nullptr) != -1) {
        return 1;
    }

    if (zGeometry_Model::AddPolygonToDi(&di, 3, points, &material, nullptr) != 0 ||
        di.entryCount != 1 || di.vertCount != 3 || di.entries == nullptr ||
        di.entries[0].material != &material ||
        static_cast<int>(di.entries[0].flagsAndIndexCount & 0xff) != 3) {
        zDi::FreeContents(&di);
        return 2;
    }

    zDi::FreeContents(&di);
    return di.entryCount == 0 && di.entries == nullptr && di.verts == nullptr ? 0 : 3;
}

extern "C" int zgeometry_model_polygon_uv_and_di_helpers_smoke() {
    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.00001f; };

    zVec3 basisPoints[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 2.0f},
    };

    zVec2 coefficients{};
    zGeometry_Polygon::SolveUvAxisCoefficientsXZ(
        &basisPoints[0], &basisPoints[1], &basisPoints[2], 0.0f, 10.0f, 14.0f,
        &coefficients);
    if (!nearFloat(coefficients.x, 10.0f) || !nearFloat(coefficients.y, 2.0f)) {
        return 1;
    }

    zVec3 degeneratePoint{1.0f, 0.0f, 0.0f};
    zGeometry_Polygon::SolveUvAxisCoefficientsXZ(
        &degeneratePoint, &degeneratePoint, &degeneratePoint, 1.0f, 2.0f, 3.0f,
        &coefficients);
    if (coefficients.x != 0.0f || coefficients.y != 0.0f) {
        return 2;
    }

    int indices[3] = {0, 1, 2};
    zModel_PolygonUvBasis uvBasis{};
    uvBasis.uv0.u = 0.0f;
    uvBasis.uv0.v = 5.0f;
    uvBasis.uv1.u = 10.0f;
    uvBasis.uv1.v = 7.0f;
    uvBasis.uv2.u = 14.0f;
    uvBasis.uv2.v = 15.0f;

    zModel_MaterialPartial material{};
    zModel_PolygonPartial polygon{};
    polygon.vertexCountAndFlags = 0x100 | 3;
    polygon.drawFlags = 0x40;
    polygon.vertexIndices = indices;
    polygon.uvBasis = &uvBasis;
    polygon.material = &material;
    polygon.userTag = 0x12345678;

    zModel_DrawBatchBasePartial model{};
    model.verts = basisPoints;

    zVec3 polygonPoints[3] = {
        {2.0f, 1.0f, 3.0f},
        {1.0f, 0.0f, 0.0f},
        {1.0f, 1.0f, 2.0f},
    };
    zClipUV *uvList =
        zGeometry_Model::BuildPolygonUvList(3, polygonPoints, &model, &polygon);
    if (uvList == nullptr || !nearFloat(uvList[0].u, 26.0f) ||
        !nearFloat(uvList[0].v, 21.0f) || !nearFloat(uvList[1].u, 10.0f) ||
        !nearFloat(uvList[1].v, 7.0f) || !nearFloat(uvList[2].u, 14.0f) ||
        !nearFloat(uvList[2].v, 15.0f)) {
        std::free(uvList);
        return 3;
    }
    std::free(uvList);

    g_zModel_MaxPolygonVertexCountBeforeSplit = 64;
    g_zModel_CoplanarTolerance = 0.01;

    zDiPartial di{};
    if (zGeometry_Model::AddPointListPolygonToDi(&di, 2, polygonPoints, &model, &polygon) !=
        -1) {
        zDi::FreeContents(&di);
        return 4;
    }
    if (zGeometry_Model::AddPointListPolygonToDi(&di, 3, polygonPoints, &model, &polygon) !=
            0 ||
        di.entryCount != 1 || di.vertCount != 3 || di.entries == nullptr ||
        di.entries[0].material != &material ||
        static_cast<int>(di.entries[0].flagsAndIndexCount & 0xff) != 3) {
        zDi::FreeContents(&di);
        return 5;
    }
    zDi::FreeContents(&di);

    zDiPartial indexedDi{};
    if (zGeometry_Model::AddIndexedPolygonToDi(&indexedDi, &model, &polygon) != 0 ||
        indexedDi.entryCount != 1 || indexedDi.vertCount != 3 || indexedDi.entries == nullptr ||
        indexedDi.entries[0].material != &material ||
        static_cast<int>(indexedDi.entries[0].flagsAndIndexCount & 0xff) != 3) {
        zDi::FreeContents(&indexedDi);
        return 6;
    }
    zDi::FreeContents(&indexedDi);

    return indexedDi.entryCount == 0 && indexedDi.entries == nullptr && indexedDi.verts == nullptr
               ? 0
               : 7;
}

extern "C" int zgeometry_vec3array_clip_leaf_helpers_smoke() {
    zVec3 points[3] = {
        {2.0f, 3.0f, 4.0f},
        {-1.0f, 5.0f, -6.0f},
        {7.0f, -8.0f, 9.0f},
    };

    zGeometry_Vec3Array::RotatePos90AroundX(3, points);
    if (points[0].x != 2.0f || points[0].y != -4.0f || points[0].z != 3.0f ||
        points[1].x != -1.0f || points[1].y != 6.0f || points[1].z != 5.0f ||
        points[2].x != 7.0f || points[2].y != -9.0f || points[2].z != -8.0f) {
        return 1;
    }

    zGeometry_Vec3Array::RotateNeg90AroundX(3, points);
    if (points[0].x != 2.0f || points[0].y != 3.0f || points[0].z != 4.0f ||
        points[1].x != -1.0f || points[1].y != 5.0f || points[1].z != -6.0f ||
        points[2].x != 7.0f || points[2].y != -8.0f || points[2].z != 9.0f) {
        return 2;
    }

    zGeometry_BoundsXY bounds{};
    zGeometry_Vec3Array::ComputeBoundsXY(&bounds, points, 3);
    if (bounds.minX != -1.0f || bounds.maxX != 7.0f || bounds.maxY != 5.0f ||
        bounds.minY != -8.0f) {
        return 3;
    }

    zVec3 duplicateMiddle[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.005f, 0.005f, 2.0f},
        {1.0f, 0.0f, 3.0f},
        {2.0f, 0.0f, 4.0f},
    };
    if (zGeometry_Vec3Array::RemoveAdjacentDuplicatePointsXY(duplicateMiddle, 4) != 3 ||
        duplicateMiddle[0].x != 0.005f || duplicateMiddle[0].y != 0.005f ||
        duplicateMiddle[1].x != 1.0f || duplicateMiddle[2].x != 2.0f) {
        return 4;
    }

    zVec3 duplicateClosing[3] = {
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 2.0f},
        {0.005f, 0.005f, 3.0f},
    };
    if (zGeometry_Vec3Array::RemoveAdjacentDuplicatePointsXY(duplicateClosing, 3) != 2) {
        return 5;
    }

    zGeometry_WeilerBufferPartial buffer{};
    zGeometry_WeilerBuffer::Init(&buffer, 2, sizeof(zVec3));
    if (buffer.elementSize != sizeof(zVec3) || buffer.capacity != 2 || buffer.count != 0 ||
        buffer.base == nullptr || buffer.appendPtr != buffer.base) {
        zGeometry_WeilerBuffer::Destroy(&buffer);
        return 6;
    }

    zGeometry_WeilerBuffer::Destroy(&buffer);
    if (buffer.elementSize != 0 || buffer.capacity != 0 || buffer.count != 0 ||
        buffer.base != nullptr || buffer.appendPtr != nullptr) {
        return 7;
    }

    zGeometry_WeilerBuffer::Init(&buffer, 1, sizeof(zVec3));
    void *outBase = nullptr;
    void *append0 = zGeometry_WeilerBuffer::GetAppendSpace(&buffer, 1, &outBase);
    if (append0 == nullptr || outBase != buffer.base || append0 != buffer.base ||
        buffer.capacity != 18 || buffer.count != 1 ||
        buffer.appendPtr != static_cast<void *>(static_cast<unsigned char *>(buffer.base) +
                                                sizeof(zVec3))) {
        zGeometry_WeilerBuffer::Destroy(&buffer);
        return 8;
    }
    void *append1 = zGeometry_WeilerBuffer::GetAppendSpace(&buffer, 2, nullptr);
    if (append1 != static_cast<void *>(static_cast<unsigned char *>(buffer.base) + sizeof(zVec3)) ||
        buffer.count != 3 ||
        buffer.appendPtr != static_cast<void *>(static_cast<unsigned char *>(buffer.base) +
                                                sizeof(zVec3) * 3)) {
        zGeometry_WeilerBuffer::Destroy(&buffer);
        return 9;
    }
    zGeometry_WeilerBuffer::Destroy(&buffer);

    zGeometry_WeilerBuffer::Init(&buffer, 4, sizeof(zVec3));
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&buffer, 3);
    if (buffer.count != 3 ||
        buffer.appendPtr != static_cast<void *>(static_cast<unsigned char *>(buffer.base) +
                                                sizeof(zVec3) * 3)) {
        zGeometry_WeilerBuffer::Destroy(&buffer);
        return 10;
    }
    zGeometry_WeilerBuffer::Destroy(&buffer);

    zVec3 segmentPoints[3] = {
        {5.0f, -2.0f, 0.0f},
        {-1.0f, 4.0f, 0.0f},
        {2.0f, 1.0f, 0.0f},
    };
    zGeometry_WeilerContourSegmentPartial segment{};
    segment.startPoint = &segmentPoints[0];
    segment.endPoint = &segmentPoints[1];
    segment.boundsDirty = 1;
    zGeometry_WeilerContourSegment::UpdateBounds(&segment);
    if (segment.minX != -1.0f || segment.maxX != 5.0f || segment.minY != -2.0f ||
        segment.maxY != 4.0f || segment.boundsDirty != 0) {
        return 11;
    }

    zGeometry_WeilerContourSegmentPartial segments[3]{};
    zGeometry_WeilerContourSegmentArray::InitFromPointList(segments, segmentPoints, 3, 4);
    if (segments[0].prev != &segments[2] || segments[0].next != &segments[1] ||
        segments[1].prev != &segments[0] || segments[1].next != &segments[2] ||
        segments[2].prev != &segments[1] || segments[2].next != &segments[0] ||
        segments[0].startPoint != &segmentPoints[0] ||
        segments[0].endPoint != &segmentPoints[1] ||
        segments[2].startPoint != &segmentPoints[2] ||
        segments[2].endPoint != &segmentPoints[0] || segments[0].contourType != 4 ||
        segments[1].contourType != 4 || segments[2].contourType != 4 ||
        segments[0].startXing != nullptr || segments[0].endXing != nullptr ||
        segments[0].contourOutput != nullptr) {
        return 12;
    }

    zGeometry_WeilerContourSegmentArray::UpdateBounds(segments, 3);
    if (segments[0].minX != -1.0f || segments[0].maxX != 5.0f ||
        segments[1].minX != -1.0f || segments[1].maxY != 4.0f ||
        segments[2].minX != 2.0f || segments[2].maxX != 5.0f) {
        return 13;
    }

    zGeometry_WeilerStatePartial outputState{};
    zGeometry_WeilerBuffer::Init(&outputState.contourBuffer, 1,
                                 sizeof(zGeometry_WeilerContourOutputPartial));
    segments[0].contourType = 6;
    if (zGeometry_Weiler::EnsureContourOutput(&outputState, &segments[0]) != 1 ||
        outputState.contourBuffer.count != 1 || segments[0].contourOutput == nullptr ||
        segments[0].contourOutput->firstSegment != &segments[0] ||
        segments[0].contourOutput->contourType != 6) {
        zGeometry_WeilerBuffer::Destroy(&outputState.contourBuffer);
        return 14;
    }
    if (zGeometry_Weiler::EnsureContourOutput(&outputState, &segments[0]) != 1 ||
        outputState.contourBuffer.count != 1) {
        zGeometry_WeilerBuffer::Destroy(&outputState.contourBuffer);
        return 15;
    }
    zGeometry_WeilerBuffer::Destroy(&outputState.contourBuffer);

    zGeometry_WeilerStatePartial pairState{};
    zGeometry_WeilerBuffer::Init(&pairState.segmentBuffer, 1,
                                 sizeof(zGeometry_WeilerContourSegmentPartial));
    zGeometry_WeilerBuffer::Init(&pairState.contourBuffer, 1,
                                 sizeof(zGeometry_WeilerContourOutputPartial));
    if (zGeometry_Weiler::InitInputContourPair(&pairState, segmentPoints, 3, 1) != 1 ||
        pairState.segmentBuffer.count != 6 || pairState.contourBuffer.count != 2) {
        zGeometry_WeilerBuffer::Destroy(&pairState.segmentBuffer);
        zGeometry_WeilerBuffer::Destroy(&pairState.contourBuffer);
        return 16;
    }
    zGeometry_WeilerContourSegmentPartial *pairSegments =
        static_cast<zGeometry_WeilerContourSegmentPartial *>(pairState.segmentBuffer.base);
    zGeometry_WeilerContourSegmentPartial *reversePairSegments = &pairSegments[3];
    if (pairSegments[0].contourType != 1 || reversePairSegments[0].contourType != 4 ||
        pairSegments[0].contourOutput == nullptr ||
        pairSegments[0].contourOutput->firstSegment != &pairSegments[0] ||
        pairSegments[0].contourOutput->contourType != 1 ||
        reversePairSegments[0].contourOutput == nullptr ||
        reversePairSegments[0].contourOutput->firstSegment != &reversePairSegments[0] ||
        reversePairSegments[0].contourOutput->contourType != 4 ||
        pairSegments[0].minX != -1.0f || pairSegments[0].maxX != 5.0f ||
        pairSegments[0].minY != -2.0f || pairSegments[0].maxY != 4.0f ||
        pairSegments[2].next != &pairSegments[0] ||
        reversePairSegments[2].next != &reversePairSegments[0]) {
        zGeometry_WeilerBuffer::Destroy(&pairState.segmentBuffer);
        zGeometry_WeilerBuffer::Destroy(&pairState.contourBuffer);
        return 17;
    }
    zGeometry_WeilerBuffer::Destroy(&pairState.segmentBuffer);
    zGeometry_WeilerBuffer::Destroy(&pairState.contourBuffer);

    zVec3 initPoints[4] = {
        {0.0f, 0.0f, 1.0f},
        {0.005f, 0.005f, 2.0f},
        {1.0f, 0.0f, 3.0f},
        {0.0f, 1.0f, 4.0f},
    };
    zGeometry_WeilerStatePartial *initState = zGeometry_Weiler::Init(initPoints, 4, 0);
    if (initState == nullptr || initState->inputContourABuffer.count != 3 ||
        initState->inputContourBBuffer.base != nullptr || initState->inputContourBBuffer.count != 0 ||
        initState->segmentBuffer.count != 6 || initState->contourBuffer.count != 2 ||
        initState->xingBuffer.count != 0 || initState->contourSource != 0) {
        zGeometry_Weiler::DestroyState(initState);
        return 18;
    }
    zVec3 *initContourA = static_cast<zVec3 *>(initState->inputContourABuffer.base);
    zGeometry_WeilerContourSegmentPartial *initSegments =
        static_cast<zGeometry_WeilerContourSegmentPartial *>(initState->segmentBuffer.base);
    if (initContourA[0].x != 0.005f || initContourA[0].y != 0.005f ||
        initContourA[0].z != 2.0f || initContourA[1].x != 1.0f ||
        initContourA[2].x != 0.0f || initSegments[0].contourType != 1 ||
        initSegments[3].contourType != 4 || initSegments[0].contourOutput == nullptr ||
        initSegments[3].contourOutput == nullptr) {
        zGeometry_Weiler::DestroyState(initState);
        return 19;
    }

    zVec3 *inputContourPoints = nullptr;
    if (zGeometry_Weiler::GetInputContourAPointList(initState, &inputContourPoints) != 3 ||
        inputContourPoints != initContourA ||
        zGeometry_Weiler::GetInputContourAPointList(nullptr, &inputContourPoints) != 0) {
        zGeometry_Weiler::DestroyState(initState);
        return 20;
    }

    zGeometry_Weiler::DestroyState(initState);

    zGeometry_WeilerClipOutputPartial clipOutput{};
    clipOutput.pointList.points = static_cast<zVec3 *>(std::malloc(sizeof(zVec3)));
    clipOutput.polygonSetA.polygons = static_cast<zGeometry_PolygonPointSpanPartial *>(
        std::malloc(sizeof(zGeometry_PolygonPointSpanPartial)));
    clipOutput.polygonSetB.polygons = static_cast<zGeometry_PolygonPointSpanPartial *>(
        std::malloc(sizeof(zGeometry_PolygonPointSpanPartial)));
    clipOutput.polygonSetC.polygons = static_cast<zGeometry_PolygonPointSpanPartial *>(
        std::malloc(sizeof(zGeometry_PolygonPointSpanPartial)));
    if (clipOutput.pointList.points == nullptr || clipOutput.polygonSetA.polygons == nullptr ||
        clipOutput.polygonSetB.polygons == nullptr || clipOutput.polygonSetC.polygons == nullptr) {
        zGeometry_WeilerClipOutput::Destroy(&clipOutput);
        return 21;
    }
    zGeometry_WeilerClipOutput::Destroy(&clipOutput);
    zGeometry_WeilerClipOutput::Destroy(nullptr);
    if (clipOutput.pointList.points != nullptr || clipOutput.polygonSetA.polygons != nullptr ||
        clipOutput.polygonSetB.polygons != nullptr || clipOutput.polygonSetC.polygons != nullptr) {
        return 22;
    }

    zGeometry_ClipPolygonPartial resetClipPolygon{};
    zVec3 resetInitial[3] = {
        {10.0f, 20.0f, 30.0f},
        {20.0f, 20.0f, 40.0f},
        {10.0f, 30.0f, 50.0f},
    };
    resetClipPolygon.weilerState = zGeometry_Weiler::Init(resetInitial, 3, 2);
    if (resetClipPolygon.weilerState == nullptr) {
        return 23;
    }
    zGeometry_WeilerStatePartial *oldResetState = resetClipPolygon.weilerState;
    if (zGeometry_ClipPolygon::ResetWeilerStateFromContourPoints(&resetClipPolygon, nullptr, 3) !=
            0 ||
        resetClipPolygon.weilerState != oldResetState) {
        zGeometry_Weiler::DestroyState(resetClipPolygon.weilerState);
        return 24;
    }
    zVec3 resetReplacement[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    if (zGeometry_ClipPolygon::ResetWeilerStateFromContourPoints(&resetClipPolygon,
                                                                 resetReplacement, 3) != 1 ||
        resetClipPolygon.weilerState == nullptr || resetClipPolygon.weilerState == oldResetState ||
        resetClipPolygon.weilerState->contourSource != 2 ||
        resetClipPolygon.weilerState->inputContourABuffer.count != 3) {
        zGeometry_Weiler::DestroyState(resetClipPolygon.weilerState);
        return 25;
    }
    zGeometry_Weiler::DestroyState(resetClipPolygon.weilerState);

    zVec3 square[4] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f},
        {10.0f, 10.0f, 0.0f},
        {0.0f, 10.0f, 0.0f},
    };
    zVec3 insidePoint{5.0f, 5.0f, 0.0f};
    zVec3 edgePoint{0.0f, 5.0f, 0.0f};
    zVec3 outsidePoint{12.0f, 5.0f, 0.0f};
    if (zGeometry_Weiler::ClassifyPointInContourPointListXY(&insidePoint, 4, square) != 1 ||
        zGeometry_Weiler::ClassifyPointInContourPointListXY(&edgePoint, 4, square) != 0 ||
        zGeometry_Weiler::ClassifyPointInContourPointListXY(&outsidePoint, 4, square) != -1 ||
        zGeometry_Weiler::ClassifyPointInContourPointListXY(&insidePoint, 0, square) != -1) {
        return 25;
    }

    zVec3 biggerSquare[4] = {
        {-1.0f, -1.0f, 0.0f},
        {11.0f, -1.0f, 0.0f},
        {11.0f, 11.0f, 0.0f},
        {-1.0f, 11.0f, 0.0f},
    };
    zVec3 smallerSquare[4] = {
        {2.0f, 2.0f, 0.0f},
        {8.0f, 2.0f, 0.0f},
        {8.0f, 8.0f, 0.0f},
        {2.0f, 8.0f, 0.0f},
    };
    zVec3 shiftedSquare[4] = {
        {20.0f, 20.0f, 0.0f},
        {30.0f, 20.0f, 0.0f},
        {30.0f, 30.0f, 0.0f},
        {20.0f, 30.0f, 0.0f},
    };
    zVec3 overlappingSquare[4] = {
        {5.0f, 5.0f, 0.0f},
        {15.0f, 5.0f, 0.0f},
        {15.0f, 15.0f, 0.0f},
        {5.0f, 15.0f, 0.0f},
    };
    if (zGeometry_Weiler::OutputPreclassifiedContourPairResult(4, square, 4, square, 2) != 4 ||
        zGeometry_Weiler::OutputPreclassifiedContourPairResult(4, smallerSquare, 4, square, 2) !=
            2 ||
        zGeometry_Weiler::OutputPreclassifiedContourPairResult(4, shiftedSquare, 4, square, 2) !=
            0) {
        return 26;
    }

    zGeometry_WeilerStatePartial boundsState{};
    boundsState.inputContourABuffer.base = square;
    boundsState.inputContourABuffer.count = 4;
    boundsState.inputContourBBuffer.base = shiftedSquare;
    boundsState.inputContourBBuffer.count = 4;
    if (zGeometry_Weiler::ClassifyInputContourPairBounds(&boundsState) != 1) {
        return 27;
    }

    boundsState.inputContourBBuffer.base = biggerSquare;
    if (zGeometry_Weiler::ClassifyInputContourPairBounds(&boundsState) != 2) {
        return 28;
    }

    boundsState.inputContourBBuffer.base = smallerSquare;
    if (zGeometry_Weiler::ClassifyInputContourPairBounds(&boundsState) != 3) {
        return 29;
    }

    boundsState.inputContourBBuffer.base = square;
    if (zGeometry_Weiler::ClassifyInputContourPairBounds(&boundsState) != 4) {
        return 30;
    }

    boundsState.inputContourBBuffer.base = overlappingSquare;
    if (zGeometry_Weiler::ClassifyInputContourPairBounds(&boundsState) != 0) {
        return 31;
    }

    zGeometry_WeilerStatePartial containedState{};
    zGeometry_WeilerContourOutputPartial containedPacket[4]{};
    zGeometry_WeilerContourSegmentPartial containedSegments[4]{};
    zVec3 containedPoints[8] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {10.0f, 10.0f, 0.0f},
        {11.0f, 10.0f, 0.0f},
        {10.0f, 10.0f, 0.0f},
        {11.0f, 10.0f, 0.0f},
    };
    for (int i = 0; i < 4; ++i) {
        containedSegments[i].prev = &containedSegments[i];
        containedSegments[i].next = &containedSegments[i];
        containedSegments[i].startPoint = &containedPoints[i * 2];
        containedSegments[i].endPoint = &containedPoints[i * 2 + 1];
        containedSegments[i].boundsDirty = 1;
        zGeometry_WeilerContourSegment::UpdateBounds(&containedSegments[i]);
        containedPacket[i].firstSegment = &containedSegments[i];
    }
    containedState.contourBuffer.base = containedPacket;
    if (zGeometry_Weiler::ClassifyContainedContour(&containedState) != 0 ||
        containedState.xingBuffer.count != 0) {
        return 136;
    }

    zGeometry_WeilerClipOutputPartial clipPointOutput{};
    if (zGeometry_Weiler::ClipPointList(nullptr, 1, square, 4, &clipPointOutput) != 0) {
        return 137;
    }

    zGeometry_WeilerStatePartial *clipPointState =
        zGeometry_Weiler::Init(square, 4, 0);
    if (clipPointState == nullptr) {
        return 138;
    }
    if (zGeometry_Weiler::ClipPointList(clipPointState, 7, shiftedSquare, 4,
                                        &clipPointOutput) != 1 ||
        clipPointState->inputContourBBuffer.base != shiftedSquare ||
        clipPointState->inputContourBBuffer.count != 4 ||
        clipPointState->clipMode != 7 ||
        clipPointOutput.polygonSetA.polygonCount != 0 ||
        clipPointOutput.polygonSetB.polygonCount != 0 ||
        clipPointOutput.polygonSetC.polygonCount != 0 ||
        clipPointOutput.pointList.pointCount != 0) {
        zGeometry_WeilerClipOutput::Destroy(&clipPointOutput);
        zGeometry_Weiler::DestroyState(clipPointState);
        return 139;
    }
    zGeometry_WeilerClipOutput::Destroy(&clipPointOutput);
    zGeometry_Weiler::DestroyState(clipPointState);

    clipPointOutput = {};
    clipPointState = zGeometry_Weiler::Init(square, 4, 0);
    if (clipPointState == nullptr) {
        return 140;
    }
    if (zGeometry_Weiler::ClipPointList(clipPointState, 1, square, 4,
                                        &clipPointOutput) != 3 ||
        clipPointOutput.polygonSetA.polygonCount != 1 ||
        clipPointOutput.polygonSetA.polygons->pointCount != 4 ||
        clipPointOutput.polygonSetA.polygons->pointDwordOffset != 0 ||
        clipPointOutput.pointList.pointCount != 4 ||
        clipPointOutput.pointList.points[0].x != square[0].x ||
        clipPointOutput.pointList.points[3].y != square[3].y) {
        zGeometry_WeilerClipOutput::Destroy(&clipPointOutput);
        zGeometry_Weiler::DestroyState(clipPointState);
        return 141;
    }
    zGeometry_WeilerClipOutput::Destroy(&clipPointOutput);
    zGeometry_Weiler::DestroyState(clipPointState);

    clipPointOutput = {};
    clipPointState = zGeometry_Weiler::Init(smallerSquare, 4, 0);
    if (clipPointState == nullptr) {
        return 142;
    }
    if (zGeometry_Weiler::ClipPointList(clipPointState, 3, biggerSquare, 4,
                                        &clipPointOutput) != 4 ||
        clipPointOutput.polygonSetA.polygonCount != 1 ||
        clipPointOutput.polygonSetB.polygonCount != 1 ||
        clipPointOutput.polygonSetA.polygons->pointCount != 4 ||
        clipPointOutput.polygonSetA.polygons->pointDwordOffset != 30 ||
        clipPointOutput.polygonSetB.polygons->pointCount != 10 ||
        clipPointOutput.polygonSetB.polygons->pointDwordOffset != 0 ||
        clipPointOutput.pointList.pointCount != 14 ||
        clipPointOutput.pointList.points[0].x != biggerSquare[1].x ||
        clipPointOutput.pointList.points[10].x != smallerSquare[0].x) {
        zGeometry_WeilerClipOutput::Destroy(&clipPointOutput);
        zGeometry_Weiler::DestroyState(clipPointState);
        return 143;
    }
    zGeometry_WeilerClipOutput::Destroy(&clipPointOutput);
    zGeometry_Weiler::DestroyState(clipPointState);

    zGeometry_WeilerStatePartial selectedState{};
    zGeometry_WeilerClipOutputPartial selectedOutput{};
    zGeometry_PolygonPointSpanPartial selectedSpan{};
    zVec3 selectedPoints[4] = {
        {-5.0f, -5.0f, 0.0f},
    };
    selectedOutput.polygonSetA.polygons = &selectedSpan;
    selectedOutput.pointList.points = selectedPoints;
    selectedOutput.pointList.pointCount = 1;
    selectedState.outClip = &selectedOutput;
    selectedState.inputContourABuffer.base = smallerSquare;
    selectedState.inputContourABuffer.count = 2;
    selectedState.inputContourBBuffer.base = square;
    selectedState.inputContourBBuffer.count = 3;
    if (zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA(&selectedState, 2) != 1 ||
        selectedOutput.polygonSetA.polygonCount != 0 || selectedOutput.pointList.pointCount != 1) {
        return 38;
    }
    selectedState.clipMode = 1;
    if (zGeometry_Weiler::OutputSelectedInputContourToPolygonSetA(&selectedState, 2) != 1 ||
        selectedOutput.polygonSetA.polygonCount != 1 || selectedSpan.pointCount != 2 ||
        selectedSpan.pointDwordOffset != 3 || selectedOutput.pointList.pointCount != 3 ||
        selectedPoints[1].x != smallerSquare[0].x || selectedPoints[2].x != smallerSquare[1].x) {
        return 39;
    }

    zVec3 restoreInputB[2] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
    };
    zVec3 restoreOutputPoints[2] = {
        {7.0f, 8.0f, 9.0f},
        {10.0f, 11.0f, 12.0f},
    };
    zGeometry_WeilerClipOutputPartial restoreOutput{};
    zGeometry_WeilerStatePartial restoreState{};
    restoreState.pointTranslationX = 100.0f;
    restoreState.pointTranslationY = -50.0f;
    restoreState.inputContourBBuffer.base = restoreInputB;
    restoreState.inputContourBBuffer.count = 2;
    restoreOutput.pointList.points = restoreOutputPoints;
    restoreOutput.pointList.pointCount = 2;
    restoreState.outClip = &restoreOutput;
    zGeometry_Weiler::RestorePointTranslation(&restoreState);
    if (restoreInputB[0].x != 101.0f || restoreInputB[0].y != -48.0f ||
        restoreInputB[1].x != 104.0f || restoreInputB[1].y != -45.0f ||
        restoreOutputPoints[0].x != 107.0f || restoreOutputPoints[0].y != -42.0f ||
        restoreOutputPoints[1].x != 110.0f || restoreOutputPoints[1].y != -39.0f) {
        return 40;
    }

    zGeometry_WeilerStatePartial tableState{};
    zVec3 sideA[2] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
    };
    zVec3 sideB[2] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    tableState.inputContourABuffer.base = sideA;
    tableState.inputContourABuffer.count = 2;
    tableState.inputContourBBuffer.base = sideB;
    tableState.inputContourBBuffer.count = 2;
    zGeometry_Weiler::BuildPointSideTablesForContourPair(&tableState);
    if (tableState.contourAPointSideByContourBEdge[0] != 0.0f ||
        tableState.contourAPointSideByContourBEdge[1] != 1.0f ||
        tableState.contourAPointSideByContourBEdge[2] != 0.0f ||
        tableState.contourAPointSideByContourBEdge[3] != 0.0f ||
        tableState.contourAPointSideByContourBEdge[4] != -1.0f ||
        tableState.contourAPointSideByContourBEdge[5] != 0.0f ||
        tableState.contourBPointSideByContourAEdge[0] != 0.0f ||
        tableState.contourBPointSideByContourAEdge[1] != -1.0f ||
        tableState.contourBPointSideByContourAEdge[2] != 0.0f ||
        tableState.contourBPointSideByContourAEdge[3] != 0.0f ||
        tableState.contourBPointSideByContourAEdge[4] != 1.0f ||
        tableState.contourBPointSideByContourAEdge[5] != 0.0f) {
        return 41;
    }

    zVec3 verticalStart{2.0f, 1.0f, 0.0f};
    zVec3 verticalEnd{2.0f, 5.0f, 0.0f};
    zVec3 verticalInside{100.0f, 3.0f, 0.0f};
    zVec3 verticalOutside{100.0f, 6.0f, 0.0f};
    zVec3 horizontalStart{1.0f, 2.0f, 0.0f};
    zVec3 horizontalEnd{5.0f, 2.0f, 0.0f};
    zVec3 horizontalInside{3.0f, 100.0f, 0.0f};
    zVec3 horizontalOutside{6.0f, 100.0f, 0.0f};
    if (zGeometry_Vec3::IsBetweenEndpointsXY(&verticalInside, &verticalStart, &verticalEnd) != 1 ||
        zGeometry_Vec3::IsBetweenEndpointsXY(&verticalOutside, &verticalStart, &verticalEnd) !=
            0 ||
        zGeometry_Vec3::IsBetweenEndpointsXY(&horizontalInside, &horizontalStart,
                                             &horizontalEnd) != 1 ||
        zGeometry_Vec3::IsBetweenEndpointsXY(&horizontalOutside, &horizontalStart,
                                             &horizontalEnd) != 0) {
        return 43;
    }

    zGeometry_WeilerStatePartial preclassState{};
    zGeometry_WeilerBuffer::Init(&preclassState.segmentBuffer, 8,
                                 sizeof(zGeometry_WeilerContourSegmentPartial));
    zGeometry_WeilerBuffer::Init(&preclassState.contourBuffer, 4,
                                 sizeof(zGeometry_WeilerContourOutputPartial));
    zGeometry_WeilerBuffer::Init(&preclassState.xingBuffer, 2, sizeof(zGeometry_WeilerXingPartial));
    zGeometry_WeilerBuffer::Init(&preclassState.polygonSetABuffer, 2,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&preclassState.polygonSetBBuffer, 2,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&preclassState.polygonSetCBuffer, 2,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&preclassState.pointListBuffer, 8, sizeof(zVec3));
    preclassState.inputContourABuffer.base = square;
    preclassState.inputContourABuffer.count = 4;
    zGeometry_Weiler::PreclassifyInputContourAAdjacentEdgePairs(&preclassState);
    zGeometry_WeilerContourSegmentPartial *preclassSegments =
        static_cast<zGeometry_WeilerContourSegmentPartial *>(preclassState.segmentBuffer.base);
    zGeometry_WeilerContourOutputPartial *preclassContours =
        static_cast<zGeometry_WeilerContourOutputPartial *>(preclassState.contourBuffer.base);
    if (preclassState.segmentBuffer.count != 8 || preclassState.contourBuffer.count != 2 ||
        preclassState.xingBuffer.count != 0 || preclassState.polygonSetABuffer.count != 0 ||
        preclassState.pointListBuffer.count != 0 || preclassSegments[0].contourType != 1 ||
        preclassSegments[4].contourType != 4 ||
        preclassSegments[0].contourOutput != &preclassContours[0] ||
        preclassSegments[4].contourOutput != &preclassContours[1] ||
        preclassContours[0].firstSegment != &preclassSegments[0] ||
        preclassContours[1].firstSegment != &preclassSegments[4]) {
        zGeometry_WeilerBuffer::Destroy(&preclassState.segmentBuffer);
        zGeometry_WeilerBuffer::Destroy(&preclassState.contourBuffer);
        zGeometry_WeilerBuffer::Destroy(&preclassState.xingBuffer);
        zGeometry_WeilerBuffer::Destroy(&preclassState.polygonSetABuffer);
        zGeometry_WeilerBuffer::Destroy(&preclassState.polygonSetBBuffer);
        zGeometry_WeilerBuffer::Destroy(&preclassState.polygonSetCBuffer);
        zGeometry_WeilerBuffer::Destroy(&preclassState.pointListBuffer);
        return 42;
    }
    zGeometry_WeilerBuffer::Destroy(&preclassState.segmentBuffer);
    zGeometry_WeilerBuffer::Destroy(&preclassState.contourBuffer);
    zGeometry_WeilerBuffer::Destroy(&preclassState.xingBuffer);
    zGeometry_WeilerBuffer::Destroy(&preclassState.polygonSetABuffer);
    zGeometry_WeilerBuffer::Destroy(&preclassState.polygonSetBBuffer);
    zGeometry_WeilerBuffer::Destroy(&preclassState.polygonSetCBuffer);
    zGeometry_WeilerBuffer::Destroy(&preclassState.pointListBuffer);

    zGeometry_WeilerStatePartial splitState{};
    zGeometry_WeilerBuffer::Init(&splitState.segmentBuffer, 4,
                                 sizeof(zGeometry_WeilerContourSegmentPartial));
    zVec3 splitPoint{3.0f, 0.0f, 0.0f};
    zVec3 firstEnd{6.0f, 0.0f, 0.0f};
    zVec3 secondEnd{3.0f, 6.0f, 0.0f};
    zGeometry_WeilerContourSegmentPartial firstSegment{};
    zGeometry_WeilerContourSegmentPartial firstNext{};
    zGeometry_WeilerContourSegmentPartial secondSegment{};
    zGeometry_WeilerContourSegmentPartial secondNext{};
    zGeometry_WeilerXingPartial xing{};
    firstSegment.next = &firstNext;
    firstNext.prev = &firstSegment;
    firstSegment.contourType = 1;
    firstSegment.endPoint = &firstEnd;
    firstSegment.endXing = &xing;
    secondSegment.next = &secondNext;
    secondNext.prev = &secondSegment;
    secondSegment.contourType = 4;
    secondSegment.endPoint = &secondEnd;
    if (zGeometry_Weiler::CreateForwardSegmentPairAtPoint(&splitState, &firstSegment,
                                                          &secondSegment, &splitPoint, 8,
                                                          16) != 1 ||
        splitState.segmentBuffer.count != 2) {
        zGeometry_WeilerBuffer::Destroy(&splitState.segmentBuffer);
        return 44;
    }
    zGeometry_WeilerContourSegmentPartial *splitSegments =
        static_cast<zGeometry_WeilerContourSegmentPartial *>(splitState.segmentBuffer.base);
    if (firstSegment.next != &splitSegments[0] || firstNext.prev != &splitSegments[0] ||
        splitSegments[0].prev != &firstSegment || splitSegments[0].next != &firstNext ||
        splitSegments[0].contourType != 9 || splitSegments[0].startPoint != &splitPoint ||
        splitSegments[0].endPoint != &firstEnd || splitSegments[0].endXing != &xing ||
        splitSegments[0].contourOutput != nullptr || secondSegment.next != &splitSegments[1] ||
        secondNext.prev != &splitSegments[1] || splitSegments[1].contourType != 20 ||
        splitSegments[1].startPoint != &splitPoint || splitSegments[1].endPoint != &secondEnd) {
        zGeometry_WeilerBuffer::Destroy(&splitState.segmentBuffer);
        return 45;
    }
    zGeometry_WeilerBuffer::Destroy(&splitState.segmentBuffer);

    zGeometry_WeilerStatePartial divideState{};
    zGeometry_WeilerBuffer::Init(&divideState.segmentBuffer, 4,
                                 sizeof(zGeometry_WeilerContourSegmentPartial));
    zVec3 divideStart{0.0f, 0.0f, 0.0f};
    zVec3 divideEnd{10.0f, 0.0f, 0.0f};
    zGeometry_WeilerXingPartial divideXing{};
    divideXing.point.x = 4.0f;
    divideXing.point.y = 0.0f;
    divideXing.point.z = 2.0f;
    zGeometry_WeilerContourSegmentPartial divideSegment{};
    zGeometry_WeilerContourSegmentPartial divideOldNext{};
    divideSegment.prev = &divideOldNext;
    divideSegment.next = &divideOldNext;
    divideSegment.contourType = 3;
    divideSegment.startPoint = &divideStart;
    divideSegment.endPoint = &divideEnd;
    divideSegment.endXing = &xing;
    divideOldNext.prev = &divideSegment;
    divideOldNext.next = &divideSegment;
    if (zGeometry_Weiler::DivideContourSegmentAtPoint(&divideState, &divideXing.point,
                                                      &divideSegment, 1) != 1 ||
        divideState.segmentBuffer.count != 1) {
        zGeometry_WeilerBuffer::Destroy(&divideState.segmentBuffer);
        return 51;
    }
    zGeometry_WeilerContourSegmentPartial *divideInserted =
        static_cast<zGeometry_WeilerContourSegmentPartial *>(divideState.segmentBuffer.base);
    if (divideSegment.endPoint != &divideXing.point || divideSegment.next != divideInserted ||
        divideSegment.endXing != &divideXing || divideSegment.boundsDirty != 1 ||
        divideInserted->prev != &divideSegment || divideInserted->next != &divideOldNext ||
        divideInserted->startPoint != &divideXing.point || divideInserted->endPoint != &divideEnd ||
        divideInserted->contourType != 3 || divideInserted->contourOutput != nullptr ||
        divideInserted->endXing != &xing || divideInserted->startXing != &divideXing ||
        divideInserted->boundsDirty != 1 || divideOldNext.prev != divideInserted) {
        zGeometry_WeilerBuffer::Destroy(&divideState.segmentBuffer);
        return 52;
    }
    zGeometry_WeilerBuffer::Destroy(&divideState.segmentBuffer);

    zGeometry_WeilerStatePartial preclassPairState{};
    zGeometry_WeilerBuffer::Init(&preclassPairState.segmentBuffer, 8,
                                 sizeof(zGeometry_WeilerContourSegmentPartial));
    zGeometry_WeilerBuffer::Init(&preclassPairState.contourBuffer, 4,
                                 sizeof(zGeometry_WeilerContourOutputPartial));
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&preclassPairState.segmentBuffer, 4);
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&preclassPairState.contourBuffer, 4);
    zGeometry_WeilerContourSegmentPartial *pairPreclassSegments =
        static_cast<zGeometry_WeilerContourSegmentPartial *>(preclassPairState.segmentBuffer.base);
    zGeometry_WeilerContourOutputPartial *pairPreclassContours =
        static_cast<zGeometry_WeilerContourOutputPartial *>(preclassPairState.contourBuffer.base);
    zVec3 pairAStart{0.0f, 0.0f, 0.0f};
    zVec3 pairAEnd{10.0f, 0.0f, 0.0f};
    zVec3 pairCStart{3.0f, 0.0f, 0.0f};
    zVec3 pairCEnd{7.0f, 0.0f, 0.0f};
    pairPreclassSegments[0].prev = &pairPreclassSegments[0];
    pairPreclassSegments[0].next = &pairPreclassSegments[0];
    pairPreclassSegments[0].contourType = 1;
    pairPreclassSegments[0].startPoint = &pairAStart;
    pairPreclassSegments[0].endPoint = &pairAEnd;
    pairPreclassSegments[1].prev = &pairPreclassSegments[1];
    pairPreclassSegments[1].next = &pairPreclassSegments[1];
    pairPreclassSegments[1].contourType = 4;
    pairPreclassSegments[1].startPoint = &pairAEnd;
    pairPreclassSegments[1].endPoint = &pairAStart;
    pairPreclassSegments[2].prev = &pairPreclassSegments[2];
    pairPreclassSegments[2].next = &pairPreclassSegments[2];
    pairPreclassSegments[2].contourType = 2;
    pairPreclassSegments[2].startPoint = &pairCStart;
    pairPreclassSegments[2].endPoint = &pairCEnd;
    pairPreclassSegments[3].prev = &pairPreclassSegments[3];
    pairPreclassSegments[3].next = &pairPreclassSegments[3];
    pairPreclassSegments[3].contourType = 8;
    pairPreclassSegments[3].startPoint = &pairCEnd;
    pairPreclassSegments[3].endPoint = &pairCStart;
    pairPreclassContours[0].firstSegment = &pairPreclassSegments[0];
    pairPreclassContours[1].firstSegment = &pairPreclassSegments[1];
    pairPreclassContours[2].firstSegment = &pairPreclassSegments[2];
    pairPreclassContours[3].firstSegment = &pairPreclassSegments[3];
    preclassPairState.inputContourABuffer.count = 1;
    preclassPairState.inputContourBBuffer.count = 1;
    if (zGeometry_Weiler::PreclassifyInputContourPair(&preclassPairState) != 1 ||
        preclassPairState.segmentBuffer.count != 6) {
        zGeometry_WeilerBuffer::Destroy(&preclassPairState.segmentBuffer);
        zGeometry_WeilerBuffer::Destroy(&preclassPairState.contourBuffer);
        return 46;
    }
    pairPreclassSegments =
        static_cast<zGeometry_WeilerContourSegmentPartial *>(preclassPairState.segmentBuffer.base);
    if (pairPreclassSegments[0].endPoint != &pairCStart ||
        pairPreclassSegments[1].endPoint != &pairCStart ||
        pairPreclassSegments[2].contourType != 3 ||
        pairPreclassSegments[3].contourType != 12 ||
        pairPreclassSegments[0].next != &pairPreclassSegments[4] ||
        pairPreclassSegments[4].prev != &pairPreclassSegments[0] ||
        pairPreclassSegments[4].next != &pairPreclassSegments[0] ||
        pairPreclassSegments[4].startPoint != &pairCEnd ||
        pairPreclassSegments[4].endPoint != &pairAEnd ||
        pairPreclassSegments[1].next != &pairPreclassSegments[5] ||
        pairPreclassSegments[5].prev != &pairPreclassSegments[1] ||
        pairPreclassSegments[5].next != &pairPreclassSegments[1] ||
        pairPreclassSegments[5].startPoint != &pairCEnd ||
        pairPreclassSegments[5].endPoint != &pairAStart) {
        zGeometry_WeilerBuffer::Destroy(&preclassPairState.segmentBuffer);
        zGeometry_WeilerBuffer::Destroy(&preclassPairState.contourBuffer);
        return 47;
    }
    zGeometry_WeilerBuffer::Destroy(&preclassPairState.segmentBuffer);
    zGeometry_WeilerBuffer::Destroy(&preclassPairState.contourBuffer);

    zGeometry_WeilerStatePartial intersectState{};
    zVec3 intersectEdge0Start{0.0f, 0.0f, 0.0f};
    zVec3 intersectEdge0End{10.0f, 0.0f, 0.0f};
    zVec3 intersectEdge1Start{5.0f, -5.0f, 2.0f};
    zVec3 intersectEdge1End{5.0f, 5.0f, 8.0f};
    zVec3 disjointEdge1Start{15.0f, -5.0f, 0.0f};
    zVec3 disjointEdge1End{15.0f, 5.0f, 0.0f};
    if (zGeometry_Weiler::ClassifyIntersect2d(&intersectEdge0Start, &intersectEdge0End,
                                              &intersectEdge1Start, &intersectEdge1End,
                                              &intersectState) != 5 ||
        zGeometry_Weiler::ClassifyIntersect2d(&intersectEdge0Start, &intersectEdge0End,
                                              &disjointEdge1Start, &disjointEdge1End,
                                              &intersectState) != 0) {
        return 48;
    }

    zGeometry_WeilerBuffer::Init(&intersectState.xingBuffer, 2, sizeof(zGeometry_WeilerXingPartial));
    zGeometry_WeilerXingPartial *createdXing = nullptr;
    if (zGeometry_Weiler::Intersect2d(&intersectState, &createdXing, intersectEdge0Start,
                                      intersectEdge0End, intersectEdge1Start,
                                      intersectEdge1End) != 5 ||
        createdXing == nullptr || intersectState.xingBuffer.count != 1 ||
        createdXing->xingType != 5 || createdXing->point.x != 5.0f ||
        createdXing->point.y != 0.0f || createdXing->point.z != 5.0f) {
        zGeometry_WeilerBuffer::Destroy(&intersectState.xingBuffer);
        return 49;
    }

    createdXing = reinterpret_cast<zGeometry_WeilerXingPartial *>(1);
    if (zGeometry_Weiler::Intersect2d(&intersectState, &createdXing, intersectEdge0Start,
                                      intersectEdge0End, disjointEdge1Start,
                                      disjointEdge1End) != 0 ||
        createdXing != nullptr || intersectState.xingBuffer.count != 1) {
        zGeometry_WeilerBuffer::Destroy(&intersectState.xingBuffer);
        return 50;
    }
    zGeometry_WeilerBuffer::Destroy(&intersectState.xingBuffer);

    zVec3 adjacentCorner{1.0f, 0.0f, 0.0f};
    zVec3 adjacentFirstStart{0.0f, -1.0f, 0.0f};
    zVec3 adjacentSecondEnd{2.0f, 1.0f, 0.0f};
    zGeometry_WeilerContourSegmentPartial adjacentFirst{};
    zGeometry_WeilerContourSegmentPartial adjacentSecond{};
    zGeometry_WeilerContourSegmentPartial contourSegment{};
    adjacentFirst.startPoint = &adjacentFirstStart;
    adjacentFirst.endPoint = &adjacentCorner;
    adjacentSecond.startPoint = &adjacentCorner;
    adjacentSecond.endPoint = &adjacentSecondEnd;
    contourSegment.startPoint = &intersectEdge0Start;
    contourSegment.endPoint = &intersectEdge0End;
    if (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
            &adjacentFirst, &adjacentSecond, &contourSegment) != 7) {
        return 53;
    }
    adjacentSecondEnd.y = -1.0f;
    if (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
            &adjacentFirst, &adjacentSecond, &contourSegment) != 1) {
        return 54;
    }
    adjacentFirstStart.y = 1.0f;
    adjacentSecondEnd.y = 1.0f;
    if (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstContourSegment(
            &adjacentFirst, &adjacentSecond, &contourSegment) != 2) {
        return 55;
    }

    zVec3 pairCornerA{1.0f, 0.0f, 0.0f};
    zVec3 pairStartA{0.0f, 0.0f, 0.0f};
    zVec3 pairEndA{1.0f, 1.0f, 0.0f};
    zGeometry_WeilerContourSegmentPartial pairAFirst{};
    zGeometry_WeilerContourSegmentPartial pairASecond{};
    zGeometry_WeilerContourSegmentPartial pairBFirst{};
    zGeometry_WeilerContourSegmentPartial pairBSecond{};
    pairAFirst.startPoint = &pairStartA;
    pairAFirst.endPoint = &pairCornerA;
    pairASecond.startPoint = &pairCornerA;
    pairASecond.endPoint = &pairEndA;
    pairBFirst.startPoint = &pairStartA;
    pairBFirst.endPoint = &pairCornerA;
    pairBSecond.startPoint = &pairCornerA;
    pairBSecond.endPoint = &pairEndA;
    if (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
            &pairAFirst, &pairASecond, &pairBFirst, &pairBSecond, &intersectState) != 5) {
        return 56;
    }
    pairBSecond.startPoint = &pairStartA;
    if (zGeometry_Weiler::ClassifyAdjacentEdgePairAgainstAdjacentEdgePair(
            &pairAFirst, &pairASecond, &pairBFirst, &pairBSecond, &intersectState) != 0) {
        return 57;
    }

    zGeometry_WeilerContourSegmentPartial validateSegments[8]{};
    zGeometry_WeilerXingPartial validateXings[2]{};
    validateXings[0].xingType = 4;
    validateXings[0].segment0 = &validateSegments[0];
    validateXings[0].segment1 = &validateSegments[1];
    validateXings[0].segment2 = &validateSegments[2];
    validateXings[0].segment3 = &validateSegments[3];
    validateXings[0].segment4 = &validateSegments[4];
    validateXings[0].segment5 = &validateSegments[5];
    validateXings[0].segment6 = &validateSegments[6];
    validateXings[0].segment7 = &validateSegments[7];
    validateXings[1].xingType = 4;
    validateXings[1].segment0 = &validateSegments[0];
    validateXings[1].segment1 = &validateSegments[1];
    validateXings[1].segment2 = &validateSegments[2];
    validateXings[1].segment3 = &validateSegments[3];
    validateXings[1].segment4 = &validateSegments[4];
    validateXings[1].segment5 = &validateSegments[5];
    validateXings[1].segment6 = &validateSegments[6];
    int failedXingIndex = -1;
    if (zGeometry_Weiler::ValidateXings(1, validateXings, &failedXingIndex) != 1 ||
        failedXingIndex != -1 ||
        zGeometry_Weiler::ValidateXings(2, validateXings, &failedXingIndex) != 0 ||
        failedXingIndex != 1) {
        return 58;
    }

    zGeometry_WeilerStatePartial mergeState{};
    zGeometry_WeilerBuffer::Init(&mergeState.xingBuffer, 1, sizeof(zGeometry_WeilerXingPartial));
    zGeometry_WeilerBuffer::Init(&mergeState.contourBuffer, 4,
                                 sizeof(zGeometry_WeilerContourOutputPartial));
    zGeometry_WeilerBuffer::SetCountAndAppendPtr(&mergeState.xingBuffer, 1);
    zGeometry_WeilerXingPartial *mergeXing =
        static_cast<zGeometry_WeilerXingPartial *>(mergeState.xingBuffer.base);
    zGeometry_WeilerContourSegmentPartial mergeSegments[8]{};
    for (int i = 0; i < 8; ++i) {
        mergeSegments[i].contourType = i + 1;
    }
    mergeXing->xingType = 4;
    mergeXing->segment0 = &mergeSegments[0];
    mergeXing->segment1 = &mergeSegments[1];
    mergeXing->segment2 = &mergeSegments[2];
    mergeXing->segment3 = &mergeSegments[3];
    mergeXing->segment4 = &mergeSegments[4];
    mergeXing->segment5 = &mergeSegments[5];
    mergeXing->segment6 = &mergeSegments[6];
    mergeXing->segment7 = &mergeSegments[7];
    if (zGeometry_Weiler::MergeContours(&mergeState) != 1 ||
        mergeState.contourBuffer.count != 2 || mergeSegments[0].next != &mergeSegments[5] ||
        mergeSegments[1].next != &mergeSegments[7] ||
        mergeSegments[2].prev != &mergeSegments[4] ||
        mergeSegments[3].prev != &mergeSegments[6] ||
        mergeSegments[4].next != &mergeSegments[2] ||
        mergeSegments[5].next != &mergeSegments[0] ||
        mergeSegments[6].prev != &mergeSegments[3] ||
        mergeSegments[7].prev != &mergeSegments[1]) {
        zGeometry_WeilerBuffer::Destroy(&mergeState.xingBuffer);
        zGeometry_WeilerBuffer::Destroy(&mergeState.contourBuffer);
        return 59;
    }
    zGeometry_WeilerContourOutputPartial *mergeContours =
        static_cast<zGeometry_WeilerContourOutputPartial *>(mergeState.contourBuffer.base);
    if (mergeSegments[4].contourOutput != &mergeContours[0] ||
        mergeSegments[6].contourOutput != &mergeContours[1] ||
        mergeContours[0].firstSegment != &mergeSegments[4] ||
        mergeContours[1].firstSegment != &mergeSegments[6] ||
        mergeContours[0].contourType != 5 || mergeContours[1].contourType != 7) {
        zGeometry_WeilerBuffer::Destroy(&mergeState.xingBuffer);
        zGeometry_WeilerBuffer::Destroy(&mergeState.contourBuffer);
        return 60;
    }
    zGeometry_WeilerBuffer::Destroy(&mergeState.xingBuffer);
    zGeometry_WeilerBuffer::Destroy(&mergeState.contourBuffer);

    zVec3 contourA[2] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
    };
    zGeometry_WeilerStatePartial axisState{};
    axisState.inputContourABuffer.base = contourA;
    axisState.inputContourABuffer.count = 2;
    axisState.contourSource = 2;
    zGeometry_Weiler::TogglePointAxesForContourSource(&axisState);
    if (contourA[0].x != 1.0f || contourA[0].y != 3.0f || contourA[0].z != 2.0f ||
        contourA[1].x != 4.0f || contourA[1].y != 6.0f || contourA[1].z != 5.0f) {
        return 32;
    }

    zVec3 contourB[1] = {
        {7.0f, 8.0f, 9.0f},
    };
    axisState.inputContourBBuffer.base = contourB;
    axisState.inputContourBBuffer.count = 1;
    axisState.contourSource = 1;
    zGeometry_Weiler::TogglePointAxesForContourSource(&axisState);
    if (contourB[0].x != 9.0f || contourB[0].y != 8.0f || contourB[0].z != 7.0f ||
        contourA[0].x != 1.0f) {
        return 33;
    }

    zGeometry_WeilerStatePartial recenterState{};
    zVec3 centered[1] = {
        {100.0f, -100.0f, 3.0f},
    };
    recenterState.inputContourABuffer.base = centered;
    recenterState.inputContourABuffer.count = 1;
    recenterState.pointsRecentered = true;
    zGeometry_Weiler::RecenterPointSetsIfOutOfRange(&recenterState);
    if (recenterState.pointsRecentered || centered[0].x != 100.0f || centered[0].y != -100.0f) {
        return 34;
    }

    zVec3 outOfRange[2] = {
        {70000.0f, -80000.0f, 1.0f},
        {70003.0f, -79996.0f, 2.0f},
    };
    recenterState = {};
    recenterState.inputContourABuffer.base = outOfRange;
    recenterState.inputContourABuffer.count = 2;
    zGeometry_Weiler::RecenterPointSetsIfOutOfRange(&recenterState);
    if (!recenterState.pointsRecentered || recenterState.pointTranslationX != 70000.0f ||
        recenterState.pointTranslationY != -80000.0f || outOfRange[0].x != 0.0f ||
        outOfRange[0].y != 0.0f || outOfRange[1].x != 3.0f || outOfRange[1].y != 4.0f) {
        return 35;
    }

    zVec3 contourBRecentering[1] = {
        {70005.0f, -79995.0f, 9.0f},
    };
    recenterState.inputContourBBuffer.base = contourBRecentering;
    recenterState.inputContourBBuffer.count = 1;
    zGeometry_Weiler::RecenterPointSetsIfOutOfRange(&recenterState);
    if (contourBRecentering[0].x != 5.0f || contourBRecentering[0].y != 5.0f ||
        contourBRecentering[0].z != 9.0f) {
        return 36;
    }

    zVec3 traversalPoints[4] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {2.0f, 0.0f, 0.0f},
        {3.0f, 0.0f, 0.0f},
    };
    zGeometry_WeilerContourSegmentPartial traversalSegments[4]{};
    traversalSegments[0].next = &traversalSegments[1];
    traversalSegments[1].next = &traversalSegments[0];
    traversalSegments[1].prev = &traversalSegments[2];
    traversalSegments[1].startPoint = &traversalPoints[0];
    traversalSegments[1].endPoint = &traversalPoints[1];
    if (zGeometry_Weiler::GetNextContourSegmentForTraversal(&traversalSegments[0]) !=
            &traversalSegments[1] ||
        traversalSegments[1].prev != &traversalSegments[0] ||
        traversalSegments[1].next != &traversalSegments[2] ||
        traversalSegments[1].startPoint != &traversalPoints[1] ||
        traversalSegments[1].endPoint != &traversalPoints[0]) {
        return 120;
    }

    traversalSegments[2].next = &traversalSegments[3];
    traversalSegments[2].prev = &traversalSegments[0];
    traversalSegments[2].startPoint = &traversalPoints[2];
    traversalSegments[2].endPoint = &traversalPoints[3];
    traversalSegments[0].next = &traversalSegments[2];
    if (zGeometry_Weiler::GetNextContourSegmentForTraversal(&traversalSegments[0]) !=
            &traversalSegments[2] ||
        traversalSegments[2].prev != &traversalSegments[0] ||
        traversalSegments[2].next != &traversalSegments[3] ||
        traversalSegments[2].startPoint != &traversalPoints[2] ||
        traversalSegments[2].endPoint != &traversalPoints[3]) {
        return 121;
    }

    zGeometry_WeilerStatePartial newContourEmptyState{};
    zGeometry_Weiler::NewContour(&newContourEmptyState);
    if (!newContourEmptyState.allContoursSingleSided) {
        return 122;
    }

    zGeometry_WeilerStatePartial newContourState{};
    zGeometry_WeilerContourOutputPartial newContourOutputs[2]{};
    zGeometry_WeilerContourOutputPartial staleOwner{};
    zGeometry_WeilerContourSegmentPartial newContourSegments[6]{};
    newContourState.contourBuffer.base = newContourOutputs;
    newContourState.contourBuffer.count = 2;

    newContourOutputs[0].firstSegment = &newContourSegments[0];
    newContourSegments[0].prev = &newContourSegments[2];
    newContourSegments[0].next = &newContourSegments[1];
    newContourSegments[0].contourType = 1;
    newContourSegments[1].prev = &newContourSegments[0];
    newContourSegments[1].next = &newContourSegments[2];
    newContourSegments[1].contourType = 2;
    newContourSegments[1].contourOutput = &staleOwner;
    staleOwner.firstSegment = &newContourSegments[1];
    newContourSegments[2].prev = &newContourSegments[1];
    newContourSegments[2].next = &newContourSegments[0];
    newContourSegments[2].contourType = 4;

    newContourOutputs[1].firstSegment = &newContourSegments[3];
    newContourSegments[3].prev = &newContourSegments[4];
    newContourSegments[3].next = &newContourSegments[5];
    newContourSegments[3].contourType = 6;
    newContourSegments[4].prev = &newContourSegments[3];
    newContourSegments[4].next = &newContourSegments[5];
    newContourSegments[4].contourType = 4;
    newContourSegments[5].prev = &newContourSegments[4];
    newContourSegments[5].next = &newContourSegments[3];
    newContourSegments[5].contourType = 4;

    zGeometry_Weiler::NewContour(&newContourState);
    if (newContourOutputs[0].contourType != 7 || newContourOutputs[0].pointCount != 3 ||
        staleOwner.firstSegment != nullptr ||
        newContourSegments[1].contourOutput != nullptr ||
        newContourOutputs[1].contourType != 6 || newContourOutputs[1].pointCount != 3 ||
        newContourSegments[3].prev != &newContourSegments[5] ||
        newContourSegments[3].next != &newContourSegments[4] ||
        newContourState.allContoursSingleSided) {
        return 123;
    }

    zGeometry_WeilerStatePartial selectState{};
    zVec3 selectContourA[3] = {
        {10.0f, 0.0f, 0.0f},
        {4.0f, 2.0f, 0.0f},
        {0.0f, 0.0f, 0.0f},
    };
    zVec3 selectPoint = {5.0f, 1.0f, 0.0f};
    zVec3 selectCandidate = {5.0f, -1.0f, 0.0f};
    zVec3 *selectedPoint = &selectCandidate;
    zGeometry_Weiler::SelectForwardStartPointInContourA(&selectPoint, &selectedPoint,
                                                        &selectState);
    if (selectedPoint != &selectCandidate) {
        return 124;
    }

    selectState.inputContourABuffer.base = selectContourA;
    selectState.inputContourABuffer.count = 3;
    zGeometry_Weiler::SelectForwardStartPointInContourA(&selectPoint, &selectedPoint,
                                                        &selectState);
    if (selectedPoint != &selectContourA[0]) {
        return 125;
    }

    selectedPoint = &selectCandidate;
    selectPoint.x = 11.0f;
    zGeometry_Weiler::SelectForwardStartPointInContourA(&selectPoint, &selectedPoint,
                                                        &selectState);
    if (selectedPoint != &selectContourA[2]) {
        return 126;
    }

    zGeometry_WeilerStatePartial outsideDisabledState{};
    if (zGeometry_Weiler::GenerateOutsideResults(&outsideDisabledState) != 1) {
        return 127;
    }

    zGeometry_WeilerStatePartial outsideState{};
    zGeometry_WeilerClipOutputPartial outsideOutput{};
    zVec3 outsideContourA[1] = {
        {1.0f, 2.0f, 3.0f},
    };
    zVec3 outsideContourB[2] = {
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    outsideState.clipMode = 2;
    outsideState.outClip = &outsideOutput;
    outsideState.inputContourABuffer.base = outsideContourA;
    outsideState.inputContourABuffer.count = 1;
    outsideState.inputContourBBuffer.base = outsideContourB;
    outsideState.inputContourBBuffer.count = 2;
    outsideOutput.pointList.pointCount = 2;
    outsideOutput.polygonSetB.polygonCount = 3;
    zGeometry_WeilerBuffer::Init(&outsideState.polygonSetBBuffer, 1,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&outsideState.pointListBuffer, 1, sizeof(zVec3));
    if (zGeometry_Weiler::GenerateOutsideResults(&outsideState) != 1 ||
        outsideOutput.polygonSetB.polygonCount != 4 ||
        outsideOutput.pointList.pointCount != 7 ||
        outsideState.polygonSetBBuffer.count != 1 ||
        outsideState.pointListBuffer.count != 5) {
        zGeometry_WeilerBuffer::Destroy(&outsideState.polygonSetBBuffer);
        zGeometry_WeilerBuffer::Destroy(&outsideState.pointListBuffer);
        return 128;
    }
    zGeometry_PolygonPointSpanPartial *outsidePolygons =
        static_cast<zGeometry_PolygonPointSpanPartial *>(outsideState.polygonSetBBuffer.base);
    zVec3 *outsidePoints = static_cast<zVec3 *>(outsideState.pointListBuffer.base);
    if (outsideOutput.polygonSetB.polygons != outsidePolygons ||
        outsideOutput.pointList.points != outsidePoints ||
        outsidePolygons[0].pointDwordOffset != 6 ||
        outsidePolygons[0].pointCount != 5 ||
        outsidePoints[0].x != 7.0f || outsidePoints[1].x != 4.0f ||
        outsidePoints[2].x != 7.0f || outsidePoints[3].x != 1.0f ||
        outsidePoints[4].x != 1.0f || outsidePoints[0].z != 9.0f ||
        outsidePoints[4].z != 3.0f) {
        zGeometry_WeilerBuffer::Destroy(&outsideState.polygonSetBBuffer);
        zGeometry_WeilerBuffer::Destroy(&outsideState.pointListBuffer);
        return 129;
    }
    zGeometry_WeilerBuffer::Destroy(&outsideState.polygonSetBBuffer);
    zGeometry_WeilerBuffer::Destroy(&outsideState.pointListBuffer);

    zGeometry_WeilerStatePartial restoreZState{};
    zGeometry_WeilerClipOutputPartial restoreZOutput{};
    zVec3 restoreZPlane[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 1.0f},
    };
    zVec3 restoreZPoints[2] = {
        {2.0f, 3.0f, 99.0f},
        {-1.0f, 4.0f, 99.0f},
    };
    restoreZState.inputContourBBuffer.base = restoreZPlane;
    restoreZState.outClip = &restoreZOutput;
    restoreZOutput.pointList.points = restoreZPoints;
    restoreZOutput.pointList.pointCount = 2;
    zGeometry_Weiler::RestoreOutputZFromInputPlane(&restoreZState);
    if (restoreZPoints[0].z != 5.0f || restoreZPoints[1].z != 3.0f) {
        return 130;
    }

    zVec3 restoreZDegeneratePlane[3] = {
        {0.0f, 0.0f, 1.0f},
        {1.0f, 0.0f, 1.0f},
        {2.0f, 0.0f, 1.0f},
    };
    restoreZPoints[0].z = 77.0f;
    restoreZState.inputContourBBuffer.base = restoreZDegeneratePlane;
    zGeometry_Weiler::RestoreOutputZFromInputPlane(&restoreZState);
    if (restoreZPoints[0].z != 77.0f) {
        return 131;
    }

    zGeometry_WeilerStatePartial outputContourState{};
    zGeometry_WeilerClipOutputPartial outputContourClip{};
    zGeometry_PolygonSpanArrayPartial outputContourPolygonSet{};
    zGeometry_WeilerBufferPartial outputContourPolygonBuffer{};
    zGeometry_WeilerContourOutputPartial outputContour{};
    zVec3 outputContourPoints[3] = {
        {1.0f, 2.0f, 3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    zGeometry_WeilerContourSegmentPartial outputContourSegments[3]{};
    outputContourSegments[0].prev = &outputContourSegments[2];
    outputContourSegments[0].next = &outputContourSegments[1];
    outputContourSegments[0].startPoint = &outputContourPoints[0];
    outputContourSegments[0].endPoint = &outputContourPoints[1];
    outputContourSegments[1].prev = &outputContourSegments[0];
    outputContourSegments[1].next = &outputContourSegments[2];
    outputContourSegments[1].startPoint = &outputContourPoints[1];
    outputContourSegments[1].endPoint = &outputContourPoints[2];
    outputContourSegments[2].prev = &outputContourSegments[1];
    outputContourSegments[2].next = &outputContourSegments[0];
    outputContourSegments[2].startPoint = &outputContourPoints[2];
    outputContourSegments[2].endPoint = &outputContourPoints[0];
    outputContour.firstSegment = &outputContourSegments[0];
    outputContour.pointCount = 3;
    outputContourClip.pointList.pointCount = 4;
    outputContourState.outClip = &outputContourClip;
    zGeometry_WeilerBuffer::Init(&outputContourPolygonBuffer, 1,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&outputContourState.pointListBuffer, 1, sizeof(zVec3));
    if (zGeometry_Weiler::OutputContourToPolygonSet(&outputContourState, &outputContour,
                                                    &outputContourPolygonBuffer,
                                                    &outputContourPolygonSet) != 1 ||
        outputContourPolygonSet.polygonCount != 1 ||
        outputContourClip.pointList.pointCount != 7 ||
        outputContourPolygonBuffer.count != 1 ||
        outputContourState.pointListBuffer.count != 3) {
        zGeometry_WeilerBuffer::Destroy(&outputContourPolygonBuffer);
        zGeometry_WeilerBuffer::Destroy(&outputContourState.pointListBuffer);
        return 132;
    }
    zGeometry_PolygonPointSpanPartial *outputContourPolygons =
        static_cast<zGeometry_PolygonPointSpanPartial *>(outputContourPolygonBuffer.base);
    zVec3 *outputContourWrittenPoints =
        static_cast<zVec3 *>(outputContourState.pointListBuffer.base);
    if (outputContourPolygonSet.polygons != outputContourPolygons ||
        outputContourClip.pointList.points != outputContourWrittenPoints ||
        outputContourPolygons[0].pointDwordOffset != 12 ||
        outputContourPolygons[0].pointCount != 3 ||
        outputContourWrittenPoints[0].x != 1.0f ||
        outputContourWrittenPoints[1].x != 4.0f ||
        outputContourWrittenPoints[2].x != 7.0f ||
        outputContourWrittenPoints[2].z != 9.0f) {
        zGeometry_WeilerBuffer::Destroy(&outputContourPolygonBuffer);
        zGeometry_WeilerBuffer::Destroy(&outputContourState.pointListBuffer);
        return 133;
    }
    zGeometry_WeilerBuffer::Destroy(&outputContourPolygonBuffer);
    zGeometry_WeilerBuffer::Destroy(&outputContourState.pointListBuffer);

    zGeometry_WeilerStatePartial outputModeState{};
    zGeometry_WeilerClipOutputPartial outputModeClip{};
    zGeometry_WeilerContourOutputPartial outputModeContours[4]{};
    zVec3 outputModePoints[3] = {
        {10.0f, 20.0f, 30.0f},
        {40.0f, 50.0f, 60.0f},
        {70.0f, 80.0f, 90.0f},
    };
    zGeometry_WeilerContourSegmentPartial outputModeSegments[3]{};
    outputModeSegments[0].prev = &outputModeSegments[2];
    outputModeSegments[0].next = &outputModeSegments[1];
    outputModeSegments[0].startPoint = &outputModePoints[0];
    outputModeSegments[0].endPoint = &outputModePoints[1];
    outputModeSegments[1].prev = &outputModeSegments[0];
    outputModeSegments[1].next = &outputModeSegments[2];
    outputModeSegments[1].startPoint = &outputModePoints[1];
    outputModeSegments[1].endPoint = &outputModePoints[2];
    outputModeSegments[2].prev = &outputModeSegments[1];
    outputModeSegments[2].next = &outputModeSegments[0];
    outputModeSegments[2].startPoint = &outputModePoints[2];
    outputModeSegments[2].endPoint = &outputModePoints[0];
    outputModeContours[0].contourType = 3;
    outputModeContours[0].firstSegment = &outputModeSegments[0];
    outputModeContours[0].pointCount = 3;
    outputModeContours[1].contourType = 2;
    outputModeContours[1].firstSegment = &outputModeSegments[0];
    outputModeContours[1].pointCount = 3;
    outputModeContours[2].contourType = 5;
    outputModeContours[2].firstSegment = &outputModeSegments[0];
    outputModeContours[2].pointCount = 3;
    outputModeContours[3].contourType = 3;
    outputModeState.clipMode = 7;
    outputModeState.outClip = &outputModeClip;
    outputModeState.contourBuffer.base = outputModeContours;
    outputModeState.contourBuffer.count = 4;
    zGeometry_WeilerBuffer::Init(&outputModeState.polygonSetABuffer, 1,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&outputModeState.polygonSetBBuffer, 1,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&outputModeState.polygonSetCBuffer, 1,
                                 sizeof(zGeometry_PolygonPointSpanPartial));
    zGeometry_WeilerBuffer::Init(&outputModeState.pointListBuffer, 1, sizeof(zVec3));
    if (zGeometry_Weiler::OutputContoursForClipMode(&outputModeState) != 1 ||
        outputModeClip.polygonSetA.polygonCount != 1 ||
        outputModeClip.polygonSetB.polygonCount != 1 ||
        outputModeClip.polygonSetC.polygonCount != 1 ||
        outputModeClip.pointList.pointCount != 9 ||
        outputModeState.polygonSetABuffer.count != 1 ||
        outputModeState.polygonSetBBuffer.count != 1 ||
        outputModeState.polygonSetCBuffer.count != 1 ||
        outputModeState.pointListBuffer.count != 9) {
        zGeometry_WeilerBuffer::Destroy(&outputModeState.polygonSetABuffer);
        zGeometry_WeilerBuffer::Destroy(&outputModeState.polygonSetBBuffer);
        zGeometry_WeilerBuffer::Destroy(&outputModeState.polygonSetCBuffer);
        zGeometry_WeilerBuffer::Destroy(&outputModeState.pointListBuffer);
        return 134;
    }
    zGeometry_PolygonPointSpanPartial *outputModePolygonsA =
        static_cast<zGeometry_PolygonPointSpanPartial *>(outputModeState.polygonSetABuffer.base);
    zGeometry_PolygonPointSpanPartial *outputModePolygonsB =
        static_cast<zGeometry_PolygonPointSpanPartial *>(outputModeState.polygonSetBBuffer.base);
    zGeometry_PolygonPointSpanPartial *outputModePolygonsC =
        static_cast<zGeometry_PolygonPointSpanPartial *>(outputModeState.polygonSetCBuffer.base);
    if (outputModePolygonsA[0].pointDwordOffset != 0 ||
        outputModePolygonsB[0].pointDwordOffset != 9 ||
        outputModePolygonsC[0].pointDwordOffset != 18 ||
        outputModePolygonsA[0].pointCount != 3 ||
        outputModePolygonsB[0].pointCount != 3 ||
        outputModePolygonsC[0].pointCount != 3) {
        zGeometry_WeilerBuffer::Destroy(&outputModeState.polygonSetABuffer);
        zGeometry_WeilerBuffer::Destroy(&outputModeState.polygonSetBBuffer);
        zGeometry_WeilerBuffer::Destroy(&outputModeState.polygonSetCBuffer);
        zGeometry_WeilerBuffer::Destroy(&outputModeState.pointListBuffer);
        return 135;
    }
    zGeometry_WeilerBuffer::Destroy(&outputModeState.polygonSetABuffer);
    zGeometry_WeilerBuffer::Destroy(&outputModeState.polygonSetBBuffer);
    zGeometry_WeilerBuffer::Destroy(&outputModeState.polygonSetCBuffer);
    zGeometry_WeilerBuffer::Destroy(&outputModeState.pointListBuffer);

    zGeometry_WeilerStatePartial *state = static_cast<zGeometry_WeilerStatePartial *>(
        std::calloc(1, sizeof(zGeometry_WeilerStatePartial)));
    if (state == nullptr) {
        return 37;
    }

    zGeometry_WeilerBuffer::Init(&state->segmentBuffer, 1,
                                 sizeof(zGeometry_WeilerContourSegmentPartial));
    zGeometry_WeilerBuffer::Init(&state->contourBuffer, 1,
                                 sizeof(zGeometry_WeilerContourOutputPartial));
    zGeometry_WeilerBuffer::Init(&state->xingBuffer, 1, sizeof(zGeometry_WeilerXingPartial));
    zGeometry_WeilerBuffer::Init(&state->inputContourABuffer, 1, sizeof(zVec3));
    zGeometry_Weiler::DestroyState(state);
    zGeometry_Weiler::DestroyState(nullptr);
    return 0;
}

extern "C" int zgeometry_clip_polygon_create_copy_finalize_smoke() {
    zVec3 input[3] = {
        {0.0f, 1.0f, 2.0f},
        {3.0f, -4.0f, 5.0f},
        {-6.0f, 7.0f, -8.0f},
    };

    zGeometry_ClipPolygonPartial *clipPolygon =
        zGeometry_ClipPolygon::CreateFromPointList(3, input);
    if (clipPolygon == nullptr || clipPolygon->pointCount != 3 ||
        clipPolygon->points == nullptr || clipPolygon->points == input ||
        clipPolygon->points[0].x != 0.0f || clipPolygon->points[0].y != -2.0f ||
        clipPolygon->points[0].z != 1.0f || clipPolygon->points[1].x != 3.0f ||
        clipPolygon->points[1].y != -5.0f || clipPolygon->points[1].z != -4.0f ||
        clipPolygon->points[2].x != -6.0f || clipPolygon->points[2].y != 8.0f ||
        clipPolygon->points[2].z != 7.0f || clipPolygon->bounds.minX != -6.0f ||
        clipPolygon->bounds.maxX != 3.0f || clipPolygon->bounds.maxY != 8.0f ||
        clipPolygon->bounds.minY != -5.0f) {
        if (clipPolygon != nullptr) {
            zGeometry_ClipPolygon::FinalizeAndDestroy(clipPolygon);
        }
        return 1;
    }

    int outPointCount = 0;
    zVec3 *outPoints = nullptr;
    if (zGeometry_ClipPolygon::CopyPointsOutRotatedBack(clipPolygon, &outPointCount,
                                                        &outPoints) != 0 ||
        outPointCount != 3 || outPoints == nullptr || outPoints[0].x != input[0].x ||
        outPoints[0].y != input[0].y || outPoints[0].z != input[0].z ||
        outPoints[1].x != input[1].x || outPoints[1].y != input[1].y ||
        outPoints[1].z != input[1].z || outPoints[2].x != input[2].x ||
        outPoints[2].y != input[2].y || outPoints[2].z != input[2].z) {
        std::free(outPoints);
        zGeometry_ClipPolygon::FinalizeAndDestroy(clipPolygon);
        return 2;
    }

    std::free(outPoints);
    zGeometry_ClipPolygon::FinalizeAndDestroy(clipPolygon);

    clipPolygon = static_cast<zGeometry_ClipPolygonPartial *>(
        std::calloc(1, sizeof(zGeometry_ClipPolygonPartial)));
    if (clipPolygon == nullptr) {
        return 3;
    }

    zGeometry_ClipPolygon::FinalizeAndDestroy(clipPolygon);
    return 0;
}

extern "C" int zgeometry_clip_polygon_upsert_point_list_xy_smoke() {
    zGeometry_ClipPolygonPartial clipPolygon{};
    clipPolygon.pointCount = 4;
    clipPolygon.points = static_cast<zVec3 *>(std::malloc(sizeof(zVec3) * 4));
    if (clipPolygon.points == nullptr) {
        return 1;
    }

    clipPolygon.points[0] = {0.0f, 0.0f, 1.0f};
    clipPolygon.points[1] = {10.0f, 0.0f, 2.0f};
    clipPolygon.points[2] = {10.0f, 10.0f, 3.0f};
    clipPolygon.points[3] = {0.0f, 10.0f, 4.0f};

    zVec3 nearExisting{10.005f, 0.004f, 20.0f};
    zVec3 missingPoint{3.0f, 4.0f, 30.0f};
    if (zGeometry_ClipPolygon::FindPointIndexXY(&clipPolygon, &nearExisting) != 1 ||
        zGeometry_ClipPolygon::FindPointIndexXY(&clipPolygon, &missingPoint) != -1) {
        std::free(clipPolygon.points);
        return 2;
    }

    zVec3 topEdgeMidpoint{5.0f, 10.0f, 40.0f};
    if (zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex(&clipPolygon,
                                                             &topEdgeMidpoint) != 2 ||
        zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex(&clipPolygon,
                                                             &missingPoint) != -1) {
        std::free(clipPolygon.points);
        return 3;
    }

    zVec3 diagonalPoints[3] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 10.0f, 0.0f},
        {20.0f, 0.0f, 0.0f},
    };
    zGeometry_ClipPolygonPartial diagonalPolygon{};
    diagonalPolygon.pointCount = 3;
    diagonalPolygon.points = diagonalPoints;
    zVec3 diagonalMidpoint{5.0f, 5.0f, 0.0f};
    if (zGeometry_ClipPolygon::FindPointInsertionEdgeXYIndex(&diagonalPolygon,
                                                             &diagonalMidpoint) != 0) {
        std::free(clipPolygon.points);
        return 4;
    }

    zVec3 upsertPoints[2] = {
        nearExisting,
        topEdgeMidpoint,
    };
    if (zGeometry_ClipPolygon::UpsertPointListXY(&clipPolygon, 2, upsertPoints) != 1 ||
        clipPolygon.pointCount != 5 || clipPolygon.points[1].z != 20.0f ||
        clipPolygon.points[3].x != 5.0f || clipPolygon.points[3].y != 10.0f ||
        clipPolygon.points[3].z != 40.0f || clipPolygon.points[4].x != 0.0f ||
        clipPolygon.points[4].y != 10.0f) {
        std::free(clipPolygon.points);
        return 5;
    }

    if (zGeometry_ClipPolygon::UpsertPointListXY(&clipPolygon, 1, &missingPoint) != 0 ||
        zGeometry_ClipPolygon::UpsertPointListXY(&clipPolygon, 0, &missingPoint) != 0) {
        std::free(clipPolygon.points);
        return 6;
    }

    std::free(clipPolygon.points);
    return 0;
}

extern "C" int zgeometry_model_linear_buffer_and_bounds_overlap_smoke() {
    zVec3 verts[4] = {
        {10.0f, 20.0f, 30.0f},
        {-1.0f, -2.0f, -3.0f},
        {4.0f, 5.0f, 6.0f},
        {7.0f, 8.0f, 9.0f},
    };
    int indices[3] = {2, 0, 3};
    zModel_PolygonPartial polygon{};
    polygon.vertexCountAndFlags = 3;
    polygon.vertexIndices = indices;

    zModel_DrawBatchBasePartial model{};
    model.verts = verts;

    zVec3 *buffer = zGeometry_Model::GetLinearBufferOfPolygonVertices(&model, &polygon, nullptr);
    if (buffer == nullptr || buffer[0].x != 4.0f || buffer[0].y != 5.0f ||
        buffer[0].z != 6.0f || buffer[1].x != 10.0f || buffer[1].y != 20.0f ||
        buffer[1].z != 30.0f || buffer[2].x != 7.0f || buffer[2].y != 8.0f ||
        buffer[2].z != 9.0f) {
        std::free(buffer);
        return 1;
    }

    int resizedIndices[2] = {1, 2};
    polygon.vertexCountAndFlags = 2;
    polygon.vertexIndices = resizedIndices;
    buffer = zGeometry_Model::GetLinearBufferOfPolygonVertices(&model, &polygon, buffer);
    if (buffer == nullptr || buffer[0].x != -1.0f || buffer[0].y != -2.0f ||
        buffer[0].z != -3.0f || buffer[1].x != 4.0f || buffer[1].y != 5.0f ||
        buffer[1].z != 6.0f) {
        std::free(buffer);
        return 2;
    }
    std::free(buffer);

    zGeometry_BoundsXY boundsA{};
    boundsA.minX = 0.0f;
    boundsA.maxY = 10.0f;
    boundsA.maxX = 10.0f;
    boundsA.minY = 0.0f;

    zGeometry_BoundsXY boundsB{};
    boundsB.minX = 10.5f;
    boundsB.maxY = 5.0f;
    boundsB.maxX = 12.0f;
    boundsB.minY = 1.0f;
    if (zGeometry_Bounds2D::OverlapsWithUnitMargin(&boundsA, &boundsB) != 1) {
        return 3;
    }

    boundsB.minX = 11.5f;
    boundsB.maxX = 12.5f;
    if (zGeometry_Bounds2D::OverlapsWithUnitMargin(&boundsA, &boundsB) != 0) {
        return 4;
    }

    boundsB.minX = 1.0f;
    boundsB.maxX = 2.0f;
    boundsB.maxY = -1.5f;
    boundsB.minY = -2.5f;
    return zGeometry_Bounds2D::OverlapsWithUnitMargin(&boundsA, &boundsB) == 0 ? 0 : 5;
}

extern "C" int zgeometry_model_is_fully_inside_clip_polygon_xy_smoke() {
    zVec3 clipPoints[4] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f},
        {10.0f, 10.0f, 0.0f},
        {0.0f, 10.0f, 0.0f},
    };

    zGeometry_ClipPolygonPartial clipPolygon{};
    clipPolygon.points = clipPoints;
    clipPolygon.pointCount = 4;
    clipPolygon.bounds.minX = 0.0f;
    clipPolygon.bounds.maxX = 10.0f;
    clipPolygon.bounds.maxY = 10.0f;
    clipPolygon.bounds.minY = 0.0f;
    clipPolygon.weilerState = zGeometry_Weiler::Init(clipPoints, 4, 0);
    if (clipPolygon.weilerState == nullptr) {
        return 1;
    }

    if (zGeometry_Model::IsFullyInsideClipPolygonXY(nullptr, nullptr) != 0 ||
        zGeometry_Model::IsFullyInsideClipPolygonXY(&clipPolygon, nullptr) != 0) {
        zGeometry_Weiler::DestroyState(clipPolygon.weilerState);
        return 2;
    }

    int faceIndices[3] = {0, 1, 2};
    zModel_PolygonPartial face{};
    face.vertexCountAndFlags = 3;
    face.vertexIndices = faceIndices;

    zModel_DrawBatchBasePartial model{};
    model.faceCount = 1;
    model.faceList = &face;

    zVec3 disjointVerts[3] = {
        {20.0f, 0.0f, -20.0f},
        {30.0f, 0.0f, -20.0f},
        {20.0f, 0.0f, -30.0f},
    };
    model.verts = disjointVerts;
    if (zGeometry_Model::IsFullyInsideClipPolygonXY(&clipPolygon, &model) != 1) {
        zGeometry_Weiler::DestroyState(clipPolygon.weilerState);
        return 3;
    }

    zVec3 containedVerts[3] = {
        {2.0f, 0.0f, -2.0f},
        {8.0f, 0.0f, -2.0f},
        {2.0f, 0.0f, -8.0f},
    };
    model.verts = containedVerts;
    if (zGeometry_Model::IsFullyInsideClipPolygonXY(&clipPolygon, &model) != 0) {
        zGeometry_Weiler::DestroyState(clipPolygon.weilerState);
        return 4;
    }

    zGeometry_Weiler::DestroyState(clipPolygon.weilerState);
    return 0;
}

extern "C" int zgeometry_model_process_clip_patch_node_smoke() {
    if (zGeometry_Model::ProcessClipPatchNode(nullptr, nullptr, nullptr) != 1) {
        return 1;
    }

    zVec3 clipPoints[4] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f},
        {10.0f, 10.0f, 0.0f},
        {0.0f, 10.0f, 0.0f},
    };
    zGeometry_ClipPolygonPartial clipPolygon{};
    clipPolygon.points = clipPoints;
    clipPolygon.pointCount = 4;
    clipPolygon.bounds.minX = 0.0f;
    clipPolygon.bounds.maxX = 10.0f;
    clipPolygon.bounds.maxY = 10.0f;
    clipPolygon.bounds.minY = 0.0f;
    clipPolygon.weilerState = zGeometry_Weiler::Init(clipPoints, 4, 0);
    if (clipPolygon.weilerState == nullptr) {
        return 2;
    }

    zVec3 verts[3] = {
        {20.0f, 0.0f, -20.0f},
        {30.0f, 0.0f, -20.0f},
        {20.0f, 1.0f, -30.0f},
    };
    int indices[3] = {0, 1, 2};
    zModel_MaterialPartial material{};
    zModel_PolygonPartial face{};
    face.vertexCountAndFlags = 3;
    face.vertexIndices = indices;
    face.material = &material;

    zModel_DrawBatchBasePartial model{};
    model.faceCount = 1;
    model.faceList = &face;
    model.verts = verts;

    zDiPartial *outDi = reinterpret_cast<zDiPartial *>(0x1);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    if (zGeometry_Model::ProcessClipPatchNode(&clipPolygon, &model, &outDi) != 0 ||
        outDi != reinterpret_cast<zDiPartial *>(0x1)) {
        zGeometry_Weiler::DestroyState(clipPolygon.weilerState);
        return 3;
    }

    zDiPartial diPool[1] = {};
    diPool[0].nextFreeIndex = -1;
    g_zModel_DiPoolBase = diPool;
    g_zModel_DiPoolCapacity = 1;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = 0;
    g_zModel_MaxPolygonVertexCountBeforeSplit = 64;
    g_zModel_CoplanarTolerance = 0.01;

    outDi = reinterpret_cast<zDiPartial *>(0x1);
    if (zGeometry_Model::ProcessClipPatchNode(&clipPolygon, &model, &outDi) != 1 ||
        outDi != nullptr || g_zModel_DiPoolInUseCount != 0 ||
        g_zModel_DiPoolFreeHeadIndex != 0 || diPool[0].entryCount != 0 ||
        diPool[0].entries != nullptr || diPool[0].verts != nullptr) {
        zDi::FreeContents(&diPool[0]);
        zGeometry_Weiler::DestroyState(clipPolygon.weilerState);
        g_zModel_DiPoolBase = nullptr;
        g_zModel_DiPoolCapacity = 0;
        g_zModel_DiPoolInUseCount = 0;
        g_zModel_DiPoolFreeHeadIndex = -1;
        return 4;
    }

    zGeometry_Weiler::DestroyState(clipPolygon.weilerState);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    return 0;
}

extern "C" int zgeometry_clip_polygon_process_node_polygon_set_xy_smoke() {
    zDiPartial *outDi = reinterpret_cast<zDiPartial *>(0x1);
    if (zGeometry_ClipPolygon::ProcessNodePolygonSetXY(nullptr, nullptr, &outDi) != 1 ||
        outDi != reinterpret_cast<zDiPartial *>(0x1)) {
        return 1;
    }

    zGeometry_ClipPolygonPartial clipPolygon{};
    zClass_NodePartial emptyNode{};
    if (zGeometry_ClipPolygon::ProcessNodePolygonSetXY(&clipPolygon, &emptyNode, &outDi) != 1) {
        return 2;
    }

    zVec3 clipPoints[4] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f},
        {10.0f, 10.0f, 0.0f},
        {0.0f, 10.0f, 0.0f},
    };
    clipPolygon.points = clipPoints;
    clipPolygon.pointCount = 4;
    clipPolygon.bounds.minX = 0.0f;
    clipPolygon.bounds.maxX = 10.0f;
    clipPolygon.bounds.maxY = 10.0f;
    clipPolygon.bounds.minY = 0.0f;
    clipPolygon.weilerState = zGeometry_Weiler::Init(clipPoints, 4, 0);
    if (clipPolygon.weilerState == nullptr) {
        return 3;
    }

    zVec3 verts[3] = {
        {20.0f, 0.0f, -20.0f},
        {30.0f, 0.0f, -20.0f},
        {20.0f, 1.0f, -30.0f},
    };
    int indices[3] = {0, 1, 2};
    zModel_PolygonPartial face{};
    face.vertexCountAndFlags = 3;
    face.vertexIndices = indices;

    zModel_DrawBatchBasePartial model{};
    model.faceCount = 1;
    model.faceList = &face;
    model.verts = verts;

    zClass_NodePartial node{};
    node.flags = 0x20000;
    node.userDataOrDiRef = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&model));

    outDi = reinterpret_cast<zDiPartial *>(0x1);
    if (zGeometry_ClipPolygon::ProcessNodePolygonSetXY(&clipPolygon, &node, &outDi) != 1 ||
        outDi != nullptr) {
        zGeometry_Weiler::DestroyState(clipPolygon.weilerState);
        return 4;
    }

    zGeometry_Weiler::DestroyState(clipPolygon.weilerState);
    return 0;
}

extern "C" int zgeometry_model_clip_patch_smoke() {
    zGeometry_ClipPatchOutputPartial output{};
    if (zGeometry_Model::ClipPatch(0, nullptr, nullptr, &output) != -1) {
        return 1;
    }

    zClass_NodePartial cameraNode{};
    cameraNode.listCountB = 0;
    cameraNode.listB = nullptr;
    g_zDEClient_CameraNode = &cameraNode;

    zDEClient_FeatureGridCell cell{};
    cell.nodeCount = 0;
    cell.nodes = nullptr;

    zVec3 outline[4] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, -10.0f},
        {0.0f, 0.0f, -10.0f},
    };
    if (zGeometry_Model::ClipPatch(4, outline, &cell, &output) != 0 ||
        output.partitionCount != 0 || output.partitions != nullptr ||
        output.pointCount != 0 || output.points != nullptr) {
        zGeometry_ClipPatchOutput::Destroy(&output);
        g_zDEClient_CameraNode = nullptr;
        return 2;
    }

    g_zDEClient_CameraNode = nullptr;
    return 0;
}

extern "C" int zdeclient_qsand_build_smoke() {
    zDEClient_QSandFeature feature{};
    zGeometry_ClipPatchOutputPartial output{};
    feature.clipPatchOutput = &output;
    if (zDEClient_QSand::Build(&feature) != 0) {
        return 1;
    }

    zClass_NodePartial cameraNode{};
    g_zDEClient_CameraNode = &cameraNode;

    zDEClient_FeatureGridCell cell{};
    feature.featureGridCell = &cell;
    feature.clipPatchOutput = &output;

    zVec3 outline[4] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, -10.0f},
        {0.0f, 0.0f, -10.0f},
    };
    feature.points = outline;
    feature.eventTemplate.pointCount = 4;
    if (zDEClient_QSand::Build(&feature) != 0 || feature.points != outline ||
        feature.eventTemplate.pointCount != 4 || output.partitionCount != 0 ||
        output.partitions != nullptr || output.points != nullptr) {
        zGeometry_ClipPatchOutput::Destroy(&output);
        g_zDEClient_CameraNode = nullptr;
        return 2;
    }

    g_zDEClient_CameraNode = nullptr;
    return 0;
}

extern "C" int zgeometry_polygon_convexify_and_triangulate_smoke() {
    zVec3 square[4] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 1.0f},
        {10.0f, 10.0f, 2.0f},
        {0.0f, 10.0f, 3.0f},
    };
    int squareOffsets[12] = {};
    for (int i = 0; i < 12; ++i) {
        squareOffsets[i] = i;
    }

    const int splitOffsetCapacity = 18;
    void *splitStorage = std::malloc(sizeof(zGeometry_PolygonSplitDwordOffsetListPair) +
                                     sizeof(int) * (splitOffsetCapacity - 1));
    if (splitStorage == nullptr) {
        return 1;
    }
    zGeometry_PolygonSplitDwordOffsetListPair *split =
        static_cast<zGeometry_PolygonSplitDwordOffsetListPair *>(splitStorage);
    if (zGeometry_Polygon::TrySplitPointDwordOffsetsAtBestDiagonal(
            4, reinterpret_cast<float *>(square), squareOffsets, split, 3) != 1 ||
        split->pointCount0 != 3 || split->pointCount1 != 3 ||
        split->pointDwordOffsets[0] != 9 || split->pointDwordOffsets[3] != 0 ||
        split->pointDwordOffsets[6] != 3 || split->pointDwordOffsets[9] != 3 ||
        split->pointDwordOffsets[12] != 6 || split->pointDwordOffsets[15] != 9) {
        std::free(splitStorage);
        return 2;
    }
    std::free(splitStorage);

    zGeometry_TriangleDwordOffsetList *triangles =
        zGeometry_Polygon::TriangulatePointDwordOffsetsRecursive(
            4, reinterpret_cast<float *>(square), nullptr, 0);
    if (triangles == nullptr || triangles->triangleCount != 2 ||
        triangles->triangleDwordOffsets[0] != 9 ||
        triangles->triangleDwordOffsets[3] != 0 ||
        triangles->triangleDwordOffsets[6] != 3 ||
        triangles->triangleDwordOffsets[9] != 3 ||
        triangles->triangleDwordOffsets[12] != 6 ||
        triangles->triangleDwordOffsets[15] != 9) {
        std::free(triangles);
        return 3;
    }
    std::free(triangles);

    zVec3 points[7] = {
        {0.0f, 0.0f, 10.0f},
        {4.0f, 0.0f, 11.0f},
        {0.0f, 4.0f, 12.0f},
        {10.0f, 0.0f, 20.0f},
        {14.0f, 0.0f, 21.0f},
        {14.0f, 4.0f, 22.0f},
        {10.0f, 4.0f, 23.0f},
    };
    zGeometry_PolygonPointSpanPartial spans[2] = {
        {0, 3},
        {9, 4},
    };
    zGeometry_PolygonSpanArrayPartial spanArray{};
    spanArray.polygonCount = 2;
    spanArray.polygons = spans;

    zGeometry_ConvexPolygonSetPartial *convex =
        zGeometry_Polygon::Convexify(&spanArray, 7, points);
    if (convex == nullptr || convex->polygonCount != 2 || convex->totalPointCount != 7 ||
        convex->polygons[0].pointDwordOffset != 0 || convex->polygons[0].pointCount != 3 ||
        convex->polygons[1].pointDwordOffset != 9 || convex->polygons[1].pointCount != 4 ||
        convex->points[0].z != 10.0f || convex->points[3].x != 10.0f ||
        convex->points[6].z != 23.0f) {
        zGeometry_ConvexPolygonSet::Destroy(convex);
        return 4;
    }
    zGeometry_ConvexPolygonSet::Destroy(convex);
    zGeometry_ConvexPolygonSet::Destroy(nullptr);

    return zGeometry_Polygon::Convexify(nullptr, 0, nullptr) == nullptr ? 0 : 5;
}

extern "C" int zgeometry_triangulate_hole_and_orientation_smoke() {
    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.0001f; };

    zVec3 pointsToReverse[4] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {2.0f, 0.0f, 0.0f},
        {3.0f, 0.0f, 0.0f},
    };
    zGeometry_Vec3Array::ReversePoints(4, pointsToReverse);
    if (pointsToReverse[0].x != 0.0f || pointsToReverse[1].x != 3.0f ||
        pointsToReverse[2].x != 2.0f || pointsToReverse[3].x != 1.0f) {
        return 1;
    }

    zVec3 clockwiseTriangle[3] = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
    };
    if (zGeometry_Vec3Array::EnsurePositiveCrossZ(3, clockwiseTriangle, 0) != 0 ||
        zGeometry_Vec3Array::EnsurePositiveCrossZ(3, clockwiseTriangle, 1) != 1 ||
        clockwiseTriangle[1].x != 1.0f || clockwiseTriangle[2].y != 1.0f) {
        return 2;
    }

    zVec3 counterClockwiseTriangle[3] = {
        {0.0f, 0.0f, 0.0f},
        {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f},
    };
    if (zGeometry_Vec3Array::EnsurePositiveCrossZ(3, counterClockwiseTriangle, 1) != 1 ||
        counterClockwiseTriangle[1].x != 1.0f || counterClockwiseTriangle[2].y != 1.0f) {
        return 3;
    }

    zVec3 segmentA0{0.0f, 0.0f, 0.0f};
    zVec3 segmentA1{10.0f, 10.0f, 0.0f};
    zVec3 segmentB0{0.0f, 10.0f, 0.0f};
    zVec3 segmentB1{10.0f, 0.0f, 0.0f};
    zVec3 segmentC0{20.0f, 0.0f, 0.0f};
    zVec3 segmentC1{30.0f, 0.0f, 0.0f};
    if (zGeometry_Segment::IntersectsSegmentXY(&segmentA0, &segmentA1, &segmentB0,
                                               &segmentB1) != 1 ||
        zGeometry_Segment::IntersectsSegmentXY(&segmentA0, &segmentA1, &segmentC0,
                                               &segmentC1) != 0) {
        return 4;
    }

    zGeometry_TriangulateHole_EdgeState edges[5] = {
        {0, 1, 1},
        {1, 2, 0},
        {2, 3, 2},
        {3, 0, 1},
        {0, 2, 1},
    };
    if (zGeometry_TriangulateHole::FindActiveEdgeState(1, 0, 5, edges) != &edges[0] ||
        zGeometry_TriangulateHole::FindActiveEdgeState(1, 2, 5, edges) != nullptr ||
        zGeometry_TriangulateHole::FindActiveEdgeState(0, 4, 5, edges) != nullptr) {
        return 5;
    }

    int edgeIndices[4] = {};
    if (zGeometry_TriangulateHole::CollectActiveEdgeIndicesForVertex(0, 5, edges,
                                                                     edgeIndices) != 3 ||
        edgeIndices[0] != 0 || edgeIndices[1] != 3 || edgeIndices[2] != 4) {
        return 6;
    }

    zVec3 planePoints[4] = {
        {0.0f, 0.0f, 2.0f},
        {10.0f, 0.0f, 2.0f},
        {10.0f, 10.0f, 2.0f},
        {0.0f, 10.0f, 2.0f},
    };
    zGeometry_PlaneEquationPartial plane{};
    zGeometry_Vec3Array::ComputeNewellPlane(4, planePoints, &plane);
    if (!nearFloat(plane.a, 0.0f) || !nearFloat(plane.b, 0.0f) || !(plane.c > 0.9f) ||
        !nearFloat(-(plane.d / plane.c), 2.0f)) {
        return 7;
    }

    zGeometry_TriangulateHole::CacheCombinedPlane(4, planePoints);
    zVec3 innerProjection[3] = {
        {2.0f, 2.0f, 10.0f},
        {4.0f, 2.0f, -3.0f},
        {3.0f, 4.0f, 7.0f},
    };
    zGeometry_TriangulateHole::ProjectInnerRingOntoCachedPlane(3, innerProjection);
    if (!nearFloat(innerProjection[0].z, 2.0f) ||
        !nearFloat(innerProjection[1].z, 2.0f) ||
        !nearFloat(innerProjection[2].z, 2.0f)) {
        return 8;
    }

    zVec3 outer[4] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f},
        {10.0f, 10.0f, 0.0f},
        {0.0f, 10.0f, 0.0f},
    };
    zVec3 inner[3] = {
        {3.0f, 3.0f, 5.0f},
        {7.0f, 3.0f, 5.0f},
        {5.0f, 7.0f, 5.0f},
    };
    zGeometry_TriangleSoup *soup =
        zGeometry::TriangulatePolygonWithHole(4, outer, 3, inner);
    if (soup == nullptr || soup->triangleCount <= 0 || soup->triangleVerts[0].z != 0.0f ||
        !nearFloat(inner[0].z, 0.0f) || !nearFloat(inner[1].z, 0.0f) ||
        !nearFloat(inner[2].z, 0.0f)) {
        std::free(soup);
        return 9;
    }
    std::free(soup);

    return 0;
}

extern "C" int zgeometry_polygon_snap_points_xy_if_near_smoke() {
    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.00001f; };

    zVec3 a{1.0f, 2.0f, 3.0f};
    zVec3 b{1.05f, 1.95f, -5.0f};
    if (zGeometry_Vec3::IsNearEqualXY(&a, &b, 0.1f) != 1 ||
        zGeometry_Vec3::IsNearEqualXY(&a, &b, 0.01f) != 0) {
        return 1;
    }

    zVec3 verticalStart{2.0f, 0.0f, 0.0f};
    zVec3 verticalEnd{2.0f, 10.0f, 0.0f};
    zVec3 verticalTest{2.02f, 4.0f, 7.0f};
    if (zGeometry_Vec3::SnapPointToSegmentXYIfNear(&verticalStart, &verticalEnd, &verticalTest,
                                                   0.05f) != 1 ||
        verticalTest.x != 2.0f || verticalTest.y != 4.0f || verticalTest.z != 7.0f) {
        return 2;
    }

    zVec3 horizontalStart{0.0f, 3.0f, 0.0f};
    zVec3 horizontalEnd{10.0f, 3.0f, 0.0f};
    zVec3 horizontalTest{7.0f, 3.03f, 5.0f};
    if (zGeometry_Vec3::SnapPointToSegmentXYIfNear(&horizontalStart, &horizontalEnd,
                                                   &horizontalTest, 0.05f) != 1 ||
        horizontalTest.x != 7.0f || horizontalTest.y != 3.0f || horizontalTest.z != 5.0f) {
        return 3;
    }

    zVec3 diagonalStart{0.0f, 0.0f, 0.0f};
    zVec3 diagonalEnd{10.0f, 10.0f, 0.0f};
    zVec3 diagonalTest{5.02f, 5.01f, 9.0f};
    if (zGeometry_Vec3::SnapPointToSegmentXYIfNear(&diagonalStart, &diagonalEnd, &diagonalTest,
                                                   0.05f) != 1 ||
        !nearFloat(diagonalTest.x, 5.02f) || !nearFloat(diagonalTest.y, 5.01f) ||
        diagonalTest.z != 9.0f) {
        return 4;
    }

    zVec3 outsideTest{12.0f, 12.0f, 0.0f};
    if (zGeometry_Vec3::SnapPointToSegmentXYIfNear(&diagonalStart, &diagonalEnd, &outsideTest,
                                                   0.1f) != 0 ||
        outsideTest.x != 12.0f || outsideTest.y != 12.0f) {
        return 5;
    }

    zVec3 polygon[3] = {
        {0.0f, 0.0f, 1.0f},
        {10.0f, 0.0f, 2.0f},
        {0.0f, 10.0f, 3.0f},
    };
    zVec3 targets[3] = {
        {0.04f, -0.03f, 40.0f},
        {4.0f, 0.02f, 50.0f},
        {20.0f, 20.0f, 60.0f},
    };

    if (zGeometry_Polygon::SnapPointsXYIfNear(polygon, 3, targets, 3, 0.1f, 0.05f) != 1 ||
        targets[0].x != 0.0f || targets[0].y != 0.0f || targets[0].z != 1.0f ||
        targets[1].x != 4.0f || targets[1].y != 0.0f || targets[1].z != 50.0f ||
        targets[2].x != 20.0f || targets[2].y != 20.0f || targets[2].z != 60.0f) {
        return 6;
    }

    return zGeometry_Polygon::SnapPointsXYIfNear(polygon, 3, targets, 0, 0.1f, 0.05f) == 0
               ? 0
               : 7;
}

extern "C" int zgeometry_clip_polygon_snap_points_near_node_model_xy_smoke() {
    zVec3 modelVerts[3] = {
        {0.0f, 0.0f, 0.0f},
        {10.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -10.0f},
    };
    int invalidIndices[2] = {0, 1};
    int validIndices[3] = {0, 1, 2};
    zModel_PolygonPartial faces[2]{};
    faces[0].vertexCountAndFlags = 2;
    faces[0].vertexIndices = invalidIndices;
    faces[1].vertexCountAndFlags = 3;
    faces[1].vertexIndices = validIndices;

    zModel_DrawBatchBasePartial batch{};
    batch.faceCount = 2;
    batch.faceList = faces;
    batch.verts = modelVerts;

    zVec3 clipPoints[3] = {
        {0.04f, -0.03f, 100.0f},
        {4.0f, 0.02f, 200.0f},
        {20.0f, 20.0f, 300.0f},
    };
    zGeometry_ClipPolygonPartial clipPolygon{};
    clipPolygon.points = clipPoints;
    clipPolygon.pointCount = 3;
    clipPolygon.bounds.minX = 0.0f;
    clipPolygon.bounds.maxX = 20.0f;
    clipPolygon.bounds.maxY = 20.0f;
    clipPolygon.bounds.minY = 0.0f;

    zClass_NodePartial node{};
    node.userDataOrDiRef = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&batch));

    if (zGeometry_ClipPolygon::SnapPointsNearNodeModelXY(nullptr, &node) != 0 ||
        zGeometry_ClipPolygon::SnapPointsNearNodeModelXY(&clipPolygon, nullptr) != 0) {
        return 1;
    }

    zClass_NodePartial emptyNode{};
    if (zGeometry_ClipPolygon::SnapPointsNearNodeModelXY(&clipPolygon, &emptyNode) != 0) {
        return 2;
    }

    struct TestBoundedNode {
        zClass_NodePartial node;
        float boundsMinX;
        std::uint32_t unknown90;
        float boundsNegMaxY;
        float boundsMaxX;
        std::uint32_t unknown9c;
        float boundsNegMinY;
    };
    TestBoundedNode boundedNode{};
    boundedNode.node.flags = 0x200;
    boundedNode.node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&batch));
    boundedNode.boundsMinX = 30.0f;
    boundedNode.boundsMaxX = 40.0f;
    boundedNode.boundsNegMaxY = -40.0f;
    boundedNode.boundsNegMinY = -30.0f;
    if (zGeometry_ClipPolygon::SnapPointsNearNodeModelXY(
            &clipPolygon, reinterpret_cast<zGeometry_ClipPatchNodeView *>(&boundedNode)) != 0) {
        return 3;
    }

    if (zGeometry_ClipPolygon::SnapPointsNearNodeModelXY(&clipPolygon, &node) != 1 ||
        clipPoints[0].x != 0.0f || clipPoints[0].y != 0.0f || clipPoints[0].z != 0.0f ||
        clipPoints[1].x != 4.0f || clipPoints[1].y != 0.0f || clipPoints[1].z != 200.0f ||
        clipPoints[2].x != 20.0f || clipPoints[2].y != 20.0f || clipPoints[2].z != 300.0f) {
        return 4;
    }

    return 0;
}

extern "C" int zmodel_material_write_gamez_smoke() {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));

    zImage_TexDirEntryPartial *frameTable[2] = {
        &g_zImage_TexDirEntries[2],
        nullptr,
    };
    zModel_MaterialCyclePartial cycle{};
    cycle.frameCount = 2;
    cycle.frameTable = frameTable;

    zModel_MaterialSlot slots[2]{};
    slots[0].material.flags = 0x0500;
    slots[0].material.currentTextureDirectoryEntry = &g_zImage_TexDirEntries[1];
    slots[0].material.cycle = &cycle;
    slots[0].prevPoolIndex = -1;
    slots[0].nextPoolIndex = 1;
    slots[1].material.flags = 0x0100;
    slots[1].material.currentTextureDirectoryEntry = &g_zImage_TexDirEntries[0];
    slots[1].prevPoolIndex = 0;
    slots[1].nextPoolIndex = -1;

    g_zModel_MatlPool = slots;
    g_zModel_MatlPoolCapacity = 2;
    g_zModel_MatlPoolInUseCount = 2;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = 0;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (zModel_MatlBuffer::WriteGameZ(file) != 2) {
        std::fclose(file);
        return 2;
    }

    const long poolOffset = 16;
    const long poolBytes = static_cast<long>(sizeof(zModel_MaterialSlot) * 2);
    const long cycleOffset = poolOffset + poolBytes;
    const long frameOffset = cycleOffset + static_cast<long>(sizeof(zModel_MaterialCyclePartial));
    const long expectedBytes = frameOffset + 8;
    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != expectedBytes) {
        std::fclose(file);
        return 3;
    }

    std::int32_t header[4] = {};
    zModel_MaterialSlot writtenSlots[2]{};
    std::rewind(file);
    if (std::fread(header, sizeof(header), 1, file) != 1 || header[0] != 2 || header[1] != 2 ||
        header[2] != -1 || header[3] != 0 ||
        std::fread(writtenSlots, sizeof(writtenSlots), 1, file) != 1) {
        std::fclose(file);
        return 4;
    }

    if (reinterpret_cast<std::intptr_t>(writtenSlots[0].material.currentTextureDirectoryEntry) !=
            1 ||
        reinterpret_cast<std::intptr_t>(writtenSlots[1].material.currentTextureDirectoryEntry) !=
            0 ||
        slots[0].material.currentTextureDirectoryEntry != &g_zImage_TexDirEntries[1]) {
        std::fclose(file);
        return 5;
    }

    zModel_MaterialCyclePartial writtenCycle{};
    std::int32_t frameIndices[2] = {};
    if (std::fread(&writtenCycle, sizeof(writtenCycle), 1, file) != 1 ||
        writtenCycle.frameCount != 2 ||
        std::fread(frameIndices, sizeof(frameIndices), 1, file) != 1 || frameIndices[0] != 2 ||
        frameIndices[1] != -1) {
        std::fclose(file);
        return 6;
    }

    std::fclose(file);
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    return 0;
}

extern "C" int zmodel_material_read_gamez_smoke() {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zModel_MaterialSlot serializedSlots[3] = {};
    serializedSlots[0].material.colorRgb.red = 255.0f;
    serializedSlots[0].material.colorRgb.green = 0.0f;
    serializedSlots[0].material.colorRgb.blue = 0.0f;
    serializedSlots[0].prevPoolIndex = -1;
    serializedSlots[0].nextPoolIndex = 1;

    serializedSlots[1].material.flags = 0x0100;
    serializedSlots[1].material.currentTextureDirectoryEntry =
        reinterpret_cast<zImage_TexDirEntryPartial *>(static_cast<std::intptr_t>(2));
    serializedSlots[1].prevPoolIndex = 0;
    serializedSlots[1].nextPoolIndex = 2;

    serializedSlots[2].material.flags = 0x0500;
    serializedSlots[2].material.currentTextureDirectoryEntry =
        reinterpret_cast<zImage_TexDirEntryPartial *>(static_cast<std::intptr_t>(1));
    serializedSlots[2].prevPoolIndex = 1;
    serializedSlots[2].nextPoolIndex = -1;

    zModel_MaterialCyclePartial serializedCycle = {};
    serializedCycle.currentFrame = 0.5f;
    serializedCycle.frameCount = 2;
    zImage_TexDirEntryPartial *serializedFrames[2] = {
        reinterpret_cast<zImage_TexDirEntryPartial *>(static_cast<std::intptr_t>(0)),
        reinterpret_cast<zImage_TexDirEntryPartial *>(static_cast<std::intptr_t>(-1)),
    };

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::int32_t header[4] = {3, 3, -1, 0};
    if (std::fwrite(header, sizeof(header), 1, file) != 1 ||
        std::fwrite(serializedSlots, sizeof(serializedSlots), 1, file) != 1 ||
        std::fwrite(&serializedCycle, sizeof(serializedCycle), 1, file) != 1 ||
        std::fwrite(serializedFrames, sizeof(serializedFrames), 1, file) != 1) {
        std::fclose(file);
        return 2;
    }

    g_zModel_MatlPool =
        static_cast<zModel_MaterialSlot *>(std::malloc(sizeof(zModel_MaterialSlot)));
    if (g_zModel_MatlPool == nullptr) {
        std::fclose(file);
        return 3;
    }
    g_zModel_MatlPoolCapacity = 1;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;

    std::rewind(file);
    if (zModel_MatlBuffer::ReadGameZ(file) != 3) {
        std::fclose(file);
        std::free(g_zModel_MatlPool);
        g_zModel_MatlPool = nullptr;
        g_zModel_MatlPoolCapacity = 0;
        return 4;
    }
    std::fclose(file);

    zModel_MaterialSlot *const loadedSlots = g_zModel_MatlPool;
    zModel_MaterialCyclePartial *const loadedCycle = loadedSlots[2].material.cycle;
    const bool ok =
        g_zModel_MatlPoolCapacity == 3 && g_zModel_MatlPoolInUseCount == 3 &&
        g_zModel_MatlFreeHeadIndex == -1 && g_zModel_MatlActiveHeadIndex == 0 &&
        loadedSlots[0].material.packedColor == 0xf800 &&
        loadedSlots[1].material.currentTextureDirectoryEntry == &g_zImage_TexDirEntries[2] &&
        loadedSlots[2].material.currentTextureDirectoryEntry == &g_zImage_TexDirEntries[1] &&
        loadedCycle != nullptr && loadedCycle->frameCount == 2 &&
        loadedCycle->frameTable != nullptr &&
        loadedCycle->frameTable[0] == &g_zImage_TexDirEntries[0] &&
        loadedCycle->frameTable[1] == nullptr;

    if (loadedCycle != nullptr) {
        std::free(loadedCycle->frameTable);
        std::free(loadedCycle);
    }
    std::free(g_zModel_MatlPool);
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;

    return ok ? 0 : 5;
}

extern "C" int zmodel_dipool_write_to_stream_smoke() {
    zVec3 verts[1] = {{1.0f, 2.0f, 3.0f}};
    zVec3 blendVerts[1] = {{4.0f, 5.0f, 6.0f}};
    zVec3 pointCamList[1] = {{7.0f, 8.0f, 9.0f}};
    zModel_PointEntryPartial pointEntries[1] = {};
    pointEntries[0].pointCamCount = 1;
    pointEntries[0].pointCamList = pointCamList;

    std::int32_t vertexIndices[3] = {0, 1, 2};
    std::int32_t normalIndices[3] = {2, 1, 0};
    zModel_Uv uvPairs[3] = {
        {0.0f, 0.25f},
        {0.5f, 0.75f},
        {1.0f, 1.25f},
    };

    zModel_MaterialSlot materialSlots[2] = {};
    materialSlots[1].material.flags = 0x0100;
    g_zModel_MatlPool = materialSlots;

    zDiEntryPartial entries[1] = {};
    entries[0].flagsAndIndexCount = 0x0203;
    entries[0].vertexIndices = vertexIndices;
    entries[0].normalIndices = normalIndices;
    entries[0].uvPairs = uvPairs;
    entries[0].material = &materialSlots[1].material;

    zDiPartial pool[2] = {};
    pool[0].entryCount = 1;
    pool[0].vertCount = 1;
    pool[0].blendVertCount = 1;
    pool[0].pointCount = 1;
    pool[0].entries = entries;
    pool[0].verts = verts;
    pool[0].pointEntries = pointEntries;
    pool[0].blendVerts = blendVerts;
    pool[0].nextFreeIndex = -1;
    pool[1].nextFreeIndex = 7;

    g_zModel_DiPoolBase = pool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 1;
    g_zModel_DiPoolFreeHeadIndex = 1;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (zModel_DiPool::WriteToStream(file) != 2) {
        std::fclose(file);
        return 2;
    }

    const long tableOffset = 12;
    const long tableBytes = static_cast<long>(sizeof(zDiPartial) * 2);
    const long dynamicOffset = tableOffset + tableBytes;
    const long expectedBytes =
        dynamicOffset + static_cast<long>(sizeof(zVec3)) + static_cast<long>(sizeof(zVec3)) +
        static_cast<long>(sizeof(zModel_PointEntryPartial)) + static_cast<long>(sizeof(zVec3)) +
        static_cast<long>(sizeof(zDiEntryPartial)) + static_cast<long>(sizeof(vertexIndices)) +
        static_cast<long>(sizeof(normalIndices)) + static_cast<long>(sizeof(uvPairs));

    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != expectedBytes ||
        pool[0].nextFreeIndex != dynamicOffset) {
        std::fclose(file);
        return 3;
    }

    std::int32_t header[3] = {};
    zDiPartial writtenPool[2] = {};
    std::rewind(file);
    if (std::fread(header, sizeof(header), 1, file) != 1 || header[0] != 2 || header[1] != 1 ||
        header[2] != 1 || std::fread(writtenPool, sizeof(writtenPool), 1, file) != 1 ||
        writtenPool[0].nextFreeIndex != dynamicOffset || writtenPool[1].nextFreeIndex != 7) {
        std::fclose(file);
        return 4;
    }

    zVec3 writtenVerts[2] = {};
    zModel_PointEntryPartial writtenPoint = {};
    zVec3 writtenPointCam = {};
    zDiEntryPartial writtenEntry = {};
    std::int32_t writtenVertexIndices[3] = {};
    std::int32_t writtenNormalIndices[3] = {};
    zModel_Uv writtenUvs[3] = {};
    if (std::fread(&writtenVerts[0], sizeof(zVec3), 1, file) != 1 ||
        std::fread(&writtenVerts[1], sizeof(zVec3), 1, file) != 1 ||
        std::fread(&writtenPoint, sizeof(writtenPoint), 1, file) != 1 ||
        std::fread(&writtenPointCam, sizeof(writtenPointCam), 1, file) != 1 ||
        std::fread(&writtenEntry, sizeof(writtenEntry), 1, file) != 1 ||
        std::fread(writtenVertexIndices, sizeof(writtenVertexIndices), 1, file) != 1 ||
        std::fread(writtenNormalIndices, sizeof(writtenNormalIndices), 1, file) != 1 ||
        std::fread(writtenUvs, sizeof(writtenUvs), 1, file) != 1) {
        std::fclose(file);
        return 5;
    }

    const bool ok =
        writtenVerts[0].x == 1.0f && writtenVerts[1].x == 4.0f && writtenPoint.pointCamCount == 1 &&
        writtenPointCam.z == 9.0f && reinterpret_cast<std::intptr_t>(writtenEntry.material) == 1 &&
        writtenVertexIndices[2] == 2 && writtenNormalIndices[0] == 2 && writtenUvs[2].v == 1.25f;

    std::fclose(file);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zModel_MatlPool = nullptr;
    return ok ? 0 : 6;
}

extern "C" int zmodel_dipool_read_from_stream_smoke() {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zModel_MaterialSlot materialSlots[1] = {};
    materialSlots[0].material.flags = 0x0100;
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 1;

    zDiPartial serializedDi = {};
    serializedDi.entryCount = 1;
    serializedDi.vertCount = 1;
    serializedDi.normalCount = 1;
    serializedDi.blendVertCount = 1;
    serializedDi.pointCount = 1;
    serializedDi.nextFreeIndex = 12;

    zVec3 verts[1] = {{1.0f, 2.0f, 3.0f}};
    zVec3 normals[1] = {{4.0f, 5.0f, 6.0f}};
    zVec3 blendVerts[1] = {{7.0f, 8.0f, 9.0f}};
    zModel_PointEntryPartial pointEntry = {};
    pointEntry.pointCamCount = 1;
    pointEntry.colorB = 255.0f;
    pointEntry.colorG = 0.0f;
    pointEntry.colorR = 0.0f;
    pointEntry.packedColor16 = static_cast<std::int32_t>(0xabcd0000u);
    zVec3 pointCam[1] = {{10.0f, 11.0f, 12.0f}};

    zDiEntryPartial diEntry = {};
    diEntry.flagsAndIndexCount = 0x0203;
    diEntry.material = reinterpret_cast<zModel_MaterialPartial *>(static_cast<std::intptr_t>(0));
    std::int32_t vertexIndices[3] = {0, 1, 2};
    std::int32_t normalIndices[3] = {2, 1, 0};
    zModel_Uv uvs[3] = {
        {0.0f, 0.25f},
        {0.5f, 0.75f},
        {1.0f, 1.25f},
    };

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::int32_t header[3] = {1, 1, -1};
    if (std::fwrite(header, sizeof(header), 1, file) != 1 ||
        std::fwrite(&serializedDi, sizeof(serializedDi), 1, file) != 1 ||
        std::fwrite(verts, sizeof(verts), 1, file) != 1 ||
        std::fwrite(normals, sizeof(normals), 1, file) != 1 ||
        std::fwrite(blendVerts, sizeof(blendVerts), 1, file) != 1 ||
        std::fwrite(&pointEntry, sizeof(pointEntry), 1, file) != 1 ||
        std::fwrite(pointCam, sizeof(pointCam), 1, file) != 1 ||
        std::fwrite(&diEntry, sizeof(diEntry), 1, file) != 1 ||
        std::fwrite(vertexIndices, sizeof(vertexIndices), 1, file) != 1 ||
        std::fwrite(normalIndices, sizeof(normalIndices), 1, file) != 1 ||
        std::fwrite(uvs, sizeof(uvs), 1, file) != 1) {
        std::fclose(file);
        return 2;
    }

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    std::rewind(file);
    if (zModel_DiPool::ReadFromStream(file) != 1) {
        std::fclose(file);
        std::free(g_zModel_DiPoolBase);
        g_zModel_DiPoolBase = nullptr;
        g_zModel_DiPoolCapacity = 0;
        g_zModel_MatlPool = nullptr;
        g_zModel_MatlPoolCapacity = 0;
        return 3;
    }
    std::fclose(file);

    zDiPartial *const loadedDi = g_zModel_DiPoolBase;
    zDiEntryPartial *const loadedEntry = loadedDi->entries;
    zModel_PointEntryPartial *const loadedPoint = loadedDi->pointEntries;
    const bool ok =
        g_zModel_DiPoolCapacity == 1 && g_zModel_DiPoolInUseCount == 1 &&
        g_zModel_DiPoolFreeHeadIndex == -1 && loadedDi->verts != nullptr &&
        loadedDi->verts[0].z == 3.0f && loadedDi->normals != nullptr &&
        loadedDi->normals[0].x == 4.0f && loadedDi->blendVerts != nullptr &&
        loadedDi->blendVerts[0].y == 8.0f && loadedPoint != nullptr &&
        loadedPoint[0].pointCamList != nullptr && loadedPoint[0].pointCamList[0].z == 12.0f &&
        loadedPoint[0].packedColor16 == static_cast<std::int32_t>(0xabcdf800u) &&
        loadedEntry != nullptr && loadedEntry[0].material == &materialSlots[0].material &&
        static_cast<std::int32_t *>(loadedEntry[0].vertexIndices)[2] == 2 &&
        static_cast<std::int32_t *>(loadedEntry[0].normalIndices)[0] == 2 &&
        static_cast<zModel_Uv *>(loadedEntry[0].uvPairs)[2].v == 1.25f;

    std::free(loadedEntry[0].uvPairs);
    std::free(loadedEntry[0].normalIndices);
    std::free(loadedEntry[0].vertexIndices);
    std::free(loadedDi->entries);
    std::free(loadedPoint[0].pointCamList);
    std::free(loadedDi->pointEntries);
    std::free(loadedDi->blendVerts);
    std::free(loadedDi->normals);
    std::free(loadedDi->verts);
    std::free(g_zModel_DiPoolBase);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;

    return ok ? 0 : 4;
}

extern "C" int zmodel_dipool_read_entry_by_index_from_stream_smoke() {
    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    const std::int32_t dynamicOffset = 12 + 2 * static_cast<std::int32_t>(sizeof(zDiPartial));
    std::int32_t header[3] = {2, 1, 0};
    zDiPartial serializedSlots[2] = {};
    serializedSlots[0].mode = 11;
    serializedSlots[0].nextFreeIndex = dynamicOffset;
    serializedSlots[1].mode = 22;
    serializedSlots[1].flags = 0x44;
    serializedSlots[1].bboxRadius = 3.5f;
    serializedSlots[1].nextFreeIndex = dynamicOffset;

    if (std::fwrite(header, sizeof(header), 1, file) != 1 ||
        std::fwrite(serializedSlots, sizeof(serializedSlots), 1, file) != 1) {
        std::fclose(file);
        return 2;
    }

    zDiPartial livePool[2] = {};
    livePool[1].nextFreeIndex = 0;
    g_zModel_DiPoolBase = livePool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = 1;

    std::rewind(file);
    zDiPartial *const loaded = zModel_DiPool::ReadEntryByIndexFromStream(file, 1);
    const bool loadedOk = loaded == &livePool[1] && livePool[1].mode == 22 &&
                          livePool[1].flags == 0x44 && livePool[1].bboxRadius == 3.5f &&
                          livePool[1].nextFreeIndex == 0 && g_zModel_DiPoolInUseCount == 1 &&
                          g_zModel_DiPoolFreeHeadIndex == 0;

    std::rewind(file);
    zDiPartial *const outOfRange = zModel_DiPool::ReadEntryByIndexFromStream(file, 2);
    const bool rangeOk = outOfRange == nullptr && g_zModel_DiPoolInUseCount == 1 &&
                         g_zModel_DiPoolFreeHeadIndex == 0;

    std::fclose(file);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    return loadedOk && rangeOk ? 0 : 3;
}

extern "C" int zmodel_material_and_di_clone_smoke() {
    zModel_MaterialPartial material = {};
    if (zModel_Material::HasAuxData(&material) != 0) {
        return 1;
    }
    material.flags = 0x0200;
    if (zModel_Material::HasAuxData(&material) != 1) {
        return 2;
    }
    material.flags = 0;
    zModel_MaterialCyclePartial cycle = {};
    material.cycle = &cycle;
    if (zModel_Material::HasAuxData(&material) != 1) {
        return 3;
    }

    zModel_MaterialSlot slots[2] = {};
    slots[0].prevPoolIndex = -1;
    slots[0].nextPoolIndex = -1;
    g_zModel_MatlPool = slots;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlPoolInUseCount = 0;

    material.flags = 0x0100;
    material.cycle = reinterpret_cast<zModel_MaterialCyclePartial *>(0x1234);
    zModel_MaterialPartial *clone = zModel_Material::Clone(&material);
    if (clone != &slots[0].material || clone->flags != 0x0100 || clone->cycle != nullptr ||
        g_zModel_MatlFreeHeadIndex != -1 || g_zModel_MatlActiveHeadIndex != 0 ||
        g_zModel_MatlPoolInUseCount != 1) {
        return 4;
    }

    zImage_TexDirEntryPartial *frameTable[2] = {};
    zModel_MaterialCyclePartial sourceCycle = {};
    sourceCycle.frameCount = 2;
    sourceCycle.frameTable = frameTable;
    material.flags = 0x0400;
    material.cycle = &sourceCycle;
    slots[0].prevPoolIndex = -1;
    slots[0].nextPoolIndex = -1;
    slots[1].prevPoolIndex = -1;
    slots[1].nextPoolIndex = -1;
    g_zModel_MatlFreeHeadIndex = 1;
    g_zModel_MatlActiveHeadIndex = 0;
    clone = zModel_Material::Clone(&material);
    if (clone != &slots[1].material || clone->cycle == &sourceCycle || clone->cycle == nullptr ||
        clone->cycle->frameTable == frameTable || clone->cycle->frameCount != 2 ||
        slots[1].nextPoolIndex != 0 || slots[0].prevPoolIndex != 1 ||
        g_zModel_MatlActiveHeadIndex != 1) {
        return 5;
    }
    std::free(clone->cycle->frameTable);
    std::free(clone->cycle);
    clone->cycle = nullptr;

    g_zModel_DefaultMaterial.flags = 0x55;
    g_zModel_MatlFreeHeadIndex = -1;
    if (zModel_Material::Clone(&material) != &g_zModel_DefaultMaterial) {
        return 6;
    }

    zDiPartial diPool[2] = {};
    diPool[1].mode = 99;
    diPool[1].flags = 0xffffffff;
    diPool[1].nextFreeIndex = 0;
    g_zModel_DiPoolBase = diPool;
    g_zModel_DiPoolFreeHeadIndex = 1;
    g_zModel_DiPoolInUseCount = 0;
    zDiPartial *const allocated = zModel_DiPool::AllocFromFreeList();
    if (allocated != &diPool[1] || g_zModel_DiPoolFreeHeadIndex != 0 ||
        g_zModel_DiPoolInUseCount != 1 || allocated->mode != 0 || allocated->flags != 3 ||
        allocated->nextFreeIndex != 0) {
        return 7;
    }

    zDi::SetClonedFlag(allocated, 0);
    if ((allocated->flags & 0x02) != 0) {
        return 8;
    }
    zDi::SetClonedFlag(allocated, 1);
    if ((allocated->flags & 0x02) == 0) {
        return 9;
    }

    zDiEntryPartial specialEntry = {};
    zModel_MaterialPartial plainMaterial = {};
    zModel_MaterialPartial auxMaterial = {};
    auxMaterial.flags = 0x0200;
    zDiPartial specialDi = {};
    specialDi.flags = 0x08;
    if (zDi::HasSpecialFlagsOrAuxMaterialData(nullptr) != 0 ||
        zDi::HasSpecialFlagsOrAuxMaterialData(&specialDi) != 1) {
        return 10;
    }
    specialDi.flags = 0;
    specialDi.entryCount = 1;
    specialDi.entries = &specialEntry;
    specialEntry.material = &plainMaterial;
    if (zDi::HasSpecialFlagsOrAuxMaterialData(&specialDi) != 0) {
        return 11;
    }
    specialEntry.material = &auxMaterial;
    if (zDi::HasSpecialFlagsOrAuxMaterialData(&specialDi) != 1) {
        return 12;
    }

    zDiPartial source = {};
    source.mode = 0x1234;
    source.flags = 0x3f;
    source.blendScale = 1.5f;
    source.textureWorldPerMeter = 2.5f;
    source.textureWorldAxis = 3;
    source.field2c = 4;

    zVec3 pointCamList[1] = {{1.0f, 2.0f, 3.0f}};
    zModel_PointEntryPartial pointEntries[1] = {};
    pointEntries[0].pointCamCount = 1;
    pointEntries[0].pointCamList = pointCamList;
    source.pointCount = 1;
    source.pointEntries = pointEntries;

    zVec3 verts[1] = {{4.0f, 5.0f, 6.0f}};
    zVec3 normals[1] = {{7.0f, 8.0f, 9.0f}};
    zVec3 blendVerts[1] = {{10.0f, 11.0f, 12.0f}};
    source.vertCount = 1;
    source.verts = verts;
    source.normalCount = 1;
    source.normals = normals;
    source.blendVertCount = 1;
    source.blendVerts = blendVerts;

    std::uint32_t vertexIndices[2] = {11, 22};
    std::uint32_t normalIndices[2] = {33, 44};
    std::uint32_t uvPairs[4] = {55, 66, 77, 88};
    zDiEntryPartial entries[2] = {};
    zModel_MaterialPartial uvMaterial = {};
    uvMaterial.flags = 0x0100;
    entries[0].flagsAndIndexCount = 0x0200 | 2;
    entries[0].drawFlags = 0x77777777;
    entries[0].vertexIndices = vertexIndices;
    entries[0].normalIndices = normalIndices;
    entries[0].uvPairs = uvPairs;
    entries[0].material = &uvMaterial;
    entries[0].variantTagInitialized = 1;
    entries[0].variantTag = 0x42;
    entries[1].material = &uvMaterial;
    source.entryCount = 2;
    source.entries = entries;

    zDiPartial clonePool[1] = {};
    clonePool[0].nextFreeIndex = -1;
    g_zModel_DiPoolBase = clonePool;
    g_zModel_DiPoolFreeHeadIndex = 0;
    g_zModel_DiPoolInUseCount = 0;
    zModel_MaterialSlot cloneMaterialSlots[1] = {};
    cloneMaterialSlots[0].prevPoolIndex = -1;
    cloneMaterialSlots[0].nextPoolIndex = -1;
    g_zModel_MatlPool = cloneMaterialSlots;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlPoolInUseCount = 0;

    zDiPartial *const diClone = zDi::CloneToInstance(&source, 1, 0);
    if (diClone != &clonePool[0] || diClone->mode != source.mode ||
        (diClone->flags & 0x3f) != (source.flags & 0x3f) ||
        diClone->pointEntries == source.pointEntries ||
        diClone->pointEntries[0].pointCamList == pointCamList || diClone->verts == verts ||
        diClone->normals == normals || diClone->blendVerts == blendVerts ||
        diClone->entries == entries ||
        diClone->entries[0].material != &cloneMaterialSlots[0].material ||
        diClone->entries[1].material != &cloneMaterialSlots[0].material ||
        diClone->entries[0].vertexIndices == vertexIndices ||
        diClone->entries[0].normalIndices == normalIndices ||
        diClone->entries[0].uvPairs == uvPairs) {
        zDi::FreeContents(diClone);
        return 13;
    }

    if (diClone->entries[0].flagsAndIndexCount != entries[0].flagsAndIndexCount ||
        diClone->entries[0].drawFlags != entries[0].drawFlags ||
        diClone->entries[0].variantTag != 0x42 ||
        static_cast<std::uint32_t *>(diClone->entries[0].vertexIndices)[1] != 22 ||
        static_cast<std::uint32_t *>(diClone->entries[0].normalIndices)[1] != 44 ||
        static_cast<std::uint32_t *>(diClone->entries[0].uvPairs)[3] != 88) {
        zDi::FreeContents(diClone);
        return 14;
    }

    zDi::FreeContents(diClone);
    g_zModel_MatlPool = nullptr;
    g_zModel_DiPoolBase = nullptr;
    return 0;
}

extern "C" int zmodel_scrolling_texture_update_smoke(void) {
    zModel_TextureScrollInfoPartial texture{};
    texture.wrapShiftU = 0;
    texture.wrapShiftV = 1;
    zModel_Uv uvs[2] = {{1.0f, 2.0f}, {3.0f, 4.0f}};
    float scrollRates[2] = {2.0f, 4.0f};
    g_FrameDeltaTimeSec = 0.5f;
    g_zVideo_ActiveRendererPath = 0;

    zModel_Instance_UpdateScrollingTextures(&texture, uvs, scrollRates, 2);
    if (uvs[0].u != 2.0f || uvs[0].v != 4.0f || uvs[1].u != 4.0f || uvs[1].v != 6.0f) {
        return 1;
    }

    g_zVideo_ActiveRendererPath = 1;
    uvs[0] = {127.5f, 0.0f};
    uvs[1] = {127.75f, 0.0f};
    scrollRates[0] = 2.0f;
    scrollRates[1] = 0.0f;
    zModel_Instance_UpdateScrollingTextures(&texture, uvs, scrollRates, 2);
    if (uvs[0].u != -127.5f || uvs[1].u != -127.25f || uvs[0].v != 0.0f || uvs[1].v != 0.0f) {
        return 2;
    }

    zModel_TextureRefPartial textureRef{&texture};
    zModel_MaterialTextureBindingPartial material{};
    material.flags = 1;
    material.textureRef = &textureRef;
    zModel_InstanceSurfaceEntryPartial entry{};
    entry.vertexCountAndFlags = 2;
    entry.uvs = uvs;
    entry.materialBinding = &material;
    zModel_InstancePartial instance{};
    instance.surfaceEntryCount = 1;
    instance.scrollRateU = 0.0f;
    instance.scrollRateV = 1.0f;
    instance.scrollingTextureFrameTick = 11;
    instance.surfaceEntries = &entry;
    g_FrameDeltaTimeSec = 1.0f;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_FrameTick = 12;
    uvs[0] = {0.0f, 1.0f};
    uvs[1] = {0.0f, 2.0f};

    if (zModel_Instance_UpdateScrollingTexturesIfNeeded(&instance) != 0 ||
        instance.scrollingTextureFrameTick != 12 || uvs[0].v != 2.0f || uvs[1].v != 3.0f) {
        return 3;
    }

    zModel_Instance_UpdateScrollingTexturesIfNeeded(&instance);
    if (uvs[0].v != 2.0f || uvs[1].v != 3.0f) {
        return 4;
    }

    return zModel_Instance_UpdateScrollingTexturesIfNeeded(nullptr) == -1 ? 0 : 5;
}

extern "C" int zmodel_material_update_cycle_if_needed_smoke(void) {
    zImage_TexDirEntryPartial frameA{};
    zImage_TexDirEntryPartial frameB{};
    zImage_TexDirEntryPartial frameC{};
    zImage_TexDirEntryPartial *frames[3] = {&frameA, &frameB, &frameC};
    zModel_MaterialCyclePartial cycle{};
    zModel_MaterialPartial material{};

    material.flags = 0x0400;
    material.cycle = &cycle;
    material.currentTextureDirectoryEntry = &frameA;
    cycle.loopEnabled = 1;
    cycle.lastUpdateFrameTick = 99;
    cycle.currentFrame = 1.0f;
    cycle.framesPerSecond = 2.0f;
    cycle.frameCount = 3;
    cycle.frameTable = frames;
    g_zVideo_FrameTick = 100;
    g_FrameDeltaTimeSec = 0.5f;

    zModel_Material::UpdateCycleIfNeeded(&material);
    if (material.currentTextureDirectoryEntry != &frameB || cycle.currentFrame != 2.0f ||
        cycle.lastUpdateFrameTick != 100) {
        return 1;
    }

    material.currentTextureDirectoryEntry = &frameA;
    zModel_Material::UpdateCycleIfNeeded(&material);
    if (material.currentTextureDirectoryEntry != &frameA || cycle.currentFrame != 2.0f) {
        return 2;
    }

    cycle.loopEnabled = 0;
    cycle.lastUpdateFrameTick = 100;
    cycle.currentFrame = 2.5f;
    cycle.framesPerSecond = 2.0f;
    material.currentTextureDirectoryEntry = &frameA;
    g_zVideo_FrameTick = 101;
    zModel_Material::UpdateCycleIfNeeded(&material);
    if (material.currentTextureDirectoryEntry != &frameC || cycle.currentFrame != 2.0f) {
        return 3;
    }

    cycle.loopEnabled = 1;
    cycle.lastUpdateFrameTick = 101;
    cycle.currentFrame = 0.0f;
    cycle.framesPerSecond = -2.0f;
    material.currentTextureDirectoryEntry = &frameC;
    g_zVideo_FrameTick = 102;
    g_FrameDeltaTimeSec = 1.0f;
    zModel_Material::UpdateCycleIfNeeded(&material);
    return material.currentTextureDirectoryEntry == &frameA && cycle.currentFrame == 4.0f ? 0 : 4;
}

extern "C" int zmodel_render_point_queue_entry_smoke(void) {
    static std::int32_t matrixFlags[1] = {1};
    static float *matrixSlots[1] = {};
    zMat4x3 matrix{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    g_zMath_ProjScaleX = 1.0f;
    g_zMath_ProjScaleY = 1.0f;
    g_zMath_ProjOffsetX = 0.0f;
    g_zMath_ProjOffsetY = 0.0f;
    g_zMath_ProjSphereRadiusScale = 10.0f;
    gClipRect_Primary.zMin = 1.0f;
    g_zVideo_ProjectClipLeft = 0.0f;
    g_zVideo_ProjectClipTop = 0.0f;
    g_zVideo_ProjectClipRight = 2.0f;
    g_zVideo_ProjectClipBottom = 3.0f;
    zRndr::g_inverseDepthBias = 0.0f;
    zRndr::g_inverseDepthScale = 1.0f;
    zRndr::g_lensFlareSampleQueueCount = 0;
    g_drawPointCallCount = 0;

    zModel_PointEntryPartial entry{};
    zVec3 point{10.0f, 20.0f, 10.0f};
    g_zVideo_ActiveRendererPath = 0;
    zModel_RenderPointQueueEntry(&point, 0x1234beef, &entry);
    if (zRndr::g_lensFlareSampleQueueCount != 1 ||
        zRndr::g_lensFlareSampleQueue[0].packedColor16 != 0xbeef ||
        zRndr::g_lensFlareSampleQueue[0].x < 0.999f ||
        zRndr::g_lensFlareSampleQueue[0].x > 1.001f ||
        zRndr::g_lensFlareSampleQueue[0].y < 1.999f ||
        zRndr::g_lensFlareSampleQueue[0].y > 2.001f ||
        zRndr::g_lensFlareSampleQueue[0].reciprocalZ < 0.099f ||
        zRndr::g_lensFlareSampleQueue[0].reciprocalZ > 0.101f) {
        return 1;
    }

    point.z = 1.0f;
    zModel_RenderPointQueueEntry(&point, 0xbeef, &entry);
    if (zRndr::g_lensFlareSampleQueueCount != 1) {
        return 2;
    }

    point = {10.0f, 20.0f, 10.0f};
    entry.depthBiasWord = 2;
    g_zRndr_InverseZTolerance = 0.5f;
    zRndr::g_lensFlareSampleQueueCount = 0;
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_pfnDrawPointColor16 = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&zmodel_draw_point_color16_stub));
    zModel_RenderPointQueueEntry(&point, 0x1ffff, &entry);
    if (g_drawPointCallCount != 1 || g_drawPointLastColor != 0xffff || g_drawPointLastCount != 1 ||
        g_drawPointLastPoint.reciprocalZ < 1.999f || g_drawPointLastPoint.reciprocalZ > 2.001f ||
        zRndr::g_lensFlareSampleQueueCount != 1) {
        return 3;
    }

    g_zVideo_pfnDrawPointColor16 = 0;
    g_zVideo_ActiveRendererPath = 0;
    return 0;
}

extern "C" int zmodel_init_smoke(void) {
    const int savedRendererPath = g_zVideo_ActiveRendererPath;
    zDiPartial *const savedPool = g_zModel_DiPoolBase;
    const int savedCapacity = g_zModel_DiPoolCapacity;
    const int savedInUse = g_zModel_DiPoolInUseCount;
    const int savedFreeHead = g_zModel_DiPoolFreeHeadIndex;
    zClass_RenderFn const savedRenderFn = gModel_RenderFn;
    int *const savedClipStackTop = gModel_ClipMaskStackTop;
    const int savedSoftwarePathActive = g_zModel_SoftwarePathActive;

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 3;
    g_zModel_DiPoolInUseCount = 99;
    g_zModel_DiPoolFreeHeadIndex = 99;
    gModel_RenderFn = nullptr;
    gModel_ClipMaskStackTop = nullptr;
    g_zModel_SoftwarePathActive = 1;
    g_zVideo_ActiveRendererPath = 1;

    const int initResult = zModel::Init();
    zDiPartial *const allocatedPool = g_zModel_DiPoolBase;
    const bool hardwareOk =
        initResult == 0 && allocatedPool != nullptr && gModel_RenderFn == zModel::RenderNodeHardware &&
        g_zModel_SoftwarePathActive == 0 && gModel_ClipMaskStackTop == gModel_ClipMaskStack &&
        g_zModel_DiPoolCapacity == 3 && g_zModel_DiPoolInUseCount == 0 &&
        g_zModel_DiPoolFreeHeadIndex == 0 && allocatedPool[0].nextFreeIndex == 1 &&
        allocatedPool[1].nextFreeIndex == 2 && allocatedPool[2].nextFreeIndex == -1;
    std::free(allocatedPool);

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    gModel_RenderFn = savedRenderFn;
    g_zModel_SoftwarePathActive = 0;
    g_zVideo_ActiveRendererPath = 0;

    const int softwareResult = zModel::Init();
    zDiPartial *const defaultPool = g_zModel_DiPoolBase;
    const bool softwareOk = softwareResult == 0 && defaultPool != nullptr &&
                            g_zModel_DiPoolCapacity == 1750 &&
                            g_zModel_SoftwarePathActive == 1 &&
                            defaultPool[1749].nextFreeIndex == -1;
    std::free(defaultPool);

    g_zModel_DiPoolBase = savedPool;
    g_zModel_DiPoolCapacity = savedCapacity;
    g_zModel_DiPoolInUseCount = savedInUse;
    g_zModel_DiPoolFreeHeadIndex = savedFreeHead;
    gModel_RenderFn = savedRenderFn;
    gModel_ClipMaskStackTop = savedClipStackTop;
    g_zModel_SoftwarePathActive = savedSoftwarePathActive;
    g_zVideo_ActiveRendererPath = savedRendererPath;

    if (!hardwareOk) {
        return 1;
    }
    return softwareOk ? 0 : 2;
}

extern "C" int zmodel_render_node_hardware_flat_smoke(void) {
    static std::int32_t matrixFlags[1] = {1};
    static float *matrixSlots[1] = {};
    zMat4x3 matrix{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    zMath::g_zMath_CameraScratchA = matrix;
    zMath::g_zMath_CameraScratchB = matrix;
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    g_zModel_TransformedVerts = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_TransformedNormals = g_zModel_SharedVec3ScratchBStorage;
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_pfnSubmitPolyFlatColor16 = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&zmodel_submit_poly_flat_stub));
    g_zVideo_pfnSubmitPolyColorAttr = 0;
    g_zVideo_pfnSubmitPolygon = 0;
    g_zVideo_pfnSubmitPolygonLit = 0;
    g_zVideo_pfnSubmitPolyRenderClass = 0;
    g_zMath_ProjScaleX = 10.0f;
    g_zMath_ProjScaleY = 10.0f;
    g_zMath_ProjOffsetX = 0.0f;
    g_zMath_ProjOffsetY = 0.0f;
    g_zMath_ProjSphereRadiusScale = 10.0f;
    g_zRndr_InverseZTolerance = 0.25f;
    g_zModel_BFETolerance = 0.0f;
    gModel_FogEnabled = 0;
    gModel_HasActiveLights = 0;
    gModel_RenderAlphaScaleCurrent = 1.0f;
    gModel_RenderVertexAlphaEnabled = 7;
    gAltClipPassEnabled = 0;
    g_submitFlatCallCount = 0;

    gClipRect_Primary.xMin = -100.0f;
    gClipRect_Primary.yMin = -100.0f;
    gClipRect_Primary.zMin = 1.0f;
    gClipRect_Primary.xMax = 100.0f;
    gClipRect_Primary.yMax = 100.0f;
    gClipRect_Primary.zMax = 1000.0f;
    gClipRect_Primary.xMaxAlt = 100.0f;
    gClipRect_Primary.yMaxAlt = 100.0f;

    zVec3 verts[3] = {
        {-1.0f, 0.0f, 10.0f},
        {0.0f, 1.0f, 10.0f},
        {1.0f, 0.0f, 10.0f},
    };
    int indices[3] = {0, 2, 1};
    zModel_MaterialPartial material{};
    material.flags = 0xff;
    material.packedColor = 0x1234;
    zDiEntryPartial entry{};
    entry.flagsAndIndexCount = 3;
    entry.drawFlags = 2;
    entry.vertexIndices = indices;
    entry.material = &material;
    zDiPartial di{};
    di.entryCount = 1;
    di.vertCount = 3;
    di.entries = &entry;
    di.verts = verts;
    zClass_NodePartial node{};
    node.userDataOrDiRef = static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&di));

    zModel::RenderNodeHardware(&node, 0x3f);

    g_zVideo_pfnSubmitPolyFlatColor16 = 0;
    g_zVideo_ActiveRendererPath = 0;
    if (g_submitFlatCallCount != 1) {
        return 1;
    }
    if (g_submitFlatLastVertexCount != 3) {
        return 3;
    }
    if (g_submitFlatLastAlpha != 255 || g_submitFlatLastColor != 0x1234) {
        return 4;
    }
    if (g_submitFlatLastRenderParam != 2 || g_submitFlatLastQueueMode != 7) {
        return 5;
    }

    const float expectedDepth = 1.5f;
    for (int i = 0; i < 3; ++i) {
        if (g_submitFlatLastVerts[i].z < expectedDepth - 0.001f ||
            g_submitFlatLastVerts[i].z > expectedDepth + 0.001f) {
            return 2;
        }
    }

    return 0;
}

extern "C" int zmodel_light_fog_fade_smoke() {
    zClass_LightDataPartial light{};
    light.range1 = 5.0f;
    light.range2 = 15.0f;
    light.invRangeDelta = 0.1f;
    if (zModel_Light::EvalDistanceWeight(&light, 15.0f) != 0.0f ||
        zModel_Light::EvalDistanceWeight(&light, 16.0f) != 0.0f ||
        zModel_Light::EvalDistanceWeight(&light, 5.0f) != 1.0f ||
        zModel_Light::EvalDistanceWeight(&light, 4.0f) != 1.0f) {
        return 5;
    }

    const float distanceFade = zModel_Light::EvalDistanceWeight(&light, 10.0f);
    if (distanceFade < 0.499f || distanceFade > 0.501f) {
        return 6;
    }

    zMath::g_zMath_CameraScratchA = {};
    zMath::g_zMath_CameraScratchA.yy = 1.0f;

    gModel_FogDistanceStart = 10.0f;
    gModel_FogDistanceEnd = 20.0f;
    gModel_FogDistanceInvRange = 0.1f;
    gModel_FogHeightHigh = 10.0f;
    gModel_FogHeightLow = 0.0f;
    gModel_FogHeightInvRange = 0.1f;

    zVec3 point{0.0f, 5.0f, 0.0f};
    if (zModel_Light::EvalSphereFogFade(&point, 1.0f) != 0.0f) {
        return 1;
    }

    gModel_FogDistanceStart = -2.0f;
    gModel_FogDistanceEnd = -1.0f;
    gModel_FogDistanceInvRange = 1.0f;
    point.y = -5.0f;
    if (zModel_Light::EvalSphereFogFade(&point, 0.5f) != 1.0f) {
        return 2;
    }

    gModel_FogDistanceStart = -1.0f;
    gModel_FogDistanceEnd = 1.0f;
    gModel_FogDistanceInvRange = 0.5f;
    point.y = 5.0f;
    const float partial = zModel_Light::EvalSphereFogFade(&point, 0.0f);
    if (partial < 0.249f || partial > 0.251f) {
        return 3;
    }

    point.y = 20.0f;
    if (zModel_Light::EvalSphereFogFade(&point, 1.0f) != 0.0f) {
        return 4;
    }

    std::memset(gModel_ActiveLights, 0, sizeof(gModel_ActiveLights));
    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    std::memset(g_Clip_PolyAttr1, 0, sizeof(g_Clip_PolyAttr1));
    zModel_LightStatePartial lightStates[3] = {};
    zClass_LightDataPartial lights[3] = {};
    for (std::int32_t i = 0; i < 3; ++i) {
        lightStates[i].flags = 4;
        lights[i].enabled = 1;
        lights[i].lightSubMode = 1;
        lights[i].range1 = 5.0f;
        lights[i].range2 = 15.0f;
        lights[i].invRangeDelta = 0.1f;
        lights[i].falloff = 1.0f;
        lights[i].intensityScale = 1.0f;
        gModel_ActiveLights[i].light = &lights[i];
        gModel_ActiveLights[i].lightState = &lightStates[i];
    }

    lights[0].falloff = 0.25f;
    lights[0].intensityScale = 0.5f;
    lights[0].viewPos.z = 10.0f;
    lights[1].viewPos.x = 10.0f;
    lights[1].viewPos.z = 10.0f;
    lights[2].isPointMode = 1;
    gModel_ActiveLightCount = 3;
    gModel_ActiveLightSpecialIndex = -1;
    g_zVideo_ActiveRendererPath = 0;
    g_zModel_SoftwarePathActive = 0;
    zVec3 sphereCenter{0.0f, 0.0f, 10.0f};

    if (zModel_Light::PointInPolygonTestRadiusXZ(&sphereCenter, 0.0f) != 3 ||
        gModel_ActiveLights[0].useFullWeight != 1 ||
        gModel_ActiveLights[0].contributesToLighting != 1 || g_Clip_PolyAttr0[0] != 0.75f ||
        gModel_ActiveLights[1].contributesToLighting != 1 || g_Clip_PolyAttr0[1] < 0.45f ||
        g_Clip_PolyAttr0[1] > 0.56f || gModel_ActiveLightSpecialIndex != 2 ||
        g_Clip_PolyAttr1[2] < 0.45f || g_Clip_PolyAttr1[2] > 0.56f) {
        return 7;
    }

    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    std::memset(g_Clip_PolyAttr1, 0, sizeof(g_Clip_PolyAttr1));
    g_zModel_SoftwarePathActive = 1;
    if (!(zModel_Light::PointInPolygonTestRadiusXZ(&sphereCenter, 0.0f) == 1 &&
          gModel_ActiveLights[0].contributesToLighting == 0 &&
          gModel_ActiveLights[1].contributesToLighting == 0 &&
          gModel_ActiveLights[2].contributesToLighting == 1 && g_Clip_PolyAttr1[2] > 0.45f &&
          g_Clip_PolyAttr1[2] < 0.56f)) {
        return 8;
    }

    static std::int32_t matrixFlags[1] = {0};
    static float *matrixSlots[1] = {};
    zMat4x3 matrix{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 10.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    std::memset(gModel_ActiveLights, 0, sizeof(gModel_ActiveLights));
    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    std::memset(g_Clip_PolyAttr1, 0, sizeof(g_Clip_PolyAttr1));
    gModel_ActiveLightCount = 2;
    gModel_HasActiveLights = 1;
    g_zModel_SoftwarePathActive = 0;
    g_zModel_FogTargetColorOverride = {};
    static std::int32_t graphicsFlags = 1;
    g_zModel_GraphicsFlagsOption = &graphicsFlags;
    zRndr::g_fogParamsActive = {};
    zRndr::g_fogParamsActive.colorRgb01[0] = -1.0f;
    zRndr::g_fogParamsActive.colorRgb01[1] = -1.0f;
    zRndr::g_fogParamsActive.colorRgb01[2] = -1.0f;

    zModel_LightStatePartial evalLightStates[2] = {};
    zClass_LightDataPartial evalLights[2] = {};
    for (std::int32_t i = 0; i < 2; ++i) {
        evalLightStates[i].flags = 4;
        evalLights[i].enabled = 1;
        evalLights[i].lightSubMode = 1;
        evalLights[i].range1 = 50.0f;
        evalLights[i].range2 = 100.0f;
        evalLights[i].invRangeDelta = 0.02f;
        evalLights[i].falloff = 1.0f;
        evalLights[i].intensityScale = 1.0f;
        evalLights[i].viewPos.z = 10.0f;
        gModel_ActiveLights[i].light = &evalLights[i];
        gModel_ActiveLights[i].lightState = &evalLightStates[i];
    }
    evalLights[0].specularColor.red = 1.0f;
    evalLights[1].specularColor.blue = 1.0f;

    zDiPartial di{};
    di.flags = 3;
    di.bboxCenter = {0.0f, 5.0f, 0.0f};
    di.bboxRadius = 1.0f;
    gModel_FogEnabled = 1;
    gModel_FogDistanceStart = 10.0f;
    gModel_FogDistanceEnd = 20.0f;
    gModel_FogDistanceInvRange = 0.1f;
    gModel_FogHeightHigh = 20.0f;
    gModel_FogHeightLow = 0.0f;
    gModel_FogHeightInvRange = 0.05f;

    std::int32_t depthFade = -1;
    std::int32_t activeLightState = -1;
    std::int32_t lensFlareVisible = -1;
    zDi::EvalBoundingSphereLightingFlags(&di, &depthFade, &activeLightState, &lensFlareVisible);
    if (depthFade != 1 || activeLightState != 1 || lensFlareVisible != 1 ||
        zRndr::g_fogParamsActive.colorRgb01[0] < 0.49f ||
        zRndr::g_fogParamsActive.colorRgb01[0] > 0.51f ||
        zRndr::g_fogParamsActive.colorRgb01[1] != 0.0f ||
        zRndr::g_fogParamsActive.colorRgb01[2] < 0.49f ||
        zRndr::g_fogParamsActive.colorRgb01[2] > 0.51f) {
        return 9;
    }

    g_zModel_FogTargetColorOverride.colorRgb01.green = 0.75f;
    g_zModel_FogTargetColorOverride.weight = 0.5f;
    gModel_HasActiveLights = 0;
    activeLightState = -1;
    lensFlareVisible = -1;
    zDi::EvalBoundingSphereLightingFlags(&di, &depthFade, &activeLightState, &lensFlareVisible);
    const bool overrideOnlyOk = activeLightState == 1 && lensFlareVisible == 0;

    gModel_HasActiveLights = 0;
    gModel_ActiveLightCount = 0;
    g_zModel_SoftwarePathActive = 0;
    g_zModel_FogTargetColorOverride = {};
    gModel_DefaultGraphicsFlags = 0;
    g_zModel_GraphicsFlagsOption = &gModel_DefaultGraphicsFlags;

    return overrideOnlyOk ? 0 : 10;
}

extern "C" int zmodel_light_build_light_weights_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_pixelPackRedMask = 0xf800;
    zRndr::g_pixelPackGreenMask = 0x07e0;
    zRndr::g_pixelPackBlueMask = 0x001f;
    zRndr::g_fogParamsActive = {};
    zRndr::g_fogTargetParamsStaged = {};
    zRndr::g_fogTargetParamsDirect = {};

    zClass_LightDataPartial light{};
    light.enabled = 1;
    light.viewDir = {0.0f, 0.0f, 1.0f};
    light.viewPos = {0.0f, 0.0f, 10.0f};
    light.falloff = 0.0f;
    light.intensityScale = 0.5f;
    light.specularColor = {1.0f, 0.0f, 0.0f};
    light.range1 = 0.0f;
    light.range2 = 100.0f;
    light.range2Sq = 10000.0f;
    light.invRangeDelta = 0.01f;

    zModel_LightStatePartial lightState{};
    lightState.flags = 4;
    gModel_ActiveLightCount = 1;
    gModel_ActiveLights[0] = {};
    gModel_ActiveLights[0].light = &light;
    gModel_ActiveLights[0].lightState = &lightState;
    gModel_ActiveLights[0].useFullWeight = 1;
    gModel_ActiveLights[0].contributesToLighting = 1;
    g_zModel_FogTargetColorOverride = {};

    g_Clip_PolyVertsScratch[0] = {0.0f, 0.0f, 5.0f};
    g_Clip_PolyVertsScratch[1] = {1.0f, 0.0f, 5.0f};
    g_Clip_PolyVertsScratch[2] = {0.0f, 1.0f, 5.0f};
    zVec3 normal{0.0f, 0.0f, 2.0f};
    std::int32_t packedColor = 0;
    if (zModel_Light_BuildLightWeights(&normal, 3, &packedColor, 0.0f) != 1) {
        return 1;
    }

    if (normal.z < 0.999f || normal.z > 1.001f || zRndr::g_fogParamsActive.colorRgb01[0] != 1.0f ||
        zRndr::g_fogParamsActive.colorRgb01[1] != 0.0f ||
        zRndr::g_fogParamsActive.colorRgb01[2] != 0.0f || packedColor == 0) {
        return 2;
    }

    gModel_ActiveLightCount = 0;
    gModel_ActiveLights[0] = {};
    g_zModel_FogTargetColorOverride = {};
    packedColor = 0x1234;
    return zModel_Light_BuildLightWeights(&normal, 3, &packedColor, 0.0f) == 0 &&
                   packedColor == 0x1234
               ? 0
               : 3;
}

extern "C" int zmodel_light_point_in_polygon_init_smoke(void) {
    zClass_LightDataPartial light0{};
    zClass_LightDataPartial light1{};
    light0.specularColor = {0.2f, 0.3f, 0.4f};
    light0.intensityScale = 0.25f;
    light1.specularColor = {0.7f, 0.6f, 0.5f};
    light1.intensityScale = 0.125f;
    light1.isPointMode = 1;
    zClass_LightDataPartial *lights[2] = {&light0, &light1};

    zModel_LightStatePartial state0{};
    zModel_LightStatePartial state1{};
    state1.flags = 4;
    zModel_LightStatePartial *states[2] = {&state0, &state1};

    g_zVideo_ActiveRendererPath = 0;
    gModel_FogColorRgb01 = {0.1f, 0.2f, 0.3f};
    gModel_SpecialLightPaletteRemapRecipe = {};
    zModel_Light_PointInPolygonInitXZ(lights, states, 2);
    if (gModel_LightInputDataList != lights || gModel_LightInputNodeStates != states ||
        gModel_LightInputCount != 2 || gModel_ActiveLightCount != 1 ||
        gModel_HasActiveLights != 1 || gModel_ActiveLightSpecialIndex != 0 ||
        gModel_ActiveLights[0].light != &light1 || gModel_AmbientColorRgb01.red != 0.7f ||
        gModel_AmbientIntensityFactor != 0.875f ||
        gModel_SpecialLightPaletteRemapRecipe.color1Strength != 1.0f ||
        gModel_SpecialLightPaletteRemapRecipe.color0Strength != 0.0f) {
        return 1;
    }

    state1.flags = 0;
    zModel_Light_PointInPolygonInitXZ(lights, states, 2);
    return gModel_ActiveLightCount == 0 && gModel_HasActiveLights == 0 &&
                   gModel_AmbientColorRgb01.red == gModel_FogColorRgb01.red &&
                   gModel_SpecialLightPaletteRemapRecipe.color1R == 0.0f
               ? 0
               : 2;
}

extern "C" int zmodel_light_set_active_lights_smoke(void) {
    std::memset(gModel_ActiveLights, 0, sizeof(gModel_ActiveLights));
    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    std::memset(g_Clip_PolyAttr1, 0, sizeof(g_Clip_PolyAttr1));
    std::memset(g_Clip_PolyAttr2, 0, sizeof(g_Clip_PolyAttr2));
    g_zModel_FogTargetColorOverride = {};
    g_zModel_CurrentPolyNormals = nullptr;

    zModel_LightStatePartial state{};
    state.flags = 4;
    zClass_LightDataPartial light{};
    light.enabled = 1;
    light.lightSubMode = 1;
    light.range1 = 0.0f;
    light.range2 = 100.0f;
    light.range2Sq = 10000.0f;
    light.invRangeDelta = 0.01f;
    light.falloff = 1.0f;
    light.intensityScale = 0.0f;
    light.viewPos = {0.0f, 0.0f, 10.0f};
    light.viewDir = {0.0f, 0.0f, 1.0f};
    light.specularColor = {0.25f, 0.5f, 0.75f};
    light.lightParam = 1;

    gModel_ActiveLightCount = 1;
    gModel_ActiveLights[0].light = &light;
    gModel_ActiveLights[0].lightState = &state;
    gModel_ActiveLights[0].useFullWeight = 1;
    gModel_ActiveLights[0].contributesToLighting = 1;

    g_Clip_PolyVertsScratch[0] = {0.0f, 0.0f, 5.0f};
    g_Clip_PolyVertsScratch[1] = {1.0f, 0.0f, 5.0f};
    g_Clip_PolyVertsScratch[2] = {0.0f, 1.0f, 5.0f};

    g_zVideo_ActiveRendererPath = 1;
    g_zModel_SoftwarePathActive = 0;
    g_zgameFogColorUpdateCount = 0;
    g_zVideo_pfnUpdateFogColor = TestZGameUpdateFogColor;
    g_zVideo_FogColorAppliedR255 = 0.0f;
    g_zVideo_FogColorAppliedG255 = 0.0f;
    g_zVideo_FogColorAppliedB255 = 0.0f;
    g_zVideo_FogColorPendingR255 = 0.0f;
    g_zVideo_FogColorPendingG255 = 0.0f;
    g_zVideo_FogColorPendingB255 = 0.0f;
    zVec3 normal{0.0f, 0.0f, 1.0f};
    std::int32_t lightFlags = 0;
    std::int32_t lightingMode = 0;
    const int hardwareResult =
        zModel_Light::SetActiveLights(&normal, 3, &lightFlags, &lightingMode, 0);
    if ((hardwareResult & 8) == 0 || (lightFlags & 9) != 9 ||
        g_Clip_PolyAttr2[0] != 1.0f || g_Clip_PolyAttr2[1] != 1.0f ||
        g_Clip_PolyAttr2[2] != 1.0f) {
        return 1;
    }
    if (g_zgameFogColorUpdateCount != 1 ||
        g_zVideo_FogColorAppliedR255 != 63.75f ||
        g_zVideo_FogColorAppliedG255 != 127.5f ||
        g_zVideo_FogColorAppliedB255 != 191.25f) {
        return 5;
    }

    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    std::memset(g_Clip_PolyAttr1, 0, sizeof(g_Clip_PolyAttr1));
    std::memset(g_Clip_PolyAttr2, 0, sizeof(g_Clip_PolyAttr2));
    light.lightParam = 0;
    light.intensityScale = 0.5f;
    light.falloff = 0.0f;
    light.isPointMode = 0;
    g_zVideo_ActiveRendererPath = 0;
    lightFlags = 0;
    lightingMode = 0;
    normal = {0.0f, 0.0f, 2.0f};
    if (zModel_Light::SetActiveLights(&normal, 3, &lightFlags, &lightingMode, 0) != 1 ||
        g_Clip_PolyAttr0[0] < 127.4f || g_Clip_PolyAttr0[0] > 127.6f ||
        g_Clip_PolyAttr0[1] < 127.4f || g_Clip_PolyAttr0[1] > 127.6f) {
        return 2;
    }

    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    std::memset(g_Clip_PolyAttr1, 0, sizeof(g_Clip_PolyAttr1));
    light.isPointMode = 1;
    light.lightParam = 0;
    light.intensityScale = 0.5f;
    light.falloff = 0.0f;
    g_zModel_CurrentPolyNormalsStorage[0] = {0.0f, 0.0f, 1.0f};
    g_zModel_CurrentPolyNormalsStorage[1] = {0.0f, 0.0f, 1.0f};
    g_zModel_CurrentPolyNormalsStorage[2] = {0.0f, 0.0f, 1.0f};
    g_zModel_CurrentPolyNormals = g_zModel_CurrentPolyNormalsStorage;
    g_zVideo_ActiveRendererPath = 1;
    lightFlags = 0;
    lightingMode = 0;
    if (zModel_Light::SetActiveLights(&normal, 3, &lightFlags, &lightingMode, 0) != 1 ||
        (lightingMode & 1) == 0 || g_Clip_PolyAttr1[0] < 0.499f ||
        g_Clip_PolyAttr1[0] > 0.501f) {
        return 3;
    }

    gModel_ActiveLightCount = 0;
    g_zModel_CurrentPolyNormals = nullptr;
    g_zModel_FogTargetColorOverride = {};
    lightFlags = 0;
    lightingMode = 7;
    return zModel_Light::SetActiveLights(&normal, 3, &lightFlags, &lightingMode, 0) == 0 &&
                   lightingMode == 0
               ? 0
               : 4;
}

extern "C" int zmodel_light_build_attr0_depth_fade_smoke(void) {
    zMath::g_zMath_CameraScratchA = {};
    zMath::g_zMath_CameraScratchA.yy = 1.0f;

    gModel_FogDistanceStart = 10.0f;
    gModel_FogDistanceEnd = 20.0f;
    gModel_FogDistanceInvRange = 0.1f;
    gModel_FogHeightHigh = 10.0f;
    gModel_FogHeightLow = 0.0f;
    gModel_FogHeightInvRange = 0.1f;

    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    g_Clip_PolyAttr0[2] = 99.0f;
    g_Clip_PolyVertsScratch[0] = {16.0f, 0.0f, 0.0f};
    g_Clip_PolyVertsScratch[1] = {32.0f, 5.0f, 0.0f};
    g_Clip_PolyVertsScratch[2] = {8.0f, 0.0f, 0.0f};

    std::int32_t hasVariation = 0;
    if (zModel_Light::BuildAttr0DepthFade(3, &hasVariation) != 1 || hasVariation != 1 ||
        g_Clip_PolyAttr0[0] < 152.9f || g_Clip_PolyAttr0[0] > 153.1f ||
        g_Clip_PolyAttr0[1] < 127.4f || g_Clip_PolyAttr0[1] > 127.6f ||
        g_Clip_PolyAttr0[2] != 99.0f) {
        return 1;
    }

    g_Clip_PolyVertsScratch[0] = {4.0f, 0.0f, 0.0f};
    hasVariation = 77;
    if (zModel_Light::BuildAttr0DepthFade(1, &hasVariation) != 0 || hasVariation != 77) {
        return 2;
    }

    g_Clip_PolyVertsScratch[0] = {16.0f, 20.0f, 0.0f};
    hasVariation = 77;
    if (zModel_Light::BuildAttr0DepthFade(1, &hasVariation) != 0 || hasVariation != 0) {
        return 3;
    }

    float fade = -1.0f;
    g_Clip_PolyVertsScratch[0] = {4.0f, 0.0f, 0.0f};
    if (zModel_Light::EvalBatchSphereFade(&fade) != 0 || fade != -1.0f) {
        return 4;
    }

    g_Clip_PolyVertsScratch[0] = {16.0f, 5.0f, 0.0f};
    if (zModel_Light::EvalBatchSphereFade(&fade) != 1 || fade < 0.299f || fade > 0.301f) {
        return 5;
    }

    g_Clip_PolyVertsScratch[0] = {16.0f, 20.0f, 0.0f};
    fade = -1.0f;
    if (zModel_Light::EvalBatchSphereFade(&fade) != 0 || fade != 0.0f) {
        return 6;
    }

    return 0;
}

extern "C" int zmodel_light_build_attr1_falloff_smoke(void) {
    zMath::g_zMath_CameraScratchA = {};
    zMath::g_zMath_CameraScratchA.yy = 1.0f;

    gModel_FogDistanceStart = 10.0f;
    gModel_FogDistanceEnd = 20.0f;
    gModel_FogDistanceInvRange = 0.1f;
    gModel_FogHeightHigh = 10.0f;
    gModel_FogHeightLow = 0.0f;
    gModel_FogHeightInvRange = 0.1f;
    gModel_FogColorRgb01 = {0.25f, 0.5f, 1.0f};
    g_zVideo_FogColorAppliedR255 = 0.0f;
    g_zVideo_FogColorAppliedG255 = 0.0f;
    g_zVideo_FogColorAppliedB255 = 0.0f;
    g_zgameFogColorUpdateCount = 0;
    g_zVideo_pfnUpdateFogColor = TestZGameUpdateFogColor;

    std::memset(g_Clip_PolyAttr2, 0, sizeof(g_Clip_PolyAttr2));
    g_Clip_PolyVertsScratch[0] = {16.0f, 0.0f, 0.0f};
    g_Clip_PolyVertsScratch[1] = {32.0f, 5.0f, 0.0f};
    g_Clip_PolyVertsScratch[2] = {8.0f, 0.0f, 0.0f};

    std::int32_t lightingFlags = 0;
    if (zModel_Light::BuildAttr1Falloff(3, &lightingFlags) != 1 ||
        lightingFlags != 2 || g_Clip_PolyAttr2[0] < 0.599f ||
        g_Clip_PolyAttr2[0] > 0.601f || g_Clip_PolyAttr2[1] < 0.499f ||
        g_Clip_PolyAttr2[1] > 0.501f || g_Clip_PolyAttr2[2] != 0.0f ||
        g_zVideo_FogColorPendingR255 != 63.75f ||
        g_zVideo_FogColorPendingG255 != 127.5f ||
        g_zVideo_FogColorPendingB255 != 255.0f ||
        g_zgameFogColorUpdateCount != 1) {
        return 1;
    }

    g_Clip_PolyVertsScratch[0] = {4.0f, 0.0f, 0.0f};
    lightingFlags = 2;
    if (zModel_Light::BuildAttr1Falloff(1, &lightingFlags) != 0 || lightingFlags != 0) {
        return 2;
    }

    g_Clip_PolyVertsScratch[0] = {16.0f, 20.0f, 0.0f};
    lightingFlags = 2;
    if (zModel_Light::BuildAttr1Falloff(1, &lightingFlags) != 0 || lightingFlags != 0 ||
        g_Clip_PolyAttr2[0] != 0.0f) {
        return 3;
    }

    g_Clip_PolyVertsScratch[0] = {20.0f, 0.0f, 0.0f};
    g_Clip_PolyVertsScratch[1] = {20.0f, 0.0f, 0.0f};
    lightingFlags = 2;
    if (zModel_Light::BuildAttr1Falloff(2, &lightingFlags) != 1 || lightingFlags != 2 ||
        g_Clip_PolyAttr2[0] != 1.0f || g_Clip_PolyAttr2[1] != 1.0f) {
        return 4;
    }

    return 0;
}

extern "C" int zclass_world_apply_pending_fog_settings_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    g_zVideo_ActiveRendererPath = 1;
    zRndr::g_fogColorParams = {};
    gModel_FogDistanceInvRange = 0.0f;
    gModel_FogHeightInvRange = 0.0f;

    zClass_WorldDataPartial worldData{};
    zClass_NodePartial worldNode{};
    worldNode.classData = &worldData;
    if (zClass_World::SetPendingFogState(&worldNode, 1) != 0 ||
        zClass_World::SetPendingFogColorRgb01(&worldNode, 1.2f, -0.5f, 0.5f) != 0 ||
        zClass_World::SetPendingFogRange(&worldNode, 5.0f, 20.0f) != 0 ||
        zClass_World::SetPendingFogAltitudeRange(&worldNode, 2.0f, 10.0f) != 0 ||
        zClass_World::SetPendingFogDensity(&worldNode, 0.75f) != 0 || worldData.flags != 0x2f) {
        return 1;
    }

    float fogStart = 0.0f;
    float fogEnd = 0.0f;
    float fogDensity = 0.0f;
    int fogState = 0;
    float fogRed = 0.0f;
    float fogGreen = 0.0f;
    float fogBlue = 0.0f;
    float fogAltitudeLow = 0.0f;
    float fogAltitudeHigh = 0.0f;
    if (zClass_World::GetPendingFogDensity(&worldNode, &fogDensity) != 0 ||
        fogDensity != 0.75f ||
        zClass_World::GetPendingFogState(&worldNode, &fogState) != 0 || fogState != 1 ||
        zClass_World::GetPendingFogColorRgb01(&worldNode, &fogRed, &fogGreen, &fogBlue) != 0 ||
        fogRed != 1.2f || fogGreen != -0.5f || fogBlue != 0.5f ||
        zClass_World::GetPendingFogAltitudeRange(&worldNode, &fogAltitudeLow,
                                                 &fogAltitudeHigh) != 0 ||
        fogAltitudeLow != 2.0f || fogAltitudeHigh != 10.0f) {
        return 6;
    }

    if (zClass_World::GetPendingFogRange(&worldNode, &fogStart, &fogEnd) != 0 ||
        fogStart != 5.0f || fogEnd != 20.0f) {
        return 5;
    }

    std::int32_t childClassData = 0;
    zClass_NodePartial areaChild{};
    areaChild.classData = &childClassData;
    areaChild.flags = 0x100;
    areaChild.cachedBounds[1] = -4.0f;
    areaChild.cachedBounds[4] = 8.0f;
    zClass_NodePartial *areaChildren[] = {&areaChild};
    zWorldAreaPartial pendingArea{};
    pendingArea.areaFlags = 1;
    pendingArea.bbox[0] = 0.0f;
    pendingArea.bbox[2] = 0.0f;
    pendingArea.bbox[3] = 10.0f;
    pendingArea.bbox[5] = 10.0f;
    pendingArea.childCount = 1;
    pendingArea.childList = areaChildren;
    zWorldAreaPartial *pendingAreas[] = {&pendingArea};
    worldData.pendingAreaUpdateCount = 1;
    worldData.pendingAreaUpdates = pendingAreas;

    if (zClass_World::ApplyPendingFogSettings(&worldNode) != 0 || worldData.flags != 0 ||
        worldData.pendingAreaUpdateCount != 0 || (pendingArea.areaFlags & 1) != 0 ||
        (pendingArea.areaFlags & 0x100) == 0) {
        return 1;
    }

    if (gModel_FogEnabled != 1 || gModel_FogLinearModeEnabled != 1 ||
        gModel_FogDistanceStart != 5.0f || gModel_FogDistanceEnd != 20.0f ||
        gModel_FogDistanceInvRange < 0.066f || gModel_FogDistanceInvRange > 0.067f ||
        gModel_FogHeightHigh != 10.0f || gModel_FogHeightLow != 2.0f ||
        gModel_FogHeightInvRange != 0.125f || gModel_FogDensity != 0.75f) {
        return 2;
    }

    if (worldData.ambientColor.red != 1.0f || worldData.ambientColor.green != 0.0f ||
        worldData.ambientColor.blue != 0.5f || gModel_FogColorRgb01.red != 1.0f ||
        gModel_FogColorRgb01.green != 0.0f || gModel_FogColorRgb01.blue != 0.5f ||
        zRndr::g_fogColorParams.colorRgb01[0] != 1.0f ||
        zRndr::g_fogColorParams.colorRgb01[1] != 0.0f ||
        zRndr::g_fogColorParams.colorRgb01[2] != 0.5f) {
        return 3;
    }

    return pendingArea.bbox[1] == -4.0f && pendingArea.bbox[4] == 8.0f &&
                   pendingArea.bboxCenter.y == 2.0f && pendingArea.bboxRadius > 0.0f
               ? 0
               : 4;
}

extern "C" int zclass_world_settings_section_callbacks_smoke(void) {
    reset_zclass_type_lists_for_test();

    zClass_WorldDataPartial worldData{};
    zClass_NodePartial worldNode{};
    std::strcpy(worldNode.name, "arena");
    worldNode.classData = &worldData;
    if (zClass_TypeList::Insert(13, &worldNode) != 0) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    zClass_WorldSettingsSectionRecord settings{};
    settings.fogState = 1;
    settings.fogColorRgb01 = {0.25f, 0.5f, 0.75f};
    settings.fogRangeNear = 3.0f;
    settings.fogRangeFar = 33.0f;
    settings.fogAltitudeHigh = 12.0f;
    settings.fogAltitudeLow = -2.0f;
    settings.fogDensity = 0.625f;

    zClass_World::ReadSettingsSection(nullptr, "missing", &settings, sizeof(settings), nullptr);
    if (worldData.flags != 0) {
        free_zclass_type_lists_for_test();
        return 2;
    }

    zClass_World::ReadSettingsSection(nullptr, "arena", &settings, sizeof(settings), nullptr);
    if (worldData.flags != 0x2f || worldData.fogState != 1 ||
        worldData.ambientColor.red != 0.25f || worldData.ambientColor.green != 0.5f ||
        worldData.ambientColor.blue != 0.75f || worldData.fogDistanceStart != 3.0f ||
        worldData.fogDistanceEnd != 33.0f || worldData.fogHeightHigh != 12.0f ||
        worldData.fogHeightLow != -2.0f || worldData.fogDensity != 0.625f) {
        free_zclass_type_lists_for_test();
        return 3;
    }

    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "zws", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        free_zclass_type_lists_for_test();
        return 4;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "WorldSettings";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    const int result = zClass_World::WriteSettingsSection(&callbackCtx, nullptr);

    SetFilePointer(file, 0, nullptr, FILE_BEGIN);
    zClass_WorldSettingsSectionRecord readBack{};
    DWORD read = 0;
    ReadFile(file, &readBack, sizeof(readBack), &read, nullptr);

    const bool ok = result == 1 && read == sizeof(readBack) &&
                    manager.indexArchive.recordCount == 1 &&
                    manager.indexArchive.records != nullptr &&
                    manager.indexArchive.records[0].fileSize == sizeof(readBack) &&
                    std::strcmp(manager.indexArchive.records[0].name,
                                "WorldSettings/arena") == 0 &&
                    readBack.fogState == 1 && readBack.fogColorRgb01.red == 0.25f &&
                    readBack.fogColorRgb01.green == 0.5f &&
                    readBack.fogColorRgb01.blue == 0.75f &&
                    readBack.fogRangeNear == 3.0f && readBack.fogRangeFar == 33.0f &&
                    readBack.fogAltitudeHigh == 12.0f &&
                    readBack.fogAltitudeLow == -2.0f && readBack.fogDensity == 0.625f;

    std::free(manager.indexArchive.records);
    manager.indexArchive.records = nullptr;
    CloseHandle(file);
    free_zclass_type_lists_for_test();
    return ok ? 0 : 5;
}

extern "C" int zclass_shutdown_core_smoke(void) {
    g_zClass_NodeArray =
        static_cast<zClass_NodeFreeListSlot *>(std::calloc(2, sizeof(zClass_NodeFreeListSlot)));
    g_zClass_NodeArraySize = 2;
    g_zClass_ActiveNodeCount = 1;
    g_zClass_NodeFreeHeadIndex = 1;
    g_zClass_IsInitialized = 1;
    std::strcpy(g_zClass_CurrentZbdPath, "mission.zbd");

    if (zClass::Shutdown() != 0) {
        return 1;
    }

    return g_zClass_NodeArray == nullptr && g_zClass_NodeArraySize == 0 &&
                   g_zClass_ActiveNodeCount == 0 && g_zClass_NodeFreeHeadIndex == -1 &&
                   g_zClass_IsInitialized == 0 && g_zClass_CurrentZbdPath[0] == '\0'
               ? 0
               : 2;
}

extern "C" int zclass_gwnode_update_smoke(void) {
    std::int32_t classData = 0;
    zClass_NodeFreeListSlot parentSlot{};
    zClass_NodePartial child{};
    child.classData = &classData;
    child.flags = 0x100;
    child.cachedBounds[0] = -1.0f;
    child.cachedBounds[1] = 2.0f;
    child.cachedBounds[2] = -3.0f;
    child.cachedBounds[3] = 4.0f;
    child.cachedBounds[4] = 5.0f;
    child.cachedBounds[5] = 6.0f;
    zClass_NodePartial *children[] = {&child};
    parentSlot.node.classId = 6;
    parentSlot.node.classData = &classData;
    parentSlot.node.boundsFlags = 0x02;
    parentSlot.node.flags = 0x02;
    parentSlot.node.listCountB = 1;
    parentSlot.node.listB = children;
    if (zClass_Class::gwNodeUpdate(&parentSlot.node) != 0 || (parentSlot.node.flags & 0x02) != 0 ||
        (parentSlot.node.flags & 0x100) == 0 || parentSlot.node.boundsFlags != 0x04 ||
        parentSlot.node.cachedBounds[0] != -1.0f || parentSlot.node.cachedBounds[5] != 6.0f) {
        return 1;
    }

    zClass_Object3DDataPartial objectData{};
    zClass_NodeFreeListSlot objectSlot{};
    objectSlot.node.classId = 5;
    objectSlot.node.classData = &objectData;
    objectSlot.node.flags = 0x202;
    objectData.flags = 0x11;
    zBBox3f *objectBounds = reinterpret_cast<zBBox3f *>(objectSlot.unknown_8c);
    *objectBounds = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    if (zClass_Class::gwNodeUpdate(&objectSlot.node) != 0 || (objectData.flags & 0x01) != 0 ||
        (objectSlot.node.flags & 0x02) != 0 || (objectSlot.node.flags & 0x100) == 0 ||
        objectSlot.node.cachedBounds[0] != 1.0f || objectSlot.node.cachedBounds[5] != 6.0f) {
        return 2;
    }

    zClass_WorldDataPartial worldData{};
    worldData.flags = 0x08;
    worldData.fogDensity = 0.375f;
    zClass_NodePartial worldNode{};
    worldNode.classId = 2;
    worldNode.flags = 0x02;
    worldNode.classData = &worldData;
    if (zClass_Class::gwNodeUpdate(&worldNode) != 0 || worldData.flags != 0 ||
        (worldNode.flags & 0x02) != 0 || gModel_FogDensity != 0.375f) {
        return 3;
    }

    zClass_NodePartial invalidNode{};
    invalidNode.classId = 3;
    return zClass_Class::gwNodeUpdate(&invalidNode) == 3 ? 0 : 4;
}

extern "C" int zclass_gwnode_update_tree_smoke(void) {
    reset_zclass_type_lists_for_test();
    g_zClass_DeferredProcessingEnabled = 0;

    zClass_NodePartial root{};
    zClass_NodePartial activeChild{};
    zClass_NodePartial inactiveChild{};
    zClass_NodePartial *children[] = {&activeChild, &inactiveChild};

    root.classId = 6;
    root.flags = 0x03;
    root.listCountB = 2;
    root.listB = children;
    activeChild.classId = 6;
    activeChild.flags = 0x03;
    inactiveChild.classId = 6;
    inactiveChild.flags = 0x02;

    zClass_TypeList::Insert(7, &root);
    zClass_TypeList::Insert(7, &activeChild);

    gwNode::UpdateTree(&root);
    int result = 0;
    if ((root.flags & 0x02) != 0 || (activeChild.flags & 0x02) != 0 ||
        (inactiveChild.flags & 0x02) == 0 ||
        !zclass_bucket_has_pending_node_for_test(7, &root) ||
        !zclass_bucket_has_pending_node_for_test(7, &activeChild) ||
        zClass_TypeList::PendingRemovalDirty(7) == 0) {
        result = 1;
    }

    free_zclass_type_lists_for_test();

    reset_zclass_type_lists_for_test();
    g_zClass_DeferredProcessingEnabled = 0;

    zClass_NodePartial leaf{};
    zClass_NodePartial parent{};
    zClass_NodePartial world{};
    zClass_NodePartial *parents[] = {&parent, &world};

    leaf.classId = 6;
    leaf.flags = 0x03;
    leaf.listCountA = 2;
    leaf.listA = parents;
    parent.classId = 6;
    parent.flags = 0x03;
    world.classId = 2;
    world.flags = 0x03;

    zClass_TypeList::Insert(7, &leaf);
    zClass_TypeList::Insert(7, &parent);
    zClass_TypeList::Insert(7, &world);

    gwNode::UpdateTree(&leaf);
    if (result == 0 &&
        ((leaf.flags & 0x02) != 0 || (parent.flags & 0x02) != 0 ||
         (world.flags & 0x02) == 0 || !zclass_bucket_has_pending_node_for_test(7, &leaf) ||
         !zclass_bucket_has_pending_node_for_test(7, &parent) ||
         zclass_bucket_has_pending_node_for_test(7, &world))) {
        result = 2;
    }

    free_zclass_type_lists_for_test();
    g_zClass_DeferredProcessingEnabled = 1;
    return result;
}

extern "C" int zclass_typelist_update_bucket_smoke(void) {
    reset_zclass_type_lists_for_test();
    g_zClass_DeferredProcessingEnabled = 1;
    g_zclassUpdateBucketCallbackCount = 0;
    g_zclassUpdateBucketLastNode = nullptr;
    g_zclassUpdateBucketDeferredDuringCallback = -1;

    zClass_NodePartial activeNode{};
    zClass_NodePartial nullCallbackNode{};
    zClass_NodePartial pendingNode{};
    zClass_NodePartial inactiveNode{};
    activeNode.flags = 4;
    pendingNode.flags = 4;
    inactiveNode.flags = 0;
    activeNode.actionCallback = reinterpret_cast<void *>(&TestZClassUpdateBucketCallback);
    pendingNode.actionCallback = reinterpret_cast<void *>(&TestZClassUpdateBucketCallback);
    inactiveNode.actionCallback = reinterpret_cast<void *>(&TestZClassUpdateBucketCallback);

    zClass_TypeListLink activeLink{&activeNode, nullptr, nullptr, 0};
    zClass_TypeListLink nullCallbackLink{&nullCallbackNode, nullptr, nullptr, 0};
    zClass_TypeListLink pendingLink{&pendingNode, nullptr, nullptr, 1};
    zClass_TypeListLink inactiveLink{&inactiveNode, nullptr, nullptr, 0};
    activeLink.next = &nullCallbackLink;
    nullCallbackLink.prev = &activeLink;
    nullCallbackLink.next = &pendingLink;
    pendingLink.prev = &nullCallbackLink;
    pendingLink.next = &inactiveLink;
    inactiveLink.prev = &pendingLink;

    zClass_TypeList::UpdateBucket(&activeLink);
    const bool updateOk =
        g_zclassUpdateBucketCallbackCount == 1 &&
        g_zclassUpdateBucketLastNode == &activeNode &&
        g_zclassUpdateBucketDeferredDuringCallback == 0 &&
        nullCallbackLink.pendingRemove == 1 && pendingLink.pendingRemove == 1 &&
        inactiveLink.pendingRemove == 0 && g_zClass_DeferredProcessingEnabled == 1;

    g_zClass_DeferredProcessingEnabled = 0;
    zClass_TypeList::UpdateBucket(nullptr);
    const bool nullBucketOk = g_zClass_DeferredProcessingEnabled == 0;

    reset_zclass_type_lists_for_test();
    return updateOk && nullBucketOk ? 0 : 1;
}

extern "C" int zclass_typelist_update_all_buckets_smoke(void) {
    reset_zclass_type_lists_for_test();
    g_zClass_DeferredProcessingEnabled = 1;
    g_zclassUpdateBucketCallbackCount = 0;
    g_zclassUpdateBucketLastNode = nullptr;
    g_zclassUpdateBucketDeferredDuringCallback = -1;

    zClass_NodePartial firstNode{};
    zClass_NodePartial secondNode{};
    firstNode.flags = 4;
    secondNode.flags = 4;
    firstNode.actionCallback = reinterpret_cast<void *>(&TestZClassUpdateBucketCallback);
    secondNode.actionCallback = reinterpret_cast<void *>(&TestZClassUpdateBucketCallback);

    zClass_TypeListLink firstLink{&firstNode, nullptr, nullptr, 0};
    zClass_TypeListLink secondLink{&secondNode, nullptr, nullptr, 0};
    zClass_TypeList::Head(0) = &firstLink;
    zClass_TypeList::Tail(0) = &firstLink;
    zClass_TypeList::Head(1) = &secondLink;
    zClass_TypeList::Tail(1) = &secondLink;

    zClass_TypeList::UpdateAllBuckets();
    const bool ok = g_zclassUpdateBucketCallbackCount == 2 &&
                    g_zclassUpdateBucketLastNode == &secondNode &&
                    g_zclassUpdateBucketDeferredDuringCallback == 0 &&
                    firstLink.pendingRemove == 0 && secondLink.pendingRemove == 0 &&
                    g_zClass_DeferredProcessingEnabled == 1;

    reset_zclass_type_lists_for_test();
    return ok ? 0 : 1;
}

extern "C" int zgame_options_find_option_smoke() {
    zOptionEntryPartial first{};
    zOptionEntryPartial second{};
    char firstName[] = "First";
    char secondName[] = "GfxFlags_HW";
    first.name = firstName;
    first.next = &second;
    second.name = secondName;
    second.payloadOrBuffer = 0x10000;

    g_zGame_Options_OptionListHead = &first;
    if (zGame::Options_FindOption("GfxFlags_HW") != &second) {
        return 1;
    }
    if (zGame::Options_FindOption("Missing") != nullptr) {
        return 2;
    }

    g_zGame_Options_OptionListHead = nullptr;
    return zGame::Options_FindOption("First") == nullptr ? 0 : 3;
}

extern "C" int zgame_options_runtime_config_copy_default_smoke() {
    std::memset(&g_zGame_Options_RuntimeConfigDefaults, 0, sizeof(g_zGame_Options_RuntimeConfigDefaults));
    std::strcpy(g_zGame_Options_RuntimeConfigDefaults.cpuVendor, "CopyDefault");
    g_zGame_Options_RuntimeConfigDefaults.cpuClass = 6;
    g_zGame_Options_RuntimeConfigDefaults.cpuMhz = 450;
    g_zGame_Options_RuntimeConfigDefaults.defaultFlags = 0x44;
    g_zGame_Options_RuntimeConfigDefaults.systemRamKb = 65536;
    g_zGame_Options_RuntimeConfigDefaults.soundHardwareMemKb = 8192;

    zGame_OptionsRuntimeConfig target{};
    zGame_OptionsRuntimeConfig *const result = target.CopyDefault();
    return result == &target && std::strcmp(target.cpuVendor, "CopyDefault") == 0 &&
                   target.cpuClass == 6 && target.cpuMhz == 450 && target.defaultFlags == 0x44 &&
                   target.systemRamKb == 65536 && target.soundHardwareMemKb == 8192
               ? 0
               : 1;
}

extern "C" int zopt_select_profile_value_for_system_smoke() {
    zReader::Node defaultRule[3] = {};
    defaultRule[0].type = zReader::ZRDR_NODE_INT;
    defaultRule[0].value.i32 = 3;
    defaultRule[1].type = zReader::ZRDR_NODE_STRING;
    defaultRule[1].value.str = const_cast<char *>("DEFAULT");
    defaultRule[2].type = zReader::ZRDR_NODE_INT;
    defaultRule[2].value.i32 = 9;

    zReader::Node defaultRules[2] = {};
    defaultRules[0].type = zReader::ZRDR_NODE_INT;
    defaultRules[0].value.i32 = 2;
    defaultRules[1].type = zReader::ZRDR_NODE_ARRAY;
    defaultRules[1].value.nodes = defaultRule;

    zReader::Node cpuCondition[4] = {};
    cpuCondition[0].type = zReader::ZRDR_NODE_INT;
    cpuCondition[0].value.i32 = 4;
    cpuCondition[1].type = zReader::ZRDR_NODE_STRING;
    cpuCondition[1].value.str = const_cast<char *>("CPU_CLASS");
    cpuCondition[2].type = zReader::ZRDR_NODE_STRING;
    cpuCondition[2].value.str = const_cast<char *>(">=");
    cpuCondition[3].type = zReader::ZRDR_NODE_INT;
    cpuCondition[3].value.i32 = 5;

    zReader::Node metricRule[3] = {};
    metricRule[0].type = zReader::ZRDR_NODE_INT;
    metricRule[0].value.i32 = 3;
    metricRule[1].type = zReader::ZRDR_NODE_ARRAY;
    metricRule[1].value.nodes = cpuCondition;
    metricRule[2].type = zReader::ZRDR_NODE_STRING;
    metricRule[2].value.str = const_cast<char *>("LOW");

    zReader::Node metricRules[2] = {};
    metricRules[0].type = zReader::ZRDR_NODE_INT;
    metricRules[0].value.i32 = 2;
    metricRules[1].type = zReader::ZRDR_NODE_ARRAY;
    metricRules[1].value.nodes = metricRule;

    zReader::Node rootChildren[5] = {};
    rootChildren[0].type = zReader::ZRDR_NODE_INT;
    rootChildren[0].value.i32 = 5;
    rootChildren[1].type = zReader::ZRDR_NODE_STRING;
    rootChildren[1].value.str = const_cast<char *>("EffectsLevel_SW");
    rootChildren[2].type = zReader::ZRDR_NODE_ARRAY;
    rootChildren[2].value.nodes = defaultRules;
    rootChildren[3].type = zReader::ZRDR_NODE_STRING;
    rootChildren[3].value.str = const_cast<char *>("ObjectLOD_SW");
    rootChildren[4].type = zReader::ZRDR_NODE_ARRAY;
    rootChildren[4].value.nodes = metricRules;

    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootChildren;
    g_zGame_Options_RuntimeConfig.cpuClass = 6;

    if (zOpt::SelectProfileValueForSystem(nullptr, "EffectsLevel_SW", 4) != 4 ||
        zOpt::SelectProfileValueForSystem(&root, "Missing", 5) != 5 ||
        zOpt::SelectProfileValueForSystem(&root, "EffectsLevel_SW", 1) != 9 ||
        zOpt::SelectProfileValueForSystem(&root, "ObjectLOD_SW", 0) != 2) {
        return 1;
    }

    g_zGame_Options_RuntimeConfig.cpuClass = 4;
    return zOpt::SelectProfileValueForSystem(&root, "ObjectLOD_SW", 7) == 7 ? 0 : 2;
}

extern "C" int zgame_options_load_from_registry_missing_smoke() {
    char *const oldRoot = g_zGame_Options_RegKeyRoot;
    char *const oldUser = g_zGame_Options_RegKeyCurrentUser;
    char *const oldGame = g_zGame_Options_RegKeyGame;
    g_zGame_Options_RegKeyRoot = const_cast<char *>("RecoilMissingRootForSmoke");
    g_zGame_Options_RegKeyCurrentUser = const_cast<char *>("MissingUser");
    g_zGame_Options_RegKeyGame = const_cast<char *>("MissingGame");

    const int result = zGame::Options_LoadFromRegistry();

    g_zGame_Options_RegKeyRoot = oldRoot;
    g_zGame_Options_RegKeyCurrentUser = oldUser;
    g_zGame_Options_RegKeyGame = oldGame;
    return result == 0 ? 0 : 1;
}

extern "C" int zgame_options_save_game_options_smoke() {
    const char *const rootName = "RecoilSmokeSave";
    const char *const userName = "CurrentUser";
    const char *const gameName = "Game";
    const char *const leafPath = "SOFTWARE\\RecoilSmokeSave\\CurrentUser\\Game";
    const char *const userPath = "SOFTWARE\\RecoilSmokeSave\\CurrentUser";
    const char *const rootPath = "SOFTWARE\\RecoilSmokeSave";
    RegDeleteKeyA(HKEY_CURRENT_USER, leafPath);
    RegDeleteKeyA(HKEY_CURRENT_USER, userPath);
    RegDeleteKeyA(HKEY_CURRENT_USER, rootPath);
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, leafPath);
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, userPath);
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, rootPath);

    char *const oldRoot = g_zGame_Options_RegKeyRoot;
    char *const oldUser = g_zGame_Options_RegKeyCurrentUser;
    char *const oldGame = g_zGame_Options_RegKeyGame;
    zOptionEntryPartial *const oldHead = g_zGame_Options_OptionListHead;
    int *const oldNetwork = ZOPT_NETWORK_ENABLED;
    int *const oldModem = g_zOpt_NetworkModemOption;

    g_zGame_Options_RegKeyRoot = const_cast<char *>(rootName);
    g_zGame_Options_RegKeyCurrentUser = const_cast<char *>(userName);
    g_zGame_Options_RegKeyGame = const_cast<char *>(gameName);

    zOptionEntryPartial option = {};
    char optionName[] = "NetworkEnabled";
    option.name = optionName;
    option.storageType = 0;
    option.dataSize = 4;
    option.registryScope = 1;
    g_zGame_Options_OptionListHead = &option;

    std::int32_t networkEnabled = 1;
    std::int32_t networkModem = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zOpt_NetworkModemOption = &networkModem;

    g_zInput_BindGroupInfoList = {};
    zInput::BindGroupList_AddGroup("Smoke");
    const bool groupCreated = zInput::BindGroupList_GetCount() == 1;

    const int saveResult = zGame::Options_SaveGameOptions();
    const bool ok = groupCreated && (saveResult == 0 || saveResult == 1) &&
                    zInput::BindGroupList_GetCount() == 0 && networkEnabled == 0 &&
                    networkModem == 0;

    ::operator delete(g_zInput_BindGroupInfoList.begin);
    g_zInput_BindGroupInfoList = {};
    ZOPT_NETWORK_ENABLED = oldNetwork;
    g_zOpt_NetworkModemOption = oldModem;
    g_zGame_Options_OptionListHead = oldHead;
    g_zGame_Options_RegKeyRoot = oldRoot;
    g_zGame_Options_RegKeyCurrentUser = oldUser;
    g_zGame_Options_RegKeyGame = oldGame;
    RegDeleteKeyA(HKEY_CURRENT_USER, leafPath);
    RegDeleteKeyA(HKEY_CURRENT_USER, userPath);
    RegDeleteKeyA(HKEY_CURRENT_USER, rootPath);
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, leafPath);
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, userPath);
    RegDeleteKeyA(HKEY_LOCAL_MACHINE, rootPath);
    return ok ? 0 : 1;
}

extern "C" int zgame_options_shutdown_registry_context_smoke() {
    zGame::Options_ShutdownRegistryContext();

    zOptionEntryPartial *const first =
        static_cast<zOptionEntryPartial *>(std::calloc(1, sizeof(zOptionEntryPartial)));
    zOptionEntryPartial *const second =
        static_cast<zOptionEntryPartial *>(std::calloc(1, sizeof(zOptionEntryPartial)));
    if (first == nullptr || second == nullptr) {
        std::free(first);
        std::free(second);
        return 1;
    }

    first->name = static_cast<char *>(std::malloc(6));
    second->name = static_cast<char *>(std::malloc(7));
    void *const payload = std::malloc(16);
    g_zGame_Options_RegKeyRoot = static_cast<char *>(std::malloc(5));
    g_zGame_Options_RegKeyCurrentUser = static_cast<char *>(std::malloc(5));
    g_zGame_Options_RegKeyGame = static_cast<char *>(std::malloc(5));
    if (first->name == nullptr || second->name == nullptr || payload == nullptr ||
        g_zGame_Options_RegKeyRoot == nullptr || g_zGame_Options_RegKeyCurrentUser == nullptr ||
        g_zGame_Options_RegKeyGame == nullptr) {
        std::free(first->name);
        std::free(second->name);
        std::free(payload);
        std::free(first);
        std::free(second);
        std::free(g_zGame_Options_RegKeyRoot);
        std::free(g_zGame_Options_RegKeyCurrentUser);
        std::free(g_zGame_Options_RegKeyGame);
        g_zGame_Options_RegKeyRoot = nullptr;
        g_zGame_Options_RegKeyCurrentUser = nullptr;
        g_zGame_Options_RegKeyGame = nullptr;
        return 2;
    }

    std::strcpy(first->name, "First");
    std::strcpy(second->name, "Second");
    std::strcpy(g_zGame_Options_RegKeyRoot, "Root");
    std::strcpy(g_zGame_Options_RegKeyCurrentUser, "User");
    std::strcpy(g_zGame_Options_RegKeyGame, "Game");

    first->storageType = 0;
    first->next = second;
    second->storageType = 5;
    second->payloadOrBuffer = static_cast<int>(reinterpret_cast<std::uintptr_t>(payload));
    second->next = nullptr;
    g_zGame_Options_OptionListHead = first;
    g_zGame_Options_RegContextInitialized = 1;

    zGame::Options_ShutdownRegistryContext();

    return g_zGame_Options_OptionListHead == nullptr && g_zGame_Options_RegKeyRoot == nullptr &&
                   g_zGame_Options_RegKeyCurrentUser == nullptr &&
                   g_zGame_Options_RegKeyGame == nullptr &&
                   g_zGame_Options_RegContextInitialized == 0
               ? 0
               : 3;
}

extern "C" int zgame_options_load_game_options_minimal_smoke() {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zgo", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 1);
    WriteU32(file, zReader::ZRDR_NODE_INT);
    WriteU32(file, 1);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "detail.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;
    g_zArchive_MountedList = &list;

    zGame::Options_InitRegistryContext("RecoilSmoke", "CurrentUser", "Game");
    zInput::BindMapSystem_Init(0x30);

    const int result = zGame::Options_LoadGameOptions();
    const bool ok = result == 1 && ZOPT_VIDEO_ACCELERATION != nullptr &&
                    zVid::GetAccelerationOption() == 1 && ZOPT_VIDEO_STRIDE != nullptr &&
                    *ZOPT_VIDEO_STRIDE == 1 && zInput::BindGroupList_GetCount() == 5 &&
                    ZOPT_NETWORK_ENABLED != nullptr && *ZOPT_NETWORK_ENABLED == 0;

    zInput::BindMapSystem_Shutdown();
    g_zInput_BindMap_Current = nullptr;
    zGame::Options_ShutdownRegistryContext();
    g_zArchive_MountedList = nullptr;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 3;
}

extern "C" int zopt_eval_int_compare_op_smoke() {
    if (zOpt::EvalIntCompareOp("==", 7, 7) != 1 || zOpt::EvalIntCompareOp("==", 7, 8) != 0 ||
        zOpt::EvalIntCompareOp("<", -2, 3) != 1 || zOpt::EvalIntCompareOp("<", 3, -2) != 0 ||
        zOpt::EvalIntCompareOp(">", 3, -2) != 1 || zOpt::EvalIntCompareOp(">", -2, 3) != 0 ||
        zOpt::EvalIntCompareOp("<=", 3, 3) != 1 || zOpt::EvalIntCompareOp("<=", 4, 3) != 0 ||
        zOpt::EvalIntCompareOp(">=", 3, 3) != 1 || zOpt::EvalIntCompareOp(">=", 2, 3) != 0 ||
        zOpt::EvalIntCompareOp("!=", 2, 3) != 1 || zOpt::EvalIntCompareOp("!=", 3, 3) != 0) {
        return 1;
    }

    if (zOpt::EvalIntCompareOp("~=", 100, 101) != 1 ||
        zOpt::EvalIntCompareOp("~=", 100, 102) != 0 || zOpt::EvalIntCompareOp("~=", 0, 0) != 0 ||
        zOpt::EvalIntCompareOp("~=", -100, -101) != 0) {
        return 2;
    }

    return zOpt::EvalIntCompareOp("??", 1, 1) == 0 ? 0 : 3;
}

extern "C" int zopt_fullscreen_accessors_smoke() {
    std::int32_t fullscreen = 0;
    std::int32_t hudSw = 1;
    std::int32_t hudHw = 0;
    std::int32_t hudTypeSw = 0;
    std::int32_t hudTypeHw = 0;
    std::int32_t gameControl = 0;
    std::int32_t difficulty = 0;
    std::int32_t effectsSw = 0;
    std::int32_t effectsHw = 0;
    std::int32_t objectLodSw = 0;
    std::int32_t objectLodHw = 0;
    std::int32_t muteSound = 0;
    float soundVolume = 0.0f;
    float globalVolume = 0.0f;
    std::int32_t soundLod = 0;
    std::int32_t textureMemorySw = 0;
    std::int32_t textureMemoryHw = 0;
    std::int32_t gfxFlagsSw = 0;
    std::int32_t gfxFlagsHw = 0;
    char playerNameBuffer[8] = {};
    zOptionEntryPartial playerNameOption = {};
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_HUD_SW = &hudSw;
    ZOPT_HUD_HW = &hudHw;
    ZOPT_HUD_TYPE_SW = &hudTypeSw;
    ZOPT_HUD_TYPE_HW = &hudTypeHw;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControl;
    g_zOpt_GameDifficultyOption = &difficulty;
    ZOPT_EFFECTS_LEVEL_SW = &effectsSw;
    ZOPT_EFFECTS_LEVEL_HW = &effectsHw;
    ZOPT_OBJECT_LOD_SW = &objectLodSw;
    ZOPT_OBJECT_LOD_HW = &objectLodHw;
    ZOPT_MUTE_SOUND = &muteSound;
    ZOPT_SOUND_VOLUME = &soundVolume;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    ZOPT_SOUND_LOD = &soundLod;
    ZOPT_TEXTURE_MEMORY_SW = &textureMemorySw;
    ZOPT_TEXTURE_MEMORY_HW = &textureMemoryHw;
    ZOPT_GFX_FLAGS_SW = &gfxFlagsSw;
    ZOPT_GFX_FLAGS_HW = &gfxFlagsHw;
    playerNameOption.payloadOrBuffer = reinterpret_cast<std::int32_t>(playerNameBuffer);
    playerNameOption.dataSize = sizeof(playerNameBuffer);
    ZOPT_PLAYER_NAME = &playerNameOption;

    zOpt::SetFullscreenOption(1);
    if (fullscreen != 1 || zOpt::GetFullscreenOption() != 1) {
        return 1;
    }

    zOpt::SetFullscreenOption(0);
    if (fullscreen != 0 || zOpt::GetFullscreenOption() != 0) {
        return 2;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetHudVisibilityOption(0);
    if (hudSw != 0 || hudHw != 0 || zOpt::GetHudVisibilityOption() != 0) {
        return 3;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetHudVisibilityOption(1);
    if (hudSw != 0 || hudHw != 1 || zOpt::GetHudVisibilityOption() != 1) {
        return 4;
    }

    g_zOpt_HwMode = 0;
    hudTypeSw = 2;
    if (zOpt::GetHudTypeForCurrentHwMode() != 2) {
        return 41;
    }

    g_zOpt_HwMode = 1;
    hudTypeHw = 3;
    if (zOpt::GetHudTypeForCurrentHwMode() != 3) {
        return 42;
    }

    zOpt::SetGameControlOptions(0);
    zOpt::SetThrottleMode(1);
    zOpt::SetSteeringMode(1);
    zOpt::SetCursorMode(1);
    if (gameControl != 7 || zOpt::GetThrottleMode() != 1 || zOpt::GetSteeringMode() != 1 ||
        zOpt::GetCursorMode() != 1 || zOpt::GetCameraModePlayerState() != 3) {
        return 5;
    }

    zOpt::SetThrottleMode(0);
    zOpt::SetSteeringMode(0);
    zOpt::SetCursorMode(0);
    gameControl |= 8;
    if (gameControl != 8 || zOpt::GetThrottleMode() != 0 || zOpt::GetSteeringMode() != 0 ||
        zOpt::GetCursorMode() != 0 || zOpt::GetCameraModePlayerState() != 1) {
        return 6;
    }

    zOpt::SetGameDifficultyMode(2);
    if (difficulty != 2 || zOpt::GetGameDifficultyMode() != 2) {
        return 7;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetEffectsLevelForCurrentHwMode(0);
    if (effectsSw != 0 || effectsHw != 0 || g_zEffect_ConditionalEffectLevel != 2 ||
        zOpt::GetEffectsLevelForCurrentHwMode() != 0) {
        return 8;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetEffectsLevelForCurrentHwMode(2);
    if (effectsHw != 2 || g_zEffect_ConditionalEffectLevel != 0 ||
        zOpt::GetEffectsLevelForCurrentHwMode() != 2) {
        return 9;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetObjectLODForCurrentHwMode(1);
    if (objectLodSw != 1 || objectLodHw != 0 || zOpt::GetObjectLODForCurrentHwMode() != 1) {
        return 10;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetObjectLODForCurrentHwMode(2);
    if (objectLodHw != 2 || zOpt::GetObjectLODForCurrentHwMode() != 2) {
        return 11;
    }

    zOpt::SetMuteSoundOption(1);
    if (zOpt::GetMuteSoundOption() != 1) {
        return 12;
    }

    zOpt::SetSoundVolumeOption(0.625f);
    if (soundVolume != 0.625f || globalVolume != 0.625f || zOpt::GetSoundVolumeOption() != 0.625f) {
        return 13;
    }

    zOpt::SetSoundLODOption(3);
    if (soundLod != 3 || zOpt::GetSoundLODOption() != 3) {
        return 14;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetTextureMemoryForCurrentHwMode(16);
    if (textureMemorySw != 16 || zOpt::GetTextureMemoryForCurrentHwMode() != 16) {
        return 15;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetTextureMemoryForCurrentHwMode(32);
    if (textureMemoryHw != 32 || zOpt::GetTextureMemoryForCurrentHwMode() != 32) {
        return 16;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetGraphicsFlagsForCurrentHwMode(0x11);
    if (zOpt::GetGraphicsFlagsForCurrentHwMode() != 0x11) {
        return 17;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetGraphicsFlagsForCurrentHwMode(0x22);
    if (zOpt::GetGraphicsFlagsForCurrentHwMode() != 0x22) {
        return 18;
    }

    zOpt::SetPlayerName("Pilot");
    if (std::strcmp(playerNameBuffer, "Pilot") != 0 || zOpt_GetPlayerName() != playerNameBuffer) {
        return 19;
    }

    zOpt::SetPlayerName("LongPlayerName");
    if (std::strcmp(playerNameBuffer, "LongPla") != 0) {
        return 20;
    }

    ZOPT_GAME_CONTROL_OPTIONS = nullptr;
    ZOPT_HUD_TYPE_SW = nullptr;
    ZOPT_HUD_TYPE_HW = nullptr;
    g_zOpt_GameDifficultyOption = nullptr;
    ZOPT_EFFECTS_LEVEL_SW = nullptr;
    ZOPT_EFFECTS_LEVEL_HW = nullptr;
    ZOPT_OBJECT_LOD_SW = nullptr;
    ZOPT_OBJECT_LOD_HW = nullptr;
    ZOPT_MUTE_SOUND = nullptr;
    ZOPT_SOUND_VOLUME = nullptr;
    g_zSnd_GlobalVolumeScalePtr = nullptr;
    ZOPT_SOUND_LOD = nullptr;
    ZOPT_TEXTURE_MEMORY_SW = nullptr;
    ZOPT_TEXTURE_MEMORY_HW = nullptr;
    ZOPT_GFX_FLAGS_SW = nullptr;
    ZOPT_GFX_FLAGS_HW = nullptr;
    ZOPT_PLAYER_NAME = nullptr;
    return 0;
}

extern "C" int zopt_toggle_hud_type_for_current_hw_mode_smoke() {
    int *const oldHudTypeSw = ZOPT_HUD_TYPE_SW;
    int *const oldHudTypeHw = ZOPT_HUD_TYPE_HW;
    const int oldHwMode = g_zOpt_HwMode;
    const int oldLayoutsInitialized = g_HudUiMgrHudLayoutsInitialized;

    int hudTypeSw = ZOPT_HUD_TYPE_STANDARD;
    int hudTypeHw = 7;
    ZOPT_HUD_TYPE_SW = &hudTypeSw;
    ZOPT_HUD_TYPE_HW = &hudTypeHw;
    g_HudUiMgrHudLayoutsInitialized = 0;

    g_zOpt_HwMode = 0;
    int returned = zOpt::ToggleHudTypeForCurrentHwMode();
    bool ok = returned == ZOPT_HUD_TYPE_STANDARD &&
              hudTypeSw == ZOPT_HUD_TYPE_PERSPECTIVE && hudTypeHw == 7;

    returned = zOpt::ToggleHudTypeForCurrentHwMode();
    ok = ok && returned == ZOPT_HUD_TYPE_PERSPECTIVE &&
         hudTypeSw == ZOPT_HUD_TYPE_STANDARD && hudTypeHw == 7;

    hudTypeSw = 9;
    returned = zOpt::ToggleHudTypeForCurrentHwMode();
    ok = ok && returned == 9 && hudTypeSw == 9 && hudTypeHw == 7;

    g_zOpt_HwMode = 1;
    hudTypeHw = ZOPT_HUD_TYPE_PERSPECTIVE;
    returned = zOpt::ToggleHudTypeForCurrentHwMode();
    ok = ok && returned == ZOPT_HUD_TYPE_PERSPECTIVE &&
         hudTypeHw == ZOPT_HUD_TYPE_STANDARD && hudTypeSw == 9;

    ZOPT_HUD_TYPE_SW = oldHudTypeSw;
    ZOPT_HUD_TYPE_HW = oldHudTypeHw;
    g_zOpt_HwMode = oldHwMode;
    g_HudUiMgrHudLayoutsInitialized = oldLayoutsInitialized;
    return ok ? 0 : 1;
}

extern "C" int zopt_network_enabled_accessor_smoke() {
    std::int32_t networkEnabled = 0;
    std::int32_t networkModem = 0;
    std::int32_t networkListen = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zOpt_NetworkModemOption = &networkModem;
    g_zOpt_NetworkListenOption = &networkListen;

    const bool disabled = zOpt::GetNetworkEnabled() == 0;
    zOpt::SetNetworkEnabled(1);
    zOpt::SetNetworkModemEnabled(1);
    zOpt::SetNetworkListenEnabled(1);
    const bool enabled = zOpt::GetNetworkEnabled() == 1;
    const bool modemEnabled = networkModem == 1 && zOpt::GetNetworkModemEnabled() == 1;
    const bool listenEnabled = networkListen == 1;

    ZOPT_NETWORK_ENABLED = nullptr;
    g_zOpt_NetworkModemOption = nullptr;
    g_zOpt_NetworkListenOption = nullptr;
    return disabled && enabled && modemEnabled && listenEnabled ? 0 : 1;
}

extern "C" int hud_sensor_objective_slot_reset_smoke() {
    HudSensorObjectiveSlot slot{};
    slot.completedFlag = 7;
    slot.objectiveTitle[0] = 'T';
    slot.objectiveDesc[0] = 'D';
    slot.objectiveSummary[0] = 'S';

    slot.Reset();

    return slot.completedFlag == 0 && slot.objectiveImage == nullptr &&
                   slot.objectiveTitle[0] == '\0' && slot.objectiveDesc[0] == '\0' &&
                   slot.objectiveSummary[0] == '\0'
               ? 0
               : 1;
}

extern "C" int hud_sensor_tracker_unload_objectives_smoke() {
    std::int32_t networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    HudSensorTracker tracker{};
    tracker.objectiveCount = 2;
    tracker.currentObjectiveIndex = 4;
    tracker.firstIncompleteObjectiveIndex = 5;
    tracker.completedObjectiveCount = 6;
    tracker.objectiveSlots[0].completedFlag = 1;
    tracker.objectiveSlots[0].objectiveTitle[0] = 'A';
    tracker.objectiveSlots[1].completedFlag = 1;
    tracker.objectiveSlots[1].objectiveTitle[0] = 'B';

    const bool disabledResult =
        tracker.UnloadObjectives() == 1 && tracker.currentObjectiveIndex == -1 &&
        tracker.firstIncompleteObjectiveIndex == 0 && tracker.objectiveCount == 0 &&
        tracker.completedObjectiveCount == 0 && tracker.objectiveSlots[0].completedFlag == 0 &&
        tracker.objectiveSlots[0].objectiveTitle[0] == '\0' &&
        tracker.objectiveSlots[1].completedFlag == 0 &&
        tracker.objectiveSlots[1].objectiveTitle[0] == '\0';

    networkEnabled = 1;
    tracker.objectiveCount = 1;
    tracker.currentObjectiveIndex = 8;
    tracker.completedObjectiveCount = 9;
    tracker.objectiveSlots[0].completedFlag = 3;
    tracker.objectiveSlots[0].objectiveTitle[0] = 'N';

    const bool enabledResult = tracker.UnloadObjectives() == 1 &&
                               tracker.currentObjectiveIndex == 8 && tracker.objectiveCount == 1 &&
                               tracker.completedObjectiveCount == 9 &&
                               tracker.objectiveSlots[0].completedFlag == 3 &&
                               tracker.objectiveSlots[0].objectiveTitle[0] == 'N';

    ZOPT_NETWORK_ENABLED = nullptr;

    return disabledResult && enabledResult ? 0 : 1;
}

extern "C" int hud_sensor_tracker_get_objective_briefing_strings_smoke() {
    HudSensorTracker tracker = {};
    zVidImagePartial image = {};

    std::strcpy(tracker.objectiveSlots[2].objectiveTitle, "brief summary");
    std::strcpy(tracker.objectiveSlots[2].objectiveDesc, "brief description");
    tracker.objectiveSlots[2].objectiveImage = &image;

    char *summary = nullptr;
    char *description = nullptr;
    zVidImagePartial *imageRef = nullptr;
    const int result =
        tracker.GetObjectiveBriefingStringsAndImageRef(2, &summary, &description, &imageRef);

    return result == 1 && summary == tracker.objectiveSlots[2].objectiveTitle &&
                   description == tracker.objectiveSlots[2].objectiveDesc && imageRef == &image
               ? 0
               : 1;
}

extern "C" int hud_sensor_tracker_load_race_checkpoint_meta_smoke() {
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    g_zArchive_MountedList = nullptr;

    HudSensorTracker missingTracker = {};
    missingTracker.missionId = 4;
    missingTracker.runtimeTimerSecRaw = FloatBitsForTest(3.0f);
    missingTracker.checkpointCount = 9;
    const bool missingOk =
        missingTracker.LoadRaceCheckpointMeta() == 0 &&
        missingTracker.runtimeTimerSecRaw == FloatBitsForTest(3.0f) &&
        missingTracker.checkpointCount == 9;

    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "rac", 0, tempPath) == 0) {
        g_zArchive_MountedList = oldMountedList;
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        g_zArchive_MountedList = oldMountedList;
        DeleteFileA(tempPath);
        return 2;
    }

    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 3);
    WriteZrdNamedIntArray(file, "cp_count", 7);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "race.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;
    g_zArchive_MountedList = &list;

    HudSensorTracker tracker = {};
    tracker.missionId = 4;
    const bool loadedOk =
        tracker.LoadRaceCheckpointMeta() == 1 &&
        tracker.runtimeTimerSecRaw == FloatBitsForTest(20.0f) && tracker.checkpointCount == 7;

    g_zArchive_MountedList = oldMountedList;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return missingOk && loadedOk ? 0 : 3;
}

extern "C" int hud_sensor_tracker_set_runtime_timer_sec_and_goal_value_smoke() {
    HudSensorTracker tracker = {};
    tracker.runtimeGoalValue = 1;
    tracker.runtimeTimerSecRaw = 2;

    tracker.SetRuntimeTimerSecAndGoalValue(3600, 7);

    return tracker.runtimeTimerSecRaw == 3600 && tracker.runtimeGoalValue == 7 ? 0 : 1;
}

extern "C" int hud_sensor_tracker_load_mission_core_resources_smoke() {
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zOpt_ViewRectSection **const oldRender = g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection **const oldDisplay = g_zOpt_DisplaySectionOption;
    zOpt_CameraSection **const oldCamera = g_zOpt_CameraSectionOption;
    zVideo_QueryMemoryBytesProc oldQueryTextureMemory = g_zVideo_pfnQueryTextureMemoryBytes;
    int *const oldTextureMemoryOption = g_zImage_TextureMemoryOption;

    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_CameraSection camera{};
    int textureMemoryOption = 0;
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_CameraSection *cameraPtr = &camera;
    g_zArchive_MountedList = nullptr;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_CameraSectionOption = &cameraPtr;
    g_zVideo_pfnQueryTextureMemoryBytes = TextureMemoryQueryMissingStub;
    g_zImage_TextureMemoryOption = &textureMemoryOption;

    CreateDirectoryA("support", nullptr);

    const char *paths[] = {
        "support\\initm1.gw",
        "support\\initm3.gw",
        "m1.gs",
        "custom_mission.gs",
    };

    for (int index = 0; index < 4; ++index) {
        FILE *const file = std::fopen(paths[index], "wb");
        if (file == nullptr) {
            g_zArchive_MountedList = oldMountedList;
            g_zOpt_RenderSectionOption = oldRender;
            g_zOpt_DisplaySectionOption = oldDisplay;
            g_zOpt_CameraSectionOption = oldCamera;
            g_zVideo_pfnQueryTextureMemoryBytes = oldQueryTextureMemory;
            g_zImage_TextureMemoryOption = oldTextureMemoryOption;
            return 1;
        }
        std::fclose(file);
    }

    HudSensorTracker defaultTracker = {};
    defaultTracker.Constructor();
    defaultTracker.missionFlags = 0;
    const int defaultResult = defaultTracker.LoadMissionCoreResources();
    const bool defaultOk =
        defaultResult == 1 && defaultTracker.missionId == 1 &&
        std::strcmp(defaultTracker.zbdPath.m_pchData, "m1.gs") == 0 &&
        defaultTracker.missionLoaded == 1 && defaultTracker.raceCheckpointMode == 0 &&
        std::strcmp(g_HudSensor_MissionSoundSetName, "M1") == 0 &&
        defaultTracker.worldNode == nullptr && defaultTracker.cameraNode == nullptr &&
        defaultTracker.windowNode == nullptr && defaultTracker.displayNode == nullptr &&
        render.target == nullptr && display.target == nullptr && camera.m_pCamera == nullptr;

    HudSensorTracker customTracker = {};
    customTracker.Constructor();
    customTracker.missionId = 3;
    customTracker.SetZbdPath("custom_mission.gs");
    const int customResult = customTracker.LoadMissionCoreResources();
    const bool customOk =
        customResult == 1 && customTracker.missionId == 3 &&
        std::strcmp(customTracker.zbdPath.m_pchData, "custom_mission.gs") == 0 &&
        customTracker.missionLoaded == 1 &&
        std::strcmp(g_HudSensor_MissionSoundSetName, "M3") == 0;

    for (int index = 0; index < 4; ++index) {
        DeleteFileA(paths[index]);
    }
    RemoveDirectoryA("support");

    defaultTracker.Shutdown();
    customTracker.Shutdown();
    g_zArchive_MountedList = oldMountedList;
    g_zOpt_RenderSectionOption = oldRender;
    g_zOpt_DisplaySectionOption = oldDisplay;
    g_zOpt_CameraSectionOption = oldCamera;
    g_zVideo_pfnQueryTextureMemoryBytes = oldQueryTextureMemory;
    g_zImage_TextureMemoryOption = oldTextureMemoryOption;

    return defaultOk && customOk ? 0 : 2;
}

extern "C" int hud_sensor_tracker_load_objectives_from_path_smoke() {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "obj", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 7);
    WriteZrdNamedIntArray(file, "READ_TIME", 9);
    WriteZrdNamedIntArray(file, "REVIEW_DELAY", 6);
    WriteZrdNamedIntArray(file, "FINAL_MISSION", 7);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "objectives.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;

    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zArchiveList *const oldMissionResourcePaths = g_zImage_MissionResourcePaths;
    int *const oldNetwork = ZOPT_NETWORK_ENABLED;
    g_zArchive_MountedList = &list;
    g_zImage_MissionResourcePaths = nullptr;

    std::int32_t networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    HudSensorTracker tracker = {};
    tracker.missionId = 12;
    const int loaded = tracker.LoadObjectivesFromPath("objectives.zrd");
    const bool disabledOk =
        loaded == 0 && tracker.objectivesRootNode != nullptr &&
        tracker.objectiveReadTimeSecRaw == FloatBitsForTest(9.0f) &&
        tracker.objectiveReviewDelaySecRaw == FloatBitsForTest(6.0f) &&
        tracker.objectiveReadSoundDelaySecRaw == FloatBitsForTest(2.0f) &&
        tracker.finalMissionFlag == 7 && tracker.currentObjectiveIndex == -1 &&
        tracker.firstIncompleteObjectiveIndex == 0 && tracker.objectiveCount == 1 &&
        tracker.completedObjectiveCount == 0;

    if (tracker.objectivesRootNode != nullptr) {
        zReader::FreeLoadedTree(tracker.objectivesRootNode);
        tracker.objectivesRootNode = nullptr;
    }

    networkEnabled = 1;
    HudSensorTracker networkTracker = {};
    networkTracker.missionId = 12;
    networkTracker.finalMissionFlag = 99;
    const int networkLoaded = networkTracker.LoadObjectivesFromPath("objectives.zrd");
    const bool networkOk = networkLoaded == 0 && networkTracker.objectivesRootNode != nullptr &&
                           networkTracker.finalMissionFlag == 99 &&
                           networkTracker.objectiveCount == 0;

    if (networkTracker.objectivesRootNode != nullptr) {
        zReader::FreeLoadedTree(networkTracker.objectivesRootNode);
        networkTracker.objectivesRootNode = nullptr;
    }

    if (g_zImage_MissionResourcePaths != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(g_zImage_MissionResourcePaths);
    }

    g_zArchive_MountedList = oldMountedList;
    g_zImage_MissionResourcePaths = oldMissionResourcePaths;
    ZOPT_NETWORK_ENABLED = oldNetwork;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return disabledOk && networkOk ? 0 : 3;
}

extern "C" int hud_sensor_tracker_load_objectives_from_zrd_smoke() {
    zReader::Node reviewSound[2] = {};
    reviewSound[0].type = zReader::ZRDR_NODE_INT;
    reviewSound[0].value.i32 = 2;
    reviewSound[1].type = zReader::ZRDR_NODE_STRING;
    reviewSound[1].value.str = const_cast<char *>("snd_review");

    zReader::Node readSound[3] = {};
    readSound[0].type = zReader::ZRDR_NODE_INT;
    readSound[0].value.i32 = 3;
    readSound[1].type = zReader::ZRDR_NODE_STRING;
    readSound[1].value.str = const_cast<char *>("snd_read");
    readSound[2].type = zReader::ZRDR_NODE_INT;
    readSound[2].value.i32 = FloatBitsForTest(3.5f);

    zReader::Node objective1[3] = {};
    objective1[0].type = zReader::ZRDR_NODE_INT;
    objective1[0].value.i32 = 3;
    objective1[1].type = zReader::ZRDR_NODE_STRING;
    objective1[1].value.str = const_cast<char *>("READ_SOUND");
    objective1[2].type = zReader::ZRDR_NODE_ARRAY;
    objective1[2].value.nodes = readSound;

    zReader::Node objectiveSound[2] = {};
    objectiveSound[0].type = zReader::ZRDR_NODE_INT;
    objectiveSound[0].value.i32 = 2;
    objectiveSound[1].type = zReader::ZRDR_NODE_STRING;
    objectiveSound[1].value.str = const_cast<char *>("snd_complete");

    zReader::Node rootNodes[7] = {};
    rootNodes[0].type = zReader::ZRDR_NODE_INT;
    rootNodes[0].value.i32 = 7;
    rootNodes[1].type = zReader::ZRDR_NODE_STRING;
    rootNodes[1].value.str = const_cast<char *>("REVIEW_SOUND");
    rootNodes[2].type = zReader::ZRDR_NODE_ARRAY;
    rootNodes[2].value.nodes = reviewSound;
    rootNodes[3].type = zReader::ZRDR_NODE_STRING;
    rootNodes[3].value.str = const_cast<char *>("OBJECTIVE1");
    rootNodes[4].type = zReader::ZRDR_NODE_ARRAY;
    rootNodes[4].value.nodes = objective1;
    rootNodes[5].type = zReader::ZRDR_NODE_STRING;
    rootNodes[5].value.str = const_cast<char *>("OBJECTIVE_SOUND");
    rootNodes[6].type = zReader::ZRDR_NODE_ARRAY;
    rootNodes[6].value.nodes = objectiveSound;

    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootNodes;

    zSndSample samples[4] = {};
    samples[0].replayFields.sampleId = "snd_review";
    samples[0].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1000);
    samples[1].replayFields.sampleId = "snd_read";
    samples[1].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1004);
    samples[2].replayFields.sampleId = "snd_complete";
    samples[2].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x1008);
    samples[3].replayFields.sampleId = "snd_incoming";
    samples[3].primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(0x100c);

    zSndSampleSet sampleSet = {};
    sampleSet.sampleCount = 4;
    sampleSet.samples = samples;
    zSndSampleSet *sampleSetSlots[1] = {&sampleSet};

    const zSndSampleSetRegistry oldRegistry = g_zSnd_SampleSetRegistry;
    const int oldSndInitialized = g_zSnd_IsInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    int *const oldNetwork = ZOPT_NETWORK_ENABLED;

    g_zSnd_SampleSetRegistry.begin = sampleSetSlots;
    g_zSnd_SampleSetRegistry.end = sampleSetSlots + 1;
    g_zSnd_SampleSetRegistry.capacityEnd = sampleSetSlots + 1;
    g_zSnd_IsInitialized = 1;
    g_zSnd_ActiveBackend = 0;

    std::int32_t networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    HudSensorTracker tracker = {};
    tracker.objectivesRootNode = &root;
    tracker.objectiveCount = 2;
    tracker.objectiveSlots[0].completedFlag = 1;
    tracker.objectiveSlots[1].completedFlag = 0;
    const int loaded = tracker.LoadObjectivesFromZrd();
    const bool loadedOk =
        loaded == 0 &&
        tracker.objectiveReviewSfx == &samples[0] &&
        tracker.objectiveSlots[0].readSoundSample == &samples[1] &&
        samples[1].playbackEventHandler == HudSensorTracker::OnObjectiveReadSoundEvent &&
        tracker.objectiveReadSoundDelaySecRaw == FloatBitsForTest(3.5f) &&
        tracker.objectiveCompleteSfx == &samples[2] &&
        tracker.objectiveIncomingSfx == &samples[3] &&
        tracker.firstIncompleteObjectiveIndex == 1;

    networkEnabled = 1;
    HudSensorTracker networkTracker = {};
    networkTracker.objectivesRootNode = &root;
    networkTracker.firstIncompleteObjectiveIndex = 9;
    const int networkLoaded = networkTracker.LoadObjectivesFromZrd();
    const bool networkOk =
        networkLoaded == 0 &&
        networkTracker.objectiveReviewSfx == &samples[0] &&
        networkTracker.objectiveCompleteSfx == nullptr &&
        networkTracker.objectiveIncomingSfx == nullptr &&
        networkTracker.firstIncompleteObjectiveIndex == 9;

    g_zSnd_SampleSetRegistry = oldRegistry;
    g_zSnd_IsInitialized = oldSndInitialized;
    g_zSnd_ActiveBackend = oldActiveBackend;
    ZOPT_NETWORK_ENABLED = oldNetwork;

    return loadedOk && networkOk ? 0 : 1;
}

extern "C" int hud_sensor_tracker_load_mission_weather_fx_smoke() {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "wfx", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 5);
    WriteZrdStringNode(file, "MISSION12");
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 15);
    WriteZrdNamedDirectInt(file, "PARTICLES", 4);
    WriteZrdNamedStringNode(file, "TYPE", "SNOW");
    WriteZrdNamedColorArray(file, "COLOR", 10, 20, 30);
    WriteZrdNamedFloatNode(file, "WIND_DIR", 1.25f);
    WriteZrdNamedFloatNode(file, "WIND_VEL", 2.5f);
    WriteZrdNamedFloatNode(file, "GRAVITY", 3.75f);
    WriteZrdNamedFloatPairArray(file, "ALPHA_GRADIENT", 0.25f, 0.75f);
    WriteZrdStringNode(file, "MISSION13");
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 5);
    WriteZrdNamedDirectInt(file, "PARTICLES", 2);
    WriteZrdNamedStringNode(file, "TYPE", "RAIN");
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "weather.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;

    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    const unsigned int oldRMask = g_zVideo_PixelPack_RMaskShifted;
    const unsigned int oldGMask = g_zVideo_PixelPack_GMaskShifted;
    const unsigned int oldRShift = g_zVideo_PixelPack_RShift;
    const unsigned int oldGShift = g_zVideo_PixelPack_GShift;
    const unsigned int oldBShift = g_zVideo_PixelPack_BShiftTo8;
    HudUiContainer *const pass3 = (HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal);
    HudUiElement *const oldPass3Head = pass3->childHead;
    HudUiElement *const oldPass3Tail = pass3->childTail;

    g_zArchive_MountedList = &list;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;
    pass3->childHead = nullptr;
    pass3->childTail = nullptr;

    HudSensorTracker snowTracker = {};
    snowTracker.missionId = 12;
    snowTracker.LoadMissionWeatherFx("weather.zrd");
    HudWeatherFx *const snowFx = static_cast<HudWeatherFx *>(snowTracker.fxPass3Obj);
    const bool snowOk =
        snowFx != nullptr && snowFx->ftable == &g_HudWeatherFxSnow_Vtable &&
        snowFx->particleCount == 4 &&
        snowFx->packedColor16 == (zVid_PackColorRGB(10, 20, 30) & 0xffffu) &&
        snowFx->windDirection == 1.25f && snowFx->windVelocity == 2.5f &&
        snowFx->gravity == 3.75f && snowFx->alphaStartScale == 0.25f &&
        snowFx->alphaEndScale == 0.75f && pass3->childHead == snowTracker.fxPass3Obj &&
        pass3->childTail == snowTracker.fxPass3Obj && snowTracker.fxPass3Obj->parent == pass3;

    pass3->childHead = nullptr;
    pass3->childTail = nullptr;
    HudSensorTracker rainTracker = {};
    rainTracker.missionId = 13;
    rainTracker.LoadMissionWeatherFx("weather.zrd");
    HudWeatherFx *const rainFx = static_cast<HudWeatherFx *>(rainTracker.fxPass3Obj);
    const bool rainOk =
        rainFx != nullptr && rainFx->ftable == &g_HudWeatherFxRain_Vtable &&
        rainFx->particleCount == 2 && rainFx->packedColor16 == 0x7fff &&
        pass3->childHead == rainTracker.fxPass3Obj && pass3->childTail == rainTracker.fxPass3Obj;

    FreeHudWeatherFxForTest(snowTracker.fxPass3Obj);
    FreeHudWeatherFxForTest(rainTracker.fxPass3Obj);
    pass3->childHead = oldPass3Head;
    pass3->childTail = oldPass3Tail;
    g_zArchive_MountedList = oldMountedList;
    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_PixelPack_RMaskShifted = oldRMask;
    g_zVideo_PixelPack_GMaskShifted = oldGMask;
    g_zVideo_PixelPack_RShift = oldRShift;
    g_zVideo_PixelPack_GShift = oldGShift;
    g_zVideo_PixelPack_BShiftTo8 = oldBShift;
    CloseHandle(file);
    DeleteFileA(tempPath);

    return snowOk && rainOk ? 0 : 3;
}

extern "C" int zopt_view_rect_target_side_effects_smoke() {
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zOpt_CameraSection cameraSection{};
    zClass_WindowDataPartial windowData{};
    zClass_DisplayDataPartial displayData{};
    zClass_CameraDataPartial cameraData{};
    zClass_NodePartial windowNode{};
    zClass_NodePartial displayNode{};
    zClass_NodePartial cameraNode{};
    windowNode.classId = 3;
    windowNode.classData = &windowData;
    displayNode.classId = 4;
    displayNode.classData = &displayData;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    render.target = &windowNode;
    display.target = &displayNode;
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_CameraSection *cameraSectionPtr = &cameraSection;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_CameraSectionOption = &cameraSectionPtr;

    render.x = 5;
    render.y = 6;
    render.width = 320;
    render.height = 240;
    render.target = nullptr;
    zOpt::RenderSection_SetTargetWindow(&windowNode);
    if (zOpt::GetRenderSection() != &render || windowData.resolutionWidth != 320 ||
        windowData.resolutionHeight != 240 || windowData.viewportWidth != 5 ||
        windowData.viewportHeight != 6) {
        return 3;
    }

    display.x = 7;
    display.y = 8;
    display.width = 400;
    display.height = 300;
    display.target = nullptr;
    zOpt::DisplaySection_SetTargetDisplay(&displayNode);
    if (displayData.width != 400 || displayData.height != 300 || displayData.x != 7 ||
        displayData.y != 8) {
        return 4;
    }

    zOpt::RenderSection_SetSize(640, 480);
    zOpt::RenderSection_SetPosition(10, 20);
    if (windowData.resolutionWidth != 640 || windowData.resolutionHeight != 480 ||
        windowData.viewportWidth != 10 || windowData.viewportHeight != 20) {
        return 1;
    }

    zOpt::DisplaySection_SetSize(800, 600);
    zOpt::DisplaySection_SetPosition(30, 40);
    if (displayData.width != 800 || displayData.height != 600 || displayData.x != 30 ||
        displayData.y != 40) {
        return 2;
    }

    int objectLodSw = 2;
    int objectLodHw = 1;
    ZOPT_OBJECT_LOD_SW = &objectLodSw;
    ZOPT_OBJECT_LOD_HW = &objectLodHw;
    g_zOpt_HwMode = 0;
    render.width = 800;
    render.height = 400;
    cameraData.viewportWidth = 400.0f;
    cameraData.viewportHeight = 200.0f;
    cameraData.frustumWidth = 0.25f;
    cameraData.frustumHeight = 0.5f;
    zOpt::CameraSection_SetActiveCamera(&cameraNode);
    if (cameraSection.m_pCamera != &cameraNode || cameraData.frustumWidth != 1.0f ||
        cameraData.frustumHeight != 0.5f || cameraData.fovX != 1.0f / 400.0f ||
        cameraData.fovY != 0.5f / 200.0f || cameraData.clipDistance != 0.5f) {
        return 5;
    }

    zOpt::CameraSection_SetActiveCamera(nullptr);
    return cameraSection.m_pCamera == nullptr ? 0 : 6;
}

extern "C" int zopt_section_accessor_smoke() {
    std::int32_t videoStride = 2048;
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    display.width = 800;
    display.height = 600;
    display.bitsPerPixel = 16;
    window.height = 480;
    std::int32_t replicate = 1;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_STRIDE = &videoStride;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    zOpt_ViewRectSection clamp{};
    clamp.x = 10;
    clamp.y = 20;
    clamp.maxXInclusive = 30;
    clamp.maxYInclusive = 40;

    float belowPoint[2] = {5.0f, 15.0f};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, belowPoint);
    const bool belowClamped = belowPoint[0] == 10.0f && belowPoint[1] == 20.0f;

    float insidePoint[2] = {11.5f, 25.25f};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, insidePoint);
    const bool insideKept = insidePoint[0] == 11.5f && insidePoint[1] == 25.25f;

    float abovePoint[2] = {31.0f, 45.0f};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, abovePoint);
    const bool aboveClamped = abovePoint[0] == 30.0f && abovePoint[1] == 40.0f;

    float nanPoint[2] = {std::numeric_limits<float>::quiet_NaN(),
                         std::numeric_limits<float>::quiet_NaN()};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, nanPoint);
    const bool nanClampedToMax = nanPoint[0] == 30.0f && nanPoint[1] == 40.0f;

    return zOpt::GetDisplaySection() == &display && zOpt_DisplaySection_GetWidth() == 800 &&
                   zOpt_DisplaySection_GetHeight() == 600 &&
                   zOpt::GetDisplaySectionBitsPerPixel() == 16 &&
                   zOpt::GetVideoStrideValue() == 2048 && zOpt::GetReplicateMode() == 1 &&
                   zOpt::GetWindowSection() == &window && zOpt::GetWindowSectionHeight() == 480 &&
                   belowClamped && insideKept && aboveClamped && nanClampedToMax
               ? 0
               : 1;
}

extern "C" int zclipalt_set_source_rect_smoke() {
    zClipAltFloatRect rect{3.0f, 4.0f, 19.0f, 25.0f};
    gAltClipSourceRectValid = 0;

    zClipAlt::SetSourceRect(&rect);

    return g_zClipAlt_SourceLeft == 3.0f && g_zClipAlt_SourceTop == 4.0f &&
                   g_zClipAlt_SourceRight == 19.0f && g_zClipAlt_SourceBottom == 25.0f &&
                   g_zClipAlt_SourceWidth == 16.0f && g_zClipAlt_SourceHeight == 21.0f &&
                   gAltClipSourceRectValid == 1
               ? 0
               : 1;
}

extern "C" int zclipalt_set_target_rect_smoke() {
    zClipAltFloatRect source{2.0f, 4.0f, 18.0f, 28.0f};
    zClipAltFloatRect target{10.0f, 20.0f, 42.0f, 44.0f};
    gClipRect_Primary.xMin = 100.0f;
    gClipRect_Primary.yMin = 60.0f;

    zClipAlt::SetSourceRect(&source);
    g_zClipAlt_BiasIncludesPrimaryOrigin = 0;
    zClipAlt::SetTargetRect(&target, 0);

    if (gClipRect_Alt.flags != 0x0f || gClipRect_Alt.xMin != 10.0f || gClipRect_Alt.yMin != 20.0f ||
        gClipRect_Alt.xMax != 42.0f || gClipRect_Alt.yMax != 44.0f ||
        gClipRect_Alt.xMaxAlt != 42.0f || gClipRect_Alt.yMaxAlt != 44.0f ||
        g_zClipAlt_RemapOffsetX != 8.0f || g_zClipAlt_RemapOffsetY != 16.0f ||
        g_zClipAlt_RemapScaleX != 0.5f || g_zClipAlt_RemapScaleY != 1.0f ||
        g_zClipAlt_RemapBiasX != -3.0f || g_zClipAlt_RemapBiasY != -16.0f) {
        return 1;
    }

    g_zClipAlt_BiasIncludesPrimaryOrigin = 1;
    zClipAlt::SetTargetRect(&target, 1);
    return g_zClipAlt_RemapBiasX == 47.0f && g_zClipAlt_RemapBiasY == 14.0f ? 0 : 2;
}

extern "C" int zclass_cls_di_segment_batch_vs_polygon_smoke() {
    zVec3 triangle[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    zModel_PickFaceScenePayload facePayload{};
    zModel_PickFaceEntry faceEntry{};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.scenePayload = &facePayload;

    zClass_NodePartial owner{};
    PlayerProbeSampleCandidateBuffer buckets[3] = {};
    zClass_DiSegmentEndpoints segments[3] = {};
    segments[0].start = {0.25f, 0.25f, 1.0f};
    segments[0].end = {0.25f, 0.25f, -1.0f};
    segments[1].start = {1.25f, 0.25f, 1.0f};
    segments[1].end = {1.25f, 0.25f, -1.0f};
    segments[2].start = {0.4f, 0.4f, 1.0f};
    segments[2].end = {0.4f, 0.4f, -1.0f};
    int activeMask[3] = {1, 1, 0};

    if (zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygon(
            &owner, buckets, segments, activeMask, 3, triangle, &faceEntry) != 1) {
        return 1;
    }

    if (buckets[0].candidateCount != 1 || buckets[1].candidateCount != 0 ||
        buckets[2].candidateCount != 0 || buckets[0].entries[0].node != &owner ||
        buckets[0].entries[0].scenePayload != &facePayload ||
        buckets[0].entries[0].surfaceNormal.x != 0.0f ||
        buckets[0].entries[0].surfaceNormal.y != 0.0f ||
        buckets[0].entries[0].surfaceNormal.z != 1.0f ||
        buckets[0].entries[0].hitPos.x != 0.25f ||
        buckets[0].entries[0].hitPos.y != 0.25f ||
        buckets[0].entries[0].hitPos.z != 0.0f || activeMask[2] != 0) {
        return 2;
    }

    buckets[0] = {};
    activeMask[0] = 1;
    segments[0].start = {0.25f, 0.25f, -1.0f};
    segments[0].end = {0.25f, 0.25f, 1.0f};
    if (zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygon(
            &owner, buckets, segments, activeMask, 1, triangle, &faceEntry) != 0 ||
        buckets[0].candidateCount != 0) {
        return 3;
    }

    faceEntry.flagsAndVertexCount = 0x103;
    if (zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygon(
            &owner, buckets, segments, activeMask, 1, triangle, &faceEntry) != 1 ||
        buckets[0].candidateCount != 1 || buckets[0].entries[0].hitPos.z != 0.0f) {
        return 4;
    }

    return 0;
}

extern "C" int zclass_cls_di_segment_batch_vs_polygon_uv_smoke() {
    zVec3 triangle[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    zModel_PickFaceUvData faceUvData = {{{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 20.0f}}};
    zModel_PickFaceScenePayload facePayload{};
    zModel_PickFaceEntry faceEntry{};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;

    zClass_NodePartial owner{};
    PlayerProbeSampleCandidateBuffer buckets[1] = {};
    zClass_DiSegmentEndpoints segments[1] = {};
    segments[0].start = {0.25f, 0.25f, 1.0f};
    segments[0].end = {0.25f, 0.25f, -1.0f};
    int activeMask[1] = {1};
    zVec2 scratchUv{};

    g_OptCatalogDamageMaskEnabled = 1;
    g_OptCatalogDamageMaskPhaseU = -1.0f;
    g_OptCatalogDamageMaskPhaseV = -1.0f;
    if (zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
            &owner, buckets, segments, activeMask, 1, triangle, &faceUvData, &scratchUv,
            &faceEntry) != 1 ||
        buckets[0].candidateCount != 1 || buckets[0].entries[0].node != &owner ||
        buckets[0].entries[0].scenePayload != &facePayload ||
        g_OptCatalogDamageMaskPhaseU != 2.5f || g_OptCatalogDamageMaskPhaseV != 5.0f ||
        scratchUv.x != 2.5f || scratchUv.y != 5.0f) {
        return 1;
    }

    buckets[0] = {};
    activeMask[0] = 1;
    g_OptCatalogDamageMaskEnabled = 0;
    g_OptCatalogDamageMaskPhaseU = 12.0f;
    g_OptCatalogDamageMaskPhaseV = 34.0f;
    scratchUv = {};
    if (zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
            &owner, buckets, segments, activeMask, 1, triangle, &faceUvData, &scratchUv,
            &faceEntry) != 1 ||
        buckets[0].candidateCount != 1 || g_OptCatalogDamageMaskPhaseU != 12.0f ||
        g_OptCatalogDamageMaskPhaseV != 34.0f || scratchUv.x != 0.0f || scratchUv.y != 0.0f) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_cls_di_filter_regions_polygon_damage_mask_uv_smoke() {
    zVec3 boxCornerValues[8] = {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f},
                                {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f},
                                {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
                                {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    zBBoxCorners bboxCorners{};
    for (std::int32_t i = 0; i < 8; ++i) {
        bboxCorners.values[i * 3 + 0] = boxCornerValues[i].x;
        bboxCorners.values[i * 3 + 1] = boxCornerValues[i].y;
        bboxCorners.values[i * 3 + 2] = boxCornerValues[i].z;
    }

    zClass_NodePartial owner{};
    PlayerProbeSampleCandidateBuffer buckets[2] = {};
    zClass_DiSegmentEndpoints segments[2] = {};
    segments[0].start = {-1.0f, 0.5f, 0.5f};
    segments[0].end = {0.5f, 0.5f, 0.5f};
    segments[1].start = {-1.0f, 1.5f, 0.5f};
    segments[1].end = {0.5f, 1.5f, 0.5f};
    int activeMask[2] = {1, 1};

    if (zClass_cls_di::FilterRegionsAgainstPolygonWithDamageMaskUv(
            &owner, buckets, segments, activeMask, 2, &bboxCorners) != 1) {
        return 1;
    }

    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.0001f; };
    if (buckets[0].candidateCount != 1 || buckets[1].candidateCount != 0 ||
        buckets[0].entries[0].node != &owner || buckets[0].entries[0].scenePayload != nullptr ||
        !nearFloat(buckets[0].entries[0].surfaceNormal.x, -1.0f) ||
        !nearFloat(buckets[0].entries[0].surfaceNormal.y, 0.0f) ||
        !nearFloat(buckets[0].entries[0].surfaceNormal.z, 0.0f) ||
        !nearFloat(buckets[0].entries[0].hitPos.x, 0.0f) ||
        !nearFloat(buckets[0].entries[0].hitPos.y, 0.5f) ||
        !nearFloat(buckets[0].entries[0].hitPos.z, 0.5f) || activeMask[0] != 1 ||
        activeMask[1] != 1) {
        return 2;
    }

    buckets[0] = {};
    activeMask[0] = 0;
    if (zClass_cls_di::FilterRegionsAgainstPolygonWithDamageMaskUv(
            &owner, buckets, segments, activeMask, 1, &bboxCorners) != 0 ||
        buckets[0].candidateCount != 0) {
        return 3;
    }

    return 0;
}

extern "C" int zclass_cls_di_filter_regions_against_polygon_smoke() {
    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.0001f; };

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zVec3 vertices[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_PickFaceUvData faceUvData = {{{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 20.0f}}};
    zModel_PickFaceScenePayload facePayload{};
    zModel_PickFaceEntry faceEntry{};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = indices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;

    zModel_PickFaceData faceData{};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = vertices;

    zClass_NodePartial owner{};
    PlayerProbeSampleCandidateBuffer buckets[1] = {};
    zClass_DiSegmentEndpoints segments[1] = {};
    segments[0].start = {0.25f, 0.25f, 1.0f};
    segments[0].end = {0.25f, 0.25f, -1.0f};
    int activeMask[1] = {1};

    facePayload.flags = 0;
    g_OptCatalogDamageMaskEnabled = 1;
    g_OptCatalogDamageMaskPhaseU = -1.0f;
    g_OptCatalogDamageMaskPhaseV = -1.0f;
    zClass_cls_di::FilterRegionsAgainstPolygon(&owner, &faceData, segments, activeMask, 1,
                                               buckets);
    if (buckets[0].candidateCount != 1 || buckets[0].entries[0].node != &owner ||
        buckets[0].entries[0].scenePayload != &facePayload ||
        !nearFloat(buckets[0].entries[0].surfaceNormal.z, 1.0f) ||
        !nearFloat(buckets[0].entries[0].hitPos.x, 0.25f) ||
        !nearFloat(buckets[0].entries[0].hitPos.y, 0.25f) ||
        !nearFloat(buckets[0].entries[0].hitPos.z, 0.0f) ||
        g_OptCatalogDamageMaskPhaseU != -1.0f || g_OptCatalogDamageMaskPhaseV != -1.0f) {
        return 1;
    }

    buckets[0] = {};
    activeMask[0] = 1;
    facePayload.flags = 0x0100;
    g_OptCatalogDamageMaskPhaseU = -1.0f;
    g_OptCatalogDamageMaskPhaseV = -1.0f;
    zClass_cls_di::FilterRegionsAgainstPolygon(&owner, &faceData, segments, activeMask, 1,
                                               buckets);
    if (buckets[0].candidateCount != 1 || buckets[0].entries[0].scenePayload != &facePayload ||
        g_OptCatalogDamageMaskPhaseU != 2.5f || g_OptCatalogDamageMaskPhaseV != 5.0f) {
        return 2;
    }

    zVec3 morphVertices[3] = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    facePayload.flags = 0;
    faceData.flags = 8;
    faceData.morphVertexCount = 3;
    faceData.morphWeight = 0.5f;
    faceData.morphVertices = morphVertices;
    buckets[0] = {};
    activeMask[0] = 1;
    segments[0].start = {0.25f, 0.25f, 1.0f};
    segments[0].end = {0.25f, 0.25f, -1.0f};
    zClass_cls_di::FilterRegionsAgainstPolygon(&owner, &faceData, segments, activeMask, 1,
                                               buckets);
    if (buckets[0].candidateCount != 1 || !nearFloat(buckets[0].entries[0].hitPos.z, 0.5f) ||
        !nearFloat(g_zModel_SharedVec3ScratchA[0].z, 0.5f)) {
        return 3;
    }

    faceData.flags = 0;
    faceData.morphVertexCount = 0;
    faceData.morphWeight = 0.0f;
    faceData.morphVertices = nullptr;
    matrixFlags[0] = 0;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 10.0f, 0.0f, 0.0f};
    buckets[0] = {};
    activeMask[0] = 1;
    segments[0].start = {10.25f, 0.25f, 1.0f};
    segments[0].end = {10.25f, 0.25f, -1.0f};
    zClass_cls_di::FilterRegionsAgainstPolygon(&owner, &faceData, segments, activeMask, 1,
                                               buckets);
    if (buckets[0].candidateCount != 1 || !nearFloat(buckets[0].entries[0].hitPos.x, 10.25f) ||
        !nearFloat(buckets[0].entries[0].hitPos.z, 0.0f) ||
        !nearFloat(g_zClass_DiFaceVertexScratch4[0].x, 10.0f)) {
        return 4;
    }

    return 0;
}

extern "C" int zclass_cls_di_frustum_test_and_pick_smoke() {
    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zClass_NodePartial node{};
    std::int32_t nodeClassData = 0;
    node.flags = 0x100;
    node.classId = 2;
    node.classData = &nodeClassData;
    node.cachedBounds[0] = 0.0f;
    node.cachedBounds[1] = 0.0f;
    node.cachedBounds[2] = 0.0f;
    node.cachedBounds[3] = 1.0f;
    node.cachedBounds[4] = 1.0f;
    node.cachedBounds[5] = 1.0f;

    g_DiPickPointCount = 2;
    g_DiSegmentBounds[0] = {0.25f, 0.25f, 0.25f, 0.75f, 0.75f, 0.75f};
    g_DiSegmentBounds[1] = {2.0f, 0.25f, 0.25f, 3.0f, 0.75f, 0.75f};
    int activeMask[2] = {1, 1};
    if (zClass_cls_di::FrustumTestAndPick(&node, activeMask) != 0 || activeMask[0] != 1 ||
        activeMask[1] != 0) {
        return 1;
    }

    activeMask[0] = 1;
    g_DiSegmentBounds[0] = {2.0f, 0.25f, 0.25f, 3.0f, 0.75f, 0.75f};
    if (zClass_cls_di::FrustumTestAndPick(&node, activeMask) != 1 || activeMask[0] != 0) {
        return 2;
    }

    zClass_DiSegmentEndpoints segments[1] = {};
    segments[0].start = {-1.0f, 0.5f, 0.5f};
    segments[0].end = {0.5f, 0.5f, 0.5f};
    g_DiPickPointArray = &segments[0].start;
    g_DiPickPointCount = 1;
    PlayerProbeSampleCandidateBuffer buckets[1] = {};
    g_DiPickCandidateBuffer = buckets;
    g_DiSegmentBounds[0] = {-1.0f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f};
    activeMask[0] = 1;
    node.flags = 0x120;
    if (zClass_cls_di::FrustumTestAndPick(&node, activeMask) != 0 ||
        buckets[0].candidateCount != 1 || buckets[0].entries[0].node != &node ||
        buckets[0].entries[0].scenePayload != nullptr) {
        return 3;
    }

    node.flags = 0;
    activeMask[0] = 1;
    if (zClass_cls_di::FrustumTestAndPick(&node, activeMask) != 1 || activeMask[0] != 1) {
        return 4;
    }

    return 0;
}

extern "C" int zclass_cls_di_point_query_chain_smoke() {
    reset_zclass_type_lists_for_test();

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zVec3 vertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                         {1.0f, 0.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_MaterialPartial material{};
    zDiEntryPartial entry{};
    entry.flagsAndIndexCount = 3;
    entry.vertexIndices = indices;
    entry.material = &material;
    entry.variantTagInitialized = 2;
    entry.variantTag = 0x44;
    entry.unknown_1a[0] = 0x55;

    zDiPartial di{};
    di.entryCount = 1;
    di.vertCount = 3;
    di.entries = &entry;
    di.verts = vertices;

    zVec3 query{0.25f, 0.5f, 0.25f};
    zClassDiPickCandidateEntry candidate{};
    if (zDi::BuildPickCandidateForQueryPoint(&di, &candidate, &query) != 1 ||
        candidate.scenePayload != &material || candidate.hitPos.y != 0.0f ||
        candidate.variantTag.count != 2 || candidate.variantTag.tags[0] != 0x44 ||
        candidate.variantTag.tags[1] != 0x55) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    query.y = -1.0f;
    if (zDi::BuildPickCandidateForQueryPoint(&di, &candidate, &query) != 0) {
        free_zclass_type_lists_for_test();
        return 2;
    }
    query.y = 0.5f;

    zClass_Object3DDataPartial objectData{};
    objectData.flags = 8;
    zClass_NodePartial objectNode{};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;

    g_DiPickQueryPoint = query;
    if (zClass_cls_di::IsPickQueryPointOutsideViewBBoxXZ(&objectNode) != 0) {
        free_zclass_type_lists_for_test();
        return 3;
    }
    g_DiPickQueryPoint.x = 2.0f;
    if (zClass_cls_di::IsPickQueryPointOutsideViewBBoxXZ(&objectNode) != 1) {
        free_zclass_type_lists_for_test();
        return 4;
    }
    objectNode.flags &= ~0x100;
    g_DiPickQueryPoint = query;
    if (zClass_cls_di::IsPickQueryPointOutsideViewBBoxXZ(&objectNode) != 1) {
        free_zclass_type_lists_for_test();
        return 5;
    }
    objectNode.flags = 0x11c;

    PlayerProbeSampleCandidateBuffer pickBuffer{};
    g_DiPickCandidateBuffer = &pickBuffer;
    g_DiPickCandidateCursor = pickBuffer.entries;
    if (zClass_cls_di::BuildPickCandidateList(&objectNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &objectNode) {
        free_zclass_type_lists_for_test();
        return 6;
    }

    zClass_NodePartial disabledNode = objectNode;
    disabledNode.flags &= ~0x04;
    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    if (zClass_cls_di::BuildPickCandidateList(&disabledNode, 1) != 1 ||
        pickBuffer.candidateCount != 0) {
        free_zclass_type_lists_for_test();
        return 7;
    }

    pickBuffer = {};
    pickBuffer.candidateCount = 32;
    g_DiPickCandidateCursor = pickBuffer.entries;
    if (zClass_cls_di::BuildPickCandidateList(&objectNode, 1) != 1 ||
        pickBuffer.candidateCount != 32) {
        free_zclass_type_lists_for_test();
        return 8;
    }

    auto setIdentityMatrix = [](float *matrixValues) {
        std::memset(matrixValues, 0, sizeof(zMat4x3));
        matrixValues[0] = 1.0f;
        matrixValues[4] = 1.0f;
        matrixValues[8] = 1.0f;
    };

    zClass_AnimateDataPartial animateData{};
    setIdentityMatrix(animateData.savedParentMatrix);
    setIdentityMatrix(animateData.animatedTransform);
    zClass_NodePartial animateNode{};
    animateNode.flags = 0x11c;
    animateNode.nodeType = 0xff;
    animateNode.classId = 8;
    animateNode.classData = &animateData;
    animateNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);
    animateNode.cachedBounds[0] = 0.0f;
    animateNode.cachedBounds[1] = 0.0f;
    animateNode.cachedBounds[2] = 0.0f;
    animateNode.cachedBounds[3] = 1.0f;
    animateNode.cachedBounds[4] = 1.0f;
    animateNode.cachedBounds[5] = 1.0f;

    pickBuffer = {};
    g_DiPickCandidateBuffer = &pickBuffer;
    g_DiPickCandidateCursor = pickBuffer.entries;
    if (zClass_cls_di::BuildPickCandidatesRecursive(&animateNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &animateNode ||
        zMath::g_currentMatrixPtrSlot != &matrixSlots[0]) {
        free_zclass_type_lists_for_test();
        return 9;
    }

    zClass_LightDataPartial lightData{};
    setIdentityMatrix(lightData.savedParentMatrix);
    zClass_NodePartial lightNode{};
    lightNode.flags = 0x11c;
    lightNode.nodeType = 0xff;
    lightNode.classId = 9;
    lightNode.classData = &lightData;
    lightNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);
    lightNode.cachedBounds[0] = 0.0f;
    lightNode.cachedBounds[1] = 0.0f;
    lightNode.cachedBounds[2] = 0.0f;
    lightNode.cachedBounds[3] = 1.0f;
    lightNode.cachedBounds[4] = 1.0f;
    lightNode.cachedBounds[5] = 1.0f;

    pickBuffer = {};
    g_DiPickCandidateBuffer = &pickBuffer;
    g_DiPickCandidateCursor = pickBuffer.entries;
    if (zClass_cls_di::BuildPickCandidatesForLight(&lightNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &lightNode ||
        zMath::g_currentMatrixPtrSlot != &matrixSlots[0]) {
        free_zclass_type_lists_for_test();
        return 10;
    }

    pickBuffer = {};
    g_DiPickCandidateBuffer = &pickBuffer;
    g_DiPickCandidateCursor = pickBuffer.entries;
    if (zClass_cls_di::BuildPickCandidateList(&animateNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &animateNode) {
        free_zclass_type_lists_for_test();
        return 11;
    }

    pickBuffer = {};
    g_DiPickCandidateBuffer = &pickBuffer;
    g_DiPickCandidateCursor = pickBuffer.entries;
    if (zClass_cls_di::BuildPickCandidateList(&lightNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &lightNode) {
        free_zclass_type_lists_for_test();
        return 12;
    }

    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area{};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *rows[1] = {&area};
    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    zClass_NodePartial world{};
    world.classData = &worldData;

    PlayerProbeSampleCandidateBuffer belowBuffer{};
    if (zClass_cls_di::BuildPickCandidateListBelowPoint(&world, &belowBuffer, 0.25f, 0.5f,
                                                        0.25f) != 0 ||
        belowBuffer.candidateCount != 1 || belowBuffer.entries[0].node != &objectNode ||
        belowBuffer.entries[0].hitPos.y != 0.0f) {
        free_zclass_type_lists_for_test();
        return 13;
    }

    belowBuffer = {};
    if (zClass_cls_di::BuildPickCandidateListBelowPoint(&world, &belowBuffer, 2.0f, 0.5f,
                                                        0.25f) != 1 ||
        belowBuffer.candidateCount != 0) {
        free_zclass_type_lists_for_test();
        return 14;
    }

    zVec3 highVertices[3] = {{0.0f, 0.25f, 0.0f}, {0.0f, 0.25f, 1.0f},
                             {1.0f, 0.25f, 0.0f}};
    zModel_MaterialPartial highMaterial{};
    zDiEntryPartial highEntry = entry;
    highEntry.material = &highMaterial;
    highEntry.variantTagInitialized = 1;
    highEntry.variantTag = 0x66;
    zDiPartial highDi = di;
    highDi.entries = &highEntry;
    highDi.verts = highVertices;
    zClass_NodePartial highNode = objectNode;
    highNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&highDi);
    highNode.cachedBounds[1] = 0.25f;
    highNode.cachedBounds[4] = 0.25f;
    zClass_NodePartial *bestChildren[2] = {&objectNode, &highNode};
    area.childCount = 2;
    area.childList = bestChildren;

    zVec3 bestPoint{0.25f, 0.5f, 0.25f};
    PlayerProbeSampleCandidateBuffer bestBuffer{};
    zClass_cls_di::FindBestPickCandidateBelowPoint(&world, &bestPoint, &bestBuffer);
    if (bestBuffer.candidateCount != 1 || bestBuffer.entries[0].node != &highNode ||
        bestBuffer.entries[0].hitPos.y != 0.25f ||
        bestBuffer.entries[0].variantTag.count != 1 ||
        bestBuffer.entries[0].variantTag.tags[0] != 0x66) {
        free_zclass_type_lists_for_test();
        return 16;
    }

    bestBuffer.entries[0].variantTag.count = 3;
    bestBuffer.entries[0].variantTag.tags[0] = 0x11;
    bestPoint.x = 2.0f;
    zClass_cls_di::FindBestPickCandidateBelowPoint(&world, &bestPoint, &bestBuffer);
    if (bestBuffer.candidateCount != 0 || bestBuffer.entries[0].variantTag.count != 0 ||
        bestBuffer.entries[0].variantTag.tags[0] != 0xff) {
        free_zclass_type_lists_for_test();
        return 17;
    }
    area.childCount = 1;
    area.childList = areaChildren;

    zClassDiPickCandidateEntry nearest{};
    g_zEffect_World = &world;
    const zVec3 nearestPoint{0.25f, 0.5f, 0.25f};
    if (zEffect::FindNearestPickCandidateBelowPoint(&nearestPoint, &nearest) != 1 ||
        nearest.node != &objectNode || nearest.hitPos.y != 0.0f) {
        g_zEffect_World = nullptr;
        free_zclass_type_lists_for_test();
        return 18;
    }

    g_zEffect_World = nullptr;
    free_zclass_type_lists_for_test();
    return 0;
}

extern "C" int zclass_cls_di_snap_probe_point_y_to_best_candidate_smoke() {
    reset_zclass_type_lists_for_test();

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zVec3 vertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                         {1.0f, 0.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_MaterialPartial material{};
    zDiEntryPartial entry{};
    entry.flagsAndIndexCount = 3;
    entry.vertexIndices = indices;
    entry.material = &material;
    entry.variantTagInitialized = 2;
    entry.variantTag = 0x44;
    entry.unknown_1a[0] = 0x55;

    zDiPartial di{};
    di.entryCount = 1;
    di.vertCount = 3;
    di.entries = &entry;
    di.verts = vertices;

    zClass_Object3DDataPartial objectData{};
    objectData.flags = 8;
    zClass_NodePartial objectNode{};
    objectNode.flags = 0x11c;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;

    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial area{};
    area.childCount = 1;
    area.childList = areaChildren;
    zWorldAreaPartial *rows[1] = {&area};
    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    zClass_NodePartial world{};
    world.classData = &worldData;

    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    g_Player_RuntimeDiScene = &world;

    zVec3 snapPoint{0.25f, 0.5f, 0.25f};
    if (zClass_cls_di::SnapProbePointYToBestCandidate(&snapPoint) != 0 ||
        std::fabs(snapPoint.y) > 0.0001f) {
        g_Player_RuntimeDiScene = oldRuntimeDiScene;
        free_zclass_type_lists_for_test();
        return 1;
    }

    snapPoint = {2.0f, 0.5f, 0.25f};
    if (zClass_cls_di::SnapProbePointYToBestCandidate(&snapPoint) != 1 ||
        std::fabs(snapPoint.y - 0.5f) > 0.0001f) {
        g_Player_RuntimeDiScene = oldRuntimeDiScene;
        free_zclass_type_lists_for_test();
        return 2;
    }

    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    free_zclass_type_lists_for_test();
    return 0;
}

extern "C" int zclass_cls_di_segment_batch_recursive_smoke() {
    auto setIdentityMatrix = [](float *matrixValues) {
        std::memset(matrixValues, 0, sizeof(zMat4x3));
        matrixValues[0] = 1.0f;
        matrixValues[4] = 1.0f;
        matrixValues[8] = 1.0f;
    };

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zVec3 vertices[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_PickFaceUvData faceUvData = {{{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 20.0f}}};
    zModel_PickFaceScenePayload facePayload{};
    zModel_PickFaceEntry faceEntry{};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = indices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;
    zModel_PickFaceData faceData{};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = vertices;

    zClass_DiSegmentEndpoints segments[1] = {};
    segments[0].start = {0.25f, 0.25f, 1.0f};
    segments[0].end = {0.25f, 0.25f, -1.0f};
    g_DiPickPointArray = &segments[0].start;
    g_DiPickPointCount = 1;
    g_DiSegmentBounds[0] = {0.25f, 0.25f, -1.0f, 0.25f, 0.25f, 1.0f};
    g_cls_di_BreakOnFirstCandidate = 0;
    g_cls_di_StopAfterFirstHit = 0;

    PlayerProbeSampleCandidateBuffer buckets[1] = {};
    g_DiPickCandidateBuffer = buckets;
    int activeMask[1] = {1};

    zClass_Object3DDataPartial objectData{};
    objectData.flags = 8;
    zClass_NodePartial objectNode{};
    objectNode.flags = 0x14;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForSegmentsRecursive(&objectNode, 1, activeMask) != 0 ||
        buckets[0].candidateCount != 1 || buckets[0].entries[0].node != &objectNode ||
        buckets[0].entries[0].scenePayload != &facePayload) {
        return 1;
    }

    buckets[0] = {};
    activeMask[0] = 1;
    zClass_AnimateDataPartial animateData{};
    setIdentityMatrix(animateData.savedParentMatrix);
    setIdentityMatrix(animateData.animatedTransform);
    zClass_NodePartial animateNode{};
    animateNode.flags = 0x14;
    animateNode.nodeType = 0xff;
    animateNode.classId = 8;
    animateNode.classData = &animateData;
    animateNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForSegmentsForAnimate(&animateNode, 1, activeMask) != 0 ||
        buckets[0].candidateCount != 1 || buckets[0].entries[0].node != &animateNode ||
        zMath::g_currentMatrixPtrSlot != &matrixSlots[0]) {
        return 2;
    }

    buckets[0] = {};
    activeMask[0] = 1;
    zClass_LightDataPartial lightData{};
    setIdentityMatrix(lightData.savedParentMatrix);
    zClass_NodePartial lightNode{};
    lightNode.flags = 0x14;
    lightNode.nodeType = 0xff;
    lightNode.classId = 9;
    lightNode.classData = &lightData;
    lightNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForSegmentsForLight(&lightNode, 1, activeMask) != 0) {
        return 3;
    }
    if (buckets[0].candidateCount != 1) {
        return 31;
    }
    if (buckets[0].entries[0].node != &lightNode || zMath::g_currentMatrixPtrSlot != &matrixSlots[0]) {
        return 32;
    }

    buckets[0] = {};
    activeMask[0] = 1;
    zClass_NodePartial *children[1] = {&objectNode};
    zClass_NodePartial parentNode{};
    parentNode.flags = 0x14;
    parentNode.nodeType = 0xff;
    parentNode.classId = 5;
    parentNode.classData = &objectData;
    parentNode.userDataOrDiRef = 0;
    parentNode.listCountB = 1;
    parentNode.listB = children;
    if (zClass_cls_di::BuildPickCandidatesForSegmentsRecursive(&parentNode, 1, activeMask) != 0 ||
        buckets[0].candidateCount != 1 || buckets[0].entries[0].node != &objectNode) {
        return 4;
    }

    buckets[0] = {};
    activeMask[0] = 1;
    objectNode.flags = 0x134;
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    g_DiSegmentBounds[0] = {2.0f, 0.25f, 0.25f, 3.0f, 0.75f, 0.75f};
    if (zClass_cls_di::BuildPickCandidatesForSegmentsRecursive(&objectNode, 2, activeMask) != 1 ||
        activeMask[0] != 1 || buckets[0].candidateCount != 0) {
        return 5;
    }

    return 0;
}

extern "C" int zclass_cls_di_segment_grid_window_smoke() {
    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zVec3 vertices[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_PickFaceUvData faceUvData = {{{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 20.0f}}};
    zModel_PickFaceScenePayload facePayload{};
    zModel_PickFaceEntry faceEntry{};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = indices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;
    zModel_PickFaceData faceData{};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = vertices;

    zClass_Object3DDataPartial objectData{};
    objectData.flags = 8;
    zClass_NodePartial objectNode{};
    objectNode.flags = 0x114;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = -10.0f;
    objectNode.cachedBounds[1] = -10.0f;
    objectNode.cachedBounds[2] = -10.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 10.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *children[1] = {&objectNode};
    zWorldAreaPartial areaCell{};
    areaCell.childCount = 1;
    areaCell.childList = children;
    zWorldAreaPartial *rows[1] = {&areaCell};
    zClass_WorldDataPartial worldData{};
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    zClass_NodePartial worldNode{};
    worldNode.classData = &worldData;

    zClass_DiSegmentEndpoints segments[1] = {};
    g_DiPickPointArray = &segments[0].start;
    g_DiPickPointCount = 1;
    g_cls_di_BreakOnFirstCandidate = 0;
    g_cls_di_StopAfterFirstHit = 0;
    int activeMask[1] = {1};

    PlayerProbeSampleCandidateBuffer buckets[1] = {};
    buckets[0].entries[0].hitPos.x = 99.0f;
    buckets[0].candidateCount = 1;
    g_DiPickCandidateBuffer = buckets;
    segments[0].start = {0.25f, 0.25f, 1.0f};
    segments[0].end = {0.25f, 0.25f, -1.0f};
    g_DiSegmentBounds[0] = {0.25f, 0.25f, -1.0f, 0.25f, 0.25f, 1.0f};
    zClass_cls_di::BuildPickCandidatesForSegmentsInGridWindow(&worldNode, activeMask);
    if (buckets[0].candidateCount != 2) {
        return 20 + buckets[0].candidateCount;
    }
    if (buckets[0].entries[1].node != &objectNode) {
        return 12;
    }
    if (buckets[0].entries[1].scenePayload != &facePayload) {
        return 13;
    }
    if (buckets[0].entries[1].hitPos.x != 0.25f) {
        return 14;
    }
    if (buckets[0].entries[1].hitPos.z != 0.0f) {
        return 15;
    }
    if (buckets[0].entries[0].hitPos.x != 99.0f) {
        return 16;
    }

    buckets[0] = {};
    buckets[0].entries[0].hitPos.x = 77.0f;
    buckets[0].candidateCount = 1;
    activeMask[0] = 1;
    worldData.clampQueriesToBounds = 1;
    segments[0].start = {-0.75f, 0.25f, 1.0f};
    segments[0].end = {-0.75f, 0.25f, -1.0f};
    g_DiSegmentBounds[0] = {-0.75f, 0.25f, -1.0f, -0.75f, 0.25f, 1.0f};
    zClass_cls_di::BuildPickCandidatesForSegmentsInGridWindow(&worldNode, activeMask);
    if (buckets[0].candidateCount != 3) {
        return 60 + buckets[0].candidateCount;
    }
    if (buckets[0].entries[0].hitPos.x != 77.0f) {
        return 62;
    }
    if (buckets[0].entries[1].node != &objectNode) {
        return 63;
    }
    if (buckets[0].entries[1].hitPos.x != -0.75f) {
        return 64;
    }
    if (buckets[0].entries[2].node != &objectNode ||
        buckets[0].entries[2].hitPos.x != -0.75f) {
        return 65;
    }
    if (segments[0].start.x != -0.75f) {
        return 66;
    }
    if (segments[0].end.x != -0.75f) {
        return 67;
    }
    if (g_DiSegmentBounds[0].minX != -0.75f || g_DiSegmentBounds[0].maxX != -0.75f) {
        return 68;
    }

    worldData.clampQueriesToBounds = 0;
    return 0;
}

extern "C" int zclass_cls_di_probe_hit_batches_for_segments_smoke() {
    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zVec3 vertices[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_PickFaceUvData faceUvData = {{{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 20.0f}}};
    zModel_PickFaceScenePayload facePayload{};
    zModel_PickFaceEntry faceEntry{};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = indices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;
    zModel_PickFaceData faceData{};
    faceData.faceCount = 1;
    faceData.vertexCount = 3;
    faceData.faces = &faceEntry;
    faceData.baseVertices = vertices;

    zClass_Object3DDataPartial objectData{};
    objectData.flags = 8;
    zClass_NodePartial objectNode{};
    objectNode.flags = 0x114;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = -10.0f;
    objectNode.cachedBounds[1] = -10.0f;
    objectNode.cachedBounds[2] = -10.0f;
    objectNode.cachedBounds[3] = 10.0f;
    objectNode.cachedBounds[4] = 10.0f;
    objectNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *children[1] = {&objectNode};
    zWorldAreaPartial areaCell{};
    zWorldAreaPartial *rows[1] = {&areaCell};
    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.worldMaxX = 1.0f;
    worldData.worldMaxZ = -2.0f;
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    zClass_NodePartial worldNode{};
    worldNode.classData = &worldData;
    worldNode.listCountB = 1;
    worldNode.listB = children;

    zClass_DiSegmentEndpoints segments[1] = {};
    segments[0].start = {0.25f, 0.25f, 1.0f};
    segments[0].end = {0.25f, 0.25f, -1.0f};

    PlayerProbeSampleCandidateBuffer buckets[1] = {};
    buckets[0].candidateCount = 9;
    g_cls_di_StopAfterFirstHit = 0;
    g_cls_di_BreakOnFirstCandidate = 0;
    g_DiPickCandidateBuffer = nullptr;
    g_DiPickPointCount = -1;
    zClass_cls_di::BuildProbeHitBatchesForSegments(&worldNode, segments, 2, buckets);
    if (g_DiPickCandidateBuffer != buckets) {
        return 30;
    }
    if (g_DiPickPointCount != 1) {
        return 31;
    }
    if (g_DiSegmentBounds[0].minX != 0.25f) {
        return 18;
    }
    if (g_DiSegmentBounds[0].minZ != -1.0f || g_DiSegmentBounds[0].maxZ != 1.0f) {
        return 19;
    }
    if (buckets[0].candidateCount != 1) {
        return 10 + buckets[0].candidateCount;
    }
    if (buckets[0].entries[0].node != &objectNode) {
        return 12;
    }
    if (buckets[0].entries[0].scenePayload != &facePayload) {
        return 13;
    }
    if (g_DiPickPointArray != &segments[0].start) {
        return 16;
    }
    if (g_cls_di_StopAfterFirstHit != 0) {
        return 17;
    }

    buckets[0] = {};
    buckets[0].candidateCount = 5;
    worldData.clampQueriesToBounds = 0;
    worldData.worldMaxX = 0.0f;
    g_cls_di_StopAfterFirstHit = 3;
    zClass_cls_di::BuildProbeHitBatchesForSegments(&worldNode, segments, 2, buckets);
    if (buckets[0].candidateCount != 0 || g_cls_di_StopAfterFirstHit != 0) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_cls_di_set_stop_after_first_hit_smoke() {
    g_cls_di_BreakOnFirstCandidate = 0;
    zClass_cls_di::SetBreakOnFirstCandidate(5);
    if (g_cls_di_BreakOnFirstCandidate != 5) {
        return 18;
    }

    g_cls_di_StopAfterFirstHit = 0;
    zClass_cls_di::SetStopAfterFirstHit(7);

    if (g_cls_di_StopAfterFirstHit != 7) {
        return 1;
    }

    zVec3 triZ[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    zVec3 segmentStart{0.25f, 0.25f, 1.0f};
    zVec3 segmentEnd{0.25f, 0.25f, -1.0f};
    zClassDiPickCandidateEntry candidate{};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triZ, 3, 0) != 1 ||
        candidate.surfaceNormal.x != 0.0f || candidate.surfaceNormal.y != 0.0f ||
        candidate.surfaceNormal.z != 1.0f || candidate.hitPos.x != 0.25f ||
        candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.0f) {
        return 2;
    }

    segmentStart = {1.25f, 0.25f, 1.0f};
    segmentEnd = {1.25f, 0.25f, -1.0f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triZ, 3, 0) != 0) {
        return 3;
    }

    segmentStart = {0.25f, 0.25f, -1.0f};
    segmentEnd = {0.25f, 0.25f, 1.0f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triZ, 3, 0) != 0 ||
        zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triZ, 3, 1) != 1) {
        return 4;
    }

    zVec3 triNegZ[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triNegZ, 3, 0) != 1 ||
        candidate.surfaceNormal.z != -1.0f) {
        return 5;
    }

    zVec3 triX[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    segmentStart = {1.0f, 0.25f, 0.25f};
    segmentEnd = {-1.0f, 0.25f, 0.25f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triX, 3, 0) != 1 ||
        candidate.surfaceNormal.x != 1.0f || candidate.hitPos.x != 0.0f ||
        candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.25f) {
        return 6;
    }

    zVec3 boxCornerValues[8] = {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f},
                                {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
                                {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    zBBoxCorners boxCorners{};
    for (std::int32_t i = 0; i < 8; ++i) {
        boxCorners.values[i * 3 + 0] = boxCornerValues[i].x;
        boxCorners.values[i * 3 + 1] = boxCornerValues[i].y;
        boxCorners.values[i * 3 + 2] = boxCornerValues[i].z;
    }

    candidate.scenePayload = &candidate;
    segmentStart = {0.5f, 0.5f, 2.0f};
    segmentEnd = {0.5f, 0.5f, 0.5f};
    if (zClass_cls_di::BuildPickCandidatesForSegmentVsBBoxFaces(&boxCorners, &candidate,
                                                                &segmentStart, &segmentEnd) != 1 ||
        candidate.scenePayload != nullptr || candidate.surfaceNormal.z != 1.0f ||
        candidate.hitPos.x != 0.5f || candidate.hitPos.y != 0.5f || candidate.hitPos.z != 1.0f ||
        g_zClass_DiFaceVertexScratch4[1].x != 1.0f || g_zClass_DiFaceVertexScratch4[2].y != 1.0f) {
        return 7;
    }

    candidate.scenePayload = &candidate;
    segmentStart = {2.0f, 2.0f, 2.0f};
    segmentEnd = {2.0f, 2.0f, -1.0f};
    if (zClass_cls_di::BuildPickCandidatesForSegmentVsBBoxFaces(&boxCorners, &candidate,
                                                                &segmentStart, &segmentEnd) != 0 ||
        candidate.scenePayload != nullptr) {
        return 8;
    }

    static std::int32_t filterMatrixFlags[8];
    static float *filterMatrixSlots[8];
    static zMat4x3 filterMatrix;
    filterMatrixFlags[0] = 1;
    filterMatrixSlots[0] = reinterpret_cast<float *>(&filterMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &filterMatrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &filterMatrixSlots[0];

    zClass_NodePartial filterNode{};
    std::int32_t filterClassData = 0;
    filterNode.classId = 2;
    filterNode.classData = &filterClassData;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 1) {
        return 9;
    }

    filterNode.flags = 0x100;
    filterNode.cachedBounds[0] = 0.0f;
    filterNode.cachedBounds[1] = 0.0f;
    filterNode.cachedBounds[2] = 0.0f;
    filterNode.cachedBounds[3] = 1.0f;
    filterNode.cachedBounds[4] = 1.0f;
    filterNode.cachedBounds[5] = 1.0f;
    g_DiSegmentMinX = 0.25f;
    g_DiSegmentMinY = 0.25f;
    g_DiSegmentMinZ = 0.25f;
    g_DiSegmentMaxX = 0.75f;
    g_DiSegmentMaxY = 0.75f;
    g_DiSegmentMaxZ = 0.75f;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 0) {
        return 10;
    }

    std::strcpy(filterNode.name, "regionTarget");
    filterNode.flags = 0x144;
    filterNode.nodeType = 0xff;
    OptCatalogRaycastHitList regionHits{};
    zVec3 regionCenter{0.5f, 0.5f, 0.5f};
    g_zClass_cls_di_FilterRegions_OutHitList = &regionHits;
    g_zClass_cls_di_FilterRegions_NodeNamePrefix = "region";
    g_zClass_cls_di_FilterRegions_Center = &regionCenter;
    g_zClass_cls_di_FilterRegions_RadiusSq = 1.0f;
    g_zClass_cls_di_FilterRegions_EnableClearanceCheck = 1;
    g_zClass_cls_di_FilterRegions_LineOfSightWorld = nullptr;
    if (zClass_cls_di::FilterRegions_TryAppendNode(&filterNode) != 0 ||
        regionHits.hitCount != 1 || regionHits.hits[0].hitNode != &filterNode ||
        regionHits.hits[0].pos.x != 0.5f || regionHits.hits[0].pos.y != 0.5f ||
        regionHits.hits[0].pos.z != 0.5f || regionHits.hits[0].distance != 0.0f ||
        regionHits.hits[0].surfaceRef != nullptr) {
        return 93;
    }
    g_zClass_cls_di_FilterRegions_NodeNamePrefix = "other";
    if (zClass_cls_di::FilterRegions_TryAppendNode(&filterNode) != 1 ||
        regionHits.hitCount != 1) {
        return 94;
    }

    zClass_NodePartial *regionChildren[1] = {&filterNode};
    zWorldAreaPartial regionArea{};
    regionArea.childCount = 1;
    regionArea.childList = regionChildren;
    zWorldAreaPartial *regionRows[1] = {&regionArea};
    zClass_WorldDataPartial regionWorldData{};
    regionWorldData.originX = 0.0f;
    regionWorldData.originZ = -2.0f;
    regionWorldData.worldMaxX = 2.0f;
    regionWorldData.worldMaxZ = 2.0f;
    regionWorldData.areaInvSizeX = 0.5f;
    regionWorldData.areaInvSizeZ = 0.25f;
    regionWorldData.areaGridColCount = 1;
    regionWorldData.areaGridRowCount = 1;
    regionWorldData.areaGridRows = regionRows;
    zClass_NodePartial regionWorld{};
    regionWorld.classData = &regionWorldData;
    regionHits = {};
    regionCenter = {0.5f, 0.5f, -3.0f};
    if (zClass_cls_di::FilterRegionsAgainstSphere(&regionWorld, &regionCenter, "region", 0.5f, 0,
                                                  0, &regionHits) != 0 ||
        regionHits.hitCount != 1 || regionHits.hits[0].hitNode != &filterNode ||
        regionHits.hits[0].pos.x != 0.5f || regionHits.hits[0].pos.y != 0.5f ||
        regionHits.hits[0].pos.z != 0.5f || regionHits.hits[0].distance != 0.0f) {
        return 95;
    }

    g_zClass_cls_di_FilterRegions_OutHitList = nullptr;
    g_zClass_cls_di_FilterRegions_NodeNamePrefix = nullptr;
    g_zClass_cls_di_FilterRegions_Center = nullptr;
    g_zClass_cls_di_FilterRegions_EnableClearanceCheck = 0;

    filterNode.flags = 0x100;
    zVec3 pickPoints[3] = {{0.5f, 99.0f, 0.5f}, {1.5f, 0.0f, 0.5f}, {0.25f, 0.0f, 2.0f}};
    int pickHitFlags[3] = {1, 1, 0};
    g_DiPickPointArray = pickPoints;
    g_DiPickPointCount = 3;
    if (zClass_cls_di::PickTestBBox2D(&filterNode, pickHitFlags) != 0 ||
        pickHitFlags[0] != 1 || pickHitFlags[1] != 0 || pickHitFlags[2] != 0) {
        return 90;
    }
    pickHitFlags[0] = 0;
    pickHitFlags[1] = 1;
    pickHitFlags[2] = 1;
    if (zClass_cls_di::PickTestBBox2D(&filterNode, pickHitFlags) != 1 ||
        pickHitFlags[0] != 0 || pickHitFlags[1] != 0 || pickHitFlags[2] != 0) {
        return 91;
    }
    filterNode.flags = 0;
    pickHitFlags[0] = 1;
    if (zClass_cls_di::PickTestBBox2D(&filterNode, pickHitFlags) != 1 ||
        pickHitFlags[0] != 1) {
        return 92;
    }
    filterNode.flags = 0x100;

    g_DiSegmentMaxX = 0.0f;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 1) {
        return 11;
    }

    filterNode.flags = 0x120;
    g_DiPickCandidateCursor = &candidate;
    g_DiPickQueryPoint = {0.5f, 0.5f, 2.0f};
    g_DiSegmentEnd = {0.5f, 0.5f, 0.5f};
    g_DiSegmentMinX = 0.5f;
    g_DiSegmentMinY = 0.5f;
    g_DiSegmentMinZ = 0.5f;
    g_DiSegmentMaxX = 0.5f;
    g_DiSegmentMaxY = 0.5f;
    g_DiSegmentMaxZ = 2.0f;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 0 || candidate.hitPos.z != 1.0f) {
        return 12;
    }

    candidate.scenePayload = &candidate;
    g_DiPickQueryPoint = {0.5f, 0.5f, 0.5f};
    g_DiSegmentEnd = {0.75f, 0.75f, 0.75f};
    g_DiSegmentMinX = 0.5f;
    g_DiSegmentMinY = 0.5f;
    g_DiSegmentMinZ = 0.5f;
    g_DiSegmentMaxX = 0.75f;
    g_DiSegmentMaxY = 0.75f;
    g_DiSegmentMaxZ = 0.75f;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 1 ||
        candidate.scenePayload != nullptr) {
        return 13;
    }

    zModel_PickFaceUvData faceUvData = {{{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 20.0f}}};
    zVec2 outUv{};
    g_OptCatalogDamageMaskPhaseU = -1.0f;
    g_OptCatalogDamageMaskPhaseV = -1.0f;
    segmentStart = {0.25f, 0.25f, 1.0f};
    segmentEnd = {0.25f, 0.25f, -1.0f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv(
            &candidate, &segmentStart, &segmentEnd, triZ, &faceUvData, &outUv, 3, 0) != 1 ||
        candidate.hitPos.x != 0.25f || candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.0f ||
        outUv.x != 2.5f || outUv.y != 5.0f || g_OptCatalogDamageMaskPhaseU != 2.5f ||
        g_OptCatalogDamageMaskPhaseV != 5.0f) {
        return 14;
    }

    zModel_PickFaceUvData xFaceUvData = {{{0.0f, 0.0f}, {4.0f, 0.0f}, {0.0f, 8.0f}}};
    outUv = {};
    segmentStart = {1.0f, 0.25f, 0.25f};
    segmentEnd = {-1.0f, 0.25f, 0.25f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv(
            &candidate, &segmentStart, &segmentEnd, triX, &xFaceUvData, &outUv, 3, 0) != 1 ||
        candidate.hitPos.x != 0.0f || candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.25f ||
        outUv.x != 1.0f || outUv.y != 2.0f || g_OptCatalogDamageMaskPhaseU != 1.0f ||
        g_OptCatalogDamageMaskPhaseV != 2.0f) {
        return 15;
    }

    g_OptCatalogDamageMaskPhaseU = 123.0f;
    g_OptCatalogDamageMaskPhaseV = 456.0f;
    segmentStart = {1.25f, 0.25f, 1.0f};
    segmentEnd = {1.25f, 0.25f, -1.0f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv(
            &candidate, &segmentStart, &segmentEnd, triZ, &faceUvData, &outUv, 3, 0) != 0 ||
        g_OptCatalogDamageMaskPhaseU != 123.0f || g_OptCatalogDamageMaskPhaseV != 456.0f) {
        return 16;
    }

    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    zModel_PickFaceScenePayload facePayload{};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_PickFaceEntry faceEntry{};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;

    zModel_PickFaceData faceData{};
    faceData.faceCount = 1;
    faceData.faces = &faceEntry;
    faceData.baseVertices = triZ;
    filterMatrixFlags[0] = 1;
    candidate = {};
    segmentStart = {0.25f, 0.25f, 1.0f};
    segmentEnd = {0.25f, 0.25f, -1.0f};
    if (zClass_cls_di::AppendPickCandidatesForFace(&faceData, &candidate, &segmentStart,
                                                   &segmentEnd) != 1 ||
        candidate.scenePayload != &facePayload || candidate.hitPos.x != 0.25f ||
        candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.0f ||
        candidate.surfaceNormal.z != 1.0f) {
        return 17;
    }

    facePayload.flags = 0x0200;
    g_OptCatalogDamageMaskPhaseU = -1.0f;
    g_OptCatalogDamageMaskPhaseV = -1.0f;
    candidate = {};
    if (zClass_cls_di::AppendPickCandidatesForFace(&faceData, &candidate, &segmentStart,
                                                   &segmentEnd) != 1 ||
        candidate.scenePayload != &facePayload || g_OptCatalogDamageMaskPhaseU != 2.5f ||
        g_OptCatalogDamageMaskPhaseV != 5.0f) {
        return 18;
    }

    facePayload.flags = 0;
    zVec3 morphDelta[3] = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    faceData.flags = 8;
    faceData.morphVertexCount = 3;
    faceData.morphWeight = 0.5f;
    faceData.morphVertices = morphDelta;
    candidate = {};
    if (zClass_cls_di::AppendPickCandidatesForFace(&faceData, &candidate, &segmentStart,
                                                   &segmentEnd) != 1 ||
        candidate.scenePayload != &facePayload || candidate.hitPos.z != 0.5f ||
        g_zModel_SharedVec3ScratchA[0].z != 0.5f || g_zModel_SharedVec3ScratchA[1].z != 0.5f ||
        g_zModel_SharedVec3ScratchA[2].z != 0.5f) {
        return 19;
    }

    faceData.flags = 0;
    faceData.morphVertexCount = 0;
    faceData.morphWeight = 0.0f;
    faceData.morphVertices = nullptr;
    filterMatrixFlags[0] = 0;
    filterMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 10.0f, 0.0f, 0.0f};
    candidate = {};
    segmentStart = {10.25f, 0.25f, 1.0f};
    segmentEnd = {10.25f, 0.25f, -1.0f};
    if (zClass_cls_di::AppendPickCandidatesForFace(&faceData, &candidate, &segmentStart,
                                                   &segmentEnd) != 1 ||
        candidate.scenePayload != &facePayload || candidate.hitPos.x != 10.25f ||
        candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.0f ||
        candidate.surfaceNormal.x != 0.0f || candidate.surfaceNormal.y != 0.0f ||
        candidate.surfaceNormal.z != 1.0f) {
        return 20;
    }

    zVec3 probeFaceVertices[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                                  {1.0f, 0.0f, 0.0f}};
    zVec3 probeSamples[3] = {{0.25f, 99.0f, 0.25f}, {1.25f, 0.0f, 0.25f},
                             {0.25f, 0.0f, 0.25f}};
    int probeSampleMask[3] = {1, 1, 0};
    PlayerProbeSampleCandidateBuffer probeBuckets[3] = {};
    faceEntry.variantTag.count = 2;
    faceEntry.variantTag.tags[0] = 0x44;
    faceEntry.variantTag.tags[1] = 0x55;
    zModelConst::AddFaceToPlayerProbeSampleBuckets(&filterNode, probeBuckets, probeSamples,
                                                   probeSampleMask, 3, 0.5f, probeFaceVertices,
                                                   &faceEntry);
    if (probeBuckets[0].candidateCount != 1 || probeBuckets[1].candidateCount != 0 ||
        probeBuckets[2].candidateCount != 0 || probeBuckets[0].entries[0].node != &filterNode ||
        probeBuckets[0].entries[0].scenePayload != &facePayload ||
        probeBuckets[0].entries[0].variantTag.count != 2 ||
        probeBuckets[0].entries[0].variantTag.tags[1] != 0x55 ||
        probeBuckets[0].entries[0].surfaceNormal.y != 1.0f ||
        probeBuckets[0].entries[0].hitPos.y != 0.0f) {
        return 201;
    }

    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;
    faceData.vertexCount = 3;
    faceData.baseVertices = probeFaceVertices;
    probeBuckets[0] = {};
    probeBuckets[1] = {};
    probeBuckets[2] = {};
    filterMatrixFlags[0] = 1;
    zClass_cls_di::PickTestMeshAtQueryXZ(&filterNode, &faceData, probeSamples, probeSampleMask, 3,
                                         0.5f, probeBuckets);
    if (probeBuckets[0].candidateCount != 1 || probeBuckets[0].entries[0].node != &filterNode ||
        probeBuckets[0].entries[0].scenePayload != &facePayload ||
        probeBuckets[0].entries[0].hitPos.y != 0.0f || probeBuckets[1].candidateCount != 0) {
        return 202;
    }

    zClass_Object3DDataPartial batchObjectData{};
    batchObjectData.flags = 8;
    zClass_NodePartial batchObjectNode{};
    batchObjectNode.flags = 0x11c;
    batchObjectNode.nodeType = 0xff;
    batchObjectNode.classId = 5;
    batchObjectNode.classData = &batchObjectData;
    batchObjectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    batchObjectNode.cachedBounds[0] = 0.0f;
    batchObjectNode.cachedBounds[1] = 0.0f;
    batchObjectNode.cachedBounds[2] = 0.0f;
    batchObjectNode.cachedBounds[3] = 1.0f;
    batchObjectNode.cachedBounds[4] = 1.0f;
    batchObjectNode.cachedBounds[5] = 1.0f;
    zClass_NodePartial *batchChildren[] = {&batchObjectNode};
    zWorldAreaPartial batchArea{};
    batchArea.childCount = 1;
    batchArea.childList = batchChildren;
    zWorldAreaPartial *batchRows[] = {&batchArea};
    zClass_WorldDataPartial batchWorldData{};
    batchWorldData.clampQueriesToBounds = 1;
    batchWorldData.areaCellSizeX = 1.0f;
    batchWorldData.areaCellSizeZ = 1.0f;
    batchWorldData.areaInvSizeX = 1.0f;
    batchWorldData.areaInvSizeZ = 1.0f;
    batchWorldData.areaGridColCount = 1;
    batchWorldData.areaGridRowCount = 1;
    batchWorldData.areaGridRows = batchRows;
    zClass_NodePartial batchWorld{};
    batchWorld.classData = &batchWorldData;
    zVec3 batchPoints[2] = {{0.25f, 99.0f, 0.25f}, {1.25f, 99.0f, 0.25f}};
    PlayerProbeSampleCandidateBuffer batchBuckets[2] = {};
    faceData.baseVertices = probeFaceVertices;
    zClass_cls_di::BuildPickCandidatesForPointBatch(&batchWorld, batchPoints, 2, 0.5f,
                                                    batchBuckets);
    if (batchBuckets[0].candidateCount != 1 || batchBuckets[1].candidateCount != 1 ||
        batchBuckets[0].entries[0].node != &batchObjectNode ||
        batchBuckets[1].entries[0].node != &batchObjectNode ||
        batchBuckets[0].entries[0].hitPos.y != 0.0f ||
        batchBuckets[1].entries[0].hitPos.y != 0.0f || batchPoints[1].x != 1.25f) {
        return 203;
    }
    faceData.baseVertices = triZ;

    auto setIdentityMatrix = [](float *matrixValues) {
        std::memset(matrixValues, 0, sizeof(zMat4x3));
        matrixValues[0] = 1.0f;
        matrixValues[4] = 1.0f;
        matrixValues[8] = 1.0f;
    };

    faceData.baseVertices = probeFaceVertices;
    zVec3 pointBatchSamples[2] = {{0.25f, 99.0f, 0.25f}, {0.6f, 99.0f, 0.2f}};
    int pointBatchMask[2] = {1, 1};
    PlayerProbeSampleCandidateBuffer pointBatchBuckets[2] = {};
    g_DiPickPointArray = pointBatchSamples;
    g_DiPickPointCount = 2;
    g_DiPickPointQueryMaxY = 0.5f;
    g_DiPickCandidateBuffer = pointBatchBuckets;
    filterMatrixFlags[0] = 1;
    filterMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};

    zClass_AnimateDataPartial pointAnimateData{};
    setIdentityMatrix(pointAnimateData.savedParentMatrix);
    setIdentityMatrix(pointAnimateData.animatedTransform);
    zClass_NodePartial pointAnimateNode{};
    pointAnimateNode.flags = 0x14;
    pointAnimateNode.nodeType = 0xff;
    pointAnimateNode.classId = 8;
    pointAnimateNode.classData = &pointAnimateData;
    pointAnimateNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForPointsRecursive(&pointAnimateNode, 1,
                                                             pointBatchMask) != 0 ||
        pointBatchBuckets[0].candidateCount != 1 ||
        pointBatchBuckets[1].candidateCount != 1 ||
        pointBatchBuckets[0].entries[0].node != &pointAnimateNode ||
        pointBatchBuckets[1].entries[0].node != &pointAnimateNode ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0]) {
        return 204;
    }

    pointBatchBuckets[0] = {};
    pointBatchBuckets[1] = {};
    pointBatchMask[0] = 1;
    pointBatchMask[1] = 1;
    zClass_LightDataPartial pointLightData{};
    setIdentityMatrix(pointLightData.savedParentMatrix);
    zClass_NodePartial pointLightNode{};
    pointLightNode.flags = 0x14;
    pointLightNode.nodeType = 0xff;
    pointLightNode.classId = 9;
    pointLightNode.classData = &pointLightData;
    pointLightNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForPointsForLight(&pointLightNode, 1,
                                                            pointBatchMask) != 0 ||
        pointBatchBuckets[0].candidateCount != 1 ||
        pointBatchBuckets[1].candidateCount != 1 ||
        pointBatchBuckets[0].entries[0].node != &pointLightNode ||
        pointBatchBuckets[1].entries[0].node != &pointLightNode ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0]) {
        return 205;
    }
    faceData.baseVertices = triZ;

    PlayerProbeSampleCandidateBuffer pickBuffer{};
    g_DiPickCandidateBuffer = &pickBuffer;
    g_DiPickCandidateCursor = pickBuffer.entries;
    g_cls_di_BreakOnFirstCandidate = 0;
    g_cls_di_StopAfterFirstHit = 0;
    g_Variant_CurrentTag = {};
    filterMatrixFlags[0] = 1;
    filterMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    zMath::g_currentMatrixIdentityFlagSlot = &filterMatrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &filterMatrixSlots[0];
    g_DiPickQueryPoint = {0.25f, 0.25f, 1.0f};
    g_DiSegmentEnd = {0.25f, 0.25f, -1.0f};

    zClass_Object3DDataPartial objectData{};
    objectData.flags = 8;
    zClass_NodePartial objectNode{};
    objectNode.flags = 0x114;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&objectNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || g_DiPickCandidateCursor != &pickBuffer.entries[1] ||
        pickBuffer.entries[0].node != &objectNode ||
        pickBuffer.entries[0].scenePayload != &facePayload ||
        pickBuffer.entries[0].hitPos.z != 0.0f) {
        return 21;
    }

    g_cls_di_BreakOnFirstCandidate = 1;
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&objectNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || g_DiPickCandidateCursor != &pickBuffer.entries[1]) {
        return 22;
    }
    g_cls_di_BreakOnFirstCandidate = 0;

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_AnimateDataPartial animateData{};
    setIdentityMatrix(animateData.savedParentMatrix);
    setIdentityMatrix(animateData.animatedTransform);
    zClass_NodePartial animateNode{};
    animateNode.flags = 0x14;
    animateNode.nodeType = 0xff;
    animateNode.classId = 8;
    animateNode.classData = &animateData;
    animateNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&animateNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &animateNode ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0]) {
        return 23;
    }

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_CameraDataPartial cameraData{};
    setIdentityMatrix(cameraData.worldTransform);
    zClass_NodePartial cameraNode{};
    cameraNode.flags = 0x14;
    cameraNode.nodeType = 0xff;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&cameraNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &cameraNode ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0]) {
        return 24;
    }

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_LightDataPartial lightData{};
    setIdentityMatrix(lightData.savedParentMatrix);
    zClass_NodePartial lightNode{};
    lightNode.flags = 0x14;
    lightNode.nodeType = 0xff;
    lightNode.classId = 9;
    lightNode.classData = &lightData;
    lightNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&lightNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &lightNode ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0]) {
        return 25;
    }

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_SequenceDataPartial sequenceData{};
    sequenceData.isActive = 1;
    sequenceData.currentIndex = 0;
    sequenceData.entries[0].node = &objectNode;
    zClass_NodePartial sequenceNode{};
    sequenceNode.flags = 0x14;
    sequenceNode.nodeType = 0xff;
    sequenceNode.classId = 7;
    sequenceNode.classData = &sequenceData;
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&sequenceNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &objectNode) {
        return 26;
    }

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_NodePartial *segmentChildren[1] = {&objectNode};
    zClass_NodePartial segmentRoot{};
    segmentRoot.listCountB = 1;
    segmentRoot.listB = segmentChildren;
    if (zClass_cls_di::BuildPickCandidatesForSegment(&segmentRoot) != 1 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &objectNode) {
        return 27;
    }

    PlayerProbeSampleCandidateBuffer rayBuffer{};
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial areaCells[1] = {};
    areaCells[0].childCount = 1;
    areaCells[0].childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {areaCells};
    zClass_WorldDataPartial rayWorldData{};
    rayWorldData.originX = 0.0f;
    rayWorldData.originZ = -2.0f;
    rayWorldData.worldMaxX = 2.0f;
    rayWorldData.worldMaxZ = 2.0f;
    rayWorldData.areaCellSizeX = 2.0f;
    rayWorldData.areaCellSizeZ = 4.0f;
    rayWorldData.areaInvSizeX = 0.5f;
    rayWorldData.areaInvSizeZ = 0.25f;
    rayWorldData.areaGridColCount = 1;
    rayWorldData.areaGridRowCount = 1;
    rayWorldData.areaGridRows = areaRows;
    zClass_NodePartial rayWorld{};
    rayWorld.classData = &rayWorldData;
    if (zClass_cls_di::RaycastFindClosest(&rayWorld, &rayBuffer, 0.25f, 0.25f, 1.0f, 0.25f, 0.25f,
                                          -1.0f) != 0 ||
        rayBuffer.candidateCount != 1 || rayBuffer.entries[0].node != &objectNode ||
        rayBuffer.entries[0].hitPos.z != 0.0f ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0] || g_cls_di_StopAfterFirstHit != 0) {
        return 28;
    }

    zVec3 rayStart{0.25f, 0.25f, 1.0f};
    zVec3 rayEnd{0.25f, 0.25f, -1.0f};
    rayBuffer = {};
    if (zClass_cls_di::RaycastSelectClosestHitBetweenPoints(&rayWorld, &rayStart, &rayEnd,
                                                            &rayBuffer) != 0 ||
        rayBuffer.candidateCount != 0 || rayBuffer.entries[0].node != &objectNode) {
        return 29;
    }

    facePayload.flags = 0;
    zVec3 selectTris[3][3] = {
        {
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        },
        {
            {0.0f, 0.0f, 0.75f},
            {1.0f, 0.0f, 0.75f},
            {0.0f, 1.0f, 0.75f},
        },
        {
            {0.0f, 0.0f, -0.5f},
            {1.0f, 0.0f, -0.5f},
            {0.0f, 1.0f, -0.5f},
        },
    };
    zModel_PickFaceEntry selectFaces[3] = {};
    zModel_PickFaceData selectFaceData[3] = {};
    zClass_NodePartial selectNodes[3] = {};
    for (std::int32_t i = 0; i < 3; ++i) {
        selectFaces[i].flagsAndVertexCount = 3;
        selectFaces[i].vertexIndices = faceIndices;
        selectFaces[i].faceUvData = &faceUvData;
        selectFaces[i].scenePayload = &facePayload;
        selectFaceData[i].faceCount = 1;
        selectFaceData[i].faces = &selectFaces[i];
        selectFaceData[i].baseVertices = selectTris[i];

        selectNodes[i].flags = 0x114;
        selectNodes[i].nodeType = 0xff;
        selectNodes[i].classId = 5;
        selectNodes[i].classData = &objectData;
        selectNodes[i].userDataOrDiRef = reinterpret_cast<std::uint32_t>(&selectFaceData[i]);
        selectNodes[i].cachedBounds[0] = 0.0f;
        selectNodes[i].cachedBounds[1] = 0.0f;
        selectNodes[i].cachedBounds[2] = -1.0f;
        selectNodes[i].cachedBounds[3] = 1.0f;
        selectNodes[i].cachedBounds[4] = 1.0f;
        selectNodes[i].cachedBounds[5] = 1.0f;
    }

    zClass_NodePartial *selectChildren[3] = {&selectNodes[0], &selectNodes[1], &selectNodes[2]};
    zClass_WorldDataPartial selectWorldData{};
    zClass_NodePartial selectWorld{};
    selectWorld.classData = &selectWorldData;
    selectWorld.listCountB = 3;
    selectWorld.listB = selectChildren;
    PlayerProbeSampleCandidateBuffer selectRayBuffer{};
    rayStart = {0.25f, 0.25f, 2.0f};
    rayEnd = {0.25f, 0.25f, -2.0f};
    if (zClass_cls_di::RaycastSelectClosestHitBetweenPoints(&selectWorld, &rayStart, &rayEnd,
                                                            &selectRayBuffer) != 0 ||
        selectRayBuffer.candidateCount != 1 || selectRayBuffer.entries[1].node != &selectNodes[1] ||
        selectRayBuffer.entries[1].hitPos.z != 0.75f) {
        return 30;
    }

    zClass_NodePartial emptyWorld{};
    emptyWorld.classData = &selectWorldData;
    PlayerProbeSampleCandidateBuffer emptyRayBuffer{};
    if (zClass_cls_di::RaycastSelectClosestHitBetweenPoints(&emptyWorld, &rayStart, &rayEnd,
                                                            &emptyRayBuffer) != 1 ||
        emptyRayBuffer.candidateCount != 0) {
        return 31;
    }

    g_DiPickCandidateBuffer = nullptr;
    g_DiPickCandidateCursor = nullptr;

    return 0;
}

extern "C" int zclass_node_propagate_transform_dirty_smoke() {
    std::int32_t parentObjectFlags = 0;
    std::int32_t childObjectFlags = 0;
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    zClass_NodePartial skippedChild{};
    zClass_NodePartial *children[2] = {&child, &skippedChild};
    parent.classId = 5;
    parent.classData = &parentObjectFlags;
    parent.listCountB = 2;
    parent.listB = children;
    child.classId = 5;
    child.classData = &childObjectFlags;
    skippedChild.flags = 0x02000000;

    zClass_Node::PropagateTransformDirtyRecursive(&parent);

    return (parentObjectFlags & 0x20) != 0 && (childObjectFlags & 0x20) != 0 &&
                   (parent.boundsFlags & 4) != 0 && (parent.flags & 0x02000000) != 0 &&
                   (child.flags & 0x02000000) != 0 && skippedChild.boundsFlags == 0
               ? 0
               : 1;
}

extern "C" int player_add_scaled_hud_counter_smoke() {
    g_Player_HudCounterValue = 10;
    g_HudSensorTracker.primaryGunDispatchCount = 0;
    g_OptCatalog_DamageFeedbackHitCount = 7;

    Player::AddScaledHudCounterValue(1.25f);
    if (g_Player_HudCounterValue != 1260) {
        return 1;
    }

    g_HudSensorTracker.primaryGunDispatchCount = 4;
    g_OptCatalog_DamageFeedbackHitCount = 3;
    Player::AddScaledHudCounterValue(2.0f);
    if (g_Player_HudCounterValue != 2760) {
        return 2;
    }

    g_HudSensorTracker.primaryGunDispatchCount = 4;
    g_OptCatalog_DamageFeedbackHitCount = 1;
    Player::AddScaledHudCounterValue(-1.5f);
    return g_Player_HudCounterValue == 2385 ? 0 : 3;
}

extern "C" int zutil_save_game_state_list_smoke() {
    zUtil_SaveGameState list{};
    list.next = reinterpret_cast<zUtil_SaveGameState *>(0x1);
    list.firstSaveState = reinterpret_cast<zUtil_SaveGameState *>(0x2);
    list.saveStateListHead = reinterpret_cast<zUtil_SaveGameState *>(0x3);
    list.saveStateListTail = reinterpret_cast<zUtil_SaveGameState *>(0x4);
    list.saveStateCount = 5;

    if (zUtil_SaveGameStateList_Init(&list) != &list || list.next != nullptr ||
        list.firstSaveState != nullptr || list.saveStateListHead != nullptr ||
        list.saveStateListTail != nullptr || list.saveStateCount != 0 ||
        list.playerState == nullptr) {
        return 1;
    }

    for (std::uint8_t value : list.playerState->bytes) {
        if (value != 0) {
            std::free(list.playerState);
            return 2;
        }
    }

    zUtil_SaveGameState *const first = zUtil_SaveGameStateList_AllocAppend(&list);
    zUtil_SaveGameState *const second = zUtil_SaveGameStateList_AllocAppend(&list);
    const bool linked = first != nullptr && second != nullptr && first->next == second &&
                        second->next == nullptr && list.firstSaveState == first &&
                        list.saveStateListHead == first && list.saveStateListTail == second &&
                        list.saveStateCount == 2;

    std::free(first);
    std::free(second);
    std::free(list.playerState);
    return linked ? 0 : 3;
}

extern "C" int zclass_object3d_visible_and_color_smoke() {
    zClass_Object3DDataPartial data{};
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    if (zClass_Object3D::gwObject3DSetVisibleFlag(&node, 1) != 0 || (data.flags & 4) == 0) {
        return 1;
    }
    if (zClass_Object3D::gwObject3DSetVisibleFlag(&node, 0) != 0 || (data.flags & 4) != 0) {
        return 2;
    }

    zColorRgb color{-1.0f, 0.5f, 2.0f};
    if (zClass_Object3D::gwObject3DSetColorAlpha(&node, &color, 1.5f) != 0) {
        return 3;
    }
    if (data.color.red != 0.0f || data.color.green != 0.5f || data.color.blue != 1.0f ||
        data.colorAlpha != 1.0f) {
        return 4;
    }

    data.color.red = 0.25f;
    if (zClass_Object3D::gwObject3DSetColorAlpha(&node, nullptr, -0.5f) != 0) {
        return 5;
    }
    if (data.color.red != 0.25f || data.colorAlpha != 0.0f) {
        return 6;
    }

    zClass_NodePartial wrongClass{};
    wrongClass.classId = 4;
    wrongClass.classData = &data;
    if (zClass_Object3D::gwObject3DSetVisibleFlag(nullptr, 1) != 5 ||
        zClass_Object3D::gwObject3DSetVisibleFlag(&wrongClass, 1) != 3) {
        return 7;
    }

    return 0;
}

extern "C" int zclass_object3d_alpha_scale_and_lit_smoke() {
    zClass_Object3DDataPartial data{};
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    if (zClass_Object3D::gwObject3DSetAlphaScale(&node, 0.375f) != 0) {
        return 1;
    }

    float alphaScale = 0.0f;
    if (zClass_Object3D::gwObject3DGetAlphaScale(&node, &alphaScale) != 0 || alphaScale != 0.375f) {
        return 2;
    }

    if (zClass_Object3D::gwObject3DSetLitFlag(&node, 1) != 0 || (data.flags & 2) == 0) {
        return 3;
    }
    if (zClass_Object3D::gwObject3DSetLitFlag(&node, 0) != 0 || (data.flags & 2) != 0) {
        return 4;
    }

    zClass_NodePartial nullDataNode{};
    nullDataNode.classId = 5;
    if (zClass_Object3D::gwObject3DGetAlphaScale(&nullDataNode, &alphaScale) != 5) {
        return 5;
    }

    return 0;
}

extern "C" int zclass_object3d_transform_getters_smoke() {
    zClass_Object3DDataPartial data{};
    data.rotation = {1.0f, 2.0f, 3.0f};
    data.scale = {4.0f, 5.0f, 6.0f};
    data.localMatrix[9] = 7.0f;
    data.localMatrix[10] = 8.0f;
    data.localMatrix[11] = 9.0f;

    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (zClass_Object3D::gwObject3DGetScale(&node, &x, &y, &z) != 0 || x != 4.0f || y != 5.0f ||
        z != 6.0f) {
        return 1;
    }

    if (zClass_Object3D::gwObject3DGetRotation(&node, &x, &y, &z) != 0 || x != 1.0f || y != 2.0f ||
        z != 3.0f) {
        return 2;
    }

    if (zClass_Object3D::gwObject3DGetPosition(&node, &x, &y, &z) != 0 || x != 7.0f || y != 8.0f ||
        z != 9.0f) {
        return 3;
    }
    if (zClass_Object3D::gwObject3DGetMatrixPtr(&node) != data.localMatrix) {
        return 4;
    }

    zClass_NodePartial wrongClass{};
    wrongClass.classId = 4;
    wrongClass.classData = &data;
    if (zClass_Object3D::gwObject3DGetScale(&wrongClass, &x, &y, &z) != 0 ||
        zClass_Object3D::gwObject3DGetPosition(&wrongClass, &x, &y, &z) != 3) {
        return 5;
    }

    return zClass_Object3D::gwObject3DGetRotation(nullptr, &x, &y, &z) == 5 ? 0 : 6;
}

extern "C" int zclass_object3d_transform_setters_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial data{};
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    data.flags = 0x18;
    if (zClass_Object3D::gwObject3DSetScale(&node, 2.0f, 1.0f, 1.0f) != 0 || data.scale.x != 2.0f ||
        data.scale.y != 1.0f || data.scale.z != 1.0f || (data.flags & 0x18) != 0 ||
        (data.flags & 0x21) != 0x21 || (node.flags & 0x02000003) != 0x02000003) {
        return 1;
    }

    data.flags = 0x18;
    if (zClass_Object3D::gwObject3DSetRotation(&node, 0.0f, 0.0f, 0.0f) != 0 ||
        (data.flags & 0x08) == 0 || (data.flags & 0x10) != 0) {
        return 2;
    }
    if (zClass_Object3D::gwObject3DTranslateRotation(&node, 0.5f, 1.0f, 1.5f) != 0 ||
        data.rotation.x != 0.5f || data.rotation.y != 1.0f || data.rotation.z != 1.5f ||
        (data.flags & 0x08) != 0) {
        return 3;
    }

    data.flags = 0x08;
    if (zClass_Object3D::gwObject3DSetPosition(&node, 0.0f, 0.0f, 0.0f) != 0 ||
        (data.flags & 0x08) == 0) {
        return 4;
    }
    if (zClass_Object3D::gwObject3DTranslatePosition(&node, 3.0f, 4.0f, 5.0f) != 0 ||
        data.localMatrix[9] != 3.0f || data.localMatrix[10] != 4.0f ||
        data.localMatrix[11] != 5.0f || (data.flags & 0x08) != 0) {
        return 5;
    }

    zClass_NodePartial wrongClass{};
    wrongClass.classId = 4;
    wrongClass.classData = &data;
    if (zClass_Object3D::gwObject3DTranslatePosition(&wrongClass, 1.0f, 2.0f, 3.0f) != 3) {
        return 6;
    }

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    return zClass_Object3D::gwObject3DSetScale(nullptr, 1.0f, 1.0f, 1.0f) == 5 &&
                   zClass_Object3D::gwObject3DSetRotation(nullptr, 0.0f, 0.0f, 0.0f) == 5 &&
                   zClass_Object3D::gwObject3DSetPosition(nullptr, 0.0f, 0.0f, 0.0f) == 5
               ? 0
               : 7;
}

extern "C" int zclass_object3d_reset_transform_dirty_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial data{};
    data.flags = 0x10;
    data.rotation = {1.0f, 2.0f, 3.0f};
    data.scale = {4.0f, 5.0f, 6.0f};
    for (int i = 0; i < 12; ++i) {
        data.localMatrix[i] = static_cast<float>(i + 1);
    }
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    if (zClass_Object3D::PropagateTransformDirty(&node) != 0) {
        return 1;
    }

    bool matrixOk = true;
    for (int i = 0; i < 12; ++i) {
        const float expected = i == 0 || i == 4 || i == 8 ? 1.0f : 0.0f;
        matrixOk = matrixOk && data.localMatrix[i] == expected;
    }

    const bool ok = data.rotation.x == 0.0f && data.rotation.y == 0.0f && data.rotation.z == 0.0f &&
                    data.scale.x == 1.0f && data.scale.y == 1.0f && data.scale.z == 1.0f &&
                    matrixOk && (data.flags & 0x39) == 0x29 && (data.flags & 0x10) == 0 &&
                    (node.flags & 0x02000003) == 0x02000003 && (node.boundsFlags & 0x04) != 0 &&
                    zClass_TypeList::Head(7) != nullptr && zClass_TypeList::Head(7)->node == &node;

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    zClass_NodePartial noData{};
    return ok && zClass_Object3D::PropagateTransformDirty(nullptr) == 5 &&
                   zClass_Object3D::PropagateTransformDirty(&noData) == 5
               ? 0
               : 2;
}

extern "C" int zclass_model_ref_lerp_queue_reset_smoke() {
    auto *const first = static_cast<zClass_Object3D_ModelRefLerpTask *>(
        ::operator new(sizeof(zClass_Object3D_ModelRefLerpTask)));
    auto *const second = static_cast<zClass_Object3D_ModelRefLerpTask *>(
        ::operator new(sizeof(zClass_Object3D_ModelRefLerpTask)));
    first->next = second;
    second->next = nullptr;

    g_ModelRefLerpQueueState.listAux = 0x11;
    g_ModelRefLerpQueueState.head = first;
    g_ModelRefLerpQueueState.tail = second;
    g_ModelRefLerpQueueState.count = 2;

    zClass_Object3D_ModelRefLerpQueue::Reset();

    return g_ModelRefLerpQueueState.listAux == 0 && g_ModelRefLerpQueueState.head == nullptr &&
                   g_ModelRefLerpQueueState.tail == nullptr && g_ModelRefLerpQueueState.count == 0
               ? 0
               : 1;
}

void RECOIL_FASTCALL zclass_model_ref_lerp_test_callback(void *) {
}

extern "C" int zclass_model_ref_lerp_queue_add_smoke() {
    zClass_Object3D_ModelRefLerpQueue::Reset();

    zClass_NodePartial firstNode{};
    zClass_Object3DDataPartial firstData{};
    firstNode.classId = 5;
    firstNode.classData = &firstData;

    zClass_NodePartial secondNode{};
    zClass_Object3DDataPartial secondData{};
    secondNode.classId = 5;
    secondNode.classData = &secondData;

    int callbackCtx = 0x1234;
    void *const callback = (void *)zclass_model_ref_lerp_test_callback;

    zClass_Object3D_ModelRefLerpQueue::Add(&firstNode, &callbackCtx, callback, 0.25f,
                                           0.75f, 2.0f);
    zClass_Object3D_ModelRefLerpTask *const first = g_ModelRefLerpQueueState.head;
    const bool firstOk =
        g_ModelRefLerpQueueState.count == 1 && first != nullptr &&
        g_ModelRefLerpQueueState.tail == first && first->node == &firstNode &&
        first->callbackCtx == &callbackCtx && first->onComplete == callback &&
        first->invertModelRef == 0 && first->targetModelRef == 0.75f &&
        first->currentModelRef == 0.25f && first->modelRefDeltaPerSec == 0.25f &&
        first->next == nullptr && (firstData.flags & 0x02) != 0;

    zClass_Object3D_ModelRefLerpQueue::Add(&secondNode, &callbackCtx, callback, 1.5f,
                                           -0.5f, 0.0f);
    zClass_Object3D_ModelRefLerpTask *const second = g_ModelRefLerpQueueState.tail;
    const bool secondOk =
        g_ModelRefLerpQueueState.count == 2 && g_ModelRefLerpQueueState.head == first &&
        first->next == second && second != nullptr && second->next == nullptr &&
        second->node == &secondNode && second->invertModelRef == 1 &&
        second->targetModelRef == 1.0f && second->currentModelRef == 0.0f &&
        second->modelRefDeltaPerSec == -99999997952.0f && (secondData.flags & 0x02) != 0;

    zClass_Object3D_ModelRefLerpQueue::Reset();
    return firstOk && secondOk && g_ModelRefLerpQueueState.count == 0 ? 0 : 1;
}

extern "C" int zclass_model_ref_lerp_queue_update_smoke() {
    zClass_Object3D_ModelRefLerpQueue::Reset();

    zClass_NodePartial completeHeadNode{};
    zClass_Object3DDataPartial completeHeadData{};
    completeHeadNode.classId = 5;
    completeHeadNode.classData = &completeHeadData;

    zClass_NodePartial ongoingNode{};
    zClass_Object3DDataPartial ongoingData{};
    ongoingNode.classId = 5;
    ongoingNode.classData = &ongoingData;

    zClass_NodePartial completeTailNode{};
    zClass_Object3DDataPartial completeTailData{};
    completeTailNode.classId = 5;
    completeTailNode.classData = &completeTailData;

    int headCallbackCtx = 0x1111;
    int tailCallbackCtx = 0x3333;
    g_modelRefLerpCallbackCount = 0;
    g_modelRefLerpLastCallbackCtx = nullptr;
    g_FrameDeltaTimeSec = 2.0f;

    zClass_Object3D_ModelRefLerpQueue::Add(&completeHeadNode, &headCallbackCtx,
                                           (void *)TestModelRefLerpCallback, 0.9f, 1.0f,
                                           1.0f);
    zClass_Object3D_ModelRefLerpQueue::Add(&ongoingNode, nullptr, nullptr, 0.2f, 0.8f,
                                           10.0f);
    zClass_Object3D_ModelRefLerpQueue::Add(&completeTailNode, &tailCallbackCtx,
                                           (void *)TestModelRefLerpCallback, 1.0f, 0.0f,
                                           1.0f);

    zClass_Object3D_ModelRefLerpTask *const ongoingTask =
        g_ModelRefLerpQueueState.head != nullptr ? g_ModelRefLerpQueueState.head->next
                                                 : nullptr;

    zClass_Object3D_ModelRefLerpQueue::Update();

    const bool listOk =
        g_ModelRefLerpQueueState.count == 1 && g_ModelRefLerpQueueState.head == ongoingTask &&
        g_ModelRefLerpQueueState.tail == ongoingTask && ongoingTask != nullptr &&
        ongoingTask->next == nullptr;
    const bool alphaOk =
        completeHeadData.alphaScale == 1.0f && ongoingData.alphaScale > 0.31f &&
        ongoingData.alphaScale < 0.33f && completeTailData.alphaScale == 0.0f;
    const bool taskOk =
        ongoingTask != nullptr && ongoingTask->currentModelRef > 0.31f &&
        ongoingTask->currentModelRef < 0.33f && ongoingTask->targetModelRef == 0.8f;
    const bool callbackOk = g_modelRefLerpCallbackCount == 2 &&
                            g_modelRefLerpLastCallbackCtx == &tailCallbackCtx;
    const bool litOk = (completeHeadData.flags & 0x02) == 0 &&
                       (ongoingData.flags & 0x02) != 0 &&
                       (completeTailData.flags & 0x02) != 0;

    zClass_Object3D_ModelRefLerpQueue::Reset();
    return listOk && alphaOk && taskOk && callbackOk && litOk ? 0 : 1;
}

extern "C" int zclass_type_list_alloc_and_insert_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_TypeListLink freeSecond{};
    zClass_TypeListLink freeFirst{};
    freeFirst.next = &freeSecond;
    freeSecond.prev = &freeFirst;
    freeFirst.pendingRemove = 1;
    g_zClass_TypeList_FreeLinkHead = &freeFirst;

    zClass_TypeListLink *reused = zClass_TypeList::AllocLink();
    if (reused != &freeFirst || g_zClass_TypeList_FreeLinkHead != &freeSecond ||
        freeSecond.prev != nullptr || reused->next != nullptr || reused->prev != nullptr ||
        reused->pendingRemove != 0 || g_zClass_TypeList_LiveLinkCount != 1 ||
        g_zClass_TypeList_PeakLiveLinkCount != 1) {
        return 1;
    }

    zClass_TypeList::FreeLink(reused);
    if (g_zClass_TypeList_FreeLinkHead != reused || reused->next != &freeSecond ||
        freeSecond.prev != reused || g_zClass_TypeList_LiveLinkCount != 0) {
        return 2;
    }

    g_zClass_TypeList_FreeLinkHead = nullptr;
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    zClass_NodePartial worldChild{};
    zClass_NodePartial flaggedChild{};
    zClass_NodePartial *children[3] = {&child, &worldChild, &flaggedChild};
    parent.listCountA = 3;
    parent.listA = children;
    worldChild.classId = 2;
    flaggedChild.flags = 1;

    if (zClass_TypeList::Insert(7, &parent) != 0) {
        return 3;
    }

    zClass_TypeListLink *head = zClass_TypeList::Head(7);
    zClass_TypeListLink *tail = zClass_TypeList::Tail(7);
    const bool ok =
        head != nullptr && tail != nullptr && head->node == &parent && tail->node == &child &&
        head->next == tail && tail->prev == head && (parent.flags & 1) != 0 &&
        (child.flags & 1) != 0 && worldChild.flags == 0 && zClass_TypeList::CountNodes(7) == 2 &&
        zClass_TypeList::GetBucketHead(7) == head && g_zClass_TypeList_LiveLinkCount == 2 &&
        g_zClass_TypeList_PeakLiveLinkCount == 2;

    if (!ok) {
        return 4;
    }

    if (zClass_TypeList::MarkPendingRemoval(3, &parent) != 1) {
        return 5;
    }
    if (zClass_TypeList::MarkPendingRemoval(7, &child) != 0 || tail->pendingRemove != 1 ||
        zClass_TypeList::PendingRemovalDirty(7) != 1) {
        return 6;
    }

    for (zClass_TypeListLink *link = head; link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    zClass_NodePartial pendingNode{};
    if (zClass_NodeList::Insert(&pendingNode) != 0 ||
        g_zClass_NodeList_PendingFreeHead == nullptr ||
        g_zClass_NodeList_PendingFreeHead->node != &pendingNode) {
        return 7;
    }
    zClass_TypeList::FreeLink(g_zClass_NodeList_PendingFreeHead);
    g_zClass_NodeList_PendingFreeHead = nullptr;

    zClass_NodePartial removeNode{};
    zClass_NodePartial keepNode{};
    zClass_NodePartial tailNode{};
    removeNode.flags = 1;
    keepNode.flags = 3;
    zClass_TypeListLink *removeLink =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    zClass_TypeListLink *keepLink =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    zClass_TypeListLink *tailLink =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    if (removeLink == nullptr || keepLink == nullptr || tailLink == nullptr) {
        std::free(removeLink);
        std::free(keepLink);
        std::free(tailLink);
        return 8;
    }
    removeLink->node = &removeNode;
    removeLink->pendingRemove = 1;
    removeLink->next = keepLink;
    keepLink->node = &keepNode;
    keepLink->pendingRemove = 1;
    keepLink->prev = removeLink;
    keepLink->next = tailLink;
    tailLink->node = &tailNode;
    tailLink->prev = keepLink;
    zClass_TypeList::Head(7) = removeLink;
    zClass_TypeList::Tail(7) = tailLink;
    zClass_TypeList::PendingRemovalDirty(7) = 1;
    g_zClass_TypeList_LiveLinkCount = 3;

    zClass_TypeList::ProcessPendingRemovals(7);
    if (zClass_TypeList::Head(7) != keepLink || zClass_TypeList::Tail(7) != tailLink ||
        keepLink->pendingRemove != 0 || keepLink->prev != nullptr || keepLink->next != tailLink ||
        tailLink->prev != keepLink || (removeNode.flags & 1) != 0 || (keepNode.flags & 1) == 0 ||
        zClass_TypeList::PendingRemovalDirty(7) != 0 ||
        g_zClass_TypeList_FreeLinkHead != removeLink) {
        return 9;
    }
    std::free(keepLink);
    std::free(tailLink);
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    zClass_NodePartial queuedNode{};
    queuedNode.classId = 6;
    queuedNode.flags = 0x03;
    zClass_TypeListLink skippedQueuedLink{};
    skippedQueuedLink.pendingRemove = 1;
    zClass_TypeListLink activeQueuedLink{};
    skippedQueuedLink.next = &activeQueuedLink;
    activeQueuedLink.prev = &skippedQueuedLink;
    activeQueuedLink.node = &queuedNode;
    zClass_TypeList::Head(7) = &skippedQueuedLink;
    zClass_TypeList::Tail(7) = &activeQueuedLink;
    g_zClass_DeferredProcessingEnabled = 0;
    if (zClass_TypeList::UpdateQueuedTrees() != 0 || activeQueuedLink.pendingRemove != 1 ||
        (queuedNode.flags & 0x02) != 0) {
        return 12;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;
    zClass_TypeList::PendingRemovalDirty(7) = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_TypeListLink *freeA =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    zClass_TypeListLink *freeB =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    if (freeA == nullptr || freeB == nullptr) {
        std::free(freeA);
        std::free(freeB);
        return 10;
    }
    freeA->next = freeB;
    g_zClass_TypeList_FreeLinkHead = freeA;
    zClass_TypeList::FreeAll();
    return g_zClass_TypeList_FreeLinkHead == nullptr ? 0 : 11;
}

extern "C" int zclass_animate_update_smoke() {
    auto nearFloat = [](float a, float b) { return std::fabs(a - b) <= 0.0001f; };

    zClass_AnimateKeyframePartial keyframes[3]{};
    keyframes[0].rotation = {0.0f, 2.0f, 4.0f};
    keyframes[0].position = {10.0f, 20.0f, 30.0f};
    keyframes[0].scale = {1.0f, 2.0f, 3.0f};
    keyframes[1].rotation = {10.0f, 12.0f, 14.0f};
    keyframes[1].position = {30.0f, 40.0f, 50.0f};
    keyframes[1].scale = {3.0f, 4.0f, 5.0f};
    keyframes[2].rotation = {20.0f, 22.0f, 24.0f};
    keyframes[2].position = {50.0f, 60.0f, 70.0f};
    keyframes[2].scale = {5.0f, 6.0f, 7.0f};

    zClass_AnimateRuntimePartial runtime{};
    runtime.keyframes = keyframes;
    runtime.duration = 4.0f;
    runtime.currentTime = 1.0f;
    runtime.maxFrameIndex = 3;
    runtime.loopCount = -1;
    runtime.outputRotationScale = {2.0f, 3.0f, 4.0f};
    runtime.outputPositionScale = {1.0f, 0.5f, 0.25f};
    runtime.outputScaleScale = {1.0f, 2.0f, 3.0f};

    if (zClass_Animate::SampleTransform(&runtime) != 1 ||
        !nearFloat(runtime.sampledRotation.x, 10.0f) ||
        !nearFloat(runtime.sampledRotation.y, 21.0f) ||
        !nearFloat(runtime.sampledRotation.z, 36.0f) ||
        !nearFloat(runtime.sampledPosition.x, 20.0f) ||
        !nearFloat(runtime.sampledPosition.y, 15.0f) ||
        !nearFloat(runtime.sampledPosition.z, 10.0f) || !nearFloat(runtime.sampledScale.x, 2.0f) ||
        !nearFloat(runtime.sampledScale.y, 6.0f) || !nearFloat(runtime.sampledScale.z, 12.0f)) {
        return 1;
    }

    runtime.state = 2;
    if (zClass_Animate::SampleTransform(&runtime) != 2) {
        return 2;
    }

    zClass_AnimateRuntimePartial endRuntime{};
    endRuntime.duration = 2.0f;
    endRuntime.currentTime = 1.5f;
    endRuntime.loopCount = -1;
    if (zClass_Animate::AdvanceTime(&endRuntime, 0.75f) != 1 || endRuntime.state != 2 ||
        endRuntime.currentTime != 0.0f || zClass_Animate::AdvanceTime(&endRuntime, 0.25f) != 2) {
        return 3;
    }

    zClass_AnimateRuntimePartial loopRuntime{};
    loopRuntime.duration = 10.0f;
    loopRuntime.currentTime = 1.0f;
    loopRuntime.startTime = 1.5f;
    loopRuntime.loopBase = 0.25f;
    loopRuntime.loopCount = 0;
    if (zClass_Animate::AdvanceTime(&loopRuntime, 1.0f) != 1 ||
        !nearFloat(loopRuntime.currentTime, 0.75f)) {
        return 4;
    }

    if (zClass_Animate::UpdateNode(nullptr) != 5) {
        return 6;
    }
    zClass_NodePartial missingDataNode{};
    if (zClass_Animate::UpdateNode(&missingDataNode) != 5) {
        return 7;
    }

    zClass_AnimateDataPartial stoppedData{};
    stoppedData.statusFlags = 0x04;
    stoppedData.runtime.state = 2;
    stoppedData.runtime.duration = 1.0f;
    stoppedData.runtime.currentTime = 0.9f;
    stoppedData.runtime.loopCount = -1;
    zClass_NodePartial stoppedNode{};
    stoppedNode.classData = &stoppedData;
    g_FrameDeltaTimeSec = 0.2f;
    if (zClass_Animate::UpdateNode(&stoppedNode) != 0 ||
        (stoppedData.statusFlags & 0x04) != 0 || stoppedNode.flags != 0) {
        return 8;
    }

    reset_zclass_type_lists_for_test();
    zClass_AnimateDataPartial data{};
    data.statusFlags = 0x04;
    data.runtime.keyframes = keyframes;
    data.runtime.duration = 4.0f;
    data.runtime.currentTime = 1.0f;
    data.runtime.maxFrameIndex = 3;
    data.runtime.loopCount = -1;
    data.runtime.outputRotationScale = {1.0f, 1.0f, 1.0f};
    data.runtime.outputPositionScale = {1.0f, 1.0f, 1.0f};
    data.runtime.outputScaleScale = {1.0f, 1.0f, 1.0f};
    zClass_NodePartial node{};
    node.flags = 0x04;
    node.classData = &data;
    g_FrameDeltaTimeSec = 1.0f;

    if (zClass_Animate::UpdateNode(&node) != 0 || data.flags != 1 || (node.flags & 0x07) != 0x07 ||
        zClass_TypeList::Head(7) == nullptr || zClass_TypeList::Head(7)->node != &node ||
        !nearFloat(data.runtime.currentTime, 2.0f) ||
        !nearFloat(data.runtime.sampledRotation.x, 10.0f) ||
        !nearFloat(data.runtime.sampledPosition.y, 40.0f) ||
        !nearFloat(data.runtime.sampledScale.z, 5.0f)) {
        free_zclass_type_lists_for_test();
        return 5;
    }

    free_zclass_type_lists_for_test();
    return 0;
}

extern "C" int zclass_sequence_new_add_child_smoke() {
    reset_zclass_type_lists_for_test();
    zClass_NodeFreeListSlot slots[1]{};
    slots[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zClass_NodePartial *sequence = zClass_Sequence::gwSequenceNew();
    if (sequence != &slots[0].node || sequence->classId != 7 || sequence->classData == nullptr ||
        g_zClass_NodeFreeHeadIndex != -1 || zClass_TypeList::Head(11) == nullptr ||
        zClass_TypeList::Head(11)->node != sequence) {
        if (sequence != nullptr) {
            std::free(sequence->classData);
        }
        free_zclass_type_lists_for_test();
        return 1;
    }

    zClass_SequenceDataPartial *sequenceData =
        static_cast<zClass_SequenceDataPartial *>(sequence->classData);
    if (sequenceData->step != 1 || sequenceData->entryCount != 0) {
        std::free(sequence->classData);
        free_zclass_type_lists_for_test();
        return 2;
    }

    std::free(sequence->classData);
    free_zclass_type_lists_for_test();

    reset_zclass_type_lists_for_test();
    zClass_NodePartial parent{};
    zClass_NodePartial first{};
    zClass_NodePartial second{};
    zClass_NodePartial inserted{};
    const std::size_t twoEntryBytes =
        sizeof(zClass_SequenceDataPartial) + sizeof(zClass_SequenceEntryPartial);
    sequenceData = static_cast<zClass_SequenceDataPartial *>(std::calloc(1, twoEntryBytes));
    if (sequenceData == nullptr) {
        return 3;
    }
    sequenceData->entryCount = 2;
    sequenceData->entries[0].node = &first;
    sequenceData->entries[0].triggerTime = 1.0f;
    sequenceData->entries[1].node = &second;
    sequenceData->entries[1].triggerTime = 3.0f;
    parent.classData = sequenceData;

    if (zClass_Sequence::gwSequenceAddChild(&parent, &inserted, 1, 2.0f) != 0) {
        std::free(parent.classData);
        free_zclass_type_lists_for_test();
        return 4;
    }

    sequenceData = static_cast<zClass_SequenceDataPartial *>(parent.classData);
    if (zClass_Sequence::SetActive(&parent, 7) != 0 ||
        zClass_Sequence::SetRepeat(&parent, 8) != 0 || zClass_Sequence::SetLoop(&parent, 9) != 0 ||
        zClass_Sequence::SetPause(&parent, 10) != 0) {
        std::free(parent.listB);
        std::free(inserted.listA);
        std::free(parent.classData);
        free_zclass_type_lists_for_test();
        return 5;
    }

    const bool ok =
        sequenceData->entryCount == 3 && sequenceData->isActive == 7 &&
        sequenceData->repeatAtBounds == 8 && sequenceData->wrapAtBounds == 9 &&
        sequenceData->isPaused == 10 && sequenceData->entries[0].node == &first &&
        sequenceData->entries[0].triggerTime == 1.0f &&
        sequenceData->entries[1].node == &inserted &&
        sequenceData->entries[1].triggerTime == 2.0f && sequenceData->entries[2].node == &second &&
        sequenceData->entries[2].triggerTime == 3.0f && parent.listCountB == 1 &&
        parent.listB[0] == &inserted && inserted.listCountA == 1 && inserted.listA[0] == &parent &&
        zClass_TypeList::Head(7) != nullptr && zClass_TypeList::Head(7)->node == &parent;

    std::free(parent.listB);
    std::free(inserted.listA);
    std::free(parent.classData);
    free_zclass_type_lists_for_test();
    return ok ? 0 : 6;
}

extern "C" int zclass_sequence_update_smoke() {
    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.0001f; };
    auto allocSequenceData = [](int entryCount) -> zClass_SequenceDataPartial * {
        const std::size_t bytes =
            sizeof(zClass_SequenceDataPartial) +
            sizeof(zClass_SequenceEntryPartial) * static_cast<std::size_t>(entryCount - 1);
        return static_cast<zClass_SequenceDataPartial *>(std::calloc(1, bytes));
    };

    if (zClass_Sequence::Update(nullptr) != 5) {
        return 1;
    }

    zClass_NodePartial node{};
    if (zClass_Sequence::Update(&node) != 5) {
        return 2;
    }

    zClass_SequenceDataPartial *data = allocSequenceData(3);
    if (data == nullptr) {
        return 3;
    }
    node.classData = data;
    data->isActive = 1;
    data->isPaused = 1;
    data->step = 1;
    data->currentIndex = 0;
    data->currentTime = 2.0f;
    data->entryCount = 3;
    data->entries[0].triggerTime = 4.0f;
    data->entries[1].triggerTime = 4.0f;
    data->entries[2].triggerTime = 4.0f;
    g_FrameDeltaTimeSec = 1.0f;
    if (zClass_Sequence::Update(&node) != 0 || !nearFloat(data->currentTime, 2.0f) ||
        data->currentIndex != 0) {
        std::free(data);
        return 4;
    }

    data->isPaused = 0;
    data->isActive = 0;
    if (zClass_Sequence::Update(&node) != 0 || !nearFloat(data->currentTime, 2.0f) ||
        data->currentIndex != 0) {
        std::free(data);
        return 5;
    }

    data->isActive = 1;
    data->currentTime = 0.5f;
    data->entries[0].triggerTime = 2.0f;
    if (zClass_Sequence::Update(&node) != 0 || !nearFloat(data->currentTime, 1.5f) ||
        data->currentIndex != 0) {
        std::free(data);
        return 6;
    }

    data->currentTime = 0.0f;
    data->entryCount = 3;
    data->step = 1;
    data->currentIndex = 0;
    data->entries[0].triggerTime = 1.0f;
    data->entries[1].triggerTime = 1.0f;
    data->entries[2].triggerTime = 5.0f;
    g_FrameDeltaTimeSec = 3.5f;
    if (zClass_Sequence::Update(&node) != 0 || data->currentIndex != 2 ||
        !nearFloat(data->currentTime, 1.5f)) {
        std::free(data);
        return 7;
    }

    data->isActive = 1;
    data->repeatAtBounds = 0;
    data->wrapAtBounds = 0;
    data->entryCount = 2;
    data->step = 1;
    data->currentIndex = 1;
    data->currentTime = 0.9f;
    data->entries[1].triggerTime = 1.0f;
    g_FrameDeltaTimeSec = 0.3f;
    if (zClass_Sequence::Update(&node) != 0 || data->currentIndex != 1 ||
        data->step != -1 || data->isActive != 0 || !nearFloat(data->currentTime, 0.2f)) {
        std::free(data);
        return 8;
    }

    data->isActive = 1;
    data->repeatAtBounds = 1;
    data->wrapAtBounds = 1;
    data->entryCount = 2;
    data->step = 1;
    data->currentIndex = 1;
    data->currentTime = 0.9f;
    data->entries[0].triggerTime = 1.0f;
    data->entries[1].triggerTime = 1.0f;
    if (zClass_Sequence::Update(&node) != 0 || data->currentIndex != 0 ||
        data->step != 1 || data->isActive != 1 || !nearFloat(data->currentTime, 0.2f)) {
        std::free(data);
        return 9;
    }

    data->isActive = 1;
    data->repeatAtBounds = 0;
    data->wrapAtBounds = 0;
    data->entryCount = 2;
    data->step = -1;
    data->currentIndex = 0;
    data->currentTime = 0.9f;
    data->entries[0].triggerTime = 1.0f;
    if (zClass_Sequence::Update(&node) != 0 || data->currentIndex != 0 ||
        data->step != 1 || data->isActive != 0 || !nearFloat(data->currentTime, 0.2f)) {
        std::free(data);
        return 10;
    }

    std::free(data);
    return 0;
}

extern "C" int zclass_typelist_update_sequences_smoke() {
    reset_zclass_type_lists_for_test();
    if (zClass_TypeList::UpdateSequences() != 0) {
        return 1;
    }

    zClass_SequenceDataPartial activeData{};
    activeData.isActive = 1;
    activeData.step = 1;
    activeData.entryCount = 1;
    activeData.entries[0].triggerTime = 3.0f;
    zClass_NodePartial activeNode{};
    activeNode.classData = &activeData;

    zClass_SequenceDataPartial pendingData{};
    pendingData.isActive = 1;
    pendingData.step = 1;
    pendingData.entryCount = 1;
    pendingData.entries[0].triggerTime = 3.0f;
    zClass_NodePartial pendingNode{};
    pendingNode.classData = &pendingData;

    zClass_TypeListLink activeLink{};
    zClass_TypeListLink pendingLink{};
    activeLink.node = &activeNode;
    activeLink.next = &pendingLink;
    pendingLink.node = &pendingNode;
    pendingLink.prev = &activeLink;
    pendingLink.pendingRemove = 1;
    zClass_TypeList::Head(11) = &activeLink;
    zClass_TypeList::Tail(11) = &pendingLink;
    g_zClass_DeferredProcessingEnabled = 1;
    g_FrameDeltaTimeSec = 1.0f;

    const int result = zClass_TypeList::UpdateSequences();
    zClass_TypeList::Head(11) = nullptr;
    zClass_TypeList::Tail(11) = nullptr;

    return result == 0 && g_zClass_DeferredProcessingEnabled == 1 &&
                   activeData.currentTime == 1.0f && pendingData.currentTime == 0.0f
               ? 0
               : 2;
}

extern "C" int zclass_typelist_update_animations_smoke() {
    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.0001f; };
    zClass_AnimateKeyframePartial keyframes[3]{};
    keyframes[0].position = {1.0f, 2.0f, 3.0f};
    keyframes[1].position = {5.0f, 6.0f, 7.0f};
    keyframes[2].position = {9.0f, 10.0f, 11.0f};

    reset_zclass_type_lists_for_test();
    zClass_AnimateDataPartial activeData{};
    activeData.statusFlags = 0x04;
    activeData.runtime.keyframes = keyframes;
    activeData.runtime.duration = 4.0f;
    activeData.runtime.currentTime = 1.0f;
    activeData.runtime.maxFrameIndex = 3;
    activeData.runtime.loopCount = -1;
    activeData.runtime.outputRotationScale = {1.0f, 1.0f, 1.0f};
    activeData.runtime.outputPositionScale = {1.0f, 1.0f, 1.0f};
    activeData.runtime.outputScaleScale = {1.0f, 1.0f, 1.0f};
    zClass_NodePartial activeNode{};
    activeNode.flags = 0x04;
    activeNode.classData = &activeData;

    zClass_AnimateDataPartial inactiveData = activeData;
    zClass_NodePartial inactiveNode{};
    inactiveNode.classData = &inactiveData;

    zClass_AnimateDataPartial pendingData = activeData;
    zClass_NodePartial pendingNode{};
    pendingNode.flags = 0x04;
    pendingNode.classData = &pendingData;

    zClass_TypeListLink activeLink{};
    zClass_TypeListLink inactiveLink{};
    zClass_TypeListLink pendingLink{};
    activeLink.node = &activeNode;
    activeLink.next = &inactiveLink;
    inactiveLink.node = &inactiveNode;
    inactiveLink.prev = &activeLink;
    inactiveLink.next = &pendingLink;
    pendingLink.node = &pendingNode;
    pendingLink.prev = &inactiveLink;
    pendingLink.pendingRemove = 1;
    zClass_TypeList::Head(12) = &activeLink;
    zClass_TypeList::Tail(12) = &pendingLink;
    g_zClass_DeferredProcessingEnabled = 1;
    g_FrameDeltaTimeSec = 1.0f;

    const int result = zClass_TypeList::UpdateAnimations();
    const bool ok = result == 0 && g_zClass_DeferredProcessingEnabled == 1 &&
                    activeData.flags == 1 && (activeNode.flags & 0x07) == 0x07 &&
                    zClass_TypeList::Head(7) != nullptr &&
                    zClass_TypeList::Head(7)->node == &activeNode &&
                    inactiveData.flags == 0 && pendingData.flags == 0 &&
                    nearFloat(activeData.runtime.currentTime, 2.0f) &&
                    nearFloat(inactiveData.runtime.currentTime, 1.0f) &&
                    nearFloat(pendingData.runtime.currentTime, 1.0f);
    zClass_TypeList::Head(12) = nullptr;
    zClass_TypeList::Tail(12) = nullptr;
    free_zclass_type_lists_for_test();
    return ok ? 0 : 1;
}

extern "C" int zclass_gwnode_update_all_smoke() {
    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.0001f; };
    reset_zclass_type_lists_for_test();

    zClass_SequenceDataPartial sequenceData{};
    sequenceData.isActive = 1;
    sequenceData.step = 1;
    sequenceData.entryCount = 1;
    sequenceData.entries[0].triggerTime = 3.0f;
    zClass_NodePartial sequenceNode{};
    sequenceNode.classData = &sequenceData;
    zClass_TypeListLink sequenceLink{};
    sequenceLink.node = &sequenceNode;
    zClass_TypeList::Head(11) = &sequenceLink;
    zClass_TypeList::Tail(11) = &sequenceLink;

    zClass_AnimateKeyframePartial keyframes[3]{};
    keyframes[0].scale = {1.0f, 1.0f, 1.0f};
    keyframes[1].scale = {2.0f, 3.0f, 4.0f};
    keyframes[2].scale = {5.0f, 6.0f, 7.0f};
    zClass_AnimateDataPartial animateData{};
    animateData.statusFlags = 0x04;
    animateData.runtime.keyframes = keyframes;
    animateData.runtime.duration = 4.0f;
    animateData.runtime.currentTime = 1.0f;
    animateData.runtime.maxFrameIndex = 3;
    animateData.runtime.loopCount = -1;
    animateData.runtime.outputRotationScale = {1.0f, 1.0f, 1.0f};
    animateData.runtime.outputPositionScale = {1.0f, 1.0f, 1.0f};
    animateData.runtime.outputScaleScale = {1.0f, 1.0f, 1.0f};
    zClass_NodePartial animateNode{};
    animateNode.flags = 0x05;
    animateNode.classData = &animateData;
    zClass_TypeListLink animateLink{};
    animateLink.node = &animateNode;
    zClass_TypeList::Head(12) = &animateLink;
    zClass_TypeList::Tail(12) = &animateLink;

    g_FrameDeltaTimeSec = 1.0f;
    const int result = zClass_Class::gwNodeUpdateAll();
    const bool ok = result == 0 && sequenceData.currentTime == 1.0f &&
                    animateData.flags == 1 && (animateNode.flags & 0x07) == 0x07 &&
                    zClass_TypeList::Head(7) == nullptr &&
                    nearFloat(animateData.runtime.currentTime, 2.0f);
    zClass_TypeList::Head(11) = nullptr;
    zClass_TypeList::Tail(11) = nullptr;
    zClass_TypeList::Head(12) = nullptr;
    zClass_TypeList::Tail(12) = nullptr;
    free_zclass_type_lists_for_test();
    return ok ? 0 : 1;
}

extern "C" int zclass_object3d_set_matrix_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial data{};
    data.flags = 0x08;
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    float matrix[12] = {};
    for (int i = 0; i < 12; ++i) {
        matrix[i] = static_cast<float>(i + 1);
    }

    if (zClass_Object3D::gwObject3DSetMatrix(&node, matrix) != 0) {
        return 1;
    }

    bool copied = true;
    for (int i = 0; i < 12; ++i) {
        copied = copied && data.localMatrix[i] == matrix[i];
    }

    const bool ok = copied && (data.flags & 0x08) == 0 && (data.flags & 0x31) == 0x31 &&
                    (node.boundsFlags & 0x04) != 0 && (node.flags & 0x02000003) == 0x02000003 &&
                    zClass_TypeList::Head(7) != nullptr && zClass_TypeList::Head(7)->node == &node;

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    return ok ? 0 : 2;
}

extern "C" int zclass_node_free_and_deferred_work_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[2]{};
    g_zClass_NodeArray = slots;
    g_zClass_ActiveNodeCount = 2;
    g_zClass_NodeFreeHeadIndex = 0x123456;

    zClass_NodePartial *node = &slots[1].node;
    node->classId = 5;
    node->listA = static_cast<zClass_NodePartial **>(std::malloc(sizeof(zClass_NodePartial *)));
    node->listB = static_cast<zClass_NodePartial **>(std::malloc(sizeof(zClass_NodePartial *)));
    node->classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    slots[1].freeTag = 0xab000000;

    if (zClass_Class::FreeNodeToFreeList(node) != 0 || node->listA != nullptr ||
        node->listB != nullptr || node->classData != nullptr || node->classId != 0 ||
        g_zClass_ActiveNodeCount != 1 || g_zClass_NodeFreeHeadIndex != 1 ||
        (slots[1].freeTag & 0xff000000) != 0xab000000 ||
        (slots[1].freeTag & 0x00ffffff) != 0x123456) {
        return 1;
    }

    zClass_NodePartial blocked{};
    blocked.listCountA = 1;
    if (zClass_Class::FreeNodeToFreeList(&blocked) != 1) {
        return 2;
    }

    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 1;
    slots[0].node.classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    slots[0].node.classId = 5;
    zClass_TypeListLink *pending =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    pending->node = &slots[0].node;
    g_zClass_NodeList_PendingFreeHead = pending;
    g_zClass_TypeList_LiveLinkCount = 1;

    zClass_NodeList::ProcessPendingFrees();
    if (g_zClass_NodeList_PendingFreeHead != nullptr || slots[0].node.classData != nullptr ||
        g_zClass_TypeList_FreeLinkHead != pending || g_zClass_ActiveNodeCount != 0) {
        return 3;
    }
    zClass_TypeList::FreeAll();

    g_zClass_DeferredProcessingEnabled = 0;
    if (zClass::ProcessDeferredWork() != 1) {
        return 4;
    }
    g_zClass_DeferredProcessingEnabled = 1;
    zClass_TypeList::PendingRemovalDirty(3) = 1;
    if (zClass::ProcessDeferredWork() != 0 || zClass_TypeList::PendingRemovalDirty(3) != 0) {
        return 5;
    }

    return zClass_Class::FreeNodeToFreeList(nullptr) == 5 ? 0 : 6;
}

extern "C" int zclass_alloc_node_from_free_list_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[2]{};
    slots[1].freeTag = 0x00ffffff;
    slots[1].node.flags = 0xffffffff;
    slots[1].damageHandler = &slots[0];
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 1;
    g_zClass_ActiveNodeCount = 0;

    zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
    if (node != &slots[1].node || g_zClass_NodeFreeHeadIndex != -1 ||
        g_zClass_ActiveNodeCount != 1 || zClass_TypeList::Head(6) == nullptr ||
        zClass_TypeList::Head(6)->node != node) {
        return 1;
    }
    if (node->flags != 0x0108001c || node->callbackPriority != 1 || node->gridCol != -1 ||
        node->gridRow != -1 || node->nodeType != 0xff ||
        std::strcmp(node->name, "Default_node_name") != 0 || slots[1].damageHandler != nullptr) {
        return 2;
    }

    zClass_List::DeleteNodeFromLists(node);
    zClass::ProcessDeferredWork();
    zClass_Class::FreeNodeToFreeList(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Class::AllocNodeFromFreeList() == nullptr ? 0 : 3;
}

extern "C" int zclass_node_flag16_flag17_smoke() {
    zClass_NodePartial node{};

    if (zClass_Class::gwNodeSetFlag16(&node, 1) != 0 || (node.flags & 0x10000) == 0) {
        return 1;
    }
    if (zClass_Class::gwNodeSetFlag16(&node, 0) != 0 || (node.flags & 0x10000) != 0) {
        return 2;
    }
    if (zClass_Class::gwNodeSetFlag17(&node, 1) != 0 || (node.flags & 0x20000) == 0) {
        return 3;
    }
    if (zClass_Class::gwNodeSetFlag17(&node, 0) != 0 || (node.flags & 0x20000) != 0) {
        return 4;
    }

    return zClass_Class::gwNodeSetFlag16(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeSetFlag17(nullptr, 1) == 5
               ? 0
               : 5;
}

extern "C" int zclass_node_metadata_accessors_smoke() {
    zClass_NodePartial node{};
    node.userDataOrDiRef = 0x12345678;
    node.nodeType = 0xab;

    std::uint32_t userData = 0;
    std::int32_t value = 0;
    if (zClass_Class::gwNodeGetUserData(&node, &userData) != 0 || userData != 0x12345678) {
        return 1;
    }
    if (zClass_Class::gwNodeGetNodeType(&node, &value) != 0 || value != 0xab) {
        return 2;
    }
    if (zClass_Class::gwNodeSetNodeType(&node, 0x135) != 0 || node.nodeType != 0x35) {
        return 3;
    }
    if (zClass_Class::gwNodeSetName(&node, "short_name") != 0 ||
        std::strcmp(node.name, "short_name") != 0 ||
        zClass_Class::gwNodeGetName(&node) != node.name) {
        return 4;
    }

    std::memset(node.name, 'Z', sizeof(node.name));
    const char *longName = "abcdefghijklmnopqrstuvwxyz0123456789LONG";
    if (zClass_Class::gwNodeSetName(&node, longName) != 0 ||
        std::strncmp(node.name, longName, 0x22) != 0 || node.name[0x22] != 'Z' ||
        node.name[0x23] != '\0') {
        return 5;
    }

    return zClass_Class::gwNodeGetUserData(nullptr, &userData) == 5 &&
                   zClass_Class::gwNodeGetNodeType(nullptr, &value) == 5 &&
                   zClass_Class::gwNodeSetNodeType(nullptr, 0) == 5 &&
                   zClass_Class::gwNodeSetName(nullptr, "name") == 5 &&
                   zClass_Class::gwNodeGetName(nullptr) == nullptr
               ? 0
               : 6;
}

extern "C" int zclass_copy_node_display_instance_smoke() {
    reset_zclass_type_lists_for_test();

    zVec3 verts[1] = {{1.0f, 2.0f, 3.0f}};
    zDiPartial displayInstance = {};
    displayInstance.mode = 0;
    displayInstance.vertCount = 1;
    displayInstance.verts = verts;

    zClass_NodeFreeListSlot sourceSlot = {};
    zClass_NodeFreeListSlot destSlot = {};
    sourceSlot.node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&displayInstance));

    g_zClass_CopyNodeCloneDiMode = 0;
    g_zClass_CopyNodeDiArg0 = 0;
    g_zClass_CopyNodeDiArg1 = 0;
    if (zClass_cls_util::CopyNodeDisplayInstance(&sourceSlot.node, &destSlot.node) != 0 ||
        destSlot.node.userDataOrDiRef != sourceSlot.node.userDataOrDiRef ||
        displayInstance.refCount != 1) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    zClass_Class::gwNodeSetDisplayInstance(&destSlot.node, nullptr);
    std::memset(&destSlot, 0, sizeof(destSlot));
    displayInstance.refCount = 0;

    g_zClass_CopyNodeCloneDiMode = 1;
    g_zClass_CopyNodeDiArg0 = 1;
    g_zClass_CopyNodeDiArg1 = 1;
    if (zClass_cls_util::CopyNodeDisplayInstance(&sourceSlot.node, &destSlot.node) != 0 ||
        destSlot.node.userDataOrDiRef != sourceSlot.node.userDataOrDiRef ||
        displayInstance.refCount != 1) {
        free_zclass_type_lists_for_test();
        return 2;
    }

    zClass_Class::gwNodeSetDisplayInstance(&destSlot.node, nullptr);
    std::memset(&destSlot, 0, sizeof(destSlot));
    displayInstance.refCount = 0;
    displayInstance.flags = 0x04;

    zDiPartial clonePool[1] = {};
    clonePool[0].nextFreeIndex = -1;
    g_zModel_DiPoolBase = clonePool;
    g_zModel_DiPoolFreeHeadIndex = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zClass_CopyNodeCloneDiMode = 1;
    g_zClass_CopyNodeDiArg0 = 0;
    g_zClass_CopyNodeDiArg1 = 1;
    if (zClass_cls_util::CopyNodeDisplayInstance(&sourceSlot.node, &destSlot.node) != 0 ||
        destSlot.node.userDataOrDiRef !=
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&clonePool[0])) ||
        clonePool[0].refCount != 1 || clonePool[0].verts == verts) {
        zDi::FreeContents(&clonePool[0]);
        free_zclass_type_lists_for_test();
        return 3;
    }

    zClass_Class::gwNodeSetDisplayInstance(&destSlot.node, nullptr);
    zDi::FreeContents(&clonePool[0]);
    g_zModel_DiPoolBase = nullptr;
    free_zclass_type_lists_for_test();
    g_zClass_CopyNodeCloneDiMode = 0;
    g_zClass_CopyNodeDiArg0 = 0;
    g_zClass_CopyNodeDiArg1 = 0;
    return 0;
}

extern "C" int zclass_copy_node_base_data_smoke() {
    reset_zclass_type_lists_for_test();

    zVec3 verts[1] = {{-1.0f, 2.0f, 5.0f}};
    zDiPartial displayInstance = {};
    displayInstance.mode = 0;
    displayInstance.vertCount = 1;
    displayInstance.verts = verts;

    zClass_NodeFreeListSlot sourceSlot = {};
    zClass_NodeFreeListSlot destSlot = {};
    std::strcpy(sourceSlot.node.name, "copy_source");
    sourceSlot.node.flags = 0x70000000 | 0x00800000 | 0x00030000 | 0x000000fc;
    sourceSlot.node.auxFlags = 0x76543210;
    sourceSlot.node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&displayInstance));
    sourceSlot.node.callbackContext = &sourceSlot.node;
    sourceSlot.node.callbackPriority = 2;
    sourceSlot.node.actionCallback = &sourceSlot.node;
    sourceSlot.node.nodeType = 0x5a;

    destSlot.node.classId = 5;
    destSlot.node.flags = 0x01000000;
    g_zClass_CopyNodeCloneDiMode = 0;
    g_zClass_CopyNodeDiArg0 = 0;
    g_zClass_CopyNodeDiArg1 = 0;

    if (zClass_cls_util::CopyNodeBaseData(&sourceSlot.node, &destSlot.node) != 0) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    const std::int32_t expectedCopiedFlags = 0x70000000 | 0x00800000 | 0x00030000 | 0x000000fc;
    const bool ok =
        std::strcmp(destSlot.node.name, "copy_source") == 0 &&
        (destSlot.node.flags & expectedCopiedFlags) == expectedCopiedFlags &&
        (destSlot.node.flags & 0x01000000) == 0 && (destSlot.node.flags & 0x203) == 0x203 &&
        destSlot.node.auxFlags == sourceSlot.node.auxFlags &&
        destSlot.node.userDataOrDiRef == sourceSlot.node.userDataOrDiRef &&
        displayInstance.refCount == 1 && destSlot.node.callbackContext == nullptr &&
        destSlot.node.callbackPriority == 2 && destSlot.node.actionCallback == &sourceSlot.node &&
        destSlot.node.nodeType == 0x5a && zClass_TypeList::Head(2) != nullptr &&
        zClass_TypeList::Head(2)->node == &destSlot.node && zClass_TypeList::Head(7) != nullptr &&
        zClass_TypeList::Head(7)->node == &destSlot.node;

    zClass_Class::gwNodeSetDisplayInstance(&destSlot.node, nullptr);
    free_zclass_type_lists_for_test();
    return ok ? 0 : 2;
}

extern "C" int zclass_copy_node_unimplemented_stubs_smoke() {
    zClass_NodePartial node = {};
    return zClass_cls_util::CopyLightNode_Unimplemented(&node) == nullptr &&
                   zClass_cls_util::CopySoundNode_Unimplemented(&node) == nullptr &&
                   zClass_cls_util::CopyAnimateNode_Unimplemented(&node) == nullptr &&
                   zClass_cls_util::CopySequenceNode_Unimplemented(&node) == nullptr &&
                   zClass_cls_util::CopySwitchNode_Stub(&node) == nullptr
               ? 0
               : 1;
}

extern "C" int zclass_copy_camera_node_smoke() {
    reset_zclass_type_lists_for_test();

    zClass_NodeFreeListSlot cameraPool[1] = {};
    cameraPool[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = cameraPool;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zClass_NodeFreeListSlot sourceCameraSlot = {};
    zClass_CameraDataPartial sourceCameraData = {};
    zClass_NodePartial worldNode = {};
    zClass_NodePartial windowNode = {};
    int worldData = 0;

    std::strcpy(sourceCameraSlot.node.name, "source_camera");
    sourceCameraSlot.node.classId = 1;
    sourceCameraSlot.node.classData = &sourceCameraData;
    worldNode.classId = 2;
    worldNode.classData = &worldData;
    sourceCameraData.worldNode = &worldNode;
    sourceCameraData.windowNode = &windowNode;
    sourceCameraData.targetOrEuler = {4.0f, 5.0f, 6.0f};
    sourceCameraData.posOffset = {1.0f, 2.0f, 3.0f};
    sourceCameraData.nearClip = 0.25f;
    sourceCameraData.farClip = 500.0f;
    sourceCameraData.clipDistance = 10.0f;
    sourceCameraData.fovX = 1.0f;
    sourceCameraData.fovY = 0.5f;

    zClass_NodePartial *const cameraCopy =
        zClass_cls_util::CopyCameraNode(&sourceCameraSlot.node);
    if (cameraCopy != &cameraPool[0].node || cameraCopy->classId != 1 ||
        std::strcmp(cameraCopy->name, "source_camera") != 0) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    zClass_CameraDataPartial *const cameraCopyData =
        static_cast<zClass_CameraDataPartial *>(cameraCopy->classData);
    const bool cameraOk =
        cameraCopyData->worldNode == &worldNode && cameraCopyData->windowNode == &windowNode &&
        cameraCopyData->targetOrEuler.x == 4.0f && cameraCopyData->targetOrEuler.y == 5.0f &&
        cameraCopyData->targetOrEuler.z == 6.0f && cameraCopyData->posOffset.x == 1.0f &&
        cameraCopyData->posOffset.y == 2.0f && cameraCopyData->posOffset.z == 3.0f &&
        cameraCopyData->nearClip == 0.25f && cameraCopyData->farClip == 500.0f &&
        cameraCopyData->clipDistance == 10.0f &&
        cameraCopyData->invClipDistanceSq == 0.01f &&
        cameraCopyData->frustumWidth == 1.0f && cameraCopyData->frustumHeight == 0.5f;

    std::free(cameraCopyData);
    g_zClass_NodeArray = nullptr;
    free_zclass_type_lists_for_test();
    return cameraOk ? 0 : 2;
}

extern "C" int zclass_copy_object3d_and_lod_smoke() {
    reset_zclass_type_lists_for_test();

    zClass_NodeFreeListSlot objectPool[1] = {};
    objectPool[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = objectPool;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zClass_NodeFreeListSlot sourceObjectSlot = {};
    zClass_Object3DDataPartial sourceObjectData = {};
    std::strcpy(sourceObjectSlot.node.name, "source_object");
    sourceObjectSlot.node.classId = 5;
    sourceObjectSlot.node.classData = &sourceObjectData;
    sourceObjectData.alphaScale = 0.75f;
    sourceObjectData.flags = 0;
    sourceObjectData.rotation = {4.0f, 5.0f, 6.0f};
    sourceObjectData.scale = {7.0f, 8.0f, 9.0f};
    sourceObjectData.localMatrix[9] = 1.0f;
    sourceObjectData.localMatrix[10] = 2.0f;
    sourceObjectData.localMatrix[11] = 3.0f;

    zClass_NodePartial *const objectCopy =
        zClass_cls_util::CopyObject3DNode(&sourceObjectSlot.node);
    if (objectCopy != &objectPool[0].node || objectCopy->classId != 5 ||
        std::strcmp(objectCopy->name, "source_object") != 0) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    zClass_Object3DDataPartial *const objectCopyData =
        static_cast<zClass_Object3DDataPartial *>(objectCopy->classData);
    if (objectCopyData->alphaScale != 0.75f || objectCopyData->localMatrix[9] != 1.0f ||
        objectCopyData->localMatrix[10] != 2.0f || objectCopyData->localMatrix[11] != 3.0f ||
        objectCopyData->rotation.x != 4.0f || objectCopyData->rotation.y != 5.0f ||
        objectCopyData->rotation.z != 6.0f || objectCopyData->scale.x != 7.0f ||
        objectCopyData->scale.y != 8.0f || objectCopyData->scale.z != 9.0f) {
        std::free(objectCopyData);
        free_zclass_type_lists_for_test();
        return 2;
    }
    std::free(objectCopyData);
    free_zclass_type_lists_for_test();

    zClass_NodeFreeListSlot lodPool[1] = {};
    lodPool[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = lodPool;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    reset_zclass_type_lists_for_test();

    zClass_NodeFreeListSlot sourceLodSlot = {};
    zClass_NodePartial rangeNode = {};
    zClass_LodDataPartial sourceLodData = {};
    sourceLodSlot.node.classId = 6;
    sourceLodSlot.node.classData = &sourceLodData;
    sourceLodData.computeOwnDistance = 0;
    sourceLodData.nearRangeSq = 25.0f;
    sourceLodData.nearRange = 5.0f;
    sourceLodData.farRangeSq = 100.0f;
    sourceLodData.fadeWidth = {1.0f, 2.0f, 3.0f};
    sourceLodData.fadeAmount = {4.0f, 5.0f, 6.0f};
    sourceLodData.fadeEndScale = {7.0f, 8.0f, 9.0f};
    sourceLodData.fogFadeWidth = 10.0f;
    sourceLodData.fogFadeAmount = 11.0f;
    sourceLodData.fogStartDist = 12.0f;
    sourceLodData.vertexShadingAmount = 13.0f;
    sourceLodData.active = 0x44;
    sourceLodData.rangeNode = &rangeNode;
    sourceLodData.rangeSq = 16.0f;

    zClass_NodePartial *const lodCopy = zClass_cls_util::CopyLodNode(&sourceLodSlot.node);
    if (lodCopy != &lodPool[0].node || lodCopy->classId != 6) {
        free_zclass_type_lists_for_test();
        return 3;
    }

    zClass_LodDataPartial *const lodCopyData =
        static_cast<zClass_LodDataPartial *>(lodCopy->classData);
    const bool lodOk = lodCopyData->computeOwnDistance == 0 && lodCopyData->nearRangeSq == 25.0f &&
                       lodCopyData->nearRange == 5.0f && lodCopyData->farRangeSq == 100.0f &&
                       lodCopyData->fadeWidth.y == 2.0f && lodCopyData->fadeAmount.z == 6.0f &&
                       lodCopyData->fadeEndScale.x == 7.0f && lodCopyData->fogFadeWidth == 10.0f &&
                       lodCopyData->fogFadeAmount == 11.0f && lodCopyData->fogStartDist == 12.0f &&
                       lodCopyData->vertexShadingAmount == 13.0f && lodCopyData->active == 0x44 &&
                       lodCopyData->rangeNode == &rangeNode && lodCopyData->rangeSq == 16.0f;

    std::free(lodCopyData);
    g_zClass_NodeArray = nullptr;
    free_zclass_type_lists_for_test();
    return lodOk ? 0 : 4;
}

extern "C" int zclass_copy_node_dispatch_and_wrappers_smoke() {
    zClass_NodePartial sharedNode = {};
    sharedNode.flags = 0x04000000;
    g_zClass_CopyNodeCloneDiMode = 0x11;
    g_zClass_CopyNodeDiArg0 = 0x22;
    g_zClass_CopyNodeDiArg1 = 0x33;

    if (zClass_cls_util::CopyNodeDispatch(nullptr) != nullptr ||
        zClass_cls_util::CopyNodeDispatch(&sharedNode) != &sharedNode) {
        return 1;
    }

    if (zClass_cls_util::CopyNodeWithCloneOptions(&sharedNode, 0x44, 0x55) != &sharedNode ||
        g_zClass_CopyNodeCloneDiMode != 0x11 || g_zClass_CopyNodeDiArg0 != 0x22 ||
        g_zClass_CopyNodeDiArg1 != 0x33) {
        return 2;
    }

    if (zClass_cls_util::CopyNode(&sharedNode, 0x66, 0x77, 0x88) != &sharedNode ||
        g_zClass_CopyNodeCloneDiMode != 0x11 || g_zClass_CopyNodeDiArg0 != 0x22 ||
        g_zClass_CopyNodeDiArg1 != 0x33) {
        return 3;
    }

    zClass_NodePartial worldNode = {};
    worldNode.classId = 2;
    zClass_NodePartial unknownNode = {};
    unknownNode.classId = 99;
    return zClass_cls_util::CopyNodeDispatch(&worldNode) == nullptr &&
                   zClass_cls_util::CopyNodeDispatch(&unknownNode) == nullptr &&
                   zClass_cls_util::CopyNodeWithCloneOptions(nullptr, 1, 2) == nullptr &&
                   zClass_cls_util::CopyNode(nullptr, 1, 2, 3) == nullptr
               ? 0
               : 4;
}

extern "C" int zclass_node_action_callback_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial node{};
    node.callbackPriority = 3;
    if (zClass_Class::gwNodeSetActionCallback(&node, &node) != 0 || node.actionCallback != &node ||
        zClass_TypeList::Head(3) == nullptr || zClass_TypeList::Head(3)->node != &node) {
        return 1;
    }
    if (zClass_Class::gwNodeSetActionCallback(&node, nullptr) != 0 ||
        node.actionCallback != nullptr || zClass_TypeList::PendingRemovalDirty(3) == 0) {
        return 2;
    }
    zClass::ProcessDeferredWork();
    if (zClass_TypeList::Head(3) != nullptr) {
        return 3;
    }

    zClass_NodePartial first{};
    zClass_NodePartial second{};
    first.callbackPriority = 4;
    second.callbackPriority = 4;
    if (zClass_Class::gwNodeSetActionCallback(&first, &first) != 0 ||
        zClass_Class::gwNodeSetActionCallbackTail(&second, &second) != 0 ||
        zClass_TypeList::Head(4)->node != &first || zClass_TypeList::Tail(4)->node != &second) {
        return 4;
    }

    zClass_List::DeleteNodeFromLists(&first);
    zClass_List::DeleteNodeFromLists(&second);
    zClass::ProcessDeferredWork();
    zClass_TypeList::FreeAll();

    zClass_NodePartial invalid{};
    invalid.callbackPriority = 6;
    return zClass_Class::gwNodeSetActionCallback(nullptr, &node) == 5 &&
                   zClass_Class::gwNodeSetActionCallbackTail(nullptr, &node) == 5 &&
                   zClass_Class::gwNodeSetActionCallback(&invalid, &invalid) == 1
               ? 0
               : 5;
}

extern "C" int zclass_node_priority_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial node{};
    node.callbackPriority = 1;
    if (zClass_Class::gwNodeSetActionCallback(&node, &node) != 0 ||
        zClass_Class::gwNodeSetPriority(&node, 2) != 0 || node.callbackPriority != 2 ||
        zClass_TypeList::PendingRemovalDirty(1) == 0 || zClass_TypeList::Head(2) == nullptr ||
        zClass_TypeList::Head(2)->node != &node) {
        return 1;
    }

    zClass::ProcessDeferredWork();
    if (zClass_TypeList::Head(1) != nullptr || zClass_TypeList::Head(2) == nullptr) {
        return 2;
    }

    zClass_Class::gwNodeSetPriority(&node, 9);
    if (node.callbackPriority != 9 || zClass_TypeList::PendingRemovalDirty(2) == 0) {
        return 3;
    }
    zClass::ProcessDeferredWork();
    zClass_TypeList::FreeAll();

    return zClass_Class::gwNodeSetPriority(nullptr, 0) == 5 ? 0 : 4;
}

extern "C" int zclass_node_pick_flag_accessors_smoke() {
    zClass_NodePartial node{};
    std::int32_t value = 0;

    if (zClass_Class::gwNodeSetCellPickable(&node, 1) != 0 ||
        zClass_Class::gwNodeGetCellPickable(&node, &value) != 0 || value != 1) {
        return 1;
    }
    if (zClass_Class::gwNodeSetCellPickable(&node, 0) != 0 ||
        zClass_Class::gwNodeGetCellPickable(&node, &value) != 0 || value != 0) {
        return 2;
    }
    if (zClass_Class::gwNodeSetRaycastable(&node, 1) != 0 ||
        zClass_Class::gwNodeGetRaycastable(&node, &value) != 0 || value != 1) {
        return 3;
    }
    if (zClass_Class::gwNodeSetRaycastable(&node, 0) != 0 ||
        zClass_Class::gwNodeGetRaycastable(&node, &value) != 0 || value != 0) {
        return 4;
    }
    if (zClass_Class::gwNodeSetPickable(&node, 1) != 0 ||
        zClass_Class::gwNodeGetPickable(&node, &value) != 0 || value != 1) {
        return 5;
    }
    if (zClass_Class::gwNodeSetPickable(&node, 0) != 0 ||
        zClass_Class::gwNodeGetPickable(&node, &value) != 0 || value != 0) {
        return 6;
    }

    return zClass_Class::gwNodeSetCellPickable(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeGetCellPickable(nullptr, &value) == 5 &&
                   zClass_Class::gwNodeSetRaycastable(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeGetRaycastable(nullptr, &value) == 5 &&
                   zClass_Class::gwNodeSetPickable(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeGetPickable(nullptr, &value) == 5
               ? 0
               : 7;
}

extern "C" int zclass_node_extra_flag_setters_smoke() {
    zClass_NodePartial node{};
    zClass_NodePartial child{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *children[] = {&child};
    zClass_NodePartial *grandchildren[] = {&grandchild};

    if (zClass_Class::gwNodeSetHasHitCallback(&node, 1) != 0 || (node.flags & 0x40) == 0) {
        return 1;
    }
    if (zClass_Class::gwNodeSetHasHitCallback(&node, 0) != 0 || (node.flags & 0x40) != 0) {
        return 2;
    }
    if (zClass_Class::gwNodeSetBypassFarClip(&node, 1) != 0 || (node.flags & 0x80) == 0) {
        return 3;
    }
    if (zClass_Class::gwNodeSetBypassFarClip(&node, 0) != 0 || (node.flags & 0x80) != 0) {
        return 4;
    }

    node.flags = 0x01000000;
    if (zClass_Class::gwNodeClearVariantGate(&node, 1) != 0 || (node.flags & 0x01000000) == 0) {
        return 5;
    }
    if (zClass_Class::gwNodeClearVariantGate(&node, 0) != 0 || (node.flags & 0x01000000) != 0) {
        return 6;
    }

    node.listCountB = 1;
    node.listB = children;
    child.listCountB = 1;
    child.listB = grandchildren;
    zClass_Node::PropagateExtraFlagsRecursive(&node, 0x22);
    if (node.auxFlags != 0x22 || child.auxFlags != 0x22 || grandchild.auxFlags != 0x22) {
        return 7;
    }

    child.auxFlags = 0x3f;
    grandchild.auxFlags = 0x15;
    zClass_Node::MaskExtraFlagsRecursive(&node, 0x14);
    if (node.auxFlags != 0x00 || child.auxFlags != 0x14 || grandchild.auxFlags != 0x14) {
        return 10;
    }

    zClass_Node::PropagateFlagsRecursive(&node, 0x400);
    if ((node.flags & 0x400) == 0 || (child.flags & 0x400) == 0 ||
        (grandchild.flags & 0x400) == 0) {
        return 11;
    }

    zClass_Node::SetContextRecursive(&node, &node, 0x200000);
    if ((node.flags & 0x200000) == 0 || (child.flags & 0x200000) == 0 ||
        (grandchild.flags & 0x200000) == 0 || node.callbackContext != &node ||
        child.callbackContext != &node || grandchild.callbackContext != &node) {
        return 8;
    }

    return zClass_Class::gwNodeSetHasHitCallback(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeSetBypassFarClip(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeClearVariantGate(nullptr, 0) == 5
               ? 0
               : 9;
}

extern "C" int zclass_node_vertex_alpha_and_root_smoke() {
    zClass_NodePartial node{};

    if (zClass_Class::gwNodeSetVertexAlphaOverride(&node, 1) != 0 ||
        (node.flags & 0x00800000) == 0) {
        return 1;
    }
    if (zClass_Class::gwNodeSetVertexAlphaOverride(&node, 0) != 0 ||
        (node.flags & 0x00800000) != 0) {
        return 2;
    }

    zClass_NodePartial root{};
    zClass_NodePartial mid{};
    zClass_NodePartial child{};
    zClass_NodePartial *midParentList[] = {&root};
    zClass_NodePartial *childParentList[] = {&mid};
    mid.listCountA = 1;
    mid.listA = midParentList;
    child.listCountA = 1;
    child.listA = childParentList;

    if (zClass_Class::gwNodeGetRoot(&child) != &root) {
        return 3;
    }
    if (zClass_Class::gwNodeGetRoot(nullptr) != nullptr) {
        return 4;
    }

    zClass_NodePartial secondParent{};
    zClass_NodePartial *multiParentList[] = {&root, &secondParent};
    child.listCountA = 2;
    child.listA = multiParentList;
    if (zClass_Class::gwNodeGetRoot(&child) != nullptr) {
        return 5;
    }

    return zClass_Class::gwNodeSetVertexAlphaOverride(nullptr, 1) == 5 ? 0 : 6;
}

extern "C" int zclass_node_world_child_smoke() {
    zClass_NodePartial world{};
    zClass_NodePartial mid{};
    zClass_NodePartial child{};
    zClass_NodePartial *midParents[] = {&world};
    zClass_NodePartial *childParents[] = {&mid};
    world.classId = 2;
    mid.listCountA = 1;
    mid.listA = midParents;
    child.listCountA = 1;
    child.listA = childParents;

    if (zClass_Class::gwNodeGetWorldChild(&child) != &mid ||
        zClass_Class::gwNodeGetWorldChild(&world) != nullptr ||
        zClass_Class::gwNodeGetWorldChild(nullptr) != nullptr) {
        return 1;
    }

    zClass_NodePartial otherParent{};
    zClass_NodePartial *multiParents[] = {&world, &otherParent};
    child.listCountA = 2;
    child.listA = multiParents;
    return zClass_Class::gwNodeGetWorldChild(&child) == nullptr ? 0 : 2;
}

extern "C" int zclass_gwnode_build_node_to_ancestor_matrix_smoke() {
    int flags[2] = {};
    float *slots[2] = {};
    zMat4x3 matrix{};
    zMath::g_currentMatrixIdentityFlagSlot = &flags[0];
    zMath::g_currentMatrixPtrSlot = &slots[0];
    zMath::MatStackPushPtr(reinterpret_cast<float *>(&matrix));

    if (gwNode::BuildNodeToAncestorMatrix(nullptr, 1) != 5) {
        zMath::MatStackPopPtr();
        return 1;
    }

    zClass_Object3DDataPartial data{};
    zClass_NodePartial node{};
    node.classId = 5;
    node.flags = 0x00080000;
    node.classData = &data;
    data.cachedWorldMatrix[0] = 1.0f;
    data.cachedWorldMatrix[4] = 1.0f;
    data.cachedWorldMatrix[8] = 1.0f;
    data.cachedWorldMatrix[9] = 3.0f;
    data.cachedWorldMatrix[10] = 4.0f;
    data.cachedWorldMatrix[11] = 5.0f;

    zMath::MatLoadIdentity();
    if (gwNode::BuildNodeToAncestorMatrix(&node, 1) != 0 || matrix.posX != 3.0f ||
        matrix.posY != 4.0f || matrix.posZ != 5.0f) {
        zMath::MatStackPopPtr();
        return 2;
    }

    data.flags = 0x20;
    data.localMatrix[0] = 1.0f;
    data.localMatrix[4] = 1.0f;
    data.localMatrix[8] = 1.0f;
    data.localMatrix[9] = 7.0f;
    data.localMatrix[10] = 8.0f;
    data.localMatrix[11] = 9.0f;
    zMath::MatLoadIdentity();
    if (gwNode::BuildNodeToAncestorMatrix(&node, 1) != 0 || matrix.posX != 7.0f ||
        matrix.posY != 8.0f || matrix.posZ != 9.0f || data.cachedWorldMatrix[9] != 7.0f ||
        data.cachedWorldMatrix[10] != 8.0f || data.cachedWorldMatrix[11] != 9.0f ||
        (data.flags & 0x20) != 0) {
        zMath::MatStackPopPtr();
        return 3;
    }

    zMath::MatStackPopPtr();
    return 0;
}

extern "C" int zclass_gwnode_get_world_position_smoke() {
    int flags[3] = {};
    float *slots[3] = {};
    zMath::g_currentMatrixIdentityFlagSlot = &flags[0];
    zMath::g_currentMatrixPtrSlot = &slots[0];

    zVec3 outPosition{9.0f, 9.0f, 9.0f};
    if (gwNode::GetWorldPosition(nullptr, &outPosition) != 1) {
        return 1;
    }

    zClass_Object3DDataPartial data{};
    zClass_NodePartial node{};
    node.classId = 5;
    node.flags = 0x00080000;
    node.classData = &data;
    data.cachedWorldMatrix[9] = 1.0f;
    data.cachedWorldMatrix[10] = 2.0f;
    data.cachedWorldMatrix[11] = 3.0f;
    if (gwNode::GetWorldPosition(&node, &outPosition) != 0 || outPosition.x != 1.0f ||
        outPosition.y != 2.0f || outPosition.z != 3.0f) {
        return 2;
    }

    data.flags = 0x20;
    data.localMatrix[0] = 1.0f;
    data.localMatrix[4] = 1.0f;
    data.localMatrix[8] = 1.0f;
    data.localMatrix[9] = 4.0f;
    data.localMatrix[10] = 5.0f;
    data.localMatrix[11] = 6.0f;
    if (gwNode::GetWorldPosition(&node, &outPosition) != 0) {
        return 3;
    }

    if (outPosition.x != 4.0f || outPosition.y != 5.0f || outPosition.z != 6.0f ||
        (data.flags & 0x20) != 0) {
        return 4;
    }

    zVec3 point{};
    if (gwNode::TransformPoint(nullptr, &point) != 1 ||
        gwNode::TransformPoint(&node, &point) != 0 || point.x != 4.0f || point.y != 5.0f ||
        point.z != 6.0f) {
        return 5;
    }

    data.flags = 0x20;
    data.localMatrix[9] = 10.0f;
    data.localMatrix[10] = 20.0f;
    data.localMatrix[11] = 30.0f;
    point = {1.0f, 2.0f, 3.0f};
    if (gwNode::TransformPoint(&node, &point) != 0 || point.x != 11.0f || point.y != 22.0f ||
        point.z != 33.0f) {
        return 6;
    }

    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.0001f; };
    zVec3 orientation{};
    if (gwNode::GetWorldPosAndOrientation(nullptr, &point, &orientation) != 1) {
        return 7;
    }

    data.flags = 0x20;
    data.localMatrix[0] = 1.0f;
    data.localMatrix[4] = 1.0f;
    data.localMatrix[8] = 1.0f;
    data.localMatrix[9] = 10.0f;
    data.localMatrix[10] = 20.0f;
    data.localMatrix[11] = 30.0f;
    point = {};
    if (gwNode::GetWorldPosAndOrientation(&node, &point, &orientation) != 0 ||
        !nearFloat(point.x, 10.0f) || !nearFloat(point.y, 20.0f) ||
        !nearFloat(point.z, 30.0f) || !nearFloat(orientation.x, 0.0f) ||
        !nearFloat(orientation.y, 0.0f) || !nearFloat(orientation.z, 1.57079637f)) {
        return 8;
    }

    data.flags = 0x20;
    point = {1.0f, 2.0f, 3.0f};
    if (gwNode::GetWorldPosAndOrientation(&node, &point, &orientation) != 0 ||
        !nearFloat(point.x, 11.0f) || !nearFloat(point.y, 22.0f) ||
        !nearFloat(point.z, 33.0f) || !nearFloat(orientation.x, 0.0f) ||
        !nearFloat(orientation.y, 0.0f) || !nearFloat(orientation.z, 1.57079637f)) {
        return 9;
    }

    return 0;
}

extern "C" int zclass_world_set_size_smoke() {
    zClass_WorldDataPartial worldData{};
    worldData.originX = -5.5f;
    worldData.originZ = 10.25f;

    zClass_NodePartial world{};
    world.classData = &worldData;

    if (zClass_World::gwWorldSetSize(&world, 12.0f, -4.5f) != 0) {
        return 1;
    }

    if (worldData.worldSizeX != 12.0f || worldData.worldSizeZ != -4.5f ||
        worldData.worldMaxX != 6.5f || worldData.worldMaxZ != 5.75f) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_world_set_origin_smoke() {
    zClass_WorldDataPartial worldData{};
    worldData.worldSizeX = 12.0f;
    worldData.worldSizeZ = -4.5f;

    zClass_NodePartial world{};
    world.classData = &worldData;

    if (zClass_World::gwWorldSetOrigin(&world, -5.5f, 10.25f) != 0) {
        return 1;
    }

    if (worldData.originX != -5.5f || worldData.originZ != 10.25f ||
        worldData.worldMaxX != 6.5f || worldData.worldMaxZ != 5.75f) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_world_partition_tolerance_smoke() {
    zClass_WorldDataPartial worldData{};
    zClass_NodePartial world{};
    world.classData = &worldData;

    if (zClass_World::gwWorldSetPartitionInclusionTolerance(&world, 1.25f, -2.5f) != 0) {
        return 1;
    }

    if (worldData.partitionInclusionTolX != 1.25f ||
        worldData.partitionInclusionTolZ != -2.5f) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_world_max_dec_features_smoke() {
    zClass_WorldDataPartial worldData{};
    zClass_NodePartial world{};
    world.classData = &worldData;

    if (zClass_World::gwWorldSetMaxDecFeatures(&world, 17) != 0 ||
        worldData.partitionMaxDecFeatureCount != 17) {
        return 1;
    }

    if (zClass_World::gwWorldSetMaxDecFeatures(&world, 300) != 0 ||
        worldData.partitionMaxDecFeatureCount != 255) {
        return 2;
    }

    if (zClass_World::gwWorldSetMaxDecFeatures(&world, -1) != 0 ||
        worldData.partitionMaxDecFeatureCount != 255) {
        return 3;
    }

    return 0;
}

extern "C" int zclass_world_virtual_partition_statics_smoke() {
    reset_zclass_type_lists_for_test();
    g_zClass_NodeList_PendingFreeHead = nullptr;

    zClass_NodeFreeListSlot staticsSlot{};
    staticsSlot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &staticsSlot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zClass_NodePartial world{};
    zClass_WorldDataPartial worldData{};
    world.classId = 2;
    world.classData = &worldData;
    if (zClass_World::InitVirtualAreaPartitions(&world) != 5) {
        free_zclass_type_lists_for_test();
        return 1;
    }
    if (zClass_World::SetVirtualPartition(&world, 0) != 0 ||
        worldData.clampQueriesToBounds != 0) {
        free_zclass_type_lists_for_test();
        return 2;
    }

    zWorldAreaPartial row0[2]{};
    zWorldAreaPartial row1[2]{};
    zWorldAreaPartial *rows[] = {row0, row1};
    worldData.areaGridRows = rows;
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 2;
    worldData.areaGridExternalOwnership = 1;
    worldData.originX = 0.0f;
    worldData.originZ = 100.0f;
    worldData.worldSizeX = 100.0f;
    worldData.worldSizeZ = -100.0f;
    worldData.worldMaxX = 100.0f;
    worldData.worldMaxZ = 0.0f;
    worldData.areaInvSizeX = 0.02f;
    worldData.areaInvSizeZ = -0.02f;
    worldData.partitionInclusionTolX = 2.0f;
    worldData.partitionInclusionTolZ = 2.0f;

    zClass_NodePartial edgeChild{};
    edgeChild.gridCol = 0;
    edgeChild.gridRow = 0;
    edgeChild.listA = static_cast<zClass_NodePartial **>(
        std::calloc(1, sizeof(zClass_NodePartial *)));
    row0[0].childList = static_cast<zClass_NodePartial **>(
        std::calloc(1, sizeof(zClass_NodePartial *)));
    if (edgeChild.listA == nullptr || row0[0].childList == nullptr) {
        std::free(edgeChild.listA);
        std::free(row0[0].childList);
        free_zclass_type_lists_for_test();
        return 3;
    }
    edgeChild.listA[0] = &world;
    edgeChild.listCountA = 1;
    row0[0].childList[0] = &edgeChild;
    row0[0].childCount = 1;

    if (zClass_World::SetVirtualPartition(&world, 1) != 0 ||
        worldData.clampQueriesToBounds != 1 || row0[0].childCount != 0 ||
        world.listCountB != 1) {
        zClass_World::FreeVirtualAreaPartitions(&world);
        std::free(edgeChild.listA);
        free_zclass_type_lists_for_test();
        return 4;
    }

    zClass_NodePartial *statics = world.listB[0];
    const bool movedToStatics =
        statics == &staticsSlot.node && std::strcmp(statics->name, "VAP_statics") == 0 &&
        statics->listCountB == 1 && statics->listB[0] == &edgeChild &&
        edgeChild.listCountA == 1 && edgeChild.listA[0] == statics;

    zClass_World::FreeVirtualAreaPartitions(&world);
    std::free(world.listB);
    std::free(statics->listA);
    std::free(statics->listB);
    std::free(statics->classData);
    std::free(edgeChild.listA);
    world.listB = nullptr;
    statics->listA = nullptr;
    statics->listB = nullptr;
    statics->classData = nullptr;
    edgeChild.listA = nullptr;
    free_zclass_type_lists_for_test();
    return movedToStatics ? 0 : 5;
}

extern "C" int zclass_world_add_child_at_grid_smoke() {
    zWorldAreaPartial row0[2]{};
    zWorldAreaPartial row1[2]{};
    row0[0].areaFlags = 1;
    row0[0].cellMinX = 0.0f;
    row0[0].cellMinZ = 100.0f;
    row0[1].areaFlags = 1;
    row0[1].cellMinX = 50.0f;
    row0[1].cellMinZ = 100.0f;
    row1[0].areaFlags = 1;
    row1[0].cellMinX = 0.0f;
    row1[0].cellMinZ = 50.0f;
    row1[1].areaFlags = 1;
    row1[1].cellMinX = 50.0f;
    row1[1].cellMinZ = 50.0f;
    zWorldAreaPartial *rows[] = {row0, row1};

    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 100.0f;
    worldData.worldSizeX = 100.0f;
    worldData.worldSizeZ = -100.0f;
    worldData.worldMaxX = 100.0f;
    worldData.worldMaxZ = 0.0f;
    worldData.areaCellSizeX = 50.0f;
    worldData.areaCellSizeZ = -50.0f;
    worldData.areaInvSizeX = 0.02f;
    worldData.areaInvSizeZ = -0.02f;
    worldData.partitionInclusionTolX = 2.0f;
    worldData.partitionInclusionTolZ = 2.0f;
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 2;
    worldData.areaGridRows = rows;

    zClass_NodePartial world{};
    world.flags = 1;
    world.classData = &worldData;

    zClass_NodePartial wholeWorldChild{};
    if (zClass_World::AddChildAtGrid(&world, &wholeWorldChild) != 0 || world.listCountB != 1 ||
        world.listB[0] != &wholeWorldChild || wholeWorldChild.gridCol != -1 ||
        wholeWorldChild.gridRow != -1) {
        return 1;
    }

    zClass_NodePartial outsideChild{};
    outsideChild.flags = 0x80;
    if (zClass_World::AddChildAtGrid(&world, &outsideChild) != 0 || world.listCountB != 2 ||
        world.listB[1] != &outsideChild || outsideChild.gridCol != -1 ||
        outsideChild.gridRow != -1) {
        return 2;
    }

    std::int32_t classData = 0;
    zClass_NodePartial bboxChild{};
    bboxChild.flags = 0x100;
    bboxChild.classData = &classData;
    bboxChild.cachedBounds[0] = 10.0f;
    bboxChild.cachedBounds[1] = 0.0f;
    bboxChild.cachedBounds[2] = 70.0f;
    bboxChild.cachedBounds[3] = 20.0f;
    bboxChild.cachedBounds[4] = 1.0f;
    bboxChild.cachedBounds[5] = 80.0f;
    if (zClass_World::AddChildAtGrid(&world, &bboxChild) != 0 || row0[0].childCount != 1 ||
        row0[0].childList[0] != &bboxChild || bboxChild.gridCol != 0 || bboxChild.gridRow != 0) {
        return 3;
    }

    std::free(row0[0].childList);
    std::free(wholeWorldChild.listA);
    std::free(outsideChild.listA);
    std::free(bboxChild.listA);
    std::free(world.listB);
    return 0;
}

extern "C" int zclass_world_remove_light_sound_smoke() {
    zClass_WorldDataPartial addWorldData{};
    zClass_NodePartial addWorld{};
    addWorld.classData = &addWorldData;

    zClass_NodePartial addedLight{};
    zClass_LightDataPartial addedLightData{};
    addedLight.classData = &addedLightData;
    if (zClass_World::AddLight(&addWorld, &addedLight) != 0 || addWorldData.lightCount != 1 ||
        addWorldData.lightNodes[0] != &addedLight ||
        addWorldData.lightDataList[0] != &addedLightData ||
        addedLightData.attachedWorldCount != 1 || addedLightData.attachedWorlds[0] != &addWorld) {
        return 10;
    }
    if (zClass_World::RemoveLight(&addWorld, &addedLight) != 0 || addWorldData.lightCount != 0 ||
        addedLightData.attachedWorldCount != 0) {
        return 11;
    }
    std::free(addWorldData.lightNodes);
    std::free(addWorldData.lightDataList);
    std::free(addedLightData.attachedWorlds);

    zClass_NodePartial addedSound{};
    zClass_SoundDataPartial addedSoundData{};
    addedSound.classData = &addedSoundData;
    if (zClass_World::AddSound(&addWorld, &addedSound) != 0 || addWorldData.soundCount != 1 ||
        addWorldData.soundNodes[0] != &addedSound ||
        addWorldData.soundDataList[0] != &addedSoundData ||
        addedSoundData.attachedWorldCount != 1 || addedSoundData.attachedWorlds[0] != &addWorld) {
        return 12;
    }
    if (zClass_World::RemoveSound(&addWorld, &addedSound) != 0 || addWorldData.soundCount != 0 ||
        addedSoundData.attachedWorldCount != 0) {
        return 13;
    }
    std::free(addWorldData.soundNodes);
    std::free(addWorldData.soundDataList);
    std::free(addedSoundData.attachedWorlds);

    zClass_WorldDataPartial worldData{};
    zClass_NodePartial world{};
    world.classData = &worldData;

    zClass_NodePartial otherWorld{};
    zClass_NodePartial extraWorld{};

    zClass_NodePartial lightA{};
    zClass_NodePartial lightB{};
    zClass_NodePartial lightC{};
    zClass_LightDataPartial lightDataA{};
    zClass_LightDataPartial lightDataB{};
    zClass_LightDataPartial lightDataC{};
    zClass_NodePartial *lightNodes[] = {&lightA, &lightB, &lightC};
    zClass_LightDataPartial *lightData[] = {&lightDataA, &lightDataB, &lightDataC};
    zClass_NodePartial *lightWorlds[] = {&otherWorld, &world, &extraWorld};
    lightDataB.attachedWorldCount = 3;
    lightDataB.attachedWorlds = lightWorlds;
    worldData.lightCount = 3;
    worldData.lightNodes = lightNodes;
    worldData.lightDataList = lightData;

    if (zClass_World::RemoveLight(&world, &lightB) != 0 || worldData.lightCount != 2 ||
        worldData.lightNodes[0] != &lightA || worldData.lightNodes[1] != &lightC ||
        worldData.lightDataList[0] != &lightDataA || worldData.lightDataList[1] != &lightDataC ||
        lightDataB.attachedWorldCount != 2 || lightDataB.attachedWorlds[0] != &otherWorld ||
        lightDataB.attachedWorlds[1] != &extraWorld) {
        return 1;
    }

    zClass_NodePartial soundA{};
    zClass_NodePartial soundB{};
    zClass_NodePartial soundC{};
    zClass_SoundDataPartial soundDataA{};
    zClass_SoundDataPartial soundDataB{};
    zClass_SoundDataPartial soundDataC{};
    zClass_NodePartial *soundNodes[] = {&soundA, &soundB, &soundC};
    zClass_SoundDataPartial *soundData[] = {&soundDataA, &soundDataB, &soundDataC};
    zClass_NodePartial *soundWorlds[] = {&world, &otherWorld};
    soundDataA.attachedWorldCount = 2;
    soundDataA.attachedWorlds = soundWorlds;
    worldData.soundCount = 3;
    worldData.soundNodes = soundNodes;
    worldData.soundDataList = soundData;

    if (zClass_World::RemoveSound(&world, &soundA) != 0 || worldData.soundCount != 2 ||
        worldData.soundNodes[0] != &soundB || worldData.soundNodes[1] != &soundC ||
        worldData.soundDataList[0] != &soundDataB || worldData.soundDataList[1] != &soundDataC ||
        soundDataA.attachedWorldCount != 1 || soundDataA.attachedWorlds[0] != &otherWorld) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_child_generic_link_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodePartial parent{};
    zClass_NodePartial otherParent{};
    zClass_NodePartial child{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *childChildren[] = {&grandchild};
    child.flags = 0x00080000;
    grandchild.flags = 0x00080000;
    child.listCountB = 1;
    child.listB = childChildren;

    if (zClass_Class::AddChildGeneric(&parent, &child) != 0 || parent.listCountB != 1 ||
        parent.listB[0] != &child || child.listCountA != 1 || child.listA[0] != &parent ||
        (parent.boundsFlags & 2) == 0 || (parent.flags & 3) != 3 ||
        zClass_TypeList::Head(7) == nullptr || zClass_TypeList::Head(7)->node != &parent) {
        return 1;
    }

    if (zClass_Class::AddChildGeneric(&otherParent, &child) != 0 || child.listCountA != 2 ||
        child.listA[1] != &otherParent || (child.flags & 0x00080000) != 0 ||
        (grandchild.flags & 0x00080000) != 0) {
        return 2;
    }

    int validatedClassData = 0;
    zClass_NodePartial validatedParent{};
    zClass_NodePartial validatedChild{};
    validatedParent.classData = &validatedClassData;
    if (zClass_Class::AddChildValidated(&validatedParent, &validatedChild) != 0 ||
        validatedParent.listCountB != 1 || validatedParent.listB[0] != &validatedChild ||
        validatedChild.listCountA != 1 || validatedChild.listA[0] != &validatedParent) {
        return 3;
    }
    if (zClass_Class::AddChildValidated(nullptr, &validatedChild) != 5 ||
        zClass_Class::AddChildValidated(&validatedParent, nullptr) != 5) {
        return 4;
    }
    validatedParent.classData = nullptr;
    if (zClass_Class::AddChildValidated(&validatedParent, &validatedChild) != 5) {
        return 5;
    }

    zClass_NodePartial dispatchParent{};
    zClass_NodePartial dispatchChild{};
    dispatchParent.classId = 3;
    if (zClass_Class::AddChild(&dispatchParent, &dispatchChild) != 0 ||
        dispatchParent.listCountB != 1 || dispatchParent.listB[0] != &dispatchChild ||
        dispatchChild.listCountA != 1 || dispatchChild.listA[0] != &dispatchParent) {
        return 6;
    }

    zClass_NodePartial animateParent{};
    zClass_NodePartial animateChild{};
    animateParent.classId = 8;
    if (zClass_Class::AddChild(&animateParent, &animateChild) != 0 ||
        animateParent.listCountB != 1 || animateChild.listCountA != 1 ||
        zClass_Animate::AddChild(nullptr, &animateChild) != 5 ||
        zClass_Animate::AddChild(&animateParent, nullptr) != 5) {
        return 7;
    }

    zClass_NodePartial sequenceParent{};
    zClass_NodePartial sequenceChild{};
    sequenceParent.classId = 7;
    if (zClass_Class::AddChild(&sequenceParent, &sequenceChild) != 1 ||
        zClass_Class::AddChild(nullptr, &sequenceChild) != 5 ||
        zClass_Class::AddChild(&sequenceParent, nullptr) != 5) {
        return 8;
    }

    std::free(parent.listB);
    std::free(otherParent.listB);
    std::free(child.listA);
    std::free(validatedParent.listB);
    std::free(validatedChild.listA);
    std::free(dispatchParent.listB);
    std::free(dispatchChild.listA);
    std::free(animateParent.listB);
    std::free(animateChild.listA);
    zClass_TypeList::FreeAll();
    zClass_TypeList::FreeAll();
    return 0;
}

extern "C" int zclass_child_generic_remove_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodePartial parent{};
    zClass_NodePartial remainingParent{};
    zClass_NodePartial child{};
    zClass_NodePartial sibling{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *parentChildren[] = {&child, &sibling};
    zClass_NodePartial *childParents[] = {&parent, &remainingParent};
    zClass_NodePartial *childChildren[] = {&grandchild};
    parent.flags = 0x00080000;
    parent.listCountB = 2;
    parent.listB = parentChildren;
    child.listCountA = 2;
    child.listA = childParents;
    child.listCountB = 1;
    child.listB = childChildren;

    if (zClass_Class::RemoveChildGeneric(&parent, &child) != 0 || parent.listCountB != 1 ||
        parent.listB[0] != &sibling || child.listCountA != 1 ||
        child.listA[0] != &remainingParent || (child.flags & 0x00080000) == 0 ||
        (grandchild.flags & 0x00080000) == 0 || (parent.boundsFlags & 2) == 0 ||
        (parent.flags & 3) != 3) {
        return 1;
    }

    zClass_NodePartial missing{};
    if (zClass_Class::RemoveChildGeneric(&parent, &missing) != 0 || parent.listCountB != 1 ||
        missing.listCountA != 0) {
        return 2;
    }

    zClass_NodePartial checkedParent{};
    zClass_NodePartial checkedChild{};
    zClass_NodePartial *checkedChildren[] = {&checkedChild};
    zClass_NodePartial *checkedParents[] = {&checkedParent};
    checkedParent.listCountB = 1;
    checkedParent.listB = checkedChildren;
    checkedChild.listCountA = 1;
    checkedChild.listA = checkedParents;
    if (zClass::RemoveChildChecked(&checkedParent, &checkedChild) != 0 ||
        checkedParent.listCountB != 0 || checkedChild.listCountA != 0) {
        return 3;
    }
    if (zClass::RemoveChildChecked(nullptr, &checkedChild) != 5 ||
        zClass::RemoveChildChecked(&checkedParent, nullptr) != 5) {
        return 4;
    }

    int validatedClassData = 0;
    zClass_NodePartial validatedParent{};
    zClass_NodePartial validatedChild{};
    zClass_NodePartial *validatedChildren[] = {&validatedChild};
    zClass_NodePartial *validatedParents[] = {&validatedParent};
    validatedParent.classData = &validatedClassData;
    validatedParent.listCountB = 1;
    validatedParent.listB = validatedChildren;
    validatedChild.listCountA = 1;
    validatedChild.listA = validatedParents;
    if (zClass_Class::RemoveChildValidated(&validatedParent, &validatedChild) != 0 ||
        validatedParent.listCountB != 0 || validatedChild.listCountA != 0) {
        return 5;
    }
    if (zClass_Class::RemoveChildValidated(nullptr, &validatedChild) != 5 ||
        zClass_Class::RemoveChildValidated(&validatedParent, nullptr) != 5) {
        return 6;
    }
    validatedParent.classData = nullptr;
    if (zClass_Class::RemoveChildValidated(&validatedParent, &validatedChild) != 5) {
        return 7;
    }

    zClass_TypeList::FreeAll();
    zClass_TypeList::FreeAll();
    return 0;
}

static void zclass_init_single_child_link(zClass_NodePartial *parent,
                                          zClass_NodePartial *child,
                                          zClass_NodePartial **children,
                                          zClass_NodePartial **parents) {
    *children = child;
    *parents = parent;
    parent->flags = 1;
    parent->listCountB = 1;
    parent->listB = children;
    child->listCountA = 1;
    child->listA = parents;
}

static int zclass_link_removed(const zClass_NodePartial *parent,
                               const zClass_NodePartial *child) {
    return parent->listCountB == 0 && child->listCountA == 0;
}

extern "C" int zclass_remove_wrapper_matrix_smoke() {
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    zClass_NodePartial *children[1]{};
    zClass_NodePartial *parents[1]{};
    int classData = 0;

    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass_Camera::gwCameraRemoveChild(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child) ||
        zClass_Camera::gwCameraRemoveChild(nullptr, &child) != 5 ||
        zClass_Camera::gwCameraRemoveChild(&parent, nullptr) != 5) {
        return 1;
    }

    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass::RemoveChildChecked(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child) ||
        zClass::RemoveChildChecked(nullptr, &child) != 5 ||
        zClass::RemoveChildChecked(&parent, nullptr) != 5) {
        return 2;
    }

    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass_Display::RemoveChild(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child) ||
        zClass_Display::RemoveChild(nullptr, &child) != 5 ||
        zClass_Display::RemoveChild(&parent, nullptr) != 5) {
        return 3;
    }

    parent.classData = &classData;
    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass_Object3D::RemoveChild(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child) ||
        zClass_Object3D::RemoveChild(nullptr, &child) != 5 ||
        zClass_Object3D::RemoveChild(&parent, nullptr) != 5) {
        return 4;
    }
    parent.classData = nullptr;
    if (zClass_Object3D::RemoveChild(&parent, &child) != 5) {
        return 5;
    }

    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass_Lod::RemoveChild(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child)) {
        return 6;
    }

    zClass_SequenceDataPartial sequenceData{};
    parent.classData = &sequenceData;
    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass_Sequence::RemoveChild(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child) ||
        zClass_Sequence::RemoveChild(nullptr, &child) != 5 ||
        zClass_Sequence::RemoveChild(&parent, nullptr) != 5) {
        return 7;
    }
    parent.classData = nullptr;
    if (zClass_Sequence::RemoveChild(&parent, &child) != 5) {
        return 8;
    }

    parent.classData = &classData;
    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass_Animate::RemoveChild(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child) ||
        zClass_Animate::RemoveChild(nullptr, &child) != 5 ||
        zClass_Animate::RemoveChild(&parent, nullptr) != 5) {
        return 9;
    }
    parent.classData = nullptr;
    if (zClass_Animate::RemoveChild(&parent, &child) != 5) {
        return 10;
    }

    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass_Light::RemoveChild(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child) ||
        zClass_Light::RemoveChild(nullptr, &child) != 5 ||
        zClass_Light::RemoveChild(&parent, nullptr) != 5) {
        return 11;
    }

    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass_Sound::RemoveChild(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child) ||
        zClass_Sound::RemoveChild(nullptr, &child) != 5 ||
        zClass_Sound::RemoveChild(&parent, nullptr) != 5) {
        return 12;
    }

    parent.classData = &classData;
    zclass_init_single_child_link(&parent, &child, children, parents);
    if (zClass_Class::RemoveChildValidated(&parent, &child) != 0 ||
        !zclass_link_removed(&parent, &child) ||
        zClass_Class::RemoveChildValidated(nullptr, &child) != 5 ||
        zClass_Class::RemoveChildValidated(&parent, nullptr) != 5) {
        return 13;
    }
    parent.classData = nullptr;
    if (zClass_Class::RemoveChildValidated(&parent, &child) != 5) {
        return 14;
    }

    const int classIds[] = {1, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    for (int i = 0; i < static_cast<int>(sizeof(classIds) / sizeof(classIds[0])); ++i) {
        zClass_SequenceDataPartial dispatchSequenceData{};
        parent = {};
        child = {};
        parent.classId = classIds[i];
        if (classIds[i] == 5 || classIds[i] == 8 || classIds[i] == 11) {
            parent.classData = &classData;
        } else if (classIds[i] == 7) {
            parent.classData = &dispatchSequenceData;
        }
        zclass_init_single_child_link(&parent, &child, children, parents);
        if (zClass_Class::RemoveChild(&parent, &child) != 0 ||
            !zclass_link_removed(&parent, &child)) {
            return 20 + i;
        }
    }

    parent = {};
    child = {};
    parent.classId = 99;
    return zClass_Class::RemoveChild(&parent, &child) == 1 &&
                   zClass_Class::RemoveChild(nullptr, &child) == 5 &&
                   zClass_Class::RemoveChild(&parent, nullptr) == 5
               ? 0
               : 40;
}

extern "C" int zclass_object3d_child_wrappers_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    parent.classData = &objectData;

    if (zClass_Object3D::gwObject3DAddChild(&parent, &child) != 0 || parent.listCountB != 1 ||
        child.listCountA != 1) {
        return 1;
    }
    if (zClass_Object3D::RemoveChild(&parent, &child) != 0 || parent.listCountB != 0 ||
        child.listCountA != 0) {
        return 2;
    }

    std::free(parent.listB);
    std::free(child.listA);
    zClass_TypeList::FreeAll();
    zClass_TypeList::FreeAll();

    zClass_NodePartial noData{};
    return zClass_Object3D::gwObject3DAddChild(nullptr, &child) == 5 &&
                   zClass_Object3D::gwObject3DAddChild(&parent, nullptr) == 5 &&
                   zClass_Object3D::gwObject3DAddChild(&noData, &child) == 5 &&
                   zClass_Object3D::RemoveChild(nullptr, &child) == 5 &&
                   zClass_Object3D::RemoveChild(&parent, nullptr) == 5 &&
                   zClass_Object3D::RemoveChild(&noData, &child) == 5
               ? 0
               : 3;
}

extern "C" int zclass_delete_node_from_lists_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodePartial node{};
    node.classId = 1;
    node.callbackPriority = 2;
    node.actionCallback = &node;

    zClass_TypeList::Insert(8, &node);
    zClass_TypeList::Insert(7, &node);
    zClass_TypeList::Insert(2, &node);
    zClass_TypeList::Insert(6, &node);

    if (zClass_List::DeleteNodeFromLists(&node) != 0 ||
        zClass_TypeList::PendingRemovalDirty(8) == 0 ||
        zClass_TypeList::PendingRemovalDirty(7) == 0 ||
        zClass_TypeList::PendingRemovalDirty(2) == 0 ||
        zClass_TypeList::PendingRemovalDirty(6) == 0) {
        return 1;
    }
    if (zClass_TypeList::Head(8)->pendingRemove != 1 ||
        zClass_TypeList::Head(7)->pendingRemove != 1 ||
        zClass_TypeList::Head(2)->pendingRemove != 1 ||
        zClass_TypeList::Head(6)->pendingRemove != 1) {
        return 2;
    }

    zClass::ProcessDeferredWork();
    if (zClass_TypeList::Head(8) != nullptr || zClass_TypeList::Head(7) != nullptr ||
        zClass_TypeList::Head(2) != nullptr || zClass_TypeList::Head(6) != nullptr ||
        (node.flags & 1) != 0) {
        return 3;
    }

    zClass_TypeList::FreeAll();
    return 0;
}

extern "C" int zclass_find_by_name_and_filtered_iter_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial alpha{};
    zClass_NodePartial beta{};
    zClass_NodePartial alpine{};
    std::strcpy(alpha.name, "alpha");
    std::strcpy(beta.name, "beta");
    std::strcpy(alpine.name, "alpine");

    zClass_TypeList::Insert(5, &alpha);
    zClass_TypeList::Insert(5, &beta);
    zClass_TypeList::Insert(5, &alpine);

    if (zClass::FindByTypeAndName(5, "alpha") != &alpha ||
        zClass::FindByTypeAndName(5, "missing") != nullptr) {
        return 1;
    }

    if (zClass_Class::gwNodeFindNextByName("beta", 5) != nullptr ||
        zClass_Class::gwNodeFindNextByName(nullptr, 5) != &beta ||
        zClass_Class::gwNodeFindNextByName(nullptr, 5) != nullptr) {
        return 2;
    }

    if (zClass::FindNextByTypePrefix("al", 5) != nullptr ||
        zClass::FindNextByTypePrefix(nullptr, 5) != &alpine ||
        zClass::FindNextByTypePrefix(nullptr, 5) != &alpha ||
        zClass::FindNextByTypePrefix(nullptr, 5) != nullptr) {
        return 3;
    }

    zClass_NodePartial root{};
    zClass_NodePartial firstChild{};
    zClass_NodePartial secondChild{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *rootChildren[] = {&firstChild, &secondChild};
    zClass_NodePartial *secondChildren[] = {&grandchild};
    std::strcpy(root.name, "root");
    std::strcpy(firstChild.name, "shared");
    std::strcpy(secondChild.name, "shared");
    std::strcpy(grandchild.name, "deep");
    root.listCountB = 2;
    root.listB = rootChildren;
    secondChild.listCountB = 1;
    secondChild.listB = secondChildren;
    if (zClass_Class::FindSubNodeByName(&root, "root") != &root ||
        zClass_Class::FindSubNodeByName(&root, "shared") != &secondChild ||
        zClass_Class::FindSubNodeByName(&root, "deep") != &grandchild ||
        zClass_Class::FindSubNodeByName(&root, "missing") != nullptr ||
        zClass_Class::FindSubNodeByName(nullptr, "root") != nullptr) {
        return 4;
    }

    FILE *const printCapture = std::tmpfile();
    if (printCapture == nullptr) {
        return 5;
    }

    std::fflush(stdout);
    const int savedStdout = _dup(_fileno(stdout));
    if (savedStdout < 0) {
        std::fclose(printCapture);
        return 5;
    }

    if (_dup2(_fileno(printCapture), _fileno(stdout)) != 0) {
        _close(savedStdout);
        std::fclose(printCapture);
        return 5;
    }

    zClass_TypeList::PrintBucket(5);
    std::fflush(stdout);

    char printOutput[96]{};
    std::fseek(printCapture, 0, SEEK_SET);
    const size_t printBytes = std::fread(printOutput, 1, sizeof(printOutput) - 1, printCapture);

    _dup2(savedStdout, _fileno(stdout));
    _close(savedStdout);
    std::fclose(printCapture);

    if (printBytes == 0 ||
        std::strcmp(printOutput,
                    "Node 0 desc: alpine\nNode 1 desc: beta\nNode 2 desc: alpha\n") != 0) {
        return 5;
    }

    zClass_TypeList::MarkPendingRemoval(5, &alpha);
    zClass_TypeList::MarkPendingRemoval(5, &beta);
    zClass_TypeList::MarkPendingRemoval(5, &alpine);
    zClass::ProcessDeferredWork();
    zClass_TypeList::FreeAll();
    return 0;
}

extern "C" int zclass_node_predicate_helpers_smoke() {
    zBBox3f bbox{-2.0f, 4.0f, 1.0f, 6.0f, 10.0f, 9.0f};
    zVec3 center{};
    float radius = 0.0f;
    if (BBox::MinMaxToBoundingSphere(&bbox, &center, &radius) != &radius ||
        center.x != 2.0f || center.y != 7.0f || center.z != 5.0f) {
        return 7;
    }
    const float radiusSq = 4.0f * 4.0f + 3.0f * 3.0f + 4.0f * 4.0f;
    std::int32_t expectedRadiusBits = FloatBitsForTest(radiusSq);
    expectedRadiusBits = (expectedRadiusBits >> 1) + 0x1fc00000;
    if (FloatBitsForTest(radius) != expectedRadiusBits) {
        return 8;
    }

    const zVec3 cornerValues[8] = {{6.0f, 4.0f, 9.0f},  {-2.0f, 10.0f, 1.0f},
                                   {6.0f, 10.0f, 9.0f}, {-2.0f, 4.0f, 1.0f},
                                   {6.0f, 4.0f, 1.0f},  {-2.0f, 10.0f, 9.0f},
                                   {6.0f, 10.0f, 1.0f}, {-2.0f, 4.0f, 9.0f}};
    zBBoxCorners minMaxCorners{};
    for (std::int32_t i = 0; i < 8; ++i) {
        minMaxCorners.values[i * 3 + 0] = cornerValues[i].x;
        minMaxCorners.values[i * 3 + 1] = cornerValues[i].y;
        minMaxCorners.values[i * 3 + 2] = cornerValues[i].z;
    }
    center = {};
    radius = 0.0f;
    BBox::CornersToBoundingSphere(&minMaxCorners, &center, &radius);
    if (center.x != 2.0f || center.y != 7.0f || center.z != 5.0f) {
        return 680;
    }
    if (FloatBitsForTest(radius) != expectedRadiusBits) {
        return 681;
    }

    zBBoxCorners expandedCorners{};
    BBox::ExpandToCorners(&bbox, &expandedCorners);
    if (expandedCorners.values[0] != -2.0f || expandedCorners.values[1] != 4.0f ||
        expandedCorners.values[2] != 9.0f || expandedCorners.values[6] != 6.0f ||
        expandedCorners.values[7] != 4.0f || expandedCorners.values[8] != 1.0f ||
        expandedCorners.values[21] != -2.0f || expandedCorners.values[22] != 10.0f ||
        expandedCorners.values[23] != 1.0f) {
        return 682;
    }

    zDiPartial di{1, 0};
    zClass_NodePartial node{};
    node.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);
    if (zClass_Node::HasRenderableDiPredicate(&node) != 1) {
        return 1;
    }
    di.flags = 0x10;
    if (zClass_Node::HasRenderableDiPredicate(&node) != 0) {
        return 2;
    }
    node.userDataOrDiRef = 0;
    if (zClass_Node::HasRenderableDiPredicate(&node) != 0) {
        return 3;
    }

    zDiPartial flagTarget{0, 0x12};
    zDi::SetFlagBit0(&flagTarget, 1);
    if (flagTarget.flags != 0x13) {
        return 4;
    }
    zDi::SetFlagBit0(&flagTarget, 0);
    if (flagTarget.flags != 0x12) {
        return 5;
    }
    zDi::SetFlagBit0(nullptr, 1);
    if (zModel_Material_SetFlagBit9(nullptr, 1) != 0) {
        return 6;
    }

    zDiPartial flagRootDi{0, 0x12};
    zDiPartial flagGrandchildDi{0, 0x04};
    zClass_NodePartial flagRootNode{};
    zClass_NodePartial flagChildNode{};
    zClass_NodePartial flagGrandchildNode{};
    zClass_NodePartial *flagRootChildren[1] = {&flagChildNode};
    zClass_NodePartial *flagChildChildren[1] = {&flagGrandchildNode};
    flagRootNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&flagRootDi);
    flagRootNode.listCountB = 1;
    flagRootNode.listB = flagRootChildren;
    flagChildNode.listCountB = 1;
    flagChildNode.listB = flagChildChildren;
    flagGrandchildNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&flagGrandchildDi);
    zClass_Node::SetDiFlagBit0Recursive(&flagRootNode, 1);
    if (flagRootDi.flags != 0x13 || flagGrandchildDi.flags != 0x05) {
        return 7;
    }
    zClass_Node::SetDiFlagBit0Recursive(&flagRootNode, 0);
    if (flagRootDi.flags != 0x12 || flagGrandchildDi.flags != 0x04) {
        return 8;
    }

    zDiEntryPartial variantEntries[3] = {};
    variantEntries[1].variantTagInitialized = 1;
    variantEntries[1].variantTag = 0x44;
    zDiPartial variantDi{};
    variantDi.entryCount = 3;
    variantDi.entries = variantEntries;
    zDi::SetVariantTagIfUnset(&variantDi, 0x21);
    if (variantEntries[0].variantTagInitialized != 1 || variantEntries[0].variantTag != 0x21 ||
        variantEntries[1].variantTagInitialized != 1 || variantEntries[1].variantTag != 0x44 ||
        variantEntries[2].variantTagInitialized != 1 || variantEntries[2].variantTag != 0x21) {
        return 9;
    }
    zDi::SetVariantTagIfUnset(nullptr, 0x22);

    g_Variant_FilterEnabled = 0;
    if (VariantTag::CurrentAllowsId(0x33) != 1) {
        return 31;
    }
    g_Variant_FilterEnabled = 1;
    g_Variant_CurrentTag.count = 0;
    if (VariantTag::CurrentAllowsId(0x33) != 1 || VariantTag::CurrentAllowsId(0xff) != 1) {
        return 32;
    }
    g_Variant_CurrentTag.count = 2;
    g_Variant_CurrentTag.tags[0] = 0x44;
    g_Variant_CurrentTag.tags[1] = 0x55;
    if (VariantTag::CurrentAllowsId(0x44) != 1 || VariantTag::CurrentAllowsId(0x33) != 0) {
        return 33;
    }
    g_Variant_CurrentTag.tags[1] = 0xff;
    if (VariantTag::CurrentAllowsId(0x33) != 1) {
        return 34;
    }

    zTag4Partial overlapA = {};
    zTag4Partial overlapB = {};
    overlapA.count = 2;
    overlapA.tags[0] = 0x12;
    overlapA.tags[1] = 0x34;
    overlapB.count = 2;
    overlapB.tags[0] = 0x56;
    overlapB.tags[1] = 0x34;
    if (VariantTag::TagsOverlap(&overlapA, &overlapB) != 1) {
        return 35;
    }
    overlapB.tags[1] = 0x78;
    if (VariantTag::TagsOverlap(&overlapA, &overlapB) != 0) {
        return 36;
    }
    overlapA.tags[1] = 0xff;
    if (VariantTag::TagsOverlap(&overlapA, &overlapB) != 1) {
        return 37;
    }
    overlapA.tags[1] = 0x34;
    overlapB.tags[0] = 0xff;
    if (VariantTag::TagsOverlap(&overlapA, &overlapB) != 1) {
        return 38;
    }
    overlapB.count = 0;
    if (VariantTag::TagsOverlap(&overlapA, &overlapB) != 1) {
        return 39;
    }
    g_Variant_FilterEnabled = 0;
    overlapB.count = 2;
    overlapB.tags[0] = 0x56;
    if (VariantTag::TagsOverlap(&overlapA, &overlapB) != 1) {
        return 40;
    }
    g_Variant_FilterEnabled = 1;

    zVec3 boundsVerts[2] = {{-1.0f, 2.0f, -3.0f}, {5.0f, -6.0f, 7.0f}};
    zDiPartial boundsDi{};
    boundsDi.mode = 0;
    boundsDi.vertCount = 2;
    boundsDi.verts = boundsVerts;
    zBoundsMinMaxPartial bounds{};
    zDi::RebuildBounds(&boundsDi, &bounds);
    if (bounds.min.x != -1.0f || bounds.min.y != -6.0f || bounds.min.z != -3.0f ||
        bounds.max.x != 5.0f || bounds.max.y != 2.0f || bounds.max.z != 7.0f ||
        boundsDi.bboxCenter.x != 2.0f || boundsDi.bboxCenter.y != -2.0f ||
        boundsDi.bboxCenter.z != 2.0f || boundsDi.bboxRadius <= 0.0f) {
        return 61;
    }

    zClass_NodeFreeListSlot displaySlot{};
    displaySlot.node.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&boundsDi);
    if (zClass_Class::gwNodeUpdateDisplayInstance(&displaySlot.node) != 0 ||
        (displaySlot.node.flags & 0x200) == 0) {
        return 62;
    }
    zBoundsMinMaxPartial *displayBounds =
        reinterpret_cast<zBoundsMinMaxPartial *>(displaySlot.unknown_8c);
    if (displayBounds->min.x != -1.0f || displayBounds->max.z != 7.0f) {
        return 63;
    }
    displaySlot.node.userDataOrDiRef = 0;
    if (zClass_Class::gwNodeUpdateDisplayInstance(&displaySlot.node) != 0 ||
        (displaySlot.node.flags & 0x200) != 0 ||
        zClass_Class::gwNodeUpdateDisplayInstance(nullptr) != 5) {
        return 64;
    }

    zClass_NodePartial bboxNode{};
    std::int32_t bboxClassData = 0;
    bboxNode.classData = &bboxClassData;
    bboxNode.flags = 0x100;
    bboxNode.cachedBounds[0] = 1.0f;
    bboxNode.cachedBounds[1] = 2.0f;
    bboxNode.cachedBounds[2] = 3.0f;
    bboxNode.cachedBounds[3] = 4.0f;
    bboxNode.cachedBounds[4] = 5.0f;
    bboxNode.cachedBounds[5] = 6.0f;
    zBBox3f bboxOut{};
    if (zClass_Class::gwNodeGetBBox(&bboxNode, &bboxOut) != 0 || bboxOut.minX != 1.0f ||
        bboxOut.minY != 2.0f || bboxOut.minZ != 3.0f || bboxOut.maxX != 4.0f ||
        bboxOut.maxY != 5.0f || bboxOut.maxZ != 6.0f) {
        return 650;
    }
    zBBoxCorners bboxCorners{};
    if (zClass_Class::gwNodeGetWorldBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 1.0f || bboxCorners.values[1] != 2.0f ||
        bboxCorners.values[2] != 6.0f || bboxCorners.values[6] != 4.0f ||
        bboxCorners.values[7] != 2.0f || bboxCorners.values[8] != 3.0f ||
        bboxCorners.values[21] != 1.0f || bboxCorners.values[22] != 5.0f ||
        bboxCorners.values[23] != 3.0f) {
        return 65;
    }
    bboxNode.flags = 0;
    if (zClass_Class::gwNodeGetBBox(&bboxNode, &bboxOut) != 1 ||
        zClass_Class::gwNodeGetWorldBBoxCorners(&bboxNode, &bboxCorners) != 1) {
        return 66;
    }
    bboxNode.classData = nullptr;
    bboxNode.flags = 0x100;
    if (zClass_Class::gwNodeGetBBox(&bboxNode, &bboxOut) != 5 ||
        zClass_Class::gwNodeGetBBox(nullptr, &bboxOut) != 5 ||
        zClass_Class::gwNodeGetWorldBBoxCorners(&bboxNode, &bboxCorners) != 5 ||
        zClass_Class::gwNodeGetWorldBBoxCorners(nullptr, &bboxCorners) != 5) {
        return 67;
    }

    zClass_Object3DDataPartial objectData{};
    objectData.localMatrix[0] = 1.0f;
    objectData.localMatrix[4] = 1.0f;
    objectData.localMatrix[8] = 1.0f;
    objectData.localMatrix[9] = 10.0f;
    objectData.localMatrix[10] = 20.0f;
    objectData.localMatrix[11] = 30.0f;
    bboxNode.classId = 5;
    bboxNode.classData = &objectData;
    if (zClass_Class::gwNodeGetWorldBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 11.0f || bboxCorners.values[1] != 22.0f ||
        bboxCorners.values[2] != 36.0f) {
        return 68;
    }

    static std::int32_t viewMatrixFlags[1];
    static float *viewMatrixSlots[1];
    static zMat4x3 viewCurrentMatrix;
    zMath::g_currentMatrixIdentityFlagSlot = &viewMatrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &viewMatrixSlots[0];

    viewMatrixFlags[0] = 0;
    viewMatrixSlots[0] = nullptr;
    objectData.flags = 0;
    if (zClass_Class::gwNodeGetViewBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 11.0f || bboxCorners.values[1] != 22.0f ||
        bboxCorners.values[2] != 36.0f) {
        return 681;
    }

    viewCurrentMatrix = {};
    viewCurrentMatrix.xx = 1.0f;
    viewCurrentMatrix.yy = 1.0f;
    viewCurrentMatrix.zz = 1.0f;
    viewCurrentMatrix.posX = 100.0f;
    viewCurrentMatrix.posY = -10.0f;
    viewCurrentMatrix.posZ = 1.0f;
    viewMatrixFlags[0] = 0;
    viewMatrixSlots[0] = reinterpret_cast<float *>(&viewCurrentMatrix);
    objectData.flags = 0x08;
    if (zClass_Class::gwNodeGetViewBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 101.0f || bboxCorners.values[1] != -8.0f ||
        bboxCorners.values[2] != 7.0f) {
        return 682;
    }

    objectData.flags = 0;
    if (zClass_Class::gwNodeGetViewBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 111.0f || bboxCorners.values[1] != 12.0f ||
        bboxCorners.values[2] != 37.0f) {
        return 683;
    }

    viewMatrixFlags[0] = 1;
    viewMatrixSlots[0] = reinterpret_cast<float *>(&viewCurrentMatrix);
    bboxNode.classId = 3;
    bboxNode.classData = &bboxClassData;
    if (zClass_Class::gwNodeGetViewBBoxCorners(&bboxNode, &bboxCorners) != 3 ||
        bboxCorners.values[0] != 1.0f || bboxCorners.values[1] != 2.0f ||
        bboxCorners.values[2] != 6.0f) {
        return 684;
    }
    bboxNode.classId = 5;
    bboxNode.classData = &objectData;

    zClass_NodeFreeListSlot parentSlot{};
    zClass_NodePartial childA{};
    zClass_NodePartial childB{};
    zClass_NodePartial childIgnored{};
    std::int32_t childClassData = 0;
    childA.classData = &childClassData;
    childB.classData = &childClassData;
    childIgnored.classData = &childClassData;
    childA.flags = 0x100;
    childB.flags = 0x100;
    childA.cachedBounds[0] = 0.0f;
    childA.cachedBounds[1] = 1.0f;
    childA.cachedBounds[2] = 2.0f;
    childA.cachedBounds[3] = 3.0f;
    childA.cachedBounds[4] = 4.0f;
    childA.cachedBounds[5] = 5.0f;
    childB.cachedBounds[0] = -2.0f;
    childB.cachedBounds[1] = 6.0f;
    childB.cachedBounds[2] = -1.0f;
    childB.cachedBounds[3] = 10.0f;
    childB.cachedBounds[4] = 7.0f;
    childB.cachedBounds[5] = 8.0f;
    childIgnored.cachedBounds[0] = -100.0f;
    childIgnored.cachedBounds[1] = -100.0f;
    childIgnored.cachedBounds[2] = -100.0f;
    childIgnored.cachedBounds[3] = 100.0f;
    childIgnored.cachedBounds[4] = 100.0f;
    childIgnored.cachedBounds[5] = 100.0f;
    zClass_NodePartial *children[] = {&childIgnored, &childA, &childB};
    parentSlot.node.listCountB = 3;
    parentSlot.node.listB = children;
    parentSlot.node.flags = 0x400;
    if (zClass_Class::gwNodeComputeChildBBox(&parentSlot.node) != 0 ||
        (parentSlot.node.flags & 0x400) == 0) {
        return 69;
    }
    zBBox3f *childBounds =
        reinterpret_cast<zBBox3f *>(reinterpret_cast<unsigned char *>(&parentSlot.node) + 0xa4);
    if (childBounds->minX != -2.0f || childBounds->minY != 1.0f || childBounds->minZ != -1.0f ||
        childBounds->maxX != 10.0f || childBounds->maxY != 7.0f || childBounds->maxZ != 8.0f) {
        return 70;
    }
    parentSlot.node.classId = 2;
    if (zClass_Class::gwNodeComputeChildBBox(&parentSlot.node) != 0 ||
        (parentSlot.node.flags & 0x400) != 0 ||
        zClass_Class::gwNodeComputeChildBBox(nullptr) != 5) {
        return 71;
    }

    zWorldAreaPartial row0[2]{};
    zWorldAreaPartial row1[2]{};
    row0[0].cellMinX = 0.0f;
    row0[0].cellMinZ = 100.0f;
    row0[1].cellMinX = 50.0f;
    row0[1].cellMinZ = 100.0f;
    row1[0].cellMinX = 0.0f;
    row1[0].cellMinZ = 50.0f;
    row1[1].cellMinX = 50.0f;
    row1[1].cellMinZ = 50.0f;
    zWorldAreaPartial *rows[] = {row0, row1};
    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 100.0f;
    worldData.worldMaxX = 100.0f;
    worldData.worldMaxZ = 0.0f;
    worldData.areaCellSizeX = 50.0f;
    worldData.areaCellSizeZ = -50.0f;
    worldData.areaInvSizeX = 0.02f;
    worldData.areaInvSizeZ = -0.02f;
    worldData.partitionInclusionTolX = 2.0f;
    worldData.partitionInclusionTolZ = 2.0f;
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 2;
    worldData.areaGridRows = rows;
    zClass_NodePartial worldNode{};
    worldNode.classData = &worldData;
    std::int32_t gridCol = -99;
    std::int32_t gridRow = -99;
    if (zClass_World::WorldRectToGridIndex(&worldNode, &gridCol, 10.0f, 20.0f, 70.0f, 80.0f,
                                           &gridRow) != 0 ||
        gridCol != 0 || gridRow != 0) {
        return 72;
    }
    if (zClass_World::WorldRectToGridIndex(&worldNode, &gridCol, 60.0f, 70.0f, 20.0f, 40.0f,
                                           &gridRow) != 0 ||
        gridCol != 1 || gridRow != 1) {
        return 73;
    }
    if (zClass_World::WorldRectToGridIndex(&worldNode, &gridCol, -5.0f, 10.0f, 70.0f, 80.0f,
                                           &gridRow) != 0 ||
        gridCol != -1 || gridRow != -1) {
        return 74;
    }
    if (zClass_World::WorldRectToGridIndex(&worldNode, &gridCol, 40.0f, 60.0f, 70.0f, 80.0f,
                                           &gridRow) != 0 ||
        gridCol != -1 || gridRow != -1) {
        return 75;
    }
    worldNode.flags = 1;
    row0[0].areaFlags = 0;
    if (zClass_World::EnsureGridCellDisplayPosition(&worldNode, 0, 0) != 0 ||
        worldData.pendingAreaUpdateCount != 1 || worldData.pendingAreaUpdateCapacity != 1 ||
        worldData.pendingAreaUpdates[0] != &row0[0] || (row0[0].areaFlags & 1) == 0 ||
        (worldNode.flags & 3) != 3 || (worldData.flags & 0x10) == 0) {
        return 76;
    }
    if (zClass_World::EnsureGridCellDisplayPosition(&worldNode, 0, 0) != 0 ||
        worldData.pendingAreaUpdateCount != 1) {
        return 77;
    }

    zClass_NodePartial gridChild{};
    if (zClass_World::AddChildToGridCell(&worldNode, &gridChild, 1, 0) != 0 ||
        row0[1].childCount != 1 || row0[1].childList[0] != &gridChild || gridChild.gridCol != 1 ||
        gridChild.gridRow != 0 || gridChild.listCountA != 1 || gridChild.listA[0] != &worldNode ||
        worldData.pendingAreaUpdateCount != 2 || (row0[1].areaFlags & 1) == 0) {
        return 80;
    }
    if (zClass_World::RemoveChildAtGrid(&worldNode, &gridChild) != 0 || row0[1].childCount != 0 ||
        gridChild.gridCol != -1 || gridChild.gridRow != -1 || gridChild.listCountA != 0) {
        return 81;
    }
    std::free(row0[1].childList);
    row0[1].childList = nullptr;
    std::free(gridChild.listA);
    gridChild.listA = nullptr;

    zClass_NodePartial fallbackChild{};
    if (zClass_World::AddChildToGridCell(&worldNode, &fallbackChild, -1, -1) != 0 ||
        worldNode.listCountB != 1 || worldNode.listB[0] != &fallbackChild ||
        fallbackChild.gridCol != -1 || fallbackChild.gridRow != -1 ||
        fallbackChild.listCountA != 1 || fallbackChild.listA[0] != &worldNode) {
        return 82;
    }
    std::free(worldNode.listB);
    worldNode.listB = nullptr;
    worldNode.listCountB = 0;
    std::free(fallbackChild.listA);
    fallbackChild.listA = nullptr;
    fallbackChild.listCountA = 0;

    zClass_NodeFreeListSlot recalcSlot{};
    zClass_NodePartial recalcParent{};
    zClass_NodePartial *recalcParents[] = {&recalcParent};
    recalcParent.flags = 1;
    recalcSlot.node.classData = &childClassData;
    recalcSlot.node.flags = 0x200 | 0x400;
    recalcSlot.node.listCountA = 1;
    recalcSlot.node.listA = recalcParents;
    zBBox3f *primaryBounds = reinterpret_cast<zBBox3f *>(recalcSlot.unknown_8c);
    zBBox3f *secondaryBounds = reinterpret_cast<zBBox3f *>(recalcSlot.unknown_8c + 0x18);
    *primaryBounds = {0.0f, 2.0f, -1.0f, 4.0f, 8.0f, 6.0f};
    *secondaryBounds = {-3.0f, 3.0f, -2.0f, 2.0f, 9.0f, 5.0f};
    if (zClass_Class::gwNodeRecalcBBox(&recalcSlot.node) != 0 ||
        (recalcSlot.node.flags & 0x100) == 0 || recalcSlot.node.cachedBounds[0] != -3.0f ||
        recalcSlot.node.cachedBounds[1] != 2.0f || recalcSlot.node.cachedBounds[2] != -2.0f ||
        recalcSlot.node.cachedBounds[3] != 4.0f || recalcSlot.node.cachedBounds[4] != 9.0f ||
        recalcSlot.node.cachedBounds[5] != 6.0f || (recalcSlot.node.boundsFlags & 0x04) == 0 ||
        (recalcParent.boundsFlags & 0x02) == 0 || (recalcParent.flags & 0x02) == 0) {
        return 83;
    }
    recalcSlot.node.flags = 0x100;
    recalcSlot.node.listCountA = 0;
    if (zClass_Class::gwNodeRecalcBBox(&recalcSlot.node) != 0 ||
        (recalcSlot.node.flags & 0x100) != 0 || zClass_Class::gwNodeRecalcBBox(nullptr) != 5) {
        return 84;
    }

    zClass_NodePartial areaChildA{};
    zClass_NodePartial areaChildB{};
    areaChildA.classData = &childClassData;
    areaChildB.classData = &childClassData;
    areaChildA.flags = 0x100;
    areaChildB.flags = 0x100;
    areaChildA.cachedBounds[1] = -2.0f;
    areaChildA.cachedBounds[4] = 3.0f;
    areaChildB.cachedBounds[1] = -5.0f;
    areaChildB.cachedBounds[4] = 10.0f;
    zClass_NodePartial *areaChildren[] = {&areaChildA, &areaChildB};
    zWorldAreaPartial rebuildArea{};
    rebuildArea.areaFlags = 1;
    rebuildArea.bbox[0] = 0.0f;
    rebuildArea.bbox[2] = 0.0f;
    rebuildArea.bbox[3] = 20.0f;
    rebuildArea.bbox[5] = 20.0f;
    rebuildArea.childCount = 2;
    rebuildArea.childList = areaChildren;
    if (zClass_World::RebuildAreaBounds(&worldData, &rebuildArea) != 0 ||
        (rebuildArea.areaFlags & 1) != 0 || (rebuildArea.areaFlags & 0x100) == 0 ||
        rebuildArea.bbox[1] != -5.0f || rebuildArea.bbox[4] != 10.0f ||
        rebuildArea.bboxCenter.y != 2.5f || rebuildArea.bboxRadius <= 0.0f) {
        return 85;
    }

    std::free(worldData.pendingAreaUpdates);
    worldData.pendingAreaUpdates = nullptr;
    if (zClass_World::EnsureGridCellDisplayPosition(nullptr, 0, 0) != 5) {
        return 78;
    }
    worldNode.classData = nullptr;
    if (zClass_World::EnsureGridCellDisplayPosition(&worldNode, 0, 0) != 5) {
        return 79;
    }

    zModel_MaterialPartial materialDirect{0xffff};
    if (zModel_Material_SetFlagBit9(&materialDirect, 0) != 1 ||
        (materialDirect.flags & 0x0200) != 0 || (materialDirect.flags & 0xfdff) != 0xfdff) {
        return 7;
    }

    zClass_NodePartial root{};
    zClass_NodePartial first{};
    zClass_NodePartial second{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *rootChildren[] = {&first, &second};
    zClass_NodePartial *secondChildren[] = {&grandchild};
    zDiPartial rootDi{0, 0};
    zDiPartial firstDi{0, 1};
    zDiPartial grandchildDi{0, 0x20};
    root.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&rootDi);
    first.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&firstDi);
    grandchild.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&grandchildDi);
    root.listCountB = 2;
    root.listB = rootChildren;
    second.listCountB = 1;
    second.listB = secondChildren;
    zClass_Node::AssignInt32ToDiRecursive(&root, 1);
    if ((rootDi.flags & 1) == 0 || (firstDi.flags & 1) == 0 || (grandchildDi.flags & 1) == 0) {
        return 6;
    }
    zClass_Node::AssignInt32ToDiRecursive(&root, 0);
    if ((rootDi.flags & 1) != 0 || (firstDi.flags & 1) != 0 || (grandchildDi.flags & 1) != 0) {
        return 8;
    }

    zModel_MaterialPartial rootMaterial{0x0100};
    zModel_MaterialPartial skippedMaterial{0};
    zModel_MaterialPartial childMaterial{0x0100};
    zDiEntryPartial rootEntries[2]{};
    zDiEntryPartial childEntries[1]{};
    zDiPartial materialRootDi{};
    zDiPartial materialChildDi{};
    rootEntries[0].material = &rootMaterial;
    rootEntries[1].material = &skippedMaterial;
    childEntries[0].material = &childMaterial;
    materialRootDi.entryCount = 2;
    materialRootDi.entries = rootEntries;
    materialChildDi.entryCount = 1;
    materialChildDi.entries = childEntries;
    root.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&materialRootDi);
    first.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&materialChildDi);
    grandchild.userDataOrDiRef = 0;
    zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive(&root, 1);
    if ((rootMaterial.flags & 0x0200) == 0 || (skippedMaterial.flags & 0x0200) != 0 ||
        (childMaterial.flags & 0x0200) == 0) {
        return 9;
    }
    zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive(&root, 0);
    if ((rootMaterial.flags & 0x0200) != 0 || (childMaterial.flags & 0x0200) != 0) {
        return 10;
    }

    zVid_Image::ReleaseIfNotDefault(&zVid_Image::g_zImage_DefaultImage);
    zImage_TexDirEntryPartial loadedEntry{};
    zImage_TexDirEntryPartial stoppedEntry{};
    zVidImagePartial *loadedImage =
        static_cast<zVidImagePartial *>(std::calloc(1, sizeof(zVidImagePartial)));
    if (loadedImage == nullptr) {
        return 11;
    }
    loadedEntry.image = loadedImage;
    loadedEntry.loadState = 1;
    loadedEntry.nextVariant = &stoppedEntry;
    stoppedEntry.image = &zVid_Image::g_zImage_DefaultImage;
    stoppedEntry.loadState = 2;
    zImage::InvalidateLoadedVariantChain(&loadedEntry);
    if (loadedEntry.image != nullptr || loadedEntry.loadState != 3 ||
        stoppedEntry.image != &zVid_Image::g_zImage_DefaultImage || stoppedEntry.loadState != 2) {
        return 12;
    }

    zImage_TexDirEntryPartial materialEntry{};
    zImage_TexDirEntryPartial frameEntry{};
    materialEntry.image = &zVid_Image::g_zImage_DefaultImage;
    materialEntry.loadState = 1;
    frameEntry.image = &zVid_Image::g_zImage_DefaultImage;
    frameEntry.loadState = 1;
    zImage_TexDirEntryPartial *frameTable[] = {&frameEntry};
    zModel_MaterialCyclePartial cycle{};
    cycle.frameCount = 1;
    cycle.frameTable = frameTable;
    zModel_MaterialPartial invalidateMaterial{};
    invalidateMaterial.flags = 0x0300;
    invalidateMaterial.currentTextureDirectoryEntry = &materialEntry;
    invalidateMaterial.cycle = &cycle;
    zDiEntryPartial invalidateEntries[1]{};
    zDiPartial invalidateDi{};
    invalidateEntries[0].material = &invalidateMaterial;
    invalidateDi.entryCount = 1;
    invalidateDi.entries = invalidateEntries;
    root.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&invalidateDi);
    first.userDataOrDiRef = 0;
    zClass_Node::InvalidateFlagBit8MaterialImagesRecursive(&root);
    if (materialEntry.loadState != 3 || frameEntry.loadState != 3) {
        return 13;
    }

    grandchild.nodeType = 0x42;

    if (zClass::AnyNodeMatchesPredicateRecursive(&root, zclass_test_node_type_0x42) != 1) {
        return 14;
    }
    grandchild.nodeType = 0;
    return zClass::AnyNodeMatchesPredicateRecursive(&root, zclass_test_node_type_0x42) == 0 ? 0
                                                                                            : 15;
}

extern "C" int zclass_lifecycle_leaf_smoke() {
    g_zClass_NodeArraySize = 0;
    g_zClass_IsInitialized = 0;

    zClass::SetNodeArraySize(12);
    if (g_zClass_NodeArraySize != 12 || zClass::IsInitialized() != 0) {
        return 1;
    }

    zClass::SetNodeArraySize(99);
    if (g_zClass_NodeArraySize != 12) {
        return 2;
    }

    g_zClass_IsInitialized = 1;
    return zClass::IsInitialized() == 1 ? 0 : 3;
}

extern "C" int zclass_init_smoke() {
    if (g_zClass_NodeArray != nullptr) {
        std::free(g_zClass_NodeArray);
    }

    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 3;
    g_zClass_ActiveNodeCount = 9;
    g_zClass_NodeFreeHeadIndex = 99;
    g_zClass_IsInitialized = 0;
    g_zClass_RebuildGwWorldBltRectOnShutdown = 1;

    zZbdSectionHandlerNode sentinel{};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    zZbdManager manager{};
    manager.sectionHandlerListSentinel = &sentinel;
    g_zUtil_ZbdManager = &manager;

    if (zClass::Init() != 0) {
        g_zUtil_ZbdManager = nullptr;
        return 1;
    }

    const bool nodeArrayOk =
        g_zClass_NodeArray != nullptr && g_zClass_NodeArraySize == 3 &&
        g_zClass_ActiveNodeCount == 0 && g_zClass_NodeFreeHeadIndex == 0 &&
        g_zClass_IsInitialized == 1 && g_zClass_NodeArray[0].freeTag == 1 &&
        g_zClass_NodeArray[1].freeTag == 2 &&
        (g_zClass_NodeArray[2].freeTag & 0x00ffffff) == 0x00ffffff;

    const bool handlerOk =
        manager.sectionHandlerCount == 1 && sentinel.next != &sentinel &&
        sentinel.next->sectionHandler.sectionName == g_zClass_GWWorldNodeName &&
        std::strcmp(sentinel.next->sectionHandler.sectionName, "GWWorld") == 0 &&
        sentinel.next->sectionHandler.onPreLoad != nullptr &&
        sentinel.next->sectionHandler.onDataReady != nullptr &&
        sentinel.next->sectionHandler.sortOrder == 1000 &&
        sentinel.next->sectionHandler.userData == nullptr;

    zClass::ShutdownCore();
    if (sentinel.next != &sentinel) {
        zZbdSectionHandlerNode *node = sentinel.next;
        sentinel.next = &sentinel;
        sentinel.prev = &sentinel;
        delete node;
    }
    g_zUtil_ZbdManager = nullptr;
    return nodeArrayOk && handlerOk ? 0 : 2;
}

extern "C" int zclass_zbd_leaf_helpers_smoke() {
    std::strcpy(g_zClass_CurrentZbdPath, "maps\\level.zbd");
    if (zClass::ResetCurrentZbdPath() != 0 || g_zClass_CurrentZbdPath[0] != 0) {
        return 1;
    }

    zClass_NodeFreeListSlot slots[3]{};
    g_zClass_NodeArray = slots;
    slots[0].freeTag = 0x01000000;
    slots[1].freeTag = 0x00ffffff;
    if (GameZ_ZBD::NodePtrToIndex(nullptr) != -1 ||
        GameZ_ZBD::NodePtrToIndex(&slots[2].node) != 2 ||
        GameZ_ZBD::NodeIndexToPtr(-1) != nullptr ||
        GameZ_ZBD::NodeIndexToPtr(1) != &slots[1].node ||
        zClass::NodePtrToValidatedIndex(nullptr) != -1 ||
        zClass::NodePtrToValidatedIndex(&slots[0].node) != 0 ||
        zClass::NodePtrToValidatedIndex(&slots[1].node) != -1) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_zbd_node_ref_list_indices_smoke() {
    zClass_NodeFreeListSlot slots[3]{};
    g_zClass_NodeArray = slots;

    zClass_NodePartial *refs[3] = {
        &slots[2].node,
        nullptr,
        &slots[0].node,
    };

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (GameZ_ZBD::WriteNodeRefListIndices(refs, 3, file) != 0 ||
        GameZ_ZBD::WriteNodeRefListIndices(refs, 0, file) != 0) {
        std::fclose(file);
        return 2;
    }

    std::rewind(file);
    std::int32_t indices[3] = {};
    if (std::fread(indices, sizeof(indices), 1, file) != 1 || indices[0] != 2 || indices[1] != -1 ||
        indices[2] != 0) {
        std::fclose(file);
        return 3;
    }

    std::rewind(file);
    zClass_NodePartial *readRefs[3] = {};
    if (GameZ_ZBD::ReadNodeRefListIndices(readRefs, 3, file) != 0 ||
        readRefs[0] != &slots[2].node || readRefs[1] != nullptr || readRefs[2] != &slots[0].node ||
        GameZ_ZBD::ReadNodeRefListIndices(readRefs, 0, file) != 0) {
        std::fclose(file);
        return 4;
    }

    std::fclose(file);
    std::free(g_GameZ_Zbd_NodeIndexScratch);
    g_GameZ_Zbd_NodeIndexScratch = nullptr;
    g_GameZ_Zbd_NodeIndexScratchCapacity = 0;
    return 0;
}

extern "C" int zclass_zbd_single_node_class_data_smoke() {
    zClass_NodeFreeListSlot slots[5]{};
    g_zClass_NodeArray = slots;

    zClass_CameraDataPartial cameraData{};
    cameraData.worldNode = &slots[2].node;
    cameraData.windowNode = &slots[0].node;
    cameraData.horizonNode = nullptr;
    cameraData.horizonXZNode = &slots[4].node;
    cameraData.cameraFlags = 0x12345678;

    zClass_NodePartial *cameraListA[1] = {&slots[1].node};
    zClass_NodePartial *cameraListB[1] = {&slots[3].node};
    zClass_NodePartial cameraNode{};
    std::strcpy(cameraNode.name, "camera_node");
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraNode.listCountA = 1;
    cameraNode.listA = cameraListA;
    cameraNode.listCountB = 1;
    cameraNode.listB = cameraListB;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (GameZ_ZBD::WriteSingleNodeClassData(&cameraNode, file) != 1) {
        std::fclose(file);
        return 2;
    }

    if (std::fseek(file, 0, SEEK_END) != 0 ||
        std::ftell(file) != static_cast<long>(sizeof(zClass_CameraDataPartial) + 8)) {
        std::fclose(file);
        return 3;
    }

    std::rewind(file);
    std::int32_t cameraIndices[4] = {};
    if (std::fread(cameraIndices, sizeof(cameraIndices), 1, file) != 1 || cameraIndices[0] != 2 ||
        cameraIndices[1] != 0 || cameraIndices[2] != -1 || cameraIndices[3] != 4) {
        std::fclose(file);
        return 4;
    }

    std::int32_t cameraListIndices[2] = {};
    if (std::fseek(file, static_cast<long>(sizeof(zClass_CameraDataPartial)), SEEK_SET) != 0 ||
        std::fread(cameraListIndices, sizeof(cameraListIndices), 1, file) != 1 ||
        cameraListIndices[0] != 1 || cameraListIndices[1] != 3) {
        std::fclose(file);
        return 5;
    }

    std::fclose(file);

    zClass_NodePartial *lightNodes[1] = {&slots[4].node};
    zClass_NodePartial *soundNodes[2] = {nullptr, &slots[1].node};
    zClass_NodePartial *areaChildren[1] = {&slots[2].node};
    zWorldAreaPartial areas[2] = {};
    areas[0].areaFlags = 0x55aa;
    areas[0].childCount = 1;
    areas[0].childList = areaChildren;
    areas[1].areaFlags = 0x33cc;
    zWorldAreaPartial *rows[1] = {areas};

    zClass_WorldDataPartial worldData{};
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    worldData.lightCount = 1;
    worldData.lightNodes = lightNodes;
    worldData.soundCount = 2;
    worldData.soundNodes = soundNodes;

    zClass_NodePartial worldNode{};
    std::strcpy(worldNode.name, "world_node");
    worldNode.classId = 2;
    worldNode.classData = &worldData;

    file = std::tmpfile();
    if (file == nullptr) {
        return 6;
    }

    if (GameZ_ZBD::WriteSingleNodeClassData(&worldNode, file) != 1) {
        std::fclose(file);
        return 7;
    }

    const long worldNodeListOffset = static_cast<long>(sizeof(zClass_WorldDataPartial));
    const long firstAreaOffset = worldNodeListOffset + 12;
    const long firstAreaChildRefOffset =
        firstAreaOffset + static_cast<long>(sizeof(zWorldAreaPartial));
    const long expectedWorldBytes =
        firstAreaChildRefOffset + 4 + static_cast<long>(sizeof(zWorldAreaPartial));
    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != expectedWorldBytes) {
        std::fclose(file);
        return 8;
    }

    std::int32_t worldIndices[3] = {};
    if (std::fseek(file, worldNodeListOffset, SEEK_SET) != 0 ||
        std::fread(worldIndices, sizeof(worldIndices), 1, file) != 1 || worldIndices[0] != 4 ||
        worldIndices[1] != -1 || worldIndices[2] != 1) {
        std::fclose(file);
        return 9;
    }

    std::int32_t areaChildIndex = 0;
    zWorldAreaPartial secondArea{};
    if (std::fseek(file, firstAreaChildRefOffset, SEEK_SET) != 0 ||
        std::fread(&areaChildIndex, sizeof(areaChildIndex), 1, file) != 1 || areaChildIndex != 2 ||
        std::fread(&secondArea, sizeof(secondArea), 1, file) != 1 ||
        secondArea.areaFlags != 0x33cc) {
        std::fclose(file);
        return 10;
    }

    std::fclose(file);
    std::free(g_GameZ_Zbd_NodeIndexScratch);
    g_GameZ_Zbd_NodeIndexScratch = nullptr;
    g_GameZ_Zbd_NodeIndexScratchCapacity = 0;
    return 0;
}

extern "C" int zclass_zbd_read_single_node_class_data_smoke() {
    zClass_NodeFreeListSlot slots[6]{};
    g_zClass_NodeArray = slots;

    reset_zclass_type_lists_for_test();

    zClass_CameraDataPartial cameraDisk{};
    cameraDisk.worldNode = reinterpret_cast<zClass_NodePartial *>(2);
    cameraDisk.windowNode = reinterpret_cast<zClass_NodePartial *>(-1);
    cameraDisk.horizonNode = reinterpret_cast<zClass_NodePartial *>(0);
    cameraDisk.horizonXZNode = reinterpret_cast<zClass_NodePartial *>(4);
    cameraDisk.nearClip = 0.25f;
    cameraDisk.farClip = 500.0f;
    cameraDisk.viewportWidth = 320.0f;
    cameraDisk.viewportHeight = 200.0f;
    cameraDisk.frustumWidth = 640.0f;
    cameraDisk.frustumHeight = 480.0f;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    const std::int32_t cameraListIndices[2] = {1, 3};
    if (std::fwrite(&cameraDisk, sizeof(cameraDisk), 1, file) != 1 ||
        std::fwrite(cameraListIndices, sizeof(cameraListIndices), 1, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 2;
    }
    std::rewind(file);

    zClass_NodePartial cameraNode{};
    std::strcpy(cameraNode.name, "camera_read");
    cameraNode.classId = 1;
    cameraNode.listCountA = 1;
    cameraNode.listCountB = 1;

    if (GameZ_ZBD::ReadSingleNodeClassData(&cameraNode, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 3;
    }
    std::fclose(file);

    zClass_CameraDataPartial *cameraData =
        static_cast<zClass_CameraDataPartial *>(cameraNode.classData);
    const float maxCameraFov = 1.39600003f;
    const bool cameraOk =
        cameraData != nullptr && cameraData->worldNode == &slots[2].node &&
        cameraData->windowNode == nullptr && cameraData->horizonNode == &slots[0].node &&
        cameraData->horizonXZNode == &slots[4].node && cameraData->nearClip == 0.25f &&
        cameraData->farClip == 500.0f && cameraData->viewportWidth == 320.0f &&
        cameraData->viewportHeight == 200.0f && cameraData->fovX == maxCameraFov &&
        cameraData->fovY == maxCameraFov && cameraData->frustumVectorsDirty == 1 &&
        cameraData->localFrustumNormalsDirty == 1 && cameraNode.listA != nullptr &&
        cameraNode.listA[0] == &slots[1].node && cameraNode.listB != nullptr &&
        cameraNode.listB[0] == &slots[3].node && zClass_TypeList::Head(8) != nullptr &&
        zClass_TypeList::Head(8)->node == &cameraNode;
    std::free(cameraNode.listA);
    std::free(cameraNode.listB);
    std::free(cameraData);
    cameraNode.listA = nullptr;
    cameraNode.listB = nullptr;
    cameraNode.classData = nullptr;
    free_zclass_type_lists_for_test();
    if (!cameraOk) {
        return 4;
    }

    reset_zclass_type_lists_for_test();

    zClass_WorldDataPartial worldDisk{};
    worldDisk.fogState = 2;
    worldDisk.ambientColor.red = 0.25f;
    worldDisk.ambientColor.green = 0.5f;
    worldDisk.ambientColor.blue = 0.75f;
    worldDisk.fogDistanceStart = 12.0f;
    worldDisk.fogDistanceEnd = 64.0f;
    worldDisk.fogHeightHigh = 18.0f;
    worldDisk.fogHeightLow = -4.0f;
    worldDisk.fogDensity = 0.125f;
    worldDisk.areaGridColCount = 2;
    worldDisk.areaGridRowCount = 1;
    worldDisk.lightCount = 1;
    worldDisk.soundCount = 1;

    zWorldAreaPartial firstArea{};
    firstArea.areaFlags = 0x22;
    firstArea.childCount = 1;
    zWorldAreaPartial secondArea{};
    secondArea.areaFlags = 0x44;

    const std::int32_t worldLightIndex = 1;
    const std::int32_t worldSoundIndex = 2;
    const std::int32_t areaChildIndex = 3;

    file = std::tmpfile();
    if (file == nullptr) {
        free_zclass_type_lists_for_test();
        return 5;
    }
    if (std::fwrite(&worldDisk, sizeof(worldDisk), 1, file) != 1 ||
        std::fwrite(&worldLightIndex, sizeof(worldLightIndex), 1, file) != 1 ||
        std::fwrite(&worldSoundIndex, sizeof(worldSoundIndex), 1, file) != 1 ||
        std::fwrite(&firstArea, sizeof(firstArea), 1, file) != 1 ||
        std::fwrite(&areaChildIndex, sizeof(areaChildIndex), 1, file) != 1 ||
        std::fwrite(&secondArea, sizeof(secondArea), 1, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 6;
    }
    std::rewind(file);

    zClass_NodePartial worldNode{};
    std::strcpy(worldNode.name, "world_read");
    worldNode.classId = 2;

    if (GameZ_ZBD::ReadSingleNodeClassData(&worldNode, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 7;
    }
    std::fclose(file);

    zClass_WorldDataPartial *worldData =
        static_cast<zClass_WorldDataPartial *>(worldNode.classData);
    const bool worldOk =
        worldData != nullptr && worldData->flags == 0 && worldData->pendingAreaUpdateCount == 0 &&
        worldData->pendingAreaUpdateCapacity == 0 && worldData->pendingAreaUpdates == nullptr &&
        worldData->lightNodes != nullptr && worldData->lightNodes[0] == &slots[1].node &&
        worldData->lightDataList != nullptr && worldData->soundNodes != nullptr &&
        worldData->soundNodes[0] == &slots[2].node && worldData->soundDataList != nullptr &&
        worldData->areaGridRows != nullptr && worldData->areaGridRows[0] != nullptr &&
        worldData->areaGridRows[0][0].areaFlags == 0x22 &&
        worldData->areaGridRows[0][0].childList != nullptr &&
        worldData->areaGridRows[0][0].childList[0] == &slots[3].node &&
        worldData->areaGridRows[0][1].areaFlags == 0x44 &&
        worldData->areaGridRows[0][1].childList == nullptr &&
        zClass_TypeList::Head(13) != nullptr && zClass_TypeList::Head(13)->node == &worldNode;

    if (worldData != nullptr) {
        std::free(worldData->lightNodes);
        std::free(worldData->lightDataList);
        std::free(worldData->soundNodes);
        std::free(worldData->soundDataList);
        if (worldData->areaGridRows != nullptr) {
            if (worldData->areaGridRows[0] != nullptr) {
                std::free(worldData->areaGridRows[0][0].childList);
                std::free(worldData->areaGridRows[0]);
            }
            std::free(worldData->areaGridRows);
        }
        std::free(worldData);
    }
    free_zclass_type_lists_for_test();
    return worldOk ? 0 : 8;
}

extern "C" int zclass_zbd_read_node_table_smoke() {
    reset_zclass_type_lists_for_test();

    zClass_NodeFreeListSlot diskSlots[3]{};
    std::strcpy(diskSlots[0].node.name, "world_table");
    diskSlots[0].node.classId = 2;
    diskSlots[0].node.userDataOrDiRef = static_cast<std::uint32_t>(-1);
    diskSlots[0].freeTag = 0x12000000u;
    std::strcpy(diskSlots[1].node.name, "light_table");
    diskSlots[1].node.classId = 9;
    diskSlots[1].node.userDataOrDiRef = static_cast<std::uint32_t>(-1);
    diskSlots[1].freeTag = 0x34000000u;
    std::strcpy(diskSlots[2].node.name, "sound_table");
    diskSlots[2].node.classId = 10;
    diskSlots[2].node.userDataOrDiRef = static_cast<std::uint32_t>(-1);
    diskSlots[2].freeTag = 0x56000000u;

    zClass_WorldDataPartial worldDisk{};
    worldDisk.lightCount = 1;
    worldDisk.soundCount = 1;
    worldDisk.fogState = 0;
    worldDisk.fogDistanceStart = 10.0f;
    worldDisk.fogDistanceEnd = 20.0f;
    worldDisk.fogHeightHigh = 30.0f;
    worldDisk.fogHeightLow = 5.0f;
    worldDisk.fogDensity = 0.5f;
    const std::int32_t lightIndex = 1;
    const std::int32_t soundIndex = 2;
    zClass_LightDataPartial lightDisk{};
    zClass_SoundDataPartial soundDisk{};

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        free_zclass_type_lists_for_test();
        return 1;
    }
    if (std::fwrite(diskSlots, sizeof(diskSlots), 1, file) != 1 ||
        std::fwrite(&worldDisk, sizeof(worldDisk), 1, file) != 1 ||
        std::fwrite(&lightIndex, sizeof(lightIndex), 1, file) != 1 ||
        std::fwrite(&soundIndex, sizeof(soundIndex), 1, file) != 1 ||
        std::fwrite(&lightDisk, sizeof(lightDisk), 1, file) != 1 ||
        std::fwrite(&soundDisk, sizeof(soundDisk), 1, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 2;
    }
    std::rewind(file);

    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_ActiveNodeCount = 0;

    if (GameZ_ZBD::ReadNodeTable(3, file) != 3) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 3;
    }
    std::fclose(file);

    zClass_WorldDataPartial *worldData =
        static_cast<zClass_WorldDataPartial *>(g_zClass_NodeArray[0].node.classData);
    const bool tableOk = g_zClass_NodeArraySize == 3 && g_zClass_ActiveNodeCount == 3 &&
                         (g_zClass_NodeArray[0].freeTag & 0x01000000u) != 0 &&
                         (g_zClass_NodeArray[1].freeTag & 0x01000000u) != 0 &&
                         (g_zClass_NodeArray[2].freeTag & 0x01000000u) != 0 &&
                         g_zClass_NodeArray[0].node.actionCallback == nullptr &&
                         g_zClass_NodeArray[1].node.actionCallback == nullptr &&
                         g_zClass_NodeArray[2].node.actionCallback == nullptr &&
                         worldData != nullptr && worldData->lightNodes != nullptr &&
                         worldData->lightNodes[0] == &g_zClass_NodeArray[1].node &&
                         worldData->soundNodes != nullptr &&
                         worldData->soundNodes[0] == &g_zClass_NodeArray[2].node &&
                         worldData->lightDataList != nullptr &&
                         worldData->lightDataList[0] == g_zClass_NodeArray[1].node.classData &&
                         worldData->soundDataList != nullptr &&
                         worldData->soundDataList[0] == g_zClass_NodeArray[2].node.classData &&
                         zClass_TypeList::Head(13) != nullptr &&
                         zClass_TypeList::Head(13)->node == &g_zClass_NodeArray[0].node;

    if (worldData != nullptr) {
        std::free(worldData->lightNodes);
        std::free(worldData->lightDataList);
        std::free(worldData->soundNodes);
        std::free(worldData->soundDataList);
        std::free(worldData->areaGridRows);
        std::free(worldData);
    }
    std::free(g_zClass_NodeArray[1].node.classData);
    std::free(g_zClass_NodeArray[2].node.classData);
    std::free(g_zClass_NodeArray);
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_ActiveNodeCount = 0;
    free_zclass_type_lists_for_test();
    return tableOk ? 0 : 4;
}

extern "C" int zclass_zbd_write_node_table_smoke() {
    zClass_NodeFreeListSlot slots[2]{};
    g_zClass_NodeArray = slots;
    g_zClass_NodeArraySize = 2;

    zDiPartial diPool[2]{};
    g_zModel_DiPoolBase = diPool;

    zClass_Object3DDataPartial objectData{};
    objectData.flags = 0x1357;
    zClass_NodePartial *staleList[1] = {&slots[1].node};
    std::strcpy(slots[0].node.name, "object_node");
    slots[0].node.classId = 5;
    slots[0].node.classData = &objectData;
    slots[0].node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&diPool[1]));
    slots[0].node.actionCallback = staleList;
    slots[0].node.listCountA = 0;
    slots[0].node.listA = staleList;
    slots[0].node.listCountB = 0;
    slots[0].node.listB = staleList;
    slots[0].freeTag = 0xab123456u;

    std::strcpy(slots[1].node.name, "empty_node");
    slots[1].node.classId = 0;
    slots[1].freeTag = 0xcd654321u;

    g_GameZ_Zbd_NodeIndexScratch = static_cast<zClass_NodePartial **>(std::malloc(8));
    g_GameZ_Zbd_NodeIndexScratchCapacity = 2;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (GameZ_ZBD::WriteNodeTable(file) != 2 || g_GameZ_Zbd_NodeIndexScratch != nullptr ||
        g_GameZ_Zbd_NodeIndexScratchCapacity != 0) {
        std::fclose(file);
        return 2;
    }

    const long tableBytes = static_cast<long>(sizeof(zClass_NodeFreeListSlot) * 2);
    const long expectedBytes = tableBytes + static_cast<long>(sizeof(zClass_Object3DDataPartial));
    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != expectedBytes) {
        std::fclose(file);
        return 3;
    }

    zClass_NodeFreeListSlot written[2]{};
    std::rewind(file);
    if (std::fread(written, sizeof(written), 1, file) != 1) {
        std::fclose(file);
        return 4;
    }

    if (written[0].node.userDataOrDiRef != 1 ||
        written[1].node.userDataOrDiRef != static_cast<std::uint32_t>(-1) ||
        written[0].node.actionCallback != nullptr || written[0].node.listA != nullptr ||
        written[0].node.listB != nullptr) {
        std::fclose(file);
        return 5;
    }

    const std::uint32_t expectedFreeTag =
        (slots[0].freeTag & 0xff000000u) | static_cast<std::uint32_t>(tableBytes);
    if (written[0].freeTag != expectedFreeTag || written[1].freeTag != slots[1].freeTag) {
        std::fclose(file);
        return 6;
    }

    zClass_Object3DDataPartial writtenObject{};
    if (std::fseek(file, tableBytes, SEEK_SET) != 0 ||
        std::fread(&writtenObject, sizeof(writtenObject), 1, file) != 1 ||
        writtenObject.flags != objectData.flags) {
        std::fclose(file);
        return 7;
    }

    std::fclose(file);
    g_zClass_NodeArraySize = 0;
    return 0;
}

extern "C" int gamez_write_zbd_file_smoke() {
    const char *path = "zbd_smoke.tmp";
    std::remove(path);

    std::strcpy(g_zClass_CurrentZbdPath, "old.zbd");
    g_zImage_TexDirEntryCount = 0;

    zModel_MaterialSlot materialSlots[1] = {};
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 1;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_NodeFreeHeadIndex = 5;
    g_zClass_NodeList_PendingFreeHead = nullptr;

    if (GameZ::WriteZBDFile(path) != 0 || std::strcmp(g_zClass_CurrentZbdPath, path) != 0) {
        std::remove(path);
        return 1;
    }

    std::FILE *file = std::fopen(path, "rb");
    if (file == nullptr) {
        std::remove(path);
        return 2;
    }

    zClass_ZbdHeader header = {};
    if (std::fread(&header, sizeof(header), 1, file) != 1) {
        std::fclose(file);
        std::remove(path);
        return 3;
    }

    const long materialOffset = 0x24;
    const long model3dOffset = materialOffset + 16 + static_cast<long>(sizeof(zModel_MaterialSlot));
    const long nodeTableOffset = model3dOffset + 12;
    const bool headerOk = header.magic == 0x02971222 && header.version == 0x0f &&
                          header.texDirArg == 0 && header.texDirOffset == 0x24 &&
                          header.matlOffset == materialOffset &&
                          header.model3dOffset == model3dOffset && header.nodeCount == 0 &&
                          header.nodeFreeHead == 5 && header.nodeTableOffset == nodeTableOffset;

    std::int32_t materialHeader[4] = {};
    const bool materialOk = std::fseek(file, materialOffset, SEEK_SET) == 0 &&
                            std::fread(materialHeader, sizeof(materialHeader), 1, file) == 1 &&
                            materialHeader[0] == 1 && materialHeader[1] == 0 &&
                            materialHeader[2] == 0 && materialHeader[3] == -1;

    const bool sizeOk = std::fseek(file, 0, SEEK_END) == 0 && std::ftell(file) == nodeTableOffset;

    std::fclose(file);
    std::remove(path);
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_CurrentZbdPath[0] = '\0';

    return headerOk && materialOk && sizeOk ? 0 : 4;
}

extern "C" int gamez_read_zbd_file_smoke() {
    const char *path = "zbd_read_smoke.tmp";
    std::remove(path);

    std::strcpy(g_zClass_CurrentZbdPath, "old.zbd");
    g_zImage_TexDirEntryCount = 0;

    zModel_MaterialSlot materialSlots[1] = {};
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 1;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_NodeFreeHeadIndex = 5;
    g_zClass_NodeList_PendingFreeHead = nullptr;

    if (GameZ::WriteZBDFile(path) != 0) {
        std::remove(path);
        return 1;
    }

    std::strcpy(g_zClass_CurrentZbdPath, "before-read.zbd");
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = -3;
    g_zModel_DiPoolInUseCount = -4;
    g_zModel_DiPoolFreeHeadIndex = -5;
    g_zClass_NodeFreeHeadIndex = -6;

    if (GameZ::ReadZBDFile(path) != 0) {
        std::free(g_zModel_MatlPool);
        g_zModel_MatlPool = nullptr;
        std::remove(path);
        return 2;
    }

    const bool ok = std::strcmp(g_zClass_CurrentZbdPath, path) == 0 &&
                    g_zClass_NodeFreeHeadIndex == 5 && g_zModel_MatlPool != nullptr &&
                    g_zModel_MatlPoolCapacity == 1 && g_zModel_MatlPoolInUseCount == 0 &&
                    g_zModel_MatlFreeHeadIndex == 0 && g_zModel_MatlActiveHeadIndex == -1 &&
                    g_zModel_DiPoolBase == nullptr && g_zModel_DiPoolCapacity == 0 &&
                    g_zModel_DiPoolInUseCount == 0 && g_zModel_DiPoolFreeHeadIndex == -1;

    std::free(g_zModel_MatlPool);
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_CurrentZbdPath[0] = '\0';
    std::remove(path);

    return ok ? 0 : 3;
}

extern "C" int gamez_open_and_read_zbd_header_smoke() {
    const char *path = "zbd_header_smoke.tmp";
    std::remove(path);

    zClass_ZbdHeader expected = {};
    expected.magic = 0x02971222;
    expected.version = 0x0f;
    expected.texDirArg = 7;
    expected.texDirOffset = 0x24;
    expected.matlOffset = 0x40;
    expected.model3dOffset = 0x80;
    expected.nodeCount = 3;
    expected.nodeFreeHead = 2;
    expected.nodeTableOffset = 0xc0;

    std::FILE *file = std::fopen(path, "wb");
    if (file == nullptr) {
        return 1;
    }

    std::fwrite(&expected, sizeof(expected), 1, file);
    std::fputc(0x5a, file);
    std::fclose(file);

    zClass_ZbdHeader actual = {};
    file = GameZ::OpenAndReadZBDHeader(path, &actual);
    if (file == nullptr) {
        std::remove(path);
        return 2;
    }

    const bool successOk = std::memcmp(&actual, &expected, sizeof(actual)) == 0 &&
                           std::ftell(file) == static_cast<long>(sizeof(zClass_ZbdHeader)) &&
                           std::fgetc(file) == 0x5a;
    std::fclose(file);
    if (!successOk) {
        std::remove(path);
        return 3;
    }

    file = std::fopen(path, "wb");
    if (file == nullptr) {
        std::remove(path);
        return 4;
    }
    std::fwrite(&expected, sizeof(expected) - 1, 1, file);
    std::fclose(file);
    if (GameZ::OpenAndReadZBDHeader(path, &actual) != nullptr) {
        std::remove(path);
        return 5;
    }

    file = std::fopen(path, "wb");
    if (file == nullptr) {
        std::remove(path);
        return 6;
    }
    expected.magic = 0x12345678;
    expected.version = 0x0f;
    std::fwrite(&expected, sizeof(expected), 1, file);
    std::fclose(file);
    if (GameZ::OpenAndReadZBDHeader(path, &actual) != nullptr) {
        std::remove(path);
        return 7;
    }

    file = std::fopen(path, "wb");
    if (file == nullptr) {
        std::remove(path);
        return 8;
    }
    expected.magic = 0x02971222;
    expected.version = 0x10;
    std::fwrite(&expected, sizeof(expected), 1, file);
    std::fclose(file);
    if (GameZ::OpenAndReadZBDHeader(path, &actual) != nullptr) {
        std::remove(path);
        return 9;
    }

    std::remove(path);
    return 0;
}

extern "C" int gamez_read_retail_m1_zbd_smoke() {
    char oldDir[MAX_PATH] = {};
    if (!EnterSupportDirectoryForRetailZbdTest(oldDir, sizeof(oldDir))) {
        return 1;
    }

    int result = 0;
    const char *const path = "zbd\\m1\\gamez.zbd";
    zClass_ZbdHeader header = {};
    std::FILE *file = GameZ::OpenAndReadZBDHeader(path, &header);
    if (file == nullptr) {
        result = 2;
        goto cleanup;
    }

    if (header.magic != 0x02971222 || header.version != 15 || header.texDirArg != 471 ||
        header.texDirOffset != 36 || header.matlOffset != 16992 ||
        header.model3dOffset != 237008 || header.nodeCount != 16000 ||
        header.nodeFreeHead != 4199 || header.nodeTableOffset != 1785172) {
        std::fclose(file);
        file = nullptr;
        result = 3;
        goto cleanup;
    }
    std::fclose(file);
    file = nullptr;

    reset_zclass_type_lists_for_test();
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 0;
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_CurrentZbdPath[0] = '\0';

    if (GameZ::ReadZBDFile(path) != 0) {
        result = 4;
        goto cleanup;
    }

    if (std::strcmp(g_zClass_CurrentZbdPath, path) != 0 || g_zClass_NodeArray == nullptr ||
        g_zClass_NodeArraySize != 16000 || g_zClass_ActiveNodeCount != 4199 ||
        g_zClass_NodeFreeHeadIndex != 4199 || g_zImage_TexDirEntryCount != 471 ||
        g_zModel_MatlPool == nullptr || g_zModel_MatlPoolCapacity <= 0 ||
        g_zModel_MatlPoolInUseCount <= 0 || g_zModel_DiPoolBase == nullptr ||
        g_zModel_DiPoolCapacity <= 0 || g_zModel_DiPoolInUseCount <= 0) {
        result = 5;
        goto cleanup;
    }

cleanup:
    if (file != nullptr) {
        std::fclose(file);
    }
    zModel_Display::Shutdown();
    if (g_zClass_NodeArray != nullptr) {
        std::free(g_zClass_NodeArray);
    }
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_CurrentZbdPath[0] = '\0';
    free_zclass_type_lists_for_test();
    SetCurrentDirectoryA(oldDir);
    return result;
}

extern "C" int gamez_reload_display_instances_smoke() {
    reset_zclass_type_lists_for_test();

    const char *path = "zbd_reload_di_smoke.tmp";
    std::remove(path);

    zClass_NodeFreeListSlot nodeSlots[2] = {};
    zClass_NodePartial *rootChildren[1] = {&nodeSlots[1].node};
    nodeSlots[0].node.listCountB = 1;
    nodeSlots[0].node.listB = rootChildren;
    g_zClass_NodeArray = nodeSlots;
    g_zClass_NodeArraySize = 2;

    zDiPartial liveDiPool[2] = {};
    liveDiPool[0].refCount = 1;
    liveDiPool[1].nextFreeIndex = -1;
    nodeSlots[0].node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&liveDiPool[0]));

    g_zModel_DiPoolBase = liveDiPool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 1;
    g_zModel_DiPoolFreeHeadIndex = 1;

    zClass_ZbdHeader header = {};
    header.magic = 0x02971222;
    header.version = 0x0f;
    header.nodeCount = 2;
    header.nodeTableOffset = sizeof(zClass_ZbdHeader);
    header.model3dOffset =
        header.nodeTableOffset + 2 * static_cast<std::int32_t>(sizeof(zClass_NodeFreeListSlot));

    zClass_NodeFreeListSlot serializedNodes[2] = {};
    serializedNodes[0].node.userDataOrDiRef = 0;
    serializedNodes[1].node.userDataOrDiRef = 1;

    std::int32_t diHeader[3] = {2, 2, -1};
    zDiPartial serializedDi[2] = {};
    const std::int32_t dynamicOffset = header.model3dOffset +
                                       static_cast<std::int32_t>(sizeof(diHeader)) +
                                       2 * static_cast<std::int32_t>(sizeof(zDiPartial));
    serializedDi[0].textureWorldAxis = 101;
    serializedDi[0].nextFreeIndex = dynamicOffset;
    serializedDi[1].textureWorldAxis = 202;
    serializedDi[1].nextFreeIndex = dynamicOffset;

    std::FILE *file = std::fopen(path, "wb");
    if (file == nullptr) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    if (std::fwrite(&header, sizeof(header), 1, file) != 1 ||
        std::fwrite(serializedNodes, sizeof(serializedNodes), 1, file) != 1 ||
        std::fwrite(diHeader, sizeof(diHeader), 1, file) != 1 ||
        std::fwrite(serializedDi, sizeof(serializedDi), 1, file) != 1) {
        std::fclose(file);
        std::remove(path);
        free_zclass_type_lists_for_test();
        return 2;
    }
    std::fclose(file);

    std::strcpy(g_zClass_CurrentZbdPath, path);
    const std::int32_t result =
        GameZ_ZBD::ReloadDisplayInstancesFromCurrentPath_Local(&nodeSlots[0].node, 1);

    zDiPartial *const rootDi = reinterpret_cast<zDiPartial *>(
        static_cast<std::uintptr_t>(nodeSlots[0].node.userDataOrDiRef));
    zDiPartial *const childDi = reinterpret_cast<zDiPartial *>(
        static_cast<std::uintptr_t>(nodeSlots[1].node.userDataOrDiRef));
    const bool ok = result == 0 && rootDi == &liveDiPool[1] && childDi == &liveDiPool[0] &&
                    rootDi->textureWorldAxis == 101 && childDi->textureWorldAxis == 202 &&
                    rootDi->refCount == 1 && childDi->refCount == 1 &&
                    g_zModel_DiPoolInUseCount == 2 && g_zModel_DiPoolFreeHeadIndex == -1;

    std::remove(path);
    g_zClass_CurrentZbdPath[0] = '\0';
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    free_zclass_type_lists_for_test();

    return ok ? 0 : 3;
}

extern "C" int zclass_try_free_node_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[3]{};
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 2;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *immediate = &slots[1].node;
    immediate->flags = 2;
    immediate->classId = 5;
    immediate->classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    if (zClass_Class::TryFreeNode(immediate) != 0 || immediate->classData != nullptr ||
        immediate->classId != 0 || (immediate->flags & 2) != 0 || g_zClass_NodeFreeHeadIndex != 1 ||
        g_zClass_ActiveNodeCount != 1) {
        return 1;
    }

    zClass_NodePartial *deferred = &slots[2].node;
    deferred->flags = 2;
    deferred->classId = 5;
    deferred->classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    g_zClass_DeferredProcessingEnabled = 0;
    if (zClass_Class::TryFreeNode(deferred) != 0 || deferred->classData == nullptr ||
        (deferred->flags & 2) != 0 || g_zClass_NodeList_PendingFreeHead == nullptr ||
        g_zClass_NodeList_PendingFreeHead->node != deferred) {
        return 2;
    }

    g_zClass_DeferredProcessingEnabled = 1;
    zClass_NodeList::ProcessPendingFrees();
    if (g_zClass_NodeList_PendingFreeHead != nullptr || deferred->classData != nullptr ||
        g_zClass_NodeFreeHeadIndex != 2 || g_zClass_ActiveNodeCount != 0) {
        return 3;
    }

    return zClass_Class::TryFreeNode(nullptr) == 5 ? 0 : 4;
}

extern "C" int zdi_ref_and_pool_free_smoke() {
    zDiPartial pool[2]{};
    g_zModel_DiPoolBase = pool;
    g_zModel_DiPoolInUseCount = 2;
    g_zModel_DiPoolFreeHeadIndex = 7;

    if (zDi::AddRef(&pool[0]) != 0 || pool[0].refCount != 1 || zDi::Release(&pool[0]) != 0 ||
        pool[0].refCount != 0 || zDi::PtrToIndexOrMinus1(nullptr) != -1 ||
        zDi::PtrToIndexOrMinus1(&pool[1]) != 1 || zDi::IndexToPtrOrNull(-1) != nullptr ||
        zDi::IndexToPtrOrNull(1) != &pool[1]) {
        return 1;
    }

    pool[1].refCount = 1;
    if (zModel_DiPool::FreeIfUnreferenced(nullptr) != 5 ||
        zModel_DiPool::FreeIfUnreferenced(&pool[1]) != 1 || g_zModel_DiPoolInUseCount != 2) {
        return 2;
    }

    pool[0].mode = 3;
    pool[0].flags = 0x44;
    if (zModel_DiPool::FreeIfUnreferenced(&pool[0]) != 0) {
        return 3;
    }

    return pool[0].mode == 0 && pool[0].flags == 0 && pool[0].nextFreeIndex == 7 &&
                   g_zModel_DiPoolFreeHeadIndex == 0 && g_zModel_DiPoolInUseCount == 1
               ? 0
               : 4;
}

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

extern "C" int zmodel_set_di_texture_world_per_meter_smoke() {
    g_zModel_TextureWorldBaseU = 0.0f;
    g_zModel_TextureWorldBaseV = 0.0f;
    g_zModel_TextureWorldPerMeterU = 0.0f;
    g_zModel_TextureWorldPerMeterV = 0.0f;

    zModel::SetTextureWorldBase(1.25f, -3.5f);
    if (g_zModel_TextureWorldBaseU != 1.25f || g_zModel_TextureWorldBaseV != -3.5f ||
        g_zModel_TextureWorldPerMeterU != 0.0f || g_zModel_TextureWorldPerMeterV != 0.0f) {
        return 1;
    }

    zModel::SetTextureWorldPerMeter(0.5f, 4.75f);
    if (g_zModel_TextureWorldBaseU != 1.25f || g_zModel_TextureWorldBaseV != -3.5f ||
        g_zModel_TextureWorldPerMeterU != 0.5f || g_zModel_TextureWorldPerMeterV != 4.75f) {
        return 2;
    }

    zDiPartial di{};
    di.flags = 0x73;

    if (zModel::SetDiTextureWorldPerMeter(nullptr, 1, 4.0f, 2) != 1) {
        return 3;
    }

    if (zModel::SetDiTextureWorldPerMeter(&di, 0, 2.5f, 7) != 0 ||
        di.flags != 0x53 || di.textureWorldPerMeter != 2.5f || di.textureWorldAxis != 7) {
        return 4;
    }

    if (zModel::SetDiTextureWorldPerMeter(&di, 3, 8.0f, -4) != 0 ||
        di.flags != 0x73 || di.textureWorldPerMeter != 8.0f || di.textureWorldAxis != -4) {
        return 5;
    }

    return 0;
}

extern "C" int zclass_set_display_instance_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodePartial node{};
    zDiPartial oldDisplay{};
    zDiPartial newDisplay{};
    oldDisplay.refCount = 2;
    node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&oldDisplay));

    if (zClass_Class::gwNodeSetDisplayInstance(&node, &newDisplay) != 0 ||
        oldDisplay.refCount != 1 || newDisplay.refCount != 1 ||
        node.userDataOrDiRef !=
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&newDisplay)) ||
        (node.flags & 0x203) != 0x203 || (node.boundsFlags & 1) == 0 ||
        zClass_TypeList::Head(7) == nullptr) {
        return 1;
    }

    if (zClass_Class::gwNodeSetDisplayInstance(&node, nullptr) != 0 || newDisplay.refCount != 0 ||
        node.userDataOrDiRef != 0 || (node.flags & 0x200) != 0) {
        return 2;
    }

    zClass_TypeList::FreeAll();
    return zClass_Class::gwNodeSetDisplayInstance(nullptr, nullptr) == 5 ? 0 : 3;
}

extern "C" int zclass_remove_dispatch_smoke() {
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    zClass_Object3DDataPartial parentData{};
    parent.classId = 5;
    parent.classData = &parentData;

    if (zClass_Class::AddChildGeneric(&parent, &child) != 0 ||
        zClass_Class::RemoveChild(&parent, &child) != 0 || parent.listCountB != 0 ||
        child.listCountA != 0) {
        return 1;
    }

    parent.classId = 99;
    if (zClass_Class::RemoveChild(&parent, &child) != 1 ||
        zClass_Class::RemoveChild(nullptr, &child) != 5 ||
        zClass_Class::RemoveChild(&parent, nullptr) != 5) {
        return 2;
    }

    std::free(parent.listB);
    std::free(child.listA);
    return 0;
}

extern "C" int zclass_destroy_node_recursive_display_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    zDiPartial displayPool[1]{};
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_ActiveNodeCount = 1;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zModel_DiPoolBase = displayPool;
    g_zModel_DiPoolFreeHeadIndex = 4;
    g_zModel_DiPoolInUseCount = 1;

    slot.node.classId = 5;
    slot.node.classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    slot.node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&displayPool[0]));
    displayPool[0].refCount = 1;

    if (zClass_Util::DestroyNodeRecursive(&slot.node) != 0) {
        return 1;
    }

    return slot.node.classData == nullptr && slot.node.userDataOrDiRef == 0 &&
                   displayPool[0].nextFreeIndex == 4 && g_zModel_DiPoolFreeHeadIndex == 0 &&
                   g_zModel_DiPoolInUseCount == 0 && g_zClass_ActiveNodeCount == 0
               ? 0
               : 2;
}

extern "C" int zclass_object3d_delete_node_smoke() {
    zClass_NodeFreeListSlot slot{};
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0x222222;
    g_zClass_ActiveNodeCount = 1;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    slot.node.classId = 5;
    slot.node.classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    if (zClass_Object3D::DeleteNode(&slot.node) != 0 || slot.node.classData != nullptr ||
        g_zClass_ActiveNodeCount != 0) {
        return 1;
    }

    return zClass_Object3D::DeleteNode(nullptr) == 5 ? 0 : 2;
}

extern "C" int zclass_world_new_smoke() {
    reset_zclass_type_lists_for_test();
    g_zClass_NodeList_PendingFreeHead = nullptr;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zClass_NodePartial *world = zClass_World::gwWorldNew();
    if (world != &slot.node || world->classId != 2 || world->classData == nullptr ||
        g_zClass_NodeFreeHeadIndex != -1 || zClass_TypeList::Head(13) == nullptr ||
        zClass_TypeList::Head(13)->node != world) {
        if (world != nullptr && world->classData != nullptr) {
            zClass_World::DeleteNode(world);
        }
        free_zclass_type_lists_for_test();
        return 1;
    }

    zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
    const bool defaultsOk =
        data->flags == 1 && data->fogState == 0 && data->clampQueriesToBounds == 0 &&
        data->partitionMaxDecFeatureCount == 16 && data->lightCount == 0 &&
        data->lightNodes == nullptr && data->lightDataList == nullptr &&
        data->soundCount == 0 && data->soundNodes == nullptr && data->soundDataList == nullptr &&
        data->scaleX == 1.0f && data->scaleY == 1.0f && data->scaleZ == 1.0f;

    zClass_World::DeleteNode(world);
    free_zclass_type_lists_for_test();
    return defaultsOk ? 0 : 2;
}

extern "C" int zclass_world_free_virtual_area_partitions_smoke() {
    zClass_NodePartial world{};
    zClass_WorldDataPartial worldData{};
    world.classId = 2;
    world.classData = &worldData;

    zWorldAreaPartial row0[2]{};
    zWorldAreaPartial row1[2]{};
    zWorldAreaPartial *rows[] = {row0, row1};
    row0[0].childList =
        static_cast<zClass_NodePartial **>(std::calloc(1, sizeof(zClass_NodePartial *)));
    row1[1].childList =
        static_cast<zClass_NodePartial **>(std::calloc(1, sizeof(zClass_NodePartial *)));
    if (row0[0].childList == nullptr || row1[1].childList == nullptr) {
        std::free(row0[0].childList);
        std::free(row1[1].childList);
        return 1;
    }

    worldData.areaGridRows = rows;
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 2;
    worldData.areaCellSizeX = 8.0f;
    worldData.areaCellSizeZ = 4.0f;
    worldData.areaGridExternalOwnership = 1;

    if (zClass_World::FreeVirtualAreaPartitions(&world) != 0 ||
        worldData.areaGridRows != nullptr || worldData.areaGridColCount != 0 ||
        worldData.areaGridRowCount != 0 || worldData.areaCellSizeX != 0.0f ||
        worldData.areaCellSizeZ != 0.0f || row0[0].childList != nullptr ||
        row1[1].childList != nullptr) {
        return 2;
    }

    return zClass_World::FreeVirtualAreaPartitions(&world) == 0 ? 0 : 3;
}

extern "C" int zclass_world_set_virtual_area_partition_smoke() {
    zClass_NodePartial world{};
    zClass_WorldDataPartial worldData{};
    world.classId = 2;
    world.classData = &worldData;
    worldData.originX = 10.0f;
    worldData.originZ = 100.0f;
    worldData.worldSizeX = 65.0f;
    worldData.worldSizeZ = -45.0f;

    if (zClass_World::gwWorldSetVirtualAreaPartition(&world, 20.0f, -20.0f) != 0) {
        return 1;
    }
    if (worldData.areaCellSizeX != 20.0f || worldData.areaCellSizeZ != -20.0f ||
        worldData.partitionInclusionTolX != 2.5f || worldData.partitionInclusionTolZ != 2.5f ||
        worldData.areaHalfSizeX != 10.0f || worldData.areaHalfSizeZ != -10.0f ||
        worldData.areaInvSizeX != 0.05f || worldData.areaInvSizeZ != -0.05f ||
        worldData.areaGridColCount != 4 || worldData.areaGridRowCount != 3 ||
        worldData.areaGridRows == nullptr) {
        zClass_World::FreeVirtualAreaPartitions(&world);
        return 2;
    }

    std::int32_t expectedBiasBits = FloatBitsForTest(20.0f * 20.0f + -20.0f * -20.0f);
    expectedBiasBits = (expectedBiasBits >> 1) + 0x1fc00000;
    float expectedBias = 0.0f;
    std::memcpy(&expectedBias, &expectedBiasBits, sizeof(expectedBias));
    expectedBias *= -0.5f;
    if (FloatBitsForTest(worldData.areaCellRadiusBias) != FloatBitsForTest(expectedBias)) {
        zClass_World::FreeVirtualAreaPartitions(&world);
        return 3;
    }

    zWorldAreaPartial *area = &worldData.areaGridRows[2][3];
    if ((area->areaFlags & 0x100) == 0 || area->areaIndex != -1 || area->cellMinX != 70.0f ||
        area->cellMinZ != 60.0f || area->bbox[0] != 70.0f || area->bbox[2] != 40.0f ||
        area->bbox[3] != 90.0f || area->bbox[5] != 60.0f || area->bboxCenter.x != 80.0f ||
        area->bboxCenter.y != 0.0f || area->bboxCenter.z != 50.0f ||
        area->bboxRadius <= 0.0f) {
        zClass_World::FreeVirtualAreaPartitions(&world);
        return 4;
    }

    return zClass_World::FreeVirtualAreaPartitions(&world) == 0 &&
                   worldData.areaGridRows == nullptr &&
                   worldData.areaGridColCount == 0 &&
                   worldData.areaGridRowCount == 0
               ? 0
               : 5;
}

extern "C" int zclass_world_get_area_partition_at_grid_smoke() {
    zClass_NodePartial world{};
    zClass_WorldDataPartial worldData{};
    zWorldAreaPartial row0[3]{};
    zWorldAreaPartial row1[3]{};
    zWorldAreaPartial *rows[2] = {row0, row1};

    world.classData = &worldData;
    worldData.areaGridRows = rows;
    row1[2].areaIndex = 37;
    row1[2].displayRefreshQueued = 1;

    if (zClass_World::GetAreaPartitionAtGrid(nullptr, 0, 0) != nullptr) {
        return 1;
    }

    world.classData = nullptr;
    if (zClass_World::GetAreaPartitionAtGrid(&world, 0, 0) != nullptr) {
        return 2;
    }

    world.classData = &worldData;
    return zClass_World::GetAreaPartitionAtGrid(&world, 2, 1) == &row1[2] &&
                   row1[2].areaIndex == 37 && row1[2].displayRefreshQueued == 1
               ? 0
               : 3;
}

extern "C" int zclass_world_to_grid_coords_clamped_smoke() {
    zClass_NodePartial world{};
    zClass_WorldDataPartial worldData{};
    world.classData = &worldData;
    worldData.originX = 0.0f;
    worldData.originZ = 0.0f;
    worldData.worldMaxX = 10.0f;
    worldData.worldMaxZ = 10.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;

    int col = -1;
    int row = -1;
    if (zClass_World::WorldToGridCoordsClamped(&world, &col, 4.0f, -5.0f, &row) != 0 ||
        col != 4 || row != 0) {
        return 1;
    }

    col = -1;
    row = -1;
    if (zClass_World::WorldToGridCoordsClamped(&world, &col, -5.0f, 12.0f, &row) != 0 ||
        col != 0 || row != 12) {
        return 2;
    }

    col = -1;
    row = -1;
    if (zClass_World::WorldToGridCoordsClamped(&world, &col, 12.0f, 4.0f, &row) != 0 ||
        col != 9 || row != 10) {
        return 3;
    }

    return 0;
}

extern "C" int zclass_world_animate_delete_node_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[2]{};
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0x333333;
    g_zClass_ActiveNodeCount = 2;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *const world = &slots[0].node;
    zClass_WorldDataPartial *const worldData =
        static_cast<zClass_WorldDataPartial *>(std::calloc(1, sizeof(zClass_WorldDataPartial)));
    if (worldData == nullptr) {
        return 1;
    }
    world->classId = 2;
    world->classData = worldData;
    worldData->lightNodes =
        static_cast<zClass_NodePartial **>(std::calloc(1, sizeof(zClass_NodePartial *)));
    worldData->lightDataList = static_cast<zClass_LightDataPartial **>(
        std::calloc(1, sizeof(zClass_LightDataPartial *)));
    worldData->soundNodes =
        static_cast<zClass_NodePartial **>(std::calloc(1, sizeof(zClass_NodePartial *)));
    worldData->soundDataList = static_cast<zClass_SoundDataPartial **>(
        std::calloc(1, sizeof(zClass_SoundDataPartial *)));
    worldData->pendingAreaUpdates =
        static_cast<zWorldAreaPartial **>(std::calloc(1, sizeof(zWorldAreaPartial *)));
    if (worldData->lightNodes == nullptr || worldData->lightDataList == nullptr ||
        worldData->soundNodes == nullptr || worldData->soundDataList == nullptr ||
        worldData->pendingAreaUpdates == nullptr) {
        return 2;
    }
    if (zClass_World::DeleteNode(world) != 0 || world->classData != nullptr ||
        g_zClass_NodeFreeHeadIndex != 0 || g_zClass_ActiveNodeCount != 1) {
        return 3;
    }

    zClass_NodePartial *const animate = &slots[1].node;
    animate->classId = 8;
    animate->classData = std::calloc(1, sizeof(zClass_AnimateDataPartial));
    if (animate->classData == nullptr) {
        return 4;
    }
    if (zClass_Animate::DeleteNode(animate) != 0 || animate->classData != nullptr ||
        g_zClass_NodeFreeHeadIndex != 1 || g_zClass_ActiveNodeCount != 0) {
        return 5;
    }

    slots[0] = {};
    g_zClass_NodeFreeHeadIndex = 0x444444;
    g_zClass_ActiveNodeCount = 1;
    zClass_NodePartial *const baseNode = &slots[0].node;
    baseNode->classId = 0;
    const int deleteByTypeResult = zClass_Class::DeleteNodeByType(baseNode);
    if (deleteByTypeResult != static_cast<int>(reinterpret_cast<std::uintptr_t>(baseNode)) ||
        g_zClass_NodeFreeHeadIndex != 0 || g_zClass_ActiveNodeCount != 0) {
        return 6;
    }

    zClass_NodePartial parented{};
    parented.listCountA = 1;
    if (zClass_Class::DeleteNodeByType(&parented) != 1) {
        return 7;
    }

    zClass_NodePartial unknown{};
    unknown.classId = 99;
    if (zClass_Class::DeleteNodeByType(&unknown) != 1) {
        return 8;
    }

    zClass_TypeList::FreeAll();
    return zClass_Animate::DeleteNode(nullptr) == 5 ? 0 : 9;
}

extern "C" int zclass_sound_leaf_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *const node = zClass_Sound::gwSoundNew();
    if (node != &slot.node || node->classId != 10 || node->classData == nullptr ||
        (node->flags & 0x104) != 0x104 || g_zClass_NodeFreeHeadIndex != -1 ||
        g_zClass_ActiveNodeCount != 1 || zClass_TypeList::Head(10) == nullptr ||
        zClass_TypeList::Head(10)->node != node) {
        return 1;
    }

    const float expectedBounds[6] = {1.0f, 1.0f, -2.0f, 2.0f, 2.0f, -1.0f};
    for (int i = 0; i < 6; ++i) {
        if (node->cachedBounds[i] != expectedBounds[i]) {
            return 2;
        }
    }

    zClass_SoundDataPartial *const newData =
        static_cast<zClass_SoundDataPartial *>(node->classData);
    if (newData->runtimeFlags != 1 || newData->falloffMode != 1 || newData->rangeMin != 32.0f ||
        newData->rangeMax != 64.0f || newData->rangeMaxSq != 4096.0f ||
        newData->invRangeSpan != 0.03125f || newData->attachedWorldCount != 0 ||
        newData->attachedWorlds != nullptr) {
        return 3;
    }

    if (zClass_Sound::DeleteNode(node) != 0 || slot.node.classData != nullptr ||
        g_zClass_ActiveNodeCount != 0) {
        zClass_TypeList::FreeAll();
        return 4;
    }

    zClass_TypeList::FreeAll();

    zClass_NodePartial stackNode = {};
    zClass_SoundDataPartial data = {};
    zSndPlayHandle playHandle = {};
    stackNode.classData = &data;
    data.playHandle = &playHandle;
    data.runtimeFlags = 0x10;
    g_zSnd_IsInitialized = 0;
    if (zClass_Sound::SetSampleSetByName(&stackNode, "short") != 0 ||
        std::strcmp(data.sampleSetName, "short") != 0 || data.sample != nullptr ||
        data.playHandle != nullptr || data.runtimeFlags != 0x11) {
        return 5;
    }

    char longName[0x30] = {};
    std::memset(longName, 'a', sizeof(longName) - 1);
    data.sampleSetName[0x22] = 'Z';
    if (zClass_Sound::SetSampleSetByName(&stackNode, longName) != 0 ||
        std::strncmp(data.sampleSetName, longName, 0x22) != 0 || data.sampleSetName[0x22] != 'Z' ||
        data.sampleSetName[0x23] != '\0') {
        return 6;
    }

    data.runtimeFlags = 0;
    if (zClass_Sound::gwSoundSetPosition(&stackNode, 1.0f, 2.0f, 3.0f) != 0 ||
        data.localPosition.x != 1.0f || data.localPosition.y != 2.0f ||
        data.localPosition.z != 3.0f || data.runtimeFlags != 3) {
        return 61;
    }

    zSndPlayHandle managedHandle = {};
    managedHandle.isActive = 1;
    data.playHandle = &managedHandle;
    data.runtimeFlags = 0x08;
    stackNode.flags = 0x04;
    if (zClass_Sound::gwSoundSetActive(&stackNode, 0) != 0 || data.playHandle != nullptr ||
        (data.runtimeFlags & 0x08) != 0 || managedHandle.isActive != 0 ||
        (stackNode.flags & 0x04) != 0) {
        return 62;
    }
    if (zClass_Sound::gwSoundSetActive(&stackNode, 1) != 0 || (stackNode.flags & 0x04) == 0) {
        return 63;
    }
    stackNode.classId = 10;
    stackNode.flags = 0x04;
    if (zClass_Class::gwNodeSetActive(&stackNode, 0) != 0 || (stackNode.flags & 0x04) != 0) {
        return 64;
    }

    stackNode.classData = nullptr;
    if (zClass_Sound::SetSampleSetByName(nullptr, "x") != 5 ||
        zClass_Sound::SetSampleSetByName(&stackNode, "x") != 5 ||
        zClass_Sound::gwSoundSetPosition(nullptr, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Sound::gwSoundSetPosition(&stackNode, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Sound::gwSoundSetActive(nullptr, 0) != 5 ||
        zClass_Sound::gwSoundSetActive(&stackNode, 0) != 5) {
        return 7;
    }

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Sound::gwSoundNew() == nullptr ? 0 : 8;
}

extern "C" int zclass_find_node_recursive_by_name_smoke() {
    zClass_NodePartial root{};
    zClass_NodePartial firstChild{};
    zClass_NodePartial secondChild{};
    zClass_NodePartial grandchild{};
    std::strcpy(root.name, "root");
    std::strcpy(firstChild.name, "shared");
    std::strcpy(secondChild.name, "shared");
    std::strcpy(grandchild.name, "deep");

    zClass_NodePartial *rootChildren[2] = {&firstChild, &secondChild};
    zClass_NodePartial *childChildren[1] = {&grandchild};
    root.listCountB = 2;
    root.listB = rootChildren;
    firstChild.listCountB = 1;
    firstChild.listB = childChildren;

    if (zClass_Class::FindNodeRecursiveByName(nullptr, "root") != nullptr) {
        return 1;
    }
    if (zClass_Class::FindNodeRecursiveByName(&root, "root") != &root) {
        return 2;
    }
    if (zClass_Class::FindNodeRecursiveByName(&root, "shared") != &firstChild) {
        return 3;
    }
    if (zClass_Class::FindNodeRecursiveByName(&root, "deep") != &grandchild) {
        return 4;
    }

    return zClass_Class::FindNodeRecursiveByName(&root, "missing") == nullptr ? 0 : 5;
}

extern "C" int zclass_object3d_init_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Object3D::gwObject3DInit();
    if (node != &slot.node || node->classId != 5 || node->classData == nullptr ||
        g_zClass_NodeFreeHeadIndex != -1 || g_zClass_ActiveNodeCount != 1) {
        return 1;
    }

    zClass_Object3DDataPartial *data = static_cast<zClass_Object3DDataPartial *>(node->classData);
    if (data->flags != 0x29 || data->scale.x != 1.0f || data->scale.y != 1.0f ||
        data->scale.z != 1.0f || data->localMatrix[0] != 1.0f || data->localMatrix[4] != 1.0f ||
        data->localMatrix[8] != 1.0f || (node->boundsFlags & 4) == 0 ||
        (node->flags & 0x02000003) != 0x02000003) {
        return 2;
    }

    zClass_Object3D::DeleteNode(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Object3D::gwObject3DInit() == nullptr ? 0 : 3;
}

extern "C" int zclass_window_new_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x76543210);
    zRndr::g_activeRegionWidth = 512;
    zRndr::g_activeRegionHeight = 384;
    zRndr::g_pitchBytes = 2048;
    zRndr::g_bytesPerPixel = 2;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Window::gwWindowNew();
    if (node != &slot.node || node->classId != 3 || node->classData == nullptr ||
        zClass_TypeList::Head(14) == nullptr || zClass_TypeList::Head(14)->node != node) {
        return 1;
    }

    zClass_WindowDataPartial *data = static_cast<zClass_WindowDataPartial *>(node->classData);
    if (data->viewportWidth != 0 || data->viewportHeight != 0 || data->resolutionWidth != 1 ||
        data->resolutionHeight != 1 || data->bufferIndex != -1 ||
        data->buffer != reinterpret_cast<void *>(0x76543210) || data->fbWidth != 512 ||
        data->fbHeight != 384 || data->fbBpp != 16) {
        return 2;
    }

    if (zClass_Window::gwWindowSetResolution(node, 320, 240) != 0) {
        return 3;
    }

    std::int32_t width = 0;
    std::int32_t height = 0;
    if (zClass_Window::gwWindowGetResolution(node, &width, &height) != 0 || width != 320 ||
        height != 240) {
        return 4;
    }

    if (zClass_Window::gwWindowSetSize(node, 123, 45) != 0 ||
        zClass_Window::gwWindowGetSize(node, &width, &height) != 0 || width != 123 ||
        height != 45) {
        return 5;
    }

    if (zClass_Window::gwWindowSetBuffer(node, 7) != 0 || data->bufferIndex != 7) {
        return 6;
    }

    data->clearPolyIndexFlags = 3;
    if (zClass_Window::gwWindowSetClearPolygon(node, 1) != 0 ||
        data->clearPolyIndexFlags != static_cast<std::int32_t>(0x80000003u)) {
        return 7;
    }
    if (zClass_Window::gwWindowSetClearPolygon(node, 2) != 0 || data->clearPolyIndexFlags != 3) {
        return 8;
    }

    data->clearPolyIndexFlags = 0;
    zVec3 point{9.0f, 8.0f, 7.0f};
    if (zClass_Window::gwWindowAddClearPolygonVertex(node, &point) != 0 ||
        data->clearPolys[0].vertCount != static_cast<std::int32_t>(0x80000001u) ||
        data->clearPolys[0].vertices[0].x != 9.0f || data->clearPolys[0].vertices[0].y != 8.0f ||
        data->clearPolys[0].vertices[0].z != 100.0f) {
        return 9;
    }

    zRndr::g_spanOccluderPolyCount = 0;
    for (int slotIndex = 0; slotIndex < 8; ++slotIndex) {
        zRndr::g_spanOccluderPolys[slotIndex] = {};
    }
    if (zClass_Window::gwWindowCloseClearPolygon(node) != 0 ||
        data->clearPolyIndexFlags != static_cast<std::int32_t>(0x80000001u) ||
        zRndr::g_spanOccluderPolyCount != 1 || zRndr::g_spanOccluderPolys[0].vertCount != 1 ||
        zRndr::g_spanOccluderPolys[0].vertices[0][0] != 9.0f ||
        zRndr::g_spanOccluderPolys[0].vertices[0][1] != 8.0f ||
        zRndr::g_spanOccluderPolys[0].vertices[0][2] != 100.0f) {
        return 10;
    }

    data->clearPolys[0].vertCount = 4;
    data->clearPolyIndexFlags = 0;
    if (zClass_Window::gwWindowAddClearPolygonVertex(node, &point) != 1) {
        return 11;
    }
    data->clearPolyIndexFlags = 4;
    if (zClass_Window::gwWindowAddClearPolygonVertex(node, &point) != 1) {
        return 12;
    }
    if (zClass_Window::gwWindowCloseClearPolygon(node) != 1) {
        return 13;
    }

    zClass_NodePartial wrongClass{};
    wrongClass.classId = 4;
    wrongClass.classData = data;
    zClass_NodePartial nullData{};
    nullData.classId = 3;
    if (zClass_Window::gwWindowSetResolution(nullptr, 1, 2) != 5 ||
        zClass_Window::gwWindowSetResolution(&nullData, 1, 2) != 5 ||
        zClass_Window::gwWindowSetResolution(&wrongClass, 1, 2) != 3) {
        return 14;
    }

    if (zClass_Window::gwWindowSetSize(nullptr, 1, 2) != 5 ||
        zClass_Window::gwWindowSetSize(&nullData, 1, 2) != 5 ||
        zClass_Window::gwWindowSetSize(&wrongClass, 1, 2) != 3) {
        return 15;
    }

    if (zClass_Window::gwWindowGetResolution(nullptr, &width, &height) != 5 ||
        zClass_Window::gwWindowGetResolution(&nullData, &width, &height) != 5 ||
        zClass_Window::gwWindowGetResolution(&wrongClass, &width, &height) != 3) {
        return 16;
    }

    if (zClass_Window::gwWindowGetSize(nullptr, &width, &height) != 5 ||
        zClass_Window::gwWindowGetSize(&nullData, &width, &height) != 5 ||
        zClass_Window::gwWindowGetSize(&wrongClass, &width, &height) != 3) {
        return 17;
    }

    if (zClass_Window::gwWindowSetBuffer(nullptr, 1) != 5 ||
        zClass_Window::gwWindowSetBuffer(&nullData, 1) != 5 ||
        zClass_Window::gwWindowSetBuffer(&wrongClass, 1) != 3) {
        return 18;
    }

    if (zClass_Window::gwWindowSetClearPolygon(nullptr, 1) != 5 ||
        zClass_Window::gwWindowSetClearPolygon(&nullData, 1) != 5 ||
        zClass_Window::gwWindowSetClearPolygon(&wrongClass, 1) != 3) {
        return 19;
    }

    if (zClass_Window::gwWindowAddClearPolygonVertex(nullptr, &point) != 5 ||
        zClass_Window::gwWindowAddClearPolygonVertex(&nullData, &point) != 5 ||
        zClass_Window::gwWindowAddClearPolygonVertex(&wrongClass, &point) != 3) {
        return 20;
    }

    if (zClass_Window::gwWindowCloseClearPolygon(nullptr) != 5 ||
        zClass_Window::gwWindowCloseClearPolygon(&nullData) != 5 ||
        zClass_Window::gwWindowCloseClearPolygon(&wrongClass) != 3) {
        return 21;
    }

    zClass_Object3D::DeleteNode(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Window::gwWindowNew() == nullptr ? 0 : 22;
}

extern "C" int zclass_display_init_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Display::gwDisplayInit();
    if (node != &slot.node || node->classId != 4 || node->classData == nullptr ||
        zClass_TypeList::Head(15) == nullptr || zClass_TypeList::Head(15)->node != node) {
        return 1;
    }

    zClass_DisplayDataPartial *data = static_cast<zClass_DisplayDataPartial *>(node->classData);
    if (data->x != 0 || data->y != 0 || data->width != 1 || data->height != 1 ||
        data->backgroundR != 0.392f || data->backgroundG != 0.392f || data->backgroundB != 1.0f) {
        return 2;
    }

    zClass_NodePartial removeParent{};
    zClass_NodePartial child{};
    zClass_NodePartial *children[] = {&child};
    zClass_NodePartial *parents[] = {&removeParent};
    removeParent.flags = 1;
    removeParent.listCountB = 1;
    removeParent.listB = children;
    child.listCountA = 1;
    child.listA = parents;
    if (zClass_Display::RemoveChild(&removeParent, &child) != 0 || removeParent.listCountB != 0 ||
        child.listCountA != 0 || zClass_Display::RemoveChild(nullptr, &child) != 5 ||
        zClass_Display::RemoveChild(&removeParent, nullptr) != 5) {
        return 3;
    }

    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;
    g_zVideo_ClearColorPacked16 = 0;
    if (zClass_Display::gwDisplaySetBackgroundColor(node, 255.0f, 128.0f, 32.0f) != 0 ||
        data->backgroundR != 255.0f || data->backgroundG != 128.0f || data->backgroundB != 32.0f ||
        g_zVideo_ClearColorPacked16 != 0xfc04) {
        return 4;
    }

    if (zClass_Display::gwDisplaySetSize(node, 640, 480) != 0 || data->width != 640 ||
        data->height != 480) {
        return 5;
    }

    if (zClass_Display::gwDisplaySetPosition(node, 12, 34) != 0 || data->x != 12 ||
        data->y != 34) {
        return 6;
    }

    zClass_NodePartial nullData{};
    nullData.classId = 4;
    zClass_NodePartial wrongClass{};
    wrongClass.classId = 3;
    wrongClass.classData = data;
    if (zClass_Display::gwDisplaySetBackgroundColor(nullptr, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Display::gwDisplaySetBackgroundColor(&nullData, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Display::gwDisplaySetBackgroundColor(&wrongClass, 1.0f, 2.0f, 3.0f) != 3 ||
        zClass_Display::gwDisplaySetSize(nullptr, 1, 2) != 5 ||
        zClass_Display::gwDisplaySetSize(&nullData, 1, 2) != 5 ||
        zClass_Display::gwDisplaySetSize(&wrongClass, 1, 2) != 3 ||
        zClass_Display::gwDisplaySetPosition(nullptr, 1, 2) != 5 ||
        zClass_Display::gwDisplaySetPosition(&nullData, 1, 2) != 5 ||
        zClass_Display::gwDisplaySetPosition(&wrongClass, 1, 2) != 3) {
        return 7;
    }

    zClass_Object3D::DeleteNode(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Display::gwDisplayInit() == nullptr ? 0 : 8;
}

extern "C" int zclass_lod_leaf_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Lod::gwLodNew();
    if (node != &slot.node || node->classId != 6 || node->classData == nullptr ||
        g_zClass_ActiveNodeCount != 1) {
        return 1;
    }

    zClass_LodDataPartial *data = static_cast<zClass_LodDataPartial *>(node->classData);
    if (data->computeOwnDistance != 1 || data->nearRange != 1000.0f ||
        data->farRangeSq != 1000000.0f || data->active != 1) {
        return 2;
    }

    zClass_NodePartial child{};
    if (zClass_Lod::SetComputeOwnDistance(node, 0) != 0 || data->computeOwnDistance != 0 ||
        zClass_Lod::SetTargetNodeAndRange(node, &child, 3.0f) != 0 || data->rangeNode != &child ||
        data->rangeSq != 9.0f) {
        return 3;
    }
    if (zClass_Lod::SetTargetNodeAndRange(node, nullptr, 4.0f) != 0 || data->rangeNode != nullptr ||
        data->rangeSq != 9.0f) {
        return 4;
    }

    if (zClass_Lod::gwLodAddChild(node, &child) != 0 || node->listCountB != 1 ||
        child.listCountA != 1 || zClass_Lod::RemoveChild(node, &child) != 0 ||
        node->listCountB != 0 || child.listCountA != 0) {
        return 5;
    }

    zClass_Class::TryFreeNode(node);
    zClass_TypeList::FreeAll();
    zClass_TypeList::FreeAll();
    return g_zClass_ActiveNodeCount == 0 ? 0 : 6;
}

extern "C" int zclass_light_new_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Light::gwLightNew();
    if (node != &slot.node || node->classId != 9 || (node->flags & 0x104) != 0x104 ||
        node->classData == nullptr || zClass_TypeList::Head(9) == nullptr ||
        zClass_TypeList::Head(9)->node != node) {
        return 1;
    }

    if (node->cachedBounds[0] != 1.0f || node->cachedBounds[1] != 1.0f ||
        node->cachedBounds[2] != -2.0f || node->cachedBounds[3] != 2.0f ||
        node->cachedBounds[4] != 2.0f || node->cachedBounds[5] != -1.0f) {
        return 2;
    }

    zClass_LightDataPartial *data = static_cast<zClass_LightDataPartial *>(node->classData);
    if (data->dirty != 1 || data->enabled != 1 || data->worldDir.x != 0.0f ||
        data->worldDir.y != 1.0f || data->worldDir.z != 0.0f || data->specularColor.red != 1.0f ||
        data->specularColor.green != 1.0f || data->specularColor.blue != 1.0f ||
        data->falloff != 0.0f || data->intensityScale != 1.0f || data->coneAngle != 0.0f ||
        data->isPointMode != 0 || data->isDirectionalMode != 1 || data->lightParam != 1 ||
        data->lightSubMode != 1 || data->range1 != 32.0f || data->range2 != 64.0f ||
        data->range2Sq != 4096.0f || data->invRangeDelta != 0.03125f ||
        data->attachedWorldCount != 0 || data->attachedWorlds != nullptr) {
        return 3;
    }

    zClass_NodePartial removeParent{};
    zClass_NodePartial child{};
    zClass_NodePartial *children[] = {&child};
    zClass_NodePartial *parents[] = {&removeParent};
    removeParent.flags = 1;
    removeParent.listCountB = 1;
    removeParent.listB = children;
    child.listCountA = 1;
    child.listA = parents;
    if (zClass_Light::RemoveChild(&removeParent, &child) != 0 || removeParent.listCountB != 0 ||
        child.listCountA != 0 || zClass_Light::RemoveChild(nullptr, &child) != 5 ||
        zClass_Light::RemoveChild(&removeParent, nullptr) != 5) {
        return 4;
    }

    data->dirty = 0;
    if (zClass_Light::gwLightSetIntensity(node, 0.25f) != 0 || data->dirty != 1 ||
        data->intensityScale != 0.25f) {
        return 5;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetFalloff(node, 0.75f) != 0 || data->dirty != 1 ||
        data->falloff != 0.75f) {
        return 6;
    }

    zClass_NodePartial nullLightData{};
    float coneAngle = 0.5f;
    std::uint32_t coneAngleBits = 0;
    std::memcpy(&coneAngleBits, &coneAngle, sizeof(coneAngleBits));
    data->dirty = 0;
    if (zClass_Light::gwLightSetConeAngle(node, coneAngleBits) != 0 || data->dirty != 1 ||
        data->coneAngle != 0.5f) {
        return 7;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetPointMode(node) != 0 || data->dirty != 1 ||
        data->isPointMode != 1 || data->isDirectionalMode != 0) {
        return 8;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetDirectionalMode(node) != 0 || data->dirty != 1 ||
        data->isPointMode != 0 || data->isDirectionalMode != 1) {
        return 9;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetParam(node, 7) != 0 || data->dirty != 1 || data->lightParam != 7) {
        return 10;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetRange(node, 20.0f, 10.0f) != 0 || data->dirty != 1 ||
        data->range1 != 10.0f || data->range2 != 20.0f || data->range2Sq != 400.0f ||
        data->invRangeDelta != 0.1f) {
        return 11;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetRange(node, 5.0f, 5.0f) != 0 || data->dirty != 1 ||
        data->range1 != 5.0f || data->range2 != 15.0f || data->range2Sq != 225.0f ||
        data->invRangeDelta != 0.1f ||
        std::strcmp(g_zError_DebugMsgBuffer,
                    "D:\\Proj\\GameZRecoil\\zClass\\Light.c: Line 540: ERROR setting light ranges; "
                    "Range2 can't be equal to Range1.\n") != 0) {
        return 12;
    }
    float rangeA = 0.0f;
    float rangeB = 0.0f;
    if (zClass_Light::gwLightGetRange(node, &rangeA, &rangeB) != 0 || rangeA != 5.0f ||
        rangeB != 15.0f) {
        return 13;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetPosition(node, 1.0f, 2.0f, 3.0f) != 0 || data->dirty != 1 ||
        data->localPosition.x != 1.0f || data->localPosition.y != 2.0f ||
        data->localPosition.z != 3.0f) {
        return 14;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetRotation(node, 4.0f, 5.0f, 6.0f) != 0 || data->dirty != 1 ||
        data->localRotation.x != 4.0f || data->localRotation.y != 5.0f ||
        data->localRotation.z != 6.0f) {
        return 15;
    }
    data->localRotation = {0.0f, 0.0f, 0.0f};
    data->localPosition = {1.0f, 2.0f, 3.0f};
    data->isPointMode = 0;
    data->coneAngle = 0.0f;
    if (zClass_Light::ComputeWorldTransform(node, data) != 0 || data->worldPosition.x != 1.0f ||
        data->worldPosition.y != 2.0f || data->worldPosition.z != 3.0f ||
        data->worldDir.x != 0.0f || data->worldDir.y != 0.0f || data->worldDir.z != -1.0f) {
        return 151;
    }
    data->localPosition = {0.0f, 0.0f, 0.0f};
    data->isPointMode = 1;
    if (zClass_Light::ComputeWorldTransform(node, data) != 0 || data->worldRotation.x != 0.0f ||
        data->worldRotation.y != 0.0f || data->worldRotation.z != 0.0f) {
        return 152;
    }
    data->specularColor = {0.2f, 0.4f, 0.6f};
    float specR = 0.0f;
    float specG = 0.0f;
    float specB = 0.0f;
    if (zClass_Light::gwLightGetSpecularColor(node, &specR, &specG, &specB) != 0 || specR != 0.2f ||
        specG != 0.4f || specB != 0.6f) {
        return 16;
    }
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_fogTargetParamsStaged = {};
    g_zVideo_RendererType = 1;
    g_zVideo_D3DColorNormalizeChannelIndex = -1;
    data->dirty = 0;
    if (zClass_Light::gwLightSetSpecularColor(node, 1.2f, -0.5f, 0.5f) != 0 || data->dirty != 1 ||
        data->specularColor.red != 1.0f || data->specularColor.green != 0.0f ||
        data->specularColor.blue != 0.5f ||
        zRndr::g_fogTargetParamsStaged.packedColor16 != 0xf810 ||
        g_zVideo_D3DColorNormalizeChannelIndex != 0) {
        return 17;
    }
    if (zClass_Light::gwLightSetIntensity(nullptr, 1.0f) != 5 ||
        zClass_Light::gwLightSetIntensity(&nullLightData, 1.0f) != 5 ||
        zClass_Light::gwLightSetFalloff(nullptr, 1.0f) != 5 ||
        zClass_Light::gwLightSetFalloff(&nullLightData, 1.0f) != 5 ||
        zClass_Light::gwLightSetConeAngle(nullptr, coneAngleBits) != 5 ||
        zClass_Light::gwLightSetConeAngle(&nullLightData, coneAngleBits) != 5 ||
        zClass_Light::gwLightSetPointMode(nullptr) != 5 ||
        zClass_Light::gwLightSetPointMode(&nullLightData) != 5 ||
        zClass_Light::gwLightSetDirectionalMode(nullptr) != 5 ||
        zClass_Light::gwLightSetDirectionalMode(&nullLightData) != 5 ||
        zClass_Light::gwLightSetParam(nullptr, 1) != 5 ||
        zClass_Light::gwLightSetParam(&nullLightData, 1) != 5 ||
        zClass_Light::gwLightSetRange(nullptr, 1.0f, 2.0f) != 5 ||
        zClass_Light::gwLightSetRange(&nullLightData, 1.0f, 2.0f) != 5 ||
        zClass_Light::gwLightGetRange(nullptr, &rangeA, &rangeB) != 5 ||
        zClass_Light::gwLightGetRange(&nullLightData, &rangeA, &rangeB) != 5 ||
        zClass_Light::gwLightSetPosition(nullptr, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Light::gwLightSetPosition(&nullLightData, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Light::gwLightSetRotation(nullptr, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Light::gwLightSetRotation(&nullLightData, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Light::gwLightGetSpecularColor(nullptr, &specR, &specG, &specB) != 5 ||
        zClass_Light::gwLightGetSpecularColor(&nullLightData, &specR, &specG, &specB) != 5 ||
        zClass_Light::gwLightSetSpecularColor(nullptr, 1.0f, 1.0f, 1.0f) != 5 ||
        zClass_Light::gwLightSetSpecularColor(&nullLightData, 1.0f, 1.0f, 1.0f) != 5) {
        return 18;
    }

    if (zClass_Light::DeleteNode(nullptr) != 5) {
        return 19;
    }

    zClass_NodePartial nullData{};
    if (zClass_Light::DeleteNode(&nullData) != 5) {
        return 20;
    }

    data->attachedWorldCount = 2;
    if (zClass_Light::DeleteNode(node) != 1 || node->classData == nullptr) {
        return 21;
    }

    data->attachedWorldCount = 0;
    data->attachedWorlds =
        static_cast<zClass_NodePartial **>(std::calloc(1, sizeof(zClass_NodePartial *)));
    if (data->attachedWorlds == nullptr) {
        return 22;
    }

    if (zClass_Light::DeleteNode(node) != 0 || g_zClass_ActiveNodeCount != 0) {
        return 23;
    }
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Light::gwLightNew() == nullptr ? 0 : 24;
}

extern "C" int zclass_damage_handler_smoke() {
    zClass_NodeFreeListSlot rootSlot{};
    zClass_NodeFreeListSlot childSlot{};
    zClass_NodeFreeListSlot grandchildSlot{};
    zClass_NodePartial *rootChildren[] = {&childSlot.node};
    zClass_NodePartial *childChildren[] = {&grandchildSlot.node};
    rootSlot.node.listCountB = 1;
    rootSlot.node.listB = rootChildren;
    childSlot.node.listCountB = 1;
    childSlot.node.listB = childChildren;

    void *hitCallback = reinterpret_cast<void *>(0x11111111);
    void *hitContext = reinterpret_cast<void *>(0x22222222);
    if (zClass_Node::SetDamageHitCallback(hitCallback, &rootSlot.node, hitContext) != 0 ||
        rootSlot.damageHandler == nullptr || childSlot.damageHandler != rootSlot.damageHandler ||
        grandchildSlot.damageHandler != rootSlot.damageHandler ||
        (rootSlot.node.flags & 0x40) == 0) {
        return 1;
    }

    OptCatalogDamageHandlerPartial *handler =
        static_cast<OptCatalogDamageHandlerPartial *>(rootSlot.damageHandler);
    if (handler->hitCallback != hitCallback || handler->hitContext != hitContext) {
        return 2;
    }

    void *replacement = reinterpret_cast<void *>(0x33333333);
    zClass_Node::SetDamageHitCallback(replacement, &rootSlot.node, replacement);
    if (handler->hitCallback != hitCallback || handler->hitContext != hitContext) {
        return 3;
    }

    zClass_Node::ClearDamageHandlerRecursive(&rootSlot.node, handler);
    if (rootSlot.damageHandler != nullptr || childSlot.damageHandler != nullptr ||
        grandchildSlot.damageHandler != nullptr) {
        std::free(handler);
        return 4;
    }
    std::free(handler);
    rootSlot.node.flags = 0;

    float capturedDamage = 0.0f;
    void *timerCallback = reinterpret_cast<void *>(zclass_damage_timer_stub);
    void *timerContext = &capturedDamage;
    if (zClass_Node::SetDamageTimerCallback(timerCallback, &rootSlot.node, timerContext) != 0 ||
        rootSlot.damageHandler == nullptr || childSlot.damageHandler != rootSlot.damageHandler) {
        return 5;
    }
    handler = static_cast<OptCatalogDamageHandlerPartial *>(rootSlot.damageHandler);
    if (handler->timerCallback != timerCallback || handler->timerContext != timerContext) {
        return 6;
    }

    g_OptCatalog_CaptureHitSnapshotEnabled = 1;
    zVec3 sourcePos{1.0f, 2.0f, 3.0f};
    OptCatalogHitEventPartial hitEvent{};
    hitEvent.hitPos = {4.0f, 5.0f, 6.0f};
    hitEvent.hitNode = &rootSlot.node;
    const float timerResult =
        OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback(&sourcePos, &hitEvent, 2.5f);
    if (timerResult != 3.5f || capturedDamage != 2.5f ||
        g_OptCatalog_CapturedDamageSourcePos.x != 1.0f ||
        g_OptCatalog_CapturedDamageSourcePos.y != 2.0f ||
        g_OptCatalog_CapturedDamageSourcePos.z != 3.0f ||
        g_OptCatalog_CapturedDamageHitPos.x != 4.0f ||
        g_OptCatalog_CapturedDamageHitPos.y != 5.0f ||
        g_OptCatalog_CapturedDamageHitPos.z != 6.0f) {
        return 7;
    }
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;

    if (zClass_Node::ClearDamageHandler(&rootSlot.node) != 0 || rootSlot.damageHandler != nullptr ||
        childSlot.damageHandler != nullptr || (rootSlot.node.flags & 0x40) != 0) {
        return 8;
    }

    return zClass_Node::ClearDamageHandler(nullptr) == 0 ? 0 : 9;
}

extern "C" int zclass_camera_view_distance_smoke() {
    if (g_zClass_CameraAutoClipDistanceThreshold != 0.04f || g_zClass_ObjectHseTestEnabled != 1) {
        return 1;
    }

    zClass_Camera::SetViewDistance(1, 20.0f);
    if (g_zClass_CameraAutoClipDistanceAdjustEnabled != 1 ||
        g_zClass_CameraAutoClipDistanceThreshold != 0.05f) {
        return 2;
    }

    zClass_Camera::SetViewDistance(0, 0.0f);
    if (g_zClass_CameraAutoClipDistanceAdjustEnabled != 0 ||
        g_zClass_CameraAutoClipDistanceThreshold != 0.04f) {
        return 3;
    }

    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *camera = zClass_Camera::gwCameraNew();
    if (camera != &slot.node || camera->classId != 1 || camera->classData == nullptr ||
        zClass_TypeList::Head(8) == nullptr || zClass_TypeList::Head(8)->node != camera) {
        return 4;
    }

    zClass_CameraDataPartial *data = static_cast<zClass_CameraDataPartial *>(camera->classData);
    if (data->viewportWidth != 1.0f || data->viewportHeight != 1.0f ||
        data->frustumVectorsDirty != 1 || data->transformDirty != 1 ||
        data->localFrustumNormalsDirty != 1 || data->variantOverrideEnabled != 0 ||
        data->variantTag.count != 0 || data->variantTag.tags[0] != 0xff ||
        data->variantTag.tags[1] != 0xff || data->variantTag.tags[2] != 0xff) {
        return 5;
    }

    if (zClass_Camera::gwCameraSetFlagBit0(camera, 1) != 0 || (data->cameraFlags & 1) == 0 ||
        zClass_Camera::gwCameraSetFlagBit0(camera, 0) != 0 || (data->cameraFlags & 1) != 0) {
        return 6;
    }

    zClass_NodePartial child{};
    if (zClass_Camera::gwCameraAddChild(camera, &child) != 0 || camera->listCountB != 1 ||
        child.listCountA != 1 || zClass_Camera::gwCameraRemoveChild(camera, &child) != 0 ||
        camera->listCountB != 0 || child.listCountA != 0) {
        return 7;
    }

    zClass_NodePartial world{};
    std::int32_t worldData = 0;
    world.classId = 2;
    world.classData = &worldData;
    if (zClass_Camera::gwCameraSetWorld(camera, &world) != 0 || data->worldNode != &world ||
        zClass_Camera::gwCameraGetWorld(camera) != &world) {
        return 8;
    }

    if (zClass_Camera::gwCameraSetWindow(camera, &child) != 0 || data->windowNode != &child) {
        return 9;
    }

    data->cameraFlags = 0x02;
    child.classId = 5;
    zClass_Object3DDataPartial childData{};
    child.classData = &childData;
    zClass_NodePartial *cameraChildren[] = {&child};
    camera->listCountA = 1;
    camera->listCountB = 1;
    camera->listB = cameraChildren;
    if (zClass_Camera::gwCameraSetPosition(camera, 1.0f, 2.0f, 3.0f) != 0 ||
        data->posOffset.x != 1.0f || data->posOffset.y != 2.0f || data->posOffset.z != 3.0f ||
        (data->cameraFlags & 0x06) != 0x04 || data->transformDirty != 1 ||
        (camera->flags & 0x03) != 0x03 || (child.flags & 0x02000000) == 0) {
        return 10;
    }

    if (zClass_Camera::gwCameraTranslate(camera, 0.5f, -1.0f, 4.0f) != 0 ||
        data->posOffset.x != 1.5f || data->posOffset.y != 1.0f || data->posOffset.z != 7.0f) {
        return 11;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (zClass_Camera::gwCameraGetPosition(camera, &x, &y, &z) != 0 || x != 1.5f || y != 1.0f ||
        z != 7.0f) {
        return 12;
    }

    data->cameraFlags &= ~0x02;
    if (zClass_Camera::gwCameraSetTarget(camera, 8.0f, 9.0f, 10.0f) != 0 ||
        data->targetOrEuler.x != 8.0f || data->targetOrEuler.y != 9.0f ||
        data->targetOrEuler.z != 10.0f) {
        return 13;
    }
    if (zClass_Camera::gwCameraTranslateTarget(camera, 1.0f, 2.0f, 3.0f) != 0 ||
        data->targetOrEuler.x != 9.0f || data->targetOrEuler.y != 11.0f ||
        data->targetOrEuler.z != 13.0f ||
        zClass_Camera::gwCameraGetTarget(camera, &x, &y, &z) != 0 || x != 9.0f || y != 11.0f ||
        z != 13.0f) {
        return 14;
    }

    data->cameraFlags |= 0x02;
    if (zClass_Camera::gwCameraSetTarget(camera, 1.0f, 2.0f, 3.0f) != 0 ||
        zClass_Camera::gwCameraTranslateTarget(camera, 4.0f, 5.0f, 6.0f) != 0 ||
        zClass_Camera::gwCameraGetTarget(camera, &x, &y, &z) != 0 || x != 5.0f || y != 7.0f ||
        z != 9.0f || data->targetOrEuler.x != 9.0f) {
        return 15;
    }

    if (zClass_Camera::gwCameraSetNearFarClip(camera, 0.25f, 500.0f) != 0 ||
        data->nearClip != 0.25f || data->farClip != 500.0f || data->frustumVectorsDirty != 1 ||
        zClass_Camera::gwCameraGetNearFarClip(camera, &x, &y) != 0 || x != 0.25f || y != 500.0f) {
        return 16;
    }

    const float maxCameraFov = 1.39600003f;
    data->frustumWidth = 640.0f;
    data->frustumHeight = 480.0f;
    if (zClass_Camera::gwCameraSetViewport(camera, 320.0f, 200.0f) != 0 ||
        data->viewportWidth != 320.0f || data->viewportHeight != 200.0f ||
        data->fovX != maxCameraFov || data->fovY != maxCameraFov ||
        data->frustumYaw != maxCameraFov * 0.5f || data->frustumPitch != maxCameraFov * 0.5f ||
        std::fabs(data->viewportScaleX - (1.0f / std::tan(static_cast<double>(data->frustumYaw)))) >
            0.0001f ||
        std::fabs(data->viewportScaleY -
                  (1.0f / std::tan(static_cast<double>(data->frustumPitch)))) > 0.0001f ||
        data->localFrustumNormalsDirty != 1 || data->frustumVectorsDirty != 1) {
        return 17;
    }

    if (zClass_Camera::gwCameraGetViewport(camera, &x, &y) != 0 || x != 320.0f || y != 200.0f) {
        return 18;
    }

    if (zClass_Camera::gwCameraGetFOV(camera, &x, &y) != 0 || x != 640.0f || y != 480.0f) {
        return 19;
    }

    if (zClass_Camera::gwCameraSetFOV(camera, 1.0f, 0.5f) != 0 || data->frustumWidth != 1.0f ||
        data->frustumHeight != 0.5f || data->fovX != 1.0f / 320.0f || data->fovY != 0.5f / 200.0f ||
        data->frustumYaw != (1.0f / 320.0f) * 0.5f ||
        data->frustumPitch != (0.5f / 200.0f) * 0.5f ||
        std::fabs(data->viewportScaleX - (1.0f / std::tan(static_cast<double>(data->frustumYaw)))) >
            0.0001f ||
        std::fabs(data->viewportScaleY -
                  (1.0f / std::tan(static_cast<double>(data->frustumPitch)))) > 0.0001f ||
        data->localFrustumNormalsDirty != 1 || data->frustumVectorsDirty != 1) {
        return 20;
    }

    if (zClass_Camera::gwCameraSetClipDistance(camera, 4.0f) != 0 || data->clipDistance != 4.0f ||
        data->invClipDistanceSq != 0.0625f ||
        zClass_Camera::gwCameraGetClipDistance(camera, &x) != 0 || x != 4.0f) {
        return 21;
    }

    if (zClass_Camera::gwCameraSetHorizon(camera, &child) != 0 || data->horizonNode != &child ||
        zClass_Camera::gwCameraSetHorizonXZ(camera, &world) != 0 || data->horizonXZNode != &world) {
        return 22;
    }

    zClass_Camera::SetTargetNode(&child);
    if (g_zClass_CameraTargetNode != &child) {
        return 23;
    }
    if (zClass_Camera::SetActiveCamera(camera) != camera || g_zClass_CurrentCamera != camera) {
        return 24;
    }
    if (zClass_Camera::SetObjectHseTestEnabled(0) != 0 || g_zClass_ObjectHseTestEnabled != 0) {
        return 23;
    }

    zTag4Partial tag{};
    zTag4::Clear(nullptr);
    zTag4::Clear(&tag);
    if (tag.count != 0 || tag.tags[0] != 0xff || tag.tags[1] != 0xff || tag.tags[2] != 0xff) {
        return 24;
    }

    tag.count = 2;
    tag.tags[0] = 11;
    tag.tags[1] = 12;
    tag.tags[2] = 0xff;
    data->variantOverrideEnabled = 0;
    if (zClass_Camera::gwCameraSetVariantTagOverride(camera, &tag) != 0 ||
        data->variantOverrideEnabled != 1 || data->variantTag.count != 2 ||
        data->variantTag.tags[0] != 11 || data->variantTag.tags[1] != 12 ||
        data->variantTag.tags[2] != 0xff) {
        return 25;
    }

    tag.count = 2;
    tag.tags[0] = 13;
    tag.tags[1] = 0xff;
    tag.tags[2] = 14;
    data->variantOverrideEnabled = 0;
    if (zClass_Camera::gwCameraSetVariantTagOverride(camera, &tag) != 0 ||
        data->variantOverrideEnabled != 0) {
        return 26;
    }

    tag.count = 0;
    tag.tags[0] = 15;
    if (zClass_Camera::gwCameraSetVariantTagOverride(camera, &tag) != 0 ||
        data->variantOverrideEnabled != 0) {
        return 27;
    }

    zClass_Object3D::DeleteNode(camera);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Camera::gwCameraNew() == nullptr &&
                   zClass_Camera::gwCameraAddChild(nullptr, &child) == 5 &&
                   zClass_Camera::gwCameraRemoveChild(camera, nullptr) == 5 &&
                   zClass_Camera::gwCameraSetFlagBit0(nullptr, 1) == 5 &&
                   zClass_Camera::gwCameraSetFlagBit0(&world, 1) == 3 &&
                   zClass_Camera::gwCameraSetWorld(camera, nullptr) == 5 &&
                   zClass_Camera::gwCameraSetPosition(nullptr, 0.0f, 0.0f, 0.0f) == 5 &&
                   zClass_Camera::gwCameraTranslate(&world, 0.0f, 0.0f, 0.0f) == 3 &&
                   zClass_Camera::gwCameraGetPosition(nullptr, &x, &y, &z) == 5 &&
                   zClass_Camera::gwCameraSetTarget(nullptr, 0.0f, 0.0f, 0.0f) == 5 &&
                   zClass_Camera::gwCameraTranslateTarget(&world, 0.0f, 0.0f, 0.0f) == 3 &&
                   zClass_Camera::gwCameraGetTarget(nullptr, &x, &y, &z) == 5 &&
                   zClass_Camera::gwCameraSetNearFarClip(nullptr, 0.0f, 0.0f) == 5 &&
                   zClass_Camera::gwCameraGetNearFarClip(nullptr, &x, &y) == 5 &&
                   zClass_Camera::gwCameraSetViewport(nullptr, 1.0f, 1.0f) == 5 &&
                   zClass_Camera::gwCameraGetViewport(nullptr, &x, &y) == 5 &&
                   zClass_Camera::gwCameraGetFOV(nullptr, &x, &y) == 5 &&
                   zClass_Camera::gwCameraSetFOV(nullptr, 1.0f, 1.0f) == 5 &&
                   zClass_Camera::gwCameraGetClipDistance(nullptr, &x) == 5 &&
                   zClass_Camera::gwCameraSetClipDistance(nullptr, 1.0f) == 5 &&
                   zClass_Camera::gwCameraSetHorizon(nullptr, &child) == 5 &&
                   zClass_Camera::gwCameraSetHorizonXZ(nullptr, &child) == 5
               ? 0
               : 25;
}
