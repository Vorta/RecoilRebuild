#ifndef RECOIL_GAMEZRECOIL_INCLUDE_ZCLASS_H
#define RECOIL_GAMEZRECOIL_INCLUDE_ZCLASS_H

#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>
#include <stdio.h>

#include "GameZRecoil/zMath/zmth_decls.h"
#include "recoil/recoil_callconv.h"

struct zBBoxCorners;
struct zDiPartial;
struct CZLightDataPartial;
struct CZSoundDataPartial;
struct zSndPlayHandle;
struct zSndSample;
struct zZbdSectionCallbackCtx;

struct CZNodePartial {
    char name[0x24];
    int flags;
    int auxFlags;
    int boundsFlags;
    unsigned char nodeType;
    unsigned char unknown_31[0x03];
    int classId;
    void *classData;
    unsigned int userDataOrDiRef;
    CZNodePartial *callbackContext;
    int callbackPriority;
    void *actionCallback;
    int gridCol;
    int gridRow;
    int listCountA;
    CZNodePartial **listA;
    int listCountB;
    CZNodePartial **listB;
    float cachedSphereCenter[4];
    float cachedBounds[6];
};

struct CZTypeListLink {
    CZNodePartial *node;
    CZTypeListLink *prev;
    CZTypeListLink *next;
    int pendingRemove;
};

struct CZTypeListBucket {
    CZTypeListLink *head;
    CZTypeListLink *tail;
    int pendingRemovalDirty;
};

typedef int(__fastcall *CZNodePredicate)(CZNodePartial *node);
typedef int(__fastcall *CZNodeActionCallback)(CZNodePartial *node);

struct CZNodeFreeListSlot {
    CZNodePartial node;
    zBBox3f primaryBounds;
    zBBox3f secondaryBounds;
    void *damageHandler;
    unsigned int freeTag;
};

struct OptCatalogDamageHandlerPartial {
    void *hitCallback;
    void *hitContext;
    void *timerContext;
    void *timerCallback;
};

/**
 * Original inline helper; no standalone retail function exists. Observed
 * zClass vector-field initialization callers include 0x452fd0, 0x453560, and
 * 0x4535c0 in D:\Proj\GameZRecoil\zClass\Light.c.
 * Purpose: construct a zVec3 value from explicit x, y, and z components.
 */
inline zVec3 zVec3Make(
    float x,
    float y,
    float z
) {
    zVec3 value = {x, y, z};
    return value;
}

struct CZWindowClearPoly {
    zVec3 vertices[4];
    int vertCount;
};

struct CZWindowDataPartial {
    int viewportWidth;
    int viewportHeight;
    int resolutionWidth;
    int resolutionHeight;
    CZWindowClearPoly clearPolys[4];
    int clearPolyIndexFlags;
    int bufferIndex;
    void *buffer;
    int fbWidth;
    int fbHeight;
    int fbBpp;
};

struct CZDisplayDataPartial {
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
    CZNodePartial **childList;
};

struct CZWorldDataPartial {
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
    CZNodePartial **lightNodes;
    CZLightDataPartial **lightDataList;
    int soundCount;
    CZNodePartial **soundNodes;
    CZSoundDataPartial **soundDataList;
    int areaGridExternalOwnership;
};

struct CZWorldSettingsSectionRecord {
    int fogState;
    zColorRgb fogColorRgb01;
    float fogRangeNear;
    float fogRangeFar;
    float fogAltitudeHigh;
    float fogAltitudeLow;
    float fogDensity;
};

struct CZSoundDataPartial {
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
    CZNodePartial **attachedWorlds;
};

struct CZSequenceEntryPartial {
    CZNodePartial *node;
    float triggerTime;
};

struct CZSequenceDataPartial {
    int isActive;
    int repeatAtBounds;
    int wrapAtBounds;
    int isPaused;
    int step;
    int currentIndex;
    float currentTime;
    int entryCount;
    CZSequenceEntryPartial entries[1];
};

struct CZSwitchDataPartial {
    int activeMaskIndex;
    unsigned int childMasks[1];
};

struct zTag4Partial {
    unsigned char count;
    unsigned char tags[3];
};

struct CZObject3DDataPartial {
    int flags;
    float alphaScale;
    zColorRgb color;
    float colorAlpha;
    zVec3 rotation;
    zVec3 scale;
    float localMatrix[12];
    float cachedWorldMatrix[12];
};

struct CZLodDataPartial {
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
    CZNodePartial *rangeNode;
    float rangeSq;
};

struct CZLightDataPartial {
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
    int isDirectional;
    int isDirectedSource;
    int isPointSource;
    int lightParam;
    int lightSubMode;
    float range1;
    float range2;
    float range2Sq;
    float invRangeDelta;
    int attachedWorldCount;
    CZNodePartial **attachedWorlds;
};

struct CZAnimateKeyframePartial {
    zVec3 rotation;
    zVec3 position;
    zVec3 scale;
};

struct CZAnimateRuntimePartial {
    unsigned char unknown_00[0x04];
    CZAnimateKeyframePartial *keyframes;
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

struct CZAnimateDataPartial {
    int flags;
    int statusFlags;
    float animatedTransform[12];
    float savedParentMatrix[12];
    CZAnimateRuntimePartial runtime;
};

struct CZCameraDataPartial {
    CZNodePartial *worldNode;
    CZNodePartial *windowNode;
    CZNodePartial *horizonNode;
    CZNodePartial *horizonXZNode;
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

struct CZZbdHeader {
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

struct CZRenderColorAlphaState {
    zColorRgb color;
    float alpha;
};

struct CZLodDistanceState {
    zVec3 center;
    float distanceSq;
};

typedef void(__fastcall *CZRenderFn)(
    CZNodePartial *node,
    int clipMask
);

RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        name
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        flags
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        auxFlags
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        boundsFlags
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        classId
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        name
    ) == 0x00
);
RECOIL_STATIC_ASSERT(sizeof(((CZNodePartial *)0)->name) == 0x24);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        nodeType
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        classData
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        userDataOrDiRef
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        callbackContext
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        callbackPriority
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        actionCallback
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        gridCol
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        gridRow
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        listCountA
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        listA
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        listCountB
    ) == 0x5c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        listB
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodePartial,
        cachedBounds
    ) == 0x74
);
RECOIL_STATIC_ASSERT(sizeof(CZNodePartial) == 0x8c);
RECOIL_STATIC_ASSERT(sizeof(CZTypeListLink) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZTypeListBucket,
        head
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZTypeListBucket,
        tail
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZTypeListBucket,
        pendingRemovalDirty
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(CZTypeListBucket) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodeFreeListSlot,
        primaryBounds
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodeFreeListSlot,
        secondaryBounds
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodeFreeListSlot,
        damageHandler
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZNodeFreeListSlot,
        freeTag
    ) == 0xc0
);
RECOIL_STATIC_ASSERT(sizeof(CZNodeFreeListSlot) == 0xc4);
RECOIL_STATIC_ASSERT(sizeof(CZZbdHeader) == 0x24);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZZbdHeader,
        texDirArg
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZZbdHeader,
        texDirOffset
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZZbdHeader,
        matlOffset
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZZbdHeader,
        model3dOffset
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZZbdHeader,
        nodeCount
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZZbdHeader,
        nodeFreeHead
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZZbdHeader,
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
        timerContext
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogDamageHandlerPartial,
        timerCallback
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogDamageHandlerPartial) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowClearPoly,
        vertices
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowClearPoly,
        vertCount
    ) == 0x30
);
RECOIL_STATIC_ASSERT(sizeof(CZWindowClearPoly) == 0x34);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DDataPartial,
        flags
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DDataPartial,
        alphaScale
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DDataPartial,
        color
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DDataPartial,
        colorAlpha
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DDataPartial,
        rotation
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DDataPartial,
        scale
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DDataPartial,
        localMatrix
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DDataPartial,
        cachedWorldMatrix
    ) == 0x60
);
RECOIL_STATIC_ASSERT(sizeof(CZObject3DDataPartial) == 0x90);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateDataPartial,
        statusFlags
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateDataPartial,
        animatedTransform
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateDataPartial,
        savedParentMatrix
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateDataPartial,
        runtime
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        keyframes
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        sampledRotation
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        sampledPosition
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        sampledScale
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        outputRotationScale
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        outputPositionScale
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        outputScaleScale
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        duration
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        currentTime
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        loopBase
    ) == 0x5c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        startTime
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        state
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        maxFrameIndex
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateRuntimePartial,
        loopCount
    ) == 0x6a
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateKeyframePartial,
        rotation
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateKeyframePartial,
        position
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateKeyframePartial,
        scale
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(CZAnimateKeyframePartial) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(CZAnimateRuntimePartial) == 0x6c);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateDataPartial,
        runtime
    ) +
        offsetof(
            CZAnimateRuntimePartial,
            sampledRotation
        ) ==
    0x70
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateDataPartial,
        runtime
    ) +
        offsetof(
            CZAnimateRuntimePartial,
            sampledPosition
        ) ==
    0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZAnimateDataPartial,
        runtime
    ) +
        offsetof(
            CZAnimateRuntimePartial,
            sampledScale
        ) ==
    0x88
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        viewportWidth
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        viewportHeight
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        resolutionWidth
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        resolutionHeight
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        clearPolys
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        clearPolyIndexFlags
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        bufferIndex
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        buffer
    ) == 0xe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        fbWidth
    ) == 0xec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        fbHeight
    ) == 0xf0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWindowDataPartial,
        fbBpp
    ) == 0xf4
);
RECOIL_STATIC_ASSERT(sizeof(CZWindowDataPartial) == 0xf8);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZDisplayDataPartial,
        x
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZDisplayDataPartial,
        y
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZDisplayDataPartial,
        width
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZDisplayDataPartial,
        height
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZDisplayDataPartial,
        backgroundR
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZDisplayDataPartial,
        backgroundG
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZDisplayDataPartial,
        backgroundB
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(CZDisplayDataPartial) == 0x1c);
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
        CZWorldDataPartial,
        pendingAreaUpdateCount
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        pendingAreaUpdateCapacity
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        pendingAreaUpdates
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        fogState
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        ambientColor
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        originX
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        originZ
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        worldMaxX
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        worldMaxZ
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        partitionMaxDecFeatureCount
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        clampQueriesToBounds
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaCellSizeX
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaCellSizeZ
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaHalfSizeX
    ) == 0x5c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaHalfSizeZ
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaInvSizeX
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaInvSizeZ
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaCellRadiusBias
    ) == 0x6c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        partitionInclusionTolX
    ) == 0x70
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        partitionInclusionTolZ
    ) == 0x74
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaGridColCount
    ) == 0x78
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaGridRowCount
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaGridRows
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        scaleX
    ) == 0x84
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        scaleY
    ) == 0x88
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        scaleZ
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        lightCount
    ) == 0x90
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        lightNodes
    ) == 0x94
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        lightDataList
    ) == 0x98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        soundCount
    ) == 0x9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        soundNodes
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        soundDataList
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldDataPartial,
        areaGridExternalOwnership
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(sizeof(CZWorldDataPartial) == 0xac);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldSettingsSectionRecord,
        fogState
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldSettingsSectionRecord,
        fogColorRgb01
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldSettingsSectionRecord,
        fogRangeNear
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldSettingsSectionRecord,
        fogRangeFar
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldSettingsSectionRecord,
        fogAltitudeHigh
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldSettingsSectionRecord,
        fogAltitudeLow
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZWorldSettingsSectionRecord,
        fogDensity
    ) == 0x20
);
RECOIL_STATIC_ASSERT(sizeof(CZWorldSettingsSectionRecord) == 0x24);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        sample
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        playHandle
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        sampleSetName
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        runtimeFlags
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        localPosition
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        worldPos
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        savedParentMatrix
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        falloffMode
    ) == 0x78
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        rangeMin
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        rangeMax
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        rangeMaxSq
    ) == 0x84
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        invRangeSpan
    ) == 0x88
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        attachedWorldCount
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSoundDataPartial,
        attachedWorlds
    ) == 0x90
);
RECOIL_STATIC_ASSERT(sizeof(CZSoundDataPartial) == 0x94);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceDataPartial,
        isActive
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceDataPartial,
        repeatAtBounds
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceDataPartial,
        wrapAtBounds
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceDataPartial,
        isPaused
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceDataPartial,
        step
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceDataPartial,
        currentIndex
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceDataPartial,
        currentTime
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceDataPartial,
        entryCount
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceDataPartial,
        entries
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceEntryPartial,
        node
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSequenceEntryPartial,
        triggerTime
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(CZSequenceEntryPartial) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSwitchDataPartial,
        activeMaskIndex
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZSwitchDataPartial,
        childMasks
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(CZRenderColorAlphaState) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(CZLodDistanceState) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        computeOwnDistance
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        nearRangeSq
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        nearRange
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        farRangeSq
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        fadeWidth
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        fadeAmount
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        fadeEndScale
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        fogFadeWidth
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        vertexShadingAmount
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        active
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        rangeNode
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLodDataPartial,
        rangeSq
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(sizeof(CZLodDataPartial) == 0x50);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        worldPosScratch
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        viewPos
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        localRotation
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        localPosition
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        worldRotation
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        worldPosition
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        savedParentMatrix
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        worldDir
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        falloff
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        intensityScale
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        specularColor
    ) == 0xac
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        isDirectional
    ) == 0xb8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        isDirectedSource
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        isPointSource
    ) == 0xc0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        lightParam
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        lightSubMode
    ) == 0xc8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        range1
    ) == 0xcc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        range2
    ) == 0xd0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        range2Sq
    ) == 0xd4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        invRangeDelta
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        attachedWorldCount
    ) == 0xdc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZLightDataPartial,
        attachedWorlds
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(sizeof(CZLightDataPartial) == 0xe4);
RECOIL_STATIC_ASSERT(sizeof(zTag4Partial) == 0x04);
RECOIL_STATIC_ASSERT(
    offsetof(
        zWorldAreaPartial,
        areaIndex
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        worldNode
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        windowNode
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        horizonNode
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        horizonXZNode
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        cameraFlags
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        targetOrEuler
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        posOffset
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        cameraPos
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        eulerAngles
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        worldTransform
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        forwardDir
    ) == 0x74
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        worldTarget
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        nearClip
    ) == 0xb0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        farClip
    ) == 0xb4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        nearClipCenter
    ) == 0xb8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        farClipCenter
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        clipDistance
    ) == 0xd0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        invClipDistanceSq
    ) == 0xd4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        viewportWidth
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        viewportHeight
    ) == 0xdc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        frustumWidth
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        frustumHeight
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        fovX
    ) == 0xe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        fovY
    ) == 0xec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        frustumYaw
    ) == 0xf0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        frustumPitch
    ) == 0xf4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        frustumVectorsDirty
    ) == 0xf8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        frustumOrigin
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        frustumCorners
    ) == 0x108
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        localFrustumNormalsDirty
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        localFrustumLeftNormal
    ) == 0x13c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        localFrustumRightNormal
    ) == 0x148
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        localFrustumBottomNormal
    ) == 0x154
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        localFrustumTopNormal
    ) == 0x160
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        localFrustumNearNormal
    ) == 0x16c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        localFrustumFarNormal
    ) == 0x178
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        transformDirty
    ) == 0x184
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        worldFrustumNormals
    ) == 0x188
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        viewportScaleX
    ) == 0x1d4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        viewportScaleY
    ) == 0x1d8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        variantOverrideEnabled
    ) == 0x1e0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZCameraDataPartial,
        variantTag
    ) == 0x1e4
);
RECOIL_STATIC_ASSERT(sizeof(CZCameraDataPartial) == 0x1e8);

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
extern zVec3 g_zCamera_FrustumFootprintPoints[5];
extern int g_zCamera_FrustumFootprintPointCount;
extern zCamera_FrustumGridTileRingPartial g_zCamera_FrustumGridTileRings[50];
extern int gModel_ClipMaskStack[0x10];
extern int *gModel_ClipMaskStackTop;
extern CZRenderFn gModel_RenderFn;
extern int g_CZClass_RenderBoundsContextActive;
extern int g_CZClass_RenderFrustumGridTileIndex;
extern int g_CZClass_RenderRangeFadeActive;
extern float g_CZClass_RenderRangeFadeScale;
extern int g_CZClass_RenderVertexAlphaOverrideActive;
extern int g_CZClass_RenderAlphaScaleStackTop;
extern float g_CZClass_RenderAlphaScaleStack[0x10];
extern int g_CZClass_SoftwarePathStateStackTop;
extern CZRenderColorAlphaState g_CZClass_SoftwarePathRenderStateStack[4];
extern int g_CZClass_LodDistanceStateStackTop;
extern CZLodDistanceState g_CZClass_LodDistanceStateStack[4];
}

/**
 * Original inline helper; no standalone retail function exists. Observed in
 * address-backed node-bounds callers including 0x448e90, 0x4491b0, and
 * 0x449420 as the typed access to the node free-list slot storage.
 * Purpose: view a scene node as the enclosing free-list slot record that owns
 * the cached primary and secondary bounds.
 */
inline CZNodeFreeListSlot *zClassNodeSlotFromNode(
    CZNodePartial *node
) {
    return (CZNodeFreeListSlot *)node;
}

/**
 * Original inline helper; no standalone retail function exists. Observed in
 * address-backed const node-bounds callers including 0x448e90 and bbox query
 * paths using the same free-list slot storage.
 * Purpose: view a const scene node as the enclosing free-list slot record that
 * owns the cached primary and secondary bounds.
 */
inline const CZNodeFreeListSlot *zClassNodeSlotFromNode(
    const CZNodePartial *node
) {
    return (const CZNodeFreeListSlot *)node;
}

/**
 * Source inline helper; retail sphere access is observed in the render
 * traversal callers at 0x44ada0, 0x44b300, and 0x44b8c0. The cached sphere
 * center occupies node bytes 0x64..0x6f, separately from the primary bounds.
 * Purpose: return the cached view-sphere center without aliasing the model
 * bounding box.
 */
inline zVec3 *zClassNodeViewSphereCenter(
    CZNodePartial *node
) {
    return (zVec3 *)node->cachedSphereCenter;
}

/**
 * Source inline helper; retail sphere access is observed in the render
 * traversal callers at 0x44ada0, 0x44b300, and 0x44b8c0. The cached sphere
 * center occupies node bytes 0x64..0x6f, separately from the primary bounds.
 * This overload preserves const access to that same cached sphere.
 * Purpose: return the const view-sphere center without aliasing the model
 * bounding box.
 */
inline const zVec3 *zClassNodeViewSphereCenter(
    const CZNodePartial *node
) {
    return (const zVec3 *)node->cachedSphereCenter;
}

/**
 * Source inline helper; retail sphere access is observed in the render
 * traversal callers at 0x44ada0, 0x44b300, and 0x44b8c0. The cached sphere
 * radius occupies node bytes 0x70..0x73, separately from the primary bounds.
 * Purpose: return the cached view-sphere radius without aliasing the model
 * bounding box.
 */
inline float *zClassNodeViewSphereRadius(
    CZNodePartial *node
) {
    return &node->cachedSphereCenter[3];
}

/**
 * Source inline helper; retail sphere access is observed in the render
 * traversal callers at 0x44ada0, 0x44b300, and 0x44b8c0. The cached sphere
 * radius occupies node bytes 0x70..0x73, separately from the primary bounds.
 * This overload preserves const access to that same cached sphere.
 * Purpose: return the const view-sphere radius without aliasing the model
 * bounding box.
 */
inline const float *zClassNodeViewSphereRadius(
    const CZNodePartial *node
) {
    return &node->cachedSphereCenter[3];
}

namespace CZBBox {
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
} // namespace CZBBox

namespace zTag4 {
void __fastcall Clear(zTag4Partial *tag);
}

namespace CZWindow {
CZNodePartial *__cdecl gwWindowNew();
int __fastcall DeleteNode(CZNodePartial *node);
int __fastcall gwWindowSetResolution(
    CZNodePartial *node,
    int width,
    int height
);
int __fastcall gwWindowGetResolution(
    CZNodePartial *node,
    int *outWidth,
    int *outHeight
);
int __fastcall gwWindowSetSize(
    CZNodePartial *node,
    int width,
    int height
);
int __fastcall gwWindowGetSize(
    CZNodePartial *node,
    int *outWidth,
    int *outHeight
);
int __fastcall gwWindowSetBuffer(
    CZNodePartial *node,
    int bufferIndex
);
int __fastcall gwWindowSetClearPolygon(
    CZNodePartial *node,
    int enabled
);
int __fastcall gwWindowAddClearPolygonVertex(
    CZNodePartial *node,
    const zVec3 *point
);
int __fastcall gwWindowCloseClearPolygon(CZNodePartial *node);
} // namespace CZWindow

namespace CZDisplay {
CZNodePartial *__cdecl gwDisplayInit();
int __fastcall DeleteNode(CZNodePartial *node);
int __fastcall RemoveChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall gwDisplaySetSize(
    CZNodePartial *node,
    int width,
    int height
);
int __fastcall gwDisplaySetPosition(
    CZNodePartial *node,
    int x,
    int y
);
int __fastcall gwDisplaySetBackgroundColor(
    CZNodePartial *node,
    float red,
    float green,
    float blue
);
} // namespace CZDisplay

namespace CZWorld {
int __fastcall WriteSettingsSection(
    zZbdSectionCallbackCtx *callbackCtx,
    void *userData
);
void __fastcall ReadSettingsSection(
    zZbdSectionCallbackCtx *callbackCtx,
    const char *worldName,
    CZWorldSettingsSectionRecord *settings,
    unsigned int size,
    void *userData
);
CZNodePartial *__cdecl gwWorldNew();
int __fastcall DeleteNode(CZNodePartial *world);
int __fastcall FreeVirtualAreaPartitions(CZNodePartial *world);
int __fastcall QueueAreaUpdate(
    CZNodePartial *world,
    CZWorldDataPartial *worldData,
    zWorldAreaPartial *area
);
int __fastcall RebuildAreaBounds(
    CZWorldDataPartial *worldData,
    zWorldAreaPartial *area
);
int __fastcall ApplyPendingFogSettings(CZNodePartial *world);
int __fastcall SetPendingFogState(
    CZNodePartial *world,
    int fogState
);
int __fastcall SetPendingFogColorRgb01(
    CZNodePartial *world,
    float red,
    float green,
    float blue
);
int __fastcall SetPendingFogAltitudeRange(
    CZNodePartial *world,
    float minAlt,
    float maxAlt
);
int __fastcall SetPendingFogRange(
    CZNodePartial *world,
    float nearRange,
    float farRange
);
int __fastcall GetPendingFogDensity(
    CZNodePartial *world,
    float *outDensity
);
int __fastcall GetPendingFogState(
    CZNodePartial *world,
    int *outState
);
int __fastcall GetPendingFogColorRgb01(
    CZNodePartial *world,
    float *outRed,
    float *outGreen,
    float *outBlue
);
int __fastcall GetPendingFogRange(
    CZNodePartial *world,
    float *outNearRange,
    float *outFarRange
);
int __fastcall GetPendingFogAltitudeRange(
    CZNodePartial *world,
    float *outMinAlt,
    float *outMaxAlt
);
int __fastcall SetPendingFogDensity(
    CZNodePartial *world,
    float density
);
int __fastcall gwWorldSetOrigin(
    CZNodePartial *world,
    float originX,
    float originZ
);
int __fastcall gwWorldSetSize(
    CZNodePartial *world,
    float sizeX,
    float sizeZ
);
int __fastcall gwWorldSetPartitionInclusionTolerance(
    CZNodePartial *world,
    float toleranceX,
    float toleranceZ
);
int __fastcall gwWorldSetMaxDecFeatures(
    CZNodePartial *world,
    int maxFeatures
);
int __fastcall gwWorldSetVirtualAreaPartition(
    CZNodePartial *world,
    float cellSizeX,
    float cellSizeZ
);
int __fastcall InitVirtualAreaPartitions(CZNodePartial *world);
int __fastcall SetVirtualPartition(
    CZNodePartial *world,
    int enabled
);
int __fastcall WorldRectToGridIndex(
    CZNodePartial *world,
    int *outGridCol,
    float minX,
    float maxX,
    float minZ,
    float maxZ,
    int *outGridRow
);
int __fastcall WorldToGridCoordsClampedEx(
    CZNodePartial *world,
    int *outGridCol,
    float worldX,
    float worldZ,
    int *outGridRow,
    int *clampedGridColOut,
    int *clampedGridRowOut,
    int *insideBoundsOut
);
int __fastcall WorldToGridCoordsClamped(
    CZNodePartial *world,
    int *outGridCol,
    float worldX,
    float worldZ,
    int *outGridRow
);
zWorldAreaPartial *__fastcall GetAreaPartitionAtGrid(
    CZNodePartial *world,
    int gridCol,
    int gridRow
);
int __fastcall AddChildAtGrid(
    CZNodePartial *world,
    CZNodePartial *child
);
int __fastcall EnsureGridCellDisplayPosition(
    CZNodePartial *world,
    int gridCol,
    int gridRow
);
int __fastcall AddChildToGridCell(
    CZNodePartial *world,
    CZNodePartial *child,
    int gridCol,
    int gridRow
);
int __fastcall RemoveChildAtGrid(
    CZNodePartial *world,
    CZNodePartial *child
);
int __fastcall AddLight(
    CZNodePartial *world,
    CZNodePartial *light
);
int __fastcall RemoveLight(
    CZNodePartial *world,
    CZNodePartial *light
);
int __fastcall InitLightPointInPolygonXZ(CZNodePartial *world);
int __fastcall UpdateAllLights(CZNodePartial *world);
int __fastcall AddSound(
    CZNodePartial *world,
    CZNodePartial *sound
);
int __fastcall RemoveSound(
    CZNodePartial *world,
    CZNodePartial *sound
);
int __fastcall UpdateAllSounds(CZNodePartial *world);
} // namespace CZWorld

namespace CZObject3D {
CZNodePartial *__cdecl gwObject3DInit();
int __fastcall RenderTraverse(
    CZNodePartial *node,
    int siblingCountHint
);
int __fastcall PropagateTransformDirty(CZNodePartial *node);
int __fastcall gwObject3DSetVisibleFlag(
    CZNodePartial *node,
    int visible
);
int __fastcall gwObject3DSetColorAlpha(
    CZNodePartial *node,
    zColorRgb *color,
    float alpha
);
int __fastcall gwObject3DSetAlphaScale(
    CZNodePartial *node,
    float alphaScale
);
int __fastcall gwObject3DGetAlphaScale(
    CZNodePartial *node,
    float *outAlphaScale
);
int __fastcall gwObject3DSetLitFlag(
    CZNodePartial *node,
    int lit
);
int __fastcall gwObject3DSetScale(
    CZNodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwObject3DGetScale(
    CZNodePartial *node,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwObject3DGetRotation(
    CZNodePartial *node,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwObject3DSetRotation(
    CZNodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwObject3DTranslateRotation(
    CZNodePartial *node,
    float dx,
    float dy,
    float dz
);
int __fastcall gwObject3DGetPosition(
    CZNodePartial *node,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwObject3DSetPosition(
    CZNodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwObject3DTranslatePosition(
    CZNodePartial *node,
    float dx,
    float dy,
    float dz
);
float *__fastcall gwObject3DGetMatrixPtr(CZNodePartial *node);
int __fastcall gwObject3DSetMatrix(
    CZNodePartial *node,
    float *matrix
);
int __fastcall gwObject3DAddChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall RemoveChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall DeleteNode(CZNodePartial *node);
} // namespace CZObject3D

struct CZObject3DModelRefLerpTask {
    CZNodePartial *node;
    void *callbackCtx;
    void *onComplete;
    int invertModelRef;
    float targetModelRef;
    float currentModelRef;
    float modelRefDeltaPerSec;
    CZObject3DModelRefLerpTask *next;
};
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DModelRefLerpTask,
        callbackCtx
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DModelRefLerpTask,
        onComplete
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DModelRefLerpTask,
        invertModelRef
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DModelRefLerpTask,
        targetModelRef
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DModelRefLerpTask,
        currentModelRef
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DModelRefLerpTask,
        modelRefDeltaPerSec
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZObject3DModelRefLerpTask,
        next
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(sizeof(CZObject3DModelRefLerpTask) == 0x20);

struct CZObject3DModelRefLerpQueueState {
    unsigned int listAux;
    CZObject3DModelRefLerpTask *head;
    CZObject3DModelRefLerpTask *tail;
    unsigned int count;
    CZObject3DModelRefLerpQueueState();
};
RECOIL_STATIC_ASSERT(sizeof(CZObject3DModelRefLerpQueueState) == 0x10);

extern "C" {
extern CZObject3DModelRefLerpQueueState g_ModelRefLerpQueueState;
}

typedef void(__fastcall *CZObject3DModelRefLerpCallback)(void *callbackCtx);

namespace CZObject3DModelRefLerpQueue {
void __fastcall Add(
    CZNodePartial *node,
    void *callbackCtx,
    void *onComplete,
    float startModelRef,
    float targetModelRef,
    float durationSec
);
void __cdecl Reset();
void __cdecl Update();
} // namespace CZObject3DModelRefLerpQueue

namespace CZLod {
CZNodePartial *__cdecl gwLodNew();
int __fastcall DeleteNode(CZNodePartial *node);
int __fastcall RenderTraverse(
    CZNodePartial *node,
    int siblingCountHint
);
int __fastcall gwLodAddChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall RemoveChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall SetComputeOwnDistance(
    CZNodePartial *node,
    int enabled
);
int __fastcall SetTargetNodeAndRange(
    CZNodePartial *node,
    CZNodePartial *target,
    float range
);
} // namespace CZLod

namespace CZLight {
CZNodePartial *__cdecl gwLightNew();
int __fastcall RenderTraverse(
    CZNodePartial *node,
    int siblingCountHint
);
int __fastcall DeleteNode(CZNodePartial *node);
int __fastcall RemoveChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall gwLightSetIntensity(
    CZNodePartial *node,
    float intensity
);
int __fastcall gwLightSetFalloff(
    CZNodePartial *node,
    float falloff
);
int __fastcall gwLightSetDirectional(
    CZNodePartial *node,
    int directional
);
int __fastcall gwLightSetDirectedSource(CZNodePartial *node);
int __fastcall gwLightSetPointSource(CZNodePartial *node);
int __fastcall gwLightSetParam(
    CZNodePartial *node,
    int param
);
int __fastcall gwLightSetRange(
    CZNodePartial *node,
    float rangeA,
    float rangeB
);
int __fastcall gwLightGetRange(
    CZNodePartial *node,
    float *outRange1,
    float *outRange2
);
int __fastcall gwLightSetPosition(
    CZNodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwLightSetRotation(
    CZNodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall ComputeWorldTransform(
    CZNodePartial *node,
    CZLightDataPartial *data
);
int __fastcall gwLightUpdate(CZNodePartial *node);
int __fastcall gwLightGetSpecularColor(
    CZNodePartial *node,
    float *outRed,
    float *outGreen,
    float *outBlue
);
int __fastcall gwLightSetSpecularColor(
    CZNodePartial *node,
    float red,
    float green,
    float blue
);
} // namespace CZLight

namespace CZCamera {
CZNodePartial *__cdecl gwCameraNew();
int __fastcall DeleteNode(CZNodePartial *node);
int __fastcall RenderTraverse(
    CZNodePartial *node,
    int siblingCountHint
);
int __fastcall gwCameraAddChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall gwCameraRemoveChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall gwCameraSetActive(
    CZNodePartial *node,
    int active
);
int __fastcall gwCameraSetFlagBit0(
    CZNodePartial *node,
    int enabled
);
int __fastcall SetTargetNode(CZNodePartial *target);
CZNodePartial *__fastcall SetActiveCamera(CZNodePartial *camera);
int __fastcall SetObjectHseTestEnabled(int enabled);
int __fastcall gwCameraSetWorld(
    CZNodePartial *camera,
    CZNodePartial *world
);
CZNodePartial *__fastcall gwCameraGetWorld(CZNodePartial *camera);
int __fastcall gwCameraSetWindow(
    CZNodePartial *camera,
    CZNodePartial *window
);
int __fastcall ActivateChildren(
    CZNodePartial *camera,
    CZCameraDataPartial *data
);
int __fastcall gwCameraSetPosition(
    CZNodePartial *camera,
    float x,
    float y,
    float z
);
int __fastcall gwCameraTranslate(
    CZNodePartial *camera,
    float dx,
    float dy,
    float dz
);
int __fastcall gwCameraGetPosition(
    CZNodePartial *camera,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwCameraSetTarget(
    CZNodePartial *camera,
    float x,
    float y,
    float z
);
int __fastcall gwCameraTranslateTarget(
    CZNodePartial *camera,
    float dx,
    float dy,
    float dz
);
int __fastcall gwCameraGetTarget(
    CZNodePartial *camera,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall gwCameraSetNearFarClip(
    CZNodePartial *camera,
    float nearClip,
    float farClip
);
int __fastcall gwCameraGetNearFarClip(
    CZNodePartial *camera,
    float *outNear,
    float *outFar
);
int __fastcall gwCameraSetViewport(
    CZNodePartial *camera,
    float viewportWidth,
    float viewportHeight
);
int __fastcall gwCameraGetViewport(
    CZNodePartial *camera,
    float *outWidth,
    float *outHeight
);
int __fastcall gwCameraGetFOV(
    CZNodePartial *camera,
    float *outFovX,
    float *outFovY
);
int __fastcall gwCameraSetFOV(
    CZNodePartial *camera,
    float fovX,
    float fovY
);
int __fastcall gwCameraGetClipDistance(
    CZNodePartial *camera,
    float *outClipDistance
);
int __fastcall gwCameraSetClipDistance(
    CZNodePartial *camera,
    float clipDistance
);
int __fastcall gwCameraSetHorizon(
    CZNodePartial *camera,
    CZNodePartial *horizonNode
);
int __fastcall gwCameraSetHorizonXZ(
    CZNodePartial *camera,
    CZNodePartial *horizonXZNode
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
    CZNodePartial *world,
    CZWorldDataPartial *worldData,
    CZCameraDataPartial *cameraData
);
int __fastcall BuildFrustumGridTilesFromParams(
    CZNodePartial *world,
    CZWorldDataPartial *worldData,
    CZCameraDataPartial *cameraData
);
int __fastcall RenderFrustumGridTiles(
    CZNodePartial *world,
    CZNodePartial *camera,
    CZCameraDataPartial *cameraData
);
void __fastcall RenderOverlayNodes(CZNodePartial *world);
void __fastcall RenderWorld(
    CZNodePartial *world,
    CZNodePartial *camera,
    CZCameraDataPartial *cameraData
);
int __fastcall gwCameraSetVariantTagOverride(
    CZNodePartial *camera,
    zTag4Partial *variantTag
);
int __fastcall RenderScene(
    CZNodePartial *camera,
    int updateFxPass3Local
);
int __fastcall BuildWorldTransform(
    CZNodePartial *camera,
    CZCameraDataPartial *data,
    zVec3 *posOffset
);
int __fastcall UpdateImpl(
    CZNodePartial *camera,
    zVec3 *posOffset
);
int __fastcall gwCameraUpdate(CZNodePartial *camera);
void __cdecl SyncViewContextPositions();
} // namespace CZCamera

namespace CZNode {
int __fastcall ClearPickupFlagsRecursive(CZNodePartial *node);
int __fastcall SetPickupFlagsRecursive(CZNodePartial *node);
void __fastcall PropagateTransformDirtyRecursive(CZNodePartial *self);
void __fastcall MaskExtraFlagsRecursive(
    CZNodePartial *self,
    int mask
);
void __fastcall PropagateExtraFlagsRecursive(
    CZNodePartial *self,
    int flags
);
void __fastcall PropagateFlagsRecursive(
    CZNodePartial *self,
    int flags
);
void __fastcall SetContextRecursive(
    CZNodePartial *self,
    CZNodePartial *context,
    int flagMask
);
void __fastcall SetDiFlagBit0Recursive(
    CZNodePartial *node,
    int enabled
);
int __fastcall HasRenderableDiPredicate(CZNodePartial *node);
void __fastcall SetMaterialFlagBit9ForFlagBit0EntriesRecursive(
    CZNodePartial *node,
    int enabled
);
void __fastcall InvalidateFlagBit8MaterialImagesRecursive(
    CZNodePartial *node
);
void __fastcall LoadFlagBit8MaterialImagesAndTexturePack(
    CZNodePartial *node
);
void __fastcall AssignInt32ToDiRecursive(
    CZNodePartial *node,
    int value
);
void __fastcall AssignDamageHandlerRecursiveIfMissing(
    CZNodePartial *node,
    OptCatalogDamageHandlerPartial *handler
);
void __fastcall ClearDamageHandlerRecursive(
    CZNodePartial *node,
    OptCatalogDamageHandlerPartial *handler
);
int __fastcall SetDamageHitCallback(
    void *context,
    CZNodePartial *node,
    void *callback
);
int __fastcall ClearDamageHandler(CZNodePartial *node);
int __fastcall SetDamageTimerCallback(
    void *context,
    CZNodePartial *node,
    void *callback
);
} // namespace CZNode

namespace CZTypeList {
CZTypeListLink *__cdecl AllocLink();
void __fastcall FreeLink(CZTypeListLink *link);
void __cdecl FreeAll();
void __fastcall ProcessPendingRemovals(int bucket);
int __fastcall CountNodes(int bucket);
void __fastcall PrintBucket(int bucket);
CZTypeListLink *__fastcall GetBucketHead(int bucket);
int __fastcall MarkPendingRemoval(
    int bucket,
    CZNodePartial *node
);
int __fastcall Insert(
    int bucket,
    CZNodePartial *node
);
int __fastcall InsertChildNodes(
    int bucket,
    CZNodePartial *node
);
void __cdecl UpdateAllBuckets();
void __fastcall UpdateBucket(CZTypeListLink *bucket);
int __cdecl UpdateQueuedTrees();
int __cdecl UpdateSequences();
int __cdecl UpdateAnimations();
} // namespace CZTypeList

namespace CZNode {
int __fastcall gwNodeBuildNodeToAncestorMatrix(
    CZNodePartial *node,
    int matMode
);
int __fastcall GetWorldPosition(
    CZNodePartial *node,
    zVec3 *outPosition
);
int __fastcall TransformPoint(
    CZNodePartial *node,
    zVec3 *point
);
int __fastcall GetWorldPosAndOrientation(
    CZNodePartial *node,
    zVec3 *inOutPosition,
    zVec3 *outOrientation
);
int __fastcall UpdateSubtree(CZNodePartial *node);
void __fastcall UpdateTree(CZNodePartial *node);
} // namespace CZNode

namespace CZNodeList {
int __fastcall Insert(CZNodePartial *node);
void __cdecl ProcessPendingFrees();
} // namespace CZNodeList

namespace CZList {
int __fastcall DeleteNodeFromLists(CZNodePartial *node);
int __fastcall _gwListDeleteANode(CZNodePartial *node);
int __fastcall DeleteAllOfType(int bucket);
int __cdecl RenderActiveCameras();
CZNodePartial *__fastcall IterateBucketFiltered(
    const char *filterText,
    int bucket,
    CZNodePredicate predicate
);
} // namespace CZList

namespace CZClass {
void __fastcall SetNodeArraySize(int size);
int __cdecl IsInitialized();
int __cdecl Init();
int __cdecl ResetCurrentZbdPath();
int __cdecl ShutdownCore();
int __cdecl Shutdown();
int __cdecl ProcessDeferredWork();
int __fastcall NodePtrToValidatedIndex(CZNodePartial *node);
CZNodePartial *__fastcall FindByTypeAndName(
    int bucket,
    const char *name
);
int __fastcall FindNextByTypePrefixPredicate(CZNodePartial *node);
CZNodePartial *__fastcall FindNextByTypePrefix(
    const char *prefixText,
    int bucket
);
int __fastcall AnyNodeMatchesPredicateRecursive(
    CZNodePartial *root,
    CZNodePredicate predicate
);
int __fastcall RemoveChildChecked(
    CZNodePartial *parent,
    CZNodePartial *child
);
} // namespace CZClass

namespace CZClass {
CZNodePartial *__cdecl gwNodeNew();
int __fastcall DeleteNodeByType(CZNodePartial *node);
int __fastcall gwNodeUpdate(CZNodePartial *node);
int __cdecl gwNodeUpdateAll();
int __fastcall gwNodeUpdateDisplayInstance(CZNodePartial *node);
int __fastcall gwNodeGetBBox(
    CZNodePartial *node,
    zBBox3f *outBBox
);
int __fastcall gwNodeGetWorldBBoxCorners(
    CZNodePartial *node,
    zBBoxCorners *outCorners
);
int __fastcall gwNodeGetViewBBoxCorners(
    CZNodePartial *node,
    zBBoxCorners *outCorners
);
int __fastcall gwNodeComputeChildBBox(CZNodePartial *node);
int __fastcall gwNodeRecalcBBox(CZNodePartial *node);
int __fastcall gwNodeSetActive(
    CZNodePartial *node,
    int active
);
int __fastcall gwNodeSetFlag16(
    CZNodePartial *node,
    int value
);
int __fastcall gwNodeSetFlag17(
    CZNodePartial *node,
    int value
);
int __fastcall gwNodeSetDisplayInstance(
    CZNodePartial *node,
    zDiPartial *displayInstance
);
int __fastcall gwNodeSetName(
    CZNodePartial *node,
    const char *name
);
char *__fastcall gwNodeGetName(CZNodePartial *node);
int __fastcall gwNodeGetUserData(
    CZNodePartial *node,
    unsigned int *outData
);
int __fastcall gwNodeSetActionCallback(
    CZNodePartial *node,
    void *actionCallback
);
int __fastcall gwNodeSetActionCallbackTail(
    CZNodePartial *node,
    void *actionCallback
);
int __fastcall gwNodeSetPriority(
    CZNodePartial *node,
    int priority
);
int __fastcall gwNodeSetCellPickable(
    CZNodePartial *node,
    int value
);
int __fastcall gwNodeGetCellPickable(
    CZNodePartial *node,
    int *outValue
);
int __fastcall gwNodeGetNodeType(
    CZNodePartial *node,
    int *outValue
);
int __fastcall gwNodeSetRaycastable(
    CZNodePartial *node,
    int value
);
int __fastcall gwNodeGetRaycastable(
    CZNodePartial *node,
    int *outValue
);
int __fastcall gwNodeSetPickable(
    CZNodePartial *node,
    int value
);
int __fastcall gwNodeGetPickable(
    CZNodePartial *node,
    int *outValue
);
int __fastcall gwNodeSetHasHitCallback(
    CZNodePartial *node,
    int value
);
int __fastcall gwNodeSetBypassFarClip(
    CZNodePartial *node,
    int value
);
int __fastcall gwNodeSetNodeType(
    CZNodePartial *node,
    int nodeType
);
int __fastcall gwNodeClearVariantGate(
    CZNodePartial *node,
    int value
);
int __fastcall gwNodeSetVertexAlphaOverride(
    CZNodePartial *node,
    int value
);
CZNodePartial *__fastcall gwNodeGetRoot(CZNodePartial *node);
CZNodePartial *__fastcall gwNodeGetWorldChild(CZNodePartial *node);
int __fastcall gwNodeFindNextByNamePredicate(CZNodePartial *node);
CZNodePartial *__fastcall gwNodeFindNextByName(
    const char *name,
    int bucket
);
CZNodePartial *__fastcall FindSubNodeByName(
    CZNodePartial *root,
    const char *name
);
CZNodePartial *__fastcall FindNodeRecursiveByName(
    CZNodePartial *root,
    const char *name
);
int __fastcall SetSingleParentFlagRecursive(
    CZNodePartial *node,
    int setFlag
);
int __fastcall AddChildValidated(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall RemoveChildValidated(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall AddChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall AddChildGeneric(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall RemoveChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall RemoveChildGeneric(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall FreeNodeToFreeList(CZNodePartial *node);
int __fastcall TryFreeNode(CZNodePartial *node);
int __fastcall gwNodeRenderDispatch(
    CZNodePartial *node,
    int siblingCountHint
);
} // namespace CZClass

namespace CZSound {
int __fastcall RenderTraverse(
    CZNodePartial *node,
    int siblingCountHint
);
}

namespace CZAnimate {
int __fastcall RenderTraverse(
    CZNodePartial *node,
    int siblingCountHint
);
}

namespace CZSequence {
int __fastcall RenderTraverse(
    CZNodePartial *node,
    int siblingCountHint
);
}

namespace CZSwitch {
int __fastcall DeleteNode(CZNodePartial *node);
int __fastcall RenderTraverse(
    CZNodePartial *node,
    int siblingCountHint
);
}

namespace CZUtil {
int __fastcall DestroyNodeRecursive(CZNodePartial *node);
}

namespace CZUtil {
int __fastcall CopyNodeDisplayInstance(
    CZNodePartial *source,
    CZNodePartial *dest
);
int __fastcall CopyNodeBaseData(
    CZNodePartial *source,
    CZNodePartial *dest
);
CZNodePartial *__fastcall CopyLightNode(
    CZNodePartial *source
);
CZNodePartial *__fastcall CopySoundNode(
    CZNodePartial *source
);
CZNodePartial *__fastcall CopyCameraNode(CZNodePartial *source);
CZNodePartial *__fastcall CopyObject3DNode(CZNodePartial *source);
CZNodePartial *__fastcall CopyAnimateNode(
    CZNodePartial *source
);
CZNodePartial *__fastcall CopyLodNode(CZNodePartial *source);
CZNodePartial *__fastcall CopySequenceNode(
    CZNodePartial *source
);
CZNodePartial *__fastcall CopySwitchNode(CZNodePartial *source);
CZNodePartial *__fastcall CopyNodeDispatch(CZNodePartial *source);
CZNodePartial *__fastcall CopyNodeWithCloneOptions(
    CZNodePartial *source,
    int cloneDiMode,
    int diArg0
);
CZNodePartial *__fastcall CopyNode(
    CZNodePartial *source,
    int cloneDiMode,
    int diArg0,
    int diArg1
);
} // namespace CZUtil

namespace CZSound {
CZNodePartial *__cdecl gwSoundNew();
int __fastcall DeleteNode(CZNodePartial *node);
int __fastcall RemoveChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall SetSampleSetByName(
    CZNodePartial *node,
    const char *name
);
int __fastcall gwSoundSetActive(
    CZNodePartial *node,
    int active
);
int __fastcall gwSoundSetPosition(
    CZNodePartial *node,
    float x,
    float y,
    float z
);
int __fastcall gwSoundGetPosition(
    CZNodePartial *node,
    float *outX,
    float *outY,
    float *outZ
);
int __fastcall UpdatePlayback(CZNodePartial *node);
int __fastcall ComputeWorldTransform(
    CZNodePartial *node,
    CZSoundDataPartial *soundData
);
} // namespace CZSound

namespace CZAnimate {
short __fastcall AdvanceTime(
    CZAnimateRuntimePartial *runtime,
    float deltaTime
);
short __fastcall SampleTransform(CZAnimateRuntimePartial *runtime);
int __fastcall UpdateNode(CZNodePartial *node);
int __fastcall AddChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall DeleteNode(CZNodePartial *node);
int __fastcall RemoveChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
} // namespace CZAnimate

namespace CZSequence {
CZNodePartial *__cdecl gwSequenceNew();
int __fastcall DeleteNode(CZNodePartial *node);
int __fastcall gwSequenceAddChild(
    CZNodePartial *parent,
    CZNodePartial *child,
    int insertIndex,
    float delay
);
int __fastcall SetActive(
    CZNodePartial *node,
    int active
);
int __fastcall SetRepeat(
    CZNodePartial *node,
    int repeat
);
int __fastcall SetLoop(
    CZNodePartial *node,
    int loop
);
int __fastcall SetPause(
    CZNodePartial *node,
    int paused
);
int __fastcall RemoveChild(
    CZNodePartial *parent,
    CZNodePartial *child
);
int __fastcall Update(CZNodePartial *node);
} // namespace CZSequence

namespace CZLight {
int __cdecl InitThermalGlowPool();
int __cdecl DestroyThermalGlowPool();
CZNodePartial *__fastcall AllocFromFreeListAndAttach(
    zColorRgb *specularColor
);
void __fastcall ReturnToFreeList(CZNodePartial *lightNode);
} // namespace CZLight

namespace CZZbd {
RECOIL_NO_GS int __fastcall WriteZBDFile(const char *filename);
RECOIL_NO_GS int __fastcall ReadZBDFile(const char *filename);
FILE *__fastcall OpenAndReadZBDHeader(
    const char *filename,
    CZZbdHeader *outHeader
);
} // namespace CZZbd

namespace CZZbd {
int __fastcall NodePtrToIndex(CZNodePartial *node);
CZNodePartial *__fastcall NodeIndexToPtr(int index);
int __fastcall WriteNodeRefListIndices(
    CZNodePartial **nodeRefList,
    int entryCount,
    void *stream
);
RECOIL_NO_GS int __fastcall WriteSingleNodeClassData(
    CZNodePartial *node,
    void *stream
);
int __fastcall WriteNodeTable(void *stream);
int __fastcall ReadNodeRefListIndices(
    CZNodePartial **nodeRefList,
    int entryCount,
    void *stream
);
int __fastcall ReadSingleNodeClassData(
    CZNodePartial *node,
    void *stream
);
int __fastcall ReadNodeTable(
    int nodeCount,
    void *stream
);
RECOIL_NO_GS int __fastcall ReloadDisplayInstancesFromCurrentPath_Local(
    CZNodePartial *node,
    int recurseChildren
);
RECOIL_NO_GS int __fastcall ReloadDisplayInstancesRecursive_Local(
    void *stream,
    CZZbdHeader *zbdHeader,
    CZNodePartial *node,
    int recurseChildren
);
} // namespace CZZbd

extern "C" {
extern CZNodeFreeListSlot *g_CZClass_NodeArray;
extern int g_CZClass_NodeArraySize;
extern int g_CZClass_ActiveNodeCount;
extern int g_CZClass_NodeFreeHeadIndex;
extern int g_CZClass_IsInitialized;
extern CZTypeListLink *g_CZTypeList_FreeLinkHead;
extern CZTypeListLink *g_CZNodeList_PendingFreeHead;
extern int g_CZClass_DeferredProcessingEnabled;
extern int g_CZTypeList_LiveLinkCount;
extern int g_CZTypeList_PeakLiveLinkCount;
extern CZTypeListBucket g_CZTypeList_Buckets[16];
extern CZTypeListLink **g_CZClassCallbackPriorityHeadSlotPtrs[6];
extern CZTypeListLink **g_CZTypeList_HeadSlotPtrs[16];
extern CZTypeListLink **g_CZTypeList_TailSlotPtrs[16];
extern CZTypeListLink *g_CZClass_FilterIterCursor;
extern unsigned int g_CZClass_FilterIterUnknownDword0;
extern const char *g_CZClass_FilterIterText;
extern unsigned int g_CZClass_FilterIterUnknownDword1;
extern int g_CZClass_FilterIterPrefixLen;
extern char g_CZClass_CurrentZbdPath[0x30];
extern CZNodePartial **g_GameZ_Zbd_NodeIndexScratch;
extern int g_GameZ_Zbd_NodeIndexScratchCapacity;
extern int g_CZClass_CameraAutoClipDistanceAdjustEnabled;
extern float g_CZClass_CameraAutoClipDistanceThreshold;
extern float g_CZClass_CameraAutoClipDistanceScale;
extern float g_CZClass_CameraAutoClipDistanceStep;
extern float g_CZClass_CameraAutoClipDistanceMinScale;
extern int g_CZClass_ObjectHseTestEnabled;
extern CZNodePartial *g_CZClass_CurrentCamera;
extern CZNodePartial *g_CZClass_CameraTargetNode;
extern char g_CZClass_VapStaticsNodeName[0x0c];
extern CZNodePartial *g_MainCamera;
extern CZCameraDataPartial *g_zVideo_pActiveViewContext;
extern CZNodePartial *g_Player_RuntimeDiScene;
extern int g_CZClass_CopyNodeCloneDiMode;
extern int g_CZClass_CopyNodeDiArg0;
extern int g_CZClass_CopyNodeDiArg1;
extern int g_CZClass_RebuildGwWorldBltRectOnShutdown;
extern char g_CZClass_GWWorldNodeName[8];
}

namespace CZTypeList {
/**
 * Original inline helper; no standalone retail function exists. Observed in
 * the List.c type-list accessor cluster used by callers including 0x44e700,
 * 0x44e920, 0x44ed90, 0x44ee10, and 0x44eed0; evidence basis is the repeated
 * bucket head, tail, and dirty-field access through the recovered bucket slot
 * tables and g_CZTypeList_Buckets.
 * Purpose: recover typed bucket record access for shared type-list bucket
 * fields while preserving the proven bucket mapping.
 */
inline CZTypeListBucket &Bucket(
    int bucket
) {
    return *(CZTypeListBucket *)(g_CZTypeList_HeadSlotPtrs[bucket]);
}

/**
 * Original inline helper; no standalone retail function exists. Observed in
 * address-backed List.c callers including 0x44e700, 0x44ed90, 0x44ee10, and
 * the active 0x44f000 -> 0x44eed0 deferred-removal path as the repeated
 * mutable type-list head slot access.
 * Purpose: return the bucket head link slot used by type-list traversal,
 * insertion, and pending-removal processing.
 */
inline CZTypeListLink *&Head(
    int bucket
) {
    return *g_CZTypeList_HeadSlotPtrs[bucket];
}

/**
 * Original inline helper; no standalone retail function exists. Observed in
 * address-backed List.c callers including 0x44e700, 0x44ed90, 0x44ee10, and
 * the active 0x44f000 deletion cluster through tail repair after list updates.
 * Purpose: return the bucket tail link slot used when appending, trimming, or
 * clearing type-list buckets.
 */
inline CZTypeListLink *&Tail(
    int bucket
) {
    return *g_CZTypeList_TailSlotPtrs[bucket];
}

/**
 * Original inline helper; no standalone retail function exists. Observed in
 * 0x44e920 CZClass::ProcessDeferredWork as the repeated pending-removal dirty
 * check for each bucket; evidence basis is the BN-visible bucket processing
 * order and the matching recovered g_CZTypeList_Buckets field reads.
 * Purpose: expose the mapped pending-removal dirty flag for the requested
 * type-list bucket.
 */
inline int &PendingRemovalDirty(
    int bucket
) {
    switch (bucket) {
    case 0:
        return g_CZTypeList_Buckets[1].pendingRemovalDirty;
    case 1:
        return g_CZTypeList_Buckets[2].pendingRemovalDirty;
    case 2:
        return g_CZTypeList_Buckets[3].pendingRemovalDirty;
    case 3:
        return g_CZTypeList_Buckets[4].pendingRemovalDirty;
    case 4:
        return g_CZTypeList_Buckets[5].pendingRemovalDirty;
    case 5:
        return g_CZTypeList_Buckets[6].pendingRemovalDirty;
    case 6:
        return g_CZTypeList_Buckets[0].pendingRemovalDirty;
    case 7:
        return g_CZTypeList_Buckets[7].pendingRemovalDirty;
    case 8:
        return g_CZTypeList_Buckets[8].pendingRemovalDirty;
    case 9:
        return g_CZTypeList_Buckets[9].pendingRemovalDirty;
    case 10:
        return g_CZTypeList_Buckets[10].pendingRemovalDirty;
    case 11:
        return g_CZTypeList_Buckets[14].pendingRemovalDirty;
    case 12:
        return g_CZTypeList_Buckets[15].pendingRemovalDirty;
    case 13:
        return g_CZTypeList_Buckets[11].pendingRemovalDirty;
    case 14:
        return g_CZTypeList_Buckets[12].pendingRemovalDirty;
    case 15:
        return g_CZTypeList_Buckets[13].pendingRemovalDirty;
    default:
        return Bucket(bucket).pendingRemovalDirty;
    }
}

/**
 * Original inline helper; no standalone retail function exists. Observed in
 * 0x44eed0 CZTypeList::MarkPendingRemoval and 0x44e700
 * CZTypeList::ProcessPendingRemovals as the repeated dirty-flag write
 * after marking or draining deferred removals.
 * Purpose: store the mapped pending-removal dirty flag for the requested
 * type-list bucket without altering the recovered bucket order.
 */
inline void SetPendingRemovalDirty(
    int bucket,
    int value
) {
    switch (bucket) {
    case 0:
        g_CZTypeList_Buckets[1].pendingRemovalDirty = value;
        break;
    case 1:
        g_CZTypeList_Buckets[2].pendingRemovalDirty = value;
        break;
    case 2:
        g_CZTypeList_Buckets[3].pendingRemovalDirty = value;
        break;
    case 3:
        g_CZTypeList_Buckets[4].pendingRemovalDirty = value;
        break;
    case 4:
        g_CZTypeList_Buckets[5].pendingRemovalDirty = value;
        break;
    case 5:
        g_CZTypeList_Buckets[6].pendingRemovalDirty = value;
        break;
    case 6:
        g_CZTypeList_Buckets[0].pendingRemovalDirty = value;
        break;
    case 7:
        g_CZTypeList_Buckets[7].pendingRemovalDirty = value;
        break;
    case 8:
        g_CZTypeList_Buckets[8].pendingRemovalDirty = value;
        break;
    case 9:
        g_CZTypeList_Buckets[9].pendingRemovalDirty = value;
        break;
    case 10:
        g_CZTypeList_Buckets[10].pendingRemovalDirty = value;
        break;
    case 11:
        g_CZTypeList_Buckets[14].pendingRemovalDirty = value;
        break;
    case 12:
        g_CZTypeList_Buckets[15].pendingRemovalDirty = value;
        break;
    case 13:
        g_CZTypeList_Buckets[11].pendingRemovalDirty = value;
        break;
    case 14:
        g_CZTypeList_Buckets[12].pendingRemovalDirty = value;
        break;
    case 15:
        g_CZTypeList_Buckets[13].pendingRemovalDirty = value;
        break;
    default:
        break;
    }
}
} // namespace CZTypeList

#endif
