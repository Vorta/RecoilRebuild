#if defined(_MSC_VER)
#pragma warning(disable : 4035)
#pragma warning(disable : 4101)
#pragma warning(disable : 4715)
#endif

namespace zSys {
// Reimplements 0x4b33f0: zSys::HasCpuidSupport
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL HasCpuidSupport() {
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
        mov ax, word ptr [result]
    }
}

// Reimplements 0x4b3640: zSys::ReadCpuidVendorAndFamily
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL ReadCpuidVendorAndFamily() {
    struct CpuVendorBuffer {
        char bytes[0x0c];
    };
    static const CpuVendorBuffer cpuidVendorSeed = {
        {'A', 'u', 't', 'h', 'e', 'n', 't', 'i', 'c', 'A', 'M', 'D'}};
    static const CpuVendorBuffer genuineIntelVendor = {
        {'G', 'e', 'n', 'u', 'i', 'n', 'e', 'I', 'n', 't', 'e', 'l'}};

    // Volatile stack buffers preserve the original aggregate-copy setup before inline cpuid.
    volatile CpuVendorBuffer cpuidVendor = cpuidVendorSeed;
    int family = 0xffff;
    unsigned char model = 0;
    unsigned char stepping = 0;
    volatile CpuVendorBuffer expectedVendor = genuineIntelVendor;
    __asm {
        xor eax, eax
        cpuid
        mov dword ptr [cpuidVendor], ebx
        mov dword ptr [cpuidVendor + 4], edx
        mov dword ptr [cpuidVendor + 8], ecx
        xor eax, eax
        mov ecx, 1
    recoil_cpu_vendor_compare:
        mov dl, byte ptr [ebp + eax - 014h]
        mov bl, byte ptr [ebp + eax - 020h]
        cmp dl, bl
        je recoil_cpu_vendor_compare_next
        mov dword ptr [g_zSys_CpuVendorNonIntelMarker], ecx
    recoil_cpu_vendor_compare_next:
        inc eax
        cmp eax, 0ch
        jl recoil_cpu_vendor_compare
        cmp eax, 1
        jl recoil_cpu_vendor_done
        xor eax, eax
        inc eax
        cpuid
        mov byte ptr [stepping], al
        and byte ptr [stepping], 0fh
        and al, 0f0h
        shr al, 4
        mov byte ptr [model], al
        and eax, 0f00h
        shr eax, 8
        and eax, 0fh
        mov word ptr [family], ax
    recoil_cpu_vendor_done:
        mov ax, word ptr [family]
        mov ax, word ptr [family]
    }
}

// Reimplements 0x4b3510: zSys::ProbeDivZeroFlagBehavior
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL ProbeDivZeroFlagBehavior() {
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
        mov al, byte ptr [result]
        and eax, 1
    }
}

// Reimplements 0x4b3550: zSys::DetectIs8086ByEflagsHiBits
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL DetectIs8086ByEflagsHiBits() {
    int result = 0xffff;
    __asm {
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

// Reimplements 0x4b35a0: zSys::DetectIs80286ByEflagsHiBits
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL DetectIs80286ByEflagsHiBits() {
    int result = 0xffff;
    __asm {
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

// Reimplements 0x4b35f0: zSys::DetectIs80386ByAcFlag
// (D:\Proj\GameZRecoil\zSys\zsys_cpu.cpp)
RECOIL_NOINLINE int RECOIL_CDECL DetectIs80386ByAcFlag() {
    int result = 0xffff;
    __asm {
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
