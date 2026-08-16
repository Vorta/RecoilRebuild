#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>
#include <set>
#include <vector>

#include "GameZRecoil/zGeometry/zgeo.h"
#include "recoil/recoil_callconv.h"
#include "zclass.h"

struct zModel_MaterialPartial;
struct zModel_MaterialSlot;
struct zEffectAnimEntry;
struct zZbdSectionCallbackCtx;
struct NetPkt0F_CraterEvent;
struct NetPkt10_QSandEvent;

namespace zReader {
struct Node;
}

typedef int(__fastcall *zDEClient_NetRelayCallback)(void *eventTemplate);

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

RECOIL_STATIC_ASSERT(sizeof(zDEClient_QSandEventTemplate) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_CraterEventTemplate) == 0x28);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_FeatureGridCell,
        featureCount
    ) == 0x39
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_FeatureGridCell,
        nodeCount
    ) == 0x3a
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_FeatureGridCell,
        nodes
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_FeatureGridCell) == 0x40);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_CameraNodeClassDataPartial,
        featureGridRows
    ) == 0x80
);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_FeatureEntry) == 0x34);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_CraterDisplaySourceEntry) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_QSandFeature,
        eventTemplate
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_QSandFeature,
        points
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_QSandFeature,
        featureGridCell
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_QSandFeature,
        clipPatchOutput
    ) == 0x48
);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_QSandFeature) == 0x50);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_CraterFeature,
        eventTemplate
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_CraterFeature,
        points
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_CraterFeature,
        boundsMinX
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_CraterFeature,
        featureGridCell
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_CraterFeature,
        clipPatchOutput
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_CraterFeature,
        displaySourceEntry
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(sizeof(zDEClient_CraterFeature) == 0x50);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_FeatureContextOverlapView,
        bounds_30
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zDEClient_FeatureContextOverlapView,
        bounds_40
    ) == 0x40
);
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
extern char g_zDEClient_NodeName[0x0a];
extern char g_zDEClient_QuickSandUntexturedMsg[0x20];
extern char g_zDEClient_QuickSandNodeName[0x0b];
extern char g_zDEClient_TextureAnimNodeName[0x0d];
extern char g_zDEClient_DefaultAnimNodeName[0x0d];
extern char g_zDEClient_DefaultTextureNodeName[0x10];
extern char g_zDEClient_RadiusFieldName[0x07];
extern char g_zDEClient_DepthFieldName[0x06];
extern char g_zDEClient_SlopeFieldName[0x06];
extern char g_zDEClient_PointsFieldName[0x07];
extern char g_zDEClient_CraterNodeName[0x07];
extern char g_zDEClient_ReadDefaultsFallbackFmt[0x24];
extern char g_zDEClient_ConfigArchiveName[0x0d];
extern char g_zDEClient_SourceFile_ZdecInitCpp[0x2c];
extern char g_zDEClient_WorldNodeNullErrorMsg[0x28];
extern char g_zDEClient_QuickSandInstanceTessellationFailedMsg[0x32];
extern char g_zDEClient_QuickSandInstanceClipFailedMsg[0x2b];
extern char g_zDEClient_SourceFile_ZdecQsandCpp[0x2d];
extern char g_zDEClient_QuickSandInstanceBuildFailedMsg[0x2c];
extern char g_zDEClient_FeatureNodeName[0x0d];
extern char g_zDEClient_CraterInstanceTessellationFailedMsg[0x2e];
extern char g_zDEClient_CraterInstanceClipFailedMsg[0x27];
extern char g_zDEClient_SourceFile_ZdecCraterCpp[0x2e];
extern char g_zDEClient_CraterInstanceBuildFailedMsg[0x28];
extern char g_zDEClient_CraterNameFmt[0x09];
extern char g_zDEClient_QuickSandNameFmt[0x08];
extern std::vector<zDEClient_FeatureEntry> g_zDEClient_FeatureList;
extern std::set<zGeometry_ClipPatchNodeView *> g_zDEClient_FeatureMapTree;
extern zClass_NodePartial *g_zDEClient_CameraNode;
extern zClass_CameraDataPartial *g_zDEClient_CameraNodeClassData;
extern zDEClient_NetRelayCallback g_zDEClientQSandNetRelayCallback;
extern zDEClient_NetRelayCallback g_zDEClientCraterNetRelayCallback;

typedef int(__fastcall *zDEClient_CraterFeatureDispatch)(
    zDEClient_CraterEventTemplate *eventTemplate
);
typedef int(__fastcall *zDEClient_QSandFeatureDispatch)(
    zDEClient_QSandEventTemplate *eventTemplate
);

namespace zDEClient {
int __fastcall LoadConfigResources(zClass_NodePartial *worldNode);
RECOIL_NO_GS int __fastcall LoadMaterialFromTexturePath_Local(
    zModel_MaterialPartial **outMaterial,
    char *texturePath
);
void __stdcall ApplyFeatureEntry(
    zDEClient_FeatureEntry *container,
    void *unused0,
    void *unused1
);
void __fastcall DispatchFeatureEventTemplates(
    zDEClient_CraterFeatureDispatch craterHandler,
    zDEClient_QSandFeatureDispatch qSandHandler
);
int __cdecl ShutdownGlobals();
int __cdecl ClearFeatureEntriesAndMapTree();
void __cdecl ClearFeatureDisplayNodes();
void __fastcall SetCameraNode(zClass_NodePartial *cameraNode);
int __fastcall WriteFeatureSectionsToZAR(zZbdSectionCallbackCtx *callbackCtx);
void __fastcall CopyQSandEventTemplateDefaults(
    zDEClient_QSandEventTemplate *eventTemplate
);
zDEClient_FeatureGridCell *__fastcall GetFeatureGridCell(
    int gridCol,
    int gridRow
);
zClass_NodePartial *__cdecl GetCameraNode();
zDiPartial *__fastcall CreateFeatureNodeAndDiFromClipPatchPartition(
    zGeometry_ClipPatchPartitionOutput *partitionOutput,
    zClass_NodePartial *parentNode,
    zClass_NodePartial **outNode
);
int __fastcall AppendFeatureEntry(
    int featureType,
    const void *featureEventData
);
void __fastcall SubmitFeatureGeometry(
    zGeometry_ClipPatchOutputPartial *clipPatchOutput
);
} // namespace zDEClient

namespace zDEClient_Crater {
int __fastcall Execute(zDEClient_CraterEventTemplate *eventTemplate);
int __fastcall NetRelayCallback(
    int senderPlayerId,
    NetPkt0F_CraterEvent *packet
);
void __fastcall DestroyFeature(zDEClient_CraterFeature *featureInstance);
void __fastcall InitEventTemplateDefaults(
    zDEClient_CraterEventTemplate *eventTemplate
);
int __fastcall InstanceEvent(
    zDEClient_CraterEventTemplate *eventTemplate,
    int playEffectAnim
);
int __fastcall InstanceEventMaybeRelay(
    zDEClient_CraterEventTemplate *eventTemplate
);
zDEClient_CraterFeature *__fastcall CreateFeatureStructFromEventTemplate(
    zDEClient_CraterEventTemplate *eventTemplate
);
zDEClient_CraterFeature *__fastcall InitFeatureFromEventTemplate(
    zDEClient_CraterEventTemplate *eventTemplate
);
int __fastcall Build(zDEClient_CraterFeature *featureInstance);
int __fastcall CreateFeature(zDEClient_CraterFeature *featureInstance);
} // namespace zDEClient_Crater

namespace zDEClient_QSand {
int __fastcall NetRelayCallback(
    int senderPlayerId,
    NetPkt10_QSandEvent *packet
);
void __fastcall DestroyFeature(zDEClient_QSandFeature *featureInstance);
zDEClient_QSandFeature *__fastcall CreateFeatureStructFromEventTemplate(
    zDEClient_QSandEventTemplate *eventTemplate
);
zDEClient_QSandFeature *__fastcall InitFeatureFromEventTemplate(
    zDEClient_QSandEventTemplate *eventTemplate
);
int __fastcall Build(zDEClient_QSandFeature *featureInstance);
int __fastcall CreateFeature(zDEClient_QSandFeature *featureInstance);
int __fastcall InstanceEventMaybeRelay(
    zDEClient_QSandEventTemplate *eventTemplate
);
} // namespace zDEClient_QSand
