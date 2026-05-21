# Function name cleanup findings

## Scope

Inputs scanned:

- `text.hlil(18).txt`
- `user_types.txt`
- `rdata.linear(18).txt`
- `data.linear.part01.txt` through `data.linear.part04.txt`
- `rsrc.linear(16).txt`

The recommendations below are meant for a rename/cleanup agent. Unless an entry explicitly says otherwise, keep the current prototype and implementation; only rename the symbol and update call-site references/comments that mention the old name.

## Summary

Primary cleanup categories found:

1. **Real class methods currently filed under a module namespace**, such as `zInput::BindMapContext_*` methods whose `self` type is `zInput_BindMapContext*`.
2. **Struct-name formatting mismatches**, such as `zVideo_FxPass3Config::...` when the reconstructed type is `zVideoFxPass3Config`.
3. **Stale semantic names in `Player`**, mostly around gun dispatch and fire-point selection.
4. **Data blobs still represented as functions**, which should be undefined as functions and retained as data symbols.
5. **Non-authored import/EH thunks** that do not follow `Class::Function`; these should be handled separately from authored game-code names.

---

## High-confidence fixes: data currently misidentified as functions

These entries should not remain function definitions. They are either explicitly documented as data or have non-code bodies produced by disassembling data bytes.

| Address | Current symbol | Recommended action | Evidence / implementation note |
|---:|---|---|---|
| `0x430c04` | `g_CZRecoilFrame_HelpDocsFindExecutableErrorRemapTable` | **Undefine as a function.** Keep this name as a data/table symbol. | The preceding comment identifies it as inline help-doc dispatch data: `FindExecutableA` failure codes index a remap table, then dispatch through `g_CZRecoilFrame_HelpDocsErrorJumpTable` at `0x430bf0`. The rendered function body is nonsensical byte arithmetic, consistent with data misdecode. |
| `0x4c7405` | `CRT_Data_MisidentifiedBytes_4c7405` | **Undefine as a function.** Leave as raw data unless a concrete data type is recovered. | Existing comment says BN misidentified a data/code stub and that the bytes do not decode as a meaningful DirectDraw factory thunk. No meaningful code body is present. |
| `0x4c7eca` | `CRT_Data_MisidentifiedBytes_4c7eca` | **Undefine as a function.** Type as a DirectInput data-format/data-object blob after layout verification. | Existing comment says bytes form structured data records. A live use passes `&CRT_Data_MisidentifiedBytes_4c7eca[0x16]` into `zInput_JoystickDevice_1->vtbl->SetDataFormat(...)`, so this should be data, not executable code. |

---

## High-confidence fixes: class/template mismatches

These functions have a reconstructed `self` type or source-name evidence showing that the current class prefix is wrong. Rename to the recommended `Class::Function` form.

### `zInput_BindGroupInfo`

| Address | Current symbol | Rename to | Reason |
|---:|---|---|---|
| `0x42a000` | `zInput::BindGroupInfo_Destroy` | `zInput_BindGroupInfo::Destroy` | The signature is `__thiscall` with `struct zInput_BindGroupInfo* self`, and the body destroys one `zInput_BindGroupInfo` instance. |

### `zOpt_ViewRectSection`

| Address | Current symbol | Rename to | Reason |
|---:|---|---|---|
| `0x4083d0` | `zOpt::ViewRectSection_SetPosition` | `zOpt_ViewRectSection::SetPosition` | First parameter is `struct zOpt_ViewRectSection* self`; this is a method on that struct, not a `zOpt` module function. |
| `0x408400` | `zOpt::ViewRectSection_SetSize` | `zOpt_ViewRectSection::SetSize` | Same pattern as above. It updates the `zOpt_ViewRectSection` payload. |

### `HudUiOptionsPanelBackButton`

| Address | Current symbol | Rename to | Reason |
|---:|---|---|---|
| `0x40c6e0` | `HudUiOptionsPanel_BackButton::OnActivate` | `HudUiOptionsPanelBackButton::OnActivate` | `user_types.txt` defines `HudUiOptionsPanelBackButton`, not `HudUiOptionsPanel_BackButton`, and the function signature uses `struct HudUiOptionsPanelBackButton* self`. |

### `zInput_BindMapContext`

The `zInput_BindMapContext` method cluster is split across three naming styles: correct `zInput_BindMapContext::...`, module-style `zInput::BindMapContext_*`, and collapsed `zInputBindMapContext::...`. Normalize the whole cluster to `zInput_BindMapContext::...`.

| Address | Current symbol | Rename to |
|---:|---|---|
| `0x4707a0` | `zInput::BindMapContext_FreeAllBuffers` | `zInput_BindMapContext::FreeAllBuffers` |
| `0x470820` | `zInput::BindMapContext_RebuildLookupIndices` | `zInput_BindMapContext::RebuildLookupIndices` |
| `0x470960` | `zInput::BindMapContext_FreeNonOwnedBuffers` | `zInput_BindMapContext::FreeNonOwnedBuffers` |
| `0x470a40` | `zInput::BindMapContext_GetPrimaryKeyboardKey` | `zInput_BindMapContext::GetPrimaryKeyboardKey` |
| `0x470a60` | `zInput::BindMapContext_GetSecondaryKeyboardKey` | `zInput_BindMapContext::GetSecondaryKeyboardKey` |
| `0x470a80` | `zInput::BindMapContext_GetJoystickButtonSlot` | `zInput_BindMapContext::GetJoystickButtonSlot` |
| `0x470ac0` | `zInput::BindMapContext_GetCommandByPrimaryKey` | `zInput_BindMapContext::GetCommandByPrimaryKey` |
| `0x470ad0` | `zInput::BindMapContext_GetCommandBySecondaryKey` | `zInput_BindMapContext::GetCommandBySecondaryKey` |
| `0x470b10` | `zInput::BindMapContext_GetCommandByMouseSlot` | `zInput_BindMapContext::GetCommandByMouseSlot` |
| `0x470df0` | `zInput::BindMapContext_SetCommandCallback` | `zInput_BindMapContext::SetCommandCallback` |
| `0x470e80` | `zInputBindMapContext::DispatchFromKeyboardEvent` | `zInput_BindMapContext::DispatchFromKeyboardEvent` |
| `0x470eb0` | `zInputBindMapContext::ReadCommandInputState` | `zInput_BindMapContext::ReadCommandInputState` |
| `0x470f50` | `zInput::BindMapContext_CopyCommandLabel` | `zInput_BindMapContext::CopyCommandLabel` |

Implementation notes:

- Leave already-correct symbols such as `zInput_BindMapContext::InitFromTemplate`, `InitCommandMap`, `ResetAllBindings`, `GetMouseButtonSlot`, `GetCommandByAnyKeyboardKey`, `GetCommandByJoystickSlot`, `SetPrimaryKeyBinding`, `SetSecondaryKeyBinding`, `SetJoystickBinding`, `SetMouseBinding`, `SetBindingRecord`, `DispatchMouseButtonCallbacks`, and `DispatchJoystickButtonCallbacks` as-is.
- After renaming, update callers such as `zInput_BindMapContext::SetCommandCallback`, which currently registers `zInputBindMapContext::DispatchFromKeyboardEvent` as the keyboard callback.

### Current bind-map forwarding helpers

These two are static/current-context forwarders. They should stay under the `zInput` namespace like the adjacent `BindMapCurrent_*` functions, not under the non-type pseudo-class `zInputBindMap`.

| Address | Current symbol | Rename to | Reason |
|---:|---|---|---|
| `0x4717c0` | `zInputBindMap::SetCurrentCommandCallback` | `zInput::BindMapCurrent_SetCommandCallback` | The body forwards to `g_zInput_BindMap_Current->SetCommandCallback(...)`; adjacent helpers use `zInput::BindMapCurrent_*`. |
| `0x4717d0` | `zInputBindMap::ReadCurrentCommandInputState` | `zInput::BindMapCurrent_ReadCommandInputState` | The body forwards to `ReadCommandInputState(g_zInput_BindMap_Current, commandIndex)`. |

### `zVideoFxPass3Config`

The reconstructed type is `zVideoFxPass3Config`. Normalize methods to that class name; do not use `zVideo::FxPass3Config_*` or `zVideo_FxPass3Config::...` for instance methods.

| Address | Current symbol | Rename to |
|---:|---|---|
| `0x4bed30` | `zVideo::FxPass3Config_UpdateLocal` | `zVideoFxPass3Config::UpdateLocal` |
| `0x4bed50` | `zVideo_FxPass3Config::SetPrimaryElementParamsLocal` | `zVideoFxPass3Config::SetPrimaryElementParamsLocal` |
| `0x4bed90` | `zVideo_FxPass3Config::QueueElementLocal` | `zVideoFxPass3Config::QueueElementLocal` |
| `0x4bee20` | `zVideo::FxPass3Config_QueuePrimitiveRaw` | `zVideoFxPass3Config::QueuePrimitiveRaw` |

Leave `zVideoFxPass3Config::SetInputRectByIndex`, `CrtInitGlobalSingleton`, `ConstructGlobalSingleton`, `RegisterDestroyAtExit`, `DestroyGlobalSingleton`, `Destructor`, and `Constructor` as-is.

### `HudUiTextInput` text-buffer helpers

`HudUiTextBuffer` does not appear as a reconstructed struct in `user_types.txt`. These helpers all take `struct HudUiTextInput* self` and are sourced from `HudUiTextInput.cpp`; normalize them to `HudUiTextInput::...`.

| Address | Current symbol | Rename to |
|---:|---|---|
| `0x4b4550` | `HudUiTextBuffer::DeleteCharForward` | `HudUiTextInput::DeleteCharForward` |
| `0x4b4560` | `HudUiTextBuffer::MoveCursorLeft` | `HudUiTextInput::MoveCursorLeft` |
| `0x4b4570` | `HudUiTextBuffer::MoveCursorRight` | `HudUiTextInput::MoveCursorRight` |
| `0x4b4590` | `HudUiTextBuffer::ShiftTextRight` | `HudUiTextInput::ShiftTextRight` |
| `0x4b45e0` | `HudUiTextBuffer::ShiftTextLeft` | `HudUiTextInput::ShiftTextLeft` |

---

## High-confidence fixes: stale or misleading `Player` names

These names are not just formatting issues; they are stale or semantically misleading after reconstruction.

| Address | Current symbol | Rename to | Reason |
|---:|---|---|---|
| `0x43a400` | `Player::ProcessPrimaryGunDispatchRequest` | `Player::ProcessPrimaryGunDispatchTick` | Existing comments and cross-references call this the primary-gun dispatch tick. The function is a once-per-tick handler that consumes `primaryGunDispatchRequested`, performs ammo/charge handling, selects a fire point, and dispatches effects. |
| `0x43aa30` | `Player::SelectAltGunFireOriginAndSlot` | `Player::SelectAltGunFirePointAndSlot` | Existing source comment names it `SelectAltGunFirePointAndSlot`; the body cycles center/right/left hardpoints, selects a `PlayerGunFireSlot`, and computes the origin. “Origin” is only one output, not the selection target. |
| `0x43acf0` | `Player::SelectPrimaryGunFireOriginAndSlot` | `Player::SelectPrimaryGunFirePointAndSlot` | Same pattern as the alt-gun counterpart. Existing source comment and nearby call-site comments use `SelectPrimaryGunFirePointAndSlot`. |
| `0x43c190` | `Player::ProcessAltGunFireDispatchRequest` | `Player::ProcessAltGunDispatchRequest` | Existing source comment and caller comments use `ProcessAltGunDispatchRequest`. The extra `Fire` makes this inconsistent with the surrounding alt-gun dispatch naming. |
| `0x43c330` | `Player::AltGunEnsureAuxEffectActive` | `Player::EnsureGunAuxEffectActive` | Existing comment says the name is legacy: both primary and alt dispatch paths call it with their active gun controller and fire origin. It is not alt-gun-specific. |

### Medium-confidence `Player` review items

These are not necessarily wrong, but the current name diverges from source/call-site wording. Review before changing.

| Address | Current symbol | Candidate rename | Review note |
|---:|---|---|---|
| `0x43c2d0` | `Player::UpdateContinuousAltGunFireController` | `Player::UpdateAltGunFireController` | Source comment uses `UpdateAltGunFireController`. Current name is more descriptive because the helper is used for the continuous/beam alt-gun path. Rename only if source-style consistency is preferred over descriptive specificity. |
| `0x43a900` | `Player::DecayAndApplyAltFireSlotOffsetToNode` | `Player::UpdateAltGunAimDirection` | Source comment uses `UpdateAltGunAimDirection`, but the current name accurately describes the reduced behavior. Review against original source naming goals before changing. |

---

## Optional bulk cleanup: imported MFC thunks

A large block of imported MFC thunks is still named with decorated MSVC import names, for example:

- `0x4c5a56` `?OnCancel@CDialog@@MAEXXZ`
- `0x4c5a5c` `?OnOK@CDialog@@MAEXXZ`
- `0x4c5a68` `?OnInitDialog@CDialog@@UAEHXZ`
- `0x4c5b64` `??0CDialog@@QAE@IPAVCWnd@@@Z`
- `0x4c5e70` `??0CFrameWnd@@QAE@XZ`

These violate the visual `Class::Function` convention, but they are imported MFC/runtime thunks rather than authored game functions. Handle this as a separate import-cleanup pass if desired:

- Demangle method imports to `MFC42::<Class>::<Function>` or `<Class>::<Function>` according to the existing import naming convention.
- Constructors/destructors should become `CDialog::CDialog`, `CDialog::~CDialog`, `CFrameWnd::CFrameWnd`, etc.
- Do not mix this bulk import demangling with the authored-code renames above.

---

## Optional bulk cleanup: compiler-generated EH funclets

There are many compiler-generated unwind funclets and SEH handlers near the CRT/MFC tail of `text.hlil`, including many `sub_4c83xx`-style symbols. These are non-authored compiler artifacts, not game functions. They are not good candidates for game-style `Class::Function` names.

Recommended handling:

- Leave as `sub_*` if the project does not track compiler-generated funclets.
- If you do rename them, use an explicit non-authored prefix such as `MSVC_EH_UnwindFunclet_<parent>_<action>`.
- Do not infer gameplay/domain names from these funclets; most just tail-call a destructor or `operator delete` during exception unwind.

---

## Do-not-change notes from this pass

These symbols have comments indicating that an earlier wrong name was already corrected. Avoid reverting them:

| Address | Current symbol | Note |
|---:|---|---|
| `0x4a7d20` | `zVideo_dd::OpenVideoMode` | Already renamed away from the misleading `EnumerateDevicesAndResetSelection`; the body opens the selected video mode and creates DirectDraw2. |
| `0x4bfb70` | `HudUiBackgroundCursorWidget::SetPos` | Previously mislabeled `SetCaptureEnabled`; current name matches the vtable slot and body. |
| `0x4c5520` | `zInterp_Context::ReportErrorf` | Previously mislabeled `CommandEquals`; current body is error-reporting/log forwarding. |
| `0x472960` / `0x4729f0` | `zMath::Vec3Lerp` / `zMath::Vec3LerpNormalize` | Comments indicate a prior cascade was corrected. Current names match behavior. |
