#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"

#include <cstdint>
#include <cstring>

extern "C" std::uint32_t g_HudUi_InvalidateMask;

namespace {
bool FloatNear(
    float actual,
    float expected
) {
    return actual > expected - 0.0001f && actual < expected + 0.0001f;
}

template <typename Method>
std::uintptr_t MethodAddress(Method method) {
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

int g_PlayerDamageHitHudVisibleCount;
void *g_PlayerDamageHitHudVisibleThis[8];
int g_PlayerDamageHitHudVisibleValue[8];

struct PlayerDamageHitHudVisibleReceiver {
    void SetVisible(int visible) {
        const int index = g_PlayerDamageHitHudVisibleCount;
        if (index < 8) {
            g_PlayerDamageHitHudVisibleThis[index] = this;
            g_PlayerDamageHitHudVisibleValue[index] = visible;
        }
        ++g_PlayerDamageHitHudVisibleCount;
    }
};

void __fastcall PlayerDestroyedEffectDoneCallback(
    zEffectAnimEntry *,
    void *,
    int
) {
}

void InitDestroyedEffectEntry(
    zEffectAnimEntry *entry,
    zClass_NodePartial *boundNode,
    zClass_NodePartial *runtimeNode,
    const char *name
) {
    std::memset(entry, 0, sizeof(*entry));
    std::strcpy(entry->name, name);
    entry->boundNode = boundNode;
    entry->callbackNode = boundNode;
    entry->runtimeNode = runtimeNode;
    entry->priority = 3;
}
} // namespace

extern "C" int player_record_recent_hit_feedback_smoke(void) {
    zEffectAnimEntry *const oldRecentHitFxAnimEntry = g_PlayerRecentHitFxAnimEntry;
    void *const oldOwnerOrCtx = g_OptCatalog_CurrentDamageOwnerOrCtx;
    const float oldTime = g_Time_AccumulatedTimeSec;
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_NodePartial rootNode = {};
    rootNode.classId = 2;
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;

    zClass_NodePartial runtimeNode = {};
    zEffectAnimEntry recentHitEffect = {};
    std::strcpy(recentHitEffect.name, "recent_hit");
    recentHitEffect.boundNode = &rootNode;
    recentHitEffect.runtimeNode = &runtimeNode;
    recentHitEffect.callbackNode = &rootNode;
    recentHitEffect.priority = 3;

    zClass_NodePartial oldRuntimeNode = {};
    zEffectAnimEntry oldHandle = {};
    oldHandle.activationState = 2;
    oldHandle.runtimeNode = &oldRuntimeNode;
    oldHandle.triggerBaseValue = 1.0f;
    oldHandle.triggerCurrentValue = 9.0f;
    playerState.recentHitLightHandle = &oldHandle;

    int owner = 0;
    OptCatalogEntryDef hitSource = {};
    g_PlayerRecentHitFxAnimEntry = &recentHitEffect;
    g_OptCatalog_CurrentDamageOwnerOrCtx = &owner;
    g_Time_AccumulatedTimeSec = 12.5f;
    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    zEffect_Anim::ClearActivationRecords();

    Player::RecordRecentHitFeedback(&saveState, &hitSource, 3.25f);

    const bool stateOk =
        playerState.recentHitValid == 1 && playerState.lastHitOwnerOrCtx == &owner &&
        playerState.recentHitSource == &hitSource && playerState.recentHitDamage == 3.25f &&
        playerState.recentHitFxExpireTime == 16.5f &&
        playerState.recentHitLightHandle == &recentHitEffect;
    const bool oldHandleStopped =
        oldHandle.triggerCurrentValue == 0.0f && oldRuntimeNode.actionCallback != 0;
    const bool effectOk =
        recentHitEffect.activationState == 2 &&
        recentHitEffect.resetScratch[0] ==
            static_cast<unsigned int>(reinterpret_cast<std::uintptr_t>(&rootNode));

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
    g_PlayerRecentHitFxAnimEntry = oldRecentHitFxAnimEntry;
    g_OptCatalog_CurrentDamageOwnerOrCtx = oldOwnerOrCtx;
    g_Time_AccumulatedTimeSec = oldTime;

    return stateOk && oldHandleStopped && effectOk ? 0 : 1;
}

extern "C" int player_update_timed_hit_status_from_source_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData masterCommonData = {};
    zClass_NodePartial lightNode = {};
    zClass_NodePartial lightParent = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &masterCommonData;
    playerState.timedHitStatus.lightNode = &lightNode;
    playerState.timedHitStatus.lightParentNode = &lightParent;

    OptCatalogEntryDef fixedSource = {};
    fixedSource.flags = 0x800u;
    masterCommonData.maxHealth = 25.0f;
    const float fixedReturn =
        Player::UpdateTimedHitStatusFromHitSource(&saveState, &fixedSource, 3.5f);
    const bool fixedOk =
        fixedReturn == 3.5f && playerState.timedHitStatus.hitSource == &fixedSource &&
        playerState.timedHitStatus.targetLevel == 1.0f;

    OptCatalogEntryDef scaledSource = {};
    masterCommonData.invMaxHealth = 0.25f;
    playerState.timedHitStatus = {};
    playerState.timedHitStatus.currentLevel = 0.25f;
    playerState.timedHitStatus.lightNode = &lightNode;
    playerState.timedHitStatus.lightParentNode = &lightParent;
    const float scaledReturn =
        Player::UpdateTimedHitStatusFromHitSource(&saveState, &scaledSource, 2.0f);
    const bool scaledOk =
        scaledReturn == 2.0f && playerState.timedHitStatus.hitSource == &scaledSource &&
        playerState.timedHitStatus.targetLevel == 0.5f;

    playerState.timedHitStatus = {};
    playerState.timedHitStatus.currentLevel = 0.0f;
    playerState.timedHitStatus.lightNode = &lightNode;
    playerState.timedHitStatus.lightParentNode = &lightParent;
    const float suppressedReturn =
        Player::UpdateTimedHitStatusFromHitSource(&saveState, &scaledSource, 2.0f);
    const bool suppressedOk =
        suppressedReturn == 0.0f && playerState.timedHitStatus.targetLevel == 0.5f;

    return fixedOk && scaledOk && suppressedOk ? 0 : 1;
}

extern "C" int player_clear_destroyed_respawn_effect_handle_callback_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zEffectAnimEntry effect = {};
    zEffectAnimEntry handle = {};
    saveState.playerState = &playerState;
    playerState.destroyedRespawnAsyncHandle = &handle;

    Player::ClearDestroyedRespawnEffectHandleCallback(&effect, &saveState, 17);
    return playerState.destroyedRespawnAsyncHandle == 0 ? 0 : 1;
}

extern "C" int player_hit_callback_record_net_context_and_timed_status_smoke(void) {
    void *const oldOwnerOrCtx = g_OptCatalog_CurrentDamageOwnerOrCtx;
    const float oldTime = g_Time_AccumulatedTimeSec;
    zEffectAnimEntry *const oldRecentHitFxAnimEntry = g_PlayerRecentHitFxAnimEntry;
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zUtil_SaveGameState inactiveSave = {};
    zUtil_PlayerStateStorage inactiveState = {};
    inactiveSave.playerState = &inactiveState;
    inactiveState.lifecycleState = 4;
    inactiveState.recentHitValid = 1;
    const bool inactiveOk =
        Player::HitCallback_RecordNetContextAndTimedStatus(
            &inactiveSave,
            0,
            0,
            5.0f
        ) == 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial lightNode = {};
    zClass_NodePartial lightParent = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.lifecycleState = 1;
    playerState.recentHitValid = 7;
    const bool nullSourceOk =
        Player::HitCallback_RecordNetContextAndTimedStatus(
            &saveState,
            0,
            0,
            2.0f
        ) == 7;

    OptCatalogEntryDef timedSource = {};
    timedSource.flags = 0x200000u;
    commonData.invMaxHealth = 0.25f;
    playerState.timedHitStatus = {};
    playerState.timedHitStatus.currentLevel = 0.25f;
    playerState.timedHitStatus.lightNode = &lightNode;
    playerState.timedHitStatus.lightParentNode = &lightParent;
    playerState.recentHitValid = 5;
    const int timedResult =
        Player::HitCallback_RecordNetContextAndTimedStatus(
            &saveState,
            &timedSource,
            0,
            2.0f
        );
    const bool timedOk =
        timedResult == 5 && playerState.timedHitStatus.hitSource == &timedSource &&
        playerState.timedHitStatus.targetLevel == 0.5f;

    zClass_NodePartial rootNode = {};
    zClass_NodePartial runtimeNode = {};
    zEffectAnimEntry recentHitEffect = {};
    rootNode.classId = 2;
    InitDestroyedEffectEntry(&recentHitEffect, &rootNode, &runtimeNode, "recent_hit");
    recentHitEffect.priority = 3;
    OptCatalogEntryDef recentSource = {};
    recentSource.flags = 0x1000u;
    int owner = 0;
    playerState.rootNode = &rootNode;
    playerState.recentHitValid = 0;
    g_PlayerRecentHitFxAnimEntry = &recentHitEffect;
    g_OptCatalog_CurrentDamageOwnerOrCtx = &owner;
    g_Time_AccumulatedTimeSec = 20.0f;
    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    zEffect_Anim::ClearActivationRecords();
    const int recentResult =
        Player::HitCallback_RecordNetContextAndTimedStatus(
            &saveState,
            &recentSource,
            0,
            3.0f
        );
    const bool recentOk =
        recentResult == 1 && playerState.recentHitValid == 1 &&
        playerState.lastHitOwnerOrCtx == &owner && playerState.recentHitSource == &recentSource &&
        playerState.recentHitDamage == 3.0f && playerState.recentHitFxExpireTime == 24.0f &&
        playerState.recentHitLightHandle == &recentHitEffect;

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
    g_PlayerRecentHitFxAnimEntry = oldRecentHitFxAnimEntry;
    g_OptCatalog_CurrentDamageOwnerOrCtx = oldOwnerOrCtx;
    g_Time_AccumulatedTimeSec = oldTime;

    return inactiveOk && nullSourceOk && timedOk && recentOk ? 0 : 1;
}

extern "C" int player_hit_callback_record_context_and_timed_status_smoke(void) {
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    const float oldDamageFeedbackScalar = g_OptCatalogDamageFeedbackIntensityScalar;
    const int oldDamageContextKind = g_OptCatalog_DamageContextKind;
    void *const oldDamageContextHitEvent = g_OptCatalog_DamageContextHitEvent;
    const zVec3 oldCapturedSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldCapturedHitPos = g_OptCatalog_CapturedDamageHitPos;

    zUtil_SaveGameState inactiveSave = {};
    zUtil_PlayerStateStorage inactiveState = {};
    PlayerModalState inactiveModalState = {};
    PlayerMasterCommonData inactiveCommonData = {};
    PlayerMasterModalData inactiveModalData = {};
    inactiveSave.playerState = &inactiveState;
    inactiveSave.primaryModalState = &inactiveModalState;
    inactiveModalState.masterModalData = &inactiveModalData;
    inactiveState.masterCommonData = &inactiveCommonData;
    inactiveState.lifecycleState = 4;
    const int inactiveResult =
        Player::HitCallback_RecordContextAndTimedStatus(&inactiveSave, 0, 0, 5.0f);
    const bool inactiveOk = inactiveResult == 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData modalData = {};
    zClass_NodePartial rootNode = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial hitNode = {};
    OptCatalogEntryDef hitSource = {};
    int matrixFlags[3] = {};
    float *matrixSlots[3] = {};
    zMat4x3 currentMatrix = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.masterCommonData = &commonData;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;
    modalData.invMass = 2.0f;
    playerState.lifecycleState = 1;
    playerState.statusMeterValue = 50.0f;
    playerState.localVel = {5.0f, 0.0f, 2.0f};
    playerState.rootNode = &rootNode;
    playerState.selectedProbeSample.node = &hitNode;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[4] = 1.0f;
    rootData.localMatrix[8] = 1.0f;

    matrixSlots[0] = reinterpret_cast<float *>(&currentMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    zMath::MatLoadIdentity();

    g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;
    g_OptCatalog_DamageContextKind = -1;
    g_OptCatalog_DamageContextHitEvent = 0;
    g_OptCatalog_CapturedDamageSourcePos = {10.0f, 0.0f, 0.0f};
    g_OptCatalog_CapturedDamageHitPos = {0.0f, 0.0f, 0.0f};

    const int liveResult =
        Player::HitCallback_RecordContextAndTimedStatus(
            &saveState,
            &hitSource,
            &hitNode,
            10.0f
        );
    const bool liveDamageOk =
        liveResult == 0 && FloatNear(playerState.statusMeterValue, 40.0f) &&
        FloatNear(playerState.statusMeterScaled, 0.4f) &&
        FloatNear(g_OptCatalogDamageFeedbackIntensityScalar, 0.4f) &&
        g_OptCatalog_DamageContextKind == 0 &&
        g_OptCatalog_DamageContextHitEvent == &playerState.selectedProbeSample &&
        FloatNear(playerState.vehiclePitchRad, 0.0f) &&
        FloatNear(playerState.vehicleRollRad, 0.5f) &&
        FloatNear(playerState.localVel.x, -28.340002f) &&
        FloatNear(playerState.localVel.z, 2.0f) &&
        zMath::g_currentMatrixIdentityFlagSlot == &matrixFlags[0] &&
        zMath::g_currentMatrixPtrSlot == &matrixSlots[0];

    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    g_OptCatalog_CapturedDamageSourcePos = oldCapturedSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldCapturedHitPos;
    g_OptCatalog_DamageContextHitEvent = oldDamageContextHitEvent;
    g_OptCatalog_DamageContextKind = oldDamageContextKind;
    g_OptCatalogDamageFeedbackIntensityScalar = oldDamageFeedbackScalar;

    return inactiveOk && liveDamageOk ? 0 : 1;
}

extern "C" int player_enter_local_inactive_destroyed_lifecycle_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState otherSaveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial destroyedRuntimeNode = {};
    zClass_NodePartial oldBubbleRuntimeNode = {};
    zEffectAnimEntry destroyedRespawn = {};
    zEffectAnimEntry oldBubble = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.destroyedRespawnFxEntry = &destroyedRespawn;
    rootNode.classId = 2;
    InitDestroyedEffectEntry(
        &destroyedRespawn,
        &rootNode,
        &destroyedRuntimeNode,
        "destroyed_respawn"
    );
    InitDestroyedEffectEntry(&oldBubble, &rootNode, &oldBubbleRuntimeNode, "bft_bubble");
    oldBubble.activationState = 2;
    playerState.masterTypeTransitionToSubLightHandle = &oldBubble;

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&otherSaveState);
    playerState.lifecycleState = 99;
    playerState.altGunTransitionState = 7;
    playerState.altGunTransitionController = &playerState.altWeaponBanks[2].controllerA;
    playerState.altGunTransitionTimerA = 3.0f;
    Player::EnterLocalInactiveDestroyedLifecycle(&saveState);
    const bool nonLocalOk =
        playerState.lifecycleState == 99 && playerState.altGunTransitionState == 7 &&
        playerState.altGunTransitionController == &playerState.altWeaponBanks[2].controllerA &&
        playerState.altGunTransitionTimerA == 3.0f &&
        destroyedRespawn.activationState == 0 &&
        playerState.masterTypeTransitionToSubLightHandle == &oldBubble;

    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    playerState.lifecycleState = 2;
    playerState.altGunTransitionState = 7;
    playerState.altGunTransitionController = &playerState.altWeaponBanks[3].controllerB;
    playerState.altGunTransitionTimerA = 4.0f;
    playerState.cameraTransitionTimer = 77;
    oldBubble.activationState = 2;
    playerState.masterTypeTransitionToSubLightHandle = &oldBubble;
    Player::EnterLocalInactiveDestroyedLifecycle(&saveState);
    const bool localOk =
        playerState.lifecycleState == 4 && playerState.altGunTransitionState == 1 &&
        playerState.altGunTransitionController == 0 &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.masterTypeTransitionToSubLightHandle == 0 &&
        destroyedRespawn.activationState == 2 && destroyedRespawn.velocityX == 0.0f &&
        destroyedRespawn.velocityY == 0.0f && destroyedRespawn.velocityZ == 0.0f &&
        playerState.cameraTransitionTimer == 77 && destroyedRespawn.eventCallback == 0 &&
        destroyedRespawn.eventCallbackContext == 0;

    zEffect_Anim::ClearActivationRecords();
    InitDestroyedEffectEntry(
        &destroyedRespawn,
        &rootNode,
        &destroyedRuntimeNode,
        "destroyed_respawn"
    );
    oldBubble = {};
    InitDestroyedEffectEntry(&oldBubble, &rootNode, &oldBubbleRuntimeNode, "bft_bubble");
    oldBubble.activationState = 2;
    playerState.destroyedRespawnFxEntry = &destroyedRespawn;
    playerState.masterTypeTransitionToSubLightHandle = &oldBubble;
    playerState.cameraTransitionTimer = 0;
    networkEnabled = 1;
    Player::EnterLocalInactiveDestroyedLifecycle(&saveState);
    const bool networkOk =
        playerState.cameraTransitionTimer == 1 &&
        destroyedRespawn.eventCallback ==
            reinterpret_cast<zEffectAnimEventCallback>(&Player::DestroyedStateResetCallback) &&
        destroyedRespawn.eventCallbackContext == &saveState;

    zEffect_Anim::ClearActivationRecords();
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    if (!nonLocalOk) {
        return 1;
    }
    if (!localOk) {
        return 2;
    }
    return networkOk ? 0 : 3;
}

extern "C" int player_enter_destroyed_state_smoke(void) {
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    const int oldDamageContextKind = g_OptCatalog_DamageContextKind;
    void *const oldDamageContextHitEvent = g_OptCatalog_DamageContextHitEvent;
    const zVec3 oldCapturedSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldCapturedHitPos = g_OptCatalog_CapturedDamageHitPos;
    const int oldDamageMaskEnabled = g_OptCatalogDamageMaskEnabled;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    const int oldForceFeedbackCaps = g_zInput_JoystickCaps_ForceFeedback;
    zInput_FFEffectSet *const oldEffectSet = g_zInputFfEffectSet;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    const unsigned int oldInvalidateMask = g_HudUi_InvalidateMask;

    zUtil_SaveGameState inactiveSave = {};
    zUtil_PlayerStateStorage inactiveState = {};
    PlayerModalState inactiveModalState = {};
    PlayerMasterCommonData inactiveCommonData = {};
    PlayerMasterModalData inactiveModalData = {};
    inactiveSave.playerState = &inactiveState;
    inactiveSave.primaryModalState = &inactiveModalState;
    inactiveModalState.masterModalData = &inactiveModalData;
    inactiveState.masterCommonData = &inactiveCommonData;
    inactiveState.lifecycleState = 4;
    inactiveState.statusMeterValue = 25.0f;
    const int inactiveResult = Player::EnterDestroyedState(&inactiveSave, 0, 0, 10.0f);
    int failureCode = 0;
    if (inactiveResult != 0 || !FloatNear(inactiveState.statusMeterValue, 25.0f)) {
        failureCode = 1;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData modalData = {};
    HudUiShieldMessageWidget shield = {};
    zClass_NodePartial rootNode = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial hitNode = {};
    OptCatalogEntryDef hitSource = {};
    int matrixFlags[3] = {};
    float *matrixSlots[3] = {};
    zMat4x3 currentMatrix = {};
    int joystickEnabled = 0;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.masterCommonData = &commonData;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;
    modalData.invMass = 2.0f;
    hitSource.flags = 2;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
    playerState.lifecycleState = 1;
    playerState.statusMeterValue = 50.0f;
    playerState.statusMeterScaled = 0.5f;
    playerState.localVel = {5.0f, 0.0f, 2.0f};
    playerState.rootNode = &rootNode;
    playerState.selectedProbeSample.node = &hitNode;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[4] = 1.0f;
    rootData.localMatrix[8] = 1.0f;

    matrixSlots[0] = reinterpret_cast<float *>(&currentMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    zMath::MatLoadIdentity();

    g_OptCatalog_DamageContextKind = -1;
    g_OptCatalog_DamageContextHitEvent = 0;
    g_OptCatalog_CapturedDamageSourcePos = {10.0f, 0.0f, 0.0f};
    g_OptCatalog_CapturedDamageHitPos = {0.0f, 0.0f, 0.0f};
    g_OptCatalogDamageMaskEnabled = 0;
    ZOPT_INPUT_JOYSTICK = &joystickEnabled;
    g_zInput_JoystickCaps_ForceFeedback = 0;
    g_zInputFfEffectSet = 0;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUi_InvalidateMask = 0;

    if (failureCode == 0) {
        const int liveResult = Player::EnterDestroyedState(&saveState, &hitSource, 0, 10.0f);
        if (liveResult != 0) {
            failureCode = 2;
        } else if (!FloatNear(playerState.statusMeterValue, 40.0f)) {
            failureCode = 3;
        } else if (!FloatNear(playerState.statusMeterScaled, 1.0f)) {
            failureCode = 4;
        } else if (!FloatNear(g_PlayerStatusMeterRatio, 0.4f)) {
            failureCode = 5;
        } else if (g_OptCatalog_DamageContextKind != 0 ||
                   g_OptCatalog_DamageContextHitEvent != &playerState.selectedProbeSample) {
            failureCode = 6;
        } else if (!FloatNear(playerState.vehiclePitchRad, 0.0f)) {
            failureCode = 7;
        } else if (!FloatNear(playerState.vehicleRollRad, 0.0f)) {
            failureCode = 8;
        } else if (!FloatNear(playerState.localVel.x, 5.0f)) {
            failureCode = 9;
        } else if (!FloatNear(playerState.localVel.z, 2.0f)) {
            failureCode = 10;
        } else if (zMath::g_currentMatrixIdentityFlagSlot != &matrixFlags[0] ||
                   zMath::g_currentMatrixPtrSlot != &matrixSlots[0]) {
            failureCode = 11;
        }
    }

    g_zInputFfEffectSet = oldEffectSet;
    g_zInput_JoystickCaps_ForceFeedback = oldForceFeedbackCaps;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_OptCatalogDamageMaskEnabled = oldDamageMaskEnabled;
    g_OptCatalog_CapturedDamageSourcePos = oldCapturedSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldCapturedHitPos;
    g_OptCatalog_DamageContextHitEvent = oldDamageContextHitEvent;
    g_OptCatalog_DamageContextKind = oldDamageContextKind;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;

    return failureCode;
}

extern "C" int player_apply_status_meter_change_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    HudUiShieldMessageWidget shield = {};

    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    commonData.maxHealth = 80.0f;
    commonData.invMaxHealth = 0.0125f;
    playerState.statusMeterValue = 30.0f;

    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;

    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_PlayerStatusMeterRatio = -1.0f;
    g_HudUi_InvalidateMask = 0;

    Player::ApplyStatusMeterChange(&saveState, 0, 60.0f);
    const bool replaceOk =
        playerState.statusMeterValue == 60.0f && g_PlayerStatusMeterRatio == 0.75f &&
        shield.meter.points[0].y == 85.0f &&
        std::strcmp(shield.percentTextPanel.cachedText, "75") == 0;

    Player::ApplyStatusMeterChange(&saveState, 1, 50.0f);
    const bool maxClampOk =
        playerState.statusMeterValue == 80.0f && g_PlayerStatusMeterRatio == 1.0f &&
        shield.meter.points[0].y == 80.0f &&
        std::strcmp(shield.percentTextPanel.cachedText, "100") == 0;

    Player::ApplyStatusMeterChange(&saveState, 0, -7.0f);
    const bool minClampOk =
        playerState.statusMeterValue == 0.0f && g_PlayerStatusMeterRatio == 0.0f &&
        shield.meter.points[0].y == 100.0f &&
        std::strcmp(shield.percentTextPanel.cachedText, "0") == 0;

    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    return replaceOk && maxClampOk && minClampOk ? 0 : 1;
}

extern "C" int player_update_status_meter_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiTextStack4 topStack = {};

    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;

    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;

    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiTopMessageStack = &topStack;
    g_HudUi_InvalidateMask = 0;

    playerState.statusMeterValue = 20.0f;
    g_PlayerStatusMeterRatio = 0.2f;
    const int addResult = Player::UpdateStatusMeter(&saveState, 1, 25.0f);
    const bool addOk =
        addResult == 1 && FloatNear(playerState.statusMeterValue, 45.0f) &&
        FloatNear(g_PlayerStatusMeterRatio, 0.45f) &&
        std::strcmp(shield.percentTextPanel.cachedText, "45") == 0;

    playerState.statusMeterValue = 10.0f;
    playerState.damageProtectionActive = 1;
    playerState.queuedFixedDamageFlag = 1;
    playerState.damageVisualFlag = 1;
    g_PlayerStatusMeterRatio = 0.1f;
    const int refillResult = Player::UpdateStatusMeter(&saveState, 0, 0.0f);
    const bool refillOk =
        refillResult == 1 && playerState.statusMeterValue == 100.0f &&
        FloatNear(g_PlayerStatusMeterRatio, 1.0f) && playerState.damageProtectionActive == 0 &&
        playerState.queuedFixedDamageFlag == 0 && playerState.damageVisualFlag == 0 &&
        std::strcmp(shield.percentTextPanel.cachedText, "100") == 0;

    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiTopMessageStack = oldTopStack;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    return addOk && refillOk ? 0 : 1;
}

extern "C" int player_reset_damage_state_and_timed_hit_status_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.damageProtectionActive = 11;
    playerState.queuedFixedDamageFlag = 12;
    playerState.damageVisualFlag = 13;
    playerState.timedHitStatus.runtimeFlags = 3;
    playerState.timedHitStatus.currentLevel = 0.25f;
    playerState.timedHitStatus.targetLevel = -0.5f;
    playerState.timedHitStatus.nextUpdateTime = 7.0f;
    playerState.timedHitStatus.lightNode = 0;

    Player::ResetDamageStateAndTimedHitStatus(&saveState);

    return playerState.damageProtectionActive == 0 &&
                   playerState.queuedFixedDamageFlag == 0 &&
                   playerState.damageVisualFlag == 0 &&
                   playerState.timedHitStatus.runtimeFlags == 3 &&
                   playerState.timedHitStatus.currentLevel == 0.25f &&
                   playerState.timedHitStatus.targetLevel == -0.5f &&
                   playerState.timedHitStatus.nextUpdateTime == 7.0f
               ? 0
               : 1;
}

extern "C" int player_reset_damage_visuals_and_timed_status_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const float oldTime = g_Time_AccumulatedTimeSec;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const float oldLowMeterNextBeepTime = g_Hud_LowMeterNextBeepTime;
    const float oldLowMeterBeepInterval = g_Hud_LowMeterBeepInterval;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial rootNode = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.statusMeterValue = 10.0f;
    playerState.recentHitValid = 1;
    playerState.recentHitFxExpireTime = 5.0f;
    playerState.recentHitLightHandle = 0;

    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_Time_AccumulatedTimeSec = 10.0f;
    g_PlayerStatusMeterRatio = 0.2f;
    g_Hud_LowMeterNextBeepTime = 20.0f;
    g_Hud_LowMeterBeepInterval = 3.0f;

    Player::ResetDamageVisualsAndTimedStatus(&saveState);

    const bool cleanupOk =
        playerState.recentHitValid == 0 && playerState.recentHitLightHandle == 0 &&
        g_Hud_LowMeterNextBeepTime == 20.0f;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_Time_AccumulatedTimeSec = oldTime;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_Hud_LowMeterNextBeepTime = oldLowMeterNextBeepTime;
    g_Hud_LowMeterBeepInterval = oldLowMeterBeepInterval;
    return cleanupOk ? 0 : 1;
}

extern "C" int player_apply_pitch_roll_velocity_impulse_from_direction_smoke(void) {
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_NodePartial rootNode = {};
    zClass_Object3DDataPartial rootData = {};
    int matrixFlags[3] = {};
    float *matrixSlots[3] = {};
    zMat4x3 currentMatrix = {};

    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[1] = 2.0f;
    rootData.localMatrix[2] = 3.0f;
    rootData.localMatrix[3] = 4.0f;
    rootData.localMatrix[4] = 5.0f;
    rootData.localMatrix[5] = 6.0f;
    rootData.localMatrix[6] = 7.0f;
    rootData.localMatrix[7] = 8.0f;
    rootData.localMatrix[8] = 9.0f;
    rootData.localMatrix[9] = 100.0f;
    rootData.localMatrix[10] = 200.0f;
    rootData.localMatrix[11] = 300.0f;

    playerState.vehiclePitchRad = 10.0f;
    playerState.vehicleRollRad = 20.0f;
    playerState.localVel = {5.0f, 6.0f, -1.0f};

    const zMat4x3 currentBefore = {0.5f, 0.0f, 0.0f, 0.0f, 0.75f, 0.0f,
                                   0.0f, 0.0f, 1.25f, 7.0f, 8.0f, 9.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&currentMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    zMath::MatLoadCurrentFrom(&currentBefore);

    const zVec3 direction = {1.0f, 2.0f, 3.0f};
    Player::ApplyPitchRollVelocityImpulseFromDirection(&saveState, &direction, 0.5f, 0.25f);

    zMat4x3 currentAfter = {};
    zMath::MatCopyCurrentTo(&currentAfter);

    const bool impulseApplied =
        FloatNear(playerState.vehiclePitchRad, -11.0f) &&
        FloatNear(playerState.vehicleRollRad, 35.0f) &&
        FloatNear(playerState.localVel.x, -2.5f) &&
        FloatNear(playerState.localVel.y, 6.0f) &&
        FloatNear(playerState.localVel.z, -11.5f);
    const bool stackRestored =
        FloatNear(currentAfter.xx, currentBefore.xx) &&
        FloatNear(currentAfter.yy, currentBefore.yy) &&
        FloatNear(currentAfter.zz, currentBefore.zz) &&
        FloatNear(currentAfter.posX, currentBefore.posX) &&
        FloatNear(currentAfter.posY, currentBefore.posY) &&
        FloatNear(currentAfter.posZ, currentBefore.posZ) &&
        zMath::g_currentMatrixIdentityFlagSlot == &matrixFlags[0] &&
        zMath::g_currentMatrixPtrSlot == &matrixSlots[0];

    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;

    return impulseApplied && stackRestored ? 0 : 2;
}

extern "C" int player_start_destroyed_state_vehicle_effect_smoke(void) {
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;
    HudUiSlot *const oldTrackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    const HudUiMeter oldSensorMeter = g_HudUiMgrSensorMeter;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "destroyed_root");
    rootNode.classId = 2;

    HudUiMgrSensorTrackNode trackNode = {};
    HudUiSlot trackedSlot = {};
    trackedSlot.trackNode = &trackNode;

    g_HudUiMgrSensorMeter.flags = 0;

    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    zEffect_Anim::ClearActivationRecords();

    bool ok = true;
    for (int scenario = 0; scenario < 5; ++scenario) {
        zUtil_SaveGameState saveState = {};
        zUtil_PlayerStateStorage playerState = {};
        saveState.playerState = &playerState;
        playerState.rootNode = &rootNode;

        zClass_NodePartial runtimeNodes[6] = {};
        zEffectAnimEntry napalm = {};
        zEffectAnimEntry shatter = {};
        zEffectAnimEntry shock = {};
        zEffectAnimEntry subTransition = {};
        zEffectAnimEntry destroyedRespawn = {};
        zEffectAnimEntry oldRecentHitHandle = {};
        InitDestroyedEffectEntry(&napalm, &rootNode, &runtimeNodes[0], "napalm_vehicle");
        InitDestroyedEffectEntry(&shatter, &rootNode, &runtimeNodes[1], "shatter_vehicle");
        InitDestroyedEffectEntry(&shock, &rootNode, &runtimeNodes[2], "shock_vehicle");
        InitDestroyedEffectEntry(&subTransition, &rootNode, &runtimeNodes[3], "sub_transition");
        InitDestroyedEffectEntry(
            &destroyedRespawn,
            &rootNode,
            &runtimeNodes[4],
            "destroyed_respawn"
        );
        InitDestroyedEffectEntry(&oldRecentHitHandle, &rootNode, &runtimeNodes[5], "recent_hit");
        oldRecentHitHandle.activationState = 2;
        oldRecentHitHandle.triggerBaseValue = 1.0f;
        oldRecentHitHandle.triggerCurrentValue = 9.0f;

        playerState.napalmVehicleFxEntry = &napalm;
        playerState.shatterVehicleFxEntry = &shatter;
        playerState.shockVehicleFxEntry = &shock;
        playerState.subTransitionFxEntry = &subTransition;
        playerState.destroyedRespawnFxEntry = &destroyedRespawn;
        playerState.destroyedRespawnAsyncHandle = &oldRecentHitHandle;

        zEffectAnimEntry *expected = &destroyedRespawn;
        if (scenario == 0) {
            playerState.queuedFixedDamageFlag = 1;
            playerState.recentHitValid = 1;
            playerState.recentHitLightHandle = &oldRecentHitHandle;
            expected = &shock;
            trackNode.payload = &saveState;
            g_HudUiMgrSensorTrackedProgressSlot = &trackedSlot;
        } else if (scenario == 1) {
            playerState.damageProtectionActive = 1;
            expected = &shatter;
            g_HudUiMgrSensorTrackedProgressSlot = 0;
        } else if (scenario == 2) {
            playerState.recentHitValid = 1;
            playerState.recentHitLightHandle = &oldRecentHitHandle;
            expected = &napalm;
            g_HudUiMgrSensorTrackedProgressSlot = 0;
        } else if (scenario == 3) {
            playerState.aiMode = 1;
            expected = &subTransition;
            g_HudUiMgrSensorTrackedProgressSlot = 0;
        } else {
            g_HudUiMgrSensorTrackedProgressSlot = 0;
        }

        void *const callback = scenario == 0 ? (void *)(&PlayerDestroyedEffectDoneCallback) : 0;
        Player::StartDestroyedStateVehicleEffect(&saveState, callback);

        ok = ok && playerState.destroyedRespawnAsyncHandle == expected &&
             expected->activationState == 2 && expected->velocityX == 0.0f &&
             expected->velocityY == 0.0f && expected->velocityZ == 0.0f;
        if (scenario == 0 || scenario == 2) {
            ok = ok && playerState.recentHitValid == 0 && playerState.recentHitLightHandle == 0 &&
                 oldRecentHitHandle.triggerCurrentValue == 0.0f;
        }
        if (scenario == 0) {
            ok = ok && expected->eventCallback == &PlayerDestroyedEffectDoneCallback &&
                 expected->eventCallbackContext == &saveState &&
                 (g_HudUiMgrSensorMeter.flags & 0x10u) != 0;
        }

        zEffect_Anim::ClearActivationRecords();
    }

    g_HudUiMgrSensorTrackedProgressSlot = oldTrackedProgressSlot;
    g_HudUiMgrSensorMeter = oldSensorMeter;
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;

    return ok ? 0 : 1;
}

extern "C" int player_apply_damage_local_smoke(void) {
    const int oldQueueEnabled = g_zEffectAnim_RecordQueueEnabled;
    const int oldDispatchEnabled = g_zEffectAnim_DispatchEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;
    HudUiSlot *const oldTrackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    const HudUiMeter oldSensorMeter = g_HudUiMgrSensorMeter;
    const int oldDamageContextKind = g_OptCatalog_DamageContextKind;
    void *const oldDamageContextHitEvent = g_OptCatalog_DamageContextHitEvent;
    const float oldDamageFeedbackScalar = g_OptCatalogDamageFeedbackIntensityScalar;

    zUtil_SaveGameState liveSaveState = {};
    zUtil_PlayerStateStorage livePlayerState = {};
    PlayerMasterCommonData liveCommonData = {};
    liveSaveState.playerState = &livePlayerState;
    livePlayerState.masterCommonData = &liveCommonData;
    liveCommonData.invMaxHealth = 0.025f;
    livePlayerState.statusMeterValue = 20.0f;
    g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;

    const int liveResult = Player::ApplyDamageLocal(&liveSaveState);
    const bool liveOk =
        liveResult == 0 && FloatNear(g_OptCatalogDamageFeedbackIntensityScalar, 0.5f) &&
        livePlayerState.destroyedRespawnAsyncHandle == 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial destroyedRuntimeNode = {};
    zClass_NodePartial recentRuntimeNode = {};
    zEffectAnimEntry destroyedRespawn = {};
    zEffectAnimEntry recentHitHandle = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.activeAltGunController = &playerState.altWeaponBanks[1].controllerA;
    playerState.statusMeterValue = 0.0f;
    playerState.destroyedRespawnFxEntry = &destroyedRespawn;
    playerState.recentHitValid = 1;
    playerState.recentHitLightHandle = &recentHitHandle;
    playerState.altGunTransitionState = 12;
    playerState.altGunTransitionController = &playerState.altWeaponBanks[2].controllerB;
    playerState.altGunTransitionTimerA = 3.0f;
    playerState.selectedProbeSample.node = &rootNode;
    rootNode.classId = 2;

    InitDestroyedEffectEntry(
        &destroyedRespawn,
        &rootNode,
        &destroyedRuntimeNode,
        "destroyed_respawn"
    );
    InitDestroyedEffectEntry(&recentHitHandle, &rootNode, &recentRuntimeNode, "recent_hit");
    recentHitHandle.activationState = 2;
    recentHitHandle.triggerCurrentValue = 8.0f;

    std::uintptr_t visibleTable[25] = {};
    visibleTable[24] = MethodAddress(&PlayerDamageHitHudVisibleReceiver::SetVisible);
    HudUiMgrSensorTrackNode trackNode = {};
    HudUiSlot trackedSlot = {};
    trackedSlot.trackNode = &trackNode;
    trackNode.payload = &saveState;
    std::memset(
        g_PlayerDamageHitHudVisibleThis,
        0,
        sizeof(g_PlayerDamageHitHudVisibleThis)
    );
    std::memset(
    g_PlayerDamageHitHudVisibleValue,
        0,
        sizeof(g_PlayerDamageHitHudVisibleValue)
    );
    g_PlayerDamageHitHudVisibleCount = 0;
    g_HudUiMgrSensorMeter = {};
    *reinterpret_cast<std::uintptr_t **>(&g_HudUiMgrSensorMeter) = visibleTable;
    g_HudUiMgrSensorTrackedProgressSlot = &trackedSlot;

    g_zEffectAnim_RecordQueueEnabled = 1;
    g_zEffectAnim_DispatchEnabled = 0;
    zEffect_Anim::ClearActivationRecords();
    g_OptCatalog_DamageContextKind = -1;
    g_OptCatalog_DamageContextHitEvent = 0;

    const int depletedResult = Player::ApplyDamageLocal(&saveState);
    const bool depletedOk =
        depletedResult == 1 && playerState.destroyedRespawnAsyncHandle == &destroyedRespawn &&
        destroyedRespawn.activationState == 2 &&
        destroyedRespawn.eventCallback == (void *)(&Player::DestroyedStateRespawnCallback) &&
        destroyedRespawn.eventCallbackContext == &saveState && playerState.recentHitValid == 0 &&
        playerState.recentHitLightHandle == 0 &&
        recentHitHandle.triggerCurrentValue == 0.0f &&
        playerState.altGunTransitionState == 1 &&
        playerState.altGunTransitionController == 0 &&
        playerState.altGunTransitionTimerA == 0.0f && g_OptCatalog_DamageContextKind == 1 &&
        g_OptCatalog_DamageContextHitEvent == &playerState.selectedProbeSample &&
        g_PlayerDamageHitHudVisibleCount == 1 &&
        g_PlayerDamageHitHudVisibleThis[0] == &g_HudUiMgrSensorMeter &&
        g_PlayerDamageHitHudVisibleValue[0] == 0;

    zEffect_Anim::ClearActivationRecords();
    g_HudUiMgrSensorTrackedProgressSlot = oldTrackedProgressSlot;
    g_HudUiMgrSensorMeter = oldSensorMeter;
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCapacity = oldRecordCapacity;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_zEffectAnim_RecordQueueEnabled = oldQueueEnabled;
    g_zEffectAnim_DispatchEnabled = oldDispatchEnabled;
    g_OptCatalog_DamageContextKind = oldDamageContextKind;
    g_OptCatalog_DamageContextHitEvent = oldDamageContextHitEvent;
    g_OptCatalogDamageFeedbackIntensityScalar = oldDamageFeedbackScalar;

    return liveOk && depletedOk ? 0 : 1;
}
