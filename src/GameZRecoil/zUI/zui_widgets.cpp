#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_game_setup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"

#include "Battlesport/turret.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zUtil/zbd.h"

#include <cctype>
#include <cstdarg>
#include <math.h>
#include <new>
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <vector>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

namespace {
const int ZOPT_GRAPHICS_PERSPECTIVE = 8;
const int ZOPT_GRAPHICS_GLOBAL_LIGHT = 0x10;
const int ZVID_HW_MODE_SOFTWARE = 0;
const float ZSND_CD_VOLUME_TO_NORMALIZED = 1.52590219e-05f;
const float ZSND_CD_NORMALIZED_TO_VOLUME = 65535.0f;

struct HudReticleAttachStatePartial {
    unsigned char unknown_00[0x0c];
    CZNodePartial* projectileNode;
};

struct HudReticleAltGunControllerPartial {
    OptCatalogEntryDef* optCatalogEntry;
    unsigned char unknown_04[0x24];
    HudReticleAttachStatePartial* attachState;
};

struct HudReticlePlayerStatePartial {
    unsigned char unknown_000[0x58c];
    int cameraState;
    unsigned char unknown_590[0x54];
    HudReticleAltGunControllerPartial* activeAltGunController;
    unsigned char unknown_5e8[0x8e8];
    CZNodePartial* rootNode;
};

RECOIL_STATIC_ASSERT(offsetof(HudReticleAttachStatePartial, projectileNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudReticleAltGunControllerPartial, attachState) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, cameraState) == 0x58c);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, activeAltGunController) == 0x5e4);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, rootNode) == 0xed0);

} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-ztimedtask-activecount
 * @recoil-artifact defines .data recoil:data:0x56bd30: g_zTimedTask_ActiveCount.
 * Purpose: preserve the recovered HUD global storage for g_zTimedTask_ActiveCount.
 */
int g_zTimedTask_ActiveCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-ztimedtask-activehead
 * @recoil-artifact defines .data recoil:data:0x56bd34: g_zTimedTask_ActiveHead.
 * Purpose: preserve the recovered HUD global storage for g_zTimedTask_ActiveHead.
 */
zTimedTask* g_zTimedTask_ActiveHead = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-ztimedtask-activetail
 * @recoil-artifact defines .data recoil:data:0x56bd38: g_zTimedTask_ActiveTail.
 * Purpose: preserve the recovered HUD global storage for g_zTimedTask_ActiveTail.
 */
zTimedTask* g_zTimedTask_ActiveTail = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcmdmousedebounceframes
 * @recoil-artifact defines .data recoil:data:0x4e5e00: g_HudCmdMouseDebounceFrames.
 * Purpose: preserve the recovered HUD global storage for g_HudCmdMouseDebounceFrames.
 */
int g_HudCmdMouseDebounceFrames = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimgrsensor-roundrobintrackindex
 * @recoil-artifact defines .data recoil:data:0x4dd1d8: g_HudUiMgrSensor_RoundRobinTrackIndex.
 * Owner data: four-byte signed index initialized to -1 (ff ff ff ff).
 * Purpose: seed the sensor-target round-robin candidate selector before
 * HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag advances it.
 */
int g_HudUiMgrSensor_RoundRobinTrackIndex = -1;
HudUiRect g_HudUiMgrSensor_FxRectScratch = { 0 };

#undef g_HudUiMgr
#undef g_HudLayoutHW
#undef g_HudLayoutSW
#undef g_HudCmdDialogState

union HudUiSensorWindowStorage {
    unsigned long align;
    unsigned char bytes[sizeof(CWnd)];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSensorWindowStorage) == sizeof(CWnd));

/*
 * Retail HUD UI storage and CRT initialization are not the same sequence.
 * Keep these definitions in the recovered storage run; VC5's compiler-emitted
 * static-lifetime helpers for HudLayoutSW and HudLayoutHW, together with the
 * HudUiMgr initialization and provider CString/CWnd constructors, form the
 * retail CRT pass:
 * CString, HudLayoutSW, HudLayoutHW, HudUiMgr, then CWnd.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduisensorwindow
 * @recoil-artifact defines .data recoil:data:0x4e5e90: g_HudUiSensorWindow.
 * Source model: zero-initialized provider CWnd storage; the explicit HUD CRT
 * row constructs it and registers the provider destructor.
 * Purpose: preserve the recovered HUD global storage for g_HudUiSensorWindow.
 */
HudUiSensorWindowStorage g_HudUiSensorWindow = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimgr
 * @recoil-artifact defines .data recoil:data:0x4e5ed0: g_HudUiMgr.
 * Source model: zero-initialized HudUiMgrData storage; HudUiMgr::StaticInit
 * constructs the typed manager through the explicit CRT row.
 * Purpose: preserve the recovered HUD global storage for g_HudUiMgr.
 */
HudUiMgrDataStorage g_HudUiMgr = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudlayoutsw
 * @recoil-artifact defines .data recoil:data:0x4eda68: g_HudLayoutSW.
 * Owner data: typed 236-byte singleton with compiler-owned static lifetime.
 * Retail startup constructs the software layout before the hardware layout.
 * Purpose: own the global software HUD layout instance.
 */
HudLayoutSW g_HudLayoutSW;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudlayouthw
 * @recoil-artifact defines .data recoil:data:0x4ed718: g_HudLayoutHW.
 * Owner data: typed 844-byte singleton with compiler-owned static lifetime.
 * Purpose: own the global hardware HUD layout instance.
 */
HudLayoutHW g_HudLayoutHW;

#define g_HudUiSensorWindow (*(CWnd*)&g_HudUiSensorWindow)
#define g_HudUiMgr (*(HudUiMgrData*)&g_HudUiMgr)

HudUiRect g_HudUiMgrSensorFxRect = { 0 };
int g_HudUiMgrSensorFxViewportWidth = 0;
int g_HudUiMgrSensorFxViewportHeight = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduisensorwindowplayback
 * @recoil-artifact defines .data recoil:data:0x4edb70: g_HudUiSensorWindowPlayback.
 * Purpose: preserve the recovered HUD global storage for g_HudUiSensorWindowPlayback.
 */
CZFMVPlayback* g_HudUiSensorWindowPlayback = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduichatmessagestack
 * @recoil-artifact defines .data recoil:data:0x56bd20: g_HudUiChatMessageStack.
 * Purpose: preserve the recovered HUD global storage for g_HudUiChatMessageStack.
 */
HudUiTextStack4* g_HudUiChatMessageStack = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduitopmessagestack
 * @recoil-artifact defines .data recoil:data:0x56bd24: g_HudUiTopMessageStack.
 * Purpose: preserve the recovered HUD global storage for g_HudUiTopMessageStack.
 */
HudUiTextStack4* g_HudUiTopMessageStack = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudui-auxoverlayenabled
 * @recoil-artifact defines .data recoil:data:0x4f3aa8: g_HudUi_AuxOverlayEnabled.
 * Purpose: preserve the recovered HUD global storage for g_HudUi_AuxOverlayEnabled.
 */
int g_HudUi_AuxOverlayEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcmddialogstate
 * @recoil-artifact defines .data recoil:data:0x4e5df0: g_HudCmdDialogState.
 * BN identifies 0x4e5df0 as an eight-byte BSS HudCmdDialogState object. VC5 emits the
 * 0x40bc20/0x40bc30/0x40bc40/0x40bc50 static init and at-exit thunks from this global object.
 * Purpose: preserve the recovered HUD global storage for g_HudCmdDialogState.
 */
HudCmdDialogStateStorage g_HudCmdDialogState = { 0 };

#define g_HudCmdDialogState (*(HudCmdDialogState*)&g_HudCmdDialogState)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-resolutioncyclenodename
 * @recoil-artifact defines .data recoil:data:0x4dac00: g_HudUiOptionsPanel_ResolutionCycleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD resolution selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_ResolutionCycleNodeName[] = "RESOLUTION_CYCLE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-musicvolumewidgetnodename
 * @recoil-artifact defines .data recoil:data:0x4dac14: g_HudUiOptionsPanel_MusicVolumeWidgetNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD music-volume widget node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_MusicVolumeWidgetNodeName[] = "MUSIC_VOLUME";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-musicenabletogglenodename
 * @recoil-artifact defines .data recoil:data:0x4dac24: g_HudUiOptionsPanel_MusicEnableToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD music-enable toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_MusicEnableToggleNodeName[] = "MUSIC_ENABLE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-soundvolumewidgetnodename
 * @recoil-artifact defines .data recoil:data:0x4dac34: g_HudUiOptionsPanel_SoundVolumeWidgetNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD sound-volume widget node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SoundVolumeWidgetNodeName[] = "SOUND_VOLUME";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-soundqualityselectornodename
 * @recoil-artifact defines .data recoil:data:0x4dac44: g_HudUiOptionsPanel_SoundQualitySelectorNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD sound-quality selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SoundQualitySelectorNodeName[] = "SOUND_QUALITY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-soundactivetogglenodename
 * @recoil-artifact defines .data recoil:data:0x4dac54: g_HudUiOptionsPanel_SoundActiveToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD sound-active toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SoundActiveToggleNodeName[] = "SOUND_ACTIVE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-effectszrdnodename
 * @recoil-artifact defines .data recoil:data:0x4dac64: g_EffectsZrdNodeName
 * (BN: g_HudUiOptionsPanel_EffectsNodeName).
 * Shared data owner: effects_weapons.shared_effects_zrd_node_name; this is
 * not HudOptionsDialog-owned data.
 * Purpose: name the shared EFFECTS ZRD node consumed by HudOptionsDialog and
 */
char g_EffectsZrdNodeName[8] = "EFFECTS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-texturememoryselectornodename
 * @recoil-artifact defines .data recoil:data:0x4dac6c: g_HudUiOptionsPanel_TextureMemorySelectorNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD texture-memory selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_TextureMemorySelectorNodeName[] = "TEXTURE_MEMORY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-objectdetailselectornodename
 * @recoil-artifact defines .data recoil:data:0x4dac7c: g_HudUiOptionsPanel_ObjectDetailSelectorNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD object-detail selector node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_ObjectDetailSelectorNodeName[] = "OBJECT_DETAIL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-fullhudtogglenodename
 * @recoil-artifact defines .data recoil:data:0x4dac8c: g_HudUiOptionsPanel_FullHudToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD full-HUD toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_FullHudToggleNodeName[] = "FULLHUD";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-perspectivetogglenodename
 * @recoil-artifact defines .data recoil:data:0x4dac94: g_HudUiOptionsPanel_PerspectiveToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD perspective toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_PerspectiveToggleNodeName[] = "PERSPECTIVE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-lightingtogglenodename
 * @recoil-artifact defines .data recoil:data:0x4daca0: g_HudUiOptionsPanel_LightingToggleNodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD lighting toggle node bound by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_LightingToggleNodeName[] = "LIGHTING";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduioptionspanel-sectionname
 * @recoil-artifact defines .data recoil:data:0x4dacac: g_HudUiOptionsPanel_SectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD options-panel section loaded by HudOptionsDialog.
 */
char g_HudUiOptionsPanel_SectionName[] = "OPTIONSPANEL";
extern char g_HudFontName_Arial[];
/**
 * Storage group:
 * hud_ui.hud_ui_mgr_ensure_hud_loaded_literals.
 * Source model: writable HUD ZRD key/source-path string globals shared by
 * HudUiMgr::EnsureHudLoaded and matching reader paths in zTurret/zImage.
 * Purpose: name the HUD layout sections and diagnostics consumed while the
 * HUD singleton loads its ZRD tree.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-modes
 * @recoil-artifact defines .data recoil:data:0x4dad2c: g_HudCfgKey_Modes.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Modes.
 */
char g_HudCfgKey_Modes[6] = "MODES";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-weapon
 * @recoil-artifact defines .data recoil:data:0x4dad34: g_HudCfgKey_Weapon.
 * Purpose: preserve the recovered shared WEAPON reader-key storage.
 */
char g_HudCfgKey_Weapon[7] = "WEAPON";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-target
 * @recoil-artifact defines .data recoil:data:0x4dad3c: g_HudCfgKey_Target.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Target.
 */
char g_HudCfgKey_Target[7] = "TARGET";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-shield
 * @recoil-artifact defines .data recoil:data:0x4dad44: g_HudCfgKey_Shield.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Shield.
 */
char g_HudCfgKey_Shield[7] = "SHIELD";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduiblankspaces8
 * @recoil-artifact defines .data recoil:data:0x4dad4c: g_HudUiBlankSpaces8.
 * Purpose: preserve the recovered HUD global storage for g_HudUiBlankSpaces8.
 */
char g_HudUiBlankSpaces8[9] = "        ";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-stats
 * @recoil-artifact defines .data recoil:data:0x4dad58: g_HudCfgKey_Stats.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Stats.
 */
char g_HudCfgKey_Stats[6] = "STATS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-reticule
 * @recoil-artifact defines .data recoil:data:0x4dad60: g_HudCfgKey_Reticule.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Reticule.
 */
char g_HudCfgKey_Reticule[9] = "RETICULE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-objective
 * @recoil-artifact defines .data recoil:data:0x4dad6c: g_HudCfgKey_Objective.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Objective.
 */
char g_HudCfgKey_Objective[10] = "OBJECTIVE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-sensor
 * @recoil-artifact defines .data recoil:data:0x4dad78: g_HudCfgKey_Sensor.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Sensor.
 */
char g_HudCfgKey_Sensor[7] = "SENSOR";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-nanite
 * @recoil-artifact defines .data recoil:data:0x4dad80: g_HudCfgKey_Nanite.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Nanite.
 */
char g_HudCfgKey_Nanite[7] = "NANITE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-ammo
 * @recoil-artifact defines .data recoil:data:0x4dad88: g_HudCfgKey_Ammo.
 * Purpose: preserve the recovered shared AMMO reader-key storage.
 */
char g_HudCfgKey_Ammo[5] = "AMMO";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-strings
 * @recoil-artifact defines .data recoil:data:0x4dad90: g_HudCfgKey_Strings.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_Strings.
 */
char g_HudCfgKey_Strings[8] = "STRINGS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-objectivedescription
 * @recoil-artifact defines .data recoil:data:0x4dad98: g_HudCfgKey_ObjectiveDescription.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_ObjectiveDescription.
 */
char g_HudCfgKey_ObjectiveDescription[16] = "OBJ_DESCRIPTION";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-objectivesummary
 * @recoil-artifact defines .data recoil:data:0x4dada8: g_HudCfgKey_ObjectiveSummary.
 * Purpose: preserve the recovered HUD global storage for g_HudCfgKey_ObjectiveSummary.
 */
char g_HudCfgKey_ObjectiveSummary[12] = "OBJ_SUMMARY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudcfgkey-fonts
 * @recoil-artifact defines .data recoil:data:0x4dadb4: g_HudCfgKey_Fonts.
 * Purpose: preserve the recovered shared FONTS reader-key storage.
 */
char g_HudCfgKey_Fonts[6] = "FONTS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hud-imagesearchpath-hud
 * @recoil-artifact defines .data recoil:data:0x4dadbc: g_Hud_ImageSearchPath_Hud.
 * Purpose: preserve the recovered HUD global storage for g_Hud_ImageSearchPath_Hud.
 */
char g_Hud_ImageSearchPath_Hud[26] = "..\\data\\common\\images\\hud";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hud-sourcefile-hudcpp
 * @recoil-artifact defines .data recoil:data:0x4dadd8: g_Hud_SourceFile_HudCpp.
 * Purpose: preserve the recovered HUD global storage for g_Hud_SourceFile_HudCpp.
 */
char g_Hud_SourceFile_HudCpp[28] = "D:\\Proj\\Battlesport\\hud.cpp";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudsensortracker-readfilefailedfmt
 * @recoil-artifact defines .data recoil:data:0x4dadf4: g_HudSensorTracker_ReadFileFailedFmt.
 * Shared data owner: hud_ui.shared_zrd_read_failed_format_literal.
 * Purpose: provide the shared failed-read diagnostic format used by HUD,
 * mission, pickup, turret, image, and opt-catalog ZRD load paths.
 */
char g_HudSensorTracker_ReadFileFailedFmt[18] = "Failed to read %s";
/**
 * Storage group:
 * hud_ui.hud_ui_zrd_widget_base_zrd_key_literals.
 * Source model: writable HUD ZRD key string globals consumed by the base
 * HudUiZrdWidget loaders. BN shows the six char[] objects in this order with
 * the address-aligned padding between ACTIVATE/DISABLE, RATE/FLASH,
 * FLASH/LABEL, LABEL/ROLLOVER, and ROLLOVER/BITMAP.
 * Purpose: name the optional activation, disable, rollover, label, and flash
 * records in a recovered HudUiZrdWidget section.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x9-0x4e46d0
 * @recoil-artifact defines .data recoil:data:0x4e46d0: g_HudZrd_Key_Activate.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Activate.
 */
char g_HudZrd_Key_Activate[0x9] = "ACTIVATE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x8-0x4e46dc
 * @recoil-artifact defines .data recoil:data:0x4e46dc: g_HudZrd_Key_Disable.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Disable.
 */
char g_HudZrd_Key_Disable[0x8] = "DISABLE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x5
 * @recoil-artifact defines .data recoil:data:0x4e46e4: g_HudZrd_Key_Rate.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Rate.
 */
char g_HudZrd_Key_Rate[0x5] = "RATE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x6-0x4e46ec
 * @recoil-artifact defines .data recoil:data:0x4e46ec: g_HudZrd_Key_Flash.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Flash.
 */
char g_HudZrd_Key_Flash[0x6] = "FLASH";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x6-0x4e46f4
 * @recoil-artifact defines .data recoil:data:0x4e46f4: g_HudZrd_Key_Label.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Label.
 */
char g_HudZrd_Key_Label[0x6] = "LABEL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x9-0x4e46fc
 * @recoil-artifact defines .data recoil:data:0x4e46fc: g_HudZrd_Key_Rollover.
 * Purpose: preserve the recovered HUD global storage for g_HudZrd_Key_Rollover.
 */
char g_HudZrd_Key_Rollover[0x9] = "ROLLOVER";
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Activate) == 0x9);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Disable) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Rate) == 0x5);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Flash) == 0x6);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Label) == 0x6);
RECOIL_STATIC_ASSERT(sizeof(g_HudZrd_Key_Rollover) == 0x9);
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-bitmap
 * @recoil-artifact defines .data recoil:data:0x4e4708: g_HudUiCycleSelectorWidget_ZrdKey_Bitmap.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name the shared BITMAP ZRD record consumed by HUD widget loaders.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Bitmap[] = "BITMAP";
/**
 * Storage group:
 * hud_ui.hud_ui_check_toggle_zrd_key_literals.
 * Source model: writable HUD ZRD key string globals consumed by
 * HudUiCheckToggleWidget::LoadFromZrd. BN shows DISABLE_SEL and
 * DISABLE_UNSEL in address order before the shared TEXT key, then CHECKED
 * immediately after TEXT. Each key includes its retail NUL terminator;
 * any further alignment bytes are not part of the string payload.
 * Purpose: name the check-toggle checked/disabled ZRD variant records.
 */
char g_HudUiZrdKey_DisableSel[0xc] = "DISABLE_SEL";
char g_HudUiZrdKey_DisableUnsel[0xe] = "DISABLE_UNSEL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-text
 * @recoil-artifact defines .data recoil:data:0x4e4738: g_HudUiCycleSelectorWidget_ZrdKey_Text.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Retail stores TEXT followed by its NUL at 0x4e473c. The three further
 * zero bytes before CHECKED are outside the C-string payload.
 * Purpose: name shared TEXT ZRD records consumed by toggle and cycle widgets.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Text[5] = "TEXT";
char g_HudUiZrdKey_Checked[0x8] = "CHECKED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-cycle
 * @recoil-artifact defines .data recoil:data:0x4e4748: g_HudUiCycleSelectorWidget_ZrdKey_Cycle.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name the CYCLE ZRD array loaded by HudUiCycleSelectorWidget.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Cycle[] = "CYCLE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-textoffset
 * @recoil-artifact defines .data recoil:data:0x4e4750: g_HudUiCycleSelectorWidget_ZrdKey_TextOffset.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name the TEXTOFFSET ZRD array loaded by HudUiCycleSelectorWidget.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_TextOffset[] = "TEXTOFFSET";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduicycleselectorwidget-zrdkey-font
 * @recoil-artifact defines .data recoil:data:0x4e475c: g_HudUiCycleSelectorWidget_ZrdKey_Font.
 * Shared data owner: hud_ui.cycle_selector_shared_zrd_key_literals.
 * Purpose: name shared FONT ZRD records consumed by HUD widget loaders.
 */
char g_HudUiCycleSelectorWidget_ZrdKey_Font[] = "FONT";
/**
 * Storage group:
 * hud_ui.zhud_background_config_zrd_key_literals.
 * Source model: writable HudUiBackground ZRD key string globals consumed by
 * HudUiBackground::LoadZrdAndSection. BN shows the keys in this order, with
 * VC5 char-array alignment padding between adjacent slots.
 * Purpose: name the background resource, cursor, capture, and sound records
 * in a recovered background ZRD section.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x12-0x4e47c0
 * @recoil-artifact defines .data recoil:data:0x4e47c0: zHudCfgKey_BACKGROUND_SOUNDS.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_SOUNDS.
 */
char zHudCfgKey_BACKGROUND_SOUNDS[0x12] = "BACKGROUND_SOUNDS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x8-0x4e47d4
 * @recoil-artifact defines .data recoil:data:0x4e47d4: zHudCfgKey_CAPTURE.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_CAPTURE.
 */
char zHudCfgKey_CAPTURE[0x8] = "CAPTURE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x7
 * @recoil-artifact defines .data recoil:data:0x4e47dc: zHudCfgKey_CURSOR.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_CURSOR.
 */
char zHudCfgKey_CURSOR[0x7] = "CURSOR";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x10
 * @recoil-artifact defines .data recoil:data:0x4e47e4: zHudCfgKey_BACKGROUND_TEXT.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_TEXT.
 */
char zHudCfgKey_BACKGROUND_TEXT[0x10] = "BACKGROUND_TEXT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x12-0x4e47f4
 * @recoil-artifact defines .data recoil:data:0x4e47f4: zHudCfgKey_BACKGROUND_VIDEOS.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_VIDEOS.
 */
char zHudCfgKey_BACKGROUND_VIDEOS[0x12] = "BACKGROUND_VIDEOS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x12-0x4e4808
 * @recoil-artifact defines .data recoil:data:0x4e4808: zHudCfgKey_BACKGROUND_IMAGES.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_BACKGROUND_IMAGES.
 */
char zHudCfgKey_BACKGROUND_IMAGES[0x12] = "BACKGROUND_IMAGES";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x12-0x4e4824
 * @recoil-artifact defines .data recoil:data:0x4e4824: zHudCfgKey_SHARED_IMAGE_PATH.
 * Purpose: preserve the recovered HUD global storage for zHudCfgKey_SHARED_IMAGE_PATH.
 */
char zHudCfgKey_SHARED_IMAGE_PATH[0x12] = "SHARED_IMAGE_PATH";
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_BACKGROUND_SOUNDS) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_CAPTURE) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_CURSOR) == 0x7);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_BACKGROUND_TEXT) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_BACKGROUND_VIDEOS) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_BACKGROUND_IMAGES) == 0x12);
RECOIL_STATIC_ASSERT(sizeof(zHudCfgKey_SHARED_IMAGE_PATH) == 0x12);

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduifillbitmap-zrdkey-fillbitmap
 * @recoil-artifact defines .data recoil:data:0x4e4764: g_HudUiFillBitmap_ZrdKey_FillBitmap.
 * Data owner: hud_ui.hud_ui_fill_bitmap_zrd_key_literals.
 * Purpose: name the FILLBITMAP ZRD record consumed by HudUiFillBitmap.
 */
char g_HudUiFillBitmap_ZrdKey_FillBitmap[] = "FILLBITMAP";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduizrdwidgetex17c-item-zrdkey-mouserect
 * @recoil-artifact defines .data recoil:data:0x4e4770: g_HudUiZrdWidgetEx17C_Item_ZrdKey_MouseRect.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the optional mouse-rectangle ZRD child loaded by CHudRadioButtonWidget.
 */
char g_HudUiZrdWidgetEx17C_Item_ZrdKey_MouseRect[] = "MOUSERECT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduizrdtoken-radio
 * @recoil-artifact defines .data recoil:data:0x4e477c: g_HudUiZrdToken_Radio.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the RADIO ZRD child array loaded by CHudRadioGroupWidget.
 */
char g_HudUiZrdToken_Radio[] = "RADIO";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudzrd-key-sound
 * @recoil-artifact defines .data recoil:data:0x4dc17c: g_HudZrd_Key_Sound.
 * Shared data owner: hud_ui.shared_zrd_sound_key_literal.
 * Purpose: name the shared SOUND ZRD record consumed by HUD widget, pickup,
 * and opt-catalog sound loaders.
 */
char g_HudZrd_Key_Sound[6] = "SOUND";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimessage-clearspecialtoken165
 * @recoil-artifact defines .data recoil:data:0x4dae08: g_HudUiMessage_ClearSpecialToken165.
 * Data owner: hud_ui.hud_ui_message_clear_special_token_literal.
 * Exact extent is the writable 4-byte .data object a5 00 00 00 referenced by
 * HudUiMessage::SetValueIfOwnerMatches and
 * Purpose: provide the special one-byte token string used to clear HUD message
 * panel text when the sentinel float value is passed.
 */
char g_HudUiMessage_ClearSpecialToken165[4] = "\xa5";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudlayout-typeisectionname
 * @recoil-artifact defines .data recoil:data:0x4dae0c: g_HudLayout_TypeISectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the TYPEI HUD layout section loaded from the HUD ZRD root.
 */
char g_HudLayout_TypeISectionName[] = "TYPEI";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hudlayout-typeiisectionname
 * @recoil-artifact defines .data recoil:data:0x4dae14: g_HudLayout_TypeIISectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the TYPEII HUD layout section loaded from the HUD ZRD root.
 */
char g_HudLayout_TypeIISectionName[] = "TYPEII";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduiblankspaces3
 * @recoil-artifact defines .data recoil:data:0x4dae1c: g_HudUiBlankSpaces3.
 * Data owner: hud_ui.hud_ui_message_layout_literals.
 * Exact extent is the writable 4-byte .data object 20 20 20 00 referenced by
 * Purpose: provide the initial blank weapon-message panel text.
 */
char g_HudUiBlankSpaces3[4] = "   ";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-hud-checkpointoverflowmsg
 * @recoil-artifact defines .data recoil:data:0x4dae20: g_Hud_CheckpointOverflowMsg.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable checkpoint-overflow diagnostic text used by
 */
char g_Hud_CheckpointOverflowMsg[20] = "Checkpoint overflow";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimessage-nodename
 * @recoil-artifact defines .data recoil:data:0x4dae40: g_HudUiMessage_NodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the objective HUD message node used for chat and save/load status prompts.
 */
char g_HudUiMessage_NodeName[8] = "Message";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-g-huduimessage-separatorcolon
 * @recoil-artifact defines .data recoil:data:0x4dae48: g_HudUiMessage_SeparatorColon.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: separate the local player name from chat text when composing HUD messages.
 */
char g_HudUiMessage_SeparatorColon[2] = ":";

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void(__cdecl* HudUiSensorWindowCrtInitializerFn)();
#pragma data_seg(".CRT$XCU")
/* VC5 emits this HUD sensor window startup callback as a direct .CRT$XCU row. */
HudUiSensorWindowCrtInitializerFn s_HudUiSensorWindowCrtInit = HudUiSensorWindow::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * Original inline helper; no standalone retail function exists. BN vtable
 * evidence at 0x4ce968, 0x4ce988, and 0x4ce9a8 points this slot at the
 * shared no-op body 0x404e80.
 * Original source name: HudLayoutBase::LayoutPreUpdate.
 * Purpose: preserve the typed HudLayoutBase virtual source model without
 * introducing a production FTable scaffold.
 */
void HudLayoutBase::LayoutPreUpdate() { }

/**
 * Original inline helper; no standalone retail function exists.
 * Original source name: HudLayoutBase::OnActivated.
 * Purpose: provide the default layout activation hook for derived HUD layouts.
 */
void HudLayoutBase::OnActivated() { }

namespace HudLayout {
} // namespace HudLayout

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-x0c
 * @recoil-artifact defines .data recoil:data:0x4e4870: g_HudUi_InvalidateMask.
 * Purpose: Stores g HudUi InvalidateMask data used by hud_ui.invalidate_mask_global.
 */
unsigned int g_HudUi_InvalidateMask = 0x0c;
}

namespace {
const char kNumericTextInputAcceptedRawKeyChars[] = "0123456789.-\x1b\r\x08\x7f\x02\x06";
extern "C" const char kClampedIntTextInputAcceptedRawKeyChars[] = "0123456789\x1b\r\x08\x7f\x02\x06";

#if defined(_MSC_VER) && _MSC_VER < 1200
// VC5 misparses explicit function-template calls such as FieldAt<unsigned int>(...).
// Keep the same call-site spelling for first-pass VC5 verification without changing
// modern compiler codegen.
template <typename T> class FieldAt {
public:
    /**
     * Original-source helper; no standalone retail function exists.
     * Evidence: recovered in the HUD source cluster near address-backed 0x4135f0 HudLayoutHW::Disable callers.
     * Purpose: preserve the recovered HUD behavior for FieldAt.
     */
    FieldAt(void* base, size_t offset)
        : address((T*)((unsigned char*)(base) + offset))
    {
    }

    /**
     * Original-source helper; no standalone retail function exists.
     * Evidence: recovered in the HUD source cluster near address-backed 0x4135f0 HudLayoutHW::Disable callers.
     * Purpose: preserve the recovered HUD behavior for FieldAt.
     */
    FieldAt(const void* base, size_t offset)
        : address((T*)((const unsigned char*)(base) + offset))
    {
    }

    operator T&()
    {
        return *address;
    }

    T* operator&() const
    {
        return address;
    }

    FieldAt& operator=(const T& value)
    {
        *address = value;
        return *this;
    }

    FieldAt& operator|=(const T& value)
    {
        *address |= value;
        return *this;
    }

    FieldAt& operator+=(const T& value)
    {
        *address += value;
        return *this;
    }

    FieldAt& operator-=(const T& value)
    {
        *address -= value;
        return *this;
    }

private:
    T* address;
};
#else
template <typename T>
/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4135f0 HudLayoutHW::Disable callers.
 * Purpose: preserve the recovered HUD behavior for FieldAt.
 */
T& FieldAt(void* base, size_t offset)
{
    return *(T*)((unsigned char*)(base) + offset);
}

template <typename T>
/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d220 HudUiListMenuEntry::CompareSortKey
 * callers. Purpose: preserve the recovered HUD behavior for FieldAt.
 */
const T& FieldAt(const void* base, size_t offset)
{
    return *(const T*)((const unsigned char*)(base) + offset);
}
#endif

const float kHudUiMessageClearSpecialTokenValue = 123456792.0f;

/**
 * Recovered original inline/static helper with no standalone retail function.
 * Observed in callers 0x4b5630, 0x4b5740, 0x4b5860, and 0x4b5900 as the
 * same HudUiPanelPtrVector begin/end loop dispatching HudUiElement::SetVisible.
 * Purpose: apply a visibility state to every panel in a recovered panel-vector
 * member while preserving the original HudUiZrdWidget source pattern.
 */
inline void HudUiSetPanelVectorVisible(HudUiPanelPtrVector& panels, int visible)
{
    for (HudUiPanelPtrVector::iterator it = panels.begin(); it != panels.end(); ++it) {
        (*it)->SetVisible(visible);
    }
}

struct HudUiZrdDeleteChildIfPresent {
    HudUiPanel* operator()(HudUiPanel* childWidgetOrNull)
    {
        return (HudUiPanel*)(HudUiZrdWidget::DeleteChildIfPresent(childWidgetOrNull));
    }
};

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayBase.
 */
inline zReader::Node* ZrdArrayBase(zReader::Node* node)
{
    if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    return node->value.nodes;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayCount.
 */
inline int ZrdArrayCount(zReader::Node* arrayBase)
{
    return arrayBase != 0 ? arrayBase[0].value.i32 : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayItem.
 */
inline zReader::Node* ZrdArrayItem(zReader::Node* arrayBase, int index)
{
    return arrayBase != 0 ? &arrayBase[index] : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayString.
 */
inline const char* ZrdArrayString(zReader::Node* arrayBase, int index)
{
    zReader::Node* const item = ZrdArrayItem(arrayBase, index);
    return item != 0 && item->type == zReader::ZRDR_NODE_STRING ? item->value.str : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayInt.
 */
inline int ZrdArrayInt(zReader::Node* arrayBase, int index, int fallback)
{
    zReader::Node* const item = ZrdArrayItem(arrayBase, index);
    return item != 0 && item->type == zReader::ZRDR_NODE_INT ? item->value.i32 : fallback;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for ZrdArrayFloat.
 */
inline float ZrdArrayFloat(zReader::Node* arrayBase, int index, float fallback)
{
    zReader::Node* const item = ZrdArrayItem(arrayBase, index);
    if (item == 0) {
        return fallback;
    }

    if (item->type == zReader::ZRDR_NODE_FLOAT) {
        return item->value.f32;
    }

    if (item->type == zReader::ZRDR_NODE_INT) {
        return (float)(item->value.i32);
    }

    return fallback;
}

} // namespace

namespace {

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdOwnerFontStyle.
 */
inline const HudFontStyle* HudUiZrdOwnerFontStyle(const HudUiBackground* owner, int styleIndex)
{
    const HudFontStyle* const style = &owner->fontStyles[styleIndex];
    return style->validMarker != 0 ? style : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: apply the recovered HUD layout or option state handled by ApplyHudFontStyleToPanel.
 */
inline void ApplyHudFontStyleToPanel(HudUiPanel* panel, const HudFontStyle* style)
{
    if (style == 0) {
        return;
    }

    panel->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
    panel->alignMode = style->alignMode;
    panel->textColor0 = style->textColor;
    panel->textColor1 = style->textColor;
    panel->textDirty = 1;
    panel->shadowEnabled = style->shadowEnabled;
    panel->shadowOffsetX = 1;
    panel->shadowOffsetY = 1;
    panel->bkMode = style->bkMode;
    panel->bkColor = style->bkColor;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: apply the recovered HUD layout or option state handled by ApplyHudFontStyleTextOnly.
 */
inline void ApplyHudFontStyleTextOnly(HudUiPanel* panel, const HudFontStyle* style)
{
    if (style == 0) {
        return;
    }

    panel->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
    panel->textColor0 = style->textColor;
    panel->textColor1 = style->textColor;
    panel->textDirty = 1;
    panel->shadowEnabled = style->shadowEnabled;
    panel->shadowOffsetX = 1;
    panel->shadowOffsetY = 1;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x413d30 HudUiLayoutNode::ApplyImageWidget callers.
 * Purpose: preserve the recovered HUD behavior for CreateHudZrdTextPanel.
 */
inline HudUiPanel* CreateHudZrdTextPanel(HudUiZrdWidget* widget, zReader::Node* textNode, int visible)
{
    zReader::Node* const textBase = ZrdArrayBase(textNode);
    if (textBase == 0) {
        return 0;
    }

    HudUiTransitionTextPanel* const transitionPanel
        = (HudUiTransitionTextPanel*)(::operator new(sizeof(HudUiTransitionTextPanel)));
    new (transitionPanel) HudUiTransitionTextPanel;

    HudUiPanel* const panel = (HudUiPanel*)(transitionPanel);
    const char* const key = ZrdArrayString(textBase, 1);
    const char* const text = key != 0 ? zLoc::ResolveMessageKeyOrFallback(key) : "";
    panel->SetTextFmt(text != 0 ? text : "");

    HudUiElement* const element = (HudUiElement*)(transitionPanel);
    element->SetPos(widget->originX + ZrdArrayInt(textBase, 2, 0), widget->originY + ZrdArrayInt(textBase, 3, 0));

    const int styleIndex = ZrdArrayInt(textBase, 4, 0);
    ApplyHudFontStyleTextOnly(panel, HudUiZrdOwnerFontStyle(widget->owner, styleIndex));

    element->SetVisible(visible);
    ((HudUiContainer*)(widget->owner))->AddChild(element);
    return panel;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x41ebd0 HudUiMgrSensor::TrackList_Reset callers.
 * Purpose: apply the recovered HUD layout or option state handled by ApplyHudZrdFlashSection.
 */
inline void ApplyHudZrdFlashSection(zReader::Node* parentNode, HudUiPanelPtrVector& panels)
{
    zReader::Node* const flashNode = zRdrGetNode(parentNode, g_HudZrd_Key_Flash);
    if (flashNode == 0) {
        return;
    }

    float flashRate = 0.0f;
    zReader::GetFloat(flashNode, g_HudZrd_Key_Rate, &flashRate);

    unsigned int flashColor = 0;
    zReader::Node* const colorNode = zRdrGetNode(flashNode, "COLOR");
    zReader::Node* const colorBase = ZrdArrayBase(colorNode);
    if (colorBase != 0) {
        const unsigned int red = (unsigned int)(ZrdArrayInt(colorBase, 1, 0)) & 0xffu;
        const unsigned int green = (unsigned int)(ZrdArrayInt(colorBase, 2, 0)) & 0xffu;
        const unsigned int blue = (unsigned int)(ZrdArrayInt(colorBase, 3, 0)) & 0xffu;
        flashColor = red | (green << 8) | (blue << 16);
    }

    if (flashRate == 0.0f) {
        return;
    }

    for (HudUiPanelPtrVector::iterator it = panels.begin(); it != panels.end(); ++it) {
        ((HudUiTransitionTextPanel*)(*it))->SetFlashColorAndRate(flashColor, flashRate);
    }
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x41ebd0 HudUiMgrSensor::TrackList_Reset callers.
 * Purpose: load the recovered HUD data handled by LoadHudZrdBitmap.
 */
inline void LoadHudZrdBitmap(zReader::Node* parentNode, const char* sectionName, zVidImagePartial** outImage)
{
    zReader::Node* const bitmapNode = zRdrGetNode(parentNode, sectionName);
    zReader::Node* const bitmapBase = ZrdArrayBase(bitmapNode);
    const char* const path = ZrdArrayString(bitmapBase, 1);
    if (path != 0) {
        *outImage = zImage::TexDirFindOrCreateByPath(path);
    }
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x41ebd0 HudUiMgrSensor::TrackList_Reset callers.
 * Purpose: load the recovered HUD data handled by LoadHudZrdSound.
 */
inline void LoadHudZrdSound(zReader::Node* parentNode, zSndSample** outSound, float* outScale)
{
    zReader::Node* const soundNode = zRdrGetNode(parentNode, g_HudZrd_Key_Sound);
    zReader::Node* const soundBase = ZrdArrayBase(soundNode);
    const char* const name = ZrdArrayString(soundBase, 1);
    if (name == 0) {
        return;
    }

    *outScale = ZrdArrayCount(soundBase) >= 3 ? ZrdArrayFloat(soundBase, 2, 1.0f) : 1.0f;
    *outSound = zSnd::FindSampleByName(name);
}

/*
 * Retail 0x4b59f0 contains these source constructs expanded at each state
 * section.  They are macros rather than callable helpers because the retail
 * body has no intervening helper calls and VC5 /Ob1 does not inline the
 * complete nested label loader when it is expressed as an ordinary function.
 */
#define HUD_ZRD_LOAD_BITMAP(parentNode_, sectionName_, outImage_)                                                      \
    do {                                                                                                               \
        zReader::Node* const hudBitmapNode_ = zRdrGetNode((parentNode_), (sectionName_));                              \
        if (hudBitmapNode_ != 0) {                                                                                     \
            zReader::Node* const hudBitmapBase_ = hudBitmapNode_->value.nodes;                                         \
            const char* const hudBitmapPath_ = hudBitmapBase_[1].value.str;                                            \
            {                                                                                                          \
                (outImage_) = zImage::TexDirFindOrCreateByPath(hudBitmapPath_);                                        \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

#define HUD_ZRD_LOAD_SOUND(parentNode_, outSound_, outScale_)                                                          \
    do {                                                                                                               \
        zReader::Node* const hudSoundNode_ = zRdrGetNode((parentNode_), g_HudZrd_Key_Sound);                           \
        if (hudSoundNode_ != 0) {                                                                                      \
            zReader::Node* const hudSoundBase_ = hudSoundNode_->value.nodes;                                           \
            const char* const hudSoundName_ = hudSoundBase_[1].value.str;                                              \
            {                                                                                                          \
                const float hudSoundScale_ = hudSoundBase_[0].value.i32 >= 3 ? hudSoundBase_[2].value.f32 : 1.0f;      \
                (outSound_) = zSnd::FindSampleByName(hudSoundName_);                                                   \
                (outScale_) = hudSoundScale_;                                                                          \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

#define HUD_ZRD_INSERT_LABEL_COUNTED(panels_, panel_) (panels_).push_back(panel_)

#define HUD_ZRD_INSERT_LABEL_NATURAL(panels_, panel_) (panels_).insert((panels_).end(), 1, (panel_))

#define HUD_ZRD_APPEND_LABEL_WITH_INSERT(widget_, panels_, labelSpecBase_, originX_, originY_, insert_)                \
    do {                                                                                                               \
        const int hudLabelOriginX_ = (originX_);                                                                       \
        const int hudLabelOriginY_ = (originY_);                                                                       \
        HudUiPanel* hudPanel_;                                                                                         \
        hudPanel_ = (HudUiPanel*)(new HudUiTransitionTextPanel);                                                       \
        HudUiElement* hudElement_;                                                                                     \
        hudElement_ = (HudUiElement*)(hudPanel_);                                                                      \
        hudElement_->flags = (hudElement_->flags & 0x10u) | 2;                                                         \
        const char* const hudLabelKey_ = (labelSpecBase_)[1].value.str;                                                \
        hudPanel_->SetTextFmt(zLoc::ResolveMessageKeyOrFallback(hudLabelKey_));                                        \
        hudElement_->SetPos(                                                                                           \
            hudLabelOriginX_ + (labelSpecBase_)[2].value.i32,                                                          \
            hudLabelOriginY_ + (labelSpecBase_)[3].value.i32                                                           \
        );                                                                                                             \
        const int hudStyleIndex_ = (labelSpecBase_)[4].value.i32;                                                      \
        const HudFontStyle* hudStyle_ = &((widget_)->owner->fontStyles[hudStyleIndex_]);                               \
        if (hudStyle_->validMarker == 0) {                                                                             \
            hudStyle_ = 0;                                                                                             \
        }                                                                                                              \
        if (hudStyle_ != 0) {                                                                                          \
            hudPanel_->alignMode = hudStyle_->alignMode;                                                               \
            hudPanel_->SetFont(hudStyle_->fontName, hudStyle_->fontSize, hudStyle_->fontWeight, 0, 0, 0, 2);           \
            hudPanel_->SetTextColorsAndMarkDirty(hudStyle_->textColor, hudStyle_->textColor);                          \
            hudPanel_->SetShadow(hudStyle_->shadowEnabled, 1, 1);                                                      \
            hudPanel_->SetTextBackground(hudStyle_->bkMode, hudStyle_->bkColor);                                       \
        }                                                                                                              \
        hudPanel_->SetVisible(1);                                                                                      \
        ((HudUiContainer*)((widget_)->owner))->AddChild(hudPanel_);                                                    \
        insert_((panels_), hudPanel_);                                                                                 \
    } while (0)

#define HUD_ZRD_APPEND_LABEL(widget_, panels_, labelSpecBase_, originX_, originY_)                                     \
    HUD_ZRD_APPEND_LABEL_WITH_INSERT(widget_, panels_, labelSpecBase_, originX_, originY_, HUD_ZRD_INSERT_LABEL_COUNTED)

#define HUD_ZRD_APPEND_LABEL_NATURAL(widget_, panels_, labelSpecBase_, originX_, originY_)                             \
    HUD_ZRD_APPEND_LABEL_WITH_INSERT(widget_, panels_, labelSpecBase_, originX_, originY_, HUD_ZRD_INSERT_LABEL_NATURAL)

inline void LoadHudZrdExpandedLabelArray(HudUiZrdWidget* widget, zReader::Node* labelBase, HudUiPanelPtrVector& panels);
#define HUD_ZRD_LOAD_LABEL_SECTION_WITH_APPEND(widget_, parentNode_, panels_, append_)                                 \
    do {                                                                                                               \
        zReader::Node* const hudLabelNode_ = zRdrGetNode((parentNode_), g_HudZrd_Key_Label);                           \
        if (hudLabelNode_ != 0) {                                                                                      \
            if (hudLabelNode_->value.nodes[1].type == zReader::ZRDR_NODE_ARRAY) {                                      \
                LoadHudZrdExpandedLabelArray((widget_), hudLabelNode_, (panels_));                                     \
            } else {                                                                                                   \
                append_((widget_), (panels_), hudLabelNode_->value.nodes, (widget_)->originX, (widget_)->originY);     \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)
#define HUD_ZRD_LOAD_LABEL_SECTION(widget_, parentNode_, panels_)                                                      \
    HUD_ZRD_LOAD_LABEL_SECTION_WITH_APPEND(widget_, parentNode_, panels_, HUD_ZRD_APPEND_LABEL)

#define HUD_ZRD_LOAD_LABEL_SECTION_NATURAL(widget_, parentNode_, panels_)                                              \
    HUD_ZRD_LOAD_LABEL_SECTION_WITH_APPEND(widget_, parentNode_, panels_, HUD_ZRD_APPEND_LABEL_NATURAL)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-apply-panel-vector-flash
 * Purpose: apply parsed flash color and rate to each panel in a widget state.
 *
 * Original inline helper hypothesis shared by the four flash sections in
 * retail 0x4b59f0. Parsing and the nonzero-rate check remain with the section;
 * the loop reloads the vector end after each panel update.
 */
inline void ApplyHudPanelVectorFlash(HudUiPanelPtrVector& panels, unsigned int color, float rate)
{
    for (HudUiPanelPtrVector::iterator it = panels.begin(); it != panels.end(); ++it) {
        ((HudUiTransitionTextPanel*)(*it))->SetFlashColorAndRate(color, rate);
    }
}
#define HUD_ZRD_APPLY_FLASH_SECTION(parentNode_, panels_)                                                              \
    do {                                                                                                               \
        zReader::Node* const hudFlashNode_ = zRdrGetNode((parentNode_), g_HudZrd_Key_Flash);                           \
        if (hudFlashNode_ != 0) {                                                                                      \
            float hudFlashRate_ = 0.0f;                                                                                \
            zReader::Node* const hudRateNode_ = zRdrGetNode(hudFlashNode_, g_HudZrd_Key_Rate);                         \
            if (hudRateNode_ != 0) {                                                                                   \
                hudFlashRate_ = hudRateNode_->value.nodes[1].value.f32;                                                \
            }                                                                                                          \
            unsigned int hudFlashColor_ = 0;                                                                           \
            zReader::Node* const hudColorNode_ = zRdrGetNode(hudFlashNode_, "COLOR");                                  \
            if (hudColorNode_ != 0) {                                                                                  \
                zReader::Node* const hudColorBase_ = hudColorNode_->value.nodes;                                       \
                const unsigned int hudRed_ = (unsigned int)(hudColorBase_[1].value.i32) & 0xffu;                       \
                const unsigned int hudGreen_ = (unsigned int)(hudColorBase_[2].value.i32) & 0xffu;                     \
                const unsigned int hudBlue_ = (unsigned int)(hudColorBase_[3].value.i32) & 0xffu;                      \
                hudFlashColor_ = hudRed_ | (hudGreen_ << 8) | (hudBlue_ << 16);                                        \
            }                                                                                                          \
            if (hudFlashRate_ != 0.0f) {                                                                               \
                ApplyHudPanelVectorFlash((panels_), hudFlashColor_, hudFlashRate_);                                    \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-load-zrd-label-array
 * Purpose: create and register every label in an array-valued widget section.
 *
 * Original inline helper hypothesis shared by all four state arrays at retail
 * 0x4b59f0. The array loader uses counted insertion; scalar sections use
 * push_back. The original lexical boundary is unknown. Fresh VC5 comparison
 * must establish each physical insert target and its helper expansion.
 */
inline void LoadHudZrdExpandedLabelArray(HudUiZrdWidget* widget, zReader::Node* labelBase, HudUiPanelPtrVector& panels)
{
    const int labelCount = labelBase->value.nodes[0].value.i32;
    for (int labelIndex = 1; labelIndex <= labelCount - 1; ++labelIndex) {
        HUD_ZRD_APPEND_LABEL_NATURAL(
            widget,
            panels,
            (labelBase->value.nodes[labelIndex].value.nodes),
            widget->originX,
            widget->originY
        );
    }
}

} // namespace

namespace HudUiMgrSensor {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-tracklist-add
 * @recoil-artifact defines .text recoil:function:0x438920: HudUiMgrSensor::TrackListAdd.
 * @recoil-match byte
 *
 * Purpose: append one payload-bearing sensor tracking node to the recovered
 * global track-list owner while preserving its head, tail, and count fields.
 */
HudUiMgrSensorTrackNode* __fastcall TrackListAdd(int trackKind, void* payload)
{
    HudUiMgrSensorTrackNode* const trackNode = (HudUiMgrSensorTrackNode*)(malloc(sizeof(HudUiMgrSensorTrackNode)));
    memset(trackNode, 0, sizeof(HudUiMgrSensorTrackNode));

    if (trackNode != 0) {
        trackNode->next = 0;
        if (g_HudUiMgrSensor_TrackList.count == 0) {
            g_HudUiMgrSensor_TrackList.head = trackNode;
        } else {
            g_HudUiMgrSensor_TrackList.tail->next = trackNode;
        }

        g_HudUiMgrSensor_TrackList.tail = trackNode;
        trackNode->next = 0;
        ++g_HudUiMgrSensor_TrackList.count;
    }

    trackNode->trackKind = trackKind;
    trackNode->payload = payload;
    return trackNode;
}

} // namespace HudUiMgrSensor

namespace HudUiMgrTarget {
} // namespace HudUiMgrTarget

namespace HudUiMgrObjective {
/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: repeated objective phase runtime update of the widget right
 * edge after slide-position changes.
 * Purpose: refresh the cached objective widget right edge from its current
 * center position and borrowed image width.
 */
static void HudUiMgrObjectiveUpdateWidgetRightX()
{
    const zVidImagePartial* const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;
    g_HudUiMgrObjectiveWidgetRightX = g_HudUiMgrObjectiveWidget.GetCenterX() + width;
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: repeated phase animation sequence updates the objective bar
 * slide edge, invalidates the bar, moves the widget, and recomputes meter X
 * points as one source-level operation.
 * Purpose: apply the objective panel slide X position and dependent meter
 * geometry.
 */
static void HudUiMgrObjectiveSetSlidePosition(float slideX)
{
    g_HudUiMgrObjectiveBar.points[2].x = slideX;
    g_HudUiMgrObjectiveBar.points[3].x = slideX;
    g_HudUiMgrObjectiveBar.Invalidate();
    ((HudUiElement*)(&g_HudUiMgrObjectiveWidget))->SetX((int)(slideX)-1);
    HudUiMgrObjective::UpdateMeterXPoints();
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: phase-3 animation branches share the same hardware-HUD dirty
 * rectangle gate through zOpt::GetHudTypeForCurrentHwMode.
 * Purpose: update the hardware HUD objective dirty rectangle only for the
 * hardware perspective HUD mode.
 */
static void HudUiMgrObjectiveUpdateHwDirtyRectIfNeeded()
{
    if (zOpt::GetHudTypeForCurrentHwMode() == 2) {
        g_HudLayoutHW.UpdateObjectiveDirtyRect();
    }
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: phase-1 and phase-3 animation branches share the sensor
 * image null guard, mirrored fade-to-noise calculation, visibility update, and
 * zVid::DrawNoiseRect call sequence.
 * Purpose: draw objective sensor transition noise while optionally revealing or
 * hiding the sensor rectangle when the fade passes the midpoint.
 */
static void HudUiMgrObjectiveDrawSensorNoise(float fade, int visibleWhenCovered)
{
    if (g_HudUiMgrObjectiveSensorRect.image == 0) {
        return;
    }

    float noise = fade + fade;
    if (noise < 1.0f) {
        zVid::DrawNoiseRect((zVidRect32*)(&g_HudUiMgrSensorBlock.sensorRectRaw), (double)(noise));
        return;
    }

    g_HudUiMgrObjectiveSensorRect.SetVisible(visibleWhenCovered);
    zVid::DrawNoiseRect((zVidRect32*)(&g_HudUiMgrSensorBlock.sensorRectRaw), (double)(2.0f - noise));
}

} // namespace HudUiMgrObjective

namespace {
/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4137c0 HudUiAuxOverlay::ClearTextLines callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdPayload.
 */
zReader::Node* HudUiZrdPayload(zReader::Node* node)
{
    return node != 0 && node->type == zReader::ZRDR_NODE_ARRAY ? node->value.nodes : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4137c0 HudUiAuxOverlay::ClearTextLines callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdStringAt.
 */
const char* HudUiZrdStringAt(zReader::Node* payload, int index)
{
    return payload != 0 ? payload[index].value.str : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4137c0 HudUiAuxOverlay::ClearTextLines callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdIntAt.
 */
int HudUiZrdIntAt(zReader::Node* payload, int index)
{
    return payload != 0 ? payload[index].value.i32 : 0;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiSetFontFromRect.
 */
void HudUiSetFontFromRect(HudUiPanel* panel, const HudUiRect& fontSpec)
{
    panel->SetFont((const char*)(fontSpec.left), fontSpec.right, fontSpec.bottom, fontSpec.top, 0, 0, 2);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiSetPanelClipWithSource.
 */
inline void HudUiSetPanelClipWithSource(HudUiPanel* panel, void* source, const HudUiRect* clipRect)
{
    panel->SetBltSourceAndClipRect(source, clipRect);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiApplyStatsTripletInt3.
 */
void HudUiApplyStatsTripletInt3(zReader::Node* payload, int nodeIndex, int& outX, int& outY, int* outZ = 0)
{
    HudUiLayoutNode::ReadInt3(&payload[nodeIndex], &outX, &outY, outZ);
}
} // namespace

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-hud-ui-text-input
 * @recoil-artifact defines .text recoil:function:0x4b42f0: HudUiTextInput::HudUiTextInput.
 * @recoil-match byte
 */
HudUiTextInput::HudUiTextInput(int bufferSize)
{
    HudUiTextInput* const input = this;
    input->cursor = 0;
    input->buffer = 0;
    input->capacity = 0;
    input->AllocTextBuffer(bufferSize);

    {
        for (int code = 0; code < 0x100; ++code) {
            if (isprint(code) != 0) {
                input->keyActionMap[code] = 0;
            } else {
                input->keyActionMap[code] = 1;
            }
        }
    }

    input->keyActionMap[0x20] = 0;
    input->keyActionMap[0x2e] = 0;
    input->keyActionMap[0x1b] = 2;
    input->keyActionMap[0x0d] = 3;
    input->keyActionMap[0x08] = 4;
    input->keyActionMap[0x7f] = 5;
    input->keyActionMap[0x02] = 6;
    input->keyActionMap[0x06] = 7;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-destroy-hud-ui-text-input
 * @recoil-artifact defines .text recoil:function:0x4b4370: HudUiTextInput::~HudUiTextInput.
 * @recoil-match byte
 */
HudUiTextInput::~HudUiTextInput()
{
    char* const ownedBuffer = buffer;
    ::operator delete(ownedBuffer);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-alloc-text-buffer
 * @recoil-artifact defines .text recoil:function:0x4b4390: HudUiTextInput::AllocTextBuffer.
 * @recoil-match byte
 */
void HudUiTextInput::AllocTextBuffer(int bufferSize)
{
    char* const newBuffer = (char*)(::operator new(bufferSize));
    char* const oldBuffer = buffer;
    if (oldBuffer != 0) {
        int copyCount = capacity;
        if (bufferSize < copyCount) {
            copyCount = bufferSize;
        }

        strncpy(newBuffer, oldBuffer, copyCount);
    }

    capacity = bufferSize;
    buffer = newBuffer;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-set-contents
 * @recoil-artifact defines .text recoil:function:0x4b43d0: HudUiTextInput::SetContents.
 * @recoil-match byte
 */
void HudUiTextInput::SetContents(const char* source)
{
    strncpy(buffer, source, capacity);
    buffer[capacity - 1] = '\0';
    SetCursorPosition((int)(cursor));
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-get-buffer
 * @recoil-artifact defines .text recoil:function:0x4b4410: HudUiTextInput::GetBuffer.
 * @recoil-match byte
 */
char* HudUiTextInput::GetBuffer()
{
    return buffer;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-set-cursor-position
 * @recoil-artifact defines .text recoil:function:0x4b4420: HudUiTextInput::SetCursorPosition.
 * @recoil-match byte
 */
void HudUiTextInput::SetCursorPosition(int position)
{
    cursor = (position < (int)(strlen(buffer))) ? (unsigned int)(position) : (unsigned int)(strlen(buffer));
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-dispatch-key-action
 * @recoil-artifact defines .text recoil:function:0x4b4460: HudUiTextInput::DispatchKeyAction.
 * @recoil-match source
 */
void HudUiTextInput::DispatchKeyAction(int key)
{
    const int keyIndex = (signed char)(key);
    const int action = (signed char)(keyActionMap[keyIndex]);

    switch (action) {
    case 0:
        OnPrintableKey(key);
        break;
    case 1:
        OnIgnoredKey(key);
        break;
    case 2:
        OnCancel();
        break;
    case 3:
        OnAccept();
        break;
    case 4:
        OnBackspace();
        break;
    case 5:
        OnDeleteForward();
        break;
    case 6:
        OnMoveCursorLeft();
        break;
    case 7:
        OnMoveCursorRight();
        break;
    default:
        break;
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-insert-char-at-cursor
 * @recoil-artifact defines .text recoil:function:0x4b44e0: HudUiTextInput::InsertCharAtCursor.
 * @recoil-match byte
 */
void HudUiTextInput::InsertCharAtCursor(int ch)
{
    const int textLength = (int)(strlen(buffer));
    if (textLength < (int)(capacity)-1) {
        ShiftTextRight(1, (int)(cursor));
        buffer[cursor] = (char)(ch);
        ++cursor;
    } else {
        OnOverflow();
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-backspace-delete-char
 * @recoil-artifact defines .text recoil:function:0x4b4530: HudUiTextInput::BackspaceDeleteChar.
 * @recoil-match byte
 */
void HudUiTextInput::BackspaceDeleteChar()
{
    if ((int)(cursor) > 0) {
        --cursor;
        ShiftTextLeft(1, (int)(cursor));
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-delete-char-forward
 * @recoil-artifact defines .text recoil:function:0x4b4550: HudUiTextInput::DeleteCharForward.
 * @recoil-match byte
 */
void HudUiTextInput::DeleteCharForward()
{
    ShiftTextLeft(1, (int)(cursor));
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-move-cursor-left
 * @recoil-artifact defines .text recoil:function:0x4b4560: HudUiTextInput::MoveCursorLeft.
 * @recoil-match byte
 */
void HudUiTextInput::MoveCursorLeft()
{
    if ((int)(cursor) > 0) {
        --cursor;
    }
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-text-input-move-cursor-right
 * @recoil-artifact defines .text recoil:function:0x4b4570: HudUiTextInput::MoveCursorRight.
 * @recoil-match byte
 *
 * Purpose: advance the insertion cursor while it precedes the end of the text.
 */
void HudUiTextInput::MoveCursorRight()
{
    if ((int)cursor < (int)strlen(buffer)) {
        ++cursor;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduitextinput-shifttextright
 * @recoil-artifact defines .text recoil:function:0x4b4590: HudUiTextInput::ShiftTextRight.
 * @recoil-match source
 *
 * Purpose: make room in the input buffer by shifting its suffix right.
 */
int HudUiTextInput::ShiftTextRight(int count, int startPos)
{
    int result = 0;
    const int length = (int)strlen(buffer);
    int index = length + count;
    if (index < (int)(capacity)) {
        while (index > startPos) {
            buffer[index] = buffer[index - count];
            --index;
        }
        result = 1;
    }
    return result;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-text-input-shift-text-left
 * @recoil-artifact defines .text recoil:function:0x4b45e0: HudUiTextInput::ShiftTextLeft.
 * @recoil-match byte
 */
int HudUiTextInput::ShiftTextLeft(int count, int startPos)
{
    const int textLength = (int)(strlen(buffer));
    {
        for (int index = startPos; index < textLength; ++index) {
            buffer[index] = buffer[index + count];
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-slider-border-hud-ui-slider-border
 * @recoil-artifact defines .text recoil:function:0x4b4620: HudUiSliderBorder::HudUiSliderBorder.
 * @recoil-match byte
 */
HudUiSliderBorder::HudUiSliderBorder()
{
    originX = 0;
    originY = 0;
    halfWidth = 1;
    height = 10;
    blinkEnabled = 0;
    blinkPeriodSec = 0.35f;
    blinkDirSign = 1;
    blinkTimeRemainingSec = 0.0f;

    SetPoint(0, -1, 0);
    SetPoint(1, halfWidth, 0);
    SetPoint(2, halfWidth, 1);
    SetPoint(3, 0, 1);
    SetPoint(4, 0, height - 1);
    SetPoint(5, halfWidth, height - 1);
    SetPoint(6, halfWidth, height);
    SetPoint(7, -halfWidth, height);
    SetPoint(8, -halfWidth, height - 1);
    SetPoint(9, 0, height - 1);
    SetPoint(10, 0, 1);
    SetPoint(11, -halfWidth, 1);
    SetPoint(12, -halfWidth, 0);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-slider-border-update
 * @recoil-artifact defines .text recoil:function:0x4b47b0: HudUiSliderBorder::Update.
 * @recoil-match byte
 */
void HudUiSliderBorder::Update(float deltaSeconds)
{
    if (((~flags) & 0x10) == 0) {
        return;
    }

    if (blinkEnabled != 0) {
        blinkTimeRemainingSec -= deltaSeconds;
        if (blinkTimeRemainingSec < 0.0) {
            blinkDirSign = -blinkDirSign;
            blinkTimeRemainingSec = blinkPeriodSec;
        }
        if (blinkDirSign == 1) {
            HudUiPolyline::Draw();
        }
    } else {
        HudUiPolyline::Draw();
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-slider-border-set-bounds
 * @recoil-artifact defines .text recoil:function:0x4b4810: HudUiSliderBorder::SetBounds.
 * @recoil-match byte
 */
void HudUiSliderBorder::SetBounds(int newOriginX, int newOriginY, int newHalfWidth, int newHeight)
{
    originX = newOriginX;
    originY = newOriginY;
    halfWidth = newHalfWidth;
    height = newHeight;

    SetPoint(0, originX - halfWidth, originY);
    SetPoint(1, originX + halfWidth, originY);
    SetPoint(2, originX + halfWidth, originY + 1);
    SetPoint(3, originX, originY + 1);
    SetPoint(4, originX, originY + height - 1);
    SetPoint(5, originX + halfWidth, originY + height - 1);
    SetPoint(6, originX + halfWidth, originY + height);
    SetPoint(7, originX - halfWidth, originY + height);
    SetPoint(8, originX - halfWidth, originY + height - 1);
    SetPoint(9, originX, originY + height - 1);
    SetPoint(10, originX, originY + 1);
    SetPoint(11, originX - halfWidth, originY + 1);
    SetPoint(12, originX - halfWidth, originY);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-numeric-text-input-hud-ui-numeric-text-input
 * @recoil-artifact defines .text recoil:function:0x4b49e0: HudUiNumericTextInput::HudUiNumericTextInput.
 * @recoil-match byte
 */
HudUiNumericTextInput::HudUiNumericTextInput()
    : HudUiZrdWidget()
    , textInput(0x100)
    , sliderBorder()
    , sliderVisibleWhenInputActive(0)
    , rawKeyFilterEnabled(0)
{
    sliderBorder.inputActive = 1;
    sliderBorder.caretHalfWidth = 0;

    HudUiElement* sliderElement = &sliderBorder;
    sliderElement->SetVisible(1);
    HudUiNumericTextInput* ownerSelf = this;
    textInput.owner = ownerSelf;
    HudUiElement* element = ownerSelf;
    element->SetVisible(1);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduinumerictextinput-huduinumerictextinput
 * @recoil-artifact defines .text recoil:function:0x4b4ac0: HudUiNumericTextInput::~HudUiNumericTextInput.
 * @recoil-match byte
 *
 * Binary Ninja shows VC5 destructor codegen: derived vtable restore, raw
 * keyboard capture release, embedded HudUiOwnedTextInput teardown, then
 * HudUiZrdWidget cleanup with EH state transitions.
 * Purpose: Disable raw keyboard capture before C++ member/base destruction.
 */
HudUiNumericTextInput::~HudUiNumericTextInput()
{
    SetRawKeyboardCapture(0);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-numeric-text-input-raw-keyboard-callback
 * @recoil-artifact defines .text recoil:function:0x4b4b30: HudUiNumericTextInput::RawKeyboardCallback.
 * @recoil-match byte
 */
int __fastcall HudUiNumericTextInput::RawKeyboardCallback(int key, HudUiNumericTextInput* callbackCtx)
{
    if (callbackCtx != 0) {
        return callbackCtx->OnRawKeyboardChar(key);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-numeric-text-input-on-raw-keyboard-char
 * @recoil-artifact defines .text recoil:function:0x4b4b50: HudUiNumericTextInput::OnRawKeyboardChar.
 * @recoil-match byte
 */
int HudUiNumericTextInput::OnRawKeyboardChar(int key)
{
    if (rawKeyFilterEnabled != 0) {
        if (strchr(kNumericTextInputAcceptedRawKeyChars, key) == 0) {
            return 0;
        }
        textInput.DispatchKeyAction(key);
        return 0;
    }

    textInput.DispatchKeyAction(key);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-numeric-text-input-set-input-active
 * @recoil-artifact defines .text recoil:function:0x4b4ba0: HudUiNumericTextInput::SetInputActive.
 * @recoil-match byte
 */
int HudUiNumericTextInput::SetInputActive(int active)
{
    HudUiPanel* firstLabelPanel = 0;
    const int previousActive = sliderBorder.inputActive;
    sliderBorder.inputActive = active;

    // VC5 vector::empty() uses the null-aware size() calculation,
    // matching the retail count and Boolean materialization.
    if (!labelPanels.empty()) {
        firstLabelPanel = labelPanels[0];
    }

    if (active != 0) {
        SetVisible(1);
        sliderBorder.SetVisible(1);
        if (firstLabelPanel != 0) {
            firstLabelPanel->SetVisible(1);
        }
    } else {
        SetVisible(0);
        if (firstLabelPanel != 0) {
            firstLabelPanel->SetVisible(0);
        }
        sliderBorder.SetVisible(0);
    }

    return previousActive;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-numeric-text-input-set-raw-keyboard-capture
 * @recoil-artifact defines .text recoil:function:0x4b4c50: HudUiNumericTextInput::SetRawKeyboardCapture.
 * @recoil-match byte
 */
void HudUiNumericTextInput::SetRawKeyboardCapture(int enable)
{
    const char enableByte = (char)(enable);
    if (enableByte == sliderVisibleWhenInputActive) {
        return;
    }

    sliderVisibleWhenInputActive = enableByte;
    if (enableByte != 0) {
        zInput::KeyboardSetRawEventCallback((void*)(&HudUiNumericTextInput::RawKeyboardCallback), this);
    } else {
        zInput::KeyboardSetRawEventCallback(0, 0);
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-numeric-text-input-on-activate
 * @recoil-artifact defines .text recoil:function:0x4b4c90: HudUiNumericTextInput::OnActivate.
 * @recoil-match byte
 */
void HudUiNumericTextInput::OnActivate()
{
    sliderBorder.inputActive = 1;
    HudUiZrdWidget::OnActivate();
}

/**
 * Purpose: Store the packed RGB565 line color.
 */
inline void HudUiPolyline::SetColor(int color)
{
    color565 = color;
}

RECOIL_NO_GS void HudUiNumericTextInput::Update(float deltaSeconds)
{
    if ((~flags & 0x10) == 0) {
        labelPanels[0]->SetVisible(0);
        labelPanels[0]->Invalidate();
        sliderBorder.SetVisible(0);
        sliderBorder.Invalidate();
        return;
    }

    if (sliderVisibleWhenInputActive != 0) {
        labelPanels[0]->SetVisible(1);
        char* const buffer = textInput.GetBuffer();

        if (labelPanels.size() != 0) {
            labelPanels[0]->SetText(buffer);
        }

        RECT textRect;
        textRect.left = labelPanels[0]->GetCenterX();
        textRect.top = labelPanels[0]->GetCenterY();
        textRect.right = labelPanels[0]->GetCenterX();
        textRect.bottom = labelPanels[0]->GetCenterY();

        if (labelPanels[0]->MeasureTextPrefixRect((int)(textInput.cursor), &textRect) != 0) {
            const unsigned int textColor = labelPanels[0]->textColor0;
            sliderBorder.SetColor((int)(zVidPackColorRGB(GetRValue(textColor), GetGValue(textColor), textColor >> 16)
                & 0xffffu));
            sliderBorder
                .SetBounds(textRect.right, textRect.top, sliderBorder.caretHalfWidth, textRect.bottom - textRect.top);
            sliderBorder.SetVisible(1);
        }

        Invalidate();
    } else {
        sliderBorder.SetVisible(0);
    }

    HudUiElement::Update(deltaSeconds);
    sliderBorder.Update(deltaSeconds);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-numeric-text-input-alloc-text-buffer
 * @recoil-artifact defines .text recoil:function:0x4b4e40: HudUiNumericTextInput::AllocTextBuffer.
 * @recoil-match byte
 */
void HudUiNumericTextInput::AllocTextBuffer(unsigned int bufferSize)
{
    textInput.AllocTextBuffer(bufferSize);
}

void HudUiNumericTextInput::Update(const char* text)
{
    textInput.SetContents(text);
    textInput.SetCursorPosition((int)(strlen(text)));
    char* const buffer = textInput.GetBuffer();

    if (labelPanels.size() != 0) {
        HudUiPanel* const firstPanel = labelPanels[0];
        firstPanel->SetText(buffer);
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-numeric-text-input-get-buffer
 * @recoil-artifact defines .text recoil:function:0x4b4ed0: HudUiNumericTextInput::GetBuffer.
 * @recoil-match byte
 */
char* HudUiNumericTextInput::GetBuffer()
{
    return textInput.GetBuffer();
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-zrd-widget-hud-ui-zrd-widget
 * @recoil-artifact defines .text recoil:function:0x4b4ee0: HudUiZrdWidget::HudUiZrdWidget.
 * @recoil-match byte
 */
HudUiZrdWidget::HudUiZrdWidget()
    : HudUiWidget(0)
{
    modeOrEnabled = 1;
    originY = 0;
    originX = 0;
    owner = 0;
    defaultImage = 0;
    rolloverImage = 0;
    disabledImage = 0;
    rolloverSound = 0;
    rolloverSoundScale = 1.0f;
    rolloverPlayHandle = 0;
    activateImage = 0;
    activateSound = 0;
    activateSoundScale = 1.0f;
    activatePlayHandle = 0;

    labelPanels.clear();
    rolloverLabelPanels.clear();
    activateLabelPanels.clear();
    (void)labelPanels.empty();
    (void)rolloverLabelPanels.empty();

    *((unsigned short*)(&imageStateWord)) = 1;
    HudUiElement* element = this;
    element->Invalidate();
    unsigned int visibleFlag = (unsigned char)(flags);
    flags = (unsigned char)((visibleFlag & ~0xefu) | 0x02u);
}

HudUiZrdWidget::~HudUiZrdWidget()
{
    {
        HudUiZrdDeleteChildIfPresent deleteChildIfPresent;
        HudUiZrdDeleteChildIfPresent deleteChildIfPresentCopy(deleteChildIfPresent);
        std::transform(labelPanels.begin(), labelPanels.end(), labelPanels.begin(), deleteChildIfPresentCopy);
    }

    {
        HudUiPanelPtrVector::iterator rolloverIt = rolloverLabelPanels.begin();
        HudUiPanelPtrVector::iterator rolloverOut = rolloverLabelPanels.begin();
        HudUiPanelPtrVector::iterator rolloverEnd = rolloverLabelPanels.end();
        while (rolloverIt != rolloverEnd) {
            if (*rolloverIt != 0) {
                delete (*rolloverIt);
            }

            *rolloverOut = 0;
            ++rolloverIt;
            ++rolloverOut;
        }
    }

    {
        HudUiPanelPtrVector::iterator activateIt = activateLabelPanels.begin();
        HudUiPanelPtrVector::iterator activateOut = activateLabelPanels.begin();
        HudUiPanelPtrVector::iterator activateEnd = activateLabelPanels.end();
        while (activateIt != activateEnd) {
            if (*activateIt != 0) {
                delete (*activateIt);
            }

            *activateOut = 0;
            ++activateIt;
            ++activateOut;
        }
    }

    typedef HudUiPanelPtrVector::iterator (HudUiPanelPtrVector::*ErasePanelRangeMethod)(
        HudUiPanelPtrVector::iterator,
        HudUiPanelPtrVector::iterator
    );
    ErasePanelRangeMethod erasePanelRange = (ErasePanelRangeMethod)(&HudUiPanelPtrVector::erase);
    (labelPanels.*erasePanelRange)(labelPanels.begin(), labelPanels.end());
    (rolloverLabelPanels.*erasePanelRange)(rolloverLabelPanels.begin(), rolloverLabelPanels.end());
    (activateLabelPanels.*erasePanelRange)(activateLabelPanels.begin(), activateLabelPanels.end());

    if (defaultImage != 0 && defaultImage != image) {
        defaultImage = zVid_Image::ReleaseIfNotDefault(defaultImage);
    }

    if (activateImage != 0 && activateImage != image) {
        activateImage = zVid_Image::ReleaseIfNotDefault(activateImage);
    }

    if (rolloverImage != 0 && rolloverImage != image) {
        rolloverImage = zVid_Image::ReleaseIfNotDefault(rolloverImage);
    }

    if (disabledImage != 0 && disabledImage != image) {
        disabledImage = zVid_Image::ReleaseIfNotDefault(disabledImage);
    }

    if (image != 0 && ownsImage == 0) {
        zVid_Image::ReleaseIfNotDefault(image);
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-zrd-widget-delete-child-if-present
 * @recoil-artifact defines .text recoil:function:0x4b52f0: HudUiZrdWidget::DeleteChildIfPresent.
 * @recoil-match byte
 */
void* __stdcall HudUiZrdWidget::DeleteChildIfPresent(void* childWidgetOrNull)
{
    if (childWidgetOrNull != 0) {
        delete ((HudUiElement*)(childWidgetOrNull));
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-zrd-widget-invalidate
 * @recoil-artifact defines .text recoil:function:0x4b5310: HudUiZrdWidget::Invalidate.
 * @recoil-match byte
 */
void HudUiZrdWidget::Invalidate()
{
    HudUiElement::Invalidate();

    HudUiPanelPtrVector::iterator panel = labelPanels.begin();
    if (panel == 0) {
        return;
    }

    while (panel != labelPanels.end()) {
        HudUiPanel* const label = *panel;
        label->Invalidate();
        ++panel;
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-zrd-widget-get-bounds-rect-or-null
 * @recoil-artifact defines .text recoil:function:0x4b5350: HudUiZrdWidget::GetBoundsRectOrNull.
 * @recoil-match byte
 */
HudUiRect* HudUiZrdWidget::GetBoundsRectOrNull()
{
    HudUiRect* result = 0;
    zVidImagePartial* const widgetImage = image;
    if (modeOrEnabled == 0) {
        return result;
    }

    if (widgetImage != 0) {
        boundsRect.top = y;
        boundsRect.left = x;
        boundsRect.bottom = y + widgetImage->height;
        boundsRect.right = x + widgetImage->width;
        result = &boundsRect;
    } else if (labelPanels.begin() != 0) {
        boundsRect.top = labelPanels[0]->GetCenterY();
        boundsRect.bottom = labelPanels[0]->QueryTextHeight() + boundsRect.top;

        for (HudUiPanelPtrVector::iterator panelIt = labelPanels.begin(); panelIt != labelPanels.end(); ++panelIt) {
            boundsRect.bottom += (*panelIt)->QueryTextHeight();

            switch ((*panelIt)->alignMode) {
            case 0:
                boundsRect.left = labelPanels[0]->GetCenterX();
                boundsRect.right = (*panelIt)->GetCenterX() + (*panelIt)->QueryTextWidth() > boundsRect.right
                    ? (*panelIt)->GetCenterX() + (*panelIt)->QueryTextWidth()
                    : boundsRect.right;
                break;

            case 1:
                boundsRect.left = (*panelIt)->GetCenterX() - (*panelIt)->QueryTextWidth() / 2 < boundsRect.left
                    ? (*panelIt)->GetCenterX() - (*panelIt)->QueryTextWidth() / 2
                    : boundsRect.left;
                boundsRect.right = (*panelIt)->GetCenterX() + (*panelIt)->QueryTextWidth() / 2 > boundsRect.right
                    ? (*panelIt)->GetCenterX() + (*panelIt)->QueryTextWidth() / 2
                    : boundsRect.right;
                break;

            case 2:
                boundsRect.right = labelPanels[0]->GetCenterX();
                boundsRect.left = (*panelIt)->GetCenterX() - (*panelIt)->QueryTextWidth() > boundsRect.left
                    ? (*panelIt)->GetCenterX() - (*panelIt)->QueryTextWidth()
                    : boundsRect.left;
                break;
            }
        }

        boundsRect.bottom -= labelPanels[0]->QueryTextHeight();
        result = &boundsRect;
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-zrd-widget-show-preview
 * @recoil-artifact defines .text recoil:function:0x4b5630: HudUiZrdWidget::ShowPreview.
 * @recoil-match byte
 */
void HudUiZrdWidget::ShowPreview()
{
    if (rolloverImage != 0) {
        if (defaultImage == 0) {
            defaultImage = image;
        }

        SetImageBorrowedAndInvalidate(rolloverImage);
    }

    if (rolloverSound != 0) {
        rolloverPlayHandle = rolloverSound->PlayA3DSimple(rolloverSoundScale);
    }

    if (rolloverLabelPanels.begin() != 0) {
        HudUiSetPanelVectorVisible(labelPanels, 0);
        HudUiSetPanelVectorVisible(activateLabelPanels, 0);
        HudUiSetPanelVectorVisible(rolloverLabelPanels, 1);
        return;
    }

    HudUiSetPanelVectorVisible(labelPanels, 1);
    HudUiSetPanelVectorVisible(activateLabelPanels, 0);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-zrd-widget-refresh-state
 * @recoil-artifact defines .text recoil:function:0x4b5740: HudUiZrdWidget::RefreshState.
 * @recoil-match byte
 */
void HudUiZrdWidget::RefreshState()
{
    for (HudUiPanelPtrVector::iterator rolloverIt = rolloverLabelPanels.begin();
        rolloverIt != rolloverLabelPanels.end();
        ++rolloverIt) {
        (*rolloverIt)->SetVisible(0);
    }

    for (HudUiPanelPtrVector::iterator activateIt = activateLabelPanels.begin();
        activateIt != activateLabelPanels.end();
        ++activateIt) {
        (*activateIt)->SetVisible(0);
    }

    if (modeOrEnabled != 0) {
        for (HudUiPanelPtrVector::iterator labelIt = labelPanels.begin(); labelIt != labelPanels.end(); ++labelIt) {
            (*labelIt)->SetVisible(1);
        }
        for (HudUiPanelPtrVector::iterator disabledIt = disabledLabelPanels.begin();
            disabledIt != disabledLabelPanels.end();
            ++disabledIt) {
            (*disabledIt)->SetVisible(0);
        }
        SetImageBorrowedAndInvalidate(defaultImage);
    } else {
        for (HudUiPanelPtrVector::iterator labelIt2 = labelPanels.begin(); labelIt2 != labelPanels.end(); ++labelIt2) {
            (*labelIt2)->SetVisible(0);
        }
        for (HudUiPanelPtrVector::iterator disabledIt2 = disabledLabelPanels.begin();
            disabledIt2 != disabledLabelPanels.end();
            ++disabledIt2) {
            (*disabledIt2)->SetVisible(1);
        }
        SetImageBorrowedAndInvalidate(disabledImage);
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-zrd-widget-hide-preview
 * @recoil-artifact defines .text recoil:function:0x4b5860: HudUiZrdWidget::HidePreview.
 * @recoil-match byte
 */
void HudUiZrdWidget::HidePreview()
{
    if (defaultImage != 0) {
        SetImageBorrowedAndInvalidate(defaultImage);
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle = 0;
    }

    for (HudUiPanelPtrVector::iterator rolloverIt = rolloverLabelPanels.begin();
        rolloverIt != rolloverLabelPanels.end();
        ++rolloverIt) {
        (*rolloverIt)->SetVisible(0);
    }
    for (HudUiPanelPtrVector::iterator activateIt = activateLabelPanels.begin();
        activateIt != activateLabelPanels.end();
        ++activateIt) {
        (*activateIt)->SetVisible(0);
    }
    for (HudUiPanelPtrVector::iterator labelIt = labelPanels.begin(); labelIt != labelPanels.end(); ++labelIt) {
        (*labelIt)->SetVisible(1);
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-zrd-widget-on-activate
 * @recoil-artifact defines .text recoil:function:0x4b5900: HudUiZrdWidget::OnActivate.
 * @recoil-match byte
 */
void HudUiZrdWidget::OnActivate()
{
    zInput::ResetAllTransitionState();

    if (activateImage != 0) {
        SetImageBorrowedAndInvalidate(activateImage);
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle->StopIfActive();
        rolloverPlayHandle = 0;
    }

    if (activateSound != 0) {
        activatePlayHandle = activateSound->PlayA3DSimple(activateSoundScale);
    }

    HudUiSetPanelVectorVisible(rolloverLabelPanels, 0);

    if (activateLabelPanels.begin() != 0) {
        for (HudUiPanelPtrVector::iterator activateIt = activateLabelPanels.begin();
            activateIt != activateLabelPanels.end();
            ++activateIt) {
            (*activateIt)->SetVisible(1);
        }
        HudUiSetPanelVectorVisible(labelPanels, 0);
        return;
    }

    HudUiSetPanelVectorVisible(labelPanels, 1);
}

/**
 * Purpose: attach the widget to its owning dialog and load its base ZRD
 * position, bitmap, sound, and label state.
 */
int HudUiZrdWidget::LoadFromZrd(zReader::Node* zrdSection, HudUiBackground* ownerDialog)
{
    owner = ownerDialog;
    SetVisible(1);
    ((HudUiContainer*)(ownerDialog))->AddChild(this);

    originX = ownerDialog->uiOriginX;
    originY = ownerDialog->uiOriginY;
    if (zrdSection == 0) {
        return 0;
    }

    zReader::Node* const positionNode = zRdrGetNode(zrdSection, "POSITION");
    if (positionNode != 0) {
        originX += positionNode->value.nodes[1].value.i32;
        originY += positionNode->value.nodes[2].value.i32;
    }

    int widgetX = originX;
    int widgetY = originY;
    zReader::Node* const bitmapNode = zRdrGetNode(zrdSection, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap);
    if (bitmapNode != 0) {
        const char* const bitmapPath = bitmapNode->value.nodes[1].value.str;
        {
            defaultImage = SetImageByPathOwned(bitmapPath);
            if (bitmapNode->value.nodes[0].value.i32 >= 4) {
                widgetX += bitmapNode->value.nodes[2].value.i32;
                widgetY += bitmapNode->value.nodes[3].value.i32;
            }
        }
    }

    SetPos(widgetX, widgetY);

    void* const clipSource = ownerDialog->capturedCompositeImage;
    if (clipSource != 0) {
        HudUiRect* const bounds = GetBoundsRectOrNull();
        if (bounds != 0) {
            // The clipping call receives a snapshot of the returned bounds.
            HudUiRect clipRect = *bounds;
            SetBltSourceAndClipRect(clipSource, &clipRect);
        }
    }

    zReader::Node* const rolloverNode = zRdrGetNode(zrdSection, g_HudZrd_Key_Rollover);
    if (rolloverNode != 0) {
        HUD_ZRD_LOAD_BITMAP(rolloverNode, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap, rolloverImage);
        HUD_ZRD_LOAD_SOUND(rolloverNode, rolloverSound, rolloverSoundScale);
        HUD_ZRD_LOAD_LABEL_SECTION(this, rolloverNode, rolloverLabelPanels);
        HUD_ZRD_APPLY_FLASH_SECTION(rolloverNode, rolloverLabelPanels);
    }

    zReader::Node* const disableNode = zRdrGetNode(zrdSection, g_HudZrd_Key_Disable);
    if (disableNode != 0) {
        HUD_ZRD_LOAD_BITMAP(disableNode, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap, disabledImage);
        HUD_ZRD_LOAD_SOUND(disableNode, disabledSound, disabledSoundScale);
        HUD_ZRD_LOAD_LABEL_SECTION(this, disableNode, disabledLabelPanels);
        HUD_ZRD_APPLY_FLASH_SECTION(disableNode, disabledLabelPanels);
    }

    zReader::Node* const activateNode = zRdrGetNode(zrdSection, g_HudZrd_Key_Activate);
    if (activateNode != 0) {
        HUD_ZRD_LOAD_BITMAP(activateNode, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap, activateImage);
        HUD_ZRD_LOAD_SOUND(activateNode, activateSound, activateSoundScale);
        HUD_ZRD_LOAD_LABEL_SECTION(this, activateNode, activateLabelPanels);
        HUD_ZRD_APPLY_FLASH_SECTION(activateNode, activateLabelPanels);
    }

    HUD_ZRD_LOAD_LABEL_SECTION(this, zrdSection, labelPanels);
    HUD_ZRD_APPLY_FLASH_SECTION(zrdSection, labelPanels);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-check-toggle-widget-hud-ui-check-toggle-widget
 * @recoil-artifact defines .text recoil:function:0x4b6fc0: HudUiCheckToggleWidget::HudUiCheckToggleWidget.
 * @recoil-match byte
 *
 * Function modeled here:
 * Purpose: initialize the toggle widget's unchecked, checked, label, and
 * disabled-state members over the ZRD widget base.
 */
HudUiCheckToggleWidget::HudUiCheckToggleWidget()
    : HudUiZrdWidget()
{
    checked = 0;
    uncheckedImage = 0;
    checkedImage = 0;
    checkedLabelPanel = 0;
    disabledCheckedImage = 0;
    disabledCheckedFallbackImage = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-huduichecktogglewidget
 * @recoil-artifact defines .text recoil:function:0x4b7020: HudUiCheckToggleWidget::~HudUiCheckToggleWidget.
 * @recoil-match byte
 *
 * Purpose: Restore the unchecked image, delete owned checked state, and tear down the ZRD widget base.
 */
HudUiCheckToggleWidget::~HudUiCheckToggleWidget()
{
    SetImageBorrowedAndInvalidate(uncheckedImage);

    if (checkedImage != 0) {
        ::operator delete(checkedImage);
        checkedImage = 0;
    }

    if (checkedLabelPanel != 0) {
        delete checkedLabelPanel;
        checkedLabelPanel = 0;
    }
}

/**
 * Provider boundary 0x40cf30: VC5 compiler/EH cleanup forwarding thunk.
 * Source compatibility wrapper for recovered callers that historically named
 * the destructor body DestructorCore. The physical row is not a standalone
 * authored body.
 * Purpose: Run the check-toggle destructor body.
 */
void HudUiCheckToggleWidget::DestructorCore()
{
    this->HudUiCheckToggleWidget::~HudUiCheckToggleWidget();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-getboundsrectornull
 * @recoil-artifact defines .text recoil:function:0x4b70b0: HudUiCheckToggleWidget::GetBoundsRectOrNull.
 * @recoil-match byte
 *
 * Purpose: return the recovered HUD value exposed by HudUiCheckToggleWidget::GetBoundsRectOrNull.
 */
HudUiRect* HudUiCheckToggleWidget::GetBoundsRectOrNull()
{
    return &boundsRect;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-refreshstate
 * @recoil-artifact defines .text recoil:function:0x4b70c0: HudUiCheckToggleWidget::RefreshState.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiCheckToggleWidget::RefreshState.
 */
void HudUiCheckToggleWidget::RefreshState()
{
    HudUiSetPanelVectorVisible(rolloverLabelPanels, 0);
    HudUiSetPanelVectorVisible(activateLabelPanels, 0);

    if (modeOrEnabled != 0) {
        HudUiSetPanelVectorVisible(labelPanels, 1);
        HudUiSetPanelVectorVisible(disabledLabelPanels, 0);

        if (checked != 0) {
            if (checkedImage != 0) {
                SetImageBorrowedAndInvalidate(checkedImage);
            } else if (uncheckedImage != 0) {
                SetImageBorrowedAndInvalidate(uncheckedImage);
            }
        }
    } else {
        HudUiSetPanelVectorVisible(labelPanels, 0);
        HudUiSetPanelVectorVisible(disabledLabelPanels, 1);

        if (checked != 0) {
            if (disabledCheckedImage != 0) {
                SetImageBorrowedAndInvalidate(disabledCheckedImage);
            } else if (disabledCheckedFallbackImage != 0) {
                SetImageBorrowedAndInvalidate(disabledCheckedFallbackImage);
            }
        }
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-showpreview
 * @recoil-artifact defines .text recoil:function:0x4b7210: HudUiCheckToggleWidget::ShowPreview.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiCheckToggleWidget::ShowPreview.
 */
void HudUiCheckToggleWidget::ShowPreview()
{
    if (modeOrEnabled == 0 || checked != 0) {
        return;
    }

    if (rolloverSound != 0) {
        rolloverPlayHandle = rolloverSound->PlayA3DSimple(rolloverSoundScale);
    }

    HudUiZrdWidget::ShowPreview();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-hidepreview
 * @recoil-artifact defines .text recoil:function:0x4b7250: HudUiCheckToggleWidget::HidePreview.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiCheckToggleWidget::HidePreview.
 */
void HudUiCheckToggleWidget::HidePreview()
{
    if (modeOrEnabled == 0 || checked != 0) {
        return;
    }

    if (rolloverPlayHandle != 0) {
        rolloverPlayHandle->StopIfActive();
        rolloverPlayHandle = 0;
    }

    HudUiZrdWidget::HidePreview();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-onactivate
 * @recoil-artifact defines .text recoil:function:0x4b7290: HudUiCheckToggleWidget::OnActivate.
 * @recoil-match byte
 *
 * Purpose: handle the recovered HUD event path for HudUiCheckToggleWidget::OnActivate.
 */
void HudUiCheckToggleWidget::OnActivate()
{
    if (modeOrEnabled == 0) {
        return;
    }

    SetChecked(checked == 0 ? 1 : 0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Purpose: handle the recovered HUD event path for HudUiCheckToggleWidget::OnActivateThunk.
 */
void HudUiCheckToggleWidget::OnActivateThunk()
{
    OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-setchecked
 * @recoil-artifact defines .text recoil:function:0x4b72c0: HudUiCheckToggleWidget::SetChecked.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by
 */
int HudUiCheckToggleWidget::SetChecked(int newChecked)
{
    const int previousChecked = checked;
    checked = newChecked;

    if (newChecked != 0) {
        if (checkedImage != 0) {
            SetImageBorrowedAndInvalidate(checkedImage);
        }

        if (checkedLabelPanel != 0) {
            checkedLabelPanel->SetVisible(1);
        }
    } else {
        if (uncheckedImage != 0) {
            SetImageBorrowedAndInvalidate(uncheckedImage);
        }

        if (checkedLabelPanel != 0) {
            checkedLabelPanel->SetVisible(0);
        }
    }

    Invalidate();
    return previousChecked;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduichecktogglewidget-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b7340: HudUiCheckToggleWidget::LoadFromZrd.
 *
 *
 * Purpose: load the base widget and its checked-state bitmap, label, and
 * disabled fallback resources from the owning ZRD section.
 */
int HudUiCheckToggleWidget::LoadFromZrd(zReader::Node* zrdSection, HudUiBackground* ownerDialog)
{
    HudUiZrdWidget::LoadFromZrd(zrdSection, ownerDialog);
    uncheckedImage = image;

    zReader::Node* const checkedNode = zRdrGetNode(zrdSection, g_HudUiZrdKey_Checked);
    if (checkedNode != 0) {
        zReader::Node* const bitmapNode = zRdrGetNode(checkedNode, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap);
        if (bitmapNode != 0) {
            checkedImage = zImage::TexDirFindOrCreateByPath(bitmapNode->value.nodes[1].value.str);
        }
        zReader::Node* const textNode = zRdrGetNode(checkedNode, g_HudUiCycleSelectorWidget_ZrdKey_Text);
        if (textNode != 0) {
            const int textOriginX = originX;
            const int textOriginY = originY;
            checkedLabelPanel = new HudUiTransitionTextPanel;
            checkedLabelPanel->SetTextFmt(zLoc::ResolveMessageKeyOrFallback(textNode->value.nodes[1].value.str));
            checkedLabelPanel->SetPos(
                textOriginX + textNode->value.nodes[2].value.i32,
                textOriginY + textNode->value.nodes[3].value.i32
            );
            const int styleIndex = textNode->value.nodes[4].value.i32;
            const HudFontStyle* style
                = owner->fontStyles[styleIndex].validMarker != 0 ? &owner->fontStyles[styleIndex] : 0;
            if (style != 0) {
                checkedLabelPanel->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
                checkedLabelPanel->SetTextColorsAndMarkDirty(style->textColor, style->textColor);
                checkedLabelPanel->SetShadow(style->shadowEnabled, 1, 1);
            }
            checkedLabelPanel->SetVisible(0);
            ((HudUiContainer*)owner)->AddChild(checkedLabelPanel);
        }
    }

    zReader::Node* const disabledUnselectedNode = zRdrGetNode(zrdSection, g_HudUiZrdKey_DisableUnsel);
    if (disabledUnselectedNode != 0) {
        zReader::Node* const bitmapNode = zRdrGetNode(disabledUnselectedNode, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap);
        if (bitmapNode != 0) {
            disabledCheckedFallbackImage = zImage::TexDirFindOrCreateByPath(bitmapNode->value.nodes[1].value.str);
        }
        zReader::Node* const textNode = zRdrGetNode(disabledUnselectedNode, g_HudUiCycleSelectorWidget_ZrdKey_Text);
        if (textNode != 0) {
            const int textOriginX = originX;
            const int textOriginY = originY;
            checkedLabelPanel = new HudUiTransitionTextPanel;
            checkedLabelPanel->SetTextFmt(zLoc::ResolveMessageKeyOrFallback(textNode->value.nodes[1].value.str));
            checkedLabelPanel->SetPos(
                textOriginX + textNode->value.nodes[2].value.i32,
                textOriginY + textNode->value.nodes[3].value.i32
            );
            const int styleIndex = textNode->value.nodes[4].value.i32;
            const HudFontStyle* style
                = owner->fontStyles[styleIndex].validMarker != 0 ? &owner->fontStyles[styleIndex] : 0;
            if (style != 0) {
                checkedLabelPanel->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
                checkedLabelPanel->SetTextColorsAndMarkDirty(style->textColor, style->textColor);
                checkedLabelPanel->SetShadow(style->shadowEnabled, 1, 1);
            }
            checkedLabelPanel->SetVisible(0);
            ((HudUiContainer*)owner)->AddChild(checkedLabelPanel);
        }

        HUD_ZRD_LOAD_LABEL_SECTION(this, zrdSection, disabledLabelPanels);
    }

    zReader::Node* const disabledSelectedNode = zRdrGetNode(zrdSection, g_HudUiZrdKey_DisableSel);
    if (disabledSelectedNode != 0) {
        zReader::Node* const bitmapNode = zRdrGetNode(disabledSelectedNode, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap);
        if (bitmapNode != 0) {
            disabledCheckedImage = zImage::TexDirFindOrCreateByPath(bitmapNode->value.nodes[1].value.str);
        }
        zReader::Node* const textNode = zRdrGetNode(disabledSelectedNode, g_HudUiCycleSelectorWidget_ZrdKey_Text);
        if (textNode != 0) {
            const int textOriginX = originX;
            const int textOriginY = originY;
            checkedLabelPanel = new HudUiTransitionTextPanel;
            checkedLabelPanel->SetTextFmt(zLoc::ResolveMessageKeyOrFallback(textNode->value.nodes[1].value.str));
            checkedLabelPanel->SetPos(
                textOriginX + textNode->value.nodes[2].value.i32,
                textOriginY + textNode->value.nodes[3].value.i32
            );
            const int styleIndex = textNode->value.nodes[4].value.i32;
            const HudFontStyle* style
                = owner->fontStyles[styleIndex].validMarker != 0 ? &owner->fontStyles[styleIndex] : 0;
            if (style != 0) {
                checkedLabelPanel->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
                checkedLabelPanel->SetTextColorsAndMarkDirty(style->textColor, style->textColor);
                checkedLabelPanel->SetShadow(style->shadowEnabled, 1, 1);
            }
            checkedLabelPanel->SetVisible(0);
            ((HudUiContainer*)owner)->AddChild(checkedLabelPanel);
        }
    }

    if (uncheckedImage != 0) {
        boundsRect.top = y;
        boundsRect.left = x;
        boundsRect.bottom = y + uncheckedImage->height;
        boundsRect.right = x + uncheckedImage->width;
    } else if (labelPanels.begin() != labelPanels.end()) {
        HudUiPanelPtrVector::iterator panelIt = labelPanels.begin();
        HudUiPanel* const firstPanel = *panelIt;
        boundsRect.top = firstPanel->GetCenterY();
        boundsRect.left = firstPanel->GetCenterX();
        boundsRect.bottom = firstPanel->QueryTextHeight() + boundsRect.top;

        while (panelIt != labelPanels.end()) {
            HudUiPanel* const panel = *panelIt;
            boundsRect.bottom += panel->QueryTextHeight();

            if (panel->QueryTextWidth() + boundsRect.left > boundsRect.right) {
                boundsRect.right = panel->QueryTextWidth() + boundsRect.left;
            }

            ++panelIt;
        }

        boundsRect.bottom -= firstPanel->QueryTextHeight();
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.hud-ui-cycle-selector-widget-hud-ui-cycle-selector-widget
 * @recoil-artifact defines .text recoil:function:0x4b7d60: HudUiCycleSelectorWidget::HudUiCycleSelectorWidget.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiCycleSelectorWidget::HudUiCycleSelectorWidget.
 */
HudUiCycleSelectorWidget::HudUiCycleSelectorWidget()
    : HudUiZrdWidget()
{
    selectedIndex = 0;
    itemCount = 0;
    for (int i = 0; i < 20; ++i) {
        entriesA[i] = 0;
        entriesB[i] = 0;
    }

    firstIndex = 0;
    visibleCount = 20;
    fontStyleRef = 0;
    textOffsetY = 0;
    textOffsetX = 0;
}

/**
 * Purpose: initialize the recovered HudUiCycleSelectorWidget::Constructor state.
 */
HudUiCycleSelectorWidget* HudUiCycleSelectorWidget::Constructor()
{
    new (this) HudUiCycleSelectorWidget;
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-huduicycleselectorwidget
 * @recoil-artifact defines .text recoil:function:0x4b7de0: HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget.
 * @recoil-match byte
 *
 * Purpose: Delete paired selector entry widgets and tear down the ZRD widget base.
 */
HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget()
{
    for (int i = 0; i < 20; ++i) {
        if (entriesA[i] != 0) {
            delete entriesA[i];
            entriesA[i] = 0;
        }

        if (entriesB[i] != 0) {
            delete entriesB[i];
            entriesB[i] = 0;
        }
    }
}

/**
 * Provider boundary 0x40cf40: VC5 compiler/EH cleanup forwarding thunk.
 * Source compatibility wrapper for recovered callers that historically named
 * the destructor body DestructorCore. The physical row is not a standalone
 * authored body.
 * Purpose: Run the cycle-selector destructor body.
 */
void HudUiCycleSelectorWidget::DestructorCore()
{
    this->HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-update
 * @recoil-artifact defines .text recoil:function:0x4b7e60: HudUiCycleSelectorWidget::Update.
 * @recoil-match byte
 *
 * Purpose: advance the recovered HUD update path for HudUiCycleSelectorWidget::Update.
 */
void HudUiCycleSelectorWidget::Update(float deltaSeconds)
{
    for (int i = 0; i < itemCount; ++i) {
        Invalidate();

        if (entriesA[i] != 0) {
            if (i == selectedIndex) {
                entriesA[i]->SetVisible(1);
            } else {
                entriesA[i]->SetVisible(0);
            }
        }

        if (entriesB[i] != 0) {
            if (i == selectedIndex) {
                entriesB[i]->SetVisible(1);
            } else {
                entriesB[i]->SetVisible(0);
            }
        }
    }

    HudUiElement::Update(deltaSeconds);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-advanceselectionandactivate
 * @recoil-artifact defines .text recoil:function:0x4b7ee0: HudUiCycleSelectorWidget::AdvanceSelectionAndActivate.
 * @recoil-match byte
 *
 * Purpose: Advance the selected cycle entry, wrap at the visible/item limit,
 * and run the base ZRD activation path.
 */
void HudUiCycleSelectorWidget::AdvanceSelectionAndActivate()
{
    ++selectedIndex;
    if (selectedIndex >= (visibleCount < itemCount ? visibleCount : itemCount)) {
        selectedIndex = firstIndex;
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-setindexclamped
 * @recoil-artifact defines .text recoil:function:0x4b7f20: HudUiCycleSelectorWidget::SetIndexClamped.
 * @recoil-match byte
 *
 * Purpose: clamp a requested cycle-selector index and return the previous
 * selected index.
 *
 * Evidence: BN assembly at 0x4b7f20 loads selectedIndex into eax before the
 * clamp branches, compares the requested index against firstIndex, itemCount,
 * and visibleCount, writes selectedIndex to the clamped value, and returns with
 * eax preserved.
 */
int HudUiCycleSelectorWidget::SetIndexClamped(int index)
{
    const int previousIndex = selectedIndex;

    if (index < firstIndex) {
        selectedIndex = firstIndex;
        return previousIndex;
    }

    if (index >= itemCount) {
        selectedIndex = itemCount - 1;
        return previousIndex;
    }

    if (index >= visibleCount) {
        selectedIndex = visibleCount - 1;
        return previousIndex;
    }

    selectedIndex = index;
    return previousIndex;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-setvisiblerange
 * @recoil-artifact defines .text recoil:function:0x4b7f80: HudUiCycleSelectorWidget::SetVisibleRange.
 * @recoil-match byte
 *
 * Purpose: set the visible selector range and clamp the selected entry into it.
 */
void HudUiCycleSelectorWidget::SetVisibleRange(int first, int last)
{
    if (first >= 0 && first < itemCount) {
        firstIndex = first;
    }

    if (last >= first && last < itemCount) {
        visibleCount = last;
    }

    if (selectedIndex < first) {
        selectedIndex = first;
    }

    if (selectedIndex >= last) {
        selectedIndex = last - 1;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-addtextentry
 * @recoil-artifact defines .text recoil:function:0x4b7fd0: HudUiCycleSelectorWidget::AddTextEntry.
 * @recoil-match byte
 *
 * Purpose: create a hidden transition text-panel entry, position it with the
 * selector text offset, and attach it to the owning HUD background container.
 *
 * Evidence: BN assembly at 0x4b7fd0 grows itemCount/visibleCount, allocates a
 * 0x2c0 HudUiTransitionTextPanel, stores it in entriesA[index], dispatches
 * SetTextFmt/SetPos/SetVisible, and adds it through the owner container.
 */
void HudUiCycleSelectorWidget::AddTextEntry(int index, const char* text, int posX, int posY)
{
    if (index >= itemCount) {
        int newCount = index + 1;
        if (newCount >= 20) {
            newCount = 20;
        }

        itemCount = newCount;
        if (newCount > visibleCount) {
            visibleCount = newCount;
        }
    }

    if (index > visibleCount) {
        return;
    }

    HudUiTransitionTextPanel* const transitionPanel = new HudUiTransitionTextPanel;

    entriesA[index] = (HudUiWidget*)(transitionPanel);
    ((HudUiPanel*)(entriesA[index]))->SetTextFmt(text);

    entriesA[index]->SetPos(textOffsetX + posX, textOffsetY + posY);
    entriesA[index]->SetVisible(0);
    ((HudUiContainer*)(owner))->AddChild(entriesA[index]);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-applyfontstyleforentry
 * @recoil-artifact defines .text recoil:function:0x4b8100: HudUiCycleSelectorWidget::ApplyFontStyleForEntry.
 * @recoil-match byte
 *
 * Purpose: grow the selector entry range when needed, validate the owning
 * background font style, and copy that style onto the text-panel entry.
 *
 * Evidence: BN assembly at 0x4b8100 selects owner->fontStyles[styleIndex] at
 * HudUiBackground offset 0x1cec, masks the style pointer to null when
 * validMarker is clear, calls the entry SetFont slot, and copies text color,
 * shadow, alignment, background mode, and background color fields.
 */
void HudUiCycleSelectorWidget::ApplyFontStyleForEntry(int index, int styleIndex)
{
    if (index >= itemCount) {
        int newCount = index + 1;
        if (newCount >= 20) {
            newCount = 20;
        }

        itemCount = newCount;
        if (newCount > visibleCount) {
            visibleCount = newCount;
        }
    }

    if (index > visibleCount) {
        return;
    }

    const HudFontStyle* const style
        = owner->fontStyles[styleIndex].validMarker != 0 ? &owner->fontStyles[styleIndex] : 0;
    if (style == 0) {
        return;
    }

    HudUiPanel* panel = (HudUiPanel*)(entriesA[index]);
    panel->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);

    panel = (HudUiPanel*)(entriesA[index]);
    const unsigned int textColor = style->textColor;
    panel->textColor0 = textColor;
    panel->textColor1 = textColor;
    panel->textDirty = 1;

    panel = (HudUiPanel*)(entriesA[index]);
    panel->shadowEnabled = style->shadowEnabled;
    panel->shadowOffsetX = 1;
    panel->shadowOffsetY = 1;

    panel = (HudUiPanel*)(entriesA[index]);
    panel->alignMode = style->alignMode;

    panel = (HudUiPanel*)(entriesA[index]);
    const unsigned int backgroundColor = style->bkColor;
    const int backgroundMode = style->bkMode;
    panel->bkMode = backgroundMode;
    panel->bkColor = backgroundColor;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-addbitmapentry
 * @recoil-artifact defines .text recoil:function:0x4b8200: HudUiCycleSelectorWidget::AddBitmapEntry.
 * @recoil-match byte
 *
 * Purpose: construct a bitmap entry, load its image, then position and attach
 * the entry reloaded from the selector array after each callback.
 */
void HudUiCycleSelectorWidget::AddBitmapEntry(int index, const char* imagePath, int posX, int posY)
{
    if (index > itemCount) {
        int newCount = index + 1;
        if (newCount >= 20) {
            newCount = 20;
        }

        itemCount = newCount;
        if (newCount > visibleCount) {
            visibleCount = newCount;
        }
    }

    if (index > visibleCount) {
        return;
    }

    HudUiWidget* const bitmapWidget = new HudUiWidget(0);
    entriesB[index] = bitmapWidget;
    bitmapWidget->SetImageByPathOwned(imagePath);
    entriesB[index]->SetPos(posX, posY);
    entriesB[index]->SetVisible(0);
    ((HudUiContainer*)(owner))->AddChild(entriesB[index]);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicycleselectorwidget-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b82e0: HudUiCycleSelectorWidget::LoadFromZrd.
 * @recoil-match byte
 *
 * Purpose: load the recovered HUD data handled by HudUiCycleSelectorWidget::LoadFromZrd.
 */
int HudUiCycleSelectorWidget::LoadFromZrd(zReader::Node* zrdSection, HudUiBackground* ownerDialog)
{
    HudUiZrdWidget::LoadFromZrd(zrdSection, ownerDialog);

    zReader::Node* const fontNode = zRdrGetNode(zrdSection, g_HudUiCycleSelectorWidget_ZrdKey_Font);
    if (fontNode != 0) {
        fontStyleRef = (void*)((unsigned int)(fontNode->value.u32));
    }

    zReader::Node* const textOffsetNode = zRdrGetNode(zrdSection, g_HudUiCycleSelectorWidget_ZrdKey_TextOffset);
    if (textOffsetNode != 0) {
        textOffsetX = textOffsetNode->value.nodes[1].value.i32;
        textOffsetY = textOffsetNode->value.nodes[2].value.i32;
    }

    zReader::Node* const cycleNode = zRdrGetNode(zrdSection, g_HudUiCycleSelectorWidget_ZrdKey_Cycle);
    if (cycleNode == 0) {
        return 1;
    }

    int count = cycleNode->value.nodes[0].value.i32 - 1;
    if (count >= 20) {
        count = 20;
    }

    itemCount = count;
    if (count > visibleCount) {
        visibleCount = count;
    }

    for (int index = 0; index < itemCount; ++index) {
        // Unsigned slot index: VC5 then forms the entry address as scaled index + array base, as in retail.
        zReader::Node* const entryNode = &cycleNode->value.nodes[(unsigned int)(index + 1)];

        zReader::Node* const textNode = zRdrGetNode(entryNode, g_HudUiCycleSelectorWidget_ZrdKey_Text);
        if (textNode != 0) {
            AddTextEntry(
                index,
                zLoc::ResolveMessageKeyOrFallback(textNode->value.nodes[1].value.str),
                originX + textNode->value.nodes[2].value.i32,
                originY + textNode->value.nodes[3].value.i32
            );
            ApplyFontStyleForEntry(index, textNode->value.nodes[4].value.i32);
        }

        zReader::Node* const bitmapNode = zRdrGetNode(entryNode, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap);
        if (bitmapNode != 0) {
            zReader::Node* const bitmapItems = bitmapNode->value.nodes;
            int bitmapX = originX;
            int bitmapY = originY;
            if (bitmapItems[0].value.i32 >= 4) {
                bitmapX += bitmapItems[2].value.i32;
                bitmapY += bitmapItems[3].value.i32;
            }

            AddBitmapEntry(index, bitmapItems[1].value.str, bitmapX, bitmapY);
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-huduifillbitmap-0x4b8450
 * @recoil-artifact defines .text recoil:function:0x4b8450: HudUiFillBitmap::HudUiFillBitmap.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiFillBitmap::HudUiFillBitmap.
 */
HudUiFillBitmap::HudUiFillBitmap()
    : HudUiZrdWidget()
{
    normalizedValue = 0.0f;
    previewImage = 0;
    fillImage = 0;
    previewRect.right = 0;
    previewRect.left = 0;
    previewRect.bottom = 0;
    previewRect.top = 0;
    fillRect.right = 0;
    fillRect.left = 0;
    fillRect.bottom = 0;
    fillRect.top = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-huduifillbitmap-0x4b84d0
 * @recoil-artifact defines .text recoil:function:0x4b84d0: HudUiFillBitmap::~HudUiFillBitmap.
 * @recoil-match byte
 *
 * Purpose: Release distinct preview/fill images and tear down the ZRD widget base.
 */
HudUiFillBitmap::~HudUiFillBitmap()
{
    if (previewImage != 0 && previewImage != image) {
        previewImage = zVid_Image::ReleaseIfNotDefault(previewImage);
    }

    if (fillImage != 0 && fillImage != image) {
        fillImage = zVid_Image::ReleaseIfNotDefault(fillImage);
    }
}

/**
 * Provider boundary 0x40cf50: VC5 compiler/EH cleanup forwarding thunk.
 * Source compatibility wrapper for recovered callers that historically named
 * the destructor body DestructorCore. The physical row is not a standalone
 * authored body.
 * Purpose: Run the fill-bitmap destructor body.
 */
void HudUiFillBitmap::DestructorCore()
{
    this->HudUiFillBitmap::~HudUiFillBitmap();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-draw
 * @recoil-artifact defines .text recoil:function:0x4b8520: HudUiFillBitmap::Draw.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiFillBitmap::Draw.
 */
void HudUiFillBitmap::Draw()
{
    if (previewImage == 0 || fillImage == 0) {
        return;
    }

    DrawBase();

    if (fillRect.left != fillRect.right) {
        zVid_Image::BlitToActiveTarget(fillImage, x + fillOffsetX, y + fillOffsetY, 0, (zVidRect32*)(&fillRect));
    }

    if (previewRect.left != previewRect.right) {
        zVid_Image::BlitToActiveTarget(
            previewImage,
            x + previewOffsetX,
            y + previewOffsetY,
            0,
            (zVidRect32*)(&previewRect)
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b85c0: HudUiFillBitmap::LoadFromZrd.
 * @recoil-match byte
 *
 * Purpose: load the recovered HUD data handled by HudUiFillBitmap::LoadFromZrd.
 */
int HudUiFillBitmap::LoadFromZrd(zReader::Node* zrdSection, HudUiBackground* ownerDialog)
{
    HudUiZrdWidget::LoadFromZrd(zrdSection, ownerDialog);

    zReader::Node* const fillBitmapNode = zRdrGetNode(zrdSection, g_HudUiFillBitmap_ZrdKey_FillBitmap);
    if (fillBitmapNode != 0) {
        int posX = originX;
        int posY = originY;
        fillImage = zImage::TexDirFindOrCreateByPath(fillBitmapNode->value.nodes[1].value.str);
        Invalidate();

        if (fillBitmapNode->value.nodes[0].value.i32 >= 4) {
            posX += fillBitmapNode->value.nodes[2].value.i32;
            posY += fillBitmapNode->value.nodes[3].value.i32;
        }

        SetPos(posX, posY);
        previewImage = image;
        Invalidate();
    }

    SetNormalizedValueAndRebuild(0.0f);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-updatenormalizedfromcursor
 * @recoil-artifact defines .text recoil:function:0x4b8650: HudUiFillBitmap::UpdateNormalizedFromCursor.
 * @recoil-match byte
 *
 * Purpose: update the normalized fill value from the owner cursor and activate the widget.
 */
void HudUiFillBitmapSlider::OnActivate()
{
    const zInput::MouseStateSnapshot* const mouse = &owner->mouseState;
    const int relativeX = mouse->cursorClientX - GetCenterX();
    const int imageWidth = image != 0 ? image->width : 0;
    SetNormalizedValueAndRebuild((float)(relativeX) / (float)(imageWidth));
    // Retail emits a direct base call here. Virtual redispatch would re-enter
    // the sound/music option override and recurse until stack exhaustion.
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-setnormalizedvalueandrebuild
 * @recoil-artifact defines .text recoil:function:0x4b86b0: HudUiFillBitmap::SetNormalizedValueAndRebuild.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUiFillBitmap::SetNormalizedValueAndRebuild.
 */
void HudUiFillBitmapSlider::SetNormalizedValueAndRebuild(float value)
{
    if (fillImage == 0) {
        return;
    }

    normalizedValue = value;
    Invalidate();

    fillRect.top = 0;
    fillRect.bottom = fillImage->height;
    fillRect.left = 0;
    const int filledWidth = (int)(fillImage->width * value);
    fillRect.right = filledWidth;
    fillOffsetX = 0;
    fillOffsetY = 0;

    if (previewImage == 0) {
        return;
    }

    previewRect.top = 0;
    previewRect.bottom = fillImage->height;
    previewRect.left = filledWidth;
    previewRect.right = previewImage->width;
    previewOffsetX = filledWidth;
    previewOffsetY = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-item-huduizrdwidgetex17c-item
 * @recoil-artifact defines .text recoil:function:0x4b8760: CHudRadioButtonWidget::CHudRadioButtonWidget.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for CHudRadioButtonWidget::CHudRadioButtonWidget.
 */
CHudRadioButtonWidget::CHudRadioButtonWidget()
    : HudUiZrdWidget()
{
    selected = 0;
    selectedImage = 0;
    unselectedImage = 0;
    ownerSelector = 0;
    mouseRectValid = 0;
}

/**
 * Purpose: run the recovered CHudRadioButtonWidget::DestructorCore teardown path.
 */
CHudRadioButtonWidget* CHudRadioButtonWidget::Constructor()
{
    new (this) CHudRadioButtonWidget;
    return this;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.chud-radio-button-widget-destroy-chud-radio-button-widget
 * @recoil-artifact defines .text recoil:function:0x4b87c0: CHudRadioButtonWidget::~CHudRadioButtonWidget.
 * @recoil-match byte
 *
 * Purpose: run the recovered CHudRadioButtonWidget::DestructorCore teardown path.
 */
CHudRadioButtonWidget::~CHudRadioButtonWidget() { }

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-item-showpreviewifnotselected
 * @recoil-artifact defines .text recoil:function:0x4b87d0: CHudRadioButtonWidget::ShowPreviewIfNotSelected.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for CHudRadioButtonWidget::ShowPreviewIfNotSelected.
 */
void CHudRadioButtonWidget::ShowPreviewIfNotSelected()
{
    if (selected == 0) {
        HudUiZrdWidget::ShowPreview();
    }
}

/**
 * Purpose: preserve the recovered HUD behavior for CHudRadioButtonWidget::HidePreviewIfNotSelected.
 */
void CHudRadioButtonWidget::ShowPreview()
{
    ShowPreviewIfNotSelected();
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.chud-radio-button-widget-hide-preview-if-not-selected
 * @recoil-artifact defines .text recoil:function:0x4b87e0: CHudRadioButtonWidget::HidePreviewIfNotSelected.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for CHudRadioButtonWidget::HidePreviewIfNotSelected.
 */
void CHudRadioButtonWidget::HidePreviewIfNotSelected()
{
    if (selected == 0) {
        HudUiZrdWidget::HidePreview();
    }
}

/**
 * Purpose: handle the recovered HUD event path for CHudRadioButtonWidget::OnActivateSelectSelf.
 */
void CHudRadioButtonWidget::HidePreview()
{
    HidePreviewIfNotSelected();
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.chud-radio-button-widget-on-activate-select-self
 * @recoil-artifact defines .text recoil:function:0x4b87f0: CHudRadioButtonWidget::OnActivateSelectSelf.
 * @recoil-match byte
 *
 * Purpose: handle the recovered HUD event path for CHudRadioButtonWidget::OnActivateSelectSelf.
 */
void CHudRadioButtonWidget::OnActivateSelectSelf()
{
    ownerSelector->SetSelectedIndex(itemIndex);
    ownerSelector->OnActivate();
    HudUiZrdWidget::OnActivate();

    {
        for (int index = 0; index < ownerSelector->optionCount; ++index) {
            CHudRadioButtonWidget* const option = ownerSelector->options[index];
            option->HidePreview();
        }
    }
}

/**
 * Purpose: load the recovered HUD data handled by CHudRadioButtonWidget::LoadFromZrd.
 */
void CHudRadioButtonWidget::OnActivate()
{
    OnActivateSelectSelf();
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.chud-radio-button-widget-load-from-zrd
 * @recoil-artifact defines .text recoil:function:0x4b8850: CHudRadioButtonWidget::LoadFromZrd.
 *
 *
 * Purpose: load the recovered HUD data handled by CHudRadioButtonWidget::LoadFromZrd.
 */
int CHudRadioButtonWidget::LoadFromZrd(zReader::Node* zrdSection, HudUiBackground* ownerDialog)
{
    HudUiZrdWidget::LoadFromZrd(zrdSection, ownerDialog);

    unselectedImage = image;
    unselectedRolloverImage = rolloverImage;
    selectedImage = activateImage;
    selectedRolloverImage = activateImage;

    boundsRect.top = GetCenterY();
    boundsRect.left = GetCenterX();
    boundsRect.bottom = boundsRect.top + (image != 0 ? image->width : 0);
    boundsRect.right = (image != 0 ? image->height : 0) + boundsRect.left;

    if (unselectedImage != 0) {
        boundsRect.top = y;
        boundsRect.left = x;
        boundsRect.bottom = unselectedImage->height + y;
        boundsRect.right = unselectedImage->width + x;
    } else if (labelPanels.begin() != 0) {
        boundsRect.top = labelPanels[0]->GetCenterY();
        boundsRect.left = labelPanels[0]->GetCenterX();
        boundsRect.bottom = labelPanels[0]->QueryTextHeight() + boundsRect.top;

        for (HudUiPanelPtrVector::iterator panelIt = labelPanels.begin(); panelIt != labelPanels.end(); ++panelIt) {
            boundsRect.bottom += (*panelIt)->QueryTextHeight();
            boundsRect.right = (*panelIt)->QueryTextWidth() + boundsRect.left > boundsRect.right
                ? (*panelIt)->QueryTextWidth() + boundsRect.left
                : boundsRect.right;
        }

        boundsRect.bottom -= labelPanels[0]->QueryTextHeight();
    }

    mouseRectValid = 1;
    mouseRect = boundsRect;

    zReader::Node* const mouseRectNode = zRdrGetNode(zrdSection, g_HudUiZrdWidgetEx17C_Item_ZrdKey_MouseRect);
    if (mouseRectNode != 0) {
        mouseRect.top += mouseRectNode->value.nodes[1].value.i32;
        mouseRect.left += mouseRectNode->value.nodes[2].value.i32;
        mouseRect.bottom = mouseRectNode->value.nodes[3].value.i32 + mouseRect.top;
        mouseRect.right = mouseRectNode->value.nodes[4].value.i32 + mouseRect.left;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-item-setselected
 * @recoil-artifact defines .text recoil:function:0x4b8a90: CHudRadioButtonWidget::SetSelected
 * @recoil-match byte
 *
 * Source file evidence: BN labels the source as HudUiZrdWidgetEx17C_Item.cpp.
 * Purpose: Record the option-item selected state and refresh the displayed image pair when enabled.
 */
void CHudRadioButtonWidget::SetSelected(int selectedValue)
{
    selected = selectedValue;
    if (modeOrEnabled == 0) {
        return;
    }

    if (selectedValue != 0) {
        defaultImage = selectedImage;
        rolloverImage = selectedRolloverImage;
    } else {
        defaultImage = unselectedImage;
        rolloverImage = unselectedRolloverImage;
    }

    SetImageBorrowedAndInvalidate(defaultImage);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-item-getmouserectorbounds
 * @recoil-artifact defines .text recoil:function:0x4b8af0: CHudRadioButtonWidget::GetMouseRectOrBounds.
 * @recoil-match byte
 *
 * Purpose: return the recovered HUD value exposed by CHudRadioButtonWidget::GetMouseRectOrBounds.
 */
HudUiRect* CHudRadioButtonWidget::GetBoundsRectOrNull()
{
    if (mouseRectValid != 0) {
        return &mouseRect;
    }
    return HudUiZrdWidget::GetBoundsRectOrNull();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-huduizrdwidgetex17c
 * @recoil-artifact defines .text recoil:function:0x4b8b10: CHudRadioGroupWidget::CHudRadioGroupWidget.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for CHudRadioGroupWidget::CHudRadioGroupWidget.
 */
CHudRadioGroupWidget::CHudRadioGroupWidget()
    : HudUiZrdWidget()
{
    optionCount = 0;

    {
        int optionIndex;
        for (optionIndex = 0; optionIndex < 10; ++optionIndex) {
            options[optionIndex] = 0;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.chud-radio-group-widget-destroy-chud-radio-group-widget
 * @recoil-artifact defines .text recoil:function:0x4b8b60: CHudRadioGroupWidget::~CHudRadioGroupWidget.
 * @recoil-match byte
 *
 * Source file evidence: BN labels the source as HudUiZrdWidgetEx17C.cpp.
 * Purpose: Delete owned option-selector items and clear their slots before compiler-generated base cleanup.
 */
CHudRadioGroupWidget::~CHudRadioGroupWidget()
{

    {
        int optionIndex;
        for (optionIndex = 0; optionIndex < 10; ++optionIndex) {
            CHudRadioButtonWidget* option = options[optionIndex];
            if (option != 0) {
                delete option;
                options[optionIndex] = 0;
            }
        }
    }
}

/**
 * Purpose: run the recovered CHudRadioGroupWidget destructor through the compatibility name.
 */
CHudRadioGroupWidget* CHudRadioGroupWidget::Constructor()
{
    new (this) CHudRadioGroupWidget;
    return this;
}

/**
 * CHudRadioGroupWidget::DestructorCore compatibility wrapper.
 * No standalone retail function; source compatibility wrapper for recovered
 * callers that historically named the destructor body DestructorCore in this
 * reconstruction.
 * Purpose: Run the option-selector destructor body.
 */
void CHudRadioGroupWidget::DestructorCore()
{
    this->CHudRadioGroupWidget::~CHudRadioGroupWidget();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduizrdwidgetex17c-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b8be0: CHudRadioGroupWidget::LoadFromZrd.
 * @recoil-match byte
 *
 * Purpose: load the recovered HUD data handled by CHudRadioGroupWidget::LoadFromZrd.
 */
int CHudRadioGroupWidget::LoadFromZrd(zReader::Node* zrdSection, HudUiBackground* ownerDialog)
{
    owner = ownerDialog;

    zReader::Node* const radioNode = zRdrGetNode(zrdSection, g_HudUiZrdToken_Radio);
    if (radioNode != 0) {
        int count = radioNode->value.nodes[0].value.i32 - 1;
        if (count >= 10) {
            count = 10;
        }

        optionCount = count;
        for (int index = 0; index < optionCount; ++index) {
            zReader::Node* const entryNode = &radioNode->value.nodes[index + 1];
            options[index] = new CHudRadioButtonWidget;
            options[index]->LoadFromZrd(entryNode, ownerDialog);
            CHudRadioButtonWidget* const option = options[index];
            option->ownerSelector = this;
            option->itemIndex = index;
        }
    }

    SetSelectedIndex(0);
    return 1;
}

/**
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: apply the recovered HUD state change handled by CHudRadioGroupWidget::SetSelectedIndex.
 */
void CHudRadioGroupWidget::SetVisible(int childIndex)
{
    EnableChildAtIndex(childIndex);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-widgets.chud-radio-group-widget-set-selected-index
 * @recoil-artifact defines .text recoil:function:0x4b8cf0: CHudRadioGroupWidget::SetSelectedIndex.
 * @recoil-match byte
 *
 * Source file evidence: BN labels the source as HudUiZrdWidgetEx17C.cpp.
 * Purpose: Store the selected option index and update every loaded option item's selected state.
 */
int CHudRadioGroupWidget::SetSelectedIndex(int index)
{
    selectedIndex = index;
    {
        for (int optionIndex = 0; optionIndex < 10; ++optionIndex) {
            CHudRadioButtonWidget* const option = options[optionIndex];
            if (option != 0) {
                option->SetSelected(optionIndex == index ? 1 : 0);
            }
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-hudcmdbindbuttonbase
 * @recoil-artifact defines .text recoil:function:0x4b8d30: HudCmdBindButtonBase::HudCmdBindButtonBase.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudCmdBindButtonBase::HudCmdBindButtonBase.
 */
HudCmdBindButtonBase::HudCmdBindButtonBase()
    : HudUiCheckToggleWidget()
{
    bindingSlotTotalCount = 0;
    bindingSlotPanels = 0;
    visibleListOffsetX = 0.0f;
    visibleListOffsetY = 0.0f;
    overflowListOffsetX = 0.0f;
    overflowListOffsetY = 0.0f;
    bindingSlotSpacing = 0xf;
    selectedBindingIndex = -1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b8de0: HudCmdBindButtonBase::LoadFromZrd.
 *
 *
 * Purpose: load binding button fonts, spacing, offsets, slot counts, and
 * child panel setup from a ZRD node.
 * Touched data: uses accepted ZRD-key literal owner
 * hud_ui.hudcmd_bind_button_base_zrd_key_literals.
 */
int HudCmdBindButtonBase::LoadFromZrd(zReader::Node* zrdSection, HudUiBackground* ownerDialog)
{
    HudUiRect clipRect;
    clipRect.left = clipRect.top = 0;
    clipRect.right = clipRect.bottom = 0;
    HudUiCheckToggleWidget::LoadFromZrd(zrdSection, ownerDialog);

    void* const clipSource = ownerDialog->capturedCompositeImage;

    zReader::Node* const selectedFontNode = zRdrGetNode(zrdSection, "SELECTED_FONT");
    if (selectedFontNode != 0) {
        selectedFontStyleRef = selectedFontNode->value.i32;
        // Retail tests the style pointer after the inline select (neg/sbb/and then a compare).
        const HudFontStyle* const selectedStyle
            = owner->fontStyles[selectedFontStyleRef].validMarker != 0 ? &owner->fontStyles[selectedFontStyleRef] : 0;
        if (selectedStyle != 0) {
            HudUiPanel* const panel = &bindPanel;
            panel->SetFont(selectedStyle->fontName, selectedStyle->fontSize, selectedStyle->fontWeight, 0, 0, 0, 2);
            const unsigned int color = selectedStyle->textColor;
            panel->textColor0 = color;
            panel->textColor1 = color;
            panel->textDirty = 1;
            panel->shadowEnabled = selectedStyle->shadowEnabled;
            panel->shadowOffsetX = 1;
            panel->shadowOffsetY = 1;
        }
    }

    zReader::Node* const listFontNode = zRdrGetNode(zrdSection, "LIST_FONT");
    if (listFontNode != 0) {
        listFontStyleRef = listFontNode->value.i32;
    }

    zReader::Node* const spacingNode = zRdrGetNode(zrdSection, "SPACING");
    if (spacingNode != 0) {
        bindingSlotSpacing = spacingNode->value.i32;
    }

    zReader::Node* const listOffsetNode = zRdrGetNode(zrdSection, "LIST_OFFSET");
    if (listOffsetNode != 0) {
        visibleListOffsetX = (float)listOffsetNode->value.nodes[1].value.nodes[1].value.i32;
        visibleListOffsetY = (float)listOffsetNode->value.nodes[1].value.nodes[2].value.i32;
        overflowListOffsetX = (float)listOffsetNode->value.nodes[2].value.nodes[1].value.i32;
        overflowListOffsetY = (float)listOffsetNode->value.nodes[2].value.nodes[2].value.i32;
    }

    zReader::Node* const listSizeNode = zRdrGetNode(zrdSection, "LISTSIZE");
    if (listSizeNode != 0) {
        zReader::Node* const listSizeBase = listSizeNode->value.nodes;
        const int totalCount = listSizeBase[1].value.i32;
        int visibleCount = 0;
        if (listSizeBase[0].value.i32 > 2) {
            visibleCount = listSizeBase[2].value.i32;
        }
        RebuildBindingSlotWidgets(totalCount, visibleCount);

        {
            for (int index = 0; index < bindingSlotTotalCount; ++index) {
                ((HudUiContainer*)(ownerDialog))->AddChild(&bindingSlotPanels[index]);
                bindingSlotPanels[index].SetVisible(1);
                bindingSlotPanels[index].owner = this;
                if (clipSource != 0) {
                    bindingSlotPanels[index].SetBltSourceAndClipRect(clipSource, &clipRect);
                }

                const HudFontStyle* const listStyle
                    = owner->fontStyles[listFontStyleRef].validMarker != 0 ? &owner->fontStyles[listFontStyleRef] : 0;
                if (listStyle != 0) {
                    // Retail repeats the complete font pass inside the child loop.
                    for (int fontIndex = 0; fontIndex < bindingSlotTotalCount; ++fontIndex) {
                        bindingSlotPanels[fontIndex]
                            .SetFont(listStyle->fontName, listStyle->fontSize, listStyle->fontWeight, 0, 0, 0, 2);
                        {
                            HudUiPanel* const panel = &bindingSlotPanels[fontIndex];
                            const unsigned int color = listStyle->textColor;
                            panel->textColor0 = color;
                            panel->textColor1 = color;
                            panel->textDirty = 1;
                        }
                        {
                            HudUiPanel* const panel = &bindingSlotPanels[fontIndex];
                            panel->shadowEnabled = listStyle->shadowEnabled;
                            panel->shadowOffsetX = 1;
                            panel->shadowOffsetY = 1;
                        }
                    }
                }
            }
        }

        ((HudUiContainer*)(ownerDialog))->AddChild((HudUiElement*)(&bindPanel));
        bindPanel.SetVisible(1);
        bindPanel.owner = this;
        if (clipSource != 0) {
            HudUiSetPanelClipWithSource(&bindPanel, clipSource, &clipRect);
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-rebuildbindingslotwidgets
 * @recoil-artifact defines .text recoil:function:0x4b90e0: HudCmdBindButtonBase::RebuildBindingSlotWidgets.
 *
 *
 * Purpose: recreate the binding-slot panel array and lay out visible and
 * overflow slots around the selected binding panel.
 */
void HudCmdBindButtonBase::RebuildBindingSlotWidgets(int totalCount, int visibleCount)
{
    if (bindingSlotPanels != 0) {
        delete[] bindingSlotPanels;
        bindingSlotPanels = 0;
    }
    bindingSlotPanels = new HudUiListSelectorItem[totalCount];
    bindingSlotTotalCount = totalCount;
    visibleBindingSlotCount = visibleCount;

    {
        for (int index = 0; index < visibleBindingSlotCount; ++index) {
            const int y
                = (int)((float)originY + (index - visibleBindingSlotCount) * bindingSlotSpacing + visibleListOffsetY);
            bindingSlotPanels[index].SetPos((int)((float)originX + visibleListOffsetX), y);
        }
    }

    bindPanel.SetPos(originX, originY);

    {
        for (int index = visibleBindingSlotCount; index < bindingSlotTotalCount; ++index) {
            const int y = (int)((float)originY + (index - visibleBindingSlotCount + 1) * bindingSlotSpacing
                + overflowListOffsetY);
            bindingSlotPanels[index].SetPos((int)((float)originX + overflowListOffsetX), y);
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-onselectedindexchanged
 * @recoil-artifact defines .text recoil:function:0x4b9320: HudCmdBindButtonBase::OnSelectedIndexChanged.
 * @recoil-match byte
 *
 * Purpose: handle the recovered HUD event path for HudCmdBindButtonBase::OnSelectedIndexChanged.
 */
void HudCmdBindButtonBase::OnSelectionChangedRefresh(int selectedIndex)
{
    SetSelectedEntry(selectedIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudcmdbindbuttonbase-setselectedentry
 * @recoil-artifact defines .text recoil:function:0x4b9330: HudCmdBindButtonBase::SetSelectedEntry.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudCmdBindButtonBase::SetSelectedEntry.
 */
void HudCmdBindButtonBase::SetSelectedEntry(int selectedIndex)
{
    int slotIndex;
    for (slotIndex = 0; slotIndex < visibleBindingSlotCount; ++slotIndex) {
        const int entryIndex = selectedIndex + slotIndex - visibleBindingSlotCount;
        if (entryIndex >= 0 && entryIndex < bindingVec.size()) {
            HudCmdBindingEntry* const entry = bindingVec[entryIndex];
            bindingSlotPanels[slotIndex].entryIndex = entryIndex;
            bindingSlotPanels[slotIndex].SetTextFmt("%s", entry->displayText);
            HudUiElement* const panel = &bindingSlotPanels[slotIndex];
            panel->SetVisible(1);
        } else {
            {
                HudUiElement* const panel = &bindingSlotPanels[slotIndex];
                panel->SetVisible(0);
            }
            (bindingSlotPanels + slotIndex)->DrawBase();
        }

        bindingSlotPanels[slotIndex].Invalidate();
    }

    if (selectedIndex >= 0 && selectedIndex < bindingVec.size()) {
        bindPanel.entryIndex = selectedIndex;
        bindPanel.SetTextFmt("%s", bindingVec[selectedIndex]->displayText);
    }

    for (slotIndex = visibleBindingSlotCount; slotIndex < bindingSlotTotalCount; ++slotIndex) {
        const int entryIndex = selectedIndex + slotIndex - visibleBindingSlotCount + 1;
        if (entryIndex >= 0 && entryIndex < bindingVec.size()) {
            HudCmdBindingEntry* const entry = bindingVec[entryIndex];
            bindingSlotPanels[slotIndex].entryIndex = entryIndex;
            bindingSlotPanels[slotIndex].SetTextFmt("%s", entry->displayText);
            HudUiElement* const panel = &bindingSlotPanels[slotIndex];
            panel->SetVisible(1);
        } else {
            {
                HudUiElement* const panel = &bindingSlotPanels[slotIndex];
                panel->SetVisible(0);
            }
            (bindingSlotPanels + slotIndex)->DrawBase();
        }

        bindingSlotPanels[slotIndex].Invalidate();
    }

    selectedBindingIndex = selectedIndex;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zui-zui_widgets-function-huduilistselectoritem-onactivate
 * @recoil-artifact defines .text recoil:function:0x4b9520: HudUiListSelectorItem::OnActivate.
 * @recoil-match byte
 *
 * Source model note: the constructor relationship is unresolved; the source
 * model lives in the inline class-body constructor in zhud_ui.h, and no source
 * edge is claimed here.
 * Purpose: handle the recovered HUD event path for HudUiListSelectorItem::OnActivate.
 */
void HudUiListSelectorItem::OnActivate()
{
    if (owner != 0) {
        ((HudCmdBindButtonBase*)(owner))->OnSelectionChangedRefresh(entryIndex);
    }
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-huduibackground-0x4b9540
 * @recoil-artifact defines .text recoil:function:0x4b9540: HudUiBackground::HudUiBackground.
 * @recoil-match byte
 *
 * Purpose: initialize the HUD background and its member arrays.
 */
HudUiBackground::HudUiBackground()
    : HudUiBackgroundContainer(1)
    , cursorWidget(0, 1)
{
    primaryClipImage = 0;
    capturedCompositeImage = 0;
    {
        for (int index = 0; index < 10; ++index) {
            backgroundSounds[index].sample = 0;
            backgroundSounds[index].volume = 1.0f;
            backgroundSounds[index].playHandle = 0;
        }
    }

    int defaultVMode = 5;
    zOptionEntryPartial* vmodeOption = zGame::OptionsFindOption("VMode");
    if (vmodeOption == 0) {
        vmodeOption = (zOptionEntryPartial*)(&defaultVMode);
    }

    switch (vmodeOption->payloadOrBuffer) {
    case 2:
    case 4:
        uiOriginX = 0;
        uiOriginY = -40;
        break;
    case 3:
    case 5:
        uiOriginX = 0;
        uiOriginY = 0;
        break;
    case 6:
        uiOriginX = 0;
        uiOriginY = 60;
        break;
    case 7:
        uiOriginX = 0;
        uiOriginY = 144;
        break;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-huduibackground-0x4b9760
 * @recoil-artifact defines .text recoil:function:0x4b9760: HudUiBackground::~HudUiBackground.
 * @recoil-match byte
 *
 * Purpose: Releases owned background clip images before compiler-generated member and base cleanup.
 */
HudUiBackground::~HudUiBackground()
{
    if (primaryClipImage != 0) {
        primaryClipImage = zVid_Image::ReleaseIfNotDefault(primaryClipImage);
    }

    if (capturedCompositeImage != 0) {
        capturedCompositeImage = zVid_Image::ReleaseIfNotDefault(capturedCompositeImage);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-setenabled
 * @recoil-artifact defines .text recoil:function:0x4b9850: HudUiBackground::SetEnabled.
 * @recoil-match byte
 *
 * Purpose: start or stop configured background sounds and update background visibility state.
 */
void HudUiBackground::SetEnabled(int enabled)
{
    if (enabled != 0) {
        int entryIndex14;
        for (entryIndex14 = 0; entryIndex14 < (int)(sizeof(backgroundSounds) / sizeof(backgroundSounds[0]));
            ++entryIndex14) {
            HudUiBackgroundSoundEntry& entry = backgroundSounds[entryIndex14];
            if (entry.sample != 0) {
                entry.playHandle = entry.sample->PlayA3DSimple(entry.volume);
            }
        }

        InvalidateChildren();
    } else {
        int entryIndex15;
        for (entryIndex15 = 0; entryIndex15 < (int)(sizeof(backgroundSounds) / sizeof(backgroundSounds[0]));
            ++entryIndex15) {
            HudUiBackgroundSoundEntry& entry = backgroundSounds[entryIndex15];
            if (entry.playHandle != 0) {
                entry.playHandle->StopIfActive();
            }

            entry.playHandle = 0;
        }
    }

    HudUiBackgroundContainer::SetEnabled(enabled);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x4b98d0: HudUiBackground::LoadFromZrd.
 * @recoil-match byte
 *
 * Purpose: load the recovered HUD data handled by HudUiBackground::LoadFromZrd.
 */
zReader::Node* HudUiBackground::LoadFromZrd(const char* zrdPath, const char* sectionName, int capturePrimary)
{
    zReader::Node* const root = zReader::Load(zrdPath, 0, 0);
    loadedRoot = root;
    return LoadZrdAndSection(root, sectionName, capturePrimary);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-loadzrdandsection
 * @recoil-artifact defines .text recoil:function:0x4b9900: HudUiBackground::LoadZrdAndSection.
 *
 *
 * Purpose: load the recovered HUD data handled by HudUiBackground::LoadZrdAndSection.
 */
zReader::Node*
HudUiBackground::LoadZrdAndSection(zReader::Node* loadedRootNode, const char* sectionName, int capturePrimary)
{
    zReader::Node* result = 0;
    zVideo::RunPostprocessOnPrimaryBuffer();

    if (capturePrimary == 0) {
        primaryClipImage = zVideobuffCaptureSurfaceToImage(1);
    }

    if (loadedRootNode != 0) {
        result = loadedRootNode;

        zReader::Node* const sharedImagePath = zRdrGetNode(loadedRootNode, zHudCfgKey_SHARED_IMAGE_PATH);
        if (sharedImagePath != 0) {
            zImageInitMissionResources(sharedImagePath->value.nodes[1].value.str);
        }

        zReader::Node* const sectionRoot = zRdrGetNode(loadedRootNode, sectionName);
        cfgRoot = sectionRoot;

        if (sectionRoot != 0) {
            zReader::Node* const imagePath = zRdrGetNode(sectionRoot, "IMAGE_PATH");
            if (imagePath != 0) {
                zImageInitMissionResources(imagePath->value.nodes[1].value.str);
            }

            zReader::Node* const fontListNode = zRdrGetNode(cfgRoot, g_HudCfgKey_Fonts);
            if (fontListNode != 0) {
                const int fontCount
                    = fontListNode->value.nodes[0].value.i32 < 20 ? fontListNode->value.nodes[0].value.i32 : 20;

                for (int index = 1; index < fontCount; ++index) {
                    zReader::Node* const fontEntry = &fontListNode->value.nodes[index];

                    const int styleIndex = fontEntry->value.nodes[1].value.i32;

                    fontStyles[styleIndex].validMarker = 1;
                    fontStyles[styleIndex].bkColor = 0;
                    fontStyles[styleIndex].bkMode = 1;
                    fontStyles[styleIndex].fontName = fontEntry->value.nodes[2].value.str;
                    fontStyles[styleIndex].fontSize = fontEntry->value.nodes[3].value.i32;

                    if (fontEntry->value.nodes[4].value.nodes[1].type == zReader::ZRDR_NODE_ARRAY) {
                        fontStyles[styleIndex].textColor = RGB(
                            fontEntry->value.nodes[4].value.nodes[1].value.nodes[1].value.i32,
                            fontEntry->value.nodes[4].value.nodes[1].value.nodes[2].value.i32,
                            fontEntry->value.nodes[4].value.nodes[1].value.nodes[3].value.i32
                        );

                        fontStyles[styleIndex].bkColor = RGB(
                            fontEntry->value.nodes[4].value.nodes[2].value.nodes[1].value.i32,
                            fontEntry->value.nodes[4].value.nodes[2].value.nodes[2].value.i32,
                            fontEntry->value.nodes[4].value.nodes[2].value.nodes[3].value.i32
                        );
                        fontStyles[styleIndex].bkMode = 2;
                    } else {
                        fontStyles[styleIndex].textColor = RGB(
                            fontEntry->value.nodes[4].value.nodes[1].value.i32,
                            fontEntry->value.nodes[4].value.nodes[2].value.i32,
                            fontEntry->value.nodes[4].value.nodes[3].value.i32
                        );
                    }

                    if (fontEntry->value.nodes[0].value.i32 >= 6) {
                        fontStyles[styleIndex].shadowEnabled = fontEntry->value.nodes[5].value.i32;
                    }
                    if (fontEntry->value.nodes[0].value.i32 >= 7) {
                        fontStyles[styleIndex].fontWeight = fontEntry->value.nodes[6].value.i32;
                    }
                    if (fontEntry->value.nodes[0].value.i32 >= 8) {
                        const char* const align = fontEntry->value.nodes[7].value.str;
                        if (strcmp(align, "LEFT") != 0) {
                            if (strcmp(align, "RIGHT") == 0) {
                                fontStyles[styleIndex].alignMode = 2;
                            } else if (strcmp(align, "CENTER") == 0) {
                                fontStyles[styleIndex].alignMode = 1;
                            }
                        } else {
                            fontStyles[styleIndex].alignMode = 0;
                        }
                    }
                }
            }

            zReader::Node* const imageListNode = zRdrGetNode(cfgRoot, zHudCfgKey_BACKGROUND_IMAGES);
            if (imageListNode != 0) {
                int imageCount = imageListNode->value.nodes[0].value.i32;
                if (imageCount >= 20) {
                    imageCount = 20;
                }

                for (int index = 1; index < imageCount; ++index) {
                    const int originX = uiOriginX;
                    const int originY = uiOriginY;
                    zReader::Node* const imageEntry = &imageListNode->value.nodes[index];

                    HudUiWidget& child = backgroundImageWidgets[index - 1];
                    child.SetImageByPathOwned(imageEntry->value.nodes[1].value.str);
                    if (imageEntry->value.nodes[0].value.i32 >= 4) {
                        ((HudUiElement*)(&child))
                            ->SetPos(
                                imageEntry->value.nodes[2].value.i32 + originX,
                                imageEntry->value.nodes[3].value.i32 + originY
                            );
                    }

                    child.flags = (unsigned int)((unsigned char)(child.flags) & 0x10u) | 0x02u;
                    ((HudUiElement*)(&child))->SetVisible(1);
                    ((HudUiElement*)(&child))->Invalidate();
                    AddChild((HudUiElement*)(&child));
                }
            }

            zReader::Node* const videoListNode = zRdrGetNode(cfgRoot, zHudCfgKey_BACKGROUND_VIDEOS);
            if (videoListNode != 0) {
                const int videoCount
                    = videoListNode->value.nodes[0].value.i32 < 10 ? videoListNode->value.nodes[0].value.i32 : 10;

                for (int index = 1; index < videoCount; ++index) {
                    const int originX = uiOriginX;
                    const int originY = uiOriginY;
                    zReader::Node* const videoEntry = &videoListNode->value.nodes[index];

                    HudUiBackgroundVideoWidget& child = backgroundVideoWidgets[index - 1];
                    child.SetMediaPathOwnedAndRefresh(videoEntry->value.nodes[1].value.str);
                    if (videoEntry->value.nodes[0].value.i32 >= 4) {
                        child.SetPos(
                            videoEntry->value.nodes[2].value.i32 + originX,
                            videoEntry->value.nodes[3].value.i32 + originY
                        );
                    }
                    if (videoEntry->value.nodes[0].value.i32 >= 5) {
                        zReader::Node* const color = videoEntry->value.nodes[4].value.nodes;
                        child.SetColorKey565((unsigned short)(zVidPackColorRGB(
                            color[1].value.i32,
                            color[2].value.i32,
                            color[3].value.i32
                        )));
                    }

                    child.SetVisible(1);
                    child.Invalidate();
                    // Retail preserves these four polymorphic queries before the
                    // blit-source update even though their return values are not
                    // retained by the surrounding path.
                    child.GetCenterX();
                    child.GetCenterY();
                    child.GetCenterX();
                    child.GetCenterY();
                    child.SetBltSourceAndClipRect(primaryClipImage, 0);
                    child.RebuildBltRect();
                    AddChild(&child);
                }
            }

            zReader::Node* const textListNode = zRdrGetNode(cfgRoot, zHudCfgKey_BACKGROUND_TEXT);
            if (textListNode != 0) {
                const int textCount
                    = textListNode->value.nodes[0].value.i32 < 50 ? textListNode->value.nodes[0].value.i32 : 50;

                for (int index = 1; index < textCount; ++index) {
                    zReader::Node* const textEntry = &textListNode->value.nodes[index];

                    HudUiPanel* const child = (HudUiPanel*)(&backgroundTextPanels[index - 1]);
                    child->SetTextFmt(zLoc::ResolveMessageKeyOrFallback(textEntry->value.nodes[1].value.str));
                    const int originX = uiOriginX;
                    const int originY = uiOriginY;
                    child->SetPos(
                        textEntry->value.nodes[2].value.i32 + originX,
                        textEntry->value.nodes[3].value.i32 + originY
                    );
                    const int styleIndex = textEntry->value.nodes[4].value.i32;
                    const HudFontStyle* const style
                        = fontStyles[styleIndex].validMarker != 0 ? &fontStyles[styleIndex] : 0;
                    if (style != 0) {
                        child->alignMode = style->alignMode;
                        child->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
                        child->SetTextColorsAndMarkDirty(style->textColor, style->textColor);
                        child->SetShadow(style->shadowEnabled, 1, 1);
                        child->SetTextBackground(style->bkMode, style->bkColor);
                    }
                    child->SetVisible(1);
                    AddChild((HudUiElement*)(child));
                }
            }

            if (capturePrimary == 0) {
                SetEnabled(1);
                UpdateAll(0.0f);
                capturedCompositeImage = zVideobuffCaptureSurfaceToImage(1);
                SetEnabled(0);
                ((HudUiDialogController*)(this))->BlitOwnedSurfaceToPrimary();
            }

            zReader::Node* const cursorNode = zRdrGetNode(cfgRoot, zHudCfgKey_CURSOR);
            if (cursorNode != 0) {
                zReader::Node* const bitmapNode = zRdrGetNode(cursorNode, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap);
                if (bitmapNode != 0) {
                    cursorWidget.SetImageByPathOwnedAndRefresh(bitmapNode->value.nodes[1].value.str);
                    SetInputFocus((HudUiElement*)(&cursorWidget));
                }

                zReader::Node* const centerNode = zRdrGetNode(cursorNode, "CENTER");
                if (centerNode != 0) {
                    // Original 0x4b9f69 stores CENTER's string pointer into HudUiWidget::alignFlags;
                    // GetCenterX/Y only test the slot for nonzero on this cursor path.
                    cursorWidget.alignFlags = (unsigned int)(centerNode->value.nodes[1].value.str);
                }

                int cursorCapture = 1;
                zReader::GetInt(cursorNode, zHudCfgKey_CAPTURE, &cursorCapture);
                cursorWidget.SetImageOwnedAndRefresh(cursorCapture);
            }

            zReader::Node* const soundListNode = zRdrGetNode(cfgRoot, zHudCfgKey_BACKGROUND_SOUNDS);
            if (soundListNode != 0) {
                int soundCount = soundListNode->value.nodes[0].value.i32;
                if (soundCount >= 10) {
                    soundCount = 10;
                }

                for (int index = 1; index < soundCount; ++index) {
                    zReader::Node* const soundEntry = &soundListNode->value.nodes[index];

                    float volume = 1.0f;
                    if (soundEntry->value.nodes[0].value.i32 >= 3) {
                        volume = soundEntry->value.nodes[2].value.f32;
                    }
                    backgroundSounds[index - 1].sample = zSnd::FindSampleByName(soundEntry->value.nodes[1].value.str);
                    backgroundSounds[index - 1].volume = volume;
                }
            }
        }
    }

    zVideo::DispatchUnlockPrimarySurfaceState();
    return result;
}

#include "GameZRecoil/zHud/zhud_ui_defs.h"

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-bindbuttonsnodetowidgetbyname
 * @recoil-artifact defines .text recoil:function:0x4ba070: HudUiBackground::BindButtonsNodeToWidgetByName.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiBackground::BindButtonsNodeToWidgetByName.
 */
unsigned char __fastcall HudUiBackground::BindButtonsNodeToWidgetByName(
    zReader::Node* parentNode,
    HudUiWidget* widget,
    const char* name
)
{
    if (parentNode != 0) {
        zReader::Node* const buttonsNode = zRdrGetNode(parentNode, "BUTTONS");
        zReader::Node* const widgetNode = zRdrGetNode(buttonsNode, name);
        if (widgetNode != 0) {
            ((HudUiZrdWidget*)widget)->LoadFromZrd(widgetNode, this);
            ((HudUiZrdWidget*)widget)->PostLoadFromZrd();
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-bindwidgetbyname
 * @recoil-artifact defines .text recoil:function:0x4ba0c0: HudUiBackground::BindWidgetByName.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiBackground::BindWidgetByName.
 */
int HudUiBackground::BindWidgetByName(zReader::Node*, HudUiWidget* widget, const char* name)
{
    return BindButtonsNodeToWidgetByName(cfgRoot, widget, name) & 0xff;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-bindprimitivenodetoelement
 * @recoil-artifact defines .text recoil:function:0x4ba0e0: HudUiBackground::BindPrimitiveNodeToElement.
 * @recoil-match byte
 *
 * Purpose: bind a named ZRD primitive node to an existing HUD element.
 * Binary Ninja: 0x4ba0e0 performs direct zReader::Node child/value reads for
 * optional BITMAP, POSITION, WORDWRAP, FONT, COLOR, ENDP_REL, and ENDP_ABS
 * records before assigning the final blit source, clip rect, and dirty state.
 */
int HudUiBackground::BindPrimitiveNodeToElement(zReader::Node*, HudUiElement* element, const char* name)
{
    zReader::Node* const cfgRoot = this->cfgRoot;
    if (cfgRoot == 0) {
        return 0;
    }

    zReader::Node* primitiveNode = zRdrGetNode(cfgRoot, "PRIMITIVES");
    if (primitiveNode != 0) {
        primitiveNode = zRdrGetNode(primitiveNode, name);
    }

    if (primitiveNode != 0) {
        ((HudUiContainer*)(this))->AddChild(element);

        zReader::Node* bitmapNode = zRdrGetNode(primitiveNode, g_HudUiCycleSelectorWidget_ZrdKey_Bitmap);
        if (bitmapNode != 0) {
            ((HudUiWidget*)(element))->SetImageByPathOwned(bitmapNode->value.nodes[1].value.str);
        }

        zReader::Node* positionNode = zRdrGetNode(primitiveNode, "POSITION");
        if (positionNode != 0) {
            zReader::Node* const positionBase = positionNode->value.nodes;
            element->SetPos(uiOriginX + positionBase[1].value.i32, uiOriginY + positionBase[2].value.i32);
        }

        zReader::Node* wordWrapNode = zRdrGetNode(primitiveNode, "WORDWRAP");
        if (wordWrapNode != 0) {
            zReader::Node* const wordWrapBase = wordWrapNode->value.nodes;
            HudUiRect wordWrapRect;
            wordWrapRect.left = 0;
            wordWrapRect.top = 0;
            wordWrapRect.right = wordWrapBase[1].value.i32;
            wordWrapRect.bottom = wordWrapBase[2].value.i32;
            element->EnableWordWrapWithRect(&wordWrapRect);
        }

        zReader::Node* fontNode = zRdrGetNode(primitiveNode, g_HudUiCycleSelectorWidget_ZrdKey_Font);
        if (fontNode != 0) {
            const int fontIndex = fontNode->value.i32;
            const HudFontStyle* style = fontStyles[fontIndex].validMarker != 0 ? &fontStyles[fontIndex] : 0;
            if (style != 0) {
                HudUiPanel* const panel = (HudUiPanel*)(element);
                panel->alignMode = style->alignMode;
                panel->SetFont(style->fontName, style->fontSize, style->fontWeight, 0, 0, 0, 2);
                const unsigned int textColor = style->textColor;
                panel->textColor0 = textColor;
                panel->textColor1 = textColor;
                panel->textDirty = 1;
                panel->shadowEnabled = style->shadowEnabled;
                panel->shadowOffsetX = 1;
                panel->shadowOffsetY = 1;
                panel->SetTextBackground(style->bkMode, style->bkColor);
            }
        }

        zReader::Node* colorNode = zRdrGetNode(primitiveNode, "COLOR");
        if (colorNode != 0) {
            zReader::Node* const colorBase = colorNode->value.nodes;
            ((HudUiPrimitiveBindTarget*)(element))->color565 = zVidPackColorRGB(
                                                                   (unsigned char)(colorBase[1].value.i32),
                                                                   (unsigned char)(colorBase[2].value.i32),
                                                                   (unsigned char)(colorBase[3].value.i32)
                                                               )
                & 0xffffu;
        }

        zReader::Node* relativeEndNode = zRdrGetNode(primitiveNode, "ENDP_REL");
        if (relativeEndNode != 0) {
            ((HudUiPrimitiveBindTarget*)(element))
                ->SetSegmentEndpoints(
                    element->GetCenterX(),
                    element->GetCenterY(),
                    element->GetCenterX() + relativeEndNode->value.nodes[1].value.i32,
                    element->GetCenterY() + relativeEndNode->value.nodes[2].value.i32
                );
        }

        zReader::Node* absoluteEndNode = zRdrGetNode(primitiveNode, "ENDP_ABS");
        if (absoluteEndNode != 0) {
            zReader::Node* const absoluteEndBase = absoluteEndNode->value.nodes;
            ((HudUiPrimitiveBindTarget*)(element))
                ->SetSegmentEndpoints(
                    element->GetCenterX(),
                    element->GetCenterY(),
                    absoluteEndBase[1].value.i32,
                    absoluteEndBase[2].value.i32
                );
        }

        HudUiRect clipRect;
        clipRect.left = element->GetCenterX();
        clipRect.top = element->GetCenterY();
        clipRect.right = element->GetCenterX();
        clipRect.bottom = element->GetCenterY();
        element->SetBltSourceAndClipRect(capturedCompositeImage, &clipRect);

        const unsigned int visibleFlag = (unsigned char)(element->flags);
        element->flags = (unsigned char)((visibleFlag & ~0xefu) | 0x02u);
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduibackground-freeloadedtreeroots
 * @recoil-artifact defines .text recoil:function:0x4ba350: HudUiBackground::FreeLoadedTreeRoots.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiBackground::FreeLoadedTreeRoots.
 */
void HudUiBackground::FreeLoadedTreeRoots(int)
{
    zReader::Node* const root = loadedRoot;
    if (root != 0) {
        zReader::Free(root);
    }

    loadedRoot = 0;
    cfgRoot = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduidialogcontroller-blitownedsurfacetoprimary
 * @recoil-artifact defines .text recoil:function:0x4ba380: HudUiDialogController::BlitOwnedSurfaceToPrimary.
 * @recoil-match byte
 *
 * Purpose: blit the captured dialog image back to the active primary target.
 */
void HudUiDialogController::BlitOwnedSurfaceToPrimary()
{
    if (capturedImage != 0) {
        zVid_Image::BlitToActiveTarget(capturedImage, 0, 0, 0, 0);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduicontainer-invalidatechildren
 * @recoil-artifact defines .text recoil:function:0x4ba3a0: HudUiContainer::InvalidateChildren.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiContainer::InvalidateChildren.
 */
void HudUiContainer::InvalidateChildren()
{
    for (HudUiElement* child = childHead; child != 0; child = child->next) {
        child->Invalidate();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduifillbitmap-setnormalizedvalue
 * @recoil-artifact defines .text recoil:function:0x4ba3c0: HudUiFillBitmap::SetNormalizedValue.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUiFillBitmap::SetNormalizedValue.
 */
void HudUiFillBitmap::SetNormalizedValueAndRebuild(float value)
{
    normalizedValue = value;
    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduiownedtextinput-onaccept
 * @recoil-artifact defines .text recoil:function:0x4ba3e0: HudUiOwnedTextInput::OnAccept.
 * @recoil-match byte
 *
 * Purpose: handle the recovered HUD event path for HudUiOwnedTextInput::OnAccept.
 */
void HudUiOwnedTextInput::OnAccept()
{
    HudUiTextInput::OnAccept();
    owner->OnAccept();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduipanel-getwraprect
 * @recoil-artifact defines .text recoil:function:0x4ba400: HudUiPanel::GetWrapRect.
 * @recoil-match byte
 *
 * Purpose: Returns the panel word-wrap rectangle storage.
 */
HudUiRect* HudUiPanel::GetBoundsRectOrNull()
{
    return &wrapRect;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-huduilistselectoritem-draw
 * @recoil-artifact defines .text recoil:function:0x4ba410: HudUiListSelectorItem::Draw.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiListSelectorItem::Draw.
 */
void HudUiListSelectorItem::Draw()
{
    HudUiPanel::Draw();

    clipRect.left = GetCenterX();
    clipRect.right = GetCenterX() + QueryTextWidth();
    clipRect.top = GetCenterY();
    clipRect.bottom = GetCenterY() + QueryTextHeight();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-widgets-hudfontstyle-hudfontstyle
 * @recoil-artifact defines .text recoil:function:0x4ba4a0: HudFontStyle::HudFontStyle.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudFontStyle::HudFontStyle.
 */
HudFontStyle::HudFontStyle()
{
    validMarker = 0;
    fontName = 0;
    fontSize = 0;
    textColor = 0;
    shadowEnabled = 0;
    alignMode = 0;
    fontWeight = 0x1f4;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-hudfontstyle-destructor
 * @recoil-artifact defines .text recoil:function:0x4ba4c0: HudFontStyle::~HudFontStyle.
 * @recoil-match byte
 *
 * Purpose: Clear the font-style validity marker when the record is destroyed.
 *
 * Retail array construction passes this destructor for the twenty font records.
 * Its body clears the first field and returns without calling another function.
 * The class has no base or owned members requiring further teardown.
 * The ordinary destructor directly expresses that lifetime operation.
 */
HudFontStyle::~HudFontStyle()
{
    validMarker = 0;
}
