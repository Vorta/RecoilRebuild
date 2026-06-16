#ifndef RECOIL_GAMEZRECOIL_INCLUDE_ZCLASS_H
#define RECOIL_GAMEZRECOIL_INCLUDE_ZCLASS_H

#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>
#include <stdio.h>

#include "GameZRecoil/zMath/zMath.h"
#include "recoil/recoil_callconv.h"

struct zBBoxCorners;
struct zDiPartial;
struct zClass_LightDataPartial;
struct zClass_SoundDataPartial;
struct zSndPlayHandle;
struct zSndSample;
struct zZbdSectionCallbackCtx;

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

typedef int(__fastcall *zClass_NodePredicate)(zClass_NodePartial *node);
typedef int(__fastcall *zClass_NodeActionCallback)(zClass_NodePartial *node);

struct zClass_NodeFreeListSlot {
    zClass_NodePartial node;
    zBBox3f primaryBounds;
    zBBox3f secondaryBounds;
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

/**
 * Original inline helper observed in zClass vector-field initialization callers
 * (D:\Proj\GameZRecoil\zClass\*.c).
 * Purpose: construct a zVec3 value from explicit x, y, and z components.
 */
inline zVec3 zVec3_Make(
    float x,
    float y,
    float z
) {
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

struct zClass_WorldSettingsSectionRecord {
    int fogState;
    zColorRgb fogColorRgb01;
    float fogRangeNear;
    float fogRangeFar;
    float fogAltitudeHigh;
    float fogAltitudeLow;
    float fogDensity;
};

struct zClass_SoundDataPartial {
    zSndSample *sample;
    zSndPlayHandle *playHandle;
    char sampleSetName[0x24];
    int runtimeFlags;
    zVec3 localPosition;
    zVec3 worldPos;
    float savedParentMatrix[12];
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

typedef void(__fastcall *zClass_RenderFn)(
    zClass_NodePartial *node,
    int clipMask
);

RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        name
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        flags
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        auxFlags
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        boundsFlags
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        classId
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        name
    ) == 0x00
);
RECOIL_STATIC_ASSERT(sizeof(((zClass_NodePartial *)0)->name) == 0x24);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        nodeType
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        classData
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        userDataOrDiRef
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        callbackContext
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        callbackPriority
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        actionCallback
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        gridCol
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        gridRow
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        listCountA
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        listA
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        listCountB
    ) == 0x5c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        listB
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodePartial,
        cachedBounds
    ) == 0x74
);
RECOIL_STATIC_ASSERT(sizeof(zClass_NodePartial) == 0x8c);
RECOIL_STATIC_ASSERT(sizeof(zClass_TypeListLink) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_TypeListBucket,
        head
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_TypeListBucket,
        tail
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_TypeListBucket,
        pendingRemovalDirty
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zClass_TypeListBucket) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodeFreeListSlot,
        primaryBounds
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodeFreeListSlot,
        secondaryBounds
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodeFreeListSlot,
        damageHandler
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_NodeFreeListSlot,
        freeTag
    ) == 0xc0
);
RECOIL_STATIC_ASSERT(sizeof(zClass_NodeFreeListSlot) == 0xc4);
RECOIL_STATIC_ASSERT(sizeof(zClass_ZbdHeader) == 0x24);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_ZbdHeader,
        texDirArg
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_ZbdHeader,
        texDirOffset
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_ZbdHeader,
        matlOffset
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_ZbdHeader,
        model3dOffset
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_ZbdHeader,
        nodeCount
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_ZbdHeader,
        nodeFreeHead
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_ZbdHeader,
        nodeTableOffset
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageHandlerPartial,
        hitCallback
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageHandlerPartial,
        hitContext
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageHandlerPartial,
        timerCallback
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageHandlerPartial,
        timerContext
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogDamageHandlerPartial) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowClearPoly,
        vertices
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowClearPoly,
        vertCount
    ) == 0x30
);
RECOIL_STATIC_ASSERT(sizeof(zClass_WindowClearPoly) == 0x34);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3DDataPartial,
        flags
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3DDataPartial,
        alphaScale
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3DDataPartial,
        color
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3DDataPartial,
        colorAlpha
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3DDataPartial,
        rotation
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3DDataPartial,
        scale
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3DDataPartial,
        localMatrix
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3DDataPartial,
        cachedWorldMatrix
    ) == 0x60
);
RECOIL_STATIC_ASSERT(sizeof(zClass_Object3DDataPartial) == 0x90);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateDataPartial,
        statusFlags
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateDataPartial,
        animatedTransform
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateDataPartial,
        savedParentMatrix
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateDataPartial,
        runtime
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        keyframes
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        sampledRotation
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        sampledPosition
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        sampledScale
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        outputRotationScale
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        outputPositionScale
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        outputScaleScale
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        duration
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        currentTime
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        loopBase
    ) == 0x5c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        startTime
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        state
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        maxFrameIndex
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateRuntimePartial,
        loopCount
    ) == 0x6a
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateKeyframePartial,
        rotation
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateKeyframePartial,
        position
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateKeyframePartial,
        scale
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zClass_AnimateKeyframePartial) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zClass_AnimateRuntimePartial) == 0x6c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateDataPartial,
        runtime
    ) +
        offsetof(
            zClass_AnimateRuntimePartial,
            sampledRotation
        ) ==
    0x70
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateDataPartial,
        runtime
    ) +
        offsetof(
            zClass_AnimateRuntimePartial,
            sampledPosition
        ) ==
    0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_AnimateDataPartial,
        runtime
    ) +
        offsetof(
            zClass_AnimateRuntimePartial,
            sampledScale
        ) ==
    0x88
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        viewportWidth
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        viewportHeight
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        resolutionWidth
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        resolutionHeight
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        clearPolys
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        clearPolyIndexFlags
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        bufferIndex
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        buffer
    ) == 0xe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        fbWidth
    ) == 0xec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        fbHeight
    ) == 0xf0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WindowDataPartial,
        fbBpp
    ) == 0xf4
);
RECOIL_STATIC_ASSERT(sizeof(zClass_WindowDataPartial) == 0xf8);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_DisplayDataPartial,
        x
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_DisplayDataPartial,
        y
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_DisplayDataPartial,
        width
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_DisplayDataPartial,
        height
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_DisplayDataPartial,
        backgroundR
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_DisplayDataPartial,
        backgroundG
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_DisplayDataPartial,
        backgroundB
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zClass_DisplayDataPartial) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        cellMinX
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        cellMinZ
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        bbox
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        bboxCenter
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        bboxRadius
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        displayRefreshQueued
    ) == 0x39
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        childCount
    ) == 0x3a
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        childList
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(sizeof(zWorldAreaPartial) == 0x40);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        pendingAreaUpdateCount
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        pendingAreaUpdateCapacity
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        pendingAreaUpdates
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        fogState
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        ambientColor
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        originX
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        originZ
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        worldMaxX
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        worldMaxZ
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        partitionMaxDecFeatureCount
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        clampQueriesToBounds
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaCellSizeX
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaCellSizeZ
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaHalfSizeX
    ) == 0x5c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaHalfSizeZ
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaInvSizeX
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaInvSizeZ
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaCellRadiusBias
    ) == 0x6c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        partitionInclusionTolX
    ) == 0x70
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        partitionInclusionTolZ
    ) == 0x74
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaGridColCount
    ) == 0x78
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaGridRowCount
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaGridRows
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        scaleX
    ) == 0x84
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        scaleY
    ) == 0x88
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        scaleZ
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        lightCount
    ) == 0x90
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        lightNodes
    ) == 0x94
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        lightDataList
    ) == 0x98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        soundCount
    ) == 0x9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        soundNodes
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        soundDataList
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldDataPartial,
        areaGridExternalOwnership
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(sizeof(zClass_WorldDataPartial) == 0xac);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldSettingsSectionRecord,
        fogState
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldSettingsSectionRecord,
        fogColorRgb01
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldSettingsSectionRecord,
        fogRangeNear
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldSettingsSectionRecord,
        fogRangeFar
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldSettingsSectionRecord,
        fogAltitudeHigh
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldSettingsSectionRecord,
        fogAltitudeLow
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_WorldSettingsSectionRecord,
        fogDensity
    ) == 0x20
);
RECOIL_STATIC_ASSERT(sizeof(zClass_WorldSettingsSectionRecord) == 0x24);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        sample
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        playHandle
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        sampleSetName
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        runtimeFlags
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        localPosition
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        worldPos
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        savedParentMatrix
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        falloffMode
    ) == 0x78
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        rangeMin
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        rangeMax
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        rangeMaxSq
    ) == 0x84
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        invRangeSpan
    ) == 0x88
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        attachedWorldCount
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SoundDataPartial,
        attachedWorlds
    ) == 0x90
);
RECOIL_STATIC_ASSERT(sizeof(zClass_SoundDataPartial) == 0x94);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceDataPartial,
        isActive
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceDataPartial,
        repeatAtBounds
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceDataPartial,
        wrapAtBounds
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceDataPartial,
        isPaused
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceDataPartial,
        step
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceDataPartial,
        currentIndex
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceDataPartial,
        currentTime
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceDataPartial,
        entryCount
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceDataPartial,
        entries
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceEntryPartial,
        node
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SequenceEntryPartial,
        triggerTime
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(zClass_SequenceEntryPartial) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SwitchDataPartial,
        activeMaskIndex
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_SwitchDataPartial,
        childMasks
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(zClass_RenderColorAlphaState) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zClass_LodDistanceState) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        computeOwnDistance
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        nearRangeSq
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        nearRange
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        farRangeSq
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        fadeWidth
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        fadeAmount
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        fadeEndScale
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        fogFadeWidth
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        vertexShadingAmount
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        active
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        rangeNode
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LodDataPartial,
        rangeSq
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(sizeof(zClass_LodDataPartial) == 0x50);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        worldPosScratch
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        viewPos
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        localRotation
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        localPosition
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        worldRotation
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        worldPosition
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        savedParentMatrix
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        worldDir
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        falloff
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        intensityScale
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        specularColor
    ) == 0xac
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        coneAngle
    ) == 0xb8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        isPointMode
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        isDirectionalMode
    ) == 0xc0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        lightParam
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        lightSubMode
    ) == 0xc8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        range1
    ) == 0xcc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        range2
    ) == 0xd0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        range2Sq
    ) == 0xd4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        invRangeDelta
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        attachedWorldCount
    ) == 0xdc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_LightDataPartial,
        attachedWorlds
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(sizeof(zClass_LightDataPartial) == 0xe4);
RECOIL_STATIC_ASSERT(sizeof(zTag4Partial) == 0x04);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        areaIndex
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        worldNode
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        windowNode
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        horizonNode
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        horizonXZNode
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        cameraFlags
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        targetOrEuler
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        posOffset
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        cameraPos
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        eulerAngles
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        worldTransform
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        forwardDir
    ) == 0x74
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        worldTarget
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        nearClip
    ) == 0xb0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        farClip
    ) == 0xb4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        nearClipCenter
    ) == 0xb8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        farClipCenter
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        clipDistance
    ) == 0xd0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        invClipDistanceSq
    ) == 0xd4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        viewportWidth
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        viewportHeight
    ) == 0xdc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        frustumWidth
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        frustumHeight
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        fovX
    ) == 0xe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        fovY
    ) == 0xec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        frustumYaw
    ) == 0xf0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        frustumPitch
    ) == 0xf4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        frustumVectorsDirty
    ) == 0xf8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        frustumOrigin
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        frustumCorners
    ) == 0x108
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        localFrustumNormalsDirty
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        localFrustumLeftNormal
    ) == 0x13c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        localFrustumRightNormal
    ) == 0x148
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        localFrustumBottomNormal
    ) == 0x154
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        localFrustumTopNormal
    ) == 0x160
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        localFrustumNearNormal
    ) == 0x16c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        localFrustumFarNormal
    ) == 0x178
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        transformDirty
    ) == 0x184
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        worldFrustumNormals
    ) == 0x188
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        viewportScaleX
    ) == 0x1d4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        viewportScaleY
    ) == 0x1d8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        variantOverrideEnabled
    ) == 0x1e0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_CameraDataPartial,
        variantTag
    ) == 0x1e4
);
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

RECOIL_STATIC_ASSERT(
    offsetof(
        zCamera_FrustumGridTilePartial,
        col
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zCamera_FrustumGridTilePartial,
        row
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zCamera_FrustumGridTilePartial,
        hasPosOffset
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zCamera_FrustumGridTilePartial,
        posOffsetX
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zCamera_FrustumGridTilePartial,
        posOffsetZ
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zCamera_FrustumGridTilePartial,
        clipMask
    ) == 0x14
);
RECOIL_STATIC_ASSERT(sizeof(zCamera_FrustumGridTilePartial) == 0x18);
RECOIL_STATIC_ASSERT(
    offsetof(
        zCamera_FrustumGridTileRingPartial,
        count
    ) == 0x2d0
);
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

// Source-faithful helper recovered from address-backed callers in this source file.
inline zClass_NodeFreeListSlot *zClass_NodeSlotFromNode(
    zClass_NodePartial *node
) {
    return (zClass_NodeFreeListSlot *)node;
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline const zClass_NodeFreeListSlot *zClass_NodeSlotFromNode(
    const zClass_NodePartial *node
) {
    return (const zClass_NodeFreeListSlot *)node;
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline zVec3 *zClass_NodeViewSphereCenter(
    zClass_NodePartial *node
) {
    return (zVec3 *)(&zClass_NodeSlotFromNode(node)->primaryBounds.minX);
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline const zVec3 *zClass_NodeViewSphereCenter(
    const zClass_NodePartial *node
) {
    return (const zVec3 *)(&zClass_NodeSlotFromNode(node)->primaryBounds.minX);
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline float *zClass_NodeViewSphereRadius(
    zClass_NodePartial *node
) {
    return &zClass_NodeSlotFromNode(node)->primaryBounds.maxX;
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline const float *zClass_NodeViewSphereRadius(
    const zClass_NodePartial *node
) {
    return &zClass_NodeSlotFromNode(node)->primaryBounds.maxX;
}

namespace BBox {
void __fastcall ExpandToCorners(
    const zBBox3f *bbox,
    zBBoxCorners *outCorners
);
float *__fastcall MinMaxToBoundingSphere(
    const zBBox3f *bbox,
    zVec3 *outCenter,
    float *outRadius
);
void __fastcall CornersToBoundingSphere(
    zBBoxCorners *corners,
    zVec3 *outCenter,
    float *outRadius
);
} // namespace BBox

namespace zTag4 {
void __fastcall Clear(zTag4Partial *tag);
}

namespace zClass_Window {
zClass_NodePartial *gwWindowNew();
int __fastcall gwWindowSetResolution(
    zClass_NodePartial *node,
    int width,
    int height
);
int __fastcall gwWindowGetResolution(
    zClass_NodePartial *node,
    int *outWidth,
    int *outHeight
);
int __fastcall gwWindowSetSize(
    zClass_NodePartial *node,
    int width,
    int height
);
int __fastcall gwWindowGetSize(
    zClass_NodePartial *node,
    int *outWidth,
    int *outHeight
);
int __fastcall gwWindowSetBuffer(
    zClass_NodePartial *node,
    int bufferIndex
);
int __fastcall gwWindowSetClearPolygon(
    zClass_NodePartial *node,
    int enabled
);
int __fastcall gwWindowAddClearPolygonVertex(
    zClass_NodePartial *node,
    const zVec3 *point
);
int __fastcall gwWindowCloseClearPolygon(zClass_NodePartial *node);
} // namespace zClass_Window

namespace zClass_Display {
zClass_NodePartial *gwDisplayInit();
int __fastcall RemoveChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall gwDisplaySetSize(
    zClass_NodePartial *node,
    int width,
    int height
);
int __fastcall gwDisplaySetPosition(
    zClass_NodePartial *node,
    int x,
    int y
);
int __fastcall gwDisplaySetBackgroundColor(
    zClass_NodePartial *node,
    float red,
    float green,
    float blue
);
} // namespace zClass_Display

namespace zClass_World {
int __fastcall WriteSettingsSection(
    zZbdSectionCallbackCtx *callbackCtx,
    void *userData
);
void __fastcall ReadSettingsSection(
    zZbdSectionCallbackCtx *callbackCtx,
    const char *worldName,
    zClass_WorldSettingsSectionRecord *settings,
    unsigned int size,
    void *userData
);
zClass_NodePartial *gwWorldNew();
int __fastcall DeleteNode(zClass_NodePartial *world);
int __fastcall FreeVirtualAreaPartitions(zClass_NodePartial *world);
int __fastcall QueueAreaUpdate(
    zClass_NodePartial *world,
    zClass_WorldDataPartial *worldData,
    zWorldAreaPartial *area
);
int __fastcall RebuildAreaBounds(
    zClass_WorldDataPartial *worldData,
    zWorldAreaPartial *area
);
int __fastcall ApplyPendingFogSettings(zClass_NodePartial *world);
int __fastcall SetPendingFogState(
    zClass_NodePartial *world,
    int fogState
);
int __fastcall SetPendingFogColorRgb01(
    zClass_NodePartial *world,
    float red,
    float green,
    float blue
);
int __fastcall SetPendingFogAltitudeRange(
    zClass_NodePartial *world,
    float minAlt,
    float maxAlt
);
int __fastcall SetPendingFogRange(
    zClass_NodePartial *world,
    float nearRange,
    float farRange
);
int __fastcall GetPendingFogDensity(
    zClass_NodePartial *world,
    float *outDensity
);
int __fastcall GetPendingFogState(
    zClass_NodePartial *world,
    int *outState
);
int __fastcall GetPendingFogColorRgb01(
    zClass_NodePartial *world,
    float *outRed,
    float *outGreen,
    float *outBlue
);
int __fastcall GetPendingFogRange(
    zClass_NodePartial *world,
    float *outNearRange,
    float *outFarRange
);
int __fastcall GetPendingFogAltitudeRange(
    zClass_NodePartial *world,
    float *outMinAlt,
    float *outMaxAlt
);
int __fastcall SetPendingFogDensity(
    zClass_NodePartial *world,
    float density
);
int __fastcall gwWorldSetOrigin(
    zClass_NodePartial *world,
    float originX,
    float originZ
);
int __fastcall gwWorldSetSize(
    zClass_NodePartial *world,
    float sizeX,
    float sizeZ
);
int __fastcall gwWorldSetPartitionInclusionTolerance(
    zClass_NodePartial *world,
    float toleranceX,
    float toleranceZ
);
int __fastcall gwWorldSetMaxDecFeatures(
    zClass_NodePartial *world,
    int maxFeatures
);
int __fastcall gwWorldSetVirtualAreaPartition(
    zClass_NodePartial *world,
    float cellSizeX,
    float cellSizeZ
);
int __fastcall InitVirtualAreaPartitions(zClass_NodePartial *world);
int __fastcall SetVirtualPartition(
    zClass_NodePartial *world,
    int enabled
);
int __fastcall WorldRectToGridIndex(
    zClass_NodePartial *world,
    int *outGridCol,
    float minX,
    float maxX,
    float minZ,
    float maxZ,
    int *outGridRow
);
int __fastcall WorldToGridCoordsClampedEx(
    zClass_NodePartial *world,
    int *outGridCol,
    float worldX,
    float worldZ,
    int *outGridRow,
    int *clampedGridColOut,
    int *clampedGridRowOut,
    int *insideBoundsOut
);
int __fastcall WorldToGridCoordsClamped(
    zClass_NodePartial *world,
    int *outGridCol,
    float worldX,
    float worldZ,
    int *outGridRow
);
zWorldAreaPartial *__fastcall GetAreaPartitionAtGrid(
    zClass_NodePartial *world,
    int gridCol,
    int gridRow
);
int __fastcall AddChildAtGrid(
    zClass_NodePartial *world,
    zClass_NodePartial *child
);
int __fastcall EnsureGridCellDisplayPosition(
    zClass_NodePartial *world,
    int gridCol,
    int gridRow
);
int __fastcall AddChildToGridCell(
    zClass_NodePartial *world,
    zClass_NodePartial *child,
    int gridCol,
    int gridRow
);
int __fastcall RemoveChildAtGrid(
    zClass_NodePartial *world,
    zClass_NodePartial *child
);
int __fastcall AddLight(
    zClass_NodePartial *world,
    zClass_NodePartial *light
);
int __fastcall RemoveLight(
    zClass_NodePartial *world,
    zClass_NodePartial *light
);
int __fastcall InitLightPointInPolygonXZ(zClass_NodePartial *world);
int __fastcall UpdateAllLights(zClass_NodePartial *world);
int __fastcall AddSound(
    zClass_NodePartial *world,
    zClass_NodePartial *sound
);
int __fastcall RemoveSound(
    zClass_NodePartial *world,
    zClass_NodePartial *sound
);
int __fastcall UpdateAllSounds(zClass_NodePartial *world);
} // namespace zClass_World

namespace zClass_Object3D {
zClass_NodePartial *gwObject3DInit();
int __fastcall RenderTraverse(
    zClass_NodePartial *node,
    int siblingCountHint
);
int __fastcall PropagateTransformDirty(zClass_NodePartial *node);
int __fastcall gwObject3DSetVisibleFlag(
    zClass_NodePartial *node,
    int visible
);
int __fastcall gwObject3DSetColorAlpha(
    zClass_NodePartial *node,
    zColorRgb *color,
    float alpha
);
int __fastcall gwObject3DSetAlphaScale(
    zClass_NodePartial *node,
    float alphaScale
);
int __fastcall gwObject3DGetAlphaScale(
    zClass_NodePartial *node,
    float *outAlphaScale
);
int __fastcall gwObject3DSetLitFlag(
    zClass_NodePartial *node,
    int lit
);
int __fastcall gwObject3DSetScale(
    zClass_NodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwObject3DGetScale(
    zClass_NodePartial *node,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwObject3DGetRotation(
    zClass_NodePartial *node,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwObject3DSetRotation(
    zClass_NodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwObject3DTranslateRotation(
    zClass_NodePartial *node,
    float dx,
    float dy,
    float dz
);
int __fastcall gwObject3DGetPosition(
    zClass_NodePartial *node,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwObject3DSetPosition(
    zClass_NodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwObject3DTranslatePosition(
    zClass_NodePartial *node,
    float dx,
    float dy,
    float dz
);
float *__fastcall gwObject3DGetMatrixPtr(zClass_NodePartial *node);
int __fastcall gwObject3DSetMatrix(
    zClass_NodePartial *node,
    float *matrix
);
int __fastcall gwObject3DAddChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall RemoveChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall DeleteNode(zClass_NodePartial *node);
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
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3D_ModelRefLerpTask,
        callbackCtx
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3D_ModelRefLerpTask,
        onComplete
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3D_ModelRefLerpTask,
        invertModelRef
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3D_ModelRefLerpTask,
        targetModelRef
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3D_ModelRefLerpTask,
        currentModelRef
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3D_ModelRefLerpTask,
        modelRefDeltaPerSec
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_Object3D_ModelRefLerpTask,
        next
    ) == 0x1c
);
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

typedef void(__fastcall *zClass_Object3D_ModelRefLerpCallback)(void *callbackCtx);

namespace zClass_Object3D_ModelRefLerpQueue {
void ClearGlobalState();
void __fastcall Add(
    zClass_NodePartial *node,
    void *callbackCtx,
    void *onComplete,
    float startModelRef,
    float targetModelRef,
    float durationSec
);
void Update();
void Reset();
} // namespace zClass_Object3D_ModelRefLerpQueue

namespace zClass_Lod {
zClass_NodePartial *gwLodNew();
int __fastcall RenderTraverse(
    zClass_NodePartial *node,
    int siblingCountHint
);
int __fastcall gwLodAddChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall RemoveChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall SetComputeOwnDistance(
    zClass_NodePartial *node,
    int enabled
);
int __fastcall SetTargetNodeAndRange(
    zClass_NodePartial *node,
    zClass_NodePartial *target,
    float range
);
} // namespace zClass_Lod

namespace zClass_Light {
zClass_NodePartial *gwLightNew();
int __fastcall RenderTraverse(
    zClass_NodePartial *node,
    int siblingCountHint
);
int __fastcall DeleteNode(zClass_NodePartial *node);
int __fastcall RemoveChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall gwLightSetIntensity(
    zClass_NodePartial *node,
    float intensity
);
int __fastcall gwLightSetFalloff(
    zClass_NodePartial *node,
    float falloff
);
int __fastcall gwLightSetConeAngle(
    zClass_NodePartial *node,
    unsigned int coneAngleBits
);
int __fastcall gwLightSetPointMode(zClass_NodePartial *node);
int __fastcall gwLightSetDirectionalMode(zClass_NodePartial *node);
int __fastcall gwLightSetParam(
    zClass_NodePartial *node,
    int param
);
int __fastcall gwLightSetRange(
    zClass_NodePartial *node,
    float rangeA,
    float rangeB
);
int __fastcall gwLightGetRange(
    zClass_NodePartial *node,
    float *outRange1,
    float *outRange2
);
int __fastcall gwLightSetPosition(
    zClass_NodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwLightSetRotation(
    zClass_NodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall ComputeWorldTransform(
    zClass_NodePartial *node,
    zClass_LightDataPartial *data
);
int __fastcall gwLightUpdate(zClass_NodePartial *node);
int __fastcall gwLightGetSpecularColor(
    zClass_NodePartial *node,
    float *outRed,
    float *outGreen,
    float *outBlue
);
int __fastcall gwLightSetSpecularColor(
    zClass_NodePartial *node,
    float red,
    float green,
    float blue
);
} // namespace zClass_Light

namespace zClass_Camera {
zClass_NodePartial *gwCameraNew();
int __fastcall RenderTraverse(
    zClass_NodePartial *node,
    int siblingCountHint
);
int __fastcall gwCameraAddChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall gwCameraRemoveChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall gwCameraSetFlagBit0(
    zClass_NodePartial *node,
    int enabled
);
int __fastcall SetTargetNode(zClass_NodePartial *target);
zClass_NodePartial *__fastcall SetActiveCamera(zClass_NodePartial *camera);
int __fastcall SetObjectHseTestEnabled(int enabled);
int __fastcall gwCameraSetWorld(
    zClass_NodePartial *camera,
    zClass_NodePartial *world
);
zClass_NodePartial *__fastcall gwCameraGetWorld(zClass_NodePartial *camera);
int __fastcall gwCameraSetWindow(
    zClass_NodePartial *camera,
    zClass_NodePartial *window
);
int __fastcall ActivateChildren(
    zClass_NodePartial *camera,
    zClass_CameraDataPartial *data
);
int __fastcall gwCameraSetPosition(
    zClass_NodePartial *camera,
    float x,
    float y,
    float z
);
int __fastcall gwCameraTranslate(
    zClass_NodePartial *camera,
    float dx,
    float dy,
    float dz
);
int __fastcall gwCameraGetPosition(
    zClass_NodePartial *camera,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwCameraSetTarget(
    zClass_NodePartial *camera,
    float x,
    float y,
    float z
);
int __fastcall gwCameraTranslateTarget(
    zClass_NodePartial *camera,
    float dx,
    float dy,
    float dz
);
int __fastcall gwCameraGetTarget(
    zClass_NodePartial *camera,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwCameraSetNearFarClip(
    zClass_NodePartial *camera,
    float nearClip,
    float farClip
);
int __fastcall gwCameraGetNearFarClip(
    zClass_NodePartial *camera,
    float *outNear,
    float *outFar
);
int __fastcall gwCameraSetViewport(
    zClass_NodePartial *camera,
    float viewportWidth,
    float viewportHeight
);
int __fastcall gwCameraGetViewport(
    zClass_NodePartial *camera,
    float *outWidth,
    float *outHeight
);
int __fastcall gwCameraGetFOV(
    zClass_NodePartial *camera,
    float *outFovX,
    float *outFovY
);
int __fastcall gwCameraSetFOV(
    zClass_NodePartial *camera,
    float fovX,
    float fovY
);
int __fastcall gwCameraGetClipDistance(
    zClass_NodePartial *camera,
    float *outClipDistance
);
int __fastcall gwCameraSetClipDistance(
    zClass_NodePartial *camera,
    float clipDistance
);
int __fastcall gwCameraSetHorizon(
    zClass_NodePartial *camera,
    zClass_NodePartial *horizonNode
);
int __fastcall gwCameraSetHorizonXZ(
    zClass_NodePartial *camera,
    zClass_NodePartial *horizonXZNode
);
void __fastcall SetViewDistance(
    int enableAutoClip,
    float distance
);
float __fastcall FastAngleXZ(
    zVec3 *point1,
    zVec3 *point2
);
int __fastcall FindConvexHullXZ(
    zVec3 *points,
    int count
);
int __fastcall BuildFrustumGridTiles(
    zClass_NodePartial *world,
    zClass_WorldDataPartial *worldData,
    zClass_CameraDataPartial *cameraData
);
int __fastcall BuildFrustumGridTilesFromParams(
    zClass_NodePartial *world,
    zClass_WorldDataPartial *worldData,
    zClass_CameraDataPartial *cameraData
);
int __fastcall RenderFrustumGridTiles(
    zClass_NodePartial *world,
    zClass_NodePartial *camera,
    zClass_CameraDataPartial *cameraData
);
void __fastcall RenderOverlayNodes(zClass_NodePartial *world);
void __fastcall RenderWorld(
    zClass_NodePartial *world,
    zClass_NodePartial *camera,
    zClass_CameraDataPartial *cameraData
);
int __fastcall gwCameraSetVariantTagOverride(
    zClass_NodePartial *camera,
    zTag4Partial *variantTag
);
int __fastcall RenderScene(
    zClass_NodePartial *camera,
    int updateFxPass3Local
);
int __fastcall BuildWorldTransform(
    zClass_NodePartial *camera,
    zClass_CameraDataPartial *data,
    zVec3 *posOffset
);
int __fastcall UpdateImpl(
    zClass_NodePartial *camera,
    zVec3 *posOffset
);
int __fastcall gwCameraUpdate(zClass_NodePartial *camera);
void SyncViewContextPositions();
} // namespace zClass_Camera

namespace zClass_Node {
int __fastcall ClearPickupFlagsRecursive(zClass_NodePartial *node);
int __fastcall SetPickupFlagsRecursive(zClass_NodePartial *node);
void __fastcall PropagateTransformDirtyRecursive(zClass_NodePartial *self);
void __fastcall MaskExtraFlagsRecursive(
    zClass_NodePartial *self,
    int mask
);
void __fastcall PropagateExtraFlagsRecursive(
    zClass_NodePartial *self,
    int flags
);
void __fastcall PropagateFlagsRecursive(
    zClass_NodePartial *self,
    int flags
);
void __fastcall SetContextRecursive(
    zClass_NodePartial *self,
    zClass_NodePartial *context,
    int flagMask
);
void __fastcall SetDiFlagBit0Recursive(
    zClass_NodePartial *node,
    int enabled
);
int __fastcall HasRenderableDiPredicate(zClass_NodePartial *node);
void __fastcall SetMaterialFlagBit9ForFlagBit0EntriesRecursive(
    zClass_NodePartial *node,
    int enabled
);
void __fastcall InvalidateFlagBit8MaterialImagesRecursive(
    zClass_NodePartial *node
);
void __fastcall LoadFlagBit8MaterialImagesAndTexturePack(
    zClass_NodePartial *node
);
void __fastcall AssignInt32ToDiRecursive(
    zClass_NodePartial *node,
    int value
);
void __fastcall AssignDamageHandlerRecursiveIfMissing(
    zClass_NodePartial *node,
    OptCatalogDamageHandlerPartial *handler
);
void __fastcall ClearDamageHandlerRecursive(
    zClass_NodePartial *node,
    OptCatalogDamageHandlerPartial *handler
);
int __fastcall SetDamageHitCallback(
    void *callback,
    zClass_NodePartial *node,
    void *context
);
int __fastcall ClearDamageHandler(zClass_NodePartial *node);
int __fastcall SetDamageTimerCallback(
    void *callback,
    zClass_NodePartial *node,
    void *context
);
} // namespace zClass_Node

namespace zClass_TypeList {
zClass_TypeListLink *AllocLink();
void __fastcall FreeLink(zClass_TypeListLink *link);
void FreeAll();
void __fastcall ProcessPendingRemovals(int bucket);
int __fastcall CountNodes(int bucket);
void __fastcall PrintBucket(int bucket);
zClass_TypeListLink *__fastcall GetBucketHead(int bucket);
int __fastcall MarkPendingRemoval(
    int bucket,
    zClass_NodePartial *node
);
int __fastcall Insert(
    int bucket,
    zClass_NodePartial *node
);
int __fastcall InsertChildNodes(
    int bucket,
    zClass_NodePartial *node
);
void UpdateAllBuckets();
void __fastcall UpdateBucket(zClass_TypeListLink *bucket);
int UpdateQueuedTrees();
int UpdateSequences();
int UpdateAnimations();
} // namespace zClass_TypeList

namespace gwNode {
int __fastcall BuildNodeToAncestorMatrix(
    zClass_NodePartial *node,
    int matMode
);
int __fastcall GetWorldPosition(
    zClass_NodePartial *node,
    zVec3 *outPosition
);
int __fastcall TransformPoint(
    zClass_NodePartial *node,
    zVec3 *point
);
int __fastcall GetWorldPosAndOrientation(
    zClass_NodePartial *node,
    zVec3 *inOutPosition,
    zVec3 *outOrientation
);
int __fastcall UpdateSubtree(zClass_NodePartial *node);
void __fastcall UpdateTree(zClass_NodePartial *node);
} // namespace gwNode

namespace zClass_NodeList {
int __fastcall Insert(zClass_NodePartial *node);
void ProcessPendingFrees();
} // namespace zClass_NodeList

namespace zClass_List {
int __fastcall DeleteNodeFromLists(zClass_NodePartial *node);
int __fastcall gwListDeleteANode(zClass_NodePartial *node);
int __fastcall DeleteAllOfType(int bucket);
int RenderActiveCameras();
zClass_NodePartial *__fastcall IterateBucketFiltered(
    const char *filterText,
    int bucket,
    zClass_NodePredicate predicate
);
} // namespace zClass_List

namespace zClass {
void __fastcall SetNodeArraySize(int size);
int IsInitialized();
int Init();
int ResetCurrentZbdPath();
int ShutdownCore();
int Shutdown();
int ProcessDeferredWork();
int __fastcall NodePtrToValidatedIndex(zClass_NodePartial *node);
zClass_NodePartial *__fastcall FindByTypeAndName(
    int bucket,
    const char *name
);
int __fastcall FindNextByTypePrefix_Predicate(zClass_NodePartial *node);
zClass_NodePartial *__fastcall FindNextByTypePrefix(
    const char *prefixText,
    int bucket
);
int __fastcall AnyNodeMatchesPredicateRecursive(
    zClass_NodePartial *root,
    zClass_NodePredicate predicate
);
int __fastcall RemoveChildChecked(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
} // namespace zClass

namespace zClass_Class {
zClass_NodePartial *AllocNodeFromFreeList();
int __fastcall DeleteNodeByType(zClass_NodePartial *node);
int __fastcall gwNodeUpdate(zClass_NodePartial *node);
int gwNodeUpdateAll();
int __fastcall gwNodeUpdateDisplayInstance(zClass_NodePartial *node);
int __fastcall gwNodeGetBBox(
    zClass_NodePartial *node,
    zBBox3f *outBBox
);
int __fastcall gwNodeGetWorldBBoxCorners(
    zClass_NodePartial *node,
    zBBoxCorners *outCorners
);
int __fastcall gwNodeGetViewBBoxCorners(
    zClass_NodePartial *node,
    zBBoxCorners *outCorners
);
int __fastcall gwNodeComputeChildBBox(zClass_NodePartial *node);
int __fastcall gwNodeRecalcBBox(zClass_NodePartial *node);
int __fastcall gwNodeSetActive(
    zClass_NodePartial *node,
    int active
);
int __fastcall gwNodeSetFlag16(
    zClass_NodePartial *node,
    int value
);
int __fastcall gwNodeSetFlag17(
    zClass_NodePartial *node,
    int value
);
int __fastcall gwNodeSetDisplayInstance(
    zClass_NodePartial *node,
    zDiPartial *displayInstance
);
int __fastcall gwNodeSetName(
    zClass_NodePartial *node,
    const char *name
);
char *__fastcall gwNodeGetName(zClass_NodePartial *node);
int __fastcall gwNodeGetUserData(
    zClass_NodePartial *node,
    unsigned int *outData
);
int __fastcall gwNodeSetActionCallback(
    zClass_NodePartial *node,
    void *actionCallback
);
int __fastcall gwNodeSetActionCallbackTail(
    zClass_NodePartial *node,
    void *actionCallback
);
int __fastcall gwNodeSetPriority(
    zClass_NodePartial *node,
    int priority
);
int __fastcall gwNodeSetCellPickable(
    zClass_NodePartial *node,
    int value
);
int __fastcall gwNodeGetCellPickable(
    zClass_NodePartial *node,
    int *outValue
);
int __fastcall gwNodeGetNodeType(
    zClass_NodePartial *node,
    int *outValue
);
int __fastcall gwNodeSetRaycastable(
    zClass_NodePartial *node,
    int value
);
int __fastcall gwNodeGetRaycastable(
    zClass_NodePartial *node,
    int *outValue
);
int __fastcall gwNodeSetPickable(
    zClass_NodePartial *node,
    int value
);
int __fastcall gwNodeGetPickable(
    zClass_NodePartial *node,
    int *outValue
);
int __fastcall gwNodeSetHasHitCallback(
    zClass_NodePartial *node,
    int value
);
int __fastcall gwNodeSetBypassFarClip(
    zClass_NodePartial *node,
    int value
);
int __fastcall gwNodeSetNodeType(
    zClass_NodePartial *node,
    int nodeType
);
int __fastcall gwNodeClearVariantGate(
    zClass_NodePartial *node,
    int value
);
int __fastcall gwNodeSetVertexAlphaOverride(
    zClass_NodePartial *node,
    int value
);
zClass_NodePartial *__fastcall gwNodeGetRoot(zClass_NodePartial *node);
zClass_NodePartial *__fastcall gwNodeGetWorldChild(zClass_NodePartial *node);
int __fastcall gwNodeFindNextByName_Predicate(zClass_NodePartial *node);
zClass_NodePartial *__fastcall gwNodeFindNextByName(
    const char *name,
    int bucket
);
zClass_NodePartial *__fastcall FindSubNodeByName(
    zClass_NodePartial *root,
    const char *name
);
zClass_NodePartial *__fastcall FindNodeRecursiveByName(
    zClass_NodePartial *root,
    const char *name
);
int __fastcall SetSingleParentFlagRecursive(
    zClass_NodePartial *node,
    int setFlag
);
int __fastcall AddChildValidated(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall RemoveChildValidated(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall AddChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall AddChildGeneric(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall RemoveChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall RemoveChildGeneric(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall FreeNodeToFreeList(zClass_NodePartial *node);
int __fastcall TryFreeNode(zClass_NodePartial *node);
int __fastcall gwNodeRenderDispatch(
    zClass_NodePartial *node,
    int siblingCountHint
);
} // namespace zClass_Class

namespace zClass_Sound {
int __fastcall RenderTraverse(
    zClass_NodePartial *node,
    int siblingCountHint
);
}

namespace zClass_Animate {
int __fastcall RenderTraverse(
    zClass_NodePartial *node,
    int siblingCountHint
);
}

namespace zClass_Sequence {
int __fastcall RenderTraverse(
    zClass_NodePartial *node,
    int siblingCountHint
);
}

namespace zClass_Switch {
int __fastcall RenderTraverse(
    zClass_NodePartial *node,
    int siblingCountHint
);
}

namespace zClass_Util {
int __fastcall DestroyNodeRecursive(zClass_NodePartial *node);
}

namespace zClass_cls_util {
int __fastcall CopyNodeDisplayInstance(
    zClass_NodePartial *source,
    zClass_NodePartial *dest
);
int __fastcall CopyNodeBaseData(
    zClass_NodePartial *source,
    zClass_NodePartial *dest
);
zClass_NodePartial *__fastcall CopyLightNode_Unimplemented(
    zClass_NodePartial *source
);
zClass_NodePartial *__fastcall CopySoundNode_Unimplemented(
    zClass_NodePartial *source
);
zClass_NodePartial *__fastcall CopyCameraNode(zClass_NodePartial *source);
zClass_NodePartial *__fastcall CopyObject3DNode(zClass_NodePartial *source);
zClass_NodePartial *__fastcall CopyAnimateNode_Unimplemented(
    zClass_NodePartial *source
);
zClass_NodePartial *__fastcall CopyLodNode(zClass_NodePartial *source);
zClass_NodePartial *__fastcall CopySequenceNode_Unimplemented(
    zClass_NodePartial *source
);
zClass_NodePartial *__fastcall CopySwitchNode_Stub(zClass_NodePartial *source);
zClass_NodePartial *__fastcall CopyNodeDispatch(zClass_NodePartial *source);
zClass_NodePartial *__fastcall CopyNodeWithCloneOptions(
    zClass_NodePartial *source,
    int cloneDiMode,
    int diArg0
);
zClass_NodePartial *__fastcall CopyNode(
    zClass_NodePartial *source,
    int cloneDiMode,
    int diArg0,
    int diArg1
);
} // namespace zClass_cls_util

namespace zClass_Sound {
zClass_NodePartial *gwSoundNew();
int __fastcall DeleteNode(zClass_NodePartial *node);
int __fastcall RemoveChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall SetSampleSetByName(
    zClass_NodePartial *node,
    const char *name
);
int __fastcall gwSoundSetActive(
    zClass_NodePartial *node,
    int active
);
int __fastcall gwSoundSetPosition(
    zClass_NodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwSoundGetPosition(
    zClass_NodePartial *node,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall UpdatePlayback(zClass_NodePartial *node);
int __fastcall ComputeWorldTransform(
    zClass_NodePartial *node,
    zClass_SoundDataPartial *soundData
);
} // namespace zClass_Sound

namespace zClass_Animate {
short __fastcall AdvanceTime(
    zClass_AnimateRuntimePartial *runtime,
    float deltaTime
);
short __fastcall SampleTransform(zClass_AnimateRuntimePartial *runtime);
int __fastcall UpdateNode(zClass_NodePartial *node);
int __fastcall AddChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall DeleteNode(zClass_NodePartial *node);
int __fastcall RemoveChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
} // namespace zClass_Animate

namespace zClass_Sequence {
zClass_NodePartial *gwSequenceNew();
int __fastcall gwSequenceAddChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child,
    int insertIndex,
    float delay
);
int __fastcall SetActive(
    zClass_NodePartial *node,
    int active
);
int __fastcall SetRepeat(
    zClass_NodePartial *node,
    int repeat
);
int __fastcall SetLoop(
    zClass_NodePartial *node,
    int loop
);
int __fastcall SetPause(
    zClass_NodePartial *node,
    int paused
);
int __fastcall RemoveChild(
    zClass_NodePartial *parent,
    zClass_NodePartial *child
);
int __fastcall Update(zClass_NodePartial *node);
} // namespace zClass_Sequence

namespace Light {
int InitThermalGlowPool();
int DestroyThermalGlowPool();
zClass_NodePartial *__fastcall AllocFromFreeListAndAttach(
    zColorRgb *specularColor
);
void __fastcall ReturnToFreeList(zClass_NodePartial *lightNode);
} // namespace Light

namespace GameZ {
RECOIL_NO_GS int __fastcall WriteZBDFile(const char *filename);
RECOIL_NO_GS int __fastcall ReadZBDFile(const char *filename);
FILE *__fastcall OpenAndReadZBDHeader(
    const char *filename,
    zClass_ZbdHeader *outHeader
);
} // namespace GameZ

namespace GameZ_ZBD {
int __fastcall NodePtrToIndex(zClass_NodePartial *node);
zClass_NodePartial *__fastcall NodeIndexToPtr(int index);
int __fastcall WriteNodeRefListIndices(
    zClass_NodePartial **nodeRefList,
    int entryCount,
    void *stream
);
RECOIL_NO_GS int __fastcall WriteSingleNodeClassData(
    zClass_NodePartial *node,
    void *stream
);
int __fastcall WriteNodeTable(void *stream);
int __fastcall ReadNodeRefListIndices(
    zClass_NodePartial **nodeRefList,
    int entryCount,
    void *stream
);
int __fastcall ReadSingleNodeClassData(
    zClass_NodePartial *node,
    void *stream
);
int __fastcall ReadNodeTable(
    int nodeCount,
    void *stream
);
RECOIL_NO_GS int __fastcall ReloadDisplayInstancesFromCurrentPath_Local(
    zClass_NodePartial *node,
    int recurseChildren
);
RECOIL_NO_GS int __fastcall ReloadDisplayInstancesRecursive_Local(
    void *stream,
    zClass_ZbdHeader *zbdHeader,
    zClass_NodePartial *node,
    int recurseChildren
);
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
extern int g_zClass_RebuildGwWorldBltRectOnShutdown;
extern char g_zClass_GWWorldNodeName[8];
}

namespace zClass_TypeList {
// Source-faithful helper recovered from address-backed callers in this source file.
inline zClass_TypeListBucket &Bucket(
    int bucket
) {
    return *(zClass_TypeListBucket *)(g_zClass_TypeList_HeadSlotPtrs[bucket]);
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline zClass_TypeListLink *&Head(
    int bucket
) {
    return *g_zClass_TypeList_HeadSlotPtrs[bucket];
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline zClass_TypeListLink *&Tail(
    int bucket
) {
    return *g_zClass_TypeList_TailSlotPtrs[bucket];
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline int &PendingRemovalDirty(
    int bucket
) {
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

// Source-faithful helper recovered from address-backed callers in this source file.
inline void SetPendingRemovalDirty(
    int bucket,
    int value
) {
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
