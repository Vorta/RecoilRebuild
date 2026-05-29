#include "Battlesport/hud.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"

#include <cstring>
#include <windows.h>

namespace {
struct FunctionJumpPatch {
    void *target;
    unsigned char original[5];
    int installed;
};

struct HudCheatFakeState {
    int messageCalls;
    unsigned int lastMessageId;
    int pickupCalls;
    int pickupTypeIds[4];
    int pickupAmounts[4];
    zUtil_SaveGameState *pickupSaveStates[4];
    int bindCalls;
    int bindCommandIds[4];
    zInputCommandCallbackFn bindCallbacks[4];
    int stopCalls;
    zEffectAnimEntry *stoppedEntry;
    int steeringCalls;
    int steeringMode;
    int cameraCalls;
    int cameraState;
    int resetMouseCalls;
    int nodeActionCalls;
    zEffectAnimEntry *nodeActionEntry;
    zClass_NodePartial *nodeActionRoot;
    int resetDamageCalls;
    int transitions[8];
    int transitionFlags[8];
    int transitionCount;
};

HudCheatFakeState g_hudCheatFakeState;

void ResetHudCheatFakeState()
{
    std::memset(&g_hudCheatFakeState, 0, sizeof(g_hudCheatFakeState));
}

bool InstallFunctionJump(void *target, void *replacement, FunctionJumpPatch *patch)
{
    patch->target = target;
    patch->installed = 0;
    DWORD oldProtect = 0;
    if (VirtualProtect(target, sizeof(patch->original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0)
    {
        return false;
    }

    std::memcpy(patch->original, target, sizeof(patch->original));
    unsigned char jumpBytes[5];
    jumpBytes[0] = 0xe9;
    const int relativeOffset =
        (int)((char *)replacement - ((char *)target + sizeof(jumpBytes)));
    std::memcpy(jumpBytes + 1, &relativeOffset, sizeof(relativeOffset));
    std::memcpy(target, jumpBytes, sizeof(jumpBytes));
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(jumpBytes));
    DWORD ignoredProtect = 0;
    VirtualProtect(target, sizeof(patch->original), oldProtect, &ignoredProtect);
    patch->installed = 1;
    return true;
}

void RestoreFunctionJump(FunctionJumpPatch *patch)
{
    if (patch->installed == 0)
    {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch->target, sizeof(patch->original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0)
    {
        std::memcpy(patch->target, patch->original, sizeof(patch->original));
        FlushInstructionCache(GetCurrentProcess(), patch->target, sizeof(patch->original));
        DWORD ignoredProtect = 0;
        VirtualProtect(patch->target, sizeof(patch->original), oldProtect, &ignoredProtect);
    }
    patch->installed = 0;
}

char *RECOIL_FASTCALL FakeHudCheatGetMessageString(unsigned int messageId)
{
    ++g_hudCheatFakeState.messageCalls;
    g_hudCheatFakeState.lastMessageId = messageId;
    switch (messageId)
    {
    case 4096:
        return const_cast<char *>("CHEAT_PICKUP_A");
    case 4097:
        return const_cast<char *>("CHEAT_RESPAWN");
    case 4098:
        return const_cast<char *>("CHEAT_PICKUP_B");
    case 4100:
        return const_cast<char *>("CHEAT_BIND_36");
    case 4101:
        return const_cast<char *>("CHEAT_BIND_31");
    default:
        return const_cast<char *>("UNMATCHED_CHEAT_MESSAGE");
    }
}

int RECOIL_FASTCALL FakeHudCheatApplyEffect(int pickupTypeId, int overrideAmount,
                                           zUtil_SaveGameState *saveState)
{
    const int index = g_hudCheatFakeState.pickupCalls++;
    if (index < 4)
    {
        g_hudCheatFakeState.pickupTypeIds[index] = pickupTypeId;
        g_hudCheatFakeState.pickupAmounts[index] = overrideAmount;
        g_hudCheatFakeState.pickupSaveStates[index] = saveState;
    }
    return 100000 + pickupTypeId;
}

int RECOIL_FASTCALL FakeHudCheatBindCommandCallback(int commandId,
                                                   zInputCommandCallbackFn callback)
{
    const int index = g_hudCheatFakeState.bindCalls++;
    if (index < 4)
    {
        g_hudCheatFakeState.bindCommandIds[index] = commandId;
        g_hudCheatFakeState.bindCallbacks[index] = callback;
    }
    return 1;
}

int RECOIL_FASTCALL FakeHudCheatStop(zEffectAnimEntry *entry)
{
    ++g_hudCheatFakeState.stopCalls;
    g_hudCheatFakeState.stoppedEntry = entry;
    return 1;
}

void RECOIL_FASTCALL FakeHudCheatSetSteeringMode(int enable)
{
    ++g_hudCheatFakeState.steeringCalls;
    g_hudCheatFakeState.steeringMode = enable;
}

void RECOIL_FASTCALL FakeHudCheatApplyCameraState(int newState)
{
    ++g_hudCheatFakeState.cameraCalls;
    g_hudCheatFakeState.cameraState = newState;
}

void RECOIL_FASTCALL FakeHudCheatResetMouseControl(zUtil_SaveGameState *)
{
    ++g_hudCheatFakeState.resetMouseCalls;
}

int RECOIL_FASTCALL FakeHudCheatNodeAction(zEffectAnimEntry *entry,
                                          zClass_NodePartial *rootNode)
{
    ++g_hudCheatFakeState.nodeActionCalls;
    g_hudCheatFakeState.nodeActionEntry = entry;
    g_hudCheatFakeState.nodeActionRoot = rootNode;
    return 1;
}

void RECOIL_FASTCALL FakeHudCheatResetDamage(zUtil_SaveGameState *)
{
    ++g_hudCheatFakeState.resetDamageCalls;
}

void RecordHudCheatTransition(int transitionId, int flags)
{
    const int index = g_hudCheatFakeState.transitionCount++;
    if (index < 8)
    {
        g_hudCheatFakeState.transitions[index] = transitionId;
        g_hudCheatFakeState.transitionFlags[index] = flags;
    }
}

int RECOIL_FASTCALL FakeHudCheatTransitionTrack(zUtil_SaveGameState *, int flags)
{
    RecordHudCheatTransition(1, flags);
    return 1;
}

int RECOIL_FASTCALL FakeHudCheatTransitionAmphib(zUtil_SaveGameState *, int, int extraFlags)
{
    RecordHudCheatTransition(2, extraFlags);
    return 1;
}

int RECOIL_FASTCALL FakeHudCheatTransitionSub(zUtil_SaveGameState *, int flags)
{
    RecordHudCheatTransition(3, flags);
    return 1;
}

int RECOIL_FASTCALL FakeHudCheatTransitionHover(zUtil_SaveGameState *, int flags)
{
    RecordHudCheatTransition(4, flags);
    return 1;
}
} // namespace

extern "C" int hud_low_meter_loop_sound_set_loop_active_smoke(void)
{
    zSndSample *const oldBeepSample = g_Hud_LowMeterBeepSample;
    zSndSample *const oldSample = g_Hud_LowMeterLoopSample;
    const int oldActive = g_Hud_LowMeterLoopActive;

    g_Hud_LowMeterBeepSample = 0;
    g_Hud_LowMeterLoopSample = 0;
    g_Hud_LowMeterLoopActive = 0;

    HudLowMeterLoopSound::SetLoopActive(1);
    const bool enabledOk = g_Hud_LowMeterLoopActive == 1;

    HudLowMeterLoopSound::SetLoopActive(1);
    const bool enableIdempotentOk = g_Hud_LowMeterLoopActive == 1;

    HudLowMeterLoopSound::SetLoopActive(0);
    const bool disabledOk = g_Hud_LowMeterLoopActive == 0;

    HudLowMeterLoopSound::SetLoopActive(0);
    const bool disableIdempotentOk = g_Hud_LowMeterLoopActive == 0;

    g_Hud_LowMeterLoopActive = 1;
    HudLowMeterLoopSound::Disable();
    const bool fullDisableOk = g_Hud_LowMeterLoopActive == 0;

    g_Hud_LowMeterBeepSample = oldBeepSample;
    g_Hud_LowMeterLoopSample = oldSample;
    g_Hud_LowMeterLoopActive = oldActive;

    return enabledOk && enableIdempotentOk && disabledOk && disableIdempotentOk &&
                   fullDisableOk
               ? 0
               : 1;
}

extern "C" int hud_cheat_clear_nanite_panel_cheat_sentinel_smoke(void)
{
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    playerState.nanitePanelLevel = 123456789;
    HudCheat::ClearNanitePanelCheatSentinel();
    const bool sentinelCleared = playerState.nanitePanelLevel == 0;

    playerState.nanitePanelLevel = 42;
    HudCheat::ClearNanitePanelCheatSentinel();
    const bool nonSentinelPreserved = playerState.nanitePanelLevel == 42;

    g_GameStateOrMapTable = 0;
    HudCheat::ClearNanitePanelCheatSentinel();
    const bool nullStateOk = g_GameStateOrMapTable == 0;

    g_GameStateOrMapTable = oldGameState;

    return sentinelCleared && nonSentinelPreserved && nullStateOk ? 0 : 1;
}

extern "C" int hud_cheat_execute_command_string_smoke(void)
{
    FunctionJumpPatch patches[16] = {};
    int patchCount = 0;
    bool patchOk = true;

#define HUD_CHEAT_PATCH(target, replacement)                                              \
    do                                                                                    \
    {                                                                                     \
        patchOk = patchOk &&                                                              \
                  InstallFunctionJump(reinterpret_cast<void *>(target),                   \
                                      reinterpret_cast<void *>(replacement),              \
                                      &patches[patchCount++]);                            \
    } while (0)

    HUD_CHEAT_PATCH(&zLoc::GetMessageString, &FakeHudCheatGetMessageString);
    HUD_CHEAT_PATCH(&Pickup::ApplyEffect, &FakeHudCheatApplyEffect);
    HUD_CHEAT_PATCH(&zInput::BindMap_Current_SetCommandCallback,
                    &FakeHudCheatBindCommandCallback);
    HUD_CHEAT_PATCH(&zEffectAnim::Stop, &FakeHudCheatStop);
    HUD_CHEAT_PATCH(&zOpt::SetSteeringMode, &FakeHudCheatSetSteeringMode);
    HUD_CHEAT_PATCH(&Player::ApplyCameraState, &FakeHudCheatApplyCameraState);
    HUD_CHEAT_PATCH(&Player::ResetMouseControlStateAndRecenterCursor,
                    &FakeHudCheatResetMouseControl);
    HUD_CHEAT_PATCH(&zEffect_Anim::NodeActionCallback, &FakeHudCheatNodeAction);
    HUD_CHEAT_PATCH(&Player::ResetDamageStateAndTimedHitStatus, &FakeHudCheatResetDamage);
    HUD_CHEAT_PATCH(&Player::TransitionToMasterTypeTrack, &FakeHudCheatTransitionTrack);
    HUD_CHEAT_PATCH(&Player::TransitionToMasterTypeAmphib, &FakeHudCheatTransitionAmphib);
    HUD_CHEAT_PATCH(&Player::TransitionToMasterTypeSub, &FakeHudCheatTransitionSub);
    HUD_CHEAT_PATCH(&Player::TransitionToMasterTypeHover, &FakeHudCheatTransitionHover);

#undef HUD_CHEAT_PATCH

    if (!patchOk)
    {
        for (int index = patchCount - 1; index >= 0; --index)
        {
            RestoreFunctionJump(&patches[index]);
        }
        return 1;
    }

    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const float oldTime = g_Time_AccumulatedTimeSec;
    const int oldPrevCamera = g_PlayerPrevCameraState;
    const int oldPrevSteering = g_PlayerPrevSteeringMode;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zEffectAnimEntry recentHitLight = {};
    zEffectAnimEntry respawnEffect = {};
    zClass_NodePartial rootNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    g_Time_AccumulatedTimeSec = 42.0f;
    g_PlayerPrevCameraState = 7;
    g_PlayerPrevSteeringMode = 3;

    ResetHudCheatFakeState();
    CString emptyCommand("");
    const bool emptyOk =
        HudCheat::ExecuteCommandString(&emptyCommand) == 0 &&
        g_hudCheatFakeState.messageCalls == 0;

    ResetHudCheatFakeState();
    CString pickup901Command("prefix cheat_pickup_a suffix");
    const bool pickup901Ok =
        HudCheat::ExecuteCommandString(&pickup901Command) == 100901 &&
        g_hudCheatFakeState.pickupCalls == 1 &&
        g_hudCheatFakeState.pickupTypeIds[0] == 901 &&
        g_hudCheatFakeState.pickupAmounts[0] == 0 &&
        g_hudCheatFakeState.pickupSaveStates[0] == &saveState;

    ResetHudCheatFakeState();
    CString pickup903Command("prefix cheat_pickup_b suffix");
    const bool pickup903Ok =
        HudCheat::ExecuteCommandString(&pickup903Command) == 100903 &&
        g_hudCheatFakeState.pickupCalls == 1 &&
        g_hudCheatFakeState.pickupTypeIds[0] == 903;

    ResetHudCheatFakeState();
    CString bindCommand("cheat_bind_31 and cheat_bind_36");
    const bool bindOk =
        HudCheat::ExecuteCommandString(&bindCommand) == 0 &&
        g_hudCheatFakeState.bindCalls == 2 &&
        g_hudCheatFakeState.bindCommandIds[0] == 31 &&
        g_hudCheatFakeState.bindCommandIds[1] == 36 &&
        g_hudCheatFakeState.bindCallbacks[0] ==
            (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand) &&
        g_hudCheatFakeState.bindCallbacks[1] ==
            (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand);

    ResetHudCheatFakeState();
    playerState.recentHitValid = 1;
    playerState.recentHitLightHandle = &recentHitLight;
    playerState.lifecycleState = 4;
    playerState.destroyedRespawnFxEntry = &respawnEffect;
    playerState.rootNode = &rootNode;
    playerState.aiMode = 99;
    playerState.nextModeSwitchAllowedTime = 9.0f;
    playerState.autoTurnSign = -1;
    playerState.motionInput = 5;
    modalData.masterType = 2;
    CString respawnCommand("xx cheat_respawn yy");
    const int respawnResult = HudCheat::ExecuteCommandString(&respawnCommand);
    const bool respawnOk =
        respawnResult == 100902 && playerState.recentHitValid == 0 &&
        playerState.recentHitLightHandle == 0 && playerState.lifecycleState == 1 &&
        playerState.aiMode == 0 && playerState.nextModeSwitchAllowedTime == 0.0f &&
        playerState.autoTurnSign == 0 && playerState.motionInput == 0 &&
        playerState.primaryGunGateUntilTime == g_Time_AccumulatedTimeSec &&
        playerState.altGunTransitionState == 16 &&
        g_hudCheatFakeState.stopCalls == 1 &&
        g_hudCheatFakeState.stoppedEntry == &recentHitLight &&
        g_hudCheatFakeState.steeringCalls == 1 &&
        g_hudCheatFakeState.steeringMode == 3 &&
        g_hudCheatFakeState.cameraCalls == 1 &&
        g_hudCheatFakeState.cameraState == 7 &&
        g_hudCheatFakeState.resetMouseCalls == 1 &&
        g_hudCheatFakeState.nodeActionCalls == 1 &&
        g_hudCheatFakeState.nodeActionEntry == &respawnEffect &&
        g_hudCheatFakeState.nodeActionRoot == &rootNode &&
        g_hudCheatFakeState.resetDamageCalls == 1 &&
        g_hudCheatFakeState.transitionCount == 3 &&
        g_hudCheatFakeState.transitions[0] == 1 &&
        g_hudCheatFakeState.transitionFlags[0] == 1 &&
        g_hudCheatFakeState.transitions[1] == 2 &&
        g_hudCheatFakeState.transitionFlags[1] == 1 &&
        g_hudCheatFakeState.transitions[2] == 3 &&
        g_hudCheatFakeState.transitionFlags[2] == 0 &&
        g_hudCheatFakeState.pickupCalls == 1 &&
        g_hudCheatFakeState.pickupTypeIds[0] == 902;

    ResetHudCheatFakeState();
    CString noMatchCommand("ordinary text");
    const bool noMatchOk =
        HudCheat::ExecuteCommandString(&noMatchCommand) == 0 &&
        g_hudCheatFakeState.pickupCalls == 0 &&
        g_hudCheatFakeState.bindCalls == 0;

    g_GameStateOrMapTable = oldGameState;
    g_Time_AccumulatedTimeSec = oldTime;
    g_PlayerPrevCameraState = oldPrevCamera;
    g_PlayerPrevSteeringMode = oldPrevSteering;

    for (int index = patchCount - 1; index >= 0; --index)
    {
        RestoreFunctionJump(&patches[index]);
    }

    return emptyOk && pickup901Ok && pickup903Ok && bindOk && respawnOk && noMatchOk
               ? 0
               : 2;
}
