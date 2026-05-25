#ifndef RECOIL_GAMEZRECOIL_INCLUDE_ZCLASS_H
#define RECOIL_GAMEZRECOIL_INCLUDE_ZCLASS_H

#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"
#include <stdio.h>

#include "recoil/recoil_callconv.h"

struct zBBoxCorners;
struct zDiPartial;
struct zClass_LightDataPartial;
struct zClass_SoundDataPartial;
struct zSndPlayHandle;
struct zSndSample;

struct zClass_NodePartial {
    char name[0x24];
    int flags;
    int auxFlags;
    int boundsFlags;
    unsigned char nodeType;
    unsigned char unknown_31[0x03];
    int classId;
    void *classData;
    unsigned int userDataOrDiRef;
    zClass_NodePartial *callbackContext;
    int callbackPriority;
    void *actionCallback;
    int gridCol;
    int gridRow;
    int listCountA;
    zClass_NodePartial **listA;
    int listCountB;
    zClass_NodePartial **listB;
    float cachedSphereCenter[4];
    float cachedBounds[6];
};

struct zClass_TypeListLink {
    zClass_NodePartial *node;
    zClass_TypeListLink *prev;
    zClass_TypeListLink *next;
    int pendingRemove;
};

struct zClass_TypeListBucket {
    zClass_TypeListLink *head;
    zClass_TypeListLink *tail;
    int pendingRemovalDirty;
};

typedef int (RECOIL_FASTCALL *zClass_NodePredicate)(zClass_NodePartial *node);
typedef int (RECOIL_FASTCALL *zClass_NodeActionCallback)(zClass_NodePartial *node);

struct zClass_NodeFreeListSlot {
    zClass_NodePartial node;
    unsigned char unknown_8c[0x30];
    void *damageHandler;
    unsigned int freeTag;
};

struct OptCatalogDamageHandlerPartial {
    void *hitCallback;
    void *hitContext;
    void *timerCallback;
    void *timerContext;
};

struct zVec3 {
    float x;
    float y;
    float z;
};

inline zVec3 zVec3_Make(float x, float y, float z) {
    zVec3 value = {x, y, z};
    return value;
}

struct zClass_WindowClearPoly {
    zVec3 vertices[4];
    int vertCount;
};

struct zClass_WindowDataPartial {
    int viewportWidth;
    int viewportHeight;
    int resolutionWidth;
    int resolutionHeight;
    zClass_WindowClearPoly clearPolys[4];
    int clearPolyIndexFlags;
    int bufferIndex;
    void *buffer;
    int fbWidth;
    int fbHeight;
    int fbBpp;
};

struct zClass_DisplayDataPartial {
    int x;
    int y;
    int width;
    int height;
    float backgroundR;
    float backgroundG;
    float backgroundB;
};

struct zColorRgb {
    float red;
    float green;
    float blue;
};

struct zWorldAreaPartial {
    int areaFlags;
    int areaIndex;
    float cellMinX;
    float cellMinZ;
    float bbox[6];
    zVec3 bboxCenter;
    float bboxRadius;
    unsigned char unknown_38;
    unsigned char displayRefreshQueued;
    short childCount;
    zClass_NodePartial **childList;
};

struct zClass_WorldDataPartial {
    int flags;
    int pendingAreaUpdateCount;
    int pendingAreaUpdateCapacity;
    zWorldAreaPartial **pendingAreaUpdates;
    int fogState;
    zColorRgb ambientColor;
    float fogDistanceStart;
    float fogDistanceEnd;
    float fogHeightHigh;
    float fogHeightLow;
    float fogDensity;
    float originX;
    float originZ;
    float worldSizeX;
    float worldSizeZ;
    float worldMaxX;
    float worldMaxZ;
    unsigned char partitionMaxDecFeatureCount;
    unsigned char unknown_4d[0x03];
    int clampQueriesToBounds;
    float areaCellSizeX;
    float areaCellSizeZ;
    float areaHalfSizeX;
    float areaHalfSizeZ;
    float areaInvSizeX;
    float areaInvSizeZ;
    float areaCellRadiusBias;
    float partitionInclusionTolX;
    float partitionInclusionTolZ;
    int areaGridColCount;
    int areaGridRowCount;
    zWorldAreaPartial **areaGridRows;
    float scaleX;
    float scaleY;
    float scaleZ;
    int lightCount;
    zClass_NodePartial **lightNodes;
    zClass_LightDataPartial **lightDataList;
    int soundCount;
    zClass_NodePartial **soundNodes;
    zClass_SoundDataPartial **soundDataList;
    int areaGridExternalOwnership;
};

struct zClass_SoundDataPartial {
    zSndSample *sample;
    zSndPlayHandle *playHandle;
    char sampleSetName[0x24];
    int runtimeFlags;
    zVec3 localPosition;
    zVec3 worldPos;
    unsigned char unknown_48[0x30];
    int falloffMode;
    float rangeMin;
    float rangeMax;
    float rangeMaxSq;
    float invRangeSpan;
    int attachedWorldCount;
    zClass_NodePartial **attachedWorlds;
};

struct zClass_SequenceEntryPartial {
    zClass_NodePartial *node;
    float triggerTime;
};

struct zClass_SequenceDataPartial {
    int isActive;
    int repeatAtBounds;
    int wrapAtBounds;
    int isPaused;
    int step;
    int currentIndex;
    float currentTime;
    int entryCount;
    zClass_SequenceEntryPartial entries[1];
};

struct zClass_SwitchDataPartial {
    int activeMaskIndex;
    unsigned int childMasks[1];
};

struct zTag4Partial {
    unsigned char count;
    unsigned char tags[3];
};

struct zClass_Object3DDataPartial {
    int flags;
    float alphaScale;
    zColorRgb color;
    float colorAlpha;
    zVec3 rotation;
    zVec3 scale;
    float localMatrix[12];
    float cachedWorldMatrix[12];
};

struct zClass_LodDataPartial {
    int computeOwnDistance;
    float nearRangeSq;
    float nearRange;
    float farRangeSq;
    zVec3 fadeWidth;
    zVec3 fadeAmount;
    zVec3 fadeEndScale;
    float fogFadeWidth;
    float fogFadeAmount;
    float fogStartDist;
    float vertexShadingAmount;
    int active;
    zClass_NodePartial *rangeNode;
    float rangeSq;
};

struct zClass_LightDataPartial {
    int dirty;
    int enabled;
    zVec3 localRotation;
    zVec3 localPosition;
    zVec3 worldRotation;
    zVec3 worldPosition;
    float savedParentMatrix[12];
    zVec3 worldPosScratch;
    zVec3 velocity;
    zVec3 viewPos;
    zVec3 worldDir;
    zVec3 viewDir;
    float falloff;
    float intensityScale;
    zColorRgb specularColor;
    float coneAngle;
    int isPointMode;
    int isDirectionalMode;
    int lightParam;
    int lightSubMode;
    float range1;
    float range2;
    float range2Sq;
    float invRangeDelta;
    int attachedWorldCount;
    zClass_NodePartial **attachedWorlds;
};

struct zClass_AnimateKeyframePartial {
    zVec3 rotation;
    zVec3 position;
    zVec3 scale;
};

struct zClass_AnimateRuntimePartial {
    unsigned char unknown_00[0x04];
    zClass_AnimateKeyframePartial *keyframes;
    zVec3 sampledRotation;
    zVec3 sampledPosition;
    zVec3 sampledScale;
    zVec3 outputRotationScale;
    zVec3 outputPositionScale;
    zVec3 outputScaleScale;
    float duration;
    unsigned char unknown_54[0x04];
    float currentTime;
    float loopBase;
    float startTime;
    short state;
    short unknown_66;
    short maxFrameIndex;
    short loopCount;
};

struct zClass_AnimateDataPartial {
    int flags;
    int statusFlags;
    float animatedTransform[12];
    float savedParentMatrix[12];
    zClass_AnimateRuntimePartial runtime;
};

struct zClass_CameraDataPartial {
    zClass_NodePartial *worldNode;
    zClass_NodePartial *windowNode;
    zClass_NodePartial *horizonNode;
    zClass_NodePartial *horizonXZNode;
    int cameraFlags;
    zVec3 targetOrEuler;
    zVec3 posOffset;
    zVec3 cameraPos;
    zVec3 eulerAngles;
    float worldTransform[12];
    zVec3 forwardDir;
    unsigned char unknown_80[0x24];
    zVec3 worldTarget;
    float nearClip;
    float farClip;
    zVec3 nearClipCenter;
    zVec3 farClipCenter;
    float clipDistance;
    float invClipDistanceSq;
    float viewportWidth;
    float viewportHeight;
    float frustumWidth;
    float frustumHeight;
    float fovX;
    float fovY;
    float frustumYaw;
    float frustumPitch;
    int frustumVectorsDirty;
    zVec3 frustumOrigin;
    zVec3 frustumCorners[4];
    int localFrustumNormalsDirty;
    zVec3 localFrustumLeftNormal;
    zVec3 localFrustumRightNormal;
    zVec3 localFrustumBottomNormal;
    zVec3 localFrustumTopNormal;
    zVec3 localFrustumNearNormal;
    zVec3 localFrustumFarNormal;
    int transformDirty;
    zVec3 worldFrustumNormals[6];
    unsigned char unknown_1d0[0x04];
    float viewportScaleX;
    float viewportScaleY;
    unsigned char unknown_1dc[0x04];
    int variantOverrideEnabled;
    zTag4Partial variantTag;
};

struct zClass_ZbdHeader {
    int magic;
    int version;
    int texDirArg;
    int texDirOffset;
    int matlOffset;
    int model3dOffset;
    int nodeCount;
    int nodeFreeHead;
    int nodeTableOffset;
};

struct zClass_RenderColorAlphaState {
    zColorRgb color;
    float alpha;
};

struct zClass_LodDistanceState {
    zVec3 center;
    float distanceSq;
};

typedef void (RECOIL_FASTCALL *zClass_RenderFn)(zClass_NodePartial *node, int clipMask);

RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, name) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, flags) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, auxFlags) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, boundsFlags) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, classId) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, nodeType) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, classData) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, userDataOrDiRef) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, callbackContext) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, callbackPriority) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, actionCallback) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, gridCol) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, gridRow) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, listCountA) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, listA) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, listCountB) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, listB) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodePartial, cachedBounds) == 0x74);
RECOIL_STATIC_ASSERT(sizeof(zClass_NodePartial) == 0x8c);
RECOIL_STATIC_ASSERT(sizeof(zClass_TypeListLink) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_TypeListBucket, head) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_TypeListBucket, tail) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_TypeListBucket, pendingRemovalDirty) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zClass_TypeListBucket) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodeFreeListSlot, damageHandler) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zClass_NodeFreeListSlot, freeTag) == 0xc0);
RECOIL_STATIC_ASSERT(sizeof(zClass_NodeFreeListSlot) == 0xc4);
RECOIL_STATIC_ASSERT(sizeof(zClass_ZbdHeader) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zClass_ZbdHeader, texDirArg) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_ZbdHeader, texDirOffset) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_ZbdHeader, matlOffset) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_ZbdHeader, model3dOffset) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClass_ZbdHeader, nodeCount) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zClass_ZbdHeader, nodeFreeHead) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zClass_ZbdHeader, nodeTableOffset) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogDamageHandlerPartial, hitCallback) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogDamageHandlerPartial, hitContext) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogDamageHandlerPartial, timerCallback) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogDamageHandlerPartial, timerContext) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogDamageHandlerPartial) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowClearPoly, vertices) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowClearPoly, vertCount) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zClass_WindowClearPoly) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3DDataPartial, flags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3DDataPartial, alphaScale) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3DDataPartial, color) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3DDataPartial, colorAlpha) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3DDataPartial, rotation) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3DDataPartial, scale) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3DDataPartial, localMatrix) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3DDataPartial, cachedWorldMatrix) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(zClass_Object3DDataPartial) == 0x90);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateDataPartial, statusFlags) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateDataPartial, animatedTransform) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateDataPartial, savedParentMatrix) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateDataPartial, runtime) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, keyframes) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, sampledRotation) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, sampledPosition) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, sampledScale) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, outputRotationScale) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, outputPositionScale) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, outputScaleScale) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, duration) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, currentTime) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, loopBase) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, startTime) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, state) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, maxFrameIndex) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateRuntimePartial, loopCount) == 0x6a);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateKeyframePartial, rotation) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateKeyframePartial, position) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateKeyframePartial, scale) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zClass_AnimateKeyframePartial) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zClass_AnimateRuntimePartial) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateDataPartial, runtime) +
                  offsetof(zClass_AnimateRuntimePartial, sampledRotation) ==
              0x70);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateDataPartial, runtime) +
                  offsetof(zClass_AnimateRuntimePartial, sampledPosition) ==
              0x7c);
RECOIL_STATIC_ASSERT(offsetof(zClass_AnimateDataPartial, runtime) +
                  offsetof(zClass_AnimateRuntimePartial, sampledScale) ==
              0x88);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, viewportWidth) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, viewportHeight) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, resolutionWidth) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, resolutionHeight) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, clearPolys) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, clearPolyIndexFlags) == 0xe0);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, bufferIndex) == 0xe4);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, buffer) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, fbWidth) == 0xec);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, fbHeight) == 0xf0);
RECOIL_STATIC_ASSERT(offsetof(zClass_WindowDataPartial, fbBpp) == 0xf4);
RECOIL_STATIC_ASSERT(sizeof(zClass_WindowDataPartial) == 0xf8);
RECOIL_STATIC_ASSERT(offsetof(zClass_DisplayDataPartial, x) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_DisplayDataPartial, y) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_DisplayDataPartial, width) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_DisplayDataPartial, height) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_DisplayDataPartial, backgroundR) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_DisplayDataPartial, backgroundG) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClass_DisplayDataPartial, backgroundB) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zClass_DisplayDataPartial) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zWorldAreaPartial, cellMinX) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zWorldAreaPartial, cellMinZ) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zWorldAreaPartial, bbox) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zWorldAreaPartial, bboxCenter) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zWorldAreaPartial, bboxRadius) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zWorldAreaPartial, displayRefreshQueued) == 0x39);
RECOIL_STATIC_ASSERT(offsetof(zWorldAreaPartial, childCount) == 0x3a);
RECOIL_STATIC_ASSERT(offsetof(zWorldAreaPartial, childList) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(zWorldAreaPartial) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, pendingAreaUpdateCount) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, pendingAreaUpdateCapacity) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, pendingAreaUpdates) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, fogState) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, ambientColor) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, originX) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, originZ) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, worldMaxX) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, worldMaxZ) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, partitionMaxDecFeatureCount) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, clampQueriesToBounds) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaCellSizeX) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaCellSizeZ) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaHalfSizeX) == 0x5c);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaHalfSizeZ) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaInvSizeX) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaInvSizeZ) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaCellRadiusBias) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, partitionInclusionTolX) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, partitionInclusionTolZ) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaGridColCount) == 0x78);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaGridRowCount) == 0x7c);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaGridRows) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, scaleX) == 0x84);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, scaleY) == 0x88);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, scaleZ) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, lightCount) == 0x90);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, lightNodes) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, lightDataList) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, soundCount) == 0x9c);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, soundNodes) == 0xa0);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, soundDataList) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zClass_WorldDataPartial, areaGridExternalOwnership) == 0xa8);
RECOIL_STATIC_ASSERT(sizeof(zClass_WorldDataPartial) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, sample) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, playHandle) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, sampleSetName) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, runtimeFlags) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, localPosition) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, worldPos) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, falloffMode) == 0x78);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, rangeMin) == 0x7c);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, rangeMax) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, rangeMaxSq) == 0x84);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, invRangeSpan) == 0x88);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, attachedWorldCount) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(zClass_SoundDataPartial, attachedWorlds) == 0x90);
RECOIL_STATIC_ASSERT(sizeof(zClass_SoundDataPartial) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceDataPartial, isActive) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceDataPartial, repeatAtBounds) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceDataPartial, wrapAtBounds) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceDataPartial, isPaused) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceDataPartial, step) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceDataPartial, currentIndex) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceDataPartial, currentTime) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceDataPartial, entryCount) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceDataPartial, entries) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceEntryPartial, node) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_SequenceEntryPartial, triggerTime) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(zClass_SequenceEntryPartial) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_SwitchDataPartial, activeMaskIndex) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_SwitchDataPartial, childMasks) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(zClass_RenderColorAlphaState) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zClass_LodDistanceState) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, computeOwnDistance) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, nearRangeSq) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, nearRange) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, farRangeSq) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, fadeWidth) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, fadeAmount) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, fadeEndScale) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, fogFadeWidth) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, vertexShadingAmount) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, active) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, rangeNode) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(zClass_LodDataPartial, rangeSq) == 0x4c);
RECOIL_STATIC_ASSERT(sizeof(zClass_LodDataPartial) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, worldPosScratch) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, viewPos) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, localRotation) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, localPosition) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, worldRotation) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, worldPosition) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, savedParentMatrix) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, worldDir) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, falloff) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, intensityScale) == 0xa8);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, specularColor) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, coneAngle) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, isPointMode) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, isDirectionalMode) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, lightParam) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, lightSubMode) == 0xc8);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, range1) == 0xcc);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, range2) == 0xd0);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, range2Sq) == 0xd4);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, invRangeDelta) == 0xd8);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, attachedWorldCount) == 0xdc);
RECOIL_STATIC_ASSERT(offsetof(zClass_LightDataPartial, attachedWorlds) == 0xe0);
RECOIL_STATIC_ASSERT(sizeof(zClass_LightDataPartial) == 0xe4);
RECOIL_STATIC_ASSERT(sizeof(zTag4Partial) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zWorldAreaPartial, areaIndex) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, worldNode) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, windowNode) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, horizonNode) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, horizonXZNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, cameraFlags) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, targetOrEuler) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, posOffset) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, cameraPos) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, eulerAngles) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, worldTransform) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, forwardDir) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, worldTarget) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, nearClip) == 0xb0);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, farClip) == 0xb4);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, nearClipCenter) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, farClipCenter) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, clipDistance) == 0xd0);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, invClipDistanceSq) == 0xd4);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, viewportWidth) == 0xd8);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, viewportHeight) == 0xdc);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, frustumWidth) == 0xe0);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, frustumHeight) == 0xe4);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, fovX) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, fovY) == 0xec);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, frustumYaw) == 0xf0);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, frustumPitch) == 0xf4);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, frustumVectorsDirty) == 0xf8);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, frustumOrigin) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, frustumCorners) == 0x108);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, localFrustumNormalsDirty) == 0x138);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, localFrustumLeftNormal) == 0x13c);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, localFrustumRightNormal) == 0x148);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, localFrustumBottomNormal) == 0x154);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, localFrustumTopNormal) == 0x160);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, localFrustumNearNormal) == 0x16c);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, localFrustumFarNormal) == 0x178);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, transformDirty) == 0x184);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, worldFrustumNormals) == 0x188);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, viewportScaleX) == 0x1d4);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, viewportScaleY) == 0x1d8);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, variantOverrideEnabled) == 0x1e0);
RECOIL_STATIC_ASSERT(offsetof(zClass_CameraDataPartial, variantTag) == 0x1e4);
RECOIL_STATIC_ASSERT(sizeof(zClass_CameraDataPartial) == 0x1e8);

struct zCamera_FrustumGridTilePartial {
    int col;
    int row;
    int hasPosOffset;
    float posOffsetX;
    float posOffsetZ;
    int clipMask;
};

struct zCamera_FrustumGridTileRingPartial {
    zCamera_FrustumGridTilePartial tiles[30];
    int count;
};

RECOIL_STATIC_ASSERT(offsetof(zCamera_FrustumGridTilePartial, col) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zCamera_FrustumGridTilePartial, row) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zCamera_FrustumGridTilePartial, hasPosOffset) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zCamera_FrustumGridTilePartial, posOffsetX) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zCamera_FrustumGridTilePartial, posOffsetZ) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zCamera_FrustumGridTilePartial, clipMask) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zCamera_FrustumGridTilePartial) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zCamera_FrustumGridTileRingPartial, count) == 0x2d0);
RECOIL_STATIC_ASSERT(sizeof(zCamera_FrustumGridTileRingPartial) == 0x2d4);

extern "C" {
extern zVec3 g_zCamera_FrustumFootprintPoints[9];
extern int g_zCamera_FrustumFootprintPointCount;
extern zCamera_FrustumGridTileRingPartial g_zCamera_FrustumGridTileRings[50];
extern int gModel_ClipMaskStack[0x40];
extern int *gModel_ClipMaskStackTop;
extern zClass_RenderFn gModel_RenderFn;
extern int g_zClass_RenderBoundsContextActive;
extern int g_zClass_RenderFrustumGridTileIndex;
extern int g_zClass_RenderRangeFadeActive;
extern float g_zClass_RenderRangeFadeScale;
extern int g_zClass_RenderVertexAlphaOverrideActive;
extern int g_zClass_RenderAlphaScaleStackTop;
extern float g_zClass_RenderAlphaScaleStack[0x20];
extern int g_zClass_SoftwarePathStateStackTop;
extern zClass_RenderColorAlphaState g_zClass_SoftwarePathRenderStateStack[0x20];
extern int g_zClass_LodDistanceStateStackTop;
extern zClass_LodDistanceState g_zClass_LodDistanceStateStack[0x20];
}

RECOIL_FORCEINLINE zVec3 *zClass_NodeViewSphereCenter(zClass_NodePartial *node) {
    return (zVec3 *)((unsigned char *)node + 0x8c);
}

RECOIL_FORCEINLINE const zVec3 *zClass_NodeViewSphereCenter(const zClass_NodePartial *node) {
    return (const zVec3 *)((const unsigned char *)node + 0x8c);
}

RECOIL_FORCEINLINE float *zClass_NodeViewSphereRadius(zClass_NodePartial *node) {
    return (float *)((unsigned char *)node + 0x98);
}

RECOIL_FORCEINLINE const float *zClass_NodeViewSphereRadius(const zClass_NodePartial *node) {
    return (const float *)((const unsigned char *)node + 0x98);
}

namespace BBox {
RECOIL_NOINLINE void RECOIL_FASTCALL CornersToBoundingSphere(zBBoxCorners *corners,
                                                            zVec3 *outCenter, float *outRadius);
}

namespace zTag4 {
RECOIL_NOINLINE void RECOIL_FASTCALL Clear(zTag4Partial *tag);
}

namespace zClass_Window {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwWindowNew();
RECOIL_NOINLINE int RECOIL_FASTCALL gwWindowSetResolution(zClass_NodePartial *node,
                                                                   int width,
                                                                   int height);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWindowGetResolution(zClass_NodePartial *node,
                                                                   int *outWidth,
                                                                   int *outHeight);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWindowSetSize(zClass_NodePartial *node,
                                                             int width,
                                                             int height);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWindowGetSize(zClass_NodePartial *node,
                                                             int *outWidth,
                                                             int *outHeight);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWindowSetBuffer(zClass_NodePartial *node,
                                                               int bufferIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWindowSetClearPolygon(zClass_NodePartial *node,
                                                                     int enabled);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWindowAddClearPolygonVertex(zClass_NodePartial *node,
                                                                           const zVec3 *point);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWindowCloseClearPolygon(zClass_NodePartial *node);
} // namespace zClass_Window

namespace zClass_Display {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwDisplayInit();
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial *parent,
                                                         zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL gwDisplaySetSize(zClass_NodePartial *node,
                                                              int width,
                                                              int height);
RECOIL_NOINLINE int RECOIL_FASTCALL gwDisplaySetPosition(zClass_NodePartial *node,
                                                                  int x, int y);
RECOIL_NOINLINE int RECOIL_FASTCALL gwDisplaySetBackgroundColor(zClass_NodePartial *node,
                                                                         float red, float green,
                                                                         float blue);
} // namespace zClass_Display

namespace zClass_World {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwWorldNew();
RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNode(zClass_NodePartial *world);
RECOIL_NOINLINE int RECOIL_FASTCALL FreeVirtualAreaPartitions(zClass_NodePartial *world);
RECOIL_NOINLINE int RECOIL_FASTCALL QueueAreaUpdate(zClass_NodePartial *world,
                                                             zClass_WorldDataPartial *worldData,
                                                             zWorldAreaPartial *area);
RECOIL_NOINLINE int RECOIL_FASTCALL RebuildAreaBounds(zClass_WorldDataPartial *worldData,
                                                               zWorldAreaPartial *area);
RECOIL_NOINLINE int RECOIL_FASTCALL ApplyPendingFogSettings(zClass_NodePartial *world);
RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogState(zClass_NodePartial *world,
                                                                int fogState);
RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogColorRgb01(zClass_NodePartial *world,
                                                                     float red, float green,
                                                                     float blue);
RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogAltitudeRange(zClass_NodePartial *world,
                                                                        float minAlt, float maxAlt);
RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogRange(zClass_NodePartial *world,
                                                                float nearRange, float farRange);
RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogDensity(zClass_NodePartial *world,
                                                                  float density);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWorldSetOrigin(zClass_NodePartial *world,
                                                              float originX, float originZ);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWorldSetSize(zClass_NodePartial *world,
                                                            float sizeX, float sizeZ);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWorldSetPartitionInclusionTolerance(
    zClass_NodePartial *world, float toleranceX, float toleranceZ);
RECOIL_NOINLINE int RECOIL_FASTCALL gwWorldSetMaxDecFeatures(zClass_NodePartial *world,
                                                                      int maxFeatures);
RECOIL_NOINLINE int RECOIL_FASTCALL WorldRectToGridIndex(zClass_NodePartial *world,
                                                                  int *outGridCol,
                                                                  float minX, float maxX,
                                                                  float minZ, float maxZ,
                                                                  int *outGridRow);
RECOIL_NOINLINE int RECOIL_FASTCALL WorldToGridCoordsClampedEx(
    zClass_NodePartial *world, int *outGridCol, float worldX, float worldZ,
    int *outGridRow, int *clampedGridColOut,
    int *clampedGridRowOut, int *insideBoundsOut);
RECOIL_NOINLINE int RECOIL_FASTCALL WorldToGridCoordsClamped(zClass_NodePartial *world,
                                                                      int *outGridCol,
                                                                      float worldX, float worldZ,
                                                                      int *outGridRow);
RECOIL_NOINLINE zWorldAreaPartial *RECOIL_FASTCALL GetAreaPartitionAtGrid(zClass_NodePartial *world,
                                                                          int gridCol,
                                                                          int gridRow);
RECOIL_NOINLINE int RECOIL_FASTCALL AddChildAtGrid(zClass_NodePartial *world,
                                                            zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL EnsureGridCellDisplayPosition(
    zClass_NodePartial *world, int gridCol, int gridRow);
RECOIL_NOINLINE int RECOIL_FASTCALL AddChildToGridCell(zClass_NodePartial *world,
                                                                zClass_NodePartial *child,
                                                                int gridCol,
                                                                int gridRow);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChildAtGrid(zClass_NodePartial *world,
                                                               zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL AddLight(zClass_NodePartial *world,
                                                      zClass_NodePartial *light);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveLight(zClass_NodePartial *world,
                                                         zClass_NodePartial *light);
RECOIL_NOINLINE int RECOIL_FASTCALL InitLightPointInPolygonXZ(
    zClass_NodePartial *world);
RECOIL_NOINLINE int RECOIL_FASTCALL UpdateAllLights(zClass_NodePartial *world);
RECOIL_NOINLINE int RECOIL_FASTCALL AddSound(zClass_NodePartial *world,
                                                      zClass_NodePartial *sound);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveSound(zClass_NodePartial *world,
                                                         zClass_NodePartial *sound);
RECOIL_NOINLINE int RECOIL_FASTCALL UpdateAllSounds(zClass_NodePartial *world);
} // namespace zClass_World

namespace zClass_Object3D {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwObject3DInit();
RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(zClass_NodePartial *node,
                                                            int siblingCountHint);
RECOIL_NOINLINE int RECOIL_FASTCALL PropagateTransformDirty(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DSetVisibleFlag(zClass_NodePartial *node,
                                                                      int visible);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DSetColorAlpha(zClass_NodePartial *node,
                                                                     zColorRgb *color, float alpha);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DSetAlphaScale(zClass_NodePartial *node,
                                                                     float alphaScale);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DGetAlphaScale(zClass_NodePartial *node,
                                                                     float *outAlphaScale);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DSetLitFlag(zClass_NodePartial *node,
                                                                  int lit);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DSetScale(zClass_NodePartial *node, float x,
                                                                float y, float z);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DGetScale(zClass_NodePartial *node,
                                                                float *outX, float *outY,
                                                                float *outZ);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DGetRotation(zClass_NodePartial *node,
                                                                   float *outX, float *outY,
                                                                   float *outZ);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DSetRotation(zClass_NodePartial *node,
                                                                   float x, float y, float z);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DTranslateRotation(zClass_NodePartial *node,
                                                                         float dx, float dy,
                                                                         float dz);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DGetPosition(zClass_NodePartial *node,
                                                                   float *outX, float *outY,
                                                                   float *outZ);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DSetPosition(zClass_NodePartial *node,
                                                                   float x, float y, float z);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DTranslatePosition(zClass_NodePartial *node,
                                                                         float dx, float dy,
                                                                         float dz);
RECOIL_NOINLINE float *RECOIL_FASTCALL gwObject3DGetMatrixPtr(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DSetMatrix(zClass_NodePartial *node,
                                                                 float *matrix);
RECOIL_NOINLINE int RECOIL_FASTCALL gwObject3DAddChild(zClass_NodePartial *parent,
                                                                zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial *parent,
                                                         zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNode(zClass_NodePartial *node);
} // namespace zClass_Object3D

struct zClass_Object3D_ModelRefLerpTask {
    zClass_NodePartial *node;
    void *callbackCtx;
    void *onComplete;
    int invertModelRef;
    float targetModelRef;
    float currentModelRef;
    float modelRefDeltaPerSec;
    zClass_Object3D_ModelRefLerpTask *next;
};
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3D_ModelRefLerpTask, callbackCtx) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3D_ModelRefLerpTask, onComplete) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3D_ModelRefLerpTask, invertModelRef) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3D_ModelRefLerpTask, targetModelRef) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3D_ModelRefLerpTask, currentModelRef) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3D_ModelRefLerpTask, modelRefDeltaPerSec) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zClass_Object3D_ModelRefLerpTask, next) == 0x1c);
RECOIL_STATIC_ASSERT(sizeof(zClass_Object3D_ModelRefLerpTask) == 0x20);

struct zClass_Object3D_ModelRefLerpQueueState {
    unsigned int listAux;
    zClass_Object3D_ModelRefLerpTask *head;
    zClass_Object3D_ModelRefLerpTask *tail;
    unsigned int count;
};
RECOIL_STATIC_ASSERT(sizeof(zClass_Object3D_ModelRefLerpQueueState) == 0x10);

extern "C" {
extern zClass_Object3D_ModelRefLerpQueueState g_ModelRefLerpQueueState;
}

namespace zClass_Object3D_ModelRefLerpQueue {
RECOIL_NOINLINE void RECOIL_FASTCALL Add(zClass_NodePartial *node, void *callbackCtx,
                                                  void *onComplete, float startModelRef,
                                                  float targetModelRef, float durationSec);
RECOIL_NOINLINE void RECOIL_CDECL Reset();
}

namespace zClass_Lod {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwLodNew();
RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(zClass_NodePartial *node,
                                                            int siblingCountHint);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLodAddChild(zClass_NodePartial *parent,
                                                           zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial *parent,
                                                         zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL SetComputeOwnDistance(zClass_NodePartial *node,
                                                                   int enabled);
RECOIL_NOINLINE int RECOIL_FASTCALL SetTargetNodeAndRange(zClass_NodePartial *node,
                                                                   zClass_NodePartial *target,
                                                                   float range);
} // namespace zClass_Lod

namespace zClass_Light {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwLightNew();
RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(zClass_NodePartial *node,
                                                            int siblingCountHint);
RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNode(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial *parent,
                                                         zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetIntensity(zClass_NodePartial *node,
                                                                 float intensity);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetFalloff(zClass_NodePartial *node,
                                                               float falloff);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetConeAngle(zClass_NodePartial *node,
                                                                 unsigned int coneAngleBits);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetPointMode(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetDirectionalMode(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetParam(zClass_NodePartial *node,
                                                             int param);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetRange(zClass_NodePartial *node, float rangeA,
                                                             float rangeB);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightGetRange(zClass_NodePartial *node,
                                                             float *outRange1, float *outRange2);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetPosition(zClass_NodePartial *node, float x,
                                                                float y, float z);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetRotation(zClass_NodePartial *node, float x,
                                                                float y, float z);
RECOIL_NOINLINE int RECOIL_FASTCALL ComputeWorldTransform(zClass_NodePartial *node,
                                                                   zClass_LightDataPartial *data);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightUpdate(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightGetSpecularColor(zClass_NodePartial *node,
                                                                     float *outRed, float *outGreen,
                                                                     float *outBlue);
RECOIL_NOINLINE int RECOIL_FASTCALL gwLightSetSpecularColor(zClass_NodePartial *node,
                                                                     float red, float green,
                                                                     float blue);
} // namespace zClass_Light

namespace zClass_Camera {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwCameraNew();
RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(zClass_NodePartial *node,
                                                            int siblingCountHint);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraAddChild(zClass_NodePartial *parent,
                                                              zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraRemoveChild(zClass_NodePartial *parent,
                                                                 zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetFlagBit0(zClass_NodePartial *node,
                                                                 int enabled);
RECOIL_NOINLINE int RECOIL_FASTCALL SetTargetNode(zClass_NodePartial *target);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL SetActiveCamera(zClass_NodePartial *camera);
RECOIL_NOINLINE int RECOIL_FASTCALL SetObjectHseTestEnabled(int enabled);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetWorld(zClass_NodePartial *camera,
                                                              zClass_NodePartial *world);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL gwCameraGetWorld(zClass_NodePartial *camera);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetWindow(zClass_NodePartial *camera,
                                                               zClass_NodePartial *window);
RECOIL_NOINLINE int RECOIL_FASTCALL ActivateChildren(zClass_NodePartial *camera,
                                                              zClass_CameraDataPartial *data);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetPosition(zClass_NodePartial *camera,
                                                                 float x, float y, float z);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraTranslate(zClass_NodePartial *camera, float dx,
                                                               float dy, float dz);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetPosition(zClass_NodePartial *camera,
                                                                 float *outX, float *outY,
                                                                 float *outZ);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetTarget(zClass_NodePartial *camera, float x,
                                                               float y, float z);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraTranslateTarget(zClass_NodePartial *camera,
                                                                     float dx, float dy, float dz);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetTarget(zClass_NodePartial *camera,
                                                               float *outX, float *outY,
                                                               float *outZ);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetNearFarClip(zClass_NodePartial *camera,
                                                                    float nearClip, float farClip);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetNearFarClip(zClass_NodePartial *camera,
                                                                    float *outNear, float *outFar);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetViewport(zClass_NodePartial *camera,
                                                                 float viewportWidth,
                                                                 float viewportHeight);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetViewport(zClass_NodePartial *camera,
                                                                 float *outWidth, float *outHeight);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetFOV(zClass_NodePartial *camera,
                                                            float *outFovX, float *outFovY);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetFOV(zClass_NodePartial *camera, float fovX,
                                                            float fovY);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraGetClipDistance(zClass_NodePartial *camera,
                                                                     float *outClipDistance);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetClipDistance(zClass_NodePartial *camera,
                                                                     float clipDistance);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraSetHorizon(zClass_NodePartial *camera,
                                                                zClass_NodePartial *horizonNode);
RECOIL_NOINLINE int RECOIL_FASTCALL
gwCameraSetHorizonXZ(zClass_NodePartial *camera, zClass_NodePartial *horizonXZNode);
RECOIL_NOINLINE void RECOIL_FASTCALL SetViewDistance(int enableAutoClip, float distance);
RECOIL_NOINLINE float RECOIL_FASTCALL FastAngleXZ(zVec3 *point1, zVec3 *point2);
RECOIL_NOINLINE int RECOIL_FASTCALL FindConvexHullXZ(zVec3 *points, int count);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildFrustumGridTiles(
    zClass_NodePartial *world, zClass_WorldDataPartial *worldData,
    zClass_CameraDataPartial *cameraData);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildFrustumGridTilesFromParams(
    zClass_NodePartial *world, zClass_WorldDataPartial *worldData,
    zClass_CameraDataPartial *cameraData);
RECOIL_NOINLINE int RECOIL_FASTCALL RenderFrustumGridTiles(
    zClass_NodePartial *world, zClass_NodePartial *camera, zClass_CameraDataPartial *cameraData);
RECOIL_NOINLINE void RECOIL_FASTCALL RenderOverlayNodes(zClass_NodePartial *world);
RECOIL_NOINLINE void RECOIL_FASTCALL RenderWorld(zClass_NodePartial *world,
                                                 zClass_NodePartial *camera,
                                                 zClass_CameraDataPartial *cameraData);
RECOIL_NOINLINE int RECOIL_FASTCALL
gwCameraSetVariantTagOverride(zClass_NodePartial *camera, zTag4Partial *variantTag);
RECOIL_NOINLINE int RECOIL_FASTCALL RenderScene(zClass_NodePartial *camera,
                                                         int updateFxPass3Local);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildWorldTransform(
    zClass_NodePartial *camera, zClass_CameraDataPartial *data, zVec3 *posOffset);
RECOIL_NOINLINE int RECOIL_FASTCALL UpdateImpl(zClass_NodePartial *camera,
                                                        zVec3 *posOffset);
RECOIL_NOINLINE int RECOIL_FASTCALL gwCameraUpdate(zClass_NodePartial *camera);
RECOIL_NOINLINE void RECOIL_CDECL SyncViewContextPositions();
} // namespace zClass_Camera

namespace zClass_Node {
RECOIL_NOINLINE int RECOIL_FASTCALL ClearPickupFlagsRecursive(zClass_NodePartial *node);
RECOIL_NOINLINE void RECOIL_FASTCALL PropagateTransformDirtyRecursive(zClass_NodePartial *self);
RECOIL_NOINLINE void RECOIL_FASTCALL PropagateExtraFlagsRecursive(zClass_NodePartial *self,
                                                                  int flags);
RECOIL_NOINLINE void RECOIL_FASTCALL SetContextRecursive(zClass_NodePartial *self,
                                                         zClass_NodePartial *context,
                                                         int flagMask);
RECOIL_NOINLINE void RECOIL_FASTCALL SetDiFlagBit0Recursive(zClass_NodePartial *node,
                                                            int enabled);
RECOIL_NOINLINE int RECOIL_FASTCALL HasRenderableDiPredicate(zClass_NodePartial *node);
RECOIL_NOINLINE void RECOIL_FASTCALL
SetMaterialFlagBit9ForFlagBit0EntriesRecursive(zClass_NodePartial *node, int enabled);
RECOIL_NOINLINE void RECOIL_FASTCALL
InvalidateFlagBit8MaterialImagesRecursive(zClass_NodePartial *node);
RECOIL_NOINLINE void RECOIL_FASTCALL
LoadFlagBit8MaterialImagesAndTexturePack(zClass_NodePartial *node);
RECOIL_NOINLINE void RECOIL_FASTCALL AssignInt32ToDiRecursive(zClass_NodePartial *node,
                                                              int value);
RECOIL_NOINLINE void RECOIL_FASTCALL AssignDamageHandlerRecursiveIfMissing(
    zClass_NodePartial *node, OptCatalogDamageHandlerPartial *handler);
RECOIL_NOINLINE void RECOIL_FASTCALL
ClearDamageHandlerRecursive(zClass_NodePartial *node, OptCatalogDamageHandlerPartial *handler);
RECOIL_NOINLINE int RECOIL_FASTCALL SetDamageHitCallback(void *callback,
                                                                  zClass_NodePartial *node,
                                                                  void *context);
RECOIL_NOINLINE int RECOIL_FASTCALL ClearDamageHandler(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL SetDamageTimerCallback(void *callback,
                                                                    zClass_NodePartial *node,
                                                                    void *context);
} // namespace zClass_Node

namespace zClass_TypeList {
RECOIL_NOINLINE zClass_TypeListLink *RECOIL_CDECL AllocLink();
RECOIL_NOINLINE void RECOIL_FASTCALL FreeLink(zClass_TypeListLink *link);
RECOIL_NOINLINE void RECOIL_CDECL FreeAll();
RECOIL_NOINLINE void RECOIL_FASTCALL ProcessPendingRemovals(int bucket);
RECOIL_NOINLINE int RECOIL_FASTCALL CountNodes(int bucket);
RECOIL_NOINLINE void RECOIL_FASTCALL PrintBucket(int bucket);
RECOIL_NOINLINE zClass_TypeListLink *RECOIL_FASTCALL GetBucketHead(int bucket);
RECOIL_NOINLINE int RECOIL_FASTCALL MarkPendingRemoval(int bucket,
                                                                zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL Insert(int bucket, zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL InsertChildNodes(int bucket,
                                                              zClass_NodePartial *node);
RECOIL_NOINLINE void RECOIL_CDECL UpdateAllBuckets();
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateBucket(zClass_TypeListLink *bucket);
RECOIL_NOINLINE int RECOIL_CDECL UpdateQueuedTrees();
RECOIL_NOINLINE int RECOIL_CDECL UpdateSequences();
RECOIL_NOINLINE int RECOIL_CDECL UpdateAnimations();
} // namespace zClass_TypeList

namespace gwNode {
RECOIL_NOINLINE int RECOIL_FASTCALL BuildNodeToAncestorMatrix(zClass_NodePartial *node,
                                                                       int matMode);
RECOIL_NOINLINE int RECOIL_FASTCALL GetWorldPosition(zClass_NodePartial *node,
                                                              zVec3 *outPosition);
RECOIL_NOINLINE int RECOIL_FASTCALL TransformPoint(zClass_NodePartial *node, zVec3 *point);
RECOIL_NOINLINE int RECOIL_FASTCALL GetWorldPosAndOrientation(zClass_NodePartial *node,
                                                                       zVec3 *inOutPosition,
                                                                       zVec3 *outOrientation);
RECOIL_NOINLINE int RECOIL_FASTCALL UpdateSubtree(zClass_NodePartial *node);
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateTree(zClass_NodePartial *node);
} // namespace gwNode

namespace zClass_NodeList {
RECOIL_NOINLINE int RECOIL_FASTCALL Insert(zClass_NodePartial *node);
RECOIL_NOINLINE void RECOIL_CDECL ProcessPendingFrees();
} // namespace zClass_NodeList

namespace zClass_List {
RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNodeFromLists(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwListDeleteANode(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL DeleteAllOfType(int bucket);
RECOIL_NOINLINE int RECOIL_CDECL RenderActiveCameras();
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
IterateBucketFiltered(const char *filterText, int bucket, zClass_NodePredicate predicate);
} // namespace zClass_List

namespace zClass {
RECOIL_NOINLINE void RECOIL_FASTCALL SetNodeArraySize(int size);
RECOIL_NOINLINE int RECOIL_CDECL IsInitialized();
RECOIL_NOINLINE int RECOIL_CDECL ResetCurrentZbdPath();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownCore();
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_CDECL ProcessDeferredWork();
RECOIL_NOINLINE int RECOIL_FASTCALL NodePtrToValidatedIndex(zClass_NodePartial *node);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL FindByTypeAndName(int bucket,
                                                                      const char *name);
RECOIL_NOINLINE int RECOIL_FASTCALL
FindNextByTypePrefix_Predicate(zClass_NodePartial *node);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL FindNextByTypePrefix(const char *prefixText,
                                                                         int bucket);
RECOIL_NOINLINE int RECOIL_FASTCALL
AnyNodeMatchesPredicateRecursive(zClass_NodePartial *root, zClass_NodePredicate predicate);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChildChecked(zClass_NodePartial *parent,
                                                                zClass_NodePartial *child);
} // namespace zClass

namespace zClass_Class {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL AllocNodeFromFreeList();
RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNodeByType(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeUpdate(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_CDECL gwNodeUpdateAll();
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeUpdateDisplayInstance(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetWorldBBoxCorners(zClass_NodePartial *node,
                                                                       zBBoxCorners *outCorners);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetViewBBoxCorners(zClass_NodePartial *node,
                                                                      zBBoxCorners *outCorners);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeComputeChildBBox(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeRecalcBBox(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetActive(zClass_NodePartial *node,
                                                             int active);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetFlag16(zClass_NodePartial *node,
                                                             int value);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetFlag17(zClass_NodePartial *node,
                                                             int value);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetDisplayInstance(zClass_NodePartial *node,
                                                                      zDiPartial *displayInstance);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetName(zClass_NodePartial *node,
                                                           const char *name);
RECOIL_NOINLINE char *RECOIL_FASTCALL gwNodeGetName(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetUserData(zClass_NodePartial *node,
                                                               unsigned int *outData);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetActionCallback(zClass_NodePartial *node,
                                                                     void *actionCallback);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetActionCallbackTail(zClass_NodePartial *node,
                                                                         void *actionCallback);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetPriority(zClass_NodePartial *node,
                                                               int priority);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetCellPickable(zClass_NodePartial *node,
                                                                   int value);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetCellPickable(zClass_NodePartial *node,
                                                                   int *outValue);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetNodeType(zClass_NodePartial *node,
                                                               int *outValue);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetRaycastable(zClass_NodePartial *node,
                                                                  int value);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetRaycastable(zClass_NodePartial *node,
                                                                  int *outValue);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetPickable(zClass_NodePartial *node,
                                                               int value);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeGetPickable(zClass_NodePartial *node,
                                                               int *outValue);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetHasHitCallback(zClass_NodePartial *node,
                                                                     int value);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetBypassFarClip(zClass_NodePartial *node,
                                                                    int value);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetNodeType(zClass_NodePartial *node,
                                                               int nodeType);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeClearVariantGate(zClass_NodePartial *node,
                                                                    int value);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeSetVertexAlphaOverride(zClass_NodePartial *node,
                                                                          int value);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL gwNodeGetRoot(zClass_NodePartial *node);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL gwNodeGetWorldChild(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL
gwNodeFindNextByName_Predicate(zClass_NodePartial *node);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL gwNodeFindNextByName(const char *name,
                                                                         int bucket);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL FindSubNodeByName(zClass_NodePartial *root,
                                                                      const char *name);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
FindNodeRecursiveByName(zClass_NodePartial *root, const char *name);
RECOIL_NOINLINE int RECOIL_FASTCALL SetSingleParentFlagRecursive(zClass_NodePartial *node,
                                                                          int setFlag);
RECOIL_NOINLINE int RECOIL_FASTCALL AddChildValidated(zClass_NodePartial *parent,
                                                               zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChildValidated(zClass_NodePartial *parent,
                                                                  zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL AddChild(zClass_NodePartial *parent,
                                                      zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL AddChildGeneric(zClass_NodePartial *parent,
                                                             zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial *parent,
                                                         zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChildGeneric(zClass_NodePartial *parent,
                                                                zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL FreeNodeToFreeList(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL TryFreeNode(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL gwNodeRenderDispatch(zClass_NodePartial *node,
                                                                  int siblingCountHint);
} // namespace zClass_Class

namespace zClass_Sound {
RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(zClass_NodePartial *node,
                                                            int siblingCountHint);
}

namespace zClass_Animate {
RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(zClass_NodePartial *node,
                                                            int siblingCountHint);
}

namespace zClass_Sequence {
RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(zClass_NodePartial *node,
                                                            int siblingCountHint);
}

namespace zClass_Switch {
RECOIL_NOINLINE int RECOIL_FASTCALL RenderTraverse(zClass_NodePartial *node,
                                                            int siblingCountHint);
}

namespace zClass_Util {
RECOIL_NOINLINE int RECOIL_FASTCALL DestroyNodeRecursive(zClass_NodePartial *node);
}

namespace zClass_cls_util {
RECOIL_NOINLINE int RECOIL_FASTCALL CopyNodeDisplayInstance(zClass_NodePartial *source,
                                                                     zClass_NodePartial *dest);
RECOIL_NOINLINE int RECOIL_FASTCALL CopyNodeBaseData(zClass_NodePartial *source,
                                                              zClass_NodePartial *dest);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
CopyLightNode_Unimplemented(zClass_NodePartial *source);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
CopySoundNode_Unimplemented(zClass_NodePartial *source);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyCameraNode(zClass_NodePartial *source);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyObject3DNode(zClass_NodePartial *source);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
CopyAnimateNode_Unimplemented(zClass_NodePartial *source);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyLodNode(zClass_NodePartial *source);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
CopySequenceNode_Unimplemented(zClass_NodePartial *source);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopySwitchNode_Stub(zClass_NodePartial *source);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyNodeDispatch(zClass_NodePartial *source);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
CopyNodeWithCloneOptions(zClass_NodePartial *source, int cloneDiMode, int diArg0);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL CopyNode(zClass_NodePartial *source,
                                                             int cloneDiMode,
                                                             int diArg0,
                                                             int diArg1);
} // namespace zClass_cls_util

namespace zClass_Sound {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwSoundNew();
RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNode(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial *parent,
                                                         zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL SetSampleSetByName(zClass_NodePartial *node,
                                                                const char *name);
RECOIL_NOINLINE int RECOIL_FASTCALL gwSoundSetActive(zClass_NodePartial *node,
                                                              int active);
RECOIL_NOINLINE int RECOIL_FASTCALL gwSoundSetPosition(zClass_NodePartial *node, float x,
                                                                float y, float z);
RECOIL_NOINLINE int RECOIL_FASTCALL UpdatePlayback(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL ComputeWorldTransform(
    zClass_NodePartial *node, zClass_SoundDataPartial *soundData);
} // namespace zClass_Sound

namespace zClass_Animate {
RECOIL_NOINLINE short RECOIL_FASTCALL AdvanceTime(zClass_AnimateRuntimePartial *runtime,
                                                         float deltaTime);
RECOIL_NOINLINE short RECOIL_FASTCALL SampleTransform(zClass_AnimateRuntimePartial *runtime);
RECOIL_NOINLINE int RECOIL_FASTCALL UpdateNode(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL AddChild(zClass_NodePartial *parent,
                                                      zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNode(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial *parent,
                                                         zClass_NodePartial *child);
} // namespace zClass_Animate

namespace zClass_Sequence {
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwSequenceNew();
RECOIL_NOINLINE int RECOIL_FASTCALL gwSequenceAddChild(zClass_NodePartial *parent,
                                                                zClass_NodePartial *child,
                                                                int insertIndex,
                                                                float delay);
RECOIL_NOINLINE int RECOIL_FASTCALL SetActive(zClass_NodePartial *node,
                                                       int active);
RECOIL_NOINLINE int RECOIL_FASTCALL SetRepeat(zClass_NodePartial *node,
                                                       int repeat);
RECOIL_NOINLINE int RECOIL_FASTCALL SetLoop(zClass_NodePartial *node, int loop);
RECOIL_NOINLINE int RECOIL_FASTCALL SetPause(zClass_NodePartial *node,
                                                      int paused);
RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChild(zClass_NodePartial *parent,
                                                         zClass_NodePartial *child);
RECOIL_NOINLINE int RECOIL_FASTCALL Update(zClass_NodePartial *node);
} // namespace zClass_Sequence

namespace Light {
RECOIL_NOINLINE int RECOIL_CDECL DestroyThermalGlowPool();
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
AllocFromFreeListAndAttach(zColorRgb *specularColor);
RECOIL_NOINLINE void RECOIL_FASTCALL ReturnToFreeList(zClass_NodePartial *lightNode);
} // namespace Light

namespace GameZ {
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL WriteZBDFile(const char *filename);
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL ReadZBDFile(const char *filename);
RECOIL_NOINLINE FILE *RECOIL_FASTCALL OpenAndReadZBDHeader(const char *filename,
                                                                zClass_ZbdHeader *outHeader);
} // namespace GameZ

namespace GameZ_ZBD {
RECOIL_NOINLINE int RECOIL_FASTCALL NodePtrToIndex(zClass_NodePartial *node);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL NodeIndexToPtr(int index);
RECOIL_NOINLINE int RECOIL_FASTCALL
WriteNodeRefListIndices(zClass_NodePartial **nodeRefList, int entryCount, void *stream);
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL
WriteSingleNodeClassData(zClass_NodePartial *node, void *stream);
RECOIL_NOINLINE int RECOIL_FASTCALL WriteNodeTable(void *stream);
RECOIL_NOINLINE int RECOIL_FASTCALL
ReadNodeRefListIndices(zClass_NodePartial **nodeRefList, int entryCount, void *stream);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadSingleNodeClassData(zClass_NodePartial *node,
                                                                     void *stream);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadNodeTable(int nodeCount, void *stream);
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL
ReloadDisplayInstancesFromCurrentPath_Local(zClass_NodePartial *node, int recurseChildren);
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL
ReloadDisplayInstancesRecursive_Local(void *stream, zClass_ZbdHeader *zbdHeader,
                                      zClass_NodePartial *node, int recurseChildren);
} // namespace GameZ_ZBD

extern "C" {
extern zClass_NodeFreeListSlot *g_zClass_NodeArray;
extern int g_zClass_NodeArraySize;
extern int g_zClass_ActiveNodeCount;
extern int g_zClass_NodeFreeHeadIndex;
extern int g_zClass_IsInitialized;
extern zClass_TypeListLink *g_zClass_TypeList_FreeLinkHead;
extern zClass_TypeListLink *g_zClass_NodeList_PendingFreeHead;
extern int g_zClass_DeferredProcessingEnabled;
extern int g_zClass_TypeList_LiveLinkCount;
extern int g_zClass_TypeList_PeakLiveLinkCount;
extern zClass_TypeListBucket g_zClass_TypeList_Buckets[16];
extern zClass_TypeListLink **g_zClassCallbackPriorityHeadSlotPtrs[6];
extern zClass_TypeListLink **g_zClass_TypeList_HeadSlotPtrs[16];
extern zClass_TypeListLink **g_zClass_TypeList_TailSlotPtrs[16];
extern zClass_TypeListLink *g_zClass_FilterIterCursor;
extern const char *g_zClass_FilterIterText;
extern int g_zClass_FilterIterPrefixLen;
extern char g_zClass_CurrentZbdPath[260];
extern zClass_NodePartial **g_GameZ_Zbd_NodeIndexScratch;
extern int g_GameZ_Zbd_NodeIndexScratchCapacity;
extern int g_zClass_CameraAutoClipDistanceAdjustEnabled;
extern float g_zClass_CameraAutoClipDistanceThreshold;
extern float g_zClass_CameraAutoClipDistanceScale;
extern float g_zClass_CameraAutoClipDistanceStep;
extern float g_zClass_CameraAutoClipDistanceMinScale;
extern int g_zClass_ObjectHseTestEnabled;
extern zClass_NodePartial *g_zClass_CurrentCamera;
extern zClass_NodePartial *g_zClass_CameraTargetNode;
extern zClass_NodePartial *g_MainCamera;
extern zClass_CameraDataPartial *g_zVideo_pActiveViewContext;
extern zClass_NodePartial *g_Player_RuntimeDiScene;
extern int g_zClass_CopyNodeCloneDiMode;
extern int g_zClass_CopyNodeDiArg0;
extern int g_zClass_CopyNodeDiArg1;
}

namespace zClass_TypeList {
RECOIL_FORCEINLINE zClass_TypeListBucket &Bucket(int bucket) {
    return *(zClass_TypeListBucket *)(g_zClass_TypeList_HeadSlotPtrs[bucket]);
}

RECOIL_FORCEINLINE zClass_TypeListLink *&Head(int bucket) {
    return *g_zClass_TypeList_HeadSlotPtrs[bucket];
}

RECOIL_FORCEINLINE zClass_TypeListLink *&Tail(int bucket) {
    return *g_zClass_TypeList_TailSlotPtrs[bucket];
}

RECOIL_FORCEINLINE int &PendingRemovalDirty(int bucket) {
    switch (bucket) {
    case 0:
        return g_zClass_TypeList_Buckets[1].pendingRemovalDirty;
    case 1:
        return g_zClass_TypeList_Buckets[2].pendingRemovalDirty;
    case 2:
        return g_zClass_TypeList_Buckets[3].pendingRemovalDirty;
    case 3:
        return g_zClass_TypeList_Buckets[4].pendingRemovalDirty;
    case 4:
        return g_zClass_TypeList_Buckets[5].pendingRemovalDirty;
    case 5:
        return g_zClass_TypeList_Buckets[6].pendingRemovalDirty;
    case 6:
        return g_zClass_TypeList_Buckets[0].pendingRemovalDirty;
    case 7:
        return g_zClass_TypeList_Buckets[7].pendingRemovalDirty;
    case 8:
        return g_zClass_TypeList_Buckets[8].pendingRemovalDirty;
    case 9:
        return g_zClass_TypeList_Buckets[9].pendingRemovalDirty;
    case 10:
        return g_zClass_TypeList_Buckets[10].pendingRemovalDirty;
    case 11:
        return g_zClass_TypeList_Buckets[14].pendingRemovalDirty;
    case 12:
        return g_zClass_TypeList_Buckets[15].pendingRemovalDirty;
    case 13:
        return g_zClass_TypeList_Buckets[11].pendingRemovalDirty;
    case 14:
        return g_zClass_TypeList_Buckets[12].pendingRemovalDirty;
    case 15:
        return g_zClass_TypeList_Buckets[13].pendingRemovalDirty;
    default:
        return Bucket(bucket).pendingRemovalDirty;
    }
}

RECOIL_FORCEINLINE void SetPendingRemovalDirty(int bucket, int value) {
    switch (bucket) {
    case 0:
        g_zClass_TypeList_Buckets[1].pendingRemovalDirty = value;
        break;
    case 1:
        g_zClass_TypeList_Buckets[2].pendingRemovalDirty = value;
        break;
    case 2:
        g_zClass_TypeList_Buckets[3].pendingRemovalDirty = value;
        break;
    case 3:
        g_zClass_TypeList_Buckets[4].pendingRemovalDirty = value;
        break;
    case 4:
        g_zClass_TypeList_Buckets[5].pendingRemovalDirty = value;
        break;
    case 5:
        g_zClass_TypeList_Buckets[6].pendingRemovalDirty = value;
        break;
    case 6:
        g_zClass_TypeList_Buckets[0].pendingRemovalDirty = value;
        break;
    case 7:
        g_zClass_TypeList_Buckets[7].pendingRemovalDirty = value;
        break;
    case 8:
        g_zClass_TypeList_Buckets[8].pendingRemovalDirty = value;
        break;
    case 9:
        g_zClass_TypeList_Buckets[9].pendingRemovalDirty = value;
        break;
    case 10:
        g_zClass_TypeList_Buckets[10].pendingRemovalDirty = value;
        break;
    case 11:
        g_zClass_TypeList_Buckets[14].pendingRemovalDirty = value;
        break;
    case 12:
        g_zClass_TypeList_Buckets[15].pendingRemovalDirty = value;
        break;
    case 13:
        g_zClass_TypeList_Buckets[11].pendingRemovalDirty = value;
        break;
    case 14:
        g_zClass_TypeList_Buckets[12].pendingRemovalDirty = value;
        break;
    case 15:
        g_zClass_TypeList_Buckets[13].pendingRemovalDirty = value;
        break;
    default:
        break;
    }
}
} // namespace zClass_TypeList

#endif
