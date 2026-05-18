#include "Battlesport/RecoilVersion.h"

namespace RecoilVersion {
// Reimplements 0x438980: RecoilVersion::GetString
RECOIL_NOINLINE const char *RECOIL_CDECL GetString() {
    return "1.0";
}
} // namespace RecoilVersion
