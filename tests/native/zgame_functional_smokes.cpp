// Focused native smokes used by zGame/zClass/zModel functional manifests.
// Kept separate from the legacy zgame_tests.cpp reservoir so these checks compile
// from a stable, checked-in translation unit.

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zclass.h"
#include "zdi.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
int g_zgameFogColorUpdateCount = 0;
int g_modelRefLerpCallbackCount = 0;
void *g_modelRefLerpLastCallbackCtx = nullptr;
int g_zclassUpdateBucketCallbackCount = 0;
zClass_NodePartial *g_zclassUpdateBucketLastNode = nullptr;
int g_zclassUpdateBucketDeferredDuringCallback = -1;

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

void __fastcall zclass_model_ref_lerp_test_callback(void *) {
}
} // namespace

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

extern "C" int zmodel_render_state_setters_smoke(void) {
    gModel_RenderVertexAlphaEnabled = -1;
    zModel_RenderVertexAlphaEnabled_SetCurrent(3);
    if (gModel_RenderVertexAlphaEnabled != 3) {
        return 1;
    }

    gModel_RenderAlphaScaleCurrent = -1.0f;
    zModel_RenderAlphaScale_SetCurrent(0.375f);
    if (gModel_RenderAlphaScaleCurrent != 0.375f) {
        return 2;
    }

    zColorRgb color{0.25f, 0.5f, 0.75f};
    g_zModel_FogTargetColorOverride = {};
    zModel_FogTargetColorOverride_SetCurrent(&color, 0.625f);
    if (g_zModel_FogTargetColorOverride.colorRgb01.red != 0.25f ||
        g_zModel_FogTargetColorOverride.colorRgb01.green != 0.5f ||
        g_zModel_FogTargetColorOverride.colorRgb01.blue != 0.75f ||
        g_zModel_FogTargetColorOverride.weight != 0.625f) {
        return 3;
    }

    color = {1.0f, 1.0f, 1.0f};
    zModel_FogTargetColorOverride_SetCurrent(nullptr, 0.0f);
    return g_zModel_FogTargetColorOverride.colorRgb01.red == 0.25f &&
                   g_zModel_FogTargetColorOverride.colorRgb01.green == 0.5f &&
                   g_zModel_FogTargetColorOverride.colorRgb01.blue == 0.75f &&
                   g_zModel_FogTargetColorOverride.weight == 0.0f
               ? 0
               : 4;
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

    zDiPartial thresholdBlendDi = {};
    thresholdBlendDi.vertCount = 920;
    thresholdBlendDi.blendVertCount = 920;
    thresholdBlendDi.verts =
        static_cast<zVec3 *>(std::malloc(static_cast<std::size_t>(thresholdBlendDi.vertCount) *
                                         sizeof(zVec3)));
    thresholdBlendDi.blendVerts = static_cast<zVec3 *>(
        std::malloc(static_cast<std::size_t>(thresholdBlendDi.blendVertCount) * sizeof(zVec3)));
    if (thresholdBlendDi.verts == nullptr || thresholdBlendDi.blendVerts == nullptr) {
        std::free(thresholdBlendDi.verts);
        std::free(thresholdBlendDi.blendVerts);
        return 97;
    }
    for (int i = 0; i < thresholdBlendDi.vertCount; ++i) {
        thresholdBlendDi.verts[i].x = 1000.0f + static_cast<float>(i);
        thresholdBlendDi.verts[i].y = 0.0f;
        thresholdBlendDi.verts[i].z = 0.0f;
        thresholdBlendDi.blendVerts[i].x = 0.0f;
        thresholdBlendDi.blendVerts[i].y = 0.0f;
        thresholdBlendDi.blendVerts[i].z = 0.0f;
    }
    mergedIndex = zModel_Const::AddOrMergeVertexAndNormal(&thresholdBlendDi, &mergePoint,
                                                          &mergeNormal);
    const bool thresholdBlendOk =
        mergedIndex == 920 && thresholdBlendDi.vertCount == 921 &&
        thresholdBlendDi.blendVertCount == 921 && thresholdBlendDi.verts != nullptr &&
        thresholdBlendDi.blendVerts != nullptr && thresholdBlendDi.verts[920].x == 1.0f &&
        thresholdBlendDi.verts[920].y == 2.0f && thresholdBlendDi.verts[920].z == 3.0f &&
        thresholdBlendDi.blendVerts[920].x == 1.0f &&
        thresholdBlendDi.blendVerts[920].y == 2.0f &&
        thresholdBlendDi.blendVerts[920].z == 3.0f;
    std::free(thresholdBlendDi.verts);
    std::free(thresholdBlendDi.blendVerts);
    if (!thresholdBlendOk) {
        return 96;
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

    zDiPartial thresholdDi = {};
    thresholdDi.vertCount = 921;
    thresholdDi.verts =
        static_cast<zVec3 *>(std::malloc(static_cast<std::size_t>(thresholdDi.vertCount) *
                                         sizeof(zVec3)));
    if (thresholdDi.verts == nullptr) {
        return 98;
    }
    for (int i = 0; i < thresholdDi.vertCount; ++i) {
        thresholdDi.verts[i].x = 1000.0f + static_cast<float>(i);
        thresholdDi.verts[i].y = 0.0f;
        thresholdDi.verts[i].z = 0.0f;
    }
    zVec3 thresholdPoint = {1.0f, 2.0f, 3.0f};
    vertexIndex = zModel_Const::AddOrMergeVertex(&thresholdDi, &thresholdPoint);
    const bool thresholdOk =
        vertexIndex == 921 && thresholdDi.vertCount == 922 && thresholdDi.verts != nullptr &&
        thresholdDi.verts[921].x == 1.0f && thresholdDi.verts[921].y == 2.0f &&
        thresholdDi.verts[921].z == 3.0f;
    std::free(thresholdDi.verts);
    if (!thresholdOk) {
        return 17;
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

extern "C" int zrndr_global_string_table_release_dynamic_entries_smoke() {
    char *savedTable[100] = {};
    for (int index = 0; index < 100; ++index) {
        savedTable[index] = g_zRndr_GlobalStringTable[index];
    }
    const int savedCount = g_zRndr_GlobalStringCount;

    char *const dynamicA = static_cast<char *>(std::malloc(6));
    char *const dynamicB = static_cast<char *>(std::malloc(6));
    if (dynamicA == nullptr || dynamicB == nullptr) {
        std::free(dynamicA);
        std::free(dynamicB);
        return 1;
    }
    std::memcpy(dynamicA, "dyn-a", 6);
    std::memcpy(dynamicB, "dyn-b", 6);

    g_zRndr_GlobalStringTable[0] = const_cast<char *>("BASE0");
    g_zRndr_GlobalStringTable[1] = const_cast<char *>("BASE1");
    g_zRndr_GlobalStringTable[2] = const_cast<char *>("BASE2");
    g_zRndr_GlobalStringTable[3] = const_cast<char *>("BASE3");
    g_zRndr_GlobalStringTable[4] = const_cast<char *>("BASE4");
    g_zRndr_GlobalStringTable[5] = const_cast<char *>("BASE5");
    g_zRndr_GlobalStringTable[6] = dynamicA;
    g_zRndr_GlobalStringTable[7] = dynamicB;
    g_zRndr_GlobalStringTable[8] = const_cast<char *>("OUTSIDE_COUNT");
    g_zRndr_GlobalStringCount = 8;

    zRndr::GlobalStringTable_ReleaseDynamicEntries();
    const bool dynamicReleaseOk =
        g_zRndr_GlobalStringCount == 6 && g_zRndr_GlobalStringTable[0] != nullptr &&
        g_zRndr_GlobalStringTable[5] != nullptr && g_zRndr_GlobalStringTable[6] == nullptr &&
        g_zRndr_GlobalStringTable[7] == nullptr &&
        std::strcmp(g_zRndr_GlobalStringTable[0], "BASE0") == 0 &&
        std::strcmp(g_zRndr_GlobalStringTable[5], "BASE5") == 0 &&
        std::strcmp(g_zRndr_GlobalStringTable[8], "OUTSIDE_COUNT") == 0;

    char *const preservedDynamic = const_cast<char *>("PRESERVED");
    g_zRndr_GlobalStringTable[6] = preservedDynamic;
    g_zRndr_GlobalStringCount = 4;
    zRndr::GlobalStringTable_ReleaseDynamicEntries();
    const bool lowCountOk =
        g_zRndr_GlobalStringCount == 6 && g_zRndr_GlobalStringTable[6] == preservedDynamic;

    for (int index = 0; index < 100; ++index) {
        g_zRndr_GlobalStringTable[index] = savedTable[index];
    }
    g_zRndr_GlobalStringCount = savedCount;

    if (!dynamicReleaseOk) {
        return 2;
    }
    return lowCountOk ? 0 : 3;
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
    static int graphicsFlags = 1;
    gModel_pGraphicsFlags = &graphicsFlags;
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
    gModel_pGraphicsFlags = &gModel_DefaultGraphicsFlags;

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

extern "C" int zclass_world_build_active_light_list_smoke(void) {
    const int savedRendererPath = g_zVideo_ActiveRendererPath;

    zClass_LightDataPartial light0{};
    zClass_LightDataPartial light1{};
    light1.specularColor = {0.4f, 0.5f, 0.6f};
    light1.intensityScale = 0.375f;
    light1.isPointMode = 1;
    zClass_LightDataPartial *lightDataList[2] = {&light0, &light1};

    zClass_NodePartial lightNode0{};
    zClass_NodePartial lightNode1{};
    lightNode1.flags = 4;
    zClass_NodePartial *lightNodes[2] = {&lightNode0, &lightNode1};

    zClass_WorldDataPartial worldData{};
    worldData.lightCount = 2;
    worldData.lightNodes = lightNodes;
    worldData.lightDataList = lightDataList;

    zClass_NodePartial world{};
    world.classData = &worldData;

    g_zVideo_ActiveRendererPath = 0;
    gModel_FogColorRgb01 = {0.1f, 0.2f, 0.3f};
    gModel_SpecialLightPaletteRemapRecipe = {};

    const int result = zClass_World::InitLightPointInPolygonXZ(&world);
    const bool ok =
        result == 0 && gModel_LightInputDataList == lightDataList &&
        gModel_LightInputNodeStates == reinterpret_cast<zModel_LightStatePartial **>(lightNodes) &&
        gModel_LightInputCount == 2 && gModel_ActiveLightCount == 1 &&
        gModel_HasActiveLights == 1 && gModel_ActiveLightSpecialIndex == 0 &&
        gModel_ActiveLights[0].light == &light1 &&
        gModel_ActiveLights[0].lightState ==
            reinterpret_cast<zModel_LightStatePartial *>(&lightNode1) &&
        gModel_AmbientColorRgb01.red == 0.4f &&
        gModel_AmbientIntensityFactor == 0.625f &&
        gModel_SpecialLightPaletteRemapRecipe.color1Strength == 1.0f;

    g_zVideo_ActiveRendererPath = savedRendererPath;
    return ok ? 0 : 1;
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

extern "C" int zclass_model_ref_lerp_queue_clear_global_state_smoke() {
    zClass_Object3D_ModelRefLerpTask first{};
    zClass_Object3D_ModelRefLerpTask second{};
    first.next = &second;
    second.next = nullptr;

    g_ModelRefLerpQueueState.listAux = 0x11;
    g_ModelRefLerpQueueState.head = &first;
    g_ModelRefLerpQueueState.tail = &second;
    g_ModelRefLerpQueueState.count = 2;

    zClass_Object3D_ModelRefLerpQueue::ClearGlobalState();

    return g_ModelRefLerpQueueState.listAux == 0 && g_ModelRefLerpQueueState.head == nullptr &&
                   g_ModelRefLerpQueueState.tail == nullptr && g_ModelRefLerpQueueState.count == 0 &&
                   first.next == &second && second.next == nullptr
               ? 0
               : 1;
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

    zClass_NodePartial updateWorld{};
    zClass_WorldDataPartial updateWorldData{};
    int updateLightMatrixIdentityFlags[4] = {};
    float *updateLightMatrixSlots[4] = {};
    zMat4x3 updateLightBaseMatrix{};
    updateLightMatrixIdentityFlags[0] = 1;
    updateLightMatrixSlots[0] = reinterpret_cast<float *>(&updateLightBaseMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &updateLightMatrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &updateLightMatrixSlots[0];
    zMath::g_zMath_CameraScratchB = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 5.0f, 6.0f, 7.0f};
    zClass_NodePartial updateLightA{};
    zClass_NodePartial updateLightB{};
    zClass_LightDataPartial updateLightDataA{};
    zClass_LightDataPartial updateLightDataB{};
    zClass_NodePartial *updateLights[2] = {&updateLightA, &updateLightB};
    updateWorld.classData = &updateWorldData;
    updateWorldData.lightCount = 2;
    updateWorldData.lightNodes = updateLights;
    updateLightA.classId = 9;
    updateLightA.flags = 0x04;
    updateLightA.classData = &updateLightDataA;
    updateLightDataA.dirty = 1;
    updateLightDataA.localPosition = {1.0f, 2.0f, 3.0f};
    updateLightDataA.isDirectionalMode = 1;
    updateLightB.classId = 9;
    updateLightB.flags = 0x04;
    updateLightB.classData = &updateLightDataB;
    updateLightDataB.dirty = 1;
    updateLightDataB.localPosition = {4.0f, 5.0f, 6.0f};
    updateLightDataB.isDirectionalMode = 1;
    if (zClass_World::UpdateAllLights(&updateWorld) != 0 || updateLightDataA.dirty != 0 ||
        updateLightDataB.dirty != 0 || updateLightDataA.worldPosition.x != 1.0f ||
        updateLightDataA.worldPosition.y != 2.0f || updateLightDataA.worldPosition.z != 3.0f ||
        updateLightDataA.viewPos.x != 6.0f || updateLightDataA.viewPos.y != 8.0f ||
        updateLightDataA.viewPos.z != 10.0f || updateLightDataB.worldPosition.x != 4.0f ||
        updateLightDataB.worldPosition.y != 5.0f || updateLightDataB.worldPosition.z != 6.0f ||
        updateLightDataB.viewPos.x != 9.0f || updateLightDataB.viewPos.y != 11.0f ||
        updateLightDataB.viewPos.z != 13.0f) {
        return 30;
    }

    zClass_NodePartial updateSoundA{};
    zClass_NodePartial updateSoundB{};
    zClass_SoundDataPartial updateSoundDataA{};
    zClass_SoundDataPartial updateSoundDataB{};
    zClass_NodePartial *updateSounds[2] = {&updateSoundA, &updateSoundB};
    updateWorld.classData = &updateWorldData;
    updateWorldData.soundCount = 2;
    updateWorldData.soundNodes = updateSounds;
    updateSoundA.classId = 10;
    updateSoundA.flags = 0x04;
    updateSoundA.classData = &updateSoundDataA;
    updateSoundDataA.runtimeFlags = 0x03;
    updateSoundDataA.localPosition = {1.0f, 2.0f, 3.0f};
    updateSoundB.classId = 10;
    updateSoundB.flags = 0x04;
    updateSoundB.classData = &updateSoundDataB;
    updateSoundDataB.runtimeFlags = 0x03;
    updateSoundDataB.localPosition = {4.0f, 5.0f, 6.0f};
    if (zClass_World::UpdateAllSounds(&updateWorld) != 0 ||
        updateSoundDataA.runtimeFlags != 0x06 || updateSoundDataB.runtimeFlags != 0x06 ||
        updateSoundDataA.worldPos.x != 1.0f || updateSoundDataA.worldPos.y != 2.0f ||
        updateSoundDataA.worldPos.z != 3.0f || updateSoundDataB.worldPos.x != 4.0f ||
        updateSoundDataB.worldPos.y != 5.0f || updateSoundDataB.worldPos.z != 6.0f) {
        return 3;
    }

    return 0;
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
