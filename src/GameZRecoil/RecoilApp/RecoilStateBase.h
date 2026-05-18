#pragma once

#include "recoil/recoil_types.h"

#include "Battlesport/RecoilApp.h"
#include "recoil/recoil_callconv.h"

struct RecoilStateBase {
    RecoilPtr32 vftable; // RecoilStateBase_Vtbl*

    RecoilApp_IState *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateBase) == 0x04);
