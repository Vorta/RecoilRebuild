#include "GameZRecoil/zMath/zmth.h"

#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "zclass.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

/**
 * Purpose: stores the projection scale, offset, inverse scale, and sphere
 * radius cache values derived by zMath projection setup.
 */
float g_zMath_ProjSphereRadiusScale = 0.0f;
float g_zMath_ProjScaleX = 0.0f;
float g_zMath_ProjScaleY = 0.0f;
float g_zMath_ProjOffsetX = 0.0f;
float g_zMath_ProjOffsetY = 0.0f;
float g_zMath_InvProjScaleX = 0.0f;
float g_zMath_InvProjScaleY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-clipzlowerbound
 * @recoil-artifact defines .data recoil:data:0x4e4880: g_zMath_ClipZLowerBound.
 * Purpose: stores the mutable lower Z clipping plane used by the zMath line
 * segment clipping helpers.
 */
float g_zMath_ClipZLowerBound = 1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-clipzupperbound
 * @recoil-artifact defines .data recoil:data:0x4e4890: g_zMath_ClipZUpperBound.
 * Purpose: stores the mutable upper Z clipping plane used by the zMath line
 * segment clipping helpers.
 */
float g_zMath_ClipZUpperBound = 1.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-shared-zmath-midpoint-half-scalar
 * @recoil-artifact defines .rdata recoil:data:0x4d08d4: shared zMath midpoint half scalar.
 * Purpose: supplies Vec3Midpoint's component scale after summing both source
 * vectors.
 */
const float g_zMath_MidpointHalf = 0.5f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-elevationpifloat
 * @recoil-artifact defines .rdata recoil:data:0x4d2998: g_zMath_ElevationPiFloat.
 * Purpose: supplies the Euler roll adjustment pi scalar.
 */
const float g_zMath_ElevationPiFloat = 3.14159274f;
/**
 * Purpose: stores the mutable screen, focal, viewport, and projection-depth
 * cache values consumed by zMath projection and unprojection helpers.
 */
int g_zMath_ScreenWidthPx = 0;
int g_zMath_ScreenHeightPx = 0;
float g_zMath_FocalScaleX = 0.0f;
float g_zMath_FocalScaleY = 0.0f;
float g_zMath_InvFocalScaleX = 0.0f;
float g_zMath_InvFocalScaleY = 0.0f;
float g_zMath_HalfViewWidth = 0.0f;
float g_zMath_HalfViewHeight = 0.0f;
float g_zMath_ViewportOriginX = 0.0f;
float g_zMath_ViewportOriginY = 0.0f;
float g_zMath_ProjDepth = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-approxexpnegtable
 * @recoil-artifact defines .data recoil:data:0x566438: g_zMath_ApproxExpNegTable.
 * Purpose: stores the lazy approximate negative-exponential lookup table used
 * by zMath::ApproxExpNeg.
 */
float g_zMath_ApproxExpNegTable[256] = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-approxexpnegscale
 * @recoil-artifact defines .data recoil:data:0x5669d0: g_zMath_ApproxExpNegScale.
 * Purpose: stores the table-index scale for zMath::ApproxExpNeg's lazy
 * approximate negative-exponential cache.
 */
float g_zMath_ApproxExpNegScale = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-approxexpnegdirty
 * @recoil-artifact defines .data recoil:data:0x4e0e8c: g_zMath_ApproxExpNegDirty.
 * Purpose: stores the rebuild flag for zMath::ApproxExpNeg's lazy lookup
 * table.
 */
int g_zMath_ApproxExpNegDirty = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfuncnamefloor
 * @recoil-artifact defines .data recoil:data:0x4e0e90: g_zMath_ExceptionFuncNameFloor.
 * Purpose: names the floor CRT math exception handled by zMath.
 */
char g_zMath_ExceptionFuncNameFloor[0x6] = "floor";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfuncnameceil
 * @recoil-artifact defines .data recoil:data:0x4e0e98: g_zMath_ExceptionFuncNameCeil.
 * Purpose: names the ceil CRT math exception handled by zMath.
 */
char g_zMath_ExceptionFuncNameCeil[0x5] = "ceil";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfuncnameasin
 * @recoil-artifact defines .data recoil:data:0x4e0ea0: g_zMath_ExceptionFuncNameAsin.
 * Purpose: names the asin CRT math exception clamped by zMath.
 */
char g_zMath_ExceptionFuncNameAsin[0x5] = "asin";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfmt
 * @recoil-artifact defines .data recoil:data:0x4e0ea8: g_zMath_ExceptionFmt.
 * Purpose: formats the stderr CRT math exception diagnostic line.
 */
char g_zMath_ExceptionFmt[0x2b] = "Math Exception: type=%d, [%s(%.8f, %.8f)]\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-sourcefile-zmthmainc
 * @recoil-artifact defines .data recoil:data:0x4e0ed4: g_zMath_SourceFile_ZmthMainC.
 * Purpose: supplies the recovered zmth_main.c source path for zError math
 * exception reports.
 */
char g_zMath_SourceFile_ZmthMainC[0x26] = "D:\\Proj\\GameZRecoil\\zMath\\zmth_main.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-exceptionfmtnonewline
 * @recoil-artifact defines .data recoil:data:0x4e0efc: g_zMath_ExceptionFmtNoNewline.
 * Purpose: formats the zError CRT math exception diagnostic message.
 */
char g_zMath_ExceptionFmtNoNewline[0x2a] = "Math Exception: type=%d, [%s(%.8f, %.8f)]";

/**
 * Provider-boundary CRT hook: user-supplied _matherr installed by VC5 CRTEXE startup.
 * Purpose: exposes the zMath math exception handler to the CRT without pulling
 * the default MSVCRT merr.obj handler.
 */
extern "C" int __cdecl _matherr(_exception* except)
{
    return zMath::CrtMatherrHandler(except);
}

namespace
{
    /*
     * Purpose: retain the current matrix-stack implementation while its original
     * allocation extent is unresolved. Retail initializes cursors to 0x566950
     * and 0x566868 and advances them by four bytes; those accesses prove flags
     * and matrix-pointer slots, not the historical 32-entry source capacities.
     */
    int g_matrixIdentityFlagSlots[32] = { 0 };
    float* g_matrixSlots[32] = { 0 };

    /**
     * Original static helper observed in callers 0x4753e0 and 0x475210
     * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
     * Purpose: subtract two zVec3 values for triangle-gradient and intersection
     * vector math.
     */
    zVec3 Subtract(const zVec3& lhs, const zVec3& rhs)
    {
        zVec3 result = { lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z };
        return result;
    }

    /**
     * Original static helper observed in caller 0x4753e0
     * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
     * Purpose: compute a zVec3 cross product for perspective texture-gradient
     * setup.
     */
    zVec3 Cross(const zVec3& lhs, const zVec3& rhs)
    {
        zVec3 result = { lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x };
        return result;
    }

    /**
     * Original static helper observed in zMath vector and projection callers
     * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
     * Purpose: compute the zVec3 dot product used by gradient, slerp, and
     * line/sphere routines.
     */
    float Dot(const zVec3& lhs, const zVec3& rhs)
    {
        return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
    }

    /**
     * Original static helper observed in zMath vector/intersection callers
     * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
     * Purpose: recover the original fast square-root estimate from a float bit
     * pattern for vector normalization and hit tests.
     */
    float FastSqrtEstimate(float value)
    {
        unsigned int bits = 0;
        memcpy(&bits, &value, sizeof(bits));
        bits = (bits >> 1) + 0x1fc00000u;
        memcpy(&value, &bits, sizeof(value));
        return value;
    }

    /**
     * Original static helper observed in caller 0x4753e0
     * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
     * Purpose: add two zVec3 values during perspective texture-gradient plane
     * construction.
     */
    zVec3 Add(const zVec3& lhs, const zVec3& rhs)
    {
        zVec3 result = { lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z };
        return result;
    }

    /**
     * Original static helper observed in caller 0x4753e0
     * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
     * Purpose: scale a zVec3 value during perspective texture-gradient plane
     * construction.
     */
    zVec3 Scale(const zVec3& value, float scale)
    {
        zVec3 result = { value.x * scale, value.y * scale, value.z * scale };
        return result;
    }

    /**
     * Original static helper observed in caller 0x4753e0
     * (D:\Proj\GameZRecoil\zMath\zMath.cpp).
     * Purpose: build one perspective-correct UV-over-Z gradient plane and base
     * term from triangle geometry and reciprocal-Z gradients.
     */
    void BuildUvOverZPlane(
        const zVec3* triVerts,
        const zVec3& edge21,
        const zVec3& edge01,
        float edge21LenSq,
        float edge01LenSq,
        float edgeDotScaled,
        float invGram,
        const zVec2& recipZGrad,
        float recipZBase,
        float uv0,
        float uv1,
        float uv2,
        zVec2* outGrad,
        float* outBase
    )
    {
        const float delta21 = uv2 - uv1;
        const float delta01 = uv0 - uv1;
        const zVec3 plane = Add(
            Scale(edge21, delta21 * edge01LenSq * invGram - delta01 * edgeDotScaled),
            Scale(edge01, delta01 * edge21LenSq * invGram - delta21 * edgeDotScaled)
        );
        const float originDelta = uv0 - Dot(plane, triVerts[0]);

        outGrad->x = originDelta * recipZGrad.x + plane.x * g_zMath_InvProjScaleX;
        outGrad->y = originDelta * recipZGrad.y + plane.y * g_zMath_InvProjScaleY;
        *outBase = originDelta * recipZBase + plane.z;
    }

} // namespace

namespace zMath
{
    /**
     * Purpose: Inline-function spelling of the reviewed vector-dot island for
     * this unit's consumers. VC5 binds simple variable arguments to their own
     * homes and address arguments to inline-parameter homes, which the capturing
     * ZMTH_VECTOR_DOT cannot express. Original header ownership is unrecovered.
     * Retail inline-expansion evidence: the listed consumer contains the operand reloads, arithmetic
     * sequence and result store without a call at that site; the original inline helper's header
     * ownership and declaration placement are not established (TU-resident reconstruction model).
     * Original inline helper evidence: no standalone retail function; observed at
     * retail 0x475210 and 0x4753e0, whose dot-product islands load a simple
     * pointer argument from its own home and an address argument from a capture.
     * Defined ahead of the unit's functions (TU-resident helper placement); after
     * the zmth_vec.c split this placement keeps 0x474010's VC5 ID-counter parity.
     */
    inline float Vec3Dot(const zVec3* left, const zVec3* right)
    {
        float result;
        ZMTH_VECTOR_DOT_BOUND(result, left, right);
        return result;
    }
} // namespace zMath

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-camerascratchb
     * @recoil-artifact defines .data recoil:data:0x5668e8: g_zMath_CameraScratchB
     * Purpose: stores the camera inverse-rotation scratch matrix loaded by the
     * camera setup/projection/view helpers.
     */
    zMat4x3 g_zMath_CameraScratchB = { 0 };
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-g-zmath-camerascratcha
     * @recoil-artifact defines .data recoil:data:0x566920: g_zMath_CameraScratchA
     * Purpose: stores the staged camera world matrix before the inverse-rotation
     * transpose is copied into camera scratch B.
     */
    zMat4x3 g_zMath_CameraScratchA = { 0 };
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-g-zmath-vec3zero
     * @recoil-artifact defines .data recoil:data:0x5669d8: zMath::g_zMath_Vec3Zero
     * Purpose: shared writable zero vector read by zMath view-matrix setup and
     * projectile runtime initialization.
     */
    zVec3 g_zMath_Vec3Zero = { 0 };
    int* g_currentMatrixIdentityFlagSlot = &g_matrixIdentityFlagSlots[0];
    float** g_currentMatrixPtrSlot = &g_matrixSlots[0];
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-crtmatherrhandler
 * @recoil-artifact defines .text recoil:function:0x472d30: zMath::CrtMatherrHandler
 * @recoil-match byte
 *
 * Purpose: reports CRT math exceptions and supplies recovered return values
 * for zMath asin, ceil, and floor failures.
 */
int __cdecl zMath::CrtMatherrHandler(_exception* except)
{
    zError::ReportOld(
        0x400,
        g_zMath_SourceFile_ZmthMainC,
        376,
        g_zMath_ExceptionFmtNoNewline,
        except->type,
        except->name,
        except->arg1,
        except->arg2
    );
    fprintf(stderr, g_zMath_ExceptionFmt, except->type, except->name, except->arg1, except->arg2);

    if (strcmp(except->name, g_zMath_ExceptionFuncNameAsin) == 0) {
        double arg = except->arg1;
        if (arg > 1.0) {
            arg = 1.0;
        } else if (arg < -1.0 || arg != arg) {
            arg = -1.0;
        }
        except->retval = asin(arg);
        return 1;
    }

    if (strcmp(except->name, g_zMath_ExceptionFuncNameCeil) == 0) {
        except->retval = 0.0;
        return 1;
    }

    if (strcmp(except->name, g_zMath_ExceptionFuncNameFloor) == 0) {
        except->retval = 0.0;
        return 1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-project-getlastscreenscalexy
 * @recoil-artifact defines .text recoil:function:0x472ed0: zMathProjectGetLastScreenScaleXY.
 * @recoil-match byte
 *
 * Purpose: returns the last cached projection X/Y scale values as a zVec2.
 */
zVec2 __cdecl zMathProjectGetLastScreenScaleXY()
{
    zVec2 scale;
    scale.x = g_zMath_ProjScaleX;
    scale.y = g_zMath_ProjScaleY;
    return scale;
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matstackpushandcloneparent
     * @recoil-artifact defines .text recoil:function:0x472ef0: zMath::MatStackPushAndCloneParent.
     * @recoil-match byte
     *
     * Purpose: pushes a caller-supplied matrix slot and clones the parent matrix
     * and identity flag into the new top-of-stack slot.
     */
    void __fastcall MatStackPushAndCloneParent(float* newSlotBuffer)
    {
        ++g_currentMatrixIdentityFlagSlot;
        ++g_currentMatrixPtrSlot;
        *g_currentMatrixIdentityFlagSlot = g_currentMatrixIdentityFlagSlot[-1];
        *g_currentMatrixPtrSlot = newSlotBuffer;
        memcpy(*g_currentMatrixPtrSlot, g_currentMatrixPtrSlot[-1], sizeof(zMat4x3));
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matstackpushptr
     * @recoil-artifact defines .text recoil:function:0x472f30: zMath::MatStackPushPtr.
     * @recoil-match byte
     *
     * Purpose: pushes a caller-supplied matrix pointer onto the zMath matrix
     * stack and marks the new slot non-identity.
     */
    void __fastcall MatStackPushPtr(float* matrix)
    {
        ++g_currentMatrixIdentityFlagSlot;
        ++g_currentMatrixPtrSlot;
        *g_currentMatrixPtrSlot = matrix;
        *g_currentMatrixIdentityFlagSlot = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matstackpopptr
     * @recoil-artifact defines .text recoil:function:0x472f60: zMath::MatStackPopPtr.
     * @recoil-match byte
     *
     * Purpose: pops the current zMath matrix pointer and identity-flag slots.
     */
    void __fastcall MatStackPopPtr()
    {
        --g_currentMatrixIdentityFlagSlot;
        --g_currentMatrixPtrSlot;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadcamerascratchb
     * @recoil-artifact defines .text recoil:function:0x472f90: zMath::MatLoadCameraScratchB
     * @recoil-match byte
     *
     * Purpose: loads camera scratch B into the current matrix stack slot.
     */
    void __cdecl MatLoadCameraScratchB()
    {
        MatLoadCurrentFrom(&g_zMath_CameraScratchB);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadcamerascratcha
     * @recoil-artifact defines .text recoil:function:0x472fa0: zMath::MatLoadCameraScratchA
     * @recoil-match byte
     *
     * Purpose: loads camera scratch A into the current matrix stack slot.
     */
    void __cdecl MatLoadCameraScratchA()
    {
        MatLoadCurrentFrom(&g_zMath_CameraScratchA);
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-loadprojection
 * @recoil-artifact defines .text recoil:function:0x472fb0: zMathMatLoadProjection
 * @recoil-match byte
 *
 * Purpose: builds the current projection-node matrix from the parent slot,
 * camera scratch B, and a caller-supplied yaw/Z offset.
 */
void __stdcall zMathMatLoadProjection(float zOffset)
{
    float parentYaw;
    if (zMath::g_currentMatrixIdentityFlagSlot[-1] == 0) {
        parentYaw = zMathMatExtractYaw((const zMat4x3*)(zMath::g_currentMatrixPtrSlot[-1]));
    } else {
        parentYaw = 0.0f;
    }

    zMath::MatLoadIdentity();
    zMath::MatRotateY(zOffset - parentYaw);
    zMath::MatMultiply((const zMat4x3*)(zMath::g_currentMatrixPtrSlot[-1]), 1);

    zMat4x3* current = (zMat4x3*)(*zMath::g_currentMatrixPtrSlot);
    const zMat4x3* parent = (const zMat4x3*)(zMath::g_currentMatrixPtrSlot[-1]);
    // Retail copies the translation row as one zVec3 (dword block copy).
    *(zVec3*)(&current->posX) = *(const zVec3*)(&parent->posX);

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float*)(&slotBuffer));
    zMath::MatLoadCameraScratchB();
    zMath::MatMultiply((const zMat4x3*)(zMath::g_currentMatrixPtrSlot[-1]), 1);
    zMath::MatStackPopPtr();
    zMath::MatLoadCurrentFrom(&slotBuffer);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-loadview
 * @recoil-artifact defines .text recoil:function:0x473060: zMathMatLoadView
 *
 *
 * Purpose: builds the current view matrix from camera and parent transforms.
 */
void __cdecl zMathMatLoadView()
{
    zVec3 parentEuler = zMath::g_zMath_Vec3Zero;
    if (zMath::g_currentMatrixIdentityFlagSlot[-1] == 0) {
        zMathMatExtractEulerAngles((const zMat4x3*)(zMath::g_currentMatrixPtrSlot[-1]), &parentEuler);
    }

    zMath::MatLoadCameraScratchA();
    zMat4x3* current = (zMat4x3*)(*zMath::g_currentMatrixPtrSlot);
    current->yx = -current->yx;
    current->yy = -current->yy;
    current->yz = -current->yz;
    current->zx = -current->zx;
    current->zy = -current->zy;
    current->zz = -current->zz;

    zVec3 cameraEuler = zMath::g_zMath_Vec3Zero;
    zMathMatExtractEulerAngles(current, &cameraEuler);

    zQuat parentQuat = { 0 };
    zMathQuatFromEulerYXZ(&parentQuat, parentEuler.y, parentEuler.x, parentEuler.z);

    zQuat cameraQuat = { 0 };
    zMathQuatFromEulerYXZ(&cameraQuat, cameraEuler.y, cameraEuler.x, cameraEuler.z);

    zQuat relativeQuat = { 0 };
    zMathQuatMultiplyConjugate(&cameraQuat, &parentQuat, &relativeQuat);

    zMat4x3 viewMatrix = { 0 };
    zMathQuatToMatrix(&relativeQuat, &viewMatrix);
    zMath::MatLoadCurrentFrom(&viewMatrix);
    zMath::MatMultiply((const zMat4x3*)(zMath::g_currentMatrixPtrSlot[-1]), 1);

    current = (zMat4x3*)(*zMath::g_currentMatrixPtrSlot);
    const zMat4x3* parent = (const zMat4x3*)(zMath::g_currentMatrixPtrSlot[-1]);
    current->posX = parent->posX;
    current->posY = parent->posY;
    current->posZ = parent->posZ;

    zMath::MatStackPushPtr((float*)(&viewMatrix));
    zMath::MatLoadCameraScratchB();
    zMath::MatMultiply((const zMat4x3*)(zMath::g_currentMatrixPtrSlot[-1]), 1);
    zMath::MatStackPopPtr();
    zMath::MatLoadCurrentFrom(&viewMatrix);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-setupcamera
 * @recoil-artifact defines .text recoil:function:0x4731f0: zMathMatSetupCamera
 * @recoil-match byte
 *
 * Purpose: loads camera scratch B and composes it through the parent matrix
 * stack slot.
 */
void __cdecl zMathMatSetupCamera()
{
    zMath::MatLoadCameraScratchB();
    zMath::MatMultiply((const zMat4x3*)(zMath::g_currentMatrixPtrSlot[-1]), 1);
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matcopycurrentto
     * @recoil-artifact defines .text recoil:function:0x473210: zMath::MatCopyCurrentTo.
     * @recoil-match byte
     *
     * Purpose: copies the current matrix stack slot into caller-provided storage
     * and returns that storage pointer.
     */
    zMat4x3* __stdcall MatCopyCurrentTo(zMat4x3 * out)
    {
        memcpy(out, *g_currentMatrixPtrSlot, sizeof(zMat4x3));
        return out;
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-getcurrent
 * @recoil-artifact defines .text recoil:function:0x473230: zMathMatGetCurrent.
 * @recoil-match byte
 *
 * Purpose: returns the current zMath matrix stack slot as a 4x3 matrix.
 */
zMat4x3* __cdecl zMathMatGetCurrent()
{
    return (zMat4x3*)(*zMath::g_currentMatrixPtrSlot);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-iscurrentidentity
 * @recoil-artifact defines .text recoil:function:0x473240: zMathMatIsCurrentIdentity.
 * @recoil-match byte
 *
 * Purpose: returns the identity flag for the current zMath matrix stack slot.
 */
int __cdecl zMathMatIsCurrentIdentity()
{
    return *zMath::g_currentMatrixIdentityFlagSlot;
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadcurrentfrom
     * @recoil-artifact defines .text recoil:function:0x473250: zMath::MatLoadCurrentFrom.
     * @recoil-match byte
     *
     * Purpose: copies a caller-supplied 4x3 matrix into the current zMath matrix
     * stack slot and clears the identity flag.
     */
    void __fastcall MatLoadCurrentFrom(const zMat4x3* src)
    {
        memcpy(*g_currentMatrixPtrSlot, src, sizeof(zMat4x3));
        *g_currentMatrixIdentityFlagSlot = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadrotationfrom3x3
     * @recoil-artifact defines .text recoil:function:0x473280: zMath::MatLoadRotationFrom3x3.
     * @recoil-match byte
     *
     * Purpose: loads only the 3x3 rotation rows into the current matrix stack
     * slot and marks the slot non-identity.
     */
    void __fastcall MatLoadRotationFrom3x3(const zMat4x3* src)
    {
        unsigned int* matrix = (unsigned int*)(*g_currentMatrixPtrSlot);
        const unsigned int* source = (const unsigned int*)src;

        *matrix++ = *source++;
        *matrix++ = *source++;
        *matrix++ = *source++;
        *matrix++ = *source++;
        *matrix++ = *source++;
        *matrix++ = *source++;
        *matrix++ = *source++;
        *matrix++ = *source++;
        *matrix = *source;
        *g_currentMatrixIdentityFlagSlot = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matloadidentity
     * @recoil-artifact defines .text recoil:function:0x4732f0: zMath::MatLoadIdentity.
     * @recoil-match byte
     *
     * Purpose: writes an identity 4x3 matrix into the current matrix stack slot
     * and marks the slot as identity.
     */
    void __cdecl MatLoadIdentity()
    {
        float* matrix = *g_currentMatrixPtrSlot;
        *matrix++ = 1.0f;
        *matrix++ = 0.0f;
        *matrix++ = 0.0f;
        *matrix++ = 0.0f;
        *matrix++ = 1.0f;
        *matrix++ = 0.0f;
        *matrix++ = 0.0f;
        *matrix++ = 0.0f;
        *matrix++ = 1.0f;
        *matrix++ = 0.0f;
        *matrix++ = 0.0f;
        *matrix = 0.0f;
        *g_currentMatrixIdentityFlagSlot = 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matmultiply
     * @recoil-artifact defines .text recoil:function:0x473370: zMath::MatMultiply.
     *
     *
     * Purpose: multiplies the current matrix stack slot by a source matrix,
     * optionally preserving the current translation for mode 2.
     */
    void __fastcall MatMultiply(const zMat4x3* src, int mode)
    {
        zMat4x3* current = (zMat4x3*)(*g_currentMatrixPtrSlot);
        if (*g_currentMatrixIdentityFlagSlot != 0) {
            memcpy(current, src, sizeof(zMat4x3));
            *g_currentMatrixIdentityFlagSlot = 0;
            return;
        }

        const zMat4x3 lhs = *current;
        zMat4x3 out = { 0 };

        out.xx = lhs.xx * src->xx + lhs.yx * src->xy + lhs.zx * src->xz;
        out.xy = lhs.xy * src->xx + lhs.yy * src->xy + lhs.zy * src->xz;
        out.xz = lhs.xz * src->xx + lhs.yz * src->xy + lhs.zz * src->xz;
        out.yx = lhs.xx * src->yx + lhs.yx * src->yy + lhs.zx * src->yz;
        out.yy = lhs.xy * src->yx + lhs.yy * src->yy + lhs.zy * src->yz;
        out.yz = lhs.xz * src->yx + lhs.yz * src->yy + lhs.zz * src->yz;
        out.zx = lhs.xx * src->zx + lhs.yx * src->zy + lhs.zx * src->zz;
        out.zy = lhs.xy * src->zx + lhs.yy * src->zy + lhs.zy * src->zz;
        out.zz = lhs.xz * src->zx + lhs.yz * src->zy + lhs.zz * src->zz;

        if (mode != 2) {
            out.posX = lhs.xx * src->posX + lhs.yx * src->posY + lhs.zx * src->posZ + lhs.posX;
            out.posY = lhs.xy * src->posX + lhs.yy * src->posY + lhs.zy * src->posZ + lhs.posY;
            out.posZ = lhs.xz * src->posX + lhs.yz * src->posY + lhs.zz * src->posZ + lhs.posZ;
        } else {
            out.posX = lhs.posX;
            out.posY = lhs.posY;
            out.posZ = lhs.posZ;
        }

        *current = out;
        *g_currentMatrixIdentityFlagSlot = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-scale-gamezrecoil-zmath-zmath-matrix-cpp
     * @recoil-artifact defines .text recoil:function:0x473690: zMath::MatScale (GameZRecoil/zMath/zmath_matrix.cpp).
     * @recoil-match byte
     *
     * Purpose: Applies per-axis scale to the current matrix basis while preserving translation.
     */
    void __stdcall MatScale(float sx, float sy, float sz)
    {
        zMat4x3 scaled;
        if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
            ((zMat4x3*)*zMath::g_currentMatrixPtrSlot)->xx = sx;
            ((zMat4x3*)*zMath::g_currentMatrixPtrSlot)->yy = sy;
            ((zMat4x3*)*zMath::g_currentMatrixPtrSlot)->zz = sz;
        } else {
            const float* matrix = *zMath::g_currentMatrixPtrSlot;
            float* dest = *zMath::g_currentMatrixPtrSlot;
            const float* source = (const float*)&scaled;
            scaled.xx = matrix[0] * sx;
            scaled.xy = matrix[1] * sx;
            scaled.xz = matrix[2] * sx;
            scaled.yx = matrix[3] * sy;
            scaled.yy = matrix[4] * sy;
            scaled.yz = matrix[5] * sy;
            scaled.zx = matrix[6] * sz;
            scaled.zy = matrix[7] * sz;
            scaled.zz = matrix[8] * sz;
            scaled.posX = matrix[9];
            scaled.posY = matrix[10];
            scaled.posZ = matrix[11];
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest = *source;
        }
        *zMath::g_currentMatrixIdentityFlagSlot = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mattranslate
     * @recoil-artifact defines .text recoil:function:0x4737e0: zMath::MatTranslate.
     *
     *
     * Purpose: applies a local translation through the current matrix basis and
     * updates the current matrix stack slot.
     */
    void __stdcall MatTranslate(float tx, float ty, float tz)
    {
        zMat4x3 translated;
        if (*g_currentMatrixIdentityFlagSlot != 0) {
            ((zMat4x3*)*g_currentMatrixPtrSlot)->posX = tx;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->posY = ty;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->posZ = tz;
        } else {
            const zMat4x3* matrix = (const zMat4x3*)*g_currentMatrixPtrSlot;
            float* dest = *g_currentMatrixPtrSlot;
            const float* source = (const float*)&translated;
            translated.xx = matrix->xx;
            translated.xy = matrix->xy;
            translated.xz = matrix->xz;
            translated.yx = matrix->yx;
            translated.yy = matrix->yy;
            translated.yz = matrix->yz;
            translated.zx = matrix->zx;
            translated.zy = matrix->zy;
            translated.zz = matrix->zz;
            translated.posX = tx * matrix->xx + ty * matrix->yx + tz * matrix->zx + matrix->posX;
            translated.posY = tx * matrix->xy + ty * matrix->yy + tz * matrix->zy + matrix->posY;
            translated.posZ = tx * matrix->xz + ty * matrix->yz + tz * matrix->zz + matrix->posZ;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest = *source;
        }
        *g_currentMatrixIdentityFlagSlot = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matrotatex
     * @recoil-artifact defines .text recoil:function:0x473970: zMath::MatRotateX.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
     * @recoil-match source
     *
     * Purpose: applies an X-axis rotation to the current matrix stack slot.
     */
    void __stdcall MatRotateX(float angleRad)
    {
        zMat4x3 rotated;
        float sinAngle;
        float cosAngle;
        SinCos(angleRad, &sinAngle, &cosAngle);
        if (*g_currentMatrixIdentityFlagSlot != 0) {
            ((zMat4x3*)*g_currentMatrixPtrSlot)->yy = cosAngle;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->yz = sinAngle;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->zy = -sinAngle;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->zz = cosAngle;
        } else {
            const zMat4x3* matrix = (const zMat4x3*)*g_currentMatrixPtrSlot;
            float* dest = *g_currentMatrixPtrSlot;
            const float* source = (const float*)&rotated;
            rotated.xx = matrix->xx;
            rotated.xy = matrix->xy;
            rotated.xz = matrix->xz;
            rotated.yx = cosAngle * matrix->yx + sinAngle * matrix->zx;
            rotated.yy = cosAngle * matrix->yy + sinAngle * matrix->zy;
            rotated.yz = cosAngle * matrix->yz + sinAngle * matrix->zz;
            rotated.zx = cosAngle * matrix->zx - sinAngle * matrix->yx;
            rotated.zy = cosAngle * matrix->zy - sinAngle * matrix->yy;
            rotated.zz = cosAngle * matrix->zz - sinAngle * matrix->yz;
            rotated.posX = matrix->posX;
            rotated.posY = matrix->posY;
            rotated.posZ = matrix->posZ;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest = *source;
        }
        *g_currentMatrixIdentityFlagSlot = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matrotatey
     * @recoil-artifact defines .text recoil:function:0x473b10: zMath::MatRotateY.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
     * @recoil-match source
     *
     * Purpose: applies a Y-axis rotation to the current matrix stack slot while
     * preserving translation.
     */
    void __stdcall MatRotateY(float angleRad)
    {
        zMat4x3 rotated;
        float sinAngle;
        float cosAngle;
        SinCos(angleRad, &sinAngle, &cosAngle);
        if (*g_currentMatrixIdentityFlagSlot != 0) {
            ((zMat4x3*)*g_currentMatrixPtrSlot)->xx = cosAngle;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->xz = -sinAngle;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->zx = sinAngle;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->zz = cosAngle;
        } else {
            const zMat4x3* matrix = (const zMat4x3*)*g_currentMatrixPtrSlot;
            float* dest = *g_currentMatrixPtrSlot;
            const float* source = (const float*)&rotated;
            rotated.xx = cosAngle * matrix->xx - sinAngle * matrix->zx;
            rotated.xy = cosAngle * matrix->xy - sinAngle * matrix->zy;
            rotated.xz = cosAngle * matrix->xz - sinAngle * matrix->zz;
            rotated.yx = matrix->yx;
            rotated.yy = matrix->yy;
            rotated.yz = matrix->yz;
            rotated.zx = sinAngle * matrix->xx + cosAngle * matrix->zx;
            rotated.zy = sinAngle * matrix->xy + cosAngle * matrix->zy;
            rotated.zz = sinAngle * matrix->xz + cosAngle * matrix->zz;
            rotated.posX = matrix->posX;
            rotated.posY = matrix->posY;
            rotated.posZ = matrix->posZ;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest = *source;
        }
        *g_currentMatrixIdentityFlagSlot = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matrotatez
     * @recoil-artifact defines .text recoil:function:0x473cc0: zMath::MatRotateZ.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
     * @recoil-match source
     *
     * Purpose: applies a Z-axis rotation to the current matrix stack slot.
     */
    void __stdcall MatRotateZ(float angleRad)
    {
        zMat4x3 rotated;
        float sinAngle;
        float cosAngle;
        SinCos(angleRad, &sinAngle, &cosAngle);
        if (*g_currentMatrixIdentityFlagSlot != 0) {
            ((zMat4x3*)*g_currentMatrixPtrSlot)->xx = cosAngle;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->xy = sinAngle;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->yx = -sinAngle;
            ((zMat4x3*)*g_currentMatrixPtrSlot)->yy = cosAngle;
        } else {
            const zMat4x3* matrix = (const zMat4x3*)*g_currentMatrixPtrSlot;
            float* dest = *g_currentMatrixPtrSlot;
            const float* source = (const float*)&rotated;
            rotated.xx = cosAngle * matrix->xx + sinAngle * matrix->yx;
            rotated.xy = cosAngle * matrix->xy + sinAngle * matrix->yy;
            rotated.xz = cosAngle * matrix->xz + sinAngle * matrix->yz;
            rotated.yx = cosAngle * matrix->yx - sinAngle * matrix->xx;
            rotated.yy = cosAngle * matrix->yy - sinAngle * matrix->xy;
            rotated.yz = cosAngle * matrix->yz - sinAngle * matrix->xz;
            rotated.zx = matrix->zx;
            rotated.zy = matrix->zy;
            rotated.zz = matrix->zz;
            rotated.posX = matrix->posX;
            rotated.posY = matrix->posY;
            rotated.posZ = matrix->posZ;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest++ = *source++;
            *dest = *source;
        }
        *g_currentMatrixIdentityFlagSlot = 0;
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-camera-stageinverserotation
 * @recoil-artifact defines .text recoil:function:0x473e60: zMathCameraStageInverseRotation
 *
 *
 * Purpose: stages camera scratch matrices for inverse rotation and translated camera position.
 */
void __fastcall zMathCameraStageInverseRotation(const zMat4x3* worldMatrix)
{
    memcpy(&zMath::g_zMath_CameraScratchA, worldMatrix, sizeof(zMat4x3));

    *(unsigned int*)(&zMath::g_zMath_CameraScratchA.yx) ^= 0x80000000u;
    *(unsigned int*)(&zMath::g_zMath_CameraScratchA.yy) ^= 0x80000000u;
    *(unsigned int*)(&zMath::g_zMath_CameraScratchA.yz) ^= 0x80000000u;
    *(unsigned int*)(&zMath::g_zMath_CameraScratchA.zx) ^= 0x80000000u;
    *(unsigned int*)(&zMath::g_zMath_CameraScratchA.zy) ^= 0x80000000u;
    *(unsigned int*)(&zMath::g_zMath_CameraScratchA.zz) ^= 0x80000000u;

    memcpy(&zMath::g_zMath_CameraScratchB, &zMath::g_zMath_CameraScratchA, sizeof(zMat4x3));

    const float yx = zMath::g_zMath_CameraScratchA.yx;
    const float zx = zMath::g_zMath_CameraScratchB.zx;
    const float xy = zMath::g_zMath_CameraScratchA.xy;
    const float zy = zMath::g_zMath_CameraScratchB.zy;
    zMath::g_zMath_CameraScratchB.zx = zMath::g_zMath_CameraScratchB.xz;
    zMath::g_zMath_CameraScratchB.xy = yx;
    zMath::g_zMath_CameraScratchB.zy = zMath::g_zMath_CameraScratchB.yz;
    zMath::g_zMath_CameraScratchB.yx = xy;
    zMath::g_zMath_CameraScratchB.xz = zx;
    zMath::g_zMath_CameraScratchB.yz = zy;

    *(unsigned int*)(&zMath::g_zMath_CameraScratchB.posX) ^= 0x80000000u;
    *(unsigned int*)(&zMath::g_zMath_CameraScratchB.posY) ^= 0x80000000u;
    *(unsigned int*)(&zMath::g_zMath_CameraScratchB.posZ) ^= 0x80000000u;

    const zVec3 pos = { zMath::g_zMath_CameraScratchB.posX,
        zMath::g_zMath_CameraScratchB.posY,
        zMath::g_zMath_CameraScratchB.posZ };
    zMath::g_zMath_CameraScratchB.posZ = pos.x * zMath::g_zMath_CameraScratchB.xz
        + pos.y * zMath::g_zMath_CameraScratchB.yz + pos.z * zMath::g_zMath_CameraScratchB.zz;
    zMath::g_zMath_CameraScratchB.posY = pos.x * zMath::g_zMath_CameraScratchB.xy
        + pos.y * zMath::g_zMath_CameraScratchB.yy + pos.z * zMath::g_zMath_CameraScratchB.zy;
    zMath::g_zMath_CameraScratchB.posX = pos.x * zMath::g_zMath_CameraScratchB.xx
        + pos.y * zMath::g_zMath_CameraScratchB.yx + pos.z * zMath::g_zMath_CameraScratchB.zx;
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3arrayprojecttocachedy
     * @recoil-artifact defines .text recoil:function:0x473fc0: zMath::Vec3ArrayProjectToCachedY.
     * @recoil-match byte
     *
     * Purpose: projects an array of points against cached camera scratch row Y
     * into caller-provided scalar output storage.
     */
    void __fastcall Vec3ArrayProjectToCachedY(const zVec3* points, float* outValues, int count)
    {
        for (int i = 0; i < count; ++i) {
            float xy = points[i].x * g_zMath_CameraScratchA.xy + points[i].y * g_zMath_CameraScratchA.yy;
            outValues[i] = xy + points[i].z * g_zMath_CameraScratchA.zy + g_zMath_CameraScratchA.posY;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matapplylocaltrs
     * @recoil-artifact defines .text recoil:function:0x474010: zMath::MatApplyLocalTRS.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
     * @recoil-match byte
     *
     * Purpose: builds a local transform from Euler angles, position, and scale,
     * then composes it into the current matrix stack slot.
     */
    void __fastcall MatApplyLocalTRS(const zVec3* angles, const zVec3* position, const zVec3* scale)
    {
        float sx, cx, sy, cy, sz = sin(angles->z),
                              cz; // Initial sz is overwritten; its evaluation reproduces retail bytes.
        SinCos(angles->x, &sx, &cx);
        const float angleY = angles->y;
        SinCos(angleY, &sy, &cy);
        const float angleZ = angles->z;
        SinCos(angleZ, &sz, &cz);
        const float sySx = sy * sx;
        const float szCy = sz * cy;
        const float czCy = cz * cy;
        zMat4x3 local;
        local.posX = 0.0f;
        local.posY = 0.0f;
        local.posZ = 0.0f;
        local.xx = sySx * sz + czCy;
        local.xy = sz * cx;
        local.xz = szCy * sx - cz * sy;
        local.yx = sySx * cz - szCy;
        local.yy = cz * cx;
        local.yz = czCy * sx + sz * sy;
        local.zx = sy * cx;
        local.zy = -sx;
        local.zz = cy * cx;
        if (position->x != 0.0) {
            local.posX = position->x;
        }
        if (position->y != 0.0) {
            local.posY = position->y;
        }
        if (position->z != 0.0) {
            local.posZ = position->z;
        }
        if (scale->x != 1.0) {
            float scaleValue; // Unused saved factors; these assignments reproduce the retail bytes.
            local.xx = (scaleValue = scale->x) * local.xx;
            local.xy = (scaleValue = scale->x) * local.xy;
            local.xz = (scaleValue = scale->x) * local.xz;
        }
        if (scale->y != 1.0) {
            local.yx *= scale->y;
            local.yy *= scale->y;
            local.yz *= scale->y;
        }
        if (scale->z != 1.0) {
            local.zx *= scale->z;
            local.zy *= scale->z;
            local.zz *= scale->z;
        }

        MatMultiply(&local, 1);
        *g_currentMatrixIdentityFlagSlot = 0;
    }

                            /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-matbuildeulerrotation3x3
     * @recoil-artifact defines .text recoil:function:0x474260: zMath::MatBuildEulerRotation3x3.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
     * @recoil-match byte
     *
     * Purpose: builds a 3x3 Euler rotation basis in caller-provided matrix
     * storage and clears the translation row.
     */
    void __fastcall MatBuildEulerRotation3x3(float angleX, float angleY, float angleZ, zMat4x3* outBasis)
    {
        float sx;
        float cx;
        float sy;
        float cy;
        float sz;
        float cz;
        SinCos(angleX, &sx, &cx);
        SinCos(angleY, &sy, &cy);
        SinCos(angleZ, &sz, &cz);

        const float sySx = sy * sx;
        const float szCy = sz * cy;
        const float czCy = cz * cy;

        float* dest = &outBasis->xx;
        *dest++ = sySx * sz + czCy;
        *dest++ = sz * cx;
        *dest++ = szCy * sx - cz * sy;
        *dest++ = sySx * cz - szCy;
        *dest++ = cz * cx;
        *dest++ = czCy * sx + sz * sy;
        *dest++ = sy * cx;
        *dest++ = -sx;
        *dest++ = cy * cx;
        *dest++ = 0.0f;
        *dest++ = 0.0f;
        *dest = 0.0f;
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-setscreensize-gamezrecoil-zmath-zmath-proj-cpp
 * @recoil-artifact defines .text recoil:function:0x4743e0: zMathSetScreenSize (GameZRecoil/zMath/zmath_proj.cpp).
 * @recoil-match byte
 *
 * Purpose: Stores the active projection screen width and height globals.
 */
void __stdcall zMathSetScreenSize(int screenWidthPx, int screenHeightPx)
{
    g_zMath_ScreenWidthPx = screenWidthPx;
    g_zMath_ScreenHeightPx = screenHeightPx;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-setup-projection-gamezrecoil-zmath-zmath-proj-cpp
 * @recoil-artifact defines .text recoil:function:0x474400: zMathSetupProjection (GameZRecoil/zMath/zmath_proj.cpp).
 * @recoil-match byte
 *
 * Purpose: Derives cached projection scale, inverse scale, viewport, offset, radius-scale, and depth globals.
 */
void __stdcall zMathSetupProjection(
    float viewportOriginX,
    float viewportOriginY,
    float halfViewWidthPx,
    float halfViewHeightPx,
    float focalScaleX,
    float focalScaleY,
    float clipDistance,
    float projDepth
)
{
    g_zMath_FocalScaleX = focalScaleX;
    g_zMath_InvFocalScaleX = 1.0f / focalScaleX;
    g_zMath_ProjScaleX = focalScaleX * halfViewWidthPx;
    g_zMath_ProjScaleY = focalScaleY * halfViewHeightPx;
    g_zMath_InvFocalScaleY = 1.0f / focalScaleY;
    g_zMath_FocalScaleY = focalScaleY;
    g_zMath_InvProjScaleX = 1.0f / g_zMath_ProjScaleX;
    g_zMath_InvProjScaleY = 1.0f / g_zMath_ProjScaleY;
    g_zMath_HalfViewWidth = halfViewWidthPx;
    g_zMath_HalfViewHeight = halfViewHeightPx;
    g_zMath_ViewportOriginX = viewportOriginX;
    g_zMath_ViewportOriginY = viewportOriginY;
    g_zMath_ProjOffsetX = halfViewWidthPx + viewportOriginX;
    g_zMath_ProjOffsetY = halfViewHeightPx + viewportOriginY;
    g_zMath_ProjSphereRadiusScale = clipDistance;
    g_zMath_ProjDepth = projDepth;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3array-addscaled
 * @recoil-artifact defines .text recoil:function:0x4744f0: zMathVec3ArrayAddScaled.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Purpose: writes bias plus scaled source vectors across a caller-provided
 * vector array.
 */
void __fastcall
zMathVec3ArrayAddScaled(zVec3* outArray, const zVec3* biasArray, const zVec3* srcArray, int count, float scale)
{
    zVec3 scaled;
    while (count--) {
        const zVec3* const src = srcArray++;
        scaled.x = src->x * scale;
        scaled.y = src->y * scale;
        scaled.z = src->z * scale;
        zMath::Vec3Add(biasArray++, &scaled, outArray++);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-dirfromyaw
 * @recoil-artifact defines .text recoil:function:0x474580: zMathVec3DirFromYaw
 * @recoil-match byte
 *
 * Purpose: Clears the output vector, stages the canonical forward direction,
 * and rotates it around Y to produce a unit XZ direction from yaw.
 */
void __fastcall zMathVec3DirFromYaw(zVec3* outDir, float yawAngle)
{
    zVec3 forward = { 0.0f, 0.0f, -1.0f };
    outDir->x = 0.0f;
    outDir->y = 0.0f;
    outDir->z = 0.0f;

    zMath::Vec3RotateY(outDir, &forward, yawAngle);
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3perpxz
     * @recoil-artifact defines .text recoil:function:0x4745c0: zMath::Vec3ToRightXZ.
     * @recoil-match byte
     *
     * Purpose: builds the XZ-plane perpendicular vector with a zero Y component.
     */
    void __fastcall Vec3ToRightXZ(const zVec3* in, zVec3* out)
    {
        out->x = -in->z;
        out->z = in->x;
        out->y = 0.0f;
    }
} // namespace zMath

namespace zMath
{
    /**
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.main.vector-transform-direction-in-place
     *
     * Purpose: Transform a direction in place by the matrix's 3x3 part, without
     * translation; in the raw arm all reads precede the z/y/x binary32 stores.
     * Reconstruction: zmth_main.c-resident copy of the reviewed gmod_pick.c
     * in-place helper (same body; retail family 0x4293da, 0x473f6d, 0x47460f,
     * 0x485315); original spelling and declaration location unproved.
     * Raw assembly: identical body to the reviewed gmod_pick.c island.
     * Island contract: EAX/EBX hold vector/matrix from compiler-owned parameter
     * homes and are clobbered; integer flags and the x87 control word unchanged;
     * x87 entry/peak/exit depth 0/6/0 on normal completion; x87 status and
     * exceptions are not preserved. The vector must not overlap the matrix.
     * Consumers are scoped by the raw-assembly allowlist.
     * Retail inline-expansion evidence: the listed consumer contains the operand reloads, arithmetic
     * sequence and result stores without a call at that site; the original inline helper's header
     * ownership and declaration placement are not established (TU-resident reconstruction model).
     * Original inline helper evidence: no standalone retail function; observed at
     * retail 0x4745e0.
     */
    inline void Vec3TransformDirectionInPlace(const zMat4x3* matrix, zVec3* vector)
    {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        __asm {
        mov eax, vector
        mov ebx, matrix
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xx
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xy
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xz
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yx
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yy
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yz
        fxch st(2)
        faddp st(5), st
        faddp st(3), st
        faddp st(1), st
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zx
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zy
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zz
        fxch st(2)
        faddp st(5), st
        faddp st(3), st
        faddp st(1), st
        fstp dword ptr [eax]zVec3.z
        fstp dword ptr [eax]zVec3.y
        fstp dword ptr [eax]zVec3.x
        }
#else
        const zVec3 source = *vector;
        vector->x = source.x * matrix->xx + source.y * matrix->yx + source.z * matrix->zx;
        vector->y = source.x * matrix->xy + source.y * matrix->yy + source.z * matrix->zy;
        vector->z = source.x * matrix->xz + source.y * matrix->yz + source.z * matrix->zz;
#endif
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3array-untransformdirection
 * @recoil-artifact defines .text recoil:function:0x4745e0: zMathVec3ArrayTransformDirection.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.main.vector-transform-direction-in-place
 * @recoil-match byte
 *
 * Raw assembly: the zmth_main.c-resident inline helper expansion at
 * retail [0x474609,0x474659), including both parameter-home reloads
 * (requires zmth_main.c /Ob1).
 *
 *
 * Purpose: applies the current matrix rotation columns to direction vectors
 * in place when the matrix stack slot is not identity.
 */
void __fastcall zMathVec3ArrayTransformDirection(zVec3* vectors, int count)
{
    if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
        return;
    }

    while (count--) {
        zMath::Vec3TransformDirectionInPlace((const zMat4x3*)(*zMath::g_currentMatrixPtrSlot), vectors++);
    }
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3arraytransformdirection
     * @recoil-artifact defines .text recoil:function:0x474670: zMath::Vec3ArrayTransformDirectionTranspose.
     * @recoil-match source
     *
     * Purpose: transforms direction vectors in place by the transposed current matrix
     * rotation when the matrix stack slot is non-identity.
     */
    void __fastcall Vec3ArrayTransformDirectionTranspose(zVec3 * vectors, int count)
    {
        if (*g_currentMatrixIdentityFlagSlot != 0 || count <= 0) {
            return;
        }

        for (int i = 0; i < count; ++i) {
            const zMat4x3* const matrix = (const zMat4x3*)(*g_currentMatrixPtrSlot);
            zVec3 result;
            result.x = vectors[i].x * matrix->xx + vectors[i].y * matrix->xy + vectors[i].z * matrix->xz;
            result.y = vectors[i].x * matrix->yx + vectors[i].y * matrix->yy + vectors[i].z * matrix->yz;
            result.z = vectors[i].x * matrix->zx + vectors[i].y * matrix->zy + vectors[i].z * matrix->zz;
            vectors[i].x = result.x;
            vectors[i].y = result.y;
            vectors[i].z = result.z;
        }
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-transformnormalbatch
 * @recoil-artifact defines .text recoil:function:0x474710: zMathMatTransformDirectionBatch
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-direction
 * @recoil-match byte
 *
 * Purpose: transforms direction batches through the current matrix rotation, or
 * copies the input directions unchanged when the current matrix is identity.
 */
void __fastcall zMathMatTransformDirectionBatch(const zVec3* normals, zVec3* outNormals, int count)
{
    if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
        memcpy(outNormals, normals, count * sizeof(zVec3));
        return;
    }

    // Retail uses a post-decrement nonzero test, not a signed-positive test.
    // This follows the retail loop and does not validate count.
    while (count--) {
        const zMat4x3* const transformMatrix = (const zMat4x3*)(*zMath::g_currentMatrixPtrSlot);
        zVec3* const transformDest = outNormals;
        const zVec3* const transformSource = normals;
        normals++;
        outNormals++;
        ZMTH_VECTOR_TRANSFORM_DIRECTION_ISLAND(transformMatrix, transformDest, transformSource);
    }
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mattransformpointbatchinplace
     * @recoil-artifact defines .text recoil:function:0x4747d0: zMath::MatTransformPointBatchInPlace.
     *
     *
     * Purpose: transforms an array of points in place by the current 4x3 matrix
     * when the matrix stack slot is non-identity.
     */
    void __fastcall MatTransformPointBatchInPlace(zVec3 * points, int count)
    {
        if (*g_currentMatrixIdentityFlagSlot != 0 || count == 0) {
            return;
        }

        const zMat4x3* matrix = (const zMat4x3*)(*g_currentMatrixPtrSlot);
        for (int i = 0; i < count; ++i) {
            const zVec3 point = points[i];
            points[i].x = point.x * matrix->xx + point.y * matrix->yx + point.z * matrix->zx + matrix->posX;
            points[i].z = point.x * matrix->xz + point.y * matrix->yz + point.z * matrix->zz + matrix->posZ;
            points[i].y = point.x * matrix->xy + point.y * matrix->yy + point.z * matrix->zy + matrix->posY;
        }
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-transformbboxtocorners
 * @recoil-artifact defines .text recoil:function:0x474870: zMathMatTransformBBoxToCorners
 *
 *
 * Purpose: Transform finite, representable geometry under the reviewed functional contract.
 * Contract and proof: docs/reconstruction/audits/bbox_474870_byte_matching_2026-09-16.md.
 */
void __fastcall zMathMatTransformBBoxToCorners(const zMat4x3* matrix, const zBBox3f* bbox, zBBoxCorners* outCorners)
{
    zVec3 minXMinY;
    zVec3 maxXMinY;
    zVec3 work;
    zVec3 minXMaxY;
    zVec3 maxXMaxY;
    float savedComponent; // Some captures are unused; preserve the closer retail x87 schedule.
    minXMinY.x = bbox->min.x; // Seed the factor to retain retail's multiply operand order.
    minXMinY.x *= matrix->xx;
    minXMinY.y = bbox->min.x * matrix->xy;
    minXMinY.z = bbox->min.x * matrix->xz;
    maxXMinY.x = matrix->xx * bbox->max.x;
    maxXMinY.y = bbox->max.x * matrix->xy;
    maxXMinY.z = bbox->max.x * matrix->xz;
    work.y = matrix->yy * bbox->max.y;
    work.z = matrix->yz * bbox->max.y;
    work.x = bbox->max.y * matrix->yx;
    minXMaxY.x = work.x + minXMinY.x;
    minXMaxY.y = work.y + minXMinY.y;
    minXMaxY.z = work.z + minXMinY.z;
    maxXMaxY.x = work.x + maxXMinY.x;
    maxXMaxY.y = work.y + maxXMinY.y;
    maxXMaxY.z = work.z + maxXMinY.z;
    // Add min-Y in place after saving the max-Y combinations.
    work.x = matrix->yx * bbox->min.y;
    work.y = matrix->yy * bbox->min.y;
    work.z = matrix->yz * bbox->min.y;
    minXMinY.x = work.x + minXMinY.x;
    minXMinY.y = work.y + minXMinY.y;
    minXMinY.z = work.z + minXMinY.z;
    maxXMinY.x = work.x + maxXMinY.x;
    maxXMinY.y = work.y + maxXMinY.y;
    maxXMinY.z = work.z + maxXMinY.z;
    // Reuse the contribution vector for each Z plane.
    work.x = matrix->zx;
    work.x = work.x * bbox->min.z + matrix->posX;
    work.y = matrix->zy * bbox->min.z + matrix->posY;
    work.z = matrix->zz * bbox->min.z + matrix->posZ;
    outCorners->corners[2].x = work.x + maxXMinY.x;
    outCorners->corners[2].y = work.y + maxXMinY.y;
    outCorners->corners[2].z = (savedComponent = work.z) + maxXMinY.z;
    outCorners->corners[3].x = work.x + minXMinY.x;
    outCorners->corners[3].y = work.y + minXMinY.y;
    outCorners->corners[3].z = work.z + minXMinY.z;
    outCorners->corners[6].x = maxXMaxY.x + work.x;
    outCorners->corners[6].y = maxXMaxY.y + work.y;
    outCorners->corners[6].z = maxXMaxY.z + work.z;

    outCorners->corners[7].x = minXMaxY.x + work.x;
    outCorners->corners[7].y = minXMaxY.y + work.y;
    outCorners->corners[7].z = minXMaxY.z + work.z;

    work.x = bbox->max.z * matrix->zx;
    work.y = bbox->max.z * matrix->zy;
    work.z = bbox->max.z * matrix->zz;
    work.x += matrix->posX;
    work.y += matrix->posY;
    work.z += matrix->posZ;
    outCorners->corners[0].x = work.x + minXMinY.x;
    savedComponent = work.y;
    outCorners->corners[0].y = savedComponent + minXMinY.y;
    outCorners->corners[0].z = work.z + minXMinY.z;

    outCorners->corners[1].x = work.x + maxXMinY.x;
    outCorners->corners[1].y = work.y + maxXMinY.y;
    outCorners->corners[1].z = work.z + maxXMinY.z;

    outCorners->corners[4].x = minXMaxY.x + work.x;
    outCorners->corners[4].y = minXMaxY.y + work.y;
    outCorners->corners[4].z = minXMaxY.z + work.z;

    outCorners->corners[5].x = maxXMaxY.x + work.x;
    outCorners->corners[5].y = maxXMaxY.y + work.y;
    outCorners->corners[5].z = maxXMaxY.z + work.z;
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-projectpointbatch
     * @recoil-artifact defines .text recoil:function:0x474b20: zMath::ProjectPointBatch.
     * @recoil-match byte
     *
     * Purpose: projects view-space points to screen coordinates and reciprocal-Z
     * values using the cached zMath projection globals.
     */
    void __fastcall ProjectPointBatch(const zVec3* viewPoints, zProjectedPoint* projectedPoints, int count)
    {
        do {
            projectedPoints->reciprocalZ = 1.0f / viewPoints->z;
            projectedPoints->x
                = viewPoints->x * g_zMath_ProjScaleX * projectedPoints->reciprocalZ + g_zMath_ProjOffsetX;
            projectedPoints->y
                = viewPoints->y * g_zMath_ProjScaleY * projectedPoints->reciprocalZ + g_zMath_ProjOffsetY;
            ++viewPoints;
            ++projectedPoints;
        } while (--count);
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-projectspherebatch
 * @recoil-artifact defines .text recoil:function:0x474b70: zMathProjectSphereBatch.
 * @recoil-match byte
 *
 * Purpose: projects sphere centers and scales radii using reciprocal Z and cached projection globals.
 */
void __fastcall zMathProjectSphereBatch(const zVec3* spherePoints, zProjectedSphere* projectedSpheres, int count)
{
    do {
        const float reciprocalZ = 1.0f / spherePoints->z;
        float savedReciprocalZ; // Unused saved factor; the assignment reproduces retail bytes.
        projectedSpheres->x
            = (savedReciprocalZ = reciprocalZ) * spherePoints->x * g_zMath_ProjScaleX + g_zMath_ProjOffsetX;
        projectedSpheres->y = spherePoints->y * reciprocalZ * g_zMath_ProjScaleY + g_zMath_ProjOffsetY;
        projectedSpheres->screenRadius = reciprocalZ * g_zMath_ProjSphereRadiusScale;
        ++spherePoints;
        ++projectedSpheres;
    } while (--count);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-unprojectpointbatch
 * @recoil-artifact defines .text recoil:function:0x474bc0: zMathUnprojectPointBatch
 * @recoil-match byte
 *
 * Purpose: converts projected screen coordinates with reciprocal Z back into view-space points.
 */
void __fastcall zMathUnprojectPointBatch(const zProjectedPoint* projectedPoints, zVec3* outPoints, int count)
{

    for (int i = 0; i < count; ++i) {
        outPoints[i].z = 1.0f / projectedPoints[i].reciprocalZ;
        outPoints[i].x = (projectedPoints[i].x - g_zMath_ProjOffsetX) * g_zMath_InvProjScaleX * outPoints[i].z;
        outPoints[i].y = (projectedPoints[i].y - g_zMath_ProjOffsetY) * g_zMath_InvProjScaleY * outPoints[i].z;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-unprojectpointbatchzbuf
 * @recoil-artifact defines .text recoil:function:0x474c20: zMathUnprojectPointBatchZBuf
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
 * @recoil-match byte
 *
 * Purpose: unprojects projected points and transforms them through the staged camera inverse matrix.
 * Contract note: the transform batch writes count entries into the one-element
 * viewPoints scratch, so count == 1 is the only contract the recovered storage
 * supports; larger counts are an unresolved caller contract or preserved retail
 * defect, not a validated general batch routine.
 */
void __fastcall zMathUnprojectPointBatchZBuf(const zProjectedPoint* projectedPoints, zVec3* outPoints, int count)
{
    zVec3 viewPoints[1];
    zMathUnprojectPointBatch(projectedPoints, viewPoints, count);

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float*)(&slotBuffer));
    zMath::MatLoadCameraScratchA();
    ZMTH_MAT_TRANSFORM_POINT_BATCH(viewPoints, outPoints, count);
    zMath::MatStackPopPtr();
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3directionanglesbetweenpoints
     * @recoil-artifact defines .text recoil:function:0x474d10: zMath::Vec3DirectionAnglesBetweenPoints.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-xz
     * @recoil-match byte
     *
     * Purpose: computes pitch and yaw angles from one point toward another and
     * clears roll in the output vector.
     */
    zVec3 __fastcall Vec3DirectionAnglesBetweenPoints(const zVec3* pointA, const zVec3* pointB)
    {
        zVec3 result;
        zVec3 delta;
        delta.x = pointA->x - pointB->x;
        delta.y = pointB->y - pointA->y;
        delta.z = pointA->z - pointB->z;
        result.y = atan2(delta.x, delta.z);
        float horizontalLength;
        ZMTH_VECTOR_LENGTH_XZ(horizontalLength, &delta);
        result.x = atan2(delta.y, horizontalLength);
        result.z = 0.0f;
        return result;
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-elevationanglebetweenpoints
 * @recoil-artifact defines .text recoil:function:0x474d90: zMathVec3ElevationAngleBetweenPoints.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-xz
 * @recoil-match byte
 *
 * Purpose: computes the elevation angle between two points from horizontal
 * distance and vertical delta.
 */
float __fastcall zMathVec3ElevationAngleBetweenPoints(const zVec3* pointA, const zVec3* pointB)
{
    zVec3 delta;
    delta.x = pointA->x - pointB->x;
    delta.y = pointB->y - pointA->y;
    delta.z = pointA->z - pointB->z;
    float horizontalLength;
    ZMTH_VECTOR_LENGTH_XZ(horizontalLength, &delta);
    return atan2(delta.y, horizontalLength);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-extractyaw
 * @recoil-artifact defines .text recoil:function:0x474de0: zMathMatExtractYaw.
 * @recoil-match byte
 *
 * Purpose: extracts yaw from the Z basis row of a 4x3 matrix, returning zero
 * for a degenerate horizontal basis.
 */
float __fastcall zMathMatExtractYaw(const zMat4x3* matrix)
{
    if (matrix->zx == 0.0f && matrix->zz == 0.0f) {
        return 0.0f;
    }

    return atan2(matrix->zx, matrix->zz);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-mat-extracteulerangles
 * @recoil-artifact defines .text recoil:function:0x474e10: zMathMatExtractEulerAngles
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-xz
 * @recoil-match byte
 *
 * Purpose: extracts pitch, yaw, and roll from a 4x3 rotation matrix.
 */
void __fastcall zMathMatExtractEulerAngles(const zMat4x3* matrix, zVec3* outEuler)
{
    const float yaw = zMathMatExtractYaw(matrix);
    float horizontalLength;
    ZMTH_VECTOR_LENGTH_XZ(horizontalLength, (const zVec3*)(&matrix->zx));
    const float pitch = atan2(-matrix->zy, horizontalLength);

    zVec3 rowX;
    zMath::Vec3RotateY(&rowX, (const zVec3*)(matrix), -yaw);

    zVec3 flattenedRowX;
    zMathVec3RotateX(&flattenedRowX, &rowX, -pitch);

    float rollHorizontalLength;
    ZMTH_VECTOR_LENGTH_XZ(rollHorizontalLength, &flattenedRowX);
    float roll = atan2(flattenedRowX.y, rollHorizontalLength);
    if (matrix->yy < 0.0f) {
        roll = g_zMath_ElevationPiFloat - roll;
    }

    outEuler->x = pitch;
    outEuler->y = yaw;
    outEuler->z = roll;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-rotatex
 * @recoil-artifact defines .text recoil:function:0x474ec0: zMathVec3RotateX.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-match source
 *
 * Purpose: rotates one vector around the X axis into caller-provided output.
 */
void __fastcall zMathVec3RotateX(zVec3* outVec, const zVec3* inVec, float angleX)
{
    float sinAngle;
    float cosAngle;
    zMath::SinCos(angleX, &sinAngle, &cosAngle);
    outVec->x = inVec->x;
    // inVec may alias outVec, so y is kept until z has read inVec->y.
    const float rotatedY = cosAngle * inVec->y - sinAngle * inVec->z;
    outVec->z = sinAngle * inVec->y + cosAngle * inVec->z;
    outVec->y = rotatedY;
}

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3rotatey-gamezrecoil-zmath-zmath-vec-cpp
     * @recoil-artifact defines .text recoil:function:0x474f40: zMath::Vec3RotateY (GameZRecoil/zMath/zmath_vec.cpp).
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
     * @recoil-match byte
     *
     * One C-linkage implementation (VC5 decorated name @Vec3RotateY@12) in
     * the angle-last parameter order of the sibling zMathVec3RotateX: outVec
     * in ECX, inVec in EDX and the stack float with four-byte callee cleanup.
     * Under canonical VC5 the angle-last declaration reproduces the eleven
     * player.cpp and zmth_main.c retail call sequences. ai_net.cpp selects the
     * angle-first declaration view in zmth_decls.h, which lowers to the same
     * fastcall interface and reproduces 0x4036bd and 0x4036cc; the C-linkage
     * name does not encode parameter order, so both views reference this one
     * body. Inferred from the retail argument sequences and the retail
     * zmth_main.c source path; the original declaration arrangement is not
     * established, and this model does not accept the callee's body.
     * Purpose: Rotates an input vector around the Y axis and copies the original Y component to the output.
     */
    extern "C" void __fastcall Vec3RotateY(zVec3 * outVec, const zVec3* inVec, float yawAngle)
    {
        float sinAngle;
        float cosAngle;
        SinCos(yawAngle, &sinAngle, &cosAngle);
        // inVec may alias outVec, so x is kept until z has read inVec->x.
        const float rotatedX = sinAngle * inVec->z + cosAngle * inVec->x;
        outVec->y = inVec->y;
        outVec->z = cosAngle * inVec->z - sinAngle * inVec->x;
        outVec->x = rotatedX;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-approxexpneg
     * @recoil-artifact defines .text recoil:function:0x474fc0: zMath::ApproxExpNeg.
     * @recoil-match byte
     *
     * Purpose: lazily builds and samples the 256-entry approximate e^-x lookup
     * table with edge clamps for negative and out-of-range inputs.
     */
    float __stdcall ApproxExpNeg(float x)
    {
        if (g_zMath_ApproxExpNegDirty != 0) {
            g_zMath_ApproxExpNegScale = 51.0f;
            for (int i = 0; i < 256; ++i) {
                g_zMath_ApproxExpNegTable[i] = exp(-((float)i / 51.0f));
            }
            g_zMath_ApproxExpNegDirty = 0;
        }

        if (x > 5.0f) {
            return 0.0f;
        }
        if (x < 0.0) {
            return 1.0f;
        }
        const float scale = g_zMath_ApproxExpNegScale;
        const int tableIndex = (int)(scale * x);
        return g_zMath_ApproxExpNegTable[tableIndex];
    }
} // namespace zMath

namespace zMath
{
    /**
     * Purpose: Inline-function spelling of the reviewed vector-cross island for
     * this unit's consumers. VC5 binds simple variable arguments to their own
     * homes and address arguments to inline-parameter homes, which the capturing
     * ZMTH_VECTOR_CROSS cannot express. Original header ownership is unrecovered.
     * Original inline helper evidence: no standalone retail function; observed at
     * retail 0x475070's cross-product island bound to the caller's homes.
     */
    inline void Vec3Cross(const zVec3* left, const zVec3* right, zVec3* dest)
    {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        ZMTH_VECTOR_CROSS_BODY(left, right, dest);
#else
        ZMTH_VECTOR_CROSS(left, right, dest);
#endif
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-vec3-trianglenormal
 * @recoil-artifact defines .text recoil:function:0x475070: zMathVec3TriangleNormal.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-cross
 * @recoil-match byte
 *
 * Purpose: Computes a normalized triangle normal from the triangle edge cross product.
 */
void __fastcall zMathVec3TriangleNormal(const zVec3* p0, const zVec3* p1, const zVec3* p2, zVec3* outNormal)
{
    zVec3 edges[2];
    zMath::Vec3Subtract(p1, p0, &edges[0]);
    zMath::Vec3Subtract(p2, p0, &edges[1]);
    zMath::Vec3Cross(&edges[0], &edges[1], outNormal);
    zMath::Vec3Normalize(outNormal);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-solvelineargradient2d
 * @recoil-artifact defines .text recoil:function:0x475130: zMathSolveLinearGradient2D
 * @recoil-match byte
 *
 * Purpose: solves the screen-space linear gradient of a scalar over a triangle.
 * Data: reads the distinct shared zMath zero double at 0x4d2970
 * (0.0) and unit float at 0x4d297c; writes only the two
 * caller-supplied output floats.
 */
void __fastcall zMathSolveLinearGradient2D(
    float* outDuDx,
    float* outDuDy,
    float ax,
    float ay,
    float bx,
    float by,
    float cx,
    float cy,
    float ua,
    float ub,
    float uc
)
{
    zVec3 edgeA, edgeC;
    edgeA.x = ax - bx;
    edgeA.y = ay - by;
    edgeC.x = cx - bx;
    edgeC.y = cy - by;
    const float determinant = edgeC.y * edgeA.x - edgeC.x * edgeA.y;
    if (determinant != 0.0) {
        edgeA.z = ua - ub;
        edgeC.z = uc - ub;
        const float invDeterminant = 1.0f / determinant;
        *outDuDx = -(edgeA.y * edgeC.z - edgeC.y * edgeA.z) * invDeterminant;
        *outDuDy = -(edgeC.x * edgeA.z - edgeA.x * edgeC.z) * invDeterminant;
    } else {
        *outDuDx = 0.0f;
        *outDuDy = 0.0f;
    }
}

namespace zMath
{
    /**
     * Purpose: Inline-function spelling of the reviewed vector-length-sq island
     * for this unit's consumers, binding its argument like Vec3Dot. Original
     * header ownership is unrecovered. Original inline helper evidence: no
     * standalone retail function; retail 0x475210 loads sphereCenterRelSegB
     * from its parameter home and 0x4753e0 loads captured edge addresses.
     */
    inline float Vec3LengthSq(const zVec3* vector)
    {
        float result;
        ZMTH_VECTOR_LENGTH_SQ_BOUND(result, vector);
        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-linevsspherehit
     * @recoil-artifact defines .text recoil:function:0x475210: zMath::LineVsSphereHit
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-sq
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.line-vs-sphere-hit.fast-sqrt-estimate recoil:function:0x475210
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmath.line-vs-sphere-hit.fast-sqrt-estimate
     * @recoil-match byte
     *
     * Raw assembly: one in-body 13-byte fast-sqrt estimate island at retail
     * [0x475328,0x475335).
     *
     * Purpose: tests a segment direction against a sphere and writes the
     * normalized inward hit normal when the hit lies in front of the segment
     * origin.
     * Data: reads the shared zMath zero scalar at 0x4d2960 for x87 comparisons;
     * writes only the caller-supplied output normal.
     */
    int __fastcall LineVsSphereHit(
        const zVec3* segA,
        const zVec3* segB,
        float radius,
        const zVec3* sphereCenterRelSegB,
        zVec3* outInwardNormal
    )
    {
        float centerDistMinusRadius;
        zVec3 lineDelta;
        float lineLengthSq;
        float centerDotLine;
        float rootNumerator;
        float centerDistSq;
        float discriminant;
        float discriminantRoot;
        float denominator;
        float hitScale;

        Vec3Subtract(segA, segB, &lineDelta);
        lineLengthSq = Vec3LengthSq(&lineDelta);
        if (lineLengthSq == 0.0f) {
            return 0;
        }

        centerDotLine = Vec3Dot(sphereCenterRelSegB, &lineDelta);
        rootNumerator = centerDotLine;
        centerDistSq = Vec3LengthSq(sphereCenterRelSegB);
        centerDistMinusRadius = centerDistSq - radius * radius;
        if (centerDistMinusRadius == 0.0f) {
            if (centerDotLine <= 0.0f) {
                return 0;
            }
            hitScale = (centerDotLine + centerDotLine) / lineLengthSq;
        } else {
            discriminant = centerDotLine * centerDotLine - lineLengthSq * centerDistMinusRadius;
            if (discriminant < 0.0f) {
                return 0;
            }

            // Raw-assembly fast square-root estimate: retail transforms the named discriminant
            // bits through EAX ((bits >> 1) + 0x1fc00000) into the named result local.
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
            __asm {
                mov eax, discriminant
                sar eax, 1
                add eax, 01fc00000h
                mov discriminantRoot, eax
            }
#else
            {
                int estimateBits;
                memcpy(&estimateBits, &discriminant, sizeof estimateBits);
                estimateBits = (estimateBits >> 1) + 0x1fc00000;
                memcpy(&discriminantRoot, &estimateBits, sizeof discriminantRoot);
            }
#endif
            if (centerDistMinusRadius < 0.0f)
            {
                centerDistMinusRadius = -centerDistMinusRadius;
                rootNumerator = -centerDotLine;
            }

            if (rootNumerator > discriminantRoot) {
                denominator = rootNumerator - discriminantRoot;
            } else {
                denominator = rootNumerator + discriminantRoot;
                if (denominator <= 0.0f) {
                    return 0;
                }
            }

            hitScale = centerDistMinusRadius / denominator;
        }

        lineDelta.x *= hitScale;
        lineDelta.y *= hitScale;
        lineDelta.z *= hitScale;
        Vec3Subtract(sphereCenterRelSegB, &lineDelta, outInwardNormal);
        Vec3Normalize(outInwardNormal);
        return 1;
    }
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-buildperspectivetextureinterpolants
 * @recoil-artifact defines .text recoil:function:0x4753e0: zMathBuildPerspectiveTextureInterpolants
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-cross
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-sq
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match source
 *
 * Purpose: recovers perspective-correct reciprocal-Z and UV-over-Z plane gradients for a triangle.
 */
void __fastcall zMathBuildPerspectiveTextureInterpolants(
    const zVec3* triVerts,
    const zVec2* triUVs,
    zVec2* outRecipZGrad,
    float* outRecipZBase,
    zVec2* outUOverZGrad,
    float* outUOverZBase,
    zVec2* outVOverZGrad,
    float* outVOverZBase
)
{
    float invGram;
    zVec3 edge12;
    zVec3 edge10;
    zVec3 normal;
    zVec3 scaled12;
    zVec3 plane;
    float normalDotOrigin;
    float edge12LenSq;
    float edge10LenSq;
    float edgeDot;
    float planeDotOrigin;

    zMath::Vec3Subtract(&triVerts[2], &triVerts[1], &edge12);
    zMath::Vec3Subtract(triVerts, &triVerts[1], &edge10);
    zMath::Vec3Cross(&edge12, &edge10, &normal);
    normalDotOrigin = zMath::Vec3Dot(&normal, triVerts);
    if (normalDotOrigin == 0.0f) {
        outRecipZGrad->x = 0.0f;
        outRecipZGrad->y = 0.0f;
        *outRecipZBase = 1000.0f;
    } else {
        const float reciprocalNormalDot = 1.0f / normalDotOrigin;
        outRecipZGrad->x = reciprocalNormalDot * normal.x * g_zMath_InvProjScaleX;
        outRecipZGrad->y = reciprocalNormalDot * normal.y * g_zMath_InvProjScaleY;
        *outRecipZBase = reciprocalNormalDot * normal.z;
    }

    edge12LenSq = zMath::Vec3LengthSq(&edge12);
    edge10LenSq = zMath::Vec3LengthSq(&edge10);
    edgeDot = zMath::Vec3Dot(&edge12, &edge10);
    const float gramDeterminant = edge10LenSq * edge12LenSq - edgeDot * edgeDot;
    if (gramDeterminant == 0.0f) {
        outUOverZGrad->x = 0.0f;
        outUOverZGrad->y = 0.0f;
        *outUOverZBase = 0.0f;
        outVOverZGrad->x = 0.0f;
        outVOverZGrad->y = 0.0f;
        *outVOverZBase = 0.0f;
        return;
    }

    invGram = 1.0f / gramDeterminant;
    edge12LenSq *= invGram;
    edgeDot *= invGram;
    edge10LenSq *= invGram;

    const float uDelta12 = triUVs[2].x - triUVs[1].x;
    const float uDelta10 = triUVs[0].x - triUVs[1].x;
    const float uScale12 = uDelta12 * edge10LenSq - uDelta10 * edgeDot;
    scaled12.x = uScale12 * edge12.x;
    scaled12.y = uScale12 * edge12.y;
    scaled12.z = uScale12 * edge12.z;
    const float uScale10 = uDelta10 * edge12LenSq - uDelta12 * edgeDot;
    plane.x = uScale10 * edge10.x;
    plane.y = uScale10 * edge10.y;
    plane.z = uScale10 * edge10.z;
    zMath::Vec3Add(&plane, &scaled12, &plane);
    planeDotOrigin = zMath::Vec3Dot(&plane, triVerts);
    const float uOriginDelta = triUVs[0].x - planeDotOrigin;
    outUOverZGrad->x = uOriginDelta * outRecipZGrad->x + plane.x * g_zMath_InvProjScaleX;
    outUOverZGrad->y = uOriginDelta * outRecipZGrad->y + plane.y * g_zMath_InvProjScaleY;
    *outUOverZBase = uOriginDelta * *outRecipZBase + plane.z;

    const float vDelta12 = triUVs[2].y - triUVs[1].y;
    const float vDelta10 = triUVs[0].y - triUVs[1].y;
    const float vScale12 = vDelta12 * edge10LenSq - vDelta10 * edgeDot;
    scaled12.x = vScale12 * edge12.x;
    scaled12.y = vScale12 * edge12.y;
    scaled12.z = vScale12 * edge12.z;
    const float vScale10 = vDelta10 * edge12LenSq - vDelta12 * edgeDot;
    plane.x = vScale10 * edge10.x;
    plane.y = vScale10 * edge10.y;
    plane.z = vScale10 * edge10.z;
    zMath::Vec3Add(&plane, &scaled12, &plane);
    planeDotOrigin = zMath::Vec3Dot(&plane, triVerts);
    const float vOriginDelta = triUVs[0].y - planeDotOrigin;
    outVOverZGrad->x = vOriginDelta * outRecipZGrad->x + plane.x * g_zMath_InvProjScaleX;
    outVOverZGrad->y = vOriginDelta * outRecipZGrad->y + plane.y * g_zMath_InvProjScaleY;
    *outVOverZBase = vOriginDelta * *outRecipZBase + plane.z;
}
