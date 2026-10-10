#pragma once
#ifndef GAMEZRECOIL_INCLUDE_ZCLIPRECT_H
#define GAMEZRECOIL_INCLUDE_ZCLIPRECT_H

#include "recoil/recoil_callconv.h"

#include "recoil/recoil_types.h"
#include <stddef.h>

typedef struct zClipRectPartial {
    int flags;
    float xMin;
    float yMin;
    float zMin;
    float xMax;
    float yMax;
    float zMax;
    float xMaxAlt;
    float yMaxAlt;
} zClipRectPartial;

typedef struct zClipVert {
    float x;
    float y;
    float z;
} zClipVert;

typedef struct zClipUV {
    float u;
    float v;
} zClipUV;

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

#ifdef __cplusplus
namespace zClipRect {
extern "C" {
#endif
/*
 * Source-model evidence: five Z clippers, reviewed 2026-10-10.
 *
 * vertexCount is reconstructed as pointer to volatile int for retail
 * 0x47a200, 0x47a4e0, 0x47aa80, 0x47af60 and 0x47e900.
 * This declaration is inferred from retail accesses, not recovered
 * original header text.
 *
 * Each function reads the count separately at evaluated guards in the
 * store-free far/near pre-scans, reads it again for the early-return
 * comparison, and uses distinct reads for prevIndex and the initial
 * main-loop guard. Same-body VC5SP3 C1 controls with int* cache or
 * combine those reads. Retail 0x47dfb0 provides the XY entry control.
 *
 * Scope: these five prototypes and definitions only. The six XY
 * interfaces and caller-local declarations remain unchanged.
 * No asynchronous-update or synchronization contract is inferred.
 *
 * Evidence: retained zclip-volatile-count review packet,
 * retail_count_access_sites.txt sections A-D, complete retail listings,
 * same-body plain-int controls, and compile_profile.txt (r12168).
 */
int __fastcall ClipPolyNearZ(zClipRectPartial* clipRect, volatile int* vertexCount);
int __fastcall ClipPolyNearZ_WithAttr0(zClipRectPartial* clipRect, volatile int* vertexCount);
int __fastcall ClipPolyZRange_NoUV(zClipRectPartial* clipRect, volatile int* vertexCount);
int __fastcall ClipPolyZRange_NoUV_WithAttribs(zClipRectPartial* clipRect, volatile int* vertexCount);
int __fastcall ClipPolyZRange_WithAttr012(zClipRectPartial* clipRect, volatile int* vertexCount);
int __fastcall ClipPoly_NoUV_Alt(zClipRectPartial* clipRect, int* vertexCount);
int __fastcall ClipPoly_NoUV(zClipRectPartial* clipRect, int* vertexCount);
int __fastcall ClipPoly(zClipRectPartial* clipRect, int* vertexCount);
int __fastcall ClipPoly_WithAttr012(zClipRectPartial* clipRect, int* vertexCount);
int __fastcall ClipPoly_NoUV_WithAttr0_Alt(zClipRectPartial* clipRect, int* vertexCount);
int __fastcall ClipPoly_NoUV_WithAttr012_Alt(zClipRectPartial* clipRect, int* vertexCount);
int __fastcall TrivialRejectPolyXY(zClipRectPartial* clipRect, int vertexCount);
#ifdef __cplusplus
}
} // namespace zClipRect
#endif

#endif // GAMEZRECOIL_INCLUDE_ZCLIPRECT_H
