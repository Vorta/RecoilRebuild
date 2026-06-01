#include "Battlesport/WestwoodOnlineUpgradeRefCountAndLock.h"

// Reimplements 0x441600: WestwoodOnlineUpgradeRefCountAndLock::Init
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeRefCountAndLock.cpp)
RECOIL_NOINLINE WestwoodOnlineUpgradeRefCountAndLock *RECOIL_THISCALL
WestwoodOnlineUpgradeRefCountAndLock::Init() {
    refCount = 0;
    InitializeCriticalSection(&lock);
    return this;
}
