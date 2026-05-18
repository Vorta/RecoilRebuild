#include "zutil.h"

namespace zUtil {
// Reimplements 0x4826a0: zUtil::StoreInt32
// (D:\Proj\Battlesport\zUtil\zutil.c)
RECOIL_NOINLINE void RECOIL_FASTCALL StoreInt32(int *outValue, int value) {
    *outValue = value;
}
} // namespace zUtil
