#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/recoil_app.h"

struct HudUiContainer;

struct RECOIL_NOVTABLE RecoilStateDialogHost : RecoilApp_IState {
    /**
     * Original helper evidence: no standalone retail function is assigned to
     * this inline dialog-host destructor declaration in the credits order slice.
     * Purpose: provide the virtual destructor declaration for hosted app states.
     */
    virtual ~RecoilStateDialogHost() {}
    virtual void OnWndActivate(int activateCode);
    virtual void OnEnter();
    virtual int OnTryBecomeCurrent();
    virtual int OnUpdateShouldQuit();
    virtual void OnExit();
    virtual void OnDeactivate();
    virtual void OnSuspend(int suspendParam);
    virtual void OnResume(int resumeParam);
    virtual int OnIdleOrDispatch(
        unsigned int wParam,
        unsigned int lParam
    );

    HudUiContainer *m_dialog;
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateDialogHost) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateDialogHost,
        m_dialog
    ) == 0x04
);
