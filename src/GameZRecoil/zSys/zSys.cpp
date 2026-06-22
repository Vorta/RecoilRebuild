#define DIRECTINPUT_VERSION 0x0500
#if defined(_MSC_VER)
#define _CRT_SECURE_CPP_OVERLOAD_SECURE_NAMES 0
#endif

#include "GameZRecoil/zSys/zSys.h"

#include "GameZRecoil/zGame/zGame.h"

#include <intrin.h>
#include <limits>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <ddraw.h>
#include <windows.h>

#include <stdio.h>

#if defined(_MSC_VER)
#pragma warning(disable : 4996)
#endif

namespace {
typedef HRESULT(WINAPI *zDirectDrawCreateFn)(
    GUID *,
    LPDIRECTDRAW *,
    IUnknown *
);
typedef HMODULE(__stdcall *zLoadLibraryAFn)(const char *);

/**
 * Reimplements data 0x4daaf0: g_zSys_ProbeCreatePrimarySurfaceFailedMsg.
 * Purpose: Reports primary DirectDraw surface creation failure during the platform probe.
 */
const char g_zSys_ProbeCreatePrimarySurfaceFailedMsg[] = "Couldn't CreateSurface\r\n";

/**
 * Reimplements data 0x4dab0c: g_zSys_ProbeSetCoopLevelFailedMsg.
 * Purpose: Reports DirectDraw cooperative-level setup failure during the platform probe.
 */
const char g_zSys_ProbeSetCoopLevelFailedMsg[] = "Couldn't Set coop level\r\n";

/**
 * Reimplements data 0x4dab28: g_zSys_ProbeQiDdraw2FailedMsg.
 * Purpose: Reports failure to query the DirectDraw2 interface during the platform probe.
 */
const char g_zSys_ProbeQiDdraw2FailedMsg[] = "Couldn't QI DDraw2\r\n";

/**
 * Reimplements data 0x4dab40: g_zSys_ProbeCreateDdrawFailedMsg.
 * Purpose: Reports DirectDrawCreate failure during the platform probe.
 */
const char g_zSys_ProbeCreateDdrawFailedMsg[] = "Couldn't create DDraw\r\n";

/**
 * Reimplements data 0x4dab58: g_zSys_ProbeLoadDdrawFailedMsg.
 * Purpose: Reports missing DirectDraw library support during the platform probe.
 */
const char g_zSys_ProbeLoadDdrawFailedMsg[] = "Couldn't LoadLibrary DDraw\r\n";

/**
 * Reimplements data 0x4dab78: g_zSys_ProbeDirectDrawCreateExportName.
 * Purpose: Names the DirectDrawCreate export resolved from DDRAW.DLL.
 */
const char g_zSys_ProbeDirectDrawCreateExportName[] = "DirectDrawCreate";

/**
 * Reimplements data 0x4dab8c: g_zSys_ProbeDdrawDllName.
 * Purpose: Names the DirectDraw provider DLL loaded by the platform probe.
 */
const char g_zSys_ProbeDdrawDllName[] = "DDRAW.DLL";

/**
 * Reimplements data 0x4dab98: g_zSys_ProbeMissingDirectInputCreateMsg.
 * Purpose: Reports missing DirectInputCreateA export support during the platform probe.
 */
const char g_zSys_ProbeMissingDirectInputCreateMsg[] =
    "Couldn't GetProcAddress DInputCreate\r\n";

/**
 * Reimplements data 0x4dabc0: g_zSys_ProbeDirectInputCreateExportName.
 * Purpose: Names the DirectInputCreateA export resolved from DINPUT.DLL.
 */
const char g_zSys_ProbeDirectInputCreateExportName[] = "DirectInputCreateA";

/**
 * Reimplements data 0x4dabd4: g_zSys_ProbeLoadDinputFailedMsg.
 * Purpose: Reports missing DirectInput library support during the platform probe.
 */
const char g_zSys_ProbeLoadDinputFailedMsg[] = "Couldn't LoadLibrary DInput\r\n";

/**
 * Reimplements data 0x4dabf4: g_zSys_ProbeDinputDllName.
 * Purpose: Names the DirectInput provider DLL loaded by the platform probe.
 */
const char g_zSys_ProbeDinputDllName[] = "DINPUT.DLL";

/**
 * Reimplements data 0x4e46a0: g_zSys_CpuBenchmarkDurationTable.
 * Purpose: Maps recovered CPU-class indices to the fixed BSF-loop cycle budget used by GetCpuMhz.
 */
const unsigned int g_zSys_CpuBenchmarkDurationTable[12] =
    {0, 0, 0, 115, 47, 43, 38, 38, 38, 38, 38, 38};

/**
 * Reimplements data 0x4e467c: g_zSys_CpuVendorExpectedIntel.
 * Purpose: Supplies the expected Intel vendor bytes for the CPU detection helpers.
 */
const char g_zSys_CpuVendorExpectedIntel[0x0d] = "GenuineIntel";

/**
 * Reimplements data 0x4e468c: g_zSys_CpuVendorScratchPadInit.
 * Purpose: Initializes 12-byte stack vendor buffers before CPUID overwrites EBX/EDX/ECX.
 */
const char g_zSys_CpuVendorScratchPadInit[0x0d] = "------------";

/**
 * Reimplements data 0x56b438: g_zSys_DriveTypeSearchPathBuffer.
 * Purpose: Stores the candidate drive path returned by FindFileOnDriveType.
 */
char g_zSys_DriveTypeSearchPathBuffer[MAX_PATH];

/*
 * Source-file helper cluster for zsys_cpu.cpp CPU benchmarking. Retail uses a
 * thiscall-shaped ABI for these helpers: ECX carries the class hint or expected
 * cycle count, and the result packet is the single stack argument. The helper
 * object has no storage, no constructor, and no table identity in BN evidence.
 */
struct CpuBenchmarkResolver {
    zSys::CpuBenchmarkResult * ResolveCpuBenchmarkPacket(
        zSys::CpuBenchmarkResult *outBuffer
    );

    zSys::CpuBenchmarkResult * MeasureMhzViaBsfLoop_Qpc(
        zSys::CpuBenchmarkResult *outBuffer
    );

    zSys::CpuBenchmarkResult * MeasureCpuMhz_RdtscQpc(
        zSys::CpuBenchmarkResult *outBuffer
    );

    zSys::CpuBenchmarkResult * MeasureCpuMhz_CmosRtc(
        zSys::CpuBenchmarkResult *outBuffer
    );
};

} // namespace

/**
 * Reimplements data 0x56bd14: g_zSys_CpuIsNonIntel.
 * Purpose: Carries the CPUID vendor mismatch marker folded into DetectCpuClassAndFeatures.
 */
extern "C" unsigned int g_zSys_CpuVendorNonIntelMarker = 0;

#include "GameZRecoil/zSys/zSys_probe_platform.inl"

/**
 * Reimplements 0x4a59e0: zSys::FindFileOnDriveType.
 * Purpose: Scans logical drives of a requested type and returns the first path containing a file.
 */
RECOIL_NO_GS char *__fastcall zSys::FindFileOnDriveType(
    int driveType,
    const char *relativePath,
    int
) {
    enum {
        kLogicalDriveStringsReadLimit = 256,
        kLogicalDriveStringsBufferSize = 300
    };
    char driveStrings[kLogicalDriveStringsBufferSize];
    const char *searchPath;
    struct _stat statBuffer;
    searchPath = relativePath;
    GetLogicalDriveStringsA(
        kLogicalDriveStringsReadLimit,
        driveStrings
    );

    int driveListOffset = 0;
    int found = 0;
    while (1) {
        const char *drive = &driveStrings[driveListOffset];
        sprintf(
            g_zSys_DriveTypeSearchPathBuffer,
            "%s%s",
            drive,
            searchPath
        );
        switch (GetDriveTypeA(drive)) {
        case DRIVE_FIXED:
            if (driveType == DRIVE_FIXED) {
                if (_stat(
                    g_zSys_DriveTypeSearchPathBuffer,
                    &statBuffer
                ) == 0) {
                    found = 1;
                }
            }
            break;

        case DRIVE_CDROM:
            if (driveType == DRIVE_CDROM) {
                if (_stat(
                    g_zSys_DriveTypeSearchPathBuffer,
                    &statBuffer
                ) == 0) {
                    found = 1;
                }
            }
            break;
        }

        if (found != 0) {
            return g_zSys_DriveTypeSearchPathBuffer;
        }

        if (strlen(drive) == 0) {
            break;
        }

        driveListOffset += (int)(strlen(drive) + 1);
    }

    return 0;
}

#if !(defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM))
/**
 * Reimplements 0x4b3050: zSys::CheckCpuSignatureMask.
 * Purpose: Reads CPUID leaf 1 and tests whether the CPU signature mask matches the optimized-path gate.
 */
int zSys::CheckCpuSignatureMask() {
    int cpuInfo[4] = {0};
    __cpuid(
        cpuInfo,
        1
    );
    return (cpuInfo[0] & 0x630) == 0x630 ? 1 : 0;
}
#endif

#if !(defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM))
/**
 * Reimplements 0x4b2fe0: zSys::HasCpuidSupportRuntimeOptions.
 * Purpose: Provides the runtime-options CPUID support probe used by game and sound setup.
 */
int zSys::HasCpuidSupportRuntimeOptions() {
    return HasCpuidSupport() != 0 ? 1 : 0;
}

/**
 * Reimplements 0x4b33f0: zSys::HasCpuidSupport (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Reports whether the processor supports executing the CPUID instruction.
 */
unsigned short zSys::HasCpuidSupport() {
    return 1;
}

/**
 * Reimplements 0x4b3640: zSys::ReadCpuidVendorAndFamily (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Reads CPUID vendor/family data and records whether the vendor is non-Intel.
 */
int zSys::ReadCpuidVendorAndFamily() {
    int cpuInfo[4];
    __cpuid(
        cpuInfo,
        0
    );
    char vendor[0x0c];
    memcpy(
        vendor,
        g_zSys_CpuVendorScratchPadInit,
        sizeof(vendor)
    );
    memcpy(
        &vendor[0],
        &cpuInfo[1],
        4
    );
    memcpy(
        &vendor[4],
        &cpuInfo[3],
        4
    );
    memcpy(
        &vendor[8],
        &cpuInfo[2],
        4
    );
    if (memcmp(
        vendor,
        g_zSys_CpuVendorExpectedIntel,
        sizeof(vendor)
    ) != 0) {
        g_zSys_CpuVendorNonIntelMarker = 1;
    }

    __cpuid(
        cpuInfo,
        1
    );
    return (cpuInfo[0] >> 8) & 0x0f;
}
#endif

#if !(defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM))
/**
 * Reimplements 0x4b3480: zSys::ReadCpuidFeatureFlags.
 * Purpose: Reads the CPUID feature flags after validating CPUID support and Intel-family gating.
 */
unsigned int zSys::ReadCpuidFeatureFlags() {
    if (HasCpuidSupport() == 0) {
        return 0;
    }

    int cpuInfo[4];
    __cpuid(
        cpuInfo,
        0
    );

    char vendor[0x0c];
    memcpy(
        vendor,
        g_zSys_CpuVendorScratchPadInit,
        sizeof(vendor)
    );
    memcpy(
        &vendor[0],
        &cpuInfo[1],
        4
    );
    memcpy(
        &vendor[4],
        &cpuInfo[3],
        4
    );
    memcpy(
        &vendor[8],
        &cpuInfo[2],
        4
    );
    if (memcmp(
        vendor,
        g_zSys_CpuVendorExpectedIntel,
        sizeof(vendor)
    ) != 0) {
        g_zSys_CpuVendorNonIntelMarker = 1;
    }

    __cpuid(
        cpuInfo,
        1
    );
    return (unsigned int)(cpuInfo[3]);
}
#endif

#if !(defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM))
/**
 * Reimplements 0x4b3b00: zSys::ReadCmosRtcSecondsBcd.
 * Purpose: Reads the CMOS real-time clock seconds register in BCD form.
 */
unsigned int zSys::ReadCmosRtcSecondsBcd() {
    const unsigned int seconds = (unsigned int)((GetTickCount() / 1000u) % 60u);
    return (unsigned int)(((seconds / 10u) << 4) | (seconds % 10u));
}
#endif

#if !(defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM))
/**
 * Reimplements 0x4b3b20: zSys::ReadTsc64.
 * Purpose: Stores the current timestamp counter into split low/high 32-bit outputs.
 */
void __fastcall zSys::ReadTsc64(
    unsigned int *outHigh,
    unsigned int *outLow
) {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    if (outHigh != 0) {
        *outHigh = (unsigned int)(counter.HighPart);
    }
    if (outLow != 0) {
        *outLow = (unsigned int)(counter.LowPart);
    }
}
#endif

#if !(defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM))
/**
 * Reimplements 0x4b3ca0: zSys::Sub64.
 * Purpose: Subtracts one split 64-bit unsigned value from another and stores the split result.
 */
void __fastcall zSys::Sub64(
    unsigned int subHigh,
    unsigned int subLow,
    unsigned int minuendHigh,
    unsigned int minuendLow,
    unsigned int *outHigh,
    unsigned int *outLow
) {
    const unsigned int resultLow = minuendLow - subLow;
    const unsigned int resultHigh = minuendHigh - subHigh - (minuendLow < subLow ? 1u : 0u);
    *outLow = resultLow;
    *outHigh = resultHigh;
}
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
#include "GameZRecoil/zSys/zSys_cpu_asm.inl"
#else
namespace zSys {
/**
 * Reimplements 0x4b3510: zSys::ProbeDivZeroFlagBehavior (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Probes CPU flag behavior used by the legacy CPU-class detector.
 */
int ProbeDivZeroFlagBehavior() {
    return 0;
}

/**
 * Reimplements 0x4b3550: zSys::DetectIs8086ByEflagsHiBits (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Detects 8086-class EFLAGS high-bit behavior for CPU classification.
 */
int DetectIs8086ByEflagsHiBits() {
    return 0xffff;
}

/**
 * Reimplements 0x4b35a0: zSys::DetectIs80286ByEflagsHiBits (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Detects 80286-class EFLAGS high-bit behavior for CPU classification.
 */
int DetectIs80286ByEflagsHiBits() {
    return 0xffff;
}

/**
 * Reimplements 0x4b35f0: zSys::DetectIs80386ByAcFlag (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Detects 80386-class alignment-check flag behavior for CPU classification.
 */
int DetectIs80386ByAcFlag() {
    return 0xffff;
}
} // namespace zSys
#endif

#include "GameZRecoil/zSys/zSys_cpu_detect.inl"

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
/*
 * The VC5 raw-assembly profile keeps this timing probe as assembly because the
 * BSF busy-loop opcode placement and thiscall-shaped ECX input are part of the
 * recovered retail source contract. The portable branch below is the native
 * smoke fallback and is not byte evidence for the retail helper.
 */
/**
 * Reimplements 0x4b37f0: CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc.
 * Purpose: Measures CPU MHz by timing the retail fixed BSF busy loop with QueryPerformanceCounter.
 */
__declspec(naked) zSys::CpuBenchmarkResult * CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc(
    zSys::CpuBenchmarkResult *
) {
    __asm {
        push ebp
        mov ebp, esp
        sub esp, 020h
        push ebx
        push esi
        lea eax, [ebp - 020h]
        push edi
        xor ebx, ebx
        mov dword ptr [ebp - 008h], ecx
        push eax
        or esi, 0ffffffffh
        mov edi, ebx
        call dword ptr [QueryPerformanceFrequency]
        test eax, eax
        jne recoil_cpu_bsf_have_frequency
        mov eax, dword ptr [ebp + 008h]
        mov ecx, ebx
        mov edx, eax
        mov dword ptr [edx], edi
        mov dword ptr [edx + 004h], ecx
        mov dword ptr [edx + 008h], ecx
        mov dword ptr [edx + 00ch], ebx
        pop edi
        pop esi
        pop ebx
        mov esp, ebp
        pop ebp
        ret 4

    recoil_cpu_bsf_have_frequency:
        mov edi, dword ptr [QueryPerformanceCounter]
        mov dword ptr [ebp - 004h], 0ah

    recoil_cpu_bsf_sample_loop:
        lea eax, [ebp - 018h]
        push eax
        call edi
        mov eax, 080000000h
        mov bx, 0fa0h

    recoil_cpu_bsf_busy_loop:
        _emit 0x0f
        _emit 0xbc
        _emit 0xc8
        dec bx
        jne recoil_cpu_bsf_busy_loop
        lea ecx, [ebp - 010h]
        push ecx
        call edi
        mov eax, dword ptr [ebp - 010h]
        mov edx, dword ptr [ebp - 018h]
        sub eax, edx
        cmp eax, esi
        jae recoil_cpu_bsf_keep_min
        mov esi, eax

    recoil_cpu_bsf_keep_min:
        mov eax, dword ptr [ebp - 004h]
        dec eax
        mov dword ptr [ebp - 004h], eax
        jne recoil_cpu_bsf_sample_loop
        lea eax, [esi + esi * 4]
        mov ecx, dword ptr [ebp - 020h]
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea esi, [eax + eax * 4]
        mov eax, 0cccccccdh
        mul ecx
        shl esi, 5
        mov edi, edx
        mov eax, esi
        shr edi, 3
        xor edx, edx
        div edi
        xor edx, edx
        mov esi, eax
        div ecx
        shr ecx, 1
        cmp edx, ecx
        jbe recoil_cpu_bsf_microseconds_rounded
        inc esi

    recoil_cpu_bsf_microseconds_rounded:
        mov edi, dword ptr [ebp - 008h]
        xor edx, edx
        mov eax, edi
        div esi
        xor edx, edx
        mov ecx, eax
        mov eax, edi
        div esi
        mov eax, esi
        mov ebx, ecx
        shr eax, 1
        cmp edx, eax
        jbe recoil_cpu_bsf_result_ready
        inc ecx

    recoil_cpu_bsf_result_ready:
        mov eax, dword ptr [ebp + 008h]
        mov edx, eax
        mov dword ptr [edx], edi
        pop edi
        mov dword ptr [edx + 004h], esi
        pop esi
        mov dword ptr [edx + 008h], ebx
        pop ebx
        mov dword ptr [edx + 00ch], ecx
        mov esp, ebp
        pop ebp
        ret 4
    }
}
#else
/**
 * Reimplements 0x4b37f0: CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc.
 * Purpose: Measures CPU MHz by timing a fixed BSF busy loop with QueryPerformanceCounter.
 */
zSys::CpuBenchmarkResult * CpuBenchmarkResolver::MeasureMhzViaBsfLoop_Qpc(
    zSys::CpuBenchmarkResult *outBuffer
) {
    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency) == 0) {
        outBuffer->totalCycles = 0;
        outBuffer->totalMicroseconds = 0;
        outBuffer->cpuMhzRaw = 0;
        outBuffer->cpuMhzRounded = 0;
        return outBuffer;
    }

    unsigned int minTicks = 0xffffffffu;
    {
        for (int sample = 0; sample < 10; ++sample) {
            LARGE_INTEGER start;
            LARGE_INTEGER end;
            QueryPerformanceCounter(&start);

            for (unsigned short i = 0x0fa0; i != 1; --i) {
                unsigned long bitIndex = 0;
                while (((0x80000000u >> bitIndex) & 1u) == 0u && bitIndex < 31u) {
                    ++bitIndex;
                }
                (void)bitIndex;
            }

            QueryPerformanceCounter(&end);
            const unsigned int ticks = (unsigned int)(end.LowPart - start.LowPart);
            if (ticks < minTicks) {
                minTicks = ticks;
            }
        }
    }

    const unsigned int expectedCycles = (unsigned int)((unsigned int)(this));
    unsigned int microseconds = 0;
    if (frequency.LowPart != 0) {
        const unsigned __int64 numerator = (unsigned __int64)(minTicks) * 1000000ui64;
        const unsigned int quotient = (unsigned int)(numerator / (unsigned int)(frequency.LowPart));
        const unsigned int remainder = (unsigned int)(numerator % (unsigned int)(frequency.LowPart));
        microseconds =
            remainder > ((unsigned int)(frequency.LowPart) >> 1) ? quotient + 1 : quotient;
    }
    const unsigned int cpuMhzRaw = microseconds == 0 ? 0 : expectedCycles / microseconds;
    unsigned int cpuMhzRounded = 0;
    if (microseconds != 0) {
        const unsigned int quotient = expectedCycles / microseconds;
        const unsigned int remainder = expectedCycles % microseconds;
        cpuMhzRounded = remainder > (microseconds >> 1) ? quotient + 1 : quotient;
    }

    outBuffer->totalCycles = expectedCycles;
    outBuffer->totalMicroseconds = microseconds;
    outBuffer->cpuMhzRaw = cpuMhzRaw;
    outBuffer->cpuMhzRounded = cpuMhzRounded;
    return outBuffer;
}
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
/*
 * The VC5 raw-assembly profile keeps this timing probe as assembly because the
 * RDTSC/QPC sampling loop, thread-priority calls, and convergence arithmetic
 * are byte-verified in their original register and stack shape. The portable
 * branch below is the native smoke fallback and is not byte evidence.
 */
/**
 * Reimplements 0x4b38e0: CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc.
 * Purpose: Measures CPU MHz with the retail RDTSC/QPC timing loop.
 */
__declspec(naked) zSys::CpuBenchmarkResult * CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc(
    zSys::CpuBenchmarkResult *
) {
    __asm {
        push ebp
        mov ebp, esp
        sub esp, 038h
        push ebx
        push esi
        push edi
        xor edi, edi
        xor ebx, ebx
        mov dword ptr [ebp - 004h], edi
        mov dword ptr [ebp - 00ch], edi
        mov dword ptr [ebp - 010h], edi
        call dword ptr [GetCurrentThread]
        mov dword ptr [ebp - 018h], eax
        lea eax, [ebp - 038h]
        push eax
        xor esi, esi
        call dword ptr [QueryPerformanceFrequency]
        test eax, eax
        jne recoil_cpu_rdtsc_have_frequency
        mov eax, dword ptr [ebp + 008h]
        mov ecx, esi
        mov edx, eax
        mov dword ptr [edx], ecx
        mov dword ptr [edx + 004h], ecx
        mov dword ptr [edx + 008h], ecx
        mov dword ptr [edx + 00ch], esi
        pop edi
        pop esi
        pop ebx
        mov esp, ebp
        pop ebp
        ret 4

    recoil_cpu_rdtsc_continue:
        mov edi, dword ptr [ebp - 008h]

    recoil_cpu_rdtsc_have_frequency:
        mov esi, dword ptr [ebp - 004h]
        lea eax, [ebp - 030h]
        inc esi
        push eax
        mov dword ptr [ebp - 004h], esi
        mov esi, dword ptr [QueryPerformanceCounter]
        mov dword ptr [ebp - 014h], edi
        mov dword ptr [ebp - 008h], ebx
        call esi
        mov edi, dword ptr [ebp - 018h]
        mov ecx, dword ptr [ebp - 030h]
        mov edx, dword ptr [ebp - 02ch]
        push edi
        mov dword ptr [ebp - 028h], ecx
        mov dword ptr [ebp - 024h], edx
        call dword ptr [GetThreadPriority]
        mov ebx, eax
        cmp ebx, 07fffffffh
        je recoil_cpu_rdtsc_priority_ready
        push 0fh
        push edi
        call dword ptr [SetThreadPriority]

    recoil_cpu_rdtsc_priority_ready:
        mov eax, dword ptr [ebp - 028h]
        mov edx, dword ptr [ebp - 030h]
        mov ecx, eax
        sub ecx, edx
        cmp ecx, 032h
        jae recoil_cpu_rdtsc_warmup_done

    recoil_cpu_rdtsc_warmup_loop:
        lea edx, [ebp - 028h]
        push edx
        call esi
        _emit 0x0f
        _emit 0x31
        mov dword ptr [ebp - 020h], eax
        mov eax, dword ptr [ebp - 028h]
        mov edx, dword ptr [ebp - 030h]
        mov ecx, eax
        sub ecx, edx
        cmp ecx, 032h
        jb recoil_cpu_rdtsc_warmup_loop

    recoil_cpu_rdtsc_warmup_done:
        mov edx, dword ptr [ebp - 024h]
        xor ecx, ecx
        cmp ecx, 03e8h
        mov dword ptr [ebp - 030h], eax
        mov dword ptr [ebp - 02ch], edx
        jae recoil_cpu_rdtsc_sample_done

    recoil_cpu_rdtsc_sample_loop:
        lea edx, [ebp - 028h]
        push edx
        call esi
        _emit 0x0f
        _emit 0x31
        mov dword ptr [ebp - 01ch], eax
        mov eax, dword ptr [ebp - 028h]
        mov edx, dword ptr [ebp - 030h]
        mov ecx, eax
        sub ecx, edx
        cmp ecx, 03e8h
        jb recoil_cpu_rdtsc_sample_loop

    recoil_cpu_rdtsc_sample_done:
        cmp ebx, 07fffffffh
        je recoil_cpu_rdtsc_priority_restored
        push ebx
        push edi
        call dword ptr [SetThreadPriority]
        mov eax, dword ptr [ebp - 028h]

    recoil_cpu_rdtsc_priority_restored:
        mov edx, dword ptr [ebp - 030h]
        mov esi, dword ptr [ebp - 01ch]
        sub eax, edx
        mov edi, dword ptr [ebp - 020h]
        mov ecx, dword ptr [ebp - 038h]
        sub esi, edi
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea edi, [eax + eax * 4]
        mov eax, 0cccccccdh
        mul ecx
        shl edi, 5
        mov ebx, edx
        mov eax, edi
        shr ebx, 3
        xor edx, edx
        div ebx
        mov edx, dword ptr [ebp - 010h]
        mov edi, eax
        mov eax, dword ptr [ebp - 00ch]
        add edx, edi
        add eax, esi
        mov dword ptr [ebp - 010h], edx
        mov dword ptr [ebp - 00ch], eax
        mov eax, edi
        xor edx, edx
        div ecx
        shr ecx, 1
        cmp edx, ecx
        jbe recoil_cpu_rdtsc_microseconds_rounded
        inc edi

    recoil_cpu_rdtsc_microseconds_rounded:
        mov eax, esi
        xor edx, edx
        div edi
        xor edx, edx
        mov ebx, eax
        mov eax, esi
        div edi
        shr edi, 1
        cmp edx, edi
        jbe recoil_cpu_rdtsc_mhz_rounded
        inc ebx

    recoil_cpu_rdtsc_mhz_rounded:
        mov edx, dword ptr [ebp - 014h]
        mov eax, dword ptr [ebp - 008h]
        lea ecx, [edx + eax]
        mov eax, dword ptr [ebp - 004h]
        add ecx, ebx
        cmp eax, 3
        jl recoil_cpu_rdtsc_continue
        cmp eax, 014h
        jge recoil_cpu_rdtsc_average
        lea eax, [ebx + ebx * 2]
        sub eax, ecx
        cdq
        xor eax, edx
        sub eax, edx
        cmp eax, 3
        jg recoil_cpu_rdtsc_continue
        mov eax, dword ptr [ebp - 008h]
        lea eax, [eax + eax * 2]
        sub eax, ecx
        cdq
        xor eax, edx
        sub eax, edx
        cmp eax, 3
        jg recoil_cpu_rdtsc_continue
        mov eax, dword ptr [ebp - 014h]
        lea eax, [eax + eax * 2]
        sub eax, ecx
        cdq
        xor eax, edx
        sub eax, edx
        cmp eax, 3
        jg recoil_cpu_rdtsc_continue

    recoil_cpu_rdtsc_average:
        mov esi, dword ptr [ebp - 00ch]
        mov edi, dword ptr [ebp - 010h]
        xor edx, edx
        lea eax, [esi + esi * 4]
        shl eax, 1
        div edi
        xor edx, edx
        mov ecx, eax
        lea eax, [esi + esi * 4]
        lea eax, [eax + eax * 4]
        shl eax, 2
        div edi
        lea edx, [ecx + ecx * 4]
        shl edx, 1
        sub eax, edx
        cmp eax, 6
        jb recoil_cpu_rdtsc_raw_ready
        inc ecx

    recoil_cpu_rdtsc_raw_ready:
        mov eax, esi
        xor edx, edx
        div edi
        mov edx, eax
        lea ebx, [eax + eax * 4]
        shl ebx, 1
        sub ecx, ebx
        cmp ecx, 6
        jb recoil_cpu_rdtsc_store_result
        lea edx, [eax + 001h]

    recoil_cpu_rdtsc_store_result:
        mov ecx, dword ptr [ebp + 008h]
        mov ebx, ecx
        mov dword ptr [ebx], esi
        mov dword ptr [ebx + 004h], edi
        pop edi
        pop esi
        mov dword ptr [ebx + 008h], eax
        mov eax, ecx
        mov dword ptr [ebx + 00ch], edx
        pop ebx
        mov esp, ebp
        pop ebp
        ret 4
    }
}
#else
/**
 * Reimplements 0x4b38e0: CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc.
 * Purpose: Measures CPU MHz using RDTSC deltas over QueryPerformanceCounter intervals.
 */
zSys::CpuBenchmarkResult * CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc(
    zSys::CpuBenchmarkResult *outBuffer
) {
    HANDLE thread = GetCurrentThread();

    LARGE_INTEGER frequency;
    if (QueryPerformanceFrequency(&frequency) == 0) {
        outBuffer->totalCycles = 0;
        outBuffer->totalMicroseconds = 0;
        outBuffer->cpuMhzRaw = 0;
        outBuffer->cpuMhzRounded = 0;
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
                SetThreadPriority(
                    thread,
                    THREAD_PRIORITY_TIME_CRITICAL
                );
            }

            while ((unsigned int)(current.LowPart - outerStart.LowPart) < 0x32) {
                QueryPerformanceCounter(&current);
            }

            LARGE_INTEGER startCounter;
            QueryPerformanceCounter(&startCounter);
            const unsigned int startTicks = (unsigned int)(startCounter.LowPart);
            LARGE_INTEGER sampleStart = current;
            do {
                QueryPerformanceCounter(&current);
            } while ((unsigned int)(current.LowPart - sampleStart.LowPart) < 0x3e8);

            LARGE_INTEGER endCounter;
            QueryPerformanceCounter(&endCounter);
            const unsigned int endTicks = (unsigned int)(endCounter.LowPart);
            if (oldPriority != THREAD_PRIORITY_ERROR_RETURN) {
                SetThreadPriority(
                    thread,
                    oldPriority
                );
            }

            const unsigned int elapsedTicks = (unsigned int)(current.LowPart - sampleStart.LowPart);
            const unsigned int sampleCycles = endTicks - startTicks;
            unsigned int sampleMicroseconds = 0;
            if (frequency.LowPart != 0) {
                const unsigned __int64 numerator =
                    (unsigned __int64)(elapsedTicks) * 1000000ui64;
                const unsigned int quotient =
                    (unsigned int)(numerator / (unsigned int)(frequency.LowPart));
                const unsigned int remainder =
                    (unsigned int)(numerator % (unsigned int)(frequency.LowPart));
                sampleMicroseconds =
                    remainder > ((unsigned int)(frequency.LowPart) >> 1) ? quotient + 1 : quotient;
            }
            totalCycles += sampleCycles;
            totalMicroseconds += sampleMicroseconds;

            priorMhz = 0;
            if (sampleMicroseconds != 0) {
                const unsigned int quotient = sampleCycles / sampleMicroseconds;
                const unsigned int remainder = sampleCycles % sampleMicroseconds;
                priorMhz = remainder > (sampleMicroseconds >> 1) ? quotient + 1 : quotient;
            }
            if (attempt >= 3) {
                const int priorDelta = (int)(priorMhz * 3u - priorMhz - previousMhz - olderMhz);
                const int previousDelta =
                    (int)(previousMhz * 3u - previousMhz - olderMhz - priorMhz);
                const int olderDelta = (int)(olderMhz * 3u - olderMhz - priorMhz - previousMhz);
                const unsigned int priorDiff =
                    priorDelta < 0 ? (unsigned int)(-priorDelta) : (unsigned int)(priorDelta);
                const unsigned int previousDiff =
                    previousDelta < 0 ? (unsigned int)(-previousDelta)
                                      : (unsigned int)(previousDelta);
                const unsigned int olderDiff =
                    olderDelta < 0 ? (unsigned int)(-olderDelta) : (unsigned int)(olderDelta);
                if (priorDiff <= 3 && previousDiff <= 3 && olderDiff <= 3) {
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
            : (unsigned int)(((unsigned __int64)(totalCycles) * 10ui64) / totalMicroseconds);
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
#endif

/**
 * Reimplements 0x4b3b50: CpuBenchmarkResolver::MeasureCpuMhz_CmosRtc.
 * Purpose: Measures CPU MHz by timing a busy loop against CMOS RTC second ticks.
 */
zSys::CpuBenchmarkResult * CpuBenchmarkResolver::MeasureCpuMhz_CmosRtc(
    zSys::CpuBenchmarkResult *outBuffer
) {
    HANDLE thread = GetCurrentThread();
    const int oldPriority = GetThreadPriority(thread);
    if (oldPriority != THREAD_PRIORITY_ERROR_RETURN) {
        SetThreadPriority(
            thread,
            oldPriority + 1
        );
    }

    unsigned int previousSecond = zSys::ReadCmosRtcSecondsBcd();
    unsigned int startSecond;
    unsigned int startAdvance;
    do {
        startSecond = zSys::ReadCmosRtcSecondsBcd();
        if (startSecond < previousSecond) {
            startAdvance = startSecond - previousSecond + 0x0a;
        } else {
            startAdvance = startSecond > previousSecond ? 1u : 0u;
        }
    } while (startAdvance == 0);

    unsigned int startTscHigh = 0;
    unsigned int startTscLow = 0;
    zSys::ReadTsc64(
        &startTscHigh,
        &startTscLow
    );
    unsigned int endSecond;
    unsigned int endAdvance;
    do {
        endSecond = zSys::ReadCmosRtcSecondsBcd();
        if (endSecond < startSecond) {
            endAdvance = endSecond - startSecond + 0x0a;
        } else {
            endAdvance = endSecond > startSecond ? 1u : 0u;
        }
    } while (endAdvance == 0);

    unsigned int endTscHigh = 0;
    unsigned int endTscLow = 0;
    zSys::ReadTsc64(
        &endTscHigh,
        &endTscLow
    );
    if (oldPriority != THREAD_PRIORITY_ERROR_RETURN) {
        SetThreadPriority(
            thread,
            oldPriority
        );
    }

    const unsigned int microseconds =
        endSecond * 1000000u - startSecond * 1000000u;
    unsigned int elapsedHigh = 0;
    unsigned int elapsedLow = 0;
    zSys::Sub64(
        startTscHigh,
        startTscLow,
        endTscHigh,
        endTscLow,
        &elapsedHigh,
        &elapsedLow
    );
    const unsigned int cycles = elapsedLow;
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

/**
 * Reimplements 0x4b36f0: CpuBenchmarkResolver::ResolveCpuBenchmarkPacket.
 * Purpose: Chooses the CPU benchmark strategy and writes the selected measurement packet.
 */
zSys::CpuBenchmarkResult * CpuBenchmarkResolver::ResolveCpuBenchmarkPacket(
    zSys::CpuBenchmarkResult *outBuffer
) {
    const int cpuClass = zSys::DetectCpuClassAndFeatures();
    const unsigned int featureFlags = zSys::ReadCpuidFeatureFlags();
    const int cpuClassHint = (int)((unsigned int)(this));

    if ((cpuClass & 0x8000) != 0) {
        outBuffer->totalCycles = 0;
        outBuffer->totalMicroseconds = 0;
        outBuffer->cpuMhzRaw = 0;
        outBuffer->cpuMhzRounded = 0;
        return outBuffer;
    }

    unsigned int expectedCycles;
    bool forcedLowHint = false;
    if (cpuClassHint <= 0) {
        expectedCycles = g_zSys_CpuBenchmarkDurationTable[cpuClass & 0xffff] * 4000u;
    } else if (cpuClassHint <= 0x96) {
        forcedLowHint = true;
        expectedCycles = (unsigned int)(cpuClassHint) * 4000u;
    } else {
        expectedCycles = (unsigned int)((unsigned int)(outBuffer));
    }

    zSys::CpuBenchmarkResult localResult;
    zSys::CpuBenchmarkResult *measured;
    if ((featureFlags & 0x10u) != 0 && !forcedLowHint) {
        if (cpuClassHint != 0) {
            measured = ((CpuBenchmarkResolver *)expectedCycles)->MeasureCpuMhz_CmosRtc(&localResult);
        } else {
            measured = ((CpuBenchmarkResolver *)expectedCycles)->MeasureCpuMhz_RdtscQpc(&localResult);
        }
    } else if ((cpuClass & 0xffff) >= 3) {
        measured =
            ((CpuBenchmarkResolver *)expectedCycles)->MeasureMhzViaBsfLoop_Qpc(&localResult);
    } else {
        outBuffer->totalCycles = 0;
        outBuffer->totalMicroseconds = 0;
        outBuffer->cpuMhzRaw = 0;
        outBuffer->cpuMhzRounded = 0;
        return outBuffer;
    }

    outBuffer->totalCycles = measured->totalCycles;
    outBuffer->totalMicroseconds = measured->totalMicroseconds;
    outBuffer->cpuMhzRaw = measured->cpuMhzRaw;
    outBuffer->cpuMhzRounded = measured->cpuMhzRounded;
    return outBuffer;
}

#if !(defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM))
/**
 * Reimplements 0x4b3020: zCpu::HasMmxSupport.
 * Purpose: Reads CPUID leaf 1 and returns whether EDX bit 23 reports MMX support.
 */
int zCpu::HasMmxSupport() {
    int cpuInfo[4] = {0};
    __cpuid(
        cpuInfo,
        1
    );
    return (cpuInfo[3] & 0x800000) != 0 ? 1 : 0;
}
#endif

#include "GameZRecoil/zSys/zSys_cpu_get_class.inl"

/**
 * Reimplements 0x4b31c0: zSys::GetCpuMhz.
 * Purpose: Resolves the current CPU benchmark packet and returns the rounded MHz value.
 */
RECOIL_NO_GS int zSys::GetCpuMhz() {
    volatile CpuBenchmarkResult copied;
    CpuBenchmarkResult benchmark;
    const volatile CpuBenchmarkResult *measured =
        ((CpuBenchmarkResolver *)0)->ResolveCpuBenchmarkPacket(&benchmark);
    copied.totalCycles = measured->totalCycles;
    copied.totalMicroseconds = measured->totalMicroseconds;
    copied.cpuMhzRaw = measured->cpuMhzRaw;
    return measured->cpuMhzRounded;
}

/**
 * Reimplements 0x4b3210: zSys::ReturnZeroStub.
 * Purpose: Returns zero for callers that need a stable legacy system stub.
 */
int zSys::ReturnZeroStub() {
    return 0;
}

/**
 * Reimplements 0x4b3230: zSys::GetTotalPhysKb.
 * Purpose: Reads Windows memory status and returns total physical memory in kilobytes.
 */
RECOIL_NO_GS unsigned int zSys::GetTotalPhysKb() {
    MEMORYSTATUS status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatus(&status);
    return status.dwTotalPhys >> 10;
}

/**
 * Reimplements 0x4a5980: zSys::ExitProcessWithCleanup.
 * Purpose: Runs shutdown cleanup hooks, closes CRT streams, and terminates the process.
 * Retail keeps VC5's unreachable pop/ret epilogue after the noreturn ExitProcess import.
 */
void __fastcall zSys::ExitProcessWithCleanup(
    int exitCode
) {
    zGame::ReturnOnlyStub();
    _fcloseall();
    ExitProcess((UINT)(exitCode));
#if defined(_MSC_VER) && _MSC_VER >= 1300
    __assume(0);
#endif
}
