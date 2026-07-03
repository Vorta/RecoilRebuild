#include "zclass.h"
#include "zdi.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
void reset_zclass_type_lists_for_test() {
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
} // namespace

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

extern "C" int zclass_cls_di_try_get_polygon_hit_at_query_xz_smoke() {
    zVec3 polygon[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
                        {1.0f, 0.0f, 0.0f}};
    zClassDiPickCandidateEntry candidate{};

    if (zClass_cls_di::TryGetPolygonHitAtQueryXZ(&candidate, polygon, 0.25f, 0.25f, 3) != 1 ||
        candidate.surfaceNormal.x != 0.0f ||
        candidate.surfaceNormal.y != 1.0f ||
        candidate.surfaceNormal.z != 0.0f ||
        candidate.hitPos.y != 0.0f) {
        return 1;
    }

    candidate.hitPos.y = -99.0f;
    if (zClass_cls_di::TryGetPolygonHitAtQueryXZ(&candidate, polygon, 1.25f, 0.25f, 3) != 0 ||
        candidate.hitPos.y != -99.0f) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_cls_di_region_filter_mesh_faces_smoke() {
    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.00001f; };

    zVec3 vertexCache[65] = {};
    zVec3 normalCache[65] = {};
    zVec3 *const savedVertices = g_zModel_PointInPolygonVertices;
    zVec3 *const savedNormals = g_zModel_PointInPolygonEdgeNormals;
    const int savedVertexCount = g_zModel_PointInPolygonVertexCount;

    g_zModel_PointInPolygonVertices = vertexCache;
    g_zModel_PointInPolygonEdgeNormals = normalCache;
    g_zModel_PointInPolygonVertexCount = 37;

    zVec3 tooMany[65] = {};
    if (zClass_cls_di::FilterRegionsAgainstMeshFaces(tooMany, 65) != 0 ||
        g_zModel_PointInPolygonVertexCount != 0) {
        g_zModel_PointInPolygonVertices = savedVertices;
        g_zModel_PointInPolygonEdgeNormals = savedNormals;
        g_zModel_PointInPolygonVertexCount = savedVertexCount;
        return 1;
    }

    zVec3 clockwiseSquare[4] = {{0.0f, 0.0f, 0.0f},
                               {0.0f, 0.0f, 2.0f},
                               {2.0f, 0.0f, 2.0f},
                               {2.0f, 0.0f, 0.0f}};
    if (zClass_cls_di::FilterRegionsAgainstMeshFaces(clockwiseSquare, 4) != 1 ||
        g_zModel_PointInPolygonVertexCount != 4) {
        g_zModel_PointInPolygonVertices = savedVertices;
        g_zModel_PointInPolygonEdgeNormals = savedNormals;
        g_zModel_PointInPolygonVertexCount = savedVertexCount;
        return 2;
    }

    if (!nearFloat(vertexCache[2].x, 2.0f) || !nearFloat(vertexCache[2].z, 2.0f) ||
        !nearFloat(normalCache[0].x, 1.0f) || !nearFloat(normalCache[0].z, 0.0f) ||
        !nearFloat(normalCache[1].x, 0.0f) || !nearFloat(normalCache[1].z, -1.0f) ||
        !nearFloat(normalCache[2].x, -1.0f) || !nearFloat(normalCache[2].z, 0.0f) ||
        !nearFloat(normalCache[3].x, 0.0f) || !nearFloat(normalCache[3].z, 1.0f)) {
        g_zModel_PointInPolygonVertices = savedVertices;
        g_zModel_PointInPolygonEdgeNormals = savedNormals;
        g_zModel_PointInPolygonVertexCount = savedVertexCount;
        return 3;
    }

    zVec3 centerInside{1.0f, 0.0f, 1.0f};
    zVec3 centerOutside{-1.0f, 0.0f, 1.0f};
    if (zClass_cls_di::FilterRegionsAgainstHexahedronFaces(&centerInside, 0.25f) != 1 ||
        zClass_cls_di::FilterRegionsAgainstHexahedronFaces(&centerOutside, 0.25f) != 0 ||
        zClass_cls_di::FilterRegionsAgainstHexahedronFaces(&centerInside, 1.5f) != 0) {
        g_zModel_PointInPolygonVertices = savedVertices;
        g_zModel_PointInPolygonEdgeNormals = savedNormals;
        g_zModel_PointInPolygonVertexCount = savedVertexCount;
        return 4;
    }

    g_zModel_PointInPolygonVertexCount = 0;
    if (zClass_cls_di::FilterRegionsAgainstHexahedronFaces(&centerOutside, 100.0f) != 1) {
        g_zModel_PointInPolygonVertices = savedVertices;
        g_zModel_PointInPolygonEdgeNormals = savedNormals;
        g_zModel_PointInPolygonVertexCount = savedVertexCount;
        return 5;
    }

    g_zModel_PointInPolygonVertices = savedVertices;
    g_zModel_PointInPolygonEdgeNormals = savedNormals;
    g_zModel_PointInPolygonVertexCount = savedVertexCount;
    return 0;
}


