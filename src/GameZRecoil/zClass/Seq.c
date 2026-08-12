#include "zclass.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <stdlib.h>

namespace {
    const char kSequenceSourceFile[] = "D:\\Proj\\GameZRecoil\\zClass\\Seq.c";
    const int kZClassNodeLod = 6;

}

namespace zClass_Sequence {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sequence.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-sequence-delete-node: zClass_Sequence::DeleteNode
     * Purpose: route sequence deletion through the generic node free path.
     */
    int __fastcall DeleteNode(zClass_NodePartial * node) {
        return zClass_Class::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-gwsequencenew
     * @recoil-artifact defines .text recoil:function:0x453ee0: zClass_Sequence::gwSequenceNew
     *
     * Purpose: allocate a sequence node, attach zeroed sequence class data,
     * seed the forward step, and register the node with the type list.
     */
    zClass_NodePartial *__cdecl gwSequenceNew() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            zError::ReportOld(
                0x400,
                kSequenceSourceFile,
                0x41,
                "Null node pointer."
            );
            return 0;
        }

        node->classId = 7;
        zClass_SequenceDataPartial *data =
            (zClass_SequenceDataPartial *)(calloc(
                1,
                sizeof(zClass_SequenceDataPartial)
            ));
        node->classData = data;
        data->step = 1;
        zClass_TypeList::Insert(
            11,
            node
        );
        return node;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-gwsequenceaddchild
     * @recoil-artifact defines .text recoil:function:0x453f40: zClass_Sequence::gwSequenceAddChild
     *
     * Purpose: append a child node, grow the sequence entry storage, and insert
     * the child delay record at the requested sequence index.
     */
    int __fastcall gwSequenceAddChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child,
        int insertIndex,
        float delay
    ) {
        const char *message;
        int line;
        int addResult;
        zClass_SequenceDataPartial *data;
        int entryCount;
        int i;

        if (parent == 0) {
            line = 0x94;
            message = "Null node pointer.";
            goto reportError;
        }
        if (child == 0) {
            line = 0x95;
            message = "Null node pointer.";
            goto reportError;
        }
        if (parent->classData == 0) {
            line = 0x96;
            message = "Null class data pointer";
            goto reportError;
        }

        addResult = zClass_Class::AddChildGeneric(
            parent,
            child
        );
        if (addResult != 0) {
            return addResult;
        }

        data = (zClass_SequenceDataPartial *)(parent->classData);
        data = (zClass_SequenceDataPartial *)(realloc(
            data,
            data->entryCount * sizeof(zClass_SequenceEntryPartial) +
                sizeof(zClass_SequenceDataPartial)
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

    reportError:
        zError::ReportOld(
            0x400,
            kSequenceSourceFile,
            line,
            message
        );
        return 5;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-removechild
     * @recoil-artifact defines .text recoil:function:0x454000: zClass_Sequence::RemoveChild
     *
     * Purpose: remove a child from both the zClass child list and the sequence
     * entry list, then clamp the active index back to the first entry if needed.
     */
    RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(
                0x400,
                kSequenceSourceFile,
                0xd3,
                "Null node pointer."
            );
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(
                0x400,
                kSequenceSourceFile,
                0xd4,
                "Null node pointer."
            );
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(
                0x400,
                kSequenceSourceFile,
                0xd5,
                "Null class data pointer"
            );
            return 5;
        }

        const int removeResult = zClass_Class::RemoveChildGeneric(
            parent,
            child
        );
        if (removeResult != 0) {
            return removeResult;
        }

        zClass_SequenceDataPartial *data = (zClass_SequenceDataPartial *)(parent->classData);
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
     * @recoil-artifact defines .text recoil:function:0x4540c0: zClass_Sequence::SetActive
     *
     * Purpose: set whether the sequence advances and renders its active child.
     */
    int __fastcall SetActive(
        zClass_NodePartial * node,
        int active
    ) {
        const char *message;
        int line;
        zClass_SequenceDataPartial *data;

        if (node == 0) {
            line = 0x113;
            message = "Null node pointer.";
            goto reportError;
        }

        data = (zClass_SequenceDataPartial *)(node->classData);
        if (data == 0) {
            line = 0x114;
            message = "Null class data pointer";
            goto reportError;
        }

        data->isActive = active;
        return 0;

    reportError:
        zError::ReportOld(
            0x400,
            kSequenceSourceFile,
            line,
            message
        );
        return 5;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-setrepeat
     * @recoil-artifact defines .text recoil:function:0x454100: zClass_Sequence::SetRepeat
     *
     * Purpose: set whether the sequence remains active when traversal reaches
     * either end of the entry list.
     */
    int __fastcall SetRepeat(
        zClass_NodePartial * node,
        int repeat
    ) {
        const char *message;
        int line;
        zClass_SequenceDataPartial *data;

        if (node == 0) {
            line = 0x133;
            message = "Null node pointer.";
            goto reportError;
        }

        data = (zClass_SequenceDataPartial *)(node->classData);
        if (data == 0) {
            line = 0x134;
            message = "Null class data pointer";
            goto reportError;
        }

        data->repeatAtBounds = repeat;
        return 0;

    reportError:
        zError::ReportOld(
            0x400,
            kSequenceSourceFile,
            line,
            message
        );
        return 5;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-setloop
     * @recoil-artifact defines .text recoil:function:0x454140: zClass_Sequence::SetLoop
     *
     * Purpose: set whether sequence traversal wraps at the entry-list bounds
     * instead of reversing direction.
     */
    int __fastcall SetLoop(
        zClass_NodePartial * node,
        int loop
    ) {
        const char *message;
        int line;
        zClass_SequenceDataPartial *data;

        if (node == 0) {
            line = 0x153;
            message = "Null node pointer.";
            goto reportError;
        }

        data = (zClass_SequenceDataPartial *)(node->classData);
        if (data == 0) {
            line = 0x154;
            message = "Null class data pointer";
            goto reportError;
        }

        data->wrapAtBounds = loop;
        return 0;

    reportError:
        zError::ReportOld(
            0x400,
            kSequenceSourceFile,
            line,
            message
        );
        return 5;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-setpause
     * @recoil-artifact defines .text recoil:function:0x454180: zClass_Sequence::SetPause
     *
     * Purpose: set the pause flag that suppresses time advancement while
     * keeping the sequence active state unchanged.
     */
    int __fastcall SetPause(
        zClass_NodePartial * node,
        int paused
    ) {
        const char *message;
        int line;
        zClass_SequenceDataPartial *data;

        if (node == 0) {
            line = 0x173;
            message = "Null node pointer.";
            goto reportError;
        }

        data = (zClass_SequenceDataPartial *)(node->classData);
        if (data == 0) {
            line = 0x174;
            message = "Null class data pointer";
            goto reportError;
        }

        data->isPaused = paused;
        return 0;

    reportError:
        zError::ReportOld(
            0x400,
            kSequenceSourceFile,
            line,
            message
        );
        return 5;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-sequence-update
     * @recoil-artifact defines .text recoil:function:0x4541c0: zClass_Sequence::Update
     *
     * Purpose: accumulate frame time and advance the active sequence entry,
     * applying repeat, wrap, and direction-reversal behavior at the bounds.
     */
    int __fastcall Update(zClass_NodePartial * node) {
        zClass_SequenceDataPartial *data;

        if (node == 0) {
            zError::ReportOld(
                0x400,
                kSequenceSourceFile,
                0x193,
                "Null node pointer."
            );
            return 5;
        }

        data = (zClass_SequenceDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(
                0x400,
                kSequenceSourceFile,
                0x194,
                "Null class data pointer"
            );
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

namespace zClass_Lod {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.lod.deletenode
     * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-lod-delete-node: zClass_Lod::DeleteNode
     * Purpose: route LOD deletion through the generic node free path.
     */
    int __fastcall DeleteNode(zClass_NodePartial * node) {
        return zClass_Class::TryFreeNode(node);
    }

    /**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-lod-gwlodnew
 * @recoil-artifact defines .text recoil:function:0x4542a0: zClass_Lod::gwLodNew.
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     *
     * Purpose: allocate an LOD node, attach zeroed LOD class data, and seed the
     * original default range and active-distance settings.
     */
    zClass_NodePartial *__cdecl gwLodNew() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        node->classId = kZClassNodeLod;

        zClass_LodDataPartial *data =
            (zClass_LodDataPartial *)(calloc(
                1,
                sizeof(zClass_LodDataPartial)
            ));
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
 * @recoil-artifact defines .text recoil:function:0x454310: zClass_Lod::gwLodAddChild.
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     *
     * Purpose: append a child to an LOD node using the shared zClass child-list
     * helper.
     */
    gwLodAddChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        return zClass_Class::AddChildGeneric(
            parent,
            child
        );
    }

    int __fastcall
    /**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-lod-removechild
 * @recoil-artifact defines .text recoil:function:0x454320: zClass_Lod::RemoveChild.
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     *
     * Purpose: remove a child from an LOD node through the shared zClass
     * child-list helper and return success.
     */
    RemoveChild(
        zClass_NodePartial * parent,
        zClass_NodePartial * child
    ) {
        zClass_Class::RemoveChildGeneric(
            parent,
            child
        );
        return 0;
    }

    int __fastcall
    /**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-lod-setcomputeowndistance
 * @recoil-artifact defines .text recoil:function:0x454330: zClass_Lod::SetComputeOwnDistance.
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     *
     * Purpose: update whether this LOD node computes its own camera distance
     * during render traversal.
     */
    SetComputeOwnDistance(
        zClass_NodePartial * node,
        int enabled
    ) {
        ((zClass_LodDataPartial *)(node->classData))->computeOwnDistance = enabled;
        return 0;
    }

    int __fastcall
    /**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.seq.zclass-lod-settargetnodeandrange
 * @recoil-artifact defines .text recoil:function:0x454340: zClass_Lod::SetTargetNodeAndRange.
     * The original implementation translation unit is unresolved; Seq.c is
     * the provisional current compile host.
     *
     * Purpose: assign the range-fade target node and cache the squared fade
     * range when a target is present.
     */
    SetTargetNodeAndRange(
        zClass_NodePartial * node,
        zClass_NodePartial * target,
        float range
    ) {
        zClass_LodDataPartial *data = (zClass_LodDataPartial *)(node->classData);
        data->rangeNode = target;
        if (target != 0) {
            data->rangeSq = range * range;
        }

        return 0;
    }
}
