#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/RecoilApp.h"
#include "recoil/recoil_callconv.h"

struct HudUiDialogController;
struct zVidRect32;
struct zOpt_ViewRectSection;

struct RecoilStateDialogHost : RecoilApp_IState {
    HudUiDialogController *m_dialog;
    int m_stateValue;
    RecoilApp_IState *m_state;

    void OnWndActivate(int activateCode);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateDialogHost) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateDialogHost,
        m_dialog
    ) == 0x04
);

struct HudUiDialogController {
    virtual void Update(float deltaSeconds);
    virtual void SetEnabled(int enabled);

    unsigned char _padding_04[0x110];
    void *m_capturedImage;

    void BlitOwnedSurfaceToPrimary();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiDialogController) == 0x118);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiDialogController,
        m_capturedImage
    ) == 0x114
);

namespace zVideo {
int RunPostprocessOnPrimaryBuffer();
int Dispatch_UnlockPrimarySurfaceState();
int __fastcall AdjustSurfacesIfEnabled(
    zVidRect32 *srcRect,
    zVidRect32 *dstRect,
    int waitForPresent,
    int blitPrimaryToSwFirst
);
} // namespace zVideo

namespace zOpt {
zOpt_ViewRectSection *GetWindowSection();
}
