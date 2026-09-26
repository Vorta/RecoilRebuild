#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zUtil/zbd.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.g-zclass-lineerrorvirtualareapartitionnullfmt
 * @recoil-artifact defines .data recoil:data:0x4de23c: g_CZClass_LineErrorVirtualAreaPartitionNullFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x5b].
 *
 * Purpose: report a missing virtual-area partition grid during world
 * partition initialization.
 */
char g_CZClass_LineErrorVirtualAreaPartitionNullFmt[0x5b]
    = "%s: Line %d: ERROR initializing virtual area partition; NULL area partitions encountered.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.g-zclass-partitionmaxdecfeaturecountoverflowfmt
 * @recoil-artifact defines .data recoil:data:0x4de2c0: g_CZClass_PartitionMaxDecFeatureCountOverflowFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x4d].
 * Purpose: report clamping of the maximum DEC feature count to the byte-sized
 * partition storage limit.
 */
char g_CZClass_PartitionMaxDecFeatureCountOverflowFmt[0x4d] = "ERROR setting Partition Max DEC Feature count to %d:\n"
                                                              "overflow limit at 255.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.g-zclass-lineerrordeletelightworldnotfoundfmt
 * @recoil-artifact defines .data recoil:data:0x4de310: g_CZClass_LineErrorDeleteLightWorldNotFoundFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x72].
 * Purpose: report that a light's attached-world list does not contain the
 * world being removed.
 */
char g_CZClass_LineErrorDeleteLightWorldNotFoundFmt[0x72]
    = "%s: Line %d: ERROR deleting light; world not found in light's world list.\n"
      "        world_ptr = %x; light_ptr = %x\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.g-zclass-lineerrordeletelightnotfoundinworldlistfmt
 * @recoil-artifact defines .data recoil:data:0x4de384: g_CZClass_LineErrorDeleteLightNotFoundInWorldListFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x64].
 * Purpose: report that a light node is absent from the world's light list.
 */
char g_CZClass_LineErrorDeleteLightNotFoundInWorldListFmt[0x64]
    = "%s: Line %d: ERROR deleting light; not found in world list.\n"
      "        world_ptr = %x; light_ptr = %x\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.g-zclass-lineerrordeletesoundworldnotfoundfmt
 * @recoil-artifact defines .data recoil:data:0x4de3e8: g_CZClass_LineErrorDeleteSoundWorldNotFoundFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x72].
 * Purpose: report that a sound's attached-world list does not contain the
 * world being removed.
 */
char g_CZClass_LineErrorDeleteSoundWorldNotFoundFmt[0x72]
    = "%s: Line %d: ERROR deleting sound; world not found in sound's world list.\n"
      "        world_ptr = %x; sound_ptr = %x\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.g-zclass-lineerrordeletesoundnotfoundinworldlistfmt
 * @recoil-artifact defines .data recoil:data:0x4de45c: g_CZClass_LineErrorDeleteSoundNotFoundInWorldListFmt.
 * BN data inventory declares writable cls_world.c diagnostic literal char[0x64].
 * Purpose: report that a sound node is absent from the world's sound list.
 */
char g_CZClass_LineErrorDeleteSoundNotFoundInWorldListFmt[0x64]
    = "%s: Line %d: ERROR deleting sound; not found in world list.\n"
      "        world_ptr = %x; sound_ptr = %x\n";
}

namespace
{

    /**
     * Original static helper observed in CZWorld grid-coordinate callers
     * (D:\Proj\GameZRecoil\zClass\cls_world.c).
     * Purpose: truncate a floating-point world/grid coordinate to an integer cell coordinate.
     */
    int TruncateToInt(float value)
    {
        return (int)(value);
    }

    /**
     * Recovered original static helper in D:\Proj\GameZRecoil\zClass\cls_world.c.
     * No standalone retail function; observed caller includes
     * 0x450c60 CZWorld::gwWorldSetVirtualAreaPartition.
     * Purpose: approximate the square root of a squared range through its
     * floating-point exponent bits.
     */
    float ApproximateSqrtFromRangeSq(float rangeSq)
    {
        int bits = 0;
        memcpy(&bits, &rangeSq, sizeof(bits));
        bits = (bits >> 1) + 0x1fc00000;
        float range = 0.0f;
        memcpy(&range, &bits, sizeof(range));
        return range;
    }

    /**
     * Recovered original static helper in D:\Proj\GameZRecoil\zClass\cls_world.c.
     * No standalone retail function; observed callers include
     * 0x450840 CZWorld::WorldRectToGridIndex.
     * Evidence: world-grid source-cluster callers share the invalid grid-cell
     * sentinel write before and after partition inclusion checks.
     * Purpose: set grid column and row outputs to the invalid cell sentinel.
     */
    void InvalidateGrid(int* outGridCol, int* outGridRow)
    {
        *outGridCol = -1;
        *outGridRow = -1;
    }

    /**
     * Recovered original static helper in D:\Proj\GameZRecoil\zClass\cls_world.c.
     * No standalone retail function; observed callers include
     * 0x450f60 CZWorld::AddChildToGridCell and
     * 0x451240 CZWorld::RemoveChildAtGrid.
     * Evidence: grid add/remove source-cluster callers share area-grid indexing.
     * Purpose: return the world area record for a grid column and row.
     */
    zWorldAreaPartial* AreaAt(CZWorldDataPartial * data, int gridCol, int gridRow)
    {
        return &data->areaGridRows[gridRow][gridCol];
    }
}

namespace CZWorld
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.queueareaupdate
     * @recoil-artifact defines .text recoil:function:0x450030: CZWorld::QueueAreaUpdate
     * @recoil-match byte
     *
     * Evidence: retail literal-backed physical source block D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: Queues a pending area update record, marks the area pending, and sets world update flags for later
     * processing.
     */
    int __fastcall QueueAreaUpdate(CZNodePartial * world, CZWorldDataPartial * worldData, zWorldAreaPartial * area)
    {
        if (worldData->pendingAreaUpdateCount == worldData->pendingAreaUpdateCapacity) {
            worldData->pendingAreaUpdates = (zWorldAreaPartial**)(realloc(
                worldData->pendingAreaUpdates,
                (worldData->pendingAreaUpdateCapacity + 1) * sizeof(zWorldAreaPartial*)
            ));
            ++worldData->pendingAreaUpdateCapacity;
        }

        worldData->pendingAreaUpdates[worldData->pendingAreaUpdateCount] = area;
        ++worldData->pendingAreaUpdateCount;
        area->areaFlags |= 0x01;

        if ((world->flags & 0x01) == 0) {
            if (CZTypeList::InsertChildNodes(7, world) == 0) {
                world->flags |= 0x01;
            }
        }
        world->flags |= 0x02;
        worldData->flags |= 0x10;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.rebuildareabounds
     * @recoil-artifact defines .text recoil:function:0x4500b0: CZWorld::RebuildAreaBounds.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: recompute an area's active Y bounds and bounding sphere from
     * child world bounding boxes.
     */
    int __fastcall RebuildAreaBounds(CZWorldDataPartial* /*worldData*/, zWorldAreaPartial * area)
    {
        const short childCount = area->childCount;
        // Recomputes bbox-present flag 0x100; ApplyPendingFogSettings clears
        // dirty flag 0x01 after this helper.
        area->areaFlags &= ~0x100;
        if (childCount == 0) {
            return 0;
        }

        zBBoxCorners corners = { 0 };
        int childIndex = 0;
        for (; childIndex < childCount; ++childIndex) {
            CZNodePartial* child = area->childList[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            area->areaFlags |= 0x100;
            CZClass::gwNodeGetWorldBBoxCorners(child, &corners);
            area->bbox[1] = corners.corners[0].y;
            area->bbox[4] = corners.corners[0].y;
            for (int i = 1; i < 8; ++i) {
                const float y = corners.corners[i].y;
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
            CZNodePartial* child = area->childList[childIndex];
            if ((child->flags & 0x100) == 0) {
                continue;
            }

            CZClass::gwNodeGetWorldBBoxCorners(child, &corners);
            for (int i = 0; i < 8; ++i) {
                const float y = corners.corners[i].y;
                if (y < area->bbox[1]) {
                    area->bbox[1] = y;
                } else if (y > area->bbox[4]) {
                    area->bbox[4] = y;
                }
            }
        }

        CZBBox::MinMaxToBoundingSphere((const zBBox3f*)(area->bbox), &area->bboxCenter, &area->bboxRadius);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.gwworldnew
     * @recoil-artifact defines .text recoil:function:0x4501c0: CZWorld::gwWorldNew.
     * @recoil-match byte
     *
     * Purpose: allocate a world node and its class data, then add it to the world type list.
     */
    CZNodePartial* __cdecl gwWorldNew()
    {
        CZNodePartial* node = CZClass::gwNodeNew();
        node->classId = 2;

        CZWorldDataPartial* data = (CZWorldDataPartial*)(calloc(1, sizeof(CZWorldDataPartial)));
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
        CZTypeList::Insert(13, node);
        return node;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.deletenode
     * @recoil-artifact defines .text recoil:function:0x450240: CZWorld::DeleteNode.
     * @recoil-match byte
     *
     * Purpose: release world-owned partition/light/sound/update lists and
     * return the world node to the shared zClass free-list machinery.
     */
    int __fastcall DeleteNode(CZNodePartial * world)
    {
        const int freeResult = FreeVirtualAreaPartitions(world);
        if (freeResult != 0) {
            return freeResult;
        }

        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
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

        return CZClass::TryFreeNode(world);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.initvirtualareapartitions
     * @recoil-artifact defines .text recoil:function:0x4502b0: CZWorld::InitVirtualAreaPartitions.
     *
     *
     * BN source path evidence: GameZRecoil/zClass/cls_world.c.
     * Purpose: initialize virtual area partition edge cells by moving their
     * children into VAP_statics nodes.
     */
    int __fastcall InitVirtualAreaPartitions(CZNodePartial * world)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        if (data->areaGridRows == 0) {
            sprintf(
                g_zError_DebugMsgBuffer,
                g_CZClass_LineErrorVirtualAreaPartitionNullFmt,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c",
                0x245
            );
            zError::EmitDebugBuffer(5);
            return 5;
        }

        CZTypeList::UpdateQueuedTrees();

        for (int col = 0; col < data->areaGridColCount; ++col) {
            zWorldAreaPartial* area = &data->areaGridRows[0][col];
            if (area->childCount > 0) {
                CZNodePartial* statics = CZObject3D::gwObject3DInit();
                CZClass::gwNodeSetName(statics, g_CZClass_VapStaticsNodeName);
                while (area->childCount > 0) {
                    CZNodePartial* child = area->childList[0];
                    CZObject3D::gwObject3DAddChild(statics, child);
                    CZWorld::RemoveChildAtGrid(world, child);
                }
                CZTypeList::UpdateQueuedTrees();
                CZWorld::AddChildAtGrid(world, statics);
            }
        }

        zWorldAreaPartial* lastRow = data->areaGridRows[data->areaGridRowCount - 1];
        for (int lastCol = 0; lastCol < data->areaGridColCount; ++lastCol) {
            zWorldAreaPartial* area = &lastRow[lastCol];
            if (area->childCount > 0) {
                CZNodePartial* statics = CZObject3D::gwObject3DInit();
                CZClass::gwNodeSetName(statics, g_CZClass_VapStaticsNodeName);
                while (area->childCount > 0) {
                    CZNodePartial* child = area->childList[0];
                    CZObject3D::gwObject3DAddChild(statics, child);
                    CZWorld::RemoveChildAtGrid(world, child);
                }
                CZTypeList::UpdateQueuedTrees();
                CZWorld::AddChildAtGrid(world, statics);
            }
        }

        for (int firstEdgeRow = 1; firstEdgeRow < data->areaGridRowCount - 1; ++firstEdgeRow) {
            zWorldAreaPartial* area = &data->areaGridRows[firstEdgeRow][0];
            if (area->childCount > 0) {
                CZNodePartial* statics = CZObject3D::gwObject3DInit();
                CZClass::gwNodeSetName(statics, g_CZClass_VapStaticsNodeName);
                while (area->childCount > 0) {
                    CZNodePartial* child = area->childList[0];
                    CZObject3D::gwObject3DAddChild(statics, child);
                    CZWorld::RemoveChildAtGrid(world, child);
                }
                CZTypeList::UpdateQueuedTrees();
                CZWorld::AddChildAtGrid(world, statics);
            }
        }

        for (int lastEdgeRow = 1; lastEdgeRow < data->areaGridRowCount - 1; ++lastEdgeRow) {
            zWorldAreaPartial* area = &data->areaGridRows[lastEdgeRow][data->areaGridColCount - 1];
            if (area->childCount > 0) {
                CZNodePartial* statics = CZObject3D::gwObject3DInit();
                CZClass::gwNodeSetName(statics, g_CZClass_VapStaticsNodeName);
                while (area->childCount > 0) {
                    CZNodePartial* child = area->childList[0];
                    CZObject3D::gwObject3DAddChild(statics, child);
                    CZWorld::RemoveChildAtGrid(world, child);
                }
                CZTypeList::UpdateQueuedTrees();
                CZWorld::AddChildAtGrid(world, statics);
            }
        }

        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.setvirtualpartition
     * @recoil-artifact defines .text recoil:function:0x450510: CZWorld::SetVirtualPartition.
     * @recoil-match byte
     *
     * BN source path evidence: GameZRecoil/zClass/cls_world.c.
     * Purpose: set the virtual-partition query flag and initialize partitions
     * when enabling the mode.
     */
    SetVirtualPartition(CZNodePartial * world, int enabled)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        data->clampQueriesToBounds = enabled;
        if (enabled != 0) {
            InitVirtualAreaPartitions(world);
        }
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.applypendingfogsettings
     * @recoil-artifact defines .text recoil:function:0x450530: CZWorld::ApplyPendingFogSettings.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: apply staged world fog changes and queued area-bound updates,
     * then clear the pending flags.
     */
    int __fastcall ApplyPendingFogSettings(CZNodePartial * world)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        if (CZTypeList::CountNodes(0x0d) > 1) {
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
                zModelFogSetEnabled(0);
                zModelFogSetLinearModeEnabled(0);
                zModelFogSetDensity(0.0f);
            } else {
                zModelFogSetEnabled(1);
                zModelFogSetEnabled(data->fogState == 1 ? 1 : 0);
                zModelFogSetLinearModeEnabled(data->fogState == 1 ? 1 : 0);
            }
        }

        if ((data->flags & 0x02) != 0) {
            fogChanged = 1;
            zRndr::FogColorSetRgb01Clamped(&data->ambientColor);
            zModelFogSetColorRgb01(&data->ambientColor);
        }

        if ((data->flags & 0x04) != 0) {
            fogChanged = 1;
            zModelFogSetDistanceStart(data->fogDistanceStart);
            zModelFogSetDistanceEnd(data->fogDistanceEnd);
        }

        if ((data->flags & 0x20) != 0) {
            fogChanged = 1;
            zModelFogSetHeightHigh(data->fogHeightHigh);
            zModelFogSetHeightLow(data->fogHeightLow);
        }

        if ((data->flags & 0x08) != 0) {
            fogChanged = 1;
            zModelFogSetDensity(data->fogDensity);
        }

        if (fogChanged != 0) {
            zGame::ReturnOnlyStub();
            zModelFogApplyCurrentColor();
        }

        if (data->pendingAreaUpdateCount > 0) {
            zWorldAreaPartial** pendingAreaUpdates = data->pendingAreaUpdates;
            do {
                zWorldAreaPartial* area = *pendingAreaUpdates;
                RebuildAreaBounds(data, area);
                ++pendingAreaUpdates;
                area->areaFlags &= ~0x01;
                --data->pendingAreaUpdateCount;
            } while (data->pendingAreaUpdateCount > 0);
        }

        data->flags = 0;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.worldtogridcoordsclampedex
     * @recoil-artifact defines .text recoil:function:0x450650: CZWorld::WorldToGridCoordsClampedEx.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: clamp world X/Z coordinates to valid grid coordinates while also returning unclamped grid coordinates
     * and an inside-bounds flag.
     */
    int __fastcall WorldToGridCoordsClampedEx(
        CZNodePartial * world,
        int* outGridCol,
        float worldX,
        float worldZ,
        int* outGridRow,
        int* clampedGridColOut,
        int* clampedGridRowOut,
        int* insideBoundsOut
    )
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);

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

        if (worldZ > data->originZ) {
            clampedZ = data->originZ - 0.1f;
            *insideBoundsOut = 0;
        } else if (worldZ <= data->worldMaxZ) {
            clampedZ = data->worldMaxZ + 0.1f;
            *insideBoundsOut = 0;
        }

        *clampedGridColOut = (int)((clampedX - data->originX) * data->areaInvSizeX);
        *clampedGridRowOut = (int)((clampedZ - data->originZ) * data->areaInvSizeZ);

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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.worldtogridcoordsclamped
     * @recoil-artifact defines .text recoil:function:0x450790: CZWorld::WorldToGridCoordsClamped.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: clamp a world X/Z position to the world's grid extents and return the corresponding grid coordinates.
     */
    int __fastcall
    WorldToGridCoordsClamped(CZNodePartial * world, int* outGridCol, float worldX, float worldZ, int* outGridRow)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);

        float clampedX;
        if (worldX < data->originX + 0.1) {
            clampedX = data->originX + 0.1f;
        } else {
            clampedX = data->worldMaxX - 0.1f;
            if (worldX < clampedX) {
                clampedX = worldX;
            }
        }

        float clampedZ;
        if (worldZ > data->originZ - 0.1f) {
            clampedZ = data->originZ - 0.1f;
        } else {
            clampedZ = data->worldMaxZ + 0.1f;
            if (worldZ > clampedZ) {
                clampedZ = worldZ;
            }
        }

        *outGridCol = (int)((clampedX - data->originX) * data->areaInvSizeX);
        *outGridRow = (int)((clampedZ - data->originZ) * data->areaInvSizeZ);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.worldrecttogridindex
     * @recoil-artifact defines .text recoil:function:0x450840: CZWorld::WorldRectToGridIndex.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: convert a world-space X/Z rectangle to a valid grid cell when
     * it fits inside the partition inclusion tolerances.
     */
    int __fastcall WorldRectToGridIndex(
        CZNodePartial * world,
        int* outGridCol,
        float minX,
        float maxX,
        float minZ,
        float maxZ,
        int* outGridRow
    )
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        *outGridCol = -1;
        *outGridRow = -1;

        if (data->originX - data->partitionInclusionTolX > minX
            || maxX >= data->worldMaxX + data->partitionInclusionTolX
            || maxZ > data->originZ + data->partitionInclusionTolZ
            || minZ <= data->worldMaxZ - data->partitionInclusionTolZ) {
            return 0;
        }

        const float centerX = (minX + maxX) * 0.5f - data->originX;
        const float centerZ = (minZ + maxZ) * 0.5f - data->originZ;
        *outGridCol = (int)(centerX * data->areaInvSizeX);
        *outGridRow = (int)(centerZ * data->areaInvSizeZ);

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

        zWorldAreaPartial* gridCell = &data->areaGridRows[*outGridRow][*outGridCol];
        const float cellMaxX = gridCell->cellMinX + data->areaCellSizeX;
        const float cellMaxZ = gridCell->cellMinZ + data->areaCellSizeZ;

        if (minX < gridCell->cellMinX && gridCell->cellMinX - minX > data->partitionInclusionTolX) {
            *outGridCol = -1;
            *outGridRow = -1;
        } else if (maxX > cellMaxX && maxX - cellMaxX > data->partitionInclusionTolX) {
            *outGridCol = -1;
            *outGridRow = -1;
        } else if (minZ < cellMaxZ && cellMaxZ - minZ > data->partitionInclusionTolZ) {
            *outGridCol = -1;
            *outGridRow = -1;
        } else if (maxZ > gridCell->cellMinZ && maxZ - gridCell->cellMinZ > data->partitionInclusionTolZ) {
            *outGridCol = -1;
            *outGridRow = -1;
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.getareapartitionatgrid
     * @recoil-artifact defines .text recoil:function:0x450a00: CZWorld::GetAreaPartitionAtGrid.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: validate the world node/data pointers and return the area
     * partition at a grid column and row.
     */
    zWorldAreaPartial* __fastcall GetAreaPartitionAtGrid(CZNodePartial * world, int gridCol, int gridRow)
    {
        if (world == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c", 0x6d4, "Null node pointer.");
            return 0;
        }

        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c", 0x6d5, "Null class data pointer");
            return 0;
        }

        return &data->areaGridRows[gridRow][gridCol];
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.ensuregridcelldisplayposition
     * @recoil-artifact defines .text recoil:function:0x450a70: CZWorld::EnsureGridCellDisplayPosition.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: ensure a grid cell is queued for display-position/bounds
     * refresh when it is not already pending.
     */
    int __fastcall EnsureGridCellDisplayPosition(CZNodePartial * world, int gridCol, int gridRow)
    {
        if (world == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c", 0x6f5, "Null node pointer.");
            return 5;
        }

        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        if (data == 0) {
            zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c", 0x6f6, "Null class data pointer");
            return 5;
        }

        zWorldAreaPartial* area = &data->areaGridRows[gridRow][gridCol];
        if ((area->areaFlags & 0x01) == 0) {
            return QueueAreaUpdate(world, data, area);
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.setpendingfogstate
     * @recoil-artifact defines .text recoil:function:0x450ae0: CZWorld::SetPendingFogState.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending fog enable/linear-mode state for the next
     * world fog application pass.
     */
    int __fastcall SetPendingFogState(CZNodePartial * world, int fogState)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        data->fogState = fogState;
        data->flags |= 0x01;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.setpendingfogcolorrgb01
     * @recoil-artifact defines .text recoil:function:0x450af0: CZWorld::SetPendingFogColorRgb01.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending fog RGB color values for the next world fog
     * application pass.
     */
    int __fastcall SetPendingFogColorRgb01(CZNodePartial * world, float red, float green, float blue)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        data->ambientColor.red = red;
        data->ambientColor.green = green;
        data->ambientColor.blue = blue;
        data->flags |= 0x02;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.setpendingfogaltituderange
     * @recoil-artifact defines .text recoil:function:0x450b20: CZWorld::SetPendingFogAltitudeRange.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending vertical fog altitude bounds for the next
     * world fog application pass.
     */
    int __fastcall SetPendingFogAltitudeRange(CZNodePartial * world, float minAlt, float maxAlt)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        data->fogHeightHigh = maxAlt;
        data->fogHeightLow = minAlt;
        data->flags |= 0x20;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.setpendingfogrange
     * @recoil-artifact defines .text recoil:function:0x450b40: CZWorld::SetPendingFogRange.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending near and far fog distance range for the next
     * world fog application pass.
     */
    int __fastcall SetPendingFogRange(CZNodePartial * world, float nearRange, float farRange)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        data->fogDistanceStart = nearRange;
        data->fogDistanceEnd = farRange;
        data->flags |= 0x04;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.setpendingfogdensity
     * @recoil-artifact defines .text recoil:function:0x450b60: CZWorld::SetPendingFogDensity.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: stage the pending fog density for the next world fog
     * application pass.
     */
    int __fastcall SetPendingFogDensity(CZNodePartial * world, float density)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        data->fogDensity = density;
        data->flags |= 0x08;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.getpendingfogdensity
     * @recoil-artifact defines .text recoil:function:0x450b80: CZWorld::GetPendingFogDensity.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged fog density value from the world data.
     */
    int __fastcall GetPendingFogDensity(CZNodePartial * world, float* outDensity)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        *outDensity = data->fogDensity;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.getpendingfogstate
     * @recoil-artifact defines .text recoil:function:0x450b90: CZWorld::GetPendingFogState.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged fog state from the world data.
     */
    int __fastcall GetPendingFogState(CZNodePartial * world, int* outState)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        *outState = data->fogState;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.getpendingfogcolorrgb01
     * @recoil-artifact defines .text recoil:function:0x450ba0: CZWorld::GetPendingFogColorRgb01.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged fog RGB color values from the world data.
     */
    int __fastcall GetPendingFogColorRgb01(CZNodePartial * world, float* outRed, float* outGreen, float* outBlue)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        *outRed = data->ambientColor.red;
        *outGreen = data->ambientColor.green;
        *outBlue = data->ambientColor.blue;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.getpendingfogrange
     * @recoil-artifact defines .text recoil:function:0x450bc0: CZWorld::GetPendingFogRange.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged near and far fog distance range from the
     * world data.
     */
    int __fastcall GetPendingFogRange(CZNodePartial * world, float* outNearRange, float* outFarRange)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        *outNearRange = data->fogDistanceStart;
        *outFarRange = data->fogDistanceEnd;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.getpendingfogaltituderange
     * @recoil-artifact defines .text recoil:function:0x450be0: CZWorld::GetPendingFogAltitudeRange.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: return the staged vertical fog altitude bounds from the world
     * data.
     */
    int __fastcall GetPendingFogAltitudeRange(CZNodePartial * world, float* outMinAlt, float* outMaxAlt)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        *outMaxAlt = data->fogHeightHigh;
        *outMinAlt = data->fogHeightLow;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.gwworldsetorigin
     * @recoil-artifact defines .text recoil:function:0x450c00: CZWorld::gwWorldSetOrigin.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: set the world origin and update the derived maximum X/Z
     * bounds.
     */
    gwWorldSetOrigin(CZNodePartial * world, float originX, float originZ)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        data->originX = originX;
        data->originZ = originZ;
        data->worldMaxX = data->worldSizeX + originX;
        data->worldMaxZ = data->worldSizeZ + originZ;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.gwworldsetsize
     * @recoil-artifact defines .text recoil:function:0x450c30: CZWorld::gwWorldSetSize.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: set the world X/Z size and update the derived maximum bounds.
     */
    gwWorldSetSize(CZNodePartial * world, float sizeX, float sizeZ)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        data->worldSizeX = sizeX;
        data->worldSizeZ = sizeZ;
        data->worldMaxX = data->originX + sizeX;
        data->worldMaxZ = data->originZ + sizeZ;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.gwworldsetvirtualareapartition
     * @recoil-artifact defines .text recoil:function:0x450c60: CZWorld::gwWorldSetVirtualAreaPartition.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: allocate and initialize the virtual area partition grid and
     * its cell metrics from the configured world bounds.
     */
    gwWorldSetVirtualAreaPartition(CZNodePartial * world, float cellSizeX, float cellSizeZ)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
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
        float areaCellRangeSq = cellSizeX * cellSizeX + cellSizeZ * cellSizeZ;
        int areaCellRangeBits = *(int*)(&areaCellRangeSq);
        areaCellRangeBits = (areaCellRangeBits >> 1) + 0x1fc00000;
        data->areaCellRadiusBias = *(float*)(&areaCellRangeBits) * -0.5f;

        int gridColCount = (int)(data->worldSizeX / data->areaCellSizeX);
        data->areaGridColCount = gridColCount;
        if ((float)(gridColCount)*data->areaCellSizeX < data->worldSizeX) {
            ++gridColCount;
            data->areaGridColCount = gridColCount;
        }

        int gridRowCount = (int)(data->worldSizeZ / data->areaCellSizeZ);
        data->areaGridRowCount = gridRowCount;
        if ((float)(gridRowCount)*data->areaCellSizeZ > data->worldSizeZ) {
            ++gridRowCount;
            data->areaGridRowCount = gridRowCount;
        }

        data->areaGridRows = (zWorldAreaPartial**)(calloc(data->areaGridRowCount, sizeof(zWorldAreaPartial*)));
        for (int row = 0; row < data->areaGridRowCount; ++row) {
            data->areaGridRows[row] = (zWorldAreaPartial*)(calloc(data->areaGridColCount, sizeof(zWorldAreaPartial)));
        }

        for (int initRow = 0; initRow < data->areaGridRowCount; ++initRow) {
            const float rowAsFloat = (float)(initRow);
            for (int col = 0; col < data->areaGridColCount; ++col) {
                zWorldAreaPartial* area = &data->areaGridRows[initRow][col];
                area->areaFlags |= 0x100;
                area->cellMinX = (float)(col)*data->areaCellSizeX + data->originX;
                area->cellMinZ = rowAsFloat * data->areaCellSizeZ + data->originZ;
                area->bbox[0] = area->cellMinX;
                area->bbox[3] = area->cellMinX + data->areaCellSizeX;
                area->bbox[5] = area->cellMinZ;
                area->bbox[2] = area->cellMinZ + data->areaCellSizeZ;
                CZBBox::MinMaxToBoundingSphere((const zBBox3f*)(area->bbox), &area->bboxCenter, &area->bboxRadius);
                area->areaIndex = -1;
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.freevirtualareapartitions
     * @recoil-artifact defines .text recoil:function:0x450e40: CZWorld::FreeVirtualAreaPartitions.
     * @recoil-match byte
     *
     * Purpose: release virtual-area child lists and owned grid storage, then
     * clear the installed partition metrics.
     */
    int __fastcall FreeVirtualAreaPartitions(CZNodePartial * world)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        if (data->areaGridRows == 0) {
            return 0;
        }

        {
            int row = 0;
            zWorldAreaPartial** rowCursor = data->areaGridRows;
            if (data->areaGridRowCount > 0) {
                do {
                    zWorldAreaPartial* area = *rowCursor;
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.gwworldsetpartitioninclusiontolerance
     * @recoil-artifact defines .text recoil:function:0x450f00: CZWorld::gwWorldSetPartitionInclusionTolerance.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: set the X/Z tolerances used when testing partition inclusion.
     */
    int __fastcall gwWorldSetPartitionInclusionTolerance(CZNodePartial * world, float toleranceX, float toleranceZ)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        data->partitionInclusionTolX = toleranceX;
        data->partitionInclusionTolZ = toleranceZ;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.gwworldsetmaxdecfeatures
     * @recoil-artifact defines .text recoil:function:0x450f20: CZWorld::gwWorldSetMaxDecFeatures.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: clamp and store the maximum DEC feature count for world
     * partitions.
     */
    gwWorldSetMaxDecFeatures(CZNodePartial * world, int maxFeatures)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        if (maxFeatures > 255) {
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c",
                0xc01,
                g_CZClass_PartitionMaxDecFeatureCountOverflowFmt,
                maxFeatures
            );
            maxFeatures = 255;
        }

        data->partitionMaxDecFeatureCount = (unsigned char)(maxFeatures);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.addchildtogridcell
     * @recoil-artifact defines .text recoil:function:0x450f60: CZWorld::AddChildToGridCell.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: add a child to either the overflow world list or a grid area
     * list while maintaining the child's listA parent ownership.
     */
    int __fastcall AddChildToGridCell(CZNodePartial * world, CZNodePartial * child, int gridCol, int gridRow)
    {
        int result;
        CZWorldDataPartial* data;

        data = (CZWorldDataPartial*)(world->classData);
        result = 0;

        if (gridCol >= 0 && gridRow >= 0) {
            if (data->areaGridRows[gridRow][gridCol].childCount >= 0x7fff) {
                gridRow = -1;
                gridCol = -1;
            }
        }

        if (gridCol < 0 || gridRow < 0) {
            int listCount = world->listCountB + 1;
            int listBytes = listCount * sizeof(CZNodePartial*);
            world->listB = (CZNodePartial**)(realloc(world->listB, listBytes));
            world->listB[listCount - 1] = child;
            ++world->listCountB;
            child->gridCol = -1;
            child->gridRow = -1;
            int parentCount = child->listCountA + 1;
            int parentBytes = parentCount * sizeof(CZNodePartial*);
            child->listA = (CZNodePartial**)(realloc(child->listA, parentBytes));
            child->listA[parentCount - 1] = world;
            ++child->listCountA;
            if (child->listCountA > 1) {
                CZClass::SetSingleParentFlagRecursive(child, 0);
            }
        } else {
            zWorldAreaPartial* area = &data->areaGridRows[gridRow][gridCol];
            int areaCount = (int)(area->childCount) + 1;
            int areaBytes = areaCount * sizeof(CZNodePartial*);
            area->childList = (CZNodePartial**)(realloc(area->childList, areaBytes));
            area->childList[areaCount - 1] = child;
            ++area->childCount;

            child->gridCol = gridCol;
            child->gridRow = gridRow;
            int parentCount = child->listCountA + 1;
            int parentBytes = parentCount * sizeof(CZNodePartial*);
            child->listA = (CZNodePartial**)(realloc(child->listA, parentBytes));
            child->listA[parentCount - 1] = world;
            ++child->listCountA;
            if (child->listCountA > 1) {
                CZClass::SetSingleParentFlagRecursive(child, 0);
            }

            if ((area->areaFlags & 0x01) == 0) {
                result = QueueAreaUpdate(world, data, area);
            } else {
                result = 0;
            }
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.addchildatgrid
     * @recoil-artifact defines .text recoil:function:0x4510e0: CZWorld::AddChildAtGrid.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: derive the child's world grid cell from bounds or world extent
     * and route insertion into the world child-link storage.
     */
    int __fastcall AddChildAtGrid(CZNodePartial * world, CZNodePartial * child)
    {
        int gridCol = -1;
        int gridRow = -1;

        if ((child->flags & 0x80) == 0) {
            float minX = 0.0f;
            float maxX = 0.0f;
            float minZ = 0.0f;
            float maxZ = 0.0f;

            if ((child->flags & 0x100) != 0) {
                zBBoxCorners corners = { 0 };
                CZClass::gwNodeGetWorldBBoxCorners(child, &corners);
                minX = corners.corners[0].x;
                maxX = corners.corners[0].x;
                minZ = corners.corners[0].z;
                maxZ = corners.corners[0].z;

                for (int i = 1; i < 8; ++i) {
                    const float x = corners.corners[i].x;
                    const float z = corners.corners[i].z;
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
                CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
                minX = data->originX;
                minZ = data->originZ;
                maxX = data->originX + data->worldSizeX;
                maxZ = data->originZ + data->worldSizeZ;
            }

            WorldRectToGridIndex(world, &gridCol, minX, maxX, minZ, maxZ, &gridRow);
        }

        return AddChildToGridCell(world, child, gridCol, gridRow);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.removechildatgrid
     * @recoil-artifact defines .text recoil:function:0x451240: CZWorld::RemoveChildAtGrid.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: remove a child from the world overflow list or its grid area
     * list while clearing the child's parent/grid ownership state.
     */
    int __fastcall RemoveChildAtGrid(CZNodePartial * world, CZNodePartial * child)
    {
        const int gridCol = child->gridCol;
        const int gridRow = child->gridRow;
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);

        if (gridCol == -1 && gridRow == -1) {
            return CZClass::RemoveChildGeneric(world, child);
        }

        zWorldAreaPartial* area = &data->areaGridRows[gridRow][gridCol];
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
                "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c",
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
            return QueueAreaUpdate(world, data, area);
        }

        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.addlight
     * @recoil-artifact defines .text recoil:function:0x451360: CZWorld::AddLight.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: append a light and its data to the world lists and attach the
     * world to the light's world list.
     */
    AddLight(CZNodePartial * world, CZNodePartial * light)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        CZLightDataPartial* lightData = (CZLightDataPartial*)(light->classData);

        const int lightListBytes = (data->lightCount + 1) * sizeof(CZNodePartial*);
        data->lightNodes = (CZNodePartial**)(realloc(data->lightNodes, lightListBytes));
        data->lightNodes[data->lightCount] = light;

        data->lightDataList = (CZLightDataPartial**)(realloc(data->lightDataList, lightListBytes));
        data->lightDataList[data->lightCount] = lightData;
        ++data->lightCount;

        lightData->attachedWorlds = (CZNodePartial**)(realloc(
            lightData->attachedWorlds,
            (lightData->attachedWorldCount + 1) * sizeof(CZNodePartial*)
        ));
        lightData->attachedWorlds[lightData->attachedWorldCount] = world;
        ++lightData->attachedWorldCount;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.removelight
     * @recoil-artifact defines .text recoil:function:0x451410: CZWorld::RemoveLight.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: remove a light from the world lists and remove the world from
     * the light's attached-world list.
     */
    RemoveLight(CZNodePartial * world, CZNodePartial * light)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);

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
                g_CZClass_LineErrorDeleteLightNotFoundInWorldListFmt,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c",
                0x108d,
                (unsigned int)((unsigned int)(world)),
                (unsigned int)((unsigned int)(light))
            );
            zError::EmitDebugBuffer(5);
            return 5;
        }

        CZLightDataPartial* lightData = data->lightDataList[lightIndex];
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
                g_CZClass_LineErrorDeleteLightWorldNotFoundFmt,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c",
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.initlightpointinpolygonxz
     * @recoil-artifact defines .text recoil:function:0x451540: CZWorld::InitLightPointInPolygonXZ.
     * @recoil-match byte
     *
     * Purpose: initialize model lighting from the world light nodes, data and count.
     */
    int __fastcall InitLightPointInPolygonXZ(CZNodePartial * world)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        zModelLightPointInPolygonInitXZ(data->lightNodes, data->lightDataList, data->lightCount);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.updatealllights
     * @recoil-artifact defines .text recoil:function:0x451560: CZWorld::UpdateAllLights.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: iterate the world light nodes and call
     * CZLight::gwLightUpdate for each.
     */
    int __fastcall UpdateAllLights(CZNodePartial * world)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);

        for (int i = 0; i < data->lightCount; ++i) {
            CZLight::gwLightUpdate(data->lightNodes[i]);
        }

        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.addsound
     * @recoil-artifact defines .text recoil:function:0x451590: CZWorld::AddSound.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: append a sound and its data to the world lists and attach the
     * world to the sound's world list.
     */
    AddSound(CZNodePartial * world, CZNodePartial * sound)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);
        CZSoundDataPartial* soundData = (CZSoundDataPartial*)(sound->classData);

        const int soundListBytes = (data->soundCount + 1) * sizeof(CZNodePartial*);
        data->soundNodes = (CZNodePartial**)(realloc(data->soundNodes, soundListBytes));
        data->soundNodes[data->soundCount] = sound;

        data->soundDataList = (CZSoundDataPartial**)(realloc(data->soundDataList, soundListBytes));
        data->soundDataList[data->soundCount] = soundData;
        ++data->soundCount;

        soundData->attachedWorlds = (CZNodePartial**)(realloc(
            soundData->attachedWorlds,
            (soundData->attachedWorldCount + 1) * sizeof(CZNodePartial*)
        ));
        soundData->attachedWorlds[soundData->attachedWorldCount] = world;
        ++soundData->attachedWorldCount;
        return 0;
    }

    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.removesound
     * @recoil-artifact defines .text recoil:function:0x451640: CZWorld::RemoveSound.
     *
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: remove a sound from the world lists and remove the world from
     * the sound's attached-world list.
     */
    RemoveSound(CZNodePartial * world, CZNodePartial * sound)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);

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
                g_CZClass_LineErrorDeleteSoundNotFoundInWorldListFmt,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c",
                0x11cc,
                (unsigned int)((unsigned int)(world)),
                (unsigned int)((unsigned int)(sound))
            );
            zError::EmitDebugBuffer(5);
            return 5;
        }

        CZSoundDataPartial* soundData = data->soundDataList[soundIndex];
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
                g_CZClass_LineErrorDeleteSoundWorldNotFoundFmt,
                "D:\\Proj\\GameZRecoil\\zClass\\cls_world.c",
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
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.updateallsounds
     * @recoil-artifact defines .text recoil:function:0x451770: CZWorld::UpdateAllSounds.
     * @recoil-match byte
     *
     * BN source path evidence: D:\Proj\GameZRecoil\zClass\cls_world.c.
     * Purpose: iterate the world sound nodes and call
     * CZSound::UpdatePlayback for each.
     */
    int __fastcall UpdateAllSounds(CZNodePartial * world)
    {
        CZWorldDataPartial* data = (CZWorldDataPartial*)(world->classData);

        for (int i = 0; i < data->soundCount; ++i) {
            CZSound::UpdatePlayback(data->soundNodes[i]);
        }

        return 0;
    }
    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.writesettingssection
     * @recoil-artifact defines .text recoil:function:0x4517a0: CZWorld::WriteSettingsSection.
     *
     *
     * BN source path evidence: GameZRecoil/zClass/cls_world.c.
     * Purpose: write each world node's pending fog settings as a ZBD settings
     * section blob.
     */
    WriteSettingsSection(zZbdSectionCallbackCtx * callbackCtx, void* userData)
    {
        (void)userData;

        int result = 1;
        CZTypeListLink* link = *g_CZTypeList_HeadSlotPtrs[13];
        while (link != 0 && result != 0) {
            CZNodePartial* world = link->node;
            CZWorldSettingsSectionRecord settings;
            GetPendingFogDensity(world, &settings.fogDensity);
            GetPendingFogState(world, &settings.fogState);
            GetPendingFogColorRgb01(
                world,
                &settings.fogColorRgb01.red,
                &settings.fogColorRgb01.green,
                &settings.fogColorRgb01.blue
            );
            GetPendingFogRange(world, &settings.fogRangeNear, &settings.fogRangeFar);
            GetPendingFogAltitudeRange(world, &settings.fogAltitudeLow, &settings.fogAltitudeHigh);
            GetPendingFogDensity(world, &settings.fogDensity);
            result = zUtil_ZAR::WriteSectionBlob(callbackCtx, world->name, &settings, sizeof(settings));
            link = link->next;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil.zclass.cls-world.readsettingssection
     * @recoil-artifact defines .text recoil:function:0x451840: CZWorld::ReadSettingsSection.
     * @recoil-match byte
     *
     * BN source path evidence: GameZRecoil/zClass/cls_world.c.
     * Purpose: apply a ZBD settings section record to the named world node's
     * pending fog settings.
     */
    void __fastcall ReadSettingsSection(
        zZbdSectionCallbackCtx * callbackCtx,
        const char* worldName,
        CZWorldSettingsSectionRecord* settings,
        unsigned int size,
        void* userData
    )
    {
        (void)callbackCtx;
        (void)size;
        (void)userData;

        CZNodePartial* world = CZClass::FindByTypeAndName(13, worldName);
        if (world == 0) {
            return;
        }

        SetPendingFogDensity(world, settings->fogDensity);
        SetPendingFogState(world, settings->fogState);
        SetPendingFogColorRgb01(
            world,
            settings->fogColorRgb01.red,
            settings->fogColorRgb01.green,
            settings->fogColorRgb01.blue
        );
        SetPendingFogRange(world, settings->fogRangeNear, settings->fogRangeFar);
        SetPendingFogAltitudeRange(world, settings->fogAltitudeLow, settings->fogAltitudeHigh);
        SetPendingFogDensity(world, settings->fogDensity);
    }
}
