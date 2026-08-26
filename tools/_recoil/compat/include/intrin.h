#pragma once

#if defined(_MSC_VER)
extern "C" void __cdecl __cpuid(
    int cpuInfo[4],
    int functionId
);
#if _MSC_VER >= 1300
#pragma intrinsic(__cpuid)
#endif
#endif
