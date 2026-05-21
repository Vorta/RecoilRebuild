#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "Battlesport/RecoilApp.h"
#include "recoil/recoil_callconv.h"

struct HudUiDialogController;
struct zVidRect32;
struct zOpt_ViewRectSection;

struct RecoilStateDialogHost {
    RecoilPtr32 vftable;            // RecoilStateDialogHost_Vtbl*
    volatile RecoilPtr32 dialog_04; // HudUiDialogController*
    int stateValue_08;
    RecoilPtr32 statePtr_0C;

    void RECOIL_THISCALL OnWndActivate(int activateCode);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateDialogHost) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateDialogHost, dialog_04) == 0x04);

struct HudUiDialogController {
    RecoilPtr32 vftable; // HudUiDialogController_Vtbl*
    unsigned char _padding_04[0x110];
    RecoilPtr32 capturedImage_114; // zVid_Image*

    void RECOIL_THISCALL BlitOwnedSurfaceToPrimary();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiDialogController) == 0x118);
RECOIL_STATIC_ASSERT(offsetof(HudUiDialogController, capturedImage_114) == 0x114);

struct HudUiDialogController_Vtbl {
    RecoilFn32 Update;
    RecoilFn32 SetEnabled;
};
RECOIL_STATIC_ASSERT(sizeof(HudUiDialogController_Vtbl) == 0x08);

struct HudUiDialogControllerVirtual {
    virtual void RECOIL_THISCALL Update();
    virtual void RECOIL_THISCALL SetEnabled(int enabled);
};
RECOIL_STATIC_ASSERT(sizeof(HudUiDialogControllerVirtual) == 0x04);

namespace zVideo {
int RECOIL_CDECL RunPostprocessOnPrimaryBuffer();
int RECOIL_CDECL Dispatch_UnlockPrimarySurfaceState();
int RECOIL_FASTCALL AdjustSurfacesIfEnabled(zVidRect32 *srcRect,
                                            zVidRect32 *dstRect,
                                            int waitForPresent,
                                            int blitPrimaryToSwFirst);
} // namespace zVideo

namespace zOpt {
zOpt_ViewRectSection *RECOIL_CDECL GetWindowSection();
}
