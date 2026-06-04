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

RECOIL_NO_GS void __fastcall ProbePlatformAndVideoCaps(
    zSysVideoCapsLevel *outVideoCaps,
    zSysPlatformCapsLevel *outPlatformCaps
);

RECOIL_NO_GS char *__fastcall FindFileOnDriveType(
    int driveType,
    const char *relativePath,
    int unused
);

int CheckCpuSignatureMask();
int HasCpuidSupportRuntimeOptions();
unsigned short HasCpuidSupport();
int DetectCpuClassAndFeatures();
int ReadCpuidVendorAndFamily();
unsigned int ReadCpuidFeatureFlags();
unsigned int ReadCmosRtcSecondsBcd();
void __fastcall ReadTsc64(
    unsigned int *outHigh,
    unsigned int *outLow
);
void __fastcall Sub64(
    unsigned int subHigh,
    unsigned int subLow,
    unsigned int minuendHigh,
    unsigned int minuendLow,
    unsigned int *outHigh,
    unsigned int *outLow
);
int GetCpuClass();
RECOIL_NO_GS int GetCpuMhz();
int ReturnZeroStub();
RECOIL_NO_GS unsigned int GetTotalPhysKb();
void __fastcall ExitProcessWithCleanup(int exitCode);
} // namespace zSys

namespace zCpu {
int HasMmxSupport();
}
