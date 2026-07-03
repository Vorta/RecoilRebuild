#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zSound/zsnd.h"

#include <cstdint>
#include <cstring>

namespace {
using TestBackendSimpleFn = std::int32_t(__stdcall *)(void *self);
using TestBackendGetStatusFn = std::int32_t(__stdcall *)(void *self, std::int32_t *status);
using TestBackendPlayDirectSoundFn = std::int32_t(__stdcall *)(void *self,
                                                               std::uint32_t reserved1,
                                                               std::uint32_t reserved2,
                                                               std::uint32_t flags);
using TestBackendSetIntFn = std::int32_t(__stdcall *)(void *self, std::int32_t value);

struct TestDirectSoundBufferVTable {
    void *slots00_1c[8];
    void *GetFrequency;
    TestBackendGetStatusFn GetStatus;
    void *slot28;
    void *slot2c;
    TestBackendPlayDirectSoundFn Play;
    TestBackendSetIntFn SetCurrentPosition;
    void *slot38;
    TestBackendSetIntFn SetVolume;
    TestBackendSetIntFn SetPan;
    TestBackendSetIntFn SetFrequency;
    TestBackendSimpleFn Stop;
};

struct TestDirectSoundBuffer {
    TestDirectSoundBufferVTable *vtable;
};

bool FloatNear(float actual, float expected) {
    return actual > expected - 0.0001f && actual < expected + 0.0001f;
}

float PlayerDampingFactor(float rate, float deltaTime) {
    const int bits = static_cast<int>(-rate * deltaTime * 12102200.0f) + 0x3f800000;
    float factor = 0.0f;
    std::memcpy(&factor, &bits, sizeof(factor));
    return factor;
}

std::int32_t __stdcall TestDirectSoundGetStatus(void *, std::int32_t *status) {
    *status = 0;
    return 0;
}

std::int32_t __stdcall TestDirectSoundSetInt(void *, std::int32_t) {
    return 0;
}

std::int32_t __stdcall TestDirectSoundPlay(void *, std::uint32_t, std::uint32_t,
                                           std::uint32_t) {
    return 0;
}
} // namespace

extern "C" int player_start_slip_sfx_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    void *const oldGlobalVolumeScale = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 1.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterModalData modalData = {};
    modalData.sfxEngine[3] = &sample;
    PlayerModalState modalState = {};
    modalState.masterModalData = &modalData;
    zUtil_PlayerStateStorage playerState = {};
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    Player::StartSlipSfx(&saveState);

    const bool ok =
        playerState.slipSfxActive == 1 && modalState.modalSfxHandle[3] == &sample.primaryVoice;

    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScale;
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
    return ok ? 0 : 1;
}

extern "C" int player_stop_slip_sfx_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    zSndPlayHandle handle = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.slipSfxActive = 1;
    modalState.modalSfxHandle[3] = &handle;

    Player::StopSlipSfx(&saveState);
    return playerState.slipSfxActive == 0 && modalState.modalSfxHandle[3] == nullptr ? 0 : 1;
}

extern "C" int player_update_bank_and_turn_dynamics_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerInvDeltaTime = g_Player_InvDeltaTime;
    void *const oldGlobalVolumeScale = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldActiveBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.0f;
    if (Player::UpdateBankAndTurnDynamics(&saveState) != 0.0f) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        return 1;
    }

    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 1.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    modalData.sfxEngine[3] = &sample;

    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 2.0f;
    modalData.frictionStatic = 5.0f;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 2.0f};
    playerState.localVel = {0.0f, 0.0f, -3.0f};
    playerState.motionBasis.xy = 0.0f;

    const float staticResidual = Player::UpdateBankAndTurnDynamics(&saveState);
    if (!FloatNear(staticResidual, 7.0f) || playerState.slipSfxActive != 1 ||
        modalState.modalSfxHandle[3] != &sample.primaryVoice) {
        g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScale;
        g_zSnd_IsInitialized = oldInitialized;
        g_zSnd_PreInitialized = oldPreInitialized;
        g_zSnd_ActiveBackend = oldActiveBackend;
        g_zSnd_MuteDepth = oldMuteDepth;
        g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        return 2;
    }

    std::memset(&playerState, 0, sizeof(playerState));
    std::memset(&modalState, 0, sizeof(modalState));
    modalState.masterModalData = &modalData;
    saveState.primaryModalState = &modalState;
    modalData.frictionStatic = 20.0f;
    modalData.frictionDynamic = 2.0f;
    playerState.slipSfxActive = 1;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 2.0f};
    playerState.localVel = {1.0f, 0.0f, -1.0f};
    playerState.motionBasis.xy = 0.0f;

    const float dynamicResidual = Player::UpdateBankAndTurnDynamics(&saveState);

    g_zSnd_GlobalVolumeScalePtr = oldGlobalVolumeScale;
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldActiveBackend;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;
    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldPlayerInvDeltaTime;

    return FloatNear(dynamicResidual, 2.0f) ? 0 : 3;
}

extern "C" int player_compute_turn_slip_delta_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerInvDeltaTime = g_Player_InvDeltaTime;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 2.0f;
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.projectileSpawnVel = {2.0f, 0.0f, 6.0f};
    playerState.axisClampRuntime = 3.0f;
    playerState.throttleInputCopy = 1.0f;
    modalData.accelRate = 4.0f;
    modalData.frictionDynamic = 0.0f;
    modalData.frictionStatic = 100.0f;

    Player::ComputeTurnSlipDelta(&saveState);
    if (!FloatNear(playerState.localVel.x, 2.0f) ||
        !FloatNear(playerState.localVel.z, 3.0f)) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        return 1;
    }

    zSndPlayHandle handle = {};
    std::memset(&playerState, 0, sizeof(playerState));
    playerState.motionBasis.xx = 1.0f;
    playerState.motionBasis.yy = 1.0f;
    playerState.motionBasis.zz = 1.0f;
    playerState.projectileSpawnVel = {1.0f, 0.0f, 1.0f};
    playerState.axisClampRuntime = 10.0f;
    playerState.slipSfxActive = 1;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 4.0f};
    modalState.modalSfxHandle[3] = &handle;
    modalData.accelRate = 0.0f;
    modalData.frictionDynamic = 0.0f;
    modalData.frictionStatic = 100.0f;
    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 1.0f;

    Player::ComputeTurnSlipDelta(&saveState);

    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
    return FloatNear(playerState.localVel.x, 0.0f) && playerState.slipSfxActive == 0 &&
                   modalState.modalSfxHandle[3] == nullptr
               ? 0
               : 2;
}

extern "C" int player_update_yaw_velocity_from_steer_input_smoke(void) {
    const float oldPlayerDeltaTime = g_Player_DeltaTime;
    const float oldPlayerInvDeltaTime = g_Player_InvDeltaTime;
    const float oldPlayerDeltaTimeScaled001 = g_Player_DeltaTimeScaled001;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;

    g_Player_DeltaTime = 0.5f;
    g_Player_InvDeltaTime = 2.0f;
    g_Player_DeltaTimeScaled001 = 0.01f;
    g_GameStateOrMapTable = nullptr;

    playerState.localVel = {0.001f, 0.0f, -0.002f};
    playerState.axisClampRuntime = 5.0f;
    Player::UpdateYawVelocityFromSteerInput(&saveState);
    if (playerState.localVel.x != 0.0f || playerState.localVel.z != 0.0f) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        return 1;
    }

    std::memset(&playerState, 0, sizeof(playerState));
    playerState.localVel = {2.0f, 0.0f, 4.0f};
    playerState.throttleInput = 1.0f;
    playerState.axisClampRuntime = 6.0f;
    modalData.rateDampingAccel = 0.125f;
    modalData.rateDampingDecel = 0.25f;
    g_Player_DeltaTimeScaled001 = 0.0f;

    Player::UpdateYawVelocityFromSteerInput(&saveState);
    if (!FloatNear(playerState.localVel.x, 2.0f * PlayerDampingFactor(0.125f, 0.5f)) ||
        !FloatNear(playerState.localVel.z, 4.0f * PlayerDampingFactor(0.25f, 0.5f))) {
        g_Player_DeltaTime = oldPlayerDeltaTime;
        g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
        g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        return 2;
    }

    std::memset(&playerState, 0, sizeof(playerState));
    playerState.localVel = {1.0f, 0.0f, -3.0f};
    playerState.throttleInput = 1.0f;
    playerState.axisClampRuntime = 10.0f;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.bankBasis = {0.0f, 0.0f, 2.0f};
    modalData.rateDampingDecel = 0.0f;
    modalData.frictionDynamic = 1.0f;
    modalData.frictionStatic = 100.0f;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    Player::UpdateYawVelocityFromSteerInput(&saveState);

    g_Player_DeltaTime = oldPlayerDeltaTime;
    g_Player_InvDeltaTime = oldPlayerInvDeltaTime;
    g_Player_DeltaTimeScaled001 = oldPlayerDeltaTimeScaled001;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    return FloatNear(playerState.localVel.x, 6.5f) && FloatNear(playerState.localVel.z, -3.0f)
               ? 0
               : 3;
}

extern "C" int player_tick_alt_gun_runtime_state_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    const int oldPrimaryGunDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;
    int *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    PlayerProgressTargetSlotRuntime *const oldPendingSpawnTargetListPtr =
        g_OptCatalogPendingSpawnTargetListPtr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController controller = {};
    OptCatalogEntryDef entry = {};
    OptCatalogTrailRuntimeState trailRuntime = {};

    saveState.playerState = &playerState;
    playerState.activeAltGunController = &controller;
    playerState.altGunTransitionState = 1;
    playerState.altGunDispatchRequested = 1;
    playerState.altGunFireHeldFlag = 1;
    playerState.worldPos = {4.0f, 5.0f, 6.0f};
    playerState.steerBasisRaw = {0.0f, 0.0f, 1.0f};
    controller.optCatalogEntry = &entry;
    controller.trailRuntimeState = &trailRuntime;
    controller.ammoOrCharge = 2.0f;
    entry.fireRateInterval = 2.0f;

    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    g_FrameDeltaTimeSec = 0.5f;
    g_HudSensorTracker.primaryGunDispatchCount = 30;
    g_OptCatalogPendingSpawnTargetCountPtr = (int *)1;
    g_OptCatalogPendingSpawnTargetListPtr = (PlayerProgressTargetSlotRuntime *)1;

    Player::TickAltGunRuntimeState(&saveState);
    const bool heldAmmoOk =
        playerState.altGunDispatchRequested == 1 && playerState.altGunFireHeldFlag == 1 &&
        FloatNear(controller.ammoOrCharge, 1.75f) &&
        FloatNear(trailRuntime.ammoOrChargeMirror, 1.75f) &&
        g_HudSensorTracker.primaryGunDispatchCount == 31 &&
        g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
        g_OptCatalogPendingSpawnTargetListPtr == nullptr;

    zUtil_SaveGameState remoteSave = {};
    zUtil_PlayerStateStorage remotePlayer = {};
    PlayerGunFireController remoteController = {};
    zClass_NodePartial doorLeft = {};
    zClass_NodePartial doorRight = {};
    zClass_Object3DDataPartial doorLeftData = {};
    zClass_Object3DDataPartial doorRightData = {};
    remoteSave.playerState = &remotePlayer;
    remotePlayer.activeAltGunController = &remoteController;
    remotePlayer.altGunTransitionState = 8;
    remotePlayer.altGunTransitionTimerB = 0.2f;
    remotePlayer.doorLeftNode = &doorLeft;
    remotePlayer.doorRightNode = &doorRight;
    doorLeft.classId = 5;
    doorLeft.classData = &doorLeftData;
    doorRight.classId = 5;
    doorRight.classData = &doorRightData;
    g_GameStateOrMapTable = nullptr;
    g_FrameDeltaTimeSec = 0.1f;
    Player::TickAltGunRuntimeState(&remoteSave);
    const bool doorOpenOk =
        remotePlayer.altGunTransitionState == 16 &&
        remotePlayer.altGunTransitionTimerB == 0.0f &&
        doorLeftData.scale.x == 1.0f && doorLeftData.scale.y == 1.0f &&
        doorRightData.scale.x == 1.0f && doorRightData.scale.z == 1.0f &&
        g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
        g_OptCatalogPendingSpawnTargetListPtr == nullptr;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_FrameDeltaTimeSec = oldFrameDelta;
    g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    return heldAmmoOk && doorOpenOk ? 0 : 1;
}
