#include "GameZRecoil/RecoilApp/RecoilStateBase.h"

/**
 * Reimplements 0x407170: RecoilStateBase::ScalarDeletingDestructor.
 *
 * Purpose: reset the state object to the base state table and optionally free
 * the object for VC scalar-deleting-destructor callers.
 */
RecoilApp_IState * RecoilStateBase::ScalarDeletingDestructor(
    unsigned int flags
) {
    vftable = RecoilSymbolPtr32(&g_RecoilStateBase_Vtbl);

    if ((flags & 1) != 0) {
        ::operator delete((void *)(this));
    }

    return (RecoilApp_IState *)(this);
}
