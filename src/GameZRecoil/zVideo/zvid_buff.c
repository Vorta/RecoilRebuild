#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zVideo/zvid.h"

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-buff-g-zvideo-sourcefile-zvidbuffc
 * @recoil-artifact defines .data recoil:data:0x4e3054: g_zVideo_SourceFile_ZvidBuffC.
 * Purpose: supply the original zvid_buff.c path used by this translation
 * unit's DirectDraw failure diagnostic.
 */
char g_zVideo_SourceFile_ZvidBuffC[0x27] = "D:\\Proj\\GameZRecoil\\zVideo\\zvid_buff.c";

RECOIL_STATIC_ASSERT(sizeof(g_zVideo_SourceFile_ZvidBuffC) == 0x27);

namespace zVideo_buff
{

    /**
     * Purpose: provide the recovered zVideo_buff::ClipCoordToRange behavior.
     */
    int __fastcall ClipCoordToRange(int* coordPtr, int minCoord, int maxCoord)
    {
        const int coord = *coordPtr;
        int clipped = 0;
        if (coord < minCoord) {
            clipped = coord - minCoord;
            *coordPtr = minCoord;
        } else if (coord > maxCoord) {
            clipped = coord - maxCoord;
            *coordPtr = maxCoord;
        }

        return clipped;
    }

    /**
     * Purpose: provide the recovered zVideo_buff::BltSourceToPrimaryClipped behavior.
     */
    void __fastcall BltSourceToPrimaryClipped(
        zVidImagePartial * srcImage,
        int dstX,
        int dstY,
        int srcColorKeyEnable,
        zVidRect32* srcRect
    )
    {
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
        dstRectLocal.left = dstX;
        dstRectLocal.top = dstY;
        dstRectLocal.right = srcRectLocal.right - srcRectLocal.left + dstX;
        dstRectLocal.bottom = srcRectLocal.bottom - srcRectLocal.top + dstY;

        int clipped = ClipCoordToRange(&dstRectLocal.left, 0, g_zVideo_PrimarySurfaceState.width - 1);
        if (clipped < 0) {
            srcRectLocal.left -= clipped;
        } else if (clipped > 0) {
            return;
        }

        clipped = ClipCoordToRange(&dstRectLocal.right, 0, g_zVideo_PrimarySurfaceState.width);
        if (clipped < 0) {
            return;
        }
        if (clipped > 0) {
            srcRectLocal.right -= clipped;
        }

        clipped = ClipCoordToRange(&dstRectLocal.top, 0, g_zVideo_PrimarySurfaceState.height - 1);
        if (clipped < 0) {
            srcRectLocal.top -= clipped;
        } else if (clipped > 0) {
            return;
        }

        clipped = ClipCoordToRange(&dstRectLocal.bottom, 0, g_zVideo_PrimarySurfaceState.height);
        if (clipped < 0) {
            return;
        }
        if (clipped > 0) {
            srcRectLocal.bottom -= clipped;
        }

        const DWORD bltFlags = DDBLT_WAIT | DDBLT_ASYNC | ((srcImage->formatFlagsPacked & 0x02u) << 14);
        if (g_zVideo_PrimarySurfaceState.surf == 0) {
            return;
        }

        const int wasLocked = g_zVideo_PrimarySurfaceState.locked;
        if (wasLocked != 0) {
            zVideo_dd::UnlockSurfaceState(&g_zVideo_PrimarySurfaceState);
        }

        const HRESULT hresult = g_zVideo_PrimarySurfaceState.surf
                                    ->Blt((RECT*)&dstRectLocal, srcImage->surface, (RECT*)&srcRectLocal, bltFlags, 0);

        if (wasLocked != 0) {
            zVideo_dd::LockSurfaceState(&g_zVideo_PrimarySurfaceState);
        }

        if (hresult != DD_OK) {
            zVideo_dd::ReportError((int)(hresult), ::g_zVideo_SourceFile_ZvidBuffC, 0x150);
        }
    }

} // namespace zVideo_buff
