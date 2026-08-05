#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>
#include <windows.h>

#include "Battlesport/recoil_app.h"
#include "Battlesport/recoil_state_dialog_host.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"
#include "recoil/recoil_callconv.h"

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
/**
 * Forward declaration for imported MFC42 CString. This is only a pointer
 * boundary here, not a local CString reimplementation.
 */
class CString;

extern char g_HudUiDialogSampleSetName[0x7];

struct HudWeatherFxPointBatch {
    float x;
    float y;
    float z;

    int ArePointBatchInsideRect(
        int pointCount,
        const HudUiRect *viewportRect
    );
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFxPointBatch) == 0x0c);

struct HudUiSaveLoadEntry : WIN32_FIND_DATAA {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSaveLoadEntry) == 0x140);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadEntry,
        ftLastWriteTime
    ) == 0x14
);
int __fastcall operator<(
    const HudUiSaveLoadEntry &lhs,
    const HudUiSaveLoadEntry &rhs
);

struct HudUiSaveLoadEntryAllocator {
    char value;

    /**
     * Restores the original-source inline one-byte VC5 allocator subobject used
     * by std::vector<HudUiSaveLoadEntry> member construction. No standalone
     * retail function exists; observed in callers 0x434680 and 0x434b90.
     * Purpose: keep the recovered save-entry vector layout aligned with the
     * retail allocator/_First/_Last/_End object shape.
     */
    HudUiSaveLoadEntryAllocator() {
#if !defined(_MSC_VER) || _MSC_VER >= 1200
        value = 0;
#endif
    }
};

struct HudUiSaveLoadEntries {
    HudUiSaveLoadEntryAllocator allocatorProxy;
    char padding[3];
    HudUiSaveLoadEntry *begin;
    HudUiSaveLoadEntry *end;
    HudUiSaveLoadEntry *capacityEnd;

    /**
     * Restores the original-source inline VC5 std::vector<HudUiSaveLoadEntry>
     * default constructor. No standalone retail function exists; observed in
     * callers 0x434680 and 0x434b90 immediately after the list-item array
     * construction.
     * Purpose: preserve the original save-entry vector member construction
     * shape before derived save/load button construction.
     */
    explicit HudUiSaveLoadEntries(
        const HudUiSaveLoadEntryAllocator &allocator = HudUiSaveLoadEntryAllocator()
    ) : allocatorProxy(allocator), begin(0), end(0), capacityEnd(0) {
    }

    HudUiSaveLoadEntry * InsertCopiesAt(
        HudUiSaveLoadEntry *position,
        unsigned int count,
        const HudUiSaveLoadEntry *entry
    );
    /**
     * Restores the original-source inline VC5 std::vector<HudUiSaveLoadEntry>
     * erase(first,last) body. No standalone retail function exists; observed in
     * caller 0x4355e0 as the save-entry vector clear path.
     * Purpose: preserve the original save-entry vector erase shape while
     * keeping the recovered file-entry storage typed.
     */
    HudUiSaveLoadEntry * EraseRangeNoDestroyInline(
        HudUiSaveLoadEntry *first,
        HudUiSaveLoadEntry *last
    ) {
        HudUiSaveLoadEntry *write = first;
        HudUiSaveLoadEntry *read = last;
        HudUiSaveLoadEntry *const oldEnd = end;
        if (read != oldEnd) {
            do {
                *write++ = *read++;
            } while (read != oldEnd);
        }
        ((StdPtrVector *)(this))->ClearNoOpDestroy(
            (int *)(write),
            (int *)(oldEnd)
        );
        end = write;
        return first;
    }
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSaveLoadEntries) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadEntries,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadEntries,
        end
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadEntries,
        capacityEnd
    ) == 0x0c
);

struct HudUiSaveLoadDialog;
struct HudUiSaveLoadListItem;

struct HudUiSaveLoadListItem : HudUiPanel {
    int layoutX;
    int layoutY;

    HudUiSaveLoadListItem();
    void Draw();
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSaveLoadListItem) == 0x2ac);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadListItem,
        parent
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadListItem,
        layoutX
    ) == 0x2a4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadListItem,
        layoutY
    ) == 0x2a8
);
struct HudUiSaveLoadDeleteButton : HudUiZrdWidget {
    void OnActivate();
};

struct HudUiSaveLoadNextButton : HudUiZrdWidget {
    void OnActivate();
};

struct HudUiSaveLoadPrevButton : HudUiZrdWidget {
    void OnActivate();
};

struct HudUiSaveGamePrimaryActionButton : HudUiZrdWidget {
    void OnActivate();
};

struct HudUiLoadGamePrimaryActionButton : HudUiZrdWidget {
    void OnActivate();
};

struct HudUiConfirmQuitOkButton : HudUiZrdWidget {
    void OnActivate();
};

struct HudUiConfirmQuitCancelButton : HudUiZrdWidget {
    virtual void OnActivate();
};

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hud-ui-background-confirm-quit.type
 * @recoil-artifact emits .text recoil:function:0x415790: VC5 compiler-generated deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: Defines the confirmation background whose ordinary virtual lifetime
 * causes VC5 to emit the deleting-destructor contribution.
 */
struct HudUiBackgroundConfirmQuit : HudUiBackground {
    HudUiConfirmQuitOkButton okButton;
    HudUiConfirmQuitCancelButton cancelButton;

    HudUiBackgroundConfirmQuit * Constructor();
    void Destructor();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundConfirmQuit) == 0xabe4);
RECOIL_STATIC_ASSERT(sizeof(HudUiConfirmQuitCancelButton) == 0x14c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundConfirmQuit,
        okButton
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundConfirmQuit,
        cancelButton
    ) == 0xaa98
);

struct HudUiSaveLoadGameNameInput : HudUiNumericTextInput {
    /**
     * Restores the original-source inline save/load game-name input constructor.
     * No standalone retail function exists; observed in callers 0x434680 and
     * 0x434b90 after the numeric input base construction and before the
     * list-item array construction.
     * Purpose: keep game-name text-buffer setup owned by the game-name input
     * member instead of the outer dialog constructor body.
     */
    HudUiSaveLoadGameNameInput() {
        textInput.AllocTextBuffer(20);
        Update("");
        SetInputActive(1);
        SetRawKeyboardCapture(1);
    }

    void OnActivate();
    int OnRawKeyboardEvent(int key);
    virtual void OnAccept();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSaveLoadGameNameInput) == 0x374);

struct HudUiSaveLoadDialog : HudUiBackground {
    HudUiSaveLoadDeleteButton deleteButton;
    HudUiMenuBackButton backButton;
    HudUiSaveLoadNextButton nextEntryButton;
    HudUiSaveLoadPrevButton prevEntryButton;
    HudUiSaveLoadGameNameInput gameNameInput;
    HudUiSaveLoadListItem entryWidgets[9];
    HudUiSaveLoadEntries fileEntries;
    int selectedEntryIndex;

    void Destructor();
    void InitializeFileEntries();
    void DeleteSaveFile(int confirmDelete);
    void RefreshSaveFileList();
    void SetSelectedEntryIndex(int selectedEntryIndex);
    void ProcessDialogResult();

};
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        deleteButton
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        backButton
    ) == 0xaa98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        nextEntryButton
    ) == 0xabe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        prevEntryButton
    ) == 0xad30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        gameNameInput
    ) == 0xae7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        entryWidgets
    ) == 0xb1f0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        entryWidgets
    ) +
        offsetof(
            HudUiSaveLoadListItem,
            layoutX
        ) ==
    0xb494
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        entryWidgets
    ) +
        offsetof(
            HudUiSaveLoadListItem,
            layoutY
        ) ==
    0xb498
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        entryWidgets
    ) + sizeof(HudUiSaveLoadListItem) * 8 +
        offsetof(
            HudUiSaveLoadListItem,
            layoutY
        ) ==
    0xc9f8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        fileEntries
    ) == 0xc9fc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        fileEntries.begin
    ) == 0xca00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        fileEntries.end
    ) == 0xca04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        fileEntries.capacityEnd
    ) == 0xca08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveLoadDialog,
        selectedEntryIndex
    ) == 0xca0c
);

struct HudUiSaveGameDialog : HudUiSaveLoadDialog {
    HudUiSaveGamePrimaryActionButton primaryActionButton;

    HudUiSaveGameDialog();
    void Destructor();
};
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSaveGameDialog,
        primaryActionButton
    ) == 0xca10
);

struct HudUiLoadGameDialog : HudUiSaveLoadDialog {
    HudUiLoadGamePrimaryActionButton primaryActionButton;

    HudUiLoadGameDialog();
    void Destructor();
    void ProcessDialogResult();
    void OnPrimaryActionThunk();
    void OnPrimaryAction();
};
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiLoadGameDialog,
        primaryActionButton
    ) == 0xca10
);

struct HudWeatherFxParticleQuad {
    int x;
    int y;
    int width;
    int height;
    unsigned short color16;
    unsigned short reserved12;
    float texCoordUStart;
    float texCoordUEnd;
    int slantOffset;
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFxParticleQuad) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudWeatherFxParticleQuad,
        texCoordUStart
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudWeatherFxParticleQuad,
        texCoordUEnd
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudWeatherFxParticleQuad,
        slantOffset
    ) == 0x1c
);

struct HudWeatherFxCameraTargetHistory {
    float x;
    float y;
    float z;
    float unknown0c;
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFxCameraTargetHistory) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudWeatherFxCameraTargetHistory,
        unknown0c
    ) == 0x0c
);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudweatherfx
 * @recoil-artifact emits .text recoil:function:0x4bde20: VC5 compiler-generated scalar deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: define the polymorphic weather-effect base whose ordinary virtual
 * lifetime causes VC5 to emit the deleting-destructor contribution.
 */
struct HudWeatherFx : zVideoFxPass3Element {
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

    HudWeatherFx(int particleCount);
    virtual ~HudWeatherFx();
    void ResetParticleSlot(
        int particleIndex,
        int unusedStack
    );
    virtual void ApplyPass3();
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFx) == 0x8c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudWeatherFx,
        particleQuads
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudWeatherFx,
        particlePositions
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudWeatherFx,
        textureRecord
    ) == 0x88
);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudweatherfxsnow
 * @recoil-artifact emits .text recoil:function:0x4be2c0: VC5 compiler-generated scalar deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: define the snow weather-effect specialization whose ordinary
 * virtual lifetime causes VC5 to emit the deleting-destructor contribution.
 */
struct HudWeatherFxSnow : HudWeatherFx {
    int emitEnabled;
    float emitRadius;
    float emitDepth;

    HudWeatherFxSnow(int particleCount);
    virtual ~HudWeatherFxSnow();
    void Update(float deltaSeconds);
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFxSnow) == 0x98);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudWeatherFxSnow,
        emitEnabled
    ) == 0x8c
);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudweatherfxrain
 * @recoil-artifact emits .text recoil:function:0x4be850: VC5 compiler-generated scalar deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: define the rain weather-effect specialization whose ordinary
 * virtual lifetime causes VC5 to emit the deleting-destructor contribution.
 */
struct HudWeatherFxRain : HudWeatherFx {
    int emitEnabled;
    float emitRadius;
    float emitDepth;

    HudWeatherFxRain(int particleCount);
    virtual ~HudWeatherFxRain();
    void Update(float deltaSeconds);
};
RECOIL_STATIC_ASSERT(sizeof(HudWeatherFxRain) == 0x98);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudWeatherFxRain,
        emitEnabled
    ) == 0x8c
);

struct RecoilStateSaveLoadTransition : RecoilApp_IState {
    // Current x86 owner-data evidence keeps these nullable object references
    // as 32-bit slots inside the 0x1c-byte singleton.
    RecoilPtr32 m_dialog; // HudUiSaveLoadDialog*
    RecoilSaveLoadDialogKind m_dialogKind;
    zVideoHalfResAdjustMode m_savedHalfResAdjustMode;
    RecoilSaveLoadPresentationCaptureMode m_capturePresentationMode;
    RecoilSaveLoadTransitionMode m_transitionMode;
    RecoilPtr32 m_pausedAudioSnapshot; // zSndPlayHandleSnapshot*

    static void __cdecl StaticInitAndRegisterAtExit();
    static RecoilStateSaveLoadTransition *__cdecl StaticInit();
    static void __cdecl RegisterAtExit();
    static void __cdecl AtExitDestructor();
    RecoilStateSaveLoadTransition * Constructor();
    void Destructor();
    int OnTryBecomeCurrent();
    int OnUpdateShouldQuit();
    void OnDeactivate();
    static void __fastcall QueueOpenSaveDialog(
        RecoilSaveLoadPresentationCaptureMode capturePresentationMode
    );
    static void __fastcall QueueOpenLoadDialog(RecoilSaveLoadTransitionMode transitionMode);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateSaveLoadTransition) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateSaveLoadTransition,
        m_dialog
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateSaveLoadTransition,
        m_dialogKind
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateSaveLoadTransition,
        m_capturePresentationMode
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateSaveLoadTransition,
        m_transitionMode
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateSaveLoadTransition,
        m_pausedAudioSnapshot
    ) == 0x18
);

union RecoilStateSaveLoadTransitionStorage {
    unsigned long align;
    unsigned char bytes[sizeof(RecoilStateSaveLoadTransition)];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateSaveLoadTransitionStorage) == 0x1c);

extern RecoilStateSaveLoadTransitionStorage g_RecoilStateSaveLoadTransition;
#define g_RecoilStateSaveLoadTransition \
    (*(RecoilStateSaveLoadTransition *)&g_RecoilStateSaveLoadTransition)

struct HudUiNewGamePanel_StartButton : HudUiZrdWidget {
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNewGamePanel_StartButton) == 0x14c);

struct HudUiNewGamePanel_NameInput : HudUiNumericTextInput {
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNewGamePanel_NameInput) == 0x374);

/**
 * BN 0x41c313 installs the panel-owned option selector table after the shared base constructor.
 */
struct HudUiNewGamePanel_Intensity : HudUiZrdWidgetEx17C {};
RECOIL_STATIC_ASSERT(sizeof(HudUiNewGamePanel_Intensity) == 0x17c);

struct HudUiNewGamePanel : HudUiBackground {
    HudUiMenuBackButton backWidget;
    HudUiNewGamePanel_StartButton startWidget;
    HudUiNewGamePanel_NameInput nameInput;
    HudUiNewGamePanel_Intensity intensity;

    HudUiNewGamePanel();
    virtual ~HudUiNewGamePanel();
    void SyncIntensityFromDifficulty();
    void StartGameFromFields();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNewGamePanel) == 0xb0d4);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNewGamePanel,
        backWidget
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNewGamePanel,
        startWidget
    ) == 0xaa98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNewGamePanel,
        nameInput
    ) == 0xabe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNewGamePanel,
        intensity
    ) == 0xaf58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNewGamePanel,
        intensity
    ) +
        offsetof(
            HudUiZrdWidgetEx17C,
            selectedIndex
        ) ==
    0xb0d0
);

struct HudUiNewGamePanelOverlayOwner : RecoilStateDialogHost {
    virtual ~HudUiNewGamePanelOverlayOwner();
    int OnTryBecomeCurrent();
    static void __cdecl StaticInitAndRegisterAtExit();
    static HudUiNewGamePanelOverlayOwner *__cdecl StaticInit();
    static void __cdecl RegisterAtExit();
    static void __cdecl AtExitDestructor();
    static void __cdecl QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNewGamePanelOverlayOwner) == 0x08);

extern HudUiNewGamePanelOverlayOwner g_HudUiNewGamePanelOverlayOwner;

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hud-ui-options-panel-overlay-owner.type
 * @recoil-artifact emits .text recoil:function:0x40d0c0: VC5 compiler-generated deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: Defines the options-panel overlay owner whose ordinary virtual
 * lifetime causes VC5 to emit the deleting-destructor contribution.
 */
struct HudUiOptionsPanelOverlayOwner : RecoilStateDialogHost {
    HudUiOptionsPanelOverlayOwner();
    static void __cdecl StaticInitAndRegisterAtExit();
    static HudUiOptionsPanelOverlayOwner *StaticInit();
    static void RegisterAtExit();
    static void __cdecl AtExitDestructor();
    ~HudUiOptionsPanelOverlayOwner();
    int OnTryBecomeCurrent();
    static void QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiOptionsPanelOverlayOwner) == 0x08);

union HudUiOptionsPanelOverlayOwnerStorage {
    unsigned long align;
    unsigned char bytes[sizeof(HudUiOptionsPanelOverlayOwner)];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiOptionsPanelOverlayOwnerStorage) == 0x08);

extern HudUiOptionsPanelOverlayOwnerStorage g_HudUiOptionsPanelOverlayOwner;
#define g_HudUiOptionsPanelOverlayOwner \
    (*(HudUiOptionsPanelOverlayOwner *)&g_HudUiOptionsPanelOverlayOwner)

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoil-state-confirm-quit.type
 * @recoil-artifact emits .text recoil:function:0x415860: VC5 compiler-generated deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: Defines the confirmation state whose ordinary virtual lifetime
 * causes VC5 to emit the deleting-destructor contribution.
 */
struct RecoilStateConfirmQuit : RecoilStateDialogHost {
    RecoilStateConfirmQuit();
    static void __cdecl StaticInitAndRegisterAtExit();
    static RecoilStateConfirmQuit *StaticInit();
    static void RegisterAtExit();
    static void __cdecl AtExitDestructor();
    int OnTryBecomeCurrent();
    void OnDeactivate();
    ~RecoilStateConfirmQuit();
    static void QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateConfirmQuit) == 0x08);

union RecoilStateConfirmQuitStorage {
    unsigned long align;
    unsigned char bytes[sizeof(RecoilStateConfirmQuit)];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateConfirmQuitStorage) == 0x08);

extern RecoilStateConfirmQuitStorage g_RecoilState_ConfirmQuit;
#define g_RecoilState_ConfirmQuit \
    (*(RecoilStateConfirmQuit *)&g_RecoilState_ConfirmQuit)

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoil-state-controls.type
 * @recoil-artifact emits .text recoil:function:0x408d70: VC5 compiler-generated deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: Defines the controls state whose ordinary virtual lifetime causes
 * VC5 to emit the deleting-destructor contribution.
 */
struct RecoilStateControls : RecoilStateDialogHost {
    RecoilStateControls();
    static void __cdecl StaticInitAndRegisterAtExit();
    static RecoilStateControls *StaticInit();
    static void RegisterAtExit();
    static void __cdecl AtExitDestructor();
    ~RecoilStateControls();
    int OnTryBecomeCurrent();
    void OnDeactivate();
    void OnSuspend(int suspendParam);
    void OnResume(int activateCode);
    static void QueueEnter();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateControls) == 0x08);

union RecoilStateControlsStorage {
    unsigned long align;
    unsigned char bytes[sizeof(RecoilStateControls)];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateControlsStorage) == 0x08);

extern RecoilStateControlsStorage g_RecoilStateControls;
#define g_RecoilStateControls \
    (*(RecoilStateControls *)&g_RecoilStateControls)

struct HudUiControlsDialog_ResumeWidget : HudUiZrdWidget {
    /**
     * Original inline constructor evidence: BN 0x408a30 constructs this
     * concrete resume widget through HudUiZrdWidget and then installs the
     * HudUiControlsDialog_ResumeWidget vptr.
     * Purpose: construct the controls-dialog resume widget subobject.
     */
    HudUiControlsDialog_ResumeWidget() : HudUiZrdWidget() {}
    virtual void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiControlsDialog_ResumeWidget) == 0x14c);

struct HudUiControlsDialog_CommandsWidget : HudUiZrdWidget {
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiControlsDialog_CommandsWidget) == 0x14c);

struct HudUiControlsDialog_OptionSelector : HudUiZrdWidgetEx17C {
    void DestructorCoreThunk();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiControlsDialog_OptionSelector) == 0x17c);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hud-ui-controls-dialog.type
 * @recoil-artifact emits .text recoil:function:0x408c40: VC5 compiler-generated deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: Defines the controls dialog whose ordinary virtual lifetime causes
 * VC5 to emit the deleting-destructor contribution.
 */
struct HudUiControlsDialog : HudUiBackground {
    HudUiControlsDialog_ResumeWidget resumeWidget;
    HudUiControlsDialog_CommandsWidget commandsWidget;
    HudUiControlsDialog_OptionSelector mouseOrJoystickSelector;
    HudUiControlsDialog_OptionSelector throttleModeSelector;
    HudUiControlsDialog_OptionSelector steeringModeSelector;
    HudUiControlsDialog_OptionSelector cursorModeSelector;
    HudUiControlsDialog_OptionSelector cameraModeSelector;

    HudUiControlsDialog();
    void Destructor();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiControlsDialog) == 0xb350);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiControlsDialog,
        resumeWidget
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiControlsDialog,
        commandsWidget
    ) == 0xaa98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiControlsDialog,
        mouseOrJoystickSelector
    ) == 0xabe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiControlsDialog,
        throttleModeSelector
    ) == 0xad60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiControlsDialog,
        steeringModeSelector
    ) == 0xaedc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiControlsDialog,
        cursorModeSelector
    ) == 0xb058
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiControlsDialog,
        cameraModeSelector
    ) == 0xb1d4
);

struct HudUiCheatCodeTitleWidget : HudUiZrdWidget {
    /**
     * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
     * Purpose: Queue the cheat-code state exit when the GO widget is activated.
     */
    virtual void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiCheatCodeTitleWidget) == 0x14c);

struct HudUiCheatTextInputWidget : HudUiNumericTextInput {
    HudUiCheatTextInputWidget();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiCheatTextInputWidget) == 0x374);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hud-ui-cheat-code-dialog.type
 * @recoil-artifact emits .text recoil:function:0x406e10: VC5 class-specific deleting-destructor contribution for the ordinary virtual HudUiCheatCodeDialog lifetime.
 * @recoil-artifact emits .text recoil:function:0x406e30: HudUiCheatCodeDialog::~HudUiCheatCodeDialog (compiler-emitted implicit destructor).
 * Purpose: Defines the complete cheat-code dialog type whose ordinary virtual
 * lifetime causes VC5 to emit both artifacts in src/Battlesport/hud.cpp.
 */
struct HudUiCheatCodeDialog : HudUiBackground {
    HudUiCheatCodeTitleWidget titleWidget;
    HudUiCheatTextInputWidget cheatInputWidget;

    HudUiCheatCodeDialog();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiCheatCodeDialog) == 0xae0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCheatCodeDialog,
        titleWidget
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCheatCodeDialog,
        cheatInputWidget
    ) == 0xaa98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCheatCodeDialog,
        cheatInputWidget
    ) +
        offsetof(
            HudUiNumericTextInput,
            textInput
        ) ==
    0xabe4
);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoil-state-cheat-code.type
 * @recoil-artifact emits .text recoil:function:0x406ee0: VC5 compiler-generated deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: Defines the cheat-code state whose ordinary virtual lifetime causes
 * VC5 to emit the deleting-destructor contribution.
 */
struct RecoilStateCheatCode : RecoilStateDialogHost {
    zVideoHalfResAdjustMode m_prevHalfResAdjustMode;
    RecoilPtr32 m_audioSnapshot; // zSndPlayHandleSnapshot*

    RecoilStateCheatCode();
    static void __cdecl StaticInitAndRegisterAtExit();
    static RecoilStateCheatCode *ConstructGlobal();
    static void StaticInit();
    static void __cdecl AtExitDestructor();
    int OnTryBecomeCurrent();
    void OnDeactivate();
    ~RecoilStateCheatCode();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCheatCode) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateCheatCode,
        m_prevHalfResAdjustMode
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateCheatCode,
        m_audioSnapshot
    ) == 0x0c
);

union RecoilStateCheatCodeStorage {
    unsigned long align;
    unsigned char bytes[sizeof(RecoilStateCheatCode)];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCheatCodeStorage) == 0x10);

extern RecoilStateCheatCodeStorage g_RecoilStateCheatCode;
#define g_RecoilStateCheatCode \
    (*(RecoilStateCheatCode *)&g_RecoilStateCheatCode)

extern "C" int g_RecoilState_MainMenuSkipExitDelay;
extern zSndSample *g_Hud_LowMeterBeepSample;
extern zSndSample *g_Hud_LowMeterLoopSample;
extern int g_Hud_LowMeterLoopActive;
extern float g_Hud_LowMeterBeepInterval;
extern float g_Hud_LowMeterNextBeepTime;
extern HudWeatherFxCameraTargetHistory g_HudWeatherFxSnow_LastCameraTarget;
extern float g_HudWeatherFxSnow_TimeAccumulator;
extern HudWeatherFxCameraTargetHistory g_HudWeatherFxRain_LastCameraTarget;
extern float g_HudWeatherFxRain_TimeAccumulator;
#define g_HudWeatherFxSnow_LastCameraTargetX (g_HudWeatherFxSnow_LastCameraTarget.x)
#define g_HudWeatherFxSnow_LastCameraTargetY (g_HudWeatherFxSnow_LastCameraTarget.y)
#define g_HudWeatherFxSnow_LastCameraTargetZ (g_HudWeatherFxSnow_LastCameraTarget.z)
#define g_HudWeatherFxRain_LastCameraTargetX (g_HudWeatherFxRain_LastCameraTarget.x)
#define g_HudWeatherFxRain_LastCameraTargetY (g_HudWeatherFxRain_LastCameraTarget.y)
#define g_HudWeatherFxRain_LastCameraTargetZ (g_HudWeatherFxRain_LastCameraTarget.z)
namespace HudUiCallback {
void QueueExitCurrentState();
int QueueCheatCodeState();
} // namespace HudUiCallback

namespace HudCheat {
int __fastcall ExecuteCommandString(CString *commandString);
void ClearNanitePanelCheatSentinel();
} // namespace HudCheat

namespace HudLowMeterLoopSound {
void __fastcall SetLoopActive(int enabled);
void Disable();
} // namespace HudLowMeterLoopSound
