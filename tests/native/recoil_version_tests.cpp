#include "Battlesport/RecoilVersion.h"

#include <cstring>

extern "C" int recoil_version_get_string_smoke(void) {
    const char *const version = RecoilVersion::GetString();
    return version != 0 && std::strcmp(version, "1.0") == 0 ? 0 : 1;
}
