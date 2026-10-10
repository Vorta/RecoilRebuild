#include "GameZRecoil/zVideo/zvid.h"

#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zVideo/zvid_state.h"

#include <malloc.h>

#define PackD3DColorFrom16(packedColor16, alpha)                                                                       \
    (((                                                                                                                \
          (((((packedColor16) & g_zVideo_PixelPack.rMask) >> g_zVideo_PixelPack.packedBase) | ((DWORD)(alpha) << 8))   \
              << 8)                                                                                                    \
          | (((packedColor16) & g_zVideo_PixelPack.gMask) >> g_zVideo_PixelPack.sumMinus8)                             \
      ) << 8)                                                                                                          \
        | (((packedColor16) & g_zVideo_PixelPack.bMask) << g_zVideo_PixelPack.bShiftTo8))

#define WriteFlatTlVertex(dst, src, packedColor)                                                                       \
    do {                                                                                                               \
        (dst).sx = (src).x;                                                                                            \
        (dst).sy = (src).y;                                                                                            \
        (dst).sz = (src).z;                                                                                            \
        (dst).rhw = (src).z;                                                                                           \
        (dst).color = (packedColor);                                                                                   \
        (dst).specular = 0xff000000;                                                                                   \
    } while (0)

#define CopyFlatVerticesReverse(dst, vertices, vertexCount, packedColor)                                               \
    {                                                                                                                  \
        int _copyIndex;                                                                                                \
        const zVideo_XyzVertex* _src = &(vertices)[(vertexCount) - 1];                                                 \
        for (_copyIndex = 0; _copyIndex < (vertexCount); ++_copyIndex, --_src) {                                       \
            WriteFlatTlVertex((dst)[_copyIndex], *_src, (packedColor));                                                \
        }                                                                                                              \
    }

#define CopyGouraudVerticesReverse(dst, sourceVertex, sourceColor, vertexCount, alpha)                                 \
    {                                                                                                                  \
        int _copyIndex;                                                                                                \
        for (_copyIndex = 0; _copyIndex < (vertexCount); ++_copyIndex, --(sourceVertex), --(sourceColor)) {            \
            WriteFlatTlVertex((dst)[_copyIndex], *(sourceVertex), PackD3DColorFrom16(*(sourceColor), (alpha)));        \
        }                                                                                                              \
    }

#define PackColorAttrConstant(baseColor, attr1Scale, alphaBits)                                                        \
    ((alphaBits)                                                                                                       \
        | ((((((DWORD)((int)((baseColor).r * (attr1Scale) + 0.5)) << 8)                                                \
                 | (DWORD)((int)((baseColor).g * (attr1Scale) + 0.5)))                                                 \
                << 8)                                                                                                  \
            | (DWORD)((int)((baseColor).b * (attr1Scale) + 0.5)))))

#define FillColorAttrSpecularReverse(attr2, lastIndex, vertexCount)                                                    \
    {                                                                                                                  \
        int _specIndex;                                                                                                \
        if ((attr2) != 0) {                                                                                            \
            const float* _specSource = &(attr2)[(lastIndex)];                                                          \
            for (_specIndex = 0; _specIndex < (vertexCount); ++_specIndex, --_specSource) {                            \
                g_zVideo_D3DSubmitTempVertices[_specIndex].specular                                                    \
                    = (DWORD)((int)(0.5 + (1.0f - *_specSource) * 255.0f)) << 24;                                      \
            }                                                                                                          \
        } else {                                                                                                       \
            for (_specIndex = 0; _specIndex < (vertexCount); ++_specIndex) {                                           \
                g_zVideo_D3DSubmitTempVertices[_specIndex].specular = 0xff000000;                                      \
            }                                                                                                          \
        }                                                                                                              \
    }

#define FillColorAttrColorsReverse(baseColor, attr0, lastIndex, attr1Scale, alphaBits, vertexCount)                    \
    {                                                                                                                  \
        int _colorIndex;                                                                                               \
        if ((attr0) != 0) {                                                                                            \
            const float* _attr0Source = &(attr0)[(lastIndex)];                                                         \
            for (_colorIndex = 0; _colorIndex < (vertexCount); ++_colorIndex, --_attr0Source) {                        \
                /* Each branch stores its color; retail shares only the OR/store tail. */                              \
                if (!(*_attr0Source > (1.0f / 255.0f))) {                                                              \
                    g_zVideo_D3DSubmitTempVertices[_colorIndex].color                                                  \
                        = PackColorAttrConstant((baseColor), (attr1Scale), (alphaBits));                               \
                } else {                                                                                               \
                    float _channels[3];                                                                                \
                    _channels[0] = (baseColor).r * (attr1Scale) + *_attr0Source * g_zVideo_D3DColorAttrBiasR;          \
                    _channels[1] = (baseColor).g * (attr1Scale) + *_attr0Source * g_zVideo_D3DColorAttrBiasG;          \
                    _channels[2] = (baseColor).b * (attr1Scale) + *_attr0Source * g_zVideo_D3DColorAttrBiasB;          \
                    if (_channels[g_zVideo_D3DColorNormalizeChannelIndex] > 255.0f) {                                  \
                        const float _scale = 1.0f / _channels[g_zVideo_D3DColorNormalizeChannelIndex] * 255.0f;        \
                        _channels[0] *= _scale;                                                                        \
                        _channels[1] *= _scale;                                                                        \
                        _channels[2] *= _scale;                                                                        \
                    }                                                                                                  \
                    g_zVideo_D3DSubmitTempVertices[_colorIndex].color                                                  \
                        = (((((DWORD)((int)(_channels[0])) << 8) | (DWORD)((int)(_channels[1]))) << 8) | (alphaBits))  \
                        | (DWORD)((int)(_channels[2]));                                                                \
                }                                                                                                      \
            }                                                                                                          \
        } else {                                                                                                       \
            const DWORD _constantColor = PackColorAttrConstant((baseColor), (attr1Scale), (alphaBits));                \
            for (_colorIndex = 0; _colorIndex < (vertexCount); ++_colorIndex) {                                        \
                g_zVideo_D3DSubmitTempVertices[_colorIndex].color = _constantColor;                                    \
            }                                                                                                          \
        }                                                                                                              \
    }

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyPositionsReverse helper behavior for zVideo callers.
 */
#define CopyPositionsReverse(dst, vertices, lastIndex, vertexCount)                                                    \
    {                                                                                                                  \
        int _copyIndex;                                                                                                \
        const zVideo_XyzVertex* _src = &(vertices)[(lastIndex)];                                                       \
        for (_copyIndex = 0; _copyIndex < (vertexCount); ++_copyIndex, --_src) {                                       \
            (dst)[_copyIndex].sx = _src->x;                                                                            \
            (dst)[_copyIndex].sy = _src->y;                                                                            \
            (dst)[_copyIndex].sz = _src->z;                                                                            \
            (dst)[_copyIndex].rhw = _src->z;                                                                           \
        }                                                                                                              \
    }

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered PackAlphaWhite helper behavior for zVideo callers.
 */
#define PackAlphaWhite(alpha) (((DWORD)((int)((alpha) * 255.0f)) << 24) | 0x00ffffff)

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered WriteTexturedTlVertex helper behavior for zVideo callers.
 */
#define WriteTexturedTlVertex(dst, src, texCoord, packed)                                                              \
    do {                                                                                                               \
        (dst).sx = (src).x;                                                                                            \
        (dst).sy = (src).y;                                                                                            \
        (dst).sz = (src).z;                                                                                            \
        (dst).rhw = (src).z;                                                                                           \
        (dst).color = (packed);                                                                                        \
        (dst).specular = 0xff000000;                                                                                   \
        (dst).tu = (texCoord).u;                                                                                       \
        (dst).tv = (texCoord).v;                                                                                       \
    } while (0)

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyTexturedVerticesReverse helper behavior for zVideo callers.
 */
#define CopyTexturedVerticesReverse(dst, vertices, texCoords, vertexCount, packed)                                     \
    {                                                                                                                  \
        int _copyIndex;                                                                                                \
        const zVideo_XyzVertex* _src = &(vertices)[(vertexCount) - 1];                                                 \
        const zVideo_TexCoord* _uv = &(texCoords)[(vertexCount) - 1];                                                  \
        for (_copyIndex = 0; _copyIndex < (vertexCount); ++_copyIndex, --_src, --_uv) {                                \
            WriteTexturedTlVertex((dst)[_copyIndex], *_src, *_uv, (packed));                                           \
        }                                                                                                              \
    }

#define CopyTexturedVerticesReverseQueued(dst, vertices, texCoords, vertexCount, packed)                               \
    {                                                                                                                  \
        int _copyIndex;                                                                                                \
        const zVideo_XyzVertex* _src = &(vertices)[(vertexCount) - 1];                                                 \
        const zVideo_TexCoord* _uv = &(texCoords)[(vertexCount) - 1];                                                  \
        for (_copyIndex = 0; _copyIndex < (vertexCount); ++_copyIndex, --_src, --_uv) {                                \
            (dst)[_copyIndex].sx = _src->x;                                                                            \
            (dst)[_copyIndex].sy = _src->y;                                                                            \
            (dst)[_copyIndex].sz = _src->z;                                                                            \
            (dst)[_copyIndex].rhw = _src->z;                                                                           \
            (dst)[_copyIndex].color = (packed);                                                                        \
            (dst)[_copyIndex].tu = _uv->u;                                                                             \
            (dst)[_copyIndex].tv = _uv->v;                                                                             \
            (dst)[_copyIndex].specular = 0xff000000;                                                                   \
        }                                                                                                              \
    }

/**
 * Original-source static helper evidence: source-faithful helper for polygon submitters.
 * Purpose: Pack a gray polygon color with optional high clamp for address-backed
 * callers 0x4abb20 and 0x4ac370; BN has no standalone retail function.
 */
#define FillPolygonColorsReverse(attr0, lastIndex, grayBase, alphaBits, vertexCount)                                   \
    {                                                                                                                  \
        int _polyIndex;                                                                                                \
        if ((attr0) != 0) {                                                                                            \
            const float _grayBase = (grayBase);                                                                        \
            const float* _attr0Source = &(attr0)[(lastIndex)];                                                         \
            for (_polyIndex = 0; _polyIndex < (vertexCount); ++_polyIndex, --_attr0Source) {                           \
                if (!(*_attr0Source > (1.0f / 255.0f))) {                                                              \
                    DWORD _grayByte = (DWORD)((int)(_grayBase));                                                       \
                    if (_grayByte > 0xff) {                                                                            \
                        _grayByte = 0xff;                                                                              \
                    }                                                                                                  \
                    g_zVideo_D3DSubmitTempVertices[_polyIndex].color                                                   \
                        = ((((_grayByte << 8) | _grayByte) << 8) | _grayByte) | (alphaBits);                           \
                } else {                                                                                               \
                    float _channels[3];                                                                                \
                    _channels[0] = *_attr0Source * g_zVideo_D3DColorAttrBiasR + _grayBase;                             \
                    _channels[1] = *_attr0Source * g_zVideo_D3DColorAttrBiasG + _grayBase;                             \
                    _channels[2] = *_attr0Source * g_zVideo_D3DColorAttrBiasB + _grayBase;                             \
                    if (_channels[g_zVideo_D3DColorNormalizeChannelIndex] > 255.0f) {                                  \
                        const float _scale = 1.0f / _channels[g_zVideo_D3DColorNormalizeChannelIndex] * 255.0f;        \
                        _channels[0] *= _scale;                                                                        \
                        _channels[1] *= _scale;                                                                        \
                        _channels[2] *= _scale;                                                                        \
                    }                                                                                                  \
                    g_zVideo_D3DSubmitTempVertices[_polyIndex].color                                                   \
                        = (((((DWORD)((int)(_channels[0])) << 8) | (DWORD)((int)(_channels[1]))) << 8) | (alphaBits))  \
                        | (DWORD)((int)(_channels[2]));                                                                \
                }                                                                                                      \
            }                                                                                                          \
        } else {                                                                                                       \
            /* Retail packs the constant gray byte in place. */                                                        \
            DWORD _packed = (DWORD)((int)(grayBase));                                                                  \
            _packed = _packed | ((((_packed << 8) | _packed) << 8) | (alphaBits));                                     \
            for (_polyIndex = 0; _polyIndex < (vertexCount); ++_polyIndex) {                                           \
                g_zVideo_D3DSubmitTempVertices[_polyIndex].color = _packed;                                            \
            }                                                                                                          \
        }                                                                                                              \
    }

#define FillPolygonLitColorsReverse(attr1, attr0, lastIndex, alphaBits, vertexCount)                                   \
    {                                                                                                                  \
        int _polyIndex;                                                                                                \
        if ((attr0) != 0) {                                                                                            \
            (attr1) += (lastIndex);                                                                                    \
            (attr0) += (lastIndex);                                                                                    \
            for (_polyIndex = 0; _polyIndex < (vertexCount); ++_polyIndex, --(attr1), --(attr0)) {                     \
                const float _grayBase = (1.0f - *(attr1)) * 255.0f;                                                    \
                if (!(*(attr0) > (1.0f / 255.0f))) {                                                                   \
                    DWORD _grayByte = (DWORD)((int)(_grayBase));                                                       \
                    if (_grayByte > 0xff) {                                                                            \
                        _grayByte = 0xff;                                                                              \
                    }                                                                                                  \
                    g_zVideo_D3DSubmitTempVertices[_polyIndex].color                                                   \
                        = ((((_grayByte << 8) | _grayByte) << 8) | _grayByte) | (alphaBits);                           \
                } else {                                                                                               \
                    float _channels[3];                                                                                \
                    _channels[0] = *(attr0) * g_zVideo_D3DColorAttrBiasR + _grayBase;                                  \
                    _channels[1] = *(attr0) * g_zVideo_D3DColorAttrBiasG + _grayBase;                                  \
                    _channels[2] = *(attr0) * g_zVideo_D3DColorAttrBiasB + _grayBase;                                  \
                    if (_channels[g_zVideo_D3DColorNormalizeChannelIndex] > 255.0f) {                                  \
                        const float _scale = 1.0f / _channels[g_zVideo_D3DColorNormalizeChannelIndex] * 255.0f;        \
                        _channels[0] *= _scale;                                                                        \
                        _channels[1] *= _scale;                                                                        \
                        _channels[2] *= _scale;                                                                        \
                    }                                                                                                  \
                    g_zVideo_D3DSubmitTempVertices[_polyIndex].color                                                   \
                        = ((((((DWORD)((int)(_channels[0])) << 8) | (DWORD)((int)(_channels[1]))) << 8)                \
                               | (DWORD)((int)(_channels[2])))                                                         \
                            | (alphaBits));                                                                            \
                }                                                                                                      \
            }                                                                                                          \
        } else {                                                                                                       \
            (attr1) += (lastIndex);                                                                                    \
            for (_polyIndex = 0; _polyIndex < (vertexCount); ++_polyIndex, --(attr1)) {                                \
                const DWORD _grayByte = (DWORD)((int)((1.0f - *(attr1)) * 255.0f));                                    \
                g_zVideo_D3DSubmitTempVertices[_polyIndex].color                                                       \
                    = ((((_grayByte << 8) | _grayByte) << 8) | _grayByte) | (alphaBits);                               \
            }                                                                                                          \
        }                                                                                                              \
    }

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyPositionUvReversePreserveColor helper behavior for zVideo callers.
 */
#define CopyPositionUvReversePreserveColor(dst, vertices, uvPairs, lastIndex, vertexCount, copyIndex)                  \
    {                                                                                                                  \
        const zVideo_XyzVertex* _src;                                                                                  \
        const zVideo_TexCoord* _uv;                                                                                    \
        (copyIndex) = 0;                                                                                               \
        _src = &(vertices)[(lastIndex)];                                                                               \
        _uv = &(uvPairs)[(lastIndex)];                                                                                 \
        for (; (copyIndex) < (vertexCount); ++(copyIndex), --_src, --_uv) {                                            \
            (dst)[(copyIndex)].sx = _src->x;                                                                           \
            (dst)[(copyIndex)].sy = _src->y;                                                                           \
            (dst)[(copyIndex)].sz = _src->z;                                                                           \
            (dst)[(copyIndex)].rhw = _src->z;                                                                          \
            (dst)[(copyIndex)].tu = _uv->u;                                                                            \
            (dst)[(copyIndex)].tv = _uv->v;                                                                            \
        }                                                                                                              \
    }

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyPositionUvWithPreparedColorReverse helper behavior for zVideo callers.
 */
#define CopyPositionUvWithPreparedColorReverse(dst, vertices, uvPairs, prepared, lastIndex, vertexCount, copyIndex)    \
    {                                                                                                                  \
        const zVideo_XyzVertex* _src;                                                                                  \
        const zVideo_TexCoord* _uv;                                                                                    \
        (copyIndex) = 0;                                                                                               \
        _src = &(vertices)[(lastIndex)];                                                                               \
        _uv = &(uvPairs)[(lastIndex)];                                                                                 \
        for (; (copyIndex) < (vertexCount); ++(copyIndex), --_src, --_uv) {                                            \
            (dst)[(copyIndex)].sx = _src->x;                                                                           \
            (dst)[(copyIndex)].sy = _src->y;                                                                           \
            (dst)[(copyIndex)].sz = _src->z;                                                                           \
            (dst)[(copyIndex)].rhw = _src->z;                                                                          \
            (dst)[(copyIndex)].color = (prepared)[(copyIndex)].color;                                                  \
            (dst)[(copyIndex)].tu = _uv->u;                                                                            \
            (dst)[(copyIndex)].tv = _uv->v;                                                                            \
            (dst)[(copyIndex)].specular = (prepared)[(copyIndex)].specular;                                            \
        }                                                                                                              \
    }

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered AppendFanCloseVertexIfNeeded helper behavior for zVideo callers.
 */
#define AppendFanCloseVertexIfNeeded(vertices, index, count)                                                           \
    do {                                                                                                               \
        if (g_zVideo_D3DAppendFanCloseVertexPending != 0) {                                                            \
            g_zVideo_D3DAppendFanCloseVertexPending = 0;                                                               \
            ++(count);                                                                                                 \
            (vertices)[(index)].sx = (vertices)[1].sx;                                                                 \
            (vertices)[(index)].sy = (vertices)[1].sy;                                                                 \
            (vertices)[(index)].sz = (vertices)[1].sz;                                                                 \
            (vertices)[(index)].rhw = (vertices)[1].rhw;                                                               \
            (vertices)[(index)].tu = (vertices)[1].tu;                                                                 \
            (vertices)[(index)].tv = (vertices)[1].tv;                                                                 \
            (vertices)[(index)].color = (vertices)[1].color;                                                           \
            (vertices)[(index)].specular = (vertices)[1].specular;                                                     \
        }                                                                                                              \
    } while (0)

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-begin-scene-and-flush-pending-render-states
 * @recoil-artifact defines .text recoil:function:0x4a9ac0: zVideo_dd3d::BeginSceneAndFlushPendingRenderStates.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: begin the Direct3D scene and flush deferred wireframe and dither states.
 *
 * Evidence: BN calls IDirect3DDevice2::BeginScene, reports zvid_ddd3d.c line 76
 * on failure, maps pending wireframe 0/1 to solid/wireframe fill mode, resets
 * applied pending states to -1, and returns zero on success.
 */
int __cdecl BeginSceneAndFlushPendingRenderStates(void)
{
    const HRESULT hresult = g_zVideo_pD3DDevice->lpVtbl->BeginScene(g_zVideo_pD3DDevice);
    int pendingWireframeState;
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 76);
    }

    pendingWireframeState = g_zVideo_PendingWireframeState;
    if (pendingWireframeState == 0) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_FILLMODE, D3DFILL_SOLID);
        g_zVideo_PendingWireframeState = -1;
    } else if (pendingWireframeState == 1) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_FILLMODE, D3DFILL_WIREFRAME);
        g_zVideo_PendingWireframeState = -1;
    }

    // VC5 matches BN when the dither global is reloaded at the call site.
    if (g_zVideo_PendingDitherEnable != -1) {
        g_zVideo_pD3DDevice->lpVtbl
            ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_DITHERENABLE, (DWORD)(g_zVideo_PendingDitherEnable));
        g_zVideo_PendingDitherEnable = -1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-end-scene
 * @recoil-artifact defines .text recoil:function:0x4a9b40: zVideo_dd3d::EndScene.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: end the active Direct3D scene and report provider failures.
 *
 * Evidence: BN calls IDirect3DDevice2::EndScene, reports zvid_ddd3d.c line 115
 * on nonzero HRESULT, and returns zero on success.
 */
int __cdecl EndScene(void)
{
    const HRESULT hresult = g_zVideo_pD3DDevice->lpVtbl->EndScene(g_zVideo_pD3DDevice);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 115);
    }

    return 0;
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: flips the Direct3D display-mode surface, optionally blits the
 * primary surface back to software first, and retries lost or busy surfaces.
 *
 * Evidence: BN uses ECX/EDX for source/destination rects, stack arguments for
 * wait/blit flags, checks g_zVideo_DisplayModeSurfaceState.surf for the 0x400
 * no-surface return, issues DirectDraw Blt and Flip provider calls with
 * DDBLT_WAIT/DDFLIP_WAIT flag construction, retries DDERR_WASSTILLDRAWING,
 * restores and retries DDERR_SURFACELOST, and reports zvid_ddd3d.c line 0xae
 * before returning 0x5a56ffff on unrecoverable provider failure.
 */
int __fastcall zVideodd3dPresentDisplayModeSurface(
    zVidRect32* srcRect,
    zVidRect32* dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
)
{
    // BN keeps the checked display surface live, then reloads it after Blt/retry paths.
    IDirectDrawSurface3* displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
    if (displaySurface == 0) {
        return 0x400;
    }

    for (;;) {
        HRESULT hresult;
        if (blitPrimaryToSwFirst != 0) {
            const DWORD bltFlags = waitForPresent != 0 ? DDBLT_WAIT : 0;
            g_zVideo_SwSurfaceState.surf->lpVtbl->Blt(
                g_zVideo_SwSurfaceState.surf,
                (RECT*)(dstRect),
                g_zVideo_PrimarySurfaceState.surf,
                (RECT*)(srcRect),
                bltFlags,
                0
            );
            displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
        }

        hresult = displaySurface->lpVtbl->Flip(displaySurface, 0, waitForPresent != 0 ? DDFLIP_WAIT : 0);
        if (hresult != DD_OK) {
            if (hresult == DDERR_WASSTILLDRAWING) {
                displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
                continue;
            }

            if (hresult == DDERR_SURFACELOST) {
                displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
                hresult = displaySurface->lpVtbl->Restore(displaySurface);
            }

            if (hresult == DD_OK) {
                displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
                continue;
            }

            ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xae);
            return 0x5a56ffff;
        }

        // Retail places the DD_OK return after the failure handling.
        return 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-create-device-state
 * @recoil-artifact defines .text recoil:function:0x4a9c20: zVideo_dd3d::CreateDeviceState.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: creates the Direct3D z-buffer/device/viewport/material state and
 * initializes the fixed render-state defaults for the active software surface.
 *
 * Evidence: BN shows z-buffer surface creation, DirectDraw/Direct3D provider
 * setup, material and caps initialization, ten
 * fixed render-state writes, fog enablement, and quad-batch depth seeding.
 */
int __fastcall CreateDeviceState(void)
{
    DDSURFACEDESC zBufferDesc = { 0 };
    D3DVIEWPORT2 viewport2 = { 0 };
    D3DMATERIAL mat = { 0 };
    HRESULT hresult;
    DWORD width;
    DWORD height;
    // VC5/BN evidence shows the original C source zeroed this provider record again here.
    memset(&zBufferDesc, 0, sizeof(zBufferDesc));
    zBufferDesc.dwWidth = (DWORD)(g_zVideo_SwSurfaceState.width);
    zBufferDesc.dwHeight = (DWORD)(g_zVideo_SwSurfaceState.height);
    g_zVideo_ClearScreenBufferEnabled = 1;
    zBufferDesc.dwSize = sizeof(zBufferDesc);
    zBufferDesc.dwFlags = 0x47;
    zBufferDesc.ddsCaps.dwCaps = 0x24000;
    zBufferDesc.dwMipMapCount = 0x10;

    hresult = g_zVideo_pDirectDraw2->lpVtbl->CreateSurface(
        g_zVideo_pDirectDraw2,
        &zBufferDesc,
        (IDirectDrawSurface**)(&g_zVideo_pZBufferSurface),
        0
    );
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xd3);
    }

    hresult = g_zVideo_pZBufferSurface->lpVtbl->QueryInterface(
        g_zVideo_pZBufferSurface,
        &IID_IDirectDrawSurface,
        (void**)(&g_zVideo_pZBufferAttachSurface)
    );
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xd9);
    }

    hresult = g_zVideo_SwSurfaceState.surf->lpVtbl->AddAttachedSurface(
        g_zVideo_SwSurfaceState.surf,
        (IDirectDrawSurface3*)(g_zVideo_pZBufferAttachSurface)
    );
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xde);
    }

    hresult = g_zVideo_pDirectDraw2->lpVtbl
                  ->QueryInterface(g_zVideo_pDirectDraw2, &IID_IDirect3D2, (void**)(&g_zVideo_pD3D2));
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xe5);
    }

    hresult = g_zVideo_pD3D2->lpVtbl->CreateDevice(
        g_zVideo_pD3D2,
        g_zVideo_pSelectedD3DDeviceInfo->pD3DDeviceGuid,
        (IDirectDrawSurface*)(g_zVideo_SwSurfaceState.surf),
        &g_zVideo_pD3DDevice
    );
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xed);
    }

    hresult = g_zVideo_pD3D2->lpVtbl->CreateViewport(g_zVideo_pD3D2, &g_zVideo_pD3DViewport2, 0);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xf4);
    }

    hresult = g_zVideo_pD3DDevice->lpVtbl->AddViewport(g_zVideo_pD3DDevice, g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xf9);
    }

    width = (DWORD)(g_zVideo_DisplayModeSurfaceState.width);
    height = (DWORD)(g_zVideo_DisplayModeSurfaceState.height);
    viewport2.dwSize = sizeof(viewport2);
    viewport2.dwX = 0;
    viewport2.dwY = 0;
    viewport2.dwWidth = width;
    viewport2.dwHeight = height;
    viewport2.dvClipX = 0.0f;
    viewport2.dvClipY = 0.0f;
    viewport2.dvClipWidth = (D3DVALUE)(width);
    viewport2.dvClipHeight = (D3DVALUE)(height);
    viewport2.dvMinZ = 0.0f;
    viewport2.dvMaxZ = 1.0f;

    hresult = g_zVideo_pD3DViewport2->lpVtbl->SetViewport2(g_zVideo_pD3DViewport2, &viewport2);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x10a);
    }

    hresult = g_zVideo_pD3DDevice->lpVtbl->SetCurrentViewport(g_zVideo_pD3DDevice, g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x10f);
    }

    hresult = g_zVideo_pD3D2->lpVtbl->CreateMaterial(g_zVideo_pD3D2, &g_zVideo_pD3DMaterial2, 0);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x116);
    }

    mat.dwSize = sizeof(mat);
    mat.diffuse.b = 0.0f;
    mat.diffuse.g = 0.0f;
    mat.diffuse.r = 0.0f;
    mat.ambient.b = 1.0f;
    mat.ambient.g = 1.0f;
    mat.ambient.r = 1.0f;
    mat.dwRampSize = 0x100;

    hresult = g_zVideo_pD3DMaterial2->lpVtbl->SetMaterial(g_zVideo_pD3DMaterial2, &mat);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x124);
    }

    hresult = g_zVideo_pD3DMaterial2->lpVtbl
                  ->GetHandle(g_zVideo_pD3DMaterial2, g_zVideo_pD3DDevice, &g_zVideo_D3DMaterialHandle);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x12a);
    }

    hresult = g_zVideo_pD3DViewport2->lpVtbl->SetBackground(g_zVideo_pD3DViewport2, g_zVideo_D3DMaterialHandle);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x12f);
    }

    g_zVideo_D3DHelDeviceDesc.dwSize = sizeof(g_zVideo_D3DHelDeviceDesc);
    g_zVideo_D3DHalDeviceDesc.dwSize = sizeof(g_zVideo_D3DHalDeviceDesc);
    hresult = g_zVideo_pD3DDevice->lpVtbl
                  ->GetCaps(g_zVideo_pD3DDevice, &g_zVideo_D3DHalDeviceDesc, &g_zVideo_D3DHelDeviceDesc);
    if (hresult != DD_OK) {
        return ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x139);
    }

    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_CULLMODE, 1);
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZENABLE, 1);
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZFUNC, 7);
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SPECULARENABLE, 0);
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 1);
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREPERSPECTIVE, 1);
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMAG, 2);
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMIN, 2);
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SRCBLEND, 5);
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_DESTBLEND, 6);

    g_zVideo_PendingWireframeState = -1;
    SetFogEnable(1);
    SetQuadBatchDepthAndRhw(0.99000001f);
    return 0;
}

/**
 * Retail literal-backed physical source block: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: validates a zVid image for Direct3D texture limits, creates upload
 * and hardware texture surfaces, loads the texture, and returns the default
 * texture record on validation or provider failure.
 *
 * Evidence: BN assembly checks device texture dimensions, power-of-two and
 * aspect-ratio caps, optionally resamples square-only textures, rejects
 * initially paletted images, builds upload/video DDSURFACEDESC records, calls
 * TexturePixelPackSetupFromMasks and UploadImageToSurface, performs
 * DirectDraw/Direct3D provider QueryInterface/Load/GetHandle calls, fills the
 * zVideo_TextureRecordPartial fields, and releases temporary provider objects
 * on failure.
 */
zVideo_TextureRecordPartial* __fastcall
/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-create-texture-record
 * @recoil-artifact defines .text recoil:function:0x4aa0f0: zVideo_dd3d::CreateTextureRecord.
 * @recoil-match byte
 */
CreateTextureRecord(register const char* textureName, zVidImagePartial* image, int useAlpha, int clampU, int clampV)
{
    DDSURFACEDESC desc = { 0 };
    IDirectDrawSurface* uploadSurface = 0;
    IDirectDrawSurface* textureSurface = 0;
    IDirect3DTexture2* uploadTexture = 0;
    IDirect3DTexture2* texture = 0;
    IDirectDrawPalette* ddPalette = 0;
    zVideo_TextureRecordPartial* result = 0;
    HRESULT hresult;
    D3DTEXTUREHANDLE textureHandle; // Retail leaves the handle uninitialized until GetHandle.

    if ((DWORD)(image->width) > g_zVideo_pSelectedD3DDeviceInfo->m_hwDesc.dwMaxTextureWidth
        || (DWORD)(image->height) > g_zVideo_pSelectedD3DDeviceInfo->m_hwDesc.dwMaxTextureHeight) {
        ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x20e,
            g_zVideo_TextureTooLargeUsingDefaultFmt,
            textureName,
            image->width,
            image->height
        );
        return g_zVideo_DefaultTextureRecord;
    }

    if ((g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) != 0) {
        int isPow2 = 1;
        if (FloorPowerOfTwo(image->width) != image->width || FloorPowerOfTwo(image->height) != image->height) {
            isPow2 = 0;
        }
        // Retail materializes the power-of-two test as a flag before branching.
        if (isPow2 == 0) {
            ReportOld(
                0x200,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x224,
                g_zVideo_TextureNotPowerOf2UsingDefaultFmt,
                textureName,
                image->width,
                image->height
            );
            return g_zVideo_DefaultTextureRecord;
        }
    }

    if (image->width > image->height * 8 || image->height > image->width * 8) {
        ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x233,
            g_zVideo_TextureBadAspectUsingDefaultFmt,
            textureName,
            image->width,
            image->height
        );
        return g_zVideo_DefaultTextureRecord;
    }

    if ((g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0
        && image->width != image->height) {
        const int squareSide = FloorPowerOfTwo((int)(sqrt((double)(image->height * image->width))));
        ResampleSquare(image, squareSide);
    }

    if (image->palette != 0) {
        ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x24a,
            g_zVideo_TexturePaletteUnsupportedUsingDefaultFmt,
            textureName
        );
        return g_zVideo_DefaultTextureRecord;
    }

    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    desc.dwHeight = (DWORD)(image->height);
    desc.dwWidth = (DWORD)(image->width);
    desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    desc.ddpfPixelFormat.dwRGBBitCount = 16;

    if (useAlpha == 0) {
        desc.ddpfPixelFormat.dwRBitMask = g_zVideo_PixelPack.rMask;
        desc.ddpfPixelFormat.dwGBitMask = g_zVideo_PixelPack.gMask;
        desc.ddpfPixelFormat.dwBBitMask = g_zVideo_PixelPack.bMask;
        TexturePixelPackSetupFromMasks(
            g_zVideo_PixelPack.rBits,
            g_zVideo_PixelPack.gBits,
            g_zVideo_PixelPack.bBits,
            useAlpha,
            g_zVideo_PixelPack.rMask,
            g_zVideo_PixelPack.gMask,
            g_zVideo_PixelPack.bMask,
            useAlpha
        );
    } else {
        desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        if (image->alphaMap == 0) {
            desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0x8000;
            desc.ddpfPixelFormat.dwRBitMask = 0x7c00;
            desc.ddpfPixelFormat.dwGBitMask = 0x03e0;
            desc.ddpfPixelFormat.dwBBitMask = 0x001f;
            // Retail passes green mask 0x03c0 here, unlike the 0x03e0 surface format mask.
            TexturePixelPackSetupFromMasks(5, 5, 5, 1, 0x7c00, 0x03c0, 0x001f, 0x8000);
        } else {
            desc.ddpfPixelFormat.dwRBitMask = 0x0f00;
            desc.ddpfPixelFormat.dwGBitMask = 0x00f0;
            desc.ddpfPixelFormat.dwBBitMask = 0x000f;
            desc.ddpfPixelFormat.dwRGBAlphaBitMask = 0xf000;
            TexturePixelPackSetupFromMasks(4, 4, 4, 4, 0x0f00, 0x00f0, 0x000f, 0xf000);
        }
    }

    hresult = g_zVideo_pDirectDraw2->lpVtbl->CreateSurface(g_zVideo_pDirectDraw2, &desc, &uploadSurface, 0);
    if (hresult == DD_OK && image->palette != 0) {
        PALETTEENTRY paletteEntries[256];
        memset(paletteEntries, 0, sizeof(paletteEntries));
        memcpy(paletteEntries, image->palette, image->paletteMetaPacked);
        hresult = g_zVideo_pDirectDraw2->lpVtbl->CreatePalette(
            g_zVideo_pDirectDraw2,
            DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            (LPPALETTEENTRY)(image->palette),
            &ddPalette,
            0
        );
        if (hresult == DD_OK) {
            hresult = uploadSurface->lpVtbl->SetPalette(uploadSurface, ddPalette);
        }
    }
    if (hresult == DD_OK) {
        UploadImageToSurface(uploadSurface, image, useAlpha);
    }
    if (hresult == DD_OK) {
        hresult
            = uploadSurface->lpVtbl->QueryInterface(uploadSurface, &IID_IDirect3DTexture2, (void**)(&uploadTexture));
    }
    if (hresult == DD_OK) {
        desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
        if ((g_zVideo_D3DHalDeviceDesc.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) != 0) {
            desc.ddsCaps.dwCaps |= DDSCAPS_NONLOCALVIDMEM;
        }

        hresult = g_zVideo_pDirectDraw2->lpVtbl->CreateSurface(g_zVideo_pDirectDraw2, &desc, &textureSurface, 0);
    }
    if (hresult == DD_OK && ddPalette != 0) {
        hresult = textureSurface->lpVtbl->SetPalette(textureSurface, ddPalette);
    }
    if (hresult == DD_OK) {
        hresult = textureSurface->lpVtbl->QueryInterface(textureSurface, &IID_IDirect3DTexture2, (void**)(&texture));
    }
    if (hresult == DD_OK) {
        hresult = texture->lpVtbl->Load(texture, uploadTexture);
    }
    if (hresult == DD_OK) {
        hresult = texture->lpVtbl->GetHandle(texture, g_zVideo_pD3DDevice, &textureHandle);
    }
    if (hresult == DD_OK) {
        result = TextureRecordCreate();
        if (result != 0) {
            result->m_uploadSurface = uploadSurface;
            result->m_textureSurface = textureSurface;
            result->m_texture = texture;
            result->m_textureHandle = textureHandle;
            result->m_alphaMode = useAlpha == 0 ? 1 : (image->alphaMap != 0 ? 4 : 5);
            result->m_uWrapMode = clampU != 0 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
            result->m_vWrapMode = clampV != 0 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
        }
        uploadTexture->lpVtbl->Release(uploadTexture);
    }

    if (hresult != DD_OK) {
        ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x30f);
        if (texture != 0) {
            texture->lpVtbl->Release(texture);
        }
        if (uploadTexture != 0) {
            uploadTexture->lpVtbl->Release(uploadTexture);
        }
        if (textureSurface != 0) {
            textureSurface->lpVtbl->Release(textureSurface);
        }
        if (uploadSurface != 0) {
            uploadSurface->lpVtbl->Release(uploadSurface);
        }
    }

    if (result != 0) {
        return result;
    }
    return g_zVideo_DefaultTextureRecord;
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: locks a DirectDraw upload surface, copies or converts image pixels
 * into its pitch layout, and unlocks the surface after upload.
 *
 * Evidence: BN assembly locks through zVideo_dd::LockSurfaceWaitRestore,
 * chooses ConvertImagePixelsForTexture only when useAlpha is nonzero, otherwise
 * copies either one contiguous block or one row per pitch, then unlocks through
 * zVideo_dd::UnlockSurfaceWaitRestore and returns one unconditionally.
 */
int __fastcall UploadImageToSurface(IDirectDrawSurface* uploadSurface, zVidImagePartial* image, int useAlpha)
{
    DDSURFACEDESC lockedDescOut;
    unsigned char* srcPixels = (unsigned char*)(image->pixels);
    unsigned char* dstPixels;
    LockSurfaceWaitRestore((IDirectDrawSurface3*)(uploadSurface), &lockedDescOut);

    dstPixels = (unsigned char*)(lockedDescOut.lpSurface);
    if (useAlpha == 0) {
        if (lockedDescOut.lPitch == image->width) {
            memcpy(dstPixels, srcPixels, (size_t)(image->height * (g_zVideo_DisplayModeBpp / 8) * image->width));
        } else {
            const int rowCopyBytes = g_zVideo_DisplayModeBpp * image->width / 8;
            int row;
            for (row = 0; row < image->height; ++row) {
                memcpy(dstPixels, srcPixels, (size_t)(rowCopyBytes));
                dstPixels += lockedDescOut.lPitch;
                srcPixels += image->width << 1;
            }
        }
    } else {
        ConvertImagePixelsForTexture((unsigned short*)(dstPixels), image, lockedDescOut.lPitch, useAlpha);
    }

    UnlockSurfaceWaitRestore((IDirectDrawSurface3*)(uploadSurface));
    return 1;
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: converts zVid 16-bit image pixels into the active Direct3D texture
 * upload pixel format, including alpha-map expansion when present.
 *
 * Evidence: BN assembly has no callees, walks image rows by destination pitch,
 * ignores the useAlpha argument, uses g_zVideo_PixelPack masks for opaque
 * pixels, and selects the 565 versus 555 alpha-map channel shifts from
 * g_zVideo_PixelPack.gBits.
 */
void __fastcall
ConvertImagePixelsForTexture(unsigned short* dstPixels, zVidImagePartial* image, int pitchBytes, int useAlpha)
{
    // Retail keeps one row/column pair for both conversion loops (shared stack slots).
    int row;
    int column;
    char* alphaMap;
    unsigned short* srcPixels;
    unsigned char* dstRowBytes;
    unsigned char* alphaCursor;
    unsigned int redAlphaMask;
    int redAlphaShift;
    unsigned int greenAlphaMask;
    int greenAlphaShift;

    (void)useAlpha;

    alphaMap = image->alphaMap;
    srcPixels = (unsigned short*)(image->pixels);
    dstRowBytes = (unsigned char*)(dstPixels);

    if (alphaMap == 0) {
        const unsigned int redGreenMask = g_zVideo_PixelPack.rMask | g_zVideo_PixelPack.gMask;
        {
            for (row = 0; row < image->height; ++row) {
                unsigned short* dstCursor = (unsigned short*)(dstRowBytes);
                {
                    for (column = 0; column < image->width; ++column) {
                        const unsigned int src = *srcPixels++;
                        const unsigned int alphaBit = src != 0 ? 0x8000 : 0;
                        *dstCursor++ = (unsigned short)((src & g_zVideo_PixelPack.bMask)
                            | ((src >> 1) & (redGreenMask >> 1)) | alphaBit);
                    }
                }
                dstRowBytes += pitchBytes;
            }
        }
        return;
    }

    alphaCursor = (unsigned char*)(alphaMap);
    if (g_zVideo_PixelPack.gBits == 6) {
        redAlphaMask = 0xf000;
        redAlphaShift = 4;
        greenAlphaMask = 0x780;
        greenAlphaShift = 3;
    } else {
        redAlphaMask = 0x7800;
        redAlphaShift = 3;
        greenAlphaMask = 0x3c0;
        greenAlphaShift = 2;
    }

    {
        for (row = 0; row < image->height; ++row) {
            unsigned short* dstCursor = (unsigned short*)(dstRowBytes);
            {
                for (column = 0; column < image->width; ++column) {
                    const unsigned short src = *srcPixels++;
                    const unsigned int alpha = (*alphaCursor++ & 0xf0) << 8;
                    *dstCursor++ = (unsigned short)(((src >> 1) & (g_zVideo_PixelPack.bMask >> 1))
                        | ((greenAlphaMask & src) >> greenAlphaShift) | ((redAlphaMask & src) >> redAlphaShift)
                        | alpha);
                }
            }
            dstRowBytes += pitchBytes;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-texture-record-lock-upload-surface
 * @recoil-artifact defines .text recoil:function:0x4aa8b0: zVideo_dd3d::TextureRecordLockUploadSurface.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: locks a texture record's upload surface and returns the provider
 * pixel pointer and row pitch to the caller.
 *
 * Evidence: BN loads m_uploadSurface at offset zero, calls
 * zVideo_dd::LockSurfaceWaitRestore with an uninitialized stack
 * DDSURFACEDESC, copies lpSurface and lPitch to the output pointers only on
 * success, and returns one or zero. The callee owns descriptor clearing and
 * dwSize initialization.
 */
int __fastcall
TextureRecordLockUploadSurface(zVideo_TextureRecordPartial* textureRecord, void** outPixels, int* outPitchBytes)
{
    DDSURFACEDESC lockedDescOut;
    if (LockSurfaceWaitRestore((IDirectDrawSurface3*)(textureRecord->m_uploadSurface), &lockedDescOut) == 0) {
        *outPitchBytes = lockedDescOut.lPitch;
        *outPixels = lockedDescOut.lpSurface;
        return 1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-texture-record-unlock-upload-surface
 * @recoil-artifact defines .text recoil:function:0x4aa8f0: zVideo_dd3d::TextureRecordUnlockUploadSurface.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: unlocks a texture record's upload surface and normalizes provider
 * success to a one-or-zero result.
 *
 * Evidence: BN loads m_uploadSurface at offset zero, calls
 * zVideo_dd::UnlockSurfaceWaitRestore, and uses neg/sbb/inc to return one
 * only when the unlock wrapper returns zero.
 */
int __fastcall TextureRecordUnlockUploadSurface(zVideo_TextureRecordPartial* textureRecord)
{
    return UnlockSurfaceWaitRestore((IDirectDrawSurface3*)(textureRecord->m_uploadSurface)) == 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-texture-record-release-upload-surface-ref
 * @recoil-artifact defines .text recoil:function:0x4aa900: zVideo_dd3d::TextureRecordReleaseUploadSurfaceRef.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: releases and clears the upload-surface reference when one is held by
 * a texture record.
 *
 * Evidence: BN tests m_uploadSurface at offset zero, calls the provider Release
 * slot at vtable offset 8 when non-null, and stores null back to offset zero.
 */
void __fastcall TextureRecordReleaseUploadSurfaceRef(zVideo_TextureRecordPartial* textureRecord)
{
    if (textureRecord->m_uploadSurface != 0) {
        textureRecord->m_uploadSurface->lpVtbl->Release(textureRecord->m_uploadSurface);
        textureRecord->m_uploadSurface = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-texture-record-finalize-upload
 * @recoil-artifact defines .text recoil:function:0x4aa920: zVideo_dd3d::TextureRecordFinalizeUpload.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: optionally refreshes a texture-record upload surface from an image
 * and loads the temporary upload texture into the target Direct3D texture.
 *
 * Evidence: BN exits when m_uploadSurface is null, optionally calls
 * UploadImageToSurface with image->formatFlagsPacked bit 1, queries the upload
 * surface for IDirect3DTexture2, calls targetTexture->Load(uploadTexture), and
 * releases the temporary upload texture only when Load succeeds.
 */
void __fastcall
TextureRecordFinalizeUpload(zVideo_TextureRecordPartial* textureRecord, void* reserved, zVidImagePartial* image)
{
    IDirectDrawSurface* uploadSurface = textureRecord->m_uploadSurface;
    IDirect3DTexture2* targetTexture;
    IDirect3DTexture2* uploadTexture;
    HRESULT hresult;
    if (uploadSurface == 0) {
        return;
    }

    targetTexture = textureRecord->m_texture;
    if (image != 0) {
        UploadImageToSurface(uploadSurface, image, image->formatFlagsPacked & 2);
    }

    hresult = uploadSurface->lpVtbl->QueryInterface(uploadSurface, &IID_IDirect3DTexture2, (void**)(&uploadTexture));
    if (hresult != DD_OK) {
        return;
    }

    hresult = targetTexture->lpVtbl->Load(targetTexture, uploadTexture);
    if (hresult == DD_OK) {
        uploadTexture->lpVtbl->Release(uploadTexture);
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-texture-record-destroy
 * @recoil-artifact defines .text recoil:function:0x4aa980: zVideo_dd3d::TextureRecordDestroy.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: release non-default Direct3D texture-record provider resources and
 * free the texture record.
 *
 * Evidence: BN hoists the upload surface, texture surface, and texture fields
 * before checking the default texture record, then releases each provider
 * reference in field order before freeing the record.
 */
void __fastcall TextureRecordDestroy(zVideo_TextureRecordPartial* textureRecord)
{
    IDirectDrawSurface* uploadSurface = textureRecord->m_uploadSurface;
    IDirectDrawSurface* textureSurface = textureRecord->m_textureSurface;
    IDirect3DTexture2* texture = textureRecord->m_texture;

    if (textureRecord == g_zVideo_DefaultTextureRecord) {
        return;
    }

    if (uploadSurface != 0) {
        uploadSurface->lpVtbl->Release(uploadSurface);
    }
    if (textureSurface != 0) {
        textureSurface->lpVtbl->Release(textureSurface);
    }
    if (texture != 0) {
        texture->lpVtbl->Release(texture);
    }

    free(textureRecord);
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-texture-record-create
 * @recoil-artifact defines .text recoil:function:0x4aa9d0: zVideo_dd3d::TextureRecordCreate.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: allocates a zeroed Direct3D texture-record structure.
 *
 * Evidence: BN assembly is a leaf that calls calloc(1, 0x1c) and returns the
 * provider result directly; zVideo_TextureRecordPartial is asserted to 0x1c.
 */
zVideo_TextureRecordPartial* __cdecl TextureRecordCreate(void)
{
    return (zVideo_TextureRecordPartial*)(calloc(1, sizeof(zVideo_TextureRecordPartial)));
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-set-fog-enable
 * @recoil-artifact defines .text recoil:function:0x4aa9e0: zVideo_dd3d::SetFogEnable.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: update the cached Direct3D fog-enable render state and force the
 * fixed fog light-state mode.
 *
 * Evidence: BN compares g_zVideo_CachedFogEnableRenderState before calling
 * IDirect3DDevice2::SetRenderState(D3DRENDERSTATE_FOGENABLE), stores the new
 * enable value, then ensures D3DLIGHTSTATE_FOGMODE is D3DFOG_LINEAR through
 * IDirect3DDevice2::SetLightState.
 */
void __fastcall SetFogEnable(int enable)
{
    if (g_zVideo_CachedFogEnableRenderState != enable) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_FOGENABLE, (DWORD)(enable));
        g_zVideo_CachedFogEnableRenderState = enable;
    }

    if (g_zVideo_CachedFogModeLightState != 3) {
        g_zVideo_pD3DDevice->lpVtbl->SetLightState(g_zVideo_pD3DDevice, D3DLIGHTSTATE_FOGMODE, D3DFOG_LINEAR);
        g_zVideo_CachedFogModeLightState = 3;
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-set-fog-start
 * @recoil-artifact defines .text recoil:function:0x4aaa30: zVideo_dd3d::SetFogStart.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: update the cached Direct3D fog-start light state only when the
 * requested start distance changes.
 *
 * Evidence: BN shows a stdcall float argument, x87 comparison against
 * g_zVideo_CachedFogStartLightStateValue, then a Direct3D provider
 * SetLightState call with selector D3DLIGHTSTATE_FOGSTART and the raw
 * fogStart float bits before updating the cache.
 */
void __stdcall SetFogStart(float fogStart)
{
    if (g_zVideo_CachedFogStartLightStateValue != fogStart) {
        g_zVideo_pD3DDevice->lpVtbl->SetLightState(g_zVideo_pD3DDevice, (D3DLIGHTSTATETYPE)(5), *(DWORD*)(&fogStart));
        g_zVideo_CachedFogStartLightStateValue = fogStart;
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-set-fog-end
 * @recoil-artifact defines .text recoil:function:0x4aaa60: zVideo_dd3d::SetFogEnd.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: update the cached Direct3D fog-end light-state value only when
 * the requested end distance changes.
 *
 * Evidence: BN shows a stdcall float argument, x87 comparison against
 * g_zVideo_CachedFogEndLightStateValue, then a Direct3D provider
 * SetLightState call using selector 5 with the raw fogEnd float bits before
 * updating the cache. The selector is the retail oddity: SetFogEnd pushes
 * D3DLIGHTSTATE_FOGSTART rather than D3DLIGHTSTATE_FOGEND.
 */
void __stdcall SetFogEnd(float fogEnd)
{
    if (g_zVideo_CachedFogEndLightStateValue != fogEnd) {
        g_zVideo_pD3DDevice->lpVtbl->SetLightState(g_zVideo_pD3DDevice, (D3DLIGHTSTATETYPE)(5), *(DWORD*)(&fogEnd));
        g_zVideo_CachedFogEndLightStateValue = fogEnd;
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-apply-fog-state-from-globals
 * @recoil-artifact defines .text recoil:function:0x4aaa90: zVideo_dd3d::ApplyFogStateFromGlobals.
 * @recoil-match source
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: apply pending global fog enable, color, mode, start, and end
 * state to the active Direct3D device.
 *
 * Evidence: BN shows stdcall fogStart, fogEnd, and unused float arguments
 * with ret 0x0c. The function emits FOGENABLE=1, packs pending RGB globals
 * into FOGCOLOR via the recovered zvid_ddd3d.c helper expression, then sends
 * linear fog mode, raw fogStart bits to D3DLIGHTSTATE_FOGSTART, and raw
 * fogEnd bits to D3DLIGHTSTATE_FOGEND through IDirect3DDevice2 providers.
 */
void __fastcall ApplyFogStateFromGlobals(float fogStart, float fogEnd, float unused)
{
    (void)unused;
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_FOGENABLE, 1);

    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
        g_zVideo_pD3DDevice,
        D3DRENDERSTATE_FOGCOLOR,
        ((
             ((DWORD)((int)(g_zVideo_FogColorPendingR255 + 0.5)) << 8)
             | (DWORD)((int)(g_zVideo_FogColorPendingG255 + 0.5))
         ) << 8)
            | (DWORD)((int)(g_zVideo_FogColorPendingB255 + 0.5))
    );

    g_zVideo_pD3DDevice->lpVtbl->SetLightState(g_zVideo_pD3DDevice, D3DLIGHTSTATE_FOGMODE, D3DFOG_LINEAR);
    g_zVideo_pD3DDevice->lpVtbl->SetLightState(g_zVideo_pD3DDevice, (D3DLIGHTSTATETYPE)(5), *(DWORD*)(&fogStart));
    g_zVideo_pD3DDevice->lpVtbl->SetLightState(g_zVideo_pD3DDevice, (D3DLIGHTSTATETYPE)(6), *(DWORD*)(&fogEnd));
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-update-fog-color
 * @recoil-artifact defines .text recoil:function:0x4aab30: zVideo_dd3d::UpdateFogColor.
 * @recoil-match source
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: upload the applied global fog RGB floats as a packed Direct3D
 * fog-color render state.
 *
 * Evidence: BN shows a cdecl no-argument function that packs
 * g_zVideo_FogColorAppliedR255/G255/B255 with the same +0.5f, _ftol,
 * 0xRRGGBB sequence used by 0x4aaa90, then calls the Direct3D provider
 * SetRenderState for D3DRENDERSTATE_FOGCOLOR.
 */
void __cdecl UpdateFogColor(void)
{
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
        g_zVideo_pD3DDevice,
        D3DRENDERSTATE_FOGCOLOR,
        ((
             ((DWORD)((int)(g_zVideo_FogColorAppliedR255 + 0.5)) << 8)
             | (DWORD)((int)(g_zVideo_FogColorAppliedG255 + 0.5))
         ) << 8)
            | (DWORD)((int)(g_zVideo_FogColorAppliedB255 + 0.5))
    );
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Convert flat 16-bit color polygons to Direct3D TL vertices and
 * submit them through the immediate, overwrite, or sorted transparent path.
 */
void __fastcall SubmitPolyFlatColor16(
    zVideo_XyzVertex* vertices,
    unsigned int packedColor16,
    int alpha,
    int vertexCount,
    int renderParam,
    int queueMode
)
{
    const DWORD packedColor = PackD3DColorFrom16(packedColor16, alpha);

    if (alpha >= 0xff) {
        HRESULT hresult;
        CopyFlatVerticesReverse(g_zVideo_D3DSubmitTempVertices, vertices, vertexCount, packedColor);

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            zVideo_OverwriteQueueEntry* entry;
            int vertexIndex;
            if (queueIndex >= 0x180) {
                ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x503,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedsFmt,
                    queueIndex
                );
                return;
            }

            entry = &g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry->type = 1;
            entry->vertexCount = vertexCount;
            entry->renderClass = 0;
            entry->renderParam = renderParam;
            for (vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                entry->vertices[vertexIndex] = g_zVideo_D3DSubmitTempVertices[vertexIndex];
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
            g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, 0);
            g_zVideo_D3DRenderStateCache.textureHandle = 0;
        }
        if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
            g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 1);
            g_zVideo_D3DRenderStateCache.shadeMode = 1;
        }

        hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
            g_zVideo_pD3DDevice,
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x520);
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        zVideo_OverwriteQueueEntry* entry;
        if (queueIndex >= 0x180) {
            ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x528,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        entry = &g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry->type = 0;
        entry->vertexCount = vertexCount;
        entry->renderClass = 0;
        entry->renderParam = renderParam;
        CopyFlatVerticesReverse(entry->vertices, vertices, vertexCount, packedColor);
        return;
    }

    if (g_zVideo_SortedPolyQueueCount >= 0x100) {
        ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x547,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            g_zVideo_SortedPolyQueueCount
        );
        return;
    }

    // Retail re-indexes the sorted queue by the live count at every store.
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertexCount = vertexCount;
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderClass = 0;
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderParam = renderParam;
    CopyFlatVerticesReverse(
        g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertices,
        vertices,
        vertexCount,
        packedColor
    );
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Convert per-vertex 16-bit color polygons to Direct3D TL vertices
 * and submit them through the immediate, overwrite, or sorted transparent path.
 */
void __fastcall SubmitPolyGouraudColor16(
    zVideo_XyzVertex* vertices,
    unsigned int* packedColors16,
    int alpha,
    int vertexCount,
    int renderParam,
    int queueMode
)
{
    const int lastIndex = vertexCount - 1;
    const zVideo_XyzVertex* sourceVertex = &vertices[lastIndex];
    const unsigned int* sourceColor = &packedColors16[lastIndex];

    if (alpha >= 0xff) {
        HRESULT hresult;
        CopyGouraudVerticesReverse(g_zVideo_D3DSubmitTempVertices, sourceVertex, sourceColor, vertexCount, alpha);

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            zVideo_OverwriteQueueEntry* entry;
            int vertexIndex;
            if (queueIndex >= 0x180) {
                ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x59d,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            entry = &g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry->type = 2;
            entry->vertexCount = vertexCount;
            entry->renderClass = 0;
            entry->renderParam = renderParam;
            for (vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                entry->vertices[vertexIndex] = g_zVideo_D3DSubmitTempVertices[vertexIndex];
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
            g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, 0);
            g_zVideo_D3DRenderStateCache.textureHandle = 0;
        }
        if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
            g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 1);
            g_zVideo_D3DRenderStateCache.shadeMode = 1;
        }

        hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
            g_zVideo_pD3DDevice,
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x5bb);
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        zVideo_OverwriteQueueEntry* entry;
        if (queueIndex >= 0x180) {
            ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x5c3,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        entry = &g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry->type = 0;
        entry->vertexCount = vertexCount;
        entry->renderClass = 0;
        entry->renderParam = renderParam;
        CopyGouraudVerticesReverse(entry->vertices, sourceVertex, sourceColor, vertexCount, alpha);
        return;
    }

    if (g_zVideo_SortedPolyQueueCount >= 0x100) {
        ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x5e2,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            g_zVideo_SortedPolyQueueCount
        );
        return;
    }

    // Retail re-indexes the sorted queue by the live count at every store.
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertexCount = vertexCount;
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderClass = 0;
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderParam = renderParam;
    CopyGouraudVerticesReverse(
        g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertices,
        sourceVertex,
        sourceColor,
        vertexCount,
        alpha
    );
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Purpose: Build color-attribute TL vertices and either draw immediately or queue
 * the overwrite polygon path.
 */
void __fastcall SubmitPolyColorAttr(
    zVideo_XyzVertex* vertices,
    unsigned int packedColor16,
    zVideo_ColorRgbFloat* baseColor,
    float* attr1,
    float* attr0,
    float* attr2,
    int alpha,
    int vertexCount,
    unsigned int renderParam,
    int queueMode
)
{
    int lastIndex;
    float attr1Scale;
    DWORD alphaBits;
    HRESULT hresult;
    (void)packedColor16;

    // Retail computes vertexCount - 1 first (lea eax,[ebp-1]) and recomputes it at each reverse copy.
    lastIndex = vertexCount - 1;
    attr1Scale = 1.0f - *attr1;
    alphaBits = alpha >= 0xff ? 0xff000000 : (DWORD)(alpha << 24);

    FillColorAttrSpecularReverse(attr2, lastIndex, vertexCount);
    FillColorAttrColorsReverse(*baseColor, attr0, lastIndex, attr1Scale, alphaBits, vertexCount);
    if (alpha < 0xff) {
        return;
    }

    CopyPositionsReverse(g_zVideo_D3DSubmitTempVertices, vertices, lastIndex, vertexCount);

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        zVideo_OverwriteQueueEntry* entry;
        int vertexIndex;
        if (queueIndex >= 0x180) {
            ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x69c,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        entry = &g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry->type = 3;
        entry->vertexCount = vertexCount;
        entry->renderClass = 0;
        entry->renderParam = (int)(renderParam);
        for (vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            entry->vertices[vertexIndex] = g_zVideo_D3DSubmitTempVertices[vertexIndex];
        }
        return;
    }

    if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, 0);
        g_zVideo_D3DRenderStateCache.textureHandle = 0;
    }
    if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 1);
        g_zVideo_D3DRenderStateCache.shadeMode = 1;
    }

    hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
        g_zVideo_pD3DDevice,
        (D3DPRIMITIVETYPE)(6),
        (D3DVERTEXTYPE)(3),
        g_zVideo_D3DSubmitTempVertices,
        (DWORD)(vertexCount),
        0
    );
    if (hresult != DD_OK) {
        ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x6ba);
    }
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Prepare textured TL vertices for a render class and route them to
 * immediate Direct3D drawing or the overwrite/sorted polygon queues.
 */
void __fastcall SubmitPolyRenderClass(
    zVideo_XyzVertex* vertices,
    zVideo_TexCoord* texCoords,
    int vertexCount,
    zVideo_RenderClass* renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
)
{
    DWORD alphaWhite;
    if (renderClass->textureMapBlend != (D3DTEXTUREBLEND)(4) && alpha >= 1.0f) {
        HRESULT hresult;
        CopyTexturedVerticesReverse(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            texCoords,
            vertexCount,
            g_zVideo_OpaqueWhiteArgb
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            zVideo_OverwriteQueueEntry* entry;
            int vertexIndex;
            if (queueIndex >= 0x180) {
                ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x6fd,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            entry = &g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry->type = 4;
            entry->vertexCount = vertexCount;
            entry->renderClass = renderClass;
            entry->renderParam = (int)(renderParam);
            for (vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                entry->vertices[vertexIndex] = g_zVideo_D3DSubmitTempVertices[vertexIndex];
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
            g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 1);
            g_zVideo_D3DRenderStateCache.shadeMode = 1;
        }
        if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, renderClass->textureHandle);
            g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderStateCache.textureMapBlend != renderClass->textureMapBlend) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMAPBLEND, renderClass->textureMapBlend);
            g_zVideo_D3DRenderStateCache.textureMapBlend = renderClass->textureMapBlend;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREADDRESSU, renderClass->textureAddressU);
            g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREADDRESSV, renderClass->textureAddressV);
            g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
        }

        hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
            g_zVideo_pD3DDevice,
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x71d);
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        DWORD alphaWhite;
        zVideo_OverwriteQueueEntry* entry;
        if (queueIndex >= 0x180) {
            ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x725,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        alphaWhite = PackAlphaWhite(alpha);
        entry = &g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry->type = 0;
        entry->vertexCount = vertexCount;
        entry->renderClass = renderClass;
        entry->renderParam = (int)(renderParam);
        CopyTexturedVerticesReverseQueued(entry->vertices, vertices, texCoords, vertexCount, alphaWhite);
        return;
    }

    if (g_zVideo_SortedPolyQueueCount >= 0x100) {
        ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x74c,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            g_zVideo_SortedPolyQueueCount
        );
        return;
    }

    alphaWhite = PackAlphaWhite(alpha);
    // Retail re-indexes the sorted queue by the live count at every store.
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertexCount = vertexCount;
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderClass = renderClass;
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderParam = (int)(renderParam);
    CopyTexturedVerticesReverseQueued(
        g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertices,
        vertices,
        texCoords,
        vertexCount,
        alphaWhite
    );
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-submit-polygon
 * @recoil-artifact defines .text recoil:function:0x4abb20: zVideo_dd3d::SubmitPolygon.
 *
 *
 * Purpose: Build textured polygon TL vertices with fog color-attribute bias and
 * route them to immediate, overwrite, or sorted transparent submission.
 */
void __fastcall SubmitPolygon(
    zVideo_XyzVertex* vertices,
    zVideo_TexCoord* uvPairs,
    float* attr1,
    float* attr0,
    float* attr2,
    int vertexCount,
    zVideo_RenderClass* renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
)
{
    const float attr1Scale = 1.0f - *attr1;
    const int lastIndex = vertexCount - 1;
    const DWORD alphaBits = alpha >= 1.0f ? 0xff000000 : ((DWORD)((int)(alpha * 255.0f)) << 24);
    int vertexIndex;

    FillColorAttrSpecularReverse(attr2, lastIndex, vertexCount);
    FillPolygonColorsReverse(attr0, lastIndex, attr1Scale * 255.0f, alphaBits, vertexCount);

    if (renderClass->textureMapBlend != (D3DTEXTUREBLEND)(4) && alpha >= 1.0f) {
        HRESULT hresult;
        CopyPositionUvReversePreserveColor(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            uvPairs,
            lastIndex,
            vertexCount,
            vertexIndex
        );
        AppendFanCloseVertexIfNeeded(g_zVideo_D3DSubmitTempVertices, vertexIndex, vertexCount);

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            zVideo_OverwriteQueueEntry* entry;
            int copyIndex;
            if (queueIndex >= 0x180) {
                ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x82a,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            entry = &g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry->type = 5;
            entry->vertexCount = vertexCount;
            entry->renderClass = renderClass;
            entry->renderParam = (int)(renderParam);
            for (copyIndex = 0; copyIndex < vertexCount; ++copyIndex) {
                entry->vertices[copyIndex] = g_zVideo_D3DSubmitTempVertices[copyIndex];
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
            g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 2);
            g_zVideo_D3DRenderStateCache.shadeMode = 2;
        }
        if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, renderClass->textureHandle);
            g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(2)) {
            g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMAPBLEND, 2);
            g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREADDRESSU, renderClass->textureAddressU);
            g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREADDRESSV, renderClass->textureAddressV);
            g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
        }

        hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
            g_zVideo_pD3DDevice,
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x84a);
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        zVideo_OverwriteQueueEntry* entry;
        if (queueIndex >= 0x180) {
            ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x853,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        entry = &g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry->renderParam = (int)(renderParam);
        entry->renderClass = renderClass;
        entry->type = 0;
        CopyPositionUvWithPreparedColorReverse(
            entry->vertices,
            vertices,
            uvPairs,
            g_zVideo_D3DSubmitTempVertices,
            lastIndex,
            vertexCount,
            vertexIndex
        );
        AppendFanCloseVertexIfNeeded(entry->vertices, vertexIndex, vertexCount);
        entry->vertexCount = vertexCount;
        return;
    }

    if (g_zVideo_SortedPolyQueueCount >= 0x100) {
        ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x88a,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            g_zVideo_SortedPolyQueueCount
        );
        return;
    }

    // Retail re-indexes the sorted queue by the live count at every store.
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderClass = renderClass;
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderParam = (int)(renderParam);
    CopyPositionUvWithPreparedColorReverse(
        g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertices,
        vertices,
        uvPairs,
        g_zVideo_D3DSubmitTempVertices,
        lastIndex,
        vertexCount,
        vertexIndex
    );
    AppendFanCloseVertexIfNeeded(
        g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertices,
        vertexIndex,
        vertexCount
    );
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertexCount = vertexCount;
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-submit-polygon-lit
 * @recoil-artifact defines .text recoil:function:0x4ac370: zVideo_dd3d::SubmitPolygonLit.
 * @recoil-match source
 *
 * Purpose: Build lit textured polygon TL vertices with fog color-attribute bias
 * and route them to immediate, overwrite, or sorted transparent submission.
 */
void __fastcall SubmitPolygonLit(
    zVideo_XyzVertex* vertices,
    zVideo_TexCoord* uvPairs,
    float* attr1,
    float* attr0,
    float* attr2,
    int vertexCount,
    zVideo_RenderClass* renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
)
{
    const int lastIndex = vertexCount - 1;
    const DWORD alphaBits = alpha >= 1.0f ? 0xff000000 : ((DWORD)((int)(alpha * 255.0f)) << 24);
    int vertexIndex;

    FillColorAttrSpecularReverse(attr2, lastIndex, vertexCount);
    FillPolygonLitColorsReverse(attr1, attr0, lastIndex, alphaBits, vertexCount);

    if (renderClass->textureMapBlend != (D3DTEXTUREBLEND)(4) && alpha >= 1.0f) {
        HRESULT hresult;
        CopyPositionUvReversePreserveColor(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            uvPairs,
            lastIndex,
            vertexCount,
            vertexIndex
        );
        AppendFanCloseVertexIfNeeded(g_zVideo_D3DSubmitTempVertices, vertexIndex, vertexCount);

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            zVideo_OverwriteQueueEntry* entry;
            int copyIndex;
            if (queueIndex >= 0x180) {
                ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x983,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            entry = &g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry->type = 6;
            entry->vertexCount = vertexCount;
            entry->renderClass = renderClass;
            entry->renderParam = (int)(renderParam);
            for (copyIndex = 0; copyIndex < vertexCount; ++copyIndex) {
                entry->vertices[copyIndex] = g_zVideo_D3DSubmitTempVertices[copyIndex];
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
            g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 2);
            g_zVideo_D3DRenderStateCache.shadeMode = 2;
        }
        if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, renderClass->textureHandle);
            g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(2)) {
            g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMAPBLEND, 2);
            g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREADDRESSU, renderClass->textureAddressU);
            g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->lpVtbl
                ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREADDRESSV, renderClass->textureAddressV);
            g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
        }

        hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
            g_zVideo_pD3DDevice,
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0x9a4);
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        zVideo_OverwriteQueueEntry* entry;
        if (queueIndex >= 0x180) {
            ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x9ad,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        entry = &g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry->renderParam = (int)(renderParam);
        entry->renderClass = renderClass;
        entry->type = 0;
        CopyPositionUvWithPreparedColorReverse(
            entry->vertices,
            vertices,
            uvPairs,
            g_zVideo_D3DSubmitTempVertices,
            lastIndex,
            vertexCount,
            vertexIndex
        );
        AppendFanCloseVertexIfNeeded(entry->vertices, vertexIndex, vertexCount);
        entry->vertexCount = vertexCount;
        return;
    }

    if (g_zVideo_SortedPolyQueueCount >= 0x100) {
        ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x9e4,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            g_zVideo_SortedPolyQueueCount
        );
        return;
    }

    // Retail re-indexes the sorted queue by the live count at every store.
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderClass = renderClass;
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].renderParam = (int)(renderParam);
    CopyPositionUvWithPreparedColorReverse(
        g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertices,
        vertices,
        uvPairs,
        g_zVideo_D3DSubmitTempVertices,
        lastIndex,
        vertexCount,
        vertexIndex
    );
    AppendFanCloseVertexIfNeeded(
        g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertices,
        vertexIndex,
        vertexCount
    );
    g_zVideo_SortedPolyQueueBase[g_zVideo_SortedPolyQueueCount].vertexCount = vertexCount;
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Convert one 16-bit colored point to a Direct3D TL vertex and draw it
 * through the cached point-list render-state path.
 */
void __fastcall DrawPointColor16(zVideo_XyzVertex* pointPos, unsigned int packedColor16, int pointCount)
{
    D3DTLVERTEX* vertex;
    HRESULT hresult;
    (void)pointCount;

    vertex = &g_zVideo_D3DSubmitTempVertices[0];
    vertex->sx = pointPos->x;
    vertex->sy = pointPos->y;
    vertex->sz = pointPos->z;
    vertex->rhw = pointPos->z;
    vertex->color = PackD3DColorFrom16(packedColor16, 0xff);
    vertex->specular = 0xff000000;

    if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, 0);
        g_zVideo_D3DRenderStateCache.textureHandle = 0;
    }
    if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 1);
        g_zVideo_D3DRenderStateCache.shadeMode = 1;
    }

    hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
        g_zVideo_pD3DDevice,
        (D3DPRIMITIVETYPE)(1),
        (D3DVERTEXTYPE)(3),
        g_zVideo_D3DSubmitTempVertices,
        1,
        0
    );
    if (hresult != DD_OK) {
        ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xa4c);
    }
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-set-quad-batch-depth-and-rhw
 * @recoil-artifact defines .text recoil:function:0x4accc0: zVideo_dd3d::SetQuadBatchDepthAndRhw.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: stamp the current Direct3D quad-batch depth and reciprocal
 * homogeneous weight across all cached TL vertices.
 *
 * Evidence: BN uses a bottomRight.z cursor and writes each item's TL vertices
 * in bottom-left, bottom-right, top-right, top-left order for z, then the
 * same order for rhw.
 */
void __stdcall SetQuadBatchDepthAndRhw(float depthAndRhw)
{
    int itemIndex;
    for (itemIndex = 0; itemIndex < 16; ++itemIndex) {
        zVideo_QuadBatchItemPartial* item = &g_zVideo_QuadBatchItemsBase[itemIndex];

        item->vertices[3].sz = depthAndRhw;
        item->vertices[2].sz = depthAndRhw;
        item->vertices[1].sz = depthAndRhw;
        item->vertices[0].sz = depthAndRhw;
        item->vertices[3].rhw = depthAndRhw;
        item->vertices[2].rhw = depthAndRhw;
        item->vertices[1].rhw = depthAndRhw;
        item->vertices[0].rhw = depthAndRhw;
    }
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Queue one alpha-blended solid screen-space quad for the Direct3D batch flush.
 */
void __fastcall QueueSolidQuad(unsigned int packedColor16, double alpha, zVidRect32* clipRect)
{
    const int batchIndex = g_zVideo_QuadBatchCount;
    if ((unsigned int)(batchIndex) >= 0x10) {
        return;
    }

    // Retail stores each shared edge through chained assignments.
    if (clipRect == 0) {
        g_zVideo_QuadBatchItemsBase[batchIndex].vertices[0].sx = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[3].sx
            = 0.0f;
        g_zVideo_QuadBatchItemsBase[batchIndex].vertices[1].sx = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[2].sx
            = (float)(DWORD)(g_zVideo_PrimarySurfaceState.height);
        g_zVideo_QuadBatchItemsBase[batchIndex].vertices[0].sy = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[1].sy
            = 0.0f;
        g_zVideo_QuadBatchItemsBase[batchIndex].vertices[2].sy = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[3].sy
            = (float)(DWORD)(g_zVideo_PrimarySurfaceState.width);
    } else {
        g_zVideo_QuadBatchItemsBase[batchIndex].vertices[0].sx = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[3].sx
            = (float)(clipRect->left);
        g_zVideo_QuadBatchItemsBase[batchIndex].vertices[1].sx = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[2].sx
            = (float)(clipRect->right);
        g_zVideo_QuadBatchItemsBase[batchIndex].vertices[0].sy = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[1].sy
            = (float)(clipRect->top);
        g_zVideo_QuadBatchItemsBase[batchIndex].vertices[2].sy = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[3].sy
            = (float)(clipRect->bottom);
    }

    g_zVideo_QuadBatchItemsBase[batchIndex].vertices[0].color
        = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[1].color
        = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[2].color
        = g_zVideo_QuadBatchItemsBase[batchIndex].vertices[3].color
        = PackD3DColorFrom16(packedColor16, (int)(alpha * 255.0));

    ++g_zVideo_QuadBatchCount;
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Sort and draw queued Direct3D polys while maintaining the shared render-state cache.
 */
void __cdecl FlushSortedPolys(void)
{
    unsigned int i;
    int swapped;

    if (g_zVideo_SortedPolyQueueCount == 0) {
        return;
    }

    if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 2);
        g_zVideo_D3DRenderStateCache.shadeMode = 2;
    }
    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 1) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, 1);
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 0) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE, 0);
        g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
    }

    if (g_zVideo_SortedPolyQueueCount > 0) {
        for (i = 0; i < g_zVideo_SortedPolyQueueCount; ++i) {
            g_zVideo_SortedPolyDrawOrder[i] = g_zVideo_SortedPolyQueueCount - i - 1;
        }

        do {
            swapped = 0;
            for (i = 0; i < g_zVideo_SortedPolyQueueCount - 1; ++i) {
                const int previousIndex = g_zVideo_SortedPolyDrawOrder[i];
                const int currentIndex = g_zVideo_SortedPolyDrawOrder[i + 1];
                if (g_zVideo_SortedPolyQueueBase[currentIndex].vertices[0].sz
                    < g_zVideo_SortedPolyQueueBase[previousIndex].vertices[0].sz) {
                    g_zVideo_SortedPolyDrawOrder[i] = currentIndex;
                    g_zVideo_SortedPolyDrawOrder[i + 1] = previousIndex;
                    swapped = 1;
                }
            }
        } while (swapped);

        for (i = 0; i < g_zVideo_SortedPolyQueueCount; ++i) {
            const int drawIndex = g_zVideo_SortedPolyDrawOrder[i];
            HRESULT hresult;

            if (g_zVideo_SortedPolyQueueBase[drawIndex].renderClass != 0) {
                D3DTEXTUREBLEND textureMapBlend;
                if (g_zVideo_D3DRenderStateCache.textureHandle
                    != g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureHandle) {
                    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                        g_zVideo_pD3DDevice,
                        D3DRENDERSTATE_TEXTUREHANDLE,
                        g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureHandle
                    );
                    g_zVideo_D3DRenderStateCache.textureHandle
                        = g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureHandle;
                }

                textureMapBlend = g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureMapBlend;
                if (textureMapBlend != (D3DTEXTUREBLEND)(4)
                    && (g_zVideo_SortedPolyQueueBase[drawIndex].vertices[0].color & 0xff000000) != 0xff000000) {
                    if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(4)) {
                        g_zVideo_pD3DDevice->lpVtbl
                            ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMAPBLEND, 4);
                        g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(4);
                    }
                } else if (g_zVideo_D3DRenderStateCache.textureMapBlend != textureMapBlend) {
                    g_zVideo_pD3DDevice->lpVtbl
                        ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMAPBLEND, textureMapBlend);
                    g_zVideo_D3DRenderStateCache.textureMapBlend
                        = g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureMapBlend;
                }

                if (g_zVideo_D3DRenderStateCache.textureAddressU
                    != g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureAddressU) {
                    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                        g_zVideo_pD3DDevice,
                        D3DRENDERSTATE_TEXTUREADDRESSU,
                        g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureAddressU
                    );
                    g_zVideo_D3DRenderStateCache.textureAddressU
                        = g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureAddressU;
                }
                if (g_zVideo_D3DRenderStateCache.textureAddressV
                    != g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureAddressV) {
                    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                        g_zVideo_pD3DDevice,
                        D3DRENDERSTATE_TEXTUREADDRESSV,
                        g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureAddressV
                    );
                    g_zVideo_D3DRenderStateCache.textureAddressV
                        = g_zVideo_SortedPolyQueueBase[drawIndex].renderClass->textureAddressV;
                }
            } else if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, 0);
                g_zVideo_D3DRenderStateCache.textureHandle = 0;
            }

            hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
                g_zVideo_pD3DDevice,
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                g_zVideo_SortedPolyQueueBase[drawIndex].vertices,
                (DWORD)(g_zVideo_SortedPolyQueueBase[drawIndex].vertexCount),
                0
            );
            if (hresult != DD_OK) {
                ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xb09);
            }
        }
    }

    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 0) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, 0);
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 1) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE, 1);
        g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
    }
    g_zVideo_SortedPolyQueueCount = 0;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd3d-flush-quad-batch
 * @recoil-artifact defines .text recoil:function:0x4ad120: zVideo_dd3d::FlushQuadBatch.
 * @recoil-match byte
 *
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Draw and clear the Direct3D solid-quad batch with cached render-state setup and restoration.
 */
void __cdecl FlushQuadBatch(void)
{
    unsigned int i;
    if (g_zVideo_QuadBatchCount == 0) {
        return;
    }

    if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 2);
        g_zVideo_D3DRenderStateCache.shadeMode = 2;
    }
    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 1) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, 1);
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 0) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE, 0);
        g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
    }
    if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, 0);
        g_zVideo_D3DRenderStateCache.textureHandle = 0;
    }

    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);

    for (i = 0; i < (unsigned int)(g_zVideo_QuadBatchCount); ++i) {
        g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
            g_zVideo_pD3DDevice,
            D3DPT_TRIANGLEFAN,
            (D3DVERTEXTYPE)(3),
            g_zVideo_QuadBatchItemsBase[i].vertices,
            4,
            0
        );
    }

    g_zVideo_QuadBatchCount = 0;
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZFUNC, D3DCMP_GREATEREQUAL);

    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 0) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, 0);
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 1) {
        g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE, 1);
        g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
    }
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Draw overwrite-queue primitives with the Direct3D render-state cache and restore depth testing.
 */
void __cdecl FlushOverwritePolys(void)
{
    HRESULT hresult;
    int i;
    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZFUNC, D3DCMP_ALWAYS);

    for (i = 0; i < g_zVideo_OverwriteQueueCount; ++i) {
        zVideo_OverwriteQueueEntry* entry = &g_zVideo_OverwriteQueueBase[i];

        switch (entry->type) {
        case 0:
            if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 2);
                g_zVideo_D3DRenderStateCache.shadeMode = 2;
            }
            if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 1) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, 1);
                g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
            }
            if (g_zVideo_D3DRenderStateCache.zWriteEnable != 0) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE, 0);
                g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
            }

            if (entry->renderClass != 0) {
                D3DTEXTUREBLEND textureMapBlend;
                if (g_zVideo_D3DRenderStateCache.textureHandle != entry->renderClass->textureHandle) {
                    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                        g_zVideo_pD3DDevice,
                        D3DRENDERSTATE_TEXTUREHANDLE,
                        entry->renderClass->textureHandle
                    );
                    g_zVideo_D3DRenderStateCache.textureHandle = entry->renderClass->textureHandle;
                }

                textureMapBlend = entry->renderClass->textureMapBlend;
                if (textureMapBlend != (D3DTEXTUREBLEND)(4) && (entry->vertices[0].color & 0xff000000) != 0xff000000) {
                    if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(4)) {
                        g_zVideo_pD3DDevice->lpVtbl
                            ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMAPBLEND, 4);
                        g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(4);
                    }
                } else if (g_zVideo_D3DRenderStateCache.textureMapBlend != textureMapBlend) {
                    g_zVideo_pD3DDevice->lpVtbl
                        ->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMAPBLEND, textureMapBlend);
                    g_zVideo_D3DRenderStateCache.textureMapBlend = entry->renderClass->textureMapBlend;
                }

                if (g_zVideo_D3DRenderStateCache.textureAddressU != entry->renderClass->textureAddressU) {
                    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                        g_zVideo_pD3DDevice,
                        D3DRENDERSTATE_TEXTUREADDRESSU,
                        entry->renderClass->textureAddressU
                    );
                    g_zVideo_D3DRenderStateCache.textureAddressU = entry->renderClass->textureAddressU;
                }
                if (g_zVideo_D3DRenderStateCache.textureAddressV != entry->renderClass->textureAddressV) {
                    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                        g_zVideo_pD3DDevice,
                        D3DRENDERSTATE_TEXTUREADDRESSV,
                        entry->renderClass->textureAddressV
                    );
                    g_zVideo_D3DRenderStateCache.textureAddressV = entry->renderClass->textureAddressV;
                }
            } else {
                if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
                    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, 0);
                    g_zVideo_D3DRenderStateCache.textureHandle = 0;
                }
            }

            hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
                g_zVideo_pD3DDevice,
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry->vertices,
                (DWORD)(entry->vertexCount),
                0
            );

            if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 0) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ALPHABLENDENABLE, 0);
                g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
            }
            if (g_zVideo_D3DRenderStateCache.zWriteEnable != 1) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZWRITEENABLE, 1);
                g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
            }
            break;
        case 1:
        case 2:
        case 3:
            if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREHANDLE, 0);
                g_zVideo_D3DRenderStateCache.textureHandle = 0;
            }
            if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 1);
                g_zVideo_D3DRenderStateCache.shadeMode = 1;
            }
            hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
                g_zVideo_pD3DDevice,
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry->vertices,
                (DWORD)(entry->vertexCount),
                0
            );
            break;
        case 4:
            if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 1);
                g_zVideo_D3DRenderStateCache.shadeMode = 1;
            }
            if (g_zVideo_D3DRenderStateCache.textureHandle != entry->renderClass->textureHandle) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                    g_zVideo_pD3DDevice,
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    entry->renderClass->textureHandle
                );
                g_zVideo_D3DRenderStateCache.textureHandle = entry->renderClass->textureHandle;
            }
            if (g_zVideo_D3DRenderStateCache.textureMapBlend != entry->renderClass->textureMapBlend) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                    g_zVideo_pD3DDevice,
                    D3DRENDERSTATE_TEXTUREMAPBLEND,
                    entry->renderClass->textureMapBlend
                );
                g_zVideo_D3DRenderStateCache.textureMapBlend = entry->renderClass->textureMapBlend;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressU != entry->renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                    g_zVideo_pD3DDevice,
                    D3DRENDERSTATE_TEXTUREADDRESSU,
                    entry->renderClass->textureAddressU
                );
                g_zVideo_D3DRenderStateCache.textureAddressU = entry->renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressV != entry->renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                    g_zVideo_pD3DDevice,
                    D3DRENDERSTATE_TEXTUREADDRESSV,
                    entry->renderClass->textureAddressV
                );
                g_zVideo_D3DRenderStateCache.textureAddressV = entry->renderClass->textureAddressV;
            }
            hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
                g_zVideo_pD3DDevice,
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry->vertices,
                (DWORD)(entry->vertexCount),
                0
            );
            break;
        case 5:
        case 6:
            if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_SHADEMODE, 2);
                g_zVideo_D3DRenderStateCache.shadeMode = 2;
            }
            if (g_zVideo_D3DRenderStateCache.textureHandle != entry->renderClass->textureHandle) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                    g_zVideo_pD3DDevice,
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    entry->renderClass->textureHandle
                );
                g_zVideo_D3DRenderStateCache.textureHandle = entry->renderClass->textureHandle;
            }
            if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(2)) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_TEXTUREMAPBLEND, 2);
                g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressU != entry->renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                    g_zVideo_pD3DDevice,
                    D3DRENDERSTATE_TEXTUREADDRESSU,
                    entry->renderClass->textureAddressU
                );
                g_zVideo_D3DRenderStateCache.textureAddressU = entry->renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressV != entry->renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->lpVtbl->SetRenderState(
                    g_zVideo_pD3DDevice,
                    D3DRENDERSTATE_TEXTUREADDRESSV,
                    entry->renderClass->textureAddressV
                );
                g_zVideo_D3DRenderStateCache.textureAddressV = entry->renderClass->textureAddressV;
            }
            hresult = g_zVideo_pD3DDevice->lpVtbl->DrawPrimitive(
                g_zVideo_pD3DDevice,
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry->vertices,
                (DWORD)(entry->vertexCount),
                0
            );
            break;
        }

        if (hresult != DD_OK) {
            ReportError((int)(hresult), g_zVideo_SourceFile_ZvidDdd3dC, 0xbb7);
        }
    }

    g_zVideo_pD3DDevice->lpVtbl->SetRenderState(g_zVideo_pD3DDevice, D3DRENDERSTATE_ZFUNC, D3DCMP_GREATEREQUAL);
    g_zVideo_OverwriteQueueCount = 0;
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: return the largest power of two less than or equal to the supplied
 * value.
 *
 * Evidence: BN starts from one, shifts left until the running power reaches or
 * exceeds the input, returns the input on exact match, otherwise shifts once
 * back down before returning.
 */
int __fastcall FloorPowerOfTwo(int value)
{
    int powerOfTwo = 1;
    do {
        powerOfTwo <<= 1;
    } while (powerOfTwo < value);

    if (powerOfTwo == value) {
        return value;
    }

    return powerOfTwo >> 1;
}

/**
 * @recoil-anchor recoil:anchor:zvideo.zvid-ddd3d.z-video-dd-report-error
 * @recoil-artifact defines .text recoil:function:0x4ad6a0: zVideo_dd::ReportError.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: maps DirectDraw/Direct3D HRESULTs to report text and emits the legacy DirectDraw error report.
 */
RECOIL_NO_GS int __fastcall ReportError(int hresult, const char* sourceFile, int sourceLine)
{
    char errorNameBuffer[0x100];
    char reportMessageBuffer[0x100];

#define ZVIDEO_DD_REPORT_ERROR_NAME(nameText) sprintf(errorNameBuffer, nameText)

    switch (hresult) {
    case DDERR_UNSUPPORTED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Unsupported);
        break;
    case DDERR_GENERIC:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Generic);
        break;
    case DDERR_NOTINITIALIZED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotInitialized);
        break;
    case DDERR_OUTOFMEMORY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OutOfMemory);
        break;
    case DDERR_INVALIDPARAMS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidParams);
        break;
    case DDERR_ALREADYINITIALIZED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_AlreadyInitialized);
        break;
    case DDERR_CANNOTATTACHSURFACE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CannotAttachSurface);
        break;
    case DDERR_CANNOTDETACHSURFACE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CannotDetachSurface);
        break;
    case DDERR_CURRENTLYNOTAVAIL:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CurrentlyNotAvail);
        break;
    case DDERR_EXCEPTION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Exception);
        break;
    case DDERR_HEIGHTALIGN:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_HeightAlign);
        break;
    case DDERR_INVALIDCAPS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidCaps);
        break;
    case DDERR_INVALIDCLIPLIST:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidClipList);
        break;
    case DDERR_INVALIDMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidMode);
        break;
    case DDERR_INVALIDOBJECT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidObject);
        break;
    case DDERR_INVALIDPIXELFORMAT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidPixelFormat);
        break;
    case DDERR_INVALIDRECT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidRect);
        break;
    case DDERR_LOCKEDSURFACES:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_LockedSurfaces);
        break;
    case DDERR_NO3D:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_No3d);
        break;
    case DDERR_NOALPHAHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoAlphaHw);
        break;
    case DDERR_NOCLIPLIST:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoClipList);
        break;
    case DDERR_NOCOLORCONVHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoColorConvHw);
        break;
    case DDERR_NOCOOPERATIVELEVELSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoCooperativeLevelSet);
        break;
    case DDERR_NOCOLORKEY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoColorKey);
        break;
    case DDERR_NOEXCLUSIVEMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoExclusiveMode);
        break;
    case DDERR_NODIRECTDRAWSUPPORT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDirectDrawSupport);
        break;
    case DDERR_NOCOLORKEYHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoColorKeyHw);
        break;
    case DDERR_NOFLIPHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoFlipHw);
        break;
    case DDERR_NOGDI:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoGdi);
        break;
    case DDERR_NOMIRRORHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoMirrorHw);
        break;
    case DDERR_NOTFOUND:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotFound);
        break;
    case DDERR_NOOVERLAYHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoOverlayHw);
        break;
    case DDERR_NORASTEROPHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoRasterOpHw);
        break;
    case DDERR_NOROTATIONHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoRotationHw);
        break;
    case DDERR_NOSTRETCHHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoStretchHw);
        break;
    case DDERR_NOT4BITCOLOR:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Not4BitColor);
        break;
    case DDERR_NOT4BITCOLORINDEX:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Not4BitColorIndex);
        break;
    case DDERR_NOT8BITCOLOR:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Not8BitColor);
        break;
    case DDERR_NOTEXTUREHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoTextureHw);
        break;
    case DDERR_NOVSYNCHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoVSyncHw);
        break;
    case DDERR_NOZBUFFERHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoZBufferHw);
        break;
    case DDERR_NOZOVERLAYHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoZOverlayHw);
        break;
    case DDERR_OUTOFCAPS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OutOfCaps);
        break;
    case DDERR_OUTOFVIDEOMEMORY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OutOfVideoMemory);
        break;
    case DDERR_PALETTEBUSY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_PaletteBusy);
        break;
    case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OverlayColorKeyOnlyOneActive);
        break;
    case DDERR_OVERLAYCANTCLIP:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OverlayCantClip);
        break;
    case DDERR_COLORKEYNOTSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ColorKeyNotSet);
        break;
    case DDERR_SURFACEALREADYATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceAlreadyAttached);
        break;
    case DDERR_SURFACEALREADYDEPENDENT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceAlreadyDependent);
        break;
    case DDERR_SURFACEBUSY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceBusy);
        break;
    case DDERR_CANTLOCKSURFACE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantLockSurface);
        break;
    case DDERR_SURFACEISOBSCURED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceIsObscured);
        break;
    case DDERR_SURFACELOST:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceLost);
        break;
    case DDERR_SURFACENOTATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_SurfaceNotAttached);
        break;
    case DDERR_TOOBIGHEIGHT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_TooBigHeight);
        break;
    case DDERR_TOOBIGSIZE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_TooBigSize);
        break;
    case DDERR_TOOBIGWIDTH:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_TooBigWidth);
        break;
    case DDERR_UNSUPPORTEDFORMAT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_UnsupportedFormat);
        break;
    case DDERR_UNSUPPORTEDMASK:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_UnsupportedMask);
        break;
    case DDERR_VERTICALBLANKINPROGRESS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_VerticalBlankInProgress);
        break;
    case DDERR_WASSTILLDRAWING:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_WasStillDrawing);
        break;
    case DDERR_BLTFASTCANTCLIP:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_BltFastCantClip);
        break;
    case DDERR_CANTCREATEDC:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantCreateDc);
        break;
    case DDERR_CANTDUPLICATE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantDuplicate);
        break;
    case DDERR_CLIPPERISUSINGHWND:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ClipperIsUsingHwnd);
        break;
    case DDERR_DCALREADYCREATED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_DcAlreadyCreated);
        break;
    case DDERR_DIRECTDRAWALREADYCREATED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_DirectDrawAlreadyCreated);
        break;
    case DDERR_EXCLUSIVEMODEALREADYSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ExclusiveModeAlreadySet);
        break;
    case DDERR_HWNDALREADYSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_HwndAlreadySet);
        break;
    case DDERR_HWNDSUBCLASSED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_HwndSubclassed);
        break;
    case DDERR_IMPLICITLYCREATED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ImplicitlyCreated);
        break;
    case DDERR_INVALIDDIRECTDRAWGUID:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidDirectDrawGuid);
        break;
    case DDERR_INVALIDPOSITION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidPosition);
        break;
    case DDERR_INVALIDSURFACETYPE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidSurfaceType);
        break;
    case DDERR_NOBLTHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoBltHw);
        break;
    case DDERR_NOCLIPPERATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoClipperAttached);
        break;
    case DDERR_NODC:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDirectDc);
        break;
    case DDERR_NODDROPSHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDdRopsHw);
        break;
    case DDERR_NODIRECTDRAWHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDirectDrawHw);
        break;
    case DDERR_NOEMULATION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoEmulation);
        break;
    case DDERR_NOHWND:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoHwnd);
        break;
    case DDERR_NOMIPMAPHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoMipMapHw);
        break;
    case DDERR_NOPALETTEATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoPaletteAttached);
        break;
    case DDERR_NOPALETTEHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoPaletteHw);
        break;
    case DDERR_NOTAOVERLAYSURFACE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoAOverlaySurface);
        break;
    case DDERR_NOTFLIPPABLE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotFlippable);
        break;
    case DDERR_NOTLOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotLocked);
        break;
    case DDERR_NOTPALETTIZED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotPalettized);
        break;
    case DDERR_OVERLAYNOTVISIBLE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OverlayNotVisible);
        break;
    case DDERR_PRIMARYSURFACEALREADYEXISTS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_PrimarySurfaceAlreadyExists);
        break;
    case DDERR_REGIONTOOSMALL:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_RegionTooSmall);
        break;
    case DDERR_UNSUPPORTEDMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_UnsupportedMode);
        break;
    case DDERR_WRONGMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_WrongMode);
        break;
    case DDERR_XALIGN:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_XAlign);
        break;
    case DDERR_CANTPAGELOCK:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantPageLock);
        break;
    case DDERR_CANTPAGEUNLOCK:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantPageUnlock);
        break;
    case DDERR_NOTPAGELOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotPageLocked);
        break;
    case D3DERR_BADMINORVERSION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_BadMinorVersion);
        break;
    case D3DERR_BADMAJORVERSION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_BadMajorVersion);
        break;
    case D3DERR_INVALID_DEVICE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InvalidDevice);
        break;
    case D3DERR_EXECUTE_CLIPPED_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteClippedFailed);
        break;
    case D3DERR_EXECUTE_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteCreateFailed);
        break;
    case D3DERR_EXECUTE_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteDestroyFailed);
        break;
    case D3DERR_EXECUTE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteFailed);
        break;
    case D3DERR_EXECUTE_LOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteLockFailed);
        break;
    case D3DERR_EXECUTE_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteLocked);
        break;
    case D3DERR_EXECUTE_NOT_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteNotLocked);
        break;
    case D3DERR_EXECUTE_UNLOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteUnlockFailed);
        break;
    case D3DERR_INVALIDCURRENTVIEWPORT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InvalidCurrentViewport);
        break;
    case D3DERR_INVALIDPRIMITIVETYPE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InvalidPrimitiveType);
        break;
    case D3DERR_INVALIDVERTEXTYPE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InvalidVertexType);
        break;
    case D3DERR_MATERIAL_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialCreateFailed);
        break;
    case D3DERR_MATERIAL_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialDestroyFailed);
        break;
    case D3DERR_MATERIAL_GETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialGetDataFailed);
        break;
    case D3DERR_MATERIAL_SETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialSetDataFailed);
        break;
    case D3DERR_MATRIX_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixCreateFailed);
        break;
    case D3DERR_MATRIX_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixDestroyFailed);
        break;
    case D3DERR_MATRIX_GETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixGetDataFailed);
        break;
    case D3DERR_MATRIX_SETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixSetDataFailed);
        break;
    case D3DERR_SETVIEWPORTDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SetViewportDataFailed);
        break;
    case D3DERR_TEXTURE_BADSIZE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureBadSize);
        break;
    case D3DERR_TEXTURE_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureCreateFailed);
        break;
    case D3DERR_TEXTURE_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureDestroyFailed);
        break;
    case D3DERR_TEXTURE_GETSURF_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureGetSurfFailed);
        break;
    case D3DERR_TEXTURE_LOAD_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureLoadFailed);
        break;
    case D3DERR_TEXTURE_LOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureLockFailed);
        break;
    case D3DERR_TEXTURE_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureLocked);
        break;
    case D3DERR_TEXTURE_NO_SUPPORT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureNoSupport);
        break;
    case D3DERR_TEXTURE_NOT_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureNotLocked);
        break;
    case D3DERR_TEXTURE_SWAP_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureSwapFailed);
        break;
    case D3DERR_TEXTURE_UNLOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureUnlockFailed);
        break;
    case D3DERR_ZBUFF_NEEDS_SYSTEMMEMORY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ZBuffNeedsSystemMemory);
        break;
    case D3DERR_ZBUFF_NEEDS_VIDEOMEMORY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ZBuffNeedsVideoMemory);
        break;
    case D3DERR_LIGHT_SET_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_LightSetFailed);
        break;
    case D3DERR_INBEGIN:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InBegin);
        break;
    case D3DERR_NOTINBEGIN:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_NotInBegin);
        break;
    case D3DERR_NOVIEWPORTS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_NoViewports);
        break;
    case D3DERR_SCENE_BEGIN_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneBeginFailed);
        break;
    case D3DERR_SCENE_END_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneEndFailed);
        break;
    case D3DERR_SCENE_IN_SCENE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneInScene);
        break;
    case D3DERR_SCENE_NOT_IN_SCENE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneNotInScene);
        break;
    case D3DERR_VIEWPORTDATANOTSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ViewportDataNotSet);
        break;
    case DD_OK:
        return 0;
    default:
        ZVIDEO_DD_REPORT_ERROR_NAME("Unknown Error");
        break;
    }

#undef ZVIDEO_DD_REPORT_ERROR_NAME

    if (hresult == DDERR_OUTOFVIDEOMEMORY) {
        int textureMemTotalBytes;
        int textureMemFreeBytes;
        int videoMemTotalBytes;
        int videoMemFreeBytes;

        g_zVideo_pfnQueryTextureMemoryBytes(-1, &textureMemTotalBytes, &textureMemFreeBytes);
        g_zVideo_pfnQueryDeviceVideoMemoryBytes(-1, &videoMemTotalBytes, &videoMemFreeBytes);
    }

    sprintf(reportMessageBuffer, g_zVideo_DirectDrawErrorFmt, errorNameBuffer, sourceFile, sourceLine);
    ReportOld(0x400, sourceFile, sourceLine, reportMessageBuffer);
    return -1;
}
