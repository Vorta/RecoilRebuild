#include "zclass.h"

#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdlib.h>

namespace {

    const int kZClassNodeLod = 6;

}

namespace CZSequence {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sequence.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-sequence-delete-node: CZSequence::DeleteNode
     * Purpose: route sequence deletion through the generic node free path.
     */
    int __fastcall DeleteNode(CZNodePartial * node) {
        return CZClass::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-gwsequencenew
     * @recoil-artifact defines .text recoil:function:0x453ee0: CZSequence::gwSequenceNew
     *
     * Purpose: allocate a sequence node, attach zeroed sequence class data,
     * seed the forward step, and register the node with the type list.
     */
    CZNodePartial *__cdecl gwSequenceNew() {
        CZNodePartial *node = CZClass::gwNodeNew();
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x41, "Null node pointer.");
            return 0;
        }

        node->classId = 7;
        CZSequenceDataPartial *data =
            (CZSequenceDataPartial *)(calloc(1, sizeof(CZSequenceDataPartial)));
        node->classData = data;
        data->step = 1;
        CZTypeList::Insert(11, node);
        return node;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-gwsequenceaddchild
     * @recoil-artifact defines .text recoil:function:0x453f40: CZSequence::gwSequenceAddChild
     *
     * Purpose: append a child node, grow the sequence entry storage, and insert
     * the child delay record at the requested sequence index.
     */
    int __fastcall gwSequenceAddChild(
        CZNodePartial * parent,
        CZNodePartial * child,
        int insertIndex,
        float delay
    ) {
        int addResult;
        CZSequenceDataPartial *data;
        int entryCount;
        int i;

        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x94, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x95, "Null node pointer.");
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x96, "Null class data pointer");
            return 5;
        }

        addResult = CZClass::AddChildGeneric(parent, child);
        if (addResult != 0) {
            return addResult;
        }

        data = (CZSequenceDataPartial *)(parent->classData);
        data = (CZSequenceDataPartial *)(realloc(
            data,
            data->entryCount * sizeof(CZSequenceEntryPartial) +
                sizeof(CZSequenceDataPartial)
        ));
        parent->classData = data;

        entryCount = data->entryCount + 1;
        data->entryCount = entryCount;
        for (i = entryCount - 1; i > insertIndex; --i) {
            data->entries[i] = data->entries[i - 1];
        }

        data->entries[insertIndex].node = child;
        data->entries[insertIndex].triggerTime = delay;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-removechild
     * @recoil-artifact defines .text recoil:function:0x454000: CZSequence::RemoveChild
     *
     * Purpose: remove a child from both the zClass child list and the sequence
     * entry list, then clamp the active index back to the first entry if needed.
     */
    RemoveChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0xd3, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0xd4, "Null node pointer.");
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0xd5, "Null class data pointer");
            return 5;
        }

        const int removeResult = CZClass::RemoveChildGeneric(parent, child);
        if (removeResult != 0) {
            return removeResult;
        }

        CZSequenceDataPartial *data = (CZSequenceDataPartial *)(parent->classData);
        int childIndex = -1;
        for (int i = 0; i < data->entryCount; ++i) {
            if (data->entries[i].node == child) {
                childIndex = i;
                break;
            }
        }

        if (childIndex >= 0) {
            for (int i = childIndex; i < data->entryCount - 1; ++i) {
                data->entries[i] = data->entries[i + 1];
            }
            --data->entryCount;
        }

        if (data->currentIndex >= data->entryCount) {
            data->currentIndex = 0;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-setactive
     * @recoil-artifact defines .text recoil:function:0x4540c0: CZSequence::SetActive
     *
     * Purpose: set whether the sequence advances and renders its active child.
     */
    int __fastcall SetActive(
        CZNodePartial * node,
        int active
    ) {
        CZSequenceDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x113, "Null node pointer.");
            return 5;
        }

        data = (CZSequenceDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x114, "Null class data pointer");
            return 5;
        }

        data->isActive = active;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-setrepeat
     * @recoil-artifact defines .text recoil:function:0x454100: CZSequence::SetRepeat
     *
     * Purpose: set whether the sequence remains active when traversal reaches
     * either end of the entry list.
     */
    int __fastcall SetRepeat(
        CZNodePartial * node,
        int repeat
    ) {
        CZSequenceDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x133, "Null node pointer.");
            return 5;
        }

        data = (CZSequenceDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x134, "Null class data pointer");
            return 5;
        }

        data->repeatAtBounds = repeat;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-setloop
     * @recoil-artifact defines .text recoil:function:0x454140: CZSequence::SetLoop
     *
     * Purpose: set whether sequence traversal wraps at the entry-list bounds
     * instead of reversing direction.
     */
    int __fastcall SetLoop(
        CZNodePartial * node,
        int loop
    ) {
        CZSequenceDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x153, "Null node pointer.");
            return 5;
        }

        data = (CZSequenceDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x154, "Null class data pointer");
            return 5;
        }

        data->wrapAtBounds = loop;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-setpause
     * @recoil-artifact defines .text recoil:function:0x454180: CZSequence::SetPause
     *
     * Purpose: set the pause flag that suppresses time advancement while
     * keeping the sequence active state unchanged.
     */
    int __fastcall SetPause(
        CZNodePartial * node,
        int paused
    ) {
        CZSequenceDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x173, "Null node pointer.");
            return 5;
        }

        data = (CZSequenceDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x174, "Null class data pointer");
            return 5;
        }

        data->isPaused = paused;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-update
     * @recoil-artifact defines .text recoil:function:0x4541c0: CZSequence::Update
     *
     * Purpose: accumulate frame time and advance the active sequence entry,
     * applying repeat, wrap, and direction-reversal behavior at the bounds.
     */
    int __fastcall Update(CZNodePartial * node) {
        CZSequenceDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x193, "Null node pointer.");
            return 5;
        }

        data = (CZSequenceDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Seq.c", 0x194, "Null class data pointer");
            return 5;
        }

        if (data->isPaused != 0 || data->isActive == 0) {
            return 0;
        }

        data->currentTime += g_FrameDeltaTimeSec;
        int currentIndex = data->currentIndex;
        if (data->currentTime <= data->entries[currentIndex].triggerTime) {
            return 0;
        }

        const int entryCount = data->entryCount;
        do {
            data->currentTime -= data->entries[currentIndex].triggerTime;

            const int step = data->step;
            currentIndex += step;
            data->currentIndex = currentIndex;

            if (currentIndex >= entryCount) {
                if (data->wrapAtBounds != 0) {
                    data->currentIndex = 0;
                } else {
                    data->currentIndex = entryCount - 1;
                    data->step = -step;
                }

                if (data->repeatAtBounds == 0) {
                    data->isActive = 0;
                }
            } else if (currentIndex < 0) {
                if (data->wrapAtBounds != 0) {
                    data->currentIndex = entryCount - 1;
                } else {
                    data->currentIndex = 0;
                    data->step = -step;
                }

                if (data->repeatAtBounds == 0) {
                    data->isActive = 0;
                }
            }

            currentIndex = data->currentIndex;
        } while (data->currentTime > data->entries[currentIndex].triggerTime);

        return 0;
    }

}

namespace CZLod {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.lod.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-lod-delete-node: CZLod::DeleteNode
     * Purpose: route LOD deletion through the generic node free path.
     */
    int __fastcall DeleteNode(CZNodePartial * node) {
        return CZClass::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-lod-gwlodnew
     * @recoil-artifact defines .text recoil:function:0x4542a0: CZLod::gwLodNew.
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     *
     * Purpose: allocate an LOD node, attach zeroed LOD class data, and seed the
     * original default range and active-distance settings.
     */
    CZNodePartial *__cdecl gwLodNew() {
        CZNodePartial *node = CZClass::gwNodeNew();
        node->classId = kZClassNodeLod;

        CZLodDataPartial *data =
            (CZLodDataPartial *)(calloc(1, sizeof(CZLodDataPartial)));
        node->classData = data;
        data->computeOwnDistance = 1;
        data->nearRange = 1000.0f;
        data->farRangeSq = 1000000.0f;
        data->active = 1;
        return node;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-lod-gwlodaddchild
     * @recoil-artifact defines .text recoil:function:0x454310: CZLod::gwLodAddChild.
     * @recoil-match byte
     *
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     * Purpose: append a child to an LOD node using the shared zClass child-list
     * helper.
     */
    gwLodAddChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        return CZClass::AddChildGeneric(parent, child);
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-lod-removechild
     * @recoil-artifact defines .text recoil:function:0x454320: CZLod::RemoveChild.
     * @recoil-match byte
     *
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     * Purpose: remove a child from an LOD node through the shared zClass
     * child-list helper and return success.
     */
    RemoveChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        CZClass::RemoveChildGeneric(parent, child);
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-lod-setcomputeowndistance
     * @recoil-artifact defines .text recoil:function:0x454330: CZLod::SetComputeOwnDistance.
     * @recoil-match byte
     *
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     * Purpose: update whether this LOD node computes its own camera distance
     * during render traversal.
     */
    SetComputeOwnDistance(
        CZNodePartial * node,
        int enabled
    ) {
        ((CZLodDataPartial *)(node->classData))->computeOwnDistance = enabled;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-lod-settargetnodeandrange
     * @recoil-artifact defines .text recoil:function:0x454340: CZLod::SetTargetNodeAndRange.
     * @recoil-match byte
     *
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     * Purpose: assign the range-fade target node and cache the squared fade
     * range when a target is present.
     */
    SetTargetNodeAndRange(
        CZNodePartial * node,
        CZNodePartial * target,
        float range
    ) {
        CZLodDataPartial *data = (CZLodDataPartial *)(node->classData);
        data->rangeNode = target;
        if (target != 0) {
            data->rangeSq = range * range;
        }

        return 0;
    }
}
