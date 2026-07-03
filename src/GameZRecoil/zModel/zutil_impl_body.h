#include "GameZRecoil/zUtil/zutil.h"

namespace zUtil {
/**
 * Reimplements 0x4826a0: zUtil::StoreInt32.
 * Purpose: Stores the supplied 32-bit integer through the destination pointer.
 */
void __fastcall StoreInt32(
    int *outValue,
    int value
) {
    *outValue = value;
}
} // namespace zUtil
