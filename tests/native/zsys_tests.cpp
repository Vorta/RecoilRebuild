#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zSys/zSys.h"

#include <windows.h>

extern "C" HWND g_RecoilError_OutputHWnd;
extern "C" std::int32_t g_RecoilError_OutputMaxBytes;
extern "C" std::int32_t g_RecoilError_OutputByteCount;

extern "C" int zsys_find_file_on_drive_type_negative_smoke(void) {
    char *result =
        zSys::FindFileOnDriveType(DRIVE_FIXED, "__recoil_missing_file_for_drive_search__.tmp", 0);
    return result == nullptr ? 0 : 1;
}

extern "C" int zsys_runtime_probe_leaves_smoke(void) {
    const std::int32_t cpuClass = zSys::GetCpuClass() & 0xffff;
    const std::int32_t cpuMhz = zSys::GetCpuMhz();
    return cpuClass >= 3 && cpuMhz >= 0 && zSys::ReturnZeroStub() == 0 &&
                   zSys::GetTotalPhysKb() != 0
               ? 0
               : 1;
}

extern "C" int zsys_cpuid_mmx_smoke(void) {
    const std::int32_t hasCpuid = zSys::HasCpuidSupport();
    const std::int32_t hasMmx = zCpu::HasMmxSupport();
    return hasCpuid == 1 && (hasMmx == 0 || hasMmx == 1) ? 0 : 1;
}

extern "C" int zerror_init_output_context_smoke(void) {
    g_RecoilError_OutputHWnd = nullptr;
    g_RecoilError_OutputMaxBytes = 0;
    g_RecoilError_OutputByteCount = 7;

    HWND hwnd = reinterpret_cast<HWND>(0x1234);
    return zError::InitOutputContext(hwnd, 0xe00, "recoil.err") == 0 &&
                   g_RecoilError_OutputHWnd == hwnd && g_RecoilError_OutputMaxBytes == 0xe00 &&
                   g_RecoilError_OutputByteCount == 0
               ? 0
               : 1;
}
