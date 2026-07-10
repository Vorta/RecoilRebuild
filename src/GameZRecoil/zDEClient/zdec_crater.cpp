/* This source-layout fragment is included by the current compatibility container.
 * Parent build/manifests must compile this path directly after retiring the container include.
 */
namespace zDEClient_Crater {
/**
 * Reimplements 0x456ad0: zDEClient_Crater::DestroyFeature
 * (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c).
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
 * Reimplements 0x456b00: zDEClient_Crater::InitEventTemplateDefaults
 * (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c).
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
 * Reimplements 0x456b20: zDEClient_Crater::InstanceEvent
 * (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c).
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
 * Reimplements 0x456c50: zDEClient_Crater::InstanceEventMaybeRelay
 * (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c).
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
 * Reimplements 0x456c80: zDEClient_Crater::InitFeatureFromEventTemplate
 * (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c).
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
 * Reimplements 0x457040: zDEClient_Crater::CreateFeatureStructFromEventTemplate
 * (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c).
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
 * Reimplements 0x4570e0: zDEClient_Crater::Build
 * Source: D:\Proj\GameZRecoil\zDEClient\zdec_crater.c
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
 * Reimplements 0x457140: zDEClient_Crater::CreateFeature
 * (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c).
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
 * Reimplements 0x4575f0: zDEClient::SubmitFeatureGeometry
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
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
                zDEClient_MapTreeLocateResult result;
                g_zDEClient_FeatureMapTree.FindOrInsertKey(
                    &result,
                    &partition->nodeDiPairs[pairIndex]
                );
            }
        }
    }
}

/**
 * Reimplements 0x457650: zDEClient::InitFeatureSystem.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: initialize feature-entry storage and register shutdown cleanup.
 */
void InitFeatureSystem() {
    InitFeatureEntryListAndMapTree();
    RegisterFeatureSystemCleanupAtExit();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *zDEClientCrtInitializerFn)();
/* VC5 emits this zdec_init.cpp startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
zDEClientCrtInitializerFn s_zDEClientCrtInit_InitFeatureSystem =
    InitFeatureSystem;
#pragma data_seg()
#endif

/**
 * Reimplements 0x457660: zDEClient::InitFeatureEntryListAndMapTree.
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
 *
 * Purpose: initialize feature entry vector globals and their map-tree lookup.
 */
void InitFeatureEntryListAndMapTree() {
    char modeValue = 0;
    char flagsValue = 0;
    g_zDEClient_FeatureMapTree.InitState(
        &modeValue,
        &flagsValue
    );

    g_zDEClient_FeatureListFlags = (unsigned char)(modeValue);
    g_zDEClient_FeatureListBegin = 0;
    g_zDEClient_FeatureListEnd = 0;
    g_zDEClient_FeatureListCapacityEnd = 0;
}

/**
 * Reimplements 0x4576a0: zDEClient::RegisterFeatureSystemCleanupAtExit.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: register feature-system shutdown with the CRT atexit list.
 */
void RegisterFeatureSystemCleanupAtExit() {
    atexit(ShutdownFeatureSystem);
}

/**
 * Reimplements 0x4576b0: zDEClient::ShutdownFeatureSystem.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: release feature-entry vector storage and destroy the feature map
 * tree.
 */
void ShutdownFeatureSystem() {
    ::operator delete(g_zDEClient_FeatureListBegin);
    g_zDEClient_FeatureListBegin = 0;
    g_zDEClient_FeatureListEnd = 0;
    g_zDEClient_FeatureListCapacityEnd = 0;

    g_zDEClient_FeatureMapTree.Destroy();
}
} // namespace zDEClient
/**
 * Reimplements 0x4576e0: zDEClient_MapTreeState::Destroy.
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
 *
 * Purpose: clear feature map-tree nodes, release the header, and drop the
 * shared nil sentinel reference.
 */
void zDEClient_MapTreeState::Destroy() {
    zDEClient_MapTreeNode *outNext = 0;
    EraseRange(
        &outNext,
        header->left,
        header
    );

    ::operator delete(header);
    header = 0;
    nodeCount = 0;

#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    {
        std::_Lockit _Lk;
#endif
        --g_zDEClient_FeatureMapTreeNilRefCount;
        if (g_zDEClient_FeatureMapTreeNilRefCount == 0) {
            ::operator delete(g_zDEClient_FeatureMapTreeNil);
            g_zDEClient_FeatureMapTreeNil = 0;
        }
#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    }
#endif
}

namespace zDEClient {
/**
 * Reimplements 0x457750: zDEClient::ClearFeatureDisplayNodes.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: reload display instances and delete generated ZDEC_FEATURE nodes.
 */
void ClearFeatureDisplayNodes() {
    zDEClient_MapTreeNode *header = g_zDEClient_FeatureMapTree.header;
    zDEClient_MapTreeNode *node = header->left;

    while (node != header) {
        zGeometry_ClipPatchNodeView *key = node->key;
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

        g_zDEClient_FeatureMapTree.IterNextNodeRef(&node);
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
 * Reimplements 0x457840: zDEClient::AppendFeatureEntry
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
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

    if (g_zDEClient_FeatureListEnd == g_zDEClient_FeatureListCapacityEnd) {
        const ptrdiff_t oldCount = g_zDEClient_FeatureListBegin != 0
                                       ? g_zDEClient_FeatureListEnd - g_zDEClient_FeatureListBegin
                                       : 0;
        const ptrdiff_t capacityIncrement = oldCount > 1 ? oldCount : 1;
        const ptrdiff_t newCapacity = oldCount + capacityIncrement;

        zDEClient_FeatureEntry *const newEntries = (zDEClient_FeatureEntry *)(::operator new(
            (size_t)(newCapacity) * sizeof(zDEClient_FeatureEntry)
        ));

        CopyFeatureEntriesForward(
            g_zDEClient_FeatureListBegin,
            g_zDEClient_FeatureListEnd,
            newEntries
        );

        ::operator delete(g_zDEClient_FeatureListBegin);
        g_zDEClient_FeatureListBegin = newEntries;
        g_zDEClient_FeatureListEnd = newEntries + oldCount;
        g_zDEClient_FeatureListCapacityEnd = newEntries + newCapacity;
    }

    FillFeatureEntries(
        g_zDEClient_FeatureListEnd,
        1,
        &featureEntry
    );
    ++g_zDEClient_FeatureListEnd;
    return 0;
}

/**
 * Reimplements 0x457ae0: zDEClient::ClearFeatureEntriesAndMapTree.
 *
 * Purpose: reset feature entry storage and clear all feature map-tree nodes.
 */
int ClearFeatureEntriesAndMapTree() {
    g_zDEClient_FeatureListEnd = g_zDEClient_FeatureListBegin;

    if (g_zDEClient_FeatureMapTree.header != 0) {
        zDEClient_MapTreeNode *outNext = 0;
        g_zDEClient_FeatureMapTree.EraseRange(
            &outNext,
            g_zDEClient_FeatureMapTree.header->left,
            g_zDEClient_FeatureMapTree.header
        );
    }

    return 0;
}

/**
 * Reimplements 0x457b40: zDEClient::WriteFeatureSectionsToZAR.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
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

    for (zDEClient_FeatureEntry *entry = g_zDEClient_FeatureListBegin;
        entry != g_zDEClient_FeatureListEnd && result != 0;
        ++entry) {
        memcpy(
            &featureEntry,
            entry,
            sizeof(featureEntry)
        );
        featureEntry.reloadFlag = 0;

        char sectionName[0x40];
        if (featureEntry.featureType == 1) {
            sprintf(
                sectionName,
                g_zDEClient_CraterNameFmt,
                craterSectionIndex
            );
            ++craterSectionIndex;
            result = zUtil_ZAR::WriteSectionBlob(
                callbackCtx,
                sectionName,
                &featureEntry,
                sizeof(featureEntry)
            );
        } else if (featureEntry.featureType == 3) {
            sprintf(
                sectionName,
                g_zDEClient_QuickSandNameFmt,
                qSandSectionIndex
            );
            ++qSandSectionIndex;
            result = zUtil_ZAR::WriteSectionBlob(
                callbackCtx,
                sectionName,
                &featureEntry,
                sizeof(featureEntry)
            );
        }
    }

    return result;
}

/**
 * Reimplements 0x457c10: zDEClient::ApplyFeatureEntry.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
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

    if (container->featureType == 1) {
        zDEClient_Crater::InstanceEvent(
            &container->eventData.crater,
            0
        );
    } else if (container->featureType == 3) {
        zDEClient_QSand::InstanceEventMaybeRelay(&container->eventData.quickSand);
    }
}

/**
 * Reimplements 0x457c50: zDEClient::DispatchFeatureEventTemplates.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: iterate feature-entry snapshots and dispatch crater or quicksand
 * event templates to caller-provided handlers.
 */
void __fastcall DispatchFeatureEventTemplates(
    zDEClient_CraterFeatureDispatch craterHandler,
    zDEClient_QSandFeatureDispatch qSandHandler
) {
    for (zDEClient_FeatureEntry *entry = g_zDEClient_FeatureListBegin;
        entry != g_zDEClient_FeatureListEnd;
        ++entry) {
        zDEClient_FeatureEntry featureEntry;
        memcpy(
            &featureEntry,
            entry,
            sizeof(featureEntry)
        );

        if (featureEntry.featureType == 1) {
            if (craterHandler != 0) {
                craterHandler(&featureEntry.eventData.crater);
            }
        } else if (featureEntry.featureType == 3) {
            if (qSandHandler != 0) {
                qSandHandler(&featureEntry.eventData.quickSand);
            }
        }
    }
}
} // namespace zDEClient
/**
 * Reimplements 0x457cc0: zDEClient_MapTreeState::InitState.
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
 *
 * Purpose: initialize feature map-tree state and lazily create the shared nil
 * sentinel.
 */
zDEClient_MapTreeState * zDEClient_MapTreeState::InitState(
    char *modeValue,
    char *flagsValue
) {
    mode = *modeValue;
    flags = *flagsValue;
    allowInsert = 0;

#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    std::_Lockit _Lk;
#endif
    if (g_zDEClient_FeatureMapTreeNil == 0) {
        g_zDEClient_FeatureMapTreeNil =
            (zDEClient_MapTreeNode *)(::operator new(sizeof(zDEClient_MapTreeNode)));
        g_zDEClient_FeatureMapTreeNil->left = 0;
        g_zDEClient_FeatureMapTreeNil->parent = 0;
        g_zDEClient_FeatureMapTreeNil->right = 0;
        g_zDEClient_FeatureMapTreeNil->key = 0;
        g_zDEClient_FeatureMapTreeNil->colorOrNil = 1;
    }

    ++g_zDEClient_FeatureMapTreeNilRefCount;

    header = (zDEClient_MapTreeNode *)(::operator new(sizeof(zDEClient_MapTreeNode)));
    header->left = header;
    header->parent = g_zDEClient_FeatureMapTreeNil;
    header->right = header;
    header->key = 0;
    header->colorOrNil = 0;
    nodeCount = 0;
    return this;
}

/**
 * Reimplements 0x457d90: zDEClient_MapTreeState::FindOrInsertKey.
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
 *
 * Purpose: locate the feature map-tree entry for a clip-patch node key, or
 * insert it when tree state allows.
 */
zDEClient_MapTreeLocateResult * zDEClient_MapTreeState::FindOrInsertKey(
    zDEClient_MapTreeLocateResult *outResult,
    zGeometry_ClipPatchNodeDiPair *key
) {
    EnsureFeatureMapTreeInitialized(this);

    zGeometry_ClipPatchNodeView *const nodeKey = key->node;
    zDEClient_MapTreeNode *parent = header;
    zDEClient_MapTreeNode *cursor = header->parent;
    unsigned char insertLeft = 1;

#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    {
        std::_Lockit _Lk;
#endif
        while (!IsNil(cursor)) {
            parent = cursor;
            if (nodeKey < cursor->key) {
                insertLeft = 1;
                cursor = cursor->left;
            } else {
                insertLeft = 0;
                cursor = cursor->right;
            }
        }
#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    }
#endif

    if (allowInsert != 0) {
        zDEClient_MapTreeNode *inserted = 0;
        InsertAt(
            &inserted,
            cursor,
            parent,
            key
        );
        outResult->node = inserted;
        outResult->inserted = 1;
        return outResult;
    }

    zDEClient_MapTreeNode *candidate = parent;
    if (insertLeft != 0) {
        if (parent == header->left) {
            zDEClient_MapTreeNode *inserted = 0;
            InsertAt(
                &inserted,
                cursor,
                parent,
                key
            );
            outResult->node = inserted;
            outResult->inserted = 1;
            return outResult;
        }

        candidate = parent;
        IterPrevNodeRef(&candidate);
    }

    if (candidate->key < nodeKey) {
        zDEClient_MapTreeNode *inserted = 0;
        InsertAt(
            &inserted,
            cursor,
            parent,
            key
        );
        outResult->node = inserted;
        outResult->inserted = 1;
        return outResult;
    }

    outResult->node = candidate;
    outResult->inserted = 0;
    return outResult;
}

/**
 * Reimplements 0x457e80: zDEClient_MapTreeState::EraseRange.
 *
 * Purpose: erase a map-tree iterator range or clear the whole tree fast path.
 */
zDEClient_MapTreeNode ** zDEClient_MapTreeState::EraseRange(
    zDEClient_MapTreeNode **outNext,
    zDEClient_MapTreeNode *first,
    zDEClient_MapTreeNode *last
) {
    if (nodeCount != 0 && first == header->left && last == header) {
#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
        std::_Lockit _Lk;
#endif
        DestroySubtree(header->parent);
        nodeCount = 0;
        ResetHeader(this);
        *outNext = header->left;
        return outNext;
    }

    while (first != last) {
        EraseAndAdvance(
            &first,
            first
        );
    }

    *outNext = first;
    return outNext;
}

/**
 * Reimplements 0x457fe0: zDEClient_MapTreeState::EraseAndAdvance.
 *
 * Purpose: remove one map-tree node, maintain tree links, and return the next
 * iterator node.
 */
zDEClient_MapTreeNode ** zDEClient_MapTreeState::EraseAndAdvance(
    zDEClient_MapTreeNode **outNext,
    zDEClient_MapTreeNode *node
) {
    zDEClient_MapTreeNode *next = node;
    IterNextNodeRef(&next);

#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    std::_Lockit _Lk;
#endif
    if (IsNil(node->left)) {
        Transplant(
            this,
            node,
            node->right
        );
    } else if (IsNil(node->right)) {
        Transplant(
            this,
            node,
            node->left
        );
    } else {
        zDEClient_MapTreeNode *successor = TreeMinimum(node->right);
        if (successor->parent != node) {
            Transplant(
                this,
                successor,
                successor->right
            );
            successor->right = node->right;
            successor->right->parent = successor;
        }

        Transplant(
            this,
            node,
            successor
        );
        successor->left = node->left;
        successor->left->parent = successor;
        successor->colorOrNil = node->colorOrNil;
    }

    if (nodeCount > 0) {
        --nodeCount;
    }

    RefreshHeaderExtents(this);
    ::operator delete(node);
    *outNext = next;
    return outNext;
}

/**
 * Reimplements 0x458510: zDEClient_MapTreeState::DestroySubtree.
 *
 * Purpose: recursively release a feature map-tree subtree below a non-nil node.
 */
void zDEClient_MapTreeState::DestroySubtree(
    zDEClient_MapTreeNode *node
) {
#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    std::_Lockit _Lk;
#endif
    while (!IsNil(node)) {
        DestroySubtree(node->right);
        zDEClient_MapTreeNode *const left = node->left;
        ::operator delete(node);
        node = left;
    }
}

/**
 * Reimplements 0x4585a0: zDEClient_MapTreeState::InsertAt.
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
 *
 * Purpose: allocate and link a feature map-tree node, then rebalance the tree.
 */
zDEClient_MapTreeNode ** zDEClient_MapTreeState::InsertAt(
    zDEClient_MapTreeNode **outNode,
    zDEClient_MapTreeNode *where,
    zDEClient_MapTreeNode *parent,
    zGeometry_ClipPatchNodeDiPair *key
) {
#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    std::_Lockit _Lk;
#endif
    zDEClient_MapTreeNode *inserted =
        (zDEClient_MapTreeNode *)(::operator new(sizeof(zDEClient_MapTreeNode)));
    inserted->left = g_zDEClient_FeatureMapTreeNil;
    inserted->parent = parent;
    inserted->right = g_zDEClient_FeatureMapTreeNil;
    inserted->key = key->node;
    inserted->colorOrNil = 0;

    ++nodeCount;

    if (parent != header && where == g_zDEClient_FeatureMapTreeNil && !(key->node < parent->key)) {
        parent->right = inserted;
        if (parent == header->right) {
            header->right = inserted;
        }
    } else {
        parent->left = inserted;
        if (parent == header) {
            header->parent = inserted;
            header->right = inserted;
        } else if (parent == header->left) {
            header->left = inserted;
        }
    }

    zDEClient_MapTreeNode *fixup = inserted;
    while (fixup != header->parent && fixup->parent->colorOrNil == 0) {
        zDEClient_MapTreeNode *const parentNode = fixup->parent;
        zDEClient_MapTreeNode *const grandParent = parentNode->parent;
        if (parentNode == grandParent->left) {
            zDEClient_MapTreeNode *const uncle = grandParent->right;
            if (uncle->colorOrNil == 0) {
                parentNode->colorOrNil = 1;
                uncle->colorOrNil = 1;
                grandParent->colorOrNil = 0;
                fixup = grandParent;
            } else {
                if (fixup == parentNode->right) {
                    fixup = parentNode;
                    RotateTreeLeft(
                        this,
                        fixup
                    );
                }

                fixup->parent->colorOrNil = 1;
                fixup->parent->parent->colorOrNil = 0;
                RotateTreeRight(
                    this,
                    fixup->parent->parent
                );
            }
        } else {
            zDEClient_MapTreeNode *const uncle = grandParent->left;
            if (uncle->colorOrNil == 0) {
                parentNode->colorOrNil = 1;
                uncle->colorOrNil = 1;
                grandParent->colorOrNil = 0;
                fixup = grandParent;
            } else {
                if (fixup == parentNode->left) {
                    fixup = parentNode;
                    RotateTreeRight(
                        this,
                        fixup
                    );
                }

                fixup->parent->colorOrNil = 1;
                fixup->parent->parent->colorOrNil = 0;
                RotateTreeLeft(
                    this,
                    fixup->parent->parent
                );
            }
        }
    }

    header->parent->colorOrNil = 1;
    *outNode = inserted;
    return outNode;
}

/**
 * Reimplements 0x4588c0: zDEClient_MapTreeState::IterNextNodeRef.
 *
 * Purpose: advance a map-tree iterator node reference to the next in-order node.
 */
zDEClient_MapTreeNode ** zDEClient_MapTreeState::IterNextNodeRef(
    zDEClient_MapTreeNode **nodeRef
) {
#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    std::_Lockit _Lk;
#endif
    zDEClient_MapTreeNode *node = *nodeRef;
    if (!IsNil(node->right)) {
        *nodeRef = TreeMinimum(node->right);
        return nodeRef;
    }

    zDEClient_MapTreeNode *parent = node->parent;
    while (node == parent->right) {
        node = parent;
        parent = parent->parent;
    }

    if (node->right != parent) {
        node = parent;
    }

    *nodeRef = node;
    return nodeRef;
}

/**
 * Reimplements 0x458970: zDEClient_MapTreeState::IterPrevNodeRef.
 *
 * Purpose: move a map-tree iterator node reference to the previous in-order
 * node, including header-end handling.
 */
void zDEClient_MapTreeState::IterPrevNodeRef(
    zDEClient_MapTreeNode **nodeRef
) {
#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    std::_Lockit _Lk;
#endif
    zDEClient_MapTreeNode *node = *nodeRef;
    if (node->colorOrNil == 0 && node->parent->parent == node) {
        *nodeRef = node->right;
        return;
    }

    if (!IsNil(node->left)) {
        *nodeRef = TreeMaximum(node->left);
        return;
    }

    zDEClient_MapTreeNode *parent = node->parent;
    while (node == parent->left) {
        node = parent;
        parent = parent->parent;
    }

    *nodeRef = parent;
}
namespace zDEClient {
/**
 * Reimplements 0x458a30: zDEClient::CopyFeatureEntriesForward.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: copy feature-entry records forward during vector growth.
 */
zDEClient_FeatureEntry *__stdcall CopyFeatureEntriesForward(
    zDEClient_FeatureEntry *first,
    zDEClient_FeatureEntry *last,
    zDEClient_FeatureEntry *dest
) {
    while (first != last) {
        if (dest != 0) {
            *dest = *first;
        }

        ++first;
        ++dest;
    }

    return dest;
}

/**
 * Reimplements 0x458a70: zDEClient::FillFeatureEntries.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: fill feature-entry vector slots with a repeated feature record.
 */
void __stdcall FillFeatureEntries(
    zDEClient_FeatureEntry *dest,
    unsigned int count,
    const zDEClient_FeatureEntry *value
) {
    while (count != 0) {
        if (dest != 0) {
            *dest = *value;
        }

        ++dest;
        --count;
    }
}

/**
 * Reimplements 0x458aa0: zDEClient::SetCameraNode
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
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
 * Reimplements 0x458ac0: zDEClient::GetFeatureGridCell
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
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
 * Reimplements 0x458ae0: zDEClient::GetCameraNode
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
 *
 * Purpose: expose the active camera node used by terrain feature helpers.
 */
zClass_NodePartial *GetCameraNode() {
    return g_zDEClient_CameraNode;
}
} // namespace zDEClient
