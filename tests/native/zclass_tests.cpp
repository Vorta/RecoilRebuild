#include "zClass.h"

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
