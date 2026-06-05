#pragma once

#if defined(_MSC_VER)
#if _MSC_VER >= 1400
#define RECOIL_NO_GS __declspec(safebuffers)
#else
#define RECOIL_NO_GS
#endif
#elif defined(__i386__) && (defined(__GNUC__) || defined(__clang__))
#ifndef __stdcall
#define __stdcall __attribute__((stdcall))
#endif
#ifndef __fastcall
#define __fastcall __attribute__((fastcall))
#endif
#define RECOIL_NO_GS
#else
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __fastcall
#define __fastcall
#endif
#define RECOIL_NO_GS
#endif
