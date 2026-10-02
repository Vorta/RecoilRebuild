// zUI compilation unit for the zTimedTask list, HudLineClip and the zMath
// Z-range clip helpers, inferred from the retail object boundary that starts
// at 0x4bd470 (subsystem grouping; Inferred): its pooled .rdata double 0.0 at
// 0x4d3d70 is separate from zui.cpp's own copy at 0x4d3c68, and its .data clip
// bounds and .bss timed-task list form one contiguous run. Original filename
// unresolved; zui_task.cpp is a provisional name (2026-10-02).

#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_game_setup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"

#include "Battlesport/turret.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zUtil/zbd.h"

#include <cctype>
#include <cstdarg>
#include <math.h>
#include <new>
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <vector>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-ztimedtask-removefromactivelist
 * @recoil-artifact defines .text recoil:function:0x4bd470: zTimedTask::RemoveFromActiveList.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for zTimedTask::RemoveFromActiveList.
 */
void zTimedTask::RemoveFromActiveList()
{
    zTimedTask* node = g_zTimedTask_ActiveHead;
    zTimedTask* previous = 0;
    while (node != 0) {
        if (this == node) {
            if (previous == 0) {
                g_zTimedTask_ActiveHead = g_zTimedTask_ActiveHead->next;
                --g_zTimedTask_ActiveCount;
                return;
            }

            if (node == g_zTimedTask_ActiveTail) {
                g_zTimedTask_ActiveTail = previous;
            }

            previous->next = node->next;
            --g_zTimedTask_ActiveCount;
            return;
        }
        previous = node;
        node = node->next;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-ztimedtask-runimmediateaction
 * @recoil-artifact defines .text recoil:function:0x4bd4d0: zTimedTask::RunImmediateAction.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for zTimedTask::RunImmediateAction.
 */
void zTimedTask::RunImmediateAction()
{
    switch (kind) {
    case 4: {
        const char* text = (const char*)(&actionArg2) + 2;
        if (*text != '\0') {
            zImage_Font::BlitStringToActiveTarget(text, (short)(actionArg0), (short)(actionArg1), (short)(actionArg2));
        }
        break;
    }

    case 5: {
        const char* text = (const char*)(actionArg3);
        if (text != 0 && *text != '\0') {
            zImage_Font::BlitStringToActiveTarget(text, (short)(actionArg0), (short)(actionArg1), (short)(actionArg2));
        }
        break;
    }

    case 1:
        if (actionArg2 != 0) {
            zVid_Image::BlitToActiveTarget(
                (zVidImagePartial*)(actionArg2),
                actionArg0,
                actionArg1,
                (unsigned short)(actionArg3),
                (zVidRect32*)(actionArg4)
            );
        }
        break;

    case 3:
        zRndrRasterizePoly((zVec3*)(&actionArg0), rasterVertexCount, rasterDrawParam);
        break;

    case 2:
        zRndrDrawImmediateLine(actionArg0, actionArg1, actionArg2, actionArg3, actionArg4);
        break;

    case 7: {
        zVec3 point0;
        zVec3 point1;
        int point0Clipped;
        int point1Clipped;
        point0.x = (float)(actionArg0);
        point0.y = (float)(actionArg1);
        point1.x = (float)(actionArg2);
        point1.y = (float)(actionArg3);

        if (HudLineClip::ClipSegmentToCurrentBounds(&point0, &point1, &point0Clipped, &point1Clipped) != 0) {
            zRndrDrawImmediateLine((int)(point0.x), (int)(point0.y), (int)(point1.x), (int)(point1.y), actionArg4);
        }
        break;
    }

    case 6:
        zRndrSpanOcclusionTestSample(actionArg0, actionArg1, actionArg2);
        break;

    case 8:
        zRndrDrawClippedImmediateLineStrip(
            (const zRndr_LinePoint2I*)(&actionArg0),
            alphaPointCount - 1,
            (void*)(alpha255),
            alphaVariantIndex
        );
        break;

    default:
        break;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-ztimedtask-tickactivelist
 * @recoil-artifact defines .text recoil:function:0x4bd660: zTimedTask::TickActiveList.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for zTimedTask::TickActiveList.
 */
void __cdecl zTimedTask::TickActiveList()
{
    zTimedTask* task = g_zTimedTask_ActiveHead;
    while (task != 0) {
        if ((task->flags & 0x02) == 0) {
            task->RunImmediateAction();
        } else if ((task->flags & 0x04) != 0) {
            task->RunImmediateAction();
            task->flags &= ~0x04;
        } else if ((task->flags & 0x08) != 0) {
            task->RunImmediateAction();
            task->flags &= ~0x08;
        }

        if ((task->flags & 0x01) != 0) {
            task->remainingSeconds -= g_FrameDeltaTimeSec;
            if (task->remainingSeconds <= 0.0) {
                task->kind = 9;
                task->RemoveFromActiveList();
            }
        }

        task = task->next;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-setcurrentboundsfromrecti
 * @recoil-artifact defines .text recoil:function:0x4bd6f0: HudLineClip::SetCurrentBoundsFromRectI
 * @recoil-match byte
 *
 * Purpose: Copy integer rectangle edges into the current float clip bounds.
 */
void __fastcall HudLineClip::SetCurrentBoundsFromRectI(const HudRectI* rect)
{
    g_HudLineClip_CurrentLeft = (float)(rect->left);
    g_HudLineClip_CurrentTop = (float)(rect->top);
    g_HudLineClip_CurrentRight = (float)(rect->right);
    g_HudLineClip_CurrentBottom = (float)(rect->bottom);
}

namespace zMath {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-cliplinesegmenttozrange
 * @recoil-artifact defines .text recoil:function:0x4bd720: zMath::ClipLineSegmentToZRange
 * @recoil-match byte
 *
 * Purpose: clips a mutable segment against the current zMath lower and upper
 * Z clipping planes, rejecting segments fully outside the range.
 * Data: reads g_zMath_ClipZLowerBound at 0x4e4880 and
 * g_zMath_ClipZUpperBound at 0x4e4890.
 */
int __fastcall ClipLineSegmentToZRange(zVec3* pointA, zVec3* pointB)
{
    if (pointA->z > g_zMath_ClipZUpperBound && pointB->z > g_zMath_ClipZUpperBound) {
        return 0;
    }

    if (pointA->z < g_zMath_ClipZLowerBound && pointB->z < g_zMath_ClipZLowerBound) {
        return 0;
    }

    if (pointA->z < g_zMath_ClipZLowerBound) {
        ClipLineSegmentPointToZ(pointA, pointB, g_zMath_ClipZLowerBound);
    }
    if (pointB->z < g_zMath_ClipZLowerBound) {
        ClipLineSegmentPointToZ(pointB, pointA, g_zMath_ClipZLowerBound);
    }

    if (pointB->z > g_zMath_ClipZUpperBound) {
        ClipLineSegmentPointToZ(pointB, pointA, g_zMath_ClipZUpperBound);
    }
    if (pointA->z > g_zMath_ClipZUpperBound) {
        ClipLineSegmentPointToZ(pointA, pointB, g_zMath_ClipZUpperBound);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-cliplinesegmentpointtoz
 * @recoil-artifact defines .text recoil:function:0x4bd800: zMath::ClipLineSegmentPointToZ
 * @recoil-match byte
 *
 * Purpose: moves one segment endpoint onto the caller-supplied Z clip plane by
 * interpolating toward the other endpoint.
 * Data: writes only the caller-supplied endpoint and reads no authored globals.
 */
void __fastcall ClipLineSegmentPointToZ(zVec3* pointToClip, const zVec3* otherPoint, float clipZ)
{
    const float t = (clipZ - pointToClip->z) / (otherPoint->z - pointToClip->z);

    pointToClip->x = (otherPoint->x - pointToClip->x) * t + pointToClip->x;
    pointToClip->y = (otherPoint->y - pointToClip->y) * t + pointToClip->y;
    pointToClip->z = clipZ;
}

} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipsegmenttocurrentbounds
 * @recoil-artifact defines .text recoil:function:0x4bd840: HudLineClip::ClipSegmentToCurrentBounds
 * @recoil-match byte
 *
 * Purpose: Clip a segment against the current X bounds, then the current Y bounds.
 */
int __fastcall HudLineClip::ClipSegmentToCurrentBounds(
    zVec3* point0,
    zVec3* point1,
    int* point0Clipped,
    int* point1Clipped
)
{
    const int result = ClipSegmentToCurrentXBounds(point0, point1, point0Clipped, point1Clipped);
    if (result == 0) {
        return 0;
    }

    return ClipSegmentToCurrentYBounds(point0, point1, point0Clipped, point1Clipped);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipsegmenttocurrentxbounds
 * @recoil-artifact defines .text recoil:function:0x4bd880: HudLineClip::ClipSegmentToCurrentXBounds
 * @recoil-match byte
 *
 * Purpose: Reject or clamp a segment against the current left and right bounds.
 */
int __fastcall HudLineClip::ClipSegmentToCurrentXBounds(
    zVec3* point0,
    zVec3* point1,
    int* point0Clipped,
    int* point1Clipped
)
{
    if (point0->x > g_HudLineClip_CurrentRight && point1->x > g_HudLineClip_CurrentRight) {
        *point0Clipped = 1;
        *point1Clipped = 1;
        return 0;
    }
    if (point0->x < g_HudLineClip_CurrentLeft && point1->x < g_HudLineClip_CurrentLeft) {
        *point0Clipped = 1;
        *point1Clipped = 1;
        return 0;
    }

    if (point0->x < g_HudLineClip_CurrentLeft) {
        ClipEndpointToX(point0, point1, g_HudLineClip_CurrentLeft);
        *point0Clipped = 1;
    } else if (point0->x > g_HudLineClip_CurrentRight) {
        ClipEndpointToX(point0, point1, g_HudLineClip_CurrentRight);
        *point0Clipped = 1;
    }

    if (point1->x < g_HudLineClip_CurrentLeft) {
        ClipEndpointToX(point1, point0, g_HudLineClip_CurrentLeft);
        *point1Clipped = 1;
    } else if (point1->x > g_HudLineClip_CurrentRight) {
        ClipEndpointToX(point1, point0, g_HudLineClip_CurrentRight);
        *point1Clipped = 1;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipendpointtox
 * @recoil-artifact defines .text recoil:function:0x4bd9c0: HudLineClip::ClipEndpointToX
 * @recoil-match byte
 *
 * Purpose: Move one segment endpoint to an X clipping plane and interpolate Y.
 */
void __fastcall HudLineClip::ClipEndpointToX(zVec3* endpoint, const zVec3* otherEndpoint, float clipX)
{
    const float clippedY
        = (otherEndpoint->y - endpoint->y) * (clipX - endpoint->x) / (otherEndpoint->x - endpoint->x) + endpoint->y;
    endpoint->x = clipX;
    endpoint->y = clippedY;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipsegmenttocurrentybounds
 * @recoil-artifact defines .text recoil:function:0x4bd9f0: HudLineClip::ClipSegmentToCurrentYBounds
 * @recoil-match byte
 *
 * Purpose: Reject or clamp a segment against the current top and bottom bounds.
 */
int __fastcall HudLineClip::ClipSegmentToCurrentYBounds(
    zVec3* point0,
    zVec3* point1,
    int* point0Clipped,
    int* point1Clipped
)
{
    if (point0->y > g_HudLineClip_CurrentBottom && point1->y > g_HudLineClip_CurrentBottom) {
        *point0Clipped = 1;
        *point1Clipped = 1;
        return 0;
    }
    if (point0->y < g_HudLineClip_CurrentTop && point1->y < g_HudLineClip_CurrentTop) {
        *point0Clipped = 1;
        *point1Clipped = 1;
        return 0;
    }

    if (point0->y < g_HudLineClip_CurrentTop) {
        ClipEndpointToY(point0, point1, g_HudLineClip_CurrentTop);
        *point0Clipped = 1;
    } else if (point0->y > g_HudLineClip_CurrentBottom) {
        ClipEndpointToY(point0, point1, g_HudLineClip_CurrentBottom);
        *point0Clipped = 1;
    }

    if (point1->y < g_HudLineClip_CurrentTop) {
        ClipEndpointToY(point1, point0, g_HudLineClip_CurrentTop);
        *point1Clipped = 1;
    } else if (point1->y > g_HudLineClip_CurrentBottom) {
        ClipEndpointToY(point1, point0, g_HudLineClip_CurrentBottom);
        *point1Clipped = 1;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudlineclip-clipendpointtoy
 * @recoil-artifact defines .text recoil:function:0x4bdb30: HudLineClip::ClipEndpointToY
 * @recoil-match byte
 *
 * Purpose: Move one segment endpoint to a Y clipping plane and interpolate X.
 */
void __fastcall HudLineClip::ClipEndpointToY(zVec3* endpoint, const zVec3* otherEndpoint, float clipY)
{
    const float clippedX
        = (otherEndpoint->x - endpoint->x) * (clipY - endpoint->y) / (otherEndpoint->y - endpoint->y) + endpoint->x;
    endpoint->y = clipY;
    endpoint->x = clippedX;
}
