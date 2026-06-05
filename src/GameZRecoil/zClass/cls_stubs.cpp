#include "GameZRecoil/zClass/cls_stubs.h"

/**
 * Reimplements 0x407130: zStub::ReturnOneNoArgs
 * (GameZRecoil/zClass/cls_stubs.c).
 *
 * Purpose: provide a generic zClass vtable stub that returns success.
 */
int zStub::ReturnOneNoArgs() {
    return 1;
}

/**
 * Reimplements 0x407140: zStub::ReturnZeroNoArgs
 * (GameZRecoil/zClass/cls_stubs.c).
 *
 * Purpose: provide a generic zClass vtable stub that returns failure or empty
 * state.
 */
int zStub::ReturnZeroNoArgs() {
    return 0;
}

/**
 * Reimplements 0x407150: zStub::NoOp1Arg
 * (GameZRecoil/zClass/cls_stubs.c).
 *
 * Purpose: provide a generic one-argument zClass vtable stub with no side
 * effects.
 */
void zStub::NoOp1Arg(
    int
) {}

/**
 * Reimplements 0x407160: zStub::ReturnOne2Args
 * (GameZRecoil/zClass/cls_stubs.c).
 *
 * Purpose: provide a generic two-argument zClass vtable stub that returns
 * success.
 */
int zStub::ReturnOne2Args(
    int,
    int
) {
    return 1;
}
