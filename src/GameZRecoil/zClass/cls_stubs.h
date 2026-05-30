#pragma once

#include "recoil/recoil_callconv.h"

struct zStub {
    int RECOIL_THISCALL ReturnOneNoArgs();
    int RECOIL_THISCALL ReturnZeroNoArgs();
    void RECOIL_THISCALL NoOp1Arg(int);
    int RECOIL_THISCALL ReturnOne2Args(int, int);
};
