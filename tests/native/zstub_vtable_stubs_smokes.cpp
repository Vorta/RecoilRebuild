#include "GameZRecoil/zClass/cls_stubs.h"

extern "C" int zstub_vtable_stubs_smoke(void) {
    zStub stub;

    if (stub.ReturnOneNoArgs() != 1) {
        return 1;
    }
    if (stub.ReturnZeroNoArgs() != 0) {
        return 2;
    }

    stub.NoOp1Arg(0x12345678);

    if (stub.ReturnOne2Args(0x11111111, -7) != 1) {
        return 3;
    }

    return 0;
}
