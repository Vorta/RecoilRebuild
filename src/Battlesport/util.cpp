#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zsave_game.h"

#include "Battlesport/ai_net.h"
#include "Battlesport/game_net.h"
#include "Battlesport/player.h"

#include <stdlib.h>
#include <string.h>

/**
 * @recoil-anchor recoil:anchor:battlesport-util-g-huduimessageboxdialog-sectionname
 * @recoil-artifact defines .data recoil:data:0x4dd1c8: g_HudUiMessageBoxDialog_SectionName.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: local MESSAGEBOX ZRD section-name data for the
 * HudUi::ShowMessageBox entrypoint wrapper; exact .data extent is the
 * writable char[11] bytes "MESSAGEBOX\0" with the sole xref in 0x438350.
 * Purpose: name the dialog.zrd section loaded by the modal message-box
 * wrapper.
 */
char g_HudUiMessageBoxDialog_SectionName[11] = "MESSAGEBOX";
/**
 * @recoil-anchor recoil:anchor:battlesport-util-k-msgboxwidgetname-message
 * @recoil-artifact defines .data recoil:data:0x4e489c: k_msgBoxWidgetName_Message.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the message text primitive from a loaded message-box layout.
 */
char k_msgBoxWidgetName_Message[8] = "MESSAGE";
/**
 * @recoil-anchor recoil:anchor:battlesport-util-k-msgboxwidgetname-title
 * @recoil-artifact defines .data recoil:data:0x4e48a4: k_msgBoxWidgetName_Title.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the title primitive from a loaded message-box layout.
 */
char k_msgBoxWidgetName_Title[6] = "TITLE";
/**
 * @recoil-anchor recoil:anchor:battlesport-util-k-msgboxwidgetname-cancel
 * @recoil-artifact defines .data recoil:data:0x4e48ac: k_msgBoxWidgetName_Cancel.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the cancel button from a loaded message-box layout.
 */
char k_msgBoxWidgetName_Cancel[10] = "MB_CANCEL";
/**
 * @recoil-anchor recoil:anchor:battlesport-util-k-msgboxwidgetname-ok
 * @recoil-artifact defines .data recoil:data:0x4e48b8: k_msgBoxWidgetName_OK.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the OK button from a loaded message-box layout.
 */
char k_msgBoxWidgetName_OK[6] = "MB_OK";

namespace HudUi {
/**
 * @recoil-anchor recoil:anchor:battlesport-util-hudui-showmessagebox
 * @recoil-artifact defines .text recoil:function:0x438350: HudUi::ShowMessageBox.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: HudUiMessageBoxDialog.cpp entrypoint wrapper that constructs
 * the stack HudUiMessageBoxDialog, not a broad HudUi owner or table scaffold.
 * Purpose: load the MESSAGEBOX section from dialog.zrd, run the dialog modally
 * with the caller strings/context and infinite timeout, then destroy it.
 * Touched data: g_HudUiMessageBoxDialog_SectionName at 0x4dd1c8 is the local
 * writable char[11] MESSAGEBOX section-name data; dialog.zrd is the accepted
 * shared dialog path literal.
 * Source placement note: this definition was provisionally moved from
 * HudUiMessageBoxDialog.cpp.
 */
int __fastcall ShowMessageBox(const char* messageText, const char* titleText, void* modalContext)
{
    HudUiMessageBoxDialog dialog("dialog.zrd", g_HudUiMessageBoxDialog_SectionName);
    return dialog.RunModal(messageText, titleText, modalContext, -1.0f);
}
} // namespace HudUi

/**
 * @recoil-anchor recoil:anchor:battlesport-util-zutil-savegamestatelist-init
 * @recoil-artifact defines .text recoil:function:0x4383e0: zUtil_SaveGameState::zUtil_SaveGameState.
 *
 *
 * Purpose: initialize a save-state list sentinel and allocate zeroed player
 * state storage for the owning save-game state.
 */
zUtil_SaveGameState::zUtil_SaveGameState()
{
    unknown_10 = 0;
    saveStateListTail = 0;
    saveStateListHead = 0;
    saveStateCount = 0;
    next = 0;
    firstSaveState = 0;

    playerState = (zUtil_PlayerStateStorage*)(malloc(sizeof(zUtil_PlayerStateStorage)));
    memset(playerState, 0, sizeof(zUtil_PlayerStateStorage));

    unknown_0c = 0;
    unknown_24 = 0;
    modeLoopBlend = 0.0f;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-util-zutil-savegamestate-freeownedresources
 * @recoil-artifact defines .text recoil:function:0x438430: zUtil_SaveGameState::FreeOwnedResources
 * @recoil-match byte
 *
 * Purpose: detach save-state back-references, free modal-state nodes, and
 * release the owned player-state storage.
 */
void zUtil_SaveGameState::FreeOwnedResources()
{
    if (playerState->lifecycleState == 2) {
        AINet::AiDiscardNegativeBranchPathNodes(this);
    }

    if (netPlayerRow != 0) {
        netPlayerRow->saveState = 0;
    }

    PlayerModalState* modalState = modalStateListHead;
    while (modalState != 0) {
        PlayerModalState* const nextModalState = modalState != 0 ? modalState->next : 0;
        if (modalState != 0 && modalStateCount != 0) {
            if (modalState == modalStateListHead) {
                --modalStateCount;
                modalStateListHead = modalState->next;
                if (modalStateListHead == 0) {
                    modalStateListAux = 0;
                    modalStateListTail = 0;
                }
            } else {
                for (PlayerModalState* cursor = modalStateListHead; cursor != 0; cursor = cursor->next) {
                    if (cursor->next == modalState) {
                        --modalStateCount;
                        cursor->next = modalState->next;
                        if (modalStateListTail == modalState) {
                            modalStateListTail = cursor;
                        }
                        break;
                    }
                }
            }
        }

        free(modalState);
        modalState = nextModalState;
    }

    primaryModalState = 0;
    free(playerState);
}

/**
 * @recoil-anchor recoil:anchor:battlesport-util-zutil-savegamestatelist-allocappend
 * @recoil-artifact defines .text recoil:function:0x4384e0: zUtilSaveGameStateListAllocAppend.
 * @recoil-match byte
 *
 * Purpose: allocate a zeroed save-state node and append it to the tracked
 * save-state list.
 */
zUtil_SaveGameState* __fastcall zUtilSaveGameStateListAllocAppend(zUtil_SaveGameState* self)
{
    zUtil_SaveGameState* const saveState = (zUtil_SaveGameState*)(malloc(sizeof(PlayerModalState)));
    memset(saveState, 0, sizeof(PlayerModalState));

    if (self->firstSaveState == 0) {
        self->firstSaveState = saveState;
    }

    if (saveState != 0) {
        saveState->next = 0;
        if (self->saveStateCount == 0) {
            self->saveStateListHead = saveState;
        } else {
            self->saveStateListTail->next = saveState;
        }

        self->saveStateListTail = saveState;
        saveState->next = 0;
        ++self->saveStateCount;
    }

    return saveState;
}

namespace {
/**
 * Original inline helper; no standalone retail function exists. Observed in address-backed caller 0x4386c0 as the
 * two-branch 0.0f..1.0f clamp. Purpose: clamp a blend value to the unit interval.
 */
float PlayerClamp01(float value)
{
    if (value > 1.0f) {
        return 1.0f;
    }
    if (value < 0.0f) {
        return 0.0f;
    }
    return value;
}

} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-selectmodalstatebymastertype-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x438540: Player::SelectModalStateByMasterType. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * @recoil-match byte
 *
 * Purpose: select the modal state matching a master type and stop existing modal loop handles before installing it as
 * primary.
 */
int zUtil_SaveGameState::SelectModalStateByMasterType(int masterType)
{
    zUtil_SaveGameState* const saveState = this;
    PlayerModalState* modalState = saveState->modalStateListHead;
    if (modalState == 0) {
        return 0;
    }

    for (; modalState != 0; modalState = modalState != 0 ? modalState->next : 0) {
        if (modalState->masterModalData->masterType == masterType) {
            saveState->StopModalLoopSfxHandle(2);
            saveState->StopModalLoopSfxHandle(0);
            saveState->StopModalLoopSfxHandle(1);
            saveState->primaryModalState = modalState;
            return 1;
        }
    }

    return 0;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-startmastertypeloopsfxhandle
 * @recoil-artifact defines .text recoil:function:0x4385a0: Player::StartMasterTypeLoopSfxHandle
 * @recoil-match byte
 *
 * Purpose: start the selected master-type weapon-up loop sample and cache the returned play handle in the player state.
 */
zSndPlayHandle* zUtil_SaveGameState::StartMasterTypeLoopSfxHandle(int modeIndex, float sfxVolume)
{
    zUtil_SaveGameState* const saveState = this;
    zVec3* worldPos;
    if (modeIndex == 3) {
        worldPos = 0;
    } else {
        worldPos = &saveState->playerState->worldPos;
    }
    zSndPlayHandle* const handle
        = saveState->playerState->masterCommonData->sfxWeaponUp[modeIndex]->PlayA3D(sfxVolume, worldPos, 0);
    saveState->playerState->modeLoopSfxHandle[modeIndex] = handle;
    return handle;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-startmodalloopsfxhandle-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x4385f0: Player::StartModalLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * @recoil-match byte
 *
 * Purpose: start one modal engine loop sample at the player world position and cache the returned play handle on the
 * active modal state.
 */
void zUtil_SaveGameState::StartModalLoopSfxHandle(int modalSfxIndex, float sfxVolume)
{
    zUtil_SaveGameState* const saveState = this;
    PlayerModalState* const modalState = saveState->primaryModalState;
    zSndSample* const sample = modalState->masterModalData->sfxEngine[modalSfxIndex];
    zSndPlayHandle* const handle = sample->PlayA3D(sfxVolume, &saveState->playerState->worldPos, 0);
    saveState->primaryModalState->modalSfxHandle[modalSfxIndex] = handle;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-ensuremastertypeloopsfxhandle-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x438630: Player::EnsureMasterTypeLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * @recoil-match byte
 *
 * Purpose: lazily start the selected master-type loop sample when configured and no cached handle is active.
 */
void zUtil_SaveGameState::EnsureMasterTypeLoopSfxHandle(int modeIndex, float sfxVolume)
{
    zUtil_SaveGameState* const saveState = this;
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    if (playerState->modeLoopSfxHandle[modeIndex] == 0 && playerState->masterCommonData->sfxWeaponUp[modeIndex] != 0) {
        saveState->StartMasterTypeLoopSfxHandle(modeIndex, sfxVolume);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-stopmastertypeloopsfxhandle-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x438660: Player::StopMasterTypeLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * @recoil-match byte
 *
 * Purpose: stop a cached master-type loop handle and clear the player-state handle slot when the handle is present.
 */
void zUtil_SaveGameState::StopMasterTypeLoopSfxHandle(int modeIndex)
{
    zUtil_SaveGameState* const saveState = this;
    zSndPlayHandle* const handle = saveState->playerState->modeLoopSfxHandle[modeIndex];
    if (handle != 0) {
        handle->StopIfActive();
        saveState->playerState->modeLoopSfxHandle[modeIndex] = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-stopmodalloopsfxhandle-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-no-authored-globals-touched
 * @recoil-artifact defines .text recoil:function:0x438690: Player::StopModalLoopSfxHandle. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; no authored globals touched.
 * @recoil-match byte
 *
 * Purpose: stop a cached modal engine loop handle and clear the modal-state slot when the handle is present.
 */
void zUtil_SaveGameState::StopModalLoopSfxHandle(int modalSfxIndex)
{
    zUtil_SaveGameState* const saveState = this;
    zSndPlayHandle* const handle = saveState->primaryModalState->modalSfxHandle[modalSfxIndex];
    if (handle != 0) {
        handle->StopIfActive();
        saveState->primaryModalState->modalSfxHandle[modalSfxIndex] = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemodalloopsfx-bn-source-path-d-proj-battlesport-player-cpp-source-model-zutil-savegamestate-modal-loop-sfx-record-method-reads-accepted-g-framedeltatimesec-and-original-inline-helpers-playerfloatfrombits-playerclamp01
 * @recoil-artifact defines .text recoil:function:0x4386c0: Player::UpdateModalLoopSfx. BN source path: D:\Proj\Battlesport\player.cpp. Source model: zUtil_SaveGameState modal loop SFX record method; reads accepted g_FrameDeltaTimeSec and original inline helpers PlayerFloatFromBits/PlayerClamp01.
 *
 *
 * Purpose: maintain modal and master loop SFX handles, blend pitch and enable scales from movement state, and update 3D
 * dispatch positions.
 */
void zUtil_SaveGameState::UpdateModalLoopSfx(int enabled)
{
    zUtil_SaveGameState* const saveState = this;
    if (enabled == 0) {
        saveState->StopModalLoopSfxHandle(2);
        saveState->StopModalLoopSfxHandle(0);
        saveState->StopModalLoopSfxHandle(1);
        saveState->StopMasterTypeLoopSfxHandle(3);
        return;
    }

    PlayerModalState* primaryModalState = saveState->primaryModalState;
    if (primaryModalState->modalSfxHandle[0] == 0) {
        if (primaryModalState->masterModalData->sfxEngine[0] != 0) {
            saveState->StartModalLoopSfxHandle(0, 0.0f);
        }
        if (saveState->primaryModalState->masterModalData->sfxEngine[1] != 0) {
            saveState->StartModalLoopSfxHandle(1, 0.0f);
        }
        if (saveState->primaryModalState->masterModalData->sfxEngine[2] != 0) {
            saveState->StartModalLoopSfxHandle(2, 0.0f);
        }
        return;
    }

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    if (playerState->slipSfxActive != 0 || playerState->airborneFlag != 0 || playerState->damageProtectionActive != 0) {
        const int smoothingBits = (int)(g_FrameDeltaTimeSec * -2.5f * 12102200.0f) + 0x3f800000;
        float smoothingFactor = 0.0f;
        memcpy(&smoothingFactor, &smoothingBits, sizeof(smoothingFactor));
        float throttleMagnitude = playerState->throttleInputCopy;
        if (throttleMagnitude < 0.0f) {
            throttleMagnitude = -throttleMagnitude;
        }
        saveState->modeLoopBlend
            = throttleMagnitude * (1.0f - smoothingFactor) + smoothingFactor * saveState->modeLoopBlend;
    } else {
        float forwardSpeed = playerState->localVel.z;
        if (forwardSpeed < 0.0f) {
            forwardSpeed = -forwardSpeed;
        }
        saveState->modeLoopBlend = forwardSpeed / primaryModalState->masterModalData->maxSpeed;
    }

    if (saveState->modeLoopBlend > 1.0f) {
        saveState->modeLoopBlend = 1.0f;
    } else if (saveState->modeLoopBlend < 0.0f) {
        saveState->modeLoopBlend = 0.0f;
    }

    primaryModalState = saveState->primaryModalState;
    zSndPlayHandle* handle = primaryModalState->modalSfxHandle[2];
    if (handle != 0) {
        handle->SetFreqScaled(primaryModalState->masterModalData->sfxPitchScale * saveState->modeLoopBlend);
        saveState->primaryModalState->modalSfxHandle[2]->SetEnableScale(1.0f - saveState->modeLoopBlend);
        saveState->primaryModalState->modalSfxHandle[2]->Update3DDispatch(&saveState->playerState->worldPos, 0, 0);
    }

    primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;
    float engineEnableScale = masterModalData->sfxVolumeScale * saveState->modeLoopBlend + 0.699999988f;
    if (engineEnableScale > 1.0f) {
        engineEnableScale = 1.0f;
    } else if (engineEnableScale < 0.0f) {
        engineEnableScale = 0.0f;
    }

    primaryModalState->modalSfxHandle[0]->SetFreqScaled(masterModalData->sfxPitchScale * saveState->modeLoopBlend);
    saveState->primaryModalState->modalSfxHandle[0]->SetEnableScale(engineEnableScale);
    saveState->primaryModalState->modalSfxHandle[0]->Update3DDispatch(&saveState->playerState->worldPos, 0, 0);

    handle = saveState->primaryModalState->modalSfxHandle[1];
    if (handle != 0) {
        handle->SetEnableScale(saveState->modeLoopBlend);
        saveState->primaryModalState->modalSfxHandle[1]->Update3DDispatch(&saveState->playerState->worldPos, 0, 0);
    }
}
