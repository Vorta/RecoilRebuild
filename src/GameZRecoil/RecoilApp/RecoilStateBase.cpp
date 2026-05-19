#include "GameZRecoil/RecoilApp/RecoilStateBase.h"


// Reimplements 0x407170: RecoilStateBase::ScalarDeletingDestructor
RecoilApp_IState *RECOIL_THISCALL RecoilStateBase::ScalarDeletingDestructor(unsigned int flags) {
    vftable = kRecoilStateBase_VtblAddress;

    if ((flags & 1) != 0) {
        ::operator delete(static_cast<void *>(this));
    }

    return (RecoilApp_IState *)(this);
}
