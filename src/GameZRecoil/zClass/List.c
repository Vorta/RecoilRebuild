#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace zDi {
    int __fastcall GetRefCount(zDiPartial * self);
}

namespace zModel_DiPool {
    int __fastcall FreeIfUnreferenced(zDiPartial * di);
}

extern "C" {
/**
 * Reimplements data 0x4f49ac..0x4f4a6b: g_zClass_TypeListZeroShadowBuckets.
 * Owner extent: first 0xc0 bytes of the unreferenced 0x4f49ac..0x4f4a7b
 * TypeList zero-shadow block; BN shows no xrefs into this shadow storage.
 * Purpose: dead zero-initialized TypeList bucket storage preserved in the
 * retail data image apart from the live List.c type-list owner.
 */
zClass_TypeListBucket g_zClass_TypeListZeroShadowBuckets[16] = {0};
/**
 * Reimplements data 0x4f4a6c: g_zClass_TypeListZeroShadowFreeLinkHead.
 * Purpose: dead zero-shadow counterpart of the type-list recycled-link head.
 */
zClass_TypeListLink *g_zClass_TypeListZeroShadowFreeLinkHead = 0;
/**
 * Reimplements data 0x4f4a70: g_zClass_NodeListZeroShadowHead.
 * Purpose: dead zero-shadow counterpart of the pending node-list head.
 */
zClass_TypeListLink *g_zClass_NodeListZeroShadowHead = 0;
/**
 * Reimplements data 0x4f4a74: g_zClass_TypeListZeroShadowAllocCount.
 * Purpose: dead zero-shadow counterpart of the type-list live allocation
 * counter.
 */
int g_zClass_TypeListZeroShadowAllocCount = 0;
/**
 * Reimplements data 0x4f4a78: g_zClass_TypeListZeroShadowAllocPeak.
 * Purpose: dead zero-shadow counterpart of the type-list peak allocation
 * counter.
 */
int g_zClass_TypeListZeroShadowAllocPeak = 0;
/**
 * Reimplements data 0x4f4a7c: g_zClass_FilterIterZeroShadowCursor.
 * Owner extent: 0x4f4a7c..0x4f4a8f is five zero-initialized authored
 * dwords with no BN xrefs, mirroring the live filtered-iterator state at
 * 0x539b98..0x539bab without participating in runtime iteration.
 * Purpose: dead zero-shadow counterpart of the filtered iterator cursor.
 */
zClass_TypeListLink *g_zClass_FilterIterZeroShadowCursor = 0;
/**
 * Reimplements data 0x4f4a80: g_zClass_FilterIterZeroShadowUnknownDword0.
 * Purpose: dead zero-shadow counterpart of the filtered iterator reserved
 * dword between cursor and filter text.
 */
unsigned int g_zClass_FilterIterZeroShadowUnknownDword0 = 0;
/**
 * Reimplements data 0x4f4a84: g_zClass_FilterIterZeroShadowText.
 * Purpose: dead zero-shadow counterpart of the filtered iterator text
 * pointer.
 */
const char *g_zClass_FilterIterZeroShadowText = 0;
/**
 * Reimplements data 0x4f4a88: g_zClass_FilterIterZeroShadowUnknownDword1.
 * Purpose: dead zero-shadow counterpart of the filtered iterator reserved
 * dword between filter text and prefix length.
 */
unsigned int g_zClass_FilterIterZeroShadowUnknownDword1 = 0;
/**
 * Reimplements data 0x4f4a8c: g_zClass_FilterIterZeroShadowPrefixLen.
 * Purpose: dead zero-shadow counterpart of the filtered iterator cached
 * prefix length.
 */
int g_zClass_FilterIterZeroShadowPrefixLen = 0;
/**
 * Reimplements data 0x539c6c: g_zClass_TypeList_FreeLinkHead.
 * Purpose: head of the recycled type-list link cache used by list allocation.
 */
zClass_TypeListLink *g_zClass_TypeList_FreeLinkHead = 0;
/**
 * Reimplements data 0x539c70: g_zClass_NodeList_PendingFreeHead.
 * Purpose: head of the deferred node-free queue drained by zClass work.
 */
zClass_TypeListLink *g_zClass_NodeList_PendingFreeHead = 0;
/**
 * Reimplements data 0x4dded8: g_zClass_DeferredProcessingEnabled.
 * Purpose: gates deferred type-list removal and pending node-free processing.
 */
int g_zClass_DeferredProcessingEnabled = 1;
/**
 * Reimplements data 0x539c74: g_zClass_TypeList_LiveLinkCount.
 * Purpose: counts type-list links currently allocated outside the free cache.
 */
int g_zClass_TypeList_LiveLinkCount = 0;
/**
 * Reimplements data 0x539c78: g_zClass_TypeList_PeakLiveLinkCount.
 * Purpose: records the peak live type-list link count for diagnostics.
 */
int g_zClass_TypeList_PeakLiveLinkCount = 0;
// Recovered storage order at 0x539bac is 6,0,1,2,3,4,5,7,8,9,10,13,14,15,11,12.
zClass_TypeListBucket g_zClass_TypeList_Buckets[16] = {0};
zClass_TypeListLink **g_zClassCallbackPriorityHeadSlotPtrs[6] = {
    &g_zClass_TypeList_Buckets[1].head,
    &g_zClass_TypeList_Buckets[2].head,
    &g_zClass_TypeList_Buckets[3].head,
    &g_zClass_TypeList_Buckets[4].head,
    &g_zClass_TypeList_Buckets[5].head,
    &g_zClass_TypeList_Buckets[6].head,
};
zClass_TypeListLink **g_zClass_TypeList_HeadSlotPtrs[16] = {
    &g_zClass_TypeList_Buckets[1].head,
    &g_zClass_TypeList_Buckets[2].head,
    &g_zClass_TypeList_Buckets[3].head,
    &g_zClass_TypeList_Buckets[4].head,
    &g_zClass_TypeList_Buckets[5].head,
    &g_zClass_TypeList_Buckets[6].head,
    &g_zClass_TypeList_Buckets[0].head,
    &g_zClass_TypeList_Buckets[7].head,
    &g_zClass_TypeList_Buckets[8].head,
    &g_zClass_TypeList_Buckets[9].head,
    &g_zClass_TypeList_Buckets[10].head,
    &g_zClass_TypeList_Buckets[14].head,
    &g_zClass_TypeList_Buckets[15].head,
    &g_zClass_TypeList_Buckets[11].head,
    &g_zClass_TypeList_Buckets[12].head,
    &g_zClass_TypeList_Buckets[13].head,
};
zClass_TypeListLink **g_zClass_TypeList_TailSlotPtrs[16] = {
    &g_zClass_TypeList_Buckets[1].tail,
    &g_zClass_TypeList_Buckets[2].tail,
    &g_zClass_TypeList_Buckets[3].tail,
    &g_zClass_TypeList_Buckets[4].tail,
    &g_zClass_TypeList_Buckets[5].tail,
    &g_zClass_TypeList_Buckets[6].tail,
    &g_zClass_TypeList_Buckets[0].tail,
    &g_zClass_TypeList_Buckets[7].tail,
    &g_zClass_TypeList_Buckets[8].tail,
    &g_zClass_TypeList_Buckets[9].tail,
    &g_zClass_TypeList_Buckets[10].tail,
    &g_zClass_TypeList_Buckets[14].tail,
    &g_zClass_TypeList_Buckets[15].tail,
    &g_zClass_TypeList_Buckets[11].tail,
    &g_zClass_TypeList_Buckets[12].tail,
    &g_zClass_TypeList_Buckets[13].tail,
};
/**
 * Reimplements data 0x539b98: g_zClass_FilterIterCursor.
 * Owner extent: 0x539b98..0x539bab is five zero-initialized authored
 * dwords used by the filtered type-list iterator; the unnamed slots are
 * modeled explicitly below and are not padding.
 * Purpose: cursor for continued filtered type-list iteration.
 */
zClass_TypeListLink *g_zClass_FilterIterCursor = 0;
/**
 * Reimplements data 0x539b9c: g_zClass_FilterIterUnknownDword0.
 * Purpose: authored zero dword reserved by the List.c filtered iterator data
 * owner between the cursor and filter text pointer.
 */
unsigned int g_zClass_FilterIterUnknownDword0 = 0;
/**
 * Reimplements data 0x539ba0: g_zClass_FilterIterText.
 * Purpose: active exact or prefix text used by filtered type-list predicates.
 */
const char *g_zClass_FilterIterText = 0;
/**
 * Reimplements data 0x539ba4: g_zClass_FilterIterUnknownDword1.
 * Purpose: authored zero dword reserved by the List.c filtered iterator data
 * owner between the filter text pointer and prefix length.
 */
unsigned int g_zClass_FilterIterUnknownDword1 = 0;
/**
 * Reimplements data 0x539ba8: g_zClass_FilterIterPrefixLen.
 * Purpose: cached prefix length used by filtered type-list prefix searches.
 */
int g_zClass_FilterIterPrefixLen = 0;
}

namespace {
    const int kQueuedTreeBucket = 7;
    const int kZClassNodeWorld = 2;
    const int kTypeListInsertedFlag = 0x01;
    const char kListSourceFile[] = "D:\\Proj\\GameZRecoil\\zClass\\List.c";

}

namespace zClass_TypeList {
    /**
     * Reimplements 0x44e630: zClass_TypeList::AllocLink.
     * Purpose: allocate or recycle a type-list link while maintaining live
     * link accounting.
     */
    zClass_TypeListLink *AllocLink() {
        const int liveCount = g_zClass_TypeList_LiveLinkCount + 1;
        g_zClass_TypeList_LiveLinkCount = liveCount;
        if (liveCount > g_zClass_TypeList_PeakLiveLinkCount) {
            g_zClass_TypeList_PeakLiveLinkCount = liveCount;
        }

        zClass_TypeListLink *link = g_zClass_TypeList_FreeLinkHead;
        if (link != 0) {
            zClass_TypeListLink **nextSlot = &link->next;
            zClass_TypeListLink *next = *nextSlot;
            g_zClass_TypeList_FreeLinkHead = next;
            if (next != 0) {
                next->prev = 0;
            }

            *nextSlot = 0;
            link->prev = 0;
            link->pendingRemove = 0;
            return link;
        }

        return (zClass_TypeListLink *)(calloc(
            1,
            sizeof(zClass_TypeListLink)
        ));
    }

    /**
     * Reimplements 0x44e690: zClass_TypeList::FreeLink.
     * Purpose: return an unused type-list link to the global recycled-link
     * list and update live link accounting.
     */
    void __fastcall FreeLink(zClass_TypeListLink * link) {
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

    /**
     * Reimplements 0x44e6d0: zClass_TypeList::FreeAll.
     * Purpose: release every recycled type-list link owned by the global
     * free-list cache.
     */
    void FreeAll() {
        zClass_TypeListLink *link = g_zClass_TypeList_FreeLinkHead;
        while (link != 0) {
            g_zClass_TypeList_FreeLinkHead = link->next;
            free(link);
            link = g_zClass_TypeList_FreeLinkHead;
        }
    }

    /**
     * Reimplements 0x44e700: zClass_TypeList::ProcessPendingRemovals.
     * Purpose: unlink deferred-removal entries from one type-list bucket and
     * recycle their list links.
     */
    void __fastcall ProcessPendingRemovals(int bucket) {
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
            zClass_TypeList::SetPendingRemovalDirty(
                bucket,
                0
            );
        }
    }
}

namespace zClass {
    /**
     * Reimplements 0x44e920: zClass::ProcessDeferredWork.
     * Purpose: process dirty deferred-removal buckets and then drain pending
     * node frees while deferred work is enabled.
     */
    int ProcessDeferredWork() {
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
}

namespace zClass_TypeList {
    /**
     * Reimplements 0x44ea70: zClass_TypeList::UpdateAllBuckets.
     * Purpose: update each non-empty callback-priority bucket and then flush
     * queued node update work.
     */
    void UpdateAllBuckets() {
        for (int i = 0; i < 6; ++i) {
            zClass_TypeListLink *bucket = *g_zClassCallbackPriorityHeadSlotPtrs[i];
            if (bucket != 0) {
                UpdateBucket(bucket);
                zClass_Class::gwNodeUpdateAll();
            }
        }
    }

    /**
     * Reimplements 0x44eaa0: zClass_TypeList::UpdateBucket.
     * Purpose: run eligible action callbacks in one bucket while deferring
     * list mutations until the pass completes.
     */
    void __fastcall UpdateBucket(zClass_TypeListLink * bucket) {
        const int wasDeferredEnabled = g_zClass_DeferredProcessingEnabled;
        g_zClass_DeferredProcessingEnabled = 0;

        for (zClass_TypeListLink *link = bucket; link != 0; link = link->next) {
            zClass_NodePartial *node = link->node;
            zClass_NodeActionCallback callback = (zClass_NodeActionCallback)(node->actionCallback);
            if (callback == 0) {
                link->pendingRemove = 1;
            } else if (link->pendingRemove == 0 && (node->flags & 0x04) != 0) {
                callback(node);
            }
        }

        g_zClass_DeferredProcessingEnabled = wasDeferredEnabled;
        zClass::ProcessDeferredWork();
    }
}

namespace gwNode {
    /**
     * Reimplements 0x44eb00: gwNode::UpdateSubtree.
     * Purpose: update a node subtree and mark each visited node for queued
     * tree-list removal.
     */
    int __fastcall UpdateSubtree(zClass_NodePartial * node) {
        for (int i = 0; i < node->listCountB; ++i) {
            zClass_NodePartial *child = node->listB[i];
            if ((child->flags & 0x01) != 0) {
                UpdateSubtree(child);
            }
        }

        zClass_Class::gwNodeUpdate(node);
        zClass_TypeList::MarkPendingRemoval(
            kQueuedTreeBucket,
            node
        );
        return 0;
    }

    /**
     * Reimplements 0x44eb50: gwNode::UpdateTree.
     * Purpose: update a node tree upward through its non-world parents and
     * process deferred work when enabled.
     */
    void __fastcall UpdateTree(zClass_NodePartial * node) {
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

namespace zClass_TypeList {
    /**
     * Reimplements 0x44eba0: zClass_TypeList::UpdateQueuedTrees.
     * Purpose: process queued tree-update nodes until the queued-tree bucket
     * contains no remaining active work.
     */
    int UpdateQueuedTrees() {
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

    /**
     * Reimplements 0x44ebe0: zClass_TypeList::UpdateSequences.
     * Purpose: update all non-pending sequence nodes while deferring list
     * mutations during the pass.
     */
    int UpdateSequences() {
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

    /**
     * Reimplements 0x44ec30: zClass_TypeList::UpdateAnimations.
     * Purpose: update active animation nodes while deferring list mutations
     * during the pass.
     */
    int UpdateAnimations() {
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

namespace zClass_Class {
    /**
     * Reimplements 0x44ec80: zClass_Class::gwNodeUpdateAll.
     * Purpose: update sequence, animation, and queued-tree work in order.
     */
    int gwNodeUpdateAll() {
        zClass_TypeList::UpdateSequences();
        zClass_TypeList::UpdateAnimations();
        return zClass_TypeList::UpdateQueuedTrees();
    }
}

namespace zClass_TypeList {
    /**
     * Reimplements 0x44ec90: zClass_TypeList::CountNodes.
     * Purpose: count the links currently present in one type-list bucket.
     */
    int __fastcall CountNodes(int bucket) {
        int count = 0;
        for (zClass_TypeListLink *link = zClass_TypeList::Head(bucket); link != 0;
            link = link->next) {
            ++count;
        }
        return count;
    }

    /**
     * Reimplements 0x44ecb0: zClass_TypeList::PrintBucket.
     * Purpose: print each node name in one type-list bucket for diagnostics.
     */
    void __fastcall PrintBucket(int bucket) {
        int index = 0;
        for (zClass_TypeListLink *link = zClass_TypeList::Head(bucket); link != 0;
            link = link->next) {
            printf(
                "Node %d desc: %s\n",
                index,
                link->node->name
            );
            ++index;
        }
    }
}

namespace zClass {
    /**
     * Reimplements 0x44ecf0: zClass::FindByTypeAndName.
     * Purpose: find the first node in a type-list bucket whose name matches.
     */
    zClass_NodePartial *__fastcall FindByTypeAndName(
        int bucket,
        const char *name
    ) {
        for (zClass_TypeListLink *link = zClass_TypeList::Head(bucket); link != 0;
            link = link->next) {
            if (strcmp(
                link->node->name,
                name
            ) == 0) {
                return link->node;
            }
        }

        return 0;
    }
}

namespace zClass_TypeList {
    /**
     * Reimplements 0x44ed50: zClass_TypeList::GetBucketHead.
     * Purpose: return the head link for one type-list bucket.
     */
    zClass_TypeListLink *__fastcall GetBucketHead(int bucket) {
        return zClass_TypeList::Head(bucket);
    }
}

namespace zClass_NodeList {
    /**
     * Reimplements 0x44ed60: zClass_NodeList::Insert.
     * Purpose: queue a node for deferred free processing on the pending-free
     * node list.
     */
    int __fastcall Insert(zClass_NodePartial * node) {
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
}

namespace zClass_TypeList {
    /**
     * Reimplements 0x44ed90: zClass_TypeList::Insert.
     * Purpose: insert a node at the head of a type-list bucket and queue
     * eligible child nodes.
     */
    int __fastcall Insert(
        int bucket,
        zClass_NodePartial *node
    ) {
        zClass_TypeListLink *link = AllocLink();
        link->node = node;

        zClass_TypeListLink **headSlot = g_zClass_TypeList_HeadSlotPtrs[bucket];
        zClass_TypeListLink *head = *headSlot;
        if (head == 0) {
            *g_zClass_TypeList_TailSlotPtrs[bucket] = link;
        } else {
            link->next = head;
            head->prev = link;
        }
        *g_zClass_TypeList_HeadSlotPtrs[bucket] = link;

        if (bucket == kQueuedTreeBucket) {
            node->flags |= kTypeListInsertedFlag;
            for (int i = 0; i < node->listCountA; ++i) {
                zClass_NodePartial *child = node->listA[i];
                if ((child->flags & kTypeListInsertedFlag) == 0 &&
                    child->classId != kZClassNodeWorld) {
                    InsertChildNodes(
                        kQueuedTreeBucket,
                        child
                    );
                }
            }
        }

        return 0;
    }

    /**
     * Reimplements 0x44ee10: zClass_TypeList::InsertChildNodes.
     * Purpose: append a node to a type-list bucket and queue eligible child
     * nodes.
     */
    int __fastcall InsertChildNodes(
        int bucket,
        zClass_NodePartial *node
    ) {
        zClass_TypeListLink *link = AllocLink();
        link->node = node;

        zClass_TypeListLink **tailSlot = g_zClass_TypeList_TailSlotPtrs[bucket];
        zClass_TypeListLink *tail = *tailSlot;
        zClass_TypeListLink **headSlot = g_zClass_TypeList_HeadSlotPtrs[bucket];
        if (*headSlot == 0) {
            *headSlot = link;
            *g_zClass_TypeList_TailSlotPtrs[bucket] = link;
        } else {
            link->prev = tail;
            tail->next = link;
            *g_zClass_TypeList_TailSlotPtrs[bucket] = link;
        }

        if (bucket == kQueuedTreeBucket) {
            node->flags |= kTypeListInsertedFlag;
            for (int i = 0; i < node->listCountA; ++i) {
                zClass_NodePartial *child = node->listA[i];
                if ((child->flags & kTypeListInsertedFlag) == 0 &&
                    child->classId != kZClassNodeWorld) {
                    InsertChildNodes(
                        kQueuedTreeBucket,
                        child
                    );
                }
            }
        }

        return 0;
    }
}

namespace zClass_NodeList {
    /**
     * Reimplements 0x44eea0: zClass_NodeList::ProcessPendingFrees.
     * Purpose: drain pending node frees through the class free-list and
     * recycle their queue links.
     */
    void ProcessPendingFrees() {
        zClass_TypeListLink *link = g_zClass_NodeList_PendingFreeHead;
        while (link != 0) {
            g_zClass_NodeList_PendingFreeHead = link->next;
            zClass_Class::FreeNodeToFreeList(link->node);
            zClass_TypeList::FreeLink(link);
            link = g_zClass_NodeList_PendingFreeHead;
        }
    }
}

namespace zClass_TypeList {
    /**
     * Reimplements 0x44eed0: zClass_TypeList::MarkPendingRemoval.
     * Purpose: mark a matching type-list link for deferred removal and set
     * the bucket dirty flag.
     */
    int __fastcall MarkPendingRemoval(
        int bucket,
        zClass_NodePartial *node
    ) {
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
                zClass_TypeList::SetPendingRemovalDirty(
                    bucket,
                    1
                );
            }
        }

        return 0;
    }
}

namespace zClass_List {
    /**
     * Reimplements 0x44f000: zClass_List::DeleteNodeFromLists.
     * Purpose: queue a node for removal from every type, callback, and
     * update list that can reference it.
     */
    int __fastcall DeleteNodeFromLists(zClass_NodePartial * node) {
        switch (node->classId) {
        case 1:
            zClass_TypeList::MarkPendingRemoval(
                8,
                node
            );
            break;
        case 2:
            zClass_TypeList::MarkPendingRemoval(
                13,
                node
            );
            break;
        case 3:
            zClass_TypeList::MarkPendingRemoval(
                14,
                node
            );
            break;
        case 4:
            zClass_TypeList::MarkPendingRemoval(
                15,
                node
            );
            break;
        case 7:
            zClass_TypeList::MarkPendingRemoval(
                11,
                node
            );
            break;
        case 8:
            zClass_TypeList::MarkPendingRemoval(
                12,
                node
            );
            break;
        case 9:
            zClass_TypeList::MarkPendingRemoval(
                9,
                node
            );
            break;
        case 10:
            zClass_TypeList::MarkPendingRemoval(
                10,
                node
            );
            break;
        default:
            if ((unsigned int)(node->classId) > 11) {
                sprintf(
                    g_zError_DebugMsgBuffer,
                    "%s: Line %d: Unknown class type while deleting node from lists.\n",
                    kListSourceFile,
                    0x75d
                );
                zError::EmitDebugBuffer(1);
            }
            break;
        }

        if ((node->flags & kTypeListInsertedFlag) != 0) {
            zClass_TypeList::MarkPendingRemoval(
                kQueuedTreeBucket,
                node
            );
        }

        if (node->actionCallback != 0 && node->callbackPriority >= 0 &&
            node->callbackPriority < 6) {
            zClass_TypeList::MarkPendingRemoval(
                node->callbackPriority,
                node
            );
        }

        zClass_TypeList::MarkPendingRemoval(
            6,
            node
        );
        return 0;
    }

    /**
     * Reimplements 0x44f120: zClass_List::DeleteAllOfType.
     * Purpose: repeatedly delete every node in one type-list bucket and
     * verify that the bucket is empty afterward.
     */
    int __fastcall DeleteAllOfType(int bucket) {
        zClass::ProcessDeferredWork();

        zClass_TypeListLink *link = zClass_TypeList::Head(bucket);
        int deletedInLastPass = 1;
        while (link != 0 && deletedInLastPass != 0) {
            deletedInLastPass = 0;
            while (link != 0 && deletedInLastPass == 0) {
                zClass_NodePartial *node = link->node;
                if (gwListDeleteANode(node) == 0) {
                    zClass_TypeList::MarkPendingRemoval(
                        bucket,
                        node
                    );
                    deletedInLastPass = 1;
                } else {
                    link = link->next;
                }
            }

            zClass::ProcessDeferredWork();
            link = zClass_TypeList::Head(bucket);
        }

        if (link != 0) {
            zError::ReportOld(
                0x400,
                kListSourceFile,
                0x92d,
                "ERROR deleting list nodes; Not all nodes were deleteable"
            );
            return 1;
        }

        if (zClass_TypeList::CountNodes(bucket) != 0) {
            zError::ReportOld(
                0x400,
                kListSourceFile,
                0x935,
                "ERROR deleting list nodes; %d nodes left on list"
            );
            return 1;
        }

        zClass_TypeList::Tail(bucket) = 0;
        return 0;
    }


    /**
     * Reimplements 0x44f1d0: zClass_List::gwListDeleteANode.
     * Purpose: delete one node according to its class-specific child,
     * ownership, and object-data cleanup rules.
     */
    int __fastcall gwListDeleteANode(zClass_NodePartial * node) {
        unsigned int displayInstanceWord;
        int result = zClass_Class::gwNodeGetUserData(
            node,
            &displayInstanceWord
        );
        if (result != 0) {
            return result;
        }

        if (displayInstanceWord != 0) {
            zClass_Class::gwNodeSetDisplayInstance(
                node,
                0
            );
            if (zDi::GetRefCount((zDiPartial *)(unsigned int)displayInstanceWord) == 0) {
                result = zModel_DiPool::FreeIfUnreferenced(
                    (zDiPartial *)(unsigned int)displayInstanceWord
                );
                if (result != 0) {
                    return result;
                }
            }
        }

        // BN emits the switch bodies in object3D/animate/lod/sequence/camera/window/display/switch/light/sound/world order.
        switch (node->classId) {
        case 5:
            while (node->listCountB > 0) {
                result = zClass_Object3D::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return zClass_Object3D::DeleteNode(node);
            }
            return 1;

        case 8:
            while (node->listCountB > 0) {
                result = zClass_Animate::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return zClass_Animate::DeleteNode(node);
            }
            return 1;

        case 6:
            while (node->listCountB > 0) {
                result = zClass_Lod::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return zClass_Object3D::DeleteNode(node);
            }
            return 1;

        case 7:
            while (node->listCountB > 0) {
                result = zClass_Sequence::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return zClass_Object3D::DeleteNode(node);
            }
            return 1;

        case 1:
            while (node->listCountB > 0) {
                result = zClass_Camera::gwCameraRemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return zClass_Object3D::DeleteNode(node);
            }
            return 1;

        case 3:
            while (node->listCountB > 0) {
                result = zClass::RemoveChildChecked(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return zClass_Object3D::DeleteNode(node);
            }
            return 1;

        case 4:
            while (node->listCountB > 0) {
                result = zClass_Display::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return zClass_Object3D::DeleteNode(node);
            }
            return 1;

        case 11:
            while (node->listCountB > 0) {
                result = zClass_Class::RemoveChildValidated(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return zClass_Object3D::DeleteNode(node);
            }
            return 1;

        case 9: {
            while (node->listCountB > 0) {
                result = zClass_Light::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            zClass_LightDataPartial *lightData = (zClass_LightDataPartial *)(node->classData);
            if (lightData->attachedWorldCount > 0) {
                return 1;
            }
            if (node->listCountA == 0) {
                return zClass_Light::DeleteNode(node);
            }
            return 1;
        }

        case 10: {
            while (node->listCountB > 0) {
                result = zClass_Sound::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            zClass_SoundDataPartial *soundData = (zClass_SoundDataPartial *)(node->classData);
            if (soundData->attachedWorldCount > 0) {
                return 1;
            }
            if (node->listCountA == 0) {
                return zClass_Sound::DeleteNode(node);
            }
            return 1;
        }

        case 2: {
            {
                zClass_WorldDataPartial *worldData =
                    (zClass_WorldDataPartial *)(node->classData);

                while (worldData->lightCount > 0) {
                    result = zClass_World::RemoveLight(
                        node,
                        worldData->lightNodes[0]
                    );
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
            }

            zClass_WorldDataPartial *worldData =
                (zClass_WorldDataPartial *)(node->classData);

            while (worldData->soundCount > 0) {
                result = zClass_World::RemoveSound(
                    node,
                    worldData->soundNodes[0]
                );
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

            while (node->listCountB > 0) {
                result = zClass_World::RemoveChildAtGrid(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }

            {
                int row = 0;
                zWorldAreaPartial **rowCursor = worldData->areaGridRows;
                if (worldData->areaGridRowCount > 0) {
                    do {
                        zWorldAreaPartial *area = *rowCursor;
                        int col = 0;
                        if (worldData->areaGridColCount > 0) {
                            do {
                                while (area->childCount > 0) {
                                    result =
                                        zClass_World::RemoveChildAtGrid(
                                            node,
                                            area->childList[0]
                                        );
                                    if (result != 0) {
                                        return result;
                                    }
                                }
                                ++area;
                                ++col;
                            } while (col < worldData->areaGridColCount);
                        }
                        ++rowCursor;
                        ++row;
                    } while (row < worldData->areaGridRowCount);
                }
            }

            if (node->listCountA == 0) {
                return zClass_World::DeleteNode(node);
            }
            return 1;
        }

        default:
            zError::ReportOld(
                0x400,
                kListSourceFile,
                0x8d4,
                "_gwListDeleteANode(): Unrecognized node class type:node = %s ptr = 0x%08x class_type = %d",
                node,
                node,
                node->classId
            );
            return 3;
        }
    }

}

namespace zClass_List {
    /**
     * Reimplements 0x44f630: zClass_List::RenderActiveCameras (GameZRecoil/zClass/List.c).
     *
     * Purpose: walk the active camera bucket and render each enabled camera through
     * the current software or scene-render path.
     */
    int RenderActiveCameras() {
        zClass_TypeListLink *link = zClass_TypeList::GetBucketHead(8);
        if (link == 0) {
            fprintf(
                stderr,
                "ERROR: No camera on camera list.\n"
            );
            return 1;
        }

        do {
            zClass_NodePartial *const camera = link->node;
            zClass_TypeListLink *const next = link->next;

            if ((camera->flags & 4) != 0) {
                if (g_zVideo_ActiveRendererPath != 0) {
                    zVideo_sw_RenderFrame(
                        camera,
                        0
                    );
                } else {
                    zClass_Camera::RenderScene(
                        camera,
                        0
                    );
                }
            }

            link = next;
        } while (link != 0);

        return 0;
    }
}

namespace zClass_List {
    /**
     * Reimplements 0x44f690: zClass_List::IterateBucketFiltered.
     * Purpose: initialize or continue filtered iteration over one type-list
     * bucket using a caller-supplied predicate.
     */
    zClass_NodePartial *__fastcall IterateBucketFiltered(
        const char *filterText,
        int bucket,
        zClass_NodePredicate predicate
    ) {
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
}

namespace zClass {
    /**
     * Reimplements 0x44f6f0: zClass::FindNextByTypePrefix.
     * Purpose: initialize or continue prefix search over one type-list bucket.
     */
    zClass_NodePartial *__fastcall FindNextByTypePrefix(
        const char *prefixText,
        int bucket
    ) {
        if (prefixText != 0) {
            g_zClass_FilterIterPrefixLen = (int)(strlen(prefixText));
        }

        return zClass_List::IterateBucketFiltered(
            prefixText,
            bucket,
            FindNextByTypePrefix_Predicate
        );
    }

    /**
     * Reimplements 0x44f720: zClass::FindNextByTypePrefix_Predicate.
     * Purpose: test whether a node name matches the active prefix-search text.
     */
    int __fastcall FindNextByTypePrefix_Predicate(zClass_NodePartial * node) {
        return strncmp(
                   node->name,
                   g_zClass_FilterIterText,
                   (size_t)(g_zClass_FilterIterPrefixLen)
               ) == 0;
    }


    /**
     * Reimplements 0x452810: zClass::AnyNodeMatchesPredicateRecursive.
     * Source-shape note: the definition is emitted by cls_util.c; List.c
     * retains callers and the public declaration.
     */

    /**
     * Reimplements 0x44f870: zClass::RemoveChildChecked.
     * Source-shape note: the definition is emitted by Window.c; List.c retains
     * callers of the shared class operation.
     */
}

namespace zClass_Class {

    /**
     * Reimplements 0x44f740: zClass_Class::gwNodeFindNextByName.
     * Purpose: initialize or continue exact-name search over one type-list
     * bucket.
     */
    zClass_NodePartial *__fastcall gwNodeFindNextByName(
        const char *name,
        int bucket
    ) {
        return zClass_List::IterateBucketFiltered(
            name,
            bucket,
            gwNodeFindNextByName_Predicate
        );
    }

    /**
     * Reimplements 0x44f750: zClass_Class::gwNodeFindNextByName_Predicate.
     * Purpose: test whether a node name matches the active exact-name search
     * text.
     */
    int __fastcall gwNodeFindNextByName_Predicate(zClass_NodePartial * node) {
        return strcmp(
            node->name,
            g_zClass_FilterIterText
        ) == 0;
    }

}
