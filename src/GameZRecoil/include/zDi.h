#pragma once
#ifndef RECOIL_GAMEZRECOIL_INCLUDE_ZDI_H
#define RECOIL_GAMEZRECOIL_INCLUDE_ZDI_H

#include <stddef.h>
#include "recoil/recoil_types.h"

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
RECOIL_NOINLINE int RECOIL_FASTCALL
AddPolygonEx(zDiPartial *self, int vertexCount, zVec3 *points, zVec3 *entryNormals,
             zClipUV *uvPairsA, zVec3 *normalsA, zVec3 *normalsB, zClipUV *uvPairsB,
             zModel_MaterialPartial *material, unsigned int drawFlags, int flagBit8,
             const int *userTag);
RECOIL_NOINLINE int RECOIL_FASTCALL
AddPolygon(zDiPartial *self, int pointCount, zVec3 *points, zClipUV *uvPairsA,
           zVec3 *normalsA, zVec3 *normalsB, zClipUV *uvPairsB, zModel_MaterialPartial *material,
           unsigned int drawFlags, int flagBit8, const int *userTag);
RECOIL_NOINLINE void RECOIL_FASTCALL
AddPolygonSplitByVertexLimit(zDiPartial *self, int totalVertexCount, zVec3 *points,
                             zVec3 *entryNormals, zClipUV *uvPairsA, zVec3 *normalsA,
                             zVec3 *normalsBInput, zClipUV *uvPairsBInput,
                             zModel_MaterialPartial *material, unsigned int drawFlags,
                             int flagBit8, const int *userTag, int maxChunkVertexCount);
RECOIL_NOINLINE void RECOIL_FASTCALL SetFlagBit0(zDiPartial *self, int enabled);
RECOIL_NOINLINE void RECOIL_FASTCALL SetClonedFlag(zDiPartial *self, int isCloned);
RECOIL_NOINLINE zDiPartial *RECOIL_FASTCALL CloneToInstance(zDiPartial *self,
                                                            int cloneMaterials,
                                                            int cloneAuxOnly);
RECOIL_NOINLINE int RECOIL_FASTCALL HasSpecialFlagsOrAuxMaterialData(zDiPartial *self);
RECOIL_NOINLINE void RECOIL_FASTCALL SetVariantTagIfUnset(zDiPartial *self,
                                                          int variantTag);
RECOIL_NOINLINE void RECOIL_FASTCALL BuildAabb(zDiPartial *self,
                                               zBoundsMinMaxPartial *outBoundsMinMax);
RECOIL_NOINLINE void RECOIL_FASTCALL
BuildOriginSymmetricAabb(zDiPartial *self, zBoundsMinMaxPartial *outBoundsMinMax);
RECOIL_NOINLINE void RECOIL_FASTCALL RebuildBounds(zDiPartial *self,
                                                   zBoundsMinMaxPartial *outBoundsMinMax);
RECOIL_NOINLINE int RECOIL_FASTCALL FreeContents(zDiPartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL AddRef(zDiPartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL Release(zDiPartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL GetRefCount(zDiPartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL PtrToIndexOrMinus1(zDiPartial *self);
RECOIL_NOINLINE zDiPartial *RECOIL_FASTCALL IndexToPtrOrNull(int index);
RECOIL_NOINLINE void RECOIL_FASTCALL ResetCurrentVariant(zDiPartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL SetCurrentVariantCycleTextureCount(zDiPartial *self,
                                                                       int textureCount);
RECOIL_NOINLINE void RECOIL_FASTCALL SetCurrentVariant(zDiPartial *self, int variantIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL SetCurrentVariantCycleTextureSpeed(zDiPartial *self,
                                                                       float cycleSpeed);
RECOIL_NOINLINE void RECOIL_FASTCALL RebuildGeneratedUvPairsForEntry(zDiPartial *self,
                                                                     int entryIndex);
RECOIL_NOINLINE void RECOIL_FASTCALL
BuildBlendVertsFromConnectivity(zDiPartial *self, int *excludedVertexIndices, float blendY,
                                int excludedVertexCount, int minSharedVertexCount);
RECOIL_NOINLINE void RECOIL_FASTCALL SetEntryValueForAllEntries(zDiPartial *self,
                                                                unsigned int entryValue);
RECOIL_NOINLINE void RECOIL_FASTCALL SetShowBackFaceForAllEntries(zDiPartial *self,
                                                                  int enabled);
RECOIL_NOINLINE void RECOIL_FASTCALL SetObject3DColorModeForMaterials(zDiPartial *self,
                                                                      int colorMode);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateForQueryPoint(
    zDiPartial *self, zClassDiPickCandidateEntry *outCandidate, const zVec3 *queryPoint);
} // namespace zDi

namespace zModel_Instance {
RECOIL_NOINLINE int RECOIL_FASTCALL SetCycleTextureLoop(zDiPartial *instance, int loopEnabled);
RECOIL_NOINLINE int RECOIL_FASTCALL
AddCycleTexture(zDiPartial *instance, zImage_TexDirEntryPartial *textureDirectoryEntry);
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
void RECOIL_FASTCALL SetBreakOnFirstCandidate(int enabled);
void RECOIL_FASTCALL SetStopAfterFirstHit(int flag);
RECOIL_NOINLINE void RECOIL_FASTCALL FindBestPickCandidateBelowPoint(
    zClass_NodePartial *world, const zVec3 *position, PlayerProbeSampleCandidateBuffer *outResults);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateListBelowPoint(
    zClass_NodePartial *world, PlayerProbeSampleCandidateBuffer *outResults, float x, float maxY,
    float z);
RECOIL_NOINLINE int RECOIL_FASTCALL SnapProbePointYToBestCandidate(zVec3 *point);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateList(zClass_NodePartial *node,
                                                                    int cullCount);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForPoints(zClass_NodePartial *node,
                                                                          int depth,
                                                                          int *hitFlags);
RECOIL_NOINLINE int RECOIL_FASTCALL
BuildPickCandidatesForPointsRecursive(zClass_NodePartial *node, int depth, int *hitFlags);
RECOIL_NOINLINE int RECOIL_FASTCALL
BuildPickCandidatesForPointsForLight(zClass_NodePartial *node, int depth, int *hitFlags);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForPointBatch(
    zClass_NodePartial *world, zVec3 *pointArray, int pointCount, float queryMaxY,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersByPoint);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesRecursive(zClass_NodePartial *node,
                                                                          int cullCount);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForLight(zClass_NodePartial *node,
                                                                         int cullCount);
RECOIL_NOINLINE int RECOIL_FASTCALL
IsPickQueryPointOutsideViewBBoxXZ(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL PickTestBBox2D(zClass_NodePartial *node,
                                                            int *hitFlags);
RECOIL_NOINLINE int RECOIL_FASTCALL FrustumTestAndPick(zClass_NodePartial *node,
                                                       int *activeMask);
RECOIL_NOINLINE int RECOIL_FASTCALL
TryGetPolygonHitAtQueryXZ(zClassDiPickCandidateEntry *candidate, const zVec3 *polygonVertices,
                          float queryX, float queryZ, int vertexCount);
RECOIL_NOINLINE void RECOIL_FASTCALL PickTestMeshAtQueryXZ(
    zClass_NodePartial *node, zModel_PickFaceData *faceData, const zVec3 *samplePoints,
    const int *sampleMaskSeeds, int samplePointCount, float maxProjectedY,
    PlayerProbeSampleCandidateBuffer *outputBuckets);
RECOIL_NOINLINE int RECOIL_FASTCALL
BuildPickCandidatesForSegment(zClass_NodePartial *self);
RECOIL_NOINLINE int RECOIL_FASTCALL RaycastSelectClosestHitBetweenPoints(
    zClass_NodePartial *world, const zVec3 *startPoint, const zVec3 *endPoint,
    PlayerProbeSampleCandidateBuffer *rayData);
RECOIL_NOINLINE int RECOIL_FASTCALL
RaycastFindClosest(zClass_NodePartial *world, PlayerProbeSampleCandidateBuffer *rayData,
                   float startX, float startY, float startZ, float endX, float endY, float endZ);
RECOIL_NOINLINE int RECOIL_FASTCALL
BuildPickCandidatesForSegmentChildFallback(zClass_NodePartial *node, int nodeCountHint);
RECOIL_NOINLINE int RECOIL_FASTCALL
BuildPickCandidatesForSegmentRecursive(zClass_NodePartial *node, int depth);
RECOIL_NOINLINE int RECOIL_FASTCALL
BuildPickCandidatesForSegmentForCamera(zClass_NodePartial *node, int depth);
RECOIL_NOINLINE int RECOIL_FASTCALL
BuildPickCandidatesForSegmentForLight(zClass_NodePartial *node, int depth);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentsRecursive(
    zClass_NodePartial *node, int nodeCountHint, int *activeMask);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentsForAnimate(
    zClass_NodePartial *node, int nodeCountHint, int *activeMask);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentsForLight(
    zClass_NodePartial *node, int nodeCountHint, int *activeMask);
RECOIL_NOINLINE void RECOIL_FASTCALL BuildProbeHitBatchesForSegments(
    zClass_NodePartial *world, zClass_DiSegmentEndpoints *segmentEndpoints, int endpointCount,
    PlayerProbeSampleCandidateBuffer *hitBatches);
RECOIL_NOINLINE void RECOIL_FASTCALL BuildPickCandidatesForSegmentsInGridWindow(
    zClass_NodePartial *world, int *activeMask);
RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegionsAgainstMeshFaces(
    zVec3 *meshVertices, int faceCount);
RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegionsAgainstHexahedronFaces(
    zVec3 *center, float radius);
RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegionsAgainstSphere(
    zClass_NodePartial *world, zVec3 *center, const char *nodeNamePrefix, float radius,
    int enableDistanceCull, int requireLineOfSight, OptCatalogRaycastHitList *outHitList);
RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegions_TryAppendNode(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL FilterPointsBBox(zClass_NodePartial *node,
                                                              void *pointData);
RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegionsAgainstPolygonWithDamageMaskUv(
    zClass_NodePartial *candidateOwner,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment,
    zClass_DiSegmentEndpoints *segmentEndpointsByBatch, int *activeMask, int segmentCount,
    const zBBoxCorners *bboxCorners);
RECOIL_NOINLINE void RECOIL_FASTCALL FilterRegionsAgainstPolygon(
    zClass_NodePartial *candidateOwner, zModel_PickFaceData *faceData,
    zClass_DiSegmentEndpoints *segmentEndpointsByBatch, int *activeMask, int segmentCount,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentVsBBoxFaces(
    const zBBoxCorners *bboxCorners, zClassDiPickCandidateEntry *candidate,
    const zVec3 *segmentStart, const zVec3 *segmentEnd);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentBatchVsPolygon(
    zClass_NodePartial *candidateOwner,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment,
    zClass_DiSegmentEndpoints *segmentEndpointsByBatch, int *activeMask, int segmentCount,
    zVec3 *polygonVertices, zModel_PickFaceEntry *faceEntry);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
    zClass_NodePartial *candidateOwner,
    PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment,
    zClass_DiSegmentEndpoints *segmentEndpointsByBatch, int *activeMask, int segmentCount,
    zVec3 *polygonVertices, zModel_PickFaceUvData *faceUvData, zVec2 *scratchUv,
    zModel_PickFaceEntry *faceEntry);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateForSegmentVsPolygon(
    zClassDiPickCandidateEntry *candidate, const zVec3 *segmentStart, const zVec3 *segmentEnd,
    const zVec3 *polygonVertices, int vertexCount, int cullBackface);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateForSegmentVsPolygonWithUv(
    zClassDiPickCandidateEntry *candidate, const zVec3 *segmentStart, const zVec3 *segmentEnd,
    const zVec3 *polygonVertices, const zModel_PickFaceUvData *faceUvData, zVec2 *outUv,
    int vertexCount, int cullBackface);
RECOIL_NOINLINE int RECOIL_FASTCALL AppendPickCandidatesForFace(
    const zModel_PickFaceData *faceData, zClassDiPickCandidateEntry *candidate,
    const zVec3 *segmentStart, const zVec3 *segmentEnd);
} // namespace zClass_cls_di

namespace zModelConst {
RECOIL_NOINLINE void RECOIL_FASTCALL AddFaceToPlayerProbeSampleBuckets(
    zClass_NodePartial *node, PlayerProbeSampleCandidateBuffer *outputBuckets,
    const zVec3 *samplePoints, const int *sampleMaskSeeds, int samplePointCount,
    float maxProjectedY, const zVec3 *polygonVertices, const zModel_PickFaceEntry *faceEntry);
} // namespace zModelConst

RECOIL_NOINLINE int RECOIL_FASTCALL
zModel_Material_SetFlagBit9(zModel_MaterialPartial *material, int enabled);
namespace zModel_Material {
RECOIL_NOINLINE void RECOIL_FASTCALL ResetDefaults(zModel_MaterialPartial *material);
RECOIL_NOINLINE int RECOIL_FASTCALL HasAuxData(zModel_MaterialPartial *material);
RECOIL_NOINLINE int RECOIL_FASTCALL CompareForReuse(zModel_MaterialPartial *lhs,
                                                             zModel_MaterialPartial *rhs);
RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_FASTCALL
FindByTexDirEntry(zImage_TexDirEntryPartial *texDirEntry);
RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_FASTCALL
FindOrClone(zModel_MaterialPartial *material);
RECOIL_NOINLINE int RECOIL_FASTCALL SetUserTag(zModel_MaterialPartial *material,
                                                        int userTag);
RECOIL_NOINLINE int RECOIL_FASTCALL SetCycleTextureCount(zModel_MaterialPartial *material,
                                                                  int textureCount);
RECOIL_NOINLINE int RECOIL_FASTCALL
AddCycleTexture(zModel_MaterialPartial *material, zImage_TexDirEntryPartial *textureDirectoryEntry);
RECOIL_NOINLINE int RECOIL_FASTCALL SetCycleTextureLoop(zModel_MaterialPartial *material,
                                                                 int loopEnabled);
RECOIL_NOINLINE int RECOIL_FASTCALL SetCycleTextureSpeed(zModel_MaterialPartial *material,
                                                                  float cycleSpeed);
RECOIL_NOINLINE void RECOIL_FASTCALL UpdateCycleIfNeeded(zModel_MaterialPartial *material);
RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_FASTCALL Clone(zModel_MaterialPartial *material);
RECOIL_NOINLINE void RECOIL_FASTCALL InvalidateImagesIfEligible(zModel_MaterialPartial *material);
} // namespace zModel_Material
RECOIL_NOINLINE void RECOIL_FASTCALL
zDi_SetMaterialFlagBit9ForFlagBit0Entries(zDiPartial *self, int enabled);
namespace zDi {
RECOIL_NOINLINE void RECOIL_FASTCALL InvalidateImagesForFlagBit8Materials(zDiPartial *self);
}

RECOIL_STATIC_ASSERT(offsetof(zDiPartial, flags) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, entryCount) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, vertCount) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, blendVertCount) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, pointCount) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, entries) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, verts) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, pointEntries) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, blendVerts) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, bboxCenter) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, bboxRadius) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zDiPartial, nextFreeIndex) == 0x54);
RECOIL_STATIC_ASSERT(sizeof(zDiPartial) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(zBoundsMinMaxPartial) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zModel_PointEntryPartial, pointCamCount) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zModel_PointEntryPartial, pointCamList) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(zModel_PointEntryPartial) == 0x4c);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialPartial, packedColor) == 0x02);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialPartial, colorRgb) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialPartial, currentTextureDirectoryEntry) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialPartial, unknown_14) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialPartial, colorScalar) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialPartial, unknown_1c) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialPartial, userTag) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialPartial, cycle) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zModel_MaterialPartial) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, loopEnabled) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, lastUpdateFrameTick) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, currentFrame) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, framesPerSecond) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, frameCount) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, frameWriteCount) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, frameTable) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zModel_MaterialCyclePartial) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, flagsAndIndexCount) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, drawFlags) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, vertexIndices) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, normalIndices) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, uvPairs) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, material) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, variantTagInitialized) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, variantTag) == 0x19);
RECOIL_STATIC_ASSERT(sizeof(zDiEntryPartial) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zClassDiPickCandidateEntry, surfaceNormal) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClassDiPickCandidateEntry, hitPos) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClassDiPickCandidateEntry, variantTag) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zClassDiPickCandidateEntry, scenePayload) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zClassDiPickCandidateEntry, node) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zClassDiPickCandidateEntry) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(PlayerProbeSampleCandidateBuffer, entries) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(PlayerProbeSampleCandidateBuffer) == 0x504);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRaycastHitEntry, pos) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRaycastHitEntry, distance) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRaycastHitEntry, surfaceRef) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRaycastHitEntry, hitNode) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(OptCatalogRaycastHitEntry) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(OptCatalogRaycastHitList, hits) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClass_DiSegmentEndpoints, end) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zClass_DiSegmentEndpoints) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zClass_DiSegmentBounds, maxX) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zClass_DiSegmentBounds) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zModel_PickFaceUvData) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceScenePayload, flags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceEntry, flagsAndVertexCount) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceEntry, vertexIndices) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceEntry, faceUvData) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceEntry, scenePayload) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceEntry, variantTag) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zModel_PickFaceEntry) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, flags) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, faceCount) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, vertexCount) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, morphVertexCount) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, morphWeight) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, faces) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, baseVertices) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, morphVertices) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(zModel_PickFaceData) == 0x44);

#endif // RECOIL_GAMEZRECOIL_INCLUDE_ZDI_H
