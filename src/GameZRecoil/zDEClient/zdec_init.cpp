#include "zdec.h"

#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "zDi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

int g_zDEClient_QuickSandEnabled = 0;
int g_zDEClient_QuickSandTextureCount = 0;
float g_zDEClient_QuickSandAnimSpeed = 0.0f;
char **g_zDEClient_QuickSandTexturePaths = 0;
zModel_MaterialPartial *g_zDEClient_QuickSandMaterial = 0;
zModel_MaterialPartial *g_zDEClient_QuickSandMaterialCycle = 0;
zDEClient_QSandEventTemplate g_zDEClient_QuickSandEventTemplateDefaults = {0};
int g_zDEClient_CraterDisplaySourceCount = 0;
zDEClient_CraterDisplaySourceEntry *g_zDEClient_CraterDisplaySourceList = 0;
zDEClient_CraterEventTemplate g_zDEClient_CraterEventTemplateDefaults = {0};
zReader::Node *g_zDEClient_ConfigReaderRoot = 0;
int g_zDEClient_RebuildBltRectOnReload = 1;
zDEClient_FeatureEntry *g_zDEClient_FeatureListBegin = 0;
zDEClient_FeatureEntry *g_zDEClient_FeatureListEnd = 0;
zDEClient_FeatureEntry *g_zDEClient_FeatureListCapacityEnd = 0;
zDEClient_MapTreeState g_zDEClient_FeatureMapTree = {0};
zDEClient_MapTreeNode *g_zDEClient_FeatureMapTreeNil = 0;
int g_zDEClient_FeatureMapTreeNilRefCount = 0;
zClass_NodePartial *g_zDEClient_CameraNode = 0;
zClass_CameraDataPartial *g_zDEClient_CameraNodeClassData = 0;
zDEClient_NetRelayCallback g_zDEClientQSandNetRelayCallback = 0;
zDEClient_NetRelayCallback g_zDEClientCraterNetRelayCallback = 0;

namespace {
template <typename T> zZbdSectionCallback ZbdCallbackPtr(T callback) {
    RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T callback;
        zZbdSectionCallback raw;
    } value = {0};
    value.callback = callback;
    return value.raw;
}

bool IsNil(const zDEClient_MapTreeNode *node) {
    return node == 0 || node == g_zDEClient_FeatureMapTreeNil;
}

zDEClient_MapTreeNode *TreeMinimum(zDEClient_MapTreeNode *node) {
    while (!IsNil(node->left)) {
        node = node->left;
    }

    return node;
}

zDEClient_MapTreeNode *TreeMaximum(zDEClient_MapTreeNode *node) {
    while (!IsNil(node->right)) {
        node = node->right;
    }

    return node;
}

void RotateTreeLeft(zDEClient_MapTreeState *tree, zDEClient_MapTreeNode *node) {
    zDEClient_MapTreeNode *const pivot = node->right;
    node->right = pivot->left;
    if (!IsNil(pivot->left)) {
        pivot->left->parent = node;
    }

    pivot->parent = node->parent;
    if (node == tree->header->parent) {
        tree->header->parent = pivot;
    } else if (node == node->parent->left) {
        node->parent->left = pivot;
    } else {
        node->parent->right = pivot;
    }

    pivot->left = node;
    node->parent = pivot;
}

void RotateTreeRight(zDEClient_MapTreeState *tree, zDEClient_MapTreeNode *node) {
    zDEClient_MapTreeNode *const pivot = node->left;
    node->left = pivot->right;
    if (!IsNil(pivot->right)) {
        pivot->right->parent = node;
    }

    pivot->parent = node->parent;
    if (node == tree->header->parent) {
        tree->header->parent = pivot;
    } else if (node == node->parent->right) {
        node->parent->right = pivot;
    } else {
        node->parent->left = pivot;
    }

    pivot->right = node;
    node->parent = pivot;
}

void ResetHeader(zDEClient_MapTreeState *tree) {
    if (tree->header == 0) {
        return;
    }

    tree->header->parent = g_zDEClient_FeatureMapTreeNil;
    tree->header->left = tree->header;
    tree->header->right = tree->header;
}

void Transplant(zDEClient_MapTreeState *tree, zDEClient_MapTreeNode *oldNode,
                zDEClient_MapTreeNode *newNode) {
    if (oldNode->parent == tree->header) {
        tree->header->parent = newNode;
    } else if (oldNode == oldNode->parent->left) {
        oldNode->parent->left = newNode;
    } else {
        oldNode->parent->right = newNode;
    }

    if (!IsNil(newNode)) {
        newNode->parent = oldNode->parent;
    }
}

void RefreshHeaderExtents(zDEClient_MapTreeState *tree) {
    zDEClient_MapTreeNode *const root = tree->header != 0 ? tree->header->parent : 0;
    if (tree->nodeCount <= 0 || IsNil(root)) {
        ResetHeader(tree);
        return;
    }

    tree->header->left = TreeMinimum(root);
    tree->header->right = TreeMaximum(root);
}

void EnsureFeatureMapTreeInitialized(zDEClient_MapTreeState *tree) {
    if (g_zDEClient_FeatureMapTreeNil == 0) {
        g_zDEClient_FeatureMapTreeNil =
            static_cast<zDEClient_MapTreeNode *>(::operator new(sizeof(zDEClient_MapTreeNode)));
        g_zDEClient_FeatureMapTreeNil->left = g_zDEClient_FeatureMapTreeNil;
        g_zDEClient_FeatureMapTreeNil->parent = g_zDEClient_FeatureMapTreeNil;
        g_zDEClient_FeatureMapTreeNil->right = g_zDEClient_FeatureMapTreeNil;
        g_zDEClient_FeatureMapTreeNil->key = 0;
        g_zDEClient_FeatureMapTreeNil->colorOrNil = 1;
        g_zDEClient_FeatureMapTreeNilRefCount = 1;
    }

    if (tree->header == 0) {
        tree->header =
            static_cast<zDEClient_MapTreeNode *>(::operator new(sizeof(zDEClient_MapTreeNode)));
        tree->header->left = tree->header;
        tree->header->parent = g_zDEClient_FeatureMapTreeNil;
        tree->header->right = tree->header;
        tree->header->key = 0;
        tree->header->colorOrNil = 0;
        tree->allowInsert = 0;
        tree->nodeCount = 0;
    }
}

int zReaderArrayCount(zReader::Node *node) {
    if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY || node->value.nodes == 0) {
        return 0;
    }

    return node->value.nodes[0].value.i32;
}

char *zReaderArrayString(zReader::Node *node, int index) {
    return node->value.nodes[index].value.str;
}
} // namespace

namespace zDEClient_Crater {
// Reimplements 0x456ad0: zDEClient_Crater::DestroyFeature
// (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c)
RECOIL_NOINLINE void RECOIL_FASTCALL DestroyFeature(zDEClient_CraterFeature *featureInstance) {
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

// Reimplements 0x456b00: zDEClient_Crater::InitEventTemplateDefaults
// (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c)
RECOIL_NOINLINE void RECOIL_FASTCALL
InitEventTemplateDefaults(zDEClient_CraterEventTemplate *eventTemplate) {
    memcpy(eventTemplate, &g_zDEClient_CraterEventTemplateDefaults,
                sizeof(zDEClient_CraterEventTemplate));
}

// Reimplements 0x457040:
// zDEClient_Crater::CreateFeatureStructFromEventTemplate
// (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c)
RECOIL_NOINLINE zDEClient_CraterFeature *RECOIL_FASTCALL
CreateFeatureStructFromEventTemplate(zDEClient_CraterEventTemplate *eventTemplate) {
    zDEClient_CraterFeature *result =
        static_cast<zDEClient_CraterFeature *>(malloc(sizeof(zDEClient_CraterFeature)));
    memset(result, 0, sizeof(zDEClient_CraterFeature));

    result->featureType = 1;
    memcpy(&result->eventTemplate, eventTemplate, sizeof(result->eventTemplate));
    result->points = static_cast<zVec3 *>(
        malloc(static_cast<size_t>(result->eventTemplate.pointCount) * sizeof(zVec3)));
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

// Reimplements 0x456c80:
// zDEClient_Crater::InitFeatureFromEventTemplate
// (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c)
RECOIL_NOINLINE zDEClient_CraterFeature *RECOIL_FASTCALL
InitFeatureFromEventTemplate(zDEClient_CraterEventTemplate *eventTemplate) {
    zDEClient_CraterFeature *featureInstance = CreateFeatureStructFromEventTemplate(eventTemplate);
    zVec3 *currentPoint = featureInstance->points;

    zClass_NodePartial *world = zDEClient::GetCameraNode();
    zClass_WorldDataPartial *worldData = static_cast<zClass_WorldDataPartial *>(world->classData);
    if (worldData == 0) {
        return 0;
    }

    int gridCol;
    int gridRow;
    zClass_World::WorldToGridCoordsClamped(world, &gridCol, eventTemplate->center.x,
                                           eventTemplate->center.z, &gridRow);

    zDEClient_FeatureGridCell *featureGridCell = zDEClient::GetFeatureGridCell(gridCol, gridRow);
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
    const float angleStep = static_cast<float>(6.2831853071800001 / pointCount);
    for (int i = 0; i < pointCount; ++i) {
        currentPoint->x = static_cast<float>(sin(static_cast<double>(angle)));
        currentPoint->z = static_cast<float>(cos(static_cast<double>(angle)));

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
                if (strcmp(node->name, "ZDEC_FEATURE") == 0) {
                    zDEClient_FeatureContextOverlapView *context =
                        (zDEClient_FeatureContextOverlapView *)(
                            node->callbackContext);
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

// Reimplements 0x4570e0: zDEClient_Crater::Build
// (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c)
RECOIL_NOINLINE int RECOIL_FASTCALL Build(zDEClient_CraterFeature *featureInstance) {
    int result = zGeometry_Model::ClipPatch(
        featureInstance->eventTemplate.pointCount, featureInstance->points,
        featureInstance->featureGridCell, featureInstance->clipPatchOutput);

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

// Reimplements 0x457140: zDEClient_Crater::CreateFeature
// (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
CreateFeature(zDEClient_CraterFeature *featureInstance) {
    zClass_NodePartial *node = 0;
    zDiPartial *displayInstance = zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(
        featureInstance->clipPatchOutput->partitions, zDEClient::GetCameraNode(), &node);
    if (node != 0) {
        zClass_Class::gwNodeSetName(node, "ZDEC_FEATURE");
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
        uvPairs = static_cast<zClipUV *>(
            malloc(static_cast<size_t>(uvCenterIndex + 1) * sizeof(zClipUV)));
    }

    zVec3 *const midPoints =
        static_cast<zVec3 *>(malloc(static_cast<size_t>(pointCount) * sizeof(zVec3)));

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

        zGeometry_Model::AddPolygonToDi(displayInstance, 4, polygonPoints, polygonMaterial, uvList);
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

        zGeometry_Model::AddPolygonToDi(displayInstance, 3, polygonPoints, polygonMaterial, uvList);
    }

    free(midPoints);
    if (hasMaterialUv) {
        free(uvPairs);
    }

    zClass_Class::gwNodeSetDisplayInstance(node, displayInstance);
    return 0;
}

// Reimplements 0x456b20: zDEClient_Crater::InstanceEvent
// (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
InstanceEvent(zDEClient_CraterEventTemplate *eventTemplate, int playEffectAnim) {
    const float vertexMergeEpsilon = zModel_Const::GetVertexMergeEpsilon();
    zModel_Const::SetVertexMergeEpsilon(0.00499999989f);

    zDEClient_CraterFeature *const featureInstance = InitFeatureFromEventTemplate(eventTemplate);
    if (featureInstance == 0) {
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_crater.cpp", 0x8b,
                          "Failed to instance crater: Build Failed");
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    if (Build(featureInstance) == 0) {
        DestroyFeature(featureInstance);
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_crater.cpp", 0xc3,
                          "Failed to instance crater: Clip Failed");
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    if (CreateFeature(featureInstance) != 0) {
        DestroyFeature(featureInstance);
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_crater.cpp", 0xd2,
                          "Failed to instance crater: Tesselation Failed");
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    zDEClient::AppendFeatureEntry(1, eventTemplate);
    zDEClient::SubmitFeatureGeometry(featureInstance->clipPatchOutput);
    zGeometry_ClipPatchOutput::ApplyNodeDiPairs(featureInstance->clipPatchOutput);
    zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);

    if (playEffectAnim != 0 && featureInstance->displaySourceEntry != 0 &&
        featureInstance->displaySourceEntry->effectAnimEntry != 0) {
        zEffectAnim::SetTransformRotAndVelocity_Thunk(
            featureInstance->displaySourceEntry->effectAnimEntry, 0,
            featureInstance->eventTemplate.center.x, featureInstance->eventTemplate.center.y,
            featureInstance->eventTemplate.center.z, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    }

    return 0;
}

// Reimplements 0x456c50: zDEClient_Crater::InstanceEventMaybeRelay
// (D:\Proj\GameZRecoil\zDEClient\zdec_crater.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
InstanceEventMaybeRelay(zDEClient_CraterEventTemplate *eventTemplate) {
    if (g_zDEClientCraterNetRelayCallback != 0 && g_zDEClientCraterNetRelayCallback() == 0) {
        return -1;
    }

    return InstanceEvent(eventTemplate, 1);
}
} // namespace zDEClient_Crater

namespace zDEClient_QSand {
// Reimplements 0x455ea0: zDEClient_QSand::DestroyFeature
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.c)
RECOIL_NOINLINE void RECOIL_FASTCALL DestroyFeature(zDEClient_QSandFeature *featureInstance) {
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

// Reimplements 0x4563d0:
// zDEClient_QSand::CreateFeatureStructFromEventTemplate
// (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c)
RECOIL_NOINLINE zDEClient_QSandFeature *RECOIL_FASTCALL
CreateFeatureStructFromEventTemplate(zDEClient_QSandEventTemplate *eventTemplate) {
    zDEClient_QSandFeature *result =
        static_cast<zDEClient_QSandFeature *>(malloc(sizeof(zDEClient_QSandFeature)));
    memset(result, 0, sizeof(zDEClient_QSandFeature));

    result->featureType = 3;
    memcpy(&result->eventTemplate, eventTemplate, sizeof(result->eventTemplate));
    result->points =
        static_cast<zVec3 *>(malloc(result->eventTemplate.pointCount * sizeof(zVec3)));
    result->clipPatchOutput = zGeometry_ClipPatchOutput::Create();

    if ((result->eventTemplate.featureFlags & 0x1008) != 0 &&
        result->eventTemplate.material == 0) {
        result->eventTemplate.material = g_zDEClient_QuickSandMaterial;
        result->eventTemplate.materialCycle = g_zDEClient_QuickSandMaterialCycle;
    }

    return result;
}

// Reimplements 0x456010:
// zDEClient_QSand::InitFeatureFromEventTemplate
// (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c)
RECOIL_NOINLINE zDEClient_QSandFeature *RECOIL_FASTCALL
InitFeatureFromEventTemplate(zDEClient_QSandEventTemplate *eventTemplate) {
    zDEClient_QSandFeature *featureInstance = CreateFeatureStructFromEventTemplate(eventTemplate);
    zVec3 *currentPoint = featureInstance->points;

    zClass_NodePartial *world = zDEClient::GetCameraNode();
    zClass_WorldDataPartial *worldData = static_cast<zClass_WorldDataPartial *>(world->classData);
    if (worldData == 0) {
        return 0;
    }

    int gridCol;
    int gridRow;
    zClass_World::WorldToGridCoordsClamped(world, &gridCol, eventTemplate->center.x,
                                           eventTemplate->center.z, &gridRow);

    zDEClient_FeatureGridCell *featureGridCell = zDEClient::GetFeatureGridCell(gridCol, gridRow);
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
    const float angleStep = static_cast<float>(6.2831853071800001 / pointCount);
    for (int i = 0; i < pointCount; ++i) {
        currentPoint->x = static_cast<float>(sin(static_cast<double>(angle)));
        currentPoint->z = static_cast<float>(cos(static_cast<double>(angle)));

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
                if (strcmp(node->name, "ZDEC_FEATURE") == 0) {
                    zDEClient_FeatureContextOverlapView *context =
                        (zDEClient_FeatureContextOverlapView *)(
                            node->callbackContext);
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

// Reimplements 0x456450: zDEClient_QSand::Build
// (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c)
RECOIL_NOINLINE int RECOIL_FASTCALL Build(zDEClient_QSandFeature *featureInstance) {
    int result = zGeometry_Model::ClipPatch(
        featureInstance->eventTemplate.pointCount, featureInstance->points,
        featureInstance->featureGridCell, featureInstance->clipPatchOutput);

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

// Reimplements 0x4564b0: zDEClient_QSand::CreateFeature
// (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
CreateFeature(zDEClient_QSandFeature *featureInstance) {
    zClass_NodePartial *node = 0;
    zDiPartial *displayInstance = zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(
        featureInstance->clipPatchOutput->partitions, zDEClient::GetCameraNode(), &node);
    if (displayInstance == 0 || node == 0) {
        if (displayInstance != 0) {
            zModel_DiPool::FreeIfUnreferenced(displayInstance);
        }

        if (node != 0) {
            zClass_Object3D::DeleteNode(node);
        }

        return -1;
    }

    zClass_Class::gwNodeSetName(node, "ZDEC_FEATURE");
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
        uvPairs = static_cast<zClipUV *>(
            malloc(static_cast<size_t>(uvCenterIndex + 1) * sizeof(zClipUV)));
    }

    zVec3 *const midPoints =
        static_cast<zVec3 *>(malloc(static_cast<size_t>(pointCount) * sizeof(zVec3)));

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
        zClipUV *uvList = 0;
        zModel_MaterialPartial *material = 0;
        if (hasMaterialUv) {
            polygonUvs[0] = uvPairs[uvCenterIndex];
            polygonUvs[1] = uvPairs[pointCount + i_860];
            polygonUvs[2] = uvPairs[pointCount + nextIndex];
            uvList = polygonUvs;
            material = featureInstance->eventTemplate.materialCycle;
        }

        zGeometry_Model::AddPolygonToDi(displayInstance, 3, polygonPoints, material, uvList);
    }

    zClass_NodePartial *capNode = 0;
    zDiPartial *const capDisplayInstance = zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition(
        featureInstance->clipPatchOutput->partitions, zDEClient::GetCameraNode(), &capNode);
    if (capDisplayInstance == 0 || capNode == 0) {
        if (capDisplayInstance != 0) {
            zModel_DiPool::FreeIfUnreferenced(capDisplayInstance);
        }

        if (capNode != 0) {
            zClass_Object3D::DeleteNode(capNode);
        }

        return -1;
    }

    zClass_Class::gwNodeSetName(capNode, "ZDEC_FEATURE");
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

        zGeometry_Model::AddPolygonToDi(capDisplayInstance, 3, polygonPoints, material, uvList);
    }

    free(midPoints);
    if (hasMaterialUv) {
        free(uvPairs);
    }

    return 0;
}

// Reimplements 0x455ef0: zDEClient_QSand::InstanceEventMaybeRelay
// (D:\Proj\GameZRecoil\zDEClient\zdec_qsand.c)
RECOIL_NOINLINE int RECOIL_FASTCALL
InstanceEventMaybeRelay(zDEClient_QSandEventTemplate *eventTemplate) {
    if (g_zDEClientQSandNetRelayCallback != 0 && g_zDEClientQSandNetRelayCallback() == 0) {
        return -1;
    }

    if (g_zDEClient_QuickSandEnabled == 0) {
        return -1;
    }

    const float vertexMergeEpsilon = zModel_Const::GetVertexMergeEpsilon();
    zModel_Const::SetVertexMergeEpsilon(0.00499999989f);

    zDEClient_QSandFeature *const featureInstance = InitFeatureFromEventTemplate(eventTemplate);
    if (featureInstance == 0) {
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_qsand.cpp", 0x81,
                          "Failed to instance quick sand: Build Failed");
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    if (Build(featureInstance) == 0) {
        DestroyFeature(featureInstance);
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_qsand.cpp", 0x92,
                          "Failed to instance quick sand: Clip Failed");
        zModel_Const::SetVertexMergeEpsilon(vertexMergeEpsilon);
        return -1;
    }

    if (CreateFeature(featureInstance) != 0) {
        DestroyFeature(featureInstance);
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_qsand.cpp", 0xa1,
                          "Failed to instance quick sand: Tesselation Failed");
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

namespace zGeometry_ClipPatchOutput {
// Reimplements 0x46af00: zGeometry_ClipPatchOutput::Create
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE zGeometry_ClipPatchOutputPartial *RECOIL_CDECL Create() {
    zGeometry_ClipPatchOutputPartial *result = static_cast<zGeometry_ClipPatchOutputPartial *>(
        malloc(sizeof(zGeometry_ClipPatchOutputPartial)));
    result->pointCount = 0;
    result->points = 0;
    result->partitionCount = 0;
    result->partitions = 0;
    return result;
}

// Reimplements 0x46af20: zGeometry_ClipPatchOutput::Destroy
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL Destroy(zGeometry_ClipPatchOutputPartial *self) {
    if (self->partitions != 0) {
        free(self->partitions);
    }

    free(self);
}

// Reimplements 0x46ae40: zGeometry_ClipPatchOutput::ApplyNodeDiPairs
// (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
ApplyNodeDiPairs(zGeometry_ClipPatchOutputPartial *self) {
    {
    for (int partitionIndex = 0; partitionIndex < self->partitionCount; ++partitionIndex) {
        zGeometry_ClipPatchPartitionOutput *const partition = &self->partitions[partitionIndex];
        for (int i = 0; i < partition->nodeDiPairCount; ++i) {
            zGeometry_ClipPatchNodeDiPair *const pair = &partition->nodeDiPairs[i];

            unsigned int oldDisplayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(pair->node, &oldDisplayInstanceValue);
            zClass_Class::gwNodeSetDisplayInstance(pair->node, pair->di);

            if (oldDisplayInstanceValue != 0) {
                zModel_DiPool::FreeIfUnreferenced((zDiPartial *)(
                    static_cast<unsigned int>(oldDisplayInstanceValue)));
            }
        }

        ++partition->featureGridCell->featureCount;

        if (partition->nodeDiPairs != 0) {
            free(partition->nodeDiPairs);
            partition->nodeDiPairs = 0;
            partition->nodeDiPairCount = 0;
        }
    }
    }

    return 0;
}
} // namespace zGeometry_ClipPatchOutput

namespace zDEClient {
// Reimplements 0x46af40:
// zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE zDiPartial *RECOIL_FASTCALL CreateFeatureNodeAndDiFromClipPatchPartition(
    zGeometry_ClipPatchPartitionOutput *partitionOutput, zClass_NodePartial *parentNode,
    zClass_NodePartial **outNode) {
    if (partitionOutput == 0) {
        return 0;
    }

    zClass_NodePartial *child = zClass_Object3D::gwObject3DInit();
    if (child == 0) {
        if (outNode != 0) {
            *outNode = child;
        }

        return 0;
    }

    if (outNode != 0) {
        *outNode = child;
    }

    zClass_Class::gwNodeSetNodeType(child, 0xff);

    for (int i = 0; i < partitionOutput->nodeDiPairCount; ++i) {
        zGeometry_ClipPatchNodeView *const node = partitionOutput->nodeDiPairs[i].node;
        if ((node->flags & 0x10000) == 0) {
            continue;
        }

        int nodeType;
        zClass_Class::gwNodeGetNodeType(node, &nodeType);
        if (nodeType != 0xff) {
            zClass_Class::gwNodeSetNodeType(child, nodeType);
            break;
        }
    }

    zClass_Class::gwNodeSetFlag17(child, 1);

    zDiPartial *const displayInstance = zModel_DiPool::AllocFromFreeList();
    if (displayInstance == 0) {
        if (outNode != 0) {
            *outNode = 0;
        }

        zClass_Object3D::DeleteNode(child);
        return 0;
    }

    zClass_Class::AddChild(parentNode, child);
    zClass_Class::gwNodeSetDisplayInstance(child, displayInstance);
    return displayInstance;
}
} // namespace zDEClient

// Reimplements 0x4588c0: zDEClient_MapTreeState::IterNextNodeRef
RECOIL_NOINLINE zDEClient_MapTreeNode **RECOIL_THISCALL
zDEClient_MapTreeState::IterNextNodeRef(zDEClient_MapTreeNode **nodeRef) {
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

// Reimplements 0x458970: zDEClient_MapTreeState::IterPrevNodeRef
RECOIL_NOINLINE void RECOIL_THISCALL
zDEClient_MapTreeState::IterPrevNodeRef(zDEClient_MapTreeNode **nodeRef) {
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

// Reimplements 0x4585a0: zDEClient_MapTreeState::InsertAt
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE zDEClient_MapTreeNode **RECOIL_THISCALL zDEClient_MapTreeState::InsertAt(
    zDEClient_MapTreeNode **outNode, zDEClient_MapTreeNode *where,
    zDEClient_MapTreeNode *parent, zGeometry_ClipPatchNodeDiPair *key) {
    zDEClient_MapTreeNode *inserted =
        static_cast<zDEClient_MapTreeNode *>(::operator new(sizeof(zDEClient_MapTreeNode)));
    inserted->left = g_zDEClient_FeatureMapTreeNil;
    inserted->parent = parent;
    inserted->right = g_zDEClient_FeatureMapTreeNil;
    inserted->key = key->node;
    inserted->colorOrNil = 0;

    ++nodeCount;

    if (parent != header && where == g_zDEClient_FeatureMapTreeNil &&
        !(key->node < parent->key)) {
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
                    RotateTreeLeft(this, fixup);
                }

                fixup->parent->colorOrNil = 1;
                fixup->parent->parent->colorOrNil = 0;
                RotateTreeRight(this, fixup->parent->parent);
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
                    RotateTreeRight(this, fixup);
                }

                fixup->parent->colorOrNil = 1;
                fixup->parent->parent->colorOrNil = 0;
                RotateTreeLeft(this, fixup->parent->parent);
            }
        }
    }

    header->parent->colorOrNil = 1;
    *outNode = inserted;
    return outNode;
}

// Reimplements 0x458510: zDEClient_MapTreeState::DestroySubtree
RECOIL_NOINLINE void RECOIL_THISCALL
zDEClient_MapTreeState::DestroySubtree(zDEClient_MapTreeNode *node) {
    while (!IsNil(node)) {
        DestroySubtree(node->right);
        zDEClient_MapTreeNode *const left = node->left;
        ::operator delete(node);
        node = left;
    }
}

// Reimplements 0x457fe0: zDEClient_MapTreeState::EraseAndAdvance
RECOIL_NOINLINE zDEClient_MapTreeNode **RECOIL_THISCALL zDEClient_MapTreeState::EraseAndAdvance(
    zDEClient_MapTreeNode **outNext, zDEClient_MapTreeNode *node) {
    zDEClient_MapTreeNode *next = node;
    IterNextNodeRef(&next);

    if (IsNil(node->left)) {
        Transplant(this, node, node->right);
    } else if (IsNil(node->right)) {
        Transplant(this, node, node->left);
    } else {
        zDEClient_MapTreeNode *successor = TreeMinimum(node->right);
        if (successor->parent != node) {
            Transplant(this, successor, successor->right);
            successor->right = node->right;
            successor->right->parent = successor;
        }

        Transplant(this, node, successor);
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

// Reimplements 0x457e80: zDEClient_MapTreeState::EraseRange
RECOIL_NOINLINE zDEClient_MapTreeNode **RECOIL_THISCALL zDEClient_MapTreeState::EraseRange(
    zDEClient_MapTreeNode **outNext, zDEClient_MapTreeNode *first, zDEClient_MapTreeNode *last) {
    if (nodeCount != 0 && first == header->left && last == header) {
        DestroySubtree(header->parent);
        nodeCount = 0;
        ResetHeader(this);
        *outNext = header->left;
        return outNext;
    }

    while (first != last) {
        EraseAndAdvance(&first, first);
    }

    *outNext = first;
    return outNext;
}

// Reimplements 0x457d90: zDEClient_MapTreeState::FindOrInsertKey
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE zDEClient_MapTreeLocateResult *RECOIL_THISCALL
zDEClient_MapTreeState::FindOrInsertKey(zDEClient_MapTreeLocateResult *outResult,
                                        zGeometry_ClipPatchNodeDiPair *key) {
    EnsureFeatureMapTreeInitialized(this);

    zGeometry_ClipPatchNodeView *const nodeKey = key->node;
    zDEClient_MapTreeNode *parent = header;
    zDEClient_MapTreeNode *cursor = header->parent;
    unsigned char insertLeft = 1;

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

    if (allowInsert != 0) {
        zDEClient_MapTreeNode *inserted = 0;
        InsertAt(&inserted, cursor, parent, key);
        outResult->node = inserted;
        outResult->inserted = 1;
        return outResult;
    }

    zDEClient_MapTreeNode *candidate = parent;
    if (insertLeft != 0) {
        if (parent == header->left) {
            zDEClient_MapTreeNode *inserted = 0;
            InsertAt(&inserted, cursor, parent, key);
            outResult->node = inserted;
            outResult->inserted = 1;
            return outResult;
        }

        candidate = parent;
        IterPrevNodeRef(&candidate);
    }

    if (candidate->key < nodeKey) {
        zDEClient_MapTreeNode *inserted = 0;
        InsertAt(&inserted, cursor, parent, key);
        outResult->node = inserted;
        outResult->inserted = 1;
        return outResult;
    }

    outResult->node = candidate;
    outResult->inserted = 0;
    return outResult;
}

namespace zDEClient {
// Reimplements 0x455ed0: zDEClient::CopyQSandEventTemplateDefaults
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.c)
RECOIL_NOINLINE void RECOIL_FASTCALL
CopyQSandEventTemplateDefaults(zDEClient_QSandEventTemplate *eventTemplate) {
    memcpy(eventTemplate, &g_zDEClient_QuickSandEventTemplateDefaults,
           sizeof(zDEClient_QSandEventTemplate));
}

// Reimplements 0x4558f0: zDEClient::LoadConfigResources
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL LoadConfigResources(zClass_NodePartial *worldNode) {
    int textureLoadPending = 0;
    if (worldNode == 0) {
        zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_init.cpp", 0x44,
                          "Failed to DEClient: world node is NULL.");
        return -1;
    }

    zGame::ReturnOnlyStub();
    SetCameraNode(worldNode);
    zVideo::ReturnSuccessStub();

    g_zDEClient_ConfigReaderRoot = zReader::LoadNodeFromPath("declient.zrd", 0, 0);
    srand(static_cast<unsigned int>(time(0)));

    if (g_zDEClient_ConfigReaderRoot == 0) {
        zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_init.cpp", 0x57,
                          "Failed to read (%s), using defaults", "declient.zrd");
    }

    zReader::Node *const craterNode = zReader_GetNamedNode(g_zDEClient_ConfigReaderRoot, "CRATER");
    g_zDEClient_CraterEventTemplateDefaults.featureFlags = 0x100c;
    g_zDEClient_CraterEventTemplateDefaults.pointCount = 7;
    g_zDEClient_CraterEventTemplateDefaults.slope = 0.0f;
    g_zDEClient_CraterEventTemplateDefaults.depth = 4.0f;
    g_zDEClient_CraterEventTemplateDefaults.radius = 20.0f;

    zReader::ReadNamedInt(craterNode, "POINTS",
                          &g_zDEClient_CraterEventTemplateDefaults.pointCount);
    zReader::ReadNamedFloat(craterNode, "SLOPE", &g_zDEClient_CraterEventTemplateDefaults.slope);
    zReader::ReadNamedFloat(craterNode, "DEPTH", &g_zDEClient_CraterEventTemplateDefaults.depth);
    zReader::ReadNamedFloat(craterNode, "RADIUS", &g_zDEClient_CraterEventTemplateDefaults.radius);

    g_zDEClient_CraterDisplaySourceCount = 1;
    g_zDEClient_CraterDisplaySourceList = static_cast<zDEClient_CraterDisplaySourceEntry *>(
        calloc(1, sizeof(zDEClient_CraterDisplaySourceEntry)));

    zDEClient_CraterDisplaySourceEntry *defaultDisplaySource = g_zDEClient_CraterDisplaySourceList;
    if (LoadMaterialFromTexturePath_Local(
            &defaultDisplaySource->craterMaterial,
            const_cast<char *>(zReader::ReadNamedString(craterNode, "DEFAULT_TEXTURE"))) != 0) {
        textureLoadPending = 1;
    }

    defaultDisplaySource->effectAnimEntry =
        zEffectAnim::FindEntryByName(zReader::ReadNamedString(craterNode, "DEFAULT_ANIM"));

    zReader::Node *const textureAnimNode = zReader_GetNamedNode(craterNode, "TEXTURE_ANIM");
    if (textureAnimNode != 0) {
        const int textureAnimCount = zReaderArrayCount(textureAnimNode);
        const int additionalDisplaySourceCount = (textureAnimCount - 1) / 2;
        g_zDEClient_CraterDisplaySourceCount += additionalDisplaySourceCount;

        g_zDEClient_CraterDisplaySourceList = static_cast<zDEClient_CraterDisplaySourceEntry *>(
            realloc(g_zDEClient_CraterDisplaySourceList,
                         static_cast<size_t>(g_zDEClient_CraterDisplaySourceCount) *
                             sizeof(zDEClient_CraterDisplaySourceEntry)));

        zDEClient_CraterDisplaySourceEntry *displaySource = &g_zDEClient_CraterDisplaySourceList[1];
        for (int i = 1; i < textureAnimCount; i += 2) {
            char *const sourceTexturePath = zReaderArrayString(textureAnimNode, i);
            if (LoadMaterialFromTexturePath_Local(&displaySource->sourceMaterial,
                                                  sourceTexturePath) != 0) {
                textureLoadPending = 1;
            }

            zReader::Node *const entryNode =
                zReader_GetNamedNode(textureAnimNode, sourceTexturePath);
            if (entryNode != 0) {
                if (LoadMaterialFromTexturePath_Local(&displaySource->craterMaterial,
                                                      zReaderArrayString(entryNode, 1)) != 0) {
                    textureLoadPending = 1;
                }

                if (zReaderArrayCount(entryNode) > 2) {
                    displaySource->effectAnimEntry =
                        zEffectAnim::FindEntryByName(zReaderArrayString(entryNode, 2));
                } else {
                    displaySource->effectAnimEntry =
                        g_zDEClient_CraterDisplaySourceList[0].effectAnimEntry;
                }
            }

            ++displaySource;
        }
    }

    zReader::Node *const quickSandNode =
        zReader_GetNamedNode(g_zDEClient_ConfigReaderRoot, "QUICK_SAND");
    if (quickSandNode == 0) {
        g_zDEClient_QuickSandEnabled = 0;
    } else {
        zReader::Node *const defaultTextureNode =
            zReader_GetNamedNode(quickSandNode, "DEFAULT_TEXTURE");
        int textureCount = 1;
        if (defaultTextureNode != 0) {
            zReader::Node *const defaultTextureArray = defaultTextureNode->value.nodes;
            g_zDEClient_QuickSandAnimSpeed = defaultTextureArray[1].value.f32;
            textureCount = defaultTextureArray[0].value.i32 - 2;
            g_zDEClient_QuickSandTextureCount = textureCount;

            if (textureCount > 0) {
                g_zDEClient_QuickSandTexturePaths = static_cast<char **>(
                    malloc(static_cast<size_t>(textureCount) * sizeof(char *)));
                for (int i = 0; i < textureCount; ++i) {
                    g_zDEClient_QuickSandTexturePaths[i] = defaultTextureArray[i + 2].value.str;
                }
            } else {
                g_zDEClient_QuickSandTexturePaths = 0;
            }
        }

        g_zDEClient_QuickSandEventTemplateDefaults.featureFlags = 0x1008;
        g_zDEClient_QuickSandMaterial = 0;
        g_zDEClient_QuickSandMaterialCycle = 0;
        g_zDEClient_QuickSandEventTemplateDefaults.pointCount = 7;
        g_zDEClient_QuickSandEventTemplateDefaults.slope = 40.0f;
        g_zDEClient_QuickSandEventTemplateDefaults.depth = 4.0f;
        g_zDEClient_QuickSandEventTemplateDefaults.radius = 20.0f;

        zReader::ReadNamedInt(quickSandNode, "POINTS",
                              &g_zDEClient_QuickSandEventTemplateDefaults.pointCount);
        zReader::ReadNamedFloat(quickSandNode, "SLOPE",
                                &g_zDEClient_QuickSandEventTemplateDefaults.slope);
        zReader::ReadNamedFloat(quickSandNode, "DEPTH",
                                &g_zDEClient_QuickSandEventTemplateDefaults.depth);
        zReader::ReadNamedFloat(quickSandNode, "RADIUS",
                                &g_zDEClient_QuickSandEventTemplateDefaults.radius);

        zModel_MaterialPartial material;
        zModel_Material::ResetDefaults(&material);
        material.flags = static_cast<unsigned short>(material.flags | 0x0100);

        if (textureCount > 1) {
            zModel_Material::SetCycleTextureCount(&material, textureCount);
            zModel_Material::SetCycleTextureSpeed(&material, g_zDEClient_QuickSandAnimSpeed);
            zModel_Material::SetCycleTextureLoop(&material, 1);

            for (int i = 0; i < textureCount; ++i) {
                zModel_Material::AddCycleTexture(
                    &material,
                    zImage::TexDir_FindOrAppendByPath(g_zDEClient_QuickSandTexturePaths[i]));
            }

            textureLoadPending = 1;
            g_zDEClient_QuickSandMaterial = zModel_Material::FindOrClone(&material);
        } else if (textureCount == 1 && g_zDEClient_QuickSandTexturePaths != 0) {
            zImage::TexDir_FindOrAppendByPath(g_zDEClient_QuickSandTexturePaths[0]);
            textureLoadPending = 1;
            g_zDEClient_QuickSandMaterial = zModel_Material::FindOrClone(&material);
        } else {
            material.flags = static_cast<unsigned short>(material.flags & 0xfeff);
            g_zDEClient_QuickSandMaterial = 0;
            zError::ReportOld(0x100, "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_init.cpp", 0xef,
                              "Quick sand will NOT be textured");
        }

        if (g_zDEClient_QuickSandMaterial != 0) {
            zModel_Material::SetUserTag(g_zDEClient_QuickSandMaterial, 3);
        }

        if (textureCount >= 1 && g_zDEClient_QuickSandTexturePaths != 0) {
            zModel_Material::ResetDefaults(&material);
            material.flags = static_cast<unsigned short>(material.flags | 0x0100);
            material.currentTextureDirectoryEntry =
                zImage::FindTexDirEntryByName(g_zDEClient_QuickSandTexturePaths[0]);
            zModel_Material::SetUserTag(&material, 0);
            g_zDEClient_QuickSandMaterialCycle = zModel_Material::Clone(&material);
            g_zDEClient_QuickSandEnabled = 1;
        } else {
            g_zDEClient_QuickSandMaterialCycle = 0;
            g_zDEClient_QuickSandEnabled = 1;
        }
    }

    if (textureLoadPending != 0) {
        zImage::TexDir_LoadPendingEntries();
    }

    zReader::FreeLoadedTree(g_zDEClient_ConfigReaderRoot);
    const int rebuildBltRectOnReload = g_zDEClient_RebuildBltRectOnReload;
    g_zDEClient_ConfigReaderRoot = 0;

    if (rebuildBltRectOnReload != 0) {
        zUtil_ZAR::RegisterSectionHandler("zDEClient", ZbdCallbackPtr(&WriteFeatureSectionsToZAR),
                                          ZbdCallbackPtr(&ApplyFeatureEntry), 0x3e8, 0);
    }

    return 0;
}

// Reimplements 0x455dd0: zDEClient::LoadMaterialFromTexturePath_Local
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.c)
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL
LoadMaterialFromTexturePath_Local(zModel_MaterialPartial **outMaterial, char *texturePath) {
    int result = 0;
    zImage_TexDirEntryPartial *textureDirectoryEntry = zImage::FindTexDirEntryByName(texturePath);
    if (textureDirectoryEntry == 0) {
        textureDirectoryEntry = zImage::TexDir_FindOrAppendByPath(texturePath);
        result = 1;
        *outMaterial = 0;
    } else {
        *outMaterial = zModel_Material::FindByTexDirEntry(textureDirectoryEntry);
    }

    if (*outMaterial == 0) {
        zModel_MaterialPartial material;
        zModel_Material::ResetDefaults(&material);
        material.flags = static_cast<unsigned short>(material.flags | 0x0100);
        material.currentTextureDirectoryEntry = textureDirectoryEntry;
        *outMaterial = zModel_Material::FindOrClone(&material);
    }

    return result;
}

// Reimplements 0x458aa0: zDEClient::SetCameraNode
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL SetCameraNode(zClass_NodePartial *cameraNode) {
    if (cameraNode != 0) {
        g_zDEClient_CameraNode = cameraNode;
        g_zDEClient_CameraNodeClassData =
            static_cast<zClass_CameraDataPartial *>(cameraNode->classData);
    }
}

// Reimplements 0x458ac0: zDEClient::GetFeatureGridCell
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE zDEClient_FeatureGridCell *RECOIL_FASTCALL
GetFeatureGridCell(int gridCol, int gridRow) {
    if (g_zDEClient_CameraNodeClassData == 0) {
        return 0;
    }

    zDEClient_CameraNodeClassDataPartial *data =
        (zDEClient_CameraNodeClassDataPartial *)(g_zDEClient_CameraNodeClassData);
    return &data->featureGridRows[gridRow][gridCol];
}

// Reimplements 0x458ae0: zDEClient::GetCameraNode
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL GetCameraNode() {
    return g_zDEClient_CameraNode;
}

// Reimplements 0x458a30: zDEClient::CopyFeatureEntriesForward
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE zDEClient_FeatureEntry *RECOIL_STDCALL
CopyFeatureEntriesForward(zDEClient_FeatureEntry *first, zDEClient_FeatureEntry *last,
                          zDEClient_FeatureEntry *dest) {
    while (first != last) {
        if (dest != 0) {
            *dest = *first;
        }

        ++first;
        ++dest;
    }

    return dest;
}

// Reimplements 0x458a70: zDEClient::FillFeatureEntries
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE void RECOIL_STDCALL FillFeatureEntries(zDEClient_FeatureEntry *dest,
                                                       unsigned int count,
                                                       const zDEClient_FeatureEntry *value) {
    while (count != 0) {
        if (dest != 0) {
            *dest = *value;
        }

        ++dest;
        --count;
    }
}

// Reimplements 0x457840: zDEClient::AppendFeatureEntry
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL AppendFeatureEntry(int featureType,
                                                                const void *featureEventData) {
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
    memset(&featureEntry.eventData, 0, sizeof(featureEntry.eventData));
    memcpy(&featureEntry.eventData, featureEventData, eventDataBytes);
    featureEntry.reloadFlag = 0;

    if (g_zDEClient_FeatureListEnd == g_zDEClient_FeatureListCapacityEnd) {
        const ptrdiff_t oldCount =
            g_zDEClient_FeatureListBegin != 0
                ? g_zDEClient_FeatureListEnd - g_zDEClient_FeatureListBegin
                : 0;
        const ptrdiff_t capacityIncrement = oldCount > 1 ? oldCount : 1;
        const ptrdiff_t newCapacity = oldCount + capacityIncrement;

        zDEClient_FeatureEntry *const newEntries = static_cast<zDEClient_FeatureEntry *>(
            ::operator new(static_cast<size_t>(newCapacity) * sizeof(zDEClient_FeatureEntry)));

        CopyFeatureEntriesForward(g_zDEClient_FeatureListBegin, g_zDEClient_FeatureListEnd,
                                  newEntries);

        ::operator delete(g_zDEClient_FeatureListBegin);
        g_zDEClient_FeatureListBegin = newEntries;
        g_zDEClient_FeatureListEnd = newEntries + oldCount;
        g_zDEClient_FeatureListCapacityEnd = newEntries + newCapacity;
    }

    FillFeatureEntries(g_zDEClient_FeatureListEnd, 1, &featureEntry);
    ++g_zDEClient_FeatureListEnd;
    return 0;
}

// Reimplements 0x4575f0: zDEClient::SubmitFeatureGeometry
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
SubmitFeatureGeometry(zGeometry_ClipPatchOutputPartial *clipPatchOutput) {
    if (clipPatchOutput == 0) {
        return;
    }

    for (int partitionIndex = 0; partitionIndex < clipPatchOutput->partitionCount;
         ++partitionIndex) {
        zGeometry_ClipPatchPartitionOutput *const partition =
            &clipPatchOutput->partitions[partitionIndex];
        {
        for (int pairIndex = 0; pairIndex < partition->nodeDiPairCount; ++pairIndex) {
            zDEClient_MapTreeLocateResult result;
            g_zDEClient_FeatureMapTree.FindOrInsertKey(&result, &partition->nodeDiPairs[pairIndex]);
        }
        }
    }
}

// Reimplements 0x457b40: zDEClient::WriteFeatureSectionsToZAR
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
WriteFeatureSectionsToZAR(zZbdSectionCallbackCtx *callbackCtx) {
    int craterSectionIndex = 0;
    int qSandSectionIndex = 0;
    zDEClient_FeatureEntry featureEntry;
    featureEntry.reloadFlag = 1;

    int result =
        zUtil_ZAR::WriteSectionBlob(callbackCtx, "Dummy", &featureEntry, sizeof(featureEntry));

    for (zDEClient_FeatureEntry *entry = g_zDEClient_FeatureListBegin;
         entry != g_zDEClient_FeatureListEnd && result != 0; ++entry) {
        memcpy(&featureEntry, entry, sizeof(featureEntry));
        featureEntry.reloadFlag = 0;

        char sectionName[0x40];
        if (featureEntry.featureType == 1) {
            sprintf(sectionName, "Crater%d", craterSectionIndex);
            ++craterSectionIndex;
            result = zUtil_ZAR::WriteSectionBlob(callbackCtx, sectionName, &featureEntry,
                                                 sizeof(featureEntry));
        } else if (featureEntry.featureType == 3) {
            sprintf(sectionName, "QSand%d", qSandSectionIndex);
            ++qSandSectionIndex;
            result = zUtil_ZAR::WriteSectionBlob(callbackCtx, sectionName, &featureEntry,
                                                 sizeof(featureEntry));
        }
    }

    return result;
}

// Reimplements 0x457c10: zDEClient::ApplyFeatureEntry
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE void RECOIL_STDCALL ApplyFeatureEntry(zDEClient_FeatureEntry *container, void *,
                                                      void *) {
    if (container->reloadFlag != 0) {
        ClearFeatureDisplayNodes();
        ClearFeatureEntriesAndMapTree();
        return;
    }

    if (container->featureType == 1) {
        zDEClient_Crater::InstanceEvent(&container->eventData.crater, 0);
    } else if (container->featureType == 3) {
        zDEClient_QSand::InstanceEventMaybeRelay(&container->eventData.quickSand);
    }
}

// Reimplements 0x457750: zDEClient::ClearFeatureDisplayNodes
// (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp)
RECOIL_NOINLINE void RECOIL_CDECL ClearFeatureDisplayNodes() {
    zDEClient_MapTreeNode *header = g_zDEClient_FeatureMapTree.header;
    zDEClient_MapTreeNode *node = header->left;

    while (node != header) {
        zGeometry_ClipPatchNodeView *key = node->key;
        if (key != 0) {
            GameZ_ZBD::ReloadDisplayInstancesFromCurrentPath_Local(key, 1);

            const int gridCol = key->gridCol;
            const int gridRow = key->gridRow;
            if (gridCol >= 0 && gridRow >= 0) {
                zWorldAreaPartial *area =
                    zClass_World::GetAreaPartitionAtGrid(key->listA[0], gridCol, gridRow);
                if (area != 0) {
                    area->displayRefreshQueued = 0;
                }
            }
        }

        g_zDEClient_FeatureMapTree.IterNextNodeRef(&node);
    }

    zClass_NodePartial *child = zClass::FindByTypeAndName(6, "ZDEC_FEATURE");
    while (child != 0) {
        while (child->listCountA > 0) {
            zClass_Class::RemoveChild(child->listA[0], child);
        }

        unsigned int displayInstanceValue = 0;
        zClass_Class::gwNodeGetUserData(child, &displayInstanceValue);
        if (displayInstanceValue != 0) {
            zDiPartial *displayInstance =
                (zDiPartial *)(static_cast<unsigned int>(displayInstanceValue));
            zClass_Class::gwNodeSetDisplayInstance(child, 0);
            zModel_DiPool::FreeIfUnreferenced(displayInstance);
        }

        zClass_Class::DeleteNodeByType(child);
        child = zClass::FindByTypeAndName(6, "ZDEC_FEATURE");
    }
}

// Reimplements 0x457ae0: zDEClient::ClearFeatureEntriesAndMapTree
RECOIL_NOINLINE int RECOIL_CDECL ClearFeatureEntriesAndMapTree() {
    g_zDEClient_FeatureListEnd = g_zDEClient_FeatureListBegin;

    if (g_zDEClient_FeatureMapTree.header != 0) {
        zDEClient_MapTreeNode *outNext = 0;
        g_zDEClient_FeatureMapTree.EraseRange(&outNext, g_zDEClient_FeatureMapTree.header->left,
                                              g_zDEClient_FeatureMapTree.header);
    }

    return 0;
}

// Reimplements 0x455e40: zDEClient::ShutdownGlobals
RECOIL_NOINLINE int RECOIL_CDECL ShutdownGlobals() {
    if (g_zDEClient_QuickSandEnabled == 0) {
        return 0;
    }

    ClearFeatureEntriesAndMapTree();

    if (g_zDEClient_QuickSandTexturePaths != 0) {
        free(g_zDEClient_QuickSandTexturePaths);
        g_zDEClient_QuickSandTexturePaths = 0;
    }

    if (g_zDEClient_CraterDisplaySourceList != 0) {
        free(g_zDEClient_CraterDisplaySourceList);
        g_zDEClient_CraterDisplaySourceList = 0;
    }

    g_zDEClient_QuickSandEnabled = 0;
    zGame::ReturnOnlyStub();
    return 0;
}
} // namespace zDEClient
