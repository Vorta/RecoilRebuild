#pragma once

#include "recoil/recoil_types.h"

/**
 * Provisional byte-match model for the unresolved 0x407170 / 0x4ccd50
 * default state table. This stays separate from RecoilApp_IState and is not
 * owner, tier, or source-block acceptance evidence.
 */
struct RecoilStateBase {
    virtual ~RecoilStateBase();
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
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateBase) == 0x04);
