// Battlesport compilation unit between RecoilFrame.cpp and saveload.cpp, inferred
// from the retail object boundary [0x431bf0, 0x434660): its .rdata constants
// [0x4d1280, 0x4d12c8) repeat 0.0f, 1.0f, -1.0f, -3.0f and 60.0f that the
// neighbouring objects pool separately, and its .data [0x4dcd88, 0x4dcfe4),
// .bss [0x4f3f10, 0x4f3fac) and two CRT initializers are its own. Original
// filename unresolved; RecoilNet.cpp is a provisional name (2026-10-02).

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/about.h"
#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/net_ui.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "Battlesport/recoil_version.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_dialog.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zimage.h"

#include <algorithm>
#include <commdlg.h>
#include <math.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Battlesport/game_net.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/briefing.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/mission.h"
#include "Battlesport/net_ui.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"

#if defined(_MSC_VER) && _MSC_VER < 1300 && !defined(_DEBUG)
/**
 * Original-source inline provider-boundary restore from MFC42 AFXCMN.INL:
 * _AFXCMN_INLINE CSpinButtonCtrl::CSpinButtonCtrl() { }.
 * No standalone Recoil-authored retail function exists; this source restores
 * the VC5/MFC42 common-control inline suppressed by Mfc42Abi.h so the config
 * dialog emits the retail local spin-control vftable reference.
 * Purpose: Construct embedded MFC42 spin-button controls with provider inline
 * behavior for NetSessionConfigDialog.
 */
inline CSpinButtonCtrl::CSpinButtonCtrl() { }
#endif

#include <shellapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER) || _MSC_VER >= 1300
#include <new>
#endif

extern "C" HWND g_RecoilApp_hWndMain;

static const unsigned int kGameNetRemoteAltGunDispatchFlag = 0x2000000u;
static const unsigned int kGameNetRemoteCloneNodeFlag = 0x400000u;
static const float kGameNetRemoteUnlimitedAmmo = 123456792.0f;

extern "C" NetPkt10_QSandEvent g_NetPkt10_QSandEventRelayBuf;

struct GameNetReaderArray {
    int countTag;
    int count;
    zReader::Node nodes[1];
};

namespace GameNetSpawnPointList {
/**
 * Purpose: Reset the GameNet-owned spawn-point list header to an empty state.
 */
void __cdecl InitGlobals()
{
    g_GameNetSpawnPointList.flags = 0;
    g_GameNetSpawnPointTail = 0;
    g_GameNetSpawnPointHead = 0;
    g_GameNetSpawnPointCount = 0;
}
} // namespace GameNetSpawnPointList

namespace GameNetPlayerRowList {
/**
 * Purpose: Reset the GameNet-owned player-row list header to an empty state.
 */
void __cdecl Reset()
{
    g_GameNetPlayerRowList.flags = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowCount = 0;
}
} // namespace GameNetPlayerRowList

namespace GameNet {
/**
 * Purpose: Register gameplay packet handlers and option catalog callbacks once.
 */
void __fastcall RegisterGameplayHandlersAndOptCatalogCallbacks()
{
    if (g_GameNet_HandlersRegistered == 0) {
        zNetwork::RegisterPacketHandler(6, (zNetworkPacketHandler)&HandlePkt06PlayerStateSnapshot, 2);
        zNetwork::RegisterPacketHandler(7, (zNetworkPacketHandler)&HandlePkt07_AltGunDispatch, 2);
        zNetwork::RegisterPacketHandler(0x0a, (zNetworkPacketHandler)&OptCatalog::HandlePkt0ARemoveRuntimeRelay, 2);
        zNetwork::RegisterPacketHandler(1, (zNetworkPacketHandler)&ReassignPlayerColorsAndRefreshRows, 2);
        zNetwork::RegisterPacketHandler(8, (zNetworkPacketHandler)&HandlePkt08PlayerKillEvent, 2);
        zNetwork::RegisterPacketHandler(9, (zNetworkPacketHandler)&HandlePkt09PlayerScoreboardSnapshot, 2);
        zNetwork::RegisterPacketHandler(0x0b, (zNetworkPacketHandler)&HandlePkt0BChatMessage, 2);
        zNetwork::RegisterPacketHandler(0x0e, (zNetworkPacketHandler)&HandlePkt0EPlayerLapProgress, 2);
        zNetwork::RegisterPacketHandler(0x0c, (zNetworkPacketHandler)&HandlePkt0CHudTimerStatusBits, 2);
        zNetwork::RegisterPacketHandler(0x0d, (zNetworkPacketHandler)&HandlePkt0DHudTimerPanelState, 2);
        zNetwork::RegisterPacketHandler(0x0f, (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback, 2);
        zNetwork::RegisterPacketHandler(0x10, (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback, 2);
        zNetwork::RegisterPacketHandler(0x11, (zNetworkPacketHandler)&Pickup::HandlePkt11SpawnDelta, 2);
        zNetwork::RegisterPacketHandler(0x12, (zNetworkPacketHandler)&Pickup::HandlePkt12AirdropSpawnChuteRelay, 2);
        zNetwork::RegisterPacketHandler(0x13, (zNetworkPacketHandler)&HandlePkt13EffectAnimActivationRecord, 2);
        zNetwork::RegisterPacketHandler(0x14, (zNetworkPacketHandler)&HandlePkt14HudTimerAndFlagsSync, 2);
        zNetwork::RegisterPacketHandler(3, (zNetworkPacketHandler)&HandlePkt03RemoveRemotePlayer, 2);
        g_GameNet_HandlersRegistered = 1;
    }

    g_zDEClientCraterNetRelayCallback = (zDEClient_NetRelayCallback)&zDEClient_Crater::Execute;
    g_zDEClientQSandNetRelayCallback = (zDEClient_NetRelayCallback)&SendPkt10QSandEvent;
    g_OptCatalog_AllocRuntimeGateCallback = &OptCatalog::AltGunDispatchAllocRuntimeGateCallback;
    g_OptCatalog_AltGunDispatchNoOpCallback = &AltGunDispatchNoOpCallback;
    g_OptCatalog_RemoveRuntimeRelayCallback = &OptCatalog::SendPkt0ARemoveRuntimeRelay;
    zEffect_Anim::SetActivationDispatchContext(&SendPkt13EffectAnimActivationRecord, 0x0c);
}
} // namespace GameNet

namespace Net {
/**
 * Purpose: Initialize multiplayer mission state from net.zrd spawn points,
 * create the local player row, initialize host HUD timer state, and respawn
 * the local player.
 */
void __cdecl InitFromZrd()
{
    zUtil_SaveGameState* saveState = g_PlayerSaveStateList.head;
    while (saveState != 0) {
        zUtil_PlayerStateStorage* const playerState = saveState->playerState;
        if (playerState->lifecycleState == 2) {
            playerState->lifecycleState = 4;
            CZNodePartial* const rootNode = playerState->rootNode;
            CZClass::RemoveChild(rootNode->listA[0], rootNode);
        }
        saveState = saveState != 0 ? saveState->next : 0;
    }

    zTurret_System::DisableTickCallback();
    zReader::Node* const treeRoot = zReader::Load("net.zrd", 0, 0);
    if (treeRoot != 0) {
        GameNetReaderArray* const rootArray = (GameNetReaderArray*)(treeRoot->value.ptr);
        GameNetReaderArray* const spawnArray = (GameNetReaderArray*)(rootArray->nodes[0].value.ptr);
        int spawnPointCount = spawnArray->count - 1;
        for (int index = 0; index < spawnPointCount; ++index) {
            GameNetSpawnPoint* const spawnPoint = (GameNetSpawnPoint*)(::operator new(sizeof(GameNetSpawnPoint)));
            memset(spawnPoint, 0, sizeof(GameNetSpawnPoint));
            if (g_GameNetSpawnPointCount == 0) {
                g_GameNetSpawnPointHead = spawnPoint;
            } else {
                g_GameNetSpawnPointTail->next = spawnPoint;
            }
            g_GameNetSpawnPointTail = spawnPoint;
            spawnPoint->next = 0;
            ++g_GameNetSpawnPointCount;

            GameNetReaderArray* const spawnValueArray = (GameNetReaderArray*)(spawnArray->nodes[index].value.ptr);
            spawnPoint->position.x = spawnValueArray->nodes[0].value.f32;
            spawnPoint->position.y = spawnValueArray->nodes[1].value.f32;
            spawnPoint->position.z = spawnValueArray->nodes[2].value.f32;
            spawnPoint->yawDegrees = spawnValueArray->nodes[3].value.f32;
        }
        zReader::Free(treeRoot);
    }

    zUtil_SaveGameState* const localSaveState = (zUtil_SaveGameState*)(g_GameStateOrMapTable);
    GameNetPlayerRow* const playerRow = g_GameNetPlayerRowList.AppendNewRow(1);
    playerRow->saveState = (GameNetPlayerSaveState*)(localSaveState);
    playerRow->playerKey = zNetworkGetLocalPlayerKey();
    zNetwork::GetPlayerNameByKey(playerRow->playerKey, playerRow->displayName, sizeof(playerRow->displayName));
    playerRow->playerColorIndex = zNetworkGetPlayerColorIndexByKey(playerRow->playerKey);

    if (playerRow->playerColorIndex <= 0) {
        if (zNetwork::IsHost() == 0) {
            playerRow->playerColorIndex = GameNet::WaitForLocalPlayerColorIndex(60);
        }
        if (playerRow->playerColorIndex <= 0) {
            zVideo_dd::FlipToGDIIfAttached();
            Briefing::StopAndShutdownThread(0);
            zSndSystem::Shutdown();
            zNetwork::ShutdownSessionRuntime();
            zVideo::ShutdownVideoSystem();
            char fatalErrorCaption[0x80];
            char fatalErrorMessage[0x80];
            strcpy(fatalErrorCaption, zLoc::GetMessageString(18));
            strcpy(fatalErrorMessage, zLoc::GetMessageString(26));
            printf("%s: %s\n", fatalErrorCaption, fatalErrorMessage);
            Sleep(1000);
            MessageBeep(MB_ICONHAND);
            MessageBoxA(g_RecoilApp_hWndMain, fatalErrorMessage, fatalErrorCaption, MB_ICONHAND);
            zSys::ExitProcessWithCleanup(2);
        }
    }

    if (zNetwork::IsHost() != 0) {
        const unsigned int styleColor = g_GameNetPlayerRowStyleColors_00RRGGBB[playerRow->playerColorIndex];
        playerRow->playerColorPackedRgb = styleColor;
        playerRow->hudWidget.textColor0 = styleColor;
        playerRow->hudWidget.textColor1 = styleColor;
        playerRow->hudWidget.textDirty = 1;
        playerRow->ApplyPlayerColorTint();
        if (g_HudSensorTracker.raceCheckpointMode == 0) {
            float runtimeTimerSec;
            memcpy(&runtimeTimerSec, &g_HudSensorTracker.runtimeTimerSec, sizeof(runtimeTimerSec));
            g_GameNetHostHudTimerInitFlag = 0;
            HudUiTimerPanel::SetSeconds(runtimeTimerSec, -1.0f);
            g_HudTimerPanelNetState.timerDirectionNeg = 1;
            g_HudTimerPanelNetState.statusBitsResendDeadline = 30.0f;
        }
    }

    g_HudTimerPanelNetState.timeWarningShown = 0;
    g_HudTimerPanelNetState.oneMinuteWarningShown = 0;
    GameNet::RefreshPlayerListMenu(playerRow);
    localSaveState->netPlayerRow = playerRow;
    GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(localSaveState, 1);
    if (g_HudSensorTracker.raceCheckpointMode != 0) {
        GameNet::ResetHudTimerPanelNetStateLongCountdown();
    }
    g_GameNetPkt06InitialSyncGate = 1;
    g_GameNetPkt06NextSendTimeSec = 0.0f;
}
} // namespace Net

namespace GameNet {
/**
 * Purpose: Pump pending DirectPlay messages until the local player receives a
 * positive color index or the wait budget expires.
 */
int __fastcall WaitForLocalPlayerColorIndex(int maxWaitSeconds)
{
    int waitedSeconds = 0;
    while (waitedSeconds < maxWaitSeconds) {
        zNetworkDPlay::ReceivePendingMessages(-1);

        const int colorIndex = zNetworkGetLocalPlayerColorIndex();
        if (colorIndex > 0) {
            return colorIndex;
        }

        Sleep(1000);
        ++waitedSeconds;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-reset-remote-players-and-spawn-lists
 * @recoil-artifact defines .text recoil:function:0x4320f0: GameNet::ResetRemotePlayersAndSpawnLists.
 * @recoil-match byte
 *
 * Purpose: Clear remote player HUD rows and network spawn-point lists.
 */
void __cdecl ResetRemotePlayersAndSpawnLists()
{
    GameNetPlayerRow* row = g_GameNetPlayerRowHead;
    while (row != 0) {
        HudUi::RemoveScoreboardEntryRow(row);
        g_HudUiTopMessageStack->RemoveChild((HudUiElement*)(&row->hudWidget));
        row = row != 0 ? row->next : 0;
    }

    GameNetSpawnPoint* spawnPoint = g_GameNetSpawnPointHead;
    while (spawnPoint != 0) {
        GameNetSpawnPoint* const next = spawnPoint != 0 ? spawnPoint->next : 0;
        ::operator delete(spawnPoint);
        spawnPoint = next;
    }

    g_GameNetSpawnPointList.flags = 0;
    g_GameNetSpawnPointTail = 0;
    g_GameNetSpawnPointHead = 0;
    g_GameNetSpawnPointCount = 0;

    row = g_GameNetPlayerRowHead;
    while (row != 0) {
        GameNetPlayerRow* const next = row != 0 ? row->next : 0;
        if (row != 0) {
            row->DestroyEmbeddedPanel();
            ::operator delete(row);
        }
        row = next;
    }

    g_GameNetPlayerRowList.flags = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowCount = 0;
}
} // namespace GameNet

namespace GameNet {
/**
 * Purpose: Remove all gameplay packet handlers registered with zNetwork.
 */
void __cdecl UnregisterGameplayPacketHandlers()
{
    zNetwork::UnregisterPacketHandler(6, (zNetworkPacketHandler)&HandlePkt06PlayerStateSnapshot);
    zNetwork::UnregisterPacketHandler(7, (zNetworkPacketHandler)&HandlePkt07_AltGunDispatch);
    zNetwork::UnregisterPacketHandler(0x0a, (zNetworkPacketHandler)&OptCatalog::HandlePkt0ARemoveRuntimeRelay);
    zNetwork::UnregisterPacketHandler(1, (zNetworkPacketHandler)&ReassignPlayerColorsAndRefreshRows);
    zNetwork::UnregisterPacketHandler(8, (zNetworkPacketHandler)&HandlePkt08PlayerKillEvent);
    zNetwork::UnregisterPacketHandler(9, (zNetworkPacketHandler)&HandlePkt09PlayerScoreboardSnapshot);
    zNetwork::UnregisterPacketHandler(0x0b, (zNetworkPacketHandler)&HandlePkt0BChatMessage);
    zNetwork::UnregisterPacketHandler(0x0e, (zNetworkPacketHandler)&HandlePkt0EPlayerLapProgress);
    zNetwork::UnregisterPacketHandler(0x0c, (zNetworkPacketHandler)&HandlePkt0CHudTimerStatusBits);
    zNetwork::UnregisterPacketHandler(0x0d, (zNetworkPacketHandler)&HandlePkt0DHudTimerPanelState);
    zNetwork::UnregisterPacketHandler(0x0f, (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback);
    zNetwork::UnregisterPacketHandler(0x10, (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback);
    zNetwork::UnregisterPacketHandler(0x11, (zNetworkPacketHandler)&Pickup::HandlePkt11SpawnDelta);
    zNetwork::UnregisterPacketHandler(0x12, (zNetworkPacketHandler)&Pickup::HandlePkt12AirdropSpawnChuteRelay);
    zNetwork::UnregisterPacketHandler(0x13, (zNetworkPacketHandler)&HandlePkt13EffectAnimActivationRecord);
    g_GameNet_HandlersRegistered = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-reset-hud-timer-panel-net-state-long-countdown
 * @recoil-artifact defines .text recoil:function:0x4322a0: GameNet::ResetHudTimerPanelNetStateLongCountdown.
 * @recoil-match byte
 *
 * Purpose: Reset the replicated HUD timer state to the long race countdown
 * defaults and update the displayed timer panel.
 */
void __cdecl ResetHudTimerPanelNetStateLongCountdown()
{
    g_HudTimerPanelNetState.timerSeconds = 36000.0f;
    HudUiTimerPanel::SetSeconds(36000.0f, -1.0f);
    g_HudTimerPanelNetState.startCountdownTriggered = 0;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 0;
    g_HudTimerPanelNetState.timeWarningThresholdSec = 120.0f;
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.startGateTriggered = 0;
    g_HudTimerPanelNetState.raceFinishCountdownTriggered = 0;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    g_GameNetOneLapLeftMessageShown = 0;
    memset(g_HudTimerPanelNetState.tailFlags, 0, sizeof(g_HudTimerPanelNetState.tailFlags));
}

/**
 * Purpose: Replicate the local pkt06 player-state snapshot and drive host HUD
 * timer warning/status packet updates.
 */
int __fastcall TickLocalPlayerPkt06ReplicationAndHudTimer(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerModalState* const primaryModalState = saveState->primaryModalState;

    if (zOpt::GetNetworkEnabled() == 0) {
        return 0;
    }

    if (zNetwork::IsHost() == 0 && g_GameNetPkt06InitialSyncGate != 0) {
        return 0;
    }

    g_GameNetPkt06InputBit16Latch |= playerState->netInputBit16Latch;
    g_GameNetPkt06InputBit17Latch |= playerState->netInputBit17Latch;
    if (g_Time_AccumulatedTimeSec < g_GameNetPkt06NextSendTimeSec) {
        return 1;
    }

    g_GameNetPkt06NextSendTimeSec = g_Time_AccumulatedTimeSec + 0.1f;

    NetPkt06_PlayerStateSnapshot* const packet = &g_NetPkt06_PlayerStateSnapshotBuf;
    packet->header.packetSizeBytes = 0x44;
    packet->header.packetType = 0x06;
    packet->header.payloadDword0 = zNetworkGetLocalPlayerKey();
    packet->cachedAltSelectionCode = (short)(playerState->cachedAltSelectionCode);
    packet->cachedPrimarySelectionCode = (short)(playerState->cachedPrimarySelectionCode);
    packet->masterType = primaryModalState->masterModalData->masterType;
    packet->colorIndex = GetLocalPlayerColorIndexOrZero();
    packet->inputBit16 = g_GameNetPkt06InputBit16Latch;
    g_GameNetPkt06InputBit16Latch = 0;
    packet->inputBit17 = g_GameNetPkt06InputBit17Latch;
    g_GameNetPkt06InputBit17Latch = 0;

    packet->altGunAimOrigin = playerState->altGunAimOrigin;
    packet->storedTargetPos = playerState->storedTargetPos;
    packet->worldPos = playerState->worldPos;
    packet->vehicleRotationAngles = playerState->vehicleRotationAngles;
    packet->statusMeterValue = playerState->statusMeterValue;

    if (playerState->progressTargetCount > 0) {
        packet->hasProgressTargets = 1;
        packet->header.packetSizeBytes += (short)(sizeof(int) + playerState->progressTargetCount * sizeof(zVec3));
        packet->progressTargetCount = playerState->progressTargetCount;
        for (int progressIndex = 0; progressIndex < playerState->progressTargetCount; ++progressIndex) {
            const zVec3* const targetPos = playerState->progressTargetSlots[progressIndex].targetPos;
            packet->progressTargetPoints[progressIndex] = *targetPos;
        }
    } else {
        packet->hasProgressTargets = 0;
    }

    const int sendResult = zNetworkSendPacketUnreliable(&packet->header);
    if (zNetwork::IsHost() != 0) {
        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
            timerState.timerSeconds = HudUiTimerPanel::GetSeconds();
            timerState.startCountdownTriggered = 0;

            if (timerState.startGateTriggered == 0) {
                if (timerState.timerSeconds <= g_FrameDeltaTimeSec) {
                    timerState.startGateTriggered = 1;
                    timerState.timerDirectionNeg = 0;
                    timerState.timerSeconds = 0.0f;

                    /**
                     * GameNet startgate effect literal.
                     * Data owner gate remains pending; this docblock records
                     * source provenance only.
                     * Purpose: name the replicated start-gate effect animation
                     * stopped when the host race countdown reaches zero.
                     */
                    zEffectAnimEntry* const startGateEntry = zEffectAnim::FindEntryByName("startgate");
                    zEffectAnim::SetVelocityThunk(startGateEntry, 0, 0.0f, 0.0f, 0.0f);
                    SendPkt0DHudTimerPanelState(&timerState);
                } else if (g_HudTimerPanelNetState.startCountdownTriggered == 0
                    && g_HudTimerPanelNetState.tenSecondWarningsEnabled != 0) {
                    timerState.timerSeconds = 10.0f;
                    HudUiTimerPanel::SetSeconds(g_FrameDeltaTimeSec + 10.0f, -1.0f);
                    timerState.startCountdownTriggered = 1;
                    SendPkt0DHudTimerPanelState(&timerState);
                } else if (timerState.timerSeconds > 10.0f && (int)(timerState.timerSeconds) % 10 == 0) {
                    if (g_GameNetHudTimerTenSecondWarningArmed != 0) {
                        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x32), 5.0f);
                        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x31), 5.0f);
                        g_GameNetHudTimerTenSecondWarningArmed = 0;
                    }
                } else {
                    g_GameNetHudTimerTenSecondWarningArmed = 1;
                }
            }

            if (timerState.raceFinishCountdownTriggered != 0 && timerState.timeWarningShown == 0) {
                timerState.timeWarningShown = 1;
                SendPkt0DHudTimerPanelState(&timerState);
            }
        } else {
            const float timerSeconds = g_HudTimerPanelNetState.timerSeconds = HudUiTimerPanel::GetSeconds();
            HudTimerPanelNetState timerState = g_HudTimerPanelNetState;

            if (timerState.oneMinuteWarningShown == 0 && timerSeconds < g_FrameDeltaTimeSec + 60.0f) {
                timerState.oneMinuteWarningShown = 1;
                SendPkt0CHudTimerStatusBits(&timerState);
            }

            if (g_HudTimerPanelNetState.timeWarningShown == 0 && timerSeconds < g_FrameDeltaTimeSec) {
                timerState.timeWarningShown = 1;
                SendPkt0CHudTimerStatusBits(&timerState);
            }

            if (g_Time_AccumulatedTimeSec > g_HudTimerPanelNetState.statusBitsResendDeadline) {
                SendPkt0CHudTimerStatusBits(&timerState);
            }
        }
    } else if (g_HudSensorTracker.raceCheckpointMode != 0) {
        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (timerState.startGateTriggered != 0 || timerState.startCountdownTriggered != 0) {
            g_GameNetHudTimerPendingSaveReminderArmed = 1;
        } else if ((int)(HudUiTimerPanel::GetSeconds()) % 10 != 0) {
            g_GameNetHudTimerPendingSaveReminderArmed = 1;
        } else if (g_GameNetHudTimerPendingSaveReminderArmed != 0) {
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x34), 5.0f);
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x33), 5.0f);
            g_GameNetHudTimerPendingSaveReminderArmed = 0;
        }
    }

    return sendResult;
}

/**
 * Purpose: Dispatch an incoming player-state snapshot to row creation or
 * existing-row update handling.
 */
int __fastcall HandlePkt06PlayerStateSnapshot(int senderPlayerId, NetPkt06_PlayerStateSnapshot* packet)
{
    if (packet == 0) {
        return -1;
    }

    GameNetPlayerRow* const row = FindPlayerRowByKey(packet->header.payloadDword0);
    if (g_GameNetPkt06InitialSyncGate != 0) {
        g_GameNetPkt06InitialSyncGate = 0;
    }

    if (packet->header.packetType == 6) {
        if (row == 0) {
            SpawnRemotePlayerFromPkt06PlayerStateSnapshot(senderPlayerId, packet);
            return 0;
        }

        ApplyPkt06PlayerStateSnapshotToRow(row, packet);
    }

    return 0;
}

/**
 * Purpose: Find the active GameNet remote-player row for a network player key.
 */
GameNetPlayerRow* __fastcall FindPlayerRowByKey(int playerKey)
{
    GameNetPlayerRow* row = g_GameNetPlayerRowHead;
    while (row != 0) {
        if (row->playerKey == playerKey) {
            return row;
        }

        row = row != 0 ? row->next : 0;
    }

    return 0;
}

/**
 * Purpose: Create a remote player row and cloned player node from an incoming
 * player-state snapshot packet.
 */
int __fastcall SpawnRemotePlayerFromPkt06PlayerStateSnapshot(int senderPlayerId, NetPkt06_PlayerStateSnapshot* packet)
{
    char displayNameScratch[0x40];
    if (zNetwork::GetPlayerNameByKey(senderPlayerId, displayNameScratch, sizeof(displayNameScratch)) == 0) {
        zNetwork_DPlay::EnumPlayers();
    }

    CZNodePartial* clonedNode = CZClass::FindByTypeAndName(6, "bft_99");
    if (clonedNode != 0) {
        clonedNode = CZUtil::CopyNodeWithCloneOptions(clonedNode, 1, 1);
    }

    if (clonedNode == 0) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x911), 5.0f);
        return 0;
    }

    clonedNode->flags |= ::kGameNetRemoteCloneNodeFlag;

    char netNodeName[0x14];
    sprintf(netNodeName, "net%d", packet->header.payloadDword0);
    CZClass::gwNodeSetName(clonedNode, netNodeName);

    zUtil_SaveGameState* const saveState = Player::CreateFromNamesAtPoseGetState(
        &packet->worldPos,
        g_Player_NodeName_Bft,
        packet->vehicleRotationAngles.y,
        netNodeName
    );
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    playerState->lifecycleState = 3;
    playerState->amphibUnlocked = 1;
    playerState->hoverUnlocked = 1;
    playerState->subUnlocked = 1;
    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank& bank = playerState->altWeaponBanks[bankIndex];
        bank.controllerA.flags |= 4u;
        bank.controllerA.ammoOrCharge = 123456792.0f;
        bank.controllerB.flags |= 4u;
        bank.controllerB.ammoOrCharge = 123456792.0f;
    }

    GameNetPlayerRowListState* const rowList = &g_GameNetPlayerRowList;
    GameNetPlayerRow* const row = rowList->AppendNewRow(0);
    row->playerKey = packet->header.payloadDword0;
    row->playerColorIndex = packet->colorIndex;
    row->playerNode = clonedNode;
    row->score = 0;
    row->lapCount = 0;
    row->turretNode = CZClass::FindSubNodeByName(clonedNode, g_Player_NodeName_Turret);
    row->gunNode = CZClass::FindSubNodeByName(clonedNode, "gun");
    row->saveState = (GameNetPlayerSaveState*)saveState;

    if (zNetwork::GetPlayerNameByKey(senderPlayerId, row->displayName, sizeof(row->displayName)) != 0) {
        HudUi::ShowTopMessageLine(row->displayName, 5.0f);
    }
    HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x912), 5.0f);

    HudUiPanel* const hudWidget = &row->hudWidget;
    hudWidget->SetText(row->displayName);
    const unsigned int hudColor = g_GameNetPlayerRowStyleColors_00RRGGBB[0];
    hudWidget->textColor0 = hudColor;
    hudWidget->textColor1 = hudColor;
    hudWidget->textDirty = 1;
    hudWidget->SetVisible(0);
    g_HudUiTopMessageStack->AddChild((HudUiElement*)(hudWidget));

    saveState->netPlayerRow = row;
    if (row->playerNode->listCountA == 0) {
        CZClass::AddChild(g_Player_RuntimeDiScene, row->playerNode);
    }
    CZClass::gwNodeSetActive(row->playerNode, 1);

    RefreshPlayerListMenu(row);
    ReassignPlayerColorsAndRefreshRows(0, 0);

    if (zNetwork::IsHost() != 0) {
        SendPkt09PlayerScoreboardSnapshot();
        zDEClient::DispatchFeatureEventTemplates(HostSendPkt0FCraterFeature, HostSendPkt10QSandFeature);
        SendAllPkt13EffectAnimActivationRecords();
        Pickup::ReconcilePrimaryAndNetworkCopySpawnLists();

        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            timerState.startCountdownTriggered = 0;
            if (g_HudTimerPanelNetState.startGateTriggered != 0) {
                timerState.ClearTailFlagsLocal();
            }
            SendPkt0DHudTimerPanelState(&timerState);
        } else {
            SendPkt0CHudTimerStatusBits(&timerState);
        }
    }

    ApplyPkt06PlayerStateSnapshotToRow(row, packet);
    return 1;
}

/**
 * Purpose: Apply a replicated player-state snapshot packet to an existing
 * remote-player row and its save-state storage.
 */
int __fastcall ApplyPkt06PlayerStateSnapshotToRow(GameNetPlayerRow* row, NetPkt06_PlayerStateSnapshot* packet)
{
    GameNetPlayerSaveState* const rowSaveState = row->saveState;
    zUtil_SaveGameState* const saveState = (zUtil_SaveGameState*)rowSaveState;
    zUtil_PlayerStateStorage* const playerState = rowSaveState->playerState;
    PlayerMasterModalData* const masterModalData = rowSaveState->primaryModalState->masterModalData;

    playerState->netUpdateReceived = 1;

    if (row->playerColorIndex != packet->colorIndex) {
        row->playerColorIndex = packet->colorIndex;
        const unsigned int packedColor = g_GameNetPlayerRowStyleColors_00RRGGBB[row->playerColorIndex];
        row->playerColorPackedRgb = packedColor;
        row->hudWidget.SetTextColorsAndMarkDirty(packedColor, packedColor);
        HudUi::RefreshScoreboardEntryRow(row);
        row->ApplyPlayerColorTint();
    }

    const int masterType = packet->masterType;
    if (masterType != masterModalData->masterType) {
        Player::ApplyMasterTypeTransition(saveState, masterType, 0);
    }

    playerState->netReceivedPos = packet->worldPos;
    playerState->netReceivedAngles = packet->vehicleRotationAngles;

    if (playerState->netLastUpdateFrameTick == g_zVideo_FrameTick) {
        playerState->netInputBit16Latch |= packet->inputBit16;
        playerState->netInputBit17Latch |= packet->inputBit17;
    } else {
        playerState->netInputBit16Latch = packet->inputBit16;
        playerState->netInputBit17Latch = packet->inputBit17;
        playerState->netLastUpdateFrameTick = g_zVideo_FrameTick;
    }

    playerState->storedTargetPos = packet->storedTargetPos;

    const int altSelectionCode = (int)((unsigned short)(packet->cachedAltSelectionCode));
    if (altSelectionCode != playerState->cachedAltSelectionCode) {
        Player::ApplyAltWeaponSwitch(
            saveState,
            playerState->activeAltGunController,
            (&playerState->altWeaponBanks[altSelectionCode / 100].controllerA) + (altSelectionCode % 100)
        );
    }

    const int primarySelectionCode = (int)((unsigned short)(packet->cachedPrimarySelectionCode));
    if (primarySelectionCode != playerState->cachedPrimarySelectionCode) {
        Player::ApplyPrimaryWeaponSwitch(
            saveState,
            playerState->activePrimaryGunController,
            (&playerState->altWeaponBanks[primarySelectionCode / 100].controllerA) + (primarySelectionCode % 100)
        );
    }

    playerState->statusMeterValue = packet->statusMeterValue;
    for (int index = 0; index < 10; ++index) {
        playerState->progressTargetRuntimeSlots[index].targetPos = 0;
    }

    if (packet->hasProgressTargets == 0) {
        playerState->progressTargetCount = 0;
        return 1;
    }

    playerState->progressTargetCount = packet->progressTargetCount;
    for (int progressIndex = 0; progressIndex < playerState->progressTargetCount; ++progressIndex) {
        playerState->progressTargetPointStorage[progressIndex] = packet->progressTargetPoints[progressIndex];
        playerState->progressTargetRuntimeSlots[progressIndex].targetPos
            = &playerState->progressTargetPointStorage[progressIndex];
    }

    return 1;
}

/**
 * Purpose: Project a remote player name-tag HUD widget into screen space.
 */
int __fastcall UpdateRemotePlayerHudWidgetScreenPos(zUtil_SaveGameState* saveState)
{
    if (GetStatusBitNameTags() == 0) {
        return 0;
    }

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    HudUiPanel* const hudWidget = &saveState->netPlayerRow->hudWidget;
    zVec3 labelWorldPos = playerState->worldPos;
    labelWorldPos.y += 3.0f;

    if (AINet::HasLineOfSightFromLocalPlayerFxOffset(playerState->rootNode, &labelWorldPos, 1) == 0) {
        hudWidget->SetVisible(0);
        return 0;
    }

    zVec3 projectedPoint;
    const int clipped = zMath::ProjectPointAndClampToScreenClip(&labelWorldPos, &projectedPoint);
    int screenX;
    int screenY;
    if (zOpt::GetReplicateMode() != 0) {
        screenX = (int)(projectedPoint.x * 2.0f);
        screenY = (int)(projectedPoint.y * 2.0f);
    } else {
        screenX = (int)projectedPoint.x;
        screenY = (int)projectedPoint.y;
    }
    screenY -= 10;

    if (screenY <= hudWidget->QueryTextHeight() + 26) {
        hudWidget->SetVisible(0);
        return 0;
    }

    if (clipped != 0) {
        hudWidget->SetVisible(0);
        return 0;
    }

    hudWidget->SetPos(screenX, screenY);
    hudWidget->SetVisible(1);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-reassign-player-colors-and-refresh-rows
 * @recoil-artifact defines .text recoil:function:0x432e70: GameNet::ReassignPlayerColorsAndRefreshRows.
 * @recoil-match byte
 *
 * Purpose: Refresh player-row colors after network color assignment changes.
 */
int __cdecl ReassignPlayerColorsAndRefreshRows(int, zNetworkPacketHeader*)
{
    GameNetPlayerRow* row = g_GameNetPlayerRowHead;
    while (row != 0) {
        const int colorIndex = zNetworkGetPlayerColorIndexByKey(row->playerKey);
        row->playerColorIndex = colorIndex;

        const unsigned int color = g_GameNetPlayerRowStyleColors_00RRGGBB[colorIndex];
        row->playerColorPackedRgb = color;
        row->hudWidget.SetTextColorsAndMarkDirty(color, color);
        HudUi::RefreshScoreboardEntryRow(row);
        row->ApplyPlayerColorTint();

        row = row != 0 ? row->next : 0;
    }

    return 1;
}

/**
 * Purpose: Handle the remote-player remove packet by retiring the player's
 * runtime state, unlinking the HUD row, and deleting the player row.
 */
int __fastcall HandlePkt03RemoveRemotePlayer(int senderPlayerId, zNetworkPacketHeader*)
{
    GameNetPlayerRow* const row = FindPlayerRowByKey(senderPlayerId);
    if (row == 0) {
        return 0;
    }

    zUtil_SaveGameState* const saveState = (zUtil_SaveGameState*)row->saveState;
    if (saveState != 0) {
        zUtil_PlayerStateStorage* const playerState = saveState->playerState;
        playerState->cameraTransitionTimer = 1;
        playerState->lifecycleState = 4;
        Player::ResetAltGunRuntimeState(saveState);
        Player::RemoveAllDeployedMines(saveState);
    }

    char message[0x80];
    zLoc::FormatMessage(message, sizeof(message), 0x913, row->displayName);
    HudUi::ShowTopMessageLine(message, 5.0f);
    HudUi::RemoveScoreboardEntryRow(row);
    HudUiPanel* const hudWidget = &row->hudWidget;
    hudWidget->SetVisible(0);
    g_HudUiTopMessageStack->RemoveChild((HudUiElement*)(hudWidget));

    if (g_GameNetPlayerRowCount == 0) {
        return 0;
    }

    if (row == g_GameNetPlayerRowHead) {
        --g_GameNetPlayerRowCount;
        GameNetPlayerRow* const next = row->next;
        g_GameNetPlayerRowHead = next;
        if (next == 0) {
            g_GameNetPlayerRowList.flags = 0;
            g_GameNetPlayerRowTail = 0;
        }
    } else {
        GameNetPlayerRow* previous = g_GameNetPlayerRowHead;
        while (previous != 0 && previous->next != row) {
            previous = previous->next;
        }

        if (previous == 0) {
            return 0;
        }

        --g_GameNetPlayerRowCount;
        previous->next = row->next;
        if (g_GameNetPlayerRowTail == row) {
            g_GameNetPlayerRowTail = previous;
        }
    }

    row->DestroyEmbeddedPanel();
    ::operator delete(row);
    return 0;
}

/**
 * Purpose: Build, send, and locally dispatch a packet-08 player kill event.
 */
void __fastcall SendPkt08PlayerKillEvent(zUtil_SaveGameState* saveState, short killMethodOrOptCatalogEntryId)
{
    zUtil_SaveGameState* saveStateOrLocal = saveState;
    if (saveStateOrLocal == 0) {
        saveStateOrLocal = (zUtil_SaveGameState*)(g_GameStateOrMapTable);
    }

    NetPkt08_PlayerKillEvent packet;
    packet.header.packetType = 0x08;
    packet.header.packetSizeBytes = sizeof(NetPkt08_PlayerKillEvent);
    packet.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    packet.killMethodOrOptCatalogEntryId = killMethodOrOptCatalogEntryId;
    packet.targetPlayerKey = saveStateOrLocal->netPlayerRow->playerKey;

    zNetworkSendPacketReliable(&packet.header);
    HandlePkt08PlayerKillEvent(zNetworkGetLocalPlayerKey(), &packet);
}

/**
 * Purpose: Apply an incoming packet-08 player kill event and host-side
 * scoreboard update.
 */
int __fastcall HandlePkt08PlayerKillEvent(int localPlayerKey, NetPkt08_PlayerKillEvent* packet)
{
    GameNetPlayerRow* const killerRow = FindPlayerRowByKey(localPlayerKey);
    GameNetPlayerRow* const victimRow = FindPlayerRowByKey(packet->targetPlayerKey);
    if (killerRow == 0 || victimRow == 0) {
        return 0;
    }

    const short killEntryId = packet->killMethodOrOptCatalogEntryId;
    if (killEntryId != 0) {
        OptCatalogEntryDef* const killEntry = OptCatalog::FindEntryById(killEntryId);
        ShowPlayerKillMessage(victimRow, killEntry, killerRow);
    } else {
        ShowPlayerKillMessage(victimRow, 0, killerRow);
    }

    if (zNetwork::IsHost() != 0) {
        if (victimRow != killerRow) {
            ++victimRow->score;
        } else {
            if (--victimRow->score < 0) {
                victimRow->score = 0;
            }
        }

        SendPkt09PlayerScoreboardSnapshot();
    }

    return 1;
}

/**
 * Purpose: Publish the local player's packed lap count and lap time packet.
 */
void __fastcall SendPkt0EPlayerLapProgress(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    NetPkt0E_PlayerLapProgress packet;
    packet.header.packetType = 0x0e;
    packet.header.packetSizeBytes = sizeof(NetPkt0E_PlayerLapProgress);
    packet.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    packet.lapCountPacked = (short)(playerState->lapCount);
    packet.lapTimeSec = playerState->lapTimeSec;

    if (zNetwork::IsHost() != 0) {
        GameNetPlayerRow* const row = saveState->netPlayerRow;
        row->lapCount = playerState->lapCount;
        row->lapTimeSec = playerState->lapTimeSec;
        HandlePkt0EPlayerLapProgress(packet.header.payloadDword0, &packet);
    } else {
        zNetworkSendPacketReliable(&packet.header);
    }
}

/**
 * Purpose: Apply a host-side player lap-progress packet and refresh race HUD
 * state when the lap target is reached.
 */
int __fastcall HandlePkt0EPlayerLapProgress(int senderPlayerId, NetPkt0E_PlayerLapProgress* packet)
{
    int result = zNetwork::IsHost();
    if (result == 0) {
        return result;
    }

    GameNetPlayerRow* const row = FindPlayerRowByKey(senderPlayerId);
    if (row == 0) {
        return 0;
    }

    row->lapCount = packet->lapCountPacked;
    row->lapTimeSec = packet->lapTimeSec;
    SendPkt09PlayerScoreboardSnapshot();

    const int lapGoal = g_HudSensorTracker.runtimeGoalValue;
    if (row->lapCount >= lapGoal) {
        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (AreAllPlayersAtLapTarget() != 0) {
            timerState.timeWarningShown = 1;
        }

        timerState.raceFinishCountdownTriggered = 1;
        SendPkt0DHudTimerPanelState(&timerState);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-are-all-players-at-lap-target
 * @recoil-artifact defines .text recoil:function:0x433200: GameNet::AreAllPlayersAtLapTarget.
 * @recoil-match byte
 *
 * Purpose: Mark the multiplayer lap-target check as started and report
 * whether every player row has reached the race goal.
 */
int __cdecl AreAllPlayersAtLapTarget()
{
    if (g_GameNetAllPlayersLapTargetCheckStarted == 0) {
        g_GameNetAllPlayersLapTargetCheckStarted = 1;
    }

    GameNetPlayerRow* row = g_GameNetPlayerRowHead;
    while (row != 0) {
        if (row->lapCount < g_HudSensorTracker.runtimeGoalValue) {
            return 0;
        }

        row = row != 0 ? row->next : 0;
    }

    return 1;
}

/**
 * Purpose: Apply the host HUD timer panel state packet to local timer state.
 */
int __fastcall HandlePkt0DHudTimerPanelState(int, NetPkt0D_HudTimerPanelState* packet)
{
    const int statusBits = packet->hudTimerFlagsPacked;
    g_HudTimerPanelNetState.timerSeconds = packet->seconds;
    g_HudTimerPanelNetState.timerDirectionNeg = statusBits & 1;

    float secondsStep = -1.0f;
    if (g_HudTimerPanelNetState.timerDirectionNeg == 0) {
        secondsStep = 1.0f;
    }

    HudUiTimerPanel::SetSeconds(g_HudTimerPanelNetState.timerSeconds, secondsStep);

    if (g_HudTimerPanelNetState.startGateTriggered == 0 && (statusBits & 8) != 0) {
        g_HudTimerPanelNetState.startGateTriggered = 1;
    }

    if ((statusBits & 0x20) != 0) {
        /* Retail literal 0x4dcfd4 names the replicated start-countdown
           effect animation; data ownership remains blocked outside this
           source slice. */
        zEffectAnimEntry* const startCountdown = zEffectAnim::FindEntryByName("startcountdown");
        zEffectAnim::SetVelocityThunk(startCountdown, 0, 0.0f, 0.0f, 0.0f);
        g_HudTimerPanelNetState.startCountdownTriggered = 1;
    }

    if (g_HudTimerPanelNetState.raceFinishCountdownTriggered == 0 && (statusBits & 0x10) != 0) {
        g_HudTimerPanelNetState.raceFinishCountdownTriggered = 1;
    }

    g_HudTimerPanelNetState.timeWarningShown = statusBits & 2;
    if (g_HudTimerPanelNetState.timeWarningShown != 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mpExitDialogState, 0);
    }

    return 1;
}

/**
 * Purpose: Send and locally apply the host HUD timer panel state packet.
 */
void __fastcall SendPkt0DHudTimerPanelState(HudTimerPanelNetState* timerState)
{
    if (zNetwork::IsHost() == 0) {
        return;
    }

    g_NetPkt0D_HudTimerPanelStateBuf.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    g_NetPkt0D_HudTimerPanelStateBuf.seconds = HudUiTimerPanel::GetSeconds();

    short statusBits = 0;
    if (timerState->timerDirectionNeg != 0) {
        statusBits = 1;
    }
    if (timerState->startGateTriggered != 0) {
        statusBits |= 8;
    }
    if (timerState->timeWarningShown != 0) {
        statusBits |= 2;
    }
    if (timerState->raceFinishCountdownTriggered != 0) {
        statusBits |= 0x10;
    }
    if (timerState->startCountdownTriggered != 0) {
        statusBits |= 0x20;
    }

    g_NetPkt0D_HudTimerPanelStateBuf.hudTimerFlagsPacked = statusBits;
    zNetworkSendPacketReliable(&g_NetPkt0D_HudTimerPanelStateBuf.header);
    HandlePkt0DHudTimerPanelState(
        g_NetPkt0D_HudTimerPanelStateBuf.header.payloadDword0,
        &g_NetPkt0D_HudTimerPanelStateBuf
    );
}

/**
 * Purpose: Send and locally apply replicated HUD timer status bits.
 */
int __fastcall SendPkt0CHudTimerStatusBits(HudTimerPanelNetState* timerState)
{
    int result = zNetwork::IsHost();
    if (result != 0) {
        g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0 = zNetworkGetLocalPlayerKey();
        g_NetPkt0C_HudTimerStatusBitsBuf.timerSeconds = HudUiTimerPanel::GetSeconds();

        short statusBits = 0;
        if (timerState->timerDirectionNeg != 0) {
            statusBits = 1;
        }
        if (timerState->timeWarningShown != 0) {
            statusBits |= 2;
        }
        if (timerState->oneMinuteWarningShown != 0) {
            statusBits |= 4;
        }

        g_NetPkt0C_HudTimerStatusBitsBuf.statusBitsPackedHiWord = statusBits;
        g_HudTimerPanelNetState.statusBitsResendDeadline = g_Time_AccumulatedTimeSec + 30.0f;
        zNetworkSendPacketReliable(&g_NetPkt0C_HudTimerStatusBitsBuf.header);
        result = HandlePkt0CHudTimerStatusBits(
            g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0,
            &g_NetPkt0C_HudTimerStatusBitsBuf
        );
    }

    return result;
}

/**
 * Purpose: Apply replicated HUD timer seconds and warning status bits.
 */
int __fastcall HandlePkt0CHudTimerStatusBits(int, NetPkt0C_HudTimerStatusBits* packet)
{
    const int statusBits = packet->statusBitsPackedHiWord;
    g_HudTimerPanelNetState.timerSeconds = packet->timerSeconds;
    g_HudTimerPanelNetState.timerDirectionNeg = statusBits & 1;

    float secondsStep = -1.0f;
    if (g_HudTimerPanelNetState.timerDirectionNeg == 0) {
        secondsStep = 1.0f;
    }

    HudUiTimerPanel::SetSeconds(g_HudTimerPanelNetState.timerSeconds, secondsStep);

    if (g_HudSensorTracker.raceCheckpointMode == 0 && g_HudTimerPanelNetState.oneMinuteWarningShown == 0
        && (statusBits & 4) != 0) {
        g_HudTimerPanelNetState.oneMinuteWarningShown = 1;
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x914), 5.0f);
        if (zNetwork::IsHost() != 0) {
            HostUpdateSessionDescStatusFields(0x100, 0, 0, 0);
        }
    }

    if (g_HudTimerPanelNetState.timeWarningShown == 0 && (statusBits & 2) != 0) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x915), 5.0f);
        g_HudTimerPanelNetState.timeWarningShown = 1;
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mpExitDialogState, 0);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.gamenet-send-pkt09-player-scoreboard-snapshot
 * @recoil-artifact defines .text recoil:function:0x4334f0: GameNet::SendPkt09PlayerScoreboardSnapshot.
 * @recoil-match byte
 *
 * Purpose: Send the host's packed player score and lap snapshot to peers.
 */
void __cdecl SendPkt09PlayerScoreboardSnapshot()
{
    if (zNetwork::IsHost() == 0) {
        return;
    }

    const int entryCount = (int)(g_GameNetPlayerRowCount);
    const size_t packetSize
        = sizeof(zNetworkPacketHeader) + sizeof(int) + (size_t)(entryCount) * sizeof(NetPkt09_PlayerScoreboardEntry);
    NetPkt09_PlayerScoreboardSnapshot* const packet = (NetPkt09_PlayerScoreboardSnapshot*)(malloc(packetSize));
    memset(packet, 0, packetSize);

    packet->header.packetType = 0x09;
    packet->header.packetSizeBytes = (unsigned short)(packetSize);
    packet->header.payloadDword0 = zNetworkGetLocalPlayerKey();
    packet->entryCount = entryCount;

    GameNetPlayerRow* row = g_GameNetPlayerRowHead;
    NetPkt09_PlayerScoreboardEntry* entry = packet->entries;
    while (row != 0) {
        entry->playerKey = row->playerKey;
        entry->packedScoreAndLapCount = (unsigned short)((row->lapCount << 9) + (row->score & 0x1ff));
        row = row != 0 ? row->next : 0;
        ++entry;
    }

    zNetworkSendPacketReliable(&packet->header);
    if (zNetwork::IsHost() != 0) {
        HandlePkt09PlayerScoreboardSnapshot(zNetworkGetLocalPlayerKey(), packet);
    }

    free(packet);
}

/**
 * Purpose: Apply packed player score and lap rows and trigger HUD warnings.
 */
int __fastcall HandlePkt09PlayerScoreboardSnapshot(int, NetPkt09_PlayerScoreboardSnapshot* packet)
{
    GameNetPlayerRow* oneLapLeftRow = 0;
    const int entryCount = packet->entryCount;

    {
        for (int index = 0; index < entryCount; ++index) {
            NetPkt09_PlayerScoreboardEntry* const entry = &packet->entries[index];
            GameNetPlayerRow* const row = FindPlayerRowByKey(entry->playerKey);
            if (row == 0) {
                continue;
            }

            row->score = entry->packedScoreAndLapCount & 0x1ff;
            row->lapCount = ((short)(entry->packedScoreAndLapCount) >> 9) & 0x7f;
            HudUi::RefreshScoreboardEntryRow(row);

            if (g_GameNetOneLapLeftMessageShown == 0 && oneLapLeftRow == 0) {
                const int lapsGoal = g_HudSensorTracker.runtimeGoalValue;
                if (row->lapCount == lapsGoal - 1) {
                    oneLapLeftRow = row;
                }
            }

            if (zNetwork::IsHost() != 0 && g_HudSensorTracker.raceCheckpointMode == 0) {
                const int scoreGoal = g_HudSensorTracker.runtimeGoalValue;
                if (row->score >= scoreGoal) {
                    HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
                    if (timerState.timeWarningShown == 0) {
                        timerState.timeWarningShown = 1;
                        SendPkt0CHudTimerStatusBits(&timerState);
                    }
                }
            }
        }
    }

    if (g_HudSensorTracker.raceCheckpointMode != 0 && g_GameNetOneLapLeftMessageShown == 0 && oneLapLeftRow != 0) {
        char message[0x80];
        zLoc::FormatMessage(message, sizeof(message), 0x918, oneLapLeftRow->displayName);
        HudUi::ShowTopMessageLine(message, 5.0f);
        g_GameNetOneLapLeftMessageShown = 1;
        if (zNetwork::IsHost() != 0) {
            HostUpdateSessionDescStatusFields(0x100, 0, 0, 0);
        }
    }

    return 1;
}

/**
 * Purpose: Return the local GameNet player-row color index when the local
 * save-state row is available, or zero otherwise.
 */
int __cdecl GetLocalPlayerColorIndexOrZero()
{
    zUtil_SaveGameState* const saveState = (zUtil_SaveGameState*)(g_GameStateOrMapTable);
    if (saveState == 0) {
        return 0;
    }

    GameNetPlayerRow* const netPlayerRow = saveState->netPlayerRow;
    if (netPlayerRow == 0) {
        return 0;
    }

    return netPlayerRow->playerColorIndex;
}

/**
 * Purpose: Decode host status flags into the cached allow-map and name-tag bits.
 */
void __fastcall SetStatusBitsFromFlags(unsigned int statusFlags)
{
    g_GameNetStatus_AllowMaps = statusFlags & 1u;
    g_GameNetStatus_NameTags = (statusFlags >> 1) & 1u;
}

/**
 * Purpose: Return the cached status bit controlling map availability.
 */
int __cdecl GetStatusBitAllowMaps()
{
    return g_GameNetStatus_AllowMaps;
}

/**
 * Purpose: Return the cached status bit controlling remote player name tags.
 */
int __cdecl GetStatusBitNameTags()
{
    return g_GameNetStatus_NameTags;
}

/**
 * Purpose: Build and send a packet-0B chat message for the local player.
 */
void __fastcall SendPkt0BChatMessage(const char* message)
{
    const int packetSize = (int)(strlen(message)) + 12;
    NetPkt0B_ChatMessage* const packet = (NetPkt0B_ChatMessage*)(malloc((size_t)(packetSize)));
    memset(packet, 0, (size_t)(packetSize));

    packet->header.packetType = 0x0b;
    packet->header.packetSizeBytes = (short)(packetSize);
    packet->header.payloadDword0 = zNetworkGetLocalPlayerKey();
    packet->messageLength = (short)(strlen(message));
    for (int i = 0; i < packet->messageLength; ++i) {
        packet->message[i] = message[i];
    }

    zNetworkSendPacketReliable(&packet->header);
    free(packet);
}

/**
 * Purpose: Copy an incoming chat payload into a bounded local string and show it.
 */
int __fastcall HandlePkt0BChatMessage(int, NetPkt0B_ChatMessage* packet)
{
    char message[0x51];
    memset(message, 0, sizeof(message));
    int messageLength = packet->messageLength < 0x50 ? packet->messageLength : 0x50;

    if (messageLength > 0) {
        memcpy(message, packet->message, (size_t)(messageLength));
    }

    message[messageLength] = '\0';
    HudUi::ShowChatLine(message, 5.0f);
    return 1;
}

/**
 * Purpose: Choose a multiplayer respawn point, optionally drop the player's
 * current weapon pickup, reset transient player state, and refresh mission
 * vehicle unlock flags.
 */
void __fastcall RespawnPlayerAndDropWeaponPickupIfAllowed(zUtil_SaveGameState* saveState, int useColorIndexedSpawn)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    const int localColorIndex = GetLocalPlayerColorIndexOrZero();
    GameNetSpawnPoint* spawnPoint = g_GameNetSpawnPointHead;
    float bestNearestDistanceSq = 0.0f;
    GameNetSpawnPoint* selectedSpawn = spawnPoint;

    if (useColorIndexedSpawn != 0) {
        for (int colorIndex = 1; colorIndex < localColorIndex; ++colorIndex) {
            if (spawnPoint == 0) {
                break;
            }
            spawnPoint = spawnPoint->next;
        }
        selectedSpawn = spawnPoint;
    } else if (g_HudSensorTracker.raceCheckpointMode != 0) {
        selectedSpawn = 0;
    } else {
        zUtil_PlayerStateStorage* const dropState = saveState->playerState;
        PickupType* const pickupType = Pickup::FindDroppableTypeForPlayerCurrentWeapon(saveState);
        PickupParsedZrdEntry entry;
        entry.typeDesc = pickupType;
        entry.amount = pickupType->defaultAmount;
        entry.position = dropState->worldPos;
        entry.rotation = dropState->vehicleRotationAngles;
        entry.param = 0;
        entry.unknown_2c = 0;
        entry.respawnDelay = 0.0f;
        PickupSpawnDef* const pickupSpawn = Pickup::SpawnFromParsedZrdEntry(&entry);
        if (pickupSpawn != 0) {
            Pickup::SendPkt11CreateDelta(pickupSpawn);
        }

        GameNetPlayerSaveState* nearestSaveState;
        while (spawnPoint != 0) {
            const float nearestDistanceSq = GetNearestOtherPlayerDistanceToSpawnPoint(spawnPoint, &nearestSaveState);
            if (nearestDistanceSq > bestNearestDistanceSq) {
                bestNearestDistanceSq = nearestDistanceSq;
                selectedSpawn = spawnPoint;
            }
            spawnPoint = spawnPoint != 0 ? spawnPoint->next : 0;
        }
    }

    if (selectedSpawn != 0) {
        const double kDegreesToRadians = 0.017453292519943295;
        zVec3 position = selectedSpawn->position;
        Player::SetWorldPoseAndRestartAnchor(
            saveState,
            &position,
            (float)(selectedSpawn->yawDegrees * kDegreesToRadians)
        );
    }

    if (saveState->primaryModalState->masterModalData->masterType != 3 && g_HudSensorTracker.raceCheckpointMode == 0) {
        Player::TransitionToMasterTypeTrack(saveState, 1);
    }

    Player::ResetMouseControlStateAndRecenterCursor(saveState);
    Player::ResetMotionTransientState(saveState);
    playerState->amphibUnlocked = Player::IsMissionProbeType1EnabledById(g_HudSensorTracker.GetMissionId());
    playerState->subUnlocked = 0;
    playerState->hoverUnlocked = 0;
}

/**
 * Purpose: Measure the nearest player-row save state other than the active
 * game-state-table row for a candidate multiplayer spawn point.
 */
float __fastcall
GetNearestOtherPlayerDistanceToSpawnPoint(GameNetSpawnPoint* spawnPoint, GameNetPlayerSaveState** outSaveState)
{
    float nearestDistanceSq = 1.0e23f;
    GameNetPlayerRow* row = g_GameNetPlayerRowHead;
    while (row != 0) {
        GameNetPlayerSaveState* const saveState = row->saveState;
        if (saveState != (GameNetPlayerSaveState*)(g_GameStateOrMapTable)) {
            const float distanceSq = zMath::Vec3DeltaLengthSq(&saveState->playerState->worldPos, &spawnPoint->position);
            if (distanceSq < nearestDistanceSq) {
                nearestDistanceSq = distanceSq;
                *outSaveState = saveState;
            }
        }

        row = row != 0 ? row->next : 0;
    }

    return nearestDistanceSq;
}
} // namespace GameNet

/**
 * Purpose: Clears the locally cached HUD timer tail flags.
 */
void HudTimerPanelNetState::ClearTailFlagsLocal()
{
    for (int index = 0; index < 8; ++index) {
        tailFlags[index] = 0;
    }
}

/**
 * Purpose: Applies the selected player color to the row's modal display.
 */
void GameNetPlayerRow::ApplyPlayerColorTint()
{
    PlayerModalState* primaryModalState = saveState->primaryModalState;
    const unsigned int packedColor = g_GameNetPlayerRowStyleColors_00RRGGBB[playerColorIndex];
    zColorRgb color = {
        (float)GetRValue(packedColor),
        (float)GetGValue(packedColor),
        (float)GetBValue(packedColor),
    };
    CZObject3D::gwObject3DSetColorAlpha(primaryModalState->modalNode, &color, 0.2f);
    CZObject3D::gwObject3DSetVisibleFlag(primaryModalState->modalNode, 1);
}

namespace zDEClient_Crater {
/**
 * Purpose: Normalizes a crater event and relays locally owned events to the network.
 */
int __fastcall Execute(zDEClient_CraterEventTemplate* eventTemplate)
{
    if (eventTemplate->radius > 0.0f) {
        if (eventTemplate->damageOwnerNode == ((zUtil_SaveGameState*)(g_GameStateOrMapTable))->playerState->rootNode) {
            g_NetPkt0F_CraterEventRelayBuf.header.payloadDword0 = zNetworkGetLocalPlayerKey();
            g_NetPkt0F_CraterEventRelayBuf.craterTypeId
                = zModel_MatlSlot::IndexFromPtrOrMinus1(eventTemplate->craterMaterialSlot);
            g_NetPkt0F_CraterEventRelayBuf.center = eventTemplate->center;
            g_NetPkt0F_CraterEventRelayBuf.radius = eventTemplate->radius;
            if (zNetwork::IsHost() != 0) {
                NetRelayCallback(zNetworkGetLocalPlayerKey(), &g_NetPkt0F_CraterEventRelayBuf);
                return 0;
            }
            zNetworkSendPacketReliable(&g_NetPkt0F_CraterEventRelayBuf.header);
        }
        return 0;
    }
    eventTemplate->radius = -eventTemplate->radius;
    return 1;
}

/**
 * Purpose: Reconstructs an incoming crater event and relays it when hosting.
 */
int __fastcall NetRelayCallback(int, NetPkt0F_CraterEvent* packet)
{
    zDEClient_CraterEventTemplate eventTemplate;
    InitEventTemplateDefaults(&eventTemplate);
    if (zNetwork::IsHost() != 0) {
        eventTemplate.craterMaterialSlot = zModel_Matl::GetPoolEntry(packet->craterTypeId);
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        if (InstanceEventMaybeRelay(&eventTemplate) == 0) {
            packet->header.payloadDword0 = zNetworkGetLocalPlayerKey();
            packet->eventFlags |= 0x80u;
            zNetworkSendPacketReliable(&packet->header);
        }
        return 1;
    }
    if ((packet->eventFlags & 0x80u) != 0) {
        eventTemplate.craterMaterialSlot = zModel_Matl::GetPoolEntry(packet->craterTypeId);
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        InstanceEventMaybeRelay(&eventTemplate);
    }
    return 1;
}
} // namespace zDEClient_Crater

namespace GameNet {
/**
 * Purpose: Relay a host-authored crater feature event to network peers.
 */
int __fastcall HostSendPkt0FCraterFeature(zDEClient_CraterEventTemplate* eventTemplate)
{
    if (zNetwork::IsHost() == 0) {
        return 0;
    }

    g_NetPkt0F_CraterEventSendBuf.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    g_NetPkt0F_CraterEventSendBuf.craterTypeId
        = zModel_MatlSlot::IndexFromPtrOrMinus1(eventTemplate->craterMaterialSlot);
    g_NetPkt0F_CraterEventSendBuf.center = eventTemplate->center;
    g_NetPkt0F_CraterEventSendBuf.radius = eventTemplate->radius;
    g_NetPkt0F_CraterEventSendBuf.eventFlags |= 0x80u;
    zNetworkSendPacketReliable(&g_NetPkt0F_CraterEventSendBuf.header);
    return 1;
}

/**
 * Purpose: relay local quicksand feature events through packet 0x10 after
 * validating local damage ownership.
 */
int __fastcall SendPkt10QSandEvent(zDEClient_QSandEventTemplate* eventTemplate)
{
    if (eventTemplate->radius > 0.0f) {
        if (eventTemplate->damageOwnerNode == ((zUtil_SaveGameState*)(g_GameStateOrMapTable))->playerState->rootNode) {
            ::g_NetPkt10_QSandEventRelayBuf.header.payloadDword0 = zNetworkGetLocalPlayerKey();
            ::g_NetPkt10_QSandEventRelayBuf.center = eventTemplate->center;
            ::g_NetPkt10_QSandEventRelayBuf.radius = eventTemplate->radius;
            ::g_NetPkt10_QSandEventRelayBuf.eventFlags = 0;

            if (zNetwork::IsHost() != 0) {
                zDEClient_QSand::NetRelayCallback(zNetworkGetLocalPlayerKey(), &::g_NetPkt10_QSandEventRelayBuf);
            } else {
                zNetworkSendPacketReliable(&::g_NetPkt10_QSandEventRelayBuf.header);
            }
        }

        return 0;
    }

    eventTemplate->radius = -eventTemplate->radius;
    return 1;
}
} // namespace GameNet

namespace zDEClient_QSand {
/**
 * Purpose: Reconstructs an incoming quicksand event and relays it when hosting.
 */
int __fastcall NetRelayCallback(int, NetPkt10_QSandEvent* packet)
{
    zDEClient_QSandEventTemplate eventTemplate;
    zDEClient::CopyQSandEventTemplateDefaults(&eventTemplate);
    if (zNetwork::IsHost() != 0) {
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        if (InstanceEventMaybeRelay(&eventTemplate) == 0) {
            packet->header.payloadDword0 = zNetworkGetLocalPlayerKey();
            packet->eventFlags |= 0x80u;
            zNetworkSendPacketReliable(&packet->header);
        }
        return 1;
    }
    if ((packet->eventFlags & 0x80u) != 0) {
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        InstanceEventMaybeRelay(&eventTemplate);
    }
    return 1;
}
} // namespace zDEClient_QSand

namespace GameNet {
/**
 * Purpose: Relay a host-authored quicksand feature event to network peers.
 */
int __fastcall HostSendPkt10QSandFeature(zDEClient_QSandEventTemplate* eventTemplate)
{
    if (zNetwork::IsHost() == 0) {
        return 0;
    }

    g_NetPkt10_QSandEventSendBuf.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    g_NetPkt10_QSandEventSendBuf.center = eventTemplate->center;
    g_NetPkt10_QSandEventSendBuf.radius = eventTemplate->radius;
    g_NetPkt10_QSandEventSendBuf.eventFlags |= 0x80u;
    zNetworkSendPacketReliable(&g_NetPkt10_QSandEventSendBuf.header);
    return 1;
}
} // namespace GameNet

namespace Pickup {
/**
 * Purpose: Sends the flag-2 state update for a pickup spawn.
 */
int __fastcall SendPkt11Flag2Delta(PickupSpawnDef* spawn)
{
    g_PickupPkt11Flag2Delta.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    g_PickupPkt11Flag2Delta.flags = 2;
    g_PickupPkt11Flag2Delta.pickupId = spawn->pickupId;
    return zNetworkSendPacketReliable(&g_PickupPkt11Flag2Delta.header);
}

/**
 * Purpose: Sends the flag-8 state update for a pickup spawn.
 */
int __fastcall SendPkt11Flag8Delta(PickupSpawnDef* spawn)
{
    g_PickupPkt11Flag8Delta.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    g_PickupPkt11Flag8Delta.flags = 8;
    g_PickupPkt11Flag8Delta.pickupId = spawn->pickupId;
    return zNetworkSendPacketReliable(&g_PickupPkt11Flag8Delta.header);
}

/**
 * Purpose: Builds and sends the network create-state packet for a pickup spawn.
 */
void __fastcall SendPkt11CreateDelta(PickupSpawnDef* spawn)
{
    PickupPkt11CreateDelta* const packet = (PickupPkt11CreateDelta*)(malloc(sizeof(PickupPkt11CreateDelta)));
    memset(packet, 0, sizeof(PickupPkt11CreateDelta));
    packet->header.packetType = 0x11;
    packet->header.packetSizeBytes = sizeof(PickupPkt11CreateDelta);
    packet->header.payloadDword0 = zNetworkGetLocalPlayerKey();
    packet->flags = 1;
    packet->pickupId = spawn->pickupId;
    packet->typeKeyIndex = (unsigned short)(PickupTypeKeyTable::FindIndex(spawn->pickupType->logicalName));
    packet->position = spawn->position;
    packet->rotation = spawn->rotation;
    packet->amount = spawn->amount;
    packet->respawnDelay = spawn->respawnDelay;
    zNetworkSendPacketReliable(&packet->header);
    free(packet);
}

/**
 * Purpose: Applies an incoming pickup creation or state-change packet.
 */
int __fastcall HandlePkt11SpawnDelta(int, PickupPkt11CreateDelta* packet)
{
    PickupSpawnDef* const spawn = FindSpawnByPickupId(packet->pickupId, &g_PickupSpawnList_Primary);
    const unsigned short flags = packet->flags;
    if ((flags & 1u) != 0 && spawn == 0) {
        PickupParsedZrdEntry entry;
        memset(&entry, 0, sizeof(entry));
        entry.typeDesc = PickupType::GetByIndex((int)(packet->typeKeyIndex));
        entry.amount = packet->amount;
        entry.position = packet->position;
        entry.rotation = packet->rotation;
        entry.respawnDelay = packet->respawnDelay;
        PickupSpawnDef* const newSpawn = SpawnFromParsedZrdEntry(&entry);
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
        PickupSpawnList::RemoveAndFreeNode(spawn, &g_PickupSpawnList_Primary);
        return 1;
    }
    if ((flags & 8u) != 0) {
        CZNodePartial* const pickupObj = spawn->pickupObj;
        CZNode::ClearPickupFlagsRecursive(pickupObj);
        CZClass::gwNodeSetRaycastable(pickupObj, 0);
        CZClass::gwNodeSetPickable(pickupObj, 0);
        RemoveObject(0, pickupObj, 0);
    }
    return 1;
}

/**
 * Purpose: Sends an airdrop pickup spawn and chute update to network peers.
 */
void __fastcall SendPkt12AirdropSpawnChuteRelay(int pickupTypeIndex, zVec3* spawnPos, int nextPickupId)
{
    g_PickupPkt12AirdropSpawnChuteRelay.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    g_PickupPkt12AirdropSpawnChuteRelay.spawnPos = *spawnPos;
    g_PickupPkt12AirdropSpawnChuteRelay.pickupTypeIndex = (unsigned short)(pickupTypeIndex);
    g_PickupPkt12AirdropSpawnChuteRelay.nextPickupId = nextPickupId;
    zNetworkSendPacketReliable(&g_PickupPkt12AirdropSpawnChuteRelay.header);
}

/**
 * Purpose: Applies an incoming airdrop pickup spawn and next-id update.
 */
int __fastcall HandlePkt12AirdropSpawnChuteRelay(int, PickupPkt12AirdropSpawnChuteRelay* packet)
{
    SetNextPickupId(packet->nextPickupId);
    SpawnWithAirdropChute((int)(packet->pickupTypeIndex), &packet->spawnPos);
    return 1;
}
} // namespace Pickup

namespace OptCatalog {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-altgundispatchallocruntimegatecallback
 * @recoil-artifact defines .text recoil:function:0x4340c0: OptCatalog::AltGunDispatchAllocRuntimeGateCallback
 * @recoil-match byte
 *
 * Purpose: Determines whether an alternate-gun catalog entry may allocate runtime state.
 */
int __fastcall AltGunDispatchAllocRuntimeGateCallback(OptCatalogEntryDef* self, void** saveStateSlot)
{
    if (self->ordinalIndex == 0 || self->ordinalIndex == 1) {
        return 1;
    }
    zUtil_SaveGameState* const saveState = (zUtil_SaveGameState*)(*saveStateSlot);
    if (saveState != 0) {
        zUtil_PlayerStateStorage* const playerState = saveState->playerState;
        if (saveState == (zUtil_SaveGameState*)(g_GameStateOrMapTable)) {
            *saveStateSlot = (void*)(zVideo::ReturnSuccessStub());
            GameNet::SendPkt07_AltGunDispatch(self->ordinalIndex, (unsigned int)(*saveStateSlot));
            *saveStateSlot = (void*)((unsigned int)(*saveStateSlot) | 0x01000000u);
            return 1;
        }
        const unsigned int dispatchFlags = (unsigned int)(playerState->altGunDispatchFlags);
        if ((dispatchFlags & 0x02000000u) != 0) {
            *saveStateSlot = (void*)(dispatchFlags);
            return 1;
        }
    }
    return 0;
}
} // namespace OptCatalog

namespace GameNet {
/**
 * Purpose: send the local alternate-gun dispatch packet to peers.
 */
void __fastcall SendPkt07_AltGunDispatch(int weaponId, unsigned int dispatchFlags)
{
    zUtil_SaveGameState* const saveState = (zUtil_SaveGameState*)(g_GameStateOrMapTable);
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    g_NetPkt07_AltGunDispatchBuf.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    g_NetPkt07_AltGunDispatchBuf.weaponId = weaponId;
    g_NetPkt07_AltGunDispatchBuf.dispatchFlags = dispatchFlags;
    g_NetPkt07_AltGunDispatchBuf.targetPos = playerState->storedTargetPos;
    zNetworkSendPacketReliable(&g_NetPkt07_AltGunDispatchBuf.header);
}

/**
 * Purpose: apply a remote pkt07 alternate-gun dispatch to the matching player
 * row.
 */
int __fastcall HandlePkt07_AltGunDispatch(int, NetPkt07_AltGunDispatch* packet)
{
    GameNetPlayerRow* const row = FindPlayerRowByKey(packet->header.payloadDword0);
    if (row == 0) {
        return 0;
    }

    zUtil_SaveGameState* const saveState = (zUtil_SaveGameState*)row->saveState;
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    playerState->altGunDispatchFlags = (int)(packet->dispatchFlags | ::kGameNetRemoteAltGunDispatchFlag);
    playerState->storedTargetPos = packet->targetPos;

    PlayerGunFireController* const oldActiveAltGunController = playerState->activeAltGunController;
    playerState->activeAltGunController
        = Player::FindAltGunFireControllerForWeaponId(saveState, (int)(packet->weaponId));

    OptCatalog::SetPendingSpawnTargetOverrides(&playerState->progressTargetCount, playerState->progressTargetSlots);
    Player::ProcessAltGunDispatchRequest(saveState);

    playerState->altGunDispatchFlags = 0;
    playerState->activeAltGunController = oldActiveAltGunController;
    OptCatalog::SetPendingSpawnTargetOverrides(0, 0);
    return 1;
}

/**
 * Purpose: accept remote alternate-gun runtime allocation without local side
 * effects.
 */
int __fastcall AltGunDispatchNoOpCallback(OptCatalogEntryDef*, void**)
{
    return 1;
}
} // namespace GameNet

// Shared zero vector in player_move.cpp's read-only data; retail reads it at
// 0x4d0788 for the removal relay's absent point.
extern const zVec3 g_Player_ConstZeroVec3;

namespace OptCatalog {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-sendpkt0a-removeruntimerelay
 * @recoil-artifact defines .text recoil:function:0x434240: OptCatalog::SendPkt0ARemoveRuntimeRelay
 * @recoil-match byte
 *
 * Purpose: Sends a network relay describing removal of a runtime catalog object.
 */
void __fastcall SendPkt0ARemoveRuntimeRelay(OptCatalogEntryDef* self, zVec3* pointOrVec3, CZNodePartial* ownerNode)
{
    if (g_OptCatalogProcessRuntimeRelayEnabled == 0 || ownerNode == 0) {
        return;
    }
    HudUiMgrSensorTrackNode* const ownerTrackContext = (HudUiMgrSensorTrackNode*)(ownerNode->callbackContext);
    if (ownerTrackContext == 0) {
        return;
    }
    zUtil_SaveGameState* const ownerSaveState = (zUtil_SaveGameState*)(ownerTrackContext->payload);
    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.optCatalogEntryId = (short)(self->ordinalIndex);
    if (pointOrVec3 == 0) {
        g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3 = g_Player_ConstZeroVec3;
    } else {
        g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3 = *pointOrVec3;
    }
    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.ownerPlayerKey = ownerSaveState->netPlayerRow->playerKey;
    zNetworkSendPacketReliable(&g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.header);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-handlepkt0a-removeruntimerelay
 * @recoil-artifact defines .text recoil:function:0x4342d0: OptCatalog::HandlePkt0ARemoveRuntimeRelay
 * @recoil-match byte
 *
 * Purpose: Resolves and applies an incoming runtime-object removal relay.
 */
int __fastcall HandlePkt0ARemoveRuntimeRelay(int, NetPkt0A_RemoveRuntimeRelay* packet)
{
    OptCatalogEntryDef* const entry = OptCatalog::FindEntryById((int)(packet->optCatalogEntryId));
    zVec3 relayPointScratch;
    zVec3* pointOrVec3;
    if (packet->pointOrVec3.x == 0.0f && packet->pointOrVec3.y == 0.0f && packet->pointOrVec3.z == 0.0f) {
        pointOrVec3 = 0;
    } else {
        pointOrVec3 = &relayPointScratch;
    }
    GameNetPlayerRow* const row = GameNet::FindPlayerRowByKey(packet->ownerPlayerKey);
    if (row == 0) {
        return 0;
    }
    zUtil_SaveGameState* const ownerSaveState = (zUtil_SaveGameState*)row->saveState;
    if (entry != 0 && ownerSaveState != 0) {
        g_OptCatalogProcessRuntimeRelayEnabled = 0;
        OptCatalog::RemoveRuntimeInstance(entry, pointOrVec3, ownerSaveState->playerState->rootNode);
        g_OptCatalogProcessRuntimeRelayEnabled = 1;
    }
    return 1;
}
} // namespace OptCatalog

namespace GameNet {
/**
 * Purpose: Send a reliable pkt13 effect-animation activation record unless
 * replay echo suppression is active.
 */
void __fastcall SendPkt13EffectAnimActivationRecord(zEffectAnimActivationRecord* record)
{
    if (g_GameNetSuppressPkt13ActivationEcho != 0) {
        return;
    }

    const int packedRecordSize = zEffect_Anim::GetActivationRecordPackedSize(record);
    const int packetSize = (int)(sizeof(zNetworkPacketHeader)) + packedRecordSize;
    zNetworkPacketHeader* const packet = (zNetworkPacketHeader*)(malloc((size_t)(packetSize)));
    memset(packet, 0, (size_t)(packetSize));

    packet->packetType = 0x13;
    packet->packetSizeBytes = (short)(packetSize);
    packet->payloadDword0 = zNetworkGetLocalPlayerKey();
    memcpy(((unsigned char*)(packet)) + sizeof(zNetworkPacketHeader), record, (size_t)(packedRecordSize));

    zNetworkSendPacketReliable(packet);
    free(packet);
}

/**
 * Purpose: Apply a new remote effect-animation activation record while
 * suppressing replay echo.
 */
int __fastcall HandlePkt13EffectAnimActivationRecord(int, zNetworkPacketHeader* packet)
{
    zEffectAnimActivationRecord* const record
        = (zEffectAnimActivationRecord*)((unsigned char*)(packet) + sizeof(zNetworkPacketHeader));
    if (zEffect_Anim::HasActivationRecord(record) == 0) {
        g_GameNetSuppressPkt13ActivationEcho = 1;
        zEffect_Anim::ProcessActivationRecord(record);
        g_GameNetSuppressPkt13ActivationEcho = 0;
    }

    return 1;
}

/**
 * Purpose: Broadcast every queued effect-animation activation record from the
 * host.
 */
void __cdecl SendAllPkt13EffectAnimActivationRecords()
{
    if (zNetwork::IsHost() == 0) {
        return;
    }

    const int recordCount = zEffect_Anim::GetActivationRecordCount();
    for (int index = 0; index < recordCount; ++index) {
        SendPkt13EffectAnimActivationRecord(zEffect_Anim::GetActivationRecordAt(index));
    }
}

/**
 * Purpose: Send the reliable packet that synchronizes HUD timer and status flags.
 */
int __fastcall SendPkt14HudTimerAndFlagsSync(int eventCode, unsigned int statusFlags, int valueOrTime, int auxParam)
{
    g_NetPkt14_HudTimerAndFlagsSyncBuf.header.payloadDword0 = zNetworkGetLocalPlayerKey();
    g_NetPkt14_HudTimerAndFlagsSyncBuf.eventCode = (short)(eventCode);
    g_NetPkt14_HudTimerAndFlagsSyncBuf.auxParam = (short)(auxParam);
    g_NetPkt14_HudTimerAndFlagsSyncBuf.valueOrTime = valueOrTime;
    g_NetPkt14_HudTimerAndFlagsSyncBuf.statusFlags = statusFlags;
    return zNetworkSendPacketReliable(&g_NetPkt14_HudTimerAndFlagsSyncBuf.header);
}

/**
 * Purpose: Receive the HUD timer/status sync packet and start the matching mission state.
 */
int __fastcall HandlePkt14HudTimerAndFlagsSync(int senderPlayerId, NetPkt14_HudTimerAndFlagsSync* packet)
{
    (void)senderPlayerId;

    UnregisterGameplayPacketHandlers();
    ResetRemotePlayersAndSpawnLists();

    g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue((float)(packet->valueOrTime) * 60.0f, packet->auxParam);

    CZRecoilFrame* const mainWnd = (CZRecoilFrame*)((unsigned int)(g_RecoilApp.GetMainWnd()));
    g_HudSensorTracker.InitMissionIdAndFlags(packet->eventCode + 6, mainWnd->m_useArchiveBanks);
    SetStatusBitsFromFlags(packet->statusFlags);

    g_RecoilApp.m_missionFmvState.m_missionId = 0;
    g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_introFmvState, 0);

    if (zNetwork::IsHost() != 0) {
        HostUpdateSessionDescStatusFields(
            packet->eventCode,
            packet->auxParam,
            packet->valueOrTime,
            packet->statusFlags
        );
    }

    return 1;
}

/**
 * Purpose: Let the host mirror timer and status fields into the session descriptor.
 */
int __fastcall HostUpdateSessionDescStatusFields(int eventCode, int auxParam, int valueOrTime, int statusFlags)
{
    int result = zNetwork::IsHost();
    if (result != 0) {
        zNetworkSessionDescStatusFields statusFields;
        result = zNetworkExtractStatusFieldsFromSessionDesc(&statusFields);
        if (result != 0) {
            statusFields.eventCode = eventCode;
            statusFields.statusFlags = statusFlags;
            statusFields.valueOrTime = valueOrTime;
            statusFields.auxParam = auxParam;
            result = zNetworkApplyStatusFieldsToSessionDesc(&statusFields);
        }
    }

    return result;
}
} // namespace GameNet

/**
 * Purpose: Allocate a scoreboard player row and append it to this GameNet
 * player-row list header.
 */
GameNetPlayerRow* GameNetPlayerRowListState::AppendNewRow(int zeroInitializeRow)
{
    GameNetPlayerRow* const row = new GameNetPlayerRow;
    if (zeroInitializeRow != 0) {
        memset(row, 0, sizeof(GameNetPlayerRow));
    }

    if (row != 0) {
        row->next = 0;
        if (count == 0) {
            head = row;
        } else {
            tail->next = row;
        }

        tail = row;
        row->next = 0;
        ++count;
    }
    return row;
}

/**
 * Purpose: Destroys the player row's embedded HUD panel.
 */
void GameNetPlayerRow::DestroyEmbeddedPanel()
{
    hudWidget.HudUiPanel::~HudUiPanel();
}

namespace GameNet {

} // namespace GameNet

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void(__cdecl* GameNetCrtInitializerFn)();
/* VC5 emits these GameNet.cpp startup callbacks as direct .CRT$XCU rows. */
#pragma data_seg(".CRT$XCU")
GameNetCrtInitializerFn s_GameNetCrtInit_SpawnPointListInitGlobals = GameNetSpawnPointList::InitGlobals;
GameNetCrtInitializerFn s_GameNetCrtInit_PlayerRowListReset = GameNetPlayerRowList::Reset;
#pragma data_seg()
#endif
