#include "recoil/Mfc42Abi.h"
#include "player.h"

#include "Battlesport/game_net.h"
#include "Battlesport/ai_net.h"
#include "Battlesport/pickup.h"
#include "Battlesport/wol_api.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "hud_sensor_tracker.h"
#include "opt_catalog.h"
#include "hud.h"

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char g_HudUiCounterText_PlayerLabel[];

/**
 * Purpose: remember the most recent mover node accepted from movers.zrd.
 */
extern "C" zClass_NodePartial *g_Mover_LastLoadedNode = 0;

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudcountervalue
 * @recoil-artifact defines .data recoil:data:0x4f3764: g_Player_HudCounterValue.
 * BN types this as a zero-filled .data int restored by
 * Player::ApplyMissionSaveData, accumulated by AddScaledHudCounterValue, and
 * mirrored into mission-save/HUD objective counter paths.
 * Purpose: Stores the local mission objective HUD counter value.
 */
int g_Player_HudCounterValue = 0;
unsigned char g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = 0;
PlayerNodeFlagRestoreEntry *g_PlayerNodeFlagRestoreEntriesBegin = 0;
PlayerNodeFlagRestoreEntry *g_PlayerNodeFlagRestoreEntriesEnd = 0;
PlayerNodeFlagRestoreEntry *g_PlayerNodeFlagRestoreEntriesCapacityEnd = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playermastercommondatalistaux
 * @recoil-artifact defines .data recoil:data:0x4f3a68: g_PlayerMasterCommonDataListAux.
 * Data owner 0x4f3a68..0x4f3a77: zero-initialized PlayerMasterCommonData intrusive-list
 * globals cleared by Player::InitMasterCommonDataList.
 * Purpose: stores the plan-tracked g_PlayerMasterCommonDataListAux gameplay data symbol.
 */
int g_PlayerMasterCommonDataListAux = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playermastercommondatahead
 * @recoil-artifact defines .data recoil:data:0x4f3a6c: g_PlayerMasterCommonDataHead.
 * Purpose: stores the plan-tracked g_PlayerMasterCommonDataHead gameplay data symbol.
 */
PlayerMasterCommonData *g_PlayerMasterCommonDataHead = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playermastercommondatatail
 * @recoil-artifact defines .data recoil:data:0x4f3a70: g_PlayerMasterCommonDataTail.
 * Purpose: stores the plan-tracked g_PlayerMasterCommonDataTail gameplay data symbol.
 */
PlayerMasterCommonData *g_PlayerMasterCommonDataTail = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playermastercommondatacount
 * @recoil-artifact defines .data recoil:data:0x4f3a74: g_PlayerMasterCommonDataCount.
 * Purpose: stores the plan-tracked g_PlayerMasterCommonDataCount gameplay data symbol.
 */
int g_PlayerMasterCommonDataCount = 0;
/**
 * Storage group:
 * g_PlayerMasterModalDataListAux, g_PlayerMasterModalDataHead,
 * g_PlayerMasterModalDataTail, and g_PlayerMasterModalDataCount.
 * BN types this as a zero-filled .data PlayerMasterModalData intrusive-list
 * bootstrap group cleared by Player::InitMasterModalDataList, appended by
 * PlayerAllocMasterModalData, and drained by Player::ClearLoadedData.
 * Purpose: Stores the master modal-data intrusive list used while creating
 * players from name/bootstrap data.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playermastermodaldatalistaux
 * @recoil-artifact defines .data recoil:data:0x4f3688: g_PlayerMasterModalDataListAux.
 * Purpose: stores the plan-tracked g_PlayerMasterModalDataListAux gameplay data symbol.
 */
int g_PlayerMasterModalDataListAux = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playermastermodaldatahead
 * @recoil-artifact defines .data recoil:data:0x4f368c: g_PlayerMasterModalDataHead.
 * Purpose: stores the plan-tracked g_PlayerMasterModalDataHead gameplay data symbol.
 */
PlayerMasterModalData *g_PlayerMasterModalDataHead = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playermastermodaldatatail
 * @recoil-artifact defines .data recoil:data:0x4f3690: g_PlayerMasterModalDataTail.
 * Purpose: stores the plan-tracked g_PlayerMasterModalDataTail gameplay data symbol.
 */
PlayerMasterModalData *g_PlayerMasterModalDataTail = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playermastermodaldatacount
 * @recoil-artifact defines .data recoil:data:0x4f3694: g_PlayerMasterModalDataCount.
 * Purpose: stores the plan-tracked g_PlayerMasterModalDataCount gameplay data symbol.
 */
int g_PlayerMasterModalDataCount = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-localcontrolenabled
 * @recoil-artifact defines .data recoil:data:0x4f36b0: g_Player_LocalControlEnabled.
 * BN types this as a zero-filled .data int seeded from the network option by
 * Player::InitMissionRuntimeFromWorldAndCamera and toggled by local-control
 * input paths.
 * Purpose: Gates local player command handling during mission runtime.
 */
int g_Player_LocalControlEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-runtimeinputflags
 * @recoil-artifact defines .data recoil:data:0x4f36a0: g_Player_RuntimeInputFlags.
 * BN types this as a zero-filled .data int reset by ZAR_RegisterSections and
 * read by local gameplay input/runtime-control paths.
 * Purpose: Stores player runtime input mode flags for the current mission.
 */
int g_Player_RuntimeInputFlags = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-camerazone
 * @recoil-artifact defines .data recoil:data:0x4f36f0: g_Player_CameraZone.
 * BN types this as a zero-filled .data float overwritten from player.zrd
 * camera_zone tuning, or by the mission-runtime default, before camera input
 * reads it.
 * Purpose: Stores the camera dead-zone threshold for player aim/camera input.
 */
float g_Player_CameraZone = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-camerazoneinvrange
 * @recoil-artifact defines .data recoil:data:0x4f36f4: g_Player_CameraZoneInvRange.
 * BN types this as a zero-filled .data float paired with g_Player_CameraZone
 * and seeded during mission-runtime player.zrd tuning load.
 * Purpose: Stores the reciprocal scale for input outside the camera dead zone.
 */
float g_Player_CameraZoneInvRange = 0.0f;
/**
 * Storage group: Player ZRD runtime tuning globals.
 * BN types these as independent zero-filled .data globals written by
 * Player::InitMissionRuntimeFromWorldAndCamera from player.zrd nodes:
 * camera, underwater-camera, gravity/sink, slope, and heat/cold option tuning.
 * Purpose: Stores mission runtime tuning loaded from player.zrd.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-maxcamyawrate
 * @recoil-artifact defines .data recoil:data:0x4f36f8: g_Player_MaxCamYawRate.
 * Purpose: stores the plan-tracked g_Player_MaxCamYawRate gameplay data symbol.
 */
float g_Player_MaxCamYawRate = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mousepushx
 * @recoil-artifact defines .data recoil:data:0x4f36fc: g_Player_MousePushX.
 * Purpose: stores the plan-tracked g_Player_MousePushX gameplay data symbol.
 */
float g_Player_MousePushX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mousepushy
 * @recoil-artifact defines .data recoil:data:0x4f3700: g_Player_MousePushY.
 * Purpose: stores the plan-tracked g_Player_MousePushY gameplay data symbol.
 */
float g_Player_MousePushY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameraelastic
 * @recoil-artifact defines .data recoil:data:0x4f3704: g_Player_CameraElastic.
 * Purpose: stores the plan-tracked g_Player_CameraElastic gameplay data symbol.
 */
float g_Player_CameraElastic = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-maxcamtetheranglerad
 * @recoil-artifact defines .data recoil:data:0x4f3708: g_Player_MaxCamTetherAngleRad.
 * Purpose: stores the plan-tracked g_Player_MaxCamTetherAngleRad gameplay data symbol.
 */
float g_Player_MaxCamTetherAngleRad = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-fpcamelevationrate
 * @recoil-artifact defines .data recoil:data:0x4f370c: g_Player_FpCamElevationRate.
 * Purpose: stores the plan-tracked g_Player_FpCamElevationRate gameplay data symbol.
 */
float g_Player_FpCamElevationRate = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-fpcamelevationmax
 * @recoil-artifact defines .data recoil:data:0x4f3710: g_Player_FpCamElevationMax.
 * Purpose: stores the plan-tracked g_Player_FpCamElevationMax gameplay data symbol.
 */
float g_Player_FpCamElevationMax = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-fpcamelevationmin
 * @recoil-artifact defines .data recoil:data:0x4f3714: g_Player_FpCamElevationMin.
 * Purpose: stores the plan-tracked g_Player_FpCamElevationMin gameplay data symbol.
 */
float g_Player_FpCamElevationMin = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamdistance
 * @recoil-artifact defines .data recoil:data:0x4f371c: g_Player_UnderwaterCamDistance.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamDistance gameplay data symbol.
 */
float g_Player_UnderwaterCamDistance = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamheight
 * @recoil-artifact defines .data recoil:data:0x4f3720: g_Player_UnderwaterCamHeight.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamHeight gameplay data symbol.
 */
float g_Player_UnderwaterCamHeight = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamstepcount
 * @recoil-artifact defines .data recoil:data:0x4f3724: g_Player_UnderwaterCamStepCount.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamStepCount gameplay data symbol.
 */
int g_Player_UnderwaterCamStepCount = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamfar
 * @recoil-artifact defines .data recoil:data:0x4f3728: g_Player_UnderwaterCamFar.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamFar gameplay data symbol.
 */
float g_Player_UnderwaterCamFar = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercampackedcolor
 * @recoil-artifact defines .data recoil:data:0x4f372c: g_Player_UnderwaterCamPackedColor.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamPackedColor gameplay data symbol.
 */
unsigned int g_Player_UnderwaterCamPackedColor = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-underwatercamalpha
 * @recoil-artifact defines .data recoil:data:0x4f3730: g_Player_UnderwaterCamAlpha.
 * Purpose: stores the plan-tracked g_Player_UnderwaterCamAlpha gameplay data symbol.
 */
float g_Player_UnderwaterCamAlpha = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-gameplayinputstepscale
 * @recoil-artifact defines .data recoil:data:0x4dc970: g_Player_GameplayInputStepScale.
 * BN types this as an initialized .data float read by local mouse/cursor
 * steering when cursor mode uses mouse deltas.
 * Purpose: Scales mouse delta input into player steering command steps.
 */
float g_Player_GameplayInputStepScale = 0.03f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameraheadingdotabs
 * @recoil-artifact defines .data recoil:data:0x4da398: g_Player_CameraHeadingDotAbs.
 * Purpose: stores the plan-tracked g_Player_CameraHeadingDotAbs gameplay data symbol.
 */
float g_Player_CameraHeadingDotAbs = 1.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameraheadinglerpbasewhenflagclear
 * @recoil-artifact defines .data recoil:data:0x4da39c: g_Player_CameraHeadingLerpBaseWhenFlagClear.
 * Purpose: stores the plan-tracked g_Player_CameraHeadingLerpBaseWhenFlagClear gameplay data symbol.
 */
float g_Player_CameraHeadingLerpBaseWhenFlagClear = 3.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameraheadinglerpbasewhenflagset
 * @recoil-artifact defines .data recoil:data:0x4da3a0: g_Player_CameraHeadingLerpBaseWhenFlagSet.
 * Purpose: stores the plan-tracked g_Player_CameraHeadingLerpBaseWhenFlagSet gameplay data symbol.
 */
float g_Player_CameraHeadingLerpBaseWhenFlagSet = 2.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-posyaw
 * @recoil-artifact defines .data recoil:data:0x4dc9a8: g_Player_HudReadoutFmt_PosYaw.
 * BN types this as a writable .data char[0x14] read by
 * Player::UpdateDebugOverlayHud for the position/yaw debug overlay line.
 * Purpose: Formats the debug HUD position and yaw readout.
 */
char g_Player_HudReadoutFmt_PosYaw[0x14] = "POS %d %d %d YAW %d";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_PosYaw) == 0x14);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-dynamics
 * @recoil-artifact defines .data recoil:data:0x4dc9bc: g_Player_HudReadoutFmt_Dynamics.
 * BN types this as a writable .data char[0x15] read by
 * Player::UpdateDebugOverlayHud for the normal dynamics debug overlay line.
 * Purpose: Formats the debug HUD player dynamics readout.
 */
char g_Player_HudReadoutFmt_Dynamics[0x15] = "%s using %s dynamics";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_Dynamics) == 0x15);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-dynamicss
 * @recoil-artifact defines .data recoil:data:0x4dc9d4: g_Player_HudReadoutFmt_DynamicsS.
 * BN types this as a writable .data char[0x19] read by
 * Player::UpdateDebugOverlayHud for the slipping dynamics debug overlay line.
 * Purpose: Formats the debug HUD player dynamics readout while slipping.
 */
char g_Player_HudReadoutFmt_DynamicsS[0x19] = "%s using %s dynamics - S";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_DynamicsS) == 0x19);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-dynamicsa
 * @recoil-artifact defines .data recoil:data:0x4dc9f0: g_Player_HudReadoutFmt_DynamicsA.
 * BN types this as a writable .data char[0x19] read by
 * Player::UpdateDebugOverlayHud for the airborne dynamics debug overlay line.
 * Purpose: Formats the debug HUD player dynamics readout while airborne.
 */
char g_Player_HudReadoutFmt_DynamicsA[0x19] = "%s using %s dynamics - A";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_DynamicsA) == 0x19);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-dead
 * @recoil-artifact defines .data recoil:data:0x4dca0c: g_Player_HudReadoutFmt_Dead.
 * BN types this as a writable .data char[0x0c] read by
 * Player::UpdateDebugOverlayHud for the inactive-player debug overlay line.
 * Purpose: Formats the debug HUD inactive player readout.
 */
char g_Player_HudReadoutFmt_Dead[0x0c] = "%s is DEAD!";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_Dead) == 0x0c);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-modegoalnode
 * @recoil-artifact defines .data recoil:data:0x4dca18: g_Player_HudReadoutFmt_ModeGoalNode.
 * BN types this as a writable .data char[0x26] read by
 * Player::UpdateDebugOverlayHud for the AI mode/goal-node debug overlay line.
 * Purpose: Formats the debug HUD AI mode and goal-node readout.
 */
char g_Player_HudReadoutFmt_ModeGoalNode[0x26] =
    "%s is in mode %d and had goal node %d";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_ModeGoalNode) == 0x26);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-unknown
 * @recoil-artifact defines .data recoil:data:0x4dca40: g_Player_MasterTypeName_Unknown.
 * BN types this as a writable .data char[0x08] shared by
 * Player::UpdateDebugOverlayHud, zNetwork_DPlay_ReportError, and zSnd error
 * reporters for fallback "UNKNOWN" diagnostics.
 * Purpose: Names the shared fallback diagnostic/master-type token.
 */
char g_Player_MasterTypeName_Unknown[0x08] = "UNKNOWN";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Unknown) == 0x08);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-fly
 * @recoil-artifact defines .data recoil:data:0x4dca48: g_Player_MasterTypeName_Fly.
 * BN types this as a writable .data char[0x04] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the fly master type in the debug HUD.
 */
char g_Player_MasterTypeName_Fly[0x04] = "FLY";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Fly) == 0x04);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-sub
 * @recoil-artifact defines .data recoil:data:0x4dca4c: g_Player_MasterTypeName_Sub.
 * BN types this as a writable .data char[0x04] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the sub master type in the debug HUD.
 */
char g_Player_MasterTypeName_Sub[0x04] = "SUB";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Sub) == 0x04);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-amphib
 * @recoil-artifact defines .data recoil:data:0x4dca50: g_Player_MasterTypeName_Amphib.
 * BN types this as a writable .data char[0x07] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the amphib master type in the debug HUD.
 */
char g_Player_MasterTypeName_Amphib[0x07] = "AMPHIB";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Amphib) == 0x07);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-hover
 * @recoil-artifact defines .data recoil:data:0x4dca58: g_Player_MasterTypeName_Hover.
 * BN types this as a writable .data char[0x06] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the hover master type in the debug HUD.
 */
char g_Player_MasterTypeName_Hover[0x06] = "HOVER";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Hover) == 0x06);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-track
 * @recoil-artifact defines .data recoil:data:0x4dca60: g_Player_MasterTypeName_Track.
 * BN types this as a writable .data char[0x06] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the track master type in the debug HUD.
 */
char g_Player_MasterTypeName_Track[0x06] = "TRACK";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Track) == 0x06);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-basic
 * @recoil-artifact defines .data recoil:data:0x4dca68: g_Player_MasterTypeName_Basic.
 * BN types this as a writable .data char[0x06] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the basic master type in the debug HUD.
 */
char g_Player_MasterTypeName_Basic[0x06] = "BASIC";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Basic) == 0x06);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptertypename02
 * @recoil-artifact defines .data recoil:data:0x4dca70: g_Player_CopterTypeName02.
 * BN types this as a writable .data char[0x09] used by the copter sound-node
 * cache when binding the second copter actor by type/name.
 * Purpose: Names the second copter object for copter sound-node caching.
 */
char g_Player_CopterTypeName02[0x09] = "copter02";
RECOIL_STATIC_ASSERT(sizeof(g_Player_CopterTypeName02) == 0x09);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptertypename01
 * @recoil-artifact defines .data recoil:data:0x4dca7c: g_Player_CopterTypeName01.
 * BN types this as a writable .data char[0x09] used by the copter sound-node
 * cache when binding the first copter actor by type/name.
 * Purpose: Names the first copter object for copter sound-node caching.
 */
char g_Player_CopterTypeName01[0x09] = "copter01";
RECOIL_STATIC_ASSERT(sizeof(g_Player_CopterTypeName01) == 0x09);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-pickupoptkey-drop
 * @recoil-artifact defines .data recoil:data:0x4dca88: g_PickupOptKey_Drop.
 * BN types this as a writable .data char[0x05] used by Player async command
 * callback case 914 when spawning a carrier-node pickup.
 * Purpose: Names the drop pickup option key used by async debug commands.
 */
char g_PickupOptKey_Drop[0x05] = "drop";
RECOIL_STATIC_ASSERT(sizeof(g_PickupOptKey_Drop) == 0x05);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-pickupoptkey-crbox
 * @recoil-artifact defines .data recoil:data:0x4dca90: g_PickupOptKey_Crbox.
 * BN types this as a writable .data char[0x06] used by Player async command
 * callback case 913 when spawning a carrier-node pickup.
 * Purpose: Names the crbox pickup option key used by async debug commands.
 */
char g_PickupOptKey_Crbox[0x06] = "crbox";
RECOIL_STATIC_ASSERT(sizeof(g_PickupOptKey_Crbox) == 0x06);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-pickupoptkey-vwbus
 * @recoil-artifact defines .data recoil:data:0x4dca98: g_PickupOptKey_Vwbus.
 * BN types this as a writable .data char[0x06] used by Player async command
 * callback case 912 when spawning a carrier-node pickup.
 * Purpose: Names the vwbus pickup option key used by async debug commands.
 */
char g_PickupOptKey_Vwbus[0x06] = "vwbus";
RECOIL_STATIC_ASSERT(sizeof(g_PickupOptKey_Vwbus) == 0x06);
/**
 * Storage group: player save-state intrusive list.
 * BN exposes the zero-filled .data aux/head/tail/count fields used by player
 * creation, teardown, and ZAR VehicleList traversal/serialization.
 * Purpose: Tracks every active player save-state record in mission order.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playersavestatelistaux
 * @recoil-artifact defines .data recoil:data:0x4f3a78: g_PlayerSaveStateListAux.
 * Purpose: stores the plan-tracked g_PlayerSaveStateListAux gameplay data symbol.
 */
int g_PlayerSaveStateListAux = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playersavestatelisthead
 * @recoil-artifact defines .data recoil:data:0x4f3a7c: g_PlayerSaveStateListHead.
 * Purpose: stores the plan-tracked g_PlayerSaveStateListHead gameplay data symbol.
 */
zUtil_SaveGameState *g_PlayerSaveStateListHead = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playersavestatelisttail
 * @recoil-artifact defines .data recoil:data:0x4f3a80: g_PlayerSaveStateListTail.
 * Purpose: stores the plan-tracked g_PlayerSaveStateListTail gameplay data symbol.
 */
zUtil_SaveGameState *g_PlayerSaveStateListTail = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playersavestatecount
 * @recoil-artifact defines .data recoil:data:0x4f3a84: g_PlayerSaveStateCount.
 * Purpose: stores the plan-tracked g_PlayerSaveStateCount gameplay data symbol.
 */
int g_PlayerSaveStateCount = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-localplayersavestate
 * @recoil-artifact defines .data recoil:data:0x4f36a4: g_LocalPlayerSaveState.
 * BN types this as a zero-filled .data zUtil_SaveGameState pointer written
 * during mission-runtime initialization and read by the mission save/load
 * payload, local-control, camera-anchor, and HUD/gameplay paths.
 * Purpose: Points at the active local player's save-state record.
 */
zUtil_SaveGameState *g_LocalPlayerSaveState = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player2savestate
 * @recoil-artifact defines .data recoil:data:0x4f3770: g_Player2SaveState.
 * BN types this as a zero-filled .data zUtil_SaveGameState pointer assigned to
 * the stealth save-state created during mission-runtime bootstrap.
 * Purpose: Holds the hidden second-player/stealth save-state record.
 */
zUtil_SaveGameState *g_Player2SaveState = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-currentplayersavestate
 * @recoil-artifact defines .data recoil:data:0x4f36a8: g_CurrentPlayerSaveState.
 * Purpose: stores the plan-tracked g_CurrentPlayerSaveState gameplay data symbol.
 */
zUtil_SaveGameState *g_CurrentPlayerSaveState = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-lastvalidcameravarianttag
 * @recoil-artifact defines .data recoil:data:0x4f3718: g_Player_LastValidCameraVariantTag.
 * BN types this as a zero-filled .data zTag4 copied into and out of the
 * Player ZAR mission-save section as one packed 32-bit value.
 * Purpose: Remembers the last camera variant tag valid for mission save/load.
 */
zTag4Partial g_Player_LastValidCameraVariantTag = {0};
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-thirdpersoncamerasideprobeoffsetscale
 * @recoil-artifact defines .data recoil:data:0x4da3a4: g_Player_ThirdPersonCameraSideProbeOffsetScale.
 * Purpose: stores the plan-tracked g_Player_ThirdPersonCameraSideProbeOffsetScale gameplay data symbol.
 */
float g_Player_ThirdPersonCameraSideProbeOffsetScale = 1.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-cameravariantupdatedthistick
 * @recoil-artifact defines .data recoil:data:0x4e5cc0: g_Player_CameraVariantUpdatedThisTick.
 * Purpose: stores the plan-tracked g_Player_CameraVariantUpdatedThisTick gameplay data symbol.
 */
int g_Player_CameraVariantUpdatedThisTick = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-rebuildcameradirflatfromcurrenttarget
 * @recoil-artifact defines .data recoil:data:0x4e5cd8: g_Player_RebuildCameraDirFlatFromCurrentTarget.
 * Purpose: stores the plan-tracked g_Player_RebuildCameraDirFlatFromCurrentTarget gameplay data symbol.
 */
int g_Player_RebuildCameraDirFlatFromCurrentTarget = 0;
zVec3 g_Player_AmphibBasisUpRef = {0.0f, 1.0f, 0.0f};
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-amphibsteerbasislerprate
 * @recoil-artifact defines .data recoil:data:0x4dc9a4: g_Player_AmphibSteerBasisLerpRate.
 * Purpose: stores the plan-tracked g_Player_AmphibSteerBasisLerpRate gameplay data symbol.
 */
float g_Player_AmphibSteerBasisLerpRate = 3.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nextordinal
 * @recoil-artifact defines .data recoil:data:0x4f3a94: g_Player_NextOrdinal.
 * Purpose: stores the plan-tracked g_Player_NextOrdinal gameplay data symbol.
 */
int g_Player_NextOrdinal = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-aimode2state1finalized
 * @recoil-artifact defines .data recoil:data:0x4f36ac: g_Player_AiMode2State1Finalized.
 * BN types this as a zero-filled .data int written by
 * AINet::AiFinalizeMode2State1ForAllPlayers and read by the Mode2 State1 AI
 * steering/latch helpers.
 * Purpose: Latches completion of the Mode2 State1 saved-state finalization pass.
 */
int g_Player_AiMode2State1Finalized = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-healthysubnodename
 * @recoil-artifact defines .data recoil:data:0x4db5ec: g_Player_HealthySubNodeName.
 * Purpose: Names the shared healthy child node used by player, pickup, and
 * turret paths.
 */
char g_Player_HealthySubNodeName[8] = "healthy";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-totaltimesecscaled
 * @recoil-artifact defines .data recoil:data:0x4f3760: g_Player_TotalTimeSecScaled.
 * Purpose: Stores the accumulated player-frame time used by gameplay timers.
 */
float g_Player_TotalTimeSecScaled = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-deltatime
 * @recoil-artifact defines .data recoil:data:0x4f3ac4: g_Player_DeltaTime.
 * BN types this as a zero-filled player timing float consumed by force-feedback
 * pitch filtering.
 * Purpose: Stores the current player-frame delta time used by input effects.
 */
float g_Player_DeltaTime = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-invdeltatime
 * @recoil-artifact defines .data recoil:data:0x4f3aac: g_Player_InvDeltaTime.
 * BN types this as a zero-filled player timing reciprocal float.
 * Purpose: Stores the inverse player-frame delta time shared with zInput code.
 */
float g_Player_InvDeltaTime = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-deltatimescaled001
 * @recoil-artifact defines .data recoil:data:0x4f3abc: g_Player_DeltaTimeScaled001.
 * BN types this as a zero-filled player timing float adjacent to the delta-time
 * globals.
 * Purpose: Stores the 0.01-scaled player-frame delta time shared with zInput.
 */
float g_Player_DeltaTimeScaled001 = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerpendingcheckpointnumber
 * @recoil-artifact defines .data recoil:data:0x4f3a98: g_PlayerPendingCheckpointNumber.
 * Purpose: stores the plan-tracked g_PlayerPendingCheckpointNumber gameplay data symbol.
 */
int g_PlayerPendingCheckpointNumber = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-checkpoint-nodenamefmt
 * @recoil-artifact defines .data recoil:data:0x4dc4e0: g_Checkpoint_NodeNameFmt.
 * BN types this as a writable 13-byte .data string referenced only by
 * Checkpoint::InstantiateNamedObjects.
 * Purpose: Formats checkpoint node names as checkpoint1..checkpointN.
 */
char g_Checkpoint_NodeNameFmt[13] = "checkpoint%d";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerstatusmeterratio
 * @recoil-artifact defines .data recoil:data:0x4f3754: g_PlayerStatusMeterRatio.
 * Purpose: Stores g PlayerStatusMeterRatio data used by battlesport_gameplay.player_damage_runtime_globals.
 */
float g_PlayerStatusMeterRatio = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nominalgravity
 * @recoil-artifact defines .data recoil:data:0x4f3ac8: g_Player_NominalGravity.
 * Purpose: Stores g Player NominalGravity data used by battlesport_gameplay.player_nominal_gravity_global.
 */
float g_Player_NominalGravity = 0.0f;
/**
 * Storage group: Player ZRD runtime tuning globals.
 * BN types these as independent zero-filled .data floats written by
 * Player::InitMissionRuntimeFromWorldAndCamera from player.zrd gravity,
 * sink-rate, and slope nodes, with defaults derived there when nodes are
 * absent.
 * Purpose: Stores terrain and gravity tuning loaded from player.zrd.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-watergravity
 * @recoil-artifact defines .data recoil:data:0x4f3ab8: g_Player_WaterGravity.
 * Purpose: stores the plan-tracked g_Player_WaterGravity gameplay data symbol.
 */
float g_Player_WaterGravity = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-quicksandgravity
 * @recoil-artifact defines .data recoil:data:0x4f3ac0: g_Player_QuicksandGravity.
 * Purpose: stores the plan-tracked g_Player_QuicksandGravity gameplay data symbol.
 */
float g_Player_QuicksandGravity = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-quicksandsinkrate
 * @recoil-artifact defines .data recoil:data:0x4f376c: g_Player_QuicksandSinkRate.
 * Purpose: stores the plan-tracked g_Player_QuicksandSinkRate gameplay data symbol.
 */
float g_Player_QuicksandSinkRate = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-lavasinkrate
 * @recoil-artifact defines .data recoil:data:0x4f3698: g_Player_LavaSinkRate.
 * Purpose: stores the plan-tracked g_Player_LavaSinkRate gameplay data symbol.
 */
float g_Player_LavaSinkRate = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-maxslope
 * @recoil-artifact defines .data recoil:data:0x4f3338: g_Player_MaxSlope.
 * Purpose: stores the plan-tracked g_Player_MaxSlope gameplay data symbol.
 */
float g_Player_MaxSlope = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-collisioncontactresolvescale
 * @recoil-artifact defines .data recoil:data:0x4dc96c: g_Player_CollisionContactResolveScale.
 * Purpose: stores the plan-tracked g_Player_CollisionContactResolveScale gameplay data symbol.
 */
float g_Player_CollisionContactResolveScale = 0.2f;
/**
 * Data owner 0x4f3778: zero-initialized underwater pass-3 HUD overlay singleton, constructed
 * by 0x41eb00 and reset by the atexit callback at 0x41eb20.
 * Purpose: stores the plan-tracked g_Player_UnderwaterFxPass3Ui gameplay data symbol.
 */
#undef g_Player_UnderwaterFxPass3Ui
Player_UnderwaterFxPass3UiStorage g_Player_UnderwaterFxPass3Ui = {0};
/**
 * Data owner 0x4f3650..0x4f3687: zero-initialized projectile-camera pass-3 HUD overlay
 * singleton, constructed by 0x41eb60 and reset by the atexit callback at 0x41eb80.
 * Purpose: stores the plan-tracked g_Player_State7FxPass3Ui gameplay data symbol.
 */
#undef g_Player_State7FxPass3Ui
Player_ProjectileCameraFxPass3UiStorage g_Player_State7FxPass3Ui = {0};
/**
 * Storage group: Player ZRD runtime tuning option pointers.
 * BN types these as zero-filled .data OptCatalogEntryDef pointers resolved
 * from player.zrd `make_hot` and `make_cold` option names during
 * Player::InitMissionRuntimeFromWorldAndCamera.
 * Purpose: Caches heat/cold gameplay option catalog entries for player damage paths.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-makehotoptentry
 * @recoil-artifact defines .data recoil:data:0x4f3734: g_Player_MakeHotOptEntry.
 * Purpose: stores the plan-tracked g_Player_MakeHotOptEntry gameplay data symbol.
 */
OptCatalogEntryDef *g_Player_MakeHotOptEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-makecoldoptentry
 * @recoil-artifact defines .data recoil:data:0x4f3738: g_Player_MakeColdOptEntry.
 * Purpose: stores the plan-tracked g_Player_MakeColdOptEntry gameplay data symbol.
 */
OptCatalogEntryDef *g_Player_MakeColdOptEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-bftsplashanimentry
 * @recoil-artifact defines .data recoil:data:0x4f3740: g_Player_BftSplashAnimEntry.
 * BN types this as a zero-filled .data zEffectAnimEntry pointer cached from
 * the "bftsplash" animation during mission-runtime bootstrap.
 * Purpose: Caches the battle-force splash animation entry for gameplay FX.
 */
zEffectAnimEntry *g_Player_BftSplashAnimEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-activedebugscriptasyncentry
 * @recoil-artifact defines .data recoil:data:0x4f3a90: g_Player_ActiveDebugScriptAsyncEntry.
 * Purpose: stores the plan-tracked g_Player_ActiveDebugScriptAsyncEntry gameplay data symbol.
 */
zEffectAnimEntry *g_Player_ActiveDebugScriptAsyncEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-horizonnodefollowcameraenabled
 * @recoil-artifact defines .data recoil:data:0x4f3768: g_Player_HorizonNodeFollowCameraEnabled.
 * Purpose: Stores g Player HorizonNodeFollowCameraEnabled data used by battlesport_gameplay.player_horizon_follow_globals.
 */
int g_Player_HorizonNodeFollowCameraEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-horizonnode
 * @recoil-artifact defines .data recoil:data:0x4f36c0: g_Player_HorizonNode.
 * Purpose: Stores g Player HorizonNode data used by battlesport_gameplay.player_horizon_follow_globals.
 */
zClass_NodePartial *g_Player_HorizonNode = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerprevcamerastate
 * @recoil-artifact defines .data recoil:data:0x4f36d0: g_PlayerPrevCameraState.
 * Purpose: Stores g PlayerPrevCameraState data used by battlesport_gameplay.player_damage_runtime_globals.
 */
int g_PlayerPrevCameraState = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerprevsteeringmode
 * @recoil-artifact defines .data recoil:data:0x4f36d4: g_PlayerPrevSteeringMode.
 * Purpose: Stores g PlayerPrevSteeringMode data used by battlesport_gameplay.player_damage_runtime_globals.
 */
int g_PlayerPrevSteeringMode = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-savedsteeringmode
 * @recoil-artifact defines .data recoil:data:0x4e5cc4: g_Player_SavedSteeringMode.
 * Purpose: stores the plan-tracked g_Player_SavedSteeringMode gameplay data symbol.
 */
int g_Player_SavedSteeringMode = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-copterhealthynode1
 * @recoil-artifact defines .data recoil:data:0x4f3bbc: g_Player_CopterHealthyNode1.
 * Data owner 0x4f36c4/0x4f36c8/0x4f36cc and 0x4f3bbc/0x4f3bc0: zero-initialized copter
 * sound-node cache used by the player.cpp copter sound helpers. Mission init seeds the
 * sample/cache, 0x42b630 lazily binds the copter nodes, and 0x42b5a0 reactivates sound nodes
 * while healthy.
 * Purpose: stores the plan-tracked g_Player_CopterHealthyNode1 gameplay data symbol.
 */
zClass_NodePartial *g_Player_CopterHealthyNode1 = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-copterhealthynode2
 * @recoil-artifact defines .data recoil:data:0x4f3bc0: g_Player_CopterHealthyNode2.
 * Purpose: stores the plan-tracked g_Player_CopterHealthyNode2 gameplay data symbol.
 */
zClass_NodePartial *g_Player_CopterHealthyNode2 = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptersndnode1
 * @recoil-artifact defines .data recoil:data:0x4f36c4: g_Player_CopterSndNode1.
 * Purpose: stores the plan-tracked g_Player_CopterSndNode1 gameplay data symbol.
 */
zClass_NodePartial *g_Player_CopterSndNode1 = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptersndnode2
 * @recoil-artifact defines .data recoil:data:0x4f36c8: g_Player_CopterSndNode2.
 * Purpose: stores the plan-tracked g_Player_CopterSndNode2 gameplay data symbol.
 */
zClass_NodePartial *g_Player_CopterSndNode2 = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptersndsample
 * @recoil-artifact defines .data recoil:data:0x4f36cc: g_Player_CopterSndSample.
 * Purpose: stores the plan-tracked g_Player_CopterSndSample gameplay data symbol.
 */
zSndSample *g_Player_CopterSndSample = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerenvprobesamplecount
 * @recoil-artifact defines .data recoil:data:0x4f3bc8: g_PlayerEnvProbeSampleCount.
 * Data owner 0x4f3bc8..0x4f3c8f: zero-initialized Player post-move environment probe globals.
 * BN exposes seven live world-point samples; the remaining zero bytes in this owner are
 * bounded padding.
 * Purpose: stores the plan-tracked g_PlayerEnvProbeSampleCount gameplay data symbol.
 */
int g_PlayerEnvProbeSampleCount = 0;
unsigned char g_PlayerEnvProbeSampleCountPadding[4] = {0};
int g_PlayerEnvProbe_AboveGroundFlags[10] = {0};
int g_PlayerEnvProbe_AboveGroundIndices[10] = {0};
zVec3 g_PlayerEnvProbeWorldPoints[7] = {0};
unsigned char g_PlayerEnvProbeWorldPointsTailPadding[24] = {0};
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerenvprobe-abovegroundcount
 * @recoil-artifact defines .data recoil:data:0x4f3c8c: g_PlayerEnvProbe_AboveGroundCount.
 * Purpose: stores the plan-tracked g_PlayerEnvProbe_AboveGroundCount gameplay data symbol.
 */
int g_PlayerEnvProbe_AboveGroundCount = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerrecenthitfxanimentry
 * @recoil-artifact defines .data recoil:data:0x4f373c: g_PlayerRecentHitFxAnimEntry.
 * Purpose: Stores g PlayerRecentHitFxAnimEntry data used by battlesport_gameplay.player_damage_runtime_globals.
 */
zEffectAnimEntry *g_PlayerRecentHitFxAnimEntry = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-localfxoffsetworldptr
 * @recoil-artifact defines .data recoil:data:0x779aa8: g_Player_LocalFxOffsetWorldPtr.
 * Purpose: stores the plan-tracked g_Player_LocalFxOffsetWorldPtr gameplay data symbol.
 */
zVec3 *g_Player_LocalFxOffsetWorldPtr = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playersavestatelistauxptr
 * @recoil-artifact defines .data recoil:data:0x4dc264: g_PlayerSaveStateListAuxPtr.
 * BN types this as initialized .data pointing at 0x4f3a78
 * g_PlayerSaveStateListAux, with no code xrefs.
 * Purpose: Preserves the retail initialized pointer to the save-state list aux field.
 */
int *g_PlayerSaveStateListAuxPtr = &g_PlayerSaveStateListAux;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-missioninitfirstrunflag
 * @recoil-artifact defines .data recoil:data:0x4dc268: g_Player_MissionInitFirstRunFlag.
 * BN types this as an initialized .data int with value 1, cleared after the
 * first mission-runtime HUD top-message panel registration.
 * Purpose: Ensures one-time attachment of player top-message HUD panels.
 */
int g_Player_MissionInitFirstRunFlag = 1;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-sourcefile-playercpp
 * @recoil-artifact defines .data recoil:data:0x4dc26c: g_Player_SourceFile_PlayerCpp.
 * BN types this as a writable player.cpp diagnostic source-file literal
 * referenced by ApplyMissionSaveData and ZAR_ReadVehicleListSection.
 * Purpose: Stores the Player source-file path used by save/ZAR diagnostics.
 */
char g_Player_SourceFile_PlayerCpp[31] = "D:\\Proj\\Battlesport\\player.cpp";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-savedatamodifiedmsg
 * @recoil-artifact defines .data recoil:data:0x4dc28c: g_Player_SaveDataModifiedMsg.
 * BN types this as a writable diagnostic literal referenced by
 * ApplyMissionSaveData when a Player save payload has an unexpected size.
 * Purpose: Reports incompatible Player mission-save data.
 */
char g_Player_SaveDataModifiedMsg[72] =
    "Player save data structure has been modified. Cannot use this save set.";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-savevehiclelistsectionname
 * @recoil-artifact defines .data recoil:data:0x4dc2d4: g_Player_SaveVehicleListSectionName.
 * BN types this as a writable ZAR section-name literal referenced by
 * ZAR_RegisterSections for the VehicleList callbacks.
 * Purpose: Names the Player VehicleList ZAR section.
 */
char g_Player_SaveVehicleListSectionName[12] = "VehicleList";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-vehiclesavedatamodifiedmsg
 * @recoil-artifact defines .data recoil:data:0x4dc2e0: g_Player_VehicleSaveDataModifiedMsg.
 * BN types this as a writable diagnostic literal referenced by
 * ZAR_ReadVehicleListSection when a VehicleList payload has an unexpected size.
 * Purpose: Reports incompatible VehicleList save data.
 */
char g_Player_VehicleSaveDataModifiedMsg[73] =
    "Vehicle save data structure has been modified. Cannot use this save set.";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-aivarchivemissingmsg
 * @recoil-artifact defines .data recoil:data:0x4dc368: g_Player_AivArchiveMissingMsg.
 * BN types this as a writable diagnostic literal referenced by
 * Player::InitMissionRuntimeFromWorldAndCamera when aiv.zrd is missing.
 * Purpose: Reports that the mission AIV archive could not be loaded.
 */
char g_Player_AivArchiveMissingMsg[0x15] = "Cannot find aiv.zrd!";
/**
 * Storage group: Player mission/player.zrd writable literals.
 * BN types these as writable .data char arrays used by mission runtime
 * bootstrap, player.zrd tuning, vehicle/common/modal loaders, and copter
 * sound-node caching.
 * Purpose: Stores Player mission runtime and player.zrd literal names.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-confignode-basic
 * @recoil-artifact defines .data recoil:data:0x4dc380: g_Player_ConfigNode_Basic.
 * Purpose: stores the plan-tracked g_Player_ConfigNode_Basic gameplay data symbol.
 */
char g_Player_ConfigNode_Basic[6] = "basic";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-confignode-commonmode
 * @recoil-artifact defines .data recoil:data:0x4dc388: g_Player_ConfigNode_CommonMode.
 * Purpose: stores the plan-tracked g_Player_ConfigNode_CommonMode gameplay data symbol.
 */
char g_Player_ConfigNode_CommonMode[12] = "common_mode";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-confignode-stealth
 * @recoil-artifact defines .data recoil:data:0x4dc394: g_Player_ConfigNode_Stealth.
 * Purpose: stores the plan-tracked g_Player_ConfigNode_Stealth gameplay data symbol.
 */
char g_Player_ConfigNode_Stealth[8] = "stealth";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-displayname-stealth
 * @recoil-artifact defines .data recoil:data:0x4dc39c: g_Player_DisplayName_Stealth.
 * Purpose: stores the plan-tracked g_Player_DisplayName_Stealth gameplay data symbol.
 */
char g_Player_DisplayName_Stealth[8] = "Stealth";
char g_Player_CopterSndName[11] = {
    's', 'n', 'd', '_', 'c', 'h', 'o', 'p', 'p', 'e', 'r'
};
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-lowshieldsndname
 * @recoil-artifact defines .data recoil:data:0x4dc3b0: g_Player_LowShieldSndName.
 * Purpose: stores the plan-tracked g_Player_LowShieldSndName gameplay data symbol.
 */
char g_Player_LowShieldSndName[15] = "low_shield_snd";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-burninganimname
 * @recoil-artifact defines .data recoil:data:0x4dc3c0: g_Player_BurningAnimName.
 * Purpose: stores the plan-tracked g_Player_BurningAnimName gameplay data symbol.
 */
char g_Player_BurningAnimName[13] = "burning_anim";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-makecold
 * @recoil-artifact defines .data recoil:data:0x4dc3d0: g_Player_ConfigKey_MakeCold.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MakeCold gameplay data symbol.
 */
char g_Player_ConfigKey_MakeCold[10] = "make_cold";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-makehot
 * @recoil-artifact defines .data recoil:data:0x4dc3dc: g_Player_ConfigKey_MakeHot.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MakeHot gameplay data symbol.
 */
char g_Player_ConfigKey_MakeHot[9] = "make_hot";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-maxslope
 * @recoil-artifact defines .data recoil:data:0x4dc3e8: g_Player_ConfigKey_MaxSlope.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MaxSlope gameplay data symbol.
 */
char g_Player_ConfigKey_MaxSlope[10] = "max_slope";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-lavasink
 * @recoil-artifact defines .data recoil:data:0x4dc3f4: g_Player_ConfigKey_LavaSink.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_LavaSink gameplay data symbol.
 */
char g_Player_ConfigKey_LavaSink[10] = "lava_sink";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-quicksandsink
 * @recoil-artifact defines .data recoil:data:0x4dc400: g_Player_ConfigKey_QuicksandSink.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_QuicksandSink gameplay data symbol.
 */
char g_Player_ConfigKey_QuicksandSink[11] = "qsand_sink";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-quicksandgravity
 * @recoil-artifact defines .data recoil:data:0x4dc40c: g_Player_ConfigKey_QuicksandGravity.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_QuicksandGravity gameplay data symbol.
 */
char g_Player_ConfigKey_QuicksandGravity[12] = "qsd_gravity";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-watergravity
 * @recoil-artifact defines .data recoil:data:0x4dc418: g_Player_ConfigKey_WaterGravity.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_WaterGravity gameplay data symbol.
 */
char g_Player_ConfigKey_WaterGravity[12] = "wat_gravity";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-normalgravity
 * @recoil-artifact defines .data recoil:data:0x4dc424: g_Player_ConfigKey_NormalGravity.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_NormalGravity gameplay data symbol.
 */
char g_Player_ConfigKey_NormalGravity[12] = "nom_gravity";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-maxcamtetherangle
 * @recoil-artifact defines .data recoil:data:0x4dc430: g_Player_ConfigKey_MaxCamTetherAngle.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MaxCamTetherAngle gameplay data symbol.
 */
char g_Player_ConfigKey_MaxCamTetherAngle[21] = "max_cam_tether_angle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-cameraelastic
 * @recoil-artifact defines .data recoil:data:0x4dc448: g_Player_ConfigKey_CameraElastic.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_CameraElastic gameplay data symbol.
 */
char g_Player_ConfigKey_CameraElastic[15] = "camera_elastic";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-underwatercam
 * @recoil-artifact defines .data recoil:data:0x4dc458: g_Player_ConfigKey_UnderwaterCam.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_UnderwaterCam gameplay data symbol.
 */
char g_Player_ConfigKey_UnderwaterCam[15] = "underwater_cam";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-firstpersoncamelevationlimit
 * @recoil-artifact defines .data recoil:data:0x4dc468: g_Player_ConfigKey_FirstPersonCamElevationLimit.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_FirstPersonCamElevationLimit gameplay data symbol.
 */
char g_Player_ConfigKey_FirstPersonCamElevationLimit[14] = "fp_cam_el_lim";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-firstpersoncamelevationrate
 * @recoil-artifact defines .data recoil:data:0x4dc478: g_Player_ConfigKey_FirstPersonCamElevationRate.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_FirstPersonCamElevationRate gameplay data symbol.
 */
char g_Player_ConfigKey_FirstPersonCamElevationRate[15] = "fp_cam_el_rate";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-mousepush
 * @recoil-artifact defines .data recoil:data:0x4dc488: g_Player_ConfigKey_MousePush.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MousePush gameplay data symbol.
 */
char g_Player_ConfigKey_MousePush[11] = "mouse_push";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-maxcamyawrate
 * @recoil-artifact defines .data recoil:data:0x4dc494: g_Player_ConfigKey_MaxCamYawRate.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_MaxCamYawRate gameplay data symbol.
 */
char g_Player_ConfigKey_MaxCamYawRate[17] = "max_cam_yaw_rate";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-camerazone
 * @recoil-artifact defines .data recoil:data:0x4dc4a8: g_Player_ConfigKey_CameraZone.
 * Purpose: stores the plan-tracked g_Player_ConfigKey_CameraZone gameplay data symbol.
 */
char g_Player_ConfigKey_CameraZone[12] = "camera_zone";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configarchivename
 * @recoil-artifact defines .data recoil:data:0x4dc4b4: g_Player_ConfigArchiveName.
 * Purpose: stores the plan-tracked g_Player_ConfigArchiveName gameplay data symbol.
 */
char g_Player_ConfigArchiveName[11] = "player.zrd";
char g_Player_BftSplashAnimName[9] = {
    'b', 'f', 't', 's', 'p', 'l', 'a', 's', 'h'
};
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-horizon
 * @recoil-artifact defines .data recoil:data:0x4dc4cc: g_Player_NodeName_Horizon.
 * Purpose: stores the plan-tracked g_Player_NodeName_Horizon gameplay data symbol.
 */
char g_Player_NodeName_Horizon[8] = "horizon";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-shadow
 * @recoil-artifact defines .data recoil:data:0x4dc4f0: g_Player_NodeName_Shadow.
 * Purpose: Player init-state node name for the shadow/mode-variant node.
 */
char g_Player_NodeName_Shadow[7] = "shadow";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-doorright
 * @recoil-artifact defines .data recoil:data:0x4dc4f8: g_Player_NodeName_DoorRight.
 * Purpose: Player init-state node name for the right door node.
 */
char g_Player_NodeName_DoorRight[10] = "doorright";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-doorleft
 * @recoil-artifact defines .data recoil:data:0x4dc504: g_Player_NodeName_DoorLeft.
 * Purpose: Player init-state node name for the left door node.
 */
char g_Player_NodeName_DoorLeft[9] = "doorleft";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-turret
 * @recoil-artifact defines .data recoil:data:0x4dc510: g_Player_NodeName_Turret.
 * Purpose: Shared Player/GameNet node name for the turret node.
 */
char g_Player_NodeName_Turret[7] = "turret";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-body
 * @recoil-artifact defines .data recoil:data:0x4dc518: g_Player_NodeName_Body.
 * Purpose: Player init-state node name for the body node.
 */
char g_Player_NodeName_Body[5] = "body";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-target
 * @recoil-artifact defines .data recoil:data:0x4dc520: g_Player_NodeName_Target.
 * Purpose: Player init-state node name for the target node.
 */
char g_Player_NodeName_Target[7] = "target";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-bft
 * @recoil-artifact defines .data recoil:data:0x4dc528: g_Player_NodeName_Bft.
 * Purpose: Shared Player/GameNet node name for BFT state lookup.
 */
char g_Player_NodeName_Bft[4] = "bft";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-bft00
 * @recoil-artifact defines .data recoil:data:0x4dc52c: g_Player_NodeName_Bft00.
 * Purpose: Player bootstrap node name for the initial BFT actor.
 */
char g_Player_NodeName_Bft00[7] = "bft_00";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-subt
 * @recoil-artifact defines .data recoil:data:0x4dc538: g_Player_NodeName_Subt.
 * Purpose: Player init-state animation/effect name for the subt node.
 */
char g_Player_NodeName_Subt[5] = "subt";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-bftbubbleeffectname
 * @recoil-artifact defines .data recoil:data:0x4dc540: g_Player_BftBubbleEffectName.
 * Purpose: Player init-state effect name for the BFT bubble animation.
 */
char g_Player_BftBubbleEffectName[12] = "bft_bubble1";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-napalmvehicleeffectname
 * @recoil-artifact defines .data recoil:data:0x4dc54c: g_Player_NapalmVehicleEffectName.
 * Purpose: Shared Player/zTurret effect name for napalm vehicle animation.
 */
char g_Player_NapalmVehicleEffectName[15] = "napalm_vehicle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-shockvehicleeffectname
 * @recoil-artifact defines .data recoil:data:0x4dc55c: g_Player_ShockVehicleEffectName.
 * Purpose: Player init-state effect name for shock vehicle animation.
 */
char g_Player_ShockVehicleEffectName[14] = "shock_vehicle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-shattervehicleeffectname
 * @recoil-artifact defines .data recoil:data:0x4dc56c: g_Player_ShatterVehicleEffectName.
 * Purpose: Player init-state effect name for shatter vehicle animation.
 */
char g_Player_ShatterVehicleEffectName[16] = "shatter_vehicle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-bftexhausttrailname
 * @recoil-artifact defines .data recoil:data:0x4dc57c: g_Player_BftExhaustTrailName.
 * Purpose: Player init-state trail name for BFT exhaust.
 */
char g_Player_BftExhaustTrailName[18] = "bft_exhaust_trail";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-boatwaketrailname
 * @recoil-artifact defines .data recoil:data:0x4dc590: g_Player_BoatWakeTrailName.
 * Purpose: Player init-state trail name for boat wake.
 */
char g_Player_BoatWakeTrailName[16] = "boat_wake_trail";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-regenskinnodename
 * @recoil-artifact defines .data recoil:data:0x4dc5a0: g_Player_RegenSkinNodeName.
 * Purpose: Player init-state effect name for the regen skin node.
 */
char g_Player_RegenSkinNodeName[11] = "regen_skin";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastercommondatamissingfmt
 * @recoil-artifact defines .data recoil:data:0x4dc5ac: g_Player_MasterCommonDataMissingFmt.
 * Purpose: Diagnostic emitted when Player master common data is missing.
 */
char g_Player_MasterCommonDataMissingFmt[39] =
    "Cannot find Master Common Data for %s!";
/**
 * Storage group: Player modal-bind writable literals.
 * BN types these as writable .data char arrays used by modal-state node
 * binding and model-derived support/collision point construction; the
 * intervening Bft99 and shared path-join literals belong to separate owners.
 * Purpose: Stores Player modal binding and modal point-builder literal names.
 */
char g_Player_CollisionPointsMissingFmt[37] =
    "Cannot find collision points for %s!";
char g_Player_SupportPointsMissingFmt[35] =
    "Cannot find support points for %s!";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-dustright
 * @recoil-artifact defines .data recoil:data:0x4dc620: g_Player_EffectNodeName_DustRight.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_DustRight gameplay data symbol.
 */
char g_Player_EffectNodeName_DustRight[7] = "dust_r";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-dustleft
 * @recoil-artifact defines .data recoil:data:0x4dc628: g_Player_EffectNodeName_DustLeft.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_DustLeft gameplay data symbol.
 */
char g_Player_EffectNodeName_DustLeft[7] = "dust_l";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-splashright
 * @recoil-artifact defines .data recoil:data:0x4dc630: g_Player_EffectNodeName_SplashRight.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_SplashRight gameplay data symbol.
 */
char g_Player_EffectNodeName_SplashRight[9] = "splash_r";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-splashleft
 * @recoil-artifact defines .data recoil:data:0x4dc63c: g_Player_EffectNodeName_SplashLeft.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_SplashLeft gameplay data symbol.
 */
char g_Player_EffectNodeName_SplashLeft[9] = "splash_l";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-wake
 * @recoil-artifact defines .data recoil:data:0x4dc648: g_Player_EffectNodeName_Wake.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_Wake gameplay data symbol.
 */
char g_Player_EffectNodeName_Wake[5] = "wake";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-effectnodename-caustic1
 * @recoil-artifact defines .data recoil:data:0x4dc650: g_Player_EffectNodeName_Caustic1.
 * Purpose: stores the plan-tracked g_Player_EffectNodeName_Caustic1 gameplay data symbol.
 */
char g_Player_EffectNodeName_Caustic1[9] = "caustic1";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-props
 * @recoil-artifact defines .data recoil:data:0x4dc65c: g_Player_NodeName_Props.
 * Purpose: stores the plan-tracked g_Player_NodeName_Props gameplay data symbol.
 */
char g_Player_NodeName_Props[6] = "props";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-lefttracks
 * @recoil-artifact defines .data recoil:data:0x4dc664: g_Player_NodeName_LeftTracks.
 * Purpose: stores the plan-tracked g_Player_NodeName_LeftTracks gameplay data symbol.
 */
char g_Player_NodeName_LeftTracks[8] = "ltracks";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-righttracks
 * @recoil-artifact defines .data recoil:data:0x4dc66c: g_Player_NodeName_RightTracks.
 * Purpose: stores the plan-tracked g_Player_NodeName_RightTracks gameplay data symbol.
 */
char g_Player_NodeName_RightTracks[8] = "rtracks";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-chassis
 * @recoil-artifact defines .data recoil:data:0x4dc674: g_Player_NodeName_Chassis.
 * Purpose: stores the plan-tracked g_Player_NodeName_Chassis gameplay data symbol.
 */
char g_Player_NodeName_Chassis[8] = "chassis";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-leftmorphs
 * @recoil-artifact defines .data recoil:data:0x4dc67c: g_Player_NodeName_LeftMorphs.
 * Purpose: stores the plan-tracked g_Player_NodeName_LeftMorphs gameplay data symbol.
 */
char g_Player_NodeName_LeftMorphs[12] = "left_morphs";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-rightmorphs
 * @recoil-artifact defines .data recoil:data:0x4dc688: g_Player_NodeName_RightMorphs.
 * Purpose: stores the plan-tracked g_Player_NodeName_RightMorphs gameplay data symbol.
 */
char g_Player_NodeName_RightMorphs[13] = "right_morphs";
char g_Player_MasterModalDataMissingFmt[38] =
    "Cannot find Master Modal Data for %s!";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-collisionpointnodenamefmt
 * @recoil-artifact defines .data recoil:data:0x4dc6d0: g_Player_CollisionPointNodeNameFmt.
 * Purpose: stores the plan-tracked g_Player_CollisionPointNodeNameFmt gameplay data symbol.
 */
char g_Player_CollisionPointNodeNameFmt[12] = "collide%02d";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-supportpointnodenamefmt
 * @recoil-artifact defines .data recoil:data:0x4dc6dc: g_Player_SupportPointNodeNameFmt.
 * Purpose: stores the plan-tracked g_Player_SupportPointNodeNameFmt gameplay data symbol.
 */
char g_Player_SupportPointNodeNameFmt[12] = "support%02d";
/**
 * Storage group: Player master ZRD record-loader writable literals.
 * BN types these as writable .data char arrays used by
 * Player::LoadMasterCommonDataFromNode and
 * Player::LoadMasterModalDataFromNode for common-mode, modal, sound, FX,
 * wave, movement, collision, platform, and master-type ZRD record lookups.
 * Purpose: Stores Player master ZRD record-loader literal names.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-weapons
 * @recoil-artifact defines .data recoil:data:0x4dc6e8: g_Player_NodeName_Weapons.
 * Purpose: Names the common-mode weapons record.
 */
char g_Player_NodeName_Weapons[8] = "weapons";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-pickups
 * @recoil-artifact defines .data recoil:data:0x4dc6f0: g_Player_NodeName_Pickups.
 * Purpose: Names the common-mode pickups record.
 */
char g_Player_NodeName_Pickups[8] = "pickups";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-health
 * @recoil-artifact defines .data recoil:data:0x4dc6f8: g_Player_NodeName_Health.
 * Purpose: Names the common-mode health record.
 */
char g_Player_NodeName_Health[7] = "health";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-trackswitch
 * @recoil-artifact defines .data recoil:data:0x4dc700: g_Player_NodeName_TrackSwitch.
 * Purpose: Names the common-mode track-switch record.
 */
char g_Player_NodeName_TrackSwitch[13] = "track_switch";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-cameraudswing
 * @recoil-artifact defines .data recoil:data:0x4dc710: g_Player_NodeName_CameraUdSwing.
 * Purpose: Names the common-mode camera swing record.
 */
char g_Player_NodeName_CameraUdSwing[16] = "camera_ud_swing";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-aimy
 * @recoil-artifact defines .data recoil:data:0x4dc720: g_Player_NodeName_AimY.
 * Purpose: Names the common-mode aim-yaw record.
 */
char g_Player_NodeName_AimY[5] = "aimy";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-camback
 * @recoil-artifact defines .data recoil:data:0x4dc728: g_Player_NodeName_CamBack.
 * Purpose: Names the common-mode camera-back record.
 */
char g_Player_NodeName_CamBack[8] = "camback";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-startanims
 * @recoil-artifact defines .data recoil:data:0x4dc730: g_Player_NodeName_StartAnims.
 * Purpose: Names the common-mode start-animations record.
 */
char g_Player_NodeName_StartAnims[12] = "start_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-activation
 * @recoil-artifact defines .data recoil:data:0x4dc73c: g_Player_NodeName_Activation.
 * Purpose: Names the common-mode activation record.
 */
char g_Player_NodeName_Activation[11] = "activation";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-pinging
 * @recoil-artifact defines .data recoil:data:0x4dc748: g_Player_NodeName_Pinging.
 * Purpose: Names the common-mode pinging sound record.
 */
char g_Player_NodeName_Pinging[8] = "pinging";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-weaponselect
 * @recoil-artifact defines .data recoil:data:0x4dc750: g_Player_NodeName_WeaponSelect.
 * Purpose: Names the common-mode weapon-select sound record.
 */
char g_Player_NodeName_WeaponSelect[14] = "weapon_select";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-weaponup
 * @recoil-artifact defines .data recoil:data:0x4dc760: g_Player_NodeName_WeaponUp.
 * Purpose: Names the common-mode weapon-up sound record.
 */
char g_Player_NodeName_WeaponUp[10] = "weapon_up";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-sounds
 * @recoil-artifact defines .data recoil:data:0x4dc76c: g_Player_NodeName_Sounds.
 * Purpose: Names the master record sounds child.
 */
char g_Player_NodeName_Sounds[7] = "sounds";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-nanite
 * @recoil-artifact defines .data recoil:data:0x4dc774: g_Player_NodeName_Nanite.
 * Purpose: Names the common-mode nanite record.
 */
char g_Player_NodeName_Nanite[7] = "nanite";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-volumescale
 * @recoil-artifact defines .data recoil:data:0x4dc77c: g_Player_NodeName_VolumeScale.
 * Purpose: Names the modal sound volume scale record.
 */
char g_Player_NodeName_VolumeScale[13] = "volume_scale";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-pitchscale
 * @recoil-artifact defines .data recoil:data:0x4dc78c: g_Player_NodeName_PitchScale.
 * Purpose: Names the modal sound pitch scale record.
 */
char g_Player_NodeName_PitchScale[12] = "pitch_scale";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-land
 * @recoil-artifact defines .data recoil:data:0x4dc798: g_Player_NodeName_Land.
 * Purpose: Names the modal land sound record.
 */
char g_Player_NodeName_Land[5] = "land";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-collide
 * @recoil-artifact defines .data recoil:data:0x4dc7a0: g_Player_NodeName_Collide.
 * Purpose: Names the modal collide sound record.
 */
char g_Player_NodeName_Collide[8] = "collide";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-external
 * @recoil-artifact defines .data recoil:data:0x4dc7a8: g_Player_NodeName_External.
 * Purpose: Names the modal external-engine sound record.
 */
char g_Player_NodeName_External[9] = "external";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-engine
 * @recoil-artifact defines .data recoil:data:0x4dc7b4: g_Player_NodeName_Engine.
 * Purpose: Names the modal engine sound record.
 */
char g_Player_NodeName_Engine[7] = "engine";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-skid
 * @recoil-artifact defines .data recoil:data:0x4dc7bc: g_Player_NodeName_Skid.
 * Purpose: Names the modal skid sound record.
 */
char g_Player_NodeName_Skid[5] = "skid";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-idle
 * @recoil-artifact defines .data recoil:data:0x4dc7c4: g_Player_NodeName_Idle.
 * Purpose: Names the modal idle sound record.
 */
char g_Player_NodeName_Idle[5] = "idle";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-s2aanims
 * @recoil-artifact defines .data recoil:data:0x4dc7cc: g_Player_NodeName_S2AAnims.
 * Purpose: Names the sub-to-amphib FX list record.
 */
char g_Player_NodeName_S2AAnims[10] = "s2a_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-a2sanims
 * @recoil-artifact defines .data recoil:data:0x4dc7d8: g_Player_NodeName_A2SAnims.
 * Purpose: Names the amphib-to-sub FX list record.
 */
char g_Player_NodeName_A2SAnims[10] = "a2s_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-h2aanims
 * @recoil-artifact defines .data recoil:data:0x4dc7e4: g_Player_NodeName_H2AAnims.
 * Purpose: Names the hover-to-amphib FX list record.
 */
char g_Player_NodeName_H2AAnims[10] = "h2a_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-a2hanims
 * @recoil-artifact defines .data recoil:data:0x4dc7f0: g_Player_NodeName_A2HAnims.
 * Purpose: Names the amphib-to-hover FX list record.
 */
char g_Player_NodeName_A2HAnims[10] = "a2h_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-h2tanims
 * @recoil-artifact defines .data recoil:data:0x4dc7fc: g_Player_NodeName_H2TAnims.
 * Purpose: Names the hover-to-track FX list record.
 */
char g_Player_NodeName_H2TAnims[10] = "h2t_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-t2hanims
 * @recoil-artifact defines .data recoil:data:0x4dc808: g_Player_NodeName_T2HAnims.
 * Purpose: Names the track-to-hover FX list record.
 */
char g_Player_NodeName_T2HAnims[10] = "t2h_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-a2tanims
 * @recoil-artifact defines .data recoil:data:0x4dc814: g_Player_NodeName_A2TAnims.
 * Purpose: Names the amphib-to-track FX list record.
 */
char g_Player_NodeName_A2TAnims[10] = "a2t_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-t2aanims
 * @recoil-artifact defines .data recoil:data:0x4dc820: g_Player_NodeName_T2AAnims.
 * Purpose: Names the track-to-amphib FX list record.
 */
char g_Player_NodeName_T2AAnims[10] = "t2a_anims";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-collisiondamage
 * @recoil-artifact defines .data recoil:data:0x4dc82c: g_Player_NodeName_CollisionDamage.
 * Purpose: Names the modal collision damping record.
 */
char g_Player_NodeName_CollisionDamage[12] = "collision_d";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-chassisroll
 * @recoil-artifact defines .data recoil:data:0x4dc838: g_Player_NodeName_ChassisRoll.
 * Purpose: Names the modal chassis roll record.
 */
char g_Player_NodeName_ChassisRoll[10] = "chas_roll";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-chassispitch
 * @recoil-artifact defines .data recoil:data:0x4dc844: g_Player_NodeName_ChassisPitch.
 * Purpose: Names the modal chassis pitch record.
 */
char g_Player_NodeName_ChassisPitch[11] = "chas_pitch";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-chassissmooth
 * @recoil-artifact defines .data recoil:data:0x4dc850: g_Player_NodeName_ChassisSmooth.
 * Purpose: Names the modal chassis smoothing record.
 */
char g_Player_NodeName_ChassisSmooth[12] = "chas_smooth";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-nodename-modealt
 * @recoil-artifact defines .data recoil:data:0x4dc85c: g_Player_NodeName_ModeAlt.
 * Purpose: Names the modal alternate-mode transition record.
 */
char g_Player_NodeName_ModeAlt[9] = "mode_alt";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-subwave
 * @recoil-artifact defines .data recoil:data:0x4dc868: g_Player_ConfigKey_SubWave.
 * Purpose: Names the modal submarine wave record.
 */
char g_Player_ConfigKey_SubWave[9] = "sub_wave";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-hoverwave
 * @recoil-artifact defines .data recoil:data:0x4dc874: g_Player_ConfigKey_HoverWave.
 * Purpose: Names the modal hover wave record.
 */
char g_Player_ConfigKey_HoverWave[11] = "hover_wave";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-amphibwave
 * @recoil-artifact defines .data recoil:data:0x4dc880: g_Player_ConfigKey_AmphibWave.
 * Purpose: Names the modal amphib wave record.
 */
char g_Player_ConfigKey_AmphibWave[12] = "amphib_wave";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-gunpitch
 * @recoil-artifact defines .data recoil:data:0x4dc88c: g_Player_ConfigKey_GunPitch.
 * Purpose: Names the modal gun pitch record.
 */
char g_Player_ConfigKey_GunPitch[10] = "gun_pitch";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-mass
 * @recoil-artifact defines .data recoil:data:0x4dc898: g_Player_ConfigKey_Mass.
 * Purpose: Names the modal mass record.
 */
char g_Player_ConfigKey_Mass[5] = "mass";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-altcontrol
 * @recoil-artifact defines .data recoil:data:0x4dc8a0: g_Player_ConfigKey_AltControl.
 * Purpose: Names the modal alternate-control record.
 */
char g_Player_ConfigKey_AltControl[12] = "alt_control";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-acceldamping
 * @recoil-artifact defines .data recoil:data:0x4dc8ac: g_Player_ConfigKey_AccelDamping.
 * Purpose: Names the modal acceleration damping record.
 */
char g_Player_ConfigKey_AccelDamping[10] = "a_damping";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-ratedamping
 * @recoil-artifact defines .data recoil:data:0x4dc8b8: g_Player_ConfigKey_RateDamping.
 * Purpose: Names the modal rate damping record.
 */
char g_Player_ConfigKey_RateDamping[13] = "rate_damping";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-turndamping
 * @recoil-artifact defines .data recoil:data:0x4dc8c8: g_Player_ConfigKey_TurnDamping.
 * Purpose: Names the modal turn damping record.
 */
char g_Player_ConfigKey_TurnDamping[13] = "turn_damping";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-turns
 * @recoil-artifact defines .data recoil:data:0x4dc8d8: g_Player_ConfigKey_Turns.
 * Purpose: Names the modal turn-rate record.
 */
char g_Player_ConfigKey_Turns[6] = "turns";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-lavaslowdown
 * @recoil-artifact defines .data recoil:data:0x4dc8e0: g_Player_ConfigKey_LavaSlowdown.
 * Purpose: Names the modal lava slowdown record.
 */
char g_Player_ConfigKey_LavaSlowdown[14] = "lava_slowdown";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-quicksandslowdown
 * @recoil-artifact defines .data recoil:data:0x4dc8f0: g_Player_ConfigKey_QuicksandSlowdown.
 * Purpose: Names the modal quicksand slowdown record.
 */
char g_Player_ConfigKey_QuicksandSlowdown[19] = "quicksand_slowdown";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-stopping
 * @recoil-artifact defines .data recoil:data:0x4dc904: g_Player_ConfigKey_Stopping.
 * Purpose: Names the modal stopping-force record.
 */
char g_Player_ConfigKey_Stopping[9] = "stopping";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-friction
 * @recoil-artifact defines .data recoil:data:0x4dc910: g_Player_ConfigKey_Friction.
 * Purpose: Names the modal friction record.
 */
char g_Player_ConfigKey_Friction[9] = "friction";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-rates
 * @recoil-artifact defines .data recoil:data:0x4dc91c: g_Player_ConfigKey_Rates.
 * Purpose: Names the modal acceleration-rate record.
 */
char g_Player_ConfigKey_Rates[6] = "rates";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-collision
 * @recoil-artifact defines .data recoil:data:0x4dc924: g_Player_ConfigKey_Collision.
 * Purpose: Names the modal collision probe list record.
 */
char g_Player_ConfigKey_Collision[10] = "collision";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configkey-platform
 * @recoil-artifact defines .data recoil:data:0x4dc930: g_Player_ConfigKey_Platform.
 * Purpose: Names the modal platform probe list record.
 */
char g_Player_ConfigKey_Platform[9] = "platform";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypeunknown
 * @recoil-artifact defines .data recoil:data:0x4dc93c: g_Player_ConfigValue_MasterTypeUnknown.
 * Purpose: Stores the fallback modal master type name.
 */
char g_Player_ConfigValue_MasterTypeUnknown[8] = "unknown";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypefly
 * @recoil-artifact defines .data recoil:data:0x4dc944: g_Player_ConfigValue_MasterTypeFly.
 * Purpose: Stores the fly modal master type name.
 */
char g_Player_ConfigValue_MasterTypeFly[4] = "fly";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypesub
 * @recoil-artifact defines .data recoil:data:0x4dc948: g_Player_ConfigValue_MasterTypeSub.
 * Purpose: Stores the sub modal master type name.
 */
char g_Player_ConfigValue_MasterTypeSub[4] = "sub";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypeamphib
 * @recoil-artifact defines .data recoil:data:0x4dc94c: g_Player_ConfigValue_MasterTypeAmphib.
 * Purpose: Stores the amphib modal master type name.
 */
char g_Player_ConfigValue_MasterTypeAmphib[7] = "amphib";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypehover
 * @recoil-artifact defines .data recoil:data:0x4dc954: g_Player_ConfigValue_MasterTypeHover.
 * Purpose: Stores the hover modal master type name.
 */
char g_Player_ConfigValue_MasterTypeHover[6] = "hover";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-configvalue-mastertypetrack
 * @recoil-artifact defines .data recoil:data:0x4dc95c: g_Player_ConfigValue_MasterTypeTrack.
 * Purpose: Stores the track modal master type name.
 */
char g_Player_ConfigValue_MasterTypeTrack[6] = "track";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-confignode-mode
 * @recoil-artifact defines .data recoil:data:0x4dc964: g_Player_ConfigNode_Mode.
 * Purpose: Names the modal mode record.
 */
char g_Player_ConfigNode_Mode[5] = "mode";
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-aivparentdir
 * @recoil-artifact defines .data recoil:data:0x4e5b50: g_Player_AivParentDir.
 * BN types this as a zero-filled char[0x104] buffer written by
 * zReader::BuildResolvedParentDir after aiv.zrd is loaded; final-data
 * evidence places the source symbol in player.obj BSS while the retail range
 * straddles the PE raw/zero-fill boundary.
 * Purpose: Stores the resolved parent directory for AIV-relative player data.
 */
char g_Player_AivParentDir[0x104];
/**
 * Data owner 0x4f33a8..0x4f364f and 0x4f37b0..0x4f3a57: zero-initialized top-message HUD panel
 * singletons constructed at startup and destroyed by their CRT exit callbacks.
 * Purpose: stores the plan-tracked g_Player_TopMsgPanel2 gameplay data symbol.
 */
#undef g_Player_TopMsgPanel2
PlayerTopMsgPanelStorage g_Player_TopMsgPanel2 = {0};
/**
 * Purpose: stores the plan-tracked g_Player_TopMsgPanel1 gameplay data symbol.
 */
#undef g_Player_TopMsgPanel1
PlayerTopMsgPanelStorage g_Player_TopMsgPanel1 = {0};
}
#define g_Player_UnderwaterFxPass3Ui \
    (*(Player_UnderwaterFxPass3Ui *)&g_Player_UnderwaterFxPass3Ui)
#define g_Player_State7FxPass3Ui \
    (*(Player_ProjectileCameraFxPass3Ui *)&g_Player_State7FxPass3Ui)
#define g_Player_TopMsgPanel1 \
    (*(HudUiPanel *)&g_Player_TopMsgPanel1)
#define g_Player_TopMsgPanel2 \
    (*(HudUiPanel *)&g_Player_TopMsgPanel2)

namespace {
/**
 * Original inline helper; no standalone retail function exists. Observed in address-backed caller 0x4386c0 as the two-branch 0.0f..1.0f clamp.
 * Purpose: clamp a blend value to the unit interval.
 */
float PlayerClamp01(
    float value
) {
    if (value > 1.0f) {
        return 1.0f;
    }
    if (value < 0.0f) {
        return 0.0f;
    }
    return value;
}
/**
 * Original inline helper; no standalone retail function exists. Observed in address-backed callers 0x4386c0, 0x4289f0, 0x42c0d0, 0x42c2e0, 0x427440, 0x427ec0, 0x43a600, and 0x43a900 as a VC5-era int-bits smoothing idiom.
 * Purpose: reinterpret an IEEE-754 bit pattern as float.
 */
float PlayerFloatFromBits(
    int bits
) {
    float value = 0.0f;
    memcpy(
        &value,
        &bits,
        sizeof(value)
    );
    return value;
}
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x428520 Player::UpdateMasterTypeSub, 0x426770 Player::UpdateMasterTypeTrack.
 * Purpose: provide the recovered player damping from rate helper for
 * the Player/Pickup gameplay source cluster.
 */
float PlayerDampingFromRate(
    float rate
) {
    return PlayerFloatFromBits((int)(-rate * g_Player_DeltaTime * 12102200.0f) + 0x3f800000);
}
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x428520 Player::UpdateMasterTypeSub, 0x426770 Player::UpdateMasterTypeTrack, 0x4279f0 Player::UpdateMasterTypeAmphib, 0x427140 Player::UpdateMasterTypeHover.
 * Purpose: provide the recovered player wrap signed two pi helper for
 * the Player/Pickup gameplay source cluster.
 */
float PlayerWrapSignedTwoPi(
    float angle
) {
    const float twoPi = 6.28318548f;
    if (angle < -twoPi) {
        angle += twoPi;
    } else if (angle > twoPi) {
        angle -= twoPi;
    }
    return angle;
}
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x425a20 Player::TickLocalPlayerControls, 0x428520 Player::UpdateMasterTypeSub, 0x426770 Player::UpdateMasterTypeTrack, 0x427440 Player::UpdateMasterTypeHover_FromModalProbe.
 * Purpose: provide the recovered player clamp signed helper for
 * the Player/Pickup gameplay source cluster.
 */
float PlayerClampSigned(
    float value,
    float limit
) {
    if (value > limit) {
        return limit;
    }
    if (value < -limit) {
        return -limit;
    }
    return value;
}
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x426770 Player::UpdateMasterTypeTrack, 0x427440 Player::UpdateMasterTypeHover_FromModalProbe, 0x43a600 Player::UpdateAltGunAimDirection.
 * Purpose: provide the recovered transform world vector to local helper for
 * the Player/Pickup gameplay source cluster.
 */
zVec3 TransformWorldVectorToLocal(
    const zVec3 &vec,
    const zMat4x3 &matrix
) {
    zVec3 out = {0};
    out.x = vec.x * matrix.xx + vec.y * matrix.xy + vec.z * matrix.xz;
    out.y = vec.x * matrix.yx + vec.y * matrix.yy + vec.z * matrix.yz;
    out.z = vec.x * matrix.zx + vec.y * matrix.zy + vec.z * matrix.zz;
    return out;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x428520 Player::UpdateMasterTypeSub, 0x426770 Player::UpdateMasterTypeTrack, 0x427440 Player::UpdateMasterTypeHover_FromModalProbe, 0x427140 Player::UpdateMasterTypeHover.
 * Purpose: provide the recovered transform local vector to world helper for
 * the Player/Pickup gameplay source cluster.
 */
zVec3 TransformLocalVectorToWorld(
    const zVec3 &vec,
    const zMat4x3 &matrix
) {
    zVec3 out = {0};
    out.x = vec.x * matrix.xx + vec.y * matrix.yx + vec.z * matrix.zx;
    out.y = vec.x * matrix.xy + vec.y * matrix.yy + vec.z * matrix.zy;
    out.z = vec.x * matrix.xz + vec.y * matrix.yz + vec.z * matrix.zz;
    return out;
}

enum PlayerMasterTypeId {
    kPlayerMasterTypeFly = 1,
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3,
    kPlayerMasterTypeHover = 4,
    kPlayerMasterTypeAmphib = 5
};

const float kPlayerMasterTypeTrackCooldownSec = 1.0f;
const float kPlayerMasterTypeFlyCooldownSec = 5.0f;
const int kPlayerAiMode2TopSteering = 1;
const int kPlayerAiMode2SteerDirectTarget = 0;
const int kPlayerAiMode2SteerOffsetTarget = 1;
const int kPlayerAiMode2SteerDynamicOffsetTarget = 2;
const int kPlayerAiMode2SteerPathFollow = 3;
const int kPlayerAiMode2SteerTurnInPlace = 5;
const int kPlayerAiMode2SteerAutoTurn = 6;
const float kPlayerAiAltGunAttackForwardMin = 0.75f;
const float kPlayerAiAltGunStatusMinScale = 0.5f;
const int kPlayerAiTopPathFollow = 0;
const int kPlayerAiTopTurnTowardTarget = 2;
const int kPlayerAiTopTurnOnlyTowardTarget = 3;
const int kPlayerAiTopPathSteering = 4;
const int kPlayerAiTopAutoTurn = 5;
const int kPlayerNodeFlagNetworkBftCloneSource = 1 << 22;
const int kPlayerPerFrameGeneralFlag = 2;
const float kPlayerMinFrameDeltaSec = 0.00499999989f;
const float kPlayerDeltaTimeScaled001Factor = 0.00999999978f;
const float kPlayerWorldCollisionStackDrop = 0.200000003f;
const float kPlayerWorldCollisionSubRestoreYOffset = -1.0f;
const float kPlayerWorldCollisionUpwardBounceDamping = -0.800000012f;
const float kPlayerTransferDamageScale = 5.0f;
const float kPlayerTransferVelocityDamping = 0.666700006f;
const int kPlayerNanitePanelDisabledSentinel = 123456789;
const float kPlayerAltAmmoDisabledSentinel = 123456792.0f;
const float kPlayerRecentHitAlertSec = 5.0f;
const int kPlayerMissionSaveLegacySize = 0x124;
const unsigned int kPlayerGunControllerAvailableFlag = 0x04;
const unsigned int kPlayerGunControllerDualMountFlag = 0x02;
const unsigned int kPlayerGunControllerRecoilFlag = 0x01;
const unsigned int kOptCatalogFlagLockOnTargetRef = 0x4000;
const unsigned int kPlayerOptCatalogFlagTetherGuided = 1u << 20;
const unsigned int kOptCatalogFlagReload = 1u << 18;
const unsigned int kOptCatalogFlagCreateTrail = 0x02;
const int kCheckpointNodeAuxFlagTracked = 0x02;
const int kCheckpointNodePickableFlag = 0x40000;
const int kCheckpointNodeContextFlag = 0x200000;
const unsigned int kPlayerTimedHitStatusActiveFlag = 0x01;
const unsigned int kOptCatalogFlagBypassDamageProtection = 0x200;
const unsigned int kOptCatalogFlagRecordsRecentHit = 0x1000;
const unsigned int kOptCatalogFlagAppliesTimedHitStatus = 0x200000;
const unsigned int kOptCatalogFlagBlockedInSub = 0x1000;
const unsigned int kOptCatalogFlagNoSubUse = 0x02;
const int kPlayerTickCameraStateProjectileAttached = 7;
const int kPlayerTickCameraStateRestorePrevious = 8;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-vehiclearchivename-easy
 * @recoil-artifact defines .data recoil:data:0x4dc334: g_Player_VehicleArchiveName_Easy.
 * Purpose: names the easy-difficulty vehicle archive selected for AIV loads.
 */
const char g_Player_VehicleArchiveName_Easy[] = "vehicle_easy.zrd";

/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-vehiclearchivename-hard
 * @recoil-artifact defines .data recoil:data:0x4dc348: g_Player_VehicleArchiveName_Hard.
 * Purpose: names the hard-difficulty vehicle archive selected for AIV loads.
 */
const char g_Player_VehicleArchiveName_Hard[] = "vehicle_hard.zrd";

/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-vehiclearchivename-default
 * @recoil-artifact defines .data recoil:data:0x4dc35c: g_Player_VehicleArchiveName_Default.
 * Purpose: names the fallback vehicle archive selected for AIV loads.
 */
const char g_Player_VehicleArchiveName_Default[] = "vehicle.zrd";

/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-aivzrdpath
 * @recoil-artifact defines .data recoil:data:0x4dc32c: g_Player_AivZrdPath.
 * Purpose: names the player AIV archive loaded during mission bootstrap.
 */
const char g_Player_AivZrdPath[] = "aiv.zrd";
const float kPlayerDefaultActivationRange = 100.0f;
const float kPlayerDefaultReturnRange = 250.0f;
const float kPlayerDefaultNotPursuitDwellTime = 3.0f;
const float kPlayerDefaultMaxHealth = 100.0f;
const float kPlayerDefaultAiAttackRadiusSq = 1500.0f;
const float kPlayerDefaultAiAttackDwellTime = 10.0f;
const float kPlayerAiInitialStateDelaySec = 10.0f;
const float kPlayerAiPathFollowMinThrottle = 0.25f;
const float kPlayerAiPathFollowAdvanceDistance = 10.0f;
const float kPlayerAiForwardPathAdvanceDistance = 5.0f;
const float kPlayerAiSyntheticPathRebuildDistanceSq = 400.0f;
const float kPlayerAiSyntheticPathWidth = 10.0f;
const float kPlayerAiSyntheticPathRebuildDelaySec = 1.0f;
const float kPlayerAiAttackLosTargetYOffset = 1.5f;
const float kPlayerAiDynamicOffsetBackUpDistance = 10.0f;
const float kPlayerCameraState2TargetYOffset = 150.0f;
const zVec3 kPlayerDefaultAltGunAimOrigin = {0.0f, 0.0f, -1.0f};
const double kPlayerRadiansToDegrees = 57.29577951308;

struct HitOwnerSaveStateLinkPartial {
    unsigned char unknown_00[0x04];
    zUtil_SaveGameState *ownerSaveState;
};

struct HitOwnerOrContextPartial {
    unsigned char unknown_00[0x40];
    HitOwnerSaveStateLinkPartial *ownerLink;
};

struct PlayerCollisionContactContextPartial {
    unsigned char unknown_00[0x04];
    zUtil_SaveGameState *saveState;
};
RECOIL_STATIC_ASSERT(
    offsetof(
        PlayerCollisionContactContextPartial,
        saveState
    ) == 0x04
);

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90, 0x422170, and 0x4226d0 as repeated direct
 * zReader node-array value loads. BN shows the caller bodies fold this access
 * into field reads instead of calling a helper target.
 * Purpose: return the child array backing a type-4 ZRD record node.
 */
zReader::Node *PlayerZrdArrayBase(
    zReader::Node *node
) {
    return node->value.nodes;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90, 0x422170, and 0x4226d0 as repeated reads of
 * the first child node's integer count before ZRD array loops and modal-count
 * calculations. The pattern is a source-level ZRD array accessor, not a
 * separate retail callee.
 * Purpose: return the stored element count for a ZRD array node.
 */
int PlayerZrdArrayCount(
    zReader::Node *node
) {
    return PlayerZrdArrayBase(node)[0].value.i32;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90, 0x422170, and 0x4226d0 as repeated string
 * fetches from indexed ZRD child records before strcpy, sound lookup, pickup
 * lookup, and FX lookup operations. BN shows inline child-array field reads at
 * those call sites.
 * Purpose: return a string field from an indexed ZRD array element.
 */
const char *PlayerZrdArrayString(
    zReader::Node *node,
    int index
) {
    return PlayerZrdArrayBase(node)[index].value.str;
}



/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90 and 0x422170 as repeated integer fetches from
 * indexed ZRD child records for tuning fields, counts, capacities, gates, and
 * weapon specs. BN shows the loads inlined into the caller bodies.
 * Purpose: return an integer field from an indexed ZRD array element.
 */
int PlayerZrdArrayInt(
    zReader::Node *node,
    int index
) {
    return PlayerZrdArrayBase(node)[index].value.i32;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x41fe90, 0x422170, and 0x4226d0 as repeated float
 * fetches from indexed ZRD child records for camera, common-mode, modal, wave,
 * and sound-scale data. BN shows direct float loads at those caller sites.
 * Purpose: return a float field from an indexed ZRD array element.
 */
float PlayerZrdArrayFloat(
    zReader::Node *node,
    int index
) {
    return PlayerZrdArrayBase(node)[index].value.f32;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x41fe90 while selecting each vehicle modal child node
 * from the loaded vehicle ZRD array. BN shows address arithmetic over the
 * child-node array, with no separate helper target.
 * Purpose: return the indexed child node from a ZRD array node.
 */
zReader::Node *PlayerZrdArrayNode(
    zReader::Node *node,
    int index
) {
    return &PlayerZrdArrayBase(node)[index];
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x422170 and 0x4226d0 where string fields from ZRD
 * records are copied into fixed Player master-data name buffers. BN shows
 * strlen/memcpy strcpy expansions around direct child-array string loads.
 * Purpose: copy an indexed ZRD string field into a destination buffer.
 */
void PlayerCopyZrdArrayString(
    char *dest,
    zReader::Node *node,
    int index
) {
    strcpy(
        dest,
        PlayerZrdArrayString(node, index)
    );
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in callers 0x422170 and 0x4226d0 as repeated named child lookups
 * followed by first-string extraction and zSnd::FindSampleByName. BN shows the
 * full pattern repeated for common and modal sound records.
 * Purpose: resolve one optional named ZRD sound sample into a Player data slot.
 */
void PlayerLoadSoundSample(
    zReader::Node *parentNode,
    const char *name,
    zSndSample **outSample
) {
    zReader::Node *const node = zReader_GetNamedNode(
        parentNode,
        name
    );
    if (node != 0) {
        *outSample = zSnd::FindSampleByName(PlayerZrdArrayString(
            node,
            1
        ));
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x4226d0 as the same count-and-copy loop for platform and
 * collision point lists. BN shows direct child-array vector loads and no
 * separate retail function target for the repeated loop.
 * Purpose: copy a ZRD list of xyz point records into a modal point buffer.
 */
void PlayerLoadModalPointList(
    zReader::Node *node,
    zVec3 *points,
    int *outCount
) {
    if (node == 0) {
        *outCount = 0;
        return;
    }

    const int count = PlayerZrdArrayCount(node) - 1;
    *outCount = count;
    for (int index = 0; index < count; ++index) {
        zReader::Node *const coords = PlayerZrdArrayBase(node)[index + 1].value.nodes;
        points[index].x = coords[1].value.f32;
        points[index].y = coords[2].value.f32;
        points[index].z = coords[3].value.f32;
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x4226d0 as eight repeated transition-FX list loads with
 * the same optional named-node lookup, two-entry cap, and
 * zEffectAnim::FindEntryByName dispatch. BN shows each loop body inlined.
 * Purpose: load up to two named transition FX entries from a modal ZRD list.
 */
void PlayerLoadModalFxList(
    zReader::Node *modalNode,
    const char *name,
    zEffectAnimEntry **entries
) {
    zReader::Node *const node = zReader_GetNamedNode(
        modalNode,
        name
    );
    if (node == 0) {
        return;
    }

    int count = PlayerZrdArrayCount(node) - 1;
    if (count > 2) {
        count = 2;
    }

    for (int index = 0; index < count; ++index) {
        entries[index] = zEffectAnim::FindEntryByName(PlayerZrdArrayString(
            node,
            index + 1
        ));
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x4226d0 as three repeated seven-float wave blocks for
 * amphib, hover, and sub modal records. BN shows direct field stores into the
 * same modal wave parameter slots rather than a separate helper call.
 * Purpose: overwrite modal hover wave parameters from a named ZRD record.
 */
void PlayerLoadModalWaveParams(
    PlayerMasterModalData *modalData,
    zReader::Node *modalNode,
    const char *name
) {
    zReader::Node *const node = zReader_GetNamedNode(
        modalNode,
        name
    );
    if (node == 0) {
        return;
    }

    modalData->hoverPitchWaveBaseRate = PlayerZrdArrayFloat(
        node,
        1
    );
    modalData->hoverPitchWaveSpeedRate = PlayerZrdArrayFloat(
        node,
        2
    );
    modalData->hoverPitchWaveAmplitude = PlayerZrdArrayFloat(
        node,
        3
    );
    modalData->hoverRollWaveBaseRate = PlayerZrdArrayFloat(
        node,
        4
    );
    modalData->hoverRollWaveSpeedRate = PlayerZrdArrayFloat(
        node,
        5
    );
    modalData->hoverRollWaveAmplitude = PlayerZrdArrayFloat(
        node,
        6
    );
    modalData->hoverRollYawCoupleScale = PlayerZrdArrayFloat(
        node,
        7
    );
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x41fe90 Player::InitMissionRuntimeFromWorldAndCamera, 0x42ac90 Player::TransitionToMasterTypeTrack, 0x42aeb0 Player::TransitionToMasterTypeAmphib, 0x42b0f0 Player::TransitionToMasterTypeHover.
 * Purpose: provide the recovered set hud ui element visible helper for
 * the Player/Pickup gameplay source cluster.
 */
void SetHudUiElementVisible(
    HudUiElement *element,
    int visible
) {
    element->SetVisible(visible);
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41fe90 Player::InitMissionRuntimeFromWorldAndCamera.
 * Purpose: provide the recovered set hud panel visible helper for
 * the Player/Pickup gameplay source cluster.
 */
void SetHudPanelVisible(
    HudUiPanel *panel,
    int visible
) {
    SetHudUiElementVisible(
        (HudUiElement *)panel,
        visible
    );
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41fe90 Player::InitMissionRuntimeFromWorldAndCamera.
 * Purpose: provide the recovered player init action callback node helper for
 * the Player/Pickup gameplay source cluster.
 */
void PlayerInitActionCallbackNode(
    void *callback
) {
    zClass_NodePartial *const node = zClass_Object3D::gwObject3DInit();
    zClass_Class::gwNodeSetPriority(
        node,
        2
    );
    zClass_Class::gwNodeSetActionCallback(
        node,
        callback
    );
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41fe90 Player::InitMissionRuntimeFromWorldAndCamera.
 * Purpose: provide the recovered player alloc master common data helper for
 * the Player/Pickup gameplay source cluster.
 */
PlayerMasterCommonData *PlayerAllocMasterCommonData() {
    PlayerMasterCommonData *const commonData =
        (PlayerMasterCommonData *)(::operator new(sizeof(PlayerMasterCommonData)));
    memset(
        commonData,
        0,
        sizeof(PlayerMasterCommonData)
    );
    commonData->next = 0;
    if (g_PlayerMasterCommonDataCount == 0) {
        g_PlayerMasterCommonDataHead = commonData;
    } else {
        g_PlayerMasterCommonDataTail->next = commonData;
    }
    g_PlayerMasterCommonDataTail = commonData;
    ++g_PlayerMasterCommonDataCount;
    return commonData;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41fe90 Player::InitMissionRuntimeFromWorldAndCamera.
 * Purpose: provide the recovered player alloc master modal data helper for
 * the Player/Pickup gameplay source cluster.
 */
PlayerMasterModalData *PlayerAllocMasterModalData() {
    PlayerMasterModalData *const modalData =
        (PlayerMasterModalData *)(::operator new(sizeof(PlayerMasterModalData)));
    memset(
        modalData,
        0,
        sizeof(PlayerMasterModalData)
    );
    modalData->next = 0;
    if (g_PlayerMasterModalDataCount == 0) {
        g_PlayerMasterModalDataHead = modalData;
    } else {
        g_PlayerMasterModalDataTail->next = modalData;
    }
    g_PlayerMasterModalDataTail = modalData;
    ++g_PlayerMasterModalDataCount;
    return modalData;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41fe90 Player::InitMissionRuntimeFromWorldAndCamera.
 * Purpose: provide the recovered player alloc linked save state helper for
 * the Player/Pickup gameplay source cluster.
 */
zUtil_SaveGameState *PlayerAllocLinkedSaveState() {
    zUtil_SaveGameState *saveState =
        (zUtil_SaveGameState *)(::operator new(sizeof(zUtil_SaveGameState)));
    saveState = zUtil_SaveGameStateList_Init(saveState);
    saveState->next = 0;
    if (g_PlayerSaveStateCount == 0) {
        g_PlayerSaveStateListHead = saveState;
    } else {
        g_PlayerSaveStateListTail->next = saveState;
    }
    g_PlayerSaveStateListTail = saveState;
    ++g_PlayerSaveStateCount;
    return saveState;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x41fe90 as the player.zrd tuning block that reads named
 * ZRD nodes and writes the Player mission-runtime tuning globals. The helper
 * keeps the recovered source cluster readable without introducing a retail
 * call target.
 * Purpose: load Player mission-runtime tuning globals from the player.zrd root.
 */
void PlayerLoadPlayerZrdTuning(
    zReader::Node *root
) {
    zReader::Node *node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_CameraZone
    );
    if (node != 0) {
        const float cameraZone = PlayerZrdArrayFloat(
            node,
            1
        );
        if (cameraZone > 0.0f && cameraZone < 1.0f) {
            g_Player_CameraZone = cameraZone;
            g_Player_CameraZoneInvRange = 1.0f / (1.0f - cameraZone);
        }
    }

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_MaxCamYawRate
    );
    g_Player_MaxCamYawRate = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 2.0f;

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_MousePush
    );
    if (node != 0) {
        g_Player_MousePushX = PlayerZrdArrayFloat(
            node,
            1
        );
        g_Player_MousePushY = PlayerZrdArrayFloat(
            node,
            2
        );
    } else {
        g_Player_MousePushX = 0.00200000009f;
        g_Player_MousePushY = 0.00999999978f;
    }

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_FirstPersonCamElevationRate
    );
    g_Player_FpCamElevationRate = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 5.0f;

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_FirstPersonCamElevationLimit
    );
    if (node != 0) {
        g_Player_FpCamElevationMin = PlayerZrdArrayFloat(
            node,
            1
        );
        g_Player_FpCamElevationMax = PlayerZrdArrayFloat(
            node,
            2
        );
    } else {
        g_Player_FpCamElevationMin = -0.75f;
        g_Player_FpCamElevationMax = 1.0f;
    }

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_UnderwaterCam
    );
    if (node != 0) {
        int rBits = 0;
        int gBits = 0;
        int bBits = 0;
        g_Player_UnderwaterCamDistance = PlayerZrdArrayFloat(
            node,
            1
        );
        g_Player_UnderwaterCamHeight = PlayerZrdArrayFloat(
            node,
            2
        );
        g_Player_UnderwaterCamStepCount = PlayerZrdArrayInt(
            node,
            3
        );
        g_Player_UnderwaterCamFar = PlayerZrdArrayFloat(
            node,
            4
        );
        zVideo::PixelPack_GetRgbBits(
            &rBits,
            &gBits,
            &bBits
        );
        g_Player_UnderwaterCamPackedColor =
            (PlayerZrdArrayInt(
                node,
                5
            ) >> (8 - rBits) << (bBits + gBits)) +
            (PlayerZrdArrayInt(
                node,
                6
            ) >> (8 - gBits) << bBits) +
            (PlayerZrdArrayInt(
                node,
                7
            ) >> (8 - bBits));
        g_Player_UnderwaterCamAlpha = PlayerZrdArrayFloat(
            node,
            8
        );
    } else {
        g_Player_UnderwaterCamDistance = 5.0f;
        g_Player_UnderwaterCamHeight = 4.0f;
        g_Player_UnderwaterCamStepCount = 12;
        g_Player_UnderwaterCamFar = 100.0f;
        g_Player_UnderwaterCamPackedColor = 0x1f5;
        g_Player_UnderwaterCamAlpha = 0.5f;
    }

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_CameraElastic
    );
    if (node != 0) {
        g_Player_CameraElastic = PlayerZrdArrayFloat(
            node,
            1
        );
    }

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_MaxCamTetherAngle
    );
    if (node != 0) {
        g_Player_MaxCamTetherAngleRad = PlayerZrdArrayFloat(
            node,
            1
        ) * 0.01745329251994f;
    }

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_NormalGravity
    );
    g_Player_NominalGravity = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 28.0f;

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_WaterGravity
    );
    g_Player_WaterGravity =
        node != 0 ? PlayerZrdArrayFloat(
            node,
            1
        ) : g_Player_NominalGravity * 0.333333343f;

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_QuicksandGravity
    );
    g_Player_QuicksandGravity =
        node != 0 ? PlayerZrdArrayFloat(
            node,
            1
        ) : g_Player_NominalGravity * 0.166666672f;

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_QuicksandSink
    );
    g_Player_QuicksandSinkRate = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 0.899999976f;

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_LavaSink
    );
    g_Player_LavaSinkRate = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 0.600000024f;

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_MaxSlope
    );
    g_Player_MaxSlope = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 0.707000017f;

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_MakeHot
    );
    if (node != 0) {
        g_Player_MakeHotOptEntry = OptCatalog::FindEntryByName(PlayerZrdArrayString(
            node,
            1
        ));
    }

    node = zReader_GetNamedNode(
        root,
        g_Player_ConfigKey_MakeCold
    );
    if (node != 0) {
        g_Player_MakeColdOptEntry = OptCatalog::FindEntryByName(PlayerZrdArrayString(
            node,
            1
        ));
    }

    node = zReader_GetNamedNode(
        root,
        g_Player_BurningAnimName
    );
    if (node != 0) {
        g_PlayerRecentHitFxAnimEntry = zEffectAnim::FindEntryByName(PlayerZrdArrayString(
            node,
            1
        ));
    }

    node = zReader_GetNamedNode(
        root,
        g_Player_LowShieldSndName
    );
    if (node != 0) {
        g_Hud_LowMeterBeepSample = zSnd::FindSampleByName(PlayerZrdArrayString(
            node,
            1
        ));
        g_Hud_LowMeterBeepInterval = PlayerZrdArrayFloat(
            node,
            2
        );
        g_Hud_LowMeterLoopSample = zSnd::FindSampleByName(PlayerZrdArrayString(
            node,
            3
        ));
    }

    g_PlayerStatusMeterRatio = 1.0f;
    g_Hud_LowMeterNextBeepTime = 0.0f;
    g_Player_CopterSndSample = zSnd::FindSampleByName(g_Player_CopterSndName);
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x42ac90 Player::TransitionToMasterTypeTrack, 0x42aeb0 Player::TransitionToMasterTypeAmphib, 0x42b2a0 Player::TransitionToMasterTypeSub, 0x42b0f0 Player::TransitionToMasterTypeHover.
 * Purpose: provide the recovered trigger zero velocity fx list helper for
 * the Player/Pickup gameplay source cluster.
 */
void TriggerZeroVelocityFxList(
    zEffectAnimEntry **entries,
    zClass_NodePartial *rootNode,
    int flags
) {
    for (int i = 0; i < 2; ++i) {
        zEffectAnimEntry *const entry = entries[i];
        if (entry != 0 && flags == 0) {
            zEffectAnim::SetVelocity_Thunk(
                entry,
                rootNode,
                0.0f,
                0.0f,
                0.0f
            );
        }
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x42d5c0 Player::ApplyEnvironmentProbeResult.
 * Purpose: provide the recovered copy node cached world matrix helper for
 * the Player/Pickup gameplay source cluster.
 */
void CopyNodeCachedWorldMatrix(
    zMat4x3 *outMatrix,
    zClass_NodePartial *node
) {
    zClass_Object3DDataPartial *const objectData = (zClass_Object3DDataPartial *)(node->classData);
    memcpy(
        outMatrix,
        objectData->cachedWorldMatrix,
        sizeof(*outMatrix)
    );
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x426770 Player::UpdateMasterTypeTrack, 0x42d5c0 Player::ApplyEnvironmentProbeResult, 0x4279f0 Player::UpdateMasterTypeAmphib.
 * Purpose: provide the recovered extract yaw from matrix helper for
 * the Player/Pickup gameplay source cluster.
 */
float ExtractYawFromMatrix(
    const zMat4x3 *matrix
) {
    return (float)(atan2(
        matrix->zx,
        matrix->zz
    ));
}

/**
 * Original-source inline helper evidence: source-faithful helper recovered from
 * inlined caller evidence.
 * No standalone retail function is present in the focused plan lookup.
 * Observed in caller 0x42aa50 Player::UpdateDebugOverlayHud; the switch and
 * string table match the HUD debug-line master-type formatting.
 * Purpose: return the debug HUD label for a player modal master type.
 */
const char *PlayerDebugMasterTypeName(
    int masterType
) {
    switch (masterType) {
    case 0:
        return g_Player_MasterTypeName_Basic;
    case kPlayerMasterTypeFly:
        return g_Player_MasterTypeName_Fly;
    case kPlayerMasterTypeSub:
        return g_Player_MasterTypeName_Sub;
    case kPlayerMasterTypeTrack:
        return g_Player_MasterTypeName_Track;
    case kPlayerMasterTypeHover:
        return g_Player_MasterTypeName_Hover;
    case kPlayerMasterTypeAmphib:
        return g_Player_MasterTypeName_Amphib;
    default:
        return g_Player_MasterTypeName_Unknown;
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x426770 Player::UpdateMasterTypeTrack, 0x42d5c0 Player::ApplyEnvironmentProbeResult.
 * Purpose: provide the recovered cache attachment local offset helper for
 * the Player/Pickup gameplay source cluster.
 */
void CacheAttachmentLocalOffset(
    zUtil_PlayerStateStorage *playerState
) {
    const float dx = playerState->worldPos.x - playerState->environmentAttachmentMatrix.posX;
    const float dy = playerState->worldPos.y - playerState->environmentAttachmentMatrix.posY;
    const float dz = playerState->worldPos.z - playerState->environmentAttachmentMatrix.posZ;
    const zMat4x3 *const matrix = &playerState->environmentAttachmentMatrix;

    playerState->fxOffsetLocal.x = dx * matrix->xx + dy * matrix->xy + dz * matrix->xz;
    playerState->fxOffsetLocal.y = dx * matrix->yx + dy * matrix->yy + dz * matrix->yz;
    playerState->fxOffsetLocal.z = dx * matrix->zx + dy * matrix->zy + dz * matrix->zz;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41f1d0 Player::ApplyMissionSaveData.
 * Purpose: provide the recovered player saved weapon controller helper for
 * the Player/Pickup gameplay source cluster.
 */
PlayerGunFireController *PlayerSavedWeaponController(
    PlayerAltWeaponBank *bank,
    int sideIndex
) {
    return sideIndex == 0 ? &bank->controllerA : &bank->controllerB;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41f1d0 Player::ApplyMissionSaveData.
 * Purpose: provide the recovered player restore saved weapon side helper for
 * the Player/Pickup gameplay source cluster.
 */
void PlayerRestoreSavedWeaponSide(
    PlayerGunFireController *controller,
    const PlayerMissionSaveWeaponSide *savedSide
) {
    controller->flags &= ~kPlayerGunControllerAvailableFlag;
    if ((savedSide->enabled & 1) != 0) {
        controller->flags |= kPlayerGunControllerAvailableFlag;
    }
    controller->ammoOrCharge = savedSide->ammoOrCharge;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41f1d0 Player::ApplyMissionSaveData.
 * Purpose: provide the recovered player refresh saved weapon bank hud helper for
 * the Player/Pickup gameplay source cluster.
 */
void PlayerRefreshSavedWeaponBankHud(
    int bankIndex,
    PlayerAltWeaponBank *bank
) {
    PlayerGunFireController *const selectedController =
        PlayerSavedWeaponController(
            bank,
            bank->selectedSide
        );

    HudUiMessage::SetValueIfOwnerMatches(
        bankIndex,
        bank->selectedSide,
        selectedController->ammoOrCharge
    );
    if ((selectedController->flags & kPlayerGunControllerAvailableFlag) != 0) {
        HudUiMessage::SelectVariantDisplay(
            bankIndex,
            bank->selectedSide
        );
    } else {
        HudUiMessage::ClearDisplay(bankIndex);
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x41f1d0 Player::ApplyMissionSaveData.
 * Purpose: provide the recovered player refresh previous weapon controller hud helper for
 * the Player/Pickup gameplay source cluster.
 */
void PlayerRefreshPreviousWeaponControllerHud(
    PlayerGunFireController *controller
) {
    if ((controller->flags & kPlayerGunControllerAvailableFlag) != 0) {
        HudUiMessage::SelectVariantDisplay(
            controller->weaponBankIndex,
            controller->weaponSideIndex
        );
    } else {
        HudUiMessage::ClearDisplay(controller->weaponBankIndex);
    }
}


















} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-selectmodalstatebymastertype-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x438540: Player::SelectModalStateByMasterType. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * Purpose: select the modal state matching a master type and stop existing modal loop handles before installing it as primary.
 */
int zUtil_SaveGameState::SelectModalStateByMasterType(
    int masterType
) {
    zUtil_SaveGameState *const saveState = this;
    PlayerModalState *modalState = saveState->modalStateListHead;
    if (modalState == 0) {
        return 0;
    }

    do {
        if (modalState->masterModalData->masterType == masterType) {
            saveState->StopModalLoopSfxHandle(2);
            saveState->StopModalLoopSfxHandle(0);
            saveState->StopModalLoopSfxHandle(1);
            saveState->primaryModalState = modalState;
            return 1;
        }
        modalState = modalState != 0 ? modalState->next : 0;
    } while (modalState != 0);

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-startmastertypeloopsfxhandle
 * @recoil-artifact defines .text recoil:function:0x4385a0: Player::StartMasterTypeLoopSfxHandle
 * Purpose: start the selected master-type weapon-up loop sample and cache the
 * returned play handle in the player state.
 */
zSndPlayHandle * zUtil_SaveGameState::StartMasterTypeLoopSfxHandle(
    int modeIndex,
    float sfxVolume
) {
    zUtil_SaveGameState *const saveState = this;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 *const worldPos = modeIndex != 3 ? &playerState->worldPos : 0;
    zSndSample *const sample = playerState->masterCommonData->sfxWeaponUp[modeIndex];
    zSndPlayHandle *const handle = sample->PlayA3D(
        worldPos,
        sfxVolume,
        0
    );
    playerState->modeLoopSfxHandle[modeIndex] = handle;
    return handle;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-startmodalloopsfxhandle-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x4385f0: Player::StartModalLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * Purpose: start one modal engine loop sample at the player world position and cache the returned play handle on the active modal state.
 */
void zUtil_SaveGameState::StartModalLoopSfxHandle(
    int modalSfxIndex,
    float sfxVolume
) {
    zUtil_SaveGameState *const saveState = this;
    PlayerModalState *const modalState = saveState->primaryModalState;
    zSndSample *const sample = modalState->masterModalData->sfxEngine[modalSfxIndex];
    zSndPlayHandle *const handle = sample->PlayA3D(
        &saveState->playerState->worldPos,
        sfxVolume,
        0
    );
    saveState->primaryModalState->modalSfxHandle[modalSfxIndex] = handle;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-ensuremastertypeloopsfxhandle-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x438630: Player::EnsureMasterTypeLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * Purpose: lazily start the selected master-type loop sample when configured and no cached handle is active.
 */
void zUtil_SaveGameState::EnsureMasterTypeLoopSfxHandle(
    int modeIndex,
    float sfxVolume
) {
    zUtil_SaveGameState *const saveState = this;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->modeLoopSfxHandle[modeIndex] == 0 &&
        playerState->masterCommonData->sfxWeaponUp[modeIndex] != 0) {
        saveState->StartMasterTypeLoopSfxHandle(
            modeIndex,
            sfxVolume
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-stopmastertypeloopsfxhandle-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x438660: Player::StopMasterTypeLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * Purpose: stop a cached master-type loop handle and clear the player-state handle slot when the handle is present.
 */
void zUtil_SaveGameState::StopMasterTypeLoopSfxHandle(
    int modeIndex
) {
    zUtil_SaveGameState *const saveState = this;
    zSndPlayHandle *const handle = saveState->playerState->modeLoopSfxHandle[modeIndex];
    if (handle != 0) {
        handle->StopIfActive();
        saveState->playerState->modeLoopSfxHandle[modeIndex] = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-stopmodalloopsfxhandle-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x438690: Player::StopModalLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * Purpose: stop a cached modal engine loop handle and clear the modal-state slot when the handle is present.
 */
void zUtil_SaveGameState::StopModalLoopSfxHandle(
    int modalSfxIndex
) {
    zUtil_SaveGameState *const saveState = this;
    zSndPlayHandle *const handle = saveState->primaryModalState->modalSfxHandle[modalSfxIndex];
    if (handle != 0) {
        handle->StopIfActive();
        saveState->primaryModalState->modalSfxHandle[modalSfxIndex] = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemodalloopsfx-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-reads-accepted-g-framedeltatimesec-and-original-inline-helpers-playerfloatfrombits-playerclamp01
 * @recoil-artifact defines .text recoil:function:0x4386c0: Player::UpdateModalLoopSfx. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; reads accepted g_FrameDeltaTimeSec and original inline helpers PlayerFloatFromBits/PlayerClamp01.
 * Purpose: maintain modal and master loop SFX handles, blend pitch and enable scales from movement state, and update 3D dispatch positions.
 */
void zUtil_SaveGameState::UpdateModalLoopSfx(
    int enabled
) {
    zUtil_SaveGameState *const saveState = this;
    if (enabled == 0) {
        saveState->StopModalLoopSfxHandle(2);
        saveState->StopModalLoopSfxHandle(0);
        saveState->StopModalLoopSfxHandle(1);
        saveState->StopMasterTypeLoopSfxHandle(3);
        return;
    }

    PlayerModalState *primaryModalState = saveState->primaryModalState;
    if (primaryModalState->modalSfxHandle[0] == 0) {
        if (primaryModalState->masterModalData->sfxEngine[0] != 0) {
            saveState->StartModalLoopSfxHandle(
                0,
                0.0f
            );
        }
        if (saveState->primaryModalState->masterModalData->sfxEngine[1] != 0) {
            saveState->StartModalLoopSfxHandle(
                1,
                0.0f
            );
        }
        if (saveState->primaryModalState->masterModalData->sfxEngine[2] != 0) {
            saveState->StartModalLoopSfxHandle(
                2,
                0.0f
            );
        }
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->slipSfxActive != 0 || playerState->airborneFlag != 0 ||
        playerState->damageProtectionActive != 0) {
        const int smoothingBits = (int)(g_FrameDeltaTimeSec * -2.5f * 12102200.0f) + 0x3f800000;
        const float smoothingFactor = PlayerFloatFromBits(smoothingBits);
        saveState->modeLoopBlend =
            fabsf(playerState->throttleInputCopy) * (1.0f - smoothingFactor) +
            smoothingFactor * saveState->modeLoopBlend;
    } else {
        saveState->modeLoopBlend =
            fabsf(playerState->localVel.z) / primaryModalState->masterModalData->maxSpeed;
    }

    saveState->modeLoopBlend = PlayerClamp01(saveState->modeLoopBlend);

    primaryModalState = saveState->primaryModalState;
    zSndPlayHandle *handle = primaryModalState->modalSfxHandle[2];
    if (handle != 0) {
        handle->SetFreqScaled(
            primaryModalState->masterModalData->sfxPitchScale * saveState->modeLoopBlend
        );
        saveState->primaryModalState->modalSfxHandle[2]->SetEnableScale(
            1.0f - saveState->modeLoopBlend
        );
        saveState->primaryModalState->modalSfxHandle[2]
            ->Update3DDispatch(
                &saveState->playerState->worldPos,
                0,
                0
            );
    }

    primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;
    const float engineEnableScale =
        PlayerClamp01(masterModalData->sfxVolumeScale * saveState->modeLoopBlend + 0.699999988f);

    primaryModalState->modalSfxHandle[0]->SetFreqScaled(
        masterModalData->sfxPitchScale * saveState->modeLoopBlend
    );
    saveState->primaryModalState->modalSfxHandle[0]->SetEnableScale(engineEnableScale);
    saveState->primaryModalState->modalSfxHandle[0]
        ->Update3DDispatch(
            &saveState->playerState->worldPos,
            0,
            0
        );

    handle = saveState->primaryModalState->modalSfxHandle[1];
    if (handle != 0) {
        handle->SetEnableScale(saveState->modeLoopBlend);
        saveState->primaryModalState->modalSfxHandle[1]
            ->Update3DDispatch(
                &saveState->playerState->worldPos,
                0,
                0
            );
    }
}

/**
 * Original inline helper; no standalone retail function exists. Observed in
 * address-backed constructor 0x41eb30 as HudUiElement::Constructor(0, 0)
 * followed by clearing the pass-3 clip pointer.
 * Purpose: construct the underwater pass-3 HUD overlay as a zVideoFxPass3Element
 * and clear the per-pass clip rectangle consumed by ApplyPass3.
 */
Player_UnderwaterFxPass3Ui::Player_UnderwaterFxPass3Ui() : zVideoFxPass3Element(
        0,
        0
    ) {
}


/**
 * Original inline helper; no standalone retail function exists. Observed in
 * address-backed constructor 0x41eb90 as HudUiElement::Constructor(0, 0)
 * followed by clearing the pass-3 clip pointer.
 * Purpose: construct the projectile-camera pass-3 HUD overlay as a
 * zVideoFxPass3Element and clear the per-pass clip rectangle consumed by
 * ApplyPass3.
 */
Player_ProjectileCameraFxPass3Ui::Player_ProjectileCameraFxPass3Ui() : zVideoFxPass3Element(
        0,
        0
    ) {
}


namespace HudUiMgrSensor {

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *HudUiCrtInitializerFn)();
/* VC5 emits this player-TU startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
HudUiCrtInitializerFn s_HudUiCrtInit_HudUiMgrSensorTrackListReset =
    TrackList_Reset;
#pragma data_seg()
#endif
} // namespace HudUiMgrSensor



namespace zVehicle {


} // namespace zVehicle

namespace Player_TopMsgPanel1 {



} // namespace Player_TopMsgPanel1

namespace Player_TopMsgPanel2 {



} // namespace Player_TopMsgPanel2

namespace PlayerNodeFlagRestore {





} // namespace PlayerNodeFlagRestore

namespace Player {















#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *PlayerCrtInitializerFn)();
/* VC5 emits these player.cpp startup callbacks as direct .CRT$XCU rows. */
#pragma data_seg(".CRT$XCU")
PlayerCrtInitializerFn s_PlayerCrtInit_InitMasterCommonDataList =
    InitMasterCommonDataList;
PlayerCrtInitializerFn s_PlayerCrtInit_InitMasterModalDataList =
    InitMasterModalDataList;
PlayerCrtInitializerFn s_PlayerCrtInit_InitAndRegisterUnderwaterFxPass3UiSingleton =
    InitAndRegisterUnderwaterFxPass3UiSingleton;
PlayerCrtInitializerFn s_PlayerCrtInit_InitAndRegisterProjectileCameraFxPass3UiSingleton =
    InitAndRegisterProjectileCameraFxPass3UiSingleton;
PlayerCrtInitializerFn s_PlayerCrtInit_InitSaveStateList =
    InitSaveStateList;
PlayerCrtInitializerFn s_PlayerCrtInit_InitAndRegisterTopMsgPanel1 =
    InitAndRegisterTopMsgPanel1;
PlayerCrtInitializerFn s_PlayerCrtInit_InitAndRegisterTopMsgPanel2 =
    InitAndRegisterTopMsgPanel2;
PlayerCrtInitializerFn s_PlayerCrtInit_PlayerNodeFlagRestoreInitGlobals =
    PlayerNodeFlagRestore::InitGlobals;
#pragma data_seg()
#endif




























} // namespace Player

namespace zMath {

} // namespace zMath

namespace Player {

} // namespace Player

namespace Player {

} // namespace Player










#include "GameZRecoil/zCom/zCom.h"

namespace {
template <typename T> struct ComReleaseOnExit {
    T *ptr;

    /**
     * Recovered original helper with no standalone retail function; observed in
     * callers 0x42dc30 and 0x42dcf0 through EH cleanup.
     *
     * Purpose: release a COM interface pointer when the helper leaves scope.
     */
    ~ComReleaseOnExit() {
        if (ptr != 0) {
            ptr->Release();
        }
    }
};

} // namespace


namespace {
struct PlayerCheckpointLapProgressView {
    unsigned char unknown_0000[0x1018];
    int checkpointVisitedFlags[33];
    float lapTimeDelta;
    float lapTimeSec;
    float lapTimestampSec;
    float checkpointTimestampSec;
    int lapCompletionCount;
};

RECOIL_STATIC_ASSERT(
    offsetof(
        PlayerCheckpointLapProgressView,
        checkpointVisitedFlags
    ) == 0x1018
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PlayerCheckpointLapProgressView,
        lapTimeDelta
    ) == 0x109c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PlayerCheckpointLapProgressView,
        lapTimeSec
    ) == 0x10a0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PlayerCheckpointLapProgressView,
        lapTimestampSec
    ) == 0x10a4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PlayerCheckpointLapProgressView,
        checkpointTimestampSec
    ) == 0x10a8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PlayerCheckpointLapProgressView,
        lapCompletionCount
    ) == 0x10ac
);
} // namespace

namespace Checkpoint {

} // namespace Checkpoint


namespace Player {
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x424010 PlayerPendingContact::SelectPreferred, 0x4251f0 Player::CollectPendingCollisionContactsForQuadProbe, 0x426770 Player::UpdateMasterTypeTrack, 0x428d60 Player::ProbeModalSampleHeights.
 * Purpose: provide the recovered transform point by matrix helper for
 * the Player/Pickup gameplay source cluster.
 */
zVec3 TransformPointByMatrix(
    const zVec3 &point,
    const zMat4x3 &matrix
) {
    zVec3 result = {0};
    result.x = point.x * matrix.xx + point.y * matrix.yx + point.z * matrix.zx + matrix.posX;
    result.y = point.x * matrix.xy + point.y * matrix.yy + point.z * matrix.zy + matrix.posY;
    result.z = point.x * matrix.xz + point.y * matrix.yz + point.z * matrix.zz + matrix.posZ;
    return result;
}

enum {
    kPlayerMasterTypeSub = 2,
    kPlayerMaxModalProbePoints = 4,
    kPlayerEnvProbeBasePointOffset = 15
};

#define PLAYER_MAX_MODAL_PROBE_POINTS 4

const int g_PlayerEnvProbeSampleMaskTable[8] = {0x89, 0x43, 0x86, 0x4c, 0x28, 0x22, 0xf0, 0x00};

enum PlayerCameraState {
    kPlayerCameraStateToggleRequest = 0,
    kPlayerCameraStateThirdPerson = 1,
    kPlayerCameraStateClearScreen = 2,
    kPlayerCameraStateFirstPerson = 3,
    kPlayerCameraStateTargeting = 4,
    kPlayerCameraStateProjectileAttached = 7,
    kPlayerCameraStateRestorePrevious = 8
};

struct PlayerContactSurfacePayload {
    unsigned char unknown_00[0x20];
    int impactSlot;
};

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x405c90 Player::ApplyCameraState.
 * Purpose: provide the recovered set state7 fx pass3 visible helper for
 * the Player/Pickup gameplay source cluster.
 */
void SetState7FxPass3Visible(
    int visible
) {
    g_Player_State7FxPass3Ui.SetVisible(visible);
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x423c20 Player::ClassifyPendingContactsForSegment.
 * Purpose: provide the recovered append pending contact helper for
 * the Player/Pickup gameplay source cluster.
 */
PlayerPendingContact *AppendPendingContact(
    PlayerPendingContactQueue *queue
) {
    PlayerPendingContact *const contact = new PlayerPendingContact;
    memset(
        contact,
        0,
        sizeof(*contact)
    );
    contact->next = 0;

    if (queue->count == 0) {
        queue->head = contact;
    } else {
        queue->tail->next = contact;
    }

    queue->tail = contact;
    contact->next = 0;
    ++queue->count;
    return contact;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x423c20 Player::ClassifyPendingContactsForSegment.
 * Purpose: provide the recovered copy pending contact payload helper for
 * the Player/Pickup gameplay source cluster.
 */
void CopyPendingContactPayload(
    PlayerPendingContact *contact,
    const zClassDiPickCandidateEntry *candidate,
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd,
    int segmentTag
) {
    contact->hit = *candidate;
    contact->sweepStart = *segmentStart;
    contact->sweepEnd = *segmentEnd;
    contact->segmentTag = segmentTag;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x423c20 Player::ClassifyPendingContactsForSegment.
 * Purpose: provide the recovered get node damage handler helper for
 * the Player/Pickup gameplay source cluster.
 */
OptCatalogDamageHandlerPartial *GetNodeDamageHandler(
    zClass_NodePartial *node
) {
    return (OptCatalogDamageHandlerPartial *)(((zClass_NodeFreeListSlot *)(node))->damageHandler);
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x423530 Player::ClearPendingContactQueues.
 * Purpose: provide the recovered free pending contact queue helper for
 * the Player/Pickup gameplay source cluster.
 */
void FreePendingContactQueue(
    PlayerPendingContactQueue *queue
) {
    PlayerPendingContact *contact = queue->head;
    while (contact != 0) {
        PlayerPendingContact *const next = contact->next;
        delete contact;
        contact = next;
    }

    queue->listAux = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x424010 PlayerPendingContact::SelectPreferred, 0x424d00 Player::ProcessTransferContactQueue.
 * Purpose: provide the recovered append existing pending contact helper for
 * the Player/Pickup gameplay source cluster.
 */
void AppendExistingPendingContact(
    PlayerPendingContactQueue *queue,
    PlayerPendingContact *contact
) {
    contact->next = 0;
    if (queue->count == 0) {
        queue->head = contact;
    } else {
        queue->tail->next = contact;
    }

    queue->tail = contact;
    contact->next = 0;
    ++queue->count;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424d00 Player::ProcessTransferContactQueue.
 * Purpose: provide the recovered remove existing pending contact helper for
 * the Player/Pickup gameplay source cluster.
 */
void RemoveExistingPendingContact(
    PlayerPendingContactQueue *queue,
    PlayerPendingContact *contact
) {
    if (queue->count == 0 || contact == 0) {
        return;
    }

    if (queue->head == contact) {
        queue->head = contact->next;
        --queue->count;
        if (queue->head == 0) {
            queue->listAux = 0;
            queue->tail = 0;
        }
        return;
    }

    PlayerPendingContact *previous = queue->head;
    while (previous != 0 && previous->next != contact) {
        previous = previous->next;
    }

    if (previous != 0) {
        previous->next = contact->next;
        --queue->count;
        if (queue->tail == contact) {
            queue->tail = previous;
        }
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x4251f0 Player::CollectPendingCollisionContactsForQuadProbe, 0x424ed0 Player::TryResolvePendingCollisionProbeSweep.
 * Purpose: provide the recovered move transfer contacts to preferred collision helper for
 * the Player/Pickup gameplay source cluster.
 */
void MoveTransferContactsToPreferredCollision(
    zUtil_PlayerStateStorage *playerState
) {
    PlayerPendingContact *contact = playerState->transferQueue.head;
    while (contact != 0) {
        PlayerPendingContact *const next = contact->next;
        playerState->transferQueue.head = next;
        --playerState->transferQueue.count;
        if (next == 0) {
            playerState->transferQueue.listAux = 0;
            playerState->transferQueue.tail = 0;
        }

        AppendExistingPendingContact(
            &playerState->preferredCollisionQueue,
            contact
        );
        contact = next;
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x4236b0 Player::BuildPendingContactQueues.
 * Purpose: provide the recovered enable contact segment helper for
 * the Player/Pickup gameplay source cluster.
 */
void EnableContactSegment(
    int *enabledSegmentFlags,
    int index
) {
    enabledSegmentFlags[index] = 1;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x4236b0 Player::BuildPendingContactQueues.
 * Purpose: provide the recovered build modal and root probe world caches helper for
 * the Player/Pickup gameplay source cluster.
 */
void BuildModalAndRootProbeWorldCaches(
    zUtil_PlayerStateStorage *playerState,
    const PlayerMasterModalData *masterModalData
) {
    for (int i = 0; i < masterModalData->probePointCount; ++i) {
        playerState->modalProbeWorldByIndex[i] =
            TransformPointByMatrix(
                masterModalData->probePoints[i],
                playerState->motionBasis
            );
        playerState->rootProbeWorldByIndex[i] =
            TransformPointByMatrix(
                masterModalData->probePoints[i],
                playerState->previousTransform
            );
    }
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424270 Player::ResolvePendingCollisionContact.
 * Purpose: provide the recovered vec3 length helper for
 * the Player/Pickup gameplay source cluster.
 */
float Vec3Length(
    const zVec3 &vec
) {
    return (float)(sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z));
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424270 Player::ResolvePendingCollisionContact.
 * Purpose: provide the recovered vec3 dot helper for
 * the Player/Pickup gameplay source cluster.
 */
float Vec3Dot(
    const zVec3 &a,
    const zVec3 &b
) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424270 Player::ResolvePendingCollisionContact.
 * Purpose: provide the recovered vec3 dot xz helper for
 * the Player/Pickup gameplay source cluster.
 */
float Vec3DotXZ(
    const zVec3 &a,
    const zVec3 &b
) {
    return a.x * b.x + a.z * b.z;
}

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424270 Player::ResolvePendingCollisionContact.
 * Purpose: provide the recovered vec3 cross helper for
 * the Player/Pickup gameplay source cluster.
 */
zVec3 Vec3Cross(
    const zVec3 &a,
    const zVec3 &b
) {
    zVec3 result;
    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;
    return result;
}


/*
 * Mission-order Player callback bodies compile from mission.cpp; retain the
 * original player.cpp address provenance for focused source guards.
 */











} // namespace Player

namespace PlayerPickupContact {


} // namespace PlayerPickupContact

namespace Player {






















































































/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x43c850 Player::ResetAltGunRuntimeState.
 * Purpose: provide the recovered reset alt gun attach node helper for
 * the Player/Pickup gameplay source cluster.
 */
void ResetAltGunAttachNode(
    PlayerGunFireController *controller
) {
    zClass_NodePartial *const attachNode = controller->attachNodePrimary;
    if (attachNode == 0) {
        return;
    }

    zClass_Class::gwNodeSetActive(
        attachNode,
        0
    );
    zClass_Object3D::gwObject3DSetPosition(
        attachNode,
        controller->attachPosX,
        controller->attachPosY,
        controller->attachPosZ
    );
    zClass_Object3D::gwObject3DSetScale(
        attachNode,
        1.0f,
        1.0f,
        1.0f
    );
}






















































/**
 * Original-source static helper; no standalone retail function exists.
 * Observed in caller 0x41fd20 Player::DestroySaveGameState.
 * Evidence: the caller contains the HUD sensor track-list unlink sequence inline.
 * Purpose: Remove a HUD sensor track node from the global mission track list.
 */
static void RemoveTrackNode(
    HudUiMgrSensorTrackNode *trackNode
) {
    if (g_HudUiMgrSensor_TrackList.count != 0) {
        HudUiMgrSensorTrackNode *cursor = g_HudUiMgrSensor_TrackList.head;
        if (trackNode == cursor) {
            --g_HudUiMgrSensor_TrackList.count;
            g_HudUiMgrSensor_TrackList.head = trackNode->next;
            if (g_HudUiMgrSensor_TrackList.head == 0) {
                g_HudUiMgrSensor_TrackList.trackListAux = 0;
                g_HudUiMgrSensor_TrackList.tail = 0;
            }
        } else {
            while (cursor != 0) {
                HudUiMgrSensorTrackNode *const next = cursor->next;
                if (next == trackNode) {
                    --g_HudUiMgrSensor_TrackList.count;
                    cursor->next = trackNode->next;
                    if (g_HudUiMgrSensor_TrackList.tail == trackNode) {
                        g_HudUiMgrSensor_TrackList.tail = cursor;
                    }
                    break;
                }

                cursor = next;
            }
        }
    }
}

/**
 * Original-source static helper; no standalone retail function exists.
 * Observed in caller 0x41fd20 Player::DestroySaveGameState.
 * Evidence: the caller contains the player save-state list unlink sequence inline.
 * Purpose: Remove a save state from the global mission save-state list.
 */
static void UnlinkSaveState(
    zUtil_SaveGameState *saveState
) {
    if (g_PlayerSaveStateCount == 0) {
        return;
    }

    zUtil_SaveGameState *cursor = g_PlayerSaveStateListHead;
    if (saveState == cursor) {
        --g_PlayerSaveStateCount;
        g_PlayerSaveStateListHead = saveState->next;
        if (g_PlayerSaveStateListHead == 0) {
            g_PlayerSaveStateListAux = 0;
            g_PlayerSaveStateListTail = 0;
        }
        return;
    }

    while (cursor != 0) {
        zUtil_SaveGameState *const next = cursor->next;
        if (next == saveState) {
            --g_PlayerSaveStateCount;
            cursor->next = saveState->next;
            if (g_PlayerSaveStateListTail == saveState) {
                g_PlayerSaveStateListTail = cursor;
            }
            break;
        }

        cursor = next;
    }
}


/**
 * Original-source static helper; no standalone retail function exists.
 * Observed in caller 0x41fb80 Player::ShutdownMissionRuntime.
 * Evidence: the caller contains the remaining HUD sensor track-node deletion loop inline.
 * Purpose: Delete leftover HUD sensor track nodes and clear the global track list.
 */
static void DeleteRemainingTrackNodes() {
    HudUiMgrSensorTrackNode *node = g_HudUiMgrSensor_TrackList.head;
    while (node != 0) {
        HudUiMgrSensorTrackNode *const next = node->next;
        ::operator delete(node);
        node = next;
    }

    memset(
        &g_HudUiMgrSensor_TrackList,
        0,
        sizeof(g_HudUiMgrSensor_TrackList)
    );
}

/**
 * Original-source static helper; no standalone retail function exists.
 * Observed in caller 0x41fb80 Player::ShutdownMissionRuntime.
 * Evidence: the caller contains the weapon-spec deletion and list-clear sequence inline.
 * Purpose: Delete all weapon specs owned by one PlayerMasterCommonData record.
 */
static void DeleteWeaponSpecs(
    PlayerMasterCommonData *commonData
) {
    PlayerMasterWeaponSpec *weaponSpec = commonData->weaponSpecHead;
    while (weaponSpec != 0) {
        PlayerMasterWeaponSpec *const next = weaponSpec->next;
        ::operator delete(weaponSpec);
        weaponSpec = next;
    }

    commonData->weaponSpecListAux = 0;
    commonData->weaponSpecTail = 0;
    commonData->weaponSpecHead = 0;
    commonData->weaponSpecCount = 0;
}

} // namespace Player

/* Governed authored-order insertion point: keep selected retail bodies below. */
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initmastercommondatalist
 * @recoil-artifact defines .text recoil:function:0x41ea90: Player::InitMasterCommonDataList.
 * Purpose: clear the master common-data intrusive-list bootstrap globals.
 */
void __cdecl InitMasterCommonDataList() {
    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataTail = 0;
    g_PlayerMasterCommonDataHead = 0;
    g_PlayerMasterCommonDataCount = 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initmastermodaldatalist
 * @recoil-artifact defines .text recoil:function:0x41eac0: Player::InitMasterModalDataList.
 * Purpose: clear the master modal-data intrusive-list bootstrap globals.
 */
void __cdecl InitMasterModalDataList() {
    g_PlayerMasterModalDataListAux = 0;
    g_PlayerMasterModalDataTail = 0;
    g_PlayerMasterModalDataHead = 0;
    g_PlayerMasterModalDataCount = 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initandregisterunderwaterfxpass3uisingleton
 * @recoil-artifact defines .text recoil:function:0x41eaf0: Player::InitAndRegisterUnderwaterFxPass3UiSingleton.
 * Purpose: run the underwater pass-3 HUD singleton constructor and register
 * its atexit reset callback.
 */
void __cdecl InitAndRegisterUnderwaterFxPass3UiSingleton() {
    InitUnderwaterFxPass3UiSingleton();
    RegisterUnderwaterFxPass3UiOnExit();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initunderwaterfxpass3uisingleton
 * @recoil-artifact defines .text recoil:function:0x41eb00: Player::InitUnderwaterFxPass3UiSingleton.
 * Purpose: construct the zero-initialized global underwater pass-3 HUD overlay
 * singleton at startup.
 */
void InitUnderwaterFxPass3UiSingleton() {
    g_Player_UnderwaterFxPass3Ui.Constructor();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-registerunderwaterfxpass3uionexit
 * @recoil-artifact defines .text recoil:function:0x41eb10: Player::RegisterUnderwaterFxPass3UiOnExit.
 * Purpose: register the underwater pass-3 HUD singleton reset callback with
 * the CRT exit list.
 */
void RegisterUnderwaterFxPass3UiOnExit() {
    atexit(ResetUnderwaterFxPass3UiSingleton);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resetunderwaterfxpass3uisingleton
 * @recoil-artifact defines .text recoil:function:0x41eb20: Player::ResetUnderwaterFxPass3UiSingleton.
 * Purpose: reset the underwater pass-3 HUD overlay singleton to the common
 * HudUiElement destruction state during CRT exit.
 */
void __cdecl ResetUnderwaterFxPass3UiSingleton() {
    g_Player_UnderwaterFxPass3Ui.~Player_UnderwaterFxPass3Ui();
}
} // namespace Player
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-underwaterfxpass3ui-constructor
 * @recoil-artifact defines .text recoil:function:0x41eb30: Player_UnderwaterFxPass3Ui::Constructor.
 * Purpose: construct the underwater pass-3 HUD overlay singleton storage and
 * return the initialized object.
 */
Player_UnderwaterFxPass3Ui * Player_UnderwaterFxPass3Ui::Constructor() {
    new (this) Player_UnderwaterFxPass3Ui();
    return this;
}
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initandregisterprojectilecamerafxpass3uisingleton
 * @recoil-artifact defines .text recoil:function:0x41eb50: Player::InitAndRegisterProjectileCameraFxPass3UiSingleton.
 * Purpose: construct the projectile-camera pass-3 HUD singleton and register
 * its CRT exit reset callback.
 */
void __cdecl InitAndRegisterProjectileCameraFxPass3UiSingleton() {
    InitProjectileCameraFxPass3UiSingleton();
    RegisterProjectileCameraFxPass3UiCleanup();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initprojectilecamerafxpass3uisingleton
 * @recoil-artifact defines .text recoil:function:0x41eb60: Player::InitProjectileCameraFxPass3UiSingleton.
 * Purpose: construct the global projectile-camera pass-3 HUD overlay singleton.
 */
void InitProjectileCameraFxPass3UiSingleton() {
    g_Player_State7FxPass3Ui.Constructor();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-registerprojectilecamerafxpass3uicleanup
 * @recoil-artifact defines .text recoil:function:0x41eb70: Player::RegisterProjectileCameraFxPass3UiCleanup.
 * Purpose: register the projectile-camera pass-3 HUD singleton reset callback
 * with the CRT exit list.
 */
void RegisterProjectileCameraFxPass3UiCleanup() {
    atexit(ResetProjectileCameraFxPass3UiSingleton);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resetprojectilecamerafxpass3uisingleton
 * @recoil-artifact defines .text recoil:function:0x41eb80: Player::ResetProjectileCameraFxPass3UiSingleton.
 * Purpose: reset the projectile-camera pass-3 HUD overlay singleton to the
 * common HudUiElement destruction state during CRT exit.
 */
void __cdecl ResetProjectileCameraFxPass3UiSingleton() {
    g_Player_State7FxPass3Ui.~Player_ProjectileCameraFxPass3Ui();
}
} // namespace Player
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-projectilecamerafxpass3ui-constructor
 * @recoil-artifact defines .text recoil:function:0x41eb90: Player_ProjectileCameraFxPass3Ui::Constructor.
 * Purpose: construct the projectile-camera pass-3 HUD overlay singleton storage
 * and return the initialized object.
 */
Player_ProjectileCameraFxPass3Ui * Player_ProjectileCameraFxPass3Ui::Constructor() {
    new (this) Player_ProjectileCameraFxPass3Ui();
    return this;
}
namespace HudUiMgrSensor {
/**
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: clear the recovered sensor track-list global before target
 * tracking records are appended for the current HUD update pass.
 */
void __cdecl TrackList_Reset() {
    memset(
        &g_HudUiMgrSensor_TrackList,
        0,
        sizeof(g_HudUiMgrSensor_TrackList)
    );
}
} // namespace HudUiMgrSensor
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initsavestatelist
 * @recoil-artifact defines .text recoil:function:0x41ec00: Player::InitSaveStateList
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: clear the player save-state list bootstrap globals.
 * Source owner/evidence: Player save-state/bootstrap record-global subsystem;
 * resets the authored head, tail, count, and auxiliary list globals.
 */
void __cdecl InitSaveStateList() {
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateListTail = 0;
    g_PlayerSaveStateListHead = 0;
    g_PlayerSaveStateCount = 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initandregistertopmsgpanel1
 * @recoil-artifact defines .text recoil:function:0x41ec30: Player::InitAndRegisterTopMsgPanel1.
 * Purpose: construct the first top-message panel singleton and register its
 * CRT exit destructor.
 */
void __cdecl InitAndRegisterTopMsgPanel1() {
    Player_TopMsgPanel1::Constructor();
    RegisterTopMsgPanel1OnExit();
}
} // namespace Player
namespace Player_TopMsgPanel1 {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-topmsgpanel1-constructor
 * @recoil-artifact defines .text recoil:function:0x41ec40: Player_TopMsgPanel1::Constructor.
 * Purpose: construct the first top-message HUD panel singleton with default
 * panel state.
 */
void Constructor() {
    g_Player_TopMsgPanel1.ConstructorDefault(
        0,
        0,
        0
    );
}
} // namespace Player_TopMsgPanel1
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-registertopmsgpanel1onexit
 * @recoil-artifact defines .text recoil:function:0x41ec60: Player::RegisterTopMsgPanel1OnExit.
 * Purpose: register the first top-message panel destructor with the CRT exit
 * list.
 */
void RegisterTopMsgPanel1OnExit() {
    atexit(Player_TopMsgPanel1::Destructor);
}
} // namespace Player
namespace Player_TopMsgPanel1 {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-topmsgpanel1-destructor
 * @recoil-artifact defines .text recoil:function:0x41ec70: Player_TopMsgPanel1::Destructor.
 * Purpose: destroy the first top-message HUD panel singleton during CRT exit.
 */
void __cdecl Destructor() {
    g_Player_TopMsgPanel1.~HudUiPanel();
}
} // namespace Player_TopMsgPanel1
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initandregistertopmsgpanel2
 * @recoil-artifact defines .text recoil:function:0x41ec80: Player::InitAndRegisterTopMsgPanel2.
 * Purpose: construct the second top-message panel singleton and register its
 * CRT exit destructor.
 */
void __cdecl InitAndRegisterTopMsgPanel2() {
    Player_TopMsgPanel2::Constructor();
    RegisterTopMsgPanel2Cleanup();
}
} // namespace Player
namespace Player_TopMsgPanel2 {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-topmsgpanel2-constructor
 * @recoil-artifact defines .text recoil:function:0x41ec90: Player_TopMsgPanel2::Constructor.
 * Purpose: construct the second top-message HUD panel singleton with default
 * panel state.
 */
void Constructor() {
    g_Player_TopMsgPanel2.ConstructorDefault(
        0,
        0,
        0
    );
}
} // namespace Player_TopMsgPanel2
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-registertopmsgpanel2cleanup
 * @recoil-artifact defines .text recoil:function:0x41ecb0: Player::RegisterTopMsgPanel2Cleanup.
 * Purpose: register the second top-message panel destructor with the CRT exit
 * list.
 */
void RegisterTopMsgPanel2Cleanup() {
    atexit(Player_TopMsgPanel2::Destructor);
}
} // namespace Player
namespace Player_TopMsgPanel2 {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-topmsgpanel2-destructor
 * @recoil-artifact defines .text recoil:function:0x41ecc0: Player_TopMsgPanel2::Destructor.
 * Purpose: destroy the second top-message HUD panel singleton during CRT exit.
 */
void __cdecl Destructor() {
    g_Player_TopMsgPanel2.~HudUiPanel();
}
} // namespace Player_TopMsgPanel2
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-recordnodeflagsforrestore
 * @recoil-artifact defines .text recoil:function:0x41ecd0: Player::RecordNodeFlagsForRestore.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::RecordNodeFlagsForRestore from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RecordNodeFlagsForRestore(
    zClass_NodePartial *node
) {
    PlayerNodeFlagRestoreEntry value;
    value.node = node;
    zClass_Class::gwNodeGetCellPickable(
        node,
        &value.wasCellPickable
    );
    zClass_Class::gwNodeGetRaycastable(
        node,
        &value.wasRaycastable
    );
    zClass_Class::gwNodeGetPickable(
        node,
        &value.wasPickable
    );

    PlayerNodeFlagRestoreEntry *begin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *end = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *capacityEnd = g_PlayerNodeFlagRestoreEntriesCapacityEnd;
    const int count = begin != 0 ? (int)(end - begin) : 0;
    const int capacity = begin != 0 ? (int)(capacityEnd - begin) : 0;

    if (count >= capacity) {
        const int newCapacity = count <= 1 ? count + 1 : count * 2;
        PlayerNodeFlagRestoreEntry *const newBegin = (PlayerNodeFlagRestoreEntry *)(::operator new(
            sizeof(PlayerNodeFlagRestoreEntry) * newCapacity
        ));

        for (int i = 0; i < count; ++i) {
            newBegin[i] = begin[i];
        }

        ::operator delete(begin);
        g_PlayerNodeFlagRestoreEntriesBegin = newBegin;
        g_PlayerNodeFlagRestoreEntriesEnd = newBegin + count;
        g_PlayerNodeFlagRestoreEntriesCapacityEnd = newBegin + newCapacity;
        begin = newBegin;
        end = newBegin + count;
    }

    *end = value;
    g_PlayerNodeFlagRestoreEntriesEnd = end + 1;
}
} // namespace Player
namespace PlayerNodeFlagRestore {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-playernodeflagrestore-initglobals
 * @recoil-artifact defines .text recoil:function:0x41ef30: PlayerNodeFlagRestore::InitGlobals.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement PlayerNodeFlagRestore::InitGlobals from the recovered
 * Battlesport gameplay source file.
 */
void __cdecl InitGlobals() {
    InitInstance();
    RegisterAtExit();
}
} // namespace PlayerNodeFlagRestore
namespace PlayerNodeFlagRestore {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-playernodeflagrestore-initinstance
 * @recoil-artifact defines .text recoil:function:0x41ef40: PlayerNodeFlagRestore::InitInstance.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement PlayerNodeFlagRestore::InitInstance from the recovered
 * Battlesport gameplay source file.
 */
void InitInstance() {
    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = 0;
    g_PlayerNodeFlagRestoreEntriesBegin = 0;
    g_PlayerNodeFlagRestoreEntriesEnd = 0;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = 0;
}
} // namespace PlayerNodeFlagRestore
namespace PlayerNodeFlagRestore {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-playernodeflagrestore-registeratexit
 * @recoil-artifact defines .text recoil:function:0x41ef60: PlayerNodeFlagRestore::RegisterAtExit.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement PlayerNodeFlagRestore::RegisterAtExit from the recovered
 * Battlesport gameplay source file.
 */
void RegisterAtExit() {
    atexit(ShutdownInstance);
}
} // namespace PlayerNodeFlagRestore
namespace PlayerNodeFlagRestore {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-playernodeflagrestore-shutdowninstance
 * @recoil-artifact defines .text recoil:function:0x41ef70: PlayerNodeFlagRestore::ShutdownInstance.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement PlayerNodeFlagRestore::ShutdownInstance from the recovered
 * Battlesport gameplay source file.
 */
void __cdecl ShutdownInstance() {
    ::operator delete(g_PlayerNodeFlagRestoreEntriesBegin);
    g_PlayerNodeFlagRestoreEntriesBegin = 0;
    g_PlayerNodeFlagRestoreEntriesEnd = 0;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = 0;
}
} // namespace PlayerNodeFlagRestore
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-restorerecordednodeflags
 * @recoil-artifact defines .text recoil:function:0x41efa0: Player::RestoreRecordedNodeFlags.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::RestoreRecordedNodeFlags from the recovered
 * Battlesport gameplay source file.
 */
void RestoreRecordedNodeFlags() {
    PlayerNodeFlagRestoreEntry *entry = g_PlayerNodeFlagRestoreEntriesBegin;
    while (entry != g_PlayerNodeFlagRestoreEntriesEnd) {
        zClass_NodePartial *const node = entry->node;
        if (entry->wasCellPickable != 0) {
            zClass_Class::gwNodeSetCellPickable(
                node,
                1
            );
        }
        if (entry->wasRaycastable != 0) {
            zClass_Class::gwNodeSetRaycastable(
                node,
                1
            );
        }
        if (entry->wasPickable != 0) {
            zClass_Class::gwNodeSetPickable(
                node,
                1
            );
        }
        ++entry;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-buildmissionsavedata
 * @recoil-artifact defines .text recoil:function:0x41f010: Player::BuildMissionSaveData
 * Purpose: copy the live local-player mission state into the save-section payload.
 */
void __fastcall BuildMissionSaveData(
    PlayerMissionSaveData *outData
) {
    zUtil_SaveGameState *const localSaveState = g_LocalPlayerSaveState;
    zUtil_PlayerStateStorage *const playerState = localSaveState->playerState;
    PlayerMasterModalData *const masterModalData =
        localSaveState->primaryModalState->masterModalData;

    outData->size = sizeof(PlayerMissionSaveData);
    {
        for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
            const PlayerAltWeaponBank &srcBank = playerState->altWeaponBanks[bankIndex];
            PlayerMissionSaveWeaponBank &dstBank = outData->weaponBank[bankIndex];

            dstBank.selectedSide = srcBank.selectedSide;
            const PlayerGunFireController *controller = &srcBank.controllerA;

            {
                for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
                    dstBank.sides[sideIndex].enabled = (controller->flags >> 2) & 1;
                    dstBank.sides[sideIndex].ammoOrCharge = controller->ammoOrCharge;
                    ++controller;
                }
            }
        }
    }

    outData->altWeaponBankIndex = playerState->activeAltGunController->weaponBankIndex;
    outData->altWeaponSideIndex = playerState->activeAltGunController->weaponSideIndex;
    outData->primaryWeaponBankIndex = playerState->activePrimaryGunController->weaponBankIndex;
    outData->primaryWeaponSideIndex = playerState->activePrimaryGunController->weaponSideIndex;
    outData->playerStatusMeterRatio = g_PlayerStatusMeterRatio;
    outData->hudCounterValue = g_Player_HudCounterValue;
    outData->amphibUnlocked = playerState->amphibUnlocked;
    outData->hoverUnlocked = playerState->hoverUnlocked;
    outData->subUnlocked = playerState->subUnlocked;
    outData->aiMode = playerState->aiMode;
    outData->nextModeSwitchAllowedTime = playerState->nextModeSwitchAllowedTime;
    outData->motionInput = playerState->motionInput;
    outData->autoTurnSign = playerState->autoTurnSign;
    outData->bankInput = playerState->bankInput;
    outData->playerMasterType = masterModalData->masterType;

    zClass_Camera::gwCameraGetTarget(
        g_MainCamera,
        &outData->cameraTarget.x,
        &outData->cameraTarget.y,
        &outData->cameraTarget.z
    );
    zClass_Camera::gwCameraGetPosition(
        g_MainCamera,
        &outData->cameraPosition.x,
        &outData->cameraPosition.y,
        &outData->cameraPosition.z
    );

    memcpy(
        &outData->timedHitStatus,
        &playerState->timedHitStatus,
        sizeof(outData->timedHitStatus)
    );

    if ((playerState->timedHitStatus.runtimeFlags & 1) != 0) {
        outData->timedHitStatus.lightNode = 0;
        outData->timedHitStatus.nextUpdateTime -= g_Time_AccumulatedTimeSec;
        outData->timedHitStatus.savedHitSourceEntryId =
            playerState->timedHitStatus.hitSource->ordinalIndex;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applymissionsavedata
 * @recoil-artifact defines .text recoil:function:0x41f1d0: Player::ApplyMissionSaveData
 * Purpose: restore the live local-player mission state from the save-section payload.
 */
void __fastcall ApplyMissionSaveData(
    PlayerMissionSaveData *saveData
) {
    if (saveData->size != sizeof(PlayerMissionSaveData) &&
        saveData->size != kPlayerMissionSaveLegacySize) {
        zError::ReportOld(
            0x200,
            g_Player_SourceFile_PlayerCpp,
            0xd1,
            g_Player_SaveDataModifiedMsg
        );
        return;
    }

    zUtil_SaveGameState *const saveState = g_LocalPlayerSaveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int hasTimedHitStatus = saveData->size == sizeof(PlayerMissionSaveData);

    PlayerGunFireController *const oldAltController = playerState->activeAltGunController;
    PlayerGunFireController *const oldPrimaryController = playerState->activePrimaryGunController;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        const PlayerMissionSaveWeaponBank *const savedBank = &saveData->weaponBank[bankIndex];
        PlayerAltWeaponBank *const bank = &playerState->altWeaponBanks[bankIndex];

        bank->selectedSide = savedBank->selectedSide;
        PlayerRestoreSavedWeaponSide(
            &bank->controllerA,
            &savedBank->sides[0]
        );
        PlayerRestoreSavedWeaponSide(
            &bank->controllerB,
            &savedBank->sides[1]
        );
        PlayerRefreshSavedWeaponBankHud(
            bankIndex,
            bank
        );
    }

    PlayerAltWeaponBank *const altBank = &playerState->altWeaponBanks[saveData->altWeaponBankIndex];
    PlayerGunFireController *const newAltController =
        PlayerSavedWeaponController(
            altBank,
            saveData->altWeaponSideIndex
        );
    playerState->activeAltGunController = newAltController;
    if (oldAltController != newAltController) {
        ApplyAltWeaponSwitch(
            saveState,
            oldAltController,
            newAltController
        );
        PlayerRefreshPreviousWeaponControllerHud(oldAltController);
    } else {
        ApplyAltWeaponSwitch(
            saveState,
            0,
            newAltController
        );
    }
    HudUiMessage::UpdateSelectedWeaponDisplay(
        newAltController->weaponBankIndex,
        newAltController->weaponSideIndex,
        newAltController->ammoOrCharge
    );

    PlayerAltWeaponBank *const primaryBank =
        &playerState->altWeaponBanks[saveData->primaryWeaponBankIndex];
    PlayerGunFireController *const newPrimaryController =
        PlayerSavedWeaponController(
            primaryBank,
            saveData->primaryWeaponSideIndex
        );
    playerState->activePrimaryGunController = newPrimaryController;
    if (oldPrimaryController != newPrimaryController) {
        ApplyPrimaryWeaponSwitch(
            saveState,
            oldPrimaryController,
            newPrimaryController
        );
        PlayerRefreshPreviousWeaponControllerHud(oldPrimaryController);
    } else {
        ApplyPrimaryWeaponSwitch(
            saveState,
            0,
            newPrimaryController
        );
    }
    HudUiMessage::UpdateSelectedWeaponDisplay(
        newPrimaryController->weaponBankIndex,
        newPrimaryController->weaponSideIndex,
        newPrimaryController->ammoOrCharge
    );

    HudUiMgrSensor::SetShieldMessageRatio(
        playerState->statusMeterValue / playerState->masterCommonData->maxHealth
    );
    HudUiMgr::SetNanitePanelCount(playerState->nanitePanelLevel);

    g_PlayerStatusMeterRatio = saveData->playerStatusMeterRatio;
    g_Player_HudCounterValue = saveData->hudCounterValue;
    playerState->amphibUnlocked = saveData->amphibUnlocked;
    playerState->hoverUnlocked = saveData->hoverUnlocked;
    playerState->subUnlocked = saveData->subUnlocked;
    playerState->aiMode = saveData->aiMode;
    playerState->nextModeSwitchAllowedTime = saveData->nextModeSwitchAllowedTime;
    playerState->motionInput = saveData->motionInput;
    playerState->autoTurnSign = saveData->autoTurnSign;
    playerState->bankInput = saveData->bankInput;

    HudUiMgrObjective::RefreshCounterText(g_Player_HudCounterValue);
    ApplyMasterTypeTransition(
        saveState,
        saveData->playerMasterType,
        1
    );
    playerState->primaryGunGateUntilTime = 0.0f;

    zClass_Camera::gwCameraSetTarget(
        g_MainCamera,
        saveData->cameraTarget.x,
        saveData->cameraTarget.y,
        saveData->cameraTarget.z
    );
    zClass_Camera::gwCameraSetPosition(
        g_MainCamera,
        saveData->cameraPosition.x,
        saveData->cameraPosition.y,
        saveData->cameraPosition.z
    );

    zUtil_PlayerStateStorage *const activePlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
    activePlayerState->timedHitStatus.ClearLightAndReset();
    playerState->damageProtectionActive = 0;
    if (hasTimedHitStatus != 0) {
        memcpy(
            &playerState->timedHitStatus,
            &saveData->timedHitStatus,
            sizeof(saveData->timedHitStatus)
        );
        playerState->timedHitStatus.lightParentNode = playerState->rootNode;

        if ((playerState->timedHitStatus.runtimeFlags & kPlayerTimedHitStatusActiveFlag) != 0) {
            OptCatalogEntryDef *const hitSource =
                OptCatalog::FindEntryById(saveData->timedHitStatus.savedHitSourceEntryId);
            playerState->timedHitStatus.hitSource = hitSource;
            HitSource::UpdateTimedStatus(
                hitSource,
                &playerState->timedHitStatus,
                0.0f
            );
            playerState->timedHitStatus.nextUpdateTime += g_Time_AccumulatedTimeSec;
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-registersections
 * @recoil-artifact defines .text recoil:function:0x41f5b0: Player::ZAR_RegisterSections
 * BN evidence: resets g_Player_RuntimeInputFlags and registers VehicleList and
 * Player callbacks through zUtil_ZAR::RegisterSectionHandler with sort orders 100
 * and 200.
 * Purpose: install Player-owned ZAR section callbacks for save/load.
 */
void ZAR_RegisterSections() {
    g_Player_RuntimeInputFlags = 0;
    zUtil_ZAR::RegisterSectionHandler(
        g_Player_SaveVehicleListSectionName,
        (zZbdSectionCallback)(&ZAR_WriteVehicleListSection),
        (zZbdSectionCallback)(&ZAR_ReadVehicleListSection),
        100,
        0
    );
    zUtil_ZAR::RegisterSectionHandler(
        g_HudUiCounterText_PlayerLabel,
        (zZbdSectionCallback)(&ZAR_WriteMissionSaveDataSection),
        (zZbdSectionCallback)(&ZAR_ReadMissionSaveDataSection),
        200,
        0
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-writemissionsavedatasection
 * @recoil-artifact defines .text recoil:function:0x41f5f0: Player::ZAR_WriteMissionSaveDataSection
 * BN evidence: __fastcall ZAR pre-load callback; builds PlayerMissionSaveData,
 * copies g_Player_LastValidCameraVariantTag as one packed zTag4 value, and writes
 * a 0x140-byte blob under the local player's root-node name.
 * Purpose: serialize local-player mission state into the Player ZAR section.
 */
int __fastcall ZAR_WriteMissionSaveDataSection(
    zZbdSectionCallbackCtx *writer,
    void *
) {
    PlayerMissionSaveData missionData;
    zUtil_PlayerStateStorage *const playerState = g_LocalPlayerSaveState->playerState;

    BuildMissionSaveData(&missionData);
    missionData.lastValidCameraVariantTag = g_Player_LastValidCameraVariantTag;
    return zUtil_ZAR::WriteSectionBlob(
        writer,
        playerState->rootNode->name,
        &missionData,
        sizeof(missionData)
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-readmissionsavedatasection
 * @recoil-artifact defines .text recoil:function:0x41f640: Player::ZAR_ReadMissionSaveDataSection
 * BN evidence: __fastcall ZAR data-ready callback; applies PlayerMissionSaveData,
 * copies lastValidCameraVariantTag to g_Player_LastValidCameraVariantTag, refreshes
 * HUD/layout state, and restores recorded node flags.
 * Purpose: restore local-player mission state from the Player ZAR section.
 */
void __fastcall ZAR_ReadMissionSaveDataSection(
    zZbdSectionCallbackCtx *,
    const char *,
    PlayerMissionSaveData *saveData,
    unsigned int,
    void *
) {
    zUtil_PlayerStateStorage *const playerState = g_LocalPlayerSaveState->playerState;

    ApplyMissionSaveData(saveData);
    g_Player_LastValidCameraVariantTag = saveData->lastValidCameraVariantTag;

    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        zEffect_Anim::NodeActionCallback(
            playerState->destroyedRespawnFxEntry,
            playerState->rootNode
        );
    }

    RefreshHudFromState((zUtil_SaveGameState *)(g_GameStateOrMapTable));
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    RestoreRecordedNodeFlags();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-writevehiclelistsection
 * @recoil-artifact defines .text recoil:function:0x41f6a0: Player::ZAR_WriteVehicleListSection
 * BN evidence: __fastcall ZAR pre-load callback; walks g_PlayerSaveStateListHead,
 * fills the 0x80-byte PlayerVehicleListSaveEntry from typed player-state fields,
 * and writes each blob under the player's root-node name.
 * Purpose: serialize all active player vehicle records into the VehicleList ZAR section.
 */
int __fastcall ZAR_WriteVehicleListSection(
    zZbdSectionCallbackCtx *writer,
    void *
) {
    int writeOk = 1;
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0 && writeOk != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        PlayerVehicleListSaveEntry vehicleRecord;
        vehicleRecord.size = 128;
        vehicleRecord.worldPos = playerState->worldPos;
        vehicleRecord.vehicleRotationAngles = playerState->vehicleRotationAngles;
        vehicleRecord.aiNetId = playerState->aiNetId;
        vehicleRecord.aiTopLevelState = playerState->aiTopLevelState;
        vehicleRecord.aiSavedTopLevelState = playerState->aiSavedTopLevelState;
        vehicleRecord.aiReturnTopLevelState = playerState->aiReturnTopLevelState;
        vehicleRecord.aiAttackRadiusSq = playerState->aiAttackRadiusSq;
        vehicleRecord.aiRestoreDistanceSq = playerState->aiRestoreDistanceSq;
        vehicleRecord.aiRestoreTarget = playerState->aiRestoreTarget;
        vehicleRecord.aiDynamicOffsetDir = playerState->aiDynamicOffsetDir;
        vehicleRecord.aiActivationRadiusSq = playerState->aiActivationRadiusSq;
        vehicleRecord.aiTickSuppressed = playerState->aiTickSuppressed;
        vehicleRecord.aiAlertFlag = playerState->recentHitFlag;
        vehicleRecord.aiStateMarkerHandle = playerState->recentHitMarkerHandle;
        vehicleRecord.aiActive = playerState->aiActive;
        vehicleRecord.aiPathCursorAdvanceRequested = playerState->aiPathCursorAdvanceRequested;
        vehicleRecord.aiCurrentSteeringSubstate = playerState->aiCurrentSteeringSubstate;
        vehicleRecord.aiReturnSteeringSubstate = playerState->aiReturnSteeringSubstate;
        vehicleRecord.masterType = playerState->masterType;
        vehicleRecord.statusMeterScaled = playerState->statusMeterScaled;
        vehicleRecord.statusMeterValue = playerState->statusMeterValue;
        vehicleRecord.nanitePanelLevel = playerState->nanitePanelLevel;
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            vehicleRecord.localMasterType =
                saveState->primaryModalState->masterModalData->masterType;
        }

        writeOk = zUtil_ZAR::WriteSectionBlob(
            writer,
            playerState->rootNode->name,
            &vehicleRecord,
            sizeof(vehicleRecord)
        );
        saveState = saveState != 0 ? saveState->next : 0;
    }

    return writeOk;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-zar-readvehiclelistsection
 * @recoil-artifact defines .text recoil:function:0x41f850: Player::ZAR_ReadVehicleListSection
 * BN evidence: __fastcall ZAR data-ready callback; validates the 0x80-byte
 * VehicleList record, finds the save state by root-node token, restores pose,
 * AI, status, visual, and lifecycle fields, and refreshes node state.
 * Purpose: restore one player vehicle record from the VehicleList ZAR section.
 */
void __fastcall ZAR_ReadVehicleListSection(
    zZbdSectionCallbackCtx *,
    const char *sectionToken,
    PlayerVehicleListSaveEntry *saveData,
    unsigned int,
    void *
) {
    if (saveData->size != 128) {
        zError::ReportOld(
            0x200,
            g_Player_SourceFile_PlayerCpp,
            419,
            g_Player_VehicleSaveDataModifiedMsg
        );
        return;
    }

    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (strcmp(
            playerState->rootNode->name,
            sectionToken
        ) == 0) {
            break;
        }
        saveState = saveState->next;
    }

    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int restoreHealthyNode = playerState->lifecycleState == kPlayerLifecycleInactive &&
                                           saveData->masterType != kPlayerLifecycleInactive
                                       ? 1
                                       : 0;

    playerState->projectileSpawnVel = zVec3_Make(
        0.0f,
        0.0f,
        0.0f
    );
    playerState->localVel = zVec3_Make(
        0.0f,
        0.0f,
        0.0f
    );
    playerState->yawRotatedLocalVel = zVec3_Make(
        0.0f,
        0.0f,
        0.0f
    );
    playerState->worldPos = saveData->worldPos;
    playerState->vehicleRotationAngles = saveData->vehicleRotationAngles;
    playerState->aiNetId = saveData->aiNetId;
    playerState->aiTopLevelState = saveData->aiTopLevelState;
    playerState->aiSavedTopLevelState = saveData->aiSavedTopLevelState;
    playerState->aiReturnTopLevelState = saveData->aiReturnTopLevelState;

    const float now = g_Time_AccumulatedTimeSec;
    playerState->aiStateUntilTime = now;
    playerState->aiHideTime0 = now;
    playerState->aiHideTime1 = now;
    playerState->unknown_0fa4 = now;
    playerState->aiStateStartTime = now;
    playerState->aiStateEndTime = playerState->aiMode2AttackDwell + now;

    playerState->aiAttackRadiusSq = saveData->aiAttackRadiusSq;
    playerState->aiRestoreDistanceSq = saveData->aiRestoreDistanceSq;
    playerState->aiRestoreTarget = saveData->aiRestoreTarget;
    playerState->aiDynamicOffsetDir = saveData->aiDynamicOffsetDir;
    playerState->unknown_0fd0 = now;
    playerState->aiActivationRadiusSq = saveData->aiActivationRadiusSq;
    playerState->aiTickSuppressed = saveData->aiTickSuppressed;
    playerState->recentHitFlag = saveData->aiAlertFlag;
    playerState->recentHitMarkerHandle = saveData->aiStateMarkerHandle;
    playerState->aiActive = saveData->aiActive;
    playerState->aiPathCursorAdvanceRequested = saveData->aiPathCursorAdvanceRequested;
    playerState->aiCurrentSteeringSubstate = saveData->aiCurrentSteeringSubstate;
    playerState->aiReturnSteeringSubstate = saveData->aiReturnSteeringSubstate;
    playerState->lifecycleState = saveData->masterType;
    playerState->statusMeterScaled = saveData->statusMeterScaled;
    playerState->statusMeterValue = saveData->statusMeterValue;
    playerState->nanitePanelLevel = saveData->nanitePanelLevel;

    SetWorldPoseAndRestartAnchor(
        saveState,
        &playerState->worldPos,
        playerState->restartYawRad
    );

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        TickMasterTypeAndForceFeedback(saveState);
    }

    AINet::AiDiscardNegativeBranchPathNodes(saveState);
    playerState->aiCurrentPathNode = (AINetNode *)playerState->aiUnknown_0f7c;
    playerState->aiCurrentPathNeighborIndex = 0;

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable && restoreHealthyNode != 0) {
        zClass_NodePartial *const healthyNode =
            zClass_Class::FindNodeRecursiveByName(
                playerState->rootNode,
                g_Player_HealthySubNodeName
            );
        if (healthyNode != 0) {
            zClass_Object3D::gwObject3DSetPosition(
                healthyNode,
                0.0f,
                0.0f,
                0.0f
            );
            zClass_Object3D::gwObject3DSetRotation(
                healthyNode,
                0.0f,
                0.0f,
                0.0f
            );
        }

        if (playerState->destroyedRespawnAsyncHandle != 0) {
            zEffect_Anim::NodeActionCallback(
                playerState->destroyedRespawnAsyncHandle,
                0
            );
        } else {
            zEffect_Anim::NodeActionCallback(
                playerState->destroyedRespawnFxEntry,
                playerState->rootNode
            );
        }
    }

    zClass_Class::gwNodeSetActive(
        playerState->rootNode,
        playerState->lifecycleState == kPlayerLifecycleInactive ? 0 : 1
    );
    zClass_Node::LoadFlagBit8MaterialImagesAndTexturePack(playerState->rootNode);
    zTag4::Clear(&playerState->variantTag);
    zClass_Class::gwNodeSetNodeType(
        playerState->rootNode,
        playerState->variantTag.tags[0]
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-shutdownmissionruntime
 * @recoil-artifact defines .text recoil:function:0x41fb80: Player::ShutdownMissionRuntime
 * Source file: D:\Proj\Battlesport\player.cpp.
 * Purpose: Clear mission-owned player runtime lists, AI net state, and pass-3 UI links.
 */
void __cdecl ShutdownMissionRuntime() {
    while (g_PlayerSaveStateListHead != 0) {
        DestroySaveGameState(g_PlayerSaveStateListHead);
    }

    DeleteRemainingTrackNodes();

    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateListTail = 0;
    g_PlayerSaveStateListHead = 0;
    g_PlayerSaveStateCount = 0;
    while (saveState != 0) {
        zUtil_SaveGameState *const next = saveState->next;
        saveState->FreeOwnedResources();
        ::operator delete(saveState);
        saveState = next;
    }

    PlayerMasterCommonData *commonData = g_PlayerMasterCommonDataHead;
    while (commonData != 0) {
        DeleteWeaponSpecs(commonData);
        commonData = commonData->next;
    }

    commonData = g_PlayerMasterCommonDataHead;
    while (commonData != 0) {
        PlayerMasterCommonData *const next = commonData->next;
        ::operator delete(commonData);
        commonData = next;
    }

    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataTail = 0;
    g_PlayerMasterCommonDataHead = 0;
    g_PlayerMasterCommonDataCount = 0;

    PlayerMasterModalData *modalData = g_PlayerMasterModalDataHead;
    while (modalData != 0) {
        PlayerMasterModalData *const next = modalData->next;
        ::operator delete(modalData);
        modalData = next;
    }

    g_PlayerMasterModalDataListAux = 0;
    g_PlayerMasterModalDataTail = 0;
    g_PlayerMasterModalDataHead = 0;
    g_PlayerMasterModalDataCount = 0;

    AINet::FreeAll();
    g_Player_NextOrdinal = 0;
    g_GameStateOrMapTable = 0;
    ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->RemoveChild(&g_Player_UnderwaterFxPass3Ui);
    ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->RemoveChild(&g_Player_State7FxPass3Ui);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-destroysavegamestate
 * @recoil-artifact defines .text recoil:function:0x41fd20: Player::DestroySaveGameState
 * Source file: D:\Proj\Battlesport\player.cpp.
 * Purpose: Tear down a mission save state, its sensor track node, and owned resources.
 */
void __fastcall DestroySaveGameState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    FreeAltWeaponTrailRuntimeStates(saveState);
    zClass_Node::ClearDamageHandler(playerState->rootNode);

    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(playerState->rootNode->callbackContext);
    if (trackNode != 0) {
        RemoveTrackNode(trackNode);
        free(trackNode);
    }

    if (saveState != 0) {
        UnlinkSaveState(saveState);
        saveState->FreeOwnedResources();
        ::operator delete(saveState);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-getaivzrdpath
 * @recoil-artifact defines .text recoil:function:0x41fe40: Player::GetAivZrdPath.
 * Purpose: return the static player AIV archive path used by mission
 * bootstrap.
 */
const char *GetAivZrdPath() {
    return g_Player_AivZrdPath;
}
} // namespace Player
namespace zVehicle {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zvehicle-selectzrdbydifficulty
 * @recoil-artifact defines .text recoil:function:0x41fe50: zVehicle::SelectZrdByDifficulty.
 * Purpose: select the difficulty-specific vehicle ZRD archive, falling back
 * to the default archive when the selected path is unavailable.
 */
const char *__fastcall SelectZrdByDifficulty(
    const char *extraSearchPath
) {
    const char *filename = g_Player_VehicleArchiveName_Default;
    const int difficultyMode = zOpt::GetGameDifficultyMode();
    if (difficultyMode == 0) {
        filename = g_Player_VehicleArchiveName_Easy;
    } else if (difficultyMode == 2) {
        filename = g_Player_VehicleArchiveName_Hard;
    }

    if (zReader::TryResolvePath(
        filename,
        extraSearchPath
    ) == 0) {
        filename = g_Player_VehicleArchiveName_Default;
    }

    return filename;
}
} // namespace zVehicle
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initmissionruntimefromworldandcamera
 * @recoil-artifact defines .text recoil:function:0x41fe90: Player::InitMissionRuntimeFromWorldAndCamera
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: initialize mission player runtime from world/camera nodes, attach
 * one-time HUD panels, load player/vehicle tuning, create the stealth
 * save-state, and continue into AIV/local-player bootstrap when aiv.zrd loads.
 * Source owner: battlesport_gameplay.player_mission_runtime_bootstrap.
 * BN evidence: current assembly writes the first-run HUD gate, world/camera
 * globals, camera-zone defaults and player.zrd overrides, runtime input flags,
 * stealth save-state pointer, AIV parent-dir buffer, and the missing-aiv.zrd
 * early return path at 0x420870.
 */
void __fastcall InitMissionRuntimeFromWorldAndCamera(
    zClass_NodePartial *worldNode,
    zClass_NodePartial *cameraNode
) {
    if (g_Player_MissionInitFirstRunFlag != 0) {
        g_HudUiTopMessageStack->AddChild((HudUiElement *)(&g_Player_TopMsgPanel1));
        g_HudUiTopMessageStack->AddChild((HudUiElement *)(&g_Player_TopMsgPanel2));
        g_Player_MissionInitFirstRunFlag = 0;
    }

    if (zOpt::GetNetworkEnabled() == 0) {
        AINet::LoadAllFromZrd();
    }

    g_Player_TopMsgPanel1.SetTextFmt(zLoc::GetMessageString(0x909));
    ((HudUiElement *)(&g_Player_TopMsgPanel1))->x = 55;
    ((HudUiElement *)(&g_Player_TopMsgPanel1))->y = 66;
    g_Player_TopMsgPanel1.Invalidate();
    SetHudPanelVisible(
        &g_Player_TopMsgPanel1,
        0
    );

    g_Player_TopMsgPanel2.SetTextFmt(zLoc::GetMessageString(0x910));
    ((HudUiElement *)(&g_Player_TopMsgPanel2))->x = 55;
    ((HudUiElement *)(&g_Player_TopMsgPanel2))->y = 66;
    g_Player_TopMsgPanel2.Invalidate();
    SetHudPanelVisible(
        &g_Player_TopMsgPanel2,
        0
    );

    ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->AddChild(&g_Player_UnderwaterFxPass3Ui);
    SetHudUiElementVisible(
        &g_Player_UnderwaterFxPass3Ui,
        0
    );
    ((HudUiContainer *)(&g_zVideo_FxPass3ConfigLocal))->AddChild(&g_Player_State7FxPass3Ui);
    SetHudUiElementVisible(
        &g_Player_State7FxPass3Ui,
        0
    );

    g_Player_RuntimeDiScene = worldNode;
    g_MainCamera = cameraNode;
    g_Player_HorizonNode = zClass_Class::FindSubNodeByName(
        g_HudSensorTracker.worldNode,
        g_Player_NodeName_Horizon
    );

    float fovX = 0.0f;
    float fovY = 0.0f;
    zClass_Camera::gwCameraGetFOV(
        g_MainCamera,
        &fovX,
        &fovY
    );
    zClass_Camera::gwCameraSetPosition(
        g_MainCamera,
        0.0f,
        0.0f,
        0.0f
    );
    zClass_Camera::gwCameraSetTarget(
        g_MainCamera,
        0.0f,
        0.0f,
        0.0f
    );
    if (g_Player_HorizonNode != 0) {
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        zClass_Object3D::gwObject3DSetPosition(
            g_Player_HorizonNode,
            0.0f,
            0.0f,
            0.0f
        );
    }

    memset(
        &g_VariantTag_Current,
        0,
        sizeof(g_VariantTag_Current)
    );
    zTag4::Clear(&g_VariantTag_Current);
    g_Variant_CurrentTag = g_VariantTag_Current;

    PlayerInitActionCallbackNode((void *)(&TickAllPlayers));
    PlayerInitActionCallbackNode((void *)(&PickupRespawnQueue::Update));
    PlayerInitActionCallbackNode((void *)(&zClass_Object3D_ModelRefLerpQueue::Update));

    g_Player_LocalControlEnabled = zOpt::GetNetworkEnabled();
    g_Player_TotalTimeSecScaled = g_Time_AccumulatedTimeSec;
    g_Player_CameraZone = 0.899999976f;
    g_Player_CameraZoneInvRange = 10.0f;
    g_Player_CopterSndNode1 = 0;
    g_Player_CopterSndNode2 = 0;
    g_Player_BftSplashAnimEntry = zEffectAnim::FindEntryByName(g_Player_BftSplashAnimName);

    zReader::Node *playerRoot = zReader::LoadNodeFromPath(
        g_Player_ConfigArchiveName,
        0,
        0
    );
    PlayerLoadPlayerZrdTuning(playerRoot);
    zReader::FreeLoadedTree(playerRoot);

    g_Player_RuntimeInputFlags = 3;
    zEffectAnimEntry *asyncEntry = zEffectAnim::FindNextAsyncEntry(0);
    while (asyncEntry != 0) {
        zEffectAnimEntry::SetOnStateDoneCallback(
            asyncEntry,
            (void *)(&AsyncCommandCallback),
            0
        );
        asyncEntry = zEffectAnim::FindNextAsyncEntry(asyncEntry);
    }

    zReader::Node *vehicleRoot =
        zReader::LoadNodeFromPath(
            zVehicle::SelectZrdByDifficulty(0),
            0,
            0
        );
    const int vehicleCount = (PlayerZrdArrayCount(vehicleRoot) - 1) / 2;
    for (int vehicleIndex = 0; vehicleIndex < vehicleCount; ++vehicleIndex) {
        char vehicleName[0x14];
        strcpy(
            vehicleName,
            PlayerZrdArrayString(vehicleRoot, vehicleIndex * 2 + 1)
        );

        PlayerMasterCommonData *const commonData = PlayerAllocMasterCommonData();
        zReader::Node *const vehicleNode = zReader_GetNamedNode(
            vehicleRoot,
            vehicleName
        );
        LoadMasterCommonDataFromNode(
            commonData,
            vehicleNode,
            vehicleName
        );

        for (int modalIndex = 0; modalIndex < commonData->modalCount; ++modalIndex) {
            PlayerMasterModalData *const modalData = PlayerAllocMasterModalData();
            zReader::Node *const modalNode = PlayerZrdArrayNode(
                vehicleNode,
                modalIndex * 2 + 4
            );
            LoadMasterModalDataFromNode(
                modalData,
                modalNode,
                vehicleName
            );
            strcpy(
                commonData->modalNames[modalIndex],
                modalData->modeName
            );
        }
    }

    zUtil_SaveGameState *const stealthSaveState = PlayerAllocLinkedSaveState();
    zUtil_PlayerStateStorage *const stealthPlayerState = stealthSaveState->playerState;
    memset(
        stealthPlayerState,
        0,
        sizeof(*stealthPlayerState)
    );
    PlayerModalState *const stealthModalState =
        (PlayerModalState *)zUtil_SaveGameStateList_AllocAppend(stealthSaveState);
    g_Player2SaveState = stealthSaveState;
    stealthPlayerState->rootNode = zClass_Object3D::gwObject3DInit();
    zClass_Class::gwNodeSetName(
        stealthPlayerState->rootNode,
        g_Player_DisplayName_Stealth
    );
    zClass_Object3D::gwObject3DSetPosition(
        stealthPlayerState->rootNode,
        500.0f,
        50.0f,
        500.0f
    );
    zClass_Object3D::gwObject3DSetRotation(
        stealthPlayerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );
    zClass_Class::gwNodeSetPriority(
        stealthPlayerState->rootNode,
        1
    );
    zClass_Class::gwNodeSetRaycastable(
        stealthPlayerState->rootNode,
        0
    );
    zClass_Class::gwNodeSetCellPickable(
        stealthPlayerState->rootNode,
        0
    );
    zReader_GetNamedNode(
        zReader_GetNamedNode(
            vehicleRoot,
            g_Player_ConfigNode_Stealth
        ),
        g_Player_ConfigNode_CommonMode
    );
    InitStateFromNameAndMasterCommonData(
        stealthSaveState,
        g_Player_ConfigNode_Stealth,
        g_Player_ConfigNode_Stealth
    );
    BindModalStateFromMasterModalData(
        stealthSaveState,
        stealthModalState,
        g_Player_ConfigNode_Stealth,
        g_Player_ConfigNode_Basic
    );
    InitSpawnStateFromPrimaryModalData(stealthSaveState);
    stealthSaveState->firstSaveState->playerState->projectileSpawnVel.x = 0.0f;
    stealthPlayerState->cameraState = zOpt::GetCameraModePlayerState();

    zReader::Node *aivRoot = zReader::LoadNodeFromPath(
        GetAivZrdPath(),
        0,
        0
    );
    if (aivRoot == 0) {
        zError::ReportOld(
            0x800,
            "D:\\Proj\\Battlesport\\player.cpp",
            0x399,
            g_Player_AivArchiveMissingMsg
        );
        return;
    }

    zReader::BuildResolvedParentDir(
        GetAivZrdPath(),
        g_Player_AivParentDir
    );
    int aivCount = (PlayerZrdArrayCount(aivRoot) - 1) / 2;
    if (zOpt::GetNetworkEnabled() != 0) {
        aivCount = 1;
    }

    for (int aivIndex = 0; aivIndex < aivCount; ++aivIndex) {
        char aivName[0x1c];
        char vehicleName[0x14];
        strcpy(
            aivName,
            PlayerZrdArrayString(aivRoot, aivIndex * 2 + 1)
        );
        ExtractVehicleNameFromAivName(
            aivName,
            vehicleName
        );

        if (zReader_GetNamedNode(
            vehicleRoot,
            vehicleName
        ) != 0) {
            zReader::Node *const aivNode = zReader_GetNamedNode(
                aivRoot,
                aivName
            );
            if (aivNode != 0) {
                zReader::Node *const spawnNode = PlayerZrdArrayNode(
                    aivNode,
                    2
                );
                zVec3 spawnPos;
                spawnPos.x = PlayerZrdArrayFloat(
                    spawnNode,
                    1
                );
                spawnPos.y = PlayerZrdArrayFloat(
                    spawnNode,
                    2
                );
                spawnPos.z = PlayerZrdArrayFloat(
                    spawnNode,
                    3
                );
                CreateFromNamesAtPose(
                    &spawnPos,
                    PlayerZrdArrayInt(
                        aivNode,
                        1
                    ),
                    PlayerZrdArrayFloat(
                        aivNode,
                        3
                    ),
                    vehicleName,
                    aivName
                );
            }
        }
    }

    zReader::FreeLoadedTree(vehicleRoot);
    zReader::FreeLoadedTree(aivRoot);

    zUtil_SaveGameState *const headSaveState = g_PlayerSaveStateListHead;
    headSaveState->playerState->lifecycleState = kPlayerLifecycleInactive;
    zUtil_SaveGameState *const localSaveState = headSaveState != 0 ? headSaveState->next : 0;
    g_LocalPlayerSaveState = localSaveState;
    g_CurrentPlayerSaveState = localSaveState;
    localSaveState->playerState->cameraTickEnabled = 1;
    localSaveState->playerState->transitionDamageSuppressed = 0;
    g_VariantTag_Current = localSaveState->playerState->variantTag;
    g_Player_LastValidCameraVariantTag = localSaveState->playerState->variantTag;
    g_Variant_CurrentTag = localSaveState->playerState->variantTag;
    zEffect::SetConditionalRefPos(&localSaveState->playerState->worldPos);
    localSaveState->playerState->lifecycleState = kPlayerLifecycleLocal;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)localSaveState;

    if (zOpt::GetNetworkEnabled() != 0) {
        localSaveState->playerState->amphibUnlocked =
            IsMissionProbeType1EnabledById(g_HudSensorTracker.GetMissionId());
    } else {
        const int missionId = g_HudSensorTracker.GetMissionId();
        if (missionId == 6) {
            localSaveState->playerState->subUnlocked = 1;
        }
        if (missionId >= 4) {
            localSaveState->playerState->hoverUnlocked = 1;
        }
        if (missionId >= 3) {
            localSaveState->playerState->amphibUnlocked = 1;
        }
    }

    stealthPlayerState->worldPos = localSaveState->playerState->worldPos;
    zClass_Object3D::gwObject3DSetPosition(
        stealthPlayerState->rootNode,
        stealthPlayerState->worldPos.x,
        stealthPlayerState->worldPos.y,
        stealthPlayerState->worldPos.z
    );
    stealthPlayerState->vehicleRotationAngles = localSaveState->playerState->vehicleRotationAngles;
    zClass_Object3D::gwObject3DSetRotation(
        stealthPlayerState->rootNode,
        stealthPlayerState->vehiclePitchRad,
        stealthPlayerState->restartYawRad,
        stealthPlayerState->vehicleRollRad
    );

    AINet::BuildAiPeerRingsByAiNetId();
    zInput::BindMap_Current_SetCommandCallback(
        15,
        (zInputCommandCallbackFn)(HandlePrimaryWeaponVariantToggleInput)
    );
    for (int commandId = 16; commandId <= 23; ++commandId) {
        zInput::BindMap_Current_SetCommandCallback(
            commandId,
            (zInputCommandCallbackFn)(HandleAltWeaponBankSelectInput)
        );
    }
    RegisterGameplayCommandCallbacksAndCreateFfEffects();
    zClass_Node::MaskExtraFlagsRecursive(
        g_Player_RuntimeDiScene,
        0
    );
    zReader::LoadMoversFromZrd();
    if (g_HudSensorTracker.raceCheckpointMode != 0) {
        Checkpoint::InstantiateNamedObjects();
    }
}
} // namespace Player
namespace zReader {
/**
 * Purpose: load mover definitions from the current ZRD tree.
 */
void LoadMoversFromZrd() {
    Node *const treeRoot = LoadNodeFromPath(
        "movers.zrd",
        0,
        0
    );
    if (treeRoot == 0) {
        return;
    }

    Node *const rootArray = treeRoot->value.nodes;
    Node *const moverArray = rootArray[1].value.nodes;
    const int moverCount = moverArray[0].value.i32 - 1;
    for (int i = 0; i < moverCount; ++i) {
        zClass_NodePartial *const mover = zClass::FindByTypeAndName(
            6,
            moverArray[i + 1].value.str
        );
        if (mover != 0) {
            zClass_Node::PropagateExtraFlagsRecursive(
                mover,
                1
            );
            zClass_Node::SetContextRecursive(
                mover,
                mover,
                0x200000
            );
            g_Mover_LastLoadedNode = mover;
        }
    }

    FreeLoadedTree(treeRoot);
}
} // namespace zReader
namespace Checkpoint {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-checkpoint-instantiatenamedobjects
 * @recoil-artifact defines .text recoil:function:0x420c60: Checkpoint::InstantiateNamedObjects
 * Purpose: Resolves checkpoint nodes by name and recursively stamps their race
 * checkpoint flags and callback context.
 */
void InstantiateNamedObjects() {
    CString searchName;
    const int checkpointCount = g_HudSensorTracker.checkpointCount;

    for (int checkpointNumber = 1; checkpointNumber <= checkpointCount; ++checkpointNumber) {
        searchName.Format(
            g_Checkpoint_NodeNameFmt,
            checkpointNumber
        );
        zClass_NodePartial *const checkpointNode =
            zClass::FindByTypeAndName(
                6,
                (const char *)searchName
            );
        if (checkpointNode != 0) {
            zClass_Node::PropagateExtraFlagsRecursive(
                checkpointNode,
                kCheckpointNodeAuxFlagTracked
            );
            zClass_Node::PropagateFlagsRecursive(
                checkpointNode,
                kCheckpointNodePickableFlag
            );
            zClass_Node::SetContextRecursive(
                checkpointNode,
                checkpointNode,
                kCheckpointNodeContextFlag
            );
        }
    }
}
} // namespace Checkpoint
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initstatefromnameandmastercommondata
 * @recoil-artifact defines .text recoil:function:0x420d10: Player::InitStateFromNameAndMasterCommonData
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: bind a save-state record to master common data by name and
 * initialize the player's common bootstrap state.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
void __fastcall InitStateFromNameAndMasterCommonData(
    zUtil_SaveGameState *saveState,
    const char *objectName,
    const char *masterCommonDataName
) {
    zUtil_SaveGameState *const localSaveState = GetSaveStateListHead();
    zUtil_PlayerStateStorage *const localPlayerState =
        localSaveState != 0 ? localSaveState->playerState : 0;
    GetSaveStateListHead();

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *commonData = g_PlayerMasterCommonDataHead;
    while (commonData != 0) {
        if (strcmp(
            commonData->vehicleName,
            masterCommonDataName
        ) == 0) {
            playerState->masterCommonData = commonData;
            break;
        }
        commonData = commonData->next;
    }

    if (playerState->masterCommonData == 0) {
        char errorText[0x80];
        sprintf(
            errorText,
            g_Player_MasterCommonDataMissingFmt,
            objectName
        );
        zError::ReportOld(
            0x800,
            "D:\\Proj\\Battlesport\\player.cpp",
            0x46d,
            errorText
        );
    }

    playerState->playerOrdinal = g_Player_NextOrdinal;
    ++g_Player_NextOrdinal;
    if (playerState->playerOrdinal == 1) {
        g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)saveState;
    }

    zClass_Object3D::gwObject3DGetPosition(
        playerState->rootNode,
        &playerState->worldPos.x,
        &playerState->worldPos.y,
        &playerState->worldPos.z
    );
    zClass_Object3D::gwObject3DGetRotation(
        playerState->rootNode,
        &playerState->vehiclePitchRad,
        &playerState->restartYawRad,
        &playerState->vehicleRollRad
    );
    playerState->pitchPoseCache = playerState->vehiclePitchRad;
    playerState->yawPoseCache = playerState->restartYawRad;
    playerState->rollPoseCache = playerState->vehicleRollRad;
    playerState->angVelPitch = 0.0f;
    playerState->angVelYaw = 0.0f;
    playerState->angVelRoll = 0.0f;

    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->previousTransform = playerState->motionBasis;
    RebuildSteerBasisFromMotionBasis(saveState);
    playerState->cameraDirFlat = playerState->steerBasisNorm;

    AINet *aiNet = 0;
    if (playerState->aiNetId != 0) {
        aiNet = AINet::FindByNetId(playerState->aiNetId);
    }
    playerState->lifecycleState = aiNet != 0 ? kPlayerLifecycleAi : kPlayerLifecycleInactive;
    if (playerState->lifecycleState == kPlayerLifecycleAi) {
        playerState->aiNet = aiNet;
        switch (aiNet->aiType) {
        case AINET_TYPE_ST:
            playerState->aiTopLevelState = kPlayerAiTopPathFollow;
            break;
        case AINET_TYPE_HI:
            playerState->aiTopLevelState = kPlayerAiTopTurnTowardTarget;
            break;
        case AINET_TYPE_FI:
            playerState->aiTopLevelState = kPlayerAiTopTurnOnlyTowardTarget;
            break;
        case AINET_TYPE_DE:
            playerState->aiTopLevelState = kPlayerAiTopPathSteering;
            break;
        }

        playerState->aiCurrentSteeringSubstate = aiNet->attackStrategy;
        playerState->aiHideTime0 = aiNet->hideTime0;
        playerState->aiHideTime1 = aiNet->hideTime1;
        playerState->aiCurrentPathNode =
            AINet::FindNearestNode(
                &playerState->worldPos,
                aiNet->nodeListHead
            );
        playerState->aiHomePathNode = playerState->aiCurrentPathNode;

        zClass_NodePartial *const healthyNode =
            zClass_Class::FindSubNodeByName(
                playerState->rootNode,
                g_Player_HealthySubNodeName
            );
        if (healthyNode != 0) {
            zClass_Class::gwNodeSetCellPickable(
                healthyNode,
                0
            );
        }

        if (aiNet->activateRadius != 0.0f) {
            playerState->aiActivationRadiusSq = aiNet->activateRadius * aiNet->activateRadius;
        }
        if (aiNet->attackRadius != 0.0f) {
            playerState->aiAttackRadiusSq = aiNet->attackRadius * aiNet->attackRadius;
        } else {
            playerState->aiAttackRadiusSq = kPlayerDefaultAiAttackRadiusSq;
        }
        if (aiNet->attackDwell != 0.0f) {
            playerState->aiMode2AttackDwell = aiNet->attackDwell;
        } else {
            playerState->aiMode2AttackDwell = kPlayerDefaultAiAttackDwellTime;
        }
        if (aiNet->notPursuitDwell != 0.0f) {
            playerState->aiNotPursuitDwell = aiNet->notPursuitDwell;
        }
        if (aiNet->returnRange != 0.0f) {
            playerState->aiRestoreDistanceSq = aiNet->returnRange * aiNet->returnRange;
        }

        saveState->aiPeerRingNext = saveState;
        playerState->aiStateUntilTime = g_Time_AccumulatedTimeSec + kPlayerAiInitialStateDelaySec;
        playerState->aiStateStartTime = playerState->aiStateUntilTime;
    }

    playerState->regenSkinFxEntry = zEffectAnim::FindEntryByName(g_Player_RegenSkinNodeName);
    playerState->masterTypeTransitionToAmphibNodeAction =
        zEffectAnim::FindEntryByName(g_Player_BoatWakeTrailName);
    playerState->masterTypeTransitionToTrackNodeAction =
        zEffectAnim::FindEntryByName(g_Player_BftExhaustTrailName);
    playerState->shatterVehicleFxEntry = zEffectAnim::FindEntryByName(
        g_Player_ShatterVehicleEffectName
    );
    playerState->shockVehicleFxEntry = zEffectAnim::FindEntryByName(
        g_Player_ShockVehicleEffectName
    );
    playerState->napalmVehicleFxEntry = zEffectAnim::FindEntryByName(
        g_Player_NapalmVehicleEffectName
    );
    playerState->masterTypeTransitionToSubNodeAction = zEffectAnim::FindEntryByName(
        g_Player_BftBubbleEffectName
    );
    playerState->subTransitionFxEntry = zEffectAnim::FindEntryByName(g_Player_NodeName_Subt);

    const int objectIsNetwork = strstr(
        objectName,
        "net"
    ) != 0;
    playerState->destroyedRespawnFxEntry =
        zEffectAnim::FindEntryByName(
            objectIsNetwork != 0 ? g_Player_NodeName_Bft00 : objectName
        );
    if (objectIsNetwork != 0 || strstr(
        objectName,
        g_Player_NodeName_Bft
    ) != 0) {
        playerState->masterTypeTransitionToTrackLightHandle = zEffectAnim::SetVelocity_Thunk(
            playerState->masterTypeTransitionToTrackNodeAction,
            playerState->rootNode,
            0.0f,
            0.0f,
            0.0f
        );
    }
    zEffectAnim::SetVelocity_Thunk(
        zEffectAnim::FindEntryByName(commonData->startAnimsName),
        playerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );

    playerState->cameraState = zOpt::GetCameraModePlayerState();
    playerState->cameraLerpActive = 0;
    playerState->thirdPersonYawOffset = 0.0f;
    playerState->thirdPersonSideOffset = commonData->cambackSide0;
    playerState->thirdPersonBaseYOffset = commonData->cambackBase0;
    playerState->cameraDistance = commonData->cambackDist0;
    playerState->cameraConfigParam0 = commonData->cambackSide1;
    playerState->cameraConfigParam1 = commonData->cambackBase1;
    playerState->cameraConfigParam2 = commonData->cambackDist1;
    playerState->cameraConfigParam3 = commonData->cambackSide2;
    playerState->cameraConfigParam4 = commonData->cambackBase2;
    playerState->cameraConfigParam5 = commonData->cambackDist2;
    playerState->cameraYOffset = commonData->aimYawRate;
    playerState->cameraYOffset = commonData->aimYawMax;
    playerState->cameraState2TargetOffset =
        zVec3_Make(
            0.0f,
            kPlayerCameraState2TargetYOffset,
            0.0f
        );
    playerState->unknown_00d4 = 0;
    playerState->unknown_00d8 = 0;
    playerState->unknown_00dc = 0;
    playerState->unknown_00e0 = 0;
    playerState->altGunAimOrigin = kPlayerDefaultAltGunAimOrigin;
    playerState->activeAltBankIndex = 1;
    playerState->autoTurnActive = 0;
    playerState->cameraTransitionTimer = 0;
    playerState->cameraTransitionBlend = 1.0f;

    zClass_NodePartial *const targetNode =
        zClass_Class::FindSubNodeByName(
            playerState->rootNode,
            g_Player_NodeName_Target
        );
    if (targetNode != 0) {
        zClass_Object3D::gwObject3DGetPosition(
            targetNode,
            &playerState->fxOffsetLocal.x,
            &playerState->fxOffsetLocal.y,
            &playerState->fxOffsetLocal.z
        );
        zClass_Class::gwNodeSetActive(
            targetNode,
            0
        );
    } else {
        playerState->fxOffsetLocal = zVec3_Make(
            0.0f,
            0.0f,
            0.0f
        );
    }
    playerState->fxOffsetWorld.x = playerState->worldPos.x + playerState->fxOffsetLocal.x;
    playerState->fxOffsetWorld.y = playerState->worldPos.y + playerState->fxOffsetLocal.y;
    playerState->fxOffsetWorld.z = playerState->worldPos.z + playerState->fxOffsetLocal.z;

    playerState->bodyNode = zClass_Class::FindSubNodeByName(
        playerState->rootNode,
        g_Player_NodeName_Body
    );
    playerState->turretNode = zClass_Class::FindSubNodeByName(
        playerState->rootNode,
        g_Player_NodeName_Turret
    );
    playerState->doorLeftNode = zClass_Class::FindSubNodeByName(
        playerState->rootNode,
        g_Player_NodeName_DoorLeft
    );
    playerState->doorRightNode =
        zClass_Class::FindSubNodeByName(
            playerState->rootNode,
            g_Player_NodeName_DoorRight
        );
    playerState->modeVariantNode = zClass_Class::FindSubNodeByName(
        playerState->rootNode,
        g_Player_NodeName_Shadow
    );

    CacheGunHardpointsAndDetachDisplays(
        saveState,
        1
    );

    playerState->statusMeterValue = commonData->maxHealth;
    playerState->statusMeterScaled = 1.0f;
    playerState->damageProtectionActive = 0;
    playerState->queuedFixedDamageFlag = 0;
    playerState->recentHitValid = 0;
    playerState->recentHitLightHandle = 0;
    playerState->nanitePanelLevel = 0;

    if (playerState != localPlayerState) {
        HudUiMgrSensorTrackNode *const context =
            HudUiMgrSensor::TrackList_Add(
                HUD_SENSOR_TRACK_KIND_PLAYER,
                saveState
            );
        zClass_Node::SetContextRecursive(
            playerState->rootNode,
            (zClass_NodePartial *)context,
            0x100000
        );
    }

    LoadWeaponBanksAndSelectDefaults(saveState);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-bindmodalstatefrommastermodaldata
 * @recoil-artifact defines .text recoil:function:0x421470: Player::BindModalStateFromMasterModalData
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: bind a modal state to matching master modal data, cache its model
 * nodes, and populate support/collision probe points when needed.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
void __fastcall BindModalStateFromMasterModalData(
    zUtil_SaveGameState *saveState,
    PlayerModalState *modalState,
    const char *objectName,
    const char *modalName
) {
    GetSaveStateListHead();
    GetSaveStateListHead();

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *masterModalData = g_PlayerMasterModalDataHead;
    while (masterModalData != 0) {
        if (strcmp(masterModalData->modalName, playerState->masterCommonData->vehicleName) == 0 &&
            strcmp(
                modalName,
                masterModalData->modeName
            ) == 0) {
            modalState->masterModalData = masterModalData;
            break;
        }
        masterModalData = masterModalData->next;
    }

    if (modalState->masterModalData == 0) {
        char errorText[0x100];
        sprintf(
            errorText,
            g_Player_MasterModalDataMissingFmt,
            objectName
        );
        zError::ReportOld(
            0x800,
            "D:\\Proj\\Battlesport\\player.cpp",
            0x5c3,
            errorText
        );
    }

    zClass_NodePartial *const rootNode = playerState->rootNode;
    modalState->nodeRightMorphs = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_NodeName_RightMorphs
    );
    modalState->nodeLeftMorphs = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_NodeName_LeftMorphs
    );
    modalState->modalNode = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_NodeName_Chassis
    );
    modalState->nodeRTracks = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_NodeName_RightTracks
    );
    modalState->nodeLTracks = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_NodeName_LeftTracks
    );
    modalState->nodeProps = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_NodeName_Props
    );
    modalState->nodeCaustic1 = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_EffectNodeName_Caustic1
    );
    modalState->nodeWake = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_EffectNodeName_Wake
    );
    modalState->nodeSplashL = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_EffectNodeName_SplashLeft
    );
    modalState->nodeSplashR = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_EffectNodeName_SplashRight
    );
    modalState->nodeDustL = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_EffectNodeName_DustLeft
    );
    modalState->nodeDustR = zClass_Class::FindSubNodeByName(
        rootNode,
        g_Player_EffectNodeName_DustRight
    );

    modalState->chassisRollFilterState = 0.0f;
    modalState->chassisPitchFilterState = 0.0f;
    modalState->modalStateCode = 4;

    if (BuildSupportPointsFromModel(saveState, rootNode) == 0 &&
        masterModalData->platformPointCount == 0) {
        char errorText[0x100];
        sprintf(
            errorText,
            g_Player_SupportPointsMissingFmt,
            objectName
        );
        zError::ReportOld(
            0x800,
            "D:\\Proj\\Battlesport\\player.cpp",
            0x5df,
            errorText
        );
    }

    if (masterModalData->probePointCount == 0 &&
        BuildCollisionPointsFromModel(
            saveState,
            rootNode
        ) == 0) {
        char errorText[0x100];
        sprintf(
            errorText,
            g_Player_CollisionPointsMissingFmt,
            objectName
        );
        zError::ReportOld(
            0x800,
            "D:\\Proj\\Battlesport\\player.cpp",
            0x5e6,
            errorText
        );
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-initspawnstatefromprimarymodaldata
 * @recoil-artifact defines .text recoil:function:0x421790: Player::InitSpawnStateFromPrimaryModalData
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: reset spawn-time state from the primary modal data, build world
 * probe-point caches, and align the root node to the sampled surface.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
void __fastcall InitSpawnStateFromPrimaryModalData(
    zUtil_SaveGameState *saveState
) {
    GetSaveStateListHead();
    GetSaveStateListHead();

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    playerState->spawnStateInitialized = 0;
    playerState->primaryGunGateUntilTime = 0.0f;
    playerState->gravityAccel = g_Player_NominalGravity;
    playerState->primaryFireSlotIndex = 0;
    playerState->altFireSlotIndex = 0;

    for (int i = 0; i < masterModalData->probePointCount; ++i) {
        playerState->rootProbeWorldByIndex[i].x =
            masterModalData->probePoints[i].x + playerState->worldPos.x;
        playerState->rootProbeWorldByIndex[i].y =
            masterModalData->probePoints[i].y + playerState->worldPos.y;
        playerState->rootProbeWorldByIndex[i].z =
            masterModalData->probePoints[i].z + playerState->worldPos.z;
    }

    SampleGroundAndAlignRootToSurface(
        saveState,
        1
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-samplegroundandalignroottosurface
 * @recoil-artifact defines .text recoil:function:0x421830: Player::SampleGroundAndAlignRootToSurface
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: sample ground under the player, update the active variant tag, and
 * optionally pitch/roll the root node to the selected surface normal.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
void __fastcall SampleGroundAndAlignRootToSurface(
    zUtil_SaveGameState *saveState,
    int updateRotation
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zTag4::Clear(&playerState->variantTag);
    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetNodeType(
        playerState->rootNode,
        playerState->variantTag.tags[0]
    );
    zClass_Class::gwNodeSetCellPickable(
        playerState->rootNode,
        0
    );

    PlayerProbeSampleCandidateBuffer candidateBuffer = {0};
    zClass_cls_di::BuildPickCandidateListBelowPoint(
        g_Player_RuntimeDiScene,
        &candidateBuffer,
        playerState->worldPos.x,
        500.0f,
        playerState->worldPos.z
    );

    int bestCandidateIndex = 0;
    int selectedImpactSlot = 0;
    float taggedHeight = -300.0f;
    SelectProbeSampleHeightFromCandidates(
        &candidateBuffer,
        &bestCandidateIndex,
        playerState->worldPos.y,
        4.0f,
        playerState->amphibUnlocked == 0,
        &selectedImpactSlot,
        &taggedHeight
    );

    zClass_Class::gwNodeSetCellPickable(
        playerState->rootNode,
        1
    );

    if (candidateBuffer.candidateCount <= 0) {
        zClass_Class::gwNodeSetNodeType(
            playerState->rootNode,
            0xff
        );
        return;
    }

    zClassDiPickCandidateEntry *const selectedCandidate =
        &candidateBuffer.entries[bestCandidateIndex];
    playerState->variantTag = selectedCandidate->variantTag;

    zClass_NodePartial *const worldChild =
        zClass_Class::gwNodeGetWorldChild(selectedCandidate->node);
    const int nodeType =
        worldChild != 0 ? worldChild->nodeType : selectedCandidate->variantTag.tags[0];
    zClass_Class::gwNodeSetNodeType(
        playerState->rootNode,
        nodeType
    );

    if (updateRotation == 0) {
        return;
    }

    playerState->steerBasisRef = selectedCandidate->surfaceNormal;
    zVec3 yawRelativeNormal = selectedCandidate->surfaceNormal;
    RebuildSteerBasisRawFromRef(saveState);
    zMath::Vec3RotateY(
        &yawRelativeNormal,
        &playerState->steerBasisRef,
        -playerState->restartYawRad
    );

    const float pitchAngleRad = (float)(asin(yawRelativeNormal.z));
    float clampedPitchAngleRad = pitchAngleRad;
    if (clampedPitchAngleRad > 0.523599982f) {
        clampedPitchAngleRad = 0.523599982f;
    } else if (clampedPitchAngleRad < -0.523599982f) {
        clampedPitchAngleRad = -0.523599982f;
    }

    const float rollAngleRad = (float)(asin(-yawRelativeNormal.x));
    playerState->vehiclePitchRad = clampedPitchAngleRad;
    playerState->vehicleRollRad = rollAngleRad;
    zClass_Object3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        rollAngleRad
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-clonetype6nodefromtemplateandrename
 * @recoil-artifact defines .text recoil:function:0x421a40: Player::CloneType6NodeFromTemplateAndRename
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: clone a type-6 template node into the runtime scene and give it a
 * new active runtime name.
 * Source owner: Player namespace bootstrap/save-state node creation cluster.
 * BN evidence: finds a type-6 template by name, uses network-enabled for both
 * clone options, clones the node, inserts it into g_Player_RuntimeDiScene,
 * renames it, activates it, and returns the clone or null. BN HLIL currently
 * folds the AddChildAtGrid status branch because the callee decompiles as
 * returning zero; assembly keeps the failure gate before rename.
 */
zClass_NodePartial *__fastcall CloneType6NodeFromTemplateAndRename(
    const char *templateName,
    const char *newName
) {
    zClass_NodePartial *const source = zClass::FindByTypeAndName(
        6,
        templateName
    );
    if (source == 0) {
        return 0;
    }

    const int cloneDiMode = zOpt::GetNetworkEnabled() != 0 ? 1 : 0;
    zClass_NodePartial *const child =
        zClass_cls_util::CopyNodeWithCloneOptions(
            source,
            cloneDiMode,
            cloneDiMode
        );
    if (child == 0) {
        return 0;
    }

    if (zClass_World::AddChildAtGrid(
        g_Player_RuntimeDiScene,
        child
    ) != 0) {
        return 0;
    }

    if (zClass_Class::gwNodeSetName(
        child,
        newName
    ) != 0) {
        return 0;
    }

    zClass_Class::gwNodeSetActive(
        child,
        1
    );
    return child;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-createfromnamesatpose
 * @recoil-artifact defines .text recoil:function:0x421ab0: Player::CreateFromNamesAtPose
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: create and link a player save state from template/object names at
 * the requested spawn pose.
 * Source owner: Player namespace bootstrap/save-state node creation cluster.
 * BN evidence: handles the network bft_00 special clone/rename path, sets the
 * 0x400000 clone-source node flag, allocates and appends a zUtil save state,
 * applies pose/yaw and aiNetId, links the root node, initializes common data,
 * registers local/net hit callbacks, binds modal states, fills destroyed
 * respawn FX, initializes spawn state, and increments the HUD mission stat for
 * non-local states. BN shows the object-name strcmp as an MSVC sbb/sbb idiom
 * and may render the 0x400000 flag as the image base symbol in HLIL.
 */
int __fastcall CreateFromNamesAtPose(
    const zVec3 *spawnPos,
    int aiNetId,
    float yawDeg,
    const char *templateName,
    const char *objectName
) {
    const int objectIsBft00 = strcmp(
        objectName,
        g_Player_NodeName_Bft00
    ) == 0;
    zClass_NodePartial *rootNode = 0;

    if (zOpt::GetNetworkEnabled() != 0 && objectIsBft00 != 0) {
        rootNode = zClass::FindByTypeAndName(
            6,
            g_Player_NodeName_Bft00
        );
        if (rootNode == 0) {
            return 0;
        }

        zClass_NodePartial *const networkClone =
            zClass_cls_util::CopyNodeWithCloneOptions(
                rootNode,
                1,
                1
            );
        if (networkClone != 0) {
            zClass_Class::gwNodeSetName(
                networkClone,
                "bft_99"
            );
        }

        rootNode->flags |= kPlayerNodeFlagNetworkBftCloneSource;
    } else {
        rootNode = zClass::FindByTypeAndName(
            6,
            objectName
        );
        if (rootNode == 0) {
            rootNode = CloneType6NodeFromTemplateAndRename(
                templateName,
                objectName
            );
        }
        if (rootNode == 0) {
            return 0;
        }
    }

    zUtil_SaveGameState *saveState =
        (zUtil_SaveGameState *)(::operator new(sizeof(zUtil_SaveGameState)));
    saveState = zUtil_SaveGameStateList_Init(saveState);
    saveState->next = 0;
    if (g_PlayerSaveStateCount == 0) {
        g_PlayerSaveStateListHead = saveState;
    } else {
        g_PlayerSaveStateListTail->next = saveState;
    }
    g_PlayerSaveStateListTail = saveState;
    saveState->next = 0;
    ++g_PlayerSaveStateCount;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (spawnPos != 0) {
        zClass_Object3D::gwObject3DSetPosition(
            rootNode,
            spawnPos->x,
            spawnPos->y,
            spawnPos->z
        );
        zClass_Object3D::gwObject3DSetRotation(
            rootNode,
            0.0f,
            (float)(yawDeg * 0.017453292519943295),
            0.0f
        );
        playerState->aiNetId = aiNetId;
    }

    if (rootNode->listCountA == 0) {
        zClass_Class::AddChild(
            g_Player_RuntimeDiScene,
            rootNode
        );
    }

    playerState->rootNode = rootNode;
    InitStateFromNameAndMasterCommonData(
        saveState,
        objectName,
        templateName
    );

    if (objectIsBft00 != 0) {
        zClass_Node::SetDamageHitCallback(
            saveState,
            playerState->rootNode,
            (void *)(&EnterDestroyedState)
        );
        g_OptCatalogDamageFeedbackTrackedNode = playerState->rootNode;
        g_Player_LocalFxOffsetWorldPtr = &playerState->fxOffsetWorld;
        zClass_Camera::SetTargetNode(playerState->rootNode);
        g_HudSensorTracker.SetTrackedSaveState(saveState);
        if (zOpt::GetNetworkEnabled() == 0 && OptCatalog_IsDamageMaskEnabled() != 0) {
            zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive(
                playerState->rootNode,
                1
            );
        }
    } else {
        void *callback = (void *)(&HitCallback_RecordContextAndTimedStatus);
        if (strstr(
            objectName,
            "net"
        ) != 0) {
            callback = (void *)(&HitCallback_RecordNetContextAndTimedStatus);
        }
        zClass_Node::SetDamageHitCallback(
            saveState,
            playerState->rootNode,
            callback
        );
    }

    PlayerMasterCommonData *const commonData = playerState->masterCommonData;
    for (int i = 0; i < commonData->modalCount; ++i) {
        PlayerModalState *const modalState =
            (PlayerModalState *)zUtil_SaveGameStateList_AllocAppend(saveState);
        BindModalStateFromMasterModalData(
            saveState,
            modalState,
            objectName,
            commonData->modalNames[i]
        );
    }

    if (playerState->destroyedRespawnFxEntry == 0) {
        playerState->destroyedRespawnFxEntry = zEffectAnim::FindEntryByName(templateName);
    }

    InitSpawnStateFromPrimaryModalData(saveState);
    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ++g_HudSensorTracker.missionStat1;
    }

    return 1;
}
} // namespace Player
namespace zClass_Node {
/**
     * BN evidence: fastcall self/mask, auxFlags at 0x28, signed
     * listCountB at 0x5c, listB at 0x60, recursive self-call only, and no
     * global data references.
     * Purpose: AND a mask into auxFlags across a node's child-list subtree.
     */
    void __fastcall MaskExtraFlagsRecursive(
        zClass_NodePartial * self,
        int mask
    ) {
        self->auxFlags &= mask;

        for (int i = 0; i < self->listCountB; ++i) {
            MaskExtraFlagsRecursive(
                self->listB[i],
                mask
            );
        }
    }
} // namespace zClass_Node
namespace zClass_Node {
/**
     * BN evidence: fastcall self/flags, auxFlags at 0x28, signed
     * listCountB at 0x5c, listB at 0x60, recursive self-call only, and no
     * global data references.
     * Purpose: OR auxFlags into each node in a child-list subtree.
     */
    void __fastcall PropagateExtraFlagsRecursive(
        zClass_NodePartial * self,
        int flags
    ) {
        self->auxFlags |= flags;

        for (int i = 0; i < self->listCountB; ++i) {
            PropagateExtraFlagsRecursive(
                self->listB[i],
                flags
            );
        }
    }
} // namespace zClass_Node
namespace zClass_Node {
/**
     * BN evidence: fastcall self/flags, flags at 0x24, signed listCountB at
     * 0x5c, listB at 0x60, recursive self-call only, and no global data
     * references.
     * Purpose: OR normal node flags into each node in a child-list subtree.
     */
    void __fastcall PropagateFlagsRecursive(
        zClass_NodePartial * self,
        int flags
    ) {
        self->flags |= flags;

        for (int i = 0; i < self->listCountB; ++i) {
            PropagateFlagsRecursive(
                self->listB[i],
                flags
            );
        }
    }
} // namespace zClass_Node
namespace zReader {
/**
 * Purpose: build the parent directory for the currently resolved ZRDR path.
 */
int __fastcall BuildResolvedParentDir(
    const char *filename,
    char *outParentDir
) {
    char fullPath[0x104] = {0};
    _fullpath(
        fullPath,
        TryResolvePath(
            filename,
            0
        ),
        sizeof(fullPath)
    );

    char drive[3] = {0};
    char dir[0x100] = {0};
    char baseName[0x100] = {0};
    char ext[0x100] = {0};
    _splitpath(
        fullPath,
        drive,
        dir,
        baseName,
        ext
    );

    return sprintf(
        outParentDir,
        "%s%s",
        drive,
        dir
    );
}
} // namespace zReader
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-createfromnamesatposegetstate
 * @recoil-artifact defines .text recoil:function:0x421ea0: Player::CreateFromNamesAtPoseGetState
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: create a player from names and return the newly appended save-state
 * tail.
 * Source owner: Player namespace bootstrap/save-state node creation cluster.
 * BN evidence: calls CreateFromNamesAtPose(spawnPos, 0, yawDeg, templateName,
 * objectName), then returns g_PlayerSaveStateListTail on success and null on
 * failure. BN leaves the MSVC neg/sbb/and success-mask expression.
 */
zUtil_SaveGameState *__fastcall CreateFromNamesAtPoseGetState(
    const zVec3 *spawnPos,
    const char *templateName,
    float yawDeg,
    const char *objectName
) {
    if (CreateFromNamesAtPose(
        spawnPos,
        0,
        yawDeg,
        templateName,
        objectName
    ) == 0) {
        return 0;
    }

    return g_PlayerSaveStateListTail;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-buildcollisionpointsfrommodel
 * @recoil-artifact defines .text recoil:function:0x421ed0: Player::BuildCollisionPointsFromModel
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: read collide00..collide11 nodes from the model, deactivate them,
 * and store the reordered probe points in the modal data.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
int __fastcall BuildCollisionPointsFromModel(
    zUtil_SaveGameState *saveState,
    zClass_NodePartial *modelNode
) {
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    zVec3 collisionPoints[12];

    for (int i = 0; i < 12; ++i) {
        char nodeName[0x50];
        sprintf(
            nodeName,
            g_Player_CollisionPointNodeNameFmt,
            i
        );
        zClass_NodePartial *const collisionNode =
            zClass_Class::FindSubNodeByName(
                modelNode,
                nodeName
            );
        if (collisionNode == 0) {
            return 0;
        }

        zClass_Object3D::gwObject3DGetPosition(
            collisionNode,
            &collisionPoints[i].x,
            &collisionPoints[i].y,
            &collisionPoints[i].z
        );
        zClass_Class::gwNodeSetActive(
            collisionNode,
            0
        );
    }

    masterModalData->probePoints[0] = collisionPoints[0];
    masterModalData->probePoints[1] = collisionPoints[1];
    masterModalData->probePoints[2] = collisionPoints[2];
    masterModalData->probePoints[3] = collisionPoints[6];
    masterModalData->probePoints[4] = collisionPoints[7];
    masterModalData->probePoints[5] = collisionPoints[8];
    masterModalData->probePoints[6] = collisionPoints[3];
    masterModalData->probePoints[7] = collisionPoints[4];
    masterModalData->probePoints[8] = collisionPoints[5];
    masterModalData->probePoints[9] = collisionPoints[9];
    masterModalData->probePoints[10] = collisionPoints[10];
    masterModalData->probePoints[11] = collisionPoints[11];
    masterModalData->probePointCount = 12;
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-buildsupportpointsfrommodel
 * @recoil-artifact defines .text recoil:function:0x4220f0: Player::BuildSupportPointsFromModel
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: read support00..support03 nodes from the model, deactivate them,
 * and cache their positions in modal probe-point slots 15..18.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
int __fastcall BuildSupportPointsFromModel(
    zUtil_SaveGameState *saveState,
    zClass_NodePartial *modelNode
) {
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    for (int i = 0; i < 4; ++i) {
        char nodeName[0x50];
        sprintf(
            nodeName,
            g_Player_SupportPointNodeNameFmt,
            i
        );
        zClass_NodePartial *const supportNode =
            zClass_Class::FindSubNodeByName(
                modelNode,
                nodeName
            );
        if (supportNode == 0) {
            return 0;
        }

        zVec3 *const supportPoint = &masterModalData->probePoints[15 + i];
        zClass_Object3D::gwObject3DGetPosition(
            supportNode,
            &supportPoint->x,
            &supportPoint->y,
            &supportPoint->z
        );
        zClass_Class::gwNodeSetActive(
            supportNode,
            0
        );
    }

    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-loadmastercommondatafromnode
 * @recoil-artifact defines .text recoil:function:0x422170: Player::LoadMasterCommonDataFromNode.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Source owner: battlesport_gameplay.player_master_zrd_record_loaders.
 * BN evidence: current decompilation shows fastcall ECX=PlayerMasterCommonData,
 * EDX=vehicle zReader node, stack vehicleName, direct type-4 child-array field
 * reads for the ZRD records, optional common_mode child lookups, sample/pickup
 * provider calls, and PlayerMasterWeaponSpec allocation/linking.
 * Purpose: load one vehicle common-mode master data record from vehicle ZRD.
 */
void __fastcall LoadMasterCommonDataFromNode(
    PlayerMasterCommonData *commonData,
    zReader::Node *vehicleNode,
    const char *vehicleName
) {
    strcpy(
        commonData->vehicleName,
        vehicleName
    );

    commonData->modalCount = ((PlayerZrdArrayCount(vehicleNode) - 1) / 2) - 1;

    zReader::Node *const commonModeNode = zReader_GetNamedNode(
        vehicleNode,
        g_Player_ConfigNode_CommonMode
    );
    zReader::Node *node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_Nanite
    );
    if (node != 0) {
        commonData->naniteBuildRate = PlayerZrdArrayInt(
            node,
            1
        );
        commonData->naniteMaxLevel = PlayerZrdArrayInt(
            node,
            2
        );
    } else {
        commonData->naniteBuildRate = 0;
        commonData->naniteMaxLevel = 0;
    }

    zReader::Node *const soundsNode = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_Sounds
    );
    if (soundsNode != 0) {
        PlayerLoadSoundSample(
            soundsNode,
            g_Player_NodeName_WeaponUp,
            &commonData->sfxWeaponUp[0]
        );
        PlayerLoadSoundSample(
            soundsNode,
            g_Player_NodeName_WeaponSelect,
            &commonData->sfxWeaponUp[2]
        );
        PlayerLoadSoundSample(
            soundsNode,
            g_Player_NodeName_Pinging,
            &commonData->sfxWeaponUp[3]
        );
    }

    node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_Activation
    );
    if (node != 0) {
        const float activationRange = PlayerZrdArrayFloat(
            node,
            1
        );
        commonData->activationRangeSq = activationRange * activationRange;
    } else {
        commonData->activationRangeSq =
            kPlayerDefaultActivationRange * kPlayerDefaultActivationRange;
    }

    node = zReader_GetNamedNode(
        commonModeNode,
        "not_pursuit_dwell"
    );
    commonData->notPursuitDwellTime =
        node != 0 ? PlayerZrdArrayFloat(
            node,
            1
        ) : kPlayerDefaultNotPursuitDwellTime;

    node = zReader_GetNamedNode(
        commonModeNode,
        "return_range"
    );
    if (node != 0) {
        const float returnRange = PlayerZrdArrayFloat(
            node,
            1
        );
        commonData->returnRangeSq = returnRange * returnRange;
    } else {
        commonData->returnRangeSq = kPlayerDefaultReturnRange * kPlayerDefaultReturnRange;
    }

    node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_StartAnims
    );
    if (node != 0) {
        PlayerCopyZrdArrayString(
            commonData->startAnimsName,
            node,
            1
        );
    }

    node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_CamBack
    );
    if (node != 0) {
        zReader::Node *const first = PlayerZrdArrayBase(node)[1].value.nodes;
        zReader::Node *const second = PlayerZrdArrayBase(node)[2].value.nodes;
        zReader::Node *const third = PlayerZrdArrayBase(node)[3].value.nodes;
        commonData->cambackSide0 = first[1].value.f32;
        commonData->cambackBase0 = first[2].value.f32;
        commonData->cambackDist0 = first[3].value.f32;
        commonData->cambackSide1 = second[1].value.f32;
        commonData->cambackBase1 = second[2].value.f32;
        commonData->cambackDist1 = second[3].value.f32;
        commonData->cambackSide2 = third[1].value.f32;
        commonData->cambackBase2 = third[2].value.f32;
        commonData->cambackDist2 = third[3].value.f32;
    } else {
        commonData->cambackSide0 = 0.0f;
        commonData->cambackBase0 = 4.0f;
        commonData->cambackDist0 = 9.0f;
        commonData->cambackSide1 = 0.0f;
        commonData->cambackBase1 = 3.5f;
        commonData->cambackDist1 = 2.25f;
        commonData->cambackSide2 = 0.0f;
        commonData->cambackBase2 = 2.25f;
        commonData->cambackDist2 = 2.25f;
    }

    node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_AimY
    );
    if (node != 0) {
        commonData->aimYawRate = PlayerZrdArrayFloat(
            node,
            1
        );
        commonData->aimYawMax = PlayerZrdArrayFloat(
            node,
            2
        );
    } else {
        commonData->aimYawRate = 3.0f;
        commonData->aimYawMax = 2.0f;
    }

    node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_CameraUdSwing
    );
    if (node != 0) {
        commonData->cameraUdSwing[0] = PlayerZrdArrayFloat(
            node,
            1
        );
        commonData->cameraUdSwing[1] = PlayerZrdArrayFloat(
            node,
            2
        );
        commonData->cameraUdSwing[2] = PlayerZrdArrayFloat(
            node,
            3
        );
        commonData->cameraUdSwing[3] = PlayerZrdArrayFloat(
            node,
            4
        );
    } else {
        commonData->cameraUdSwing[0] = 5.5f;
        commonData->cameraUdSwing[1] = 2.5f;
        commonData->cameraUdSwing[2] = 0.0f;
        commonData->cameraUdSwing[3] = 0.0f;
    }

    node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_TrackSwitch
    );
    if (node != 0) {
        commonData->trackSwitchDist0 = PlayerZrdArrayFloat(
            node,
            1
        );
        commonData->trackSwitchDist1 = PlayerZrdArrayFloat(
            node,
            2
        );
        commonData->trackSwitchDist2 = PlayerZrdArrayFloat(
            node,
            3
        );
    } else {
        commonData->trackSwitchDist0 = 10000.0f;
        commonData->trackSwitchDist1 = 10000.0f;
        commonData->trackSwitchDist2 = 10000.0f;
    }

    node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_Health
    );
    if (node != 0) {
        commonData->maxHealth = zOpt::GetNetworkEnabled() != 0 ? PlayerZrdArrayFloat(node, 2)
                                                               : PlayerZrdArrayFloat(
                                                                   node,
                                                                   1
                                                               );
    } else {
        commonData->maxHealth = kPlayerDefaultMaxHealth;
    }
    commonData->invMaxHealth = 1.0f / commonData->maxHealth;

    node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_Pickups
    );
    if (node != 0) {
        PickupType::FindByLogicalName(
            PlayerZrdArrayString(
                node,
                1
            ),
            &commonData->pickupType
        );
        commonData->pickupCapacity = PlayerZrdArrayInt(
            node,
            2
        );
    } else {
        commonData->pickupType = 0;
        commonData->pickupCapacity = 0;
    }

    node = zReader_GetNamedNode(
        commonModeNode,
        g_Player_NodeName_Weapons
    );
    if (node == 0) {
        return;
    }

    commonData->weaponNodeCount = PlayerZrdArrayCount(node) - 1;
    if (commonData->weaponNodeCount <= 0) {
        return;
    }

    for (int index = 0; index < commonData->weaponNodeCount; ++index) {
        PlayerMasterWeaponSpec *const weaponSpec =
            (PlayerMasterWeaponSpec *)(::operator new(sizeof(PlayerMasterWeaponSpec)));
        memset(
            weaponSpec,
            0,
            sizeof(PlayerMasterWeaponSpec)
        );
        if (commonData->weaponSpecCount == 0) {
            commonData->weaponSpecHead = weaponSpec;
        } else {
            commonData->weaponSpecTail->next = weaponSpec;
        }
        commonData->weaponSpecTail = weaponSpec;
        weaponSpec->next = 0;
        ++commonData->weaponSpecCount;

        zReader::Node *const weaponFields = PlayerZrdArrayBase(node)[index + 1].value.nodes;
        strcpy(
            weaponSpec->optCatalogName,
            weaponFields[1].value.str
        );
        weaponSpec->missionRequirementOrGateId = weaponFields[2].value.i32;
        weaponSpec->mountLayoutFlags = weaponFields[3].value.i32;
        weaponSpec->startAmmoOrCharge = (float)(weaponFields[4].value.i32);
        weaponSpec->dispatchRepeatDelay = weaponFields[5].value.f32;
        weaponSpec->aiAttackRangeMin = weaponFields[6].value.f32;
        weaponSpec->aiAttackRangeMax = weaponFields[7].value.f32;
        weaponSpec->fireSlotRecoilFlags = weaponFields[8].value.i32;
        weaponSpec->initialHardpointSelectState = weaponFields[9].value.i32;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-loadmastermodaldatafromnode
 * @recoil-artifact defines .text recoil:function:0x4226d0: Player::LoadMasterModalDataFromNode.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Source owner: battlesport_gameplay.player_master_zrd_record_loaders.
 * BN evidence: current decompilation shows fastcall ECX=PlayerMasterModalData,
 * EDX=modal zReader node, stack modalName, direct type-4 child-array field
 * reads for modal scalar/list records, two-character mode dispatch, repeated
 * point/FX/wave/sound loader patterns, and no authored globals touched.
 * Purpose: load one modal master data record from a vehicle modal ZRD node.
 */
void __fastcall LoadMasterModalDataFromNode(
    PlayerMasterModalData *modalData,
    zReader::Node *modalNode,
    const char *modalName
) {
    strcpy(
        modalData->modalName,
        modalName
    );

    zReader::Node *node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigNode_Mode
    );
    if (node != 0) {
        PlayerCopyZrdArrayString(
            modalData->modeName,
            node,
            1
        );
    } else {
        strcpy(
            modalData->modeName,
            g_Player_ConfigValue_MasterTypeUnknown
        );
    }

    modalData->masterType = 0;
    if (strncmp(
        modalData->modeName,
        g_Player_ConfigNode_Basic,
        2
    ) == 0) {
        modalData->masterType = 0;
    } else if (strncmp(
        modalData->modeName,
        g_Player_ConfigValue_MasterTypeTrack,
        2
    ) == 0) {
        modalData->masterType = 3;
    } else if (strncmp(
        modalData->modeName,
        g_Player_ConfigValue_MasterTypeHover,
        2
    ) == 0) {
        modalData->masterType = 4;
    } else if (strncmp(
        modalData->modeName,
        g_Player_ConfigValue_MasterTypeAmphib,
        2
    ) == 0) {
        modalData->masterType = 5;
    } else if (strncmp(
        modalData->modeName,
        g_Player_ConfigValue_MasterTypeSub,
        2
    ) == 0) {
        modalData->masterType = 2;
    } else if (strncmp(
        modalData->modeName,
        g_Player_ConfigValue_MasterTypeFly,
        2
    ) == 0) {
        modalData->masterType = 1;
    }

    PlayerLoadModalPointList(
        zReader_GetNamedNode(
            modalNode,
            g_Player_ConfigKey_Platform
        ),
        &modalData->probePoints[15],
        &modalData->platformPointCount
    );
    PlayerLoadModalPointList(
        zReader_GetNamedNode(
            modalNode,
            g_Player_ConfigKey_Collision
        ),
        modalData->probePoints,
        &modalData->probePointCount
    );

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_Rates
    );
    if (node != 0) {
        modalData->accelRate = PlayerZrdArrayFloat(
            node,
            1
        );
        modalData->maxSpeed = PlayerZrdArrayFloat(
            node,
            2
        );
    } else {
        modalData->accelRate = 10.0f;
        modalData->maxSpeed = 30.0f;
    }

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_Friction
    );
    if (node != 0) {
        modalData->frictionStatic = PlayerZrdArrayFloat(
            node,
            1
        );
        modalData->frictionDynamic = PlayerZrdArrayFloat(
            node,
            2
        );
        modalData->frictionSlide = PlayerZrdArrayFloat(
            node,
            3
        );
    } else {
        modalData->frictionStatic = 10000.0f;
        modalData->frictionDynamic = 10.0f;
        modalData->frictionSlide = 0.0f;
    }
    if (modalData->frictionDynamic >= modalData->frictionStatic) {
        modalData->frictionDynamic = modalData->frictionStatic * 0.899999976f;
    }

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_Stopping
    );
    modalData->stoppingForce = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 8.0f;

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_QuicksandSlowdown
    );
    modalData->quicksandSlowdown = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 0.899999976f;

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_LavaSlowdown
    );
    modalData->lavaSlowdown = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 0.800000012f;

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_Turns
    );
    if (node != 0) {
        modalData->yawAccel = PlayerZrdArrayFloat(
            node,
            1
        );
        modalData->yawRateMax = PlayerZrdArrayFloat(
            node,
            2
        );
    } else {
        modalData->yawAccel = 0.600000024f;
        modalData->yawRateMax = 2.0f;
    }

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_TurnDamping
    );
    modalData->yawDamping = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 30.0f;

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_RateDamping
    );
    if (node != 0) {
        modalData->rateDampingAccel = PlayerZrdArrayFloat(
            node,
            1
        );
        modalData->rateDampingDecel = PlayerZrdArrayFloat(
            node,
            2
        );
    } else {
        modalData->rateDampingAccel = 30.0f;
        modalData->rateDampingDecel = 30.0f;
    }

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_AccelDamping
    );
    modalData->aDamping = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 8.0f;

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_AltControl
    );
    if (node != 0) {
        modalData->hoverLiftDampingRate = PlayerZrdArrayFloat(
            node,
            1
        );
        modalData->hoverLiftScale = PlayerZrdArrayFloat(
            node,
            2
        );
        modalData->hoverNormalLerpRate = PlayerZrdArrayFloat(
            node,
            3
        );
    } else {
        modalData->hoverLiftDampingRate = -10.0f;
        modalData->hoverLiftScale = 0.800000012f;
        modalData->hoverNormalLerpRate = -3.0f;
    }

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_Mass
    );
    modalData->mass = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 1.0f;
    modalData->invMass = 1.0f / modalData->mass;

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_ConfigKey_GunPitch
    );
    if (node != 0) {
        modalData->gunPitchMin = PlayerZrdArrayFloat(
            node,
            1
        );
        modalData->gunPitchRate = PlayerZrdArrayFloat(
            node,
            2
        );
    } else {
        modalData->gunPitchMin = -0.2588f;
        modalData->gunPitchRate = 0.5f;
    }

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_NodeName_ModeAlt
    );
    modalData->modeAltTransitionTime = node != 0 ? PlayerZrdArrayFloat(
        node,
        1
    ) : 2.0f;

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_NodeName_ChassisSmooth
    );
    modalData->chassisSmoothFactor = node != 0 ? (float)(fabs(PlayerZrdArrayFloat(
        node,
        1
    ))) : 0.0f;

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_NodeName_ChassisPitch
    );
    if (node != 0) {
        modalData->chassisPitchRate = PlayerZrdArrayFloat(
            node,
            1
        );
        modalData->chassisPitchMax = PlayerZrdArrayFloat(
            node,
            2
        );
        modalData->chassisPitchDamping = (float)(fabs(PlayerZrdArrayFloat(
            node,
            3
        )));
    } else {
        modalData->chassisPitchRate = 0.0f;
        modalData->chassisPitchMax = 0.0f;
        modalData->chassisPitchDamping = 0.0f;
    }

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_NodeName_ChassisRoll
    );
    if (node != 0) {
        modalData->chassisRollRate = PlayerZrdArrayFloat(
            node,
            1
        );
        modalData->chassisRollMax = PlayerZrdArrayFloat(
            node,
            2
        );
        modalData->chassisRollDamping = (float)(fabs(PlayerZrdArrayFloat(
            node,
            3
        )));
    } else {
        // Retail code clears the pitch slots here when chas_roll is absent.
        modalData->chassisPitchRate = 0.0f;
        modalData->chassisPitchMax = 0.0f;
        modalData->chassisPitchDamping = 0.0f;
    }

    PlayerLoadModalWaveParams(
        modalData,
        modalNode,
        g_Player_ConfigKey_AmphibWave
    );
    PlayerLoadModalWaveParams(
        modalData,
        modalNode,
        g_Player_ConfigKey_HoverWave
    );
    PlayerLoadModalWaveParams(
        modalData,
        modalNode,
        g_Player_ConfigKey_SubWave
    );

    node = zReader_GetNamedNode(
        modalNode,
        g_Player_NodeName_CollisionDamage
    );
    if (node != 0) {
        modalData->collisionDampingA = PlayerZrdArrayFloat(
            node,
            1
        );
        modalData->collisionDampingB = PlayerZrdArrayFloat(
            node,
            2
        );
    } else {
        modalData->collisionDampingA = 0.5f;
        modalData->collisionDampingB = 0.150000006f;
    }

    PlayerLoadModalFxList(
        modalNode,
        g_Player_NodeName_T2AAnims,
        modalData->fxList_fromTrackToAmphib
    );
    PlayerLoadModalFxList(
        modalNode,
        g_Player_NodeName_A2TAnims,
        modalData->fxList_fromAmphibToTrack
    );
    PlayerLoadModalFxList(
        modalNode,
        g_Player_NodeName_T2HAnims,
        modalData->fxList_fromTrackToHover
    );
    PlayerLoadModalFxList(
        modalNode,
        g_Player_NodeName_H2TAnims,
        modalData->fxList_fromHoverToTrack
    );
    PlayerLoadModalFxList(
        modalNode,
        g_Player_NodeName_S2AAnims,
        modalData->fxList_fromSubToAmphib
    );
    PlayerLoadModalFxList(
        modalNode,
        g_Player_NodeName_A2SAnims,
        modalData->fxList_fromAmphibToSub
    );
    PlayerLoadModalFxList(
        modalNode,
        g_Player_NodeName_H2AAnims,
        modalData->fxList_fromHoverToAmphib
    );
    PlayerLoadModalFxList(
        modalNode,
        g_Player_NodeName_A2HAnims,
        modalData->fxList_fromAmphibToHover
    );

    zReader::Node *const soundsNode = zReader_GetNamedNode(
        modalNode,
        g_Player_NodeName_Sounds
    );
    if (soundsNode == 0) {
        return;
    }

    PlayerLoadSoundSample(
        soundsNode,
        g_Player_NodeName_Engine,
        &modalData->sfxEngine[0]
    );
    PlayerLoadSoundSample(
        soundsNode,
        g_Player_NodeName_External,
        &modalData->sfxEngine[1]
    );
    PlayerLoadSoundSample(
        soundsNode,
        g_Player_NodeName_Idle,
        &modalData->sfxEngine[2]
    );
    PlayerLoadSoundSample(
        soundsNode,
        g_Player_NodeName_Skid,
        &modalData->sfxEngine[3]
    );
    PlayerLoadSoundSample(
        soundsNode,
        g_Player_NodeName_Collide,
        &modalData->sfxCollide
    );
    PlayerLoadSoundSample(
        soundsNode,
        g_Player_NodeName_Land,
        &modalData->sfxLand
    );

    node = zReader_GetNamedNode(
        soundsNode,
        g_Player_NodeName_PitchScale
    );
    if (node != 0) {
        modalData->sfxPitchScale = PlayerZrdArrayFloat(
            node,
            1
        );
    }

    node = zReader_GetNamedNode(
        soundsNode,
        g_Player_NodeName_VolumeScale
    );
    if (node != 0) {
        modalData->sfxVolumeScale = PlayerZrdArrayFloat(
            node,
            1
        );
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-extractvehiclenamefromaivname
 * @recoil-artifact defines .text recoil:function:0x423150: Player::ExtractVehicleNameFromAivName.
 * Purpose: copy the vehicle-name prefix from an AIV name until the numeric
 * suffix separator.
 */
void __fastcall ExtractVehicleNameFromAivName(
    const char *aivName,
    char *outVehicleName
) {
    int outLen = 0;
    outVehicleName[0] = '\0';
    if (aivName[0] == '\0') {
        return;
    }

    const char *cursor = aivName;
    do {
        if (*cursor == '_' && isdigit(aivName[outLen + 1]) != 0) {
            break;
        }

        outVehicleName[outLen] = *cursor;
        ++outLen;
        outVehicleName[outLen] = '\0';
        ++cursor;
    } while (*cursor != '\0');
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-refreshhudfromstate
 * @recoil-artifact defines .text recoil:function:0x4231b0: Player::RefreshHudFromState.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: refresh the HUD weapon, health, mode, damage, and status displays
 * from the current player save-state fields.
 */
void __fastcall RefreshHudFromState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_SaveGameState *const localSaveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    HudUiMgrSensor::SetShieldMessageRatio(
        playerState->statusMeterValue / localSaveState->playerState->masterCommonData->maxHealth
    );
    HudUiMgr::SetNanitePanelCount(playerState->nanitePanelLevel);

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
        PlayerGunFireController &left = bank.controllerA;
        PlayerGunFireController &right = bank.controllerB;
        const int leftEnabled = (left.flags >> 2) & 1;
        const int rightEnabled = (right.flags >> 2) & 1;

        if (leftEnabled != 0) {
            if (left.ammoOrCharge != 0.0f) {
                HudUiMessage::SelectVariantDisplay(
                    bankIndex,
                    0
                );
                HudUiMessage::SetValueIfOwnerMatches(
                    bankIndex,
                    0,
                    left.ammoOrCharge
                );
                bank.selectedSide = 0;
                if (rightEnabled != 0) {
                    HudUiMessage::ApplySideImageSwap(
                        bankIndex,
                        1
                    );
                }
            } else if (rightEnabled != 0 && right.ammoOrCharge != 0.0f) {
                HudUiMessage::SelectVariantDisplay(
                    bankIndex,
                    1
                );
                HudUiMessage::SetValueIfOwnerMatches(
                    bankIndex,
                    1,
                    right.ammoOrCharge
                );
                bank.selectedSide = 1;
                HudUiMessage::ApplySideImageSwap(
                    bankIndex,
                    0
                );
            }
        } else if (rightEnabled != 0) {
            HudUiMessage::SelectVariantDisplay(
                bankIndex,
                1
            );
            HudUiMessage::SetValueIfOwnerMatches(
                bankIndex,
                1,
                right.ammoOrCharge
            );
            bank.selectedSide = 1;
        } else {
            HudUiMessage::ClearDisplay(bankIndex);
            if (left.ammoOrCharge != 0.0f) {
                HudUiMessage::SetValueIfOwnerMatches(
                    bankIndex,
                    0,
                    left.ammoOrCharge
                );
            } else if (right.ammoOrCharge != 0.0f) {
                HudUiMessage::SetValueIfOwnerMatches(
                    bankIndex,
                    0,
                    right.ammoOrCharge
                );
            }
        }
    }

    HudUiMessage::UpdateSelectedWeaponDisplay(
        0,
        0,
        0.0f
    );

    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activeAltGunController->weaponBankIndex,
        activeAltGunController->weaponSideIndex,
        activeAltGunController->ammoOrCharge
    );

    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activePrimaryGunController->weaponBankIndex,
        activePrimaryGunController->weaponSideIndex,
        activePrimaryGunController->ammoOrCharge
    );

    HudUiMgr::SetModeCounterState(
        1,
        playerState->amphibUnlocked != 0 ? 1 : 0
    );
    HudUiMgr::SetModeCounterState(
        2,
        playerState->hoverUnlocked != 0 ? 1 : 0
    );
    HudUiMgr::SetModeCounterState(
        3,
        playerState->subUnlocked != 0 ? 1 : 0
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-ismissionprobetype1enabledbyid
 * @recoil-artifact defines .text recoil:function:0x423380: Player::IsMissionProbeType1EnabledById
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: identify the mission probe ids that enable type-1 mission probe
 * handling.
 * Source owner: standalone mission probe type predicate leaf, not the Player
 * C++ class.
 * Evidence: retail body is a pure integer predicate over ids 9, 11, 12, and
 * 13 with no calls, globals, object state, or table dispatch.
 */
int __fastcall IsMissionProbeType1EnabledById(
    int missionId
) {
    return missionId == 9 || missionId == 11 || missionId == 12 || missionId == 13;
}
} // namespace Player
/**
 * Purpose: applies the underwater blue-tint pass to the active pass-3 input
 * rectangle through the recovered ApplyPass3 virtual slot.
 */
void Player_UnderwaterFxPass3Ui::ApplyPass3() {
    zVideo_FxSurface::ApplyBlueTintRect((zVidRect32 *)(clipRectOrNull));
}
/**
 * Purpose: applies the projectile-camera green-mask pass to the active pass-3
 * input rectangle through the recovered ApplyPass3 virtual slot.
 */
void Player_ProjectileCameraFxPass3Ui::ApplyPass3() {
    zVideo_FxSurface::ApplyGreenMaskRect((zVidRect32 *)(clipRectOrNull));
}
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-processpendingcontactqueues
 * @recoil-artifact defines .text recoil:function:0x423460: Player::ProcessPendingContactQueues.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ProcessPendingContactQueues from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ProcessPendingContactQueues(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    playerState->pickupQueueProcessed = 0;
    playerState->playerCollisionResolved = 0;
    playerState->worldCollisionResolved = 0;
    playerState->preferredCollisionResolved = 0;
    playerState->checkpointLapProgressNotified = 0;

    ClearPendingContactQueues(saveState);
    BuildPendingContactQueues(saveState);
    if (playerState->noPendingContactsQueued != 0) {
        return;
    }

    if (playerState->checkpointQueue.count != 0) {
        Checkpoint::UpdatePlayerLapProgressAndNotifyNet(
            saveState,
            g_PlayerPendingCheckpointNumber
        );
        playerState->checkpointLapProgressNotified = 1;
    }

    if (playerState->pickupQueue.count != 0) {
        ProcessPendingPickupContacts(saveState);
        playerState->pickupQueueProcessed = 1;
    }

    if (playerState->playerCollisionQueue.count != 0) {
        ResolvePendingPlayerCollisionContact(saveState);
        playerState->playerCollisionResolved = 1;
    }

    if (playerState->worldCollisionQueue.count != 0) {
        ResolvePendingWorldCollisionContact(saveState);
        playerState->worldCollisionResolved = 1;
    }

    if (playerState->transferQueue.count != 0) {
        ProcessTransferContactQueue(saveState);
    }

    if (playerState->preferredCollisionQueue.count != 0) {
        SelectAndResolvePreferredPendingCollisionContact(saveState);
    }

    ClearPendingContactQueues(saveState);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-clearpendingcontactqueues
 * @recoil-artifact defines .text recoil:function:0x423530: Player::ClearPendingContactQueues.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ClearPendingContactQueues from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ClearPendingContactQueues(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    FreePendingContactQueue(&playerState->preferredCollisionQueue);
    FreePendingContactQueue(&playerState->playerCollisionQueue);
    FreePendingContactQueue(&playerState->worldCollisionQueue);
    FreePendingContactQueue(&playerState->pickupQueue);
    FreePendingContactQueue(&playerState->checkpointQueue);
    FreePendingContactQueue(&playerState->transferQueue);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-buildpendingcontactqueues
 * @recoil-artifact defines .text recoil:function:0x4236b0: Player::BuildPendingContactQueues.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::BuildPendingContactQueues from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall BuildPendingContactQueues(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    int enabledSegmentFlags[15];

    const float localVelLengthSq = playerState->localVel.x * playerState->localVel.x +
                                   playerState->localVel.y * playerState->localVel.y +
                                   playerState->localVel.z * playerState->localVel.z;
    playerState->noPendingContactsQueued = 1;
    memset(
        enabledSegmentFlags,
        0,
        sizeof(enabledSegmentFlags)
    );

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        if (fabs(playerState->angVelYaw) > 0.0f) {
            EnableContactSegment(
                enabledSegmentFlags,
                0
            );
            EnableContactSegment(
                enabledSegmentFlags,
                1
            );
            EnableContactSegment(
                enabledSegmentFlags,
                2
            );
            EnableContactSegment(
                enabledSegmentFlags,
                3
            );
            EnableContactSegment(
                enabledSegmentFlags,
                4
            );
            EnableContactSegment(
                enabledSegmentFlags,
                5
            );
        }
    } else if (fabs(playerState->localVel.z) < fabs(playerState->angVelYaw * 3.29999995f)) {
        EnableContactSegment(
            enabledSegmentFlags,
            0
        );
        EnableContactSegment(
            enabledSegmentFlags,
            1
        );
        EnableContactSegment(
            enabledSegmentFlags,
            2
        );
        EnableContactSegment(
            enabledSegmentFlags,
            3
        );
        EnableContactSegment(
            enabledSegmentFlags,
            4
        );
        EnableContactSegment(
            enabledSegmentFlags,
            5
        );
    }

    if (localVelLengthSq > 0.0000001f) {
        if (playerState->localVel.z < 0.0f) {
            EnableContactSegment(
                enabledSegmentFlags,
                0
            );
            EnableContactSegment(
                enabledSegmentFlags,
                1
            );
            EnableContactSegment(
                enabledSegmentFlags,
                2
            );
        } else {
            EnableContactSegment(
                enabledSegmentFlags,
                3
            );
            EnableContactSegment(
                enabledSegmentFlags,
                5
            );
        }

        if (masterModalData->masterType == kPlayerMasterTypeSub &&
            playerState->localVel.y > 0.001f) {
            EnableContactSegment(
                enabledSegmentFlags,
                0
            );
            EnableContactSegment(
                enabledSegmentFlags,
                1
            );
            EnableContactSegment(
                enabledSegmentFlags,
                2
            );
            EnableContactSegment(
                enabledSegmentFlags,
                3
            );
            EnableContactSegment(
                enabledSegmentFlags,
                5
            );
        }

        if (playerState->localVel.x > 0.001f) {
            EnableContactSegment(
                enabledSegmentFlags,
                6
            );
            EnableContactSegment(
                enabledSegmentFlags,
                7
            );
            EnableContactSegment(
                enabledSegmentFlags,
                8
            );
            EnableContactSegment(
                enabledSegmentFlags,
                4
            );
        } else {
            if (playerState->localVel.x < -0.001f) {
                EnableContactSegment(
                    enabledSegmentFlags,
                    9
                );
                EnableContactSegment(
                    enabledSegmentFlags,
                    10
                );
                EnableContactSegment(
                    enabledSegmentFlags,
                    11
                );
            }
            EnableContactSegment(
                enabledSegmentFlags,
                4
            );
        }
    }

    BuildModalAndRootProbeWorldCaches(
        playerState,
        masterModalData
    );

    zClass_DiSegmentEndpoints segmentPairs[15];
    int segmentTags[15];
    int segmentCount = 0;
    const float probeYAdvance = playerState->projectileSpawnVel.y * g_Player_DeltaTime;

    for (int i = 0; i < 15; ++i) {
        if (enabledSegmentFlags[i] == 0) {
            continue;
        }

        segmentTags[segmentCount] = i;
        zVec3 *const modalPoint = &playerState->modalProbeWorldByIndex[i];
        zVec3 *const rootPoint = &playerState->rootProbeWorldByIndex[i];
        ConstrainToUnitDistanceFrom(
            rootPoint,
            modalPoint
        );
        segmentPairs[segmentCount].start = *rootPoint;
        segmentPairs[segmentCount].end = *modalPoint;
        segmentPairs[segmentCount].end.y += probeYAdvance;
        ++segmentCount;
    }

    if (segmentCount != 0) {
        playerState->noPendingContactsQueued = CollectPendingContactsForSegments(
            saveState,
            segmentPairs,
            segmentCount * 2,
            segmentTags
        );
    }

    if (masterModalData->masterType != kPlayerMasterTypeSub) {
        return;
    }

    segmentCount = 0;
    for (int subSegmentIndex = 0; subSegmentIndex < 15; ++subSegmentIndex) {
        if (enabledSegmentFlags[subSegmentIndex] == 0) {
            continue;
        }

        segmentPairs[segmentCount].start = playerState->rootProbeWorldByIndex[subSegmentIndex];
        segmentPairs[segmentCount].start.y -= -3.0f;
        segmentPairs[segmentCount].end = playerState->modalProbeWorldByIndex[subSegmentIndex];
        segmentPairs[segmentCount].end.y += probeYAdvance - -3.0f;
        ++segmentCount;
    }

    if (segmentCount != 0) {
        playerState->noPendingContactsQueued = CollectPendingContactsForSegments(
            saveState,
            segmentPairs,
            segmentCount * 2,
            segmentTags
        );
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-collectpendingcontactsforsegments
 * @recoil-artifact defines .text recoil:function:0x423b10: Player::CollectPendingContactsForSegments.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::CollectPendingContactsForSegments from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall CollectPendingContactsForSegments(
    zUtil_SaveGameState *saveState,
    zClass_DiSegmentEndpoints *segmentPairs,
    int endpointCount,
    int *segmentTags
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    g_Variant_CurrentTag = playerState->variantTag;

    PlayerProbeSampleCandidateBuffer hitBatches[24] = {0};
    zClass_cls_di::BuildProbeHitBatchesForSegments(
        g_Player_RuntimeDiScene,
        segmentPairs,
        endpointCount,
        hitBatches
    );

    g_Variant_CurrentTag = g_VariantTag_Current;
    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        1
    );

    for (int endpointIndex = 0; endpointIndex < endpointCount; endpointIndex += 2) {
        const int segmentIndex = endpointIndex >> 1;
        ClassifyPendingContactsForSegment(
            saveState,
            &hitBatches[segmentIndex],
            &segmentPairs[segmentIndex].start,
            &segmentPairs[segmentIndex].end,
            segmentTags[segmentIndex]
        );
    }

    return playerState->preferredCollisionQueue.count == 0 &&
                   playerState->playerCollisionQueue.count == 0 &&
                   playerState->worldCollisionQueue.count == 0 &&
                   playerState->transferQueue.count == 0 &&
                   playerState->checkpointQueue.count == 0 && playerState->pickupQueue.count == 0
               ? 1
               : 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-classifypendingcontactsforsegment
 * @recoil-artifact defines .text recoil:function:0x423c20: Player::ClassifyPendingContactsForSegment.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ClassifyPendingContactsForSegment from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ClassifyPendingContactsForSegment(
    zUtil_SaveGameState *saveState,
    PlayerProbeSampleCandidateBuffer *sceneResults,
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd,
    int segmentTag
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    for (int hitIndex = 0; hitIndex < sceneResults->candidateCount; ++hitIndex) {
        zClassDiPickCandidateEntry *const candidate = &sceneResults->entries[hitIndex];
        zClass_NodePartial *node = candidate->node;
        PlayerPendingContact *queuedContact = 0;

        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            const int checkpointNumber =
                HudSensorTracker::ParseCheckpointNumberFromNode(candidate->node);
            g_PlayerPendingCheckpointNumber = checkpointNumber;
            if (checkpointNumber != 0) {
                queuedContact = AppendPendingContact(&playerState->checkpointQueue);
                CopyPendingContactPayload(
                    queuedContact,
                    candidate,
                    segmentStart,
                    segmentEnd,
                    segmentTag
                );
                continue;
            }
        }

        if ((node->flags & 0x8000000) != 0) {
            continue;
        }

        if (Pickup::ResolveOwnerFromBvolHit(&candidate->node) != 0) {
            queuedContact = AppendPendingContact(&playerState->pickupQueue);
        } else {
            node = candidate->node;
            if ((node->flags & 0x100000) != 0 && node->callbackContext != 0) {
                int *const playerType = (int *)(node->callbackContext);
                if (*playerType == 2) {
                    queuedContact = AppendPendingContact(&playerState->playerCollisionQueue);
                }
            } else {
                OptCatalogDamageHandlerPartial *const damageHandler = GetNodeDamageHandler(node);
                if (damageHandler != 0 && damageHandler != (OptCatalogDamageHandlerPartial *)(1) &&
                    damageHandler->timerContext != 0) {
                    queuedContact = AppendPendingContact(&playerState->transferQueue);
                } else if (candidate->surfaceNormal.y < -0.9f) {
                    queuedContact = AppendPendingContact(&playerState->worldCollisionQueue);
                } else if (candidate->surfaceNormal.y < 0.71f) {
                    queuedContact = AppendPendingContact(&playerState->preferredCollisionQueue);
                } else {
                    PlayerContactSurfacePayload *const scenePayload =
                        (PlayerContactSurfacePayload *)(candidate->scenePayload);
                    const int impactSlot = scenePayload != 0 ? scenePayload->impactSlot : 0;
                    if (impactSlot == 5 && playerState->recentHitValid == 0) {
                        playerState->recentHitValid = 1;
                    }
                }
            }
        }

        if (queuedContact != 0) {
            CopyPendingContactPayload(
                queuedContact,
                candidate,
                segmentStart,
                segmentEnd,
                segmentTag
            );
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-selectandresolvepreferredpendingcollisioncontact
 * @recoil-artifact defines .text recoil:function:0x423fc0: Player::SelectAndResolvePreferredPendingCollisionContact.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::SelectAndResolvePreferredPendingCollisionContact from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall SelectAndResolvePreferredPendingCollisionContact(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerPendingContact *selectedContact = playerState->preferredCollisionQueue.head;
    PlayerPendingContact *contact = selectedContact->next;
    while (contact != 0) {
        selectedContact = selectedContact->SelectPreferred(contact);
        contact = contact->next;
    }

    ResolvePendingCollisionContact(
        saveState,
        selectedContact
    );
    playerState->preferredCollisionResolved = 1;
}
} // namespace Player
/**
 * @recoil-anchor recoil:anchor:battlesport-player-playerpendingcontact-selectpreferred
 * @recoil-artifact defines .text recoil:function:0x424010: PlayerPendingContact::SelectPreferred.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement PlayerPendingContact::SelectPreferred from the recovered
 * Battlesport gameplay source file.
 */
PlayerPendingContact *__fastcall PlayerPendingContact::SelectPreferred(
    PlayerPendingContact *rhs
) {
    const float selfApproachDot = (sweepEnd.x - hit.hitPos.x) * hit.surfaceNormal.x +
                                  (sweepEnd.z - hit.hitPos.z) * hit.surfaceNormal.z;
    const float rhsApproachDot = (rhs->sweepEnd.x - rhs->hit.hitPos.x) * rhs->hit.surfaceNormal.x +
                                 (rhs->sweepEnd.z - rhs->hit.hitPos.z) * rhs->hit.surfaceNormal.z;

    if (-rhsApproachDot < -selfApproachDot) {
        return this;
    }
    return rhs;
}
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resolvependingworldcollisioncontact
 * @recoil-artifact defines .text recoil:function:0x424110: Player::ResolvePendingWorldCollisionContact.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ResolvePendingWorldCollisionContact from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ResolvePendingWorldCollisionContact(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerPendingContact *const contact = playerState->worldCollisionQueue.head;
    PreparePendingWorldCollisionResponse(
        saveState,
        contact
    );
    if (playerState->lifecycleState == kPlayerLifecycleLocal) {
        saveState->StartModalLoopSfxHandle(
            4,
            1.0f
        );
    }
    ResolvePendingCollisionContact(
        saveState,
        playerState->worldCollisionQueue.head
    );
}
} // namespace Player
namespace PlayerPickupContact {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-playerpickupcontact-passescollectiontest
 * @recoil-artifact defines .text recoil:function:0x424150: PlayerPickupContact::PassesCollectionTest.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement PlayerPickupContact::PassesCollectionTest from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall PassesCollectionTest(
    zUtil_SaveGameState *saveState,
    PlayerPendingContact *contact
) {
    (void)saveState;
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
    zClass_NodePartial *const pickupNode = contact->hit.node;

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetRaycastable(
        pickupNode,
        0
    );
    zClass_cls_di::SetBreakOnFirstCandidate(1);
    zClass_cls_di::SetStopAfterFirstHit(0x40000);

    const zVec3 startPoint = {
        contact->hit.hitPos.x,
        contact->hit.hitPos.y - 1.0f,
        contact->hit.hitPos.z,
    };
    const zVec3 endPoint = {
        pickupNode->cachedSphereCenter[0],
        pickupNode->cachedSphereCenter[1] - 1.0f,
        pickupNode->cachedSphereCenter[2],
    };

    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int raycastResult = zClass_cls_di::RaycastFindClosest(
        g_Player_RuntimeDiScene,
        &rayData,
        startPoint.x,
        startPoint.y,
        startPoint.z,
        endPoint.x,
        endPoint.y,
        endPoint.z
    );

    zClass_cls_di::SetBreakOnFirstCandidate(0);
    zClass_Class::gwNodeSetRaycastable(
        pickupNode,
        1
    );

    return raycastResult == 0 && rayData.candidateCount != 0 ? 0 : 1;
}
} // namespace PlayerPickupContact
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-processpendingpickupcontacts
 * @recoil-artifact defines .text recoil:function:0x424210: Player::ProcessPendingPickupContacts.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ProcessPendingPickupContacts from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ProcessPendingPickupContacts(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if ((zInput_GameStateOrMapTablePartial *)(saveState) != g_GameStateOrMapTable) {
        return;
    }

    if (playerState->lifecycleState == 4 || playerState->lifecycleState == 5) {
        return;
    }

    PlayerPendingContact *contact = playerState->pickupQueue.head;
    while (contact != 0) {
        if (PlayerPickupContact::PassesCollectionTest(
            saveState,
            contact
        ) != 0) {
            Pickup::OnCollected(
                contact->hit.node,
                saveState
            );
        }

        contact = contact != 0 ? contact->next : 0;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resolvependingcollisioncontact
 * @recoil-artifact defines .text recoil:function:0x424270: Player::ResolvePendingCollisionContact.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ResolvePendingCollisionContact from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ResolvePendingCollisionContact(
    zUtil_SaveGameState *saveState,
    PlayerPendingContact *contact
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    zClass_NodePartial *const hitNode = contact->hit.node;

    zVec3 sweepStart = contact->sweepStart;
    zVec3 sweepEnd = contact->sweepEnd;
    zVec3 contactPoint = contact->hit.hitPos;
    zVec3 contactNormal = contact->hit.surfaceNormal;

    const zVec3 contactToSweepStart = {sweepStart.x - contactPoint.x,
        sweepStart.y - contactPoint.y,
        sweepStart.z - contactPoint.z};
    if (Vec3Dot(
        contactToSweepStart,
        contactNormal
    ) < 0.0f) {
        contactNormal.x *= -1.0f;
        contactNormal.y *= -1.0f;
        contactNormal.z *= -1.0f;
    }

    if (contactNormal.x == 0.0f && contactNormal.z == 0.0f) {
        return;
    }

    const float localSpeed = Vec3Length(playerState->localVel);
    const float originalNormalY = contactNormal.y;
    sweepStart.y = 0.0f;
    sweepEnd.y = 0.0f;
    contactPoint.y = 0.0f;
    contactNormal.y = 0.0f;

    zVec3 contactToSweepEnd = {sweepEnd.x - contactPoint.x,
        sweepEnd.y - contactPoint.y,
        sweepEnd.z - contactPoint.z};
    zMath::Vec3NormalizeXZ(
        &contactNormal,
        &contactNormal
    );

    zVec3 reflectedSweepDir;
    zMath::Vec3Reflect(
        &contactNormal,
        &contactToSweepEnd,
        &reflectedSweepDir
    );
    const zVec3 reflectedContactPoint = {contactPoint.x + reflectedSweepDir.x,
        contactPoint.y + reflectedSweepDir.y,
        contactPoint.z + reflectedSweepDir.z};
    zVec3 worldPosCorrection = {reflectedContactPoint.x - sweepEnd.x,
        reflectedContactPoint.y - sweepEnd.y,
        reflectedContactPoint.z - sweepEnd.z};
    Vec3_FastNormalize(&worldPosCorrection);

    playerState->worldPos.x += worldPosCorrection.x;
    playerState->worldPos.y += worldPosCorrection.y;
    playerState->worldPos.z += worldPosCorrection.z;

    for (int i = 0; i < masterModalData->probePointCount; ++i) {
        playerState->modalProbeWorldByIndex[i].x += worldPosCorrection.x;
        playerState->modalProbeWorldByIndex[i].y += worldPosCorrection.y;
        playerState->modalProbeWorldByIndex[i].z += worldPosCorrection.z;
    }

    const int probeResolved = TryResolvePendingCollisionProbeSweep(saveState);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    if (probeResolved == 0) {
        const float collisionDampingA = masterModalData->collisionDampingA;
        float projectileVelY = playerState->projectileSpawnVel.y;
        zMath::Vec3NormalizeXZ(
            &reflectedSweepDir,
            &reflectedSweepDir
        );

        zVec3 surfaceTangent = Vec3Cross(
            reflectedSweepDir,
            contactNormal
        );
        surfaceTangent = Vec3Cross(
            contactNormal,
            surfaceTangent
        );

        reflectedSweepDir.x *= localSpeed;
        reflectedSweepDir.y *= localSpeed;
        reflectedSweepDir.z *= localSpeed;

        const float tangentSpeed = Vec3DotXZ(
            reflectedSweepDir,
            surfaceTangent
        );
        const zVec3 tangentVelocityDelta = {surfaceTangent.x * tangentSpeed,
            surfaceTangent.y * tangentSpeed,
            surfaceTangent.z * tangentSpeed};
        const float normalSpeed = collisionDampingA * Vec3DotXZ(
            reflectedSweepDir,
            contactNormal
        );
        const zVec3 normalVelocityDelta = {contactNormal.x * normalSpeed,
            contactNormal.y * normalSpeed,
            contactNormal.z * normalSpeed};

        playerState->projectileSpawnVel.x = normalVelocityDelta.x + tangentVelocityDelta.x;
        playerState->projectileSpawnVel.y = normalVelocityDelta.y + tangentVelocityDelta.y;
        playerState->projectileSpawnVel.z = normalVelocityDelta.z + tangentVelocityDelta.z;

        if (playerState->airborneFlag != 0) {
            if (originalNormalY > 0.01f) {
                if (projectileVelY < 0.0f) {
                    projectileVelY *= -0.5f;
                }
                zClass_Class::gwNodeSetCellPickable(
                    hitNode,
                    1
                );
            }

            if (Vec3Dot(
                playerState->projectileSpawnVel,
                playerState->projectileSpawnVel
            ) < 1.0f) {
                playerState->projectileSpawnVel.x = contactNormal.x * 10.0f;
                playerState->projectileSpawnVel.y = contactNormal.y * 10.0f;
                playerState->projectileSpawnVel.z = contactNormal.z * 10.0f;
            }
        }

        playerState->projectileSpawnVel.y = projectileVelY;
        zMath::Vec3RotateY(
            &playerState->localVel,
            &playerState->projectileSpawnVel,
            -playerState->restartYawRad
        );
    }

    const float yawImpulseCross =
        reflectedSweepDir.x * contactToSweepEnd.z - reflectedSweepDir.z * contactToSweepEnd.x;
    const int yawImpulseSign = yawImpulseCross < 0.0f ? -1 : 1;
    playerState->angVelYaw +=
        (float)(yawImpulseSign)*localSpeed * masterModalData->collisionDampingB;

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        return;
    }

    float impactGain = localSpeed / masterModalData->maxSpeed;
    if (impactGain > 1.0f) {
        impactGain = 1.0f;
    }
    saveState->StartModalLoopSfxHandle(
        4,
        impactGain
    );
    if (zInput_DI_IsForceFeedbackEnabled() != 0 && g_zInputFfEffectSet != 0) {
        g_zInputFfEffectSet->PlayCollisionImpactEffect(
            &contactNormal,
            impactGain
        );
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-preparependingworldcollisionresponse
 * @recoil-artifact defines .text recoil:function:0x4248e0: Player::PreparePendingWorldCollisionResponse.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::PreparePendingWorldCollisionResponse from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall PreparePendingWorldCollisionResponse(
    zUtil_SaveGameState *saveState,
    PlayerPendingContact *worldContacts
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->airborneFlag != 0 && playerState->projectileSpawnVel.y > 0.0f) {
        playerState->projectileSpawnVel.y = -playerState->projectileSpawnVel.y;
        playerState->localVel.y = playerState->projectileSpawnVel.y;
        playerState->worldPos.y = playerState->previousTransform.posY;
        while (worldContacts != 0) {
            playerState->worldPos.y -= kPlayerWorldCollisionStackDrop;
            worldContacts = worldContacts->next;
        }
        playerState->motionBasis.posY = playerState->worldPos.y;
        return;
    }

    const float restoreYOffset = masterModalData->masterType == kPlayerMasterTypeSub
                                     ? kPlayerWorldCollisionSubRestoreYOffset
                                     : 0.0f;
    playerState->worldPos.x = playerState->previousTransform.posX;
    playerState->worldPos.y = playerState->previousTransform.posY + restoreYOffset;
    playerState->worldPos.z = playerState->previousTransform.posZ;
    playerState->vehiclePitchRad = playerState->cachedPitchRad;
    playerState->restartYawRad = playerState->cachedYawRad;
    playerState->vehicleRollRad = playerState->cachedRollRad;
    playerState->angVelPitch = 0.0f;
    playerState->angVelYaw = 0.0f;
    playerState->angVelRoll = 0.0f;

    if (playerState->projectileSpawnVel.y > 0.0f) {
        playerState->projectileSpawnVel.y *= kPlayerWorldCollisionUpwardBounceDamping;
    }

    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    const zVec3 projectileVel = playerState->projectileSpawnVel;
    const zMat4x3 &motionBasis = playerState->motionBasis;
    playerState->localVel.x = projectileVel.x * motionBasis.xx + projectileVel.y * motionBasis.xy +
                              projectileVel.z * motionBasis.xz;
    playerState->localVel.y = projectileVel.x * motionBasis.yx + projectileVel.y * motionBasis.yy +
                              projectileVel.z * motionBasis.yz;
    playerState->localVel.z = projectileVel.x * motionBasis.zx + projectileVel.y * motionBasis.zy +
                              projectileVel.z * motionBasis.zz;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resolvependingplayercollisioncontact
 * @recoil-artifact defines .text recoil:function:0x424ac0: Player::ResolvePendingPlayerCollisionContact.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ResolvePendingPlayerCollisionContact from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ResolvePendingPlayerCollisionContact(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    PlayerPendingContact *const queuedContact = playerState->playerCollisionQueue.head;
    zClassDiPickCandidateEntry contactSnapshot = queuedContact->hit;
    zVec3 transferredLocalVel = playerState->projectileSpawnVel;

    PlayerCollisionContactContextPartial *const targetContext =
        (PlayerCollisionContactContextPartial *)(void *)(contactSnapshot.node->callbackContext);
    zUtil_SaveGameState *const targetSaveState = targetContext->saveState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetSaveState->playerState;
    PlayerMasterCommonData *const targetCommonData = targetPlayerState->masterCommonData;
    PlayerMasterModalData *const targetModalData =
        targetSaveState->primaryModalState->masterModalData;

    const float massScale = masterModalData->mass * targetModalData->invMass;
    transferredLocalVel.x *= massScale;
    transferredLocalVel.y *= massScale;
    transferredLocalVel.z *= massScale;
    zMath::Vec3RotateY(
        &transferredLocalVel,
        &transferredLocalVel,
        -targetPlayerState->restartYawRad
    );
    transferredLocalVel.y = 0.0f;

    targetPlayerState->localVel.x += transferredLocalVel.x;
    targetPlayerState->localVel.y += transferredLocalVel.y;
    targetPlayerState->localVel.z += transferredLocalVel.z;

    ResolvePendingCollisionContact(
        saveState,
        playerState->playerCollisionQueue.head
    );

    if (targetPlayerState->lifecycleState == kPlayerLifecycleAi) {
        const float damage = massScale * 1.10000002f;
        const float remainingFraction =
            (targetPlayerState->statusMeterValue - damage) * targetCommonData->invMaxHealth;
        if (remainingFraction > 0.200000003f) {
            HitCallback_RecordContextAndTimedStatus(
                targetSaveState,
                0,
                0,
                damage
            );
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-vec3-fastnormalize
 * @recoil-artifact defines .text recoil:function:0x424bf0: Player::Vec3_FastNormalize
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: scale short nonzero contact deltas with the fast approximate
 * square-root normalizer used by collision contact resolution.
 * Source owner: player contact unit-distance helper subsystem, not
 * player_camera_control_state_bridge or the broader Player C++ class.
 * Evidence: retail body reads only the zVec3 argument and
 * g_Player_CollisionContactResolveScale, uses the integer half-exponent
 * approximation, and returns whether the vector was rescaled.
 */
int __fastcall Vec3_FastNormalize(
    zVec3 *vec
) {
    const float lengthSq = vec->x * vec->x + vec->y * vec->y + vec->z * vec->z;
    if (lengthSq >= 0.01f || lengthSq == 0.0f) {
        return 0;
    }

    int lengthSqBits = 0;
    memcpy(
        &lengthSqBits,
        &lengthSq,
        sizeof(lengthSqBits)
    );
    lengthSqBits = (lengthSqBits >> 1) + 532676608;

    float approxLength = 0.0f;
    memcpy(
        &approxLength,
        &lengthSqBits,
        sizeof(approxLength)
    );
    const float scale = g_Player_CollisionContactResolveScale / (approxLength + 0.00000001f);

    vec->x *= scale;
    vec->y *= scale;
    vec->z *= scale;
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-constraintounitdistancefrom
 * @recoil-artifact defines .text recoil:function:0x424c90: Player::ConstrainToUnitDistanceFrom
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: constrain a nearby position to the contact resolve distance around
 * a center point.
 * Source owner: player contact unit-distance helper subsystem, not
 * player_camera_control_state_bridge or the broader Player C++ class.
 * Evidence: retail body forms a stack zVec3 delta, calls
 * Player::Vec3_FastNormalize, and writes back center plus normalized delta
 * only when the helper reports a short nonzero contact vector.
 */
void __fastcall ConstrainToUnitDistanceFrom(
    zVec3 *pos,
    const zVec3 *center
) {
    zVec3 delta = {pos->x - center->x, pos->y - center->y, pos->z - center->z};
    if (Vec3_FastNormalize(&delta) == 0) {
        return;
    }

    pos->x = center->x + delta.x;
    pos->y = center->y + delta.y;
    pos->z = center->z + delta.z;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-processtransfercontactqueue
 * @recoil-artifact defines .text recoil:function:0x424d00: Player::ProcessTransferContactQueue.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ProcessTransferContactQueue from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ProcessTransferContactQueue(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    const float localSpeedSq = playerState->localVel.x * playerState->localVel.x +
                               playerState->localVel.y * playerState->localVel.y +
                               playerState->localVel.z * playerState->localVel.z;
    const float transferDamage = (localSpeedSq * kPlayerTransferDamageScale) /
                                 (masterModalData->maxSpeed * masterModalData->maxSpeed);

    PlayerPendingContact *contact = playerState->transferQueue.head;
    while (contact != 0) {
        PlayerPendingContact *const next = contact->next;
        const float callbackResult = OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback(
            &contact->sweepStart,
            (OptCatalogHitEventPartial *)(void *)contact,
            transferDamage
        );
        if (callbackResult > 0.0f) {
            RemoveExistingPendingContact(
                &playerState->transferQueue,
                contact
            );
            AppendExistingPendingContact(
                &playerState->preferredCollisionQueue,
                contact
            );
        } else {
            zClass_NodePartial *const hitNode = contact->hit.node;
            RecordNodeFlagsForRestore(hitNode);
            zClass_Class::gwNodeSetCellPickable(
                hitNode,
                0
            );
            zClass_Class::gwNodeSetRaycastable(
                hitNode,
                0
            );
        }
        contact = next;
    }

    playerState->localVel.x *= kPlayerTransferVelocityDamping;
    playerState->localVel.y *= kPlayerTransferVelocityDamping;
    playerState->localVel.z *= kPlayerTransferVelocityDamping;
    playerState->projectileSpawnVel.x *= kPlayerTransferVelocityDamping;
    playerState->projectileSpawnVel.y *= kPlayerTransferVelocityDamping;
    playerState->projectileSpawnVel.z *= kPlayerTransferVelocityDamping;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-tryresolvependingcollisionprobesweep
 * @recoil-artifact defines .text recoil:function:0x424ed0: Player::TryResolvePendingCollisionProbeSweep.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::TryResolvePendingCollisionProbeSweep from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall TryResolvePendingCollisionProbeSweep(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    ClearPendingContactQueues(saveState);

    zClass_DiSegmentEndpoints segmentPairs[6];
    int segmentTags[6];
    for (int i = 0; i < 6; ++i) {
        segmentPairs[i].start = playerState->rootProbeWorldByIndex[i];
        segmentPairs[i].end = playerState->modalProbeWorldByIndex[i];
        segmentTags[i] = i;
    }

    CollectPendingContactsForSegments(
        saveState,
        segmentPairs,
        12,
        segmentTags
    );
    MoveTransferContactsToPreferredCollision(playerState);

    if (playerState->preferredCollisionQueue.count == 0 &&
        playerState->playerCollisionQueue.count == 0) {
        playerState->collisionProbeResolved = 0;
        return 0;
    }

    ApplyPendingCollisionProbeVelocity(saveState);
    playerState->collisionProbeResolved = 1;
    return 1;
}
} // namespace Player
/**
 * @recoil-anchor recoil:anchor:battlesport-player-hudsensortracker-parsecheckpointnumberfromnode
 * @recoil-artifact defines .text recoil:function:0x425060: HudSensorTracker::ParseCheckpointNumberFromNode
 * Source model: checkpoint-node name parser used by player contact handling;
 * MFC CString construction/Right/destruction are provider behavior.
 * Touched data: no authored globals; reads only the node flags, callback
 * context flags, and context node name.
 * Purpose: parse a nonnegative checkpoint number from the callback context node
 * name when checkpoint flags permit it.
 */
int __fastcall HudSensorTracker::ParseCheckpointNumberFromNode(
    zClass_NodePartial *node
) {
    if ((node->flags & 0x200000) == 0) {
        return 0;
    }

    zClass_NodePartial *const contextNode = node->callbackContext;
    if ((contextNode->auxFlags & 2) == 0) {
        return 0;
    }

    CString name(contextNode->name);
    int suffixLength = name.GetLength() - 10;
    if (suffixLength < 0) {
        suffixLength = 0;
    }

    CString checkpointNumber = name.Right(suffixLength);
    if (checkpointNumber.GetLength() == 0) {
        return 0;
    }

    const long parsedNumber = atol((const char *)checkpointNumber);
    return parsedNumber < 0 ? 0 : (int)(parsedNumber);
}
namespace Checkpoint {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-checkpoint-updateplayerlapprogressandnotifynet
 * @recoil-artifact defines .text recoil:function:0x425150: Checkpoint::UpdatePlayerLapProgressAndNotifyNet
 * Purpose: Marks checkpoint visits, completes laps after all checkpoint flags
 * are set, and notifies networking of lap progress.
 */
void __fastcall UpdatePlayerLapProgressAndNotifyNet(
    zUtil_SaveGameState *saveState,
    int checkpointIndex
) {
    const int checkpointCount = g_HudSensorTracker.checkpointCount;
    PlayerCheckpointLapProgressView *const playerProgress =
        (PlayerCheckpointLapProgressView *)(saveState->playerState);

    if (playerProgress->checkpointVisitedFlags[checkpointIndex] != 0) {
        return;
    }

    playerProgress->checkpointVisitedFlags[checkpointIndex] = 1;
    if (checkpointCount != checkpointIndex) {
        return;
    }

    int allPriorCheckpointsVisited = 1;
    for (int index = 1; index <= checkpointCount; ++index) {
        allPriorCheckpointsVisited =
            allPriorCheckpointsVisited != 0 && playerProgress->checkpointVisitedFlags[index] != 0
                ? 1
                : 0;
        playerProgress->checkpointVisitedFlags[index] = 0;
    }

    if (allPriorCheckpointsVisited == 0) {
        return;
    }

    playerProgress->lapTimeDelta = g_Time_AccumulatedTimeSec - playerProgress->lapTimestampSec;
    playerProgress->lapTimestampSec = g_Time_AccumulatedTimeSec;
    playerProgress->lapCompletionCount += 1;
    playerProgress->lapTimeSec = g_Time_AccumulatedTimeSec - playerProgress->checkpointTimestampSec;
    GameNet::SendPkt0E_PlayerLapProgress(saveState);
}
} // namespace Checkpoint
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-collectpendingcollisioncontactsforquadprobe
 * @recoil-artifact defines .text recoil:function:0x4251f0: Player::CollectPendingCollisionContactsForQuadProbe.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::CollectPendingCollisionContactsForQuadProbe from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall CollectPendingCollisionContactsForQuadProbe(
    zUtil_SaveGameState *saveState,
    float expandRadius
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    ClearPendingContactQueues(saveState);

    enum { kQuadProbePointCount = 4, kQuadProbeSegmentCount = 6 };

    const int probeIndices[kQuadProbePointCount] = {0, 2, 3, 5};
    for (int i = 0; i < kQuadProbePointCount; ++i) {
        zVec3 probePoint = masterModalData->probePoints[probeIndices[i]];
        probePoint.y += expandRadius;
        playerState->modalProbeWorldByIndex[probeIndices[i]] =
            TransformPointByMatrix(
                probePoint,
                playerState->motionBasis
            );
    }

    zClass_DiSegmentEndpoints segmentPairs[kQuadProbeSegmentCount];
    int segmentTags[kQuadProbeSegmentCount] = {0, 1, 2, 3, 4, 5};
    segmentPairs[0].start = playerState->modalProbeWorldByIndex[0];
    segmentPairs[0].end = playerState->modalProbeWorldByIndex[2];
    segmentPairs[1].start = playerState->modalProbeWorldByIndex[2];
    segmentPairs[1].end = playerState->modalProbeWorldByIndex[0];
    segmentPairs[2].start = playerState->modalProbeWorldByIndex[2];
    segmentPairs[2].end = playerState->modalProbeWorldByIndex[3];
    segmentPairs[3].start = playerState->modalProbeWorldByIndex[3];
    segmentPairs[3].end = playerState->modalProbeWorldByIndex[2];
    segmentPairs[4].start = playerState->modalProbeWorldByIndex[3];
    segmentPairs[4].end = playerState->modalProbeWorldByIndex[5];
    segmentPairs[5].start = playerState->modalProbeWorldByIndex[5];
    segmentPairs[5].end = playerState->modalProbeWorldByIndex[3];

    CollectPendingContactsForSegments(
        saveState,
        segmentPairs,
        kQuadProbeSegmentCount * 2,
        segmentTags
    );

    MoveTransferContactsToPreferredCollision(playerState);

    return playerState->preferredCollisionQueue.count != 0 ||
                   playerState->playerCollisionQueue.count != 0
               ? 1
               : 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applypendingcollisionprobevelocity
 * @recoil-artifact defines .text recoil:function:0x425770: Player::ApplyPendingCollisionProbeVelocity.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ApplyPendingCollisionProbeVelocity from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ApplyPendingCollisionProbeVelocity(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->collisionProbeResolved == 0) {
        playerState->worldPos.x = playerState->previousTransform.posX;
        playerState->worldPos.y = playerState->previousTransform.posY;
        playerState->worldPos.z = playerState->previousTransform.posZ;
        playerState->restartYawRad = playerState->cachedYawRad;
        zMath::MatBuildEulerRotation3x3(
            &playerState->motionBasis,
            playerState->vehiclePitchRad,
            playerState->cachedYawRad,
            playerState->vehicleRollRad
        );
        playerState->motionBasis.posX = playerState->worldPos.x;
        playerState->motionBasis.posY = playerState->worldPos.y;
        playerState->motionBasis.posZ = playerState->worldPos.z;
    }

    PlayerPendingContact *contact = playerState->preferredCollisionQueue.head;
    if (contact == 0) {
        contact = playerState->playerCollisionQueue.head;
    }
    if (contact == 0) {
        return;
    }

    const float previousY = playerState->projectileSpawnVel.y;
    const zVec3 surfaceNormal = contact->hit.surfaceNormal;
    playerState->projectileSpawnVel.x = surfaceNormal.x * 20.0f;
    playerState->projectileSpawnVel.y = surfaceNormal.y * 20.0f;
    playerState->projectileSpawnVel.z = surfaceNormal.z * 20.0f;

    if (playerState->projectileSpawnVel.y > 0.0f) {
        if (previousY > playerState->projectileSpawnVel.y) {
            playerState->projectileSpawnVel.y = previousY;
        }
    } else if (previousY < playerState->projectileSpawnVel.y) {
        playerState->projectileSpawnVel.y = previousY;
    }

    const zVec3 pushVel = playerState->projectileSpawnVel;
    const zMat4x3 &motionBasis = playerState->motionBasis;
    playerState->localVel.x =
        pushVel.x * motionBasis.xx + pushVel.y * motionBasis.xy + pushVel.z * motionBasis.xz;
    playerState->localVel.y =
        pushVel.x * motionBasis.yx + pushVel.y * motionBasis.yy + pushVel.z * motionBasis.yz;
    playerState->localVel.z =
        pushVel.x * motionBasis.zx + pushVel.y * motionBasis.zy + pushVel.z * motionBasis.zz;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-registergameplaycommandcallbacksandcreateffeffects
 * @recoil-artifact defines .text recoil:function:0x425920: Player::RegisterGameplayCommandCallbacksAndCreateFfEffects.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::RegisterGameplayCommandCallbacksAndCreateFfEffects from the recovered
 * Battlesport gameplay source file.
 */
void RegisterGameplayCommandCallbacksAndCreateFfEffects() {
    // zInput's keyboard bridge tail-jumps to these handlers with commandId in ECX.
    zInputCommandCallbackFn hudHotkeyCallback =
        (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand);
    zInput::BindMap_Current_SetCommandCallback(
        30,
        hudHotkeyCallback
    );
    zInput::BindMap_Current_SetCommandCallback(
        9,
        hudHotkeyCallback
    );
    zInput::BindMap_Current_SetCommandCallback(
        32,
        hudHotkeyCallback
    );
    zInput::BindMap_Current_SetCommandCallback(
        33,
        hudHotkeyCallback
    );

    if (zVid::GetAccelerationOption() == 0) {
        zInput::BindMap_Current_SetCommandCallback(
            34,
            (zInputCommandCallbackFn)(zVideo::HandleSoftwareModeHotkeyCommand)
        );
    }

    zInput::BindMap_Current_SetCommandCallback(
        35,
        hudHotkeyCallback
    );
    zInput::BindMap_Current_SetCommandCallback(
        42,
        hudHotkeyCallback
    );
    zInput::BindMap_Current_SetCommandCallback(
        43,
        hudHotkeyCallback
    );
    zInput::BindMap_Current_SetCommandCallback(
        44,
        hudHotkeyCallback
    );
    zInput::BindMap_Current_SetCommandCallback(
        45,
        hudHotkeyCallback
    );

    zInput_FFEffectSet *const effectSet = new zInput_FFEffectSet;
    if (effectSet != 0) {
        g_zInputFfEffectSet = zInput_DI_InitForceFeedbackEffectSet(effectSet);
    } else {
        g_zInputFfEffectSet = 0;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-ticklocalplayercontrols
 * @recoil-artifact defines .text recoil:function:0x425a20: Player::TickLocalPlayerControls.
 *
 * Purpose: advance local player control input, camera, movement, weapon, and
 * HUD interaction state for the current frame.
 *
 */
void __fastcall TickLocalPlayerControls(
    zUtil_SaveGameState *saveState
) {
    if (g_Player_LocalControlEnabled == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    zInput::MouseStateSnapshot mouseState = {0};
    if (zInp::GetJoystickOption() != 0) {
        DIJOYSTATE2 *const joyState = zInput::DI_GetCurrentState();
        if (playerState->cameraState == kPlayerCameraStateProjectileAttached) {
            playerState->cursorDeltaX = 0.0f;
            playerState->cursorDeltaY = 0.0f;
            playerState->cursorNormX =
                (float)(joyState->lX) * g_zInput_JoystickAxisConfig_Gameplay.axes[0].normScale;
            playerState->cursorNormY =
                (float)(-joyState->lY) * g_zInput_JoystickAxisConfig_Gameplay.axes[1].normScale;
        } else {
            playerState->cursorNormX = 0.0f;
            const float joyCursorY =
                (float)(-joyState->lY) * g_zInput_JoystickAxisConfig_Gameplay.axes[1].normScale;
            const float cursorBlend =
                PlayerFloatFromBits((int)(g_Player_DeltaTime * -3.2f * 12102200.0f) + 0x3f800000);
            playerState->cursorNormY =
                cursorBlend * playerState->cursorNormY + (1.0f - cursorBlend) * joyCursorY;
            playerState->steeringInput =
                (float)(-joyState->lX) * g_zInput_JoystickAxisConfig_Gameplay.axes[0].normScale;
            playerState->throttleInput =
                (float)(-joyState->lZ) * g_zInput_JoystickAxisConfig_Gameplay.axes[2].normScale;
            playerState->joyCameraYawInput =
                (float)(joyState->lRz) * g_zInput_JoystickAxisConfig_Gameplay.axes[3].normScale;
        }
    } else if ((g_Player_RuntimeInputFlags & 2) != 0) {
        zInput::Mouse_GetStateSnapshot(&mouseState);
        playerState->cursorDeltaX = mouseState.cursorNormX - playerState->cursorNormX;
        playerState->cursorDeltaY = mouseState.cursorNormY - playerState->cursorNormY;
        playerState->cursorNormX = mouseState.cursorNormX;
        playerState->cursorNormY = mouseState.cursorNormY;
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(4) & 3) != 0) {
        if (zOpt::GetThrottleMode() != 0) {
            playerState->throttleInput += g_FrameDeltaTimeSec;
        } else {
            playerState->throttleInput = 1.0f;
        }
    } else if ((zInput::BindMap_Current_ReadCommandInputState(1) & 3) != 0) {
        if (zOpt::GetThrottleMode() != 0) {
            playerState->throttleInput -= g_FrameDeltaTimeSec;
        } else {
            playerState->throttleInput = -1.0f;
        }
    } else if (zOpt::GetThrottleMode() == 0 && zInp::GetJoystickOption() == 0) {
        playerState->throttleInput = 0.0f;
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(2) & 3) != 0) {
        playerState->steeringInput = 1.0f;
    } else if ((zInput::BindMap_Current_ReadCommandInputState(3) & 3) != 0) {
        playerState->steeringInput = -1.0f;
    } else if (zInp::GetJoystickOption() == 0) {
        playerState->steeringInput = 0.0f;
    }

    if (zOpt::GetSteeringMode() == 0 && playerState->steeringInput == 0.0f &&
        zInp::GetJoystickOption() == 0) {
        if (zOpt::GetCursorMode() == 0) {
            if (playerState->cursorNormX > g_Player_CameraZone) {
                playerState->steeringInput =
                    (playerState->cursorNormX - g_Player_CameraZone) * -g_Player_CameraZoneInvRange;
            } else if (playerState->cursorNormX < -g_Player_CameraZone) {
                playerState->steeringInput =
                    (g_Player_CameraZone + playerState->cursorNormX) * -g_Player_CameraZoneInvRange;
            }
        } else if (playerState->cursorDeltaX == 0.0f && mouseState.deltaX != 0) {
            playerState->steeringInput =
                (float)(-mouseState.deltaX) * g_Player_GameplayInputStepScale;
        }
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(5) & 3) != 0) {
        playerState->subVerticalInput = 1.0f;
    } else if ((zInput::BindMap_Current_ReadCommandInputState(6) & 3) != 0) {
        playerState->subVerticalInput = -1.0f;
    } else {
        playerState->subVerticalInput = 0.0f;
    }

    playerState->subPitchInput = 0.0f;
    if (masterModalData->masterType == kPlayerMasterTypeSub &&
        (float)(fabs(playerState->localVel.z)) >= 10.0f) {
        if (playerState->cursorNormY > g_Player_CameraZone) {
            playerState->subPitchInput =
                (playerState->cursorNormY - g_Player_CameraZone) * -g_Player_CameraZoneInvRange;
        } else if (playerState->cursorNormY < -g_Player_CameraZone) {
            playerState->subPitchInput =
                (g_Player_CameraZone + playerState->cursorNormY) * -g_Player_CameraZoneInvRange;
        }
    }

    playerState->subVerticalInput = PlayerClampSigned(
        playerState->subVerticalInput,
        1.0f
    );
    playerState->throttleInput = PlayerClampSigned(
        playerState->throttleInput,
        1.0f
    );
    playerState->steeringInput = PlayerClampSigned(
        playerState->steeringInput,
        1.0f
    );
    playerState->subPitchInput = PlayerClampSigned(
        playerState->subPitchInput,
        1.0f
    );

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->subVerticalInputCopy = playerState->subVerticalInput;
    playerState->subPitchInputCopy = playerState->subPitchInput;
    playerState->steeringInputCopy = playerState->steeringInput;
    HudUiMgr::UpdateTargetReticleFromCursor(
        2,
        &playerState->storedTargetPos,
        playerState->cursorNormX,
        playerState->cursorNormY
    );

    const int altFireState = zInput::BindMap_Current_ReadCommandInputState(12);
    if ((altFireState & 3) != 0) {
        PlayerGunFireController *const activeAltGun = playerState->activeAltGunController;
        if ((activeAltGun->optCatalogEntry->flags & 2u) != 0) {
            if (activeAltGun->ammoOrCharge > 0.0f) {
                playerState->altGunDispatchRequested = 1;
            } else if (altFireState == 1) {
                playerState->altGunDispatchRequested = altFireState;
            }
        } else if ((playerState->altGunTransitionState & 0x180) == 0) {
            if (g_Player_TotalTimeSecScaled >= activeAltGun->nextDispatchTime &&
                playerState->playerOrdinal != 0 &&
                activeAltGun != &playerState->altWeaponBanks[1].controllerA) {
                playerState->altGunDispatchRequested = 1;
                activeAltGun->nextDispatchTime =
                    activeAltGun->dispatchRepeatDelay + g_Player_TotalTimeSecScaled;
            }
        } else if ((altFireState & 1) != 0) {
            playerState->pendingAltCameraToggle = 1;
        }
    } else {
        playerState->altGunDispatchRequested = 0;
    }

    playerState->usePresetGunFireDir = 0;
    if ((zInput::BindMap_Current_ReadCommandInputState(11) & 3) != 0) {
        PlayerGunFireController *const activePrimaryGun = playerState->activePrimaryGunController;
        if ((playerState->altGunTransitionState & 0x180) != 0) {
            playerState->usePresetGunFireDir = 1;
        } else if (g_Player_TotalTimeSecScaled >= activePrimaryGun->nextDispatchTime &&
                   playerState->playerOrdinal != 0 &&
                   g_Time_AccumulatedTimeSec >= playerState->primaryGunGateUntilTime) {
            playerState->primaryGunDispatchRequested = 1;
            activePrimaryGun->nextDispatchTime =
                activePrimaryGun->dispatchRepeatDelay + g_Player_TotalTimeSecScaled;
        }
    } else {
        playerState->primaryGunDispatchRequested = 0;
    }

    if (zInput::BindMap_Current_ReadCommandInputState(13) == 1) {
        playerState->altGunTriggerProcessFlag = 1;
        if (zOpt::GetNetworkEnabled() != 0 && g_HudSensorTracker.raceCheckpointMode != 0) {
            g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
        }
    } else {
        playerState->altGunTriggerProcessFlag = 0;
    }

    if (zInput::BindMap_Current_ReadCommandInputState(7) == 1) {
        ResetMouseControlStateAndRecenterCursor(saveState);
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(37) & 3) != 0 &&
        masterModalData->masterType == kPlayerMasterTypeHover && playerState->autoTurnSign == 0) {
        TransitionToMasterTypeTrack(
            g_LocalPlayerSaveState,
            0
        );
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(38) & 3) != 0 &&
        masterModalData->masterType == kPlayerMasterTypeHover &&
        playerState->nextModeSwitchAllowedTime != 0.0f) {
        TransitionToMasterTypeAmphib(
            g_LocalPlayerSaveState,
            1,
            0
        );
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(39) & 3) != 0 &&
        (masterModalData->masterType == kPlayerMasterTypeTrack ||
            masterModalData->masterType == kPlayerMasterTypeAmphib)) {
        TransitionToMasterTypeHover(
            g_LocalPlayerSaveState,
            0
        );
    }

    if ((zInput::BindMap_Current_ReadCommandInputState(40) & 3) != 0 &&
        masterModalData->masterType == kPlayerMasterTypeAmphib) {
        TransitionToMasterTypeSub(
            g_LocalPlayerSaveState,
            0
        );
    }

    if (zInput::BindMap_Current_ReadCommandInputState(8) != 1) {
        return;
    }

    playerState->autoTurnTargetWorldPos = playerState->storedTargetPos;
    SetAutoTurnTargetDirFromWorldPoint(
        saveState,
        &playerState->autoTurnTargetWorldPos
    );

    zVec3 cameraTarget = {0};
    zClass_Camera::gwCameraGetTarget(
        g_MainCamera,
        &cameraTarget.x,
        &cameraTarget.y,
        &cameraTarget.z
    );
    playerState->cameraLerpStart.x = cameraTarget.x - playerState->worldPos.x;
    playerState->cameraLerpStart.y = cameraTarget.y - playerState->worldPos.y;
    playerState->cameraLerpStart.z = cameraTarget.z - playerState->worldPos.z;

    const float cameraDistance = -playerState->cameraDistance;
    playerState->cameraLerpEnd.x = cameraDistance * playerState->autoTurnTargetDir.x;
    playerState->cameraLerpEnd.y = cameraDistance * playerState->autoTurnTargetDir.y;
    playerState->cameraLerpEnd.z = cameraDistance * playerState->autoTurnTargetDir.z;
    playerState->cameraLerpEnd.y = playerState->cameraLerpStart.y;
    ApplyCameraState(kPlayerCameraStateTargeting);
}
} // namespace Player
namespace HudUi {
/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hudui.cpp.
 * Purpose: Dispatch gameplay hotkeys to camera, HUD, cheat, chat, aux overlay, throttle, and save/load commands.
 */
void __fastcall HandleHotkeyCommand(
    int commandId
) {
    switch (commandId) {
    case 9:
        Player::ToggleSteeringModeAndResetMouseLook();
        return;
    case 30:
        Player::ApplyCameraState(0);
        return;
    case 31:
        Player::ApplyCameraState(2);
        return;
    case 32:
        HudUiMgr::ToggleHud();
        return;
    case 33:
        zOpt::ToggleHudTypeForCurrentHwMode();
        return;
    case 35:
        if (zOpt::GetNetworkEnabled() == 0) {
            HudUiCallback::QueueCheatCodeState();
        }
        zInput::Keyboard_ResetTransitionState();
        return;
    case 36:
        if (g_HudUi_AuxOverlayEnabled == 0) {
            g_HudUi_AuxOverlayEnabled = 1;
            HudUiMgr::SetFloatTimerVisible(1);
            HudUiMgr::SetAuxOverlayVisible(1);
        } else {
            g_HudUi_AuxOverlayEnabled = 0;
            HudUiMgr::SetFloatTimerVisible(0);
            HudUiMgr::SetAuxOverlayVisible(0);
        }
        return;
    case 42:
        GameNet::BeginChatCompose();
        return;
    case 43:
        if (zOpt::GetThrottleMode() == 0) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x24c),
                5.0f
            );
            zOpt::SetThrottleMode(1);
        } else {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x24d),
                5.0f
            );
            zOpt::SetThrottleMode(0);
        }
        return;
    case 44:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(
                0,
                g_HudUiMessage_NodeName,
                zLoc::GetMessageString(0x86),
                2.0f
            );
        } else if (HudUiMainMenuDialog::CanLoadGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_QUICKLOAD);
        } else {
            HudUiMgrObjective::Show(
                0,
                g_HudUiMessage_NodeName,
                zLoc::GetMessageString(0x87),
                2.0f
            );
        }
        return;
    case 45:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(
                0,
                g_HudUiMessage_NodeName,
                zLoc::GetMessageString(0x85),
                2.0f
            );
        } else if (HudUiMainMenuDialog::CanSaveGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
                RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED
            );
        } else {
            HudUiMgrObjective::Show(
                0,
                g_HudUiMessage_NodeName,
                zLoc::GetMessageString(0x82),
                2.0f
            );
        }
        return;
    default:
        return;
    }
}
} // namespace HudUi
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resetmousecontrolstateandrecentercursor
 * @recoil-artifact defines .text recoil:function:0x426330: Player::ResetMouseControlStateAndRecenterCursor
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\Player\Player_Camera.cpp.
 * Purpose: Reset a save state's mouse-look offsets and recenter the mouse
 * cursor.
 * Source owner: battlesport_gameplay.player_camera_control_state_bridge,
 * not a C++ Player class and not the accepted player_camera.c source-file
 * owner.
 */
void __fastcall ResetMouseControlStateAndRecenterCursor(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->thirdPersonYawOffset = 0.0f;
    playerState->cameraElevationOffset = 0.0f;
    zInput::Mouse_RecenterCursor();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-floatsign
 * @recoil-artifact defines .text recoil:function:0x426350: Player::FloatSign.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::FloatSign from the recovered
 * Battlesport gameplay source file.
 */
int __stdcall FloatSign(
    float value
) {
    if (value == 0.0f) {
        return 0;
    }

    if (value < 0.0f) {
        return -1;
    }

    return 1;
}
} // namespace Player
namespace Player {
/**
 * Provisional source-placement hypothesis: GameZRecoil/player.cpp.
 * Purpose: reimplement PlayerMgr::TickAllPlayers from the recovered
 * Battlesport gameplay source file.
 */
void TickAllPlayers() {
    g_Player_DeltaTime = g_FrameDeltaTimeSec >= kPlayerMinFrameDeltaSec ? g_FrameDeltaTimeSec
                                                                        : kPlayerMinFrameDeltaSec;
    g_Player_InvDeltaTime = 1.0f / g_Player_DeltaTime;
    g_Player_TotalTimeSecScaled = g_Time_AccumulatedTimeSec;
    g_Player_DeltaTimeScaled001 = g_Player_DeltaTime * kPlayerDeltaTimeScaled001Factor;

    int totalMode2Count = 0;
    int activeMode2Count = 0;
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        const int lifecycleState = playerState->lifecycleState;
        playerState->generalFlags |= kPlayerPerFrameGeneralFlag;

        if (lifecycleState == kPlayerLifecycleInactive ||
            lifecycleState == kPlayerLifecycleState6Inactive) {
            if (playerState->altGunFireHeldFlag != 0) {
                PlayerGunFireController *const activeAltGunController =
                    playerState->activeAltGunController;
                playerState->altGunFireHeldFlag = 0;
                OptCatalog::DeactivateTrailRuntimeState(activeAltGunController->trailRuntimeState);
            }

            if (saveState == g_LocalPlayerSaveState) {
                TickLocalPlayerControls(saveState);
            }

            if (playerState->cameraTickEnabled != 0) {
                TickActiveCameraState(saveState);
            }

            if (zOpt::GetNetworkEnabled() != 0 &&
                saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable &&
                saveState != g_Player2SaveState) {
                zClass_Class::gwNodeSetActive(
                    playerState->rootNode,
                    0
                );
            }
        } else if (lifecycleState == kPlayerLifecycleRemote) {
            TickRemoteNetworkPlayer(saveState);
        } else {
            if (saveState == g_LocalPlayerSaveState) {
                TickLocalPlayerControls(saveState);
            } else if (lifecycleState == kPlayerLifecycleAi) {
                ++totalMode2Count;
                if (VariantTag::TagsOverlap(
                    &playerState->variantTag,
                    &g_VariantTag_Current
                ) != 0) {
                    zUtil_PlayerStateStorage *const localPlayerState =
                        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
                    const float targetDistanceSq =
                        zMath::Vec3DistSqXZ(
                            &playerState->worldPos,
                            &localPlayerState->worldPos
                        );
                    playerState->targetDistanceSq = targetDistanceSq;

                    if ((targetDistanceSq <= playerState->aiActivationRadiusSq ||
                            playerState->recentHitFlag != 0) &&
                        playerState->aiTickSuppressed == 0) {
                        playerState->aiActive = 1;
                        ++activeMode2Count;
                        AINet::TickAiMode2TopLevel(saveState);
                    } else {
                        if (playerState->cameraTickEnabled != 0) {
                            TickActiveCameraState(saveState);
                        }
                        if (zSnd::GetAudioApiOption() == 1) {
                            saveState->UpdateModalLoopSfx(0);
                        }

                        const int altGunFireHeldFlag = playerState->altGunFireHeldFlag;
                        playerState->aiActive = 0;
                        if (altGunFireHeldFlag != 0) {
                            PlayerGunFireController *const activeAltGunController =
                                playerState->activeAltGunController;
                            playerState->altGunFireHeldFlag = 0;
                            OptCatalog::DeactivateTrailRuntimeState(
                                activeAltGunController->trailRuntimeState
                            );
                        }

                        saveState = saveState != 0 ? saveState->next : 0;
                        continue;
                    }
                } else {
                    if (playerState->cameraTickEnabled != 0) {
                        TickActiveCameraState(saveState);
                    }
                    if (zSnd::GetAudioApiOption() == 1) {
                        saveState->UpdateModalLoopSfx(0);
                    }

                    const int altGunFireHeldFlag = playerState->altGunFireHeldFlag;
                    playerState->aiActive = 0;
                    if (altGunFireHeldFlag != 0) {
                        PlayerGunFireController *const activeAltGunController =
                            playerState->activeAltGunController;
                        playerState->altGunFireHeldFlag = 0;
                        OptCatalog::DeactivateTrailRuntimeState(
                            activeAltGunController->trailRuntimeState
                        );
                    }

                    saveState = saveState != 0 ? saveState->next : 0;
                    continue;
                }
            }

            const int postTickLifecycleState = playerState->lifecycleState;
            if (postTickLifecycleState == kPlayerLifecycleLocal || postTickLifecycleState == 0 ||
                VariantTag::TagsOverlap(
                    &playerState->variantTag,
                    &g_VariantTag_Current
                ) != 0) {
                TickMasterTypeAndForceFeedback(saveState);

                if (playerState->masterType != kPlayerMasterTypeAmphib) {
                    if (g_Player_LocalControlEnabled != 0) {
                        UpdateAltGunAimDirection(saveState);
                    }

                    int altGunLatch = 0;
                    if (playerState->altGunDispatchRequested != 0 &&
                        playerState->activeAltGunController->ammoOrCharge > 0.0f) {
                        altGunLatch = 1;
                    }
                    playerState->netInputBit16Latch = altGunLatch;

                    int primaryGunLatch = 0;
                    if (playerState->primaryGunDispatchRequested != 0 &&
                        playerState->activePrimaryGunController->ammoOrCharge > 0.0f) {
                        primaryGunLatch = 1;
                    }
                    playerState->netInputBit17Latch = primaryGunLatch;

                    TickAltGunRuntimeState(saveState);
                }

                ResetDamageVisualsAndTimedStatus(saveState);
            }

            if (playerState->cameraTickEnabled != 0) {
                TickActiveCameraState(saveState);
            }
            if (zSnd::GetAudioApiOption() == 1) {
                saveState->UpdateModalLoopSfx(1);
            }
        }

        saveState = saveState != 0 ? saveState->next : 0;
    }

    if (zSnd::GetAudioApiOption() != 1) {
        zUtil_SaveGameState *const localSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
        if (localSaveState->playerState->lifecycleState != kPlayerLifecycleInactive) {
            localSaveState->UpdateModalLoopSfx(1);
        }
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer(
            (zUtil_SaveGameState *)g_GameStateOrMapTable
        );
    }

    UpdateDebugOverlayHud(
        g_CurrentPlayerSaveState,
        activeMode2Count,
        totalMode2Count
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-tickmastertypeandforcefeedback
 * @recoil-artifact defines .text recoil:function:0x4266b0: Player::TickMasterTypeAndForceFeedback.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::TickMasterTypeAndForceFeedback from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall TickMasterTypeAndForceFeedback(
    zUtil_SaveGameState *saveState
) {
    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        return;
    }

    if (playerState->damageProtectionActive != 0) {
        playerState->subPitchInput = 0.0f;
        playerState->subVerticalInput = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInput = 0.0f;
    }

    switch (masterModalData->masterType) {
    case 0:
        UpdateMasterTypeBasic(saveState);
        break;
    case kPlayerMasterTypeSub:
        UpdateMasterTypeSub(saveState);
        break;
    case kPlayerMasterTypeTrack:
        UpdateMasterTypeTrack(saveState);
        break;
    case kPlayerMasterTypeHover:
        UpdateMasterTypeHover(saveState);
        break;
    case kPlayerMasterTypeAmphib:
        UpdateMasterTypeAmphib(saveState);
        break;
    default:
        break;
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        zEffect::SetConditionalRefPos(&playerState->worldPos);
        if (zInput_DI_IsForceFeedbackEnabled() != 0) {
            zInput_DI_UpdateSteerAndPitchForceEffects(g_zInputFfEffectSet);
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypetrack
 * @recoil-artifact defines .text recoil:function:0x426770: Player::UpdateMasterTypeTrack.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeTrack from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeTrack(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    RebuildSteerBasisFromMotionAxes(saveState);

    if (playerState->airborneFlag != 0) {
        playerState->angVelYaw *= PlayerDampingFromRate(1.0f);
        if (playerState->slipSfxActive != 0) {
            StopSlipSfx(saveState);
        }
    } else {
        UpdateAutoTurnAndSteerFromTarget(saveState);
    }

    const float yawDelta = g_Player_DeltaTime * playerState->angVelYaw;
    if (playerState->environmentAttachmentActive != 0) {
        playerState->yawPoseCache = PlayerWrapSignedTwoPi(playerState->yawPoseCache + yawDelta);
        zMath::MatStackPushPtr((float *)&playerState->environmentAttachmentMatrix);
        zMath::MatLoadIdentity();
        gwNode::BuildNodeToAncestorMatrix(
            playerState->environmentAttachmentNode,
            3
        );
        zMath::MatStackPopPtr();
        playerState->restartYawRad =
            ExtractYawFromMatrix(&playerState->environmentAttachmentMatrix) +
            playerState->yawPoseCache;
    } else {
        playerState->restartYawRad = PlayerWrapSignedTwoPi(playerState->restartYawRad + yawDelta);
        playerState->pitchPoseCache = playerState->vehiclePitchRad;
        playerState->yawPoseCache = playerState->restartYawRad;
        playerState->rollPoseCache = playerState->vehicleRollRad;
    }

    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    RebuildSteerBasisFromMotionBasis(saveState);
    if (playerState->airborneFlag == 0) {
        UpdateYawVelocityFromSteerInput(saveState);
    }

    if (playerState->environmentAttachmentActive != 0) {
        zMath::Vec3RotateY(
            &playerState->yawRotatedLocalVel,
            &playerState->localVel,
            playerState->yawPoseCache
        );
        playerState->fxOffsetLocal.x += g_Player_DeltaTime * playerState->yawRotatedLocalVel.x;
        playerState->fxOffsetLocal.z += g_Player_DeltaTime * playerState->yawRotatedLocalVel.z;

        const zVec3 attachedWorld = TransformPointByMatrix(
            playerState->fxOffsetLocal,
            playerState->environmentAttachmentMatrix
        );
        playerState->projectileSpawnVel.x =
            (attachedWorld.x - playerState->worldPos.x) * g_Player_InvDeltaTime;
        playerState->projectileSpawnVel.y =
            (attachedWorld.y - playerState->worldPos.y) * g_Player_InvDeltaTime;
        playerState->projectileSpawnVel.z =
            (attachedWorld.z - playerState->worldPos.z) * g_Player_InvDeltaTime;
        playerState->worldPos = attachedWorld;
    } else {
        if (playerState->airborneFlag != 0) {
            const float airborneDamping = PlayerDampingFromRate(0.200000003f);
            playerState->projectileSpawnVel.x *= airborneDamping;
            playerState->projectileSpawnVel.z *= airborneDamping;
            playerState->localVel = playerState->projectileSpawnVel;
            playerState->localVel =
                TransformWorldVectorToLocal(
                    playerState->localVel,
                    playerState->motionBasis
                );
        } else {
            playerState->projectileSpawnVel =
                TransformLocalVectorToWorld(
                    playerState->localVel,
                    playerState->motionBasis
                );
        }

        playerState->worldPos.x += g_Player_DeltaTime * playerState->projectileSpawnVel.x;
        playerState->yawRotatedLocalVel = playerState->projectileSpawnVel;
        playerState->worldPos.z += g_Player_DeltaTime * playerState->projectileSpawnVel.z;
    }

    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    if (playerState->lifecycleState != 0) {
        ProcessPendingContactQueues(saveState);
    }

    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    if (masterModalData->masterType == 0) {
        UpdateMasterTypeBasicOrTrack_FromModalProbe(saveState);
        playerState->airborneFlag = 0;
    } else if (masterModalData->masterType == kPlayerMasterTypeTrack) {
        UpdatePostMoveEnvironment(
            saveState,
            saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable ? 7 : 4
        );
    }

    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(
            saveState,
            0.0f
        ) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    if (playerState->airborneFlag != playerState->airborneFlagPrev) {
        zClass_Class::gwNodeSetActive(
            playerState->modeVariantNode,
            playerState->airborneFlag == 0 ? 1 : 0
        );
    }
    playerState->airborneFlagPrev = playerState->airborneFlag;

    if (playerState->environmentAttachmentActive != 0) {
        CacheAttachmentLocalOffset(playerState);
    }

    zClass_Object3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    zClass_Object3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );
    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    if (primaryModalState->modalNode != 0 &&
        masterModalData->masterType == kPlayerMasterTypeTrack) {
        const float dampingWeight = PlayerDampingFromRate(masterModalData->chassisSmoothFactor);
        const float newWeight = 1.0f - dampingWeight;
        const float pitchTarget = masterModalData->chassisPitchRate * playerState->angVelPitch +
                                  masterModalData->chassisPitchMax * playerState->localVel.z;
        const float pitchFiltered =
            dampingWeight * primaryModalState->chassisPitchFilterState + newWeight * pitchTarget;
        primaryModalState->chassisPitchFilterState = pitchFiltered;
        const float rollFiltered = dampingWeight * primaryModalState->chassisRollFilterState;
        primaryModalState->chassisRollFilterState = rollFiltered;

        primaryModalState->chassisPitchAngleRad =
            PlayerClampSigned(
                pitchTarget - pitchFiltered,
                masterModalData->chassisPitchDamping
            );
        primaryModalState->chassisRollAngleRad = PlayerClampSigned(
            masterModalData->chassisRollMax * playerState->angVelYaw * playerState->localVel.z -
                rollFiltered,
            masterModalData->chassisRollDamping
        );
        zClass_Object3D::gwObject3DSetRotation(
            primaryModalState->modalNode,
            primaryModalState->chassisPitchAngleRad,
            0.0f,
            primaryModalState->chassisRollAngleRad
        );
    }

    zClass_Class::gwNodeUpdate(playerState->rootNode);
    if (primaryModalState->modalNode != 0) {
        zClass_Class::gwNodeUpdate(primaryModalState->modalNode);
    }
    float *const rootMatrix = zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode);
    memcpy(
        &playerState->previousTransform,
        rootMatrix,
        sizeof(playerState->previousTransform)
    );
    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedVehicleRotationAngles = playerState->vehicleRotationAngles;

    if (primaryModalState->nodeRTracks != 0) {
        const float rightTrackSpeed = -playerState->localVel.z - playerState->angVelYaw * -2.25f;
        const float rightTrackSpeedAbs = (float)(fabs(rightTrackSpeed));
        int variantIndex = 0;
        if (rightTrackSpeedAbs >= playerState->masterCommonData->trackSwitchDist2) {
            variantIndex = 3;
        } else if (rightTrackSpeedAbs >= playerState->masterCommonData->trackSwitchDist1) {
            variantIndex = 2;
        } else if (rightTrackSpeedAbs >= playerState->masterCommonData->trackSwitchDist0) {
            variantIndex = 1;
        }

        unsigned int displayInstanceValue = 0;
        zClass_Class::gwNodeGetUserData(
            primaryModalState->nodeRTracks,
            &displayInstanceValue
        );
        zDi::SetCurrentVariant(
            (zDiPartial *)displayInstanceValue,
            variantIndex
        );
        zClass_Class::gwNodeGetUserData(
            primaryModalState->nodeRTracks,
            &displayInstanceValue
        );
        zModel::SetDiTextureWorldPerMeter(
            (zDiPartial *)displayInstanceValue,
            1,
            rightTrackSpeed * 1.72000003f,
            0
        );
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)displayInstanceValue
        );

        const float leftTrackSpeed = -playerState->localVel.z - playerState->angVelYaw * 2.25f;
        zClass_Class::gwNodeGetUserData(
            primaryModalState->nodeLTracks,
            &displayInstanceValue
        );
        zModel::SetDiTextureWorldPerMeter(
            (zDiPartial *)displayInstanceValue,
            1,
            leftTrackSpeed * 1.72000003f,
            0
        );
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)displayInstanceValue
        );
    }

    if (primaryModalState->nodeDustL != 0 && primaryModalState->nodeDustR != 0) {
        if (playerState->airborneFlag != 0) {
            zClass_Object3D::gwObject3DSetScale(
                primaryModalState->nodeDustL,
                0.0f,
                0.0f,
                0.0f
            );
            zClass_Object3D::gwObject3DSetScale(
                primaryModalState->nodeDustR,
                0.0f,
                0.0f,
                0.0f
            );
        } else {
            const float dustScale =
                (float)(fabs(playerState->localVel.z)) / playerState->axisClampRuntime;
            zClass_Object3D::gwObject3DSetScale(
                primaryModalState->nodeDustL,
                dustScale,
                dustScale,
                dustScale
            );
            zClass_Object3D::gwObject3DSetScale(
                primaryModalState->nodeDustR,
                dustScale,
                dustScale,
                dustScale
            );
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypehover
 * @recoil-artifact defines .text recoil:function:0x427140: Player::UpdateMasterTypeHover.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeHover from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeHover(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    RebuildSteerBasisFromMotionAxes(saveState);
    UpdateAutoTurnAndSteerFromTarget(saveState);

    playerState->restartYawRad = PlayerWrapSignedTwoPi(
        playerState->restartYawRad + playerState->angVelYaw * g_Player_DeltaTime
    );

    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    RebuildSteerBasisFromMotionBasis(saveState);

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    UpdateYawVelocityFromSteerInput(saveState);

    playerState->projectileSpawnVel =
        TransformLocalVectorToWorld(
            playerState->localVel,
            playerState->motionBasis
        );

    playerState->worldPos.x += g_Player_DeltaTime * playerState->projectileSpawnVel.x;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->worldPos.z += g_Player_DeltaTime * playerState->projectileSpawnVel.z;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->worldPos.y += g_Player_DeltaTime * playerState->projectileSpawnVel.y;
    playerState->motionBasis.posY = playerState->worldPos.y;

    if (playerState->lifecycleState != 0) {
        ProcessPendingContactQueues(saveState);
    }

    if (playerState->slipSfxActive != 0 &&
        (playerState->playerCollisionResolved != 0 || playerState->worldCollisionResolved != 0 ||
            playerState->preferredCollisionResolved != 0)) {
        StopSlipSfx(saveState);
    }

    UpdateMasterTypeHover_FromModalProbe(saveState);

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(
            saveState,
            0.0f
        ) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    zClass_Object3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    zClass_Object3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );

    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    zClass_Class::gwNodeUpdate(playerState->rootNode);
    memcpy(
        &playerState->previousTransform,
        zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode),
        sizeof(playerState->previousTransform)
    );

    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedPitchRad = playerState->vehiclePitchRad;
    playerState->cachedYawRad = playerState->restartYawRad;
    playerState->cachedRollRad = playerState->vehicleRollRad;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypehover-frommodalprobe
 * @recoil-artifact defines .text recoil:function:0x427440: Player::UpdateMasterTypeHover_FromModalProbe.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeHover_FromModalProbe from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeHover_FromModalProbe(
    zUtil_SaveGameState *saveState
) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    float probeHeightByPoint[PLAYER_MAX_MODAL_PROBE_POINTS] = {0};
    float outBestHeight = 0.0f;
    PlayerProbeTypeHistogram outTypeHistogram = {0};
    int outAttachmentCandidateCount = 0;
    zClass_NodePartial *outAttachmentNode = 0;
    ProbeModalSampleHeights(
        saveState,
        probeHeightByPoint,
        &outBestHeight,
        0,
        &outTypeHistogram,
        &outAttachmentCandidateCount,
        &outAttachmentNode
    );

    playerState->yawVelocityLimit = masterModalData->yawRateMax;

    int lowestProbeIndex = 0;
    float lowestProbeHeight = 5000.0f;
    const int probePointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < probePointCount; ++i) {
        if (probeHeightByPoint[i] < lowestProbeHeight) {
            lowestProbeHeight = probeHeightByPoint[i];
            lowestProbeIndex = i;
        }
    }

    int supportPointIndex[3] = {0};
    int supportCount = 0;
    for (int supportIndex = 0; supportIndex < probePointCount && supportCount < 3; ++supportIndex) {
        if (supportIndex != lowestProbeIndex) {
            supportPointIndex[supportCount] = supportIndex;
            ++supportCount;
        }
    }

    zVec3 supportPoint0 =
        primaryModalState->transformedProbePointWorldByIndex[supportPointIndex[0]];
    supportPoint0.y = probeHeightByPoint[supportPointIndex[0]];
    zVec3 supportPoint1 =
        primaryModalState->transformedProbePointWorldByIndex[supportPointIndex[1]];
    supportPoint1.y = probeHeightByPoint[supportPointIndex[1]];
    zVec3 supportPoint2 =
        primaryModalState->transformedProbePointWorldByIndex[supportPointIndex[2]];
    supportPoint2.y = probeHeightByPoint[supportPointIndex[2]];

    zVec3 probePlaneNormal = {0};
    zMath_Vec3_TriangleNormal(
        &supportPoint0,
        &supportPoint1,
        &supportPoint2,
        &probePlaneNormal
    );

    float gravityScale = playerState->gravityAccel;
    if (probePlaneNormal.y < g_Player_MaxSlope) {
        gravityScale *= 12.0f;
    }
    const float slopeBase = g_Player_DeltaTime * gravityScale;
    zVec3 slopeImpulse = {0};
    slopeImpulse.x = probePlaneNormal.x * slopeBase;
    slopeImpulse.z = probePlaneNormal.z * slopeBase;
    slopeImpulse.y = (probePlaneNormal.y - 1.0f) * g_Player_DeltaTime * slopeBase;
    playerState->projectileSpawnVel.x += slopeImpulse.x;
    playerState->projectileSpawnVel.y += slopeImpulse.y;
    playerState->projectileSpawnVel.z += slopeImpulse.z;

    playerState->localVel =
        TransformWorldVectorToLocal(
            playerState->projectileSpawnVel,
            playerState->motionBasis
        );

    const int normalLerpBits =
        (int)(masterModalData->hoverNormalLerpRate * g_FrameDeltaTimeSec * 12102200.0f) +
        0x3f800000;
    zMath::Vec3LerpNormalize(
        &playerState->steerBasisRef,
        &probePlaneNormal,
        PlayerFloatFromBits(normalLerpBits)
    );
    RebuildSteerBasisRawFromRef(saveState);
    RebuildMotionBasisFromSteerBasis(saveState);

    float minHoverClearance = 1000.0f;
    for (int clearanceIndex = 0; clearanceIndex < probePointCount; ++clearanceIndex) {
        const zVec3 transformedProbePoint = TransformPointByMatrix(
            masterModalData->probePoints[kPlayerEnvProbeBasePointOffset + clearanceIndex],
            playerState->motionBasis
        );
        primaryModalState->transformedProbePointWorldByIndex[clearanceIndex] =
            transformedProbePoint;

        const float clearance = transformedProbePoint.y - probeHeightByPoint[clearanceIndex];
        if (clearance < minHoverClearance) {
            minHoverClearance = clearance;
        }
    }

    const float hoverLiftError = minHoverClearance - masterModalData->modeAltTransitionTime;
    if (playerState->modeVariantNode != 0) {
        zClass_Class::gwNodeSetActive(
            playerState->modeVariantNode,
            hoverLiftError <= 2.0f ? 1 : 0
        );
    }

    if (hoverLiftError >= 2.0f && playerState->localVel.y >= 0.0f) {
        playerState->localVel.y = 0.0f;
    }

    const int liftDampingBits =
        (int)(masterModalData->hoverLiftDampingRate * g_Player_DeltaTime * 12102200.0f) +
        0x3f800000;
    const float liftDamping = PlayerFloatFromBits(liftDampingBits);
    playerState->localVel.y =
        liftDamping * playerState->localVel.y -
        (1.0f - liftDamping) * masterModalData->hoverLiftScale * hoverLiftError;

    if (minHoverClearance < 0.0f) {
        playerState->worldPos.y -= minHoverClearance - 0.5f;
        playerState->motionBasis.posY = playerState->worldPos.y;
    }

    if (probePlaneNormal.y >= g_Player_MaxSlope) {
        playerState->localVel.y += 5.0f;
    }

    if (playerState->slipSfxActive != 0) {
        playerState->projectileSpawnVel =
            TransformLocalVectorToWorld(
                playerState->localVel,
                playerState->motionBasis
            );
    }

    zVec3 yawRelativeNormal = {0};
    zMath::Vec3RotateY(
        &yawRelativeNormal,
        &playerState->steerBasisRef,
        -playerState->restartYawRad
    );
    playerState->vehiclePitchRad = (float)(asin(yawRelativeNormal.z));
    playerState->vehicleRollRad = (float)(asin(-yawRelativeNormal.x));

    const float speedAbs = (float)(fabs(playerState->localVel.z));
    const float pitchWaveArg = (masterModalData->hoverPitchWaveSpeedRate * speedAbs +
                                   masterModalData->hoverPitchWaveBaseRate) *
                               g_Time_AccumulatedTimeSec;
    const float rollWaveArg = (masterModalData->hoverRollWaveSpeedRate * speedAbs +
                                  masterModalData->hoverRollWaveBaseRate) *
                              g_Time_AccumulatedTimeSec;
    const float pitchWave = (float)(sin(pitchWaveArg)) * masterModalData->hoverPitchWaveAmplitude;
    const float rollWave =
        (float)(sin(rollWaveArg)) * masterModalData->hoverRollWaveAmplitude +
        masterModalData->hoverRollYawCoupleScale * playerState->angVelYaw * playerState->localVel.z;
    playerState->vehiclePitchRad += g_Player_DeltaTime * pitchWave;
    playerState->vehicleRollRad += g_Player_DeltaTime * rollWave;

    playerState->vehiclePitchRad = PlayerClampSigned(
        playerState->vehiclePitchRad,
        0.523599982f
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypeamphib
 * @recoil-artifact defines .text recoil:function:0x4279f0: Player::UpdateMasterTypeAmphib.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeAmphib from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeAmphib(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    RebuildSteerBasisFromMotionAxes(saveState);
    UpdateAutoTurnAndSteerFromTarget(saveState);

    const float yawDelta = playerState->angVelYaw * g_Player_DeltaTime;
    if (playerState->environmentAttachmentActive != 0) {
        playerState->yawPoseCache = PlayerWrapSignedTwoPi(playerState->yawPoseCache + yawDelta);
        zMath::MatStackPushPtr((float *)&playerState->environmentAttachmentMatrix);
        zMath::MatLoadIdentity();
        gwNode::BuildNodeToAncestorMatrix(
            playerState->environmentAttachmentNode,
            3
        );
        zMath::MatStackPopPtr();
        playerState->restartYawRad =
            ExtractYawFromMatrix(&playerState->environmentAttachmentMatrix) +
            playerState->yawPoseCache;
    } else {
        playerState->restartYawRad = PlayerWrapSignedTwoPi(playerState->restartYawRad + yawDelta);
        playerState->pitchPoseCache = playerState->vehiclePitchRad;
        playerState->yawPoseCache = playerState->restartYawRad;
        playerState->rollPoseCache = playerState->vehicleRollRad;
    }

    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    RebuildSteerBasisFromMotionBasis(saveState);

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    UpdateYawVelocityFromSteerInput(saveState);

    if (playerState->environmentAttachmentActive != 0) {
        zMath::Vec3RotateY(
            &playerState->yawRotatedLocalVel,
            &playerState->localVel,
            playerState->yawPoseCache
        );
        playerState->fxOffsetLocal.x += playerState->yawRotatedLocalVel.x * g_Player_DeltaTime;
        playerState->fxOffsetLocal.z += playerState->yawRotatedLocalVel.z * g_Player_DeltaTime;
        playerState->fxOffsetLocal.y = 0.0f;

        const zVec3 attachedWorld = TransformPointByMatrix(
            playerState->fxOffsetLocal,
            playerState->environmentAttachmentMatrix
        );
        playerState->projectileSpawnVel.x =
            (attachedWorld.x - playerState->worldPos.x) * g_Player_InvDeltaTime;
        playerState->projectileSpawnVel.y =
            (attachedWorld.y - playerState->worldPos.y) * g_Player_InvDeltaTime;
        playerState->projectileSpawnVel.z =
            (attachedWorld.z - playerState->worldPos.z) * g_Player_InvDeltaTime;
        playerState->worldPos = attachedWorld;
    } else {
        const float negSteerX = -playerState->steerBasisNorm.x;
        const float negSteerZ = -playerState->steerBasisNorm.z;
        playerState->projectileSpawnVel.x =
            negSteerX * playerState->localVel.z + negSteerZ * playerState->localVel.x;
        playerState->projectileSpawnVel.y = playerState->localVel.y;
        playerState->projectileSpawnVel.z =
            negSteerZ * playerState->localVel.z - negSteerX * playerState->localVel.x;
        playerState->worldPos.x += playerState->projectileSpawnVel.x * g_Player_DeltaTime;
        playerState->yawRotatedLocalVel = playerState->projectileSpawnVel;
        playerState->worldPos.z += playerState->projectileSpawnVel.z * g_Player_DeltaTime;
    }

    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    if (playerState->lifecycleState != 0) {
        ProcessPendingContactQueues(saveState);
    }

    UpdateMasterTypeAmphib_FromModalProbe(saveState);

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(
            saveState,
            0.0f
        ) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    zClass_Object3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    zClass_Object3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );
    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    zClass_Class::gwNodeUpdate(playerState->rootNode);
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    float *const rootMatrix = zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode);
    memcpy(
        &playerState->previousTransform,
        rootMatrix,
        sizeof(playerState->previousTransform)
    );
    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedVehicleRotationAngles = playerState->vehicleRotationAngles;

    if (primaryModalState->nodeWake != 0) {
        const float wakeScale =
            (float)(fabs(playerState->localVel.z)) / playerState->axisClampRuntime;
        zClass_Object3D::gwObject3DSetScale(
            primaryModalState->nodeWake,
            wakeScale,
            wakeScale,
            wakeScale
        );
        zClass_Object3D::gwObject3DSetScale(
            primaryModalState->nodeSplashL,
            wakeScale,
            wakeScale,
            wakeScale
        );
        zClass_Object3D::gwObject3DSetScale(
            primaryModalState->nodeSplashR,
            wakeScale,
            wakeScale,
            wakeScale
        );
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypeamphib-frommodalprobe
 * @recoil-artifact defines .text recoil:function:0x427ec0: Player::UpdateMasterTypeAmphib_FromModalProbe.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeAmphib_FromModalProbe from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeAmphib_FromModalProbe(
    zUtil_SaveGameState *saveState
) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    float probeHeightByPoint[PLAYER_MAX_MODAL_PROBE_POINTS] = {0};
    float outBestHeight = 0.0f;
    PlayerProbeTypeHistogram outTypeHistogram = {0};
    int outAttachmentCandidateCount = 0;
    zClass_NodePartial *outAttachmentNode = 0;
    ProbeModalSampleHeights(
        saveState,
        probeHeightByPoint,
        &outBestHeight,
        0,
        &outTypeHistogram,
        &outAttachmentCandidateCount,
        &outAttachmentNode
    );

    playerState->yawVelocityLimit = masterModalData->yawRateMax;
    const int probePointCount = primaryModalState->modalStateCode;
    if (outTypeHistogram.countByImpactSlot[1] >= probePointCount) {
        playerState->amphibProbeCoverageFailed = 0;
    } else if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        playerState->amphibProbeCoverageFailed = 1;
        TransitionToMasterTypeTrack(
            saveState,
            0
        );
    } else {
        playerState->projectileSpawnVel.x = 0.0f;
        playerState->projectileSpawnVel.z = 0.0f;
        playerState->localVel.x = 0.0f;
        playerState->localVel.z = 0.0f;
        playerState->aiTopLevelState = 0;
        playerState->aiStateUntilTime = g_Time_AccumulatedTimeSec + 8.0f;
    }

    float maxSampleHeight = outBestHeight;
    for (int i = 0; i < probePointCount; ++i) {
        if (maxSampleHeight < probeHeightByPoint[i]) {
            maxSampleHeight = probeHeightByPoint[i];
        }
    }

    const float oldWorldY = playerState->worldPos.y;
    playerState->worldPos.y = maxSampleHeight + masterModalData->modeAltTransitionTime;
    const float verticalVelocity = (playerState->worldPos.y - oldWorldY) * g_Player_InvDeltaTime;
    playerState->localVel.y = verticalVelocity;
    playerState->projectileSpawnVel.y = verticalVelocity;

    zVec3 amphibUpVector = g_Player_AmphibBasisUpRef;
    ApplyAmphibSpeedOscillation(
        saveState,
        &amphibUpVector,
        1
    );

    const int steerLerpBits =
        (int)(-(g_FrameDeltaTimeSec * g_Player_AmphibSteerBasisLerpRate) * 12102200.0f) +
        0x3f800000;
    zMath::Vec3LerpNormalize(
        &playerState->steerBasisRef,
        &amphibUpVector,
        PlayerFloatFromBits(steerLerpBits)
    );
    if (playerState->steerBasisRef.y == 0.0f) {
        playerState->steerBasisRef.y = 0.00100000005f;
    }

    zVec3 rawBasis = playerState->steerBasisNorm;
    rawBasis.y =
        -((rawBasis.x * playerState->steerBasisRef.x + rawBasis.z * playerState->steerBasisRef.z) /
            playerState->steerBasisRef.y);
    zMath::Vec3Normalize(&rawBasis);
    playerState->steerBasisRaw = rawBasis;
    RebuildMotionBasisFromSteerBasis(saveState);

    zMath::Vec3RotateY(
        &amphibUpVector,
        &playerState->steerBasisRef,
        -playerState->restartYawRad
    );
    playerState->vehiclePitchRad = (float)(asin(amphibUpVector.z));
    playerState->vehicleRollRad = (float)(asin(-amphibUpVector.x));
    playerState->vehiclePitchRad = PlayerClampSigned(
        playerState->vehiclePitchRad,
        0.523599982f
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypebasic
 * @recoil-artifact defines .text recoil:function:0x428120: Player::UpdateMasterTypeBasic.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeBasic from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeBasic(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    float savedLocalVelX = 0.0f;
    if (playerState->cameraState == 2) {
        UpdateBankVelocityFromSteerInput(saveState);
        savedLocalVelX = playerState->localVel.x;
    } else {
        IntegrateYawAndWrapFromYawVelocity(saveState);
    }

    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    RebuildSteerBasisFromMotionBasis(saveState);

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    UpdateYawVelocityFromSteerInput(saveState);
    if (playerState->cameraState == 2) {
        playerState->localVel.x = savedLocalVelX;
    }

    const float negSteerBasisX = -playerState->steerBasisNorm.x;
    const float negSteerBasisZ = -playerState->steerBasisNorm.z;
    const float worldVelX =
        negSteerBasisX * playerState->localVel.z + negSteerBasisZ * playerState->localVel.x;
    const float worldVelZ = playerState->steerBasisNorm.x * playerState->localVel.x +
                            negSteerBasisZ * playerState->localVel.z;

    playerState->projectileSpawnVel.y = playerState->localVel.y;
    playerState->projectileSpawnVel.x = worldVelX;
    playerState->projectileSpawnVel.z = worldVelZ;

    playerState->worldPos.x += worldVelX * g_Player_DeltaTime;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->worldPos.z += worldVelZ * g_Player_DeltaTime;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    UpdateMasterTypeBasicOrTrack_FromModalProbe(saveState);

    zClass_Object3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    zClass_Object3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );

    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    zClass_Class::gwNodeUpdate(playerState->rootNode);
    memcpy(
        &playerState->previousTransform,
        zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode),
        sizeof(playerState->previousTransform)
    );

    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedPitchRad = playerState->vehiclePitchRad;
    playerState->cachedYawRad = playerState->restartYawRad;
    playerState->cachedRollRad = playerState->vehicleRollRad;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypebasicortrack-frommodalprobe
 * @recoil-artifact defines .text recoil:function:0x428350: Player::UpdateMasterTypeBasicOrTrack_FromModalProbe.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeBasicOrTrack_FromModalProbe from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeBasicOrTrack_FromModalProbe(
    zUtil_SaveGameState *saveState
) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    float sampleHeights[PLAYER_MAX_MODAL_PROBE_POINTS] = {0};
    float unusedBestHeight = 0.0f;
    PlayerProbeTypeHistogram unusedHistogram = {0};
    int unusedAttachmentCandidateCount = 0;
    zClass_NodePartial *unusedAttachmentNode = 0;
    ProbeModalSampleHeights(
        saveState,
        sampleHeights,
        &unusedBestHeight,
        0,
        &unusedHistogram,
        &unusedAttachmentCandidateCount,
        &unusedAttachmentNode
    );

    playerState->yawVelocityLimit = masterModalData->yawRateMax;

    float maxSampleHeight = 0.0f;
    const int probePointCount = primaryModalState->modalStateCode;
    if (probePointCount > 0) {
        maxSampleHeight = sampleHeights[0];
        for (int i = 1; i < probePointCount; ++i) {
            if (maxSampleHeight < sampleHeights[i]) {
                maxSampleHeight = sampleHeights[i];
            }
        }
    }

    playerState->vehiclePitchRad = 0.0f;
    playerState->vehicleRollRad = 0.0f;
    playerState->worldPos.y = masterModalData->modeAltTransitionTime + maxSampleHeight;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatebankvelocityfromsteerinput
 * @recoil-artifact defines .text recoil:function:0x4283f0: Player::UpdateBankVelocityFromSteerInput.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\player.cpp.
 * Purpose: reimplement Player::UpdateBankVelocityFromSteerInput from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateBankVelocityFromSteerInput(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    playerState->restartYawRad = 0.0f;
    if (playerState->steeringInput == 0.0f) {
        playerState->localVel.x = 0.0f;
        return;
    }

    if ((playerState->steeringInputCopy > 0.0f && playerState->localVel.x > 0.0f) ||
        (playerState->steeringInputCopy < 0.0f && playerState->localVel.x < 0.0f)) {
        playerState->localVel.x = 0.0f;
    }

    playerState->localVel.x -=
        masterModalData->accelRate * g_Player_DeltaTime * playerState->steeringInputCopy;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-integrateyawandwrapfromyawvelocity
 * @recoil-artifact defines .text recoil:function:0x428490: Player::IntegrateYawAndWrapFromYawVelocity.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\player.cpp.
 * Purpose: reimplement Player::IntegrateYawAndWrapFromYawVelocity from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall IntegrateYawAndWrapFromYawVelocity(
    zUtil_SaveGameState *saveState
) {
    const float kTwoPi = 6.28318548f;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->autoTurnActive != 0) {
        playerState->restartYawRad =
            (float)(atan2(
                -playerState->autoTurnTargetDir.z,
                -playerState->autoTurnTargetDir.x
            ));
        playerState->steeringInput = 0.0f;
        playerState->angVelYaw = 0.0f;
        playerState->autoTurnActive = 0;
    }

    UpdateAutoTurnAndSteerFromTarget(saveState);

    float yaw = playerState->restartYawRad + playerState->angVelYaw * g_Player_DeltaTime;
    playerState->restartYawRad = yaw;
    if (yaw < -kTwoPi) {
        playerState->restartYawRad = yaw + kTwoPi;
    } else if (yaw > kTwoPi) {
        playerState->restartYawRad = yaw - kTwoPi;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypesub
 * @recoil-artifact defines .text recoil:function:0x428520: Player::UpdateMasterTypeSub.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeSub from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeSub(
    zUtil_SaveGameState *saveState
) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    CacheDisableCopterSndNodesAndStopSample();
    RebuildSteerBasisFromMotionAxes(saveState);
    UpdateAutoTurnAndSteerFromTarget(saveState);

    if (playerState->subPitchInput == 0.0f) {
        playerState->angVelPitch = 0.0f;
        playerState->vehiclePitchRad *= PlayerDampingFromRate(7.0f);
    } else {
        if ((playerState->subPitchInputCopy > 0.0f && playerState->angVelPitch < 0.0f) ||
            (playerState->subPitchInputCopy < 0.0f && playerState->angVelPitch > 0.0f)) {
            playerState->angVelPitch = 0.0f;
        }

        playerState->angVelPitch +=
            masterModalData->yawAccel * g_Player_DeltaTime * playerState->subPitchInputCopy * 0.5f;
        playerState->angVelPitch =
            PlayerClampSigned(
                playerState->angVelPitch,
                masterModalData->yawRateMax
            );
    }

    playerState->vehiclePitchRad += playerState->angVelPitch * g_Player_DeltaTime;
    playerState->restartYawRad = PlayerWrapSignedTwoPi(
        playerState->restartYawRad + playerState->angVelYaw * g_Player_DeltaTime
    );
    playerState->vehicleRollRad += playerState->angVelRoll * g_Player_DeltaTime;
    playerState->vehicleRollRad -=
        masterModalData->hoverRollYawCoupleScale * playerState->angVelYaw * playerState->localVel.z;
    playerState->vehiclePitchRad = PlayerClampSigned(
        playerState->vehiclePitchRad,
        0.5f
    );
    playerState->vehicleRollRad = PlayerClampSigned(
        playerState->vehicleRollRad,
        0.349999994f
    );

    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    RebuildSteerBasisFromMotionBasis(saveState);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->axisClampRuntime = masterModalData->maxSpeed;

    UpdateYawVelocityFromSteerInput(saveState);
    UpdateSubVerticalDamping(saveState);

    playerState->projectileSpawnVel =
        TransformLocalVectorToWorld(
            playerState->localVel,
            playerState->motionBasis
        );
    playerState->worldPos.x += playerState->projectileSpawnVel.x * g_Player_DeltaTime;
    playerState->worldPos.y += playerState->projectileSpawnVel.y * g_Player_DeltaTime;
    playerState->worldPos.z += playerState->projectileSpawnVel.z * g_Player_DeltaTime;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    ProcessPendingContactQueues(saveState);
    UpdateSubModeWaterProbeState(saveState);
    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(saveState, 0.0f) != 0 ||
            CollectPendingCollisionContactsForQuadProbe(
                saveState,
                1.25f
            ) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    zClass_Object3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    zClass_Object3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );
    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;
    zClass_Class::gwNodeUpdate(playerState->rootNode);

    float *const rootMatrix = zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode);
    memcpy(
        &playerState->previousTransform,
        rootMatrix,
        sizeof(playerState->previousTransform)
    );
    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedPitchRad = playerState->vehiclePitchRad;
    playerState->cachedYawRad = playerState->restartYawRad;
    playerState->cachedRollRad = playerState->vehicleRollRad;

    zClass_NodePartial *const nodeProps = primaryModalState->nodeProps;
    if (nodeProps != 0) {
        const float cycleSpeed = 6.0f - playerState->localVel.z * 0.5f;
        unsigned int displayInstanceValue = 0;
        zClass_Class::gwNodeGetUserData(
            nodeProps,
            &displayInstanceValue
        );
        zDi::SetCurrentVariantCycleTextureSpeed(
            (zDiPartial *)displayInstanceValue,
            cycleSpeed
        );
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatesubmodewaterprobestate
 * @recoil-artifact defines .text recoil:function:0x4289f0: Player::UpdateSubModeWaterProbeState.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateSubModeWaterProbeState from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateSubModeWaterProbeState(
    zUtil_SaveGameState *saveState
) {
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    float probeHeightByPoint[PLAYER_MAX_MODAL_PROBE_POINTS] = {0};
    float outBestHeight = 0.0f;
    PlayerProbeTypeHistogram outTypeHistogram = {0};
    int outAttachmentCandidateCount = 0;
    zClass_NodePartial *outAttachmentNode = 0;
    ProbeModalSampleHeights(
        saveState,
        probeHeightByPoint,
        &outBestHeight,
        1,
        &outTypeHistogram,
        &outAttachmentCandidateCount,
        &outAttachmentNode
    );

    playerState->yawVelocityLimit = masterModalData->yawRateMax;
    if (outBestHeight == -300.0f) {
        outBestHeight = 1000.0f;
    }
    playerState->subModeProbeBestHeight = outBestHeight;

    float deepestSubmergedSampleHeight = -300.0f;
    int deepestSubmergedSampleIndex = 0;
    const int probePointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < probePointCount; ++i) {
        const float sampleHeight = probeHeightByPoint[i];
        if (sampleHeight < outBestHeight && deepestSubmergedSampleHeight < sampleHeight) {
            deepestSubmergedSampleHeight = sampleHeight;
            deepestSubmergedSampleIndex = i;
        }
    }

    if (playerState->worldCollisionResolved != 1) {
        float resolvedY = masterModalData->modeAltTransitionTime + outBestHeight;
        if (playerState->worldPos.y >= resolvedY) {
            const float submergedProbeBaseHeight =
                deepestSubmergedSampleHeight -
                masterModalData->probePoints[15 + deepestSubmergedSampleIndex].y;
            if (playerState->worldPos.y > submergedProbeBaseHeight) {
                resolvedY = submergedProbeBaseHeight;
            } else {
                resolvedY = playerState->worldPos.y;
            }
        }

        playerState->worldPos.y = resolvedY;
        playerState->motionBasis.posY = resolvedY;
    }

    const float rollDampingFactor =
        PlayerFloatFromBits((int)(-g_Player_DeltaTime * 12102200.0f) + 0x3f800000);
    playerState->angVelRoll = -(rollDampingFactor * playerState->vehicleRollRad);

    const float speedAbs = (float)(fabs(playerState->localVel.z));
    const float pitchWaveRate = speedAbs * masterModalData->hoverPitchWaveSpeedRate +
                                masterModalData->hoverPitchWaveBaseRate;
    const float rollWaveRate =
        speedAbs * masterModalData->hoverRollWaveSpeedRate + masterModalData->hoverRollWaveBaseRate;
    const float pitchBobDelta = (float)(sin(pitchWaveRate * g_Time_AccumulatedTimeSec)) *
                                masterModalData->hoverPitchWaveAmplitude;
    const float rollBobDelta = (float)(sin(rollWaveRate * g_Time_AccumulatedTimeSec)) *
                               masterModalData->hoverRollWaveAmplitude;

    playerState->vehiclePitchRad += g_Player_DeltaTime * pitchBobDelta;
    playerState->vehicleRollRad += g_Player_DeltaTime * rollBobDelta;

    if (playerState->underwaterFxEnabled != 0 && playerState->cameraTarget.y < outBestHeight) {
        SetHudUiElementVisible(
            &g_Player_UnderwaterFxPass3Ui,
            1
        );
        g_Player_HorizonNodeFollowCameraEnabled = 0;

        zClass_NodePartial *const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(
                nodeCaustic1,
                &displayInstanceValue
            );
            zDi::SetCurrentVariantCycleTextureSpeed(
                (zDiPartial *)displayInstanceValue,
                12.0f
            );
        }
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        playerState->worldPos.y + 2.20000005f >= outBestHeight) {
        TransitionToMasterTypeAmphib(
            saveState,
            0,
            0
        );
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatesubverticaldamping
 * @recoil-artifact defines .text recoil:function:0x428c20: Player::UpdateSubVerticalDamping.
 * Source model: bounded Player namespace subsystem helper, not a C++ Player class member.
 * Purpose: Apply submarine vertical input acceleration, velocity clamp, and neutral-input vertical damping.
 */
void __fastcall UpdateSubVerticalDamping(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->subVerticalInput != 0.0f) {
        if ((playerState->subVerticalInputCopy > 0.0f && playerState->localVel.y < 0.0f) ||
            (playerState->subVerticalInputCopy < 0.0f && playerState->localVel.y > 0.0f)) {
            playerState->localVel.y = 0.0f;
        }

        const float localY =
            masterModalData->accelRate * g_Player_DeltaTime * playerState->subVerticalInputCopy +
            playerState->localVel.y;
        playerState->localVel.y = localY;
        if (localY > 20.0f) {
            playerState->localVel.y = 20.0f;
        } else if (localY < -20.0f) {
            playerState->localVel.y = -20.0f;
        }
        return;
    }

    if (playerState->throttleInputCopy == 0.0f) {
        const float dampingRate =
            g_Time_AccumulatedTimeSec < playerState->primaryGunGateUntilTime ? 2.0f : 10.0f;
        float dampingScale = dampingRate * g_Player_DeltaTime;
        dampingScale = -dampingScale;
        int dampingBits = (int)(dampingScale * 12102200.0f);
        const int dampingFloatBits = dampingBits + 0x3f800000;

        float dampingFactor = 0.0f;
        memcpy(
            &dampingFactor,
            &dampingFloatBits,
            sizeof(dampingFactor)
        );
        playerState->localVel.y *= dampingFactor;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-probemodalsampleheights
 * @recoil-artifact defines .text recoil:function:0x428d60: Player::ProbeModalSampleHeights.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Source model: bounded Player modal-probe subsystem helper over zUtil_SaveGameState,
 * PlayerModalState, PlayerMasterModalData, accepted zClass/zDI dependencies, and
 * accepted Player/frame/variant/zInput runtime globals; no Player C++ class object or
 * table ownership is required by current BN evidence.
 * Purpose: transform active modal probe points, build scene height candidates, select
 * per-sample impact heights, and publish histogram and attachment outputs.
 */
void __fastcall ProbeModalSampleHeights(
    zUtil_SaveGameState *saveState,
    float *outSampleHeightByPoint,
    float *outBestHeight,
    int preferAttachmentSlot1,
    PlayerProbeTypeHistogram *outTypeHistogram,
    int *outAttachmentCandidateCount,
    zClass_NodePartial **outAttachmentNode
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const globalPlayerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    memset(
        outTypeHistogram,
        0,
        sizeof(*outTypeHistogram)
    );
    zClass_Class::gwNodeSetCellPickable(
        playerState->rootNode,
        0
    );
    zClass_Class::gwNodeSetCellPickable(
        globalPlayerState->rootNode,
        0
    );

    const float probeYAdvance = playerState->projectileSpawnVel.y * g_Player_DeltaTime;
    const int probePointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < probePointCount; ++i) {
        zVec3 transformed =
            TransformPointByMatrix(
                masterModalData->probePoints[kPlayerEnvProbeBasePointOffset + i],
                playerState->motionBasis
            );
        if (masterModalData->masterType != kPlayerMasterTypeSub) {
            transformed.y += probeYAdvance;
        }
        primaryModalState->transformedProbePointWorldByIndex[i] = transformed;
    }

    float maxRiseWindow = 1.0f - probeYAdvance;
    if (maxRiseWindow > 4.0f) {
        maxRiseWindow = 4.0f;
    }

    zClass_Class::gwNodeSetCellPickable(
        playerState->rootNode,
        0
    );
    zClass_Class::gwNodeSetCellPickable(
        globalPlayerState->rootNode,
        0
    );
    g_Variant_CurrentTag = playerState->variantTag;

    PlayerProbeSampleCandidateBuffer candidateBuffers[PLAYER_MAX_MODAL_PROBE_POINTS] = {0};
    zClass_cls_di::BuildPickCandidatesForPointBatch(
        g_Player_RuntimeDiScene,
        primaryModalState->transformedProbePointWorldByIndex,
        probePointCount,
        500.0f,
        candidateBuffers
    );

    g_Variant_CurrentTag = g_VariantTag_Current;
    zClass_Class::gwNodeSetCellPickable(
        playerState->rootNode,
        1
    );
    zClass_Class::gwNodeSetCellPickable(
        globalPlayerState->rootNode,
        1
    );

    *outBestHeight = -300.0f;
    *outAttachmentCandidateCount = 0;

    for (int sampleIndex = 0; sampleIndex < probePointCount; ++sampleIndex) {
        int bestCandidateIndex = 0;
        int selectedImpactSlot = 0;
        float taggedHeight = -300.0f;
        PlayerProbeSampleCandidateBuffer *const candidateBuffer = &candidateBuffers[sampleIndex];
        const float sampleHeight =
            primaryModalState->transformedProbePointWorldByIndex[sampleIndex].y;

        outSampleHeightByPoint[sampleIndex] = SelectProbeSampleHeightFromCandidates(
            candidateBuffer,
            &bestCandidateIndex,
            sampleHeight,
            maxRiseWindow,
            preferAttachmentSlot1,
            &selectedImpactSlot,
            &taggedHeight
        );

        if (*outBestHeight < taggedHeight) {
            *outBestHeight = taggedHeight;
        }

        if (sampleIndex == 0) {
            if (candidateBuffers[0].candidateCount <= 0) {
                zClass_Class::gwNodeSetNodeType(
                    playerState->rootNode,
                    0xff
                );
            } else {
                const zClassDiPickCandidateEntry *const selectedCandidate =
                    &candidateBuffers[0].entries[bestCandidateIndex];
                playerState->selectedProbeSample = *selectedCandidate;
                playerState->selectedProbeSample.hitPos.x =
                    primaryModalState->transformedProbePointWorldByIndex[0].x;
                playerState->selectedProbeSample.hitPos.z =
                    primaryModalState->transformedProbePointWorldByIndex[0].z;
                playerState->variantTag = selectedCandidate->variantTag;

                zClass_NodePartial *const worldChild =
                    zClass_Class::gwNodeGetWorldChild(selectedCandidate->node);
                if (worldChild != 0) {
                    zClass_Class::gwNodeSetNodeType(
                        playerState->rootNode,
                        worldChild->nodeType
                    );
                } else {
                    zClass_Class::gwNodeSetNodeType(
                        playerState->rootNode,
                        selectedCandidate->variantTag.tags[0]
                    );
                }
            }
        }

        outTypeHistogram->countByImpactSlot[selectedImpactSlot] += 1;

        if (candidateBuffer->candidateCount != 0) {
            zClass_NodePartial *const candidateNode =
                candidateBuffer->entries[bestCandidateIndex].node;
            if (candidateNode != 0 && candidateNode->auxFlags != 0) {
                *outAttachmentCandidateCount += 1;
                *outAttachmentNode = (zClass_NodePartial *)(candidateNode->callbackContext);
            }
        }
    }

    playerState->probeImpactSlot1SeenFlag = outTypeHistogram->countByImpactSlot[1] > 0 ? 1 : 0;
    playerState->probeImpactSlot4SeenFlag = outTypeHistogram->countByImpactSlot[4] > 0 ? 1 : 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-selectprobesampleheightfromcandidates
 * @recoil-artifact defines .text recoil:function:0x4290f0: Player::SelectProbeSampleHeightFromCandidates.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SelectProbeSampleHeightFromCandidates from the recovered
 * Battlesport gameplay source file.
 */
float __fastcall SelectProbeSampleHeightFromCandidates(
    PlayerProbeSampleCandidateBuffer *candidateBuffer,
    int *outBestCandidateIndex,
    float sampleHeight,
    float maxRiseWindow,
    int preferAttachmentSlot1,
    int *outSelectedImpactSlot,
    float *outTaggedHeight
) {
    float selectedHeight = -250.0f;
    float nearestFallbackHeight = -300.0f;
    int selectedImpactSlot = 0;

    *outBestCandidateIndex = 0;
    *outSelectedImpactSlot = 0;
    *outTaggedHeight = -300.0f;

    const int candidateCount = candidateBuffer->candidateCount;
    if (candidateCount <= 0) {
        return sampleHeight;
    }

    float bestAbsDelta = 10000.9f;
    for (int i = 0; i < candidateCount; ++i) {
        zClassDiPickCandidateEntry *const candidate = &candidateBuffer->entries[i];
        const float candidateHeight = candidate->hitPos.y;
        int impactSlot = 0;
        if (candidate->scenePayload != 0) {
            impactSlot = ((zModel_MaterialPartial *)candidate->scenePayload)->userTag;
        }

        if (impactSlot != 0) {
            *outTaggedHeight = candidateHeight;
            selectedImpactSlot = impactSlot;
            if (preferAttachmentSlot1 != 0 && impactSlot == 1) {
                continue;
            }
        }

        const float absDelta = (float)(fabs(candidateHeight - sampleHeight));
        if (absDelta < bestAbsDelta) {
            bestAbsDelta = absDelta;
            nearestFallbackHeight = candidateHeight;
        }

        if (candidateHeight > selectedHeight && candidateHeight - maxRiseWindow <= sampleHeight) {
            selectedHeight = candidateHeight;
            *outBestCandidateIndex = i;
        }
    }

    if (*outTaggedHeight + maxRiseWindow >= sampleHeight) {
        *outSelectedImpactSlot = selectedImpactSlot;
    }

    if (selectedHeight == -250.0f && nearestFallbackHeight != -300.0f) {
        return nearestFallbackHeight;
    }
    if (selectedHeight <= -250.0f) {
        return -250.0f;
    }
    return selectedHeight;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applyamphibspeedoscillation
 * @recoil-artifact defines .text recoil:function:0x429240: Player::ApplyAmphibSpeedOscillation.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ApplyAmphibSpeedOscillation from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ApplyAmphibSpeedOscillation(
    zUtil_SaveGameState *saveState,
    zVec3 *inOutUpVector,
    int includeYawCoupling
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    const float speedAbs = (float)(fabs(playerState->localVel.z));
    const float pitchArg = (masterModalData->hoverPitchWaveSpeedRate * speedAbs +
                               masterModalData->hoverPitchWaveBaseRate) *
                           g_Time_AccumulatedTimeSec;
    const float rollArg = (masterModalData->hoverRollWaveSpeedRate * speedAbs +
                              masterModalData->hoverRollWaveBaseRate) *
                          g_Time_AccumulatedTimeSec;

    const float pitchAngle = (float)(sin(pitchArg)) * masterModalData->hoverPitchWaveAmplitude;
    float rollAngle = (float)(sin(rollArg)) * masterModalData->hoverRollWaveAmplitude;
    if (includeYawCoupling != 0) {
        rollAngle += playerState->angVelYaw * masterModalData->hoverRollYawCoupleScale *
                     playerState->localVel.z;
    }

    const float yawSin = -playerState->steerBasisNorm.x;
    const float yawCos = -playerState->steerBasisNorm.z;
    const float pitchSin = (float)(sin(pitchAngle));
    const float pitchCos = (float)(cos(pitchAngle));
    const float rollSin = (float)(sin(rollAngle));
    const float rollCos = (float)(cos(rollAngle));

    zMat4x3 oscillationBasis = {0};
    oscillationBasis.xx = yawSin * pitchSin * rollSin + rollCos * yawCos;
    oscillationBasis.xy = rollSin * pitchCos;
    oscillationBasis.xz = rollSin * yawCos * pitchSin - rollCos * yawSin;
    oscillationBasis.yx = yawSin * pitchSin * rollCos - rollSin * yawCos;
    oscillationBasis.yy = rollCos * pitchCos;
    oscillationBasis.yz = rollCos * yawCos * pitchSin + rollSin * yawSin;
    oscillationBasis.zx = yawSin * pitchCos;
    oscillationBasis.zy = -pitchSin;
    oscillationBasis.zz = yawCos * pitchCos;

    const zVec3 original = *inOutUpVector;
    inOutUpVector->x = original.x * oscillationBasis.xx + original.y * oscillationBasis.yx +
                       original.z * oscillationBasis.zx;
    inOutUpVector->y = original.x * oscillationBasis.xy + original.y * oscillationBasis.yy +
                       original.z * oscillationBasis.zy;
    inOutUpVector->z = original.x * oscillationBasis.xz + original.y * oscillationBasis.yz +
                       original.z * oscillationBasis.zz;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applypitchrollvelocityimpulsefromdirection
 * @recoil-artifact defines .text recoil:function:0x429430: Player::ApplyPitchRollVelocityImpulseFromDirection
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: transform an incoming hit direction into player-local space and
 * apply the matching pitch/roll and local X/Z velocity impulse.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a standalone C++ Player class owner.
 * Evidence: status names this address-backed helper; body loads the root-node
 * 3x3 rotation, transforms one direction vector, then applies the scaled local
 * X/Z components to vehicle pitch, roll, and local velocity.
 */
void __fastcall ApplyPitchRollVelocityImpulseFromDirection(
    zUtil_SaveGameState *saveState,
    const zVec3 *direction,
    float angleScale,
    float velocityScale
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 localDirection = *direction;

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadRotationFrom3x3(
        (const zMat4x3 *)(zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode))
    );
    zMath::Vec3ArrayTransformDirection(
        &localDirection,
        1
    );
    zMath::MatStackPopPtr();

    playerState->vehiclePitchRad -= localDirection.z * angleScale;
    playerState->vehicleRollRad += localDirection.x * angleScale;
    playerState->localVel.x -= localDirection.x * velocityScale;
    playerState->localVel.z -= localDirection.z * velocityScale;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildsteerbasisfrommotionbasis
 * @recoil-artifact defines .text recoil:function:0x4294d0: Player::RebuildSteerBasisFromMotionBasis.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::RebuildSteerBasisFromMotionBasis from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RebuildSteerBasisFromMotionBasis(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    playerState->steerBasisRaw.x = -playerState->motionBasis.zx;
    playerState->steerBasisRaw.y = -playerState->motionBasis.zy;
    playerState->steerBasisRaw.z = -playerState->motionBasis.zz;

    playerState->steerBasisRef.x = playerState->motionBasis.yx;
    playerState->steerBasisRef.y = playerState->motionBasis.yy;
    playerState->steerBasisRef.z = playerState->motionBasis.yz;

    playerState->steerBasisNorm = playerState->steerBasisRaw;
    playerState->steerBasisNorm.y = 0.0f;
    zMath::Vec3NormalizeXZ(
        &playerState->steerBasisNorm,
        &playerState->steerBasisNorm
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildsteerbasisfrommotionaxes
 * @recoil-artifact defines .text recoil:function:0x429560: Player::RebuildSteerBasisFromMotionAxes.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::RebuildSteerBasisFromMotionAxes from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RebuildSteerBasisFromMotionAxes(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->autoTurnActive == 0) {
        return;
    }

    if (playerState->steeringInput != 0.0f) {
        playerState->autoTurnActive = 0;
        if (saveState == g_LocalPlayerSaveState) {
            ApplyCameraState(playerState->previousCameraState);
        }
    }

    if (playerState->autoTurnActive == 0) {
        return;
    }

    const float cross = playerState->steerBasisNorm.z * playerState->autoTurnTargetDir.x -
                        playerState->autoTurnTargetDir.z * playerState->steerBasisNorm.x;
    const float dot = playerState->steerBasisNorm.z * playerState->autoTurnTargetDir.z +
                      playerState->autoTurnTargetDir.x * playerState->steerBasisNorm.x;
    if (dot < (float)(cos(g_Player_DeltaTime * masterModalData->yawRateMax))) {
        const int turnSign = cross < 0.0f ? -1 : 1;
        const float turnSignFloat = (float)(turnSign);
        playerState->steeringInput = turnSignFloat;
        playerState->steeringInputCopy = turnSignFloat;
        playerState->angVelYaw = turnSignFloat * masterModalData->yawRateMax;

        if (saveState == g_LocalPlayerSaveState && playerState->lifecycleState != 2) {
            zVec3 normalizedCursor = {0};
            HudUiMgr::ProjectPointToNormalizedClamped(
                &playerState->autoTurnTargetWorldPos,
                &normalizedCursor
            );
            playerState->autoTurnCursorNormX = normalizedCursor.x;
            playerState->autoTurnCursorNormY = normalizedCursor.y;
            zInput::Mouse_SetNormalizedCursorPos(
                normalizedCursor.x,
                normalizedCursor.y
            );

            float autoTurnCursorLerpStep = g_FrameDeltaTimeSec * -2.0f;
            int lerpBits = (int)(autoTurnCursorLerpStep * 12102200.0f);
            lerpBits += 0x3f800000;
            float lerpFactor = 0.0f;
            memcpy(
                &lerpFactor,
                &lerpBits,
                sizeof(lerpFactor)
            );
            zMath::Vec3Lerp(
                &playerState->cameraLerpStart,
                &playerState->cameraLerpEnd,
                lerpFactor
            );
        }
        return;
    }

    playerState->thirdPersonYawOffset = 0.0f;
    playerState->cameraDirFlat = playerState->cameraDir;
    playerState->cameraDirFlat.y = 0.0f;
    zMath::Vec3NormalizeXZ(
        &playerState->cameraDirFlat,
        &playerState->cameraDirFlat
    );

    if (saveState == (zUtil_SaveGameState *)(g_GameStateOrMapTable) &&
        saveState == g_LocalPlayerSaveState) {
        ApplyCameraState(playerState->previousCameraState);
        zInput::Mouse_RecenterCursorX();
    }

    playerState->restartYawRad =
        (float)(atan2(
            -playerState->autoTurnTargetDir.z,
            -playerState->autoTurnTargetDir.x
        ));
    playerState->autoTurnActive = 0;
    playerState->steeringInputCopy = 0.0f;
    playerState->angVelYaw = 0.0f;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updateautoturnandsteerfromtarget
 * @recoil-artifact defines .text recoil:function:0x429750: Player::UpdateAutoTurnAndSteerFromTarget
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: damp yaw angular velocity when steering is neutral, otherwise apply
 * steering yaw acceleration and clamp it to the active yaw velocity limit.
 * Source owner: proposed Player auto-turn yaw steering helper; owner/data gates
 * are still pending outside this docblock-only edit.
 * Evidence: status names this address-backed helper; body branches on steering
 * input, builds the recovered yaw-damping scale, zeroes opposing yaw velocity,
 * accumulates yaw acceleration from steering input, and clamps angVelYaw.
 */
void __fastcall UpdateAutoTurnAndSteerFromTarget(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->steeringInput == 0.0f) {
        float dampingScale = masterModalData->yawDamping * g_Player_DeltaTime;
        dampingScale = -dampingScale;
        int dampingBits = (int)(dampingScale * 12102200.0f);
        const int dampingFloatBits = dampingBits + 0x3f800000;

        float dampingFactor = 0.0f;
        memcpy(
            &dampingFactor,
            &dampingFloatBits,
            sizeof(dampingFactor)
        );
        playerState->angVelYaw *= dampingFactor;
        return;
    }

    if ((playerState->steeringInputCopy > 0.0f && playerState->angVelYaw < 0.0f) ||
        (playerState->steeringInputCopy < 0.0f && playerState->angVelYaw > 0.0f)) {
        playerState->angVelYaw = 0.0f;
    }

    const float newYawVelocity =
        masterModalData->yawAccel * g_Player_DeltaTime * playerState->steeringInputCopy +
        playerState->angVelYaw;
    playerState->angVelYaw = newYawVelocity;

    if (newYawVelocity > playerState->yawVelocityLimit) {
        playerState->angVelYaw = playerState->yawVelocityLimit;
    } else if (newYawVelocity < -playerState->yawVelocityLimit) {
        playerState->angVelYaw = -playerState->yawVelocityLimit;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updateyawvelocityfromsteerinput
 * @recoil-artifact defines .text recoil:function:0x429870: Player::UpdateYawVelocityFromSteerInput.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::UpdateYawVelocityFromSteerInput from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateYawVelocityFromSteerInput(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    if (fabs(playerState->localVel.x) < g_Player_DeltaTimeScaled001) {
        playerState->localVel.x = 0.0f;
    }

    if (fabs(playerState->localVel.z) < g_Player_DeltaTimeScaled001) {
        playerState->localVel.z = 0.0f;
    }

    if (playerState->slipSfxActive != 0) {
        ComputeTurnSlipDelta(saveState);
        return;
    }

    if (playerState->throttleInput != 0.0f) {
        float dampingScale = masterModalData->rateDampingDecel * g_Player_DeltaTime;
        dampingScale = -dampingScale;
        int dampingBits = (int)(dampingScale * 12102200.0f);
        const int dampingFloatBits = dampingBits + 0x3f800000;

        float dampingFactor = 0.0f;
        memcpy(
            &dampingFactor,
            &dampingFloatBits,
            sizeof(dampingFactor)
        );
        playerState->localVel.z *= dampingFactor;
    } else {
        if ((playerState->throttleInputCopy > 0.0f && playerState->localVel.z > 0.0f) ||
            (playerState->throttleInputCopy < 0.0f && playerState->localVel.z < 0.0f)) {
            float dampingScale = masterModalData->rateDampingDecel * g_Player_DeltaTime;
            dampingScale = -dampingScale;
            int dampingBits = (int)(dampingScale * 12102200.0f);
            const int dampingFloatBits = dampingBits + 0x3f800000;

            float dampingFactor = 0.0f;
            memcpy(
                &dampingFactor,
                &dampingFloatBits,
                sizeof(dampingFactor)
            );
            playerState->localVel.z *= dampingFactor;
        }

        playerState->localVel.z -=
            masterModalData->accelRate * g_Player_DeltaTime * playerState->throttleInputCopy;
        const float velocityLimit =
            (float)(fabs(playerState->throttleInputCopy)) * playerState->axisClampRuntime;
        if (playerState->localVel.z > velocityLimit) {
            playerState->localVel.z = velocityLimit;
        } else if (playerState->localVel.z < -velocityLimit) {
            playerState->localVel.z = -velocityLimit;
        }
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        const float residual = UpdateBankAndTurnDynamics(saveState);
        if (residual != 0.0f) {
            const float oldLocalX = playerState->localVel.x;
            float localX = oldLocalX + residual * g_Player_DeltaTime;

            if (localX > playerState->axisClampRuntime) {
                localX = playerState->axisClampRuntime;
            } else if (localX < -playerState->axisClampRuntime) {
                localX = -playerState->axisClampRuntime;
            }

            if (oldLocalX != 0.0f && FloatSign(localX) != FloatSign(oldLocalX)) {
                playerState->localVel.x = 0.0f;
                return;
            }

            playerState->localVel.x = localX;
            return;
        }
    }

    if (playerState->localVel.x != 0.0f) {
        float dampingScale = masterModalData->rateDampingAccel * g_Player_DeltaTime;
        dampingScale = -dampingScale;
        int dampingBits = (int)(dampingScale * 12102200.0f);
        const int dampingFloatBits = dampingBits + 0x3f800000;

        float dampingFactor = 0.0f;
        memcpy(
            &dampingFactor,
            &dampingFloatBits,
            sizeof(dampingFactor)
        );
        playerState->localVel.x *= dampingFactor;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatebankandturndynamics
 * @recoil-artifact defines .text recoil:function:0x429b40: Player::UpdateBankAndTurnDynamics.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::UpdateBankAndTurnDynamics from the recovered
 * Battlesport gameplay source file.
 */
float __fastcall UpdateBankAndTurnDynamics(
    zUtil_SaveGameState *saveState
) {
    if (g_Player_DeltaTime < 0.0000001) {
        return 0.0f;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    const float crossYaw = playerState->steerBasisNorm.x * playerState->bankBasis.z -
                           playerState->steerBasisNorm.z * playerState->bankBasis.x;
    const float slipDelta = crossYaw * -playerState->localVel.z * g_Player_InvDeltaTime +
                            playerState->motionBasis.xy * -28.0f;

    float residual = 0.0f;
    if (playerState->localVel.x == 0.0f) {
        if (fabs(slipDelta) <= masterModalData->frictionStatic) {
            return residual;
        }

        const int sign = slipDelta < 0.0f ? -1 : 1;
        residual = slipDelta - (float)(sign)*masterModalData->frictionStatic;
        StartSlipSfx(saveState);
        return residual;
    }

    residual =
        slipDelta - (float)(FloatSign(playerState->localVel.x)) * masterModalData->frictionDynamic;

    if (playerState->throttleInputCopy != 0.0f &&
        FloatSign(playerState->steeringInputCopy) == FloatSign(playerState->restartYawRad)) {
        const int residualSign = residual < 0.0f ? -1 : 1;
        const int velocitySign = playerState->localVel.x < 0.0f ? -1 : 1;
        if (residualSign != velocitySign) {
            residual = 0.0f;
        }
    }

    if (playerState->slipSfxActive == 0 && fabs(slipDelta) > masterModalData->frictionStatic) {
        StartSlipSfx(saveState);
    }

    return residual;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computeturnslipdelta
 * @recoil-artifact defines .text recoil:function:0x429d30: Player::ComputeTurnSlipDelta.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ComputeTurnSlipDelta from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ComputeTurnSlipDelta(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    playerState->localVel = playerState->projectileSpawnVel;

    const zVec3 localVel = playerState->localVel;
    const zMat4x3 &motionBasis = playerState->motionBasis;
    playerState->localVel.x =
        localVel.x * motionBasis.xx + localVel.y * motionBasis.xy + localVel.z * motionBasis.xz;
    playerState->localVel.y =
        localVel.x * motionBasis.yx + localVel.y * motionBasis.yy + localVel.z * motionBasis.yz;
    playerState->localVel.z =
        localVel.x * motionBasis.zx + localVel.y * motionBasis.zy + localVel.z * motionBasis.zz;

    const float axisClampRuntime = playerState->axisClampRuntime;
    playerState->localVel.z -=
        masterModalData->accelRate * playerState->throttleInputCopy * g_Player_DeltaTime;
    if (playerState->localVel.z > axisClampRuntime) {
        playerState->localVel.z = axisClampRuntime;
    } else if (playerState->localVel.z < -axisClampRuntime) {
        playerState->localVel.z = -axisClampRuntime;
    }

    float localX =
        playerState->localVel.x + UpdateBankAndTurnDynamics(saveState) * g_Player_DeltaTime;
    if (playerState->localVel.x != 0.0f) {
        const int oldSign = playerState->localVel.x < 0.0f ? -1 : 1;
        const int newSign = localX < 0.0f ? -1 : 1;
        if (oldSign != newSign) {
            localX = 0.0f;
            StopSlipSfx(saveState);
        }
    }

    playerState->localVel.x = localX;
    if (localX > axisClampRuntime) {
        playerState->localVel.x = axisClampRuntime;
    } else if (localX < -axisClampRuntime) {
        playerState->localVel.x = -axisClampRuntime;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-startslipsfx
 * @recoil-artifact defines .text recoil:function:0x429ed0: Player::StartSlipSfx.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::StartSlipSfx from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall StartSlipSfx(
    zUtil_SaveGameState *saveState
) {
    saveState->playerState->slipSfxActive = 1;
    saveState->StartModalLoopSfxHandle(
        3,
        1.0f
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-stopslipsfx
 * @recoil-artifact defines .text recoil:function:0x429ef0: Player::StopSlipSfx.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::StopSlipSfx from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall StopSlipSfx(
    zUtil_SaveGameState *saveState
) {
    saveState->playerState->slipSfxActive = 0;
    saveState->StopModalLoopSfxHandle(3);
}
} // namespace Player
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Binary Ninja shows this static initializer calls the bind-group list default
 * constructor and tail-calls the atexit registration wrapper.
 * Purpose: Initializes the bind-group vector global and registers its cleanup.
 */
int __cdecl BindGroupList_StaticInitAndRegisterAtExit() {
    BindGroupListStaticInit();
    return BindGroupListRegisterAtExit();
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Binary Ninja identifies this as the global vector default construction for
 * g_zInput_BindGroupInfoList; the saved-ECX allocator byte write is a compiler
 * artifact and the source-level owner is the typed bind-group global.
 * Purpose: Default-constructs the global bind-group pointer vector storage.
 */
void BindGroupListStaticInit() {
    zInput_BindGroupInfoListAllocator allocator;
#if !defined(_MSC_VER) || _MSC_VER >= 1200
    allocator.value = 0;
#endif
    g_zInput_BindGroupInfoList.allocatorProxy = allocator;
    g_zInput_BindGroupInfoList.begin = 0;
    g_zInput_BindGroupInfoList.end = 0;
    g_zInput_BindGroupInfoList.capacity = 0;
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Binary Ninja tail registers BindGroupListAtExitDestructor with atexit.
 * Purpose: Registers the bind-group global vector cleanup callback.
 */
int BindGroupListRegisterAtExit() {
    return atexit(BindGroupListAtExitDestructor);
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Binary Ninja shows the VC5 std::vector<T*> destructor shape: an inlined
 * _Destroy(first,end) range over pointer elements, allocator buffer free, then
 * cleared begin/end/capacity. Pointer elements have no destructor, but the
 * optimized range still accounts for the saved first-iterator scratch slot.
 * Purpose: Releases the global bind-group pointer vector buffer at process exit.
 */
void __cdecl BindGroupListAtExitDestructor() {
    for (
        zInput_BindGroupInfo **first = g_zInput_BindGroupInfoList.begin;
        first != g_zInput_BindGroupInfoList.end;
        ++first
    ) {
        // VC5 std::vector<T*>::_Destroy visits pointer elements with no body.
    }
    ::operator delete(g_zInput_BindGroupInfoList.begin);
    g_zInput_BindGroupInfoList.begin = 0;
    g_zInput_BindGroupInfoList.end = 0;
    g_zInput_BindGroupInfoList.capacity = 0;
}
} // namespace zInput
namespace zInput {
/**
 * Purpose: Destroys active bind-group records and resets the vector end pointer.
 */
void __cdecl BindGroupList_Clear() {
#if defined(_MSC_VER) && _MSC_VER < 1200
    zInput_BindGroupInfoStdVector *groups =
        (zInput_BindGroupInfoStdVector *)(&g_zInput_BindGroupInfoList);
    zInput_BindGroupInfoStdVector::iterator cursor = groups->begin();
    zInput_BindGroupInfoStdVector::iterator last = groups->end();
    while (cursor != last) {
        zInput_BindGroupInfo *const group = *cursor;
        if (group != 0) {
            group->Destroy();
            ::operator delete(group);
        }
        *cursor = 0;
        ++cursor;
    }

    groups->erase(groups->begin(), groups->end());
#else
    zInput_BindGroupInfo **first = g_zInput_BindGroupInfoList.begin;
    zInput_BindGroupInfo **last = g_zInput_BindGroupInfoList.end;
    zInput_BindGroupInfo **cursor = first;
    while (cursor != last) {
        const int zeroOffset = (int)(first - cursor);
        zInput_BindGroupInfo *const group = *cursor;
        if (group != 0) {
            group->Destroy();
            ::operator delete(group);
        }
        cursor[zeroOffset] = 0;
        ++cursor;
    }

    zInput_BindGroupInfo **copy = g_zInput_BindGroupInfoList.end;
    zInput_BindGroupInfo **result = g_zInput_BindGroupInfoList.begin;
    zInput_BindGroupInfo **finish = g_zInput_BindGroupInfoList.end;
    while (copy != finish) {
        *result = *copy;
        ++copy;
        ++result;
    }
    g_zInput_BindGroupInfoList.end = result;
#endif
}
} // namespace zInput
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_cmd.cpp.
 * Binary Ninja shows the VC EH-framed record destructor empties the CString,
 * deletes commandIds storage, clears the vector triplet, then destroys title.
 * Purpose: Releases a bind-group record's CString title and command-id vector.
 */
void zInput_BindGroupInfo::Destroy() {
    title.Empty();
    ::operator delete(commandIds.begin);
    commandIds.begin = 0;
    commandIds.end = 0;
    commandIds.capacity = 0;
    title.CString::~CString();
}
namespace zInput {
/**
 * Purpose: Allocates a bind-group record and appends it to the global vector.
 */
int __fastcall BindGroupList_AddGroup(
    const char *title
) {
    zInput_BindGroupInfo **begin = g_zInput_BindGroupInfoList.begin;
    const int groupIndex = begin != 0 ? (int)(g_zInput_BindGroupInfoList.end - begin) : 0;

    zInput_BindGroupInfo *group = new zInput_BindGroupInfo;
    group->commandIds.allocatorByte = 0;
    group->commandIds.begin = 0;
    group->commandIds.end = 0;
    group->commandIds.capacity = 0;
    group->title = title;

    zInput_BindGroupInfo **end = g_zInput_BindGroupInfoList.end;
    zInput_BindGroupInfo **const capacity = g_zInput_BindGroupInfoList.capacity;
    if (end != 0 && capacity != 0 && capacity - end >= 1) {
        *end = group;
        g_zInput_BindGroupInfoList.end = end + 1;
        return groupIndex;
    }

    const int count = begin != 0 ? (int)(end - begin) : 0;
    const int growth = count > 1 ? count : 1;
    const int newCapacity = count + growth;
    zInput_BindGroupInfo **const newBegin = (zInput_BindGroupInfo **)(::operator new(
        (size_t)(newCapacity) * sizeof(zInput_BindGroupInfo *)
    ));

    for (int i = 0; i < count; ++i) {
        newBegin[i] = begin[i];
    }
    newBegin[count] = group;

    ::operator delete(begin);
    g_zInput_BindGroupInfoList.begin = newBegin;
    g_zInput_BindGroupInfoList.end = newBegin + count + 1;
    g_zInput_BindGroupInfoList.capacity = newBegin + newCapacity;
    return groupIndex;
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_cmd.cpp.
 * Binary Ninja shows the VC vector append-at-end template for the selected
 * group's commandIds storage; the source model is the typed command-id vector,
 * not a raw offset or copied template scaffold.
 * Purpose: Appends a command id to the selected bind group's command-id vector.
 */
void __fastcall BindGroupList_AddCommandToGroup(
    int groupIndex,
    int commandId
) {
    zInput_BindGroupInfo *const group = g_zInput_BindGroupInfoList.begin[groupIndex];

    int *begin = group->commandIds.begin;
    int *end = group->commandIds.end;
    int *const capacity = group->commandIds.capacity;
    if (end != 0 && capacity != 0 && capacity - end >= 1) {
        *end = commandId;
        group->commandIds.end = end + 1;
        return;
    }

    const int count = begin != 0 ? (int)(end - begin) : 0;
    const int growth = count > 1 ? count : 1;
    const int newCapacity = count + growth;
    int *const newBegin = (int *)(::operator new((size_t)(newCapacity) * sizeof(int)));

    for (int i = 0; i < count; ++i) {
        newBegin[i] = begin[i];
    }
    newBegin[count] = commandId;

    ::operator delete(begin);
    group->commandIds.begin = newBegin;
    group->commandIds.end = newBegin + count + 1;
    group->commandIds.capacity = newBegin + newCapacity;
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_cmd.cpp.
 * Binary Ninja reads the global bind-group vector begin/end pointers and
 * returns zero when begin is null.
 * Purpose: Returns the number of active bind groups in the global vector.
 */
int __cdecl BindGroupList_GetCount() {
    zInput_BindGroupInfo **const begin = g_zInput_BindGroupInfoList.begin;
    if (begin == 0) {
        return 0;
    }

    return (int)(g_zInput_BindGroupInfoList.end - begin);
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_cmd.cpp.
 * Binary Ninja indexes g_zInput_BindGroupInfoList and returns the CString
 * buffer pointer from the selected group title.
 * Purpose: Returns the CString buffer for the selected bind-group title.
 */
char *__fastcall BindGroupList_GetGroupTitle(
    int groupIndex
) {
    zInput_BindGroupInfo **const groups = g_zInput_BindGroupInfoList.begin;
    zInput_BindGroupInfo *const group = groups[groupIndex];
    return (char *)(LPCTSTR)(group->title);
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_cmd.cpp.
 * Binary Ninja indexes the accepted global bind-group vector, selects the
 * embedded commandIds vector, and returns zero for a null command begin.
 * Purpose: Returns the number of command ids stored in a bind group.
 */
int __fastcall BindGroupList_GetGroupCommandCount(
    int groupIndex
) {
    zInput_BindGroupInfo *const group = g_zInput_BindGroupInfoList.begin[groupIndex];
    zInput_CommandIdVector *const commandIds = &group->commandIds;
    int *const begin = commandIds->begin;
    if (begin == 0) {
        return 0;
    }

    return (int)(commandIds->end - begin);
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_cmd.cpp.
 * Binary Ninja indexes the accepted global bind-group vector and then indexes
 * the selected record's embedded commandIds begin pointer.
 * Purpose: Returns one command id from a bind group's command-id vector.
 */
int __fastcall BindGroupList_GetGroupCommandId(
    int groupIndex,
    int commandIndex
) {
    zInput_BindGroupInfo *const group = g_zInput_BindGroupInfoList.begin[groupIndex];
    int *const begin = group->commandIds.begin;
    return begin[commandIndex];
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_bindmap.cpp.
 * Binary Ninja indexes g_zInput_CommandLocIdTable by command id and tail-calls
 * zLoc::GetMessageString for the command's localized label.
 * Purpose: Resolve a bind-map command id to its localized display label.
 */
char *__fastcall BindMap_GetCommandLabel(
    int commandId
) {
    return zLoc::GetMessageString(g_zInput_CommandLocIdTable[commandId]);
}
} // namespace zInput
namespace zInput {
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_bindmap.cpp.
 * Binary Ninja indexes g_zInput_CommandLocIdTable by command id, increments
 * the recovered localization id, and tail-calls zLoc::GetMessageString for
 * the command hint.
 * Purpose: Resolve a bind-map command id to its localized hint text.
 */
char *__fastcall BindMap_GetCommandHint(
    int commandId
) {
    return zLoc::GetMessageString(g_zInput_CommandLocIdTable[commandId] + 1);
}
} // namespace zInput
namespace zInput {
/**
 * Purpose: Add one localized default command binding to the active bind map and bind-group list.
 */
void __fastcall BindMap_AddDefaultBinding(
    int commandId,
    int messageId,
    int primaryKey,
    int secondaryKey,
    int joystickSlot,
    int mouseSlot
) {
    const int boundCommandId = BindMap_Current_SetBindingRecord(
        commandId,
        zLoc::GetMessageString(messageId),
        primaryKey,
        secondaryKey,
        joystickSlot,
        mouseSlot
    );
    BindGroupList_AddCommandToGroup(
        g_zInput_CurrentBindGroupIndex,
        boundCommandId
    );
    g_zInput_CommandLocIdTable[commandId] = messageId;
}
} // namespace zInput
namespace zInput {
/**
 * Purpose: Clear the bind-group list and seed the retail default command bindings.
 */
int __cdecl BindMap_InitDefaultBindings() {
    BindGroupList_Clear();
    g_zInput_CurrentBindGroupIndex = BindGroupList_AddGroup(zLoc::GetMessageString(0x750));
    BindMap_AddDefaultBinding(0x04, 0x806, 0x0c8, 0, 0, 0);
    BindMap_AddDefaultBinding(0x01, 0x800, 0x0d0, 0, 0, 0);
    BindMap_AddDefaultBinding(0x02, 0x802, 0x0cb, 0, 0, 0);
    BindMap_AddDefaultBinding(0x03, 0x804, 0x0cd, 0, 0, 0);
    BindMap_AddDefaultBinding(0x2b, 0x8c6, 0x01f, 0, 0, 0);
    BindMap_AddDefaultBinding(0x25, 0x874, 0x03b, 0, 0, 0);
    BindMap_AddDefaultBinding(0x26, 0x876, 0x03c, 0, 0, 0);
    BindMap_AddDefaultBinding(0x27, 0x878, 0x03d, 0, 0, 0);
    BindMap_AddDefaultBinding(0x28, 0x87a, 0x03e, 0, 0, 0);
    BindMap_AddDefaultBinding(0x05, 0x80e, 0x01e, 0, 0, 0);
    BindMap_AddDefaultBinding(0x06, 0x810, 0x02c, 0, 0, 0);
    BindMap_AddDefaultBinding(0x07, 0x82a, 0x02e, 0, 6, 0);
    BindMap_AddDefaultBinding(0x08, 0x82c, 0x02b, 0, 5, 0);
    BindMap_AddDefaultBinding(0x09, 0x872, 0x030, 0, 0, 0);
    BindMap_AddDefaultBinding(0x0a, 0x8c2, 0x230, 0, 0, 0);

    g_zInput_CurrentBindGroupIndex = BindGroupList_AddGroup(zLoc::GetMessageString(0x751));
    BindMap_AddDefaultBinding(0x0b, 0x88c, 0, 0, 1, 1);
    BindMap_AddDefaultBinding(0x0c, 0x88e, 0, 0, 2, 2);
    BindMap_AddDefaultBinding(0x0d, 0x8b8, 0x039, 0, 3, 0);
    BindMap_AddDefaultBinding(0x0f, 0x812, 0x002, 0x04f, 0, 0);
    BindMap_AddDefaultBinding(0x10, 0x814, 0x003, 0x050, 0, 0);
    BindMap_AddDefaultBinding(0x11, 0x816, 0x004, 0x051, 0, 0);
    BindMap_AddDefaultBinding(0x12, 0x818, 0x005, 0x04b, 0, 0);
    BindMap_AddDefaultBinding(0x13, 0x81a, 0x006, 0x04c, 0, 0);
    BindMap_AddDefaultBinding(0x14, 0x81c, 0x007, 0x04d, 0, 0);
    BindMap_AddDefaultBinding(0x15, 0x81e, 0x008, 0x047, 0, 0);
    BindMap_AddDefaultBinding(0x16, 0x820, 0x009, 0x048, 0, 0);
    BindMap_AddDefaultBinding(0x17, 0x822, 0x00a, 0x049, 0, 0);

    g_zInput_CurrentBindGroupIndex = BindGroupList_AddGroup(zLoc::GetMessageString(0x752));
    BindMap_AddDefaultBinding(0x1e, 0x84e, 0x02f, 0, 0, 0);
    BindMap_AddDefaultBinding(0x20, 0x888, 0x03f, 0, 0, 0);
    BindMap_AddDefaultBinding(0x21, 0x8a6, 0x040, 0, 0, 0);
    BindMap_AddDefaultBinding(0x22, 0x8a8, 0x041, 0, 0, 0);

    g_zInput_CurrentBindGroupIndex = BindGroupList_AddGroup(zLoc::GetMessageString(0x753));
    BindMap_AddDefaultBinding(0x19, 0x8a4, 0x013, 0, 0, 0);
    BindMap_AddDefaultBinding(0x18, 0x826, 0x018, 0, 0, 0);
    BindMap_AddDefaultBinding(0x1a, 0x8c4, 0x011, 0, 0, 0);

    g_zInput_CurrentBindGroupIndex = BindGroupList_AddGroup(zLoc::GetMessageString(0x754));
    BindMap_AddDefaultBinding(0x2d, 0x8b6, 0x042, 0, 0, 0);
    BindMap_AddDefaultBinding(0x2c, 0x8b4, 0x043, 0, 0, 0);
    BindMap_AddDefaultBinding(0x2a, 0x8bc, 0x014, 0, 0, 0);
    BindMap_AddDefaultBinding(0x1b, 0x864, 0x032, 0, 0, 0);
    BindMap_AddDefaultBinding(0x1c, 0x866, 0x034, 0, 0, 0);
    BindMap_AddDefaultBinding(0x1d, 0x868, 0x033, 0, 0, 0);
    BindMap_AddDefaultBinding(0x23, 0x88a, 0x22d, 0, 0, 0);

    BindMap_Current_SetBindingRecord(
        0x24,
        zLoc::GetMessageString(0x83c),
        0x418,
        0,
        0,
        0
    );
    BindMap_Current_SetBindingRecord(
        0x1f,
        zLoc::GetMessageString(0x850),
        0x22,
        0,
        0,
        0
    );
    return 1;
}
} // namespace zInput
/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_cmd.cpp.
 * Binary Ninja reads begin at offset 4, returns zero when begin is null, and
 * otherwise returns the end-begin pointer distance divided by four.
 * Purpose: Returns the number of bind-group pointers stored in the VC vector.
 */
int zInput_BindGroupInfoVec::Count() {
    zInput_BindGroupInfo **const begin = this->begin;
    if (begin == 0) {
        return 0;
    }

    return (int)(end - begin);
}
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-addscaledhudcountervalue
 * @recoil-artifact defines .text recoil:function:0x42a9f0: Player::AddScaledHudCounterValue.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: scale a HUD objective counter contribution by active primary-gun
 * dispatch count and add it to the mission HUD counter accumulator.
 */
void __fastcall AddScaledHudCounterValue(
    float value
) {
    float scale = 1.0f;
    if (g_HudSensorTracker.primaryGunDispatchCount > 0) {
        scale = (float)(g_OptCatalog_DamageFeedbackHitCount) /
                (float)(g_HudSensorTracker.primaryGunDispatchCount);
    }

    g_Player_HudCounterValue += (int)(value * scale * 1000.0f);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-getsavestatelisthead
 * @recoil-artifact defines .text recoil:function:0x42aa40: Player::GetSaveStateListHead
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: return the global head of the player save-state list.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
zUtil_SaveGameState *GetSaveStateListHead() {
    return g_PlayerSaveStateListHead;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatedebugoverlayhud
 * @recoil-artifact defines .text recoil:function:0x42aa50: Player::UpdateDebugOverlayHud.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: refresh weapon HUD values, objective counter text, and the debug
 * overlay lines for the current player save state.
 */
void __fastcall UpdateDebugOverlayHud(
    zUtil_SaveGameState *saveState,
    int unusedActiveMode2Count,
    int unusedTotalMode2Count
) {
    (void)unusedActiveMode2Count;
    (void)unusedTotalMode2Count;

    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const altController = playerState->activeAltGunController;
    const int reticleMode =
        altController->optCatalogEntry->range > playerState->aimTargetDistanceApprox &&
                altController->ammoOrCharge != 0.0f
            ? 1
            : 0;
    HudUiMgr::SetReticleMode(reticleMode);

    HudUiMessage::SetValueIfOwnerMatches(
        altController->weaponBankIndex,
        altController->weaponSideIndex,
        altController->ammoOrCharge
    );

    PlayerGunFireController *const primaryController = playerState->activePrimaryGunController;
    if (primaryController != 0) {
        HudUiMessage::SetValueIfOwnerMatches(
            primaryController->weaponBankIndex,
            primaryController->weaponSideIndex,
            primaryController->ammoOrCharge
        );
    }

    HudUiMgrObjective::RefreshCounterText(g_Player_HudCounterValue);

    char masterTypeName[12];
    strcpy(
        masterTypeName,
        PlayerDebugMasterTypeName(saveState->primaryModalState->masterModalData->masterType)
    );

    char debugLine[256];
    const char *const rootName = playerState->rootNode->name;
    if (playerState->lifecycleState == kPlayerLifecycleAi) {
        sprintf(
            debugLine,
            g_Player_HudReadoutFmt_ModeGoalNode,
            rootName,
            playerState->aiTopLevelState,
            playerState->aiCurrentPathNode->nodeIndex
        );
    } else if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        sprintf(
            debugLine,
            g_Player_HudReadoutFmt_Dead,
            rootName
        );
    } else if (playerState->lifecycleState == kPlayerLifecycleLocal ||
               playerState->lifecycleState == 0) {
        if (playerState->airborneFlag != 0) {
            sprintf(
                debugLine,
                g_Player_HudReadoutFmt_DynamicsA,
                rootName,
                masterTypeName
            );
        } else if (playerState->slipSfxActive != 0) {
            sprintf(
                debugLine,
                g_Player_HudReadoutFmt_DynamicsS,
                rootName,
                masterTypeName
            );
        } else {
            sprintf(
                debugLine,
                g_Player_HudReadoutFmt_Dynamics,
                rootName,
                masterTypeName
            );
        }
    }

    HudUiAuxOverlay::UpdateTextLine(
        2,
        1,
        debugLine
    );

    sprintf(
        debugLine,
        g_Player_HudReadoutFmt_PosYaw,
        (int)(playerState->worldPos.x),
        (int)(playerState->worldPos.y),
        (int)(playerState->worldPos.z),
        (int)((double)(playerState->restartYawRad) * kPlayerRadiansToDegrees)
    );
    HudUiAuxOverlay::UpdateTextLine(
        2,
        2,
        debugLine
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypetrack
 * @recoil-artifact defines .text recoil:function:0x42ac90: Player::TransitionToMasterTypeTrack
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: enter track mode after cooldown and source-mode transition rules
 * allow it.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation matches the known Player modal/state
 * model with SUB/HOVER/AMPHIB source gates, underwater HUD and copter sound
 * cleanup, source FX dispatch, mode variant activation, HUD counter update,
 * stale amphib light stop, track node action, and transition light handle
 * creation.
 */
int __fastcall TransitionToMasterTypeTrack(
    zUtil_SaveGameState *saveState,
    int flags
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }

    const int sourceMasterType = masterModalData->masterType;
    if (sourceMasterType == kPlayerMasterTypeSub) {
        if (flags == 0) {
            return 0;
        }

        zClass_Object3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerA.attachNodePrimary,
            0.0f,
            0.0f,
            0.0f
        );
        zClass_Object3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerA.attachNodeSecondary,
            0.0f,
            0.0f,
            0.0f
        );
        zClass_Object3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerB.attachNodePrimary,
            0.0f,
            0.0f,
            0.0f
        );
        zClass_Object3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerB.attachNodeSecondary,
            0.0f,
            0.0f,
            0.0f
        );
        SetHudUiElementVisible(
            &g_Player_UnderwaterFxPass3Ui,
            0
        );
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        ReactivateCopterSndNodesIfHealthy();

        zClass_NodePartial *const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(
                nodeCaustic1,
                &displayInstanceValue
            );
            zDi::SetCurrentVariantCycleTextureSpeed(
                (zDiPartial *)displayInstanceValue,
                0.0f
            );
        }

        playerState->damageVisualFlag = 1;
    } else if (sourceMasterType == kPlayerMasterTypeHover) {
        if (playerState->autoTurnSign != 0) {
            return 0;
        }

        TriggerZeroVelocityFxList(
            masterModalData->fxList_fromHoverToTrack,
            playerState->rootNode,
            flags
        );
    } else if (sourceMasterType == kPlayerMasterTypeAmphib) {
        TriggerZeroVelocityFxList(
            masterModalData->fxList_fromAmphibToTrack,
            playerState->rootNode,
            flags
        );
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeTrack);
    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeTrackCooldownSec;
    zClass_Class::gwNodeSetActive(
        playerState->modeVariantNode,
        1
    );

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x238),
            5.0f
        );
        HudUiMgr::SetModeCounterState(
            0,
            2
        );
    }

    zEffectAnimEntry *const toAmphibLightHandle =
        playerState->masterTypeTransitionToAmphibLightHandle;
    if (toAmphibLightHandle != 0) {
        zEffectAnim::Stop(toAmphibLightHandle);
        playerState->masterTypeTransitionToAmphibLightHandle = 0;
    }

    zEffect_Anim::NodeActionCallback(
        playerState->masterTypeTransitionToTrackNodeAction,
        playerState->rootNode
    );
    playerState->masterTypeTransitionToTrackLightHandle = zEffectAnim::SetVelocity_Thunk(
        playerState->masterTypeTransitionToTrackNodeAction,
        playerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypeamphib
 * @recoil-artifact defines .text recoil:function:0x42aeb0: Player::TransitionToMasterTypeAmphib
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: enter amphib mode when unlocked, off cooldown, and accepted by the
 * source-mode transition rules.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation preserves the fastcall-plus-stack source
 * shape for transition and extra flags, amphib unlock/cooldown guards, SUB
 * cleanup and FX path, TRACK/HOVER source FX paths, modal selection, pitch/roll
 * reset, HUD counter update, stale track light stop, and amphib light start.
 */
int __fastcall TransitionToMasterTypeAmphib(
    zUtil_SaveGameState *saveState,
    int transitionFlags,
    int extraFlags
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }
    if (playerState->amphibUnlocked == 0) {
        return 0;
    }

    const int sourceMasterType = masterModalData->masterType;
    if (sourceMasterType == kPlayerMasterTypeSub) {
        if (transitionFlags != 0) {
            return 0;
        }

        SetHudUiElementVisible(
            &g_Player_UnderwaterFxPass3Ui,
            0
        );
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        ReactivateCopterSndNodesIfHealthy();

        zClass_NodePartial *const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(
                nodeCaustic1,
                &displayInstanceValue
            );
            zDi::SetCurrentVariantCycleTextureSpeed(
                (zDiPartial *)displayInstanceValue,
                0.0f
            );
        }

        StopBftBubbleFxHandle(saveState);
        TriggerZeroVelocityFxList(
            masterModalData->fxList_fromSubToAmphib,
            playerState->rootNode,
            extraFlags
        );
        playerState->damageVisualFlag = 1;
    } else if (sourceMasterType == kPlayerMasterTypeTrack) {
        playerState->airborneFlag = 0;
        zClass_NodePartial *const modalNode = primaryModalState->modalNode;
        if (modalNode != 0) {
            zClass_Object3D::gwObject3DSetRotation(
                modalNode,
                0.0f,
                0.0f,
                0.0f
            );
        }
        zClass_Class::gwNodeSetActive(
            playerState->modeVariantNode,
            1
        );
        TriggerZeroVelocityFxList(
            masterModalData->fxList_fromTrackToAmphib,
            playerState->rootNode,
            extraFlags
        );
    } else if (sourceMasterType == kPlayerMasterTypeHover) {
        TriggerZeroVelocityFxList(
            masterModalData->fxList_fromHoverToAmphib,
            playerState->rootNode,
            extraFlags
        );
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeAmphib);
    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeTrackCooldownSec;

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x239),
            5.0f
        );
        HudUiMgr::SetModeCounterState(
            1,
            2
        );
    }

    playerState->vehicleRollRad = 0.0f;
    playerState->vehiclePitchRad = 0.0f;

    zEffectAnimEntry *const toTrackLightHandle =
        playerState->masterTypeTransitionToTrackLightHandle;
    if (toTrackLightHandle != 0) {
        zEffectAnim::Stop(toTrackLightHandle);
        playerState->masterTypeTransitionToTrackLightHandle = 0;
    }

    zEffect_Anim::NodeActionCallback(
        playerState->masterTypeTransitionToAmphibNodeAction,
        playerState->rootNode
    );
    playerState->masterTypeTransitionToAmphibLightHandle = zEffectAnim::SetVelocity_Thunk(
        playerState->masterTypeTransitionToAmphibNodeAction,
        playerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypehover
 * @recoil-artifact defines .text recoil:function:0x42b0f0: Player::TransitionToMasterTypeHover
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: enter hover mode when unlocked, off cooldown, and accepted by the
 * source-mode transition rules.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation follows the Player modal/state source
 * model with hover unlock/cooldown guards, SUB cleanup and damage visual latch,
 * TRACK rotation and airborne reset, AMPHIB/HOVER FX paths, modal selection,
 * one-second cooldown update, and local HUD counter update.
 */
int __fastcall TransitionToMasterTypeHover(
    zUtil_SaveGameState *saveState,
    int flags
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }
    if (playerState->hoverUnlocked == 0) {
        return 0;
    }

    const int sourceMasterType = masterModalData->masterType;
    if (sourceMasterType == kPlayerMasterTypeSub) {
        if (flags == 0) {
            return 0;
        }

        SetHudUiElementVisible(
            &g_Player_UnderwaterFxPass3Ui,
            0
        );
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        ReactivateCopterSndNodesIfHealthy();

        zClass_NodePartial *const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(
                nodeCaustic1,
                &displayInstanceValue
            );
            zDi::SetCurrentVariantCycleTextureSpeed(
                (zDiPartial *)displayInstanceValue,
                0.0f
            );
        }

        playerState->damageVisualFlag = 1;
    } else if (sourceMasterType == kPlayerMasterTypeTrack) {
        playerState->airborneFlag = 0;
        zClass_NodePartial *const modalNode = primaryModalState->modalNode;
        if (modalNode != 0) {
            zClass_Object3D::gwObject3DSetRotation(
                modalNode,
                0.0f,
                0.0f,
                0.0f
            );
        }
        zClass_Class::gwNodeSetActive(
            playerState->modeVariantNode,
            1
        );
        TriggerZeroVelocityFxList(
            masterModalData->fxList_fromTrackToHover,
            playerState->rootNode,
            flags
        );
    } else if (sourceMasterType == kPlayerMasterTypeAmphib) {
        TriggerZeroVelocityFxList(
            masterModalData->fxList_fromAmphibToHover,
            playerState->rootNode,
            flags
        );
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeHover);
    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeTrackCooldownSec;

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x23a),
            5.0f
        );
        HudUiMgr::SetModeCounterState(
            2,
            2
        );
    }

    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypesub
 * @recoil-artifact defines .text recoil:function:0x42b2a0: Player::TransitionToMasterTypeSub
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: enter sub mode after applying gun-slot offsets, transition gates,
 * source-mode cleanup, modal selection, alternate-weapon validation, and FX
 * updates.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation matches the known save-state and player
 * state model with damage visual latching before gates, cooldown/sub unlock
 * exits, SUB/TRACK/AMPHIB source rules, forced descent nudge, alt-weapon
 * fallback, loop SFX and copter sound handling, stale light stops, and sub
 * transition light creation.
 */
int __fastcall TransitionToMasterTypeSub(
    zUtil_SaveGameState *saveState,
    int flags
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    playerState->damageVisualFlag = 1;
    ApplyGunFireSlotOffsetToNode(saveState);

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }
    if (playerState->subUnlocked == 0) {
        return 0;
    }

    const int sourceMasterType = masterModalData->masterType;
    if (sourceMasterType == kPlayerMasterTypeSub) {
        if (flags == 0) {
            return 1;
        }

        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
    } else if (sourceMasterType == kPlayerMasterTypeTrack) {
        if (flags == 0) {
            return 0;
        }

        playerState->airborneFlag = 0;
        zClass_NodePartial *const modalNode = primaryModalState->modalNode;
        if (modalNode != 0) {
            zClass_Object3D::gwObject3DSetRotation(
                modalNode,
                0.0f,
                0.0f,
                0.0f
            );
        }

        playerState->localVel.y = -3.0f;
        playerState->worldPos.y -= 4.0999999f;
    } else if (sourceMasterType == kPlayerMasterTypeAmphib) {
        if (playerState->bankInput != 0 && flags == 0) {
            return 0;
        }

        TriggerZeroVelocityFxList(
            masterModalData->fxList_fromAmphibToSub,
            playerState->rootNode,
            flags
        );
        playerState->localVel.y = -3.0f;
        playerState->worldPos.y -= 4.0999999f;
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeSub);

    if (Player::IsAltWeaponAllowedInCurrentMasterMode(
            saveState,
            playerState->activeAltGunController->optCatalogEntry
        ) == 0) {
        Player::AutoSwitchToNextUsableAltWeapon(saveState);
    }

    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeTrackCooldownSec;

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x23b),
            5.0f
        );
        HudUiMgr::SetModeCounterState(
            3,
            2
        );
        saveState->EnsureMasterTypeLoopSfxHandle(
            kPlayerMasterTypeTrack,
            0.5f
        );
        CacheDisableCopterSndNodesAndStopSample();
    }

    zEffectAnimEntry *const toTrackLightHandle =
        playerState->masterTypeTransitionToTrackLightHandle;
    if (toTrackLightHandle != 0) {
        zEffectAnim::Stop(toTrackLightHandle);
        playerState->masterTypeTransitionToTrackLightHandle = 0;
    }

    zEffectAnimEntry *const toAmphibLightHandle =
        playerState->masterTypeTransitionToAmphibLightHandle;
    if (toAmphibLightHandle != 0) {
        zEffectAnim::Stop(toAmphibLightHandle);
        playerState->masterTypeTransitionToAmphibLightHandle = 0;
    }

    zEffect_Anim::NodeActionCallback(
        playerState->masterTypeTransitionToSubNodeAction,
        playerState->rootNode
    );
    playerState->masterTypeTransitionToSubLightHandle = zEffectAnim::SetVelocity_Thunk(
        playerState->masterTypeTransitionToSubNodeAction,
        playerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-stopbftbubblefxhandle
 * @recoil-artifact defines .text recoil:function:0x42b4a0: Player::StopBftBubbleFxHandle.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::StopBftBubbleFxHandle from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall StopBftBubbleFxHandle(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zEffectAnimEntry *const handle = playerState->masterTypeTransitionToSubLightHandle;
    if (handle != 0) {
        zEffectAnim::Stop(handle);
        playerState->masterTypeTransitionToSubLightHandle = 0;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypefly
 * @recoil-artifact defines .text recoil:function:0x42b4c0: Player::TransitionToMasterTypeFly
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: select the fly modal state when the master-type transition cooldown
 * allows it.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation follows the reviewed Player save-state
 * model: cooldown guard, SUB-source damage visual latch, source master-type
 * capture, fly modal selection, five-second cooldown update, and integer
 * success/failure return.
 */
int __fastcall TransitionToMasterTypeFly(
    zUtil_SaveGameState *saveState,
    int flags
) {
    (void)flags;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }

    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        playerState->damageVisualFlag = 1;
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeFly);
    playerState->masterTypeTransitionCooldownUntilTime =
        g_Time_AccumulatedTimeSec + kPlayerMasterTypeFlyCooldownSec;
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applymastertypetransition
 * @recoil-artifact defines .text recoil:function:0x42b520: Player::ApplyMasterTypeTransition
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reset the primary-gun gate timestamp and dispatch a requested
 * master type to the concrete transition helper.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation preserves the dispatcher source shape:
 * writes primaryGunGateUntilTime from accumulated time, maps FLY/SUB/TRACK/
 * HOVER/AMPHIB cases to the reviewed transition helpers, passes AMPHIB
 * transitionFlags as zero with caller flags as extraFlags, and returns
 * masterType - 1 for unsupported requests.
 */
int __fastcall ApplyMasterTypeTransition(
    zUtil_SaveGameState *saveState,
    int masterType,
    int flags
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->primaryGunGateUntilTime = g_Time_AccumulatedTimeSec;

    switch (masterType) {
    case kPlayerMasterTypeFly:
        return TransitionToMasterTypeFly(
            saveState,
            flags
        );
    case kPlayerMasterTypeSub:
        return TransitionToMasterTypeSub(
            saveState,
            flags
        );
    case kPlayerMasterTypeTrack:
        return TransitionToMasterTypeTrack(
            saveState,
            flags
        );
    case kPlayerMasterTypeHover:
        return TransitionToMasterTypeHover(
            saveState,
            flags
        );
    case kPlayerMasterTypeAmphib:
        return TransitionToMasterTypeAmphib(
            saveState,
            0,
            flags
        );
    default:
        return masterType - 1;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-reactivatecoptersndnodesifhealthy
 * @recoil-artifact defines .text recoil:function:0x42b5a0: Player::ReactivateCopterSndNodesIfHealthy
 * Purpose: reactivate each cached copter sound node whose healthy node remains
 * active, then restart the cached chopper sample through the node play handle.
 */
void ReactivateCopterSndNodesIfHealthy() {
    zClass_NodePartial *const healthyNode1 = g_Player_CopterHealthyNode1;
    if (healthyNode1 != 0 && (healthyNode1->flags & 0x04) != 0) {
        zClass_NodePartial *const sndNode1 = g_Player_CopterSndNode1;
        if (sndNode1 != 0) {
            zClass_Class::gwNodeSetActive(
                sndNode1,
                1
            );

            zClass_SoundDataPartial *const soundData =
                (zClass_SoundDataPartial *)(sndNode1->classData);
            if (soundData != 0) {
                zSndPlayHandle *const playHandle = soundData->playHandle;
                if (playHandle != 0) {
                    zSndPlayHandle::PlayWithDelta_BackendDispatch(
                        g_Player_CopterSndSample,
                        playHandle,
                        0,
                        0.0f
                    );
                }
            }
        }
    }

    zClass_NodePartial *const healthyNode2 = g_Player_CopterHealthyNode2;
    if (healthyNode2 != 0 && (healthyNode2->flags & 0x04) != 0) {
        zClass_NodePartial *const sndNode2 = g_Player_CopterSndNode2;
        if (sndNode2 != 0) {
            zClass_Class::gwNodeSetActive(
                sndNode2,
                1
            );

            zClass_SoundDataPartial *const soundData =
                (zClass_SoundDataPartial *)(sndNode2->classData);
            if (soundData != 0) {
                zSndPlayHandle *const playHandle = soundData->playHandle;
                if (playHandle != 0) {
                    zSndPlayHandle::PlayWithDelta_BackendDispatch(
                        g_Player_CopterSndSample,
                        playHandle,
                        0,
                        0.0f
                    );
                }
            }
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-cachedisablecoptersndnodesandstopsample
 * @recoil-artifact defines .text recoil:function:0x42b630: Player::CacheDisableCopterSndNodesAndStopSample
 * Purpose: lazily cache the two copter healthy/sound scene nodes, disable the
 * sound nodes, and stop active chopper sample voices.
 */
void CacheDisableCopterSndNodesAndStopSample() {
    if (g_Player_CopterSndNode1 == 0) {
        zClass_NodePartial *const copterRoot = zClass::FindByTypeAndName(
            6,
            g_Player_CopterTypeName01
        );
        if (copterRoot != 0) {
            g_Player_CopterHealthyNode1 = zClass_Class::FindSubNodeByName(
                copterRoot,
                g_Player_HealthySubNodeName
            );
            g_Player_CopterSndNode1 = zClass_Class::FindSubNodeByName(
                copterRoot,
                g_Player_CopterSndName
            );
        }
    }

    if (g_Player_CopterSndNode2 == 0) {
        zClass_NodePartial *const copterRoot = zClass::FindByTypeAndName(
            6,
            g_Player_CopterTypeName02
        );
        if (copterRoot != 0) {
            g_Player_CopterHealthyNode2 = zClass_Class::FindSubNodeByName(
                copterRoot,
                g_Player_HealthySubNodeName
            );
            g_Player_CopterSndNode2 = zClass_Class::FindSubNodeByName(
                copterRoot,
                g_Player_CopterSndName
            );
        }
    }

    if (g_Player_CopterSndNode1 != 0) {
        zClass_Class::gwNodeSetActive(
            g_Player_CopterSndNode1,
            0
        );
    }
    if (g_Player_CopterSndNode2 != 0) {
        zClass_Class::gwNodeSetActive(
            g_Player_CopterSndNode2,
            0
        );
    }

    g_Player_CopterSndSample->StopActiveVoicesIfPlaying();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-findnearestthirdpersoncameraprobepoint
 * @recoil-artifact defines .text recoil:function:0x42b6e0: Player::FindNearestThirdPersonCameraProbePoint.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::FindNearestThirdPersonCameraProbePoint from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall FindNearestThirdPersonCameraProbePoint(
    PlayerProbeSampleCandidateBuffer *batches,
    int batchCount,
    const zVec3 *referencePos,
    zVec3 *outHitPos
) {
    int found = 0;
    int bestBatchIndex = 0;
    int bestEntryIndex = 0;

    for (int batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        PlayerProbeSampleCandidateBuffer *const batch = &batches[batchIndex];
        for (int hitIndex = 0; hitIndex < batch->candidateCount; ++hitIndex) {
            if (batch->entries[hitIndex].node != 0) {
                bestBatchIndex = batchIndex;
                bestEntryIndex = hitIndex;
                found = 1;
                batchIndex = batchCount;
                break;
            }
        }
    }

    if (found == 0) {
        return 0;
    }

    float bestDistSq = zMath::Vec3DeltaLengthSq(
        &batches[bestBatchIndex].entries[bestEntryIndex].hitPos,
        referencePos
    );

    for (int searchBatchIndex = 0; searchBatchIndex < batchCount; ++searchBatchIndex) {
        PlayerProbeSampleCandidateBuffer *const batch = &batches[searchBatchIndex];
        for (int hitIndex = 0; hitIndex < batch->candidateCount; ++hitIndex) {
            zClassDiPickCandidateEntry *const candidate = &batch->entries[hitIndex];
            if (candidate->node != 0) {
                const float distSq = zMath::Vec3DeltaLengthSq(
                    &candidate->hitPos,
                    referencePos
                );
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    bestEntryIndex = hitIndex;
                    bestBatchIndex = searchBatchIndex;
                }
            }
        }
    }

    *outHitPos = batches[bestBatchIndex].entries[bestEntryIndex].hitPos;
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-synclocalposefromrootnode
 * @recoil-artifact defines .text recoil:function:0x42b810: Player::SyncLocalPoseFromRootNode.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SyncLocalPoseFromRootNode from the recovered
 * Battlesport gameplay source file.
 */
void SyncLocalPoseFromRootNode() {
    zUtil_PlayerStateStorage *const playerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;

    zClass_Object3D::gwObject3DGetPosition(
        playerState->rootNode,
        &playerState->worldPos.x,
        &playerState->worldPos.y,
        &playerState->worldPos.z
    );
    zClass_Object3D::gwObject3DGetRotation(
        playerState->rootNode,
        &playerState->vehiclePitchRad,
        &playerState->restartYawRad,
        &playerState->vehicleRollRad
    );
    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->lifecycleState = 1;
    playerState->previousTransform = playerState->motionBasis;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildsteerbasisrawfromref
 * @recoil-artifact defines .text recoil:function:0x42b8c0: Player::RebuildSteerBasisRawFromRef.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::RebuildSteerBasisRawFromRef from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RebuildSteerBasisRawFromRef(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->steerBasisRef.y == 0.0f) {
        return;
    }

    zVec3 rawBasis = playerState->steerBasisNorm;
    rawBasis.y =
        -((playerState->steerBasisRef.x * playerState->steerBasisNorm.x +
              playerState->steerBasisRef.z * playerState->steerBasisNorm.z) /
            playerState->steerBasisRef.y);
    zMath::Vec3Normalize(&rawBasis);
    playerState->steerBasisRaw = rawBasis;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildmotionbasisfromsteerbasis
 * @recoil-artifact defines .text recoil:function:0x42b970: Player::RebuildMotionBasisFromSteerBasis.
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::RebuildMotionBasisFromSteerBasis from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RebuildMotionBasisFromSteerBasis(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zVec3 basisSide = {0};
    basisSide.x = playerState->steerBasisRaw.y * playerState->steerBasisRef.z -
                  playerState->steerBasisRaw.z * playerState->steerBasisRef.y;
    basisSide.y = playerState->steerBasisRaw.z * playerState->steerBasisRef.x -
                  playerState->steerBasisRaw.x * playerState->steerBasisRef.z;
    basisSide.z = playerState->steerBasisRaw.x * playerState->steerBasisRef.y -
                  playerState->steerBasisRaw.y * playerState->steerBasisRef.x;

    zMat4x3 motionBasis = {0};
    motionBasis.xx = basisSide.x;
    motionBasis.xy = basisSide.y;
    motionBasis.xz = basisSide.z;
    motionBasis.yx = playerState->steerBasisRef.x;
    motionBasis.yy = playerState->steerBasisRef.y;
    motionBasis.yz = playerState->steerBasisRef.z;
    motionBasis.zx = -playerState->steerBasisRaw.x;
    motionBasis.zy = -playerState->steerBasisRaw.y;
    motionBasis.zz = -playerState->steerBasisRaw.z;
    motionBasis.posX = playerState->worldPos.x;
    motionBasis.posY = playerState->worldPos.y;
    motionBasis.posZ = playerState->worldPos.z;

    playerState->motionBasis = motionBasis;
}
} // namespace Player
namespace zClass_cls_di {
/**
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall SnapProbePointYToBestCandidate(zVec3 * point) {
        PlayerProbeSampleCandidateBuffer candidateBuffer;
        const int result = BuildPickCandidateListBelowPoint(
            g_Player_RuntimeDiScene,
            &candidateBuffer,
            point->x,
            500.0f,
            point->z
        );
        int selectedImpactSlot;
        int bestCandidateIndex;
        float taggedHeight;
        point->y = Player::SelectProbeSampleHeightFromCandidates(
            &candidateBuffer,
            &bestCandidateIndex,
            point->y,
            2.0f,
            0,
            &selectedImpactSlot,
            &taggedHeight
        );
        return result;
    }
} // namespace zClass_cls_di
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-setautoturntargetdirfromworldpoint
 * @recoil-artifact defines .text recoil:function:0x42bab0: Player::SetAutoTurnTargetDirFromWorldPoint.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SetAutoTurnTargetDirFromWorldPoint from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall SetAutoTurnTargetDirFromWorldPoint(
    zUtil_SaveGameState *saveState,
    const zVec3 *worldPoint
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zVec3 targetDir = {0};
    targetDir.x = worldPoint->x - playerState->worldPos.x;
    targetDir.y = worldPoint->y - playerState->worldPos.y;
    targetDir.z = worldPoint->z - playerState->worldPos.z;
    targetDir.y = 0.0f;

    zVec3 normalizedTargetDir = {0};
    zMath::Vec3NormalizeXZ(
        &targetDir,
        &normalizedTargetDir
    );
    playerState->autoTurnTargetDir = normalizedTargetDir;
    playerState->autoTurnActive = 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-asynccommandcallback
 * @recoil-artifact defines .text recoil:function:0x42bb30: Player::AsyncCommandCallback
 * Purpose: Dispatches script async command events that toggle HUD/gameplay
 * state, apply debug damage, and spawn debug pickup carrier nodes.
 */
void __fastcall AsyncCommandCallback(
    zEffectAnimEntry *animEntry,
    void *,
    int eventCode
) {
    zUtil_SaveGameState *const localSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;

    switch (eventCode) {
    case 0:
        if (animEntry == g_Player_ActiveDebugScriptAsyncEntry) {
            g_Player_ActiveDebugScriptAsyncEntry = 0;
        }
        return;

    case 1:
        g_Player_RebuildCameraDirFlatFromCurrentTarget = 1;
        BindActiveGameStateAsCurrentSaveState();
        return;

    case 2:
        UnbindCurrentSaveStateIfSinglePlayer();
        HudUiMgr::DisableHud();
        HudUiMgr::UpdateTargetReticleFromCursor(
            0,
            0,
            0.0f,
            0.0f
        );
        HudUiMgr::DisableTopAndChatStacks();
        return;

    case 10:
        SyncLocalPoseFromRootNode();
        HudUiMgr::EnableTopAndChatStacks();
        return;

    case 11:
        if (zOpt::GetNetworkEnabled() == 0) {
            localSaveState->playerState->lifecycleState = kPlayerLifecycleState6Inactive;
            localSaveState->UpdateModalLoopSfx(0);
        }
        return;

    case 14:
        if (zOpt::GetNetworkEnabled() == 0) {
            g_Player_LocalControlEnabled = 0;
            HudUiMgr::DisableHud();
            HudUiMgr::UpdateTargetReticleFromCursor(
                0,
                0,
                0.0f,
                0.0f
            );
            HudUiTimerPanel::SetRunning(0);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
        }
        zTurret_System::DisableTickCallback();
        return;

    case 15:
        if (zOpt::GetNetworkEnabled() == 0) {
            g_Player_LocalControlEnabled = 1;
            if (zOpt::GetHudVisibilityOption() != 0) {
                HudUiMgr::ApplyHudModeSwitch(zOpt::GetHudTypeForCurrentHwMode());
                HudUiMgr::EnableHud();
            }
            HudUiMgr::UpdateTargetReticleFromCursor(
                1,
                0,
                0.5f,
                0.5f
            );
            HudUi::ShowTopMessageLine(
                localSaveState->playerState->activeAltGunController->optCatalogEntry->description,
                5.0f
            );
            HudUiTimerPanel::SetRunning(1);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
            if (g_HudSensorTracker.GetMissionId() == 1 &&
                g_HudSensorTracker.firstIncompleteObjectiveIndex == 0 &&
                g_HudSensorTracker.primaryGunDispatchCount == 0) {
                HudUi::PlayPowerupSfx(1);
            }
        }
        zTurret_System::EnableTickCallback();
        return;

    case 16:
        ResetMotionTransientState(localSaveState);
        return;

    case 17:
        CaptureCurrentObjectPoseAsRestartAnchor(localSaveState);
        return;

    case 20:
        g_Player_ActiveDebugScriptAsyncEntry = animEntry;
        return;

    case 25:
        localSaveState->playerState->nanitePanelLevel = 0;
        HudUiMgr::SetNanitePanelCount(0);
        EnterDestroyedState(
            localSaveState,
            0,
            0,
            localSaveState->playerState->statusMeterValue - -1.0f
        );
        return;

    case 26:
        EnterDestroyedState(
            localSaveState,
            0,
            0,
            localSaveState->playerState->statusMeterValue - -1.0f
        );
        return;

    case 27:
        EnterDestroyedState(
            localSaveState,
            0,
            0,
            10.0f
        );
        return;

    case 99:
        g_HudSensorTracker.SaveAndQueueMissionState();
        return;

    case 911:
        PickupAirdropSpawnRef::TrySpawnRandomPickupFromGlobal();
        return;

    case 912:
        Pickup::SpawnAtCarrierNodeByName(
            g_PickupOptKey_Vwbus,
            32,
            1
        );
        return;

    case 913:
        Pickup::SpawnAtCarrierNodeByName(
            g_PickupOptKey_Crbox,
            36,
            1
        );
        return;

    case 914:
        Pickup::SpawnAtCarrierNodeByName(
            g_PickupOptKey_Drop,
            30,
            1
        );
        return;

    default:
        return;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-setworldposeandrestartanchor
 * @recoil-artifact defines .text recoil:function:0x42be00: Player::SetWorldPoseAndRestartAnchor.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SetWorldPoseAndRestartAnchor from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall SetWorldPoseAndRestartAnchor(
    zUtil_SaveGameState *saveState,
    const zVec3 *position,
    float yawRad
) {
    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->worldPos.x = position->x;
    playerState->worldPos.y = position->y;
    playerState->worldPos.z = position->z;
    playerState->restartYawRad = yawRad;
    playerState->previousTransform.posX = position->x;
    playerState->previousTransform.posY = position->y;
    playerState->previousTransform.posZ = position->z;
    zTag4::Clear(&g_VariantTag_Current);
    g_Variant_CurrentTag = g_VariantTag_Current;
    playerState->variantTag = g_VariantTag_Current;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-capturecurrentobjectposeasrestartanchor
 * @recoil-artifact defines .text recoil:function:0x42be70: Player::CaptureCurrentObjectPoseAsRestartAnchor.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::CaptureCurrentObjectPoseAsRestartAnchor from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall CaptureCurrentObjectPoseAsRestartAnchor(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = g_LocalPlayerSaveState->playerState;

    zVec3 worldPos;
    zClass_Object3D::gwObject3DGetPosition(
        playerState->rootNode,
        &worldPos.x,
        &worldPos.y,
        &worldPos.z
    );

    float pitchRad;
    float yawRad;
    float rollRad;
    zClass_Object3D::gwObject3DGetRotation(
        playerState->rootNode,
        &pitchRad,
        &yawRad,
        &rollRad
    );

    SetWorldPoseAndRestartAnchor(
        saveState,
        &worldPos,
        yawRad
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resetmotiontransientstate
 * @recoil-artifact defines .text recoil:function:0x42bed0: Player::ResetMotionTransientState.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ResetMotionTransientState from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ResetMotionTransientState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->localVel.x = 0.0f;
    playerState->localVel.y = 0.0f;
    playerState->localVel.z = 0.0f;
    playerState->projectileSpawnVel.x = 0.0f;
    playerState->projectileSpawnVel.y = 0.0f;
    playerState->projectileSpawnVel.z = 0.0f;
    playerState->yawRotatedLocalVel.x = 0.0f;
    playerState->yawRotatedLocalVel.y = 0.0f;
    playerState->yawRotatedLocalVel.z = 0.0f;
    playerState->angVelPitch = 0.0f;
    playerState->angVelYaw = 0.0f;
    playerState->angVelRoll = 0.0f;
    playerState->steeringInput = 0.0f;
    playerState->throttleInput = 0.0f;
    playerState->subVerticalInput = 0.0f;
    playerState->subVerticalInputCopy = 0.0f;
    playerState->steeringInputCopy = 0.0f;
    playerState->throttleInputCopy = 0.0f;
}
} // namespace Player
namespace HudUi {
/**
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud.cpp.
 * Purpose: lazily resolve the powerup sound sample and play or stop its active voices.
 */
void __fastcall PlayPowerupSfx(
    int shouldPlay
) {
    static zSndSample *powerupSample = zSnd::FindSampleByName("snd_powerup");

    if (shouldPlay != 0) {
        powerupSample->PlayA3DSimple(1.0f);
        return;
    }

    powerupSample->StopActiveVoicesIfPlaying();
}
} // namespace HudUi
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatepostmoveenvironment
 * @recoil-artifact defines .text recoil:function:0x42bf90: Player::UpdatePostMoveEnvironment.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdatePostMoveEnvironment from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdatePostMoveEnvironment(
    zUtil_SaveGameState *saveState,
    int probeSampleCount
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    g_PlayerEnvProbeSampleCount = probeSampleCount;

    PlayerEnvProbeResult probeResult;
    memset(
        &probeResult,
        0,
        sizeof(probeResult)
    );
    probeResult.minProbeDepth = 4.0f;
    probeResult.preferAttachmentSlot1 = playerState->amphibUnlocked == 0 ? 1 : 0;

    const float restartYawRad = playerState->restartYawRad;
    playerState->vehiclePitchRad += playerState->angVelPitch * g_Player_DeltaTime;
    playerState->vehicleRollRad += playerState->angVelRoll * g_Player_DeltaTime;
    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        restartYawRad,
        playerState->vehicleRollRad
    );
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    RebuildSteerBasisFromMotionBasis(saveState);

    const float verticalVelocityAfterGravity =
        playerState->projectileSpawnVel.y - playerState->gravityAccel * g_Player_DeltaTime;
    playerState->projectileSpawnVel.y = verticalVelocityAfterGravity;
    const float advancedWorldPosY =
        playerState->worldPos.y + verticalVelocityAfterGravity * g_Player_DeltaTime;
    playerState->worldPos.y = advancedWorldPosY;
    playerState->motionBasis.posY = advancedWorldPosY;

    BuildEnvironmentProbeResult(
        saveState,
        &probeResult
    );
    if (ApplyEnvironmentProbeResult(
        saveState,
        &probeResult
    ) == 0) {
        return;
    }

    ProcessEnvProbeResults(
        saveState,
        &probeResult
    );
    RebuildOrientationFromNormal(saveState);
    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        playerState->airborneFlag == 0) {
        FindThirdProbeAndComputeNormal(
            saveState,
            &probeResult
        );
    }
    UpdateVerticalVelocityAndTransform(
        saveState,
        &probeResult
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-processenvproberesults
 * @recoil-artifact defines .text recoil:function:0x42c0d0: Player::ProcessEnvProbeResults.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ProcessEnvProbeResults from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ProcessEnvProbeResults(
    zUtil_SaveGameState *saveState,
    PlayerEnvProbeResult *probeResult
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const float supportDepthThreshold = g_Player_DeltaTime * 5.0f;
    g_PlayerEnvProbe_AboveGroundCount = 0;

    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        const int impactSlot = probeResult->impactSlotBySample[sampleIndex];
        if (impactSlot == 3) {
            probeResult->candidateScoreBySample[sampleIndex] -= g_Player_QuicksandSinkRate;
        }
        if (impactSlot == 4) {
            probeResult->candidateScoreBySample[sampleIndex] -= g_Player_LavaSinkRate;
        }

        if (g_PlayerEnvProbeWorldPoints[sampleIndex].y - supportDepthThreshold >
            probeResult->candidateScoreBySample[sampleIndex]) {
            g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] = 1;
            g_PlayerEnvProbe_AboveGroundIndices[g_PlayerEnvProbe_AboveGroundCount] = sampleIndex;
            ++g_PlayerEnvProbe_AboveGroundCount;
        } else {
            g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] = 0;
        }
    }

    const int aboveGroundSampleCount = g_PlayerEnvProbe_AboveGroundCount;
    if (aboveGroundSampleCount == 0) {
        playerState->airborneFlag = 1;
        const float unclampedPitchRecoveryVel =
            (playerState->vehiclePitchRad - -0.523599982f) * -0.699999988f;
        const float targetPitchRecoveryVel =
            unclampedPitchRecoveryVel <= -0.699999988f ? -0.699999988f : unclampedPitchRecoveryVel;
        const float targetRollRecoveryVel = playerState->vehicleRollRad * -0.699999988f;
        const float previousAngularVelocityBlendWeight = PlayerFloatFromBits(
            (int)(-saveState->primaryModalState->masterModalData->aDamping * g_Player_DeltaTime *
                  12102200.0f) +
            0x3f800000
        );
        const float newAngularVelocityBlendWeight = 1.0f - previousAngularVelocityBlendWeight;
        playerState->angVelPitch = previousAngularVelocityBlendWeight * playerState->angVelPitch +
                                   newAngularVelocityBlendWeight * targetPitchRecoveryVel;
        playerState->angVelRoll = previousAngularVelocityBlendWeight * playerState->angVelRoll +
                                  newAngularVelocityBlendWeight * targetRollRecoveryVel;
        return;
    }

    if (aboveGroundSampleCount == 1) {
        ComputeSurfaceFrom1Probe(
            saveState,
            probeResult
        );
        playerState->airborneFlag = 0;
        return;
    }

    if (aboveGroundSampleCount == 2) {
        ComputeSurfaceFrom2Probes(
            saveState,
            probeResult
        );
        playerState->airborneFlag = 0;
        return;
    }

    if (aboveGroundSampleCount != 3) {
        SelectBestProbesByDotProduct(
            &playerState->steerBasisRef,
            probeResult
        );
    }

    if (CheckProbeSampleMaskOverlap(
            g_PlayerEnvProbe_AboveGroundIndices[0],
            g_PlayerEnvProbe_AboveGroundIndices[1],
            g_PlayerEnvProbe_AboveGroundIndices[2]
        ) != 0) {
        ComputeSurfaceFrom2Probes(
            saveState,
            probeResult
        );
    } else {
        ComputeSurfaceFrom3Probes(
            saveState,
            probeResult
        );
    }
    playerState->airborneFlag = 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updateverticalvelocityandtransform
 * @recoil-artifact defines .text recoil:function:0x42c2e0: Player::UpdateVerticalVelocityAndTransform.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateVerticalVelocityAndTransform from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateVerticalVelocityAndTransform(
    zUtil_SaveGameState *saveState,
    PlayerEnvProbeResult *probeResult
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const float measuredFrameDeltaY =
        (playerState->worldPos.y - playerState->previousTransform.posY) * g_Player_InvDeltaTime;
    if (g_PlayerEnvProbe_AboveGroundCount >= 3) {
        playerState->projectileSpawnVel.y = measuredFrameDeltaY;
    } else {
        const float previousVerticalVelocityBlendWeight =
            PlayerFloatFromBits((int)(g_Player_DeltaTime * -5.0f * 12102200.0f) + 0x3f800000);
        playerState->projectileSpawnVel.y =
            previousVerticalVelocityBlendWeight * playerState->projectileSpawnVel.y +
            (1.0f - previousVerticalVelocityBlendWeight) * measuredFrameDeltaY;
    }

    AccumulateSlopeForces(
        saveState,
        probeResult
    );
    if (playerState->projectileSpawnVel.y > 55.0f) {
        playerState->projectileSpawnVel.y = 0.0f;
    }

    if (playerState->environmentAttachmentActive != 0) {
        return;
    }

    const zVec3 worldVelocity = playerState->projectileSpawnVel;
    playerState->localVel.x = worldVelocity.x * playerState->motionBasis.xx +
                              worldVelocity.y * playerState->motionBasis.xy +
                              worldVelocity.z * playerState->motionBasis.xz;
    playerState->localVel.y = worldVelocity.x * playerState->motionBasis.yx +
                              worldVelocity.y * playerState->motionBasis.yy +
                              worldVelocity.z * playerState->motionBasis.yz;
    playerState->localVel.z = worldVelocity.x * playerState->motionBasis.zx +
                              worldVelocity.y * playerState->motionBasis.zy +
                              worldVelocity.z * playerState->motionBasis.zz;
    if (g_PlayerEnvProbe_AboveGroundCount >= 3) {
        playerState->localVel.y = 0.0f;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-accumulateslopeforces
 * @recoil-artifact defines .text recoil:function:0x42c420: Player::AccumulateSlopeForces.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::AccumulateSlopeForces from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall AccumulateSlopeForces(
    zUtil_SaveGameState *saveState,
    PlayerEnvProbeResult *probeResult
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    int clampedAboveGroundCount = g_PlayerEnvProbe_AboveGroundCount;
    if (clampedAboveGroundCount >= 3) {
        clampedAboveGroundCount = 3;
    }

    for (int i = 0; i < clampedAboveGroundCount; ++i) {
        const int sampleIndex = g_PlayerEnvProbe_AboveGroundIndices[i];
        const int bestCandidateIndex = probeResult->bestIndexBySample[sampleIndex];
        const zVec3 &surfaceNormal =
            probeResult->candidateBuffers[sampleIndex].entries[bestCandidateIndex].surfaceNormal;
        if (surfaceNormal.y < g_Player_MaxSlope) {
            const float slopeScale = g_Player_DeltaTime * playerState->gravityAccel * 5.0f;
            playerState->projectileSpawnVel.x += surfaceNormal.x * slopeScale;
            playerState->projectileSpawnVel.y += (surfaceNormal.y - 1.0f) * slopeScale;
            playerState->projectileSpawnVel.z += surfaceNormal.z * slopeScale;
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computesurfacefrom1probe
 * @recoil-artifact defines .text recoil:function:0x42c520: Player::ComputeSurfaceFrom1Probe.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ComputeSurfaceFrom1Probe from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ComputeSurfaceFrom1Probe(
    zUtil_SaveGameState *saveState,
    PlayerEnvProbeResult *probeResult
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int sampleIndex = g_PlayerEnvProbe_AboveGroundIndices[0];
    zVec3 samplePoint = g_PlayerEnvProbeWorldPoints[sampleIndex];
    samplePoint.y = probeResult->candidateScoreBySample[sampleIndex];

    const float supportPlaneDot = playerState->steerBasisRef.x * samplePoint.x +
                                  playerState->steerBasisRef.y * samplePoint.y +
                                  playerState->steerBasisRef.z * samplePoint.z;
    playerState->worldPos.y = SolveHeightOnSurface(
        saveState,
        supportPlaneDot
    );

    const zVec3 sampleOffsetFromPlayer = {
        samplePoint.x - playerState->worldPos.x,
        samplePoint.y - playerState->worldPos.y,
        samplePoint.z - playerState->worldPos.z,
    };
    const zVec3 tiltVector = {
        sampleOffsetFromPlayer.y * playerState->steerBasisRef.z -
            sampleOffsetFromPlayer.z * playerState->steerBasisRef.y,
        sampleOffsetFromPlayer.z * playerState->steerBasisRef.x -
            sampleOffsetFromPlayer.x * playerState->steerBasisRef.z,
        sampleOffsetFromPlayer.x * playerState->steerBasisRef.y -
            sampleOffsetFromPlayer.y * playerState->steerBasisRef.x,
    };
    ApplyTerrainTilt(
        saveState,
        &tiltVector,
        1.0f
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computesurfacefrom2probes
 * @recoil-artifact defines .text recoil:function:0x42c640: Player::ComputeSurfaceFrom2Probes.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ComputeSurfaceFrom2Probes from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ComputeSurfaceFrom2Probes(
    zUtil_SaveGameState *saveState,
    PlayerEnvProbeResult *probeResult
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int sampleIndexB = g_PlayerEnvProbe_AboveGroundIndices[1];
    const int sampleIndexA = g_PlayerEnvProbe_AboveGroundIndices[0];

    zVec3 pointA = g_PlayerEnvProbeWorldPoints[sampleIndexA];
    pointA.y = probeResult->candidateScoreBySample[sampleIndexA];

    zVec3 pointB = g_PlayerEnvProbeWorldPoints[sampleIndexB];
    const float pointASupportDot = playerState->steerBasisRef.x * pointA.x +
                                   playerState->steerBasisRef.y * pointA.y +
                                   playerState->steerBasisRef.z * pointA.z;
    pointB.y = SolveHeightOnSurface(
        saveState,
        pointASupportDot
    );

    zVec3 supportEdge = {
        pointB.x - pointA.x,
        pointB.y - pointA.y,
        pointB.z - pointA.z,
    };
    const zVec3 perpOffset = {
        playerState->steerBasisRef.y * supportEdge.z - playerState->steerBasisRef.z * supportEdge.y,
        playerState->steerBasisRef.z * supportEdge.x - playerState->steerBasisRef.x * supportEdge.z,
        playerState->steerBasisRef.x * supportEdge.y - playerState->steerBasisRef.y * supportEdge.x,
    };
    const zVec3 pointC = {
        pointA.x + perpOffset.x,
        pointA.y + perpOffset.y,
        pointA.z + perpOffset.z,
    };

    pointB.y = probeResult->candidateScoreBySample[sampleIndexB];
    ComputeTriangleNormal(
        saveState,
        &pointA,
        &pointB,
        &pointC
    );

    const float surfaceDot = playerState->steerBasisRef.x * pointA.x +
                             playerState->steerBasisRef.y * pointA.y +
                             playerState->steerBasisRef.z * pointA.z;
    playerState->worldPos.y = SolveHeightOnSurface(
        saveState,
        surfaceDot
    );

    zMath::Vec3Normalize(&supportEdge);
    const zVec3 pointOffsetFromPlayer = {
        pointA.x - playerState->worldPos.x,
        pointA.y - playerState->worldPos.y,
        pointA.z - playerState->worldPos.z,
    };
    const zVec3 tiltPerp = {
        playerState->steerBasisRef.y * supportEdge.z - playerState->steerBasisRef.z * supportEdge.y,
        playerState->steerBasisRef.z * supportEdge.x - playerState->steerBasisRef.x * supportEdge.z,
        playerState->steerBasisRef.x * supportEdge.y - playerState->steerBasisRef.y * supportEdge.x,
    };
    const float tiltScale = pointOffsetFromPlayer.x * tiltPerp.x +
                            pointOffsetFromPlayer.y * tiltPerp.y +
                            pointOffsetFromPlayer.z * tiltPerp.z;
    ApplyTerrainTilt(
        saveState,
        &supportEdge,
        tiltScale
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applyterraintilt
 * @recoil-artifact defines .text recoil:function:0x42c8d0: Player::ApplyTerrainTilt.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ApplyTerrainTilt from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ApplyTerrainTilt(
    zUtil_SaveGameState *saveState,
    const zVec3 *tiltVector,
    float tiltScale
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const float tiltFactor = (g_Player_NominalGravity / playerState->gravityAccel) * tiltScale;
    zVec3 rotatedTilt = {
        tiltVector->x * tiltFactor,
        tiltVector->y * tiltFactor,
        tiltVector->z * tiltFactor,
    };
    zMath::Vec3RotateY(
        &rotatedTilt,
        &rotatedTilt,
        -playerState->restartYawRad
    );

    if (playerState->airborneFlag != 0) {
        ResetTerrainContactImpulsesAndPlayImpactSfx(saveState);
    }

    playerState->angVelPitch += rotatedTilt.x;
    playerState->angVelRoll += rotatedTilt.z;
    if (playerState->angVelPitch > 1.20000005f) {
        playerState->angVelPitch = 1.20000005f;
    } else if (playerState->angVelPitch < -1.20000005f) {
        playerState->angVelPitch = -1.20000005f;
    }
    if (playerState->angVelRoll > 1.20000005f) {
        playerState->angVelRoll = 1.20000005f;
    } else if (playerState->angVelRoll < -1.20000005f) {
        playerState->angVelRoll = -1.20000005f;
    }

    const float velocityScale = g_Player_DeltaTime * playerState->gravityAccel * 5.0f;
    zVec3 impulse = {
        playerState->steerBasisRef.x * velocityScale,
        0.0f,
        playerState->steerBasisRef.z * velocityScale,
    };
    playerState->projectileSpawnVel.x += impulse.x;
    playerState->projectileSpawnVel.y += impulse.y;
    playerState->projectileSpawnVel.z += impulse.z;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computesurfacefrom3probes
 * @recoil-artifact defines .text recoil:function:0x42ca40: Player::ComputeSurfaceFrom3Probes.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ComputeSurfaceFrom3Probes from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ComputeSurfaceFrom3Probes(
    zUtil_SaveGameState *saveState,
    PlayerEnvProbeResult *probeResult
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->airborneFlag != 0) {
        ResetTerrainContactImpulsesAndPlayImpactSfx(saveState);
    }

    const int sampleIndexA = g_PlayerEnvProbe_AboveGroundIndices[0];
    const int sampleIndexB = g_PlayerEnvProbe_AboveGroundIndices[1];
    const int sampleIndexC = g_PlayerEnvProbe_AboveGroundIndices[2];
    zVec3 pointA = g_PlayerEnvProbeWorldPoints[sampleIndexA];
    zVec3 pointB = g_PlayerEnvProbeWorldPoints[sampleIndexB];
    zVec3 pointC = g_PlayerEnvProbeWorldPoints[sampleIndexC];
    pointA.y = probeResult->candidateScoreBySample[sampleIndexA];
    pointB.y = probeResult->candidateScoreBySample[sampleIndexB];
    pointC.y = probeResult->candidateScoreBySample[sampleIndexC];

    ComputeTriangleNormal(
        saveState,
        &pointA,
        &pointB,
        &pointC
    );
    const float surfaceDot = playerState->steerBasisRef.x * pointA.x +
                             playerState->steerBasisRef.y * pointA.y +
                             playerState->steerBasisRef.z * pointA.z;
    playerState->worldPos.y = SolveHeightOnSurface(
        saveState,
        surfaceDot
    );
    playerState->angVelPitch = 0.0f;
    playerState->angVelRoll = 0.0f;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resetterraincontactimpulsesandplayimpactsfx
 * @recoil-artifact defines .text recoil:function:0x42cb50: Player::ResetTerrainContactImpulsesAndPlayImpactSfx.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ResetTerrainContactImpulsesAndPlayImpactSfx from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ResetTerrainContactImpulsesAndPlayImpactSfx(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->angVelRoll = 0.0f;
    playerState->angVelPitch = 0.0f;

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        return;
    }

    float sfxVolume = (float)(fabs(playerState->projectileSpawnVel.y * 0.100000001f));
    if (sfxVolume > 1.0f) {
        sfxVolume = 1.0f;
    } else if (sfxVolume < 0.0f) {
        sfxVolume = 0.0f;
    }
    saveState->StartModalLoopSfxHandle(
        5,
        sfxVolume
    );
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-checkprobesamplemaskoverlap
 * @recoil-artifact defines .text recoil:function:0x42cbd0: Player::CheckProbeSampleMaskOverlap.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::CheckProbeSampleMaskOverlap from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall CheckProbeSampleMaskOverlap(
    int sampleIndexA,
    int sampleIndexB,
    int sampleIndexC
) {
    return g_PlayerEnvProbeSampleMaskTable[sampleIndexC] &
           g_PlayerEnvProbeSampleMaskTable[sampleIndexB] &
           g_PlayerEnvProbeSampleMaskTable[sampleIndexA];
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-selectbestprobesbydotproduct
 * @recoil-artifact defines .text recoil:function:0x42cc00: Player::SelectBestProbesByDotProduct.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SelectBestProbesByDotProduct from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall SelectBestProbesByDotProduct(
    const zVec3 *referenceNormal,
    PlayerEnvProbeResult *probeResult
) {
    int sampleIndexA = -1;
    int sampleIndexB = -1;
    int sampleIndexC = -1;
    int sampleIndexD = -1;
    float scoreA = -100000000.0f;
    float scoreB = -100000000.0f;
    float scoreC = -100000000.0f;
    float scoreD = -100000000.0f;

    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        if (g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] == 0) {
            continue;
        }

        zVec3 candidatePoint = g_PlayerEnvProbeWorldPoints[sampleIndex];
        candidatePoint.y = probeResult->candidateScoreBySample[sampleIndex];
        const float score = referenceNormal->x * candidatePoint.x +
                            referenceNormal->y * candidatePoint.y +
                            referenceNormal->z * candidatePoint.z;

        if (score > scoreA) {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            scoreD = scoreC;
            sampleIndexD = sampleIndexC;
            sampleIndexC = sampleIndexB;
            scoreC = scoreB;
            scoreB = scoreA;
            sampleIndexB = sampleIndexA;
            scoreA = score;
            sampleIndexA = sampleIndex;
        } else if (score > scoreB) {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            sampleIndexD = sampleIndexC;
            sampleIndexC = sampleIndexB;
            scoreD = scoreC;
            scoreC = scoreB;
            scoreB = score;
            sampleIndexB = sampleIndex;
        } else if (score > scoreC) {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            sampleIndexD = sampleIndexC;
            scoreD = scoreC;
            scoreC = score;
            sampleIndexC = sampleIndex;
        } else if (score > scoreD) {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            scoreD = score;
            sampleIndexD = sampleIndex;
        } else {
            g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] = 0;
        }
    }

    if (CheckProbeSampleMaskOverlap(
        sampleIndexA,
        sampleIndexB,
        sampleIndexC
    ) == 0) {
        g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
    } else {
        g_PlayerEnvProbe_AboveGroundFlags[sampleIndexC] = 0;
    }
    RebuildAboveGroundIndices();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-solveheightonsurface
 * @recoil-artifact defines .text recoil:function:0x42cde0: Player::SolveHeightOnSurface.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SolveHeightOnSurface from the recovered
 * Battlesport gameplay source file.
 */
float __fastcall SolveHeightOnSurface(
    zUtil_SaveGameState *saveState,
    float supportPlaneDot
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    float steerBasisRefY = playerState->steerBasisRef.y;
    if (steerBasisRefY == 0.0f) {
        steerBasisRefY = 0.0000999999975f;
    }

    return (supportPlaneDot - playerState->worldPos.x * playerState->steerBasisRef.x -
               playerState->worldPos.z * playerState->steerBasisRef.z) /
           steerBasisRefY;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computetrianglenormal
 * @recoil-artifact defines .text recoil:function:0x42ce50: Player::ComputeTriangleNormal.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ComputeTriangleNormal from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ComputeTriangleNormal(
    zUtil_SaveGameState *saveState,
    const zVec3 *pointA,
    const zVec3 *pointB,
    const zVec3 *pointC
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const zVec3 edgeAB = {
        pointB->x - pointA->x,
        pointB->y - pointA->y,
        pointB->z - pointA->z,
    };
    const zVec3 edgeAC = {
        pointC->x - pointA->x,
        pointC->y - pointA->y,
        pointC->z - pointA->z,
    };
    zVec3 normal = {
        edgeAB.y * edgeAC.z - edgeAB.z * edgeAC.y,
        edgeAB.z * edgeAC.x - edgeAB.x * edgeAC.z,
        edgeAB.x * edgeAC.y - edgeAB.y * edgeAC.x,
    };
    zMath::Vec3Normalize(&normal);
    if (normal.y <= 0.0f) {
        normal.x = -normal.x;
        normal.y = -normal.y;
        normal.z = -normal.z;
    }
    playerState->steerBasisRef = normal;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildabovegroundindices
 * @recoil-artifact defines .text recoil:function:0x42cf60: Player::RebuildAboveGroundIndices.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::RebuildAboveGroundIndices from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RebuildAboveGroundIndices() {
    int *aboveGroundIndexCursor = g_PlayerEnvProbe_AboveGroundIndices;
    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        if (g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] != 0) {
            *aboveGroundIndexCursor = sampleIndex;
            ++aboveGroundIndexCursor;
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-buildenvironmentproberesult
 * @recoil-artifact defines .text recoil:function:0x42cf90: Player::BuildEnvironmentProbeResult.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::BuildEnvironmentProbeResult from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall BuildEnvironmentProbeResult(
    zUtil_SaveGameState *saveState,
    PlayerEnvProbeResult *outProbe
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData *const masterModalData = primaryModalState->masterModalData;

    const int modalPointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < modalPointCount; ++i) {
        const zVec3 transformed = TransformPointByMatrix(
            masterModalData->probePoints[kPlayerEnvProbeBasePointOffset + i],
            playerState->motionBasis
        );
        primaryModalState->transformedProbePointWorldByIndex[i] = transformed;
        g_PlayerEnvProbeWorldPoints[i] = transformed;
    }

    if (g_PlayerEnvProbeSampleCount > 4) {
        zMath::Vec3Midpoint(
            &g_PlayerEnvProbeWorldPoints[0],
            &g_PlayerEnvProbeWorldPoints[3],
            &g_PlayerEnvProbeWorldPoints[4]
        );
        zMath::Vec3Midpoint(
            &g_PlayerEnvProbeWorldPoints[1],
            &g_PlayerEnvProbeWorldPoints[2],
            &g_PlayerEnvProbeWorldPoints[5]
        );
    }

    if (g_PlayerEnvProbeSampleCount > 6) {
        g_PlayerEnvProbeWorldPoints[6] = playerState->worldPos;
    }

    zUtil_PlayerStateStorage *const globalPlayerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
    zClass_Class::gwNodeSetCellPickable(
        playerState->rootNode,
        0
    );
    zClass_Class::gwNodeSetCellPickable(
        globalPlayerState->rootNode,
        0
    );

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_cls_di::BuildPickCandidatesForPointBatch(
        g_Player_RuntimeDiScene,
        g_PlayerEnvProbeWorldPoints,
        g_PlayerEnvProbeSampleCount,
        500.0f,
        outProbe->candidateBuffers
    );
    g_Variant_CurrentTag = g_VariantTag_Current;

    outProbe->highestSelectedHitY = -300.0f;
    outProbe->attachmentCandidateCount = 0;

    float maxRiseWindow = -(playerState->projectileSpawnVel.y * g_Player_DeltaTime);
    if (outProbe->minProbeDepth > maxRiseWindow) {
        maxRiseWindow = outProbe->minProbeDepth;
    }

    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        int bestCandidateIndex = 0;
        int selectedImpactSlot = 0;
        float taggedHeight = -300.0f;
        PlayerProbeSampleCandidateBuffer *const candidateBuffer =
            &outProbe->candidateBuffers[sampleIndex];

        outProbe->candidateScoreBySample[sampleIndex] = SelectProbeSampleHeightFromCandidates(
            candidateBuffer,
            &bestCandidateIndex,
            g_PlayerEnvProbeWorldPoints[sampleIndex].y,
            maxRiseWindow,
            outProbe->preferAttachmentSlot1,
            &selectedImpactSlot,
            &taggedHeight
        );
        outProbe->bestIndexBySample[sampleIndex] = bestCandidateIndex;
        outProbe->impactSlotBySample[sampleIndex] = selectedImpactSlot;

        if (outProbe->highestSelectedHitY < taggedHeight) {
            outProbe->highestSelectedHitY = taggedHeight;
        }

        if (sampleIndex < 4 && sampleIndex == 0) {
            if (outProbe->candidateBuffers[0].candidateCount <= 0) {
                zClass_Class::gwNodeSetNodeType(
                    playerState->rootNode,
                    0xff
                );
            } else {
                const zClassDiPickCandidateEntry *const selectedCandidate =
                    &outProbe->candidateBuffers[0].entries[outProbe->bestIndexBySample[0]];
                playerState->selectedProbeSample = *selectedCandidate;
                playerState->selectedProbeSample.hitPos.x =
                    primaryModalState->transformedProbePointWorldByIndex[0].x;
                playerState->selectedProbeSample.hitPos.z =
                    primaryModalState->transformedProbePointWorldByIndex[0].z;
                playerState->variantTag = selectedCandidate->variantTag;

                zClass_NodePartial *const worldChild =
                    zClass_Class::gwNodeGetWorldChild(selectedCandidate->node);
                const int nodeType =
                    worldChild != 0 ? worldChild->nodeType : selectedCandidate->variantTag.tags[0];
                zClass_Class::gwNodeSetNodeType(
                    playerState->rootNode,
                    nodeType
                );
            }
        }

        outProbe->hitHistogram.countByImpactSlot[selectedImpactSlot] += 1;

        if (candidateBuffer->candidateCount != 0) {
            zClass_NodePartial *const candidateNode =
                candidateBuffer->entries[bestCandidateIndex].node;
            if (candidateNode != 0 && candidateNode->auxFlags != 0) {
                outProbe->attachmentCandidateCount += 1;
                outProbe->attachmentNode = (zClass_NodePartial *)(candidateNode->callbackContext);
            }
        }

        if (sampleIndex >= 4 && (g_PlayerEnvProbeSampleMaskTable[sampleIndex] & 0x0a) == 0) {
            outProbe->candidateScoreBySample[sampleIndex] -= 0.2f;
        }
    }

    playerState->probeImpactSlot1SeenFlag = outProbe->hitHistogram.countByImpactSlot[1];
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-findthirdprobeandcomputenormal
 * @recoil-artifact defines .text recoil:function:0x42d320: Player::FindThirdProbeAndComputeNormal.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::FindThirdProbeAndComputeNormal from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall FindThirdProbeAndComputeNormal(
    zUtil_SaveGameState *saveState,
    PlayerEnvProbeResult *probeResult
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    int thirdProbeCandidateScanCount = 4;
    if (g_PlayerEnvProbeSampleCount <= 4) {
        thirdProbeCandidateScanCount = g_PlayerEnvProbeSampleCount;
    }

    const int firstAboveGroundSampleIndex = g_PlayerEnvProbe_AboveGroundIndices[0];
    const int secondAboveGroundSampleIndex = g_PlayerEnvProbe_AboveGroundIndices[1];
    int bestThirdProbeSampleIndex = 0;
    float bestThirdProbeHeightDelta = 0.0f;
    for (int candidateProbeSampleIndex = 0;
        candidateProbeSampleIndex < thirdProbeCandidateScanCount;
        ++candidateProbeSampleIndex) {
        if (candidateProbeSampleIndex == firstAboveGroundSampleIndex ||
            candidateProbeSampleIndex == secondAboveGroundSampleIndex) {
            continue;
        }

        const zVec3 transformedCandidateProbePoint = TransformPointByMatrix(
            masterModalData
                ->probePoints[kPlayerEnvProbeBasePointOffset + candidateProbeSampleIndex],
            playerState->motionBasis
        );
        const float candidateHeightDelta =
            probeResult->candidateScoreBySample[candidateProbeSampleIndex] -
            transformedCandidateProbePoint.y;
        if (candidateHeightDelta > bestThirdProbeHeightDelta && CheckProbeSampleMaskOverlap(
                                                                    firstAboveGroundSampleIndex,
                                                                    secondAboveGroundSampleIndex,
                                                                    candidateProbeSampleIndex
                                                                ) == 0) {
            bestThirdProbeSampleIndex = candidateProbeSampleIndex;
            bestThirdProbeHeightDelta = candidateHeightDelta;
        }
    }

    if (bestThirdProbeHeightDelta <= g_Player_DeltaTime) {
        return;
    }

    zVec3 firstSynthSupportPoint = g_PlayerEnvProbeWorldPoints[firstAboveGroundSampleIndex];
    zVec3 secondSynthSupportPoint = g_PlayerEnvProbeWorldPoints[secondAboveGroundSampleIndex];
    zVec3 thirdSynthSupportPoint = g_PlayerEnvProbeWorldPoints[bestThirdProbeSampleIndex];
    firstSynthSupportPoint.y = probeResult->candidateScoreBySample[firstAboveGroundSampleIndex];
    secondSynthSupportPoint.y = probeResult->candidateScoreBySample[secondAboveGroundSampleIndex];
    thirdSynthSupportPoint.y = probeResult->candidateScoreBySample[bestThirdProbeSampleIndex];

    ComputeTriangleNormal(
        saveState,
        &firstSynthSupportPoint,
        &secondSynthSupportPoint,
        &thirdSynthSupportPoint
    );
    const float surfaceDot = playerState->steerBasisRef.x * firstSynthSupportPoint.x +
                             playerState->steerBasisRef.y * firstSynthSupportPoint.y +
                             playerState->steerBasisRef.z * firstSynthSupportPoint.z;
    playerState->worldPos.y = SolveHeightOnSurface(
        saveState,
        surfaceDot
    );
    RebuildOrientationFromNormal(saveState);
}
} // namespace Player

namespace zMath {
/**
 * Purpose: Writes the component-wise midpoint of two vectors and returns the output pointer.
 * Data: reads shared zMath scalar constant 0x4d08d4 and writes only the
 * caller-supplied output vector.
 */
zVec3 *__fastcall Vec3Midpoint(
    const zVec3 *a,
    const zVec3 *b,
    zVec3 *outMidpoint
) {
    const float sumX = a->x + b->x;
    const float sumY = a->y + b->y;
    const float sumZ = a->z + b->z;
    outMidpoint->x = sumX;
    outMidpoint->y = sumY;
    outMidpoint->z = sumZ;
    outMidpoint->x *= g_zMath_MidpointHalf;
    outMidpoint->y *= g_zMath_MidpointHalf;
    outMidpoint->z *= g_zMath_MidpointHalf;
    return outMidpoint;
}
} // namespace zMath

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applyenvironmentproberesult
 * @recoil-artifact defines .text recoil:function:0x42d5c0: Player::ApplyEnvironmentProbeResult.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ApplyEnvironmentProbeResult from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall ApplyEnvironmentProbeResult(
    zUtil_SaveGameState *saveState,
    PlayerEnvProbeResult *envProbe
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    const int wasAttached = playerState->environmentAttachmentActive;

    if (envProbe->attachmentCandidateCount > 3) {
        if (wasAttached == 0) {
            playerState->environmentAttachmentActive = 1;
            playerState->environmentAttachmentNode = envProbe->attachmentNode;
            CopyNodeCachedWorldMatrix(
                &playerState->environmentAttachmentMatrix,
                envProbe->attachmentNode
            );
            playerState->yawPoseCache =
                playerState->restartYawRad -
                ExtractYawFromMatrix(&playerState->environmentAttachmentMatrix);
            CacheAttachmentLocalOffset(playerState);
        }
    } else if (wasAttached != 0) {
        CopyNodeCachedWorldMatrix(
            &playerState->environmentAttachmentMatrix,
            playerState->environmentAttachmentNode
        );
        playerState->restartYawRad =
            ExtractYawFromMatrix(&playerState->environmentAttachmentMatrix) +
            playerState->yawPoseCache;
        playerState->pitchPoseCache = playerState->vehiclePitchRad;
        playerState->yawPoseCache = playerState->restartYawRad;
        playerState->rollPoseCache = playerState->vehicleRollRad;
        playerState->environmentAttachmentActive = 0;
        playerState->environmentAttachmentNode = 0;
    }

    if (playerState->amphibUnlocked != 0 && envProbe->hitHistogram.countByImpactSlot[1] > 1 &&
        TransitionToMasterTypeAmphib(
            saveState,
            0,
            0
        ) != 0) {
        playerState->currentMasterType = masterModalData->masterType;
        if (playerState->projectileSpawnVel.y < -10.0f) {
            zEffectAnim::SetTransformRotAndVelocity_Thunk(
                g_Player_BftSplashAnimEntry,
                0,
                playerState->worldPos.x,
                envProbe->highestSelectedHitY,
                playerState->worldPos.z,
                0.0f,
                playerState->restartYawRad,
                0.0f,
                0.0f,
                0.0f,
                0.0f
            );
            playerState->projectileSpawnVel.z = 0.0f;
            playerState->projectileSpawnVel.x = 0.0f;
            return 0;
        }
    }

    playerState->gravityAccel = g_Player_NominalGravity;
    zUtil_SaveGameState *const originalSaveState = saveState;
    const int waterHitCount = envProbe->hitHistogram.countByImpactSlot[1];
    if (envProbe->highestSelectedHitY - playerState->worldPos.y > 1.0f && waterHitCount > 1) {
        const int wasUnderwater = playerState->underwaterStatusActive;
        playerState->gravityAccel = g_Player_WaterGravity;
        if (wasUnderwater == 0) {
            playerState->underwaterStatusActive = 1;
            if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
                HudUi::ShowTopMessageLine(
                    zLoc::GetMessageString(0x909),
                    5.0f
                );
                HudLowMeterLoopSound::SetLoopActive(1);
            }
        }

        const float damage = g_Player_DeltaTime * 8.0f;
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            EnterDestroyedState(
                saveState,
                0,
                0,
                damage
            );
            if (playerState->cameraTarget.y < envProbe->highestSelectedHitY) {
                SetHudUiElementVisible(
                    &g_Player_UnderwaterFxPass3Ui,
                    1
                );
            } else {
                SetHudUiElementVisible(
                    &g_Player_UnderwaterFxPass3Ui,
                    0
                );
            }
        } else {
            HitCallback_RecordContextAndTimedStatus(
                saveState,
                0,
                0,
                damage
            );
        }
    } else {
        if (playerState->underwaterStatusActive != 0) {
            playerState->underwaterStatusActive = 0;
            if (originalSaveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
                SetHudUiElementVisible(
                    &g_Player_UnderwaterFxPass3Ui,
                    0
                );
                HudLowMeterLoopSound::SetLoopActive(0);
            }
        }
    }

    if (envProbe->hitHistogram.countByImpactSlot[3] > 1) {
        playerState->axisClampRuntime =
            masterModalData->maxSpeed * masterModalData->quicksandSlowdown;
        playerState->yawVelocityLimit =
            masterModalData->yawRateMax * masterModalData->quicksandSlowdown;
        playerState->gravityAccel = g_Player_QuicksandGravity;
        return 1;
    }

    if (envProbe->hitHistogram.countByImpactSlot[4] > 1) {
        if (playerState->hoverUnlocked != 0 && TransitionToMasterTypeHover(
            saveState,
            0
        ) != 0) {
            return 0;
        }

        if (playerState->motionInput == 0) {
            playerState->motionInput = 1;
        }
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x910),
                5.0f
            );
            HudLowMeterLoopSound::SetLoopActive(1);
        }

        playerState->axisClampRuntime = masterModalData->maxSpeed * masterModalData->lavaSlowdown;
        playerState->yawVelocityLimit = masterModalData->yawRateMax * masterModalData->lavaSlowdown;
        const float damage =
            (float)(envProbe->hitHistogram.countByImpactSlot[4]) * g_Player_DeltaTime * 12.0f;
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            EnterDestroyedState(
                saveState,
                g_Player_MakeHotOptEntry,
                0,
                damage
            );
        } else {
            HitCallback_RecordContextAndTimedStatus(
                saveState,
                g_Player_MakeHotOptEntry,
                0,
                damage
            );
        }
        return 1;
    }

    if (playerState->motionInput != 0) {
        playerState->motionInput = 0;
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            HudLowMeterLoopSound::SetLoopActive(0);
        }
    }

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    playerState->yawVelocityLimit = masterModalData->yawRateMax;
    return 1;
}
} // namespace Player

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildorientationfromnormal
 * @recoil-artifact defines .text recoil:function:0x42da40: Player::RebuildOrientationFromNormal.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::RebuildOrientationFromNormal from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RebuildOrientationFromNormal(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->steerBasisRef.y == 0.0f) {
        playerState->steerBasisRef.y = 0.00100000005f;
    }

    zVec3 rawBasis = playerState->steerBasisNorm;
    rawBasis.y =
        -((rawBasis.x * playerState->steerBasisRef.x + playerState->steerBasisRef.z * rawBasis.z) /
            playerState->steerBasisRef.y);
    zMath::Vec3Normalize(&rawBasis);
    playerState->steerBasisRaw = rawBasis;

    zVec3 yawRelativeNormal = {0};
    zMath::Vec3RotateY(
        &yawRelativeNormal,
        &playerState->steerBasisRef,
        -playerState->restartYawRad
    );
    playerState->vehiclePitchRad = (float)(asin(yawRelativeNormal.z));
    playerState->vehicleRollRad = (float)(asin(-yawRelativeNormal.x));
    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
}
} // namespace Player

/**
 *
 * Purpose: resolve an interface-map entry for a requested IID and AddRef the
 * adjusted interface pointer returned to the caller.
 */
HRESULT WINAPI zCom::QueryInterfaceFromInterfaceMap(
    void *objectBase,
    const InterfaceMapEntry *interfaceMap,
    const GUID *requestedIid,
    void **outInterface
) {
    if (outInterface == 0) {
        return E_POINTER;
    }

    *outInterface = 0;

    const unsigned int *const requestedWords = (const unsigned int *)(requestedIid);
    unsigned int resolverRaw;
    const InterfaceMapEntry *currentEntry;
    if (requestedWords[0] == 0 && requestedWords[1] == 0 && requestedWords[2] == 0x000000c0 &&
        requestedWords[3] == 0x46000000) {
        IUnknown *const resolvedInterface =
            (IUnknown *)((DWORD)objectBase + interfaceMap->interfaceOffset);
        resolvedInterface->AddRef();
        *outInterface = resolvedInterface;
        return S_OK;
    }

    currentEntry = interfaceMap;
    while ((resolverRaw = currentEntry->resolverRaw) != ZCOM_INTERFACE_MAP_END) {
        const GUID *entryIid = currentEntry->iid;
        int blindEntry = entryIid == 0;
        const unsigned int *const entryWords = (const unsigned int *)(entryIid);
        if (blindEntry != 0 ||
            (entryWords[0] == requestedWords[0] && entryWords[1] == requestedWords[1] &&
                entryWords[2] == requestedWords[2] && entryWords[3] == requestedWords[3])) {
            if (resolverRaw == ZCOM_INTERFACE_MAP_DIRECT) {
                IUnknown *const resolvedInterface =
                    (IUnknown *)((DWORD)objectBase + currentEntry->interfaceOffset);
                resolvedInterface->AddRef();
                *outInterface = resolvedInterface;
                return S_OK;
            }

            QueryInterfaceResolver resolver = (QueryInterfaceResolver)(resolverRaw);
            const HRESULT result =
                resolver(
                    objectBase,
                    requestedIid,
                    outInterface,
                    currentEntry->interfaceOffset
                );
            if (result == S_OK || (!blindEntry && result < 0)) {
                return result;
            }
        }

        ++currentEntry;
    }

    return E_NOINTERFACE;
}

/**
 *
 * Purpose: query a source for IConnectionPointContainer, find the requested
 * connection point, and advise the sink while releasing temporary interfaces.
 */
HRESULT WINAPI zCom::ConnectionPointContainer_Advise(
    IUnknown *source,
    IUnknown *sink,
    REFIID connectionPointIid,
    DWORD *cookie
) {
    ComReleaseOnExit<IConnectionPointContainer> cpc = {0};
    ComReleaseOnExit<IConnectionPoint> cp = {0};

    HRESULT result = source->QueryInterface(
        IID_IConnectionPointContainer,
        (void **)(&cpc.ptr)
    );
    if (result >= 0) {
        result = cpc.ptr->FindConnectionPoint(
            connectionPointIid,
            &cp.ptr
        );
        if (result >= 0) {
            result = cp.ptr->Advise(
                sink,
                cookie
            );
        }
    }

    return result;
}

/**
 *
 * Purpose: query a source for IConnectionPointContainer, find the requested
 * connection point, and unadvise the cookie while releasing temporary interfaces.
 */
HRESULT WINAPI zCom::ConnectionPointContainer_Unadvise(
    IUnknown *source,
    REFIID connectionPointIid,
    DWORD cookie
) {
    ComReleaseOnExit<IConnectionPointContainer> cpc = {0};
    ComReleaseOnExit<IConnectionPoint> cp = {0};

    HRESULT result = source->QueryInterface(
        IID_IConnectionPointContainer,
        (void **)(&cpc.ptr)
    );
    if (result >= 0) {
        result = cpc.ptr->FindConnectionPoint(
            connectionPointIid,
            &cp.ptr
        );
        if (result >= 0) {
            result = cp.ptr->Unadvise(cookie);
        }
    }

    return result;
}

/**
 *
 * Purpose: validate and initialize the transient WOL bootstrap-state block,
 * module handles, event-sink live count, and critical sections.
 */
HRESULT __stdcall WestwoodOnlineUpgradeApiInitState::Init(
    WestwoodOnlineUpgradeApiInitState *self,
    HANDLE bootstrapServerListEvent,
    HINSTANCE moduleHandle
) {
    if (self == 0) {
        return E_INVALIDARG;
    }

    if (self->structSize < sizeof(WestwoodOnlineUpgradeApiInitState)) {
        return E_INVALIDARG;
    }

    self->eventSinkLiveCount = 0;
    self->failureEvent = 0;
    self->bootstrapServerListEvent = bootstrapServerListEvent;
    self->moduleHandleSecondary = moduleHandle;
    self->moduleHandleTertiary = moduleHandle;
    self->moduleHandlePrimary = moduleHandle;
    InitializeCriticalSection(&self->criticalSection0);
    InitializeCriticalSection(&self->criticalSection1);
    InitializeCriticalSection(&self->criticalSection2);
    return S_OK;
}
