#include "zClass.h"
#include "zDi.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zSound/zSound.h"

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
