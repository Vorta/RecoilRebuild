#include "Battlesport/hud_sensor_tracker.h"

#include "Battlesport/game_net.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/hud.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zInterp/zinterp.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep.h"

#include <math.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" char g_HudSensorTracker_ZarSectionName_MissionData[0x0c];
extern "C" char g_HudSensorTracker_ObjectivesZrdPath[0x0f];
extern "C" const unsigned char g_HudSensorTracker_ObjectiveBlinkColorRedRgb24[4];
extern "C" const unsigned char g_HudSensorTracker_ObjectiveMarkerColorBlueRgb24[4];
extern "C" char g_HudSensorTracker_ZarHandlerName_MissionLate[0x0c];
extern "C" char g_HudSensorTracker_ZarHandlerName_Mission[0x08];
extern "C" char g_HudSensorTracker_LateMissionDataSectionName[0x10];
extern "C" const char g_HudSensorTracker_StartAnimsZrdPath[0x0e];
extern "C" char g_HudSensorTracker_DisplayNodeName[0x08];
extern "C" char g_HudSensorTracker_WindowNodeName[0x08];
extern "C" char g_HudSensorTracker_CameraNodeName[0x08];
extern "C" char g_HudSensorTracker_WorldNodeName[0x07];
extern "C" char g_HudSensorTracker_MissionSoundSetNameFmt[0x04];
extern "C" char g_HudSensorTracker_MissionGsFmt[0x07];
extern "C" char g_HudSensorTracker_MissionZbdGsFmt[0x0b];
extern "C" char g_HudSensorTracker_InitScriptPathFmt[0x13];
extern "C" const char g_HudSensorTracker_FindMissionObjectivesMsg[0x18];
extern "C" const char g_HudSensorTracker_DefaultAirdropCarrierNodeName[0x06];
extern "C" const char kHudSensorTrackerWeatherArchiveName[];
extern "C" const char kHudSensorTrackerAiArchiveName[];
extern "C" const char kHudSensorTrackerWeaponsArchiveName[];
extern "C" const char kHudSensorTrackerEffectsArchiveName[];
extern "C" const char kHudSensorTrackerPickupArchiveName[];
extern "C" int g_HudSensorTracker_ObjectiveCommandLocked;
extern "C" const char g_HudSensorTracker_MissionUnloadedMsg[0x14];
extern "C" const char g_HudSensorTracker_ClosingModelsMsg[0x0f];
extern "C" const char g_HudSensorTracker_ClosingClassMsg[0x0e];
extern "C" const char g_HudSensorTracker_LargeModelsCheckpointFmt[0x11];
extern "C" const char g_HudSensorTracker_ClosingAnimationsMsg[0x13];
extern "C" const char g_HudSensorTracker_ClosingEffectsMsg[0x10];
extern "C" const char g_HudSensorTracker_ClosingWeaponsMsg[0x10];
extern "C" const char g_HudSensorTracker_ClosingPlayerMsg[0x0f];
extern "C" const char g_HudSensorTracker_UnloadObjectivesMsg[0x12];
extern "C" const char g_HudSensorTracker_UnloadingMissionMsg[0x15];
extern "C" const char g_HudLoading_StopAllSoundsMsg[0x10];
extern "C" char g_HudSensorTracker_ObjectiveImageMissingFmt[0x2b];
extern "C" char g_HudSensorTracker_ObjectivesArrayOverflowFmt[0x36];
extern "C" char g_HudSensorTracker_ObjectiveNode_Autoplay[0x09];
extern "C" char g_HudSensorTracker_ObjectiveNodeNameFmt[0x0c];
extern "C" char g_HudSensorTracker_ObjectiveNode_FinalMission[0x0e];
extern "C" char g_HudSensorTracker_ObjectiveNode_ReviewDelay[0x0d];
extern "C" char g_HudSensorTracker_ObjectiveNode_ReadTime[0x0a];
extern "C" char g_HudSensorTracker_MissionImageSearchPathFmt[0x14];
extern "C" char g_HudSensorTracker_ObjectiveIncomingSfxName[0x0d];
extern "C" char g_HudSensorTracker_ObjectiveNode_ObjectiveSound[0x10];
extern "C" char g_HudSensorTracker_ObjectiveNode_ReadSound[0x0b];
extern "C" char g_HudSensorTracker_ObjectiveInactivationNodeMissingFmt[0x31];
extern "C" char g_HudSensorTracker_ObjectiveNode_Inactive[0x09];
extern "C" char g_HudSensorTracker_ObjectiveActivationNodeMissingFmt[0x2f];
extern "C" char g_HudSensorTracker_ObjectiveNode_Active[0x07];
extern "C" char g_HudSensorTracker_ObjectiveNode_ReviewSound[0x0d];
extern "C" char g_HudSensorTracker_ObjectivePanelThreeLineFmt[0x09];
extern "C" char g_HudUiWeaponStatsFmt_Basic[0x3f];
extern "C" char g_HudUiWeaponStatsFmt_Proximity[0x5b];
extern "C" char g_HudUiWeaponFeatureSuffix_Mine[0x06];
extern "C" char g_HudUiWeaponFeatureSuffix_Beam[0x06];
extern "C" char g_HudUiWeaponFeatureSuffix_LockOn[0x09];
extern "C" char g_HudUiWeaponFeatureSuffix_Tether[0x08];
extern "C" char g_HudUiWeaponFeatureSuffix_Multi[0x07];
extern "C" char g_HudUiWeaponFeatureSuffix_Thermal[0x09];
extern "C" char g_HudUiWeaponFeatureSuffix_Remote[0x08];
extern "C" char g_HudUiWeaponFeaturesLabel[0x0a];
extern "C" char g_HudWeatherFx_AlphaGradientNodeName[0x0f];
extern "C" char g_HudWeatherFx_WindVelocityNodeName[0x09];
extern "C" char g_HudWeatherFx_WindDirectionNodeName[0x09];
extern "C" char g_HudWeatherFx_TypeValue_Rain[0x05];
extern "C" char g_HudWeatherFx_TypeValue_Snow[0x05];
extern "C" char g_HudWeatherFx_TypeNodeName[0x05];
extern "C" char g_HudWeatherFx_ParticlesNodeName[0x0a];
extern "C" char g_HudWeatherFx_MissionNodeNameFmt[0x0a];
extern "C" char g_HudSensorTracker_MissionCppSourcePath[0x20];
extern "C" const char kHudSensorTrackerRaceCheckpointCountNodeName[];
extern "C" const char kHudSensorTrackerRaceCheckpointArchiveName[];
extern "C" const char kHudSensorTrackerRaceZrdrSearchPathFmt[];

extern "C" int g_HudSensorTracker_ObjectiveCommandLocked = 0;
extern "C" int g_Hud_MapOverlayRefCount = 0;
extern "C" int g_RecoilApp_QuitAfterCredits = 0;
extern "C" char g_HudSensor_MissionSoundSetName[0x20] = {0};
extern "C" zVec3 g_HudSensor_ProjectScratch[0x400] = {0};
extern "C" zVec3 g_HudSensor_ClipSegmentStart = {0};
extern "C" zVec3 g_HudSensor_ClipSegmentEnd = {0};
extern "C" float g_HudLineClip_CurrentLeft = 0.0f;
extern "C" float g_HudLineClip_CurrentTop = 0.0f;
extern "C" float g_HudLineClip_CurrentRight = 0.0f;
extern "C" float g_HudLineClip_CurrentBottom = 0.0f;

namespace {
/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-mapcppsourcepath
 * @recoil-artifact defines .data recoil:data:0x4daf04: g_HudSensorTracker_MapCppSourcePath (D:\Proj\Battlesport\map.cpp).
 * Purpose: preserve the map.cpp source path used by map-load error reports.
 */
char g_HudSensorTracker_MapCppSourcePath[0x1c] = "D:\\Proj\\Battlesport\\map.cpp";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MapCppSourcePath) == 0x1c);

/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-incorrectmapfileversionfmt
 * @recoil-artifact defines .data recoil:data:0x4daf20: g_HudSensorTracker_IncorrectMapFileVersionFmt (D:\Proj\Battlesport\map.cpp).
 * Purpose: format the map-file version mismatch diagnostic.
 */
char g_HudSensorTracker_IncorrectMapFileVersionFmt[0x32] = "Incorrect Map File Version (found %d, wanted %d)\n";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_IncorrectMapFileVersionFmt) == 0x32);

/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-mapclicksfxname
 * @recoil-artifact defines .data recoil:data:0x4daf5c: g_HudSensorTracker_MapClickSfxName (D:\Proj\Battlesport\map.cpp).
 * Purpose: name the map-click HUD sound sample.
 */
char g_HudSensorTracker_MapClickSfxName[0x0d] = "snd_mapClick";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MapClickSfxName) == 0x0d);

/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-mapoffsfxname
 * @recoil-artifact defines .data recoil:data:0x4daf6c: g_HudSensorTracker_MapOffSfxName (D:\Proj\Battlesport\map.cpp).
 * Purpose: name the map-off HUD sound sample.
 */
char g_HudSensorTracker_MapOffSfxName[0x0b] = "snd_mapOff";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MapOffSfxName) == 0x0b);

/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-maponsfxname
 * @recoil-artifact defines .data recoil:data:0x4daf78: g_HudSensorTracker_MapOnSfxName (D:\Proj\Battlesport\map.cpp).
 * Purpose: name the map-on HUD sound sample.
 */
char g_HudSensorTracker_MapOnSfxName[0x0a] = "snd_mapOn";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MapOnSfxName) == 0x0a);

/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-missionmappathfmt
 * @recoil-artifact defines .data recoil:data:0x4daf84: g_HudSensorTracker_MissionMapPathFmt (D:\Proj\Battlesport\map.cpp).
 * Purpose: format the mission-specific map path.
 */
char g_HudSensorTracker_MissionMapPathFmt[0x10] = ".\\maps\\m%d.zmap";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MissionMapPathFmt) == 0x10);

/**
 * Purpose: supply the RGB24 red objective-marker blink color with VC5 four-byte storage.
 */
extern "C" const unsigned char g_HudSensorTracker_ObjectiveBlinkColorRedRgb24[4] = {0xff, 0x00, 0x00, 0x00};
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveBlinkColorRedRgb24) == 4);

/**
 * Purpose: supply the RGB24 blue objective-marker color with VC5 four-byte storage.
 */
extern "C" const unsigned char g_HudSensorTracker_ObjectiveMarkerColorBlueRgb24[4] = {0x00, 0x00, 0xff, 0x00};
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveMarkerColorBlueRgb24) == 4);

} // namespace

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-zarsectionname-missiondata
 * @recoil-artifact defines .data recoil:data:0x4daf9c: g_HudSensorTracker_ZarSectionName_MissionData.
 * Purpose: names the fixed HUD mission-state ZAR payload section.
 */
char g_HudSensorTracker_ZarSectionName_MissionData[0x0c] = "MissionData";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ZarSectionName_MissionData) == 0x0c);
/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-objectiveszrdpath
 * @recoil-artifact defines .data recoil:data:0x4dafa8: g_HudSensorTracker_ObjectivesZrdPath.
 * Purpose: names the objective definition archive loaded during mission startup
 * and saved-game mission restore.
 */
char g_HudSensorTracker_ObjectivesZrdPath[0x0f] = "objectives.zrd";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectivesZrdPath) == 0x0f);
/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-zarhandlername-missionlate
 * @recoil-artifact defines .data recoil:data:0x4dafb8: g_HudSensorTracker_ZarHandlerName_MissionLate.
 * Purpose: names the late mission restore ZAR callback section.
 */
char g_HudSensorTracker_ZarHandlerName_MissionLate[0x0c] = "MissionLate";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ZarHandlerName_MissionLate) == 0x0c);
/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-zarhandlername-mission
 * @recoil-artifact defines .data recoil:data:0x4dafc4: g_HudSensorTracker_ZarHandlerName_Mission.
 * Purpose: names the primary HUD mission save/restore ZAR callback section.
 */
char g_HudSensorTracker_ZarHandlerName_Mission[0x08] = "Mission";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ZarHandlerName_Mission) == 0x08);
/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-hudsensortracker-latemissiondatasectionname
 * @recoil-artifact defines .data recoil:data:0x4dafcc: g_HudSensorTracker_LateMissionDataSectionName.
 * Purpose: names the one-word marker payload written for late mission restore.
 */
char g_HudSensorTracker_LateMissionDataSectionName[0x10] = "LateMissionData";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_LateMissionDataSectionName) == 0x10);
}

namespace {

/**
 * Purpose: name the display node activated during core mission resource load.
 */
extern "C" char g_HudSensorTracker_DisplayNodeName[0x08] = "display";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_DisplayNodeName) == 0x08);
/**
 * Purpose: name the render target window node activated during mission load.
 */
extern "C" char g_HudSensorTracker_WindowNodeName[0x08] = "window1";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_WindowNodeName) == 0x08);
/**
 * Purpose: name the active camera node used by mission load and HUD runtime.
 */
extern "C" char g_HudSensorTracker_CameraNodeName[0x08] = "camera1";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_CameraNodeName) == 0x08);
/**
 * Purpose: name the mission world node bound after core scripts load.
 */
extern "C" char g_HudSensorTracker_WorldNodeName[0x07] = "world1";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_WorldNodeName) == 0x07);
/**
 * Purpose: format the mission sound-set name for load and shutdown.
 */
extern "C" char g_HudSensorTracker_MissionSoundSetNameFmt[0x04] = "M%d";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MissionSoundSetNameFmt) == 0x04);
/**
 * Purpose: format the normal mission game-state script name.
 */
extern "C" char g_HudSensorTracker_MissionGsFmt[0x07] = "m%d.gs";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MissionGsFmt) == 0x07);
/**
 * Purpose: format the mission game-state script name used when ZBD flags are set.
 */
extern "C" char g_HudSensorTracker_MissionZbdGsFmt[0x0b] = "m%d_zbd.gs";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MissionZbdGsFmt) == 0x0b);
/**
 * Purpose: format the support init script path for the selected mission.
 */
extern "C" char g_HudSensorTracker_InitScriptPathFmt[0x13] = "support\\initm%d.gw";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_InitScriptPathFmt) == 0x13);

/**
 * Purpose: name the mission weather archive loaded during HUD mission startup.
 */
extern "C" const char kHudSensorTrackerWeatherArchiveName[] = "Weather.zrd";
/**
 * Purpose: report the objective-loading phase during HUD mission startup.
 */
extern "C" const char g_HudSensorTracker_FindMissionObjectivesMsg[0x18] = "Find mission objectives";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_FindMissionObjectivesMsg) == 0x18);
/**
 * Purpose: name the default carrier node used for pickup airdrop spawn setup.
 */
extern "C" const char g_HudSensorTracker_DefaultAirdropCarrierNodeName[0x06] = "vtol2";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_DefaultAirdropCarrierNodeName) == 0x06);
/**
 * Purpose: name the turret/AI definition archive loaded during HUD mission startup.
 */
extern "C" const char kHudSensorTrackerAiArchiveName[] = "ai.zrd";
/**
 * Purpose: name the weapon opt-catalog archive loaded during HUD mission startup.
 */
extern "C" const char kHudSensorTrackerWeaponsArchiveName[] = "weapons.zrd";
/**
 * @recoil-anchor recoil:anchor:battlesport.map.g-zeffectanim-defaultarchivename
 * @recoil-artifact defines .data recoil:data:0x4db088: g_zEffectAnim_DefaultArchiveName.
 * Purpose: keep the standalone default animation archive-name literal separate
 * from HudSensorTracker mission archive names and zEffect runtime filename state.
 */
char g_zEffectAnim_DefaultArchiveName[0x09] = "anim.zrd";
RECOIL_STATIC_ASSERT(sizeof(g_zEffectAnim_DefaultArchiveName) == 0x09);
/**
 * Purpose: name the effects archive loaded during HUD mission startup.
 */
extern "C" const char kHudSensorTrackerEffectsArchiveName[] = "effects.zrd";
/**
 * Purpose: name the pickup archive loaded during HUD mission startup.
 */
extern "C" const char kHudSensorTrackerPickupArchiveName[] = "pickup.zrd";
/**
 * Purpose: report completion of mission gameplay shutdown.
 */
extern "C" const char g_HudSensorTracker_MissionUnloadedMsg[0x14] = "...Mission Unloaded";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MissionUnloadedMsg) == 0x14);
/**
 * Purpose: report the model-system shutdown checkpoint.
 */
extern "C" const char g_HudSensorTracker_ClosingModelsMsg[0x0f] = "Closing Models";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ClosingModelsMsg) == 0x0f);
/**
 * Purpose: report the class-system shutdown checkpoint.
 */
extern "C" const char g_HudSensorTracker_ClosingClassMsg[0x0e] = "Closing Class";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ClosingClassMsg) == 0x0e);
/**
 * Purpose: format the world large-model count checkpoint during shutdown.
 */
extern "C" const char g_HudSensorTracker_LargeModelsCheckpointFmt[0x11] = "Large Models: %d";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_LargeModelsCheckpointFmt) == 0x11);
/**
 * Purpose: report the animation-system shutdown checkpoint.
 */
extern "C" const char g_HudSensorTracker_ClosingAnimationsMsg[0x13] = "Closing Animations";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ClosingAnimationsMsg) == 0x13);
/**
 * Purpose: report the effects-system shutdown checkpoint.
 */
extern "C" const char g_HudSensorTracker_ClosingEffectsMsg[0x10] = "Closing Effects";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ClosingEffectsMsg) == 0x10);
/**
 * Purpose: report the weapon-catalog shutdown checkpoint.
 */
extern "C" const char g_HudSensorTracker_ClosingWeaponsMsg[0x10] = "Closing Weapons";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ClosingWeaponsMsg) == 0x10);
/**
 * Purpose: report the player/gameplay runtime shutdown checkpoint.
 */
extern "C" const char g_HudSensorTracker_ClosingPlayerMsg[0x0f] = "Closing Player";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ClosingPlayerMsg) == 0x0f);
/**
 * Purpose: report the objective-runtime unload checkpoint.
 */
extern "C" const char g_HudSensorTracker_UnloadObjectivesMsg[0x12] = "Unload Objectives";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_UnloadObjectivesMsg) == 0x12);
} // namespace

/**
 * Purpose: report the shared sound shutdown checkpoint used by mission shutdown
 * and play-state deactivation without duplicating literal storage.
 */
extern "C" const char g_HudLoading_StopAllSoundsMsg[0x10] = "Stop All Sounds";
RECOIL_STATIC_ASSERT(sizeof(g_HudLoading_StopAllSoundsMsg) == 0x10);

namespace {
/**
 * Purpose: report the beginning of mission gameplay shutdown.
 */
extern "C" const char g_HudSensorTracker_UnloadingMissionMsg[0x15] = "Unloading Mission...";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_UnloadingMissionMsg) == 0x15);
/**
 * Purpose: format the missing objective image diagnostic during objective ZRD load.
 */
extern "C" char g_HudSensorTracker_ObjectiveImageMissingFmt[0x2b] =
    "Cannot find objective %d's image file - %s";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveImageMissingFmt) == 0x2b);
/**
 * Purpose: format the mission objective array-capacity diagnostic.
 */
extern "C" char g_HudSensorTracker_ObjectivesArrayOverflowFmt[0x36] =
    "Mission objectives array overflow; MAX allowable = %d";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectivesArrayOverflowFmt) == 0x36);
/**
 * Purpose: name the objective autoplay flag node in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNode_Autoplay[0x09] = "AUTOPLAY";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNode_Autoplay) == 0x09);
/**
 * Purpose: format numbered objective node names in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNodeNameFmt[0x0c] = "OBJECTIVE%d";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNodeNameFmt) == 0x0c);
/**
 * Purpose: name the final-mission objective flag node in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNode_FinalMission[0x0e] = "FINAL_MISSION";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNode_FinalMission) == 0x0e);
/**
 * Purpose: name the objective review-delay node in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNode_ReviewDelay[0x0d] = "REVIEW_DELAY";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNode_ReviewDelay) == 0x0d);
/**
 * Purpose: name the objective read-time node in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNode_ReadTime[0x0a] = "READ_TIME";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNode_ReadTime) == 0x0a);
/**
 * Purpose: format the mission-specific image search path for objective artwork.
 */
extern "C" char g_HudSensorTracker_MissionImageSearchPathFmt[0x14] = "..\\data\\m%d\\images\\";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MissionImageSearchPathFmt) == 0x14);
/**
 * Purpose: preserve the mission.cpp source path used by HUD mission-load error
 * reports.
 */
extern "C" char g_HudSensorTracker_MissionCppSourcePath[0x20] = "D:\\Proj\\Battlesport\\mission.cpp";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_MissionCppSourcePath) == 0x20);
/**
 * Purpose: name the default incoming-objective HUD sound sample.
 */
extern "C" char g_HudSensorTracker_ObjectiveIncomingSfxName[0x0d] = "snd_incoming";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveIncomingSfxName) == 0x0d);
/**
 * Purpose: name the objective-complete sound node in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNode_ObjectiveSound[0x10] = "OBJECTIVE_SOUND";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNode_ObjectiveSound) == 0x10);
/**
 * Purpose: name the objective read-sound node in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNode_ReadSound[0x0b] = "READ_SOUND";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNode_ReadSound) == 0x0b);
/**
 * Purpose: format the missing objective inactivation-node diagnostic.
 */
extern "C" char g_HudSensorTracker_ObjectiveInactivationNodeMissingFmt[0x31] =
    "Cannot find Objective %d's inactivation node: %s";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveInactivationNodeMissingFmt) == 0x31);
/**
 * Purpose: name the objective inactivation path node in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNode_Inactive[0x09] = "INACTIVE";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNode_Inactive) == 0x09);
/**
 * Purpose: format the missing objective activation-node diagnostic.
 */
extern "C" char g_HudSensorTracker_ObjectiveActivationNodeMissingFmt[0x2f] =
    "Cannot find Objective %d's activation node: %s";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveActivationNodeMissingFmt) == 0x2f);
/**
 * Purpose: name the objective activation path node in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNode_Active[0x07] = "ACTIVE";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNode_Active) == 0x07);
/**
 * Purpose: name the objective review-sound node in objectives.zrd.
 */
extern "C" char g_HudSensorTracker_ObjectiveNode_ReviewSound[0x0d] = "REVIEW_SOUND";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectiveNode_ReviewSound) == 0x0d);
/**
 * Purpose: format the objective summary panel as three localized text lines.
 */
extern "C" char g_HudSensorTracker_ObjectivePanelThreeLineFmt[0x09] = "%s\n%s\n%s";
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_ObjectivePanelThreeLineFmt) == 0x09);
/**
 * Purpose: format active pickup weapon stats when no proximity value is shown.
 */
extern "C" char g_HudUiWeaponStatsFmt_Basic[0x3f] =
    "Fire Rate: %d rds/min   Max. Range: %d m\nDamage Power: %.1f\n%s";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponStatsFmt_Basic) == 0x3f);
/**
 * Purpose: format active pickup weapon stats when proximity damage is shown.
 */
extern "C" char g_HudUiWeaponStatsFmt_Proximity[0x5b] =
    "Fire Rate: %d rds/min   Max. Range: %d m\n"
    "Damage Power: %.1f      Damage Proximity: %d m\n%s";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponStatsFmt_Proximity) == 0x5b);
/**
 * Purpose: append the mine feature label to active pickup feature text.
 */
extern "C" char g_HudUiWeaponFeatureSuffix_Mine[0x06] = " Mine";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponFeatureSuffix_Mine) == 0x06);
/**
 * Purpose: append the beam feature label to active pickup feature text.
 */
extern "C" char g_HudUiWeaponFeatureSuffix_Beam[0x06] = " Beam";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponFeatureSuffix_Beam) == 0x06);
/**
 * Purpose: append the lock-on feature label to active pickup feature text.
 */
extern "C" char g_HudUiWeaponFeatureSuffix_LockOn[0x09] = " Lock On";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponFeatureSuffix_LockOn) == 0x09);
/**
 * Purpose: append the tether feature label to active pickup feature text.
 */
extern "C" char g_HudUiWeaponFeatureSuffix_Tether[0x08] = " Tether";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponFeatureSuffix_Tether) == 0x08);
/**
 * Purpose: append the multi-shot feature label to active pickup feature text.
 */
extern "C" char g_HudUiWeaponFeatureSuffix_Multi[0x07] = " Multi";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponFeatureSuffix_Multi) == 0x07);
/**
 * Purpose: append the thermal feature label to active pickup feature text.
 */
extern "C" char g_HudUiWeaponFeatureSuffix_Thermal[0x09] = " Thermal";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponFeatureSuffix_Thermal) == 0x09);
/**
 * Purpose: append the remote feature label to active pickup feature text.
 */
extern "C" char g_HudUiWeaponFeatureSuffix_Remote[0x08] = " Remote";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponFeatureSuffix_Remote) == 0x08);
/**
 * Purpose: seed active pickup feature text before suffixes are appended.
 */
extern "C" char g_HudUiWeaponFeaturesLabel[0x0a] = "Features:";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiWeaponFeaturesLabel) == 0x0a);
/**
 * Purpose: name the optional alpha-gradient tuning node in Weather.zrd.
 */
extern "C" char g_HudWeatherFx_AlphaGradientNodeName[0x0f] = "ALPHA_GRADIENT";
RECOIL_STATIC_ASSERT(sizeof(g_HudWeatherFx_AlphaGradientNodeName) == 0x0f);
/**
 * Purpose: name the optional wind-velocity tuning node in Weather.zrd.
 */
extern "C" char g_HudWeatherFx_WindVelocityNodeName[0x09] = "WIND_VEL";
RECOIL_STATIC_ASSERT(sizeof(g_HudWeatherFx_WindVelocityNodeName) == 0x09);
/**
 * Purpose: name the optional wind-direction tuning node in Weather.zrd.
 */
extern "C" char g_HudWeatherFx_WindDirectionNodeName[0x09] = "WIND_DIR";
RECOIL_STATIC_ASSERT(sizeof(g_HudWeatherFx_WindDirectionNodeName) == 0x09);
/**
 * Purpose: select the rain weather FX class from Weather.zrd TYPE values.
 */
extern "C" char g_HudWeatherFx_TypeValue_Rain[0x05] = "RAIN";
RECOIL_STATIC_ASSERT(sizeof(g_HudWeatherFx_TypeValue_Rain) == 0x05);
/**
 * Purpose: select the snow weather FX class from Weather.zrd TYPE values.
 */
extern "C" char g_HudWeatherFx_TypeValue_Snow[0x05] = "SNOW";
RECOIL_STATIC_ASSERT(sizeof(g_HudWeatherFx_TypeValue_Snow) == 0x05);
/**
 * Purpose: name the Weather.zrd node that selects rain or snow FX.
 */
extern "C" char g_HudWeatherFx_TypeNodeName[0x05] = "TYPE";
RECOIL_STATIC_ASSERT(sizeof(g_HudWeatherFx_TypeNodeName) == 0x05);
/**
 * Purpose: name the optional particle-count tuning node in Weather.zrd.
 */
extern "C" char g_HudWeatherFx_ParticlesNodeName[0x0a] = "PARTICLES";
RECOIL_STATIC_ASSERT(sizeof(g_HudWeatherFx_ParticlesNodeName) == 0x0a);
/**
 * Purpose: format the mission-specific Weather.zrd node name.
 */
extern "C" char g_HudWeatherFx_MissionNodeNameFmt[0x0a] = "MISSION%d";
RECOIL_STATIC_ASSERT(sizeof(g_HudWeatherFx_MissionNodeNameFmt) == 0x0a);
/**
 * Purpose: name the race checkpoint count node queried from race.zrd.
 */
extern "C" const char kHudSensorTrackerRaceCheckpointCountNodeName[] = "cp_count";
/**
 * Purpose: name the race checkpoint metadata archive loaded during mission startup.
 */
extern "C" const char kHudSensorTrackerRaceCheckpointArchiveName[] = "race.zrd";
/**
 * Purpose: format the mission-specific ZRDR search path used to load race.zrd.
 */
extern "C" const char kHudSensorTrackerRaceZrdrSearchPathFmt[] = "..\\data\\m%d\\zrdr";

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: caller recoil:function:0x416f10 inlines this approximation at [0x416f87,0x416f97).
 * Purpose: preserve the recovered HUD behavior for ApproxSqrtScaleFromBits.
 */
static inline float ApproxSqrtScaleFromBits(
    float value
) {
    int bits;
    memcpy(
        &bits,
        &value,
        sizeof(bits)
    );
    bits = (bits >> 1) + 0x1fc00000;

    float approxValue;
    memcpy(
        &approxValue,
        &bits,
        sizeof(approxValue)
    );
    return approxValue;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: caller recoil:function:0x416f10 inlines this rectangle test.
 * Purpose: preserve the recovered HUD behavior for IsPointStrictlyInsideRect.
 */
static inline bool IsPointStrictlyInsideRect(
    const HudUiRect &rect,
    const zVec3 &point
) {
    return (float)(rect.left) < point.x && (float)(rect.right) > point.x &&
           (float)(rect.top) < point.y && (float)(rect.bottom) > point.y;
}

} // namespace

/**
 *
 * Purpose: names the start-animation ZRDR loaded during play-state startup
 * and late mission restore.
 */
extern "C" const char g_HudSensorTracker_StartAnimsZrdPath[0x0e] = {
    'S', 't', 'a', 'r', 't', 'A', 'n', 'i', 'm', 's', '.', 'z', 'r', 'd'
};
RECOIL_STATIC_ASSERT(sizeof(g_HudSensorTracker_StartAnimsZrdPath) == 0x0e);
/**
 *
 * Purpose: selects the saved-game start-animation node during play-state
 * startup and late mission restore.
 */
extern "C" const char g_RecoilApp_LoadGameStartAnimStateName[0x10] = "LOAD_GAME_START";
RECOIL_STATIC_ASSERT(sizeof(g_RecoilApp_LoadGameStartAnimStateName) == 0x10);

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-init
 * @recoil-artifact defines .text recoil:function:0x415ab0: HudSensorMapNode::Init
 * Purpose: Apply map-node defaults and return this node.
 */
HudSensorMapNode * HudSensorMapNode::Init() {
    InitDefaults();
    return this;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-freepointarray
 * @recoil-artifact defines .text recoil:function:0x415ac0: HudSensorMapNode::FreePointArray
 * Purpose: Release the dynamically loaded map point array when present.
 */
void HudSensorMapNode::FreePointArray() {
    if (points != 0) {
        free(points);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-setenabled
 * @recoil-artifact defines .text recoil:function:0x415ae0: HudSensorMapNode::SetEnabled
 * Purpose: Toggle marker visibility, refreshing color state and clearing point selection.
 */
int HudSensorMapNode::SetEnabled(
    int enabled
) {
    if (isEnabled == enabled) {
        return 0;
    }

    isEnabled = enabled;
    SetColorRgb(0);
    SelectPoint(-1);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-selectpoint
 * @recoil-artifact defines .text recoil:function:0x415b10: HudSensorMapNode::SelectPoint
 * Purpose: Select an in-range path point or clear the active selection.
 */
HudSensorMapPoint * HudSensorMapNode::SelectPoint(
    int pointIndex
) {
    if (pointIndex >= 0 && pointIndex < pointCount) {
        selectedPointIndex = pointIndex;
        return &points[pointIndex];
    }

    selectedPointIndex = -1;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-initdefaults
 * @recoil-artifact defines .text recoil:function:0x415b40: HudSensorMapNode::InitDefaults
 * Purpose: Initialize map-node links, point storage, marker state, and default color fields.
 */
int HudSensorMapNode::InitDefaults() {
    colorRgb[0] = (char)(0xff);
    colorRgb[1] = (char)(0xff);
    colorRgb[2] = (char)(0xff);
    pointCount = 0;
    points = 0;
    isEnabled = 0;
    blinkTimerSec = 0.0f;
    next = 0;
    objectiveIndex = -1;
    selectedPointIndex = -1;
    packedColor565Pair = -1;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-setcolorrgb
 * @recoil-artifact defines .text recoil:function:0x415b70: HudSensorMapNode::SetColorRgb
 * Purpose: Optionally copy RGB bytes and rebuild the full/half-intensity packed color pair.
 */
int HudSensorMapNode::SetColorRgb(
    const unsigned char *rgbOrNull
) {
    if (rgbOrNull != 0) {
        colorRgb[0] = (char)(rgbOrNull[0]);
        colorRgb[1] = (char)(rgbOrNull[1]);
        colorRgb[2] = (char)(rgbOrNull[2]);
    }

    const unsigned char red = (unsigned char)(colorRgb[0]);
    const unsigned char green = (unsigned char)(colorRgb[2]);
    const unsigned char blue = (unsigned char)(colorRgb[1]);
    const unsigned short fullColor = (unsigned short)(zVid_PackColorRGB(
        red,
        green,
        blue
    ));
    const unsigned short halfColor = (unsigned short)(zVid_PackColorRGB(
        (unsigned char)(red >> 1),
        (unsigned char)(green >> 1),
        (unsigned char)(blue >> 1)
    ));
    packedColor565Pair = ((int)(halfColor) << 16) | fullColor;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-loadfromstream
 * @recoil-artifact defines .text recoil:function:0x415bd0: HudSensorMapNode::LoadFromStream
 * Purpose: Load color, points, and objective binding from a sensor-map stream.
 */
int HudSensorMapNode::LoadFromStream(
    FILE *stream
) {
    if (stream == 0) {
        return 0;
    }

    if (fread(
        colorRgb,
        3,
        1,
        stream
    ) != 1) {
        return 0;
    }

    if (fread(
        &pointCount,
        4,
        1,
        stream
    ) != 1) {
        return 0;
    }

    const size_t byteCount = (size_t)(pointCount) * sizeof(HudSensorMapPoint);
    points = (HudSensorMapPoint *)(malloc(byteCount));

    if (fread(points, sizeof(HudSensorMapPoint), (size_t)(pointCount), stream) !=
        (size_t)(pointCount)) {
        return 0;
    }

    if (fread(
        &objectiveIndex,
        4,
        1,
        stream
    ) != 1) {
        return 0;
    }

    SetColorRgb(0);
    UpdateCachedBounds(0);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-updatecachedbounds
 * @recoil-artifact defines .text recoil:function:0x415c90: HudSensorMapNode::UpdateCachedBounds
 * Purpose: Copy cached bounds or recompute X/Z extents from the loaded point array.
 */
int HudSensorMapNode::UpdateCachedBounds(
    HudSensorMapBounds *outBoundsOrNull
) {
    if (outBoundsOrNull != 0) {
        *outBoundsOrNull = cachedBounds;
        return 1;
    }

    HudSensorMapPoint *point = points;
    cachedBounds.minX = point->x;
    cachedBounds.maxX = point->x;
    cachedBounds.minY = 0.0f;
    cachedBounds.minZ = point->z;
    cachedBounds.maxZ = point->z;

    ++point;
    {
        for (int remaining = pointCount - 1; remaining != 0; --remaining, ++point) {
            if (point->x < cachedBounds.minX) {
                cachedBounds.minX = point->x;
            }

            if (point->x > cachedBounds.maxX) {
                cachedBounds.maxX = point->x;
            }

            if (point->z < cachedBounds.minZ) {
                cachedBounds.minZ = point->z;
            }

            if (point->z > cachedBounds.maxZ) {
                cachedBounds.maxZ = point->z;
            }
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-drawontracker
 * @recoil-artifact defines .text recoil:function:0x415d30: HudSensorMapNode::DrawOnTracker
 * Purpose: Draw this map node on the tracker, including blink state and selected-point marker.
 */
int HudSensorMapNode::DrawOnTracker(
    HudSensorTracker *tracker,
    const zVec3 *drawPathWorldPos
) {
    if (isEnabled != 0) {
        blinkTimerSec -= 0.075000003f;
        if (blinkTimerSec <= 0.0f) {
            const unsigned int colorPair = (unsigned int)(packedColor565Pair);
            blinkTimerSec = 0.25f;
            packedColor565Pair = (int)((colorPair << 16) | (colorPair >> 16));
        }
    }

    zVec3 projectedPathPointBuffer[0x401];
    tracker->ProjectWorldPointsToOverlay(
        (const zVec3 *)(points),
        projectedPathPointBuffer,
        pointCount
    );
    projectedPathPointBuffer[pointCount] = projectedPathPointBuffer[0];

    for (int i = 0; i < pointCount; ++i) {
        zVec3 segmentStart = projectedPathPointBuffer[i];
        zVec3 segmentEnd = projectedPathPointBuffer[i + 1];
        int point0Clipped;
        int point1Clipped;

        HudLineClip::SetCurrentBoundsFromRectI((const HudRectI *)(&tracker->outerRect));
        if (HudLineClip::ClipSegmentToCurrentBounds(
                &segmentStart,
                &segmentEnd,
                &point0Clipped,
                &point1Clipped
            ) == 0) {
            continue;
        }

        const int splitResult = ((HudRectI *)(&tracker->innerRectExpanded))
                                    ->ClipOrSplitSegment(
                                        &segmentStart,
                                        &segmentEnd
                                    );
        if (splitResult == 0) {
            continue;
        }

        const int color16 = packedColor565Pair & 0xffff;
        zRndr_DrawImmediateLine(
            (int)(segmentStart.x),
            (int)(segmentStart.y),
            (int)(segmentEnd.x),
            (int)(segmentEnd.y),
            color16
        );

        if (splitResult == 2) {
            zRndr_DrawImmediateLine(
                (int)(g_HudSensor_ClipSegmentStart.x),
                (int)(g_HudSensor_ClipSegmentStart.y),
                (int)(g_HudSensor_ClipSegmentEnd.x),
                (int)(g_HudSensor_ClipSegmentEnd.y),
                color16
            );
        }
    }

    if (selectedPointIndex != -1) {
        zVec3 selectedPoint;
        tracker->ProjectWorldPointsToOverlay(
            (const zVec3 *)(&points[selectedPointIndex]),
            &selectedPoint,
            1
        );
        HudSensorTracker::DrawDiamondMarker(
            (int)(selectedPoint.x),
            (int)(selectedPoint.y),
            4,
            4,
            (unsigned int)(packedColor565Pair) >> 16,
            tracker
        );
    }

    if (drawPathWorldPos != 0) {
        DrawProjectedPath(tracker);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-drawdiamondmarker
 * @recoil-artifact defines .text recoil:function:0x415f40: HudSensorTracker::DrawDiamondMarker
 * Purpose: Draw a centered diamond marker as a closed clipped immediate line strip.
 */
void __fastcall HudSensorTracker::DrawDiamondMarker(
    int centerX,
    int centerY,
    int halfWidth,
    int halfHeight,
    int markerColor,
    HudSensorTracker *tracker
) {
    zRndr_LinePoint2I points[5];

    points[0].x = centerX - halfWidth;
    points[0].y = centerY;
    points[1].x = centerX;
    points[1].y = centerY + halfHeight;
    points[2].x = centerX + halfWidth;
    points[2].y = centerY;
    points[3].x = centerX;
    points[3].y = centerY - halfHeight;
    points[4].x = points[0].x;
    points[4].y = centerY;

    zRndr_DrawClippedImmediateLineStrip(
        points,
        4,
        tracker,
        markerColor & 0xffff
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudrecti-cliporsplitsegment
 * @recoil-artifact defines .text recoil:function:0x415fb0: HudRectI::ClipOrSplitSegment
 * Purpose: Clip or split a segment against this rectangle and preserve split output globals.
 */
int HudRectI::ClipOrSplitSegment(
    zVec3 *segmentStart,
    zVec3 *segmentEnd
) {
    if (left == right) {
        return 1;
    }

    int startOutcode = CalcOutcode(segmentStart);
    int endOutcode = CalcOutcode(segmentEnd);
    if (startOutcode == 0 && endOutcode == 0) {
        return 0;
    }
    if ((startOutcode & endOutcode) != 0) {
        return 1;
    }

    if ((startOutcode == 0) != (endOutcode == 0)) {
        if (startOutcode != 0) {
            zVec3 *const oldStart = segmentStart;
            segmentStart = segmentEnd;
            segmentEnd = oldStart;
            endOutcode = startOutcode;
            startOutcode = 0;
        }

        if (SegmentIntersectsEdge(
            8,
            segmentStart,
            segmentEnd
        ) != 0) {
            HudLineClip::ClipEndpointToY(
                segmentEnd,
                segmentStart,
                (float)(top)
            );
            return 1;
        }
        else if (SegmentIntersectsEdge(
            4,
            segmentStart,
            segmentEnd
        ) != 0) {
            HudLineClip::ClipEndpointToY(
                segmentStart,
                segmentEnd,
                (float)(bottom)
            );
            return 1;
        }
        else if (SegmentIntersectsEdge(
            1,
            segmentStart,
            segmentEnd
        ) != 0) {
            HudLineClip::ClipEndpointToX(
                segmentStart,
                segmentEnd,
                (float)(left)
            );
            return 1;
        }
        else if (SegmentIntersectsEdge(
            2,
            segmentStart,
            segmentEnd
        ) != 0) {
            HudLineClip::ClipEndpointToX(
                segmentStart,
                segmentEnd,
                (float)(right)
            );
            return 1;
        }
        return 0;
    }

    g_HudSensor_ClipSegmentStart = *segmentStart;
    g_HudSensor_ClipSegmentEnd = *segmentEnd;
    if ((SegmentIntersectsEdge(8, segmentStart, segmentEnd) |
            SegmentIntersectsEdge(
                4,
                segmentStart,
                segmentEnd
            ) |
            SegmentIntersectsEdge(
                1,
                segmentStart,
                segmentEnd
            ) |
            SegmentIntersectsEdge(
                2,
                segmentStart,
                segmentEnd
            )) == 0) {
        return 1;
    }

    if (IsCornerOutcode(startOutcode) != 0) {
        zVec3 *const oldStart = segmentStart;
        segmentStart = segmentEnd;
        segmentEnd = oldStart;
        const int oldStartOutcode = startOutcode;
        startOutcode = endOutcode;
        endOutcode = oldStartOutcode;
    }

    if ((startOutcode & 1) != 0) {
        HudLineClip::ClipEndpointToX(
            segmentStart,
            segmentEnd,
            (float)(left)
        );
    } else if ((startOutcode & 2) != 0) {
        HudLineClip::ClipEndpointToX(
            segmentStart,
            segmentEnd,
            (float)(right)
        );
    }

    if ((startOutcode & 8) != 0) {
        HudLineClip::ClipEndpointToY(
            segmentStart,
            segmentEnd,
            (float)(top)
        );
    } else if ((startOutcode & 4) != 0) {
        HudLineClip::ClipEndpointToY(
            segmentStart,
            segmentEnd,
            (float)(bottom)
        );
    }

    if ((endOutcode & 1) != 0) {
        HudLineClip::ClipEndpointToX(
            &g_HudSensor_ClipSegmentStart,
            &g_HudSensor_ClipSegmentEnd,
            (float)(left)
        );
    } else if ((endOutcode & 2) != 0) {
        HudLineClip::ClipEndpointToX(
            &g_HudSensor_ClipSegmentStart,
            &g_HudSensor_ClipSegmentEnd,
            (float)(right)
        );
    }

    if ((endOutcode & 8) != 0) {
        HudLineClip::ClipEndpointToY(
            &g_HudSensor_ClipSegmentStart,
            &g_HudSensor_ClipSegmentEnd,
            (float)(top)
        );
    } else if ((endOutcode & 4) != 0) {
        HudLineClip::ClipEndpointToY(
            &g_HudSensor_ClipSegmentStart,
            &g_HudSensor_ClipSegmentEnd,
            (float)(bottom)
        );
    }

    return 2;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudrecti-calcoutcode
 * @recoil-artifact defines .text recoil:function:0x416240: HudRectI::CalcOutcode
 * Purpose: Build the rectangle outside-code bits for a point.
 */
int HudRectI::CalcOutcode(
    const zVec3 *point
) {
    int outcode = 0;
    if (point->x < (float)(left)) {
        outcode = 1;
    } else if (point->x > (float)(right)) {
        outcode = 2;
    }

    if (point->y < (float)(top)) {
        outcode |= 8;
    } else if (point->x > (float)(bottom)) {
        outcode |= 4;
    }

    return outcode;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudrecti-iscorneroutcode
 * @recoil-artifact defines .text recoil:function:0x416290: HudRectI::IsCornerOutcode
 * Purpose: Identify outside-code combinations that lie beyond a rectangle corner.
 */
int __fastcall HudRectI::IsCornerOutcode(
    int outcode
) {
    return outcode == 9 || outcode == 10 || outcode == 5 || outcode == 6 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudrecti-segmentintersectsedge
 * @recoil-artifact defines .text recoil:function:0x4162b0: HudRectI::SegmentIntersectsEdge
 * Purpose: Test whether a segment crosses the requested rectangle edge.
 */
int HudRectI::SegmentIntersectsEdge(
    int edgeCode,
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd
) {
    zVec3 edgeStart = {0};
    zVec3 edgeEnd = {0};

    switch (edgeCode) {
    case 1:
        edgeStart.x = (float)(left);
        edgeStart.y = (float)(top);
        edgeEnd.x = (float)(left);
        edgeEnd.y = (float)(bottom);
        break;
    case 2:
        edgeStart.x = (float)(right);
        edgeStart.y = (float)(top);
        edgeEnd.x = (float)(right);
        edgeEnd.y = (float)(bottom);
        break;
    case 4:
        edgeStart.x = (float)(left);
        edgeStart.y = (float)(bottom);
        edgeEnd.x = (float)(right);
        edgeEnd.y = (float)(bottom);
        break;
    case 8:
        edgeStart.x = (float)(left);
        edgeStart.y = (float)(top);
        edgeEnd.x = (float)(right);
        edgeEnd.y = (float)(top);
        break;
    default:
        return 0;
    }

    const int edgeStartSide =
        HudGeom2D::ClassifyPointAgainstSegment(
            &edgeStart,
            &edgeEnd,
            segmentStart
        );
    const int edgeEndSide =
        HudGeom2D::ClassifyPointAgainstSegment(
            &edgeStart,
            &edgeEnd,
            segmentEnd
        );
    const int segEdgeStartSide =
        HudGeom2D::ClassifyPointAgainstSegment(
            segmentStart,
            segmentEnd,
            &edgeStart
        );
    const int segEdgeEndSide =
        HudGeom2D::ClassifyPointAgainstSegment(
            segmentStart,
            segmentEnd,
            &edgeEnd
        );

    if (edgeStartSide * edgeEndSide <= 0 && segEdgeStartSide * segEdgeEndSide <= 0) {
        return edgeCode;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudgeom2d-classifypointagainstsegment
 * @recoil-artifact defines .text recoil:function:0x416390: HudGeom2D::ClassifyPointAgainstSegment
 * Purpose: Classify a point against a 2D segment using the segment cross product and extents.
 */
int __fastcall HudGeom2D::ClassifyPointAgainstSegment(
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd,
    const zVec3 *point
) {
    const float dx = segmentEnd->x - segmentStart->x;
    const float dy = segmentEnd->y - segmentStart->y;
    const float px = point->x - segmentStart->x;
    const float py = point->y - segmentStart->y;
    const float cross = dx * py - dy * px;

    if (cross > 0.0f) {
        return 1;
    }
    if (cross < 0.0f) {
        return -1;
    }
    if (px * dx < 0.0f) {
        return -1;
    }
    if (py * dy < 0.0f) {
        return -1;
    }
    if (px * px + py * py > dx * dx + dy * dy) {
        return 1;
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensormapnode-drawprojectedpath
 * @recoil-artifact defines .text recoil:function:0x416480: HudSensorMapNode::DrawProjectedPath
 * Purpose: Draw the camera-projected sensor-map path with clipped immediate line strips.
 */
int HudSensorMapNode::DrawProjectedPath(
    HudSensorTracker *tracker
) {
    if (pointCount == 0) {
        return 1;
    }

    zMat4x3 cameraScratchMatrix;
    zMath::MatStackPushPtr((float *)(&cameraScratchMatrix));
    zMath::MatLoadCameraScratchB();

    if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
        memcpy(
            g_HudSensor_ProjectScratch,
            points,
            (size_t)(pointCount) * sizeof(zVec3)
        );
    } else {
        const zMat4x3 *const matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
        for (int i = 0; i < pointCount; ++i) {
            const HudSensorMapPoint *const sourcePoint = &points[i];
            zVec3 *const projectedPoint = &g_HudSensor_ProjectScratch[i];

            projectedPoint->x = sourcePoint->x * matrix->xx + sourcePoint->y * matrix->yx +
                                sourcePoint->z * matrix->zx + matrix->posX;
            projectedPoint->z = sourcePoint->x * matrix->xz + sourcePoint->y * matrix->yz +
                                sourcePoint->z * matrix->zz + matrix->posZ;
            projectedPoint->y = sourcePoint->x * matrix->xy + sourcePoint->y * matrix->yy +
                                sourcePoint->z * matrix->zy + matrix->posY;
        }
    }

    zMath::MatStackPopPtr();

    g_HudSensor_ProjectScratch[pointCount] = g_HudSensor_ProjectScratch[0];

    for (int i = 0; i < pointCount; ++i) {
        zVec3 segmentPoints[2];
        segmentPoints[0] = g_HudSensor_ProjectScratch[i];
        segmentPoints[1] = g_HudSensor_ProjectScratch[i + 1];

        if (zMath::ClipLineSegmentToZRange(
            &segmentPoints[0],
            &segmentPoints[1]
        ) == 0) {
            continue;
        }

        zMath::ProjectPointBatch(
            segmentPoints,
            (zProjectedPoint *)(segmentPoints),
            2
        );

        zRndr_LinePoint2I linePoints[2];
        linePoints[0].x = (int)(segmentPoints[0].x) << 1;
        linePoints[0].y = (int)(segmentPoints[0].y) << 1;
        linePoints[1].x = (int)(segmentPoints[1].x) << 1;
        linePoints[1].y = (int)(segmentPoints[1].y) << 1;

        zRndr_DrawClippedImmediateLineStrip(
            linePoints,
            1,
            tracker,
            (unsigned int)(packedColor565Pair) >> 16
        );
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-initnobounds
 * @recoil-artifact defines .text recoil:function:0x416650: HudSensorTracker::InitNoBounds
 * Purpose: Initialize tracker state without replacing the existing map bounds.
 */
HudSensorTracker * HudSensorTracker::InitNoBounds() {
    Init(0);
    return this;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-init
 * @recoil-artifact defines .text recoil:function:0x416660: HudSensorTracker::Init
 * Purpose: Initialize map bounds, save-state marker state, and map runtime defaults.
 */
void HudSensorTracker::Init(
    const HudUiRect *outerRectOrNull
) {
    mapFileVersion = 5;
    mapHeaderDword = 0;
    mapBoundsMaxZ = 0.0f;
    unknown_38 = 0;
    mapBoundsMaxX = 0.0f;
    mapBoundsMinZ = 0.0f;
    unknown_2c = 0;
    mapBoundsMinX = 0.0f;
    mapNodeListHead = 0;
    loadedMapPath = 0;
    mapScaleLerpActive = 0;
    mapLoadedFlag = 0;
    mapScaleCurrent.x = 0.0f;
    mapScaleCurrent.z = 0.0f;
    mapZoom = 0.7f;
    SetBounds(
        outerRectOrNull,
        0
    );
    SetTrackedSaveState(0);
    mapWorldNode = 0;
    mapSndOff = 0;
    mapSndOn = 0;
    SetSaveStateMarkerMaxDistance(450.0f);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-setbounds
 * @recoil-artifact defines .text recoil:function:0x4166e0: HudSensorTracker::SetBounds
 * Purpose: Copy HUD map bounds and cache the overlay center from the outer rect.
 */
void HudSensorTracker::SetBounds(
    const HudUiRect *outerRectIn,
    const HudUiRect *innerRectOrNull
) {
    if (outerRectIn == 0) {
        return;
    }

    outerRect = *outerRectIn;
    if (innerRectOrNull != 0) {
        innerRectExpanded = *innerRectOrNull;
        --innerRectExpanded.top;
        ++innerRectExpanded.right;
        --innerRectExpanded.left;
        ++innerRectExpanded.bottom;
    } else {
        innerRectExpanded.left = 0;
        innerRectExpanded.top = 0;
        innerRectExpanded.right = 0;
        innerRectExpanded.bottom = 0;
    }

    mapOverlayCenterX = outerRect.left + ((outerRect.right - outerRect.left) / 2);
    mapOverlayCenterY = outerRect.top + ((outerRect.bottom - outerRect.top) / 2);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-mapshutdownandresetthunk
 * @recoil-artifact defines .text recoil:function:0x416790: HudSensorTracker::MapShutdownAndResetThunk
 * Purpose: Tail-call the shared map shutdown and reset routine.
 */
int HudSensorTracker::MapShutdownAndResetThunk() {
    return MapShutdownAndReset();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-mapshutdownandreset
 * @recoil-artifact defines .text recoil:function:0x4167a0: HudSensorTracker::MapShutdownAndReset
 * Purpose: End the overlay, remove loaded map nodes, free the map path, and reset map state.
 */
int HudSensorTracker::MapShutdownAndReset() {
    MapOverlayEndShow();
    while (mapNodeListHead != 0) {
        MapRemoveNode(mapNodeListHead);
    }

    if (loadedMapPath != 0) {
        free(loadedMapPath);
    }

    Init(0);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-mapremovenode
 * @recoil-artifact defines .text recoil:function:0x4167e0: HudSensorTracker::MapRemoveNode
 * Purpose: Unlink the requested map node from the tracker list and release head-node storage.
 */
int HudSensorTracker::MapRemoveNode(
    HudSensorMapNode *mapNode
) {
    HudSensorMapNode *head = mapNodeListHead;
    if (mapNode == head) {
        mapNodeListHead = head->next;
        if (mapNode != 0) {
            mapNode->FreePointArray();
            ::operator delete(mapNode);
        }

        return 1;
    }

    if (head == 0) {
        return 0;
    }

    while (head->next != mapNode) {
        head = head->next;
        if (head == 0) {
            return 0;
        }
    }

    if (head->next != mapNode) {
        return 0;
    }

    head->next = mapNode->next;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-mapinsertnodeandgrowbounds
 * @recoil-artifact defines .text recoil:function:0x416840: HudSensorTracker::MapInsertNodeAndGrowBounds
 * Purpose: Insert a map node at the list head and grow the tracker bounds from its cached extent.
 */
int HudSensorTracker::MapInsertNodeAndGrowBounds(
    HudSensorMapNode *mapNode
) {
    if (mapNode == 0) {
        return 0;
    }

    mapNode->next = mapNodeListHead;
    mapNodeListHead = mapNode;

    HudSensorMapBounds nodeBounds;
    mapNode->UpdateCachedBounds(&nodeBounds);

    if (nodeBounds.minX < mapBoundsMinX) {
        mapBoundsMinX = nodeBounds.minX;
    }
    if (nodeBounds.minZ < mapBoundsMinZ) {
        mapBoundsMinZ = nodeBounds.minZ;
    }
    if (nodeBounds.maxX > mapBoundsMaxX) {
        mapBoundsMaxX = nodeBounds.maxX;
    }
    if (nodeBounds.maxZ > mapBoundsMaxZ) {
        mapBoundsMaxZ = nodeBounds.maxZ;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-loadmapfromstream
 * @recoil-artifact defines .text recoil:function:0x4168d0: HudSensorTracker::LoadMapFromStream
 * Purpose: Read a versioned map stream into tracker bounds and linked map nodes.
 */
int HudSensorTracker::LoadMapFromStream(
    FILE *stream
) {
    if (stream == 0) {
        return 0;
    }

    fread(
        &mapFileVersion,
        sizeof(mapFileVersion),
        1,
        stream
    );
    if (mapFileVersion != 5) {
        zError::ReportOld(
            0x200,
            g_HudSensorTracker_MapCppSourcePath,
            0x30e,
            g_HudSensorTracker_IncorrectMapFileVersionFmt,
            mapFileVersion,
            5
        );
        return 0;
    }

    fread(
        &mapHeaderDword,
        sizeof(mapHeaderDword),
        1,
        stream
    );
    fread(
        &mapBoundsMinX,
        sizeof(HudSensorMapBounds),
        1,
        stream
    );

    for (;;) {
        HudSensorMapNode *mapNode = (HudSensorMapNode *)(::operator new(sizeof(HudSensorMapNode)));
        mapNode = mapNode != 0 ? mapNode->Init() : 0;
        if (mapNode->LoadFromStream(stream) == 0) {
            if (mapNode != 0) {
                mapNode->FreePointArray();
                ::operator delete(mapNode);
            }
            break;
        }

        MapInsertNodeAndGrowBounds(mapNode);
    }

    mapLoadedFlag = 1;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-loadmapfrompath
 * @recoil-artifact defines .text recoil:function:0x4169d0: HudSensorTracker::LoadMapFromPath
 * Purpose: Open a map file path, remember it, and load the tracker map from the stream.
 */
int HudSensorTracker::LoadMapFromPath(
    const char *path
) {
    if (path == 0) {
        return 0;
    }

    FILE *const stream = fopen(
        path,
        "rb"
    );
    if (stream == 0) {
        return 0;
    }

    loadedMapPath = _strdup(path);
    const int result = LoadMapFromStream(stream);
    fclose(stream);
    return result;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-mapoverlaybeginshow
 * @recoil-artifact defines .text recoil:function:0x416a30: HudSensorTracker::MapOverlayBeginShow
 * Purpose: Begin the map overlay scale lerp from the current scale to the fitted map bounds scale.
 */
int HudSensorTracker::MapOverlayBeginShow() {
    if (mapScaleLerpActive != 0) {
        return 0;
    }

    const int rectWidth = outerRect.right - outerRect.left;
    const int rectHeight = outerRect.bottom - outerRect.top;
    const int minExtent = rectWidth < rectHeight ? rectWidth : rectHeight;
    const float scaleExtent = (float)(minExtent);

    mapScaleLerpT = 0.0f;
    mapScaleLerpActive = 1;
    mapScaleStart = mapScaleCurrent;
    mapScaleGoal.x = scaleExtent / (mapBoundsMaxX - mapBoundsMinX);
    mapScaleGoal.z = scaleExtent / (mapBoundsMaxZ - mapBoundsMinZ);

    if (mapLoadedFlag != 0) {
        mapSndOn->PlayA3DSimple(1.0f);
        mapScaleLerpRunning = 1;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-mapoverlayendshow
 * @recoil-artifact defines .text recoil:function:0x416ad0: HudSensorTracker::MapOverlayEndShow
 * Purpose: Stop the active map overlay lerp and queue the deterministic map-off sound path.
 */
void HudSensorTracker::MapOverlayEndShow() {
    if (mapScaleLerpActive == 0) {
        return;
    }

    mapScaleLerpT = 0.0f;
    mapScaleStart.x = mapScaleCurrent.x;
    mapScaleGoal.x = 0.0f;
    mapScaleGoal.z = 0.0f;
    mapScaleStart.y = mapScaleCurrent.y;
    mapScaleLerpActive = 0;
    mapScaleStart.z = mapScaleCurrent.z;

    if (mapLoadedFlag != 0) {
        mapScaleLerpRunning = 1;
        mapSndOff->PlayA3DSimple(1.0f);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-mapoverlayreftoggle
 * @recoil-artifact defines .text recoil:function:0x416b30: HudSensorTracker::MapOverlayRefToggle
 * Purpose: Reference-count map overlay visibility requests and route transitions through begin/end show.
 */
int HudSensorTracker::MapOverlayRefToggle(
    int enable
) {
    if (enable != 0) {
        ++g_Hud_MapOverlayRefCount;
        if (g_Hud_MapOverlayRefCount > 0) {
            MapOverlayBeginShow();
        }
    } else {
        --g_Hud_MapOverlayRefCount;
        if (g_Hud_MapOverlayRefCount <= 0) {
            g_Hud_MapOverlayRefCount = 0;
            MapOverlayEndShow();
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-mapzoomin
 * @recoil-artifact defines .text recoil:function:0x416b80: HudSensorTracker::MapZoomIn
 * Purpose: Increase the active overlay zoom and play the map click sound while the overlay is shown.
 */
void HudSensorTracker::MapZoomIn() {
    if (mapScaleLerpActive != 0) {
        mapZoom *= 1.10000002f;
        mapSndClick->PlayA3DSimple(1.0f);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-mapzoomout
 * @recoil-artifact defines .text recoil:function:0x416bb0: HudSensorTracker::MapZoomOut
 * Purpose: Decrease the active overlay zoom and play the map click sound while the overlay is shown.
 */
void HudSensorTracker::MapZoomOut() {
    if (mapScaleLerpActive != 0) {
        mapZoom *= 0.899999976f;
        mapSndClick->PlayA3DSimple(1.0f);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-updatemapscalelerp
 * @recoil-artifact defines .text recoil:function:0x416be0: HudSensorTracker::UpdateMapScaleLerp
 * Purpose: Advance the map overlay scale interpolation and update the current overlay scale vector.
 */
int HudSensorTracker::UpdateMapScaleLerp() {
    if (mapScaleLerpRunning != 0) {
        mapScaleLerpT += mapScaleLerpStep;
        if (mapScaleLerpT >= 1.0f) {
            mapScaleLerpRunning = 0;
            mapScaleLerpT = 1.0f;
        }

        const float lerpT = mapScaleLerpT;
        mapScaleCurrent.x = (mapScaleGoal.x - mapScaleStart.x) * lerpT + mapScaleStart.x;
        mapScaleCurrent.y = (mapScaleGoal.y - mapScaleStart.y) * lerpT + mapScaleStart.y;
        mapScaleCurrent.z = (mapScaleGoal.z - mapScaleStart.z) * lerpT + mapScaleStart.z;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-projectworldpointstooverlay
 * @recoil-artifact defines .text recoil:function:0x416c90: HudSensorTracker::ProjectWorldPointsToOverlay
 * Purpose: Project world-space map points into overlay coordinates using the tracked origin and forward vector.
 */
int HudSensorTracker::ProjectWorldPointsToOverlay(
    const zVec3 *inputWorldPoints,
    zVec3 *projectedOverlayPoints,
    int pointCount
) {
    const zVec3 *const trackedPos = trackedWorldOriginPtr;
    const zVec3 *const trackedForward = trackedForwardVecPtr;

    {
        for (int index = 0; index < pointCount; ++index) {
            const float deltaX =
                (inputWorldPoints[index].x - trackedPos->x) * mapZoom * mapScaleCurrent.x;
            const float deltaZ =
                (inputWorldPoints[index].z - trackedPos->z) * mapScaleCurrent.z * mapZoom;

            projectedOverlayPoints[index].x = (float)(mapOverlayCenterX) +
                                              trackedForward->x * deltaZ -
                                              trackedForward->z * deltaX;
            projectedOverlayPoints[index].y =
                (float)(mapOverlayCenterY)-trackedForward->x * deltaX - trackedForward->z * deltaZ;
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-drawtrackedsavestatemarker
 * @recoil-artifact defines .text recoil:function:0x416d50: HudSensorTracker::DrawTrackedSaveStateMarker
 * Purpose: Draw the cross marker for the currently tracked save-state at its projected map position.
 */
int HudSensorTracker::DrawTrackedSaveStateMarker() {
    unsigned short markerColor;
    if (zOpt::GetNetworkEnabled() != 0) {
        zUtil_SaveGameState *const gameState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
        markerColor =
            (unsigned short)(zVid_PackColor00RRGGBB(gameState->netPlayerRow->playerColorPackedRgb));
    } else {
        markerColor = (unsigned short)(zVid_PackColorRGB(
            0,
            0xff,
            0
        ));
    }

    zVec3 projectedScreenPoint;
    ProjectWorldPointsToOverlay(
        &trackedSaveStateSelection->playerState->worldPos,
        &projectedScreenPoint,
        1
    );
    DrawMarkerCross(
        (int)(projectedScreenPoint.x),
        (int)(projectedScreenPoint.y),
        3,
        3,
        markerColor,
        this
    );
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-drawmarkercross
 * @recoil-artifact defines .text recoil:function:0x416dd0: HudSensorTracker::DrawMarkerCross
 * Purpose: Draw a centered cross marker as two clipped immediate line strips.
 */
void __fastcall HudSensorTracker::DrawMarkerCross(
    int centerX,
    int centerY,
    int armHalfWidth,
    int armHalfHeight,
    int markerColor,
    HudSensorTracker *tracker
) {
    zRndr_LinePoint2I points[2];
    const int color16 = markerColor & 0xffff;

    points[0].x = centerX - armHalfWidth;
    points[0].y = centerY;
    points[1].x = centerX + armHalfWidth;
    points[1].y = centerY;
    zRndr_DrawClippedImmediateLineStrip(
        points,
        1,
        tracker,
        color16
    );

    points[0].x = centerX;
    points[0].y = centerY + armHalfHeight;
    points[1].x = centerX;
    points[1].y = centerY - armHalfHeight;
    zRndr_DrawClippedImmediateLineStrip(
        points,
        1,
        tracker,
        color16
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-getsavestaterelativevectorlen
 * @recoil-artifact defines .text recoil:function:0x416e50: HudSensorTracker::GetSaveStateRelativeVectorLen
 * Purpose: Compute the flat relative vector and squared or true distance to a save-state marker.
 */
float HudSensorTracker::GetSaveStateRelativeVectorLen(
    zUtil_SaveGameState *saveState,
    zVec3 *relativeDelta,
    int takeSqrt
) {
    const zVec3 *const saveStatePos = &saveState->playerState->worldPos;
    const zVec3 *const trackedOrigin = trackedWorldOriginPtr;

    relativeDelta->x = saveStatePos->x - trackedOrigin->x;
    relativeDelta->y = saveStatePos->y - trackedOrigin->y;
    relativeDelta->z = saveStatePos->z - trackedOrigin->z;
    relativeDelta->y = 0.0f;

    const float lengthSq = relativeDelta->x * relativeDelta->x +
                           relativeDelta->y * relativeDelta->y +
                           relativeDelta->z * relativeDelta->z;
    if (takeSqrt != 0) {
        return sqrt(lengthSq);
    }

    return lengthSq;
}

 /**
  * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-setsavestatemarkermaxdistance
  * @recoil-artifact defines .text recoil:function:0x416ef0: HudSensorTracker::SetSaveStateMarkerMaxDistance
  * Purpose: Store the squared maximum distance for drawing save-state markers.
  */
int HudSensorTracker::SetSaveStateMarkerMaxDistance(
    float maxDist
) {
    saveStateMarkerMaxDistSq = maxDist * maxDist;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-drawsavestatemarker
 * @recoil-artifact defines .text recoil:function:0x416f10: HudSensorTracker::DrawSaveStateMarker
 * Purpose: Draw one non-tracked save-state marker or its edge-clamped network marker.
 */
int HudSensorTracker::DrawSaveStateMarker(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->lifecycleState == 4 || saveState == trackedSaveStateSelection) {
        return 0;
    }

    zVec3 relativeDelta;
    const float distanceSq = GetSaveStateRelativeVectorLen(
        saveState,
        &relativeDelta,
        0
    );

    zVec3 markerPoint;
    if (saveStateMarkerMaxDistSq != 0.0f && distanceSq > saveStateMarkerMaxDistSq) {
        if (zOpt::GetNetworkEnabled() == 0) {
            return 0;
        }

        float edgeScale = saveStateMarkerMaxDistSq / distanceSq;
        int edgeScaleBits;
        memcpy(&edgeScaleBits, &edgeScale, sizeof(edgeScaleBits));
        edgeScaleBits = (edgeScaleBits >> 1) + 0x1fc00000;
        memcpy(&edgeScale, &edgeScaleBits, sizeof(edgeScale));
        relativeDelta.x *= edgeScale;
        relativeDelta.y *= edgeScale;
        relativeDelta.z *= edgeScale;

        const zVec3 *const localWorldPos =
            &((zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState))->worldPos;
        markerPoint.x = localWorldPos->x + relativeDelta.x;
        markerPoint.y = localWorldPos->y + relativeDelta.y;
        markerPoint.z = localWorldPos->z + relativeDelta.z;
        ProjectWorldPointsToOverlay(
            &markerPoint,
            &markerPoint,
            1
        );

        const unsigned short markerColor =
            (unsigned short)(zVid_PackColor00RRGGBB(saveState->netPlayerRow->playerColorPackedRgb));
        if (IsPointStrictlyInsideRect(
            outerRect,
            markerPoint
        )) {
            DrawMarkerCross(
                (int)(markerPoint.x),
                (int)(markerPoint.y),
                3,
                3,
                markerColor,
                this
            );
        }

        return 1;
    }

    ProjectWorldPointsToOverlay(
        &playerState->worldPos,
        &markerPoint,
        1
    );

    unsigned short markerColor;
    if (zOpt::GetNetworkEnabled() != 0) {
        markerColor =
            (unsigned short)(zVid_PackColor00RRGGBB(saveState->netPlayerRow->playerColorPackedRgb));
    } else {
        markerColor = (unsigned short)(zVid_PackColorRGB(
            0xff,
            0,
            0
        ));
    }

    if (IsPointStrictlyInsideRect(
        outerRect,
        markerPoint
    )) {
        zRndr_SpanOcclusion_TestSample(
            (int)(markerPoint.x),
            (int)(markerPoint.y),
            markerColor
        );
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-update
 * @recoil-artifact defines .text recoil:function:0x417130: HudSensorTracker::Update
 * Purpose: Advance map interpolation, draw map nodes, and draw save-state tracker markers.
 */
void HudSensorTracker::Update() {
    mapScaleLerpStep = 0.150000006f;
    if (mapScaleLerpActive == 0 && mapScaleLerpRunning == 0) {
        return;
    }

    UpdateMapScaleLerp();

    if (trackedWorldOriginPtr == 0) {
        trackedWorldFallbackOrigin.x = mapBoundsMinX - (mapBoundsMaxX - mapBoundsMinX) * -0.5f;
        trackedWorldFallbackOrigin.z = mapBoundsMinZ - (mapBoundsMaxZ - mapBoundsMinZ) * -0.5f;
        trackedWorldOriginPtr = &trackedWorldFallbackOrigin;
    }

    if (trackedForwardVecPtr == 0) {
        trackedForwardFallbackVec.x = 0.0f;
        trackedForwardFallbackVec.y = 0.0f;
        trackedForwardFallbackVec.z = -1.0f;
    }

    if (zOpt::GetNetworkEnabled() == 0) {
        HudSensorMapNode *mapNode = mapNodeListHead;
        while (mapNode != 0) {
            mapNode->DrawOnTracker(
                this,
                trackedWorldPosPtr
            );
            mapNode = mapNode->next;
        }
    }

    zUtil_SaveGameState *saveState =
        g_PlayerSaveStateListHead != 0 ? g_PlayerSaveStateListHead->next : 0;
    while (saveState != 0) {
        DrawSaveStateMarker(saveState);
        saveState = saveState != 0 ? saveState->next : 0;
    }

    if (mapLoadedFlag != 0) {
        DrawTrackedSaveStateMarker();
    }
}

 /**
  * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-settrackedsavestate
  * @recoil-artifact defines .text recoil:function:0x417220: HudSensorTracker::SetTrackedSaveState
  * Purpose: Select the save-state player pose used by the HUD map marker.
  */
int HudSensorTracker::SetTrackedSaveState(
    zUtil_SaveGameState *saveState
) {
    zVec3 *trackedForwardVec = 0;
    if (saveState == 0) {
        trackedSaveStateSelection = 0;
        trackedWorldOriginPtr = 0;
    } else {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        trackedSaveStateSelection = saveState;
        trackedWorldOriginPtr = &playerState->worldPos;
        trackedForwardVec = &playerState->cameraBasisCache;
    }

    trackedForwardVecPtr = trackedForwardVec;
    mapZoom = 1.0f;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-loadmissionmapandsfx
 * @recoil-artifact defines .text recoil:function:0x417260: HudSensorTracker::LoadMissionMapAndSfx
 * Purpose: Load the mission map path and resolve the map on, off, and click samples.
 */
int HudSensorTracker::LoadMissionMapAndSfx(
    int missionIdValue
) {
    char mapPath[0x40];
    sprintf(
        mapPath,
        g_HudSensorTracker_MissionMapPathFmt,
        missionIdValue
    );

    const int result = LoadMapFromPath(mapPath);
    mapSndOn = zSnd::FindSampleByName(g_HudSensorTracker_MapOnSfxName);
    mapSndOff = zSnd::FindSampleByName(g_HudSensorTracker_MapOffSfxName);
    mapSndClick = zSnd::FindSampleByName(g_HudSensorTracker_MapClickSfxName);
    return result;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-setobjectivemarkerenabledandcolor
 * @recoil-artifact defines .text recoil:function:0x4172c0: HudSensorTracker::SetObjectiveMarkerEnabledAndColor
 * Purpose: Apply visibility and RGB color to every map node for the requested objective index.
 */
int HudSensorTracker::SetObjectiveMarkerEnabledAndColor(
    int objectiveIndex,
    int enabled,
    const unsigned char *colorRgb24
) {
    HudSensorMapNode *mapNode = mapNodeListHead;
    while (mapNode != 0) {
        if (mapNode->objectiveIndex == objectiveIndex) {
            mapNode->SetColorRgb(colorRgb24);
            mapNode->SetEnabled(enabled);
        }

        mapNode = mapNode->next;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.map.hudsensortracker-setobjectivemarkercolorblink
 * @recoil-artifact defines .text recoil:function:0x417300: HudSensorTracker::SetObjectiveMarkerColorBlink
 * Purpose: Recolor matching objective map nodes and swap their packed full/half 565 blink colors.
 */
int HudSensorTracker::SetObjectiveMarkerColorBlink(
    int objectiveIndex,
    const unsigned char *colorRgb24
) {
    HudSensorMapNode *mapNode = mapNodeListHead;
    while (mapNode != 0) {
        if (mapNode->objectiveIndex == objectiveIndex) {
            mapNode->SetColorRgb(colorRgb24);
            const unsigned int packedColor = (unsigned int)(mapNode->packedColor565Pair);
            mapNode->packedColor565Pair =
                (int)((packedColor << 16) | ((packedColor >> 16) & 0xffff));
        }

        mapNode = mapNode->next;
    }

    return 1;
}
