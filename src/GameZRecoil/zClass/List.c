#include "zClass.h"

#include "GameZRecoil/zError/zError.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace zDi {
RECOIL_NOINLINE int RECOIL_FASTCALL GetRefCount(zDiPartial *self);
}

namespace zModel_DiPool {
RECOIL_NOINLINE int RECOIL_FASTCALL FreeIfUnreferenced(zDiPartial *di);
}

extern "C" {
zClass_TypeListLink *g_zClass_TypeList_FreeLinkHead = 0;
zClass_TypeListLink *g_zClass_NodeList_PendingFreeHead = 0;
int g_zClass_DeferredProcessingEnabled = 1;
int g_zClass_TypeList_LiveLinkCount = 0;
int g_zClass_TypeList_PeakLiveLinkCount = 0;
// Recovered storage order at 0x539bac is 6,0,1,2,3,4,5,7,8,9,10,13,14,15,11,12.
zClass_TypeListBucket g_zClass_TypeList_Buckets[16] = {0};
zClass_TypeListLink **g_zClassCallbackPriorityHeadSlotPtrs[6] = {
    &g_zClass_TypeList_Buckets[1].head, &g_zClass_TypeList_Buckets[2].head,
    &g_zClass_TypeList_Buckets[3].head, &g_zClass_TypeList_Buckets[4].head,
    &g_zClass_TypeList_Buckets[5].head, &g_zClass_TypeList_Buckets[6].head,
};
zClass_TypeListLink **g_zClass_TypeList_HeadSlotPtrs[16] = {
    &g_zClass_TypeList_Buckets[1].head,  &g_zClass_TypeList_Buckets[2].head,
    &g_zClass_TypeList_Buckets[3].head,  &g_zClass_TypeList_Buckets[4].head,
    &g_zClass_TypeList_Buckets[5].head,  &g_zClass_TypeList_Buckets[6].head,
    &g_zClass_TypeList_Buckets[0].head,  &g_zClass_TypeList_Buckets[7].head,
    &g_zClass_TypeList_Buckets[8].head,  &g_zClass_TypeList_Buckets[9].head,
    &g_zClass_TypeList_Buckets[10].head, &g_zClass_TypeList_Buckets[14].head,
    &g_zClass_TypeList_Buckets[15].head, &g_zClass_TypeList_Buckets[11].head,
    &g_zClass_TypeList_Buckets[12].head, &g_zClass_TypeList_Buckets[13].head,
};
zClass_TypeListLink **g_zClass_TypeList_TailSlotPtrs[16] = {
    &g_zClass_TypeList_Buckets[1].tail,  &g_zClass_TypeList_Buckets[2].tail,
    &g_zClass_TypeList_Buckets[3].tail,  &g_zClass_TypeList_Buckets[4].tail,
    &g_zClass_TypeList_Buckets[5].tail,  &g_zClass_TypeList_Buckets[6].tail,
    &g_zClass_TypeList_Buckets[0].tail,  &g_zClass_TypeList_Buckets[7].tail,
    &g_zClass_TypeList_Buckets[8].tail,  &g_zClass_TypeList_Buckets[9].tail,
    &g_zClass_TypeList_Buckets[10].tail, &g_zClass_TypeList_Buckets[14].tail,
    &g_zClass_TypeList_Buckets[15].tail, &g_zClass_TypeList_Buckets[11].tail,
    &g_zClass_TypeList_Buckets[12].tail, &g_zClass_TypeList_Buckets[13].tail,
};
zClass_TypeListLink *g_zClass_FilterIterCursor = 0;
const char *g_zClass_FilterIterText = 0;
int g_zClass_FilterIterPrefixLen = 0;
}

namespace {
    const int kQueuedTreeBucket = 7;
    const int kZClassNodeWorld = 2;
    const int kTypeListInsertedFlag = 0x01;
    const char *kListSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\List.c";

    typedef int (RECOIL_FASTCALL *RemoveChildProc)(zClass_NodePartial *,
                                                            zClass_NodePartial *);

    int RemoveAllChildren(zClass_NodePartial *node, RemoveChildProc removeChild) {
        while (node->listCountB > 0) {
            const int result = removeChild(node, node->listB[0]);
            if (result != 0) {
                return result;
            }
        }

        return 0;
    }

    int DeleteObject3DWhenDetached(zClass_NodePartial *node) {
        if (node->listCountA != 0) {
            return 1;
        }

        return zClass_Object3D::DeleteNode(node);
    }
}

namespace zClass_List {
    // Reimplements 0x44f000: zClass_List::DeleteNodeFromLists
    RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNodeFromLists(zClass_NodePartial * node) {
        switch (node->classId) {
        case 1:
            zClass_TypeList::MarkPendingRemoval(8, node);
            break;
        case 2:
            zClass_TypeList::MarkPendingRemoval(13, node);
            break;
        case 3:
            zClass_TypeList::MarkPendingRemoval(14, node);
            break;
        case 4:
            zClass_TypeList::MarkPendingRemoval(15, node);
            break;
        case 7:
            zClass_TypeList::MarkPendingRemoval(11, node);
            break;
        case 8:
            zClass_TypeList::MarkPendingRemoval(12, node);
            break;
        case 9:
            zClass_TypeList::MarkPendingRemoval(9, node);
            break;
        case 10:
            zClass_TypeList::MarkPendingRemoval(10, node);
            break;
        default:
            if (static_cast<unsigned int>(node->classId) > 11) {
                zError::ReportOld(0x200, kListSourceFile, 0x75d,
                                  "Unknown class type while deleting node from lists.\n");
            }
            break;
        }

        if ((node->flags & kTypeListInsertedFlag) != 0) {
            zClass_TypeList::MarkPendingRemoval(kQueuedTreeBucket, node);
        }

        if (node->actionCallback != 0 && node->callbackPriority >= 0 &&
            node->callbackPriority < 6) {
            zClass_TypeList::MarkPendingRemoval(node->callbackPriority, node);
        }

        zClass_TypeList::MarkPendingRemoval(6, node);
        return 0;
    }

    // Reimplements 0x44f690: zClass_List::IterateBucketFiltered
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL IterateBucketFiltered(
        const char *filterText, int bucket, zClass_NodePredicate predicate) {
        if (filterText != 0) {
            g_zClass_FilterIterText = filterText;
            g_zClass_FilterIterCursor = zClass_TypeList::GetBucketHead(bucket);
            return 0;
        }

        zClass_TypeListLink *link = g_zClass_FilterIterCursor;
        while (link != 0) {
            zClass_NodePartial *node = link->node;
            g_zClass_FilterIterCursor = link->next;
            if (predicate(node) != 0) {
                return node;
            }
            link = g_zClass_FilterIterCursor;
        }

        return 0;
    }

    // Reimplements 0x44f1d0: zClass_List::gwListDeleteANode (GameZRecoil/zClass/List.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwListDeleteANode(zClass_NodePartial * node) {
        unsigned int displayInstanceWord = 0;
        int result = zClass_Class::gwNodeGetUserData(node, &displayInstanceWord);
        if (result != 0) {
            return result;
        }

        if (displayInstanceWord != 0) {
            zDiPartial *displayInstance = (zDiPartial *)(unsigned int)displayInstanceWord;
            zClass_Class::gwNodeSetDisplayInstance(node, 0);
            if (zDi::GetRefCount(displayInstance) == 0) {
                result = zModel_DiPool::FreeIfUnreferenced(displayInstance);
                if (result != 0) {
                    return result;
                }
            }
        }

        switch (node->classId) {
        case 1:
            result = RemoveAllChildren(node, zClass_Object3D::RemoveChild);
            if (result != 0) {
                return result;
            }
            return DeleteObject3DWhenDetached(node);

        case 2:
            result = RemoveAllChildren(node, zClass_Animate::RemoveChild);
            if (result != 0) {
                return result;
            }
            if (node->listCountA != 0) {
                return 1;
            }
            return zClass_Animate::DeleteNode(node);

        case 3:
            result = RemoveAllChildren(node, zClass_Lod::RemoveChild);
            if (result != 0) {
                return result;
            }
            return DeleteObject3DWhenDetached(node);

        case 4:
            result = RemoveAllChildren(node, zClass_Sequence::RemoveChild);
            if (result != 0) {
                return result;
            }
            return DeleteObject3DWhenDetached(node);

        case 5:
            result = RemoveAllChildren(node, zClass_Camera::gwCameraRemoveChild);
            if (result != 0) {
                return result;
            }
            return DeleteObject3DWhenDetached(node);

        case 6:
            result = RemoveAllChildren(node, zClass::RemoveChildChecked);
            if (result != 0) {
                return result;
            }
            return DeleteObject3DWhenDetached(node);

        case 7:
            result = RemoveAllChildren(node, zClass_Display::RemoveChild);
            if (result != 0) {
                return result;
            }
            return DeleteObject3DWhenDetached(node);

        case 8:
            result = RemoveAllChildren(node, zClass_Class::RemoveChildValidated);
            if (result != 0) {
                return result;
            }
            return DeleteObject3DWhenDetached(node);

        case 9: {
            result = RemoveAllChildren(node, zClass_Light::RemoveChild);
            if (result != 0) {
                return result;
            }
            zClass_LightDataPartial *lightData =
                static_cast<zClass_LightDataPartial *>(node->classData);
            if (lightData->attachedWorldCount > 0 || node->listCountA != 0) {
                return 1;
            }
            return zClass_Light::DeleteNode(node);
        }

        case 10: {
            result = RemoveAllChildren(node, zClass_Sound::RemoveChild);
            if (result != 0) {
                return result;
            }
            zClass_SoundDataPartial *soundData =
                static_cast<zClass_SoundDataPartial *>(node->classData);
            if (soundData->attachedWorldCount > 0 || node->listCountA != 0) {
                return 1;
            }
            return zClass_Sound::DeleteNode(node);
        }

        case 11: {
            zClass_WorldDataPartial *worldData =
                static_cast<zClass_WorldDataPartial *>(node->classData);

            while (worldData->lightCount > 0) {
                result = zClass_World::RemoveLight(node, worldData->lightNodes[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (worldData->lightNodes != 0) {
                free(worldData->lightNodes);
                worldData->lightNodes = 0;
                free(worldData->lightDataList);
                worldData->lightDataList = 0;
            }

            while (worldData->soundCount > 0) {
                result = zClass_World::RemoveSound(node, worldData->soundNodes[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (worldData->soundNodes != 0) {
                free(worldData->soundNodes);
                worldData->soundNodes = 0;
                free(worldData->soundDataList);
                worldData->soundDataList = 0;
            }

            result = RemoveAllChildren(node, zClass_World::RemoveChildAtGrid);
            if (result != 0) {
                return result;
            }

            {
            for (int row = 0; row < worldData->areaGridRowCount; ++row) {
                zWorldAreaPartial *area = worldData->areaGridRows[row];
                {
                for (int col = 0; col < worldData->areaGridColCount; ++col) {
                    while (area[col].childCount > 0) {
                        result = zClass_World::RemoveChildAtGrid(node, area[col].childList[0]);
                        if (result != 0) {
                            return result;
                        }
                    }
                }
                }
            }
            }

            if (node->listCountA != 0) {
                return 1;
            }
            return zClass_World::DeleteNode(node);
        }

        default:
            zError::ReportOld(0x400, kListSourceFile, 0x8d4,
                              "Unrecognized node class type while deleting node %p (%s): %d\n",
                              node, node, node->classId);
            return 3;
        }
    }

    // Reimplements 0x44f120: zClass_List::DeleteAllOfType
    RECOIL_NOINLINE int RECOIL_FASTCALL DeleteAllOfType(int bucket) {
        zClass::ProcessDeferredWork();

        zClass_TypeListLink *link = zClass_TypeList::Head(bucket);
        int deletedInLastPass = 1;
        while (link != 0 && deletedInLastPass != 0) {
            deletedInLastPass = 0;
            while (link != 0 && deletedInLastPass == 0) {
                zClass_NodePartial *node = link->node;
                if (gwListDeleteANode(node) == 0) {
                    zClass_TypeList::MarkPendingRemoval(bucket, node);
                    deletedInLastPass = 1;
                } else {
                    link = link->next;
                }
            }

            zClass::ProcessDeferredWork();
            link = zClass_TypeList::Head(bucket);
        }

        if (link != 0) {
            zError::ReportOld(0x400, kListSourceFile, 0x92d,
                              "Unable to delete all nodes from list.\n");
            return 1;
        }

        if (zClass_TypeList::CountNodes(bucket) != 0) {
            zError::ReportOld(0x400, kListSourceFile, 0x935,
                              "Deleted list still contains nodes.\n");
            return 1;
        }

        zClass_TypeList::Tail(bucket) = 0;
        return 0;
    }
}

namespace zClass_TypeList {
    // Reimplements 0x44e630: zClass_TypeList::AllocLink
    RECOIL_NOINLINE zClass_TypeListLink *RECOIL_CDECL AllocLink() {
        const int liveCount = g_zClass_TypeList_LiveLinkCount + 1;
        g_zClass_TypeList_LiveLinkCount = liveCount;
        if (liveCount > g_zClass_TypeList_PeakLiveLinkCount) {
            g_zClass_TypeList_PeakLiveLinkCount = liveCount;
        }

        zClass_TypeListLink *link = g_zClass_TypeList_FreeLinkHead;
        if (link == 0) {
            return static_cast<zClass_TypeListLink *>(calloc(1, sizeof(zClass_TypeListLink)));
        }

        zClass_TypeListLink *next = link->next;
        g_zClass_TypeList_FreeLinkHead = next;
        if (next != 0) {
            next->prev = 0;
        }

        link->next = 0;
        link->prev = 0;
        link->pendingRemove = 0;
        return link;
    }

    // Reimplements 0x44e690: zClass_TypeList::FreeLink
    RECOIL_NOINLINE void RECOIL_FASTCALL FreeLink(zClass_TypeListLink * link) {
        --g_zClass_TypeList_LiveLinkCount;

        zClass_TypeListLink *head = g_zClass_TypeList_FreeLinkHead;
        if (head == 0) {
            g_zClass_TypeList_FreeLinkHead = link;
            link->prev = 0;
            g_zClass_TypeList_FreeLinkHead->next = 0;
            return;
        }

        link->next = head;
        link->prev = 0;
        g_zClass_TypeList_FreeLinkHead->prev = link;
        g_zClass_TypeList_FreeLinkHead = link;
    }

    // Reimplements 0x44e6d0: zClass_TypeList::FreeAll
    RECOIL_NOINLINE void RECOIL_CDECL FreeAll() {
        zClass_TypeListLink *link = g_zClass_TypeList_FreeLinkHead;
        while (link != 0) {
            g_zClass_TypeList_FreeLinkHead = link->next;
            free(link);
            link = g_zClass_TypeList_FreeLinkHead;
        }
    }

    // Reimplements 0x44e700: zClass_TypeList::ProcessPendingRemovals
    RECOIL_NOINLINE void RECOIL_FASTCALL ProcessPendingRemovals(int bucket) {
        if (g_zClass_DeferredProcessingEnabled == 0) {
            return;
        }

        zClass_TypeListLink *next = zClass_TypeList::Head(bucket);
        bool removed;
        do {
            removed = false;
            while (next != 0 && next->pendingRemove == 0) {
                next = next->next;
            }

            if (next != 0) {
                zClass_TypeListLink *link = next;
                next = link->next;
                removed = true;

                if (bucket == 7 && (link->node->flags & 0x02) != 0) {
                    link->pendingRemove = 0;
                } else {
                    if (link == zClass_TypeList::Head(bucket)) {
                        zClass_TypeList::Head(bucket) = link->next;
                    }
                    if (link == zClass_TypeList::Tail(bucket)) {
                        zClass_TypeList::Tail(bucket) = link->prev;
                    }
                    if (link->prev != 0) {
                        link->prev->next = link->next;
                    }
                    if (link->next != 0) {
                        link->next->prev = link->prev;
                    }
                    if (bucket == 7) {
                        link->node->flags &= ~0x01;
                    }
                    FreeLink(link);
                }
            }
        } while (removed);

        if (bucket >= 0 && bucket < 16) {
            zClass_TypeList::SetPendingRemovalDirty(bucket, 0);
        }
    }

    // Reimplements 0x44ec90: zClass_TypeList::CountNodes
    RECOIL_NOINLINE int RECOIL_FASTCALL CountNodes(int bucket) {
        int count = 0;
        for (zClass_TypeListLink *link = zClass_TypeList::Head(bucket); link != 0;
             link = link->next) {
            ++count;
        }
        return count;
    }

    // Reimplements 0x44ecb0: zClass_TypeList::PrintBucket
    RECOIL_NOINLINE void RECOIL_FASTCALL PrintBucket(int bucket) {
        int index = 0;
        for (zClass_TypeListLink *link = zClass_TypeList::Head(bucket); link != 0;
             link = link->next) {
            printf("Node %d desc: %s\n", index, link->node->name);
            ++index;
        }
    }

    // Reimplements 0x44ed50: zClass_TypeList::GetBucketHead
    RECOIL_NOINLINE zClass_TypeListLink *RECOIL_FASTCALL GetBucketHead(int bucket) {
        return zClass_TypeList::Head(bucket);
    }

    // Reimplements 0x44eed0: zClass_TypeList::MarkPendingRemoval
    RECOIL_NOINLINE int RECOIL_FASTCALL MarkPendingRemoval(int bucket,
                                                                    zClass_NodePartial *node) {
        zClass_TypeListLink *link = zClass_TypeList::Head(bucket);
        if (link == 0) {
            return 1;
        }

        while (link != 0 && (link->node != node || link->pendingRemove != 0)) {
            link = link->next;
        }

        if (link != 0) {
            link->pendingRemove = 1;
            if (bucket >= 0 && bucket < 16) {
                zClass_TypeList::SetPendingRemovalDirty(bucket, 1);
            }
        }

        return 0;
    }

    // Reimplements 0x44ed90: zClass_TypeList::Insert
    RECOIL_NOINLINE int RECOIL_FASTCALL Insert(int bucket,
                                                        zClass_NodePartial *node) {
        zClass_TypeListLink *link = AllocLink();
        link->node = node;

        zClass_TypeListLink *head = zClass_TypeList::Head(bucket);
        if (head == 0) {
            zClass_TypeList::Tail(bucket) = link;
        } else {
            link->next = head;
            head->prev = link;
        }
        zClass_TypeList::Head(bucket) = link;

        if (bucket == kQueuedTreeBucket) {
            node->flags |= kTypeListInsertedFlag;
            for (int i = 0; i < node->listCountA; ++i) {
                zClass_NodePartial *child = node->listA[i];
                if ((child->flags & kTypeListInsertedFlag) == 0 &&
                    child->classId != kZClassNodeWorld) {
                    InsertChildNodes(kQueuedTreeBucket, child);
                }
            }
        }

        return 0;
    }

    // Reimplements 0x44ee10: zClass_TypeList::InsertChildNodes
    RECOIL_NOINLINE int RECOIL_FASTCALL InsertChildNodes(int bucket,
                                                                  zClass_NodePartial *node) {
        zClass_TypeListLink *link = AllocLink();
        link->node = node;

        if (zClass_TypeList::Head(bucket) == 0) {
            zClass_TypeList::Head(bucket) = link;
            zClass_TypeList::Tail(bucket) = link;
        } else {
            zClass_TypeListLink *tail = zClass_TypeList::Tail(bucket);
            link->prev = tail;
            tail->next = link;
            zClass_TypeList::Tail(bucket) = link;
        }

        if (bucket == kQueuedTreeBucket) {
            node->flags |= kTypeListInsertedFlag;
            for (int i = 0; i < node->listCountA; ++i) {
                zClass_NodePartial *child = node->listA[i];
                if ((child->flags & kTypeListInsertedFlag) == 0 &&
                    child->classId != kZClassNodeWorld) {
                    InsertChildNodes(kQueuedTreeBucket, child);
                }
            }
        }

        return 0;
    }

    // Reimplements 0x44ea70: zClass_TypeList::UpdateAllBuckets
    RECOIL_NOINLINE void RECOIL_CDECL UpdateAllBuckets() {
        for (int i = 0; i < 6; ++i) {
            zClass_TypeListLink *bucket = *g_zClassCallbackPriorityHeadSlotPtrs[i];
            if (bucket != 0) {
                UpdateBucket(bucket);
                zClass_Class::gwNodeUpdateAll();
            }
        }
    }

    // Reimplements 0x44eaa0: zClass_TypeList::UpdateBucket
    RECOIL_NOINLINE void RECOIL_FASTCALL UpdateBucket(zClass_TypeListLink * bucket) {
        const int wasDeferredEnabled = g_zClass_DeferredProcessingEnabled;
        g_zClass_DeferredProcessingEnabled = 0;

        for (zClass_TypeListLink *link = bucket; link != 0; link = link->next) {
            zClass_NodePartial *node = link->node;
            zClass_NodeActionCallback callback =
                (zClass_NodeActionCallback)(node->actionCallback);
            if (callback == 0) {
                link->pendingRemove = 1;
            } else if (link->pendingRemove == 0 && (node->flags & 0x04) != 0) {
                callback(node);
            }
        }

        g_zClass_DeferredProcessingEnabled = wasDeferredEnabled;
        zClass::ProcessDeferredWork();
    }

    // Reimplements 0x44eba0: zClass_TypeList::UpdateQueuedTrees
    RECOIL_NOINLINE int RECOIL_CDECL UpdateQueuedTrees() {
        zClass_TypeListLink *link = zClass_TypeList::Head(kQueuedTreeBucket);
        while (link != 0) {
            while (link != 0 && link->pendingRemove != 0) {
                link = link->next;
            }
            if (link == 0) {
                break;
            }

            gwNode::UpdateTree(link->node);
            link = zClass_TypeList::Head(kQueuedTreeBucket);
        }

        return 0;
    }

    // Reimplements 0x44ebe0: zClass_TypeList::UpdateSequences
    RECOIL_NOINLINE int RECOIL_CDECL UpdateSequences() {
        zClass_TypeListLink *link = zClass_TypeList::Head(11);
        if (link == 0) {
            return 0;
        }

        const int wasDeferredEnabled = g_zClass_DeferredProcessingEnabled;
        g_zClass_DeferredProcessingEnabled = 0;
        do {
            if (link->pendingRemove == 0) {
                zClass_Sequence::Update(link->node);
            }

            link = link->next;
        } while (link != 0);

        g_zClass_DeferredProcessingEnabled = wasDeferredEnabled;
        zClass::ProcessDeferredWork();
        return 0;
    }

    // Reimplements 0x44ec30: zClass_TypeList::UpdateAnimations
    RECOIL_NOINLINE int RECOIL_CDECL UpdateAnimations() {
        zClass_TypeListLink *link = zClass_TypeList::Head(12);
        if (link == 0) {
            return 0;
        }

        const int wasDeferredEnabled = g_zClass_DeferredProcessingEnabled;
        g_zClass_DeferredProcessingEnabled = 0;
        do {
            if (link->pendingRemove == 0 && (link->node->flags & 0x04) != 0) {
                zClass_Animate::UpdateNode(link->node);
            }

            link = link->next;
        } while (link != 0);

        g_zClass_DeferredProcessingEnabled = wasDeferredEnabled;
        zClass::ProcessDeferredWork();
        return 0;
    }
}

namespace gwNode {
    // Reimplements 0x44eb00: gwNode::UpdateSubtree
    RECOIL_NOINLINE int RECOIL_FASTCALL UpdateSubtree(zClass_NodePartial * node) {
        for (int i = 0; i < node->listCountB; ++i) {
            zClass_NodePartial *child = node->listB[i];
            if ((child->flags & 0x01) != 0) {
                UpdateSubtree(child);
            }
        }

        zClass_Class::gwNodeUpdate(node);
        zClass_TypeList::MarkPendingRemoval(kQueuedTreeBucket, node);
        return 0;
    }

    // Reimplements 0x44eb50: gwNode::UpdateTree
    RECOIL_NOINLINE void RECOIL_FASTCALL UpdateTree(zClass_NodePartial * node) {
        UpdateSubtree(node);
        for (int i = 0; i < node->listCountA; ++i) {
            zClass_NodePartial *parent = node->listA[i];
            if (parent->classId != kZClassNodeWorld) {
                UpdateTree(parent);
            }
        }

        if (g_zClass_DeferredProcessingEnabled != 0) {
            zClass::ProcessDeferredWork();
        }
    }
}

namespace zClass_NodeList {
    // Reimplements 0x44ed60: zClass_NodeList::Insert
    RECOIL_NOINLINE int RECOIL_FASTCALL Insert(zClass_NodePartial * node) {
        zClass_TypeListLink *link = zClass_TypeList::AllocLink();
        link->node = node;

        zClass_TypeListLink *head = g_zClass_NodeList_PendingFreeHead;
        if (head != 0) {
            link->next = head;
            head->prev = link;
        }
        g_zClass_NodeList_PendingFreeHead = link;
        return 0;
    }

    // Reimplements 0x44eea0: zClass_NodeList::ProcessPendingFrees
    RECOIL_NOINLINE void RECOIL_CDECL ProcessPendingFrees() {
        zClass_TypeListLink *link = g_zClass_NodeList_PendingFreeHead;
        while (link != 0) {
            g_zClass_NodeList_PendingFreeHead = link->next;
            zClass_Class::FreeNodeToFreeList(link->node);
            zClass_TypeList::FreeLink(link);
            link = g_zClass_NodeList_PendingFreeHead;
        }
    }
}

namespace zClass {
    // Reimplements 0x44e920: zClass::ProcessDeferredWork
    RECOIL_NOINLINE int RECOIL_CDECL ProcessDeferredWork() {
        if (g_zClass_DeferredProcessingEnabled == 0) {
            return 1;
        }

#define ZCLASS_PROCESS_PENDING_BUCKET(bucket)                                                      \
    if (zClass_TypeList::PendingRemovalDirty(bucket) != 0) {                                       \
        zClass_TypeList::ProcessPendingRemovals(bucket);                                           \
    }

        ZCLASS_PROCESS_PENDING_BUCKET(6);
        ZCLASS_PROCESS_PENDING_BUCKET(0);
        ZCLASS_PROCESS_PENDING_BUCKET(1);
        ZCLASS_PROCESS_PENDING_BUCKET(2);
        ZCLASS_PROCESS_PENDING_BUCKET(3);
        ZCLASS_PROCESS_PENDING_BUCKET(4);
        ZCLASS_PROCESS_PENDING_BUCKET(5);
        ZCLASS_PROCESS_PENDING_BUCKET(7);
        ZCLASS_PROCESS_PENDING_BUCKET(8);
        ZCLASS_PROCESS_PENDING_BUCKET(9);
        ZCLASS_PROCESS_PENDING_BUCKET(10);
        ZCLASS_PROCESS_PENDING_BUCKET(13);
        ZCLASS_PROCESS_PENDING_BUCKET(14);
        ZCLASS_PROCESS_PENDING_BUCKET(15);
        ZCLASS_PROCESS_PENDING_BUCKET(11);
        ZCLASS_PROCESS_PENDING_BUCKET(12);

#undef ZCLASS_PROCESS_PENDING_BUCKET

        if (g_zClass_NodeList_PendingFreeHead != 0) {
            zClass_NodeList::ProcessPendingFrees();
        }

        return 0;
    }

    // Reimplements 0x44ecf0: zClass::FindByTypeAndName
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL FindByTypeAndName(int bucket,
                                                                          const char *name) {
        for (zClass_TypeListLink *link = zClass_TypeList::Head(bucket); link != 0;
             link = link->next) {
            if (strcmp(link->node->name, name) == 0) {
                return link->node;
            }
        }

        return 0;
    }

    // Reimplements 0x44f720: zClass::FindNextByTypePrefix_Predicate
    RECOIL_NOINLINE int RECOIL_FASTCALL FindNextByTypePrefix_Predicate(zClass_NodePartial *
                                                                                node) {
        return strncmp(node->name, g_zClass_FilterIterText,
                            static_cast<size_t>(g_zClass_FilterIterPrefixLen)) == 0;
    }

    // Reimplements 0x44f6f0: zClass::FindNextByTypePrefix
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL FindNextByTypePrefix(const char *prefixText,
                                                                             int bucket) {
        if (prefixText != 0) {
            g_zClass_FilterIterPrefixLen = static_cast<int>(strlen(prefixText));
        }

        return zClass_List::IterateBucketFiltered(prefixText, bucket,
                                                  FindNextByTypePrefix_Predicate);
    }

    // Reimplements 0x452810: zClass::AnyNodeMatchesPredicateRecursive
    RECOIL_NOINLINE int RECOIL_FASTCALL AnyNodeMatchesPredicateRecursive(
        zClass_NodePartial * root, zClass_NodePredicate predicate) {
        if (predicate(root) == 1) {
            return 1;
        }

        for (int i = root->listCountB - 1; i >= 0; --i) {
            if (AnyNodeMatchesPredicateRecursive(root->listB[i], predicate) == 1) {
                return 1;
            }
        }

        return 0;
    }

    // Reimplements 0x44f870: zClass::RemoveChildChecked
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChildChecked(zClass_NodePartial * parent,
                                                                    zClass_NodePartial * child) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0xb6,
                              "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Window.c", 0xb7,
                              "Null node pointer.");
            return 5;
        }

        return zClass_Class::RemoveChildGeneric(parent, child);
    }
}

namespace zClass_Class {
    // Reimplements 0x44ec80: zClass_Class::gwNodeUpdateAll
    RECOIL_NOINLINE int RECOIL_CDECL gwNodeUpdateAll() {
        zClass_TypeList::UpdateSequences();
        zClass_TypeList::UpdateAnimations();
        return zClass_TypeList::UpdateQueuedTrees();
    }

    // Reimplements 0x44f750: zClass_Class::gwNodeFindNextByName_Predicate
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeFindNextByName_Predicate(zClass_NodePartial *
                                                                                node) {
        return strcmp(node->name, g_zClass_FilterIterText) == 0;
    }

    // Reimplements 0x44f740: zClass_Class::gwNodeFindNextByName
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL gwNodeFindNextByName(const char *name,
                                                                             int bucket) {
        return zClass_List::IterateBucketFiltered(name, bucket, gwNodeFindNextByName_Predicate);
    }
}
