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

extern "C" {
/**
 * Reimplements data 0x4de23c: g_zClass_LineErrorVirtualAreaPartitionNullFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x5b].
 * Purpose: report a missing virtual-area partition grid during world
 * partition initialization.
 */
char g_zClass_LineErrorVirtualAreaPartitionNullFmt[0x5b] =
    "%s: Line %d: ERROR initializing virtual area partition; NULL area partitions encountered.\n";
/**
 * Reimplements data 0x4de2c0: g_zClass_PartitionMaxDecFeatureCountOverflowFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x4d].
 * Purpose: report clamping of the maximum DEC feature count to the byte-sized
 * partition storage limit.
 */
char g_zClass_PartitionMaxDecFeatureCountOverflowFmt[0x4d] =
    "ERROR setting Partition Max DEC Feature count to %d:\n"
    "overflow limit at 255.\n";
/**
 * Reimplements data 0x4de310: g_zClass_LineErrorDeleteLightWorldNotFoundFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x72].
 * Purpose: report that a light's attached-world list does not contain the
 * world being removed.
 */
char g_zClass_LineErrorDeleteLightWorldNotFoundFmt[0x72] =
    "%s: Line %d: ERROR deleting light; world not found in light's world list.\n"
    "        world_ptr = %x; light_ptr = %x\n";
/**
 * Reimplements data 0x4de384: g_zClass_LineErrorDeleteLightNotFoundInWorldListFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x64].
 * Purpose: report that a light node is absent from the world's light list.
 */
char g_zClass_LineErrorDeleteLightNotFoundInWorldListFmt[0x64] =
    "%s: Line %d: ERROR deleting light; not found in world list.\n"
    "        world_ptr = %x; light_ptr = %x\n";
/**
 * Reimplements data 0x4de3e8: g_zClass_LineErrorDeleteSoundWorldNotFoundFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x72].
 * Purpose: report that a sound's attached-world list does not contain the
 * world being removed.
 */
char g_zClass_LineErrorDeleteSoundWorldNotFoundFmt[0x72] =
    "%s: Line %d: ERROR deleting sound; world not found in sound's world list.\n"
    "        world_ptr = %x; sound_ptr = %x\n";
/**
 * Reimplements data 0x4de45c: g_zClass_LineErrorDeleteSoundNotFoundInWorldListFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x64].
 * Purpose: report that a sound node is absent from the world's sound list.
 */
char g_zClass_LineErrorDeleteSoundNotFoundInWorldListFmt[0x64] =
    "%s: Line %d: ERROR deleting sound; not found in world list.\n"
    "        world_ptr = %x; sound_ptr = %x\n";
}

namespace {
    const char kWorldSourceFile[] = "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c";

    /**
     * Original static helper observed in zClass_World grid-coordinate callers
     * (D:\Proj\GameZRecoil\zClass\cls_world.c).
     * Purpose: truncate a floating-point world/grid coordinate to an integer cell coordinate.
     */
    int TruncateToInt(float value) {
        return (int)(value);
    }

    /**
     * Recovered original static helper in D:\Proj\GameZRecoil\zClass\cls_world.c.
     * No standalone retail function; observed caller includes
     * 0x450c60 zClass_World::gwWorldSetVirtualAreaPartition.
     * Purpose: approximate the square root of a squared range through its
     * floating-point exponent bits.
     */
    float ApproximateSqrtFromRangeSq(float rangeSq) {
        int bits = 0;
        memcpy(
            &bits,
            &rangeSq,
            sizeof(bits)
        );
        bits = (bits >> 1) + 0x1fc00000;
        float range = 0.0f;
        memcpy(
            &range,
            &bits,
            sizeof(range)
        );
        return range;
    }

    /**
     * Recovered original static helper in D:\Proj\GameZRecoil\zClass\cls_world.c.
     * No standalone retail function; observed callers include
     * 0x450840 zClass_World::WorldRectToGridIndex.
     * Evidence: world-grid source-cluster callers share the invalid grid-cell
     * sentinel write before and after partition inclusion checks.
     * Purpose: set grid column and row outputs to the invalid cell sentinel.
     */
    void InvalidateGrid(
        int *outGridCol,
        int *outGridRow
    ) {
        *outGridCol = -1;
        *outGridRow = -1;
    }

    /**
     * Recovered original static helper in D:\Proj\GameZRecoil\zClass\cls_world.c.
     * No standalone retail function; observed callers include
     * 0x450f60 zClass_World::AddChildToGridCell and
     * 0x451240 zClass_World::RemoveChildAtGrid.
     * Evidence: grid add/remove source-cluster callers share area-grid indexing.
     * Purpose: return the world area record for a grid column and row.
     */
    zWorldAreaPartial *AreaAt(
        zClass_WorldDataPartial * data,
        int gridCol,
        int gridRow
    ) {
        return &data->areaGridRows[gridRow][gridCol];
    }

    /**
     * Recovered original static helper in D:\Proj\GameZRecoil\zClass\cls_world.c.
     * No standalone retail function; observed caller includes
     * 0x4500b0 zClass_World::RebuildAreaBounds.
     * Evidence: area-bounds source-cluster recomputes min/max Y from
     * gwNodeGetWorldBBoxCorners eight-corner arrays.
     * Purpose: expand an area's Y bounds to include all corners from one child bbox.
     */
    void ExpandAreaYBounds(
        zWorldAreaPartial * area,
        const zBBoxCorners &corners
    ) {
        for (int i = 0; i < 8; ++i) {
            const float y = corners.values[i * 3 + 1];
            if (y < area->bbox[1]) {
                area->bbox[1] = y;
            } else if (y > area->bbox[4]) {
                area->bbox[4] = y;
            }
        }
    }

    /**
     * Recovered original static helper in D:\Proj\GameZRecoil\zClass\cls_world.c.
     * No standalone retail function; observed caller includes
     * 0x4502b0 zClass_World::InitVirtualAreaPartitions.
     * Purpose: move all children from a virtual-area edge cell into a
     * VAP_statics object and reinsert that object into the world grid.
     */
    inline void MoveAreaChildrenToVapStatics(
        zClass_NodePartial * world,
        zWorldAreaPartial * area
    ) {
        if (area->childCount <= 0) {
            return;
        }

        zClass_NodePartial *statics = zClass_Object3D::gwObject3DInit();
        zClass_Class::gwNodeSetName(
            statics,
            g_zClass_VapStaticsNodeName
        );
        while (area->childCount > 0) {
            zClass_NodePartial *child = area->childList[0];
            zClass_Object3D::gwObject3DAddChild(
                statics,
                child
            );
            zClass_World::RemoveChildAtGrid(
                world,
                child
            );
        }

        zClass_TypeList::UpdateQueuedTrees();
        zClass_World::AddChildAtGrid(
            world,
            statics
        );
    }
}

namespace zClass_World {
    int __fastcall
    /**
     * Reimplements 0x4517a0: zClass_World::WriteSettingsSection.
     * BN source path evidence: GameZRecoil/zClass/cls_world.c.
     * Purpose: write each world node's pending fog settings as a ZBD settings
     * section blob.
     */
    WriteSettingsSection(
        zZbdSectionCallbackCtx * callbackCtx,
        void *userData
    ) {
        (void)userData;

        int result = 1;
        zClass_TypeListLink *link = zClass_TypeList::Head(13);
        while (link != 0 && result != 0) {
            zClass_NodePartial *world = link->node;
            zClass_WorldSettingsSectionRecord settings;
            GetPendingFogDensity(
                world,
                &settings.fogDensity
            );
            GetPendingFogState(
                world,
                &settings.fogState
            );
            GetPendingFogColorRgb01(
                world,
                &settings.fogColorRgb01.red,
                &settings.fogColorRgb01.green,
                &settings.fogColorRgb01.blue
            );
            GetPendingFogRange(
                world,
                &settings.fogRangeNear,
                &settings.fogRangeFar
            );
            GetPendingFogAltitudeRange(
                world,
                &settings.fogAltitudeLow,
                &settings.fogAltitudeHigh
            );
            GetPendingFogDensity(
                world,
                &settings.fogDensity
            );
            result =
                zUtil_ZAR::WriteSectionBlob(
                    callbackCtx,
                    world->name,
                    &settings,
                    sizeof(settings)
                );
            link = link->next;
        }

        return result;
    }

    /**
     * Reimplements 0x451840: zClass_World::ReadSettingsSection.
     * BN source path evidence: GameZRecoil/zClass/cls_world.c.
     * Purpose: apply a ZBD settings section record to the named world node's
     * pending fog settings.
     */
    void __fastcall ReadSettingsSection(
        zZbdSectionCallbackCtx * callbackCtx,
        const char *worldName,
        zClass_WorldSettingsSectionRecord *settings,
        unsigned int size,
        void *userData
    ) {
        (void)callbackCtx;
        (void)size;
        (void)userData;

        zClass_NodePartial *world = zClass::FindByTypeAndName(
            13,
            worldName
        );
        if (world == 0) {
            return;
        }

        SetPendingFogDensity(
            world,
            settings->fogDensity
        );
        SetPendingFogState(
            world,
            settings->fogState
        );
        SetPendingFogColorRgb01(
            world,
            settings->fogColorRgb01.red,
            settings->fogColorRgb01.green,
            settings->fogColorRgb01.blue
        );
        SetPendingFogRange(
            world,
            settings->fogRangeNear,
            settings->fogRangeFar
        );
        SetPendingFogAltitudeRange(
            world,
            settings->fogAltitudeLow,
            settings->fogAltitudeHigh
        );
        SetPendingFogDensity(
            world,
            settings->fogDensity
        );
    }

    /**
     * Reimplements 0x4501c0: zClass_World::gwWorldNew.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: allocate and initialize a world node and its class data, then
     * insert it into the world type list.
     */
    zClass_NodePartial *gwWorldNew() {
        zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
        node->classId = 2;

        zClass_WorldDataPartial *data =
            (zClass_WorldDataPartial *)(calloc(
                1,
                sizeof(zClass_WorldDataPartial)
            ));
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
        zClass_TypeList::Insert(
            13,
            node
        );
        return node;
    }

    /**
     * Reimplements 0x450ae0: zClass_World::SetPendingFogState.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending fog enable/linear-mode state for the next
     * world fog application pass.
     */
    int __fastcall SetPendingFogState(
        zClass_NodePartial * world,
        int fogState
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        data->fogState = fogState;
        data->flags |= 0x01;
        return 0;
    }

    /**
     * Reimplements 0x450af0: zClass_World::SetPendingFogColorRgb01.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending fog RGB color values for the next world fog
     * application pass.
     */
    int __fastcall SetPendingFogColorRgb01(
        zClass_NodePartial * world,
        float red,
        float green,
        float blue
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        data->ambientColor.red = red;
        data->ambientColor.blue = blue;
        data->ambientColor.green = green;
        data->flags |= 0x02;
        return 0;
    }

    /**
     * Reimplements 0x450b20: zClass_World::SetPendingFogAltitudeRange.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending vertical fog altitude bounds for the next
     * world fog application pass.
     */
    int __fastcall SetPendingFogAltitudeRange(
        zClass_NodePartial * world,
        float minAlt,
        float maxAlt
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        data->fogHeightHigh = maxAlt;
        data->fogHeightLow = minAlt;
        data->flags |= 0x20;
        return 0;
    }

    /**
     * Reimplements 0x450b40: zClass_World::SetPendingFogRange.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending near and far fog distance range for the next
     * world fog application pass.
     */
    int __fastcall SetPendingFogRange(
        zClass_NodePartial * world,
        float nearRange,
        float farRange
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        data->fogDistanceStart = nearRange;
        data->fogDistanceEnd = farRange;
        data->flags |= 0x04;
        return 0;
    }

    /**
     * Reimplements 0x450b80: zClass_World::GetPendingFogDensity.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged fog density value from the world data.
     */
    int __fastcall GetPendingFogDensity(
        zClass_NodePartial * world,
        float *outDensity
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        *outDensity = data->fogDensity;
        return 0;
    }

    /**
     * Reimplements 0x450b90: zClass_World::GetPendingFogState.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged fog state from the world data.
     */
    int __fastcall GetPendingFogState(
        zClass_NodePartial * world,
        int *outState
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        *outState = data->fogState;
        return 0;
    }

    /**
     * Reimplements 0x450ba0: zClass_World::GetPendingFogColorRgb01.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged fog RGB color values from the world data.
     */
    int __fastcall GetPendingFogColorRgb01(
        zClass_NodePartial * world,
        float *outRed,
        float *outGreen,
        float *outBlue
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        *outRed = data->ambientColor.red;
        *outGreen = data->ambientColor.green;
        *outBlue = data->ambientColor.blue;
        return 0;
    }

    /**
     * Reimplements 0x450bc0: zClass_World::GetPendingFogRange.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged near and far fog distance range from the
     * world data.
     */
    int __fastcall GetPendingFogRange(
        zClass_NodePartial * world,
        float *outNearRange,
        float *outFarRange
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        *outNearRange = data->fogDistanceStart;
        *outFarRange = data->fogDistanceEnd;
        return 0;
    }

    /**
     * Reimplements 0x450be0: zClass_World::GetPendingFogAltitudeRange.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged vertical fog altitude bounds from the world
     * data.
     */
    int __fastcall GetPendingFogAltitudeRange(
        zClass_NodePartial * world,
        float *outMinAlt,
        float *outMaxAlt
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        *outMaxAlt = data->fogHeightHigh;
        *outMinAlt = data->fogHeightLow;
        return 0;
    }

    /**
     * Reimplements 0x450b60: zClass_World::SetPendingFogDensity.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending fog density for the next world fog
     * application pass.
     */
    int __fastcall SetPendingFogDensity(
        zClass_NodePartial * world,
        float density
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        data->fogDensity = density;
        data->flags |= 0x08;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x450c00: zClass_World::gwWorldSetOrigin.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: set the world origin and update the derived maximum X/Z
     * bounds.
     */
    gwWorldSetOrigin(
        zClass_NodePartial * world,
        float originX,
        float originZ
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        data->originX = originX;
        data->originZ = originZ;
        data->worldMaxX = data->worldSizeX + originX;
        data->worldMaxZ = data->worldSizeZ + originZ;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x450c30: zClass_World::gwWorldSetSize.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: set the world X/Z size and update the derived maximum bounds.
     */
    gwWorldSetSize(
        zClass_NodePartial * world,
        float sizeX,
        float sizeZ
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        data->worldSizeX = sizeX;
        data->worldSizeZ = sizeZ;
        data->worldMaxX = data->originX + sizeX;
        data->worldMaxZ = data->originZ + sizeZ;
        return 0;
    }

    /**
     * Reimplements 0x450f00: zClass_World::gwWorldSetPartitionInclusionTolerance.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: set the X/Z tolerances used when testing partition inclusion.
     */
    int __fastcall gwWorldSetPartitionInclusionTolerance(
        zClass_NodePartial * world,
        float toleranceX,
        float toleranceZ
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        data->partitionInclusionTolX = toleranceX;
        data->partitionInclusionTolZ = toleranceZ;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x450f20: zClass_World::gwWorldSetMaxDecFeatures.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: clamp and store the maximum DEC feature count for world
     * partitions.
     */
    gwWorldSetMaxDecFeatures(
        zClass_NodePartial * world,
        int maxFeatures
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        if (maxFeatures > 255) {
            zError::ReportOld(
                0x200,
                kWorldSourceFile,
                0xc01,
                g_zClass_PartitionMaxDecFeatureCountOverflowFmt,
                maxFeatures
            );
            maxFeatures = 255;
        }

        data->partitionMaxDecFeatureCount = (unsigned char)(maxFeatures);
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x450c60: zClass_World::gwWorldSetVirtualAreaPartition.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: allocate and initialize the virtual area partition grid and
     * its cell metrics from the configured world bounds.
     */
    gwWorldSetVirtualAreaPartition(
        zClass_NodePartial * world,
        float cellSizeX,
        float cellSizeZ
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
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
        if ((float)(gridColCount)*data->areaCellSizeX < data->worldSizeX) {
            ++gridColCount;
            data->areaGridColCount = gridColCount;
        }

        int gridRowCount = TruncateToInt(data->worldSizeZ / data->areaCellSizeZ);
        data->areaGridRowCount = gridRowCount;
        if ((float)(gridRowCount)*data->areaCellSizeZ > data->worldSizeZ) {
            ++gridRowCount;
            data->areaGridRowCount = gridRowCount;
        }

        data->areaGridRows =
            (zWorldAreaPartial **)(calloc(
                data->areaGridRowCount,
                sizeof(zWorldAreaPartial *)
            ));
        for (int row = 0; row < data->areaGridRowCount; ++row) {
            data->areaGridRows[row] =
                (zWorldAreaPartial *)(calloc(
                    data->areaGridColCount,
                    sizeof(zWorldAreaPartial)
                ));
        }

        for (int initRow = 0; initRow < data->areaGridRowCount; ++initRow) {
            const float rowAsFloat = (float)(initRow);
            for (int col = 0; col < data->areaGridColCount; ++col) {
                zWorldAreaPartial *area = &data->areaGridRows[initRow][col];
                area->areaFlags |= 0x100;
                area->cellMinX = (float)(col)*data->areaCellSizeX + data->originX;
                area->cellMinZ = rowAsFloat * data->areaCellSizeZ + data->originZ;
                area->bbox[0] = area->cellMinX;
                area->bbox[3] = area->cellMinX + data->areaCellSizeX;
                area->bbox[5] = area->cellMinZ;
                area->bbox[2] = area->cellMinZ + data->areaCellSizeZ;
                BBox::MinMaxToBoundingSphere(
                    (const zBBox3f *)(area->bbox),
                    &area->bboxCenter,
                    &area->bboxRadius
                );
                area->areaIndex = -1;
            }
        }

        return 0;
    }

    /**
     * Reimplements 0x4502b0: zClass_World::InitVirtualAreaPartitions.
     * BN source path evidence: GameZRecoil/zClass/cls_world.c.
     * Purpose: initialize virtual area partition edge cells by moving their
     * children into VAP_statics nodes.
     */
    int __fastcall InitVirtualAreaPartitions(zClass_NodePartial * world) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        if (data->areaGridRows == 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                g_zClass_LineErrorVirtualAreaPartitionNullFmt,
                kWorldSourceFile,
                0x245
            );
            zError::EmitDebugBuffer(5);
            return 5;
        }

        zClass_TypeList::UpdateQueuedTrees();

        for (int col = 0; col < data->areaGridColCount; ++col) {
            MoveAreaChildrenToVapStatics(
                world,
                &data->areaGridRows[0][col]
            );
        }

        zWorldAreaPartial *lastRow = data->areaGridRows[data->areaGridRowCount - 1];
        for (int lastCol = 0; lastCol < data->areaGridColCount; ++lastCol) {
            MoveAreaChildrenToVapStatics(
                world,
                &lastRow[lastCol]
            );
        }

        for (int firstEdgeRow = 1; firstEdgeRow < data->areaGridRowCount - 1; ++firstEdgeRow) {
            MoveAreaChildrenToVapStatics(
                world,
                &data->areaGridRows[firstEdgeRow][0]
            );
        }

        for (int lastEdgeRow = 1; lastEdgeRow < data->areaGridRowCount - 1; ++lastEdgeRow) {
            MoveAreaChildrenToVapStatics(
                world,
                &data->areaGridRows[lastEdgeRow][data->areaGridColCount - 1]
            );
        }

        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x450510: zClass_World::SetVirtualPartition.
     * BN source path evidence: GameZRecoil/zClass/cls_world.c.
     * Purpose: set the virtual-partition query flag and initialize partitions
     * when enabling the mode.
     */
    SetVirtualPartition(
        zClass_NodePartial * world,
        int enabled
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        data->clampQueriesToBounds = enabled;
        if (enabled != 0) {
            InitVirtualAreaPartitions(world);
        }
        return 0;
    }

    /**
     * Reimplements 0x450e40: zClass_World::FreeVirtualAreaPartitions.
     * Purpose: release virtual-area child lists and owned grid storage, then
     * clear the installed partition metrics.
     */
    int __fastcall FreeVirtualAreaPartitions(zClass_NodePartial * world) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        if (data->areaGridRows == 0) {
            return 0;
        }

        {
            int row = 0;
            zWorldAreaPartial **rowCursor = data->areaGridRows;
            if (data->areaGridRowCount > 0) {
                do {
                    zWorldAreaPartial *area = *rowCursor;
                    int col = 0;
                    if (data->areaGridColCount > 0) {
                        do {
                            if (area->childList != 0) {
                                free(area->childList);
                                area->childList = 0;
                            }
                            ++area;
                            ++col;
                        } while (col < data->areaGridColCount);
                    }

                    if (data->areaGridExternalOwnership == 0) {
                        free(*rowCursor);
                    }
                    ++rowCursor;
                    ++row;
                } while (row < data->areaGridRowCount);
            }
        }

        if (data->areaGridExternalOwnership == 0) {
            free(data->areaGridRows);
            data->areaGridRows = 0;
        }

        data->areaGridRows = 0;
        data->areaCellSizeZ = 0.0f;
        data->areaCellSizeX = 0.0f;
        data->areaGridRowCount = 0;
        data->areaGridColCount = 0;
        return 0;
    }

    /**
     * Reimplements 0x450240: zClass_World::DeleteNode.
     * Purpose: release world-owned partition/light/sound/update lists and
     * return the world node to the shared zClass free-list machinery.
     */
    int __fastcall DeleteNode(zClass_NodePartial * world) {
        const int freeResult = FreeVirtualAreaPartitions(world);
        if (freeResult != 0) {
            return freeResult;
        }

        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
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

    /**
     * Reimplements 0x450030: zClass_World::QueueAreaUpdate
     * Evidence: Binary Ninja/original source path D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: Queues a pending area update record, marks the area pending, and sets world update flags for later processing.
     */
    int __fastcall QueueAreaUpdate(
        zClass_NodePartial * world,
        zClass_WorldDataPartial * worldData,
        zWorldAreaPartial * area
    ) {
        if (worldData->pendingAreaUpdateCount == worldData->pendingAreaUpdateCapacity) {
            worldData->pendingAreaUpdates = (zWorldAreaPartial **)(realloc(
                worldData->pendingAreaUpdates,
                (worldData->pendingAreaUpdateCapacity + 1) * sizeof(zWorldAreaPartial *)
            ));
            ++worldData->pendingAreaUpdateCapacity;
        }

        worldData->pendingAreaUpdates[worldData->pendingAreaUpdateCount] = area;
        ++worldData->pendingAreaUpdateCount;
        area->areaFlags |= 0x01;

        if ((world->flags & 0x01) == 0) {
            if (zClass_TypeList::InsertChildNodes(
                7,
                world
            ) == 0) {
                world->flags |= 0x01;
            }
        }
        world->flags |= 0x02;
        worldData->flags |= 0x10;
        return 0;
    }

    /**
     * Reimplements 0x4500b0: zClass_World::RebuildAreaBounds.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: recompute an area's active Y bounds and bounding sphere from
     * child world bounding boxes.
     */
    int __fastcall RebuildAreaBounds(zClass_WorldDataPartial * /*worldData*/, zWorldAreaPartial * area) {
        const short childCount = area->childCount;
        // Recomputes bbox-present flag 0x100; ApplyPendingFogSettings clears
        // dirty flag 0x01 after this helper.
        area->areaFlags &= ~0x100;
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
            zClass_Class::gwNodeGetWorldBBoxCorners(
                child,
                &corners
            );
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

            zClass_Class::gwNodeGetWorldBBoxCorners(
                child,
                &corners
            );
            ExpandAreaYBounds(
                area,
                corners
            );
        }

        BBox::MinMaxToBoundingSphere(
            (const zBBox3f *)(area->bbox),
            &area->bboxCenter,
            &area->bboxRadius
        );
        return 0;
    }

    /**
     * Reimplements 0x450530: zClass_World::ApplyPendingFogSettings.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: apply staged world fog changes and queued area-bound updates,
     * then clear the pending flags.
     */
    int __fastcall ApplyPendingFogSettings(zClass_NodePartial * world) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
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
                RebuildAreaBounds(
                    data,
                    area
                );
                ++pendingAreaUpdates;
                area->areaFlags &= ~0x01;
                --data->pendingAreaUpdateCount;
            } while (data->pendingAreaUpdateCount > 0);
        }

        data->flags = 0;
        return 0;
    }

    /**
     * Reimplements 0x450840: zClass_World::WorldRectToGridIndex.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: convert a world-space X/Z rectangle to a valid grid cell when
     * it fits inside the partition inclusion tolerances.
     */
    int __fastcall WorldRectToGridIndex(
        zClass_NodePartial * world,
        int *outGridCol,
        float minX,
        float maxX,
        float minZ,
        float maxZ,
        int *outGridRow
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        InvalidateGrid(
            outGridCol,
            outGridRow
        );

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
            InvalidateGrid(
                outGridCol,
                outGridRow
            );
        } else if (maxX > cellMaxX && maxX - cellMaxX > data->partitionInclusionTolX) {
            InvalidateGrid(
                outGridCol,
                outGridRow
            );
        } else if (minZ < cellMaxZ && cellMaxZ - minZ > data->partitionInclusionTolZ) {
            InvalidateGrid(
                outGridCol,
                outGridRow
            );
        } else if (maxZ > gridCell->cellMinZ &&
                   maxZ - gridCell->cellMinZ > data->partitionInclusionTolZ) {
            InvalidateGrid(
                outGridCol,
                outGridRow
            );
        }

        return 0;
    }

    /**
     * Reimplements 0x450650: zClass_World::WorldToGridCoordsClampedEx.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: clamp world X/Z coordinates to valid grid coordinates while also returning unclamped grid coordinates and an inside-bounds flag.
     */
    int __fastcall WorldToGridCoordsClampedEx(
        zClass_NodePartial * world,
        int *outGridCol,
        float worldX,
        float worldZ,
        int *outGridRow,
        int *clampedGridColOut,
        int *clampedGridRowOut,
        int *insideBoundsOut
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);

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

        *outGridCol = (int)(floor((worldX - data->originX) * data->areaInvSizeX));
        *outGridRow = (int)(floor((worldZ - data->originZ) * data->areaInvSizeZ));
        return 0;
    }

    /**
     * Reimplements 0x450790: zClass_World::WorldToGridCoordsClamped.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: clamp a world X/Z position to the world's grid extents and return the corresponding grid coordinates.
     */
    int __fastcall WorldToGridCoordsClamped(
        zClass_NodePartial * world,
        int *outGridCol,
        float worldX,
        float worldZ,
        int *outGridRow
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);

        float clampedX;
        if (worldX < data->originX + 0.1f) {
            clampedX = data->originX - 0.1f;
        } else {
            clampedX = data->worldMaxX - 0.1f;
            if (worldX < clampedX) {
                clampedX = worldX;
            }
        }

        const float minZ = data->originZ < data->worldMaxZ ? data->originZ : data->worldMaxZ;
        const float maxZ = data->originZ < data->worldMaxZ ? data->worldMaxZ : data->originZ;
        float clampedZ;
        if (worldZ < minZ - 0.1f) {
            clampedZ = minZ - 0.1f;
        } else {
            clampedZ = maxZ + 0.1f;
            if (worldZ < clampedZ) {
                clampedZ = worldZ;
            }
        }

        *outGridCol = TruncateToInt((clampedX - data->originX) * data->areaInvSizeX);
        *outGridRow = TruncateToInt((clampedZ - data->originZ) * data->areaInvSizeZ);
        return 0;
    }

    /**
     * Reimplements 0x450a00: zClass_World::GetAreaPartitionAtGrid.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: validate the world node/data pointers and return the area
     * partition at a grid column and row.
     */
    zWorldAreaPartial *__fastcall GetAreaPartitionAtGrid(
        zClass_NodePartial * world,
        int gridCol,
        int gridRow
    ) {
        if (world == 0) {
            zError::ReportOld(
                0x400,
                kWorldSourceFile,
                0x6d4,
                "Null node pointer."
            );
            return 0;
        }

        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        if (data == 0) {
            zError::ReportOld(
                0x400,
                kWorldSourceFile,
                0x6d5,
                "Null class data pointer"
            );
            return 0;
        }

        return AreaAt(
            data,
            gridCol,
            gridRow
        );
    }

    /**
     * Reimplements 0x450a70: zClass_World::EnsureGridCellDisplayPosition.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: ensure a grid cell is queued for display-position/bounds
     * refresh when it is not already pending.
     */
    int __fastcall EnsureGridCellDisplayPosition(
        zClass_NodePartial * world,
        int gridCol,
        int gridRow
    ) {
        if (world == 0) {
            zError::ReportOld(
                0x400,
                kWorldSourceFile,
                0x6f5,
                "Null node pointer."
            );
            return 5;
        }

        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        if (data == 0) {
            zError::ReportOld(
                0x400,
                kWorldSourceFile,
                0x6f6,
                "Null class data pointer"
            );
            return 5;
        }

        zWorldAreaPartial *area = &data->areaGridRows[gridRow][gridCol];
        if ((area->areaFlags & 0x01) == 0) {
            return QueueAreaUpdate(
                world,
                data,
                area
            );
        }

        return 0;
    }

    /**
     * Reimplements 0x4510e0: zClass_World::AddChildAtGrid.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: derive the child's world grid cell from bounds or world extent
     * and route insertion into the world child-link storage.
     */
    int __fastcall AddChildAtGrid(
        zClass_NodePartial * world,
        zClass_NodePartial * child
    ) {
        int gridCol = -1;
        int gridRow = -1;

        if ((child->flags & 0x80) == 0) {
            float minX = 0.0f;
            float maxX = 0.0f;
            float minZ = 0.0f;
            float maxZ = 0.0f;

            if ((child->flags & 0x100) != 0) {
                zBBoxCorners corners = {0};
                zClass_Class::gwNodeGetWorldBBoxCorners(
                    child,
                    &corners
                );
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
                zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
                minX = data->originX;
                minZ = data->originZ;
                maxX = data->originX + data->worldSizeX;
                maxZ = data->originZ + data->worldSizeZ;
            }

            WorldRectToGridIndex(
                world,
                &gridCol,
                minX,
                maxX,
                minZ,
                maxZ,
                &gridRow
            );
        }

        return AddChildToGridCell(
            world,
            child,
            gridCol,
            gridRow
        );
    }

    /**
     * Reimplements 0x450f60: zClass_World::AddChildToGridCell.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: add a child to either the overflow world list or a grid area
     * list while maintaining the child's listA parent ownership.
     */
    int __fastcall AddChildToGridCell(
        zClass_NodePartial * world,
        zClass_NodePartial * child,
        int gridCol,
        int gridRow
    ) {
        int result;
        zClass_WorldDataPartial *data;

        data = (zClass_WorldDataPartial *)(world->classData);
        result = 0;

        if (gridCol >= 0 && gridRow >= 0) {
            if (data->areaGridRows[gridRow][gridCol].childCount >= 0x7fff) {
                gridRow = -1;
                gridCol = -1;
            }
        }

        if (gridCol < 0 || gridRow < 0) {
            int listCount = world->listCountB + 1;
            int listBytes = listCount * sizeof(zClass_NodePartial *);
            world->listB = (zClass_NodePartial **)(realloc(
                world->listB,
                listBytes
            ));
            world->listB[listCount - 1] = child;
            ++world->listCountB;
            child->gridCol = -1;
            child->gridRow = -1;
            int parentCount = child->listCountA + 1;
            int parentBytes = parentCount * sizeof(zClass_NodePartial *);
            child->listA = (zClass_NodePartial **)(realloc(
                child->listA,
                parentBytes
            ));
            child->listA[parentCount - 1] = world;
            ++child->listCountA;
            if (child->listCountA > 1) {
                zClass_Class::SetSingleParentFlagRecursive(
                    child,
                    0
                );
            }
        } else {
            zWorldAreaPartial *area = &data->areaGridRows[gridRow][gridCol];
            int areaCount = (int)(area->childCount) + 1;
            int areaBytes = areaCount * sizeof(zClass_NodePartial *);
            area->childList = (zClass_NodePartial **)(realloc(
                area->childList,
                areaBytes
            ));
            area->childList[areaCount - 1] = child;
            ++area->childCount;

            child->gridCol = gridCol;
            child->gridRow = gridRow;
            int parentCount = child->listCountA + 1;
            int parentBytes = parentCount * sizeof(zClass_NodePartial *);
            child->listA = (zClass_NodePartial **)(realloc(
                child->listA,
                parentBytes
            ));
            child->listA[parentCount - 1] = world;
            ++child->listCountA;
            if (child->listCountA > 1) {
                zClass_Class::SetSingleParentFlagRecursive(
                    child,
                    0
                );
            }

            if ((area->areaFlags & 0x01) == 0) {
                result = QueueAreaUpdate(
                    world,
                    data,
                    area
                );
            } else {
                result = 0;
            }
        }

        return result;
    }

    /**
     * Reimplements 0x451240: zClass_World::RemoveChildAtGrid.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: remove a child from the world overflow list or its grid area
     * list while clearing the child's parent/grid ownership state.
     */
    int __fastcall RemoveChildAtGrid(
        zClass_NodePartial * world,
        zClass_NodePartial * child
    ) {
        const int gridCol = child->gridCol;
        const int gridRow = child->gridRow;
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);

        if (gridCol == -1 && gridRow == -1) {
            return zClass_Class::RemoveChildGeneric(
                world,
                child
            );
        }

        zWorldAreaPartial *area = &data->areaGridRows[gridRow][gridCol];
        int childIndex = -1;
        for (int i = 0; i < area->childCount; ++i) {
            if (area->childList[i] == child) {
                childIndex = i;
                break;
            }
        }

        if (childIndex < 0) {
            zError::ReportOld(
                0x200,
                kWorldSourceFile,
                0xfaf,
                "ERROR deleting child node %s from parent node %s",
                child,
                world
            );
            return 1;
        }

        for (int areaIndex = childIndex; areaIndex < area->childCount - 1; ++areaIndex) {
            area->childList[areaIndex] = area->childList[areaIndex + 1];
        }
        --area->childCount;

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
            for (int listIndex = parentIndex; listIndex < child->listCountA - 1; ++listIndex) {
                child->listA[listIndex] = child->listA[listIndex + 1];
            }
            --child->listCountA;
        }

        if ((area->areaFlags & 0x01) == 0) {
            return QueueAreaUpdate(
                world,
                data,
                area
            );
        }

        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x451360: zClass_World::AddLight.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: append a light and its data to the world lists and attach the
     * world to the light's world list.
     */
    AddLight(
        zClass_NodePartial * world,
        zClass_NodePartial * light
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        zClass_LightDataPartial *lightData = (zClass_LightDataPartial *)(light->classData);

        const int lightListBytes = (data->lightCount + 1) * sizeof(zClass_NodePartial *);
        data->lightNodes = (zClass_NodePartial **)(realloc(
            data->lightNodes,
            lightListBytes
        ));
        data->lightNodes[data->lightCount] = light;

        data->lightDataList =
            (zClass_LightDataPartial **)(realloc(
                data->lightDataList,
                lightListBytes
            ));
        data->lightDataList[data->lightCount] = lightData;
        ++data->lightCount;

        lightData->attachedWorlds = (zClass_NodePartial **)(realloc(
            lightData->attachedWorlds,
            (lightData->attachedWorldCount + 1) * sizeof(zClass_NodePartial *)
        ));
        lightData->attachedWorlds[lightData->attachedWorldCount] = world;
        ++lightData->attachedWorldCount;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x451410: zClass_World::RemoveLight.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: remove a light from the world lists and remove the world from
     * the light's attached-world list.
     */
    RemoveLight(
        zClass_NodePartial * world,
        zClass_NodePartial * light
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);

        int lightIndex = -1;
        for (int i = 0; i < data->lightCount; ++i) {
            if (data->lightNodes[i] == light) {
                lightIndex = i;
                break;
            }
        }

        if (lightIndex < 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                g_zClass_LineErrorDeleteLightNotFoundInWorldListFmt,
                kWorldSourceFile,
                0x108d,
                (unsigned int)((unsigned int)(world)),
                (unsigned int)((unsigned int)(light))
            );
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
                g_zClass_LineErrorDeleteLightWorldNotFoundFmt,
                kWorldSourceFile,
                0x10b4,
                (unsigned int)((unsigned int)(world)),
                (unsigned int)((unsigned int)(light))
            );
            zError::EmitDebugBuffer(5);
            return 5;
        }

        for (int i_707 = worldIndex; i_707 < lightData->attachedWorldCount - 1; ++i_707) {
            lightData->attachedWorlds[i_707] = lightData->attachedWorlds[i_707 + 1];
        }
        --lightData->attachedWorldCount;

        return 0;
    }

    /**
     * Reimplements 0x451540: zClass_World::InitLightPointInPolygonXZ.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: forward the world's light data/node lists and count into
     * zModel_Light_PointInPolygonInitXZ.
     */
    int __fastcall InitLightPointInPolygonXZ(zClass_NodePartial * world) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        zModel_Light_PointInPolygonInitXZ(
            data->lightDataList,
            (zModel_LightStatePartial **)(data->lightNodes),
            data->lightCount
        );
        return 0;
    }

    /**
     * Reimplements 0x451560: zClass_World::UpdateAllLights.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: iterate the world light nodes and call
     * zClass_Light::gwLightUpdate for each.
     */
    int __fastcall UpdateAllLights(zClass_NodePartial * world) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);

        for (int i = 0; i < data->lightCount; ++i) {
            zClass_Light::gwLightUpdate(data->lightNodes[i]);
        }

        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x451590: zClass_World::AddSound.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: append a sound and its data to the world lists and attach the
     * world to the sound's world list.
     */
    AddSound(
        zClass_NodePartial * world,
        zClass_NodePartial * sound
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);
        zClass_SoundDataPartial *soundData = (zClass_SoundDataPartial *)(sound->classData);

        const int soundListBytes = (data->soundCount + 1) * sizeof(zClass_NodePartial *);
        data->soundNodes = (zClass_NodePartial **)(realloc(
            data->soundNodes,
            soundListBytes
        ));
        data->soundNodes[data->soundCount] = sound;

        data->soundDataList =
            (zClass_SoundDataPartial **)(realloc(
                data->soundDataList,
                soundListBytes
            ));
        data->soundDataList[data->soundCount] = soundData;
        ++data->soundCount;

        soundData->attachedWorlds = (zClass_NodePartial **)(realloc(
            soundData->attachedWorlds,
            (soundData->attachedWorldCount + 1) * sizeof(zClass_NodePartial *)
        ));
        soundData->attachedWorlds[soundData->attachedWorldCount] = world;
        ++soundData->attachedWorldCount;
        return 0;
    }

    int __fastcall
    /**
     * Reimplements 0x451640: zClass_World::RemoveSound.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: remove a sound from the world lists and remove the world from
     * the sound's attached-world list.
     */
    RemoveSound(
        zClass_NodePartial * world,
        zClass_NodePartial * sound
    ) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);

        int soundIndex = -1;
        for (int i = 0; i < data->soundCount; ++i) {
            if (data->soundNodes[i] == sound) {
                soundIndex = i;
                break;
            }
        }

        if (soundIndex < 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                g_zClass_LineErrorDeleteSoundNotFoundInWorldListFmt,
                kWorldSourceFile,
                0x11cc,
                (unsigned int)((unsigned int)(world)),
                (unsigned int)((unsigned int)(sound))
            );
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
                g_zClass_LineErrorDeleteSoundWorldNotFoundFmt,
                kWorldSourceFile,
                0x11f3,
                (unsigned int)((unsigned int)(world)),
                (unsigned int)((unsigned int)(sound))
            );
            zError::EmitDebugBuffer(5);
            return 5;
        }

        for (int i_815 = worldIndex; i_815 < soundData->attachedWorldCount - 1; ++i_815) {
            soundData->attachedWorlds[i_815] = soundData->attachedWorlds[i_815 + 1];
        }
        --soundData->attachedWorldCount;

        return 0;
    }

    /**
     * Reimplements 0x451770: zClass_World::UpdateAllSounds.
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: iterate the world sound nodes and call
     * zClass_Sound::UpdatePlayback for each.
     */
    int __fastcall UpdateAllSounds(zClass_NodePartial * world) {
        zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(world->classData);

        for (int i = 0; i < data->soundCount; ++i) {
            zClass_Sound::UpdatePlayback(data->soundNodes[i]);
        }

        return 0;
    }
}
