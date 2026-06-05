#include "Battlesport/RecoilVersion.h"

namespace RecoilVersion {
/**
 * Reimplements 0x438980: RecoilVersion::GetString.
 *
 * Purpose: return the fixed retail version string used by the shell and about
 * dialog paths.
 */
const char *GetString() {
    return "1.0";
}
} // namespace RecoilVersion
