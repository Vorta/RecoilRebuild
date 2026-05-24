#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zSys/zSys.h"

#include <windows.h>

#include <cstdio>

extern "C" HWND g_RecoilError_OutputHWnd;
extern "C" std::int32_t g_RecoilError_OutputMaxBytes;
extern "C" std::int32_t g_RecoilError_OutputByteCount;
extern "C" char g_zError_DebugMsgBuffer[1024];

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
    const std::int32_t hasCpuSignatureMask = zSys::CheckCpuSignatureMask();
    return hasCpuid == 1 && (hasMmx == 0 || hasMmx == 1) &&
                   (hasCpuSignatureMask == 0 || hasCpuSignatureMask == 1)
               ? 0
               : 1;
}

extern "C" int zsys_cpu_leaf_helpers_smoke(void) {
    const unsigned int secondBcd = zSys::ReadCmosRtcSecondsBcd();
    const unsigned int lowNibble = secondBcd & 0x0f;
    const unsigned int highNibble = (secondBcd >> 4) & 0x0f;

    unsigned int firstHigh = 0;
    unsigned int firstLow = 0;
    unsigned int secondHigh = 0;
    unsigned int secondLow = 0;
    zSys::ReadTsc64(&firstHigh, &firstLow);
    zSys::ReadTsc64(&secondHigh, &secondLow);

    unsigned int diffHigh = 0;
    unsigned int diffLow = 0;
    zSys::Sub64(firstHigh, firstLow, secondHigh, secondLow, &diffHigh, &diffLow);

    unsigned int borrowHigh = 0;
    unsigned int borrowLow = 0;
    zSys::Sub64(0, 1, 1, 0, &borrowHigh, &borrowLow);

    return highNibble <= 5 && lowNibble <= 9 && (secondHigh > firstHigh ||
                                                 (secondHigh == firstHigh && secondLow >= firstLow)) &&
                   (diffHigh != 0 || diffLow != 0 || secondLow == firstLow) &&
                   borrowHigh == 0 && borrowLow == 0xffffffffu
               ? 0
               : 1;
}

extern "C" int zsys_exit_process_with_cleanup_child_smoke(void) {
    char enabled[2] = {};
    if (GetEnvironmentVariableA("RECOIL_EXIT_PROCESS_CHILD", enabled, sizeof(enabled)) == 0 ||
        enabled[0] != '1') {
        return 0;
    }

    zSys::ExitProcessWithCleanup(37);
    return 1;
}

extern "C" int zsys_exit_process_with_cleanup_smoke(void) {
    char exePath[MAX_PATH] = {};
    if (GetModuleFileNameA(nullptr, exePath, sizeof(exePath)) == 0) {
        return 1;
    }

    char commandLine[MAX_PATH + 80] = {};
    if (std::snprintf(commandLine, sizeof(commandLine),
                      "\"%s\" zsys_exit_process_with_cleanup_child_smoke",
                      exePath) <= 0) {
        return 2;
    }

    STARTUPINFOA startupInfo = {};
    PROCESS_INFORMATION processInfo = {};
    startupInfo.cb = sizeof(startupInfo);
    SetEnvironmentVariableA("RECOIL_EXIT_PROCESS_CHILD", "1");
    const BOOL created = CreateProcessA(nullptr, commandLine, nullptr, nullptr, FALSE, 0, nullptr,
                                        nullptr, &startupInfo, &processInfo);
    SetEnvironmentVariableA("RECOIL_EXIT_PROCESS_CHILD", nullptr);
    if (created == 0) {
        return 3;
    }

    const DWORD waitResult = WaitForSingleObject(processInfo.hProcess, 10000);
    DWORD exitCode = 0;
    const BOOL gotExitCode = GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    if (waitResult != WAIT_OBJECT_0 || gotExitCode == 0) {
        return 4;
    }

    return exitCode == 37 ? 0 : 5;
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

extern "C" int zerror_emit_debug_buffer_smoke(void) {
    g_zError_DebugMsgBuffer[0] = 'x';
    g_zError_DebugMsgBuffer[1] = '\0';
    zError::EmitDebugBuffer(5);
    return g_zError_DebugMsgBuffer[0] == 'x' && g_zError_DebugMsgBuffer[1] == '\0' ? 0 : 1;
}
