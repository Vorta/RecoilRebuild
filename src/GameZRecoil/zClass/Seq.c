#include "zClass.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <stdlib.h>

namespace {
    const char *kSequenceSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\Seq.c";

    int CullNodeForRender(zClass_NodePartial *node, int siblingCountHint,
                                   int *clipMask) {
        int result = 0;
        if (*clipMask != 0 && siblingCountHint > 1) {
            if ((node->boundsFlags & 0x04) != 0 || g_zClass_RenderBoundsContextActive != 0) {
                zBBoxCorners corners = {0};
                zClass_Class::gwNodeGetViewBBoxCorners(node, &corners);
                BBox::CornersToBoundingSphere(&corners, zClass_NodeViewSphereCenter(node),
                                              zClass_NodeViewSphereRadius(node));
                node->boundsFlags &= ~0x04;
            }
            result = zVideo_FrustumTestSphereClipMask(zClass_NodeViewSphereCenter(node), clipMask,
                                                      *zClass_NodeViewSphereRadius(node));
            if ((node->flags & 0x80) != 0 && result == 0x20) {
                result = 0;
                *clipMask &= ~0x20;
            }
        }
        return result;
    }
}

namespace zClass_Sequence {
    // Reimplements 0x453ee0: zClass_Sequence::gwSequenceNew
    // (GameZRecoil/zClass/Seq.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwSequenceNew() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            zError::ReportOld(0x400, kSequenceSourceFile, 0x41, "Null node pointer.");
            return 0;
        }

        node->classId = 7;
        zClass_SequenceDataPartial *data = (zClass_SequenceDataPartial *)(
            calloc(1, sizeof(zClass_SequenceDataPartial)));
        node->classData = data;
        data->step = 1;
        zClass_TypeList::Insert(11, node);
        return node;
    }

    // Reimplements 0x453f40: zClass_Sequence::gwSequenceAddChild
    // (GameZRecoil/zClass/Seq.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwSequenceAddChild(
        zClass_NodePartial * parent, zClass_NodePartial * child, int insertIndex,
        float delay) {
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

        addResult = zClass_Class::AddChildGeneric(parent, child);
        if (addResult != 0) {
            return addResult;
        }

        data = (zClass_SequenceDataPartial *)(parent->classData);
        data = (zClass_SequenceDataPartial *)(
            realloc(data, data->entryCount * sizeof(zClass_SequenceEntryPartial) +
                                   sizeof(zClass_SequenceDataPartial)));
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
        zError::ReportOld(0x400, kSequenceSourceFile, line, message);
        return 5;
    }

    // Reimplements 0x4540c0: zClass_Sequence::SetActive
    // (GameZRecoil/zClass/Seq.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetActive(zClass_NodePartial * node,
                                                           int active) {
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
        zError::ReportOld(0x400, kSequenceSourceFile, line, message);
        return 5;
    }

    // Reimplements 0x454100: zClass_Sequence::SetRepeat
    // (GameZRecoil/zClass/Seq.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetRepeat(zClass_NodePartial * node,
                                                           int repeat) {
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
        zError::ReportOld(0x400, kSequenceSourceFile, line, message);
        return 5;
    }

    // Reimplements 0x454140: zClass_Sequence::SetLoop
    // (GameZRecoil/zClass/Seq.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetLoop(zClass_NodePartial * node,
                                                         int loop) {
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
        zError::ReportOld(0x400, kSequenceSourceFile, line, message);
        return 5;
    }

    // Reimplements 0x454180: zClass_Sequence::SetPause
    // (GameZRecoil/zClass/Seq.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetPause(zClass_NodePartial * node,
                                                          int paused) {
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
        zError::ReportOld(0x400, kSequenceSourceFile, line, message);
        return 5;
    }

    // Reimplements 0x454000: zClass_Sequence::RemoveChild
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial * parent,
                                                             zClass_NodePartial * child) {
        if (parent == 0) {
            zError::ReportOld(0x400, kSequenceSourceFile, 0xd3, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, kSequenceSourceFile, 0xd4, "Null node pointer.");
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(0x400, kSequenceSourceFile, 0xd5, "Null class data pointer");
            return 5;
        }

        const int removeResult = zClass_Class::RemoveChildGeneric(parent, child);
        if (removeResult != 0) {
            return removeResult;
        }

        zClass_SequenceDataPartial *data =
            (zClass_SequenceDataPartial *)(parent->classData);
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

    // Reimplements 0x4541c0: zClass_Sequence::Update (GameZRecoil/zClass/Seq.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL Update(zClass_NodePartial * node) {
        zClass_SequenceDataPartial *data;

        if (node == 0) {
            zError::ReportOld(0x400, kSequenceSourceFile, 0x193, "Null node pointer.");
            return 5;
        }

        data = (zClass_SequenceDataPartial *)(node->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kSequenceSourceFile, 0x194, "Null class data pointer");
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

    // Reimplements 0x44bea0: zClass_Sequence::RenderTraverse
    // (GameZRecoil/zClass/Seq.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(
        zClass_NodePartial *node, int siblingCountHint) {
        int boundsContextPushed = 0;
        zClass_SequenceDataPartial *data;
        const int flags = node->flags;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        data = (zClass_SequenceDataPartial *)(node->classData);
        node->flags = flags & ~0x02000000;
        if (data->isActive == 0) {
            return 0;
        }

        int clipMask = *gModel_ClipMaskStackTop;
        const int result = CullNodeForRender(node, siblingCountHint, &clipMask);
        if (g_zClass_RenderBoundsContextActive == 0) {
            boundsContextPushed = 1;
            g_zClass_RenderBoundsContextActive = 1;
        }

        if (result == 0) {
            node->flags |= 0x80000000;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;
            zClass_Class::gwNodeRenderDispatch(data->entries[data->currentIndex].node,
                                               node->listCountB);
            --gModel_ClipMaskStackTop;
        }

        if (boundsContextPushed != 0) {
            g_zClass_RenderBoundsContextActive = 0;
        }
        return result;
    }
}
