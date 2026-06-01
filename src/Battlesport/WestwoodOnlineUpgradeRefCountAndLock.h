#pragma once

#include <stddef.h>

#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <windows.h>

struct WestwoodOnlineUpgradeRefCountAndLock {
    long refCount;
    CRITICAL_SECTION lock;

    RECOIL_NOINLINE WestwoodOnlineUpgradeRefCountAndLock *RECOIL_THISCALL Init();
};

RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeRefCountAndLock) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeRefCountAndLock,
        refCount
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeRefCountAndLock,
        lock
    ) == 0x04
);
