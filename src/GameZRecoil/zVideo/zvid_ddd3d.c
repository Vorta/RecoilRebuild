#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zVideo/zvid.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"
#include "zclass.h"

#include <math.h>
#include <malloc.h>
#include <new>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace zVideo_dd3d {

namespace {
/**
 * Original-source static helper evidence: source-faithful helper for fog color.
 * Purpose: Pack 0..255 RGB floats for address-backed callers 0x4aaa90 and
 * 0x4aab30; BN has no standalone retail function for this body.
 */
DWORD PackFogColorFrom255Floats(
    float red,
    float green,
    float blue
) {
    const DWORD redByte = (DWORD)((int)(red + 0.5f));
    const DWORD greenByte = (DWORD)((int)(green + 0.5f));
    const DWORD blueByte = (DWORD)((int)(blue + 0.5f));
    return ((redByte << 8) | greenByte) << 8 | blueByte;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered PackD3DColorFrom16 helper behavior for zVideo callers.
 */
DWORD PackD3DColorFrom16(
    unsigned int packedColor16,
    int alpha
) {
    const DWORD red = (packedColor16 & g_zVideo_PixelPack.rMask) >> g_zVideo_PixelPack.packedBase;
    const DWORD green = (packedColor16 & g_zVideo_PixelPack.gMask) >> g_zVideo_PixelPack.sumMinus8;
    const DWORD blue = (packedColor16 & g_zVideo_PixelPack.bMask) << g_zVideo_PixelPack.bShiftTo8;
    return ((((red | ((DWORD)(alpha) << 8)) << 8) | green) << 8) | blue;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered WriteFlatTlVertex helper behavior for zVideo callers.
 */
void WriteFlatTlVertex(
    D3DTLVERTEX &dst,
    const zVideo_XyzVertex &src,
    DWORD packedColor
) {
    dst.sx = src.x;
    dst.sy = src.y;
    dst.sz = src.z;
    dst.rhw = src.z;
    dst.color = packedColor;
    dst.specular = 0xff000000;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyFlatVerticesReverse helper behavior for zVideo callers.
 */
void CopyFlatVerticesReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    int vertexCount,
    DWORD packedColor
) {
    for (int i = 0; i < vertexCount; ++i) {
        WriteFlatTlVertex(
            dst[i],
            vertices[vertexCount - 1 - i],
            packedColor
        );
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyGouraudVerticesReverse helper behavior for zVideo callers.
 */
void CopyGouraudVerticesReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    const unsigned int *packedColors16,
    int vertexCount,
    int alpha
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        WriteFlatTlVertex(
            dst[i],
            vertices[sourceIndex],
            PackD3DColorFrom16(packedColors16[sourceIndex], alpha)
        );
    }
}

/**
 * Original-source static helper evidence: source-faithful helper for D3D color-attribute submitters.
 * Purpose: Pack a constant RGB color and alpha for address-backed callers 0x4ab320,
 * 0x4abb20, and 0x4ac370; BN has no standalone retail function for this body.
 */
DWORD PackColorAttrConstant(
    const zVideo_ColorRgbFloat &baseColor,
    float attr1Scale,
    DWORD alphaBits
) {
    const DWORD red = (DWORD)((int)(baseColor.r * attr1Scale + 0.5f));
    const DWORD green = (DWORD)((int)(baseColor.g * attr1Scale + 0.5f));
    const DWORD blue = (DWORD)((int)(baseColor.b * attr1Scale + 0.5f));
    return alphaBits | (((red << 8) | green) << 8) | blue;
}

/**
 * Original-source static helper evidence: source-faithful helper for D3D color-attribute submitters.
 * Purpose: Apply the pending fog color bias and normalize channel for address-backed
 * callers 0x4ab320, 0x4abb20, and 0x4ac370; BN has no standalone retail function.
 */
DWORD PackColorAttrBiased(
    const zVideo_ColorRgbFloat &baseColor,
    float attr1Scale,
    float attr0Value,
    DWORD alphaBits
) {
    float red = baseColor.r * attr1Scale + attr0Value * g_zVideo_D3DColorAttrBiasR;
    float green = baseColor.g * attr1Scale + attr0Value * g_zVideo_D3DColorAttrBiasG;
    float blue = baseColor.b * attr1Scale + attr0Value * g_zVideo_D3DColorAttrBiasB;

    const float channels[3] = {red, green, blue};
    const float selected = channels[g_zVideo_D3DColorNormalizeChannelIndex];
    if (selected > 255.0f) {
        const float scale = 255.0f / selected;
        red *= scale;
        green *= scale;
        blue *= scale;
    }

    const DWORD redByte = (DWORD)((int)(red));
    const DWORD greenByte = (DWORD)((int)(green));
    const DWORD blueByte = (DWORD)((int)(blue));
    return alphaBits | (((redByte << 8) | greenByte) << 8) | blueByte;
}

/**
 * Original-source static helper evidence: source-faithful helper for D3D color-attribute submitters.
 * Purpose: Fill reversed specular alpha values for address-backed callers 0x4ab320,
 * 0x4abb20, and 0x4ac370; BN has no standalone retail function for this body.
 */
void FillColorAttrSpecularReverse(
    const float *attr2,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        DWORD specular = 0xff000000;
        if (attr2 != 0) {
            const float source = attr2[vertexCount - 1 - i];
            specular = (DWORD)((int)(0.5f + (1.0f - source) * 255.0f)) << 24;
        }
        g_zVideo_D3DSubmitTempVertices[i].specular = specular;
    }
}

/**
 * Original-source static helper evidence: source-faithful helper for zVideo_dd3d::SubmitPolyColorAttr.
 * Purpose: Fill reversed D3D colors for caller 0x4ab320, including the optional
 * fog-color bias path; BN has no standalone retail function for this body.
 */
void FillColorAttrColorsReverse(
    const zVideo_ColorRgbFloat &baseColor,
    const float *attr0,
    float attr1Scale,
    DWORD alphaBits,
    int vertexCount
) {
    const DWORD constantColor = PackColorAttrConstant(
        baseColor,
        attr1Scale,
        alphaBits
    );
    for (int i = 0; i < vertexCount; ++i) {
        DWORD color = constantColor;
        if (attr0 != 0) {
            const float attr0Value = attr0[vertexCount - 1 - i];
            if (attr0Value > (1.0f / 255.0f)) {
                color = PackColorAttrBiased(
                    baseColor,
                    attr1Scale,
                    attr0Value,
                    alphaBits
                );
            }
        }
        g_zVideo_D3DSubmitTempVertices[i].color = color;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyPositionsReverse helper behavior for zVideo callers.
 */
void CopyPositionsReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        const zVideo_XyzVertex &src = vertices[vertexCount - 1 - i];
        dst[i].sx = src.x;
        dst[i].sy = src.y;
        dst[i].sz = src.z;
        dst[i].rhw = src.z;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered PackAlphaWhite helper behavior for zVideo callers.
 */
DWORD PackAlphaWhite(
    float alpha
) {
    return ((DWORD)((int)(alpha * 255.0f)) << 24) | 0x00ffffff;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered WriteTexturedTlVertex helper behavior for zVideo callers.
 */
void WriteTexturedTlVertex(
    D3DTLVERTEX &dst,
    const zVideo_XyzVertex &src,
    const zVideo_TexCoord &texCoord,
    DWORD color
) {
    dst.sx = src.x;
    dst.sy = src.y;
    dst.sz = src.z;
    dst.rhw = src.z;
    dst.color = color;
    dst.specular = 0xff000000;
    dst.tu = texCoord.u;
    dst.tv = texCoord.v;
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyTexturedVerticesReverse helper behavior for zVideo callers.
 */
void CopyTexturedVerticesReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    const zVideo_TexCoord *texCoords,
    int vertexCount,
    DWORD color
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        WriteTexturedTlVertex(
            dst[i],
            vertices[sourceIndex],
            texCoords[sourceIndex],
            color
        );
    }
}

/**
 * Original-source static helper evidence: source-faithful helper for polygon submitters.
 * Purpose: Pack a gray polygon color with optional high clamp for address-backed
 * callers 0x4abb20 and 0x4ac370; BN has no standalone retail function.
 */
DWORD PackGrayColor(
    float gray,
    DWORD alphaBits,
    bool clampHigh
) {
    DWORD grayByte = (DWORD)((int)(gray));
    if (clampHigh && grayByte > 0xff) {
        grayByte = 0xff;
    }
    return alphaBits | (((grayByte << 8) | grayByte) << 8) | grayByte;
}

/**
 * Original-source static helper evidence: source-faithful helper for polygon submitters.
 * Purpose: Apply the pending fog color bias and normalize channel for address-backed
 * callers 0x4abb20 and 0x4ac370; BN has no standalone retail function.
 */
DWORD PackPolygonBiasedColor(
    float grayBase,
    float attr0Value,
    DWORD alphaBits
) {
    float red = grayBase + attr0Value * g_zVideo_D3DColorAttrBiasR;
    float green = grayBase + attr0Value * g_zVideo_D3DColorAttrBiasG;
    float blue = grayBase + attr0Value * g_zVideo_D3DColorAttrBiasB;

    const float channels[3] = {red, green, blue};
    const float selected = channels[g_zVideo_D3DColorNormalizeChannelIndex];
    if (selected > 255.0f) {
        const float scale = 255.0f / selected;
        red *= scale;
        green *= scale;
        blue *= scale;
    }

    const DWORD redByte = (DWORD)((int)(red));
    const DWORD greenByte = (DWORD)((int)(green));
    const DWORD blueByte = (DWORD)((int)(blue));
    return alphaBits | (((redByte << 8) | greenByte) << 8) | blueByte;
}

/**
 * Original-source static helper evidence: source-faithful helper for zVideo_dd3d::SubmitPolygon.
 * Purpose: Fill reversed polygon colors for caller 0x4abb20, including the
 * optional fog-color bias path; BN has no standalone retail function.
 */
void FillPolygonColorsReverse(
    const float *attr0,
    float grayBase,
    DWORD alphaBits,
    int vertexCount
) {
    if (attr0 == 0) {
        const DWORD color = PackGrayColor(
            grayBase,
            alphaBits,
            false
        );
        for (int i = 0; i < vertexCount; ++i) {
            g_zVideo_D3DSubmitTempVertices[i].color = color;
        }
        return;
    }

    for (int i = 0; i < vertexCount; ++i) {
        const float attr0Value = attr0[vertexCount - 1 - i];
        DWORD color;
        if (attr0Value > (1.0f / 255.0f)) {
            color = PackPolygonBiasedColor(
                grayBase,
                attr0Value,
                alphaBits
            );
        } else {
            color = PackGrayColor(
                grayBase,
                alphaBits,
                true
            );
        }
        g_zVideo_D3DSubmitTempVertices[i].color = color;
    }
}

/**
 * Original-source static helper evidence: source-faithful helper for zVideo_dd3d::SubmitPolygonLit.
 * Purpose: Fill reversed lit polygon colors for caller 0x4ac370, including the
 * optional fog-color bias path; BN has no standalone retail function.
 */
void FillPolygonLitColorsReverse(
    const float *attr1,
    const float *attr0,
    DWORD alphaBits,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        const float grayBase = (1.0f - attr1[sourceIndex]) * 255.0f;
        DWORD color;
        if (attr0 != 0 && attr0[sourceIndex] > (1.0f / 255.0f)) {
            color = PackPolygonBiasedColor(
                grayBase,
                attr0[sourceIndex],
                alphaBits
            );
        } else {
            color = PackGrayColor(
                grayBase,
                alphaBits,
                attr0 != 0
            );
        }
        g_zVideo_D3DSubmitTempVertices[i].color = color;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyPositionUvReversePreserveColor helper behavior for zVideo callers.
 */
void CopyPositionUvReversePreserveColor(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    const zVideo_TexCoord *uvPairs,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        const zVideo_XyzVertex &src = vertices[sourceIndex];
        const zVideo_TexCoord &uv = uvPairs[sourceIndex];
        dst[i].sx = src.x;
        dst[i].sy = src.y;
        dst[i].sz = src.z;
        dst[i].rhw = src.z;
        dst[i].tu = uv.u;
        dst[i].tv = uv.v;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered CopyPositionUvWithPreparedColorReverse helper behavior for zVideo callers.
 */
void CopyPositionUvWithPreparedColorReverse(
    D3DTLVERTEX *dst,
    const zVideo_XyzVertex *vertices,
    const zVideo_TexCoord *uvPairs,
    const D3DTLVERTEX *prepared,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        const int sourceIndex = vertexCount - 1 - i;
        const zVideo_XyzVertex &src = vertices[sourceIndex];
        const zVideo_TexCoord &uv = uvPairs[sourceIndex];
        dst[i].sx = src.x;
        dst[i].sy = src.y;
        dst[i].sz = src.z;
        dst[i].rhw = src.z;
        dst[i].color = prepared[i].color;
        dst[i].specular = prepared[i].specular;
        dst[i].tu = uv.u;
        dst[i].tv = uv.v;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered AppendFanCloseVertexIfNeeded helper behavior for zVideo callers.
 */
void AppendFanCloseVertexIfNeeded(
    D3DTLVERTEX *vertices,
    int &count
) {
    if (g_zVideo_D3DAppendFanCloseVertexPending != 0) {
        vertices[count] = vertices[1];
        ++count;
        g_zVideo_D3DAppendFanCloseVertexPending = 0;
    }
}
} // namespace

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: begin the Direct3D scene and flush deferred wireframe and dither states.
 *
 * Evidence: BN calls IDirect3DDevice2::BeginScene, reports zvid_ddd3d.c line 76
 * on failure, maps pending wireframe 0/1 to solid/wireframe fill mode, resets
 * applied pending states to -1, and returns zero on success.
 */
int BeginSceneAndFlushPendingRenderStates() {
    const HRESULT hresult = g_zVideo_pD3DDevice->BeginScene();
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            76
        );
    }

    const int pendingWireframeState = g_zVideo_PendingWireframeState;
    if (pendingWireframeState == 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_FILLMODE,
            D3DFILL_SOLID
        );
        g_zVideo_PendingWireframeState = -1;
    } else if (pendingWireframeState == 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_FILLMODE,
            D3DFILL_WIREFRAME
        );
        g_zVideo_PendingWireframeState = -1;
    }

    // VC5 matches BN when the dither global is reloaded at the call site.
    if (g_zVideo_PendingDitherEnable != -1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_DITHERENABLE,
            (DWORD)(g_zVideo_PendingDitherEnable)
        );
        g_zVideo_PendingDitherEnable = -1;
    }

    return 0;
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: end the active Direct3D scene and report provider failures.
 *
 * Evidence: BN calls IDirect3DDevice2::EndScene, reports zvid_ddd3d.c line 115
 * on nonzero HRESULT, and returns zero on success.
 */
int EndScene() {
    const HRESULT hresult = g_zVideo_pD3DDevice->EndScene();
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            115
        );
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
int __fastcall PresentDisplayModeSurface(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
) {
    // BN keeps the checked display surface live, then reloads it after Blt/retry paths.
    IDirectDrawSurface3 *displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
    if (displaySurface == 0) {
        return 0x400;
    }

    for (;;) {
        if (blitPrimaryToSwFirst != 0) {
            const DWORD bltFlags = waitForPresent != 0 ? DDBLT_WAIT : 0;
            g_zVideo_SwSurfaceState.surf->Blt(
                (RECT *)(dstRect),
                g_zVideo_PrimarySurfaceState.surf,
                (RECT *)(srcRect),
                bltFlags,
                0
            );
            displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
        }

        HRESULT hresult = displaySurface->Flip(
            0,
            waitForPresent != 0 ? DDFLIP_WAIT : 0
        );
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_WASSTILLDRAWING) {
            displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
            continue;
        }

        if (hresult == DDERR_SURFACELOST) {
            displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
            hresult = displaySurface->Restore();
        }

        if (hresult == DD_OK) {
            displaySurface = g_zVideo_DisplayModeSurfaceState.surf;
            continue;
        }

        zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xae
        );
        return 0x5a56ffff;
    }
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: creates the Direct3D z-buffer/device/viewport/material state and
 * initializes the fixed render-state defaults for the active software surface.
 *
 * Evidence: BN shows z-buffer surface creation, DirectDraw/Direct3D provider
 * setup, material and caps initialization, ten
 * fixed render-state writes, fog enablement, and quad-batch depth seeding.
 */
int CreateDeviceState() {
    DDSURFACEDESC zBufferDesc = {0};
    D3DVIEWPORT2 viewport2 = {0};
    D3DMATERIAL mat = {0};
    // VC5/BN evidence shows the original C source zeroed this provider record again here.
    memset(
        &zBufferDesc,
        0,
        sizeof(zBufferDesc)
    );
    zBufferDesc.dwWidth = (DWORD)(g_zVideo_SwSurfaceState.width);
    zBufferDesc.dwHeight = (DWORD)(g_zVideo_SwSurfaceState.height);
    g_zVideo_ClearScreenBufferEnabled = 1;
    zBufferDesc.dwSize = sizeof(zBufferDesc);
    zBufferDesc.dwFlags = 0x47;
    zBufferDesc.ddsCaps.dwCaps = 0x24000;
    zBufferDesc.dwMipMapCount = 0x10;

    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(
        &zBufferDesc,
        (IDirectDrawSurface **)(&g_zVideo_pZBufferSurface),
        0
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xd3
        );
    }

    hresult = g_zVideo_pZBufferSurface->QueryInterface(
        IID_IDirectDrawSurface,
        (void **)(&g_zVideo_pZBufferAttachSurface)
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xd9
        );
    }

    hresult = g_zVideo_SwSurfaceState.surf->AddAttachedSurface(
        (IDirectDrawSurface3 *)(g_zVideo_pZBufferAttachSurface)
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xde
        );
    }

    hresult = g_zVideo_pDirectDraw2->QueryInterface(
        IID_IDirect3D2,
        (void **)(&g_zVideo_pD3D2)
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xe5
        );
    }

    hresult = g_zVideo_pD3D2->CreateDevice(
        *g_zVideo_pSelectedD3DDeviceInfo->pD3DDeviceGuid,
        (IDirectDrawSurface *)(g_zVideo_SwSurfaceState.surf),
        &g_zVideo_pD3DDevice
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xed
        );
    }

    hresult = g_zVideo_pD3D2->CreateViewport(
        &g_zVideo_pD3DViewport2,
        0
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xf4
        );
    }

    hresult = g_zVideo_pD3DDevice->AddViewport(g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xf9
        );
    }

    const DWORD width = (DWORD)(g_zVideo_DisplayModeSurfaceState.width);
    const DWORD height = (DWORD)(g_zVideo_DisplayModeSurfaceState.height);
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

    hresult = g_zVideo_pD3DViewport2->SetViewport2(&viewport2);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x10a
        );
    }

    hresult = g_zVideo_pD3DDevice->SetCurrentViewport(g_zVideo_pD3DViewport2);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x10f
        );
    }

    hresult = g_zVideo_pD3D2->CreateMaterial(
        &g_zVideo_pD3DMaterial2,
        0
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x116
        );
    }

    mat.dwSize = sizeof(mat);
    mat.diffuse.b = 0.0f;
    mat.diffuse.g = 0.0f;
    mat.diffuse.r = 0.0f;
    mat.ambient.b = 1.0f;
    mat.ambient.g = 1.0f;
    mat.ambient.r = 1.0f;
    mat.dwRampSize = 0x100;

    hresult = g_zVideo_pD3DMaterial2->SetMaterial(&mat);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x124
        );
    }

    hresult = g_zVideo_pD3DMaterial2->GetHandle(
        g_zVideo_pD3DDevice,
        &g_zVideo_D3DMaterialHandle
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x12a
        );
    }

    hresult = g_zVideo_pD3DViewport2->SetBackground(g_zVideo_D3DMaterialHandle);
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x12f
        );
    }

    g_zVideo_D3DHelDeviceDesc.dwSize = sizeof(g_zVideo_D3DHelDeviceDesc);
    g_zVideo_D3DHalDeviceDesc.dwSize = sizeof(g_zVideo_D3DHalDeviceDesc);
    hresult = g_zVideo_pD3DDevice->GetCaps(
        &g_zVideo_D3DHalDeviceDesc,
        &g_zVideo_D3DHelDeviceDesc
    );
    if (hresult != DD_OK) {
        return zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x139
        );
    }

    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_CULLMODE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZENABLE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        7
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_SPECULARENABLE,
        0
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_SHADEMODE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_TEXTUREPERSPECTIVE,
        1
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_TEXTUREMAG,
        2
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_TEXTUREMIN,
        2
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_SRCBLEND,
        5
    );
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_DESTBLEND,
        6
    );

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
 * TexturePixelPack_SetupFromMasks and UploadImageToSurface, performs
 * DirectDraw/Direct3D provider QueryInterface/Load/GetHandle calls, fills the
 * zVideo_TextureRecordPartial fields, and releases temporary provider objects
 * on failure.
 */
zVideo_TextureRecordPartial *__fastcall CreateTextureRecord(
    const char *textureName,
    zVidImagePartial *image,
    int useAlpha,
    int clampU,
    int clampV
) {
    IDirectDrawSurface *uploadSurface = 0;
    IDirectDrawSurface *textureSurface = 0;
    IDirect3DTexture2 *uploadTexture = 0;
    IDirect3DTexture2 *texture = 0;
    IDirectDrawPalette *ddPalette = 0;

    const D3DDEVICEDESC *selectedDeviceDesc =
        &g_zVideo_pSelectedD3DDeviceInfo->m_hwDesc;
    int width = image->width;
    int height = image->height;
    if ((DWORD)(width) > selectedDeviceDesc->dwMaxTextureWidth ||
        (DWORD)(height) > selectedDeviceDesc->dwMaxTextureHeight) {
        zError::ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x20e,
            g_zVideo_TextureTooLargeUsingDefaultFmt,
            textureName,
            width,
            height
        );
        return g_zVideo_DefaultTextureRecord;
    }

    if ((g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_POW2) != 0 &&
        (FloorPowerOfTwo(width) != width || FloorPowerOfTwo(height) != height)) {
        zError::ReportOld(
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

    if (width > height * 8 || height > width * 8) {
        zError::ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x233,
            g_zVideo_TextureBadAspectUsingDefaultFmt,
            textureName,
            width,
            height
        );
        return g_zVideo_DefaultTextureRecord;
    }

    if ((g_zVideo_D3DHalDeviceDesc.dpcTriCaps.dwTextureCaps & D3DPTEXTURECAPS_SQUAREONLY) != 0 &&
        image->width != image->height) {
        const int squareSide = FloorPowerOfTwo((int)(sqrt((double)(height * width))));
        zVid_Image::ResampleSquare(
            image,
            squareSide
        );
        width = image->width;
        height = image->height;
    }

    if (image->palette != 0) {
        zError::ReportOld(
            0x200,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x24a,
            g_zVideo_TexturePaletteUnsupportedUsingDefaultFmt,
            textureName
        );
        return g_zVideo_DefaultTextureRecord;
    }

    DDSURFACEDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwHeight = (DWORD)(height);
    desc.dwWidth = (DWORD)(width);
    desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    desc.ddpfPixelFormat.dwSize = sizeof(desc.ddpfPixelFormat);
    desc.ddpfPixelFormat.dwFlags = DDPF_RGB;
    desc.ddpfPixelFormat.dwRGBBitCount = 16;

    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    DWORD redMask;
    DWORD greenMask;
    DWORD blueMask;
    DWORD alphaMask;
    if (useAlpha == 0) {
        redBits = g_zVideo_PixelPack.rBits;
        greenBits = g_zVideo_PixelPack.gBits;
        blueBits = g_zVideo_PixelPack.bBits;
        alphaBits = 0;
        redMask = g_zVideo_PixelPack.rMask;
        greenMask = g_zVideo_PixelPack.gMask;
        blueMask = g_zVideo_PixelPack.bMask;
        alphaMask = 0;
    } else {
        desc.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
        if (image->alphaMap != 0) {
            redBits = 4;
            greenBits = 4;
            blueBits = 4;
            alphaBits = 4;
            redMask = 0x0f00;
            greenMask = 0x00f0;
            blueMask = 0x000f;
            alphaMask = 0xf000;
        } else {
            redBits = 5;
            greenBits = 5;
            blueBits = 5;
            alphaBits = 1;
            redMask = 0x7c00;
            greenMask = 0x03e0;
            blueMask = 0x001f;
            alphaMask = 0x8000;
        }
    }

    desc.ddpfPixelFormat.dwRBitMask = redMask;
    desc.ddpfPixelFormat.dwGBitMask = greenMask;
    desc.ddpfPixelFormat.dwBBitMask = blueMask;
    desc.ddpfPixelFormat.dwRGBAlphaBitMask = alphaMask;
    zVideo::TexturePixelPack_SetupFromMasks(
        redBits,
        greenBits,
        blueBits,
        alphaBits,
        redMask,
        greenMask,
        blueMask,
        alphaMask
    );

    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(
        &desc,
        &uploadSurface,
        0
    );
    zVideo_TextureRecordPartial *result = 0;
    if (hresult == DD_OK && image->palette != 0) {
        PALETTEENTRY paletteEntries[256];
        memset(
            paletteEntries,
            0,
            sizeof(paletteEntries)
        );
        memcpy(
            paletteEntries,
            image->palette,
            image->paletteMetaPacked
        );
        hresult = g_zVideo_pDirectDraw2->CreatePalette(
            DDPCAPS_8BIT | DDPCAPS_ALLOW256,
            (LPPALETTEENTRY)(image->palette),
            &ddPalette,
            0
        );
        if (hresult == DD_OK) {
            hresult = uploadSurface->SetPalette(ddPalette);
        }
    }
    if (hresult == DD_OK) {
        UploadImageToSurface(
            uploadSurface,
            image,
            useAlpha
        );
        hresult = uploadSurface->QueryInterface(
            IID_IDirect3DTexture2,
            (void **)(&uploadTexture)
        );
    }

    D3DTEXTUREHANDLE textureHandle = 0;
    if (hresult == DD_OK) {
        desc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
        if ((g_zVideo_D3DHalDeviceDesc.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM) != 0) {
            desc.ddsCaps.dwCaps |= DDSCAPS_NONLOCALVIDMEM;
        }

        hresult = g_zVideo_pDirectDraw2->CreateSurface(
            &desc,
            &textureSurface,
            0
        );
    }
    if (hresult == DD_OK && ddPalette != 0) {
        hresult = textureSurface->SetPalette(ddPalette);
    }
    if (hresult == DD_OK) {
        hresult = textureSurface->QueryInterface(
            IID_IDirect3DTexture2,
            (void **)(&texture)
        );
    }
    if (hresult == DD_OK) {
        hresult = texture->Load(uploadTexture);
    }
    if (hresult == DD_OK) {
        hresult = texture->GetHandle(
            g_zVideo_pD3DDevice,
            &textureHandle
        );
    }
    if (hresult == DD_OK) {
        result = TextureRecord_Create();
        if (result != 0) {
            result->m_uploadSurface = uploadSurface;
            result->m_textureSurface = textureSurface;
            result->m_texture = texture;
            result->m_textureHandle = textureHandle;
            result->m_alphaMode = useAlpha == 0 ? 1 : (image->alphaMap != 0 ? 4 : 5);
            result->m_uWrapMode = clampU != 0 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
            result->m_vWrapMode = clampV != 0 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP;
        }
        uploadTexture->Release();
    }

    if (hresult != DD_OK) {
        zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x30f
        );
        if (texture != 0) {
            texture->Release();
        }
        if (uploadTexture != 0) {
            uploadTexture->Release();
        }
        if (textureSurface != 0) {
            textureSurface->Release();
        }
        if (uploadSurface != 0) {
            uploadSurface->Release();
        }
    }

    return result != 0 ? result : g_zVideo_DefaultTextureRecord;
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: locks a DirectDraw upload surface, copies or converts image pixels
 * into its pitch layout, and unlocks the surface after upload.
 *
 * Evidence: BN assembly locks through zVideo_dd::LockSurface_WaitRestore,
 * chooses ConvertImagePixelsForTexture only when useAlpha is nonzero, otherwise
 * copies either one contiguous block or one row per pitch, then unlocks through
 * zVideo_dd::UnlockSurface_WaitRestore and returns one unconditionally.
 */
int __fastcall UploadImageToSurface(
    IDirectDrawSurface *uploadSurface,
    zVidImagePartial *image,
    int useAlpha
) {
    DDSURFACEDESC lockedDescOut = {0};
    zVideo_dd::LockSurface_WaitRestore(
        (IDirectDrawSurface3 *)(uploadSurface),
        &lockedDescOut
    );

    unsigned char *dstPixels = (unsigned char *)(lockedDescOut.lpSurface);
    unsigned char *srcPixels = (unsigned char *)(image->pixels);
    if (useAlpha != 0) {
        ConvertImagePixelsForTexture(
            (unsigned short *)(dstPixels),
            image,
            lockedDescOut.lPitch,
            useAlpha
        );
    } else {
        const int width = image->width;
        const int height = image->height;
        if (lockedDescOut.lPitch == width) {
            const int bytesPerPixel = (g_zVideo_DisplayModeBpp + 7) >> 3;
            memcpy(
                dstPixels,
                srcPixels,
                (size_t)(height * bytesPerPixel * width)
            );
        } else {
            const int rowCopyBytes = (g_zVideo_DisplayModeBpp * width + 7) >> 3;
            {
                for (int row = 0; row < height; ++row) {
                    memcpy(
                        dstPixels,
                        srcPixels,
                        (size_t)(rowCopyBytes)
                    );
                    dstPixels += lockedDescOut.lPitch;
                    srcPixels += width << 1;
                }
            }
        }
    }

    zVideo_dd::UnlockSurface_WaitRestore((IDirectDrawSurface3 *)(uploadSurface));
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
void __fastcall ConvertImagePixelsForTexture(
    unsigned short *dstPixels,
    zVidImagePartial *image,
    int pitchBytes,
    int useAlpha
) {
    (void)useAlpha;

    char *alphaMap = image->alphaMap;
    unsigned short *srcPixels = (unsigned short *)(image->pixels);
    unsigned char *dstRowBytes = (unsigned char *)(dstPixels);

    if (alphaMap == 0) {
        const unsigned int redGreenMask = g_zVideo_PixelPack.rMask | g_zVideo_PixelPack.gMask;
        {
            for (int row = 0; row < image->height; ++row) {
                unsigned short *dstCursor = (unsigned short *)(dstRowBytes);
                {
                    for (int column = 0; column < image->width; ++column) {
                        const unsigned short src = *srcPixels++;
                        const unsigned short alphaBit = src != 0 ? 0x8000 : 0;
                        *dstCursor++ =
                            (unsigned short)((src & g_zVideo_PixelPack.bMask) |
                                             ((src >> 1) & (redGreenMask >> 1)) | alphaBit);
                    }
                }
                dstRowBytes += pitchBytes;
            }
        }
        return;
    }

    unsigned char *alphaCursor = (unsigned char *)(alphaMap);
    unsigned int redAlphaMask;
    int redAlphaShift;
    unsigned int greenAlphaMask;
    int greenAlphaShift;
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
        for (int row = 0; row < image->height; ++row) {
            unsigned short *dstCursor = (unsigned short *)(dstRowBytes);
            {
                for (int column = 0; column < image->width; ++column) {
                    const unsigned short src = *srcPixels++;
                    const unsigned int alpha = (*alphaCursor++ & 0xf0) << 8;
                    *dstCursor++ =
                        (unsigned short)(((src >> 1) & (g_zVideo_PixelPack.bMask >> 1)) |
                                         ((greenAlphaMask & src) >> greenAlphaShift) |
                                         ((redAlphaMask & src) >> redAlphaShift) | alpha);
                }
            }
            dstRowBytes += pitchBytes;
        }
    }
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: locks a texture record's upload surface and returns the provider
 * pixel pointer and row pitch to the caller.
 *
 * Evidence: BN loads m_uploadSurface at offset zero, calls
 * zVideo_dd::LockSurface_WaitRestore with an uninitialized stack
 * DDSURFACEDESC, copies lpSurface and lPitch to the output pointers only on
 * success, and returns one or zero. The callee owns descriptor clearing and
 * dwSize initialization.
 */
int __fastcall TextureRecord_LockUploadSurface(
    zVideo_TextureRecordPartial *textureRecord,
    void **outPixels,
    int *outPitchBytes
) {
    DDSURFACEDESC lockedDescOut;
    if (zVideo_dd::LockSurface_WaitRestore(
            (IDirectDrawSurface3 *)(textureRecord->m_uploadSurface),
            &lockedDescOut
        ) == 0) {
        *outPitchBytes = lockedDescOut.lPitch;
        *outPixels = lockedDescOut.lpSurface;
        return 1;
    }

    return 0;
}

/**
 * Retail literal-backed physical source block: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: unlocks a texture record's upload surface and normalizes provider
 * success to a one-or-zero result.
 *
 * Evidence: BN loads m_uploadSurface at offset zero, calls
 * zVideo_dd::UnlockSurface_WaitRestore, and uses neg/sbb/inc to return one
 * only when the unlock wrapper returns zero.
 */
int __fastcall TextureRecord_UnlockUploadSurface(
    zVideo_TextureRecordPartial *textureRecord
) {
    return zVideo_dd::UnlockSurface_WaitRestore(
               (IDirectDrawSurface3 *)(textureRecord->m_uploadSurface)
           ) == 0
               ? 1
               : 0;
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: releases and clears the upload-surface reference when one is held by
 * a texture record.
 *
 * Evidence: BN tests m_uploadSurface at offset zero, calls the provider Release
 * slot at vtable offset 8 when non-null, and stores null back to offset zero.
 */
void __fastcall TextureRecord_ReleaseUploadSurfaceRef(
    zVideo_TextureRecordPartial *textureRecord
) {
    if (textureRecord->m_uploadSurface != 0) {
        textureRecord->m_uploadSurface->Release();
        textureRecord->m_uploadSurface = 0;
    }
}

/**
 * Retail literal-backed physical source block: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: optionally refreshes a texture-record upload surface from an image
 * and loads the temporary upload texture into the target Direct3D texture.
 *
 * Evidence: BN exits when m_uploadSurface is null, optionally calls
 * UploadImageToSurface with image->formatFlagsPacked bit 1, queries the upload
 * surface for IDirect3DTexture2, calls targetTexture->Load(uploadTexture), and
 * releases the temporary upload texture only when Load succeeds.
 */
void __fastcall TextureRecord_FinalizeUpload(
    zVideo_TextureRecordPartial *textureRecord,
    void *,
    zVidImagePartial *image
) {
    IDirectDrawSurface *uploadSurface = textureRecord->m_uploadSurface;
    if (uploadSurface == 0) {
        return;
    }

    IDirect3DTexture2 *targetTexture = textureRecord->m_texture;
    if (image != 0) {
        UploadImageToSurface(
            uploadSurface,
            image,
            image->formatFlagsPacked & 2
        );
    }

    IDirect3DTexture2 *uploadTexture = 0;
    HRESULT hresult =
        uploadSurface->QueryInterface(
            IID_IDirect3DTexture2,
            (void **)(&uploadTexture)
        );
    if (hresult != DD_OK) {
        return;
    }

    hresult = targetTexture->Load(uploadTexture);
    if (hresult == DD_OK) {
        uploadTexture->Release();
    }
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: release non-default Direct3D texture-record provider resources and
 * free the texture record.
 *
 * Evidence: BN hoists the upload surface, texture surface, and texture fields
 * before checking the default texture record, then releases each provider
 * reference in field order before freeing the record.
 */
void __fastcall TextureRecord_Destroy(
    zVideo_TextureRecordPartial *textureRecord
) {
    IDirectDrawSurface *uploadSurface = textureRecord->m_uploadSurface;
    IDirectDrawSurface *textureSurface = textureRecord->m_textureSurface;
    IDirect3DTexture2 *texture = textureRecord->m_texture;

    if (textureRecord == g_zVideo_DefaultTextureRecord) {
        return;
    }

    if (uploadSurface != 0) {
        uploadSurface->Release();
    }
    if (textureSurface != 0) {
        textureSurface->Release();
    }
    if (texture != 0) {
        texture->Release();
    }

    free(textureRecord);
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: allocates a zeroed Direct3D texture-record structure.
 *
 * Evidence: BN assembly is a leaf that calls calloc(1, 0x1c) and returns the
 * provider result directly; zVideo_TextureRecordPartial is asserted to 0x1c.
 */
zVideo_TextureRecordPartial *TextureRecord_Create() {
    return (zVideo_TextureRecordPartial *)(calloc(
        1,
        sizeof(zVideo_TextureRecordPartial)
    ));
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: update the cached Direct3D fog-enable render state and force the
 * fixed fog light-state mode.
 *
 * Evidence: BN compares g_zVideo_CachedFogEnableRenderState before calling
 * IDirect3DDevice2::SetRenderState(D3DRENDERSTATE_FOGENABLE), stores the new
 * enable value, then ensures D3DLIGHTSTATE_FOGMODE is D3DFOG_LINEAR through
 * IDirect3DDevice2::SetLightState.
 */
void __fastcall SetFogEnable(
    int enable
) {
    if (g_zVideo_CachedFogEnableRenderState != enable) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_FOGENABLE,
            (DWORD)(enable)
        );
        g_zVideo_CachedFogEnableRenderState = enable;
    }

    if (g_zVideo_CachedFogModeLightState != 3) {
        g_zVideo_pD3DDevice->SetLightState(
            D3DLIGHTSTATE_FOGMODE,
            D3DFOG_LINEAR
        );
        g_zVideo_CachedFogModeLightState = 3;
    }
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: update the cached Direct3D fog-start light state only when the
 * requested start distance changes.
 *
 * Evidence: BN shows a stdcall float argument, x87 comparison against
 * g_zVideo_CachedFogStartLightStateValue, then a Direct3D provider
 * SetLightState call with selector D3DLIGHTSTATE_FOGSTART and the raw
 * fogStart float bits before updating the cache.
 */
void __stdcall SetFogStart(
    float fogStart
) {
    if (g_zVideo_CachedFogStartLightStateValue != fogStart) {
        g_zVideo_pD3DDevice->SetLightState(
            (D3DLIGHTSTATETYPE)(5),
            *(DWORD *)(&fogStart)
        );
        g_zVideo_CachedFogStartLightStateValue = fogStart;
    }
}

/**
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
void __stdcall SetFogEnd(
    float fogEnd
) {
    if (g_zVideo_CachedFogEndLightStateValue != fogEnd) {
        g_zVideo_pD3DDevice->SetLightState(
            (D3DLIGHTSTATETYPE)(5),
            *(DWORD *)(&fogEnd)
        );
        g_zVideo_CachedFogEndLightStateValue = fogEnd;
    }
}

/**
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
void __stdcall ApplyFogStateFromGlobals(
    float fogStart,
    float fogEnd,
    float unused
) {
    (void)unused;
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_FOGENABLE,
        1
    );

    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_FOGCOLOR,
        PackFogColorFrom255Floats(
            g_zVideo_FogColorPendingR255,
            g_zVideo_FogColorPendingG255,
            g_zVideo_FogColorPendingB255
        )
    );

    g_zVideo_pD3DDevice->SetLightState(
        D3DLIGHTSTATE_FOGMODE,
        D3DFOG_LINEAR
    );
    g_zVideo_pD3DDevice->SetLightState(
        (D3DLIGHTSTATETYPE)(5),
        *(DWORD *)(&fogStart)
    );
    g_zVideo_pD3DDevice->SetLightState(
        (D3DLIGHTSTATETYPE)(6),
        *(DWORD *)(&fogEnd)
    );
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: upload the applied global fog RGB floats as a packed Direct3D
 * fog-color render state.
 *
 * Evidence: BN shows a cdecl no-argument function that packs
 * g_zVideo_FogColorAppliedR255/G255/B255 with the same +0.5f, _ftol,
 * 0xRRGGBB sequence used by 0x4aaa90, then calls the Direct3D provider
 * SetRenderState for D3DRENDERSTATE_FOGCOLOR.
 */
void UpdateFogColor() {
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_FOGCOLOR,
        PackFogColorFrom255Floats(
            g_zVideo_FogColorAppliedR255,
            g_zVideo_FogColorAppliedG255,
            g_zVideo_FogColorAppliedB255
        )
    );
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Convert flat 16-bit color polygons to Direct3D TL vertices and
 * submit them through the immediate, overwrite, or sorted transparent path.
 */
void __fastcall SubmitPolyFlatColor16(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
) {
    const DWORD packedColor = PackD3DColorFrom16(
        packedColor16,
        alpha
    );

    if (alpha >= 0xff) {
        CopyFlatVerticesReverse(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            vertexCount,
            packedColor
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x503,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedsFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 1;
            entry.vertexCount = vertexCount;
            entry.renderClass = 0;
            entry.renderParam = renderParam;
            if (vertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(vertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                0
            );
            g_zVideo_D3DRenderStateCache.textureHandle = 0;
        }
        if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                1
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 1;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x520
            );
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x528,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.vertexCount = vertexCount;
        entry.renderClass = 0;
        entry.renderParam = renderParam;
        if (vertexCount > 0) {
            CopyFlatVerticesReverse(
                entry.vertices,
                vertices,
                vertexCount,
                packedColor
            );
        }
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x547,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.vertexCount = vertexCount;
    entry.renderClass = 0;
    entry.renderParam = renderParam;
    if (vertexCount > 0) {
        CopyFlatVerticesReverse(
            entry.vertices,
            vertices,
            vertexCount,
            packedColor
        );
    }
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Convert per-vertex 16-bit color polygons to Direct3D TL vertices
 * and submit them through the immediate, overwrite, or sorted transparent path.
 */
void __fastcall SubmitPolyGouraudColor16(
    zVideo_XyzVertex *vertices,
    unsigned int *packedColors16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
) {
    if (alpha >= 0xff) {
        CopyGouraudVerticesReverse(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            packedColors16,
            vertexCount,
            alpha
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x59d,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 2;
            entry.vertexCount = vertexCount;
            entry.renderClass = 0;
            entry.renderParam = renderParam;
            if (vertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(vertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                0
            );
            g_zVideo_D3DRenderStateCache.textureHandle = 0;
        }
        if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                1
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 1;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x5bb
            );
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x5c3,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.vertexCount = vertexCount;
        entry.renderClass = 0;
        entry.renderParam = renderParam;
        if (vertexCount > 0) {
            CopyGouraudVerticesReverse(
                entry.vertices,
                vertices,
                packedColors16,
                vertexCount,
                alpha
            );
        }
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x5e2,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.vertexCount = vertexCount;
    entry.renderClass = 0;
    entry.renderParam = renderParam;
    if (vertexCount > 0) {
        CopyGouraudVerticesReverse(
            entry.vertices,
            vertices,
            packedColors16,
            vertexCount,
            alpha
        );
    }
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Purpose: Build color-attribute TL vertices and either draw immediately or queue
 * the overwrite polygon path.
 */
void __fastcall SubmitPolyColorAttr(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    zVideo_ColorRgbFloat *baseColor,
    float *attr1,
    float *attr0,
    float *attr2,
    int alpha,
    int vertexCount,
    unsigned int renderParam,
    int queueMode
) {
    (void)packedColor16;

    const float attr1Scale = 1.0f - *attr1;
    const DWORD alphaBits = alpha < 0xff ? (DWORD)(alpha << 24) : 0xff000000;

    FillColorAttrSpecularReverse(
        attr2,
        vertexCount
    );
    FillColorAttrColorsReverse(
        *baseColor,
        attr0,
        attr1Scale,
        alphaBits,
        vertexCount
    );
    if (alpha < 0xff) {
        return;
    }

    CopyPositionsReverse(
        g_zVideo_D3DSubmitTempVertices,
        vertices,
        vertexCount
    );

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x69c,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 3;
        entry.vertexCount = vertexCount;
        entry.renderClass = 0;
        entry.renderParam = (int)(renderParam);
        if (vertexCount > 0) {
            memcpy(
                entry.vertices,
                g_zVideo_D3DSubmitTempVertices,
                (size_t)(vertexCount) * sizeof(D3DTLVERTEX)
            );
        }
        return;
    }

    if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_TEXTUREHANDLE,
            0
        );
        g_zVideo_D3DRenderStateCache.textureHandle = 0;
    }
    if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_SHADEMODE,
            1
        );
        g_zVideo_D3DRenderStateCache.shadeMode = 1;
    }

    const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
        (D3DPRIMITIVETYPE)(6),
        (D3DVERTEXTYPE)(3),
        g_zVideo_D3DSubmitTempVertices,
        (DWORD)(vertexCount),
        0
    );
    if (hresult != DD_OK) {
        zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x6ba
        );
    }
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Prepare textured TL vertices for a render class and route them to
 * immediate Direct3D drawing or the overwrite/sorted polygon queues.
 */
void __fastcall SubmitPolyRenderClass(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *texCoords,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
) {
    if (renderClass == 0) {
        renderClass = (zVideo_RenderClass *)(g_zVideo_DefaultTextureRecord);
        if (renderClass == 0) {
            return;
        }
    }
    const bool opaquePath = renderClass->textureMapBlend != (D3DTEXTUREBLEND)(4) && alpha >= 1.0f;

    if (opaquePath) {
        CopyTexturedVerticesReverse(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            texCoords,
            vertexCount,
            g_zVideo_OpaqueWhiteArgb
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x6fd,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 4;
            entry.vertexCount = vertexCount;
            entry.renderClass = (int)(renderClass);
            entry.renderParam = (int)(renderParam);
            if (vertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(vertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                1
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 1;
        }
        if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                renderClass->textureHandle
            );
            g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderStateCache.textureMapBlend != renderClass->textureMapBlend) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREMAPBLEND,
                renderClass->textureMapBlend
            );
            g_zVideo_D3DRenderStateCache.textureMapBlend = renderClass->textureMapBlend;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSU,
                renderClass->textureAddressU
            );
            g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSV,
                renderClass->textureAddressV
            );
            g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(vertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x71d
            );
        }
        return;
    }

    const DWORD alphaWhite = PackAlphaWhite(alpha);
    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x725,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.vertexCount = vertexCount;
        entry.renderClass = (int)(renderClass);
        entry.renderParam = (int)(renderParam);
        if (vertexCount > 0) {
            CopyTexturedVerticesReverse(
                entry.vertices,
                vertices,
                texCoords,
                vertexCount,
                alphaWhite
            );
        }
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x74c,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.vertexCount = vertexCount;
    entry.renderClass = (int)(renderClass);
    entry.renderParam = (int)(renderParam);
    if (vertexCount > 0) {
        CopyTexturedVerticesReverse(
            entry.vertices,
            vertices,
            texCoords,
            vertexCount,
            alphaWhite
        );
    }
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Purpose: Build textured polygon TL vertices with fog color-attribute bias and
 * route them to immediate, overwrite, or sorted transparent submission.
 */
void __fastcall SubmitPolygon(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *uvPairs,
    float *attr1,
    float *attr0,
    float *attr2,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
) {
    const float attr1Scale = 1.0f - *attr1;
    const DWORD alphaBits = alpha < 1.0f ? ((DWORD)((int)(alpha * 255.0f)) << 24) : 0xff000000;
    const float grayBase = attr1Scale * 255.0f;

    FillColorAttrSpecularReverse(
        attr2,
        vertexCount
    );
    FillPolygonColorsReverse(
        attr0,
        grayBase,
        alphaBits,
        vertexCount
    );

    const bool opaquePath = renderClass->textureMapBlend != (D3DTEXTUREBLEND)(4) && alpha >= 1.0f;

    if (opaquePath) {
        int preparedVertexCount = vertexCount;
        CopyPositionUvReversePreserveColor(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            uvPairs,
            preparedVertexCount
        );
        AppendFanCloseVertexIfNeeded(
            g_zVideo_D3DSubmitTempVertices,
            preparedVertexCount
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x82a,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 5;
            entry.vertexCount = preparedVertexCount;
            entry.renderClass = (int)(renderClass);
            entry.renderParam = (int)(renderParam);
            if (preparedVertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(preparedVertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                2
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 2;
        }
        if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                renderClass->textureHandle
            );
            g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(2)) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREMAPBLEND,
                2
            );
            g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSU,
                renderClass->textureAddressU
            );
            g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSV,
                renderClass->textureAddressV
            );
            g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(preparedVertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x84a
            );
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x853,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.renderClass = (int)(renderClass);
        entry.renderParam = (int)(renderParam);
        int preparedVertexCount = vertexCount;
        if (vertexCount > 0) {
            CopyPositionUvWithPreparedColorReverse(
                entry.vertices,
                vertices,
                uvPairs,
                g_zVideo_D3DSubmitTempVertices,
                vertexCount
            );
        }
        AppendFanCloseVertexIfNeeded(
            entry.vertices,
            preparedVertexCount
        );
        entry.vertexCount = preparedVertexCount;
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x88a,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.renderClass = (int)(renderClass);
    entry.renderParam = (int)(renderParam);
    int preparedVertexCount = vertexCount;
    if (vertexCount > 0) {
        CopyPositionUvWithPreparedColorReverse(
            entry.vertices,
            vertices,
            uvPairs,
            g_zVideo_D3DSubmitTempVertices,
            vertexCount
        );
    }
    AppendFanCloseVertexIfNeeded(
        entry.vertices,
        preparedVertexCount
    );
    entry.vertexCount = preparedVertexCount;
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Purpose: Build lit textured polygon TL vertices with fog color-attribute bias
 * and route them to immediate, overwrite, or sorted transparent submission.
 */
void __fastcall SubmitPolygonLit(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *uvPairs,
    float *attr1,
    float *attr0,
    float *attr2,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
) {
    const DWORD alphaBits = alpha < 1.0f ? ((DWORD)((int)(alpha * 255.0f)) << 24) : 0xff000000;

    FillColorAttrSpecularReverse(
        attr2,
        vertexCount
    );
    FillPolygonLitColorsReverse(
        attr1,
        attr0,
        alphaBits,
        vertexCount
    );

    const bool opaquePath = renderClass->textureMapBlend != (D3DTEXTUREBLEND)(4) && alpha >= 1.0f;

    if (opaquePath) {
        int preparedVertexCount = vertexCount;
        CopyPositionUvReversePreserveColor(
            g_zVideo_D3DSubmitTempVertices,
            vertices,
            uvPairs,
            preparedVertexCount
        );
        AppendFanCloseVertexIfNeeded(
            g_zVideo_D3DSubmitTempVertices,
            preparedVertexCount
        );

        if (queueMode != 0) {
            const int queueIndex = g_zVideo_OverwriteQueueCount;
            if (queueIndex >= 0x180) {
                zError::ReportOld(
                    0x400,
                    g_zVideo_SourceFile_ZvidDdd3dC,
                    0x983,
                    g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                    queueIndex
                );
                return;
            }

            zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
            ++g_zVideo_OverwriteQueueCount;
            entry.type = 6;
            entry.vertexCount = preparedVertexCount;
            entry.renderClass = (int)(renderClass);
            entry.renderParam = (int)(renderParam);
            if (preparedVertexCount > 0) {
                memcpy(
                    entry.vertices,
                    g_zVideo_D3DSubmitTempVertices,
                    (size_t)(preparedVertexCount) * sizeof(D3DTLVERTEX)
                );
            }
            return;
        }

        if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_SHADEMODE,
                2
            );
            g_zVideo_D3DRenderStateCache.shadeMode = 2;
        }
        if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                renderClass->textureHandle
            );
            g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
        }
        if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(2)) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREMAPBLEND,
                2
            );
            g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSU,
                renderClass->textureAddressU
            );
            g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
        }
        if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREADDRESSV,
                renderClass->textureAddressV
            );
            g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            (D3DPRIMITIVETYPE)(6),
            (D3DVERTEXTYPE)(3),
            g_zVideo_D3DSubmitTempVertices,
            (DWORD)(preparedVertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x9a4
            );
        }
        return;
    }

    if (queueMode != 0) {
        const int queueIndex = g_zVideo_OverwriteQueueCount;
        if (queueIndex >= 0x180) {
            zError::ReportOld(
                0x400,
                g_zVideo_SourceFile_ZvidDdd3dC,
                0x9ad,
                g_zVideo_NotEnoughMaxOverwritePolysNeedFmt,
                queueIndex
            );
            return;
        }

        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[queueIndex];
        ++g_zVideo_OverwriteQueueCount;
        entry.type = 0;
        entry.renderClass = (int)(renderClass);
        entry.renderParam = (int)(renderParam);
        int preparedVertexCount = vertexCount;
        if (vertexCount > 0) {
            CopyPositionUvWithPreparedColorReverse(
                entry.vertices,
                vertices,
                uvPairs,
                g_zVideo_D3DSubmitTempVertices,
                vertexCount
            );
        }
        AppendFanCloseVertexIfNeeded(
            entry.vertices,
            preparedVertexCount
        );
        entry.vertexCount = preparedVertexCount;
        return;
    }

    const int queueIndex = g_zVideo_SortedPolyQueueCount;
    if ((unsigned int)(queueIndex) >= 0x100) {
        zError::ReportOld(
            0x400,
            g_zVideo_SourceFile_ZvidDdd3dC,
            0x9e4,
            g_zVideo_NotEnoughMaxTransparentPolysFmt,
            queueIndex
        );
        return;
    }

    zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[queueIndex];
    entry.renderClass = (int)(renderClass);
    entry.renderParam = (int)(renderParam);
    int preparedVertexCount = vertexCount;
    if (vertexCount > 0) {
        CopyPositionUvWithPreparedColorReverse(
            entry.vertices,
            vertices,
            uvPairs,
            g_zVideo_D3DSubmitTempVertices,
            vertexCount
        );
    }
    AppendFanCloseVertexIfNeeded(
        entry.vertices,
        preparedVertexCount
    );
    entry.vertexCount = preparedVertexCount;
    ++g_zVideo_SortedPolyQueueCount;
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Convert one 16-bit colored point to a Direct3D TL vertex and draw it
 * through the cached point-list render-state path.
 */
void __fastcall DrawPointColor16(
    zVideo_XyzVertex *pointPos,
    unsigned int packedColor16,
    int pointCount
) {
    (void)pointCount;

    D3DTLVERTEX &vertex = g_zVideo_D3DSubmitTempVertices[0];
    vertex.sx = pointPos->x;
    vertex.sy = pointPos->y;
    vertex.sz = pointPos->z;
    vertex.rhw = pointPos->z;
    vertex.color = PackD3DColorFrom16(
        packedColor16,
        0xff
    );
    vertex.specular = 0xff000000;

    if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_TEXTUREHANDLE,
            0
        );
        g_zVideo_D3DRenderStateCache.textureHandle = 0;
    }
    if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_SHADEMODE,
            1
        );
        g_zVideo_D3DRenderStateCache.shadeMode = 1;
    }

    const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
        (D3DPRIMITIVETYPE)(1),
        (D3DVERTEXTYPE)(3),
        g_zVideo_D3DSubmitTempVertices,
        1,
        0
    );
    if (hresult != DD_OK) {
        zVideo_dd::ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdd3dC,
            0xa4c
        );
    }
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: stamp the current Direct3D quad-batch depth and reciprocal
 * homogeneous weight across all cached TL vertices.
 *
 * Evidence: BN uses a bottomRight.z cursor and writes each item's TL vertices
 * in bottom-left, bottom-right, top-right, top-left order for z, then the
 * same order for rhw.
 */
void __stdcall SetQuadBatchDepthAndRhw(
    float depthAndRhw
) {
    for (int itemIndex = 0; itemIndex < 16; ++itemIndex) {
        zVideo_QuadBatchItemPartial &item = g_zVideo_QuadBatchItemsBase[itemIndex];

        item.vertices[3].sz = depthAndRhw;
        item.vertices[2].sz = depthAndRhw;
        item.vertices[1].sz = depthAndRhw;
        item.vertices[0].sz = depthAndRhw;
        item.vertices[3].rhw = depthAndRhw;
        item.vertices[2].rhw = depthAndRhw;
        item.vertices[1].rhw = depthAndRhw;
        item.vertices[0].rhw = depthAndRhw;
    }
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Queue one alpha-blended solid screen-space quad for the Direct3D batch flush.
 */
void __fastcall QueueSolidQuad(
    unsigned int packedColor16,
    zVidRect32 *clipRect,
    double alpha
) {
    const int batchIndex = g_zVideo_QuadBatchCount;
    if ((unsigned int)(batchIndex) >= 0x10) {
        return;
    }

    zVideo_QuadBatchItemPartial &item = g_zVideo_QuadBatchItemsBase[batchIndex];

    float left;
    float top;
    float right;
    float bottom;
    if (clipRect != 0) {
        left = (float)(clipRect->left);
        top = (float)(clipRect->top);
        right = (float)(clipRect->right);
        bottom = (float)(clipRect->bottom);
    } else {
        left = 0.0f;
        top = 0.0f;
        right = (float)(g_zVideo_PrimarySurfaceState.height);
        bottom = (float)(g_zVideo_PrimarySurfaceState.width);
    }

    item.vertices[0].sx = left;
    item.vertices[0].sy = top;
    item.vertices[1].sx = right;
    item.vertices[1].sy = top;
    item.vertices[2].sx = right;
    item.vertices[2].sy = bottom;
    item.vertices[3].sx = left;
    item.vertices[3].sy = bottom;

    const int alphaByte = (int)(alpha * 255.0);
    const DWORD packedColor = PackD3DColorFrom16(
        packedColor16,
        alphaByte
    );
    for (int i = 0; i < 4; ++i) {
        item.vertices[i].color = packedColor;
    }

    ++g_zVideo_QuadBatchCount;
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Sort and draw queued Direct3D polys while maintaining the shared render-state cache.
 */
void FlushSortedPolys() {
    int queueCount = g_zVideo_SortedPolyQueueCount;
    if (queueCount == 0) {
        return;
    }

    if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_SHADEMODE,
            2
        );
        queueCount = g_zVideo_SortedPolyQueueCount;
        g_zVideo_D3DRenderStateCache.shadeMode = 2;
    }
    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ALPHABLENDENABLE,
            1
        );
        queueCount = g_zVideo_SortedPolyQueueCount;
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ZWRITEENABLE,
            0
        );
        queueCount = g_zVideo_SortedPolyQueueCount;
        g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
    }

    for (unsigned int i = 0; i < (unsigned int)(queueCount); ++i) {
        g_zVideo_SortedPolyDrawOrder[i] = queueCount - (int)(i)-1;
        queueCount = g_zVideo_SortedPolyQueueCount;
    }

    bool swapped;
    do {
        swapped = false;
        for (unsigned int i = 1; i < (unsigned int)(queueCount); ++i) {
            const int currentIndex = g_zVideo_SortedPolyDrawOrder[i];
            const int previousIndex = g_zVideo_SortedPolyDrawOrder[i - 1];
            if (g_zVideo_SortedPolyQueueBase[currentIndex].vertices[0].sz <
                g_zVideo_SortedPolyQueueBase[previousIndex].vertices[0].sz) {
                g_zVideo_SortedPolyDrawOrder[i - 1] = currentIndex;
                g_zVideo_SortedPolyDrawOrder[i] = previousIndex;
                queueCount = g_zVideo_SortedPolyQueueCount;
                swapped = true;
            }
        }
    } while (swapped);

    for (unsigned int i_4102 = 0; i_4102 < (unsigned int)(g_zVideo_SortedPolyQueueCount);
        ++i_4102) {
        const int drawIndex = g_zVideo_SortedPolyDrawOrder[i_4102];
        zVideo_SortedPolyQueueEntry &entry = g_zVideo_SortedPolyQueueBase[drawIndex];
        zVideo_RenderClass *renderClass = (zVideo_RenderClass *)(entry.renderClass);

        if (renderClass != 0) {
            if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    renderClass->textureHandle
                );
                g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
            }

            const D3DTEXTUREBLEND textureMapBlend = renderClass->textureMapBlend;
            const bool forceTransparentTextureBlend =
                textureMapBlend != (D3DTEXTUREBLEND)(4) &&
                (entry.vertices[0].color & 0xff000000) != 0xff000000;
            if (forceTransparentTextureBlend) {
                if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(4)) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREMAPBLEND,
                        4
                    );
                    g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(4);
                }
            } else if (g_zVideo_D3DRenderStateCache.textureMapBlend != textureMapBlend) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREMAPBLEND,
                    textureMapBlend
                );
                g_zVideo_D3DRenderStateCache.textureMapBlend = textureMapBlend;
            }

            if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSU,
                    renderClass->textureAddressU
                );
                g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSV,
                    renderClass->textureAddressV
                );
                g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
            }
        } else if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
            g_zVideo_pD3DDevice->SetRenderState(
                D3DRENDERSTATE_TEXTUREHANDLE,
                0
            );
            g_zVideo_D3DRenderStateCache.textureHandle = 0;
        }

        const HRESULT hresult = g_zVideo_pD3DDevice->DrawPrimitive(
            D3DPT_TRIANGLEFAN,
            (D3DVERTEXTYPE)(3),
            entry.vertices,
            (DWORD)(entry.vertexCount),
            0
        );
        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0xb09
            );
        }
    }

    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ALPHABLENDENABLE,
            0
        );
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ZWRITEENABLE,
            1
        );
        g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
    }
    g_zVideo_SortedPolyQueueCount = 0;
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Draw and clear the Direct3D solid-quad batch with cached render-state setup and restoration.
 */
void FlushQuadBatch() {
    if (g_zVideo_QuadBatchCount == 0) {
        return;
    }

    if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_SHADEMODE,
            2
        );
        g_zVideo_D3DRenderStateCache.shadeMode = 2;
    }
    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ALPHABLENDENABLE,
            1
        );
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ZWRITEENABLE,
            0
        );
        g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
    }
    if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_TEXTUREHANDLE,
            0
        );
        g_zVideo_D3DRenderStateCache.textureHandle = 0;
    }

    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        D3DCMP_ALWAYS
    );

    for (unsigned int i = 0; i < (unsigned int)(g_zVideo_QuadBatchCount); ++i) {
        g_zVideo_pD3DDevice->DrawPrimitive(
            D3DPT_TRIANGLEFAN,
            (D3DVERTEXTYPE)(3),
            g_zVideo_QuadBatchItemsBase[i].vertices,
            4,
            0
        );
    }

    g_zVideo_QuadBatchCount = 0;
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        D3DCMP_GREATEREQUAL
    );

    if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 0) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ALPHABLENDENABLE,
            0
        );
        g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
    }
    if (g_zVideo_D3DRenderStateCache.zWriteEnable != 1) {
        g_zVideo_pD3DDevice->SetRenderState(
            D3DRENDERSTATE_ZWRITEENABLE,
            1
        );
        g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
    }
}

/**
 * Source file evidence: GameZRecoil/zVideo/zvid_ddd3d.c.
 * Purpose: Draw overwrite-queue primitives with the Direct3D render-state cache and restore depth testing.
 */
void FlushOverwritePolys() {
    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        D3DCMP_ALWAYS
    );

    for (int i = 0; i < g_zVideo_OverwriteQueueCount; ++i) {
        zVideo_OverwriteQueueEntry &entry = g_zVideo_OverwriteQueueBase[i];
        HRESULT hresult = DD_OK;

        switch (entry.type) {
        case 0: {
            if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_SHADEMODE,
                    2
                );
                g_zVideo_D3DRenderStateCache.shadeMode = 2;
            }
            if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 1) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_ALPHABLENDENABLE,
                    1
                );
                g_zVideo_D3DRenderStateCache.alphaBlendEnable = 1;
            }
            if (g_zVideo_D3DRenderStateCache.zWriteEnable != 0) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_ZWRITEENABLE,
                    0
                );
                g_zVideo_D3DRenderStateCache.zWriteEnable = 0;
            }

            zVideo_RenderClass *renderClass = (zVideo_RenderClass *)(entry.renderClass);
            if (renderClass != 0) {
                if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREHANDLE,
                        renderClass->textureHandle
                    );
                    g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
                }

                const D3DTEXTUREBLEND textureMapBlend = renderClass->textureMapBlend;
                const bool forceTransparentTextureBlend =
                    textureMapBlend != (D3DTEXTUREBLEND)(4) &&
                    (entry.vertices[0].color & 0xff000000) != 0xff000000;
                if (forceTransparentTextureBlend) {
                    if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(4)) {
                        g_zVideo_pD3DDevice->SetRenderState(
                            D3DRENDERSTATE_TEXTUREMAPBLEND,
                            4
                        );
                        g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(4);
                    }
                } else if (g_zVideo_D3DRenderStateCache.textureMapBlend != textureMapBlend) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREMAPBLEND,
                        textureMapBlend
                    );
                    g_zVideo_D3DRenderStateCache.textureMapBlend = textureMapBlend;
                }

                if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREADDRESSU,
                        renderClass->textureAddressU
                    );
                    g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
                }
                if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
                    g_zVideo_pD3DDevice->SetRenderState(
                        D3DRENDERSTATE_TEXTUREADDRESSV,
                        renderClass->textureAddressV
                    );
                    g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
                }
            } else if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    0
                );
                g_zVideo_D3DRenderStateCache.textureHandle = 0;
            }

            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry.vertices,
                (DWORD)(entry.vertexCount),
                0
            );

            if (g_zVideo_D3DRenderStateCache.alphaBlendEnable != 0) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_ALPHABLENDENABLE,
                    0
                );
                g_zVideo_D3DRenderStateCache.alphaBlendEnable = 0;
            }
            if (g_zVideo_D3DRenderStateCache.zWriteEnable != 1) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_ZWRITEENABLE,
                    1
                );
                g_zVideo_D3DRenderStateCache.zWriteEnable = 1;
            }
            break;
        }

        case 1:
        case 2:
        case 3:
            if (g_zVideo_D3DRenderStateCache.textureHandle != 0) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    0
                );
                g_zVideo_D3DRenderStateCache.textureHandle = 0;
            }
            if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_SHADEMODE,
                    1
                );
                g_zVideo_D3DRenderStateCache.shadeMode = 1;
            }
            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry.vertices,
                (DWORD)(entry.vertexCount),
                0
            );
            break;

        case 4: {
            if (g_zVideo_D3DRenderStateCache.shadeMode != 1) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_SHADEMODE,
                    1
                );
                g_zVideo_D3DRenderStateCache.shadeMode = 1;
            }

            zVideo_RenderClass *renderClass = (zVideo_RenderClass *)(entry.renderClass);
            if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    renderClass->textureHandle
                );
                g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
            }
            if (g_zVideo_D3DRenderStateCache.textureMapBlend != renderClass->textureMapBlend) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREMAPBLEND,
                    renderClass->textureMapBlend
                );
                g_zVideo_D3DRenderStateCache.textureMapBlend = renderClass->textureMapBlend;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSU,
                    renderClass->textureAddressU
                );
                g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSV,
                    renderClass->textureAddressV
                );
                g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
            }

            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry.vertices,
                (DWORD)(entry.vertexCount),
                0
            );
            break;
        }

        case 5:
        case 6: {
            if (g_zVideo_D3DRenderStateCache.shadeMode != 2) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_SHADEMODE,
                    2
                );
                g_zVideo_D3DRenderStateCache.shadeMode = 2;
            }

            zVideo_RenderClass *renderClass = (zVideo_RenderClass *)(entry.renderClass);
            if (g_zVideo_D3DRenderStateCache.textureHandle != renderClass->textureHandle) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREHANDLE,
                    renderClass->textureHandle
                );
                g_zVideo_D3DRenderStateCache.textureHandle = renderClass->textureHandle;
            }
            if (g_zVideo_D3DRenderStateCache.textureMapBlend != (D3DTEXTUREBLEND)(2)) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREMAPBLEND,
                    2
                );
                g_zVideo_D3DRenderStateCache.textureMapBlend = (D3DTEXTUREBLEND)(2);
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressU != renderClass->textureAddressU) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSU,
                    renderClass->textureAddressU
                );
                g_zVideo_D3DRenderStateCache.textureAddressU = renderClass->textureAddressU;
            }
            if (g_zVideo_D3DRenderStateCache.textureAddressV != renderClass->textureAddressV) {
                g_zVideo_pD3DDevice->SetRenderState(
                    D3DRENDERSTATE_TEXTUREADDRESSV,
                    renderClass->textureAddressV
                );
                g_zVideo_D3DRenderStateCache.textureAddressV = renderClass->textureAddressV;
            }

            hresult = g_zVideo_pD3DDevice->DrawPrimitive(
                D3DPT_TRIANGLEFAN,
                (D3DVERTEXTYPE)(3),
                entry.vertices,
                (DWORD)(entry.vertexCount),
                0
            );
            break;
        }

        default:
            break;
        }

        if (hresult != DD_OK) {
            zVideo_dd::ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdd3dC,
                0xbb7
            );
        }
    }

    g_zVideo_pD3DDevice->SetRenderState(
        D3DRENDERSTATE_ZFUNC,
        D3DCMP_GREATEREQUAL
    );
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
int __fastcall FloorPowerOfTwo(
    int value
) {
    int powerOfTwo = 1;
    do {
        powerOfTwo <<= 1;
    } while (powerOfTwo < value);

    if (powerOfTwo == value) {
        return value;
    }

    return powerOfTwo >> 1;
}

} // namespace zVideo_dd3d

namespace zVideo_dd {

/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: maps DirectDraw/Direct3D HRESULTs to report text and emits the legacy DirectDraw error report.
 */
RECOIL_NO_GS int __fastcall ReportError(
    int hresult,
    const char *sourceFile,
    int sourceLine
) {
    char errorNameBuffer[0x100];

#define ZVIDEO_DD_REPORT_ERROR_NAME(nameText) \
    sprintf( \
        errorNameBuffer, \
        nameText \
    )

    switch (hresult) {
    case DDERR_GENERIC:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Generic);
        break;
    case DDERR_UNSUPPORTED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_Unsupported);
        break;
    case DDERR_OUTOFMEMORY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OutOfMemory);
        break;
    case DDERR_NOTINITIALIZED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotInitialized);
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
    case DDERR_NOCOLORKEYHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoColorKeyHw);
        break;
    case DDERR_NODIRECTDRAWSUPPORT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDirectDrawSupport);
        break;
    case DDERR_NOEXCLUSIVEMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoExclusiveMode);
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
    case DDERR_OVERLAYCANTCLIP:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OverlayCantClip);
        break;
    case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OverlayColorKeyOnlyOneActive);
        break;
    case DDERR_PALETTEBUSY:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_PaletteBusy);
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
    case DDERR_CANTPAGELOCK:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantPageLock);
        break;
    case DDERR_CANTPAGEUNLOCK:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantPageUnlock);
        break;
    case DDERR_NOTPAGELOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotPageLocked);
        break;
    case DDERR_XALIGN:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_XAlign);
        break;
    case DDERR_INVALIDDIRECTDRAWGUID:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidDirectDrawGuid);
        break;
    case DDERR_DIRECTDRAWALREADYCREATED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_DirectDrawAlreadyCreated);
        break;
    case DDERR_NODIRECTDRAWHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDirectDrawHw);
        break;
    case DDERR_PRIMARYSURFACEALREADYEXISTS:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_PrimarySurfaceAlreadyExists);
        break;
    case DDERR_NOEMULATION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoEmulation);
        break;
    case DDERR_REGIONTOOSMALL:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_RegionTooSmall);
        break;
    case DDERR_CLIPPERISUSINGHWND:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ClipperIsUsingHwnd);
        break;
    case DDERR_NOCLIPPERATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoClipperAttached);
        break;
    case DDERR_NOHWND:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoHwnd);
        break;
    case DDERR_HWNDSUBCLASSED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_HwndSubclassed);
        break;
    case DDERR_HWNDALREADYSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_HwndAlreadySet);
        break;
    case DDERR_NOPALETTEATTACHED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoPaletteAttached);
        break;
    case DDERR_NOPALETTEHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoPaletteHw);
        break;
    case DDERR_BLTFASTCANTCLIP:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_BltFastCantClip);
        break;
    case DDERR_NOBLTHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoBltHw);
        break;
    case DDERR_NODDROPSHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDdRopsHw);
        break;
    case DDERR_OVERLAYNOTVISIBLE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_OverlayNotVisible);
        break;
    case DDERR_INVALIDPOSITION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidPosition);
        break;
    case DDERR_NOTAOVERLAYSURFACE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoAOverlaySurface);
        break;
    case DDERR_EXCLUSIVEMODEALREADYSET:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ExclusiveModeAlreadySet);
        break;
    case DDERR_NOTFLIPPABLE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotFlippable);
        break;
    case DDERR_CANTDUPLICATE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantDuplicate);
        break;
    case DDERR_NOTLOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotLocked);
        break;
    case DDERR_CANTCREATEDC:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_CantCreateDc);
        break;
    case DDERR_NODC:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoDirectDc);
        break;
    case DDERR_WRONGMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_WrongMode);
        break;
    case DDERR_IMPLICITLYCREATED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_ImplicitlyCreated);
        break;
    case DDERR_NOTPALETTIZED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NotPalettized);
        break;
    case DDERR_UNSUPPORTEDMODE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_UnsupportedMode);
        break;
    case DDERR_NOMIPMAPHW:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_NoMipMapHw);
        break;
    case DDERR_INVALIDSURFACETYPE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_InvalidSurfaceType);
        break;
    case DDERR_DCALREADYCREATED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_DDErrorName_DcAlreadyCreated);
        break;
    case D3DERR_BADMAJORVERSION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_BadMajorVersion);
        break;
    case D3DERR_BADMINORVERSION:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_BadMinorVersion);
        break;
    case D3DERR_INVALID_DEVICE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_InvalidDevice);
        break;
    case D3DERR_EXECUTE_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteCreateFailed);
        break;
    case D3DERR_EXECUTE_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteDestroyFailed);
        break;
    case D3DERR_EXECUTE_LOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteLockFailed);
        break;
    case D3DERR_EXECUTE_UNLOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteUnlockFailed);
        break;
    case D3DERR_EXECUTE_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteLocked);
        break;
    case D3DERR_EXECUTE_NOT_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteNotLocked);
        break;
    case D3DERR_EXECUTE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteFailed);
        break;
    case D3DERR_EXECUTE_CLIPPED_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_ExecuteClippedFailed);
        break;
    case D3DERR_TEXTURE_NO_SUPPORT:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureNoSupport);
        break;
    case D3DERR_TEXTURE_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureCreateFailed);
        break;
    case D3DERR_TEXTURE_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureDestroyFailed);
        break;
    case D3DERR_TEXTURE_LOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureLockFailed);
        break;
    case D3DERR_TEXTURE_UNLOCK_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureUnlockFailed);
        break;
    case D3DERR_TEXTURE_LOAD_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureLoadFailed);
        break;
    case D3DERR_TEXTURE_SWAP_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureSwapFailed);
        break;
    case D3DERR_TEXTURE_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureLocked);
        break;
    case D3DERR_TEXTURE_NOT_LOCKED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureNotLocked);
        break;
    case D3DERR_TEXTURE_GETSURF_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureGetSurfFailed);
        break;
    case D3DERR_MATRIX_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixCreateFailed);
        break;
    case D3DERR_MATRIX_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixDestroyFailed);
        break;
    case D3DERR_MATRIX_SETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixSetDataFailed);
        break;
    case D3DERR_MATRIX_GETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MatrixGetDataFailed);
        break;
    case D3DERR_SETVIEWPORTDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SetViewportDataFailed);
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
    case D3DERR_TEXTURE_BADSIZE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_TextureBadSize);
        break;
    case D3DERR_MATERIAL_CREATE_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialCreateFailed);
        break;
    case D3DERR_MATERIAL_DESTROY_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialDestroyFailed);
        break;
    case D3DERR_MATERIAL_SETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialSetDataFailed);
        break;
    case D3DERR_MATERIAL_GETDATA_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_MaterialGetDataFailed);
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
    case D3DERR_SCENE_IN_SCENE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneInScene);
        break;
    case D3DERR_SCENE_NOT_IN_SCENE:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneNotInScene);
        break;
    case D3DERR_SCENE_BEGIN_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneBeginFailed);
        break;
    case D3DERR_SCENE_END_FAILED:
        ZVIDEO_DD_REPORT_ERROR_NAME(g_zVideo_D3DErrorName_SceneEndFailed);
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

        g_zVideo_pfnQueryTextureMemoryBytes(
            -1,
            &textureMemTotalBytes,
            &textureMemFreeBytes
        );
        g_zVideo_pfnQueryDeviceVideoMemoryBytes(
            -1,
            &videoMemTotalBytes,
            &videoMemFreeBytes
        );
    }

    char reportMessageBuffer[0x100];
    sprintf(
        reportMessageBuffer,
        g_zVideo_DirectDrawErrorFmt,
        errorNameBuffer,
        sourceFile,
        sourceLine
    );
    zError::ReportOld(
        0x400,
        sourceFile,
        sourceLine,
        reportMessageBuffer
    );
    return -1;
}

} // namespace zVideo_dd
