#include "GameZRecoil/zSys/zsys.h"

#include <intrin.h>
#include <string.h>
#include <windows.h>

namespace {

using zSys::ReadCmosRtcSecondsBcd;
using zSys::ReadTsc64;
using zSys::Sub64;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-g-zsys-cpubenchmarkdurationtable
 * @recoil-artifact defines .data recoil:data:0x4e46a0: g_zSys_CpuBenchmarkDurationTable.
 * Purpose: maps CPU-class indices to the fixed BSF-loop cycle budget.
 */
const unsigned int g_zSys_CpuBenchmarkDurationTable[12] =
    {0, 0, 0, 115, 47, 43, 38, 38, 38, 38, 38, 38};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-g-zsys-cpuvendorexpectedintel
 * @recoil-artifact defines .data recoil:data:0x4e467c: g_zSys_CpuVendorExpectedIntel.
 * Purpose: supplies the expected Intel vendor bytes for CPU detection.
 */
const char g_zSys_CpuVendorExpectedIntel[0x0d] = "GenuineIntel";

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-g-zsys-cpuvendorscratchpadinit
 * @recoil-artifact defines .data recoil:data:0x4e468c: g_zSys_CpuVendorScratchPadInit.
 * Purpose: initializes the 12-byte CPUID vendor scratch buffer.
 */
const char g_zSys_CpuVendorScratchPadInit[0x0d] = "------------";

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
 * Purpose: carries the CPUID vendor mismatch marker.
 */
extern "C" unsigned int g_zSys_CpuVendorNonIntelMarker = 0;

namespace zSys {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-zsys-getcpumhz
 * @recoil-artifact defines .text recoil:function:0x4b31c0: zSys::GetCpuMhz.
 * Purpose: resolve the current CPU benchmark packet and return the rounded MHz value.
 */
RECOIL_NO_GS int __cdecl GetCpuMhz() {
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-zsys-hascpuidsupport
 * @recoil-artifact defines .text recoil:function:0x4b33f0: zSys::HasCpuidSupport.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.has-cpuid-support
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.has-cpuid-support
 * Purpose: reports whether the processor supports toggling the CPUID EFLAGS ID
 * bit; VC5 C++ cannot express the required flag mutation, so this documented
 * raw-assembly CPU-probe exception keeps the retail EFLAGS sequence local.
 */
unsigned short __cdecl HasCpuidSupport() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    int result = 1;
    __asm {
        pushfd
        pop eax
        mov ecx, eax
        xor eax, 0200000h
        push eax
        popfd
        pushfd
        pop eax
        xor eax, ecx
        jne recoil_cpu_cpuid_support_done
        mov dword ptr [result], 0
    recoil_cpu_cpuid_support_done:
    }
    return result;
#else
    return 1;
#endif

}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-zsys-detectcpuclassandfeatures
 * @recoil-artifact defines .text recoil:function:0x4b3420: zSys::DetectCpuClassAndFeatures.
 * Purpose: classifies the CPU family and carries the non-Intel marker in bit 15.
 */
int __cdecl DetectCpuClassAndFeatures() {
    int result;
    if ((unsigned short)HasCpuidSupport() != 0) {
        result = ReadCpuidVendorAndFamily();
    } else {
        unsigned int divProbe = (unsigned int)ProbeDivZeroFlagBehavior();
        divProbe &= 0xffffu;
        g_zSys_CpuVendorNonIntelMarker = divProbe;
        result = DetectIs8086ByEflagsHiBits();
        if ((unsigned short)result != 0) {
            result = DetectIs80286ByEflagsHiBits();
            if ((unsigned short)result != 2) {
                result = DetectIs80386ByAcFlag();
                if ((unsigned short)result != 3) {
                    result = 4;
                }
            }
        }
    }
    if (g_zSys_CpuVendorNonIntelMarker != 0) {
        result |= 0x8000;
    }
    return result;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zsys-zsys_cpu-function-readcpuidfeatureflags
 * @recoil-artifact defines .text recoil:function:0x4b3480: zSys::ReadCpuidFeatureFlags.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.read-cpuid-feature-flags
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.read-cpuid-feature-flags
 * Purpose: reads CPUID feature flags after validating CPUID support and vendor
 * state; VC5 C++ has no CPUID intrinsic, so this documented raw-assembly
 * CPU-probe exception emits the opcode while preserving its fixed-register
 * vendor result.
 */
unsigned int __cdecl ReadCpuidFeatureFlags() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    struct CpuVendorBuffer { char bytes[0x0c]; };
    struct CpuFeatureScratch {
        CpuVendorBuffer expectedVendor;
        CpuVendorBuffer cpuidVendor;
        unsigned int result;
    };
    volatile CpuFeatureScratch scratch;
    __asm {
        mov eax, dword ptr [g_zSys_CpuVendorScratchPadInit]
        mov ecx, dword ptr [g_zSys_CpuVendorScratchPadInit + 4]
        mov edx, dword ptr [g_zSys_CpuVendorScratchPadInit + 8]
        mov dword ptr [scratch.cpuidVendor], eax
        mov eax, dword ptr [g_zSys_CpuVendorExpectedIntel]
        mov dword ptr [scratch.cpuidVendor + 4], ecx
        mov ecx, dword ptr [g_zSys_CpuVendorExpectedIntel + 4]
        mov dword ptr [scratch.cpuidVendor + 8], edx
        mov edx, dword ptr [g_zSys_CpuVendorExpectedIntel + 8]
        mov dword ptr [scratch.result], 0
        mov dword ptr [scratch.expectedVendor], eax
        mov dword ptr [scratch.expectedVendor + 4], ecx
        mov dword ptr [scratch.expectedVendor + 8], edx
    }
    if ((unsigned short)HasCpuidSupport() != 0) {
        __asm {
            xor eax, eax
            _emit 0x0f
            _emit 0xa2
            mov dword ptr [scratch.cpuidVendor], ebx
            mov dword ptr [scratch.cpuidVendor + 4], edx
            mov dword ptr [scratch.cpuidVendor + 8], ecx
            xor eax, eax
            mov ecx, 1
        recoil_cpu_feature_vendor_compare:
            mov dl, byte ptr [ebp + eax - 010h]
            mov bl, byte ptr [ebp + eax - 01ch]
            cmp dl, bl
            je recoil_cpu_feature_vendor_compare_next
            mov dword ptr [g_zSys_CpuVendorNonIntelMarker], ecx
        recoil_cpu_feature_vendor_compare_next:
            inc eax
            cmp eax, 0ch
            jl recoil_cpu_feature_vendor_compare
            cmp eax, 1
            jl recoil_cpu_feature_done
            xor eax, eax
            inc eax
            _emit 0x0f
            _emit 0xa2
            mov dword ptr [scratch.result], edx
        recoil_cpu_feature_done:
            mov eax, dword ptr [scratch.result]
        }
    }
    return scratch.result;
#else
    int cpuInfo[4] = {0};
    if (HasCpuidSupport() == 0) {
        return 0;
    }
    __cpuid(cpuInfo, 1);
    return (unsigned int)(cpuInfo[3]);
#endif
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-zsys-probedivzeroflagbehavior
 * @recoil-artifact defines .text recoil:function:0x4b3510: zSys::ProbeDivZeroFlagBehavior.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.probe-div-zero-flag-behavior
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.probe-div-zero-flag-behavior
 * Purpose: observe legacy DIV flag behavior; VC5 C++ cannot expose the needed
 * FLAGS result, so this documented raw-assembly CPU-probe exception keeps the
 * flag sequence local to the probe.
 */
int __cdecl ProbeDivZeroFlagBehavior() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    int result = 0;
    __asm {
        mov ax, 05555h
        xor dx, dx
        mov cx, 2
        div cx
        clc
        jne recoil_cpu_div_sets_carry
        jmp recoil_cpu_div_push_flags
    recoil_cpu_div_sets_carry:
        stc
    recoil_cpu_div_push_flags:
        pushf
        pop ax
        and al, 1
        xor al, 1
        mov word ptr [result], ax
    }
    return (unsigned char)result & 1;
#else
    return 0;
#endif
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-zsys-detectis8086byeflagshibits
 * @recoil-artifact defines .text recoil:function:0x4b3550: zSys::DetectIs8086ByEflagsHiBits.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.detect-is-8086-by-eflags-hi-bits
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.detect-is-8086-by-eflags-hi-bits
 * Purpose: classify 8086-style FLAGS high-bit behavior; VC5 C++ cannot read and
 * restore those flags directly, so this documented raw-assembly CPU-probe
 * exception keeps the exact flag sequence local.
 */
int __cdecl DetectIs8086ByEflagsHiBits() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    int result;
    __asm {
        mov dword ptr [result], 0ffffh
        pushf
        pop ax
        mov cx, ax
        and ax, 0fffh
        push ax
        popf
        pushf
        pop ax
        and ax, 0f000h
        cmp ax, 0f000h
        mov word ptr [result], 0
        je recoil_cpu_8086_restore_flags
        mov word ptr [result], 0ffffh
    recoil_cpu_8086_restore_flags:
        push cx
        popf
        mov ax, word ptr [result]
        mov ax, word ptr [result]
    }
#else
    return 0xffff;
#endif
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-zsys-detectis80286byeflagshibits
 * @recoil-artifact defines .text recoil:function:0x4b35a0: zSys::DetectIs80286ByEflagsHiBits.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.detect-is-80286-by-eflags-hi-bits
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.detect-is-80286-by-eflags-hi-bits
 * Purpose: classify 80286-style FLAGS high-bit behavior; VC5 C++ cannot read
 * and restore those flags directly, so this documented raw-assembly CPU-probe
 * exception keeps the exact flag sequence local.
 */
int __cdecl DetectIs80286ByEflagsHiBits() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    int result;
    __asm {
        mov dword ptr [result], 0ffffh
        pushf
        pop cx
        mov bx, cx
        or cx, 0f000h
        push cx
        popf
        pushf
        pop ax
        and ax, 0f000h
        mov word ptr [result], 2
        je recoil_cpu_286_restore_flags
        mov word ptr [result], 0ffffh
    recoil_cpu_286_restore_flags:
        push bx
        popf
        mov ax, word ptr [result]
        mov ax, word ptr [result]
    }
#else
    return 0xffff;
#endif
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-zsys-detectis80386byacflag
 * @recoil-artifact defines .text recoil:function:0x4b35f0: zSys::DetectIs80386ByAcFlag.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.detect-is-80386-by-ac-flag
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.detect-is-80386-by-ac-flag
 * Purpose: classify 80386-style AC-flag behavior; VC5 C++ cannot toggle and
 * restore EFLAGS directly, so this documented raw-assembly CPU-probe exception
 * keeps the exact flag and stack-alignment sequence local.
 */
int __cdecl DetectIs80386ByAcFlag() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    int result;
    __asm {
        mov dword ptr [result], 0ffffh
        mov bx, sp
        and sp, 0fffch
        pushfd
        pop eax
        mov ecx, eax
        xor eax, 040000h
        push eax
        popfd
        pushfd
        pop eax
        xor eax, ecx
        mov word ptr [result], 3
        je recoil_cpu_386_restore_flags
        mov word ptr [result], 0ffffh
    recoil_cpu_386_restore_flags:
        push ecx
        popfd
        mov sp, bx
        mov ax, word ptr [result]
        and eax, 0ffffh
        mov ax, word ptr [result]
    }
#else
    return 0xffff;
#endif
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zsys-zsys_cpu-function-readcpuidvendorandfamily
 * @recoil-artifact defines .text recoil:function:0x4b3640: zSys::ReadCpuidVendorAndFamily.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.read-cpuid-vendor-and-family
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.read-cpuid-vendor-and-family
 * Purpose: reads CPUID vendor/family data and records non-Intel state; VC5 C++
 * has no CPUID intrinsic and did not preserve the retail register/byte shape,
 * so this documented raw-assembly CPU-probe exception emits the required
 * opcodes locally.
 */
int __cdecl ReadCpuidVendorAndFamily() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    struct CpuVendorBuffer { char bytes[0x0c]; };
    CpuVendorBuffer cpuidVendor;
    CpuVendorBuffer expectedVendor;
    unsigned int cpuidFamily;
    unsigned char cpuidModel;
    unsigned char cpuidStepping;
    __asm {
        mov eax, dword ptr [g_zSys_CpuVendorScratchPadInit]
        mov ecx, dword ptr [g_zSys_CpuVendorScratchPadInit + 4]
        mov edx, dword ptr [g_zSys_CpuVendorScratchPadInit + 8]
        _emit 0x89
        _emit 0x45
        _emit 0xec
        mov eax, dword ptr [g_zSys_CpuVendorExpectedIntel]
        _emit 0x89
        _emit 0x4d
        _emit 0xf0
        mov ecx, dword ptr [g_zSys_CpuVendorExpectedIntel + 4]
        _emit 0x89
        _emit 0x55
        _emit 0xf4
        mov edx, dword ptr [g_zSys_CpuVendorExpectedIntel + 8]
        _emit 0x53
        mov dword ptr [cpuidFamily], 0ffffh
        mov byte ptr [cpuidStepping], 0
        mov dword ptr [expectedVendor], eax
        mov dword ptr [expectedVendor + 4], ecx
        mov dword ptr [expectedVendor + 8], edx
        xor eax, eax
        _emit 0x0f
        _emit 0xa2
        _emit 0x89
        _emit 0x5d
        _emit 0xec
        mov dword ptr [cpuidVendor + 4], edx
        mov dword ptr [cpuidVendor + 8], ecx
        xor eax, eax
        mov ecx, 1
    recoil_cpu_vendor_family_compare:
        _emit 0x8a
        _emit 0x54
        _emit 0x05
        _emit 0xec
        _emit 0x8a
        _emit 0x5c
        _emit 0x05
        _emit 0xe0
        _emit 0x3a
        _emit 0xd3
        je recoil_cpu_vendor_family_compare_next
        mov dword ptr [g_zSys_CpuVendorNonIntelMarker], ecx
    recoil_cpu_vendor_family_compare_next:
        inc eax
        cmp eax, 0ch
        jl recoil_cpu_vendor_family_compare
        cmp eax, 1
        jl recoil_cpu_vendor_family_done
        xor eax, eax
        inc eax
        _emit 0x0f
        _emit 0xa2
        mov byte ptr [cpuidStepping], al
        and byte ptr [cpuidStepping], 0fh
        and al, 0f0h
        shr al, 4
        mov byte ptr [cpuidModel], al
        and eax, 0f00h
        shr eax, 8
        and eax, 0fh
        mov word ptr [cpuidFamily], ax
    recoil_cpu_vendor_family_done:
        mov ax, word ptr [cpuidFamily]
        mov ax, word ptr [cpuidFamily]
        _emit 0x5b
    }
#else
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);
    char vendor[0x0c];
    memcpy(&vendor[0], &cpuInfo[1], 4);
    memcpy(&vendor[4], &cpuInfo[3], 4);
    memcpy(&vendor[8], &cpuInfo[2], 4);
    if (memcmp(vendor, g_zSys_CpuVendorExpectedIntel, sizeof(vendor)) != 0) {
        g_zSys_CpuVendorNonIntelMarker = 1;
    }
    __cpuid(cpuInfo, 1);
    return (cpuInfo[0] >> 8) & 0x0f;
#endif
}

} // namespace zSys

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-cpubenchmarkresolver-resolvecpubenchmarkpacket
 * @recoil-artifact defines .text recoil:function:0x4b36f0: CpuBenchmarkResolver::ResolveCpuBenchmarkPacket.
 * Purpose: chooses the CPU benchmark strategy and writes the result packet.
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
    int forcedLowHint = 0;
    if (cpuClassHint <= 0) {
        expectedCycles = g_zSys_CpuBenchmarkDurationTable[cpuClass & 0xffff] * 4000u;
    } else if (cpuClassHint <= 0x96) {
        forcedLowHint = 1;
        expectedCycles = (unsigned int)(cpuClassHint) * 4000u;
    } else {
        expectedCycles = (unsigned int)((unsigned int)(outBuffer));
    }
    zSys::CpuBenchmarkResult localResult;
    zSys::CpuBenchmarkResult *measured;
    if ((featureFlags & 0x10u) != 0 && !forcedLowHint) {
        if (cpuClassHint == 0) {
            measured = ((CpuBenchmarkResolver *)expectedCycles)->MeasureCpuMhz_RdtscQpc(&localResult);
        } else {
            measured = ((CpuBenchmarkResolver *)expectedCycles)->MeasureCpuMhz_CmosRtc(&localResult);
        }
    } else if ((cpuClass & 0xffff) >= 3) {
        measured = ((CpuBenchmarkResolver *)expectedCycles)->MeasureMhzViaBsfLoop_Qpc(&localResult);
    } else {
        outBuffer->totalCycles = 0;
        outBuffer->totalMicroseconds = 0;
        outBuffer->cpuMhzRaw = 0;
        outBuffer->cpuMhzRounded = 0;
        return outBuffer;
    }
    *outBuffer = *measured;
    return outBuffer;
}

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.cpu-benchmark-resolver.measure-mhz-via-bsf-loop-qpc
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.cpu-benchmark-resolver.measure-mhz-via-bsf-loop-qpc recoil:function:0x4b37f0
 * Original function evidence: retail 0x4b37f0 contains this exact CPU timing body.
 * Purpose: measures CPU MHz with the fixed BSF/QPC loop; VC5 C++ did not
 * preserve the retail fixed-register loop or epilogue, so this documented CPU
 * raw-assembly timing exception retains the exact address-scoped benchmark
 * body.
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
 * Original function evidence: retail 0x4b37f0 has this portable conditional definition.
 * Purpose: provide the portable fallback for the fixed BSF/QPC CPU benchmark
 * when the address-scoped VC5 x86 raw-assembly exception is not enabled.
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
    for (int sample = 0; sample < 10; ++sample) {
        LARGE_INTEGER start;
        LARGE_INTEGER end;
        QueryPerformanceCounter(&start);
        for (unsigned short i = 0x0fa0; i != 1; --i) {
            volatile unsigned int value = 0x80000000u;
            (void)value;
        }
        QueryPerformanceCounter(&end);
        const unsigned int ticks = (unsigned int)(end.LowPart - start.LowPart);
        if (ticks < minTicks) {
            minTicks = ticks;
        }
    }
    const unsigned int expectedCycles = (unsigned int)((unsigned int)(this));
    const unsigned int microseconds = frequency.LowPart == 0
        ? 0
        : (unsigned int)(((unsigned __int64)(minTicks) * 1000000ui64) /
                         (unsigned int)(frequency.LowPart));
    const unsigned int raw = microseconds == 0 ? 0 : expectedCycles / microseconds;
    outBuffer->totalCycles = expectedCycles;
    outBuffer->totalMicroseconds = microseconds;
    outBuffer->cpuMhzRaw = raw;
    outBuffer->cpuMhzRounded = raw;
    return outBuffer;
}
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.cpu-benchmark-resolver.measure-cpu-mhz-rdtsc-qpc
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.cpu-benchmark-resolver.measure-cpu-mhz-rdtsc-qpc recoil:function:0x4b38e0
 * Original function evidence: retail 0x4b38e0 contains this exact CPU timing body.
 * Purpose: measures CPU MHz with the RDTSC/QPC sampling loop; VC5 C++ cannot
 * issue RDTSC or preserve the retail priority/register/epilogue shape, so this
 * documented raw-assembly CPU timing exception retains the address-scoped
 * benchmark body.
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
 * Original function evidence: retail 0x4b38e0 has this portable conditional definition.
 * Purpose: provide the portable fallback for the RDTSC/QPC CPU benchmark when
 * the address-scoped VC5 x86 raw-assembly exception is not enabled.
 */
zSys::CpuBenchmarkResult * CpuBenchmarkResolver::MeasureCpuMhz_RdtscQpc(
    zSys::CpuBenchmarkResult *outBuffer
) {
    LARGE_INTEGER frequency;
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    if (QueryPerformanceFrequency(&frequency) == 0) {
        memset(outBuffer, 0, sizeof(*outBuffer));
        return outBuffer;
    }
    QueryPerformanceCounter(&start);
    QueryPerformanceCounter(&end);
    outBuffer->totalCycles = (unsigned int)(end.LowPart - start.LowPart);
    outBuffer->totalMicroseconds = 1;
    outBuffer->cpuMhzRaw = (int)(outBuffer->totalCycles);
    outBuffer->cpuMhzRounded = outBuffer->cpuMhzRaw;
    return outBuffer;
}
#endif

namespace zSys {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-zsys-readcmosrtcsecondsbcd
 * @recoil-artifact defines .text recoil:function:0x4b3b00: zSys::ReadCmosRtcSecondsBcd.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.read-cmos-rtc-seconds-bcd
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.read-cmos-rtc-seconds-bcd
 * Purpose: read the CMOS RTC seconds byte for CPU timing; VC5 C++ cannot issue
 * the required port I/O, so this documented raw-assembly CPU timing exception
 * keeps the address-scoped IN/OUT sequence local.
 */
unsigned int __cdecl ReadCmosRtcSecondsBcd() {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    unsigned int secondsBcd = 0;
    __asm {
        xor ax, ax
        out 70h, al
        xor ax, ax
        in al, 71h
        mov word ptr [secondsBcd], ax
    }
    return secondsBcd;
#else
    const unsigned int seconds = (unsigned int)((GetTickCount() / 1000u) % 60u);
    return (unsigned int)(((seconds / 10u) << 4) | (seconds % 10u));
#endif
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zsys-zsys_cpu-function-readtsc64
 * @recoil-artifact defines .text recoil:function:0x4b3b20: zSys::ReadTsc64.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.read-tsc64
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.read-tsc64
 * Purpose: read the split 64-bit timestamp counter; VC5 C++ has no RDTSC
 * intrinsic, so this documented raw-assembly CPU timing exception emits the
 * opcode and stores the fixed-register result directly.
 */
void __fastcall ReadTsc64(
    unsigned int *outHigh,
    unsigned int *outLow
) {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    unsigned int tscHigh = 0;
    unsigned int tscLow = 0;
    __asm {
        _emit 0x0f
        _emit 0x31
        mov dword ptr [tscLow], eax
        mov dword ptr [tscHigh], edx
        mov eax, dword ptr [tscHigh]
        mov dword ptr [ecx], eax
        mov eax, dword ptr [tscLow]
        mov dword ptr [edx], eax
    }
#else
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    *outHigh = (unsigned int)(counter.HighPart);
    *outLow = (unsigned int)(counter.LowPart);
#endif
}

} // namespace zSys
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.cpu-benchmark-resolver.measure-cpu-mhz-cmos-rtc
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.cpu-benchmark-resolver.measure-cpu-mhz-cmos-rtc recoil:function:0x4b3b50
 * Original function evidence: retail 0x4b3b50 contains this exact CPU timing body.
 * Purpose: measures CPU MHz against CMOS RTC second transitions; VC5 C++ did
 * not preserve the retail port-I/O/TSC coordination or epilogue, so this
 * documented raw-assembly CPU timing exception retains the address-scoped
 * benchmark body.
 */
__declspec(naked) zSys::CpuBenchmarkResult * CpuBenchmarkResolver::MeasureCpuMhz_CmosRtc(
    zSys::CpuBenchmarkResult *
) {
    __asm {
        sub esp, 028h
        push ebx
        push ebp
        push esi
        push edi
        call dword ptr [GetCurrentThread]
        mov ebp, eax
        xor eax, eax
        mov dword ptr [esp + 028h], eax
        push ebp
        mov dword ptr [esp + 030h], eax
        mov dword ptr [esp + 034h], eax
        mov dword ptr [esp + 038h], eax
        call dword ptr [GetThreadPriority]
        mov ebx, eax
        cmp ebx, 07fffffffh
        je recoil_cpu_cmos_priority_ready
        lea ecx, [ebx + 001h]
        push ecx
        push ebp
        call dword ptr [SetThreadPriority]
    recoil_cpu_cmos_priority_ready:
        call ReadCmosRtcSecondsBcd
        mov esi, eax
    recoil_cpu_cmos_wait_start:
        call ReadCmosRtcSecondsBcd
        mov edi, eax
        cmp edi, esi
        jge recoil_cpu_cmos_start_forward
        sub eax, esi
        add eax, 00ah
        jmp recoil_cpu_cmos_start_delta_ready
    recoil_cpu_cmos_start_forward:
        mov edx, edi
        xor eax, eax
        sub edx, esi
        test edx, edx
        setg al
    recoil_cpu_cmos_start_delta_ready:
        test eax, eax
        je recoil_cpu_cmos_wait_start
        lea edx, [esp + 018h]
        lea ecx, [esp + 01ch]
        call ReadTsc64
    recoil_cpu_cmos_wait_end:
        call ReadCmosRtcSecondsBcd
        mov esi, eax
        cmp esi, edi
        jge recoil_cpu_cmos_end_forward
        sub eax, edi
        add eax, 00ah
        jmp recoil_cpu_cmos_end_delta_ready
    recoil_cpu_cmos_end_forward:
        sub eax, edi
        xor ecx, ecx
        test eax, eax
        setg cl
        mov eax, ecx
    recoil_cpu_cmos_end_delta_ready:
        test eax, eax
        je recoil_cpu_cmos_wait_end
        lea edx, [esp + 010h]
        lea ecx, [esp + 014h]
        call ReadTsc64
        cmp ebx, 07fffffffh
        je recoil_cpu_cmos_priority_restored
        push ebx
        push ebp
        call dword ptr [SetThreadPriority]
    recoil_cpu_cmos_priority_restored:
        mov ecx, dword ptr [esp + 010h]
        lea edx, [esp + 020h]
        lea eax, [esp + 024h]
        push edx
        mov edx, dword ptr [esp + 018h]
        push eax
        push ecx
        mov ecx, dword ptr [esp + 028h]
        push edx
        mov edx, dword ptr [esp + 028h]
        call Sub64
        mov ebp, dword ptr [esp + 020h]
        mov eax, 0431bde83h
        mul ebp
        mov eax, 04f8b588fh
        mov ebx, edx
        mul ebp
        mov eax, ebp
        sub eax, edx
        shr ebx, 012h
        shr eax, 1
        add eax, edx
        lea ecx, [ebx + ebx * 4]
        shr eax, 010h
        shl ecx, 1
        sub eax, ecx
        mov dword ptr [esp + 030h], ebx
        cmp eax, 6
        jb recoil_cpu_cmos_mhz_rounded
        inc ebx
    recoil_cpu_cmos_mhz_rounded:
        lea eax, [edi + edi * 4]
        pop edi
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea edx, [eax + eax * 4]
        lea eax, [esi + esi * 4]
        shl edx, 6
        lea eax, [eax + eax * 4]
        pop esi
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea eax, [eax + eax * 4]
        lea ecx, [eax + eax * 4]
        mov eax, dword ptr [esp + 034h]
        shl ecx, 6
        sub ecx, edx
        mov edx, eax
        mov dword ptr [edx], ebp
        pop ebp
        mov dword ptr [edx + 004h], ecx
        mov ecx, dword ptr [esp + 024h]
        mov dword ptr [edx + 008h], ecx
        mov dword ptr [edx + 00ch], ebx
        pop ebx
        add esp, 028h
        ret 4
    }
}
#else
/**
 * Original function evidence: retail 0x4b3b50 has this portable conditional definition.
 * Purpose: provide the portable fallback for the CMOS/TSC CPU benchmark when
 * the address-scoped VC5 x86 raw-assembly exception is not enabled.
 */
zSys::CpuBenchmarkResult * CpuBenchmarkResolver::MeasureCpuMhz_CmosRtc(
    zSys::CpuBenchmarkResult *outBuffer
) {
    unsigned int high0 = 0, low0 = 0, high1 = 0, low1 = 0;
    zSys::ReadTsc64(&high0, &low0);
    zSys::ReadTsc64(&high1, &low1);
    unsigned int elapsedHigh = 0, elapsedLow = 0;
    zSys::Sub64(high0, low0, high1, low1, &elapsedHigh, &elapsedLow);
    outBuffer->totalCycles = elapsedLow;
    outBuffer->totalMicroseconds = 1000000u;
    outBuffer->cpuMhzRaw = (int)(elapsedLow / 1000000u);
    outBuffer->cpuMhzRounded = outBuffer->cpuMhzRaw;
    return outBuffer;
}
#endif

namespace zSys {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsys-zsys-cpu-zsys-sub64
 * @recoil-artifact defines .text recoil:function:0x4b3ca0: zSys::Sub64.
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsys.sub64
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsys.sub64
 * Purpose: subtract split 64-bit CPU timing values with the retail borrow and
 * register-store sequence; VC5 C++ did not preserve that byte shape, so this
 * documented raw-assembly CPU timing exception keeps the address-scoped
 * sequence local.
 */
void __fastcall Sub64(
    unsigned int subHigh,
    unsigned int subLow,
    unsigned int minuendHigh,
    unsigned int minuendLow,
    unsigned int *outHigh,
    unsigned int *outLow
) {
#if defined(_MSC_VER) && defined(_M_IX86) && defined(RECOIL_ENABLE_ZSYS_CPU_RAW_ASM)
    struct Sub64Scratch {
        unsigned int resultHigh;
        unsigned int resultLow;
        unsigned int savedSubLow;
        unsigned int savedSubHigh;
    };
    Sub64Scratch scratch;
    __asm {
        _emit 0x53
        mov dword ptr [scratch.savedSubLow], edx
        mov dword ptr [scratch.savedSubHigh], ecx
        mov eax, dword ptr [minuendLow]
        _emit 0x8b
        _emit 0x5d
        _emit 0xf8
        _emit 0x2b
        _emit 0xc3
        mov dword ptr [scratch.resultLow], eax
        mov eax, dword ptr [minuendHigh]
        _emit 0x8b
        _emit 0x5d
        _emit 0xfc
        _emit 0x1b
        _emit 0xc3
        mov dword ptr [scratch.resultHigh], eax
        mov eax, dword ptr [outLow]
        mov ecx, dword ptr [scratch.resultLow]
        mov edx, dword ptr [outHigh]
        _emit 0x5b
        mov dword ptr [eax], ecx
        mov ecx, dword ptr [scratch.resultHigh]
        mov dword ptr [edx], ecx
        mov eax, dword ptr [eax]
    }
#else
    const unsigned __int64 subtrahend =
        ((unsigned __int64)(subHigh) << 32) | subLow;
    const unsigned __int64 minuend =
        ((unsigned __int64)(minuendHigh) << 32) | minuendLow;
    const unsigned __int64 result = minuend - subtrahend;
    *outHigh = (unsigned int)(result >> 32);
    *outLow = (unsigned int)(result);
#endif
}

} // namespace zSys
