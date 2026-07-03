#include "GameZRecoil/mission.h"

#include "Battlesport/hud_sensor_tracker.h"

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

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *MissionCrtInitializerFn)();
/* VC5 emits this mission-objectives startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
MissionCrtInitializerFn s_MissionCrtInit_Objectives =
    Mission::InitObjectives;
#pragma data_seg()
#endif
