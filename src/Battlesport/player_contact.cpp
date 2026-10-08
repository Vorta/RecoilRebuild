// Battlesport compilation unit between player.cpp and player_input.cpp, inferred
// from the retail object boundary [0x423460, 0x425920): its .rdata float constants
// [0x4d0708, 0x4d0758) repeat 0.0f, 1.0f, 0.01f, 5.0f and 10.0f that player.cpp
// pools at [0x4d06b0, 0x4d0708), and its .data static 0x4dc96c follows
// player.cpp's strings. Original filename unresolved; player_contact.cpp is a
// provisional name (2026-10-02).

#include "recoil/Mfc42Abi.h"
#include "player.h"

#include "Battlesport/ai_net.h"
#include "Battlesport/game_net.h"
#include "Battlesport/pickup.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_api.h"
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
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "hud.h"
#include "hud_sensor_tracker.h"
#include "opt_catalog.h"

#include <algorithm>
#include <atlbase.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern char g_HudUiCounterText_PlayerLabel[];

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-collisioncontactresolvescale
 * @recoil-artifact defines .data recoil:data:0x4dc96c: g_Player_CollisionContactResolveScale.
 * Purpose: stores the plan-tracked g_Player_CollisionContactResolveScale gameplay data symbol.
 */
float g_Player_CollisionContactResolveScale = 0.2f;
} // extern "C"
namespace {
enum PlayerMasterTypeId {
    kPlayerMasterTypeFly = 1,
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3,
    kPlayerMasterTypeHover = 4,
    kPlayerMasterTypeAmphib = 5
};
const float kPlayerWorldCollisionStackDrop = 0.200000003f;
// Unused since 0x4248e0 reads the pooled -1.0f literal (retail 0x4d0728); kept so the
// TU's C1 ID counter layout, which 0x425060's codegen depends on, stays unchanged.
const float kPlayerWorldCollisionSubRestoreYOffset = -1.0f;
const float kPlayerTransferDamageScale = 5.0f;
const float kPlayerTransferVelocityDamping = 0.666700006f;

struct PlayerCollisionContactContextPartial {
    unsigned char unknown_00[0x04];
    zUtil_SaveGameState* saveState;
};
RECOIL_STATIC_ASSERT(offsetof(PlayerCollisionContactContextPartial, saveState) == 0x04);
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

RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, checkpointVisitedFlags) == 0x1018);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, lapTimeDelta) == 0x109c);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, lapTimeSec) == 0x10a0);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, lapTimestampSec) == 0x10a4);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, checkpointTimestampSec) == 0x10a8);
RECOIL_STATIC_ASSERT(offsetof(PlayerCheckpointLapProgressView, lapCompletionCount) == 0x10ac);
} // namespace
namespace Player {
/**
 * Original inline helper; no standalone retail function exists.
 * Observed in address-backed callers 0x424010 PlayerPendingContact::SelectPreferred, 0x4251f0
 * Player::CollectPendingCollisionContactsForQuadProbe, 0x426770 Player::UpdateMasterTypeTrack, 0x428d60
 * Player::ProbeModalSampleHeights. Purpose: transform a point without emitting an out-of-line helper under the retail
 * translation unit's /Ob0 compile mode.
 */
#define PLAYER_TRANSFORM_POINT_BY_MATRIX(result, point, matrix)                                                        \
    do {                                                                                                               \
        (result).x = (point).x * (matrix).xx + (point).y * (matrix).yx + (point).z * (matrix).zx + (matrix).posX;      \
        (result).y = (point).x * (matrix).xy + (point).y * (matrix).yy + (point).z * (matrix).zy + (matrix).posY;      \
        (result).z = (point).x * (matrix).xz + (point).y * (matrix).yz + (point).z * (matrix).zz + (matrix).posZ;      \
    } while (0)
enum { kPlayerMasterTypeSub = 2, kPlayerMaxModalProbePoints = 4, kPlayerEnvProbeBasePointOffset = 15 };

struct PlayerContactSurfacePayload {
    unsigned char unknown_00[0x20];
    int impactSlot;
};

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x423c20 Player::ClassifyPendingContactsForSegment.
 * Purpose: provide the recovered append pending contact helper for
 * the Player/Pickup gameplay source cluster.
 */
#define PLAYER_APPEND_PENDING_CONTACT(queue, contactOut)                                                               \
    do {                                                                                                               \
        PlayerPendingContactQueue* const playerPendingQueue = (queue);                                                 \
        PlayerPendingContact* const playerPendingContact = new PlayerPendingContact;                                   \
        memset(playerPendingContact, 0, sizeof(*playerPendingContact));                                                \
        if (playerPendingContact != 0) {                                                                               \
            playerPendingContact->next = 0;                                                                            \
            if (playerPendingQueue->count == 0) {                                                                      \
                playerPendingQueue->head = playerPendingContact;                                                       \
            } else {                                                                                                   \
                playerPendingQueue->tail->next = playerPendingContact;                                                 \
            }                                                                                                          \
            playerPendingQueue->tail = playerPendingContact;                                                           \
            playerPendingContact->next = 0;                                                                            \
            ++playerPendingQueue->count;                                                                               \
        }                                                                                                              \
        (contactOut) = playerPendingContact;                                                                           \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x423c20 Player::ClassifyPendingContactsForSegment.
 * Purpose: provide the recovered copy pending contact payload helper for
 * the Player/Pickup gameplay source cluster.
 */
#define CopyPendingContactPayload(contact, candidate, segmentStart, segmentEnd, tagValue)                              \
    do {                                                                                                               \
        (contact)->hit = *(candidate);                                                                                 \
        (contact)->sweepStart = *(segmentStart);                                                                       \
        (contact)->sweepEnd = *(segmentEnd);                                                                           \
        (contact)->segmentTag = (tagValue);                                                                            \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x423c20 Player::ClassifyPendingContactsForSegment.
 * Purpose: provide the recovered get node damage handler helper for
 * the Player/Pickup gameplay source cluster.
 */
#define GetNodeDamageHandler(node) ((OptCatalogDamageHandlerPartial*)(((CZNodeFreeListSlot*)(node))->damageHandler))

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x423530 Player::ClearPendingContactQueues.
 * Purpose: provide the recovered free pending contact queue helper for
 * the Player/Pickup gameplay source cluster.
 */
#define FreePendingContactQueue(queue)                                                                                 \
    do {                                                                                                               \
        PlayerPendingContactQueue* const playerFreedQueue = (queue);                                                   \
        PlayerPendingContact* playerFreedContact = playerFreedQueue->head;                                             \
        if (playerFreedContact != 0) {                                                                                 \
            do {                                                                                                       \
                PlayerPendingContact* const playerFreedNext = playerFreedContact != 0 ? playerFreedContact->next : 0;  \
                delete playerFreedContact;                                                                             \
                playerFreedContact = playerFreedNext;                                                                  \
            } while (playerFreedContact != 0);                                                                         \
        }                                                                                                              \
        playerFreedQueue->listAux = 0;                                                                                 \
        playerFreedQueue->tail = 0;                                                                                    \
        playerFreedQueue->head = 0;                                                                                    \
        playerFreedQueue->count = 0;                                                                                   \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x424010 PlayerPendingContact::SelectPreferred, 0x424d00
 * Player::ProcessTransferContactQueue. Purpose: provide the recovered append existing pending contact helper for the
 * Player/Pickup gameplay source cluster.
 */
#define PLAYER_APPEND_EXISTING_PENDING_CONTACT(queue, contact)                                                         \
    do {                                                                                                               \
        PlayerPendingContactQueue* const playerAppendQueue = (queue);                                                  \
        PlayerPendingContact* const playerAppendContact = (contact);                                                   \
        playerAppendContact->next = 0;                                                                                 \
        if (playerAppendQueue->count == 0) {                                                                           \
            playerAppendQueue->head = playerAppendContact;                                                             \
        } else {                                                                                                       \
            playerAppendQueue->tail->next = playerAppendContact;                                                       \
        }                                                                                                              \
        playerAppendQueue->tail = playerAppendContact;                                                                 \
        playerAppendContact->next = 0;                                                                                 \
        ++playerAppendQueue->count;                                                                                    \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424d00 Player::ProcessTransferContactQueue.
 * Purpose: provide the recovered remove existing pending contact helper for
 * the Player/Pickup gameplay source cluster.
 */
#define PLAYER_REMOVE_EXISTING_PENDING_CONTACT(queue, contact)                                                         \
    do {                                                                                                               \
        PlayerPendingContactQueue* const playerRemoveQueue = (queue);                                                  \
        PlayerPendingContact* const playerRemoveContact = (contact);                                                   \
        if (playerRemoveContact != 0 && playerRemoveQueue->count != 0) {                                               \
            if (playerRemoveContact == playerRemoveQueue->head) {                                                      \
                --playerRemoveQueue->count;                                                                            \
                playerRemoveQueue->head = playerRemoveContact->next;                                                   \
                if (playerRemoveQueue->head == 0) {                                                                    \
                    playerRemoveQueue->listAux = 0;                                                                    \
                    playerRemoveQueue->tail = 0;                                                                       \
                }                                                                                                      \
            } else {                                                                                                   \
                PlayerPendingContact* playerPreviousContact = playerRemoveQueue->head;                                 \
                while (playerPreviousContact != 0) {                                                                   \
                    if (playerPreviousContact->next == playerRemoveContact) {                                          \
                        --playerRemoveQueue->count;                                                                    \
                        playerPreviousContact->next = playerRemoveContact->next;                                       \
                        if (playerRemoveQueue->tail == playerRemoveContact) {                                          \
                            playerRemoveQueue->tail = playerPreviousContact;                                           \
                        }                                                                                              \
                        break;                                                                                         \
                    }                                                                                                  \
                    playerPreviousContact = playerPreviousContact->next;                                               \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x4251f0 Player::CollectPendingCollisionContactsForQuadProbe, 0x424ed0
 * Player::TryResolvePendingCollisionProbeSweep. Purpose: provide the recovered move transfer contacts to preferred
 * collision helper for the Player/Pickup gameplay source cluster.
 */
#define PLAYER_MOVE_TRANSFER_CONTACTS_TO_PREFERRED_COLLISION(playerState)                                              \
    do {                                                                                                               \
        PlayerPendingContact* playerTransferContact = (playerState)->transferQueue.head;                               \
        while (playerTransferContact != 0) {                                                                           \
            PlayerPendingContact* const playerNextTransferContact = playerTransferContact->next;                       \
            PLAYER_REMOVE_EXISTING_PENDING_CONTACT(&(playerState)->transferQueue, playerTransferContact);              \
            if (playerTransferContact != 0) {                                                                          \
                PLAYER_APPEND_EXISTING_PENDING_CONTACT(                                                                \
                    &(playerState)->preferredCollisionQueue,                                                           \
                    playerTransferContact                                                                              \
                );                                                                                                     \
            }                                                                                                          \
            playerTransferContact = playerNextTransferContact;                                                         \
        }                                                                                                              \
    } while (0)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x4236b0 Player::BuildPendingContactQueues.
 * Purpose: provide the recovered enable contact segment helper for
 * the Player/Pickup gameplay source cluster.
 */
#define EnableContactSegment(enabledSegmentFlags, index) ((enabledSegmentFlags)[index] = 1)

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x424270 Player::ResolvePendingCollisionContact.
 * Purpose: provide the recovered vec3 dot helper for
 * the Player/Pickup gameplay source cluster.
 */
#define Vec3Dot(a, b) ((a).x * (b).x + (a).y * (b).y + (a).z * (b).z)
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-processpendingcontactqueues
 * @recoil-artifact defines .text recoil:function:0x423460: Player::ProcessPendingContactQueues.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ProcessPendingContactQueues from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ProcessPendingContactQueues(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

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
        Checkpoint::UpdatePlayerLapProgressAndNotifyNet(saveState, g_PlayerPendingCheckpointNumber);
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
 * @recoil-match byte
 *
 * Purpose: reimplement Player::ClearPendingContactQueues from the recovered Battlesport gameplay source file.
 */
void __fastcall ClearPendingContactQueues(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
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
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-sq
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: enable probe segments from the local velocity and yaw rate,
 * transform the modal probe points by the motion basis and previous
 * transform, and queue contacts for the enabled root/modal segments.
 * Data: segment endpoints are stored as a flat point list counted per
 * endpoint (retail advances the point and .y induction pointers by 12 per
 * endpoint and passes the endpoint count directly); tags stay per segment.
 */
void __fastcall BuildPendingContactQueues(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    int enabledSegmentFlags[15];
    zVec3 segmentPoints[30];
    int segmentTags[15];

    float localVelLengthSq;
    ZMTH_VECTOR_LENGTH_SQ(localVelLengthSq, &playerState->localVel);
    playerState->noPendingContactsQueued = 1;
    memset(enabledSegmentFlags, 0, sizeof(enabledSegmentFlags));

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        if ((float)fabs(playerState->angVelYaw) > 0.0f) {
            EnableContactSegment(enabledSegmentFlags, 0);
            EnableContactSegment(enabledSegmentFlags, 1);
            EnableContactSegment(enabledSegmentFlags, 2);
            EnableContactSegment(enabledSegmentFlags, 3);
            EnableContactSegment(enabledSegmentFlags, 4);
            EnableContactSegment(enabledSegmentFlags, 5);
        }
    } else if ((float)fabs(playerState->angVelYaw * 3.29999995f) > (float)fabs(playerState->localVel.z)) {
        EnableContactSegment(enabledSegmentFlags, 0);
        EnableContactSegment(enabledSegmentFlags, 1);
        EnableContactSegment(enabledSegmentFlags, 2);
        EnableContactSegment(enabledSegmentFlags, 3);
        EnableContactSegment(enabledSegmentFlags, 4);
        EnableContactSegment(enabledSegmentFlags, 5);
    }

    if (localVelLengthSq > 0.0000001f) {
        if (playerState->localVel.z < 0.0f) {
            EnableContactSegment(enabledSegmentFlags, 0);
            EnableContactSegment(enabledSegmentFlags, 1);
            EnableContactSegment(enabledSegmentFlags, 2);
        } else {
            EnableContactSegment(enabledSegmentFlags, 3);
            EnableContactSegment(enabledSegmentFlags, 5);
        }

        if (masterModalData->masterType == kPlayerMasterTypeSub && playerState->localVel.y > 0.001f) {
            EnableContactSegment(enabledSegmentFlags, 0);
            EnableContactSegment(enabledSegmentFlags, 1);
            EnableContactSegment(enabledSegmentFlags, 2);
            EnableContactSegment(enabledSegmentFlags, 3);
            EnableContactSegment(enabledSegmentFlags, 5);
        }

        if (playerState->localVel.x > 0.001f) {
            EnableContactSegment(enabledSegmentFlags, 6);
            EnableContactSegment(enabledSegmentFlags, 7);
            EnableContactSegment(enabledSegmentFlags, 8);
            EnableContactSegment(enabledSegmentFlags, 4);
        } else {
            if (playerState->localVel.x < -0.001f) {
                EnableContactSegment(enabledSegmentFlags, 9);
                EnableContactSegment(enabledSegmentFlags, 10);
                EnableContactSegment(enabledSegmentFlags, 11);
            }
            EnableContactSegment(enabledSegmentFlags, 4);
        }
    }

    int pointCount = 0;
    for (int probeIndex = 0; probeIndex < masterModalData->probePointCount; ++probeIndex) {
        ZMTH_VECTOR_TRANSFORM_POINT(
            &playerState->motionBasis,
            &playerState->modalProbeWorldByIndex[probeIndex],
            &masterModalData->probePoints[probeIndex]
        );
        ZMTH_VECTOR_TRANSFORM_POINT(
            &playerState->previousTransform,
            &playerState->rootProbeWorldByIndex[probeIndex],
            &masterModalData->probePoints[probeIndex]
        );
    }

    int segmentCount = 0;
    for (int i = 0; i < 15; ++i) {
        if (enabledSegmentFlags[i] == 0) {
            continue;
        }

        segmentTags[segmentCount++] = i;
        zVec3* const rootPoint = &playerState->rootProbeWorldByIndex[i];
        zVec3* const modalPoint = &playerState->modalProbeWorldByIndex[i];
        ConstrainToUnitDistanceFrom(rootPoint, modalPoint);
        segmentPoints[pointCount++] = *rootPoint;
        segmentPoints[pointCount] = *modalPoint;
        segmentPoints[pointCount].y += playerState->projectileSpawnVel.y * g_Player_DeltaTime;
        pointCount++;
    }

    if (pointCount != 0) {
        playerState->noPendingContactsQueued = CollectPendingContactsForSegments(
            saveState,
            (CZDisplayInstanceSegmentEndpoints*)segmentPoints,
            pointCount,
            segmentTags
        );
    }

    if (masterModalData->masterType != kPlayerMasterTypeSub) {
        return;
    }

    pointCount = 0;
    for (int subSegmentIndex = 0; subSegmentIndex < 15; ++subSegmentIndex) {
        if (enabledSegmentFlags[subSegmentIndex] == 0) {
            continue;
        }

        segmentPoints[pointCount] = playerState->rootProbeWorldByIndex[subSegmentIndex];
        segmentPoints[pointCount++].y -= -3.0f;
        segmentPoints[pointCount] = playerState->modalProbeWorldByIndex[subSegmentIndex];
        segmentPoints[pointCount++].y += playerState->projectileSpawnVel.y * g_Player_DeltaTime - -3.0f;
    }

    if (pointCount != 0) {
        playerState->noPendingContactsQueued = CollectPendingContactsForSegments(
            saveState,
            (CZDisplayInstanceSegmentEndpoints*)segmentPoints,
            pointCount,
            segmentTags
        );
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-collectpendingcontactsforsegments
 * @recoil-artifact defines .text recoil:function:0x423b10: Player::CollectPendingContactsForSegments.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::CollectPendingContactsForSegments from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall CollectPendingContactsForSegments(
    zUtil_SaveGameState* saveState,
    CZDisplayInstanceSegmentEndpoints* segmentPairs,
    int endpointCount,
    int* segmentTags
)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    CZClass::gwNodeSetRaycastable(playerState->rootNode, 0);
    g_Variant_CurrentTag = playerState->variantTag;

    PlayerProbeSampleCandidateBuffer hitBatches[24];
    CZDisplayInstance::BuildProbeHitBatchesForSegments(
        g_Player_RuntimeDiScene,
        segmentPairs,
        endpointCount,
        hitBatches
    );

    g_Variant_CurrentTag = g_VariantTag_Current;
    CZClass::gwNodeSetRaycastable(playerState->rootNode, 1);

    // Retail walks the pairs as one flat endpoint array indexed by endpointIndex.
    const zVec3* const endpoints = &segmentPairs->start;
    for (int endpointIndex = 0; endpointIndex < endpointCount; endpointIndex += 2) {
        const int segmentIndex = endpointIndex >> 1;
        ClassifyPendingContactsForSegment(
            saveState,
            &hitBatches[segmentIndex],
            &endpoints[endpointIndex],
            &endpoints[endpointIndex + 1],
            segmentTags[segmentIndex]
        );
    }

    return playerState->preferredCollisionQueue.count == 0 && playerState->playerCollisionQueue.count == 0
            && playerState->worldCollisionQueue.count == 0 && playerState->transferQueue.count == 0
            && playerState->checkpointQueue.count == 0 && playerState->pickupQueue.count == 0
        ? 1
        : 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-classifypendingcontactsforsegment
 * @recoil-artifact defines .text recoil:function:0x423c20: Player::ClassifyPendingContactsForSegment.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ClassifyPendingContactsForSegment from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ClassifyPendingContactsForSegment(
    zUtil_SaveGameState* saveState,
    PlayerProbeSampleCandidateBuffer* sceneResults,
    const zVec3* segmentStart,
    const zVec3* segmentEnd,
    int segmentTag
)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    for (int hitIndex = 0; hitIndex < sceneResults->candidateCount; ++hitIndex) {
        zClassDiPickCandidateEntry* const candidate = &sceneResults->entries[hitIndex];
        PlayerPendingContact* queuedContact;

        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            const int checkpointNumber = HudSensorTracker::ParseCheckpointNumberFromNode(candidate->node);
            g_PlayerPendingCheckpointNumber = checkpointNumber;
            if (checkpointNumber != 0) {
                PLAYER_APPEND_PENDING_CONTACT(&playerState->checkpointQueue, queuedContact);
                queuedContact->sweepStart = *segmentStart;
                queuedContact->sweepEnd = *segmentEnd;
                queuedContact->segmentTag = segmentTag;
                queuedContact->hit = *candidate;
                continue;
            }
        }

        if ((candidate->node->flags & 0x8000000) != 0) {
            continue;
        }

        if (Pickup::ResolveOwnerFromBvolHit(&candidate->node) != 0) {
            PLAYER_APPEND_PENDING_CONTACT(&playerState->pickupQueue, queuedContact);
        } else if ((candidate->node->flags & 0x100000) != 0 && candidate->node->callbackContext != 0) {
            if (*(int*)(candidate->node->callbackContext) == 2) {
                PLAYER_APPEND_PENDING_CONTACT(&playerState->playerCollisionQueue, queuedContact);
            }
        } else if ((unsigned int)(GetNodeDamageHandler(candidate->node)) > 1
            && GetNodeDamageHandler(candidate->node)->timerCallback != 0) {
            PLAYER_APPEND_PENDING_CONTACT(&playerState->transferQueue, queuedContact);
        } else if (candidate->surfaceNormal.y < -0.9f) {
            PLAYER_APPEND_PENDING_CONTACT(&playerState->worldCollisionQueue, queuedContact);
        } else if (candidate->surfaceNormal.y < 0.71f) {
            PLAYER_APPEND_PENDING_CONTACT(&playerState->preferredCollisionQueue, queuedContact);
        } else {
            PlayerContactSurfacePayload* const scenePayload = (PlayerContactSurfacePayload*)(candidate->scenePayload);
            const int impactSlot = scenePayload != 0 ? scenePayload->impactSlot : 0;
            if (impactSlot != 5) {
                continue;
            }
            if (playerState->recentHitValid == 0) {
                playerState->recentHitValid = 1;
            }
        }

        CopyPendingContactPayload(queuedContact, candidate, segmentStart, segmentEnd, segmentTag);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-selectandresolvepreferredpendingcollisioncontact
 * @recoil-artifact defines .text recoil:function:0x423fc0: Player::SelectAndResolvePreferredPendingCollisionContact.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::SelectAndResolvePreferredPendingCollisionContact from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall SelectAndResolvePreferredPendingCollisionContact(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerPendingContact* selectedContact = playerState->preferredCollisionQueue.head;
    PlayerPendingContact** linkCursor = &selectedContact->next;
    while (*linkCursor != 0) {
        selectedContact = selectedContact->SelectPreferred(*linkCursor);
        linkCursor = &(*linkCursor)->next;
    }

    ResolvePendingCollisionContact(saveState, selectedContact);
    playerState->preferredCollisionResolved = 1;
}
} // namespace Player
/**
 * @recoil-anchor recoil:anchor:battlesport-player-playerpendingcontact-selectpreferred
 * @recoil-artifact defines .text recoil:function:0x424010: PlayerPendingContact::SelectPreferred.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot-xz
 * @recoil-match byte
 *
 * Purpose: Select a contact from its rounded horizontal approach score.
 * The negated comparison preserves retail's unordered selection of this.
 */
PlayerPendingContact* __fastcall PlayerPendingContact::SelectPreferred(PlayerPendingContact* rhs)
{
    zVec3 normal = hit.surfaceNormal;
    zVec3 delta;
    zMath::Vec3Subtract(&sweepEnd, &hit.hitPos, &delta);
    float selfApproachDot;
    ZMTH_VECTOR_DOT_XZ(selfApproachDot, &delta, &normal);

    normal = rhs->hit.surfaceNormal;
    zMath::Vec3Subtract(&rhs->sweepEnd, &rhs->hit.hitPos, &delta);
    float rhsApproachDot;
    ZMTH_VECTOR_DOT_XZ(rhsApproachDot, &delta, &normal);

    if (!(-selfApproachDot <= -rhsApproachDot)) {
        return this;
    }
    return rhs;
}

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resolvependingworldcollisioncontact
 * @recoil-artifact defines .text recoil:function:0x424110: Player::ResolvePendingWorldCollisionContact.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ResolvePendingWorldCollisionContact from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ResolvePendingWorldCollisionContact(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerPendingContact* const contact = playerState->worldCollisionQueue.head;
    PreparePendingWorldCollisionResponse(saveState, contact);
    if (playerState->lifecycleState == kPlayerLifecycleLocal) {
        saveState->StartModalLoopSfxHandle(4, 1.0f);
    }
    ResolvePendingCollisionContact(saveState, playerState->worldCollisionQueue.head);
}
} // namespace Player
namespace PlayerPickupContact {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-playerpickupcontact-passescollectiontest
 * @recoil-artifact defines .text recoil:function:0x424150: PlayerPickupContact::PassesCollectionTest.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement PlayerPickupContact::PassesCollectionTest from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall PassesCollectionTest(zUtil_SaveGameState* saveState, PlayerPendingContact* contact)
{
    (void)saveState;
    zUtil_PlayerStateStorage* const playerState
        = (zUtil_PlayerStateStorage*)((void*)(g_GameStateOrMapTable->playerState));

    g_Variant_CurrentTag = playerState->variantTag;
    CZClass::gwNodeSetRaycastable(contact->hit.node, 0);
    CZDisplayInstance::SetBreakOnFirstCandidate(1);
    CZDisplayInstance::SetStopAfterFirstHit(0x40000);

    PlayerProbeSampleCandidateBuffer rayData;
    const int raycastResult = CZDisplayInstance::RaycastFindClosest(
        g_Player_RuntimeDiScene,
        contact->hit.hitPos.x,
        contact->hit.hitPos.y + 1.0f,
        contact->hit.hitPos.z,
        contact->hit.node->cachedSphereCenter[0],
        contact->hit.node->cachedSphereCenter[1] + 1.0f,
        contact->hit.node->cachedSphereCenter[2],
        &rayData
    );

    CZDisplayInstance::SetBreakOnFirstCandidate(0);
    CZClass::gwNodeSetRaycastable(contact->hit.node, 1);

    return raycastResult == 0 && rayData.candidateCount != 0 ? 0 : 1;
}
} // namespace PlayerPickupContact
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-processpendingpickupcontacts
 * @recoil-artifact defines .text recoil:function:0x424210: Player::ProcessPendingPickupContacts.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ProcessPendingPickupContacts from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ProcessPendingPickupContacts(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    if ((zInput_GameStateOrMapTablePartial*)(saveState) != g_GameStateOrMapTable) {
        return;
    }

    if (playerState->lifecycleState == 4 || playerState->lifecycleState == 5) {
        return;
    }

    PlayerPendingContact* contact = playerState->pickupQueue.head;
    while (contact != 0) {
        if (PlayerPickupContact::PassesCollectionTest(saveState, contact) != 0) {
            Pickup::OnCollected(contact->hit.node, saveState);
        }

        contact = contact != 0 ? contact->next : 0;
    }
}
} // namespace Player
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.player-contact.vector-length
 *
 * Purpose: write FSQRT of the grouped (x*x + y*y) + z*z sum into the result
 * float as binary32.
 * Reconstruction: player_contact.cpp-resident copy of the reviewed Camera.c
 * Vec3Length body in output-parameter macro form; retail 0x424270 expands it at
 * [0x4243a6,0x4243c4) and stores straight into the caller's float.
 * Raw assembly: reviewed (Pro batch Z, run 96d501c4); permission covers only the
 * listed consumer range.
 * Contract: vector is captured once and must identify a valid readable zVec3;
 * result must be a named writable, non-volatile float object. ECX is clobbered;
 * integer flags and the x87 control word are unchanged by this body; x87
 * entry/peak/exit depths are 0/3/0 on normal completion. The final store is
 * binary32; intermediate precision, status and exceptions follow the existing
 * length-family contract. Arguments must not collide with the internal capture
 * name lengthVector (a caller variable of that name would turn the capture
 * initializer into a self-reference).
 * Fallback: mathematical reference only; identical rounding, NaN handling and
 * exception behaviour are not promised.
 */
#define PLAYER_VECTOR_LENGTH(result, vector)                                                                           \
    do {                                                                                                               \
        const zVec3* const lengthVector = (vector);                                                                    \
        __asm { \
            __asm mov ecx, lengthVector \
            __asm fld dword ptr [ecx]zVec3.x \
            __asm fmul dword ptr [ecx]zVec3.x \
            __asm fld dword ptr [ecx]zVec3.y \
            __asm fmul dword ptr [ecx]zVec3.y \
            __asm fld dword ptr [ecx]zVec3.z \
            __asm fmul dword ptr [ecx]zVec3.z \
            __asm fxch st(1) \
            __asm faddp st(2), st \
            __asm faddp st(1), st \
            __asm fsqrt \
            __asm fstp result }                                                                              \
    } while (0)
#else
#define PLAYER_VECTOR_LENGTH(result, vector)                                                                           \
    do {                                                                                                               \
        const zVec3* const lengthVector = (vector);                                                                    \
        (result) = (float)sqrt(                                                                                        \
            (lengthVector->x * lengthVector->x + lengthVector->y * lengthVector->y)                                    \
            + lengthVector->z * lengthVector->z                                                                        \
        );                                                                                                             \
    } while (0)
#endif

/**
 * Original inline helper; no standalone retail function exists.
 * Evidence: retail 0x425770 stores surfaceNormal * 20 through the output
 * pointer and re-reads the stored y for the clamp (same helper shape as
 * Camera.c Vec3ScaleTo).
 * Purpose: scale a vector by a scalar into an output vector.
 */
inline void Vec3ScaleTo(const zVec3* vec, float scale, zVec3* out)
{
    out->x = vec->x * scale;
    out->y = vec->y * scale;
    out->z = vec->z * scale;
}

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resolvependingcollisioncontact
 * @recoil-artifact defines .text recoil:function:0x424270: Player::ResolvePendingCollisionContact.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-cross
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot-xz
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-sq
 * @recoil-raw-consumer recoil:raw-asm:battlesport.player-contact.vector-length
 *
 *
 * Raw assembly: reviewed (Pro batch Z, run 96d501c4), each range separately:
 * Vec3Subtract [0x4242f7,0x42431a), [0x4243fb,0x42441e), [0x424485,0x4244a8);
 * vector dot [0x424326,0x424345); vector length [0x4243a6,0x4243c4); Vec3Add
 * [0x42444d,0x424470), [0x4244c5,0x4244e8), [0x42451d,0x424540),
 * [0x424727,0x42474d); vector cross [0x4245b6,0x4245f7), [0x42460f,0x424650);
 * XZ dot [0x42467a,0x42468f), [0x4246c8,0x4246dd); squared length
 * [0x42479d,0x4247bf) (requires player_contact.cpp /Ob1).
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ResolvePendingCollisionContact from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ResolvePendingCollisionContact(zUtil_SaveGameState* saveState, PlayerPendingContact* contact)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    CZNodePartial* const hitNode = contact->hit.node;

    zVec3 sweepStart = contact->sweepStart;
    zVec3 sweepEnd = contact->sweepEnd;
    zVec3 contactPoint;
    contactPoint.x = contact->hit.hitPos.x;
    contactPoint.y = contact->hit.hitPos.y;
    contactPoint.z = contact->hit.hitPos.z;
    zVec3 contactNormal = contact->hit.surfaceNormal;

    zVec3 contactToSweepStart;
    zMath::Vec3Subtract(&sweepStart, &contactPoint, &contactToSweepStart);
    float sweepStartSide;
    ZMTH_VECTOR_DOT(sweepStartSide, &contactToSweepStart, &contactNormal);
    if (sweepStartSide < 0.0f) {
        contactNormal.x *= -1.0f;
        contactNormal.y *= -1.0f;
        contactNormal.z *= -1.0f;
    }

    if (contactNormal.x == 0.0f && contactNormal.z == 0.0f) {
        return;
    }

    // Retail [0x4243a6,0x4243c4): the player_contact.cpp-resident length island.
    float localSpeed;
    PLAYER_VECTOR_LENGTH(localSpeed, &playerState->localVel);
    const float originalNormalY = contactNormal.y;
    sweepStart.y = 0.0f;
    sweepEnd.y = 0.0f;
    contactPoint.y = 0.0f;
    contactNormal.y = 0.0f;

    zVec3 contactToSweepEnd;
    zMath::Vec3Subtract(&sweepEnd, &contactPoint, &contactToSweepEnd);
    zMath::Vec3NormalizeXZ(&contactNormal, &contactNormal);

    zVec3 reflectedSweepDir;
    zMath::Vec3Reflect(&contactNormal, &contactToSweepEnd, &reflectedSweepDir);
    zVec3 reflectedPoint;
    zMath::Vec3Add(&contactPoint, &reflectedSweepDir, &reflectedPoint);
    zVec3 positionCorrection;
    zMath::Vec3Subtract(&reflectedPoint, &sweepEnd, &positionCorrection);
    Vec3FastNormalize(&positionCorrection);

    zVec3 correctedWorldPos;
    zMath::Vec3Add(&playerState->worldPos, &positionCorrection, &correctedWorldPos);
    playerState->worldPos = correctedWorldPos;

    for (int i = 0; i < masterModalData->probePointCount; ++i) {
        zMath::Vec3Add(
            &playerState->modalProbeWorldByIndex[i],
            &positionCorrection,
            &playerState->modalProbeWorldByIndex[i]
        );
    }

    const int probeResolved = TryResolvePendingCollisionProbeSweep(saveState);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    if (probeResolved == 0) {
        const float collisionDampingA = masterModalData->collisionDampingA;
        float projectileVelY = playerState->projectileSpawnVel.y;
        zMath::Vec3NormalizeXZ(&reflectedSweepDir, &reflectedSweepDir);

        float tangentSpeed;
        float normalDot;
        zVec3 surfaceTangent;
        ZMTH_VECTOR_CROSS(&reflectedSweepDir, &contactNormal, &surfaceTangent);
        ZMTH_VECTOR_CROSS(&contactNormal, &surfaceTangent, &surfaceTangent);

        reflectedSweepDir.x *= localSpeed;
        reflectedSweepDir.y *= localSpeed;
        reflectedSweepDir.z *= localSpeed;

        ZMTH_VECTOR_DOT_XZ(tangentSpeed, &reflectedSweepDir, &surfaceTangent);
        zVec3 tangentVelocity;
        tangentVelocity.x = surfaceTangent.x * tangentSpeed;
        tangentVelocity.y = surfaceTangent.y * tangentSpeed;
        tangentVelocity.z = surfaceTangent.z * tangentSpeed;

        ZMTH_VECTOR_DOT_XZ(normalDot, &reflectedSweepDir, &contactNormal);
        zVec3 normalVelocity;
        normalVelocity.x = contactNormal.x * (collisionDampingA * normalDot);
        normalVelocity.y = contactNormal.y * (collisionDampingA * normalDot);
        normalVelocity.z = contactNormal.z * (collisionDampingA * normalDot);
        zMath::Vec3Add(&normalVelocity, &tangentVelocity, &playerState->projectileSpawnVel);

        if (playerState->airborneFlag != 0) {
            if (originalNormalY > 0.01f) {
                if (projectileVelY < 0.0f) {
                    projectileVelY *= -0.5f;
                }
                CZClass::gwNodeSetCellPickable(hitNode, 1);
            }

            float spawnSpeedSq;
            ZMTH_VECTOR_LENGTH_SQ(spawnSpeedSq, &playerState->projectileSpawnVel);
            if (spawnSpeedSq < 1.0f) {
                Vec3ScaleTo(&contactNormal, 10.0f, &playerState->projectileSpawnVel);
            }
        }

        playerState->projectileSpawnVel.y = projectileVelY;
        zMath::Vec3RotateY(&playerState->localVel, &playerState->projectileSpawnVel, -playerState->restartYawRad);
    }

    const float yawImpulseCross = reflectedSweepDir.x * contactToSweepEnd.z - reflectedSweepDir.z * contactToSweepEnd.x;
    const int yawImpulseSign = yawImpulseCross < 0.0f ? -1 : 1;
    playerState->angVelYaw += ((float)(yawImpulseSign)*localSpeed) * masterModalData->collisionDampingB;

    if (saveState != (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        return;
    }

    const float impactGain = __min(1.0f, localSpeed / masterModalData->maxSpeed);
    saveState->StartModalLoopSfxHandle(4, impactGain);
    // Retail 0x4248bc calls through g_zInputFfEffectSet without a second null test.
    if (zInputDIIsForceFeedbackEnabled(g_zInputFfEffectSet) != 0) {
        g_zInputFfEffectSet->PlayCollisionImpactEffect(&contactNormal, impactGain);
    }
}
} // namespace Player
extern const zVec3 g_Player_ConstZeroVec3;

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-preparependingworldcollisionresponse
 * @recoil-artifact defines .text recoil:function:0x4248e0: Player::PreparePendingWorldCollisionResponse.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-rotate-rows-in-place
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::PreparePendingWorldCollisionResponse from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall
PreparePendingWorldCollisionResponse(zUtil_SaveGameState* saveState, PlayerPendingContact* worldContacts)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

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

    const float restoreYOffset = masterModalData->masterType == kPlayerMasterTypeSub ? -1.0f : 0.0f;
    playerState->worldPos.x = playerState->previousTransform.posX;
    playerState->worldPos.y = playerState->previousTransform.posY + restoreYOffset;
    playerState->worldPos.z = playerState->previousTransform.posZ;
    playerState->vehicleRotationAngles = playerState->cachedVehicleRotationAngles;
    playerState->angVel = g_Player_ConstZeroVec3;

    if (playerState->projectileSpawnVel.y > 0.0f) {
        playerState->projectileSpawnVel.y *= -0.8f;
    }

    zMath::MatBuildEulerRotation3x3(
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad,
        &playerState->motionBasis
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    playerState->localVel = playerState->projectileSpawnVel;
    ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE(&playerState->motionBasis, &playerState->localVel);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resolvependingplayercollisioncontact
 * @recoil-artifact defines .text recoil:function:0x424ac0: Player::ResolvePendingPlayerCollisionContact.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match source
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ResolvePendingPlayerCollisionContact from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ResolvePendingPlayerCollisionContact(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    PlayerPendingContact* const queuedContact = playerState->playerCollisionQueue.head;
    zClassDiPickCandidateEntry contactSnapshot = queuedContact->hit;
    zVec3 transferredLocalVel = playerState->projectileSpawnVel;

    PlayerCollisionContactContextPartial* const targetContext
        = (PlayerCollisionContactContextPartial*)(void*)(contactSnapshot.node->callbackContext);
    zUtil_SaveGameState* const targetSaveState = targetContext->saveState;
    zUtil_PlayerStateStorage* const targetPlayerState = targetSaveState->playerState;
    PlayerMasterCommonData* const targetCommonData = targetPlayerState->masterCommonData;
    PlayerMasterModalData* const targetModalData = targetSaveState->primaryModalState->masterModalData;

    float massScale = masterModalData->mass * targetModalData->invMass;
    transferredLocalVel.x *= massScale;
    transferredLocalVel.y *= massScale;
    transferredLocalVel.z *= massScale;
    zMath::Vec3RotateY(&transferredLocalVel, &transferredLocalVel, -targetPlayerState->restartYawRad);
    transferredLocalVel.y = 0.0f;
    zMath::Vec3Add(&transferredLocalVel, &targetPlayerState->localVel, &targetPlayerState->localVel);

    ResolvePendingCollisionContact(saveState, playerState->playerCollisionQueue.head);

    if (targetPlayerState->lifecycleState == kPlayerLifecycleAi) {
        massScale *= 1.10000002f;
        // Retail reads the named 0.2f constant's storage (0x4d073c) here, not a separate literal.
        if ((targetPlayerState->statusMeterValue - massScale) * targetCommonData->invMaxHealth
            > kPlayerWorldCollisionStackDrop) {
            HitCallbackRecordContextAndTimedStatus(targetSaveState, 0, 0, massScale);
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-vec3-fastnormalize
 * @recoil-artifact defines .text recoil:function:0x424bf0: Player::Vec3FastNormalize
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:battlesport.player.vec3-fast-normalize.fast-sqrt-estimate recoil:function:0x424bf0
 * @recoil-raw-asm recoil:raw-asm:battlesport.player.vec3-fast-normalize.fast-sqrt-estimate
 * @recoil-match source
 *
 * Raw assembly: the reviewed full-XYZ dot island bound directly to the vec
 * parameter (retail loads ECX and EDX from the one home [ebp-8]) and one
 * in-body 13-byte fast-sqrt estimate island at retail [0x424c4a,0x424c57).
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: scale short nonzero contact deltas with the fast approximate
 * square-root normalizer used by collision contact resolution.
 * Source owner: player contact unit-distance helper subsystem, not
 * player_camera_control_state_bridge or the broader Player C++ class.
 * Evidence: retail body reads only the zVec3 argument and
 * g_Player_CollisionContactResolveScale, uses the integer half-exponent
 * approximation, and returns whether the vector was rescaled.
 */
int __fastcall Vec3FastNormalize(zVec3* vec)
{
    // vecLength holds the squared length until the fast estimate replaces it.
    float vecLength;
    ZMTH_VECTOR_DOT_BOUND(vecLength, vec, vec);
    float lengthSq = vecLength;
    int scaled = 0;
    if (vecLength < 0.01f) {
        if (vecLength == 0.0f) {
            return 0;
        }

        // Raw-assembly fast square-root estimate: retail transforms the named lengthSq
        // bits through EAX ((bits >> 1) + 0x1fc00000) into the named vecLength local.
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        __asm {
            mov eax, lengthSq
            sar eax, 1
            add eax, 01fc00000h
            mov vecLength, eax
        }
#else
        {
            int estimateBits;
            memcpy(&estimateBits, &lengthSq, sizeof estimateBits);
            estimateBits = (estimateBits >> 1) + 0x1fc00000;
            memcpy(&vecLength, &estimateBits, sizeof vecLength);
        }
#endif
        const float scale = g_Player_CollisionContactResolveScale / (vecLength + 0.00000001f);
        scaled = 1;
        vec->x *= scale;
        vec->y *= scale;
        vec->z *= scale;
    }
    return scaled;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-constraintounitdistancefrom
 * @recoil-artifact defines .text recoil:function:0x424c90: Player::ConstrainToUnitDistanceFrom
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: constrain a nearby position to the contact resolve distance around
 * a center point.
 * Source owner: player contact unit-distance helper subsystem, not
 * player_camera_control_state_bridge or the broader Player C++ class.
 * Evidence: retail body forms a stack zVec3 delta, calls
 * Player::Vec3FastNormalize, and writes back center plus normalized delta
 * only when the helper reports a short nonzero contact vector.
 */
void __fastcall ConstrainToUnitDistanceFrom(zVec3* pos, const zVec3* center)
{
    zVec3 delta;
    zMath::Vec3Subtract(pos, center, &delta);
    if (Vec3FastNormalize(&delta) == 0) {
        return;
    }

    zMath::Vec3Add(center, &delta, pos);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-processtransfercontactqueue
 * @recoil-artifact defines .text recoil:function:0x424d00: Player::ProcessTransferContactQueue.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ProcessTransferContactQueue from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ProcessTransferContactQueue(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    PlayerPendingContact* contact = playerState->transferQueue.head;
    float localSpeedSq;
    ZMTH_VECTOR_DOT(localSpeedSq, &playerState->localVel, &playerState->localVel);
    const float transferDamage
        = localSpeedSq * kPlayerTransferDamageScale / (masterModalData->maxSpeed * masterModalData->maxSpeed);

    while (contact != 0) {
        const float callbackResult = OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback(
            &contact->sweepStart,
            (OptCatalogHitEventPartial*)(void*)contact,
            transferDamage
        );
        if (callbackResult > 0.0f) {
            PlayerPendingContact* const next = contact->next;
            PLAYER_REMOVE_EXISTING_PENDING_CONTACT(&playerState->transferQueue, contact);
            if (contact != 0) {
                PLAYER_APPEND_EXISTING_PENDING_CONTACT(&playerState->preferredCollisionQueue, contact);
            }
            contact = next;
        } else {
            RecordNodeFlagsForRestore(contact->hit.node);
            CZClass::gwNodeSetCellPickable(contact->hit.node, 0);
            CZClass::gwNodeSetRaycastable(contact->hit.node, 0);
            contact = contact->next;
        }
    }

    // Retail multiplies each member by the pooled literal (fld member; fmul m32).
    playerState->localVel.x *= 0.666700006f;
    playerState->localVel.y *= 0.666700006f;
    playerState->localVel.z *= 0.666700006f;
    playerState->projectileSpawnVel.x *= 0.666700006f;
    playerState->projectileSpawnVel.y *= 0.666700006f;
    playerState->projectileSpawnVel.z *= 0.666700006f;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-tryresolvependingcollisionprobesweep
 * @recoil-artifact defines .text recoil:function:0x424ed0: Player::TryResolvePendingCollisionProbeSweep.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::TryResolvePendingCollisionProbeSweep from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall TryResolvePendingCollisionProbeSweep(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    ClearPendingContactQueues(saveState);

    CZDisplayInstanceSegmentEndpoints segmentPairs[6];
    int segmentTags[6];
    for (int i = 0; i < 6; ++i) {
        segmentPairs[i].start = playerState->rootProbeWorldByIndex[i];
        segmentPairs[i].end = playerState->modalProbeWorldByIndex[i];
        segmentTags[i] = i;
    }

    CollectPendingContactsForSegments(saveState, segmentPairs, 12, segmentTags);
    PLAYER_MOVE_TRANSFER_CONTACTS_TO_PREFERRED_COLLISION(playerState);

    if (playerState->preferredCollisionQueue.count == 0 && playerState->playerCollisionQueue.count == 0) {
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
 * @recoil-match byte
 *
 * Source model: checkpoint-node name parser used by player contact handling;
 * MFC CString construction/Right/destruction are provider behavior.
 * Touched data: no authored globals; reads only the node flags, callback
 * context flags, and context node name.
 * Purpose: parse a nonnegative checkpoint number from the callback context node
 * name when checkpoint flags permit it.
 */
int __fastcall HudSensorTracker::ParseCheckpointNumberFromNode(CZNodePartial* node)
{
    if ((node->flags & 0x200000) == 0) {
        return 0;
    }

    CZNodePartial* const contextNode = node->callbackContext;
    if ((contextNode->auxFlags & 2) == 0) {
        return 0;
    }

    CString name(contextNode->name);
    // The canonical max-style expansion produces the retail branchless clamp.
    // Preserve the comparison and selected-value expression of the clamp.
    CString checkpointNumber = name.Right(0 > name.GetLength() - 10 ? 0 : name.GetLength() - 10);
    if (checkpointNumber.GetLength() != 0) {
        const long parsedNumber = atol((const char*)checkpointNumber);
        return parsedNumber < 0 ? 0 : (int)(parsedNumber);
    }

    return 0;
}
namespace Checkpoint {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-checkpoint-updateplayerlapprogressandnotifynet
 * @recoil-artifact defines .text recoil:function:0x425150: Checkpoint::UpdatePlayerLapProgressAndNotifyNet
 * @recoil-match byte
 *
 * Purpose: Marks checkpoint visits, completes laps after all checkpoint flags
 * are set, and notifies networking of lap progress.
 */
void __fastcall UpdatePlayerLapProgressAndNotifyNet(zUtil_SaveGameState* saveState, int checkpointIndex)
{
    const int checkpointCount = g_HudSensorTracker.checkpointCount;
    PlayerCheckpointLapProgressView* const playerProgress = (PlayerCheckpointLapProgressView*)(saveState->playerState);

    if (playerProgress->checkpointVisitedFlags[checkpointIndex] != 0) {
        return;
    }

    playerProgress->checkpointVisitedFlags[checkpointIndex] = 1;
    if (checkpointCount != checkpointIndex) {
        return;
    }

    int allPriorCheckpointsVisited = 1;
    for (int index = 0; index < checkpointCount; ++index) {
        allPriorCheckpointsVisited
            = allPriorCheckpointsVisited != 0 && playerProgress->checkpointVisitedFlags[index + 1] != 0 ? 1 : 0;
        playerProgress->checkpointVisitedFlags[index + 1] = 0;
    }

    if (allPriorCheckpointsVisited == 0) {
        return;
    }

    playerProgress->lapTimeDelta = g_Time_AccumulatedTimeSec - playerProgress->lapTimestampSec;
    playerProgress->lapTimestampSec = g_Time_AccumulatedTimeSec;
    playerProgress->lapTimeSec = g_Time_AccumulatedTimeSec - playerProgress->checkpointTimestampSec;
    playerProgress->lapCompletionCount += 1;
    GameNet::SendPkt0EPlayerLapProgress(saveState);
}
} // namespace Checkpoint
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-collectpendingcollisioncontactsforquadprobe
 * @recoil-artifact defines .text recoil:function:0x4251f0: Player::CollectPendingCollisionContactsForQuadProbe.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: raise the four quad probe points, transform them by the motion
 * basis and collect contacts along the quad edges.
 * Data: retail builds eight segments but passes twelve endpoints, so only
 * the first six are probed; the 0x140-byte frame holds sixteen tag slots
 * below the eight segment pairs, and the zero result flag is stored and
 * reloaded on the empty-queue return.
 */
int __fastcall CollectPendingCollisionContactsForQuadProbe(zUtil_SaveGameState* saveState, float expandRadius)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    const PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    int hasContacts = 0;
    zVec3 probePoints[4];
    CZDisplayInstanceSegmentEndpoints segmentPairs[8];
    int segmentTags[16];

    ClearPendingContactQueues(saveState);

    probePoints[0] = masterModalData->probePoints[0];
    probePoints[0].y += expandRadius;
    probePoints[1] = masterModalData->probePoints[2];
    probePoints[1].y += expandRadius;
    probePoints[2] = masterModalData->probePoints[3];
    probePoints[2].y += expandRadius;
    probePoints[3] = masterModalData->probePoints[5];
    probePoints[3].y += expandRadius;

    ZMTH_VECTOR_TRANSFORM_POINT(&playerState->motionBasis, &playerState->modalProbeWorldByIndex[0], &probePoints[0]);
    ZMTH_VECTOR_TRANSFORM_POINT(&playerState->motionBasis, &playerState->modalProbeWorldByIndex[2], &probePoints[1]);
    ZMTH_VECTOR_TRANSFORM_POINT(&playerState->motionBasis, &playerState->modalProbeWorldByIndex[3], &probePoints[2]);
    ZMTH_VECTOR_TRANSFORM_POINT(&playerState->motionBasis, &playerState->modalProbeWorldByIndex[5], &probePoints[3]);

    segmentPairs[0].start = playerState->modalProbeWorldByIndex[0];
    segmentPairs[0].end = playerState->modalProbeWorldByIndex[2];
    segmentTags[0] = 0;
    segmentPairs[1].start = playerState->modalProbeWorldByIndex[2];
    segmentPairs[1].end = playerState->modalProbeWorldByIndex[0];
    segmentTags[1] = 1;
    segmentPairs[2].start = playerState->modalProbeWorldByIndex[2];
    segmentPairs[2].end = playerState->modalProbeWorldByIndex[3];
    segmentTags[2] = 2;
    segmentPairs[3].start = playerState->modalProbeWorldByIndex[3];
    segmentPairs[3].end = playerState->modalProbeWorldByIndex[2];
    segmentTags[3] = 3;
    segmentPairs[4].start = playerState->modalProbeWorldByIndex[3];
    segmentPairs[4].end = playerState->modalProbeWorldByIndex[5];
    segmentTags[4] = 4;
    segmentPairs[5].start = playerState->modalProbeWorldByIndex[5];
    segmentPairs[5].end = playerState->modalProbeWorldByIndex[3];
    segmentTags[5] = 5;
    segmentPairs[6].start = playerState->modalProbeWorldByIndex[5];
    segmentPairs[6].end = playerState->modalProbeWorldByIndex[0];
    segmentTags[6] = 6;
    segmentPairs[7].start = playerState->modalProbeWorldByIndex[0];
    segmentPairs[7].end = playerState->modalProbeWorldByIndex[5];
    segmentTags[7] = 7;

    CollectPendingContactsForSegments(saveState, segmentPairs, 12, segmentTags);

    PLAYER_MOVE_TRANSFER_CONTACTS_TO_PREFERRED_COLLISION(playerState);

    if (playerState->preferredCollisionQueue.count != 0 || playerState->playerCollisionQueue.count != 0) {
        hasContacts = 1;
    }
    return hasContacts;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applypendingcollisionprobevelocity
 * @recoil-artifact defines .text recoil:function:0x425770: Player::ApplyPendingCollisionProbeVelocity.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-rotate-rows-in-place
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ApplyPendingCollisionProbeVelocity from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ApplyPendingCollisionProbeVelocity(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    if (playerState->collisionProbeResolved == 0) {
        playerState->worldPos.x = playerState->previousTransform.posX;
        playerState->worldPos.y = playerState->previousTransform.posY;
        playerState->worldPos.z = playerState->previousTransform.posZ;
        float cachedYaw = playerState->cachedYawRad;
        playerState->restartYawRad = cachedYaw;
        zMath::MatBuildEulerRotation3x3(
            playerState->vehiclePitchRad,
            playerState->restartYawRad,
            playerState->vehicleRollRad,
            &playerState->motionBasis
        );
        playerState->motionBasis.posX = playerState->worldPos.x;
        playerState->motionBasis.posY = playerState->worldPos.y;
        playerState->motionBasis.posZ = playerState->worldPos.z;
    }

    PlayerPendingContact* contact = playerState->preferredCollisionQueue.head;
    if (contact == 0) {
        contact = playerState->playerCollisionQueue.head;
    }
    if (contact == 0) {
        return;
    }

    const float previousY = playerState->projectileSpawnVel.y;
    const zVec3 surfaceNormal = contact->hit.surfaceNormal;
    Vec3ScaleTo(&surfaceNormal, 20.0f, &playerState->projectileSpawnVel);

    playerState->projectileSpawnVel.y = playerState->projectileSpawnVel.y <= 0.0f
        ? __min(previousY, playerState->projectileSpawnVel.y)
        : __max(previousY, playerState->projectileSpawnVel.y);

    playerState->localVel = playerState->projectileSpawnVel;
    ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE(&playerState->motionBasis, &playerState->localVel);
}
} // namespace Player
