#include "Battlesport/wol_ref_count_and_lock.h"

/**
 * Reimplements 0x441600: WestwoodOnlineUpgradeRefCountAndLock::Init.
 * Purpose: Resets the embedded reference count and initializes its critical section.
 */
WestwoodOnlineUpgradeRefCountAndLock * WestwoodOnlineUpgradeRefCountAndLock::Init() {
    refCount = 0;
    InitializeCriticalSection(&lock);
    return this;
}
