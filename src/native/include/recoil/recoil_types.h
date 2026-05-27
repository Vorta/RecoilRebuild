#pragma once

#include <stddef.h>

#if defined(__cplusplus) && (!defined(_MSC_VER) || _MSC_VER >= 1300)
#include <new>
#endif

namespace recoil {
typedef unsigned int Ptr32;
typedef unsigned int Fn32;
}

#define RECOIL_STATIC_ASSERT_JOIN_DETAIL(a, b) a##b
#define RECOIL_STATIC_ASSERT_JOIN(a, b) RECOIL_STATIC_ASSERT_JOIN_DETAIL(a, b)
#if defined(_MSC_VER) && _MSC_VER < 1300
#define RECOIL_STATIC_ASSERT(expr)                                                              \
    extern char RECOIL_STATIC_ASSERT_JOIN(recoil_static_assert_, __LINE__)[(expr) ? 1 : -1]
#else
#define RECOIL_STATIC_ASSERT(expr)                                                              \
    typedef char RECOIL_STATIC_ASSERT_JOIN(recoil_static_assert_, __LINE__)[(expr) ? 1 : -1]
#endif

RECOIL_STATIC_ASSERT(sizeof(recoil::Ptr32) == 4);
RECOIL_STATIC_ASSERT(sizeof(recoil::Fn32) == 4);

#if defined(__cplusplus) && (!defined(_MSC_VER) || _MSC_VER < 1300)
#if defined(_MSC_VER)
#define RECOIL_PLACEMENT_NEW_CDECL __cdecl
#else
#define RECOIL_PLACEMENT_NEW_CDECL
#endif
inline void *RECOIL_PLACEMENT_NEW_CDECL operator new(size_t, void *place)
{
    return place;
}

#if !defined(_MSC_VER)
inline void RECOIL_PLACEMENT_NEW_CDECL operator delete(void *, void *)
{
}
#endif
#undef RECOIL_PLACEMENT_NEW_CDECL
#endif
