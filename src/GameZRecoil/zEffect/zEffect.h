#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/zMath/zMath.h"
#include "recoil/recoil_callconv.h"
#include "zClass.h"

struct zArchiveList;
struct zZbdSectionCallbackCtx;
struct zEffect_RuntimeEntry;
struct zClassDiPickCandidateEntry;

struct zEffect_RuntimeManager {
    int initialized;
    int templateCount;
    zClass_NodePartial *loadedTemplateTree;
    zArchiveList *freeList;
    zEffect_RuntimeEntry *templates;
    zClass_NodePartial *parentNode;
    zClass_NodePartial *listenerNode;
    int freshAllocCount;
    int activatedCount;
    int recycleCount;
};

struct zEffect_RuntimeEntry {
    int effectIndex;
    char *modelNodeName;
    char *effectName;
    zClass_NodePartial *effectNode;
    void *effectGfxData;
    float fadeInTimeSec;
    float fadeInScaleRate;
    float baseScale;
    float lifeTimeSec;
    float fadeOutScaleRate;
    float fadeOutStartScale;
    float fadeOutEndScale;
    float fadeOutStartTimeSec;
    float elapsedSec;
    float currentScale;
    float initialScale;
    float nearCullDistSq;
    float farFadeDistSq;
};

struct zEffectAnimSurfaceRuntime {
    char sequenceName[0x20];
    unsigned char runState;
    unsigned char resetMode;
    unsigned char unknown_22[0x02];
    float sequenceElapsedSec;
    float eventElapsedSec;
    float loopElapsedSec;
    unsigned short loopIterationCount;
    unsigned char unknown_32[0x02];
    void *currentEvent;
    void *eventStream;
    int eventStreamSize;
};

struct zEffectAnimEntry;

typedef void (RECOIL_FASTCALL *zEffectAnimEventCallback)(zEffectAnimEntry *self, void *context,
                                                         int value);

struct zEffectAnimEventHeader {
    union {
        unsigned char eventType;
        unsigned char typeAndStartMode;
    };
    unsigned char startMode;
    unsigned char unknown_02[0x02];
    union {
        int recordSize;
        int byteSize;
    };
    float startThreshold;
};

union zEffectAnimEventValue {
    unsigned int rawValue;
    int i32;
    unsigned short u16;
    float f32;
};

struct zEffectAnimLoopEvent {
    unsigned char unknown_00[0x0c];
    unsigned int stopModeFlags;
    zEffectAnimEventValue stopValue;
};

struct zEffectAnimEmitterEvent {
    unsigned char unknown_00[0x0c];
    char animName[0x20];
    int cachedEntryIndex;
};

struct zEffectAnimCallbackEvent {
    unsigned char unknown_00[0x0c];
    int value;
};

struct zEffectAnimRefOffsetEvent {
    unsigned char unknown_00[0x0c];
    short refIndex;
    short nodeRefIndex;
    float offsetX;
    float offsetY;
    float offsetZ;
};

struct zEffectAnimSoundEvent {
    unsigned char unknown_00[0x0c];
    char soundName[0x20];
    int soundRefIndex;
    int fieldMask;
    int activeState;
    int parentNodeRefIndex;
    float offsetX;
    float offsetY;
    float offsetZ;
};

struct zEffectAnimLightEvent {
    unsigned char unknown_00[0x0c];
    char lightName[0x20];
    int lightRefIndex;
    int fieldMask;
    int activeState;
    int mode;
    unsigned int coneAngleBits;
    int param;
    int basisNodeRefIndex;
    float basisOrColorX;
    float basisOrColorY;
    float basisOrColorZ;
    float positionX;
    float positionY;
    float positionZ;
    float rangeInner;
    float rangeOuter;
    float specularR;
    float specularG;
    float specularB;
    float intensity;
    float falloff;
};

struct zEffectLightRangeSpecularAnimEvent {
    zEffectAnimEventHeader header;
    char lightName[0x20];
    int lightRefIndex;
    float initialRangeInner;
    float initialRangeOuter;
    float rangeInnerDelta;
    float rangeOuterDelta;
    float currentRangeInner;
    float currentRangeOuter;
    float initialSpecularR;
    float initialSpecularG;
    float initialSpecularB;
    float specularRDelta;
    float specularGDelta;
    float specularBDelta;
    float currentSpecularR;
    float currentSpecularG;
    float currentSpecularB;
    float durationSec;
};

struct zEffectFogEvent {
    zEffectAnimEventHeader header;
    char unused0[0x20];
    int flags;
    int fogState;
    float fogColorR;
    float fogColorG;
    float fogColorB;
    float fogAltitudeMin;
    float fogAltitudeMax;
    float fogRangeStart;
    float fogRangeEnd;
};

struct zEffectCameraEvent {
    zEffectAnimEventHeader header;
    int flags;
    int targetNodeRefIndex;
    float nearClip;
    float farClip;
    float clipDistance;
    float fovPrimary;
    float fovSecondary;
    float viewportPrimary;
    float viewportSecondary;
};

struct zEffectCameraAnimEvent {
    zEffectAnimEventHeader header;
    int flags;
    int targetNodeRefIndex;
    float nearClipStart;
    float nearClipEnd;
    float nearClipRate;
    float farClipStart;
    float farClipEnd;
    float farClipRate;
    float clipDistanceStart;
    float clipDistanceEnd;
    float clipDistanceRate;
    float fovPrimaryStart;
    float fovPrimaryEnd;
    float fovPrimaryRate;
    float fovSecondaryStart;
    float fovSecondaryEnd;
    float fovSecondaryRate;
    float viewportPrimaryStart;
    float viewportPrimaryEnd;
    float viewportPrimaryRate;
    float viewportSecondaryStart;
    float viewportSecondaryEnd;
    float viewportSecondaryRate;
    float endTime;
};

struct zEffectNodeAnimEvent {
    zEffectAnimEventHeader header;
    int flags;
    int targetNodeRefIndex;
    float nodeAlphaStart;
    float nodeAlphaEnd;
    float nodeAlphaRate;
    zVec3 positionOrTargetStart;
    zVec3 positionOrTargetEnd;
    zVec3 positionOrTargetRate;
    zVec3 rotationOrCameraPosStart;
    zVec3 rotationOrCameraPosEnd;
    zVec3 rotationOrCameraPosRate;
    zVec3 scaleStart;
    zVec3 scaleEnd;
    zVec3 scaleRate;
    float endTimeSec;
    unsigned char unknown_90[0x04];
    zVec3 runtimeVecA;
    zVec3 runtimeVecB;
    zVec3 runtimeVecC;
    zVec3 runtimeVecD;
    zVec3 runtimeVecE;
    char targetName[0x20];
    short packedRuntimeIndex;
    short sampleRefIndex;
    float lookupScale;
    float runtimeElapsedSec;
};

struct zEffectTransformEvent {
    zEffectAnimEventHeader header;
    int flags;
    float vecX;
    float vecY;
    float vecZ;
    short targetNodeRefIndex;
    short basisNodeRefIndex;
};

struct zEffectNodeScaleEvent {
    zEffectAnimEventHeader header;
    float scaleX;
    float scaleY;
    float scaleZ;
    short targetNodeRefIndex;
    unsigned char unknown_1a[0x02];
};

struct zEffectActivateEvent {
    zEffectAnimEventHeader header;
    int activeValue;
    short targetNodeRefIndex;
    unsigned char unknown_12[0x02];
};

struct zEffectParentChildEvent {
    zEffectAnimEventHeader header;
    short parentNodeRefIndex;
    short childNodeRefIndex;
};

struct zEffectAttachEvent {
    zEffectAnimEventHeader header;
    int flags;
    short targetNodeRefIndex;
    short variantIndex;
};

struct zEffectSurfaceControlEvent {
    zEffectAnimEventHeader header;
    char sequenceName[0x20];
    int surfaceSlotIndex;
};

struct zEffectSurfaceRefEvent {
    zEffectAnimEventHeader header;
    char sequenceName[0x14];
    int runtimeState;
    unsigned char unknown_24[0x08];
    short boundNodeRefIndex;
    short flags;
    short animEntryIndex;
    short runtimeRefIndex;
    short refNodeIndex;
    unsigned char unknown_36[0x02];
    zVec3 position;
    zVec3 orientationOffset;
};

struct zEffectBeamDetachEvent {
    zEffectAnimEventHeader header;
    int flags;
    short beamNodeRefIndex;
    short pointANodeRefIndex;
    short pointBNodeRefIndex;
    short reserved;
    zVec3 pointA;
    zVec3 pointB;
    float segmentStartInitial;
    float segmentStartFinal;
    float segmentStartRate;
    float segmentStartCurrent;
    float segmentEndInitial;
    float segmentEndFinal;
    float segmentEndRate;
    float segmentEndCurrent;
    float endTimeSec;
    float lengthThreshold;
};

struct zEffectKeyframeSampleHeader {
    int channelFlags;
    float startTimeSec;
    float endTimeSec;
};

struct zEffectKeyframeSampleChannel {
    zQuat baseQuat;
    zVec3 rate;
};

struct zEffectKeyframeEvent {
    zEffectAnimEventHeader header;
    int targetNodeRefIndex;
    int reserved;
    float keyframeLocalTime;
    int currentKeyframeOffset;
    int lookaheadAdvanceCount;
};

struct zEffectEvaluateKeyframeEvent {
    zEffectAnimEventHeader header;
    short litFlag;
    short hasAlphaScale;
    float alphaScale;
    short targetNodeRefIndex;
    short reserved;
};

struct zEffectRunKeyframeEvent {
    zEffectAnimEventHeader header;
    int targetNodeRefIndex;
    short startLitFlag;
    short endLitFlag;
    float startAlphaScale;
    float endAlphaScale;
    float alphaScaleRate;
    float endTimeSec;
};

struct zEffectScreenColorFxEvent {
    zEffectAnimEventHeader header;
    float redBase;
    float redEnd;
    float redSlope;
    float greenBase;
    float greenEnd;
    float greenSlope;
    float alphaBase;
    float alphaEnd;
    float alphaSlope;
    float blueBase;
    float blueEnd;
    float blueSlope;
    float endTimeSec;
};

struct zEffectScreenOverlayFxEvent {
    zEffectAnimEventHeader header;
    int flagsAndAnchorNodePacked;
    zVec3 worldAnchor;
    float centerXBase;
    float centerXEnd;
    float centerXSlope;
    float centerYBase;
    float centerYEnd;
    float centerYSlope;
    float maxRadiusNearWorld;
    float maxRadiusFarWorld;
    float maxRadiusNearPixels;
    float maxRadiusFarPixels;
    float maxRadiusPixelsSlope;
    float extentBase;
    float extentEnd;
    float extentSlope;
    float sinFreqBase;
    float sinFreqEnd;
    float sinFreqSlope;
    float sinPhaseBase;
    float sinPhaseEnd;
    float sinPhaseSlope;
    float endTimeSec;
};

struct zEffectTransformRefsEvent {
    zEffectAnimEventHeader header;
    int flags;
    char animName[0x20];
    short animEntryIndex;
    short runtimeRefIndex;
    short refNodeAIndex;
    short refNodeBIndex;
    zVec3 refPointA;
    zVec3 refPointB;
};

struct zEffectConditionalEvent {
    zEffectAnimEventHeader header;
    int conditionMask;
    int nodeIndex;
    zEffectAnimEventValue conditionThreshold;
};

struct zEffectTopMessageEvent {
    zEffectAnimEventHeader header;
    int textIdIndex;
};

struct zEffectAnimCapturedNodeState {
    int activeFlag;
    int usesCachedMatrix;
    float transformSnapshot[12];
};

struct zEffectAnimRefName {
    char text[0x24];
};

struct zEffectAnimTrackedNode {
    char trackedNodeName[0x24];
    zClass_NodePartial *trackedNode;
    zEffectAnimCapturedNodeState capturedState;
};

struct zEffectAnimNodeRef28 {
    zEffectAnimRefName name;
    zClass_NodePartial *node;
};

struct zEffectAnimRuntimeNodeRef {
    zEffectAnimRefName name;
    zClass_NodePartial *runtimeNode;
    int isAttached;
};

struct zEffectAnimSampleRef {
    char name[0x20];
    zSndSample *sample;
};

struct zEffectAnimTemplateIndexRef {
    char name[0x20];
    int templateIndex;
};

struct zEffectAnimRuntimeRef {
    char entryName[0x20];
    char spawnDescriptor[0x20];
    int stopCachedChildOnCleanup;
    zEffectAnimEntry *cachedChildEntry;
};

struct zEffectAnimActivationPrereq {
    int requireMatch;
    unsigned char mode;
    unsigned char unknown_05[0x03];
    char targetName[0x20];
    zEffectAnimEntry *targetEntry;
    zClass_NodePartial *targetNode;
};

struct zEffectAnimEntry {
    char name[0x20];
    char rootNodeName[0x20];
    zClass_NodePartial *boundNode;
    char attachNodeName[0x20];
    zClass_NodePartial *callbackNode;
    zClass_NodePartial *runtimeNode;
    zEffectAnimEventCallback eventCallback;
    void *eventCallbackContext;
    unsigned int resetScratch[8];
    unsigned int flags;
    unsigned char activationState;
    unsigned char activationMode;
    unsigned char priority;
    unsigned char variantCycleDelay;
    float distRefMinSq;
    float distRefMaxSq;
    float triggerBaseValue;
    float triggerCurrentValue;
    int triggerContext;
    float activationCountdown;
    float velocityX;
    float velocityY;
    float velocityZ;
    zEffectAnimSurfaceRuntime *runtimeList;
    zEffectAnimSurfaceRuntime surfacePrimary;
    unsigned char runtimeSequenceCount;
    unsigned char trackedNodeCount;
    unsigned char nodeRefCount;
    unsigned char lightRefCount;
    unsigned char soundRefCount;
    unsigned char sampleRefCount;
    unsigned char effectTemplateRefCount;
    unsigned char activationPrereqCount;
    unsigned char activationPrereqMinimumMatchCount;
    unsigned char runtimeRefCount;
    unsigned char unknown_10e[0x02];
    zEffectAnimTrackedNode *trackedNodeList;
    zEffectAnimNodeRef28 *nodeRefList;
    zEffectAnimRuntimeNodeRef *lightRefList;
    zEffectAnimRuntimeNodeRef *soundRefList;
    zEffectAnimSampleRef *sampleRefList;
    zEffectAnimTemplateIndexRef *effectTemplateRefList;
    zEffectAnimActivationPrereq *activationPrereqList;
    zEffectAnimRuntimeRef *runtimeRefList;
    zEffectAnimEntry *runtimeSibling;

    static RECOIL_NOINLINE void RECOIL_FASTCALL SetOnStateDoneCallback(zEffectAnimEntry *self,
                                                                       void *callback, void *user);
};

struct zEffectAnimTextIdEntry {
    char messageKey[0x20];
    int messageId;
};

struct zEffectAnimSourceFileStamp {
    char sourcePath[0x50];
    int fileMtime;
};

union zEffectAnimActivationParam {
    unsigned int u32;
    int i32;
    float f32;
};

struct zEffectAnimActivationRecord {
    int commandType;
    unsigned int recordId;
    char animName[0x20];
    int nodeToken;
    zEffectAnimActivationParam params[9];
};

RECOIL_STATIC_ASSERT(sizeof(zEffect_RuntimeManager) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zEffect_RuntimeManager, freshAllocCount) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSurfaceRuntime) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSurfaceRuntime, runState) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSurfaceRuntime, resetMode) == 0x21);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSurfaceRuntime, sequenceElapsedSec) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSurfaceRuntime, eventElapsedSec) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSurfaceRuntime, loopElapsedSec) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSurfaceRuntime, loopIterationCount) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSurfaceRuntime, currentEvent) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSurfaceRuntime, eventStream) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSurfaceRuntime, eventStreamSize) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEventHeader, byteSize) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEventHeader, startThreshold) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimActivationPrereq) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationPrereq, targetName) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationPrereq, targetEntry) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationPrereq, targetNode) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimEventValue) == 4);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, targetNodeRefIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, nodeAlphaStart) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, positionOrTargetStart) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, rotationOrCameraPosStart) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, scaleStart) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, scaleRate) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, endTimeSec) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, runtimeVecA) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, targetName) == 0xd0);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeAnimEvent, runtimeElapsedSec) == 0xf8);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLoopEvent, stopModeFlags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLoopEvent, stopValue) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEmitterEvent, animName) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEmitterEvent, cachedEntryIndex) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimCallbackEvent, value) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimRefOffsetEvent, refIndex) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimRefOffsetEvent, nodeRefIndex) == 0x0e);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimRefOffsetEvent, offsetX) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSoundEvent, soundName) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSoundEvent, soundRefIndex) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSoundEvent, fieldMask) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSoundEvent, activeState) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSoundEvent, parentNodeRefIndex) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSoundEvent, offsetX) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, lightName) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, lightRefIndex) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, fieldMask) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, activeState) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, mode) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, coneAngleBits) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, basisNodeRefIndex) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, basisOrColorX) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, positionX) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, rangeInner) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, specularR) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, intensity) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimLightEvent, falloff) == 0x78);
RECOIL_STATIC_ASSERT(sizeof(zEffectLightRangeSpecularAnimEvent) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zEffectLightRangeSpecularAnimEvent, lightName) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectLightRangeSpecularAnimEvent, lightRefIndex) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zEffectLightRangeSpecularAnimEvent, currentRangeInner) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zEffectLightRangeSpecularAnimEvent, currentSpecularR) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zEffectLightRangeSpecularAnimEvent, durationSec) == 0x6c);
RECOIL_STATIC_ASSERT(sizeof(zEffectFogEvent) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zEffectFogEvent, flags) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zEffectFogEvent, fogState) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectFogEvent, fogColorR) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zEffectFogEvent, fogAltitudeMin) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zEffectFogEvent, fogRangeStart) == 0x48);
RECOIL_STATIC_ASSERT(sizeof(zEffectCameraEvent) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectCameraEvent, flags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectCameraEvent, targetNodeRefIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectCameraEvent, nearClip) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zEffectCameraEvent, viewportSecondary) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(zEffectCameraAnimEvent) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(zEffectCameraAnimEvent, flags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectCameraAnimEvent, nearClipStart) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zEffectCameraAnimEvent, fovPrimaryStart) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zEffectCameraAnimEvent, viewportSecondaryRate) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(zEffectCameraAnimEvent, endTime) == 0x68);
RECOIL_STATIC_ASSERT(sizeof(zEffectTransformEvent) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformEvent, flags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformEvent, vecX) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformEvent, targetNodeRefIndex) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformEvent, basisNodeRefIndex) == 0x1e);
RECOIL_STATIC_ASSERT(sizeof(zEffectNodeScaleEvent) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeScaleEvent, scaleX) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectNodeScaleEvent, targetNodeRefIndex) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zEffectActivateEvent) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zEffectActivateEvent, activeValue) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectActivateEvent, targetNodeRefIndex) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zEffectParentChildEvent) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectParentChildEvent, parentNodeRefIndex) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectParentChildEvent, childNodeRefIndex) == 0x0e);
RECOIL_STATIC_ASSERT(sizeof(zEffectAttachEvent) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zEffectAttachEvent, flags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAttachEvent, targetNodeRefIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectAttachEvent, variantIndex) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(zEffectSurfaceControlEvent) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceControlEvent, sequenceName) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceControlEvent, surfaceSlotIndex) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(zEffectSurfaceRefEvent) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceRefEvent, sequenceName) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceRefEvent, runtimeState) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceRefEvent, boundNodeRefIndex) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceRefEvent, flags) == 0x2e);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceRefEvent, animEntryIndex) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceRefEvent, runtimeRefIndex) == 0x32);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceRefEvent, refNodeIndex) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceRefEvent, position) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zEffectSurfaceRefEvent, orientationOffset) == 0x44);
RECOIL_STATIC_ASSERT(sizeof(zEffectBeamDetachEvent) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, flags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, beamNodeRefIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, pointANodeRefIndex) == 0x12);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, pointBNodeRefIndex) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, pointA) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, pointB) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, segmentStartInitial) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, segmentEndInitial) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, endTimeSec) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zEffectBeamDetachEvent, lengthThreshold) == 0x54);
RECOIL_STATIC_ASSERT(sizeof(zEffectKeyframeSampleHeader) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectKeyframeSampleHeader, startTimeSec) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zEffectKeyframeSampleHeader, endTimeSec) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zEffectKeyframeSampleChannel) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zEffectKeyframeSampleChannel, rate) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zEffectKeyframeEvent) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zEffectKeyframeEvent, targetNodeRefIndex) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectKeyframeEvent, keyframeLocalTime) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zEffectKeyframeEvent, currentKeyframeOffset) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zEffectKeyframeEvent, lookaheadAdvanceCount) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(zEffectEvaluateKeyframeEvent) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zEffectEvaluateKeyframeEvent, litFlag) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectEvaluateKeyframeEvent, alphaScale) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectEvaluateKeyframeEvent, targetNodeRefIndex) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zEffectRunKeyframeEvent) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zEffectRunKeyframeEvent, targetNodeRefIndex) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectRunKeyframeEvent, startLitFlag) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectRunKeyframeEvent, startAlphaScale) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zEffectRunKeyframeEvent, endAlphaScale) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zEffectRunKeyframeEvent, alphaScaleRate) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zEffectRunKeyframeEvent, endTimeSec) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(zEffectScreenColorFxEvent) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenColorFxEvent, redBase) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenColorFxEvent, greenBase) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenColorFxEvent, alphaBase) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenColorFxEvent, blueBase) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenColorFxEvent, endTimeSec) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(zEffectScreenOverlayFxEvent) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenOverlayFxEvent, flagsAndAnchorNodePacked) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenOverlayFxEvent, worldAnchor) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenOverlayFxEvent, centerXBase) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenOverlayFxEvent, maxRadiusNearWorld) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenOverlayFxEvent, maxRadiusNearPixels) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenOverlayFxEvent, extentBase) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenOverlayFxEvent, sinFreqBase) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenOverlayFxEvent, sinPhaseBase) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zEffectScreenOverlayFxEvent, endTimeSec) == 0x6c);
RECOIL_STATIC_ASSERT(sizeof(zEffectTransformRefsEvent) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformRefsEvent, flags) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformRefsEvent, animName) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformRefsEvent, animEntryIndex) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformRefsEvent, refNodeAIndex) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformRefsEvent, refPointA) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zEffectTransformRefsEvent, refPointB) == 0x44);
RECOIL_STATIC_ASSERT(sizeof(zEffectConditionalEvent) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zEffectConditionalEvent, conditionMask) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zEffectConditionalEvent, nodeIndex) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectConditionalEvent, conditionThreshold) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zEffectTopMessageEvent) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zEffectTopMessageEvent, textIdIndex) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimCapturedNodeState) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimCapturedNodeState, transformSnapshot) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimTrackedNode) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimTrackedNode, trackedNodeName) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimTrackedNode, trackedNode) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimTrackedNode, capturedState) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimNodeRef28) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimNodeRef28, node) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRuntimeNodeRef) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimRuntimeNodeRef, runtimeNode) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimRuntimeNodeRef, isAttached) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSampleRef) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimTemplateIndexRef) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRuntimeRef) == 0x48);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimTextIdEntry) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimTextIdEntry, messageId) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSourceFileStamp) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSourceFileStamp, fileMtime) == 0x50);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimEntry) == 0x134);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, rootNodeName) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, boundNode) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, attachNodeName) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, callbackNode) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, runtimeNode) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, eventCallback) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, eventCallbackContext) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, resetScratch) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, flags) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, activationState) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, activationMode) == 0x99);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, priority) == 0x9a);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, variantCycleDelay) == 0x9b);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, distRefMinSq) == 0x9c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, distRefMaxSq) == 0xa0);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, triggerBaseValue) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, triggerCurrentValue) == 0xa8);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, triggerContext) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, activationCountdown) == 0xb0);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, velocityX) == 0xb4);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, velocityY) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, velocityZ) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, runtimeList) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, surfacePrimary) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, surfacePrimary.eventStream) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, runtimeSequenceCount) == 0x104);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, trackedNodeCount) == 0x105);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, nodeRefCount) == 0x106);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, lightRefCount) == 0x107);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, soundRefCount) == 0x108);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, sampleRefCount) == 0x109);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, effectTemplateRefCount) == 0x10a);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, activationPrereqCount) == 0x10b);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, activationPrereqMinimumMatchCount) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, runtimeRefCount) == 0x10d);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, trackedNodeList) == 0x110);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, nodeRefList) == 0x114);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, lightRefList) == 0x118);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, soundRefList) == 0x11c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, sampleRefList) == 0x120);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, effectTemplateRefList) == 0x124);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, activationPrereqList) == 0x128);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, runtimeRefList) == 0x12c);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimEntry, runtimeSibling) == 0x130);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimActivationRecord) == 0x50);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimActivationParam) == 4);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationRecord, animName) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationRecord, nodeToken) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationRecord, params) == 0x2c);

extern "C" {
extern zEffect_RuntimeManager g_zEffect_RuntimeManager;
extern float g_zEffect_RandUnitTable[200];
extern float g_zEffect_RandUnitScale;
extern int g_zEffect_RandTableIndex;
extern char g_zEffectAnim_ZbdFilename[0x80];
extern int g_zEffectAnim_EntriesInstantiated;
extern void *g_zEffectAnim_HeapPtr;
extern short g_zEffectAnim_CountsPackedLoWord;
extern short g_zEffectAnim_EntryCount;
extern zEffectAnimEntry *g_zEffectAnim_EntryList;
extern int g_zEffectAnim_TextIdEntryCount;
extern zEffectAnimTextIdEntry *g_zEffectAnim_TextIdEntryList;
extern int g_zEffectAnim_SourceFileStampCount;
extern zEffectAnimSourceFileStamp *g_zEffectAnim_SourceFileStampList;
extern int g_zEffectAnim_CopyNodeMode;
extern int g_zEffectAnim_CopyNodeArg1;
extern int g_zEffectAnim_CopyNodeArg2;
extern int g_zEffectAnim_ForceCloneNonDynamicRoot;
extern int g_zEffect_CloneCopyMode;
extern int g_zEffect_CloneCopyChildrenMode;
extern zClass_NodePartial *g_zEffect_World;
extern float g_zEffect_DefaultGravity;
extern int g_zEffect_ConditionalRefPosEnabled;
extern float g_zEffect_ConditionalRefPosX;
extern float g_zEffect_ConditionalRefPosY;
extern float g_zEffect_ConditionalRefPosZ;
extern int g_zEffect_ConditionalEffectLevel;
extern int g_zEffect_VariantOverrideEnabled;
extern unsigned int g_zEffect_VariantOverridePackedIds;
extern unsigned char g_zEffect_VariantCycleId;
extern int g_zEffect_SkipStopDelay;
extern float g_zEffect_FrameDeltaRemainingSec;
extern int g_zEffect_Anim_DebugFrameTag;
extern zClass_NodePartial *g_zEffect_ResourceNode;
extern zEffectAnimActivationRecord *g_zEffectAnim_ActivationRecordTable;
extern int g_zEffectAnim_ActivationRecordCapacity;
extern int g_zEffectAnim_ActivationRecordCount;
extern void(RECOIL_FASTCALL *g_zEffectAnim_ActivationDispatchCallback)(
    zEffectAnimActivationRecord *record);
extern unsigned int g_zEffectAnim_ActivationDispatchTagHigh;
extern int g_zEffectAnim_RecordQueueEnabled;
extern int g_zEffectAnim_DispatchEnabled;
extern int g_zEffectAnim_EnableZarRegistration;
extern char g_zEffectAnim_ZarSectionName_Anim[5];
extern char g_zEffectAnim_ZarSectionName_RunningAnim[12];
extern char g_zEffectAnim_ZarSectionName_AnimActivation[15];
}

namespace zEffect_Anim {
RECOIL_NOINLINE int RECOIL_CDECL Init();
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownIfLoaded();
RECOIL_NOINLINE void RECOIL_CDECL ClearActivationRecords();
RECOIL_NOINLINE int RECOIL_FASTCALL
HasActivationRecord(zEffectAnimActivationRecord *record);
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_CDECL AllocActivationRecord();
RECOIL_NOINLINE int RECOIL_CDECL GetActivationRecordCount();
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL
GetActivationRecordAt(int index);
RECOIL_NOINLINE int RECOIL_FASTCALL
SaveActivationRecords(zZbdSectionCallbackCtx *callbackCtx);
RECOIL_NOINLINE void RECOIL_FASTCALL LoadActivationRecords(void *unused, const char *sectionToken,
                                                           void *data, int dataSize,
                                                           void *extraCtx);
RECOIL_NOINLINE int RECOIL_FASTCALL
SaveRunningAnimRecord(zZbdSectionCallbackCtx *callbackCtx, zEffectAnimEntry *entry,
                      int runningIndex, int includePrimaryEntry);
RECOIL_NOINLINE int RECOIL_FASTCALL
SaveRunningAnimRecords(zZbdSectionCallbackCtx *callbackCtx);
RECOIL_NOINLINE void RECOIL_FASTCALL LoadRunningAnimRecords(void *unused, const char *sectionToken,
                                                            void *data, int dataSize,
                                                            void *extraCtx);
RECOIL_NOINLINE int RECOIL_FASTCALL SaveAnimRecords(zZbdSectionCallbackCtx *callbackCtx);
RECOIL_NOINLINE void RECOIL_FASTCALL LoadAnimRecords(void *unused, const char *sectionToken,
                                                     void *data, int dataSize,
                                                     void *extraCtx);
RECOIL_NOINLINE void RECOIL_FASTCALL ResetFromActivationRecord(zEffectAnimActivationRecord *record);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL
ProcessActivationRecord(zEffectAnimActivationRecord *record);
RECOIL_NOINLINE int RECOIL_FASTCALL CaptureNodeStates(zEffectAnimEntry *self);
RECOIL_NOINLINE int RECOIL_FASTCALL RestoreNodeStates(zEffectAnimEntry *self);
RECOIL_NOINLINE int RECOIL_FASTCALL AdvanceKeyframeSample(
    zEffectAnimSurfaceRuntime *sequenceRuntime, zEffectKeyframeEvent *keyframeEvent,
    zEffectKeyframeSampleHeader *sampleHeader);
RECOIL_NOINLINE float RECOIL_FASTCALL AnimateKeyframeSample(
    zEffectAnimSurfaceRuntime *sequenceRuntime, zEffectKeyframeEvent *keyframeEvent,
    zClass_NodePartial *targetNode, zEffectKeyframeSampleHeader *sampleHeader, float *deltaTime);
RECOIL_NOINLINE int RECOIL_FASTCALL
AdvanceKeyframe(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                zEffectKeyframeEvent *keyframeEvent);
RECOIL_NOINLINE int RECOIL_FASTCALL
EvaluateKeyframe(zEffectAnimEntry *self, zEffectEvaluateKeyframeEvent *keyframeEvent);
RECOIL_NOINLINE int RECOIL_FASTCALL
RunKeyframes(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
             zEffectRunKeyframeEvent *keyframeEvent);
RECOIL_NOINLINE int RECOIL_FASTCALL
RunSequenceEvents(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime);
RECOIL_NOINLINE int RECOIL_FASTCALL RunSequence(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL NodeActionCallback(zEffectAnimEntry *self,
                                                                zClass_NodePartial *rootNode);
RECOIL_NOINLINE int RECOIL_FASTCALL
GetActivationRecordPackedSize(zEffectAnimActivationRecord *record);
RECOIL_NOINLINE void RECOIL_CDECL DiscardLastActivationRecord();
RECOIL_NOINLINE void RECOIL_FASTCALL SetZbdFilename(const char *filename);
RECOIL_NOINLINE int RECOIL_CDECL LoadZbd();
RECOIL_NOINLINE int RECOIL_CDECL LoadAndInstantiate();
RECOIL_NOINLINE void RECOIL_FASTCALL SetActivationDispatchContext(
    void(RECOIL_FASTCALL *callback)(zEffectAnimActivationRecord *record), int context);
} // namespace zEffect_Anim

namespace zEffectAnim {
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL FindEntryByName(const char *name);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL
FindNextAsyncEntry(zEffectAnimEntry *currentEntry);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
FindNodeRecursiveByName(zClass_NodePartial *rootNode, const char *name);
RECOIL_NOINLINE int RECOIL_FASTCALL FindSoundRefIndexByName(zEffectAnimEntry *self,
                                                                     const char *name);
RECOIL_NOINLINE int RECOIL_FASTCALL FindLightRefIndexByName(zEffectAnimEntry *self,
                                                                     const char *name);
RECOIL_NOINLINE int RECOIL_FASTCALL FindOrCreateSoundRef(zEffectAnimEntry *self,
                                                                  const char *name);
RECOIL_NOINLINE int RECOIL_FASTCALL FindOrCreateLightRef(zEffectAnimEntry *self,
                                                                  const char *name);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL ResolveNodeByName(zEffectAnimEntry *self,
                                                                      const char *name);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL RebindEntryToNode(zEffectAnimEntry *self,
                                                                    zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL EnsureCopiedRootTree(zEffectAnimEntry *self,
                                                                  zClass_NodePartial *sourceRoot);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL CloneEntryForNode(zEffectAnimEntry *self,
                                                                    zClass_NodePartial *node);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL GetRootNodeOrNull(zEffectAnimEntry *self);
RECOIL_NOINLINE void RECOIL_FASTCALL ResetActivationPrereqCount(zEffectAnimEntry *self);
RECOIL_NOINLINE int RECOIL_FASTCALL CheckActivationPrereqs(zEffectAnimEntry *self);
RECOIL_NOINLINE int RECOIL_FASTCALL ResetForNode(zEffectAnimEntry *self);
RECOIL_NOINLINE int RECOIL_FASTCALL Stop(zEffectAnimEntry *self);
RECOIL_NOINLINE int RECOIL_FASTCALL FinalizeStop(zEffectAnimEntry *self);
RECOIL_NOINLINE int RECOIL_FASTCALL RunStopDelayCallback(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL RunStopSequenceCallback(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL StopAndCleanup(zEffectAnimEntry *self,
                                                            zClass_NodePartial *targetNode,
                                                            int immediateCleanup);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL ActivateRuntime(zEffectAnimEntry *self,
                                                                  zClass_NodePartial *targetNode);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetTransformRotAndVelocity(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, float posX, float posY, float posZ,
    float rotX, float rotY, float rotZ, float velocityX, float velocityY, float velocityZ);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetTransformRotAndVelocity_Thunk(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, float posX, float posY, float posZ,
    float rotX, float rotY, float rotZ, float velocityX, float velocityY, float velocityZ);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetVelocity(zEffectAnimEntry *self,
                                                              zClass_NodePartial *boundNode,
                                                              float velocityX, float velocityY,
                                                              float velocityZ);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetVelocity_Thunk(zEffectAnimEntry *self,
                                                                    zClass_NodePartial *boundNode,
                                                                    float velocityX,
                                                                    float velocityY,
                                                                    float velocityZ);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetPositionRefAndVelocity(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNode,
    const zVec3 *refVec, const zVec3 *velocityVec);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetPositionRefAndVelocity_Thunk(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNode,
    const zVec3 *refVec, const zVec3 *velocityVec);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetTransformRefs(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNodeA,
    const zVec3 *refVecA, zClass_NodePartial *refNodeB, const zVec3 *refVecB);
RECOIL_NOINLINE zEffectAnimEntry *RECOIL_FASTCALL SetTransformRefs_Thunk(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNodeA,
    const zVec3 *refVecA, zClass_NodePartial *refNodeB, const zVec3 *refVecB);
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL QueueCmdType1TransformRotVelocity(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, float posX, float posY, float posZ,
    float rotX, float rotY, float rotZ, float velocityX, float velocityY, float velocityZ);
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL
QueueCmdType2Velocity(zEffectAnimEntry *self, zClass_NodePartial *boundNode, float velocityX,
                      float velocityY, float velocityZ);
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL QueueCmdType3PositionRefAndVelocity(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNode,
    const zVec3 *refVec, const zVec3 *velocityVec);
RECOIL_NOINLINE zEffectAnimActivationRecord *RECOIL_FASTCALL QueueCmdType4TransformRefs(
    zEffectAnimEntry *self, zClass_NodePartial *boundNode, zClass_NodePartial *refNodeA,
    const zVec3 *refVecA, zClass_NodePartial *refNodeB, const zVec3 *refVecB);
RECOIL_NOINLINE int RECOIL_FASTCALL ShutdownEntry(zEffectAnimEntry *self);
} // namespace zEffectAnim

namespace zEffect {
RECOIL_NOINLINE int RECOIL_CDECL Init();
RECOIL_NOINLINE int RECOIL_FASTCALL InitFromPath(zClass_NodePartial *worldNode,
                                                          zClass_NodePartial *cameraNode,
                                                          const char *path);
RECOIL_NOINLINE void RECOIL_FASTCALL SetWorldNode(zClass_NodePartial *worldNode);
RECOIL_NOINLINE void RECOIL_FASTCALL SetResourceNode(zClass_NodePartial *resourceNode);
RECOIL_NOINLINE float RECOIL_FASTCALL TickResetDelayOnTimer(zEffectAnimEntry *self, float deltaSec);
RECOIL_NOINLINE int RECOIL_FASTCALL TickResetDelayOnHit(zEffectAnimEntry *self,
                                                                 zClass_NodePartial *hitNode,
                                                                 int unused,
                                                                 float damageAmount);
RECOIL_NOINLINE void RECOIL_FASTCALL SetConditionalRefPos(const zVec3 *position);
RECOIL_NOINLINE void RECOIL_FASTCALL SetConditionalEffectLevel(int level);
RECOIL_NOINLINE int RECOIL_CDECL SetAnimDebugFrameTag();
RECOIL_NOINLINE float RECOIL_FASTCALL GetConditionalRefPosDistanceSq(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL
TraceUpwardHitFromNodeOrPos(zClass_NodePartial *nodeOrNull, const zVec3 *positionOrNull,
                            const float *rayHeight, int *outHit);
RECOIL_NOINLINE void *RECOIL_FASTCALL FindNodeUserDataRecursive(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL SpawnRuntimeInstanceAt(int effectIndex,
                                                                    const zVec3 *worldPos);
RECOIL_NOINLINE int RECOIL_FASTCALL
ActivateRuntimeEntryAtPosition(zEffect_RuntimeEntry *runtimeEntry, const zVec3 *worldPos);
RECOIL_NOINLINE float RECOIL_FASTCALL ComputeDistanceSqToListener(const zVec3 *worldPos);
RECOIL_NOINLINE zEffect_RuntimeEntry *RECOIL_FASTCALL
AcquireRuntimeEntryByIndex(int effectIndex);
RECOIL_NOINLINE zEffect_RuntimeEntry *RECOIL_FASTCALL
CloneRuntimeEntryFromTemplate(int effectIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL FindTemplateIndexByName(const char *name);
RECOIL_NOINLINE int RECOIL_FASTCALL RuntimeNodeActionCallback(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleEffectTemplateOffsetEvent(zEffectAnimEntry *self, zEffectAnimRefOffsetEvent *event);
RECOIL_NOINLINE float RECOIL_FASTCALL UpdateBeamNodeBetweenPoints(zClass_NodePartial *obj3d,
                                                                  const zVec3 *srcPos,
                                                                  const zVec3 *destPos);
RECOIL_NOINLINE float RECOIL_FASTCALL UpdateBeamNodeBetweenFractions(zClass_NodePartial *obj3d,
                                                                     const zVec3 *srcPos, float t0,
                                                                     const zVec3 *destPos,
                                                                     float t1);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleSampleRefOffsetEvent(zEffectAnimEntry *self, zEffectAnimRefOffsetEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleSoundEvent(zEffectAnimEntry *self,
                                                              zEffectAnimSoundEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleLightEvent(zEffectAnimEntry *self,
                                                              zEffectAnimLightEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleLightAnimEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                     zEffectLightRangeSpecularAnimEvent *animEvent);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleFogEvent(zEffectAnimEntry *self,
                                                            zEffectFogEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleCameraParamsEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime, zEffectCameraEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
AnimateCameraParamsOverTime(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                            zEffectCameraAnimEvent *animEvent);
RECOIL_NOINLINE int RECOIL_FASTCALL
FindNearestPickCandidateBelowPoint(const zVec3 *point, zClassDiPickCandidateEntry *outCandidate);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleNodeAnimEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                    zEffectNodeAnimEvent *nodeAnimEvent);
RECOIL_NOINLINE int RECOIL_FASTCALL
AnimateNodeOverTime(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                    zEffectNodeAnimEvent *nodeAnimEvent);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleActivateEvent(zEffectAnimEntry *self,
                                                                 zEffectActivateEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandlePositionEvent(zEffectAnimEntry *self,
                                                                 zEffectTransformEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleNodeScaleEvent(zEffectAnimEntry *self,
                                                                  zEffectNodeScaleEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleRotationEvent(zEffectAnimEntry *self,
                                                                 zEffectTransformEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleAddChildEvent(zEffectAnimEntry *self,
                                                                 zEffectParentChildEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleRemoveChildEvent(zEffectAnimEntry *self,
                                                                    zEffectParentChildEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleAttachEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime, zEffectAttachEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleSurfaceStopEvent(zEffectAnimEntry *self, zEffectSurfaceControlEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleSurfacePlayEvent(zEffectAnimEntry *self, zEffectSurfaceControlEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleSurfaceRefEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *runtime, zEffectSurfaceRefEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleDetachEvent(zEffectAnimEntry *self,
                                                               zEffectAnimSurfaceRuntime *runtime,
                                                               zEffectBeamDetachEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleScreenColorFxEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                         zEffectScreenColorFxEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleScreenOverlayFxEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *sequenceRuntime,
                           zEffectScreenOverlayFxEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleTransformRefsEvent(zEffectAnimEntry *self, zEffectTransformRefsEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleNamedAnimStopEvent(zEffectAnimEntry *self, zEffectAnimEmitterEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleEmitterPlayEvent(zEffectAnimEntry *self,
                                                                    zEffectAnimEmitterEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleConditionalChainEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *runtime, zEffectConditionalEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleEmitterResetEvent(zEffectAnimSurfaceRuntime *runtime);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleEmitterLoopEvent(
    zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *runtime, zEffectAnimLoopEvent *loopEvent);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleEmitterStopEvent(zEffectAnimEntry *self,
                                                                    zEffectAnimEmitterEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
SkipConditionalChainToEnd(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *runtime, void *event);
RECOIL_NOINLINE int RECOIL_FASTCALL
HandleNoOpMarkerEvent(zEffectAnimEntry *self, zEffectAnimSurfaceRuntime *runtime, void *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleCallbackEvent(zEffectAnimEntry *self,
                                                                 zEffectAnimSurfaceRuntime *runtime,
                                                                 zEffectAnimCallbackEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL HandleTopMessageEvent(zEffectAnimEntry *self,
                                                                   zEffectTopMessageEvent *event);
RECOIL_NOINLINE int RECOIL_FASTCALL CleanupLightRefs(zEffectAnimEntry *self);
RECOIL_NOINLINE int RECOIL_FASTCALL CleanupSoundRefs(zEffectAnimEntry *self);
RECOIL_NOINLINE int RECOIL_CDECL Reset();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownAll();
} // namespace zEffect
