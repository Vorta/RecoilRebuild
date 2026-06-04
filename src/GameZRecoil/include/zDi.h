#pragma once
#ifndef RECOIL_GAMEZRECOIL_INCLUDE_ZDI_H
#define RECOIL_GAMEZRECOIL_INCLUDE_ZDI_H

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/zMath/zMath.h"
#include "recoil/recoil_callconv.h"
#include "zClass.h"
#include "zImage.h"

struct OptCatalogSurfaceMaterialRef;

struct zBoundsMinMaxPartial {
    zVec3 min;
    zVec3 max;
};

struct zClipUV;

struct zModel_PointEntryPartial {
    int mode;
    int behavior;
    float timerSec;
    int pointCamCount;
    float elapsedTime;
    int pointCamPackedState;
    int depthBiasWord;
    float colorB;
    float colorG;
    float colorR;
    int packedColor16;
    zVec3 *pointCamList;
    unsigned char lensFlareSource[0x1c];
};

struct zDiPartial {
    int mode;
    int flags;
    int refCount;
    int entryCount;
    int vertCount;
    int normalCount;
    int blendVertCount;
    int pointCount;
    float blendScale;
    float textureWorldPerMeter;
    int textureWorldAxis;
    int field2c;
    struct zDiEntryPartial *entries;
    zVec3 *verts;
    zVec3 *normals;
    zModel_PointEntryPartial *pointEntries;
    zVec3 *blendVerts;
    zVec3 bboxCenter;
    float bboxRadius;
    int nextFreeIndex;
};

struct zModel_MaterialPartial {
    unsigned short flags;
    unsigned short packedColor;
    zColorRgb colorRgb;
    zImage_TexDirEntryPartial *currentTextureDirectoryEntry;
    float unknown_14;
    float colorScalar;
    float unknown_1c;
    int userTag;
    struct zModel_MaterialCyclePartial *cycle;
};

struct zModel_MaterialCyclePartial {
    int loopEnabled;
    int lastUpdateFrameTick;
    float currentFrame;
    float framesPerSecond;
    int frameCount;
    int frameWriteCount;
    zImage_TexDirEntryPartial **frameTable;
};

struct zDiEntryPartial {
    unsigned int flagsAndIndexCount;
    unsigned int drawFlags;
    void *vertexIndices;
    void *normalIndices;
    void *uvPairs;
    zModel_MaterialPartial *material;
    unsigned char variantTagInitialized;
    unsigned char variantTag;
    unsigned char unknown_1a[0x02];
};

struct zClassDiPickCandidateEntry {
    zVec3 surfaceNormal;
    zVec3 hitPos;
    zTag4Partial variantTag;
    unsigned char unknown_1c[0x04];
    void *scenePayload;
    zClass_NodePartial *node;
};

struct PlayerProbeSampleCandidateBuffer {
    int candidateCount;
    zClassDiPickCandidateEntry entries[0x20];
};

struct OptCatalogRaycastHitEntry {
    unsigned char unknown_00[0x0c];
    zVec3 pos;
    float unknown_18;
    float distance;
    OptCatalogSurfaceMaterialRef *surfaceRef;
    zClass_NodePartial *hitNode;
};

struct OptCatalogRaycastHitList {
    int hitCount;
    OptCatalogRaycastHitEntry hits[0x20];
};

struct zClass_DiSegmentEndpoints {
    zVec3 start;
    zVec3 end;
};

struct zClass_DiSegmentBounds {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

struct zModel_PickFaceUvData {
    zVec2 uvs[3];
};

struct zModel_PickFaceScenePayload {
    unsigned short flags;
};

struct zModel_PickFaceEntry {
    unsigned int flagsAndVertexCount;
    unsigned int unknown_04;
    int *vertexIndices;
    unsigned int unknown_0c;
    zModel_PickFaceUvData *faceUvData;
    zModel_PickFaceScenePayload *scenePayload;
    zTag4Partial variantTag;
};

struct zModel_PickFaceData {
    unsigned int unknown_00;
    unsigned int flags;
    unsigned int unknown_08;
    int faceCount;
    int vertexCount;
    unsigned int unknown_14;
    int morphVertexCount;
    unsigned char unknown_1c[0x04];
    float morphWeight;
    unsigned char unknown_24[0x0c];
    zModel_PickFaceEntry *faces;
    zVec3 *baseVertices;
    unsigned char unknown_38[0x08];
    zVec3 *morphVertices;
};

namespace zDi {
int __fastcall AddPolygonEx(
    zDiPartial *self,
    int vertexCount,
    zVec3 *points,
    zVec3 *entryNormals,
    zClipUV *uvPairsA,
    zVec3 *normalsA,
    zVec3 *normalsB,
    zClipUV *uvPairsB,
    zModel_MaterialPartial *material,
    unsigned int drawFlags,
    int flagBit8,
    const int *userTag
);
int __fastcall AddPolygon(
    zDiPartial *self,
    int pointCount,
    zVec3 *points,
    zClipUV *uvPairsA,
    zVec3 *normalsA,
    zVec3 *normalsB,
    zClipUV *uvPairsB,
    zModel_MaterialPartial *material,
    unsigned int drawFlags,
    int flagBit8,
    const int *userTag
);
void __fastcall AddPolygonSplitByVertexLimit(
    zDiPartial *self,
    int totalVertexCount,
    zVec3 *points,
    zVec3 *entryNormals,
    zClipUV *uvPairsA,
    zVec3 *normalsA,
    zVec3 *normalsBInput,
    zClipUV *uvPairsBInput,
    zModel_MaterialPartial *material,
    unsigned int drawFlags,
    int flagBit8,
    const int *userTag,
    int maxChunkVertexCount
);
void __fastcall SetFlagBit0(
    zDiPartial *self,
    int enabled
);
void __fastcall SetClonedFlag(
    zDiPartial *self,
    int isCloned
);
zDiPartial *__fastcall CloneToInstance(
    zDiPartial *self,
    int cloneMaterials,
    int cloneAuxOnly
);
int __fastcall HasSpecialFlagsOrAuxMaterialData(zDiPartial *self);
void __fastcall SetVariantTagIfUnset(
    zDiPartial *self,
    int variantTag
);
void __fastcall BuildAabb(
    zDiPartial *self,
    zBoundsMinMaxPartial *outBoundsMinMax
);
void __fastcall BuildOriginSymmetricAabb(
    zDiPartial *self,
    zBoundsMinMaxPartial *outBoundsMinMax
);
void __fastcall RebuildBounds(
    zDiPartial *self,
    zBoundsMinMaxPartial *outBoundsMinMax
);
int __fastcall FreeContents(zDiPartial *self);
int __fastcall AddRef(zDiPartial *self);
int __fastcall Release(zDiPartial *self);
int __fastcall GetRefCount(zDiPartial *self);
int __fastcall PtrToIndexOrMinus1(zDiPartial *self);
zDiPartial *__fastcall IndexToPtrOrNull(int index);
void __fastcall ResetCurrentVariant(zDiPartial *self);
int __fastcall SetCurrentVariantCycleTextureCount(
    zDiPartial *self,
    int textureCount
);
void __fastcall SetCurrentVariant(
    zDiPartial *self,
    int variantIndex
);
int __fastcall SetCurrentVariantCycleTextureSpeed(
    zDiPartial *self,
    float cycleSpeed
);
void __fastcall RebuildGeneratedUvPairsForEntry(
    zDiPartial *self,
    int entryIndex
);
void __fastcall BuildBlendVertsFromConnectivity(
    zDiPartial *self,
    int *excludedVertexIndices,
    float blendY,
    int excludedVertexCount,
    int minSharedVertexCount
);
void __fastcall SetEntryValueForAllEntries(
    zDiPartial *self,
    unsigned int entryValue
);
void __fastcall SetShowBackFaceForAllEntries(
    zDiPartial *self,
    int enabled
);
void __fastcall SetObject3DColorModeForMaterials(
    zDiPartial *self,
    int colorMode
);
int __fastcall BuildPickCandidateForQueryPoint(
    zDiPartial *self,
    zClassDiPickCandidateEntry *outCandidate,
    const zVec3 *queryPoint
);
} // namespace zDi

namespace zModel_Instance {
int __fastcall SetCycleTextureLoop(
    zDiPartial *instance,
    int loopEnabled
);
int __fastcall AddCycleTexture(
    zDiPartial *instance,
    zImage_TexDirEntryPartial *textureDirectoryEntry
);
} // namespace zModel_Instance

extern int g_cls_di_StopAfterFirstHit;
extern int g_cls_di_BreakOnFirstCandidate;
extern zVec3 g_zClass_DiFaceVertexScratch4[4];
extern zVec3 g_DiPickQueryPoint;
extern zVec3 g_DiSegmentEnd;
extern float g_DiSegmentMinX;
extern float g_DiSegmentMinY;
extern float g_DiSegmentMinZ;
extern float g_DiSegmentMaxX;
extern float g_DiSegmentMaxY;
extern float g_DiSegmentMaxZ;
extern zClass_DiSegmentBounds g_DiSegmentBounds[24];
extern zVec3 *g_DiPickPointArray;
extern int g_DiPickPointCount;
extern float g_DiPickPointQueryMaxY;
extern PlayerProbeSampleCandidateBuffer *g_DiPickCandidateBuffer;
extern zClassDiPickCandidateEntry *g_DiPickCandidateCursor;
extern const char *g_zClass_cls_di_FilterRegions_NodeNamePrefix;
extern zVec3 *g_zClass_cls_di_FilterRegions_Center;
extern float g_zClass_cls_di_FilterRegions_RadiusSq;
extern int g_zClass_cls_di_FilterRegions_EnableClearanceCheck;
extern zClass_NodePartial *g_zClass_cls_di_FilterRegions_LineOfSightWorld;
extern OptCatalogRaycastHitList *g_zClass_cls_di_FilterRegions_OutHitList;

namespace zClass_cls_di {
void __fastcall SetBreakOnFirstCandidate(int enabled);
void __fastcall SetStopAfterFirstHit(int flag);
void __fastcall FindBestPickCandidateBelowPoint(
    zClass_NodePartial *world,
    const zVec3 *position,
    PlayerProbeSampleCandidateBuffer *outResults
);
int __fastcall BuildPickCandidateListBelowPoint(
    zClass_NodePartial *world,
    PlayerProbeSampleCandidateBuffer *outResults,
    float x,
    float maxY,
    float z
);
int __fastcall SnapProbePointYToBestCandidate(zVec3 *point);
int __fastcall BuildPickCandidateList(
    zClass_NodePartial *node,
    int cullCount
);
int __fastcall BuildPickCandidatesForPoints(
    zClass_NodePartial *node,
    int depth,
    int *hitFlags
);
int __fastcall BuildPickCandidatesForPointsRecursive(
    zClass_NodePartial *node,
    int depth,
    int *hitFlags
);
int __fastcall BuildPickCandidatesForPointsForLight(
    zClass_NodePartial *node,
    int depth,
    int *hitFlags
);
int __fastcall BuildPickCandidatesForPointBatch(
    zClass_NodePartial *world,
    zVec3 *pointArray,
    int pointCount,
    float queryMaxY,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersByPoint
);
int __fastcall BuildPickCandidatesRecursive(
    zClass_NodePartial *node,
    int cullCount
);
int __fastcall BuildPickCandidatesForLight(
    zClass_NodePartial *node,
    int cullCount
);
int __fastcall IsPickQueryPointOutsideViewBBoxXZ(zClass_NodePartial *node);
int __fastcall PickTestBBox2D(
    zClass_NodePartial *node,
    int *hitFlags
);
int __fastcall FrustumTestAndPick(
    zClass_NodePartial *node,
    int *activeMask
);
int __fastcall TryGetPolygonHitAtQueryXZ(
    zClassDiPickCandidateEntry *candidate,
    const zVec3 *polygonVertices,
    float queryX,
    float queryZ,
    int vertexCount
);
void __fastcall PickTestMeshAtQueryXZ(
    zClass_NodePartial *node,
    zModel_PickFaceData *faceData,
    const zVec3 *samplePoints,
    const int *sampleMaskSeeds,
    int samplePointCount,
    float maxProjectedY,
    PlayerProbeSampleCandidateBuffer *outputBuckets
);
int __fastcall BuildPickCandidatesForSegment(zClass_NodePartial *self);
int __fastcall RaycastSelectClosestHitBetweenPoints(
    zClass_NodePartial *world,
    const zVec3 *startPoint,
    const zVec3 *endPoint,
    PlayerProbeSampleCandidateBuffer *rayData
);
int __fastcall RaycastFindClosest(
    zClass_NodePartial *world,
    PlayerProbeSampleCandidateBuffer *rayData,
    float startX,
    float startY,
    float startZ,
    float endX,
    float endY,
    float endZ
);
int __fastcall BuildPickCandidatesForSegmentChildFallback(
    zClass_NodePartial *node,
    int nodeCountHint
);
int __fastcall BuildPickCandidatesForSegmentRecursive(
    zClass_NodePartial *node,
    int depth
);
int __fastcall BuildPickCandidatesForSegmentForCamera(
    zClass_NodePartial *node,
    int depth
);
int __fastcall BuildPickCandidatesForSegmentForLight(
    zClass_NodePartial *node,
    int depth
);
int __fastcall BuildPickCandidatesForSegmentsRecursive(
    zClass_NodePartial *node,
    int nodeCountHint,
    int *activeMask
);
int __fastcall BuildPickCandidatesForSegmentsForAnimate(
    zClass_NodePartial *node,
    int nodeCountHint,
    int *activeMask
);
int __fastcall BuildPickCandidatesForSegmentsForLight(
    zClass_NodePartial *node,
    int nodeCountHint,
    int *activeMask
);
void __fastcall BuildProbeHitBatchesForSegments(
    zClass_NodePartial *world,
    zClass_DiSegmentEndpoints *segmentEndpoints,
    int endpointCount,
    PlayerProbeSampleCandidateBuffer *hitBatches
);
void __fastcall BuildPickCandidatesForSegmentsInGridWindow(
    zClass_NodePartial *world,
    int *activeMask
);
int __fastcall FilterRegionsAgainstMeshFaces(
    zVec3 *meshVertices,
    int faceCount
);
int __fastcall FilterRegionsAgainstHexahedronFaces(
    zVec3 *center,
    float radius
);
int __fastcall FilterRegionsAgainstSphere(
    zClass_NodePartial *world,
    zVec3 *center,
    const char *nodeNamePrefix,
    float radius,
    int enableDistanceCull,
    int requireLineOfSight,
    OptCatalogRaycastHitList *outHitList
);
int __fastcall FilterRegions_TryAppendNode(zClass_NodePartial *node);
int __fastcall FilterPointsBBox(
    zClass_NodePartial *node,
    void *pointData
);
int __fastcall FilterRegionsAgainstPolygonWithDamageMaskUv(
    zClass_NodePartial *candidateOwner,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment,
    zClass_DiSegmentEndpoints *segmentEndpointsByBatch,
    int *activeMask,
    int segmentCount,
    const zBBoxCorners *bboxCorners
);
void __fastcall FilterRegionsAgainstPolygon(
    zClass_NodePartial *candidateOwner,
    zModel_PickFaceData *faceData,
    zClass_DiSegmentEndpoints *segmentEndpointsByBatch,
    int *activeMask,
    int segmentCount,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment
);
int __fastcall BuildPickCandidatesForSegmentVsBBoxFaces(
    const zBBoxCorners *bboxCorners,
    zClassDiPickCandidateEntry *candidate,
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd
);
int __fastcall BuildPickCandidatesForSegmentBatchVsPolygon(
    zClass_NodePartial *candidateOwner,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment,
    zClass_DiSegmentEndpoints *segmentEndpointsByBatch,
    int *activeMask,
    int segmentCount,
    zVec3 *polygonVertices,
    zModel_PickFaceEntry *faceEntry
);
int __fastcall BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
    zClass_NodePartial *candidateOwner,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment,
    zClass_DiSegmentEndpoints *segmentEndpointsByBatch,
    int *activeMask,
    int segmentCount,
    zVec3 *polygonVertices,
    zModel_PickFaceUvData *faceUvData,
    zVec2 *scratchUv,
    zModel_PickFaceEntry *faceEntry
);
int __fastcall BuildPickCandidateForSegmentVsPolygon(
    zClassDiPickCandidateEntry *candidate,
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd,
    const zVec3 *polygonVertices,
    int vertexCount,
    int cullBackface
);
int __fastcall BuildPickCandidateForSegmentVsPolygonWithUv(
    zClassDiPickCandidateEntry *candidate,
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd,
    const zVec3 *polygonVertices,
    const zModel_PickFaceUvData *faceUvData,
    zVec2 *outUv,
    int vertexCount,
    int cullBackface
);
int __fastcall AppendPickCandidatesForFace(
    const zModel_PickFaceData *faceData,
    zClassDiPickCandidateEntry *candidate,
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd
);
} // namespace zClass_cls_di

namespace zModelConst {
void __fastcall AddFaceToPlayerProbeSampleBuckets(
    zClass_NodePartial *node,
    PlayerProbeSampleCandidateBuffer *outputBuckets,
    const zVec3 *samplePoints,
    const int *sampleMaskSeeds,
    int samplePointCount,
    float maxProjectedY,
    const zVec3 *polygonVertices,
    const zModel_PickFaceEntry *faceEntry
);
} // namespace zModelConst

int __fastcall zModel_Material_SetFlagBit9(
    zModel_MaterialPartial *material,
    int enabled
);
namespace zModel_Material {
void __fastcall ResetDefaults(zModel_MaterialPartial *material);
int __fastcall HasAuxData(zModel_MaterialPartial *material);
int __fastcall CompareForReuse(
    zModel_MaterialPartial *lhs,
    zModel_MaterialPartial *rhs
);
zModel_MaterialPartial *__fastcall FindByTexDirEntry(
    zImage_TexDirEntryPartial *texDirEntry
);
zModel_MaterialPartial *__fastcall FindOrClone(
    zModel_MaterialPartial *material
);
int __fastcall SetUserTag(
    zModel_MaterialPartial *material,
    int userTag
);
int __fastcall SetCycleTextureCount(
    zModel_MaterialPartial *material,
    int textureCount
);
int __fastcall AddCycleTexture(
    zModel_MaterialPartial *material,
    zImage_TexDirEntryPartial *textureDirectoryEntry
);
int __fastcall SetCycleTextureLoop(
    zModel_MaterialPartial *material,
    int loopEnabled
);
int __fastcall SetCycleTextureSpeed(
    zModel_MaterialPartial *material,
    float cycleSpeed
);
void __fastcall UpdateCycleIfNeeded(zModel_MaterialPartial *material);
zModel_MaterialPartial *__fastcall Clone(zModel_MaterialPartial *material);
void __fastcall InvalidateImagesIfEligible(zModel_MaterialPartial *material);
} // namespace zModel_Material
void __fastcall zDi_SetMaterialFlagBit9ForFlagBit0Entries(
    zDiPartial *self,
    int enabled
);
namespace zDi {
void __fastcall InvalidateImagesForFlagBit8Materials(zDiPartial *self);
}

RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        mode
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        flags
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        refCount
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        entryCount
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        vertCount
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        normalCount
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        blendVertCount
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        pointCount
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        blendScale
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        textureWorldPerMeter
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        textureWorldAxis
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        field2c
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        entries
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        verts
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        normals
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        pointEntries
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        blendVerts
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        bboxCenter
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        bboxRadius
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiPartial,
        nextFreeIndex
    ) == 0x54
);
RECOIL_STATIC_ASSERT(sizeof(zDiPartial) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(zBoundsMinMaxPartial) == 0x18);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PointEntryPartial,
        pointCamCount
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PointEntryPartial,
        pointCamList
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(sizeof(zModel_PointEntryPartial) == 0x4c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialPartial,
        packedColor
    ) == 0x02
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialPartial,
        colorRgb
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialPartial,
        currentTextureDirectoryEntry
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialPartial,
        unknown_14
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialPartial,
        colorScalar
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialPartial,
        unknown_1c
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialPartial,
        userTag
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialPartial,
        cycle
    ) == 0x24
);
RECOIL_STATIC_ASSERT(sizeof(zModel_MaterialPartial) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialCyclePartial,
        loopEnabled
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialCyclePartial,
        lastUpdateFrameTick
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialCyclePartial,
        currentFrame
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialCyclePartial,
        framesPerSecond
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialCyclePartial,
        frameCount
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialCyclePartial,
        frameWriteCount
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialCyclePartial,
        frameTable
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zModel_MaterialCyclePartial) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiEntryPartial,
        flagsAndIndexCount
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiEntryPartial,
        drawFlags
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiEntryPartial,
        vertexIndices
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiEntryPartial,
        normalIndices
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiEntryPartial,
        uvPairs
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiEntryPartial,
        material
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiEntryPartial,
        variantTagInitialized
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDiEntryPartial,
        variantTag
    ) == 0x19
);
RECOIL_STATIC_ASSERT(sizeof(zDiEntryPartial) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClassDiPickCandidateEntry,
        surfaceNormal
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClassDiPickCandidateEntry,
        hitPos
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClassDiPickCandidateEntry,
        variantTag
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClassDiPickCandidateEntry,
        scenePayload
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClassDiPickCandidateEntry,
        node
    ) == 0x24
);
RECOIL_STATIC_ASSERT(sizeof(zClassDiPickCandidateEntry) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        PlayerProbeSampleCandidateBuffer,
        entries
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(PlayerProbeSampleCandidateBuffer) == 0x504);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRaycastHitEntry,
        pos
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRaycastHitEntry,
        distance
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRaycastHitEntry,
        surfaceRef
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRaycastHitEntry,
        hitNode
    ) == 0x24
);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogRaycastHitEntry) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        OptCatalogRaycastHitList,
        hits
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_DiSegmentEndpoints,
        end
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(zClass_DiSegmentEndpoints) == 0x18);
RECOIL_STATIC_ASSERT(
    offsetof(
        zClass_DiSegmentBounds,
        maxX
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(zClass_DiSegmentBounds) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zModel_PickFaceUvData) == 0x18);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceScenePayload,
        flags
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceEntry,
        flagsAndVertexCount
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceEntry,
        vertexIndices
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceEntry,
        faceUvData
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceEntry,
        scenePayload
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceEntry,
        variantTag
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zModel_PickFaceEntry) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceData,
        flags
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceData,
        faceCount
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceData,
        vertexCount
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceData,
        morphVertexCount
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceData,
        morphWeight
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceData,
        faces
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceData,
        baseVertices
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_PickFaceData,
        morphVertices
    ) == 0x40
);
RECOIL_STATIC_ASSERT(sizeof(zModel_PickFaceData) == 0x44);

#endif // RECOIL_GAMEZRECOIL_INCLUDE_ZDI_H
