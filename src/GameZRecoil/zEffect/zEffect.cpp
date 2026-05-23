#include "zEffect.h"

#include "Battlesport/zUtil/zutil.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zDi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

extern "C" {
zEffect_RuntimeManager g_zEffect_RuntimeManager = {0};
float g_zEffect_RandUnitTable[200] = {0};
float g_zEffect_RandUnitScale = 0.0f;
int g_zEffect_RandTableIndex = 0;
char g_zEffectAnim_ZbdFilename[0x80] = {0};
int g_zEffectAnim_EntriesInstantiated = 0;
void *g_zEffectAnim_HeapPtr = 0;
short g_zEffectAnim_CountsPackedLoWord = 0;
short g_zEffectAnim_EntryCount = 0;
zEffectAnimEntry *g_zEffectAnim_EntryList = 0;
int g_zEffectAnim_TextIdEntryCount = 0;
zEffectAnimTextIdEntry *g_zEffectAnim_TextIdEntryList = 0;
int g_zEffectAnim_SourceFileStampCount = 0;
zEffectAnimSourceFileStamp *g_zEffectAnim_SourceFileStampList = 0;
int g_zEffectAnim_CopyNodeMode = 1;
int g_zEffectAnim_CopyNodeArg1 = 1;
int g_zEffectAnim_CopyNodeArg2 = 1;
int g_zEffectAnim_ForceCloneNonDynamicRoot = 0;
int g_zEffect_CloneCopyMode = 1;
int g_zEffect_CloneCopyChildrenMode = 1;
zClass_NodePartial *g_zEffect_World = 0;
float g_zEffect_DefaultGravity = 0.0f;
int g_zEffect_ConditionalRefPosEnabled = 0;
float g_zEffect_ConditionalRefPosX = 0.0f;
float g_zEffect_ConditionalRefPosY = 0.0f;
float g_zEffect_ConditionalRefPosZ = 0.0f;
int g_zEffect_ConditionalEffectLevel = 0;
int g_zEffect_VariantOverrideEnabled = 0;
unsigned int g_zEffect_VariantOverridePackedIds = 0;
unsigned char g_zEffect_VariantCycleId = 1;
int g_zEffect_SkipStopDelay = 0;
float g_zEffect_FrameDeltaRemainingSec = 0.0f;
int g_zEffect_Anim_DebugFrameTag = 0;
zClass_NodePartial *g_zEffect_ResourceNode = 0;
zEffectAnimActivationRecord *g_zEffectAnim_ActivationRecordTable = 0;
int g_zEffectAnim_ActivationRecordCapacity = 0;
int g_zEffectAnim_ActivationRecordCount = 0;
void(RECOIL_FASTCALL *g_zEffectAnim_ActivationDispatchCallback)(
    zEffectAnimActivationRecord *record) = 0;
unsigned int g_zEffectAnim_ActivationDispatchTagHigh = 0;
int g_zEffectAnim_RecordQueueEnabled = 0;
int g_zEffectAnim_DispatchEnabled = 0;
int g_zEffectAnim_EnableZarRegistration = 1;
char g_zEffectAnim_ZarSectionName_Anim[5] = "Anim";
char g_zEffectAnim_ZarSectionName_RunningAnim[12] = "RunningAnim";
char g_zEffectAnim_ZarSectionName_AnimActivation[15] = "AnimActivation";
}

namespace {
const unsigned int kRandUnitScaleBits = 0x38000100u;
const int kInitialActivationRecordCapacity = 1000;
const unsigned int kActivationRecordNoQueueDispatchFlag = 0x00000100u;
const unsigned int kActivationRecordQueueOverrideFlag = 0x00001000u;
const unsigned int kActivationRecordQueueOverrideValue = 0x00002000u;
const unsigned int kActivationRecordDispatchOverrideFlag = 0x00000400u;
const unsigned int kActivationRecordDispatchOverrideValue = 0x00000800u;
const unsigned int kEffectAnimWorldChildAttachedFlag = 0x00000100u;
const unsigned int kEffectAnimNeedsCopiedRootFlag = 0x00008000u;
const float kEmitterLoopTriggerClampValue = 86400.0f;
const float kEffectAnimStopDelaySkipBias = -0.01f;
const float kEffectAnimActivationSentinel = -99.0f;
const float kEffectAnimActivationSentinelTolerance = 0.1f;
const float kEffectAnimVelocityEpsilon = 0.01f;
const short kEffectAnimResetScratchRefIndex = -200;
const short kEffectAnimBoundNodeRefIndex = -100;
const char *kZeffAnimInitSourceFile = "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_init.c";
const char *kZeffAnimRunSourceFile = "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_run.c";
const char *kZeffInitSourceFile = "D:\\Proj\\GameZRecoil\\zEffect\\zeff_init.c";
const char *kAnimationNodeNotFoundMessage =
    "Animation node not found.\n  Animation: %s; Node: %s\n";
void *const kSaveActivationRecordsProc = (void *)(0x00460490);
void *const kLoadActivationRecordsProc = (void *)(0x004606d0);
void *const kSaveRunningAnimRecordsProc = (void *)(0x00460f80);
void *const kLoadRunningAnimRecordsProc = (void *)(0x00461040);
void *const kSaveAnimRecordsProc = (void *)(0x00461430);
void *const kLoadAnimRecordsProc = (void *)(0x00461670);

void FreeIfSet(void *ptr) {
    if (ptr != 0) {
        free(ptr);
    }
}

struct zEffectAnimZbdHeaderBlock {
    int entriesInstantiated;
    void *heapPtr;
    short countsPackedLoWord;
    short entryCount;
    zEffectAnimEntry *entryList;
    int textIdEntryCount;
    zEffectAnimTextIdEntry *textIdEntryList;
    zClass_NodePartial *worldNode;
    float defaultGravity;
    int conditionalRefPosEnabled;
    int variantOverrideEnabled;
    float conditionalRefPosX;
    float conditionalRefPosY;
    float conditionalRefPosZ;
    unsigned int variantOverridePackedIds;
    float frameDeltaRemainingSec;
};

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimZbdHeaderBlock) == 0x3c);

struct zEffectAnimTrackedNodeSaveRecord {
    int nodeIndex;
    int activeFlag;
    int usesCachedMatrix;
    float transform[12];
    int diFlagBits;
    int diUserValue;
};

struct zEffectAnimActivationSaveRecord {
    zEffectAnimActivationRecord base;
    unsigned char unknown_50[4];
    unsigned char savedActivationState;
    unsigned char trackedNodeCount;
    unsigned char unknown_56[2];
    zEffectAnimTrackedNodeSaveRecord trackedNodes[1];
};

struct zEffectAnimSaveHeader {
    zEffectAnimActivationRecord base;
    int entryTableIndex;
    unsigned char savedActivationState;
    unsigned char trackedNodeCount;
    unsigned char unknown_56[2];
};

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimTrackedNodeSaveRecord) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationSaveRecord, savedActivationState) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationSaveRecord, trackedNodeCount) == 0x55);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationSaveRecord, trackedNodes) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSaveHeader) == 0x58);

struct zEffectAnimRunningSaveHeader {
    int entryTableIndex;
    int matchSavedRootNode;
    char entryName[0x20];
    int rootNodeIndex;
    int nodeRefAIndex;
    zVec3 refVecA;
    int nodeRefBIndex;
    zVec3 refVecB;
    unsigned char activationState;
    unsigned char unknown_4d[3];
    float triggerCurrentValue;
    float activationCountdown;
    float velocityX;
    float velocityY;
    float velocityZ;
    unsigned char runtimeSurfaceCount;
    unsigned char lightRefCount;
    unsigned char soundRefCount;
    unsigned char reserved;
};

struct zEffectAnimRuntimeNodeSaveRecord {
    char name[0x24];
    int isAttached;
    float posX;
    float posY;
    float posZ;
    int parentNodeIndex;
};

struct zEffectAnimSoundNodeSaveRecord {
    char name[0x24];
    int isAttached;
    int hasPosition;
    float posX;
    float posY;
    float posZ;
    int parentNodeIndex;
};

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRunningSaveHeader) == 0x68);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRuntimeNodeSaveRecord) == 0x38);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSoundNodeSaveRecord) == 0x3c);

bool ReadOne(FILE *stream, void *data, size_t size) {
    return fread(data, size, 1, stream) == 1;
}

bool ReadArrayRaw(FILE *stream, void **outData, size_t stride, unsigned int count) {
    *outData = 0;
    if (count == 0) {
        return true;
    }

    void *data = malloc(stride * count);
    if (data == 0) {
        return false;
    }

    *outData = data;
    return fread(data, stride, count, stream) == count;
}

template <typename T> bool ReadArray(FILE *stream, T **outData, unsigned int count) {
    void *data = 0;
    if (!ReadArrayRaw(stream, &data, sizeof(T), count)) {
        return false;
    }

    *outData = static_cast<T *>(data);
    return true;
}

bool ReadEventStream(FILE *stream, zEffectAnimSurfaceRuntime *runtime) {
    runtime->eventStream = 0;
    if (runtime->eventStreamSize <= 0) {
        return true;
    }

    runtime->eventStream = malloc(runtime->eventStreamSize);
    if (runtime->eventStream == 0) {
        return false;
    }

    return ReadOne(stream, runtime->eventStream, runtime->eventStreamSize);
}

struct MemoryReader {
    const unsigned char *cursor;
    const unsigned char *end;
};

bool ReadMemory(MemoryReader *reader, void *data, size_t size) {
    if (reader->cursor > reader->end ||
        size > static_cast<size_t>(reader->end - reader->cursor)) {
        return false;
    }

    memcpy(data, reader->cursor, size);
    reader->cursor += size;
    return true;
}

zReader::Node *zReaderArrayBase(zReader::Node *node) {
    return node->value.nodes;
}

int zReaderArrayCount(zReader::Node *node) {
    return zReaderArrayBase(node)->value.i32;
}

char *zReaderArrayStringAt(zReader::Node *node, int index) {
    return zReaderArrayBase(node)[index].value.str;
}

void ReportAnimationNodeNotFound(zEffectAnimEntry *self, int sourceLine,
                                 const char *nodeName) {
    zError::ReportOld(0x400, kZeffAnimInitSourceFile, sourceLine, kAnimationNodeNotFoundMessage,
                      self, nodeName);
    self->activationState = 5;
}

int ResolveEmitterEventEntryIndex(zEffectAnimEmitterEvent *event) {
    if (event->cachedEntryIndex <= 0) {
        zEffectAnimEntry *entry = g_zEffectAnim_EntryList;
        for (int i = 0; i < g_zEffectAnim_EntryCount; ++i, ++entry) {
            if (strcmp(entry->name, event->animName) == 0) {
                event->cachedEntryIndex = i;
                break;
            }
        }
    }

    return event->cachedEntryIndex;
}

int ResolveAnimEntryIndexByName(short *cachedIndex, const char *animName) {
    if (*cachedIndex <= 0) {
        zEffectAnimEntry *entry = g_zEffectAnim_EntryList;
        for (int i = 0; i < g_zEffectAnim_EntryCount; ++i, ++entry) {
            if (strcmp(entry->name, animName) == 0) {
                *cachedIndex = static_cast<short>(i);
                break;
            }
        }
    }

    return *cachedIndex;
}

int ResolveSurfaceRuntimeIndex(zEffectAnimEntry *self, zEffectSurfaceControlEvent *event) {
    if (event->surfaceSlotIndex < 0) {
        for (int i = 0; i < self->runtimeSequenceCount; ++i) {
            if (strcmp(self->runtimeList[i].sequenceName, event->sequenceName) == 0) {
                event->surfaceSlotIndex = i;
                break;
            }
        }
    }

    return event->surfaceSlotIndex;
}

void CopyPackedVariantOverrideToCurrentTag() {
    memcpy(&g_Variant_CurrentTag, &g_zEffect_VariantOverridePackedIds,
                sizeof(g_Variant_CurrentTag));
}

void AdvanceVariantCycleDelay(zEffectAnimEntry *self) {
    self->variantCycleDelay = g_zEffect_VariantCycleId;
    ++g_zEffect_VariantCycleId;

    const int maxCycleId = (static_cast<int>(self->priority) * 10 + 3) >> 2;
    if (static_cast<int>(g_zEffect_VariantCycleId) > maxCycleId) {
        g_zEffect_VariantCycleId = 1;
    }
}

void ResetActivatedBoundTransform(zEffectAnimEntry *activatedEntry) {
    if ((activatedEntry->flags & kEffectAnimWorldChildAttachedFlag) == 0) {
        return;
    }

    zClass_NodePartial *const boundNode = activatedEntry->boundNode;
    if (boundNode->classId == 1) {
        zClass_Camera::gwCameraSetTarget(boundNode, 0.0f, 0.0f, 0.0f);
        if ((activatedEntry->flags & 0x00000200u) == 0) {
            zClass_Camera::gwCameraSetPosition(activatedEntry->boundNode, 0.0f, 0.0f, 0.0f);
        }
    } else if (boundNode->classId == 5) {
        zClass_Object3D::gwObject3DSetPosition(boundNode, 0.0f, 0.0f, 0.0f);
        if ((activatedEntry->flags & 0x00000200u) == 0) {
            zClass_Object3D::gwObject3DSetRotation(activatedEntry->boundNode, 0.0f, 0.0f, 0.0f);
        }
    }
}

bool VelocityIsActive(float x, float y, float z) {
    return fabs(x) > kEffectAnimVelocityEpsilon || fabs(y) > kEffectAnimVelocityEpsilon ||
           fabs(z) > kEffectAnimVelocityEpsilon;
}

float BeamLengthFromLengthSq(float lengthSq) {
    int bits = 0;
    memcpy(&bits, &lengthSq, sizeof(bits));
    bits = (bits >> 1) + 0x1fc00000;

    float length = 0.0f;
    memcpy(&length, &bits, sizeof(length));
    return length;
}

float ClampUnitFloat(float value) {
    if (value < 0.0f) {
        return 0.0f;
    }
    if (value > 1.0f) {
        return 1.0f;
    }
    return value;
}

unsigned char UnitFloatToByte(float value) {
    return static_cast<unsigned char>(static_cast<int>(value * 255.0f + 0.5f));
}

void SetNodeDiBlendScale(zClass_NodePartial *node, float blendScale) {
    if (node == 0 || node->userDataOrDiRef == 0) {
        return;
    }

    zDiPartial *const di = (zDiPartial *)(node->userDataOrDiRef);
    di->flags |= 0x08;
    di->blendScale = blendScale;
    if (di->blendScale > 1.0f) {
        di->blendScale = 1.0f;
    } else if (di->blendScale < 0.00001f) {
        di->flags &= ~0x08;
    }
}

void AddNodeDiBlendScale(zClass_NodePartial *node, float blendDelta) {
    if (node == 0 || node->userDataOrDiRef == 0) {
        return;
    }

    zDiPartial *const di = (zDiPartial *)(node->userDataOrDiRef);
    SetNodeDiBlendScale(node, di->blendScale + blendDelta);
}

float NextEffectRandUnit() {
    const float value = g_zEffect_RandUnitTable[g_zEffect_RandTableIndex];
    g_zEffect_RandTableIndex = (g_zEffect_RandTableIndex + 1) % 200;
    return value;
}

zVec3 TransformByCurrentMatrix(const zVec3 *vec) {
    const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    zVec3 result = {vec->x * matrix->xx + vec->y * matrix->yx + vec->z * matrix->zx + matrix->posX,
            vec->x * matrix->xy + vec->y * matrix->yy + vec->z * matrix->zy + matrix->posY,
            vec->x * matrix->xz + vec->y * matrix->yz + vec->z * matrix->zz + matrix->posZ};
    return result;
}

zVec3 TransformNodeBasisVector(zClass_NodePartial *node, const zVec3 *vec) {
    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    gwNode::BuildNodeToAncestorMatrix(node, 2);

    zMat4x3 basisMatrix = {0};
    basisMatrix.xx = slotBuffer.xx;
    basisMatrix.xy = slotBuffer.yx;
    basisMatrix.xz = slotBuffer.zx;
    basisMatrix.yx = slotBuffer.xy;
    basisMatrix.yy = slotBuffer.yy;
    basisMatrix.yz = slotBuffer.zy;
    basisMatrix.zx = slotBuffer.xz;
    basisMatrix.zy = slotBuffer.yz;
    basisMatrix.zz = slotBuffer.zz;
    zMath::MatLoadCurrentFrom(&basisMatrix);

    const zVec3 out = TransformByCurrentMatrix(vec);
    zMath::MatStackPopPtr();
    return out;
}

short ResolveNodeAnimRuntimeIndex(zEffectAnimEntry *self, zEffectNodeAnimEvent *event) {
    if (event->packedRuntimeIndex < 0) {
        for (int i = 0; i < self->runtimeSequenceCount; ++i) {
            if (strcmp(self->runtimeList[i].sequenceName, event->targetName) == 0) {
                event->packedRuntimeIndex = static_cast<short>(i);
                break;
            }
        }
    }

    return event->packedRuntimeIndex;
}

void StoreResetScratchVec(zEffectAnimEntry *entry, size_t firstIndex, const zVec3 *vec) {
    if (vec != 0) {
        memcpy(&entry->resetScratch[firstIndex], &vec->x, sizeof(vec->x));
        memcpy(&entry->resetScratch[firstIndex + 1], &vec->y, sizeof(vec->y));
        memcpy(&entry->resetScratch[firstIndex + 2], &vec->z, sizeof(vec->z));
    } else {
        entry->resetScratch[firstIndex] = 0;
        entry->resetScratch[firstIndex + 1] = 0;
        entry->resetScratch[firstIndex + 2] = 0;
    }
}

zVec3 LoadResetScratchVec(zEffectAnimEntry *entry, size_t firstIndex) {
    zVec3 value = {0};
    memcpy(&value.x, &entry->resetScratch[firstIndex], sizeof(value.x));
    memcpy(&value.y, &entry->resetScratch[firstIndex + 1], sizeof(value.y));
    memcpy(&value.z, &entry->resetScratch[firstIndex + 2], sizeof(value.z));
    return value;
}

zClass_NodePartial *LoadResetScratchNode(zEffectAnimEntry *entry, size_t index) {
    return (zClass_NodePartial *)(
        static_cast<unsigned int>(entry->resetScratch[index]));
}

zClass_NodePartial *ResolveNodeRefOrResetScratch(zEffectAnimEntry *self,
                                                 short nodeRefIndex) {
    if (nodeRefIndex > 0) {
        return self->nodeRefList[nodeRefIndex].node;
    }
    if (nodeRefIndex == -200) {
        return LoadResetScratchNode(self, 0);
    }
    return 0;
}

int DispatchSequenceEvent(zEffectAnimEntry *self,
                                   zEffectAnimSurfaceRuntime *sequenceRuntime,
                                   zEffectAnimEventHeader *currentEvent) {
    switch (currentEvent->eventType) {
    case 1:
        return zEffect::HandleSampleRefOffsetEvent(
            self, (zEffectAnimRefOffsetEvent *)(currentEvent));
    case 2:
        return zEffect::HandleSoundEvent(self,
                                         (zEffectAnimSoundEvent *)(currentEvent));
    case 3:
        return zEffect::HandleEffectTemplateOffsetEvent(
            self, (zEffectAnimRefOffsetEvent *)(currentEvent));
    case 4:
        return zEffect::HandleLightEvent(self,
                                         (zEffectAnimLightEvent *)(currentEvent));
    case 5:
        return zEffect::HandleLightAnimEvent(
            self, sequenceRuntime,
            (zEffectLightRangeSpecularAnimEvent *)(currentEvent));
    case 6:
        return zEffect::HandleActivateEvent(self,
                                            (zEffectActivateEvent *)(currentEvent));
    case 7:
        return zEffect::HandlePositionEvent(
            self, (zEffectTransformEvent *)(currentEvent));
    case 8:
        return zEffect::HandleNodeScaleEvent(
            self, (zEffectNodeScaleEvent *)(currentEvent));
    case 9:
        return zEffect::HandleRotationEvent(
            self, (zEffectTransformEvent *)(currentEvent));
    case 0x0a:
        return zEffect::HandleNodeAnimEvent(self, sequenceRuntime,
                                            (zEffectNodeAnimEvent *)(currentEvent));
    case 0x0b:
        return zEffect::AnimateNodeOverTime(self, sequenceRuntime,
                                            (zEffectNodeAnimEvent *)(currentEvent));
    case 0x0c:
        return zEffect_Anim::AdvanceKeyframe(
            self, sequenceRuntime, (zEffectKeyframeEvent *)(currentEvent));
    case 0x0d:
        return zEffect_Anim::EvaluateKeyframe(
            self, (zEffectEvaluateKeyframeEvent *)(currentEvent));
    case 0x0e:
        return zEffect_Anim::RunKeyframes(
            self, sequenceRuntime, (zEffectRunKeyframeEvent *)(currentEvent));
    case 0x1b:
        return zEffect::HandleEmitterStopEvent(
            self, (zEffectAnimEmitterEvent *)(currentEvent));
    case 0x14:
        return zEffect::HandleCameraParamsEvent(
            self, sequenceRuntime, (zEffectCameraEvent *)(currentEvent));
    case 0x15:
        return zEffect::AnimateCameraParamsOverTime(
            self, sequenceRuntime, (zEffectCameraAnimEvent *)(currentEvent));
    case 0x0f:
        return zEffect::HandleAddChildEvent(
            self, (zEffectParentChildEvent *)(currentEvent));
    case 0x10:
        return zEffect::HandleRemoveChildEvent(
            self, (zEffectParentChildEvent *)(currentEvent));
    case 0x11:
        return zEffect::HandleAttachEvent(self, sequenceRuntime,
                                          (zEffectAttachEvent *)(currentEvent));
    case 0x12:
        return zEffect::HandleDetachEvent(self, sequenceRuntime,
                                          (zEffectBeamDetachEvent *)(currentEvent));
    case 0x16:
        return zEffect::HandleSurfaceStopEvent(
            self, (zEffectSurfaceControlEvent *)(currentEvent));
    case 0x17:
        return zEffect::HandleSurfacePlayEvent(
            self, (zEffectSurfaceControlEvent *)(currentEvent));
    case 0x18:
        return zEffect::HandleSurfaceRefEvent(
            self, sequenceRuntime, (zEffectSurfaceRefEvent *)(currentEvent));
    case 0x24:
        return zEffect::HandleScreenColorFxEvent(
            self, sequenceRuntime, (zEffectScreenColorFxEvent *)(currentEvent));
    case 0x25:
        return zEffect::HandleScreenOverlayFxEvent(
            self, sequenceRuntime, (zEffectScreenOverlayFxEvent *)(currentEvent));
    case 0x13:
        return zEffect::HandleTransformRefsEvent(
            self, (zEffectTransformRefsEvent *)(currentEvent));
    case 0x19:
        return zEffect::HandleNamedAnimStopEvent(
            self, (zEffectAnimEmitterEvent *)(currentEvent));
    case 0x1a:
        return zEffect::HandleEmitterPlayEvent(
            self, (zEffectAnimEmitterEvent *)(currentEvent));
    case 0x1c:
        return zEffect::HandleFogEvent(self, (zEffectFogEvent *)(currentEvent));
    case 0x1e:
        return zEffect::HandleEmitterLoopEvent(
            self, sequenceRuntime, (zEffectAnimLoopEvent *)(currentEvent));
    case 0x1f:
        return zEffect::HandleConditionalChainEvent(
            self, sequenceRuntime, (zEffectConditionalEvent *)(currentEvent));
    case 0x20:
    case 0x21:
        return zEffect::SkipConditionalChainToEnd(self, sequenceRuntime, currentEvent);
    case 0x22:
    case 0x27:
    case 0x28:
        return zEffect::HandleNoOpMarkerEvent(self, sequenceRuntime, currentEvent);
    case 0x23:
        return zEffect::HandleCallbackEvent(
            self, sequenceRuntime, (zEffectAnimCallbackEvent *)(currentEvent));
    case 0x26:
        return zEffect::HandleTopMessageEvent(
            self, (zEffectTopMessageEvent *)(currentEvent));
    default:
        zError::ReportOld(0x400, kZeffAnimRunSourceFile, 0x171c,
                          "Invalid Sequence Event\n  Animation: %s\n", self);
        return -1;
    }
}
} // namespace

// Reimplements 0x45e0d0: zEffectAnimEntry::SetOnStateDoneCallback
// (D:\Proj\GameZRecoil\zEffect\zeff_anim.c)
RECOIL_NOINLINE void RECOIL_FASTCALL
zEffectAnimEntry::SetOnStateDoneCallback(zEffectAnimEntry *self, void *callback, void *user) {
    if (self != 0) {
        self->eventCallback = (zEffectAnimEventCallback)(callback);
        self->eventCallbackContext = user;
    }
}

namespace zEffectAnim {
// Reimplements 0x45d7a0: zEffectAnim::ResetActivationPrereqCount
RECOIL_NOINLINE void RECOIL_FASTCALL ResetActivationPrereqCount(zEffectAnimEntry *self) {
    self->activationPrereqCount = 0;
}

// Reimplements 0x45db20: zEffectAnim::CheckActivationPrereqs (zeff_anim.c)
RECOIL_NOINLINE int RECOIL_FASTCALL CheckActivationPrereqs(zEffectAnimEntry *self) {
    if (self->activationPrereqCount == 0) {
        return 1;
    }

    int matchedPrereqTotal = 0;
    for (int i = 0; i < self->activationPrereqCount; ++i) {
        zEffectAnimActivationPrereq *const prereq = &self->activationPrereqList[i];

        if (prereq->mode == 1) {
            if (prereq->targetEntry == 0) {
                zEffectAnimEntry *candidate = g_zEffectAnim_EntryList;
                for (int entryIndex = 0; entryIndex < g_zEffectAnim_EntryCount;
                     ++entryIndex, ++candidate) {
                    if (strcmp(candidate->name, prereq->targetName) == 0) {
                        prereq->targetEntry = candidate;
                        break;
                    }
                }
            }

            zEffectAnimEntry *const targetEntry = prereq->targetEntry;
            if (targetEntry == 0) {
                return 0;
            }

            if (targetEntry->activationState == 1) {
                if (prereq->requireMatch == 0) {
                    return 0;
                }
            } else if (prereq->requireMatch != 0) {
                ++matchedPrereqTotal;
            }
        } else if (prereq->mode == 2) {
            zClass_NodePartial *const targetNode = prereq->targetNode;
            if (targetNode != 0) {
                const int nodeFlagValue = (targetNode->flags >> 2) & 1;
                int expectedValue = 0;
                memcpy(&expectedValue, prereq->targetName, sizeof(expectedValue));
                if (nodeFlagValue == expectedValue) {
                    if (prereq->requireMatch != 0) {
                        ++matchedPrereqTotal;
                    }
                } else if (prereq->requireMatch == 0) {
                    return 0;
                }
            }
        }
    }

    return matchedPrereqTotal >= self->activationPrereqMinimumMatchCount ? 1 : 0;
}

// Reimplements 0x45d6c0: zEffectAnim::ResetForNode (zeff_anim.c)
RECOIL_NOINLINE int RECOIL_FASTCALL ResetForNode(zEffectAnimEntry *self) {
    if (self == 0) {
        return -1;
    }

    zClass_NodePartial *const rootNode = zClass_Class::gwNodeGetRoot(self->boundNode);
    if (rootNode == 0) {
        return -1;
    }

    if (rootNode->classId == 2 || rootNode->classId == 1) {
        self->flags &= ~0x00000100u;
    } else {
        if (zClass_World::AddChildAtGrid(g_zEffect_World, self->boundNode) != 0) {
            return -1;
        }
        self->flags |= 0x00000100u;
    }

    self->triggerCurrentValue = 0.0f;
    zEffect_Anim::CaptureNodeStates(self);

    zEffectAnimSurfaceRuntime *runtime = self->runtimeList;
    for (int i = 0; i < self->runtimeSequenceCount; ++i, ++runtime) {
        runtime->loopIterationCount = 0;
        runtime->loopElapsedSec = 0.0f;
        zEffect::HandleEmitterResetEvent(runtime);
    }

    return 0;
}

// Reimplements 0x45d3d0: zEffectAnim::FinalizeStop (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL FinalizeStop(zEffectAnimEntry *self) {
    if (self == 0 || self->activationState == 5) {
        return -1;
    }

    if ((self->flags & kEffectAnimWorldChildAttachedFlag) != 0) {
        if (zClass_World::RemoveChildAtGrid(g_zEffect_World, self->boundNode) != 0) {
            return -1;
        }

        self->flags &= ~kEffectAnimWorldChildAttachedFlag;
    }

    zEffect::CleanupLightRefs(self);
    zEffect::CleanupSoundRefs(self);

    for (int i = 0; i < self->runtimeRefCount; ++i) {
        zEffectAnimRuntimeRef *const runtimeRef = &self->runtimeRefList[i];
        zEffectAnimEntry *const cachedChildEntry = runtimeRef->cachedChildEntry;
        if (cachedChildEntry != 0 && runtimeRef->stopCachedChildOnCleanup != 0) {
            Stop(cachedChildEntry);
        }
        runtimeRef->cachedChildEntry = 0;
    }

    if (self->runtimeNode != 0) {
        zClass_Class::gwNodeSetActionCallback(self->runtimeNode, 0);
    }

    const unsigned char activationState = self->activationState;
    if (activationState != 5 && activationState != 4) {
        self->activationState = activationState == 6 ? 4 : 1;
    }

    memcpy(&self->activationCountdown, &self->triggerContext,
                sizeof(self->activationCountdown));
    return 0;
}

// Reimplements 0x45d4c0: zEffectAnim::RunStopSequenceCallback
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL RunStopSequenceCallback(zClass_NodePartial *node) {
    int stopSequenceFinished = 1;
    if (node == 0) {
        return -1;
    }

    zEffectAnimEntry *const entry = (zEffectAnimEntry *)(node->callbackContext);
    if (entry == 0) {
        return -1;
    }

    entry->triggerCurrentValue += g_FrameDeltaTimeSec;
    if (entry->surfacePrimary.eventStream != 0) {
        g_zEffect_FrameDeltaRemainingSec = g_FrameDeltaTimeSec;
        const unsigned char runState = entry->surfacePrimary.runState;
        if (runState == 0 || runState == 1) {
            if (zEffect_Anim::RunSequenceEvents(entry, &entry->surfacePrimary) != 0) {
                entry->activationState = 5;
                zError::ReportOld(0x400, kZeffAnimRunSourceFile, 0x196d,
                                  "Corrupt animation:\n  Animation: %s; Sequence: %s\n", entry,
                                  &entry->surfacePrimary);
            }
        }

        const unsigned char runStateAfterDispatch = entry->surfacePrimary.runState;
        if (runStateAfterDispatch == 0 || runStateAfterDispatch == 1) {
            stopSequenceFinished = 0;
        }
    }

    if (stopSequenceFinished != 0) {
        FinalizeStop(entry);
    }

    return 0;
}

// Reimplements 0x45d570: zEffectAnim::StopAndCleanup (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL StopAndCleanup(zEffectAnimEntry *self,
                                                            zClass_NodePartial *targetNode,
                                                            int immediateCleanup) {
    if (self == 0 || self->activationState == 5) {
        return -1;
    }

    zEffectAnimEntry *entry = self;
    unsigned char activationState = self->activationState;
    if (targetNode != 0 && self->boundNode != targetNode) {
        while (entry->runtimeSibling != 0 && entry->activationState == 2) {
            entry = entry->runtimeSibling;
        }

        if (entry->activationState == 2) {
            zEffectAnimEntry *const clonedEntry = CloneEntryForNode(entry, targetNode);
            entry->runtimeSibling = clonedEntry;
            if (clonedEntry == 0) {
                return -1;
            }
            entry = clonedEntry;
        }

        if (RebindEntryToNode(entry, targetNode) == 0) {
            return -1;
        }
    }

    if (activationState == 2 || activationState == 6) {
        Stop(self);
    }

    entry->activationState = 0;
    if (immediateCleanup != 0) {
        entry->flags |= 0x40u;
    } else {
        entry->flags &= ~0x40u;
    }

    if (entry->activationState != 1) {
        if ((entry->flags & 0x40u) != 0) {
            zEffect_Anim::RestoreNodeStates(entry);
        }

        if (entry->surfacePrimary.eventStream != 0) {
            zEffect::HandleEmitterResetEvent(&entry->surfacePrimary);
            zEffect_Anim::RunSequenceEvents(entry, &entry->surfacePrimary);
            const unsigned char runState = entry->surfacePrimary.runState;
            if (runState == 0 || runState == 1) {
                zClass_Class::gwNodeSetActionCallbackTail(
                    entry->runtimeNode, (void *)(&RunStopSequenceCallback));
                return 0;
            }
        }

        FinalizeStop(entry);
    }

    return 0;
}

// Reimplements 0x45c040: zEffectAnim::Stop (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL Stop(zEffectAnimEntry *self) {
    if (self == 0) {
        return -1;
    }

    const unsigned char activationState = self->activationState;
    if ((activationState == 2 || activationState == 6) && self->runtimeNode != 0) {
        if (self->triggerBaseValue >= 0.0f) {
            if (g_zEffect_SkipStopDelay != 0) {
                self->triggerCurrentValue = self->triggerBaseValue - kEffectAnimStopDelaySkipBias;
            } else {
                self->triggerCurrentValue = 0.0f;
            }

            if (self->triggerBaseValue > self->triggerCurrentValue) {
                zClass_Class::gwNodeSetActionCallbackTail(
                    self->runtimeNode, (void *)(&RunStopDelayCallback));
                return 0;
            }

            RunStopDelayCallback(self->runtimeNode);
            return 0;
        }

        FinalizeStop(self);
        const unsigned char stateAfterFinalize = self->activationState;
        if (stateAfterFinalize != 5 && stateAfterFinalize != 4) {
            self->activationState = stateAfterFinalize == 6 ? 4 : 3;
        }
    }

    return 0;
}

// Reimplements 0x45d770: zEffectAnim::RunStopDelayCallback
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL RunStopDelayCallback(zClass_NodePartial *node) {
    zEffectAnimEntry *const entry =
        node != 0 ? (zEffectAnimEntry *)(node->callbackContext) : 0;
    if (entry == 0) {
        return 0;
    }

    entry->triggerCurrentValue += g_FrameDeltaTimeSec;
    if (entry->triggerCurrentValue >= entry->triggerBaseValue) {
        return zEffect_Anim::NodeActionCallback(entry, 0);
    }

    return 0;
}

// Reimplements 0x45d930: zEffectAnim::ActivateRuntime (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL ActivateRuntime(zEffectAnimEntry *self,
                                                                  zClass_NodePartial *targetNode) {
    int immediateCleanup = 0;
    if (self == 0) {
        return 0;
    }

    const unsigned char activationState = self->activationState;
    if (activationState == 6 || activationState == 4 || activationState == 5) {
        return 0;
    }

    zEffectAnimEntry *entryToActivate = self;
    if (activationState == 3 || activationState == 2) {
        if (fabs(self->triggerBaseValue - kEffectAnimActivationSentinel) <
            kEffectAnimActivationSentinelTolerance) {
            return 0;
        }

        if (activationState == 2) {
            if ((self->flags & kEffectAnimWorldChildAttachedFlag) != 0) {
                while (entryToActivate->runtimeSibling != 0 &&
                       entryToActivate->activationState == 2) {
                    entryToActivate = entryToActivate->runtimeSibling;
                }

                if (entryToActivate->activationState == 2) {
                    zEffectAnimEntry *const clonedEntry =
                        CloneEntryForNode(entryToActivate, targetNode);
                    entryToActivate->runtimeSibling = clonedEntry;
                    if (clonedEntry == 0) {
                        return 0;
                    }
                    entryToActivate = clonedEntry;
                    immediateCleanup = 1;
                }
            } else {
                if (targetNode == 0) {
                    return 0;
                }

                zEffectAnimEntry *siblingTail = self;
                zEffectAnimEntry *matchedEntry = 0;
                int targetNodeMatched = 0;
                for (zEffectAnimEntry *cursor = self; cursor != 0;
                     cursor = cursor->runtimeSibling) {
                    if (cursor->activationState == 2) {
                        if (cursor->boundNode == targetNode) {
                            targetNodeMatched = 1;
                            break;
                        }
                    } else {
                        matchedEntry = cursor;
                    }

                    siblingTail = cursor;
                }

                if (targetNodeMatched != 0) {
                    return 0;
                }

                if (matchedEntry == 0) {
                    matchedEntry = CloneEntryForNode(siblingTail, targetNode);
                    siblingTail->runtimeSibling = matchedEntry;
                    if (matchedEntry == 0) {
                        return 0;
                    }
                }

                entryToActivate = matchedEntry;
            }
        }
    }

    if (targetNode != 0 && RebindEntryToNode(entryToActivate, targetNode) == 0) {
        return 0;
    }

    if (CheckActivationPrereqs(entryToActivate) == 0) {
        return 0;
    }

    if ((entryToActivate->flags & 0x20u) != 0) {
        StopAndCleanup(entryToActivate, 0, immediateCleanup);
    }

    if (ResetForNode(entryToActivate) != 0) {
        return 0;
    }

    if (entryToActivate->runtimeNode == 0) {
        entryToActivate->runtimeNode = zClass_Object3D::gwObject3DInit();

        char runtimeNodeName[0x24];
        sprintf(runtimeNodeName, "_%s", entryToActivate->name);
        zClass_Class::gwNodeSetName(entryToActivate->runtimeNode, runtimeNodeName);
        if (entryToActivate->runtimeNode == 0) {
            return 0;
        }
        zClass_Class::gwNodeSetPriority(entryToActivate->runtimeNode, entryToActivate->priority);
    }

    entryToActivate->runtimeNode->callbackContext =
        (zClass_NodePartial *)(entryToActivate);
    zClass_Class::gwNodeSetActionCallbackTail(entryToActivate->runtimeNode,
                                              (void *)(&zEffect_Anim::RunSequence));
    entryToActivate->activationState = 2;
    return entryToActivate;
}

// Reimplements 0x45d7b0: zEffectAnim::SetTransformRotAndVelocity
// (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetTransformRotAndVelocity(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, float posX, float posY, float posZ,
    float rotX, float rotY, float rotZ, float velocityX, float velocityY, float velocityZ) {
    zEffectAnimEntry *const activatedEntry = ActivateRuntime(self, boundNode);
    if (activatedEntry == 0) {
        return 0;
    }

    if ((activatedEntry->flags & kEffectAnimWorldChildAttachedFlag) != 0) {
        zClass_NodePartial *const activatedBoundNode = activatedEntry->boundNode;
        if (activatedBoundNode->classId == 1) {
            zClass_Camera::gwCameraSetTarget(activatedBoundNode, posX, posY, posZ);
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Camera::gwCameraSetPosition(activatedEntry->boundNode, rotX, rotY, rotZ);
            }
        } else if (activatedBoundNode->classId == 5) {
            zClass_Object3D::gwObject3DSetPosition(activatedBoundNode, posX, posY, posZ);
            if ((activatedEntry->flags & 0x00000200u) == 0) {
                zClass_Object3D::gwObject3DSetRotation(activatedEntry->boundNode, rotX, rotY, rotZ);
            }
        }
    }

    if (fabs(velocityX) > kEffectAnimVelocityEpsilon ||
        fabs(velocityY) > kEffectAnimVelocityEpsilon ||
        fabs(velocityZ) > kEffectAnimVelocityEpsilon) {
        activatedEntry->flags |= 0x80u;
    } else {
        activatedEntry->flags &= ~0x80u;
    }

    activatedEntry->velocityX = velocityX;
    activatedEntry->velocityZ = velocityZ;
    activatedEntry->velocityY = velocityY;
    memset(activatedEntry->resetScratch, 0, sizeof(activatedEntry->resetScratch));

    QueueCmdType1TransformRotVelocity(self, boundNode, posX, posY, posZ, rotX, rotY, rotZ,
                                      velocityX, velocityY, velocityZ);
    return activatedEntry;
}

// Reimplements 0x45dc70: zEffectAnim::SetTransformRotAndVelocity_Thunk
// (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetTransformRotAndVelocity_Thunk(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, float posX, float posY, float posZ,
    float rotX, float rotY, float rotZ, float velocityX, float velocityY, float velocityZ) {
    return SetTransformRotAndVelocity(self, boundNode, posX, posY, posZ, rotX, rotY, rotZ,
                                      velocityX, velocityY, velocityZ);
}

// Reimplements 0x45dcb0: zEffectAnim::SetVelocity
// (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetVelocity(zEffectAnimEntry *self,
                                                              zClass_NodePartial *boundNode,
                                                              float velocityX, float velocityY,
                                                              float velocityZ) {
    zEffectAnimEntry *const activatedEntry = ActivateRuntime(self, boundNode);
    if (activatedEntry == 0) {
        return 0;
    }

    ResetActivatedBoundTransform(activatedEntry);
    if (VelocityIsActive(velocityX, velocityY, velocityZ)) {
        activatedEntry->flags |= 0x80u;
    } else {
        activatedEntry->flags &= ~0x80u;
    }

    activatedEntry->velocityX = velocityX;
    activatedEntry->velocityZ = velocityZ;
    activatedEntry->velocityY = velocityY;
    memset(activatedEntry->resetScratch, 0, sizeof(activatedEntry->resetScratch));

    QueueCmdType2Velocity(self, boundNode, velocityX, velocityY, velocityZ);
    return activatedEntry;
}

// Reimplements 0x45dde0: zEffectAnim::SetVelocity_Thunk
// (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetVelocity_Thunk(zEffectAnimEntry *self,
                                                                    zClass_NodePartial *boundNode,
                                                                    float velocityX,
                                                                    float velocityY,
                                                                    float velocityZ) {
    return SetVelocity(self, boundNode, velocityX, velocityY, velocityZ);
}

// Reimplements 0x45de00: zEffectAnim::SetPositionRefAndVelocity
// (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetPositionRefAndVelocity(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNode,
    const zVec3 *refVec, const zVec3 *velocityVec) {
    zEffectAnimEntry *const activatedEntry = ActivateRuntime(self, boundNode);
    if (activatedEntry == 0) {
        return 0;
    }

    ResetActivatedBoundTransform(activatedEntry);
    activatedEntry->resetScratch[0] =
        static_cast<unsigned int>((unsigned int)(refNode));
    StoreResetScratchVec(activatedEntry, 1, refVec);
    activatedEntry->resetScratch[4] = 0;
    activatedEntry->resetScratch[5] = 0;
    activatedEntry->resetScratch[6] = 0;
    activatedEntry->resetScratch[7] = 0;

    if (velocityVec != 0 &&
        VelocityIsActive(velocityVec->x, velocityVec->y, velocityVec->z)) {
        activatedEntry->flags |= 0x80u;
        activatedEntry->velocityX = velocityVec->x;
        activatedEntry->velocityY = velocityVec->y;
        activatedEntry->velocityZ = velocityVec->z;
    } else {
        activatedEntry->flags &= ~0x80u;
        activatedEntry->velocityZ = 0.0f;
        activatedEntry->velocityY = 0.0f;
        activatedEntry->velocityX = 0.0f;
    }

    QueueCmdType3PositionRefAndVelocity(self, boundNode, refNode, refVec, velocityVec);
    return activatedEntry;
}

// Reimplements 0x45df70: zEffectAnim::SetPositionRefAndVelocity_Thunk
// (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetPositionRefAndVelocity_Thunk(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNode,
    const zVec3 *refVec, const zVec3 *velocityVec) {
    return SetPositionRefAndVelocity(self, boundNode, refNode, refVec, velocityVec);
}

// Reimplements 0x45df90: zEffectAnim::SetTransformRefs
// (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetTransformRefs(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNodeA,
    const zVec3 *refVecA, zClass_NodePartial *refNodeB, const zVec3 *refVecB) {
    zEffectAnimEntry *const activatedEntry = ActivateRuntime(self, boundNode);
    if (activatedEntry == 0) {
        return 0;
    }

    ResetActivatedBoundTransform(activatedEntry);
    activatedEntry->resetScratch[0] =
        static_cast<unsigned int>((unsigned int)(refNodeA));
    StoreResetScratchVec(activatedEntry, 1, refVecA);
    activatedEntry->resetScratch[4] =
        static_cast<unsigned int>((unsigned int)(refNodeB));
    StoreResetScratchVec(activatedEntry, 5, refVecB);

    QueueCmdType4TransformRefs(self, boundNode, refNodeA, refVecA, refNodeB, refVecB);
    return activatedEntry;
}

// Reimplements 0x45e0b0: zEffectAnim::SetTransformRefs_Thunk
// (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetTransformRefs_Thunk(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNodeA,
    const zVec3 *refVecA, zClass_NodePartial *refNodeB, const zVec3 *refVecB) {
    return SetTransformRefs(self, boundNode, refNodeA, refVecA, refNodeB, refVecB);
}

// Reimplements 0x461970: zEffectAnim::QueueCmdType1TransformRotVelocity
// (GameZRecoil/zEffect/zeff_anim_activation.c)
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL QueueCmdType1TransformRotVelocity(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, float posX, float posY, float posZ,
    float rotX, float rotY, float rotZ, float velocityX, float velocityY, float velocityZ) {
    zEffectAnimActivationRecord *result = 0;
    const int boundNodeToken = zClass::NodePtrToValidatedIndex(boundNode);
    const unsigned int flags = self->flags;

    int recordQueueRequested = 0;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_RecordQueueEnabled != 0 &&
               (flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0' &&
               (boundNode == 0 || boundNodeToken > 0)) {
        recordQueueRequested = 1;
    }

    int dispatchRequested = 0;
    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_DispatchEnabled != 0 &&
               (flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0' &&
               (boundNode == 0 || boundNodeToken > 0)) {
        dispatchRequested = 1;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord *const record = zEffect_Anim::AllocActivationRecord();
        record->commandType = 1;
        strncpy(record->animName, self->name, sizeof(record->animName));
        record->nodeToken = boundNodeToken;
        record->params[0].f32 = posX;
        record->params[1].f32 = posY;
        record->params[2].f32 = posZ;
        record->params[3].f32 = rotX;
        record->params[4].f32 = rotY;
        record->params[5].f32 = rotZ;
        record->params[6].f32 = velocityX;
        record->params[7].f32 = velocityY;
        record->params[8].f32 = velocityZ;

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            zEffect_Anim::DiscardLastActivationRecord();
        }
    }

    return result;
}

// Reimplements 0x461aa0: zEffectAnim::QueueCmdType2Velocity
// (GameZRecoil/zEffect/zeff_anim_activation.c)
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL
QueueCmdType2Velocity(zEffectAnimEntry *self, zClass_NodePartial *boundNode, float velocityX,
                      float velocityY, float velocityZ) {
    zEffectAnimActivationRecord *result = 0;
    const int boundNodeToken = zClass::NodePtrToValidatedIndex(boundNode);
    const unsigned int flags = self->flags;

    int recordQueueRequested = 0;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_RecordQueueEnabled != 0 &&
               (flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0' &&
               (boundNode == 0 || boundNodeToken > 0)) {
        recordQueueRequested = 1;
    }

    int dispatchRequested = 0;
    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_DispatchEnabled != 0 &&
               (flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0' &&
               (boundNode == 0 || boundNodeToken > 0)) {
        dispatchRequested = 1;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord *const record = zEffect_Anim::AllocActivationRecord();
        record->commandType = 2;
        strncpy(record->animName, self->name, sizeof(record->animName));
        record->nodeToken = boundNodeToken;
        record->params[0].f32 = velocityX;
        record->params[1].f32 = velocityY;
        record->params[2].f32 = velocityZ;

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            zEffect_Anim::DiscardLastActivationRecord();
        }
    }

    return result;
}

// Reimplements 0x461ba0: zEffectAnim::QueueCmdType3PositionRefAndVelocity
// (GameZRecoil/zEffect/zeff_anim_activation.c)
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL QueueCmdType3PositionRefAndVelocity(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNode,
    const zVec3 *refVec, const zVec3 *velocityVec) {
    zEffectAnimActivationRecord *result = 0;
    const int boundNodeToken = zClass::NodePtrToValidatedIndex(boundNode);
    const int refNodeToken = zClass::NodePtrToValidatedIndex(refNode);
    const unsigned int flags = self->flags;
    const bool refsValid = (flags & kActivationRecordNoQueueDispatchFlag) == 0 &&
                           self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0) &&
                           (refNode == 0 || refNodeToken > 0);

    int recordQueueRequested = 0;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (refsValid) {
        recordQueueRequested = 1;
    }

    int dispatchRequested = 0;
    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (refsValid) {
        dispatchRequested = 1;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord *const record = zEffect_Anim::AllocActivationRecord();
        record->commandType = 3;
        strncpy(record->animName, self->name, sizeof(record->animName));
        record->nodeToken = boundNodeToken;
        record->params[0].i32 = refNodeToken;
        if (refVec != 0) {
            record->params[1].f32 = refVec->x;
            record->params[2].f32 = refVec->y;
            record->params[3].f32 = refVec->z;
        } else {
            record->params[1].u32 = 0;
            record->params[2].u32 = 0;
            record->params[3].u32 = 0;
        }

        if (velocityVec != 0) {
            record->params[4].f32 = velocityVec->x;
            record->params[5].f32 = velocityVec->y;
            record->params[6].f32 = velocityVec->z;
        } else {
            record->params[4].u32 = 0;
            record->params[5].u32 = 0;
            record->params[6].u32 = 0;
        }

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            zEffect_Anim::DiscardLastActivationRecord();
        }
    }

    return result;
}

// Reimplements 0x461d00: zEffectAnim::QueueCmdType4TransformRefs
// (GameZRecoil/zEffect/zeff_anim_activation.c)
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL QueueCmdType4TransformRefs(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNodeA,
    const zVec3 *refVecA, zClass_NodePartial *refNodeB, const zVec3 *refVecB) {
    zEffectAnimActivationRecord *result = 0;
    const int boundNodeToken = zClass::NodePtrToValidatedIndex(boundNode);
    const int refNodeAToken = zClass::NodePtrToValidatedIndex(refNodeA);
    const int refNodeBToken = zClass::NodePtrToValidatedIndex(refNodeB);
    const unsigned int flags = self->flags;
    const bool refsValid = (flags & kActivationRecordNoQueueDispatchFlag) == 0 &&
                           self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0) &&
                           (refNodeA == 0 || refNodeAToken > 0) &&
                           (refNodeB == 0 || refNodeBToken > 0);

    int recordQueueRequested = 0;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_RecordQueueEnabled != 0 && refsValid) {
        recordQueueRequested = 1;
    }

    int dispatchRequested = 0;
    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_DispatchEnabled != 0 && refsValid) {
        dispatchRequested = 1;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord *const record = zEffect_Anim::AllocActivationRecord();
        record->commandType = 4;
        strncpy(record->animName, self->name, sizeof(record->animName));
        record->nodeToken = boundNodeToken;
        record->params[0].i32 = refNodeAToken;
        if (refVecA != 0) {
            record->params[1].f32 = refVecA->x;
            record->params[2].f32 = refVecA->y;
            record->params[3].f32 = refVecA->z;
        } else {
            record->params[1].u32 = 0;
            record->params[2].u32 = 0;
            record->params[3].u32 = 0;
        }

        record->params[4].i32 = refNodeBToken;
        if (refVecB != 0) {
            record->params[5].f32 = refVecB->x;
            record->params[6].f32 = refVecB->y;
            record->params[7].f32 = refVecB->z;
        } else {
            record->params[5].u32 = 0;
            record->params[6].u32 = 0;
            record->params[7].u32 = 0;
        }

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            zEffect_Anim::DiscardLastActivationRecord();
        }
    }

    return result;
}

// Reimplements 0x45ff10: zEffectAnim::FindEntryByName (zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL FindEntryByName(const char *name) {
    if (name == 0) {
        return 0;
    }

    const int count = g_zEffectAnim_EntryCount;
    for (int i = 0; i < count; ++i) {
        if (strcmp(g_zEffectAnim_EntryList[i].name, name) == 0) {
            return &g_zEffectAnim_EntryList[i];
        }
    }

    return 0;
}

// Reimplements 0x45e650: zEffectAnim::FindNodeRecursiveByName (zeff_anim.c)
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
FindNodeRecursiveByName(zClass_NodePartial *rootNode, const char *name) {
    if (rootNode == 0) {
        return 0;
    }

    if (strcmp(rootNode->name, name) == 0) {
        return rootNode;
    }

    for (int i = 0; i < rootNode->listCountB; ++i) {
        zClass_NodePartial *const childMatch = FindNodeRecursiveByName(rootNode->listB[i], name);
        if (childMatch != 0) {
            return childMatch;
        }
    }

    return 0;
}

// Reimplements 0x45ffa0: zEffectAnim::FindNextAsyncEntry (D:\Proj\GameZRecoil\zEffect\zeff_anim.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL
FindNextAsyncEntry(zEffectAnimEntry *currentEntry) {
    int index = 0;
    if (currentEntry != 0) {
        index = static_cast<int>(currentEntry - g_zEffectAnim_EntryList) + 1;
    }

    for (; index < g_zEffectAnim_EntryCount; ++index) {
        if ((g_zEffectAnim_EntryList[index].flags & 0x10) != 0) {
            return &g_zEffectAnim_EntryList[index];
        }
    }

    return 0;
}

// Reimplements 0x45e280: zEffectAnim::FindSoundRefIndexByName (zeff_anim.c)
RECOIL_NOINLINE int RECOIL_FASTCALL FindSoundRefIndexByName(zEffectAnimEntry *self,
                                                                     const char *name) {
    for (int i = 0; i < self->soundRefCount; ++i) {
        zClass_NodePartial *const node = self->soundRefList[i].runtimeNode;
        if (node != 0 && strcmp(node->name, name) == 0) {
            return i;
        }
    }

    return -1;
}

// Reimplements 0x45e300: zEffectAnim::FindLightRefIndexByName (zeff_anim.c)
RECOIL_NOINLINE int RECOIL_FASTCALL FindLightRefIndexByName(zEffectAnimEntry *self,
                                                                     const char *name) {
    for (int i = 0; i < self->lightRefCount; ++i) {
        zClass_NodePartial *const node = self->lightRefList[i].runtimeNode;
        if (node != 0 && strcmp(node->name, name) == 0) {
            return i;
        }
    }

    return -1;
}

// Reimplements 0x45e380: zEffectAnim::FindOrCreateSoundRef (zeff_anim.c)
RECOIL_NOINLINE int RECOIL_FASTCALL FindOrCreateSoundRef(zEffectAnimEntry *self,
                                                                  const char *name) {
    const int existingIndex = FindSoundRefIndexByName(self, name);
    if (existingIndex > 0) {
        return existingIndex;
    }

    zClass_NodePartial *const node = zClass_Sound::gwSoundNew();
    if (node == 0) {
        return -1;
    }

    zClass_Class::gwNodeSetName(node, name);
    zClass_Sound::SetSampleSetByName(node, name);

    if (self->soundRefList == 0) {
        const int initialCount = static_cast<int>(self->soundRefCount) + 1;
        self->soundRefList = static_cast<zEffectAnimRuntimeNodeRef *>(
            realloc(0, initialCount * sizeof(zEffectAnimRuntimeNodeRef)));
        memset(self->soundRefList, 0, sizeof(zEffectAnimRuntimeNodeRef));
        ++self->soundRefCount;
    }

    if (self->soundRefCount == 0xff) {
        zError::ReportOld(0x400, kZeffAnimInitSourceFile, 0x1b7,
                          "Sound list overflow.\n  Animation: %s\n", self);
        return -1;
    }

    const int resizedCount = static_cast<int>(self->soundRefCount) + 1;
    self->soundRefList = static_cast<zEffectAnimRuntimeNodeRef *>(
        realloc(self->soundRefList, resizedCount * sizeof(zEffectAnimRuntimeNodeRef)));

    zEffectAnimRuntimeNodeRef *const newRef = &self->soundRefList[self->soundRefCount];
    memcpy(newRef->name.text, node->name, sizeof(newRef->name.text));
    newRef->runtimeNode = node;
    newRef->isAttached = 0;
    ++self->soundRefCount;
    return static_cast<int>(self->soundRefCount) - 1;
}

// Reimplements 0x45e4a0: zEffectAnim::FindOrCreateLightRef (zeff_anim.c)
RECOIL_NOINLINE int RECOIL_FASTCALL FindOrCreateLightRef(zEffectAnimEntry *self,
                                                                  const char *name) {
    const int existingIndex = FindLightRefIndexByName(self, name);
    if (existingIndex > 0) {
        return existingIndex;
    }

    zClass_NodePartial *const node = zClass_Light::gwLightNew();
    if (node == 0) {
        return -1;
    }

    zClass_Class::gwNodeSetName(node, name);

    if (self->lightRefList == 0) {
        const int initialCount = static_cast<int>(self->lightRefCount) + 1;
        self->lightRefList = static_cast<zEffectAnimRuntimeNodeRef *>(
            realloc(0, initialCount * sizeof(zEffectAnimRuntimeNodeRef)));
        memset(self->lightRefList, 0, sizeof(zEffectAnimRuntimeNodeRef));
        ++self->lightRefCount;
    }

    if (self->lightRefCount == 0xff) {
        zError::ReportOld(0x400, kZeffAnimInitSourceFile, 0x200,
                          "Light list overflow.\n  Animation: %s\n", self);
        return -1;
    }

    const int resizedCount = static_cast<int>(self->lightRefCount) + 1;
    self->lightRefList = static_cast<zEffectAnimRuntimeNodeRef *>(
        realloc(self->lightRefList, resizedCount * sizeof(zEffectAnimRuntimeNodeRef)));

    zEffectAnimRuntimeNodeRef *const newRef = &self->lightRefList[self->lightRefCount];
    memcpy(newRef->name.text, node->name, sizeof(newRef->name.text));
    newRef->runtimeNode = node;
    newRef->isAttached = 0;
    ++self->lightRefCount;
    return static_cast<int>(self->lightRefCount) - 1;
}

// Reimplements 0x45e5c0: zEffectAnim::ResolveNodeByName (zeff_anim.c)
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL ResolveNodeByName(zEffectAnimEntry *self,
                                                                      const char *name) {
    zClass_NodePartial *resolvedNode = FindNodeRecursiveByName(self->callbackNode, name);
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    resolvedNode = FindNodeRecursiveByName(self->boundNode, name);
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    const int lightRefIndex = FindLightRefIndexByName(self, name);
    if (lightRefIndex > 0) {
        resolvedNode = self->lightRefList[lightRefIndex].runtimeNode;
    }
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    const int soundRefIndex = FindSoundRefIndexByName(self, name);
    if (soundRefIndex > 0) {
        resolvedNode = self->soundRefList[soundRefIndex].runtimeNode;
    }
    if (resolvedNode != 0) {
        return resolvedNode;
    }

    return zClass::FindByTypeAndName(6, name);
}

// Reimplements 0x45ed80: zEffectAnim::RebindEntryToNode
// (D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL RebindEntryToNode(zEffectAnimEntry *self,
                                                                    zClass_NodePartial *node) {
    if (self == 0 || node == 0 || self->activationState == 5) {
        return 0;
    }

    const bool callbackWasBound = self->callbackNode == self->boundNode;
    self->boundNode = node;
    strcpy(self->rootNodeName, node->name);

    if (callbackWasBound) {
        self->callbackNode = node;
        strcpy(self->attachNodeName, node->name);
    } else {
        zClass_NodePartial *const callbackNode =
            FindNodeRecursiveByName(self->boundNode, self->attachNodeName);
        if (callbackNode == 0) {
            ReportAnimationNodeNotFound(self, 0x27f9, self->attachNodeName);
            return 0;
        }
        self->callbackNode = callbackNode;
    }

    for (int i = 0; i < self->trackedNodeCount; ++i) {
        zEffectAnimTrackedNode *const tracked = &self->trackedNodeList[i];
        if (tracked->trackedNode != 0) {
            zClass_NodePartial *const trackedNode =
                ResolveNodeByName(self, tracked->trackedNodeName);
            if (trackedNode == 0) {
                ReportAnimationNodeNotFound(self, 0x2811, tracked->trackedNodeName);
                return 0;
            }
            tracked->trackedNode = trackedNode;
        }
    }

    for (int i_1669 = 0; i_1669 < self->nodeRefCount; ++i_1669) {
        zEffectAnimNodeRef28 *const nodeRef = &self->nodeRefList[i_1669];
        if (nodeRef->node != 0) {
            zClass_NodePartial *const resolvedNode = ResolveNodeByName(self, nodeRef->name.text);
            if (resolvedNode == 0) {
                ReportAnimationNodeNotFound(self, 0x282c, nodeRef->name.text);
                return 0;
            }
            nodeRef->node = resolvedNode;
        }
    }

    zClass_NodePartial *prereqSearchRoot = 0;
    for (int i_1682 = 0; i_1682 < self->activationPrereqCount; ++i_1682) {
        zEffectAnimActivationPrereq *const prereq = &self->activationPrereqList[i_1682];
        if (prereq->mode == 2 || prereq->mode == 3) {
            const char *const nodeName = &prereq->targetName[4];
            if (prereqSearchRoot != 0) {
                prereqSearchRoot = zClass_Class::FindSubNodeByName(prereqSearchRoot, nodeName);
            } else {
                prereqSearchRoot = ResolveNodeByName(self, nodeName);
            }

            if (prereq->mode == 2) {
                prereq->targetNode = prereqSearchRoot;
                prereqSearchRoot = 0;
            }
        }
    }

    return self;
}

// Reimplements 0x45e6d0: zEffectAnim::EnsureCopiedRootTree (zeff_anim.c)
RECOIL_NOINLINE int RECOIL_FASTCALL EnsureCopiedRootTree(zEffectAnimEntry *self,
                                                                  zClass_NodePartial *sourceRoot) {
    if (self == 0) {
        return 0;
    }

    if ((self->flags & kEffectAnimNeedsCopiedRootFlag) != 0) {
        zClass_NodePartial *const copiedRoot =
            zClass_cls_util::CopyNode(sourceRoot, g_zEffectAnim_CopyNodeMode,
                                      g_zEffectAnim_CopyNodeArg1, g_zEffectAnim_CopyNodeArg2);
        self->boundNode = copiedRoot;
        if (copiedRoot == 0) {
            return 0;
        }

        RebindEntryToNode(self, copiedRoot);
        self->flags &= ~kEffectAnimNeedsCopiedRootFlag;
    }

    return 1;
}

// Reimplements 0x45e730: zEffectAnim::CloneEntryForNode
// (D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL CloneEntryForNode(zEffectAnimEntry *self,
                                                                    zClass_NodePartial *node) {
    if (self == 0) {
        return 0;
    }

    zEffectAnimEntry *const clonedEntry =
        static_cast<zEffectAnimEntry *>(calloc(1, sizeof(zEffectAnimEntry)));
    memcpy(clonedEntry, self, sizeof(zEffectAnimEntry));

    clonedEntry->runtimeNode = zClass_Object3D::gwObject3DInit();

    char runtimeNodeName[0x24];
    sprintf(runtimeNodeName, "_%s", self->name);
    zClass_Class::gwNodeSetName(clonedEntry->runtimeNode, runtimeNodeName);
    zClass_Class::gwNodeSetPriority(clonedEntry->runtimeNode, clonedEntry->priority);

    clonedEntry->runtimeSibling = 0;
    clonedEntry->flags &= ~kEffectAnimWorldChildAttachedFlag;

    if (node != 0) {
        clonedEntry->boundNode = node;
        strcpy(clonedEntry->rootNodeName, node->name);
    } else {
        zClass_NodePartial *const copiedRoot =
            zClass_cls_util::CopyNode(self->boundNode, g_zEffectAnim_CopyNodeMode,
                                      g_zEffectAnim_CopyNodeArg1, g_zEffectAnim_CopyNodeArg2);
        clonedEntry->boundNode = copiedRoot;
        if (copiedRoot == 0) {
            zError::ReportOld(0x400, kZeffAnimInitSourceFile, 0x26d1,
                              "ERROR:\n  Copying Animation Node Tree: %s\n", clonedEntry);
            return 0;
        }
    }

    if (self->callbackNode == self->boundNode) {
        clonedEntry->callbackNode = clonedEntry->boundNode;
        strcpy(clonedEntry->attachNodeName, clonedEntry->boundNode->name);
    } else {
        clonedEntry->callbackNode =
            FindNodeRecursiveByName(clonedEntry->boundNode, self->attachNodeName);
    }

    if (clonedEntry->callbackNode == 0) {
        zError::ReportOld(0x400, kZeffAnimInitSourceFile, 0x26e7, kAnimationNodeNotFoundMessage,
                          clonedEntry, self->attachNodeName);
        clonedEntry->activationState = 5;
        return 0;
    }

    if (clonedEntry->lightRefCount > 0) {
        const int count = clonedEntry->lightRefCount;
        clonedEntry->lightRefList = static_cast<zEffectAnimRuntimeNodeRef *>(
            calloc(count, sizeof(zEffectAnimRuntimeNodeRef)));
        memcpy(clonedEntry->lightRefList, self->lightRefList,
                    sizeof(zEffectAnimRuntimeNodeRef) * count);

        for (int i = 0; i < count; ++i) {
            zEffectAnimRuntimeNodeRef *const lightRef = &clonedEntry->lightRefList[i];
            if (self->lightRefList[i].runtimeNode != 0) {
                zClass_NodePartial *const lightNode = zClass_Light::gwLightNew();
                lightRef->runtimeNode = lightNode;
                zClass_Class::gwNodeSetName(lightNode, lightRef->name.text);
            }
            lightRef->isAttached = 0;
        }
    }

    if (clonedEntry->soundRefCount > 0) {
        const int count = clonedEntry->soundRefCount;
        clonedEntry->soundRefList = static_cast<zEffectAnimRuntimeNodeRef *>(
            calloc(count, sizeof(zEffectAnimRuntimeNodeRef)));
        memcpy(clonedEntry->soundRefList, self->soundRefList,
                    sizeof(zEffectAnimRuntimeNodeRef) * count);

        for (int i = 0; i < count; ++i) {
            zEffectAnimRuntimeNodeRef *const soundRef = &clonedEntry->soundRefList[i];
            if (self->soundRefList[i].runtimeNode != 0) {
                zClass_NodePartial *const soundNode = zClass_Sound::gwSoundNew();
                soundRef->runtimeNode = soundNode;
                zClass_Class::gwNodeSetName(soundNode, soundRef->name.text);
                zClass_Sound::SetSampleSetByName(soundNode, soundRef->name.text);
            }
            soundRef->isAttached = 0;
        }
    }

    if (clonedEntry->trackedNodeCount > 0) {
        const int count = clonedEntry->trackedNodeCount;
        clonedEntry->trackedNodeList = static_cast<zEffectAnimTrackedNode *>(
            calloc(count, sizeof(zEffectAnimTrackedNode)));
        memcpy(clonedEntry->trackedNodeList, self->trackedNodeList,
                    sizeof(zEffectAnimTrackedNode) * count);

        for (int i = 0; i < count; ++i) {
            if (self->trackedNodeList[i].trackedNode != 0) {
                clonedEntry->trackedNodeList[i].trackedNode =
                    ResolveNodeByName(clonedEntry, clonedEntry->trackedNodeList[i].trackedNodeName);
            }
        }
    }

    if (clonedEntry->nodeRefCount > 0) {
        const int count = clonedEntry->nodeRefCount;
        clonedEntry->nodeRefList =
            static_cast<zEffectAnimNodeRef28 *>(calloc(count, sizeof(zEffectAnimNodeRef28)));
        memcpy(clonedEntry->nodeRefList, self->nodeRefList,
                    sizeof(zEffectAnimNodeRef28) * count);

        for (int i = 0; i < count; ++i) {
            if (self->nodeRefList[i].node != 0) {
                clonedEntry->nodeRefList[i].node =
                    ResolveNodeByName(clonedEntry, clonedEntry->nodeRefList[i].name.text);
            }
        }
    }

    if (clonedEntry->runtimeSequenceCount > 0) {
        const int count = clonedEntry->runtimeSequenceCount;
        clonedEntry->runtimeList = static_cast<zEffectAnimSurfaceRuntime *>(
            calloc(count, sizeof(zEffectAnimSurfaceRuntime)));

        for (int i = 0; i < count; ++i) {
            memcpy(&clonedEntry->runtimeList[i], &self->runtimeList[i],
                        sizeof(zEffectAnimSurfaceRuntime));

            const int eventStreamSize = self->runtimeList[i].eventStreamSize;
            if (eventStreamSize > 0) {
                clonedEntry->runtimeList[i].eventStream = calloc(1, eventStreamSize);
                memcpy(clonedEntry->runtimeList[i].eventStream,
                            self->runtimeList[i].eventStream, eventStreamSize);
            }
        }
    }

    if (self->surfacePrimary.eventStreamSize > 0) {
        clonedEntry->surfacePrimary.eventStream =
            calloc(1, self->surfacePrimary.eventStreamSize);
        memcpy(clonedEntry->surfacePrimary.eventStream, self->surfacePrimary.eventStream,
                    self->surfacePrimary.eventStreamSize);
    }

    if (clonedEntry->sampleRefCount > 0) {
        const int count = clonedEntry->sampleRefCount;
        clonedEntry->sampleRefList =
            static_cast<zEffectAnimSampleRef *>(calloc(count, sizeof(zEffectAnimSampleRef)));
        memcpy(clonedEntry->sampleRefList, self->sampleRefList,
                    sizeof(zEffectAnimSampleRef) * count);
    }

    if (clonedEntry->effectTemplateRefCount > 0) {
        const int count = clonedEntry->effectTemplateRefCount;
        clonedEntry->effectTemplateRefList = static_cast<zEffectAnimTemplateIndexRef *>(
            calloc(count, sizeof(zEffectAnimTemplateIndexRef)));
        memcpy(clonedEntry->effectTemplateRefList, self->effectTemplateRefList,
                    sizeof(zEffectAnimTemplateIndexRef) * count);
    }

    if (clonedEntry->activationPrereqCount > 0) {
        const int count = clonedEntry->activationPrereqCount;
        clonedEntry->activationPrereqList = static_cast<zEffectAnimActivationPrereq *>(
            calloc(count, sizeof(zEffectAnimActivationPrereq)));
        memcpy(clonedEntry->activationPrereqList, self->activationPrereqList,
                    sizeof(zEffectAnimActivationPrereq) * count);
    }

    if (clonedEntry->runtimeRefCount > 0) {
        const int count = clonedEntry->runtimeRefCount;
        clonedEntry->runtimeRefList =
            static_cast<zEffectAnimRuntimeRef *>(calloc(count, sizeof(zEffectAnimRuntimeRef)));
        memcpy(clonedEntry->runtimeRefList, self->runtimeRefList,
                    sizeof(zEffectAnimRuntimeRef) * count);

        for (int i = 0; i < count; ++i) {
            clonedEntry->runtimeRefList[i].cachedChildEntry = 0;
        }
    }

    return clonedEntry;
}

// Reimplements 0x45fd10: zEffectAnim::ShutdownEntry
RECOIL_NOINLINE int RECOIL_FASTCALL ShutdownEntry(zEffectAnimEntry *self) {
    if (self->activationState == 5) {
        return 0;
    }

    if (self->runtimeNode != 0) {
        zClass_Object3D::DeleteNode(self->runtimeNode);
    }

    const unsigned char activationMode = self->activationMode;
    if (activationMode == 1 || activationMode == 0 || activationMode == 2) {
        zClass_Node::ClearDamageHandler(self->callbackNode);
    }

    FreeIfSet(self->surfacePrimary.eventStream);
    self->surfacePrimary.eventStream = 0;
    self->surfacePrimary.eventStreamSize = 0;

    for (int i = 0; i < self->runtimeSequenceCount; ++i) {
        FreeIfSet(self->runtimeList[i].eventStream);
    }

    FreeIfSet(self->runtimeList);
    FreeIfSet(self->trackedNodeList);
    FreeIfSet(self->nodeRefList);
    FreeIfSet(self->lightRefList);
    FreeIfSet(self->soundRefList);
    FreeIfSet(self->sampleRefList);
    FreeIfSet(self->effectTemplateRefList);
    FreeIfSet(self->activationPrereqList);
    FreeIfSet(self->runtimeRefList);

    if (self->runtimeSibling != 0) {
        ShutdownEntry(self->runtimeSibling);
        free(self->runtimeSibling);
    }

    return 0;
}

// Reimplements 0x460010: zEffectAnim::GetRootNodeOrNull (D:\Proj\GameZRecoil\zEffect\zeff_anim.c)
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL GetRootNodeOrNull(zEffectAnimEntry *self) {
    if (self == 0) {
        return 0;
    }

    return self->boundNode;
}
} // namespace zEffectAnim

namespace zEffect_Anim {
// Reimplements 0x45e210: zEffect_Anim::SetZbdFilename
// (D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c)
RECOIL_NOINLINE void RECOIL_FASTCALL SetZbdFilename(const char *filename) {
    if (strlen(filename) > 0x80) {
        zError::ReportOld(0x400, kZeffAnimInitSourceFile, 0xd1,
                          "Animation ZBD filename too long: %s\n", filename);
        return;
    }

    strcpy(g_zEffectAnim_ZbdFilename, filename);
}

// Reimplements 0x45efb0: zEffect_Anim::LoadZbd
// (D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c)
RECOIL_NOINLINE int RECOIL_CDECL LoadZbd() {
    if (g_zEffect_ResourceNode == 0 || g_zEffectAnim_ZbdFilename[0] == '\0') {
        return -1;
    }

    FILE *stream = fopen(g_zEffectAnim_ZbdFilename, "rb");
    if (stream == 0) {
        return -1;
    }

    int zbdSignature = 0;
    int sourceFileStampRecordSize = 0;
    int sourceFileStampCount = 0;
    if (!ReadOne(stream, &zbdSignature, sizeof(zbdSignature)) ||
        !ReadOne(stream, &sourceFileStampRecordSize, sizeof(sourceFileStampRecordSize)) ||
        !ReadOne(stream, &sourceFileStampCount, sizeof(sourceFileStampCount)) ||
        zbdSignature != 0x08170616 ||
        sourceFileStampRecordSize != sizeof(zEffectAnimSourceFileStamp)) {
        fclose(stream);
        return -1;
    }

    g_zEffectAnim_SourceFileStampCount = sourceFileStampCount;
    if (!ReadArray(stream, &g_zEffectAnim_SourceFileStampList,
                   static_cast<unsigned int>(sourceFileStampCount))) {
        fclose(stream);
        return -1;
    }

    bool stampMismatch = false;
    for (int i = 0; i < g_zEffectAnim_SourceFileStampCount; ++i) {
        struct _stat statBuffer;
        zEffectAnimSourceFileStamp *const stamp = &g_zEffectAnim_SourceFileStampList[i];
        if (_stat(stamp->sourcePath, &statBuffer) == 0 &&
            stamp->fileMtime != static_cast<int>(statBuffer.st_mtime)) {
            stampMismatch = true;
            break;
        }
    }

    if (g_zEffectAnim_SourceFileStampList != 0) {
        free(g_zEffectAnim_SourceFileStampList);
        g_zEffectAnim_SourceFileStampList = 0;
    }
    g_zEffectAnim_SourceFileStampCount = 0;
    if (stampMismatch) {
        fclose(stream);
        return -1;
    }

    zClass_NodePartial *const savedWorld = g_zEffect_World;
    zEffectAnimZbdHeaderBlock headerBlock = {0};
    if (!ReadOne(stream, &headerBlock, sizeof(headerBlock))) {
        fclose(stream);
        return -1;
    }

    g_zEffectAnim_EntriesInstantiated = headerBlock.entriesInstantiated;
    g_zEffectAnim_HeapPtr = headerBlock.heapPtr;
    g_zEffectAnim_CountsPackedLoWord = headerBlock.countsPackedLoWord;
    g_zEffectAnim_EntryCount = headerBlock.entryCount;
    g_zEffectAnim_EntryList = 0;
    g_zEffectAnim_TextIdEntryCount = headerBlock.textIdEntryCount;
    g_zEffectAnim_TextIdEntryList = 0;
    g_zEffect_World = savedWorld;
    g_zEffect_DefaultGravity = headerBlock.defaultGravity;
    g_zEffect_ConditionalRefPosEnabled = headerBlock.conditionalRefPosEnabled;
    g_zEffect_VariantOverrideEnabled = headerBlock.variantOverrideEnabled;
    g_zEffect_ConditionalRefPosX = headerBlock.conditionalRefPosX;
    g_zEffect_ConditionalRefPosY = headerBlock.conditionalRefPosY;
    g_zEffect_ConditionalRefPosZ = headerBlock.conditionalRefPosZ;
    g_zEffect_VariantOverridePackedIds = headerBlock.variantOverridePackedIds;
    g_zEffect_FrameDeltaRemainingSec = headerBlock.frameDeltaRemainingSec;

    const int entryCount = g_zEffectAnim_EntryCount;
    g_zEffectAnim_EntryList =
        static_cast<zEffectAnimEntry *>(malloc(sizeof(zEffectAnimEntry) * entryCount));
    if (entryCount > 0 && g_zEffectAnim_EntryList == 0) {
        fclose(stream);
        return -1;
    }
    if (entryCount > 0) {
        memset(g_zEffectAnim_EntryList, 0, sizeof(zEffectAnimEntry) * entryCount);
    }

    for (int i_2059 = 0; i_2059 < entryCount; ++i_2059) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i_2059];
        if (!ReadOne(stream, entry, sizeof(zEffectAnimEntry))) {
            fclose(stream);
            return -1;
        }

        entry->trackedNodeList = 0;
        entry->nodeRefList = 0;
        entry->lightRefList = 0;
        entry->soundRefList = 0;
        entry->sampleRefList = 0;
        entry->effectTemplateRefList = 0;
        entry->activationPrereqList = 0;
        entry->runtimeRefList = 0;
        entry->runtimeList = 0;
        entry->surfacePrimary.eventStream = 0;

        if (!ReadArray(stream, &entry->trackedNodeList, entry->trackedNodeCount) ||
            !ReadArray(stream, &entry->nodeRefList, entry->nodeRefCount) ||
            !ReadArray(stream, &entry->lightRefList, entry->lightRefCount) ||
            !ReadArray(stream, &entry->soundRefList, entry->soundRefCount) ||
            !ReadArray(stream, &entry->sampleRefList, entry->sampleRefCount) ||
            !ReadArray(stream, &entry->effectTemplateRefList, entry->effectTemplateRefCount) ||
            !ReadArray(stream, &entry->activationPrereqList, entry->activationPrereqCount) ||
            !ReadArray(stream, &entry->runtimeRefList, entry->runtimeRefCount)) {
            fclose(stream);
            return -1;
        }

        for (int j = 0; j < entry->runtimeRefCount; ++j) {
            entry->runtimeRefList[j].cachedChildEntry = 0;
        }

        if (entry->runtimeSequenceCount > 0) {
            entry->runtimeList = static_cast<zEffectAnimSurfaceRuntime *>(
                malloc(sizeof(zEffectAnimSurfaceRuntime) * entry->runtimeSequenceCount));
            if (entry->runtimeList == 0) {
                fclose(stream);
                return -1;
            }
        }

        if (!ReadOne(stream, &entry->surfacePrimary, sizeof(entry->surfacePrimary)) ||
            !ReadEventStream(stream, &entry->surfacePrimary)) {
            fclose(stream);
            return -1;
        }

        for (int j_2108 = 0; j_2108 < entry->runtimeSequenceCount; ++j_2108) {
            zEffectAnimSurfaceRuntime *const runtime = &entry->runtimeList[j_2108];
            if (!ReadOne(stream, runtime, sizeof(zEffectAnimSurfaceRuntime)) ||
                !ReadEventStream(stream, runtime)) {
                fclose(stream);
                return -1;
            }
        }
    }

    const char *previousRootNodeName = 0;
    for (int i_2119 = 0; i_2119 < entryCount; ++i_2119) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i_2119];
        const unsigned char savedLightRefCount = entry->lightRefCount;
        const unsigned char savedSoundRefCount = entry->soundRefCount;
        entry->lightRefCount = 0;
        entry->soundRefCount = 0;

        if (i_2119 != 0 && entry->activationState != 5) {
            entry->boundNode = 0;
            if (previousRootNodeName != 0 &&
                strcmp(previousRootNodeName, entry->rootNodeName) == 0) {
                entry->boundNode = zClass_Class::gwNodeFindNextByName(0, 0);
            }

            if (entry->boundNode == 0) {
                zClass_Class::gwNodeFindNextByName(entry->rootNodeName, 6);
                entry->boundNode = zClass_Class::gwNodeFindNextByName(0, 0);
                previousRootNodeName = entry->rootNodeName;
            }

            if (entry->boundNode == 0) {
                fclose(stream);
                return -1;
            }

            zClass_NodePartial *const rootNode = zClass_Class::gwNodeGetRoot(entry->boundNode);
            if (g_zEffectAnim_ForceCloneNonDynamicRoot != 0 && rootNode != 0 &&
                rootNode->classId != 2 && rootNode->classId != 1) {
                entry->boundNode = zClass_cls_util::CopyNode(
                    entry->boundNode, g_zEffectAnim_CopyNodeMode, g_zEffectAnim_CopyNodeArg1,
                    g_zEffectAnim_CopyNodeArg2);
                if (entry->boundNode == 0) {
                    fclose(stream);
                    return -1;
                }
            }

            entry->callbackNode = zEffectAnim::ResolveNodeByName(entry, entry->attachNodeName);
            if (entry->callbackNode == 0) {
                fclose(stream);
                return -1;
            }
        }

        entry->lightRefCount = savedLightRefCount;
        for (int j = 1; j < entry->lightRefCount; ++j) {
            zEffectAnimRuntimeNodeRef *const lightRef = &entry->lightRefList[j];
            lightRef->runtimeNode = zClass_Light::gwLightNew();
            if (lightRef->runtimeNode == 0) {
                fclose(stream);
                return -1;
            }
            zClass_Class::gwNodeSetName(lightRef->runtimeNode, lightRef->name.text);
        }

        entry->soundRefCount = savedSoundRefCount;
        for (int j_2175 = 1; j_2175 < entry->soundRefCount; ++j_2175) {
            zEffectAnimRuntimeNodeRef *const soundRef = &entry->soundRefList[j_2175];
            soundRef->runtimeNode = zClass_Sound::gwSoundNew();
            if (soundRef->runtimeNode == 0) {
                fclose(stream);
                return -1;
            }
            zClass_Class::gwNodeSetName(soundRef->runtimeNode, soundRef->name.text);
            zClass_Sound::SetSampleSetByName(soundRef->runtimeNode, soundRef->name.text);
        }

        for (int j_2186 = 1; j_2186 < entry->trackedNodeCount; ++j_2186) {
            zEffectAnimTrackedNode *const tracked = &entry->trackedNodeList[j_2186];
            tracked->trackedNode = zEffectAnim::ResolveNodeByName(entry, tracked->trackedNodeName);
            if (tracked->trackedNode == 0) {
                fclose(stream);
                return -1;
            }
        }

        for (int j_2195 = 1; j_2195 < entry->nodeRefCount; ++j_2195) {
            zEffectAnimNodeRef28 *const nodeRef = &entry->nodeRefList[j_2195];
            nodeRef->node = zEffectAnim::ResolveNodeByName(entry, nodeRef->name.text);
            if (nodeRef->node == 0) {
                fclose(stream);
                return -1;
            }
        }

        for (int j_2204 = 1; j_2204 < entry->sampleRefCount; ++j_2204) {
            entry->sampleRefList[j_2204].sample = zSnd::FindSampleByName(entry->sampleRefList[j_2204].name);
        }

        for (int j_2208 = 1; j_2208 < entry->effectTemplateRefCount; ++j_2208) {
            zEffectAnimTemplateIndexRef *const templateRef = &entry->effectTemplateRefList[j_2208];
            templateRef->templateIndex = zEffect::FindTemplateIndexByName(templateRef->name);
            if (templateRef->templateIndex == -1) {
                fclose(stream);
                return -1;
            }
        }

        zClass_NodePartial *prereqSearchRoot = 0;
        zClass_NodePartial *cachedPrereqSearchRoot = 0;
        for (int j_2219 = 0; j_2219 < entry->activationPrereqCount; ++j_2219) {
            zEffectAnimActivationPrereq *const prereq = &entry->activationPrereqList[j_2219];
            if (prereq->mode == 1) {
                prereq->targetEntry = zEffectAnim::FindEntryByName(prereq->targetName);
                prereqSearchRoot = cachedPrereqSearchRoot;
            } else if (prereq->mode == 2 || prereq->mode == 3) {
                const char *const nodeName = &prereq->targetName[4];
                if (prereqSearchRoot != 0) {
                    prereqSearchRoot = zClass_Class::FindSubNodeByName(prereqSearchRoot, nodeName);
                } else {
                    prereqSearchRoot = zClass::FindByTypeAndName(6, nodeName);
                }

                cachedPrereqSearchRoot = prereqSearchRoot;
                if (prereqSearchRoot == 0) {
                    zError::ReportOld(0x400, kZeffAnimInitSourceFile, 0x2c17,
                                      "ACTIVATION_PREREQUISITE error; couldn't find node.\n"
                                      "  Animation: %s; node: %s\n",
                                      entry, nodeName);
                    entry->activationPrereqCount = 0;
                    break;
                }

                prereq->targetNode = prereqSearchRoot;
                if (prereq->mode == 2) {
                    prereqSearchRoot = 0;
                    cachedPrereqSearchRoot = 0;
                }
            }
        }
    }

    if (g_zEffectAnim_TextIdEntryCount > 0) {
        if (!ReadArray(stream, &g_zEffectAnim_TextIdEntryList,
                       static_cast<unsigned int>(g_zEffectAnim_TextIdEntryCount))) {
            fclose(stream);
            return -1;
        }

        for (int i = 0; i < g_zEffectAnim_TextIdEntryCount; ++i) {
            g_zEffectAnim_TextIdEntryList[i].messageId =
                zLoc::GetMessageId(g_zEffectAnim_TextIdEntryList[i].messageKey);
        }
    }

    fclose(stream);
    return 0;
}

// Reimplements 0x45fb30: zEffect_Anim::LoadAndInstantiate
// (D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c)
RECOIL_NOINLINE int RECOIL_CDECL LoadAndInstantiate() {
    if (g_zEffectAnim_EntriesInstantiated != 0) {
        return 0;
    }

    LoadZbd();

    for (int i = 1; i < g_zEffectAnim_EntryCount; ++i) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i];
        if (entry->activationState != 5 && entry->activationState != 4 &&
            zClass::AnyNodeMatchesPredicateRecursive(entry->boundNode,
                                                     zClass_Node::HasRenderableDiPredicate) != 0) {
            entry->flags |= 0x200;
        }
    }

    for (int i_2286 = 1; i_2286 < g_zEffectAnim_EntryCount; ++i_2286) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i_2286];
        if (entry->activationState == 5) {
            continue;
        }

        if (entry->boundNode == 0) {
            zClass_NodePartial *const objectNode = zClass_Object3D::gwObject3DInit();
            entry->boundNode = objectNode;
            entry->callbackNode = objectNode;
        }

        if ((entry->flags & kEffectAnimNeedsCopiedRootFlag) != 0) {
            zEffectAnim::EnsureCopiedRootTree(entry, entry->boundNode);
        }

        entry->runtimeNode = zClass_Object3D::gwObject3DInit();
        char runtimeNodeName[0x24];
        sprintf(runtimeNodeName, "_%s", entry->name);
        zClass_Class::gwNodeSetName(entry->runtimeNode, runtimeNodeName);
        zClass_Class::gwNodeSetPriority(entry->runtimeNode, entry->priority);

        CaptureNodeStates(entry);
        if (entry->activationMode == 1) {
            zClass_Node::SetDamageTimerCallback(
                entry, entry->callbackNode,
                (void *)(zEffect::TickResetDelayOnTimer));
        }
        if (entry->activationMode == 0) {
            zClass_Node::SetDamageHitCallback(
                entry, entry->callbackNode, (void *)(zEffect::TickResetDelayOnHit));
        }
        if (entry->activationMode == 2) {
            zClass_Node::SetDamageTimerCallback(
                entry, entry->callbackNode,
                (void *)(zEffect::TickResetDelayOnTimer));
            zClass_Node::SetDamageHitCallback(
                entry, entry->callbackNode, (void *)(zEffect::TickResetDelayOnHit));
        }

        if (entry->activationState != 2) {
            if ((entry->flags & 0x20) != 0) {
                zEffectAnim::StopAndCleanup(entry, 0, 0);
            }
            if (entry->activationMode == 4) {
                zEffectAnim::SetVelocity(entry, 0, 0.0f, 0.0f, 0.0f);
            }
        } else {
            zError::ReportOld(0x400, kZeffAnimInitSourceFile, 0x2d55,
                              "Corrupt animation loaded:\n  Animation: %s\n", entry);
        }
    }

    g_zEffectAnim_EntriesInstantiated = 1;
    return 0;
}

// Reimplements 0x4603d0: zEffect_Anim::ClearActivationRecords
RECOIL_NOINLINE void RECOIL_CDECL ClearActivationRecords() {
    if (g_zEffectAnim_ActivationRecordTable != 0) {
        free(g_zEffectAnim_ActivationRecordTable);
        g_zEffectAnim_ActivationRecordTable = 0;
    }

    g_zEffectAnim_ActivationRecordCount = 0;
}

// Reimplements 0x460400: zEffect_Anim::HasActivationRecord
RECOIL_NOINLINE int RECOIL_FASTCALL
HasActivationRecord(zEffectAnimActivationRecord *record) {
    for (int i = 0; i < g_zEffectAnim_ActivationRecordCount; ++i) {
        zEffectAnimActivationRecord *const queuedRecord = &g_zEffectAnim_ActivationRecordTable[i];
        if (queuedRecord->nodeToken == record->nodeToken &&
            strncmp(queuedRecord->animName, record->animName, sizeof(record->animName)) == 0) {
            return 1;
        }
    }

    return 0;
}

// Reimplements 0x460470: zEffect_Anim::GetActivationRecordCount
RECOIL_NOINLINE int RECOIL_CDECL GetActivationRecordCount() {
    return g_zEffectAnim_ActivationRecordCount;
}

// Reimplements 0x460480: zEffect_Anim::GetActivationRecordAt
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL
GetActivationRecordAt(int index) {
    return &g_zEffectAnim_ActivationRecordTable[index];
}

// Reimplements 0x460490: zEffect_Anim::SaveActivationRecords
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
SaveActivationRecords(zZbdSectionCallbackCtx *callbackCtx) {
    int result = 1;
    for (int i = 0; result != 0 && i < g_zEffectAnim_ActivationRecordCount; ++i) {
        zEffectAnimActivationRecord *const sourceRecord = &g_zEffectAnim_ActivationRecordTable[i];
        zEffectAnimEntry *const entry = zEffectAnim::FindEntryByName(sourceRecord->animName);
        unsigned char trackedNodeCount = 0;
        if (entry != 0 && entry->trackedNodeList != 0) {
            trackedNodeCount = entry->trackedNodeCount;
        }

        const size_t recordSize = offsetof(zEffectAnimActivationSaveRecord, trackedNodes) +
                                       sizeof(zEffectAnimTrackedNodeSaveRecord) * trackedNodeCount;
        zEffectAnimActivationSaveRecord *const saveRecord =
            static_cast<zEffectAnimActivationSaveRecord *>(calloc(1, recordSize));
        if (saveRecord == 0) {
            return 0;
        }

        memcpy(&saveRecord->base, sourceRecord, sizeof(*sourceRecord));
        if (entry != 0) {
            zClass_NodePartial *const rootNode = GameZ_ZBD::NodeIndexToPtr(sourceRecord->nodeToken);
            saveRecord->savedActivationState = entry->activationState;
            zEffectAnim::RebindEntryToNode(entry, rootNode);
            saveRecord->trackedNodeCount = trackedNodeCount;

            for (int j = 0; j < trackedNodeCount; ++j) {
                zEffectAnimTrackedNode *const tracked = &entry->trackedNodeList[j];
                zEffectAnimTrackedNodeSaveRecord *const savedTracked = &saveRecord->trackedNodes[j];
                zClass_NodePartial *const node = tracked->trackedNode;
                if (node == 0 || node->classId != 5) {
                    savedTracked->nodeIndex = -1;
                    continue;
                }

                savedTracked->nodeIndex = zClass::NodePtrToValidatedIndex(node);
                savedTracked->activeFlag = (node->flags >> 2) & 1;

                zClass_Object3DDataPartial *const objectData =
                    static_cast<zClass_Object3DDataPartial *>(node->classData);
                savedTracked->usesCachedMatrix = (objectData->flags >> 4) & 1;
                if (savedTracked->usesCachedMatrix != 0) {
                    memcpy(savedTracked->transform,
                                zClass_Object3D::gwObject3DGetMatrixPtr(node),
                                sizeof(savedTracked->transform));
                } else {
                    zClass_Object3D::gwObject3DGetPosition(node, &savedTracked->transform[0],
                                                           &savedTracked->transform[1],
                                                           &savedTracked->transform[2]);
                    zClass_Object3D::gwObject3DGetRotation(node, &savedTracked->transform[3],
                                                           &savedTracked->transform[4],
                                                           &savedTracked->transform[5]);
                    zClass_Object3D::gwObject3DGetScale(node, &savedTracked->transform[6],
                                                        &savedTracked->transform[7],
                                                        &savedTracked->transform[8]);
                }

                if (node->userDataOrDiRef != 0) {
                    unsigned int *const di =
                        (unsigned int *)(node->userDataOrDiRef);
                    savedTracked->diFlagBits = (savedTracked->diFlagBits & ~1) | ((di[1] >> 3) & 1);
                    savedTracked->diUserValue = static_cast<int>(di[8]);
                } else {
                    savedTracked->diFlagBits &= ~1;
                }
            }
        } else {
            saveRecord->savedActivationState = 0;
            saveRecord->trackedNodeCount = 0;
        }

        char sectionName[0x14];
        sprintf(sectionName, "Activation%04d", i);
        result = zUtil_ZAR::WriteSectionBlob(callbackCtx, sectionName, saveRecord,
                                             static_cast<unsigned int>(recordSize));
        free(saveRecord);
    }

    return result;
}

static void QueueLoadedActivationRecord(const zEffectAnimActivationRecord *record) {
    memcpy(AllocActivationRecord(), record, sizeof(zEffectAnimActivationRecord));
}

// Reimplements 0x4606d0: zEffect_Anim::LoadActivationRecords
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE void RECOIL_FASTCALL LoadActivationRecords(void *, const char *sectionToken,
                                                           void *data, int, void *) {
    zEffectAnimActivationSaveRecord *const record =
        static_cast<zEffectAnimActivationSaveRecord *>(data);
    zEffectAnimEntry *entry = zEffectAnim::FindEntryByName(record->base.animName);

    if (strcmp(sectionToken, "Activation0000") == 0) {
        for (int i = 0; i < GetActivationRecordCount(); ++i) {
            zEffectAnimActivationRecord *const queued = GetActivationRecordAt(i);
            ResetFromActivationRecord(queued);
            zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x190,
                              "zEffResetActivationRecord: %s", queued->animName);
        }

        ClearActivationRecords();
        for (int i_2482 = 1; i_2482 < g_zEffectAnim_EntryCount; ++i_2482) {
            zEffectAnimEntry *cursor = &g_zEffectAnim_EntryList[i_2482];
            while (cursor != 0) {
                cursor->flags &= ~0x4000u;
                cursor = cursor->runtimeSibling;
            }
        }
    }

    if (record->base.nodeToken >= 0 && entry != 0) {
        zClass_NodePartial *const rootNode = GameZ_ZBD::NodeIndexToPtr(record->base.nodeToken);
        zEffectAnimEntry *sibling = entry->runtimeSibling;
        while (sibling != 0 && entry->boundNode != rootNode) {
            entry = sibling;
            sibling = entry->runtimeSibling;
        }

        NodeActionCallback(entry, rootNode);
        entry = ProcessActivationRecord(&record->base);
    }

    if (entry == 0) {
        return;
    }

    entry->flags |= 0x4000u;
    const unsigned char savedState = record->savedActivationState;
    if (savedState == 2) {
        if (entry->activationState != 2) {
            NodeActionCallback(entry, GameZ_ZBD::NodeIndexToPtr(record->base.nodeToken));
            entry = ProcessActivationRecord(&record->base);
            zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x1b6,
                              "zEffProcessActivationRecord");
        }
        if (record->base.nodeToken == -1) {
            QueueLoadedActivationRecord(&record->base);
        }
    }

    if (savedState == 3) {
        if (entry->activationState != 3) {
            zEffectAnim::Stop(entry);
            entry->activationState = 3;
        }
        if (record->base.nodeToken == -1) {
            QueueLoadedActivationRecord(&record->base);
        }
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x1c9,
                          "Set anim_state = ANIM_STATE_EXECUTED");
    }

    if (savedState == 1) {
        if (entry->activationState != 1) {
            if (entry->activationState == 4) {
                entry->activationState = 3;
            }
            NodeActionCallback(entry, GameZ_ZBD::NodeIndexToPtr(record->base.nodeToken));
            zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x1d3,
                              "zEffAnimReset");
        }
        if (record->base.nodeToken == -1) {
            QueueLoadedActivationRecord(&record->base);
        }
    }

    if (savedState == 6) {
        if (entry->activationState != 6) {
            if (entry->activationState == 4) {
                entry->activationState = 3;
            }
            NodeActionCallback(entry, GameZ_ZBD::NodeIndexToPtr(record->base.nodeToken));
            entry = ProcessActivationRecord(&record->base);
            zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x1e4,
                              "zEffProcessActivationRecord");
        }
        if (record->base.nodeToken == -1) {
            QueueLoadedActivationRecord(&record->base);
        }
    }

    if (savedState == 4) {
        if (entry->activationState != 4) {
            zEffectAnim::Stop(entry);
            entry->activationState = 4;
            zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x1f5,
                              "Set anim_state = ANIM_STATE_INVALID");
        }
        if (record->base.nodeToken == -1) {
            QueueLoadedActivationRecord(&record->base);
        }
    }

    for (int i = 0; i < record->trackedNodeCount; ++i) {
        zEffectAnimTrackedNodeSaveRecord *const tracked = &record->trackedNodes[i];
        zClass_NodePartial *const node = GameZ_ZBD::NodeIndexToPtr(tracked->nodeIndex);
        if (node == 0 || node->classId != 5) {
            continue;
        }

        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x201,
                          "Restore node: %s %d", node, tracked->activeFlag);
        zClass_Class::gwNodeSetActive(node, tracked->activeFlag != 0 ? 1 : 0);
        if (tracked->activeFlag != 0) {
            if (tracked->usesCachedMatrix != 0) {
                zClass_Object3D::gwObject3DSetMatrix(node, tracked->transform);
            } else {
                zClass_Object3D::gwObject3DSetPosition(
                    node, tracked->transform[0], tracked->transform[1], tracked->transform[2]);
                zClass_Object3D::gwObject3DSetRotation(
                    node, tracked->transform[3], tracked->transform[4], tracked->transform[5]);
                zClass_Object3D::gwObject3DSetScale(node, tracked->transform[6],
                                                    tracked->transform[7], tracked->transform[8]);
            }
        }

        if (node->userDataOrDiRef != 0 && (tracked->diFlagBits & 1) != 0) {
            unsigned int *const di = (unsigned int *)(node->userDataOrDiRef);
            di[1] = (di[1] & ~0x08u) | 0x08u;
            di[8] = static_cast<unsigned int>(tracked->diUserValue);
        }
    }
}

// Reimplements 0x460bc0: zEffect_Anim::SaveRunningAnimRecord
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
SaveRunningAnimRecord(zZbdSectionCallbackCtx *callbackCtx, zEffectAnimEntry *entry,
                      int runningIndex, int includePrimaryEntry) {
    size_t totalSize = sizeof(zEffectAnimRunningSaveHeader);
    for (int i = 0; i < entry->runtimeSequenceCount; ++i) {
        totalSize += sizeof(zEffectAnimSurfaceRuntime);
        if (entry->runtimeList[i].eventStreamSize > 0) {
            totalSize += entry->runtimeList[i].eventStreamSize;
        }
    }
    totalSize += sizeof(zEffectAnimRuntimeNodeSaveRecord) * entry->lightRefCount;
    totalSize += sizeof(zEffectAnimSoundNodeSaveRecord) * entry->soundRefCount;

    unsigned char *const buffer = static_cast<unsigned char *>(calloc(1, totalSize));
    if (buffer == 0) {
        return 0;
    }

    zEffectAnimRunningSaveHeader *const header =
        (zEffectAnimRunningSaveHeader *)(buffer);
    header->entryTableIndex = runningIndex;
    header->matchSavedRootNode = includePrimaryEntry;
    strncpy(header->entryName, entry->name, sizeof(header->entryName));
    header->rootNodeIndex = zClass::NodePtrToValidatedIndex(entry->boundNode);
    header->nodeRefAIndex = zClass::NodePtrToValidatedIndex(LoadResetScratchNode(entry, 0));
    header->refVecA = LoadResetScratchVec(entry, 1);
    header->nodeRefBIndex = zClass::NodePtrToValidatedIndex(LoadResetScratchNode(entry, 4));
    header->refVecB = LoadResetScratchVec(entry, 5);
    header->activationState = entry->activationState;
    header->triggerCurrentValue = entry->triggerCurrentValue;
    header->activationCountdown = entry->activationCountdown;
    header->velocityX = entry->velocityX;
    header->velocityY = entry->velocityY;
    header->velocityZ = entry->velocityZ;
    header->runtimeSurfaceCount = entry->runtimeSequenceCount;
    header->lightRefCount = entry->lightRefCount;
    header->soundRefCount = entry->soundRefCount;

    unsigned char *cursor = buffer + sizeof(*header);
    for (int i_2646 = 0; i_2646 < entry->runtimeSequenceCount; ++i_2646) {
        zEffectAnimSurfaceRuntime runtimeCopy = entry->runtimeList[i_2646];
        runtimeCopy.currentEvent =
            (void *)(static_cast<unsigned char *>(runtimeCopy.currentEvent) -
                                     static_cast<unsigned char *>(runtimeCopy.eventStream));
        memcpy(cursor, &runtimeCopy, sizeof(runtimeCopy));
        cursor += sizeof(runtimeCopy);
        if (entry->runtimeList[i_2646].eventStreamSize > 0) {
            memcpy(cursor, entry->runtimeList[i_2646].eventStream,
                        entry->runtimeList[i_2646].eventStreamSize);
            cursor += entry->runtimeList[i_2646].eventStreamSize;
        }
    }

    for (int i_2660 = 0; i_2660 < entry->lightRefCount; ++i_2660) {
        zEffectAnimRuntimeNodeSaveRecord record = {0};
        zEffectAnimRuntimeNodeRef *const lightRef = &entry->lightRefList[i_2660];
        strncpy(record.name, lightRef->name.text, sizeof(record.name));
        record.isAttached = lightRef->isAttached;
        zClass_NodePartial *const node = lightRef->runtimeNode;
        if (node != 0) {
            zClass_LightDataPartial *const lightData =
                static_cast<zClass_LightDataPartial *>(node->classData);
            record.posX = lightData->localPosition.x;
            record.posY = lightData->localPosition.y;
            record.posZ = lightData->localPosition.z;
            record.parentNodeIndex =
                node->listCountA > 0 ? zClass::NodePtrToValidatedIndex(node->listA[0]) : -1;
        }
        memcpy(cursor, &record, sizeof(record));
        cursor += sizeof(record);
    }

    for (int i_2679 = 0; i_2679 < entry->soundRefCount; ++i_2679) {
        zEffectAnimSoundNodeSaveRecord record = {0};
        zEffectAnimRuntimeNodeRef *const soundRef = &entry->soundRefList[i_2679];
        strncpy(record.name, soundRef->name.text, sizeof(record.name));
        record.isAttached = soundRef->isAttached;
        zClass_NodePartial *const node = soundRef->runtimeNode;
        if (node != 0) {
            zClass_SoundDataPartial *const soundData =
                static_cast<zClass_SoundDataPartial *>(node->classData);
            record.hasPosition = (soundData->runtimeFlags >> 1) & 1;
            record.posX = soundData->localPosition.x;
            record.posY = soundData->localPosition.y;
            record.posZ = soundData->localPosition.z;
            record.parentNodeIndex =
                node->listCountA > 0 ? zClass::NodePtrToValidatedIndex(node->listA[0]) : -1;
        }
        memcpy(cursor, &record, sizeof(record));
        cursor += sizeof(record);
    }

    char sectionName[0x14];
    sprintf(sectionName, "Running%03d", runningIndex);
    const int result = zUtil_ZAR::WriteSectionBlob(callbackCtx, sectionName, buffer,
                                                            static_cast<unsigned int>(totalSize));
    free(buffer);
    return result;
}

// Reimplements 0x460f80: zEffect_Anim::SaveRunningAnimRecords
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
SaveRunningAnimRecords(zZbdSectionCallbackCtx *callbackCtx) {
    int result = 1;
    for (int i = 1; result != 0 && i < g_zEffectAnim_EntryCount; ++i) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i];
        if (entry != 0 && (entry->activationState == 2 || entry->activationState == 6)) {
            const unsigned short flags = static_cast<unsigned short>(entry->flags);
            if (((flags & 0x1000) == 0 || (flags & 0x2000) != 0) &&
                g_zEffectAnim_RecordQueueEnabled != 0) {
                result = SaveRunningAnimRecord(callbackCtx, entry, i, 1);
            }
        }

        zEffectAnimEntry *sibling = entry->runtimeSibling;
        while (result != 0 && sibling != 0) {
            if (sibling->activationState == 2) {
                result = SaveRunningAnimRecord(callbackCtx, sibling, i, 0);
            }
            sibling = sibling->runtimeSibling;
        }
    }

    return result;
}

// Reimplements 0x461040: zEffect_Anim::LoadRunningAnimRecords
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE void RECOIL_FASTCALL LoadRunningAnimRecords(void *unused, const char *sectionToken,
                                                            void *data, int dataSize,
                                                            void *extraCtx) {
    (void)unused;
    (void)sectionToken;
    (void)extraCtx;

    if (data == 0 || dataSize < 0) {
        return;
    }

    MemoryReader reader = {static_cast<const unsigned char *>(data),
                           static_cast<const unsigned char *>(data) + dataSize};

    zEffectAnimRunningSaveHeader header = {0};
    if (!ReadMemory(&reader, &header, sizeof(header))) {
        return;
    }

    zEffectAnimEntry *entry = &g_zEffectAnim_EntryList[header.entryTableIndex];
    zClass_NodePartial *const rootNode = GameZ_ZBD::NodeIndexToPtr(header.rootNodeIndex);

    if (header.matchSavedRootNode != 0) {
        zEffectAnimEntry *sibling = entry->runtimeSibling;
        if (sibling != 0) {
            while (entry->boundNode != rootNode) {
                entry = sibling;
                sibling = entry->runtimeSibling;
                if (sibling == 0) {
                    break;
                }
            }
        }

        if (entry->boundNode != rootNode) {
            sibling = entry->runtimeSibling;
            if (sibling != 0) {
                while (entry->activationState == 2) {
                    entry = sibling;
                    sibling = entry->runtimeSibling;
                    if (sibling == 0) {
                        break;
                    }
                }
            }

            if (entry->activationState == 2) {
                entry->runtimeSibling = zEffectAnim::CloneEntryForNode(entry, rootNode);
            }
        }
    }

    entry->flags |= 0x4000u;
    entry->resetScratch[0] = static_cast<unsigned int>(
        (unsigned int)(GameZ_ZBD::NodeIndexToPtr(header.nodeRefAIndex)));
    StoreResetScratchVec(entry, 1, &header.refVecA);
    entry->resetScratch[4] = static_cast<unsigned int>(
        (unsigned int)(GameZ_ZBD::NodeIndexToPtr(header.nodeRefBIndex)));
    StoreResetScratchVec(entry, 5, &header.refVecB);
    entry->activationState = header.activationState;
    entry->triggerCurrentValue = header.triggerCurrentValue;
    entry->activationCountdown = header.activationCountdown;
    entry->velocityX = header.velocityX;
    entry->velocityY = header.velocityY;
    entry->velocityZ = header.velocityZ;
    entry->runtimeSequenceCount = header.runtimeSurfaceCount;

    for (int i = 0; i < entry->runtimeSequenceCount; ++i) {
        zEffectAnimSurfaceRuntime *const runtime = &entry->runtimeList[i];
        if (runtime->eventStream != 0) {
            free(runtime->eventStream);
            runtime->eventStream = 0;
        }

        if (!ReadMemory(&reader, runtime, sizeof(*runtime))) {
            return;
        }

        if (runtime->eventStreamSize > 0) {
            const unsigned int currentEventOffset =
                (unsigned int)(runtime->currentEvent);
            void *const eventStream = malloc(runtime->eventStreamSize);
            if (eventStream == 0) {
                runtime->eventStream = 0;
                return;
            }

            runtime->currentEvent = static_cast<unsigned char *>(eventStream) + currentEventOffset;
            runtime->eventStream = eventStream;
            if (!ReadMemory(&reader, runtime->eventStream, runtime->eventStreamSize)) {
                return;
            }
        }
    }

    for (int i_2831 = 0; i_2831 < header.lightRefCount; ++i_2831) {
        zEffectAnimRuntimeNodeSaveRecord record = {0};
        if (!ReadMemory(&reader, &record, sizeof(record))) {
            return;
        }

        if (record.isAttached == 0) {
            continue;
        }

        const int lightIndex = zEffectAnim::FindOrCreateLightRef(entry, record.name);
        if (lightIndex < 0) {
            continue;
        }

        zEffectAnimRuntimeNodeRef *const lightRef = &entry->lightRefList[lightIndex];
        zClass_NodePartial *const lightNode = lightRef->runtimeNode;
        if (lightRef->isAttached != 0 || lightNode == 0) {
            continue;
        }

        zClass_Class::gwNodeSetActive(lightNode, 1);
        if (record.parentNodeIndex >= 0 && lightNode->listCountA == 0) {
            zClass_Class::AddChild(GameZ_ZBD::NodeIndexToPtr(record.parentNodeIndex), lightNode);
        }
        zClass_Light::gwLightSetPosition(lightNode, record.posX, record.posY, record.posZ);
        zClass_World::AddLight(g_zEffect_World, lightNode);
        lightRef->isAttached = 1;
    }

    for (int i_2861 = 0; i_2861 < header.soundRefCount; ++i_2861) {
        zEffectAnimSoundNodeSaveRecord record = {0};
        if (!ReadMemory(&reader, &record, sizeof(record))) {
            return;
        }

        if (record.isAttached == 0) {
            continue;
        }

        const int soundIndex = zEffectAnim::FindOrCreateSoundRef(entry, record.name);
        if (soundIndex < 0) {
            continue;
        }

        zEffectAnimRuntimeNodeRef *const soundRef = &entry->soundRefList[soundIndex];
        zClass_NodePartial *const soundNode = soundRef->runtimeNode;
        if (soundRef->isAttached != 0 || soundNode == 0) {
            continue;
        }

        zClass_Class::gwNodeSetActive(soundNode, 1);
        if (record.parentNodeIndex >= 0 && soundNode->listCountA == 0) {
            zClass_Class::AddChild(GameZ_ZBD::NodeIndexToPtr(record.parentNodeIndex), soundNode);
        }
        if (record.hasPosition != 0) {
            zClass_Sound::gwSoundSetPosition(soundNode, record.posX, record.posY, record.posZ);
        }
        zClass_World::AddSound(g_zEffect_World, soundNode);
        soundRef->isAttached = 1;
    }

    entry->runtimeNode->callbackContext = (zClass_NodePartial *)(entry);
    zClass_Class::gwNodeSetActionCallbackTail(entry->runtimeNode,
                                              (void *)(&zEffect_Anim::RunSequence));
}

// Reimplements 0x461430: zEffect_Anim::SaveAnimRecords
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE int RECOIL_FASTCALL SaveAnimRecords(zZbdSectionCallbackCtx *callbackCtx) {
    int result = 1;
    for (int i = 1; result != 0 && i < g_zEffectAnim_EntryCount; ++i) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i];
        if (entry->activationState == 5) {
            continue;
        }

        const unsigned short flags = static_cast<unsigned short>(entry->flags);
        if (((flags & 0x1000) != 0 && (flags & 0x2000) == 0) ||
            g_zEffectAnim_RecordQueueEnabled == 0) {
            continue;
        }

        const unsigned char trackedNodeCount =
            entry->trackedNodeList != 0 ? entry->trackedNodeCount : 0;
        const size_t payloadSize = sizeof(zEffectAnimSaveHeader) +
                                        sizeof(zEffectAnimTrackedNodeSaveRecord) * trackedNodeCount;
        unsigned char *const payload = static_cast<unsigned char *>(calloc(1, payloadSize));
        if (payload == 0) {
            return 0;
        }

        zEffectAnimSaveHeader *const header = (zEffectAnimSaveHeader *)(payload);
        strncpy(header->base.animName, entry->name, sizeof(header->base.animName));
        header->entryTableIndex = i;
        header->savedActivationState = entry->activationState;
        header->trackedNodeCount = trackedNodeCount;

        zEffectAnimTrackedNodeSaveRecord *records =
            (zEffectAnimTrackedNodeSaveRecord *)(payload + sizeof(*header));
        {
        for (int childIndex = 0; childIndex < trackedNodeCount; ++childIndex) {
            zEffectAnimTrackedNodeSaveRecord *const record = &records[childIndex];
            zClass_NodePartial *const node = entry->trackedNodeList[childIndex].trackedNode;
            if (node == 0 || node->classId != 5) {
                record->nodeIndex = -1;
                continue;
            }

            zClass_Object3DDataPartial *const objectData =
                static_cast<zClass_Object3DDataPartial *>(node->classData);
            record->nodeIndex = zClass::NodePtrToValidatedIndex(node);
            record->activeFlag = (node->flags >> 2) & 1;
            record->usesCachedMatrix = (objectData->flags >> 4) & 1;
            if (record->usesCachedMatrix != 0) {
                memcpy(record->transform, zClass_Object3D::gwObject3DGetMatrixPtr(node),
                            sizeof(record->transform));
            } else {
                zClass_Object3D::gwObject3DGetPosition(
                    node, &record->transform[0], &record->transform[1], &record->transform[2]);
                zClass_Object3D::gwObject3DGetRotation(
                    node, &record->transform[3], &record->transform[4], &record->transform[5]);
                zClass_Object3D::gwObject3DGetScale(node, &record->transform[6],
                                                    &record->transform[7], &record->transform[8]);
            }
        }
        }

        char sectionName[0x14];
        sprintf(sectionName, "Anim%04d", i + g_zEffectAnim_ActivationRecordCount);
        result = zUtil_ZAR::WriteSectionBlob(callbackCtx, sectionName, payload,
                                             static_cast<unsigned int>(payloadSize));
        free(payload);
    }

    return result;
}

// Reimplements 0x461670: zEffect_Anim::LoadAnimRecords
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE void RECOIL_FASTCALL LoadAnimRecords(void *unused, const char *sectionToken,
                                                     void *data, int dataSize,
                                                     void *extraCtx) {
    (void)unused;
    (void)sectionToken;
    (void)extraCtx;

    if (data == 0 || dataSize < static_cast<int>(sizeof(zEffectAnimSaveHeader))) {
        return;
    }

    zEffectAnimSaveHeader *const header = static_cast<zEffectAnimSaveHeader *>(data);
    zEffectAnimEntry *entry = &g_zEffectAnim_EntryList[header->entryTableIndex];
    if (entry == 0 || (entry->flags & 0x4000u) != 0) {
        return;
    }

    if (header->savedActivationState == 1 && entry->activationState != 1) {
        if (entry->activationState == 4) {
            entry->activationState = 3;
        }
        NodeActionCallback(entry, entry->boundNode);
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x419,
                          "zEffAnimReset: %s", entry);
    }

    for (zEffectAnimEntry *cursor = entry; cursor != 0; cursor = cursor->runtimeSibling) {
        if ((cursor->flags & 0x4000u) == 0 && cursor->activationState == 2) {
            NodeActionCallback(cursor, cursor->boundNode);
            zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x41f,
                              "zEffAnimReset: %s", cursor);
        }
    }

    const size_t requiredSize =
        sizeof(*header) + sizeof(zEffectAnimTrackedNodeSaveRecord) * header->trackedNodeCount;
    if (dataSize < static_cast<int>(requiredSize)) {
        return;
    }

    zEffectAnimTrackedNodeSaveRecord *records =
        (zEffectAnimTrackedNodeSaveRecord *)(static_cast<unsigned char *>(data) +
                                                             sizeof(*header));
    for (int i = 0; i < header->trackedNodeCount; ++i) {
        zEffectAnimTrackedNodeSaveRecord *const record = &records[i];
        zClass_NodePartial *const node = GameZ_ZBD::NodeIndexToPtr(record->nodeIndex);
        if (node == 0 || node->classId != 5) {
            continue;
        }

        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zEffect\\zeff_anim_save.c", 0x426,
                          "Restore node: %s %d", node, record->activeFlag);
        zClass_Class::gwNodeSetActive(node, record->activeFlag != 0 ? 1 : 0);
        if (record->activeFlag == 0) {
            continue;
        }

        if (record->usesCachedMatrix != 0) {
            zClass_Object3D::gwObject3DSetMatrix(node, record->transform);
        } else {
            zClass_Object3D::gwObject3DSetPosition(node, record->transform[0], record->transform[1],
                                                   record->transform[2]);
            zClass_Object3D::gwObject3DSetRotation(node, record->transform[3], record->transform[4],
                                                   record->transform[5]);
            zClass_Object3D::gwObject3DSetScale(node, record->transform[6], record->transform[7],
                                                record->transform[8]);
        }
    }
}

// Reimplements 0x461840: zEffect_Anim::ResetFromActivationRecord
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE void RECOIL_FASTCALL
ResetFromActivationRecord(zEffectAnimActivationRecord *record) {
    NodeActionCallback(zEffectAnim::FindEntryByName(record->animName),
                       GameZ_ZBD::NodeIndexToPtr(record->nodeToken));
}

// Reimplements 0x461870: zEffect_Anim::ProcessActivationRecord
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL
ProcessActivationRecord(zEffectAnimActivationRecord *record) {
    zEffectAnimEntry *const entry = zEffectAnim::FindEntryByName(record->animName);
    if (entry == 0) {
        return 0;
    }

    zClass_NodePartial *const rootNode = GameZ_ZBD::NodeIndexToPtr(record->nodeToken);
    switch (record->commandType) {
    case 1:
        return zEffectAnim::SetTransformRotAndVelocity_Thunk(
            entry, rootNode, record->params[0].f32, record->params[1].f32, record->params[2].f32,
            record->params[3].f32, record->params[4].f32, record->params[5].f32,
            record->params[6].f32, record->params[7].f32, record->params[8].f32);

    case 2:
        return zEffectAnim::SetVelocity_Thunk(entry, rootNode, record->params[0].f32,
                                              record->params[1].f32, record->params[2].f32);

    case 3:
        return zEffectAnim::SetPositionRefAndVelocity_Thunk(
            entry, rootNode, GameZ_ZBD::NodeIndexToPtr(record->params[0].i32),
            (const zVec3 *)(&record->params[1]),
            (const zVec3 *)(&record->params[4]));

    case 4:
        return zEffectAnim::SetTransformRefs_Thunk(
            entry, rootNode, GameZ_ZBD::NodeIndexToPtr(record->params[0].i32),
            (const zVec3 *)(&record->params[1]),
            GameZ_ZBD::NodeIndexToPtr(record->params[4].i32),
            (const zVec3 *)(&record->params[5]));

    default:
        return 0;
    }
}

// Reimplements 0x45d240: zEffect_Anim::CaptureNodeStates
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL CaptureNodeStates(zEffectAnimEntry *self) {
    if (self == 0) {
        return -1;
    }

    for (int i = 0; i < self->trackedNodeCount; ++i) {
        zEffectAnimTrackedNode *const tracked = &self->trackedNodeList[i];
        zClass_NodePartial *const node = tracked->trackedNode;
        if (node == 0) {
            continue;
        }

        zEffectAnimCapturedNodeState *const state = &tracked->capturedState;
        state->activeFlag = (node->flags >> 2) & 1;
        if (node->classId != 5) {
            continue;
        }

        zClass_Object3DDataPartial *const objectData =
            static_cast<zClass_Object3DDataPartial *>(node->classData);
        state->usesCachedMatrix = (objectData->flags >> 4) & 1;
        if (state->usesCachedMatrix != 0) {
            memcpy(state->transformSnapshot, zClass_Object3D::gwObject3DGetMatrixPtr(node),
                        sizeof(state->transformSnapshot));
        } else {
            zClass_Object3D::gwObject3DGetPosition(node, &state->transformSnapshot[0],
                                                   &state->transformSnapshot[1],
                                                   &state->transformSnapshot[2]);
            zClass_Object3D::gwObject3DGetRotation(node, &state->transformSnapshot[3],
                                                   &state->transformSnapshot[4],
                                                   &state->transformSnapshot[5]);
            zClass_Object3D::gwObject3DGetScale(node, &state->transformSnapshot[6],
                                                &state->transformSnapshot[7],
                                                &state->transformSnapshot[8]);
        }
    }

    return 0;
}

// Reimplements 0x45d310: zEffect_Anim::RestoreNodeStates
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL RestoreNodeStates(zEffectAnimEntry *self) {
    if (self == 0) {
        return -1;
    }

    for (int i = 0; i < self->trackedNodeCount; ++i) {
        zEffectAnimTrackedNode *const tracked = &self->trackedNodeList[i];
        zClass_NodePartial *const node = tracked->trackedNode;
        if (node == 0) {
            continue;
        }

        zEffectAnimCapturedNodeState *const state = &tracked->capturedState;
        zClass_Class::gwNodeSetActive(node, state->activeFlag);
        if (node->classId == 5) {
            if (state->usesCachedMatrix != 0) {
                zClass_Object3D::gwObject3DSetMatrix(node, state->transformSnapshot);
            } else {
                zClass_Object3D::gwObject3DSetPosition(node, state->transformSnapshot[0],
                                                       state->transformSnapshot[1],
                                                       state->transformSnapshot[2]);
                zClass_Object3D::gwObject3DSetRotation(node, state->transformSnapshot[3],
                                                       state->transformSnapshot[4],
                                                       state->transformSnapshot[5]);
                zClass_Object3D::gwObject3DSetScale(node, state->transformSnapshot[6],
                                                    state->transformSnapshot[7],
                                                    state->transformSnapshot[8]);
            }
        }

        zDiPartial *const di = (zDiPartial *)(node->userDataOrDiRef);
        if (di != 0) {
            di->flags &= ~0x08;
            di->blendScale = 0.0f;
        }
    }

    return 0;
}

// Reimplements 0x45ae30: zEffect_Anim::AdvanceKeyframeSample
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL AdvanceKeyframeSample(
    zEffectAnimSurfaceRuntime *sequenceRuntime, zEffectKeyframeEvent *keyframeEvent,
    zEffectKeyframeSampleHeader *sampleHeader) {
    const int nextSampleOffset =
        keyframeEvent->currentKeyframeOffset +
        static_cast<int>(sizeof(zEffectKeyframeSampleHeader));

    ++keyframeEvent->lookaheadAdvanceCount;
    keyframeEvent->currentKeyframeOffset = nextSampleOffset;

    const int channelFlags = sampleHeader->channelFlags;
    if ((channelFlags & 0x01) != 0) {
        keyframeEvent->currentKeyframeOffset =
            nextSampleOffset + static_cast<int>(sizeof(zEffectKeyframeSampleChannel));
    }
    if ((channelFlags & 0x02) != 0) {
        keyframeEvent->currentKeyframeOffset +=
            static_cast<int>(sizeof(zEffectKeyframeSampleChannel));
    }
    if ((channelFlags & 0x04) != 0) {
        keyframeEvent->currentKeyframeOffset +=
            static_cast<int>(sizeof(zEffectKeyframeSampleChannel));
    }

    keyframeEvent->keyframeLocalTime = 0.0f;

    zEffectAnimEventHeader *const currentEvent =
        static_cast<zEffectAnimEventHeader *>(sequenceRuntime->currentEvent);
    return keyframeEvent->currentKeyframeOffset < currentEvent->recordSize ? 1 : 0;
}

// Reimplements 0x45ae90: zEffect_Anim::AnimateKeyframeSample
// (zeff_anim_run.c)
RECOIL_NOINLINE float RECOIL_FASTCALL AnimateKeyframeSample(
    zEffectAnimSurfaceRuntime *sequenceRuntime, zEffectKeyframeEvent *keyframeEvent,
    zClass_NodePartial *targetNode, zEffectKeyframeSampleHeader *sampleHeader, float *deltaTime) {
    float preStartDelaySec = 0.0f;
    if (sequenceRuntime->eventElapsedSec < sampleHeader->startTimeSec) {
        const float savedDeltaTimeSec = *deltaTime;
        *deltaTime = sequenceRuntime->eventElapsedSec;
        return savedDeltaTimeSec;
    }

    if (*deltaTime < sampleHeader->startTimeSec) {
        preStartDelaySec = sampleHeader->startTimeSec - *deltaTime;
        *deltaTime = sampleHeader->startTimeSec;
    }

    float sampleEndTimeSec = sequenceRuntime->eventElapsedSec;
    if (sequenceRuntime->eventElapsedSec > sampleHeader->endTimeSec) {
        const float savedLocalTime = keyframeEvent->keyframeLocalTime;
        const int savedOffset = keyframeEvent->currentKeyframeOffset;
        if (AdvanceKeyframeSample(sequenceRuntime, keyframeEvent, sampleHeader) != 0) {
            zEffectKeyframeSampleHeader *const nextSample =
                (zEffectKeyframeSampleHeader *)(
                    (unsigned char *)(keyframeEvent) +
                    keyframeEvent->currentKeyframeOffset);
            if (nextSample->channelFlags == sampleHeader->channelFlags) {
                const float consumed = sampleHeader->endTimeSec - *deltaTime + preStartDelaySec;
                *deltaTime = sampleHeader->endTimeSec;
                keyframeEvent->keyframeLocalTime = sampleHeader->endTimeSec;
                keyframeEvent->currentKeyframeOffset = savedOffset;
                return consumed;
            }
        }

        keyframeEvent->currentKeyframeOffset = savedOffset;
        keyframeEvent->keyframeLocalTime = savedLocalTime;
        --keyframeEvent->lookaheadAdvanceCount;
        sampleEndTimeSec = sampleHeader->endTimeSec;
    }

    zEffectKeyframeSampleChannel *sampleChannel =
        (zEffectKeyframeSampleChannel *)(sampleHeader + 1);
    const float sampleDurationSec = sampleEndTimeSec - *deltaTime;
    keyframeEvent->keyframeLocalTime += sampleDurationSec;
    const float sampleLocalTime = keyframeEvent->keyframeLocalTime;

    zVec3 outEuler = {0};
    if ((sampleHeader->channelFlags & 0x01) != 0) {
        outEuler.x = sampleChannel->rate.x * sampleLocalTime + sampleChannel->baseQuat.w;
        outEuler.y = sampleChannel->rate.y * sampleLocalTime + sampleChannel->baseQuat.x;
        outEuler.z = sampleChannel->rate.z * sampleLocalTime + sampleChannel->baseQuat.y;

        if (targetNode->classId == 1) {
            zClass_Camera::gwCameraSetTarget(targetNode, outEuler.x, outEuler.y, outEuler.z);
        } else if (targetNode->classId == 5) {
            zClass_Object3D::gwObject3DSetPosition(targetNode, outEuler.x, outEuler.y, outEuler.z);
        }

        ++sampleChannel;
    }

    if ((sampleHeader->channelFlags & 0x02) != 0) {
        const zVec3 rotationVector = {sampleChannel->rate.x * sampleLocalTime,
                                   sampleChannel->rate.y * sampleLocalTime,
                                   sampleChannel->rate.z * sampleLocalTime};

        zQuat deltaQuat = {0};
        zMath_Quat_FromRotationVector(&rotationVector, &deltaQuat);

        zQuat blendedQuat = {0};
        zMath_Quat_Multiply(&deltaQuat, &sampleChannel->baseQuat, &blendedQuat);

        zMat4x3 rotationMatrix = {0};
        zMath_Quat_ToMatrix(&blendedQuat, &rotationMatrix);
        zMath_Mat_ExtractEulerAngles(&rotationMatrix, &outEuler);

        if (targetNode->classId == 1) {
            zClass_Camera::gwCameraSetPosition(targetNode, outEuler.x, outEuler.y, outEuler.z);
        } else if (targetNode->classId == 5) {
            zClass_Object3D::gwObject3DSetRotation(targetNode, outEuler.x, outEuler.y, outEuler.z);
        }

        ++sampleChannel;
    }

    if ((sampleHeader->channelFlags & 0x04) != 0) {
        outEuler.x =
            sampleChannel->rate.x * keyframeEvent->keyframeLocalTime + sampleChannel->baseQuat.w;
        outEuler.y =
            sampleChannel->rate.y * keyframeEvent->keyframeLocalTime + sampleChannel->baseQuat.x;
        outEuler.z =
            sampleChannel->rate.z * keyframeEvent->keyframeLocalTime + sampleChannel->baseQuat.y;

        if (targetNode->classId == 5) {
            zClass_Object3D::gwObject3DSetScale(targetNode, outEuler.x, outEuler.y, outEuler.z);
        }
    }

    *deltaTime += sampleDurationSec;
    return preStartDelaySec + sampleDurationSec;
}

// Reimplements 0x45b120: zEffect_Anim::AdvanceKeyframe
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
AdvanceKeyframe(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                zEffectKeyframeEvent *keyframeEvent) {
    int runState = 1;
    if (self == 0 || sequenceRuntime == 0 || keyframeEvent == 0 ||
        keyframeEvent->targetNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const targetNode =
        self->nodeRefList[keyframeEvent->targetNodeRefIndex].node;
    if (sequenceRuntime->runState == 0) {
        keyframeEvent->currentKeyframeOffset =
            static_cast<int>(sizeof(zEffectKeyframeEvent));
        keyframeEvent->keyframeLocalTime = 0.0f;
        keyframeEvent->lookaheadAdvanceCount = 0;
    }

    float sampleTimeSec = sequenceRuntime->eventElapsedSec - g_zEffect_FrameDeltaRemainingSec;

    while (keyframeEvent->currentKeyframeOffset < keyframeEvent->header.recordSize) {
        zEffectKeyframeSampleHeader *const sampleHeader =
            (zEffectKeyframeSampleHeader *)(
                (unsigned char *)(keyframeEvent) +
                keyframeEvent->currentKeyframeOffset);
        const float consumedSec = AnimateKeyframeSample(sequenceRuntime, keyframeEvent, targetNode,
                                                        sampleHeader, &sampleTimeSec);
        g_zEffect_FrameDeltaRemainingSec -= consumedSec;

        if (sampleTimeSec < sequenceRuntime->eventElapsedSec &&
            AdvanceKeyframeSample(sequenceRuntime, keyframeEvent, sampleHeader) == 0) {
            runState = 2;
        }

        if (sampleTimeSec >= sequenceRuntime->eventElapsedSec || runState == 2) {
            break;
        }
    }

    return keyframeEvent->currentKeyframeOffset < keyframeEvent->header.recordSize ? runState : 2;
}

// Reimplements 0x45b210: zEffect_Anim::EvaluateKeyframe
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
EvaluateKeyframe(zEffectAnimEntry *self, zEffectEvaluateKeyframeEvent *keyframeEvent) {
    zClass_NodePartial *targetNode = 0;
    if (keyframeEvent->targetNodeRefIndex >= 0) {
        targetNode = self->nodeRefList[keyframeEvent->targetNodeRefIndex].node;
    } else if (keyframeEvent->targetNodeRefIndex == -100) {
        targetNode = self->boundNode;
    }

    if (targetNode != 0) {
        zClass_Object3D::gwObject3DSetLitFlag(targetNode, keyframeEvent->litFlag == 1 ? 1 : 0);

        if (keyframeEvent->hasAlphaScale == 1) {
            zClass_Object3D::gwObject3DSetAlphaScale(targetNode, keyframeEvent->alphaScale);
        }
    }

    return 2;
}

// Reimplements 0x45b280: zEffect_Anim::RunKeyframes
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
RunKeyframes(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
             zEffectRunKeyframeEvent *keyframeEvent) {
    if (self == 0 || sequenceRuntime == 0 || keyframeEvent == 0 ||
        keyframeEvent->targetNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const targetNode =
        self->nodeRefList[keyframeEvent->targetNodeRefIndex].node;
    if (sequenceRuntime->runState == 0) {
        if (keyframeEvent->startLitFlag == 1) {
            zClass_Object3D::gwObject3DSetLitFlag(targetNode, 1);
        } else if (keyframeEvent->startLitFlag == 0) {
            zClass_Object3D::gwObject3DSetLitFlag(targetNode, 0);
        }

        zClass_Object3D::gwObject3DSetAlphaScale(targetNode, keyframeEvent->startAlphaScale);
    }

    const float frameDeltaUsedSec =
        sequenceRuntime->eventElapsedSec <= keyframeEvent->endTimeSec
            ? g_zEffect_FrameDeltaRemainingSec
            : g_zEffect_FrameDeltaRemainingSec -
                  (sequenceRuntime->eventElapsedSec - keyframeEvent->endTimeSec);

    float alphaScale = 0.0f;
    if (zClass_Object3D::gwObject3DGetAlphaScale(targetNode, &alphaScale) != 0) {
        return 2;
    }

    zClass_Object3D::gwObject3DSetAlphaScale(
        targetNode, keyframeEvent->alphaScaleRate * frameDeltaUsedSec + alphaScale);
    g_zEffect_FrameDeltaRemainingSec -= frameDeltaUsedSec;

    if (sequenceRuntime->eventElapsedSec <= keyframeEvent->endTimeSec) {
        return 1;
    }

    if (keyframeEvent->endLitFlag == 1) {
        zClass_Object3D::gwObject3DSetLitFlag(targetNode, 1);
    } else if (keyframeEvent->endLitFlag == 0) {
        zClass_Object3D::gwObject3DSetLitFlag(targetNode, 0);
    }

    zClass_Object3D::gwObject3DSetAlphaScale(targetNode, keyframeEvent->endAlphaScale);
    return 2;
}

// Reimplements 0x45cc00: zEffect_Anim::RunSequenceEvents
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
RunSequenceEvents(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime) {
    if (self == 0 || sequenceRuntime == 0 || sequenceRuntime->currentEvent == 0) {
        if (sequenceRuntime != 0) {
            sequenceRuntime->runState = 2;
        }
        return -1;
    }

    zEffectAnimEventHeader *currentEvent =
        static_cast<zEffectAnimEventHeader *>(sequenceRuntime->currentEvent);
    if (sequenceRuntime->runState == 1 ||
        currentEvent > static_cast<zEffectAnimEventHeader *>(sequenceRuntime->eventStream)) {
        sequenceRuntime->sequenceElapsedSec += g_zEffect_FrameDeltaRemainingSec;
        sequenceRuntime->eventElapsedSec += g_zEffect_FrameDeltaRemainingSec;
    }

    while (true) {
        currentEvent = static_cast<zEffectAnimEventHeader *>(sequenceRuntime->currentEvent);

        int startSatisfied = 0;
        switch (currentEvent->startMode) {
        case 1:
            startSatisfied = self->triggerCurrentValue >= currentEvent->startThreshold;
            break;
        case 2:
            startSatisfied = sequenceRuntime->sequenceElapsedSec >= currentEvent->startThreshold;
            break;
        case 3:
            startSatisfied = sequenceRuntime->eventElapsedSec >= currentEvent->startThreshold;
            break;
        default:
            zError::ReportOld(0x400, kZeffAnimRunSourceFile, 0x15fe,
                              "Invalid Start Time\n  Animation: %s\n", self);
            return -1;
        }

        if (startSatisfied == 0) {
            return 0;
        }

        sequenceRuntime->eventElapsedSec = g_zEffect_FrameDeltaRemainingSec;
        if (currentEvent == static_cast<zEffectAnimEventHeader *>(sequenceRuntime->eventStream)) {
            sequenceRuntime->sequenceElapsedSec = g_zEffect_FrameDeltaRemainingSec;
        }

        const int dispatchResult =
            DispatchSequenceEvent(self, sequenceRuntime, currentEvent);
        if (dispatchResult < 0) {
            return dispatchResult;
        }

        sequenceRuntime->runState = static_cast<unsigned char>(dispatchResult);
        if (sequenceRuntime->runState == 2) {
            unsigned char *const currentEventBytes =
                static_cast<unsigned char *>(sequenceRuntime->currentEvent);
            sequenceRuntime->eventElapsedSec = 0.0f;
            unsigned char *const nextEvent = currentEventBytes + currentEvent->recordSize;
            sequenceRuntime->currentEvent = nextEvent;

            unsigned char *const eventStreamEnd =
                static_cast<unsigned char *>(sequenceRuntime->eventStream) +
                sequenceRuntime->eventStreamSize;
            if (nextEvent < eventStreamEnd) {
                sequenceRuntime->runState = 0;
            } else if (sequenceRuntime->resetMode == 3) {
                zEffect::HandleEmitterResetEvent(sequenceRuntime);
            }
            return 0;
        }

        if (sequenceRuntime->runState != 0) {
            return 0;
        }
    }
}

// Reimplements 0x45d010: zEffect_Anim::RunSequence (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL RunSequence(zClass_NodePartial *node) {
    int allSequencesFinished = 1;
    if (node == 0) {
        return 0;
    }

    zEffectAnimEntry *const entry = (zEffectAnimEntry *)(node->callbackContext);
    if (entry == 0) {
        return 0;
    }

    const unsigned int flags = entry->flags;
    if ((flags & 0x0eu) != 0) {
        if (entry->variantCycleDelay > 0) {
            --entry->variantCycleDelay;
            return entry->variantCycleDelay;
        }

        if ((flags & 0x02u) != 0) {
            if (g_zEffect_ConditionalRefPosEnabled == 0) {
                return 0;
            }

            const float distanceSq = zEffect::GetConditionalRefPosDistanceSq(entry->callbackNode);
            if (distanceSq < entry->distRefMinSq || distanceSq >= entry->distRefMaxSq) {
                AdvanceVariantCycleDelay(entry);
                return 0;
            }
        }

        if ((entry->flags & 0x08u) != 0) {
            if (g_zEffect_VariantOverrideEnabled == 0) {
                return 0;
            }

            CopyPackedVariantOverrideToCurrentTag();
            if (VariantTag::CurrentAllowsId(entry->callbackNode->nodeType) == 0) {
                entry->variantCycleDelay = g_zEffect_VariantCycleId;
                ++g_zEffect_VariantCycleId;
                if (g_zEffect_VariantCycleId > 10) {
                    g_zEffect_VariantCycleId = 1;
                }
                return 0;
            }
        }

        if ((entry->flags & 0x04u) != 0) {
            const unsigned int callbackFlags = entry->callbackNode->flags;
            if ((callbackFlags & 0x80000000u) == 0) {
                AdvanceVariantCycleDelay(entry);
                return 0;
            }
            entry->callbackNode->flags = callbackFlags & 0x7fffffffu;
        }
    }

    entry->triggerCurrentValue += g_FrameDeltaTimeSec;
    zEffectAnimSurfaceRuntime *sequenceRuntime = entry->runtimeList;
    for (int i = 0; i < entry->runtimeSequenceCount; ++i, ++sequenceRuntime) {
        g_zEffect_FrameDeltaRemainingSec = g_FrameDeltaTimeSec;
        const unsigned char runState = sequenceRuntime->runState;
        if (runState == 0 || runState == 1) {
            if (RunSequenceEvents(entry, sequenceRuntime) != 0) {
                entry->activationState = 5;
                zError::ReportOld(0x400, kZeffAnimRunSourceFile, 0x180f,
                                  "Corrupt animation:\n  Animation: %s; Sequence: %s\n", entry,
                                  sequenceRuntime);
            }
        }

        const unsigned char runStateAfterDispatch = sequenceRuntime->runState;
        if (runStateAfterDispatch == 0 || runStateAfterDispatch == 1) {
            allSequencesFinished = 0;
        }
    }

    if (allSequencesFinished != 0) {
        if (entry->eventCallback != 0) {
            entry->eventCallback(entry, entry->eventCallbackContext, 0);
        }
        zEffectAnim::Stop(entry);
    }

    return 0;
}

// Reimplements 0x45d6b0: zEffect_Anim::NodeActionCallback
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL NodeActionCallback(zEffectAnimEntry *self,
                                                                zClass_NodePartial *rootNode) {
    return zEffectAnim::StopAndCleanup(self, rootNode, 1);
}

// Reimplements 0x460ae0: zEffect_Anim::AllocActivationRecord
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_CDECL AllocActivationRecord() {
    zEffectAnimActivationRecord *recordTable = g_zEffectAnim_ActivationRecordTable;
    int recordCount = 0;
    int recordCapacity = 0;

    if (recordTable == 0) {
        recordTable = static_cast<zEffectAnimActivationRecord *>(
            malloc(sizeof(zEffectAnimActivationRecord) * kInitialActivationRecordCapacity));
        recordCapacity = kInitialActivationRecordCapacity;
        recordCount = 0;
        g_zEffectAnim_ActivationRecordTable = recordTable;
        g_zEffectAnim_ActivationRecordCapacity = recordCapacity;
        g_zEffectAnim_ActivationRecordCount = 0;
    } else {
        recordCount = g_zEffectAnim_ActivationRecordCount;
        recordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    }

    if (recordCount >= recordCapacity) {
        zEffectAnimActivationRecord *const oldTable = recordTable;
        recordTable = static_cast<zEffectAnimActivationRecord *>(
            malloc(sizeof(zEffectAnimActivationRecord) * recordCapacity * 2));
        g_zEffectAnim_ActivationRecordTable = recordTable;
        memcpy(recordTable, oldTable, sizeof(zEffectAnimActivationRecord) * recordCapacity);
        g_zEffectAnim_ActivationRecordCapacity = recordCapacity * 2;
        free(oldTable);
        recordCount = g_zEffectAnim_ActivationRecordCount;
    }

    zEffectAnimActivationRecord *const record = &recordTable[recordCount];
    record->recordId =
        (g_zEffectAnim_ActivationDispatchTagHigh & static_cast<unsigned int>(recordCount)) &
        0x00ffffffu;
    g_zEffectAnim_ActivationRecordCount = recordCount + 1;
    return record;
}

// Reimplements 0x461800: zEffect_Anim::GetActivationRecordPackedSize
// (GameZRecoil/zEffect/zeff_anim_save.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
GetActivationRecordPackedSize(zEffectAnimActivationRecord *record) {
    switch (record->commandType) {
    case 1:
        return 0x38;
    case 2:
        return 0x48;
    case 3:
        return 0x4c;
    default:
        return 0x50;
    }
}

// Reimplements 0x461a90: zEffect_Anim::DiscardLastActivationRecord
RECOIL_NOINLINE void RECOIL_CDECL DiscardLastActivationRecord() {
    --g_zEffectAnim_ActivationRecordCount;
}

// Reimplements 0x461eb0: zEffect_Anim::SetActivationDispatchContext
RECOIL_NOINLINE void RECOIL_FASTCALL SetActivationDispatchContext(
    void(RECOIL_FASTCALL *callback)(zEffectAnimActivationRecord *record), int context) {
    g_zEffectAnim_ActivationDispatchCallback = callback;
    g_zEffectAnim_ActivationDispatchTagHigh = static_cast<unsigned int>(context) << 24;
}

// Reimplements 0x45fe50: zEffect_Anim::Shutdown
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    if (g_zEffectAnim_HeapPtr != 0) {
        free(g_zEffectAnim_HeapPtr);
        g_zEffectAnim_HeapPtr = 0;
    }

    g_zEffectAnim_CountsPackedLoWord = 0;
    for (int i = 0; i < g_zEffectAnim_EntryCount; ++i) {
        zEffectAnim::ShutdownEntry(&g_zEffectAnim_EntryList[i]);
    }

    if (g_zEffectAnim_EntryList != 0) {
        free(g_zEffectAnim_EntryList);
        g_zEffectAnim_EntryList = 0;
    }

    g_zEffectAnim_EntryCount = 0;
    if (g_zEffectAnim_TextIdEntryList != 0) {
        free(g_zEffectAnim_TextIdEntryList);
        g_zEffectAnim_TextIdEntryList = 0;
    }

    g_zEffectAnim_TextIdEntryCount = 0;
    ClearActivationRecords();
    g_zEffectAnim_EntriesInstantiated = 0;
    return 0;
}

// Reimplements 0x45fef0: zEffect_Anim::ShutdownIfLoaded
RECOIL_NOINLINE int RECOIL_CDECL ShutdownIfLoaded() {
    if (g_zEffectAnim_EntriesInstantiated != 0) {
        Shutdown();
    }

    return 0;
}

// Reimplements 0x45e100: zEffect_Anim::Init
RECOIL_NOINLINE int RECOIL_CDECL Init() {
    if (g_zEffectAnim_EntriesInstantiated != 0) {
        Shutdown();
    }

    g_zEffectAnim_ZbdFilename[0] = '\0';
    g_zEffectAnim_EntriesInstantiated = 0;
    g_zEffectAnim_HeapPtr = 0;
    g_zEffectAnim_CountsPackedLoWord = 0;
    g_zEffectAnim_EntryCount = 0;
    g_zEffectAnim_EntryList = 0;
    g_zEffectAnim_TextIdEntryCount = 0;
    g_zEffectAnim_TextIdEntryList = 0;
    g_zEffect_World = 0;
    g_zEffect_ConditionalRefPosEnabled = 0;
    g_zEffect_VariantOverrideEnabled = 0;
    g_zEffect_VariantOverridePackedIds = 0;
    g_zEffect_VariantCycleId = 1;
    g_zEffect_SkipStopDelay = 0;
    g_zEffect_FrameDeltaRemainingSec = 0.0f;
    g_zEffect_DefaultGravity = -9.8f;

    srand(static_cast<unsigned int>(time(0)));
    *(unsigned int *)(&g_zEffect_RandUnitScale) = kRandUnitScaleBits;
    {
        int valueIndex1;
        for (valueIndex1 = 0; valueIndex1 < (int)(sizeof(g_zEffect_RandUnitTable) / sizeof((g_zEffect_RandUnitTable)[0])); ++valueIndex1) {
            float & value = (g_zEffect_RandUnitTable)[valueIndex1];
        value = static_cast<float>(rand()) * g_zEffect_RandUnitScale;
    }
    }

    if (g_zEffectAnim_EnableZarRegistration != 0) {
        zUtil_ZAR::RegisterSectionHandler(g_zEffectAnim_ZarSectionName_AnimActivation,
                                          kSaveActivationRecordsProc, kLoadActivationRecordsProc,
                                          0x32, 0);
        zUtil_ZAR::RegisterSectionHandler(g_zEffectAnim_ZarSectionName_RunningAnim,
                                          kSaveRunningAnimRecordsProc, kLoadRunningAnimRecordsProc,
                                          0x33, 0);
        zUtil_ZAR::RegisterSectionHandler(g_zEffectAnim_ZarSectionName_Anim, kSaveAnimRecordsProc,
                                          kLoadAnimRecordsProc, 0x34, 0);
    }

    return 0;
}
} // namespace zEffect_Anim

namespace zEffect {
// Reimplements 0x460020: zEffect::Init
RECOIL_NOINLINE int RECOIL_CDECL Init() {
    g_zEffect_RuntimeManager.initialized = 0;
    g_zEffect_RuntimeManager.templateCount = 0;
    g_zEffect_RuntimeManager.loadedTemplateTree = 0;
    g_zEffect_RuntimeManager.freeList = 0;
    g_zEffect_RuntimeManager.templates = 0;
    g_zEffect_RuntimeManager.parentNode = 0;
    g_zEffect_RuntimeManager.freshAllocCount = 0;
    g_zEffect_RuntimeManager.activatedCount = 0;
    g_zEffect_RuntimeManager.recycleCount = 0;
    return zEffect_Anim::Init();
}

// Reimplements 0x460070: zEffect::InitFromPath
// (D:\Proj\GameZRecoil\zEffect\zeff_init.c)
RECOIL_NOINLINE int RECOIL_FASTCALL InitFromPath(zClass_NodePartial *worldNode,
                                                          zClass_NodePartial *cameraNode,
                                                          const char *path) {
    if (g_zEffect_RuntimeManager.initialized != 0) {
        return 0;
    }

    zReader::Node *const rootNode = zReader::LoadNodeFromPath(path, 0, 0);
    g_zEffect_RuntimeManager.loadedTemplateTree = (zClass_NodePartial *)(rootNode);
    if (rootNode == 0) {
        fprintf(stderr, "%s(%d): Failed to read %s\n", kZeffInitSourceFile, 0xd8, path);
        return -1;
    }

    zReader::Node *const effectsNode = zReader_GetNamedNode(rootNode, "EFFECTS");
    g_zEffect_RuntimeManager.templateCount = zReaderArrayCount(effectsNode) - 1;
    g_zEffect_RuntimeManager.templates = static_cast<zEffect_RuntimeEntry *>(
        calloc(g_zEffect_RuntimeManager.templateCount, sizeof(zEffect_RuntimeEntry)));
    g_zEffect_RuntimeManager.parentNode = worldNode;
    g_zEffect_RuntimeManager.listenerNode = cameraNode;

    for (int i = 0; i < g_zEffect_RuntimeManager.templateCount; ++i) {
        zReader::Node *const effectNode = &zReaderArrayBase(effectsNode)[i + 1];
        zReader::Node *const mapsNode = zReader_GetNamedNode(effectNode, "MAPS");
        zEffect_RuntimeEntry *const runtimeEntry = &g_zEffect_RuntimeManager.templates[i];
        runtimeEntry->effectIndex = -1;
        runtimeEntry->modelNodeName = zReaderArrayStringAt(effectNode, 1);
        runtimeEntry->effectName = const_cast<char *>(zReader::ReadNamedString(effectNode, "NAME"));

        zClass_NodePartial *const templateNode =
            zClass::FindByTypeAndName(6, runtimeEntry->modelNodeName);
        runtimeEntry->effectNode = templateNode;
        if (templateNode == 0) {
            fprintf(stderr, "%s(%d): Failed to find node (%s) for (%s)\n", kZeffInitSourceFile,
                         0xf3, runtimeEntry->modelNodeName, runtimeEntry->effectName);
            continue;
        }

        void *const gfxData = FindNodeUserDataRecursive(templateNode);
        if (gfxData == 0) {
            zError::ReportOld(0x400, kZeffInitSourceFile, 0xfb,
                              "Failed to find gfx data for effect (%s)\n",
                              runtimeEntry->modelNodeName);
            continue;
        }

        zClass_Class::gwNodeSetCellPickable(runtimeEntry->effectNode, 0);
        zClass_Class::gwNodeSetRaycastable(runtimeEntry->effectNode, 0);
        zClass_Class::gwNodeSetActive(runtimeEntry->effectNode, 0);
        runtimeEntry->effectIndex = i;
        runtimeEntry->effectGfxData = gfxData;
        zUtil::StoreInt32((int *)(gfxData), 1);

        zModel_MaterialPartial *const material = static_cast<zModel_MaterialPartial *>(gfxData);
        const int textureCount = zReaderArrayCount(mapsNode) - 1;
        zModel_Material::SetCycleTextureCount(material, textureCount);

        float textureSpeed = 0.0f;
        zReader::ReadNamedFloat(effectNode, "SPEED", &textureSpeed);
        zModel_Material::SetCycleTextureSpeed(material, textureSpeed);

        zReader::Node *const loopingNode = zReader_GetNamedNode(effectNode, "LOOPING");
        if (loopingNode != 0) {
            const char *const loopingText = loopingNode->type == zReader::ZRDR_NODE_ARRAY
                                                ? zReaderArrayStringAt(loopingNode, 1)
                                                : loopingNode->value.str;
            zModel_Material::SetCycleTextureLoop(material,
                                                 strcmp(loopingText, "ON") == 0 ? 1 : 0);
        }

        {
        for (int textureIndex = 1; textureIndex <= textureCount; ++textureIndex) {
            zModel_Material::AddCycleTexture(
                material,
                zImage::TexDir_FindOrAppendByPath(zReaderArrayStringAt(mapsNode, textureIndex)));
        }
        }
    }

    zImage::TexDir_LoadPendingEntries();
    g_zEffect_RuntimeManager.freeList = zArchiveList_CreateEmpty();
    g_zEffect_RuntimeManager.recycleCount = 0;
    g_zEffect_RuntimeManager.initialized = 1;
    return 0;
}

// Reimplements 0x45e200: zEffect::SetWorldNode (D:\Proj\GameZRecoil\zEffect\zeff_anim.c)
RECOIL_NOINLINE void RECOIL_FASTCALL SetWorldNode(zClass_NodePartial *worldNode) {
    g_zEffect_World = worldNode;
}

// Reimplements 0x45e270: zEffect::SetResourceNode (D:\Proj\GameZRecoil\zEffect\zeff_anim.c)
RECOIL_NOINLINE void RECOIL_FASTCALL SetResourceNode(zClass_NodePartial *resourceNode) {
    g_zEffect_ResourceNode = resourceNode;
}

// Reimplements 0x458b50: zEffect::TickResetDelayOnTimer
// (D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c)
RECOIL_NOINLINE float RECOIL_FASTCALL TickResetDelayOnTimer(zEffectAnimEntry *self,
                                                            float deltaSec) {
    if (self->activationMode == 1 || self->activationMode == 2) {
        self->activationCountdown -= deltaSec;
        if (self->activationCountdown <= 0.0f) {
            zEffectAnim::SetTransformRotAndVelocity(self, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                    0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    return self->activationCountdown;
}

// Reimplements 0x458bb0: zEffect::TickResetDelayOnHit
// (D:\Proj\GameZRecoil\zEffect\zeff_anim_init.c)
RECOIL_NOINLINE int RECOIL_FASTCALL TickResetDelayOnHit(zEffectAnimEntry *self,
                                                                 zClass_NodePartial *hitNode,
                                                                 int, float damageAmount) {
    if ((hitNode->listCountA & 0x200) == 0 &&
        (self->activationMode == 0 || self->activationMode == 2)) {
        self->activationCountdown -= damageAmount;
        if (self->activationCountdown <= 0.0f) {
            zEffectAnim::SetTransformRotAndVelocity(self, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                                                    0.0f, 0.0f, 0.0f, 0.0f);
        }
    }

    return 0;
}

// Reimplements 0x458af0: zEffect::SetConditionalRefPos
// (D:\Proj\GameZRecoil\zEffect\zeffect.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetConditionalRefPos(const zVec3 *position) {
    g_zEffect_ConditionalRefPosX = position->x;
    g_zEffect_ConditionalRefPosY = position->y;
    g_zEffect_ConditionalRefPosZ = position->z;
    g_zEffect_ConditionalRefPosEnabled = 1;
}

// Reimplements 0x45e0f0: zEffect::SetConditionalEffectLevel
RECOIL_NOINLINE void RECOIL_FASTCALL SetConditionalEffectLevel(int level) {
    g_zEffect_ConditionalEffectLevel = level;
}

// Reimplements 0x45d000: zEffect::SetAnimDebugFrameTag
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_CDECL SetAnimDebugFrameTag() {
    const int tag = g_zVideo_FrameTick + 1;
    g_zEffect_Anim_DebugFrameTag = tag;
    return tag;
}

// Reimplements 0x45c640: zEffect::GetConditionalRefPosDistanceSq
// (zeff_anim_run.c)
RECOIL_NOINLINE float RECOIL_FASTCALL GetConditionalRefPosDistanceSq(zClass_NodePartial *node) {
    zVec3 worldPosition = {0};
    if (gwNode::GetWorldPosition(node, &worldPosition) != 0) {
        return 0.0f;
    }

    const float dx = worldPosition.x - g_zEffect_ConditionalRefPosX;
    const float dy = worldPosition.y - g_zEffect_ConditionalRefPosY;
    const float dz = worldPosition.z - g_zEffect_ConditionalRefPosZ;
    return dx * dx + dy * dy + dz * dz;
}

// Reimplements 0x45c530: zEffect::TraceUpwardHitFromNodeOrPos
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
TraceUpwardHitFromNodeOrPos(zClass_NodePartial *nodeOrNull, const zVec3 *positionOrNull,
                            const float *rayHeight, int *outHit) {
    zVec3 startPosition = {0};
    if (nodeOrNull != 0) {
        const int result = gwNode::GetWorldPosition(nodeOrNull, &startPosition);
        if (result != 0) {
            return result;
        }
    } else {
        if (positionOrNull == 0) {
            return 1;
        }

        startPosition = *positionOrNull;
    }

    const float height = rayHeight != 0 ? *rayHeight : 50.0f;
    zClass_Class::gwNodeSetRaycastable(nodeOrNull, 0);
    zClass_cls_di::SetStopAfterFirstHit(0x40000);
    zClass_cls_di::SetBreakOnFirstCandidate(1);

    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int result = zClass_cls_di::RaycastFindClosest(
        g_zEffect_World, &rayData, startPosition.x, startPosition.y, startPosition.z,
        startPosition.x, startPosition.y + height, startPosition.z);

    zClass_cls_di::SetBreakOnFirstCandidate(0);
    zClass_Class::gwNodeSetRaycastable(nodeOrNull, 1);

    *outHit = result == 0 && rayData.candidateCount > 0 ? 1 : 0;
    return result;
}

// Reimplements 0x461ec0: zEffect::FindNodeUserDataRecursive
// (GameZRecoil/zEffect/zeff_init.c)
RECOIL_NOINLINE void *RECOIL_FASTCALL FindNodeUserDataRecursive(zClass_NodePartial *node) {
    unsigned int userDataValue = 0;
    zClass_Class::gwNodeGetUserData(node, &userDataValue);
    if (userDataValue != 0) {
        return (void *)(static_cast<unsigned int>(userDataValue));
    }

    for (int i = 0; i < node->listCountB; ++i) {
        void *const result = FindNodeUserDataRecursive(node->listB[i]);
        if (result != 0) {
            return result;
        }
    }

    return 0;
}

// Reimplements 0x461f00: zEffect::SpawnRuntimeInstanceAt
// (GameZRecoil/zEffect/eff_runtime.c)
RECOIL_NOINLINE int RECOIL_FASTCALL SpawnRuntimeInstanceAt(int effectIndex,
                                                                    const zVec3 *worldPos) {
    const int initialized = g_zEffect_RuntimeManager.initialized;
    if (initialized != 0 && effectIndex != -1) {
        zEffect_RuntimeEntry *const entry = AcquireRuntimeEntryByIndex(effectIndex);
        if (entry != 0) {
            ActivateRuntimeEntryAtPosition(entry, worldPos);
            zClass_Class::gwNodeSetActive(entry->effectNode, 1);
            entry->effectNode->callbackContext = (zClass_NodePartial *)(entry);
            zClass_Class::gwNodeSetActionCallback(
                entry->effectNode, (void *)(&RuntimeNodeActionCallback));
        }
    }

    return initialized;
}

// Reimplements 0x461f50: zEffect::ActivateRuntimeEntryAtPosition
// (GameZRecoil/zEffect/eff_runtime.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
ActivateRuntimeEntryAtPosition(zEffect_RuntimeEntry *runtimeEntry, const zVec3 *worldPos) {
    runtimeEntry->elapsedSec = 0.0f;
    runtimeEntry->currentScale = 5.0f;
    runtimeEntry->initialScale = 1.4f;
    runtimeEntry->nearCullDistSq = 56.25f;
    runtimeEntry->farFadeDistSq = 1600.0f;
    runtimeEntry->fadeInTimeSec = 0.3f;
    runtimeEntry->fadeInScaleRate = 6.0f;
    runtimeEntry->baseScale = 6.0f;
    runtimeEntry->lifeTimeSec = 30.0f;
    runtimeEntry->fadeOutScaleRate = 3.75f;
    runtimeEntry->fadeOutStartScale = 3.0f;
    runtimeEntry->fadeOutEndScale = 10.5f;
    runtimeEntry->fadeOutStartTimeSec = 1.0f;

    const float distanceSq = ComputeDistanceSqToListener(worldPos);
    if (distanceSq < runtimeEntry->nearCullDistSq) {
        runtimeEntry->currentScale = 0.0f;
        runtimeEntry->fadeInTimeSec = 0.0f;
        runtimeEntry->fadeOutStartTimeSec = 0.0f;
        return 0;
    }

    if (distanceSq < runtimeEntry->farFadeDistSq) {
        const float scaleFactor = distanceSq / runtimeEntry->farFadeDistSq;
        runtimeEntry->currentScale *= scaleFactor;
        runtimeEntry->fadeInScaleRate *= scaleFactor;
    }

    const float currentScale = runtimeEntry->currentScale;
    zClass_Object3D::gwObject3DSetScale(runtimeEntry->effectNode, currentScale, currentScale,
                                        currentScale);
    zClass_Object3D::gwObject3DSetPosition(runtimeEntry->effectNode, worldPos->x, worldPos->y,
                                           worldPos->z);
    zDi::ResetCurrentVariant(static_cast<zDiPartial *>(runtimeEntry->effectGfxData));
    zClass_Class::AddChild(g_zEffect_RuntimeManager.parentNode, runtimeEntry->effectNode);
    ++g_zEffect_RuntimeManager.activatedCount;
    return 1;
}

// Reimplements 0x462050: zEffect::ComputeDistanceSqToListener
// (GameZRecoil/zEffect/eff_runtime.c)
RECOIL_NOINLINE float RECOIL_FASTCALL ComputeDistanceSqToListener(const zVec3 *worldPos) {
    zVec3 listenerPosition = {0};
    gwNode::GetWorldPosition(g_zEffect_RuntimeManager.listenerNode, &listenerPosition);
    listenerPosition.x -= worldPos->x;
    listenerPosition.y -= worldPos->y;
    listenerPosition.z -= worldPos->z;
    return listenerPosition.x * listenerPosition.x + listenerPosition.y * listenerPosition.y +
           listenerPosition.z * listenerPosition.z;
}

// Reimplements 0x4620d0: zEffect::AcquireRuntimeEntryByIndex
// (GameZRecoil/zEffect/eff_runtime.c)
RECOIL_NOINLINE zEffect_RuntimeEntry *RECOIL_FASTCALL
AcquireRuntimeEntryByIndex(int effectIndex) {
    if (effectIndex == -1) {
        return 0;
    }

    zEffect_RuntimeEntry *const payload =
        static_cast<zEffect_RuntimeEntry *>(zArchiveList_FindPayloadByValue(
            g_zEffect_RuntimeManager.freeList, static_cast<unsigned int>(effectIndex)));
    if (payload != 0) {
        ++g_zEffect_RuntimeManager.recycleCount;
        zArchiveList_RemovePayload(g_zEffect_RuntimeManager.freeList, payload);
        return payload;
    }

    ++g_zEffect_RuntimeManager.freshAllocCount;
    return CloneRuntimeEntryFromTemplate(effectIndex);
}

// Reimplements 0x462130: zEffect::CloneRuntimeEntryFromTemplate
// (GameZRecoil/zEffect/eff_runtime.c)
RECOIL_NOINLINE zEffect_RuntimeEntry *RECOIL_FASTCALL
CloneRuntimeEntryFromTemplate(int effectIndex) {
    if (effectIndex == -1) {
        return 0;
    }

    zEffect_RuntimeEntry *const templateEntry = &g_zEffect_RuntimeManager.templates[effectIndex];
    if (templateEntry->effectNode == 0) {
        return 0;
    }

    zEffect_RuntimeEntry *const clone =
        static_cast<zEffect_RuntimeEntry *>(malloc(sizeof(zEffect_RuntimeEntry)));
    memcpy(clone, templateEntry, sizeof(zEffect_RuntimeEntry));

    zClass_NodePartial *const node = zClass_cls_util::CopyNodeWithCloneOptions(
        clone->effectNode, g_zEffect_CloneCopyMode, g_zEffect_CloneCopyChildrenMode);
    clone->effectNode = node;
    if (node == 0) {
        return 0;
    }

    clone->effectGfxData = FindNodeUserDataRecursive(node);
    return clone->effectGfxData != 0 ? clone : 0;
}

// Reimplements 0x4621b0: zEffect::RuntimeNodeActionCallback
// (GameZRecoil/zEffect/eff_runtime.c)
RECOIL_NOINLINE int RECOIL_FASTCALL RuntimeNodeActionCallback(zClass_NodePartial *node) {
    if ((node->flags & 0x04) == 0) {
        return 0;
    }

    zEffect_RuntimeEntry *const runtimeEntry =
        (zEffect_RuntimeEntry *)(node->callbackContext);
    runtimeEntry->elapsedSec += g_FrameDeltaTimeSec;

    if (runtimeEntry->elapsedSec < runtimeEntry->fadeInTimeSec) {
        runtimeEntry->currentScale += runtimeEntry->fadeInScaleRate * g_FrameDeltaTimeSec;
        const float currentScale = runtimeEntry->currentScale;
        return zClass_Object3D::gwObject3DSetScale(node, currentScale, currentScale, currentScale);
    }

    if (runtimeEntry->elapsedSec < runtimeEntry->fadeOutStartTimeSec) {
        runtimeEntry->currentScale -= runtimeEntry->fadeOutScaleRate * g_FrameDeltaTimeSec;
        if (runtimeEntry->currentScale < 0.01f) {
            runtimeEntry->currentScale = 0.01f;
        }

        const float currentScale = runtimeEntry->currentScale;
        return zClass_Object3D::gwObject3DSetScale(node, currentScale, currentScale, currentScale);
    }

    node->callbackContext = 0;
    zArchiveList_PushBackPayload(g_zEffect_RuntimeManager.freeList, runtimeEntry);
    zClass_Class::gwNodeSetActionCallback(node, 0);
    zClass_Class::gwNodeSetActive(node, 0);
    const int parentCount = node->listCountA;
    if (parentCount != 0) {
        return zClass_Class::RemoveChild(g_zEffect_RuntimeManager.parentNode, node);
    }

    return parentCount;
}

// Reimplements 0x462280: zEffect::FindTemplateIndexByName (GameZRecoil/zEffect/Effect.c)
RECOIL_NOINLINE int RECOIL_FASTCALL FindTemplateIndexByName(const char *name) {
    for (int i = 0; i < g_zEffect_RuntimeManager.templateCount; ++i) {
        if (strcmp(name, g_zEffect_RuntimeManager.templates[i].effectName) == 0) {
            return i;
        }
    }

    return -1;
}

// Reimplements 0x458c10: zEffect::UpdateBeamNodeBetweenPoints
// (zeff_detach.c)
RECOIL_NOINLINE float RECOIL_FASTCALL UpdateBeamNodeBetweenPoints(zClass_NodePartial *obj3d,
                                                                  const zVec3 *srcPos,
                                                                  const zVec3 *destPos) {
    if (obj3d == 0) {
        return 0.0f;
    }

    zClass_Object3D::gwObject3DSetPosition(obj3d, srcPos->x, srcPos->y, srcPos->z);

    zVec3 angles = {0};
    zMath::Vec3DirectionAnglesBetweenPoints(srcPos, destPos, &angles);
    zClass_Object3D::gwObject3DSetRotation(obj3d, angles.x, angles.y, 0.0f);

    zVec3 scale = {0};
    zClass_Object3D::gwObject3DGetScale(obj3d, &scale.x, &scale.y, &scale.z);

    const float dx = destPos->x - srcPos->x;
    const float dy = destPos->y - srcPos->y;
    const float dz = destPos->z - srcPos->z;
    const float length = BeamLengthFromLengthSq(dx * dx + dy * dy + dz * dz);

    zClass_Object3D::gwObject3DSetScale(obj3d, scale.x, scale.y, length);
    return length;
}

// Reimplements 0x458ce0: zEffect::UpdateBeamNodeBetweenFractions
// (zeff_detach.c)
RECOIL_NOINLINE float RECOIL_FASTCALL UpdateBeamNodeBetweenFractions(zClass_NodePartial *obj3d,
                                                                     const zVec3 *srcPos, float t0,
                                                                     const zVec3 *destPos,
                                                                     float t1) {
    if (obj3d == 0) {
        return 0.0f;
    }

    const zVec3 delta = {destPos->x - srcPos->x, destPos->y - srcPos->y, destPos->z - srcPos->z};
    const zVec3 start = {srcPos->x + delta.x * t0, srcPos->y + delta.y * t0, srcPos->z + delta.z * t0};
    const zVec3 end = {srcPos->x + delta.x * t1, srcPos->y + delta.y * t1, srcPos->z + delta.z * t1};

    zClass_Object3D::gwObject3DSetPosition(obj3d, start.x, start.y, start.z);

    zVec3 angles = {0};
    zMath::Vec3DirectionAnglesBetweenPoints(srcPos, destPos, &angles);
    zClass_Object3D::gwObject3DSetRotation(obj3d, angles.x, angles.y, 0.0f);

    zVec3 scale = {0};
    zClass_Object3D::gwObject3DGetScale(obj3d, &scale.x, &scale.y, &scale.z);

    const float dx = end.x - start.x;
    const float dy = end.y - start.y;
    const float dz = end.z - start.z;
    const float length = BeamLengthFromLengthSq(dx * dx + dy * dy + dz * dz);

    zClass_Object3D::gwObject3DSetScale(obj3d, scale.x, scale.y, length);
    return length;
}

// Reimplements 0x458eb0: zEffect::HandleEffectTemplateOffsetEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleEffectTemplateOffsetEvent(zEffectAnimEntry *self, zEffectAnimRefOffsetEvent *event) {
    zVec3 worldPosition = {0};
    zClass_NodePartial *node = 0;

    if (event->nodeRefIndex > 0) {
        node = self->nodeRefList[event->nodeRefIndex].node;
    } else if (event->nodeRefIndex == -200) {
        node = (zClass_NodePartial *)(
            static_cast<unsigned int>(self->resetScratch[0]));
        memcpy(&worldPosition.x, &self->resetScratch[1], sizeof(worldPosition.x));
        memcpy(&worldPosition.y, &self->resetScratch[2], sizeof(worldPosition.y));
        memcpy(&worldPosition.z, &self->resetScratch[3], sizeof(worldPosition.z));
    }

    if (node != 0) {
        gwNode::TransformPoint(node, &worldPosition);
    }

    worldPosition.x += event->offsetX;
    worldPosition.y += event->offsetY;
    worldPosition.z += event->offsetZ;
    SpawnRuntimeInstanceAt(self->effectTemplateRefList[event->refIndex].templateIndex,
                           &worldPosition);
    return 2;
}

// Reimplements 0x458e10: zEffect::HandleSampleRefOffsetEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleSampleRefOffsetEvent(zEffectAnimEntry *self, zEffectAnimRefOffsetEvent *event) {
    zSndSample *const sample = self->sampleRefList[event->refIndex].sample;
    if (event->nodeRefIndex <= 0) {
        sample->PlayA3DSimple(1.0f);
        return 2;
    }

    zVec3 worldPosition = {0};
    gwNode::GetWorldPosition(self->nodeRefList[event->nodeRefIndex].node, &worldPosition);
    worldPosition.x += event->offsetX;
    worldPosition.y += event->offsetY;
    worldPosition.z += event->offsetZ;
    sample->PlayA3D(&worldPosition, 1.0f, 0);
    return 2;
}

// Reimplements 0x458f70: zEffect::HandleSoundEvent (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleSoundEvent(zEffectAnimEntry *self,
                                                              zEffectAnimSoundEvent *event) {
    if (event->soundRefIndex <= 0) {
        event->soundRefIndex = zEffectAnim::FindOrCreateSoundRef(self, event->soundName);
        if (event->soundRefIndex <= 0) {
            return 2;
        }
    }

    zEffectAnimRuntimeNodeRef *const soundRef = &self->soundRefList[event->soundRefIndex];
    zClass_Class::gwNodeSetActive(soundRef->runtimeNode, event->activeState);

    if (event->activeState == 1) {
        if (soundRef->isAttached == 0) {
            zClass_World::AddSound(g_zEffect_World, soundRef->runtimeNode);
            soundRef->isAttached = 1;
        }
    } else if (soundRef->isAttached != 0) {
        zClass_World::RemoveSound(g_zEffect_World, soundRef->runtimeNode);
        soundRef->isAttached = 0;
    }

    if ((event->fieldMask & 0x01) != 0) {
        zClass_Sound::gwSoundSetPosition(soundRef->runtimeNode, event->offsetX, event->offsetY,
                                         event->offsetZ);
    }

    if ((event->fieldMask & 0x02) != 0 && event->parentNodeRefIndex > 0) {
        zVec3 worldPosition = {0};
        gwNode::GetWorldPosition(self->nodeRefList[event->parentNodeRefIndex].node, &worldPosition);
        worldPosition.x += event->offsetX;
        worldPosition.y += event->offsetY;
        worldPosition.z += event->offsetZ;
        zClass_Sound::gwSoundSetPosition(soundRef->runtimeNode, worldPosition.x, worldPosition.y,
                                         worldPosition.z);
    }

    return 2;
}

// Reimplements 0x459080: zEffect::HandleLightEvent (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleLightEvent(zEffectAnimEntry *self,
                                                              zEffectAnimLightEvent *event) {
    if (event->lightRefIndex <= 0) {
        event->lightRefIndex = zEffectAnim::FindOrCreateLightRef(self, event->lightName);
        if (event->lightRefIndex <= 0) {
            return 2;
        }
    }

    zEffectAnimRuntimeNodeRef *const lightRef = &self->lightRefList[event->lightRefIndex];
    zClass_NodePartial *const lightNode = lightRef->runtimeNode;
    zClass_Class::gwNodeSetActive(lightNode, event->activeState);

    if (event->activeState == 1) {
        if (lightRef->isAttached == 0) {
            zClass_World::AddLight(g_zEffect_World, lightNode);
            lightRef->isAttached = 1;
        }
    } else if (lightRef->isAttached != 0) {
        zClass_World::RemoveLight(g_zEffect_World, lightNode);
        lightRef->isAttached = 0;
    }

    if (event->mode == 0) {
        zClass_Light::gwLightSetPointMode(lightNode);
    } else {
        zClass_Light::gwLightSetDirectionalMode(lightNode);
    }

    if ((event->fieldMask & 0x01) != 0) {
        zClass_Light::gwLightSetPosition(lightNode, event->basisOrColorX, event->basisOrColorY,
                                         event->basisOrColorZ);
    }

    if ((event->fieldMask & 0x02) != 0) {
        zVec3 worldPosition = {0};
        zClass_NodePartial *basisNode = 0;

        if (event->basisNodeRefIndex > 0) {
            basisNode = self->nodeRefList[event->basisNodeRefIndex].node;
        } else if (event->basisNodeRefIndex == -200) {
            basisNode = (zClass_NodePartial *)(
                static_cast<unsigned int>(self->resetScratch[0]));
            memcpy(&worldPosition.x, &self->resetScratch[1], sizeof(worldPosition.x));
            memcpy(&worldPosition.y, &self->resetScratch[2], sizeof(worldPosition.y));
            memcpy(&worldPosition.z, &self->resetScratch[3], sizeof(worldPosition.z));
        }

        if (basisNode != 0) {
            gwNode::TransformPoint(basisNode, &worldPosition);
        }

        worldPosition.x += event->basisOrColorX;
        worldPosition.y += event->basisOrColorY;
        worldPosition.z += event->basisOrColorZ;
        zClass_Light::gwLightSetPosition(lightNode, worldPosition.x, worldPosition.y,
                                         worldPosition.z);
    }

    if ((event->fieldMask & 0x04) != 0) {
        zClass_Light::gwLightSetRotation(lightNode, event->positionX, event->positionY,
                                         event->positionZ);
    }

    if ((event->fieldMask & 0x08) != 0) {
        zClass_Light::gwLightSetRange(lightNode, event->rangeInner, event->rangeOuter);
    }

    if ((event->fieldMask & 0x10) != 0) {
        zClass_Light::gwLightSetSpecularColor(lightNode, event->specularR, event->specularG,
                                              event->specularB);
    }

    if ((event->fieldMask & 0x20) != 0) {
        zClass_Light::gwLightSetIntensity(lightNode, event->intensity);
    }

    if ((event->fieldMask & 0x40) != 0) {
        zClass_Light::gwLightSetFalloff(lightNode, event->falloff);
    }

    if ((event->fieldMask & 0x80) != 0) {
        zClass_Light::gwLightSetConeAngle(lightNode, event->coneAngleBits);
    }

    if ((event->fieldMask & 0x100) != 0) {
        zClass_Light::gwLightSetParam(lightNode, event->param);
    }

    return 2;
}

// Reimplements 0x459280: zEffect::HandleLightAnimEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleLightAnimEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                     zEffectLightRangeSpecularAnimEvent *animEvent) {
    if (animEvent->lightRefIndex <= 0) {
        animEvent->lightRefIndex = zEffectAnim::FindOrCreateLightRef(self, animEvent->lightName);
        if (animEvent->lightRefIndex <= 0) {
            return 2;
        }
    }

    zEffectAnimRuntimeNodeRef *const lightRef = &self->lightRefList[animEvent->lightRefIndex];
    if (sequenceRuntime->runState == 0) {
        animEvent->currentRangeInner = animEvent->initialRangeInner;
        animEvent->currentRangeOuter = animEvent->initialRangeOuter;
        animEvent->currentSpecularR = animEvent->initialSpecularR;
        animEvent->currentSpecularG = animEvent->initialSpecularG;
        animEvent->currentSpecularB = animEvent->initialSpecularB;
    }

    float stepSec = g_zEffect_FrameDeltaRemainingSec;
    if (sequenceRuntime->eventElapsedSec > animEvent->durationSec) {
        stepSec = g_zEffect_FrameDeltaRemainingSec -
                  (sequenceRuntime->eventElapsedSec - animEvent->durationSec);
    }

    float lightRangeInner = 0.0f;
    float lightRangeOuter = 0.0f;
    zClass_Light::gwLightGetRange(lightRef->runtimeNode, &lightRangeInner, &lightRangeOuter);

    lightRangeInner += stepSec * animEvent->currentRangeInner;
    lightRangeOuter += stepSec * animEvent->currentRangeOuter;
    if (lightRangeInner < 0.0f) {
        lightRangeInner = 1.0f;
    }
    if (lightRangeOuter < lightRangeInner) {
        lightRangeOuter = lightRangeInner + 1.0f;
    }

    animEvent->currentRangeInner += stepSec * animEvent->rangeInnerDelta;
    animEvent->currentRangeOuter += stepSec * animEvent->rangeOuterDelta;
    zClass_Light::gwLightSetRange(lightRef->runtimeNode, lightRangeInner, lightRangeOuter);

    float specularR = 0.0f;
    float specularG = 0.0f;
    float specularB = 0.0f;
    zClass_Light::gwLightGetSpecularColor(lightRef->runtimeNode, &specularR, &specularG,
                                          &specularB);

    specularR += stepSec * animEvent->currentSpecularR;
    specularG += stepSec * animEvent->currentSpecularG;
    specularB += stepSec * animEvent->currentSpecularB;

    animEvent->currentSpecularR += stepSec * animEvent->specularRDelta;
    animEvent->currentSpecularG += stepSec * animEvent->specularGDelta;
    animEvent->currentSpecularB += stepSec * animEvent->specularBDelta;

    if (specularR > 1.0f) {
        specularR = 1.0f;
    } else if (specularR < 0.0f) {
        specularR = 0.0f;
    }

    if (specularG > 1.0f) {
        specularG = 1.0f;
    } else if (specularG < 0.0f) {
        specularG = 0.0f;
    }

    if (specularB > 1.0f) {
        specularB = 1.0f;
    } else if (specularB < 0.0f) {
        specularB = 0.0f;
    }

    zClass_Light::gwLightSetSpecularColor(lightRef->runtimeNode, specularR, specularG, specularB);

    g_zEffect_FrameDeltaRemainingSec -= stepSec;
    return sequenceRuntime->eventElapsedSec > animEvent->durationSec ? 2 : 1;
}

// Reimplements 0x459510: zEffect::HandleFogEvent (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleFogEvent(zEffectAnimEntry * /*self*/,
                                                            zEffectFogEvent *event) {
    if ((event->flags & 0x01) != 0) {
        zClass_World::SetPendingFogState(g_zEffect_World, event->fogState);
    }

    if ((event->flags & 0x02) != 0) {
        zClass_World::SetPendingFogColorRgb01(g_zEffect_World, event->fogColorR, event->fogColorG,
                                              event->fogColorB);
    }

    if ((event->flags & 0x04) != 0) {
        zClass_World::SetPendingFogAltitudeRange(g_zEffect_World, event->fogAltitudeMin,
                                                 event->fogAltitudeMax);
    }

    if ((event->flags & 0x08) != 0) {
        zClass_World::SetPendingFogRange(g_zEffect_World, event->fogRangeStart, event->fogRangeEnd);
    }

    return 2;
}

// Reimplements 0x459580: zEffect::HandleCameraParamsEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleCameraParamsEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime, zEffectCameraEvent *event) {
    if (self == 0 || sequenceRuntime == 0 || event == 0 ||
        event->targetNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const node = self->nodeRefList[event->targetNodeRefIndex].node;
    float primaryValue = 0.0f;
    float secondaryValue = 0.0f;

    if ((event->flags & 0x01) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetNearFarClip(node, event->nearClip, secondaryValue);
    }

    if ((event->flags & 0x02) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetNearFarClip(node, primaryValue, event->farClip);
    }

    if ((event->flags & 0x04) != 0) {
        zClass_Camera::gwCameraSetClipDistance(node, event->clipDistance);
    }

    if ((event->flags & 0x08) != 0) {
        zClass_Camera::gwCameraGetFOV(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetFOV(node, event->fovPrimary, secondaryValue);
    }

    if ((event->flags & 0x10) != 0) {
        zClass_Camera::gwCameraGetFOV(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetFOV(node, primaryValue, event->fovSecondary);
    }

    if ((event->flags & 0x20) != 0) {
        zClass_Camera::gwCameraGetViewport(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetViewport(node, event->viewportPrimary, secondaryValue);
    }

    if ((event->flags & 0x40) != 0) {
        zClass_Camera::gwCameraGetViewport(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetViewport(node, primaryValue, event->viewportSecondary);
    }

    return 2;
}

// Reimplements 0x4596c0: zEffect::AnimateCameraParamsOverTime
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
AnimateCameraParamsOverTime(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                            zEffectCameraAnimEvent *animEvent) {
    if (self == 0 || sequenceRuntime == 0 || animEvent == 0 ||
        animEvent->targetNodeRefIndex < 0) {
        return 1;
    }

    zClass_NodePartial *const node = self->nodeRefList[animEvent->targetNodeRefIndex].node;
    float primaryValue = 0.0f;
    float secondaryValue = 0.0f;

    if (sequenceRuntime->runState == 0) {
        if ((animEvent->flags & 0x01) != 0) {
            zClass_Camera::gwCameraGetNearFarClip(node, &primaryValue, &secondaryValue);
            zClass_Camera::gwCameraSetNearFarClip(node, animEvent->nearClipStart, secondaryValue);
        }

        if ((animEvent->flags & 0x02) != 0) {
            zClass_Camera::gwCameraGetNearFarClip(node, &primaryValue, &secondaryValue);
            zClass_Camera::gwCameraSetNearFarClip(node, primaryValue, animEvent->farClipStart);
        }

        if ((animEvent->flags & 0x04) != 0) {
            zClass_Camera::gwCameraSetClipDistance(node, animEvent->clipDistanceStart);
        }

        if ((animEvent->flags & 0x08) != 0) {
            zClass_Camera::gwCameraGetFOV(node, &primaryValue, &secondaryValue);
            zClass_Camera::gwCameraSetFOV(node, animEvent->fovPrimaryStart, secondaryValue);
        }

        if ((animEvent->flags & 0x10) != 0) {
            zClass_Camera::gwCameraGetFOV(node, &primaryValue, &secondaryValue);
            zClass_Camera::gwCameraSetFOV(node, primaryValue, animEvent->fovSecondaryStart);
        }

        if ((animEvent->flags & 0x20) != 0) {
            zClass_Camera::gwCameraGetViewport(node, &primaryValue, &secondaryValue);
            zClass_Camera::gwCameraSetViewport(node, animEvent->viewportPrimaryStart,
                                               secondaryValue);
        }

        if ((animEvent->flags & 0x40) != 0) {
            zClass_Camera::gwCameraGetViewport(node, &primaryValue, &secondaryValue);
            zClass_Camera::gwCameraSetViewport(node, primaryValue,
                                               animEvent->viewportSecondaryStart);
        }
    }

    float stepSec = g_zEffect_FrameDeltaRemainingSec;
    if (sequenceRuntime->eventElapsedSec > animEvent->endTime) {
        stepSec = g_zEffect_FrameDeltaRemainingSec -
                  (sequenceRuntime->eventElapsedSec - animEvent->endTime);
    }

    if ((animEvent->flags & 0x01) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetNearFarClip(
            node, primaryValue + animEvent->nearClipRate * stepSec, secondaryValue);
    }

    if ((animEvent->flags & 0x02) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetNearFarClip(node, primaryValue,
                                              secondaryValue + animEvent->farClipRate * stepSec);
    }

    if ((animEvent->flags & 0x04) != 0) {
        zClass_Camera::gwCameraGetClipDistance(node, &primaryValue);
        zClass_Camera::gwCameraSetClipDistance(node, primaryValue +
                                                         animEvent->clipDistanceRate * stepSec);
    }

    if ((animEvent->flags & 0x08) != 0) {
        zClass_Camera::gwCameraGetFOV(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetFOV(node, primaryValue + animEvent->fovPrimaryRate * stepSec,
                                      secondaryValue);
    }

    if ((animEvent->flags & 0x10) != 0) {
        zClass_Camera::gwCameraGetFOV(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetFOV(node, primaryValue,
                                      secondaryValue + animEvent->fovSecondaryRate * stepSec);
    }

    if ((animEvent->flags & 0x20) != 0) {
        zClass_Camera::gwCameraGetViewport(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetViewport(
            node, primaryValue + animEvent->viewportPrimaryRate * stepSec, secondaryValue);
    }

    if ((animEvent->flags & 0x40) != 0) {
        zClass_Camera::gwCameraGetViewport(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetViewport(
            node, primaryValue, secondaryValue + animEvent->viewportSecondaryRate * stepSec);
    }

    g_zEffect_FrameDeltaRemainingSec -= stepSec;
    if (sequenceRuntime->eventElapsedSec <= animEvent->endTime) {
        return 1;
    }

    if ((animEvent->flags & 0x01) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetNearFarClip(node, animEvent->nearClipEnd, secondaryValue);
    }

    if ((animEvent->flags & 0x02) != 0) {
        zClass_Camera::gwCameraGetNearFarClip(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetNearFarClip(node, primaryValue, animEvent->farClipEnd);
    }

    if ((animEvent->flags & 0x04) != 0) {
        zClass_Camera::gwCameraSetClipDistance(node, animEvent->clipDistanceEnd);
    }

    if ((animEvent->flags & 0x08) != 0) {
        zClass_Camera::gwCameraGetFOV(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetFOV(node, animEvent->fovPrimaryEnd, secondaryValue);
    }

    if ((animEvent->flags & 0x10) != 0) {
        zClass_Camera::gwCameraGetFOV(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetFOV(node, primaryValue, animEvent->fovSecondaryEnd);
    }

    if ((animEvent->flags & 0x20) != 0) {
        zClass_Camera::gwCameraGetViewport(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetViewport(node, animEvent->viewportPrimaryEnd, secondaryValue);
    }

    if ((animEvent->flags & 0x40) != 0) {
        zClass_Camera::gwCameraGetViewport(node, &primaryValue, &secondaryValue);
        zClass_Camera::gwCameraSetViewport(node, primaryValue, animEvent->viewportSecondaryEnd);
    }

    return 2;
}

// Reimplements 0x45a920: zEffect::FindNearestPickCandidateBelowPoint
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
FindNearestPickCandidateBelowPoint(const zVec3 *point, zClassDiPickCandidateEntry *outCandidate) {
    PlayerProbeSampleCandidateBuffer outResults = {0};
    zClass_cls_di::BuildPickCandidateListBelowPoint(g_zEffect_World, &outResults, point->x,
                                                    point->y, point->z);

    int bestIndex = -1;
    float closestDistance = 0.0f;
    for (int i = 0; i < outResults.candidateCount; ++i) {
        zClassDiPickCandidateEntry *candidate = &outResults.entries[i];
        const float distance = fabs(point->y - candidate->hitPos.y);
        if (bestIndex < 0) {
            closestDistance = distance;
            bestIndex = i;
        } else if (distance < closestDistance && candidate->hitPos.y - point->y < 10.0f) {
            closestDistance = distance;
            bestIndex = i;
        }
    }

    if (bestIndex < 0) {
        return 0;
    }

    *outCandidate = outResults.entries[bestIndex];
    return 1;
}

// Reimplements 0x459e70: zEffect::HandleNodeAnimEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleNodeAnimEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                    zEffectNodeAnimEvent *animEvent) {
    if (self == 0 || sequenceRuntime == 0 || animEvent == 0 ||
        animEvent->targetNodeRefIndex < 0) {
        return 1;
    }

    zClass_NodePartial *node = self->nodeRefList[animEvent->targetNodeRefIndex].node;
    int result = 1;

    if (sequenceRuntime->runState == 0) {
        if ((animEvent->flags & 0x0400) == 0) {
            animEvent->runtimeElapsedSec = 0.0f;
        }
        if ((animEvent->flags & 0x0100) != 0) {
            animEvent->runtimeVecE = animEvent->runtimeVecC;
        }
        if ((animEvent->flags & 0x20) != 0) {
            animEvent->runtimeVecB.x = animEvent->scaleRate.z;
            animEvent->runtimeVecB.y = animEvent->endTimeSec;
            memcpy(&animEvent->runtimeVecB.z, animEvent->unknown_90,
                        sizeof(animEvent->runtimeVecB.z));
        }
        if ((animEvent->flags & 0x04) != 0) {
            animEvent->rotationOrCameraPosEnd.z = animEvent->positionOrTargetRate.z;
            animEvent->rotationOrCameraPosRate = animEvent->rotationOrCameraPosStart;
            animEvent->scaleStart.x = animEvent->rotationOrCameraPosEnd.x;
            animEvent->scaleStart.y = animEvent->rotationOrCameraPosEnd.y;
        }
        if ((animEvent->flags & 0x08) != 0) {
            const float randAngle =
                (animEvent->positionOrTargetStart.y - animEvent->positionOrTargetStart.x) *
                    NextEffectRandUnit() +
                animEvent->positionOrTargetStart.x;
            const float yaw =
                (animEvent->positionOrTargetEnd.x - animEvent->positionOrTargetStart.z) *
                    NextEffectRandUnit() +
                animEvent->positionOrTargetStart.z;
            const float launchMagnitude =
                (animEvent->positionOrTargetEnd.z - animEvent->positionOrTargetEnd.y) *
                    NextEffectRandUnit() +
                animEvent->positionOrTargetEnd.y;
            const float spinMagnitude =
                (animEvent->positionOrTargetRate.y - animEvent->positionOrTargetRate.x) *
                    NextEffectRandUnit() +
                animEvent->positionOrTargetRate.x;

            zVec3 dir = {0};
            zMath_Vec3_DirFromYaw(&dir, spinMagnitude * 0.01745329252f);
            const float vertical = yaw * 0.0111111114f;
            const float horizontal = vertical < 0.0f ? vertical + 1.0f : 1.0f - vertical;
            const float vx = horizontal * dir.x;
            const float vz = horizontal * dir.z;

            animEvent->scaleEnd.x = vertical;
            animEvent->scaleStart.z = vx;
            animEvent->scaleEnd.y = vz;
            animEvent->positionOrTargetRate.z = launchMagnitude * vx;
            animEvent->rotationOrCameraPosStart.x = launchMagnitude * vertical;
            animEvent->rotationOrCameraPosStart.y = launchMagnitude * vz;
            animEvent->rotationOrCameraPosStart.z = randAngle * vx;
            animEvent->rotationOrCameraPosEnd.x = randAngle * vertical;
            animEvent->rotationOrCameraPosEnd.y = randAngle * vz;
            animEvent->rotationOrCameraPosEnd.z = animEvent->positionOrTargetRate.z;
            animEvent->rotationOrCameraPosRate = animEvent->rotationOrCameraPosStart;
            animEvent->scaleStart.x = animEvent->rotationOrCameraPosEnd.x;
            animEvent->scaleStart.y = animEvent->rotationOrCameraPosEnd.y;
        }
        if ((animEvent->flags & 0xc0) != 0) {
            const float scaleEndZ = animEvent->scaleEnd.z;
            const float scaleRateX = animEvent->scaleRate.x;
            animEvent->scaleRate.y = scaleEndZ;
            animEvent->scaleRate.z = scaleRateX;
            animEvent->endTimeSec = animEvent->scaleRate.y;
        }

        bool transformBasis = false;
        if ((animEvent->flags & 0x02) != 0 && (self->flags & 0x80) != 0) {
            transformBasis = true;
        }
        if ((animEvent->flags & 0x01) != 0) {
            transformBasis = true;
            if ((animEvent->flags & 0x2000) != 0) {
                animEvent->scaleStart.x += animEvent->nodeAlphaEnd;
            }
        }

        if (transformBasis) {
            if ((animEvent->flags & 0x02) != 0 && (self->flags & 0x80) != 0) {
                const zVec3 velocity = {self->velocityX, self->velocityY, self->velocityZ};
                const zVec3 out = TransformNodeBasisVector(node, &velocity);
                animEvent->rotationOrCameraPosEnd.z += out.x;
                animEvent->rotationOrCameraPosRate.x += out.y;
                animEvent->rotationOrCameraPosRate.y += out.z;
            }

            if ((animEvent->flags & 0x01) != 0) {
                const zVec3 offset = {0.0f, animEvent->nodeAlphaEnd, 0.0f};
                const zVec3 out = TransformNodeBasisVector(node, &offset);
                animEvent->rotationOrCameraPosRate.z += out.x;
                animEvent->scaleStart.x += out.y;
                animEvent->scaleStart.y += out.z;
            }
        }
    }

    float frameStepSec = g_zEffect_FrameDeltaRemainingSec;
    if ((animEvent->flags & 0x0400) != 0 &&
        sequenceRuntime->eventElapsedSec > animEvent->runtimeElapsedSec) {
        frameStepSec -= sequenceRuntime->eventElapsedSec - animEvent->runtimeElapsedSec;
    }

    zVec3 worldPos = {0};
    if ((animEvent->flags & 0x0c) != 0) {
        float dx = animEvent->rotationOrCameraPosEnd.z * frameStepSec;
        float dy = animEvent->rotationOrCameraPosRate.x * frameStepSec;
        float dz = animEvent->rotationOrCameraPosRate.y * frameStepSec;
        int movementClamped = 0;

        if ((animEvent->flags & 0x01) != 0) {
            gwNode::GetWorldPosition(node, &worldPos);
            zClass_Class::gwNodeSetCellPickable(self->boundNode, 0);
            zClassDiPickCandidateEntry candidate = {0};
            const int found = FindNearestPickCandidateBelowPoint(&worldPos, &candidate);
            zClass_Class::gwNodeSetCellPickable(self->boundNode, 1);

            if (dy < 0.0f && found != 0 && worldPos.y + dy < candidate.hitPos.y) {
                movementClamped = 1;
                if (fabs(animEvent->rotationOrCameraPosEnd.z) < 0.100000001f &&
                    fabs(animEvent->rotationOrCameraPosRate.y) < 0.100000001f) {
                    dy = candidate.hitPos.y - worldPos.y;
                } else {
                    dy = fabs(dy * 0.5f) + candidate.hitPos.y - worldPos.y;
                }
            } else if (found == 0 && (animEvent->flags & 0x0400) == 0) {
                animEvent->runtimeElapsedSec += frameStepSec;
                if (animEvent->runtimeElapsedSec > 15.0f) {
                    result = 2;
                    movementClamped = 1;
                }
            }
        }

        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslateTarget(node, dx, dy, dz);
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslatePosition(node, dx, dy, dz);
        }

        if (movementClamped != 0) {
            const float velocitySq =
                animEvent->rotationOrCameraPosEnd.z * animEvent->rotationOrCameraPosEnd.z +
                animEvent->rotationOrCameraPosRate.x * animEvent->rotationOrCameraPosRate.x +
                animEvent->rotationOrCameraPosRate.y * animEvent->rotationOrCameraPosRate.y;
            const float accelSq =
                animEvent->rotationOrCameraPosRate.z * animEvent->rotationOrCameraPosRate.z +
                animEvent->scaleStart.x * animEvent->scaleStart.x +
                animEvent->scaleStart.y * animEvent->scaleStart.y;
            if (velocitySq < accelSq) {
                result = 2;
            } else {
                animEvent->rotationOrCameraPosEnd.z *= 0.199999988f;
                animEvent->rotationOrCameraPosRate.y *= 0.199999988f;
                animEvent->rotationOrCameraPosRate.x *= 0.199999988f;
            }
        }

        if (movementClamped != 0 && (animEvent->flags & 0x0800) != 0) {
            const short runtimeIndex = ResolveNodeAnimRuntimeIndex(self, animEvent);
            if (runtimeIndex >= 0 && self->runtimeList[runtimeIndex].runState == 3) {
                self->runtimeList[runtimeIndex].runState = 0;
            }
        }

        if ((animEvent->flags & 0x1000) != 0 && animEvent->sampleRefIndex > 0) {
            const float speed = sqrt(
                animEvent->rotationOrCameraPosEnd.z * animEvent->rotationOrCameraPosEnd.z +
                animEvent->rotationOrCameraPosRate.x * animEvent->rotationOrCameraPosRate.x +
                animEvent->rotationOrCameraPosRate.y * animEvent->rotationOrCameraPosRate.y);
            const float threshold = animEvent->lookupScale < 0.0f ? animEvent->nodeAlphaEnd * 10.0f
                                                                  : animEvent->lookupScale;
            const float gain = speed >= threshold ? 1.0f : speed / threshold;
            zSndSample *const sample = self->sampleRefList[animEvent->sampleRefIndex].sample;
            sample->PlayA3D(&worldPos, gain, 0);
        }

        animEvent->rotationOrCameraPosEnd.z += animEvent->rotationOrCameraPosRate.z * frameStepSec;
        animEvent->rotationOrCameraPosRate.x += animEvent->scaleStart.x * frameStepSec;
        animEvent->rotationOrCameraPosRate.y += animEvent->scaleStart.y * frameStepSec;
    }

    if ((animEvent->flags & 0x20) != 0) {
        const float dx = animEvent->runtimeVecB.x * frameStepSec;
        const float dy = animEvent->runtimeVecB.y * frameStepSec;
        const float dz = animEvent->runtimeVecB.z * frameStepSec;
        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslate(node, dx, dy, dz);
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslateRotation(node, dx, dy, dz);
        }
        animEvent->runtimeVecB.x += animEvent->runtimeVecA.x * frameStepSec;
        animEvent->runtimeVecB.y += animEvent->runtimeVecA.y * frameStepSec;
        animEvent->runtimeVecB.z += animEvent->runtimeVecA.z * frameStepSec;
    }

    if ((animEvent->flags & 0x40) != 0) {
        zClass_Object3D::gwObject3DTranslateRotation(
            node, animEvent->rotationOrCameraPosRate.y * frameStepSec * animEvent->scaleRate.y,
            0.0f, -animEvent->rotationOrCameraPosEnd.z * frameStepSec * animEvent->scaleRate.y);
    }

    if ((animEvent->flags & 0x80) != 0) {
        const float dx = animEvent->scaleEnd.y * frameStepSec * animEvent->scaleRate.y;
        const float dz = -animEvent->scaleStart.z * frameStepSec * animEvent->scaleRate.y;
        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslate(node, dx, 0.0f, dz);
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslateRotation(node, dx, 0.0f, dz);
        }
        animEvent->scaleRate.y += animEvent->scaleRate.x * frameStepSec;
    }

    if ((animEvent->flags & 0x0100) != 0) {
        zVec3 scale = {0};
        zClass_Object3D::gwObject3DGetScale(node, &scale.x, &scale.y, &scale.z);
        scale.x += animEvent->runtimeVecE.x * frameStepSec;
        scale.y += animEvent->runtimeVecE.y * frameStepSec;
        scale.z += animEvent->runtimeVecE.z * frameStepSec;
        if (scale.x < 0.001f) {
            scale.x = 0.001f;
        }
        if (scale.y < 0.001f) {
            scale.y = 0.001f;
        }
        if (scale.z < 0.001f) {
            scale.z = 0.001f;
        }
        animEvent->runtimeVecD.x += animEvent->runtimeVecD.x * frameStepSec;
        animEvent->runtimeVecD.y += animEvent->runtimeVecD.y * frameStepSec;
        animEvent->runtimeVecD.z += animEvent->runtimeVecD.z * frameStepSec;
        zClass_Object3D::gwObject3DSetScale(node, scale.x, scale.y, scale.z);
    }

    if ((animEvent->flags & 0x0200) != 0) {
        AddNodeDiBlendScale(node, animEvent->nodeAlphaRate * frameStepSec);
    }

    g_zEffect_FrameDeltaRemainingSec -= frameStepSec;
    if ((animEvent->flags & 0x0400) != 0 &&
        sequenceRuntime->eventElapsedSec > animEvent->runtimeElapsedSec) {
        return 2;
    }

    return result;
}

// Reimplements 0x45a9d0: zEffect::AnimateNodeOverTime
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
AnimateNodeOverTime(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                    zEffectNodeAnimEvent *nodeAnimEvent) {
    if (self == 0 || sequenceRuntime == 0 || nodeAnimEvent == 0 ||
        nodeAnimEvent->targetNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const node = self->nodeRefList[nodeAnimEvent->targetNodeRefIndex].node;
    if (sequenceRuntime->runState == 0) {
        if ((nodeAnimEvent->flags & 0x04) != 0) {
            zClass_Object3D::gwObject3DSetScale(node, nodeAnimEvent->scaleStart.x,
                                                nodeAnimEvent->scaleStart.y,
                                                nodeAnimEvent->scaleStart.z);
        }
        if ((nodeAnimEvent->flags & 0x02) != 0) {
            if (node->classId == 1) {
                zClass_Camera::gwCameraSetPosition(node, nodeAnimEvent->rotationOrCameraPosStart.x,
                                                   nodeAnimEvent->rotationOrCameraPosStart.y,
                                                   nodeAnimEvent->rotationOrCameraPosStart.z);
            } else if (node->classId == 5) {
                zClass_Object3D::gwObject3DSetRotation(node,
                                                       nodeAnimEvent->rotationOrCameraPosStart.x,
                                                       nodeAnimEvent->rotationOrCameraPosStart.y,
                                                       nodeAnimEvent->rotationOrCameraPosStart.z);
            }
        }
        if ((nodeAnimEvent->flags & 0x01) != 0) {
            if (node->classId == 1) {
                zClass_Camera::gwCameraSetTarget(node, nodeAnimEvent->positionOrTargetStart.x,
                                                 nodeAnimEvent->positionOrTargetStart.y,
                                                 nodeAnimEvent->positionOrTargetStart.z);
            } else if (node->classId == 5) {
                zClass_Object3D::gwObject3DSetPosition(node, nodeAnimEvent->positionOrTargetStart.x,
                                                       nodeAnimEvent->positionOrTargetStart.y,
                                                       nodeAnimEvent->positionOrTargetStart.z);
            }
        }
        if ((nodeAnimEvent->flags & 0x08) != 0) {
            SetNodeDiBlendScale(node, nodeAnimEvent->nodeAlphaStart);
        }
    }

    const float deltaTimeSec =
        sequenceRuntime->eventElapsedSec <= nodeAnimEvent->endTimeSec
            ? g_zEffect_FrameDeltaRemainingSec
            : g_zEffect_FrameDeltaRemainingSec -
                  (sequenceRuntime->eventElapsedSec - nodeAnimEvent->endTimeSec);

    if ((nodeAnimEvent->flags & 0x01) != 0) {
        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslateTarget(
                node, nodeAnimEvent->positionOrTargetRate.x * deltaTimeSec,
                nodeAnimEvent->positionOrTargetRate.y * deltaTimeSec,
                nodeAnimEvent->positionOrTargetRate.z * deltaTimeSec);
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslatePosition(
                node, nodeAnimEvent->positionOrTargetRate.x * deltaTimeSec,
                nodeAnimEvent->positionOrTargetRate.y * deltaTimeSec,
                nodeAnimEvent->positionOrTargetRate.z * deltaTimeSec);
        }
    }

    if ((nodeAnimEvent->flags & 0x02) != 0) {
        if (node->classId == 1) {
            zClass_Camera::gwCameraTranslate(
                node, nodeAnimEvent->rotationOrCameraPosRate.x * deltaTimeSec,
                nodeAnimEvent->rotationOrCameraPosRate.y * deltaTimeSec,
                nodeAnimEvent->rotationOrCameraPosRate.z * deltaTimeSec);
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DTranslateRotation(
                node, nodeAnimEvent->rotationOrCameraPosRate.x * deltaTimeSec,
                nodeAnimEvent->rotationOrCameraPosRate.y * deltaTimeSec,
                nodeAnimEvent->rotationOrCameraPosRate.z * deltaTimeSec);
        }
    }

    if ((nodeAnimEvent->flags & 0x04) != 0) {
        zVec3 scale = {0};
        zClass_Object3D::gwObject3DGetScale(node, &scale.x, &scale.y, &scale.z);
        scale.x += nodeAnimEvent->scaleRate.x * deltaTimeSec;
        scale.y += nodeAnimEvent->scaleRate.y * deltaTimeSec;
        scale.z += nodeAnimEvent->scaleRate.z * deltaTimeSec;
        if (scale.x < 0.001f) {
            scale.x = 0.001f;
        }
        if (scale.y < 0.001f) {
            scale.y = 0.001f;
        }
        if (scale.z < 0.001f) {
            scale.z = 0.001f;
        }
        zClass_Object3D::gwObject3DSetScale(node, scale.x, scale.y, scale.z);
    }

    if ((nodeAnimEvent->flags & 0x08) != 0) {
        AddNodeDiBlendScale(node, nodeAnimEvent->nodeAlphaRate * deltaTimeSec);
    }

    g_zEffect_FrameDeltaRemainingSec -= deltaTimeSec;
    if (sequenceRuntime->eventElapsedSec <= nodeAnimEvent->endTimeSec) {
        return 1;
    }

    if ((nodeAnimEvent->flags & 0x04) != 0) {
        zClass_Object3D::gwObject3DSetScale(node, nodeAnimEvent->scaleEnd.x,
                                            nodeAnimEvent->scaleEnd.y, nodeAnimEvent->scaleEnd.z);
    }
    if ((nodeAnimEvent->flags & 0x02) != 0) {
        zClass_Object3D::gwObject3DSetRotation(node, nodeAnimEvent->rotationOrCameraPosEnd.x,
                                               nodeAnimEvent->rotationOrCameraPosEnd.y,
                                               nodeAnimEvent->rotationOrCameraPosEnd.z);
    }
    if ((nodeAnimEvent->flags & 0x01) != 0) {
        if (node->classId == 1) {
            zClass_Camera::gwCameraSetTarget(node, nodeAnimEvent->positionOrTargetEnd.x,
                                             nodeAnimEvent->positionOrTargetEnd.y,
                                             nodeAnimEvent->positionOrTargetEnd.z);
        } else if (node->classId == 5) {
            zClass_Object3D::gwObject3DSetPosition(node, nodeAnimEvent->positionOrTargetEnd.x,
                                                   nodeAnimEvent->positionOrTargetEnd.y,
                                                   nodeAnimEvent->positionOrTargetEnd.z);
        }
    }
    if ((nodeAnimEvent->flags & 0x08) != 0) {
        SetNodeDiBlendScale(node, nodeAnimEvent->nodeAlphaEnd);
    }

    return 2;
}

// Reimplements 0x459e30: zEffect::HandleActivateEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleActivateEvent(zEffectAnimEntry *self,
                                                                 zEffectActivateEvent *event) {
    const short targetIndex = event->targetNodeRefIndex;
    if (targetIndex >= 0) {
        zClass_Class::gwNodeSetActive(self->nodeRefList[targetIndex].node, event->activeValue);
    } else if (targetIndex == kEffectAnimBoundNodeRefIndex) {
        zClass_Class::gwNodeSetActive(self->boundNode, event->activeValue);
    }

    return 2;
}

// Reimplements 0x459ce0: zEffect::HandlePositionEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandlePositionEvent(zEffectAnimEntry *self,
                                                                 zEffectTransformEvent *event) {
    zVec3 point = {0};
    zClass_NodePartial *basisNode = 0;

    if (event->basisNodeRefIndex > 0) {
        basisNode = self->nodeRefList[event->basisNodeRefIndex].node;
    } else if (event->basisNodeRefIndex == kEffectAnimResetScratchRefIndex) {
        basisNode = (zClass_NodePartial *)(
            static_cast<unsigned int>(self->resetScratch[0]));
        memcpy(&point.x, &self->resetScratch[1], sizeof(point.x));
        memcpy(&point.y, &self->resetScratch[2], sizeof(point.y));
        memcpy(&point.z, &self->resetScratch[3], sizeof(point.z));
    }

    if (basisNode != 0) {
        gwNode::TransformPoint(basisNode, &point);
    }

    point.x += event->vecX;
    point.y += event->vecY;
    point.z += event->vecZ;

    zClass_NodePartial *const targetNode = self->nodeRefList[event->targetNodeRefIndex].node;
    if (targetNode != 0) {
        if (targetNode->classId == 1) {
            if ((event->flags & 0x01) != 0) {
                zClass_Camera::gwCameraTranslateTarget(targetNode, point.x, point.y, point.z);
            } else {
                zClass_Camera::gwCameraSetTarget(targetNode, point.x, point.y, point.z);
            }
        } else if (targetNode->classId == 5) {
            if ((event->flags & 0x01) != 0) {
                zClass_Object3D::gwObject3DTranslatePosition(targetNode, point.x, point.y, point.z);
            } else {
                zClass_Object3D::gwObject3DSetPosition(targetNode, point.x, point.y, point.z);
            }
        }
    }

    return 2;
}

// Reimplements 0x459cb0: zEffect::HandleNodeScaleEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleNodeScaleEvent(zEffectAnimEntry *self,
                                                                  zEffectNodeScaleEvent *event) {
    zClass_Object3D::gwObject3DSetScale(self->nodeRefList[event->targetNodeRefIndex].node,
                                        event->scaleX, event->scaleY, event->scaleZ);
    return 2;
}

// Reimplements 0x459ae0: zEffect::HandleRotationEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleRotationEvent(zEffectAnimEntry *self,
                                                                 zEffectTransformEvent *event) {
    zClass_NodePartial *const targetNode = self->nodeRefList[event->targetNodeRefIndex].node;
    if (targetNode->classId == 1) {
        if ((event->flags & 0x01) != 0) {
            zClass_Camera::gwCameraTranslate(targetNode, event->vecX, event->vecY, event->vecZ);
        } else {
            zClass_Camera::gwCameraSetPosition(targetNode, event->vecX, event->vecY, event->vecZ);
        }
    } else if (targetNode->classId == 5) {
        if ((event->flags & 0x02) != 0 || (event->flags & 0x04) != 0) {
            zClass_NodePartial *basisNode = 0;
            zVec3 basisAngles = {0};

            if (event->basisNodeRefIndex > 0) {
                basisNode = self->nodeRefList[event->basisNodeRefIndex].node;
            } else if (event->basisNodeRefIndex == kEffectAnimResetScratchRefIndex) {
                basisNode = (zClass_NodePartial *)(
                    static_cast<unsigned int>(self->resetScratch[0]));
            }

            if (basisNode != 0) {
                if ((event->flags & 0x02) != 0) {
                    zClass_Object3D::gwObject3DGetRotation(basisNode, &basisAngles.x,
                                                           &basisAngles.y, &basisAngles.z);
                } else {
                    zMat4x3 matrix;
                    zMath::MatStackPushPtr((float *)(&matrix));
                    zMath::MatLoadIdentity();
                    gwNode::BuildNodeToAncestorMatrix(basisNode, 1);
                    zMath_Mat_ExtractEulerAngles(&matrix, &basisAngles);
                    zMath::MatStackPopPtr();
                }
            }

            zClass_Object3D::gwObject3DSetRotation(targetNode, event->vecX + basisAngles.x,
                                                   event->vecY + basisAngles.y,
                                                   event->vecZ + basisAngles.z);
        } else if ((event->flags & 0x01) != 0) {
            zClass_Object3D::gwObject3DTranslateRotation(targetNode, event->vecX, event->vecY,
                                                         event->vecZ);
        } else {
            zClass_Object3D::gwObject3DSetRotation(targetNode, event->vecX, event->vecY,
                                                   event->vecZ);
        }
    }

    return 2;
}

// Reimplements 0x45b3b0: zEffect::HandleAddChildEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleAddChildEvent(zEffectAnimEntry *self,
                                                                 zEffectParentChildEvent *event) {
    if (event->parentNodeRefIndex > 0 && event->childNodeRefIndex > 0) {
        zClass_NodePartial *const parentNode = self->nodeRefList[event->parentNodeRefIndex].node;
        zClass_NodePartial *const childNode = self->nodeRefList[event->childNodeRefIndex].node;

        for (int i = 0; i < parentNode->listCountB; ++i) {
            if (parentNode->listB[i] == childNode) {
                return 2;
            }
        }

        zClass_Class::AddChild(parentNode, childNode);
    }

    return 2;
}

// Reimplements 0x45b410: zEffect::HandleRemoveChildEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleRemoveChildEvent(zEffectAnimEntry *self, zEffectParentChildEvent *event) {
    zEffectAnimNodeRef28 *const nodeRefList = self->nodeRefList;
    zClass_Class::RemoveChild(nodeRefList[event->parentNodeRefIndex].node,
                              nodeRefList[event->childNodeRefIndex].node);
    return 2;
}

// Reimplements 0x45b440: zEffect::HandleAttachEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleAttachEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime, zEffectAttachEvent *event) {
    if (self != 0 && sequenceRuntime != 0 && event != 0) {
        if (event->targetNodeRefIndex >= 0) {
            zClass_NodePartial *const targetNode =
                self->nodeRefList[event->targetNodeRefIndex].node;
            if ((event->flags & 0x01) != 0) {
                zDiPartial *const targetDi =
                    (zDiPartial *)(targetNode->userDataOrDiRef);
                if (targetDi != 0) {
                    zDi::ResetCurrentVariant(targetDi);
                }
            }

            zDi::SetCurrentVariant((zDiPartial *)(targetNode->userDataOrDiRef),
                                   event->variantIndex);
        }
    }

    return 2;
}

// Reimplements 0x45b4a0: zEffect::HandleDetachEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleDetachEvent(zEffectAnimEntry *self,
                                                               zEffectAnimSurfaceRuntime *runtime,
                                                               zEffectBeamDetachEvent *event) {
    if (self == 0 || runtime == 0 || event == 0 || event->beamNodeRefIndex < 0) {
        return 2;
    }

    zClass_NodePartial *const beamNode = self->nodeRefList[event->beamNodeRefIndex].node;

    if (runtime->runState == 0) {
        unsigned int flags = static_cast<unsigned int>(event->flags);
        if ((flags & 0x0800u) != 0) {
            event->segmentStartCurrent = event->segmentStartInitial;
        }
        if ((flags & 0x2000u) != 0) {
            event->segmentEndCurrent = event->segmentEndInitial;
        }

        if ((flags & 0x04u) != 0) {
            flags &= ~0x04u;
            event->flags = static_cast<int>(flags);

            zVec3 point = {0};
            if ((flags & 0x08u) != 0) {
                flags &= ~0x08u;
                event->flags = static_cast<int>(flags);
                point = event->pointA;
            } else if ((flags & 0x10u) != 0) {
                flags &= ~0x10u;
                event->flags = static_cast<int>(flags);
                point = LoadResetScratchVec(self, 1);
            }

            zClass_NodePartial *const refNode = LoadResetScratchNode(self, 0);
            if (refNode != 0) {
                gwNode::TransformPoint(refNode, &point);
            }
            event->pointA = point;
            flags = static_cast<unsigned int>(event->flags) | 0x08u;
            event->flags = static_cast<int>(flags);
        }

        flags = static_cast<unsigned int>(event->flags);
        if ((flags & 0x80u) != 0) {
            flags &= ~0x80u;
            event->flags = static_cast<int>(flags);

            zVec3 point = {0};
            if ((flags & 0x0100u) != 0) {
                flags &= ~0x0100u;
                event->flags = static_cast<int>(flags);
                point = event->pointB;
            } else if ((flags & 0x0200u) != 0) {
                flags &= ~0x0200u;
                event->flags = static_cast<int>(flags);
                point = LoadResetScratchVec(self, 5);
            }

            zClass_NodePartial *const refNode = LoadResetScratchNode(self, 4);
            if (refNode != 0) {
                gwNode::TransformPoint(refNode, &point);
            }
            event->pointB = point;
            flags = static_cast<unsigned int>(event->flags) | 0x0100u;
            event->flags = static_cast<int>(flags);
        }
    }

    const float timeSlice =
        runtime->eventElapsedSec <= event->endTimeSec
            ? g_zEffect_FrameDeltaRemainingSec
            : g_zEffect_FrameDeltaRemainingSec - (runtime->eventElapsedSec - event->endTimeSec);

    const unsigned int flags = static_cast<unsigned int>(event->flags);

    zClass_NodePartial *pointANode = 0;
    if ((flags & 0x01u) != 0) {
        if (event->pointANodeRefIndex >= 0) {
            pointANode = self->nodeRefList[event->pointANodeRefIndex].node;
        }
    } else if ((flags & 0x02u) != 0) {
        pointANode = LoadResetScratchNode(self, 0);
    }

    zVec3 pointA = {0};
    if ((flags & 0x08u) != 0) {
        pointA = event->pointA;
    } else if ((flags & 0x10u) != 0) {
        pointA = LoadResetScratchVec(self, 1);
    }
    if (pointANode != 0) {
        gwNode::TransformPoint(pointANode, &pointA);
    }

    zClass_NodePartial *pointBNode = 0;
    if ((flags & 0x20u) != 0) {
        if (event->pointBNodeRefIndex >= 0) {
            pointBNode = self->nodeRefList[event->pointBNodeRefIndex].node;
        }
    } else if ((flags & 0x40u) != 0) {
        pointBNode = LoadResetScratchNode(self, 4);
    }

    zVec3 pointB = {0};
    if ((flags & 0x0100u) != 0) {
        pointB = event->pointB;
    } else if ((flags & 0x0200u) != 0) {
        pointB = LoadResetScratchVec(self, 5);
    }
    if (pointBNode != 0) {
        gwNode::TransformPoint(pointBNode, &pointB);
    }

    int fractionChanged = 0;
    if ((flags & 0x0400u) != 0) {
        fractionChanged = 1;
        event->segmentStartCurrent = event->segmentStartInitial;
    } else if ((flags & 0x0800u) != 0) {
        fractionChanged = 1;
        event->segmentStartCurrent += event->segmentStartRate * timeSlice;
    } else {
        event->segmentStartCurrent = 0.0f;
    }

    if ((flags & 0x1000u) != 0) {
        fractionChanged = 1;
        event->segmentEndCurrent = event->segmentEndInitial;
    } else if ((flags & 0x2000u) != 0) {
        fractionChanged = 1;
        event->segmentEndCurrent += event->segmentEndRate * timeSlice;
    } else {
        event->segmentEndCurrent = 1.0f;
    }

    float beamLength = 0.0f;
    if (fractionChanged != 0) {
        if (runtime->eventElapsedSec > event->endTimeSec) {
            event->segmentStartCurrent = event->segmentStartFinal;
            event->segmentEndCurrent = event->segmentEndFinal;
        }

        beamLength = UpdateBeamNodeBetweenFractions(beamNode, &pointA, event->segmentStartCurrent,
                                                    &pointB, event->segmentEndCurrent);
    } else {
        beamLength = UpdateBeamNodeBetweenPoints(beamNode, &pointA, &pointB);
    }

    int result = 2;
    if ((flags & 0x8000u) != 0 && beamLength <= event->lengthThreshold) {
        result = 1;
    }
    if (runtime->eventElapsedSec < event->endTimeSec) {
        result = 1;
    }

    g_zEffect_FrameDeltaRemainingSec -= timeSlice;
    return result;
}

// Reimplements 0x45c710: zEffect::HandleScreenColorFxEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleScreenColorFxEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                         zEffectScreenColorFxEvent *event) {
    if (self == 0 || sequenceRuntime == 0 || event == 0) {
        return 2;
    }

    int result = 1;
    const float timeSlice = sequenceRuntime->eventElapsedSec <= event->endTimeSec
                                ? g_zEffect_FrameDeltaRemainingSec
                                : g_zEffect_FrameDeltaRemainingSec -
                                      (sequenceRuntime->eventElapsedSec - event->endTimeSec);
    const float colorTime = sequenceRuntime->eventElapsedSec <= event->endTimeSec
                                ? sequenceRuntime->eventElapsedSec
                                : event->endTimeSec;

    float red = event->redSlope * colorTime + event->redBase;
    float green = event->greenSlope * colorTime + event->greenBase;
    float alpha = event->alphaSlope * colorTime + event->alphaBase;
    float blue = event->blueSlope * colorTime + event->blueBase;
    g_zEffect_FrameDeltaRemainingSec -= timeSlice;

    if (sequenceRuntime->eventElapsedSec > event->endTimeSec) {
        red = event->redEnd;
        green = event->greenEnd;
        alpha = event->alphaEnd;
        blue = event->blueEnd;
        result = 2;
    }

    red = ClampUnitFloat(red);
    green = ClampUnitFloat(green);
    alpha = ClampUnitFloat(alpha);
    blue = ClampUnitFloat(blue);

    const unsigned int packedColor =
        zVid_PackColorRGB(UnitFloatToByte(red), UnitFloatToByte(green), UnitFloatToByte(blue));
    zVideo::FxPass3_SetPrimaryElementParamsLocal(packedColor, static_cast<double>(alpha));
    return result;
}

// Reimplements 0x45c920: zEffect::HandleScreenOverlayFxEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleScreenOverlayFxEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                           zEffectScreenOverlayFxEvent *event) {
    if (self == 0 || sequenceRuntime == 0 || event == 0) {
        return 2;
    }

    const unsigned char flags = static_cast<unsigned char>(event->flagsAndAnchorNodePacked);
    if (sequenceRuntime->runState == 0 && (flags & 0x08u) != 0) {
        const float referenceDistance =
            BeamLengthFromLengthSq(GetConditionalRefPosDistanceSq(self->callbackNode));
        const zVec2 screenScale = zMath_Project_GetLastScreenScaleXY();
        event->maxRadiusNearPixels = event->maxRadiusNearWorld / referenceDistance * screenScale.x;
        event->maxRadiusFarPixels = event->maxRadiusFarWorld / referenceDistance * screenScale.x;

        const int surfaceWidth = zVideo::GetSwSurfaceWidth();
        const int surfaceHeight = zVideo::GetSwSurfaceHeight();
        const float radiusCap =
            static_cast<float>(surfaceWidth > surfaceHeight ? surfaceWidth : surfaceHeight) * 0.25f;
        if (radiusCap < event->maxRadiusNearPixels) {
            event->maxRadiusNearPixels = radiusCap;
        }
        if (radiusCap < event->maxRadiusFarPixels) {
            event->maxRadiusFarPixels = radiusCap;
        }

        event->maxRadiusPixelsSlope =
            (event->maxRadiusFarPixels - event->maxRadiusNearPixels) / event->endTimeSec;
    }

    const float timeSlice = sequenceRuntime->eventElapsedSec <= event->endTimeSec
                                ? g_zEffect_FrameDeltaRemainingSec
                                : g_zEffect_FrameDeltaRemainingSec -
                                      (sequenceRuntime->eventElapsedSec - event->endTimeSec);
    const float overlayTime = sequenceRuntime->eventElapsedSec <= event->endTimeSec
                                  ? sequenceRuntime->eventElapsedSec
                                  : event->endTimeSec;
    g_zEffect_FrameDeltaRemainingSec -= timeSlice;

    int rectLeftPixels = 0;
    int rectTopPixels = 0;
    if ((flags & 0x02u) != 0) {
        const short anchorNodeRefIndex = static_cast<short>(
            static_cast<unsigned int>(event->flagsAndAnchorNodePacked) >> 16);
        if (anchorNodeRefIndex > 0) {
            zVec3 anchorPoint = event->worldAnchor;
            gwNode::TransformPoint(self->nodeRefList[anchorNodeRefIndex].node, &anchorPoint);

            zVec3 projectedPoint = {0};
            if (zMath::ProjectPointAndClampToScreenClip(&anchorPoint, &projectedPoint) != 0) {
                return 1;
            }

            rectLeftPixels = static_cast<int>(projectedPoint.x + 0.5f);
            rectTopPixels = static_cast<int>(projectedPoint.y + 0.5f);
        }
    } else if ((flags & 0x01u) != 0) {
        rectLeftPixels =
            static_cast<int>(event->centerXSlope * overlayTime + event->centerXBase);
        rectTopPixels =
            static_cast<int>(event->centerYSlope * overlayTime + event->centerYBase);
    }

    int maxRadius = static_cast<int>(event->maxRadiusPixelsSlope * overlayTime +
                                                       event->maxRadiusNearPixels);
    int extent =
        static_cast<int>(event->extentSlope * overlayTime + event->extentBase);
    int sinFreqInt =
        static_cast<int>(event->sinFreqSlope * overlayTime + event->sinFreqBase);
    int sinPhaseInt =
        static_cast<int>(event->sinPhaseSlope * overlayTime + event->sinPhaseBase);

    int result = 1;
    if (sequenceRuntime->eventElapsedSec > event->endTimeSec) {
        if ((flags & 0x01u) != 0) {
            rectLeftPixels = static_cast<int>(event->centerXEnd);
            rectTopPixels = static_cast<int>(event->centerYEnd);
        }

        maxRadius = static_cast<int>(event->maxRadiusFarPixels);
        extent = static_cast<int>(event->extentEnd);
        sinFreqInt = static_cast<int>(event->sinFreqEnd);
        sinPhaseInt = static_cast<int>(event->sinPhaseEnd);
        result = 2;
    }

    zVideo::FxPass3_QueueElementLocal(rectLeftPixels, rectTopPixels, 0, maxRadius, extent,
                                      static_cast<float>(sinFreqInt),
                                      static_cast<float>(sinPhaseInt));
    return result;
}

// Reimplements 0x45bb00: zEffect::HandleSurfaceStopEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleSurfaceStopEvent(zEffectAnimEntry *self, zEffectSurfaceControlEvent *event) {
    const int surfaceIndex = ResolveSurfaceRuntimeIndex(self, event);
    if (surfaceIndex >= 0 && self->runtimeList[surfaceIndex].runState == 3) {
        self->runtimeList[surfaceIndex].runState = 0;
    }

    return 2;
}

// Reimplements 0x45bbb0: zEffect::HandleSurfacePlayEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleSurfacePlayEvent(zEffectAnimEntry *self, zEffectSurfaceControlEvent *event) {
    const int surfaceIndex = ResolveSurfaceRuntimeIndex(self, event);
    if (surfaceIndex >= 0) {
        self->runtimeList[surfaceIndex].runState = 2;
    }

    return 2;
}

// Reimplements 0x45bc60: zEffect::HandleSurfaceRefEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleSurfaceRefEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *runtime, zEffectSurfaceRefEvent *event) {
    if (runtime->runState == 0) {
        zEffectAnimEntry *childEntry = 0;
        if (event->runtimeRefIndex >= 0) {
            self->runtimeRefList[event->runtimeRefIndex].cachedChildEntry = 0;
        }

        if (ResolveAnimEntryIndexByName(&event->animEntryIndex, event->sequenceName) > 0) {
            zClass_NodePartial *boundNode = 0;
            if (event->boundNodeRefIndex > 0) {
                boundNode = self->nodeRefList[event->boundNodeRefIndex].node;
            }

            const unsigned short flags = static_cast<unsigned short>(event->flags);
            zEffectAnimEntry *const targetEntry = &g_zEffectAnim_EntryList[event->animEntryIndex];

            if ((flags & 0x01u) != 0) {
                zClass_NodePartial *const refNode =
                    ResolveNodeRefOrResetScratch(self, event->refNodeIndex);
                if (refNode != 0) {
                    zVec3 position = event->position;
                    zVec3 orientation = {0};
                    if ((flags & 0x04u) != 0) {
                        gwNode::GetWorldPosAndOrientation(refNode, &position, &orientation);
                        orientation.x += event->orientationOffset.x;
                        orientation.y += event->orientationOffset.y;
                        orientation.z += event->orientationOffset.z;
                    } else {
                        gwNode::TransformPoint(refNode, &position);
                    }

                    childEntry = zEffectAnim::SetTransformRotAndVelocity(
                        targetEntry, boundNode, position.x, position.y, position.z, orientation.x,
                        orientation.y, orientation.z, self->velocityX, self->velocityY,
                        self->velocityZ);
                }
            } else if ((flags & 0x08u) != 0) {
                zVec3 position = event->position;
                childEntry = zEffectAnim::SetPositionRefAndVelocity(
                    targetEntry, boundNode, ResolveNodeRefOrResetScratch(self, event->refNodeIndex),
                    &position, (const zVec3 *)(&self->velocityX));
            } else {
                childEntry = zEffectAnim::SetVelocity(targetEntry, boundNode, self->velocityX,
                                                      self->velocityY, self->velocityZ);
            }

            if (event->runtimeRefIndex >= 0) {
                self->runtimeRefList[event->runtimeRefIndex].cachedChildEntry = childEntry;
            }
        }

        runtime->runState = 1;
    }

    if ((static_cast<unsigned short>(event->flags) & 0x10u) != 0 && event->runtimeRefIndex >= 0) {
        zEffectAnimEntry *&childEntry =
            self->runtimeRefList[event->runtimeRefIndex].cachedChildEntry;
        if (childEntry != 0 &&
            (childEntry->activationState == 2 || childEntry->activationState == 6)) {
            return 1;
        }
        childEntry = 0;
    }

    runtime->runState = 2;
    return 2;
}

// Reimplements 0x45b8b0: zEffect::HandleTransformRefsEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleTransformRefsEvent(zEffectAnimEntry *self, zEffectTransformRefsEvent *event) {
    if (ResolveAnimEntryIndexByName(&event->animEntryIndex, event->animName) > 0) {
        const unsigned int flags = static_cast<unsigned int>(event->flags);

        zVec3 refPointA = {0};
        if ((flags & 0x10u) != 0) {
            refPointA = event->refPointA;
        } else if ((flags & 0x20u) != 0) {
            refPointA = LoadResetScratchVec(self, 1);
        }

        zVec3 refPointB = {0};
        if ((flags & 0x0400u) != 0) {
            refPointB = event->refPointB;
        } else if ((flags & 0x0800u) != 0) {
            refPointB = LoadResetScratchVec(self, 5);
        }

        zClass_NodePartial *refNodeA = 0;
        if ((flags & 0x01u) != 0) {
            refNodeA = self->nodeRefList[event->refNodeAIndex].node;
        } else if ((flags & 0x02u) != 0) {
            gwNode::TransformPoint(self->nodeRefList[event->refNodeAIndex].node, &refPointA);
        } else if ((flags & 0x04u) != 0) {
            refNodeA = LoadResetScratchNode(self, 0);
        } else if ((flags & 0x08u) != 0) {
            gwNode::TransformPoint(LoadResetScratchNode(self, 0), &refPointA);
        } else {
            refNodeA = (zClass_NodePartial *)(self);
        }

        zClass_NodePartial *refNodeB = 0;
        if ((flags & 0x40u) != 0) {
            refNodeB = self->nodeRefList[event->refNodeBIndex].node;
        } else if ((flags & 0x80u) != 0) {
            gwNode::TransformPoint(self->nodeRefList[event->refNodeBIndex].node, &refPointB);
        } else if ((flags & 0x0100u) != 0) {
            refNodeB = LoadResetScratchNode(self, 4);
        } else if ((flags & 0x0200u) != 0) {
            gwNode::TransformPoint(LoadResetScratchNode(self, 4), &refPointB);
        } else {
            refNodeB = (zClass_NodePartial *)(self);
        }

        zEffectAnimEntry *const childEntry =
            zEffectAnim::SetTransformRefs(&g_zEffectAnim_EntryList[event->animEntryIndex], 0,
                                          refNodeA, &refPointA, refNodeB, &refPointB);

        if (event->runtimeRefIndex >= 0) {
            self->runtimeRefList[event->runtimeRefIndex].cachedChildEntry = childEntry;
        }
    }

    return 2;
}

// Reimplements 0x45c2f0: zEffect::HandleEmitterResetEvent (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleEmitterResetEvent(zEffectAnimSurfaceRuntime *runtime) {
    if (runtime == 0) {
        return -1;
    }

    runtime->sequenceElapsedSec = 0.0f;
    runtime->eventElapsedSec = 0.0f;
    runtime->currentEvent = runtime->eventStream;
    runtime->runState = runtime->resetMode;
    return 0;
}

// Reimplements 0x45c310: zEffect::HandleEmitterLoopEvent (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleEmitterLoopEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *runtime, zEffectAnimLoopEvent *loopEvent) {
    const float loopElapsed = runtime->sequenceElapsedSec + runtime->loopElapsedSec;
    ++runtime->loopIterationCount;
    runtime->loopElapsedSec = loopElapsed;

    const unsigned char stopModeFlags = static_cast<unsigned char>(loopEvent->stopModeFlags);
    if ((stopModeFlags & 0x01) != 0) {
        const unsigned short loopCountLimit = loopEvent->stopValue.u16;
        if (loopCountLimit != 0xffffu && runtime->loopIterationCount == loopCountLimit) {
            runtime->runState = 2;
            return 2;
        }
    } else if ((stopModeFlags & 0x02) != 0) {
        const float elapsedLimit = loopEvent->stopValue.f32;
        if (elapsedLimit >= 0.0f && loopElapsed >= elapsedLimit) {
            runtime->runState = 2;
            return 2;
        }
    }

    if (self->triggerCurrentValue > kEmitterLoopTriggerClampValue) {
        self->triggerCurrentValue = kEmitterLoopTriggerClampValue;
    }

    HandleEmitterResetEvent(runtime);
    return 0;
}

// Reimplements 0x45c240: zEffect::HandleEmitterStopEvent (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleEmitterStopEvent(zEffectAnimEntry * /*self*/, zEffectAnimEmitterEvent *event) {
    const int entryIndex = ResolveEmitterEventEntryIndex(event);
    if (entryIndex > 0) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[entryIndex];
        const unsigned char activationState = entry->activationState;
        if (activationState != 5) {
            entry->activationState = activationState == 2 ? 6 : 4;
        }
    }

    return 2;
}

// Reimplements 0x45c100: zEffect::HandleNamedAnimStopEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleNamedAnimStopEvent(zEffectAnimEntry * /*self*/, zEffectAnimEmitterEvent *event) {
    const int entryIndex = ResolveEmitterEventEntryIndex(event);
    if (entryIndex > 0) {
        zEffectAnim::Stop(&g_zEffectAnim_EntryList[entryIndex]);
    }

    return 2;
}

// Reimplements 0x45c1a0: zEffect::HandleEmitterPlayEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleEmitterPlayEvent(zEffectAnimEntry * /*self*/, zEffectAnimEmitterEvent *event) {
    const int entryIndex = ResolveEmitterEventEntryIndex(event);
    if (entryIndex > 0) {
        zEffect_Anim::NodeActionCallback(&g_zEffectAnim_EntryList[entryIndex], 0);
    }

    return 2;
}

// Reimplements 0x45c3c0: zEffect::HandleConditionalChainEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleConditionalChainEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *runtime, zEffectConditionalEvent *event) {
    int conditionMask = event->conditionMask;
    zEffectAnimEventValue threshold = event->conditionThreshold;
    int stopAfterGroup = 0;
    int cachedConditionMask = 0;
    float conditionalValue = 0.0f;

    while (stopAfterGroup == 0) {
        int conditionMatched = 0;
        if ((conditionMask & 0x01) != 0) {
            const int randIndex = g_zEffect_RandTableIndex;
            conditionalValue = g_zEffect_RandUnitTable[randIndex];
            g_zEffect_RandTableIndex = (randIndex + 1) % 200;
            if (conditionalValue <= threshold.f32) {
                conditionMatched = 1;
            }
        } else if ((conditionMask & 0x02) != 0) {
            if (g_zEffect_ConditionalRefPosEnabled != 0) {
                if (cachedConditionMask != conditionMask) {
                    conditionalValue = GetConditionalRefPosDistanceSq(self->callbackNode);
                }
                if (conditionalValue <= threshold.f32) {
                    conditionMatched = 1;
                }
            }
        } else if ((conditionMask & 0x08) != 0) {
            if (g_zEffect_ConditionalRefPosEnabled != 0) {
                const zVec3 conditionalRefPos = {g_zEffect_ConditionalRefPosX,
                                                 g_zEffect_ConditionalRefPosY,
                                                 g_zEffect_ConditionalRefPosZ};
                int hit = 0;
                if (TraceUpwardHitFromNodeOrPos(0, &conditionalRefPos, &threshold.f32,
                                                &hit) == 0 &&
                    hit != 0) {
                    conditionMatched = 1;
                }
            }
        } else if ((conditionMask & 0x10) != 0) {
            int hit = 0;
            if (TraceUpwardHitFromNodeOrPos(self->nodeRefList[event->nodeIndex].node, 0,
                                            &threshold.f32, &hit) == 0 &&
                hit != 0) {
                conditionMatched = 1;
            }
        } else if ((conditionMask & 0x04) != 0 &&
                   g_zEffect_ConditionalEffectLevel >= threshold.i32) {
            conditionMatched = 1;
        }

        cachedConditionMask = conditionMask;
        if (conditionMatched == 0) {
            unsigned char *nextEvent = static_cast<unsigned char *>(runtime->currentEvent);
            unsigned char *const eventStreamEnd =
                static_cast<unsigned char *>(runtime->eventStream) + runtime->eventStreamSize;

            do {
                const zEffectAnimEventHeader *const header =
                    (zEffectAnimEventHeader *)(nextEvent);
                nextEvent += header->byteSize;
                runtime->currentEvent = nextEvent;

                const unsigned char eventType =
                    ((zEffectAnimEventHeader *)(nextEvent))->eventType;
                if (eventType == 0x20 || eventType == 0x21 || eventType == 0x22) {
                    break;
                }
            } while (nextEvent < eventStreamEnd);

            zEffectAnimEventHeader *const marker =
                (zEffectAnimEventHeader *)(nextEvent);
            if (marker->eventType == 0x21) {
                zEffectConditionalEvent *const elseIfEvent =
                    (zEffectConditionalEvent *)(marker);
                threshold = elseIfEvent->conditionThreshold;
                conditionMask = elseIfEvent->conditionMask;
            } else {
                stopAfterGroup = 1;
            }
        } else {
            stopAfterGroup = 1;
        }
    }

    return 2;
}

// Reimplements 0x45c6b0: zEffect::SkipConditionalChainToEnd
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL SkipConditionalChainToEnd(
    zEffectAnimEntry * /*self*/, zEffectAnimSurfaceRuntime *runtime, void * /*event*/) {
    unsigned char *currentEvent = static_cast<unsigned char *>(runtime->currentEvent);
    unsigned char *const eventStreamEnd =
        static_cast<unsigned char *>(runtime->eventStream) + runtime->eventStreamSize;

    do {
        const zEffectAnimEventHeader *const header =
            (zEffectAnimEventHeader *)(currentEvent);
        currentEvent += header->byteSize;
        runtime->currentEvent = currentEvent;
    } while (currentEvent[0] != 0x22 && currentEvent < eventStreamEnd);

    return 2;
}

// Reimplements 0x45c6e0: zEffect::HandleNoOpMarkerEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleNoOpMarkerEvent(
    zEffectAnimEntry * /*self*/, zEffectAnimSurfaceRuntime * /*runtime*/, void * /*event*/) {
    return 2;
}

// Reimplements 0x45c6f0: zEffect::HandleCallbackEvent (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleCallbackEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime * /*runtime*/,
                    zEffectAnimCallbackEvent *event) {
    if (self->eventCallback != 0) {
        self->eventCallback(self, self->eventCallbackContext, event->value);
    }

    return 2;
}

// Reimplements 0x45cbc0: zEffect::HandleTopMessageEvent
// (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL HandleTopMessageEvent(zEffectAnimEntry * /*self*/,
                                                                   zEffectTopMessageEvent *event) {
    const int textIdIndex = event->textIdIndex;
    if (textIdIndex >= 0) {
        const int messageId = g_zEffectAnim_TextIdEntryList[textIdIndex].messageId;
        if (messageId != 0) {
            HudUi::PushTopMessageLine(zLoc::GetMessageString(static_cast<unsigned int>(messageId)),
                                      3.0f);
        }
    }

    return 2;
}

// Reimplements 0x45bf60: zEffect::CleanupLightRefs (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL CleanupLightRefs(zEffectAnimEntry *self) {
    for (int i = 0; i < self->lightRefCount; ++i) {
        zEffectAnimRuntimeNodeRef *const lightRef =
            self->lightRefList != 0 ? &self->lightRefList[i] : 0;
        if (lightRef == 0) {
            continue;
        }

        zClass_NodePartial *const runtimeNode = lightRef->runtimeNode;
        if (runtimeNode == 0) {
            continue;
        }

        if ((runtimeNode->flags & 0x04) != 0) {
            zClass_Class::gwNodeSetActive(runtimeNode, 0);
        }

        if (lightRef->isAttached != 0) {
            zClass_World::RemoveLight(g_zEffect_World, runtimeNode);
            lightRef->isAttached = 0;
        }
    }

    return 0;
}

// Reimplements 0x45bfd0: zEffect::CleanupSoundRefs (zeff_anim_run.c)
RECOIL_NOINLINE int RECOIL_FASTCALL CleanupSoundRefs(zEffectAnimEntry *self) {
    for (int i = 0; i < self->soundRefCount; ++i) {
        zEffectAnimRuntimeNodeRef *const soundRef =
            self->soundRefList != 0 ? &self->soundRefList[i] : 0;
        if (soundRef == 0) {
            continue;
        }

        zClass_NodePartial *const runtimeNode = soundRef->runtimeNode;
        if (runtimeNode == 0) {
            continue;
        }

        if ((runtimeNode->flags & 0x04) != 0) {
            zClass_Class::gwNodeSetActive(runtimeNode, 0);
        }

        if (soundRef->isAttached != 0) {
            zClass_World::RemoveSound(g_zEffect_World, runtimeNode);
            soundRef->isAttached = 0;
        }
    }

    return 0;
}

// Reimplements 0x460330: zEffect::Reset
RECOIL_NOINLINE int RECOIL_CDECL Reset() {
    if (g_zEffect_RuntimeManager.loadedTemplateTree != 0) {
        zReader::FreeLoadedTree(
            (zReader::Node *)(g_zEffect_RuntimeManager.loadedTemplateTree));
        g_zEffect_RuntimeManager.loadedTemplateTree = 0;
    }

    if (g_zEffect_RuntimeManager.templates != 0) {
        free(g_zEffect_RuntimeManager.templates);
        g_zEffect_RuntimeManager.templates = 0;
    }

    zArchiveList *freeList = g_zEffect_RuntimeManager.freeList;
    if (freeList != 0) {
        zEffect_RuntimeEntry * entry = static_cast<zEffect_RuntimeEntry *>(zArchiveList_PopFrontPayload(freeList));
        while (entry != 0) {
            if (entry->effectNode != 0) {
                zClass_Object3D::DeleteNode(entry->effectNode);
            }

            free(entry);
            entry = static_cast<zEffect_RuntimeEntry *>(zArchiveList_PopFrontPayload(freeList));
        }

        zArchiveList_Destroy(freeList);
        g_zEffect_RuntimeManager.freeList = 0;
    }

    g_zEffect_RuntimeManager.recycleCount = 0;
    Init();
    return 0;
}

// Reimplements 0x460060: zEffect::ShutdownAll
RECOIL_NOINLINE int RECOIL_CDECL ShutdownAll() {
    Reset();
    return zEffect_Anim::ShutdownIfLoaded();
}
} // namespace zEffect
