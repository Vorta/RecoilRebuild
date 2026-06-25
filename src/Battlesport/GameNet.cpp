#include "Battlesport/GameNet.h"

#include "Battlesport/Briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/NetUi.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/mission.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zTurret/zTurret.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <shellapi.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_MSC_VER) || _MSC_VER >= 1300
#include <new>
#endif

// Provider-boundary accessor for imported MFC42 CDialog metadata; this does not reimplement
// CDialog behavior.
class NetSessionBrowserCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

// Provider-boundary accessor for imported MFC42 CDialog metadata; this does not reimplement
// CDialog behavior.
class NetSessionConfigCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

// Provider-boundary accessor for imported MFC42 protected window/dialog members; this does
// not reimplement provider behavior.
class GameNetMfcWndAccess : public CWnd {
  public:
    long CallDefault();
    void CallOnDestroy();
};

// Provider-boundary accessor for imported MFC42 protected dialog members; this does not
// reimplement provider behavior.
class GameNetMfcDialogAccess : public CDialog {
  public:
    void CallOnOK();
};

void __stdcall DDX_Control(
    CDataExchange *dataExchange,
    int controlId,
    CWnd &control
);
void __stdcall DDX_Text(
    CDataExchange *dataExchange,
    int controlId,
    CString &value
);
void __stdcall DDX_Text(
    CDataExchange *dataExchange,
    int controlId,
    unsigned int &value
);
void __stdcall DDX_Check(
    CDataExchange *dataExchange,
    int controlId,
    int &value
);
void __stdcall DDV_MaxChars(
    CDataExchange *dataExchange,
    const CString &value,
    int maxChars
);
void __stdcall DDV_MinMaxUInt(
    CDataExchange *dataExchange,
    unsigned int value,
    unsigned int minValue,
    unsigned int maxValue
);

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CEdit) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CButton) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CListBox) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CComboBox) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CSpinButtonCtrl) == 0x40);

extern "C" {
/**
 * Reimplements data 0x4f3f10: g_GameNetPlayerRowList.
 * Purpose: Owns the multiplayer player-row linked-list header for active
 * local and remote network participants.
 */
GameNetPlayerRowListState g_GameNetPlayerRowList = {0, 0, 0, 0};
/**
 * Reimplements data 0x4f3f78: g_GameNetSpawnPointList.
 * Purpose: Owns the multiplayer spawn-point linked-list header loaded from
 * net.zrd during network mission startup.
 */
GameNetSpawnPointListState g_GameNetSpawnPointList = {0, 0, 0, 0};
/**
 * Reimplements data 0x4dcd88: g_GameNetPlayerRowStyleColors_00RRGGBB.
 * Purpose: Maps network player color indices to HUD row and player tint
 * colors in packed 00RRGGBB order.
 */
unsigned int g_GameNetPlayerRowStyleColors_00RRGGBB[9] = {
    0x00000000,
    0x000000ff,
    0x0000ff00,
    0x00ff0000,
    0x00ff00ff,
    0x00ffff00,
    0x0000ffff,
    0x00ffffff,
    0x000040ff,
};
/**
 * Reimplements data 0x4f3f20: g_HudTimerPanelNetState.
 * Purpose: Stores replicated multiplayer HUD timer state and resend/warning
 * flags shared by GameNet timer packet handlers.
 */
HudTimerPanelNetState g_HudTimerPanelNetState = {0};
/**
 * Reimplements data 0x4dce88: g_NetPkt0C_HudTimerStatusBitsBuf.
 * Purpose: Holds the reusable network packet buffer for HUD timer status-bit
 * replication.
 */
NetPkt0C_HudTimerStatusBits g_NetPkt0C_HudTimerStatusBitsBuf = {
    {0x0c, sizeof(NetPkt0C_HudTimerStatusBits), 0},
    0.0f,
    0,
    0,
    0,
};
/**
 * Reimplements data 0x4dce78: g_NetPkt0D_HudTimerPanelStateBuf.
 * Purpose: Holds the reusable network packet buffer for HUD timer panel state
 * replication.
 */
NetPkt0D_HudTimerPanelState g_NetPkt0D_HudTimerPanelStateBuf = {
    {0x0d, sizeof(NetPkt0D_HudTimerPanelState), 0},
    0.0f,
    0,
    0,
};
/**
 * Reimplements data 0x4dcfa0: g_NetPkt14_HudTimerAndFlagsSyncBuf.
 * Purpose: Holds the reusable network packet buffer for HUD timer and status
 * flag synchronization.
 */
NetPkt14_HudTimerAndFlagsSync g_NetPkt14_HudTimerAndFlagsSyncBuf = {
    {0x14, sizeof(NetPkt14_HudTimerAndFlagsSync), 0},
    0,
    0,
    0,
    0,
};
/**
 * Reimplements data 0x4dcf80: g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.
 * Purpose: Holds the reusable packet buffer for OptCatalog runtime relay
 * removal events.
 */
NetPkt0A_RemoveRuntimeRelay g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf = {
    {0x0a, sizeof(NetPkt0A_RemoveRuntimeRelay), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0,
};
/**
 * Reimplements data 0x4dcf60: g_NetPkt07_AltGunDispatchBuf.
 * Purpose: Stores g NetPkt07 AltGunDispatchBuf data used by network_online.gamenet_pkt07_packet_buffer_data.
 */
NetPkt07_AltGunDispatch g_NetPkt07_AltGunDispatchBuf = {
    {0x07, sizeof(NetPkt07_AltGunDispatch), 0},
    0,
    0,
    0,
    {0.0f, 0.0f, 0.0f},
};
/**
 * Reimplements data 0x4dcdb0: g_NetPkt06_PlayerStateSnapshotBuf.
 * Purpose: Holds the reusable local player-state snapshot buffer for packet 6
 * replication.
 */
NetPkt06_PlayerStateSnapshot g_NetPkt06_PlayerStateSnapshotBuf = {
    {0x06, 0, 0},
};
/**
 * Reimplements data 0x4dcea0: g_NetPkt0F_CraterEventRelayBuf.
 * Purpose: Holds the reusable crater packet used by zDEClient_Crater::Execute
 * before host relay or reliable send.
 */
NetPkt0F_CraterEvent g_NetPkt0F_CraterEventRelayBuf = {
    {0x0f, sizeof(NetPkt0F_CraterEvent), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0.0f,
};
/**
 * Reimplements data 0x4dcec0: g_NetPkt0F_CraterEventSendBuf.
 * Purpose: Holds the reusable network packet buffer for crater feature
 * events.
 */
NetPkt0F_CraterEvent g_NetPkt0F_CraterEventSendBuf = {
    {0x0f, sizeof(NetPkt0F_CraterEvent), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0.0f,
};
/**
 * Reimplements data 0x4dcee0: g_NetPkt10_QSandEventRelayBuf.
 * Purpose: Holds the reusable quicksand packet used by GameNet local event
 * relay before host callback or reliable send.
 */
NetPkt10_QSandEvent g_NetPkt10_QSandEventRelayBuf = {
    {0x10, sizeof(NetPkt10_QSandEvent), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0.0f,
};
/**
 * Reimplements data 0x4dcf00: g_NetPkt10_QSandEventSendBuf.
 * Purpose: Holds the reusable network packet buffer for quicksand feature
 * events.
 */
NetPkt10_QSandEvent g_NetPkt10_QSandEventSendBuf = {
    {0x10, sizeof(NetPkt10_QSandEvent), 0},
    0,
    0,
    {0.0f, 0.0f, 0.0f},
    0.0f,
};
/**
 * Reimplements data 0x4f3f8c: g_GameNetOneLapLeftMessageShown.
 * Purpose: Caches whether the local race HUD has already shown the one-lap
 * remaining network message.
 */
int g_GameNetOneLapLeftMessageShown = 0;
/**
 * Reimplements data 0x4f3f88: g_GameNetStatus_AllowMaps.
 * Purpose: Stores the replicated session status bit that allows map use.
 */
int g_GameNetStatus_AllowMaps = 0;
/**
 * Reimplements data 0x4f3f90: g_GameNetStatus_NameTags.
 * Purpose: Stores the replicated session status bit that enables name tags.
 */
int g_GameNetStatus_NameTags = 0;
/**
 * Reimplements data 0x4f3fa0: g_GameNetAllPlayersLapTargetCheckStarted.
 * Purpose: Tracks whether the multiplayer race goal completion check has
 * started for the current session.
 */
int g_GameNetAllPlayersLapTargetCheckStarted = 0;
/**
 * Reimplements data 0x4f3fa8: g_GameNetSuppressPkt13ActivationEcho.
 * Purpose: Suppresses local echo while replaying replicated effect animation
 * activation records.
 */
int g_GameNetSuppressPkt13ActivationEcho = 0;
/**
 * Reimplements data 0x4dcdac: g_GameNetPkt06InitialSyncGate.
 * Purpose: Blocks local pkt06 replication until the first multiplayer state
 * synchronization has been initialized.
 */
int g_GameNetPkt06InitialSyncGate = 1;
/**
 * Reimplements data 0x4f3f6c: g_GameNetPkt06InputBit17Latch.
 * Purpose: Accumulates local pkt06 input bit 17 until the next player-state
 * snapshot is sent.
 */
int g_GameNetPkt06InputBit17Latch = 0;
/**
 * Reimplements data 0x4f3f70: g_GameNetPkt06InputBit16Latch.
 * Purpose: Accumulates local pkt06 input bit 16 until the next player-state
 * snapshot is sent.
 */
int g_GameNetPkt06InputBit16Latch = 0;
/**
 * Reimplements data 0x4f3f9c: g_GameNetPkt06NextSendTimeSec.
 * Purpose: Stores the next accumulated-time deadline for local pkt06 state
 * snapshot transmission.
 */
float g_GameNetPkt06NextSendTimeSec = 0.0f;
/**
 * Reimplements data 0x4f3f98: g_GameNetHostHudTimerInitFlag.
 * Purpose: Tracks host-side HUD timer initialization during multiplayer
 * mission startup.
 */
int g_GameNetHostHudTimerInitFlag = 0;
/**
 * Reimplements data 0x4dce70: g_GameNetHudTimerTenSecondWarningArmed.
 * Purpose: Arms the replicated HUD timer ten-second warning message.
 */
int g_GameNetHudTimerTenSecondWarningArmed = 1;
/**
 * Reimplements data 0x4dce74: g_GameNetHudTimerPendingSaveReminderArmed.
 * Purpose: Arms the replicated HUD timer pending-save reminder message.
 */
int g_GameNetHudTimerPendingSaveReminderArmed = 1;
/**
 * Reimplements data 0x4f3fa4: g_GameNet_HandlersRegistered.
 * Purpose: Records whether GameNet gameplay packet handlers and callback
 * hooks are registered.
 */
int g_GameNet_HandlersRegistered = 0;
/**
 * Reimplements data 0x4f32b4: g_NetUiTcpIpProviderWarningShown.
 * Purpose: Records whether the Net UI has already shown the TCP/IP provider
 * warning for the current UI lifetime.
 */
int g_NetUiTcpIpProviderWarningShown = 0;
}

extern "C" HWND g_RecoilApp_hWndMain;

/**
 * Reimplements data 0x4f32d8: g_NetSessionConfigDialog_MapNameStrings.
 * Purpose: Stores the seven static CString objects used by the multiplayer
 * session configuration map list.
 */
unsigned int g_NetSessionConfigDialog_MapNameStringStorage[7] = {0};
CString *g_NetSessionConfigDialog_MapNameStrings =
    (CString *)&g_NetSessionConfigDialog_MapNameStringStorage[0];

namespace {
const float kGameNetPkt06SendIntervalSec = 0.100000001f;
const float kGameNetHudTimerWarningDurationSec = 5.0f;
const float kGameNetHudTimerTenSecondThreshold = 10.0f;
const float kGameNetHudTimerOneMinuteLeadSec = 60.0f;
const unsigned int kGameNetPkt06InputBit16Flag = 0x10000u;
const unsigned int kGameNetPkt06InputBit17Flag = 0x20000u;
const unsigned int kGameNetPkt06ProgressTargetsFlag = 0x40000u;
const unsigned int kGameNetRemoteAltGunDispatchFlag = 0x2000000u;
const unsigned int kGameNetRemoteCloneNodeFlag = 0x400000u;
const float kGameNetRemoteUnlimitedAmmo = 123456792.0f;
const int kGameNetChatComposeTextCapacity = 0x20;
const int kGameNetChatComposeShiftModifierMask = 0x400;
const int kGameNetChatComposeDigitFirstDik = 0x02;
const int kGameNetChatComposeDigitLastDik = 0x0e;
const int kGameNetChatComposeLetterRowFirstDik = 0x10;
const int kGameNetChatComposeLetterRowLastDik = 0x2b;
const int kGameNetChatComposeHomeRowFirstDik = 0x1e;
const int kGameNetChatComposeHomeRowLastDik = 0x28;
const int kGameNetChatComposeBottomRowFirstDik = 0x2c;
const int kGameNetChatComposeBottomRowLastDik = 0x35;
const int kGameNetChatComposeSpaceDik = 0x39;
const UINT kNetSessionBrowserDialogResourceId = 136;
const int kNetSessionBrowserPlayerNameEditId = 1048;
const int kNetSessionBrowserOkButtonId = 1;
const int kNetSessionBrowserHelpButtonId = 1029;
const int kNetSessionBrowserCreateSessionButtonId = 1030;
const int kNetSessionBrowserSessionListId = 1040;
const int kNetSessionBrowserProviderComboId = 1114;
const int kNetSessionBrowserPlayerNameMaxChars = 21;
const unsigned int kNetSessionBrowserHelpCaptionMessageId = 25;
const unsigned int kNetSessionBrowserHelpNoAssociationMessageId = 32;
const unsigned int kNetSessionBrowserHelpNoDdeAssociationMessageId = 33;
const unsigned int kNetSessionBrowserHelpFileNotFoundMessageId = 34;
const unsigned int kNetSessionBrowserHelpAssociationIncompleteMessageId = 36;
const unsigned int kNetSessionBrowserPlayerNameRequiredMessageId = 23;
const unsigned int kNetSessionBrowserPlayerNameCaptionMessageId = 24;
const unsigned int kNetSessionBrowserNoProviderMessageId = 273;
const unsigned int kNetSessionBrowserTcpIpWarningCaptionMessageId = 18;
const unsigned int kNetSessionBrowserTcpIpWarningFormatMessageId = 38;
const unsigned int kNetSessionBrowserModemOkButtonMessageId = 53;
const unsigned int kNetSessionBrowserModemCreateButtonMessageId = 54;
const unsigned int kNetSessionBrowserJoinButtonMessageId = 55;
const unsigned int kNetSessionBrowserRefreshButtonMessageId = 56;
const UINT kNetSessionConfigDialogResourceId = 146;
const int kNetSessionConfigMaxPlayersSpinId = 1072;
const int kNetSessionConfigSessionNameEditId = 1115;
const int kNetSessionConfigMapComboId = 1116;
const int kNetSessionConfigValueLimitEditId = 1117;
const int kNetSessionConfigTimeLimitEditId = 1118;
const int kNetSessionConfigMaxPlayersEditId = 1119;
const int kNetSessionConfigTimeLimitSpinId = 1120;
const int kNetSessionConfigValueLimitSpinId = 1121;
const int kNetSessionConfigUnusedCheckboxId = 1122;
const int kNetSessionConfigMaxPlayersLabelId = 1125;
const int kNetSessionConfigSessionNameMaxChars = 80;
const unsigned int kNetSessionConfigLimitMax = 10000;
const unsigned int kNetSessionConfigMaxPlayersMin = 2;
const unsigned int kNetSessionConfigMaxPlayersMax = 8;
const unsigned int kNetSessionConfigMaxPlayersSpecialMapMessageId = 12352;
const unsigned int kNetSessionConfigMaxPlayersDefaultMessageId = 12353;
const int kNetSessionConfigSpecialMapIndex = 2;
const int kNetSessionConfigMapNameCount = 7;
const char kNetSessionConfigSessionNameFormat[] = "Exercise %03d";
const UINT kNetSessionConfigSpinSetRangeMessage = 1125;
const unsigned int kNetSessionConfigDefaultValueLimit = 5;
const unsigned int kNetSessionConfigDefaultTimeLimitMinutes = 10;
const unsigned int kNetSessionConfigDefaultMaxPlayers = 8;
const int kNetSessionBrowserModemEventCode = 256;
const int kNetSessionBrowserModemValueOrTime = 10;
const int kNetSessionBrowserModemAuxParam = 10;
const int kNetSessionBrowserModemMaxPlayers = 2;
// Reimplements 0x41b8ac: NetSessionBrowserDialog::kHelpDocsFindExecutableErrorClassTable
// (D:\Proj\Battlesport\GameNet.cpp)
const unsigned char kNetSessionBrowserHelpDocsFindExecutableErrorClassTable[32] = {
    0,
    4,
    1,
    1,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    2,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    4,
    3,
};
RECOIL_STATIC_ASSERT(sizeof(kNetSessionBrowserHelpDocsFindExecutableErrorClassTable) == 32);

struct GameNetReaderArray {
    int countTag;
    int count;
    zReader::Node nodes[1];
};

/**
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Original helper evidence: no standalone retail function; callers 0x431dd0,
 * 0x432ae0, 0x432860, and 0x432e70 share the same HudUiPanel text-color
 * writes.
 * Purpose: Apply a row style color to the embedded scoreboard HUD text
 * panel and mark its cached text image dirty.
 */
void SetEmbeddedHudPanelColor(
    GameNetPlayerRow *row,
    unsigned int color
) {
    row->hudWidget.textColor0 = color;
    row->hudWidget.textColor1 = color;
    row->hudWidget.textDirty = 1;
}

/**
 * Source: D:\Proj\Battlesport\ai_net.cpp
 * Original helper evidence: no standalone retail function; caller 0x4143d0
 * repeats this unregister/register pair across the chat-compose key ranges and
 * the standalone space-bar binding.
 * Purpose: Register one chat-compose keyboard callback binding.
 */
inline void RegisterChatComposeKey(
    int comboIdx
) {
    zInput::Keyboard_UnregisterKeyCallback(comboIdx);
    zInput::Keyboard_RegisterKeyCallback(
        comboIdx,
        (void *)(&GameNet::ChatComposeKeyCallback),
        ""
    );
}

/**
 * Source: D:\Proj\Battlesport\ai_net.cpp
 * Original helper evidence: no standalone retail function; caller 0x4143d0
 * repeats contiguous chat-compose key registration for unmodified and modified
 * DIK ranges.
 * Purpose: Register a contiguous range of chat-compose keyboard bindings.
 */
inline void RegisterChatComposeKeyRange(
    int firstComboIdx,
    int lastComboIdx
) {
    for (int comboIdx = firstComboIdx; comboIdx <= lastComboIdx; ++comboIdx) {
        RegisterChatComposeKey(comboIdx);
        RegisterChatComposeKey(comboIdx | kGameNetChatComposeShiftModifierMask);
    }
}

/**
 * Original helper evidence: no standalone retail function; callers 0x432860,
 * 0x432d60, and 0x432ed0 share the same HudUiPanel visibility dispatch.
 * Purpose: Set remote-player HUD widget visibility.
 */
void GameNetSetRemoteHudVisible(
    HudUiPanel *panel,
    int visible
) {
    panel->SetVisible(visible);
}

/**
 * Original helper evidence: no standalone retail function; caller 0x432d60
 * uses the same typed HudUiPanel position dispatch after screen projection.
 * Purpose: Move a remote-player HUD widget to a projected screen position.
 */
void GameNetSetRemoteHudPos(
    HudUiPanel *panel,
    int x,
    int y
) {
    panel->SetPos(
        x,
        y
    );
}

/**
 * Original helper evidence: no standalone retail function; pkt06 spawn and
 * apply callers share the same ten-bank remote weapon unlock loop.
 * Purpose: Unlock every alternate weapon bank for a replicated remote player.
 */
void GameNetUnlockRemotePlayerWeaponBanks(
    zUtil_PlayerStateStorage *playerState
) {
    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
        bank.controllerA.flags |= 4u;
        bank.controllerA.ammoOrCharge = kGameNetRemoteUnlimitedAmmo;
        bank.controllerB.flags |= 4u;
        bank.controllerB.ammoOrCharge = kGameNetRemoteUnlimitedAmmo;
    }
}

/**
 * Original helper evidence: no standalone retail function; caller 0x432300
 * uses paired top-message display with the same duration constant in host and
 * client HUD timer branches.
 * Purpose: Show two localized HUD timer warning messages as a pair.
 */
void GameNetShowPairedTimerMessages(
    int firstMessageId,
    int secondMessageId
) {
    HudUi::ShowTopMessageLine(
        zLoc::GetMessageString(firstMessageId),
        kGameNetHudTimerWarningDurationSec
    );
    HudUi::ShowTopMessageLine(
        zLoc::GetMessageString(secondMessageId),
        kGameNetHudTimerWarningDurationSec
    );
}

/**
 * Original helper recovered from address-backed pkt06 caller 0x432300 in this
 * source file; no standalone retail function is present.
 * Purpose: Copy the active progress target positions into the outgoing pkt06
 * player-state snapshot payload.
 */
void GameNetCopyPkt06ProgressTargets(
    NetPkt06_PlayerStateSnapshot *packet,
    zUtil_PlayerStateStorage *playerState
) {
    for (int index = 0; index < playerState->progressTargetCount; ++index) {
        const zVec3 *const targetPos = playerState->progressTargetSlots[index].targetPos;
        packet->progressTargetPoints[index] = *targetPos;
    }
}

/**
 * Original helper evidence: no standalone retail function; pkt06 decode
 * callers use the same bank/side arithmetic for weapon-controller selection.
 * Purpose: Decode a packed alternate-weapon selection into a controller slot.
 */
PlayerGunFireController *GameNetPkt06DecodeWeaponController(
    zUtil_PlayerStateStorage *playerState,
    int selectionCode
) {
    const int bankIndex = selectionCode / 100;
    const int sideIndex = selectionCode % 100;
    return &playerState->altWeaponBanks[bankIndex].controllerA + sideIndex;
}
} // namespace

/**
 * Provider-boundary accessor for imported MFC42 CDialog message-map metadata.
 * Purpose: Return the provider CDialog message map for the browser dialog chain.
 */
const AFX_MSGMAP *__stdcall NetSessionBrowserCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Original helper evidence: no standalone retail function; MFC message-map
 * tables reference this base-map accessor for NetSessionBrowserDialog.
 * Purpose: Return the browser dialog base message map.
 */
const AFX_MSGMAP *__stdcall NetSessionBrowserDialog::GetBaseMessageMapForMfc() {
    return NetSessionBrowserCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const NetSessionBrowserDialog::messageEntries[] = {
    {WM_COMMAND,
        CBN_CLOSEUP,
        kNetSessionBrowserProviderComboId,
        kNetSessionBrowserProviderComboId,
        12,
        (AFX_PMSG)&NetSessionBrowserDialog::ConnectSelectedProvider},
    {WM_COMMAND,
        BN_CLICKED,
        kNetSessionBrowserCreateSessionButtonId,
        kNetSessionBrowserCreateSessionButtonId,
        12,
        (AFX_PMSG)&NetSessionBrowserDialog::OnCreateSession},
    {WM_TIMER, 0, 0, 0, 13, (AFX_PMSG)&NetSessionBrowserDialog::OnTimer},
    {WM_DESTROY, 0, 0, 0, 12, (AFX_PMSG)&NetSessionBrowserDialog::OnDestroy},
    {WM_COMMAND,
        BN_CLICKED,
        kNetSessionBrowserHelpButtonId,
        kNetSessionBrowserHelpButtonId,
        12,
        (AFX_PMSG)&NetSessionBrowserDialog::OnHelpDocs},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP NetSessionBrowserDialog::messageMap = {
    &NetSessionBrowserDialog::GetBaseMessageMapForMfc,
    &NetSessionBrowserDialog::messageEntries[0],
};

/**
 * Provider-boundary accessor for imported MFC42 CDialog message-map metadata.
 * Purpose: Return the provider CDialog message map for the config dialog chain.
 */
const AFX_MSGMAP *__stdcall NetSessionConfigCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Original helper evidence: no standalone retail function; MFC dialog methods
 * in this source file share the same protected CWnd::Default dispatch.
 * Purpose: Call the provider CWnd default handler.
 */
long GameNetMfcWndAccess::CallDefault() {
    return CWnd::Default();
}

/**
 * Original helper evidence: no standalone retail function; MFC dialog methods
 * in this source file share the same protected CWnd::OnDestroy dispatch.
 * Purpose: Call the provider CWnd destroy handler.
 */
void GameNetMfcWndAccess::CallOnDestroy() {
    CWnd::OnDestroy();
}

/**
 * Original helper evidence: no standalone retail function; MFC dialog methods
 * in this source file share the same protected CDialog::OnOK dispatch.
 * Purpose: Call the provider CDialog OK handler.
 */
void GameNetMfcDialogAccess::CallOnOK() {
    CDialog::OnOK();
}

/**
 * Original helper evidence: no standalone retail function; MFC message-map
 * tables reference this base-map accessor for NetSessionConfigDialog.
 * Purpose: Return the config dialog base message map.
 */
const AFX_MSGMAP *__stdcall NetSessionConfigDialog::GetBaseMessageMapForMfc() {
    return NetSessionConfigCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const NetSessionConfigDialog::messageEntries[] = {
    {WM_DESTROY, 0, 0, 0, 12, (AFX_PMSG)&NetSessionConfigDialog::OnDestroy},
    {WM_COMMAND,
        CBN_SELCHANGE,
        kNetSessionConfigMapComboId,
        kNetSessionConfigMapComboId,
        12,
        (AFX_PMSG)&NetSessionConfigDialog::OnMapChanged},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP NetSessionConfigDialog::messageMap = {
    &NetSessionConfigDialog::GetBaseMessageMapForMfc,
    &NetSessionConfigDialog::messageEntries[0],
};

/**
 * Reimplements 0x41afd0: NetSessionBrowserDialog::GetMessageMap
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Return the browser dialog MFC message map.
 */
const AFX_MSGMAP * NetSessionBrowserDialog::GetMessageMap() const {
    return &NetSessionBrowserDialog::messageMap;
}

/**
 * Reimplements 0x41afe0: NetSessionBrowserDialog::OnInitDialog
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Initialize the multiplayer session browser controls and providers.
 */
BOOL NetSessionBrowserDialog::OnInitDialog() {
    ((CDialog *)this)->CDialog::OnInitDialog();
    m_playerName = zOpt_GetPlayerName();
    m_shouldEnterHostSetup = FALSE;
    m_sessionCount = 0;

    zNetworkServiceProviderListVec *const providerList =
        zNetworkDPlay::RefreshAndGetServiceProviderList();
    int providerCount = 0;
    if (providerList->begin != 0) {
        providerCount = (int)(providerList->end - providerList->begin);
    }

    HWND providerComboHwnd = m_providerCombo.m_hWnd;
    int providerIndex;
    for (providerIndex = 0; providerIndex < providerCount; ++providerIndex) {
        zNetworkDPlayServiceProviderInfo *const providerInfo = providerList->begin[providerIndex];
        char *const displayName = providerInfo->displayName;
        if (strstr(displayName, g_zNetwork_ProviderName_Ipx) != 0 ||
            strstr(displayName, g_zNetwork_ProviderName_TcpIp) != 0 ||
            strstr(
                displayName,
                g_zNetwork_ProviderName_Modem
            ) != 0) {
            const LRESULT comboIndex =
                ::SendMessageA(
                    providerComboHwnd,
                    CB_ADDSTRING,
                    0,
                    (LPARAM)displayName
                );
            ::SendMessageA(
                providerComboHwnd,
                CB_SETITEMDATA,
                comboIndex,
                (LPARAM)providerInfo
            );
        }
    }

    const LRESULT noProviderIndex = ::SendMessageA(
        providerComboHwnd,
        CB_ADDSTRING,
        0,
        (LPARAM)zLoc::GetMessageString(kNetSessionBrowserNoProviderMessageId)
    );
    ::SendMessageA(
        providerComboHwnd,
        CB_SETITEMDATA,
        noProviderIndex,
        0
    );
    ::SendMessageA(
        providerComboHwnd,
        CB_SETCURSEL,
        0,
        0
    );
    ((CWnd *)&m_okButton)
        ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserJoinButtonMessageId));
    ((CWnd *)this)->UpdateData(FALSE);
    return TRUE;
}

/**
 * Reimplements 0x433a50: GameNetPlayerRow::ApplyPlayerColorTint
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Apply the row's packed multiplayer color to the remote player's
 * primary modal object and force that object visible.
 */
void GameNetPlayerRow::ApplyPlayerColorTint() {
    PlayerModalState *primaryModalState = saveState->primaryModalState;
    const unsigned int packedColor = g_GameNetPlayerRowStyleColors_00RRGGBB[playerColorIndex];
    zColorRgb color = {
        (float)(packedColor & 0xff),
        (float)((packedColor >> 8) & 0xff),
        (float)((packedColor >> 16) & 0xff),
    };

    zClass_Object3D::gwObject3DSetColorAlpha(
        primaryModalState->modalNode,
        &color,
        0.2f
    );
    zClass_Object3D::gwObject3DSetVisibleFlag(
        primaryModalState->modalNode,
        1
    );
}

/**
 * Reimplements 0x434650: GameNetPlayerRow::DestroyEmbeddedPanel
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Destroy the embedded HUD panel owned by a multiplayer player row.
 */
void GameNetPlayerRow::DestroyEmbeddedPanel() {
    hudWidget.~HudUiPanel();
}

/**
 * Reimplements 0x433a40: HudTimerPanelNetState::ClearTailFlagsLocal
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Clear the local HUD timer tail flags before countdown state is
 * copied or reset.
 */
void HudTimerPanelNetState::ClearTailFlagsLocal() {
    {
        for (int index = 0; index < 8; ++index) {
            tailFlags[index] = 0;
        }
    }
}

namespace Net {
/**
 * Reimplements 0x431dd0: Net::InitFromZrd
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Initialize multiplayer mission state from net.zrd spawn points,
 * create the local player row, initialize host HUD timer state, and respawn
 * the local player.
 */
void InitFromZrd() {
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (playerState->lifecycleState == 2) {
            playerState->lifecycleState = 4;
            zClass_NodePartial *const rootNode = playerState->rootNode;
            zClass_Class::RemoveChild(
                rootNode->listA[0],
                rootNode
            );
        }

        saveState = saveState->next;
    }

    zTurret_System::DisableTickCallback();

    /* Retail literal 0x4dcfb4 names the multiplayer spawn-point ZRD; data
       ownership remains blocked outside this source slice. */
    zReader::Node *const treeRoot = zReader::LoadNodeFromPath(
        "net.zrd",
        0,
        0
    );
    if (treeRoot != 0) {
        GameNetReaderArray *const rootArray = (GameNetReaderArray *)(treeRoot->value.ptr);
        GameNetReaderArray *const spawnArray =
            (GameNetReaderArray *)(rootArray->nodes[0].value.ptr);
        int spawnPointCount = spawnArray->count - 1;
        for (int index = 0; index < spawnPointCount; ++index) {
            GameNetSpawnPoint *const spawnPoint =
                (GameNetSpawnPoint *)(::operator new(sizeof(GameNetSpawnPoint)));
            memset(
                spawnPoint,
                0,
                sizeof(GameNetSpawnPoint)
            );

            if (g_GameNetSpawnPointCount == 0) {
                g_GameNetSpawnPointHead = spawnPoint;
            } else {
                g_GameNetSpawnPointTail->next = spawnPoint;
            }
            g_GameNetSpawnPointTail = spawnPoint;
            spawnPoint->next = 0;
            ++g_GameNetSpawnPointCount;

            GameNetReaderArray *const spawnValueArray =
                (GameNetReaderArray *)(spawnArray->nodes[index].value.ptr);
            spawnPoint->position.x = spawnValueArray->nodes[0].value.f32;
            spawnPoint->position.y = spawnValueArray->nodes[1].value.f32;
            spawnPoint->position.z = spawnValueArray->nodes[2].value.f32;
            spawnPoint->yawDegrees = spawnValueArray->nodes[3].value.f32;
        }

        zReader::FreeLoadedTree(treeRoot);
    }

    zUtil_SaveGameState *const localSaveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    GameNetPlayerRow *const playerRow = GameNetPlayerRowList::AppendNewRow(
        &g_GameNetPlayerRowList,
        1
    );
    playerRow->saveState = (GameNetPlayerSaveState *)(localSaveState);
    playerRow->playerKey = zNetwork_GetLocalPlayerKey();
    zNetwork::GetPlayerNameByKey(
        playerRow->playerKey,
        playerRow->displayName,
        sizeof(playerRow->displayName)
    );
    playerRow->playerColorIndex = zNetwork_GetPlayerColorIndexByKey(playerRow->playerKey);

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
            strcpy(
                fatalErrorCaption,
                zLoc::GetMessageString(18)
            );
            strcpy(
                fatalErrorMessage,
                zLoc::GetMessageString(26)
            );
            /* Retail literal 0x4db4ac formats the fatal shutdown caption and
               message; data ownership remains blocked outside this source
               slice. */
            printf(
                "%s: %s\n",
                fatalErrorCaption,
                fatalErrorMessage
            );
            Sleep(1000);
            MessageBeep(MB_ICONHAND);
            MessageBoxA(
                g_RecoilApp_hWndMain,
                fatalErrorMessage,
                fatalErrorCaption,
                MB_ICONHAND
            );
            zSys::ExitProcessWithCleanup(2);
        }
    }

    if (zNetwork::IsHost() != 0) {
        const unsigned int styleColor =
            g_GameNetPlayerRowStyleColors_00RRGGBB[playerRow->playerColorIndex];
        playerRow->playerColorPackedRgb = styleColor;
        SetEmbeddedHudPanelColor(
            playerRow,
            styleColor
        );
        playerRow->ApplyPlayerColorTint();

        if (g_HudSensorTracker.raceCheckpointMode == 0) {
            float runtimeTimerSec;
            memcpy(
                &runtimeTimerSec,
                &g_HudSensorTracker.runtimeTimerSecRaw,
                sizeof(runtimeTimerSec)
            );
            g_GameNetHostHudTimerInitFlag = 0;
            HudUiTimerPanel::SetSeconds(
                runtimeTimerSec,
                -1.0f
            );
            g_HudTimerPanelNetState.timerDirectionNeg = 1;
            g_HudTimerPanelNetState.statusBitsResendDeadline = 30.0f;
        }
    }

    g_HudTimerPanelNetState.timeWarningShown = 0;
    g_HudTimerPanelNetState.oneMinuteWarningShown = 0;
    GameNet::RefreshPlayerListMenu(playerRow);
    localSaveState->netPlayerRow = playerRow;
    GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(
        localSaveState,
        1
    );
    if (g_HudSensorTracker.raceCheckpointMode != 0) {
        GameNet::ResetHudTimerPanelNetStateLongCountdown();
    }

    g_GameNetPkt06InitialSyncGate = 1;
    g_GameNetPkt06NextSendTimeSec = 0.0f;
}

/**
 * Reimplements 0x43cf40: Net::FormatIpv4Address
 * Source: D:\Proj\Battlesport\Net.cpp
 * Purpose: Format a little-endian IPv4 address for session UI text.
 */
void __fastcall FormatIpv4Address(
    char *outText,
    unsigned int ipAddress
) {
    int octets[4];
    int index;
    for (index = 0; index < 4; ++index) {
        octets[index] = (int)(ipAddress & 0xff);
        ipAddress >>= 8;
    }

    sprintf(
        outText,
        /* Retail literal 0x4dd250 is the anonymous compiler string for the
           dotted-quad IPv4 format; data ownership remains blocked outside
           this source slice. */
        "%d.%d.%d.%d",
        octets[0],
        octets[1],
        octets[2],
        octets[3]
    );
}
} // namespace Net

/**
 * Original helper evidence: no standalone retail function; the 0x41ada0
 * Constructor wrapper placement-invokes this owner-shaped C++ constructor so
 * VC5 emits the derived dialog vftable.
 * Purpose: Construct the multiplayer session browser dialog and child controls.
 */
NetSessionBrowserDialog::NetSessionBrowserDialog(
    CWnd *parentWnd
) :
    CDialog(
        kNetSessionBrowserDialogResourceId,
        parentWnd
    ),
    m_playerNameEdit(),
    m_okButton(),
    m_createSessionButton(),
    m_sessionList(),
    m_providerCombo(),
    m_playerName()
{
    m_playerName = "";
}

/**
 * Reimplements 0x41ada0: NetSessionBrowserDialog::Constructor
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Placement-construct the multiplayer session browser dialog owner.
 */
NetSessionBrowserDialog * NetSessionBrowserDialog::Constructor(
    CWnd *parentWnd
) {
    new (this) NetSessionBrowserDialog(parentWnd);
    return this;
}

/**
 * Reimplements 0x41ae90: NetSessionBrowserDialog::ScalarDeletingDestructor
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Destroy the browser dialog and optionally release its storage.
 */
NetSessionBrowserDialog * NetSessionBrowserDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }
    return this;
}

/**
 * Original helper evidence: no standalone retail function; the 0x41aeb0
 * Destructor wrapper invokes this owner-shaped C++ destructor so VC5 emits the
 * derived dialog deleting-destructor slot.
 * Purpose: Let the compiler emit the browser dialog vftable destructor slot.
 */
NetSessionBrowserDialog::~NetSessionBrowserDialog() {
}

/**
 * Reimplements 0x41aeb0: NetSessionBrowserDialog::Destructor
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Invoke the real browser dialog destructor.
 */
void NetSessionBrowserDialog::Destructor() {
    this->NetSessionBrowserDialog::~NetSessionBrowserDialog();
}

/**
 * Reimplements 0x41af50: NetSessionBrowserDialog::DoDataExchange
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Bind browser dialog controls and validate the player-name field.
 */
void NetSessionBrowserDialog::DoDataExchange(
    CDataExchange *dataExchange
) {
    DDX_Control(
        dataExchange,
        kNetSessionBrowserPlayerNameEditId,
        *((CWnd *)&m_playerNameEdit)
    );
    DDX_Control(
        dataExchange,
        kNetSessionBrowserOkButtonId,
        *((CWnd *)&m_okButton)
    );
    DDX_Control(
        dataExchange,
        kNetSessionBrowserCreateSessionButtonId,
        *((CWnd *)&m_createSessionButton)
    );
    DDX_Control(
        dataExchange,
        kNetSessionBrowserSessionListId,
        *((CWnd *)&m_sessionList)
    );
    DDX_Control(
        dataExchange,
        kNetSessionBrowserProviderComboId,
        *((CWnd *)&m_providerCombo)
    );
    DDX_Text(
        dataExchange,
        kNetSessionBrowserPlayerNameEditId,
        m_playerName
    );
    DDV_MaxChars(
        dataExchange,
        m_playerName,
        kNetSessionBrowserPlayerNameMaxChars
    );
}

/**
 * Reimplements 0x41b150: NetSessionBrowserDialog::RefreshSessionList
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Refresh and restore the visible DirectPlay session list.
 */
int NetSessionBrowserDialog::RefreshSessionList() {
    CString selectedSessionText;
    m_sessionCount = zNetwork_DPlay::EnumSessions();

    HWND sessionListHwnd = m_sessionList.m_hWnd;
    const int selectedIndex = (int)(::SendMessageA(
        sessionListHwnd,
        LB_GETCURSEL,
        0,
        0
    ));
    if (selectedIndex != LB_ERR) {
        ((CListBox *)&m_sessionList)->GetText(
            selectedIndex,
            selectedSessionText
        );
    }

    ::SendMessageA(
        sessionListHwnd,
        LB_RESETCONTENT,
        0,
        0
    );
    for (int index = 0; index < m_sessionCount; ++index) {
        int maxPlayers = 0;
        int currentPlayers = 0;
        zNetworkDPlay::GetEnumeratedSessionPlayerCountsByIndex(
            index,
            &currentPlayers,
            &maxPlayers
        );

        char sessionText[120];
        zLoc::FormatMessage(
            sessionText,
            sizeof(sessionText),
            0x112,
            zNetworkDPlay::GetEnumeratedSessionNameByIndex(index),
            maxPlayers,
            currentPlayers
        );

        const int rowIndex =
            (int)(::SendMessageA(
                sessionListHwnd,
                LB_ADDSTRING,
                0,
                (LPARAM)sessionText
            ));
        ::SendMessageA(
            sessionListHwnd,
            LB_SETITEMDATA,
            rowIndex,
            index
        );
    }

    if (selectedIndex != LB_ERR) {
        const int restoredIndex = (int)(::SendMessageA(
            sessionListHwnd,
            LB_FINDSTRINGEXACT,
            (WPARAM)-1,
            (LPARAM)((const char *)selectedSessionText)
        ));
        if (restoredIndex != LB_ERR) {
            ::SendMessageA(
                sessionListHwnd,
                LB_SETCURSEL,
                restoredIndex,
                0
            );
            ((CWnd *)&m_okButton)->EnableWindow(TRUE);
        }
    } else if (m_sessionCount > 0) {
        ::SendMessageA(
            sessionListHwnd,
            LB_SETCURSEL,
            0,
            0
        );
        ((CWnd *)&m_okButton)->EnableWindow(TRUE);
    } else if (zOpt::GetNetworkModemEnabled() == 0) {
        ((CWnd *)&m_okButton)->EnableWindow(FALSE);
    }

    return m_sessionCount;
}

/**
 * Reimplements 0x41b2f0: NetSessionBrowserDialog::ConnectSelectedProvider
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Connect to the selected provider and update browser dialog actions.
 */
void NetSessionBrowserDialog::ConnectSelectedProvider() {
    ::KillTimer(
        m_hWnd,
        2
    );

    HWND providerComboHwnd = m_providerCombo.m_hWnd;
    const LRESULT selectedProviderIndex = ::SendMessageA(
        providerComboHwnd,
        CB_GETCURSEL,
        0,
        0
    );
    if (selectedProviderIndex == CB_ERR) {
        return;
    }

    zNetworkDPlayServiceProviderInfo *providerInfo = (zNetworkDPlayServiceProviderInfo
            *)(::SendMessageA(
                providerComboHwnd,
                CB_GETITEMDATA,
                selectedProviderIndex,
                0
            ));
    if (providerInfo == 0) {
        ((CWnd *)&m_okButton)->EnableWindow(FALSE);
        ((CWnd *)&m_createSessionButton)->EnableWindow(FALSE);
        return;
    }

    if (strstr(
        providerInfo->displayName,
        g_zNetwork_ProviderName_TcpIp
    ) != 0 && g_NetUiTcpIpProviderWarningShown == 0) {
        g_NetUiTcpIpProviderWarningShown = 1;

        char caption[256];
        strcpy(
            caption,
            zLoc::GetMessageString(kNetSessionBrowserTcpIpWarningCaptionMessageId)
        );

        char messageFormat[256];
        strcpy(
            messageFormat,
            zLoc::GetMessageString(kNetSessionBrowserTcpIpWarningFormatMessageId)
        );

        if (NetUi::VerifyWinsock2OrPromptContinue(
            caption,
            messageFormat
        ) == 0) {
            ::SendMessageA(
                providerComboHwnd,
                CB_SETCURSEL,
                0,
                0
            );
            ((CWnd *)&m_okButton)->EnableWindow(FALSE);
            ((CWnd *)&m_createSessionButton)->EnableWindow(FALSE);
            return;
        }
    }

    zNetworkDPlay::SelectServiceProviderAndInitConnection(providerInfo);
    if (strstr(
        providerInfo->displayName,
        g_zNetwork_ProviderName_Modem
    ) != 0) {
        ::SendMessageA(
            m_sessionList.m_hWnd,
            LB_RESETCONTENT,
            0,
            0
        );
        ((CWnd *)&m_okButton)->EnableWindow(TRUE);
        ((CWnd *)&m_okButton)
            ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserModemOkButtonMessageId));
        ((CWnd *)&m_createSessionButton)
            ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserModemCreateButtonMessageId));
        ((CWnd *)&m_createSessionButton)->EnableWindow(TRUE);
        m_selectedProviderIsModem = TRUE;
        return;
    }

    if (RefreshSessionList() >= 0) {
        ::SetTimer(
            m_hWnd,
            2,
            1000,
            0
        );
    }

    ((CWnd *)&m_createSessionButton)->EnableWindow(TRUE);
    ((CWnd *)&m_okButton)
        ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserJoinButtonMessageId));
    ((CWnd *)&m_createSessionButton)
        ->SetWindowTextA(zLoc::GetMessageString(kNetSessionBrowserRefreshButtonMessageId));
    m_selectedProviderIsModem = FALSE;
}

/**
 * Reimplements 0x41b660: NetSessionBrowserDialog::OnTimer
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Poll for updated DirectPlay sessions while the browser is open.
 */
void NetSessionBrowserDialog::OnTimer(
    UINT_PTR
) {
    RefreshSessionList();
    ((GameNetMfcWndAccess *)this)->CallDefault();
}

/**
 * Reimplements 0x41b6a0: NetSessionBrowserDialog::ValidatePlayerName
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Trim, validate, and prompt for the multiplayer player name.
 */
int NetSessionBrowserDialog::ValidatePlayerName() {
    ((CWnd *)this)->UpdateData(TRUE);
    m_playerName.TrimLeft();
    m_playerName.TrimRight();
    ((CWnd *)this)->UpdateData(FALSE);

    if (!m_playerName.IsEmpty()) {
        return TRUE;
    }

    char caption[128];
    strcpy(
        caption,
        zLoc::GetMessageString(kNetSessionBrowserPlayerNameCaptionMessageId)
    );

    char messageText[128];
    strcpy(
        messageText,
        zLoc::GetMessageString(kNetSessionBrowserPlayerNameRequiredMessageId)
    );

    ((CWnd *)this)->MessageBoxA(
        messageText,
        caption,
        MB_ICONHAND
    );
    ((CWnd *)&m_playerNameEdit)->SetFocus();
    return FALSE;
}

/**
 * Reimplements 0x41b510: NetSessionBrowserDialog::OnOK
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Join or initialize the selected multiplayer provider/session.
 */
void NetSessionBrowserDialog::OnOK() {
    int canCloseDialog = FALSE;
    if (ValidatePlayerName() == 0) {
        return;
    }

    if (m_selectedProviderIsModem == 0) {
        zOpt::SetNetworkModemEnabled(FALSE);
        ::KillTimer(
            m_hWnd,
            2
        );

        HWND sessionListHwnd = m_sessionList.m_hWnd;
        const LRESULT selectedSessionRow = ::SendMessageA(
            sessionListHwnd,
            LB_GETCURSEL,
            0,
            0
        );
        if (selectedSessionRow != LB_ERR) {
            m_selectedSessionIndex =
                (int)(::SendMessageA(
                    sessionListHwnd,
                    LB_GETITEMDATA,
                    selectedSessionRow,
                    0
                ));
            canCloseDialog = TRUE;
        }
    } else {
        zOpt::SetNetworkModemEnabled(TRUE);
        if (RefreshSessionList() >= 0) {
            canCloseDialog = TRUE;
            m_selectedSessionIndex = 0;
        }
    }

    if (canCloseDialog != 0) {
        ((GameNetMfcDialogAccess *)this)->CallOnOK();
    }
}

/**
 * Reimplements 0x41b5a0: NetSessionBrowserDialog::OnCreateSession
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Enter host setup or create a modem session from browser state.
 */
void NetSessionBrowserDialog::OnCreateSession() {
    if (ValidatePlayerName() == 0) {
        return;
    }

    if (m_selectedProviderIsModem == 0) {
        ::KillTimer(
            m_hWnd,
            2
        );
        m_shouldEnterHostSetup = TRUE;
    } else {
        zNetworkSessionDescStatusFields statusFields;
        statusFields.eventCode = kNetSessionBrowserModemEventCode;
        statusFields.statusFlags = 0;
        statusFields.valueOrTime = kNetSessionBrowserModemValueOrTime;
        statusFields.auxParam = kNetSessionBrowserModemAuxParam;
        statusFields.maxPlayers = kNetSessionBrowserModemMaxPlayers;
        strcpy(
            statusFields.sessionNameBuf,
            g_zNetwork_ModemSessionName
        );

        if (zNetwork_DPlay::CreateSessionFromStatusFields(&statusFields) != 0) {
            zOpt::SetNetworkEnabled(TRUE);
            zOpt::SetNetworkModemEnabled(TRUE);
            zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(zOpt_GetPlayerName());
            m_shouldEnterHostSetup = TRUE;
        }
    }

    if (m_shouldEnterHostSetup != 0) {
        ((GameNetMfcDialogAccess *)this)->CallOnOK();
    }
}

/**
 * Reimplements 0x41b680: NetSessionBrowserDialog::OnDestroy
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Forward browser dialog destruction and stop session polling.
 */
void NetSessionBrowserDialog::OnDestroy() {
    ((GameNetMfcWndAccess *)this)->CallOnDestroy();
    ::KillTimer(
        m_hWnd,
        2
    );
}

/**
 * Reimplements 0x41b780: NetSessionBrowserDialog::OnHelpDocs
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Open the bundled help document or show the matching shell error.
 */
void NetSessionBrowserDialog::OnHelpDocs() {
    char caption[128];
    strcpy(
        caption,
        zLoc::GetMessageString(kNetSessionBrowserHelpCaptionMessageId)
    );

    char executablePath[256];
    HINSTANCE findResult = FindExecutableA(
        "Docs\\Index.html",
        0,
        executablePath
    );
    const UINT resultCode = (UINT)((UINT_PTR)(findResult));
    if (resultCode <= 31) {
        switch (kNetSessionBrowserHelpDocsFindExecutableErrorClassTable[resultCode]) {
        case 0:
            ((CWnd *)this)
                ->MessageBoxA(
                    zLoc::GetMessageString(kNetSessionBrowserHelpNoAssociationMessageId),
                    caption,
                    MB_ICONEXCLAMATION
                );
            return;

        case 1:
            ((CWnd *)this)
                ->MessageBoxA(
                    zLoc::GetMessageString(kNetSessionBrowserHelpFileNotFoundMessageId),
                    caption,
                    MB_ICONEXCLAMATION
                );
            return;

        case 2:
            ((CWnd *)this)
                ->MessageBoxA(
                    zLoc::GetMessageString(kNetSessionBrowserHelpAssociationIncompleteMessageId),
                    caption,
                    MB_ICONEXCLAMATION
                );
            return;

        case 3:
            ((CWnd *)this)
                ->MessageBoxA(
                    zLoc::GetMessageString(kNetSessionBrowserHelpNoDdeAssociationMessageId),
                    caption,
                    MB_ICONEXCLAMATION
                );
            return;

        default:
            break;
        }
    }

    ShellExecuteA(
        g_RecoilApp_hWndMain,
        "open",
        "Docs\\Index.html",
        0,
        0,
        SW_HIDE
    );
}

/**
 * Reimplements 0x41c6e0: NetSessionConfigDialog::NetSessionConfigDialog
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Construct the multiplayer session configuration dialog controls.
 */
NetSessionConfigDialog::NetSessionConfigDialog(
    CWnd *parentWnd
) :
    CDialog(
        kNetSessionConfigDialogResourceId,
        parentWnd
    ),
    m_maxPlayersSpin(),
    m_valueLimitSpin(),
    m_timeLimitSpin(),
    m_mapCombo(),
    m_sessionName()
{
    m_sessionName = "";
    m_valueLimit = 0;
    m_timeLimitMinutes = 0;
    m_maxPlayers = 0;
    m_unusedCheckboxEnabled = 0;
}

/**
 * Original helper evidence: no standalone retail function; reconstructed
 * callers in this source file still use the recovered constructor helper
 * spelling while the owner model is the real C++ constructor above.
 * Purpose: Placement-construct the config dialog and return self.
 */
NetSessionConfigDialog * NetSessionConfigDialog::Constructor(
    CWnd *parentWnd
) {
    new (this) NetSessionConfigDialog(parentWnd);
    return this;
}

/**
 * Reimplements 0x41c7f0: NetSessionConfigDialog::~NetSessionConfigDialog
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Destroy config dialog owned controls and CString state.
 */
NetSessionConfigDialog::~NetSessionConfigDialog() {
}

/**
 * Original helper evidence: no standalone retail function; reconstructed
 * callers in this source file still use the recovered destructor helper
 * spelling while the owner model is the real C++ destructor above.
 * Purpose: Invoke the real config dialog destructor.
 */
void NetSessionConfigDialog::Destructor() {
    this->NetSessionConfigDialog::~NetSessionConfigDialog();
}

/**
 * Reimplements 0x41c880: NetSessionConfigDialog::DoDataExchange
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Bind config dialog controls and validate session numeric fields.
 */
void NetSessionConfigDialog::DoDataExchange(
    CDataExchange *dataExchange
) {
    DDX_Control(
        dataExchange,
        kNetSessionConfigMaxPlayersSpinId,
        (CWnd &)m_maxPlayersSpin
    );
    DDX_Control(
        dataExchange,
        kNetSessionConfigValueLimitSpinId,
        (CWnd &)m_valueLimitSpin
    );
    DDX_Control(
        dataExchange,
        kNetSessionConfigTimeLimitSpinId,
        (CWnd &)m_timeLimitSpin
    );
    DDX_Control(
        dataExchange,
        kNetSessionConfigMapComboId,
        (CWnd &)m_mapCombo
    );
    DDX_Text(
        dataExchange,
        kNetSessionConfigSessionNameEditId,
        m_sessionName
    );
    DDV_MaxChars(
        dataExchange,
        m_sessionName,
        kNetSessionConfigSessionNameMaxChars
    );
    DDX_Text(
        dataExchange,
        kNetSessionConfigValueLimitEditId,
        m_valueLimit
    );
    DDV_MinMaxUInt(
        dataExchange,
        m_valueLimit,
        0,
        kNetSessionConfigLimitMax
    );
    DDX_Text(
        dataExchange,
        kNetSessionConfigTimeLimitEditId,
        m_timeLimitMinutes
    );
    DDV_MinMaxUInt(
        dataExchange,
        m_timeLimitMinutes,
        0,
        kNetSessionConfigLimitMax
    );
    DDX_Text(
        dataExchange,
        kNetSessionConfigMaxPlayersEditId,
        m_maxPlayers
    );
    DDV_MinMaxUInt(
        dataExchange,
        m_maxPlayers,
        kNetSessionConfigMaxPlayersMin,
        kNetSessionConfigMaxPlayersMax
    );
    DDX_Check(
        dataExchange,
        kNetSessionConfigUnusedCheckboxId,
        m_unusedCheckboxEnabled
    );
}

/**
 * Reimplements 0x41c970: NetSessionConfigDialog::GetMessageMap
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Return the config dialog MFC message map.
 */
const AFX_MSGMAP * NetSessionConfigDialog::GetMessageMap() const {
    return &NetSessionConfigDialog::messageMap;
}

/**
 * Reimplements 0x41ca30: NetSessionConfigDialog::OnInitDialog
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Initialize multiplayer session config fields, maps, and spin ranges.
 */
BOOL NetSessionConfigDialog::OnInitDialog() {
    ((CDialog *)this)->CDialog::OnInitDialog();

    m_sessionName.Format(
        kNetSessionConfigSessionNameFormat,
        m_defaultExerciseOrdinal
    );

    for (int mapIndex = 0; mapIndex < kNetSessionConfigMapNameCount; ++mapIndex) {
        const LRESULT comboItemIndex = ::SendMessageA(
            m_mapCombo.m_hWnd,
            CB_ADDSTRING,
            0,
            (LPARAM)((const char *)g_NetSessionConfigDialog_MapNameStrings[mapIndex])
        );
        ::SendMessageA(
            m_mapCombo.m_hWnd,
            CB_SETITEMDATA,
            comboItemIndex,
            mapIndex
        );
    }

    ::SendMessageA(
        m_mapCombo.m_hWnd,
        CB_SETCURSEL,
        0,
        0
    );
    ::SendMessageA(
        m_timeLimitSpin.m_hWnd,
        kNetSessionConfigSpinSetRangeMessage,
        0,
        MAKELPARAM(360, 0)
    );
    ::SendMessageA(
        m_valueLimitSpin.m_hWnd,
        kNetSessionConfigSpinSetRangeMessage,
        0,
        MAKELPARAM(100, 0)
    );

    LPARAM maxPlayersRange =
        MAKELPARAM(
            kNetSessionConfigMaxPlayersMax,
            kNetSessionConfigMaxPlayersMin
        );
    if (zOpt::GetNetworkModemEnabled() != 0) {
        maxPlayersRange =
            MAKELPARAM(
                kNetSessionConfigMaxPlayersMin,
                kNetSessionConfigMaxPlayersMin
            );
    }
    ::SendMessageA(
        m_maxPlayersSpin.m_hWnd,
        kNetSessionConfigSpinSetRangeMessage,
        0,
        maxPlayersRange
    );

    m_valueLimit = kNetSessionConfigDefaultValueLimit;
    m_timeLimitMinutes = kNetSessionConfigDefaultTimeLimitMinutes;
    m_maxPlayers = kNetSessionConfigDefaultMaxPlayers;
    m_unusedCheckboxEnabled = 1;
    ((CWnd *)this)->UpdateData(FALSE);
    ((CWnd *)this)
        ->SetDlgItemTextA(
            kNetSessionConfigMaxPlayersLabelId,
            zLoc::GetMessageString(kNetSessionConfigMaxPlayersDefaultMessageId)
        );
    return TRUE;
}

/**
 * Reimplements 0x41cb50: NetSessionConfigDialog::OnDestroy
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Persist the selected map index as the config dialog closes.
 */
void NetSessionConfigDialog::OnDestroy() {
    ((GameNetMfcWndAccess *)this)->CallOnDestroy();
    const LRESULT selectedMapComboIndex = ::SendMessageA(
        m_mapCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    m_selectedMapIndex =
        (int) ::SendMessageA(
            m_mapCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedMapComboIndex,
            0
        );
}

/**
 * Reimplements 0x41cb90: NetSessionConfigDialog::OnMapChanged
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Track map selection and refresh max-player label text.
 */
void NetSessionConfigDialog::OnMapChanged() {
    const LRESULT selectedMapComboIndex = ::SendMessageA(
        m_mapCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    const LRESULT selectedMapIndex =
        ::SendMessageA(
            m_mapCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedMapComboIndex,
            0
        );
    m_selectedMapIndex = (int)selectedMapIndex;

    unsigned int messageId = kNetSessionConfigMaxPlayersSpecialMapMessageId;
    if (selectedMapIndex != kNetSessionConfigSpecialMapIndex) {
        messageId = kNetSessionConfigMaxPlayersDefaultMessageId;
    }
    ((CWnd *)this)
        ->SetDlgItemTextA(
            kNetSessionConfigMaxPlayersLabelId,
            zLoc::GetMessageString(messageId)
        );
}

namespace Mission {
/**
 * Reimplements 0x41c980: Mission::RegisterMultiplayerMaps
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Construct and register cleanup for multiplayer map name strings.
 */
void RegisterMultiplayerMaps() {
    NetSessionConfigDialog::InitMapNameStrings();
    NetSessionConfigDialog::RegisterMapNameCleanup();
}
} // namespace Mission

/**
 * Reimplements 0x41c990: NetSessionConfigDialog::InitMapNameStrings
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Construct the seven multiplayer map-name CString entries.
 */
void NetSessionConfigDialog::InitMapNameStrings() {
    new (&g_NetSessionConfigDialog_MapNameStrings[0]) CString("RiverWorks");
    new (&g_NetSessionConfigDialog_MapNameStrings[1]) CString("Crater Chaos");
    new (&g_NetSessionConfigDialog_MapNameStrings[2]) CString("Beach Rally");
    new (&g_NetSessionConfigDialog_MapNameStrings[3]) CString("Clone City");
    new (&g_NetSessionConfigDialog_MapNameStrings[4]) CString("Frozen Tundra");
    new (&g_NetSessionConfigDialog_MapNameStrings[5]) CString("Poison Valley");
    new (&g_NetSessionConfigDialog_MapNameStrings[6]) CString("New Clone City");
}

/**
 * Reimplements 0x41ca00: NetSessionConfigDialog::RegisterMapNameCleanup
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Register process-exit cleanup for multiplayer map-name strings.
 */
void NetSessionConfigDialog::RegisterMapNameCleanup() {
    atexit(&NetSessionConfigDialog::CleanupMapNameStringsOnExit);
}

/**
 * Reimplements 0x41ca10: NetSessionConfigDialog::CleanupMapNameStringsOnExit
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Destroy the static multiplayer map-name CString entries.
 */
void NetSessionConfigDialog::CleanupMapNameStringsOnExit() {
    for (int index = kNetSessionConfigMapNameCount - 1; index >= 0; --index) {
        g_NetSessionConfigDialog_MapNameStrings[index].~CString();
    }
}

namespace GameNet {
/**
 * Reimplements 0x414550: GameNet::ChatComposeKeyCallback
 * Source: D:\Proj\Battlesport\ai_net.cpp
 * Purpose: Append a translated key to active chat-compose text and mirror the
 * buffer into the objective description panel.
 */
void __fastcall ChatComposeKeyCallback(
    int dikCodeWithMods
) {
    const int key = zInput::Keyboard_TranslateDikToAscii(dikCodeWithMods);
    if (key == 0) {
        return;
    }

    g_HudUiMgrObjectiveChatComposeTextInput.DispatchKeyAction(key);

    g_HudUiMgrObjectiveDescTextPanel->SetTextFmt(
        g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer()
    );
}

/**
 * Reimplements 0x4143d0: GameNet::BeginChatCompose
 * Source: D:\Proj\Battlesport\ai_net.cpp
 * Purpose: Open chat-compose mode and bind text-entry keys.
 */
void BeginChatCompose() {
    if (zOpt::GetNetworkEnabled() == 0) {
        return;
    }

    HudUiMgrObjective::Show(
        0,
        g_HudUiMessage_NodeName,
        "",
        0.0f
    );
    g_HudUiMgrObjectiveChatComposeActive = 1;
    g_HudUiMgrObjectiveChatComposeTextInput.AllocTextBuffer(kGameNetChatComposeTextCapacity);
    g_HudUiMgrObjectiveChatComposeTextInput.SetContents("");
    zInput::BindMapContext_Push(0);

    RegisterChatComposeKeyRange(
        kGameNetChatComposeDigitFirstDik,
        kGameNetChatComposeDigitLastDik
    );
    RegisterChatComposeKeyRange(
        kGameNetChatComposeLetterRowFirstDik,
        kGameNetChatComposeLetterRowLastDik
    );
    RegisterChatComposeKeyRange(
        kGameNetChatComposeHomeRowFirstDik,
        kGameNetChatComposeHomeRowLastDik
    );
    RegisterChatComposeKeyRange(
        kGameNetChatComposeBottomRowFirstDik,
        kGameNetChatComposeBottomRowLastDik
    );
    RegisterChatComposeKey(kGameNetChatComposeSpaceDik);
}

/**
 * Reimplements 0x414590: GameNet::EndChatComposeAndSend
 * Source: D:\Proj\Battlesport\ai_net.cpp
 * Purpose: Close chat compose, show the local chat line, and send packet 0x0b.
 */
void EndChatComposeAndSend() {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    GameNetPlayerRow *const playerRow = saveState->netPlayerRow;
    char chatLine[0x51];
    chatLine[0x50] = '\0';

    g_HudUiMgrObjectiveChatComposeActive = 0;
    zInput::BindMapContext_Pop();
    HudUiMgrObjective::Begin();

    if (strlen(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer()) == 0) {
        return;
    }

    strncpy(
        chatLine,
        playerRow->displayName,
        0x50
    );
    strncat(
        chatLine,
        g_HudUiMessage_SeparatorColon,
        0x50 - strlen(chatLine)
    );
    strncat(
        chatLine,
        g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(),
        0x50 - strlen(chatLine)
    );
    HudUi::ShowChatLine(
        chatLine,
        5.0f
    );
    SendPkt0B_ChatMessage(chatLine);
}

/**
 * Reimplements 0x414660: GameNet::EndChatComposeAndSendThunk
 * Source: D:\Proj\Battlesport\ai_net.cpp
 * Purpose: Forward the chat-compose dispatch callback to EndChatComposeAndSend.
 */
void EndChatComposeAndSendThunk() {
    EndChatComposeAndSend();
}

/**
 * Reimplements 0x432830: GameNet::FindPlayerRowByKey
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Find the active GameNet remote-player row for a network player key.
 */
GameNetPlayerRow *__fastcall FindPlayerRowByKey(
    int playerKey
) {
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        if (row->playerKey == playerKey) {
            return row;
        }

        row = row->next;
    }

    return 0;
}

/**
 * Reimplements 0x4336f0: GameNet::GetLocalPlayerColorIndexOrZero
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Return the local GameNet player-row color index when the local
 * save-state row is available, or zero otherwise.
 */
int GetLocalPlayerColorIndexOrZero() {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    if (saveState == 0) {
        return 0;
    }

    GameNetPlayerRow *const netPlayerRow = saveState->netPlayerRow;
    if (netPlayerRow == 0) {
        return 0;
    }

    return netPlayerRow->playerColorIndex;
}

/**
 * Reimplements 0x4339d0: GameNet::GetNearestOtherPlayerDistanceToSpawnPoint
 * Source: D:\Proj\Battlesport\net.cpp
 * Purpose: Measure the nearest player-row save state other than the active
 * game-state-table row for a candidate multiplayer spawn point.
 */
float __fastcall GetNearestOtherPlayerDistanceToSpawnPoint(
    GameNetSpawnPoint *spawnPoint,
    GameNetPlayerSaveState **outSaveState
) {
    float nearestDistanceSq = 1.0e23f;
    GameNetPlayerSaveState *const localSaveState =
        (GameNetPlayerSaveState *)(g_GameStateOrMapTable);
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        GameNetPlayerSaveState *const saveState = row->saveState;
        if (saveState != localSaveState) {
            const float distanceSq =
                zMath::Vec3DeltaLengthSq(
                    &saveState->playerState->worldPos,
                    &spawnPoint->position
                );
            if (distanceSq < nearestDistanceSq) {
                nearestDistanceSq = distanceSq;
                *outSaveState = saveState;
            }
        }

        row = row->next;
    }

    return nearestDistanceSq;
}

/**
 * Reimplements 0x433200: GameNet::AreAllPlayersAtLapTarget
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Mark the multiplayer lap-target check as started and report
 * whether every player row has reached the race goal.
 */
int AreAllPlayersAtLapTarget() {
    if (g_GameNetAllPlayersLapTargetCheckStarted == 0) {
        g_GameNetAllPlayersLapTargetCheckStarted = 1;
    }

    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        if (row->lapCount < g_HudSensorTracker.runtimeGoalValue) {
            return 0;
        }

        row = row->next;
    }

    return 1;
}

/**
 * Reimplements 0x433840: GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Choose a multiplayer respawn point, optionally drop the player's
 * current weapon pickup, reset transient player state, and refresh mission
 * vehicle unlock flags.
 */
void __fastcall RespawnPlayerAndDropWeaponPickupIfAllowed(
    zUtil_SaveGameState *saveState,
    int useColorIndexedSpawn
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const int localColorIndex = GetLocalPlayerColorIndexOrZero();
    GameNetSpawnPoint *spawnPoint = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *selectedSpawn = spawnPoint;

    if (useColorIndexedSpawn != 0) {
        if (localColorIndex > 1) {
            int colorIndex = 1;
            while (spawnPoint != 0 && colorIndex < localColorIndex) {
                spawnPoint = spawnPoint->next;
                ++colorIndex;
            }
        }
        selectedSpawn = spawnPoint;
    } else if (g_HudSensorTracker.raceCheckpointMode != 0) {
        selectedSpawn = 0;
    } else {
        PickupType *const pickupType = Pickup::FindDroppableTypeForPlayerCurrentWeapon(saveState);
        PickupParsedZrdEntry entry = {0};
        entry.typeDesc = pickupType;
        entry.amount = pickupType->defaultAmount;
        entry.position = playerState->worldPos;
        entry.rotation = playerState->vehicleRotationAngles;
        PickupSpawnDef *const pickupSpawn = Pickup::SpawnFromParsedZrdEntry(&entry);
        if (pickupSpawn != 0) {
            Pickup::SendPkt11_CreateDelta(pickupSpawn);
        }

        selectedSpawn = 0;
        float bestNearestDistanceSq = 0.0f;
        GameNetPlayerSaveState *nearestSaveState = 0;
        while (spawnPoint != 0) {
            const float nearestDistanceSq =
                GetNearestOtherPlayerDistanceToSpawnPoint(
                    spawnPoint,
                    &nearestSaveState
                );
            if (nearestDistanceSq > bestNearestDistanceSq) {
                bestNearestDistanceSq = nearestDistanceSq;
                selectedSpawn = spawnPoint;
            }
            spawnPoint = spawnPoint->next;
        }
    }

    if (selectedSpawn != 0) {
        const double kDegreesToRadians = 0.017453292519943295;
        const float yawRad = (float)(selectedSpawn->yawDegrees * kDegreesToRadians);
        Player::SetWorldPoseAndRestartAnchor(
            saveState,
            &selectedSpawn->position,
            yawRad
        );
    }

    if (saveState->primaryModalState->masterModalData->masterType != 3 &&
        g_HudSensorTracker.raceCheckpointMode == 0) {
        Player::TransitionToMasterTypeTrack(
            saveState,
            1
        );
    }

    Player::ResetMouseControlStateAndRecenterCursor(saveState);
    Player::ResetMotionTransientState(saveState);
    playerState->amphibUnlocked =
        Player::IsMissionProbeType1EnabledById(g_HudSensorTracker.GetMissionId());
    playerState->hoverUnlocked = 0;
    playerState->subUnlocked = 0;
}

/**
 * Reimplements 0x432300: GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Replicate the local pkt06 player-state snapshot and drive host HUD
 * timer warning/status packet updates.
 */
int __fastcall TickLocalPlayerPkt06ReplicationAndHudTimer(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;

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

    g_GameNetPkt06NextSendTimeSec = g_Time_AccumulatedTimeSec + kGameNetPkt06SendIntervalSec;

    NetPkt06_PlayerStateSnapshot *const packet = &g_NetPkt06_PlayerStateSnapshotBuf;
    packet->header.packetType = 0x06;
    packet->header.packetSizeBytes = 0x44;
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->cachedAltSelectionCode = (short)(playerState->cachedAltSelectionCode);
    packet->cachedPrimarySelectionCode = (short)(playerState->cachedPrimarySelectionCode);

    unsigned int packedFlags = packet->packedMasterTypeColorFlags;
    packedFlags = (packedFlags & ~0xffu) |
                  ((unsigned int)(primaryModalState->masterModalData->masterType) & 0xffu);
    packedFlags = (packedFlags & ~0xff00u) |
                  (((unsigned int)(GetLocalPlayerColorIndexOrZero()) & 0xffu) << 8);
    if ((g_GameNetPkt06InputBit16Latch & 1) != 0) {
        packedFlags |= 0x10000u;
    } else {
        packedFlags &= ~0x10000u;
    }
    g_GameNetPkt06InputBit16Latch = 0;
    if ((g_GameNetPkt06InputBit17Latch & 1) != 0) {
        packedFlags |= 0x20000u;
    } else {
        packedFlags &= ~0x20000u;
    }
    g_GameNetPkt06InputBit17Latch = 0;

    packet->altGunAimOrigin = playerState->altGunAimOrigin;
    packet->storedTargetPos = playerState->storedTargetPos;
    packet->worldPos = playerState->worldPos;
    packet->vehicleRotationAngles = playerState->vehicleRotationAngles;
    packet->statusMeterValue = playerState->statusMeterValue;

    if (playerState->progressTargetCount > 0) {
        packedFlags |= 0x40000u;
        packet->header.packetSizeBytes =
            (short)(0x44 + 4 + playerState->progressTargetCount * sizeof(zVec3));
        packet->progressTargetCount = playerState->progressTargetCount;
        GameNetCopyPkt06ProgressTargets(
            packet,
            playerState
        );
    } else {
        packedFlags &= ~0x40000u;
    }
    packet->packedMasterTypeColorFlags = packedFlags;

    const int sendResult = zNetwork_SendPacketUnreliable(&packet->header);
    const int raceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;
    if (zNetwork::IsHost() != 0) {
        if (raceCheckpointMode != 0) {
            HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
            const float timerSeconds = HudUiTimerPanel::GetSeconds();
            timerState.startCountdownTriggered = 0;
            timerState.timerSeconds = timerSeconds;

            if (timerState.startGateTriggered == 0) {
                if (timerSeconds <= 0.0f) {
                    timerState.startGateTriggered = 1;
                    timerState.timerDirectionNeg = 0;
                    timerState.timerSeconds = 0.0f;
                    /**
                     * Reimplements data 0x4dcfbc: GameNet startgate effect literal.
                     * Data owner gate remains pending; this docblock records
                     * source provenance only.
                     * Purpose: name the replicated start-gate effect animation
                     * stopped when the host race countdown reaches zero.
                     */
                    zEffectAnim::SetVelocity_Thunk(
                        zEffectAnim::FindEntryByName("startgate"),
                        0,
                        0.0f,
                        0.0f,
                        0.0f
                    );
                    SendPkt0D_HudTimerPanelState(&timerState);
                } else if (g_HudTimerPanelNetState.startCountdownTriggered == 0 &&
                           g_HudTimerPanelNetState.tenSecondWarningsEnabled != 0 &&
                           timerSeconds <= kGameNetHudTimerTenSecondThreshold) {
                    timerState.timerSeconds = kGameNetHudTimerTenSecondThreshold;
                    HudUiTimerPanel::SetSeconds(
                        g_FrameDeltaTimeSec + kGameNetHudTimerTenSecondThreshold,
                        -1.0f
                    );
                    timerState.startCountdownTriggered = 1;
                    SendPkt0D_HudTimerPanelState(&timerState);
                } else if (timerSeconds > kGameNetHudTimerTenSecondThreshold &&
                           (int)(timerSeconds) % 10 == 0) {
                    if (g_GameNetHudTimerTenSecondWarningArmed != 0) {
                        GameNetShowPairedTimerMessages(
                            0x32,
                            0x31
                        );
                        g_GameNetHudTimerTenSecondWarningArmed = 0;
                    }
                } else {
                    g_GameNetHudTimerTenSecondWarningArmed = 1;
                }
            }

            if (timerState.raceFinishCountdownTriggered != 0 && timerState.timeWarningShown == 0) {
                timerState.timeWarningShown = 1;
                SendPkt0D_HudTimerPanelState(&timerState);
                return sendResult;
            }
        } else {
            const float timerSeconds = HudUiTimerPanel::GetSeconds();
            g_HudTimerPanelNetState.timerSeconds = timerSeconds;
            HudTimerPanelNetState timerState = g_HudTimerPanelNetState;

            if (timerState.oneMinuteWarningShown == 0 &&
                timerSeconds < g_FrameDeltaTimeSec + kGameNetHudTimerOneMinuteLeadSec) {
                timerState.oneMinuteWarningShown = 1;
                SendPkt0C_HudTimerStatusBits(&timerState);
            }

            if (g_HudTimerPanelNetState.timeWarningShown == 0 &&
                timerSeconds < g_FrameDeltaTimeSec) {
                timerState.timeWarningShown = 1;
                SendPkt0C_HudTimerStatusBits(&timerState);
            }

            if (g_Time_AccumulatedTimeSec > g_HudTimerPanelNetState.statusBitsResendDeadline) {
                SendPkt0C_HudTimerStatusBits(&timerState);
                return sendResult;
            }
        }

        return sendResult;
    }

    if (raceCheckpointMode != 0) {
        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (timerState.startGateTriggered != 0 || timerState.startCountdownTriggered != 0) {
            g_GameNetHudTimerPendingSaveReminderArmed = 1;
        } else if ((int)(HudUiTimerPanel::GetSeconds()) % 10 != 0) {
            g_GameNetHudTimerPendingSaveReminderArmed = 1;
        } else if (g_GameNetHudTimerPendingSaveReminderArmed != 0) {
            GameNetShowPairedTimerMessages(
                0x34,
                0x33
            );
            g_GameNetHudTimerPendingSaveReminderArmed = 0;
            return sendResult;
        }
    }

    return sendResult;
}

/**
 * Reimplements 0x432ae0: GameNet::ApplyPkt06_PlayerStateSnapshotToRow
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Apply a replicated player-state snapshot packet to an existing
 * remote-player row and its save-state storage.
 */
int __fastcall ApplyPkt06_PlayerStateSnapshotToRow(
    GameNetPlayerRow *row,
    NetPkt06_PlayerStateSnapshot *packet
) {
    GameNetPlayerSaveState *const rowSaveState = row->saveState;
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)rowSaveState;
    zUtil_PlayerStateStorage *const playerState = rowSaveState->playerState;
    PlayerMasterModalData *const masterModalData = rowSaveState->primaryModalState->masterModalData;
    const unsigned int packedFlags = packet->packedMasterTypeColorFlags;

    playerState->netUpdateReceived = 1;

    const int colorIndex = (int)((packedFlags >> 8) & 0xffu);
    if (row->playerColorIndex != colorIndex) {
        row->playerColorIndex = colorIndex;
        const unsigned int packedColor = g_GameNetPlayerRowStyleColors_00RRGGBB[colorIndex];
        row->playerColorPackedRgb = packedColor;
        SetEmbeddedHudPanelColor(
            row,
            packedColor
        );
        HudUi::RefreshScoreboardEntryRow(row);
        row->ApplyPlayerColorTint();
    }

    const int masterType = (int)(packedFlags & 0xffu);
    if (masterType != masterModalData->masterType) {
        Player::ApplyMasterTypeTransition(
            saveState,
            masterType,
            0
        );
    }

    playerState->netReceivedPos = packet->worldPos;
    playerState->netReceivedAngles = packet->vehicleRotationAngles;

    const int inputBit16 = (packedFlags & kGameNetPkt06InputBit16Flag) != 0 ? 1 : 0;
    const int inputBit17 = (packedFlags & kGameNetPkt06InputBit17Flag) != 0 ? 1 : 0;
    if (playerState->netLastUpdateFrameTick == g_zVideo_FrameTick) {
        playerState->netInputBit16Latch |= inputBit16;
        playerState->netInputBit17Latch |= inputBit17;
    } else {
        playerState->netInputBit16Latch = inputBit16;
        playerState->netInputBit17Latch = inputBit17;
        playerState->netLastUpdateFrameTick = g_zVideo_FrameTick;
    }

    playerState->storedTargetPos = packet->storedTargetPos;

    const int altSelectionCode = (int)((unsigned short)(packet->cachedAltSelectionCode));
    if (altSelectionCode != playerState->cachedAltSelectionCode) {
        Player::ApplyAltWeaponSwitch(
            saveState,
            playerState->activeAltGunController,
            GameNetPkt06DecodeWeaponController(playerState, altSelectionCode)
        );
    }

    const int primarySelectionCode = (int)((unsigned short)(packet->cachedPrimarySelectionCode));
    if (primarySelectionCode != playerState->cachedPrimarySelectionCode) {
        Player::ApplyPrimaryWeaponSwitch(
            saveState,
            playerState->activePrimaryGunController,
            GameNetPkt06DecodeWeaponController(playerState, primarySelectionCode)
        );
    }

    playerState->statusMeterValue = packet->statusMeterValue;
    for (int index = 0; index < 10; ++index) {
        playerState->progressTargetRuntimeSlots[index].targetPos = 0;
    }

    if ((packedFlags & kGameNetPkt06ProgressTargetsFlag) == 0) {
        playerState->progressTargetCount = 0;
        return 1;
    }

    playerState->progressTargetCount = packet->progressTargetCount;
    for (int progressIndex = 0; progressIndex < playerState->progressTargetCount; ++progressIndex) {
        playerState->progressTargetPointStorage[progressIndex] =
            packet->progressTargetPoints[progressIndex];
        playerState->progressTargetRuntimeSlots[progressIndex].targetPos =
            &playerState->progressTargetPointStorage[progressIndex];
    }

    return 1;
}

/**
 * Reimplements 0x432860: GameNet::SpawnRemotePlayerFromPkt06_PlayerStateSnapshot
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Create a remote player row and cloned player node from an incoming
 * player-state snapshot packet.
 */
int __fastcall SpawnRemotePlayerFromPkt06_PlayerStateSnapshot(
    int senderPlayerId,
    NetPkt06_PlayerStateSnapshot *packet
) {
    char displayNameScratch[0x40];
    if (zNetwork::GetPlayerNameByKey(
            senderPlayerId,
            displayNameScratch,
            sizeof(displayNameScratch)
        ) == 0) {
        zNetwork_DPlay::EnumPlayers();
    }

    zClass_NodePartial *const sourceNode = zClass::FindByTypeAndName(
        6,
        "bft_99"
    );
    zClass_NodePartial *clonedNode = 0;
    if (sourceNode != 0) {
        clonedNode = zClass_cls_util::CopyNodeWithCloneOptions(
            sourceNode,
            1,
            1
        );
    }

    if (clonedNode == 0) {
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x911),
            5.0f
        );
        return 0;
    }

    clonedNode->flags |= kGameNetRemoteCloneNodeFlag;

    char netNodeName[0x14];
    sprintf(
        netNodeName,
        "net%d",
        packet->header.payloadDword0
    );
    zClass_Class::gwNodeSetName(
        clonedNode,
        netNodeName
    );

    zUtil_SaveGameState *const saveState = Player::CreateFromNamesAtPoseGetState(
        &packet->worldPos,
        g_Player_NodeName_Bft,
        packet->vehicleRotationAngles.y,
        netNodeName
    );
    if (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        playerState->lifecycleState = 3;
        playerState->amphibUnlocked = 1;
        playerState->hoverUnlocked = 1;
        playerState->subUnlocked = 1;
        GameNetUnlockRemotePlayerWeaponBanks(playerState);
    }

    GameNetPlayerRowListState *const rowList = &g_GameNetPlayerRowList;
    GameNetPlayerRow *const row = GameNetPlayerRowList::AppendNewRow(
        rowList,
        0
    );
    row->playerKey = packet->header.payloadDword0;
    row->playerColorIndex = (int)((packet->packedMasterTypeColorFlags >> 8) & 0xffu);
    row->playerNode = clonedNode;
    row->score = 0;
    row->lapCount = 0;
    row->turretNode = zClass_Class::FindSubNodeByName(
        clonedNode,
        g_Player_NodeName_Turret
    );
    row->gunNode = zClass_Class::FindSubNodeByName(
        clonedNode,
        "gun"
    );
    row->saveState = (GameNetPlayerSaveState *)saveState;

    if (zNetwork::GetPlayerNameByKey(senderPlayerId, row->displayName, sizeof(row->displayName)) !=
        0) {
        HudUi::ShowTopMessageLine(
            row->displayName,
            5.0f
        );
    }
    HudUi::ShowTopMessageLine(
        zLoc::GetMessageString(0x912),
        5.0f
    );

    row->hudWidget.SetText(row->displayName);
    SetEmbeddedHudPanelColor(
        row,
        g_GameNetPlayerRowStyleColors_00RRGGBB[0]
    );
    GameNetSetRemoteHudVisible(
        &row->hudWidget,
        0
    );
    g_HudUiTopMessageStack->AddChild((HudUiElement *)(&row->hudWidget));

    if (saveState != 0) {
        saveState->netPlayerRow = row;
        if (row->playerNode->listCountA == 0) {
            zClass_Class::AddChild(
                g_Player_RuntimeDiScene,
                row->playerNode
            );
        }
        zClass_Class::gwNodeSetActive(
            row->playerNode,
            1
        );
    }

    RefreshPlayerListMenu(row);
    ReassignPlayerColorsAndRefreshRows(
        0,
        0
    );

    if (zNetwork::IsHost() != 0) {
        SendPkt09_PlayerScoreboardSnapshot();
        zDEClient::DispatchFeatureEventTemplates(
            HostSendPkt0F_CraterFeature,
            HostSendPkt10_QSandFeature
        );
        SendAllPkt13_EffectAnimActivationRecords();
        Pickup::ReconcilePrimaryAndNetworkCopySpawnLists();

        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            timerState.startCountdownTriggered = 0;
            if (g_HudTimerPanelNetState.startGateTriggered != 0) {
                timerState.ClearTailFlagsLocal();
            }
            SendPkt0D_HudTimerPanelState(&timerState);
        } else {
            SendPkt0C_HudTimerStatusBits(&timerState);
        }
    }

    ApplyPkt06_PlayerStateSnapshotToRow(
        row,
        packet
    );
    return 1;
}

/**
 * Reimplements 0x4327e0: GameNet::HandlePkt06_PlayerStateSnapshot
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Dispatch an incoming player-state snapshot to row creation or
 * existing-row update handling.
 */
int __fastcall HandlePkt06_PlayerStateSnapshot(
    int senderPlayerId,
    NetPkt06_PlayerStateSnapshot *packet
) {
    if (packet == 0) {
        return -1;
    }

    GameNetPlayerRow *const row = FindPlayerRowByKey(packet->header.payloadDword0);
    if (g_GameNetPkt06InitialSyncGate != 0) {
        g_GameNetPkt06InitialSyncGate = 0;
    }

    if (packet->header.packetType == 6) {
        if (row == 0) {
            SpawnRemotePlayerFromPkt06_PlayerStateSnapshot(
                senderPlayerId,
                packet
            );
            return 0;
        }

        ApplyPkt06_PlayerStateSnapshotToRow(
            row,
            packet
        );
    }

    return 0;
}

/**
 * Reimplements 0x434190: GameNet::HandlePkt07_AltGunDispatch
 * Source path: src/Battlesport/GameNet.cpp.
 * BN source path: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp.
 * Purpose: apply a remote pkt07 alternate-gun dispatch to the matching player
 * row.
 */
int __fastcall HandlePkt07_AltGunDispatch(
    int,
    NetPkt07_AltGunDispatch *packet
) {
    GameNetPlayerRow *const row = FindPlayerRowByKey(packet->header.payloadDword0);
    if (row == 0) {
        return 0;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)row->saveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    playerState->altGunDispatchFlags =
        (int)(packet->dispatchFlags | kGameNetRemoteAltGunDispatchFlag);
    playerState->storedTargetPos = packet->targetPos;

    PlayerGunFireController *const oldActiveAltGunController = playerState->activeAltGunController;
    playerState->activeAltGunController =
        Player::FindAltGunFireControllerForWeaponId(
            saveState,
            (int)(packet->weaponId)
        );

    OptCatalog::SetPendingSpawnTargetOverrides(
        &playerState->progressTargetCount,
        playerState->progressTargetSlots
    );
    Player::ProcessAltGunDispatchRequest(saveState);

    playerState->altGunDispatchFlags = 0;
    playerState->activeAltGunController = oldActiveAltGunController;
    OptCatalog::SetPendingSpawnTargetOverrides(
        0,
        0
    );
    return 1;
}

/**
 * Reimplements 0x434130: GameNet::SendPkt07_AltGunDispatch
 * Source path: src/Battlesport/GameNet.cpp.
 * BN source path: D:\Proj\Battlesport\ai_net.cpp.
 * Purpose: send the local alternate-gun dispatch packet to peers.
 */
void __fastcall SendPkt07_AltGunDispatch(
    short weaponId,
    unsigned int dispatchFlags
) {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    g_NetPkt07_AltGunDispatchBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt07_AltGunDispatchBuf.weaponId = weaponId;
    g_NetPkt07_AltGunDispatchBuf.dispatchFlags = dispatchFlags;
    g_NetPkt07_AltGunDispatchBuf.targetPos = playerState->storedTargetPos;
    zNetwork_SendPacketReliable(&g_NetPkt07_AltGunDispatchBuf.header);
}

/**
 * Reimplements 0x434230: GameNet::AltGunDispatchNoOpCallback
 * Source path: src/Battlesport/GameNet.cpp.
 * BN source path: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp.
 * Purpose: accept remote alternate-gun runtime allocation without local side
 * effects.
 */
int __fastcall AltGunDispatchNoOpCallback(
    OptCatalogEntryDef *,
    void **
) {
    return 1;
}

/**
 * Reimplements 0x433ca0: GameNet::SendPkt10_QSandEvent
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: relay local quicksand feature events through packet 0x10 after
 * validating local damage ownership.
 */
int __fastcall SendPkt10_QSandEvent(
    zDEClient_QSandEventTemplate *eventTemplate
) {
    if (eventTemplate->radius <= 0.0f) {
        eventTemplate->radius = -eventTemplate->radius;
        return 1;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    if (eventTemplate->damageOwnerNode != saveState->playerState->rootNode) {
        return 0;
    }

    g_NetPkt10_QSandEventRelayBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt10_QSandEventRelayBuf.center = eventTemplate->center;
    g_NetPkt10_QSandEventRelayBuf.radius = eventTemplate->radius;
    g_NetPkt10_QSandEventRelayBuf.eventFlags &= 0xffff0000u;

    if (zNetwork::IsHost() != 0) {
        zDEClient_QSand::NetRelayCallback(
            zNetwork_GetLocalPlayerKey(),
            &g_NetPkt10_QSandEventRelayBuf
        );
        return 0;
    }

    zNetwork_SendPacketReliable(&g_NetPkt10_QSandEventRelayBuf.header);
    return 0;
}

/**
 * Reimplements 0x433de0: GameNet::HostSendPkt10_QSandFeature
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Relay a host-authored quicksand feature event to network peers.
 */
int __fastcall HostSendPkt10_QSandFeature(
    zDEClient_QSandEventTemplate *eventTemplate
) {
    if (zNetwork::IsHost() == 0) {
        return 0;
    }

    g_NetPkt10_QSandEventSendBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt10_QSandEventSendBuf.center = eventTemplate->center;
    g_NetPkt10_QSandEventSendBuf.eventFlags |= 0x80u;
    g_NetPkt10_QSandEventSendBuf.radius = eventTemplate->radius;
    zNetwork_SendPacketReliable(&g_NetPkt10_QSandEventSendBuf.header);
    return 1;
}

/**
 * Reimplements 0x433c30: GameNet::HostSendPkt0F_CraterFeature
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Relay a host-authored crater feature event to network peers.
 */
int __fastcall HostSendPkt0F_CraterFeature(
    zDEClient_CraterEventTemplate *eventTemplate
) {
    if (zNetwork::IsHost() == 0) {
        return 0;
    }

    g_NetPkt0F_CraterEventSendBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt0F_CraterEventSendBuf.craterTypeId =
        zModel_MatlSlot::IndexFromPtrOrMinus1(eventTemplate->craterMaterialSlot);
    g_NetPkt0F_CraterEventSendBuf.center = eventTemplate->center;
    g_NetPkt0F_CraterEventSendBuf.eventFlags |= 0x80u;
    g_NetPkt0F_CraterEventSendBuf.radius = eventTemplate->radius;
    zNetwork_SendPacketReliable(&g_NetPkt0F_CraterEventSendBuf.header);
    return 1;
}

/**
 * Reimplements 0x4321b0: GameNet::UnregisterGameplayPacketHandlers
 * Source: D:\Proj\Battlesport\gamenet.cpp
 * Purpose: Remove all gameplay packet handlers registered with zNetwork.
 */
void UnregisterGameplayPacketHandlers() {
    zNetwork::UnregisterPacketHandler(
        6,
        (zNetworkPacketHandler)&HandlePkt06_PlayerStateSnapshot
    );
    zNetwork::UnregisterPacketHandler(
        7,
        (zNetworkPacketHandler)&HandlePkt07_AltGunDispatch
    );
    zNetwork::UnregisterPacketHandler(
        0x0a,
        (zNetworkPacketHandler)&OptCatalog::HandlePkt0A_RemoveRuntimeRelay
    );
    zNetwork::UnregisterPacketHandler(
        1,
        (zNetworkPacketHandler)&ReassignPlayerColorsAndRefreshRows
    );
    zNetwork::UnregisterPacketHandler(
        8,
        (zNetworkPacketHandler)&HandlePkt08_PlayerKillEvent
    );
    zNetwork::UnregisterPacketHandler(
        9,
        (zNetworkPacketHandler)&HandlePkt09_PlayerScoreboardSnapshot
    );
    zNetwork::UnregisterPacketHandler(
        0x0b,
        (zNetworkPacketHandler)&HandlePkt0B_ChatMessage
    );
    zNetwork::UnregisterPacketHandler(
        0x0e,
        (zNetworkPacketHandler)&HandlePkt0E_PlayerLapProgress
    );
    zNetwork::UnregisterPacketHandler(
        0x0c,
        (zNetworkPacketHandler)&HandlePkt0C_HudTimerStatusBits
    );
    zNetwork::UnregisterPacketHandler(
        0x0d,
        (zNetworkPacketHandler)&HandlePkt0D_HudTimerPanelState
    );
    zNetwork::UnregisterPacketHandler(
        0x0f,
        (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback
    );
    zNetwork::UnregisterPacketHandler(
        0x10,
        (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback
    );
    zNetwork::UnregisterPacketHandler(
        0x11,
        (zNetworkPacketHandler)&Pickup::HandlePkt11_SpawnDelta
    );
    zNetwork::UnregisterPacketHandler(
        0x12,
        (zNetworkPacketHandler)&Pickup::HandlePkt12_AirdropSpawnChuteRelay
    );
    zNetwork::UnregisterPacketHandler(
        0x13,
        (zNetworkPacketHandler)&HandlePkt13_EffectAnimActivationRecord
    );
    g_GameNet_HandlersRegistered = 0;
}

/**
 * Reimplements 0x431c50: GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Register gameplay packet handlers and option catalog callbacks once.
 */
void RegisterGameplayHandlersAndOptCatalogCallbacks() {
    if (g_GameNet_HandlersRegistered == 0) {
        zNetwork::RegisterPacketHandler(
            6,
            (zNetworkPacketHandler)&HandlePkt06_PlayerStateSnapshot,
            2
        );
        zNetwork::RegisterPacketHandler(
            7,
            (zNetworkPacketHandler)&HandlePkt07_AltGunDispatch,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0a,
            (zNetworkPacketHandler)&OptCatalog::HandlePkt0A_RemoveRuntimeRelay,
            2
        );
        zNetwork::RegisterPacketHandler(
            1,
            (zNetworkPacketHandler)&ReassignPlayerColorsAndRefreshRows,
            2
        );
        zNetwork::RegisterPacketHandler(
            8,
            (zNetworkPacketHandler)&HandlePkt08_PlayerKillEvent,
            2
        );
        zNetwork::RegisterPacketHandler(
            9,
            (zNetworkPacketHandler)&HandlePkt09_PlayerScoreboardSnapshot,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0b,
            (zNetworkPacketHandler)&HandlePkt0B_ChatMessage,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0e,
            (zNetworkPacketHandler)&HandlePkt0E_PlayerLapProgress,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0c,
            (zNetworkPacketHandler)&HandlePkt0C_HudTimerStatusBits,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0d,
            (zNetworkPacketHandler)&HandlePkt0D_HudTimerPanelState,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x0f,
            (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x10,
            (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x11,
            (zNetworkPacketHandler)&Pickup::HandlePkt11_SpawnDelta,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x12,
            (zNetworkPacketHandler)&Pickup::HandlePkt12_AirdropSpawnChuteRelay,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x13,
            (zNetworkPacketHandler)&HandlePkt13_EffectAnimActivationRecord,
            2
        );
        zNetwork::RegisterPacketHandler(
            0x14,
            (zNetworkPacketHandler)&HandlePkt14_HudTimerAndFlagsSync,
            2
        );
        zNetwork::RegisterPacketHandler(
            3,
            (zNetworkPacketHandler)&HandlePkt03_RemoveRemotePlayer,
            2
        );
        g_GameNet_HandlersRegistered = 1;
    }

    g_zDEClientCraterNetRelayCallback = (zDEClient_NetRelayCallback)&zDEClient_Crater::Execute;
    g_zDEClientQSandNetRelayCallback = (zDEClient_NetRelayCallback)&SendPkt10_QSandEvent;
    g_OptCatalog_AllocRuntimeGateCallback = &OptCatalog::AltGunDispatchAllocRuntimeGateCallback;
    g_OptCatalog_AltGunDispatchNoOpCallback = &AltGunDispatchNoOpCallback;
    g_OptCatalog_RemoveRuntimeRelayCallback = &OptCatalog::SendPkt0A_RemoveRuntimeRelay;
    zEffect_Anim::SetActivationDispatchContext(
        &SendPkt13_EffectAnimActivationRecord,
        0x0c
    );
}

/**
 * Reimplements 0x4320f0: GameNet::ResetRemotePlayersAndSpawnLists
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Clear remote player HUD rows and network spawn-point lists.
 */
void ResetRemotePlayersAndSpawnLists() {
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        HudUi::RemoveScoreboardEntryRow(row);
        g_HudUiTopMessageStack->RemoveChild((HudUiElement *)(&row->hudWidget));
        row = row->next;
    }

    GameNetSpawnPoint *spawnPoint = g_GameNetSpawnPointHead;
    while (spawnPoint != 0) {
        GameNetSpawnPoint *const next = spawnPoint->next;
        ::operator delete(spawnPoint);
        spawnPoint = next;
    }

    g_GameNetSpawnPointList.flags = 0;
    g_GameNetSpawnPointTail = 0;
    g_GameNetSpawnPointHead = 0;
    g_GameNetSpawnPointCount = 0;

    row = g_GameNetPlayerRowHead;
    while (row != 0) {
        GameNetPlayerRow *const next = row->next;
        row->DestroyEmbeddedPanel();
        ::operator delete(row);
        row = next;
    }

    g_GameNetPlayerRowList.flags = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowCount = 0;
}

/**
 * Reimplements 0x4320b0: GameNet::WaitForLocalPlayerColorIndex
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Pump pending DirectPlay messages until the local player receives a
 * positive color index or the wait budget expires.
 */
int __fastcall WaitForLocalPlayerColorIndex(
    int maxWaitSeconds
) {
    int waitedSeconds = 0;
    while (waitedSeconds < maxWaitSeconds) {
        zNetworkDPlay::ReceivePendingMessages(-1);

        const int colorIndex = zNetwork_GetLocalPlayerColorIndex();
        if (colorIndex > 0) {
            return colorIndex;
        }

        Sleep(1000);
        ++waitedSeconds;
    }

    return 0;
}

/**
 * Reimplements 0x4322a0: GameNet::ResetHudTimerPanelNetStateLongCountdown
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Reset the replicated HUD timer state to the long race countdown
 * defaults and update the displayed timer panel.
 */
void ResetHudTimerPanelNetStateLongCountdown() {
    g_HudTimerPanelNetState.timerSeconds = 36000.0f;
    HudUiTimerPanel::SetSeconds(
        36000.0f,
        -1.0f
    );
    g_HudTimerPanelNetState.startCountdownTriggered = 0;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 0;
    g_HudTimerPanelNetState.timeWarningThresholdSec = 120.0f;
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.startGateTriggered = 0;
    g_HudTimerPanelNetState.raceFinishCountdownTriggered = 0;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    memset(
        g_HudTimerPanelNetState.tailFlags,
        0,
        sizeof(g_HudTimerPanelNetState.tailFlags)
    );
    g_GameNetOneLapLeftMessageShown = 0;
}

/**
 * Reimplements 0x433710: GameNet::SetStatusBitsFromFlags
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Decode host status flags into the cached allow-map and name-tag bits.
 */
void __fastcall SetStatusBitsFromFlags(
    unsigned int statusFlags
) {
    g_GameNetStatus_AllowMaps = statusFlags & 1u;
    g_GameNetStatus_NameTags = (statusFlags >> 1) & 1u;
}

/**
 * Reimplements 0x433730: GameNet::GetStatusBitAllowMaps
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Return the cached status bit controlling map availability.
 */
int GetStatusBitAllowMaps() {
    return g_GameNetStatus_AllowMaps;
}

/**
 * Reimplements 0x433740: GameNet::GetStatusBitNameTags
 * Source: src/Battlesport/gamenet.cpp
 * Purpose: Return the cached status bit controlling remote player name tags.
 */
int GetStatusBitNameTags() {
    return g_GameNetStatus_NameTags;
}

/**
 * Reimplements 0x432d60: GameNet::UpdateRemotePlayerHudWidgetScreenPos
 * Source: src/Battlesport/gamenet.cpp
 * Purpose: Project a remote player name-tag HUD widget into screen space.
 */
int __fastcall UpdateRemotePlayerHudWidgetScreenPos(
    zUtil_SaveGameState *saveState
) {
    if (GetStatusBitNameTags() == 0) {
        return 0;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    HudUiPanel *const hudWidget = &saveState->netPlayerRow->hudWidget;
    zVec3 labelWorldPos = playerState->worldPos;
    labelWorldPos.y += 3.0f;

    if (Player::HasLineOfSightFromLocalPlayerFxOffset(playerState->rootNode, &labelWorldPos, 1) ==
        0) {
        GameNetSetRemoteHudVisible(
            hudWidget,
            0
        );
        return 0;
    }

    zVec3 projectedPoint = {0};
    const int clipped = zMath::ProjectPointAndClampToScreenClip(
        &labelWorldPos,
        &projectedPoint
    );
    const float replicateScale = zOpt::GetReplicateMode() != 0 ? 2.0f : 1.0f;
    const int screenX = (int)(projectedPoint.x * replicateScale);
    const int screenY = (int)(projectedPoint.y * replicateScale) - 10;

    if (screenY <= hudWidget->QueryTextHeight() + 26) {
        GameNetSetRemoteHudVisible(
            hudWidget,
            0
        );
        return 0;
    }

    if (clipped != 0) {
        GameNetSetRemoteHudVisible(
            hudWidget,
            0
        );
        return 0;
    }

    GameNetSetRemoteHudPos(
        hudWidget,
        screenX,
        screenY
    );
    GameNetSetRemoteHudVisible(
        hudWidget,
        1
    );
    return 1;
}

/**
 * Reimplements 0x414330: GameNet::ShowPlayerKillMessage
 * Source: D:\Proj\Battlesport\HudUi.cpp
 * Purpose: Format and display a multiplayer kill-feed message.
 */
void __fastcall ShowPlayerKillMessage(
    GameNetPlayerRow *victimRow,
    OptCatalogEntryDef *killEntry,
    GameNetPlayerRow *killerRow
) {
    const char *killVerb = "";
    if (killEntry == 0) {
        killVerb = zLoc::GetMessageString(0x253);
    } else if (killEntry->killVerbString != 0) {
        killVerb = killEntry->killVerbString;
    }

    char message[0x50];
    sprintf(
        message,
        "%s %s %s",
        victimRow->displayName,
        killVerb,
        killerRow->displayName
    );
    HudUi::ShowTopMessageLine(
        message,
        2.0f
    );
}

/**
 * Reimplements 0x432e70: GameNet::ReassignPlayerColorsAndRefreshRows
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Refresh player-row colors after network color assignment changes.
 */
int ReassignPlayerColorsAndRefreshRows(
    int,
    zNetworkPacketHeader *
) {
    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    while (row != 0) {
        const int colorIndex = zNetwork_GetPlayerColorIndexByKey(row->playerKey);
        row->playerColorIndex = colorIndex;

        const unsigned int color = g_GameNetPlayerRowStyleColors_00RRGGBB[colorIndex];
        row->playerColorPackedRgb = color;
        SetEmbeddedHudPanelColor(
            row,
            color
        );
        HudUi::RefreshScoreboardEntryRow(row);
        row->ApplyPlayerColorTint();

        row = row->next;
    }

    return 1;
}

/**
 * Reimplements 0x432ed0: GameNet::HandlePkt03_RemoveRemotePlayer
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Handle the remote-player remove packet by retiring the player's
 * runtime state, unlinking the HUD row, and deleting the player row.
 */
int __fastcall HandlePkt03_RemoveRemotePlayer(
    int senderPlayerId,
    zNetworkPacketHeader *
) {
    GameNetPlayerRow *const row = FindPlayerRowByKey(senderPlayerId);
    if (row == 0) {
        return 0;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)row->saveState;
    if (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        playerState->cameraTransitionTimer = 1;
        playerState->lifecycleState = 4;
        Player::ResetAltGunRuntimeState(saveState);
        Player::RemoveAllDeployedMines(saveState);
    }

    char message[0x80] = {0};
    zLoc::FormatMessage(
        message,
        sizeof(message),
        0x913,
        row->displayName
    );
    HudUi::ShowTopMessageLine(
        message,
        5.0f
    );
    HudUi::RemoveScoreboardEntryRow(row);
    GameNetSetRemoteHudVisible(
        &row->hudWidget,
        0
    );
    g_HudUiTopMessageStack->RemoveChild((HudUiElement *)(&row->hudWidget));

    if (g_GameNetPlayerRowCount == 0) {
        return 0;
    }

    if (row == g_GameNetPlayerRowHead) {
        --g_GameNetPlayerRowCount;
        GameNetPlayerRow *const next = row->next;
        g_GameNetPlayerRowHead = next;
        if (next == 0) {
            g_GameNetPlayerRowList.flags = 0;
            g_GameNetPlayerRowTail = 0;
        }
    } else {
        GameNetPlayerRow *previous = g_GameNetPlayerRowHead;
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
 * Reimplements 0x414390: GameNet::RefreshPlayerListMenu
 * Source: D:\Proj\Battlesport\ai_net.cpp
 * Purpose: Forward a player row to the HUD stats list triplet for scoreboard
 * entry insertion.
 */
void __fastcall RefreshPlayerListMenu(
    GameNetPlayerRow *playerRow
) {
    g_HudUiMgrStatsList->triplet->AddEntry(playerRow);
}

/**
 * Reimplements 0x433410: GameNet::HandlePkt0C_HudTimerStatusBits
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Apply replicated HUD timer seconds and warning status bits.
 */
int __fastcall HandlePkt0C_HudTimerStatusBits(
    int,
    NetPkt0C_HudTimerStatusBits *packet
) {
    const int statusBits = packet->statusBitsPackedHiWord;
    g_HudTimerPanelNetState.timerSeconds = packet->timerSeconds;
    g_HudTimerPanelNetState.timerDirectionNeg = statusBits & 1;

    float secondsStep = -1.0f;
    if (g_HudTimerPanelNetState.timerDirectionNeg == 0) {
        secondsStep = 1.0f;
    }

    HudUiTimerPanel::SetSeconds(
        g_HudTimerPanelNetState.timerSeconds,
        secondsStep
    );

    if (g_HudSensorTracker.raceCheckpointMode == 0 &&
        g_HudTimerPanelNetState.oneMinuteWarningShown == 0 && (statusBits & 4) != 0) {
        g_HudTimerPanelNetState.oneMinuteWarningShown = 1;
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x914),
            5.0f
        );
        if (zNetwork::IsHost() != 0) {
            HostUpdateSessionDescStatusFields(
                0x100,
                0,
                0,
                0
            );
        }
    }

    if (g_HudTimerPanelNetState.timeWarningShown == 0 && (statusBits & 2) != 0) {
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x915),
            5.0f
        );
        g_HudTimerPanelNetState.timeWarningShown = 1;
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_mpExitDialogState,
            0
        );
    }

    return 1;
}

/**
 * Reimplements 0x4337e0: GameNet::HandlePkt0B_ChatMessage
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Copy an incoming chat payload into a bounded local string and show it.
 */
int __fastcall HandlePkt0B_ChatMessage(
    int,
    NetPkt0B_ChatMessage *packet
) {
    char message[0x51] = {0};
    int messageLength = packet->messageLength;
    if (messageLength >= 0x50) {
        messageLength = 0x50;
    }

    if (messageLength > 0) {
        memcpy(
            message,
            packet->message,
            (size_t)(messageLength)
        );
    }

    message[messageLength] = '\0';
    HudUi::ShowChatLine(
        message,
        5.0f
    );
    return 1;
}

/**
 * Reimplements 0x433750: GameNet::SendPkt0B_ChatMessage
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Build and send a packet-0B chat message for the local player.
 */
void __fastcall SendPkt0B_ChatMessage(
    const char *message
) {
    const int messageLength = (int)(strlen(message));
    const int packetSize = messageLength + 12;
    NetPkt0B_ChatMessage *const packet = (NetPkt0B_ChatMessage *)(malloc((size_t)(packetSize)));
    memset(
        packet,
        0,
        (size_t)(packetSize)
    );

    packet->header.packetType = 0x0b;
    packet->header.packetSizeBytes = (short)(packetSize);
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->messageLength = (short)(messageLength);
    if (messageLength > 0) {
        memcpy(
            packet->message,
            message,
            (size_t)(messageLength)
        );
    }

    zNetwork_SendPacketReliable(&packet->header);
    free(packet);
}

/**
 * Reimplements 0x433250: GameNet::HandlePkt0D_HudTimerPanelState
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Apply the host HUD timer panel state packet to local timer state.
 */
int __fastcall HandlePkt0D_HudTimerPanelState(
    int,
    NetPkt0D_HudTimerPanelState *packet
) {
    const int statusBits = packet->hudTimerFlagsPacked;
    g_HudTimerPanelNetState.timerSeconds = packet->seconds;
    g_HudTimerPanelNetState.timerDirectionNeg = statusBits & 1;

    float secondsStep = -1.0f;
    if (g_HudTimerPanelNetState.timerDirectionNeg == 0) {
        secondsStep = 1.0f;
    }

    HudUiTimerPanel::SetSeconds(
        g_HudTimerPanelNetState.timerSeconds,
        secondsStep
    );

    if (g_HudTimerPanelNetState.startGateTriggered == 0 && (statusBits & 8) != 0) {
        g_HudTimerPanelNetState.startGateTriggered = 1;
    }

    if ((statusBits & 0x20) != 0) {
        zEffectAnim::SetVelocity_Thunk(
            /* Retail literal 0x4dcfd4 names the replicated start-countdown
               effect animation; data ownership remains blocked outside this
               source slice. */
            zEffectAnim::FindEntryByName("startcountdown"),
            0,
            0.0f,
            0.0f,
            0.0f
        );
        g_HudTimerPanelNetState.startCountdownTriggered = 1;
    }

    if (g_HudTimerPanelNetState.raceFinishCountdownTriggered == 0 && (statusBits & 0x10) != 0) {
        g_HudTimerPanelNetState.raceFinishCountdownTriggered = 1;
    }

    g_HudTimerPanelNetState.timeWarningShown = statusBits & 2;
    if (g_HudTimerPanelNetState.timeWarningShown != 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_mpExitDialogState,
            0
        );
    }

    return 1;
}

/**
 * Reimplements 0x433060: GameNet::HandlePkt08_PlayerKillEvent
 * Source: D:\Proj\Battlesport\ai_net.cpp
 * Purpose: Apply an incoming packet-08 player kill event and host-side
 * scoreboard update.
 */
int __fastcall HandlePkt08_PlayerKillEvent(
    int localPlayerKey,
    NetPkt08_PlayerKillEvent *packet
) {
    GameNetPlayerRow *const killerRow = FindPlayerRowByKey(localPlayerKey);
    GameNetPlayerRow *const victimRow = FindPlayerRowByKey(packet->targetPlayerKey);
    if (killerRow == 0 || victimRow == 0) {
        return 0;
    }

    OptCatalogEntryDef *killEntry = 0;
    const short killEntryId = packet->killMethodOrOptCatalogEntryId;
    if (killEntryId != 0) {
        killEntry = OptCatalog::FindEntryById(killEntryId);
    }

    ShowPlayerKillMessage(
        victimRow,
        killEntry,
        killerRow
    );

    if (zNetwork::IsHost() != 0) {
        const int score = victimRow->score;
        if (victimRow != killerRow) {
            victimRow->score = score + 1;
        } else {
            victimRow->score = score - 1;
            if (victimRow->score < 0) {
                victimRow->score = 0;
            }
        }

        SendPkt09_PlayerScoreboardSnapshot();
    }

    return 1;
}

/**
 * Reimplements 0x433000: GameNet::SendPkt08_PlayerKillEvent
 * Source: D:\Proj\Battlesport\ai_net.cpp
 * Purpose: Build, send, and locally dispatch a packet-08 player kill event.
 */
void __fastcall SendPkt08_PlayerKillEvent(
    zUtil_SaveGameState *saveState,
    short killMethodOrOptCatalogEntryId
) {
    zUtil_SaveGameState *saveStateOrLocal = saveState;
    if (saveStateOrLocal == 0) {
        saveStateOrLocal = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    }

    NetPkt08_PlayerKillEvent packet;
    packet.header.packetType = 0x08;
    packet.header.packetSizeBytes = sizeof(NetPkt08_PlayerKillEvent);
    packet.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet.killMethodOrOptCatalogEntryId = killMethodOrOptCatalogEntryId;
    packet.targetPlayerKey = saveStateOrLocal->netPlayerRow->playerKey;

    zNetwork_SendPacketReliable(&packet.header);
    HandlePkt08_PlayerKillEvent(
        zNetwork_GetLocalPlayerKey(),
        &packet
    );
}

/**
 * Reimplements 0x4330f0: GameNet::SendPkt0E_PlayerLapProgress
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Publish the local player's packed lap count and lap time packet.
 */
void __fastcall SendPkt0E_PlayerLapProgress(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    NetPkt0E_PlayerLapProgress packet;
    packet.header.packetType = 0x0e;
    packet.header.packetSizeBytes = sizeof(NetPkt0E_PlayerLapProgress);
    packet.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet.lapCountPacked = (short)(playerState->lapCount);
    packet.reserved_0a = 0;
    packet.lapTimeSec = playerState->lapTimeSec;

    if (zNetwork::IsHost() != 0) {
        saveState->netPlayerRow->lapCount = playerState->lapCount;
        saveState->netPlayerRow->lapTimeSec = playerState->lapTimeSec;
        HandlePkt0E_PlayerLapProgress(
            packet.header.payloadDword0,
            &packet
        );
    } else {
        zNetwork_SendPacketReliable(&packet.header);
    }
}

/**
 * Reimplements 0x4334f0: GameNet::SendPkt09_PlayerScoreboardSnapshot
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Send the host's packed player score and lap snapshot to peers.
 */
void SendPkt09_PlayerScoreboardSnapshot() {
    if (zNetwork::IsHost() == 0) {
        return;
    }

    const int entryCount = (int)(g_GameNetPlayerRowCount);
    const size_t packetSize = sizeof(zNetworkPacketHeader) + sizeof(int) +
                              (size_t)(entryCount) * sizeof(NetPkt09_PlayerScoreboardEntry);
    NetPkt09_PlayerScoreboardSnapshot *const packet =
        (NetPkt09_PlayerScoreboardSnapshot *)(malloc(packetSize));
    memset(
        packet,
        0,
        packetSize
    );

    packet->header.packetType = 0x09;
    packet->header.packetSizeBytes = (unsigned short)(packetSize);
    packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    packet->entryCount = entryCount;

    GameNetPlayerRow *row = g_GameNetPlayerRowHead;
    NetPkt09_PlayerScoreboardEntry *entry = packet->entries;
    while (row != 0) {
        entry->playerKey = row->playerKey;
        entry->packedScoreAndLapCount =
            (unsigned short)((row->lapCount << 9) + (row->score & 0x1ff));
        row = row->next;
        ++entry;
    }

    zNetwork_SendPacketReliable(&packet->header);
    if (zNetwork::IsHost() != 0) {
        HandlePkt09_PlayerScoreboardSnapshot(
            zNetwork_GetLocalPlayerKey(),
            packet
        );
    }

    free(packet);
}

/**
 * Reimplements 0x4335b0: GameNet::HandlePkt09_PlayerScoreboardSnapshot
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Apply packed player score and lap rows and trigger HUD warnings.
 */
int __fastcall HandlePkt09_PlayerScoreboardSnapshot(
    int,
    NetPkt09_PlayerScoreboardSnapshot *packet
) {
    GameNetPlayerRow *oneLapLeftRow = 0;
    const int entryCount = packet->entryCount;

    {
        for (int index = 0; index < entryCount; ++index) {
            NetPkt09_PlayerScoreboardEntry *const entry = &packet->entries[index];
            GameNetPlayerRow *const row = FindPlayerRowByKey(entry->playerKey);
            if (row == 0) {
                continue;
            }

            const unsigned short packed = entry->packedScoreAndLapCount;
            row->score = packed & 0x1ff;
            row->lapCount = ((short)(packed) >> 9) & 0x7f;
            HudUi::RefreshScoreboardEntryRow(row);

            if (g_GameNetOneLapLeftMessageShown == 0 && oneLapLeftRow == 0 &&
                row->lapCount == g_HudSensorTracker.runtimeGoalValue - 1) {
                oneLapLeftRow = row;
            }

            if (zNetwork::IsHost() != 0 && g_HudSensorTracker.raceCheckpointMode == 0 &&
                row->score >= g_HudSensorTracker.runtimeGoalValue) {
                HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
                if (timerState.timeWarningShown == 0) {
                    timerState.timeWarningShown = 1;
                    SendPkt0C_HudTimerStatusBits(&timerState);
                }
            }
        }
    }

    if (g_HudSensorTracker.raceCheckpointMode != 0 && g_GameNetOneLapLeftMessageShown == 0 &&
        oneLapLeftRow != 0) {
        char message[0x80];
        zLoc::FormatMessage(
            message,
            sizeof(message),
            0x918,
            oneLapLeftRow->displayName
        );
        HudUi::ShowTopMessageLine(
            message,
            5.0f
        );
        g_GameNetOneLapLeftMessageShown = 1;
        if (zNetwork::IsHost() != 0) {
            HostUpdateSessionDescStatusFields(
                0x100,
                0,
                0,
                0
            );
        }
    }

    return 1;
}

/**
 * Reimplements 0x433310: GameNet::SendPkt0D_HudTimerPanelState
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Send and locally apply the host HUD timer panel state packet.
 */
void __fastcall SendPkt0D_HudTimerPanelState(
    HudTimerPanelNetState *timerState
) {
    if (zNetwork::IsHost() == 0) {
        return;
    }

    g_NetPkt0D_HudTimerPanelStateBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
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
    zNetwork_SendPacketReliable(&g_NetPkt0D_HudTimerPanelStateBuf.header);
    HandlePkt0D_HudTimerPanelState(
        g_NetPkt0D_HudTimerPanelStateBuf.header.payloadDword0,
        &g_NetPkt0D_HudTimerPanelStateBuf
    );
}

/**
 * Reimplements 0x433170: GameNet::HandlePkt0E_PlayerLapProgress
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Apply a host-side player lap-progress packet and refresh race HUD
 * state when the lap target is reached.
 */
int __fastcall HandlePkt0E_PlayerLapProgress(
    int senderPlayerId,
    NetPkt0E_PlayerLapProgress *packet
) {
    int result = zNetwork::IsHost();
    if (result == 0) {
        return result;
    }

    GameNetPlayerRow *const row = FindPlayerRowByKey(senderPlayerId);
    if (row == 0) {
        return 0;
    }

    row->lapCount = packet->lapCountPacked;
    row->lapTimeSec = packet->lapTimeSec;
    SendPkt09_PlayerScoreboardSnapshot();

    if (row->lapCount >= g_HudSensorTracker.runtimeGoalValue) {
        HudTimerPanelNetState timerState = g_HudTimerPanelNetState;
        if (AreAllPlayersAtLapTarget() != 0) {
            timerState.timeWarningShown = 1;
            timerState.raceFinishCountdownTriggered = 1;
        }

        SendPkt0D_HudTimerPanelState(&timerState);
    }

    return 1;
}

/**
 * Reimplements 0x434370: GameNet::SendPkt13_EffectAnimActivationRecord
 * Source: D:\Proj\GameZRecoil\GameNet.cpp
 * Purpose: Send a reliable pkt13 effect-animation activation record unless
 * replay echo suppression is active.
 */
void __fastcall SendPkt13_EffectAnimActivationRecord(
    zEffectAnimActivationRecord *record
) {
    if (g_GameNetSuppressPkt13ActivationEcho != 0) {
        return;
    }

    const int packedRecordSize = zEffect_Anim::GetActivationRecordPackedSize(record);
    const int packetSize = (int)(sizeof(zNetworkPacketHeader)) + packedRecordSize;
    zNetworkPacketHeader *const packet = (zNetworkPacketHeader *)(malloc((size_t)(packetSize)));
    memset(
        packet,
        0,
        (size_t)(packetSize)
    );

    packet->packetType = 0x13;
    packet->packetSizeBytes = (short)(packetSize);
    packet->payloadDword0 = zNetwork_GetLocalPlayerKey();
    memcpy(
        ((unsigned char *)(packet)) + sizeof(zNetworkPacketHeader),
        record,
        (size_t)(packedRecordSize)
    );

    zNetwork_SendPacketReliable(packet);
    free(packet);
}

/**
 * Reimplements 0x434430: GameNet::SendAllPkt13_EffectAnimActivationRecords
 * Source: D:\Proj\GameZRecoil\GameNet.cpp
 * Purpose: Broadcast every queued effect-animation activation record from the
 * host.
 */
void SendAllPkt13_EffectAnimActivationRecords() {
    if (zNetwork::IsHost() == 0) {
        return;
    }

    const int recordCount = zEffect_Anim::GetActivationRecordCount();
    for (int index = 0; index < recordCount; ++index) {
        SendPkt13_EffectAnimActivationRecord(zEffect_Anim::GetActivationRecordAt(index));
    }
}

/**
 * Reimplements 0x4343f0: GameNet::HandlePkt13_EffectAnimActivationRecord
 * Source: D:\Proj\GameZRecoil\GameNet.cpp
 * Purpose: Apply a new remote effect-animation activation record while
 * suppressing replay echo.
 */
int __fastcall HandlePkt13_EffectAnimActivationRecord(
    int,
    zNetworkPacketHeader *packet
) {
    zEffectAnimActivationRecord *const record =
        (zEffectAnimActivationRecord *)((unsigned char *)(packet) + sizeof(zNetworkPacketHeader));
    if (zEffect_Anim::HasActivationRecord(record) == 0) {
        g_GameNetSuppressPkt13ActivationEcho = 1;
        zEffect_Anim::ProcessActivationRecord(record);
        g_GameNetSuppressPkt13ActivationEcho = 0;
    }

    return 1;
}

/**
 * Reimplements 0x433390: GameNet::SendPkt0C_HudTimerStatusBits
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Send and locally apply replicated HUD timer status bits.
 */
int __fastcall SendPkt0C_HudTimerStatusBits(
    HudTimerPanelNetState *timerState
) {
    const int result = zNetwork::IsHost();
    if (result == 0) {
        return result;
    }

    g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
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
    zNetwork_SendPacketReliable(&g_NetPkt0C_HudTimerStatusBitsBuf.header);
    return HandlePkt0C_HudTimerStatusBits(
        g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0,
        &g_NetPkt0C_HudTimerStatusBitsBuf
    );
}

/**
 * Reimplements 0x434460: GameNet::SendPkt14_HudTimerAndFlagsSync
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Send the reliable packet that synchronizes HUD timer and status flags.
 */
int __fastcall SendPkt14_HudTimerAndFlagsSync(
    int eventCode,
    unsigned int statusFlags,
    int valueOrTime,
    int auxParam
) {
    g_NetPkt14_HudTimerAndFlagsSyncBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt14_HudTimerAndFlagsSyncBuf.valueOrTime = valueOrTime;
    g_NetPkt14_HudTimerAndFlagsSyncBuf.eventCode = (short)(eventCode);
    g_NetPkt14_HudTimerAndFlagsSyncBuf.auxParam = (short)(auxParam);
    g_NetPkt14_HudTimerAndFlagsSyncBuf.statusFlags = statusFlags;
    return zNetwork_SendPacketReliable(&g_NetPkt14_HudTimerAndFlagsSyncBuf.header);
}

/**
 * Reimplements 0x4344b0: GameNet::HandlePkt14_HudTimerAndFlagsSync
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Receive the HUD timer/status sync packet and start the matching mission state.
 */
int __fastcall HandlePkt14_HudTimerAndFlagsSync(
    int senderPlayerId,
    NetPkt14_HudTimerAndFlagsSync *packet
) {
    (void)senderPlayerId;

    UnregisterGameplayPacketHandlers();
    ResetRemotePlayersAndSpawnLists();

    union TimerSecondsBits {
        float seconds;
        int raw;
    } timerSeconds = {(float)(packet->valueOrTime) * 60.0f};
    g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue(
        timerSeconds.raw,
        packet->auxParam
    );

    CZRecoilFrame *const mainWnd = (CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd()));
    g_HudSensorTracker.InitMissionIdAndFlags(
        packet->eventCode + 6,
        mainWnd->m_useArchiveBanks
    );
    SetStatusBitsFromFlags(packet->statusFlags);

    g_RecoilApp.m_missionFmvState.m_missionId = 0;
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_introFmvState,
        0
    );

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
 * Reimplements 0x434550: GameNet::HostUpdateSessionDescStatusFields
 * Source: D:\Proj\Battlesport\GameNet.cpp
 * Purpose: Let the host mirror timer and status fields into the session descriptor.
 */
int __fastcall HostUpdateSessionDescStatusFields(
    int eventCode,
    int auxParam,
    int valueOrTime,
    int statusFlags
) {
    if (zNetwork::IsHost() == 0) {
        return 0;
    }

    zNetworkSessionDescStatusFields statusFields;
    if (zNetwork_ExtractStatusFieldsFromSessionDesc(&statusFields) == 0) {
        return 0;
    }

    statusFields.valueOrTime = valueOrTime;
    statusFields.eventCode = eventCode;
    statusFields.statusFlags = statusFlags;
    statusFields.auxParam = auxParam;
    return zNetwork_ApplyStatusFieldsToSessionDesc(&statusFields);
}
} // namespace GameNet

namespace GameNetSpawnPointList {
/**
 * Reimplements 0x431bf0: GameNetSpawnPointList::InitGlobals
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Reset the GameNet-owned spawn-point list header to an empty state.
 */
void InitGlobals() {
    g_GameNetSpawnPointList.flags = 0;
    g_GameNetSpawnPointTail = 0;
    g_GameNetSpawnPointHead = 0;
    g_GameNetSpawnPointCount = 0;
}
} // namespace GameNetSpawnPointList

namespace GameNetPlayerRowList {
/**
 * Reimplements 0x431c20: GameNetPlayerRowList::Reset
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Reset the GameNet-owned player-row list header to an empty state.
 */
void Reset() {
    g_GameNetPlayerRowList.flags = 0;
    g_GameNetPlayerRowTail = 0;
    g_GameNetPlayerRowHead = 0;
    g_GameNetPlayerRowCount = 0;
}

/**
 * Reimplements 0x4345a0: GameNetPlayerRowList::AppendNewRow
 * Source: D:\Proj\GameZRecoil\RecoilApp\GameNet.cpp
 * Purpose: Allocate a scoreboard player row and append it to the supplied
 * GameNet player-row list header.
 */
GameNetPlayerRow *__fastcall AppendNewRow(
    GameNetPlayerRowListState *self,
    int zeroInitializeRow
) {
    GameNetPlayerRow *const row = (GameNetPlayerRow *)(::operator new(sizeof(GameNetPlayerRow)));
    row->hudWidget.ConstructorDefault(
        0,
        0,
        0
    );
    if (zeroInitializeRow != 0) {
        memset(
            row,
            0,
            sizeof(GameNetPlayerRow)
        );
    }

    row->next = 0;
    if (self->count == 0) {
        self->head = row;
    } else {
        self->tail->next = row;
    }

    self->tail = row;
    row->next = 0;
    ++self->count;
    return row;
}
} // namespace GameNetPlayerRowList
