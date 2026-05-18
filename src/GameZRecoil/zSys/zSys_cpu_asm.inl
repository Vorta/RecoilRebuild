#if defined(_MSC_VER)
#pragma warning(disable : 4035)
#pragma warning(disable : 4715)
#endif

namespace zSys {
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
