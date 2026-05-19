#include "zClass.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zDi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
    const char *kSoundSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\Sound.c";

    float *SavedParentMatrix(zClass_SoundDataPartial *data) {
        return (float *)((unsigned char *)data + 0x48);
    }

    int CullNodeForRender(zClass_NodePartial *node, int siblingCountHint,
                                   int *clipMask) {
        int result = 0;
        if ((*clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
            if ((node->boundsFlags & 0x04) != 0 || g_zClass_RenderBoundsContextActive != 0 ||
                (node->flags & 0x00080000) == 0) {
                zBBoxCorners corners = {0};
                zClass_Class::gwNodeGetViewBBoxCorners(node, &corners);
                BBox::CornersToBoundingSphere(&corners, zClass_NodeViewSphereCenter(node),
                                              zClass_NodeViewSphereRadius(node));
                if ((node->flags & 0x00080000) != 0) {
                    node->boundsFlags &= ~0x04;
                }
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

    void RenderNodeAndChildren(zClass_NodePartial *node, int clipMask) {
        node->flags |= 0x80000000;
        zDiPartial *di = (zDiPartial *)(unsigned int)node->userDataOrDiRef;
        if (di != 0 && g_zClass_RenderRangeFadeActive != 0) {
            di->flags |= 0x08;
            di->blendScale = g_zClass_RenderRangeFadeScale;
        }
        if (gModel_RenderFn != 0) {
            gModel_RenderFn(node, clipMask);
        }
        if (node->listCountB > 0) {
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;
            for (int i = 0; i < node->listCountB; ++i) {
                zClass_Class::gwNodeRenderDispatch(node->listB[i], node->listCountB);
            }
            --gModel_ClipMaskStackTop;
        }
    }
}

namespace zClass_Sound {
    // Reimplements 0x4529c0: zClass_Sound::gwSoundNew
    // (D:\Proj\GameZRecoil\zClass\Sound.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwSoundNew() {
        zClass_NodePartial *const node = zClass_Class::AllocNodeFromFreeList();
        if (node == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x76, "Null node pointer.");
            return 0;
        }

        node->cachedBounds[0] = 1.0f;
        node->cachedBounds[1] = 1.0f;
        node->cachedBounds[2] = -2.0f;
        node->cachedBounds[3] = 2.0f;
        node->cachedBounds[4] = 2.0f;
        node->cachedBounds[5] = -1.0f;
        node->flags |= 0x100;
        node->classId = 10;

        zClass_SoundDataPartial *const soundData =
            static_cast<zClass_SoundDataPartial *>(calloc(1, sizeof(zClass_SoundDataPartial)));
        node->classData = soundData;
        soundData->sample = 0;
        soundData->playHandle = 0;
        soundData->runtimeFlags |= 0x01;
        soundData->falloffMode = 1;
        soundData->rangeMin = 32.0f;
        soundData->rangeMax = 64.0f;
        soundData->rangeMaxSq = 4096.0f;
        soundData->invRangeSpan = 0.03125f;

        zClass_Class::gwNodeSetActive(node, 1);
        soundData->attachedWorldCount = 0;
        soundData->attachedWorlds = 0;
        zClass_TypeList::Insert(10, node);

        return node;
    }

    // Reimplements 0x452ab0: zClass_Sound::DeleteNode
    RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNode(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0xc3, "Null node pointer.");
            return 5;
        }

        zClass_SoundDataPartial *soundData =
            static_cast<zClass_SoundDataPartial *>(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0xc4, "Null class data pointer");
            return 5;
        }

        zSndPlayHandle *playHandle = soundData->playHandle;
        if (playHandle != 0) {
            playHandle->StopIfActive();
            if ((soundData->runtimeFlags & 0x08) != 0) {
                zSndPlayHandle_TryDisableManaged(soundData->playHandle);
                soundData->runtimeFlags &= ~0x08;
            }
            soundData->playHandle = 0;
        }

        if (soundData->attachedWorldCount > 0) {
            sprintf(g_zError_DebugMsgBuffer,
                         "%s: Line %d: ERROR deleting sound; Sound attached to %d world nodes.\n",
                         kSoundSourceFile, 0xda, soundData->attachedWorldCount);
            zError::EmitDebugBuffer(1);
            return 1;
        }

        if (soundData->attachedWorlds != 0) {
            free(soundData->attachedWorlds);
            soundData->attachedWorlds = 0;
        }

        return zClass_Class::TryFreeNode(node);
    }

    // Reimplements 0x452b80: zClass_Sound::RemoveChild
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial * parent,
                                                             zClass_NodePartial * child) {
        if (parent == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x100, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x101, "Null node pointer.");
            return 5;
        }

        return zClass_Class::RemoveChildGeneric(parent, child);
    }

    // Reimplements 0x452bc0: zClass_Sound::SetSampleSetByName
    // (D:\Proj\GameZRecoil\zClass\Sound.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetSampleSetByName(zClass_NodePartial * node,
                                                                    const char *name) {
        if (node == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x11e, "Null node pointer.");
            return 5;
        }

        zClass_SoundDataPartial *const soundData =
            static_cast<zClass_SoundDataPartial *>(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x11f, "Null class data pointer");
            return 5;
        }

        if (strlen(name) >= sizeof(soundData->sampleSetName)) {
            strncpy(soundData->sampleSetName, name, 0x22);
            soundData->sampleSetName[0x23] = '\0';
        } else {
            sprintf(soundData->sampleSetName, "%s", name);
        }

        soundData->sample = zSnd::FindSampleByName(soundData->sampleSetName);
        soundData->playHandle = 0;
        soundData->runtimeFlags |= 0x01;

        return 0;
    }

    // Reimplements 0x452c60: zClass_Sound::gwSoundSetActive
    RECOIL_NOINLINE int RECOIL_FASTCALL gwSoundSetActive(zClass_NodePartial * node,
                                                                  int active) {
        if (node == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x149, "Null node pointer.");
            return 5;
        }

        zClass_SoundDataPartial *const soundData =
            static_cast<zClass_SoundDataPartial *>(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x14a, "Null class data pointer");
            return 5;
        }

        zSndPlayHandle *const playHandle = soundData->playHandle;
        if (playHandle != 0 && active == 0) {
            playHandle->StopIfActive();
            if ((soundData->runtimeFlags & 0x08) != 0) {
                zSndPlayHandle_TryDisableManaged(soundData->playHandle);
                soundData->runtimeFlags &= ~0x08;
            }
            soundData->playHandle = 0;
        }

        if (active == 1) {
            node->flags |= 0x04;
        } else if (active == 0) {
            node->flags &= ~0x04;
        }

        return 0;
    }

    // Reimplements 0x452d00: zClass_Sound::gwSoundSetPosition
    // (D:\Proj\GameZRecoil\zClass\Sound.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwSoundSetPosition(zClass_NodePartial * node,
                                                                    float x, float y, float z) {
        if (node == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x17e, "Null node pointer.");
            return 5;
        }

        zClass_SoundDataPartial *const soundData =
            static_cast<zClass_SoundDataPartial *>(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x17f, "Null class data pointer");
            return 5;
        }

        soundData->localPosition.x = x;
        soundData->localPosition.y = y;
        soundData->localPosition.z = z;
        soundData->runtimeFlags |= 0x03;
        return 0;
    }

    // Reimplements 0x452dc0: zClass_Sound::UpdatePlayback
    // (D:\Proj\GameZRecoil\zClass\Sound.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL UpdatePlayback(zClass_NodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x224, "Null node pointer.");
            return 5;
        }

        if ((node->flags & 0x04) == 0) {
            return 0;
        }

        zClass_SoundDataPartial *soundData =
            static_cast<zClass_SoundDataPartial *>(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, kSoundSourceFile, 0x22a, "Null class data pointer");
            return 5;
        }

        if (soundData->playHandle == 0 &&
            (zClass_Class::gwNodeGetRoot(node) != node || (soundData->runtimeFlags & 0x02) != 0)) {
            soundData->runtimeFlags |= 0x04;
        }

        if ((soundData->runtimeFlags & 0x04) != 0) {
            ComputeWorldTransform(node, soundData);
            if (soundData->playHandle != 0) {
                soundData->playHandle->Update3DDispatch(&soundData->worldPos, 0, 0);
                soundData->runtimeFlags &= ~0x01;
                return 0;
            }

            if (soundData->sample != 0) {
                soundData->playHandle =
                    soundData->sample->PlayA3D(&soundData->worldPos, 1.0f, 0);
                if (zSndPlayHandle_TryEnableManaged(soundData->playHandle) != 0) {
                    soundData->runtimeFlags |= 0x08;
                }
            }
        } else if (soundData->playHandle == 0 && soundData->sample != 0) {
            soundData->playHandle = soundData->sample->PlayA3DSimple(1.0f);
            if (zSndPlayHandle_TryEnableManaged(soundData->playHandle) != 0) {
                soundData->runtimeFlags |= 0x08;
            }
        }

        soundData->runtimeFlags &= ~0x01;
        return 0;
    }

    // Reimplements 0x452ec0: zClass_Sound::ComputeWorldTransform
    // (D:\Proj\GameZRecoil\zClass\Sound.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL ComputeWorldTransform(
        zClass_NodePartial * node, zClass_SoundDataPartial * soundData) {
        zVec3 localPoint = {0.0f, 0.0f, 0.0f};
        zMat4x3 slotBuffer = {0};

        zMath::MatStackPushPtr((float *)(&slotBuffer));
        zMath::MatLoadIdentity();
        gwNode::BuildNodeToAncestorMatrix(node, 1);

        if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
            soundData->worldPos = localPoint;
        } else {
            const zMat4x3 *matrix =
                (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
            soundData->worldPos.x =
                localPoint.x * matrix->xx + localPoint.y * matrix->yx +
                localPoint.z * matrix->zx + matrix->posX;
            soundData->worldPos.z =
                localPoint.x * matrix->xz + localPoint.y * matrix->yz +
                localPoint.z * matrix->zz + matrix->posZ;
            soundData->worldPos.y =
                localPoint.x * matrix->xy + localPoint.y * matrix->yy +
                localPoint.z * matrix->zy + matrix->posY;
        }

        zMath::MatStackPopPtr();
        return 0;
    }

    // Reimplements 0x44af60: zClass_Sound::RenderTraverse
    // (D:\Proj\GameZRecoil\zClass\Sound.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(
        zClass_NodePartial *node, int siblingCountHint) {
        const int flags = node->flags;
        int boundsContextPushed = 0;
        if ((flags & 0x04) == 0) {
            return 0;
        }

        node->flags = flags & ~0x02000000;
        zClass_SoundDataPartial *data = static_cast<zClass_SoundDataPartial *>(node->classData);
        int clipMask = *gModel_ClipMaskStackTop;
        const int result = CullNodeForRender(node, siblingCountHint, &clipMask);
        if (g_zClass_RenderBoundsContextActive == 0) {
            boundsContextPushed = 1;
            g_zClass_RenderBoundsContextActive = 1;
        }

        if (result == 0) {
            const zVec3 angles = {0.0f, 0.0f, 0.0f};
            const zVec3 unitScale = {1.0f, 1.0f, 1.0f};
            zMath::MatStackPushAndCloneParent(SavedParentMatrix(data));
            zMath::MatApplyLocalTRS(&angles, &data->localPosition, &unitScale);
            RenderNodeAndChildren(node, clipMask);
            zMath::MatStackPopPtr();
        }

        if (boundsContextPushed != 0) {
            g_zClass_RenderBoundsContextActive = 0;
        }
        return result;
    }
}
