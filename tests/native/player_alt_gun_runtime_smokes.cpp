#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zSound/zSound.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {
void SetObjectLocalMatrix(zClass_Object3DDataPartial *data, const zMat4x3 &matrix) {
    std::memcpy(data->localMatrix, &matrix, sizeof(matrix));
}

bool Vec3Equals(const zVec3 &value, const zVec3 &expected) {
    return value.x == expected.x && value.y == expected.y && value.z == expected.z;
}

bool MatrixEquals(const zMat4x3 &value, const zMat4x3 &expected) {
    return value.xx == expected.xx && value.xy == expected.xy && value.xz == expected.xz &&
           value.yx == expected.yx && value.yy == expected.yy && value.yz == expected.yz &&
           value.zx == expected.zx && value.zy == expected.zy && value.zz == expected.zz &&
           value.posX == expected.posX && value.posY == expected.posY &&
           value.posZ == expected.posZ;
}

bool FloatNear(float actual, float expected) {
    return actual > expected - 0.0001f && actual < expected + 0.0001f;
}

float PlayerFastSqrtEstimateForAltGunTest(float value) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    bits = (bits >> 1) + 0x1fc00000;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

int g_playerAltGunFfStopCount = 0;
int g_playerAltGunFfStartCount = 0;
std::uint32_t g_playerAltGunFfLastIterations = 0;
std::uint32_t g_playerAltGunFfLastStartFlags = 0;

std::int32_t __stdcall PlayerAltGunTestEffectStart(zInput_DiEffect *, std::uint32_t iterations,
                                                   std::uint32_t flags) {
    ++g_playerAltGunFfStartCount;
    g_playerAltGunFfLastIterations = iterations;
    g_playerAltGunFfLastStartFlags = flags;
    return 0;
}

std::int32_t __stdcall PlayerAltGunTestEffectStop(zInput_DiEffect *) {
    ++g_playerAltGunFfStopCount;
    return 0;
}

struct PlayerAltGunTestEffect : IDirectInputEffect {
    HRESULT __stdcall QueryInterface(REFIID, LPVOID *) override { return E_NOINTERFACE; }
    ULONG __stdcall AddRef(void) override { return 1; }
    ULONG __stdcall Release(void) override { return 1; }
    HRESULT __stdcall Initialize(HINSTANCE, DWORD, REFGUID) override { return DI_OK; }
    HRESULT __stdcall GetEffectGuid(LPGUID) override { return DI_OK; }
    HRESULT __stdcall GetParameters(LPDIEFFECT, DWORD) override { return DI_OK; }
    HRESULT __stdcall SetParameters(LPCDIEFFECT, DWORD) override { return DI_OK; }
    HRESULT __stdcall Start(DWORD iterations, DWORD flags) override {
        ++g_playerAltGunFfStartCount;
        g_playerAltGunFfLastIterations = iterations;
        g_playerAltGunFfLastStartFlags = flags;
        return DI_OK;
    }
    HRESULT __stdcall Stop(void) override {
        ++g_playerAltGunFfStopCount;
        return DI_OK;
    }
    HRESULT __stdcall GetEffectStatus(LPDWORD) override { return DI_OK; }
    HRESULT __stdcall Download(void) override { return DI_OK; }
    HRESULT __stdcall Unload(void) override { return DI_OK; }
    HRESULT __stdcall Escape(LPDIEFFESCAPE) override { return DI_OK; }
};
} // namespace

extern "C" int player_find_alt_gun_controller_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    OptCatalogEntryDef entries[3] = {};
    entries[0].keyName = const_cast<char *>("weapon-a");
    entries[0].ordinalIndex = 101;
    entries[1].keyName = const_cast<char *>("weapon-b");
    entries[1].ordinalIndex = 202;
    entries[2].keyName = const_cast<char *>("other");
    entries[2].ordinalIndex = 303;

    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    g_OptCatalog_EntryCount = 3;
    g_OptCatalog_EntryTable = entries;

    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &entries[0];
    playerState.altWeaponBanks[5].controllerB.optCatalogEntry = &entries[1];
    playerState.altWeaponBanks[6].controllerA.optCatalogEntry = &entries[2];
    playerState.altWeaponBanks[6].controllerB.optCatalogEntry = &entries[2];

    const bool ctrlAFound = Player::FindAltGunFireControllerForWeaponId(&saveState, 101) ==
                            &playerState.altWeaponBanks[4].controllerA;
    const bool ctrlBFound = Player::FindAltGunFireControllerForWeaponId(&saveState, 202) ==
                            &playerState.altWeaponBanks[5].controllerB;
    const bool ctrlAPriority = Player::FindAltGunFireControllerForWeaponId(&saveState, 303) ==
                               &playerState.altWeaponBanks[6].controllerA;

    for (std::int32_t i = 2; i < 10; ++i) {
        playerState.altWeaponBanks[i].controllerA.optCatalogEntry = &entries[2];
        playerState.altWeaponBanks[i].controllerB.optCatalogEntry = &entries[2];
    }
    const bool fallback = Player::FindAltGunFireControllerForWeaponId(&saveState, 404) ==
                          &playerState.altWeaponBanks[1].controllerA;

    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;

    return ctrlAFound && ctrlBFound && ctrlAPriority && fallback ? 0 : 1;
}

extern "C" int player_alt_gun_fire_point_selection_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.steerBasisRaw = {1.0f, 2.0f, 3.0f};
    PlayerGunFireSlot sentinelSlot = {};
    PlayerGunFireSlot *outSlot = &sentinelSlot;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.aimBasisOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.gunFireDir, {1.0f, 2.0f, 3.0f}) || outSlot != &sentinelSlot) {
        return 1;
    }

    PlayerModalState modalState = {};
    saveState.primaryModalState = &modalState;
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    playerState.rootNode = &rootNode;

    const zMat4x3 rootMatrix = {2.0f, 3.0f, 5.0f, 7.0f, 11.0f, 13.0f,
                                17.0f, 19.0f, 23.0f, 29.0f, 31.0f, 37.0f};
    SetObjectLocalMatrix(&rootData, rootMatrix);
    modalState.modalNode = nullptr;
    Player::BuildGunFireTransform(&saveState);
    if (!MatrixEquals(playerState.gunFireTransform, rootMatrix)) {
        return 20;
    }

    zClass_Object3DDataPartial modalData = {};
    zClass_NodePartial modalNode = {};
    modalNode.classId = 5;
    modalNode.classData = &modalData;
    const zMat4x3 modalMatrix = {41.0f, 43.0f, 47.0f, 53.0f, 59.0f, 61.0f,
                                 67.0f, 71.0f, 73.0f, 79.0f, 83.0f, 89.0f};
    SetObjectLocalMatrix(&modalData, modalMatrix);
    modalState.modalNode = &modalNode;
    Player::BuildGunFireTransform(&saveState);
    const zMat4x3 expectedGunFireTransform = {
        modalMatrix.xx * rootMatrix.xx + modalMatrix.xy * rootMatrix.yx +
            modalMatrix.xz * rootMatrix.zx,
        modalMatrix.xx * rootMatrix.xy + modalMatrix.xy * rootMatrix.yy +
            modalMatrix.xz * rootMatrix.zy,
        modalMatrix.xx * rootMatrix.xz + modalMatrix.xy * rootMatrix.yz +
            modalMatrix.xz * rootMatrix.zz,
        modalMatrix.yx * rootMatrix.xx + modalMatrix.yy * rootMatrix.yx +
            modalMatrix.yz * rootMatrix.zx,
        modalMatrix.yx * rootMatrix.xy + modalMatrix.yy * rootMatrix.yy +
            modalMatrix.yz * rootMatrix.zy,
        modalMatrix.yx * rootMatrix.xz + modalMatrix.yy * rootMatrix.yz +
            modalMatrix.yz * rootMatrix.zz,
        modalMatrix.zy * rootMatrix.yx + modalMatrix.zz * rootMatrix.zx,
        modalMatrix.zy * rootMatrix.yy + modalMatrix.zz * rootMatrix.zy,
        modalMatrix.zy * rootMatrix.yz + modalMatrix.zz * rootMatrix.zz,
        modalMatrix.posY * rootMatrix.yx + modalMatrix.posZ * rootMatrix.zx +
            rootMatrix.posX,
        modalMatrix.posY * rootMatrix.yy + modalMatrix.posZ * rootMatrix.zy +
            rootMatrix.posY,
        modalMatrix.posY * rootMatrix.yz + modalMatrix.posZ * rootMatrix.zz +
            rootMatrix.posZ};
    if (!MatrixEquals(playerState.gunFireTransform, expectedGunFireTransform)) {
        return 21;
    }

    zClass_Object3DDataPartial gunData = {};
    zClass_NodePartial gunNode = {};
    gunNode.classId = 5;
    gunNode.classData = &gunData;
    zClass_Object3DDataPartial turretData = {};
    zClass_NodePartial turretNode = {};
    turretNode.classId = 5;
    turretNode.classData = &turretData;

    zMat4x3 gunMatrix = {0.0f, 0.0f, 0.0f, 0.0f, 2.0f, 3.0f, 0.0f, 5.0f, 7.0f, 0.0f, 0.0f, 0.0f};
    zMat4x3 turretMatrix = {11.0f, 0.0f, 13.0f, 0.0f, 0.0f, 0.0f,
                            17.0f, 0.0f, 19.0f, 0.0f, 0.0f, 0.0f};
    playerState.gunFireTransform = {23.0f, 29.0f, 31.0f, 37.0f, 41.0f, 43.0f,
                                    47.0f, 53.0f, 59.0f, 61.0f, 67.0f, 71.0f};
    playerState.aimBasisOrigin = {101.0f, 102.0f, 103.0f};
    playerState.gunNode = &gunNode;
    playerState.turretNode = &turretNode;
    SetObjectLocalMatrix(&gunData, gunMatrix);
    SetObjectLocalMatrix(&turretData, turretMatrix);

    const float localAimX = turretMatrix.zx * gunMatrix.posZ;
    const float localAimY = turretMatrix.posY + gunMatrix.posY;
    const float localAimZ = turretMatrix.zz * gunMatrix.posZ + turretMatrix.posZ;
    const zVec3 expectedAimBasis = {
        playerState.gunFireTransform.xx * localAimX +
            playerState.gunFireTransform.yx * localAimY +
            playerState.gunFireTransform.zx * localAimZ + playerState.gunFireTransform.posX,
        playerState.gunFireTransform.xy * localAimX +
            playerState.gunFireTransform.yy * localAimY +
            playerState.gunFireTransform.zy * localAimZ + playerState.gunFireTransform.posY,
        playerState.gunFireTransform.xz * localAimX +
            playerState.gunFireTransform.yz * localAimY +
            playerState.gunFireTransform.zz * localAimZ + playerState.gunFireTransform.posZ};
    zVec3 aimBasis = {};
    Player::UpdateAltGunAimBasisOrigin(&saveState, &aimBasis);
    if (!Vec3Equals(aimBasis, expectedAimBasis)) {
        return 22;
    }

    const zVec3 aimDirection = {3.0f, 0.5f, 4.0f};
    const float aimHorizontalLen = PlayerFastSqrtEstimateForAltGunTest(
        aimDirection.x * aimDirection.x + aimDirection.z * aimDirection.z);
    Player::UpdateGunAndTurretAimNodes(&aimDirection, &gunNode, &turretNode);
    const bool gunAimOk =
        gunData.localMatrix[0] == 1.0f && gunData.localMatrix[1] == 0.0f &&
        gunData.localMatrix[2] == 0.0f && gunData.localMatrix[3] == 0.0f &&
        gunData.localMatrix[4] == aimHorizontalLen && gunData.localMatrix[5] == 0.5f &&
        gunData.localMatrix[6] == 0.0f && gunData.localMatrix[7] == -0.5f &&
        gunData.localMatrix[8] == aimHorizontalLen;
    const float yawForward = -(aimDirection.z / aimHorizontalLen);
    const float yawSide = -(aimDirection.x / aimHorizontalLen);
    const bool turretAimOk =
        turretData.localMatrix[0] == yawForward && turretData.localMatrix[1] == 0.0f &&
        turretData.localMatrix[2] == -yawSide && turretData.localMatrix[3] == 0.0f &&
        turretData.localMatrix[4] == 1.0f && turretData.localMatrix[5] == 0.0f &&
        turretData.localMatrix[6] == yawSide && turretData.localMatrix[7] == 0.0f &&
        turretData.localMatrix[8] == yawForward;
    if (!gunAimOk || !turretAimOk) {
        return 23;
    }
    SetObjectLocalMatrix(&gunData, gunMatrix);
    SetObjectLocalMatrix(&turretData, turretMatrix);

    const float yawX = turretMatrix.zx * playerState.gunFireTransform.xx +
                       turretMatrix.zz * playerState.gunFireTransform.zx;
    const float yawY = turretMatrix.zx * playerState.gunFireTransform.xy +
                       turretMatrix.zz * playerState.gunFireTransform.zy;
    const float yawZ = turretMatrix.zx * playerState.gunFireTransform.xz +
                       turretMatrix.zz * playerState.gunFireTransform.zz;
    const zMat4x3 expectedComposed = {
        turretMatrix.xx * playerState.gunFireTransform.xx +
            turretMatrix.xz * playerState.gunFireTransform.zx,
        turretMatrix.xx * playerState.gunFireTransform.xy +
            turretMatrix.xz * playerState.gunFireTransform.zy,
        turretMatrix.xx * playerState.gunFireTransform.xz +
            turretMatrix.xz * playerState.gunFireTransform.zz,
        playerState.gunFireTransform.yx * gunMatrix.yy + gunMatrix.yz * yawX,
        playerState.gunFireTransform.yy * gunMatrix.yy + gunMatrix.yz * yawY,
        playerState.gunFireTransform.yz * gunMatrix.yy + gunMatrix.yz * yawZ,
        playerState.gunFireTransform.yx * gunMatrix.zy + gunMatrix.zz * yawX,
        playerState.gunFireTransform.yy * gunMatrix.zy + gunMatrix.zz * yawY,
        playerState.gunFireTransform.yz * gunMatrix.zy + gunMatrix.zz * yawZ,
        101.0f,
        102.0f,
        103.0f};
    zMat4x3 composed = {};
    Player::ComposeAimBasisWorldMatrix(&saveState, &composed);
    if (!MatrixEquals(composed, expectedComposed)) {
        return 2;
    }

    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {10.0f, 20.0f, 30.0f};

    PlayerGunFireController controller = {};
    zClass_NodePartial primaryAttachNode = {};
    zClass_NodePartial secondaryAttachNode = {};
    zClass_NodePartial activeAttachState = {};
    controller.attachNodePrimary = &primaryAttachNode;
    controller.attachNodeSecondary = &secondaryAttachNode;
    playerState.activeAltGunController = &controller;

    OptCatalogEntryDef aimEntry = {};
    aimEntry.range = 1000.0f;
    aimEntry.gravity = 0.0f;
    controller.optCatalogEntry = &aimEntry;

    PlayerMasterModalData masterModalData = {};
    masterModalData.gunPitchRate = 0.8f;
    masterModalData.gunPitchMin = -0.8f;
    modalState.masterModalData = &masterModalData;
    modalState.modalNode = nullptr;

    SetObjectLocalMatrix(&rootData, identityMatrix);
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {};
    playerState.storedTargetPos = {0.0f, 0.0f, 10.0f};
    playerState.altGunAimOrigin = {0.0f, 0.0f, 1.0f};
    playerState.gunFireDir = {9.0f, 8.0f, 7.0f};
    playerState.usePresetGunFireDir = 0;
    playerState.altGunTransitionState = 1;
    playerState.cameraTickEnabled = 0;

    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    g_FrameDeltaTimeSec = 0.0f;
    Player::UpdateAltGunAimDirection(&saveState);
    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;

    const float expectedAimDistanceApprox = PlayerFastSqrtEstimateForAltGunTest(100.0f);
    const bool updateAimOk =
        Vec3Equals(playerState.aimBasisOrigin, {0.0f, 0.0f, 0.0f}) &&
        Vec3Equals(playerState.altGunAimOrigin, {0.0f, 0.0f, 1.0f}) &&
        Vec3Equals(playerState.gunFireDir, {0.0f, 0.0f, 1.0f}) &&
        playerState.aimPitchResult == -1.0f &&
        playerState.aimTargetDistanceApprox == expectedAimDistanceApprox &&
        playerState.usePresetGunFireDir == 0 && gunData.localMatrix[0] == 1.0f &&
        gunData.localMatrix[4] == 1.0f && gunData.localMatrix[8] == 1.0f &&
        turretData.localMatrix[0] == -1.0f && turretData.localMatrix[8] == -1.0f;
    if (!updateAimOk) {
        return 24;
    }

    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {10.0f, 20.0f, 30.0f};

    playerState.firePointCenter = {1.0f, 2.0f, 3.0f};
    playerState.firePointRight = {4.0f, 5.0f, 6.0f};
    playerState.firePointLeft = {7.0f, 8.0f, 9.0f};

    outSlot = nullptr;
    playerState.altHardpointSelectState = 0;
    controller.attachState = nullptr;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {11.0f, 22.0f, 33.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.altHardpointSelectState != 0) {
        return 3;
    }

    outSlot = nullptr;
    controller.attachState = &activeAttachState;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {10.0f, 20.0f, 30.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.altHardpointSelectState != 0) {
        return 4;
    }

    outSlot = nullptr;
    controller.attachState = nullptr;
    playerState.altHardpointSelectState = 1;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {14.0f, 25.0f, 36.0f}) ||
        outSlot != &playerState.altFireSlotRight ||
        playerState.altFireSlotRight.attachNode != &secondaryAttachNode ||
        playerState.altHardpointSelectState != 2) {
        return 5;
    }

    outSlot = nullptr;
    playerState.altHardpointSelectState = 2;
    Player::SelectAltGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.altFireOrigin, {17.0f, 28.0f, 39.0f}) ||
        outSlot != &playerState.altFireSlotLeft ||
        playerState.altFireSlotLeft.attachNode != &primaryAttachNode ||
        playerState.altHardpointSelectState != 1) {
        return 6;
    }

    return 0;
}

extern "C" int player_primary_gun_fire_point_selection_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.steerBasisRaw = {1.0f, 2.0f, 3.0f};
    PlayerGunFireSlot sentinelSlot = {};
    PlayerGunFireSlot *outSlot = &sentinelSlot;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.aimBasisOrigin, {10.0f, 21.0f, 30.0f}) ||
        !Vec3Equals(playerState.gunFireDir, {1.0f, 2.0f, 3.0f}) || outSlot != &sentinelSlot) {
        return 1;
    }

    PlayerModalState modalState = {};
    saveState.primaryModalState = &modalState;
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    playerState.rootNode = &rootNode;

    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&rootData, identityMatrix);
    modalState.modalNode = nullptr;
    Player::BuildGunFireTransform(&saveState);

    zClass_Object3DDataPartial gunData = {};
    zClass_NodePartial gunNode = {};
    gunNode.classId = 5;
    gunNode.classData = &gunData;
    zClass_Object3DDataPartial turretData = {};
    zClass_NodePartial turretNode = {};
    turretNode.classId = 5;
    turretNode.classData = &turretData;
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    playerState.gunNode = &gunNode;
    playerState.turretNode = &turretNode;
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {10.0f, 20.0f, 30.0f};

    PlayerGunFireController controller = {};
    zClass_NodePartial primaryAttachNode = {};
    zClass_NodePartial secondaryAttachNode = {};
    zClass_NodePartial activeAttachState = {};
    controller.attachNodePrimary = &primaryAttachNode;
    controller.attachNodeSecondary = &secondaryAttachNode;
    playerState.activePrimaryGunController = &controller;
    playerState.firePointCenter = {1.0f, 2.0f, 3.0f};
    playerState.firePointRight = {4.0f, 5.0f, 6.0f};
    playerState.firePointLeft = {7.0f, 8.0f, 9.0f};

    outSlot = nullptr;
    playerState.primaryHardpointSelectState = 0;
    controller.attachState = nullptr;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {11.0f, 22.0f, 33.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.primaryHardpointSelectState != 0) {
        return 2;
    }

    outSlot = nullptr;
    controller.attachState = &activeAttachState;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {10.0f, 20.0f, 30.0f}) ||
        outSlot != &playerState.altFireSlotCenter ||
        playerState.altFireSlotCenter.attachNode != &primaryAttachNode ||
        playerState.primaryHardpointSelectState != 0) {
        return 3;
    }

    outSlot = nullptr;
    controller.attachState = nullptr;
    playerState.primaryHardpointSelectState = 1;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {14.0f, 25.0f, 36.0f}) ||
        outSlot != &playerState.altFireSlotRight ||
        playerState.altFireSlotRight.attachNode != &secondaryAttachNode ||
        playerState.primaryHardpointSelectState != 2) {
        return 4;
    }

    outSlot = nullptr;
    playerState.primaryHardpointSelectState = 2;
    Player::SelectPrimaryGunFirePointAndSlot(&saveState, &outSlot);
    if (!Vec3Equals(playerState.primaryFireOrigin, {17.0f, 28.0f, 39.0f}) ||
        outSlot != &playerState.altFireSlotLeft ||
        playerState.altFireSlotLeft.attachNode != &primaryAttachNode ||
        playerState.primaryHardpointSelectState != 1) {
        return 5;
    }

    return 0;
}

extern "C" int player_alt_gun_ensure_aux_effect_active_smoke(void) {
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    const int oldNetworkOptionState = g_OptCatalogNetworkOptionState;
    OptCatalogAllocRuntimeGateCallback const oldGateCallback =
        g_OptCatalog_AllocRuntimeGateCallback;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    const int oldForceFeedbackCaps = g_zInput_JoystickCaps_ForceFeedback;
    zInput_FFEffectSet *const oldEffectSet = g_zInputFfEffectSet;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    int result = 0;
    int joystickEnabled = 1;
    ZOPT_INPUT_JOYSTICK = &joystickEnabled;
    g_zInput_JoystickCaps_ForceFeedback = 1;
    g_OptCatalogNetworkOptionState = 0;
    g_OptCatalog_AllocRuntimeGateCallback = nullptr;
    g_OptCatalogNextSpawnScale = 1.0f;

    PlayerAltGunTestEffect primaryEffect = {};
    zInput_FFEffectSet effectSet = {};
    effectSet.PrimaryFire = &primaryEffect;
    g_zInputFfEffectSet = &effectSet;
    g_playerAltGunFfStopCount = 0;
    g_playerAltGunFfStartCount = 0;
    g_playerAltGunFfLastIterations = 0;
    g_playerAltGunFfLastStartFlags = 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController controller = {};
    OptCatalogEntryDef entry = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial runtimeWorld = {};
    zClass_NodeFreeListSlot projectileSlot = {};
    zClass_Object3DDataPartial projectileData = {};
    OptCatalogRuntimeInstanceStorage runtime = {};

    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.variantTag.count = 2;
    playerState.variantTag.tags[0] = 3;
    playerState.variantTag.tags[1] = 4;
    playerState.variantTag.tags[2] = 5;
    playerState.projectileSpawnVel = {2.0f, 3.0f, 4.0f};
    playerState.gunFireDir = {0.0f, 1.0f, 0.0f};
    playerState.usePresetGunFireDir = 1;
    controller.optCatalogEntry = &entry;
    entry.velocity = 5.0f;
    entry.fireFxSelectedSoundIndex = -1;
    entry.fireFxSelectedEffectIndex = -1;
    entry.flyoutSelectedEffectIndex = -1;
    runtimeWorld.classId = 3;
    projectileSlot.node.classId = 5;
    projectileSlot.node.classData = &projectileData;
    runtime.projectileNode = &projectileSlot.node;
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &runtime;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    zVec3 effectPos = {10.0f, 11.0f, 12.0f};
    const int presetResult =
        Player::EnsureGunAuxEffectActive(&saveState, &controller, &effectPos);

    if (presetResult != 1) {
        result = 1;
    } else if (entry.activeRuntimeListHead != &runtime) {
        result = 2;
    } else if (g_OptCatalogFreeRuntimeInstanceList != nullptr) {
        result = 3;
    } else if (runtime.ownerNode != &rootNode) {
        result = 4;
    } else if (!Vec3Equals(runtime.pos, effectPos) || !Vec3Equals(runtime.origin, effectPos)) {
        result = 5;
    } else if (!Vec3Equals(runtime.dir, {0.0f, 1.0f, 0.0f})) {
        result = 6;
    } else if (!Vec3Equals(runtime.velocity, {2.0f, 8.0f, 4.0f})) {
        result = 7;
    } else if (runtime.saveState != &saveState) {
        result = 8;
    } else if (runtimeWorld.listCountB != 1 || runtimeWorld.listB[0] != &projectileSlot.node) {
        result = 9;
    } else if (g_playerAltGunFfStopCount != 1 || g_playerAltGunFfStartCount != 1 ||
               g_playerAltGunFfLastIterations != 1 || g_playerAltGunFfLastStartFlags != 0) {
        result = 10;
    }

    if (result == 0) {
        zClass_Class::RemoveChild(&runtimeWorld, &projectileSlot.node);
        g_OptCatalogFreeRuntimeInstanceList = nullptr;
        entry.activeRuntimeListHead = nullptr;
        const int failResult =
            Player::EnsureGunAuxEffectActive(&saveState, &controller, &effectPos);
        if (failResult != 0 || entry.activeRuntimeListHead != nullptr) {
            result = 15;
        }
    }

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_OptCatalogNetworkOptionState = oldNetworkOptionState;
    g_OptCatalog_AllocRuntimeGateCallback = oldGateCallback;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    g_zInput_JoystickCaps_ForceFeedback = oldForceFeedbackCaps;
    g_zInputFfEffectSet = oldEffectSet;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return result;
}

extern "C" int player_update_continuous_alt_gun_fire_controller_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldPrimaryGunDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    int *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    PlayerProgressTargetSlotRuntime *const oldPendingSpawnTargetListPtr =
        g_OptCatalogPendingSpawnTargetListPtr;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerGunFireController controller = {};
    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState trailRuntime = {};
    zSndSample trailStopSample = {};
    zSndSample trailLoopSample = {};
    zVec3 spawnPos = {1.0f, 2.0f, 3.0f};
    zVec3 spawnDir = {0.0f, 0.0f, 1.0f};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.activeAltGunController = &controller;
    controller.trailRuntimeState = &trailRuntime;
    owner.trailStopSample = &trailStopSample;
    owner.trailLoopSample = &trailLoopSample;
    trailRuntime.ownerEntry = &owner;
    trailRuntime.spawnPos = &spawnPos;
    trailRuntime.spawnDir = &spawnDir;
    trailRuntime.trailDistance = 9.0f;
    trailRuntime.volumeFadeTimer = 8.0f;
    trailRuntime.alphaPulsePhase = 7.0f;

    modalData.masterType = 2;
    g_HudSensorTracker.primaryGunDispatchCount = 5;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    Player::UpdateContinuousAltGunFireController(&saveState);
    if (playerState.queuedFixedDamageFlag != 1 || playerState.altGunFireHeldFlag != 0 ||
        owner.activeTrailRuntime != nullptr || g_HudSensorTracker.primaryGunDispatchCount != 5) {
        g_GameStateOrMapTable = oldGameStateOrMapTable;
        g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
        g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
        return 1;
    }

    playerState.queuedFixedDamageFlag = 0;
    playerState.playerOrdinal = 7;
    modalData.masterType = 3;
    g_OptCatalogNextSpawnScale = 2.5f;
    Player::UpdateContinuousAltGunFireController(&saveState);
    const bool firstFireOk =
        playerState.queuedFixedDamageFlag == 0 && playerState.altGunFireHeldFlag == 1 &&
        owner.activeTrailRuntime == &trailRuntime && trailRuntime.trailDistance == 0.0f &&
        trailRuntime.volumeFadeTimer == 0.0f && trailRuntime.alphaPulsePhase == 0.0f &&
        trailRuntime.spawnScale == 2.5f && g_OptCatalogNextSpawnScale == 1.0f &&
        g_HudSensorTracker.primaryGunDispatchCount == 6;

    g_GameStateOrMapTable = nullptr;
    Player::UpdateContinuousAltGunFireController(&saveState);
    const bool heldRemoteOk =
        owner.activeTrailRuntime == &trailRuntime &&
        g_HudSensorTracker.primaryGunDispatchCount == 6;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    return firstFireOk && heldRemoteOk ? 0 : 2;
}

extern "C" int player_alt_gun_projectile_dispatch_helpers_smoke(void) {
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_SaveGameState tetherSave = {};
    zUtil_PlayerStateStorage tetherPlayer = {};
    PlayerGunFireController tetherController = {};
    OptCatalogEntryDef tetherEntry = {};
    OptCatalogRuntimeInstanceStorage tetherRuntime = {};
    zClass_NodeFreeListSlot tetherProjectile = {};
    zClass_Object3DDataPartial tetherProjectileData = {};
    zClass_NodePartial tetherRoot = {};
    zClass_NodePartial tetherRuntimeWorld = {};
    zClass_NodePartial tetherMount = {};
    tetherSave.playerState = &tetherPlayer;
    tetherPlayer.activeAltGunController = &tetherController;
    tetherPlayer.rootNode = &tetherRoot;
    tetherPlayer.altFireOrigin = {4.0f, 5.0f, 6.0f};
    tetherPlayer.gunFireDir = {0.0f, 0.0f, 1.0f};
    tetherPlayer.projectileSpawnVel = {1.0f, 2.0f, 3.0f};
    tetherController.optCatalogEntry = &tetherEntry;
    tetherController.attachNodePrimary = &tetherMount;
    tetherController.attachState = &tetherRuntime;
    tetherEntry.flags = 1u << 20;
    tetherEntry.fireFxSelectedSoundIndex = -1;
    tetherEntry.fireFxSelectedEffectIndex = -1;
    tetherEntry.flyoutSelectedEffectIndex = -1;
    tetherProjectile.node.classId = 5;
    tetherProjectile.node.classData = &tetherProjectileData;
    tetherRuntime.projectileNode = &tetherProjectile.node;
    tetherRuntimeWorld.classId = 3;
    zClass_Class::AddChild(&tetherMount, &tetherProjectile.node);
    g_OptCatalogRuntimeWorld = &tetherRuntimeWorld;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&tetherSave;
    const int tetherResult = Player::AltGunLaunchProjectile(&tetherSave);
    const bool tetherOk =
        tetherResult == 1 && tetherPlayer.pendingAltCameraToggle == 1 &&
        tetherPlayer.altGunTransitionState == 0x100 &&
        tetherPlayer.altGunTransitionController == &tetherController &&
        tetherController.attachState == &tetherRuntime &&
        tetherEntry.activeRuntimeListHead == &tetherRuntime &&
        Vec3Equals(tetherRuntime.pos, tetherPlayer.altFireOrigin) &&
        Vec3Equals(tetherRuntime.dir, tetherPlayer.gunFireDir) &&
        tetherRuntimeWorld.listCountB == 1 &&
        tetherRuntimeWorld.listB[0] == &tetherProjectile.node && tetherMount.listCountB == 0;

    zUtil_SaveGameState nonTetherSave = {};
    zUtil_PlayerStateStorage nonTetherPlayer = {};
    PlayerGunFireController nonTetherController = {};
    OptCatalogEntryDef nonTetherEntry = {};
    OptCatalogRuntimeInstanceStorage nonTetherRuntime = {};
    zClass_NodeFreeListSlot nonTetherProjectile = {};
    zClass_Object3DDataPartial nonTetherProjectileData = {};
    zClass_NodePartial nonTetherRoot = {};
    zClass_NodePartial nonTetherRuntimeWorld = {};
    zClass_NodePartial nonTetherMount = {};
    nonTetherSave.playerState = &nonTetherPlayer;
    nonTetherPlayer.activeAltGunController = &nonTetherController;
    nonTetherPlayer.rootNode = &nonTetherRoot;
    nonTetherPlayer.altFireOrigin = {7.0f, 8.0f, 9.0f};
    nonTetherPlayer.gunFireDir = {1.0f, 0.0f, 0.0f};
    nonTetherPlayer.projectileSpawnVel = {2.0f, 3.0f, 4.0f};
    nonTetherController.optCatalogEntry = &nonTetherEntry;
    nonTetherController.attachNodePrimary = &nonTetherMount;
    nonTetherController.attachState = &nonTetherRuntime;
    nonTetherController.ammoOrCharge = 2.0f;
    nonTetherEntry.fireFxSelectedSoundIndex = -1;
    nonTetherEntry.fireFxSelectedEffectIndex = -1;
    nonTetherEntry.flyoutSelectedEffectIndex = -1;
    nonTetherProjectile.node.classId = 5;
    nonTetherProjectile.node.classData = &nonTetherProjectileData;
    nonTetherRuntime.projectileNode = &nonTetherProjectile.node;
    nonTetherRuntimeWorld.classId = 3;
    zClass_Class::AddChild(&nonTetherMount, &nonTetherProjectile.node);
    g_OptCatalogRuntimeWorld = &nonTetherRuntimeWorld;
    g_GameStateOrMapTable = nullptr;
    const int nonTetherResult = Player::AltGunLaunchProjectile(&nonTetherSave);
    const bool nonTetherOk =
        nonTetherResult == 1 && nonTetherController.attachState == nullptr &&
        nonTetherPlayer.altGunTransitionState == 2 &&
        nonTetherPlayer.altGunTransitionController == &nonTetherController &&
        nonTetherEntry.activeRuntimeListHead == &nonTetherRuntime &&
        Vec3Equals(nonTetherRuntime.dir, nonTetherPlayer.gunFireDir);

    zUtil_SaveGameState simpleSave = {};
    zUtil_PlayerStateStorage simplePlayer = {};
    PlayerGunFireController simpleController = {};
    OptCatalogEntryDef simpleEntry = {};
    OptCatalogRuntimeInstanceStorage simpleRuntime = {};
    zClass_NodeFreeListSlot simpleProjectile = {};
    zClass_Object3DDataPartial simpleProjectileData = {};
    zClass_NodePartial simpleRoot = {};
    zClass_NodePartial simpleRuntimeWorld = {};
    simpleSave.playerState = &simplePlayer;
    simplePlayer.activeAltGunController = &simpleController;
    simplePlayer.rootNode = &simpleRoot;
    simplePlayer.altFireOrigin = {1.0f, 2.0f, 3.0f};
    simplePlayer.storedTargetPos = {1.0f, 2.0f, 7.0f};
    simplePlayer.projectileSpawnVel = {5.0f, 6.0f, 7.0f};
    simpleController.optCatalogEntry = &simpleEntry;
    simpleEntry.fireFxSelectedSoundIndex = -1;
    simpleEntry.fireFxSelectedEffectIndex = -1;
    simpleEntry.flyoutSelectedEffectIndex = -1;
    simpleProjectile.node.classId = 5;
    simpleProjectile.node.classData = &simpleProjectileData;
    simpleRuntime.projectileNode = &simpleProjectile.node;
    simpleRuntimeWorld.classId = 3;
    g_OptCatalogRuntimeWorld = &simpleRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &simpleRuntime;
    const int simpleResult = Player::AltGunFireSimpleProjectile(&simpleSave);
    const bool simpleZeroGravityOk =
        simpleResult == 1 && simpleEntry.activeRuntimeListHead == &simpleRuntime &&
        Vec3Equals(simpleRuntime.pos, simplePlayer.altFireOrigin) &&
        Vec3Equals(simpleRuntime.dir, {0.0f, 0.0f, 1.0f});

    zUtil_SaveGameState ballisticSave = {};
    zUtil_PlayerStateStorage ballisticPlayer = {};
    PlayerGunFireController ballisticController = {};
    OptCatalogEntryDef ballisticEntry = {};
    OptCatalogRuntimeInstanceStorage ballisticRuntime = {};
    zClass_NodeFreeListSlot ballisticProjectile = {};
    zClass_Object3DDataPartial ballisticProjectileData = {};
    zClass_NodePartial ballisticRoot = {};
    zClass_NodePartial ballisticRuntimeWorld = {};
    ballisticSave.playerState = &ballisticPlayer;
    ballisticPlayer.activeAltGunController = &ballisticController;
    ballisticPlayer.rootNode = &ballisticRoot;
    ballisticPlayer.altFireOrigin = {3.0f, 4.0f, 5.0f};
    ballisticPlayer.gunFireDir = {0.25f, 0.5f, 0.75f};
    ballisticPlayer.projectileSpawnVel = {1.0f, 1.0f, 1.0f};
    ballisticController.optCatalogEntry = &ballisticEntry;
    ballisticEntry.gravity = 9.0f;
    ballisticEntry.fireFxSelectedSoundIndex = -1;
    ballisticEntry.fireFxSelectedEffectIndex = -1;
    ballisticEntry.flyoutSelectedEffectIndex = -1;
    ballisticProjectile.node.classId = 5;
    ballisticProjectile.node.classData = &ballisticProjectileData;
    ballisticRuntime.projectileNode = &ballisticProjectile.node;
    ballisticRuntimeWorld.classId = 3;
    g_OptCatalogRuntimeWorld = &ballisticRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &ballisticRuntime;
    const int ballisticResult = Player::AltGunFireSimpleProjectile(&ballisticSave);
    const bool simpleBallisticOk =
        ballisticResult == 1 && ballisticEntry.activeRuntimeListHead == &ballisticRuntime &&
        Vec3Equals(ballisticRuntime.dir, ballisticPlayer.gunFireDir);

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return tetherOk && nonTetherOk && simpleZeroGravityOk && simpleBallisticOk ? 0 : 1;
}

extern "C" int player_process_alt_gun_fire_dispatch_request_smoke(void) {
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldJoystickOption = ZOPT_INPUT_JOYSTICK;
    const int oldForceFeedbackCaps = g_zInput_JoystickCaps_ForceFeedback;
    const int oldPrimaryGunDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;

    int joystickEnabled = 0;
    ZOPT_INPUT_JOYSTICK = &joystickEnabled;
    g_zInput_JoystickCaps_ForceFeedback = 0;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController controller = {};
    OptCatalogEntryDef entry = {};
    OptCatalogRuntimeInstanceStorage runtime = {};
    zClass_NodeFreeListSlot projectile = {};
    zClass_Object3DDataPartial projectileData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial runtimeWorld = {};
    zClass_NodePartial mountNode = {};
    zClass_NodePartial gunNode = {};
    zClass_NodePartial turretNode = {};
    zClass_Object3DDataPartial gunData = {};
    zClass_Object3DDataPartial turretData = {};

    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&gunData, identityMatrix);
    SetObjectLocalMatrix(&turretData, identityMatrix);
    gunNode.classId = 5;
    gunNode.classData = &gunData;
    turretNode.classId = 5;
    turretNode.classData = &turretData;

    saveState.playerState = &playerState;
    playerState.activeAltGunController = &controller;
    playerState.rootNode = &rootNode;
    playerState.gunNode = &gunNode;
    playerState.turretNode = &turretNode;
    playerState.gunFireTransform = identityMatrix;
    playerState.aimBasisOrigin = {0.0f, 0.0f, 0.0f};
    playerState.firePointCenter = {1.0f, 0.0f, 0.0f};
    playerState.gunFireDir = {0.0f, 0.0f, 1.0f};
    playerState.projectileSpawnVel = {2.0f, 3.0f, 4.0f};
    playerState.altGunDispatchRequested = 1;
    controller.optCatalogEntry = &entry;
    controller.attachNodePrimary = &mountNode;
    controller.flags = 1;
    controller.ammoOrCharge = 2.0f;
    entry.gravity = 1.0f;
    entry.fireFxSelectedSoundIndex = -1;
    entry.fireFxSelectedEffectIndex = -1;
    entry.flyoutSelectedEffectIndex = -1;
    projectile.node.classId = 5;
    projectile.node.classData = &projectileData;
    runtime.projectileNode = &projectile.node;
    runtimeWorld.classId = 3;
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &runtime;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;
    g_HudSensorTracker.primaryGunDispatchCount = 10;

    Player::ProcessAltGunDispatchRequest(&saveState);
    const bool fireOk =
        playerState.altGunDispatchRequested == 0 && entry.activeRuntimeListHead == &runtime &&
        Vec3Equals(runtime.dir, playerState.gunFireDir) &&
        playerState.altFireSlotCenter.offset == 1.5f && controller.ammoOrCharge == 1.0f &&
        g_HudSensorTracker.primaryGunDispatchCount == 11;

    playerState.altGunFireHeldFlag = 1;
    playerState.altGunDispatchRequested = 9;
    g_HudSensorTracker.primaryGunDispatchCount = 20;
    Player::ProcessAltGunDispatchRequest(&saveState);
    const bool heldOk =
        playerState.altGunDispatchRequested == 9 &&
        g_HudSensorTracker.primaryGunDispatchCount == 21;

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_INPUT_JOYSTICK = oldJoystickOption;
    g_zInput_JoystickCaps_ForceFeedback = oldForceFeedbackCaps;
    g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
    return fireOk && heldOk ? 0 : 1;
}

extern "C" int player_solve_alt_gun_lead_target_point_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerGunFireController activeAltGunController = {};
    OptCatalogEntryDef optCatalogEntry = {};
    zVec3 outTargetPos = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    playerState.activeAltGunController = &activeAltGunController;
    activeAltGunController.optCatalogEntry = &optCatalogEntry;

    optCatalogEntry.velocity = 10.0f;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.projectileSpawnVel = {0.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {3.0f, 4.0f, 5.0f};
    targetPlayerState.fxOffsetWorld = {100.0f, 101.0f, 102.0f};
    targetPlayerState.projectileSpawnVel = {20.0f, 0.0f, 0.0f};
    Player::SolveAltGunLeadTargetPoint(&saveState, &targetState, &outTargetPos);
    const bool fallbackOk = Vec3Equals(outTargetPos, targetPlayerState.worldPos);

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.projectileSpawnVel = {0.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {30.0f, 0.0f, 0.0f};
    targetPlayerState.fxOffsetWorld = {31.0f, 2.0f, 3.0f};
    targetPlayerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};

    std::srand(12345);
    const int expectedRand = std::rand();
    std::srand(12345);
    Player::SolveAltGunLeadTargetPoint(&saveState, &targetState, &outTargetPos);

    const float leadScale = (PlayerFastSqrtEstimateForAltGunTest(9.0f) + 0.3f) / 0.99f;
    const float expectedY =
        2.0f - (static_cast<float>(expectedRand) * 3.05185094e-05f - 0.5f) * -2.0f;
    const bool leadOk =
        FloatNear(outTargetPos.x, 31.0f + leadScale) &&
        FloatNear(outTargetPos.y, expectedY) &&
        FloatNear(outTargetPos.z, 3.0f);

    if (!fallbackOk) {
        return 1;
    }
    return leadOk ? 0 : 2;
}

extern "C" int player_tick_ai_mode2_alt_gun_attack_window_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const int oldBreakOnFirst = g_cls_di_BreakOnFirstCandidate;
    const int oldStopAfterFirst = g_cls_di_StopAfterFirstHit;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerGunFireController activeAltGunController = {};
    OptCatalogEntryDef optCatalogEntry = {};
    zClass_NodePartial aiRootNode = {};
    zClass_NodePartial targetRootNode = {};
    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};

    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    playerState.activeAltGunController = &activeAltGunController;
    playerState.rootNode = &aiRootNode;
    playerState.statusMeterScaled = 0.25f;
    playerState.fxOffsetWorld = {1.0f, 2.0f, 3.0f};
    playerState.projectileSpawnVel = {0.0f, 0.0f, 0.0f};
    targetPlayerState.rootNode = &targetRootNode;
    targetPlayerState.lifecycleState = 1;
    targetPlayerState.worldPos = {10.0f, 11.0f, 12.0f};
    targetPlayerState.fxOffsetWorld = {20.0f, 21.0f, 22.0f};
    targetPlayerState.projectileSpawnVel = {20.0f, 0.0f, 0.0f};
    activeAltGunController.optCatalogEntry = &optCatalogEntry;
    activeAltGunController.dispatchRepeatDelay = 4.0f;
    activeAltGunController.aiAttackRangeMin = 5.0f;
    activeAltGunController.aiAttackRangeMax = 50.0f;
    optCatalogEntry.velocity = 10.0f;
    worldNode.classData = &worldData;
    aiRootNode.flags = 0x10;
    targetRootNode.flags = 0x10;

    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));
    g_Player_RuntimeDiScene = &worldNode;
    g_cls_di_BreakOnFirstCandidate = 99;
    g_cls_di_StopAfterFirstHit = 99;

    g_Player_TotalTimeSecScaled = 10.0f;
    playerState.aiStateEndTime = 5.0f;
    playerState.aiNotPursuitDwell = 2.0f;
    playerState.aiMode2AttackDwell = 3.0f;
    activeAltGunController.nextDispatchTime = 20.0f;
    Player::TickAiMode2AltGunAttackWindow(&saveState, 10.0f, 1.0f);
    const bool refreshOk =
        FloatNear(playerState.aiStateStartTime, 12.0f) &&
        FloatNear(playerState.aiStateEndTime, 15.0f) &&
        playerState.altGunDispatchRequested == 0;

    g_Player_TotalTimeSecScaled = 20.0f;
    playerState.aiStateStartTime = 19.0f;
    playerState.aiStateEndTime = 30.0f;
    playerState.altGunDispatchRequested = 0;
    playerState.altGunFireHeldFlag = 0;
    playerState.progressTargetCount = 7;
    playerState.progressTargetSlots[0].targetPos = &playerState.worldPos;
    playerState.progressTargetSlots[0].targetVelocity = &playerState.projectileSpawnVel;
    activeAltGunController.nextDispatchTime = 1.0f;
    optCatalogEntry.flags = 0;
    Player::TickAiMode2AltGunAttackWindow(&saveState, 10.0f, 0.8f);
    const bool dispatchOk =
        playerState.altGunDispatchRequested == 1 &&
        FloatNear(activeAltGunController.nextDispatchTime, 28.0f) &&
        playerState.progressTargetCount == 0 &&
        playerState.progressTargetSlots[0].targetPos == nullptr &&
        playerState.progressTargetSlots[0].targetVelocity == nullptr &&
        Vec3Equals(playerState.storedTargetPos, targetPlayerState.worldPos);

    playerState.altGunFireHeldFlag = 1;
    playerState.altGunDispatchRequested = 5;
    activeAltGunController.nextDispatchTime = 30.0f;
    Player::TickAiMode2AltGunAttackWindow(&saveState, 50.0f, 0.75f);
    const bool heldCopyOk =
        playerState.altGunDispatchRequested == 5 &&
        Vec3Equals(playerState.storedTargetPos, targetPlayerState.fxOffsetWorld);

    targetPlayerState.lifecycleState = 4;
    Player::TickAiMode2AltGunAttackWindow(&saveState, 10.0f, 1.0f);
    const bool heldClearOk =
        playerState.altGunDispatchRequested == 0 &&
        FloatNear(activeAltGunController.nextDispatchTime, 24.0f);

    g_cls_di_StopAfterFirstHit = oldStopAfterFirst;
    g_cls_di_BreakOnFirstCandidate = oldBreakOnFirst;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!refreshOk) {
        return 1;
    }
    if (!dispatchOk) {
        return 2;
    }
    if (!heldCopyOk) {
        return 3;
    }
    return heldClearOk ? 0 : 4;
}

extern "C" int player_alt_gun_fire_slot_offset_smoke(void) {
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    g_FrameDeltaTimeSec = 0.0f;

    zClass_Object3DDataPartial directNodeData = {};
    zClass_NodePartial directNode = {};
    directNode.classId = 5;
    directNode.classData = &directNodeData;
    const zMat4x3 identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                    0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    SetObjectLocalMatrix(&directNodeData, identityMatrix);

    PlayerGunFireSlot slot = {};
    slot.offset = 2.0f;
    slot.attachNode = &directNode;
    Player::DecayAndApplyAltFireSlotOffsetToNode(&slot, &directNode, 0.25f, 1);
    if (!FloatNear(slot.offset, 2.0f) || !FloatNear(directNodeData.localMatrix[10], -0.5f) ||
        !FloatNear(directNodeData.localMatrix[11], 2.0f)) {
        g_FrameDeltaTimeSec = oldFrameDelta;
        return 1;
    }

    slot.offset = 0.005f;
    directNodeData.localMatrix[10] = 7.0f;
    directNodeData.localMatrix[11] = 8.0f;
    Player::DecayAndApplyAltFireSlotOffsetToNode(&slot, &directNode, 3.0f, 0);
    if (slot.offset != 0.0f || directNodeData.localMatrix[10] != 0.0f ||
        directNodeData.localMatrix[11] != 0.0f) {
        g_FrameDeltaTimeSec = oldFrameDelta;
        return 2;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    zClass_NodePartial gunNode = {};
    playerState.gunNode = &gunNode;
    playerState.gunFireDir.y = 4.0f;

    zClass_Object3DDataPartial leftData = {};
    zClass_Object3DDataPartial rightData = {};
    zClass_Object3DDataPartial centerData = {};
    zClass_NodePartial leftNode = {};
    zClass_NodePartial rightNode = {};
    zClass_NodePartial centerNode = {};
    leftNode.classId = 5;
    rightNode.classId = 5;
    centerNode.classId = 5;
    leftNode.classData = &leftData;
    rightNode.classData = &rightData;
    centerNode.classData = &centerData;
    SetObjectLocalMatrix(&leftData, identityMatrix);
    SetObjectLocalMatrix(&rightData, identityMatrix);
    SetObjectLocalMatrix(&centerData, identityMatrix);
    playerState.altFireSlotLeft.offset = 1.0f;
    playerState.altFireSlotRight.offset = -2.0f;
    playerState.altFireSlotCenter.offset = 3.0f;
    playerState.altFireSlotLeft.attachNode = &leftNode;
    playerState.altFireSlotRight.attachNode = &rightNode;
    playerState.altFireSlotCenter.attachNode = &centerNode;

    Player::ApplyGunFireSlotOffsetToNode(&saveState);
    if (playerState.altFireSlotLeft.offset != 0.0f ||
        playerState.altFireSlotRight.offset != 0.0f ||
        playerState.altFireSlotCenter.offset != 0.0f || leftData.localMatrix[10] != 0.0f ||
        leftData.localMatrix[11] != 0.0f || rightData.localMatrix[10] != 0.0f ||
        rightData.localMatrix[11] != 0.0f || centerData.localMatrix[10] != 0.0f ||
        centerData.localMatrix[11] != 0.0f) {
        g_FrameDeltaTimeSec = oldFrameDelta;
        return 3;
    }

    playerState.gunNode = nullptr;
    playerState.altFireSlotLeft.offset = 5.0f;
    leftData.localMatrix[10] = 6.0f;
    leftData.localMatrix[11] = 7.0f;
    Player::ApplyGunFireSlotOffsetToNode(&saveState);
    const bool gunlessUnchanged = playerState.altFireSlotLeft.offset == 5.0f &&
                                  leftData.localMatrix[10] == 6.0f &&
                                  leftData.localMatrix[11] == 7.0f;

    g_FrameDeltaTimeSec = oldFrameDelta;
    return gunlessUnchanged ? 0 : 4;
}

extern "C" int player_reset_alt_gun_runtime_state_smoke(void) {
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    PlayerGunFireController *const activeController =
        &playerState.altWeaponBanks[2].controllerA;
    playerState.activeAltGunController = activeController;
    playerState.altGunFireHeldFlag = 1;
    playerState.altGunTransitionState = 7;
    playerState.altGunTransitionTimerA = 3.0f;
    playerState.altGunTransitionTimerB = 4.0f;
    playerState.altGunTransitionController = activeController;

    zClass_NodePartial mountNode = {};
    zClass_Object3DDataPartial mountData = {};
    mountNode.classId = 5;
    mountNode.classData = &mountData;
    mountNode.flags = 0x04;
    mountData.scale = {0.25f, 0.5f, 0.75f};
    activeController->attachNodePrimary = &mountNode;
    activeController->attachPosX = 7.0f;
    activeController->attachPosY = 8.0f;
    activeController->attachPosZ = 9.0f;

    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState trailRuntime = {};
    zClass_NodePartial trailNode = {};
    owner.activeTrailRuntime = &trailRuntime;
    trailRuntime.ownerEntry = &owner;
    trailRuntime.activeNodeSlotCount = 1;
    trailRuntime.activeNodeSlotCursor = 1;
    trailRuntime.activeNodeSlots[0].node = &trailNode;
    trailNode.classId = 5;
    trailNode.flags = 0x04;
    activeController->trailRuntimeState = &trailRuntime;
    activeController->optCatalogEntry = &owner;

    OptCatalogRuntimeInstanceStorage runtime = {};
    zClass_NodeFreeListSlot projectileSlot = {};
    zClass_Object3DDataPartial projectileData = {};
    projectileSlot.node.classId = 5;
    projectileSlot.node.classData = &projectileData;
    projectileSlot.damageHandler = &owner;
    runtime.projectileNode = &projectileSlot.node;
    runtime.lifetime = 0.0f;
    activeController->attachState = &runtime;
    zClass_Class::AddChild(&mountNode, &projectileSlot.node);

    zClass_NodePartial doorLeft = {};
    zClass_Object3DDataPartial doorLeftData = {};
    doorLeft.classId = 5;
    doorLeft.classData = &doorLeftData;
    doorLeftData.scale = {0.3f, 0.4f, 0.5f};
    playerState.doorLeftNode = &doorLeft;

    zClass_NodePartial bank9Node = {};
    zClass_Object3DDataPartial bank9Data = {};
    bank9Node.classId = 5;
    bank9Node.classData = &bank9Data;
    bank9Node.flags = 0x04;
    bank9Data.scale = {2.0f, 2.0f, 2.0f};
    PlayerGunFireController &bank9Controller = playerState.altWeaponBanks[9].controllerB;
    bank9Controller.attachNodePrimary = &bank9Node;
    bank9Controller.attachPosX = 1.0f;
    bank9Controller.attachPosY = 2.0f;
    bank9Controller.attachPosZ = 3.0f;

    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;

    Player::ResetAltGunRuntimeState(&saveState);

    const bool cleanupOk =
        playerState.altGunFireHeldFlag == 0 && owner.activeTrailRuntime == nullptr &&
        trailRuntime.activeNodeSlotCursor == 0 && (trailNode.flags & 0x04) == 0 &&
        activeController->attachState == nullptr &&
        g_OptCatalogFreeRuntimeInstanceList == &runtime && runtime.next == &freeSentinel &&
        projectileSlot.damageHandler == nullptr && mountNode.listCountB == 0 &&
        projectileSlot.node.listCountA == 0;
    const bool stateOk =
        playerState.altGunTransitionState == 1 &&
        playerState.altGunTransitionController == nullptr &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.altGunTransitionTimerB == 0.0f;
    const bool attachResetOk =
        (mountNode.flags & 0x04) == 0 && mountData.scale.x == 1.0f &&
        mountData.scale.y == 1.0f && mountData.scale.z == 1.0f &&
        mountData.localMatrix[9] == 7.0f && mountData.localMatrix[10] == 8.0f &&
        mountData.localMatrix[11] == 9.0f && (bank9Node.flags & 0x04) == 0 &&
        bank9Data.scale.x == 1.0f && bank9Data.localMatrix[9] == 1.0f &&
        bank9Data.localMatrix[10] == 2.0f && bank9Data.localMatrix[11] == 3.0f &&
        doorLeftData.scale.x == 1.0f && doorLeftData.scale.y == 1.0f &&
        doorLeftData.scale.z == 1.0f;

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    return cleanupOk && stateOk && attachResetOk ? 0 : 1;
}
