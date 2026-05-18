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

    return 0;
}
