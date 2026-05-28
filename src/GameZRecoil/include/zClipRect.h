#pragma once
#ifndef GAMEZRECOIL_INCLUDE_ZCLIPRECT_H
#define GAMEZRECOIL_INCLUDE_ZCLIPRECT_H

#include "recoil/recoil_callconv.h"

#include <stddef.h>
#include "recoil/recoil_types.h"

struct zClipRectPartial {
    int flags;
    float xMin;
    float yMin;
    float zMin;
    float xMax;
    float yMax;
    float zMax;
    float xMaxAlt;
    float yMaxAlt;
};

struct zClipVert {
    float x;
    float y;
    float z;
};

struct zClipUV {
    float u;
    float v;
};

RECOIL_STATIC_ASSERT(offsetof(zClipRectPartial, flags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zClipRectPartial, xMin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zClipRectPartial, yMin) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zClipRectPartial, zMin) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zClipRectPartial, xMax) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zClipRectPartial, yMax) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zClipRectPartial, zMax) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zClipRectPartial, xMaxAlt) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zClipRectPartial, yMaxAlt) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(zClipRectPartial) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zClipVert) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zClipUV) == 0x08);

extern zClipVert g_Clip_PolyVerts[0x40];
extern zClipVert g_Clip_PolyVertsScratch[0x40];
extern zClipUV *g_Clip_PolyUvs;
extern zClipRectPartial gClipRect_Primary;

namespace zClipRect {
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPolyNearZ(zClipRectPartial *clipRect,
                                                           int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPolyNearZ_WithAttr0(zClipRectPartial *clipRect,
                                                                     int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPolyZRange_NoUV(zClipRectPartial *clipRect,
                                                                 int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL
ClipPolyZRange_NoUV_WithAttribs(zClipRectPartial *clipRect, int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPolyZRange_WithAttr012(zClipRectPartial *clipRect,
                                                                        int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPoly_NoUV_Alt(zClipRectPartial *clipRect,
                                                               int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPoly_NoUV(zClipRectPartial *clipRect,
                                                           int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPoly(zClipRectPartial *clipRect,
                                                      int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPoly_WithAttr012(zClipRectPartial *clipRect,
                                                                  int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipPoly_NoUV_WithAttr0_Alt(zClipRectPartial *clipRect,
                                                                         int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL
ClipPoly_NoUV_WithAttr012_Alt(zClipRectPartial *clipRect, int *vertexCount);
RECOIL_NOINLINE int RECOIL_FASTCALL TrivialRejectPolyXY(zClipRectPartial *clipRect,
                                                                 int vertexCount);
} // namespace zClipRect

#endif // GAMEZRECOIL_INCLUDE_ZCLIPRECT_H
