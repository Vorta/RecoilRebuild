#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/zGeometry/zGeometry.h"
#include "recoil/recoil_callconv.h"
#include "zClass.h"

struct zDEClient_MapTreeNode;
struct zModel_MaterialPartial;
struct zModel_MaterialSlot;
struct zEffectAnimEntry;
struct zZbdSectionCallbackCtx;

namespace zReader {
struct Node;
}

typedef int (RECOIL_CDECL *zDEClient_NetRelayCallback)();

struct zDEClient_FeatureGridCell {
    int areaFlags;
    int areaIndex;
    float originX;
    float originZ;
    float bbox[6];
    zVec3 bboxCenter;
    float bboxRadius;
    unsigned char unknown_38;
    unsigned char featureCount;
    short nodeCount;
    zGeometry_ClipPatchNodeView **nodes;
};

struct zDEClient_CameraNodeClassDataPartial {
    unsigned char unknown_00[0x80];
    zDEClient_FeatureGridCell **featureGridRows;
};

struct zDEClient_QSandEventTemplate {
    int featureFlags;
    int pointCount;
    zModel_MaterialPartial *material;
    zModel_MaterialPartial *materialCycle;
    float slope;
    float depth;
    float radius;
    zVec3 center;
    zClass_NodePartial *damageOwnerNode;
};

struct zDEClient_CraterEventTemplate {
    int featureFlags;
    int pointCount;
    zModel_MaterialSlot *craterMaterialSlot;
    float slope;
    float depth;
    float radius;
    zVec3 center;
    zClass_NodePartial *damageOwnerNode;
};

union zDEClient_FeatureEventData {
    zDEClient_QSandEventTemplate quickSand;
    zDEClient_CraterEventTemplate crater;
};

struct zDEClient_FeatureEntry {
    int featureType;
    zDEClient_FeatureEventData eventData;
    int reloadFlag;
};

struct zDEClient_CraterDisplaySourceEntry {
    zModel_MaterialPartial *sourceMaterial;
    zModel_MaterialPartial *craterMaterial;
    zEffectAnimEntry *effectAnimEntry;
};

struct zDEClient_QSandFeature {
    int featureType;
    zDEClient_QSandEventTemplate eventTemplate;
    zVec3 *points;
    float boundsMinX;
    float boundsMinZ;
    float boundsMaxX;
    float boundsMaxZ;
    zDEClient_FeatureGridCell *featureGridCell;
    zGeometry_ClipPatchOutputPartial *clipPatchOutput;
    unsigned char unknown_4c[0x04];
};

struct zDEClient_CraterFeature {
    int featureType;
    zDEClient_CraterEventTemplate eventTemplate;
    zVec3 *points;
    float boundsMinX;
    float boundsMinZ;
    float boundsMaxX;
    float boundsMaxZ;
    zDEClient_FeatureGridCell *featureGridCell;
    zGeometry_ClipPatchOutputPartial *clipPatchOutput;
    unsigned char unknown_48[0x04];
    zDEClient_CraterDisplaySourceEntry *displaySourceEntry;
};

struct zDEClient_FeatureContextOverlapView {
    int featureType;
    unsigned char unknown_04[0x2c];
    float bounds_30;
    float bounds_34;
    float bounds_38;
    float bounds_3c;
    float bounds_40;
};

struct zDEClient_MapTreeNode {
    zDEClient_MapTreeNode *left;
    zDEClient_MapTreeNode *parent;
    zDEClient_MapTreeNode *right;
    zGeometry_ClipPatchNodeView *key;
    int colorOrNil;
};

struct zDEClient_MapTreeLocateResult {
    zDEClient_MapTreeNode *node;
    unsigned char inserted;
    unsigned char unknown_05[0x03];
};

struct zDEClient_MapTreeState {
    unsigned char mode;
    unsigned char flags;
    unsigned char unknown_02[0x02];
    zDEClient_MapTreeNode *header;
    int allowInsert;
    int nodeCount;

    RECOIL_NOINLINE zDEClient_MapTreeNode **RECOIL_THISCALL EraseRange(
        zDEClient_MapTreeNode **outNext, zDEClient_MapTreeNode *first, zDEClient_MapTreeNode *last);
    RECOIL_NOINLINE zDEClient_MapTreeNode **RECOIL_THISCALL
    EraseAndAdvance(zDEClient_MapTreeNode **outNext, zDEClient_MapTreeNode *node);
    RECOIL_NOINLINE void RECOIL_THISCALL DestroySubtree(zDEClient_MapTreeNode *node);
    RECOIL_NOINLINE zDEClient_MapTreeNode **RECOIL_THISCALL
    IterNextNodeRef(zDEClient_MapTreeNode **nodeRef);
    RECOIL_NOINLINE void RECOIL_THISCALL IterPrevNodeRef(zDEClient_MapTreeNode **nodeRef);
    RECOIL_NOINLINE zDEClient_MapTreeNode **RECOIL_THISCALL
    InsertAt(zDEClient_MapTreeNode **outNode, zDEClient_MapTreeNode *where,
             zDEClient_MapTreeNode *parent, zGeometry_ClipPatchNodeDiPair *key);
    RECOIL_NOINLINE zDEClient_MapTreeLocateResult *RECOIL_THISCALL
    FindOrInsertKey(zDEClient_MapTreeLocateResult *outResult, zGeometry_ClipPatchNodeDiPair *key);
};

RECOIL_STATIC_ASSERT(sizeof(zDEClient_QSandEventTemplate) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_CraterEventTemplate) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_FeatureGridCell, featureCount) == 0x39);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_FeatureGridCell, nodeCount) == 0x3a);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_FeatureGridCell, nodes) == 0x3c);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_FeatureGridCell) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_CameraNodeClassDataPartial, featureGridRows) == 0x80);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_FeatureEntry) == 0x34);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_CraterDisplaySourceEntry) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_QSandFeature, eventTemplate) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_QSandFeature, points) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_QSandFeature, featureGridCell) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_QSandFeature, clipPatchOutput) == 0x48);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_QSandFeature) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_CraterFeature, eventTemplate) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_CraterFeature, points) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_CraterFeature, boundsMinX) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_CraterFeature, featureGridCell) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_CraterFeature, clipPatchOutput) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_CraterFeature, displaySourceEntry) == 0x4c);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_CraterFeature) == 0x50);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_FeatureContextOverlapView, bounds_30) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_FeatureContextOverlapView, bounds_40) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_MapTreeNode, parent) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_MapTreeNode, colorOrNil) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_MapTreeNode) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_MapTreeLocateResult) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_MapTreeState, header) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zDEClient_MapTreeState, nodeCount) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_MapTreeState) == 0x10);

extern int g_zDEClient_QuickSandEnabled;
extern int g_zDEClient_QuickSandTextureCount;
extern float g_zDEClient_QuickSandAnimSpeed;
extern char **g_zDEClient_QuickSandTexturePaths;
extern zModel_MaterialPartial *g_zDEClient_QuickSandMaterial;
extern zModel_MaterialPartial *g_zDEClient_QuickSandMaterialCycle;
extern zDEClient_QSandEventTemplate g_zDEClient_QuickSandEventTemplateDefaults;
extern int g_zDEClient_CraterDisplaySourceCount;
extern zDEClient_CraterDisplaySourceEntry *g_zDEClient_CraterDisplaySourceList;
extern zDEClient_CraterEventTemplate g_zDEClient_CraterEventTemplateDefaults;
extern zReader::Node *g_zDEClient_ConfigReaderRoot;
extern int g_zDEClient_RebuildBltRectOnReload;
extern zDEClient_FeatureEntry *g_zDEClient_FeatureListBegin;
extern zDEClient_FeatureEntry *g_zDEClient_FeatureListEnd;
extern zDEClient_FeatureEntry *g_zDEClient_FeatureListCapacityEnd;
extern zDEClient_MapTreeState g_zDEClient_FeatureMapTree;
extern zDEClient_MapTreeNode *g_zDEClient_FeatureMapTreeNil;
extern int g_zDEClient_FeatureMapTreeNilRefCount;
extern zClass_NodePartial *g_zDEClient_CameraNode;
extern zClass_CameraDataPartial *g_zDEClient_CameraNodeClassData;
extern zDEClient_NetRelayCallback g_zDEClientQSandNetRelayCallback;
extern zDEClient_NetRelayCallback g_zDEClientCraterNetRelayCallback;

namespace zDEClient {
RECOIL_NOINLINE int RECOIL_FASTCALL LoadConfigResources(zClass_NodePartial *worldNode);
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL
LoadMaterialFromTexturePath_Local(zModel_MaterialPartial **outMaterial, char *texturePath);
RECOIL_NOINLINE void RECOIL_STDCALL ApplyFeatureEntry(zDEClient_FeatureEntry *container,
                                                      void *unused0, void *unused1);
RECOIL_NOINLINE int RECOIL_CDECL ShutdownGlobals();
RECOIL_NOINLINE int RECOIL_CDECL ClearFeatureEntriesAndMapTree();
RECOIL_NOINLINE void RECOIL_CDECL ClearFeatureDisplayNodes();
RECOIL_NOINLINE void RECOIL_FASTCALL SetCameraNode(zClass_NodePartial *cameraNode);
RECOIL_NOINLINE int RECOIL_FASTCALL
WriteFeatureSectionsToZAR(zZbdSectionCallbackCtx *callbackCtx);
RECOIL_NOINLINE void RECOIL_FASTCALL
CopyQSandEventTemplateDefaults(zDEClient_QSandEventTemplate *eventTemplate);
RECOIL_NOINLINE zDEClient_FeatureGridCell *RECOIL_FASTCALL GetFeatureGridCell(int gridCol,
                                                                              int gridRow);
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL GetCameraNode();
RECOIL_NOINLINE zDiPartial *RECOIL_FASTCALL CreateFeatureNodeAndDiFromClipPatchPartition(
    zGeometry_ClipPatchPartitionOutput *partitionOutput, zClass_NodePartial *parentNode,
    zClass_NodePartial **outNode);
RECOIL_NOINLINE zDEClient_FeatureEntry *RECOIL_STDCALL
CopyFeatureEntriesForward(zDEClient_FeatureEntry *first, zDEClient_FeatureEntry *last,
                          zDEClient_FeatureEntry *dest);
RECOIL_NOINLINE void RECOIL_STDCALL FillFeatureEntries(zDEClient_FeatureEntry *dest,
                                                       unsigned int count,
                                                       const zDEClient_FeatureEntry *value);
RECOIL_NOINLINE int RECOIL_FASTCALL AppendFeatureEntry(int featureType,
                                                                const void *featureEventData);
RECOIL_NOINLINE void RECOIL_FASTCALL
SubmitFeatureGeometry(zGeometry_ClipPatchOutputPartial *clipPatchOutput);
} // namespace zDEClient

namespace zDEClient_Crater {
RECOIL_NOINLINE void RECOIL_FASTCALL DestroyFeature(zDEClient_CraterFeature *featureInstance);
RECOIL_NOINLINE void RECOIL_FASTCALL
InitEventTemplateDefaults(zDEClient_CraterEventTemplate *eventTemplate);
RECOIL_NOINLINE int RECOIL_FASTCALL
InstanceEvent(zDEClient_CraterEventTemplate *eventTemplate, int playEffectAnim);
RECOIL_NOINLINE int RECOIL_FASTCALL
InstanceEventMaybeRelay(zDEClient_CraterEventTemplate *eventTemplate);
RECOIL_NOINLINE zDEClient_CraterFeature *RECOIL_FASTCALL
CreateFeatureStructFromEventTemplate(zDEClient_CraterEventTemplate *eventTemplate);
RECOIL_NOINLINE zDEClient_CraterFeature *RECOIL_FASTCALL
InitFeatureFromEventTemplate(zDEClient_CraterEventTemplate *eventTemplate);
RECOIL_NOINLINE int RECOIL_FASTCALL Build(zDEClient_CraterFeature *featureInstance);
RECOIL_NOINLINE int RECOIL_FASTCALL
CreateFeature(zDEClient_CraterFeature *featureInstance);
} // namespace zDEClient_Crater

namespace zDEClient_QSand {
RECOIL_NOINLINE void RECOIL_FASTCALL DestroyFeature(zDEClient_QSandFeature *featureInstance);
RECOIL_NOINLINE zDEClient_QSandFeature *RECOIL_FASTCALL
CreateFeatureStructFromEventTemplate(zDEClient_QSandEventTemplate *eventTemplate);
RECOIL_NOINLINE zDEClient_QSandFeature *RECOIL_FASTCALL
InitFeatureFromEventTemplate(zDEClient_QSandEventTemplate *eventTemplate);
RECOIL_NOINLINE int RECOIL_FASTCALL Build(zDEClient_QSandFeature *featureInstance);
RECOIL_NOINLINE int RECOIL_FASTCALL CreateFeature(zDEClient_QSandFeature *featureInstance);
RECOIL_NOINLINE int RECOIL_FASTCALL
InstanceEventMaybeRelay(zDEClient_QSandEventTemplate *eventTemplate);
} // namespace zDEClient_QSand
