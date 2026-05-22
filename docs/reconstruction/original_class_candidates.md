# Original C++ Class Candidate Map

_Originally generated from an exported decompilation snapshot. Validated and
selectively updated against the currently loaded Binary Ninja database
`D:/Recoil Project/Decomp/Recoil.bndb` on 2026-05-22._

## Scope

This report extrapolates likely original C++ classes and class-like systems from
the current Binary Ninja/MFC-style reconstruction. The input is a best-effort
reconstruction of a 1999 Windows/DirectX game primarily built with VC5 SP3-era
tooling. The result is intended for reconstruction agents that are
reimplementing functions and need guidance on when to form classes, interfaces,
records, singletons, and namespace-style subsystems.

The findings use these evidence types:

| Evidence type | Weight | How it was used |
|---|---:|---|
| Constructor/destructor writes to a vtable/ftable | Highest | Confirms polymorphic class identity and often inheritance. |
| `.rdata` vtable/function-table blocks | Highest | Confirms virtual layout, function-table slot order, overrides, and shared base functions. |
| Struct `base` fields at offset `0x00` in current BN local types | High | Confirms inheritance-like composition or embedded base layout. |
| CRT dynamic initializer entries | High | Confirms global singleton/static objects and lifetime. |
| `__thiscall` methods without vtable evidence | Medium | Confirms class/record-style implementation but not necessarily polymorphism. |
| Many `Namespace::Function` functions with no instance pointer | Medium/Low | Usually a namespace/static subsystem rather than a class. |
| Shared prefix layouts but no vtable | Medium/Low | Often a C-style class system, especially the `zClass_*` node system. |

## Extracted summary statistics

| Item | Count / observation |
|---|---:|
| Current BN class/namespace names | 2,709 total: 2,218 class/struct type names plus 491 namespaces |
| Current BN functions/methods | 4,951 |
| Current BN local types | 4,741 |
| Current BN local types matching `VTable` | 41 |
| Current BN local types matching `FTable` | 58 |
| Dominant function groups | `Player`, `zInput`, `zRndr`, `zMath`, `HudSensorTracker`, `zOpt`, `zVideo`, `CZRecoilFrame`, `zEffect`, `OptCatalog`, `GameNet`, `zClass_*` |

Do **not** convert every `X::Y` group into a C++ class. Several original modules used namespaced C/C++ static subsystems with global state and free functions.

## How to use this map

Use this map for class/layout triage only. Before implementing or reshaping
source, confirm the exact function signature, field offsets, table slots,
constructor/destructor writes, and ownership against the current Binary Ninja
database. Binary Ninja and `.agent/RECOIL_PLAN.md` remain authoritative.

## Current BN validation limits

This pass validated broad class boundaries and high-value anchor types, not
every listed slot or subclass. Treat unverified generated labels, wildcard table
names, and placeholder `function_*` slots as prompts for per-function Binary
Ninja inspection rather than final source contracts.

## Confidence scale

| Code | Meaning | Reconstruction action |
|---|---|---|
| **A** | Confirmed polymorphic class/interface | Implement as class/struct with exact vtable or function-table layout. |
| **B** | Confirmed non-polymorphic class/record | Implement as class/struct with methods, constructors, ownership, and exact layout. |
| **C** | Static subsystem / namespace | Keep functions grouped in a namespace/module. Do not invent an instance class. |
| **D** | Data-driven or uncertain class family | Preserve layout and behavior first; add C++ inheritance only after more evidence. |

---

# 1. Top-level architecture

The codebase appears to have five large layers:

1. **MFC shell and state machine**: `RecoilApp`, `CZRecoilFrame`, `CZGameFrame`, modal/dialog states, app state stack, MFC resource dialogs.
2. **HUD/UI framework**: a custom immediate-ish retained UI tree around `HudUiElement`, `HudUiWidget`, panels, containers, ZRD-loaded widgets, and many specialized screens.
3. **Game runtime**: `Player` subsystem, `AINet`, pickups, weapons/`OptCatalog`, mission/briefing, `GameNet`.
4. **Engine/runtime services**: `zClass` scene graph, `zModel`, `zGeometry`, `zVideo`, `zRndr`, `zSnd`, `zInput`, `zNetwork`, `zInterp`, `zReader`.
5. **External integration**: DirectDraw/Direct3D/DirectInput/DirectSound/A3D/DirectPlay COM wrappers, AVI/MCI FMV, Westwood Online upgrade dialogs/API.

The original source likely mixed C++ classes, C-style structs, namespaces, and custom class systems. The `HudUi*`, `RecoilApp*`, `zFMV_Action*`, `WestwoodOnlineUpgrade*`, MFC dialogs, and DirectInput/DirectSound wrappers are strongly class-like. `Player`, `zVideo`, `zRndr`, `zClass_*`, `zMath`, `zOpt`, `zNetwork`, `OptCatalog`, and `Pickup` are more often static subsystems over records/globals.

---

# 2. Confirmed app/MFC class families

## 2.1 `RecoilApp` and MFC frame classes

### `RecoilApp` — confidence A

Likely original class:

```cpp
class RecoilApp : public CWinApp /* possibly COleControlModule-derived in MFC import metadata */ {
public:
    virtual BOOL InitInstance();
    virtual int Run();
    virtual BOOL PreTranslateMessage(MSG*);
    virtual int PumpMessage();
    virtual BOOL OnIdle(long);
    virtual int ExitInstance();

    virtual void OnAppActivate();
    virtual void OnAppDeactivate();
    virtual int StartEngine(HWND* hwnd);
    virtual void ShutdownEngine();
    virtual int OnIdleOrDispatch(int wParam, int lParam);
    virtual CZRecoilFrame* CreateMainWnd();

    RecoilApp_IState* m_startupState;
    int m_stateStackTopIndex;
    RecoilAppWaitMessageMode m_skipWaitMessage;
    RecoilAppMissionShutdownMode m_missionShutdownMode;
    RecoilApp_IState* m_stateStack[16];
    RecoilApp_StateQueue m_stateQueue;
    int m_playStateActivatedFlag;
    RecoilAppIntroFmvSkipMode m_skipIntroFmv;
    float m_transitionTimerSec;

    RecoilApp_AttractFmvState m_attractFmvState;
    RecoilApp_IntroFmvState m_introFmvState;
    RecoilApp_MainMenuPrepState m_mainMenuPrepState;
    RecoilApp_LeaveNetworkState m_leaveNetworkState;
    RecoilApp_MissionFmvState m_missionFmvState;
    RecoilApp_PlayState m_playState;
    RecoilApp_MpExitDialogState m_mpExitDialogState;
};
```

Evidence anchors:

- `g_RecoilApp_Vtbl` contains inherited MFC slots and app-specific virtuals: `InitMainWindow`/`InitInstance`, `Run`, `StartEngine`, `ShutdownEngine`, `OnIdleOrDispatch`, `CreateMainWnd`.
- The constructor builds embedded state objects and installs `g_RecoilApp_Vtbl` at the end.
- CRT init array contains `RecoilApp::StaticInitAndRegisterAtExit`, described as constructing the live `g_RecoilApp` singleton.
- `Run()` is a true app loop: message pump, DirectPlay receive polling, state queue processing, current-state update, suspend/resume/deactivate transitions.

Implementation notes:

- Reconstruct `RecoilApp` early. It is the owner of global state flow.
- Preserve the state queue as a deque/chunked queue if binary-accurate behavior matters; otherwise a small `std::deque<StateQueueItem>` equivalent is acceptable for clean reimplementation.
- `m_stateStack[16]` is a stack of state interface pointers. `QueueExitCurrentState`, `QueuePushState`, and `QueueSwitchCurrentState` should be methods on `RecoilApp`.
- Do not inline all state behavior into the app. The vtables show a real `RecoilApp_IState` interface.

### `CZRecoilFrame` — confidence A

Likely original class:

```cpp
class CZRecoilFrame : public CFrameWnd {
public:
    afx_msg void OnClose();
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT type, int cx, int cy);
    afx_msg void OnMove(int x, int y);
    afx_msg int OnCreate(CREATESTRUCTA*);
    afx_msg void OnDestroy();
    afx_msg void OnActivate(UINT, CWnd*, BOOL);

    // Menu actions and command-UI update handlers.
    void OnMenuSelectVideoModeN();
    void OnUpdateVideoModeNCmdUI(MfcCmdUI*);
    void OnMenuSelectSoftware();
    void OnMenuSelectDirectSound();
    void OnMenuSelectA3D();
};
```

Evidence anchors:

- `g_CZRecoilFrame_VTable` is a `CFrameWndVTable` with MFC inherited slots and a trailing title-builder override.
- Message map includes `WM_CLOSE`, `WM_PAINT`, `WM_SIZE`, `WM_MOVE`, `WM_CREATE`, `WM_DESTROY`, `MM_MCINOTIFY`, `WM_ACTIVATE`, plus many menu command/update handlers.
- Function group `CZRecoilFrame` is one of the largest confirmed MFC groups.

Implementation notes:

- This is the main game frame/window for the retail game.
- Keep MFC glue thin in a portable reimplementation: a platform window wrapper can map to these event handlers.
- The video/audio menu state arrays are likely a per-frame cached UI state rather than engine state.

### `CZGameFrame` — confidence A

Likely original class:

```cpp
class CZGameFrame : public CFrameWnd {
public:
    afx_msg void OnClose();
    afx_msg void OnPaint();
    afx_msg void OnSize(UINT, int, int);
    afx_msg void OnMove(int, int);
    afx_msg int OnCreate(CREATESTRUCTA*);
    afx_msg void OnDestroy();
    afx_msg void OnActivate(UINT, CWnd*, BOOL);
};
```

Evidence anchors:

- Separate MFC runtime-class record named `CZGameFrame` with `CreateObject` thunk.
- Separate frame vtable and message map from `CZRecoilFrame`.
- Likely used by the editor/tooling or an alternate shell path.

### `CAboutDlg` — confidence A

Likely original class:

```cpp
class CAboutDlg : public CDialog {
public:
    CAboutDlg(CWnd* parent = nullptr); // resource ID 0x67 / IDD_ABOUTBOX
};
```

Evidence anchors:

- Constructor calls `CDialog::CDialog(self, 0x67, parent)` then installs `g_CAboutDlg_Vtbl`.

### `AiPropertyDlg` — confidence A/B

Likely original class:

```cpp
class AiPropertyDlg : public CWnd /* or CDialog-like MFC wrapper */ {
    int selectedIndexFromCombo88;
    int selectedIndexFromComboC8;
    HWND comboHwnd_88;
    HWND comboHwnd_C8;

    void OnSelChange();
    void UpdatePropertyLabels();
};
```

Evidence anchors:

- Struct has MFC `CWnd base` and combo HWND/state fields.
- `OnSelChange()` reads combo selection via `SendMessageA`, stores selected index, and calls `UpdatePropertyLabels()`.
- `UpdatePropertyLabels()` uses MFC `CString` and writes labels such as `Min Pursuit Rng`, `Max Pursuit Rng`, `Attack Range`, `Movement`, and `Unused`.

Implementation notes:

- This looks like a developer/editor dialog rather than in-game runtime UI.
- Keep it separate from `AINet` itself.

### Networking/WOL MFC dialogs — confidence A

Confirmed MFC-like dialog classes include:

| Class | Likely base | Notes |
|---|---|---|
| `NetSessionBrowserDialog` | `CDialog` | DirectPlay session browser. |
| `NetSessionConfigDialog` | `CDialog` | DirectPlay session setup/config. |
| `WestwoodOnlineUpgradeConfigDialog` | `CDialog` | WOL configuration. |
| `WestwoodOnlineUpgradeDialog` | `CDialog` | Main WOL upgrade/session dialog. |
| `WestwoodOnlineUpgradeProgressDialog` | `CDialog` | Download/progress UI. |

Implementation notes:

- These are shell/UI classes. For a modern reimplementation, replace with platform-independent lobby/update UI, but keep data structures and callbacks for behavior parity.

---

# 3. Application state-machine classes

## 3.1 `RecoilApp_IState` interface — confidence A

Vtable layout:

```cpp
struct RecoilApp_IState_Vtbl {
    RecoilApp_IState* (*ScalarDeletingDtor)(RecoilApp_IState*, int flags);
    void (*OnWndActivate)(RecoilApp_IState*, RecoilWndActivateState);
    void (*OnEnter)(RecoilApp_IState*);
    int  (*OnTryBecomeCurrent)(RecoilApp_IState*);
    int  (*OnUpdateShouldQuit)(RecoilApp_IState*);
    void (*OnExit)(RecoilApp_IState*);
    void (*OnDeactivate)(RecoilApp_IState*);
    void (*OnSuspend)(RecoilApp_IState*, int param);
    void (*OnResume)(RecoilApp_IState*, int param);
    int  (*OnIdleOrDispatch)(RecoilApp_IState*, int wParam, int lParam);
};
```

Current BN validation:

- `RecoilApp_IState` is a concrete BN local type with only a
  `RecoilApp_IState_Vtbl* vftable` field.
- `RecoilApp_IState_Vtbl` currently names ten slots: `ScalarDeletingDtor`,
  `OnWndActivate`, `OnEnter`, `OnTryBecomeCurrent`,
  `OnUpdateShouldQuit`, `OnExit`, `OnDeactivate`, `OnSuspend`,
  `OnResume`, and `OnIdleOrDispatch`.
- Current BN also has recovered state-related structs and enums such as
  `RecoilApp_StateQueue`, `RecoilApp_StateQueueItem`,
  `RecoilApp_StateQueueBlock`, and `RecoilAppStateBecomeCurrentResult`.

Confirmed implementers:

| Class | Confidence | Notes |
|---|---:|---|
| `RecoilApp_AttractFmvState` | A | Owns `zFMV_Script`; plays/reloads `ATTRACT` from `fmv.zrd`. |
| `RecoilApp_IntroFmvState` | A | Owns `zFMV_Script`; intro FMV. |
| `RecoilApp_MainMenuPrepState` | A | Transition into main-menu state. |
| `RecoilApp_LeaveNetworkState` | A | Network-leave transition. |
| `RecoilApp_MissionFmvState` | A | Owns mission FMV script and mission id. |
| `RecoilApp_PlayState` | A | Active gameplay state; owns view rect pointers and pending load path. |
| `RecoilApp_MpExitDialogState` | A | Overlay/modal exit state for multiplayer. |
| `RecoilStateBase` | A | Base for HUD dialog-host states. |
| `RecoilStateDialogHost` | A | Base for modal HUD dialog states. |
| `RecoilStateCheatCode` | A | Static singleton dialog state. |
| `RecoilStateControls` | A | Static singleton dialog state. |
| `RecoilStateCredits` | A | Static singleton dialog state. |
| `RecoilStateConfirmQuit` | A | Owns `HudUiBackgroundConfirmQuit*`. |
| `RecoilStateMainMenuTransition` | A | Main-menu route state. |
| `RecoilStateSaveLoadTransition` | A | Save/load transition state. |
| `HudCmdDialogState` | A | Command-binding dialog state. |

Implementation notes:

- Treat app states as objects, not enum switch branches. The state queue stores state object pointers and an action kind.
- Several state objects are global singletons constructed from CRT init entries; `RecoilApp` embeds some state objects directly.
- Destructors frequently restore the base vtable before releasing embedded resources. For clean C++ this can be expressed with virtual destructors and RAII; for binary matching, preserve scalar deleting destructor semantics.

## 3.2 `RecoilApp_StateQueue` — confidence B

Likely original class/record:

```cpp
struct RecoilApp_StateQueueItem {
    int reserved;
    RecoilApp_StateQueueKind kind;
    RecoilApp_IState* stateObj;
    int param;
};

struct RecoilApp_StateQueueBlock {
    RecoilApp_StateQueueItem** chunkBegin;
    RecoilApp_StateQueueItem** chunkEnd;
    RecoilApp_StateQueueItem** cursor;
    RecoilApp_StateQueueItem*** chunkBaseSlot;
};

struct RecoilApp_StateQueue {
    int ctorTag;
    RecoilApp_StateQueueBlock readBlock;
    RecoilApp_StateQueueBlock writeBlock;
    RecoilApp_StateQueueItem*** chunkBaseList;
    int chunkBaseCapacity;
    int itemCount;
};
```

Implementation notes:

- This is almost certainly a hand-written or STL-like deque lowering. Use it as an implementation detail of `RecoilApp`.
- Queue item kind values: `ExitCurrent = 1`, `PushState = 2`, `SwitchCurrent = 3`.

---

# 4. HUD/UI framework

The HUD/UI framework is the clearest large C++ hierarchy in the binary. It uses custom function tables rather than normal MFC vtables. Most tables start with a common prefix, so reconstructing exact base-class layouts will greatly simplify the rest of the game.

Current BN validation:

- `HudUiElement` is a BN local type with an `ftable` pointer, `next`,
  `parent`, flags/timer/position fields, `zVideo_BltSourceHead* bltSource`,
  `HudUiRect clipRect`, `state`, and explicit trailing padding.
- The current database has named function-table types such as
  `HudUiCommon_FTable`, `HudUiWidget_FTable`, `HudUiContainer_FTable`,
  `HudUiBackground_FTable`, and many specialized widget/panel tables.
- Some table slots still have placeholder names like `function_28`;
  keep those unresolved until constructor/table data and xrefs prove their role.

## 4.1 Core UI hierarchy — confidence A

### Likely inheritance tree

```text
HudUiElement
├── HudUiWidget
│   ├── HudUiZrdWidget
│   │   ├── HudUiCheckToggleWidget
│   │   ├── HudUiCycleSelectorWidget
│   │   ├── HudUiFillBitmap
│   │   │   ├── HudUiBriefingTransportProgress
│   │   │   ├── HudUiOptionsPanelMusicVolumeWidget
│   │   │   └── HudUiOptionsPanelSoundVolumeWidget
│   │   ├── HudUiNumericTextInput
│   │   │   ├── HudUiClampedIntTextInput
│   │   │   └── HudUiSaveLoadGameNameInput
│   │   ├── HudUiClampedIntStepButton
│   │   ├── HudUiZrdScrollingText
│   │   ├── HudUiZrdWidgetEx150
│   │   └── HudUiZrdWidgetEx17C
│   │       ├── HudUiOptionSelectorWidget
│   │       └── HudUiZrdWidgetEx17C_Item
│   ├── HudUiCounter
│   ├── HudUiLoadingCheckpoint
│   ├── HudUiBriefingObjectivePicture
│   ├── HudUiMessageBoxBackdropWidget
│   └── many screen-specific buttons/widgets
├── HudUiPanel / HudUiPanelCore
│   ├── HudUiPanelSimple
│   ├── HudUiCounterTextPanel
│   ├── HudUiTimerPanel
│   ├── HudUiTimerPanelFloat
│   ├── HudUiFlashPanel
│   │   └── HudUiTransitionTextPanel
│   │       └── HudUiCompositePanelEntry
│   └── Player_TopMsgPanel
├── HudUiContainer
│   ├── HudUiBackground
│   │   ├── HudUiMainMenuDialog
│   │   ├── HudUiControlsDialog
│   │   ├── HudUiCreditsPanel
│   │   ├── HudUiBackgroundConfirmQuit
│   │   ├── HudUiMessageBoxDialog
│   │   ├── HudUiMpExitDialog
│   │   ├── HudUiNetExitPanel
│   │   ├── HudUiNetGameSetupPanel
│   │   ├── HudUiNewGamePanel
│   │   ├── HudUiSaveLoadDialog_BackgroundBase
│   │   │   └── HudUiSaveLoadDialog
│   │   └── HudUiBriefingRuntime
│   ├── HudUiZrdDialog
│   ├── HudUiListMenu
│   ├── HudUiStringMenu
│   ├── HudUiTextStack4
│   │   ├── HudUiTopMessageStack
│   │   └── HudUiChatMessageStack
│   └── HudUiTriplet
├── HudUiCircle
│   └── HudUiBriefingLocatorPanel
├── HudUiPolyline
│   └── HudUiSliderBorder
├── HudUiPrimitiveBindTarget
├── HudUiTextLabel
├── HudUiStatsListElement
├── HudUiBar
├── zVideoFxPass3Element
│   ├── zVideoFxPass3RootElement
│   └── zVideoFxPass3Slot
├── HudWeatherFx
│   ├── HudWeatherFxRain
│   └── HudWeatherFxSnow
└── PlayerScreenFxPass3Ui
```

### `HudUiElement` — confidence A

Function-table prefix:

```cpp
struct HudUiCommon_FTable {
    Dtor dtor;
    Noarg Draw;
    Noarg DrawBase;
    SetPos SetPos;
    SetCoord SetX;
    SetCoord SetY;
    SetClip SetClip;
    SetClipRect SetClipRect;
    Noarg Invalidate;
    Update Update;
    void* field_28;
    void* field_2c;
    Noarg OnPrimaryButtonReleased;
    Noarg OnSecondaryButtonReleased;
    Noarg OnHoverRepeat;
    Noarg OnHoverEnter;
    Noarg OnHoverExit;
    Noarg OnCaptureEnter;
    Noarg OnCaptureExit;
    PointerButtonState OnPointerButtonState;
    Noarg OnActivate;
    ShouldHandleInput ShouldHandleInput;
    AfterInputUpdate AfterInputUpdate;
    HitTest HitTest;
    SetVisible SetVisible;
    GetCoord GetX;
    GetCoord GetY;
    void* reserved_6c;
    GetRect GetRect;
};
```

Core fields:

```cpp
struct HudUiElement {
    HudUiCommon_FTable* ftable;
    HudUiElement* next;
    void* parent;
    uint32_t flags;
    float timer;
    int x;
    int y;
    zVideo_BltSourceHead* bltSource;
    HudUiRect clipRect;
    uint16_t state;
    uint16_t __padding_32;
};
```

Implementation notes:

- `HudUiElement::Draw()` virtual-forwards to `DrawBase`.
- `DrawBase()` blits `bltSource` at `(x, y)` with `clipRect`.
- `SetPos`, `SetX`, `SetY`, and `SetVisible` call `Invalidate()` through the ftable.
- This is the base for most HUD, weather, and pass-3 overlay objects.

### `HudUiWidget` — confidence A

Likely extension of `HudUiElement` with widget-specific slots:

```cpp
struct HudUiWidget_FTable : HudUiCommon_FTable {
    GetBoundsRectOrNull GetBoundsRectOrNull; // effective slot around +0x2c in widget tables
    Noarg OnActivate;
    // preview/show/hide slots and hit-test helpers
    GetCoord GetCenterX;
    GetCoord GetCenterY;
    GetRect GetRect;
    Noarg RebuildBltRectFromImage;
    Noarg RefreshState;
    LoadFromZrd LoadFromZrd;
    Noarg PostLoadFromZrd;
    SetNormalizedValue SetNormalizedValue;
};
```

Implementation notes:

- Several widgets use `HudUiZrdWidget::ScalarDeletingDestructorThunk`, `HudUiWidget::Draw`, `HudUiElement::DrawBase`, `HudUiWidget::SetPos`, and `HudUiElement` setters.
- Many screen-specific buttons are not distinct data classes; they are `HudUiZrdWidget` objects with a different ftable overriding only `OnActivate`.
- Current BN has `HudUiWidget_FTable` slots for `dtor`, `Draw`, `DrawBase`,
  `SetPos`, `SetX`, `SetY`, `SetClip`, `SetClipRect`, `Invalidate`,
  `Update`, `GetBoundsRectOrNull`, `OnActivate`, `HitTest`, `SetVisible`,
  `GetCenterX`, `GetCenterY`, `GetRect`, `RebuildBltRectFromImage`,
  `RefreshState`, `LoadFromZrd`, `PostLoadFromZrd`, and
  `SetNormalizedValue`, plus unresolved placeholder slots.
- Examples: `g_HudCmdPrevCommandButton_FTable`, `g_HudCmdNextCommandButton_FTable`, `g_HudCmdResetButton_FTable`, `g_HudCmdResumeButton_FTable`, `g_HudUiControlsDialog_ResumeWidget_Vtbl`, etc.

### `HudUiContainer` and `HudUiBackground` — confidence A

`HudUiContainer` owns a child linked list and has a small container ftable/header with `UpdateAll` and `SetEnabled`. `HudUiBackground` extends this to full-screen/modal backgrounds and typically includes scalar deleting destructor and primary/secondary action slots.

Implementation notes:

- Use `HudUiContainer::AddChild`, `UpdateAll`, `SetEnabled` as the composition root for most screens.
- A modal screen should be expressed as a `HudUiBackground`/`HudUiContainer` containing child widgets, not as many independent globals.

### `HudUiPanel` / `HudUiPanelCore` — confidence A/B

These are text panel classes with text buffers, font handles, colors, alignment, wrapping, dirty text bounds, and shadow settings.

Important methods:

- `HudUiPanel::SetTextColorsAndMarkDirty(color0, color1)`
- `HudUiPanel::SetShadow(enabled, offsetX, offsetY)`
- `SetText`, `SetTextFmt`, `SetFont`, `SetFontHandle`, `RebuildTextRect`

Implementation notes:

- `HudUiPanel` and `HudUiPanelCore` may represent two names for a closely related text-panel base; keep exact struct layout from current BN local types and only collapse after verification.
- `Player_TopMsgPanel` is essentially a panel-derived text object.

## 4.2 Screen/dialog classes — confidence A

Likely screen classes:

| Class | Base | Reimplementation role |
|---|---|---|
| `HudUiMainMenuDialog` | `HudUiBackground` | Front-end/in-game main menu. |
| `HudUiControlsDialog` | `HudUiBackground` | Control options plus command-binding entry. |
| `HudCmdDialog` | Background panel/container | Command-binding UI; has specialized key/mouse/joy buttons. |
| `HudUiOptionsPanel` | `HudUiDialogController` | Options composition owner/controller. |
| `HudUiNewGamePanel` | `HudUiBackground` | Campaign/mission start panel. |
| `HudUiNetGameSetupPanel` | `HudUiBackground` | Multiplayer setup. |
| `HudUiSaveLoadDialog` | `HudUiSaveLoadDialog_BackgroundBase` | Save/load slots, filename entry, list item selection. |
| `HudUiMessageBoxDialog` | `HudUiBackground` | Modal message box. |
| `HudUiMpExitDialog` | `HudUiBackground` | Multiplayer exit dialog. |
| `HudUiNetExitPanel` | `HudUiBackground` | Network-exit/status panel. |
| `HudUiCreditsPanel` | `HudUiBackground` | Credits screen. |
| `HudUiBackgroundConfirmQuit` | `HudUiBackground` | Confirm-quit dialog with OK/cancel buttons. |
| `HudUiCheatCodeDialog` | Dialog/controller | Cheat-code entry UI. |

Implementation notes:

- Screen-specific buttons should usually be small derived classes or function-table variants that override `OnActivate` only.
- Use owner pointers from the reconstructed structs. For example, save/load list item activation calls parent dialog selection when the parent is non-null.
- Dialog ZRD path names such as `dialog.zrd`, section names (`COMMANDS_DIALOG`, `CONTROLS_DIALOG`, `OPTIONSPANEL`, etc.), and node names are in `.data` and should remain constants.

## 4.3 `HudUiMgr` and sensor/HUD runtime — confidence A/B

`HudUiMgr` is a global HUD manager singleton with a function table and a large object graph for weapon slots, mode counters, objective bar, reticle, sensor, messages, top/chat stacks, timer, stats list, and overlays.

Implementation notes:

- Treat `HudUiMgr` as the owner of runtime HUD elements and layout application.
- HUD layout is data-driven from ZRD/ZAR-like nodes. Reconstruct `HudLayoutBase`, `HudLayoutSW`, and `HudLayoutHW` as singleton layout interpreters rather than one-off functions.
- The `HudSensorTracker` function group is large and likely a subsystem class/manager; preserve its mission/runtime data blocks.

## 4.4 Briefing UI and actions — confidence A

Likely hierarchy:

```text
HudUiBriefingRuntime : HudUiBackground
HudUiBriefingAction
├── Hide / Show element action
├── HudUiBriefingAction_FadeInElement
├── HudUiBriefingAction_SetPanelText
├── HudUiBriefingAction_SetWidgetImageTimed
├── HudUiBriefingAction_PlaySample
└── HudUiBriefingAction_DelayUntilProgress
```

`HudUiBriefingRuntime` owns:

- `HudUiBriefingActionQueueCtx actionQueue`
- `HudUiBriefingTransportProgress transportProgress`
- mission/objective panels
- `HudUiBriefingObjectivePicture objectivePicture`
- `HudUiCompositePanel messagesPanel`
- six `HudUiBriefingLocatorPanel` objects

Implementation notes:

- Build briefing as an action queue over owned UI objects.
- `BuildObjectiveActionsFromIndex()` is a method of `HudUiBriefingRuntime`.
- The global wrapper only calls into the runtime if `g_Briefing_Runtime` is non-null.

## 4.5 Weather and pass-3 overlay UI — confidence A

Classes:

| Class | Base | Notes |
|---|---|---|
| `zVideoFxPass3Config` | container-like config object | Global singleton controlling pass-3 overlay surface effects. |
| `zVideoFxPass3Element` | `HudUiElement`-prefix | Base element for pass-3 effects. |
| `zVideoFxPass3RootElement` | `zVideoFxPass3Element` | Packed color + alpha overlay. |
| `zVideoFxPass3Slot` | `zVideoFxPass3Element` | Radius/max/extent/frequency/phase effect slot. |
| `HudWeatherFx` | `HudUiElement` | Weather particle base. |
| `HudWeatherFxRain` | `HudWeatherFx` | Rain particle effect. |
| `HudWeatherFxSnow` | `HudWeatherFx` | Snow particle effect. |
| `PlayerScreenFxPass3Ui` | `HudUiElement` | Player-specific underwater/projectile-camera screen FX. |

Implementation notes:

- Keep pass-3 effects in the UI/effects layer, not in the renderer core.
- Player-specific `UnderwaterFxPass3Ui` and `State7FxPass3Ui` are global singleton objects initialized by CRT entries.

---

# 5. FMV/cutscene classes

## 5.1 `zFMV_Action` hierarchy — confidence A

Vtable layout:

```cpp
struct zFMV_ActionVtable {
    zFMV_ActionDeleteFn      pfnDelete;
    zFMV_ActionUpdateFn      pfnUpdate;
    zFMV_ActionBeginFn       pfnBegin;
    zFMV_ActionEndFn         pfnEnd;
    zFMV_ActionRunBlockingFn pfnRunBlocking;
    void*                    pReserved;
};

struct zFMV_Action {
    zFMV_ActionVtable* vftable;
    zFMV_Action* next;
};
```

Confirmed derived action classes:

| Class | Data / role |
|---|---|
| `zFMV_ActionWait` | Duration and start time. |
| `zFMV_ActionFade` | Fade direction, packed color, duration/start, captured frame, max alpha. |
| `zFMV_ActionBlur` | Blur frame/pass state and rects. |
| `zFMV_ActionImage` | Image path/ref, adjustment flags, blit rect. |
| `zFMV_ActionPlayAvi` | AVI playback action. |
| `zFMV_ActionPlayMci` | MCI playback pointer and media path. |
| `zFMV_ActionPlaySound` | Sound sample/voice and sample name. |

Current BN validation:

- `zFMV_Action` is a BN local type with `zFMV_ActionVtable* vftable` and
  `zFMV_Action* next`.
- `zFMV_ActionVtable` currently names `pfnDelete`, `pfnUpdate`,
  `pfnBegin`, `pfnEnd`, `pfnRunBlocking`, and `pReserved`.
- Current BN has local types for `zFMV_ActionWait`, `zFMV_ActionFade`,
  `zFMV_ActionBlur`, `zFMV_ActionImage`, `zFMV_ActionPlayAvi`,
  `zFMV_ActionPlayMci`, and `zFMV_ActionPlaySound`.

Implementation notes:

- This is a real polymorphic hierarchy. Implement virtual destructor/update/begin/end/run-blocking methods or equivalent.
- The action list is singly linked through `next`; script owns the chain.
- Derived destructors restore base vtable in decompiled output because of MSVC destruction order.

## 5.2 `zFMV_Script`, `zFMV_Stream`, `zFMV_Playback` — confidence B

`zFMV_Script` owns the current action chain:

```cpp
struct zFMV_Script {
    char* m_fmvPath;
    HWND m_hWnd;
    double m_startTimeSec;
    int m_abortOnKey;
    zFMV_Action* m_head;
    zFMV_Action* m_tail;
    zFMV_Action* m_cur;
};
```

`zFMV_Stream` is a larger AVI/audio/video stream object with DirectDraw surface, AVI video/audio streams, decompressor buffers, frame timing, and audio format/lock state.

Implementation notes:

- Keep `zFMV_Script` as a small action-list owner.
- Keep `zFMV_Stream` and `zFMV_Playback` as non-polymorphic media backends.
- `RecoilApp_*FmvState` classes embed `zFMV_Script` directly.

---

# 6. Player and vehicle runtime

## 6.1 `Player` — confidence C for namespace/subsystem, B for records

The `Player::` function group is one of the largest groups in current BN. However, most functions take `zUtil_SaveGameState* saveState`, then immediately access `saveState->playerState`, `primaryModalState`, and global `g_GameStateOrMapTable`. This argues for a static subsystem or namespace over large state records, not necessarily a single C++ `Player` object.

Current BN validation:

- BN now has a small `Player` local type with `vtbl`, `PlayerState*`, and
  `PlayerModalState*` fields.
- BN also has many supporting player records and enums, including
  `PlayerCameraRuntimeState`, `PlayerMovementRuntimeState`,
  `PlayerPendingContactQueue`, `PlayerGunFireController`,
  `PlayerAltWeaponBankRuntime`, and `PlayerScreenFxPass3Ui`.
- This strengthens the record/subsystem split: do not collapse the entire
  `Player::` namespace into one monolithic C++ class.

Recommended split:

| Candidate | Confidence | Notes |
|---|---:|---|
| `namespace Player` / `PlayerSystem` | C | Static functions: ticking, camera, controls, AI, spawning, save/load, weapons, collisions. |
| `PlayerState` | B | Huge live runtime record. Core gameplay state belongs here. |
| `PlayerMasterCommonData` | B | Vehicle/common tuning data list. CRT-initialized intrusive list. |
| `PlayerMasterModalData` | B | Mode-specific vehicle data; probe points, master type, transitions. |
| `PlayerModalState` | B | Active vehicle modal state bound to common/modal data. |
| `PlayerLifecycleRuntime` | B | Lifecycle/inactive/destroyed/respawn state. |
| `PlayerCameraRuntimeState` | B | Camera smoothing, third/first person, projectile camera. |
| `PlayerMovementRuntimeState` | B | Steering/throttle/pitch/inputs/motion. |
| `PlayerPendingContactQueue` and contact structs | B | Collision queues and probe state. |
| `PlayerGunFireController` | B | Primary/alt gun dispatch, cooldowns, opt metadata. |
| `PlayerAltWeaponBankRuntime` | B | Alt weapon slots and controller runtime. |
| `PlayerSave*` structs | B | Save serialization records. |
| `PlayerScreenFxPass3Ui` | A | Real UI-like class through `HudUiElement` ftable. |

Implementation notes:

- Do not start with `class Player { ... 200 methods ... }` unless later evidence shows a stable instance layout. Start with `PlayerState` + static/free subsystem functions.
- For clean modern code, a façade class such as `PlayerSystem` can own functions, but keep binary-layout records exact.
- Most `Player::` methods are better mapped to modules: `PlayerControls`, `PlayerCamera`, `PlayerAI`, `PlayerWeapons`, `PlayerCollision`, `PlayerSave`, `PlayerHudFx`.

## 6.2 AI mode 2 runtime — confidence B

Confirmed data-driven AI helpers:

- `Player::TickAiMode2TopLevel`
- `TickAiMode2PathFollow`
- `AiMode2ForwardProbeRequiresAutoTurn`
- `AiAdvancePathCursorAndComputeTargetVec`
- `AiChooseNextPathBranchIndex`
- `TickAiMode2SteeringSubstate`
- `AiTryEnterMode2AttackPursuitIfLineOfSight`
- `AiAlertAttackBuddies`
- `AiEnterMode2SteeringPursuit`
- `AiRebuildSyntheticPathToNodeIfFar`
- `AiRestoreSavedTopLevelState`
- `TickAiMode2AltGunAttackWindow`

AI mode 2 uses:

- `AINet`/`AINetNode` path graph
- synthetic temporary `AINetNode` objects where `nodeIndex == -1`
- `aiPeerRingNext` rings among save states with matching `aiNetId`
- collision probe queues and line-of-sight raycasts through `zClass_cls_di`

Implementation notes:

- Implement `PlayerAIState`/`PlayerAiMode2HandlerState` as slices of `PlayerState`, even if the original did not define a class.
- `AiDiscardNegativeBranchPathNodes` confirms temporary path nodes must be freed and then replaced by their `neighborRefs[0].node` chain.
- Preserve XZ-only vector logic and pitch handling for submarines/copters/alternate master types.

---

# 7. AI path-network classes

## 7.1 `AINet`, `AINetNode`, `AINetPathProbeFan` — confidence B

These are non-polymorphic but clearly class/record-like and have methods.

```cpp
struct AINet {
    int netId;
    char name[0x14];
    AINetType aiType;
    float pathWidth;
    float activateRadius;
    float attackRadius;
    float attackDwell;
    float notPursuitDwell;
    float pursuitParam0;
    float pursuitParam1;
    float returnRange;
    float hideTime0;
    float hideTime1;
    int activateBuddyNetId;
    int attackBuddyNetId;
    AINetAttackStrategy attackStrategy;
    AINetNode* nodeListHead;
    AINet* next;
};

struct AINetNode {
    zVec3 position;
    AINetNeighborSlot neighborRefs[3];
    AINetPathProbeFan* probeFans[3];
    int costOrType;
    int nodeIndex;
    AINetNode* next;
};

struct AINetPathProbeFan {
    zVec3 delta;
    float clampedTravel;
    zVec3 perpendicular;
    zVec3 probeDirPlus45;
    zVec3 probeDirMinus45;
    float pathWidth;
};
```

Confirmed methods:

| Method | Role |
|---|---|
| `AINetNode::Free` | Frees up to three `probeFans`, then frees node. |
| `AINet::Free` | Frees all owned nodes and the net. |
| `AINet::FreeAll` | Releases global list. |
| `AINetPathProbeFan::InitFromSegment` | Builds probe fan from segment and path width. |
| AINet load/parse methods | Read `ai_paths.zrd`; parse type, strategy, parameters, node references. |

Implementation notes:

- `neighborRefs` is a union: serialized node index during load, pointer after fix-up.
- `probeFans` are owned by each `AINetNode` and freed by `AINetNode::Free`.
- Use `nodeIndex == -1` for synthetic temporary path nodes created by `Player::AiRebuildSyntheticPathToNodeIfFar`.
- Constants and ZRD field names are explicit: `attack_strategy`, `activate_buddy`, `attack_buddy`, `hide_times`, `return_range`, `not_pursuit_dwell`, `pursuit_range`, `pursuit_params`, `attack_dwell`, `attack_rad`, `activate_rad`, `path_width`, `type`, `name`, `version`.

---

# 8. Scene graph / `zClass` engine objects

## 8.1 Do not over-model `zClass_*` as C++ inheritance — confidence D/C

There are many `zClass_*` structs and functions, but the evidence points to a custom C-style scene graph / class-data system rather than normal C++ inheritance for every node type.

Current BN validation:

- `zClass_Node` is a concrete BN local type with name/flag fields,
  `zClass_NodeClassType classType`, `zClass_NodeClassDataPtr classData`,
  `zDiRef diRef`, callback context/priority, list refs, bounding volumes,
  and `OptCatalogDamageHandler* damageHandler`.
- BN has separate payload records such as `zClass_WorldData`,
  `zClass_Object3DData`, `zClass_CameraData`, `zClass_LightData`,
  `zClass_DisplayData`, `zClass_WindowData`, `zClass_SoundData`,
  `zClass_AnimateData`, `zClass_SequenceData`, `zClass_LodData`, and
  `zClass_SwitchData`.
- No current evidence requires modeling every `zClass_*` payload as a compiler
  C++ vtable hierarchy.

Important candidates:

| Candidate | Likely role | Recommended implementation |
|---|---|---|
| `zClass_Node` | Scene graph node base. | Core tagged node record with transform, child/sibling links, class data pointer/union, callbacks. |
| `zClass_World` / `zClass_WorldData` | World root/environment settings. | Non-polymorphic object with static/free functions. |
| `zClass_Object3D` / `zClass_Object3DData` | Mesh/model scene object. | Node class data + static methods. |
| `zClass_Camera` / `zClass_CameraData` | Camera node and frustum/runtime data. | Static class functions over node/data. |
| `zClass_Light` / `zClass_LightData` | Light node and parameters. | Static class functions over node/data. |
| `zClass_DisplayData`, `WindowData`, `SoundData`, `AnimateData`, `SequenceData`, `LodData`, `SwitchData` | Node-specific payloads. | Payload records under a scene-node class tag. |
| `zClass_cls_di` | Dynamic-intersection/collision scene query. | Static query subsystem over grid/DI records. |

Implementation notes:

- Functions named `zClass_Camera::gwCameraGetTarget`, `zClass_World::AddLight`, `zClass_Light::gwLightSetRange`, etc. are probably C-style API functions, not necessarily virtual methods.
- `zClass_Class::gwNodeSetRaycastable` and similar names indicate a common node API.
- Use a tagged node plus function tables only if actual vtables are found. Current evidence supports a data-driven engine class system.

## 8.2 `zDi` and collision/picking — confidence C/B

Candidates:

| Candidate | Role |
|---|---|
| `zDi` | Dynamic-intersection subsystem. |
| `zDiEntry` | Registration/query entry. |
| `zClass_cls_di_GridData` | Spatial grid. |
| `zClass_cls_di_GridCell` | Grid bucket/cell. |
| `zClass_DiQueryOutputBuffer` | Query output. |
| `zClass_DiPickCandidate` | Pick/raycast candidate. |
| `zClass_DiIntersectionHitBuffer` | Hit results. |

Implementation notes:

- `zClass_cls_di` methods have many functions and no normal C++ vtable evidence. Treat as a static module.
- Player collision, AI line-of-sight, and model picking rely on this subsystem.

---

# 9. Rendering, video, image, and model systems

## 9.1 `zVideo` / `zVid` / `zRndr` — confidence C with several B records

These are mostly static subsystems with global queues and configuration records.

| Candidate | Confidence | Role |
|---|---:|---|
| `zVideo` | C | High-level video mode/surface lifecycle, frame tick, hotkeys, pass-3 config. |
| `zVid` | C | Image/surface low-level helpers and blitting. |
| `zVideo_dd` | C | DirectDraw backend helpers. |
| `zVideo_dd3d` | C | Direct3D backend helpers. |
| `zRndr` | C | Software/hardware renderer queues, span/polygon rasterization, lens flare, transparent queue. |
| `zVideoFxPass3Config` | A | Global pass-3 overlay-effect singleton. |
| `zVideoFxPass3Element`, `RootElement`, `Slot` | A | UI-like function-table classes. |
| `zVideo_BltSourceHead` | B | Common image/surface header record. |
| `zVid_Image` | B/C | Image object plus static image functions. |

Implementation notes:

- `zRndr` queue globals such as transparent draw-command queue and lens-flare sample queue are records, not classes.
- `zVideoFxPass3*` is an exception: it has confirmed ftable/vtable entries and should be implemented as classes.
- DirectDraw/Direct3D COM interfaces are imported/external; wrap them with backend classes only in the new code, not when matching original class evidence.

## 9.2 `zImage` and texture records — confidence B/C

Candidates:

| Candidate | Role |
|---|---|
| `zImage_TexDirEntry` | Texture directory/material entry. |
| `zImage_Font` | Font/image font resources. |
| `zVid_TexturePackFileHeader` | Texture pack file header. |
| `zVid_TexturePackRecord` | Texture pack entry. |
| `zVid_Image` | Runtime image/surface object. |

Implementation notes:

- Keep `zImage` as a texture manager namespace with records; no strong vtable evidence for a polymorphic class.
- Many HUD/ZRD widgets reference images by path via texture directory find/create functions.

## 9.3 `zModel` and geometry — confidence B/C

Candidates:

| Candidate | Role |
|---|---|
| `zModel` | Model resource and runtime mesh data. |
| `zModel_Instance` | Per-node model instance data. |
| `zModel_Material`, `zModel_MaterialSlot`, `zModel_TextureScrollState` | Material/texture state. |
| `zModel_Polygon`, `zModel_PointEntry`, `zModel_PickFaceEntry` | Geometry and picking data. |
| `zGeometry_*` | Static geometry algorithms and buffer records. |
| `zGeometry_Weiler*` | Polygon clipping/Weiler algorithm records. |

Implementation notes:

- `zGeometry` functions are largely namespace-style algorithms.
- `zModel` is a record/resource class family, not clearly polymorphic.
- Keep model pick/query records exact because collision and AI probes depend on them.

---

# 10. Input subsystem

## 10.1 `zInput` static subsystem — confidence C

`zInput` has many functions and global state for keyboard/mouse/joystick, bind maps, DirectInput lifecycle, and force feedback.

Important records/classes:

| Candidate | Confidence | Role |
|---|---:|---|
| `zInput_GlobalState` | B | Global input runtime state. |
| `zInput_BindMapContext` | B | Command binding arrays and callbacks. |
| `zInput_BindMapOverlayStackNode` | B | Overlay stack for modal bind maps. |
| `zInput_BindGroupInfo` / `Vec` | B | Command group metadata. |
| `zInput_DeviceRegistry` | B | Device enumeration/registry. |
| `zInput_JoystickAxisConfig` | B | Axis dead-zone/scale config. |
| `zInput_FFEffectSet` | B | Force-feedback effect set. |

Implementation notes:

- Keep `zInput::Keyboard_*`, `Mouse_*`, `DI_*`, and bind-map functions grouped as a namespace/module.
- The command callback table is tied to gameplay and HUD hotkeys; set it up during app/play-state activation.

## 10.2 DirectInput wrapper classes — confidence A/B

Confirmed COM-like vtable records:

| Class/record | Role |
|---|---|
| `zInput_DirectInput` | DirectInput root wrapper. |
| `zInput_DirectInputDevice` | DirectInput device wrapper. |
| `zInput_DIDevice` | COM-like DirectInput device interface pointer. |
| `zInput_DiEffect` | DirectInput effect wrapper. |

Implementation notes:

- These are COM-interface-shaped vtables. Treat them as backend wrappers or imported external interface layouts.
- Reimplementation can use an abstraction layer, but preserve original method names in wrapper modules for agents matching functions.

---

# 11. Sound subsystem

## 11.1 `zSnd` and related records — confidence C/B

Likely static subsystem with multiple backend object records.

| Candidate | Confidence | Role |
|---|---:|---|
| `zSnd` | C | Main sound namespace: sample sets, playback, CD audio, options. |
| `zSndDevice` | A/B | Sound backend device vtable. |
| `zSndBuffer` | A/B | Sound buffer vtable. |
| `zSndDirectSoundBuffer` | A/B | DirectSound-backed buffer. |
| `zSndListener` | A/B | 3D listener backend wrapper. |
| `zSndSample`, `zSndSampleSet` | B | Loaded sample resources. |
| `zSndPlayHandle` | B | Active playback/voice handle. |
| `zSndGroup`, `zSndGroupSampleSet`, `zSndVoice` | B | Grouping and voice data. |
| `zSndCdTrackList` | B | CD track metadata. |
| `zSndFadeLists` | B | Fade state lists. |

Implementation notes:

- Sound uses DirectSound and A3D paths. Backend objects may be polymorphic through explicit vtables.
- Keep `zSnd` high-level APIs static, with backend device/buffer objects below.

## 11.2 A3D wrapper classes — confidence A/B

Likely COM-like wrappers:

| Class | Role |
|---|---|
| `zA3dApi` | A3D API root. |
| `zA3dGeom` | A3D geometry. |
| `zA3dListener` | A3D listener. |
| `zA3dSource` | A3D source. |

Implementation notes:

- These should be backend abstractions in a modern port.
- Do not mix A3D objects into `zSndSample` records.

---

# 12. Networking and Westwood Online

## 12.1 `zNetwork` / `zNetworkDPlay` — confidence C/B

Candidates:

| Candidate | Confidence | Role |
|---|---:|---|
| `zNetwork` | C | Game network message dispatch and player/session records. |
| `zNetworkDPlay` | C | DirectPlay integration module. |
| `zDPlayIface` | A/B | DirectPlay interface wrapper with vtable. |
| `zNetworkPacketHeader`, `zNetworkPacketIdAndLen` | B | Packet headers. |
| `zNetworkDispatchHandlerRecord/ListNode` | B | Handler table. |
| `zNetworkPlayerRecord/List/Node` | B | Player records. |
| `zNetworkDPlaySessionDesc/Cache` | B | Session metadata/cache. |
| `zNetworkServiceProviderListVec` | B | Service providers. |

Implementation notes:

- `zNetwork::InitMessageHandlers` is a CRT init target; make message dispatch registration explicit.
- Game-specific network packets (`NetPkt*`) are POD serialization records, not classes.

## 12.2 `GameNet` gameplay network layer — confidence C/B

Candidates:

| Candidate | Role |
|---|---|
| `GameNet` namespace | Multiplayer game-mode network logic. |
| `GameNetSpawnPoint` / `ZrdArray` | Spawn definitions. |
| `GameNetPlayerRow`, `GameNetPlayerRowList`, `GameNetRowHudWidget` | Scoreboard/player-row UI/data. |
| `GameNet` packet handlers | Player colors, state snapshots, kills, scoreboard, lap progress, pickup/crater/qsand/effect events. |

Implementation notes:

- Keep network packet structs exact; dispatch can be a modern registry.
- `GameNetSpawnPointList::InitGlobals` and `GameNetPlayerRowList::Reset` are CRT init/reset entries.

## 12.3 Westwood Online upgrade/API classes — confidence A/B

Confirmed class-like objects:

| Class | Role |
|---|---|
| `WestwoodOnlineUpgradeApi` | API object with vtable and state. |
| `WestwoodOnlineUpgradeApiEventSink` | COM/event sink vtable. |
| `WestwoodOnlineUpgradeDownload` | Download object with vtable. |
| `WestwoodOnlineUpgradeDownloadEventSink` | Download event sink. |
| `WestwoodOnlineUpgradeDialog` | MFC dialog. |
| `WestwoodOnlineUpgradeConfigDialog` | MFC config dialog. |
| `WestwoodOnlineUpgradeProgressDialog` | MFC progress dialog. |
| `WestwoodOnlineUpgradeBootstrapServer`, `BrowseRecord`, `SelectionNode`, `WaitHandles`, etc. | Data records. |

Implementation notes:

- Separate MFC UI from API/download event sinks.
- COM-like event sinks should be small reference-counted wrappers in the original style.

---

# 13. Options, reader, interpreter, and scripting

## 13.1 `zOpt` options/profile system — confidence C/B

Candidates:

| Candidate | Role |
|---|---|
| `zOpt` namespace | Option storage, profile rules, hardware/software mode options. |
| `zOptionEntry` | Base option record. |
| `zOptionEntry_Int32` | Integer option. |
| `zOptionEntry_String` | String option. |
| `zOptionEntry_CameraSection` | Camera section option. |
| `zOptionEntry_ViewRectSection` | View-rect option. |
| `zOpt_ProfileRuleEntry/ListNode/ListArray/CondArray` | Auto-detect/profile rules. |
| `zOpt_ViewRectSection`, `zOpt_CameraSection` | Config sections. |

Implementation notes:

- The code has many option string constants (`Display`, `Render`, `Camera`, `SoundVolume`, `TextureMemory_HW`, etc.). Keep these as canonical config keys.
- `zOpt` should remain a subsystem namespace with option record classes.

## 13.2 `zReader` parse tree — confidence B/C

Candidates:

| Candidate | Role |
|---|---|
| `zReader_Node` | Generic parsed node. |
| `zReader_NodeArray*` and typed node arrays | Strongly typed views for ZRD/ZAR sections. |
| `zReader_Array_*` | Typed arrays. |
| `ZarSectionContext` | ZAR/ZRD section parse context. |

Implementation notes:

- Treat `zReader` as data parser and typed-node facade, not as a polymorphic class hierarchy unless vtables are found.
- Many HUD and AI classes load from `zReader_Node`/ZRD nodes.

## 13.3 `zInterp_Context` — confidence A/B

Candidates:

| Candidate | Role |
|---|---|
| `zInterp_Context` | Script/interpreter context with vtable. |
| `zInterp_GlobalContext` | Static singleton derived/specialized context. |
| `zInterp_FileFrame`, `PreparedScriptEntry`, `MacroEntry`, `VarEntry`, `RuntimeBlob`, etc. | Interpreter data records. |

Implementation notes:

- `zInterp_GlobalContext::StaticInitAndRegisterAtExit` constructs a global context and installs `g_zInterp_GlobalContext_VTable`.
- Preserve global context lifecycle through init/atexit or modern singleton equivalent.

---

# 14. Effects, weapons, pickups, mission systems

## 14.1 `zEffect` and `zEffect_Anim` — confidence C/B

Candidates:

| Candidate | Role |
|---|---|
| `zEffect` namespace | Generic effect/event functions. |
| `zEffect_Anim` namespace/class-like manager | Effect animation loading, activation, save/load, runtime. |
| `zEffect_RuntimeManager` | Runtime template/entry manager. |
| `zEffect_RuntimeEntry` | Active/runtime effect template entry. |
| `zEffectAnimEntry`, `zEffectAnimActivationRecord`, `zEffectAnimTrackedNode`, etc. | Animation runtime records. |
| `zEffect*Event` structs | Typed effect event payloads. |

Implementation notes:

- The effect system is data-driven. Most event records are POD structs.
- `zEffect_Anim::AllocActivationRecord` grows a global table and returns a record pointer. Treat as manager/static subsystem.

## 14.2 `OptCatalog` weapon/object catalog — confidence C/B

Candidates:

| Candidate | Role |
|---|---|
| `OptCatalog` namespace | Weapon/object catalog runtime. |
| `OptCatalogEntryDef` | Catalog entry definition. |
| `OptCatalogRuntimeInstance` | Spawned runtime catalog instance. |
| `OptCatalogDamageHandler`, `DamageAnim`, `DamageEffectThresholdEntry` | Damage handling data. |
| `OptCatalogFxSpec`, `ImpactInfo`, `ImpactResponseEntry` | FX/impact behavior. |
| `OptCatalogTrailRuntimeState`, `TrailSegment`, `TrailNodeSlot` | Projectile/trail runtime. |
| `OptCatalogRaycastHit/List` | Raycast result records. |

Implementation notes:

- Treat as a catalog manager plus records, not a single giant class.
- `Light::AllocFromFreeListAndAttach` and `Light::ReturnToFreeList` operate with opt catalog thermal glow state; keep as a small light pool helper.

## 14.3 Pickups — confidence C/B

Candidates:

| Candidate | Role |
|---|---|
| `Pickup` namespace | Pickup type lookup, spawn, carrier-node spawn, network/save. |
| `PickupType` | Pickup metadata, opt entry, image, amount. |
| `PickupNode` | Runtime pickup node. |
| `PickupSpawnList` | Static list initialized by CRT entry. |
| `PickupRespawnQueue` | Static respawn queue initialized by CRT entry. |
| `PickupSpawnDef`, `PickupRespawnEntry`, `PickupArchiveRecord` | Data records. |

Implementation notes:

- The functions are namespace-style; records/lists are the class-like parts.
- `Pickup::SpawnAtCarrierNodeByName()` looks up a `zClass_Node`, reads position/rotation, and spawns pickup runtime.

## 14.4 Mission/briefing/objective systems — confidence C/B

Candidates:

| Candidate | Role |
|---|---|
| `Mission` namespace | Objective and multiplayer-map registration. |
| `HudSensorTracker` | Mission/HUD objective tracker manager. |
| `HudLoadingCheckpointTable` | Loading checkpoints. |
| `HudUiObjective*`, `HudUiSensor*`, `HudUiReticle*`, `HudUiStats*` | HUD layout/runtime records. |

Implementation notes:

- `Mission::InitObjectives` and `Mission::RegisterMultiplayerMaps` are CRT init targets.
- `HudSensorTracker` should be a manager/singleton with mission data and late mission data records.

---

# 15. Low-level/system support classes and records

## 15.1 COM helper classes — confidence A/B

Candidates:

| Candidate | Role |
|---|---|
| `zComObject` | Generic COM object wrapper with vtable. |
| `zComInterfaceMapEntry` | Interface map record. |
| `IUnknown`, DirectX COM interface structs | External interface layouts. |
| `IConnectionPoint`, `IConnectionPointContainer` | COM event subscription interfaces. |

Implementation notes:

- Keep DirectX and COM interfaces as external ABI layouts.
- Original code probably used small C++ wrappers around COM ref-counting.

## 15.2 Math and geometry records — confidence C/B

Candidates:

| Candidate | Role |
|---|---|
| `zMath` namespace | Vector/matrix/quaternion utilities. |
| `zVec3`, `zMat4x3`, `zBasis3x3`, `zQuat` | Math records. |
| `zGeometry_*` | Algorithm records and static functions. |

Implementation notes:

- `zMath` is a namespace/static utility group, not a class.
- Do not add virtuals here.

## 15.3 Archive/file systems — confidence B/C

Candidates:

| Candidate | Role |
|---|---|
| `zArchiveList` | Archive/free list record. |
| `zIndexArchive` | Indexed archive object/record. |
| `zZbd*` writer/preload contexts | ZBD archive/load helpers. |
| `zUtil` namespace | Mission path setup, save-game helper, general utility. |

Implementation notes:

- Archive systems are mostly static functions and records.
- Preserve file path constants and mission path conventions.

---

# 16. Concrete vtable/function-table inventory by family

This is not exhaustive, but these are strong class anchors agents should use when grouping functions. Generated table labels below are advisory; prefer current BN local type names and data xrefs when a label differs.

## App/state vtables

| Table/type anchor | Class/interface |
|---|---|
| `g_RecoilApp_Vtbl` | `RecoilApp` |
| `g_RecoilApp_MpExitDialogState_Vtbl` | `RecoilApp_MpExitDialogState` |
| `g_RecoilApp_LeaveNetworkState_Vtbl` | `RecoilApp_LeaveNetworkState` |
| `g_RecoilApp_MainMenuPrepState_Vtbl` | `RecoilApp_MainMenuPrepState` |
| `g_RecoilApp_IntroFmvState_Vtbl` | `RecoilApp_IntroFmvState` |
| `g_RecoilApp_AttractFmvState_Vtbl` | `RecoilApp_AttractFmvState` |
| `g_RecoilApp_MissionFmvState_Vtbl` | `RecoilApp_MissionFmvState` |
| `g_RecoilApp_PlayState_Vtbl` | `RecoilApp_PlayState` |
| `g_RecoilStateBase_Vtbl` | `RecoilStateBase` |
| `g_RecoilStateCheatCode_Vtbl` | `RecoilStateCheatCode` |
| `g_RecoilStateControls_Vtbl` | `RecoilStateControls` |
| `g_RecoilStateCredits_Vtbl` | `RecoilStateCredits` |
| `g_RecoilStateConfirmQuit_Vtbl` | `RecoilStateConfirmQuit` |
| `g_RecoilStateMainMenuTransition_Vtbl` | `RecoilStateMainMenuTransition` |
| `g_RecoilStateSaveLoadTransition_Vtbl` | `RecoilStateSaveLoadTransition` |
| `g_HudCmdDialogState_Vtbl` | `HudCmdDialogState` |

Current BN type anchors include `RecoilApp_Vtbl`, `RecoilApp_MfcOleModule_Vtbl`,
and `RecoilApp_IState_Vtbl`.

## MFC/window vtables

| Table/type anchor | Class |
|---|---|
| `g_CZRecoilFrame_VTable` | `CZRecoilFrame : CFrameWnd` |
| `g_CZGameFrame_VTable` | `CZGameFrame : CFrameWnd` |
| `g_CAboutDlg_Vtbl` | `CAboutDlg : CDialog` |
| `g_NetSessionBrowserDialog_Vtbl` | `NetSessionBrowserDialog` |
| `g_NetSessionConfigDialog_Vtbl` | `NetSessionConfigDialog` |
| `g_WestwoodOnlineUpgradeConfigDialog_Vtbl` | `WestwoodOnlineUpgradeConfigDialog` |
| `g_WestwoodOnlineUpgradeDialog_Vtbl` | `WestwoodOnlineUpgradeDialog` |
| `g_WestwoodOnlineUpgradeProgressDialog_Vtbl` | `WestwoodOnlineUpgradeProgressDialog` |

## HUD/UI ftable families

| Table/type anchor | Class/family |
|---|---|
| `HudUiCommon_FTable` | `HudUiElement` / common base |
| `HudUiWidget_FTable` | `HudUiWidget` |
| `HudUiZrdWidget_FTable` | `HudUiZrdWidget` |
| `HudUiPanel_FTable` | `HudUiPanel` |
| `HudUiPanelCore_FTable` | `HudUiPanelCore` |
| `HudUiContainer_FTable` | `HudUiContainer` |
| `HudUiBackground_FTable` | `HudUiBackground` |
| `HudUiMessage_FTable` | `HudUiMessage` |
| `HudUiMeter_FTable` | `HudUiMeter` |
| `HudUiCounter_FTable` | `HudUiCounter` |
| `HudUiSlot_FTable` | `HudUiSlot` |
| `HudUiTriplet_FTable` | `HudUiTriplet` |
| `HudUiTextInput_FTable` | `HudUiTextInput` |
| `HudUiCheckToggleWidget_FTable` | `HudUiCheckToggleWidget` |
| `HudUiCycleSelectorWidget_FTable` | `HudUiCycleSelectorWidget` |
| `HudUiClampedIntStepButton_FTable` | `HudUiClampedIntStepButton` variants |
| `HudUiClampedIntTextInput_FTable` | `HudUiClampedIntTextInput` variants |
| `HudUiBriefingRuntime_VTable` | `HudUiBriefingRuntime` |
| `HudUiBriefingActionVtable` | Briefing action subclasses |
| `HudWeatherFxVtable` | Weather FX classes |
| `zVideoFxPass3RootElement_VTable` | `zVideoFxPass3RootElement` |
| `zVideoFxPass3Slot_FTable` | `zVideoFxPass3Slot` |

## FMV vtables

| Table/type anchor | Class |
|---|---|
| `zFMV_ActionVtable` | `zFMV_Action` |
| `g_zFMV_ActionWait_Vtable` | `zFMV_ActionWait` |
| `g_zFMV_ActionFadeIn_Vtable`, `FadeOut` | `zFMV_ActionFade` variants |
| `g_zFMV_ActionBlur_Vtable` | `zFMV_ActionBlur` |
| `g_zFMV_ActionImage_Vtable` | `zFMV_ActionImage` |
| `g_zFMV_ActionPlayAvi_*` | `zFMV_ActionPlayAvi` variants |
| `g_zFMV_ActionPlayMci_Vtable` | `zFMV_ActionPlayMci` |
| `g_zFMV_ActionPlaySound_Vtable` | `zFMV_ActionPlaySound` |

## Network/WOL vtables

| Table | Class |
|---|---|
| `g_WestwoodOnlineUpgradeApi_Vtbl` | `WestwoodOnlineUpgradeApi` |
| `g_WestwoodOnlineUpgradeApiEventSink_Vtbl` | API event sink |
| `g_WestwoodOnlineUpgradeDownload_Vtbl` | Download object |
| `g_WestwoodOnlineUpgradeDownloadEventSink_Vtbl` | Download event sink |
| `g_zDPlayIface_VTable` | DirectPlay interface wrapper |

---

# 17. CRT singleton/static-object candidates

The CRT init array is a strong guide for global singleton reconstruction. These should usually become explicit global objects or singleton managers in the reimplementation:

| CRT init entry | Suggested reconstructed owner |
|---|---|
| `RecoilStateCheatCode::StaticInitAndRegisterAtExit` | `g_RecoilState_CheatCode` |
| `RecoilStateControls::StaticInitAndRegisterAtExit` | `g_RecoilState_Controls` |
| `RecoilStateCredits::StaticInitAndRegisterAtExit` | `g_RecoilState_Credits` |
| `HudCmdDialogState::StaticInitAndRegisterAtExit` | `g_HudCmdDialogState` |
| `HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit` | options overlay singleton/owner |
| `HudUiTriplet::StaticInitWndClassNameAndRegisterAtExit` | triplet window class/static name |
| `HudLayoutSW::CrtInitGlobalSingleton` | software HUD layout singleton |
| `HudLayoutHW::CrtInitGlobalSingleton` | hardware HUD layout singleton |
| `HudUiMgr::StaticInitAndRegisterAtExit` | global HUD manager |
| `HudUiSensorWindow::StaticInitAndRegisterAtExit` | sensor window singleton |
| `zInterp_GlobalContext::StaticInitAndRegisterAtExit` | global script/interpreter context |
| `RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit` | main-menu transition state singleton |
| `RecoilStateConfirmQuit::StaticInitAndRegisterAtExit` | confirm-quit state singleton |
| `Mission::InitObjectives` | objective registry |
| `HudUiNetGameSetupOverlayOwner::StaticInitAndRegisterAtExit` | net setup overlay singleton |
| `HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit` | new-game overlay singleton |
| `Mission::RegisterMultiplayerMaps` | multiplayer map registry |
| `PickupSpawnList::Primary_Init` | primary pickup spawn list |
| `PickupSpawnList::NetCopy_Init` | network pickup spawn list |
| `PickupRespawnQueue::Init` | pickup respawn queue |
| `Player::InitMasterCommonDataList` | player common-data list globals |
| `Player::InitMasterModalDataList` | player modal-data list globals |
| `Player::InitAndRegisterUnderwaterFxPass3UiSingleton` | underwater screen-FX singleton |
| `Player::InitAndRegisterProjectileCameraFxPass3UiSingleton` | projectile-camera screen-FX singleton |
| `Player::InitSaveStateList` | player save-state intrusive list |
| `Player::InitAndRegisterTopMsgPanel1/2` | top-message panel singletons |
| `PlayerNodeFlagRestore::InitGlobals` | node flag restore list |
| `zInput::BindGroupList_StaticInitAndRegisterAtExit` | input bind-group metadata |
| `RecoilApp::StaticInitAndRegisterAtExit` | live `g_RecoilApp` singleton |
| `GameNetSpawnPointList::InitGlobals` | multiplayer spawn points |
| `GameNetPlayerRowList::Reset` | multiplayer scoreboard/player rows |
| `RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit` | save/load state singleton |
| `zDEClient::InitFeatureSystem` | terrain/deformation feature system |
| `zInput::GlobalStateStaticInitAndRegisterAtExit` | global input state |
| `zNetwork::InitMessageHandlers` | packet-handler registry |
| `zSnd::SetUseArchiveBanksAndRegisterAtExit` | sound archive/sample-bank mode |
| `zSndCdTrackList::StaticInit` | CD track list |
| `zSndFadeLists::Init` | sound fade lists |
| `zVideoFxPass3Config::CrtInitGlobalSingleton` | pass-3 overlay config singleton |

---

# 18. Source-module grouping hints

Use these original-source comments from HLIL as grouping hints. They are not proof of class identity, but they are useful for organizing files.

| Source module hint | Likely reconstructed module/classes |
|---|---|
| `D:\Proj\Battlesport\RecoilApp.cpp` / `GameZRecoil/RecoilApp/RecoilApp.cpp` | `RecoilApp`, state stack, app states, frame loop. |
| `D:\Proj\Battlesport\CZRecoilFrame.cpp` | MFC frame/window and menu command handlers. |
| `D:\Proj\Battlesport\hud.cpp`, `HudUiPanel.cpp`, `HudUiTriplet.cpp` | HUD core, panels, containers, widgets. |
| `HudConfirmQuitDialog.cpp`, `HudOptionsDialog.cpp`, `HudUiSaveLoadDialog.cpp`, etc. | HUD modal dialogs and state wrappers. |
| `D:\Proj\Battlesport\player.cpp`, `GameZRecoil/Player.cpp` | Player subsystem, AI, controls, weapons, camera, collision. |
| `D:\Proj\Battlesport\ai_net.cpp` / `src/Battlesport/ainet.cpp` | `AINet`, path nodes, probe fans. |
| `D:\Proj\Battlesport\Briefing.cpp` | Briefing runtime and action queue. |
| `D:\Proj\Battlesport\pickup.cpp` | Pickup subsystem. |
| `GameZRecoil/zClass/*.c` | Scene graph node system. |
| `GameZRecoil/zVideo/*.cpp` | Video backend and surface logic. |
| `GameZRecoil/zImage/*.cpp` | Texture/image management. |
| `GameZRecoil/zEffect/*.c` | Effect animation/runtime. |
| `GameZRecoil/zWeapon/OptCatalog.c` | Weapon/object catalog. |
| `GameZRecoil/westwoodonline/*.cpp` | Westwood Online dialogs, API, event sinks. |
| `GameZRecoil/zInterp/*.cpp` | Script/interpreter context. |

---

# 19. Suggested reconstruction plan for agents

## Phase 0 — Exact record layout

Start with exact POD layouts from current BN local types. Do not add C++ virtuals or base classes beyond what the ftable/vtable evidence supports. Critical records:

- `RecoilApp`, `RecoilApp_IState`, state structs, `RecoilApp_StateQueue`
- `HudUiElement`, `HudUiWidget`, `HudUiPanel`, `HudUiContainer`, `HudUiBackground`, `HudUiZrdWidget`
- `PlayerState`, `PlayerMasterCommonData`, `PlayerMasterModalData`, `PlayerModalState`, weapon/contact/save structs
- `AINet`, `AINetNode`, `AINetPathProbeFan`
- `zClass_Node` and node data records
- `zModel`, `zImage`, `zVideo`, `zRndr`, `zSnd`, `zInput`, `zNetwork` records

## Phase 1 — Polymorphic class shells

Implement confirmed vtable/ftable classes first:

1. `RecoilApp` and `RecoilApp_IState` hierarchy.
2. `HudUiElement`/`Widget`/`Panel`/`Container`/`Background` hierarchy.
3. `zFMV_Action` hierarchy.
4. `WestwoodOnlineUpgrade*` event sinks and dialogs.
5. `zVideoFxPass3*` and `HudWeatherFx*` overlay classes.
6. MFC frame/dialog shell classes if Windows/MFC compatibility is required.

## Phase 2 — Static subsystem modules

Implement these as namespaces/modules with explicit state objects:

- `Player`
- `zVideo`, `zVid`, `zRndr`
- `zInput`
- `zSnd`
- `zNetwork`, `GameNet`
- `zClass_Class`, `zClass_World`, `zClass_Camera`, `zClass_Object3D`, etc.
- `zMath`, `zGeometry`, `zReader`, `zOpt`, `Pickup`, `OptCatalog`, `zEffect_Anim`

## Phase 3 — Data-driven loaders and UI composition

Then wire data-driven systems:

- ZRD/ZAR parse tree through `zReader_Node` and typed node arrays.
- HUD layout loading through `HudLayoutSW/HW` and `HudUiMgr`.
- `dialog.zrd`, `briefing.zrd`, `ai_paths.zrd`, mission archives.
- FMV actions from `fmv.zrd`.
- `zInterp_GlobalContext` script loading and prepared scripts.

## Phase 4 — Behavior parity

Finally, focus on behavior parity for:

- app state transitions and fade/mute timing
- player input/camera/AI/weapons/collision
- network packet processing
- save/load sorting and save entry metadata
- HUD invalidation/update/draw order
- sound/video backend timing

---

# 20. Rules of thumb for deciding “class or namespace”

Use these rules when assigning remaining functions:

1. **Constructor writes a specific vtable/ftable to offset 0** → class, confidence A.
2. **Destructor restores a base vtable/ftable** → inheritance or base-class destruction, confidence A.
3. **Struct has `base` as the first field** → inheritance-like layout, confidence A/B.
4. **Large function group but every function takes a separate state pointer** → namespace/subsystem plus record, confidence C/B.
5. **Functions operate on globals only** → namespace/subsystem, confidence C.
6. **COM-like QueryInterface/AddRef/Release vtable** → wrapper/interface class, confidence A/B.
7. **Scene graph functions named `gwXxx` over `zClass_Node*`** → data-driven node class, not ordinary C++ inheritance.
8. **Many ftable variants differ only in `OnActivate`** → do not invent deep classes unless the object has extra fields. Use small derived button classes or callback strategy objects.
9. **Imported MFC/CRT thunks** → not authored classes; keep as external/runtime glue.
10. **BN x87/SEH artifacts** → do not encode artifacts as source logic. Recover plain float comparisons and RAII/C++ exception cleanup.

---

# 21. High-priority class skeletons

## 21.1 App state interface

```cpp
struct RecoilApp_IState {
    RecoilApp_IState_Vtbl* vftable;
};

class RecoilAppState {
public:
    virtual ~RecoilAppState();
    virtual void OnWndActivate(RecoilWndActivateState) {}
    virtual void OnEnter() {}
    virtual bool OnTryBecomeCurrent() { return true; }
    virtual bool OnUpdateShouldQuit() { return false; }
    virtual void OnExit() {}
    virtual void OnDeactivate() {}
    virtual void OnSuspend(int) {}
    virtual void OnResume(int) {}
    virtual bool OnIdleOrDispatch(int, int) { return true; }
};
```

Use the C++ form above for modern code, but preserve the binary ftable layout if matching original ABI.

## 21.2 HUD element/widget base

```cpp
struct HudUiElement {
    HudUiCommon_FTable* ftable;
    HudUiElement* next;
    void* parent;
    uint32_t flags;
    float timer;
    int x, y;
    zVideo_BltSourceHead* bltSource;
    HudUiRect clipRect;
    uint16_t state;

    void Draw();       // ftable->DrawBase(this)
    void DrawBase();   // blit if bltSource
    void SetPos(int x, int y);
    void SetVisible(bool visible);
};

struct HudUiWidget : HudUiElement {
    // widget-specific fields from user_types layout
    // bounds, owner, ZRD/load state, callbacks depending on subtype
};
```

## 21.3 FMV action base

```cpp
struct zFMV_Action {
    zFMV_ActionVtable* vftable;
    zFMV_Action* next;
};

struct zFMV_Script {
    char* fmvPath;
    HWND hwnd;
    double startTimeSec;
    int abortOnKey;
    zFMV_Action* head;
    zFMV_Action* tail;
    zFMV_Action* cur;
};
```

## 21.4 AINet

```cpp
struct AINetNode {
    zVec3 position;
    AINetNeighborSlot neighborRefs[3];
    AINetPathProbeFan* probeFans[3];
    int costOrType;
    int nodeIndex;
    AINetNode* next;

    static void Free(AINetNode*);
};

struct AINet {
    int netId;
    char name[0x14];
    AINetType aiType;
    float pathWidth;
    float activateRadius;
    float attackRadius;
    float attackDwell;
    float notPursuitDwell;
    float pursuitParam0;
    float pursuitParam1;
    float returnRange;
    float hideTime0;
    float hideTime1;
    int activateBuddyNetId;
    int attackBuddyNetId;
    AINetAttackStrategy attackStrategy;
    AINetNode* nodeListHead;
    AINet* next;

    static void Free(AINet*);
    static void FreeAll();
};
```

## 21.5 Player subsystem pattern

```cpp
namespace Player {
    void TickLocalPlayerControls(zUtil_SaveGameState*);
    void TickAiMode2TopLevel(zUtil_SaveGameState*);
    void UpdateCameraByState(zUtil_SaveGameState*);
    void ResetAltGunRuntimeState(zUtil_SaveGameState*);
    void InitMasterCommonDataList();
    void InitMasterModalDataList();
}

struct PlayerState {
    // Exact layout from current BN local types.
    // Do not replace with virtual class unless later evidence proves one.
};
```

---

# 22. Open uncertainties / cautions

| Area | Uncertainty | Recommendation |
|---|---|---|
| `Player` | Large method namespace, unclear true instance class. | Use static subsystem + `PlayerState` records until a stable vtable or constructor appears. |
| `HudUiPanel` vs `HudUiPanelCore` | Very similar text panel layouts. | Keep both reconstructed names initially; merge only after constructors/layouts prove identity. |
| `RecoilApp` MFC base | Vtable shows MFC/CWinApp/COleControlModule imports and custom module naming. | Model as `CWinApp`-derived app shell; do not overfit MFC internals unless matching ABI. |
| `zClass_*` | Shared engine object API, not normal C++ vtables. | Implement tagged scene graph and class data records. |
| DirectX/COM wrappers | Some vtables are external ABI interfaces, not authored classes. | Keep external interface layouts separate from game wrapper objects. |
| Function names | Some names were assigned before reconstruction and may be wrong. | Trust field access, vtable slots, constructors, source-file comments, and ownership patterns over names. |
| BN artifacts | x87 comparisons, reused stack slots, SEH frames, and placeholder table slots are noisy. | Translate to normal C++ comparisons, RAII destructors, and clear local variables; do not promote placeholder slots without table/xref evidence. |

---

# 23. Recommended reimplementation priority list

1. **Core records**: `zVec3`, `HudUiRect`, `zVideo_BltSourceHead`, `zClass_Node`, `PlayerState`, `AINet`, `RecoilApp` records.
2. **App state system**: `RecoilApp`, `RecoilApp_IState`, state queue, embedded state objects.
3. **HUD base hierarchy**: `HudUiElement`, `HudUiWidget`, `HudUiPanel`, `HudUiContainer`, `HudUiBackground`, `HudUiZrdWidget`.
4. **FMV action system**: `zFMV_Script`, `zFMV_Action` hierarchy.
5. **ZRD/ZAR reader and loaders**: `zReader_Node`, HUD layout loading, AINet loading.
6. **Player subsystem records and behavior**: controls, AI mode 2, weapons, camera, collision, save-state list.
7. **Scene graph**: `zClass_Node` and data-driven world/camera/light/object3D functions.
8. **Input/sound/video/network services**: initialize enough backend abstraction to support game loop and UI.
9. **Game systems**: `Pickup`, `OptCatalog`, `GameNet`, `Mission`, `HudSensorTracker`, `zEffect_Anim`.
10. **MFC/WOL/editor dialogs**: port or stub depending on reimplementation target.

---

# 24. Minimal class-candidate checklist

Use this checklist when an agent is about to reimplement a function:

- [ ] Does the first argument behave as `this` and is it stored at `ecx`/`__thiscall`?
- [ ] Does the object at offset `0` contain a vtable/ftable pointer?
- [ ] Does any constructor install a table matching this function family?
- [ ] Does the destructor restore a base table or destruct embedded base objects?
- [ ] Is there a `base` field in current BN local types at offset `0`?
- [ ] Are related methods static subsystem functions over globals instead?
- [ ] Is the method tied to a global singleton from CRT init?
- [ ] Is the function only an imported MFC/CRT/DirectX thunk?
- [ ] Are stack variables corrupted by BN artifacts and should be split manually?

When the answer to the first four questions is yes, promote to a class method. When most answers point to global state and explicit record pointers, keep it as a subsystem function.
