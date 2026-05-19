#include "zClass.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "zDi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
zClass_NodeFreeListSlot *g_zClass_NodeArray = 0;
int g_zClass_ActiveNodeCount = 0;
int g_zClass_NodeFreeHeadIndex = 0;
char g_zClass_CurrentZbdPath[260] = {0};
zClass_NodePartial *g_MainCamera = 0;
zClass_NodePartial *g_Player_RuntimeDiScene = 0;
int gModel_ClipMaskStack[0x40] = {0};
int *gModel_ClipMaskStackTop = gModel_ClipMaskStack;
zClass_RenderFn gModel_RenderFn = 0;
int g_zClass_RenderBoundsContextActive = 0;
int g_zClass_RenderFrustumGridTileIndex = 0;
int g_zClass_RenderRangeFadeActive = 0;
float g_zClass_RenderRangeFadeScale = 0.0f;
int g_zClass_RenderVertexAlphaOverrideActive = 0;
int g_zClass_RenderAlphaScaleStackTop = -1;
float g_zClass_RenderAlphaScaleStack[0x20] = {0};
int g_zClass_SoftwarePathStateStackTop = -1;
zClass_RenderColorAlphaState g_zClass_SoftwarePathRenderStateStack[0x20] = {0};
int g_zClass_LodDistanceStateStackTop = 0;
zClass_LodDistanceState g_zClass_LodDistanceStateStack[0x20] = {0};
}

namespace {
    const char *kClassSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\Class.c";
    const int kQueuedTreeBucket = 7;
    const int kTypeListInsertedFlag = 0x01;
    const int kTransformQueuedFlag = 0x02;
    const int kBoundsDirtyFlag = 0x02;
    const int kSingleParentFlag = 0x00080000;
    const int kNodeTransformDirtyPropagatedFlag = 0x02000000;
    const char *kSwitchSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\Switch.c";

    bool ReportNullNode(int sourceLine, zClass_NodePartial *node) {
        if (node != 0) {
            return false;
        }

        zError::ReportOld(0x400, kClassSourceFile, sourceLine, "Null node pointer.");
        return true;
    }

    bool IsCallbackPriorityValid(int priority) {
        return priority >= 0 && priority < 6;
    }

    int SetActionCallbackCommon(zClass_NodePartial * node, void *actionCallback,
                                         bool appendToTail, int nullLine, int priorityLine) {
        if (ReportNullNode(nullLine, node)) {
            return 5;
        }
        if (!IsCallbackPriorityValid(node->callbackPriority)) {
            zError::ReportOld(0x400, kClassSourceFile, priorityLine,
                              "Action callback priority out of range: %d", node->callbackPriority);
            return 1;
        }

        if (node->actionCallback == 0 && actionCallback != 0) {
            if (appendToTail) {
                zClass_TypeList::InsertChildNodes(node->callbackPriority, node);
            } else {
                zClass_TypeList::Insert(node->callbackPriority, node);
            }
        } else if (node->actionCallback != 0 && actionCallback == 0) {
            zClass_TypeList::MarkPendingRemoval(node->callbackPriority, node);
        }

        node->actionCallback = actionCallback;
        return 0;
    }

    int ValidateParentChildForSwitch(zClass_NodePartial * parent,
                                              zClass_NodePartial * child, int nullParentLine,
                                              int nullChildLine, int nullClassDataLine) {
        if (parent == 0) {
            zError::ReportOld(0x400, kSwitchSourceFile, nullParentLine, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, kSwitchSourceFile, nullChildLine, "Null node pointer.");
            return 5;
        }
        if (parent->classData == 0) {
            zError::ReportOld(0x400, kSwitchSourceFile, nullClassDataLine,
                              "Null class data pointer");
            return 5;
        }

        return 0;
    }

    zMat4x3 *MatrixAt(void *base, size_t offset) {
        return (zMat4x3 *)(static_cast<unsigned char *>(base) + offset);
    }

    const zMat4x3 *MatrixAt(const void *base, size_t offset) {
        return (const zMat4x3 *)(static_cast<const unsigned char *>(base) + offset);
    }

    zVec3 *Vec3At(void *base, size_t offset) {
        return (zVec3 *)(static_cast<unsigned char *>(base) + offset);
    }

    const zVec3 *Vec3At(const void *base, size_t offset) {
        return (const zVec3 *)(static_cast<const unsigned char *>(base) + offset);
    }

    void CopyCurrentMatrixTo(float *outMatrix) {
        zMat4x3 out = {0};
        zMath::MatCopyCurrentTo(&out);
        memcpy(outMatrix, &out, sizeof(out));
    }

    zVec3 UnitScale() {
        zVec3 result = {1.0f, 1.0f, 1.0f};
        return result;
    }

    const zBBox3f *CachedBBox(const zClass_NodePartial *node) {
        return (const zBBox3f *)(node->cachedBounds);
    }

    zBBox3f *SecondaryBBox(zClass_NodePartial * node) {
        return (zBBox3f *)((unsigned char *)(node) + 0xa4);
    }

    zBBox3f *PrimaryBBox(zClass_NodePartial * node) {
        return (zBBox3f *)((unsigned char *)(node) + 0x8c);
    }

    const zBBox3f *PrimaryBBox(const zClass_NodePartial *node) {
        return (const zBBox3f *)((const unsigned char *)(node) +
                                                 0x8c);
    }

    const zBBox3f *SecondaryBBox(const zClass_NodePartial *node) {
        return (const zBBox3f *)((const unsigned char *)(node) +
                                                 0xa4);
    }

    void CopyBBoxToCorners(const zBBox3f *bbox, zBBoxCorners *outCorners) {
        float *out = outCorners->values;
        out[0] = bbox->minX;
        out[1] = bbox->minY;
        out[2] = bbox->maxZ;
        out[3] = bbox->maxX;
        out[4] = bbox->minY;
        out[5] = bbox->maxZ;
        out[6] = bbox->maxX;
        out[7] = bbox->minY;
        out[8] = bbox->minZ;
        out[9] = bbox->minX;
        out[10] = bbox->minY;
        out[11] = bbox->minZ;
        out[12] = bbox->minX;
        out[13] = bbox->maxY;
        out[14] = bbox->maxZ;
        out[15] = bbox->maxX;
        out[16] = bbox->maxY;
        out[17] = bbox->maxZ;
        out[18] = bbox->maxX;
        out[19] = bbox->maxY;
        out[20] = bbox->minZ;
        out[21] = bbox->minX;
        out[22] = bbox->maxY;
        out[23] = bbox->minZ;
    }

    void MultiplyMatricesForViewBBox(const zMat4x3 *currentMatrix, const zMat4x3 *nodeMatrix,
                                     zMat4x3 *outMatrix) {
        outMatrix->xx = currentMatrix->xx * nodeMatrix->xx + currentMatrix->yx * nodeMatrix->xy +
                        currentMatrix->zx * nodeMatrix->xz;
        outMatrix->yx = currentMatrix->xx * nodeMatrix->yx + currentMatrix->yx * nodeMatrix->yy +
                        currentMatrix->zx * nodeMatrix->yz;
        outMatrix->zx = currentMatrix->xx * nodeMatrix->zx + currentMatrix->yx * nodeMatrix->zy +
                        currentMatrix->zx * nodeMatrix->zz;
        outMatrix->xy = currentMatrix->xy * nodeMatrix->xx + currentMatrix->yy * nodeMatrix->xy +
                        currentMatrix->zy * nodeMatrix->xz;
        outMatrix->yy = currentMatrix->xy * nodeMatrix->yx + currentMatrix->yy * nodeMatrix->yy +
                        currentMatrix->zy * nodeMatrix->yz;
        outMatrix->zy = currentMatrix->xy * nodeMatrix->zx + currentMatrix->yy * nodeMatrix->zy +
                        currentMatrix->zy * nodeMatrix->zz;
        outMatrix->xz = currentMatrix->xz * nodeMatrix->xx + currentMatrix->yz * nodeMatrix->xy +
                        currentMatrix->zz * nodeMatrix->xz;
        outMatrix->yz = currentMatrix->xz * nodeMatrix->yx + currentMatrix->yz * nodeMatrix->yy +
                        currentMatrix->zz * nodeMatrix->yz;
        outMatrix->zz = currentMatrix->xz * nodeMatrix->zx + currentMatrix->yz * nodeMatrix->zy +
                        currentMatrix->zz * nodeMatrix->zz;
        outMatrix->posX = currentMatrix->xx * nodeMatrix->posX +
                          currentMatrix->yx * nodeMatrix->posY +
                          currentMatrix->zx * nodeMatrix->posZ + currentMatrix->posX;
        outMatrix->posY = currentMatrix->xy * nodeMatrix->posX +
                          currentMatrix->yy * nodeMatrix->posY +
                          currentMatrix->zy * nodeMatrix->posZ + currentMatrix->posY;
        outMatrix->posZ = currentMatrix->xz * nodeMatrix->posX +
                          currentMatrix->yz * nodeMatrix->posY +
                          currentMatrix->zz * nodeMatrix->posZ + currentMatrix->posZ;
    }

    void ExpandBBoxWithCorner(zBBox3f * bbox, const float *corner) {
        if (corner[0] < bbox->minX) {
            bbox->minX = corner[0];
        } else if (corner[0] > bbox->maxX) {
            bbox->maxX = corner[0];
        }
        if (corner[1] < bbox->minY) {
            bbox->minY = corner[1];
        } else if (corner[1] > bbox->maxY) {
            bbox->maxY = corner[1];
        }
        if (corner[2] < bbox->minZ) {
            bbox->minZ = corner[2];
        } else if (corner[2] > bbox->maxZ) {
            bbox->maxZ = corner[2];
        }
    }

    zBBox3f MergeBBoxes(const zBBox3f *a, const zBBox3f *b) {
        zBBox3f merged = {0};
        merged.minX = a->minX < b->minX ? a->minX : b->minX;
        merged.minY = a->minY < b->minY ? a->minY : b->minY;
        merged.minZ = a->minZ < b->minZ ? a->minZ : b->minZ;
        merged.maxX = a->maxX > b->maxX ? a->maxX : b->maxX;
        merged.maxY = a->maxY > b->maxY ? a->maxY : b->maxY;
        merged.maxZ = a->maxZ > b->maxZ ? a->maxZ : b->maxZ;
        return merged;
    }

    void CopyBBoxToCachedBounds(zClass_NodePartial * node, const zBBox3f *bbox) {
        memcpy(node->cachedBounds, bbox, sizeof(*bbox));
    }

    void ComputeXZRectFromCorners(zClass_NodePartial * node, float *outMinX, float *outMaxX,
                                  float *outMinZ, float *outMaxZ) {
        zBBoxCorners corners = {0};
        zClass_Class::gwNodeGetWorldBBoxCorners(node, &corners);
        *outMinX = corners.values[0];
        *outMaxX = corners.values[0];
        *outMinZ = corners.values[2];
        *outMaxZ = corners.values[2];
        for (int i = 1; i < 8; ++i) {
            const float *corner = &corners.values[i * 3];
            if (corner[0] < *outMinX) {
                *outMinX = corner[0];
            } else if (corner[0] > *outMaxX) {
                *outMaxX = corner[0];
            }
            if (corner[2] < *outMinZ) {
                *outMinZ = corner[2];
            } else if (corner[2] > *outMaxZ) {
                *outMaxZ = corner[2];
            }
        }
    }
}

namespace zClass_Class {
    // Reimplements 0x4478c0: zClass_Class::AllocNodeFromFreeList
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL AllocNodeFromFreeList() {
        const int index = g_zClass_NodeFreeHeadIndex;
        if (index == -1) {
            zError::ReportOld(0x400, kClassSourceFile, 0x1ed,
                              "gwNodeNew(): GameZ node buffer is full:\n");
            return 0;
        }

        zClass_NodeFreeListSlot *slot = &g_zClass_NodeArray[index];
        g_zClass_NodeFreeHeadIndex = static_cast<int>(slot->freeTag << 8) >> 8;

        memset(&slot->node, 0, 0xc0);
        ++g_zClass_ActiveNodeCount;
        zClass_TypeList::Insert(6, &slot->node);

        slot->node.flags = 0x0108001c;
        slot->node.callbackPriority = 1;
        slot->node.gridCol = -1;
        slot->node.gridRow = -1;
        slot->node.nodeType = 0xff;
        sprintf(slot->node.name, "%s", "Default_node_name");
        slot->damageHandler = 0;
        return &slot->node;
    }

    // Reimplements 0x448cc0: zClass_Class::gwNodeUpdate
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeUpdate(zClass_NodePartial * node) {
        int result = 0;
        bool needsBBoxRecalc = false;
        const zVec3 unitScale = UnitScale();

        if ((node->boundsFlags & 0x01) != 0) {
            gwNodeUpdateDisplayInstance(node);
            needsBBoxRecalc = true;
        }
        if ((node->boundsFlags & 0x02) != 0) {
            gwNodeComputeChildBBox(node);
            needsBBoxRecalc = true;
        }
        node->boundsFlags &= 0x04;

        switch (node->classId) {
        case 1: {
            zClass_CameraDataPartial *cameraData =
                static_cast<zClass_CameraDataPartial *>(node->classData);
            if (cameraData != 0 && (cameraData->cameraFlags & 0x04) != 0) {
                if ((cameraData->cameraFlags & 0x02) == 0) {
                    zMath::MatStackPushPtr((float *)(MatrixAt(cameraData, 0x80)));
                    zMath::MatLoadIdentity();
                    zMath::MatApplyLocalTRS(&cameraData->posOffset, &cameraData->targetOrEuler,
                                            &unitScale);
                    zMath::MatStackPopPtr();
                }
                gwNodeRecalcBBox(node);
                cameraData->cameraFlags &= ~0x04;
                needsBBoxRecalc = false;
            }
            break;
        }
        case 2:
            zClass_World::ApplyPendingFogSettings(node);
            break;
        case 5: {
            zClass_Object3DDataPartial *objectData =
                static_cast<zClass_Object3DDataPartial *>(node->classData);
            if (objectData != 0 && (objectData->flags & 0x01) != 0) {
                if ((objectData->flags & 0x10) == 0) {
                    zMath::MatStackPushPtr(objectData->localMatrix);
                    zMath::MatLoadIdentity();
                    zMath::MatApplyLocalTRS(&objectData->rotation,
                                            (zVec3 *)(&objectData->localMatrix[9]),
                                            &objectData->scale);
                    zMath::MatStackPopPtr();
                }
                gwNodeRecalcBBox(node);
                objectData->flags &= ~0x01;
                needsBBoxRecalc = false;
            }
            break;
        }
        case 6:
        case 7:
            break;
        case 8: {
            zClass_AnimateDataPartial *animateData =
                static_cast<zClass_AnimateDataPartial *>(node->classData);
            if ((node->flags & 0x04) != 0 && (animateData->statusFlags & 0x04) != 0 &&
                animateData->flags != 0) {
                if ((animateData->flags & 0x01) != 0) {
                    zMath::MatStackPushPtr(animateData->animatedTransform);
                    zMath::MatLoadIdentity();
                    zMath::MatApplyLocalTRS(&animateData->runtime.sampledRotation,
                                            &animateData->runtime.sampledPosition,
                                            &animateData->runtime.sampledScale);
                    zMath::MatStackPopPtr();
                    gwNodeRecalcBBox(node);
                    needsBBoxRecalc = false;
                }
                animateData->flags = 0;
            }
            break;
        }
        default:
            zError::ReportOld(
                0x200, kClassSourceFile, 0x99e,
                "gwNodeUpdate(): Unrecognized node class type:\n  node = %s class_type = %d\n",
                node, node->classId);
            result = 3;
            break;
        }

        if (needsBBoxRecalc) {
            gwNodeRecalcBBox(node);
        }
        node->flags &= ~kTransformQueuedFlag;
        return result;
    }

    // Reimplements 0x449420: zClass_Class::gwNodeUpdateDisplayInstance
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeUpdateDisplayInstance(zClass_NodePartial *
                                                                             node) {
        if (ReportNullNode(0xb31, node)) {
            return 5;
        }

        zDiPartial *di =
            (zDiPartial *)(static_cast<unsigned int>(node->userDataOrDiRef));
        if (di != 0) {
            zDi::RebuildBounds(di, (zBoundsMinMaxPartial *)(
                                       (unsigned char *)(node) + 0x8c));
            node->flags |= 0x200;
        } else {
            node->flags &= ~0x200;
        }

        return 0;
    }

    // Reimplements 0x4487c0: zClass_Class::gwNodeGetWorldBBoxCorners
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetWorldBBoxCorners(
        zClass_NodePartial * node, zBBoxCorners * outCorners) {
        if (ReportNullNode(0x81b, node)) {
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, kClassSourceFile, 0x81c, "Null class data pointer");
            return 5;
        }
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        const zBBox3f *bbox = CachedBBox(node);
        if (node->classId == 5) {
            const zClass_Object3DDataPartial *objectData =
                static_cast<const zClass_Object3DDataPartial *>(node->classData);
            if ((objectData->flags & 0x08) == 0) {
                zMath_Mat_TransformBBoxToCorners(
                    (const zMat4x3 *)(objectData->localMatrix), bbox, outCorners);
                return 0;
            }
        } else if (node->classId == 1) {
            zMath_Mat_TransformBBoxToCorners(MatrixAt(node->classData, 0x80), bbox, outCorners);
            return 0;
        } else if (node->classId == 8 && (node->flags & 0x04) != 0 &&
                   (static_cast<const unsigned char *>(node->classData)[4] & 0x04) != 0) {
            zMath_Mat_TransformBBoxToCorners(MatrixAt(node->classData, 0x08), bbox, outCorners);
            return 0;
        }

        CopyBBoxToCorners(bbox, outCorners);
        return 0;
    }

    // Reimplements 0x448920: zClass_Class::gwNodeGetViewBBoxCorners
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetViewBBoxCorners(
        zClass_NodePartial * node, zBBoxCorners * outCorners) {
        if (ReportNullNode(0x85f, node)) {
            return 5;
        }
        if (node->classData == 0) {
            zError::ReportOld(0x400, kClassSourceFile, 0x860, "Null class data pointer");
            return 5;
        }
        if ((node->flags & 0x100) == 0) {
            return 1;
        }

        int returnCode = 0;
        int currentIsIdentity = zMath_Mat_IsCurrentIdentity();
        zMat4x3 *currentMatrix = zMath_Mat_GetCurrent();
        int skipTransform = 0;
        const zMat4x3 *nodeMatrix = 0;

        switch (node->classId) {
        case 1:
            nodeMatrix = MatrixAt(node->classData, 0x80);
            break;
        case 2:
        case 6:
        case 7:
            skipTransform = 1;
            break;
        case 5: {
            const zClass_Object3DDataPartial *objectData =
                static_cast<const zClass_Object3DDataPartial *>(node->classData);
            skipTransform = (objectData->flags >> 3) & 0x01;
            nodeMatrix = MatrixAt(node->classData, 0x30);
            break;
        }
        case 8:
            if ((node->flags & 0x04) == 0 ||
                (static_cast<const unsigned char *>(node->classData)[4] & 0x04) == 0) {
                skipTransform = 1;
            }
            nodeMatrix = MatrixAt(node->classData, 0x08);
            break;
        case 9:
        case 10:
            break;
        default:
            returnCode = 3;
            break;
        }

        if (currentMatrix == 0) {
            currentIsIdentity = 1;
        }
        if (nodeMatrix == 0) {
            skipTransform = 1;
        }

        const zBBox3f *bbox = CachedBBox(node);
        if (currentIsIdentity != 0) {
            if (skipTransform != 0) {
                CopyBBoxToCorners(bbox, outCorners);
                return returnCode;
            }
            zMath_Mat_TransformBBoxToCorners(nodeMatrix, bbox, outCorners);
            return returnCode;
        }

        if (skipTransform != 0) {
            zMath_Mat_TransformBBoxToCorners(currentMatrix, bbox, outCorners);
            return returnCode;
        }

        zMat4x3 combinedMatrix = {0};
        MultiplyMatricesForViewBBox(currentMatrix, nodeMatrix, &combinedMatrix);
        zMath_Mat_TransformBBoxToCorners(&combinedMatrix, bbox, outCorners);
        return returnCode;
    }

    // Reimplements 0x4491b0: zClass_Class::gwNodeComputeChildBBox
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeComputeChildBBox(zClass_NodePartial * node) {
        if (ReportNullNode(0xaa3, node)) {
            return 5;
        }

        node->flags &= ~0x400;
        if (node->listCountB == 0 || node->classId == 2) {
            return 0;
        }

        zBBoxCorners corners = {0};
        int childIndex = 0;
        for (; childIndex < node->listCountB; ++childIndex) {
            zClass_NodePartial *child = node->listB[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            gwNodeGetWorldBBoxCorners(child, &corners);
            zBBox3f *childBBox = SecondaryBBox(node);
            childBBox->minX = corners.values[0];
            childBBox->minY = corners.values[1];
            childBBox->minZ = corners.values[2];
            childBBox->maxX = corners.values[0];
            childBBox->maxY = corners.values[1];
            childBBox->maxZ = corners.values[2];
            node->flags |= 0x400;

            {
            for (int cornerIndex = 1; cornerIndex < 8; ++cornerIndex) {
                ExpandBBoxWithCorner(childBBox, &corners.values[cornerIndex * 3]);
            }
            }
            ++childIndex;
            break;
        }

        if ((node->flags & 0x400) == 0) {
            return 0;
        }

        for (; childIndex < node->listCountB; ++childIndex) {
            zClass_NodePartial *child = node->listB[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            gwNodeGetWorldBBoxCorners(child, &corners);
            zBBox3f *childBBox = SecondaryBBox(node);
            {
            for (int cornerIndex = 0; cornerIndex < 8; ++cornerIndex) {
                ExpandBBoxWithCorner(childBBox, &corners.values[cornerIndex * 3]);
            }
            }
        }

        return 0;
    }

    // Reimplements 0x448e90: zClass_Class::gwNodeRecalcBBox
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeRecalcBBox(zClass_NodePartial * node) {
        if (ReportNullNode(0x9d0, node)) {
            return 5;
        }
        if (node->classId == 2) {
            return 0;
        }

        zBBox3f merged = {0};
        const zBBox3f *bboxSource = 0;
        const bool hasPrimaryBBox = (node->flags & 0x200) != 0;
        const bool hasChildBBox = (node->flags & 0x400) != 0;
        if (hasPrimaryBBox && hasChildBBox) {
            merged = MergeBBoxes(PrimaryBBox(node), SecondaryBBox(node));
            bboxSource = &merged;
        } else if (hasPrimaryBBox) {
            bboxSource = PrimaryBBox(node);
        } else if (hasChildBBox) {
            bboxSource = SecondaryBBox(node);
        } else {
            node->flags &= ~0x100;
            return 0;
        }

        node->flags |= 0x100;
        CopyBBoxToCachedBounds(node, bboxSource);
        node->boundsFlags |= 0x04;

        bool worldRectComputed = false;
        float minX = 0.0f;
        float maxX = 0.0f;
        float minZ = 0.0f;
        float maxZ = 0.0f;
        for (int i = 0; i < node->listCountA; ++i) {
            zClass_NodePartial *parent = node->listA[i];
            if (parent->classId != 2) {
                parent->boundsFlags |= 0x02;
                if ((parent->flags & 0x01) == 0) {
                    zClass_TypeList::InsertChildNodes(kQueuedTreeBucket, parent);
                    parent->flags |= 0x01;
                }
                parent->flags |= 0x02;
                continue;
            }

            if (!worldRectComputed) {
                ComputeXZRectFromCorners(node, &minX, &maxX, &minZ, &maxZ);
                worldRectComputed = true;
            }

            int gridCol = -1;
            int gridRow = -1;
            if ((node->flags & 0x80) == 0) {
                zClass_World::WorldRectToGridIndex(parent, &gridCol, minX, maxX, minZ, maxZ,
                                                   &gridRow);
            }

            if (gridCol == node->gridCol && gridRow == node->gridRow) {
                if (node->gridCol >= 0 && node->gridRow >= 0) {
                    zClass_World::EnsureGridCellDisplayPosition(parent, node->gridCol,
                                                                node->gridRow);
                }
            } else {
                zClass_World::RemoveChildAtGrid(parent, node);
                zClass_World::AddChildToGridCell(parent, node, gridCol, gridRow);
            }
        }

        return 0;
    }

    // Reimplements 0x447c60: zClass_Class::gwNodeSetActive
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetActive(zClass_NodePartial * node,
                                                                 int active) {
        if (ReportNullNode(0x38d, node)) {
            return 5;
        }

        switch (node->classId) {
        case 1:
        case 2:
        case 5:
        case 6:
        case 9:
            if (active == 1) {
                node->flags |= 0x04;
            } else if (active == 0) {
                node->flags &= ~0x04;
            }
            return 0;
        case 10:
            zClass_Sound::gwSoundSetActive(node, active);
            return 0;
        default:
            zError::ReportOld(
                0x400, kClassSourceFile, 0x3a4,
                "gwNodeSetActive(): Unrecognized node class type:\n  node = %s class_type = %d\n",
                node, node->classId);
            return 3;
        }
    }

    // Reimplements 0x447d20: zClass_Class::gwNodeSetFlag16
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetFlag16(zClass_NodePartial * node,
                                                                 int value) {
        if (ReportNullNode(0x3b7, node)) {
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x10000;
        } else {
            node->flags &= ~0x10000;
        }

        return 0;
    }

    // Reimplements 0x447d70: zClass_Class::gwNodeSetFlag17
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetFlag17(zClass_NodePartial * node,
                                                                 int value) {
        if (ReportNullNode(0x3c6, node)) {
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x20000;
        } else {
            node->flags &= ~0x20000;
        }

        return 0;
    }

    // Reimplements 0x447e60: zClass_Class::gwNodeSetDisplayInstance
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetDisplayInstance(
        zClass_NodePartial * node, zDiPartial * displayInstance) {
        if (ReportNullNode(0x424, node)) {
            return 5;
        }

        zDiPartial *oldDisplayInstance =
            (zDiPartial *)(static_cast<unsigned int>(node->userDataOrDiRef));
        if (oldDisplayInstance != 0) {
            zDi::Release(oldDisplayInstance);
        }

        node->userDataOrDiRef =
            static_cast<unsigned int>((unsigned int)(displayInstance));
        int flags = node->flags;
        if (displayInstance != 0) {
            zDi::AddRef(displayInstance);
            zDi::RebuildBounds(displayInstance,
                               (zBoundsMinMaxPartial *)(PrimaryBBox(node)));
            flags |= 0x200;
        } else {
            flags &= ~0x200;
        }

        node->flags = flags;
        node->boundsFlags |= 0x01;
        if ((node->flags & kTypeListInsertedFlag) == 0) {
            zClass_TypeList::Insert(kQueuedTreeBucket, node);
            node->flags |= kTypeListInsertedFlag;
        }
        node->flags |= kTransformQueuedFlag;
        return 0;
    }

    // Reimplements 0x447dc0: zClass_Class::gwNodeSetName
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetName(zClass_NodePartial * node,
                                                               const char *name) {
        if (ReportNullNode(0x3df, node)) {
            return 5;
        }

        if (strlen(name) >= sizeof(node->name)) {
            strncpy(node->name, name, 0x22);
            node->name[0x23] = '\0';
        } else {
            sprintf(node->name, "%s", name);
        }

        return 0;
    }

    // Reimplements 0x447e30: zClass_Class::gwNodeGetName
    RECOIL_NOINLINE char *RECOIL_FASTCALL gwNodeGetName(zClass_NodePartial * node) {
        if (ReportNullNode(0x40d, node)) {
            return 0;
        }

        return node->name;
    }

    // Reimplements 0x447f00: zClass_Class::gwNodeGetUserData
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetUserData(zClass_NodePartial * node,
                                                                   unsigned int *outData) {
        if (ReportNullNode(0x464, node)) {
            return 5;
        }

        *outData = node->userDataOrDiRef;
        return 0;
    }

    // Reimplements 0x447f30: zClass_Class::gwNodeSetActionCallback
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetActionCallback(zClass_NodePartial * node,
                                                                         void *actionCallback) {
        return SetActionCallbackCommon(node, actionCallback, false, 0x47e, 0x483);
    }

    // Reimplements 0x447fe0: zClass_Class::gwNodeSetActionCallbackTail
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetActionCallbackTail(
        zClass_NodePartial * node, void *actionCallback) {
        return SetActionCallbackCommon(node, actionCallback, true, 0x4c3, 0x4c8);
    }

    // Reimplements 0x448090: zClass_Class::gwNodeSetPriority
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetPriority(zClass_NodePartial * node,
                                                                   int priority) {
        if (ReportNullNode(0x4fc, node)) {
            return 5;
        }

        if (node->actionCallback != 0) {
            if (IsCallbackPriorityValid(node->callbackPriority)) {
                zClass_TypeList::MarkPendingRemoval(node->callbackPriority, node);
            }
            if (IsCallbackPriorityValid(priority)) {
                zClass_TypeList::Insert(priority, node);
            }
        }

        node->callbackPriority = priority;
        return 0;
    }

    // Reimplements 0x448100: zClass_Class::gwNodeSetCellPickable
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetCellPickable(zClass_NodePartial * node,
                                                                       int value) {
        if (ReportNullNode(0x529, node)) {
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x08;
        } else {
            node->flags &= ~0x08;
        }

        return 0;
    }

    // Reimplements 0x448140: zClass_Class::gwNodeGetCellPickable
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetCellPickable(zClass_NodePartial * node,
                                                                       int *outValue) {
        if (ReportNullNode(0x542, node)) {
            return 5;
        }

        *outValue = (node->flags >> 3) & 1;
        return 0;
    }

    // Reimplements 0x448180: zClass_Class::gwNodeGetNodeType
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetNodeType(zClass_NodePartial * node,
                                                                   int *outValue) {
        if (ReportNullNode(0x556, node)) {
            return 5;
        }

        *outValue = node->nodeType;
        return 0;
    }

    // Reimplements 0x4481b0: zClass_Class::gwNodeSetRaycastable
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetRaycastable(zClass_NodePartial * node,
                                                                      int value) {
        if (ReportNullNode(0x56c, node)) {
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x10;
        } else {
            node->flags &= ~0x10;
        }

        return 0;
    }

    // Reimplements 0x4481f0: zClass_Class::gwNodeGetRaycastable
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetRaycastable(zClass_NodePartial * node,
                                                                      int *outValue) {
        if (ReportNullNode(0x584, node)) {
            return 5;
        }

        *outValue = (node->flags >> 4) & 1;
        return 0;
    }

    // Reimplements 0x448230: zClass_Class::gwNodeSetPickable
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetPickable(zClass_NodePartial * node,
                                                                   int value) {
        if (ReportNullNode(0x59a, node)) {
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x20;
        } else {
            node->flags &= ~0x20;
        }

        return 0;
    }

    // Reimplements 0x448270: zClass_Class::gwNodeGetPickable
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetPickable(zClass_NodePartial * node,
                                                                   int *outValue) {
        if (ReportNullNode(0x5b2, node)) {
            return 5;
        }

        *outValue = (node->flags >> 5) & 1;
        return 0;
    }

    // Reimplements 0x4482b0: zClass_Class::gwNodeSetHasHitCallback
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetHasHitCallback(zClass_NodePartial * node,
                                                                         int value) {
        if (ReportNullNode(0x5c7, node)) {
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x40;
        } else {
            node->flags &= ~0x40;
        }

        return 0;
    }

    // Reimplements 0x4482f0: zClass_Class::gwNodeSetBypassFarClip
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetBypassFarClip(zClass_NodePartial * node,
                                                                        int value) {
        if (ReportNullNode(0x5e1, node)) {
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x80;
        } else {
            node->flags &= ~0x80;
        }

        return 0;
    }

    // Reimplements 0x448330: zClass_Class::gwNodeSetNodeType
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetNodeType(zClass_NodePartial * node,
                                                                   int nodeType) {
        if (ReportNullNode(0x5f9, node)) {
            return 5;
        }

        node->nodeType = static_cast<unsigned char>(nodeType);
        return 0;
    }

    // Reimplements 0x448360: zClass_Class::gwNodeClearVariantGate
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeClearVariantGate(zClass_NodePartial * node,
                                                                        int value) {
        if (ReportNullNode(0x60f, node)) {
            return 5;
        }

        if ((node->flags & 0x01000000) != 0 && value == 0) {
            node->flags &= ~0x01000000;
        }

        return 0;
    }

    // Reimplements 0x4483a0: zClass_Class::gwNodeSetVertexAlphaOverride
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetVertexAlphaOverride(
        zClass_NodePartial * node, int value) {
        if (ReportNullNode(0x62d, node)) {
            return 5;
        }

        if (value != 0) {
            node->flags |= 0x00800000;
        } else {
            node->flags &= ~0x00800000;
        }

        return 0;
    }

    // Reimplements 0x449ab0: zClass_Class::gwNodeGetRoot
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL gwNodeGetRoot(zClass_NodePartial * node) {
        zClass_NodePartial *current = node;
        if (current == 0) {
            return 0;
        }

        while (current->listCountA != 0) {
            if (current->listCountA != 1) {
                zError::ReportOld(0x200, kClassSourceFile, 0xd0d,
                                  "Error getting root node; Multiple parents found.\n  Node: %s\n",
                                  current);
                return 0;
            }

            current = current->listA[0];
            if (current == 0) {
                return 0;
            }
        }

        return current;
    }

    // Reimplements 0x452770: zClass_Class::FindSubNodeByName
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL FindSubNodeByName(zClass_NodePartial * root,
                                                                          const char *name) {
        if (root == 0) {
            return 0;
        }
        if (strcmp(name, root->name) == 0) {
            return root;
        }

        for (int i = root->listCountB - 1; i >= 0; --i) {
            zClass_NodePartial *found = FindSubNodeByName(root->listB[i], name);
            if (found != 0) {
                return found;
            }
        }

        return 0;
    }

    // Reimplements 0x447bc0: zClass_Class::FindNodeRecursiveByName
    // (D:\Proj\GameZRecoil\zClass\Class.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL FindNodeRecursiveByName(
        zClass_NodePartial * root, const char *name) {
        if (ReportNullNode(0x33a, root)) {
            return 0;
        }

        if (strcmp(root->name, name) == 0) {
            return root;
        }

        for (int i = 0; i < root->listCountB; ++i) {
            zClass_NodePartial *const childMatch = FindNodeRecursiveByName(root->listB[i], name);
            if (childMatch != 0) {
                return childMatch;
            }
        }

        return 0;
    }

    // Reimplements 0x449af0: zClass_Class::gwNodeGetWorldChild
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL gwNodeGetWorldChild(zClass_NodePartial *
                                                                            node) {
        zClass_NodePartial *current = node;
        while (current != 0 && current->listCountA != 0) {
            if (current->listCountA != 1) {
                zError::ReportOld(
                    0x200, kClassSourceFile, 0xd4e,
                    "Error getting world child; Multiple parents found.\n  Node: %s\n", current);
                return 0;
            }

            zClass_NodePartial *parent = current->listA[0];
            if (parent == 0) {
                return 0;
            }
            if (parent->classId == 2) {
                return parent;
            }
            current = parent;
        }

        return 0;
    }

    // Reimplements 0x449b40: zClass_Class::SetSingleParentFlagRecursive
    RECOIL_NOINLINE int RECOIL_FASTCALL SetSingleParentFlagRecursive(
        zClass_NodePartial * node, int setFlag) {
        if (node == 0) {
            return 1;
        }

        if (setFlag != 0) {
            if (node->listCountA > 1) {
                return 0;
            }
            node->flags |= kSingleParentFlag;
        } else {
            node->flags &= ~kSingleParentFlag;
        }

        for (int i = 0; i < node->listCountB; ++i) {
            SetSingleParentFlagRecursive(node->listB[i], setFlag);
        }

        return 0;
    }

    // Reimplements 0x452920: zClass_Class::AddChildValidated
    RECOIL_NOINLINE int RECOIL_FASTCALL AddChildValidated(zClass_NodePartial * parent,
                                                                   zClass_NodePartial * child) {
        if (ValidateParentChildForSwitch(parent, child, 0x80, 0x81, 0x82) != 0) {
            return 5;
        }

        return AddChildGeneric(parent, child);
    }

    // Reimplements 0x452970: zClass_Class::RemoveChildValidated
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChildValidated(zClass_NodePartial * parent,
                                                                      zClass_NodePartial * child) {
        if (ValidateParentChildForSwitch(parent, child, 0x9f, 0xa0, 0xa1) != 0) {
            return 5;
        }

        return RemoveChildGeneric(parent, child);
    }

    // Reimplements 0x4483f0: zClass_Class::AddChild
    // (GameZRecoil/zClass/Class.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL AddChild(zClass_NodePartial * parent,
                                                          zClass_NodePartial * child) {
        if (child == 0) {
            zError::ReportOld(0x400, kClassSourceFile, 0x666, "Null node pointer.");
            return 5;
        }
        if (parent == 0) {
            zError::ReportOld(0x400, kClassSourceFile, 0x667, "Null node pointer.");
            return 5;
        }

        switch (parent->classId) {
        case 1:
            return zClass_Camera::gwCameraAddChild(parent, child);
        case 2:
            return zClass_World::AddChildAtGrid(parent, child);
        case 3:
        case 4:
        case 9:
        case 10:
            return zClass_Class::AddChildGeneric(parent, child);
        case 5:
            return zClass_Object3D::gwObject3DAddChild(parent, child);
        case 6:
            return zClass_Lod::gwLodAddChild(parent, child);
        case 7:
            sprintf(g_zError_DebugMsgBuffer,
                         "%s: Line %d: ERROR: Please use dedicated function "
                         "gwSequenceAddChild() for node: %s\n",
                         kClassSourceFile, 0x687, parent->name);
            zError::EmitDebugBuffer(1);
            return 1;
        case 8:
            return zClass_Animate::AddChild(parent, child);
        case 11:
            return zClass_Class::AddChildValidated(parent, child);
        default:
            sprintf(g_zError_DebugMsgBuffer,
                         "%s: Line %d: ERROR: Unrecognized node class type for node: %s\n",
                         kClassSourceFile, 0x69f, parent->name);
            zError::EmitDebugBuffer(1);
            return 1;
        }
    }

    // Reimplements 0x448570: zClass_Class::RemoveChild
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial * parent,
                                                             zClass_NodePartial * child) {
        if (parent == 0) {
            zError::ReportOld(0x400, kClassSourceFile, 0x713, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, kClassSourceFile, 0x714, "Null node pointer.");
            return 5;
        }

        switch (parent->classId) {
        case 1:
            return zClass_Camera::gwCameraRemoveChild(parent, child);
        case 2:
            return zClass_World::RemoveChildAtGrid(parent, child);
        case 3:
            return zClass::RemoveChildChecked(parent, child);
        case 4:
            return zClass_Display::RemoveChild(parent, child);
        case 5:
            return zClass_Object3D::RemoveChild(parent, child);
        case 6:
            return zClass_Lod::RemoveChild(parent, child);
        case 7:
            return zClass_Sequence::RemoveChild(parent, child);
        case 8:
            return zClass_Animate::RemoveChild(parent, child);
        case 9:
            return zClass_Light::RemoveChild(parent, child);
        case 10:
            return zClass_Sound::RemoveChild(parent, child);
        case 11:
            return zClass_Class::RemoveChildValidated(parent, child);
        default:
            sprintf(g_zError_DebugMsgBuffer,
                         "%s: Line %d: ERROR: Unrecognized node class type for node: %s\n",
                         kClassSourceFile, 0x748, parent->name);
            zError::EmitDebugBuffer(1);
            return 1;
        }
    }

    // Reimplements 0x4484d0: zClass_Class::AddChildGeneric
    RECOIL_NOINLINE int RECOIL_FASTCALL AddChildGeneric(zClass_NodePartial * parent,
                                                                 zClass_NodePartial * child) {
        const int newChildCount = parent->listCountB + 1;
        parent->listB = static_cast<zClass_NodePartial **>(realloc(
            parent->listB, static_cast<size_t>(newChildCount) * sizeof(parent->listB[0])));
        parent->listB[parent->listCountB] = child;
        parent->listCountB = newChildCount;

        const int newParentCount = child->listCountA + 1;
        child->listA = static_cast<zClass_NodePartial **>(realloc(
            child->listA, static_cast<size_t>(newParentCount) * sizeof(child->listA[0])));
        child->listA[child->listCountA] = parent;
        child->listCountA = newParentCount;
        if (newParentCount > 1) {
            SetSingleParentFlagRecursive(child, 0);
        }

        parent->boundsFlags |= kBoundsDirtyFlag;
        if ((parent->flags & kTypeListInsertedFlag) == 0) {
            zClass_TypeList::Insert(kQueuedTreeBucket, parent);
            parent->flags |= kTypeListInsertedFlag;
        }
        parent->flags |= kTransformQueuedFlag;

        return 0;
    }

    // Reimplements 0x448660: zClass_Class::RemoveChildGeneric
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChildGeneric(zClass_NodePartial * parent,
                                                                    zClass_NodePartial * child) {
        int childIndex = -1;
        for (int i = 0; i < parent->listCountB; ++i) {
            if (parent->listB[i] == child) {
                childIndex = i;
                break;
            }
        }

        if (childIndex < 0) {
            zError::ReportOld(0x200, kClassSourceFile, 0x79c,
                              "ERROR deleting child node %s from parent node %s", child, parent);
        } else {
            for (int i = childIndex + 1; i < parent->listCountB; ++i) {
                parent->listB[i - 1] = parent->listB[i];
            }
            --parent->listCountB;
        }

        int parentIndex = -1;
        for (int i_1261 = 0; i_1261 < child->listCountA; ++i_1261) {
            if (child->listA[i_1261] == parent) {
                parentIndex = i_1261;
                break;
            }
        }

        if (parentIndex >= 0) {
            for (int i = parentIndex + 1; i < child->listCountA; ++i) {
                child->listA[i - 1] = child->listA[i];
            }
            --child->listCountA;
            if (child->listCountA == 1 && (parent->flags & kSingleParentFlag) != 0) {
                SetSingleParentFlagRecursive(child, 1);
            }
        }

        parent->boundsFlags |= kBoundsDirtyFlag;
        if ((parent->flags & kTypeListInsertedFlag) == 0) {
            zClass_TypeList::Insert(kQueuedTreeBucket, parent);
            parent->flags |= kTypeListInsertedFlag;
        }
        parent->flags |= kTransformQueuedFlag;

        return 0;
    }

    // Reimplements 0x447a70: zClass_Class::FreeNodeToFreeList
    RECOIL_NOINLINE int RECOIL_FASTCALL FreeNodeToFreeList(zClass_NodePartial * node) {
        if (ReportNullNode(0x28e, node)) {
            return 5;
        }
        if (node->listCountB > 0 || node->listCountA > 0 || node->userDataOrDiRef != 0) {
            return 1;
        }

        if (node->listB != 0) {
            free(node->listB);
            node->listB = 0;
        }
        if (node->listA != 0) {
            free(node->listA);
            node->listA = 0;
        }
        if (node->classData != 0) {
            free(node->classData);
            node->classData = 0;
            node->classId = 0;
        }

        const ptrdiff_t index =
            (zClass_NodeFreeListSlot *)(node) - g_zClass_NodeArray;
        zClass_NodeFreeListSlot &slot = g_zClass_NodeArray[index];
        slot.freeTag = (slot.freeTag & 0xff000000) |
                       (static_cast<unsigned int>(g_zClass_NodeFreeHeadIndex) & 0x00ffffff);
        --g_zClass_ActiveNodeCount;
        g_zClass_NodeFreeHeadIndex = static_cast<int>(index);

        return 0;
    }

    // Reimplements 0x447b60: zClass_Class::TryFreeNode
    RECOIL_NOINLINE int RECOIL_FASTCALL TryFreeNode(zClass_NodePartial * node) {
        if (ReportNullNode(0x2f0, node)) {
            return 5;
        }

        node->flags &= ~kTransformQueuedFlag;
        zClass_List::DeleteNodeFromLists(node);
        if (zClass::ProcessDeferredWork() != 0) {
            zClass_NodeList::Insert(node);
        } else {
            FreeNodeToFreeList(node);
        }

        return 0;
    }
}

namespace zClass_Class {
    RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNodeByType(zClass_NodePartial * node) {
        if (ReportNullNode(0x231, node)) {
            return 5;
        }

        if (node->listCountA > 0) {
            return 1;
        }

        switch (node->classId) {
        case 0:
            TryFreeNode(node);
            return static_cast<int>((unsigned int)(node));
        case 1:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 11:
            return zClass_Object3D::DeleteNode(node);
        case 2:
            return zClass_World::DeleteNode(node);
        case 8:
            return zClass_Animate::DeleteNode(node);
        case 9:
            return zClass_Light::DeleteNode(node);
        case 10:
            return zClass_Sound::DeleteNode(node);
        default:
            zError::ReportOld(0x400, kClassSourceFile, 0x272,
                              "ERROR: Unrecognized node class type for node: %s\n", node->name);
            return 1;
        }
    }
}

namespace gwNode {
    // Reimplements 0x449480: gwNode::BuildNodeToAncestorMatrix
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildNodeToAncestorMatrix(
        zClass_NodePartial * node, int matMode) {
        zVec3 unitScale = {1.0f, 1.0f, 1.0f};
        zVec3 zeroAngles = {0};

        if (node == 0) {
            zError::ReportOld(0x400, kClassSourceFile, 0xb66, "Null node pointer.");
            return 5;
        }

        if (node->classId == 5 && (node->flags & kSingleParentFlag) != 0) {
            zClass_Object3DDataPartial *objectData =
                static_cast<zClass_Object3DDataPartial *>(node->classData);
            if ((objectData->flags & 0x20) == 0) {
                zMath::MatLoadCurrentFrom(
                    (const zMat4x3 *)(objectData->cachedWorldMatrix));
                return 0;
            }
        }

        zClass_NodePartial *parentChain[15] = {0};
        int chainCount = 1;
        parentChain[0] = node;
        zClass_NodePartial *current = node;
        while (current->listCountA == 1) {
            current = current->listA[0];
            if (current == 0) {
                break;
            }
            parentChain[chainCount++] = current;
            if (current->listCountA > 1) {
                zError::ReportOld(
                    0x800, kClassSourceFile, 0xb80,
                    "node has multiple parents; count = %d.\n  node = %s class_type = %d\n",
                    current->listCountA, current, current->classId);
                return 1;
            }
        }

        if (current != 0 && current->listCountA > 1) {
            zError::ReportOld(
                0x800, kClassSourceFile, 0xb80,
                "node has multiple parents; count = %d.\n  node = %s class_type = %d\n",
                current->listCountA, current, current->classId);
            return 1;
        }

        for (int i = 0; i < chainCount; ++i) {
            zClass_NodePartial *chainNode = parentChain[i];
            if (chainNode->classId != 2 && (chainNode->flags & 0x01) != 0) {
                UpdateTree(chainNode);
                break;
            }
        }

        for (int i_1435 = chainCount - 1; i_1435 >= 0; --i_1435) {
            zClass_NodePartial *ancestor = parentChain[i_1435];
            const int ancestorFlags = ancestor->flags & ~kNodeTransformDirtyPropagatedFlag;
            ancestor->flags = ancestorFlags;

            switch (ancestor->classId) {
            case 1: {
                zClass_CameraDataPartial *cameraData =
                    static_cast<zClass_CameraDataPartial *>(ancestor->classData);
                if ((cameraData->cameraFlags & 0x02) == 0) {
                    zMath::MatApplyLocalTRS(&cameraData->posOffset, &cameraData->targetOrEuler,
                                            &unitScale);
                } else {
                    zMath::MatMultiply(MatrixAt(cameraData, 0x80), 1);
                }
                break;
            }
            case 2:
            case 6:
                break;
            case 5: {
                zClass_Object3DDataPartial *objectData =
                    static_cast<zClass_Object3DDataPartial *>(ancestor->classData);
                const int objectFlags = objectData->flags;
                if ((objectFlags & 0x08) == 0) {
                    if ((ancestorFlags & kSingleParentFlag) != 0) {
                        if ((objectFlags & 0x20) != 0) {
                            zMath::MatMultiply(
                                (const zMat4x3 *)(objectData->localMatrix),
                                matMode);
                            CopyCurrentMatrixTo(objectData->cachedWorldMatrix);
                            objectData->flags &= ~0x20;
                        } else {
                            zMath::MatLoadCurrentFrom(
                                (const zMat4x3 *)(objectData->cachedWorldMatrix));
                        }
                    } else {
                        zMath::MatMultiply(
                            (const zMat4x3 *)(objectData->localMatrix), matMode);
                    }
                } else if ((ancestorFlags & kSingleParentFlag) != 0 && (objectFlags & 0x20) != 0) {
                    CopyCurrentMatrixTo(objectData->cachedWorldMatrix);
                    objectData->flags &= ~0x20;
                }
                break;
            }
            case 8: {
                void *animateData = ancestor->classData;
                if ((ancestorFlags & 0x04) != 0 &&
                    (*static_cast<unsigned char *>(static_cast<unsigned char *>(animateData) +
                                                  0x04) &
                     0x04) != 0) {
                    zMath::MatMultiply(MatrixAt(animateData, 0x08), matMode);
                }
                break;
            }
            case 9: {
                zClass_LightDataPartial *lightData =
                    static_cast<zClass_LightDataPartial *>(ancestor->classData);
                zMath::MatApplyLocalTRS(&lightData->localRotation, &lightData->localPosition,
                                        &unitScale);
                break;
            }
            case 10:
                zMath::MatApplyLocalTRS(&zeroAngles, Vec3At(ancestor->classData, 0x30), &unitScale);
                break;
            default:
                sprintf(g_zError_DebugMsgBuffer,
                             "%s: Line %d: gwNodeBuildNodeToAncestorMatrix(): Unrecognized node "
                             "class type:\n",
                             kClassSourceFile, 0xbfa);
                sprintf(g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
                             "  node = %s class_type = %d\n", ancestor->name, ancestor->classId);
                zError::EmitDebugBuffer(3);
                return 3;
            }
        }

        return 0;
    }

    // Reimplements 0x4497b0: gwNode::GetWorldPosition
    // (D:\Proj\GameZRecoil\zClass\Class.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL GetWorldPosition(zClass_NodePartial * node,
                                                                  zVec3 * outPosition) {
        if (node == 0) {
            return 1;
        }

        if (node->classId == 5 && (node->flags & kSingleParentFlag) != 0) {
            zClass_Object3DDataPartial *objectData =
                static_cast<zClass_Object3DDataPartial *>(node->classData);
            if ((objectData->flags & 0x20) == 0) {
                outPosition->x = objectData->cachedWorldMatrix[9];
                outPosition->y = objectData->cachedWorldMatrix[10];
                outPosition->z = objectData->cachedWorldMatrix[11];
                return 0;
            }
        }

        outPosition->x = 0.0f;
        outPosition->y = 0.0f;
        outPosition->z = 0.0f;

        float matrix[12];
        zMath::MatStackPushPtr(matrix);
        zMath::MatLoadIdentity();
        BuildNodeToAncestorMatrix(node, 1);
        outPosition->x = matrix[9];
        outPosition->y = matrix[10];
        outPosition->z = matrix[11];
        zMath::MatStackPopPtr();
        return 0;
    }

    // Reimplements 0x449850: gwNode::TransformPoint
    // (D:\Proj\GameZRecoil\zClass\Class.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL TransformPoint(zClass_NodePartial * node,
                                                                zVec3 * point) {
        if (node == 0) {
            return 1;
        }

        if (point->x == 0.0f && point->y == 0.0f && point->z == 0.0f) {
            GetWorldPosition(node, point);
            return 0;
        }

        zMat4x3 matrix = {0};
        zMath::MatStackPushPtr((float *)(&matrix));
        zMath::MatLoadIdentity();
        BuildNodeToAncestorMatrix(node, 1);
        zMath::MatTransformPointBatchInPlace(point, 1);
        zMath::MatStackPopPtr();
        return 0;
    }

    // Reimplements 0x4498e0: gwNode::GetWorldPosAndOrientation
    // (D:\Proj\GameZRecoil\zClass\Class.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL GetWorldPosAndOrientation(
        zClass_NodePartial * node, zVec3 * inOutPosition, zVec3 * outOrientation) {
        zVec3 localOrientationBasis[2] = {{0.0f, 0.0f, -1.0f}, {1.0f, 0.0f, 0.0f}};

        if (node == 0) {
            return 1;
        }

        zMat4x3 matrix = {0};
        zMath::MatStackPushPtr((float *)(&matrix));
        zMath::MatLoadIdentity();
        BuildNodeToAncestorMatrix(node, 1);

        if (inOutPosition->x == 0.0f && inOutPosition->y == 0.0f && inOutPosition->z == 0.0f) {
            inOutPosition->x = matrix.posX;
            inOutPosition->y = matrix.posY;
            inOutPosition->z = matrix.posZ;
        } else {
            zMath::MatTransformPointBatchInPlace(inOutPosition, 1);
        }

        zVec3 worldPosition = {matrix.posX, matrix.posY, matrix.posZ};
        zVec3 worldOrientationBasis[2];
        memcpy(worldOrientationBasis, localOrientationBasis, sizeof(worldOrientationBasis));
        if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
            zMath::MatTransformPointBatchInPlace(worldOrientationBasis, 2);
        }

        zMath::MatLoadIdentity();
        zVec3 directionAngles = {0};
        zMath::Vec3DirectionAnglesBetweenPoints(&worldPosition, &worldOrientationBasis[0],
                                                &directionAngles);
        outOrientation->x = directionAngles.x;
        outOrientation->y = directionAngles.y;
        outOrientation->z = directionAngles.z;
        outOrientation->z =
            zMath_Vec3_ElevationAngleBetweenPoints(&worldPosition, &worldOrientationBasis[1]);

        zMath::MatStackPopPtr();
        return 0;
    }
}

namespace zClass_Class {
    // Reimplements 0x44c0e0: zClass_Class::gwNodeRenderDispatch
    // (D:\Proj\GameZRecoil\zClass\Class.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeRenderDispatch(
        zClass_NodePartial *node, int siblingCountHint) {
        const int variantId = node->nodeType;
        const int variantAllowed = VariantTag::CurrentAllowsId(variantId);
        if (variantAllowed == 0) {
            return variantAllowed;
        }

        switch (node->classId - 1) {
        case 0:
            return zClass_Camera::RenderTraverse(node, siblingCountHint);
        case 4:
            return zClass_Object3D::RenderTraverse(node, siblingCountHint);
        case 5:
            return zClass_Lod::RenderTraverse(node, siblingCountHint);
        case 6:
            return zClass_Sequence::RenderTraverse(node, siblingCountHint);
        case 7:
            return zClass_Animate::RenderTraverse(node, siblingCountHint);
        case 8:
            return zClass_Light::RenderTraverse(node, siblingCountHint);
        case 9:
            return zClass_Sound::RenderTraverse(node, siblingCountHint);
        case 10:
            return zClass_Switch::RenderTraverse(node, siblingCountHint);
        default:
            return fprintf(stderr, "Unrecognized node rendering type: %s\n", node->name);
        }
    }
}

namespace zClass_Node {
    namespace {
        OptCatalogDamageHandlerPartial *&DamageHandlerRef(zClass_NodePartial * node) {
            return (OptCatalogDamageHandlerPartial *&)(
                ((zClass_NodeFreeListSlot *)(node))->damageHandler);
        }
    }

    // Reimplements 0x421da0: zClass_Node::PropagateExtraFlagsRecursive
    RECOIL_NOINLINE void RECOIL_FASTCALL PropagateExtraFlagsRecursive(zClass_NodePartial * self,
                                                                      int flags) {
        self->auxFlags |= flags;

        for (int i = 0; i < self->listCountB; ++i) {
            PropagateExtraFlagsRecursive(self->listB[i], flags);
        }
    }

    // Reimplements 0x437e60: zClass_Node::SetContextRecursive
    RECOIL_NOINLINE void RECOIL_FASTCALL SetContextRecursive(
        zClass_NodePartial * self, zClass_NodePartial * context, int flagMask) {
        self->callbackContext = context;
        self->flags |= flagMask;

        for (int i = 0; i < self->listCountB; ++i) {
            SetContextRecursive(self->listB[i], context, flagMask);
        }
    }

    // Reimplements 0x452860: zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive
    RECOIL_NOINLINE void RECOIL_FASTCALL SetMaterialFlagBit9ForFlagBit0EntriesRecursive(
        zClass_NodePartial * node, int enabled) {
        zDiPartial *di =
            (zDiPartial *)(static_cast<unsigned int>(node->userDataOrDiRef));
        if (di != 0) {
            zDi_SetMaterialFlagBit9ForFlagBit0Entries(di, enabled);
        }

        for (int i = 0; i < node->listCountB; ++i) {
            SetMaterialFlagBit9ForFlagBit0EntriesRecursive(node->listB[i], enabled);
        }
    }

    // Reimplements 0x4528b0: zClass_Node::InvalidateFlagBit8MaterialImagesRecursive
    RECOIL_NOINLINE void RECOIL_FASTCALL InvalidateFlagBit8MaterialImagesRecursive(
        zClass_NodePartial * node) {
        zDiPartial *di =
            (zDiPartial *)(static_cast<unsigned int>(node->userDataOrDiRef));
        if (di != 0) {
            zDi::InvalidateImagesForFlagBit8Materials(di);
        }

        for (int i = 0; i < node->listCountB; ++i) {
            InvalidateFlagBit8MaterialImagesRecursive(node->listB[i]);
        }
    }

    // Reimplements 0x4528e0: zClass_Node::AssignInt32ToDiRecursive
    RECOIL_NOINLINE void RECOIL_FASTCALL AssignInt32ToDiRecursive(zClass_NodePartial * node,
                                                                  int value) {
        zDiPartial *di =
            (zDiPartial *)(static_cast<unsigned int>(node->userDataOrDiRef));
        if (di != 0) {
            zDi::SetFlagBit0(di, value);
        }

        for (int i = 0; i < node->listCountB; ++i) {
            AssignInt32ToDiRecursive(node->listB[i], value);
        }
    }

    // Reimplements 0x4b25f0: zClass_Node::AssignDamageHandlerRecursiveIfMissing
    RECOIL_NOINLINE void RECOIL_FASTCALL AssignDamageHandlerRecursiveIfMissing(
        zClass_NodePartial * node, OptCatalogDamageHandlerPartial * handler) {
        if (DamageHandlerRef(node) != 0) {
            return;
        }

        for (int i = 0; i < node->listCountB; ++i) {
            AssignDamageHandlerRecursiveIfMissing(node->listB[i], handler);
        }

        DamageHandlerRef(node) = handler;
    }

    // Reimplements 0x4b2670: zClass_Node::ClearDamageHandlerRecursive
    RECOIL_NOINLINE void RECOIL_FASTCALL ClearDamageHandlerRecursive(
        zClass_NodePartial * node, OptCatalogDamageHandlerPartial * handler) {
        for (int i = 0; i < node->listCountB; ++i) {
            ClearDamageHandlerRecursive(node->listB[i], handler);
        }

        if (DamageHandlerRef(node) == handler) {
            DamageHandlerRef(node) = 0;
        }
    }

    // Reimplements 0x4b25a0: zClass_Node::SetDamageHitCallback
    RECOIL_NOINLINE int RECOIL_FASTCALL SetDamageHitCallback(
        void *callback, zClass_NodePartial *node, void *context) {
        OptCatalogDamageHandlerPartial *handler = DamageHandlerRef(node);
        if (handler == 0) {
            handler = static_cast<OptCatalogDamageHandlerPartial *>(
                calloc(1, sizeof(OptCatalogDamageHandlerPartial)));
        } else if (handler->hitContext != 0) {
            return 0;
        }

        handler->hitContext = context;
        handler->hitCallback = callback;
        AssignDamageHandlerRecursiveIfMissing(node, handler);
        zClass_Class::gwNodeSetHasHitCallback(node, 1);
        return 0;
    }

    // Reimplements 0x4b2630: zClass_Node::ClearDamageHandler
    RECOIL_NOINLINE int RECOIL_FASTCALL ClearDamageHandler(zClass_NodePartial * node) {
        if (node == 0) {
            return 0;
        }

        OptCatalogDamageHandlerPartial *handler = DamageHandlerRef(node);
        if (handler != 0) {
            ClearDamageHandlerRecursive(node, handler);
            if (handler->hitContext != 0) {
                zClass_Class::gwNodeSetHasHitCallback(node, 0);
            }
            free(handler);
        }

        return 0;
    }

    // Reimplements 0x4b26b0: zClass_Node::SetDamageTimerCallback
    RECOIL_NOINLINE int RECOIL_FASTCALL SetDamageTimerCallback(
        void *callback, zClass_NodePartial *node, void *context) {
        OptCatalogDamageHandlerPartial *handler = DamageHandlerRef(node);
        if (handler == 0) {
            handler = static_cast<OptCatalogDamageHandlerPartial *>(
                calloc(1, sizeof(OptCatalogDamageHandlerPartial)));
        }

        handler->timerContext = context;
        handler->timerCallback = callback;
        AssignDamageHandlerRecursiveIfMissing(node, handler);
        return 0;
    }
}
