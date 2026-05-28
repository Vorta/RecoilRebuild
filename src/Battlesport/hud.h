#pragma once

#include <stddef.h>
#include <windows.h>
#include "recoil/recoil_types.h"

#include "Battlesport/RecoilApp.h"
#include "recoil/recoil_callconv.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zVideo.h"

enum RecoilSaveLoadDialogKind {
    RECOIL_SAVELOAD_DIALOG_LOAD = 0,
    RECOIL_SAVELOAD_DIALOG_SAVE = 1,
};

enum RecoilSaveLoadPresentationCaptureMode {
    RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED = 0,
    RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED = 1,
};

enum RecoilSaveLoadTransitionMode {
    RECOIL_SAVELOAD_MODE_STANDARD = 0,
    RECOIL_SAVELOAD_MODE_FADE = 1,
    RECOIL_SAVELOAD_MODE_QUICKLOAD = 2,
};

enum zVideoHalfResAdjustMode {
    ZVIDEO_HALFRES_ADJUST_DISABLED = 0,
    ZVIDEO_HALFRES_ADJUST_ENABLED = 1,
};

struct HudUiDialogController;
struct HudUiNewGamePanel;
struct HudUiOptionsPanel;
struct HudUiBackgroundConfirmQuit;
struct HudUiControlsDialog;
struct HudUiCheatCodeDialog;
struct zSndSample;
struct zSndPlayHandleSnapshot;
struct zClass_NodePartial;

struct HudUiSaveLoadEntry : WIN32_FIND_DATAA {
    RECOIL_NOINLINE int RECOIL_FASTCALL IsNewerThan(const HudUiSaveLoadEntry *other) const;
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSaveLoadEntry) == 0x140);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadEntry, ftLastWriteTime) == 0x14);

struct HudUiSaveLoadEntries {
    int reserved00;
    HudUiSaveLoadEntry *begin;
    HudUiSaveLoadEntry *end;
    HudUiSaveLoadEntry *capacityEnd;
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSaveLoadEntries) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadEntries, begin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadEntries, end) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadEntries, capacityEnd) == 0x0c);

struct HudUiSaveLoadDialog;
struct HudUiSaveLoadListItem;
typedef void(RECOIL_THISCALL *HudUiSaveLoadInvalidateFn)(HudUiSaveLoadListItem *self);
typedef void(RECOIL_THISCALL *HudUiSaveLoadOnActivateFn)(HudUiSaveLoadListItem *self);
typedef void(RECOIL_THISCALL *HudUiSaveLoadSetVisibleFn)(HudUiSaveLoadListItem *self,
                                                         int visible);
typedef void(RECOIL_CDECL *HudUiSaveLoadSetTextFmtFn)(HudUiSaveLoadListItem *self,
                                                      const char *format,
                                                      const char *text);

struct HudUiSaveLoadListItemVtable {
    void *reserved00[8];
    HudUiSaveLoadInvalidateFn Invalidate;
    void *reserved24[3];
    HudUiSaveLoadOnActivateFn OnActivate;
    void *reserved34[11];
    HudUiSaveLoadSetVisibleFn SetVisible;
    void *reserved64[4];
    HudUiSaveLoadSetTextFmtFn SetTextFmt;
};
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadListItemVtable, Invalidate) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadListItemVtable, OnActivate) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadListItemVtable, SetVisible) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadListItemVtable, SetTextFmt) == 0x74);

struct HudUiSaveLoadListItem {
    const HudUiSaveLoadListItemVtable *vftable;
    void *next;
    HudUiSaveLoadDialog *parent;
    unsigned char reserved0c[0x298];
    int layoutX;
    int layoutY;

    RECOIL_NOINLINE HudUiSaveLoadListItem *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSaveLoadListItem) == 0x2ac);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadListItem, parent) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadListItem, layoutX) == 0x2a4);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadListItem, layoutY) == 0x2a8);

extern const HudUiSaveLoadListItemVtable g_HudUiSaveLoadListItem_Vtbl;

struct HudUiSaveLoadDeleteButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiSaveLoadNextButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiSaveLoadPrevButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiSaveGamePrimaryActionButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiLoadGamePrimaryActionButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiSaveLoadGameNameInput : HudUiNumericTextInput {
    void RECOIL_THISCALL OnActivate();
    int RECOIL_THISCALL OnRawKeyboardEvent(int key);
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSaveLoadGameNameInput) == 0x374);

struct HudUiSaveLoadDialog {
    HudUiBackground base;
    HudUiSaveLoadDeleteButton deleteButton;
    HudUiZrdWidget backButton;
    HudUiSaveLoadNextButton nextEntryButton;
    HudUiSaveLoadPrevButton prevEntryButton;
    HudUiSaveLoadGameNameInput gameNameInput;
    HudUiSaveLoadListItem entryWidgets[9];
    HudUiSaveLoadEntries fileEntries;
    int selectedEntryIndex;

    RECOIL_NOINLINE void RECOIL_THISCALL InitializeFileEntries();
    RECOIL_NOINLINE void RECOIL_THISCALL DeleteSaveFile(int confirmDelete);
    RECOIL_NOINLINE void RECOIL_THISCALL RefreshSaveFileList();
    RECOIL_NOINLINE void RECOIL_THISCALL SetSelectedEntryIndex(int selectedEntryIndex);
    RECOIL_NOINLINE void RECOIL_THISCALL ProcessDialogResult();

    static RECOIL_NOINLINE void RECOIL_FASTCALL
    InsertEntryIntoSortedPrefix(HudUiSaveLoadEntry *entryPosition, HudUiSaveLoadEntry entry);
    static RECOIL_NOINLINE HudUiSaveLoadEntry *RECOIL_FASTCALL
    PartitionEntriesByPivot(HudUiSaveLoadEntry *begin, HudUiSaveLoadEntry *end,
                            HudUiSaveLoadEntry pivot);
    static RECOIL_NOINLINE void RECOIL_FASTCALL
    SortEntryRange(HudUiSaveLoadEntry *begin, HudUiSaveLoadEntry *end, int unused);
};
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, deleteButton) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, backButton) == 0xaa98);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, nextEntryButton) == 0xabe4);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, prevEntryButton) == 0xad30);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, gameNameInput) == 0xae7c);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, entryWidgets) == 0xb1f0);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, entryWidgets) +
                     offsetof(HudUiSaveLoadListItem, layoutX) == 0xb494);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, entryWidgets) +
                     offsetof(HudUiSaveLoadListItem, layoutY) == 0xb498);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, entryWidgets) +
                     sizeof(HudUiSaveLoadListItem) * 8 +
                     offsetof(HudUiSaveLoadListItem, layoutY) == 0xc9f8);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, fileEntries) == 0xc9fc);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, fileEntries.begin) == 0xca00);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, fileEntries.end) == 0xca04);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, fileEntries.capacityEnd) == 0xca08);
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveLoadDialog, selectedEntryIndex) == 0xca0c);

struct HudUiSaveGameDialog : HudUiSaveLoadDialog {
    HudUiSaveGamePrimaryActionButton primaryActionButton;

    RECOIL_NOINLINE HudUiSaveGameDialog *RECOIL_THISCALL InitLayout();
};
RECOIL_STATIC_ASSERT(offsetof(HudUiSaveGameDialog, primaryActionButton) == 0xca10);

struct HudUiLoadGameDialog : HudUiSaveLoadDialog {
    HudUiLoadGamePrimaryActionButton primaryActionButton;

    RECOIL_NOINLINE HudUiLoadGameDialog *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE HudUiLoadGameDialog *RECOIL_THISCALL
    ScalarDeletingDestructor(unsigned int flags);
    RECOIL_NOINLINE void RECOIL_THISCALL ProcessDialogResult();
    RECOIL_NOINLINE void RECOIL_THISCALL OnPrimaryAction();
};
RECOIL_STATIC_ASSERT(offsetof(HudUiLoadGameDialog, primaryActionButton) == 0xca10);

struct HudWeatherFxParticleQuad {
    int x;
    int y;
    int width;
    int height;
    unsigned char reserved10[0x10];
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFxParticleQuad) == 0x20);

struct HudWeatherFx : HudUiElement {
    HudUiRect *viewportRect;
    HudWeatherFxParticleQuad *particleQuads;
    int maxParticles;
    int particleCount;
    unsigned short packedColor16;
    unsigned short reserved46;
    float alphaStartScale;
    float alphaEndScale;
    zClass_NodePartial *camera;
    int activeParticleCount;
    zVec3 *particlePositions[2];
    int sourceBufferIndex;
    int destBufferIndex;
    float windDirection;
    float windVelocity;
    float gravity;
    zVec3 basisVector;
    const char *textureName;
    zVidImagePartial *softwareImage;
    zVideo_TextureRecordPartial *textureRecord;

    RECOIL_NOINLINE HudWeatherFx *RECOIL_THISCALL Constructor(int particleCount);
    RECOIL_NOINLINE void RECOIL_THISCALL ResetParticleSlot(int particleIndex, int unusedStack);
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFx) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(HudWeatherFx, particleQuads) == 0x38);
RECOIL_STATIC_ASSERT(offsetof(HudWeatherFx, particlePositions) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(HudWeatherFx, textureRecord) == 0x88);

struct HudWeatherFxSnow : HudWeatherFx {
    int emitEnabled;
    float emitRadius;
    float emitDepth;

    RECOIL_NOINLINE HudWeatherFxSnow *RECOIL_THISCALL Constructor(int particleCount);
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFxSnow) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(HudWeatherFxSnow, emitEnabled) == 0x8c);

struct HudWeatherFxRain : HudWeatherFx {
    int emitEnabled;
    float emitRadius;
    float emitDepth;

    RECOIL_NOINLINE HudWeatherFxRain *RECOIL_THISCALL Constructor(int particleCount);
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFxRain) == 0x98);
RECOIL_STATIC_ASSERT(offsetof(HudWeatherFxRain, emitEnabled) == 0x8c);

struct RecoilStateSaveLoadTransition : RecoilApp_IState {
    RecoilPtr32 m_dialog; // HudUiDialogController*
    RecoilSaveLoadDialogKind m_dialogKind;
    zVideoHalfResAdjustMode m_savedHalfResAdjustMode;
    RecoilSaveLoadPresentationCaptureMode m_capturePresentationMode;
    RecoilSaveLoadTransitionMode m_transitionMode;
    RecoilPtr32 m_pausedAudioSnapshot; // zSndPlayHandleSnapshot*

    RECOIL_NOINLINE int RECOIL_THISCALL OnTryBecomeCurrent();
    RECOIL_NOINLINE int RECOIL_THISCALL OnUpdateShouldQuit();
    static void RECOIL_FASTCALL
    QueueOpenSaveDialog(RecoilSaveLoadPresentationCaptureMode capturePresentationMode);
    static void RECOIL_FASTCALL
    QueueOpenLoadDialog(RecoilSaveLoadTransitionMode transitionMode);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateSaveLoadTransition) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateSaveLoadTransition, m_dialogKind) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateSaveLoadTransition, m_capturePresentationMode) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateSaveLoadTransition, m_transitionMode) == 0x14);

extern RecoilStateSaveLoadTransition g_RecoilStateSaveLoadTransition;

struct HudUiNewGamePanelOverlayOwner : RecoilApp_IState {
    RecoilPtr32 m_panel; // HudUiNewGamePanel*

    static void RECOIL_CDECL QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNewGamePanelOverlayOwner) == 0x08);

struct HudUiOptionsPanelOverlayOwner : RecoilApp_IState {
    RecoilPtr32 m_panel; // HudUiOptionsPanel*

    static void RECOIL_CDECL QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiOptionsPanelOverlayOwner) == 0x08);

struct RecoilStateConfirmQuit : RecoilApp_IState {
    RecoilPtr32 m_dialog; // HudUiBackgroundConfirmQuit*

    RECOIL_NOINLINE ~RecoilStateConfirmQuit();
    RECOIL_NOINLINE RecoilStateConfirmQuit *RECOIL_THISCALL
    ScalarDeletingDestructor(unsigned int flags);
    static void RECOIL_CDECL QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateConfirmQuit) == 0x08);

struct RecoilStateControls : RecoilApp_IState {
    RecoilPtr32 m_dialog; // HudUiControlsDialog*

    static void RECOIL_CDECL QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateControls) == 0x08);

struct RecoilStateCheatCode : RecoilApp_IState {
    RecoilPtr32 m_dialog; // HudUiCheatCodeDialog*
    zVideoHalfResAdjustMode m_prevHalfResAdjustMode;
    RecoilPtr32 m_audioSnapshot; // zSndPlayHandleSnapshot*
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCheatCode) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateCheatCode, m_prevHalfResAdjustMode) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(RecoilStateCheatCode, m_audioSnapshot) == 0x0c);

extern HudUiNewGamePanelOverlayOwner g_HudUiNewGamePanelOverlayOwner;
extern HudUiOptionsPanelOverlayOwner g_HudUiOptionsPanelOverlayOwner;
extern RecoilStateConfirmQuit g_RecoilState_ConfirmQuit;
extern RecoilStateControls g_RecoilStateControls;
extern RecoilStateCheatCode g_RecoilStateCheatCode;
extern zSndSample *g_Hud_LowMeterBeepSample;
extern zSndSample *g_Hud_LowMeterLoopSample;
extern int g_Hud_LowMeterLoopActive;
extern float g_Hud_LowMeterBeepInterval;
extern float g_Hud_LowMeterNextBeepTime;
extern const HudUiCommon_FTable g_HudWeatherFx_Vtable;
extern const HudUiCommon_FTable g_HudWeatherFxSnow_Vtable;
extern const HudUiCommon_FTable g_HudWeatherFxRain_Vtable;

namespace HudUiCallback {
int RECOIL_CDECL QueueCheatCodeState();
}

namespace HudLowMeterLoopSound {
RECOIL_NOINLINE void RECOIL_FASTCALL SetLoopActive(int enabled);
RECOIL_NOINLINE void RECOIL_CDECL Disable();
}
