#pragma once

#include "recoil/recoil_callconv.h"

struct CZStub {
    int ReturnOneNoArgs();
    int ReturnZeroNoArgs();
    void NoOp1Arg(int);
    int ReturnOne2Args(int, int);
};
