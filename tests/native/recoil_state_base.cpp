#include "GameZRecoil/RecoilApp/RecoilStateBase.h"

extern "C" int recoil_state_base_scalar_deleting_destructor_smoke(void) {
    RecoilStateBase state{};
    RecoilApp_IState *const returned = state.ScalarDeletingDestructor(0);

    if (returned != reinterpret_cast<RecoilApp_IState *>(&state)) {
        return 1;
    }

    if (state.vftable != kRecoilStateBase_VtblAddress) {
        return 2;
    }

    RecoilStateBase *const heapState = new RecoilStateBase{0x12345678};
    RecoilApp_IState *const heapReturned = heapState->ScalarDeletingDestructor(1);
    if (heapReturned != reinterpret_cast<RecoilApp_IState *>(heapState)) {
        return 3;
    }

    return 0;
}
