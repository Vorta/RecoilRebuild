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

/*
 * Recovered literal-backed zvid_dd.c physical contribution
 * [0x4a7b40, 0x4a9ac0). Definitions remain in natural retail source order.
 */

namespace zVideo_dd {
namespace {
const int kPresentMissingSurfaceResult = 0x400;
const int kPresentFailureResult = 0x5a56ffff;
const int kPresentLinePageLock = 0x6c;
const int kPresentLinePageUnlock = 0x91;
const int kPresentLineBltOrRestore = 0xac;

template <typename InterfaceT>
/**
 * Original inline helper; no standalone retail function exists.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: release a DirectDraw/Direct3D COM interface pointer and clear the
 * owning global or field.
 *
 * Original inline helper evidence: BN callers including 0x4a95e0 and 0x4a9300
 * emit the same null-check, provider Release call, and zero-store pattern for
 * temporary and subsystem-owned COM interfaces.
 */
void ReleaseComInterface(
    InterfaceT *&value
) {
    if (value != 0) {
        value->Release();
        value = 0;
    }
}

/**
 * Original-source helper evidence: source-faithful helper recovered from address-backed callers in this source file.
 * Purpose: provide the recovered PageUnlockBeforeRelease helper behavior for zVideo callers.
 */
bool PageUnlockBeforeRelease(
    zVideo_SurfaceStatePartial &state,
    int reportLine
) {
    if (state.surf != 0 && state.pageLockActive != 0) {
        const HRESULT hresult = state.surf->PageUnlock(0);
        if (hresult != DD_OK) {
            ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                reportLine
            );
            return false;
        }

        state.pageLockActive = 0;
    }

    return true;
}
} // namespace
} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: enumerate DirectDraw devices and select the first hardware device
 * record as the default startup renderer.
 *
 * Evidence: BN calls RunDirectDrawDeviceEnumeration, stores
 * &g_zVideo_HwApiDeviceTable[0] in g_zVideo_pSelectedHwApiDeviceRecord, clears
 * g_zVideo_pSelectedD3DDeviceInfo, and VC5SP3 byte verification has zero
 * unmasked mismatches for this body.
 */
void __cdecl StartupEnumerateAndDefaultSelect() {
    RunDirectDrawDeviceEnumeration();
    g_zVideo_pSelectedHwApiDeviceRecord = &g_zVideo_HwApiDeviceTable[0];
    g_zVideo_pSelectedD3DDeviceInfo = 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Purpose: Present the software/display-mode surface through DirectDraw.
 *
 * Evidence: BN source file zvid_dd.c page-locks the primary surface for the
 * fullscreen software adjustment path, copies the 0x20-byte primary and
 * software surface records through g_zVideo_SurfaceStateSwapScratch when the
 * swap is enabled, unlocks the active primary record when it remains locked,
 * and retries a failed Blt after DDERR_SURFACELOST Restore succeeds.
 */
int __fastcall PresentDisplayModeSurface(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int skipSurfaceStateSwap
) {
    zVidRect32 *const presentSrcRect = srcRect;
    DWORD presentBltFlags =
        DDBLT_WAIT + (waitForPresent != 0 ? 0 : DDBLT_ASYNC);

    if (g_zVideo_DisplayModeSurfaceState.surf == 0 ||
        g_zVideo_PrimarySurfaceState.surf == 0) {
        return kPresentMissingSurfaceResult;
    }

    HRESULT hresult;

    for (;;) {
        if (g_zVideo_UseHalfResBackbuffer != 0) {
            hresult = g_zVideo_DisplayModeSurfaceState.surf->Blt(
                (RECT *)(dstRect),
                g_zVideo_PrimarySurfaceState.surf,
                (RECT *)(presentSrcRect),
                presentBltFlags,
                0
            );
        } else if (g_zVideo_HalfResAdjustMode != 0) {
            hresult = g_zVideo_PrimarySurfaceState.surf->PageLock(0);
            if (hresult != DD_OK) {
                ReportError(
                    (int)(hresult),
                    g_zVideo_SourceFile_ZvidDdC,
                    kPresentLinePageLock
                );
                return 0;
            }

            hresult = g_zVideo_DisplayModeSurfaceState.surf->Blt(
                (RECT *)(dstRect),
                g_zVideo_PrimarySurfaceState.surf,
                (RECT *)(presentSrcRect),
                DDBLT_ASYNC,
                0
            );
            g_zVideo_PrimarySurfaceState.pageLockActive = 1;

            if (skipSurfaceStateSwap == 0) {
                memcpy(
                    &g_zVideo_SurfaceStateSwapScratch,
                    &g_zVideo_PrimarySurfaceState,
                    sizeof(g_zVideo_SurfaceStateSwapScratch)
                );
                memcpy(
                    &g_zVideo_PrimarySurfaceState,
                    &g_zVideo_SwSurfaceState,
                    sizeof(g_zVideo_PrimarySurfaceState)
                );
                memcpy(
                    &g_zVideo_SwSurfaceState,
                    &g_zVideo_SurfaceStateSwapScratch,
                    sizeof(g_zVideo_SwSurfaceState)
                );

                if (g_zVideo_PrimarySurfaceState.pageLockActive != 0) {
                    const HRESULT pageUnlockResult =
                        g_zVideo_PrimarySurfaceState.surf->PageUnlock(0);
                    if (pageUnlockResult != DD_OK) {
                        ReportError(
                            (int)(pageUnlockResult),
                            g_zVideo_SourceFile_ZvidDdC,
                            kPresentLinePageUnlock
                        );
                        return 0;
                    }

                    g_zVideo_PrimarySurfaceState.pageLockActive = 0;
                }
            }
        } else {
            hresult = g_zVideo_DisplayModeSurfaceState.surf->Blt(
                (RECT *)(dstRect),
                g_zVideo_PrimarySurfaceState.surf,
                (RECT *)(presentSrcRect),
                presentBltFlags,
                0
            );
        }

        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = g_zVideo_DisplayModeSurfaceState.surf->Restore();
            if (hresult == DD_OK) {
                continue;
            }
        }

        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            kPresentLineBltOrRestore
        );
        return kPresentFailureResult;
    }
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: run the fullscreen window preparation step and create the selected
 * DirectDraw2 device, returning one on failure and zero on success.
 */
int __fastcall OpenVideoMode(
    int
) {
    if (PrepareWindowForMode() != 0) {
        return 1;
    }

    if (CreateDirectDraw2ForSelectedDevice() != 0) {
        return 1;
    }
    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: clear the default texture record and tear down the DirectDraw
 * backend state.
 *
 * Evidence: BN clears g_zVideo_DefaultTextureRecord after destroying the
 * default record when present, calls TeardownVideoSubsystem unconditionally,
 * and returns zero.
 */
int __cdecl ShutdownVideoSystem() {
    if (g_zVideo_DefaultTextureRecord != 0) {
        zVideo_dd3d::TextureRecord_Destroy(g_zVideo_DefaultTextureRecord);
        g_zVideo_DefaultTextureRecord = 0;
    }

    TeardownVideoSubsystem();
    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: flip the attached primary DirectDraw surface back to GDI when the
 * fullscreen primary owns an attached backbuffer.
 */
void __cdecl FlipToGDIIfAttached() {
    if (g_zVideo_pDirectDraw2 != 0 && g_zVideo_PrimaryHasAttachedBackbuffer != 0) {
        g_zVideo_pDirectDraw2->FlipToGDISurface();
    }
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: copy a source rectangle from the software surface directly to the
 * primary DirectDraw surface.
 *
 * Evidence: BN uses fastcall ECX=srcRect and EDX=dstRect, calls
 * IDirectDrawSurface3::Blt on g_zVideo_PrimarySurfaceState.surf with dstRect,
 * g_zVideo_SwSurfaceState.surf, srcRect, DDBLT_WAIT, and null DDBLTFX, then
 * reports line 0xe9 on nonzero HRESULT.
 */
void __fastcall BltSwToPrimaryRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf->Blt(
        (RECT *)(dstRect),
        g_zVideo_SwSurfaceState.surf,
        (RECT *)(srcRect),
        DDBLT_WAIT,
        0
    );
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0xe9
        );
    }
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: copy a source rectangle from the primary surface directly back to
 * the software DirectDraw surface.
 *
 * Evidence: BN uses fastcall ECX=srcRect and EDX=dstRect, calls
 * IDirectDrawSurface3::Blt on g_zVideo_SwSurfaceState.surf with dstRect,
 * g_zVideo_PrimarySurfaceState.surf, srcRect, DDBLT_WAIT, and null DDBLTFX,
 * then reports line 0xfc on nonzero HRESULT. BN's raw name shows zVideoDD,
 * while the source/plan owner uses zVideo_dd.
 */
void __fastcall BltPrimaryToSwRectDirect(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    const HRESULT hresult = g_zVideo_SwSurfaceState.surf->Blt(
        (RECT *)(dstRect),
        g_zVideo_PrimarySurfaceState.surf,
        (RECT *)(srcRect),
        DDBLT_WAIT,
        0
    );
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0xfc
        );
    }
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: blit an image-backed software surface to the primary surface with
 * default rectangles, primary clipping, optional source color key, and primary
 * lock preservation.
 *
 * Evidence: BN lazily creates srcImage->surface with caps 0x20004000 when the
 * selected hardware device has feature flags, otherwise DDSCAPS_SYSTEMMEMORY;
 * builds default source and destination rectangles, clips destination against
 * g_zVideo_PrimarySurfaceState width/height while mirroring offsets into the
 * source rect, unlocks/relocks the primary state if it was locked, calls
 * IDirectDrawSurface3::Blt with DDBLT_WAIT | DDBLT_ASYNC plus optional
 * DDBLT_KEYSRC, and reports line 0x159 on Blt failure.
 */
void __fastcall BltSwToPrimaryRect(
    zVidImagePartial *srcImage,
    int srcColorKeyEnable,
    zVidRect32 *srcRect,
    zVidRect32 *dstRect
) {
    if (srcImage->surface == 0) {
        if (g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags != 0) {
            if (Image_LazyCreateBackingSurface(
                srcImage,
                0x20004000
            ) == 0) {
                return;
            }
        } else if (Image_LazyCreateBackingSurface(
            srcImage,
            DDSCAPS_SYSTEMMEMORY
        ) == 0) {
            return;
        }
    }

    zVidRect32 srcRectLocal;
    if (srcRect != 0) {
        srcRectLocal = *srcRect;
    } else {
        srcRectLocal.left = 0;
        srcRectLocal.top = 0;
        srcRectLocal.right = srcImage->width;
        srcRectLocal.bottom = srcImage->height;
    }

    zVidRect32 dstRectLocal;
    if (dstRect != 0) {
        dstRectLocal = *dstRect;
    } else {
        dstRectLocal.left = 0;
        dstRectLocal.right = srcRectLocal.right - srcRectLocal.left;
        dstRectLocal.top = 0;
        dstRectLocal.bottom = srcRectLocal.bottom - srcRectLocal.top;
    }

    const DWORD bltFlags =
        DDBLT_WAIT + DDBLT_ASYNC + (srcColorKeyEnable != 0 ? DDBLT_KEYSRC : 0);

    int clipped = zVideo_buff::ClipCoordToRange(
        &dstRectLocal.left,
        0,
        g_zVideo_PrimarySurfaceState.width - 1
    );
    if (clipped < 0) {
        srcRectLocal.left -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped =
        zVideo_buff::ClipCoordToRange(
            &dstRectLocal.right,
            0,
            g_zVideo_PrimarySurfaceState.width
        );
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.right -= clipped;
    }

    clipped = zVideo_buff::ClipCoordToRange(
        &dstRectLocal.top,
        0,
        g_zVideo_PrimarySurfaceState.height - 1
    );
    if (clipped < 0) {
        srcRectLocal.top -= clipped;
    } else if (clipped > 0) {
        return;
    }

    clipped =
        zVideo_buff::ClipCoordToRange(
            &dstRectLocal.bottom,
            0,
            g_zVideo_PrimarySurfaceState.height
        );
    if (clipped < 0) {
        return;
    }
    if (clipped > 0) {
        srcRectLocal.bottom -= clipped;
    }

    const int wasLocked = g_zVideo_PrimarySurfaceState.locked;
    if (wasLocked != 0) {
        UnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    const HRESULT hresult =
        g_zVideo_PrimarySurfaceState.surf
            ->Blt(
                (RECT *)(&dstRectLocal),
                srcImage->surface,
                (RECT *)(&srcRectLocal),
                bltFlags,
                0
            );

    if (wasLocked != 0) {
        LockSurfaceState(&g_zVideo_PrimarySurfaceState);
    }

    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x159
        );
    }
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Purpose: lock a tracked DirectDraw surface state and cache the surface
 * descriptor fields used by software rendering paths.
 */
int __fastcall LockSurfaceState(
    zVideo_SurfaceStatePartial *surfaceState
) {
    if (g_zVideo_FullscreenOption == 0 &&
        surfaceState == &g_zVideo_DisplayModeSurfaceState) {
        return 0;
    }

    if (surfaceState->locked != 0) {
        return 0;
    }

    DDSURFACEDESC lockedSurfaceDesc;
    const int result = LockDirectDrawSurface(
        surfaceState->surf,
        &lockedSurfaceDesc
    );
    if (result == 0) {
        surfaceState->locked = 1;
        surfaceState->width = (int)(lockedSurfaceDesc.dwWidth);
        surfaceState->height = (int)(lockedSurfaceDesc.dwHeight);
        surfaceState->lockInfoValid = 1;
        surfaceState->pixels = lockedSurfaceDesc.lpSurface;
        surfaceState->pitch = (int)(lockedSurfaceDesc.lPitch);
    }

    return result;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Purpose: unlock a tracked DirectDraw surface state when the runtime policy
 * and lock flag require it.
 */
int __fastcall UnlockSurfaceState(
    zVideo_SurfaceStatePartial *surfaceState
) {
    if (g_zVideo_FullscreenOption == 0 &&
        surfaceState == &g_zVideo_DisplayModeSurfaceState) {
        return 0;
    }

    if (surfaceState->locked == 0) {
        return 0;
    }

    surfaceState->locked = 0;
    return UnlockDirectDrawSurface(surfaceState->surf);
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: lock a DirectDraw surface descriptor, restoring and retrying when
 * the provider reports a lost surface.
 *
 * Evidence: BN zeroes and sizes the DDSURFACEDESC to 0x6c bytes, calls
 * IDirectDrawSurface3::Lock with null rect, DDLOCK_WAIT, and null event, loops
 * through Restore on DDERR_SURFACELOST, reports line 0x1b9 on unrecovered
 * provider errors, and returns 0x5a56ffff on failure.
 */
int __fastcall LockDirectDrawSurface(
    IDirectDrawSurface3 *surface,
    DDSURFACEDESC *outLockedSurfaceDesc
) {
    memset(
        outLockedSurfaceDesc,
        0,
        sizeof(*outLockedSurfaceDesc)
    );
    outLockedSurfaceDesc->dwSize = sizeof(*outLockedSurfaceDesc);

    HRESULT hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = surface->Lock(
            0,
            outLockedSurfaceDesc,
            DDLOCK_WAIT,
            0
        );
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
        }
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x1b9
    );
    return 0x5a56ffff;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: unlock a DirectDraw surface, restoring and retrying when the
 * provider reports a lost surface.
 *
 * Evidence: BN calls IDirectDrawSurface3::Unlock with a null surface pointer,
 * loops through Restore on DDERR_SURFACELOST, reports line 0x1d7 on
 * unrecovered provider errors, returns zero on success, and returns
 * 0x5a56ffff on failure.
 */
int __fastcall UnlockDirectDrawSurface(
    IDirectDrawSurface3 *surface
) {
    HRESULT hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = surface->Unlock(0);
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
        }
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x1d7
    );
    return 0x5a56ffff;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: locks a DirectDraw surface with DDLOCK_WAIT, retrying once the
 * provider restores a lost surface and reporting permanent failures.
 */
int __fastcall LockSurface_WaitRestore(
    IDirectDrawSurface3 *surface,
    DDSURFACEDESC *lockedDescOut
) {
    memset(
        lockedDescOut,
        0,
        sizeof(*lockedDescOut)
    );
    lockedDescOut->dwSize = sizeof(*lockedDescOut);

    HRESULT hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = surface->Lock(
            0,
            lockedDescOut,
            DDLOCK_WAIT,
            0
        );
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
        }
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x1fd
    );
    return 0x5a56ffff;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: unlocks a DirectDraw surface, retrying once the provider restores a
 * lost surface and reporting permanent failures.
 */
int __fastcall UnlockSurface_WaitRestore(
    IDirectDrawSurface3 *surface
) {
    HRESULT hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = surface->Unlock(0);
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = surface->Restore();
        }
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x21b
    );
    return 0x5a56ffff;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Purpose: clear the current DirectDraw Z-buffer rectangle to depth zero.
 *
 * Evidence: BN source file zvid_dd.c tests g_zVideo_pZBufferSurface, builds a
 * DDBLTFX with dwSize 0x64 and dwFillDepth zero, calls DirectDrawSurface3::Blt
 * with DDBLT_DEPTHFILL, and reports line 0x242 after a failed Restore retry.
 */
int __fastcall ZBuffer_DepthFillRect(
    zVidRect32 *dstRect
) {
    // BN writes only the DirectDraw fields consumed by the selected fill mode.
    DDBLTFX bltFx;
    HRESULT hresult;
    bltFx.dwSize = sizeof(bltFx);
    if (g_zVideo_pZBufferSurface == 0) {
        return 0;
    }

    bltFx.dwFillDepth = 0;
    hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = g_zVideo_pZBufferSurface->Blt(
            (RECT *)(dstRect),
            0,
            0,
            DDBLT_DEPTHFILL,
            &bltFx
        );
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = g_zVideo_pZBufferSurface->Restore();
        }
    }

    return ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x242
    );
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Purpose: clear a color surface rectangle and then the matching Z-buffer.
 *
 * Evidence: BN source file zvid_dd.c gates the color fill with
 * g_zVideo_ClearScreenBufferEnabled, uses g_zVideo_ClearColorPacked16 for
 * DDBLT_COLORFILL|DDBLT_WAIT, then optionally clears g_zVideo_pZBufferSurface
 * with DDBLT_DEPTHFILL and report lines 0x267 and 0x27f.
 */
int __fastcall ClearScreenAndZBufferRect(
    zVidRect32 *dstRect,
    zVideo_SurfaceStatePartial *colorSurfaceState
) {
    // BN writes only the DirectDraw fields consumed by the selected fill mode.
    DDBLTFX bltFx;
    bltFx.dwSize = sizeof(bltFx);

    if (g_zVideo_ClearScreenBufferEnabled != 0) {
        bltFx.dwFillColor = g_zVideo_ClearColorPacked16;
        HRESULT hresult = DD_OK;
        while (hresult == DD_OK) {
            hresult = colorSurfaceState->surf->Blt(
                (RECT *)(dstRect),
                0,
                0,
                DDBLT_COLORFILL | DDBLT_WAIT,
                &bltFx
            );
            if (hresult == DD_OK) {
                break;
            }

            if (hresult == DDERR_SURFACELOST) {
                hresult = colorSurfaceState->surf->Restore();
            }
        }
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x267
            );
        }
    }

    if (g_zVideo_pZBufferSurface == 0) {
        return 0;
    }

    bltFx.dwFillDepth = 0;
    HRESULT hresult;
    hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = g_zVideo_pZBufferSurface->Blt(
            (RECT *)(dstRect),
            0,
            0,
            DDBLT_DEPTHFILL,
            &bltFx
        );
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = g_zVideo_pZBufferSurface->Restore();
        }
    }

    return ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x27f
    );
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Purpose: clear the software backbuffer rectangle and a separate Z rectangle.
 *
 * Evidence: BN source file zvid_dd.c fills g_zVideo_SwSurfaceState.surf when
 * screen-buffer clearing is enabled, then optionally clears
 * g_zVideo_pZBufferSurface with the supplied Z rectangle and reports lines
 * 0x2a5 and 0x2bd on permanent provider failures.
 */
int __fastcall ClearSwBackbufferAndZBufferRects(
    zVidRect32 *colorRect,
    zVidRect32 *zRect
) {
    // BN writes only the DirectDraw fields consumed by the selected fill mode.
    DDBLTFX bltFx;
    bltFx.dwSize = sizeof(bltFx);

    if (g_zVideo_ClearScreenBufferEnabled != 0) {
        bltFx.dwFillColor = g_zVideo_ClearColorPacked16;
        HRESULT hresult = DD_OK;
        while (hresult == DD_OK) {
            hresult = g_zVideo_SwSurfaceState.surf->Blt(
                (RECT *)(colorRect),
                0,
                0,
                DDBLT_COLORFILL | DDBLT_WAIT,
                &bltFx
            );
            if (hresult == DD_OK) {
                break;
            }

            if (hresult == DDERR_SURFACELOST) {
                hresult = g_zVideo_SwSurfaceState.surf->Restore();
            }
        }
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x2a5
            );
        }
    }

    if (g_zVideo_pZBufferSurface == 0) {
        return 0;
    }

    bltFx.dwFillDepth = 0;
    HRESULT hresult;
    hresult = DD_OK;
    while (hresult == DD_OK) {
        hresult = g_zVideo_pZBufferSurface->Blt(
            (RECT *)(zRect),
            0,
            0,
            DDBLT_DEPTHFILL,
            &bltFx
        );
        if (hresult == DD_OK) {
            return 0;
        }

        if (hresult == DDERR_SURFACELOST) {
            hresult = g_zVideo_pZBufferSurface->Restore();
        }
    }

    return ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x2bd
    );
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: lazily create a DirectDrawSurface3 backing store for a heap-backed
 * image and populate it from the image pixels.
 *
 * Evidence: BN guards alpha maps, null pixels, and zero width/height, creates
 * an offscreen DirectDraw surface from the requested caps, queries
 * IID_IDirectDrawSurface3, stores image->surface only after QueryInterface
 * succeeds, calls Image_PopulateSurfaceFromHeapPixels, and reports line 0x2ed
 * on provider failure.
 */
IDirectDrawSurface3 *__fastcall Image_LazyCreateBackingSurface(
    zVidImagePartial *image,
    unsigned int ddsCapsFlags
) {
    if (image->alphaMap != 0 || image->pixels == 0 || image->height == 0 || image->width == 0) {
        return 0;
    }

    IDirectDrawSurface *baseSurface = 0;
    IDirectDrawSurface3 *surface3 = 0;
    DDSURFACEDESC desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x10007;
    desc.dwHeight = (DWORD)(image->height);
    desc.dwWidth = (DWORD)(image->width);
    desc.ddsCaps.dwCaps = ddsCapsFlags | DDSCAPS_OFFSCREENPLAIN;
    image->surface = 0;

    HRESULT hresult = g_zVideo_pDirectDraw2->CreateSurface(
        &desc,
        &baseSurface,
        0
    );
    if (hresult == DD_OK) {
        hresult = baseSurface->QueryInterface(
            IID_IDirectDrawSurface3,
            (void **)(&surface3)
        );
        if (hresult == DD_OK) {
            image->surface = surface3;
            Image_PopulateSurfaceFromHeapPixels(image);
            return image->surface;
        }
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x2ed
    );
    return image->surface;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create an image video-memory backing surface when the current
 * renderer/device state requires one.
 *
 * Evidence: BN reads g_zVideo_UseHalfResBackbuffer and
 * g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags, returns null
 * when neither requests video memory, then tail-calls
 * Image_LazyCreateBackingSurface with DDSCAPS_VIDEOMEMORY plus optional
 * DDSCAPS_NONLOCALVIDMEM.
 */
IDirectDrawSurface3 *__fastcall Image_LazyCreateVideoMemorySurface(
    zVidImagePartial *image
) {
    if (g_zVideo_UseHalfResBackbuffer == 0 &&
        g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags == 0) {
        return 0;
    }

    const unsigned int caps =
        (g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags != 0
            ? DDSCAPS_NONLOCALVIDMEM
            : 0) + DDSCAPS_VIDEOMEMORY;
    return Image_LazyCreateBackingSurface(
        image,
        caps
    );
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: copy an image heap pixel buffer into its locked DirectDraw surface
 * and rebind the image pixels to the surface memory.
 *
 * Evidence: BN locks image->surface with DDLOCK_WAIT, retries after
 * DDERR_SURFACELOST restores, copies width * 2 bytes per row, frees the heap
 * buffer, stores the locked surface pointer and half-pitch, unlocks with the
 * same lost-surface retry pattern, and reports lines 0x31b, 0x31f, 0x33b, and
 * 0x33f on provider failures.
 */
int __fastcall Image_PopulateSurfaceFromHeapPixels(
    zVidImagePartial *image
) {
    DDSURFACEDESC lockedSurfaceDesc = {0};
    lockedSurfaceDesc.dwSize = sizeof(lockedSurfaceDesc);
    HRESULT hresult;

    for (;;) {
        hresult = image->surface->Lock(
            0,
            &lockedSurfaceDesc,
            DDLOCK_WAIT,
            0
        );
        if (hresult == DD_OK) {
            break;
        }

        if (hresult != DDERR_SURFACELOST) {
            ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x31f
            );
            return 0;
        }

        hresult = image->surface->Restore();
        if (hresult != DD_OK) {
            ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x31b
            );
        }
    }

    const int rowBytes = (int)(image->width) << 1;
    unsigned char *srcPixels = (unsigned char *)(image->pixels);
    unsigned char *dstPixels = (unsigned char *)(lockedSurfaceDesc.lpSurface);
    {
        for (int row = 0; row < image->height; ++row) {
            memcpy(
                dstPixels,
                srcPixels,
                rowBytes
            );
            dstPixels += lockedSurfaceDesc.lPitch;
            srcPixels += rowBytes;
        }
    }

    free(image->pixels);
    image->pixels = lockedSurfaceDesc.lpSurface;
    image->pitchWords = (int)((unsigned int)(lockedSurfaceDesc.lPitch) >> 1);

    for (;;) {
        hresult = image->surface->Unlock(&lockedSurfaceDesc);
        if (hresult == DD_OK) {
            return 1;
        }

        if (hresult != DDERR_SURFACELOST) {
            break;
        }

        hresult = image->surface->Restore();
        if (hresult != DD_OK) {
            ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x33b
            );
        }
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x33f
    );
    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: release and clear an image-owned DirectDraw surface so it can be
 * recreated for the current video device.
 *
 * Evidence: BN releases image->surface only when g_zVideo_IsInitialized is
 * nonzero and the surface is present, then clears image->surface and
 * image->pixels whenever a stale surface pointer remains.
 */
void __fastcall Image_EnsureSurfaceForCurrentDevice(
    zVidImagePartial *image
) {
    if (g_zVideo_IsInitialized != 0 && image->surface != 0) {
        image->surface->Release();
    }

    if (image->surface != 0) {
        image->surface = 0;
        image->pixels = 0;
    }
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: ensure an image has a DirectDraw surface and acquire a GDI DC for
 * drawing into it.
 *
 * Evidence: BN returns zero for renderer type 2, lazily creates a backing
 * surface from selected-device feature flags when image->surface is null,
 * calls IDirectDrawSurface3::GetDC, returns one on DD_OK, and reports line
 * 0x36d on provider failure.
 */
int __fastcall Image_UploadPixelsToSurface(
    zVidImagePartial *image,
    HDC *outHdc
) {
    if (g_zVideo_RendererType == 2) {
        return 0;
    }

    if (image->surface == 0) {
        // Original upload path assumes device selection is complete before lazy creation.
        unsigned int caps = DDSCAPS_NONLOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
        if (g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags == 0) {
            caps = DDSCAPS_SYSTEMMEMORY;
        }

        IDirectDrawSurface3 *surface = Image_LazyCreateBackingSurface(
            image,
            caps
        );
        if (surface == 0) {
            return (int)(surface);
        }
    }

    const HRESULT hresult = image->surface->GetDC(outHdc);
    if (hresult == DD_OK) {
        return 1;
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x36d
    );
    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: release a GDI DC acquired from an image-backed DirectDraw surface.
 *
 * Evidence: BN returns zero when image->surface is null, calls
 * IDirectDrawSurface3::ReleaseDC with the supplied HDC, returns one on DD_OK,
 * and reports line 0x382 on provider failure.
 */
int __fastcall Image_ReleaseSurface(
    zVidImagePartial *image,
    HDC hdc
) {
    IDirectDrawSurface3 *surface = image->surface;
    if (surface == 0) {
        return (int)(surface);
    }

    const HRESULT hresult = surface->ReleaseDC(hdc);
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x382
        );
        return 0;
    }

    return 1;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: enter exclusive fullscreen cooperative mode and apply the current
 * DirectDraw display mode.
 *
 * Evidence: BN calls IDirectDraw2::SetCooperativeLevel with g_zVideo_hWnd and
 * flags 0x13, reports source line 0x393 on failure, then calls SetDisplayMode
 * with the display surface width/height, BPP, zero refresh, and zero flags,
 * reporting line 0x39c on failure.
 */
int __cdecl SetDisplayMode() {
    HRESULT hresult = g_zVideo_pDirectDraw2->SetCooperativeLevel(
        g_zVideo_hWnd,
        0x13
    );
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x393
        );
        return 0;
    }

    hresult = g_zVideo_pDirectDraw2->SetDisplayMode(
        g_zVideo_DisplayModeSurfaceState.width,
        g_zVideo_DisplayModeSurfaceState.height,
        g_zVideo_DisplayModeBpp,
        0,
        0
    );
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x39c
        );
        return 0;
    }

    return 1;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: rebuild fullscreen DirectDraw surfaces for the active renderer and
 * verify the restored display surfaces.
 *
 * Evidence: BN uses the mode-independent backend path after zVideo has already
 * applied geometry, gates each DirectDraw setup step on nonzero failure, tests
 * g_zVideo_RendererType for the hardware-only Direct3D device creation, and
 * normalizes the final surface-lock verification result to 0 or 1.
 */
int __fastcall SetVideoMode(
    int
) {
    if (SetDisplayMode() == 0) {
        return 1;
    }

    if (RestoreDisplaySurfaces() != 0) {
        return 1;
    }

    if (ReleaseAllInterfacesAndSurfaces() != 0) {
        return 1;
    }

    if (CreateFullscreenSurfacesForRenderer() != 0) {
        return 1;
    }

    if (g_zVideo_RendererType == 1 && zVideo_dd3d::CreateDeviceState() != 0) {
        return 1;
    }

    if (RestoreDisplaySurfaces() != 0) {
        return 1;
    }

    if (VerifyFullscreenSurfaceLocks() != 0) {
        return 1;
    }
    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create the selected DirectDraw device, query its IDirectDraw2
 * interface, and cache that interface for the active video backend.
 *
 * Evidence: BN reads g_zVideo_pSelectedHwApiDeviceRecord->pDirectDrawGuid,
 * calls the DirectDrawCreate provider import, queries IID_IDirectDraw2 into
 * g_zVideo_pDirectDraw2, releases the temporary IDirectDraw on success, and
 * routes the two HRESULT failures through ReportError.
 */
int __cdecl CreateDirectDraw2ForSelectedDevice() {
    IDirectDraw *directDraw1;
    const HRESULT createResult =
        DirectDrawCreate(
            g_zVideo_pSelectedHwApiDeviceRecord->pDirectDrawGuid,
            &directDraw1,
            0
        );
    if (createResult != DD_OK) {
        return ReportError(
            (int)(createResult),
            g_zVideo_SourceFile_ZvidDdC,
            0x3c4
        );
    }

    const HRESULT queryResult =
        directDraw1->QueryInterface(
            IID_IDirectDraw2,
            (void **)(&g_zVideo_pDirectDraw2)
        );
    if (queryResult != DD_OK) {
        return ReportError(
            (int)(queryResult),
            g_zVideo_SourceFile_ZvidDdC,
            0x3cb
        );
    }

    directDraw1->Release();
    return 0;
}

} // namespace zVideo_dd

namespace zVideo {

/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Purpose: commit an accepted hardware API device as the active renderer
 * backend.
 *
 * Evidence: BN calls BindRendererDispatch(1, 1), indexes
 * g_zVideo_HwApiDeviceTable by the supplied index, stores
 * g_zVideo_pSelectedHwApiDeviceRecord to that record, and stores
 * g_zVideo_pSelectedD3DDeviceInfo to the record's m_d3dDrivers field.
 */
void __fastcall CommitHwApiDeviceSelection(
    int hwApiIndex
) {
    BindRendererDispatch(
        1,
        1
    );
    zVidHwApiDeviceRecordPartial &selected = g_zVideo_HwApiDeviceTable[hwApiIndex];
    g_zVideo_pSelectedHwApiDeviceRecord = &selected;
    g_zVideo_pSelectedD3DDeviceInfo = selected.m_d3dDrivers;
}

} // namespace zVideo

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create a DirectDraw surface and return its DirectDrawSurface3
 * interface.
 *
 * Evidence: BN shows a DirectDraw2::CreateSurface call, a successful-surface
 * QueryInterface for IID_IDirectDrawSurface3, Release of the temporary base
 * surface only after successful QueryInterface, and direct propagation of the
 * current provider HRESULT. BN callers pass one unused zero stack argument and
 * the callee returns with ret 8.
 */
HRESULT __fastcall CreateSurface3FromDesc(
    IDirectDraw2 *directDraw,
    DDSURFACEDESC *desc,
    IDirectDrawSurface3 **outSurface,
    int reserved
) {
    IDirectDrawSurface *createdSurface;
    reserved;
    HRESULT result = directDraw->CreateSurface(
        desc,
        &createdSurface,
        0
    );
    if (result == DD_OK) {
        result = createdSurface->QueryInterface(
            IID_IDirectDrawSurface3,
            (void **)(outSurface)
        );
        if (result == DD_OK) {
            return createdSurface->Release();
        }
    }

    return result;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: select the active fullscreen surface-creation path for the current
 * renderer and half-resolution setting.
 *
 * Evidence: BN tests g_zVideo_UseHalfResBackbuffer first, then dispatches by
 * g_zVideo_RendererType to the Direct3D hardware or software surface builders.
 */
int __cdecl CreateFullscreenSurfacesForRenderer() {
    if (g_zVideo_UseHalfResBackbuffer != 0) {
        return CreateHalfResBackbufferSurfaces();
    }

    if (g_zVideo_RendererType == 1) {
        return CreateFullscreenHardwareSurfaces();
    }

    return CreateFullscreenSoftwareSurfaces();
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create the half-resolution display, primary, software, and clipper
 * surfaces used by fullscreen rendering.
 *
 * Evidence: BN builds the primary/backbuffer surface desc, retrieves the
 * attached backbuffer, reads the selected hardware-device feature flags
 * directly when choosing software-surface caps, creates the half-resolution
 * software surface from the current video dimensions, initializes pixel
 * packing, then attaches a window clipper to the display-mode surface.
 */
int __cdecl CreateHalfResBackbufferSurfaces() {
    DDSURFACEDESC desc = {0};
    DDSCAPS attachedCaps = {0};
    int defaultGfxFlagsPayload = 0;
    zOptionEntryPartial *gfxFlagsOption =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    if (gfxFlagsOption == 0) {
        gfxFlagsOption = (zOptionEntryPartial *)(&defaultGfxFlagsPayload);
    }

    desc.dwSize = sizeof(desc);
    desc.dwBackBufferCount = 1;
    desc.dwFlags = 0x21;
    desc.ddsCaps.dwCaps = 0x218;

    HRESULT hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_DisplayModeSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x41f
        );
    }

    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    attachedCaps.dwCaps = DDSCAPS_BACKBUFFER;
    hresult = g_zVideo_DisplayModeSurfaceState.surf->GetAttachedSurface(
        &attachedCaps,
        &g_zVideo_PrimarySurfaceState.surf
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x429
        );
    }

    desc.dwFlags = 0x07;
    if ((gfxFlagsOption->payloadOrBuffer & 0x10000) != 0) {
        desc.ddsCaps.dwCaps = 0x4040;
    } else {
        const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
        desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;
    }
    desc.dwWidth = (DWORD)(g_zVideo_SwSurfaceState.width);
    desc.dwHeight = (DWORD)(g_zVideo_SwSurfaceState.height);

    hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_SwSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x43f
        );
    }

    if (InitFullscreenSoftwarePixelPack(g_zVideo_DisplayModeSurfaceState.surf) != 0) {
        return 1;
    }

    hresult = g_zVideo_pDirectDraw2->CreateClipper(
        0,
        &g_zVideo_pClipper,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x447
        );
    }

    hresult = g_zVideo_pClipper->SetHWnd(
        0,
        g_zVideo_hWnd
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x44b
        );
    }

    hresult = g_zVideo_DisplayModeSurfaceState.surf->SetClipper(g_zVideo_pClipper);
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x450
        );
    }

    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create fullscreen DirectDraw display, primary, software, and
 * clipper surfaces for the software renderer path.
 *
 * Evidence: BN creates the display-mode surface, probes lockability with a
 * fallback to a plain primary surface, creates primary and software surfaces,
 * initializes pixel packing from the display surface, and installs the window
 * clipper.
 */
int __cdecl CreateFullscreenSoftwareSurfaces() {
    DDSURFACEDESC desc = {0};
    int defaultGfxFlagsPayload = 0;
    zOptionEntryPartial *gfxFlagsOption =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    if (gfxFlagsOption == 0) {
        gfxFlagsOption = (zOptionEntryPartial *)(&defaultGfxFlagsPayload);
    }

    desc.dwSize = sizeof(desc);
    desc.dwFlags = 1;
    desc.ddsCaps.dwCaps = 0xa00;

    HRESULT hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_DisplayModeSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x4cc
        );
    }

    if (LockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0) {
        g_zVideo_DisplayModeSurfaceState.surf->Release();
        desc.ddsCaps.dwCaps = 0x200;
        hresult = CreateSurface3FromDesc(
            g_zVideo_pDirectDraw2,
            &desc,
            &g_zVideo_DisplayModeSurfaceState.surf,
            0
        );
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x4da
            );
        }
    } else {
        UnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState);
    }

    g_zVideo_PrimaryHasAttachedBackbuffer = 0;
    desc.dwFlags = 7;
    if ((gfxFlagsOption->payloadOrBuffer & 0x10000) != 0) {
        desc.ddsCaps.dwCaps = 0x4040;
    } else {
        const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
        desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;
    }
    desc.dwWidth = (DWORD)(g_zVideo_DisplayModeSurfaceState.width);
    desc.dwHeight = (DWORD)(g_zVideo_DisplayModeSurfaceState.height);

    hresult =
        CreateSurface3FromDesc(
            g_zVideo_pDirectDraw2,
            &desc,
            &g_zVideo_PrimarySurfaceState.surf,
            0
        );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x4f7
        );
    }

    desc.dwFlags = 7;
    if ((gfxFlagsOption->payloadOrBuffer & 0x10000) != 0) {
        desc.ddsCaps.dwCaps = 0x4040;
    } else {
        const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
        desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;
    }
    desc.dwWidth = (DWORD)(g_zVideo_DisplayModeSurfaceState.width);
    desc.dwHeight = (DWORD)(g_zVideo_DisplayModeSurfaceState.height);

    hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_SwSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x50d
        );
    }

    if (InitFullscreenSoftwarePixelPack(g_zVideo_DisplayModeSurfaceState.surf) != 0) {
        return 1;
    }

    hresult = g_zVideo_pDirectDraw2->CreateClipper(
        0,
        &g_zVideo_pClipper,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x515
        );
    }

    hresult = g_zVideo_pClipper->SetHWnd(
        0,
        g_zVideo_hWnd
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x519
        );
    }

    hresult = g_zVideo_DisplayModeSurfaceState.surf->SetClipper(g_zVideo_pClipper);
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x51d
        );
    }

    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: create fullscreen DirectDraw display, attached software, primary,
 * and clipper surfaces for the hardware renderer path.
 *
 * Evidence: BN creates a flipping display-mode surface, obtains the attached
 * backbuffer as the software surface, creates the primary render surface using
 * selected-device feature flags, initializes pixel packing, and installs the
 * window clipper.
 */
int __cdecl CreateFullscreenHardwareSurfaces() {
    DDSURFACEDESC desc = {0};
    DDSCAPS attachedCaps = {0};
    desc.dwSize = sizeof(desc);
    desc.dwBackBufferCount = 1;
    desc.dwFlags = 0x21;
    desc.ddsCaps.dwCaps = 0x2218;

    HRESULT hresult = CreateSurface3FromDesc(
        g_zVideo_pDirectDraw2,
        &desc,
        &g_zVideo_DisplayModeSurfaceState.surf,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x53b
        );
    }

    g_zVideo_PrimaryHasAttachedBackbuffer = 1;
    attachedCaps.dwCaps = DDSCAPS_BACKBUFFER;
    hresult = g_zVideo_DisplayModeSurfaceState.surf->GetAttachedSurface(
        &attachedCaps,
        &g_zVideo_SwSurfaceState.surf
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x546
        );
    }

    desc.dwFlags = 7;
    desc.dwWidth = (DWORD)(g_zVideo_DisplayModeSurfaceState.width);
    desc.dwHeight = (DWORD)(g_zVideo_DisplayModeSurfaceState.height);
    const int featureFlags = g_zVideo_pSelectedHwApiDeviceRecord->m_deviceFeatureFlags;
    desc.ddsCaps.dwCaps = (featureFlags != 0 ? 0x20003800 : 0) + 0x840;

    hresult =
        CreateSurface3FromDesc(
            g_zVideo_pDirectDraw2,
            &desc,
            &g_zVideo_PrimarySurfaceState.surf,
            0
        );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x557
        );
    }

    if (InitFullscreenSoftwarePixelPack(g_zVideo_DisplayModeSurfaceState.surf) != 0) {
        return 1;
    }

    hresult = g_zVideo_pDirectDraw2->CreateClipper(
        0,
        &g_zVideo_pClipper,
        0
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x55f
        );
    }

    hresult = g_zVideo_pClipper->SetHWnd(
        0,
        g_zVideo_hWnd
    );
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x563
        );
    }

    hresult = g_zVideo_DisplayModeSurfaceState.surf->SetClipper(g_zVideo_pClipper);
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x567
        );
    }

    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: read the fullscreen display pixel format and initialize the
 * software pixel-pack masks for supported formats.
 *
 * Evidence: BN initializes DDPIXELFORMAT.dwSize to 0x20, calls the
 * IDirectDrawSurface3::GetPixelFormat provider slot, reports provider failures
 * at source line 0x597, accepts green masks 0x07e0, 0x03e0, and 0xff00, and
 * tears down plus reports line 0x5bd for unrecognized formats.
 */
int __fastcall InitFullscreenSoftwarePixelPack(
    IDirectDrawSurface3 *displaySurface
) {
    DDPIXELFORMAT pixelFormat = {0};
    pixelFormat.dwSize = sizeof(pixelFormat);

    const HRESULT hresult = displaySurface->GetPixelFormat(&pixelFormat);
    if (hresult != DD_OK) {
        return ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x597
        );
    }

    if (pixelFormat.dwGBitMask == 0x07e0) {
        zVideo::PixelPack_SetupFromMasks(
            5,
            6,
            5,
            pixelFormat.dwRBitMask,
            pixelFormat.dwGBitMask,
            pixelFormat.dwBBitMask
        );
        return 0;
    }

    if (pixelFormat.dwGBitMask == 0x03e0) {
        zVideo::PixelPack_SetupFromMasks(
            5,
            5,
            5,
            pixelFormat.dwRBitMask,
            0x03e0,
            pixelFormat.dwBBitMask
        );
        return 0;
    }

    if (pixelFormat.dwGBitMask == 0xff00) {
        zVideo::PixelPack_SetupFromMasks(
            5,
            6,
            5,
            pixelFormat.dwRBitMask,
            pixelFormat.dwGBitMask,
            pixelFormat.dwBBitMask
        );
        return 0;
    }

    TeardownVideoSubsystem();
    zError::ReportOld(
        0x800,
        g_zVideo_SourceFile_ZvidDdC,
        0x5bd,
        g_zVideo_UnrecognizedPixelFormatMsg
    );
    return 0x5a56ffff;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: verify that the software, primary, and display-mode DirectDraw
 * surface states can each lock and unlock.
 *
 * Evidence: BN calls LockSurfaceState and UnlockSurfaceState for
 * g_zVideo_SwSurfaceState, g_zVideo_PrimarySurfaceState, and
 * g_zVideo_DisplayModeSurfaceState in that order, returning 1 after any
 * failed probe.
 */
int __cdecl VerifyFullscreenSurfaceLocks() {
    if (LockSurfaceState(&g_zVideo_SwSurfaceState) != 0) {
        return 1;
    }
    if (UnlockSurfaceState(&g_zVideo_SwSurfaceState) != 0) {
        return 1;
    }
    if (LockSurfaceState(&g_zVideo_PrimarySurfaceState) != 0) {
        return 1;
    }
    if (UnlockSurfaceState(&g_zVideo_PrimarySurfaceState) != 0) {
        return 1;
    }
    if (LockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0) {
        return 1;
    }

    return UnlockSurfaceState(&g_zVideo_DisplayModeSurfaceState) != 0 ? 1 : 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: restore the display-mode, primary, and software DirectDraw
 * surfaces when they are present.
 *
 * Evidence: BN restores g_zVideo_DisplayModeSurfaceState.surf, then
 * g_zVideo_PrimarySurfaceState.surf, then g_zVideo_SwSurfaceState.surf, and
 * reports DirectDraw failures at source lines 0x5e1, 0x5e8, and 0x5ef.
 */
int __cdecl RestoreDisplaySurfaces() {
    if (g_zVideo_DisplayModeSurfaceState.surf != 0) {
        const HRESULT hresult = g_zVideo_DisplayModeSurfaceState.surf->Restore();
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x5e1
            );
        }
    }

    if (g_zVideo_PrimarySurfaceState.surf != 0) {
        const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf->Restore();
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x5e8
            );
        }
    }

    if (g_zVideo_SwSurfaceState.surf != 0) {
        const HRESULT hresult = g_zVideo_SwSurfaceState.surf->Restore();
        if (hresult != DD_OK) {
            return ReportError(
                (int)(hresult),
                g_zVideo_SourceFile_ZvidDdC,
                0x5ef
            );
        }
    }

    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: optionally ask the surface-lock verifier to validate the current
 * surface state for a teardown caller context.
 *
 * Evidence: BN gates on g_zVideo_SurfaceLockVerifyFlags bit 0x20, builds a
 * 0x28-byte zVideo_SurfaceLockVerifyArgs record with callerContext at offset
 * 0x1c, calls g_zVideo_pSurfaceLockVerifier->VerifySurfaceState, and reports
 * nonzero HRESULTs at source line 0x61a.
 */
void __fastcall VerifySurfaceStateLocking(
    int callerContext
) {
    if ((g_zVideo_SurfaceLockVerifyFlags & 0x20) == 0) {
        return;
    }

    zVideo_SurfaceLockVerifyArgs args = {0};
    args.size = sizeof(args);
    args.callerContext = callerContext;
    const int hresult = g_zVideo_pSurfaceLockVerifier->VerifySurfaceState(&args);
    if (hresult != DD_OK) {
        ReportError(
            hresult,
            g_zVideo_SourceFile_ZvidDdC,
            0x61a
        );
    }
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: release the Direct3D and DirectDraw interface globals plus tracked
 * surfaces, page-unlocking locked surfaces before their COM release.
 *
 * Evidence: BN releases the D3D material, viewport, device, Direct3D2,
 * clipper, Z-buffer, software, primary, display-mode, and palette globals in
 * this order; PageUnlock failures at source lines 0x652 and 0x662 route
 * through ReportError and stop the remaining release pass.
 */
int __cdecl ReleaseAllInterfacesAndSurfaces() {
    if (g_zVideo_pD3DMaterial2 != 0) {
        g_zVideo_pD3DMaterial2->Release();
        g_zVideo_pD3DMaterial2 = 0;
    }
    if (g_zVideo_pD3DViewport2 != 0) {
        g_zVideo_pD3DViewport2->Release();
        g_zVideo_pD3DViewport2 = 0;
    }
    if (g_zVideo_pD3DDevice != 0) {
        g_zVideo_pD3DDevice->Release();
        g_zVideo_pD3DDevice = 0;
    }
    if (g_zVideo_pD3D2 != 0) {
        g_zVideo_pD3D2->Release();
        g_zVideo_pD3D2 = 0;
    }
    if (g_zVideo_pClipper != 0) {
        g_zVideo_pClipper->Release();
        g_zVideo_pClipper = 0;
    }
    if (g_zVideo_pZBufferSurface != 0) {
        g_zVideo_pZBufferSurface->Release();
        g_zVideo_pZBufferSurface = 0;
    }

    if (g_zVideo_SwSurfaceState.surf != 0) {
        if (g_zVideo_SwSurfaceState.pageLockActive != 0) {
            const HRESULT hresult = g_zVideo_SwSurfaceState.surf->PageUnlock(0);
            if (hresult != DD_OK) {
                ReportError(
                    (int)(hresult),
                    g_zVideo_SourceFile_ZvidDdC,
                    0x652
                );
                return 0;
            }
            g_zVideo_SwSurfaceState.pageLockActive = 0;
        }
        g_zVideo_SwSurfaceState.surf->Release();
        g_zVideo_SwSurfaceState.surf = 0;
    }

    if (g_zVideo_PrimarySurfaceState.surf != 0) {
        if (g_zVideo_PrimarySurfaceState.pageLockActive != 0) {
            const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf->PageUnlock(0);
            if (hresult != DD_OK) {
                ReportError(
                    (int)(hresult),
                    g_zVideo_SourceFile_ZvidDdC,
                    0x662
                );
                return 0;
            }
            g_zVideo_PrimarySurfaceState.pageLockActive = 0;
        }
        g_zVideo_PrimarySurfaceState.surf->Release();
        g_zVideo_PrimarySurfaceState.surf = 0;
    }

    if (g_zVideo_DisplayModeSurfaceState.surf != 0) {
        g_zVideo_DisplayModeSurfaceState.surf->Release();
        g_zVideo_DisplayModeSurfaceState.surf = 0;
    }
    if (g_zVideo_pDDPalette != 0) {
        g_zVideo_pDDPalette->Release();
        g_zVideo_pDDPalette = 0;
    }

    return 0;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: tear down the remaining DirectDraw fullscreen state after the
 * surface/interface release pass.
 *
 * Evidence: BN first calls ReleaseAllInterfacesAndSurfaces, then page-unlocks
 * and releases g_zVideo_pPageUnlockSurface, verifies and releases
 * g_zVideo_pSurfaceLockVerifier, restores IDirectDraw2 cooperative level to
 * normal, releases g_zVideo_pDirectDraw2, and clears each released global.
 */
void __cdecl TeardownVideoSubsystem() {
    ReleaseAllInterfacesAndSurfaces();

    if (g_zVideo_pPageUnlockSurface != 0) {
        g_zVideo_pPageUnlockSurface->PageUnlock(0);
        g_zVideo_pPageUnlockSurface->Release();
        g_zVideo_pPageUnlockSurface = 0;
    }

    if (g_zVideo_pSurfaceLockVerifier != 0) {
        VerifySurfaceStateLocking(g_zVideo_SurfaceLockVerifyContext);
        g_zVideo_pSurfaceLockVerifier->Release();
        g_zVideo_pSurfaceLockVerifier = 0;
    }

    if (g_zVideo_pDirectDraw2 != 0) {
        g_zVideo_pDirectDraw2->SetCooperativeLevel(
            g_zVideo_hWnd,
            8
        );
        g_zVideo_pDirectDraw2->Release();
        g_zVideo_pDirectDraw2 = 0;
    }
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: run DirectDraw device enumeration during video startup.
 *
 * Evidence: BN prints the enumeration banner, calls the DirectDrawEnumerateA
 * provider import with EnumDirectDrawDeviceCallback and a null context, returns
 * one on DD_OK, and routes nonzero HRESULTs through ReportError at source line
 * 0x6ad before returning zero.
 */
int __cdecl RunDirectDrawDeviceEnumeration() {
    printf(g_zVideo_DDrawEnumBeginMsg);
    const HRESULT hresult = DirectDrawEnumerateA(
        EnumDirectDrawDeviceCallback,
        0
    );
    if (hresult != DD_OK) {
        ReportError(
            (int)(hresult),
            g_zVideo_SourceFile_ZvidDdC,
            0x6ad
        );
        return 0;
    }

    return 1;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: collect one DirectDraw device record and enumerate its usable
 * Direct3D drivers during startup.
 *
 * Evidence: BN indexes g_zVideo_HwApiDeviceTable by
 * g_zVideo_NumAcceptedDirectDrawDevices, copies the optional DirectDraw GUID
 * plus driver strings, temporarily selects the record, creates DirectDraw2,
 * gathers caps and video/texture memory, tags AGP-capable devices, calls
 * EnumerateDirect3DDevicesForRecord, increments the accepted DirectDraw count
 * only when D3D enumeration succeeds, tears down temporary interfaces, and
 * returns TRUE to continue enumeration except on capacity or caps failure.
 */
BOOL CALLBACK EnumDirectDrawDeviceCallback(
    GUID *guid,
    LPSTR driverDescription,
    LPSTR driverName,
    LPVOID context
) {
    (void)context;

    zVidHwApiDeviceRecordPartial *entry =
        &g_zVideo_HwApiDeviceTable[g_zVideo_NumAcceptedDirectDrawDevices];
    const int ordinal = g_zVideo_DirectDrawEnumOrdinal++;

    printf(
        g_zVideo_DDrawEnumDevicePrintfFmt,
        ordinal,
        driverName,
        driverDescription
    );
    fflush(stdout);

    if (g_zVideo_NumAcceptedDirectDrawDevices >= 4) {
        printf(g_zVideo_DDrawEnumTooManyDevicesMsg);
        return FALSE;
    }

    memset(
        entry,
        0,
        sizeof(*entry)
    );
    if (guid == 0) {
        entry->pDirectDrawGuid = 0;
    } else {
        entry->pDirectDrawGuid = &entry->m_directDrawGuidStorage;
        entry->m_directDrawGuidStorage = *guid;
    }

    strncpy(
        entry->m_driverName,
        driverName,
        sizeof(entry->m_driverName)
    );
    strncpy(
        entry->m_driverDescription,
        driverDescription,
        sizeof(entry->m_driverDescription)
    );
    g_zVideo_pSelectedHwApiDeviceRecord = entry;

    CreateDirectDraw2ForSelectedDevice();

    memset(
        &g_zVideo_DDrawCapsHal,
        0,
        sizeof(g_zVideo_DDrawCapsHal)
    );
    memset(
        &g_zVideo_DDrawCapsHel,
        0,
        sizeof(g_zVideo_DDrawCapsHel)
    );
    g_zVideo_DDrawCapsHal.dwSize = sizeof(g_zVideo_DDrawCapsHal);
    g_zVideo_DDrawCapsHel.dwSize = sizeof(g_zVideo_DDrawCapsHel);

    const HRESULT capsResult =
        g_zVideo_pDirectDraw2->GetCaps(
            &g_zVideo_DDrawCapsHal,
            &g_zVideo_DDrawCapsHel
        );
    if (capsResult != DD_OK) {
        ReportError(
            (int)(capsResult),
            g_zVideo_SourceFile_ZvidDdC,
            0x739
        );
        return FALSE;
    }

    if ((g_zVideo_DDrawCapsHal.dwCaps & 0x200) != 0 ||
        (g_zVideo_DDrawCapsHel.dwCaps & 0x200) != 0) {
        entry->m_deviceFeatureFlags = 1;
        strcat(
            entry->m_driverName,
            g_zVideo_DDrawEnumAgpSuffix
        );
    }

    DDSCAPS memoryCaps;
    memoryCaps.dwCaps = DDSCAPS_VIDEOMEMORY;
    if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
            &memoryCaps,
            (DWORD *)(&entry->m_videoMemTotalBytes),
            (DWORD *)(&entry->m_videoMemFreeBytes)
        ) != DD_OK) {
        entry->m_videoMemFreeBytes = 0;
        entry->m_videoMemTotalBytes = 0;
    }

    memoryCaps.dwCaps = DDSCAPS_TEXTURE;
    if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
            &memoryCaps,
            (DWORD *)(&entry->m_textureMemTotalBytes),
            (DWORD *)(&entry->m_textureMemFreeBytes)
        ) != DD_OK) {
        entry->m_textureMemFreeBytes = 0;
        entry->m_textureMemTotalBytes = 0;
    }

    if (EnumerateDirect3DDevicesForRecord(entry) != 0) {
        g_zVideo_NumAcceptedDirectDrawDevices += 1;
    }

    TeardownVideoSubsystem();
    return TRUE;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: query IDirect3D2 from the active DirectDraw2 object and enumerate
 * usable Direct3D drivers for one DirectDraw device record.
 *
 * Evidence: BN preserves the original 0x68-byte stack zeroing, prints the
 * selected DirectDraw record name, queries IID_IDirect3D2 into g_zVideo_pD3D2,
 * resets m_acceptedD3DDeviceCount, runs EnumDevices with
 * EnumDirect3DDeviceCallback, releases g_zVideo_pD3D2, and returns one only
 * when the callback accepted at least one driver.
 */
int __fastcall EnumerateDirect3DDevicesForRecord(
    zVidHwApiDeviceRecordPartial *entry
) {
    unsigned int unusedStackScratch[0x1b];
    memset(
        &unusedStackScratch[1],
        0,
        0x68
    );

    printf(
        g_zVideo_D3DEnumBeginMsgFmt,
        entry->m_driverName
    );
    fflush(stdout);

    const HRESULT queryResult =
        g_zVideo_pDirectDraw2->QueryInterface(
            IID_IDirect3D2,
            (void **)(&g_zVideo_pD3D2)
        );
    if (queryResult != DD_OK) {
        ReportError(
            (int)(queryResult),
            g_zVideo_SourceFile_ZvidDdC,
            0x781
        );
        return 0;
    }

    entry->m_acceptedD3DDeviceCount = 0;
    g_zVideo_pD3D2->EnumDevices(
        EnumDirect3DDeviceCallback,
        entry
    );
    if (g_zVideo_pD3D2 != 0) {
        g_zVideo_pD3D2->Release();
        g_zVideo_pD3D2 = 0;
    }

    if (entry->m_acceptedD3DDeviceCount == 0) {
        printf(g_zVideo_D3DEnumNoUsableDriversMsg);
        return 0;
    }

    return 1;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: filter Direct3D enumeration callbacks and append accepted hardware
 * RGB devices with 16-bit Z-buffer support to the current DirectDraw record.
 *
 * Evidence: BN indexes entry->m_d3dDrivers by m_acceptedD3DDeviceCount,
 * rejects non-hardware, non-RGB, and missing 16-bit Z-buffer devices, aborts
 * with teardown plus zError::ReportOld on capacity overflow, copies optional
 * GUID storage and the hardware D3D device descriptor, defaults zero max
 * texture dimensions to 0x100, stores device strings, and increments both
 * accepted-driver counters.
 */
HRESULT CALLBACK EnumDirect3DDeviceCallback(
    GUID *guid,
    LPSTR deviceDescription,
    LPSTR deviceName,
    D3DDEVICEDESC *hwDesc,
    D3DDEVICEDESC *,
    LPVOID context
) {
    zVidHwApiDeviceRecordPartial *entry = (zVidHwApiDeviceRecordPartial *)(context);
    zVidD3DDriverRecordPartial &driver = entry->m_d3dDrivers[entry->m_acceptedD3DDeviceCount];

    printf(
        g_zVideo_D3DEnumDriverPrintfFmt,
        deviceName,
        deviceDescription
    );
    fflush(stdout);

    const unsigned int descFlags = hwDesc->dwFlags;
    if (descFlags == 0) {
        printf(g_zVideo_D3DEnumSkipNoHardwareMsg);
        fflush(stdout);
        return 1;
    }

    if ((descFlags & D3DDD_COLORMODEL) != 0 && hwDesc->dcmColorModel != D3DCOLOR_RGB) {
        printf(g_zVideo_D3DEnumSkipNoRgbColorMsg);
        fflush(stdout);
        return 1;
    }

    if ((hwDesc->dwDeviceZBufferBitDepth & DDBD_16) == 0) {
        printf(g_zVideo_D3DEnumSkipNo16BitZBufferMsg);
        fflush(stdout);
        return 1;
    }

    if (entry->m_acceptedD3DDeviceCount >= 4) {
        TeardownVideoSubsystem();
        zError::ReportOld(
            0x800,
            g_zVideo_SourceFile_ZvidDdC,
            0x7d3,
            g_zVideo_D3DEnumTooManyDriversMsg
        );
        return 0;
    }

    if (guid == 0) {
        driver.pD3DDeviceGuid = 0;
    } else {
        driver.pD3DDeviceGuid = &driver.m_d3dDeviceGuidStorage;
        driver.m_d3dDeviceGuidStorage = *guid;
    }

    memcpy(
        &driver.m_hwDesc,
        hwDesc,
        sizeof(driver.m_hwDesc)
    );
    if (driver.m_hwDesc.dwMaxTextureWidth == 0) {
        driver.m_hwDesc.dwMaxTextureWidth = 0x100;
    }
    if (driver.m_hwDesc.dwMaxTextureHeight == 0) {
        driver.m_hwDesc.dwMaxTextureHeight = 0x100;
    }

    strncpy(
        driver.m_deviceName,
        deviceName,
        sizeof(driver.m_deviceName)
    );
    strncpy(
        driver.m_deviceDescription,
        deviceDescription,
        sizeof(driver.m_deviceDescription)
    );
    printf(g_zVideo_D3DEnumAcceptedMsg);
    fflush(stdout);
    entry->m_acceptedD3DDeviceCount += 1;
    g_zVid_AcceptedHardwareRendererCount += 1;
    return 1;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: forward palette updates to the active DirectDraw palette only in
 * 8-bpp display modes.
 *
 * Evidence: BN gates on g_zVideo_DisplayModeBpp == 8, calls
 * IDirectDrawPalette::SetEntries with flags zero, and reports failures at
 * source line 0x823 before returning 0x5a56ffff.
 */
int __fastcall PaletteSetEntries(
    unsigned short firstEntry,
    unsigned short entryCount,
    PALETTEENTRY *entries
) {
    if (g_zVideo_DisplayModeBpp != 8) {
        return 0;
    }

    const HRESULT hresult = g_zVideo_pDDPalette->SetEntries(
        0,
        firstEntry,
        entryCount,
        entries
    );
    if (hresult == DD_OK) {
        return 0;
    }

    ReportError(
        (int)(hresult),
        g_zVideo_SourceFile_ZvidDdC,
        0x823
    );
    return 0x5a56ffff;
}

} // namespace zVideo_dd

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: return the cached number of accepted DirectDraw hardware API records.
 *
 * Evidence: BN emits a single load from g_zVideo_NumAcceptedDirectDrawDevices
 * at 0x632f98; EnumDirectDrawDeviceCallback is the only writer.
 */
int __cdecl GetAcceptedDirectDrawDeviceCountCached() {
    return g_zVideo_NumAcceptedDirectDrawDevices;
}

} // namespace zVideo_dd

namespace zVid {

/**
 * Purpose: provide the recovered zVid::GetAcceptedHardwareRendererCount_Cached behavior.
 */
int __cdecl GetAcceptedHardwareRendererCount_Cached() {
    return g_zVid_AcceptedHardwareRendererCount;
}

} // namespace zVid

namespace zVideo_dd {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: return the DirectDraw hardware API feature flags for the indexed
 * device record.
 *
 * Evidence: BN indexes g_zVideo_HwApiDeviceTable with the 0x6ec-byte
 * zVidHwApiDeviceRecord stride and returns the m_deviceFeatureFlags field
 * without side effects. The table data owner remains blocked separately.
 */
int __fastcall GetHwApiDeviceFeatureFlags(
    int deviceIndex
) {
    return g_zVideo_HwApiDeviceTable[deviceIndex].m_deviceFeatureFlags;
}

} // namespace zVideo_dd

namespace zVid {

/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zvid_ddd3d.c.
 * Purpose: return the selected Direct3D device name or the writable default
 * device name when no D3D device record is selected.
 */
char *__cdecl GetSelectedD3DDeviceNameOrDefault() {
    return g_zVideo_pSelectedD3DDeviceInfo != 0
               ? g_zVideo_pSelectedD3DDeviceInfo->m_deviceName
               : g_zVideo_DefaultD3DDeviceName;
}

} // namespace zVid

namespace zVid {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: queries live or cached DirectDraw video memory totals and free bytes for a device.
 */
int __fastcall QueryDeviceVideoMemoryBytes(
    int deviceIndexOrMinus1,
    int *totalBytes,
    int *freeBytes
) {
    if (g_zVideo_RendererType == 0) {
        *freeBytes = 0;
        *totalBytes = 0;
        return 0;
    }

    if (deviceIndexOrMinus1 == -1) {
        DDSCAPS caps = {0};
        caps.dwCaps = DDSCAPS_VIDEOMEMORY;
        if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
                &caps,
                (DWORD *)totalBytes,
                (DWORD *)freeBytes
            ) == DD_OK) {
            *freeBytes -= g_zVideo_pSelectedHwApiDeviceRecord->m_textureMemTotalBytes;
        } else {
            *freeBytes = 0;
            *totalBytes = 0;
        }
        return 1;
    }

    const zVidHwApiDeviceRecordPartial &device = g_zVideo_HwApiDeviceTable[deviceIndexOrMinus1];
    *totalBytes = device.m_videoMemTotalBytes;
    if (device.m_videoMemTotalBytes == device.m_textureMemTotalBytes) {
        *freeBytes = device.m_videoMemFreeBytes - 0x1f4000;
    } else {
        *freeBytes = device.m_videoMemFreeBytes - device.m_textureMemTotalBytes;
    }

    return 1;
}

} // namespace zVid

namespace zVid {

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zVideo\zvid_dd.c.
 * Purpose: queries live or cached DirectDraw texture memory totals and free bytes for a device.
 */
int __fastcall QueryTextureMemoryBytes(
    int deviceIndexOrMinus1,
    int *totalBytes,
    int *freeBytes
) {
    if (g_zVideo_pDirectDraw2 == 0) {
        *freeBytes = 0;
        *totalBytes = 0;
        return 0;
    }

    if (deviceIndexOrMinus1 == -1) {
        DDSCAPS caps = {0};
        caps.dwCaps = DDSCAPS_TEXTURE;
        if (g_zVideo_pDirectDraw2->GetAvailableVidMem(
                &caps,
                (DWORD *)totalBytes,
                (DWORD *)freeBytes
            ) != DD_OK) {
            *freeBytes = 0;
            *totalBytes = 0;
        }
        return 1;
    }

    const zVidHwApiDeviceRecordPartial &device = g_zVideo_HwApiDeviceTable[deviceIndexOrMinus1];
    *totalBytes = device.m_textureMemTotalBytes;
    *freeBytes = device.m_textureMemFreeBytes;
    return 1;
}

} // namespace zVid

/* RECOIL_ZVID_DD_ORDER_INSERT */
