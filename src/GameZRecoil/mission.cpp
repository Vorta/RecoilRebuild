#include "GameZRecoil/mission.h"

#include "Battlesport/HudSensorTracker.h"

namespace Mission {
// Reimplements 0x417350: Mission::InitObjectives (D:\Proj\GameZRecoil\mission.cpp)
RECOIL_NOINLINE void RECOIL_CDECL InitObjectives() {
    HudSensorTracker::ConstructGlobal();
    HudSensorTracker::RegisterGlobalOnExit();
}
} // namespace Mission
