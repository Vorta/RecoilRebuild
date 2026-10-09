#include "zdec.h"

#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "zdi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
#include <yvals.h>
#endif

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-craterinstancetessellationfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4df5ac: g_zDEClient_CraterInstanceTessellationFailedMsg.
 * Purpose: Reports crater instancing failure when tessellation fails.
 */
char g_zDEClient_CraterInstanceTessellationFailedMsg[] = "Failed to instance crater: Tesselation Failed";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-craterinstanceclipfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4df5dc: g_zDEClient_CraterInstanceClipFailedMsg.
 * Purpose: Reports crater instancing failure when feature clipping fails.
 */
char g_zDEClient_CraterInstanceClipFailedMsg[] = "Failed to instance crater: Clip Failed";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-sourcefile-zdeccratercpp
 * @recoil-artifact defines .data recoil:data:0x4df604: g_zDEClient_SourceFile_ZdecCraterCpp.
 * Purpose: Provides the original source path for crater feature diagnostics.
 */
char g_zDEClient_SourceFile_ZdecCraterCpp[] = "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_crater.cpp";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-craterinstancebuildfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4df634: g_zDEClient_CraterInstanceBuildFailedMsg.
 * Purpose: Reports crater instancing failure when display construction fails.
 */
char g_zDEClient_CraterInstanceBuildFailedMsg[] = "Failed to instance crater: Build Failed";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-craternamefmt
 * @recoil-artifact defines .data recoil:data:0x4df65c: g_zDEClient_CraterNameFmt.
 * Purpose: Formats saved crater feature section names.
 */
char g_zDEClient_CraterNameFmt[] = "Crater%d";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-quicksandnamefmt
 * @recoil-artifact defines .data recoil:data:0x4df668: g_zDEClient_QuickSandNameFmt.
 * Purpose: Formats saved quicksand feature section names.
 */
char g_zDEClient_QuickSandNameFmt[] = "QSand%d";

RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_CraterInstanceTessellationFailedMsg) == 0x2e);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_CraterInstanceClipFailedMsg) == 0x27);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_SourceFile_ZdecCraterCpp) == 0x2e);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_CraterInstanceBuildFailedMsg) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_CraterNameFmt) == 0x09);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandNameFmt) == 0x08);

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-featurelist
 * @recoil-artifact defines .data recoil:data:0x539df0: g_zDEClient_FeatureList.
 * @recoil-artifact emits .data recoil:data:0x539df4: First pointer component.
 * @recoil-artifact emits .data recoil:data:0x539df8: Last pointer component.
 * @recoil-artifact emits .data recoil:data:0x539dfc: End pointer component.
 * @recoil-artifact emits .text recoil:function:0x457650: VC5 vector static initialization contribution.
 * @recoil-artifact emits .text recoil:function:0x4576a0: VC5 vector cleanup-registration contribution.
 * Purpose: Owns the feature-entry snapshots and their VC5 vector storage.
 */
std::vector<zDEClient_FeatureEntry> g_zDEClient_FeatureList;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-featuremaptree
 * @recoil-artifact defines .data recoil:data:0x539e00: g_zDEClient_FeatureMapTree.
 * @recoil-artifact emits .text recoil:function:0x457660: VC5 set static initialization contribution.
 * @recoil-artifact emits .text recoil:function:0x4576b0: VC5 set cleanup contribution.
 * Purpose: Owns the set index from feature display nodes to their
 * generated display-instance pairs.
 */
std::set<zGeometry_ClipPatchNodeView*> g_zDEClient_FeatureMapTree;

namespace zDEClient_Crater {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-destroyfeature
 * @recoil-artifact defines .text recoil:function:0x456ad0: zDEClient_Crater::DestroyFeature
 * @recoil-match byte
 *
 * Purpose: release a crater feature instance, including its generated point
 * buffer and clip-patch output.
 */
void __fastcall DestroyFeature(zDEClient_CraterFeature* featureInstance)
{
    if (featureInstance == 0) {
        return;
    }

    if (featureInstance->points != 0) {
        free(featureInstance->points);
    }

    if (featureInstance->clipPatchOutput != 0) {
        zGeometry_ClipPatchOutput::Destroy(featureInstance->clipPatchOutput);
    }

    free(featureInstance);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-initeventtemplatedefaults
 * @recoil-artifact defines .text recoil:function:0x456b00: zDEClient_Crater::InitEventTemplateDefaults
 * @recoil-match byte
 *
 * Purpose: copy the configured crater event template defaults into a caller
 * supplied event template and return zero status.
 * Retail explicitly clears EAX before the copy; both indexed callers
 * currently discard the result.
 */
int __fastcall InitEventTemplateDefaults(zDEClient_CraterEventTemplate* eventTemplate)
{
    *eventTemplate = g_zDEClient_CraterEventTemplateDefaults;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-instanceevent
 * @recoil-artifact defines .text recoil:function:0x456b20: zDEClient_Crater::InstanceEvent
 * @recoil-match byte
 *
 * Purpose: instance and submit crater geometry for an event template, restore
 * vertex merge state, and optionally start the crater effect animation.
 */
int __fastcall InstanceEvent(zDEClient_CraterEventTemplate* eventTemplate, int playEffectAnim)
{
    const float vertexMergeEpsilon = zModel_Const::GetVertexMergeEpsilon();
    zModel_Const::SetVertexMergeEpsilon(0.00499999989f);

    zDEClient_CraterFeature* const featureInstance = InitFeatureFromEventTemplate(eventTemplate);
    if (featureInstance == 0) {
        zError::ReportOld(0x100, g_zDEClient_SourceFile_ZdecCraterCpp, 0x8b, g_zDEClient_CraterInstanceBuildFailedMsg);
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    if (Build(featureInstance) == 0) {
        DestroyFeature(featureInstance);
        zError::ReportOld(0x100, g_zDEClient_SourceFile_ZdecCraterCpp, 0xc3, g_zDEClient_CraterInstanceClipFailedMsg);
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    if (CreateFeature(featureInstance) != 0) {
        DestroyFeature(featureInstance);
        zError::ReportOld(
            0x100,
            g_zDEClient_SourceFile_ZdecCraterCpp,
            0xd2,
            g_zDEClient_CraterInstanceTessellationFailedMsg
        );
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    zDEClient::AppendFeatureEntry(1, eventTemplate);
    zDEClient::SubmitFeatureGeometry(featureInstance->clipPatchOutput);
    zGeometry_ClipPatchOutput::ApplyNodeDiPairs(featureInstance->clipPatchOutput);
    zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);

    if (playEffectAnim != 0 && featureInstance->displaySourceEntry->effectAnimEntry != 0) {
        zEffectAnim::SetTransformRotAndVelocityThunk(
            featureInstance->displaySourceEntry->effectAnimEntry,
            0,
            featureInstance->eventTemplate.center.x,
            featureInstance->eventTemplate.center.y,
            featureInstance->eventTemplate.center.z,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f
        );
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-instanceeventmayberelay
 * @recoil-artifact defines .text recoil:function:0x456c50: zDEClient_Crater::InstanceEventMaybeRelay
 * @recoil-match byte
 *
 * Purpose: let the registered crater relay callback veto remote crater
 * instancing before creating the crater locally.
 */
int __fastcall InstanceEventMaybeRelay(zDEClient_CraterEventTemplate* eventTemplate)
{
    if (g_zDEClientCraterNetRelayCallback != 0 && g_zDEClientCraterNetRelayCallback(eventTemplate) == 0) {
        return -1;
    }

    return InstanceEvent(eventTemplate, 1);
}
} // namespace zDEClient_Crater

/*
 * Grow a [minValue, maxValue] range to include value. Retail compares and copies the
 * parenthesized arguments (fld/fld/fcompp compares, fld/fstp copies).
 */
#define ZDEC_EXPAND_BOUNDS(minValue, maxValue, value)                                                                  \
    if ((value) < (minValue)) {                                                                                        \
        (minValue) = (value);                                                                                          \
    }                                                                                                                  \
    if ((value) > (maxValue)) {                                                                                        \
        (maxValue) = (value);                                                                                          \
    }

namespace zDEClient_Crater {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-initfeaturefromeventtemplate
 * @recoil-artifact defines .text recoil:function:0x456c80: zDEClient_Crater::InitFeatureFromEventTemplate
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-match byte
 *
 * Purpose: create a crater feature from an event template, fit it to the
 * owning feature grid cell, and generate its circular point bounds.
 */
zDEClient_CraterFeature* __fastcall InitFeatureFromEventTemplate(zDEClient_CraterEventTemplate* eventTemplate)
{
    int i;
    int col;
    int row;
    CZWorldDataPartial* worldData;
    float cellSizeZ;
    float angle;
    zVec3* currentPoint;
    float angleStep;
    // Cell-local X/Z of the center: retail keeps the pair in adjacent dwords below the scalar homes.
    zVec2 localCenter;
    float cellSizeX;
    CZNodePartial* world;
    zDEClient_FeatureGridCell* featureGridCell;
    zDEClient_CraterFeature* featureInstance;
    zVec3* points;

    featureInstance = CreateFeatureStructFromEventTemplate(eventTemplate);
    currentPoint = featureInstance->points;

    world = zDEClient::GetCameraNode();
    worldData = (CZWorldDataPartial*)(world->classData);
    if (worldData == 0) {
        return 0;
    }

    CZWorld::WorldToGridCoordsClamped(world, eventTemplate->center.x, eventTemplate->center.z, &col, &row);

    featureGridCell = zDEClient::GetFeatureGridCell(col, row);
    featureInstance->featureGridCell = featureGridCell;
    if (featureGridCell == 0) {
        DestroyFeature(featureInstance);
        return 0;
    }

    if (featureGridCell->featureCount >= worldData->partitionMaxDecFeatureCount) {
        DestroyFeature(featureInstance);
        return 0;
    }

    localCenter.x = featureInstance->eventTemplate.center.x - featureGridCell->originX;
    localCenter.y = featureInstance->eventTemplate.center.z - featureGridCell->originZ;
    cellSizeX = worldData->areaCellSizeX;
    cellSizeZ = worldData->areaCellSizeZ;

    if (localCenter.x + featureInstance->eventTemplate.radius > cellSizeX) {
        featureInstance->eventTemplate.center.x
            -= (localCenter.x + featureInstance->eventTemplate.radius - cellSizeX) + 1.0f;
    } else if (localCenter.x - featureInstance->eventTemplate.radius < 0.0f) {
        featureInstance->eventTemplate.center.x += (featureInstance->eventTemplate.radius - localCenter.x) + 1.0f;
    }

    if (localCenter.y - featureInstance->eventTemplate.radius < cellSizeZ) {
        featureInstance->eventTemplate.center.z
            += (cellSizeZ - (localCenter.y - featureInstance->eventTemplate.radius)) + 1.0f;
    } else if (localCenter.y + featureInstance->eventTemplate.radius > 0.0) {
        featureInstance->eventTemplate.center.z -= localCenter.y + featureInstance->eventTemplate.radius + 1.0f;
    }

    angleStep = (float)(6.2831853071800001 / eventTemplate->pointCount);
    angle = 0.0f;
    // One counter serves all three loops; retail keeps it in a single home.
    for (i = 0; i < eventTemplate->pointCount; ++i) {
        zMath::SinCos(angle, &currentPoint->x, &currentPoint->z);

        currentPoint->x *= featureInstance->eventTemplate.radius;
        currentPoint->z *= featureInstance->eventTemplate.radius;
        currentPoint->x += featureInstance->eventTemplate.center.x;
        currentPoint->y = featureInstance->eventTemplate.center.y;
        currentPoint->z += featureInstance->eventTemplate.center.z;

        ++currentPoint;
        angle += angleStep;
    }

    points = featureInstance->points;
    featureInstance->boundsMinX = points[0].x;
    featureInstance->boundsMaxX = points[0].x;
    featureInstance->boundsMinZ = points[0].z;
    featureInstance->boundsMaxZ = points[0].z;

    for (i = 1; i < eventTemplate->pointCount; ++i) {
        ZDEC_EXPAND_BOUNDS(featureInstance->boundsMinX, featureInstance->boundsMaxX, points[i].x);
        ZDEC_EXPAND_BOUNDS(featureInstance->boundsMinZ, featureInstance->boundsMaxZ, points[i].z);
    }

    if (featureInstance->featureGridCell->featureCount > 0) {
        // Retail hoists the node count into a loop temporary and strength-reduces nodes[i].
        for (i = 0; i < featureInstance->featureGridCell->nodeCount; ++i) {
            zGeometry_ClipPatchNodeView* node = featureInstance->featureGridCell->nodes[i];
            if (strcmp(node->name, g_zDEClient_FeatureNodeName) == 0) {
                zDEClient_FeatureContextOverlapView* context
                    = (zDEClient_FeatureContextOverlapView*)(node->callbackContext);
                if (context != 0) {
                    const int featureType = context->featureType;
                    if (featureType == 1) {
                        if (context->bounds_38 + 5.0f > featureInstance->boundsMinX
                            && context->bounds_30 - 5.0f < featureInstance->boundsMaxX
                            && context->bounds_3c + 5.0f > featureInstance->boundsMinZ
                            && context->bounds_34 - 5.0f < featureInstance->boundsMaxZ) {
                            DestroyFeature(featureInstance);
                            return 0;
                        }
                    } else if (featureType == 3) {
                        if (context->bounds_3c + 5.0f > featureInstance->boundsMinX
                            && context->bounds_34 - 5.0f < featureInstance->boundsMaxX
                            && context->bounds_40 + 5.0f > featureInstance->boundsMinZ
                            && context->bounds_38 - 5.0f < featureInstance->boundsMaxZ) {
                            DestroyFeature(featureInstance);
                            return 0;
                        }
                    }
                }
            }
        }
    }

    return featureInstance;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-createfeaturestructfromeventtemplate
 * @recoil-artifact defines .text recoil:function:0x457040: zDEClient_Crater::CreateFeatureStructFromEventTemplate
 * @recoil-match byte
 *
 * Purpose: allocate and initialize the crater feature record copied from an
 * event template, including point storage, clip output, and display material
 * source selection.
 */
zDEClient_CraterFeature* __fastcall CreateFeatureStructFromEventTemplate(zDEClient_CraterEventTemplate* eventTemplate)
{
    zDEClient_CraterFeature* result = (zDEClient_CraterFeature*)(malloc(sizeof(zDEClient_CraterFeature)));
    memset(result, 0, sizeof(zDEClient_CraterFeature));

    result->featureType = 1;
    memcpy(&result->eventTemplate, eventTemplate, sizeof(result->eventTemplate));
    result->points = (zVec3*)(malloc((size_t)(result->eventTemplate.pointCount) * sizeof(zVec3)));
    result->clipPatchOutput = zGeometry_ClipPatchOutput::Create();

    if ((result->eventTemplate.featureFlags & 0x1008) != 0) {
        zModel_MaterialPartial* const sourceMaterial
            = (zModel_MaterialPartial*)(result->eventTemplate.craterMaterialSlot);
        result->displaySourceEntry = g_zDEClient_CraterDisplaySourceList;

        for (unsigned int i = 1; i < g_zDEClient_CraterDisplaySourceCount; ++i) {
            if (g_zDEClient_CraterDisplaySourceList[i].sourceMaterial == sourceMaterial) {
                result->displaySourceEntry = &g_zDEClient_CraterDisplaySourceList[i];
                break;
            }
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-build
 * @recoil-artifact defines .text recoil:function:0x4570e0: zDEClient_Crater::Build
 * @recoil-match byte
 *
 * Purpose: clip crater geometry, adopt positive results, and normalize negative results to zero.
 */
int __fastcall Build(zDEClient_CraterFeature* featureInstance)
{
    int result = zGeometry_Model::ClipPatch(
        featureInstance->eventTemplate.pointCount,
        featureInstance->points,
        featureInstance->featureGridCell,
        featureInstance->clipPatchOutput
    );

    if (result > 0) {
        if (featureInstance->clipPatchOutput->points == 0) {
            return 0;
        }

        if (featureInstance->points != 0) {
            free(featureInstance->points);
        }

        featureInstance->points = featureInstance->clipPatchOutput->points;
        featureInstance->eventTemplate.pointCount = featureInstance->clipPatchOutput->pointCount;
    } else if (result < 0) {
        result = 0;
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-createfeature
 * @recoil-artifact defines .text recoil:function:0x457140: zDEClient_Crater::CreateFeature
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-lerp
 * @recoil-match byte
 *
 * Purpose: create crater display geometry from the clipped crater points and
 * attach the display instance to the generated feature node.
 */
int __fastcall CreateFeature(zDEClient_CraterFeature* featureInstance)
{
    zClipUV* uvPairs = 0;
    // Retail passes the node slot as an out-parameter only; it is never cleared first.
    CZNodePartial* node;
    zDiPartial* displayInstance = zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(
        featureInstance->clipPatchOutput->partitions,
        zDEClient::GetCameraNode(),
        &node
    );
    if (node != 0) {
        CZClass::gwNodeSetName(node, g_zDEClient_FeatureNodeName);
        node->callbackContext = (CZNodePartial*)(featureInstance);
    }

    if (displayInstance == 0) {
        return -1;
    }

    zVec3* const points = featureInstance->points;
    const int pointCount = featureInstance->eventTemplate.pointCount;
    float featureRadius;
    zModel_MaterialPartial* material;
    float uScale;
    float vScale;
    if ((featureInstance->eventTemplate.featureFlags & 0x1008) != 0) {
        featureRadius = featureInstance->eventTemplate.radius;
        // Retail reads the display source entry without a null test.
        material = featureInstance->displaySourceEntry->craterMaterial;
        uScale = vScale = 1.0f / (featureRadius + featureRadius);
    }

    zVec3* const center = &featureInstance->eventTemplate.center;
    center->y -= featureInstance->eventTemplate.depth;
    const int uvCenterIndex = pointCount * 3;
    if ((featureInstance->eventTemplate.featureFlags & 0x1008) != 0) {
        uvPairs = (zClipUV*)(malloc((size_t)(uvCenterIndex + 1) * sizeof(zClipUV)));
    }

    zVec3* const midPoints = (zVec3*)(malloc((size_t)(pointCount) * sizeof(zVec3)));

    zVec3 lowCenter = *center;
    lowCenter.y = center->y - featureInstance->eventTemplate.depth;

    // Supported domain requires live, initialized crater resources, successful allocations, and featureFlags == 0x100c.
    // With (featureFlags & 0x1008) == 0, retail uses unset UV scalars and a null uvPairs base (retained defect).
    uvPairs[uvCenterIndex].u = uScale * featureRadius;
    uvPairs[uvCenterIndex].v = vScale * featureRadius;

    int i;
    for (i = 0; i < pointCount; ++i) {
        float deltaY = center->y - points[i].y;
        ZMTH_VECTOR_LERP(&midPoints[i], &points[i], &lowCenter, 0.5f);
        deltaY *= 0.5f;
        midPoints[i].y = points[i].y + deltaY - featureInstance->eventTemplate.depth;

        if ((featureInstance->eventTemplate.featureFlags & 0x1008) != 0) {
            uvPairs[i].u = (points[i].x - center->x + featureRadius) * uScale;
            uvPairs[i].v = (points[i].z - center->z + featureRadius) * vScale;
            uvPairs[pointCount + i].u = (midPoints[i].x - center->x + featureRadius) * uScale;
            uvPairs[pointCount + i].v = (midPoints[i].z - center->z + featureRadius) * vScale;
        }
    }

    zVec3 polygonPoints[4];
    zClipUV polygonUvs[4];
    // The wrapped index is spelled at each use; retail counts i + 1 as its own induction variable.
    for (i = 0; i < pointCount; ++i) {
        polygonPoints[0] = points[i];
        polygonPoints[1] = points[(i + 1) % featureInstance->eventTemplate.pointCount];
        polygonPoints[2] = midPoints[(i + 1) % featureInstance->eventTemplate.pointCount];
        polygonPoints[3] = midPoints[i];

        if ((featureInstance->eventTemplate.featureFlags & 0x1008) != 0) {
            polygonUvs[0] = uvPairs[i];
            polygonUvs[1] = uvPairs[(i + 1) % featureInstance->eventTemplate.pointCount];
            polygonUvs[2] = uvPairs[pointCount + (i + 1) % featureInstance->eventTemplate.pointCount];
            polygonUvs[3] = uvPairs[pointCount + i];
            zGeometry_Model::AddPolygonToDi(displayInstance, 4, polygonPoints, material, polygonUvs);
        } else {
            zGeometry_Model::AddPolygonToDi(displayInstance, 4, polygonPoints, 0, 0);
        }
    }

    polygonPoints[0] = *center;
    polygonPoints[0].y = lowCenter.y;
    if ((featureInstance->eventTemplate.featureFlags & 0x1008) != 0) {
        polygonUvs[0] = uvPairs[uvCenterIndex];
    }

    for (i = 0; i < pointCount; ++i) {
        polygonPoints[1] = midPoints[i];
        polygonPoints[2] = midPoints[(i + 1) % featureInstance->eventTemplate.pointCount];

        if ((featureInstance->eventTemplate.featureFlags & 0x1008) != 0) {
            polygonUvs[1] = uvPairs[pointCount + i];
            polygonUvs[2] = uvPairs[pointCount + (i + 1) % featureInstance->eventTemplate.pointCount];
            zGeometry_Model::AddPolygonToDi(displayInstance, 3, polygonPoints, material, polygonUvs);
        } else {
            zGeometry_Model::AddPolygonToDi(displayInstance, 3, polygonPoints, 0, 0);
        }
    }

    free(midPoints);
    if ((featureInstance->eventTemplate.featureFlags & 0x1008) != 0) {
        free(uvPairs);
    }

    CZClass::gwNodeSetDisplayInstance(node, displayInstance);
    return 0;
}
} /* namespace zDEClient_Crater */
namespace zDEClient {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-submitfeaturegeometry
 * @recoil-artifact defines .text recoil:function:0x4575f0: zDEClient::SubmitFeatureGeometry
 * @recoil-match byte
 *
 * Purpose: submit each generated feature node/DI pair to the feature map tree
 * so later cleanup and serialization can locate it.
 */
void __fastcall SubmitFeatureGeometry(zGeometry_ClipPatchOutputPartial* clipPatchOutput)
{
    for (int partitionIndex = 0; partitionIndex < clipPatchOutput->partitionCount; ++partitionIndex) {
        for (int pairIndex = 0; pairIndex < clipPatchOutput->partitions[partitionIndex].nodeDiPairCount; ++pairIndex) {
            g_zDEClient_FeatureMapTree.insert(clipPatchOutput->partitions[partitionIndex].nodeDiPairs[pairIndex].node);
        }
    }
}

} // namespace zDEClient

namespace zDEClient {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-clearfeaturedisplaynodes
 * @recoil-artifact defines .text recoil:function:0x457750: zDEClient::ClearFeatureDisplayNodes.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: reload display instances and delete generated ZDEC_FEATURE nodes.
 */
void __cdecl ClearFeatureDisplayNodes()
{
    for (std::set<zGeometry_ClipPatchNodeView*>::iterator entry = g_zDEClient_FeatureMapTree.begin();
        entry != g_zDEClient_FeatureMapTree.end();
        ++entry) {
        zGeometry_ClipPatchNodeView* key = *entry;
        if (key != 0) {
            CZZbd::ReloadDisplayInstancesFromCurrentPath_Local(key, 1);

            const int gridCol = key->gridCol;
            const int gridRow = key->gridRow;
            if (gridCol >= 0 && gridRow >= 0) {
                zWorldAreaPartial* area = CZWorld::GetAreaPartitionAtGrid(key->listA[0], gridCol, gridRow);
                if (area != 0) {
                    area->displayRefreshQueued = 0;
                }
            }
        }
    }

    CZNodePartial* child;
    while ((child = CZClass::FindByTypeAndName(6, g_zDEClient_FeatureNodeName)) != 0) {
        unsigned int displayInstanceValue = 0;
        while (child->listCountA > 0) {
            CZClass::RemoveChild(child->listA[0], child);
        }

        CZClass::gwNodeGetUserData(child, &displayInstanceValue);
        if (displayInstanceValue != 0) {
            CZClass::gwNodeSetDisplayInstance(child, 0);
            zModel_DiPool::FreeIfUnreferenced((zDiPartial*)(displayInstanceValue));
        }

        CZClass::DeleteNodeByType(child);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-appendfeatureentry
 * @recoil-artifact defines .text recoil:function:0x457840: zDEClient::AppendFeatureEntry
 * @recoil-match byte
 *
 * Purpose: append a crater or quicksand event snapshot to the feature-entry
 * list, growing the VC-era vector storage when needed.
 */
int __fastcall AppendFeatureEntry(int featureType, const void* featureEventData)
{
    zDEClient_FeatureEntry featureEntry;
    featureEntry.featureType = featureType;
    switch (featureType) {
    case 1:
        featureEntry.eventData.crater = *(const zDEClient_CraterEventTemplate*)(featureEventData);
        break;
    case 3:
        featureEntry.eventData.quickSand = *(const zDEClient_QSandEventTemplate*)(featureEventData);
        break;
    default:
        return 0;
    }
    featureEntry.reloadFlag = 0;

    g_zDEClient_FeatureList.push_back(featureEntry);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-clearfeatureentriesandmaptree
 * @recoil-artifact defines .text recoil:function:0x457ae0: zDEClient::ClearFeatureEntriesAndMapTree.
 * @recoil-match byte
 *
 * Purpose: reset feature entry storage and clear all feature map-tree nodes.
 */
int __cdecl ClearFeatureEntriesAndMapTree()
{
    g_zDEClient_FeatureList.clear();
    g_zDEClient_FeatureMapTree.clear();

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-writefeaturesectionstozar
 * @recoil-artifact defines .text recoil:function:0x457b40: zDEClient::WriteFeatureSectionsToZAR.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: serialize saved crater and quicksand feature entries into ZAR
 * sections.
 */
int __fastcall WriteFeatureSectionsToZAR(zZbdSectionCallbackCtx* callbackCtx)
{
    int craterSectionIndex = 0;
    int qSandSectionIndex = 0;
    zDEClient_FeatureEntry featureEntry;
    featureEntry.reloadFlag = 1;

    int result = zUtil_ZAR::WriteSectionBlob(callbackCtx, "Dummy", &featureEntry, sizeof(featureEntry));

    for (std::vector<zDEClient_FeatureEntry>::iterator entry = g_zDEClient_FeatureList.begin();
        entry != g_zDEClient_FeatureList.end() && result != 0;
        ++entry) {
        featureEntry = *entry;
        featureEntry.reloadFlag = 0;

        char sectionName[0x40];
        switch (featureEntry.featureType) {
        case 1:
            sprintf(sectionName, g_zDEClient_CraterNameFmt, craterSectionIndex++);
            break;
        case 3:
            sprintf(sectionName, g_zDEClient_QuickSandNameFmt, qSandSectionIndex++);
            break;
        default:
            continue;
        }

        result = zUtil_ZAR::WriteSectionBlob(callbackCtx, sectionName, &featureEntry, sizeof(featureEntry));
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-applyfeatureentry
 * @recoil-artifact defines .text recoil:function:0x457c10: zDEClient::ApplyFeatureEntry.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: reload a serialized zDEClient feature entry or clear feature
 * display state for a reload marker.
 */
void __stdcall ApplyFeatureEntry(zDEClient_FeatureEntry* container, void*, void*)
{
    if (container->reloadFlag != 0) {
        ClearFeatureDisplayNodes();
        ClearFeatureEntriesAndMapTree();
        return;
    }

    switch (container->featureType) {
    case 3:
        zDEClient_QSand::InstanceEventMaybeRelay(&container->eventData.quickSand);
        break;

    case 1:
        zDEClient_Crater::InstanceEvent(&container->eventData.crater, 0);
        break;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-dispatchfeatureeventtemplates
 * @recoil-artifact defines .text recoil:function:0x457c50: zDEClient::DispatchFeatureEventTemplates.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: iterate feature-entry snapshots and dispatch crater or quicksand
 * event templates to caller-provided handlers.
 */
void __fastcall DispatchFeatureEventTemplates(
    zDEClient_CraterFeatureDispatch craterHandler,
    zDEClient_QSandFeatureDispatch qSandHandler
)
{
    for (std::vector<zDEClient_FeatureEntry>::iterator entry = g_zDEClient_FeatureList.begin();
        entry != g_zDEClient_FeatureList.end();
        ++entry) {
        zDEClient_FeatureEntry featureEntry = *entry;

        switch (featureEntry.featureType) {
        case 3:
            if (qSandHandler != 0) {
                qSandHandler(&featureEntry.eventData.quickSand);
            }
            break;

        case 1:
            if (craterHandler != 0) {
                craterHandler(&featureEntry.eventData.crater);
            }
            break;
        }
    }
}
} // namespace zDEClient
namespace zDEClient {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-setcameranode
 * @recoil-artifact defines .text recoil:function:0x458aa0: zDEClient::SetCameraNode
 * @recoil-match byte
 *
 * Purpose: record the active camera node and its class-data feature grid.
 */
void __fastcall SetCameraNode(CZNodePartial* cameraNode)
{
    if (cameraNode != 0) {
        g_zDEClient_CameraNode = cameraNode;
        g_zDEClient_CameraNodeClassData = (CZCameraDataPartial*)(cameraNode->classData);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-getfeaturegridcell
 * @recoil-artifact defines .text recoil:function:0x458ac0: zDEClient::GetFeatureGridCell
 * @recoil-match byte
 *
 * Purpose: return a feature-grid cell from the current camera node data.
 */
zDEClient_FeatureGridCell* __fastcall GetFeatureGridCell(int gridCol, int gridRow)
{
    if (g_zDEClient_CameraNodeClassData == 0) {
        return 0;
    }

    zDEClient_CameraNodeClassDataPartial* data
        = (zDEClient_CameraNodeClassDataPartial*)(g_zDEClient_CameraNodeClassData);
    return &data->featureGridRows[gridRow][gridCol];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-getcameranode
 * @recoil-artifact defines .text recoil:function:0x458ae0: zDEClient::GetCameraNode
 * @recoil-match byte
 *
 * Purpose: expose the active camera node used by terrain feature helpers.
 */
CZNodePartial* __cdecl GetCameraNode()
{
    return g_zDEClient_CameraNode;
}
} // namespace zDEClient
