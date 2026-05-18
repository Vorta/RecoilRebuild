/*
 * zmth_main.c -- zMath subsystem: vector, matrix, quaternion, projection math
 * Original: D:\Proj\GameZRecoil\zMath\zmth_main.c
 *
 * Reconstructed from Recoil.exe (1998 Zipper Interactive)
 * Binary Ninja analysis of functions at 0x402F60, 0x4726D0-0x475B80
 *
 * Math conventions:
 *   - Vec3:   float[3] = { x, y, z }
 *   - Quat4:  float[4] = { w, x, y, z }
 *   - Mat4x3: float[12] stored row-major, 3 rows of 4 columns (48 bytes / 0x30)
 *             Row 0: [m00 m01 m02 tx]   indices [0..3]
 *             Row 1: [m10 m11 m12 ty]   indices [4..7]
 *             Row 2: [m20 m21 m22 tz]   indices [8..11]
 *   - Coordinate system: Y-up, left-handed (typical DirectX 6 era)
 *   - Angles are in radians
 *   - Fast inverse sqrt via bit hack: (*(int*)&x >> 1) + 0x1FC00000
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* ---- Constants ---- */

#define SLERP_THRESHOLD 0.95f /* dot product threshold for slerp fallback */
#define FOG_TABLE_SIZE 256
#define FOG_SCALE 0.0196078438f /* ~= 1/51 or 5/255 */

/* ---- Types ---- */

typedef float Vec3[3];
typedef float Quat4[4];
typedef float Mat4x3[12]; /* 3x4 row-major: 3 rotation rows + translation column */

/* ---- Globals ---- */

extern float *g_zMath_CurrentMatrixPtr;    /* pointer to current matrix on stack */
extern int *g_zMath_MatStackDirtyPtr;      /* pointer to current dirty flag on stack */
extern float g_zMath_ViewMatrix[12];       /* 0x566920: global view matrix */
extern float g_zMath_ProjectionMatrix[12]; /* 0x5668E8: global projection matrix */

extern float g_zMath_ProjScaleX;    /* 0x566850: screen projection scale X */
extern float g_zMath_ProjScaleY;    /* 0x566854: screen projection scale Y */
extern float g_zMath_ProjOffsetX;   /* 0x5761E0: screen center offset X */
extern float g_zMath_ProjOffsetY;   /* 0x5761E4: screen center offset Y */
extern float g_zMath_InvProjScaleX; /* 0x566860: inverse projection scale X */
extern float g_zMath_InvProjScaleY; /* 0x566864: inverse projection scale Y */
extern float g_zMath_ZBufScale;     /* 0x566840: Z-buffer depth scale */

extern float g_zMath_TempVec3[3]; /* 0x566420: temporary Vec3 */

extern float g_zMath_FogTable[FOG_TABLE_SIZE]; /* 0x566438: exp decay lookup */
extern float g_zMath_FogRange;                 /* 0x5669D0: fog max range (51.0) */
extern int g_zMath_FogTableDirty;              /* 0x4E0E8C: rebuild flag */

/* ==================================================================
 * SECTION 1: VECTOR OPERATIONS
 * ================================================================== */

/* ==================================================================
 * zMath_Vec3_Normalize -- 0x402F60
 * Normalizes a Vec3 in-place. Returns the original length.
 * If length is zero, vector is unchanged.
 * NOTE: Located outside main zMath range (early .text).
 * ================================================================== */
float __cdecl zMath_Vec3_Normalize(float *v) {
    float len = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (len != 0.0f) {
        float inv = 1.0f / len;
        v[0] *= inv;
        v[1] *= inv;
        v[2] *= inv;
    }
    return len;
}

/* ==================================================================
 * zMath_Vec3_Distance -- 0x4726D0
 * Returns the distance between two 3D points.
 * ================================================================== */
float __cdecl zMath_Vec3_Distance(const float *a, const float *b) {
    float dx = a[0] - b[0];
    float dy = a[1] - b[1];
    float dz = a[2] - b[2];
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

/* ==================================================================
 * zMath_Vec3_DistSqXZ -- 0x472730
 * Returns the squared distance in the XZ plane (ignores Y).
 * ================================================================== */
float __cdecl zMath_Vec3_DistSqXZ(const float *a, const float *b) {
    float dx = a[0] - b[0];
    float dz = a[2] - b[2];
    return dx * dx + dz * dz;
}

/* ==================================================================
 * zMath_Vec3_ScaleAdd -- 0x472770
 * out = a + b * t
 * ================================================================== */
void __cdecl zMath_Vec3_ScaleAdd(float *out, const float *a, const float *b, float t) {
    out[0] = a[0] + b[0] * t;
    out[1] = a[1] + b[1] * t;
    out[2] = a[2] + b[2] * t;
}

/* ==================================================================
 * zMath_Vec3_DivScalar -- 0x4727A0
 * out = v / s, with zero-division protection (returns unchanged).
 * ================================================================== */
void __cdecl zMath_Vec3_DivScalar(float *out, const float *v, float s) {
    if (s == 0.0f) {
        out[0] = v[0];
        out[1] = v[1];
        out[2] = v[2];
        return;
    }
    float inv = 1.0f / s;
    out[0] = v[0] * inv;
    out[1] = v[1] * inv;
    out[2] = v[2] * inv;
}

/* ==================================================================
 * zMath_Vec3_NormalizeXZ -- 0x4727F0
 * Normalizes a Vec3 in the XZ plane only (Y set to 0).
 * ================================================================== */
void __cdecl zMath_Vec3_NormalizeXZ(float *v) {
    float len = sqrtf(v[0] * v[0] + v[2] * v[2]);
    if (len != 0.0f) {
        float inv = 1.0f / len;
        v[0] *= inv;
        v[2] *= inv;
    }
    v[1] = 0.0f;
}

/* ==================================================================
 * zMath_Vec3_Reflect -- 0x472860
 * Reflects vector 'dir' off a surface with normal 'n'.
 * out = dir - 2 * dot(dir, n) * n
 * ================================================================== */
void __cdecl zMath_Vec3_Reflect(float *out, const float *dir, const float *n) {
    float dot = dir[0] * n[0] + dir[1] * n[1] + dir[2] * n[2];
    float factor = 2.0f * dot;
    out[0] = dir[0] - factor * n[0];
    out[1] = dir[1] - factor * n[1];
    out[2] = dir[2] - factor * n[2];
}

/* ==================================================================
 * zMath_Vec3_Lerp -- 0x472960
 * out = a + (b - a) * t  (linear interpolation)
 * ================================================================== */
void __cdecl zMath_Vec3_Lerp(float *out, const float *a, const float *b, float t) {
    out[0] = a[0] + (b[0] - a[0]) * t;
    out[1] = a[1] + (b[1] - a[1]) * t;
    out[2] = a[2] + (b[2] - a[2]) * t;
}

/* ==================================================================
 * zMath_Vec3_DirectionTo -- 0x4729B0
 * Computes normalized direction from 'from' to 'to'.
 * ================================================================== */
void __cdecl zMath_Vec3_DirectionTo(float *out, const float *from, const float *to) {
    out[0] = to[0] - from[0];
    out[1] = to[1] - from[1];
    out[2] = to[2] - from[2];
    zMath_Vec3_Normalize(out);
}

/* ==================================================================
 * zMath_Vec3_LerpNormalize -- 0x4729F0
 * Lerp then normalize. Used for blending directions.
 * ================================================================== */
void __cdecl zMath_Vec3_LerpNormalize(float *out, const float *a, const float *b, float t) {
    out[0] = a[0] + (b[0] - a[0]) * t;
    out[1] = a[1] + (b[1] - a[1]) * t;
    out[2] = a[2] + (b[2] - a[2]) * t;
    zMath_Vec3_Normalize(out);
}

/* ==================================================================
 * zMath_Vec3_Slerp -- 0x472A10
 * Spherical linear interpolation between two unit vectors.
 * Falls back to lerp+normalize when vectors are nearly parallel
 * (dot > +/-SLERP_THRESHOLD).
 * ================================================================== */
void __cdecl zMath_Vec3_Slerp(float *out, const float *a, const float *b, float t) {
    float dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];

    /* Clamp and handle near-parallel case */
    if (dot > SLERP_THRESHOLD || dot < -SLERP_THRESHOLD) {
        zMath_Vec3_LerpNormalize(out, a, b, t);
        return;
    }

    float theta = acosf(dot);
    float sinTheta = sinf(theta);
    float wa = sinf((1.0f - t) * theta) / sinTheta;
    float wb = sinf(t * theta) / sinTheta;

    out[0] = wa * a[0] + wb * b[0];
    out[1] = wa * a[1] + wb * b[1];
    out[2] = wa * a[2] + wb * b[2];
}

/* ==================================================================
 * zMath_Vec3_Perp2D -- 0x472CC0
 * Returns 2D perpendicular in the XY plane: out = (-v.y, v.x, 0).
 * ================================================================== */
void __cdecl zMath_Vec3_Perp2D(float *out, const float *v) {
    out[0] = -v[1];
    out[1] = v[0];
    out[2] = 0.0f;
}

/* ==================================================================
 * zMath_Vec3_PerpXZ -- 0x4745C0
 * Returns perpendicular in the XZ plane: out = (-v.z, 0, v.x).
 * ================================================================== */
void __cdecl zMath_Vec3_PerpXZ(float *out, const float *v) {
    out[0] = -v[2];
    out[1] = 0.0f;
    out[2] = v[0];
}

/* ==================================================================
 * zMath_Vec3_ScaleAddBatch -- 0x4744F0
 * Batch version: out[i] = a[i] + b[i] * t, for 'count' Vec3s.
 * ================================================================== */
void __cdecl zMath_Vec3_ScaleAddBatch(float *out, const float *a, const float *b, float t,
                                      int count) {
    for (int i = 0; i < count; i++) {
        out[0] = a[0] + b[0] * t;
        out[1] = a[1] + b[1] * t;
        out[2] = a[2] + b[2] * t;
        out += 3;
        a += 3;
        b += 3;
    }
}

/* ==================================================================
 * zMath_Vec3_RotateAroundY -- 0x474F40
 * Rotates vector 'in' around the Y axis by 'angle' radians.
 * out.x = cos*in.x + sin*in.z
 * out.y = in.y
 * out.z = -sin*in.x + cos*in.z
 * ================================================================== */
void __cdecl zMath_Vec3_RotateAroundY(float *out, const float *in, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    out[0] = c * in[0] + s * in[2];
    out[1] = in[1];
    out[2] = -s * in[0] + c * in[2];
}

/* ==================================================================
 * zMath_Vec3_RotateAroundX -- 0x474EC0
 * Rotates vector 'in' around the X axis by 'angle' radians.
 * out.x = in.x
 * out.y = cos*in.y - sin*in.z
 * out.z = sin*in.y + cos*in.z
 * ================================================================== */
void __cdecl zMath_Vec3_RotateAroundX(float *out, const float *in, float angle) {
    float c = cosf(angle);
    float s = sinf(angle);
    out[0] = in[0];
    out[1] = c * in[1] - s * in[2];
    out[2] = s * in[1] + c * in[2];
}

/* ==================================================================
 * zMath_Vec3_DirectionAngles -- 0x474D10
 * Computes (pitch, yaw, 0) angles from 'from' looking toward 'to'.
 * out[0] = pitch = atan2(horizontalDist, dy)
 * out[1] = yaw   = atan2(dz, dx)
 * out[2] = 0 (no roll)
 * ================================================================== */
void __cdecl zMath_Vec3_DirectionAngles(const float *from, const float *to, float *out) {
    float dx = from[0] - to[0];
    float dy = to[1] - from[1];
    float dz = from[2] - to[2];

    out[1] = atan2f(dz, dx);
    out[0] = atan2f(sqrtf(dx * dx + dz * dz), dy);
    out[2] = 0.0f;
}

/* ==================================================================
 * zMath_Vec3_ElevationAngle -- 0x474D90
 * Returns the elevation/pitch angle from 'from' toward 'to'.
 * = atan2(horizontalDist, dy)
 * ================================================================== */
float __cdecl zMath_Vec3_ElevationAngle(const float *from, const float *to) {
    float dx = from[0] - to[0];
    float dz = from[2] - to[2];
    float dy = to[1] - from[1];
    return atan2f(sqrtf(dx * dx + dz * dz), dy);
}

/* ==================================================================
 * SECTION 2: ERROR HANDLER
 * ================================================================== */

/* ==================================================================
 * zMath_Main_MathErrHandler -- 0x472D30
 * Handles floating-point math errors (set via _matherr).
 * In retail build this is a no-op/stub.
 * ================================================================== */
int __cdecl zMath_Main_MathErrHandler(void *exception) {
    /* Retail: stub/no-op */
    return 0;
}

/* ==================================================================
 * SECTION 3: MATRIX STACK
 * ================================================================== */

/* ==================================================================
 * zMath_GetProjectedScreenSize -- 0x472ED0
 * Returns the projected screen-space size of an object at distance z.
 * ================================================================== */
float __cdecl zMath_GetProjectedScreenSize(float worldSize, float z);

/* ==================================================================
 * zMath_MatStack_Push -- 0x472EF0
 * Pushes the current matrix, copies it to the new top of stack.
 * ================================================================== */
void __cdecl zMath_MatStack_Push(void) {
    memcpy(g_zMath_CurrentMatrixPtr + 12, g_zMath_CurrentMatrixPtr, 0x30);
    g_zMath_CurrentMatrixPtr += 12;
    g_zMath_MatStackDirtyPtr++;
    *g_zMath_MatStackDirtyPtr = g_zMath_MatStackDirtyPtr[-1]; /* copy dirty flag */
}

/* ==================================================================
 * zMath_MatStack_PushNew -- 0x472F30
 * Pushes and marks the new matrix as "clean" (not dirty).
 * ================================================================== */
void __cdecl zMath_MatStack_PushNew(void) {
    g_zMath_CurrentMatrixPtr += 12;
    g_zMath_MatStackDirtyPtr++;
    *g_zMath_MatStackDirtyPtr = 0;
}

/* ==================================================================
 * zMath_MatStack_Pop -- 0x472F60
 * Pops the matrix stack by one level.
 * ================================================================== */
void __cdecl zMath_MatStack_Pop(void) {
    g_zMath_CurrentMatrixPtr -= 12;
    g_zMath_MatStackDirtyPtr--;
}

/* ==================================================================
 * zMath_Mat_LoadProjection -- 0x472F90
 * Copies the global projection matrix into the given destination.
 * ================================================================== */
void __cdecl zMath_Mat_LoadProjection(float *dst) {
    memcpy(dst, g_zMath_ProjectionMatrix, 0x30);
}

/* ==================================================================
 * zMath_Mat_LoadView -- 0x472FA0
 * Copies the global view matrix into the given destination.
 * ================================================================== */
void __cdecl zMath_Mat_LoadView(float *dst) {
    memcpy(dst, g_zMath_ViewMatrix, 0x30);
}

/* ==================================================================
 * SECTION 4: CORE MATRIX OPERATIONS
 * ================================================================== */

/* ==================================================================
 * zMath_Mat_SetupCamera -- 0x472FB0
 * Sets up the camera by computing view and projection matrices from
 * the given world transform. Stores results in global view/projection.
 * ================================================================== */
void __cdecl zMath_Mat_SetupCamera(const float *worldTransform);

/* ==================================================================
 * zMath_Mat_TransformPoint -- 0x473060
 * Transforms a single point by a 4x3 matrix (rotation + translation).
 * out = M * in + T
 * ================================================================== */
void __cdecl zMath_Mat_TransformPoint(float *out, const float *mat, const float *in) {
    out[0] = mat[0] * in[0] + mat[1] * in[1] + mat[2] * in[2] + mat[3];
    out[1] = mat[4] * in[0] + mat[5] * in[1] + mat[6] * in[2] + mat[7];
    out[2] = mat[8] * in[0] + mat[9] * in[1] + mat[10] * in[2] + mat[11];
}

/* ==================================================================
 * zMath_Mat_Load -- 0x473250
 * Copies a source matrix (48 bytes) into the current stack matrix.
 * Marks dirty flag.
 * ================================================================== */
void __cdecl zMath_Mat_Load(const float *src) {
    memcpy(g_zMath_CurrentMatrixPtr, src, 0x30);
    *g_zMath_MatStackDirtyPtr = 1;
}

/* ==================================================================
 * zMath_Mat_LoadIdentity -- 0x4732F0
 * Sets the current stack matrix to identity.
 *   [1 0 0 0]
 *   [0 1 0 0]
 *   [0 0 1 0]
 * ================================================================== */
void __cdecl zMath_Mat_LoadIdentity(void) {
    memset(g_zMath_CurrentMatrixPtr, 0, 0x30);
    g_zMath_CurrentMatrixPtr[0] = 1.0f;
    g_zMath_CurrentMatrixPtr[5] = 1.0f;
    g_zMath_CurrentMatrixPtr[10] = 1.0f;
    *g_zMath_MatStackDirtyPtr = 1;
}

/* ==================================================================
 * zMath_Mat_Multiply -- 0x473370
 * Multiplies the current matrix by the given matrix (right-multiply).
 * current = current * rhs
 * 3x3 rotation block multiplied, translation accumulated.
 * ================================================================== */
void __cdecl zMath_Mat_Multiply(const float *rhs);

/* ==================================================================
 * zMath_Mat_Scale -- 0x473690
 * Scales the current matrix by (sx, sy, sz).
 * ================================================================== */
void __cdecl zMath_Mat_Scale(float sx, float sy, float sz);

/* ==================================================================
 * zMath_Mat_Translate -- 0x4737E0
 * Applies a translation (tx, ty, tz) to the current matrix.
 * ================================================================== */
void __cdecl zMath_Mat_Translate(float tx, float ty, float tz);

/* ==================================================================
 * zMath_Mat_RotateX -- 0x473970
 * Applies an X-axis rotation by 'angle' radians to the current matrix.
 * ================================================================== */
void __cdecl zMath_Mat_RotateX(float angle);

/* ==================================================================
 * zMath_Mat_RotateY -- 0x473B10
 * Applies a Y-axis rotation by 'angle' radians to the current matrix.
 * ================================================================== */
void __cdecl zMath_Mat_RotateY(float angle);

/* ==================================================================
 * zMath_Mat_RotateZ -- 0x473CC0
 * Applies a Z-axis rotation by 'angle' radians to the current matrix.
 * ================================================================== */
void __cdecl zMath_Mat_RotateZ(float angle);

/* ==================================================================
 * SECTION 5: ADVANCED MATRIX OPERATIONS
 * ================================================================== */

/* ==================================================================
 * zMath_Mat_BuildViewFromWorld -- 0x473E60
 * Inverts a world transform to produce a view matrix.
 * Rotation transposed, translation negated and rotated.
 * ================================================================== */
void __cdecl zMath_Mat_BuildViewFromWorld(float *viewOut, const float *world);

/* ==================================================================
 * zMath_Mat_TransformPointBatch -- 0x473FC0
 * Transforms an array of Vec3 points by the given matrix.
 * ================================================================== */
void __cdecl zMath_Mat_TransformPointBatch(float *outArray, const float *mat, const float *inArray,
                                           int count);

/* ==================================================================
 * zMath_Mat_BuildFromEulerTRS -- 0x474010
 * Builds a 4x3 matrix from Euler angles (X,Y,Z), translation, and scale.
 * ================================================================== */
void __cdecl zMath_Mat_BuildFromEulerTRS(float *mat, float rx, float ry, float rz, float tx,
                                         float ty, float tz, float sx, float sy, float sz);

/* ==================================================================
 * zMath_Mat_BuildFromAngles -- 0x474260
 * Builds a rotation matrix from Euler angles (pitch, yaw, roll).
 * ================================================================== */
void __cdecl zMath_Mat_BuildFromAngles(float *mat, float pitch, float yaw, float roll);

/* ==================================================================
 * zMath_SetScreenSize -- 0x4743E0
 * Stores screen dimensions for projection calculations.
 * ================================================================== */
void __cdecl zMath_SetScreenSize(int width, int height);

/* ==================================================================
 * zMath_SetViewport -- 0x474400
 * Sets projection scale and offset globals from viewport dimensions.
 * Also computes inverse projection scales for unprojection.
 * ================================================================== */
void __cdecl zMath_SetViewport(float left, float top, float right, float bottom, float nearZ,
                               float farZ);

/* ==================================================================
 * zMath_Mat_ExtractAnglesDown -- 0x474580
 * Extracts pitch/heading from a matrix by examining the forward (-Z) column.
 * ================================================================== */
void __cdecl zMath_Mat_ExtractAnglesDown(const float *mat, float *outAngles);

/* ==================================================================
 * zMath_Mat_ExtractYaw -- 0x474DE0
 * Extracts the yaw (heading around Y) from a rotation matrix.
 * Returns atan2(m20, m00).
 * ================================================================== */
float __cdecl zMath_Mat_ExtractYaw(const float *mat);

/* ==================================================================
 * zMath_Mat_ExtractEulerAngles -- 0x474E10
 * Extracts all three Euler angles (pitch, yaw, roll) from a rotation matrix.
 * ================================================================== */
void __cdecl zMath_Mat_ExtractEulerAngles(const float *mat, float *outAngles);

/* ==================================================================
 * SECTION 6: NORMAL/BBOX TRANSFORMS
 * ================================================================== */

/* ==================================================================
 * zMath_Mat_TransformNormalBatch -- 0x4745E0
 * Transforms an array of normals by the 3x3 rotation part of a matrix
 * (no translation). Source and destination are separate arrays.
 * ================================================================== */
void __cdecl zMath_Mat_TransformNormalBatch(float *outNormals, const float *mat,
                                            const float *inNormals, int count);

/* ==================================================================
 * zMath_Mat_TransformNormalBatchInPlace -- 0x474670
 * In-place version: transforms normals in the same array.
 * ================================================================== */
void __cdecl zMath_Mat_TransformNormalBatchInPlace(float *normals, const float *mat, int count);

/* ==================================================================
 * zMath_Mat_TransformNormalBatchToOut -- 0x474710
 * Transforms normals from source array to a separate output array.
 * (Similar to TransformNormalBatch but with different stride/layout.)
 * ================================================================== */
void __cdecl zMath_Mat_TransformNormalBatchToOut(float *outNormals, const float *mat,
                                                 const float *inNormals, int count);

/* ==================================================================
 * zMath_BBox_Transform -- 0x474870
 * Transforms an axis-aligned bounding box by a matrix.
 * Produces a new AABB that encloses the transformed corners.
 * ================================================================== */
void __cdecl zMath_BBox_Transform(float *outMin, float *outMax, const float *mat,
                                  const float *inMin, const float *inMax);

/* ==================================================================
 * SECTION 7: PERSPECTIVE PROJECTION
 * ================================================================== */

/* ==================================================================
 * zMath_ProjectPointBatch -- 0x474B20
 * Projects a batch of 3D points to 2D screen coordinates.
 * For each point:
 *   invZ    = 1.0 / src.z
 *   dst.x   = src.x * ProjScaleX * invZ + ProjOffsetX
 *   dst.y   = src.y * ProjScaleY * invZ + ProjOffsetY
 *   dst.z   = invZ   (raw reciprocal depth)
 * ================================================================== */
void __cdecl zMath_ProjectPointBatch(const float *srcArray, float *dstArray, int count) {
    for (int i = 0; i < count; i++) {
        float invZ = 1.0f / srcArray[2];
        dstArray[0] = srcArray[0] * g_zMath_ProjScaleX * invZ + g_zMath_ProjOffsetX;
        dstArray[1] = srcArray[1] * g_zMath_ProjScaleY * invZ + g_zMath_ProjOffsetY;
        dstArray[2] = invZ;
        srcArray += 3;
        dstArray += 3;
    }
}

/* ==================================================================
 * zMath_ProjectPointBatchZBuf -- 0x474B70
 * Same as ProjectPointBatch but scales depth for Z-buffer:
 *   dst.z = invZ * ZBufScale
 * Used for hardware (D3D) rendering path.
 * ================================================================== */
void __cdecl zMath_ProjectPointBatchZBuf(const float *srcArray, float *dstArray, int count) {
    for (int i = 0; i < count; i++) {
        float invZ = 1.0f / srcArray[2];
        dstArray[0] = invZ * srcArray[0] * g_zMath_ProjScaleX + g_zMath_ProjOffsetX;
        dstArray[1] = srcArray[1] * invZ * g_zMath_ProjScaleY + g_zMath_ProjOffsetY;
        dstArray[2] = invZ * g_zMath_ZBufScale;
        srcArray += 3;
        dstArray += 3;
    }
}

/* ==================================================================
 * zMath_UnprojectPointBatch -- 0x474BC0
 * Reverse-projects screen coordinates back to view-space.
 * For each point:
 *   invZ  = 1.0 / src.z   (src.z is stored 1/Z from projection)
 *   dst.x = (src.x - OffsetX) * InvProjScaleX * invZ
 *   dst.y = (src.y - OffsetY) * InvProjScaleY * invZ
 *   dst.z = invZ
 * ================================================================== */
void __cdecl zMath_UnprojectPointBatch(const float *srcArray, float *dstArray, int count) {
    if (count <= 0)
        return;
    for (int i = 0; i < count; i++) {
        float invZ = 1.0f / srcArray[2];
        dstArray[2] = invZ;
        dstArray[0] = (srcArray[0] - g_zMath_ProjOffsetX) * g_zMath_InvProjScaleX * invZ;
        dstArray[1] = (srcArray[1] - g_zMath_ProjOffsetY) * g_zMath_InvProjScaleY * invZ;
        srcArray += 3;
        dstArray += 3;
    }
}

/* ==================================================================
 * SECTION 8: FOG
 * ================================================================== */

/* ==================================================================
 * zMath_FogTableLookup -- 0x474FC0
 * Builds a 256-entry exponential decay table if dirty, then looks up
 * the fog factor for a given distance.
 * Table: fogTable[i] = exp(-i * FOG_SCALE)
 * Returns fog alpha in [0, 1] range.
 * ================================================================== */
float __cdecl zMath_FogTableLookup(int unused, float distance) {
    /* Rebuild table if dirty */
    if (g_zMath_FogTableDirty) {
        g_zMath_FogRange = 51.0f;
        for (int i = 0; i < FOG_TABLE_SIZE; i++) {
            g_zMath_FogTable[i] = expf(-((float)i) * FOG_SCALE);
        }
        g_zMath_FogTableDirty = 0;
    }

    /* Clamp distance */
    if (distance >= 5.0f)
        return 0.0f;
    if (distance < 0.0f)
        return 1.0f;

    /* Look up in table */
    int idx = (int)(distance * g_zMath_FogRange);
    return g_zMath_FogTable[idx];
}

/* ==================================================================
 * SECTION 9: GEOMETRY / INTERSECTION
 * ================================================================== */

/* ==================================================================
 * zMath_TriangleNormal -- 0x475070
 * Computes the normalized face normal from three triangle vertices.
 * normal = normalize(cross(v1 - v0, v2 - v0))
 * ================================================================== */
void __cdecl zMath_TriangleNormal(const float *v0, const float *v1, const float *v2,
                                  float *outNormal) {
    float e1[3], e2[3];
    e1[0] = v1[0] - v0[0];
    e1[1] = v1[1] - v0[1];
    e1[2] = v1[2] - v0[2];
    e2[0] = v2[0] - v0[0];
    e2[1] = v2[1] - v0[1];
    e2[2] = v2[2] - v0[2];

    /* Cross product: e1 x e2 */
    outNormal[0] = e1[1] * e2[2] - e1[2] * e2[1];
    outNormal[1] = e1[2] * e2[0] - e1[0] * e2[2];
    outNormal[2] = e1[0] * e2[1] - e1[1] * e2[0];

    zMath_Vec3_Normalize(outNormal);
}

/* ==================================================================
 * zMath_SolveLinear2x2 -- 0x475130
 * Solves a 2x2 linear system using Cramer's rule.
 * Used for computing parametric coordinates (UV mapping, line-line
 * intersection). Returns 0 if degenerate (parallel lines).
 * ================================================================== */
void __cdecl zMath_SolveLinear2x2(float *outU, float *outV, float px, float py, float x0, float y0,
                                  float x1, float y1, float z0, float z1, float z2);

/* ==================================================================
 * zMath_RaySphereIntersect -- 0x475210
 * Tests a ray against a sphere.
 * Args: (rayOrigin, sphereCenter, radius, rayDir, outHitNormal)
 * Returns 1 on hit (outHitNormal set), 0 on miss.
 * Uses fast inverse sqrt bit hack: (*(int*)&x >> 1) + 0x1FC00000
 * ================================================================== */
int __cdecl zMath_RaySphereIntersect(const float *rayOrigin, const float *sphereCenter,
                                     float radius, const float *rayDir, float *outHitNormal) {
    float delta[3];
    delta[0] = rayOrigin[0] - sphereCenter[0];
    delta[1] = rayOrigin[1] - sphereCenter[1];
    delta[2] = rayOrigin[2] - sphereCenter[2];

    float distSq = delta[0] * delta[0] + delta[1] * delta[1] + delta[2] * delta[2];
    if (distSq == 0.0f)
        return 0;

    float b = rayDir[0] * delta[0] + rayDir[1] * delta[1] + rayDir[2] * delta[2];
    float c =
        (rayDir[0] * rayDir[0] + rayDir[1] * rayDir[1] + rayDir[2] * rayDir[2]) - radius * radius;

    if (c == 0.0f) {
        /* On the sphere surface */
        if (b == 0.0f)
            return 0;
        float t = (b + b) / distSq;
        /* Compute hit point and normal */
        float hit[3];
        hit[0] = t * delta[0];
        hit[1] = t * delta[1];
        hit[2] = t * delta[2];
        outHitNormal[0] = rayDir[0] - hit[0];
        outHitNormal[1] = rayDir[1] - hit[1];
        outHitNormal[2] = rayDir[2] - hit[2];
        zMath_Vec3_Normalize(outHitNormal);
        return 1;
    }

    float discriminant = b * b - distSq * c;
    if (discriminant < 0.0f)
        return 0;

    /* Fast inverse sqrt for the discriminant */
    float sqrtDisc;
    {
        int bits = *(int *)&discriminant;
        bits = (bits >> 1) + 0x1FC00000;
        sqrtDisc = *(float *)&bits;
    }

    /* Select the correct root based on sign of c */
    float t;
    if (c < 0.0f) {
        c = -c;
        if (-b + sqrtDisc <= 0.0f)
            return 0;
        t = c / (-b + sqrtDisc);
    } else {
        if (-b - sqrtDisc <= 0.0f) {
            if (-b + sqrtDisc <= 0.0f)
                return 0;
            t = c / (-b + sqrtDisc);
        } else {
            t = c / (-b - sqrtDisc);
        }
    }

    float hit[3];
    hit[0] = t * delta[0];
    hit[1] = t * delta[1];
    hit[2] = t * delta[2];
    outHitNormal[0] = rayDir[0] - hit[0];
    outHitNormal[1] = rayDir[1] - hit[1];
    outHitNormal[2] = rayDir[2] - hit[2];
    zMath_Vec3_Normalize(outHitNormal);
    return 1;
}

/* ==================================================================
 * zMath_TriangleScreenGradients -- 0x4753E0
 * Computes screen-space UV gradients for a triangle.
 * Used for perspective-correct texture mapping setup.
 * Takes: triangle vertices (9 floats), UV coords (6 floats),
 *        and outputs screen-space partial derivatives.
 * ================================================================== */
void __cdecl zMath_TriangleScreenGradients(const float *triVerts, const float *uvCoords,
                                           float *outScreenNormal, float *outDepth, float *outDuDxy,
                                           float *outDuDepth, float *outDvDxy, float *outDvDepth);

/* ==================================================================
 * SECTION 10: QUATERNION OPERATIONS
 * ================================================================== */

/* ==================================================================
 * zMath_Quat_FromEuler -- 0x4757C0
 * Converts Euler angles (pitch, yaw, roll) to a quaternion.
 * Uses half-angle formula with ZYX rotation order.
 * q = [w, x, y, z]
 * ================================================================== */
void __cdecl zMath_Quat_FromEuler(float *qOut, float pitch, float yaw, float roll) {
    float sp = sinf(pitch * 0.5f), cp = cosf(pitch * 0.5f);
    float sy = sinf(yaw * 0.5f), cy = cosf(yaw * 0.5f);
    float sr = sinf(roll * 0.5f), cr = cosf(roll * 0.5f);

    float cpcy = cy * cp;
    float spcy = sy * cp;
    float cpsy = cy * sp;
    float spsy = sy * sp;

    qOut[0] = spsy * sr + cpcy * cr; /* w */
    qOut[1] = cpsy * sr + spcy * cr; /* x */
    qOut[2] = cpsy * cr - spcy * sr; /* y */
    qOut[3] = cpcy * sr - spsy * cr; /* z */
}

/* ==================================================================
 * zMath_Quat_Multiply -- 0x475910
 * Hamilton product: out = q1 * q2
 * Both quaternions are [w, x, y, z].
 * ================================================================== */
float *__cdecl zMath_Quat_Multiply(const float *q1, const float *q2, float *out) {
    out[0] = q2[0] * q1[0] - q1[1] * q2[1] - q1[2] * q2[2] - q1[3] * q2[3];
    out[1] = q2[0] * q1[1] + q1[0] * q2[1] + q2[3] * q1[2] - q1[3] * q2[2];
    out[2] = q2[0] * q1[2] + q1[0] * q2[2] + q1[3] * q2[1] - q2[3] * q1[1];
    out[3] = q2[0] * q1[3] + q1[0] * q2[3] + q2[2] * q1[1] - q1[2] * q2[1];
    return out;
}

/* ==================================================================
 * zMath_Quat_MultiplyInverse -- 0x4759D0
 * Multiplies q1 by the conjugate of q2: out = q2* * q1
 * Effectively computes the relative rotation from q2 to q1.
 * ================================================================== */
float *__cdecl zMath_Quat_MultiplyInverse(const float *q1, const float *q2, float *out) {
    out[0] = q2[3] * q1[3] + q1[2] * q2[2] + q2[0] * q1[0] + q2[1] * q1[1];
    out[1] = q2[0] * q1[1] - q1[0] * q2[1] - q2[3] * q1[2] + q1[3] * q2[2];
    out[2] = q2[0] * q1[2] - q1[0] * q2[2] - q1[3] * q2[1] + q2[3] * q1[1];
    out[3] = q2[0] * q1[3] - q1[0] * q2[3] - q2[2] * q1[1] + q1[2] * q2[1];
    return out;
}

/* ==================================================================
 * zMath_Quat_ToMatrix -- 0x475A80
 * Converts a quaternion [w, x, y, z] to a 3x3 rotation matrix.
 * Output is written to a float[9] or the rotation part of a 4x3 matrix.
 * Standard formula:
 *   [1-2(yy+zz)  2(xy+wz)   2(xz-wy)]
 *   [2(xy-wz)    1-2(xx+zz) 2(wx+yz)]
 *   [2(wy+xz)    2(yz-wx)   1-2(xx+yy)]
 * ================================================================== */
void __cdecl zMath_Quat_ToMatrix(const float *q, float *mat) {
    float tx = 2.0f * q[1]; /* 2x */
    float ty = 2.0f * q[2]; /* 2y */
    float tz = 2.0f * q[3]; /* 2z */

    float txx = tx * q[1]; /* 2xx */
    float txy = ty * q[1]; /* 2xy */
    float txz = tz * q[2]; /* 2yz */
    float txw = tx * q[3]; /* 2xz */
    float tyw = tx * q[0]; /* 2wx */
    float tzw = ty * q[0]; /* 2wy */
    float twz = tz * q[0]; /* 2wz */

    mat[0] = 1.0f - ty * q[2] - tz * q[3]; /* 1 - 2(yy+zz) */
    mat[1] = twz + txy;                    /* 2(xy+wz) */
    mat[2] = txw - tzw;                    /* 2(xz-wy) */
    mat[3] = txy - twz;                    /* 2(xy-wz) */
    mat[4] = 1.0f - twz - txx;             /* 1 - 2(xx+zz) */
    mat[5] = tyw + txz;                    /* 2(wx+yz) */
    mat[6] = tzw + txw;                    /* 2(wy+xz) */
    mat[7] = txz - tyw;                    /* 2(yz-wx) */
    mat[8] = 1.0f - txx - twz;             /* 1 - 2(xx+yy) */
}

/* ==================================================================
 * zMath_Quat_FromRotationVector -- 0x475B80
 * Converts a rotation vector (axis * angle) to a quaternion.
 * The vector length is the rotation angle in radians.
 * If length is 0, returns identity quaternion (1, 0, 0, 0).
 * ================================================================== */
int __cdecl zMath_Quat_FromRotationVector(const float *rv, float *qOut) {
    float angle = sqrtf(rv[0] * rv[0] + rv[1] * rv[1] + rv[2] * rv[2]);
    if (angle == 0.0f) {
        qOut[0] = 1.0f; /* w = 1 */
        qOut[1] = 0.0f;
        qOut[2] = 0.0f;
        qOut[3] = 0.0f;
        return 0;
    }

    qOut[0] = cosf(angle); /* w = cos(angle) */
    float s = sinf(angle) / angle;
    qOut[1] = s * rv[0];
    qOut[2] = s * rv[1];
    qOut[3] = s * rv[2];
    return 1;
}

/* ==================================================================
 * END OF FILE
 *
 * Function summary (61 zMath functions total):
 *   Vec3 operations:  18 functions (0x402F60, 0x4726D0-0x472CC0, 0x4744F0-0x474F40)
 *   Matrix stack:      6 functions (0x472ED0-0x472FA0)
 *   Core matrix:      10 functions (0x472FB0-0x473CC0)
 *   Advanced matrix:  12 functions (0x473E60-0x474E10)
 *   Projection:        3 functions (0x474B20-0x474BC0)
 *   Fog:               1 function  (0x474FC0)
 *   Geometry:          4 functions (0x475070-0x4753E0)
 *   Quaternion:        5 functions (0x4757C0-0x475B80)
 *   Error handler:     1 function  (0x472D30)
 *   Math init:         1 function  (gModInit-adjacent, not counted here)
 * ================================================================== */
