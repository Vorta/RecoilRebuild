#include "cls_api.h"
#include "zclass.h"

#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int __fastcall BindWorldNode(CZNodePartial* worldNode);
/* zRender and zVideo entry points; zrndr.h and zvid.h declare them for the C++ units only. */
int __cdecl zRndrLensFlareGetQueuedSampleCount(void);
void __fastcall zRndrLensFlareDrawQueuedSamples16AndBuildVisibleList(int startIndex);
int __fastcall zRndrLensFlareBuildVisibleSampleListFromQueue(int startIndex);
void __fastcall zRndrLensFlareDrawVisibleSample(int sampleIndex);
void __cdecl zRndrLensFlareDrawVisibleSamples(void);
void __fastcall zRndrSpanOcclusionFilterSampleList(int visibleSampleIndex, zVec3* outPoint);
void __cdecl zRndrFlushTransparentQueue(void);
void __cdecl zRndrFlushOverwriteQueue(void);
void __fastcall zRndrOverlayRectFlushSw(void);
void __cdecl SpanOcclusionResetFrame(void);
void __fastcall SpanOcclusionAddPolygon(const zVec3* vertices, int vertCount);
void __cdecl SpanOcclusionBuildColumnHeadTable(void);
void __fastcall FxPass3UpdateLocal(float deltaTime);
void __fastcall CallClearZBufferRect(zVidRect32* rect);
int __cdecl SceneEnter(void);
int __cdecl SceneLeave(void);

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zclass.camera.vector-negate
 *
 * Purpose: negate all three components by flipping their binary32 sign bits.
 *
 * Reconstruction: recurring inline-helper family (the same island recurs at
 * retail 0x453848, 0x4730b9, 0x4730f0, 0x473e82, 0x473eb4 and 0x473f3b);
 * original spelling and declaration location unproved. Camera.c-resident
 * inline definition.
 *
 * Raw assembly: the repeated home materialization/reload and scheduling
 * strongly support an inferred inline-assembly helper; the documented VC5SP3
 * C/C++ candidates did not reproduce the complete consumer body.
 *
 * Island contract: reads the named src/dest pointers; clobbers EAX, EBX, ECX
 * and EDX; arithmetic flags follow the final XOR. No x87 instructions. All
 * three component representations are loaded before any result is stored.
 *
 * Authorized consumer: 0x44abf0, range [0x44ac5e,0x44ac85). Fresh governed
 * complete-body, relocation and linked-identity proof remains required.
 */
__inline void Vec3Negate(const zVec3* src, zVec3* dest)
{
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
    mov ebx, src
    mov ecx, dest
    mov eax, dword ptr [ebx]zVec3.x
    mov edx, dword ptr [ebx]zVec3.y
    mov ebx, dword ptr [ebx]zVec3.z
    xor eax, 080000000h
    xor edx, 080000000h
    xor ebx, 080000000h
    mov dword ptr [ecx]zVec3.x, eax
    mov dword ptr [ecx]zVec3.y, edx
    mov dword ptr [ecx]zVec3.z, ebx
    }
#else
    const unsigned int* const srcBits = (const unsigned int*)src;
    unsigned int* const destBits = (unsigned int*)dest;
    const unsigned int x = srcBits[0] ^ 0x80000000u;
    const unsigned int y = srcBits[1] ^ 0x80000000u;
    const unsigned int z = srcBits[2] ^ 0x80000000u;
    destBits[0] = x;
    destBits[1] = y;
    destBits[2] = z;
#endif
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zclass.camera.vector-length-sq
 *
 * Purpose: return the grouped (x*x + y*y) + z*z square sum as binary32.
 *
 * Reconstruction: recurring inline-helper family (the same ECX island recurs
 * in about fifteen retail functions); original spelling and declaration
 * location unproved. Camera.c-resident inline definition.
 *
 * Raw assembly: the documented VC5SP3 C/C++ candidates did not reproduce the
 * home, schedule and result-store shape of the complete consumer body.
 *
 * Island contract: reads the named vector pointer; clobbers ECX; integer flags
 * unchanged. x87 depth 0/3/0: grouped square sum followed by one binary32
 * result store.
 *
 * Authorized consumer: 0x44b8c0, range [0x44ba25,0x44ba41). Fresh governed
 * complete-body, relocation and linked-identity proof remains required.
 */
__inline float Vec3LengthSq(const zVec3* vec)
{
    float lengthSq;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
    mov ecx, vec
    fld dword ptr [ecx]zVec3.x
    fmul dword ptr [ecx]zVec3.x
    fld dword ptr [ecx]zVec3.y
    fmul dword ptr [ecx]zVec3.y
    fld dword ptr [ecx]zVec3.z
    fmul dword ptr [ecx]zVec3.z
    fxch st(1)
    faddp st(2), st
    faddp st(1), st
    fstp lengthSq
    }
#else
    lengthSq = (vec->x * vec->x + vec->y * vec->y) + vec->z * vec->z;
#endif
    return lengthSq;
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zclass.camera.vector-length
 *
 * Purpose: return FSQRT of the grouped (x*x + y*y) + z*z sum as binary32.
 *
 * Reconstruction: recurring inline-helper family (the same ECX island recurs
 * in about fifteen retail functions); original spelling and declaration
 * location unproved. Camera.c-resident inline definition.
 *
 * Raw assembly: the documented VC5SP3 C/C++ candidates did not reproduce the
 * home, schedule and result-store shape of the complete consumer body.
 *
 * Island contract: reads the named vector pointer; clobbers ECX; integer flags
 * unchanged. x87 depth 0/3/0: grouped square sum, then FSQRT, then one
 * binary32 result store; no intervening rounded squared-length store.
 *
 * Authorized consumer: 0x44abf0, range [0x44ad28,0x44ad46). Fresh governed
 * complete-body, relocation and linked-identity proof remains required.
 */
__inline float Vec3Length(const zVec3* vec)
{
    float vecLength;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
    mov ecx, vec
    fld dword ptr [ecx]zVec3.x
    fmul dword ptr [ecx]zVec3.x
    fld dword ptr [ecx]zVec3.y
    fmul dword ptr [ecx]zVec3.y
    fld dword ptr [ecx]zVec3.z
    fmul dword ptr [ecx]zVec3.z
    fxch st(1)
    faddp st(2), st
    faddp st(1), st
    fsqrt
    fstp vecLength
    }
#else
    vecLength = (float)sqrt((vec->x * vec->x + vec->y * vec->y) + vec->z * vec->z);
#endif
    return vecLength;
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zclass.camera.fast-sqrt-estimate
 *
 * Purpose: reproduce the legacy binary32 square-root estimate by reading
 * the input representation, arithmetic-shifting it right by one, adding
 * 0x1fc00000, and writing the resulting representation to the named result.
 *
 * Reconstruction: recurring inline-helper family; original spelling and
 * declaration location unproved. Camera.c-resident inline definition.
 *
 * Raw assembly: documented VC5SP3 C/C++ candidates did not reproduce the
 * argument/result homes and instruction shape at the authorized sites.
 * Compiler owns argument setup, homes, frame, register saves and return.
 *
 * Island contract: named 32-bit input/output; clobbers EAX and arithmetic
 * flags, with final flags from ADD. No x87 instructions; depth 0/0/0 here.
 * No special-value correction or zero-input special case is performed.
 *
 * Authorized consumer: 0x44b8c0.
 * Ranges: [0x44baab,0x44bab8), [0x44bc39,0x44bc46).
 * Fresh governed complete-body, relocation and linked-identity proof
 * remains required.
 */
__inline float FastSqrt(float value)
{
    float result;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
    mov eax, value
    sar eax, 1
    add eax, 01fc00000h
    mov result, eax
    }
#else
    *(int*)&result = (*(int*)&value >> 1) + 0x1fc00000;
#endif
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistanceadjustenabled
 * @recoil-artifact defines .data recoil:data:0x4ddd14: g_CZClass_CameraAutoClipDistanceAdjustEnabled.
 *
 * Purpose: enable adaptive camera clip-distance changes during scene render.
 */
int g_CZClass_CameraAutoClipDistanceAdjustEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistancethreshold
 * @recoil-artifact defines .data recoil:data:0x4ddd18: g_CZClass_CameraAutoClipDistanceThreshold.
 * Purpose: frame-time threshold used by adaptive camera clip-distance scaling.
 */
float g_CZClass_CameraAutoClipDistanceThreshold = 0.04f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistancescale
 * @recoil-artifact defines .data recoil:data:0x4ddd1c: g_CZClass_CameraAutoClipDistanceScale.
 * Purpose: current adaptive camera clip-distance scale.
 */
float g_CZClass_CameraAutoClipDistanceScale = 1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistancestep
 * @recoil-artifact defines .data recoil:data:0x4ddd20: g_CZClass_CameraAutoClipDistanceStep.
 * Purpose: per-frame adaptive camera clip-distance scale step.
 */
float g_CZClass_CameraAutoClipDistanceStep = 0.05f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameraautoclipdistanceminscale
 * @recoil-artifact defines .data recoil:data:0x4ddd24: g_CZClass_CameraAutoClipDistanceMinScale.
 * Purpose: minimum adaptive camera clip-distance scale clamp.
 */
float g_CZClass_CameraAutoClipDistanceMinScale = 0.6f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-objecthsetestenabled
 * @recoil-artifact defines .data recoil:data:0x4ddd10: g_CZClass_ObjectHseTestEnabled.
 * Purpose: enable projected object visibility testing during tiled render.
 */
int g_CZClass_ObjectHseTestEnabled = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-currentcamera
 * @recoil-artifact defines .data recoil:data:0x4ddd34: g_CZClass_CurrentCamera.
 * Purpose: track the current active camera node.
 */
CZNodePartial* g_CZClass_CurrentCamera = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-cameratargetnode
 * @recoil-artifact defines .data recoil:data:0x4ddd38: g_CZClass_CameraTargetNode.
 * Purpose: track the current camera target node.
 */
CZNodePartial* g_CZClass_CameraTargetNode = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-camera-prevlistenerposx
 * @recoil-artifact defines .data recoil:data:0x4f4988: g_Camera_PrevListenerPosX.
 * BN data inventory classifies this as an adjacent zero-initialized legacy
 * Camera.c float with no current source or BN consumers.
 * Purpose: preserve the retired camera previous-listener-position X storage.
 */
float g_Camera_PrevListenerPosX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-camera-prevlistenerposy
 * @recoil-artifact defines .data recoil:data:0x4f498c: g_Camera_PrevListenerPosY.
 * BN data inventory classifies this as an adjacent zero-initialized legacy
 * Camera.c float with no current source or BN consumers.
 * Purpose: preserve the retired camera previous-listener-position Y storage.
 */
float g_Camera_PrevListenerPosY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-camera-prevlistenerposz
 * @recoil-artifact defines .data recoil:data:0x4f4990: g_Camera_PrevListenerPosZ.
 * BN data inventory classifies this as an adjacent zero-initialized legacy
 * Camera.c float with no current source or BN consumers.
 * Purpose: preserve the retired camera previous-listener-position Z storage.
 */
float g_Camera_PrevListenerPosZ = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-findconvexhullunexpectedreturnmsg
 * @recoil-artifact defines .data recoil:data:0x4dddbc: g_CZClass_FindConvexHullUnexpectedReturnMsg.
 * BN data inventory declares writable Camera.c diagnostic literal char[0x37].
 * Purpose: report the unexpected convex-hull exit path during frustum-grid
 * footprint construction.
 */
char g_CZClass_FindConvexHullUnexpectedReturnMsg[0x37] = "Returning from find_convex_hull_xz in unexpected line.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-diamondtilerneedmoreringsmsg
 * @recoil-artifact defines .data recoil:data:0x4dddf4: g_CZClass_DiamondTilerNeedMoreRingsMsg.
 * BN data inventory declares writable Camera.c diagnostic literal char[0x26].
 * Purpose: report overflow of camera frustum-grid diamond ring buckets.
 */
char g_CZClass_DiamondTilerNeedMoreRingsMsg[0x26] = "Error: Need more diamond tiler rings.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-diamondtilerneedmorecellsperringmsg
 * @recoil-artifact defines .data recoil:data:0x4dde1c: g_CZClass_DiamondTilerNeedMoreCellsPerRingMsg.
 * BN data inventory declares writable Camera.c diagnostic literal char[0x2f].
 * Purpose: report overflow of a camera frustum-grid diamond ring's cell list.
 */
char g_CZClass_DiamondTilerNeedMoreCellsPerRingMsg[0x2f] = "Error: Need more diamond tiler cells per ring.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-lineerrorpointinpolygoninitcamerafrustumfmt
 * @recoil-artifact defines .data recoil:data:0x4dde4c: g_CZClass_LineErrorPointInPolygonInitCameraFrustumFmt.
 * BN data inventory declares writable Camera.c diagnostic format char[0x53].
 * Purpose: format the camera frustum-footprint mesh-face filter failure
 * diagnostic with the legacy source file and line.
 */
char g_CZClass_LineErrorPointInPolygonInitCameraFrustumFmt[0x53]
    = "%s: Line %d: ERROR from gModDIPointInPolygonInit() for camera "
      "frustrum footprint.\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zclass-vapstaticsnodename
 * @recoil-artifact defines .data recoil:data:0x4ddea0: g_CZClass_VapStaticsNodeName.
 * BN data inventory declares the shared writable zClass VAP statics node-name
 * literal char[0xc], referenced by Camera.c render filtering and cls_world.c
 * virtual-area partition creation.
 * Purpose: name generated virtual-area statics nodes and identify them during
 * offset-tile camera rendering.
 */
char g_CZClass_VapStaticsNodeName[0x0c] = "VAP_statics";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zcamera-frustumfootprintpoints
 * @recoil-artifact defines .data recoil:data:0x56cc40: g_zCamera_FrustumFootprintPoints.
 * Purpose: cache the frustum origin plus four corner points used by camera
 * grid-tile construction; BN bounds this zero-initialized array to five zVec3
 * entries, with the adjacent zero gaps outside this symbol.
 */
zVec3 g_zCamera_FrustumFootprintPoints[5] = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zcamera-frustumfootprintpointcount
 * @recoil-artifact defines .data recoil:data:0x56ccac: g_zCamera_FrustumFootprintPointCount.
 * Purpose: count active frustum footprint points for grid-tile construction.
 */
int g_zCamera_FrustumFootprintPointCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.g-zcamera-frustumgridtilerings
 * @recoil-artifact defines .data recoil:data:0x56ccc0: g_zCamera_FrustumGridTileRings.
 * Purpose: cache up to 50 diamond-ring buckets of camera frustum grid tiles
 * for the scene render pass.
 */
zCamera_FrustumGridTileRingPartial g_zCamera_FrustumGridTileRings[50] = { 0 };

/**
 * Grid cell coordinate pair filled by the world grid queries.
 * Evidence: the retail frames of 0x44c8e0 and 0x44c3c0 keep every (col, row)
 * output pair of their world grid queries in adjacent dwords.
 * Purpose: address one world area-grid cell by column and row.
 */
typedef struct GridCell {
    int col;
    int row;
} GridCell;

enum { kZClassNodeCamera = 1, kZClassNodeWorld = 2 };

/*
 * BN diagnostic string data used by 0x44a760 gwCameraGetFOV:
 * 0x4dd9d4 null-node text, 0x4dd9bc null-class-data text,
 * 0x4ddd44 Camera.c source path, and 0x4ddd68 bad-class format.
 * The generic diagnostic text/format strings are pooled across zClass
 * callers, so their owner is shared rather than Camera.c-only.
 */

/**
 * Original inline validation construct observed in camera setter/getter
 * callers (D:\Proj\GameZRecoil\zClass\Camera.c).
 * Purpose: validate a camera node, recover its camera data pointer, and
 * preserve the caller-specific legacy source-line diagnostics.
 */
#define ValidateCameraNode(node, nullLine, dataLine, classLine)                                                        \
    if ((node) == 0) {                                                                                                 \
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", (nullLine), "Null node pointer.");                 \
        return 5;                                                                                                      \
    }                                                                                                                  \
    if ((node)->classData == 0) {                                                                                      \
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", (dataLine), "Null class data pointer");            \
        return 5;                                                                                                      \
    }                                                                                                                  \
    if ((node)->classId != kZClassNodeCamera) {                                                                        \
        ReportOld(                                                                                                     \
            0x400,                                                                                                     \
            "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",                                                                 \
            (classLine),                                                                                               \
            "Bad Class Found.\n Wanted (%d)\n Found (%d)",                                                             \
            (node)->classId,                                                                                           \
            kZClassNodeCamera                                                                                          \
        );                                                                                                             \
        return 3;                                                                                                      \
    }

/**
 * Original inline construct observed in camera target callers
 * (D:\Proj\GameZRecoil\zClass\Camera.c).
 * Purpose: select the active target vector field based on the camera flag
 * that switches between world target and Euler/target storage.
 */
#define GetSelectedTargetVector(data)                                                                                  \
    (((data)->cameraFlags & 0x02) != 0 ? &(data)->worldTarget : &(data)->localPosition)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.setviewdistance
 * @recoil-artifact defines .text recoil:function:0x449ba0: CZCamera::SetViewDistance.
 * @recoil-match byte
 *
 * Purpose: configure adaptive camera clip-distance scaling from view distance.
 */
void __fastcall SetViewDistance(int enableAutoClip, float distance)
{
    if (distance == 0.0f) {
        g_CZClass_CameraAutoClipDistanceThreshold = 0.04f;
    } else {
        g_CZClass_CameraAutoClipDistanceThreshold = 1.0f / distance;
    }
    g_CZClass_CameraAutoClipDistanceAdjustEnabled = enableAutoClip;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameranew
 * @recoil-artifact defines .text recoil:function:0x449be0: CZCamera::gwCameraNew.
 *
 *
 * Purpose: allocate and initialize a camera node and its class data.
 */
CZNodePartial* __cdecl gwCameraNew(void)
{
    CZNodePartial* node = gwNodeNew();
    CZCameraDataPartial* data;
    if (node == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x1e8, "Null node pointer.");
        return 0;
    }

    node->classId = kZClassNodeCamera;
    data = (CZCameraDataPartial*)(calloc(1, sizeof(CZCameraDataPartial)));
    node->classData = data;
    // Retail zeroes localRotation through its own zero register (VC5 intrinsic memset expansion).
    memset(&data->localRotation, 0, sizeof(data->localRotation));
    data->viewportWidth = 1.0f;
    data->viewportHeight = 1.0f;
    data->localPosition.x = 0.0f;
    data->localPosition.y = 0.0f;
    data->localPosition.z = 0.0f;
    data->frustumVectorsDirty = 1;
    data->transformDirty = 1;
    data->localFrustumNormalsDirty = 1;
    data->variantOverrideEnabled = 0;
    Clear(&data->variantTag);
    CZTypeListInsert(8, node);
    return node;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameraaddchild
 * @recoil-artifact defines .text recoil:function:0x449c90: CZCamera::gwCameraAddChild.
 * @recoil-match byte
 *
 * BN source path evidence: D:\Proj\GameZRecoil\zClass\Camera.c.
 * Purpose: validate camera parent/child inputs before using the generic
 * zClass listA/listB child-link routine.
 */
int __fastcall gwCameraAddChild(CZNodePartial* parent, CZNodePartial* child)
{
    if (parent == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x239, "Null node pointer.");
        return 5;
    }
    if (child == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x23a, "Null node pointer.");
        return 5;
    }

    return AddChildGeneric(parent, child);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameraremovechild
 * @recoil-artifact defines .text recoil:function:0x449cd0: CZCamera::gwCameraRemoveChild.
 * @recoil-match byte
 *
 * BN source path evidence: D:\Proj\GameZRecoil\zClass\Camera.c.
 * Purpose: validate camera parent/child inputs before using the generic
 * zClass listA/listB child-unlink routine.
 */
int __fastcall gwCameraRemoveChild(CZNodePartial* parent, CZNodePartial* child)
{
    if (parent == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x251, "Null node pointer.");
        return 5;
    }
    if (child == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x252, "Null node pointer.");
        return 5;
    }

    return RemoveChildGeneric(parent, child);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetactive
 * @recoil-artifact defines .text recoil:function:0x449d10: CZCamera::gwCameraSetActive.
 * @recoil-match byte
 *
 * Purpose: route the camera active-state update through the generic node helper.
 */
int __fastcall gwCameraSetActive(CZNodePartial* node, int active)
{
    return gwNodeSetActive(node, active);
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetflagbit0
 * @recoil-artifact defines .text recoil:function:0x449d20: CZCamera::gwCameraSetFlagBit0.
 * @recoil-match byte
 *
 * Camera data flag bit 0 gates the zSound listener-state update in
 * BuildWorldTransform.
 * Purpose: validate a camera node and set or clear camera flag bit 0.
 */
int __fastcall gwCameraSetFlagBit0(CZNodePartial* node, int enabled)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(node, 0x274, 0x275, 0x276);
    data = (CZCameraDataPartial*)(node->classData);

    if (enabled != 0) {
        data->cameraFlags |= 0x01;
    } else {
        data->cameraFlags &= ~0x01;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.settargetnode
 * @recoil-artifact defines .text recoil:function:0x449da0: CZCamera::SetTargetNode.
 * @recoil-match byte
 *
 * Purpose: store the current global camera target node and report success.
 */
int __fastcall SetTargetNode(CZNodePartial* target)
{
    g_CZClass_CameraTargetNode = target;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.setactivecamera
 * @recoil-artifact defines .text recoil:function:0x449db0: CZCamera::SetActiveCamera.
 * @recoil-match byte
 *
 * Purpose: store the current global camera node and return it.
 */
CZNodePartial* __fastcall SetActiveCamera(CZNodePartial* camera)
{
    g_CZClass_CurrentCamera = camera;
    return camera;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.setobjecthsetestenabled
 * @recoil-artifact defines .text recoil:function:0x449dc0: CZCamera::SetObjectHseTestEnabled.
 * @recoil-match byte
 *
 * Purpose: store the object HSE test enable flag and report success.
 */
int __fastcall SetObjectHseTestEnabled(int enabled)
{
    g_CZClass_ObjectHseTestEnabled = enabled;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetworld
 * @recoil-artifact defines .text recoil:function:0x449dd0: CZCamera::gwCameraSetWorld.
 * @recoil-match byte
 *
 * Purpose: validate camera and world nodes before assigning the camera world.
 */
gwCameraSetWorld(CZNodePartial * camera, CZNodePartial * world)
{
    if (camera == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x2be, "Null node pointer.");
        return 5;
    }
    if (world == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x2bf, "Null node pointer.");
        return 5;
    }

    if (camera->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x2c1, "Null class data pointer");
        return 5;
    }
    if (world->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x2c2, "Null class data pointer");
        return 5;
    }

    if (camera->classId != kZClassNodeCamera) {
        ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
            0x2c4,
            "Bad Class Found.\n Wanted (%d)\n Found (%d)",
            camera->classId,
            kZClassNodeCamera
        );
        return 3;
    }
    if (world->classId != kZClassNodeWorld) {
        ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
            0x2c5,
            "Bad Class Found.\n Wanted (%d)\n Found (%d)",
            world->classId,
            kZClassNodeWorld
        );
        return 3;
    }

    ((CZCameraDataPartial*)(camera->classData))->worldNode = world;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetworld
 * @recoil-artifact defines .text recoil:function:0x449e80: CZCamera::gwCameraGetWorld.
 * @recoil-match byte
 *
 * Purpose: return the world node currently assigned to the camera.
 */
CZNodePartial* __fastcall gwCameraGetWorld(CZNodePartial* camera)
{
    return ((CZCameraDataPartial*)(camera->classData))->worldNode;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetwindow
 * @recoil-artifact defines .text recoil:function:0x449e90: CZCamera::gwCameraSetWindow.
 * @recoil-match byte
 *
 * Purpose: assign the window node used by the camera view context.
 */
gwCameraSetWindow(CZNodePartial * camera, CZNodePartial * window)
{
    ((CZCameraDataPartial*)(camera->classData))->windowNode = window;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetposition
 * @recoil-artifact defines .text recoil:function:0x449ea0: CZCamera::gwCameraSetEulerAngles.
 * @recoil-match byte
 *
 * Purpose: set the camera Euler angles and dirty dependent transforms.
 */
gwCameraSetEulerAngles(CZNodePartial * camera, float x, float y, float z)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x3a7, 0x3a8, 0x3a9);
    data = (CZCameraDataPartial*)(camera->classData);

    data->transformDirty = 1;
    data->localRotation.x = x;
    data->localRotation.y = y;
    data->localRotation.z = z;
    data->cameraFlags &= ~0x02;
    if (camera->listCountA > 0) {
        ActivateChildren(camera, data);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.activatechildren
 * @recoil-artifact defines .text recoil:function:0x449f50: CZCamera::ActivateChildren.
 * @recoil-match byte
 *
 * Purpose: mark camera children dirty and register the active camera node.
 */
int __fastcall ActivateChildren(CZNodePartial* camera, CZCameraDataPartial* data)
{
    data->cameraFlags |= 0x04;
    if ((camera->flags & 0x01) == 0) {
        CZTypeListInsert(7, camera);
        camera->flags |= 0x01;
    }
    camera->flags |= 0x02;

    if (camera->listCountB > 0) {
        int i;
        for (i = 0; i < camera->listCountB; ++i) {
            PropagateTransformDirtyRecursive(camera->listB[i]);
        }
    }

    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameratranslate
 * @recoil-artifact defines .text recoil:function:0x449fb0: CZCamera::gwCameraAddEulerAngles.
 * @recoil-match byte
 *
 * Purpose: add deltas to the camera Euler angles and dirty dependent transforms.
 */
gwCameraAddEulerAngles(CZNodePartial * camera, float dx, float dy, float dz)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x3df, 0x3e0, 0x3e1);
    data = (CZCameraDataPartial*)(camera->classData);

    data->localRotation.x += dx;
    data->localRotation.y += dy;
    data->localRotation.z += dz;
    data->transformDirty = 1;
    if (camera->listCountA > 0) {
        ActivateChildren(camera, data);
    }

    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetposition
 * @recoil-artifact defines .text recoil:function:0x44a060: CZCamera::gwCameraGetEulerAngles.
 * @recoil-match byte
 *
 * Purpose: return the camera Euler angle components.
 */
gwCameraGetEulerAngles(CZNodePartial * camera, float* outX, float* outY, float* outZ)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x414, 0x415, 0x416);
    data = (CZCameraDataPartial*)(camera->classData);

    *outX = data->localRotation.x;
    *outY = data->localRotation.y;
    *outZ = data->localRotation.z;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasettarget
 * @recoil-artifact defines .text recoil:function:0x44a0f0: CZCamera::gwCameraSetPosition.
 * @recoil-match byte
 *
 * Purpose: set the camera position (matrix translation in matrix mode) and update children.
 */
gwCameraSetPosition(CZNodePartial * camera, float x, float y, float z)
{
    CZCameraDataPartial* data;
    zVec3* target;
    ValidateCameraNode(camera, 0x43c, 0x43d, 0x43e);
    data = (CZCameraDataPartial*)(camera->classData);

    target = GetSelectedTargetVector(data);
    target->x = x;
    target->y = y;
    target->z = z;
    if (camera->listCountA > 0) {
        ActivateChildren(camera, data);
    }

    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameratranslatetarget
 * @recoil-artifact defines .text recoil:function:0x44a1a0: CZCamera::gwCameraTranslate.
 * @recoil-match source
 *
 * Purpose: translate the camera position (matrix translation in matrix mode) and update children.
 */
gwCameraTranslate(CZNodePartial * camera, float dx, float dy, float dz)
{
    CZCameraDataPartial* data;
    zVec3* target;
    ValidateCameraNode(camera, 0x46f, 0x470, 0x471);
    data = (CZCameraDataPartial*)(camera->classData);

    target = GetSelectedTargetVector(data);
    target->x += dx;
    target->y += dy;
    target->z += dz;
    if (camera->listCountA > 0) {
        ActivateChildren(camera, data);
    }

    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragettarget
 * @recoil-artifact defines .text recoil:function:0x44a250: CZCamera::gwCameraGetPosition.
 * @recoil-match byte
 *
 * Purpose: return the camera position components (matrix translation in matrix mode).
 */
gwCameraGetPosition(CZNodePartial * camera, float* outX, float* outY, float* outZ)
{
    CZCameraDataPartial* data;
    zVec3* target;
    ValidateCameraNode(camera, 0x4a1, 0x4a2, 0x4a3);
    data = (CZCameraDataPartial*)(camera->classData);

    target = GetSelectedTargetVector(data);
    *outX = target->x;
    *outY = target->y;
    *outZ = target->z;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetnearfarclip
 * @recoil-artifact defines .text recoil:function:0x44a2f0: CZCamera::gwCameraSetNearFarClip.
 * @recoil-match byte
 *
 * Purpose: store near/far clip distances and dirty frustum vectors.
 */
gwCameraSetNearFarClip(CZNodePartial * camera, float nearClip, float farClip)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x509, 0x50a, 0x50b);
    data = (CZCameraDataPartial*)(camera->classData);

    data->nearClip = nearClip;
    data->farClip = farClip;
    data->frustumVectorsDirty = 1;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetnearfarclip
 * @recoil-artifact defines .text recoil:function:0x44a380: CZCamera::gwCameraGetNearFarClip.
 * @recoil-match byte
 *
 * Purpose: return the camera near/far clip distances.
 */
gwCameraGetNearFarClip(CZNodePartial * camera, float* outNear, float* outFar)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x52f, 0x530, 0x531);
    data = (CZCameraDataPartial*)(camera->classData);

    *outNear = data->nearClip;
    *outFar = data->farClip;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetviewport
 * @recoil-artifact defines .text recoil:function:0x44a410: CZCamera::gwCameraSetViewport.
 * @recoil-match byte
 *
 * Purpose: update viewport dimensions and derived frustum scale values.
 */
gwCameraSetViewport(CZNodePartial * camera, float viewportWidth, float viewportHeight)
{
    CZCameraDataPartial* data;
    float maxFov;
    float halfFovX;
    float halfFovY;
    float tanHalfFovX;
    float tanHalfFovY;
    ValidateCameraNode(camera, 0x553, 0x554, 0x555);
    data = (CZCameraDataPartial*)(camera->classData);

    maxFov = 1.39600003f;
    data->viewportWidth = viewportWidth;
    data->viewportHeight = viewportHeight;
    data->fovX = data->frustumWidth / viewportWidth;
    data->fovY = data->frustumHeight / viewportHeight;
    if (data->fovX > maxFov) {
        data->fovX = maxFov;
    }
    if (data->fovY > maxFov) {
        data->fovY = maxFov;
    }

    halfFovX = data->fovX * 0.5f;
    halfFovY = data->fovY * 0.5f;
    tanHalfFovX = (float)(tan((double)(halfFovX)));
    tanHalfFovY = (float)(tan((double)(halfFovY)));

    data->localFrustumNormalsDirty = 1;
    data->frustumVectorsDirty = 1;
    data->frustumYaw = halfFovX;
    data->frustumPitch = halfFovY;
    data->viewportScaleX = 1.0f / tanHalfFovX;
    data->viewportScaleY = 1.0f / tanHalfFovY;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetviewport
 * @recoil-artifact defines .text recoil:function:0x44a580: CZCamera::gwCameraGetViewport.
 * @recoil-match byte
 *
 * Purpose: return the camera viewport dimensions.
 */
gwCameraGetViewport(CZNodePartial * camera, float* outWidth, float* outHeight)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x58e, 0x58f, 0x590);
    data = (CZCameraDataPartial*)(camera->classData);

    *outWidth = data->viewportWidth;
    *outHeight = data->viewportHeight;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetfov
 * @recoil-artifact defines .text recoil:function:0x44a610: CZCamera::gwCameraSetFOV.
 * @recoil-match byte
 *
 * Purpose: set camera frustum dimensions and derived projection scale values.
 */
gwCameraSetFOV(CZNodePartial * camera, float fovX, float fovY)
{
    CZCameraDataPartial* data;
    float normalizedFovX;
    float normalizedFovY;
    float halfFovX;
    float halfFovY;
    float tanHalfFovX;
    float tanHalfFovY;
    ValidateCameraNode(camera, 0x5b2, 0x5b3, 0x5b4);
    data = (CZCameraDataPartial*)(camera->classData);

    normalizedFovX = fovX / data->viewportWidth;
    normalizedFovY = fovY / data->viewportHeight;
    halfFovX = normalizedFovX * 0.5f;
    halfFovY = normalizedFovY * 0.5f;
    tanHalfFovX = (float)(tan((double)(halfFovX)));
    tanHalfFovY = (float)(tan((double)(halfFovY)));

    data->localFrustumNormalsDirty = 1;
    data->frustumVectorsDirty = 1;
    data->frustumWidth = fovX;
    data->frustumHeight = fovY;
    data->fovX = normalizedFovX;
    data->fovY = normalizedFovY;
    data->frustumYaw = halfFovX;
    data->frustumPitch = halfFovY;
    data->viewportScaleX = 1.0f / tanHalfFovX;
    data->viewportScaleY = 1.0f / tanHalfFovY;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetfov
 * @recoil-artifact defines .text recoil:function:0x44a760: CZCamera::gwCameraGetFOV.
 * @recoil-match byte
 *
 * BN source path evidence: D:\Proj\GameZRecoil\zClass\Camera.c.
 * Touched diagnostic string data: 0x4dd9d4, 0x4dd9bc, 0x4ddd44,
 * and 0x4ddd68.
 * Purpose: return the camera frustum FOV pair after legacy camera-node
 * validation diagnostics.
 */
int __fastcall gwCameraGetFOV(CZNodePartial* camera, float* outFovX, float* outFovY)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x5e7, 0x5e8, 0x5e9);
    data = (CZCameraDataPartial*)(camera->classData);

    *outFovX = data->frustumWidth;
    *outFovY = data->frustumHeight;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameragetclipdistance
 * @recoil-artifact defines .text recoil:function:0x44a7f0: CZCamera::gwCameraGetClipDistance.
 * @recoil-match byte
 *
 * Purpose: return the camera clip distance.
 */
gwCameraGetClipDistance(CZNodePartial * camera, float* outClipDistance)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x609, 0x60a, 0x60b);
    data = (CZCameraDataPartial*)(camera->classData);

    *outClipDistance = data->clipDistance;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetclipdistance
 * @recoil-artifact defines .text recoil:function:0x44a870: CZCamera::gwCameraSetClipDistance.
 * @recoil-match byte
 *
 * Purpose: store the camera clip distance and inverse squared distance.
 */
gwCameraSetClipDistance(CZNodePartial * camera, float clipDistance)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x62a, 0x62b, 0x62c);
    data = (CZCameraDataPartial*)(camera->classData);

    data->clipDistance = clipDistance;
    data->invClipDistanceSq = 1.0f / (clipDistance * clipDistance);
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasethorizon
 * @recoil-artifact defines .text recoil:function:0x44a910: CZCamera::gwCameraSetHorizon.
 * @recoil-match byte
 *
 * Purpose: assign the horizon node that follows the camera position.
 */
gwCameraSetHorizon(CZNodePartial * camera, CZNodePartial * horizonNode)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x64d, 0x64e, 0x64f);
    data = (CZCameraDataPartial*)(camera->classData);

    data->horizonNode = horizonNode;
    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasethorizonxz
 * @recoil-artifact defines .text recoil:function:0x44a980: CZCamera::gwCameraSetHorizonXZ.
 * @recoil-match byte
 *
 * Purpose: assign the horizon node that follows camera X/Z position.
 */
gwCameraSetHorizonXZ(CZNodePartial * camera, CZNodePartial * horizonXZNode)
{
    CZCameraDataPartial* data;
    ValidateCameraNode(camera, 0x66e, 0x66f, 0x670);
    data = (CZCameraDataPartial*)(camera->classData);

    data->horizonXZNode = horizonXZNode;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcameraupdate
 * @recoil-artifact defines .text recoil:function:0x44a9f0: CZCamera::gwCameraUpdate.
 * @recoil-match byte
 *
 * Purpose: validate the camera node and run the camera update implementation.
 */
int __fastcall gwCameraUpdate(CZNodePartial* camera)
{
    if (camera == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x75c, "Null node pointer.");
        return 5;
    }

    if (camera->classData == 0) {
        ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x75d, "Null class data pointer");
        return 5;
    }

    return UpdateImpl(camera, 0);
}

/**
 * Original inline helper evidence: no standalone retail function; 0x44aa30
 * expands it twice (forward direction scaled by the near/far clip distance)
 * immediately before each reviewed vector-add island. The spelling is
 * descriptive; no original name is known.
 * Purpose: scale a vector by a scalar into an output vector.
 */
__inline void Vec3ScaleTo(const zVec3* vec, float scale, zVec3* out)
{
    out->x = vec->x * scale;
    out->y = vec->y * scale;
    out->z = vec->z * scale;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.updateimpl
 * @recoil-artifact defines .text recoil:function:0x44aa30: CZCamera::UpdateImpl.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match source
 *
 * Purpose: rebuild camera transforms, frustum planes, and clip centers.
 */
int __fastcall UpdateImpl(CZNodePartial* camera, zVec3* posOffset)
{
    CZCameraDataPartial* data = (CZCameraDataPartial*)(camera->classData);

    BuildWorldTransform(camera, data, posOffset);

    data->transformDirty = 1;
    if (data->localFrustumNormalsDirty != 0) {
        data->localFrustumNormalsDirty = 0;
        zVideoUpdateProjectionStateFromCameraData(data);
        data->transformDirty = 1;
    }

    data->transformDirty = 0;
    zClipAltBuildFrustumPlanes(data);

    if (data->frustumVectorsDirty != 0) {
        float halfWidth;
        float negHalfHeight;
        float negFarClip;
        float negHalfWidth;
        data->frustumVectorsDirty = 0;
        data->frustumOrigin.x = 0.0f;
        data->frustumOrigin.y = 0.0f;
        data->frustumOrigin.z = 0.0f;
        halfWidth = (float)(tan(data->fovX * 0.5f)) * data->farClip;
        data->frustumCorners[0].x = halfWidth;
        negHalfHeight = -((float)(tan(data->fovY * 0.5f)) * data->farClip);
        data->frustumCorners[0].y = negHalfHeight;
        negFarClip = -data->farClip;
        data->frustumCorners[0].z = negFarClip;
        negHalfWidth = -halfWidth;
        data->frustumCorners[1].x = negHalfWidth;
        data->frustumCorners[1].y = negHalfHeight;
        data->frustumCorners[1].z = negFarClip;
        data->frustumCorners[2].x = halfWidth;
        data->frustumCorners[2].y = -negHalfHeight;
        data->frustumCorners[2].z = negFarClip;
        data->frustumCorners[3].x = negHalfWidth;
        data->frustumCorners[3].y = -negHalfHeight;
        data->frustumCorners[3].z = negFarClip;
    }

    Vec3ScaleTo(&data->forwardDir, data->nearClip, &data->nearClipCenter);
    Vec3Add(&data->nearClipCenter, &data->cameraPos, &data->nearClipCenter);

    Vec3ScaleTo(&data->forwardDir, data->farClip, &data->farClipCenter);
    Vec3Add(&data->farClipCenter, &data->cameraPos, &data->farClipCenter);

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.buildworldtransform
 * @recoil-artifact defines .text recoil:function:0x44abf0: CZCamera::BuildWorldTransform.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zclass.camera.vector-negate
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zclass.camera.vector-length
 * @recoil-match byte
 *
 * Purpose: build the camera world transform and update the zSound
 * listener bridge previous-position state.
 */
int __fastcall BuildWorldTransform(CZNodePartial* camera, CZCameraDataPartial* data, zVec3* posOffset)
{
    zMat4x3* matrix;
    MatLoadIdentity();
    gwNodeBuildNodeToAncestorMatrix(camera, 1);

    matrix = zMathMatGetCurrent();
    if (posOffset != 0) {
        matrix->posX += posOffset->x;
        matrix->posY += posOffset->y;
        matrix->posZ += posOffset->z;
    }

    data->cameraPos = *(zVec3*)&matrix->posX;
    Vec3Negate((zVec3*)&matrix->zx, &data->forwardDir);

    memcpy(data->worldTransform, matrix, sizeof(zMat4x3));
    zMathMatExtractEulerAngles(matrix, &data->eulerAngles);
    MatLoadIdentity();
    zMathCameraStageInverseRotation((zMat4x3*)(data->worldTransform));

    if ((data->cameraFlags & 0x01) != 0) {
        /*
         * Purpose: compute a candidate listener velocity from
         * g_zSnd_PreviousListenerPos and retain it only when it passes
         * the strict subsonic test. Both failure paths share one
         * velocity-clearing block; listener position/state updates
         * follow either outcome.
         */
        zVec3 listenerVelocity;
        do {
            if (g_FrameDeltaTimeSec != 0.0f) {
                float inverseDeltaTime;
                float listenerSpeed;
                Vec3Subtract((zVec3*)&data->worldTransform[9], &g_zSnd_PreviousListenerPos, &listenerVelocity);
                inverseDeltaTime = 1.0f / g_FrameDeltaTimeSec;
                listenerVelocity.x *= inverseDeltaTime;
                listenerVelocity.y *= inverseDeltaTime;
                listenerVelocity.z *= inverseDeltaTime;
                listenerSpeed = Vec3Length(&listenerVelocity);
                if (zSndGetSpeedOfSoundMps() > listenerSpeed) {
                    break;
                }
            }
            listenerVelocity.x = listenerVelocity.y = listenerVelocity.z = 0.0f;
        } while (0);

        g_zSnd_PreviousListenerPos = *(zVec3*)&data->worldTransform[9];
        zSndUpdateListenerState((zSndListenerState*)(data->worldTransform), &listenerVelocity);
    }

    return 0;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44ada0
 * @recoil-artifact defines .text recoil:function:0x44ada0: CZCameraRenderTraverse.
 * @recoil-match byte
 *
 * Purpose: frustum-test and render a camera node traversal branch.
 */
CZCameraRenderTraverse(CZNodePartial * node, int siblingCountHint)
{
    const zVec3 unitScale = { 1.0f, 1.0f, 1.0f };
    int boundsContextPushed = 0;
    const int flags = node->flags;
    CZCameraDataPartial* data;
    int clipMask;
    int result;
    if ((flags & 0x04) == 0) {
        return 0;
    }

    node->flags = flags & ~0x02000000;
    data = (CZCameraDataPartial*)(node->classData);

    clipMask = *gModel_ClipMaskStackTop;
    result = 0;
    if ((clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
        if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0
            || (node->flags & 0x00080000) == 0) {
            zBBoxCorners corners;
            gwNodeGetViewBBoxCorners(node, &corners);
            CornersToBoundingSphere(&corners, (zVec3*)node->cachedSphereCenter, &node->cachedSphereCenter[3]);
            if ((node->flags & 0x00080000) != 0) {
                node->boundsFlags &= ~0x04;
            }
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
        }
        result
            = zVideoFrustumTestSphereClipMask((zVec3*)node->cachedSphereCenter, node->cachedSphereCenter[3], &clipMask);
        if ((node->flags & 0x80) != 0) {
            if (result == 0x20) {
                result = 0;
            }
            clipMask &= ~0x20;
        }
    }

    if (result == 0) {
        node->flags |= 0x80000000;
        MatStackPushAndCloneParent(data->worldTransform);
        MatApplyLocalTRS(&data->localRotation, &data->localPosition, &unitScale);
        if (g_CZClass_RenderBoundsContextActive == 0) {
            boundsContextPushed = 1;
            g_CZClass_RenderBoundsContextActive = 1;
        }
        if (node->userDataOrDiRef != 0) {
            if (g_CZClass_RenderRangeFadeActive != 0) {
                ((zDiPartial*)(unsigned int)node->userDataOrDiRef)->flags |= 0x08;
                ((zDiPartial*)(unsigned int)node->userDataOrDiRef)->blendScale = g_CZClass_RenderRangeFadeScale;
            }
            gModel_RenderFn(node, clipMask);
        }
        if (node->listCountB > 0) {
            int i;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;
            for (i = 0; i < node->listCountB; ++i) {
                gwNodeRenderDispatch(node->listB[i], node->listCountB);
            }
            --gModel_ClipMaskStackTop;
        }
        MatStackPopPtr();
    }

    if (boundsContextPushed != 0) {
        g_CZClass_RenderBoundsContextActive = 0;
    }
    return result;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44af60
 * @recoil-artifact defines .text recoil:function:0x44af60: CZSoundRenderTraverse
 * @recoil-match byte
 *
 * Purpose: cull a sound node, push its local transform, render the node and
 * children, and restore traversal state.
 */
CZSoundRenderTraverse(CZNodePartial * node, int siblingCountHint)
{
    const zVec3 angles = { 0.0f, 0.0f, 0.0f };
    const zVec3 unitScale = { 1.0f, 1.0f, 1.0f };
    int boundsContextPushed = 0;
    const int flags = node->flags;
    CZSoundDataPartial* data;
    int clipMask;
    int result;
    if ((flags & 0x04) == 0) {
        return 0;
    }

    node->flags = flags & ~0x02000000;
    data = (CZSoundDataPartial*)(node->classData);

    clipMask = *gModel_ClipMaskStackTop;
    result = 0;
    if ((clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
        if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0
            || (node->flags & 0x00080000) == 0) {
            zBBoxCorners corners;
            gwNodeGetViewBBoxCorners(node, &corners);
            CornersToBoundingSphere(&corners, (zVec3*)node->cachedSphereCenter, &node->cachedSphereCenter[3]);
            if ((node->flags & 0x00080000) != 0) {
                node->boundsFlags &= ~0x04;
            }
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
        }
        result
            = zVideoFrustumTestSphereClipMask((zVec3*)node->cachedSphereCenter, node->cachedSphereCenter[3], &clipMask);
        if ((node->flags & 0x80) != 0) {
            if (result == 0x20) {
                result = 0;
            }
            clipMask &= ~0x20;
        }
    }

    if (result == 0) {
        node->flags |= 0x80000000;
        MatStackPushAndCloneParent(data->savedParentMatrix);
        MatApplyLocalTRS(&angles, &data->localPosition, &unitScale);
        if (g_CZClass_RenderBoundsContextActive == 0) {
            boundsContextPushed = 1;
            g_CZClass_RenderBoundsContextActive = 1;
        }
        if (node->userDataOrDiRef != 0) {
            if (g_CZClass_RenderRangeFadeActive != 0) {
                ((zDiPartial*)(unsigned int)node->userDataOrDiRef)->flags |= 0x08;
                ((zDiPartial*)(unsigned int)node->userDataOrDiRef)->blendScale = g_CZClass_RenderRangeFadeScale;
            }
            gModel_RenderFn(node, clipMask);
        }
        if (node->listCountB > 0) {
            int i;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;
            for (i = 0; i < node->listCountB; ++i) {
                gwNodeRenderDispatch(node->listB[i], node->listCountB);
            }
            --gModel_ClipMaskStackTop;
        }
        MatStackPopPtr();
    }

    if (boundsContextPushed != 0) {
        g_CZClass_RenderBoundsContextActive = 0;
    }
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44b140
 * @recoil-artifact defines .text recoil:function:0x44b140: CZLightRenderTraverse
 * @recoil-match byte
 *
 * Purpose: cull an enabled light node, push render-bounds context when
 * needed, apply local transform, render the node subtree, and restore state.
 */
int __fastcall CZLightRenderTraverse(CZNodePartial* node, int siblingCountHint)
{
    const zVec3 unitScale = { 1.0f, 1.0f, 1.0f };
    int boundsContextPushed = 0;
    const int flags = node->flags;
    CZLightDataPartial* data;
    int clipMask;
    int result;
    if ((flags & 0x04) == 0) {
        return 0;
    }

    node->flags = flags & ~0x02000000;
    data = (CZLightDataPartial*)(node->classData);

    clipMask = *gModel_ClipMaskStackTop;
    result = 0;
    if ((clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
        if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0
            || (node->flags & 0x00080000) == 0) {
            zBBoxCorners corners;
            gwNodeGetViewBBoxCorners(node, &corners);
            CornersToBoundingSphere(&corners, (zVec3*)node->cachedSphereCenter, &node->cachedSphereCenter[3]);
            if ((node->flags & 0x00080000) != 0) {
                node->boundsFlags &= ~0x04;
            }
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
        }
        result
            = zVideoFrustumTestSphereClipMask((zVec3*)node->cachedSphereCenter, node->cachedSphereCenter[3], &clipMask);
        if ((node->flags & 0x80) != 0) {
            if (result == 0x20) {
                result = 0;
            }
            clipMask &= ~0x20;
        }
    }

    if (result == 0) {
        node->flags |= 0x80000000;
        MatStackPushAndCloneParent(data->savedParentMatrix);
        MatApplyLocalTRS(&data->localRotation, &data->localPosition, &unitScale);
        if (g_CZClass_RenderBoundsContextActive == 0) {
            boundsContextPushed = 1;
            g_CZClass_RenderBoundsContextActive = 1;
        }
        if (node->userDataOrDiRef != 0) {
            if (g_CZClass_RenderRangeFadeActive != 0) {
                ((zDiPartial*)(unsigned int)node->userDataOrDiRef)->flags |= 0x08;
                ((zDiPartial*)(unsigned int)node->userDataOrDiRef)->blendScale = g_CZClass_RenderRangeFadeScale;
            }
            gModel_RenderFn(node, clipMask);
        }
        if (node->listCountB > 0) {
            int i;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;
            for (i = 0; i < node->listCountB; ++i) {
                gwNodeRenderDispatch(node->listB[i], node->listCountB);
            }
            --gModel_ClipMaskStackTop;
        }
        MatStackPopPtr();
    }

    if (boundsContextPushed != 0) {
        g_CZClass_RenderBoundsContextActive = 0;
    }
    return result;
}

enum {
    kZClassNodeObject3D = 5,
    kObject3DVisibleFlag = 0x04,
    kObject3DTransformDirtyFlag = 0x20,
    kNodeBoundsDirtyFlag = 0x04,
    kSingleParentFlag = 0x00080000,
    kNodeTransformDirtyPropagatedFlag = 0x02000000
};

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
 * BN keeps the bounds refresh, sphere test, and far-clip repair as
 * caller-local render traversal code rather than a separate call target.
 * Purpose: decide whether object render culling is needed, refresh the view
 * bounding sphere, and run the frustum sphere clip-mask test.
 */
static __inline int CullNodeForRender(CZNodePartial* node, int siblingCountHint, int* clipMask)
{
    int testNeeded = 0;
    int result;
    if (g_CZClass_ObjectHseTestEnabled == 0) {
        testNeeded = ((*clipMask != 0 || g_CZClass_RenderFrustumGridTileIndex > 0) && siblingCountHint > 1);
    } else {
        testNeeded = (*clipMask != 0 && siblingCountHint > 1);
    }

    if (testNeeded == 0 && (node->flags & kSingleParentFlag) != 0) {
        return 0;
    }

    if ((node->boundsFlags & kNodeBoundsDirtyFlag) != 0 || g_CZClass_RenderBoundsContextActive != 0
        || (node->flags & kSingleParentFlag) == 0) {
        zBBoxCorners corners = { 0 };
        gwNodeGetViewBBoxCorners(node, &corners);
        CornersToBoundingSphere(&corners, zClassNodeViewSphereCenter(node), zClassNodeViewSphereRadius(node));
        if ((node->flags & kSingleParentFlag) != 0) {
            node->boundsFlags &= ~kNodeBoundsDirtyFlag;
        }
    }

    result = zVideoFrustumTestSphereClipMask(
        zClassNodeViewSphereCenter(node),
        *zClassNodeViewSphereRadius(node),
        clipMask
    );
    if ((node->flags & 0x80) != 0 && result == 0x20) {
        result = 0;
        *clipMask &= ~0x20;
    }
    return result;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
 * BN shows the Object3D matrix-selection branches in the render traversal
 * body, with only direct zMath provider calls inside the pattern.
 * Purpose: push the correct object matrix onto the zMath stack, recomputing
 * cached world matrix state when the transform is dirty.
 */
static __inline void PushObjectMatrix(CZObject3DDataPartial* data, int* pushed)
{
    const int flags = data->flags;
    if ((flags & 0x08) != 0) {
        *pushed = 0;
        return;
    }

    *pushed = 1;
    if ((flags & kObject3DTransformDirtyFlag) != 0) {
        MatStackPushAndCloneParent(data->cachedWorldMatrix);
        MatMultiply((const zMat4x3*)data->localMatrix, 3);
        data->flags &= ~kObject3DTransformDirtyFlag;
    } else if ((flags & kSingleParentFlag) == 0) {
        MatStackPushAndCloneParent(data->cachedWorldMatrix);
        MatMultiply((const zMat4x3*)data->localMatrix, 3);
    } else {
        MatStackPushPtr(data->cachedWorldMatrix);
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
 * BN keeps the render-state stack pushes and zModel setter calls in the
 * Object3D render traversal body.
 * Purpose: push vertex-alpha, alpha-scale, and software color override
 * render state for an Object3D node.
 */
static __inline void PushObjectRenderState(
    CZNodePartial* node,
    CZObject3DDataPartial* data,
    int* pushedVertexAlpha,
    int* pushedAlphaScale,
    int* pushedSoftwareState
)
{
    *pushedVertexAlpha = 0;
    *pushedAlphaScale = 0;
    *pushedSoftwareState = 0;

    if ((node->flags & 0x00800000) != 0 && g_CZClass_RenderVertexAlphaOverrideActive == 0) {
        *pushedVertexAlpha = 1;
        g_CZClass_RenderVertexAlphaOverrideActive = 1;
        zModelRenderVertexAlphaEnabledSetCurrent(1);
    }

    if ((data->flags & 0x02) != 0) {
        *pushedAlphaScale = 1;
        ++g_CZClass_RenderAlphaScaleStackTop;
        g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop] = data->alphaScale;
        zModelRenderAlphaScaleSetCurrent(data->alphaScale);
    }

    if ((data->flags & 0x04) != 0) {
        *pushedSoftwareState = 1;
        ++g_CZClass_SoftwarePathStateStackTop;
        g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].color = data->color;
        g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].alpha = data->colorAlpha;
        zModelFogTargetColorOverrideSetCurrent(
            &g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].color,
            data->colorAlpha
        );
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
 * BN keeps the vertex-alpha, alpha-scale, and software-state restore
 * sequence in the Object3D render traversal epilogue.
 * Purpose: restore Object3D render-state stacks after rendering a node
 * subtree.
 */
static __inline void PopObjectRenderState(int pushedVertexAlpha, int pushedAlphaScale, int pushedSoftwareState)
{
    if (pushedVertexAlpha != 0) {
        g_CZClass_RenderVertexAlphaOverrideActive = 0;
        zModelRenderVertexAlphaEnabledSetCurrent(0);
    }

    if (pushedAlphaScale != 0) {
        float scale;
        --g_CZClass_RenderAlphaScaleStackTop;
        scale = g_CZClass_RenderAlphaScaleStackTop >= 0
            ? g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop]
            : 1.0f;
        zModelRenderAlphaScaleSetCurrent(scale);
    }

    if (pushedSoftwareState != 0) {
        --g_CZClass_SoftwarePathStateStackTop;
        if (g_CZClass_SoftwarePathStateStackTop >= 0) {
            zModelFogTargetColorOverrideSetCurrent(
                &g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].color,
                g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].alpha
            );
        } else {
            zModelFogTargetColorOverrideSetCurrent(0, 0.0f);
        }
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x44b300 (D:\Proj\GameZRecoil\zClass\Object3d.c);
 * BN shows the child-loop variant check, recursive Object3D call, and
 * generic dispatch call inline in the traversal body.
 * Purpose: render Object3D children through variant filtering and dispatch
 * non-Object3D children through the generic node renderer.
 */
static __inline void RenderObjectChildren(CZNodePartial* node, int clipMask)
{
    int i;
    if (node->listCountB <= 0) {
        return;
    }

    ++gModel_ClipMaskStackTop;
    *gModel_ClipMaskStackTop = clipMask;
    for (i = 0; i < node->listCountB; ++i) {
        CZNodePartial* child = node->listB[i];
        if (child != 0 && child->classId == kZClassNodeObject3D) {
            if (CurrentAllowsId(child->nodeType) != 0) {
                CZObject3DRenderTraverse(child, node->listCountB);
            }
        } else if (child != 0) {
            gwNodeRenderDispatch(child, node->listCountB);
        }
    }
    --gModel_ClipMaskStackTop;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44b300
 * @recoil-artifact defines .text recoil:function:0x44b300: CZObject3DRenderTraverse
 * @recoil-match byte
 *
 * Purpose: cull Object3D nodes, manage transforms and render state, and render children.
 */
CZObject3DRenderTraverse(CZNodePartial * node, int siblingCountHint)
{
    const int flags = node->flags;
    int boundsContextPushed = 0;
    int altClipReset;
    CZObject3DDataPartial* data;
    int clipMask;
    int result;
    int testNeeded;
    if ((flags & kObject3DVisibleFlag) == 0) {
        return 0;
    }

    node->flags = flags & ~kNodeTransformDirtyPropagatedFlag;
    if (gAltClipPassEnabled != 0 && node == g_CZClass_CameraTargetNode) {
        altClipReset = 1;
        gAltClipPassEnabled = 0;
    } else {
        altClipReset = 0;
    }

    data = (CZObject3DDataPartial*)(node->classData);

    clipMask = *gModel_ClipMaskStackTop;
    result = 0;
    testNeeded = 0;
    if (g_CZClass_ObjectHseTestEnabled != 0) {
        testNeeded = ((clipMask != 0 || g_CZClass_RenderFrustumGridTileIndex > 0) && siblingCountHint > 1);
    } else {
        testNeeded = (clipMask != 0 && siblingCountHint > 1);
    }

    if (testNeeded != 0 || (node->flags & kSingleParentFlag) == 0) {
        if ((node->boundsFlags & kNodeBoundsDirtyFlag) != 0 || g_CZClass_RenderBoundsContextActive != 0
            || (node->flags & kSingleParentFlag) == 0) {
            zBBoxCorners corners;
            if ((node->flags & 0x100) == 0) {
                if (altClipReset != 0) {
                    gAltClipPassEnabled = 1;
                }
                return 0;
            }
            gwNodeGetViewBBoxCorners(node, &corners);
            CornersToBoundingSphere(&corners, (zVec3*)node->cachedSphereCenter, (&node->cachedSphereCenter[3]));
            if ((node->flags & kSingleParentFlag) != 0) {
                node->boundsFlags &= ~kNodeBoundsDirtyFlag;
            }
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
        }

        result
            = zVideoFrustumTestSphereClipMask((zVec3*)node->cachedSphereCenter, node->cachedSphereCenter[3], &clipMask);
        if ((node->flags & 0x80) != 0) {
            if (result == 0x20) {
                result = 0;
            }
            clipMask &= ~0x20;
        }
    }
    if (result == 0) {
        int matrixPushed;
        int pushedVertexAlpha;
        int pushedAlphaScale;
        int pushedSoftwareState;
        int visibleByProjectedSphere;
        if ((data->flags & 0x08) == 0) {
            matrixPushed = 1;
            if ((node->flags & kSingleParentFlag) != 0) {
                if ((data->flags & kObject3DTransformDirtyFlag) != 0) {
                    MatStackPushAndCloneParent(data->cachedWorldMatrix);
                    MatMultiply((const zMat4x3*)data->localMatrix, 3);
                    data->flags &= ~kObject3DTransformDirtyFlag;
                    if (g_CZClass_RenderBoundsContextActive == 0) {
                        boundsContextPushed = 1;
                        g_CZClass_RenderBoundsContextActive = 1;
                    }
                } else {
                    MatStackPushPtr(data->cachedWorldMatrix);
                }
            } else {
                MatStackPushAndCloneParent(data->cachedWorldMatrix);
                MatMultiply((const zMat4x3*)data->localMatrix, 3);
                if (g_CZClass_RenderBoundsContextActive == 0) {
                    boundsContextPushed = 1;
                    g_CZClass_RenderBoundsContextActive = 1;
                }
            }
        } else {
            matrixPushed = 0;
        }

        if ((node->flags & 0x00800000) != 0 && g_CZClass_RenderVertexAlphaOverrideActive == 0) {
            pushedVertexAlpha = 1;
            g_CZClass_RenderVertexAlphaOverrideActive = 1;
            zModelRenderVertexAlphaEnabledSetCurrent(1);
        } else {
            pushedVertexAlpha = 0;
        }

        if ((data->flags & 0x02) != 0) {
            pushedAlphaScale = 1;
            ++g_CZClass_RenderAlphaScaleStackTop;
            g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop] = data->alphaScale;
            zModelRenderAlphaScaleSetCurrent(data->alphaScale);
        } else {
            pushedAlphaScale = 0;
        }

        if ((data->flags & 0x04) != 0) {
            float colorAlpha;
            pushedSoftwareState = 1;
            ++g_CZClass_SoftwarePathStateStackTop;
            g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].color = data->color;
            g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].alpha = colorAlpha
                = data->colorAlpha;
            zModelFogTargetColorOverrideSetCurrent(
                &g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].color,
                colorAlpha
            );
        } else {
            pushedSoftwareState = 0;
        }

        if (g_CZClass_ObjectHseTestEnabled != 0 && g_CZClass_RenderFrustumGridTileIndex > 0 && siblingCountHint != 1
            && g_CZClass_RenderVertexAlphaOverrideActive == 0) {
            visibleByProjectedSphere
                = TestProjectedSphereVisible((zVec3*)node->cachedSphereCenter, node->cachedSphereCenter[3]);
        } else {
            visibleByProjectedSphere = 1;
        }
        if (visibleByProjectedSphere != 0) {
            zDiPartial* di;
            node->flags |= 0x80000000;
            di = (zDiPartial*)(unsigned int)node->userDataOrDiRef;
            if (di != 0) {
                if (g_CZClass_RenderRangeFadeActive != 0) {
                    di->flags |= 0x08;
                    ((zDiPartial*)(unsigned int)node->userDataOrDiRef)->blendScale = g_CZClass_RenderRangeFadeScale;
                }
                gModel_RenderFn(node, clipMask);
            }
            if (node->listCountB > 0) {
                int i;
                ++gModel_ClipMaskStackTop;
                *gModel_ClipMaskStackTop = clipMask;
                for (i = 0; i < node->listCountB; ++i) {
                    CZNodePartial* child = node->listB[i];
                    if (child != 0) {
                        if (child->classId == kZClassNodeObject3D) {
                            if (CurrentAllowsId(child->nodeType) != 0) {
                                CZObject3DRenderTraverse(node->listB[i], node->listCountB);
                            }
                        } else {
                            gwNodeRenderDispatch(child, node->listCountB);
                        }
                    }
                }
                --gModel_ClipMaskStackTop;
            }
        }

        if (pushedVertexAlpha != 0) {
            g_CZClass_RenderVertexAlphaOverrideActive = 0;
            zModelRenderVertexAlphaEnabledSetCurrent(0);
        }

        if (pushedAlphaScale != 0) {
            --g_CZClass_RenderAlphaScaleStackTop;
            if (g_CZClass_RenderAlphaScaleStackTop < 0) {
                zModelRenderAlphaScaleSetCurrent(1.0f);
            } else {
                zModelRenderAlphaScaleSetCurrent(g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop]);
            }
        }

        if (pushedSoftwareState != 0) {
            --g_CZClass_SoftwarePathStateStackTop;
            if (g_CZClass_SoftwarePathStateStackTop < 0) {
                zModelFogTargetColorOverrideSetCurrent(0, 0.0f);
            } else {
                zModelFogTargetColorOverrideSetCurrent(
                    &g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].color,
                    g_CZClass_SoftwarePathRenderStateStack[g_CZClass_SoftwarePathStateStackTop].alpha
                );
            }
        }
        if (matrixPushed != 0) {
            MatStackPopPtr();
        }
    }

    if (boundsContextPushed != 0) {
        g_CZClass_RenderBoundsContextActive = 0;
    }
    if (altClipReset != 0) {
        gAltClipPassEnabled = 1;
    }
    return result;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44b710
 * @recoil-artifact defines .text recoil:function:0x44b710: CZAnimateRenderTraverse
 * @recoil-match byte
 *
 * Purpose: cull an animate node, push its animated transform when active,
 * render the node and children, and restore traversal state.
 */
CZAnimateRenderTraverse(CZNodePartial * node, int siblingCountHint)
{
    int boundsContextPushed = 0;
    const int flags = node->flags;
    CZAnimateDataPartial* data;
    int clipMask;
    int result;
    if ((flags & 0x04) == 0) {
        return 0;
    }

    node->flags = flags & ~0x02000000;
    data = (CZAnimateDataPartial*)(node->classData);

    clipMask = *gModel_ClipMaskStackTop;
    result = 0;
    if ((clipMask != 0 && siblingCountHint > 1) || (node->flags & 0x00080000) == 0) {
        if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0
            || (node->flags & 0x00080000) == 0) {
            zBBoxCorners corners;
            gwNodeGetViewBBoxCorners(node, &corners);
            CornersToBoundingSphere(&corners, (zVec3*)node->cachedSphereCenter, &node->cachedSphereCenter[3]);
            if ((node->flags & 0x00080000) != 0) {
                node->boundsFlags &= ~0x04;
            }
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
        }
        result
            = zVideoFrustumTestSphereClipMask((zVec3*)node->cachedSphereCenter, node->cachedSphereCenter[3], &clipMask);
        if ((node->flags & 0x80) != 0) {
            if (result == 0x20) {
                result = 0;
            }
            clipMask &= ~0x20;
        }
    }

    if (result == 0) {
        int matrixPushed;
        node->flags |= 0x80000000;
        if ((data->statusFlags & 0x04) != 0) {
            MatStackPushAndCloneParent(data->savedParentMatrix);
            MatMultiply((const zMat4x3*)data->animatedTransform, 3);
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
            matrixPushed = 1;
        } else {
            matrixPushed = 0;
        }
        if (node->userDataOrDiRef != 0) {
            if (g_CZClass_RenderRangeFadeActive != 0) {
                ((zDiPartial*)(unsigned int)node->userDataOrDiRef)->flags |= 0x08;
                ((zDiPartial*)(unsigned int)node->userDataOrDiRef)->blendScale = g_CZClass_RenderRangeFadeScale;
            }
            gModel_RenderFn(node, clipMask);
        }
        if (node->listCountB > 0) {
            int i;
            ++gModel_ClipMaskStackTop;
            *gModel_ClipMaskStackTop = clipMask;
            for (i = 0; i < node->listCountB; ++i) {
                gwNodeRenderDispatch(node->listB[i], node->listCountB);
            }
            --gModel_ClipMaskStackTop;
        }
        if (matrixPushed != 0) {
            MatStackPopPtr();
        }
    }

    if (boundsContextPushed != 0) {
        g_CZClass_RenderBoundsContextActive = 0;
    }
    return result;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44b8c0
 * @recoil-artifact defines .text recoil:function:0x44b8c0: CZLodRenderTraverse
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zclass.camera.vector-length-sq
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zclass.camera.fast-sqrt-estimate
 * @recoil-match byte
 *
 * Purpose: cull and render an LOD node, applying range, scale, alpha, and
 * vertex-alpha fades while maintaining the render traversal stacks.
 */
CZLodRenderTraverse(CZNodePartial * node, int siblingCountHint)
{
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float scaleZ = 1.0f;
    int pushScaleMatrix = 0;
    int pushAlphaScale = 0;
    int boundsContextPushed = 0;
    zBBoxCorners corners;
    zVec3 delta;
    const int flags = node->flags;
    CZLodDataPartial* data;
    float distance;
    float nearRange;
    float alphaScale;
    int clipMask;
    int result;
    if ((flags & 0x04) == 0) {
        return 0;
    }

    data = (CZLodDataPartial*)(node->classData);
    node->flags = flags & ~0x02000000;
    if (data->computeOwnDistance == 0
        && (g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].distanceSq < data->nearRangeSq
            || g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].distanceSq >= data->farRangeSq)) {
        return 0;
    }

    if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0
        || (node->flags & 0x00080000) == 0) {
        gwNodeGetViewBBoxCorners(node, &corners);
        CornersToBoundingSphere(&corners, zClassNodeViewSphereCenter(node), zClassNodeViewSphereRadius(node));
        if ((node->flags & 0x00080000) != 0) {
            node->boundsFlags &= ~0x04;
        }
        if (g_CZClass_RenderBoundsContextActive == 0) {
            boundsContextPushed = 1;
            g_CZClass_RenderBoundsContextActive = 1;
        }
    }
    if (data->computeOwnDistance != 0) {
        g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].center = *zClassNodeViewSphereCenter(node);
        Vec3Subtract(
            &g_zVideo_pActiveViewContext->cameraPos,
            &g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].center,
            &delta
        );
        g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].distanceSq = Vec3LengthSq(&delta);
        g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].distanceSq
            *= g_zVideo_pActiveViewContext->invClipDistanceSq;
        if (g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].distanceSq < data->nearRangeSq
            || g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].distanceSq >= data->farRangeSq) {
            if (boundsContextPushed != 0) {
                g_CZClass_RenderBoundsContextActive = 0;
            }
            return 0;
        }
    }

    distance = FastSqrt(g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].distanceSq);
    nearRange = data->nearRange;
    if (distance > nearRange) {
        distance = nearRange;
    }

    if (data->fadeAmount.x > 0.01f) {
        const float fadeWidth = data->fadeWidth.x;
        const float fadeBegin = nearRange - fadeWidth;
        pushScaleMatrix = 1;
        scaleX
            = distance > fadeBegin ? 1.0f - (fadeBegin - distance) * (data->fadeEndScale.x - 1.0f) / fadeWidth : 1.0f;
        if (data->active != 0) {
            scaleY = scaleX;
            scaleZ = scaleX;
        }
    }
    if (data->active == 0) {
        if (data->fadeAmount.y > 0.01f) {
            const float fadeWidth = data->fadeWidth.y;
            const float fadeBegin = nearRange - fadeWidth;
            pushScaleMatrix = 1;
            scaleY = distance > fadeBegin ? 1.0f - (fadeBegin - distance) * (data->fadeEndScale.y - 1.0f) / fadeWidth
                                          : 1.0f;
        }
        if (data->fadeAmount.z > 0.01f) {
            const float fadeWidth = data->fadeWidth.z;
            const float fadeBegin = nearRange - fadeWidth;
            pushScaleMatrix = 1;
            scaleZ = distance > fadeBegin ? 1.0f - (fadeBegin - distance) * (data->fadeEndScale.z - 1.0f) / fadeWidth
                                          : 1.0f;
        }
    }

    alphaScale = 1.0f;
    if (data->vertexShadingAmount > 0.01f) {
        const float fogStartDist = data->fogStartDist;
        if (distance > nearRange - fogStartDist) {
            alphaScale = (nearRange - distance) / fogStartDist;
            pushAlphaScale = 1;
        }
    }
    if (data->fogFadeAmount > 0.01f) {
        const float fogFadeWidth = data->fogFadeWidth;
        const float nearDistance = FastSqrt(data->nearRangeSq);
        if (distance < nearDistance) {
            distance = nearDistance;
        }
        if (distance < nearDistance + fogFadeWidth) {
            const float fogScale = (distance - nearDistance) / fogFadeWidth;
            if (fogScale < alphaScale) {
                alphaScale = fogScale;
                pushAlphaScale = 1;
            }
        }
    }

    clipMask = *gModel_ClipMaskStackTop;
    result = 0;
    if (clipMask != 0 && siblingCountHint > 1) {
        result = zVideoFrustumTestSphereClipMask(
            zClassNodeViewSphereCenter(node),
            *zClassNodeViewSphereRadius(node),
            &clipMask
        );
        if ((node->flags & 0x80) != 0) {
            if (result == 0x20) {
                result = 0;
            }
            clipMask &= ~0x20;
        }
    }

    if (result == 0) {
        node->flags |= 0x80000000;
        ++gModel_ClipMaskStackTop;
        *gModel_ClipMaskStackTop = clipMask;

        if (data->rangeNode != 0) {
            const float fadeBegin = data->farRangeSq - data->rangeSq;
            if (fadeBegin < g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].distanceSq) {
                g_CZClass_RenderRangeFadeActive = 1;
                g_CZClass_RenderRangeFadeScale
                    = (g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop].distanceSq - fadeBegin)
                    / (data->farRangeSq - fadeBegin);
            } else {
                g_CZClass_RenderRangeFadeActive = 1;
                g_CZClass_RenderRangeFadeScale = 0.0f;
            }
        }

        if (node->listCountB > 0) {
            zMat4x3 slotBuffer;
            int pushedVertexAlpha;
            int i;
            ++g_CZClass_LodDistanceStateStackTop;
            g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop]
                = g_CZClass_LodDistanceStateStack[g_CZClass_LodDistanceStateStackTop - 1];

            if (pushScaleMatrix != 0) {
                MatStackPushAndCloneParent((float*)&slotBuffer);
                zMath_Mat_Scale(scaleX, scaleY, scaleZ);
            }
            if (pushAlphaScale != 0) {
                ++g_CZClass_RenderAlphaScaleStackTop;
                g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop] = alphaScale;
                zModelRenderAlphaScaleSetCurrent(alphaScale);
            }

            if ((node->flags & 0x00800000) != 0 && g_CZClass_RenderVertexAlphaOverrideActive == 0) {
                pushedVertexAlpha = 1;
                g_CZClass_RenderVertexAlphaOverrideActive = 1;
                zModelRenderVertexAlphaEnabledSetCurrent(1);
            } else {
                pushedVertexAlpha = 0;
            }

            for (i = 0; i < node->listCountB; ++i) {
                gwNodeRenderDispatch(node->listB[i], node->listCountB);
            }

            if (pushScaleMatrix != 0) {
                MatStackPopPtr();
            }
            if (pushAlphaScale != 0) {
                --g_CZClass_RenderAlphaScaleStackTop;
                if (g_CZClass_RenderAlphaScaleStackTop < 0) {
                    zModelRenderAlphaScaleSetCurrent(1.0f);
                } else {
                    zModelRenderAlphaScaleSetCurrent(
                        g_CZClass_RenderAlphaScaleStack[g_CZClass_RenderAlphaScaleStackTop]
                    );
                }
            }
            if (pushedVertexAlpha != 0) {
                g_CZClass_RenderVertexAlphaOverrideActive = 0;
                zModelRenderVertexAlphaEnabledSetCurrent(0);
            }
            --g_CZClass_LodDistanceStateStackTop;
        }
        g_CZClass_RenderRangeFadeActive = 0;
        --gModel_ClipMaskStackTop;
    }

    if (boundsContextPushed != 0) {
        g_CZClass_RenderBoundsContextActive = 0;
    }
    return result;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44bea0
 * @recoil-artifact defines .text recoil:function:0x44bea0: CZSequenceRenderTraverse
 * @recoil-match byte
 *
 * Purpose: cull an active sequence node, push traversal state, and render
 * only the currently selected child entry.
 */
CZSequenceRenderTraverse(CZNodePartial * node, int siblingCountHint)
{
    int boundsContextPushed = 0;
    const int flags = node->flags;
    CZSequenceDataPartial* data;
    int clipMask;
    int result;
    if ((flags & 0x04) == 0) {
        return 0;
    }

    data = (CZSequenceDataPartial*)(node->classData);
    node->flags = flags & ~0x02000000;
    if (data->isActive == 0) {
        return 0;
    }

    clipMask = *gModel_ClipMaskStackTop;
    result = 0;
    if (clipMask != 0 && siblingCountHint > 1) {
        if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0) {
            zBBoxCorners corners;
            gwNodeGetViewBBoxCorners(node, &corners);
            CornersToBoundingSphere(&corners, (zVec3*)node->cachedSphereCenter, &node->cachedSphereCenter[3]);
            node->boundsFlags &= ~0x04;
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
        }

        result
            = zVideoFrustumTestSphereClipMask((zVec3*)node->cachedSphereCenter, node->cachedSphereCenter[3], &clipMask);
        if ((node->flags & 0x80) != 0) {
            if (result == 0x20) {
                result = 0;
            }
            clipMask &= ~0x20;
        }
    }

    if (result == 0) {
        node->flags |= 0x80000000;
        ++gModel_ClipMaskStackTop;
        *gModel_ClipMaskStackTop = clipMask;
        gwNodeRenderDispatch(data->entries[data->currentIndex].node, node->listCountB);
        --gModel_ClipMaskStackTop;
    }

    if (boundsContextPushed != 0) {
        g_CZClass_RenderBoundsContextActive = 0;
    }
    return result;
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.rendertraverse-44bfb0
 * @recoil-artifact defines .text recoil:function:0x44bfb0: CZSwitchRenderTraverse
 * @recoil-match byte
 *
 * Purpose: cull the switch node, push the clip mask, and render only the
 * active child-mask entries.
 */
CZSwitchRenderTraverse(CZNodePartial * node, int siblingCountHint)
{
    int boundsContextPushed = 0;
    const int flags = node->flags;
    CZSwitchDataPartial* data;
    int clipMask;
    int result;
    if ((flags & 0x04) == 0) {
        return 0;
    }

    data = (CZSwitchDataPartial*)(node->classData);
    node->flags = flags & ~0x02000000;
    clipMask = *gModel_ClipMaskStackTop;
    result = 0;
    if (clipMask != 0 && siblingCountHint > 1) {
        if ((node->boundsFlags & 0x04) != 0 || g_CZClass_RenderBoundsContextActive != 0) {
            zBBoxCorners corners;
            gwNodeGetViewBBoxCorners(node, &corners);
            CornersToBoundingSphere(&corners, (zVec3*)node->cachedSphereCenter, &node->cachedSphereCenter[3]);
            node->boundsFlags &= ~0x04;
            if (g_CZClass_RenderBoundsContextActive == 0) {
                boundsContextPushed = 1;
                g_CZClass_RenderBoundsContextActive = 1;
            }
        }

        result
            = zVideoFrustumTestSphereClipMask((zVec3*)node->cachedSphereCenter, node->cachedSphereCenter[3], &clipMask);
        if ((node->flags & 0x80) != 0) {
            if (result == 0x20) {
                result = 0;
            }
            clipMask &= ~0x20;
        }
    }

    if (result == 0) {
        unsigned int activeMask;
        int i;
        node->flags |= 0x80000000;
        ++gModel_ClipMaskStackTop;
        *gModel_ClipMaskStackTop = clipMask;
        activeMask = data->childMasks[data->activeMaskIndex];
        for (i = 0; i < node->listCountB; ++i) {
            if (((activeMask >> i) & 1U) != 0) {
                gwNodeRenderDispatch(node->listB[i], node->listCountB);
            }
        }
        --gModel_ClipMaskStackTop;
    }

    if (boundsContextPushed != 0) {
        g_CZClass_RenderBoundsContextActive = 0;
    }
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwnoderenderdispatch
 * @recoil-artifact defines .text recoil:function:0x44c0e0: CZClass::gwNodeRenderDispatch.
 * @recoil-match byte
 *
 * Purpose: route visible scene nodes to the class-specific render
 * traversal after variant-tag filtering.
 */
int __fastcall gwNodeRenderDispatch(CZNodePartial* node, int siblingCountHint)
{
    int result = CurrentAllowsId(node->nodeType);
    if (result != 0) {
        switch (node->classId - 1) {
        case 4:
            return CZObject3DRenderTraverse(node, siblingCountHint);
        case 5:
            return CZLodRenderTraverse(node, siblingCountHint);
        case 8:
            return CZLightRenderTraverse(node, siblingCountHint);
        case 9:
            return CZSoundRenderTraverse(node, siblingCountHint);
        case 0:
            return CZCameraRenderTraverse(node, siblingCountHint);
        case 7:
            return CZAnimateRenderTraverse(node, siblingCountHint);
        case 6:
            return CZSequenceRenderTraverse(node, siblingCountHint);
        case 10:
            return CZSwitchRenderTraverse(node, siblingCountHint);
        default:
            result = fprintf(stderr, "Unrecognized node rendering type: %s\n", node->name);
            break;
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.fastanglexz
 * @recoil-artifact defines .text recoil:function:0x44c1b0: CZCamera::theta_x_z.
 * @recoil-match byte
 *
 * Purpose: approximate the XZ-plane angle between two points.
 */
float __fastcall theta_x_z(zVec3* point1, zVec3* point2)
{
    const int deltaX = (int)(point2->x - point1->x);
    // abs() intrinsic: retail forms |dx| and |dz| with cdq/xor/sub (0x44c1cc, 0x44c1da); the
    // twice-used |dx| local is formed before the second __ftol and held in ESI, as in retail.
    const int absDeltaX = abs(deltaX);
    const int deltaZ = (int)(point1->z - point2->z);
    const int absDeltaZ = abs(deltaZ);

    float angle;
    if (absDeltaX + absDeltaZ == 0) {
        angle = 0.0f;
    } else {
        angle = (float)(deltaZ) / (float)(absDeltaX + absDeltaZ);
    }

    if (deltaX < 0) {
        return (2.0f - angle) * 1.57079601f;
    }

    if (deltaZ < 0) {
        angle -= -4.0f;
    }

    return angle * 1.57079601f;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.findconvexhullxz
 * @recoil-artifact defines .text recoil:function:0x44c230: CZCamera::find_convex_hull_xz.
 * @recoil-match byte
 *
 * Purpose: build the XZ convex hull ordering for frustum footprint points.
 */
int __fastcall find_convex_hull_xz(zVec3* points, int count)
{
    int selectedIndex = 0;
    int candidateIndex;
    float previousAngle;
    int scanStart;
    int hullIndex;
    for (candidateIndex = 1; candidateIndex < count; ++candidateIndex) {
        if (points[candidateIndex].z - points[selectedIndex].z > 0.1) {
            selectedIndex = candidateIndex;
        }
    }

    previousAngle = 0.0f;
    points[count] = points[selectedIndex];

    scanStart = 1;
    for (hullIndex = 0; hullIndex < count; ++hullIndex) {
        zVec3* const hullPoint = &points[hullIndex];
        const zVec3 savedPoint = *hullPoint;
        float minAngle;
        int scanIndex;
        *hullPoint = points[selectedIndex];
        points[selectedIndex] = savedPoint;
        selectedIndex = count;

        minAngle = previousAngle;
        previousAngle = 6.28318548f;

        for (scanIndex = scanStart; scanIndex <= count; ++scanIndex) {
            const float angle = theta_x_z(hullPoint, &points[scanIndex]);
            if (angle > minAngle && angle < previousAngle) {
                previousAngle = angle;
                selectedIndex = scanIndex;
            }
        }

        if (selectedIndex == count) {
            return hullIndex + 1;
        }

        ++scanStart;
    }

    ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zClass\\Camera.c", 0x1049, g_CZClass_FindConvexHullUnexpectedReturnMsg);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.buildfrustumgridtiles
 * @recoil-artifact defines .text recoil:function:0x44c3c0: CZCamera::BuildFrustumGridTiles
 *
 *
 * Purpose: build clamped in-world frustum grid rings from the active
 * camera footprint.
 */
int __fastcall
BuildFrustumGridTiles(CZNodePartial* world, CZWorldDataPartial* worldData, CZCameraDataPartial* cameraData)
{
    int ringIndex;
    GridCell origin;
    int result;
    zMat4x3 slotBuffer;
    zVec2 boundsMin;
    zVec2 boundsMax;
    const zVec3* point;
    int i;
    GridCell minCell;
    GridCell maxCell;
    int areaIndex;
    int col;
    for (ringIndex = 0; ringIndex < 50; ++ringIndex) {
        g_zCamera_FrustumGridTileRings[ringIndex].count = 0;
    }

    // Grid queries fill (col, row) pairs and the footprint bounds are X/Z pairs, as in the
    // sibling BuildFrustumGridTilesFromParams (retail keeps each pair in adjacent dwords).
    result
        = WorldToGridCoordsClamped(world, cameraData->cameraPos.x, cameraData->cameraPos.z, &origin.col, &origin.row);
    if (result != 0) {
        return result;
    }

    MatStackPushPtr((float*)&slotBuffer);
    MatLoadIdentity();
    MatTranslate(cameraData->cameraPos.x, cameraData->cameraPos.y, cameraData->cameraPos.z);
    MatRotateY(cameraData->eulerAngles.y);

    if (fabs(cameraData->eulerAngles.x) <= 0.174533 && fabs(cameraData->eulerAngles.z) <= 0.174533) {
        g_zCamera_FrustumFootprintPointCount = 3;
    } else {
        g_zCamera_FrustumFootprintPointCount = 5;
        MatRotateX(cameraData->eulerAngles.x);
        MatRotateZ(cameraData->eulerAngles.z);
    }
    ZMTH_MAT_TRANSFORM_POINT_BATCH(
        &cameraData->frustumOrigin,
        g_zCamera_FrustumFootprintPoints,
        g_zCamera_FrustumFootprintPointCount
    );
    if (g_zCamera_FrustumFootprintPointCount > 3) {
        g_zCamera_FrustumFootprintPointCount
            = find_convex_hull_xz(g_zCamera_FrustumFootprintPoints, g_zCamera_FrustumFootprintPointCount);
    }
    if (FilterRegionsAgainstMeshFaces(g_zCamera_FrustumFootprintPoints, g_zCamera_FrustumFootprintPointCount) == 0) {
        sprintf(
            g_zError_DebugMsgBuffer,
            g_CZClass_LineErrorPointInPolygonInitCameraFrustumFmt,
            "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
            0x10ea
        );
        EmitDebugBuffer(1);
    }

    // Retail seeds the bounds from the first two footprint points, then scans from the third.
    boundsMin.x = g_zCamera_FrustumFootprintPoints[0].x < g_zCamera_FrustumFootprintPoints[1].x
        ? g_zCamera_FrustumFootprintPoints[0].x
        : g_zCamera_FrustumFootprintPoints[1].x;
    boundsMax.x = g_zCamera_FrustumFootprintPoints[0].x > g_zCamera_FrustumFootprintPoints[1].x
        ? g_zCamera_FrustumFootprintPoints[0].x
        : g_zCamera_FrustumFootprintPoints[1].x;
    boundsMin.y = g_zCamera_FrustumFootprintPoints[0].z < g_zCamera_FrustumFootprintPoints[1].z
        ? g_zCamera_FrustumFootprintPoints[0].z
        : g_zCamera_FrustumFootprintPoints[1].z;
    boundsMax.y = g_zCamera_FrustumFootprintPoints[0].z > g_zCamera_FrustumFootprintPoints[1].z
        ? g_zCamera_FrustumFootprintPoints[0].z
        : g_zCamera_FrustumFootprintPoints[1].z;
    point = &g_zCamera_FrustumFootprintPoints[2];
    for (i = 2; i < g_zCamera_FrustumFootprintPointCount; ++i, ++point) {
        if (point->x < boundsMin.x) {
            boundsMin.x = point->x;
        }
        if (point->x > boundsMax.x) {
            boundsMax.x = point->x;
        }
        if (point->z < boundsMin.y) {
            boundsMin.y = point->z;
        }
        if (point->z > boundsMax.y) {
            boundsMax.y = point->z;
        }
    }

    result = WorldToGridCoordsClamped(world, boundsMin.x, boundsMin.y, &minCell.col, &minCell.row);
    if (result != 0) {
        MatStackPopPtr();
        return result;
    }

    result = WorldToGridCoordsClamped(world, boundsMax.x, boundsMax.y, &maxCell.col, &maxCell.row);
    if (result != 0) {
        MatStackPopPtr();
        return result;
    }

    if (minCell.row > maxCell.row) {
        const int savedRow = minCell.row;
        minCell.row = maxCell.row;
        maxCell.row = savedRow;
    }

    if (minCell.col > worldData->areaGridColCount - 1) {
        minCell.col = worldData->areaGridColCount - 1;
    } else if (minCell.col < 0) {
        minCell.col = 0;
    }
    if (maxCell.col > worldData->areaGridColCount - 1) {
        maxCell.col = worldData->areaGridColCount - 1;
    } else if (maxCell.col < 0) {
        maxCell.col = 0;
    }
    if (minCell.row > worldData->areaGridRowCount - 1) {
        minCell.row = worldData->areaGridRowCount - 1;
    } else if (minCell.row < 0) {
        minCell.row = 0;
    }
    if (maxCell.row > worldData->areaGridRowCount - 1) {
        maxCell.row = worldData->areaGridRowCount - 1;
    } else if (maxCell.row < 0) {
        maxCell.row = 0;
    }

    areaIndex = worldData->areaGridRows[origin.row][origin.col].areaIndex;
    for (col = minCell.col; col <= maxCell.col; ++col) {
        int row;
        for (row = minCell.row; row <= maxCell.row; ++row) {
            zWorldAreaPartial* area = &worldData->areaGridRows[row][col];
            zVec3 center;
            int clipMask;
            int frustumResult;
            if ((area->areaIndex & areaIndex) == 0) {
                continue;
            }

            center.x = area->cellMinX + worldData->areaHalfSizeX;
            center.y = 0.0f;
            center.z = area->cellMinZ + worldData->areaHalfSizeZ;
            if (FilterRegionsAgainstHexahedronFaces(&center, worldData->areaCellRadiusBias) == 0) {
                continue;
            }

            clipMask = 0x3f;
            if ((area->areaFlags & 0x100) != 0) {
                frustumResult = zVideoFrustumTestSphereClipMask(&area->bboxCenter, area->bboxRadius, &clipMask);
            } else {
                frustumResult = zVideoFrustumTestSphereClipMask(&center, -worldData->areaCellRadiusBias, &clipMask);
            }

            if (frustumResult == 0) {
                const int ringIndex = abs(col - origin.col) + abs(row - origin.row);
                if (ringIndex < 50) {
                    const int tileIndex = g_zCamera_FrustumGridTileRings[ringIndex].count;
                    if (tileIndex < 30) {
                        // Retail writes only col/row/clip mask and clears hasPosOffset here.
                        g_zCamera_FrustumGridTileRings[ringIndex].count = tileIndex + 1;
                        g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].col = col;
                        g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].row = row;
                        g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].clipMask = clipMask;
                        g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].hasPosOffset = 0;
                    } else {
                        ReportOld(
                            0x200,
                            "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                            0x11a4,
                            g_CZClass_DiamondTilerNeedMoreCellsPerRingMsg
                        );
                    }
                } else {
                    ReportOld(
                        0x200,
                        "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                        0x11aa,
                        g_CZClass_DiamondTilerNeedMoreRingsMsg
                    );
                }
            }
        }
    }

    MatStackPopPtr();
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.buildfrustumgridtilesfromparams
 * @recoil-artifact defines .text recoil:function:0x44c8e0: CZCamera::BuildFrustumGridTilesFromParams
 * @recoil-match source
 *
 * Purpose: build frustum grid rings while preserving raw out-of-bounds
 * grid offsets for wrapped/clamped world positions.
 */
int __fastcall
BuildFrustumGridTilesFromParams(CZNodePartial* world, CZWorldDataPartial* worldData, CZCameraDataPartial* cameraData)
{
    int ringIndex;
    int insideBounds;
    int clipMask;
    int row;
    int col;
    GridCell origin;
    GridCell originClamped;
    int result;
    zMat4x3 slotBuffer;
    zVec2 boundsMin;
    zVec2 boundsMax;
    const zVec3* point;
    int i;
    GridCell minCell;
    GridCell minClamped;
    GridCell maxCell;
    GridCell maxClamped;
    int areaIndex;
    for (ringIndex = 0; ringIndex < 50; ++ringIndex) {
        g_zCamera_FrustumGridTileRings[ringIndex].count = 0;
    }

    // Retail passes one shared inside-bounds out slot to all three queries.
    // Function-scope locals: retail gives the address-taken clip mask its own home.
    result = WorldToGridCoordsClampedEx(
        world,
        cameraData->cameraPos.x,
        cameraData->cameraPos.z,
        &origin.col,
        &origin.row,
        &originClamped.col,
        &originClamped.row,
        &insideBounds
    );
    if (result != 0) {
        return result;
    }

    MatStackPushPtr((float*)&slotBuffer);
    MatLoadIdentity();
    MatTranslate(cameraData->cameraPos.x, cameraData->cameraPos.y, cameraData->cameraPos.z);
    MatRotateY(cameraData->eulerAngles.y);

    if (fabs(cameraData->eulerAngles.x) <= 0.174533 && fabs(cameraData->eulerAngles.z) <= 0.174533) {
        g_zCamera_FrustumFootprintPointCount = 3;
    } else {
        g_zCamera_FrustumFootprintPointCount = 5;
        MatRotateX(cameraData->eulerAngles.x);
        MatRotateZ(cameraData->eulerAngles.z);
    }
    ZMTH_MAT_TRANSFORM_POINT_BATCH(
        &cameraData->frustumOrigin,
        g_zCamera_FrustumFootprintPoints,
        g_zCamera_FrustumFootprintPointCount
    );
    if (g_zCamera_FrustumFootprintPointCount > 3) {
        g_zCamera_FrustumFootprintPointCount
            = find_convex_hull_xz(g_zCamera_FrustumFootprintPoints, g_zCamera_FrustumFootprintPointCount);
    }
    if (FilterRegionsAgainstMeshFaces(g_zCamera_FrustumFootprintPoints, g_zCamera_FrustumFootprintPointCount) == 0) {
        sprintf(
            g_zError_DebugMsgBuffer,
            g_CZClass_LineErrorPointInPolygonInitCameraFrustumFmt,
            "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
            0x1279
        );
        EmitDebugBuffer(1);
    }

    // Retail seeds the bounds from the first two footprint points, then scans from the third.
    // Footprint X/Z bounds as pairs: retail keeps min and max X/Z in adjacent dwords.
    boundsMin.x = g_zCamera_FrustumFootprintPoints[0].x < g_zCamera_FrustumFootprintPoints[1].x
        ? g_zCamera_FrustumFootprintPoints[0].x
        : g_zCamera_FrustumFootprintPoints[1].x;
    boundsMax.x = g_zCamera_FrustumFootprintPoints[0].x > g_zCamera_FrustumFootprintPoints[1].x
        ? g_zCamera_FrustumFootprintPoints[0].x
        : g_zCamera_FrustumFootprintPoints[1].x;
    boundsMin.y = g_zCamera_FrustumFootprintPoints[0].z < g_zCamera_FrustumFootprintPoints[1].z
        ? g_zCamera_FrustumFootprintPoints[0].z
        : g_zCamera_FrustumFootprintPoints[1].z;
    boundsMax.y = g_zCamera_FrustumFootprintPoints[0].z > g_zCamera_FrustumFootprintPoints[1].z
        ? g_zCamera_FrustumFootprintPoints[0].z
        : g_zCamera_FrustumFootprintPoints[1].z;
    point = &g_zCamera_FrustumFootprintPoints[2];
    for (i = 2; i < g_zCamera_FrustumFootprintPointCount; ++i, ++point) {
        if (point->x < boundsMin.x) {
            boundsMin.x = point->x;
        }
        if (point->x > boundsMax.x) {
            boundsMax.x = point->x;
        }
        if (point->z < boundsMin.y) {
            boundsMin.y = point->z;
        }
        if (point->z > boundsMax.y) {
            boundsMax.y = point->z;
        }
    }

    // Retail returns these failures directly, leaving the matrix-stack slot pushed.
    result = WorldToGridCoordsClampedEx(
        world,
        boundsMin.x,
        boundsMin.y,
        &minCell.col,
        &minCell.row,
        &minClamped.col,
        &minClamped.row,
        &insideBounds
    );
    if (result != 0) {
        return result;
    }
    result = WorldToGridCoordsClampedEx(
        world,
        boundsMax.x,
        boundsMax.y,
        &maxCell.col,
        &maxCell.row,
        &maxClamped.col,
        &maxClamped.row,
        &insideBounds
    );
    if (result != 0) {
        return result;
    }

    if (minClamped.row > maxClamped.row) {
        const int savedClampedRow = minClamped.row;
        minClamped.row = maxClamped.row;
        maxClamped.row = savedClampedRow;
    }
    if (minCell.row > maxCell.row) {
        const int savedRow = minCell.row;
        minCell.row = maxCell.row;
        maxCell.row = savedRow;
    }

    areaIndex = worldData->areaGridRows[originClamped.row][originClamped.col].areaIndex;
    for (col = minCell.col; col <= maxCell.col; ++col) {
        for (row = minCell.row; row <= maxCell.row; ++row) {
            int hasPosOffset = 0;
            int areaCol = col;
            int areaRow = row;
            zVec2 posOffset;
            zWorldAreaPartial* area;
            zVec3 center;
            int frustumResult;

            if (areaCol < 0) {
                hasPosOffset = 1;
                areaCol = 0;
            } else if (areaCol >= worldData->areaGridColCount) {
                hasPosOffset = 1;
                areaCol = worldData->areaGridColCount - 1;
            }

            if (areaRow < 0) {
                hasPosOffset = 1;
                areaRow = 0;
            } else if (areaRow >= worldData->areaGridRowCount) {
                hasPosOffset = 1;
                areaRow = worldData->areaGridRowCount - 1;
            }

            // X/Z wrap offset pair (retail: adjacent, unshared dwords).
            if (hasPosOffset != 0) {
                posOffset.x = (float)(col - areaCol) * worldData->areaCellSizeX;
                posOffset.y = (float)(row - areaRow) * worldData->areaCellSizeZ;
            } else {
                posOffset.x = 0.0f;
                posOffset.y = 0.0f;
            }

            area = &worldData->areaGridRows[areaRow][areaCol];
            if ((area->areaIndex & areaIndex) == 0) {
                continue;
            }

            center.x = area->cellMinX + worldData->areaHalfSizeX + posOffset.x;
            center.y = 0.0f;
            center.z = area->cellMinZ + worldData->areaHalfSizeZ + posOffset.y;
            if (FilterRegionsAgainstHexahedronFaces(&center, worldData->areaCellRadiusBias) == 0) {
                continue;
            }

            clipMask = 0x3f;
            if (hasPosOffset != 0) {
                frustumResult = 0;
            } else if ((area->areaFlags & 0x100) != 0) {
                frustumResult = zVideoFrustumTestSphereClipMask(&area->bboxCenter, area->bboxRadius, &clipMask);
            } else {
                frustumResult = zVideoFrustumTestSphereClipMask(&center, -worldData->areaCellRadiusBias, &clipMask);
            }

            if (frustumResult == 0) {
                const int ringIndex = abs(row - origin.row) + abs(col - origin.col);
                if (ringIndex < 50) {
                    const int tileIndex = g_zCamera_FrustumGridTileRings[ringIndex].count;
                    if (tileIndex < 30) {
                        g_zCamera_FrustumGridTileRings[ringIndex].count = tileIndex + 1;
                        g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].col = areaCol;
                        g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].row = areaRow;
                        g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].clipMask = clipMask;
                        g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].hasPosOffset = hasPosOffset;
                        // Retail stores the offsets only for wrapped cells.
                        if (hasPosOffset != 0) {
                            g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].posOffsetX = posOffset.x;
                            g_zCamera_FrustumGridTileRings[ringIndex].tiles[tileIndex].posOffsetZ = posOffset.y;
                        }
                    } else {
                        ReportOld(
                            0x200,
                            "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                            0x1351,
                            g_CZClass_DiamondTilerNeedMoreCellsPerRingMsg
                        );
                    }
                } else {
                    ReportOld(
                        0x200,
                        "D:\\Proj\\GameZRecoil\\zClass\\Camera.c",
                        0x1357,
                        g_CZClass_DiamondTilerNeedMoreRingsMsg
                    );
                }
            }
        }
    }

    MatStackPopPtr();
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.renderfrustumgridtiles
 * @recoil-artifact defines .text recoil:function:0x44ce70: CZCamera::RenderFrustumGridTiles.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zclass.camera.vector-length
 *
 *
 * Purpose: render world grid tiles selected by the camera frustum.
 */
void __fastcall RenderFrustumGridTiles(CZNodePartial* world, CZNodePartial* camera, CZCameraDataPartial* cameraData)
{
    int cameraAtBasePos = 1;
    CZWorldDataPartial* worldData = (CZWorldDataPartial*)(world->classData);
    int result;
    int fogWasEnabled;
    float fogDistanceStart;
    int lightIndex;

    if (worldData->clampQueriesToBounds != 0) {
        result = BuildFrustumGridTilesFromParams(world, worldData, cameraData);
    } else {
        result = BuildFrustumGridTiles(world, worldData, cameraData);
    }
    if (result != 0) {
        return;
    }

    fogWasEnabled = zModelFogIsEnabled();
    if (fogWasEnabled != 0) {
        fogDistanceStart = zModelFogGetDistanceStart();
    }

    for (g_CZClass_RenderFrustumGridTileIndex = 0; g_CZClass_RenderFrustumGridTileIndex < 50;
        ++g_CZClass_RenderFrustumGridTileIndex) {
        int tileIndex;
        for (tileIndex = 0; tileIndex < g_zCamera_FrustumGridTileRings[g_CZClass_RenderFrustumGridTileIndex].count;
            ++tileIndex) {
            zCamera_FrustumGridTilePartial* tile
                = &g_zCamera_FrustumGridTileRings[g_CZClass_RenderFrustumGridTileIndex].tiles[tileIndex];
            zWorldAreaPartial* area = &worldData->areaGridRows[tile->row][tile->col];
            zVec3 center = area->bboxCenter;
            zVec3 delta;
            int visible;

            if (tile->hasPosOffset != 0) {
                zVec3 posOffset = { -tile->posOffsetX, 0.0f, -tile->posOffsetZ };
                UpdateImpl(camera, &posOffset);
                cameraAtBasePos = 0;
            } else if (cameraAtBasePos == 0) {
                gwCameraUpdate(camera);
                cameraAtBasePos = 1;
            }

            if (g_CZClass_ObjectHseTestEnabled != 0 && g_CZClass_RenderFrustumGridTileIndex > 0) {
                visible = TestProjectedSphereVisible(&center, area->bboxRadius);
            } else {
                visible = 1;
            }
            if (visible == 0) {
                continue;
            }

            for (lightIndex = 0; lightIndex < worldData->lightCount; ++lightIndex) {
                CZNodePartial* lightNode = worldData->lightNodes[lightIndex];
                CZLightDataPartial* lightData;
                float range;
                float distanceSq;
                if ((lightNode->flags & 0x04) == 0) {
                    continue;
                }

                lightData = worldData->lightDataList[lightIndex];
                if (lightData->isPointSource == 0 || lightData->enabled == 0) {
                    lightData->lightSubMode = 1;
                    continue;
                }

                delta.x = center.x - lightData->worldPosScratch.x;
                delta.y = center.y - lightData->worldPosScratch.y;
                delta.z = center.z - lightData->worldPosScratch.z;
                range = lightData->range2 + area->bboxRadius;
                distanceSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                if (distanceSq >= range * range) {
                    lightData->lightSubMode = 0;
                } else {
                    lightData->lightSubMode = 1;
                }
            }

            if (fogWasEnabled != 0) {
                float distance;
                float fogMargin;
                Vec3Subtract(&center, &cameraData->cameraPos, &delta);
                distance = Vec3Length(&delta);
                // Named margin: retail 'fmul 1.1; fadd distance' (inline folds to 'fmul -1.1; fsubr').
                fogMargin = area->bboxRadius * 1.10000002f;
                distance += fogMargin;
                zModelFogSetEnabled(distance < fogDistanceStart ? 0 : 1);
            }

            *gModel_ClipMaskStackTop = tile->clipMask;
            if (tile->hasPosOffset == 0) {
                int childIndex;
                for (childIndex = 0; childIndex < area->childCount; ++childIndex) {
                    gwNodeRenderDispatch(area->childList[childIndex], area->childCount);
                }
            } else {
                int childIndex;
                for (childIndex = 0; childIndex < area->childCount; ++childIndex) {
                    if (strstr(area->childList[childIndex]->name, g_CZClass_VapStaticsNodeName) != 0) {
                        gwNodeRenderDispatch(area->childList[childIndex], area->childCount);
                    }
                }
            }
        }
    }

    if (cameraAtBasePos == 0) {
        gwCameraUpdate(camera);
    }
    for (lightIndex = 0; lightIndex < worldData->lightCount; ++lightIndex) {
        worldData->lightDataList[lightIndex]->lightSubMode = 1;
    }
    if (fogWasEnabled != 0) {
        zModelFogSetEnabled(fogWasEnabled);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.renderoverlaynodes
 * @recoil-artifact defines .text recoil:function:0x44d200: CZCamera::RenderOverlayNodes.
 * @recoil-match byte
 *
 * Purpose: render overlay child nodes from the world node.
 */
void __fastcall RenderOverlayNodes(CZNodePartial* world)
{
    int i;
    *gModel_ClipMaskStackTop = 0x3f;
    for (i = 0; i < world->listCountB; ++i) {
        gwNodeRenderDispatch(world->listB[i], 2);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.renderworld
 * @recoil-artifact defines .text recoil:function:0x44d240: CZCamera::RenderWorld.
 * @recoil-match byte
 *
 * Purpose: render frustum grid tiles and overlay nodes for the world.
 */
void __fastcall RenderWorld(CZNodePartial* world, CZNodePartial* camera, CZCameraDataPartial* cameraData)
{
    RenderFrustumGridTiles(world, camera, cameraData);
    RenderOverlayNodes(world);
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.gwcamerasetvarianttagoverride
 * @recoil-artifact defines .text recoil:function:0x44d260: CZCamera::gwCameraSetVariantTagOverride.
 * @recoil-match byte
 *
 * Purpose: validate and store the camera variant tag override.
 */
gwCameraSetVariantTagOverride(CZNodePartial * camera, zTag4Partial * variantTag)
{
    int validVariantTag = 1;
    ValidateCameraNode(camera, 0x1527, 0x1528, 0x1529);

    if (variantTag->count > 0) {
        int i;
        for (i = 0; i < variantTag->count; ++i) {
            if (variantTag->tags[i] == 0xff) {
                validVariantTag = 0;
            }
        }

        if (validVariantTag != 0) {
            ((CZCameraDataPartial*)(camera->classData))->variantOverrideEnabled = 1;
            ((CZCameraDataPartial*)(camera->classData))->variantTag = *variantTag;
        }
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.syncviewcontextpositions
 * @recoil-artifact defines .text recoil:function:0x44d320: CZCamera::SyncViewContextPositions.
 * @recoil-match byte
 *
 * Purpose: synchronize horizon helper nodes with the active view context.
 * Preserve the horizon-XZ helper height while updating its X and Z.
 */
void __cdecl SyncViewContextPositions(void)
{
    int updatedAnyNode = 0;

    if (g_zVideo_pActiveViewContext->horizonNode != 0) {
        gwObject3DSetPosition(
            g_zVideo_pActiveViewContext->horizonNode,
            g_zVideo_pActiveViewContext->cameraPos.x,
            g_zVideo_pActiveViewContext->cameraPos.y,
            g_zVideo_pActiveViewContext->cameraPos.z
        );
        updatedAnyNode = 1;
    }

    if (g_zVideo_pActiveViewContext->horizonXZNode != 0) {
        float horizonX;
        float preservedY;
        float horizonZ;
        gwObject3DGetPosition(g_zVideo_pActiveViewContext->horizonXZNode, &horizonX, &preservedY, &horizonZ);
        gwObject3DSetPosition(
            g_zVideo_pActiveViewContext->horizonXZNode,
            g_zVideo_pActiveViewContext->cameraPos.x,
            preservedY,
            g_zVideo_pActiveViewContext->cameraPos.z
        );
        updatedAnyNode = 1;
    }

    if (updatedAnyNode != 0) {
        gwNodeUpdateAll();
    }
}

int __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.renderscene
 * @recoil-artifact defines .text recoil:function:0x44d3a0: CZCamera::RenderScene.
 *
 *
 * Purpose: update camera scene state and render the active world.
 */
RenderScene(CZNodePartial * camera, int updateFxPass3Local)
{
    const int queuedLensFlareSampleCount = zRndrLensFlareGetQueuedSampleCount();
    zMat4x3 slotBuffer;
    CZNodePartial* world;
    CZWindowDataPartial* windowData;
    int variantFilterEnabled;
    MatStackPushPtr((float*)&slotBuffer);

    g_zVideo_pActiveViewContext = (CZCameraDataPartial*)(camera->classData);
    world = gwCameraGetWorld(camera);
    windowData = (CZWindowDataPartial*)(g_zVideo_pActiveViewContext->windowNode->classData);

    if (g_CZClass_CameraAutoClipDistanceAdjustEnabled != 0) {
        const float scale = g_CZClass_CameraAutoClipDistanceScale;
        if (g_FrameDeltaTimeSec > g_CZClass_CameraAutoClipDistanceThreshold) {
            g_CZClass_CameraAutoClipDistanceScale = scale - g_CZClass_CameraAutoClipDistanceStep;
        } else {
            g_CZClass_CameraAutoClipDistanceScale = scale + g_CZClass_CameraAutoClipDistanceStep;
        }

        if (g_CZClass_CameraAutoClipDistanceScale > 1.0f) {
            g_CZClass_CameraAutoClipDistanceScale = 1.0f;
        } else if (g_CZClass_CameraAutoClipDistanceScale < g_CZClass_CameraAutoClipDistanceMinScale) {
            g_CZClass_CameraAutoClipDistanceScale = g_CZClass_CameraAutoClipDistanceMinScale;
        }

        gwCameraSetClipDistance(camera, g_CZClass_CameraAutoClipDistanceScale);
    }

    InitLightPointInPolygonXZ(world);
    BindWorldNode(world);
    gwCameraUpdate(camera);
    SyncViewContextPositions();
    zVideoSetActiveViewContext(g_zVideo_pActiveViewContext);
    UpdateAllLights(world);
    UpdateAllSounds(world);

    g_CZClass_LodDistanceStateStackTop = 0;
    if (CountNodes(8) > 1) {
        SpanOcclusionResetFrame();
        if ((windowData->clearPolyIndexFlags & 0x80000000) != 0) {
            unsigned int i;
            for (i = 0; i < (unsigned int)(windowData->clearPolyIndexFlags & 0x7fffffff); ++i) {
                if ((windowData->clearPolys[i].vertCount & 0x80000000) != 0) {
                    SpanOcclusionAddPolygon(
                        windowData->clearPolys[i].vertices,
                        windowData->clearPolys[i].vertCount & 0x7fffffff
                    );
                }
            }
        }
    }
    SpanOcclusionBuildColumnHeadTable();

    variantFilterEnabled = g_Variant_FilterEnabled;
    if (variantFilterEnabled != 0) {
        if (g_zVideo_pActiveViewContext->variantOverrideEnabled != 0 && variantFilterEnabled == 1) {
            g_Variant_CurrentTag = g_zVideo_pActiveViewContext->variantTag;
        } else {
            PlayerProbeSampleCandidateBuffer pickCandidates;
            g_Variant_FilterEnabled = 0;
            FindBestPickCandidateBelowPoint(world, &g_zVideo_pActiveViewContext->cameraPos, &pickCandidates);
            g_Variant_FilterEnabled = variantFilterEnabled;

            if (pickCandidates.candidateCount > 0) {
                if (pickCandidates.entries[0].variantTag.count > 0) {
                    g_zVideo_pActiveViewContext->variantTag = pickCandidates.entries[0].variantTag;
                    g_Variant_CurrentTag = pickCandidates.entries[0].variantTag;
                }
            } else {
                Clear(&g_zVideo_pActiveViewContext->variantTag);
                g_Variant_CurrentTag = g_zVideo_pActiveViewContext->variantTag;
            }
        }
        g_zVideo_ActiveViewVariantTag = g_zVideo_pActiveViewContext->variantTag;
    }

    RenderWorld(world, camera, g_zVideo_pActiveViewContext);
    MatStackPopPtr();
    zRndrFlushTransparentQueue();
    if (updateFxPass3Local != 0) {
        FxPass3UpdateLocal(g_FrameDeltaTimeSec);
    }
    zRndrFlushOverwriteQueue();
    zRndrLensFlareDrawQueuedSamples16AndBuildVisibleList(queuedLensFlareSampleCount);
    zRndrLensFlareDrawVisibleSamples();
    zRndrFlushTransparentQueue();
    zRndrOverlayRectFlushSw();

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.zvideo-sw-renderframe
 * @recoil-artifact defines .text recoil:function:0x44d600: zVideo_sw::RenderFrame.
 *
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Data evidence: BN writes the render-frame active view context at 0x5398fc,
 * updates the active variant tag at 0x5398f8, dispatches the three renderer
 * flush callbacks at 0x56bc6c..0x56bc74, and brackets rendering through the
 * scene-depth owner at 0x632148.
 * Purpose: provide the recovered zVideoswRenderFrame behavior.
 */
int __fastcall zVideoswRenderFrame(CZNodePartial* camera, int updateFxPass3Local)
{
    const int queuedLensFlareSampleCount = zRndrLensFlareGetQueuedSampleCount();
    zMat4x3 slotBuffer;
    CZNodePartial* world;
    CZWindowDataPartial* windowData;
    PlayerProbeSampleCandidateBuffer pickCandidates;
    int visibleLensFlareSampleCount;
    int sampleIndex;
    MatStackPushPtr((float*)&slotBuffer);

    g_zVideo_pActiveViewContext = (CZCameraDataPartial*)(camera->classData);
    world = gwCameraGetWorld(camera);
    windowData = (CZWindowDataPartial*)(g_zVideo_pActiveViewContext->windowNode->classData);

    if (g_CZClass_CameraAutoClipDistanceAdjustEnabled != 0) {
        const float scale = g_CZClass_CameraAutoClipDistanceScale;
        if (g_FrameDeltaTimeSec > g_CZClass_CameraAutoClipDistanceThreshold) {
            g_CZClass_CameraAutoClipDistanceScale = scale - g_CZClass_CameraAutoClipDistanceStep;
        } else {
            g_CZClass_CameraAutoClipDistanceScale = scale + g_CZClass_CameraAutoClipDistanceStep;
        }

        if (g_CZClass_CameraAutoClipDistanceScale > 1.0f) {
            g_CZClass_CameraAutoClipDistanceScale = 1.0f;
        } else if (g_CZClass_CameraAutoClipDistanceScale < g_CZClass_CameraAutoClipDistanceMinScale) {
            g_CZClass_CameraAutoClipDistanceScale = g_CZClass_CameraAutoClipDistanceMinScale;
        }

        gwCameraSetClipDistance(camera, g_CZClass_CameraAutoClipDistanceScale);
    }

    InitLightPointInPolygonXZ(world);
    BindWorldNode(world);
    gwCameraUpdate(camera);
    SyncViewContextPositions();
    zVideoSetActiveViewContext(g_zVideo_pActiveViewContext);
    UpdateAllLights(world);
    UpdateAllSounds(world);

    g_CZClass_LodDistanceStateStackTop = 0;
    if (g_Variant_FilterEnabled != 0) {
        if (g_zVideo_pActiveViewContext->variantOverrideEnabled != 0 && g_Variant_FilterEnabled == 1) {
            g_Variant_CurrentTag = g_zVideo_pActiveViewContext->variantTag;
        } else {
            const int variantFilterEnabled = g_Variant_FilterEnabled;
            g_Variant_FilterEnabled = 0;
            FindBestPickCandidateBelowPoint(world, &g_zVideo_pActiveViewContext->cameraPos, &pickCandidates);
            g_Variant_FilterEnabled = variantFilterEnabled;

            if (pickCandidates.candidateCount > 0) {
                if (pickCandidates.entries[0].variantTag.count > 0) {
                    g_zVideo_pActiveViewContext->variantTag = pickCandidates.entries[0].variantTag;
                    g_Variant_CurrentTag = pickCandidates.entries[0].variantTag;
                }
            } else {
                Clear(&g_zVideo_pActiveViewContext->variantTag);
                g_Variant_CurrentTag = g_zVideo_pActiveViewContext->variantTag;
            }
        }

        g_zVideo_ActiveViewVariantTag = g_zVideo_pActiveViewContext->variantTag;
    }

    SceneEnter();
    RenderWorld(world, camera, g_zVideo_pActiveViewContext);
    MatStackPopPtr();

    g_zVideo_pfnFlushSortedPolys();
    if (updateFxPass3Local != 0) {
        FxPass3UpdateLocal(g_FrameDeltaTimeSec);
    }
    g_zVideo_pfnFlushSortedPolys();
    g_zVideo_pfnFlushOverwritePolys();

    visibleLensFlareSampleCount = zRndrLensFlareBuildVisibleSampleListFromQueue(queuedLensFlareSampleCount);
    for (sampleIndex = 0; sampleIndex < visibleLensFlareSampleCount; ++sampleIndex) {
        zVec3 visibleSamplePoint;
        int raycastHit;
        zRndrSpanOcclusionFilterSampleList(sampleIndex, &visibleSamplePoint);
        SetStopAfterFirstHit(0x40000);
        SetBreakOnFirstCandidate(1);
        raycastHit = RaycastFindClosest(
            g_zVideo_pActiveViewContext->worldNode,
            g_zVideo_pActiveViewContext->cameraPos.x,
            g_zVideo_pActiveViewContext->cameraPos.y,
            g_zVideo_pActiveViewContext->cameraPos.z,
            visibleSamplePoint.x,
            visibleSamplePoint.y,
            visibleSamplePoint.z,
            &pickCandidates
        );
        SetBreakOnFirstCandidate(0);
        if (raycastHit != 0 || pickCandidates.candidateCount == 0) {
            zRndrLensFlareDrawVisibleSample(sampleIndex);
        }
    }

    g_zVideo_pfnFlushSortedPolys();
    g_zVideo_pfnFlushOverwritePolys();
    g_zVideo_pfnFlushQuadBatch();
    SceneLeave();

    if (CountNodes(8) > 1 && (windowData->clearPolyIndexFlags & 0x80000000) != 0) {
        unsigned int i;
        for (i = 0; i < (unsigned int)(windowData->clearPolyIndexFlags & 0x7fffffff); ++i) {
            if ((windowData->clearPolys[i].vertCount & 0x80000000) != 0) {
                zVidRect32 rect;
                unsigned int vertexIndex;
                rect.left = rect.right = (int)(windowData->clearPolys[i].vertices[0].x);
                rect.top = rect.bottom = (int)(windowData->clearPolys[i].vertices[0].y);

                for (vertexIndex = 1; vertexIndex < (unsigned int)(windowData->clearPolys[i].vertCount & 0x7fffffff);
                    ++vertexIndex) {
                    if (rect.left > windowData->clearPolys[i].vertices[vertexIndex].x) {
                        rect.left = (int)(windowData->clearPolys[i].vertices[vertexIndex].x);
                    }
                    if (rect.right < windowData->clearPolys[i].vertices[vertexIndex].x) {
                        rect.right = (int)(windowData->clearPolys[i].vertices[vertexIndex].x);
                    }
                    if (rect.top > windowData->clearPolys[i].vertices[vertexIndex].y) {
                        rect.top = (int)(windowData->clearPolys[i].vertices[vertexIndex].y);
                    }
                    if (rect.bottom < windowData->clearPolys[i].vertices[vertexIndex].y) {
                        rect.bottom = (int)(windowData->clearPolys[i].vertices[vertexIndex].y);
                    }
                }

                CallClearZBufferRect(&rect);
            }
        }
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zclass.camera.deletenode
 * @recoil-artifact defines .text recoil:logical-function:0x44db00:zclass-camera-delete-node: CZCameraDeleteNode
 * Purpose: route camera deletion through the generic node free path.
 */
int __fastcall CZCameraDeleteNode(CZNodePartial* node)
{
    return TryFreeNode(node);
}
