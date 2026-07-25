#include "Battlesport/recoil_version.h"

/**
 * @recoil-anchor recoil:anchor:battlesport-version-g-recoilapp-versionstring
 * @recoil-artifact defines .data recoil:data:0x4dd1d4: g_RecoilApp_VersionString.
 *
 * Purpose: keep the retail app-shell version string in named initialized
 * storage so callers share the original mutable .data symbol.
 */
char g_RecoilApp_VersionString[4] = "1.0";

namespace RecoilVersion {
/**
 * @recoil-anchor recoil:anchor:battlesport-version-recoilversion-getstring
 * @recoil-artifact defines .text recoil:function:0x438980: RecoilVersion::GetString.
 *
 * Purpose: return the fixed retail version string used by the shell and about
 * dialog paths.
 */
const char *GetString() {
    return g_RecoilApp_VersionString;
}
} // namespace RecoilVersion
