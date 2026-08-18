#include "recoil/Mfc42Abi.h"
#include "pickup.h"

#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zUtil/zbd.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char g_PickupLogicalName_ERFPG_AMMO[];
extern char g_PickupLogicalName_HEMORTAR_AMMO[];
extern char g_PickupLogicalName_QMORTAR_AMMO[];
extern char g_PickupLogicalName_FREON_AMMO[];
extern char g_PickupLogicalName_NAPALM_AMMO[];
extern char g_PickupLogicalName_P_HEMINE_AMMO[];
extern char g_PickupLogicalName_P_QMINE_AMMO[];
extern char g_PickupLogicalName_R_HEMINE_AMMO[];
extern char g_PickupLogicalName_R_QMINE_AMMO[];
extern char g_PickupLogicalName_LOCKON_LASER_AMMO[];
extern char g_PickupLogicalName_LASER_SABRE_AMMO[];
extern char g_PickupLogicalName_SONIC_CANNON_AMMO[];
extern char g_PickupLogicalName_ARC_SABRE_AMMO[];
extern char g_PickupLogicalName_MISSILE_AMMO[];
extern char g_PickupLogicalName_GUIDED_MISSILE_AMMO[];
extern char g_PickupLogicalName_NUKE_AMMO[];
extern char g_PickupLogicalName_GUIDED_NUKE_AMMO[];
extern char g_PickupOptKey_Wep1_1[];
extern char g_PickupLogicalName_ERFPG_WEAPON[];
extern char g_PickupOptKey_Wep2_0[];
extern char g_PickupLogicalName_HEMORTAR_WEAPON[];
extern char g_PickupOptKey_Wep2_1[];
extern char g_PickupLogicalName_QMORTAR_WEAPON[];
extern char g_PickupOptKey_Wep3_0[];
extern char g_PickupLogicalName_FREON_WEAPON[];
extern char g_PickupOptKey_Wep3_1[];
extern char g_PickupLogicalName_NAPALM_WEAPON[];
extern char g_PickupOptKey_Wep4_0[];
extern char g_PickupLogicalName_P_HEMINE_WEAPON[];
extern char g_PickupOptKey_Wep4_1[];
extern char g_PickupLogicalName_P_QMINE_WEAPON[];
extern char g_PickupOptKey_Wep5_0[];
extern char g_PickupLogicalName_R_HEMINE_WEAPON[];
extern char g_PickupOptKey_Wep5_1[];
extern char g_PickupLogicalName_R_QMINE_WEAPON[];
extern char g_PickupOptKey_Wep6_0[];
extern char g_PickupLogicalName_LOCKON_LASER_WEAPON[];
extern char g_PickupOptKey_Wep6_1[];
extern char g_PickupLogicalName_LASER_SABRE_WEAPON[];
extern char g_PickupOptKey_Wep7_0[];
extern char g_PickupLogicalName_SONIC_CANNON_WEAPON[];
extern char g_PickupOptKey_Wep7_1[];
extern char g_PickupLogicalName_ARC_SABRE_WEAPON[];
extern char g_PickupOptKey_Wep8_0[];
extern char g_PickupLogicalName_MISSILE_WEAPON[];
extern char g_PickupOptKey_Wep8_1[];
extern char g_PickupLogicalName_GUIDED_MISSILE_WEAPON[];
extern char g_PickupOptKey_Wep9_0[];
extern char g_PickupLogicalName_NUKE_WEAPON[];
extern char g_PickupOptKey_Wep9_1[];
extern char g_PickupLogicalName_GUIDED_NUKE_WEAPON[];
extern char g_PickupLogicalName_NANITE100[];
extern char g_PickupLogicalName_NANO_CANISTER[];
extern char g_PickupLogicalName_PUP_AMPHIB[];
extern char g_PickupLogicalName_PUP_HOVER[];
extern char g_PickupLogicalName_PUP_SUB[];

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuptypes
 * @recoil-artifact defines .data recoil:data:0x4db6e8: g_PickupTypes (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: define the 40-row pickup type table used by pickup spawning and
 * effect application.
 */
PickupType g_PickupTypes[40] = {
    {0, 515, 0, 30, g_PickupLogicalName_ERFPG_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 517, 1, 3, g_PickupLogicalName_HEMORTAR_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 519, 2, 3, g_PickupLogicalName_QMORTAR_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 526, 3, 10, g_PickupLogicalName_FREON_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 523, 4, 10, g_PickupLogicalName_NAPALM_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 545, 5, 5, g_PickupLogicalName_P_HEMINE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 547, 6, 5, g_PickupLogicalName_P_QMINE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 549, 7, 5, g_PickupLogicalName_R_HEMINE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 551, 8, 5, g_PickupLogicalName_R_QMINE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 553, 9, 10, g_PickupLogicalName_LOCKON_LASER_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 555, 10, 10, g_PickupLogicalName_LASER_SABRE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 557, 11, 2, g_PickupLogicalName_SONIC_CANNON_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 559, 12, 10, g_PickupLogicalName_ARC_SABRE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 521, 13, 3, g_PickupLogicalName_MISSILE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 561, 14, 5, g_PickupLogicalName_GUIDED_MISSILE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 565, 15, 1, g_PickupLogicalName_NUKE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {0, 563, 16, 1, g_PickupLogicalName_GUIDED_NUKE_AMMO, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep1_1, 516, 17, 30, g_PickupLogicalName_ERFPG_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep2_0, 518, 18, 3, g_PickupLogicalName_HEMORTAR_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep2_1, 520, 19, 3, g_PickupLogicalName_QMORTAR_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep3_0, 527, 20, 10, g_PickupLogicalName_FREON_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep3_1, 524, 21, 10, g_PickupLogicalName_NAPALM_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep4_0, 546, 22, 5, g_PickupLogicalName_P_HEMINE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep4_1, 548, 23, 5, g_PickupLogicalName_P_QMINE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep5_0, 550, 24, 5, g_PickupLogicalName_R_HEMINE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep5_1, 552, 25, 5, g_PickupLogicalName_R_QMINE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep6_0, 554, 26, 10, g_PickupLogicalName_LOCKON_LASER_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep6_1, 556, 27, 10, g_PickupLogicalName_LASER_SABRE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep7_0, 558, 28, 10, g_PickupLogicalName_SONIC_CANNON_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep7_1, 560, 29, 10, g_PickupLogicalName_ARC_SABRE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep8_0, 522, 30, 3, g_PickupLogicalName_MISSILE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep8_1, 562, 31, 5, g_PickupLogicalName_GUIDED_MISSILE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep9_0, 566, 32, 1, g_PickupLogicalName_NUKE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {g_PickupOptKey_Wep9_1, 564, 33, 1, g_PickupLogicalName_GUIDED_NUKE_WEAPON, 0, 0, 0, 0, 0, 0, 0},
    {0, 528, 34, 60, "NANITE", 0, 0, 0, 0, 0, 0, 0},
    {0, 586, 35, 100, g_PickupLogicalName_NANITE100, 0, 0, 0, 0, 0, 0, 0},
    {0, 525, 36, 1, g_PickupLogicalName_NANO_CANISTER, 0, 0, 0, 0, 0, 0, 0},
    {0, 577, 37, 1, g_PickupLogicalName_PUP_AMPHIB, 0, 0, 0, 0, 0, 0, 0},
    {0, 578, 38, 1, g_PickupLogicalName_PUP_HOVER, 0, 0, 0, 0, 0, 0, 0},
    {0, 581, 39, 1, g_PickupLogicalName_PUP_SUB, 0, 0, 0, 0, 0, 0, 0}
};
PickupSpawnList g_PickupSpawnList_NetworkCopy = {0};
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuprespawnqueue
 * @recoil-artifact defines .data recoil:data:0x4f3308: g_PickupRespawnQueue (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: hold the BSS-zeroed head/tail/count state for pending pickup respawns.
 */
PickupRespawnQueue g_PickupRespawnQueue = {0};
PickupSpawnList g_PickupSpawnList_Primary = {0};
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-nextpickupid
 * @recoil-artifact defines .data recoil:data:0x4f3330: g_NextPickupId (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: track the next pickup instance id assigned to spawned pickup nodes.
 */
int g_NextPickupId = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickup-lastvtoldropindex
 * @recoil-artifact defines .data recoil:data:0x4dbe68: g_Pickup_LastVTOLDropIndex (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: remember the rotating VTOL weapon-drop cursor between airdrops.
 */
int g_Pickup_LastVTOLDropIndex = 19;
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-pup-sub
 * @recoil-artifact defines .data recoil:data:0x4dbe6c: g_PickupLogicalName_PUP_SUB.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the submarine puppy pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_PUP_SUB[] = "PUP_SUB";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-pup-hover
 * @recoil-artifact defines .data recoil:data:0x4dbe74: g_PickupLogicalName_PUP_HOVER.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the hover puppy pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_PUP_HOVER[] = "PUP_HOVER";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-pup-amphib
 * @recoil-artifact defines .data recoil:data:0x4dbe80: g_PickupLogicalName_PUP_AMPHIB.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the amphibious puppy pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_PUP_AMPHIB[] = "PUP_AMPHIB";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-nano-canister
 * @recoil-artifact defines .data recoil:data:0x4dbe8c: g_PickupLogicalName_NANO_CANISTER.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the nano-canister pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_NANO_CANISTER[] = "NANO-CANISTER";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-nanite100
 * @recoil-artifact defines .data recoil:data:0x4dbe9c: g_PickupLogicalName_NANITE100.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the full nanite pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_NANITE100[] = "NANITE100";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-guided-nuke-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbea8: g_PickupLogicalName_GUIDED_NUKE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the guided nuke weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_GUIDED_NUKE_WEAPON[] = "GUIDED_NUKE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep9-1
 * @recoil-artifact defines .data recoil:data:0x4dbebc: g_PickupOptKey_Wep9_1.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the guided nuke weapon pickup to the wep_9.1 option key.
 */
char g_PickupOptKey_Wep9_1[] = "wep_9.1";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-nuke-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbec4: g_PickupLogicalName_NUKE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the nuke weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_NUKE_WEAPON[] = "NUKE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep9-0
 * @recoil-artifact defines .data recoil:data:0x4dbed0: g_PickupOptKey_Wep9_0.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the nuke weapon pickup to the wep_9.0 option key.
 */
char g_PickupOptKey_Wep9_0[] = "wep_9.0";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-guided-missile-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbed8: g_PickupLogicalName_GUIDED_MISSILE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the guided missile weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_GUIDED_MISSILE_WEAPON[] = "GUIDED_MISSILE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep8-1
 * @recoil-artifact defines .data recoil:data:0x4dbef0: g_PickupOptKey_Wep8_1.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the guided missile weapon pickup to the wep_8.1 option key.
 */
char g_PickupOptKey_Wep8_1[] = "wep_8.1";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-missile-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbef8: g_PickupLogicalName_MISSILE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the missile weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_MISSILE_WEAPON[] = "MISSILE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep8-0
 * @recoil-artifact defines .data recoil:data:0x4dbf08: g_PickupOptKey_Wep8_0.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the missile weapon pickup to the wep_8.0 option key.
 */
char g_PickupOptKey_Wep8_0[] = "wep_8.0";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-arc-sabre-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbf10: g_PickupLogicalName_ARC_SABRE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the arc sabre weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_ARC_SABRE_WEAPON[] = "ARC_SABRE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep7-1
 * @recoil-artifact defines .data recoil:data:0x4dbf24: g_PickupOptKey_Wep7_1.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the arc sabre weapon pickup to the wep_7.1 option key.
 */
char g_PickupOptKey_Wep7_1[] = "wep_7.1";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-sonic-cannon-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbf2c: g_PickupLogicalName_SONIC_CANNON_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the sonic cannon weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_SONIC_CANNON_WEAPON[] = "SONIC_CANNON_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep7-0
 * @recoil-artifact defines .data recoil:data:0x4dbf40: g_PickupOptKey_Wep7_0.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the sonic cannon weapon pickup to the wep_7.0 option key.
 */
char g_PickupOptKey_Wep7_0[] = "wep_7.0";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-laser-sabre-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbf48: g_PickupLogicalName_LASER_SABRE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the laser sabre weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_LASER_SABRE_WEAPON[] = "LASER_SABRE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep6-1
 * @recoil-artifact defines .data recoil:data:0x4dbf5c: g_PickupOptKey_Wep6_1.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the laser sabre weapon pickup to the wep_6.1 option key.
 */
char g_PickupOptKey_Wep6_1[] = "wep_6.1";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-lockon-laser-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbf64: g_PickupLogicalName_LOCKON_LASER_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the lock-on laser weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_LOCKON_LASER_WEAPON[] = "LOCKON_LASER_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep6-0
 * @recoil-artifact defines .data recoil:data:0x4dbf78: g_PickupOptKey_Wep6_0.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the lock-on laser weapon pickup to the wep_6.0 option key.
 */
char g_PickupOptKey_Wep6_0[] = "wep_6.0";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-r-qmine-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbf80: g_PickupLogicalName_R_QMINE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the red quantum mine weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_R_QMINE_WEAPON[] = "R_QMINE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep5-1
 * @recoil-artifact defines .data recoil:data:0x4dbf90: g_PickupOptKey_Wep5_1.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the red quantum mine weapon pickup to the wep_5.1 option key.
 */
char g_PickupOptKey_Wep5_1[] = "wep_5.1";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-r-hemine-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbf98: g_PickupLogicalName_R_HEMINE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the red high-explosive mine weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_R_HEMINE_WEAPON[] = "R_HEMINE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep5-0
 * @recoil-artifact defines .data recoil:data:0x4dbfa8: g_PickupOptKey_Wep5_0.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the red high-explosive mine weapon pickup to the wep_5.0 option key.
 */
char g_PickupOptKey_Wep5_0[] = "wep_5.0";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-p-qmine-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbfb0: g_PickupLogicalName_P_QMINE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the proximity quantum mine weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_P_QMINE_WEAPON[] = "P_QMINE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep4-1
 * @recoil-artifact defines .data recoil:data:0x4dbfc0: g_PickupOptKey_Wep4_1.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the proximity quantum mine weapon pickup to the wep_4.1 option key.
 */
char g_PickupOptKey_Wep4_1[] = "wep_4.1";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-p-hemine-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbfc8: g_PickupLogicalName_P_HEMINE_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the proximity high-explosive mine weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_P_HEMINE_WEAPON[] = "P_HEMINE_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep4-0
 * @recoil-artifact defines .data recoil:data:0x4dbfd8: g_PickupOptKey_Wep4_0.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the proximity high-explosive mine weapon pickup to the wep_4.0 option key.
 */
char g_PickupOptKey_Wep4_0[] = "wep_4.0";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-napalm-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbfe0: g_PickupLogicalName_NAPALM_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the napalm weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_NAPALM_WEAPON[] = "NAPALM_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep3-1
 * @recoil-artifact defines .data recoil:data:0x4dbff0: g_PickupOptKey_Wep3_1.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the napalm weapon pickup to the wep_3.1 option key.
 */
char g_PickupOptKey_Wep3_1[] = "wep_3.1";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-freon-weapon
 * @recoil-artifact defines .data recoil:data:0x4dbff8: g_PickupLogicalName_FREON_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the freon weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_FREON_WEAPON[] = "FREON_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep3-0
 * @recoil-artifact defines .data recoil:data:0x4dc008: g_PickupOptKey_Wep3_0.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the freon weapon pickup to the wep_3.0 option key.
 */
char g_PickupOptKey_Wep3_0[] = "wep_3.0";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-qmortar-weapon
 * @recoil-artifact defines .data recoil:data:0x4dc010: g_PickupLogicalName_QMORTAR_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the quantum mortar weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_QMORTAR_WEAPON[] = "QMORTAR_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep2-1
 * @recoil-artifact defines .data recoil:data:0x4dc020: g_PickupOptKey_Wep2_1.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the quantum mortar weapon pickup to the wep_2.1 option key.
 */
char g_PickupOptKey_Wep2_1[] = "wep_2.1";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-hemortar-weapon
 * @recoil-artifact defines .data recoil:data:0x4dc028: g_PickupLogicalName_HEMORTAR_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the high-explosive mortar weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_HEMORTAR_WEAPON[] = "HEMORTAR_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep2-0
 * @recoil-artifact defines .data recoil:data:0x4dc038: g_PickupOptKey_Wep2_0.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the high-explosive mortar weapon pickup to the wep_2.0 option key.
 */
char g_PickupOptKey_Wep2_0[] = "wep_2.0";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-erfpg-weapon
 * @recoil-artifact defines .data recoil:data:0x4dc040: g_PickupLogicalName_ERFPG_WEAPON.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the ERFPG weapon pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_ERFPG_WEAPON[] = "ERFPG_WEAPON";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickupoptkey-wep1-1
 * @recoil-artifact defines .data recoil:data:0x4dc050: g_PickupOptKey_Wep1_1.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: bind the ERFPG weapon pickup to the wep_1.1 option key.
 */
char g_PickupOptKey_Wep1_1[] = "wep_1.1";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-guided-nuke-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc058: g_PickupLogicalName_GUIDED_NUKE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the guided nuke ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_GUIDED_NUKE_AMMO[] = "GUIDED_NUKE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-nuke-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc06c: g_PickupLogicalName_NUKE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the nuke ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_NUKE_AMMO[] = "NUKE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-guided-missile-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc078: g_PickupLogicalName_GUIDED_MISSILE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the guided missile ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_GUIDED_MISSILE_AMMO[] = "GUIDED_MISSILE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-missile-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc08c: g_PickupLogicalName_MISSILE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the missile ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_MISSILE_AMMO[] = "MISSILE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-arc-sabre-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc09c: g_PickupLogicalName_ARC_SABRE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the arc sabre ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_ARC_SABRE_AMMO[] = "ARC_SABRE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-sonic-cannon-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc0ac: g_PickupLogicalName_SONIC_CANNON_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the sonic cannon ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_SONIC_CANNON_AMMO[] = "SONIC_CANNON_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-laser-sabre-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc0c0: g_PickupLogicalName_LASER_SABRE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the laser sabre ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_LASER_SABRE_AMMO[] = "LASER_SABRE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-lockon-laser-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc0d4: g_PickupLogicalName_LOCKON_LASER_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the lock-on laser ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_LOCKON_LASER_AMMO[] = "LOCKON_LASER_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-r-qmine-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc0e8: g_PickupLogicalName_R_QMINE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the red quantum mine ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_R_QMINE_AMMO[] = "R_QMINE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-r-hemine-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc0f8: g_PickupLogicalName_R_HEMINE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the red high-explosive mine ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_R_HEMINE_AMMO[] = "R_HEMINE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-p-qmine-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc108: g_PickupLogicalName_P_QMINE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the proximity quantum mine ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_P_QMINE_AMMO[] = "P_QMINE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-p-hemine-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc118: g_PickupLogicalName_P_HEMINE_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the proximity high-explosive mine ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_P_HEMINE_AMMO[] = "P_HEMINE_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-napalm-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc128: g_PickupLogicalName_NAPALM_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the napalm ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_NAPALM_AMMO[] = "NAPALM_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-freon-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc134: g_PickupLogicalName_FREON_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the freon ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_FREON_AMMO[] = "FREON_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-qmortar-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc140: g_PickupLogicalName_QMORTAR_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the quantum mortar ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_QMORTAR_AMMO[] = "QMORTAR_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-hemortar-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc150: g_PickupLogicalName_HEMORTAR_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the high-explosive mortar ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_HEMORTAR_AMMO[] = "HEMORTAR_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickuplogicalname-erfpg-ammo
 * @recoil-artifact defines .data recoil:data:0x4dc160: g_PickupLogicalName_ERFPG_AMMO.
 * Data owner: battlesport_gameplay.pickup_type_name_key_literals_data.
 * Purpose: name the ERFPG ammo pickup type in g_PickupTypes.
 */
char g_PickupLogicalName_ERFPG_AMMO[] = "ERFPG_AMMO";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickup-scenenode
 * @recoil-artifact defines .data recoil:data:0x4f3328: g_Pickup_SceneNode (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: cache the scene node used as the parent and terrain query root for pickups.
 */
zClass_NodePartial *g_Pickup_SceneNode = 0;

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickup-globalairdropspawnref
 * @recoil-artifact defines .data recoil:data:0x4f4210: g_Pickup_GlobalAirdropSpawnRef (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: hold the optional global airdrop spawn reference allocated during pickup setup.
 */
PickupAirdropSpawnRef *g_Pickup_GlobalAirdropSpawnRef = 0;
}

struct PickupArchiveRecord {
    int firstRecord;
    int typeIndex;
    int pickupId;
    int amount;
    zVec3 position;
    zVec3 rotation;
    int spawnParam;
    float respawnDelay;
};

RECOIL_STATIC_ASSERT(
    offsetof(
        PickupArchiveRecord,
        typeIndex
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupArchiveRecord,
        pickupId
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupArchiveRecord,
        amount
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupArchiveRecord,
        position
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupArchiveRecord,
        rotation
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupArchiveRecord,
        spawnParam
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        PickupArchiveRecord,
        respawnDelay
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(sizeof(PickupArchiveRecord) == 0x30);

namespace {
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kstatuspickupfullthreshold
 * @recoil-artifact defines .rdata recoil:data:0x4d05bc: kStatusPickupFullThreshold (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: preserve the full-status threshold used before applying health pickup recovery.
 */
const float kStatusPickupFullThreshold = 0.99000001f;
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupaltammodisabledsentinel
 * @recoil-artifact defines .rdata recoil:data:0x4d178c: kPickupAltAmmoDisabledSentinel (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: preserve the alt-weapon ammo sentinel that disables VTOL airdrop pickup spawning.
 */
const float kPickupAltAmmoDisabledSentinel = 123456792.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickuparchivesectionname
 * @recoil-artifact defines .data recoil:data:0x4dc16c: kPickupArchiveSectionName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the pickup archive section registered with the ZAR handler table.
 */
const char kPickupArchiveSectionName[] = "Pickup";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupconfigimagekey
 * @recoil-artifact defines .data recoil:data:0x4dc174: kPickupConfigImageKey (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the pickup config IMAGE key used to load optional metadata images.
 */
const char kPickupConfigImageKey[] = "IMAGE";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupconfigdatanodename
 * @recoil-artifact defines .data recoil:data:0x4dc184: kPickupConfigDataNodeName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the PICKUP_DATA node in the pickup config tree.
 */
const char kPickupConfigDataNodeName[] = "PICKUP_DATA";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupsourcefilepath
 * @recoil-artifact defines .data recoil:data:0x4dc190: kPickupSourceFilePath (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: preserve the source path emitted in pickup zError reports.
 */
const char kPickupSourceFilePath[] = "D:\\Proj\\Battlesport\\pickup.cpp";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickuptemplatenameformat
 * @recoil-artifact defines .data recoil:data:0x4dc1b0: kPickupTemplateNameFormat (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: format pickup template and pickup-removal lookup node names.
 */
const char kPickupTemplateNameFormat[] = "pu%03d";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupdefaultsoundname
 * @recoil-artifact defines .data recoil:data:0x4dc1b8: kPickupDefaultSoundName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the default pickup sound sample.
 */
const char kPickupDefaultSoundName[] = "snd_pickup";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupunhandledtypeformat
 * @recoil-artifact defines .data recoil:data:0x4dc1c4: kPickupUnhandledTypeFormat (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: format pickup-type error reports when effect dispatch has no handler.
 */
const char kPickupUnhandledTypeFormat[] = "Unhandled Pickup Type: %d";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupinstancenameformat
 * @recoil-artifact defines .data recoil:data:0x4dc1e0: kPickupInstanceNameFormat (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: format cloned pickup object names from type and instance suffix.
 */
const char kPickupInstanceNameFormat[] = "pu%03d%02d";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupmissingbvolformat
 * @recoil-artifact defines .data recoil:data:0x4dc1ec: kPickupMissingBvolFormat (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: format the error for pickup nodes missing the required bvol child.
 */
const char kPickupMissingBvolFormat[] = "Pickup: (%s) has no bvol child node";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupbvolnodename
 * @recoil-artifact defines .data recoil:data:0x4dc210: kPickupBvolNodeName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the pickup collision bvol child node.
 */
const char kPickupBvolNodeName[] = "bvol";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupairdropattachnodename
 * @recoil-artifact defines .data recoil:data:0x4dc218: kPickupAirdropAttachNodeName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the chute node that receives an airdropped pickup child.
 */
const char kPickupAirdropAttachNodeName[] = "airdroppup";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickupchuteeffectname
 * @recoil-artifact defines .data recoil:data:0x4dc224: kPickupChuteEffectName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the airdrop chute effect used while spawning pickups.
 */
const char kPickupChuteEffectName[] = "chutes";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickuppuppieseasyzrd
 * @recoil-artifact defines .data recoil:data:0x4dc22c: kPickupPuppiesEasyZrd (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the easy-difficulty puppy spawn ZRD; this starts the
 * 0x4dc22c..0x4dc25f consecutive puppy ZRD filename data owner.
 */
const char kPickupPuppiesEasyZrd[] = "puppies_easy.zrd";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickuppuppieshardzrd
 * @recoil-artifact defines .data recoil:data:0x4dc240: kPickupPuppiesHardZrd (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the hard-difficulty puppy spawn ZRD.
 */
const char kPickupPuppiesHardZrd[] = "puppies_hard.zrd";
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.kpickuppuppiesdefaultzrd
 * @recoil-artifact defines .data recoil:data:0x4dc254: kPickupPuppiesDefaultZrd (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the default puppy spawn ZRD fallback.
 */
const char kPickupPuppiesDefaultZrd[] = "puppies.zrd";

} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.g-pickup-nodeprefix
 * @recoil-artifact defines .data recoil:data:0x4dc260: g_Pickup_NodePrefix (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: name the pickup scene-node type prefix used while enumerating puppy spawns.
 */
char g_Pickup_NodePrefix[3] = "pu";

PickupPkt11Delta g_PickupPkt11Flag2Delta = {{0x11, sizeof(PickupPkt11Delta), 0}, 0, 0, 0};
PickupPkt11Delta g_PickupPkt11Flag8Delta = {{0x11, sizeof(PickupPkt11Delta), 0}, 0, 0, 0};
PickupPkt12AirdropSpawnChuteRelay g_PickupPkt12AirdropSpawnChuteRelay =
    {{0x12, sizeof(PickupPkt12AirdropSpawnChuteRelay), 0}, {0.0f, 0.0f, 0.0f}, 0, 0, 0};

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupspawnlist-primary-init
 * @recoil-artifact defines .text recoil:function:0x41cc10: PickupSpawnList::Primary_Init (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: clear the primary pickup spawn list global.
 */
void __cdecl PickupSpawnList::Primary_Init() {
    g_PickupSpawnList_Primary.unused = 0;
    g_PickupSpawnList_Primary.tail = 0;
    g_PickupSpawnList_Primary.head = 0;
    g_PickupSpawnList_Primary.count = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupspawnlist-netcopy-init
 * @recoil-artifact defines .text recoil:function:0x41cc40: PickupSpawnList::NetCopy_Init (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: clear the network-copy pickup spawn list global.
 */
void __cdecl PickupSpawnList::NetCopy_Init() {
    g_PickupSpawnList_NetworkCopy.unused = 0;
    g_PickupSpawnList_NetworkCopy.tail = 0;
    g_PickupSpawnList_NetworkCopy.head = 0;
    g_PickupSpawnList_NetworkCopy.count = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickuprespawnqueue-init
 * @recoil-artifact defines .text recoil:function:0x41cc70: PickupRespawnQueue::Init (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: clear the pickup respawn queue global.
 */
void __cdecl PickupRespawnQueue::Init() {
    g_PickupRespawnQueue.unused = 0;
    g_PickupRespawnQueue.tail = 0;
    g_PickupRespawnQueue.head = 0;
    g_PickupRespawnQueue.count = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickuptypetable-freeoptmeta
 * @recoil-artifact defines .text recoil:function:0x41cca0: PickupTypeTable::FreeOptMeta (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: release optional pickup metadata images and clear their table slots.
 */
void __cdecl PickupTypeTable::FreeOptMeta() {
    {
        for (int index = 0; index < 40; ++index) {
            PickupType &pickupType = g_PickupTypes[index];
            zVidImagePartial *const image = pickupType.optMetaImage;
            if (image != 0) {
                zVid_Image::ReleaseIfNotDefault(image);
                pickupType.optMetaImage = 0;
            }
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-shutdown
 * @recoil-artifact defines .text recoil:function:0x41ccd0: Pickup::Shutdown (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: clear pickup spawn, network-copy, and respawn queue state.
 */
void __cdecl Pickup::Shutdown() {
    g_PickupSpawnList_Primary.Clear();
    g_PickupSpawnList_NetworkCopy.Clear();
    g_PickupRespawnQueue.ClearAndFree();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-init
 * @recoil-artifact defines .text recoil:function:0x41ccf0: Pickup::Init (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: initialize pickup templates, metadata images, sounds, and archive handlers.
 */
int __fastcall Pickup::Init(
    zClass_NodePartial *sceneNode,
    const char *pickupsCfgPath
) {
    g_Pickup_SceneNode = sceneNode;

    zSndSample *const defaultPickupSound = zSnd::FindSampleByName(kPickupDefaultSoundName);
    for (int index = 0; index < 40; ++index) {
        PickupType &pickupType = g_PickupTypes[index];

        char templateName[0x28];
        sprintf(
            templateName,
            kPickupTemplateNameFormat,
            pickupType.typeIndex
        );
        pickupType.templateNode = zClass::FindByTypeAndName(
            6,
            templateName
        );
        pickupType.pickupSound = defaultPickupSound;
        pickupType.nameSuffixMax = 0;

        zClass_NodePartial *const templateNode = pickupType.templateNode;
        if (templateNode != 0) {
            PickupNodeRuntimeFields *const fields =
                (PickupNodeRuntimeFields *)(templateNode->name);
            fields->pickupId = 0;
            fields->pickupTypeIndex = pickupType.typeIndex;
            fields->amount = pickupType.defaultAmount;
        }
    }

    zReader::Node *const rootNode = zReader::LoadNodeFromPath(
        pickupsCfgPath,
        0,
        0
    );
    if (rootNode == 0) {
        zError::ReportOld(
            0x200,
            kPickupSourceFilePath,
            0xc1,
            g_HudSensorTracker_ReadFileFailedFmt,
            pickupsCfgPath
        );
        return 0;
    }

    zReader::Node *const pickupDataNode = zReader_GetNamedNode(
        rootNode,
        kPickupConfigDataNodeName
    );
    if (pickupDataNode != 0) {
        zReader::Node *const pickupData = pickupDataNode->value.nodes;
        const int pickupDataCount = pickupData[0].value.i32;
        for (int fieldIndex = 1; fieldIndex < pickupDataCount; fieldIndex += 2) {
            int pickupTypeIndex = 0;
            const char *const logicalName = pickupData[fieldIndex].value.str;
            if (PickupType::FindByLogicalName(
                logicalName,
                &pickupTypeIndex
            ) == 0) {
                continue;
            }

            PickupType &pickupType = g_PickupTypes[pickupTypeIndex];
            zReader::Node *const entryNode = zReader_GetNamedNode(
                pickupDataNode,
                logicalName
            );
            zReader::Node *const soundNode = zReader_GetNamedNode(
                entryNode,
                g_HudZrd_Key_Sound
            );
            zReader::Node *const imageNode = zReader_GetNamedNode(
                entryNode,
                kPickupConfigImageKey
            );
            if (soundNode != 0) {
                zSndSample *const pickupSound =
                    zSnd::FindSampleByName(soundNode->value.nodes[1].value.str);
                if (pickupSound != 0) {
                    pickupType.pickupSound = pickupSound;
                } else {
                    pickupType.pickupSound = defaultPickupSound;
                }
            }

            if (imageNode != 0 && pickupType.optMetaImage == 0) {
                pickupType.optMetaImage =
                    zImage::TexDir_FindOrCreateByPath(imageNode->value.nodes[1].value.str);
            }
        }
    }

    zReader::FreeLoadedTree(rootNode);
    zUtil_ZAR::RegisterSectionHandler(
        kPickupArchiveSectionName,
        (zZbdSectionCallback)(&ArchiveWriteAll),
        (zZbdSectionCallback)(&ArchiveReadRecord),
        300,
        0
    );
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.zclass-node-clearpickupflagsrecursive
 * @recoil-artifact defines .text recoil:function:0x41ceb0: zClass_Node::ClearPickupFlagsRecursive (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: clear pickup interaction flags from a node and each child in its
 * secondary child list.
 */
int __fastcall zClass_Node::ClearPickupFlagsRecursive(
    zClass_NodePartial *node
) {
    node->flags &= ~0x40018;

    for (int i = 0; i < node->listCountB; ++i) {
        ClearPickupFlagsRecursive(node->listB[i]);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.zclass-node-setpickupflagsrecursive
 * @recoil-artifact defines .text recoil:function:0x41cef0: zClass_Node::SetPickupFlagsRecursive (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: restore pickup active/raycast flags on a node and its secondary children.
 */
int __fastcall zClass_Node::SetPickupFlagsRecursive(
    zClass_NodePartial *node
) {
    node->flags = (node->flags & ~0x08) | 0x40010;

    for (int i = 0; i < node->listCountB; ++i) {
        SetPickupFlagsRecursive(node->listB[i]);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-resolveownerfrombvolhit
 * @recoil-artifact defines .text recoil:function:0x41cf30: Pickup::ResolveOwnerFromBvolHit (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: resolve a pickup bvol hit node back to its owning pickup node.
 */
int __fastcall Pickup::ResolveOwnerFromBvolHit(
    zClass_NodePartial **nodeInOut
) {
    zClass_NodePartial *const node = *nodeInOut;
    if (node == 0 || (node->flags & 0x40000) == 0) {
        return 0;
    }

    PickupBvolHitCallbackContext *const context =
        (PickupBvolHitCallbackContext *)(node->callbackContext);
    *nodeInOut = context->ownerNode;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-removeobject
 * @recoil-artifact defines .text recoil:function:0x41cf50: Pickup::RemoveObject (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: deactivate a pickup object and return its spawn to respawn timing.
 */
void __fastcall Pickup::RemoveObject(
    zEffectAnimEntry *animEntry,
    zClass_NodePartial *pickupObj,
    int
) {
    PickupSpawnDef *const spawn = (PickupSpawnDef *)(pickupObj->callbackContext);
    zClass_Class::gwNodeSetActive(
        pickupObj,
        0
    );

    if (spawn != 0 && spawn->respawnDelay != 0.0f) {
        if (zOpt::GetNetworkEnabled() != 0 && animEntry != 0) {
            SendPkt11_Flag8Delta(spawn);
        }

        PickupRespawnEntry *const respawnEntry =
            (PickupRespawnEntry *)(::operator new(sizeof(PickupRespawnEntry)));
        respawnEntry->spawn = 0;
        respawnEntry->when = 0.0f;
        respawnEntry->next = 0;

        if (g_PickupRespawnQueue.count == 0) {
            g_PickupRespawnQueue.head = respawnEntry;
        } else {
            g_PickupRespawnQueue.tail->next = respawnEntry;
        }

        g_PickupRespawnQueue.tail = respawnEntry;
        respawnEntry->next = 0;
        ++g_PickupRespawnQueue.count;
        respawnEntry->spawn = spawn;
        respawnEntry->when = g_Time_UnscaledAccumulatedTimeSec + spawn->respawnDelay;
        return;
    }

    if (pickupObj->listCountA == 1 && pickupObj->listA[0] != 0) {
        if (zOpt::GetNetworkEnabled() != 0 && spawn != 0) {
            SendPkt11_Flag2Delta(spawn);
        }

        zClass_Class::RemoveChild(
            pickupObj->listA[0],
            pickupObj
        );
    }

    if (spawn != 0 && g_PickupSpawnList_Primary.count != 0) {
        PickupSpawnDef *current = g_PickupSpawnList_Primary.head;
        if (spawn == current) {
            --g_PickupSpawnList_Primary.count;
            PickupSpawnDef *const next = spawn->next;
            g_PickupSpawnList_Primary.head = next;
            if (next == 0) {
                g_PickupSpawnList_Primary.unused = 0;
                g_PickupSpawnList_Primary.tail = 0;
                free(current);
                return;
            }

            free(current);
            return;
        }

        if (current != 0) {
            while (current->next != spawn) {
                current = current->next;
                if (current == 0) {
                    free(spawn);
                    return;
                }
            }

            --g_PickupSpawnList_Primary.count;
            current->next = spawn->next;
            if (g_PickupSpawnList_Primary.tail == spawn) {
                g_PickupSpawnList_Primary.tail = current;
            }
        }
    }

    free(spawn);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-oncollected
 * @recoil-artifact defines .text recoil:function:0x41d0c0: Pickup::OnCollected (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: apply a collected pickup and transition the pickup object to its pickup animation or removal.
 */
int __fastcall Pickup::OnCollected(
    zClass_NodePartial *hitNode,
    zUtil_SaveGameState *saveState
) {
    zClass_NodePartial *pickupObj = hitNode;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (ResolveOwnerFromBvolHit(&pickupObj) == 0) {
        return 0;
    }

    const int pickupTypeId =
        ((PickupNodeRuntimeFields *)(pickupObj->name))->pickupTypeIndex;
    PickupSpawnDef *spawn = GetSpawnDefFromNode(pickupObj);
    if (spawn == 0 || ApplyEffect(
        pickupTypeId,
        spawn->amount,
        saveState
    ) == 0) {
        return 0;
    }

    zClass_Node::ClearPickupFlagsRecursive(pickupObj);
    zClass_Class::gwNodeSetRaycastable(
        pickupObj,
        0
    );
    zClass_Class::gwNodeSetPickable(
        pickupObj,
        0
    );

    PickupType *const pickupType = &g_PickupTypes[pickupTypeId];
    if (pickupType->weaponKeyName != 0) {
        g_HudSensorTracker.ShowObjectivePickupInfo(
            1,
            1,
            pickupType->optEntry
        );
        if (zOpt::GetNetworkEnabled() == 0) {
            RemoveOtherSpawnsWithSameOptEntry(
                pickupType->optEntry,
                pickupObj
            );
        }
    }

    char pickupAnimName[8];
    sprintf(
        pickupAnimName,
        kPickupTemplateNameFormat,
        pickupTypeId
    );
    zEffectAnimEntry *const animEntry = zEffectAnim::FindEntryByName(pickupAnimName);
    if (animEntry != 0) {
        zVec3 worldPosition;
        gwNode::GetWorldPosition(
            pickupObj,
            &worldPosition
        );
        pickupType->pickupSound->PlayA3DSimple(1.0f);
        zClass_Class::gwNodeSetName(
            pickupObj,
            pickupAnimName
        );
        zEffectAnimEntry *const runtimeEntry = zEffectAnim::SetTransformRefs_Thunk(
            animEntry,
            pickupObj,
            pickupObj,
            &worldPosition,
            playerState->rootNode,
            0
        );
        zEffectAnimEntry::SetOnStateDoneCallback(
            runtimeEntry,
            (void *)RemoveObject,
            pickupObj
        );
        GetSpawnDefFromNode(pickupObj)->refCount = 1;
        return 1;
    }

    RemoveObject(
        0,
        pickupObj,
        0
    );
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-applyeffect
 * @recoil-artifact defines .text recoil:function:0x41d220: Pickup::ApplyEffect (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: apply the gameplay effect for a pickup type to the player save state.
 */
int __fastcall Pickup::ApplyEffect(
    int pickupTypeId,
    int overrideAmount,
    zUtil_SaveGameState *saveState
) {
    const float kUnlimitedAmmoSentinel = 123456792.0f;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;
    int result = 1;
    char message[64];

    if (pickupTypeId >= 0 && pickupTypeId <= 0x27) {
        const int normalizedWeaponId =
            pickupTypeId >= 0x11 && pickupTypeId <= 0x21 ? pickupTypeId - 0x11 : pickupTypeId;
        if (normalizedWeaponId >= 0 && normalizedWeaponId <= 0x10) {
            int weaponBankIndex;
            int weaponSideIndex;
            if (normalizedWeaponId == 0) {
                weaponBankIndex = 1;
                weaponSideIndex = 1;
            } else {
                weaponBankIndex = (normalizedWeaponId + 3) / 2;
                weaponSideIndex = (normalizedWeaponId + 1) & 1;
            }

            result = GrantAmmoOrWeapon(
                &g_PickupTypes[pickupTypeId],
                message,
                saveState,
                weaponBankIndex,
                weaponSideIndex,
                weaponSideIndex == 0 ? 1 : 0,
                overrideAmount
            );
        } else {
            switch (pickupTypeId) {
            case 0x24:
                zLoc::FormatMessage(
                    message,
                    sizeof(message),
                    0x20d
                );
                if (playerState->nanitePanelLevel < 3) {
                    ++playerState->nanitePanelLevel;
                    HudUiMgr::SetNanitePanelCount(playerState->nanitePanelLevel);
                } else {
                    result = 0;
                }
                break;

            case 0x22:
            case 0x23:
                {
                int statusAmount = overrideAmount;
                if (pickupTypeId == 0x23) {
                    statusAmount = (int)(masterCommonData->maxHealth);
                }
                zLoc::FormatMessage(
                    message,
                    sizeof(message),
                    g_PickupTypes[pickupTypeId].msgIdOrClassId
                );
                if (masterCommonData->invMaxHealth * playerState->statusMeterValue >
                    kStatusPickupFullThreshold) {
                    result = 0;
                } else {
                    result = Player::UpdateStatusMeter(
                        saveState,
                        1,
                        (float)(statusAmount)
                    );
                }
                break;
                }

            case 0x25:
                zLoc::FormatMessage(
                    message,
                    sizeof(message),
                    0x241
                );
                HudUiMgr::SetModeCounterState(
                    1,
                    1
                );
                playerState->amphibUnlocked = 1;
                break;

            case 0x26:
                zLoc::FormatMessage(
                    message,
                    sizeof(message),
                    0x242
                );
                HudUiMgr::SetModeCounterState(
                    2,
                    1
                );
                playerState->hoverUnlocked = 1;
                break;

            case 0x27:
                zLoc::FormatMessage(
                    message,
                    sizeof(message),
                    0x245
                );
                HudUiMgr::SetModeCounterState(
                    3,
                    1
                );
                playerState->subUnlocked = 1;
                break;

            default:
                zError::ReportOld(
                    0x200,
                    kPickupSourceFilePath,
                    0x370,
                    kPickupUnhandledTypeFormat,
                    pickupTypeId
                );
                return 0;
            }
        }
    } else if (pickupTypeId == 0x385) {
        zLoc::FormatMessage(
            message,
            sizeof(message),
            0x243
        );
        HudUiMessage::ClearDisplay(playerState->activeAltGunController->weaponBankIndex);
        HudUiMessage::ClearDisplay(playerState->activePrimaryGunController->weaponBankIndex);

        for (int bankIndex = 1; bankIndex < 10; ++bankIndex) {
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            bank.controllerA.ammoOrCharge = kUnlimitedAmmoSentinel;
            bank.controllerA.flags |= 4;
            bank.controllerB.ammoOrCharge = kUnlimitedAmmoSentinel;
            bank.controllerB.flags |= 4;
            bank.selectedSide = 0;
            HudUiMessage::ApplySideImageSwap(
                bankIndex,
                1
            );
            HudUiMessage::SetValueIfOwnerMatches(
                bankIndex,
                0,
                kUnlimitedAmmoSentinel
            );
            HudUiMessage::SelectVariantDisplay(
                bankIndex,
                0
            );
        }

        zUtil_PlayerStateStorage *const displayPlayerState =
            (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
        PlayerGunFireController *activeController = displayPlayerState->activeAltGunController;
        HudUiMessage::UpdateSelectedWeaponDisplay(
            activeController->weaponBankIndex,
            activeController->weaponSideIndex,
            activeController->ammoOrCharge
        );
        activeController = displayPlayerState->activePrimaryGunController;
        HudUiMessage::UpdateSelectedWeaponDisplay(
            activeController->weaponBankIndex,
            activeController->weaponSideIndex,
            activeController->ammoOrCharge
        );
    } else if (pickupTypeId != 0x386 && pickupTypeId != 0x387) {
        zError::ReportOld(
            0x200,
            kPickupSourceFilePath,
            0x370,
            kPickupUnhandledTypeFormat,
            pickupTypeId
        );
        return 0;
    } else if (pickupTypeId == 0x387) {
        zLoc::FormatMessage(
            message,
            sizeof(message),
            0x247
        );
        HudUiMgr::SetNanitePanelCount(3);
        playerState->nanitePanelLevel = 123456789;
    } else {
        sprintf(
            message,
            zLoc::GetMessageString(0x20d)
        );
        Player::UpdateStatusMeter(
            saveState,
            0,
            0.0f
        );
    }

    HudUi::ShowTopMessageLine(
        message,
        5.0f
    );
    return result;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-grantammoorweapon
 * @recoil-artifact defines .text recoil:function:0x41d650: Pickup::GrantAmmoOrWeapon (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: grant ammo or weapon ownership for a pickup and prepare HUD feedback.
 */
int __fastcall Pickup::GrantAmmoOrWeapon(
    PickupType *pickupType,
    char *messageBuffer,
    zUtil_SaveGameState *saveState,
    int weaponBankIndex,
    int weaponSideIndex,
    int pairedWeaponSideIndex,
    int overrideAmount
) {
    const float kUnlimitedAmmoSentinel = 123456792.0f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerAltWeaponBank *const bank = &playerState->altWeaponBanks[weaponBankIndex];
    PlayerGunFireController *const controller =
        weaponSideIndex == 0 ? &bank->controllerA : &bank->controllerB;
    PlayerGunFireController *const pairedController =
        pairedWeaponSideIndex == 0 ? &bank->controllerA : &bank->controllerB;

    const float maxAmount = controller->optCatalogEntry->ammoOrChargeMax;
    if (controller->ammoOrCharge != kUnlimitedAmmoSentinel &&
        controller->ammoOrCharge >= maxAmount && pickupType->weaponKeyName == 0) {
        zLoc::FormatMessage(
            messageBuffer,
            64,
            0x237,
            zLoc::GetMessageString(pickupType->msgIdOrClassId)
        );
        return 0;
    }

    if (overrideAmount == 0) {
        overrideAmount = pickupType->defaultAmount;
    }

    int updateValueText = 1;
    if (pickupType->optEntry != 0) {
        if ((controller->flags & 4) == 0) {
            controller->flags |= 4;
            ++g_HudSensorTracker.primaryGunDispatchCount;
            if ((pairedController->flags & 4) != 0 && pairedController->ammoOrCharge != 0.0f) {
                HudUiMessage::ApplySideImageSwap(
                    weaponBankIndex,
                    weaponSideIndex
                );
                updateValueText = 0;
            } else {
                HudUiMessage::SelectVariantDisplay(
                    weaponBankIndex,
                    weaponSideIndex
                );
                bank->selectedSide = weaponSideIndex;
            }
        }
    } else if (controller->ammoOrCharge == 0.0f) {
        if (playerState->activeAltGunController == controller) {
            HudUiMessage::SelectVariantDisplay(
                controller->weaponBankIndex,
                controller->weaponSideIndex + 3
            );
        }

        if (pairedController->ammoOrCharge == 0.0f) {
            bank->selectedSide = weaponSideIndex;
        }
    }

    if (controller->ammoOrCharge != kUnlimitedAmmoSentinel) {
        controller->ammoOrCharge += (float)(overrideAmount);
        if (controller->ammoOrCharge > maxAmount) {
            controller->ammoOrCharge = maxAmount;
        }

        if (updateValueText != 0) {
            HudUiMessage::SetValueIfOwnerMatches(
                weaponBankIndex,
                weaponSideIndex,
                controller->ammoOrCharge
            );
        }
    }

    if (overrideAmount == 1) {
        zLoc::FormatMessage(
            messageBuffer,
            64,
            0x23f,
            zLoc::GetMessageString(pickupType->msgIdOrClassId)
        );
    } else {
        zLoc::FormatMessage(
            messageBuffer,
            64,
            0x240,
            overrideAmount,
            zLoc::GetMessageString(pickupType->msgIdOrClassId)
        );
    }

    if (pickupType->weaponKeyName != 0) {
        if (weaponBankIndex == 1) {
            Player::HandlePrimaryWeaponVariantToggleInput(weaponBankIndex);
            return weaponBankIndex;
        }

        if (playerState->activeAltGunController == pairedController) {
            Player::HandleAltWeaponBankSelectInput(controller->weaponBankIndex + 14);
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupspawnlist-removeandfreenode
 * @recoil-artifact defines .text recoil:function:0x41d8a0: PickupSpawnList::RemoveAndFreeNode (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: unlink a pickup spawn from a spawn list and release its node.
 */
void __fastcall PickupSpawnList::RemoveAndFreeNode(
    PickupSpawnDef *node,
    PickupSpawnList *list
) {
    if (node != 0 && list->count != 0) {
        PickupSpawnDef *current = list->head;
        if (node == current) {
            --list->count;
            PickupSpawnDef *const next = node->next;
            list->head = next;
            if (next == 0) {
                list->unused = 0;
                list->tail = 0;
            }
        } else {
            while (current != 0) {
                PickupSpawnDef *const next = current->next;
                if (next == node) {
                    --list->count;
                    current->next = node->next;
                    if (list->tail == node) {
                        list->tail = current;
                    }
                    break;
                }
                current = next;
            }
        }
    }

    zClass_NodePartial *const pickupObj = node->pickupObj;
    if (pickupObj != 0) {
        if (pickupObj->listA != 0) {
            zClass_World::RemoveChildAtGrid(
                pickupObj->listA[0],
                pickupObj
            );
        }

        zClass_Util::DestroyNodeRecursive(pickupObj);
    }

    free(node);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-createspawndefandlink
 * @recoil-artifact defines .text recoil:function:0x41d920: Pickup::CreateSpawnDefAndLink (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: allocate a spawn definition, attach it to the primary list, and bind node context.
 */
PickupSpawnDef *__fastcall Pickup::CreateSpawnDefAndLink(
    zClass_NodePartial *pickupObj,
    zVec3 *position,
    zVec3 *rotation,
    int spawnParam,
    int linkToScene
) {
    if (linkToScene != 0) {
        zClass_Class::AddChild(
            g_Pickup_SceneNode,
            pickupObj
        );
    }

    PickupSpawnDef *const spawn = (PickupSpawnDef *)(malloc(sizeof(PickupSpawnDef)));

    PickupType *pickupType = 0;
    const PickupNodeRuntimeFields *const pickupFields =
        (const PickupNodeRuntimeFields *)(pickupObj->name);
    const int pickupTypeIndex = pickupFields->pickupTypeIndex;
    for (int index = 0; index < 40; ++index) {
        if (g_PickupTypes[index].typeIndex == pickupTypeIndex) {
            pickupType = &g_PickupTypes[index];
            break;
        }
    }

    spawn->pickupId = pickupFields->pickupId;
    spawn->pickupType = pickupType;
    spawn->amount = pickupFields->amount;
    spawn->position = *position;
    if (rotation != 0) {
        spawn->rotation = *rotation;
    } else {
        spawn->rotation.x = 0.0f;
        spawn->rotation.y = 0.0f;
        spawn->rotation.z = 0.0f;
    }

    spawn->pickupObj = pickupObj;
    spawn->spawnParam = spawnParam;
    spawn->refCount = 0;
    spawn->respawnDelay = 0.0f;
    spawn->next = 0;

    if (g_PickupSpawnList_Primary.count == 0) {
        g_PickupSpawnList_Primary.head = spawn;
    } else {
        g_PickupSpawnList_Primary.tail->next = spawn;
    }
    g_PickupSpawnList_Primary.tail = spawn;
    spawn->next = 0;
    ++g_PickupSpawnList_Primary.count;

    zClass_Node::SetContextRecursive(
        pickupObj,
        (zClass_NodePartial *)spawn,
        0x240000
    );
    return spawn;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-spawnat
 * @recoil-artifact defines .text recoil:function:0x41da20: Pickup::SpawnAt (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: create a pickup object at a supplied transform and link its spawn definition.
 */
PickupSpawnDef *__fastcall Pickup::SpawnAt(
    int typeIndex,
    int amount,
    zVec3 *position,
    zVec3 *rotation,
    int spawnParam
) {
    zClass_NodePartial *const pickupObj = CreateObjectInstance(
        typeIndex,
        amount
    );
    if (pickupObj == 0) {
        return 0;
    }

    SetVariantFromTerrain(
        pickupObj,
        position
    );
    zClass_Object3D::gwObject3DSetPosition(
        pickupObj,
        position->x,
        position->y,
        position->z
    );

    if (rotation != 0) {
        zClass_Object3D::gwObject3DSetRotation(
            pickupObj,
            rotation->x,
            rotation->y,
            rotation->z
        );
    }

    PickupSpawnDef *const spawn =
        CreateSpawnDefAndLink(
            pickupObj,
            position,
            rotation,
            spawnParam,
            1
        );
    strcpy(
        spawn->name,
        pickupObj->name
    );
    return spawn;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-createobjectinstance
 * @recoil-artifact defines .text recoil:function:0x41dab0: Pickup::CreateObjectInstance (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: clone a pickup template node, assign its runtime fields, and name it for spawning.
 */
zClass_NodePartial *__fastcall Pickup::CreateObjectInstance(
    int typeIndex,
    int overrideAmount
) {
    PickupType *const pickupType = PickupType::GetByIndex_Pure(typeIndex);
    if (pickupType == 0 || pickupType->templateNode == 0) {
        return 0;
    }

    zClass_NodePartial *const pickupObj =
        zClass_cls_util::CopyNodeWithCloneOptions(
            pickupType->templateNode,
            1,
            0
        );
    if (pickupObj == 0) {
        return 0;
    }

    char pickupName[0x40];
    sprintf(
        pickupName,
        kPickupInstanceNameFormat,
        pickupType->typeIndex,
        pickupType->nameSuffixMax
    );
    zClass_Class::gwNodeSetName(
        pickupObj,
        pickupName
    );

    int amount = overrideAmount;
    if (amount == 0) {
        amount = pickupType->defaultAmount;
    }
    ((PickupNodeRuntimeFields *)(pickupObj->name))->amount = amount;

    if (AssignBvolGroupAndId(pickupObj) != 0) {
        return pickupObj;
    }

    zClass_Util::DestroyNodeRecursive(pickupObj);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickuptype-getbyindex-pure
 * @recoil-artifact defines .text recoil:function:0x41db40: PickupType::GetByIndex_Pure (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: return the pickup type record when the index is below the table size.
 */
PickupType *__fastcall PickupType::GetByIndex_Pure(
    int pickupTypeIndex
) {
    if (pickupTypeIndex < 40) {
        return &g_PickupTypes[pickupTypeIndex];
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-assignbvolgroupandid
 * @recoil-artifact defines .text recoil:function:0x41db60: Pickup::AssignBvolGroupAndId (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: parse a pickup node name, assign runtime pickup fields, and disable its bvol child.
 */
int __fastcall Pickup::AssignBvolGroupAndId(
    zClass_NodePartial *pickupObj
) {
    zClass_NodePartial *const bvolNode = zClass_Class::FindSubNodeByName(
        pickupObj,
        kPickupBvolNodeName
    );
    if (bvolNode == 0) {
        zError::ReportOld(
            0x400,
            kPickupSourceFilePath,
            0x152,
            kPickupMissingBvolFormat,
            pickupObj
        );
        return 0;
    }

    const int parsedSuffix = atol(pickupObj->name + 2);
    const int pickupTypeIndex = parsedSuffix / 100;
    const int pickupNameSuffix = parsedSuffix % 100;
    if (pickupTypeIndex > 40) {
        return 0;
    }

    PickupNodeRuntimeFields *const fields =
        (PickupNodeRuntimeFields *)(pickupObj->name);
    fields->pickupTypeIndex = pickupTypeIndex;
    fields->pickupId = g_NextPickupId;
    ++g_NextPickupId;

    PickupType *const pickupType = PickupType::GetByIndex_Pure(pickupTypeIndex);
    if (pickupType != 0 && pickupNameSuffix + 1 > pickupType->nameSuffixMax) {
        pickupType->nameSuffixMax = pickupNameSuffix + 1;
    }

    pickupObj->flags = (pickupObj->flags & ~0x08) | 0x20;
    zClass_Class::gwNodeSetActive(
        bvolNode,
        0
    );
    zClass_Node::SetDiFlagBit0Recursive(
        pickupObj,
        0
    );
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-spawnfromparsedzrdentry
 * @recoil-artifact defines .text recoil:function:0x41dc30: Pickup::SpawnFromParsedZrdEntry (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: create a pickup spawn from parsed ZRD pickup placement data.
 */
PickupSpawnDef *__fastcall Pickup::SpawnFromParsedZrdEntry(
    PickupParsedZrdEntry *entry
) {
    PickupSpawnDef *const spawn = SpawnAt(
        entry->typeDesc->typeIndex,
        entry->amount,
        &entry->position,
        &entry->rotation,
        entry->param
    );
    if (spawn != 0) {
        spawn->respawnDelay = entry->respawnDelay;
    }
    return spawn;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-spawnwithairdropchute
 * @recoil-artifact defines .text recoil:function:0x41dc60: Pickup::SpawnWithAirdropChute (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: create a pickup object at an airdrop position and attach its chute
 * animation state.
 */
int __fastcall Pickup::SpawnWithAirdropChute(
    int typeIndex,
    zVec3 *position
) {
    zClass_NodePartial *const pickupObj = CreateObjectInstance(
        typeIndex,
        0
    );
    if (pickupObj == 0) {
        return 0;
    }

    SetVariantFromTerrain(
        pickupObj,
        position
    );

    zEffectAnimEntry *const chuteTemplate = zEffectAnim::FindEntryByName(kPickupChuteEffectName);
    zEffectAnimEntry *const chuteEntry = zEffectAnim::SetTransformRotAndVelocity_Thunk(
        chuteTemplate,
        0,
        position->x,
        position->y,
        position->z,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        0.0f
    );
    zClass_NodePartial *const chuteRoot = zEffectAnim::GetRootNodeOrNull(chuteEntry);
    zClass_NodePartial *const attachNode = zClass_Class::FindSubNodeByName(
        chuteRoot,
        kPickupAirdropAttachNodeName
    );
    zClass_Class::AddChild(
        attachNode,
        pickupObj
    );
    zClass_Class::gwNodeSetActive(
        pickupObj,
        0
    );
    zEffectAnimEntry::SetOnStateDoneCallback(
        chuteEntry,
        (void *)&RegisterExistingObject,
        pickupObj
    );
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-registerexistingobject
 * @recoil-artifact defines .text recoil:function:0x41dcf0: Pickup::RegisterExistingObject (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: activate an existing pickup object, preserve its world position, and link a spawn record.
 */
void __fastcall Pickup::RegisterExistingObject(
    int,
    zClass_NodePartial *pickupObj,
    int
) {
    zVec3 worldPos;
    gwNode::GetWorldPosition(
        pickupObj,
        &worldPos
    );

    if (pickupObj->listCountA == 1 && pickupObj->listA[0] != 0) {
        zClass_Class::RemoveChild(
            pickupObj->listA[0],
            pickupObj
        );
    }

    zClass_Class::gwNodeSetActive(
        pickupObj,
        1
    );
    zClass_Object3D::gwObject3DSetPosition(
        pickupObj,
        worldPos.x,
        worldPos.y,
        worldPos.z
    );
    CreateSpawnDefAndLink(
        pickupObj,
        &worldPos,
        0,
        0,
        1
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickuptype-findbylogicalname
 * @recoil-artifact defines .text recoil:function:0x41dd60: PickupType::FindByLogicalName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: map a pickup logical name to its stored pickup type id.
 */
int __fastcall PickupType::FindByLogicalName(
    const char *logicalName,
    int *outTypeIndex
) {
    for (int index = 0; index < 40; ++index) {
        const PickupType &pickupType = g_PickupTypes[index];
        if (pickupType.logicalName != 0 && strcmp(
            logicalName,
            pickupType.logicalName
        ) == 0) {
            *outTypeIndex = pickupType.typeIndex;
            return 1;
        }
    }

    *outTypeIndex = 0;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-selectpuppieszrdbydifficulty
 * @recoil-artifact defines .text recoil:function:0x41ddf0: Pickup::SelectPuppiesZrdByDifficulty (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: choose the puppy spawn ZRD path for the current difficulty with fallback.
 */
const char *__fastcall Pickup::SelectPuppiesZrdByDifficulty(
    const char *extraSearchPath
) {
    const char *filename = kPickupPuppiesDefaultZrd;
    const int difficultyMode = zOpt::GetGameDifficultyMode();
    if (difficultyMode == 0) {
        filename = kPickupPuppiesEasyZrd;
    } else if (difficultyMode == 2) {
        filename = kPickupPuppiesHardZrd;
    }

    if (zReader::TryResolvePath(
        filename,
        extraSearchPath
    ) == 0) {
        filename = kPickupPuppiesDefaultZrd;
    }

    return filename;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.net-isoptentryactiveinanyslot
 * @recoil-artifact defines .text recoil:function:0x41de30: Net::IsOptEntryActiveInAnySlot (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: report whether any player weapon slot already owns an option entry.
 */
int __fastcall Net::IsOptEntryActiveInAnySlot(
    OptCatalogEntryDef *optEntry
) {
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));

    for (int index = 0; index < 10; ++index) {
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[index];
        if (bank.controllerA.optCatalogEntry == optEntry && (bank.controllerA.flags & 4) != 0) {
            return 1;
        }
        if (bank.controllerB.optCatalogEntry == optEntry && (bank.controllerB.flags & 4) != 0) {
            return 1;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-initandloadpuppyspawns
 * @recoil-artifact defines .text recoil:function:0x41de70: Pickup::InitAndLoadPuppySpawns (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: initialize weapon pickup metadata and load puppy pickup spawn records.
 */
int __cdecl Pickup::InitAndLoadPuppySpawns() {
    for (int index = 17; index <= 33; ++index) {
        PickupType &pickupType = g_PickupTypes[index];
        if (pickupType.weaponKeyName != 0) {
            pickupType.optEntry = OptCatalog::FindEntryByName(pickupType.weaponKeyName);
            pickupType.weaponPresenceCount = 0;
        }
    }

    g_PickupSpawnList_Primary.Clear();

    zClass::FindNextByTypePrefix(
        g_Pickup_NodePrefix,
        6
    );
    zClass_NodePartial *pickupObj = zClass::FindNextByTypePrefix(
        0,
        6
    );
    while (pickupObj != 0) {
        if (strlen(pickupObj->name) > 5 && isdigit((unsigned char)(pickupObj->name[2])) != 0) {
            zVec3 zeroVec = {0.0f, 0.0f, 0.0f};
            ((PickupNodeRuntimeFields *)(pickupObj->name))->pickupId =
                g_NextPickupId;
            if (AssignBvolGroupAndId(pickupObj) != 0) {
                PickupSpawnDef *const spawn =
                    CreateSpawnDefAndLink(
                        pickupObj,
                        &zeroVec,
                        &zeroVec,
                        0,
                        0
                    );
                if (spawn->pickupType->weaponKeyName != 0) {
                    ++spawn->pickupType->weaponPresenceCount;
                }
            }
        }

        pickupObj = zClass::FindNextByTypePrefix(
            0,
            6
        );
    }

    zReader::Node *const treeRoot =
        zReader::LoadNodeFromPath(
            SelectPuppiesZrdByDifficulty(0),
            0,
            0
        );
    if (treeRoot == 0) {
        return 0;
    }

    zReader::Node *const rootFields = treeRoot->value.nodes;
    zReader::Node *const spawnList = rootFields[1].value.nodes;
    const int spawnCount = spawnList[0].value.i32 - 1;
    for (int spawnIndex = 0; spawnIndex < spawnCount; ++spawnIndex) {
        zReader::Node *const entryFields = spawnList[spawnIndex + 1].value.nodes;
        PickupType *const pickupType = PickupTypeMeta::FindByName(entryFields[1].value.str);
        if (pickupType == 0) {
            continue;
        }

        if (zOpt::GetNetworkEnabled() == 0 && pickupType->weaponKeyName != 0 &&
            Net::IsOptEntryActiveInAnySlot(pickupType->optEntry) != 0) {
            continue;
        }

        zReader::Node *const position = entryFields[3].value.nodes;
        zReader::Node *const rotation = entryFields[4].value.nodes;
        PickupParsedZrdEntry parsedEntry = {0};
        parsedEntry.typeDesc = pickupType;
        parsedEntry.amount = entryFields[2].value.i32;
        parsedEntry.position.x = position[1].value.f32;
        parsedEntry.position.y = position[2].value.f32;
        parsedEntry.position.z = position[3].value.f32;
        parsedEntry.rotation.x = rotation[1].value.f32;
        parsedEntry.rotation.y = rotation[2].value.f32;
        parsedEntry.rotation.z = rotation[3].value.f32;
        parsedEntry.param = 1;
        parsedEntry.unknown_2c = 0;
        parsedEntry.respawnDelay = entryFields[5].value.f32;

        SpawnFromParsedZrdEntry(&parsedEntry);
        if (pickupType->weaponKeyName != 0) {
            ++pickupType->weaponPresenceCount;
        }
    }

    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
    for (int weaponIndex = 17; weaponIndex <= 33; ++weaponIndex) {
        PickupType &pickupType = g_PickupTypes[weaponIndex];
        if (pickupType.weaponPresenceCount != 0) {
            ++g_HudSensorTracker.weaponsFoundMask;
        } else if (zOpt::GetNetworkEnabled() != 0 && pickupType.weaponKeyName != 0 &&
                   weaponIndex < 32) {
            const int bankIndex = pickupType.weaponKeyName[4] - '0';
            const int sideIndex = pickupType.weaponKeyName[6] - '0';
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            PlayerGunFireController *const controller =
                sideIndex == 0 ? &bank.controllerA : &bank.controllerB;
            if (controller->ammoOrCharge != 0.0f) {
                pickupType.weaponPresenceCount = 1;
            }
        }
    }

    zReader::FreeLoadedTree(treeRoot);

    if (zOpt::GetNetworkEnabled() != 0) {
        g_PickupSpawnList_NetworkCopy.Clear();
        PickupSpawnDef *primarySpawn = g_PickupSpawnList_Primary.head;
        while (primarySpawn != 0) {
            PickupSpawnDef *const copy = (PickupSpawnDef *)(malloc(sizeof(PickupSpawnDef)));
            if (copy != 0) {
                memset(
                    copy,
                    0,
                    sizeof(*copy)
                );
                memcpy(
                    copy,
                    primarySpawn,
                    sizeof(*copy)
                );
                copy->next = 0;
                if (g_PickupSpawnList_NetworkCopy.count == 0) {
                    g_PickupSpawnList_NetworkCopy.head = copy;
                } else {
                    g_PickupSpawnList_NetworkCopy.tail->next = copy;
                }
                g_PickupSpawnList_NetworkCopy.tail = copy;
                copy->next = 0;
                ++g_PickupSpawnList_NetworkCopy.count;
            }

            primarySpawn = primarySpawn->next;
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickuptypemeta-findbyname
 * @recoil-artifact defines .text recoil:function:0x41e1a0: PickupTypeMeta::FindByName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: resolve a pickup logical name to its pickup type record.
 */
PickupType *__fastcall PickupTypeMeta::FindByName(
    const char *typeName
) {
    const int index = PickupTypeKeyTable::FindIndex(typeName);
    if (index < 0) {
        return 0;
    }

    return &g_PickupTypes[index];
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickuptype-getbyindex
 * @recoil-artifact defines .text recoil:function:0x41e1c0: PickupType::GetByIndex (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: return the pickup type record for an in-range pickup type index.
 */
PickupType *__fastcall PickupType::GetByIndex(
    int pickupTypeIndex
) {
    if (pickupTypeIndex >= 0 && pickupTypeIndex < 40) {
        return &g_PickupTypes[pickupTypeIndex];
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickuptypekeytable-findindex
 * @recoil-artifact defines .text recoil:function:0x41e1e0: PickupTypeKeyTable::FindIndex (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: scan the pickup type table for a matching logical name index.
 */
int __fastcall PickupTypeKeyTable::FindIndex(
    const char *logicalName
) {
    for (int index = 0; index < 40; ++index) {
        const char *const candidateName = g_PickupTypes[index].logicalName;
        if (candidateName != 0 && strcmp(
            logicalName,
            candidateName
        ) == 0) {
            return index;
        }
    }

    return -1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupspawnlist-clear
 * @recoil-artifact defines .text recoil:function:0x41e240: PickupSpawnList::Clear (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: free every spawn in a pickup spawn list and reset primary ids when needed.
 */
void PickupSpawnList::Clear() {
    PickupSpawnDef *node = head;
    while (node != 0) {
        PickupSpawnDef *const next = node->next;
        PickupSpawnList::RemoveAndFreeNode(
            node,
            this
        );
        node = next;
    }

    if (this == &g_PickupSpawnList_Primary) {
        g_NextPickupId = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickuprespawnqueue-clearandfree
 * @recoil-artifact defines .text recoil:function:0x41e270: PickupRespawnQueue::ClearAndFree (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: unlink and release every pending pickup respawn queue entry.
 */
void PickupRespawnQueue::ClearAndFree() {
    PickupRespawnEntry *node = head;
    while (node != 0) {
        PickupRespawnEntry *const nextNode = node->next;
        bool removed = false;
        if (count != 0) {
            PickupRespawnEntry *current = head;
            if (node == current) {
                --count;
                head = node->next;
                removed = true;
                if (head == 0) {
                    unused = 0;
                    tail = 0;
                }
            } else {
                while (current != 0) {
                    PickupRespawnEntry *const next = current->next;
                    if (next == node) {
                        --count;
                        current->next = node->next;
                        if (tail == node) {
                            tail = current;
                        }
                        removed = true;
                        break;
                    }
                    current = next;
                }
            }
        }

        if (removed) {
            ::operator delete(node);
        }
        node = nextNode;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-removeotherspawnswithsameoptentry
 * @recoil-artifact defines .text recoil:function:0x41e2f0: Pickup::RemoveOtherSpawnsWithSameOptEntry (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: remove other primary pickup spawns that grant the same option entry.
 */
void __fastcall Pickup::RemoveOtherSpawnsWithSameOptEntry(
    OptCatalogEntryDef *optEntry,
    zClass_NodePartial *keepPickupObj
) {
    PickupSpawnDef *spawn = g_PickupSpawnList_Primary.head;
    while (spawn != 0) {
        PickupSpawnDef *const next = spawn->next;
        if (spawn->pickupObj != keepPickupObj && spawn->pickupType->optEntry == optEntry) {
            PickupSpawnList::RemoveAndFreeNode(
                spawn,
                &g_PickupSpawnList_Primary
            );
        }
        spawn = next;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-setvariantfromterrain
 * @recoil-artifact defines .text recoil:function:0x41e330: Pickup::SetVariantFromTerrain (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: choose the pickup variant tag from terrain below the spawn point.
 */
void __fastcall Pickup::SetVariantFromTerrain(
    zClass_NodePartial *pickupObj,
    zVec3 *position
) {
    zTag4::Clear(&g_VariantTag_Current);
    g_Variant_CurrentTag = g_VariantTag_Current;

    PlayerProbeSampleCandidateBuffer candidateBuffer;
    zClass_cls_di::BuildPickCandidateListBelowPoint(
        g_Pickup_SceneNode,
        &candidateBuffer,
        position->x,
        500.0f,
        position->z
    );

    int bestCandidateIndex;
    int selectedImpactSlot;
    float taggedHeight;
    Player::SelectProbeSampleHeightFromCandidates(
        &candidateBuffer,
        &bestCandidateIndex,
        position->y,
        0.5f,
        1,
        &selectedImpactSlot,
        &taggedHeight
    );

    int variantTag = 255;
    if (candidateBuffer.candidateCount != 0) {
        zClassDiPickCandidateEntry *const candidate = &candidateBuffer.entries[bestCandidateIndex];
        zClass_NodePartial *const worldChild = zClass_Class::gwNodeGetWorldChild(candidate->node);
        if (worldChild != 0) {
            zClass_Class::gwNodeSetNodeType(
                pickupObj,
                worldChild->nodeType
            );
            zDi::SetVariantTagIfUnset(
                (zDiPartial *)(pickupObj->userDataOrDiRef),
                worldChild->nodeType
            );
            return;
        } else {
            variantTag = candidate->variantTag.tags[0];
            zClass_Class::gwNodeSetNodeType(
                pickupObj,
                variantTag
            );
            zDi::SetVariantTagIfUnset(
                (zDiPartial *)(pickupObj->userDataOrDiRef),
                variantTag
            );
        }
    }

    zClass_Class::gwNodeSetNodeType(
        pickupObj,
        variantTag
    );
    zDi::SetVariantTagIfUnset(
        (zDiPartial *)(pickupObj->userDataOrDiRef),
        variantTag
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-spawnlisthasentrynearxz
 * @recoil-artifact defines .text recoil:function:0x41e430: Pickup::SpawnListHasEntryNearXZ (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: test whether a primary spawn lies inside a requested XZ clearance window.
 */
int __fastcall Pickup::SpawnListHasEntryNearXZ(
    zVec3 *position,
    float clearanceRadius
) {
    PickupSpawnDef *spawn = g_PickupSpawnList_Primary.head;
    while (spawn != 0) {
        if (fabs(spawn->position.x - position->x) < clearanceRadius &&
            fabs(spawn->position.z - position->z) < clearanceRadius) {
            return 1;
        }
        spawn = spawn->next;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-selectnextvtolspawntypeindex
 * @recoil-artifact defines .text recoil:function:0x41e480: Pickup::SelectNextVTOLSpawnTypeIndex (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: rotate through available weapon pickups for the next VTOL drop.
 */
int __cdecl Pickup::SelectNextVTOLSpawnTypeIndex() {
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
    int cursor = g_Pickup_LastVTOLDropIndex + 1;

    while (cursor != g_Pickup_LastVTOLDropIndex) {
        if (cursor >= 20) {
            cursor = 3;
        }

        if (cursor != 18 && cursor != 19) {
            const int dropVariantIndex = cursor & 1;
            const int dropGroupIndex = cursor >> 1;
            int available = 0;

            if (zOpt::GetNetworkEnabled() != 0) {
                available = g_PickupTypes[14 + cursor].weaponPresenceCount != 0;
            } else {
                PlayerAltWeaponBank *const bank = &playerState->altWeaponBanks[dropGroupIndex];
                PlayerGunFireController *const controller =
                    dropVariantIndex != 0 ? &bank->controllerB : &bank->controllerA;
                available = (controller->flags & 4) != 0;
            }

            if (available != 0) {
                g_Pickup_LastVTOLDropIndex = cursor;
                return MapVTOLDropGroupVariantToTypeIndex(
                    dropGroupIndex,
                    dropVariantIndex
                );
            }
        }

        ++cursor;
    }

    g_Pickup_LastVTOLDropIndex = 3;
    return MapVTOLDropGroupVariantToTypeIndex(
        1,
        1
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-mapvtoldropgroupvarianttotypeindex
 * @recoil-artifact defines .text recoil:function:0x41e540: Pickup::MapVTOLDropGroupVariantToTypeIndex (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: convert a VTOL drop weapon group and variant into a pickup type index.
 */
int __fastcall Pickup::MapVTOLDropGroupVariantToTypeIndex(
    int dropGroupIndex,
    int dropVariantIndex
) {
    if (dropGroupIndex < 2 || dropGroupIndex > 9) {
        return 0;
    }

    return (dropGroupIndex - 2) * 2 + 1 + (dropVariantIndex != 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickuprespawnqueue-update
 * @recoil-artifact defines .text recoil:function:0x41e5d0: PickupRespawnQueue::Update (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: respawn due pickups and remove their queue entries.
 */
void __cdecl PickupRespawnQueue::Update() {
    if (g_PickupRespawnQueue.count == 0) {
        return;
    }

    PickupRespawnEntry *entry = g_PickupRespawnQueue.head;
    if (entry == 0) {
        return;
    }

    while (entry != 0) {
        if (entry->when < g_Time_UnscaledAccumulatedTimeSec) {
            Pickup::RespawnSpawnDef(entry->spawn);

            PickupRespawnEntry *const nextEntry = entry->next;
            if (g_PickupRespawnQueue.count != 0) {
                PickupRespawnEntry *prev = g_PickupRespawnQueue.head;
                if (entry == prev) {
                    --g_PickupRespawnQueue.count;
                    g_PickupRespawnQueue.head = entry->next;
                    if (g_PickupRespawnQueue.head == 0) {
                        g_PickupRespawnQueue.unused = 0;
                        g_PickupRespawnQueue.tail = 0;
                    }
                    ::operator delete(entry);
                } else if (prev != 0) {
                    while (prev != 0) {
                        PickupRespawnEntry *const prevNext = prev->next;
                        if (prevNext == entry) {
                            --g_PickupRespawnQueue.count;
                            prev->next = entry->next;
                            if (g_PickupRespawnQueue.tail == entry) {
                                g_PickupRespawnQueue.tail = prev;
                            }
                            ::operator delete(entry);
                            break;
                        }
                        prev = prevNext;
                    }
                }
            }
            entry = nextEntry;
        } else {
            entry = entry->next;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-respawnspawndef
 * @recoil-artifact defines .text recoil:function:0x41e6c0: Pickup::RespawnSpawnDef (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: restore a hidden pickup object to its saved spawn transform and flags.
 */
void __fastcall Pickup::RespawnSpawnDef(
    PickupSpawnDef *spawn
) {
    zClass_NodePartial *const pickupObj = spawn->pickupObj;

    zClass_Class::gwNodeSetActive(
        pickupObj,
        1
    );
    zClass_Class::gwNodeSetRaycastable(
        pickupObj,
        1
    );
    zClass_Class::gwNodeSetPickable(
        pickupObj,
        1
    );
    zClass_Class::gwNodeSetName(
        pickupObj,
        spawn->name
    );
    zClass_Node::SetPickupFlagsRecursive(pickupObj);

    zClass_Object3D::gwObject3DSetPosition(
        pickupObj,
        spawn->position.x,
        spawn->position.y,
        spawn->position.z
    );
    zClass_Object3D::gwObject3DSetRotation(
        pickupObj,
        spawn->rotation.x,
        spawn->rotation.y,
        spawn->rotation.z
    );
    zClass_Object3D::gwObject3DSetScale(
        pickupObj,
        1.0f,
        1.0f,
        1.0f
    );
    zClass_Object3D::gwObject3DSetLitFlag(
        pickupObj,
        1
    );
    zClass_Object3D::gwObject3DSetAlphaScale(
        pickupObj,
        0.0f
    );
    zClass_Object3D_ModelRefLerpQueue::Add(
        pickupObj,
        0,
        0,
        0.0f,
        1.0f,
        7.0f
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-archivewriteall
 * @recoil-artifact defines .text recoil:function:0x41e780: Pickup::ArchiveWriteAll (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: serialize active pickup spawns to the pickup archive section.
 */
int __fastcall Pickup::ArchiveWriteAll(
    zZbdSectionCallbackCtx *callbackCtx,
    void *userData
) {
    (void)userData;

    int result = 1;
    int firstRecord = 1;
    PickupSpawnDef *spawn = g_PickupSpawnList_Primary.head;
    while (spawn != 0 && result != 0) {
        zClass_NodePartial *const pickupObj = spawn->pickupObj;
        if ((pickupObj->flags & 0x40000) != 0) {
            PickupArchiveRecord record;
            record.firstRecord = firstRecord;
            record.typeIndex = spawn->pickupType->typeIndex;
            record.pickupId = spawn->pickupId;
            record.amount = spawn->amount;
            record.position = spawn->position;
            record.rotation = spawn->rotation;
            record.spawnParam = spawn->spawnParam;
            record.respawnDelay = spawn->respawnDelay;
            result =
                zUtil_ZAR::WriteSectionBlob(
                    callbackCtx,
                    pickupObj->name,
                    &record,
                    sizeof(record)
                );
        }

        spawn = spawn->next;
        firstRecord = 0;
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-archivereadrecord
 * @recoil-artifact defines .text recoil:function:0x41e840: Pickup::ArchiveReadRecord (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: restore one pickup spawn record from the pickup archive section.
 */
void __fastcall Pickup::ArchiveReadRecord(
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken,
    void *buffer,
    unsigned int size,
    void *userData
) {
    (void)callbackCtx;
    (void)sectionToken;
    (void)size;
    (void)userData;

    const PickupArchiveRecord *const record = (const PickupArchiveRecord *)(buffer);
    if (record->firstRecord != 0) {
        g_PickupSpawnList_Primary.Clear();
        for (int index = 0; index < 40; ++index) {
            g_PickupTypes[index].nameSuffixMax = 0;
        }
    }

    PickupSpawnDef *const spawn = SpawnAt(
        record->typeIndex,
        record->amount,
        (zVec3 *)(&record->position),
        (zVec3 *)(&record->rotation),
        record->spawnParam
    );
    if (spawn != 0) {
        spawn->respawnDelay = record->respawnDelay;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-reconcileprimaryandnetworkcopyspawnlists
 * @recoil-artifact defines .text recoil:function:0x41e890: Pickup::ReconcilePrimaryAndNetworkCopySpawnLists (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: reconcile primary and network-copy pickup spawn lists by sending
 * create or delete pkt11 deltas for missing spawn ids.
 */
void __cdecl Pickup::ReconcilePrimaryAndNetworkCopySpawnLists() {
    PickupSpawnDef *primarySpawn = g_PickupSpawnList_Primary.head;
    while (primarySpawn != 0) {
        if (SpawnListContainsPickupId(
            primarySpawn,
            &g_PickupSpawnList_NetworkCopy
        ) == 0) {
            SendPkt11_CreateDelta(primarySpawn);
        }

        primarySpawn = primarySpawn->next;
    }

    PickupSpawnDef *networkCopySpawn = g_PickupSpawnList_NetworkCopy.head;
    while (networkCopySpawn != 0) {
        if (SpawnListContainsPickupId(
            networkCopySpawn,
            &g_PickupSpawnList_Primary
        ) == 0) {
            SendPkt11_Flag2Delta(networkCopySpawn);
        }

        networkCopySpawn = networkCopySpawn->next;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-spawnlistcontainspickupid
 * @recoil-artifact defines .text recoil:function:0x41e900: Pickup::SpawnListContainsPickupId (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: scan a sorted pickup spawn list for a matching pickup id, stopping
 * early once the list passes the queried id.
 */
int __fastcall Pickup::SpawnListContainsPickupId(
    PickupSpawnDef *spawn,
    PickupSpawnList *list
) {
    PickupSpawnDef *entry = list->head;
    if (entry == 0) {
        return 0;
    }

    const int targetPickupId = spawn->pickupId;
    while (entry != 0) {
        const int entryPickupId = entry->pickupId;
        if (targetPickupId == entryPickupId) {
            return 1;
        }

        if (targetPickupId < entryPickupId) {
            return 0;
        }

        entry = entry->next;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-findspawnbypickupid
 * @recoil-artifact defines .text recoil:function:0x41e930: Pickup::FindSpawnByPickupId (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: search a pickup spawn list for the spawn with the requested pickup id.
 */
PickupSpawnDef *__fastcall Pickup::FindSpawnByPickupId(
    int pickupId,
    PickupSpawnList *list
) {
    PickupSpawnDef *spawn = list->head;
    while (spawn != 0) {
        if (spawn->pickupId == pickupId) {
            return spawn;
        }

        spawn = spawn->next;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-getspawndeffromnode
 * @recoil-artifact defines .text recoil:function:0x41e950: Pickup::GetSpawnDefFromNode (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: return the spawn definition stored in a pickup node callback context.
 */
PickupSpawnDef *__fastcall Pickup::GetSpawnDefFromNode(
    zClass_NodePartial *pickupNode
) {
    return (PickupSpawnDef *)(pickupNode->callbackContext);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-setnextpickupid
 * @recoil-artifact defines .text recoil:function:0x41e960: Pickup::SetNextPickupId (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: update the next pickup id counter and return its previous value.
 */
int __fastcall Pickup::SetNextPickupId(
    int nextPickupId
) {
    const int oldNextPickupId = g_NextPickupId;
    g_NextPickupId = nextPickupId;
    return oldNextPickupId;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-getnextpickupid
 * @recoil-artifact defines .text recoil:function:0x41e970: Pickup::GetNextPickupId (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: return the next pickup id counter.
 */
int __cdecl Pickup::GetNextPickupId() {
    return g_NextPickupId;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-finddroppabletypeforplayercurrentweapon
 * @recoil-artifact defines .text recoil:function:0x41e980: Pickup::FindDroppableTypeForPlayerCurrentWeapon (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: map the active player weapon option to the matching droppable pickup type.
 */
PickupType *__fastcall Pickup::FindDroppableTypeForPlayerCurrentWeapon(
    zUtil_SaveGameState *saveState
) {
    const char *const keyName =
        saveState->playerState->activeAltGunController->optCatalogEntry->keyName;
    {
        for (int index = 0x11; index <= 0x21; ++index) {
            if (strcmp(
                keyName,
                g_PickupTypes[index].weaponKeyName
            ) == 0) {
                return &g_PickupTypes[index];
            }
        }
    }

    return &g_PickupTypes[0];
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-findoptmetaimagebyoptentry
 * @recoil-artifact defines .text recoil:function:0x41ea00: Pickup::FindOptMetaImageByOptEntry (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: find the optional metadata image for a weapon pickup option entry.
 */
zVidImagePartial *__fastcall Pickup::FindOptMetaImageByOptEntry(
    OptCatalogEntryDef *optEntry
) {
    {
        for (int index = 0x11; index <= 0x21; ++index) {
            if (g_PickupTypes[index].optEntry == optEntry) {
                return g_PickupTypes[index].optMetaImage;
            }
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickup-spawnatcarriernodebyname
 * @recoil-artifact defines .text recoil:function:0x41ea30: Pickup::SpawnAtCarrierNodeByName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: spawn a pickup at the world transform of a named carrier node.
 */
void __fastcall Pickup::SpawnAtCarrierNodeByName(
    const char *carrierNodeName,
    int typeIndex,
    int amount
) {
    zClass_NodePartial *const carrierNode = zClass::FindByTypeAndName(
        6,
        carrierNodeName
    );
    if (carrierNode == 0) {
        return;
    }

    zVec3 position;
    gwNode::GetWorldPosition(
        carrierNode,
        &position
    );

    zVec3 rotation;
    zClass_Object3D::gwObject3DGetRotation(
        carrierNode,
        &rotation.x,
        &rotation.y,
        &rotation.z
    );
    SpawnAt(
        typeIndex,
        amount,
        &position,
        &rotation,
        0
    );
}

namespace PickupTypeKeyTable {
} // namespace PickupTypeKeyTable

namespace PickupTypeMeta {
} // namespace PickupTypeMeta

namespace Net {
} // namespace Net

namespace zClass_Node {
} // namespace zClass_Node

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupairdropspawnref-initnodesfromcarriernodename
 * @recoil-artifact defines .text recoil:function:0x438990: PickupAirdropSpawnRef::InitNodesFromCarrierNodeName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: cache a carrier node and its healthy drop-attachment child.
 */
PickupAirdropSpawnRef *PickupAirdropSpawnRef::InitNodesFromCarrierNodeName(
    const char *carrierNodeName
) {
    carrierNode = zClass::FindByTypeAndName(
        6,
        carrierNodeName
    );
    dropAttachNode = zClass_Class::FindSubNodeByName(
        carrierNode,
        g_Player_HealthySubNodeName
    );
    return this;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupairdropspawnref-spawnpickuptypeandrelay
 * @recoil-artifact defines .text recoil:function:0x4389c0: PickupAirdropSpawnRef::SpawnPickupTypeAndRelay (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: spawn an airdrop pickup locally and relay it from network hosts.
 */
int PickupAirdropSpawnRef::SpawnPickupTypeAndRelay(
    int pickupTypeIndex
) {
    if ((dropAttachNode->flags & 4) == 0) {
        return 0;
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        if (zNetwork::IsHost() == 0) {
            return 0;
        }

        Pickup::SendPkt12_AirdropSpawnChuteRelay(
            pickupTypeIndex,
            &worldPos,
            Pickup::GetNextPickupId()
        );
    }

    Pickup::SpawnWithAirdropChute(
        pickupTypeIndex,
        &worldPos
    );
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupairdropspawnref-canspawnwithclearance
 * @recoil-artifact defines .text recoil:function:0x438a20: PickupAirdropSpawnRef::CanSpawnWithClearance (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: gate VTOL pickup spawning on attach-node state, ammo state, and XZ clearance.
 */
int PickupAirdropSpawnRef::CanSpawnWithClearance(
    float clearanceRadius
) {
    if ((dropAttachNode->flags & 4) == 0) {
        return 0;
    }

    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
    if (playerState->activeAltGunController->ammoOrCharge == kPickupAltAmmoDisabledSentinel) {
        return 0;
    }

    return Pickup::SpawnListHasEntryNearXZ(
        GetWorldPos(),
        clearanceRadius
    ) == 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupairdropspawnref-getworldpos
 * @recoil-artifact defines .text recoil:function:0x438a70: PickupAirdropSpawnRef::GetWorldPos (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: refresh and return the cached world position of the airdrop carrier.
 */
zVec3 *PickupAirdropSpawnRef::GetWorldPos() {
    gwNode::GetWorldPosition(
        carrierNode,
        &worldPos
    );
    return &worldPos;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupairdropspawnref-initglobalfromcarriernodename
 * @recoil-artifact defines .text recoil:function:0x438a90: PickupAirdropSpawnRef::InitGlobalFromCarrierNodeName (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: allocate and validate the global airdrop spawn reference.
 */
void __fastcall PickupAirdropSpawnRef::InitGlobalFromCarrierNodeName(
    const char *carrierNodeName
) {
    PickupAirdropSpawnRef *const spawnRef = new PickupAirdropSpawnRef;
    g_Pickup_GlobalAirdropSpawnRef = spawnRef->InitNodesFromCarrierNodeName(carrierNodeName);

    if (g_Pickup_GlobalAirdropSpawnRef->carrierNode == 0 ||
        g_Pickup_GlobalAirdropSpawnRef->dropAttachNode == 0) {
        ::operator delete(g_Pickup_GlobalAirdropSpawnRef);
        g_Pickup_GlobalAirdropSpawnRef = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupairdropspawnref-shutdownglobal
 * @recoil-artifact defines .text recoil:function:0x438b10: PickupAirdropSpawnRef::ShutdownGlobal (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: release the global airdrop spawn reference and clear its pointer.
 */
void PickupAirdropSpawnRef::ShutdownGlobal() {
    PickupAirdropSpawnRef *const spawnRef = g_Pickup_GlobalAirdropSpawnRef;
    if (spawnRef != 0) {
        ::operator delete(spawnRef);
    }

    g_Pickup_GlobalAirdropSpawnRef = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.pickupairdropspawnref-tryspawnrandompickupfromglobal
 * @recoil-artifact defines .text recoil:function:0x438b30: PickupAirdropSpawnRef::TrySpawnRandomPickupFromGlobal (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: attempt a random VTOL pickup spawn through the global airdrop ref.
 */
int __cdecl PickupAirdropSpawnRef::TrySpawnRandomPickupFromGlobal() {
    zOpt::GetNetworkEnabled();
    if (g_Pickup_GlobalAirdropSpawnRef->CanSpawnWithClearance(20.0f) == 0) {
        return 0;
    }

    return g_Pickup_GlobalAirdropSpawnRef->SpawnPickupTypeAndRelay(
        Pickup::SelectNextVTOLSpawnTypeIndex()
    );
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *PickupCrtInitializerFn)();
/* VC5 emits these pickup.cpp startup callbacks as direct .CRT$XCU rows. */
#pragma data_seg(".CRT$XCU")
PickupCrtInitializerFn s_PickupCrtInit_PrimarySpawnList =
    PickupSpawnList::Primary_Init;
PickupCrtInitializerFn s_PickupCrtInit_NetworkCopySpawnList =
    PickupSpawnList::NetCopy_Init;
PickupCrtInitializerFn s_PickupCrtInit_RespawnQueue =
    PickupRespawnQueue::Init;
#pragma data_seg()
#endif

#if defined(RECOILAPP_LINK_SPLIT_EARLY_SHARD)
namespace Pickup {
/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.sendpkt11-flag2delta
 * @recoil-artifact defines .text recoil:function:0x433e40: Pickup::SendPkt11_Flag2Delta (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: send a reliable pkt11 delete delta for a spawn missing from the
 * primary pickup spawn list.
 */
int __fastcall SendPkt11_Flag2Delta(
    PickupSpawnDef *spawn
) {
    g_PickupPkt11Flag2Delta.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_PickupPkt11Flag2Delta.flags = 2;
    g_PickupPkt11Flag2Delta.pickupId = spawn->pickupId;
    return zNetwork_SendPacketReliable(&g_PickupPkt11Flag2Delta.header);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.sendpkt11-flag8delta
 * @recoil-artifact defines .text recoil:function:0x433e70: Pickup::SendPkt11_Flag8Delta (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: send a reliable pkt11 hidden-state delta for a pickup spawn.
 */
int __fastcall SendPkt11_Flag8Delta(
    PickupSpawnDef *spawn
) {
    g_PickupPkt11Flag8Delta.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_PickupPkt11Flag8Delta.flags = 8;
    g_PickupPkt11Flag8Delta.pickupId = spawn->pickupId;
    return zNetwork_SendPacketReliable(&g_PickupPkt11Flag8Delta.header);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.sendpkt11-createdelta
 * @recoil-artifact defines .text recoil:function:0x433ea0: Pickup::SendPkt11_CreateDelta (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: allocate, populate, send, and release a reliable pkt11 create
 * delta for a primary pickup spawn.
 */
void __fastcall SendPkt11_CreateDelta(
    PickupSpawnDef *spawn
) {
    PickupPkt11CreateDelta *const packet =
        (PickupPkt11CreateDelta *)(malloc(sizeof(PickupPkt11CreateDelta)));
    memset(
        packet,
        0,
        sizeof(PickupPkt11CreateDelta)
    );

    packet->header.packetType = 0x11;
    packet->header.packetSizeBytes = sizeof(PickupPkt11CreateDelta);
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->flags = 1;
    packet->pickupId = spawn->pickupId;
    packet->typeKeyIndex =
        (unsigned short)(PickupTypeKeyTable::FindIndex(spawn->pickupType->logicalName));
    packet->amount = spawn->amount;
    packet->position = spawn->position;
    packet->rotation = spawn->rotation;
    packet->respawnDelay = spawn->respawnDelay;

    zNetwork_SendPacketReliable(&packet->header);
    free(packet);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.handlepkt11-spawndelta
 * @recoil-artifact defines .text recoil:function:0x433f40: Pickup::HandlePkt11_SpawnDelta (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: apply a network pkt11 pickup spawn delta by creating, deleting, or
 * hiding the addressed spawn.
 */
int __fastcall HandlePkt11_SpawnDelta(
    int,
    PickupPkt11CreateDelta *packet
) {
    PickupSpawnDef *const spawn = FindSpawnByPickupId(
        packet->pickupId,
        &g_PickupSpawnList_Primary
    );
    const unsigned int flags = packet->flags;

    if ((flags & 1u) != 0 && spawn == 0) {
        PickupParsedZrdEntry entry;
        memset(
            &entry,
            0,
            sizeof(entry)
        );
        entry.typeDesc = PickupType::GetByIndex((int)(packet->typeKeyIndex));
        entry.amount = packet->amount;
        entry.position = packet->position;
        entry.rotation = packet->rotation;
        entry.respawnDelay = packet->respawnDelay;

        PickupSpawnDef *const newSpawn = SpawnFromParsedZrdEntry(&entry);
        if (newSpawn != 0) {
            newSpawn->pickupId = packet->pickupId;
        }
        SetNextPickupId(packet->pickupId + 1);
        return 1;
    }

    if (spawn == 0 || spawn->refCount != 0) {
        return 1;
    }

    if ((flags & 2u) != 0) {
        PickupSpawnList::RemoveAndFreeNode(
            spawn,
            &g_PickupSpawnList_Primary
        );
        return 1;
    }

    if ((flags & 8u) != 0) {
        zClass_NodePartial *const pickupObj = spawn->pickupObj;
        zClass_Node::ClearPickupFlagsRecursive(pickupObj);
        zClass_Class::gwNodeSetRaycastable(
            pickupObj,
            0
        );
        zClass_Class::gwNodeSetPickable(
            pickupObj,
            0
        );
        RemoveObject(
            0,
            pickupObj,
            0
        );
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.sendpkt12-airdropspawnchuterelay
 * @recoil-artifact defines .text recoil:function:0x434050: Pickup::SendPkt12_AirdropSpawnChuteRelay (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: send a reliable pkt12 airdrop chute-spawn relay to peers.
 */
void __fastcall SendPkt12_AirdropSpawnChuteRelay(
    int pickupTypeIndex,
    zVec3 *spawnPos,
    int nextPickupId
) {
    g_PickupPkt12AirdropSpawnChuteRelay.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_PickupPkt12AirdropSpawnChuteRelay.spawnPos = *spawnPos;
    g_PickupPkt12AirdropSpawnChuteRelay.pickupTypeIndex = (unsigned short)(pickupTypeIndex);
    g_PickupPkt12AirdropSpawnChuteRelay.nextPickupId = nextPickupId;
    zNetwork_SendPacketReliable(&g_PickupPkt12AirdropSpawnChuteRelay.header);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.pickup.handlepkt12-airdropspawnchuterelay
 * @recoil-artifact defines .text recoil:function:0x4340a0: Pickup::HandlePkt12_AirdropSpawnChuteRelay (D:\Proj\Battlesport\pickup.cpp).
 * Purpose: relay an airdrop chute spawn packet into the local pickup state.
 */
int __fastcall HandlePkt12_AirdropSpawnChuteRelay(
    int,
    PickupPkt12AirdropSpawnChuteRelay *packet
) {
    SetNextPickupId(packet->nextPickupId);
    SpawnWithAirdropChute(
        (int)(packet->pickupTypeIndex),
        &packet->spawnPos
    );
    return 1;
}

} // namespace Pickup
#endif

namespace PickupTypeTable {
} // namespace PickupTypeTable
