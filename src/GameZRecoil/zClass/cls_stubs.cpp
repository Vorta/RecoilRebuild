#include "GameZRecoil/zClass/cls_stubs.h"

// Reimplements 0x407130: zStub::ReturnOneNoArgs
// (GameZRecoil/zClass/cls_stubs.c)
int RECOIL_THISCALL zStub::ReturnOneNoArgs() {
    return 1;
}

// Reimplements 0x407140: zStub::ReturnZeroNoArgs
// (GameZRecoil/zClass/cls_stubs.c)
int RECOIL_THISCALL zStub::ReturnZeroNoArgs() {
    return 0;
}

// Reimplements 0x407150: zStub::NoOp1Arg
// (GameZRecoil/zClass/cls_stubs.c)
void RECOIL_THISCALL zStub::NoOp1Arg(int) {
}

// Reimplements 0x407160: zStub::ReturnOne2Args
// (GameZRecoil/zClass/cls_stubs.c)
int RECOIL_THISCALL zStub::ReturnOne2Args(int, int) {
    return 1;
}
