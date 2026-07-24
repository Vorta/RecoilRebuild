// Checked-in focused native smoke translation unit, formerly extracted from zgame_tests.cpp.
// Emits only the zClass camera XZ hull smoke needed by functional manifests.

#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zUtil/zutil.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "opt_catalog.h"
#include "zclass.h"
#include "zclip_alt.h"
#include "zdi.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <io.h>
#include <limits>

extern "C" int zclass_camera_convex_hull_xz_smoke() {
    auto nearFloat = [](float lhs, float rhs) { return std::fabs(lhs - rhs) <= 0.00001f; };
    auto nearPoint = [](const zVec3 &point, float x, float z) {
        return std::fabs(point.x - x) <= 0.00001f && std::fabs(point.z - z) <= 0.00001f;
    };

    zVec3 origin{0.0f, 0.0f, 0.0f};
    zVec3 east{10.0f, 0.0f, 0.0f};
    zVec3 north{0.0f, 0.0f, -10.0f};
    zVec3 west{-10.0f, 0.0f, 0.0f};
    zVec3 south{0.0f, 0.0f, 10.0f};
    zVec3 northEast{10.0f, 0.0f, -10.0f};

    if (!nearFloat(zClass_Camera::FastAngleXZ(&origin, &origin), 0.0f) ||
        !nearFloat(zClass_Camera::FastAngleXZ(&origin, &east), 0.0f) ||
        !nearFloat(zClass_Camera::FastAngleXZ(&origin, &north), 1.57079601f) ||
        !nearFloat(zClass_Camera::FastAngleXZ(&origin, &west), 3.14159203f) ||
        !nearFloat(zClass_Camera::FastAngleXZ(&origin, &south), 4.71238804f) ||
        !nearFloat(zClass_Camera::FastAngleXZ(&origin, &northEast), 0.785398006f)) {
        return 1;
    }

    zVec3 points[6] = {{0.0f, 0.0f, 0.0f},
                       {2.0f, 0.0f, 0.0f},
                       {2.0f, 0.0f, 2.0f},
                       {0.0f, 0.0f, 2.0f},
                       {1.0f, 0.0f, 1.0f},
                       {123.0f, 0.0f, 123.0f}};

    const int hullCount = zClass_Camera::FindConvexHullXZ(points, 5);
    if (hullCount != 3) {
        return 2;
    }

    if (!nearPoint(points[0], 2.0f, 2.0f) || !nearPoint(points[1], 2.0f, 0.0f) ||
        !nearPoint(points[2], 0.0f, 2.0f)) {
        return 3;
    }

    return 0;
}
