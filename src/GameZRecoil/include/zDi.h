#pragma once
#ifndef RECOIL_GAMEZRECOIL_INCLUDE_ZDI_H
#define RECOIL_GAMEZRECOIL_INCLUDE_ZDI_H

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/zMath/zMath.h"
#include "recoil/recoil_callconv.h"
#include "zClass.h"
#include "zImage.h"

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
    unsigned char unknown_04[0x04];
    float currentFrame;
    float framesPerSecond;
    int frameCount;
    int frameWriteCount;
    zImage_TexDirEntryPartial **frameTable;
};

struct zDiEntryPartial {
    unsigned int flagsAndIndexCount;
    unsigned int unknown_04;
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
    unsigned int unknown_18;
};

struct zModel_PickFaceData {
    unsigned int unknown_00;
    unsigned int flags;
    unsigned int unknown_08;
    int faceCount;
    unsigned char unknown_10[0x08];
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
RECOIL_NOINLINE void RECOIL_FASTCALL SetCurrentVariant(zDiPartial *self, int variantIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateForQueryPoint(
    zDiPartial *self, zClassDiPickCandidateEntry *outCandidate, const zVec3 *queryPoint);
} // namespace zDi

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
extern PlayerProbeSampleCandidateBuffer *g_DiPickCandidateBuffer;
extern zClassDiPickCandidateEntry *g_DiPickCandidateCursor;

namespace zClass_cls_di {
void RECOIL_FASTCALL SetBreakOnFirstCandidate(int enabled);
void RECOIL_FASTCALL SetStopAfterFirstHit(int flag);
RECOIL_NOINLINE void RECOIL_FASTCALL FindBestPickCandidateBelowPoint(
    zClass_NodePartial *world, const zVec3 *position, PlayerProbeSampleCandidateBuffer *outResults);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateListBelowPoint(
    zClass_NodePartial *world, PlayerProbeSampleCandidateBuffer *outResults, float x, float maxY,
    float z);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidateList(zClass_NodePartial *node,
                                                                    int cullCount);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesRecursive(zClass_NodePartial *node,
                                                                          int cullCount);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForLight(zClass_NodePartial *node,
                                                                         int cullCount);
RECOIL_NOINLINE int RECOIL_FASTCALL
IsPickQueryPointOutsideViewBBoxXZ(zClass_NodePartial *node);
RECOIL_NOINLINE int RECOIL_FASTCALL
TryGetPolygonHitAtQueryXZ(zClassDiPickCandidateEntry *candidate, const zVec3 *polygonVertices,
                          float queryX, float queryZ, int vertexCount);
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
RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegionsAgainstMeshFaces(
    zVec3 *meshVertices, int faceCount);
RECOIL_NOINLINE int RECOIL_FASTCALL FilterRegionsAgainstHexahedronFaces(
    zVec3 *center, float radius);
RECOIL_NOINLINE int RECOIL_FASTCALL FilterPointsBBox(zClass_NodePartial *node,
                                                              void *pointData);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildPickCandidatesForSegmentVsBBoxFaces(
    const zBBoxCorners *bboxCorners, zClassDiPickCandidateEntry *candidate,
    const zVec3 *segmentStart, const zVec3 *segmentEnd);
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
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, currentFrame) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, framesPerSecond) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, frameCount) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, frameWriteCount) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialCyclePartial, frameTable) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zModel_MaterialCyclePartial) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, flagsAndIndexCount) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zDiEntryPartial, unknown_04) == 0x04);
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
RECOIL_STATIC_ASSERT(sizeof(zModel_PickFaceUvData) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceScenePayload, flags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceEntry, flagsAndVertexCount) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceEntry, vertexIndices) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceEntry, faceUvData) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceEntry, scenePayload) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zModel_PickFaceEntry) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, flags) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, faceCount) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, morphVertexCount) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, morphWeight) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, faces) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, baseVertices) == 0x34);
RECOIL_STATIC_ASSERT(offsetof(zModel_PickFaceData, morphVertices) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(zModel_PickFaceData) == 0x44);

#endif // RECOIL_GAMEZRECOIL_INCLUDE_ZDI_H
