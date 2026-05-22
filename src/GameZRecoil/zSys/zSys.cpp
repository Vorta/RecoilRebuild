#define DIRECTINPUT_VERSION 0x0500
#if defined(_MSC_VER)
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 0
#endif

#include "GameZRecoil/zSys/zSys.h"

#include "GameZRecoil/zGame/zGame.h"

#include <stdio.h>
#include <string.h>
#include <intrin.h>
#include <limits>
#include <sys/stat.h>

#include <ddraw.h>
#include <windows.h>

#include <stdio.h>

#if defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif

namespace {
typedef HRESULT(WINAPI *zDirectDrawCreateFn)(GUID *, LPDIRECTDRAW *, IUnknown *);
typedef HMODULE(RECOIL_STDCALL *zLoadLibraryAFn)(const char *);

const char kDdrawDll[] = "DDRAW.DLL";
const char kDinputDll[] = "DINPUT.DLL";
const char kDirectDrawCreateName[] = "DirectDrawCreate";
const char kDirectInputCreateName[] = "DirectInputCreateA";

const char kCouldNotCreateDDraw[] = "Couldn't create DDraw\r\n";
const char kCouldNotCreateSurface[] = "Couldn't CreateSurface\r\n";
const char kCouldNotGetDinputCreate[] = "Couldn't GetProcAddress DInputCreate\r\n";
const char kCouldNotLoadDDraw[] = "Couldn't LoadLibrary DDraw\r\n";
const char kCouldNotLoadDinput[] = "Couldn't LoadLibrary DInput\r\n";
const char kCouldNotQiDDraw2[] = "Couldn't QI DDraw2\r\n";
const char kCouldNotSetCoopLevel[] = "Couldn't Set coop level\r\n";

const unsigned int kCpuBenchmarkDurationTable[0x0c] = {0,    0,    0,    0x73, 0x2f, 0x2b,
                                                            0x26, 0x26, 0x26, 0x26, 0x26, 0x26};

char g_zSys_DriveTypeSearchPathBuffer[MAX_PATH];

struct CpuBenchmarkResolver {
    RECOIL_NOINLINE zSys::CpuBenchmarkResult *RECOIL_THISCALL
    ResolveCpuBenchmarkPacket(zSys::CpuBenchmarkResult *outBuffer);

    RECOIL_NOINLINE zSys::CpuBenchmarkResult *RECOIL_THISCALL
    MeasureMhzViaBsfLoop_Qpc(zSys::CpuBenchmarkResult *outBuffer);

    RECOIL_NOINLINE zSys::CpuBenchmarkResult *RECOIL_THISCALL
    MeasureCpuMhz_RdtscQpc(zSys::CpuBenchmarkResult *outBuffer);

    RECOIL_NOINLINE zSys::CpuBenchmarkResult *RECOIL_THISCALL
    MeasureCpuMhz_CmosRtc(zSys::CpuBenchmarkResult *outBuffer);
};

CpuBenchmarkResolver *CpuBenchmarkResolverFromValue(unsigned int value) {
    return (CpuBenchmarkResolver *)(static_cast<unsigned int>(value));
}

unsigned long RecoilBitScanForward(unsigned long value) {
    unsigned long index = 0;
    while (((value >> index) & 1u) == 0u && index < 31u) {
        ++index;
    }
    return index;
}

unsigned __int64 RecoilReadTimestampCounter() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return ((unsigned __int64)counter.HighPart << 32) | counter.LowPart;
}

void ZeroBenchmarkResult(zSys::CpuBenchmarkResult *outBuffer) {
    outBuffer->totalCycles = 0;
    outBuffer->totalMicroseconds = 0;
    outBuffer->cpuMhzRaw = 0;
    outBuffer->cpuMhzRounded = 0;
}

unsigned int DivideRoundedHalfUp(unsigned __int64 numerator, unsigned int denominator) {
    if (denominator == 0) {
        return 0;
    }

    const unsigned int quotient = static_cast<unsigned int>(numerator / denominator);
    const unsigned int remainder = static_cast<unsigned int>(numerator % denominator);
    return remainder > (denominator >> 1) ? quotient + 1 : quotient;
}

unsigned int AbsDiffFromTripleAverage(unsigned int value, unsigned int previous,
                                       unsigned int prior) {
    const int delta = static_cast<int>(value * 3u - value - previous - prior);
    return delta < 0 ? static_cast<unsigned int>(-delta) : static_cast<unsigned int>(delta);
}

unsigned int ReadCmosRtcSecondsBcd() {
    SYSTEMTIME localTime;
    GetLocalTime(&localTime);
    return static_cast<unsigned int>(((localTime.wSecond / 10) << 4) | (localTime.wSecond % 10));
}
} // namespace

extern "C" unsigned int g_zSys_CpuVendorNonIntelMarker = 0;

#include "GameZRecoil/zSys/zSys_probe_platform.inl"

// Reimplements 0x4a59e0: zSys::FindFileOnDriveType
RECOIL_NOINLINE RECOIL_NO_GS char *RECOIL_FASTCALL
zSys::FindFileOnDriveType(int driveType, const char *relativePath, int) {
    char driveStrings[0x100];
    GetLogicalDriveStringsA(sizeof(driveStrings), driveStrings);

    for (const char *drive = driveStrings; *drive != '\0'; drive += strlen(drive) + 1) {
        sprintf(g_zSys_DriveTypeSearchPathBuffer, "%s%s", drive, relativePath);
        const UINT currentType = GetDriveTypeA(drive);
        if (currentType != static_cast<UINT>(driveType)) {
            continue;
        }

        struct stat statBuffer;
        if (stat(g_zSys_DriveTypeSearchPathBuffer, &statBuffer) == 0) {
            return g_zSys_DriveTypeSearchPathBuffer;
        }
    }

    return 0;
}

// Reimplements 0x4b3050: zSys::CheckCpuSignatureMask
RECOIL_NOINLINE int RECOIL_CDECL zSys::CheckCpuSignatureMask() {
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 1);
    return (cpuInfo[0] & 0x630) == 0x630 ? 1 : 0;
}

#if !(defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM))
// Reimplements 0x4b33f0: zSys::HasCpuidSupport
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL zSys::HasCpuidSupport() {
    return 1;
}

// Reimplements 0x4b3640: zSys::ReadCpuidVendorAndFamily
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL zSys::ReadCpuidVendorAndFamily() {
    int cpuInfo[4];
    __cpuid(cpuInfo, 0);
    const char genuineIntel[0x0c] = {'G', 'e', 'n', 'u', 'i', 'n', 'e', 'I', 'n', 't', 'e', 'l'};
    char vendor[0x0c];
    memcpy(&vendor[0], &cpuInfo[1], 4);
    memcpy(&vendor[4], &cpuInfo[3], 4);
    memcpy(&vendor[8], &cpuInfo[2], 4);
    g_zSys_CpuVendorNonIntelMarker =
        memcmp(vendor, genuineIntel, sizeof(vendor)) == 0 ? 0u : 1u;

    __cpuid(cpuInfo, 1);
    int family = (cpuInfo[0] >> 8) & 0x0f;
    const int extendedFamily = (cpuInfo[0] >> 20) & 0xff;
    if (family == 0x0f) {
        family += extendedFamily;
    }

    return family;
}
#endif

// Reimplements 0x4b3480: zSys::ReadCpuidFeatureFlags
RECOIL_NOINLINE unsigned int RECOIL_CDECL zSys::ReadCpuidFeatureFlags() {
    if (HasCpuidSupport() == 0) {
        return 0;
    }

    int cpuInfo[4];
    __cpuid(cpuInfo, 0);

    const char genuineIntel[0x0c] = {'G', 'e', 'n', 'u', 'i', 'n', 'e', 'I', 'n', 't', 'e', 'l'};
    char vendor[0x0c];
    memcpy(&vendor[0], &cpuInfo[1], 4);
    memcpy(&vendor[4], &cpuInfo[3], 4);
    memcpy(&vendor[8], &cpuInfo[2], 4);
    if (memcmp(vendor, genuineIntel, sizeof(vendor)) != 0) {
        g_zSys_CpuVendorNonIntelMarker = 1;
    }

    __cpuid(cpuInfo, 1);
    return static_cast<unsigned int>(cpuInfo[3]);
}

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
#include "GameZRecoil/zSys/zSys_cpu_asm.inl"
#else
namespace zSys {
// Reimplements 0x4b3510: zSys::ProbeDivZeroFlagBehavior
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL ProbeDivZeroFlagBehavior() {
    return 0;
}

// Reimplements 0x4b3550: zSys::DetectIs8086ByEflagsHiBits
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL DetectIs8086ByEflagsHiBits() {
    return 0xffff;
}

// Reimplements 0x4b35a0: zSys::DetectIs80286ByEflagsHiBits
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL DetectIs80286ByEflagsHiBits() {
    return 0xffff;
}

// Reimplements 0x4b35f0: zSys::DetectIs80386ByAcFlag
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL DetectIs80386ByAcFlag() {
    return 0xffff;
}
} // namespace zSys
#endif

#include "GameZRecoil/zSys/zSys_cpu_detect.inl"

// Reimplements 0x4b37f0: CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc
RECOIL_NOINLINE zSys::CpuBenchmarkResult *RECOIL_THISCALL
CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc(zSys::CpuBenchmarkResult *outBuffer) {
    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency) == 0) {
        ZeroBenchmarkResult(outBuffer);
        return outBuffer;
    }

    unsigned int minTicks = 0xffffffffu;
    {
    for (int sample = 0; sample < 10; ++sample) {
        LARGE_INTEGER start;
        LARGE_INTEGER end;
        QueryPerformanceCounter(&start);

        for (unsigned short i = 0x0fa0; i != 1; --i) {
            unsigned long bitIndex = RecoilBitScanForward(0x80000000u);
            (void)bitIndex;
        }

        QueryPerformanceCounter(&end);
        const unsigned int ticks = static_cast<unsigned int>(end.LowPart - start.LowPart);
        if (ticks < minTicks) {
            minTicks = ticks;
        }
    }
    }

    const unsigned int expectedCycles =
        static_cast<unsigned int>((unsigned int)(this));
    const unsigned int microseconds =
        DivideRoundedHalfUp(static_cast<unsigned __int64>(minTicks) * 1000000ui64,
                            static_cast<unsigned int>(frequency.LowPart));
    const unsigned int cpuMhzRaw = microseconds == 0 ? 0 : expectedCycles / microseconds;
    const unsigned int cpuMhzRounded = DivideRoundedHalfUp(expectedCycles, microseconds);

    outBuffer->totalCycles = expectedCycles;
    outBuffer->totalMicroseconds = microseconds;
    outBuffer->cpuMhzRaw = cpuMhzRaw;
    outBuffer->cpuMhzRounded = cpuMhzRounded;
    return outBuffer;
}

// Reimplements 0x4b38e0: CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc
RECOIL_NOINLINE zSys::CpuBenchmarkResult *RECOIL_THISCALL
CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc(zSys::CpuBenchmarkResult *outBuffer) {
    HANDLE thread = GetCurrentThread();

    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency) == 0) {
        ZeroBenchmarkResult(outBuffer);
        return outBuffer;
    }

    unsigned int priorMhz = 0;
    unsigned int previousMhz = 0;
    unsigned int totalCycles = 0;
    unsigned int totalMicroseconds = 0;

    {
    for (int attempt = 1;; ++attempt) {
        const unsigned int olderMhz = previousMhz;
        previousMhz = priorMhz;

        LARGE_INTEGER outerStart;
        LARGE_INTEGER current;
        QueryPerformanceCounter(&outerStart);
        current = outerStart;

        const int oldPriority = GetThreadPriority(thread);
        if (oldPriority != THREAD_PRIORITY_ERROR_RETURN) {
            SetThreadPriority(thread, THREAD_PRIORITY_TIME_CRITICAL);
        }

        while (static_cast<unsigned int>(current.LowPart - outerStart.LowPart) < 0x32) {
            QueryPerformanceCounter(&current);
        }

        const unsigned int startTicks = static_cast<unsigned int>(RecoilReadTimestampCounter());
        LARGE_INTEGER sampleStart = current;
        do {
            QueryPerformanceCounter(&current);
        } while (static_cast<unsigned int>(current.LowPart - sampleStart.LowPart) < 0x3e8);

        const unsigned int endTicks = static_cast<unsigned int>(RecoilReadTimestampCounter());
        if (oldPriority != THREAD_PRIORITY_ERROR_RETURN) {
            SetThreadPriority(thread, oldPriority);
        }

        const unsigned int elapsedTicks =
            static_cast<unsigned int>(current.LowPart - sampleStart.LowPart);
        const unsigned int sampleCycles = endTicks - startTicks;
        const unsigned int sampleMicroseconds =
            DivideRoundedHalfUp(static_cast<unsigned __int64>(elapsedTicks) * 1000000ui64,
                                static_cast<unsigned int>(frequency.LowPart));
        totalCycles += sampleCycles;
        totalMicroseconds += sampleMicroseconds;

        priorMhz = DivideRoundedHalfUp(sampleCycles, sampleMicroseconds);
        if (attempt >= 3) {
            if (AbsDiffFromTripleAverage(priorMhz, previousMhz, olderMhz) <= 3 &&
                AbsDiffFromTripleAverage(previousMhz, olderMhz, priorMhz) <= 3 &&
                AbsDiffFromTripleAverage(olderMhz, priorMhz, previousMhz) <= 3) {
                break;
            }
        }

        if (attempt >= 0x14) {
            break;
        }
    }
    }

    const unsigned int cpuMhzRaw = totalMicroseconds == 0 ? 0 : totalCycles / totalMicroseconds;
    const unsigned int tenthMhz =
        totalMicroseconds == 0
            ? 0
            : static_cast<unsigned int>((static_cast<unsigned __int64>(totalCycles) * 10ui64) /
                                         totalMicroseconds);
    unsigned int cpuMhzRounded = cpuMhzRaw;
    if (tenthMhz - cpuMhzRaw * 10u >= 6u) {
        cpuMhzRounded += 1;
    }

    outBuffer->totalCycles = totalCycles;
    outBuffer->totalMicroseconds = totalMicroseconds;
    outBuffer->cpuMhzRaw = cpuMhzRaw;
    outBuffer->cpuMhzRounded = cpuMhzRounded;
    return outBuffer;
}

// Reimplements 0x4b3b50: CpuBenchmarkResolver::MeasureCpuMhz_CmosRtc
RECOIL_NOINLINE zSys::CpuBenchmarkResult *RECOIL_THISCALL
CpuBenchmarkResolver::MeasureCpuMhz_CmosRtc(zSys::CpuBenchmarkResult *outBuffer) {
    HANDLE thread = GetCurrentThread();
    const int oldPriority = GetThreadPriority(thread);
    if (oldPriority != THREAD_PRIORITY_ERROR_RETURN) {
        SetThreadPriority(thread, oldPriority + 1);
    }

    unsigned int previousSecond = ReadCmosRtcSecondsBcd();
    unsigned int startSecond;
    do {
        startSecond = ReadCmosRtcSecondsBcd();
    } while (startSecond == previousSecond);

    const unsigned __int64 startTsc = RecoilReadTimestampCounter();
    unsigned int endSecond;
    do {
        endSecond = ReadCmosRtcSecondsBcd();
    } while (endSecond == startSecond);

    const unsigned __int64 endTsc = RecoilReadTimestampCounter();
    if (oldPriority != THREAD_PRIORITY_ERROR_RETURN) {
        SetThreadPriority(thread, oldPriority);
    }

    const unsigned int elapsedSeconds =
        endSecond >= startSecond ? endSecond - startSecond : endSecond - startSecond + 0x0a;
    const unsigned int microseconds = elapsedSeconds * 1000000u;
    const unsigned int cycles = static_cast<unsigned int>(endTsc - startTsc);
    const unsigned int cpuMhzRaw = cycles / 1000000u;
    unsigned int cpuMhzRounded = cpuMhzRaw;
    if (cycles / 100000u - cpuMhzRaw * 10u >= 6u) {
        cpuMhzRounded += 1;
    }

    outBuffer->totalCycles = cycles;
    outBuffer->totalMicroseconds = microseconds;
    outBuffer->cpuMhzRaw = cpuMhzRaw;
    outBuffer->cpuMhzRounded = cpuMhzRounded;
    return outBuffer;
}

// Reimplements 0x4b36f0: CpuBenchmarkResolver::ResolveCpuBenchmarkPacket
RECOIL_NOINLINE zSys::CpuBenchmarkResult *RECOIL_THISCALL
CpuBenchmarkResolver::ResolveCpuBenchmarkPacket(zSys::CpuBenchmarkResult *outBuffer) {
    const int cpuClass = zSys::DetectCpuClassAndFeatures();
    const unsigned int featureFlags = zSys::ReadCpuidFeatureFlags();
    const int cpuClassHint =
        static_cast<int>((unsigned int)(this));

    if ((cpuClass & 0x8000) != 0) {
        ZeroBenchmarkResult(outBuffer);
        return outBuffer;
    }

    unsigned int expectedCycles;
    bool forcedLowHint = false;
    if (cpuClassHint <= 0) {
        expectedCycles = kCpuBenchmarkDurationTable[cpuClass & 0xffff] * 0x0fa0u;
    } else if (cpuClassHint <= 0x96) {
        forcedLowHint = true;
        expectedCycles = static_cast<unsigned int>(cpuClassHint) * 0x0fa0u;
    } else {
        expectedCycles = static_cast<unsigned int>((unsigned int)(outBuffer));
    }

    zSys::CpuBenchmarkResult localResult;
    zSys::CpuBenchmarkResult *measured;
    if ((featureFlags & 0x10u) != 0 && !forcedLowHint) {
        if (cpuClassHint != 0) {
            measured = CpuBenchmarkResolverFromValue(0)->MeasureCpuMhz_CmosRtc(&localResult);
        } else {
            measured = CpuBenchmarkResolverFromValue(0)->MeasureCpuMhz_RdtscQpc(&localResult);
        }
    } else if ((cpuClass & 0xffff) >= 3) {
        measured =
            CpuBenchmarkResolverFromValue(expectedCycles)->MeasureMhzViaBsfLoop_Qpc(&localResult);
    } else {
        ZeroBenchmarkResult(outBuffer);
        return outBuffer;
    }

    outBuffer->totalCycles = measured->totalCycles;
    outBuffer->totalMicroseconds = measured->totalMicroseconds;
    outBuffer->cpuMhzRaw = measured->cpuMhzRaw;
    outBuffer->cpuMhzRounded = measured->cpuMhzRounded;
    return outBuffer;
}

// Reimplements 0x4b3020: zCpu::HasMmxSupport
RECOIL_NOINLINE int RECOIL_CDECL zCpu::HasMmxSupport() {
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 1);
    return (cpuInfo[3] & 0x800000) != 0 ? 1 : 0;
}

#include "GameZRecoil/zSys/zSys_cpu_get_class.inl"

// Reimplements 0x4b31c0: zSys::GetCpuMhz
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_CDECL zSys::GetCpuMhz() {
    volatile CpuBenchmarkResult copied;
    CpuBenchmarkResult benchmark;
    const CpuBenchmarkResult *measured =
        CpuBenchmarkResolverFromValue(0)->ResolveCpuBenchmarkPacket(&benchmark);
    copied.totalCycles = measured->totalCycles;
    copied.totalMicroseconds = measured->totalMicroseconds;
    copied.cpuMhzRaw = measured->cpuMhzRaw;
    return measured->cpuMhzRounded;
}

// Reimplements 0x4b3210: zSys::ReturnZeroStub
RECOIL_NOINLINE int RECOIL_CDECL zSys::ReturnZeroStub() {
    return 0;
}

// Reimplements 0x4b3230: zSys::GetTotalPhysKb
RECOIL_NOINLINE RECOIL_NO_GS unsigned int RECOIL_CDECL zSys::GetTotalPhysKb() {
    MEMORYSTATUS status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwTotalPhys >> 10;
}

// Reimplements 0x4a5980: zSys::ExitProcessWithCleanup
RECOIL_NOINLINE void RECOIL_FASTCALL zSys::ExitProcessWithCleanup(int exitCode) {
    zGame::ReturnOnlyStub();
    _fcloseall();
    ExitProcess(static_cast<UINT>(exitCode));
#if defined(_MSC_VER)
    __assume(0);
#endif
}
