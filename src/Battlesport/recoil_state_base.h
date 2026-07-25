#pragma once

#include "recoil/recoil_types.h"

/**
 * Provisional byte-match model for the unresolved 0x407170 / 0x4ccd50
 * default state table. This stays separate from RecoilApp_IState and is not
 * owner, tier, or source-block acceptance evidence.
 * @recoil-anchor recoil:anchor:battlesport.recoil-state-base.type
 * @recoil-artifact emits .text recoil:function:0x407170: VC5 compiler-generated scalar deleting-destructor contribution for this provisional complete type when its inline destructor definition is present; not an authored body.
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
