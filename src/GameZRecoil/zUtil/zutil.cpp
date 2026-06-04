#include "zutil.h"

namespace zUtil {
// Reimplements 0x4826a0: zUtil::StoreInt32
// (D:\Proj\GameZRecoil\zUtil\zutil.c)
void __fastcall StoreInt32(
    int *outValue,
    int value
) {
    *outValue = value;
}
} // namespace zUtil
