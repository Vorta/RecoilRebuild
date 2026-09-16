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
 * Unresolved candidate: retail references do not establish a TypeList
 * zero-shadow object at 0x4f49ac, its extent, or the scalar identities below.
 * The full .data audit withdrew these BN definitions and positive tracker
 * gates. Existing source contributions remain pending evidence-backed
 * storage/placement recovery; zero-fill does not prove dead TypeList objects.
 * Purpose: retain candidate storage without asserting original bucket ownership.
 */
CZTypeListBucket g_CZClass_TypeListZeroShadowBuckets[16] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-typelistzeroshadowfreelinkhead
 * @recoil-artifact defines .data recoil:data:0x4f4a6c: g_CZClass_TypeListZeroShadowFreeLinkHead.
 * Purpose: retain candidate storage; original recycled-link identity is unresolved.
 */
CZTypeListLink *g_CZClass_TypeListZeroShadowFreeLinkHead = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-nodelistzeroshadowhead
 * @recoil-artifact defines .data recoil:data:0x4f4a70: g_CZClass_NodeListZeroShadowHead.
 * Purpose: retain candidate storage; original node-list identity is unresolved.
 */
CZTypeListLink *g_CZClass_NodeListZeroShadowHead = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-typelistzeroshadowalloccount
 * @recoil-artifact defines .data recoil:data:0x4f4a74: g_CZClass_TypeListZeroShadowAllocCount.
 * Purpose: retain candidate storage; original allocation-count identity is unresolved.
 */
int g_CZClass_TypeListZeroShadowAllocCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-typelistzeroshadowallocpeak
 * @recoil-artifact defines .data recoil:data:0x4f4a78: g_CZClass_TypeListZeroShadowAllocPeak.
 * Purpose: retain candidate storage; original allocation-peak identity is unresolved.
 */
int g_CZClass_TypeListZeroShadowAllocPeak = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteriterzeroshadowcursor
 * @recoil-artifact defines .data recoil:data:0x4f4a7c: g_CZClass_FilterIterZeroShadowCursor.
 * Unresolved candidate: no references prove five independent iterator
 * objects in 0x4f4a7c..0x4f4a8f. Similarity to live storage is not ownership
 * or extent evidence; the former positive tracker gates are blocked.
 * Purpose: retain candidate storage without asserting an iterator-cursor role.
 */
CZTypeListLink *g_CZClass_FilterIterZeroShadowCursor = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteriterzeroshadowunknowndword0
 * @recoil-artifact defines .data recoil:data:0x4f4a80: g_CZClass_FilterIterZeroShadowUnknownDword0.
 * Purpose: retain candidate storage; original scalar identity is unresolved.
 */
unsigned int g_CZClass_FilterIterZeroShadowUnknownDword0 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteriterzeroshadowtext
 * @recoil-artifact defines .data recoil:data:0x4f4a84: g_CZClass_FilterIterZeroShadowText.
 * Purpose: retain candidate storage; original text-pointer identity is unresolved.
 */
const char *g_CZClass_FilterIterZeroShadowText = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteriterzeroshadowunknowndword1
 * @recoil-artifact defines .data recoil:data:0x4f4a88: g_CZClass_FilterIterZeroShadowUnknownDword1.
 * Purpose: retain candidate storage; original scalar identity is unresolved.
 */
unsigned int g_CZClass_FilterIterZeroShadowUnknownDword1 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteriterzeroshadowprefixlen
 * @recoil-artifact defines .data recoil:data:0x4f4a8c: g_CZClass_FilterIterZeroShadowPrefixLen.
 * Purpose: retain candidate storage; original prefix-length identity is unresolved.
 */
int g_CZClass_FilterIterZeroShadowPrefixLen = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-typelist-freelinkhead
 * @recoil-artifact defines .data recoil:data:0x539c6c: g_CZTypeList_FreeLinkHead.
 * Purpose: head of the recycled type-list link cache used by list allocation.
 */
CZTypeListLink *g_CZTypeList_FreeLinkHead = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-nodelist-pendingfreehead
 * @recoil-artifact defines .data recoil:data:0x539c70: g_CZNodeList_PendingFreeHead.
 * Purpose: head of the deferred node-free queue drained by zClass work.
 */
CZTypeListLink *g_CZNodeList_PendingFreeHead = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-deferredprocessingenabled
 * @recoil-artifact defines .data recoil:data:0x4dded8: g_CZClass_DeferredProcessingEnabled.
 * Purpose: gates deferred type-list removal and pending node-free processing.
 */
int g_CZClass_DeferredProcessingEnabled = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-typelist-livelinkcount
 * @recoil-artifact defines .data recoil:data:0x539c74: g_CZTypeList_LiveLinkCount.
 * Purpose: counts type-list links currently allocated outside the free cache.
 */
int g_CZTypeList_LiveLinkCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-typelist-peaklivelinkcount
 * @recoil-artifact defines .data recoil:data:0x539c78: g_CZTypeList_PeakLiveLinkCount.
 * Purpose: records the peak live type-list link count for diagnostics.
 */
int g_CZTypeList_PeakLiveLinkCount = 0;
// Recovered storage order at 0x539bac is 6,0,1,2,3,4,5,7,8,9,10,13,14,15,11,12.
CZTypeListBucket g_CZTypeList_Buckets[16] = {0};
CZTypeListLink **g_CZClassCallbackPriorityHeadSlotPtrs[6] = {
    &g_CZTypeList_Buckets[1].head,
    &g_CZTypeList_Buckets[2].head,
    &g_CZTypeList_Buckets[3].head,
    &g_CZTypeList_Buckets[4].head,
    &g_CZTypeList_Buckets[5].head,
    &g_CZTypeList_Buckets[6].head,
};
CZTypeListLink **g_CZTypeList_HeadSlotPtrs[16] = {
    &g_CZTypeList_Buckets[1].head,
    &g_CZTypeList_Buckets[2].head,
    &g_CZTypeList_Buckets[3].head,
    &g_CZTypeList_Buckets[4].head,
    &g_CZTypeList_Buckets[5].head,
    &g_CZTypeList_Buckets[6].head,
    &g_CZTypeList_Buckets[0].head,
    &g_CZTypeList_Buckets[7].head,
    &g_CZTypeList_Buckets[8].head,
    &g_CZTypeList_Buckets[9].head,
    &g_CZTypeList_Buckets[10].head,
    &g_CZTypeList_Buckets[14].head,
    &g_CZTypeList_Buckets[15].head,
    &g_CZTypeList_Buckets[11].head,
    &g_CZTypeList_Buckets[12].head,
    &g_CZTypeList_Buckets[13].head,
};
CZTypeListLink **g_CZTypeList_TailSlotPtrs[16] = {
    &g_CZTypeList_Buckets[1].tail,
    &g_CZTypeList_Buckets[2].tail,
    &g_CZTypeList_Buckets[3].tail,
    &g_CZTypeList_Buckets[4].tail,
    &g_CZTypeList_Buckets[5].tail,
    &g_CZTypeList_Buckets[6].tail,
    &g_CZTypeList_Buckets[0].tail,
    &g_CZTypeList_Buckets[7].tail,
    &g_CZTypeList_Buckets[8].tail,
    &g_CZTypeList_Buckets[9].tail,
    &g_CZTypeList_Buckets[10].tail,
    &g_CZTypeList_Buckets[14].tail,
    &g_CZTypeList_Buckets[15].tail,
    &g_CZTypeList_Buckets[11].tail,
    &g_CZTypeList_Buckets[12].tail,
    &g_CZTypeList_Buckets[13].tail,
};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteritercursor
 * @recoil-artifact defines .data recoil:data:0x539b98: g_CZClass_FilterIterCursor.
 * Owner extent: 0x539b98..0x539bab is five zero-initialized authored
 * dwords used by the filtered type-list iterator; the unnamed slots are
 * modeled explicitly below and are not padding.
 * Purpose: cursor for continued filtered type-list iteration.
 */
CZTypeListLink *g_CZClass_FilterIterCursor = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteriterunknowndword0
 * @recoil-artifact defines .data recoil:data:0x539b9c: g_CZClass_FilterIterUnknownDword0.
 * Purpose: authored zero dword reserved by the List.c filtered iterator data
 * owner between the cursor and filter text pointer.
 */
unsigned int g_CZClass_FilterIterUnknownDword0 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteritertext
 * @recoil-artifact defines .data recoil:data:0x539ba0: g_CZClass_FilterIterText.
 * Purpose: active exact or prefix text used by filtered type-list predicates.
 */
const char *g_CZClass_FilterIterText = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteriterunknowndword1
 * @recoil-artifact defines .data recoil:data:0x539ba4: g_CZClass_FilterIterUnknownDword1.
 * Purpose: authored zero dword reserved by the List.c filtered iterator data
 * owner between the filter text pointer and prefix length.
 */
unsigned int g_CZClass_FilterIterUnknownDword1 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.g-zclass-filteriterprefixlen
 * @recoil-artifact defines .data recoil:data:0x539ba8: g_CZClass_FilterIterPrefixLen.
 * Purpose: cached prefix length used by filtered type-list prefix searches.
 */
int g_CZClass_FilterIterPrefixLen = 0;
}

namespace {
    const int kQueuedTreeBucket = 7;
    const int kZClassNodeWorld = 2;
    const int kTypeListInsertedFlag = 0x01;
}

namespace CZTypeList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.alloclink
     * @recoil-artifact defines .text recoil:function:0x44e630: CZTypeList::AllocLink.
     * Purpose: allocate or recycle a type-list link while maintaining live
     * link accounting.
     */
    CZTypeListLink *__cdecl AllocLink() {
        const int liveCount = g_CZTypeList_LiveLinkCount + 1;
        g_CZTypeList_LiveLinkCount = liveCount;
        if (liveCount > g_CZTypeList_PeakLiveLinkCount) {
            g_CZTypeList_PeakLiveLinkCount = liveCount;
        }

        CZTypeListLink *link = g_CZTypeList_FreeLinkHead;
        if (link != 0) {
            CZTypeListLink **nextSlot = &link->next;
            CZTypeListLink *next = *nextSlot;
            g_CZTypeList_FreeLinkHead = next;
            if (next != 0) {
                next->prev = 0;
            }

            *nextSlot = 0;
            link->prev = 0;
            link->pendingRemove = 0;
            return link;
        }

        return (CZTypeListLink *)(calloc(1, sizeof(CZTypeListLink)));
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.freelink
     * @recoil-artifact defines .text recoil:function:0x44e690: CZTypeList::FreeLink.
     * @recoil-match byte
     *
     * Purpose: return an unused type-list link to the global recycled-link
     * list and update live link accounting.
     */
    void __fastcall FreeLink(CZTypeListLink * link) {
        --g_CZTypeList_LiveLinkCount;

        CZTypeListLink *head = g_CZTypeList_FreeLinkHead;
        if (head == 0) {
            g_CZTypeList_FreeLinkHead = link;
            link->prev = 0;
            g_CZTypeList_FreeLinkHead->next = 0;
            return;
        }

        link->next = head;
        link->prev = 0;
        g_CZTypeList_FreeLinkHead->prev = link;
        g_CZTypeList_FreeLinkHead = link;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.freeall
     * @recoil-artifact defines .text recoil:function:0x44e6d0: CZTypeList::FreeAll.
     * @recoil-match byte
     *
     * Purpose: release every recycled type-list link owned by the global
     * free-list cache.
     */
    void __cdecl FreeAll() {
        CZTypeListLink *link = g_CZTypeList_FreeLinkHead;
        while (link != 0) {
            g_CZTypeList_FreeLinkHead = link->next;
            free(link);
            link = g_CZTypeList_FreeLinkHead;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.processpendingremovals
     * @recoil-artifact defines .text recoil:function:0x44e700: CZTypeList::ProcessPendingRemovals.
     * Purpose: unlink deferred-removal entries from one type-list bucket and
     * recycle their list links.
     */
    void __fastcall ProcessPendingRemovals(int bucket) {
        if (g_CZClass_DeferredProcessingEnabled == 0) {
            return;
        }

        CZTypeListLink *next = *g_CZTypeList_HeadSlotPtrs[bucket];
        bool removed;
        do {
            removed = false;
            while (next != 0 && next->pendingRemove == 0) {
                next = next->next;
            }

            if (next != 0) {
                CZTypeListLink *link = next;
                next = link->next;
                removed = true;

                if (bucket == 7 && (link->node->flags & 0x02) != 0) {
                    link->pendingRemove = 0;
                } else {
                    if (link == *g_CZTypeList_HeadSlotPtrs[bucket]) {
                        *g_CZTypeList_HeadSlotPtrs[bucket] = link->next;
                    }
                    if (link == *g_CZTypeList_TailSlotPtrs[bucket]) {
                        *g_CZTypeList_TailSlotPtrs[bucket] = link->prev;
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
            ((CZTypeListBucket *)g_CZTypeList_HeadSlotPtrs[bucket])
                ->pendingRemovalDirty = 0;
        }
    }
}

namespace CZClass {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.processdeferredwork
     * @recoil-artifact defines .text recoil:function:0x44e920: CZClass::ProcessDeferredWork.
     * Purpose: process dirty deferred-removal buckets and then drain pending
     * node frees while deferred work is enabled.
     */
    int __cdecl ProcessDeferredWork() {
        if (g_CZClass_DeferredProcessingEnabled == 0) {
            return 1;
        }

#define ZCLASS_PROCESS_PENDING_BUCKET(bucket)                                                      \
    if (((CZTypeListBucket *)g_CZTypeList_HeadSlotPtrs[bucket])                           \
            ->pendingRemovalDirty != 0) {                                                          \
        CZTypeList::ProcessPendingRemovals(bucket);                                           \
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

        if (g_CZNodeList_PendingFreeHead != 0) {
            CZNodeList::ProcessPendingFrees();
        }

        return 0;
    }
}

namespace CZTypeList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.updateallbuckets
     * @recoil-artifact defines .text recoil:function:0x44ea70: CZTypeList::UpdateAllBuckets.
     * Purpose: update each non-empty callback-priority bucket and then flush
     * queued node update work.
     */
    void __cdecl UpdateAllBuckets() {
        for (int i = 0; i < 6; ++i) {
            CZTypeListLink *bucket = *g_CZClassCallbackPriorityHeadSlotPtrs[i];
            if (bucket != 0) {
                UpdateBucket(bucket);
                CZClass::gwNodeUpdateAll();
            }
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.updatebucket
     * @recoil-artifact defines .text recoil:function:0x44eaa0: CZTypeList::UpdateBucket.
     * Purpose: run eligible action callbacks in one bucket while deferring
     * list mutations until the pass completes.
     */
    void __fastcall UpdateBucket(CZTypeListLink * bucket) {
        const int wasDeferredEnabled = g_CZClass_DeferredProcessingEnabled;
        g_CZClass_DeferredProcessingEnabled = 0;

        while (bucket != 0) {
            CZNodePartial *node = bucket->node;
            CZNodeActionCallback callback = (CZNodeActionCallback)(node->actionCallback);
            if (callback == 0) {
                bucket->pendingRemove = 1;
            } else if (bucket->pendingRemove == 0 && (node->flags & 0x04) != 0) {
                callback(node);
            }
            bucket = bucket->next;
        }
        g_CZClass_DeferredProcessingEnabled = wasDeferredEnabled;
        CZClass::ProcessDeferredWork();
    }
}

namespace CZNode {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.updatesubtree
     * @recoil-artifact defines .text recoil:function:0x44eb00: CZNode::UpdateSubtree.
     * @recoil-match byte
     *
     * Purpose: update a node subtree and mark each visited node for queued
     * tree-list removal.
     */
    int __fastcall UpdateSubtree(CZNodePartial * node) {
        for (int i = 0; i < node->listCountB; ++i) {
            CZNodePartial *child = node->listB[i];
            if ((child->flags & 0x01) != 0) {
                UpdateSubtree(child);
            }
        }

        CZClass::gwNodeUpdate(node);
        CZTypeList::MarkPendingRemoval(kQueuedTreeBucket, node);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.updatetree
     * @recoil-artifact defines .text recoil:function:0x44eb50: CZNode::UpdateTree.
     * @recoil-match byte
     *
     * Purpose: update a node tree upward through its non-world parents and
     * process deferred work when enabled.
     */
    void __fastcall UpdateTree(CZNodePartial * node) {
        UpdateSubtree(node);
        for (int i = 0; i < node->listCountA; ++i) {
            CZNodePartial *parent = node->listA[i];
            if (parent->classId != kZClassNodeWorld) {
                UpdateTree(parent);
            }
        }

        if (g_CZClass_DeferredProcessingEnabled != 0) {
            CZClass::ProcessDeferredWork();
        }
    }
}

namespace CZTypeList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.updatequeuedtrees
     * @recoil-artifact defines .text recoil:function:0x44eba0: CZTypeList::UpdateQueuedTrees.
     * @recoil-match byte
     *
     * Purpose: Update queued trees, reloading the queue after each update
     * and skipping links whose removal is still pending.
     */
    int __cdecl UpdateQueuedTrees() {
        CZTypeListLink *link = g_CZTypeList_Buckets[kQueuedTreeBucket].head;
        while (link != 0) {
            if (link->pendingRemove == 0) {
                CZNode::UpdateTree(link->node);
            }
            link = g_CZTypeList_Buckets[kQueuedTreeBucket].head;
            while (link != 0 && link->pendingRemove != 0) {
                link = link->next;
            }
        }
        return 0;
    }



    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.updatesequences
     * @recoil-artifact defines .text recoil:function:0x44ebe0: CZTypeList::UpdateSequences.
     * Purpose: update all non-pending sequence nodes while deferring list
     * mutations during the pass.
     */
    int __cdecl UpdateSequences() {
        CZTypeListLink *link = *g_CZTypeList_HeadSlotPtrs[11];
        if (link == 0) {
            return 0;
        }

        const int wasDeferredEnabled = g_CZClass_DeferredProcessingEnabled;
        g_CZClass_DeferredProcessingEnabled = 0;
        do {
            if (link->pendingRemove == 0) {
                CZSequence::Update(link->node);
            }

            link = link->next;
        } while (link != 0);

        g_CZClass_DeferredProcessingEnabled = wasDeferredEnabled;
        CZClass::ProcessDeferredWork();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.updateanimations
     * @recoil-artifact defines .text recoil:function:0x44ec30: CZTypeList::UpdateAnimations.
     * Purpose: update active animation nodes while deferring list mutations
     * during the pass.
     */
    int __cdecl UpdateAnimations() {
        CZTypeListLink *link = *g_CZTypeList_HeadSlotPtrs[12];
        if (link == 0) {
            return 0;
        }

        const int wasDeferredEnabled = g_CZClass_DeferredProcessingEnabled;
        g_CZClass_DeferredProcessingEnabled = 0;
        do {
            if (link->pendingRemove == 0 && (link->node->flags & 0x04) != 0) {
                CZAnimate::UpdateNode(link->node);
            }

            link = link->next;
        } while (link != 0);

        g_CZClass_DeferredProcessingEnabled = wasDeferredEnabled;
        CZClass::ProcessDeferredWork();
        return 0;
    }
}

namespace CZClass {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.gwnodeupdateall
     * @recoil-artifact defines .text recoil:function:0x44ec80: CZClass::gwNodeUpdateAll.
     * @recoil-match byte
     *
     * Purpose: update sequence, animation, and queued-tree work in order.
     */
    int __cdecl gwNodeUpdateAll() {
        CZTypeList::UpdateSequences();
        CZTypeList::UpdateAnimations();
        return CZTypeList::UpdateQueuedTrees();
    }
}

namespace CZTypeList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.countnodes
     * @recoil-artifact defines .text recoil:function:0x44ec90: CZTypeList::CountNodes.
     *
     * Purpose: count the links currently present in one type-list bucket.
     */
    int __fastcall CountNodes(int bucket) {
        CZTypeListLink *link = *g_CZTypeList_HeadSlotPtrs[bucket];
        int count = 0;
        for (; link != 0; link = link->next) {
            ++count;
        }
        return count;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.printbucket
     * @recoil-artifact defines .text recoil:function:0x44ecb0: CZTypeList::PrintBucket.
     * @recoil-match byte
     *
     * Purpose: print each node name in one type-list bucket for diagnostics.
     */
    void __fastcall PrintBucket(int bucket) {
        int index = 0;
        for (CZTypeListLink *link = *g_CZTypeList_HeadSlotPtrs[bucket]; link != 0;
            link = link->next) {
            printf("Node %d desc: %s\n", index, link->node->name);
            ++index;
        }
    }
}

namespace CZClass {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.findbytypeandname
     * @recoil-artifact defines .text recoil:function:0x44ecf0: CZClass::FindByTypeAndName.
     * Purpose: find the first node in a type-list bucket whose name matches.
     */
    CZNodePartial *__fastcall FindByTypeAndName(
        int bucket,
        const char *name
    ) {
        for (CZTypeListLink *link = *g_CZTypeList_HeadSlotPtrs[bucket]; link != 0;
            link = link->next) {
            if (strcmp(link->node->name, name) == 0) {
                return link->node;
            }
        }

        return 0;
    }
}

namespace CZTypeList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.getbuckethead
     * @recoil-artifact defines .text recoil:function:0x44ed50: CZTypeList::GetBucketHead.
     * @recoil-match byte
     *
     * Purpose: return the head link for one type-list bucket.
     */
    CZTypeListLink *__fastcall GetBucketHead(int bucket) {
        return *g_CZTypeList_HeadSlotPtrs[bucket];
    }
}

namespace CZNodeList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.insert-44ed60
     * @recoil-artifact defines .text recoil:function:0x44ed60: CZNodeList::Insert.
     * @recoil-match byte
     *
     * Purpose: queue a node for deferred free processing on the pending-free
     * node list.
     */
    int __fastcall Insert(CZNodePartial * node) {
        CZTypeListLink *link = CZTypeList::AllocLink();
        link->node = node;

        CZTypeListLink *head = g_CZNodeList_PendingFreeHead;
        if (head != 0) {
            link->next = head;
            head->prev = link;
        }
        g_CZNodeList_PendingFreeHead = link;
        return 0;
    }
}

namespace CZTypeList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.insert-44ed90
     * @recoil-artifact defines .text recoil:function:0x44ed90: CZTypeList::Insert.
     * @recoil-match byte
     *
     * Purpose: insert a node at the head of a type-list bucket and queue
     * eligible child nodes.
     */
    int __fastcall Insert(
        int bucket,
        CZNodePartial *node
    ) {
        CZTypeListLink *link = AllocLink();
        link->node = node;

        CZTypeListLink **headSlot = g_CZTypeList_HeadSlotPtrs[bucket];
        CZTypeListLink *head = *headSlot;
        if (head == 0) {
            *g_CZTypeList_TailSlotPtrs[bucket] = link;
        } else {
            link->next = head;
            head->prev = link;
        }
        *g_CZTypeList_HeadSlotPtrs[bucket] = link;

        if (bucket == kQueuedTreeBucket) {
            node->flags |= kTypeListInsertedFlag;
            for (int i = 0; i < node->listCountA; ++i) {
                CZNodePartial *child = node->listA[i];
                if ((child->flags & kTypeListInsertedFlag) == 0 &&
                    child->classId != kZClassNodeWorld) {
                    InsertChildNodes(kQueuedTreeBucket, child);
                }
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.insertchildnodes
     * @recoil-artifact defines .text recoil:function:0x44ee10: CZTypeList::InsertChildNodes.
     * @recoil-match byte
     *
     * Purpose: append a node to a type-list bucket and queue eligible child
     * nodes.
     */
    int __fastcall InsertChildNodes(
        int bucket,
        CZNodePartial *node
    ) {
        CZTypeListLink *link = AllocLink();
        link->node = node;

        CZTypeListLink **tailSlot = g_CZTypeList_TailSlotPtrs[bucket];
        CZTypeListLink *tail = *tailSlot;
        CZTypeListLink **headSlot = g_CZTypeList_HeadSlotPtrs[bucket];
        if (*headSlot == 0) {
            *headSlot = link;
            *g_CZTypeList_TailSlotPtrs[bucket] = link;
        } else {
            link->prev = tail;
            tail->next = link;
            *g_CZTypeList_TailSlotPtrs[bucket] = link;
        }

        if (bucket == kQueuedTreeBucket) {
            node->flags |= kTypeListInsertedFlag;
            for (int i = 0; i < node->listCountA; ++i) {
                CZNodePartial *child = node->listA[i];
                if ((child->flags & kTypeListInsertedFlag) == 0 &&
                    child->classId != kZClassNodeWorld) {
                    InsertChildNodes(kQueuedTreeBucket, child);
                }
            }
        }

        return 0;
    }
}

namespace CZNodeList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.processpendingfrees
     * @recoil-artifact defines .text recoil:function:0x44eea0: CZNodeList::ProcessPendingFrees.
     * @recoil-match byte
     *
     * Purpose: drain pending node frees through the class free-list and
     * recycle their queue links.
     */
    void __cdecl ProcessPendingFrees() {
        CZTypeListLink *link = g_CZNodeList_PendingFreeHead;
        while (link != 0) {
            g_CZNodeList_PendingFreeHead = link->next;
            CZClass::FreeNodeToFreeList(link->node);
            CZTypeList::FreeLink(link);
            link = g_CZNodeList_PendingFreeHead;
        }
    }
}

namespace CZTypeList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.markpendingremoval
     * @recoil-artifact defines .text recoil:function:0x44eed0: CZTypeList::MarkPendingRemoval.
     * Purpose: mark a matching type-list link for deferred removal and set
     * the bucket dirty flag.
     */
    int __fastcall MarkPendingRemoval(
        int bucket,
        CZNodePartial *node
    ) {
        CZTypeListLink *link = *g_CZTypeList_HeadSlotPtrs[bucket];
        if (link == 0) {
            return 1;
        }

        while (link != 0 && (link->node != node || link->pendingRemove != 0)) {
            link = link->next;
        }

        if (link != 0) {
            link->pendingRemove = 1;
            if (bucket >= 0 && bucket < 16) {
                ((CZTypeListBucket *)g_CZTypeList_HeadSlotPtrs[bucket])
                    ->pendingRemovalDirty = 1;
            }
        }

        return 0;
    }
}

namespace CZList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.deletenodefromlists
     * @recoil-artifact defines .text recoil:function:0x44f000: CZList::DeleteNodeFromLists.
     * Purpose: queue a node for removal from every type, callback, and
     * update list that can reference it.
     */
    int __fastcall DeleteNodeFromLists(CZNodePartial * node) {
        switch (node->classId) {
        case 1:
            CZTypeList::MarkPendingRemoval(8, node);
            break;
        case 2:
            CZTypeList::MarkPendingRemoval(13, node);
            break;
        case 3:
            CZTypeList::MarkPendingRemoval(14, node);
            break;
        case 4:
            CZTypeList::MarkPendingRemoval(15, node);
            break;
        case 7:
            CZTypeList::MarkPendingRemoval(11, node);
            break;
        case 8:
            CZTypeList::MarkPendingRemoval(12, node);
            break;
        case 9:
            CZTypeList::MarkPendingRemoval(9, node);
            break;
        case 10:
            CZTypeList::MarkPendingRemoval(10, node);
            break;
        default:
            if ((unsigned int)(node->classId) > 11) {
                sprintf(
                    g_zError_DebugMsgBuffer,
                    "%s: Line %d: Unknown class type while deleting node from lists.\n",
                    "D:\\Proj\\GameZRecoil\\zClass\\List.c",
                    0x75d
                );
                zError::EmitDebugBuffer(1);
            }
            break;
        }

        if ((node->flags & kTypeListInsertedFlag) != 0) {
            CZTypeList::MarkPendingRemoval(kQueuedTreeBucket, node);
        }

        if (node->actionCallback != 0 && node->callbackPriority >= 0 &&
            node->callbackPriority < 6) {
            CZTypeList::MarkPendingRemoval(node->callbackPriority, node);
        }

        CZTypeList::MarkPendingRemoval(6, node);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.deletealloftype
     * @recoil-artifact defines .text recoil:function:0x44f120: CZList::DeleteAllOfType.
     * Purpose: repeatedly delete every node in one type-list bucket and
     * verify that the bucket is empty afterward.
     */
    int __fastcall DeleteAllOfType(int bucket) {
        CZClass::ProcessDeferredWork();

        CZTypeListLink *link = *g_CZTypeList_HeadSlotPtrs[bucket];
        int deletedInLastPass = 1;
        while (link != 0 && deletedInLastPass != 0) {
            deletedInLastPass = 0;
            while (link != 0 && deletedInLastPass == 0) {
                CZNodePartial *node = link->node;
                if (_gwListDeleteANode(node) == 0) {
                    deletedInLastPass = 1;
                } else {
                    link = link->next;
                }
            }

            CZClass::ProcessDeferredWork();
            link = *g_CZTypeList_HeadSlotPtrs[bucket];
        }

        if (link != 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\List.c",
                0x92d,
                "ERROR deleting list nodes; Not all nodes were deleteable"
            );
            return 1;
        }

        if (CZTypeList::CountNodes(bucket) != 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\List.c",
                0x935,
                "ERROR deleting list nodes; %d nodes left on list"
            );
            return 1;
        }

        *g_CZTypeList_TailSlotPtrs[bucket] = 0;
        return 0;
    }


    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.gwlistdeleteanode
     * @recoil-artifact defines .text recoil:function:0x44f1d0: CZList::_gwListDeleteANode.
     *
     * Purpose: delete one node according to its class-specific child,
     * ownership, and object-data cleanup rules.
     */
    int __fastcall _gwListDeleteANode(CZNodePartial * node) {
        unsigned int displayInstanceWord;
        int result = CZClass::gwNodeGetUserData(node, &displayInstanceWord);
        if (result != 0) {
            return result;
        }

        if (displayInstanceWord != 0) {
            CZClass::gwNodeSetDisplayInstance(node, 0);
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
                result = CZObject3D::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return CZObject3D::DeleteNode(node);
            }
            return 1;

        case 8:
            while (node->listCountB > 0) {
                result = CZAnimate::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return CZAnimate::DeleteNode(node);
            }
            return 1;

        case 6:
            while (node->listCountB > 0) {
                result = CZLod::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }

            if (node->listCountA == 0) {
                return CZLod::DeleteNode(node);
            }
            return 1;

        case 7:
            while (node->listCountB > 0) {
                result = CZSequence::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return CZSequence::DeleteNode(node);
            }
            return 1;

        case 1:
            while (node->listCountB > 0) {
                result = CZCamera::gwCameraRemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return CZCamera::DeleteNode(node);
            }
            return 1;

        case 3:
            while (node->listCountB > 0) {
                result = CZClass::RemoveChildChecked(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return CZWindow::DeleteNode(node);
            }
            return 1;

        case 4:
            while (node->listCountB > 0) {
                result = CZDisplay::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return CZDisplay::DeleteNode(node);
            }
            return 1;

        case 11:
            while (node->listCountB > 0) {
                result = CZClass::RemoveChildValidated(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            if (node->listCountA == 0) {
                return CZSwitch::DeleteNode(node);
            }
            return 1;

        case 9: {
            while (node->listCountB > 0) {
                result = CZLight::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            CZLightDataPartial *lightData = (CZLightDataPartial *)(node->classData);
            if (lightData->attachedWorldCount > 0) {
                return 1;
            }
            if (node->listCountA == 0) {
                return CZLight::DeleteNode(node);
            }
            return 1;
        }

        case 10: {
            while (node->listCountB > 0) {
                result = CZSound::RemoveChild(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }
            CZSoundDataPartial *soundData = (CZSoundDataPartial *)(node->classData);
            if (soundData->attachedWorldCount > 0) {
                return 1;
            }
            if (node->listCountA == 0) {
                return CZSound::DeleteNode(node);
            }
            return 1;
        }

        case 2: {
            {
                CZWorldDataPartial *worldData =
                    (CZWorldDataPartial *)(node->classData);

                while (worldData->lightCount > 0) {
                    result = CZWorld::RemoveLight(node, worldData->lightNodes[0]);
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

            CZWorldDataPartial *worldData =
                (CZWorldDataPartial *)(node->classData);

            while (worldData->soundCount > 0) {
                result = CZWorld::RemoveSound(node, worldData->soundNodes[0]);
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
                result = CZWorld::RemoveChildAtGrid(node, node->listB[0]);
                if (result != 0) {
                    return result;
                }
            }

            // Each grid row contains contiguous areas. Remove every area child
            // before checking whether the world node can be deleted.

            {
                zWorldAreaPartial **rowCursor = worldData->areaGridRows;
                int row = 0;
                for (; row < worldData->areaGridRowCount; ++row) {
                    zWorldAreaPartial *area = *rowCursor;
                    int col = 0;
                    if (worldData->areaGridColCount > 0) {
                        do {
                            while (area->childCount > 0) {
                                result =
                                    CZWorld::RemoveChildAtGrid(node, area->childList[0]);
                                if (result != 0) {
                                    return result;
                                }
                            }
                            ++area;
                            ++col;
                        } while (col < worldData->areaGridColCount);
                    }
                    ++rowCursor;
                }
            }

            if (node->listCountA == 0) {
                return CZWorld::DeleteNode(node);
            }
            return 1;
        }

        default:
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zClass\\List.c",
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

namespace CZList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.renderactivecameras
     * @recoil-artifact defines .text recoil:function:0x44f630: CZList::RenderActiveCameras (GameZRecoil/zClass/List.c).
     *
     * Purpose: walk the active camera bucket and render each enabled camera through
     * the current software or scene-render path.
     */
    int __cdecl RenderActiveCameras() {
        CZTypeListLink *link = CZTypeList::GetBucketHead(8);
        if (link == 0) {
            fprintf(stderr, "ERROR: No camera on camera list.\n");
            return 1;
        }

        do {
            CZNodePartial *const camera = link->node;
            CZTypeListLink *const next = link->next;

            if ((camera->flags & 4) != 0) {
                if (g_zVideo_ActiveRendererPath != 0) {
                    zVideoswRenderFrame(camera, 0);
                } else {
                    CZCamera::RenderScene(camera, 0);
                }
            }

            link = next;
        } while (link != 0);

        return 0;
    }
}

namespace CZList {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.iteratebucketfiltered
     * @recoil-artifact defines .text recoil:function:0x44f690: CZList::IterateBucketFiltered.
     * @recoil-match byte
     *
     * Purpose: initialize or continue filtered iteration over one type-list
     * bucket using a caller-supplied predicate.
     */
    CZNodePartial *__fastcall IterateBucketFiltered(
        const char *filterText,
        int bucket,
        CZNodePredicate predicate
    ) {
        if (filterText != 0) {
            g_CZClass_FilterIterText = filterText;
            g_CZClass_FilterIterCursor = CZTypeList::GetBucketHead(bucket);
            return 0;
        }

        CZTypeListLink *link = g_CZClass_FilterIterCursor;
        while (link != 0) {
            CZNodePartial *node = link->node;
            g_CZClass_FilterIterCursor = link->next;
            if (predicate(node) != 0) {
                return node;
            }
            link = g_CZClass_FilterIterCursor;
        }

        return 0;
    }
}

namespace CZClass {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.findnextbytypeprefix
     * @recoil-artifact defines .text recoil:function:0x44f6f0: CZClass::FindNextByTypePrefix.
     * @recoil-match byte
     *
     * Purpose: initialize or continue prefix search over one type-list bucket.
     */
    CZNodePartial *__fastcall FindNextByTypePrefix(
        const char *prefixText,
        int bucket
    ) {
        if (prefixText != 0) {
            g_CZClass_FilterIterPrefixLen = (int)(strlen(prefixText));
        }

        return CZList::IterateBucketFiltered(
            prefixText,
            bucket,
            FindNextByTypePrefixPredicate
        );
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.findnextbytypeprefix-predicate
     * @recoil-artifact defines .text recoil:function:0x44f720: CZClass::FindNextByTypePrefixPredicate.
     * @recoil-match byte
     *
     * Purpose: test whether a node name matches the active prefix-search text.
     */
    int __fastcall FindNextByTypePrefixPredicate(CZNodePartial * node) {
        return strncmp(
                   node->name,
                   g_CZClass_FilterIterText,
                   (size_t)(g_CZClass_FilterIterPrefixLen)
               ) == 0;
    }


    /**
     * Source-shape note: the definition is emitted by cls_util.c; List.c
     * retains callers and the public declaration.
     */

    /**
     * Source-shape note: the definition is emitted by Window.c; List.c retains
     * callers of the shared class operation.
     */
}

namespace CZClass {

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.gwnodefindnextbyname
     * @recoil-artifact defines .text recoil:function:0x44f740: CZClass::gwNodeFindNextByName.
     * @recoil-match byte
     *
     * Purpose: initialize or continue exact-name search over one type-list
     * bucket.
     */
    CZNodePartial *__fastcall gwNodeFindNextByName(
        const char *name,
        int bucket
    ) {
        return CZList::IterateBucketFiltered(name, bucket, gwNodeFindNextByNamePredicate);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.list.gwnodefindnextbyname-predicate
     * @recoil-artifact defines .text recoil:function:0x44f750: CZClass::gwNodeFindNextByNamePredicate.
     * @recoil-match byte
     *
     * Purpose: test whether a node name matches the active exact-name search
     * text.
     */
    int __fastcall gwNodeFindNextByNamePredicate(CZNodePartial * node) {
        return strcmp(node->name, g_CZClass_FilterIterText) == 0;
    }

}
