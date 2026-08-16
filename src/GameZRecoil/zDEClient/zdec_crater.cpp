#include "zdec.h"

#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
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
char g_zDEClient_CraterInstanceTessellationFailedMsg[] =
    "Failed to instance crater: Tesselation Failed";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-craterinstanceclipfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4df5dc: g_zDEClient_CraterInstanceClipFailedMsg.
 * Purpose: Reports crater instancing failure when feature clipping fails.
 */
char g_zDEClient_CraterInstanceClipFailedMsg[] =
    "Failed to instance crater: Clip Failed";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-sourcefile-zdeccratercpp
 * @recoil-artifact defines .data recoil:data:0x4df604: g_zDEClient_SourceFile_ZdecCraterCpp.
 * Purpose: Provides the original source path for crater feature diagnostics.
 */
char g_zDEClient_SourceFile_ZdecCraterCpp[] =
    "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_crater.cpp";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-g-zdeclient-craterinstancebuildfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4df634: g_zDEClient_CraterInstanceBuildFailedMsg.
 * Purpose: Reports crater instancing failure when display construction fails.
 */
char g_zDEClient_CraterInstanceBuildFailedMsg[] =
    "Failed to instance crater: Build Failed";
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
std::set<zGeometry_ClipPatchNodeView *> g_zDEClient_FeatureMapTree;

namespace zDEClient_Crater {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-destroyfeature
 * @recoil-artifact defines .text recoil:function:0x456ad0: zDEClient_Crater::DestroyFeature
 *
 * Purpose: release a crater feature instance, including its generated point
 * buffer and clip-patch output.
 */
void __fastcall DestroyFeature(
    zDEClient_CraterFeature *featureInstance
) {
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
 *
 * Purpose: copy the configured crater event template defaults into a caller
 * supplied event template.
 */
void __fastcall InitEventTemplateDefaults(
    zDEClient_CraterEventTemplate *eventTemplate
) {
    memcpy(
        eventTemplate,
        &g_zDEClient_CraterEventTemplateDefaults,
        sizeof(zDEClient_CraterEventTemplate)
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-instanceevent
 * @recoil-artifact defines .text recoil:function:0x456b20: zDEClient_Crater::InstanceEvent
 *
 * Purpose: instance and submit crater geometry for an event template, restore
 * vertex merge state, and optionally start the crater effect animation.
 */
int __fastcall InstanceEvent(
    zDEClient_CraterEventTemplate *eventTemplate,
    int playEffectAnim
) {
    const float vertexMergeEpsilon = zModel_Const::GetVertexMergeEpsilon();
    zModel_Const::SetVertexMergeEpsilon(0.00499999989f);

    zDEClient_CraterFeature *const featureInstance = InitFeatureFromEventTemplate(eventTemplate);
    if (featureInstance == 0) {
        zError::ReportOld(
            0x100,
            g_zDEClient_SourceFile_ZdecCraterCpp,
            0x8b,
            g_zDEClient_CraterInstanceBuildFailedMsg
        );
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    if (Build(featureInstance) == 0) {
        DestroyFeature(featureInstance);
        zError::ReportOld(
            0x100,
            g_zDEClient_SourceFile_ZdecCraterCpp,
            0xc3,
            g_zDEClient_CraterInstanceClipFailedMsg
        );
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

    zDEClient::AppendFeatureEntry(
        1,
        eventTemplate
    );
    zDEClient::SubmitFeatureGeometry(featureInstance->clipPatchOutput);
    zGeometry_ClipPatchOutput::ApplyNodeDiPairs(featureInstance->clipPatchOutput);
    zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);

    if (playEffectAnim != 0 && featureInstance->displaySourceEntry != 0 &&
        featureInstance->displaySourceEntry->effectAnimEntry != 0) {
        zEffectAnim::SetTransformRotAndVelocity_Thunk(
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
 *
 * Purpose: let the registered crater relay callback veto remote crater
 * instancing before creating the crater locally.
 */
int __fastcall InstanceEventMaybeRelay(
    zDEClient_CraterEventTemplate *eventTemplate
) {
    if (g_zDEClientCraterNetRelayCallback != 0 &&
        g_zDEClientCraterNetRelayCallback(eventTemplate) == 0) {
        return -1;
    }

    return InstanceEvent(
        eventTemplate,
        1
    );
}
} // namespace zDEClient_Crater

namespace zDEClient_Crater {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-initfeaturefromeventtemplate
 * @recoil-artifact defines .text recoil:function:0x456c80: zDEClient_Crater::InitFeatureFromEventTemplate
 *
 * Purpose: create a crater feature from an event template, fit it to the
 * owning feature grid cell, and generate its circular point bounds.
 */
zDEClient_CraterFeature *__fastcall InitFeatureFromEventTemplate(
    zDEClient_CraterEventTemplate *eventTemplate
) {
    zDEClient_CraterFeature *featureInstance = CreateFeatureStructFromEventTemplate(eventTemplate);
    zVec3 *currentPoint = featureInstance->points;

    zClass_NodePartial *world = zDEClient::GetCameraNode();
    zClass_WorldDataPartial *worldData = (zClass_WorldDataPartial *)(world->classData);
    if (worldData == 0) {
        return 0;
    }

    int gridCol;
    int gridRow;
    zClass_World::WorldToGridCoordsClamped(
        world,
        &gridCol,
        eventTemplate->center.x,
        eventTemplate->center.z,
        &gridRow
    );

    zDEClient_FeatureGridCell *featureGridCell = zDEClient::GetFeatureGridCell(
        gridCol,
        gridRow
    );
    featureInstance->featureGridCell = featureGridCell;
    if (featureGridCell == 0) {
        DestroyFeature(featureInstance);
        return 0;
    }

    if (featureGridCell->featureCount >= worldData->partitionMaxDecFeatureCount) {
        DestroyFeature(featureInstance);
        return 0;
    }

    const float localX = featureInstance->eventTemplate.center.x - featureGridCell->originX;
    const float localZ = featureInstance->eventTemplate.center.z - featureGridCell->originZ;
    const float radius = featureInstance->eventTemplate.radius;

    const float localXPlusRadius = localX + radius;
    if (localXPlusRadius > worldData->areaCellSizeX) {
        featureInstance->eventTemplate.center.x -=
            (localXPlusRadius - worldData->areaCellSizeX) + 1.0f;
    } else if (localX - radius < 0.0f) {
        featureInstance->eventTemplate.center.x += (radius - localX) + 1.0f;
    }

    const float localZMinusRadius = localZ - radius;
    if (localZMinusRadius < worldData->areaCellSizeZ) {
        featureInstance->eventTemplate.center.z +=
            (worldData->areaCellSizeZ - localZMinusRadius) + 1.0f;
    } else {
        const float localZPlusRadius = localZ + radius;
        if (localZPlusRadius > 0.0f) {
            featureInstance->eventTemplate.center.z -= localZPlusRadius + 1.0f;
        }
    }

    const int pointCount = eventTemplate->pointCount;
    float angle = 0.0f;
    const float angleStep = (float)(6.2831853071800001 / pointCount);
    for (int i = 0; i < pointCount; ++i) {
        currentPoint->x = (float)(sin((double)(angle)));
        currentPoint->z = (float)(cos((double)(angle)));

        currentPoint->x *= radius;
        currentPoint->z *= radius;
        currentPoint->x += featureInstance->eventTemplate.center.x;
        currentPoint->y = featureInstance->eventTemplate.center.y;
        currentPoint->z += featureInstance->eventTemplate.center.z;

        ++currentPoint;
        angle += angleStep;
    }

    zVec3 *const points = featureInstance->points;
    featureInstance->boundsMinX = points[0].x;
    featureInstance->boundsMaxX = points[0].x;
    featureInstance->boundsMinZ = points[0].z;
    featureInstance->boundsMaxZ = points[0].z;

    for (int i_282 = 1; i_282 < eventTemplate->pointCount; ++i_282) {
        zVec3 *const point = &points[i_282];
        if (featureInstance->boundsMinX > point->x) {
            featureInstance->boundsMinX = point->x;
        }

        if (featureInstance->boundsMaxX < point->x) {
            featureInstance->boundsMaxX = point->x;
        }

        if (featureInstance->boundsMinZ > point->z) {
            featureInstance->boundsMinZ = point->z;
        }

        if (featureInstance->boundsMaxZ < point->z) {
            featureInstance->boundsMaxZ = point->z;
        }
    }

    if (featureGridCell->featureCount > 0) {
        const int nodeCount = featureGridCell->nodeCount;
        if (nodeCount > 0) {
            zGeometry_ClipPatchNodeView **nodeCursor = featureGridCell->nodes;
            for (int i = 0; i < nodeCount; ++i) {
                zGeometry_ClipPatchNodeView *node = *nodeCursor;
                if (strcmp(
                    node->name,
                    g_zDEClient_FeatureNodeName
                ) == 0) {
                    zDEClient_FeatureContextOverlapView *context =
                        (zDEClient_FeatureContextOverlapView *)(node->callbackContext);
                    if (context != 0) {
                        const int featureType = context->featureType;
                        if (featureType == 1) {
                            if (context->bounds_38 + 5.0f > featureInstance->boundsMinX &&
                                context->bounds_30 - 5.0f < featureInstance->boundsMaxX &&
                                context->bounds_3c + 5.0f > featureInstance->boundsMinZ &&
                                context->bounds_34 - 5.0f < featureInstance->boundsMaxZ) {
                                DestroyFeature(featureInstance);
                                return 0;
                            }
                        } else if (featureType == 3) {
                            if (context->bounds_3c + 5.0f > featureInstance->boundsMinX &&
                                context->bounds_34 - 5.0f < featureInstance->boundsMaxX &&
                                context->bounds_40 + 5.0f > featureInstance->boundsMinZ &&
                                context->bounds_38 - 5.0f < featureInstance->boundsMaxZ) {
                                DestroyFeature(featureInstance);
                                return 0;
                            }
                        }
                    }
                }

                ++nodeCursor;
            }
        }
    }

    return featureInstance;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-createfeaturestructfromeventtemplate
 * @recoil-artifact defines .text recoil:function:0x457040: zDEClient_Crater::CreateFeatureStructFromEventTemplate
 *
 * Purpose: allocate and initialize the crater feature record copied from an
 * event template, including point storage, clip output, and display material
 * source selection.
 */
zDEClient_CraterFeature *__fastcall CreateFeatureStructFromEventTemplate(
    zDEClient_CraterEventTemplate *eventTemplate
) {
    zDEClient_CraterFeature *result =
        (zDEClient_CraterFeature *)(malloc(sizeof(zDEClient_CraterFeature)));
    memset(
        result,
        0,
        sizeof(zDEClient_CraterFeature)
    );

    result->featureType = 1;
    memcpy(
        &result->eventTemplate,
        eventTemplate,
        sizeof(result->eventTemplate)
    );
    result->points = (zVec3 *)(malloc((size_t)(result->eventTemplate.pointCount) * sizeof(zVec3)));
    result->clipPatchOutput = zGeometry_ClipPatchOutput::Create();

    if ((result->eventTemplate.featureFlags & 0x1008) != 0) {
        zModel_MaterialPartial *const sourceMaterial =
            (zModel_MaterialPartial *)(result->eventTemplate.craterMaterialSlot);
        result->displaySourceEntry = g_zDEClient_CraterDisplaySourceList;

        for (int i = 1; i < g_zDEClient_CraterDisplaySourceCount; ++i) {
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
 * Purpose: Clip the crater polygon into the feature grid cell and adopt the clipped point list.
 */
int __fastcall Build(
    zDEClient_CraterFeature *featureInstance
) {
    int result = zGeometry_Model::ClipPatch(
        featureInstance->eventTemplate.pointCount,
        featureInstance->points,
        featureInstance->featureGridCell,
        featureInstance->clipPatchOutput
    );

    if (result <= 0) {
        if (result < 0) {
            result = 0;
        }

        return result;
    }

    if (featureInstance->clipPatchOutput->points == 0) {
        return 0;
    }

    if (featureInstance->points != 0) {
        free(featureInstance->points);
    }

    featureInstance->points = featureInstance->clipPatchOutput->points;
    featureInstance->eventTemplate.pointCount = featureInstance->clipPatchOutput->pointCount;
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-createfeature
 * @recoil-artifact defines .text recoil:function:0x457140: zDEClient_Crater::CreateFeature
 *
 * Purpose: create crater display geometry from the clipped crater points and
 * attach the display instance to the generated feature node.
 */
int __fastcall CreateFeature(
    zDEClient_CraterFeature *featureInstance
) {
    zClass_NodePartial *node = 0;
    zDiPartial *displayInstance = zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(
        featureInstance->clipPatchOutput->partitions,
        zDEClient::GetCameraNode(),
        &node
    );
    if (node != 0) {
        zClass_Class::gwNodeSetName(
            node,
            g_zDEClient_FeatureNodeName
        );
        node->callbackContext = (zClass_NodePartial *)(featureInstance);
    }

    if (displayInstance == 0) {
        return -1;
    }

    zVec3 *const points = featureInstance->points;
    const int pointCount = featureInstance->eventTemplate.pointCount;
    const int featureFlags = featureInstance->eventTemplate.featureFlags;
    const int uvCenterIndex = pointCount * 3;
    const bool hasMaterialUv = (featureFlags & 0x1008) != 0;

    zModel_MaterialPartial *material = 0;
    float featureRadius = 0.0f;
    float uvScale = 0.0f;
    if (hasMaterialUv) {
        featureRadius = featureInstance->eventTemplate.radius;
        if (featureInstance->displaySourceEntry != 0) {
            material = featureInstance->displaySourceEntry->craterMaterial;
        }

        uvScale = 1.0f / (featureRadius + featureRadius);
    }

    featureInstance->eventTemplate.center.y -= featureInstance->eventTemplate.depth;

    zClipUV *uvPairs = 0;
    if (hasMaterialUv) {
        uvPairs = (zClipUV *)(malloc((size_t)(uvCenterIndex + 1) * sizeof(zClipUV)));
    }

    zVec3 *const midPoints = (zVec3 *)(malloc((size_t)(pointCount) * sizeof(zVec3)));

    const zVec3 center = featureInstance->eventTemplate.center;
    const float lowCenterY =
        featureInstance->eventTemplate.center.y - featureInstance->eventTemplate.depth;

    if (hasMaterialUv) {
        uvPairs[uvCenterIndex].u = uvScale * featureRadius;
        uvPairs[uvCenterIndex].v = uvScale * featureRadius;
    }

    for (int i = 0; i < pointCount; ++i) {
        const zVec3 *const point = &points[i];
        zVec3 *const midPoint = &midPoints[i];

        midPoint->x = (center.x - point->x) * 0.5f + point->x;
        midPoint->y =
            ((center.y - point->y) * 0.5f + point->y) - featureInstance->eventTemplate.depth;
        midPoint->z = (center.z - point->z) * 0.5f + point->z;

        if (hasMaterialUv) {
            uvPairs[i].u = (point->x - center.x + featureRadius) * uvScale;
            uvPairs[i].v = (point->z - center.z + featureRadius) * uvScale;
            uvPairs[pointCount + i].u = (midPoint->x - center.x + featureRadius) * uvScale;
            uvPairs[pointCount + i].v = (midPoint->z - center.z + featureRadius) * uvScale;
        }
    }

    for (int i_440 = 0; i_440 < pointCount; ++i_440) {
        const int nextIndex = (i_440 + 1) % pointCount;
        zVec3 polygonPoints[4];
        polygonPoints[0] = points[i_440];
        polygonPoints[1] = points[nextIndex];
        polygonPoints[2] = midPoints[nextIndex];
        polygonPoints[3] = midPoints[i_440];

        zClipUV polygonUvs[4];
        zClipUV *uvList = 0;
        zModel_MaterialPartial *polygonMaterial = 0;
        if (hasMaterialUv) {
            polygonUvs[0] = uvPairs[i_440];
            polygonUvs[1] = uvPairs[nextIndex];
            polygonUvs[2] = uvPairs[pointCount + nextIndex];
            polygonUvs[3] = uvPairs[pointCount + i_440];
            uvList = polygonUvs;
            polygonMaterial = material;
        }

        zGeometry_Model::AddPolygonToDi(
            displayInstance,
            4,
            polygonPoints,
            polygonMaterial,
            uvList
        );
    }

    for (int i_463 = 0; i_463 < pointCount; ++i_463) {
        const int nextIndex = (i_463 + 1) % pointCount;
        zVec3 polygonPoints[3];
        polygonPoints[0].x = center.x;
        polygonPoints[0].y = lowCenterY;
        polygonPoints[0].z = center.z;
        polygonPoints[1] = midPoints[i_463];
        polygonPoints[2] = midPoints[nextIndex];

        zClipUV polygonUvs[3];
        zClipUV *uvList = 0;
        zModel_MaterialPartial *polygonMaterial = 0;
        if (hasMaterialUv) {
            polygonUvs[0] = uvPairs[uvCenterIndex];
            polygonUvs[1] = uvPairs[pointCount + i_463];
            polygonUvs[2] = uvPairs[pointCount + nextIndex];
            uvList = polygonUvs;
            polygonMaterial = material;
        }

        zGeometry_Model::AddPolygonToDi(
            displayInstance,
            3,
            polygonPoints,
            polygonMaterial,
            uvList
        );
    }

    free(midPoints);
    if (hasMaterialUv) {
        free(uvPairs);
    }

    zClass_Class::gwNodeSetDisplayInstance(
        node,
        displayInstance
    );
    return 0;
}
} /* namespace zDEClient_Crater */ namespace zDEClient {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-submitfeaturegeometry
 * @recoil-artifact defines .text recoil:function:0x4575f0: zDEClient::SubmitFeatureGeometry
 *
 * Purpose: submit each generated feature node/DI pair to the feature map tree
 * so later cleanup and serialization can locate it.
 */
void __fastcall SubmitFeatureGeometry(
    zGeometry_ClipPatchOutputPartial *clipPatchOutput
) {
    for (int partitionIndex = 0; partitionIndex < clipPatchOutput->partitionCount;
        ++partitionIndex) {
        zGeometry_ClipPatchPartitionOutput *const partition =
            &clipPatchOutput->partitions[partitionIndex];
        {
            for (int pairIndex = 0; pairIndex < partition->nodeDiPairCount; ++pairIndex) {
                g_zDEClient_FeatureMapTree.insert(
                    partition->nodeDiPairs[pairIndex].node
                );
            }
        }
    }
}

} // namespace zDEClient

namespace zDEClient {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-clearfeaturedisplaynodes
 * @recoil-artifact defines .text recoil:function:0x457750: zDEClient::ClearFeatureDisplayNodes.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: reload display instances and delete generated ZDEC_FEATURE nodes.
 */
void __cdecl ClearFeatureDisplayNodes() {
    for (std::set<zGeometry_ClipPatchNodeView *>::iterator entry =
            g_zDEClient_FeatureMapTree.begin();
        entry != g_zDEClient_FeatureMapTree.end();
        ++entry) {
        zGeometry_ClipPatchNodeView *key = *entry;
        if (key != 0) {
            GameZ_ZBD::ReloadDisplayInstancesFromCurrentPath_Local(
                key,
                1
            );

            const int gridCol = key->gridCol;
            const int gridRow = key->gridRow;
            if (gridCol >= 0 && gridRow >= 0) {
                zWorldAreaPartial *area =
                    zClass_World::GetAreaPartitionAtGrid(
                        key->listA[0],
                        gridCol,
                        gridRow
                    );
                if (area != 0) {
                    area->displayRefreshQueued = 0;
                }
            }
        }
    }

    zClass_NodePartial *child = zClass::FindByTypeAndName(
        6,
        g_zDEClient_FeatureNodeName
    );
    while (child != 0) {
        while (child->listCountA > 0) {
            zClass_Class::RemoveChild(
                child->listA[0],
                child
            );
        }

        unsigned int displayInstanceValue = 0;
        zClass_Class::gwNodeGetUserData(
            child,
            &displayInstanceValue
        );
        if (displayInstanceValue != 0) {
            zDiPartial *displayInstance = (zDiPartial *)((unsigned int)(displayInstanceValue));
            zClass_Class::gwNodeSetDisplayInstance(
                child,
                0
            );
            zModel_DiPool::FreeIfUnreferenced(displayInstance);
        }

        zClass_Class::DeleteNodeByType(child);
        child = zClass::FindByTypeAndName(
            6,
            g_zDEClient_FeatureNodeName
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-appendfeatureentry
 * @recoil-artifact defines .text recoil:function:0x457840: zDEClient::AppendFeatureEntry
 *
 * Purpose: append a crater or quicksand event snapshot to the feature-entry
 * list, growing the VC-era vector storage when needed.
 */
int __fastcall AppendFeatureEntry(
    int featureType,
    const void *featureEventData
) {
    size_t eventDataBytes = 0;
    if (featureType == 1) {
        eventDataBytes = sizeof(zDEClient_CraterEventTemplate);
    } else if (featureType == 3) {
        eventDataBytes = sizeof(zDEClient_QSandEventTemplate);
    } else {
        return 0;
    }

    zDEClient_FeatureEntry featureEntry;
    featureEntry.featureType = featureType;
    memset(
        &featureEntry.eventData,
        0,
        sizeof(featureEntry.eventData)
    );
    memcpy(
        &featureEntry.eventData,
        featureEventData,
        eventDataBytes
    );
    featureEntry.reloadFlag = 0;

    g_zDEClient_FeatureList.push_back(featureEntry);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-clearfeatureentriesandmaptree
 * @recoil-artifact defines .text recoil:function:0x457ae0: zDEClient::ClearFeatureEntriesAndMapTree.
 *
 * Purpose: reset feature entry storage and clear all feature map-tree nodes.
 */
int __cdecl ClearFeatureEntriesAndMapTree() {
    g_zDEClient_FeatureList.clear();
    g_zDEClient_FeatureMapTree.clear();

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-writefeaturesectionstozar
 * @recoil-artifact defines .text recoil:function:0x457b40: zDEClient::WriteFeatureSectionsToZAR.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: serialize saved crater and quicksand feature entries into ZAR
 * sections.
 */
int __fastcall WriteFeatureSectionsToZAR(
    zZbdSectionCallbackCtx *callbackCtx
) {
    int craterSectionIndex = 0;
    int qSandSectionIndex = 0;
    zDEClient_FeatureEntry featureEntry;
    featureEntry.reloadFlag = 1;

    int result =
        zUtil_ZAR::WriteSectionBlob(
            callbackCtx,
            "Dummy",
            &featureEntry,
            sizeof(featureEntry)
        );

    for (std::vector<zDEClient_FeatureEntry>::iterator entry =
            g_zDEClient_FeatureList.begin();
        entry != g_zDEClient_FeatureList.end() && result != 0;
        ++entry) {
        featureEntry = *entry;
        featureEntry.reloadFlag = 0;

        char sectionName[0x40];
        const char *sectionNameFormat;
        int sectionIndex;
        switch (featureEntry.featureType) {
        case 1:
            sectionNameFormat = g_zDEClient_CraterNameFmt;
            sectionIndex = craterSectionIndex++;
            break;
        case 3:
            sectionNameFormat = g_zDEClient_QuickSandNameFmt;
            sectionIndex = qSandSectionIndex++;
            break;
        default:
            continue;
        }

        sprintf(sectionName, sectionNameFormat, sectionIndex);
        result = zUtil_ZAR::WriteSectionBlob(
            callbackCtx,
            sectionName,
            &featureEntry,
            sizeof(featureEntry)
        );
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-applyfeatureentry
 * @recoil-artifact defines .text recoil:function:0x457c10: zDEClient::ApplyFeatureEntry.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: reload a serialized zDEClient feature entry or clear feature
 * display state for a reload marker.
 */
void __stdcall ApplyFeatureEntry(
    zDEClient_FeatureEntry *container,
    void *,
    void *
) {
    if (container->reloadFlag != 0) {
        ClearFeatureDisplayNodes();
        ClearFeatureEntriesAndMapTree();
        return;
    }

    if (container->featureType == 3) {
        zDEClient_QSand::InstanceEventMaybeRelay(&container->eventData.quickSand);
    } else if (container->featureType == 1) {
        zDEClient_Crater::InstanceEvent(
            &container->eventData.crater,
            0
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-dispatchfeatureeventtemplates
 * @recoil-artifact defines .text recoil:function:0x457c50: zDEClient::DispatchFeatureEventTemplates.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: iterate feature-entry snapshots and dispatch crater or quicksand
 * event templates to caller-provided handlers.
 */
void __fastcall DispatchFeatureEventTemplates(
    zDEClient_CraterFeatureDispatch craterHandler,
    zDEClient_QSandFeatureDispatch qSandHandler
) {
    for (std::vector<zDEClient_FeatureEntry>::iterator entry =
            g_zDEClient_FeatureList.begin();
        entry != g_zDEClient_FeatureList.end();
        ++entry) {
        zDEClient_FeatureEntry featureEntry = *entry;

        if (featureEntry.featureType == 3) {
            if (qSandHandler != 0) {
                qSandHandler(&featureEntry.eventData.quickSand);
            }
        } else if (featureEntry.featureType == 1) {
            if (craterHandler != 0) {
                craterHandler(&featureEntry.eventData.crater);
            }
        }
    }
}
} // namespace zDEClient
namespace zDEClient {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-setcameranode
 * @recoil-artifact defines .text recoil:function:0x458aa0: zDEClient::SetCameraNode
 *
 * Purpose: record the active camera node and its class-data feature grid.
 */
void __fastcall SetCameraNode(
    zClass_NodePartial *cameraNode
) {
    if (cameraNode != 0) {
        g_zDEClient_CameraNode = cameraNode;
        g_zDEClient_CameraNodeClassData = (zClass_CameraDataPartial *)(cameraNode->classData);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-getfeaturegridcell
 * @recoil-artifact defines .text recoil:function:0x458ac0: zDEClient::GetFeatureGridCell
 *
 * Purpose: return a feature-grid cell from the current camera node data.
 */
zDEClient_FeatureGridCell *__fastcall GetFeatureGridCell(
    int gridCol,
    int gridRow
) {
    if (g_zDEClient_CameraNodeClassData == 0) {
        return 0;
    }

    zDEClient_CameraNodeClassDataPartial *data =
        (zDEClient_CameraNodeClassDataPartial *)(g_zDEClient_CameraNodeClassData);
    return &data->featureGridRows[gridRow][gridCol];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-crater-getcameranode
 * @recoil-artifact defines .text recoil:function:0x458ae0: zDEClient::GetCameraNode
 *
 * Purpose: expose the active camera node used by terrain feature helpers.
 */
zClass_NodePartial *__cdecl GetCameraNode() {
    return g_zDEClient_CameraNode;
}
} // namespace zDEClient
