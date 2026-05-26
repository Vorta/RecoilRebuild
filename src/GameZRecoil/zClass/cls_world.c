#include "zClass.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zUtil/zZbd.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
    const char *kWorldSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c";

    int TruncateToInt(float value) {
        return static_cast<int>(value);
    }

    float ApproximateSqrtFromRangeSq(float rangeSq) {
        int bits = 0;
        memcpy(&bits, &rangeSq, sizeof(bits));
        bits = (bits >> 1) + 0x1fc00000;
        float range = 0.0f;
        memcpy(&range, &bits, sizeof(range));
        return range;
    }

    void InvalidateGrid(int *outGridCol, int *outGridRow) {
        *outGridCol = -1;
        *outGridRow = -1;
    }

    void AppendNodeRef(zClass_NodePartial * **list, int *count, zClass_NodePartial *node) {
        zClass_NodePartial **resized = static_cast<zClass_NodePartial **>(
            realloc(*list, (*count + 1) * sizeof(zClass_NodePartial *)));
        *list = resized;
        resized[*count] = node;
        ++*count;
    }

    void CompactNodeRefList(zClass_NodePartial * *list, int *count,
                            int removedIndex) {
        for (int i = removedIndex; i < *count - 1; ++i) {
            list[i] = list[i + 1];
        }
        --*count;
    }

    zWorldAreaPartial *AreaAt(zClass_WorldDataPartial * data, int gridCol,
                              int gridRow) {
        return &data->areaGridRows[gridRow][gridCol];
    }

    void ExpandAreaYBounds(zWorldAreaPartial * area, const zBBoxCorners &corners) {
        for (int i = 0; i < 8; ++i) {
            const float y = corners.values[i * 3 + 1];
            if (y < area->bbox[1]) {
                area->bbox[1] = y;
            } else if (y > area->bbox[4]) {
                area->bbox[4] = y;
            }
        }
    }

    // Restores likely inlined VAP edge-cell helper observed four times in
    // 0x4502b0; no standalone function exists in the retail executable.
    RECOIL_FORCEINLINE void MoveAreaChildrenToVapStatics(zClass_NodePartial * world,
                                                         zWorldAreaPartial *area) {
        if (area->childCount <= 0) {
            return;
        }

        zClass_NodePartial *statics = zClass_Object3D::gwObject3DInit();
        zClass_Class::gwNodeSetName(statics, "VAP_statics");
        while (area->childCount > 0) {
            zClass_NodePartial *child = area->childList[0];
            zClass_Object3D::gwObject3DAddChild(statics, child);
            zClass_World::RemoveChildAtGrid(world, child);
        }

        zClass_TypeList::UpdateQueuedTrees();
        zClass_World::AddChildAtGrid(world, statics);
    }
}

namespace zClass_World {
    // Reimplements 0x4517a0: zClass_World::WriteSettingsSection
    // (GameZRecoil/zClass/cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL WriteSettingsSection(zZbdSectionCallbackCtx *callbackCtx,
                                                                      void *userData) {
        (void)userData;

        int result = 1;
        zClass_TypeListLink *link = zClass_TypeList::Head(13);
        while (link != 0 && result != 0) {
            zClass_NodePartial *world = link->node;
            zClass_WorldSettingsSectionRecord settings;
            GetPendingFogDensity(world, &settings.fogDensity);
            GetPendingFogState(world, &settings.fogState);
            GetPendingFogColorRgb01(world, &settings.fogColorRgb01.red,
                                    &settings.fogColorRgb01.green,
                                    &settings.fogColorRgb01.blue);
            GetPendingFogRange(world, &settings.fogRangeNear, &settings.fogRangeFar);
            GetPendingFogAltitudeRange(world, &settings.fogAltitudeLow,
                                       &settings.fogAltitudeHigh);
            GetPendingFogDensity(world, &settings.fogDensity);
            result = zUtil_ZAR::WriteSectionBlob(callbackCtx, world->name, &settings,
                                                 sizeof(settings));
            link = link->next;
        }

        return result;
    }

    // Reimplements 0x451840: zClass_World::ReadSettingsSection
    // (GameZRecoil/zClass/cls_world.c)
    RECOIL_NOINLINE void RECOIL_FASTCALL ReadSettingsSection(
        zZbdSectionCallbackCtx *callbackCtx, const char *worldName,
        zClass_WorldSettingsSectionRecord *settings, unsigned int size, void *userData) {
        (void)callbackCtx;
        (void)size;
        (void)userData;

        zClass_NodePartial *world = zClass::FindByTypeAndName(13, worldName);
        if (world == 0) {
            return;
        }

        SetPendingFogDensity(world, settings->fogDensity);
        SetPendingFogState(world, settings->fogState);
        SetPendingFogColorRgb01(world, settings->fogColorRgb01.red,
                                settings->fogColorRgb01.green,
                                settings->fogColorRgb01.blue);
        SetPendingFogRange(world, settings->fogRangeNear, settings->fogRangeFar);
        SetPendingFogAltitudeRange(world, settings->fogAltitudeLow,
                                   settings->fogAltitudeHigh);
        SetPendingFogDensity(world, settings->fogDensity);
    }

    // Reimplements 0x4501c0: zClass_World::gwWorldNew
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_CDECL gwWorldNew() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        node->classId = 2;

        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(
            calloc(1, sizeof(zClass_WorldDataPartial)));
        node->classData = data;
        data->fogState = 0;
        data->lightCount = 0;
        data->lightNodes = 0;
        data->lightDataList = 0;
        data->soundCount = 0;
        data->soundNodes = 0;
        data->soundDataList = 0;
        data->scaleX = 1.0f;
        data->scaleY = 1.0f;
        data->scaleZ = 1.0f;
        data->clampQueriesToBounds = 0;
        data->flags = 1;
        data->partitionMaxDecFeatureCount = 16;
        zClass_TypeList::Insert(13, node);
        return node;
    }

    // Reimplements 0x450ae0: zClass_World::SetPendingFogState
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogState(zClass_NodePartial * world,
                                                                    int fogState) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        data->fogState = fogState;
        data->flags |= 0x01;
        return 0;
    }

    // Reimplements 0x450af0: zClass_World::SetPendingFogColorRgb01
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogColorRgb01(
        zClass_NodePartial * world, float red, float green, float blue) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        data->ambientColor.red = red;
        data->ambientColor.blue = blue;
        data->ambientColor.green = green;
        data->flags |= 0x02;
        return 0;
    }

    // Reimplements 0x450b20: zClass_World::SetPendingFogAltitudeRange
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogAltitudeRange(
        zClass_NodePartial * world, float minAlt, float maxAlt) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        data->fogHeightHigh = maxAlt;
        data->fogHeightLow = minAlt;
        data->flags |= 0x20;
        return 0;
    }

    // Reimplements 0x450b40: zClass_World::SetPendingFogRange
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogRange(
        zClass_NodePartial * world, float nearRange, float farRange) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        data->fogDistanceStart = nearRange;
        data->fogDistanceEnd = farRange;
        data->flags |= 0x04;
        return 0;
    }

    // Reimplements 0x450b80: zClass_World::GetPendingFogDensity
    // (GameZRecoil/zClass/cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL GetPendingFogDensity(zClass_NodePartial * world,
                                                                      float *outDensity) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        *outDensity = data->fogDensity;
        return 0;
    }

    // Reimplements 0x450b90: zClass_World::GetPendingFogState
    // (GameZRecoil/zClass/cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL GetPendingFogState(zClass_NodePartial * world,
                                                                    int *outState) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        *outState = data->fogState;
        return 0;
    }

    // Reimplements 0x450ba0: zClass_World::GetPendingFogColorRgb01
    // (GameZRecoil/zClass/cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL GetPendingFogColorRgb01(zClass_NodePartial * world,
                                                                         float *outRed,
                                                                         float *outGreen,
                                                                         float *outBlue) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        *outRed = data->ambientColor.red;
        *outGreen = data->ambientColor.green;
        *outBlue = data->ambientColor.blue;
        return 0;
    }

    // Reimplements 0x450bc0: zClass_World::GetPendingFogRange
    // (GameZRecoil/zClass/cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL GetPendingFogRange(
        zClass_NodePartial * world, float *outNearRange, float *outFarRange) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        *outNearRange = data->fogDistanceStart;
        *outFarRange = data->fogDistanceEnd;
        return 0;
    }

    // Reimplements 0x450be0: zClass_World::GetPendingFogAltitudeRange
    // (GameZRecoil/zClass/cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL GetPendingFogAltitudeRange(
        zClass_NodePartial * world, float *outMinAlt, float *outMaxAlt) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        *outMaxAlt = data->fogHeightHigh;
        *outMinAlt = data->fogHeightLow;
        return 0;
    }

    // Reimplements 0x450b60: zClass_World::SetPendingFogDensity
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetPendingFogDensity(zClass_NodePartial * world,
                                                                      float density) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        data->fogDensity = density;
        data->flags |= 0x08;
        return 0;
    }

    // Reimplements 0x450c00: zClass_World::gwWorldSetOrigin
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwWorldSetOrigin(zClass_NodePartial * world,
                                                                 float originX, float originZ) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        data->originX = originX;
        data->originZ = originZ;
        data->worldMaxX = data->worldSizeX + originX;
        data->worldMaxZ = data->worldSizeZ + originZ;
        return 0;
    }

    // Reimplements 0x450c30: zClass_World::gwWorldSetSize
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwWorldSetSize(zClass_NodePartial * world,
                                                               float sizeX, float sizeZ) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        data->worldSizeX = sizeX;
        data->worldSizeZ = sizeZ;
        data->worldMaxX = data->originX + sizeX;
        data->worldMaxZ = data->originZ + sizeZ;
        return 0;
    }

    // Reimplements 0x450f00: zClass_World::gwWorldSetPartitionInclusionTolerance
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwWorldSetPartitionInclusionTolerance(
        zClass_NodePartial * world, float toleranceX, float toleranceZ) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        data->partitionInclusionTolX = toleranceX;
        data->partitionInclusionTolZ = toleranceZ;
        return 0;
    }

    // Reimplements 0x450f20: zClass_World::gwWorldSetMaxDecFeatures
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwWorldSetMaxDecFeatures(zClass_NodePartial * world,
                                                                           int maxFeatures) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        if (maxFeatures > 255) {
            zError::ReportOld(
                0x200, kWorldSourceFile, 0xc01,
                "ERROR setting Partition Max DEC Feature count to %d:\noverflow limit at 255.\n",
                maxFeatures);
            maxFeatures = 255;
        }

        data->partitionMaxDecFeatureCount = static_cast<unsigned char>(maxFeatures);
        return 0;
    }

    // Reimplements 0x450c60: zClass_World::gwWorldSetVirtualAreaPartition
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL gwWorldSetVirtualAreaPartition(
        zClass_NodePartial * world, float cellSizeX, float cellSizeZ) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        if (data->areaGridRows != 0) {
            FreeVirtualAreaPartitions(world);
        }

        data->areaCellSizeX = cellSizeX;
        data->areaCellSizeZ = cellSizeZ;
        data->partitionInclusionTolX = cellSizeX * 0.125f;
        data->partitionInclusionTolZ = cellSizeZ * -0.125f;
        data->areaHalfSizeX = cellSizeX * 0.5f;
        data->areaHalfSizeZ = cellSizeZ * 0.5f;
        data->areaInvSizeX = 1.0f / cellSizeX;
        data->areaInvSizeZ = 1.0f / cellSizeZ;
        data->areaCellRadiusBias =
            ApproximateSqrtFromRangeSq(cellSizeX * cellSizeX + cellSizeZ * cellSizeZ) * -0.5f;

        int gridColCount = TruncateToInt(data->worldSizeX / data->areaCellSizeX);
        data->areaGridColCount = gridColCount;
        if (static_cast<float>(gridColCount) * data->areaCellSizeX < data->worldSizeX) {
            ++gridColCount;
            data->areaGridColCount = gridColCount;
        }

        int gridRowCount = TruncateToInt(data->worldSizeZ / data->areaCellSizeZ);
        data->areaGridRowCount = gridRowCount;
        if (static_cast<float>(gridRowCount) * data->areaCellSizeZ > data->worldSizeZ) {
            ++gridRowCount;
            data->areaGridRowCount = gridRowCount;
        }

        data->areaGridRows =
            static_cast<zWorldAreaPartial **>(calloc(data->areaGridRowCount,
                                                     sizeof(zWorldAreaPartial *)));
        for (int row = 0; row < data->areaGridRowCount; ++row) {
            data->areaGridRows[row] =
                static_cast<zWorldAreaPartial *>(calloc(data->areaGridColCount,
                                                        sizeof(zWorldAreaPartial)));
        }

        for (int row = 0; row < data->areaGridRowCount; ++row) {
            const float rowAsFloat = static_cast<float>(row);
            for (int col = 0; col < data->areaGridColCount; ++col) {
                zWorldAreaPartial *area = &data->areaGridRows[row][col];
                area->areaFlags |= 0x100;
                area->cellMinX = static_cast<float>(col) * data->areaCellSizeX + data->originX;
                area->cellMinZ = rowAsFloat * data->areaCellSizeZ + data->originZ;
                area->bbox[0] = area->cellMinX;
                area->bbox[3] = area->cellMinX + data->areaCellSizeX;
                area->bbox[5] = area->cellMinZ;
                area->bbox[2] = area->cellMinZ + data->areaCellSizeZ;
                BBox::MinMaxToBoundingSphere((const zBBox3f *)(area->bbox),
                                             &area->bboxCenter, &area->bboxRadius);
                area->areaIndex = -1;
            }
        }

        return 0;
    }

    // Reimplements 0x4502b0: zClass_World::InitVirtualAreaPartitions
    // (GameZRecoil/zClass/cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL InitVirtualAreaPartitions(zClass_NodePartial * world) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        if (data->areaGridRows == 0) {
            sprintf(g_zError_DebugMsgBuffer,
                    "%s: Line %d: ERROR initializing virtual area partition; NULL area partitions encountered.\n",
                    kWorldSourceFile, 0x245);
            zError::EmitDebugBuffer(5);
            return 5;
        }

        zClass_TypeList::UpdateQueuedTrees();

        for (int col = 0; col < data->areaGridColCount; ++col) {
            MoveAreaChildrenToVapStatics(world, &data->areaGridRows[0][col]);
        }

        zWorldAreaPartial *lastRow = data->areaGridRows[data->areaGridRowCount - 1];
        for (int col = 0; col < data->areaGridColCount; ++col) {
            MoveAreaChildrenToVapStatics(world, &lastRow[col]);
        }

        for (int row = 1; row < data->areaGridRowCount - 1; ++row) {
            MoveAreaChildrenToVapStatics(world, &data->areaGridRows[row][0]);
        }

        for (int row = 1; row < data->areaGridRowCount - 1; ++row) {
            MoveAreaChildrenToVapStatics(world,
                                         &data->areaGridRows[row][data->areaGridColCount - 1]);
        }

        return 0;
    }

    // Reimplements 0x450510: zClass_World::SetVirtualPartition
    // (GameZRecoil/zClass/cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL SetVirtualPartition(zClass_NodePartial * world,
                                                            int enabled) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        data->clampQueriesToBounds = enabled;
        if (enabled != 0) {
            InitVirtualAreaPartitions(world);
        }
        return 0;
    }

    // Reimplements 0x450e40: zClass_World::FreeVirtualAreaPartitions
    RECOIL_NOINLINE int RECOIL_FASTCALL FreeVirtualAreaPartitions(zClass_NodePartial *
                                                                           world) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        if (data->areaGridRows == 0) {
            return 0;
        }

        {
        for (int row = 0; row < data->areaGridRowCount; ++row) {
            zWorldAreaPartial *areas = data->areaGridRows[row];
            {
            for (int col = 0; col < data->areaGridColCount; ++col) {
                if (areas[col].childList != 0) {
                    free(areas[col].childList);
                    areas[col].childList = 0;
                }
            }
            }

            if (data->areaGridExternalOwnership == 0) {
                free(areas);
            }
        }
        }

        if (data->areaGridExternalOwnership == 0) {
            free(data->areaGridRows);
        }

        data->areaGridRows = 0;
        data->areaCellSizeZ = 0.0f;
        data->areaCellSizeX = 0.0f;
        data->areaGridRowCount = 0;
        data->areaGridColCount = 0;
        return 0;
    }

    // Reimplements 0x450240: zClass_World::DeleteNode
    RECOIL_NOINLINE int RECOIL_FASTCALL DeleteNode(zClass_NodePartial * world) {
        const int freeResult = FreeVirtualAreaPartitions(world);
        if (freeResult != 0) {
            return freeResult;
        }

        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        if (data->lightNodes != 0) {
            free(data->lightNodes);
        }
        if (data->lightDataList != 0) {
            free(data->lightDataList);
        }
        if (data->soundNodes != 0) {
            free(data->soundNodes);
        }
        if (data->soundDataList != 0) {
            free(data->soundDataList);
        }
        if (data->pendingAreaUpdates != 0) {
            free(data->pendingAreaUpdates);
        }

        return zClass_Class::TryFreeNode(world);
    }

    // Reimplements 0x450030: zClass_World::QueueAreaUpdate
    RECOIL_NOINLINE int RECOIL_FASTCALL QueueAreaUpdate(
        zClass_NodePartial * world, zClass_WorldDataPartial * worldData, zWorldAreaPartial * area) {
        if (worldData->pendingAreaUpdateCount == worldData->pendingAreaUpdateCapacity) {
            worldData->pendingAreaUpdates = static_cast<zWorldAreaPartial **>(realloc(
                worldData->pendingAreaUpdates,
                (worldData->pendingAreaUpdateCapacity + 1) * sizeof(zWorldAreaPartial *)));
            ++worldData->pendingAreaUpdateCapacity;
        }

        worldData->pendingAreaUpdates[worldData->pendingAreaUpdateCount] = area;
        ++worldData->pendingAreaUpdateCount;
        area->areaFlags |= 0x01;

        if ((world->flags & 0x01) == 0) {
            if (zClass_TypeList::InsertChildNodes(7, world) == 0) {
                world->flags |= 0x01;
            }
        }
        world->flags |= 0x02;
        worldData->flags |= 0x10;
        return 0;
    }

    // Reimplements 0x4500b0: zClass_World::RebuildAreaBounds
    RECOIL_NOINLINE int RECOIL_FASTCALL RebuildAreaBounds(
        zClass_WorldDataPartial * /*worldData*/, zWorldAreaPartial * area) {
        const short childCount = area->childCount;
        area->areaFlags &= ~0x01;
        if (childCount == 0) {
            return 0;
        }

        zBBoxCorners corners = {0};
        int childIndex = 0;
        for (; childIndex < childCount; ++childIndex) {
            zClass_NodePartial *child = area->childList[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            area->areaFlags |= 0x100;
            zClass_Class::gwNodeGetWorldBBoxCorners(child, &corners);
            area->bbox[1] = corners.values[1];
            area->bbox[4] = corners.values[1];
            for (int i = 1; i < 8; ++i) {
                const float y = corners.values[i * 3 + 1];
                if (y < area->bbox[1]) {
                    area->bbox[1] = y;
                } else if (y > area->bbox[4]) {
                    area->bbox[4] = y;
                }
            }
            ++childIndex;
            break;
        }

        if ((area->areaFlags & 0x100) == 0) {
            return 0;
        }

        for (; childIndex < childCount; ++childIndex) {
            zClass_NodePartial *child = area->childList[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            zClass_Class::gwNodeGetWorldBBoxCorners(child, &corners);
            ExpandAreaYBounds(area, corners);
        }

        BBox::MinMaxToBoundingSphere((const zBBox3f *)(area->bbox),
                                     &area->bboxCenter, &area->bboxRadius);
        return 0;
    }

    // Reimplements 0x450530: zClass_World::ApplyPendingFogSettings
    RECOIL_NOINLINE int RECOIL_FASTCALL ApplyPendingFogSettings(zClass_NodePartial *
                                                                         world) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        if (zClass_TypeList::CountNodes(0x0d) > 1) {
            data->flags = 0x2f;
        }

        const int pendingFlags = data->flags;
        if (pendingFlags == 0) {
            return 0;
        }

        int fogChanged = 0;
        if ((pendingFlags & 0x01) != 0) {
            fogChanged = 1;
            if (data->fogState == 0) {
                zModel_Fog_SetEnabled(0);
                zModel_Fog_SetLinearModeEnabled(0);
                zModel_Fog_SetDensity(0.0f);
            } else {
                zModel_Fog_SetEnabled(1);
                zModel_Fog_SetLinearModeEnabled(data->fogState == 1 ? 1 : 0);
            }
        }

        if ((data->flags & 0x02) != 0) {
            fogChanged = 1;
            zRndr::FogColor_SetRgb01Clamped(&data->ambientColor);
            zModel_Fog_SetColorRgb01(&data->ambientColor);
        }

        if ((data->flags & 0x04) != 0) {
            fogChanged = 1;
            zModel_Fog_SetDistanceStart(data->fogDistanceStart);
            zModel_Fog_SetDistanceEnd(data->fogDistanceEnd);
        }

        if ((data->flags & 0x20) != 0) {
            fogChanged = 1;
            zModel_Fog_SetHeightHigh(data->fogHeightHigh);
            zModel_Fog_SetHeightLow(data->fogHeightLow);
        }

        if ((data->flags & 0x08) != 0) {
            fogChanged = 1;
            zModel_Fog_SetDensity(data->fogDensity);
        }

        if (fogChanged != 0) {
            zModel_Fog_ApplyCurrentColor();
        }

        if (data->pendingAreaUpdateCount > 0) {
            zWorldAreaPartial **pendingAreaUpdates = data->pendingAreaUpdates;
            do {
                zWorldAreaPartial *area = *pendingAreaUpdates;
                RebuildAreaBounds(data, area);
                ++pendingAreaUpdates;
                area->areaFlags &= ~0x01;
                --data->pendingAreaUpdateCount;
            } while (data->pendingAreaUpdateCount > 0);
        }

        data->flags = 0;
        return 0;
    }

    // Reimplements 0x450840: zClass_World::WorldRectToGridIndex
    RECOIL_NOINLINE int RECOIL_FASTCALL WorldRectToGridIndex(
        zClass_NodePartial * world, int *outGridCol, float minX, float maxX, float minZ,
        float maxZ, int *outGridRow) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        InvalidateGrid(outGridCol, outGridRow);

        if (data->originX - data->partitionInclusionTolX > minX ||
            maxX >= data->worldMaxX + data->partitionInclusionTolX ||
            maxZ > data->originZ + data->partitionInclusionTolZ ||
            minZ <= data->worldMaxZ - data->partitionInclusionTolZ) {
            return 0;
        }

        const float centerX = (minX + maxX) * 0.5f - data->originX;
        const float centerZ = (minZ + maxZ) * 0.5f - data->originZ;
        *outGridCol = TruncateToInt(centerX * data->areaInvSizeX);
        *outGridRow = TruncateToInt(centerZ * data->areaInvSizeZ);

        if (*outGridCol < 0) {
            *outGridCol = 0;
        } else if (*outGridCol >= data->areaGridColCount) {
            *outGridCol = data->areaGridColCount - 1;
        }

        if (*outGridRow < 0) {
            *outGridRow = 0;
        } else if (*outGridRow >= data->areaGridRowCount) {
            *outGridRow = data->areaGridRowCount - 1;
        }

        zWorldAreaPartial *gridCell = &data->areaGridRows[*outGridRow][*outGridCol];
        const float cellMaxX = gridCell->cellMinX + data->areaCellSizeX;
        const float cellMaxZ = gridCell->cellMinZ + data->areaCellSizeZ;

        if (minX < gridCell->cellMinX && gridCell->cellMinX - minX > data->partitionInclusionTolX) {
            InvalidateGrid(outGridCol, outGridRow);
        } else if (maxX > cellMaxX && maxX - cellMaxX > data->partitionInclusionTolX) {
            InvalidateGrid(outGridCol, outGridRow);
        } else if (minZ < cellMaxZ && cellMaxZ - minZ > data->partitionInclusionTolZ) {
            InvalidateGrid(outGridCol, outGridRow);
        } else if (maxZ > gridCell->cellMinZ &&
                   maxZ - gridCell->cellMinZ > data->partitionInclusionTolZ) {
            InvalidateGrid(outGridCol, outGridRow);
        }

        return 0;
    }

    // Reimplements 0x450650: zClass_World::WorldToGridCoordsClampedEx
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL WorldToGridCoordsClampedEx(
        zClass_NodePartial *world, int *outGridCol, float worldX, float worldZ,
        int *outGridRow, int *clampedGridColOut,
        int *clampedGridRowOut, int *insideBoundsOut) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);

        float clampedX = worldX;
        float clampedZ = worldZ;
        *insideBoundsOut = 1;

        if (worldX < data->originX) {
            clampedX = data->originX + 0.1f;
            *insideBoundsOut = 0;
        } else if (worldX >= data->worldMaxX) {
            clampedX = data->worldMaxX - 0.1f;
            *insideBoundsOut = 0;
        }

        if (worldZ <= data->originZ) {
            clampedZ = data->originZ - 0.1f;
            *insideBoundsOut = 0;
        } else if (worldZ > data->worldMaxZ) {
            clampedZ = data->worldMaxZ + 0.1f;
            *insideBoundsOut = 0;
        }

        *clampedGridColOut = TruncateToInt((clampedX - data->originX) * data->areaInvSizeX);
        *clampedGridRowOut = TruncateToInt((clampedZ - data->originZ) * data->areaInvSizeZ);

        if (*insideBoundsOut != 0) {
            *outGridCol = *clampedGridColOut;
            *outGridRow = *clampedGridRowOut;
            return 0;
        }

        *outGridCol = static_cast<int>(
            floor((worldX - data->originX) * data->areaInvSizeX));
        *outGridRow = static_cast<int>(
            floor((worldZ - data->originZ) * data->areaInvSizeZ));
        return 0;
    }

    // Reimplements 0x450790: zClass_World::WorldToGridCoordsClamped
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL WorldToGridCoordsClamped(
        zClass_NodePartial * world, int *outGridCol, float worldX, float worldZ,
        int *outGridRow) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);

        float clampedX;
        if (worldX < data->originX + 0.1f) {
            clampedX = data->originX - 0.1f;
        } else {
            clampedX = data->worldMaxX - 0.1f;
            if (worldX < clampedX) {
                clampedX = worldX;
            }
        }

        float clampedZ = data->originZ - 0.1f;
        if (worldZ > clampedZ) {
            clampedZ = data->worldMaxZ + 0.1f;
            if (worldZ > clampedZ) {
                clampedZ = worldZ;
            }
        }

        *outGridCol = TruncateToInt((clampedX - data->originX) * data->areaInvSizeX);
        *outGridRow = TruncateToInt((clampedZ - data->originZ) * data->areaInvSizeZ);
        return 0;
    }

    // Reimplements 0x450a00: zClass_World::GetAreaPartitionAtGrid
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE zWorldAreaPartial *RECOIL_FASTCALL GetAreaPartitionAtGrid(
        zClass_NodePartial * world, int gridCol, int gridRow) {
        if (world == 0) {
            zError::ReportOld(0x400, kWorldSourceFile, 0x6d4, "Null node pointer.");
            return 0;
        }

        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kWorldSourceFile, 0x6d5, "Null class data pointer");
            return 0;
        }

        return AreaAt(data, gridCol, gridRow);
    }

    // Reimplements 0x450a70: zClass_World::EnsureGridCellDisplayPosition
    RECOIL_NOINLINE int RECOIL_FASTCALL EnsureGridCellDisplayPosition(
        zClass_NodePartial * world, int gridCol, int gridRow) {
        if (world == 0) {
            zError::ReportOld(0x400, kWorldSourceFile, 0x6f5, "Null node pointer.");
            return 5;
        }

        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        if (data == 0) {
            zError::ReportOld(0x400, kWorldSourceFile, 0x6f6, "Null class data pointer");
            return 5;
        }

        zWorldAreaPartial *area = &data->areaGridRows[gridRow][gridCol];
        if ((area->areaFlags & 0x01) == 0) {
            return QueueAreaUpdate(world, data, area);
        }

        return 0;
    }

    // Reimplements 0x4510e0: zClass_World::AddChildAtGrid
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL AddChildAtGrid(zClass_NodePartial * world,
                                                                zClass_NodePartial * child) {
        int gridCol = -1;
        int gridRow = -1;

        if ((child->flags & 0x80) == 0) {
            float minX = 0.0f;
            float maxX = 0.0f;
            float minZ = 0.0f;
            float maxZ = 0.0f;

            if ((child->flags & 0x100) != 0) {
                zBBoxCorners corners = {0};
                zClass_Class::gwNodeGetWorldBBoxCorners(child, &corners);
                minX = corners.values[0];
                maxX = corners.values[0];
                minZ = corners.values[2];
                maxZ = corners.values[2];

                for (int i = 1; i < 8; ++i) {
                    const float x = corners.values[i * 3];
                    const float z = corners.values[i * 3 + 2];
                    if (x < minX) {
                        minX = x;
                    } else if (x > maxX) {
                        maxX = x;
                    }

                    if (z < minZ) {
                        minZ = z;
                    } else if (z > maxZ) {
                        maxZ = z;
                    }
                }
            } else {
                zClass_WorldDataPartial *data =
                    static_cast<zClass_WorldDataPartial *>(world->classData);
                minX = data->originX;
                minZ = data->originZ;
                maxX = data->originX + data->worldSizeX;
                maxZ = data->originZ + data->worldSizeZ;
            }

            WorldRectToGridIndex(world, &gridCol, minX, maxX, minZ, maxZ, &gridRow);
        }

        return AddChildToGridCell(world, child, gridCol, gridRow);
    }

    // Reimplements 0x450f60: zClass_World::AddChildToGridCell
    RECOIL_NOINLINE int RECOIL_FASTCALL AddChildToGridCell(
        zClass_NodePartial * world, zClass_NodePartial * child, int gridCol,
        int gridRow) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);

        if (gridCol >= 0 && gridRow >= 0 && AreaAt(data, gridCol, gridRow)->childCount >= 0x7fff) {
            gridCol = -1;
            gridRow = -1;
        }

        if (gridCol < 0 || gridRow < 0) {
            AppendNodeRef(&world->listB, &world->listCountB, child);
            child->gridCol = -1;
            child->gridRow = -1;
            AppendNodeRef(&child->listA, &child->listCountA, world);
            if (child->listCountA > 1) {
                zClass_Class::SetSingleParentFlagRecursive(child, 0);
            }
            return 0;
        }

        zWorldAreaPartial *area = AreaAt(data, gridCol, gridRow);
        zClass_NodePartial **resized = static_cast<zClass_NodePartial **>(
            realloc(area->childList, (static_cast<int>(area->childCount) + 1) *
                                              sizeof(zClass_NodePartial *)));
        area->childList = resized;
        resized[area->childCount] = child;
        ++area->childCount;

        child->gridCol = gridCol;
        child->gridRow = gridRow;
        AppendNodeRef(&child->listA, &child->listCountA, world);
        if (child->listCountA > 1) {
            zClass_Class::SetSingleParentFlagRecursive(child, 0);
        }

        if ((area->areaFlags & 0x01) == 0) {
            return QueueAreaUpdate(world, data, area);
        }

        return 0;
    }

    // Reimplements 0x451240: zClass_World::RemoveChildAtGrid
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveChildAtGrid(zClass_NodePartial * world,
                                                                   zClass_NodePartial * child) {
        const int gridCol = child->gridCol;
        const int gridRow = child->gridRow;
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);

        if (gridCol == -1 && gridRow == -1) {
            return zClass_Class::RemoveChildGeneric(world, child);
        }

        zWorldAreaPartial *area = AreaAt(data, gridCol, gridRow);
        int childIndex = -1;
        for (int i = 0; i < area->childCount; ++i) {
            if (area->childList[i] == child) {
                childIndex = i;
                break;
            }
        }

        if (childIndex < 0) {
            zError::ReportOld(0x200, kWorldSourceFile, 0xfaf,
                              "ERROR deleting child node %s from parent node %s", child, world);
            return 1;
        }

        int areaChildCount = area->childCount;
        CompactNodeRefList(area->childList, &areaChildCount, childIndex);
        area->childCount = static_cast<short>(areaChildCount);

        child->gridCol = -1;
        child->gridRow = -1;

        int parentIndex = -1;
        for (int i_613 = 0; i_613 < child->listCountA; ++i_613) {
            if (child->listA[i_613] == world) {
                parentIndex = i_613;
                break;
            }
        }
        if (parentIndex >= 0) {
            CompactNodeRefList(child->listA, &child->listCountA, parentIndex);
        }

        if ((area->areaFlags & 0x01) == 0) {
            return QueueAreaUpdate(world, data, area);
        }

        return 0;
    }

    // Reimplements 0x451360: zClass_World::AddLight
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL AddLight(zClass_NodePartial * world,
                                                          zClass_NodePartial * light) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        zClass_LightDataPartial *lightData =
            static_cast<zClass_LightDataPartial *>(light->classData);

        const int lightListBytes = (data->lightCount + 1) * sizeof(zClass_NodePartial *);
        data->lightNodes =
            static_cast<zClass_NodePartial **>(realloc(data->lightNodes, lightListBytes));
        data->lightNodes[data->lightCount] = light;

        data->lightDataList = static_cast<zClass_LightDataPartial **>(
            realloc(data->lightDataList, lightListBytes));
        data->lightDataList[data->lightCount] = lightData;
        ++data->lightCount;

        lightData->attachedWorlds = static_cast<zClass_NodePartial **>(
            realloc(lightData->attachedWorlds,
                         (lightData->attachedWorldCount + 1) * sizeof(zClass_NodePartial *)));
        lightData->attachedWorlds[lightData->attachedWorldCount] = world;
        ++lightData->attachedWorldCount;
        return 0;
    }

    // Reimplements 0x451410: zClass_World::RemoveLight
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveLight(zClass_NodePartial * world,
                                                             zClass_NodePartial * light) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);

        int lightIndex = -1;
        for (int i = 0; i < data->lightCount; ++i) {
            if (data->lightNodes[i] == light) {
                lightIndex = i;
                break;
            }
        }

        if (lightIndex < 0) {
            sprintf(g_zError_DebugMsgBuffer,
                         "%s: Line %d: ERROR deleting light; not found in world list.\n"
                         "        world_ptr = %x; light_ptr = %x\n",
                         kWorldSourceFile, 0x108d,
                         static_cast<unsigned int>((unsigned int)(world)),
                         static_cast<unsigned int>((unsigned int)(light)));
            zError::EmitDebugBuffer(5);
            return 5;
        }

        zClass_LightDataPartial *lightData = data->lightDataList[lightIndex];
        for (int i_681 = lightIndex; i_681 < data->lightCount - 1; ++i_681) {
            data->lightNodes[i_681] = data->lightNodes[i_681 + 1];
            data->lightDataList[i_681] = data->lightDataList[i_681 + 1];
        }
        --data->lightCount;

        int worldIndex = -1;
        for (int i_688 = 0; i_688 < lightData->attachedWorldCount; ++i_688) {
            if (lightData->attachedWorlds[i_688] == world) {
                worldIndex = i_688;
                break;
            }
        }

        if (worldIndex < 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR deleting light; world not found in light's world list.\n"
                "        world_ptr = %x; light_ptr = %x\n",
                kWorldSourceFile, 0x10b4,
                static_cast<unsigned int>((unsigned int)(world)),
                static_cast<unsigned int>((unsigned int)(light)));
            zError::EmitDebugBuffer(5);
            return 5;
        }

        for (int i_707 = worldIndex; i_707 < lightData->attachedWorldCount - 1; ++i_707) {
            lightData->attachedWorlds[i_707] = lightData->attachedWorlds[i_707 + 1];
        }
        --lightData->attachedWorldCount;

        return 0;
    }

    // Reimplements 0x451540: zClass_World::InitLightPointInPolygonXZ
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    InitLightPointInPolygonXZ(zClass_NodePartial *world) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        zModel_Light_PointInPolygonInitXZ(
            data->lightDataList, (zModel_LightStatePartial **)(data->lightNodes),
            data->lightCount);
        return 0;
    }

    // Reimplements 0x451560: zClass_World::UpdateAllLights
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL UpdateAllLights(zClass_NodePartial * world) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);

        for (int i = 0; i < data->lightCount; ++i) {
            zClass_Light::gwLightUpdate(data->lightNodes[i]);
        }

        return 0;
    }

    // Reimplements 0x451590: zClass_World::AddSound
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL AddSound(zClass_NodePartial * world,
                                                          zClass_NodePartial * sound) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);
        zClass_SoundDataPartial *soundData =
            static_cast<zClass_SoundDataPartial *>(sound->classData);

        const int soundListBytes = (data->soundCount + 1) * sizeof(zClass_NodePartial *);
        data->soundNodes =
            static_cast<zClass_NodePartial **>(realloc(data->soundNodes, soundListBytes));
        data->soundNodes[data->soundCount] = sound;

        data->soundDataList = static_cast<zClass_SoundDataPartial **>(
            realloc(data->soundDataList, soundListBytes));
        data->soundDataList[data->soundCount] = soundData;
        ++data->soundCount;

        soundData->attachedWorlds = static_cast<zClass_NodePartial **>(
            realloc(soundData->attachedWorlds,
                         (soundData->attachedWorldCount + 1) * sizeof(zClass_NodePartial *)));
        soundData->attachedWorlds[soundData->attachedWorldCount] = world;
        ++soundData->attachedWorldCount;
        return 0;
    }

    // Reimplements 0x451640: zClass_World::RemoveSound
    RECOIL_NOINLINE int RECOIL_FASTCALL RemoveSound(zClass_NodePartial * world,
                                                             zClass_NodePartial * sound) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);

        int soundIndex = -1;
        for (int i = 0; i < data->soundCount; ++i) {
            if (data->soundNodes[i] == sound) {
                soundIndex = i;
                break;
            }
        }

        if (soundIndex < 0) {
            sprintf(g_zError_DebugMsgBuffer,
                         "%s: Line %d: ERROR deleting sound; not found in world list.\n"
                         "        world_ptr = %x; sound_ptr = %x\n",
                         kWorldSourceFile, 0x11cc,
                         static_cast<unsigned int>((unsigned int)(world)),
                         static_cast<unsigned int>((unsigned int)(sound)));
            zError::EmitDebugBuffer(5);
            return 5;
        }

        zClass_SoundDataPartial *soundData = data->soundDataList[soundIndex];
        for (int i_789 = soundIndex; i_789 < data->soundCount - 1; ++i_789) {
            data->soundNodes[i_789] = data->soundNodes[i_789 + 1];
            data->soundDataList[i_789] = data->soundDataList[i_789 + 1];
        }
        --data->soundCount;

        int worldIndex = -1;
        for (int i_796 = 0; i_796 < soundData->attachedWorldCount; ++i_796) {
            if (soundData->attachedWorlds[i_796] == world) {
                worldIndex = i_796;
                break;
            }
        }

        if (worldIndex < 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                "%s: Line %d: ERROR deleting sound; world not found in sound's world list.\n"
                "        world_ptr = %x; sound_ptr = %x\n",
                kWorldSourceFile, 0x11f3,
                static_cast<unsigned int>((unsigned int)(world)),
                static_cast<unsigned int>((unsigned int)(sound)));
            zError::EmitDebugBuffer(5);
            return 5;
        }

        for (int i_815 = worldIndex; i_815 < soundData->attachedWorldCount - 1; ++i_815) {
            soundData->attachedWorlds[i_815] = soundData->attachedWorlds[i_815 + 1];
        }
        --soundData->attachedWorldCount;

        return 0;
    }

    // Reimplements 0x451770: zClass_World::UpdateAllSounds
    // (D:\Proj\GameZRecoil\zClass\cls_world.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL UpdateAllSounds(zClass_NodePartial * world) {
        zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(world->classData);

        for (int i = 0; i < data->soundCount; ++i) {
            zClass_Sound::UpdatePlayback(data->soundNodes[i]);
        }

        return 0;
    }
}
