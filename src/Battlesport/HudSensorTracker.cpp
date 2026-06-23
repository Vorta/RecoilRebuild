#include "Battlesport/HudSensorTracker.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/hud.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zInterp/zInterp.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zTurret/zTurret.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/zWeapon/zWeapon.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" int g_HudSensorTracker_ObjectiveCommandLocked = 0;
extern "C" int g_Hud_MapOverlayRefCount = 0;
extern "C" int g_RecoilApp_QuitAfterCredits = 0;
extern "C" char g_HudSensor_MissionSoundSetName[0x20] = {0};
extern "C" zVec3 g_HudSensor_ProjectScratch[0x400] = {0};
extern "C" zVec3 g_HudSensor_ClipSegmentStart = {0};
extern "C" zVec3 g_HudSensor_ClipSegmentEnd = {0};
extern "C" float g_HudLineClip_CurrentLeft = 0.0f;
extern "C" float g_HudLineClip_CurrentTop = 0.0f;
extern "C" float g_HudLineClip_CurrentRight = 0.0f;
extern "C" float g_HudLineClip_CurrentBottom = 0.0f;

namespace {
struct HudSensorTrackerMissionData {
    int missionId;
    int missionFlags;
    int currentObjectiveIndex;
    int firstIncompleteObjectiveIndex;
    int completedObjectiveCount;
    int objectiveFlowState;
    int objectiveFlowDeadlineSecRaw;
    int missionStat0;
    int missionStat1;
    int missionStat2;
    int missionStat3;
    int weaponsFoundMask;
    int objectiveCompletedFlags[10];
    int difficultyMode;
};

RECOIL_STATIC_ASSERT(sizeof(HudSensorTrackerMissionData) == 0x5c);

const unsigned char g_HudSensorTracker_ObjectiveMarkerColorBlueRgb24[4] = {0x00, 0x00, 0xff, 0x00};

const unsigned char g_HudSensorTracker_ObjectiveBlinkColorRedRgb24[4] = {0xff, 0x00, 0x00, 0x00};

template <typename T>
// Source-faithful helper recovered from address-backed callers in this source file.
zZbdSectionCallback ZbdCallbackPtr(
    T callback
) {
    RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T callback;
        zZbdSectionCallback raw;
    } value = {0};
    value.callback = callback;
    return value.raw;
}

// Source-faithful helper recovered from address-backed callers in this source file.
int FloatToRawSeconds(
    float value
) {
    int rawValue;
    memcpy(
        &rawValue,
        &value,
        sizeof(rawValue)
    );
    return rawValue;
}

// Source-faithful helper recovered from address-backed callers in this source file.
float RawSecondsToFloat(
    int rawValue
) {
    float value;
    memcpy(
        &value,
        &rawValue,
        sizeof(value)
    );
    return value;
}

// Source-faithful helper recovered from address-backed callers in this source file.
float ApproxSqrtScaleFromBits(
    float value
) {
    int bits;
    memcpy(
        &bits,
        &value,
        sizeof(bits)
    );
    bits = (bits >> 1) + 0x1fc00000;

    float approxValue;
    memcpy(
        &approxValue,
        &bits,
        sizeof(approxValue)
    );
    return approxValue;
}

// Source-faithful helper recovered from address-backed callers in this source file.
bool IsPointStrictlyInsideRect(
    const HudUiRect &rect,
    const zVec3 &point
) {
    return (float)(rect.left) < point.x && (float)(rect.right) > point.x &&
           (float)(rect.top) < point.y && (float)(rect.bottom) > point.y;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void AppendPickupFeature(
    char *featureText,
    const char *feature
) {
    strcat(
        featureText,
        feature
    );
}

// Source-faithful helper recovered from address-backed callers in this source file.
zClass_NodePartial *ResolveObjectiveNodePath(
    zReader::Node *pathNode,
    int objectiveIndex,
    const char *missingFormat,
    int sourceLine
) {
    zReader::Node *const pathFields = pathNode->value.nodes;
    zClass_NodePartial *resolvedNode = zClass::FindByTypeAndName(
        6,
        pathFields[1].value.str
    );
    if (resolvedNode == 0) {
        zError::ReportOld(
            0x400,
            "D:\\Proj\\Battlesport\\mission.cpp",
            sourceLine,
            missingFormat,
            objectiveIndex,
            pathFields[1].value.str
        );
        return 0;
    }

    const int pathCount = pathFields[0].value.i32;
    for (int i = 2; i < pathCount; ++i) {
        resolvedNode = zClass_Class::FindNodeRecursiveByName(
            resolvedNode,
            pathFields[i].value.str
        );
    }

    return resolvedNode;
}
} // namespace

/**
 * Reimplements 0x416390: HudGeom2D::ClassifyPointAgainstSegment
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Classify a point against a 2D segment using the segment cross product and extents.
 */
int __fastcall HudGeom2D::ClassifyPointAgainstSegment(
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd,
    const zVec3 *point
) {
    const float dx = segmentEnd->x - segmentStart->x;
    const float dy = segmentEnd->y - segmentStart->y;
    const float px = point->x - segmentStart->x;
    const float py = point->y - segmentStart->y;
    const float cross = dx * py - dy * px;

    if (cross > 0.0f) {
        return 1;
    }
    if (cross < 0.0f) {
        return -1;
    }
    if (px * dx < 0.0f) {
        return -1;
    }
    if (py * dy < 0.0f) {
        return -1;
    }
    if (px * px + py * py > dx * dx + dy * dy) {
        return 1;
    }
    return 0;
}

/**
 * Reimplements 0x4bd9c0: HudLineClip::ClipEndpointToX
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Move one segment endpoint to an X clipping plane and interpolate Y.
 */
void __fastcall HudLineClip::ClipEndpointToX(
    zVec3 *endpoint,
    const zVec3 *otherEndpoint,
    float clipX
) {
    endpoint->y += (otherEndpoint->y - endpoint->y) *
                   ((clipX - endpoint->x) / (otherEndpoint->x - endpoint->x));
    endpoint->x = clipX;
}

/**
 * Reimplements 0x4bdb30: HudLineClip::ClipEndpointToY
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Move one segment endpoint to a Y clipping plane and interpolate X.
 */
void __fastcall HudLineClip::ClipEndpointToY(
    zVec3 *endpoint,
    const zVec3 *otherEndpoint,
    float clipY
) {
    endpoint->x += (otherEndpoint->x - endpoint->x) *
                   ((clipY - endpoint->y) / (otherEndpoint->y - endpoint->y));
    endpoint->y = clipY;
}

/**
 * Reimplements 0x4bd6f0: HudLineClip::SetCurrentBoundsFromRectI
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Copy integer rectangle edges into the current float clip bounds.
 */
void __fastcall HudLineClip::SetCurrentBoundsFromRectI(
    const HudRectI *rect
) {
    g_HudLineClip_CurrentLeft = (float)(rect->left);
    g_HudLineClip_CurrentTop = (float)(rect->top);
    g_HudLineClip_CurrentRight = (float)(rect->right);
    g_HudLineClip_CurrentBottom = (float)(rect->bottom);
}

/**
 * Reimplements 0x4bd880: HudLineClip::ClipSegmentToCurrentXBounds
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Reject or clamp a segment against the current left and right bounds.
 */
int __fastcall HudLineClip::ClipSegmentToCurrentXBounds(
    zVec3 *point0,
    zVec3 *point1,
    int *point0Clipped,
    int *point1Clipped
) {
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
        ClipEndpointToX(
            point0,
            point1,
            g_HudLineClip_CurrentLeft
        );
        *point0Clipped = 1;
    } else if (point0->x > g_HudLineClip_CurrentRight) {
        ClipEndpointToX(
            point0,
            point1,
            g_HudLineClip_CurrentRight
        );
        *point0Clipped = 1;
    }

    if (point1->x < g_HudLineClip_CurrentLeft) {
        ClipEndpointToX(
            point1,
            point0,
            g_HudLineClip_CurrentLeft
        );
        *point1Clipped = 1;
    } else if (point1->x > g_HudLineClip_CurrentRight) {
        ClipEndpointToX(
            point1,
            point0,
            g_HudLineClip_CurrentRight
        );
        *point1Clipped = 1;
    }

    return 1;
}

/**
 * Reimplements 0x4bd9f0: HudLineClip::ClipSegmentToCurrentYBounds
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Reject or clamp a segment against the current top and bottom bounds.
 */
int __fastcall HudLineClip::ClipSegmentToCurrentYBounds(
    zVec3 *point0,
    zVec3 *point1,
    int *point0Clipped,
    int *point1Clipped
) {
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
        ClipEndpointToY(
            point0,
            point1,
            g_HudLineClip_CurrentTop
        );
        *point0Clipped = 1;
    } else if (point0->y > g_HudLineClip_CurrentBottom) {
        ClipEndpointToY(
            point0,
            point1,
            g_HudLineClip_CurrentBottom
        );
        *point0Clipped = 1;
    }

    if (point1->y < g_HudLineClip_CurrentTop) {
        ClipEndpointToY(
            point1,
            point0,
            g_HudLineClip_CurrentTop
        );
        *point1Clipped = 1;
    } else if (point1->y > g_HudLineClip_CurrentBottom) {
        ClipEndpointToY(
            point1,
            point0,
            g_HudLineClip_CurrentBottom
        );
        *point1Clipped = 1;
    }

    return 1;
}

/**
 * Reimplements 0x4bd840: HudLineClip::ClipSegmentToCurrentBounds
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Clip a segment against the current X bounds, then the current Y bounds.
 */
int __fastcall HudLineClip::ClipSegmentToCurrentBounds(
    zVec3 *point0,
    zVec3 *point1,
    int *point0Clipped,
    int *point1Clipped
) {
    const int result = ClipSegmentToCurrentXBounds(
        point0,
        point1,
        point0Clipped,
        point1Clipped
    );
    if (result == 0) {
        return 0;
    }

    return ClipSegmentToCurrentYBounds(
        point0,
        point1,
        point0Clipped,
        point1Clipped
    );
}

/**
 * Reimplements 0x416240: HudRectI::CalcOutcode
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Build the rectangle outside-code bits for a point.
 */
int HudRectI::CalcOutcode(
    const zVec3 *point
) {
    int outcode = 0;
    if (point->x < (float)(left)) {
        outcode = 1;
    } else if (point->x > (float)(right)) {
        outcode = 2;
    }

    if (point->y < (float)(top)) {
        outcode |= 8;
    } else if (point->x > (float)(bottom)) {
        outcode |= 4;
    }

    return outcode;
}

/**
 * Reimplements 0x416290: HudRectI::IsCornerOutcode
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Identify outside-code combinations that lie beyond a rectangle corner.
 */
int __fastcall HudRectI::IsCornerOutcode(
    int outcode
) {
    return outcode == 9 || outcode == 10 || outcode == 5 || outcode == 6 ? 1 : 0;
}

/**
 * Reimplements 0x4162b0: HudRectI::SegmentIntersectsEdge
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Test whether a segment crosses the requested rectangle edge.
 */
int HudRectI::SegmentIntersectsEdge(
    int edgeCode,
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd
) {
    zVec3 edgeStart = {0};
    zVec3 edgeEnd = {0};

    switch (edgeCode) {
    case 1:
        edgeStart.x = (float)(left);
        edgeStart.y = (float)(top);
        edgeEnd.x = (float)(left);
        edgeEnd.y = (float)(bottom);
        break;
    case 2:
        edgeStart.x = (float)(right);
        edgeStart.y = (float)(top);
        edgeEnd.x = (float)(right);
        edgeEnd.y = (float)(bottom);
        break;
    case 4:
        edgeStart.x = (float)(left);
        edgeStart.y = (float)(bottom);
        edgeEnd.x = (float)(right);
        edgeEnd.y = (float)(bottom);
        break;
    case 8:
        edgeStart.x = (float)(left);
        edgeStart.y = (float)(top);
        edgeEnd.x = (float)(right);
        edgeEnd.y = (float)(top);
        break;
    default:
        return 0;
    }

    const int edgeStartSide =
        HudGeom2D::ClassifyPointAgainstSegment(
            &edgeStart,
            &edgeEnd,
            segmentStart
        );
    const int edgeEndSide =
        HudGeom2D::ClassifyPointAgainstSegment(
            &edgeStart,
            &edgeEnd,
            segmentEnd
        );
    const int segEdgeStartSide =
        HudGeom2D::ClassifyPointAgainstSegment(
            segmentStart,
            segmentEnd,
            &edgeStart
        );
    const int segEdgeEndSide =
        HudGeom2D::ClassifyPointAgainstSegment(
            segmentStart,
            segmentEnd,
            &edgeEnd
        );

    if (edgeStartSide * edgeEndSide <= 0 && segEdgeStartSide * segEdgeEndSide <= 0) {
        return edgeCode;
    }

    return 0;
}

/**
 * Reimplements 0x415fb0: HudRectI::ClipOrSplitSegment
 * Source: D:\Proj\Battlesport\hud.cpp
 * Purpose: Clip or split a segment against this rectangle and preserve split output globals.
 */
int HudRectI::ClipOrSplitSegment(
    zVec3 *segmentStart,
    zVec3 *segmentEnd
) {
    if (left == right) {
        return 1;
    }

    int startOutcode = CalcOutcode(segmentStart);
    int endOutcode = CalcOutcode(segmentEnd);
    if (startOutcode == 0 && endOutcode == 0) {
        return 0;
    }
    if ((startOutcode & endOutcode) != 0) {
        return 1;
    }

    if ((startOutcode == 0) != (endOutcode == 0)) {
        if (startOutcode != 0) {
            zVec3 *const oldStart = segmentStart;
            segmentStart = segmentEnd;
            segmentEnd = oldStart;
            startOutcode = 0;
            endOutcode = CalcOutcode(segmentEnd);
        }

        if (SegmentIntersectsEdge(
            8,
            segmentStart,
            segmentEnd
        ) != 0) {
            HudLineClip::ClipEndpointToY(
                segmentStart,
                segmentEnd,
                (float)(top)
            );
            return 1;
        }
        if (SegmentIntersectsEdge(
            4,
            segmentStart,
            segmentEnd
        ) != 0) {
            HudLineClip::ClipEndpointToY(
                segmentStart,
                segmentEnd,
                (float)(bottom)
            );
            return 1;
        }
        if (SegmentIntersectsEdge(
            1,
            segmentStart,
            segmentEnd
        ) != 0) {
            HudLineClip::ClipEndpointToX(
                segmentStart,
                segmentEnd,
                (float)(left)
            );
            return 1;
        }
        if (SegmentIntersectsEdge(
            2,
            segmentStart,
            segmentEnd
        ) != 0) {
            HudLineClip::ClipEndpointToX(
                segmentStart,
                segmentEnd,
                (float)(right)
            );
            return 1;
        }
        return 0;
    }

    g_HudSensor_ClipSegmentStart = *segmentStart;
    g_HudSensor_ClipSegmentEnd = *segmentEnd;
    if ((SegmentIntersectsEdge(8, segmentStart, segmentEnd) |
            SegmentIntersectsEdge(
                4,
                segmentStart,
                segmentEnd
            ) |
            SegmentIntersectsEdge(
                1,
                segmentStart,
                segmentEnd
            ) |
            SegmentIntersectsEdge(
                2,
                segmentStart,
                segmentEnd
            )) == 0) {
        return 1;
    }

    if (IsCornerOutcode(startOutcode) != 0) {
        zVec3 *const oldStart = segmentStart;
        segmentStart = segmentEnd;
        segmentEnd = oldStart;
        const int oldStartOutcode = startOutcode;
        startOutcode = endOutcode;
        endOutcode = oldStartOutcode;
    }

    if ((startOutcode & 1) != 0) {
        HudLineClip::ClipEndpointToX(
            segmentStart,
            segmentEnd,
            (float)(left)
        );
    } else if ((startOutcode & 2) != 0) {
        HudLineClip::ClipEndpointToX(
            segmentStart,
            segmentEnd,
            (float)(right)
        );
    }

    if ((startOutcode & 8) != 0) {
        HudLineClip::ClipEndpointToY(
            segmentStart,
            segmentEnd,
            (float)(top)
        );
    } else if ((startOutcode & 4) != 0) {
        HudLineClip::ClipEndpointToY(
            segmentStart,
            segmentEnd,
            (float)(bottom)
        );
    }

    if ((endOutcode & 1) != 0) {
        HudLineClip::ClipEndpointToX(
            &g_HudSensor_ClipSegmentStart,
            &g_HudSensor_ClipSegmentEnd,
            (float)(left)
        );
    } else if ((endOutcode & 2) != 0) {
        HudLineClip::ClipEndpointToX(
            &g_HudSensor_ClipSegmentStart,
            &g_HudSensor_ClipSegmentEnd,
            (float)(right)
        );
    }

    if ((endOutcode & 8) != 0) {
        HudLineClip::ClipEndpointToY(
            &g_HudSensor_ClipSegmentStart,
            &g_HudSensor_ClipSegmentEnd,
            (float)(top)
        );
    } else if ((endOutcode & 4) != 0) {
        HudLineClip::ClipEndpointToY(
            &g_HudSensor_ClipSegmentStart,
            &g_HudSensor_ClipSegmentEnd,
            (float)(bottom)
        );
    }

    return 2;
}

/**
 * Reimplements 0x415ab0: HudSensorMapNode::Init
 * Source: D:\Proj\Battlesport\HudSensorMapNode.cpp
 * Purpose: Apply map-node defaults and return this node.
 */
HudSensorMapNode * HudSensorMapNode::Init() {
    InitDefaults();
    return this;
}

/**
 * Reimplements 0x415ac0: HudSensorMapNode::FreePointArray
 * Source: D:\Proj\Battlesport\HudSensorMapNode.cpp
 * Purpose: Release the dynamically loaded map point array when present.
 */
void HudSensorMapNode::FreePointArray() {
    if (points != 0) {
        free(points);
    }
}

/**
 * Reimplements 0x415ae0: HudSensorMapNode::SetEnabled
 * Source: D:\Proj\Battlesport\HudSensorMapNode.cpp
 * Purpose: Toggle marker visibility, refreshing color state and clearing point selection.
 */
int HudSensorMapNode::SetEnabled(
    int enabled
) {
    if (isEnabled == enabled) {
        return 0;
    }

    isEnabled = enabled;
    SetColorRgb(0);
    SelectPoint(-1);
    return 1;
}

/**
 * Reimplements 0x415b10: HudSensorMapNode::SelectPoint
 * Source: D:\Proj\Battlesport\HudSensorMapNode.cpp
 * Purpose: Select an in-range path point or clear the active selection.
 */
HudSensorMapPoint * HudSensorMapNode::SelectPoint(
    int pointIndex
) {
    if (pointIndex >= 0 && pointIndex < pointCount) {
        selectedPointIndex = pointIndex;
        return &points[pointIndex];
    }

    selectedPointIndex = -1;
    return 0;
}

/**
 * Reimplements 0x415b40: HudSensorMapNode::InitDefaults
 * Source: D:\Proj\Battlesport\HudSensorMapNode.cpp
 * Purpose: Initialize map-node links, point storage, marker state, and default color fields.
 */
int HudSensorMapNode::InitDefaults() {
    colorRgb[0] = (char)(0xff);
    colorRgb[1] = (char)(0xff);
    colorRgb[2] = (char)(0xff);
    pointCount = 0;
    points = 0;
    isEnabled = 0;
    blinkTimerSec = 0.0f;
    next = 0;
    objectiveIndex = -1;
    selectedPointIndex = -1;
    packedColor565Pair = -1;
    return 1;
}

/**
 * Reimplements 0x415b70: HudSensorMapNode::SetColorRgb
 * Source: D:\Proj\Battlesport\HudSensorMapNode.cpp
 * Purpose: Optionally copy RGB bytes and rebuild the full/half-intensity packed color pair.
 */
int HudSensorMapNode::SetColorRgb(
    const unsigned char *rgbOrNull
) {
    if (rgbOrNull != 0) {
        colorRgb[0] = (char)(rgbOrNull[0]);
        colorRgb[1] = (char)(rgbOrNull[1]);
        colorRgb[2] = (char)(rgbOrNull[2]);
    }

    const unsigned char red = (unsigned char)(colorRgb[0]);
    const unsigned char green = (unsigned char)(colorRgb[2]);
    const unsigned char blue = (unsigned char)(colorRgb[1]);
    const unsigned short fullColor = (unsigned short)(zVid_PackColorRGB(
        red,
        green,
        blue
    ));
    const unsigned short halfColor = (unsigned short)(zVid_PackColorRGB(
        (unsigned char)(red >> 1),
        (unsigned char)(green >> 1),
        (unsigned char)(blue >> 1)
    ));
    packedColor565Pair = ((int)(halfColor) << 16) | fullColor;
    return 1;
}

/**
 * Reimplements 0x415bd0: HudSensorMapNode::LoadFromStream
 * Source: D:\Proj\Battlesport\HudSensorMapNode.cpp
 * Purpose: Load color, points, and objective binding from a sensor-map stream.
 */
int HudSensorMapNode::LoadFromStream(
    FILE *stream
) {
    if (stream == 0) {
        return 0;
    }

    if (fread(
        colorRgb,
        3,
        1,
        stream
    ) != 1) {
        return 0;
    }

    if (fread(
        &pointCount,
        4,
        1,
        stream
    ) != 1) {
        return 0;
    }

    const size_t byteCount = (size_t)(pointCount) * sizeof(HudSensorMapPoint);
    points = (HudSensorMapPoint *)(malloc(byteCount));

    if (fread(points, sizeof(HudSensorMapPoint), (size_t)(pointCount), stream) !=
        (size_t)(pointCount)) {
        return 0;
    }

    if (fread(
        &objectiveIndex,
        4,
        1,
        stream
    ) != 1) {
        return 0;
    }

    SetColorRgb(0);
    UpdateCachedBounds(0);
    return 1;
}

/**
 * Reimplements 0x415c90: HudSensorMapNode::UpdateCachedBounds
 * Source: D:\Proj\Battlesport\HudSensorMapNode.cpp
 * Purpose: Copy cached bounds or recompute X/Z extents from the loaded point array.
 */
int HudSensorMapNode::UpdateCachedBounds(
    HudSensorMapBounds *outBoundsOrNull
) {
    if (outBoundsOrNull != 0) {
        *outBoundsOrNull = cachedBounds;
        return 1;
    }

    HudSensorMapPoint *point = points;
    cachedBounds.minX = point->x;
    cachedBounds.maxX = point->x;
    cachedBounds.minY = 0.0f;
    cachedBounds.minZ = point->z;
    cachedBounds.maxZ = point->z;

    ++point;
    {
        for (int remaining = pointCount - 1; remaining != 0; --remaining, ++point) {
            if (point->x < cachedBounds.minX) {
                cachedBounds.minX = point->x;
            }

            if (point->x > cachedBounds.maxX) {
                cachedBounds.maxX = point->x;
            }

            if (point->z < cachedBounds.minZ) {
                cachedBounds.minZ = point->z;
            }

            if (point->z > cachedBounds.maxZ) {
                cachedBounds.maxZ = point->z;
            }
        }
    }

    return 1;
}

/**
 * Reimplements 0x416660: HudSensorTracker::Init
 * Source: D:\Proj\Battlesport\HudSensorTracker.cpp
 * Purpose: Initialize map bounds, save-state marker state, and map runtime defaults.
 */
void HudSensorTracker::Init(
    const HudUiRect *outerRectOrNull
) {
    mapFileVersion = 5;
    mapHeaderDword = 0;
    mapBoundsMaxZ = 0.0f;
    unknown_38 = 0;
    mapBoundsMaxX = 0.0f;
    mapBoundsMinZ = 0.0f;
    unknown_2c = 0;
    mapBoundsMinX = 0.0f;
    mapNodeListHead = 0;
    loadedMapPath = 0;
    mapScaleLerpActive = 0;
    mapLoadedFlag = 0;
    mapScaleCurrent.x = 0.0f;
    mapScaleCurrent.z = 0.0f;
    mapZoom = 0.7f;
    SetBounds(
        outerRectOrNull,
        0
    );
    SetTrackedSaveState(0);
    mapWorldNode = 0;
    mapSndOff = 0;
    mapSndOn = 0;
    SetSaveStateMarkerMaxDistance(450.0f);
}

/**
 * Reimplements 0x416650: HudSensorTracker::InitNoBounds
 * Source: D:\Proj\Battlesport\HudSensorTracker.cpp
 * Purpose: Initialize tracker state without replacing the existing map bounds.
 */
HudSensorTracker * HudSensorTracker::InitNoBounds() {
    Init(0);
    return this;
}

// Reimplements 0x417390: HudSensorTracker::Constructor
// (D:\Proj\Battlesport\map.cpp)
HudSensorTracker * HudSensorTracker::Constructor() {
    InitNoBounds();
    missionDataPath.Empty();
    zbdPath.Empty();
    missionGsPath.Empty();
    fxPass3Obj = 0;
    hudScale = 1.0f;
    raceCheckpointMode = 0;
    hasPendingPlayerSave = 0;
    pendingPlayerSave.skipTimerResetOnStart = 0;
    ResetMissionState();
    return this;
}

// Reimplements 0x417360: HudSensorTracker::ConstructGlobal
// (D:\Proj\Battlesport\map.cpp)
HudSensorTracker *HudSensorTracker::ConstructGlobal() {
    return g_HudSensorTracker.Constructor();
}

// Reimplements 0x417370: HudSensorTracker::RegisterGlobalOnExit
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::RegisterGlobalOnExit() {
    atexit(&HudSensorTracker::ShutdownGlobal);
}

// Reimplements 0x417380: HudSensorTracker::ShutdownGlobal
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::ShutdownGlobal() {
    g_HudSensorTracker.Shutdown();
}

/**
 * Reimplements 0x4166e0: HudSensorTracker::SetBounds
 * Source: D:\Proj\Battlesport\HudSensorTracker.cpp
 * Purpose: Copy HUD map bounds and cache the overlay center from the outer rect.
 */
void HudSensorTracker::SetBounds(
    const HudUiRect *outerRectIn,
    const HudUiRect *innerRectOrNull
) {
    if (outerRectIn == 0) {
        return;
    }

    outerRect = *outerRectIn;
    if (innerRectOrNull != 0) {
        innerRectExpanded = *innerRectOrNull;
        --innerRectExpanded.top;
        ++innerRectExpanded.right;
        --innerRectExpanded.left;
        ++innerRectExpanded.bottom;
    } else {
        innerRectExpanded.left = 0;
        innerRectExpanded.top = 0;
        innerRectExpanded.right = 0;
        innerRectExpanded.bottom = 0;
    }

    mapOverlayCenterX = outerRect.left + ((outerRect.right - outerRect.left) / 2);
    mapOverlayCenterY = outerRect.top + ((outerRect.bottom - outerRect.top) / 2);
}

 /**
  * Reimplements 0x416ef0: HudSensorTracker::SetSaveStateMarkerMaxDistance
 * Source: D:\Proj\Battlesport\HudSensorTracker.cpp
  * Purpose: Store the squared maximum distance for drawing save-state markers.
  */
int HudSensorTracker::SetSaveStateMarkerMaxDistance(
    float maxDist
) {
    saveStateMarkerMaxDistSq = maxDist * maxDist;
    return 1;
}

 /**
  * Reimplements 0x417220: HudSensorTracker::SetTrackedSaveState
 * Source: D:\Proj\Battlesport\HudSensorTracker.cpp
  * Purpose: Select the save-state player pose used by the HUD map marker.
  */
int HudSensorTracker::SetTrackedSaveState(
    zUtil_SaveGameState *saveState
) {
    zVec3 *trackedForwardVec = 0;
    if (saveState == 0) {
        trackedSaveStateSelection = 0;
        trackedWorldOriginPtr = 0;
    } else {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        trackedSaveStateSelection = saveState;
        trackedWorldOriginPtr = &playerState->worldPos;
        trackedForwardVec = &playerState->cameraBasisCache;
    }

    trackedForwardVecPtr = trackedForwardVec;
    mapZoom = 1.0f;
    return 1;
}

/**
 * Reimplements 0x416ad0: HudSensorTracker::MapOverlayEndShow
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Stop the active map overlay lerp and queue the deterministic map-off sound path.
 */
void HudSensorTracker::MapOverlayEndShow() {
    if (mapScaleLerpActive == 0) {
        return;
    }

    mapScaleLerpT = 0.0f;
    mapScaleStart.x = mapScaleCurrent.x;
    mapScaleGoal.x = 0.0f;
    mapScaleGoal.z = 0.0f;
    mapScaleStart.y = mapScaleCurrent.y;
    mapScaleLerpActive = 0;
    mapScaleStart.z = mapScaleCurrent.z;

    if (mapLoadedFlag != 0) {
        mapScaleLerpRunning = 1;
        mapSndOff->PlayA3DSimple(1.0f);
    }
}

/**
 * Reimplements 0x416a30: HudSensorTracker::MapOverlayBeginShow
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Begin the map overlay scale lerp from the current scale to the fitted map bounds scale.
 */
int HudSensorTracker::MapOverlayBeginShow() {
    if (mapScaleLerpActive != 0) {
        return 0;
    }

    const int rectWidth = outerRect.right - outerRect.left;
    const int rectHeight = outerRect.bottom - outerRect.top;
    const int minExtent = rectWidth < rectHeight ? rectWidth : rectHeight;
    const float scaleExtent = (float)(minExtent);

    mapScaleLerpT = 0.0f;
    mapScaleLerpActive = 1;
    mapScaleStart = mapScaleCurrent;
    mapScaleGoal.x = scaleExtent / (mapBoundsMaxX - mapBoundsMinX);
    mapScaleGoal.z = scaleExtent / (mapBoundsMaxZ - mapBoundsMinZ);

    if (mapLoadedFlag != 0) {
        mapSndOn->PlayA3DSimple(1.0f);
        mapScaleLerpRunning = 1;
    }

    return 1;
}

/**
 * Reimplements 0x416b30: HudSensorTracker::MapOverlayRefToggle
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Reference-count map overlay visibility requests and route transitions through begin/end show.
 */
int HudSensorTracker::MapOverlayRefToggle(
    int enable
) {
    if (enable != 0) {
        ++g_Hud_MapOverlayRefCount;
        if (g_Hud_MapOverlayRefCount > 0) {
            MapOverlayBeginShow();
        }
    } else {
        --g_Hud_MapOverlayRefCount;
        if (g_Hud_MapOverlayRefCount <= 0) {
            g_Hud_MapOverlayRefCount = 0;
            MapOverlayEndShow();
        }
    }

    return 1;
}

/**
 * Reimplements 0x416b80: HudSensorTracker::MapZoomIn
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Increase the active overlay zoom and play the map click sound while the overlay is shown.
 */
void HudSensorTracker::MapZoomIn() {
    if (mapScaleLerpActive != 0) {
        mapZoom *= 1.10000002f;
        mapSndClick->PlayA3DSimple(1.0f);
    }
}

/**
 * Reimplements 0x416bb0: HudSensorTracker::MapZoomOut
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Decrease the active overlay zoom and play the map click sound while the overlay is shown.
 */
void HudSensorTracker::MapZoomOut() {
    if (mapScaleLerpActive != 0) {
        mapZoom *= 0.899999976f;
        mapSndClick->PlayA3DSimple(1.0f);
    }
}

/**
 * Reimplements 0x416be0: HudSensorTracker::UpdateMapScaleLerp
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Advance the map overlay scale interpolation and update the current overlay scale vector.
 */
int HudSensorTracker::UpdateMapScaleLerp() {
    if (mapScaleLerpRunning != 0) {
        mapScaleLerpT += mapScaleLerpStep;
        if (mapScaleLerpT >= 1.0f) {
            mapScaleLerpRunning = 0;
            mapScaleLerpT = 1.0f;
        }

        const float lerpT = mapScaleLerpT;
        mapScaleCurrent.x = (mapScaleGoal.x - mapScaleStart.x) * lerpT + mapScaleStart.x;
        mapScaleCurrent.y = (mapScaleGoal.y - mapScaleStart.y) * lerpT + mapScaleStart.y;
        mapScaleCurrent.z = (mapScaleGoal.z - mapScaleStart.z) * lerpT + mapScaleStart.z;
    }

    return 1;
}

/**
 * Reimplements 0x416c90: HudSensorTracker::ProjectWorldPointsToOverlay
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Project world-space map points into overlay coordinates using the tracked origin and forward vector.
 */
int HudSensorTracker::ProjectWorldPointsToOverlay(
    const zVec3 *inputWorldPoints,
    zVec3 *projectedOverlayPoints,
    int pointCount
) {
    const zVec3 *const trackedPos = trackedWorldOriginPtr;
    const zVec3 *const trackedForward = trackedForwardVecPtr;

    {
        for (int index = 0; index < pointCount; ++index) {
            const float deltaX =
                (inputWorldPoints[index].x - trackedPos->x) * mapZoom * mapScaleCurrent.x;
            const float deltaZ =
                (inputWorldPoints[index].z - trackedPos->z) * mapScaleCurrent.z * mapZoom;

            projectedOverlayPoints[index].x = (float)(mapOverlayCenterX) +
                                              trackedForward->x * deltaZ -
                                              trackedForward->z * deltaX;
            projectedOverlayPoints[index].y =
                (float)(mapOverlayCenterY)-trackedForward->x * deltaX - trackedForward->z * deltaZ;
        }
    }

    return 1;
}

// Reimplements 0x416e50: HudSensorTracker::GetSaveStateRelativeVectorLen
// (D:\Proj\Battlesport\map.cpp)
float HudSensorTracker::GetSaveStateRelativeVectorLen(
    zUtil_SaveGameState *saveState,
    zVec3 *relativeDelta,
    int takeSqrt
) {
    const zVec3 *const saveStatePos = &saveState->playerState->worldPos;
    const zVec3 *const trackedOrigin = trackedWorldOriginPtr;

    relativeDelta->x = saveStatePos->x - trackedOrigin->x;
    relativeDelta->y = saveStatePos->y - trackedOrigin->y;
    relativeDelta->z = saveStatePos->z - trackedOrigin->z;
    relativeDelta->y = 0.0f;

    const float lengthSq = relativeDelta->x * relativeDelta->x +
                           relativeDelta->y * relativeDelta->y +
                           relativeDelta->z * relativeDelta->z;
    if (takeSqrt != 0) {
        return sqrt(lengthSq);
    }

    return lengthSq;
}

/**
 * Reimplements 0x416dd0: HudSensorTracker::DrawMarkerCross
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Draw a centered cross marker as two clipped immediate line strips.
 */
void __fastcall HudSensorTracker::DrawMarkerCross(
    int centerX,
    int centerY,
    int armHalfWidth,
    int armHalfHeight,
    int markerColor,
    HudSensorTracker *tracker
) {
    zRndr_LinePoint2I points[2];
    const int color16 = markerColor & 0xffff;

    points[0].x = centerX - armHalfWidth;
    points[0].y = centerY;
    points[1].x = centerX + armHalfWidth;
    points[1].y = centerY;
    zRndr_DrawClippedImmediateLineStrip(
        points,
        1,
        tracker,
        color16
    );

    points[0].x = centerX;
    points[0].y = centerY + armHalfHeight;
    points[1].x = centerX;
    points[1].y = centerY - armHalfHeight;
    zRndr_DrawClippedImmediateLineStrip(
        points,
        1,
        tracker,
        color16
    );
}

/**
 * Reimplements 0x415f40: HudSensorTracker::DrawDiamondMarker
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Draw a centered diamond marker as a closed clipped immediate line strip.
 */
void __fastcall HudSensorTracker::DrawDiamondMarker(
    int centerX,
    int centerY,
    int halfWidth,
    int halfHeight,
    int markerColor,
    HudSensorTracker *tracker
) {
    zRndr_LinePoint2I points[5];

    points[0].x = centerX - halfWidth;
    points[0].y = centerY;
    points[1].x = centerX;
    points[1].y = centerY + halfHeight;
    points[2].x = centerX + halfWidth;
    points[2].y = centerY;
    points[3].x = centerX;
    points[3].y = centerY - halfHeight;
    points[4].x = points[0].x;
    points[4].y = centerY;

    zRndr_DrawClippedImmediateLineStrip(
        points,
        4,
        tracker,
        markerColor & 0xffff
    );
}

/**
 * Reimplements 0x416480: HudSensorMapNode::DrawProjectedPath
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Draw the camera-projected sensor-map path with clipped immediate line strips.
 */
int HudSensorMapNode::DrawProjectedPath(
    HudSensorTracker *tracker
) {
    if (pointCount == 0) {
        return 1;
    }

    zMat4x3 cameraScratchMatrix;
    zMath::MatStackPushPtr((float *)(&cameraScratchMatrix));
    zMath::MatLoadCameraScratchB();

    if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
        memcpy(
            g_HudSensor_ProjectScratch,
            points,
            (size_t)(pointCount) * sizeof(zVec3)
        );
    } else {
        const zMat4x3 *const matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
        for (int i = 0; i < pointCount; ++i) {
            const HudSensorMapPoint *const sourcePoint = &points[i];
            zVec3 *const projectedPoint = &g_HudSensor_ProjectScratch[i];

            projectedPoint->x = sourcePoint->x * matrix->xx + sourcePoint->y * matrix->yx +
                                sourcePoint->z * matrix->zx + matrix->posX;
            projectedPoint->z = sourcePoint->x * matrix->xz + sourcePoint->y * matrix->yz +
                                sourcePoint->z * matrix->zz + matrix->posZ;
            projectedPoint->y = sourcePoint->x * matrix->xy + sourcePoint->y * matrix->yy +
                                sourcePoint->z * matrix->zy + matrix->posY;
        }
    }

    zMath::MatStackPopPtr();

    g_HudSensor_ProjectScratch[pointCount] = g_HudSensor_ProjectScratch[0];

    for (int i = 0; i < pointCount; ++i) {
        zVec3 segmentPoints[2];
        segmentPoints[0] = g_HudSensor_ProjectScratch[i];
        segmentPoints[1] = g_HudSensor_ProjectScratch[i + 1];

        if (zMath::ClipLineSegmentToZRange(
            &segmentPoints[0],
            &segmentPoints[1]
        ) == 0) {
            continue;
        }

        zMath::ProjectPointBatch(
            segmentPoints,
            (zProjectedPoint *)(segmentPoints),
            2
        );

        zRndr_LinePoint2I linePoints[2];
        linePoints[0].x = (int)(segmentPoints[0].x) << 1;
        linePoints[0].y = (int)(segmentPoints[0].y) << 1;
        linePoints[1].x = (int)(segmentPoints[1].x) << 1;
        linePoints[1].y = (int)(segmentPoints[1].y) << 1;

        zRndr_DrawClippedImmediateLineStrip(
            linePoints,
            1,
            tracker,
            (unsigned int)(packedColor565Pair) >> 16
        );
    }

    return 1;
}

/**
 * Reimplements 0x415d30: HudSensorMapNode::DrawOnTracker
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Draw this map node on the tracker, including blink state and selected-point marker.
 */
int HudSensorMapNode::DrawOnTracker(
    HudSensorTracker *tracker,
    const zVec3 *drawPathWorldPos
) {
    if (isEnabled != 0) {
        blinkTimerSec -= 0.075000003f;
        if (blinkTimerSec <= 0.0f) {
            const unsigned int colorPair = (unsigned int)(packedColor565Pair);
            blinkTimerSec = 0.25f;
            packedColor565Pair = (int)((colorPair << 16) | (colorPair >> 16));
        }
    }

    zVec3 projectedPathPointBuffer[0x401];
    tracker->ProjectWorldPointsToOverlay(
        (const zVec3 *)(points),
        projectedPathPointBuffer,
        pointCount
    );
    projectedPathPointBuffer[pointCount] = projectedPathPointBuffer[0];

    for (int i = 0; i < pointCount; ++i) {
        zVec3 segmentStart = projectedPathPointBuffer[i];
        zVec3 segmentEnd = projectedPathPointBuffer[i + 1];
        int point0Clipped;
        int point1Clipped;

        HudLineClip::SetCurrentBoundsFromRectI((const HudRectI *)(&tracker->outerRect));
        if (HudLineClip::ClipSegmentToCurrentBounds(
                &segmentStart,
                &segmentEnd,
                &point0Clipped,
                &point1Clipped
            ) == 0) {
            continue;
        }

        const int splitResult = ((HudRectI *)(&tracker->innerRectExpanded))
                                    ->ClipOrSplitSegment(
                                        &segmentStart,
                                        &segmentEnd
                                    );
        if (splitResult == 0) {
            continue;
        }

        const int color16 = packedColor565Pair & 0xffff;
        zRndr_DrawImmediateLine(
            (int)(segmentStart.x),
            (int)(segmentStart.y),
            (int)(segmentEnd.x),
            (int)(segmentEnd.y),
            color16
        );

        if (splitResult == 2) {
            zRndr_DrawImmediateLine(
                (int)(g_HudSensor_ClipSegmentStart.x),
                (int)(g_HudSensor_ClipSegmentStart.y),
                (int)(g_HudSensor_ClipSegmentEnd.x),
                (int)(g_HudSensor_ClipSegmentEnd.y),
                color16
            );
        }
    }

    if (selectedPointIndex != -1) {
        zVec3 selectedPoint;
        tracker->ProjectWorldPointsToOverlay(
            (const zVec3 *)(&points[selectedPointIndex]),
            &selectedPoint,
            1
        );
        HudSensorTracker::DrawDiamondMarker(
            (int)(selectedPoint.x),
            (int)(selectedPoint.y),
            4,
            4,
            (unsigned int)(packedColor565Pair) >> 16,
            tracker
        );
    }

    if (drawPathWorldPos != 0) {
        DrawProjectedPath(tracker);
    }

    return 1;
}

// Reimplements 0x416d50: HudSensorTracker::DrawTrackedSaveStateMarker
// (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::DrawTrackedSaveStateMarker() {
    unsigned short markerColor;
    if (zOpt::GetNetworkEnabled() != 0) {
        zUtil_SaveGameState *const gameState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
        markerColor =
            (unsigned short)(zVid_PackColor00RRGGBB(gameState->netPlayerRow->playerColorPackedRgb));
    } else {
        markerColor = (unsigned short)(zVid_PackColorRGB(
            0,
            0xff,
            0
        ));
    }

    zVec3 projectedScreenPoint;
    ProjectWorldPointsToOverlay(
        &trackedSaveStateSelection->playerState->worldPos,
        &projectedScreenPoint,
        1
    );
    DrawMarkerCross(
        (int)(projectedScreenPoint.x),
        (int)(projectedScreenPoint.y),
        3,
        3,
        markerColor,
        this
    );
    return 1;
}

// Reimplements 0x416f10: HudSensorTracker::DrawSaveStateMarker
// (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::DrawSaveStateMarker(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->lifecycleState == 4 || saveState == trackedSaveStateSelection) {
        return 0;
    }

    zVec3 relativeDelta;
    const float distanceSq = GetSaveStateRelativeVectorLen(
        saveState,
        &relativeDelta,
        0
    );

    zVec3 markerPoint;
    if (saveStateMarkerMaxDistSq != 0.0f && distanceSq > saveStateMarkerMaxDistSq) {
        if (zOpt::GetNetworkEnabled() == 0) {
            return 0;
        }

        const float edgeScale = ApproxSqrtScaleFromBits(saveStateMarkerMaxDistSq / distanceSq);
        relativeDelta.x *= edgeScale;
        relativeDelta.y *= edgeScale;
        relativeDelta.z *= edgeScale;

        const zVec3 *const localWorldPos =
            &((zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState))->worldPos;
        markerPoint.x = localWorldPos->x + relativeDelta.x;
        markerPoint.y = localWorldPos->y + relativeDelta.y;
        markerPoint.z = localWorldPos->z + relativeDelta.z;
        ProjectWorldPointsToOverlay(
            &markerPoint,
            &markerPoint,
            1
        );

        const unsigned short markerColor =
            (unsigned short)(zVid_PackColor00RRGGBB(saveState->netPlayerRow->playerColorPackedRgb));
        if (IsPointStrictlyInsideRect(
            outerRect,
            markerPoint
        )) {
            DrawMarkerCross(
                (int)(markerPoint.x),
                (int)(markerPoint.y),
                3,
                3,
                markerColor,
                this
            );
        }

        return 1;
    }

    ProjectWorldPointsToOverlay(
        &playerState->worldPos,
        &markerPoint,
        1
    );

    unsigned short markerColor;
    if (zOpt::GetNetworkEnabled() != 0) {
        markerColor =
            (unsigned short)(zVid_PackColor00RRGGBB(saveState->netPlayerRow->playerColorPackedRgb));
    } else {
        markerColor = (unsigned short)(zVid_PackColorRGB(
            0xff,
            0,
            0
        ));
    }

    if (IsPointStrictlyInsideRect(
        outerRect,
        markerPoint
    )) {
        zRndr_SpanOcclusion_TestSample(
            (int)(markerPoint.x),
            (int)(markerPoint.y),
            markerColor
        );
    }

    return 1;
}

// Reimplements 0x417130: HudSensorTracker::Update
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::Update() {
    mapScaleLerpStep = 0.150000006f;
    if (mapScaleLerpActive == 0 && mapScaleLerpRunning == 0) {
        return;
    }

    UpdateMapScaleLerp();

    if (trackedWorldOriginPtr == 0) {
        trackedWorldFallbackOrigin.x = mapBoundsMinX - (mapBoundsMaxX - mapBoundsMinX) * -0.5f;
        trackedWorldFallbackOrigin.z = mapBoundsMinZ - (mapBoundsMaxZ - mapBoundsMinZ) * -0.5f;
        trackedWorldOriginPtr = &trackedWorldFallbackOrigin;
    }

    if (trackedForwardVecPtr == 0) {
        trackedForwardFallbackVec.x = 0.0f;
        trackedForwardFallbackVec.y = 0.0f;
        trackedForwardFallbackVec.z = -1.0f;
    }

    if (zOpt::GetNetworkEnabled() == 0) {
        HudSensorMapNode *mapNode = mapNodeListHead;
        while (mapNode != 0) {
            mapNode->DrawOnTracker(
                this,
                trackedWorldPosPtr
            );
            mapNode = mapNode->next;
        }
    }

    zUtil_SaveGameState *saveState =
        g_PlayerSaveStateListHead != 0 ? g_PlayerSaveStateListHead->next : 0;
    while (saveState != 0) {
        DrawSaveStateMarker(saveState);
        saveState = saveState != 0 ? saveState->next : 0;
    }

    if (mapLoadedFlag != 0) {
        DrawTrackedSaveStateMarker();
    }
}

/**
 * Reimplements 0x4167e0: HudSensorTracker::MapRemoveNode
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Unlink the requested map node from the tracker list and release head-node storage.
 */
int HudSensorTracker::MapRemoveNode(
    HudSensorMapNode *mapNode
) {
    HudSensorMapNode *head = mapNodeListHead;
    if (mapNode == head) {
        mapNodeListHead = head->next;
        if (mapNode != 0) {
            mapNode->FreePointArray();
            ::operator delete(mapNode);
        }

        return 1;
    }

    if (head == 0) {
        return 0;
    }

    while (head->next != mapNode) {
        head = head->next;
        if (head == 0) {
            return 0;
        }
    }

    if (head->next != mapNode) {
        return 0;
    }

    head->next = mapNode->next;
    return 1;
}

/**
 * Reimplements 0x416840: HudSensorTracker::MapInsertNodeAndGrowBounds
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Insert a map node at the list head and grow the tracker bounds from its cached extent.
 */
int HudSensorTracker::MapInsertNodeAndGrowBounds(
    HudSensorMapNode *mapNode
) {
    if (mapNode == 0) {
        return 0;
    }

    mapNode->next = mapNodeListHead;
    mapNodeListHead = mapNode;

    HudSensorMapBounds nodeBounds;
    mapNode->UpdateCachedBounds(&nodeBounds);

    if (nodeBounds.minX < mapBoundsMinX) {
        mapBoundsMinX = nodeBounds.minX;
    }
    if (nodeBounds.minZ < mapBoundsMinZ) {
        mapBoundsMinZ = nodeBounds.minZ;
    }
    if (nodeBounds.maxX > mapBoundsMaxX) {
        mapBoundsMaxX = nodeBounds.maxX;
    }
    if (nodeBounds.maxZ > mapBoundsMaxZ) {
        mapBoundsMaxZ = nodeBounds.maxZ;
    }

    return 1;
}

/**
 * Reimplements 0x4168d0: HudSensorTracker::LoadMapFromStream
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Read a versioned map stream into tracker bounds and linked map nodes.
 */
int HudSensorTracker::LoadMapFromStream(
    FILE *stream
) {
    if (stream == 0) {
        return 0;
    }

    fread(
        &mapFileVersion,
        sizeof(mapFileVersion),
        1,
        stream
    );
    if (mapFileVersion != 5) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\map.cpp",
            0x30e,
            "Incorrect Map File Version (found %d, wanted %d)\n",
            mapFileVersion,
            5
        );
        return 0;
    }

    fread(
        &mapHeaderDword,
        sizeof(mapHeaderDword),
        1,
        stream
    );
    fread(
        &mapBoundsMinX,
        sizeof(HudSensorMapBounds),
        1,
        stream
    );

    for (;;) {
        HudSensorMapNode *mapNode = (HudSensorMapNode *)(::operator new(sizeof(HudSensorMapNode)));
        mapNode = mapNode != 0 ? mapNode->Init() : 0;
        if (mapNode->LoadFromStream(stream) == 0) {
            if (mapNode != 0) {
                mapNode->FreePointArray();
                ::operator delete(mapNode);
            }
            break;
        }

        MapInsertNodeAndGrowBounds(mapNode);
    }

    mapLoadedFlag = 1;
    return 1;
}

/**
 * Reimplements 0x4169d0: HudSensorTracker::LoadMapFromPath
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Open a map file path, remember it, and load the tracker map from the stream.
 */
int HudSensorTracker::LoadMapFromPath(
    const char *path
) {
    if (path == 0) {
        return 0;
    }

    FILE *const stream = fopen(
        path,
        "rb"
    );
    if (stream == 0) {
        return 0;
    }

    loadedMapPath = _strdup(path);
    const int result = LoadMapFromStream(stream);
    fclose(stream);
    return result;
}

/**
 * Reimplements 0x417260: HudSensorTracker::LoadMissionMapAndSfx
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Load the mission map path and resolve the map on, off, and click samples.
 */
int HudSensorTracker::LoadMissionMapAndSfx(
    int missionIdValue
) {
    char mapPath[0x40];
    sprintf(
        mapPath,
        ".\\maps\\m%d.zmap",
        missionIdValue
    );

    const int result = LoadMapFromPath(mapPath);
    mapSndOn = zSnd::FindSampleByName("snd_mapOn");
    mapSndOff = zSnd::FindSampleByName("snd_mapOff");
    mapSndClick = zSnd::FindSampleByName("snd_mapClick");
    return result;
}

/**
 * Reimplements 0x416790: HudSensorTracker::MapShutdownAndResetThunk
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Tail-call the shared map shutdown and reset routine.
 */
int HudSensorTracker::MapShutdownAndResetThunk() {
    return MapShutdownAndReset();
}

/**
 * Reimplements 0x4167a0: HudSensorTracker::MapShutdownAndReset
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: End the overlay, remove loaded map nodes, free the map path, and reset map state.
 */
int HudSensorTracker::MapShutdownAndReset() {
    MapOverlayEndShow();
    while (mapNodeListHead != 0) {
        MapRemoveNode(mapNodeListHead);
    }

    if (loadedMapPath != 0) {
        free(loadedMapPath);
    }

    Init(0);
    return 1;
}

// Reimplements 0x419490: HudSensorTracker::Shutdown
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::Shutdown() {
    missionGsPath.Empty();
    zbdPath.Empty();
    missionDataPath.Empty();
    MapShutdownAndResetThunk();
}

// Reimplements 0x4176f0: HudSensorTracker::ResetMissionState (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::ResetMissionState() {
    missionLoaded = 0;
    missionId = 0;
    missionDataPath.Empty();
    zbdPath.Empty();
    HudUiElement *const fxElement = fxPass3Obj;
    worldNode = 0;
    missionFlags = 1;
    objectiveCount = 0;

    if (fxElement != 0) {
        fxElement->SetVisible(0);
        ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->RemoveChild(fxPass3Obj);

        if (fxPass3Obj != 0) {
            HudWeatherFx *const weatherFx = (HudWeatherFx *)(fxPass3Obj);
            weatherFx->Destructor();
            ::operator delete(weatherFx);
        }

        fxPass3Obj = 0;
    }

    return 1;
}

// Reimplements 0x417f60: HudSensorObjectiveSlot::Reset
void HudSensorObjectiveSlot::Reset() {
    zVidImagePartial *const image = objectiveImage;
    objectiveTitle[0] = '\0';
    objectiveDesc[0] = '\0';
    objectiveSummary[0] = '\0';
    completedFlag = 0;
    if (image != 0) {
        zVid_Image::ReleaseIfNotDefault(image);
        objectiveImage = 0;
    }
}

// Reimplements 0x417430: HudSensorTracker::WriteMissionDataSection
int HudSensorTracker::WriteMissionDataSection(
    zZbdSectionCallbackCtx *writer
) {
    HudSensorTrackerMissionData missionData = {0};
    missionData.missionId = missionId;
    missionData.missionFlags = missionFlags;
    missionData.currentObjectiveIndex = currentObjectiveIndex;
    missionData.firstIncompleteObjectiveIndex = firstIncompleteObjectiveIndex;
    missionData.completedObjectiveCount = completedObjectiveCount;
    missionData.objectiveFlowState = objectiveFlowState;
    missionData.objectiveFlowDeadlineSecRaw = objectiveFlowDeadlineSecRaw;
    missionData.missionStat0 = missionStat0;
    missionData.missionStat1 = missionStat1;
    missionData.missionStat2 = primaryGunDispatchCount;
    missionData.missionStat3 = missionStat3;
    missionData.weaponsFoundMask = weaponsFoundMask;

    {
        for (int index = 0; index < 10; ++index) {
            missionData.objectiveCompletedFlags[index] = objectiveSlots[index].completedFlag;
        }
    }

    missionData.difficultyMode = zOpt::GetGameDifficultyMode();
    return zUtil_ZAR::WriteSectionBlob(
        writer,
        "MissionData",
        &missionData,
        sizeof(missionData)
    );
}

// Reimplements 0x4174f0: HudSensorTracker::ApplyMissionDataAndReload
// (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::ApplyMissionDataAndReload(
    void *,
    const char *,
    const void *missionDataBlob,
    unsigned int
) {
    const HudSensorTrackerMissionData *const missionData =
        (const HudSensorTrackerMissionData *)(missionDataBlob);

    pendingPlayerSave.skipTimerResetOnStart = 1;
    completedObjectiveCount = missionData->completedObjectiveCount;
    zOpt::SetGameDifficultyMode(missionData->difficultyMode);

    const int currentMissionId = missionId;
    if (currentMissionId == 0) {
        InitMissionIdAndFlags(
            missionData->missionId,
            missionData->missionFlags
        );
        zUtil::ZAR_RequestStopGlobal();
        return 1;
    }

    if (missionData->missionId != currentMissionId) {
        ShutdownMissionGameplaySystems();
        RecoilStateMainMenuTransition::ClearPausedAudioSnapshot();
        InitMissionIdAndFlags(
            missionData->missionId,
            missionData->missionFlags
        );
        zUtil_ZRDR_UnloadMountedArchives(0);
        zUtil::SetMissionZrdrPathsAndMountZbd(missionData->missionId);
        LoadObjectivesFromPath("objectives.zrd");
        LoadMissionCoreResources();
        InitMissionGameplaySystems();
        if (zVid::GetAccelerationOption() != 0) {
            zClass_Camera::SetActiveCamera(0);
            zClass_Camera::SetObjectHseTestEnabled(0);
        }
    }

    completedObjectiveCount = missionData->completedObjectiveCount;
    currentObjectiveIndex = missionData->currentObjectiveIndex;
    firstIncompleteObjectiveIndex = missionData->firstIncompleteObjectiveIndex;
    objectiveFlowState = missionData->objectiveFlowState;
    objectiveFlowDeadlineSecRaw = missionData->objectiveFlowDeadlineSecRaw;
    missionStat0 = missionData->missionStat0;
    missionStat1 = missionData->missionStat1;
    primaryGunDispatchCount = missionData->missionStat2;
    missionStat3 = missionData->missionStat3;
    weaponsFoundMask = missionData->weaponsFoundMask;

    for (int objectiveIndex = 0; objectiveIndex < 10; ++objectiveIndex) {
        const int completedFlag = missionData->objectiveCompletedFlags[objectiveIndex];
        objectiveSlots[objectiveIndex].completedFlag = completedFlag;
        if (completedFlag != 0) {
            SetObjectiveMarkerEnabledAndColor(
                objectiveIndex,
                0,
                0
            );
            SetObjectiveMarkerColorBlink(
                objectiveIndex,
                g_HudSensorTracker_ObjectiveBlinkColorRedRgb24
            );
        }
    }

    return 1;
}

// Reimplements 0x417640: HudSensorTracker::RegisterMissionSectionHandlers
void HudSensorTracker::RegisterMissionSectionHandlers() {
    zUtil_ZAR::RegisterSectionHandler(
        "Mission",
        ZbdCallbackPtr(&HudSensorTracker::ZarMission_SaveCallback),
        ZbdCallbackPtr(&HudSensorTracker::ZarMission_RestoreCallback),
        0,
        this
    );
    zUtil_ZAR::RegisterSectionHandler(
        "MissionLate",
        ZbdCallbackPtr(&HudSensorTracker::ZarMissionLate_SaveCallback),
        ZbdCallbackPtr(&HudSensorTracker::ZarMissionLate_RestoreCallback),
        0x7d0,
        this
    );
}

// Reimplements 0x417770: HudSensorTracker::InitMissionIdAndFlags
int HudSensorTracker::InitMissionIdAndFlags(
    int newMissionId,
    int flags
) {
    missionFlags = flags;
    missionId = newMissionId;
    if (newMissionId != 0) {
        zbdPath.Empty();
    }

    return 1;
}

// Reimplements 0x4177d0: HudSensorTracker::SetZbdPath
int HudSensorTracker::SetZbdPath(
    const char *path
) {
    if (path != 0) {
        zbdPath = path;
    } else {
        zbdPath.Empty();
    }

    return 1;
}

// Reimplements 0x4177a0: HudSensorTracker::SetMissionId
int HudSensorTracker::SetMissionId(
    int newMissionId
) {
    missionId = newMissionId;
    if (newMissionId != 0) {
        zbdPath.Empty();
    }

    return 1;
}

/**
 * Reimplements 0x417800: HudSensorTracker::GetMissionId
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Return the mission id currently owned by the HUD sensor tracker.
 */
int HudSensorTracker::GetMissionId() {
    return missionId;
}

// Reimplements 0x417810: HudSensorTracker::LoadMissionCoreResources
// (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::LoadMissionCoreResources() {
    CString scriptPath;
    zImg::Init();

    if (missionId == 0) {
        missionId = 1;
    }

    raceCheckpointMode = LoadRaceCheckpointMeta();
    scriptPath.Format(
        "support\\initm%d.gw",
        missionId
    );
    g_zInterp_GlobalContext.RunScriptFile(scriptPath);

    zClass::Init();
    zModel::Init();

    if (((const char *)zbdPath)[0] == '\0') {
        if (missionFlags != 0) {
            zbdPath.Format(
                "m%d_zbd.gs",
                missionId
            );
        } else {
            zbdPath.Format(
                "m%d.gs",
                missionId
            );
        }
    }

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x102));
    g_zInterp_GlobalContext.RunScriptFile(zbdPath);

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10e));
    sprintf(
        g_HudSensor_MissionSoundSetName,
        "M%d",
        missionId
    );
    zSndSampleSet_InitByName(g_HudSensor_MissionSoundSetName);

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x103));
    zImage::TexDir_LoadPendingEntries();

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x104));
    worldNode = zClass::FindByTypeAndName(
        13,
        "world1"
    );
    cameraNode = zClass::FindByTypeAndName(
        8,
        "camera1"
    );
    windowNode = zClass::FindByTypeAndName(
        14,
        "window1"
    );
    displayNode = zClass::FindByTypeAndName(
        15,
        "display"
    );

    zClass_Class::gwNodeUpdateAll();
    zClass::ProcessDeferredWork();
    zOpt::RenderSection_SetTargetWindow(windowNode);
    zOpt::DisplaySection_SetTargetDisplay(displayNode);
    zOpt::CameraSection_SetActiveCamera(cameraNode);

    missionLoaded = 1;
    return 1;
}

// Reimplements 0x417a00: HudSensorTracker::InitMissionGameplaySystems
// (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::InitMissionGameplaySystems() {
    missionStat0 = 0;
    missionStat1 = 0;
    primaryGunDispatchCount = 0;
    missionStat3 = 0;
    weaponsFoundMask = 0;
    g_Player_HudCounterValue = 0;
    g_OptCatalog_DamageFeedbackHitCount = 0;
    menuTransitionDelaySec = -1.0f;

    LoadMissionMapAndSfx(missionId);
    mapWorldNode = worldNode;

    zInputCommandCallbackFn objectiveCommandCallback =
        (zInputCommandCallbackFn)(HudSensorTracker::OnObjectiveCommand);
    zInput::BindMap_Current_SetCommandCallback(
        27,
        objectiveCommandCallback
    );
    if (zOpt::GetNetworkEnabled() == 0) {
        zInput::BindMap_Current_SetCommandCallback(
            28,
            objectiveCommandCallback
        );
        zInput::BindMap_Current_SetCommandCallback(
            29,
            objectiveCommandCallback
        );
        zInput::BindMap_Current_SetCommandCallback(
            24,
            objectiveCommandCallback
        );
        zInput::BindMap_Current_SetCommandCallback(
            25,
            objectiveCommandCallback
        );
    }
    zInput::BindMap_Current_SetCommandCallback(
        26,
        objectiveCommandCallback
    );

    Pickup::Init(
        worldNode,
        "pickup.zrd"
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x106));
    zEffect::InitFromPath(
        worldNode,
        cameraNode,
        "effects.zrd"
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x108));
    zEffect::SetWorldNode(worldNode);
    zEffect::SetResourceNode(effectResourceNode);
    zEffect_Anim::LoadAndInstantiate();
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x105));
    zDEClient::LoadConfigResources(worldNode);
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x109));
    zWeapon::LoadOptCatalogFromPath(
        worldNode,
        "weapons.zrd",
        zOpt::GetNetworkEnabled(),
        zWeapon_OptCatalog::LoadKillVerbString
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10a));
    zTurret_System::LoadDefinitionsFromPath(
        worldNode,
        "ai.zrd"
    );
    PickupAirdropSpawnRef::InitGlobalFromCarrierNodeName("vtol2");
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10b));

    HudUiMgr::ActivateHud(
        (const HudUiRect *)zOpt::GetDisplaySection(),
        (const HudUiRect *)zOpt::GetWindowSection()
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10c));
    Player::InitMissionRuntimeFromWorldAndCamera(
        worldNode,
        cameraNode
    );

    if (hasPendingPlayerSave != 0) {
        g_LocalPlayerSaveState->playerState->nanitePanelLevel =
            pendingPlayerSave.savedNanitePanelLevel;
        Player::ApplyMissionSaveData(&pendingPlayerSave.playerSaveData);
        g_PlayerStatusMeterRatio = 1.0f;
        hasPendingPlayerSave = 0;
    }

    Pickup::InitAndLoadPuppySpawns();
    if (zOpt::GetNetworkEnabled() != 0) {
        GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks();
        Net::InitFromZrd();
    }

    HudUiLoadingCheckpoint::AdvanceAndLog("Find mission objectives");
    LoadObjectivesFromZrd("objectives.zrd");
    LoadMissionWeatherFx("Weather.zrd");
    zInput::Keyboard_ResetTransitionState();

    if (zOpt::GetNetworkEnabled() != 0 && GameNet::GetStatusBitAllowMaps() != 0) {
        MapOverlayRefToggle(1);
    }

    zClass_Camera::gwCameraSetFlagBit0(
        cameraNode,
        1
    );
    if (g_zVideo_ActiveRendererPath != 0) {
        zModel_MatlBuffer::ReleaseTextureSurfaces();
    }

    Player::RefreshHudFromState((zUtil_SaveGameState *)g_GameStateOrMapTable);
    return 1;
}

// Reimplements 0x417d40: HudSensorTracker::ShutdownMissionGameplaySystems
// (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::ShutdownMissionGameplaySystems() {
    if (missionLoaded == 0) {
        return 1;
    }

    HudUiLoadingCheckpoint::AdvanceAndLog("Unloading Mission...");
    HudUiMgr::UpdateTargetReticleFromCursor(
        0,
        0,
        0.0f,
        0.0f
    );
    HudUiMgr::DisableHud();
    HudUiLoadingCheckpoint::AdvanceAndLog(0);
    HudUiAuxOverlay::ClearTextLines();

    HudUiLoadingCheckpoint::AdvanceAndLog("Stop All Sounds");
    zSndPlayHandleSnapshot *const soundSnapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
    soundSnapshot->StopAllIfPlaying();
    zClass_Camera::gwCameraSetFlagBit0(
        cameraNode,
        0
    );
    MapShutdownAndReset();

    HudUiLoadingCheckpoint::AdvanceAndLog("Unload Objectives");
    UnloadObjectives();

    HudUiLoadingCheckpoint::AdvanceAndLog("Closing Player");
    GameNet::UnregisterGameplayPacketHandlers();
    Player::ShutdownMissionRuntime();
    PickupAirdropSpawnRef::ShutdownGlobal();
    zTurret_System::FreeAllRuntimes();

    HudUiLoadingCheckpoint::AdvanceAndLog("Closing Weapons");
    OptCatalog::ShutdownCore();

    HudUiLoadingCheckpoint::AdvanceAndLog("Closing Effects");
    zEffect::Reset();

    HudUiLoadingCheckpoint::AdvanceAndLog("Closing Animations");
    zEffect_Anim::Shutdown();
    zDEClient::ShutdownGlobals();
    Pickup::Shutdown();
    zClass_Object3D_ModelRefLerpQueue::Reset();
    zOpt::RenderSection_SetTargetWindow(0);
    zOpt::DisplaySection_SetTargetDisplay(0);
    zOpt::CameraSection_SetActiveCamera(0);

    char loadingMessage[80];
    sprintf(
        loadingMessage,
        "Large Models: %d",
        worldNode->listCountB
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    HudUiLoadingCheckpoint::AdvanceAndLog("Closing Class");
    zClass::ShutdownCore();

    HudUiLoadingCheckpoint::AdvanceAndLog("Closing Models");
    zModel_Display::Shutdown();
    zImage::Shutdown();

    sprintf(
        g_HudSensor_MissionSoundSetName + 16,
        "M%d",
        missionId
    );
    zSndSampleSet_DestroyByName(g_HudSensor_MissionSoundSetName + 16);

    HudUiLoadingCheckpoint::AdvanceAndLog("...Mission Unloaded");
    HudUiLoadingCheckpoint::AdvanceAndLog(0);

    displayNode = 0;
    windowNode = 0;
    cameraNode = 0;
    worldNode = 0;
    missionLoaded = 0;
    ResetMissionState();
    return 1;
}

// Reimplements 0x419050: HudSensorTracker::LoadMissionWeatherFx
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::LoadMissionWeatherFx(
    const char *zrdPath
) {
    zReader::Node *rootNode = zReader::LoadNodeFromPath(
        zrdPath,
        0,
        0
    );
    if (rootNode == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\mission.cpp",
            0x5f6,
            g_HudSensorTracker_ReadFileFailedFmt,
            zrdPath
        );
        return;
    }

    char missionNodeName[0x40];
    sprintf(
        missionNodeName,
        "MISSION%d",
        missionId
    );
    zReader::Node *missionNode = zReader_GetNamedNode(
        rootNode,
        missionNodeName
    );
    if (missionNode != 0) {
        int particleCount = 100;
        zReader::Node *particleNode = zReader_GetNamedNode(
            missionNode,
            "PARTICLES"
        );
        if (particleNode != 0) {
            particleCount = particleNode->value.i32;
        }

        zReader::Node *typeNode = zReader_GetNamedNode(
            missionNode,
            "TYPE"
        );
        if (typeNode != 0) {
            const char *const weatherType = typeNode->value.str;
            if (strcmp(
                weatherType,
                "SNOW"
            ) == 0) {
                HudWeatherFxSnow *const snow =
                    (HudWeatherFxSnow *)(::operator new(sizeof(HudWeatherFxSnow)));
                fxPass3Obj = snow != 0 ? snow->Constructor(particleCount) : 0;
            } else if (strcmp(
                weatherType,
                "RAIN"
            ) == 0) {
                HudWeatherFxRain *const rain =
                    (HudWeatherFxRain *)(::operator new(sizeof(HudWeatherFxRain)));
                fxPass3Obj = rain != 0 ? rain->Constructor(particleCount) : 0;
            }
        }

        if (fxPass3Obj != 0) {
            HudWeatherFx *const weatherFx = (HudWeatherFx *)(fxPass3Obj);

            zReader::Node *colorNode = zReader_GetNamedNode(
                missionNode,
                "COLOR"
            );
            if (colorNode != 0) {
                zReader::Node *const colorFields = colorNode->value.nodes;
                weatherFx->packedColor16 = zVid_PackColorRGB(
                    colorFields[1].value.i32,
                    colorFields[2].value.i32,
                    colorFields[3].value.i32
                );
            }

            zReader::Node *windDirNode = zReader_GetNamedNode(
                missionNode,
                "WIND_DIR"
            );
            if (windDirNode != 0) {
                weatherFx->windDirection = windDirNode->value.f32;
            }

            zReader::Node *windVelNode = zReader_GetNamedNode(
                missionNode,
                "WIND_VEL"
            );
            if (windVelNode != 0) {
                weatherFx->windVelocity = windVelNode->value.f32;
            }

            zReader::Node *gravityNode = zReader_GetNamedNode(
                missionNode,
                "GRAVITY"
            );
            if (gravityNode != 0) {
                weatherFx->gravity = gravityNode->value.f32;
            }

            zReader::Node *alphaGradientNode = zReader_GetNamedNode(
                missionNode,
                "ALPHA_GRADIENT"
            );
            if (alphaGradientNode != 0) {
                zReader::Node *const alphaFields = alphaGradientNode->value.nodes;
                weatherFx->alphaStartScale = alphaFields[1].value.f32;
                weatherFx->alphaEndScale = alphaFields[2].value.f32;
            }

            ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->AddChild(fxPass3Obj);
        }
    }

    zReader::FreeLoadedTree(rootNode);
}

// Reimplements 0x417ee0: HudSensorTracker::UnloadObjectives
int HudSensorTracker::UnloadObjectives() {
    if (zOpt::GetNetworkEnabled() == 0) {
        {
            for (int index = 0; index < objectiveCount; ++index) {
                objectiveSlots[index].Reset();
            }
        }

        currentObjectiveIndex = -1;
        firstIncompleteObjectiveIndex = 0;
        objectiveCount = 0;
        completedObjectiveCount = 0;
    }

    if (objectivesRootNode != 0) {
        zReader::FreeLoadedTree(objectivesRootNode);
    }

    return 1;
}

// Reimplements 0x417f90: HudSensorTracker::LoadObjectivesFromPath
// (D:\Proj\Battlesport\mission.cpp)
int HudSensorTracker::LoadObjectivesFromPath(
    const char *path
) {
    zReader::Node *rootNode = zReader::LoadNodeFromPath(
        path,
        0,
        0
    );
    if (rootNode == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\mission.cpp",
            0x2c7,
            g_HudSensorTracker_ReadFileFailedFmt,
            path
        );
        return 1;
    }

    objectivesRootNode = rootNode;

    char imagesPath[0x40];
    sprintf(
        imagesPath,
        "..\\data\\m%d\\images\\",
        missionId
    );
    zImage_InitMissionResources(imagesPath);

    objectiveReviewDelaySecRaw = FloatToRawSeconds(4.0f);
    objectiveReadTimeSecRaw = FloatToRawSeconds(4.0f);
    objectiveReadSoundDelaySecRaw = FloatToRawSeconds(2.0f);

    zReader::Node *readTimeNode = zReader_GetNamedNode(
        rootNode,
        "READ_TIME"
    );
    if (readTimeNode != 0) {
        objectiveReadTimeSecRaw =
            FloatToRawSeconds((float)(readTimeNode->value.nodes[1].value.i32));
    }

    zReader::Node *reviewDelayNode = zReader_GetNamedNode(
        rootNode,
        "REVIEW_DELAY"
    );
    if (reviewDelayNode != 0) {
        objectiveReviewDelaySecRaw =
            FloatToRawSeconds((float)(reviewDelayNode->value.nodes[1].value.i32));
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        return 0;
    }

    zReader::Node *finalMissionNode = zReader_GetNamedNode(
        rootNode,
        "FINAL_MISSION"
    );
    if (finalMissionNode != 0) {
        finalMissionFlag = finalMissionNode->value.nodes[1].value.i32;
    } else {
        finalMissionFlag = 0;
    }

    int lastObjectiveIndex = 0;
    {
        for (int objectiveNumber = 1;; ++objectiveNumber) {
            char objectiveName[0x20];
            sprintf(
                objectiveName,
                "OBJECTIVE%d",
                objectiveNumber
            );

            zReader::Node *objectiveNode = zReader_GetNamedNode(
                rootNode,
                objectiveName
            );
            if (objectiveNode == 0) {
                break;
            }

            lastObjectiveIndex = objectiveNumber - 1;
            HudSensorObjectiveSlot &slot = objectiveSlots[lastObjectiveIndex];
            zReader::Node *objectiveFields = objectiveNode->value.nodes;

            const char *imagePath = objectiveFields[1].value.str;
            slot.objectiveImage = zImage::TexDir_FindOrCreateByPath(imagePath);
            if (slot.objectiveImage == 0) {
                zError::ReportOld(
                    0x800,
                    "D:\\Proj\\Battlesport\\mission.cpp",
                    0x2ff,
                    "Cannot find objective %d's image file - %s",
                    objectiveNumber,
                    imagePath
                );
                return 1;
            }

            strncpy(
                slot.objectiveTitle,
                zLoc::ResolveMessageKeyOrFallback(objectiveFields[2].value.str),
                0x100
            );
            slot.objectiveTitle[0xff] = '\0';

            strncpy(
                slot.objectiveDesc,
                zLoc::ResolveMessageKeyOrFallback(objectiveFields[3].value.str),
                0x100
            );
            slot.objectiveDesc[0xff] = '\0';

            strncpy(
                slot.objectiveSummary,
                zLoc::ResolveMessageKeyOrFallback(objectiveFields[4].value.str),
                0x100
            );
            slot.objectiveSummary[0xff] = '\0';

            slot.completedFlag = 0;
            if (zReader_GetNamedNode(
                objectiveNode,
                "AUTOPLAY"
            ) != 0) {
                slot.autoplayFlag = 1;
            }

            if (objectiveNumber + 1 == 0xb) {
                zError::ReportOld(
                    0x400,
                    "D:\\Proj\\Battlesport\\mission.cpp",
                    0x2ee,
                    "Mission objectives array overflow; MAX allowable = %d",
                    objectiveNumber
                );
                break;
            }
        }
    }

    currentObjectiveIndex = -1;
    firstIncompleteObjectiveIndex = 0;
    objectiveCount = lastObjectiveIndex + 1;
    completedObjectiveCount = 0;
    return 0;
}

// Reimplements 0x418230: HudSensorTracker::LoadObjectivesFromZrd
// (D:\Proj\Battlesport\mission.cpp)
int HudSensorTracker::LoadObjectivesFromZrd(
    const char *
) {
    zReader::Node *reviewSoundNode = zReader_GetNamedNode(
        objectivesRootNode,
        "REVIEW_SOUND"
    );
    if (reviewSoundNode != 0) {
        objectiveReviewSfx = zSnd::FindSampleByName(reviewSoundNode->value.nodes[1].value.str);
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        return 0;
    }

    char objectiveName[0x20];
    int objectiveNumber = 1;
    sprintf(
        objectiveName,
        "OBJECTIVE%d",
        objectiveNumber
    );

    zReader::Node *objectiveNode = zReader_GetNamedNode(
        objectivesRootNode,
        objectiveName
    );
    while (objectiveNode != 0) {
        HudSensorObjectiveSlot &slot = objectiveSlots[objectiveNumber - 1];

        zReader::Node *activeNode = zReader_GetNamedNode(
            objectiveNode,
            "ACTIVE"
        );
        if (activeNode != 0) {
            slot.activationNode = ResolveObjectiveNodePath(
                activeNode,
                objectiveNumber - 1,
                "Cannot find Objective %d's activation node: %s",
                0x355
            );
            slot.inactivationNode = 0;
        } else {
            zReader::Node *inactiveNode = zReader_GetNamedNode(
                objectiveNode,
                "INACTIVE"
            );
            if (inactiveNode != 0) {
                slot.activationNode = 0;
                slot.inactivationNode = ResolveObjectiveNodePath(
                    inactiveNode,
                    objectiveNumber - 1,
                    "Cannot find Objective %d's inactivation node: %s",
                    0x36b
                );
            } else {
                slot.activationNode = 0;
                slot.inactivationNode = 0;
            }
        }

        slot.objectiveReadFlag = 0;
        zReader::Node *readSoundNode = zReader_GetNamedNode(
            objectiveNode,
            "READ_SOUND"
        );
        if (readSoundNode != 0) {
            zReader::Node *const readSoundFields = readSoundNode->value.nodes;
            slot.readSoundSample = zSnd::FindSampleByName(readSoundFields[1].value.str);
            if (slot.readSoundSample != 0) {
                slot.readSoundSample->SetPlaybackEventHandler(OnObjectiveReadSoundEvent);
            }
            if (readSoundFields[0].value.i32 > 2) {
                objectiveReadSoundDelaySecRaw = readSoundFields[2].value.i32;
            }
        }

        ++objectiveNumber;
        sprintf(
            objectiveName,
            "OBJECTIVE%d",
            objectiveNumber
        );
        objectiveNode = zReader_GetNamedNode(
            objectivesRootNode,
            objectiveName
        );
    }

    zReader::Node *objectiveSoundNode = zReader_GetNamedNode(
        objectivesRootNode,
        "OBJECTIVE_SOUND"
    );
    if (objectiveSoundNode != 0) {
        objectiveCompleteSfx = zSnd::FindSampleByName(objectiveSoundNode->value.nodes[1].value.str);
    }

    objectiveIncomingSfx = zSnd::FindSampleByName("snd_incoming");
    firstIncompleteObjectiveIndex = FindAndHighlightFirstIncompleteObjective();
    return 0;
}

// Reimplements 0x4193c0: HudSensorTracker::LoadRaceCheckpointMeta
// (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::LoadRaceCheckpointMeta() {
    CString raceZrdrSearchPath;
    raceZrdrSearchPath.Format(
        "..\\data\\m%d\\zrdr",
        missionId
    );

    int raceCheckpointMode = 0;
    zReader::Node *raceRoot = zReader::LoadNodeFromPath(
        "race.zrd",
        raceZrdrSearchPath,
        0
    );
    if (raceRoot != 0) {
        zReader::Node *cpCountNode = zReader_GetNamedNode(
            raceRoot,
            "cp_count"
        );
        if (cpCountNode != 0) {
            raceCheckpointMode = 1;
            runtimeTimerSecRaw = FloatToRawSeconds(20.0f);
            checkpointCount = cpCountNode->value.nodes[1].value.i32;
        }

        zReader::FreeLoadedTree(raceRoot);
    }

    return raceCheckpointMode;
}

// Reimplements 0x4192d0: HudSensorTracker::RunStartAnimsFromZrd
// (D:\Proj\Battlesport\mission.cpp)
void HudSensorTracker::RunStartAnimsFromZrd(
    const char *zrdPath,
    const char *namedNodeName
) {
    if (zOpt::GetNetworkEnabled() != 0) {
        return;
    }

    zReader::Node *rootNode = zReader::LoadNodeFromPath(
        zrdPath,
        0,
        0
    );
    if (rootNode == 0) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\mission.cpp",
            0x646,
            g_HudSensorTracker_ReadFileFailedFmt,
            zrdPath
        );
        return;
    }

    zReader::Node *startAnimList = zReader_GetNamedNode(
        rootNode,
        namedNodeName
    );
    if (startAnimList != 0) {
        zReader::Node *startAnimFields = startAnimList->value.nodes;
        const int startAnimCount = startAnimFields[0].value.i32 - 1;
        {
            for (int startAnimIndex = 0; startAnimIndex < startAnimCount; ++startAnimIndex) {
                zReader::Node *startAnimEntry = startAnimFields[startAnimIndex + 1].value.nodes;
                zEffectAnimEntry *effectAnim =
                    zEffectAnim::FindEntryByName(startAnimEntry[1].value.str);
                if (effectAnim != 0) {
                    zEffectAnim::ResetActivationPrereqCount(effectAnim);
                    zEffectAnim::SetVelocity_Thunk(
                        effectAnim,
                        0,
                        0.0f,
                        0.0f,
                        0.0f
                    );
                }
            }
        }
    }

    zReader::FreeLoadedTree(rootNode);
}

// Reimplements 0x419010: HudSensorTracker::QueueMissionFmvStateForMissionId
// (D:\Proj\GameZRecoil\recoilapp.cpp)
int HudSensorTracker::QueueMissionFmvStateForMissionId(
    int missionId
) {
    g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 0;
    g_RecoilApp.m_missionFmvState.m_missionId = missionId;
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_missionFmvState,
        0
    );
    return 1;
}

// Reimplements 0x418fb0: HudSensorTracker::SaveAndQueueMissionState
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::SaveAndQueueMissionState() {
    if (finalMissionFlag != 0) {
        g_RecoilApp_QuitAfterCredits = 1;
        return;
    }

    zUtil_PlayerStateStorage *const playerState = g_LocalPlayerSaveState->playerState;
    Player::BuildMissionSaveData(&pendingPlayerSave.playerSaveData);
    pendingPlayerSave.savedNanitePanelLevel = playerState->nanitePanelLevel;
    hasPendingPlayerSave = 1;
    QueueMissionFmvStateForMissionId(missionId + 1);
}

/**
 * Reimplements 0x4186f0: HudSensorTracker::GetObjectiveBriefingStringsAndImageRef
 * Source: D:\Proj\Battlesport\map.cpp
 * Purpose: Return the briefing text buffers and image pointer for one objective slot.
 */
int HudSensorTracker::GetObjectiveBriefingStringsAndImageRef(
    int objectiveIndex,
    char **outSummary,
    char **outDesc,
    zVidImagePartial **outImageRef
) {
    HudSensorObjectiveSlot &slot = objectiveSlots[objectiveIndex];
    *outSummary = slot.objectiveTitle;
    *outDesc = slot.objectiveDesc;
    *outImageRef = slot.objectiveImage;
    return 1;
}

// Reimplements 0x4172c0: HudSensorTracker::SetObjectiveMarkerEnabledAndColor
// (HudSensorTracker.cpp)
int HudSensorTracker::SetObjectiveMarkerEnabledAndColor(
    int objectiveIndex,
    int enabled,
    const unsigned char *colorRgb24
) {
    HudSensorMapNode *mapNode = mapNodeListHead;
    while (mapNode != 0) {
        if (mapNode->objectiveIndex == objectiveIndex) {
            mapNode->SetColorRgb(colorRgb24);
            mapNode->SetEnabled(enabled);
        }

        mapNode = mapNode->next;
    }

    return 1;
}

// Reimplements 0x417300: HudSensorTracker::SetObjectiveMarkerColorBlink
// (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::SetObjectiveMarkerColorBlink(
    int objectiveIndex,
    const unsigned char *colorRgb24
) {
    HudSensorMapNode *mapNode = mapNodeListHead;
    while (mapNode != 0) {
        if (mapNode->objectiveIndex == objectiveIndex) {
            mapNode->SetColorRgb(colorRgb24);
            const unsigned int packedColor = (unsigned int)(mapNode->packedColor565Pair);
            mapNode->packedColor565Pair =
                (int)((packedColor << 16) | ((packedColor >> 16) & 0xffff));
        }

        mapNode = mapNode->next;
    }

    return 1;
}

// Reimplements 0x418c30: HudSensorTracker::FindAndHighlightFirstIncompleteObjective
// (HudSensorTracker.cpp)
int HudSensorTracker::FindAndHighlightFirstIncompleteObjective() {
    int objectiveIndex = 0;
    while (objectiveIndex < objectiveCount && objectiveSlots[objectiveIndex].completedFlag != 0) {
        ++objectiveIndex;
    }

    if (objectiveIndex < objectiveCount) {
        SetObjectiveMarkerEnabledAndColor(
            objectiveIndex,
            1,
            g_HudSensorTracker_ObjectiveMarkerColorBlueRgb24
        );
    }

    return objectiveIndex;
}

/**
 * Reimplements 0x419470: HudSensorTracker::SetRuntimeTimerSecAndGoalValue
 * Source: D:\Proj\Battlesport\HudSensor.cpp
 * Purpose: Store the runtime timer seconds payload and mission goal value.
 */
void HudSensorTracker::SetRuntimeTimerSecAndGoalValue(
    int timerSecRaw,
    int goalValue
) {
    runtimeGoalValue = goalValue;
    runtimeTimerSecRaw = timerSecRaw;
}

// Reimplements 0x418620: HudSensorTracker::SetObjectiveReviewVisible
// (D:\Proj\Battlesport\hud.cpp)
int HudSensorTracker::SetObjectiveReviewVisible(
    int visible
) {
    objectiveFlowState = 0x65;
    if (visible != 0) {
        objectiveUiMode = 1;
        if (firstIncompleteObjectiveIndex < objectiveCount) {
            HudSensorObjectiveSlot &slot = objectiveSlots[firstIncompleteObjectiveIndex];
            HudUiMgrObjective::Show(
                slot.objectiveImage,
                slot.objectiveTitle,
                slot.objectiveDesc,
                0.0f
            );
        } else {
            HudSensorObjectiveSlot &slot = objectiveSlots[currentObjectiveIndex];
            HudUiMgrObjective::Show(
                slot.objectiveImage,
                zLoc::GetMessageString(0xf0f),
                0,
                0.0f
            );
        }

        return 1;
    }

    objectiveUiMode = 0;
    HudUiMgrObjective::Begin();
    zSnd::SetGlobalVolumeScale(g_HudSensorTracker.hudScale);
    zSnd::SetFlag10PlaybackEnabled(1);
    return 1;
}

// Reimplements 0x419380: HudSensorTracker::OnObjectiveReadSoundEvent
// (D:\Proj\Battlesport\hud.cpp)
void __fastcall HudSensorTracker::OnObjectiveReadSoundEvent(
    int eventCode
) {
    if (eventCode == 0) {
        g_HudSensorTracker.SetObjectiveReviewVisible(1);
    } else if (eventCode == 1) {
        g_HudSensorTracker.SetObjectiveReviewVisible(0);
    } else if (eventCode == 2) {
        zSnd::SetGlobalVolumeScale(g_HudSensorTracker.hudScale);
        zSnd::SetFlag10PlaybackEnabled(1);
    }
}

// Reimplements 0x4184e0: HudSensorTracker::AdvanceObjectiveState
// (D:\Proj\Battlesport\hud.cpp)
void HudSensorTracker::AdvanceObjectiveState() {
    const int flowState = objectiveFlowState;
    if (flowState != 0x6b && flowState != 0x67 && flowState != 0x64) {
        if (objectiveUiMode != 1) {
            objectiveCompleteSfx->PlayA3DSimple(1.0f);
            SetObjectiveReviewVisible(1);
            return;
        }

        if (currentObjectiveReadSound != 0) {
            currentObjectiveReadSound->StopActiveVoicesIfPlaying();
        }
        objectiveCompleteSfx->PlayA3DSimple(1.0f);
        SetObjectiveReviewVisible(0);
        return;
    }

    HudUiMgrObjective::SetVisibleAndResetMeterFill(0);
    if (missionId == 1 && firstIncompleteObjectiveIndex == 0) {
        HudUi::PlayPowerupSfx(0);
    }

    HudSensorObjectiveSlot &firstIncompleteSlot = objectiveSlots[firstIncompleteObjectiveIndex];
    if (firstIncompleteObjectiveIndex == currentObjectiveIndex + 1) {
        SetObjectivePanelVisible(1);
        currentObjectiveReadSound = firstIncompleteSlot.readSoundSample;
        currentObjectiveReadSound->PlayA3DSimple(1.0f);
        objectiveFlowState = 0x68;
        objectiveFlowDeadlineSecRaw = FloatToRawSeconds(
            RawSecondsToFloat(objectiveReadTimeSecRaw) + g_Time_UnscaledAccumulatedTimeSec
        );
    } else {
        currentObjectiveReadSound = firstIncompleteSlot.readSoundSample;
        currentObjectiveReadSound->PlayDirectSound(
            0,
            1.0f,
            0x3e7
        );
        objectiveFlowState = 0x69;
    }

    hudScale = zSnd::MulGlobalVolumeScaleAndGetPrev(0.600000024f);
    zSnd::SetFlag10PlaybackEnabled(0);
}

// Reimplements 0x417ca0: HudSensorTracker::OnObjectiveCommand
// (D:\Proj\Battlesport\map.cpp)
void __fastcall HudSensorTracker::OnObjectiveCommand(
    int commandId
) {
    switch (commandId) {
    case 0x18:
        if (g_HudSensorTracker_ObjectiveCommandLocked == 0) {
            g_HudSensorTracker.AdvanceObjectiveState();
        }
        break;

    case 0x19:
        if (g_HudSensorTracker_ObjectiveCommandLocked == 0) {
            g_HudSensorTracker.Command_ToggleObjectivePanel();
        }
        break;

    case 0x1a:
        if (g_HudSensorTracker_ObjectiveCommandLocked == 0) {
            g_HudSensorTracker.Command_ShowObjectivePickupInfo();
        }
        break;

    case 0x1b:
        if (zOpt::GetNetworkEnabled() == 0 || GameNet::GetStatusBitAllowMaps() != 0) {
            g_HudSensorTracker.MapOverlayRefToggle(
                g_HudSensorTracker.mapScaleLerpActive == 0 ? 1 : 0
            );
        }
        break;

    case 0x1c:
        g_HudSensorTracker.MapZoomIn();
        break;

    case 0x1d:
        g_HudSensorTracker.MapZoomOut();
        break;

    default:
        break;
    }
}

// Reimplements 0x418c70: HudSensorTracker::ResetHudForMissionStart
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::ResetHudForMissionStart() {
    objectiveMeterSeconds = 0.0f;
    HudUiMgrObjective::SetVisibleAndResetMeterFill(0);

    if (zOpt::GetNetworkEnabled() != 0) {
        HudUiMgr::DisableHud();
        if (zOpt::GetHudVisibilityOption() != 0) {
            HudUiMgr::ApplyHudModeSwitch(zOpt::GetHudTypeForCurrentHwMode());
            HudUiMgr::EnableHud();
        }

        zUtil_PlayerStateStorage *const playerState =
            (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
        HudUi::ShowTopMessageLine(
            playerState->activeAltGunController->optCatalogEntry->description,
            5.0f
        );
        HudUiTimerPanel::SetRunning(1);
        HudUiMgr::TriggerCurrentLayoutOnActivated();
        HudUiMgr::UpdateTargetReticleFromCursor(
            1,
            0,
            0.5f,
            0.5f
        );
        OptCatalog_SetDamageMaskEnabled(0);
        pendingPlayerSave.skipTimerResetOnStart = 0;
        return;
    }

    if (pendingPlayerSave.skipTimerResetOnStart == 0) {
        HudUiTimerPanel::SetElapsedSeconds(0.0f);
    }

    const float readSoundDelaySec = RawSecondsToFloat(objectiveReadSoundDelaySecRaw);
    objectiveFlowState = 0x64;
    objectiveUiMode = 0;
    currentObjectiveReadSound = 0;
    pendingPlayerSave.skipTimerResetOnStart = 0;
    objectiveFlowDeadlineSecRaw =
        FloatToRawSeconds(readSoundDelaySec + g_Time_UnscaledAccumulatedTimeSec);
}

// Reimplements 0x418730: HudSensorTracker::Command_ToggleObjectivePanel
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::Command_ToggleObjectivePanel() {
    objectiveReviewSfx->PlayA3DSimple(1.0f);
    SetObjectivePanelVisible(objectiveUiMode != 2 ? 1 : 0);
}

// Reimplements 0x418760: HudSensorTracker::SetObjectivePanelVisible
// (D:\Proj\Battlesport\hud.cpp)
void HudSensorTracker::SetObjectivePanelVisible(
    int visible
) {
    if (visible == 0) {
        objectiveUiMode = 0;
        HudUiMgrObjective::Begin();
        return;
    }

    objectiveUiMode = 2;

    float damageRatio = 1.0f;
    if (primaryGunDispatchCount > 0) {
        damageRatio =
            (float)(g_OptCatalog_DamageFeedbackHitCount) / (float)(primaryGunDispatchCount);
    }
    const int damagePercent = (int)(damageRatio * 100.0f);

    char objectiveLine[0x80];
    zLoc::FormatMessage(
        objectiveLine,
        0x40,
        0x116,
        completedObjectiveCount,
        objectiveCount,
        damagePercent
    );

    int cappedStat0 = missionStat0;
    if (cappedStat0 > missionStat1) {
        cappedStat0 = missionStat1;
    }

    char statLine[0x80];
    zLoc::FormatMessage(
        statLine,
        0x40,
        0x117,
        cappedStat0,
        missionStat1,
        missionStat3,
        weaponsFoundMask
    );

    const int elapsedSeconds = (int)(objectiveMeterSeconds);
    char timeLine[0x80];
    zLoc::FormatMessage(
        timeLine,
        0x40,
        0x118,
        elapsedSeconds / 60,
        elapsedSeconds % 60
    );

    sprintf(
        objectiveSummaryText,
        "%s\n%s\n%s",
        objectiveLine,
        statLine,
        timeLine
    );

    if (currentObjectiveIndex < 0) {
        HudSensorObjectiveSlot &firstSlot = objectiveSlots[0];
        HudUiMgrObjective::Show(
            firstSlot.objectiveImage,
            firstSlot.objectiveTitle,
            objectiveSummaryText,
            0.0f
        );
        return;
    }

    HudSensorObjectiveSlot &slot = objectiveSlots[currentObjectiveIndex];
    HudUiMgrObjective::Show(
        slot.objectiveImage,
        slot.objectiveSummary,
        objectiveSummaryText,
        0.0f
    );
}

// Reimplements 0x4188f0: HudSensorTracker::Command_ShowObjectivePickupInfo
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::Command_ShowObjectivePickupInfo() {
    objectiveReviewSfx->PlayA3DSimple(1.0f);

    const int visible = (objectiveUiMode == 3 || objectiveUiMode == 4) ? 0 : 1;
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
    ShowObjectivePickupInfo(
        visible,
        0,
        playerState->activeAltGunController->optCatalogEntry
    );
}

// Reimplements 0x418940: HudSensorTracker::ShowObjectivePickupInfo
// (D:\Proj\Battlesport\map.cpp)
void HudSensorTracker::ShowObjectivePickupInfo(
    int visible,
    int startAutoAdvance,
    OptCatalogEntryDef *optEntry
) {
    if (visible == 0) {
        objectiveUiMode = 0;
        HudUiMgrObjective::Begin();
        return;
    }

    char featureText[0x40];
    strcpy(
        featureText,
        "Features:"
    );

    const unsigned int flags = optEntry->flags;
    if ((flags & 0x00080000) != 0) {
        AppendPickupFeature(
            featureText,
            " Remote"
        );
    }
    if ((flags & 0x00200000) != 0) {
        AppendPickupFeature(
            featureText,
            " Thermal"
        );
    }
    if ((flags & 0x00010000) != 0) {
        AppendPickupFeature(
            featureText,
            " Multi"
        );
    }
    if ((flags & 0x00100000) != 0) {
        AppendPickupFeature(
            featureText,
            " Tether"
        );
    } else if ((flags & 0x00004000) != 0) {
        AppendPickupFeature(
            featureText,
            " Lock On"
        );
    }
    if ((flags & 0x00000002) != 0) {
        AppendPickupFeature(
            featureText,
            " Beam"
        );
    }
    if ((flags & 0x00002000) != 0) {
        AppendPickupFeature(
            featureText,
            " Mine"
        );
    }

    if (featureText[0x0c] == '\0') {
        strcpy(
            featureText,
            "\n"
        );
    }

    const int fireRatePerMinute = (int)(60.0f / optEntry->fireRateInterval + 0.5f);
    const int maxRange = (int)(optEntry->range);

    char weaponStatsText[0x200];
    if (optEntry->impactProximity > 0.0f) {
        sprintf(
            weaponStatsText,
            "Fire Rate: %d rds/min   Max. Range: %d m\n"
            "Damage Power: %.1f      Damage Proximity: %d m\n%s",
            fireRatePerMinute,
            maxRange,
            (double)(optEntry->damage),
            (int)(optEntry->impactProximity),
            featureText
        );
    } else {
        sprintf(
            weaponStatsText,
            "Fire Rate: %d rds/min   Max. Range: %d m\n"
            "Damage Power: %.1f\n%s",
            fireRatePerMinute,
            maxRange,
            (double)(optEntry->damage),
            featureText
        );
    }

    HudUiMgrObjective::Show(
        Pickup::FindOptMetaImageByOptEntry(optEntry),
        optEntry->description,
        weaponStatsText,
        0.0f
    );

    if (startAutoAdvance != 0) {
        objectiveUiMode = 4;
        objectiveFlowDeadlineSecRaw = FloatToRawSeconds(
            RawSecondsToFloat(objectiveReadTimeSecRaw) + g_Time_UnscaledAccumulatedTimeSec
        );
        return;
    }

    objectiveUiMode = 3;
}

// Reimplements 0x418d40: HudSensorTracker::UpdateObjectiveFlow
// (D:\Proj\Battlesport\map.cpp)
int HudSensorTracker::UpdateObjectiveFlow() {
    objectiveMeterSeconds = HudUiTimerPanel::GetSeconds();

    if (zOpt::GetNetworkEnabled() == 0) {
        firstIncompleteObjectiveIndex = FindAndHighlightFirstIncompleteObjective();

        if (menuTransitionDelaySec > 0.0f &&
            RawSecondsToFloat(objectiveReadTimeSecRaw) + menuTransitionDelaySec <=
                g_Time_AccumulatedTimeSec) {
            zUtil_PlayerStateStorage *const playerState =
                (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
            if (playerState->lifecycleState == 4) {
                RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_INGAME);
            }

            menuTransitionDelaySec = -1.0f;
        }

        {
            for (int objectiveIndex = 0; objectiveIndex < objectiveCount; ++objectiveIndex) {
                HudSensorObjectiveSlot &slot = objectiveSlots[objectiveIndex];
                if (slot.completedFlag != 0) {
                    continue;
                }

                int completed = 0;
                if (slot.activationNode != 0 && (slot.activationNode->flags & 4) != 0) {
                    completed = 1;
                } else if (slot.inactivationNode != 0 && (slot.inactivationNode->flags & 4) == 0) {
                    completed = 1;
                }

                if (completed == 0) {
                    continue;
                }

                slot.completedFlag = 1;
                ++completedObjectiveCount;
                objectiveFlowState = 0x67;
                currentObjectiveIndex = objectiveIndex;
                objectiveFlowDeadlineSecRaw = FloatToRawSeconds(
                    RawSecondsToFloat(objectiveReviewDelaySecRaw) +
                    g_Time_UnscaledAccumulatedTimeSec
                );
                SetObjectiveMarkerEnabledAndColor(
                    firstIncompleteObjectiveIndex,
                    0,
                    0
                );
                SetObjectiveMarkerColorBlink(
                    firstIncompleteObjectiveIndex,
                    g_HudSensorTracker_ObjectiveBlinkColorRedRgb24
                );

                if (finalMissionFlag != 0 && completedObjectiveCount == 5) {
                    g_HudSensorTracker_ObjectiveCommandLocked = 1;
                    Player::AiFinalizeMode2State1ForAllPlayers();
                    zTurret_System::DisableTickCallback();
                    HudUiMgrObjective::SetVisibleAndResetMeterFill(0);
                    SetObjectivePanelVisible(1);
                    objectiveFlowState = 0x68;
                    objectiveFlowDeadlineSecRaw =
                        FloatToRawSeconds(g_Time_UnscaledAccumulatedTimeSec + 60.0f);
                }

                break;
            }
        }

        switch (objectiveFlowState) {
        case 0x64:
        case 0x67:
            if (g_Time_UnscaledAccumulatedTimeSec >=
                RawSecondsToFloat(objectiveFlowDeadlineSecRaw)) {
                objectiveIncomingSfx->PlayA3DSimple(1.0f);
                HudUiMgrObjective::SetVisibleAndResetMeterFill(1);
                objectiveFlowState = 0x6b;
            }
            break;

        case 0x68:
            if (g_Time_UnscaledAccumulatedTimeSec >=
                RawSecondsToFloat(objectiveFlowDeadlineSecRaw)) {
                SetObjectivePanelVisible(0);
                objectiveFlowState = 0x69;
            }
            break;

        case 0x6b:
            if (objectiveSlots[firstIncompleteObjectiveIndex].autoplayFlag != 0) {
                AdvanceObjectiveState();
            }
            break;
        }
    }

    if (objectiveUiMode == 4 &&
        g_Time_UnscaledAccumulatedTimeSec >= RawSecondsToFloat(objectiveFlowDeadlineSecRaw)) {
        zUtil_PlayerStateStorage *const playerState =
            (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
        ShowObjectivePickupInfo(
            0,
            1,
            playerState->activeAltGunController->optCatalogEntry
        );
    }

    return 1;
}

// Reimplements 0x417680: HudSensorTracker::ZarMission_SaveCallback
int __fastcall HudSensorTracker::ZarMission_SaveCallback(
    zZbdSectionCallbackCtx *writer,
    HudSensorTracker *self
) {
    return self->WriteMissionDataSection(writer);
}

// Reimplements 0x417690: HudSensorTracker::ZarMission_RestoreCallback
// (D:\Proj\Battlesport\map.cpp)
int __fastcall HudSensorTracker::ZarMission_RestoreCallback(
    void *reader,
    const char *token,
    const void *missionData,
    unsigned int dataSize,
    HudSensorTracker *self
) {
    self->ApplyMissionDataAndReload(
        reader,
        token,
        missionData,
        dataSize
    );
    return 1;
}

// Reimplements 0x4176b0: HudSensorTracker::ZarMissionLate_SaveCallback
void __fastcall HudSensorTracker::ZarMissionLate_SaveCallback(
    zZbdSectionCallbackCtx *writer,
    HudSensorTracker *
) {
    unsigned int lateMissionData = 1;
    zUtil_ZAR::WriteSectionBlob(
        writer,
        "LateMissionData",
        &lateMissionData,
        sizeof(lateMissionData)
    );
}

// Reimplements 0x4176d0: HudSensorTracker::ZarMissionLate_RestoreCallback
// (D:\Proj\Battlesport\map.cpp)
void __fastcall HudSensorTracker::ZarMissionLate_RestoreCallback(
    void *,
    const char *,
    const void *,
    unsigned int,
    HudSensorTracker *self
) {
    self->RunStartAnimsFromZrd(
        "StartAnims.zrd",
        "LOAD_GAME_START"
    );
}
