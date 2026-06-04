#include "GameZRecoil/mission.h"

#include "Battlesport/HudSensorTracker.h"

namespace Mission {
// Reimplements 0x417350: Mission::InitObjectives (D:\Proj\GameZRecoil\mission.cpp)
void InitObjectives() {
    HudSensorTracker::ConstructGlobal();
    HudSensorTracker::RegisterGlobalOnExit();
}
} // namespace Mission
