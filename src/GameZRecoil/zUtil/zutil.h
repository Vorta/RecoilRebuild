#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

#ifdef __cplusplus
namespace zUtil {
extern "C" {
#endif
void __fastcall StoreInt32(int* outValue, int value);
#ifdef __cplusplus
}
}
#endif
