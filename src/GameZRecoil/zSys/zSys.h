#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

enum zSysVideoCapsLevel {
    ZSYS_VIDEO_CAPS_NONE = 0,
    ZSYS_VIDEO_CAPS_DDRAW = 0x100,
    ZSYS_VIDEO_CAPS_DDRAW2 = 0x200,
    ZSYS_VIDEO_CAPS_DDRAW2_DINPUT = 0x300,
    ZSYS_VIDEO_CAPS_SURFACE3 = 0x500,
    ZSYS_VIDEO_CAPS_SURFACE4 = 0x600,
};

enum zSysPlatformCapsLevel {
    ZSYS_PLATFORM_CAPS_UNSUPPORTED = 0,
    ZSYS_PLATFORM_CAPS_NON_NT = 1,
    ZSYS_PLATFORM_CAPS_NT4_PLUS = 2,
};

namespace zSys {
struct CpuBenchmarkResult {
    unsigned int totalCycles;
    unsigned int totalMicroseconds;
    unsigned int cpuMhzRaw;
    unsigned int cpuMhzRounded;
};

RECOIL_NO_GS void RECOIL_FASTCALL ProbePlatformAndVideoCaps(zSysVideoCapsLevel *outVideoCaps,
                                                            zSysPlatformCapsLevel *outPlatformCaps);

RECOIL_NOINLINE RECOIL_NO_GS char *RECOIL_FASTCALL FindFileOnDriveType(int driveType,
                                                                       const char *relativePath,
                                                                       int unused);

RECOIL_NOINLINE int RECOIL_CDECL CheckCpuSignatureMask();
RECOIL_NOINLINE int RECOIL_CDECL HasCpuidSupportRuntimeOptions();
RECOIL_NOINLINE int RECOIL_CDECL HasCpuidSupport();
RECOIL_NOINLINE int RECOIL_CDECL DetectCpuClassAndFeatures();
RECOIL_NOINLINE int RECOIL_CDECL ReadCpuidVendorAndFamily();
RECOIL_NOINLINE unsigned int RECOIL_CDECL ReadCpuidFeatureFlags();
RECOIL_NOINLINE unsigned int RECOIL_CDECL ReadCmosRtcSecondsBcd();
RECOIL_NOINLINE void RECOIL_FASTCALL ReadTsc64(unsigned int *outHigh, unsigned int *outLow);
RECOIL_NOINLINE void RECOIL_FASTCALL Sub64(unsigned int subHigh, unsigned int subLow,
                                           unsigned int minuendHigh, unsigned int minuendLow,
                                           unsigned int *outHigh, unsigned int *outLow);
RECOIL_NOINLINE int RECOIL_CDECL GetCpuClass();
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_CDECL GetCpuMhz();
RECOIL_NOINLINE int RECOIL_CDECL ReturnZeroStub();
RECOIL_NOINLINE RECOIL_NO_GS unsigned int RECOIL_CDECL GetTotalPhysKb();
RECOIL_NOINLINE void RECOIL_FASTCALL ExitProcessWithCleanup(int exitCode);
} // namespace zSys

namespace zCpu {
RECOIL_NOINLINE int RECOIL_CDECL HasMmxSupport();
}
