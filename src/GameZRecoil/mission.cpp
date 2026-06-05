#include "GameZRecoil/mission.h"

#include "Battlesport/HudSensorTracker.h"

namespace Mission {
/**
 * Reimplements 0x417350: Mission::InitObjectives (D:\Proj\GameZRecoil\mission.cpp).
 *
 * Purpose: initialize the global HUD sensor objective tracker and register its
 * process-exit cleanup hook.
 */
void InitObjectives() {
    HudSensorTracker::ConstructGlobal();
    HudSensorTracker::RegisterGlobalOnExit();
}
} // namespace Mission
