#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace CZSound {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sound.zclass-sound-gwsoundnew
     * @recoil-artifact defines .text recoil:function:0x4529c0: CZSound::gwSoundNew
     * @recoil-match byte
     *
     * Purpose: allocate a sound node, seed default bounds and attenuation
     * state, activate it, and register it with the sound type list.
     */
    CZNodePartial *__cdecl gwSoundNew() {
        CZNodePartial *const node = CZClass::gwNodeNew();
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x76, "Null node pointer.");
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

        CZSoundDataPartial *const soundData =
            (CZSoundDataPartial *)(calloc(1, sizeof(CZSoundDataPartial)));
        node->classData = soundData;
        soundData->sample = 0;
        soundData->playHandle = 0;
        soundData->falloffMode = 1;
        soundData->rangeMin = 32.0f;
        soundData->rangeMax = 64.0f;
        soundData->rangeMaxSq = 4096.0f;
        soundData->invRangeSpan = 0.03125f;
        soundData->runtimeFlags |= 0x01;

        CZClass::gwNodeSetActive(node, 1);
        soundData->attachedWorldCount = 0;
        soundData->attachedWorlds = 0;
        CZTypeList::Insert(10, node);

        return node;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sound.zclass-sound-deletenode
     * @recoil-artifact defines .text recoil:function:0x452ab0: CZSound::DeleteNode
     * @recoil-match byte
     *
     * Purpose: stop and release active playback, reject deletion while attached
     * to world nodes, free world attachment storage, and free the node.
     */
    int __fastcall DeleteNode(CZNodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0xc3, "Null node pointer.");
            return 5;
        }

        CZSoundDataPartial *soundData = (CZSoundDataPartial *)(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0xc4, "Null class data pointer");
            return 5;
        }

        zSndPlayHandle *playHandle = soundData->playHandle;
        if (playHandle != 0) {
            playHandle->StopIfActive();
            if ((soundData->runtimeFlags & 0x08) != 0) {
                zSndPlayHandleTryDisableManaged(soundData->playHandle);
                soundData->runtimeFlags &= ~0x08;
            }
            soundData->playHandle = 0;
        }

        if (soundData->attachedWorldCount > 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR deleting sound; Sound attached to %d world nodes.\n",
                "D:\\Proj\\GameZRecoil\\zClass\\Sound.c",
                0xda,
                soundData->attachedWorldCount
            );
            zError::EmitDebugBuffer(1);
            return 1;
        }

        if (soundData->attachedWorlds != 0) {
            free(soundData->attachedWorlds);
            soundData->attachedWorlds = 0;
        }

        return CZClass::TryFreeNode(node);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sound.zclass-sound-removechild
     * @recoil-artifact defines .text recoil:function:0x452b80: CZSound::RemoveChild
     * @recoil-match byte
     *
     * Purpose: validate sound parent and child nodes, then remove the child
     * through the shared zClass child-list helper.
     */
    int __fastcall RemoveChild(
        CZNodePartial * parent,
        CZNodePartial * child
    ) {
        if (parent == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x100, "Null node pointer.");
            return 5;
        }
        if (child == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x101, "Null node pointer.");
            return 5;
        }

        return CZClass::RemoveChildGeneric(parent, child);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sound.zclass-sound-setsamplesetbyname
     * @recoil-artifact defines .text recoil:function:0x452bc0: CZSound::SetSampleSetByName
     * @recoil-match byte
     *
     * Purpose: copy the sample-set name into the sound data, resolve the sound
     * sample, reset playback, and mark the runtime state dirty.
     */
    int __fastcall SetSampleSetByName(
        CZNodePartial * node,
        const char *name
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x11e, "Null node pointer.");
            return 5;
        }

        CZSoundDataPartial *const soundData = (CZSoundDataPartial *)(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x11f, "Null class data pointer");
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

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sound.zclass-sound-gwsoundsetactive
     * @recoil-artifact defines .text recoil:function:0x452c60: CZSound::gwSoundSetActive
     * @recoil-match byte
     *
     * Purpose: toggle sound-node activity, stopping managed playback when the
     * node is deactivated.
     */
    int __fastcall gwSoundSetActive(
        CZNodePartial * node,
        int active
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x149, "Null node pointer.");
            return 5;
        }

        CZSoundDataPartial *const soundData = (CZSoundDataPartial *)(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x14a, "Null class data pointer");
            return 5;
        }

        zSndPlayHandle *const playHandle = soundData->playHandle;
        if (playHandle != 0 && active == 0) {
            playHandle->StopIfActive();
            if ((soundData->runtimeFlags & 0x08) != 0) {
                zSndPlayHandleTryDisableManaged(soundData->playHandle);
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

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sound.zclass-sound-gwsoundsetposition
     * @recoil-artifact defines .text recoil:function:0x452d00: CZSound::gwSoundSetPosition
     * @recoil-match byte
     *
     * Purpose: store the sound node's local position and mark transform and
     * playback state dirty.
     */
    int __fastcall gwSoundSetPosition(
        CZNodePartial * node,
        float x,
        float y,
        float z
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x17e, "Null node pointer.");
            return 5;
        }

        CZSoundDataPartial *const soundData = (CZSoundDataPartial *)(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x17f, "Null class data pointer");
            return 5;
        }

        soundData->localPosition.x = x;
        soundData->localPosition.y = y;
        soundData->localPosition.z = z;
        soundData->runtimeFlags |= 0x03;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sound.zclass-sound-gwsoundgetposition
     * @recoil-artifact defines .text recoil:function:0x452d60: CZSound::gwSoundGetPosition
     * @recoil-match byte
     *
     * Purpose: copy the sound node's local position into the caller-provided
     * output coordinates.
     */
    int __fastcall gwSoundGetPosition(
        CZNodePartial * node,
        float *outX,
        float *outY,
        float *outZ
    ) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x1d0, "Null node pointer.");
            return 5;
        }

        CZSoundDataPartial *const soundData = (CZSoundDataPartial *)(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x1d1, "Null class data pointer");
            return 5;
        }

        *outX = soundData->localPosition.x;
        *outY = soundData->localPosition.y;
        *outZ = soundData->localPosition.z;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sound.zclass-sound-updateplayback
     * @recoil-artifact defines .text recoil:function:0x452dc0: CZSound::UpdatePlayback
     *
     *
     * Purpose: update or create positional and non-positional playback handles
     * for active sound nodes, then clear the dirty playback flag.
     */
    int __fastcall UpdatePlayback(CZNodePartial * node) {
        if (node == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x224, "Null node pointer.");
            return 5;
        }

        if ((node->flags & 0x04) == 0) {
            return 0;
        }

        CZSoundDataPartial *soundData = (CZSoundDataPartial *)(node->classData);
        if (soundData == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Sound.c", 0x22a, "Null class data pointer");
            return 5;
        }

        if (soundData->playHandle == 0 &&
            (CZClass::gwNodeGetRoot(node) != node || (soundData->runtimeFlags & 0x02) != 0)) {
            soundData->runtimeFlags |= 0x04;
        }

        if ((soundData->runtimeFlags & 0x04) != 0) {
            ComputeWorldTransform(node, soundData);
            if (soundData->playHandle == 0) {
                if (soundData->sample != 0) {
                    soundData->playHandle = soundData->sample->PlayA3D(
                        &soundData->worldPos,
                        1.0f,
                        0
                    );
                    if (zSndPlayHandleTryEnableManaged(soundData->playHandle) != 0) {
                        soundData->runtimeFlags |= 0x08;
                    }
                }
            } else {
                soundData->playHandle->Update3DDispatch(&soundData->worldPos, 0, 0);
                soundData->runtimeFlags &= ~0x01;
                return 0;
            }
        } else if (soundData->playHandle == 0 && soundData->sample != 0) {
            soundData->playHandle = soundData->sample->PlayA3DSimple(1.0f);
            if (zSndPlayHandleTryEnableManaged(soundData->playHandle) != 0) {
                soundData->runtimeFlags |= 0x08;
            }
        }

        soundData->runtimeFlags &= ~0x01;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.sound.zclass-sound-computeworldtransform
     * @recoil-artifact defines .text recoil:function:0x452ec0: CZSound::ComputeWorldTransform
     *
     *
     * Purpose: build the node-to-world matrix and cache the sound emitter's
     * world position in sound runtime data.
     * The matrix stack is restored before returning.
     */
    int __fastcall ComputeWorldTransform(
        CZNodePartial * node,
        CZSoundDataPartial * soundData
    ) {
        zVec3 localPoint = {0.0f, 0.0f, 0.0f};
        zMat4x3 slotBuffer = {0};

        zMath::MatStackPushPtr((float *)(&slotBuffer));
        zMath::MatLoadIdentity();
        CZNode::gwNodeBuildNodeToAncestorMatrix(node, 1);

        zVec3 worldPoint = localPoint;
        if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
            const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
            worldPoint.x = localPoint.x * matrix->xx + localPoint.y * matrix->yx +
                           localPoint.z * matrix->zx + matrix->posX;
            worldPoint.z = localPoint.x * matrix->xz + localPoint.y * matrix->yz +
                           localPoint.z * matrix->zz + matrix->posZ;
            worldPoint.y = localPoint.x * matrix->xy + localPoint.y * matrix->yy +
                           localPoint.z * matrix->zy + matrix->posY;
        }

        soundData->worldPos = worldPoint;
        zMath::MatStackPopPtr();
        return 0;
    }

}
