#include "GameZRecoil/zLoc/zLoc.h"

#include <cstring>

namespace {
std::uint32_t RECOIL_CDECL TestGetMessageIdProc(const char *key) {
    return std::strcmp(key, "known") == 0 ? 77u : 0u;
}
} // namespace

extern "C" int zloc_message_lookup_failure_smoke(void) {
    g_zLoc_MessagesDllHandle = nullptr;
    g_zLoc_GetIdProc = nullptr;
    std::memset(g_zLoc_TempMessageBuffer, 0x5a, sizeof(g_zLoc_TempMessageBuffer));

    char buffer[16] = {};
    const std::uint32_t result = zLoc::FormatMessage(buffer, sizeof(buffer), 1);
    char fallback[] = "missing";
    const bool noProc = zLoc::GetMessageId(fallback) == 0 &&
                        zLoc::ResolveMessageKeyOrFallback(fallback) == fallback;
    g_zLoc_GetIdProc = TestGetMessageIdProc;
    const bool proc = zLoc::GetMessageId("known") == 77 && zLoc::GetMessageId("missing") == 0;
    g_zLoc_GetIdProc = nullptr;

    return noProc && proc && result == 0 && zLoc::GetMessageString(1) == nullptr &&
                   g_zLoc_TempMessageBuffer[0] == static_cast<char>(0x5a)
               ? 0
               : 1;
}

extern "C" int zloc_load_unload_messages_dll_smoke(void) {
    zLoc::UnloadMessagesDll();

    if (zLoc::LoadMessagesDll("definitely_missing_recoil_messages.dll") != 0 ||
        g_zLoc_MessagesDllHandle != nullptr) {
        return 1;
    }

    if (zLoc::LoadMessagesDll("kernel32.dll") != 1 || g_zLoc_MessagesDllHandle == nullptr) {
        return 2;
    }

    zLoc::UnloadMessagesDll();
    return g_zLoc_MessagesDllHandle == nullptr ? 0 : 3;
}
