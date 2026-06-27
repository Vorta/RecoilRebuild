#if defined(_MSC_VER)
#include <conio.h>
#pragma intrinsic(_inp)
#pragma intrinsic(_outp)
#pragma warning(disable : 4035)
#pragma warning(disable : 4101)
#pragma warning(disable : 4715)
#endif

/**
 * Reimplements 0x4b3020: zCpu::HasMmxSupport.
 * Purpose: Probes MMX support with the documented zSys CPU raw-assembly CPUID exception.
 */
int zCpu::HasMmxSupport() {
    int result;
    __asm {
        push ebx
        push ecx
        push edx
        mov eax, 1
        _emit 0x0f
        _emit 0xa2
        test edx, 0800000h
        jne recoil_cpu_mmx_support_done
        xor eax, eax
    recoil_cpu_mmx_support_done:
        mov dword ptr [result], eax
        pop edx
        pop ecx
        pop ebx
    }
    return result != 0 ? 1 : 0;
}

namespace zSys {
/**
 * Reimplements 0x4b33f0: zSys::HasCpuidSupport (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Reports whether the processor supports toggling the CPUID EFLAGS ID bit.
 */
unsigned short HasCpuidSupport() {
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
}

/**
 * Reimplements 0x4b2fe0: zSys::HasCpuidSupportRuntimeOptions.
 * Purpose: Repeats the EFLAGS ID-bit probe for runtime option setup callers.
 */
int HasCpuidSupportRuntimeOptions() {
    int changedFlags = 0;
    __asm {
        push ebx
        push ecx
        push edx
        pushfd
        pop eax
        mov ecx, eax
        xor eax, 0200000h
        push eax
        popfd
        pushfd
        pop eax
        xor eax, ecx
        mov dword ptr [changedFlags], eax
        pop edx
        pop ecx
        pop ebx
    }
    return changedFlags != 0 ? 1 : 0;
}

/**
 * Reimplements 0x4b3050: zSys::CheckCpuSignatureMask (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Reads CPUID leaf 1 and tests the CPU signature mask for optimized-path selection.
 */
int CheckCpuSignatureMask() {
    unsigned int cpuidSignature;
    __asm {
        xor esi, esi
        push ebx
        push ecx
        push edx
        mov eax, 1
        _emit 0x0f
        _emit 0xa2
        mov dword ptr [cpuidSignature], eax
        pop edx
        pop ecx
        pop ebx
        mov eax, dword ptr [cpuidSignature]
        and eax, 0630h
        cmp eax, 0630h
        jne recoil_cpu_signature_done
        mov esi, 1
    recoil_cpu_signature_done:
        mov eax, esi
    }
}

/**
 * Reimplements 0x4b3480: zSys::ReadCpuidFeatureFlags (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Reads CPUID feature flags after validating CPUID support and Intel vendor state.
 */
unsigned int ReadCpuidFeatureFlags() {
    struct CpuVendorBuffer {
        char bytes[0x0c];
    };
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
}

/**
 * Reimplements 0x4b3640: zSys::ReadCpuidVendorAndFamily (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Reads CPUID vendor/family data and records whether the vendor is non-Intel.
 */
int ReadCpuidVendorAndFamily() {
    struct CpuVendorBuffer {
        char bytes[0x0c];
    };

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
}

/**
 * Reimplements 0x4b3b00: zSys::ReadCmosRtcSecondsBcd (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Selects CMOS RTC register 0 and returns the raw seconds BCD byte.
 */
unsigned int ReadCmosRtcSecondsBcd() {
    unsigned int secondsBcd = 0;
    __asm {
        xor ax, ax
        out 70h, al
        xor ax, ax
        in al, 71h
        mov word ptr [secondsBcd], ax
    }
    return secondsBcd;
}

/**
 * Reimplements 0x4b3b20: zSys::ReadTsc64 (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Reads the timestamp counter into split high/low output storage for CPU timing.
 */
void __fastcall ReadTsc64(
    unsigned int *outHigh,
    unsigned int *outLow
) {
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
}

/**
 * Reimplements 0x4b3ca0: zSys::Sub64 (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Subtracts split 64-bit values with the retail frame-local sub/sbb helper shape.
 */
void __fastcall Sub64(
    unsigned int subHigh,
    unsigned int subLow,
    unsigned int minuendHigh,
    unsigned int minuendLow,
    unsigned int *outHigh,
    unsigned int *outLow
) {
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
}

/**
 * Reimplements 0x4b3510: zSys::ProbeDivZeroFlagBehavior (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Probes legacy divide/flag behavior used by the no-CPUID CPU-class fallback.
 */
int ProbeDivZeroFlagBehavior() {
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
}

/**
 * Reimplements 0x4b3550: zSys::DetectIs8086ByEflagsHiBits (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Detects 8086-class FLAGS high-bit behavior for CPU classification.
 */
int DetectIs8086ByEflagsHiBits() {
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
}

/**
 * Reimplements 0x4b35a0: zSys::DetectIs80286ByEflagsHiBits (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Detects 80286-class FLAGS high-bit behavior for CPU classification.
 */
int DetectIs80286ByEflagsHiBits() {
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
}

/**
 * Reimplements 0x4b35f0: zSys::DetectIs80386ByAcFlag (GameZRecoil/zSys/zsys_cpu.cpp).
 * Purpose: Toggles the EFLAGS AC bit to distinguish 80386-class behavior from later CPUs.
 */
int DetectIs80386ByAcFlag() {
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
}
} // namespace zSys
