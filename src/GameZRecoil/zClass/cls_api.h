#pragma once

/*
 * C declarations of the zClass API for the zClass, zEffect and zWeapon units
 * (C++ units see the same functions with C linkage in zclass.h and zdi.h; the
 * zMath and zModel C units see only the zclass.h C view).
 */

#include "recoil/recoil_callconv.h"
#include "zclass.h"
#include "zdi.h"

extern zVec3 g_zCamera_FrustumFootprintPoints[5];
extern int g_zCamera_FrustumFootprintPointCount;
extern zCamera_FrustumGridTileRingPartial g_zCamera_FrustumGridTileRings[50];
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
extern CZNodeFreeListSlot* g_CZClass_NodeArray;
extern int g_CZClass_NodeArraySize;
extern int g_CZClass_ActiveNodeCount;
extern int g_CZClass_NodeFreeHeadIndex;
extern int g_CZClass_IsInitialized;
extern CZTypeListLink* g_CZTypeList_FreeLinkHead;
extern CZTypeListLink* g_CZNodeList_PendingFreeHead;
extern int g_CZClass_DeferredProcessingEnabled;
extern int g_CZTypeList_LiveLinkCount;
extern int g_CZTypeList_PeakLiveLinkCount;
extern CZTypeListBucket g_CZTypeList_Buckets[16];
extern CZTypeListLink** g_CZClassCallbackPriorityHeadSlotPtrs[6];
extern CZTypeListLink** g_CZTypeList_HeadSlotPtrs[16];
extern CZTypeListLink** g_CZTypeList_TailSlotPtrs[16];
extern CZTypeListLink* g_CZClass_FilterIterCursor;
extern unsigned int g_CZClass_FilterIterUnknownDword0;
extern const char* g_CZClass_FilterIterText;
extern unsigned int g_CZClass_FilterIterUnknownDword1;
extern int g_CZClass_FilterIterPrefixLen;
extern char g_CZClass_CurrentZbdPath[0x30];
extern CZNodePartial** g_GameZ_Zbd_NodeIndexScratch;
extern int g_GameZ_Zbd_NodeIndexScratchCapacity;
extern int g_CZClass_CameraAutoClipDistanceAdjustEnabled;
extern float g_CZClass_CameraAutoClipDistanceThreshold;
extern float g_CZClass_CameraAutoClipDistanceScale;
extern float g_CZClass_CameraAutoClipDistanceStep;
extern float g_CZClass_CameraAutoClipDistanceMinScale;
extern int g_CZClass_ObjectHseTestEnabled;
extern CZNodePartial* g_CZClass_CurrentCamera;
extern CZNodePartial* g_CZClass_CameraTargetNode;
extern char g_CZClass_VapStaticsNodeName[0x0c];
extern CZNodePartial* g_MainCamera;
extern CZCameraDataPartial* g_zVideo_pActiveViewContext;
extern CZNodePartial* g_Player_RuntimeDiScene;
extern int g_CZClass_CopyNodeCloneDiMode;
extern int g_CZClass_CopyNodeDiArg0;
extern int g_CZClass_CopyNodeDiArg1;
extern int g_CZClass_RebuildGwWorldBltRectOnShutdown;
extern char g_CZClass_GWWorldNodeName[8];
extern CZDisplayInstanceRaycastFilterRuntime g_CZDisplayInstance_RaycastFilterRuntime;

/* C views of the zclass.h node-sphere helpers (C++ adds const overloads). */
__inline zVec3* zClassNodeViewSphereCenter(CZNodePartial* node)
{
    return (zVec3*)node->cachedSphereCenter;
}

__inline float* zClassNodeViewSphereRadius(CZNodePartial* node)
{
    return &node->cachedSphereCenter[3];
}

void __fastcall ExpandToCorners(const zBBox3f* bbox, zBBoxCorners* outCorners);
float* __fastcall MinMaxToBoundingSphere(const zBBox3f* bbox, zVec3* outCenter, float* outRadius);
void __fastcall CornersToBoundingSphere(zBBoxCorners* corners, zVec3* outCenter, float* outRadius);

void __fastcall Clear(zTag4Partial* tag);

CZNodePartial* __cdecl gwWindowNew();
int __fastcall gwWindowSetResolution(CZNodePartial* node, int width, int height);
int __fastcall gwWindowGetResolution(CZNodePartial* node, int* outWidth, int* outHeight);
int __fastcall gwWindowSetSize(CZNodePartial* node, int width, int height);
int __fastcall gwWindowGetSize(CZNodePartial* node, int* outWidth, int* outHeight);
int __fastcall gwWindowSetBuffer(CZNodePartial* node, int bufferIndex);
int __fastcall gwWindowSetClearPolygon(CZNodePartial* node, int enabled);
int __fastcall gwWindowAddClearPolygonVertex(CZNodePartial* node, const zVec3* point);
int __fastcall gwWindowCloseClearPolygon(CZNodePartial* node);
int __fastcall CZWindowDeleteNode(CZNodePartial* node);

CZNodePartial* __cdecl gwDisplayInit();
int __fastcall gwDisplaySetSize(CZNodePartial* node, int width, int height);
int __fastcall gwDisplaySetPosition(CZNodePartial* node, int x, int y);
int __fastcall gwDisplaySetBackgroundColor(CZNodePartial* node, float red, float green, float blue);
int __fastcall CZDisplayDeleteNode(CZNodePartial* node);
int __fastcall CZDisplayRemoveChild(CZNodePartial* parent, CZNodePartial* child);

int __fastcall WriteSettingsSection(zZbdSectionCallbackCtx* callbackCtx, void* userData);
void __fastcall ReadSettingsSection(
    zZbdSectionCallbackCtx* callbackCtx,
    const char* worldName,
    CZWorldSettingsSectionRecord* settings,
    unsigned int size,
    void* userData
);
CZNodePartial* __cdecl gwWorldNew();
int __fastcall FreeVirtualAreaPartitions(CZNodePartial* world);
int __fastcall QueueAreaUpdate(CZNodePartial* world, CZWorldDataPartial* worldData, zWorldAreaPartial* area);
int __fastcall RebuildAreaBounds(CZWorldDataPartial* worldData, zWorldAreaPartial* area);
int __fastcall ApplyPendingFogSettings(CZNodePartial* world);
int __fastcall SetPendingFogState(CZNodePartial* world, int fogState);
int __fastcall SetPendingFogColorRgb01(CZNodePartial* world, float red, float green, float blue);
int __fastcall SetPendingFogAltitudeRange(CZNodePartial* world, float minAlt, float maxAlt);
int __fastcall SetPendingFogRange(CZNodePartial* world, float nearRange, float farRange);
int __fastcall GetPendingFogDensity(CZNodePartial* world, float* outDensity);
int __fastcall GetPendingFogState(CZNodePartial* world, int* outState);
int __fastcall GetPendingFogColorRgb01(CZNodePartial* world, float* outRed, float* outGreen, float* outBlue);
int __fastcall GetPendingFogRange(CZNodePartial* world, float* outNearRange, float* outFarRange);
int __fastcall GetPendingFogAltitudeRange(CZNodePartial* world, float* outMinAlt, float* outMaxAlt);
int __fastcall SetPendingFogDensity(CZNodePartial* world, float density);
int __fastcall gwWorldSetOrigin(CZNodePartial* world, float originX, float originZ);
int __fastcall gwWorldSetSize(CZNodePartial* world, float sizeX, float sizeZ);
int __fastcall gwWorldSetPartitionInclusionTolerance(CZNodePartial* world, float toleranceX, float toleranceZ);
int __fastcall gwWorldSetMaxDecFeatures(CZNodePartial* world, int maxFeatures);
int __fastcall gwWorldSetVirtualAreaPartition(CZNodePartial* world, float cellSizeX, float cellSizeZ);
int __fastcall InitVirtualAreaPartitions(CZNodePartial* world);
int __fastcall SetVirtualPartition(CZNodePartial* world, int enabled);
// Grid query parameter order (inputs, then out-pointers) follows the retail call-site argument order.
int __fastcall WorldRectToGridIndex(
    CZNodePartial* world,
    float minX,
    float maxX,
    float minZ,
    float maxZ,
    int* outGridCol,
    int* outGridRow
);
int __fastcall WorldToGridCoordsClampedEx(
    CZNodePartial* world,
    float worldX,
    float worldZ,
    int* outGridCol,
    int* outGridRow,
    int* clampedGridColOut,
    int* clampedGridRowOut,
    int* insideBoundsOut
);
int __fastcall
WorldToGridCoordsClamped(CZNodePartial* world, float worldX, float worldZ, int* outGridCol, int* outGridRow);
zWorldAreaPartial* __fastcall GetAreaPartitionAtGrid(CZNodePartial* world, int gridCol, int gridRow);
int __fastcall AddChildAtGrid(CZNodePartial* world, CZNodePartial* child);
int __fastcall EnsureGridCellDisplayPosition(CZNodePartial* world, int gridCol, int gridRow);
int __fastcall AddChildToGridCell(CZNodePartial* world, CZNodePartial* child, int gridCol, int gridRow);
int __fastcall RemoveChildAtGrid(CZNodePartial* world, CZNodePartial* child);
int __fastcall AddLight(CZNodePartial* world, CZNodePartial* light);
int __fastcall RemoveLight(CZNodePartial* world, CZNodePartial* light);
int __fastcall InitLightPointInPolygonXZ(CZNodePartial* world);
int __fastcall UpdateAllLights(CZNodePartial* world);
int __fastcall AddSound(CZNodePartial* world, CZNodePartial* sound);
int __fastcall RemoveSound(CZNodePartial* world, CZNodePartial* sound);
int __fastcall UpdateAllSounds(CZNodePartial* world);
int __fastcall CZWorldDeleteNode(CZNodePartial* world);

CZNodePartial* __fastcall gwObject3DInit();
int __fastcall PropagateTransformDirty(CZNodePartial* node);
int __fastcall gwObject3DSetVisibleFlag(CZNodePartial* node, int visible);
int __fastcall gwObject3DSetColorAlpha(CZNodePartial* node, zColorRgb* color, float alpha);
int __fastcall gwObject3DSetAlphaScale(CZNodePartial* node, float alphaScale);
int __fastcall gwObject3DGetAlphaScale(CZNodePartial* node, float* outAlphaScale);
int __fastcall gwObject3DSetLitFlag(CZNodePartial* node, int lit);
int __fastcall gwObject3DSetScale(CZNodePartial* node, float x, float y, float z);
int __fastcall gwObject3DGetScale(CZNodePartial* node, float* outX, float* outY, float* outZ);
int __fastcall gwObject3DGetRotation(CZNodePartial* node, float* outX, float* outY, float* outZ);
int __fastcall gwObject3DSetRotation(CZNodePartial* node, float x, float y, float z);
int __fastcall gwObject3DTranslateRotation(CZNodePartial* node, float dx, float dy, float dz);
int __fastcall gwObject3DGetPosition(CZNodePartial* node, float* outX, float* outY, float* outZ);
int __fastcall gwObject3DSetPosition(CZNodePartial* node, float x, float y, float z);
int __fastcall gwObject3DTranslatePosition(CZNodePartial* node, float dx, float dy, float dz);
float* __fastcall gwObject3DGetMatrixPtr(CZNodePartial* node);
int __fastcall gwObject3DSetMatrix(CZNodePartial* node, float* matrix);
int __fastcall gwObject3DAddChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall CZObject3DRenderTraverse(CZNodePartial* node, int siblingCountHint);
int __fastcall CZObject3DRemoveChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall CZObject3DDeleteNode(CZNodePartial* node);

CZNodePartial* __cdecl gwLodNew();
int __fastcall gwLodAddChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall SetComputeOwnDistance(CZNodePartial* node, int enabled);
int __fastcall SetTargetNodeAndRange(CZNodePartial* node, CZNodePartial* target, float range);
int __fastcall CZLodDeleteNode(CZNodePartial* node);
int __fastcall CZLodRenderTraverse(CZNodePartial* node, int siblingCountHint);
int __fastcall CZLodRemoveChild(CZNodePartial* parent, CZNodePartial* child);

CZNodePartial* __fastcall gwLightNew();
int __fastcall gwLightSetIntensity(CZNodePartial* node, float intensity);
int __fastcall gwLightSetFalloff(CZNodePartial* node, float falloff);
int __fastcall gwLightSetDirectional(CZNodePartial* node, int directional);
int __fastcall gwLightSetDirectedSource(CZNodePartial* node);
int __fastcall gwLightSetPointSource(CZNodePartial* node);
int __fastcall gwLightSetParam(CZNodePartial* node, int param);
int __fastcall gwLightSetRange(CZNodePartial* node, float rangeA, float rangeB);
int __fastcall gwLightGetRange(CZNodePartial* node, float* outRange1, float* outRange2);
int __fastcall gwLightSetPosition(CZNodePartial* node, float x, float y, float z);
int __fastcall gwLightSetRotation(CZNodePartial* node, float x, float y, float z);
int __fastcall gwLightUpdate(CZNodePartial* node);
int __fastcall gwLightGetSpecularColor(CZNodePartial* node, float* outRed, float* outGreen, float* outBlue);
int __fastcall gwLightSetSpecularColor(CZNodePartial* node, float red, float green, float blue);
int __fastcall CZLightRenderTraverse(CZNodePartial* node, int siblingCountHint);
int __fastcall CZLightDeleteNode(CZNodePartial* node);
int __fastcall CZLightRemoveChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall CZLightComputeWorldTransform(CZNodePartial* node, CZLightDataPartial* data);

CZNodePartial* __cdecl gwCameraNew();
int __fastcall gwCameraAddChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall gwCameraRemoveChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall gwCameraSetActive(CZNodePartial* node, int active);
int __fastcall gwCameraSetFlagBit0(CZNodePartial* node, int enabled);
int __fastcall SetTargetNode(CZNodePartial* target);
CZNodePartial* __fastcall SetActiveCamera(CZNodePartial* camera);
int __fastcall SetObjectHseTestEnabled(int enabled);
int __fastcall gwCameraSetWorld(CZNodePartial* camera, CZNodePartial* world);
CZNodePartial* __fastcall gwCameraGetWorld(CZNodePartial* camera);
int __fastcall gwCameraSetWindow(CZNodePartial* camera, CZNodePartial* window);
int __fastcall ActivateChildren(CZNodePartial* camera, CZCameraDataPartial* data);
int __fastcall gwCameraSetEulerAngles(CZNodePartial* camera, float x, float y, float z);
int __fastcall gwCameraAddEulerAngles(CZNodePartial* camera, float dx, float dy, float dz);
int __fastcall gwCameraGetEulerAngles(CZNodePartial* camera, float* outX, float* outY, float* outZ);
int __fastcall gwCameraSetPosition(CZNodePartial* camera, float x, float y, float z);
int __fastcall gwCameraTranslate(CZNodePartial* camera, float dx, float dy, float dz);
int __fastcall gwCameraGetPosition(CZNodePartial* camera, float* outX, float* outY, float* outZ);
int __fastcall gwCameraSetNearFarClip(CZNodePartial* camera, float nearClip, float farClip);
int __fastcall gwCameraGetNearFarClip(CZNodePartial* camera, float* outNear, float* outFar);
int __fastcall gwCameraSetViewport(CZNodePartial* camera, float viewportWidth, float viewportHeight);
int __fastcall gwCameraGetViewport(CZNodePartial* camera, float* outWidth, float* outHeight);
int __fastcall gwCameraGetFOV(CZNodePartial* camera, float* outFovX, float* outFovY);
int __fastcall gwCameraSetFOV(CZNodePartial* camera, float fovX, float fovY);
int __fastcall gwCameraGetClipDistance(CZNodePartial* camera, float* outClipDistance);
int __fastcall gwCameraSetClipDistance(CZNodePartial* camera, float clipDistance);
int __fastcall gwCameraSetHorizon(CZNodePartial* camera, CZNodePartial* horizonNode);
int __fastcall gwCameraSetHorizonXZ(CZNodePartial* camera, CZNodePartial* horizonXZNode);
void __fastcall SetViewDistance(int enableAutoClip, float distance);
float __fastcall theta_x_z(zVec3* point1, zVec3* point2);
int __fastcall find_convex_hull_xz(zVec3* points, int count);
int __fastcall
BuildFrustumGridTiles(CZNodePartial* world, CZWorldDataPartial* worldData, CZCameraDataPartial* cameraData);
int __fastcall
BuildFrustumGridTilesFromParams(CZNodePartial* world, CZWorldDataPartial* worldData, CZCameraDataPartial* cameraData);
void __fastcall RenderFrustumGridTiles(CZNodePartial* world, CZNodePartial* camera, CZCameraDataPartial* cameraData);
void __fastcall RenderOverlayNodes(CZNodePartial* world);
void __fastcall RenderWorld(CZNodePartial* world, CZNodePartial* camera, CZCameraDataPartial* cameraData);
int __fastcall gwCameraSetVariantTagOverride(CZNodePartial* camera, zTag4Partial* variantTag);
int __fastcall RenderScene(CZNodePartial* camera, int updateFxPass3Local);
int __fastcall BuildWorldTransform(CZNodePartial* camera, CZCameraDataPartial* data, zVec3* posOffset);
int __fastcall UpdateImpl(CZNodePartial* camera, zVec3* posOffset);
int __fastcall gwCameraUpdate(CZNodePartial* camera);
void __cdecl SyncViewContextPositions();
int __fastcall CZCameraDeleteNode(CZNodePartial* node);
int __fastcall CZCameraRenderTraverse(CZNodePartial* node, int siblingCountHint);

int __fastcall ClearPickupFlagsRecursive(CZNodePartial* node);
int __fastcall SetPickupFlagsRecursive(CZNodePartial* node);
void __fastcall PropagateTransformDirtyRecursive(CZNodePartial* self);
void __fastcall MaskExtraFlagsRecursive(CZNodePartial* self, int mask);
void __fastcall PropagateExtraFlagsRecursive(CZNodePartial* self, int flags);
void __fastcall PropagateFlagsRecursive(CZNodePartial* self, int flags);
void __fastcall SetContextRecursive(CZNodePartial* self, CZNodePartial* context, int flagMask);
void __fastcall SetDiFlagBit0Recursive(CZNodePartial* node, int enabled);
int __fastcall HasRenderableDiPredicate(CZNodePartial* node);
void __fastcall SetMaterialFlagBit9ForFlagBit0EntriesRecursive(CZNodePartial* node, int enabled);
void __fastcall InvalidateFlagBit8MaterialImagesRecursive(CZNodePartial* node);
void __fastcall LoadFlagBit8MaterialImagesAndTexturePack(CZNodePartial* node);
void __fastcall AssignInt32ToDiRecursive(CZNodePartial* node, int value);
void __fastcall AssignDamageHandlerRecursiveIfMissing(CZNodePartial* node, OptCatalogDamageHandlerPartial* handler);
void __fastcall ClearDamageHandlerRecursive(CZNodePartial* node, OptCatalogDamageHandlerPartial* handler);
int __fastcall SetDamageHitCallback(void* context, CZNodePartial* node, void* callback);
int __fastcall ClearDamageHandler(CZNodePartial* node);
int __fastcall SetDamageTimerCallback(void* context, CZNodePartial* node, void* callback);

CZTypeListLink* __cdecl AllocLink();
void __fastcall FreeLink(CZTypeListLink* link);
void __cdecl FreeAll();
int __fastcall ProcessPendingRemovals(int bucket);
int __fastcall CountNodes(int bucket);
void __fastcall PrintBucket(int bucket);
CZTypeListLink* __fastcall GetBucketHead(int bucket);
int __fastcall MarkPendingRemoval(int bucket, CZNodePartial* node);
int __fastcall InsertChildNodes(int bucket, CZNodePartial* node);
void __cdecl UpdateAllBuckets();
void __fastcall UpdateBucket(CZTypeListLink* bucket);
int __cdecl UpdateQueuedTrees();
int __fastcall UpdateSequences();
int __fastcall UpdateAnimations();
int __fastcall CZTypeListInsert(int bucket, CZNodePartial* node);

int __fastcall gwNodeBuildNodeToAncestorMatrix(CZNodePartial* node, int matMode);
int __fastcall GetWorldPosition(CZNodePartial* node, zVec3* outPosition);
int __fastcall TransformPoint(CZNodePartial* node, zVec3* point);
int __fastcall GetWorldPosAndOrientation(CZNodePartial* node, zVec3* inOutPosition, zVec3* outOrientation);
int __fastcall UpdateSubtree(CZNodePartial* node);
void __fastcall UpdateTree(CZNodePartial* node);

void __cdecl ProcessPendingFrees();
int __fastcall CZNodeListInsert(CZNodePartial* node);

int __fastcall DeleteNodeFromLists(CZNodePartial* node);
int __fastcall _gwListDeleteANode(CZNodePartial* node);
int __fastcall DeleteAllOfType(int bucket);
int __cdecl RenderActiveCameras();
CZNodePartial* __fastcall IterateBucketFiltered(const char* filterText, int bucket, CZNodePredicate predicate);

void __fastcall SetNodeArraySize(int size);
int __cdecl IsInitialized();
int __cdecl Init();
int __cdecl ResetCurrentZbdPath();
int __cdecl ShutdownCore();
int __cdecl Shutdown();
int __fastcall ProcessDeferredWork();
int __fastcall NodePtrToValidatedIndex(CZNodePartial* node);
CZNodePartial* __fastcall FindByTypeAndName(int bucket, const char* name);
int __fastcall FindNextByTypePrefixPredicate(CZNodePartial* node);
CZNodePartial* __fastcall FindNextByTypePrefix(const char* prefixText, int bucket);
int __fastcall AnyNodeMatchesPredicateRecursive(CZNodePartial* root, CZNodePredicate predicate);
int __fastcall RemoveChildChecked(CZNodePartial* parent, CZNodePartial* child);

CZNodePartial* __fastcall gwNodeNew();
int __fastcall DeleteNodeByType(CZNodePartial* node);
int __fastcall gwNodeUpdate(CZNodePartial* node);
int __cdecl gwNodeUpdateAll();
int __fastcall gwNodeUpdateDisplayInstance(CZNodePartial* node);
int __fastcall gwNodeGetBBox(CZNodePartial* node, zBBox3f* outBBox);
int __fastcall gwNodeGetWorldBBoxCorners(CZNodePartial* node, zBBoxCorners* outCorners);
int __fastcall gwNodeGetViewBBoxCorners(CZNodePartial* node, zBBoxCorners* outCorners);
int __fastcall gwNodeComputeChildBBox(CZNodePartial* node);
int __fastcall gwNodeRecalcBBox(CZNodePartial* node);
int __fastcall gwNodeSetActive(CZNodePartial* node, int active);
int __fastcall gwNodeSetFlag16(CZNodePartial* node, int value);
int __fastcall gwNodeSetFlag17(CZNodePartial* node, int value);
int __fastcall gwNodeSetDisplayInstance(CZNodePartial* node, zDiPartial* displayInstance);
int __fastcall gwNodeSetName(CZNodePartial* node, const char* name);
char* __fastcall gwNodeGetName(CZNodePartial* node);
int __fastcall gwNodeGetUserData(CZNodePartial* node, unsigned int* outData);
int __fastcall gwNodeSetActionCallback(CZNodePartial* node, void* actionCallback);
int __fastcall gwNodeSetActionCallbackTail(CZNodePartial* node, void* actionCallback);
int __fastcall gwNodeSetPriority(CZNodePartial* node, int priority);
int __fastcall gwNodeSetCellPickable(CZNodePartial* node, int value);
int __fastcall gwNodeGetCellPickable(CZNodePartial* node, int* outValue);
int __fastcall gwNodeGetNodeType(CZNodePartial* node, int* outValue);
int __fastcall gwNodeSetRaycastable(CZNodePartial* node, int value);
int __fastcall gwNodeGetRaycastable(CZNodePartial* node, int* outValue);
int __fastcall gwNodeSetPickable(CZNodePartial* node, int value);
int __fastcall gwNodeGetPickable(CZNodePartial* node, int* outValue);
int __fastcall gwNodeSetHasHitCallback(CZNodePartial* node, int value);
int __fastcall gwNodeSetBypassFarClip(CZNodePartial* node, int value);
int __fastcall gwNodeSetNodeType(CZNodePartial* node, int nodeType);
int __fastcall gwNodeClearVariantGate(CZNodePartial* node, int value);
int __fastcall gwNodeSetVertexAlphaOverride(CZNodePartial* node, int value);
CZNodePartial* __fastcall gwNodeGetRoot(CZNodePartial* node);
CZNodePartial* __fastcall gwNodeGetWorldChild(CZNodePartial* node);
int __fastcall gwNodeFindNextByNamePredicate(CZNodePartial* node);
CZNodePartial* __fastcall gwNodeFindNextByName(const char* name, int bucket);
CZNodePartial* __fastcall FindSubNodeByName(CZNodePartial* root, const char* name);
CZNodePartial* __fastcall FindNodeRecursiveByName(CZNodePartial* root, const char* name);
int __fastcall SetSingleParentFlagRecursive(CZNodePartial* node, int setFlag);
int __fastcall AddChildValidated(CZNodePartial* parent, CZNodePartial* child);
int __fastcall RemoveChildValidated(CZNodePartial* parent, CZNodePartial* child);
int __fastcall AddChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall AddChildGeneric(CZNodePartial* parent, CZNodePartial* child);
int __fastcall RemoveChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall RemoveChildGeneric(CZNodePartial* parent, CZNodePartial* child);
int __fastcall FreeNodeToFreeList(CZNodePartial* node);
int __fastcall TryFreeNode(CZNodePartial* node);
int __fastcall gwNodeRenderDispatch(CZNodePartial* node, int siblingCountHint);

int __fastcall CZSoundRenderTraverse(CZNodePartial* node, int siblingCountHint);

int __fastcall CZAnimateRenderTraverse(CZNodePartial* node, int siblingCountHint);

int __fastcall CZSequenceRenderTraverse(CZNodePartial* node, int siblingCountHint);

int __fastcall CZSwitchDeleteNode(CZNodePartial* node);
int __fastcall CZSwitchRenderTraverse(CZNodePartial* node, int siblingCountHint);

int __fastcall DestroyNodeRecursive(CZNodePartial* node);

int __fastcall CopyNodeDisplayInstance(CZNodePartial* source, CZNodePartial* dest);
int __fastcall CopyNodeBaseData(CZNodePartial* source, CZNodePartial* dest);
CZNodePartial* __fastcall CopyLightNode(CZNodePartial* source);
CZNodePartial* __fastcall CopySoundNode(CZNodePartial* source);
CZNodePartial* __fastcall CopyCameraNode(CZNodePartial* source);
CZNodePartial* __fastcall CopyObject3DNode(CZNodePartial* source);
CZNodePartial* __fastcall CopyAnimateNode(CZNodePartial* source);
CZNodePartial* __fastcall CopyLodNode(CZNodePartial* source);
CZNodePartial* __fastcall CopySequenceNode(CZNodePartial* source);
CZNodePartial* __fastcall CopySwitchNode(CZNodePartial* source);
CZNodePartial* __fastcall CopyNodeDispatch(CZNodePartial* source);
CZNodePartial* __fastcall CopyNodeWithCloneOptions(CZNodePartial* source, int cloneDiMode, int diArg0);
CZNodePartial* __fastcall CopyNode(CZNodePartial* source, int cloneDiMode, int diArg0, int diArg1);

CZNodePartial* __cdecl gwSoundNew();
int __fastcall SetSampleSetByName(CZNodePartial* node, const char* name);
int __fastcall gwSoundSetActive(CZNodePartial* node, int active);
int __fastcall gwSoundSetPosition(CZNodePartial* node, float x, float y, float z);
int __fastcall gwSoundGetPosition(CZNodePartial* node, float* outX, float* outY, float* outZ);
int __fastcall UpdatePlayback(CZNodePartial* node);
int __fastcall CZSoundDeleteNode(CZNodePartial* node);
int __fastcall CZSoundRemoveChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall CZSoundComputeWorldTransform(CZNodePartial* node, CZSoundDataPartial* soundData);

short __fastcall AdvanceTime(CZAnimateRuntimePartial* runtime, float deltaTime);
short __fastcall SampleTransform(CZAnimateRuntimePartial* runtime);
int __fastcall UpdateNode(CZNodePartial* node);
int __fastcall CZAnimateAddChild(CZNodePartial* parent, CZNodePartial* child);
int __fastcall CZAnimateDeleteNode(CZNodePartial* node);
int __fastcall CZAnimateRemoveChild(CZNodePartial* parent, CZNodePartial* child);

CZNodePartial* __cdecl gwSequenceNew();
int __fastcall gwSequenceAddChild(CZNodePartial* parent, CZNodePartial* child, int insertIndex, float delay);
int __fastcall SetActive(CZNodePartial* node, int active);
int __fastcall SetRepeat(CZNodePartial* node, int repeat);
int __fastcall SetLoop(CZNodePartial* node, int loop);
int __fastcall SetPause(CZNodePartial* node, int paused);
int __fastcall Update(CZNodePartial* node);
int __fastcall CZSequenceDeleteNode(CZNodePartial* node);
int __fastcall CZSequenceRemoveChild(CZNodePartial* parent, CZNodePartial* child);

int __fastcall InitThermalGlowPool();
int __cdecl DestroyThermalGlowPool();
CZNodePartial* __fastcall AllocFromFreeListAndAttach(zColorRgb* specularColor);
void __fastcall ReturnToFreeList(CZNodePartial* lightNode);

RECOIL_NO_GS int __fastcall WriteZBDFile(const char* filename);
RECOIL_NO_GS int __fastcall ReadZBDFile(const char* filename);
FILE* __fastcall OpenAndReadZBDHeader(const char* filename, CZZbdHeader* outHeader);

int __fastcall NodePtrToIndex(CZNodePartial* node);
CZNodePartial* __fastcall NodeIndexToPtr(int index);
int __fastcall WriteNodeRefListIndices(CZNodePartial** nodeRefList, int entryCount, void* stream);
RECOIL_NO_GS int __fastcall gClsWriteNode(CZNodePartial* node, void* stream);
int __fastcall WriteNodeTable(void* stream);
int __fastcall ReadNodeRefListIndices(CZNodePartial** nodeRefList, int entryCount, void* stream);
int __fastcall gClsReadNode(CZNodePartial* node, void* stream);
int __fastcall ReadNodeTable(int nodeCount, void* stream);
RECOIL_NO_GS int __fastcall ReloadDisplayInstancesFromCurrentPath_Local(CZNodePartial* node, int recurseChildren);
RECOIL_NO_GS int __fastcall
ReloadDisplayInstancesRecursive_Local(void* stream, CZZbdHeader* zbdHeader, CZNodePartial* node, int recurseChildren);

void __fastcall SetBreakOnFirstCandidate(int enabled);
void __fastcall SetStopAfterFirstHit(int flag);
int __fastcall FindBestPickCandidateBelowPoint(
    CZNodePartial* world,
    const zVec3* position,
    PlayerProbeSampleCandidateBuffer* outResults
);
int __fastcall BuildPickCandidateListBelowPoint(
    CZNodePartial* world,
    float x,
    float maxY,
    float z,
    PlayerProbeSampleCandidateBuffer* outResults
);
int __fastcall SnapProbePointYToBestCandidate(zVec3* point);
int __fastcall BuildPickCandidateList(CZNodePartial* node, int cullCount);
int __fastcall BuildPickCandidatesForPoints(CZNodePartial* node, int depth, int* hitFlags);
int __fastcall BuildPickCandidatesForPointsRecursive(CZNodePartial* node, int depth, int* hitFlags);
int __fastcall BuildPickCandidatesForPointsForLight(CZNodePartial* node, int depth, int* hitFlags);
int __fastcall BuildPickCandidatesForPointBatch(
    CZNodePartial* world,
    zVec3* pointArray,
    int pointCount,
    float queryMaxY,
    PlayerProbeSampleCandidateBuffer* outCandidateBuffersByPoint
);
int __fastcall BuildPickCandidatesRecursive(CZNodePartial* node, int cullCount);
int __fastcall BuildPickCandidatesForLight(CZNodePartial* node, int cullCount);
int __fastcall IsPickQueryPointOutsideViewBBoxXZ(CZNodePartial* node);
int __fastcall PickTestBBox2D(CZNodePartial* node, int* hitFlags);
int __fastcall FrustumTestAndPick(CZNodePartial* node, int* activeMask);
int __fastcall BuildPickCandidatesForSegment(CZNodePartial* self);
int __fastcall RaycastSelectClosestHitBetweenPoints(
    CZNodePartial* world,
    const zVec3* startPoint,
    const zVec3* endPoint,
    PlayerProbeSampleCandidateBuffer* rayData
);
int __fastcall RaycastFindClosest(
    CZNodePartial* world,
    float startX,
    float startY,
    float startZ,
    float endX,
    float endY,
    float endZ,
    PlayerProbeSampleCandidateBuffer* rayData
);
int __fastcall BuildPickCandidatesForSegmentChildFallback(CZNodePartial* node, int nodeCountHint);
int __fastcall BuildPickCandidatesForSegmentRecursive(CZNodePartial* node, int depth);
int __fastcall BuildPickCandidatesForSegmentForCamera(CZNodePartial* node, int depth);
int __fastcall BuildPickCandidatesForSegmentForLight(CZNodePartial* node, int depth);
int __fastcall BuildPickCandidatesForSegmentsRecursive(CZNodePartial* node, int nodeCountHint, int* activeMask);
int __fastcall BuildPickCandidatesForSegmentsForAnimate(CZNodePartial* node, int nodeCountHint, int* activeMask);
int __fastcall BuildPickCandidatesForSegmentsForLight(CZNodePartial* node, int nodeCountHint, int* activeMask);
void __fastcall BuildProbeHitBatchesForSegments(
    CZNodePartial* world,
    CZDisplayInstanceSegmentEndpoints* segmentEndpoints,
    int endpointCount,
    PlayerProbeSampleCandidateBuffer* hitBatches
);
void __fastcall BuildPickCandidatesForSegmentsInGridWindow(CZNodePartial* world, int* activeMask);
int __fastcall FilterRegionsAgainstSphere(
    CZNodePartial* world,
    zVec3* center,
    const char* nodeNamePrefix,
    float radius,
    int enableDistanceCull,
    int requireLineOfSight,
    OptCatalogRaycastHitList* outHitList
);
int __fastcall FilterRegionsTryAppendNode(CZNodePartial* node);
int __fastcall FilterPointsBBox(CZNodePartial* node, void* pointData);

/* Defined in Camera.c; zvid.h declares it for the C++ units. */
int __fastcall zVideoswRenderFrame(CZNodePartial* camera, int updateFxPass3Local);
