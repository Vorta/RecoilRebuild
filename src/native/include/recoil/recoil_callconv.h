#pragma once

#if defined(_MSC_VER)
#define RECOIL_CDECL __cdecl
#define RECOIL_STDCALL __stdcall
#define RECOIL_FASTCALL __fastcall
#if _MSC_VER >= 1300
#define RECOIL_THISCALL __thiscall
#else
#define RECOIL_THISCALL
#endif
#if _MSC_VER >= 1400
#define RECOIL_NO_GS __declspec(safebuffers)
#else
#define RECOIL_NO_GS
#endif
#if _MSC_VER >= 1300
#define RECOIL_NOINLINE __declspec(noinline)
#else
#define RECOIL_NOINLINE
#endif
#define RECOIL_FORCEINLINE __forceinline
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#define RECOIL_CDECL __attribute__((cdecl))
#define RECOIL_STDCALL __attribute__((stdcall))
#define RECOIL_FASTCALL __attribute__((fastcall))
#define RECOIL_THISCALL __attribute__((thiscall))
#define RECOIL_NO_GS
#define RECOIL_NOINLINE __attribute__((noinline))
#define RECOIL_FORCEINLINE inline __attribute__((always_inline))
#else
#define RECOIL_CDECL
#define RECOIL_STDCALL
#define RECOIL_FASTCALL
#define RECOIL_THISCALL
#define RECOIL_NO_GS
#define RECOIL_NOINLINE
#define RECOIL_FORCEINLINE inline
#endif
