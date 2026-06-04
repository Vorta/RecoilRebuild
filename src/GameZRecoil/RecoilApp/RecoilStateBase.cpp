#include "GameZRecoil/RecoilApp/RecoilStateBase.h"

// Reimplements 0x407170: RecoilStateBase::ScalarDeletingDestructor
RecoilApp_IState * RecoilStateBase::ScalarDeletingDestructor(
    unsigned int flags
) {
    vftable = RecoilSymbolPtr32(&g_RecoilStateBase_Vtbl);

    if ((flags & 1) != 0) {
        ::operator delete((void *)(this));
    }

    return (RecoilApp_IState *)(this);
}
