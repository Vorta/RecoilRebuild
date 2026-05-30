#pragma once

#include "Battlesport/Mfc42Abi.h"
#include "recoil/recoil_callconv.h"

#if defined(_MSC_VER) && _MSC_VER >= 1300
#define RECOIL_WOL_CONFIG_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define RECOIL_WOL_CONFIG_NOINLINE __attribute__((noinline))
#else
#define RECOIL_WOL_CONFIG_NOINLINE
#endif

class WestwoodOnlineUpgradeConfigDialog {
  public:
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    static const AFX_MSGMAP *RECOIL_STDCALL GetBaseMessageMapForMfc();
    RECOIL_WOL_CONFIG_NOINLINE const AFX_MSGMAP *RECOIL_THISCALL GetMessageMap() const;
};
