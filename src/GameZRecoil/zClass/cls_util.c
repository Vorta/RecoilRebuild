#include "zClass.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"

#include <stdlib.h>
#include <string.h>

extern "C" {
int g_zClass_NodeArraySize = 0;
int g_zClass_IsInitialized = 0;
int g_zClass_CopyNodeCloneDiMode = 0;
int g_zClass_CopyNodeDiArg0 = 0;
int g_zClass_CopyNodeDiArg1 = 0;
}

namespace {
    const char *kClsUtilSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\cls_util.c";

    float ApproximateRangeFromRangeSq(float rangeSq) {
        int bits = 0;
        memcpy(&bits, &rangeSq, sizeof(bits));
        bits = (bits >> 1) + 0x1fc00000;
        float range = 0.0f;
        memcpy(&range, &bits, sizeof(range));
        return range;
    }
}

namespace BBox {
    // Reimplements 0x452650: BBox::CornersToBoundingSphere
    // (GameZRecoil/zClass/cls_util.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL CornersToBoundingSphere(zBBoxCorners * corners,
                                                                 zVec3 *outCenter,
                                                                 float *outRadius) {
        const zVec3 *corner = (const zVec3 *)corners->values;
        float minX = corner[0].x;
        float maxX = corner[0].x;
        float minY = corner[0].y;
        float maxY = corner[0].y;
        float minZ = corner[0].z;
        float maxZ = corner[0].z;

        for (int i = 1; i < 8; ++i) {
            if (corner[i].x < minX) {
                minX = corner[i].x;
            } else if (corner[i].x > maxX) {
                maxX = corner[i].x;
            }
            if (corner[i].y < minY) {
                minY = corner[i].y;
            } else if (corner[i].y > maxY) {
                maxY = corner[i].y;
            }
            if (corner[i].z < minZ) {
                minZ = corner[i].z;
            } else if (corner[i].z > maxZ) {
                maxZ = corner[i].z;
            }
        }

        const float halfX = (maxX - minX) * 0.5f;
        const float halfY = (maxY - minY) * 0.5f;
        const float halfZ = (maxZ - minZ) * 0.5f;
        outCenter->x = minX + halfX;
        outCenter->y = minY + halfY;
        outCenter->z = minZ + halfZ;
        *outRadius = ApproximateRangeFromRangeSq(halfX * halfX + halfY * halfY + halfZ * halfZ);
    }
}

namespace zClass {
    // Reimplements 0x4518b0: zClass::SetNodeArraySize
    RECOIL_NOINLINE void RECOIL_FASTCALL SetNodeArraySize(int size) {
        if (g_zClass_NodeArraySize != 0) {
            zError::ReportOld(0x200, kClsUtilSourceFile, 0x210,
                              "Error setting node array size; size already set to %d.",
                              g_zClass_NodeArraySize);
            return;
        }

        g_zClass_NodeArraySize = size;
    }

    // Reimplements 0x4518f0: zClass::IsInitialized
    RECOIL_NOINLINE int RECOIL_CDECL IsInitialized() {
        return g_zClass_IsInitialized;
    }

    // Reimplements 0x454360: zClass::ResetCurrentZbdPath
    RECOIL_NOINLINE int RECOIL_CDECL ResetCurrentZbdPath() {
        g_zClass_CurrentZbdPath[0] = 0;
        return 0;
    }

    // Reimplements 0x451a00: zClass::ShutdownCore
    RECOIL_NOINLINE int RECOIL_CDECL ShutdownCore() {
        zClass_List::DeleteAllOfType(6);
        zClass_TypeList::FreeAll();

        if (g_zClass_NodeArraySize > 0) {
            if (g_zClass_NodeArray != 0) {
                free(g_zClass_NodeArray);
                g_zClass_NodeArray = 0;
            }

            g_zClass_NodeArraySize = 0;
            g_zClass_ActiveNodeCount = 0;
            g_zClass_NodeFreeHeadIndex = -1;
        }

        ResetCurrentZbdPath();
        g_zClass_IsInitialized = 0;
        return 0;
    }

    // Reimplements 0x4518e0: zClass::Shutdown (GameZRecoil/zClass/cls_util.c)
    RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
        ShutdownCore();
        return 0;
    }
}

namespace zClass_Util {
    RECOIL_NOINLINE int RECOIL_FASTCALL DestroyNodeRecursive(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x2b6, "Null node pointer.");
            return 1;
        }

        if (node->listCountA > 0) {
            return 1;
        }

        while (node->listCountB > 0) {
            zClass_NodePartial *child = node->listB[0];
            const int removeResult = zClass_Class::RemoveChild(node, child);
            if (removeResult != 0) {
                return removeResult;
            }

            if (child->listCountA == 0) {
                const int destroyResult = DestroyNodeRecursive(child);
                if (destroyResult != 0) {
                    return destroyResult;
                }
            }
        }

        zDiPartial *displayInstance =
            (zDiPartial *)(static_cast<unsigned int>(node->userDataOrDiRef));
        if (displayInstance != 0) {
            const int setResult = zClass_Class::gwNodeSetDisplayInstance(node, 0);
            if (setResult != 0) {
                return setResult;
            }
            if (displayInstance->refCount == 0) {
                const int freeResult = zModel_DiPool::FreeIfUnreferenced(displayInstance);
                if (freeResult != 0) {
                    return freeResult;
                }
            }
        }

        return zClass_Class::DeleteNodeByType(node);
    }
}

namespace zClass_cls_util {
    // Reimplements 0x451b20: zClass_cls_util::CopyNodeDisplayInstance
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL CopyNodeDisplayInstance(
        zClass_NodePartial * source, zClass_NodePartial * dest) {
        int result = 0;

        if (source->userDataOrDiRef == 0) {
            return result;
        }

        unsigned int displayInstanceValue = 0;
        if (g_zClass_CopyNodeCloneDiMode == 0) {
            result = zClass_Class::gwNodeGetUserData(source, &displayInstanceValue);
            if (result == 0) {
                return zClass_Class::gwNodeSetDisplayInstance(
                    dest, (zDiPartial *)(
                              static_cast<unsigned int>(displayInstanceValue)));
            }
            return result;
        }

        unsigned int sourceDisplayInstanceValue = 0;
        result = zClass_Class::gwNodeGetUserData(source, &sourceDisplayInstanceValue);
        if (result != 0) {
            return result;
        }

        zDiPartial *const sourceDisplayInstance =
            (zDiPartial *)(static_cast<unsigned int>(sourceDisplayInstanceValue));
        displayInstanceValue = sourceDisplayInstanceValue;
        int cloneInstance = 1;
        if (g_zClass_CopyNodeDiArg1 != 0 &&
            zDi::HasSpecialFlagsOrAuxMaterialData(sourceDisplayInstance) == 0) {
            cloneInstance = 0;
        }

        if (cloneInstance == 0) {
            return zClass_Class::gwNodeSetDisplayInstance(
                dest,
                (zDiPartial *)(static_cast<unsigned int>(displayInstanceValue)));
        }

        zDiPartial *const clonedDisplayInstance = zDi::CloneToInstance(
            sourceDisplayInstance, g_zClass_CopyNodeDiArg0, g_zClass_CopyNodeDiArg1);
        if (clonedDisplayInstance == 0) {
            return 1;
        }

        return zClass_Class::gwNodeSetDisplayInstance(dest, clonedDisplayInstance);
    }

    // Reimplements 0x451bd0: zClass_cls_util::CopyNodeBaseData
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL CopyNodeBaseData(zClass_NodePartial * source,
                                                                  zClass_NodePartial * dest) {
        int result = zClass_Class::gwNodeSetName(dest, source->name);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x34e,
                              "ERROR copying node while setting description field.  Source Node: "
                              "(address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetActive(dest, (source->flags >> 2) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x359,
                              "ERROR copying node while setting active field.  Source Node: "
                              "(address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetCellPickable(dest, (source->flags >> 3) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x364,
                              "ERROR copying node while setting altitude surface field.  Source "
                              "Node: (address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetRaycastable(dest, (source->flags >> 4) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x36f,
                              "ERROR copying node while setting intersection field.  Source Node: "
                              "(address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetPickable(dest, (source->flags >> 5) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x37a,
                              "ERROR copying node while setting intersect bbox field.  Source "
                              "Node: (address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetHasHitCallback(dest, (source->flags >> 6) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x385,
                              "ERROR copying node while setting proximity flag  Source Node: "
                              "(address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetBypassFarClip(dest, (source->flags >> 7) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x390,
                              "ERROR copying node while setting landmark flag  Source Node: "
                              "(address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetFlag16(dest, (source->flags >> 16) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x39b,
                              "ERROR copying node while setting can_modify flag  Source Node: "
                              "(address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetFlag17(dest, (source->flags >> 17) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x3a6,
                              "ERROR copying node while setting clip_to flag  Source Node: "
                              "(address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeClearVariantGate(dest, (source->flags >> 24) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x3b1,
                              "ERROR copying node while setting DI zone check flag  Source Node: "
                              "(address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetVertexAlphaOverride(dest, (source->flags >> 23) & 1);
        if (result != 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x3bc,
                              "ERROR copying node while setting overwrite flag  Source Node: "
                              "(address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        dest->flags |= source->flags & 0x70000000;
        dest->auxFlags = source->auxFlags;

        result = CopyNodeDisplayInstance(source, dest);
        if (result != 0) {
            zError::ReportOld(
                0x400, kClsUtilSourceFile, 0x3ce,
                "ERROR copying node graphics data.  Source Node: (address =%x) (desc = %s)", source,
                source);
            return result;
        }

        if (source->callbackContext != 0) {
            zError::ReportOld(0x100, kClsUtilSourceFile, 0x3d8,
                              "Source node (%s) has environment data.  Ignoring.", source);
        }
        dest->callbackContext = 0;

        result = zClass_Class::gwNodeSetPriority(dest, source->callbackPriority);
        if (result != 0) {
            zError::ReportOld(0x100, kClsUtilSourceFile, 0x3e2,
                              "ERROR copying node while setting action callback priority field.  "
                              "Source Node: (address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetActionCallback(dest, source->actionCallback);
        if (result != 0) {
            zError::ReportOld(0x100, kClsUtilSourceFile, 0x3ed,
                              "ERROR copying node while setting action callback field  Source "
                              "Node: (address =%x) (desc = %s)",
                              source, source);
            return result;
        }

        result = zClass_Class::gwNodeSetNodeType(dest, source->nodeType);
        if (result != 0) {
            zError::ReportOld(
                0x100, kClsUtilSourceFile, 0x3f8,
                "ERROR copying node while setting zone ID.  Source Node: (address =%x) (desc = %s)",
                source, source);
            return result;
        }

        return 0;
    }

    // Reimplements 0x451f70: zClass_cls_util::CopyCameraNode
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyCameraNode(zClass_NodePartial *
                                                                       source) {
        zClass_NodePartial *const camera = zClass_Camera::gwCameraNew();
        if (camera == 0) {
            return camera;
        }

        if (CopyNodeBaseData(source, camera) != 0) {
            zError::ReportOld(0x100, kClsUtilSourceFile, 0x422, "ERROR copying node base data.",
                              "  Source Node: (address =%x) (desc = %s)", source, source);
            return 0;
        }

        zClass_CameraDataPartial *const data =
            static_cast<zClass_CameraDataPartial *>(source->classData);
        if (zClass_Camera::gwCameraSetWorld(camera, data->worldNode) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetWindow(camera, data->windowNode) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetTarget(camera, data->targetOrEuler.x, data->targetOrEuler.y,
                                             data->targetOrEuler.z) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetPosition(camera, data->posOffset.x, data->posOffset.y,
                                               data->posOffset.z) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetNearFarClip(camera, data->nearClip, data->farClip) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetClipDistance(camera, data->clipDistance) != 0) {
            return 0;
        }
        if (zClass_Camera::gwCameraSetFOV(camera, data->fovX, data->fovY) != 0) {
            return 0;
        }

        for (int i = 0; i < source->listCountB; ++i) {
            zClass_NodePartial *const child = CopyNodeDispatch(source->listB[i]);
            if (child == 0 || zClass_Camera::gwCameraAddChild(camera, child) != 0) {
                return 0;
            }
        }

        return camera;
    }

    // Reimplements 0x452100: zClass_cls_util::CopyObject3DNode
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyObject3DNode(zClass_NodePartial *
                                                                         source) {
        zClass_NodePartial *const parent = zClass_Object3D::gwObject3DInit();
        if (parent == 0) {
            return parent;
        }

        if (CopyNodeBaseData(source, parent) != 0) {
            zError::ReportOld(0x100, kClsUtilSourceFile, 0x4b9, "ERROR copying node base data.",
                              "  Source Node: (address =%x) (desc = %s)", source, source);
            return 0;
        }

        zClass_Object3DDataPartial *const data =
            static_cast<zClass_Object3DDataPartial *>(source->classData);
        if (zClass_Object3D::gwObject3DSetAlphaScale(parent, data->alphaScale) != 0) {
            return 0;
        }
        if (zClass_Object3D::gwObject3DSetLitFlag(parent, (data->flags >> 1) & 1) != 0) {
            return 0;
        }

        if ((data->flags & 0x08) == 0) {
            if ((data->flags & 0x10) != 0) {
                if (zClass_Object3D::gwObject3DSetMatrix(parent, data->localMatrix) != 0) {
                    return 0;
                }
            } else {
                if (zClass_Object3D::gwObject3DSetPosition(parent, data->localMatrix[9],
                                                           data->localMatrix[10],
                                                           data->localMatrix[11]) != 0) {
                    return 0;
                }
                if (zClass_Object3D::gwObject3DSetRotation(
                        parent, data->rotation.x, data->rotation.y, data->rotation.z) != 0) {
                    return 0;
                }
                if (zClass_Object3D::gwObject3DSetScale(parent, data->scale.x, data->scale.y,
                                                        data->scale.z) != 0) {
                    return 0;
                }
            }
        }

        for (int i = 0; i < source->listCountB; ++i) {
            zClass_NodePartial *const child = CopyNodeDispatch(source->listB[i]);
            if (child != 0 && zClass_Object3D::gwObject3DAddChild(parent, child) != 0) {
                return 0;
            }
        }

        return parent;
    }

    // Reimplements 0x4520c0: zClass_cls_util::CopyLightNode_Unimplemented
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyLightNode_Unimplemented(
        zClass_NodePartial *) {
        zError::ReportOld(0x800, kClsUtilSourceFile, 0x47d,
                          "Can't copy light node; Function not yet implemented");
        return 0;
    }

    // Reimplements 0x4520e0: zClass_cls_util::CopySoundNode_Unimplemented
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopySoundNode_Unimplemented(
        zClass_NodePartial *) {
        zError::ReportOld(0x800, kClsUtilSourceFile, 0x493,
                          "Can't copy sound node; Function not yet implemented");
        return 0;
    }

    // Reimplements 0x452230: zClass_cls_util::CopyAnimateNode_Unimplemented
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyAnimateNode_Unimplemented(
        zClass_NodePartial *) {
        zError::ReportOld(0x100, kClsUtilSourceFile, 0x518,
                          "ERROR copying animate node; Function not implemented yet.");
        return 0;
    }

    // Reimplements 0x452250: zClass_cls_util::CopyLodNode
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyLodNode(zClass_NodePartial * source) {
        zClass_NodePartial *const parent = zClass_Lod::gwLodNew();
        if (parent == 0) {
            return parent;
        }

        if (CopyNodeBaseData(source, parent) != 0) {
            zError::ReportOld(0x100, kClsUtilSourceFile, 0x53e, "ERROR copying node base data.",
                              "  Source Node: (address =%x) (desc = %s)", source, source);
            return 0;
        }

        zClass_LodDataPartial *const sourceData =
            static_cast<zClass_LodDataPartial *>(source->classData);
        if (zClass_Lod::SetComputeOwnDistance(parent, sourceData->computeOwnDistance) != 0) {
            return 0;
        }

        zClass_LodDataPartial *const destData =
            static_cast<zClass_LodDataPartial *>(parent->classData);
        destData->nearRangeSq = sourceData->nearRangeSq;
        destData->nearRange = sourceData->nearRange;
        destData->farRangeSq = sourceData->farRangeSq;
        destData->fadeWidth = sourceData->fadeWidth;
        destData->fadeAmount = sourceData->fadeAmount;
        destData->fadeEndScale = sourceData->fadeEndScale;
        destData->fogFadeWidth = sourceData->fogFadeWidth;
        destData->fogFadeAmount = sourceData->fogFadeAmount;
        destData->fogStartDist = sourceData->fogStartDist;
        destData->vertexShadingAmount = sourceData->vertexShadingAmount;
        destData->active = sourceData->active;

        if (zClass_Lod::SetTargetNodeAndRange(parent, sourceData->rangeNode,
                                              ApproximateRangeFromRangeSq(sourceData->rangeSq)) !=
            0) {
            return 0;
        }

        for (int i = 0; i < source->listCountB; ++i) {
            zClass_NodePartial *const child = CopyNodeDispatch(source->listB[i]);
            if (child == 0 || zClass_Lod::gwLodAddChild(parent, child) != 0) {
                return 0;
            }
        }

        return parent;
    }

    // Reimplements 0x4523c0: zClass_cls_util::CopySequenceNode_Unimplemented
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopySequenceNode_Unimplemented(
        zClass_NodePartial *) {
        zError::ReportOld(0x100, kClsUtilSourceFile, 0x585,
                          "ERROR copying sequence node; Function not implemented yet.");
        return 0;
    }

    // Reimplements 0x4523e0: zClass_cls_util::CopySwitchNode_Stub
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopySwitchNode_Stub(zClass_NodePartial *) {
        zError::ReportOld(0x100, kClsUtilSourceFile, 0x59c,
                          "ERROR copying switch node; Function not implemented yet.");
        return 0;
    }

    // Reimplements 0x452400: zClass_cls_util::CopyNodeDispatch
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyNodeDispatch(zClass_NodePartial *
                                                                         source) {
        if (source == 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x5b8, "Null node pointer.");
            return 0;
        }

        if ((source->flags & 0x04000000) != 0) {
            return source;
        }

        switch (source->classId) {
        case 1:
            return CopyCameraNode(source);
        case 2:
            zError::ReportOld(0x100, kClsUtilSourceFile, 0x5e1,
                              "ERROR cannot copy world class nodes.");
            return 0;
        case 5:
            return CopyObject3DNode(source);
        case 6:
            return CopyLodNode(source);
        case 7:
            CopySequenceNode_Unimplemented(source);
            return 0;
        case 8:
            CopyAnimateNode_Unimplemented(source);
            return 0;
        case 9:
            CopyLightNode_Unimplemented(source);
            return 0;
        case 10:
            CopySoundNode_Unimplemented(source);
            return 0;
        case 11:
            CopySwitchNode_Stub(source);
            return 0;
        default:
            zError::ReportOld(0x100, kClsUtilSourceFile, 0x5e8,
                              "ERROR Unrecognized node in copying process: %s", source);
            return 0;
        }
    }

    // Reimplements 0x452500: zClass_cls_util::CopyNodeWithCloneOptions
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyNodeWithCloneOptions(
        zClass_NodePartial * source, int cloneDiMode, int diArg0) {
        if (source == 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x60f, "Null node pointer.");
            return 0;
        }

        const int savedDiArg0 = g_zClass_CopyNodeDiArg0;
        const int savedCloneDiMode = g_zClass_CopyNodeCloneDiMode;
        g_zClass_CopyNodeCloneDiMode = cloneDiMode;
        g_zClass_CopyNodeDiArg0 = diArg0;

        zClass_NodePartial *const result = CopyNodeDispatch(source);
        g_zClass_CopyNodeCloneDiMode = savedCloneDiMode;
        g_zClass_CopyNodeDiArg0 = savedDiArg0;
        return result;
    }

    // Reimplements 0x452560: zClass_cls_util::CopyNode
    // (D:\Proj\GameZRecoil\zClass\cls_util.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyNode(
        zClass_NodePartial * source, int cloneDiMode, int diArg0,
        int diArg1) {
        if (source == 0) {
            zError::ReportOld(0x400, kClsUtilSourceFile, 0x648, "Null node pointer.");
            return 0;
        }

        const int savedDiArg0 = g_zClass_CopyNodeDiArg0;
        const int savedCloneDiMode = g_zClass_CopyNodeCloneDiMode;
        const int savedDiArg1 = g_zClass_CopyNodeDiArg1;
        g_zClass_CopyNodeCloneDiMode = cloneDiMode;
        g_zClass_CopyNodeDiArg0 = diArg0;
        g_zClass_CopyNodeDiArg1 = diArg1;

        zClass_NodePartial *const result = CopyNodeDispatch(source);
        g_zClass_CopyNodeDiArg1 = savedDiArg1;
        g_zClass_CopyNodeCloneDiMode = savedCloneDiMode;
        g_zClass_CopyNodeDiArg0 = savedDiArg0;
        return result;
    }
}

RECOIL_NOINLINE float *RECOIL_FASTCALL BBox_MinMaxToBoundingSphere(const zBBox3f *bbox,
                                                                   zVec3 *outCenter,
                                                                   float *outRadius) {
    const float halfX = (bbox->maxX - bbox->minX) * 0.5f;
    const float halfY = (bbox->maxY - bbox->minY) * 0.5f;
    const float halfZ = (bbox->maxZ - bbox->minZ) * 0.5f;
    outCenter->x = bbox->minX + halfX;
    outCenter->y = bbox->minY + halfY;
    outCenter->z = bbox->minZ + halfZ;

    float radiusSq = halfX * halfX + halfY * halfY + halfZ * halfZ;
    int bits = 0;
    memcpy(&bits, &radiusSq, sizeof(bits));
    bits = (bits >> 1) + 0x1fc00000;
    memcpy(outRadius, &bits, sizeof(bits));
    return outRadius;
}
