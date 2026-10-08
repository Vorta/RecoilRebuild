#include "zdec.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "zdi.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zdeclient.zdec-qsand.g-zdeclient-quicksandinstancetessellationfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4df4e0: g_zDEClient_QuickSandInstanceTessellationFailedMsg.
 * Purpose: Reports quicksand instancing failure when tessellation fails.
 */
char g_zDEClient_QuickSandInstanceTessellationFailedMsg[] = "Failed to instance quick sand: Tesselation Failed";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zdeclient.zdec-qsand.g-zdeclient-quicksandinstanceclipfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4df514: g_zDEClient_QuickSandInstanceClipFailedMsg.
 * Purpose: Reports quicksand instancing failure when feature clipping fails.
 */
char g_zDEClient_QuickSandInstanceClipFailedMsg[] = "Failed to instance quick sand: Clip Failed";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zdeclient.zdec-qsand.g-zdeclient-sourcefile-zdecqsandcpp
 * @recoil-artifact defines .data recoil:data:0x4df540: g_zDEClient_SourceFile_ZdecQsandCpp.
 * Purpose: Provides the original source path for quicksand feature diagnostics.
 */
char g_zDEClient_SourceFile_ZdecQsandCpp[] = "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_qsand.cpp";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zdeclient.zdec-qsand.g-zdeclient-quicksandinstancebuildfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4df570: g_zDEClient_QuickSandInstanceBuildFailedMsg.
 * Purpose: Reports quicksand instancing failure when display construction fails.
 */
char g_zDEClient_QuickSandInstanceBuildFailedMsg[] = "Failed to instance quick sand: Build Failed";

RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandInstanceTessellationFailedMsg) == 0x32);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandInstanceClipFailedMsg) == 0x2b);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_SourceFile_ZdecQsandCpp) == 0x2d);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandInstanceBuildFailedMsg) == 0x2c);

namespace zDEClient_QSand {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zdeclient.zdec-qsand.zdeclient-qsand-destroyfeature
 * @recoil-artifact defines .text recoil:function:0x455ea0: zDEClient_QSand::DestroyFeature
 * @recoil-match byte
 *
 * Purpose: release a quicksand feature instance, including its generated point
 * buffer and clip-patch output.
 */
void __fastcall DestroyFeature(zDEClient_QSandFeature* featureInstance)
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
} /* namespace zDEClient_QSand */
namespace zDEClient {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zdeclient.zdec-qsand.zdeclient-copyqsandeventtemplatedefaults
 * @recoil-artifact defines .text recoil:function:0x455ed0: zDEClient::CopyQSandEventTemplateDefaults
 * @recoil-match byte
 *
 * Purpose: copy the configured quicksand event template defaults into a
 * caller-owned event template.
 */
int __fastcall CopyQSandEventTemplateDefaults(zDEClient_QSandEventTemplate* eventTemplate)
{
    memcpy(eventTemplate, &g_zDEClient_QuickSandEventTemplateDefaults, sizeof(zDEClient_QSandEventTemplate));
    return 0;
}
} /* namespace zDEClient */
namespace zDEClient_QSand {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zdeclient.zdec-qsand.zdeclient-qsand-instanceeventmayberelay
 * @recoil-artifact defines .text recoil:function:0x455ef0: zDEClient_QSand::InstanceEventMaybeRelay
 * @recoil-match byte
 *
 * Purpose: let the registered quicksand relay callback veto instancing before
 * building and submitting the quicksand feature locally.
 */
int __fastcall InstanceEventMaybeRelay(zDEClient_QSandEventTemplate* eventTemplate)
{
    if (g_zDEClientQSandNetRelayCallback != 0 && g_zDEClientQSandNetRelayCallback(eventTemplate) == 0) {
        return -1;
    }

    if (g_zDEClient_QuickSandEnabled == 0) {
        return -1;
    }

    const float vertexMergeEpsilon = zModel_Const::GetVertexMergeEpsilon();
    zModel_Const::SetVertexMergeEpsilon(0.00499999989f);

    zDEClient_QSandFeature* const featureInstance = InitFeatureFromEventTemplate(eventTemplate);
    if (featureInstance == 0) {
        zError::ReportOld(
            0x100,
            g_zDEClient_SourceFile_ZdecQsandCpp,
            0x81,
            g_zDEClient_QuickSandInstanceBuildFailedMsg
        );
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    if (Build(featureInstance) == 0) {
        DestroyFeature(featureInstance);
        zError::ReportOld(0x100, g_zDEClient_SourceFile_ZdecQsandCpp, 0x92, g_zDEClient_QuickSandInstanceClipFailedMsg);
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    if (CreateFeature(featureInstance) != 0) {
        DestroyFeature(featureInstance);
        zError::ReportOld(
            0x100,
            g_zDEClient_SourceFile_ZdecQsandCpp,
            0xa1,
            g_zDEClient_QuickSandInstanceTessellationFailedMsg
        );
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    zDEClient::AppendFeatureEntry(3, eventTemplate);
    zDEClient::SubmitFeatureGeometry(featureInstance->clipPatchOutput);
    zGeometry_ClipPatchOutput::ApplyNodeDiPairs(featureInstance->clipPatchOutput);

    zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
    return 0;
}
} // namespace zDEClient_QSand

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

namespace zDEClient_QSand {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-qsand-initfeaturefromeventtemplate
 * @recoil-artifact defines .text recoil:function:0x456010: zDEClient_QSand::InitFeatureFromEventTemplate.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-match byte
 *
 * Function modeled here:
 * zDEClient_QSand::InitFeatureFromEventTemplate
 *
 * Purpose: create a quicksand feature from an event template, fit it to the
 * owning feature grid cell, and generate its circular point bounds.
 */
zDEClient_QSandFeature* __fastcall InitFeatureFromEventTemplate(zDEClient_QSandEventTemplate* eventTemplate)
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
    zDEClient_QSandFeature* featureInstance;
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
 * @recoil-anchor recoil:anchor:zdeclient.zdec-qsand.z-declient-qsand-create-feature-struct-from-event-template
 * @recoil-artifact defines .text recoil:function:0x4563d0: zDEClient_QSand::CreateFeatureStructFromEventTemplate.
 * @recoil-match byte
 *
 * Function modeled here:
 * zDEClient_QSand::CreateFeatureStructFromEventTemplate
 *
 * Purpose: allocate and initialize the quicksand feature record copied from an
 * event template, including point storage, clip output, and default material
 * binding.
 */
zDEClient_QSandFeature* __fastcall CreateFeatureStructFromEventTemplate(zDEClient_QSandEventTemplate* eventTemplate)
{
    zDEClient_QSandFeature* result = (zDEClient_QSandFeature*)(malloc(sizeof(zDEClient_QSandFeature)));
    memset(result, 0, sizeof(zDEClient_QSandFeature));

    result->featureType = 3;
    memcpy(&result->eventTemplate, eventTemplate, sizeof(result->eventTemplate));
    result->points = (zVec3*)(malloc(result->eventTemplate.pointCount * sizeof(zVec3)));
    result->clipPatchOutput = zGeometry_ClipPatchOutput::Create();

    if ((result->eventTemplate.featureFlags & 0x1008) != 0 && result->eventTemplate.material == 0) {
        result->eventTemplate.material = g_zDEClient_QuickSandMaterial;
        result->eventTemplate.materialCycle = g_zDEClient_QuickSandMaterialCycle;
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zdeclient.zdec-qsand.zdeclient-qsand-build
 * @recoil-artifact defines .text recoil:function:0x456450: zDEClient_QSand::Build
 * @recoil-match byte
 *
 * Purpose: clip the quicksand polygon into the feature grid cell and adopt the
 * clipped point list.
 * Only positive results replace the point list; negative results become zero.
 */
int __fastcall Build(zDEClient_QSandFeature* featureInstance)
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zdeclient.zdec-qsand.zdeclient-qsand-createfeature
 * @recoil-artifact defines .text recoil:function:0x4564b0: zDEClient_QSand::CreateFeature
 *
 *
 * Purpose: create quicksand side and cap display geometry from the clipped
 * feature points and attach both display instances to generated feature nodes.
 */
int __fastcall CreateFeature(zDEClient_QSandFeature* featureInstance)
{
    CZNodePartial* node = 0;
    zDiPartial* displayInstance = zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(
        featureInstance->clipPatchOutput->partitions,
        zDEClient::GetCameraNode(),
        &node
    );
    if (displayInstance == 0 || node == 0) {
        if (displayInstance != 0) {
            zModel_DiPool::FreeIfUnreferenced(displayInstance);
        }

        if (node != 0) {
            CZObject3D::DeleteNode(node);
        }

        return -1;
    }

    CZClass::gwNodeSetName(node, g_zDEClient_FeatureNodeName);
    node->callbackContext = (CZNodePartial*)(featureInstance);

    zVec3* const points = featureInstance->points;
    const int pointCount = featureInstance->eventTemplate.pointCount;
    const int featureFlags = featureInstance->eventTemplate.featureFlags;
    const int uvCenterIndex = pointCount * 3;
    const bool hasMaterialUv = (featureFlags & 0x1008) != 0;

    zModel_MaterialPartial* sideMaterial = 0;
    float featureRadius = 0.0f;
    float uvScale = 0.0f;
    if (hasMaterialUv) {
        featureRadius = featureInstance->eventTemplate.radius;
        sideMaterial = featureInstance->eventTemplate.material;
        uvScale = 4.5f / (featureRadius + featureRadius);
    }

    featureInstance->eventTemplate.center.y -= featureInstance->eventTemplate.depth;

    zClipUV* uvPairs = 0;
    if (hasMaterialUv) {
        uvPairs = (zClipUV*)(malloc((size_t)(uvCenterIndex + 1) * sizeof(zClipUV)));
    }

    zVec3* const midPoints = (zVec3*)(malloc((size_t)(pointCount) * sizeof(zVec3)));

    const zVec3 center = featureInstance->eventTemplate.center;
    const float lowCenterY = featureInstance->eventTemplate.center.y - featureInstance->eventTemplate.depth;
    const float topCenterY = featureInstance->eventTemplate.center.y + featureInstance->eventTemplate.depth;

    if (hasMaterialUv) {
        uvPairs[uvCenterIndex].u = uvScale * featureRadius;
        uvPairs[uvCenterIndex].v = uvScale * featureRadius;
    }

    for (int i = 0; i < pointCount; ++i) {
        const zVec3* const point = &points[i];
        zVec3* const midPoint = &midPoints[i];

        midPoint->x = (center.x - point->x) * 0.5f + point->x;
        midPoint->y = ((center.y - point->y) * 0.5f + point->y) - featureInstance->eventTemplate.depth;
        midPoint->z = (center.z - point->z) * 0.5f + point->z;

        if (hasMaterialUv) {
            uvPairs[i].u = (point->x - center.x + featureRadius) * uvScale;
            uvPairs[i].v = (point->z - center.z + featureRadius) * uvScale;
            uvPairs[pointCount + i].u = (midPoint->x - center.x + featureRadius) * uvScale;
            uvPairs[pointCount + i].v = (midPoint->z - center.z + featureRadius) * uvScale;
        }
    }

    for (int i_837 = 0; i_837 < pointCount; ++i_837) {
        const int nextIndex = (i_837 + 1) % pointCount;
        zVec3 polygonPoints[4];
        polygonPoints[0] = points[i_837];
        polygonPoints[1] = points[nextIndex];
        polygonPoints[2] = midPoints[nextIndex];
        polygonPoints[3] = midPoints[i_837];

        zClipUV polygonUvs[4];
        zClipUV* uvList = 0;
        zModel_MaterialPartial* material = 0;
        if (hasMaterialUv) {
            polygonUvs[0] = uvPairs[i_837];
            polygonUvs[1] = uvPairs[nextIndex];
            polygonUvs[2] = uvPairs[pointCount + nextIndex];
            polygonUvs[3] = uvPairs[pointCount + i_837];
            uvList = polygonUvs;
            material = featureInstance->eventTemplate.materialCycle;
        }

        zGeometry_Model::AddPolygonToDi(displayInstance, 4, polygonPoints, material, uvList);
    }

    for (int i_860 = 0; i_860 < pointCount; ++i_860) {
        const int nextIndex = (i_860 + 1) % pointCount;
        zVec3 polygonPoints[3];
        polygonPoints[0].x = center.x;
        polygonPoints[0].y = lowCenterY;
        polygonPoints[0].z = center.z;
        polygonPoints[1] = midPoints[i_860];
        polygonPoints[2] = midPoints[nextIndex];

        zClipUV polygonUvs[3];
        zClipUV* uvList = 0;
        zModel_MaterialPartial* material = 0;
        if (hasMaterialUv) {
            polygonUvs[0] = uvPairs[uvCenterIndex];
            polygonUvs[1] = uvPairs[pointCount + i_860];
            polygonUvs[2] = uvPairs[pointCount + nextIndex];
            uvList = polygonUvs;
            material = featureInstance->eventTemplate.materialCycle;
        }

        zGeometry_Model::AddPolygonToDi(displayInstance, 3, polygonPoints, material, uvList);
    }

    CZNodePartial* capNode = 0;
    zDiPartial* const capDisplayInstance = zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(
        featureInstance->clipPatchOutput->partitions,
        zDEClient::GetCameraNode(),
        &capNode
    );
    if (capDisplayInstance == 0 || capNode == 0) {
        if (capDisplayInstance != 0) {
            zModel_DiPool::FreeIfUnreferenced(capDisplayInstance);
        }

        if (capNode != 0) {
            CZObject3D::DeleteNode(capNode);
        }

        return -1;
    }

    CZClass::gwNodeSetName(capNode, g_zDEClient_FeatureNodeName);
    capNode->callbackContext = (CZNodePartial*)(featureInstance);

    for (int i_901 = 0; i_901 < pointCount; ++i_901) {
        const int nextIndex = (i_901 + 1) % pointCount;
        zVec3 polygonPoints[3];
        polygonPoints[0].x = center.x;
        polygonPoints[0].y = topCenterY;
        polygonPoints[0].z = center.z;
        polygonPoints[1] = points[i_901];
        polygonPoints[2] = points[nextIndex];

        zClipUV polygonUvs[3];
        zClipUV* uvList = 0;
        zModel_MaterialPartial* material = 0;
        if (hasMaterialUv) {
            polygonUvs[0] = uvPairs[uvCenterIndex];
            polygonUvs[1] = uvPairs[pointCount + i_901];
            polygonUvs[2] = uvPairs[pointCount + nextIndex];
            uvList = polygonUvs;
            material = sideMaterial;
        }

        zGeometry_Model::AddPolygonToDi(capDisplayInstance, 3, polygonPoints, material, uvList);
    }

    free(midPoints);
    if (hasMaterialUv) {
        free(uvPairs);
    }

    return 0;
}
} // namespace zDEClient_QSand
