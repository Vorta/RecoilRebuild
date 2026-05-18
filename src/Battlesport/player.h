#pragma once

#include "recoil/recoil_types.h"

#include "Battlesport/ainet.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "recoil/recoil_callconv.h"

struct zInput_GameStateOrMapTablePartial;
struct zEffectAnimEntry;

extern "C" {
extern int g_Player_HudCounterValue;
extern zUtil_SaveGameState *g_PlayerSaveStateListHead;
extern zUtil_SaveGameState *g_PlayerSaveStateListTail;
extern int g_PlayerSaveStateListAux;
extern int g_PlayerSaveStateCount;
extern zUtil_SaveGameState *g_LocalPlayerSaveState;
extern zInput_GameStateOrMapTablePartial *g_GameStateOrMapTable;
extern int g_Player_NextOrdinal;
extern int g_Player_AiMode2State1Finalized;
extern float g_PlayerStatusMeterRatio;
extern HudUiElement g_Player_UnderwaterFxPass3Ui;
extern HudUiElement g_Player_State7FxPass3Ui;
extern zEffectAnimEntry *g_Player_ActiveDebugScriptAsyncEntry;
extern int g_Player_HorizonNodeFollowCameraEnabled;
extern zClass_NodePartial *g_Player_HorizonNode;
}

struct HudUiMgrSensorTrackNode {
    int trackKind;
    void *payload;
    HudUiMgrSensorTrackNode *next;
};

enum HudUiMgrSensorTrackKind {
    HUD_SENSOR_TRACK_KIND_PLAYER = 2,
    HUD_SENSOR_TRACK_KIND_TURRET = 3
};

struct HudUiMgrSensorTrackList {
    int trackListAux;
    HudUiMgrSensorTrackNode *head;
    HudUiMgrSensorTrackNode *tail;
    int count;
};

struct PlayerMasterWeaponSpec {
    PlayerMasterWeaponSpec *next;
};

struct PlayerMasterCommonData {
    PlayerMasterCommonData *next;
    unsigned char unknown_004[0x394];
    float maxHealth;
    unsigned char unknown_39c[0x0c];
    int weaponSpecListAux;
    PlayerMasterWeaponSpec *weaponSpecHead;
    PlayerMasterWeaponSpec *weaponSpecTail;
    int weaponSpecCount;
};

struct PlayerMasterModalData {
    PlayerMasterModalData *next;
    unsigned char unknown_004[0xa0];
    int masterType;
};

struct PlayerMissionSaveWeaponSide {
    int enabled;
    float ammoOrCharge;
};

struct PlayerMissionSaveWeaponBank {
    int selectedSide;
    PlayerMissionSaveWeaponSide sides[2];
};

struct PlayerMissionSaveTimedHitStatus {
    unsigned int runtimeFlags;
    unsigned int savedHitSourceEntryId;
    unsigned int unknown_08;
    unsigned int unknown_0c;
    zClass_NodePartial *lightNode;
    float nextUpdateTime;
    unsigned int unknown_18;
};

struct PlayerMissionSaveData {
    int size;
    int altWeaponBankIndex;
    int altWeaponSideIndex;
    int primaryWeaponBankIndex;
    int primaryWeaponSideIndex;
    PlayerMissionSaveWeaponBank weaponBank[10];
    float playerStatusMeterRatio;
    int hudCounterValue;
    int amphibUnlocked;
    int hoverUnlocked;
    int subUnlocked;
    int aiMode;
    float nextModeSwitchAllowedTime;
    int motionInput;
    int autoTurnSign;
    int bankInput;
    int playerMasterType;
    zVec3 cameraTarget;
    zVec3 cameraPosition;
    int unknown_120;
    PlayerMissionSaveTimedHitStatus timedHitStatus;
};

extern "C" {
extern HudUiMgrSensorTrackList g_HudUiMgrSensor_TrackList;
extern PlayerMasterCommonData *g_PlayerMasterCommonDataHead;
extern PlayerMasterCommonData *g_PlayerMasterCommonDataTail;
extern int g_PlayerMasterCommonDataListAux;
extern int g_PlayerMasterCommonDataCount;
extern PlayerMasterModalData *g_PlayerMasterModalDataHead;
extern PlayerMasterModalData *g_PlayerMasterModalDataTail;
extern int g_PlayerMasterModalDataListAux;
extern int g_PlayerMasterModalDataCount;
}

namespace Player {
RECOIL_NOINLINE void RECOIL_FASTCALL AddScaledHudCounterValue(float value);
RECOIL_NOINLINE void RECOIL_FASTCALL
AiDiscardNegativeBranchPathNodes(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_FASTCALL AiRestorePreviousTopLevelState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_CDECL AiFinalizeMode2State1ForAllPlayers();
RECOIL_NOINLINE void RECOIL_FASTCALL BuildMissionSaveData(PlayerMissionSaveData *outData);
RECOIL_NOINLINE void RECOIL_FASTCALL
FreeAltWeaponTrailRuntimeStates(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE PlayerGunFireController *RECOIL_FASTCALL
FindAltGunFireControllerForWeaponId(zUtil_SaveGameState *saveState, int weaponId);
RECOIL_NOINLINE int RECOIL_FASTCALL
TestScenePathBetweenCameraTargetAndPoint(zClass_NodePartial *node, const zVec3 *point,
                                         int directionMode);
RECOIL_NOINLINE void RECOIL_FASTCALL ComposeAimBasisWorldMatrix(zUtil_SaveGameState *saveState,
                                                                zMat4x3 *outMatrix34);
RECOIL_NOINLINE void RECOIL_FASTCALL SelectAltGunFirePointAndSlot(
    zUtil_SaveGameState *saveState, PlayerGunFireSlot **outActiveFireSlotPtr);
RECOIL_NOINLINE void RECOIL_FASTCALL DestroySaveGameState(zUtil_SaveGameState *saveState);
RECOIL_NOINLINE void RECOIL_CDECL ShutdownMissionRuntime();
} // namespace Player

RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackNode, next) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackNode, trackKind) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackNode, payload) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(HudUiMgrSensorTrackList) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackList, trackListAux) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackList, head) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackList, tail) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiMgrSensorTrackList, count) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterWeaponSpec, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, maxHealth) == 0x398);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecListAux) == 0x3a8);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecHead) == 0x3ac);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecTail) == 0x3b0);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterCommonData, weaponSpecCount) == 0x3b4);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerMasterModalData, masterType) == 0xa4);
RECOIL_STATIC_ASSERT(sizeof(PlayerMissionSaveWeaponSide) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(PlayerMissionSaveWeaponBank) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, weaponBank) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, playerStatusMeterRatio) == 0xdc);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, playerMasterType) == 0x104);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, cameraTarget) == 0x108);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, cameraPosition) == 0x114);
RECOIL_STATIC_ASSERT(offsetof(PlayerMissionSaveData, timedHitStatus) == 0x124);
RECOIL_STATIC_ASSERT(sizeof(PlayerMissionSaveData) == 0x140);
