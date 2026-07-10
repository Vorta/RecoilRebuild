/* This source-layout fragment is included by the current compatibility container.
 * Parent build/manifests must compile this path directly after retiring the container include.
 */
namespace zDEClient_QSand {
/**
 * Reimplements 0x455ea0: zDEClient_QSand::DestroyFeature
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.c).
 *
 * Purpose: release a quicksand feature instance, including its generated point
 * buffer and clip-patch output.
 */
void __fastcall DestroyFeature(
    zDEClient_QSandFeature *featureInstance
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
} /* namespace zDEClient_QSand */ namespace zDEClient {
/**
 * Reimplements 0x455ed0: zDEClient::CopyQSandEventTemplateDefaults
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.c).
 *
 * Purpose: copy the configured quicksand event template defaults into a
 * caller-owned event template.
 */
void __fastcall CopyQSandEventTemplateDefaults(
    zDEClient_QSandEventTemplate *eventTemplate
) {
    memcpy(
        eventTemplate,
        &g_zDEClient_QuickSandEventTemplateDefaults,
        sizeof(zDEClient_QSandEventTemplate)
    );
}
} /* namespace zDEClient */ namespace zDEClient_QSand {
/**
 * Reimplements 0x455ef0: zDEClient_QSand::InstanceEventMaybeRelay
 * (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c).
 *
 * Purpose: let the registered quicksand relay callback veto instancing before
 * building and submitting the quicksand feature locally.
 */
int __fastcall InstanceEventMaybeRelay(
    zDEClient_QSandEventTemplate *eventTemplate
) {
    if (g_zDEClientQSandNetRelayCallback != 0 &&
        g_zDEClientQSandNetRelayCallback(eventTemplate) == 0) {
        return -1;
    }

    if (g_zDEClient_QuickSandEnabled == 0) {
        return -1;
    }

    const float vertexMergeEpsilon = zModel_Const::GetVertexMergeEpsilon();
    zModel_Const::SetVertexMergeEpsilon(0.00499999989f);

    zDEClient_QSandFeature *const featureInstance = InitFeatureFromEventTemplate(eventTemplate);
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
        zError::ReportOld(
            0x100,
            g_zDEClient_SourceFile_ZdecQsandCpp,
            0x92,
            g_zDEClient_QuickSandInstanceClipFailedMsg
        );
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

    zDEClient::AppendFeatureEntry(
        3,
        eventTemplate
    );
    zDEClient::SubmitFeatureGeometry(featureInstance->clipPatchOutput);
    zGeometry_ClipPatchOutput::ApplyNodeDiPairs(featureInstance->clipPatchOutput);

    zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
    return 0;
}
} // namespace zDEClient_QSand

namespace zDEClient_QSand {
/**
 * Reimplements 0x456010:
 * zDEClient_QSand::InitFeatureFromEventTemplate
 * (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c).
 *
 * Purpose: create a quicksand feature from an event template, fit it to the
 * owning feature grid cell, and generate its circular point bounds.
 */
zDEClient_QSandFeature *__fastcall InitFeatureFromEventTemplate(
    zDEClient_QSandEventTemplate *eventTemplate
) {
    zDEClient_QSandFeature *featureInstance = CreateFeatureStructFromEventTemplate(eventTemplate);
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

    for (int i_674 = 1; i_674 < eventTemplate->pointCount; ++i_674) {
        zVec3 *const point = &points[i_674];
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
 * Reimplements 0x4563d0:
 * zDEClient_QSand::CreateFeatureStructFromEventTemplate
 * (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c).
 *
 * Purpose: allocate and initialize the quicksand feature record copied from an
 * event template, including point storage, clip output, and default material
 * binding.
 */
zDEClient_QSandFeature *__fastcall CreateFeatureStructFromEventTemplate(
    zDEClient_QSandEventTemplate *eventTemplate
) {
    zDEClient_QSandFeature *result =
        (zDEClient_QSandFeature *)(malloc(sizeof(zDEClient_QSandFeature)));
    memset(
        result,
        0,
        sizeof(zDEClient_QSandFeature)
    );

    result->featureType = 3;
    memcpy(
        &result->eventTemplate,
        eventTemplate,
        sizeof(result->eventTemplate)
    );
    result->points = (zVec3 *)(malloc(result->eventTemplate.pointCount * sizeof(zVec3)));
    result->clipPatchOutput = zGeometry_ClipPatchOutput::Create();

    if ((result->eventTemplate.featureFlags & 0x1008) != 0 && result->eventTemplate.material == 0) {
        result->eventTemplate.material = g_zDEClient_QuickSandMaterial;
        result->eventTemplate.materialCycle = g_zDEClient_QuickSandMaterialCycle;
    }

    return result;
}

/**
 * Reimplements 0x456450: zDEClient_QSand::Build
 * (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c).
 *
 * Purpose: clip the quicksand polygon into the feature grid cell and adopt the
 * clipped point list.
 */
int __fastcall Build(
    zDEClient_QSandFeature *featureInstance
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
 * Reimplements 0x4564b0: zDEClient_QSand::CreateFeature
 * (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c).
 *
 * Purpose: create quicksand side and cap display geometry from the clipped
 * feature points and attach both display instances to generated feature nodes.
 */
int __fastcall CreateFeature(
    zDEClient_QSandFeature *featureInstance
) {
    zClass_NodePartial *node = 0;
    zDiPartial *displayInstance = zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(
        featureInstance->clipPatchOutput->partitions,
        zDEClient::GetCameraNode(),
        &node
    );
    if (displayInstance == 0 || node == 0) {
        if (displayInstance != 0) {
            zModel_DiPool::FreeIfUnreferenced(displayInstance);
        }

        if (node != 0) {
            zClass_Object3D::DeleteNode(node);
        }

        return -1;
    }

    zClass_Class::gwNodeSetName(
        node,
        g_zDEClient_FeatureNodeName
    );
    node->callbackContext = (zClass_NodePartial *)(featureInstance);

    zVec3 *const points = featureInstance->points;
    const int pointCount = featureInstance->eventTemplate.pointCount;
    const int featureFlags = featureInstance->eventTemplate.featureFlags;
    const int uvCenterIndex = pointCount * 3;
    const bool hasMaterialUv = (featureFlags & 0x1008) != 0;

    zModel_MaterialPartial *sideMaterial = 0;
    float featureRadius = 0.0f;
    float uvScale = 0.0f;
    if (hasMaterialUv) {
        featureRadius = featureInstance->eventTemplate.radius;
        sideMaterial = featureInstance->eventTemplate.material;
        uvScale = 4.5f / (featureRadius + featureRadius);
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
    const float topCenterY =
        featureInstance->eventTemplate.center.y + featureInstance->eventTemplate.depth;

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

    for (int i_837 = 0; i_837 < pointCount; ++i_837) {
        const int nextIndex = (i_837 + 1) % pointCount;
        zVec3 polygonPoints[4];
        polygonPoints[0] = points[i_837];
        polygonPoints[1] = points[nextIndex];
        polygonPoints[2] = midPoints[nextIndex];
        polygonPoints[3] = midPoints[i_837];

        zClipUV polygonUvs[4];
        zClipUV *uvList = 0;
        zModel_MaterialPartial *material = 0;
        if (hasMaterialUv) {
            polygonUvs[0] = uvPairs[i_837];
            polygonUvs[1] = uvPairs[nextIndex];
            polygonUvs[2] = uvPairs[pointCount + nextIndex];
            polygonUvs[3] = uvPairs[pointCount + i_837];
            uvList = polygonUvs;
            material = featureInstance->eventTemplate.materialCycle;
        }

        zGeometry_Model::AddPolygonToDi(
            displayInstance,
            4,
            polygonPoints,
            material,
            uvList
        );
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
        zClipUV *uvList = 0;
        zModel_MaterialPartial *material = 0;
        if (hasMaterialUv) {
            polygonUvs[0] = uvPairs[uvCenterIndex];
            polygonUvs[1] = uvPairs[pointCount + i_860];
            polygonUvs[2] = uvPairs[pointCount + nextIndex];
            uvList = polygonUvs;
            material = featureInstance->eventTemplate.materialCycle;
        }

        zGeometry_Model::AddPolygonToDi(
            displayInstance,
            3,
            polygonPoints,
            material,
            uvList
        );
    }

    zClass_NodePartial *capNode = 0;
    zDiPartial *const capDisplayInstance = zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(
        featureInstance->clipPatchOutput->partitions,
        zDEClient::GetCameraNode(),
        &capNode
    );
    if (capDisplayInstance == 0 || capNode == 0) {
        if (capDisplayInstance != 0) {
            zModel_DiPool::FreeIfUnreferenced(capDisplayInstance);
        }

        if (capNode != 0) {
            zClass_Object3D::DeleteNode(capNode);
        }

        return -1;
    }

    zClass_Class::gwNodeSetName(
        capNode,
        g_zDEClient_FeatureNodeName
    );
    capNode->callbackContext = (zClass_NodePartial *)(featureInstance);

    for (int i_901 = 0; i_901 < pointCount; ++i_901) {
        const int nextIndex = (i_901 + 1) % pointCount;
        zVec3 polygonPoints[3];
        polygonPoints[0].x = center.x;
        polygonPoints[0].y = topCenterY;
        polygonPoints[0].z = center.z;
        polygonPoints[1] = points[i_901];
        polygonPoints[2] = points[nextIndex];

        zClipUV polygonUvs[3];
        zClipUV *uvList = 0;
        zModel_MaterialPartial *material = 0;
        if (hasMaterialUv) {
            polygonUvs[0] = uvPairs[uvCenterIndex];
            polygonUvs[1] = uvPairs[pointCount + i_901];
            polygonUvs[2] = uvPairs[pointCount + nextIndex];
            uvList = polygonUvs;
            material = sideMaterial;
        }

        zGeometry_Model::AddPolygonToDi(
            capDisplayInstance,
            3,
            polygonPoints,
            material,
            uvList
        );
    }

    free(midPoints);
    if (hasMaterialUv) {
        free(uvPairs);
    }

    return 0;
}
} // namespace zDEClient_QSand
