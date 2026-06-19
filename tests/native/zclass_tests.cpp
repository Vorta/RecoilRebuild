#include "zClass.h"
#include "zDi.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
void ResetTypeListsForTest() {
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

void FreeTypeListsForTest() {
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

std::int32_t FloatBitsForTest(float value) {
    std::int32_t raw = 0;
    std::memcpy(&raw, &value, sizeof(raw));
    return raw;
}

std::int32_t __fastcall zclass_test_node_type_0x42(zClass_NodePartial *node) {
    return node->nodeType == 0x42 ? 1 : 0;
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

extern "C" int zclass_type_list_alloc_and_insert_smoke() {
    ResetTypeListsForTest();

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
        FreeTypeListsForTest();
        return 3;
    }

    zClass_TypeListLink *head = zClass_TypeList::Head(7);
    zClass_TypeListLink *tail = zClass_TypeList::Tail(7);
    const bool insertOk =
        head != nullptr && tail != nullptr && head->node == &parent && tail->node == &child &&
        head->next == tail && tail->prev == head && (parent.flags & 1) != 0 &&
        (child.flags & 1) != 0 && worldChild.flags == 0 && zClass_TypeList::CountNodes(7) == 2 &&
        zClass_TypeList::GetBucketHead(7) == head && g_zClass_TypeList_LiveLinkCount == 2 &&
        g_zClass_TypeList_PeakLiveLinkCount == 2;
    if (!insertOk) {
        FreeTypeListsForTest();
        return 4;
    }

    if (zClass_TypeList::MarkPendingRemoval(3, &parent) != 1) {
        FreeTypeListsForTest();
        return 5;
    }
    if (zClass_TypeList::MarkPendingRemoval(7, &child) != 0 || tail->pendingRemove != 1 ||
        zClass_TypeList::PendingRemovalDirty(7) != 1) {
        FreeTypeListsForTest();
        return 6;
    }

    FreeTypeListsForTest();
    zClass_NodePartial pendingNode{};
    if (zClass_NodeList::Insert(&pendingNode) != 0 ||
        g_zClass_NodeList_PendingFreeHead == nullptr ||
        g_zClass_NodeList_PendingFreeHead->node != &pendingNode) {
        FreeTypeListsForTest();
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
        FreeTypeListsForTest();
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
        FreeTypeListsForTest();
        return 9;
    }

    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;
    std::free(keepLink);
    std::free(tailLink);
    FreeTypeListsForTest();

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
        zClass_TypeList::Head(7) = nullptr;
        zClass_TypeList::Tail(7) = nullptr;
        return 10;
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
        return 11;
    }
    freeA->next = freeB;
    g_zClass_TypeList_FreeLinkHead = freeA;
    zClass_TypeList::FreeAll();
    return g_zClass_TypeList_FreeLinkHead == nullptr ? 0 : 12;
}

extern "C" int zclass_zbd_leaf_helpers_smoke() {
    zClass_NodeFreeListSlot slots[3] = {};
    g_zClass_NodeArray = slots;
    g_zClass_NodeArraySize = 3;
    slots[0].freeTag = 0x01000000u;
    slots[1].freeTag = 0x00ffffffu;

    int result = 0;
    if (GameZ_ZBD::NodePtrToIndex(0) != -1) {
        result = 1;
    } else if (GameZ_ZBD::NodePtrToIndex(&slots[2].node) != 2) {
        result = 2;
    } else if (GameZ_ZBD::NodeIndexToPtr(-1) != 0) {
        result = 3;
    } else if (GameZ_ZBD::NodeIndexToPtr(1) != &slots[1].node) {
        result = 4;
    } else if (zClass::NodePtrToValidatedIndex(0) != -1) {
        result = 5;
    } else if (zClass::NodePtrToValidatedIndex(&slots[0].node) != 0) {
        result = 6;
    } else if (zClass::NodePtrToValidatedIndex(&slots[1].node) != -1) {
        result = 7;
    }

    g_zClass_NodeArray = 0;
    g_zClass_NodeArraySize = 0;
    return result;
}

extern "C" int zclass_alloc_node_from_free_list_smoke() {
    ResetTypeListsForTest();

    zClass_NodeFreeListSlot slots[2] = {};
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
        FreeTypeListsForTest();
        return 1;
    }

    if (node->flags != 0x0108001c || node->callbackPriority != 1 || node->gridCol != -1 ||
        node->gridRow != -1 || node->nodeType != 0xff ||
        std::strcmp(
            node->name,
            "Default_node_name"
        ) != 0 ||
        slots[1].damageHandler != nullptr) {
        FreeTypeListsForTest();
        return 2;
    }

    zClass_List::DeleteNodeFromLists(node);
    zClass::ProcessDeferredWork();
    zClass_Class::FreeNodeToFreeList(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    const int result = zClass_Class::AllocNodeFromFreeList() == nullptr ? 0 : 3;
    FreeTypeListsForTest();
    return result;
}

extern "C" int zclass_gwnode_update_tree_smoke(void) {
    ResetTypeListsForTest();
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

    FreeTypeListsForTest();

    ResetTypeListsForTest();
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

    FreeTypeListsForTest();
    g_zClass_DeferredProcessingEnabled = 1;
    return result;
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
    int parentObjectFlags = 0;
    int childObjectFlags = 0;
    zClass_NodePartial parent = {};
    zClass_NodePartial child = {};
    zClass_NodePartial skippedChild = {};
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

extern "C" int zclass_object3d_reset_transform_dirty_smoke() {
    ResetTypeListsForTest();

    zClass_Object3DDataPartial data = {};
    data.flags = 0x10;
    data.rotation.x = 1.0f;
    data.rotation.y = 2.0f;
    data.rotation.z = 3.0f;
    data.scale.x = 4.0f;
    data.scale.y = 5.0f;
    data.scale.z = 6.0f;
    for (int i = 0; i < 12; ++i) {
        data.localMatrix[i] = (float)(i + 1);
    }

    zClass_NodePartial node = {};
    node.classId = 5;
    node.classData = &data;

    if (zClass_Object3D::PropagateTransformDirty(&node) != 0) {
        FreeTypeListsForTest();
        return 1;
    }

    bool matrixOk = true;
    for (int i = 0; i < 12; ++i) {
        const float expected = i == 0 || i == 4 || i == 8 ? 1.0f : 0.0f;
        matrixOk = matrixOk && data.localMatrix[i] == expected;
    }

    const bool ok = data.rotation.x == 0.0f && data.rotation.y == 0.0f &&
                    data.rotation.z == 0.0f && data.scale.x == 1.0f &&
                    data.scale.y == 1.0f && data.scale.z == 1.0f && matrixOk &&
                    (data.flags & 0x39) == 0x29 && (data.flags & 0x10) == 0 &&
                    (node.flags & 0x02000003) == 0x02000003 &&
                    (node.boundsFlags & 0x04) != 0 && zClass_TypeList::Head(7) != nullptr &&
                    zClass_TypeList::Head(7)->node == &node;

    FreeTypeListsForTest();

    zClass_NodePartial noData = {};
    return ok && zClass_Object3D::PropagateTransformDirty(nullptr) == 5 &&
                   zClass_Object3D::PropagateTransformDirty(&noData) == 5
               ? 0
               : 2;
}

extern "C" int zclass_object3d_init_smoke() {
    ResetTypeListsForTest();

    zClass_NodeFreeListSlot slot = {};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Object3D::gwObject3DInit();
    if (node != &slot.node || node->classId != 5 || node->classData == nullptr ||
        g_zClass_NodeFreeHeadIndex != -1 || g_zClass_ActiveNodeCount != 1) {
        FreeTypeListsForTest();
        return 1;
    }

    zClass_Object3DDataPartial *data = (zClass_Object3DDataPartial *)(node->classData);
    if (data->flags != 0x29 || data->scale.x != 1.0f || data->scale.y != 1.0f ||
        data->scale.z != 1.0f || data->localMatrix[0] != 1.0f ||
        data->localMatrix[4] != 1.0f || data->localMatrix[8] != 1.0f ||
        (node->boundsFlags & 4) == 0 || (node->flags & 0x02000003) != 0x02000003) {
        zClass_Object3D::DeleteNode(node);
        FreeTypeListsForTest();
        return 2;
    }

    zClass_Object3D::DeleteNode(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    const int result = zClass_Object3D::gwObject3DInit() == nullptr ? 0 : 3;
    FreeTypeListsForTest();
    return result;
}

extern "C" int zclass_object3d_visible_and_color_smoke() {
    zClass_Object3DDataPartial data = {};
    zClass_NodePartial node = {};
    node.classId = 5;
    node.classData = &data;

    if (zClass_Object3D::gwObject3DSetVisibleFlag(&node, 1) != 0 ||
        (data.flags & 4) == 0) {
        return 1;
    }
    if (zClass_Object3D::gwObject3DSetVisibleFlag(&node, 0) != 0 ||
        (data.flags & 4) != 0) {
        return 2;
    }

    zColorRgb color = {-1.0f, 0.5f, 2.0f};
    if (zClass_Object3D::gwObject3DSetColorAlpha(&node, &color, 1.5f) != 0) {
        return 3;
    }
    if (data.color.red != 0.0f || data.color.green != 0.5f ||
        data.color.blue != 1.0f || data.colorAlpha != 1.0f) {
        return 4;
    }

    data.color.red = 0.25f;
    if (zClass_Object3D::gwObject3DSetColorAlpha(&node, 0, -0.5f) != 0) {
        return 5;
    }
    if (data.color.red != 0.25f || data.colorAlpha != 0.0f) {
        return 6;
    }

    zClass_NodePartial wrongClass = {};
    wrongClass.classId = 4;
    wrongClass.classData = &data;
    if (zClass_Object3D::gwObject3DSetVisibleFlag(0, 1) != 5 ||
        zClass_Object3D::gwObject3DSetVisibleFlag(&wrongClass, 1) != 3) {
        return 7;
    }

    return 0;
}

extern "C" int zclass_object3d_alpha_scale_and_lit_smoke() {
    zClass_Object3DDataPartial data = {};
    zClass_NodePartial node = {};
    node.classId = 5;
    node.classData = &data;

    if (zClass_Object3D::gwObject3DSetAlphaScale(&node, 0.375f) != 0) {
        return 1;
    }

    float alphaScale = 0.0f;
    if (zClass_Object3D::gwObject3DGetAlphaScale(&node, &alphaScale) != 0 ||
        alphaScale != 0.375f) {
        return 2;
    }

    if (zClass_Object3D::gwObject3DSetLitFlag(&node, 1) != 0 ||
        (data.flags & 2) == 0) {
        return 3;
    }
    if (zClass_Object3D::gwObject3DSetLitFlag(&node, 0) != 0 ||
        (data.flags & 2) != 0) {
        return 4;
    }

    zClass_NodePartial nullDataNode = {};
    nullDataNode.classId = 5;
    return zClass_Object3D::gwObject3DGetAlphaScale(&nullDataNode, &alphaScale) == 5 ? 0 : 5;
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
    if (zClass_Object3D::gwObject3DGetScale(&node, &x, &y, &z) != 0 ||
        x != 4.0f ||
        y != 5.0f ||
        z != 6.0f) {
        return 1;
    }

    if (zClass_Object3D::gwObject3DGetRotation(&node, &x, &y, &z) != 0 ||
        x != 1.0f ||
        y != 2.0f ||
        z != 3.0f) {
        return 2;
    }

    if (zClass_Object3D::gwObject3DGetPosition(&node, &x, &y, &z) != 0 ||
        x != 7.0f ||
        y != 8.0f ||
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
    ResetTypeListsForTest();

    zClass_Object3DDataPartial data = {};
    zClass_NodePartial node = {};
    node.classId = 5;
    node.classData = &data;

    data.flags = 0x18;
    if (zClass_Object3D::gwObject3DSetScale(&node, 2.0f, 1.0f, 1.0f) != 0 ||
        data.scale.x != 2.0f ||
        data.scale.y != 1.0f ||
        data.scale.z != 1.0f ||
        (data.flags & 0x18) != 0 ||
        (data.flags & 0x21) != 0x21 ||
        (node.flags & 0x02000003) != 0x02000003) {
        FreeTypeListsForTest();
        return 1;
    }

    data.flags = 0x18;
    if (zClass_Object3D::gwObject3DSetRotation(&node, 0.0f, 0.0f, 0.0f) != 0 ||
        (data.flags & 0x08) == 0 ||
        (data.flags & 0x10) != 0) {
        FreeTypeListsForTest();
        return 2;
    }
    if (zClass_Object3D::gwObject3DTranslateRotation(&node, 0.5f, 1.0f, 1.5f) != 0 ||
        data.rotation.x != 0.5f ||
        data.rotation.y != 1.0f ||
        data.rotation.z != 1.5f ||
        (data.flags & 0x08) != 0) {
        FreeTypeListsForTest();
        return 3;
    }

    data.flags = 0x08;
    if (zClass_Object3D::gwObject3DSetPosition(&node, 0.0f, 0.0f, 0.0f) != 0 ||
        (data.flags & 0x08) == 0) {
        FreeTypeListsForTest();
        return 4;
    }
    if (zClass_Object3D::gwObject3DTranslatePosition(&node, 3.0f, 4.0f, 5.0f) != 0 ||
        data.localMatrix[9] != 3.0f ||
        data.localMatrix[10] != 4.0f ||
        data.localMatrix[11] != 5.0f ||
        (data.flags & 0x08) != 0) {
        FreeTypeListsForTest();
        return 5;
    }

    zClass_NodePartial wrongClass = {};
    wrongClass.classId = 4;
    wrongClass.classData = &data;
    if (zClass_Object3D::gwObject3DTranslatePosition(&wrongClass, 1.0f, 2.0f, 3.0f) != 3) {
        FreeTypeListsForTest();
        return 6;
    }

    FreeTypeListsForTest();
    return zClass_Object3D::gwObject3DSetScale(0, 1.0f, 1.0f, 1.0f) == 5 &&
                   zClass_Object3D::gwObject3DSetRotation(0, 0.0f, 0.0f, 0.0f) == 5 &&
                   zClass_Object3D::gwObject3DSetPosition(0, 0.0f, 0.0f, 0.0f) == 5
               ? 0
               : 7;
}

extern "C" int zclass_child_generic_link_smoke() {
    ResetTypeListsForTest();

    int result = 0;
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
        result = 1;
    }

    if (result == 0 &&
        (zClass_Class::AddChildGeneric(&otherParent, &child) != 0 || child.listCountA != 2 ||
         child.listA[1] != &otherParent || (child.flags & 0x00080000) != 0 ||
         (grandchild.flags & 0x00080000) != 0)) {
        result = 2;
    }

    int validatedClassData = 0;
    zClass_NodePartial validatedParent{};
    zClass_NodePartial validatedChild{};
    validatedParent.classData = &validatedClassData;
    if (result == 0 &&
        (zClass_Class::AddChildValidated(&validatedParent, &validatedChild) != 0 ||
         validatedParent.listCountB != 1 || validatedParent.listB[0] != &validatedChild ||
         validatedChild.listCountA != 1 || validatedChild.listA[0] != &validatedParent)) {
        result = 3;
    }
    if (result == 0 &&
        (zClass_Class::AddChildValidated(nullptr, &validatedChild) != 5 ||
         zClass_Class::AddChildValidated(&validatedParent, nullptr) != 5)) {
        result = 4;
    }
    validatedParent.classData = nullptr;
    if (result == 0 && zClass_Class::AddChildValidated(&validatedParent, &validatedChild) != 5) {
        result = 5;
    }

    zClass_NodePartial dispatchParent{};
    zClass_NodePartial dispatchChild{};
    dispatchParent.classId = 3;
    if (result == 0 &&
        (zClass_Class::AddChild(&dispatchParent, &dispatchChild) != 0 ||
         dispatchParent.listCountB != 1 || dispatchParent.listB[0] != &dispatchChild ||
         dispatchChild.listCountA != 1 || dispatchChild.listA[0] != &dispatchParent)) {
        result = 6;
    }

    zClass_NodePartial animateParent{};
    zClass_NodePartial animateChild{};
    animateParent.classId = 8;
    if (result == 0 &&
        (zClass_Class::AddChild(&animateParent, &animateChild) != 0 ||
         animateParent.listCountB != 1 || animateChild.listCountA != 1 ||
         zClass_Animate::AddChild(nullptr, &animateChild) != 5 ||
         zClass_Animate::AddChild(&animateParent, nullptr) != 5)) {
        result = 7;
    }

    zClass_NodePartial sequenceParent{};
    zClass_NodePartial sequenceChild{};
    sequenceParent.classId = 7;
    if (result == 0 &&
        (zClass_Class::AddChild(&sequenceParent, &sequenceChild) != 1 ||
         zClass_Class::AddChild(nullptr, &sequenceChild) != 5 ||
         zClass_Class::AddChild(&sequenceParent, nullptr) != 5)) {
        result = 8;
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
    FreeTypeListsForTest();
    return result;
}

extern "C" int zclass_child_generic_remove_smoke() {
    ResetTypeListsForTest();

    int result = 0;
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

    if (zClass_Class::RemoveChildGeneric(&parent, &child) != 0 ||
        parent.listCountB != 1 || parent.listB[0] != &sibling ||
        child.listCountA != 1 || child.listA[0] != &remainingParent ||
        (child.flags & 0x00080000) == 0 || (grandchild.flags & 0x00080000) == 0 ||
        (parent.boundsFlags & 2) == 0 || (parent.flags & 3) != 3) {
        result = 1;
    }

    zClass_NodePartial missing{};
    if (result == 0 &&
        (zClass_Class::RemoveChildGeneric(&parent, &missing) != 0 ||
         parent.listCountB != 1 || missing.listCountA != 0)) {
        result = 2;
    }

    zClass_NodePartial checkedParent{};
    zClass_NodePartial checkedChild{};
    zClass_NodePartial *checkedChildren[] = {&checkedChild};
    zClass_NodePartial *checkedParents[] = {&checkedParent};
    checkedParent.listCountB = 1;
    checkedParent.listB = checkedChildren;
    checkedChild.listCountA = 1;
    checkedChild.listA = checkedParents;
    if (result == 0 &&
        (zClass::RemoveChildChecked(&checkedParent, &checkedChild) != 0 ||
         checkedParent.listCountB != 0 || checkedChild.listCountA != 0)) {
        result = 3;
    }
    if (result == 0 &&
        (zClass::RemoveChildChecked(nullptr, &checkedChild) != 5 ||
         zClass::RemoveChildChecked(&checkedParent, nullptr) != 5)) {
        result = 4;
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
    if (result == 0 &&
        (zClass_Class::RemoveChildValidated(&validatedParent, &validatedChild) != 0 ||
         validatedParent.listCountB != 0 || validatedChild.listCountA != 0)) {
        result = 5;
    }
    if (result == 0 &&
        (zClass_Class::RemoveChildValidated(nullptr, &validatedChild) != 5 ||
         zClass_Class::RemoveChildValidated(&validatedParent, nullptr) != 5)) {
        result = 6;
    }
    validatedParent.classData = nullptr;
    if (result == 0 &&
        zClass_Class::RemoveChildValidated(&validatedParent, &validatedChild) != 5) {
        result = 7;
    }

    FreeTypeListsForTest();
    return result;
}

static void zclass_init_single_child_link(
    zClass_NodePartial *parent,
    zClass_NodePartial *child,
    zClass_NodePartial **children,
    zClass_NodePartial **parents
) {
    *children = child;
    *parents = parent;
    parent->flags = 1;
    parent->listCountB = 1;
    parent->listB = children;
    child->listCountA = 1;
    child->listA = parents;
}

static int zclass_link_removed(
    const zClass_NodePartial *parent,
    const zClass_NodePartial *child
) {
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
    ResetTypeListsForTest();

    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    parent.classData = &objectData;

    if (zClass_Object3D::gwObject3DAddChild(&parent, &child) != 0 ||
        parent.listCountB != 1 || child.listCountA != 1) {
        FreeTypeListsForTest();
        return 1;
    }
    if (zClass_Object3D::RemoveChild(&parent, &child) != 0 || parent.listCountB != 0 ||
        child.listCountA != 0) {
        FreeTypeListsForTest();
        return 2;
    }

    std::free(parent.listB);
    std::free(child.listA);
    FreeTypeListsForTest();

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

extern "C" int zclass_remove_dispatch_smoke() {
    ResetTypeListsForTest();

    int result = 0;
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    zClass_Object3DDataPartial parentData{};
    parent.classId = 5;
    parent.classData = &parentData;

    if (zClass_Class::AddChildGeneric(&parent, &child) != 0 ||
        zClass_Class::RemoveChild(&parent, &child) != 0 || parent.listCountB != 0 ||
        child.listCountA != 0) {
        result = 1;
    }

    parent.classId = 99;
    if (result == 0 &&
        (zClass_Class::RemoveChild(&parent, &child) != 1 ||
         zClass_Class::RemoveChild(nullptr, &child) != 5 ||
         zClass_Class::RemoveChild(&parent, nullptr) != 5)) {
        result = 2;
    }

    std::free(parent.listB);
    std::free(child.listA);
    FreeTypeListsForTest();
    return result;
}

extern "C" int zclass_destroy_node_recursive_display_smoke() {
    ResetTypeListsForTest();

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
    slot.node.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&displayPool[0]);
    displayPool[0].refCount = 1;

    if (zClass_Util::DestroyNodeRecursive(&slot.node) != 0) {
        FreeTypeListsForTest();
        return 1;
    }

    const int result =
        slot.node.classData == nullptr && slot.node.userDataOrDiRef == 0 &&
                displayPool[0].nextFreeIndex == 4 && g_zModel_DiPoolFreeHeadIndex == 0 &&
                g_zModel_DiPoolInUseCount == 0 && g_zClass_ActiveNodeCount == 0
            ? 0
            : 2;
    FreeTypeListsForTest();
    return result;
}

extern "C" int zclass_world_animate_delete_node_smoke() {
    ResetTypeListsForTest();

    zClass_NodeFreeListSlot slots[2]{};
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0x333333;
    g_zClass_ActiveNodeCount = 2;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *const world = &slots[0].node;
    zClass_WorldDataPartial *const worldData =
        static_cast<zClass_WorldDataPartial *>(std::calloc(1, sizeof(zClass_WorldDataPartial)));
    if (worldData == nullptr) {
        FreeTypeListsForTest();
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
        FreeTypeListsForTest();
        return 2;
    }
    if (zClass_World::DeleteNode(world) != 0 || world->classData != nullptr ||
        g_zClass_NodeFreeHeadIndex != 0 || g_zClass_ActiveNodeCount != 1) {
        FreeTypeListsForTest();
        return 3;
    }

    zClass_NodePartial *const animate = &slots[1].node;
    animate->classId = 8;
    animate->classData = std::calloc(1, sizeof(zClass_AnimateDataPartial));
    if (animate->classData == nullptr) {
        FreeTypeListsForTest();
        return 4;
    }
    if (zClass_Animate::DeleteNode(animate) != 0 || animate->classData != nullptr ||
        g_zClass_NodeFreeHeadIndex != 1 || g_zClass_ActiveNodeCount != 0) {
        FreeTypeListsForTest();
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
        FreeTypeListsForTest();
        return 6;
    }

    zClass_NodePartial parented{};
    parented.listCountA = 1;
    if (zClass_Class::DeleteNodeByType(&parented) != 1) {
        FreeTypeListsForTest();
        return 7;
    }

    zClass_NodePartial unknown{};
    unknown.classId = 99;
    if (zClass_Class::DeleteNodeByType(&unknown) != 1) {
        FreeTypeListsForTest();
        return 8;
    }

    FreeTypeListsForTest();
    return zClass_Animate::DeleteNode(nullptr) == 5 ? 0 : 9;
}

extern "C" int zclass_display_init_smoke() {
    ResetTypeListsForTest();

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Display::gwDisplayInit();
    if (node != &slot.node || node->classId != 4 || node->classData == nullptr ||
        zClass_TypeList::Head(15) == nullptr || zClass_TypeList::Head(15)->node != node) {
        FreeTypeListsForTest();
        return 1;
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
    if (zClass_Display::RemoveChild(&removeParent, &child) != 0 ||
        removeParent.listCountB != 0 ||
        child.listCountA != 0 || zClass_Display::RemoveChild(nullptr, &child) != 5 ||
        zClass_Display::RemoveChild(&removeParent, nullptr) != 5) {
        FreeTypeListsForTest();
        return 2;
    }

    zClass_Object3D::DeleteNode(node);
    FreeTypeListsForTest();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Display::gwDisplayInit() == nullptr ? 0 : 3;
}

extern "C" int zclass_lod_leaf_smoke() {
    ResetTypeListsForTest();

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Lod::gwLodNew();
    if (node != &slot.node || node->classId != 6 || node->classData == nullptr ||
        g_zClass_ActiveNodeCount != 1) {
        FreeTypeListsForTest();
        return 1;
    }

    zClass_LodDataPartial *data = static_cast<zClass_LodDataPartial *>(node->classData);
    if (data->computeOwnDistance != 1 || data->nearRange != 1000.0f ||
        data->farRangeSq != 1000000.0f || data->active != 1) {
        FreeTypeListsForTest();
        return 2;
    }

    zClass_NodePartial child{};
    if (zClass_Lod::gwLodAddChild(node, &child) != 0 || node->listCountB != 1 ||
        child.listCountA != 1 || zClass_Lod::RemoveChild(node, &child) != 0 ||
        node->listCountB != 0 || child.listCountA != 0) {
        FreeTypeListsForTest();
        return 3;
    }

    zClass_Class::TryFreeNode(node);
    FreeTypeListsForTest();
    return g_zClass_ActiveNodeCount == 0 ? 0 : 4;
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

    ResetTypeListsForTest();

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *camera = zClass_Camera::gwCameraNew();
    if (camera != &slot.node || camera->classId != 1 || camera->classData == nullptr ||
        zClass_TypeList::Head(8) == nullptr || zClass_TypeList::Head(8)->node != camera) {
        FreeTypeListsForTest();
        return 4;
    }

    zClass_NodePartial child{};
    if (zClass_Camera::gwCameraAddChild(camera, &child) != 0 || camera->listCountB != 1 ||
        child.listCountA != 1 || zClass_Camera::gwCameraRemoveChild(camera, &child) != 0 ||
        camera->listCountB != 0 || child.listCountA != 0) {
        FreeTypeListsForTest();
        return 5;
    }
    std::free(camera->listB);
    std::free(child.listA);
    camera->listB = nullptr;
    child.listA = nullptr;

    zClass_Object3D::DeleteNode(camera);
    FreeTypeListsForTest();
    return g_zClass_ActiveNodeCount == 0 ? 0 : 6;
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
    if (zClass_Class::gwNodeGetWorldChild(&child) != nullptr) {
        return 2;
    }

    zClass_NodePartial flagParent{};
    zClass_NodePartial flagChild{};
    zClass_NodePartial flagGrandchild{};
    zClass_NodePartial *flagChildren[] = {&flagChild};
    zClass_NodePartial *flagGrandchildren[] = {&flagGrandchild};
    flagParent.listCountB = 1;
    flagParent.listB = flagChildren;
    flagChild.listCountB = 1;
    flagChild.listB = flagGrandchildren;

    if (zClass_Class::SetSingleParentFlagRecursive(&flagParent, 1) != 0 ||
        (flagParent.flags & 0x00080000) == 0 || (flagChild.flags & 0x00080000) == 0 ||
        (flagGrandchild.flags & 0x00080000) == 0) {
        return 3;
    }
    if (zClass_Class::SetSingleParentFlagRecursive(&flagParent, 0) != 0 ||
        (flagParent.flags & 0x00080000) != 0 || (flagChild.flags & 0x00080000) != 0 ||
        (flagGrandchild.flags & 0x00080000) != 0) {
        return 4;
    }

    return 0;
}

extern "C" int zclass_world_add_child_at_grid_smoke() {
    ResetTypeListsForTest();

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

    int result = 0;
    zClass_NodePartial wholeWorldChild{};
    if (zClass_World::AddChildAtGrid(&world, &wholeWorldChild) != 0 || world.listCountB != 1 ||
        world.listB[0] != &wholeWorldChild || wholeWorldChild.gridCol != -1 ||
        wholeWorldChild.gridRow != -1) {
        result = 1;
    }

    zClass_NodePartial outsideChild{};
    outsideChild.flags = 0x80;
    if (result == 0 &&
        (zClass_World::AddChildAtGrid(&world, &outsideChild) != 0 || world.listCountB != 2 ||
         world.listB[1] != &outsideChild || outsideChild.gridCol != -1 ||
         outsideChild.gridRow != -1)) {
        result = 2;
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
    if (result == 0 &&
        (zClass_World::AddChildAtGrid(&world, &bboxChild) != 0 || row0[0].childCount != 1 ||
         row0[0].childList[0] != &bboxChild || bboxChild.gridCol != 0 ||
         bboxChild.gridRow != 0)) {
        result = 3;
    }

    std::free(row0[0].childList);
    std::free(wholeWorldChild.listA);
    std::free(outsideChild.listA);
    std::free(bboxChild.listA);
    std::free(world.listB);
    FreeTypeListsForTest();
    return result;
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

extern "C" int zclass_world_queue_area_update_smoke() {
    zClass_WorldDataPartial worldData{};
    zClass_NodePartial world{};
    zWorldAreaPartial firstArea{};
    zWorldAreaPartial secondArea{};

    world.flags = 1;
    world.classData = &worldData;

    if (zClass_World::QueueAreaUpdate(&world, &worldData, &firstArea) != 0) {
        return 1;
    }
    if (worldData.pendingAreaUpdateCapacity != 1 ||
        worldData.pendingAreaUpdateCount != 1 ||
        worldData.pendingAreaUpdates == nullptr ||
        worldData.pendingAreaUpdates[0] != &firstArea ||
        (firstArea.areaFlags & 1) == 0 ||
        (world.flags & 3) != 3 ||
        (worldData.flags & 0x10) == 0) {
        std::free(worldData.pendingAreaUpdates);
        return 2;
    }

    if (zClass_World::QueueAreaUpdate(&world, &worldData, &secondArea) != 0) {
        std::free(worldData.pendingAreaUpdates);
        return 3;
    }
    if (worldData.pendingAreaUpdateCapacity != 2 ||
        worldData.pendingAreaUpdateCount != 2 ||
        worldData.pendingAreaUpdates[0] != &firstArea ||
        worldData.pendingAreaUpdates[1] != &secondArea ||
        (secondArea.areaFlags & 1) == 0 ||
        (world.flags & 3) != 3 ||
        (worldData.flags & 0x10) == 0) {
        std::free(worldData.pendingAreaUpdates);
        return 4;
    }

    std::free(worldData.pendingAreaUpdates);
    return 0;
}

extern "C" int zclass_node_metadata_accessors_smoke() {
    zClass_NodePartial node = {};
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

    std::memset(
        node.name,
        'Z',
        sizeof(node.name)
    );
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
    ResetTypeListsForTest();

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
        FreeTypeListsForTest();
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
        FreeTypeListsForTest();
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
        FreeTypeListsForTest();
        return 3;
    }

    zClass_Class::gwNodeSetDisplayInstance(&destSlot.node, nullptr);
    zDi::FreeContents(&clonePool[0]);
    g_zModel_DiPoolBase = nullptr;
    FreeTypeListsForTest();
    g_zClass_CopyNodeCloneDiMode = 0;
    g_zClass_CopyNodeDiArg0 = 0;
    g_zClass_CopyNodeDiArg1 = 0;
    return 0;
}

extern "C" int zclass_set_display_instance_smoke() {
    ResetTypeListsForTest();

    zClass_NodePartial node = {};
    zDiPartial oldDisplay = {};
    zDiPartial newDisplay = {};
    oldDisplay.refCount = 2;
    node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&oldDisplay));

    if (zClass_Class::gwNodeSetDisplayInstance(&node, &newDisplay) != 0 ||
        oldDisplay.refCount != 1 || newDisplay.refCount != 1 ||
        node.userDataOrDiRef !=
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&newDisplay)) ||
        (node.flags & 0x203) != 0x203 || (node.boundsFlags & 1) == 0 ||
        zClass_TypeList::Head(7) == nullptr) {
        FreeTypeListsForTest();
        return 1;
    }

    if (zClass_Class::gwNodeSetDisplayInstance(&node, nullptr) != 0 ||
        newDisplay.refCount != 0 || node.userDataOrDiRef != 0 ||
        (node.flags & 0x200) != 0) {
        FreeTypeListsForTest();
        return 2;
    }

    FreeTypeListsForTest();
    return zClass_Class::gwNodeSetDisplayInstance(nullptr, nullptr) == 5 ? 0 : 3;
}

extern "C" int zclass_copy_node_base_data_smoke() {
    ResetTypeListsForTest();

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
        FreeTypeListsForTest();
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
    FreeTypeListsForTest();
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
    ResetTypeListsForTest();

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
        FreeTypeListsForTest();
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
    FreeTypeListsForTest();
    return cameraOk ? 0 : 2;
}

extern "C" int zclass_copy_object3d_and_lod_smoke() {
    ResetTypeListsForTest();

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
        FreeTypeListsForTest();
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
        FreeTypeListsForTest();
        return 2;
    }
    std::free(objectCopyData);
    FreeTypeListsForTest();

    zClass_NodeFreeListSlot lodPool[1] = {};
    lodPool[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = lodPool;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    ResetTypeListsForTest();

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
        FreeTypeListsForTest();
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
    FreeTypeListsForTest();
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
    ResetTypeListsForTest();

    zClass_NodePartial node{};
    node.callbackPriority = 3;
    if (zClass_Class::gwNodeSetActionCallback(&node, &node) != 0 || node.actionCallback != &node ||
        zClass_TypeList::Head(3) == nullptr || zClass_TypeList::Head(3)->node != &node) {
        FreeTypeListsForTest();
        return 1;
    }
    if (zClass_Class::gwNodeSetActionCallback(&node, nullptr) != 0 ||
        node.actionCallback != nullptr || zClass_TypeList::PendingRemovalDirty(3) == 0) {
        FreeTypeListsForTest();
        return 2;
    }
    zClass::ProcessDeferredWork();
    if (zClass_TypeList::Head(3) != nullptr) {
        FreeTypeListsForTest();
        return 3;
    }

    zClass_NodePartial first{};
    zClass_NodePartial second{};
    first.callbackPriority = 4;
    second.callbackPriority = 4;
    if (zClass_Class::gwNodeSetActionCallback(&first, &first) != 0 ||
        zClass_Class::gwNodeSetActionCallbackTail(&second, &second) != 0 ||
        zClass_TypeList::Head(4)->node != &first || zClass_TypeList::Tail(4)->node != &second) {
        FreeTypeListsForTest();
        return 4;
    }

    zClass_List::DeleteNodeFromLists(&first);
    zClass_List::DeleteNodeFromLists(&second);
    zClass::ProcessDeferredWork();
    zClass_TypeList::FreeAll();

    zClass_NodePartial invalid{};
    invalid.callbackPriority = 6;
    const bool errorsOk =
        zClass_Class::gwNodeSetActionCallback(nullptr, &node) == 5 &&
        zClass_Class::gwNodeSetActionCallbackTail(nullptr, &node) == 5 &&
        zClass_Class::gwNodeSetActionCallback(&invalid, &invalid) == 1;
    FreeTypeListsForTest();
    return errorsOk ? 0 : 5;
}

extern "C" int zclass_node_priority_smoke() {
    ResetTypeListsForTest();

    zClass_NodePartial node{};
    node.callbackPriority = 1;
    if (zClass_Class::gwNodeSetActionCallback(&node, &node) != 0 ||
        zClass_Class::gwNodeSetPriority(&node, 2) != 0 || node.callbackPriority != 2 ||
        zClass_TypeList::PendingRemovalDirty(1) == 0 || zClass_TypeList::Head(2) == nullptr ||
        zClass_TypeList::Head(2)->node != &node) {
        FreeTypeListsForTest();
        return 1;
    }

    zClass::ProcessDeferredWork();
    if (zClass_TypeList::Head(1) != nullptr || zClass_TypeList::Head(2) == nullptr) {
        FreeTypeListsForTest();
        return 2;
    }

    zClass_Class::gwNodeSetPriority(&node, 9);
    if (node.callbackPriority != 9 || zClass_TypeList::PendingRemovalDirty(2) == 0) {
        FreeTypeListsForTest();
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
    if (zClass_Class::gwNodeClearVariantGate(&node, 1) != 0 ||
        (node.flags & 0x01000000) == 0) {
        return 5;
    }
    if (zClass_Class::gwNodeClearVariantGate(&node, 0) != 0 ||
        (node.flags & 0x01000000) != 0) {
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

    zClass_NodePartial signedCountNode{};
    zClass_NodePartial ignoredChild{};
    zClass_NodePartial *ignoredChildren[] = {&ignoredChild};
    signedCountNode.auxFlags = 0xff;
    signedCountNode.listCountB = -1;
    signedCountNode.listB = ignoredChildren;
    ignoredChild.auxFlags = 0xff;
    zClass_Node::MaskExtraFlagsRecursive(&signedCountNode, 0x0f);
    if (signedCountNode.auxFlags != 0x0f || ignoredChild.auxFlags != 0xff) {
        return 12;
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

extern "C" int zclass_find_by_name_and_filtered_iter_smoke() {
    ResetTypeListsForTest();

    zClass_NodePartial first{};
    zClass_NodePartial second{};
    zClass_TypeListLink firstLink{};
    zClass_TypeListLink secondLink{};

    std::strcpy(first.name, "sunlight");
    std::strcpy(second.name, "sunlight");
    first.classId = 6;
    second.classId = 6;
    firstLink.node = &first;
    firstLink.next = &secondLink;
    secondLink.node = &second;
    secondLink.prev = &firstLink;
    zClass_TypeList::Head(6) = &firstLink;
    zClass_TypeList::Tail(6) = &secondLink;

    zClass_NodePartial *const found =
        zClass::FindByTypeAndName(6, "sunlight");
    zClass_NodePartial *const missing =
        zClass::FindByTypeAndName(6, "moonlight");

    zClass_TypeList::Head(6) = nullptr;
    zClass_TypeList::Tail(6) = nullptr;
    zClass_NodePartial *const empty =
        zClass::FindByTypeAndName(6, "sunlight");

    FreeTypeListsForTest();
    return found == &first && missing == nullptr && empty == nullptr ? 0 : 1;
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
    int lightMatrixIdentityFlags[4] = {};
    float *lightMatrixSlots[4] = {};
    zMat4x3 lightBaseMatrix{};
    lightMatrixIdentityFlags[0] = 1;
    lightMatrixSlots[0] = reinterpret_cast<float *>(&lightBaseMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &lightMatrixIdentityFlags[0];
    zMath::g_currentMatrixPtrSlot = &lightMatrixSlots[0];

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
    const int activeLightFlags = node->flags;
    node->flags = activeLightFlags & ~0x04;
    data->dirty = 1;
    if (zClass_Light::gwLightUpdate(node) != 0 || data->dirty != 1) {
        return 153;
    }
    node->flags = activeLightFlags;
    zClass_NodePartial updateNullData{};
    updateNullData.flags = 0x04;
    if (zClass_Light::gwLightUpdate(nullptr) != 5 ||
        zClass_Light::gwLightUpdate(&updateNullData) != 5) {
        return 154;
    }

    zMath::g_zMath_CameraScratchB = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                     0.0f, 0.0f, 1.0f, 10.0f, 20.0f, 30.0f};
    data->dirty = 1;
    data->localPosition = {2.0f, 3.0f, 4.0f};
    data->localRotation = {0.0f, 0.0f, 0.0f};
    data->isPointMode = 1;
    data->coneAngle = 0.0f;
    data->isDirectionalMode = 1;
    data->viewDir = {};
    data->viewPos = {};
    data->worldPosScratch = {};
    if (zClass_Light::gwLightUpdate(node) != 0 || data->dirty != 0 ||
        data->worldPosition.x != 2.0f || data->worldPosition.y != 3.0f ||
        data->worldPosition.z != 4.0f || data->worldDir.x != 0.0f ||
        data->worldDir.y != 0.0f || data->worldDir.z != -1.0f ||
        data->viewDir.x != 0.0f || data->viewDir.y != 0.0f || data->viewDir.z != 1.0f ||
        data->worldPosScratch.x != 2.0f || data->worldPosScratch.y != 3.0f ||
        data->worldPosScratch.z != 4.0f || data->viewPos.x != 12.0f ||
        data->viewPos.y != 23.0f || data->viewPos.z != 34.0f) {
        return 155;
    }

    data->specularColor = {0.2f, 0.4f, 0.6f};
    float specR = 0.0f;
    float specG = 0.0f;
    float specB = 0.0f;
    if (zClass_Light::gwLightGetSpecularColor(node, &specR, &specG, &specB) != 0 || specR != 0.2f ||
        specG != 0.4f || specB != 0.6f) {
        return 16;
    }
    const int savedRendererPath = g_zVideo_ActiveRendererPath;
    const int savedRendererType = g_zVideo_RendererType;
    const int savedNormalizeChannel = g_zVideo_D3DColorNormalizeChannelIndex;
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_fogTargetParamsStaged = {};
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_RendererType = 1;
    g_zVideo_D3DColorNormalizeChannelIndex = -1;
    data->dirty = 0;
    if (zClass_Light::gwLightSetSpecularColor(node, 1.2f, -0.5f, 0.5f) != 0 || data->dirty != 1 ||
        data->specularColor.red != 1.0f || data->specularColor.green != 0.0f ||
        data->specularColor.blue != 0.5f ||
        zRndr::g_fogTargetParamsStaged.packedColor16 != 0xf810 ||
        g_zVideo_D3DColorNormalizeChannelIndex != 0) {
        g_zVideo_ActiveRendererPath = savedRendererPath;
        g_zVideo_RendererType = savedRendererType;
        g_zVideo_D3DColorNormalizeChannelIndex = savedNormalizeChannel;
        return 17;
    }
    g_zVideo_ActiveRendererPath = savedRendererPath;
    g_zVideo_RendererType = savedRendererType;
    g_zVideo_D3DColorNormalizeChannelIndex = savedNormalizeChannel;
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


extern "C" int zclass_sound_leaf_smoke() {
    ResetTypeListsForTest();

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

    stackNode.classId = 10;
    stackNode.classData = &data;
    data.localPosition = {4.0f, 5.0f, 6.0f};
    data.worldPos = {};
    if (zClass_Sound::ComputeWorldTransform(&stackNode, &data) != 0 ||
        data.worldPos.x != 4.0f || data.worldPos.y != 5.0f || data.worldPos.z != 6.0f) {
        return 65;
    }

    data.runtimeFlags = 0x01;
    stackNode.flags = 0;
    if (zClass_Sound::UpdatePlayback(&stackNode) != 0 || data.runtimeFlags != 0x01) {
        return 66;
    }

    stackNode.flags = 0x04;
    data.playHandle = nullptr;
    data.sample = nullptr;
    data.runtimeFlags = 0x03;
    data.localPosition = {7.0f, 8.0f, 9.0f};
    data.worldPos = {};
    if (zClass_Sound::UpdatePlayback(&stackNode) != 0 || data.runtimeFlags != 0x06 ||
        data.worldPos.x != 7.0f || data.worldPos.y != 8.0f || data.worldPos.z != 9.0f) {
        return 67;
    }

    data.runtimeFlags = 0x01;
    data.worldPos = {1.0f, 2.0f, 3.0f};
    if (zClass_Sound::UpdatePlayback(&stackNode) != 0 || data.runtimeFlags != 0 ||
        data.worldPos.x != 1.0f || data.worldPos.y != 2.0f || data.worldPos.z != 3.0f) {
        return 68;
    }

    stackNode.classData = nullptr;
    if (zClass_Sound::UpdatePlayback(nullptr) != 5 ||
        zClass_Sound::UpdatePlayback(&stackNode) != 5 ||
        zClass_Sound::SetSampleSetByName(nullptr, "x") != 5 ||
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

extern "C" int zclass_sound_get_position_smoke() {
    zClass_NodePartial node{};
    zClass_SoundDataPartial data{};
    node.classData = &data;
    data.localPosition = {7.0f, 8.0f, 9.0f};

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (zClass_Sound::gwSoundGetPosition(&node, &x, &y, &z) != 0 ||
        x != 7.0f ||
        y != 8.0f ||
        z != 9.0f) {
        return 1;
    }

    node.classData = nullptr;
    if (zClass_Sound::gwSoundGetPosition(nullptr, &x, &y, &z) != 5 ||
        zClass_Sound::gwSoundGetPosition(&node, &x, &y, &z) != 5) {
        return 2;
    }

    return 0;
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
        gwNode::TransformPoint(&node, &point) != 0 || point.x != 4.0f ||
        point.y != 5.0f || point.z != 6.0f) {
        return 5;
    }

    data.flags = 0x20;
    data.localMatrix[9] = 10.0f;
    data.localMatrix[10] = 20.0f;
    data.localMatrix[11] = 30.0f;
    point = {1.0f, 2.0f, 3.0f};
    if (gwNode::TransformPoint(&node, &point) != 0 || point.x != 11.0f ||
        point.y != 22.0f || point.z != 33.0f) {
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
    if (zModel_Material::SetFlagBit9(nullptr, 1) != 0) {
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

    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    zVec3 boundsBaseVerts[2] = {{1.0f, 2.0f, 3.0f}, {5.0f, -2.0f, 1.0f}};
    zVec3 boundsBlendVerts[2] = {{10.0f, 0.0f, -8.0f}, {-1.0f, 4.0f, 2.0f}};
    zVec3 pointEntry0Cams[1] = {{-4.0f, 1.0f, 6.0f}};
    zVec3 pointEntry1Cams[2] = {{2.0f, -5.0f, 0.0f}, {7.0f, 3.0f, -9.0f}};
    zModel_PointEntryPartial pointEntries[2] = {};
    pointEntries[0].pointCamCount = 1;
    pointEntries[0].pointCamList = pointEntry0Cams;
    pointEntries[1].pointCamCount = 2;
    pointEntries[1].pointCamList = pointEntry1Cams;
    zDiPartial aabbDi{};
    aabbDi.vertCount = 2;
    aabbDi.blendVertCount = 2;
    aabbDi.pointCount = 2;
    aabbDi.verts = boundsBaseVerts;
    aabbDi.blendVerts = boundsBlendVerts;
    aabbDi.pointEntries = pointEntries;
    zBoundsMinMaxPartial aabbBounds{};
    zDi::BuildAabb(&aabbDi, &aabbBounds);
    if (aabbBounds.min.x != -4.0f || aabbBounds.min.y != -5.0f ||
        aabbBounds.min.z != -9.0f || aabbBounds.max.x != 11.0f ||
        aabbBounds.max.y != 3.0f || aabbBounds.max.z != 6.0f ||
        g_zModel_SharedVec3ScratchA[0].x != 11.0f ||
        g_zModel_SharedVec3ScratchA[0].z != -5.0f ||
        g_zModel_SharedVec3ScratchA[1].x != 4.0f ||
        g_zModel_SharedVec3ScratchA[1].y != 2.0f) {
        return 641;
    }

    zVec3 symmetricVerts[2] = {{-2.0f, -3.0f, -4.0f}, {5.0f, 1.0f, 6.0f}};
    zDiPartial symmetricDi{};
    symmetricDi.mode = 1;
    symmetricDi.flags = 0x20;
    symmetricDi.vertCount = 2;
    symmetricDi.verts = symmetricVerts;
    zBoundsMinMaxPartial symmetricBounds{};
    zDi::BuildOriginSymmetricAabb(&symmetricDi, &symmetricBounds);
    if (symmetricBounds.min.x != -6.0f || symmetricBounds.min.y != -3.0f ||
        symmetricBounds.min.z != -6.0f || symmetricBounds.max.x != 6.0f ||
        symmetricBounds.max.y != 3.0f || symmetricBounds.max.z != 6.0f) {
        return 642;
    }

    symmetricDi.flags = 0x10;
    zDi::BuildOriginSymmetricAabb(&symmetricDi, &symmetricBounds);
    if (symmetricBounds.min.x != -6.0f || symmetricBounds.min.y != -6.0f ||
        symmetricBounds.min.z != -6.0f || symmetricBounds.max.x != 6.0f ||
        symmetricBounds.max.y != 6.0f || symmetricBounds.max.z != 6.0f) {
        return 643;
    }

    zClass_NodeFreeListSlot displaySlot{};
    displaySlot.node.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&boundsDi);
    if (zClass_Class::gwNodeUpdateDisplayInstance(&displaySlot.node) != 0 ||
        (displaySlot.node.flags & 0x200) == 0) {
        return 62;
    }
    zBoundsMinMaxPartial *displayBounds =
        reinterpret_cast<zBoundsMinMaxPartial *>(&displaySlot.primaryBounds);
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
    zBBox3f *childBounds = &parentSlot.secondaryBounds;
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
    zBBox3f *primaryBounds = &recalcSlot.primaryBounds;
    zBBox3f *secondaryBounds = &recalcSlot.secondaryBounds;
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
        (rebuildArea.areaFlags & 1) == 0 || (rebuildArea.areaFlags & 0x100) == 0 ||
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
    if (zModel_Material::SetFlagBit9(&materialDirect, 0) != 1 ||
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
