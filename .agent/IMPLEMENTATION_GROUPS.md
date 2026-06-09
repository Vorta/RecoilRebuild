# Implementation Groups

Use this tracked file for temporary dependency-group notes during
reconstruction. The plan remains address-based; this file lists only active
multi-function, source-readiness, or coherent tier `S` groups currently being
coordinated. Keep the header and template available even when no groups are
active.

## Rules

- Create or update a group before editing when a task touches more than one
  function or a shared type/global/vtable.
- Keep groups scoped. Prefer one class, one source file cluster, one recursive
  cycle, or one call-chain frontier.
- Do not mark plan entries done from this file alone. Plan markers still require
  current source/build/Binary Ninja evidence.
- Keep notes concise and temporary. Move durable facts into source comments,
  Binary Ninja comments, tests, `docs/reconstruction/`, or narrow subsystem docs before
  pruning.
- Stage this file only when an active group update belongs with a qualifying
  source checkpoint under the root `AGENTS.md` git rules. Do not commit stale or
  group-only bookkeeping.
- Verification-only queues that no longer carry source blockers should not live
  in this active working file unless they are coordinating a current coherent
tier `S` pass. Use `.agent/RECOIL_PLAN.md`, `python tools/recoil.py status
  0xNNNNNN`, VC verification manifests, and
  `python tools/recoil.py audit backlog` for current verification state.
- Normal binary-lane planning prioritizes owner structure blockers before
  isolated implementation/behavior work and prioritizes owner/data blockers
  before verify-only tier `S` work. Active verify-only groups should condense
  or move out of this file when any nearby class/source-file/global owner debt
  remains.
- Recompute verification scope with `python tools/recoil.py status 0xNNNNNN` or
  `python tools/recoil.py frontier 0xNNNNNN --depth 1` after source blockers
  clear.
- Use `python tools/recoil.py audit groups --summary --wip-limit 4` to check
  for stale, completed, or overgrown groups.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
- Reason: dependency closure / class cluster / recursive cycle / shared ABI layout / source file cluster
- Source blockers:
  - 0xNNNNNN Name
- Next action:
  - python tools/recoil.py status 0xNNNNNN
```

## Active Groups

### Group: zVideo renderer dispatch/global owner audit

- Anchor: 0x4a77a0 zVideo::BindRendererDispatch
- Reason: shared renderer-dispatch function-pointer globals and cached
  DirectDraw hardware-device data used by zVid memory-query and primary blit
  callers.
- Source blockers:
  - Current BN for 0x4a77a0 writes the renderer dispatch slots at
    0x6333ac..0x6333d4 and 0x56bbfc..0x56bc74, binds the two memory-query
    callback globals 0x56bc2c/0x56bc30, and clears selected-device feature
    flags when present. `g_zVideo_HwApiDeviceTable`,
    `g_zVideo_pSelectedHwApiDeviceRecord`, and the adjacent dispatch slots are
    modeled as typed globals in production source, but the referenced
    DirectDraw/Direct3D backend functions still carry source-owner/data debt.
- Next action:
  - Work the lowest 0x4a77a0 frontier owner blockers, starting with
    0x4a7d20 zVideo_dd::OpenVideoMode, before promoting query-function,
    ReportError, or primary-blit data markers.

### Group: zVideo adjust-surfaces dispatch cleanup

- Anchor: 0x4a6900 zVideo::PresentOrAdjustSurfacesIfEnabled
- Reason: source-file owner/data readiness for renderer-present dispatch used
  by RecoilApp display initialization and per-frame callers.
- Source blockers:
  - Removed the production `PresentOrAdjustSurfacesIfEnabled` adapter that
    only unlocked the primary surface and ignored the four retail
    present/adjust arguments. BN for `RecoilApp::InitializeDisplay` at
    0x42e330 shows the two display-initialization call sites target 0x4a6900
    directly, so `RecoilApp.cpp` now calls `zVideo::AdjustSurfacesIfEnabled`
    there. The active 0x4a6900 body already has VC5SP3 zero-mismatch byte
    evidence in `zvideo_adjust_surfaces`; remaining marker review is the
    source-file owner/data gate for `g_zVideo_AdjustSurfacesDisableGate`,
    `g_zVideo_pfnAdjustSurfaces`, and `g_zVideo_FrameTick`.

### Group: zSound tick/fade/playhandle owner cleanup

- Anchor: 0x49f620 zSnd::Tick
- Reason: source-file/subsystem owner and shared authored global data
- Source blockers:
  - 0x4a3c20 zSndFadeActiveList::TickAll, 0x4a3ad0
    zSndFadeEntry::TickAndMaybeDispatch, 0x49fda0
    zSndPlayHandle::StopIfActive, and 0x49f620 zSnd::Tick now have
    owner/source-faithful markers from current BN/source evidence.
  - Data remains open for the shared zSound BSS/global owner set: backend/init
    globals, last-voice marker globals, neighboring CD zero/padding layout,
    active/dispatch fade-list records, and report-error callee data.
  - 0x4a3a80 zSndFadeDispatchList::PushBack is isolated enough for accepted
    dispatch-list data and tier S; broader tick/fade/playhandle callers remain
    tier C until the shared data owner is accepted.
- Next action:
  - Continue the broader zSound data-owner pass before tier B review:
    model or classify the zero padding/neighbor records around 0x56b3d0 through
    0x56b430, then update data markers only with same-session BN/source/build
    evidence.

### Group: HUD UI save/load class recovery

- Anchor: 0x434680 HudUiSaveGameDialog::InitLayout
- Reason: class cluster / HUD UI owner source-shape recovery
- Source blockers:
  - Current pass: `HudUiCompositePanelVector` / `HudUiCompositePanelEntry`
    owner cleanup. `0x4bbff0` depends on `0x4bc410` and `0x4bc3a0`;
    focused native smokes for entry copy, vector insert, and vector clear are
    now registered and pass functional verification for `0x4bc410`,
    `0x4bc3a0`, and `0x4bbff0`. Next source action is to replace the
    composite owner functions' legacy `// Reimplements` comments with required
    provenance docblocks before any source-owner/data marker review.
  - Briefing owner cleanup removed the identity-only `BriefingLayout` and
    `BriefingPanel` helpers from `src/Battlesport/Briefing.cpp`; constructors,
    destructors, update, and selector helpers now use direct typed
    `HudUiBriefingRuntime` member/base access. `Briefing.h` now records compact
    ownership/evidence comments for the recovered briefing runtime, action
    queue, and child panel/widget owner structs.
  - Current HUD dispatch cleanup status: production `src/` now has zero
    source-shape scaffold findings from
    `python tools/recoil.py guard source-shape --root src --summary
    --top 20`. The zHud, zVideo pass-3, Briefing HUD, and player pass-3
    cleanup removed production `FTable`/`VTable`/`Vtbl`/raw-slot scaffolds and
    replaced remaining table-dispatch behavior with class methods or narrow
    virtual overrides. `recoil_native` builds after the cleanup. CTest's
    `recoil_no_modern_cpp_constructs` and
    `recoil_vc5_manifest_source_policy` pass after replacing post-VC5 casts
    and deleting the obsolete local `hud_ui_element_reset_common_ftable` VC5
    target. Full CTest still has unrelated workspace failures in the existing
    native build-anchor `__asm`, existing MFC/provider shim comments, stale
    source-file-map/tooling state, and missing `RecoilStateBase.cpp` build
    inputs; do not use those failures to retier this HUD class cleanup.
  - 0x434680 and the related `HudUiSaveLoad*` save/load dialog methods were
    downgraded to `Reimplemented [X]` after the corrected source-shape guard
    reported 76 focused scaffold hits in `src\Battlesport\RecoilApp.cpp` and
    `src\Battlesport\hud.h`.
  - The FMV blur action dependency encountered in save/load and main-menu
    transition code is now class-shaped in production source:
    `zFMV_Action` owns virtual `Begin`, `Update`, `End`, `RunBlocking`, and a
    virtual destructor; `zFMV_ActionBlur`, `zFMV_ActionBlurH/V`,
    `zFMV_ActionWait`, image, AVI, and MCI actions now use C++ virtual dispatch
    instead of hand-authored `g_zFMV_Action*Vtable` globals, scalar-deleting
    helpers, or constructor table overwrites. Focused
    `recoil_no_source_shape_scaffolds.py` over `zFMV`, touched RecoilApp blur
    call sites, and save/load files reports 0 scaffold hits, and
    `recoil_native` builds. This clears the FMV action table scaffold from the
    active save/load path, but the broader HUD dispatch owner remains blocked.
  - BN constructor evidence for `HudUiWidget`, `HudUiZrdWidget`,
    `HudUiBackgroundContainer`, `HudUiBackground`, and
    `HudUiNumericTextInput` shows constructor-owned table writes at object
    offset zero, so this is HUD/UI class hierarchy debt, not a table/global
    source model.
  - `HudUiSaveLoadDialog` is now modeled as a `HudUiBackground`-derived class
    instead of embedding a `HudUiBackground base` member; native source builds
    with the inherited call sites. `HudUiSaveLoadListItem` remains blocked on
    the wider HUD dispatch owner: current BN decompile for 0x434920 shows
    `HudUiPanel::ConstructorDefault(self, nullptr, 0, 0)` followed by a
    compiler-generated derived dispatch-table write, but production source no
    longer recreates a local hand-authored list-item table.
  - The background owner chain is now class-shaped in production source:
    `HudUiBackgroundContainer : HudUiContainer` and
    `HudUiBackground : HudUiBackgroundContainer`. Current BN decompile for
    0x4b9540 shows `HudUiBackground::Constructor` calls
    `HudUiBackgroundContainer::Constructor(self, 1)` before constructing the
    cursor, image, video, font, and text-panel members. The save/load
    dialog-local `HudUiSaveLoadDialogVtable` factory/globals and constructor
    assignments were removed; the focused guard over `RecoilApp.cpp` and
    `hud.h` dropped from 76 to 64 scaffold hits.
  - Current constructor-chain pass replaced the production
    `HudUiContainer::ConstructorDefault`,
    `HudUiBackgroundContainer::Constructor`, and
    `HudUiBackground::Constructor` helper bodies with real C++ constructors.
    Direct storage construction sites now use typed placement construction
    instead of the removed base helper. VC5SP3 byte evidence now passes for
    0x4bc780 `HudUiContainer::HudUiContainer` and 0x4bc510
    `HudUiBackgroundContainer::HudUiBackgroundContainer` with zero unmasked
    mismatches after relocation masking. 0x4b9540 remains blocked because the
    current `HudUiBackground` source now emits the EH array-constructor
    helper sequence for image/video/font/text-panel members and a distinct
    member-cursor class for the BN member-vtable evidence, but VC5SP3 still
    differs in EH state ordering around cursor construction and VMode/default
    stack branch codegen (BN 426 bytes vs VC5 448, 255 unmasked mismatches).
  - `HudUiWidget` is now modeled as `HudUiWidget : HudUiElement` in
    production source. Current BN decompile for 0x4b3d00 shows
    `HudUiWidget::Constructor` calls `HudUiElement::Constructor(self, 0, 0)`
    before installing the widget dispatch table and initializing widget-local
    fields. This compile pass keeps existing legacy table assignments as
    explicit `HudUiCommon_FTable` casts because the wider HUD dispatch owner is
    still scaffold debt, not accepted source-faithful evidence. The production
    `recoil_native` target builds after the inheritance change. Focused guards
    still fail as expected: `RecoilApp.cpp`/`hud.h` remain at 64 scaffold hits,
    and `zhud_ui.h`/`zhud_ui.cpp` report 905 scaffold hits in the broader HUD
    dispatch model.
  - Core `HudUiElement` method entries are now downgraded back to
    `Reimplemented [X]` with `Source dependencies satisfied` failed and
    `Source owner` set to `Kind: class; Parent: HudUiElement;
    State: parent-pending`. Current BN evidence for 0x4b4070, 0x404d70,
    0x4b47a0, and 0x404ca0 shows constructor/destructor-owned offset-zero
    dispatch and virtual call behavior, but production source still spells the
    owner as `HudUiCommon_FTable` globals, factories, and raw slot dispatch.
    The next source-faithful pass must recover that class/interface owner
    directly; copied or hand-authored FTable data remains invalid source-shape
    scaffolding.
    Local fake virtual dispatch-view structs in `zhud_ui.cpp` were removed
    from `HudUiContainer::InvalidateChildren`,
    `HudUiZrdWidget::DeleteChildIfPresent`, and
    `HudUiCheckToggleWidget::DestructorCore`. The file now builds without
    those misleading production `virtual` view types; the focused scaffold
    guard still fails because the remaining HUD dispatch layer is raw
    `FTable`/slot scaffold debt.
    A follow-up cleanup removed the five remaining local `*FTable` record
    declarations from `zhud_ui.cpp` helper bodies
    (`HudUiElement::SetBltSourceAndClipRect`, `HudUiCircle::DrawDirty`,
    `HudUiListSelectorItem::OnActivate`,
    `HudUiWidget::RebuildBltRectFromImage`, and
    `HudUiOwnedTextInput::OnAcceptNotifyOwner`). The native source still
    builds, and the focused `zhud_ui.cpp` scaffold guard now reports only raw
    slot access, FTable object/global references, and table factories. Those
    remaining findings still require the full HUD class/interface owner
    recovery before any affected plan entry can be retiered.
    A follow-up BN-backed direct-call cleanup removed the remaining concrete
    `HudUiVirtual*` helper call sites from typed HUD members/globals and label
    panel vectors, including `HudUiNumericTextInput::SetInputActive`,
    `HudUiFillBitmap::UpdateNormalizedFromCursor`,
    `HudUiCheckToggleWidget::LoadFromZrd`/`SetChecked`,
    `HudUiZrdWidget`/`HudUiZrdWidgetEx17C_Item` bounds calculations,
    `HudUiMgr` sensor/objective visibility paths,
    `HudUiZrdScrollingText::UpdateScrollPositions`,
    background text-panel binding, cycle-selector entry creation/update, and
    command binding list item load. Dead `HudUiVirtual*` helper scaffolds were
    removed after their call sites disappeared. The production `recoil_native`
    target builds after these conversions. The only remaining `HudUiVirtual*`
    use is dynamic child invalidation in `HudUiContainer::InvalidateChildren`;
    it is coupled to the unresolved root HUD virtual owner. The focused
    scaffold guard over `src\GameZRecoil\zHud` now reports 528 hits, all from
    the central `FTable`/`Vtbl` types/globals/factories and raw slot dispatch,
    not concrete typed member-call sites.
  - `HudUiZrdWidget` is now modeled as `HudUiZrdWidget : HudUiWidget` in
    production source. Current BN decompile for 0x4b4ee0 shows
    `HudUiZrdWidget::Constructor` calls `HudUiWidget::Constructor(self, 0)`
    before installing the ZRD widget dispatch table and initializing
    image/state fields. Direct subclasses and bind sites in `RecoilApp.cpp`,
    `hud.cpp`, `HudUiMainMenuDialog.cpp`, `HudUiMpExitDialog.cpp`, and
    `HudUiNetGameSetup.cpp` were adjusted to use the inherited widget object
    directly instead of `base`-member paths. The production `recoil_native`
    target builds after the inheritance change. Focused guards remain
    unchanged and still fail as expected: `RecoilApp.cpp`/`hud.h` remain at 64
    scaffold hits, and `zhud_ui.h`/`zhud_ui.cpp` remain at 905 hits in the
    broader HUD dispatch model.
  - `HudUiNumericTextInput` is now modeled as
    `HudUiNumericTextInput : HudUiZrdWidget` in production source. Current BN
    decompile for 0x4b49e0 shows
    `HudUiNumericTextInput::BaseConstructor` calls
    `HudUiZrdWidget::Constructor(self)` before constructing the owned
    `HudUiTextInput` and `HudUiSliderBorder` members and installing the
    numeric-input dispatch table. Current BN decompile for 0x41a200 shows
    `HudUiClampedIntTextInput::Constructor` passes `self` directly to
    `HudUiNumericTextInput::BaseConstructor`, so the clamped input remains a
    derived class over the numeric input owner. New-game, cheat-code,
    save/load, and net-game setup numeric-input bind sites were adjusted to
    pass the inherited widget object directly. The production `recoil_native`
    target builds after the inheritance change. Focused guards remain
    unchanged and still fail as expected: `RecoilApp.cpp`/`hud.h` remain at 64
    scaffold hits, and `zhud_ui.h`/`zhud_ui.cpp` remain at 905 hits in the
    broader HUD dispatch model.
  - `HudUiPolyline` and `HudUiSliderBorder` are now modeled as
    `HudUiPolyline : HudUiElement` and
    `HudUiSliderBorder : HudUiPolyline` in production source. Current BN
    decompile for 0x4bf840 shows `HudUiPolyline::Constructor` calls
    `HudUiElement::Constructor(self, 0, 0)` before installing the polyline
    dispatch table, and current BN decompile for 0x4b4620 shows
    `HudUiSliderBorder::Constructor` calls
    `HudUiPolyline::Constructor(self)` before installing the slider-border
    dispatch table and setting polyline points through `self`. The production
    `recoil_native` target builds after the inheritance change. Focused
    `zhud_ui.h`/`zhud_ui.cpp` guard output remains unchanged at 905 scaffold
    hits in the broader HUD dispatch model.
  - `HudUiOwnedTextInput` is now modeled as
    `HudUiOwnedTextInput : HudUiTextInput`, and `HudUiNumericTextInput` now
    owns a typed `HudUiOwnedTextInput textInput` member instead of a flattened
    `HudUiTextInput` plus separate owner pointer. Current BN decompile for
    0x4b49e0 shows `HudUiNumericTextInput::BaseConstructor` calls
    `HudUiTextInput::Constructor(&self->textInput, 0x100)`, writes the
    following owner slot at `textInput + 0x110`, then installs the
    numeric-input text dispatch table. Current BN decompile for 0x4ba3e0 shows
    `HudUiOwnedTextInput::OnAcceptNotifyOwner` using that owner pointer to
    call the owning widget's accept slot. The production `recoil_native` target
    builds after this typed owned-member recovery. The `HudUiTextInput`
    dispatch table itself remains unresolved source-shape debt: duplicate
    default slot targets mean the current evidence is not enough to claim an
    authored compiler-generated C++ vtable model.
  - `HudUiCounter` is now modeled as `HudUiCounter : HudUiWidget` in
    production source. Current BN decompile for 0x40dac0 shows
    `HudUiCounter::Constructor` calls `HudUiWidget::Constructor(self, 0)`
    before installing the counter dispatch table and clearing `stateImages`.
    Mode-counter initialization/destruction and state-change paths now use the
    inherited widget object and typed `stateImages[]` fields directly instead
    of embedded `base` paths or object-relative image offsets. The production
    `recoil_native` target builds after the inheritance change. The
    `HudUiCounter` dispatch table and wider `zhud_ui` table model remain
    unresolved source-shape debt, so no source-owner or tier marker should be
    promoted from this cleanup alone.
  - `HudUiObjectiveBar` is now modeled as a `HudUiBar`-derived objective bar
    instead of a flattened copy of the HUD element/bar header with its own
    explicit table pointer. Current BN type/layout evidence keeps the object at
    size 0x140 with `slideRangeX` and `chatComposeActive` at the existing
    `HudUiBar` variant payload offsets 0x138 and 0x13c. The global
    `g_HudUiMgrObjectiveBar` now uses the recovered objective-bar type, and
    objective update code accesses `slideRangeX` as a typed field instead of
    casting the global through an overlay. The production `recoil_native`
    target builds after the change; the broader HUD dispatch table model
    remains unresolved source-shape debt.
  - `HudUiOwnedTextInput::owner` is now typed as the containing
    `HudUiNumericTextInput *`, matching current BN decompile for 0x4b49e0 where
    `HudUiNumericTextInput::BaseConstructor` constructs the owned text input
    and stores `self` in the owner slot at offset 0x25c. The accept-notify path
    now calls the named owner method instead of manually indexing the owner's
    dispatch table. The deeper numeric-input commit dispatch remains unresolved
    HUD virtual owner debt.
  - `HudUiSlot::Draw` now uses the recovered embedded-widget source model:
    current BN decompile for 0x40db90 shows visible checks over the slot widget
    and track-marker widget members, and the production source now calls each
    member's `Draw()` method instead of manually indexing draw slots. The
    constructor still installs `g_HudUiSlot_FTable`, so the class dispatch table
    itself remains unresolved HUD owner debt.
  - `HudUiTripletPanel::Draw` now calls the inherited `DrawBase()` and the
    three owned `HudUiWidget` item `Draw()` methods directly. Current BN
    decompile for 0x40f400 shows that exact reverse-order member draw sequence;
    the current native smoke verifies the base blit followed by visible item
    blits for item 2 then item 0, supporting source-owner acceptance for this
    method.
  - `HudUiTripletPanel::SetVisibleCount` now calls the owned `HudUiWidget`
    item `SetVisible()` methods and inherited `Invalidate()` directly. Current
    BN decompile for 0x40f460 shows the item-array visibility loop followed by
    `self` invalidation; the production source no longer spells this method as
    raw slot-24/slot-8 dispatch. The current native smoke verifies clamping,
    item visibility, unchanged-count early return, invalidation, and
    `HudUiMgr::SetNanitePanelCount` forwarding.
  - `HudLayoutHW::UpdateObjectiveDirtyRect` now calls
    `g_HudUiMgrObjectiveWidget.GetCenterX()`/`GetCenterY()` directly. Current
    BN decompile for 0x4132b0 shows those objective-widget center queries
    feeding the dirty rectangle before typed invalidation/draw calls; the source
    no longer spells them as raw slot-25/slot-26 table calls.
  - `HudUiMgrObjective::UpdateMeterXPoints` now calls
    `g_HudUiMgrObjectiveWidget.GetCenterX()` directly. Current BN decompile for
    0x4118b0 shows the same objective-widget center query feeding the objective
    meter point X coordinates; the source no longer spells it as a raw
    slot-25 table call.
  - `HudUiMgrData` now recovers the BN `g_HudUiMgr` owner slice through
    offset 0xbcc: manager root global, nanite panel at offset 0x420,
    objective block at offset 0x690, objective widget at objective offset
    0x1c, objective meter at objective offset 0x19c, and
    objective-widget-right X at objective offset 0x18. This clears the data
    gate for 0x4132b0 `HudLayoutHW::UpdateObjectiveDirtyRect` and 0x4118b0
    `HudUiMgrObjective::UpdateMeterXPoints`; the later sensor/weapon/message
    manager span and tier-S VC targets remain open.
  - `HudUiLayoutNode::ApplyImageWidget` now calls the typed
    `HudUiWidget::SetPos()` and inherited `Invalidate()` methods after applying
    the image layout. Current BN decompile for 0x413d30 shows the dispatch tail
    as widget `SetPos` followed by `Invalidate`; the unrelated string-compare
    flag-lift warnings do not affect that call-shape evidence.
  - `HudUiNanitePanel::InitLayout` now calls the layout/anchor
    `HudUiWidget::GetCenterX()`/`GetCenterY()` methods, inherited `SetPos()`,
    and inherited `SetBltSourceAndClipRect()` directly. Current BN decompile for
    0x40f2e0 shows those widget center queries and self position/clip calls; the
    source no longer spells this method as raw slot-25/slot-26/slot-3/slot-6
    dispatch.
  - `HudUiWidget::RebuildBltRectFromImage` now calls inherited
    `SetClipRect()` directly after rebuilding the image bounds. Current BN
    decompile for 0x404e10 shows `self->ftable->SetClipRect(self, &rect)`;
    source now uses the recovered class method instead of raw slot-7 dispatch.
  - `HudUiWidget::SetImageByPathOwned` and `HudUiWidget::SetPos` now call
    inherited `Invalidate()` directly after changing image/position state.
    Current BN decompiles for 0x4b3e30 and 0x4b3dd0 show the same self
    invalidation tail; the source no longer spells these methods as raw slot-8
    dispatch.
  - `HudUiWidget::Draw` and `HudUiWidget::SetImageBorrowedAndInvalidate` now
    call inherited `DrawBase()`/`Invalidate()` directly. Current BN decompiles
    for 0x4b3fb0 and 0x4b3e70 show those self dispatch tails; the source no
    longer spells them as raw slot-2/slot-8 calls.
  - `HudUiMessage::RebuildWeaponLayout` now calls typed
    `HudUiWidget::GetCenterX()`/`GetCenterY()` for the layout anchor, inherited
    `SetPos()`/`SetBltSourceAndClipRect()` for the message widget, and
    `widget.SetPos()` for the side image widget. Current BN decompile for
    0x414070 shows those same calls; the source no longer spells this method as
    raw slot-25/slot-26/slot-3/slot-6 dispatch.
  - `HudUiMessage::LoadWeaponLayoutFromNode` now calls inherited
    `Invalidate()` after setting `imageStateWord`. Current BN decompile for
    0x413ec0 shows the post-layout self invalidation tail before panel font/text
    setup; the source no longer spells it as raw slot-8 dispatch.
  - `HudUiShieldMessageWidget::ApplyLayout` now calls typed
    `HudUiWidget::GetCenterX()`/`GetCenterY()`, panel `GetX()`/`GetY()`,
    `SetClip()`, `SetTextFmt()`, `UpdateTextBoundsFromContent()`, and meter
    `SetBltSourceAndClipRect()` directly. Current BN decompile for 0x40eb00
    shows the same widget/panel/meter dispatch sequence; the source no longer
    spells this method as raw slot-25/slot-26/slot-6/slot-29/slot-30 dispatch.
  - `HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh`, `Draw`, and
    `RebuildBltRect` now call `RebuildBltRect()`, `DrawBase()`, inherited
    `GetX()`/`GetY()`, and inherited `SetClipRect()` directly. Current BN
    decompiles for 0x4bfd40, 0x4bfe90, and 0x4bff00 show those same self
    dispatches; the source no longer spells them as raw video-widget table
    slots.
  - `HudUiZrdWidget::Constructor` and `HudUiFillBitmap::SetNormalizedValue` now
    call inherited `Invalidate()` directly. Current BN decompiles for 0x4b4ee0
    and 0x4ba3c0 show self invalidation after state initialization/value
    updates.
  - `HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf` now calls the owner
    selector `OnActivate()` and each option `HidePreviewIfNotSelected()`
    directly after `SetSelectedIndex()`. Current BN decompile for 0x4b87f0
    shows the owner activation dispatch and per-option hide-preview dispatch;
    the source no longer spells this method as raw slot-12/slot-16 calls.
  - `HudUiSetPanelVectorVisible` and `HudUiZrdWidget::HidePreview` now call
    typed `HudUiPanel::SetVisible()` directly for label-panel vectors. Current
    BN decompile for 0x4b5860 shows the same rollover/activate/label panel
    visibility loops; the source no longer spells the hide-preview path as raw
    slot-24 panel dispatch.
  - `HudScoreboard::DispatchSetScale` now calls
    `g_HudUiMgrStatsList->Update(deltaTime)` through the typed
    `HudUiStatsListElement` global pointer. Current BN decompile for 0x40eae0
    shows the wrapper dispatching the stats-list element update; the source no
    longer spells this as a raw slot-9 table call.
  - `HudUiBar::Draw`, `HudUiBar::SetPointXY`, `HudUiPolyline::Draw`,
    `HudUiPolyline::SetPoint`, and `HudUiTextLabel::OnDraw` now call inherited
    element methods directly for `DrawBase`, `SetPos`, and `Invalidate`.
    Current BN decompiles for 0x4bcff0, 0x4bcf80, 0x4bf900, 0x4bf8b0, and
    0x4bce30 show those inherited element dispatch targets; the remaining bar,
    polyline, and text-label table factories/globals still block source-owner
    acceptance.
  - `HudUiNumericTextInput::Update` now updates the first label through the
    typed `HudUiPanel *` stored in `labelPanels` instead of reading a panel
    table and indexing slot 35. Current BN decompile for 0x4b4e60 shows the
    label-panel `SetText` dispatch followed by invalidating the numeric input;
    the numeric input raw keyboard/commit callbacks remain unresolved virtual
    owner debt.
  - `HudUiCounter::UpdateLayoutPosition` now calls the inherited `SetPos`
    method directly before rebuilding its clip viewport. Current BN decompile
    for 0x40f130 shows the counter owner dispatching element `SetPos` and then
    updating local layout/image bounds; the counter table factory/global remain
    unresolved dispatch-owner scaffold debt.
  - `HudUiCircle` is now modeled as `HudUiCircle : HudUiElement` in
    production source. Current BN decompile for 0x4bc480 shows
    `HudUiCircle::Constructor` calls `HudUiElement::Constructor(self, x, y)`
    before installing the circle dispatch table and initializing
    radius/color fields. `HudUiBriefingLocatorPanel` call sites now use the
    inherited element fields and methods directly, and the production
    `recoil_native` target builds after the inheritance change. Current source
    has no production `g_HudUiCircle_FTable` scaffold; the retail table pointer
    remains dispatch data and tier-S evidence debt rather than a source-owner
    blocker for the constructor.
  - `HudUiBriefingLocatorPanel` is now modeled as
    `HudUiBriefingLocatorPanel : HudUiCircle` in production source. Current
    BN decompile for 0x403c10 shows the constructor calls
    `HudUiCircle::Constructor(self, 0x64, 0x6e, 0x1e, color)` before
    installing the locator-panel dispatch table and hiding the object. Current
    BN decompile for 0x403c90 and 0x403cb0 uses inherited
    `HudUiElement`/`HudUiCircle` fields for dirty blit, clip rect, radius
    shrink, update, and invalidation. The production `recoil_native` target
    builds after replacing the embedded `HudUiCircle base` spelling with
    inheritance. A follow-up cleanup removed the local
    `g_HudUiBriefingLocatorPanel_Vtbl` factory/global and constructor table
    overwrite from production Briefing source. The locator panel class owner is
    source-modeled by the inherited `HudUiCircle` shape plus its constructor,
    dirty-rect blit, update, and no-standalone `DrawBase` override; the retail
    locator dispatch table remains data/tier-S evidence debt, not a production
    source scaffold.
  - `HudUiBriefingObjectivePicture` is now modeled as
    `HudUiBriefingObjectivePicture : HudUiWidget` in production source.
    Current BN decompile for 0x403930 shows
    `HudUiWidget::Constructor(&self->objectivePicture, 0)` at object offset
    zero before installing `g_HudUiBriefingObjectivePicture_Vtbl`, clearing
    `noiseAlpha`, and invalidating the object. Current BN decompile for
    0x4038a0 shows `DrawWithNoiseOverlay` calls `HudUiWidget::Draw(self)` and
    then uses inherited center/image fields plus `noiseAlpha`. The production
    `recoil_native` target builds after replacing the embedded
    `HudUiWidget base` spelling with inheritance. A follow-up cleanup removed
    the local `g_HudUiBriefingObjectivePicture_Vtbl` factory/global and
    constructor table overwrite from production Briefing source; the wider
    shared HUD dispatch owner remains unresolved source-shape debt, so no
    source-owner or tier marker should be promoted from this cleanup alone.
  - `HudUiBriefingRuntime` is now modeled in the public header as a
    `HudUiBackground` subclass with typed `Briefing_ActionQueue`,
    `HudUiFillBitmap`, `HudUiPanel`, `HudUiBriefingObjectivePicture`,
    `HudUiCompositePanel`, and `HudUiBriefingLocatorPanel` members instead of
    a raw vptr/padding shell or private `HudUiBriefingRuntimeLayout` cast.
    Current BN evidence for 0x403930 shows
    `HudUiBackground::Constructor(this)` before constructing the action queue,
    transport progress, panels, objective picture, messages panel, and locator
    panels; current BN evidence for 0x403ed0 destroys those members and then
    calls `HudUiBackground::Destructor(this)`. The production `recoil_native`
    target builds after replacing the embedded-background layout calls with
    inherited/base calls, and the focused briefing guard no longer reports
    header findings. A follow-up cleanup removed `HudUiBriefingRuntimeVtable`,
    `g_HudUiBriefingRuntime_Vtbl`, local briefing child table
    globals/factories, constructor table overwrites, raw slot helpers, and the
    hand-authored scalar-deleting destructor from production Briefing source.
    This is still not an accepted reimplementation because the broader
    `HudUiBackground`/`HudUiElement` dispatch owner, composite-entry destructor
    owner, touched data audit, and tier S byte/provider evidence remain
    unresolved. Plan entries 0x403930, 0x403ed0, 0x404070, and 0x404400 are
    therefore kept at `Source dependencies satisfied ❌`, `Source owner ❌`,
    `Reimplemented [X]`, and `Model: pending`.
    `transportProgress` is now typed as
    `HudUiBriefingTransportProgress : HudUiFillBitmap`, matching the BN
    constructor pattern at 0x403930: `HudUiFillBitmap::Constructor` followed by
    compiler dispatch-table evidence for the briefing-specific transport
    progress override. The local briefing transport progress table
    global/factory was removed from production Briefing source; this improves
    the member source model only, and the wider HUD virtual owner remains
    unresolved.
  - `HudUiFillBitmap` is now modeled as `HudUiFillBitmap : HudUiZrdWidget`
    in production source. Current BN decompile for 0x4b8450 shows
    `HudUiFillBitmap::Constructor` calls `HudUiZrdWidget::Constructor(self)`
    at object offset zero before installing `g_HudUiFillBitmap_Vtbl` and
    clearing the fill/preview fields. Fill-bitmap methods now use inherited
    widget/ZRD fields directly, and briefing/options-panel call sites were
    adjusted to pass the inherited widget object without an embedded
    `base` hop. The production `recoil_native` target builds after the
    inheritance change. The `g_HudUiFillBitmap_FTable` and related
    options/briefing fill-bitmap dispatch factories remain unresolved
    dispatch-owner scaffold debt, so no new tier or data marker should be
    promoted from this cleanup alone.
  - `HudUiZrdWidgetEx17C_Item` and `HudUiZrdWidgetEx17C` are now modeled as
    `HudUiZrdWidget` subclasses in production source. Current BN decompile
    for 0x4b8760 shows `HudUiZrdWidgetEx17C_Item::Constructor` calls
    `HudUiZrdWidget::Constructor(self)` at object offset zero before
    installing the item dispatch table and clearing item state. Current BN
    decompile for 0x4b8b10 shows `HudUiZrdWidgetEx17C::Constructor` calls
    `HudUiZrdWidget::Constructor(self)` at object offset zero before
    installing the selector dispatch table and clearing the option array.
    Item/selector methods and new-game/controls call sites now use inherited
    widget/ZRD fields directly instead of embedded `base` hops. The
    production `recoil_native` target builds after the inheritance change.
    The `g_HudUiZrdWidgetEx17C*_FTable` factories/globals and derived
    new-game/controls selector table initializers remain unresolved
    dispatch-owner scaffold debt, so no source-owner or tier marker should be
    promoted from this cleanup alone.
  - The text-panel owner chain is now partially class-shaped in production
    source: `HudUiTextLabel : HudUiElement`, `HudUiPanel : HudUiTextLabel`,
    and `HudUiSaveLoadListItem : HudUiPanel`. Current BN decompile for
    0x4bcb50 shows `HudUiTextLabel::ConstructorWithPosAndFlags` calls
    `HudUiElement::Constructor(self, 0, 0)` before installing the text-label
    table. Current BN decompile for 0x4ba740 shows
    `HudUiPanel::ConstructorDefault` calls
    `HudUiTextLabel::ConstructorWithPosAndFlags(self, text, x, y, nullptr)`
    before installing the panel table and initializing named panel fields.
    Current BN decompile for 0x434920 shows
    `HudUiSaveLoadListItem::Constructor` calls
    `HudUiPanel::ConstructorDefault(self, nullptr, 0, 0)` before installing
    the save/load list-item table and initializing `layoutX/layoutY`.
    Production source now uses inherited panel/element methods and fields for
    save/load list-item text, visibility, invalidation, draw, activation, and
    panel field initialization. The custom `HudUiSaveLoadListItemVtable` type,
    local `g_HudUiSaveLoadListItem_Vtbl` global/factory, constructor table
    overwrite, and source-level list-item slot calls were removed, and the
    production `recoil_native` target builds. The wider `HudUiPanel_FTable`
    scaffolds are still unresolved dispatch-owner debt, so 0x434920 stays
    `Reimplemented [X]`/`Model: pending` until the full panel virtual owner is
    recovered.
  - The `HudUiPanel` layout pass now replaces the panel-tail raw offsets in
    copy construction, text color/shadow/font setters, draw, text-rect
    rebuild, text-bounds update, cached-text access, and query helpers with
    named fields backed by static offset asserts. `HudUiPanelSimple` is now
    modeled as `HudUiPanelSimple : HudUiPanel`; current BN evidence for its
    constructor follows the same pattern as other HUD UI classes: construct
    the panel owner first, then install the simple-panel dispatch table and
    initialize inherited panel fields. The production `recoil_native` target
    builds after the panel-layout cleanup. Focused save/load guard output is
    unchanged at 62 hits because the remaining findings are dispatch
    table/factory scaffolds, not panel field-layout offsets.
  - The follow-up panel pass removed the remaining raw `FieldAt` access for
    recovered `HudUiPanel` tail fields in draw/text-rect helper paths and kept
    the production `recoil_native` target building. The remaining focused
    the focused save/load guard no longer reports `RecoilApp.cpp` or the
    save/load table factories after the local list-item, button, primary
    action, and game-name input table globals/factories were removed. The
    remaining focused findings are in `hud.h` new-game/controls table
    declarations and must be removed through those owner models, not by
    adding narrower table wrappers.
    A narrower panel cleanup replaced the last raw `FieldAt` access in
    `HudUiPanel::Draw`, `HitTest`, `SetTextFmtV`, `SetText`,
    `RebuildTextRect`, `MeasureTextPrefixRect`, and `QueryTextHeight` with
    recovered class fields such as `textBuffer`, `cachedText`, `centerText`,
    `textDirty`, `textWidthPx`, `textHeightPx`, `unknown274`, and inherited
    `flags`/`x`/`y`. The production `recoil_native` target builds, the
    original-source symbol guard passes for `zhud_ui.cpp`, and focused
    functional targets for those seven panel methods pass. This does not
    promote source-owner metadata: `HudUiPanel_FTable` and the wider HUD
    dispatch table/factory layer remain unresolved owner debt.
    A follow-up panel owner cleanup replaced raw self-dispatch through
    `HudUiPanel_FTable` slot 36 with direct member calls to `RebuildTextRect`
    in `HudUiPanel::Draw`, `HitTest`, `GetLastTextPtr`, `GetTextRect`,
    `UpdateTextBoundsFromContent`, and `QueryTextHeight`. Current BN decompile
    for these methods names the call as `self->vtbl->RebuildTextRect`, which
    is compiler-style class dispatch evidence rather than a data-table source
    model. This removes local raw slot scaffolding only; the `HudUiPanel`
    virtual owner is still incomplete until the production FTable declarations,
    globals, constructor assignments, and external slot helpers are replaced by
    the source-faithful class/interface model.
    The related `HudUiPanelVirtualRebuildTextRect*` and `HudUiPanelTextWidth`
    helper scaffolds were then removed from `zhud_ui.cpp`. Their call sites now
    use typed `HudUiPanel` fields (`textDirty`, `textWidthPx`, `alignMode`) and
    direct `RebuildTextRect()` member calls. This removes a local offset/table
    wrapper layer but still does not clear the panel owner gate while
    `HudUiPanel_FTable` declarations/globals and constructor table writes
    remain in production source.
    `HudUiPanel::Draw` now also calls the inherited `DrawBase()` member
    directly instead of using the local `HudUiPanelVirtualDrawBase` raw slot
    helper, and that helper was removed. Current BN decompile names the call as
    `self->vtbl->DrawBase`; no current panel-family evidence requires a
    data-table helper source model for this call.
    A follow-up class-shape cleanup replaced remaining local base casts in
    `HudUiPanel::GetTextRect`, `Invalidate`, `UpdateTextBoundsFromContent`,
    `HudUiPanelSimple::Constructor`, `HudUiTimerPanel::SetTimeSeconds`, and
    `HudUiTimerPanel::Update` with inherited fields and base-qualified member
    calls. The production `recoil_native` target builds, `zhud_ui.cpp` passes
    the original-source symbol guard, and the focused targets
    `hud_ui_panel_get_text_rect`, `hud_ui_panel_invalidate`,
    `hud_ui_panel_update_text_bounds_from_content`,
    `hud_ui_panel_simple_constructor`,
    `hud_ui_timer_panel_set_time_seconds`, and `hud_ui_timer_panel_update`
    pass. This still leaves the same HUD dispatch owner blocker.
    `HudUiPanel::SetClip` now writes the inherited `bltSource` and `clipRect`
    fields directly instead of creating a local `HudUiElement *` view over
    `this`. The production `recoil_native` target builds, `zhud_ui.cpp` passes
    the original-source symbol guard, and `hud_ui_panel_set_clip` passes. This
    remains source-shape cleanup only; the dispatch through slot 8 and the
    wider `HudUiPanel_FTable` owner are still unresolved table debt.
    `HudUiZrdScrollingText::Update` now calls the inherited
    `HudUiElement::Update` directly instead of creating a local base pointer
    over `this`; `hud_ui_zrd_scrolling_text_update` passes after the change.
    The method remains `Reimplemented [X]` because the credits/scroller FTable
    owner and per-panel raw update-slot dispatch are still unresolved source
    model debt.
    A follow-up inherited-call cleanup removed local `HudUiElement *` views
    from `HudUiCompositePanel::ResizeEntryVectorAndRelayout`,
    `HudUiWidget::Constructor`, `HudUiZrdWidget::LoadFromZrd`,
    `HudUiZrdWidget::Invalidate`, `HudUiCycleSelectorWidget::Update`,
    `HudUiFillBitmap::LoadFromZrd`, `SetNormalizedValue`, and
    `SetNormalizedValueAndRebuild`, and `HudUiTripletPanel::Constructor`.
    The production `recoil_native` target builds and focused functional
    targets for these methods pass. This is source-shape cleanup only:
    `HudUiSlot`, `HudUiCounter`, `HudUiMessage`, and `HudUiNumericTextInput`
    still needed follow-up local base-cast cleanup in this pass, and the
    broader HUD FTable/raw-slot dispatch owner remains unresolved.
    `HudUiSlot` is now modeled as a `HudUiElement` subclass instead of a
    copied element-prefix record. Current BN decompile for 0x40db20 shows
    `HudUiElement::Constructor(self, 0, 0)` at object offset zero, followed by
    `HudUiWidget::Constructor` for the embedded widgets at offsets 0x48 and
    0x104 and final installation of `g_HudUiSlot_FTable`; 0x40d780 destroys
    the embedded widgets in reverse owner order before resetting the base HUD
    element table. Production source builds, focused slot functional targets
    pass for constructor/destructor/draw/scalar-deleting destructor, and the
    focused zHud scaffold guard drops to 864 findings. This does not promote
    the slot entries because `g_HudUiSlot_FTable` and embedded-widget raw slot
    dispatch remain part of the unresolved HUD dispatch owner.
    `HudUiCounter::ApplyFromLayoutNode` now passes `this` directly to
    `g_HudUiMgr.AddChild`, matching current BN decompile for 0x40f070 where
    the `HudUiCounter` object is the `HudUiWidget`/`HudUiElement` object at
    offset zero. The counter constructor, release, layout-load, and layout
    position methods now have address-backed docblocks. The production
    `recoil_native` target builds and `hud_ui_counter_release_state_images`
    passes; the constructor/layout-position/layout-node counter smokes still
    fail in the current stale smoke binary and cannot be refreshed until the
    broader dirty native-smoke test tree is made buildable. No counter marker
    is promoted from this cleanup, and `HudUiCounter_FTable` plus raw slot
    dispatch remain HUD dispatch-owner debt.
    `HudUiMessage::LoadWeaponLayoutFromNode` now registers `this` and
    `&widget` directly with `g_HudUiMgr.AddChild`; current BN decompile for
    0x40da00 shows `HudUiMessage` constructs `HudUiWidget` at object offset
    zero before constructing the embedded `panel` and `widget`, and current
    BN decompile for 0x413ec0 passes `self` and `&self->widget` to the HUD
    manager. `HudUiNumericTextInput::BaseConstructor` and `Update` now call
    inherited `SetVisible`/`Invalidate` directly; current BN decompile for
    0x4b49e0 shows `HudUiZrdWidget::Constructor(self)` at offset zero, and
    0x4b4e60 invalidates `self` through the base dispatch pointer. The touched
    methods now have address-backed docblocks. This cleanup does not promote
    any marker because `g_HudUiMessage_FTable`,
    `HudUiNumericTextInput_Base_FTable`, and raw label-panel slot dispatch
    remain unresolved HUD dispatch-owner scaffolding.
    `HudUiSaveLoadDialog::InitializeFileEntries` (0x434ee0) and
    `HudUiSaveLoadDialog::SetSelectedEntryIndex` (0x4353f0) now carry tier C
    functional evidence from their existing smoke targets because the method
    bodies are class-shaped and contain no local table factory. Their
    `Source owner` and `Data reimplemented` gates remain blocked by the wider
    `HudUiSaveLoadDialog`/HUD dispatch owner. `HudUiSaveLoadListItem::Constructor`
    (0x434920) stays `Reimplemented [X]`: the local list-item table scaffold
    has been removed, but the method cannot be retiered until the
    `HudUiPanel`/`HudUiElement` virtual class owner replaces the remaining HUD
    table/factory scaffolds.
    A follow-up source cleanup removed remaining embedded-base casts in the
    save/load list item and dialog construction/destruction paths: the list-item
    constructor now calls `HudUiPanel::ConstructorDefault` directly, the
    `LIST_0` through `LIST_8` bind loop passes `HudUiSaveLoadListItem*` through
    normal inheritance conversion, and the reverse destruction loops call
    `HudUiPanel::Destructor` as a base-qualified inherited method. The
    production `recoil_native` target builds. Older focused save/load smokes
    that expected the removed table scaffold are no longer accepted as fresh
    functional evidence until the smoke source is rebuilt around the
    source-faithful HUD virtual owner.
  - `HudUiCheckToggleWidget` and `HudUiCycleSelectorWidget` are now modeled as
    `HudUiZrdWidget` subclasses in production source. Current BN decompile
    for 0x4b6fc0 shows `HudUiCheckToggleWidget::Constructor` calls
    `HudUiZrdWidget::Constructor(self)` at object offset zero before
    installing `g_HudUiCheckToggleWidget_FTable` and clearing toggle fields.
    Current BN decompile for 0x4b7d60 shows
    `HudUiCycleSelectorWidget::Constructor` calls
    `HudUiZrdWidget::Constructor(self)` at object offset zero before
    installing `g_HudUiCycleSelectorWidget_FTable` and initializing selector
    fields. Command, options, net-game setup, and list-selector call sites now
    use the inherited widget/ZRD object directly where these classes are the
    member owner. The production `recoil_native` target builds after the
    inheritance change. The `g_HudUiCheckToggleWidget_FTable`,
    `g_HudUiCycleSelectorWidget_FTable`, and derived command/options table
    factories remain unresolved dispatch-owner scaffold debt, so no source
    owner, data, or tier marker should be promoted from this cleanup alone.
  - `HudCmdBindButtonBase` is now modeled as
    `HudUiCheckToggleWidget`-derived source. Current BN decompile for
    0x4b8d30 shows `HudCmdBindButtonBase::Constructor` calls
    `HudUiCheckToggleWidget::Constructor(self)` at object offset zero before
    constructing `bindPanel`, installing `g_HudCmdBindButtonBase_FTable`, and
    initializing binding-list state. Concrete command button wrappers now use
    the inherited bind-button fields directly where they dispatch activation,
    owner callbacks, checked state, and cleanup. The production
    `recoil_native` target builds after the inheritance change. The
    `g_HudCmdBindButtonBase_FTable` family and command-dialog table factories
    remain unresolved dispatch-owner scaffold debt, so no source owner, data,
    or tier marker should be promoted from this cleanup alone.
  - `HudUiListSelectorItem` and `HudUiCompositePanel` are now modeled as
    `HudUiPanel` subclasses instead of raw panel-storage shells. Current BN
    decompile for 0x4b92a0 shows `HudUiListSelectorItem::Constructor` calls
    `HudUiPanel::ConstructorDefault(self, nullptr, 0, 0)` at object offset
    zero before installing `g_HudUiListSelectorItem_FTable`. Current BN
    decompile for 0x4bb790 shows
    `HudUiCompositePanel::ConstructorWithEntryCount` calls
    `HudUiPanel::ConstructorDefault(self, nullptr, 0, 0)` at object offset
    zero before initializing the entry vector and installing
    `g_HudUiCompositePanel_FTable`. Their methods now use inherited
    `HudUiPanel` fields and methods directly for construction, destruction,
    visibility, text bounds, and layout. The production `recoil_native` target
    builds after the inheritance change. The `g_HudUiListSelectorItem_FTable`,
    `g_HudUiCompositePanel_FTable`, and related slot-table factories remain
    unresolved dispatch-owner scaffold debt, so no source owner, data, or tier
    marker should be promoted from this cleanup alone.
  - `HudUiTransitionTextPanel` is now modeled as a `HudUiPanel` subclass
    instead of a raw panel-storage shell. Current BN decompile for 0x4ba020
    shows `HudUiTransitionTextPanel::Constructor` calls
    `HudUiPanel::ConstructorDefault(self, nullptr, 0, 0)` at object offset
    zero before initializing the flash fields and installing
    `g_HudUiTransitionTextPanel_FTable`. Transition-text construction and
    flash ticking now use inherited `HudUiPanel` fields and methods directly,
    and composite-panel copy helpers copy the typed flash fields instead of
    using raw panel-tail offsets. The production `recoil_native` target builds
    after the inheritance change. Provenance docblocks for the constructor,
    destructor helper, flash tick, and color-flash setter now record the BN
    address/source-path evidence. The `g_HudUiTransitionTextPanel_FTable` and
    related slot-table factories remain unresolved dispatch-owner scaffold
    debt, so no source owner, data, or tier marker should be promoted from this
    cleanup alone.
  - `HudUiTimerPanelFloat`, `HudUiTimerPanel`, and
    `HudUiCounterTextPanel` are now modeled as `HudUiPanel` subclasses instead
    of raw panel-storage shells. Current BN decompile and assembly for
    0x40ef60, 0x40ed80, and 0x40dbf0 show each constructor calls
    `HudUiPanel::ConstructorDefault(self, ...)` at object offset zero before
    installing its derived table. Timer fields now use named typed members at
    offsets 0x2a4, 0x2a8, and 0x2ac, and the float timer draw path now calls
    inherited panel methods directly instead of raw slot dispatch. The
    production `recoil_native` target builds after the inheritance change. The
    `g_HudUiTimerPanelFloat_FTable`, `g_HudUiTimerPanel_FTable`,
    `g_HudUiCounterTextPanel_FTable`, and wider HUD slot-table factories remain
    unresolved dispatch-owner scaffold debt, so the affected plan entries stay
    `Source owner ❌`, `Data reimplemented ❌`, and `Model: pending`.
  - The embedded `HudUiMessage::panel` member is now modeled as
    `HudUiPanelFull : HudUiPanel` with named `activeSideIndex`, `layoutX`, and
    `layoutY` fields instead of an anonymous panel-storage shell. Current BN
    decompile and assembly for 0x40da00 show `HudUiMessage::Constructor` calls
    `HudUiPanel::ConstructorDefault(&self->panel, nullptr, 0, 0)` at member
    offset 0xe0 and then writes the panel-tail side index at offset 0x384.
    Message-side image pointers at 0xd0/0xd4 are now named
    `activeSideImages`, and weapon-message selection/layout paths use the
    typed panel and image fields instead of raw offsets or panel slot dispatch.
    The production `recoil_native` target builds after the embedded-member
    recovery. The `g_HudUiMessage_FTable`, message panel dispatch, and wider
    HUD slot-table factories remain unresolved dispatch-owner scaffold debt, so
    0x40da00 stays `Source owner ❌`, `Data reimplemented ❌`, and
    `Model: pending`.
  - `HudUiBackgroundCursorWidget` is now modeled as a `HudUiWidget` subclass
    instead of an embedded `HudUiWidget base` shell. Current BN decompile and
    assembly for 0x4bf980 show
    `HudUiBackgroundCursorWidget::MemberConstructorLocal` calls
    `HudUiWidget::Constructor(self, 0)` at object offset zero before installing
    the cursor-widget dispatch table and initializing cursor capture fields at
    offsets 0xbc through 0xcc. Cursor draw, image-refresh, position, and
    captured-image rebuild paths now use inherited `HudUiWidget` and
    `HudUiElement` fields/methods directly. The production `recoil_native`
    target builds after the inheritance change. The
    `g_HudUiBackgroundCursorWidget_FTable`,
    `g_HudUiBackgroundCursorWidget_MemberFTable`, and wider HUD slot-table
    factories remain unresolved dispatch-owner scaffold debt, so 0x4bf980 stays
    `Source owner ❌`, `Data reimplemented ❌`, and `Model: pending`.
  - `HudUiBackgroundVideoWidget` is now modeled as a `HudUiElement` subclass
    instead of an embedded `HudUiElement base` shell. Current BN decompile and
    assembly for 0x4bfc80 show
    `HudUiBackgroundVideoWidget::Constructor` calls
    `HudUiElement::Constructor(self, 0, 0)` at object offset zero before
    installing `g_HudUiBackgroundVideoWidget_FTable` and initializing
    `stream`, `elapsedTimeSec`, `colorKey565`, and `mediaPath` at offsets
    0x34 through 0x3e. Video load, update, draw, clip rebuild, and background
    child-bind paths now use inherited `HudUiElement` fields/methods directly.
    The production `recoil_native` target builds after the inheritance change.
    The `g_HudUiBackgroundVideoWidget_FTable` factory/global and wider HUD
    slot-table factories remain unresolved dispatch-owner scaffold debt, so
    0x4bfc80 stays `Source owner ❌`, `Data reimplemented ❌`, and
    `Model: pending`.
  - `HudUiTripletPanel` is now modeled as a `HudUiElement` subclass instead
    of an embedded `HudUiElement base` shell. Current BN decompile and
    assembly for 0x40f200 show `HudUiTripletPanel::Constructor` calls
    `HudUiElement::Constructor(self, 0, 0)` at object offset zero, constructs
    three `HudUiWidget` items at offset 0x3c, installs
    `g_HudUiTripletPanel_FTable`, clears `visibleCount` at 0x34, hides each
    item, and registers `self` with `g_HudUiMgr`. Current BN decompile for
    0x40d610 shows the matching item-array destructor followed by resetting
    `self->ftable` to `g_HudUiCommon_FTable`. Constructor, draw, visible-count,
    nanite layout, and destructor paths now use inherited `HudUiElement`
    fields/methods directly. The production `recoil_native` target builds
    after the inheritance change. The `g_HudUiTripletPanel_FTable`
    factory/global and wider HUD slot-table factories remain unresolved
    dispatch-owner scaffold debt, so the affected triplet-panel method entries
    stay `Source owner ❌`, `Data reimplemented ❌`, and `Model: pending`.
  - `HudUiTriplet` is now modeled as a `HudUiContainer` subclass instead of
    an embedded `HudUiContainer base` shell. Current BN decompile and assembly
    for 0x40dcd0 show `HudUiTriplet::Constructor` calls
    `HudUiContainer::ConstructorDefault(self)` at object offset zero before
    initializing the scoreboard entry vector, allocating header/row
    `HudUiPanelSimple` children, and enabling the container. Current BN
    decompile for 0x40e070 shows owned panel cleanup, vector storage delete,
    and `HudUiContainer::DestructorCore(self)` call. Triplet constructor,
    destructor, and stats-list update paths now use inherited
    `HudUiContainer` fields/methods and virtual `UpdateAll` directly. The
    production `recoil_native` target builds after the inheritance change, and
    the container-specific `g_HudUiTriplet_FTable` factory/global is removed.
    Wider HUD element/panel slot-table factories remain unresolved
    dispatch-owner scaffold debt, so the affected triplet method entries stay
    `Source owner ❌`, `Data reimplemented ❌`, and `Model: pending`.
  - `HudUiStatsListElement` is now modeled as a `HudUiElement` subclass
    instead of an embedded `HudUiElement base` shell. Current BN decompile and
    assembly for 0x40f4c0 show the inlined construction path allocates 0x38
    bytes, calls `HudUiElement::Constructor(self, 0, 0)` at object offset zero,
    installs `g_HudUiStatsListElement_FTable`, allocates the owned
    `HudUiTriplet`, and stores it at offset 0x34. Current BN decompile and
    assembly for 0x40fa40 show the matching destructor resets the stats-list
    table, deletes the owned triplet, clears offset 0x34, and restores
    `g_HudUiCommon_FTable`. Init, update dispatch, destructor, and add-child
    paths now use inherited `HudUiElement` fields/methods directly. The
    production `recoil_native` target builds after the inheritance change. The
    `g_HudUiStatsListElement_FTable` factory/global and wider HUD slot-table
    factories remain unresolved dispatch-owner scaffold debt, so the affected
    stats-list method entries stay `Source owner ❌`, `Data reimplemented ❌`,
    and `Model: pending`.
  - `HudUiStringMenu` is now modeled as a `HudUiContainer` subclass instead of
    an embedded `HudUiContainer base` shell. Current BN decompile and assembly
    for 0x40f4c0 show the inlined construction path allocates 0x3cdc bytes,
    calls `HudUiContainer::ConstructorDefault(self)` at object offset zero,
    constructs 23 `HudUiPanelSimple` items at offset 0x20, adds each item as a
    child, and enables the container. Current BN decompile and assembly for
    0x40fdd0 show the
    matching item-array destructor followed by
    `HudUiContainer::DestructorCore(self)`. Init, per-frame update,
    aux-overlay enable, and destructor paths now use inherited
    `HudUiContainer` fields/methods directly. The production `recoil_native`
    target builds after the inheritance change, and the container-specific
    `g_HudUiStringMenu_FTable` factory/global is removed. Wider HUD
    element/panel slot-table factories remain unresolved dispatch-owner
    scaffold debt, so 0x40fdd0 stays `Source owner ❌`, `Data reimplemented ❌`,
    and `Model: pending`.
  - `HudUiTextStack4` is now modeled as a `HudUiContainer` subclass, with
    `HudUiTopMessageStack` and `HudUiChatMessageStack` as concrete derived
    classes. Current BN decompile and assembly for 0x4bd020 and 0x4bd2d0 show
    both constructors call `HudUiContainer::ConstructorDefault(self)` at object
    offset zero, construct four `HudUiPanel` rows at offset 0x10, add each row
    as a child, and seed the per-stack text layout. Current BN decompile for
    0x40fe90 and 0x40fef0
    shows the matching row-array destructor followed by
    `HudUiContainer::DestructorCore(self)`. Per-frame update, enable/disable,
    pushed-line, network/player row attachment, and top/chat constructors now
    use inherited `HudUiContainer` fields/methods directly. The production
    `recoil_native` target builds after the inheritance change, and the
    container-specific `g_HudUiTopMessageStack_FTable` and
    `g_HudUiChatMessageStack_FTable` globals/factories are removed. Wider HUD
    element/panel slot-table factories remain unresolved dispatch-owner
    scaffold debt, so the affected text-stack entries stay `Source owner ❌`,
    `Data reimplemented ❌`, and `Model: pending`.
  - `HudUiBar` is now modeled as a `HudUiElement` subclass instead of an
    embedded or duplicated element-prefix record. Current BN decompile and
    assembly for 0x4bcf20 show `HudUiBar::Constructor` calls
    `HudUiElement::Constructor(self, 0, 0)` at object offset zero, clears
    offset 0x130, installs `g_HudUiBar_FTable`, zeroes the point array at
    offset 0x34, and invalidates the inherited element. Current BN evidence
    for 0x4bcf80 and 0x4bcff0 uses the same inherited dispatch slots and
    `HudUiBar` point/draw state. The wider `HudUiBar_FTable`/HUD dispatch
    owner and touched data remain unresolved scaffold debt, so 0x4bcf20,
    0x4bcf80, and 0x4bcff0 stay `Source owner ❌`, `Data reimplemented ❌`,
    and `Model: pending`. `HudUiMeter` still needs a separate source-shape
    pass because BN shows it calls `HudUiBar::Constructor` but reuses the
    final `HudUiBar` tail offsets as meter-specific state rather than adding
    tail storage.
  - `HudUiMessage` is now modeled as a `HudUiWidget` subclass instead of
    containing an embedded `HudUiWidget base` member. Current BN decompile and
    assembly for 0x40da00 show `HudUiMessage::Constructor` calls
    `HudUiWidget::Constructor(self, 0)` at object offset zero, constructs the
    `HudUiPanelFull` member at offset 0xe0 and the secondary `HudUiWidget`
    member at offset 0x390, clears offsets 0xbc..0xdc, clears
    `panel.activeSideIndex` at 0x384, and installs `g_HudUiMessage_FTable` at
    offset zero. Current BN decompile and assembly for 0x40d590 show the
    matching destruction order: secondary widget, panel, then the inherited
    widget base. Message draw/layout/update helpers now use inherited
    `HudUiWidget` fields/methods directly. The `g_HudUiMessage_FTable`
    factory/global and wider HUD slot-table factories remain unresolved
    dispatch-owner scaffold debt, so the affected message entries stay
    `Source owner ❌`, `Data reimplemented ❌`, and `Model: pending`.
  - `HudUiMessageBoxOkButton` and `HudUiMessageBoxCancelButton` are now
    modeled as `HudUiZrdWidget` subclasses instead of embedded
    `HudUiZrdWidget base` records. Current BN evidence for 0x4bf060 shows the
    message-box dialog constructor calls `HudUiZrdWidget::Constructor` at the
    OK and cancel button member addresses, then installs
    `g_HudUiMessageBoxOkButton_Vtbl` and
    `g_HudUiMessageBoxCancelButton_Vtbl` at object offset zero. Constructor,
    destructor, bind, fallback-layout, and activation paths now use the
    inherited ZRD/widget fields directly. Current BN evidence for
    `HudUiMessageBoxOkButton::OnActivate` (0x4bf800) and
    `HudUiMessageBoxCancelButton::OnActivate` (0x4bf820) shows owner-dialog
    primary/secondary dispatch to `HudUiMessageBoxDialog::OnOk` (0x4bf7c0)
    and `HudUiMessageBoxDialog::OnCancel` (0x4bf7e0). Source now calls those
    dialog methods directly and no longer exposes `HudUiMessageBoxDialog_FTable`,
    `g_HudUiMessageBoxDialog_FTable`, or its factory. The message-box button
    dispatch globals and wider HUD/ZRD slot-table layer remain unresolved
    source-shape debt, so the affected message-box entries stay
    `Source owner ❌`, `Data reimplemented ❌`, and `Model: pending`. After
    this cleanup, the focused zHud scaffold guard reports 673 remaining
    findings in the shared HUD dispatch layer. The related native smoke tests
    no longer reference the removed dialog table; the aggregate smoke build is
    still blocked earlier by unrelated stale test scaffolds such as the removed
    RecoilStateBase header and old FMV action vtable test data.
  - `HudLayoutHW` is now modeled as a `HudLayoutBase` subclass instead of an
    embedded `HudLayoutBase base` record. Current BN decompile and assembly
    for 0x412ea0 show the constructor calls
    `HudUiContainer::ConstructorDefault(self)`, constructs inherited
    `widget0` at offset 0x30, constructs derived `widget1`, `widget2`, and
    `widget3` at offsets 0xec, 0x1b4, and 0x27c, and uses the layout virtual
    methods for update, activation, enable/disable, and active-layout
    switching. Constructor/destructor paths and focused layout tests now use
    the inherited layout fields directly. The `HudLayout*` table globals/raw
    slot dispatch are removed from production source; wider HUD element/panel
    slot-table scaffolds still block the affected `HudLayoutHW` entries, which
    stay `Source owner ❌`, `Data reimplemented ❌`, and `Model: pending`.
  - `HudLayoutBase` is now modeled as a `HudUiContainer` subclass instead of a
    duplicated container-prefix record. Current BN evidence for
    `HudLayoutSW::Constructor` (0x412b60), `HudLayoutHW::Constructor`
    (0x412ea0), and `HudLayoutBase::Destructor` (0x40d3b0) shows
    `HudUiContainer` construction/destruction at `this` and `widget0` at
    offset 0x30. `HudLayoutSW` remains a `HudLayoutBase` subclass, and source
    no longer exposes a separate `HudLayoutBase::ftable` field. The layout
    owner now uses named virtual methods (`SetActive`, `LayoutPreUpdate`,
    `Enable`, `Disable`, `OnActivated`, and `UpdateObjectiveDirtyRect`) instead
    of production `HudLayout*` table globals/raw slots. `HudLayoutBase`,
    `HudLayoutSW`, and `HudLayoutHW` entries still stay `Source owner ❌`,
    `Data reimplemented ❌`, and `Model: pending` until the wider
    `HudUiElement`/panel dispatch owner and touched data are recovered.
  - `HudUiMeter` is now modeled as a `HudUiBar` subclass instead of a
    duplicated bar-prefix record. Current BN evidence for
    `HudUiMeter::Constructor` (0x40fb70) and `HudUiMeter::ConstructorEx`
    (0x40d9e0) shows `HudUiBar::Constructor(self)` at object offset zero,
    followed by a meter table write and zeroing offsets 0x138 and 0x13c.
    Source now captures the same-size tail reuse by giving the final
    `HudUiBar` words both bar and meter field names, so `HudUiMeter` does not
    add storage beyond the base class. The `g_HudUiMeter*_FTable` globals and
    raw slot dispatch remain unresolved source-shape debt, so the meter
    constructor entries stay `Source owner ❌`, `Data reimplemented ❌`,
    `Reimplemented [X]`, and `Model: pending`.
  - The panel text/font cleanup removed the local
    `HudUiPanelVirtualSetTextFmt*` and `HudUiPanelVirtualSetFontRequired`
    helper scaffolds from `zhud_ui.cpp`. Typed panel-family call sites now use
    direct `SetTextFmt`/`SetFont` member calls, using `"%s"` where the removed
    string helper previously passed display text as data rather than as a
    format string. The production `recoil_native` target builds after the
    cleanup, and focused targets pass for `hud_ui_panel_set_text_fmt`,
    `hud_ui_panel_set_font`, `hud_ui_aux_overlay_apply_text_line_op`,
    `hud_ui_cycle_selector_widget_apply_font_style_for_entry`,
    `hud_cmd_bind_button_base_set_selected_entry`,
    `hud_cmd_dialog_on_command_selection_changed`,
    `hud_cmd_dialog_update_capture_state`, and
    `hud_ui_text_stack_push_line`. The focused zHud scaffold guard still
    reports 833 findings, all in the remaining HUD dispatch table/factory/raw
    slot layer, so no source-owner, data, or tier marker is promoted from this
    cleanup alone.
  - `HudUiTextStack4::SetTextColors`, `Clear`, `SetFontAll`, `SetXAll`, and
    `SetYDescending` now operate on the owned typed `HudUiPanel` row objects
    directly instead of raw panel-tail offsets, `HudUiVirtualSetTextFmtEmpty`,
    or `HudUiPanel_FTable` slot calls. Current BN evidence for 0x4bd3d0,
    0x4bd2a0, 0x4bd110, 0x4bd410, and 0x4bd440 shows iteration over the four
    `lines` panels and dispatch to panel methods, which is class-owner
    evidence rather than a hand-authored table source model. The production
    `recoil_native` target builds after the cleanup. Focused functional
    targets pass for text colors, x positions, y positions, and pushed lines;
    `hud_ui_text_stack_set_font_all` needs refreshed smoke evidence after the
    unrelated stale native-smoke test files that still reference removed
    RecoilApp/HUD table/base scaffolds are cleaned up. The focused zHud
    scaffold guard now reports 827 findings, all in the remaining HUD dispatch
    table/factory/raw-slot layer.
  - `HudUiZrdWidget::Invalidate` and `HudUiZrdWidget::RefreshState` now call
    typed `HudUiPanel::Invalidate`/`SetVisible` for label-panel vectors and
    `HudUiZrdWidget::Invalidate` for the owning widget instead of reading
    `HudUiPanel_FTable` slots. Current BN evidence for 0x4b5310 and 0x4b5740
    shows panel-vector dispatch to those methods, which remains class-owner
    evidence rather than hand-authored FTable source. The production
    `recoil_native` target builds, focused functional targets
    `hud_ui_zrd_widget_invalidate` and `hud_ui_zrd_widget_refresh_state` pass,
    and the focused zHud scaffold guard now reports 811 findings.
- Next action:
  - Recover the HUD/UI core class owner starting from `HudUiElement`,
    `HudUiTextLabel`, `HudUiPanel`, `HudUiWidget`, `HudUiZrdWidget`,
    `HudUiContainer`, `HudUiBackgroundContainer`, `HudUiBackground`, and
    `HudUiNumericTextInput`; remove the `HudUiSaveLoad*` `FTable`/`Vtbl`
    globals/factories only when the class owner can express the dispatch
    contract source-faithfully.
  - Use `python tools/recoil.py status 0x434680 --lane binary`,
    `python tools/recoil.py frontier 0x434680 --depth 1 --lane binary`, and
    `python tools/recoil.py guard source-shape --root
    src\Battlesport\RecoilApp.cpp --root src\Battlesport\hud.h --summary
    --top 20` as the focused owner checks.

### Group: RecoilApp owner EH tier S

- Anchor: 0x42de60 RecoilApp::Destructor and 0x42dfa0 RecoilApp::Constructor
- Reason: class cluster / compiler-generated constructor/destructor cleanup-state model
- Source blockers:
  - 0x42de60 RecoilApp::Destructor is tier C but not tier S; BN shows an
    MSVC EH registration frame and cleanup state transitions for embedded
    RecoilApp state destruction, while the current authored source uses a
    manual non-EH `Destructor()` body.
  - 0x42dfa0 RecoilApp::Constructor is tier C but not tier S; BN shows the
    paired MSVC EH registration frame and constructor unwind map. Its
    intermediate owner dependency at 0x442c70 is now modeled as
    `RecoilApp_MfcOleModule`; current local VC5 byte evidence accepts both
    0x442c70 and 0x4428b0, so the remaining blocker is the root owner
    constructor/destructor cleanup-state model.
- Next action:
  - Recover the smallest source-faithful RecoilApp owner model that lets VC
    emit the paired member/base constructor and destructor cleanup chains
    before retrying byte verification with
    `python tools/recoil.py verify vc5 0x42dfa0` and
    `python tools/recoil.py verify vc5 0x42de60`.
    The BN EH unwind map has member/base cleanup states 0-4 plus parent-frame
    `IState*` local cleanup states 5-7; rejected probes show `try`/`catch`
    emits the wrong EBP/catch EH shape, a synthetic automatic local emits the
    correct compact EH family but introduces wrong state numbering and an extra
    destructor call, and a VC5 automatic owner destructor with nested or
    inherited IState base destructors adds per-FMV temporary pointer slots plus
    an extra saved register that retail does not have. A raw-member owner probe
    removed those temporaries and improved 0x42de60 to 97 mismatches, but its
    EH states remained `4`, `1`, and `0` where retail uses `5`, `6`, and `7`.
    Layering local IState cleanup guards over that raw-member owner produced
    the desired `5`, `6`, and `7` state writes, but forced extra stack space,
    saved `ebx`, guard-pointer stores, and duplicate vtable resets, regressing
    to 150 mismatches.
    Current direct binary-lane dependencies are split: `zFMV_Script::Cleanup`
    at 0x462630 and `zFMV_Script::Reset` at 0x462660 pass under the shared
    `zfmv_script_cleanup_reset` VC target. The paired MFC/OLE owner
    dependencies now pass current VC5SP3 byte verification: 0x442c70
    `RecoilApp_MfcOleModule::RecoilApp_MfcOleModule` passes
    `recoil_app_mfc_ole_module_constructor_s` with zero unmasked mismatches
    after 8 relocation-masked bytes, and 0x4428b0
    `RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule` passes
    `recoil_app_mfc_ole_module_destructor` with zero unmasked mismatches after
    16 relocation-masked bytes. Both ignored local owner manifests compile with
    `RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER` so VC5SP3 sees the original
    `std::deque<RecoilApp_StateQueueItem*>` member for this constructor and
    destructor pair; native host builds and the standalone
    `recoil_app_state_queue` VC target keep the recovered manual queue owner.
    Current implementation restores address-backed
    `RecoilApp_MfcOleModule::RecoilApp_MfcOleModule` at 0x442c70,
    `RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule` at 0x4428b0, with
    0x4429b0 now classified as compiler-generated scalar-deleting destructor
    glue for the real `CWinApp`-derived owner subobject. This fixes the stale
    `recoil_app_mfc_ole_module_constructor_s` provenance-manifest blocker and
    compiles under VC5. Current byte compares close the MFC/OLE pair; the
    residual drift is now above them in the root RecoilApp constructor and
    destructor EH cleanup-state model.
    The top-level owner checks still fail after the split:
    `python tools/recoil.py verify vc5 0x42dfa0` reports 131 unmasked
    mismatches with a 176-byte VC body versus 194-byte BN body, and
    `python tools/recoil.py verify vc5 0x42de60 --build-root
    build\vc5-verify-42de60-owner-pass` reports 109 unmasked mismatches with a
    192-byte VC body versus 168-byte BN body.
    `python tools/recoil.py build msvc-x86 -- cmake --build --preset
    ninja-x86-debug --target recoil_native` passes for the production static
    library after this owner split. The full native smoke build still fails in
    tests with old test-only assumptions about RecoilApp copy assignment,
    `.base` state fields, and 32-bit queue pointer aliases.
    `python tools/recoil.py guard source-shape --root
    src\Battlesport\RecoilApp.cpp --root src\Battlesport\RecoilApp.h
    --summary --top 20` now reports zero hits in `RecoilApp.cpp/.h`.
    The raw `RecoilApp_IState_Vtbl`, `g_RecoilStateBase_Vtbl`, and
    production `RecoilStateBase` shell were removed; current BN assembly for
    0x407170 shows compiler-generated scalar-deleting destructor glue, so the
    plan maps that address to a provider boundary.
    The save/load dialog cleanup path now uses concrete C++ destructors and
    `delete` instead of authored `ScalarDeletingDestructor` wrappers; plan
    entries 0x434980 and 0x434dd0 are provider-boundary compiler glue. The
    static `RecoilStateSaveLoadTransition` cleanup path similarly keeps the
    authored destructor/atexit cleanup and maps 0x435ca0 to provider glue
    instead of a production wrapper.
    The RecoilStateCredits app-state owner has been promoted from a local
    raw `RecoilApp_IState_Vtbl`/`HudUiCreditsPanelVirtual` scaffold to the
    authored `RecoilStateCredits : RecoilApp_IState` class in
    `src\Battlesport\RecoilStateCredits.h/.cpp`. Current BN evidence for
    0x409990/0x4099f0/0x409a60/0x409ad0 shows a constructor/destructor-owned
    app-state object with a vptr at offset 0 and a `HudUiCreditsPanel*` at
    offset 4. The focused source-shape guard reports zero hits for the
    credits header and implementation, production `recoil_native` builds, and
    0x409a60 now has `Source owner` kind `class`, parent
    `RecoilStateCredits`, `Model: source-faithful`. Existing credits VC
    manifests are now stale binary evidence: they still target the old
    non-virtual/synthetic symbols such as
    `?Constructor@RecoilStateCredits@@QAEPAU1@XZ` and non-virtual `QAE`
    method decorations; current VC5 output emits the actual C++ constructor
    `??0RecoilStateCredits@@QAE@XZ`, virtual method `UAE` decorations, and a
    generated scalar-deleting destructor. The ignored local VC manifests were
    updated to those class symbols; current compares still fail and remain
    blockers rather than acceptance evidence: 0x409990 has 15 unmasked
    mismatches with a 32-byte VC body versus 16-byte BN body, 0x4099f0 has 32
    unmasked mismatches with a 112-byte VC body versus 100-byte BN body, and
    0x409a60 has 93 unmasked mismatches with a 48-byte VC body versus
    106-byte BN body. Treat previous credits tier S notes as invalid until the
    class-model byte comparisons are resolved.
    Production source compiles with the intermediate `RecoilApp_MfcOleModule`
    owner, but the native smoke suite still needs broad test-only cleanup from
    raw `vftable`/`.base`/32-bit queue-slot assumptions to the typed
    `RecoilApp_IState` and `RecoilApp_StateQueue` source model.
    `zFMV_Script::Init` at 0x4625e0 remains tier C/data-blocked through
    0x4626b0 `zFMV_Script::LoadActionsFromZrd`; action construction is now
    folded into the loader body, but BN assembly at 0x4159e0 and 0x462e30
    proves the FMV action dispatch is VC-style virtual dispatch
    (`ecx=this`, slots 0x4/0x8/0xc). The null words after slot 0x10 in
    `.rdata` are alignment padding, not authored reserved fields. The
    scalar-deleting destructor entries 0x415a80, 0x462e70, 0x4631d0,
    0x463650, and 0x463bf0 are now classified in the plan as VC++ provider
    glue, but production source still authors manual
    `ScalarDeletingDestructor`/`Make...Vtable` stand-ins. The helper/data
    guard now treats this as not reimplemented until the zFMV action virtual
    class family, generated vtables, and destructor model are recovered.
    Current VC5 comparisons accept 0x4a5780 `RecoilApp::InitStdLogFiles` as
    tier S. Existing plan entries should be checked with
    `python tools/recoil.py status 0xNNNNNN --lane binary` before relying on
    any older note for 0x42e220 `RecoilApp::StartEngine`, 0x442bc0
    `RecoilApp::ShutdownSubsystems`, 0x42e430 `RecoilApp::ShutdownEngine`, or
    0x42e330 `RecoilApp::InitializeDisplay`.
    The FMV state constructor cleanup model is now accepted: 0x42eb70
    `RecoilApp_AttractFmvState::Constructor`, 0x42ed30
    `RecoilApp_MissionFmvState::Constructor`, and 0x42eb00
    `RecoilApp_FmvState::OnIdleOrDispatch` pass under
    `recoil_app_fmv_state_constructors` with VC5SP3
    `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs` zero-mismatch evidence.
    `RecoilApp_PlayState::Constructor` at 0x42eea0 is now tier S under
    `recoil_app_register_at_exit` after restoring the constructor shape that
    leaves offsets 0x04-0x0c untouched and clears only offsets 0x10 and 0x14.
    The RecoilApp vtable-order pass also makes 0x4429d0
    `RecoilApp::InitMainWindow` tier S by putting `CreateMainWnd` at the retail
    virtual slot, and keeps 0x42e220 `RecoilApp::StartEngine` plus 0x42e430
    `RecoilApp::ShutdownEngine` tier S with virtual method decorations.
    The recovered state-queue map helper 0x443690
    `RecoilApp_StateQueue::GrowAndCenterChunkBaseList` remains tier S; 0x443700
    `RecoilApp_StateQueueBlock::InitFromCursor` and the queue entrypoints
    remain tier C unless their touched globals have since been audited to tier
    B; current clean C++ spelling is source-plausible but still differs in
    stack/register scheduling.
    Remaining current VC5 blockers in the owner cluster are 0x42dfa0
    `RecoilApp::Constructor` and 0x42de60 `RecoilApp::Destructor`, both still
    requiring the source-faithful owner/EH model rather than queue or vtable
    cleanup.

### Group: Main menu transition state class cleanup

- Anchor: 0x415170 RecoilStateMainMenuTransition::RecoilStateMainMenuTransition
- Reason: class cluster / app-state source-shape cleanup
- Source blockers:
  - Main-menu transition state is now class-shaped in production source, but
    data gates remain `❌` for the owner cluster and tier S is not accepted.
  - `0x415170` still needs VC5 byte recovery for target
    `recoil_state_main_menu_transition`: current local evidence used symbol
    `??0RecoilStateMainMenuTransition@@QAE@XZ` and still had 34 unmasked
    mismatches.
  - `0x408f50` is modeled as `RecoilStateDialogHost : RecoilApp_IState` with
    a typed `HudUiDialogController *` member and direct virtual
    `SetEnabled(int)` dispatch. Target `recoil_state_dialog_host` had zero
    unmasked byte mismatches, but tier S remains blocked until the data gate is
    audited.
- Next action:
  - Continue grouped data-gate audit and VC5 tier S recovery for
    `app.main_menu_transition`; recheck current scope with
    `python tools/recoil.py status 0x415170` before editing.

### Group: HUD app-state class cleanup

- Anchor: 0x406ed0 RecoilStateCheatCode::RecoilStateCheatCode,
  0x408d60 RecoilStateControls::RecoilStateControls, 0x415850
  RecoilStateConfirmQuit::RecoilStateConfirmQuit, 0x41c560
  HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent, and 0x40d150
  HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent
- Reason: class cluster / app-state source-shape cleanup
- Source blockers:
  - The selected app-state owners no longer rely on local
    `RecoilApp_IState_Vtbl` globals/factories, local `...Virtual` dispatch
    views, or manual scalar-deleting-destructor source. The new-game and
    options overlay owners are also promoted to typed `RecoilApp_IState`
    classes. The wider `src/Battlesport/hud.cpp` and `hud.h` files still
    contain HUD widget and callback/table scaffolds, so the file-level guard
    remains failing.
  - Data gates remain `❌`; tier S is not accepted for these state classes.
- Next action:
  - Continue class-first cleanup with the remaining HUD overlay/widget owners.
    Current source models `RecoilStateCheatCode`, `RecoilStateControls`, and
    `RecoilStateConfirmQuit`, `HudUiNewGamePanelOverlayOwner`, and
    `HudUiOptionsPanelOverlayOwner` as authored `RecoilApp_IState`-derived C++
    classes with typed dialog/panel members and class
    constructor/destructor/static lifecycle methods. BN decompile confirms
    0x406ee0, 0x408d70, 0x415860, 0x41c610, and 0x40d0c0 are only VC++ scalar
    deleting destructor glue, so those addresses are provider-boundary
    entries. Plan owner metadata for the selected class methods is now
    `Kind: class`, `State: implemented`, and `Model: source-faithful`.
    Focused source-shape guard reruns report 233 hits in `hud.cpp` and 25
    hits in `hud.h`, all in the broader HUD widget/callback table debt rather
    than these app-state owner wrappers.
  - `HudCmdDialogState` and `HudUiNetGameSetupOverlayOwner` in
    `src/GameZRecoil/zHud/zhud_ui.*` are now also modeled as
    `RecoilApp_IState`-derived C++ classes with typed `HudCmdDialog*` and
    `HudUiNetGameSetupPanel*` members. Removed the local
    `g_HudCmdDialogState_Vtbl` and
    `g_HudUiNetGameSetupOverlayOwner_Vtbl` globals, base-vtable guard structs,
    local `...Virtual` dispatch views, and hand-authored scalar-deleting
    destructor source. BN assembly confirms 0x40bc70 and 0x41abc0 are VC++
    scalar-deleting destructor glue; both are provider-boundary entries and
    map to `external`. The production `recoil_native` build passes. Local VC5
    target `hud_cmd_dialog_state_lifecycle` now uses C++ constructor/destructor
    symbols: 0x40bc20, 0x40bc40, and 0x40bc50 are zero-mismatch; 0x40bc30,
    0x40bc60, and 0x40bc90 still fail byte comparison, so tier S remains
    blocked.
  - `HudUiCreditsPanel` is now modeled as a `HudUiBackground` subclass and
    `HudUiZrdScrollingText` is now modeled as a `HudUiZrdWidget` subclass
    instead of embedding `base` prefix members. Current BN evidence:
    0x409040 calls `HudUiBackground::Constructor(this)`, constructs
    `backButton` at +0xa950, `quitButton` at +0xaa9c, and
    `creditsScreen` at +0xabe8, writes the credits panel table at offset 0,
    and initializes fade/scroll fields. 0x4092a0 destroys the credits screen
    rows, then the credits screen, buttons, and `HudUiBackground` base in
    reverse member order. 0x4091e0 destroys `HudUiZrdScrollingText` rows and
    then calls `HudUiZrdWidget::DestructorCore(this)`. The production
    `recoil_native` target builds after this class-shape cleanup, but
    `g_HudUiCreditsPanel_FTable`, its secondary slot table, and the related
    raw slot dispatch/factory source remain unresolved source-shape debt.
    Plan entry 0x409380 has tier C behavior evidence but remains at
    `Source owner ❌`, `Data reimplemented ❌`, and `Model: pending`; 0x409040,
    0x4091c0, 0x4091e0, 0x4092a0, 0x409360, 0x409410, 0x409470, 0x409550, and
    0x409570 are therefore kept at `Source owner ❌`, `Reimplemented [X]`, and
    `Model: pending`.
  - `HudCmdDialog` is now modeled as a `HudUiBackground` subclass, and the
    command-list/key/joy/mouse/set-list/callback/prompt/description widgets
    are modeled as subclasses of their BN-proven HUD widget owners instead of
    embedding `base` prefix members. Current BN evidence: 0x40a5b0 calls
    `HudUiBackground::Constructor(this)`, constructs the dialog child widgets
    at fixed member offsets beginning at +0xa94c, writes the command-dialog
    table at offset 0, then loads `dialog.zrd`; 0x40adf0 destroys the child
    widgets in reverse order and then calls `HudUiBackground::Destructor`.
    The production `recoil_native` target builds after this class-shape
    cleanup. The `HudCmd*` FTable structs/globals/table factories and
    constructor/destructor table assignments remain unresolved source-shape
    debt, so the affected HudCmd class-family entries are downgraded to
    `Source dependencies satisfied ❌`, `Source owner ❌`,
    `Reimplemented [X]`, and `Model: pending`.
    `HudCmdDialogState` lifecycle entries that allocate or destroy the dialog
    are also downgraded because their direct constructor/destructor dependency
    is no longer source-ready; pure state registration/static-init entries were
    left unchanged.
  - `HudOptionsDialog` is now modeled as a `HudUiBackground` subclass, and the
    options-panel back/toggle/cycle/fill controls are modeled as subclasses of
    their BN-proven HUD widget owners instead of embedding `base` prefix
    members. Current BN evidence: 0x40c720 calls
    `HudUiBackground::Constructor(this)`, constructs each option child widget
    at its member offset, installs the options-panel table at offset 0, then
    loads `dialog.zrd`; 0x40cf60 destroys the option child widgets in reverse
    order and then calls `HudUiBackground::Destructor`. The production
    `recoil_native` target builds after this class-shape cleanup. The
    `HudUiOptionsPanel_*` FTable structs/globals/table factories and
    constructor/destructor table assignments remain unresolved source-shape
    debt, so the affected options dialog/control entries are downgraded to
    `Source dependencies satisfied ❌`, `Source owner ❌`,
    `Reimplemented [X]`, and `Model: pending`. Overlay-owner lifecycle entries
    that allocate or destroy `HudOptionsDialog` are also downgraded; pure
    state registration/static-init/queue entrypoints were left unchanged.
  - `HudUiClampedIntStepButton` is now modeled as a `HudUiZrdWidget` subclass
    instead of embedding a `base` prefix member. Current BN evidence from
    0x419aa0 `HudUiNetGameSetupPanel::Constructor` shows each step button is
    constructed with `HudUiZrdWidget::Constructor(&self->...StepButton)`,
    followed by `targetInput`/`stepDelta` initialization and a step-button
    dispatch-table write at offset zero. The production `recoil_native` target
    builds after updating the net-game setup call sites. A follow-up pass
    removed the local `HudUiNetGameSetupPanel` table type, panel/widget table
    globals, table factories, and constructor table overwrites from
    `src/Battlesport/HudUiNetGameSetup.cpp` and `.h`; the focused
    source-shape scaffold guard over those files now reports zero findings.
    The panel constructor/destructor, direct panel button methods, and
    0x41a350 `HudUiClampedIntStepButton::OnActivate` remain downgraded to
    `Source dependencies satisfied ❌`, `Source owner ❌`,
    `Reimplemented [X]`, and `Model: pending` until the broader
    `HudUiBackground`/`HudUiElement`/HUD widget dispatch owner and virtual
    delete model are recovered.
  - `HudUiPrimitiveBindTarget` is now modeled as a `HudUiElement` subclass
    instead of embedding a `base` prefix member. Current BN decompile for
    0x4bffb0 shows `SetSegmentEndpoints` dispatches `SetPos` through the
    offset-zero `HudUiElement` object and then writes `endX`/`endY`. The
    production `recoil_native` target builds after the class-shape cleanup.
    The method still depends on the unresolved `HudUiElement` dispatch-table
    source model, so 0x4bffb0 is downgraded to
    `Source dependencies satisfied ❌`, `Source owner ❌`,
    `Reimplemented [X]`, and `Model: pending`.
  - `HudUiNetExitPanel`, `HudUiNetExitPanel_ResumeWidget`, and
    `HudUiNetExitPanel_ExitButton` are now modeled as
    `HudUiBackground`/`HudUiZrdWidget` subclasses instead of embedding
    `base` prefix members. Current BN evidence for 0x41bd80 shows
    `HudUiBackground::Constructor(this)`, `HudUiZrdWidget::Constructor` for
    `resumeWidget` at +0xa94c and `exitWidget` at +0xaa9c, then a panel table
    write at offset zero, `dialog.zrd` NETEXIT binding, focus setup, and
    initial disable. Current BN evidence for 0x41beb0 destroys the exit
    widget, resume widget, and `HudUiBackground` base in reverse order. The
    production `recoil_native` target builds after this class-shape cleanup.
    A follow-up pass removed the local `HudUiNetExitPanel_FTable`, widget
    FTable globals/factories, constructor table overwrites, raw table
    dispatch, and hand-authored scalar-deleting destructor source from
    `src/Battlesport/HudUiNetExitPanel.cpp` and `.h`; the focused
    source-shape scaffold guard over those files now reports zero findings.
    0x41be90 is classified as compiler-generated scalar deleting destructor
    provider glue. The affected Net Exit panel/button entries remain
    downgraded to `Source dependencies satisfied ❌`, `Source owner ❌`,
    `Reimplemented [X]`, and `Model: pending` until the broader
    `HudUiBackground`/`HudUiElement`/HUD widget dispatch owner and virtual
    delete model are recovered.
  - `HudUiMpExitDialog`, `HudUiMpExitDialog_NewGameButton`, and
    `HudUiMpExitDialog_ExitButton` are now modeled as
    `HudUiBackground`/`HudUiZrdWidget` subclasses instead of embedding a
    `HudUiBackground base` prefix member. Current BN evidence for 0x419740
    shows allocation of 0xabf0 bytes, `HudUiBackground::Constructor(this)`,
    `HudUiZrdWidget::Constructor` for `m_mpNewGameButton` at +0xa94c and
    `m_mpExitButton` at +0xaa98, then a dialog table write at offset zero.
    Current BN evidence for 0x419870 destroys the exit button, new-game
    button, and `HudUiBackground` base in reverse order. The production
    `recoil_native` target builds after this class-shape cleanup. A follow-up
    cleanup removed the local `HudUiMpExitDialog_Vtbl`, widget table
    globals/factories, constructor table overwrites, raw update/delete
    dispatch, and hand-authored scalar-deleting wrapper from production source.
    The affected MP Exit dialog/button/app-state entries remain at
    `Source dependencies satisfied ❌`, `Source owner ❌`, `Reimplemented [X]`,
    and `Model: pending` until the broader HUD dispatch owner, button
    activation dispatch owner, touched data audit, and tier S byte/provider
    evidence are recovered; 0x419850 is classified in the plan as VC++
    scalar-deleting-destructor provider glue from BN assembly evidence.
  - `Player_UnderwaterFxPass3Ui` and `Player_ProjectileCameraFxPass3Ui` are
    modeled as `HudUiElement` subclasses with `overlayRectOrNull` at offset
    0x34. A follow-up cleanup removed the local player FX table
    globals/factories, constructor table writes, and raw visibility dispatch
    from production source. The focused player source-shape guard now reports
    zero findings and `recoil_native` builds. Constructor entries 0x41eb30 and
    0x41eb90 were downgraded to `Reimplemented [X]` because the previous tier C
    evidence depended on the removed local table scaffolds; recovery is blocked
    on the broader `zVideoFxPass3`/`HudUiElement` dispatch owner and the source
    binding for `ApplyBlueTint`/`ApplyGreenMask`.
  - `GameNet` HUD panel helpers now call typed `HudUiPanel::SetVisible`,
    `HudUiPanel::SetPos`, and `HudUiPanel::SetTextFmt` directly instead of
    resolving panel methods through local raw slot helpers. The unused
    `kNetSessionBrowserDialog_Vtable`/`kNetSessionConfigDialog_Vtable` named
    marker globals and externs were also removed from production source. The
    focused `GameNet.cpp`/`.h` source-shape guard reports zero findings and
    `recoil_native` builds. MFC dialog behavior remains represented by the
    actual `CDialog`-derived classes, message maps, and provider boundaries;
    no source-owner or tier marker is promoted from this cleanup alone.
  - `HudSensorTracker::ResetMissionState` now calls the typed
    `HudUiElement::SetVisible` method and destroys the recovered
    `HudWeatherFx` owner produced by `LoadMissionWeatherFx`, instead of using
    HudSensorTracker-local raw SetVisible/delete slot helpers. The focused
    `HudSensorTracker.cpp`/`.h` source-shape guard reports zero findings and
    `recoil_native` builds. The accepted C-tier behavior evidence remains
    functional only; the plan blocker stays on the broader HUD/HudUiElement
    and weather FX dispatch owner, touched data audit, and tier S
    byte/provider evidence.
  - The remaining Battlesport source-shape scaffold guard hits were removed
    from the Westwood Online and MFC frame cleanup surface. `CZGameFrame` and
    `CZRecoilFrame` no longer install fake named frame table markers;
    Westwood Online startup/config/dialog/download provider calls now use
    typed COM-style interfaces or direct MFC calls instead of local table
    structs, marker globals, or raw progress-window table dispatch. The broad
    `src/Battlesport` source-shape guard now reports zero findings and
    `recoil_native` builds. This cleanup does not promote Westwood/MFC plan
    markers; the provider interfaces and authored dialog/frame classes still
    need owner/data/tier S evidence before source-shape acceptance.
  - HUD text-stack, ZRD-widget, and composite-panel methods touched in this
    pass no longer use local fake FTable recorder/scaffold paths for their
    class-owned behavior. `HudUiTextStack4::{SetTextColors,Clear,SetFontAll,
    SetXAll,SetYDescending}` now call typed panel fields/members; ZRD widget
    label visibility/invalidation now uses typed panel methods; and
    `HudUiCompositePanel` constructor/update/layout/text/font/history/vector
    paths now operate on typed `HudUiTransitionTextPanel` entries instead of
    local slot dispatch. Production `recoil_native` builds, `git diff --check`
    and JSON manifest checks pass, and the focused zHud source-shape guard
    dropped from 811 to 791 findings. The rebuilt native smoke executable is
    still blocked by unrelated stale tests (`RecoilStateBase.h` removal and
    old RecoilApp/HUD `base`/`vtbl` assumptions), so updated composite text
    smokes cannot be used as fresh functional evidence yet. Do not promote
    plan markers from this cleanup; the active 0x434920 blocker remains the
    unresolved HUD virtual class owner and wider HUD table/factory scaffolds.
  - The save/load-local list item, delete/next/previous button, primary action
    button, and game-name input FTable factories/globals were removed from
    `src/Battlesport/RecoilApp.cpp`, along with the matching constructor table
    overwrites in 0x434680, 0x434920, and 0x434b90 source. Current BN evidence
    still records compiler-generated table writes at those points, but that is
    class-dispatch evidence, not permission to keep hand-authored production
    table objects. The production `recoil_native` target builds after the
    cleanup. Focused save/load guard output for `RecoilApp.cpp` plus `hud.h`
    dropped from 57 to 22 findings; the remaining focused findings are in
    `hud.h` new-game/controls table declarations. Battlesport-wide scaffold
    findings dropped from 595 to 560. Plan entries 0x434680, 0x434920, and
    0x434b90 remain `Reimplemented [X]`/`Model: pending` until the
    `HudUiSaveLoadDialog`/`HudUiBackground`/`HudUiElement` virtual class owner
    is recovered; older native smokes that asserted the removed vtable symbols
    are stale and cannot be used as fresh evidence.
  - The New Game and Controls dialog owner-local table scaffolds were removed
    from `src/Battlesport/hud.cpp` and `src/Battlesport/hud.h`:
    `HudUiNewGamePanel_FTableHeader`,
    `HudUiControlsDialog_FTableHeader`, their `Make...FTable` factories, the
    child `g_HudUiNewGamePanel_*`/`g_HudUiControlsDialog_*` table globals, and
    the constructor assignments to those globals are gone. Current Binary Ninja
    evidence at 0x41c290 and 0x408a30 still proves class-shaped constructors
    with embedded child objects and compiler-generated dispatch-table writes,
    so both entries are downgraded to `Reimplemented [X]`/`Model: pending`
    until the broader `HudUiBackground`/`HudUiElement` virtual class owner is
    recovered. The focused `hud.cpp`/`hud.h` scaffold guard now reports 111
    remaining findings, all in other weather, confirm-quit, cheat-code, and
    wider HUD dispatch-table debt.
  - The follow-up `hud.cpp`/`hud.h` cleanup removed the remaining local
    confirm-quit, cheat-code, weather, and main-menu-back table scaffolds in
    these files: confirm-quit and cheat-code `*FTable` record/factory/global
    objects, `HudWeatherFx`/snow/rain FTable factory/globals, the
    New Game back-widget assignment to `g_HudUiMainMenu_BackButton_FTable`,
    and the matching constructor/destructor table writes. The focused
    `python tools/recoil.py guard source-shape --root
    src\Battlesport\hud.cpp --root src\Battlesport\hud.h --summary --top 10`
    guard now reports zero findings. Affected constructor/destructor entries
    were downgraded to `Reimplemented [X]`/`Model: pending` where prior
    accepted evidence depended on removed table identities; retiering remains
    blocked on the broader `HudUiElement`/`HudUiBackground`/`HudWeatherFx`
    virtual class owner and provider-glue model.
  - The main-menu dialog cleanup removed the local main-menu button/dialog
    table factories, table globals, dialog table install, raw refresh slot
    dispatch, and remaining back-button table assignments from
    `src/Battlesport/HudUiMainMenuDialog.cpp` and related save/load call
    sites. The focused scaffold guard now reports zero findings across
    `hud.cpp`, `hud.h`, `RecoilApp.cpp`, and `HudUiMainMenuDialog.cpp` for
    this cleanup batch. The affected constructor evidence is intentionally
    downgraded/stale until the broader `HudUiBackground`/`HudUiElement`
    virtual class owner is recovered.
  - `HudUiTextInput`, `HudUiOwnedTextInput`, and
    `HudUiChatComposeTextInput` are now modeled as authored C++ classes with
    virtual key-action hooks instead of production `HudUiTextInput_FTable`
    structs/globals/factories. Current BN evidence for 0x4b42f0 and
    0x4b4460 shows constructor-owned offset-zero dispatch and key-action slot
    calls; current BN evidence for the numeric and chat-compose tables shows
    authored overrides for accept behavior. Production `recoil_native` builds,
    `git diff --check` passes for touched HUD source/tests, and focused
    `zhud_ui` scaffold guard output drops from 791 to 749 findings. No plan
    markers are promoted: the wider `HudUiElement`/HUD dispatch owner and
    remaining `zhud_ui` FTable factory/global scaffolds are still unresolved.
  - The remaining zVideo pass-3 source-shape findings are intentionally left
    blocked on the same HUD base owner instead of being rewritten as another
    local table shape. Current BN decompile for 0x4bdbe0 shows
    `zVideoFxPass3Slot::Constructor` calls `HudUiElement::Constructor`, clears
    the pass-3 clip member, then installs the slot dispatch at object offset
    zero. Current BN decompile for 0x4bef90 shows
    `zVideoFxPass3Config::Constructor` calls
    `HudUiContainer::ConstructorDefault`, constructs the root and five
    `HudUiElement`-based slots, links them as children, and dispatches
    `SetEnabled` through the config table. The source-faithful cleanup unit is
    therefore `HudUiElement`/`HudUiContainer` class recovery first, then the
    pass-3 derived/root/slot classes; zVideo-local `FTable` renames or wrapper
    views are not acceptable endpoints.
  - The message-box dialog cleanup removed the local
    `HudUiMessageBoxDialog_FTable` production table/factory/global and now
    dispatches OK/Cancel through the recovered dialog class methods. The
    aggregate `recoil_native_smoke` target was pruned to exclude stale
    table-era suites whose tests still assert deleted vtable/ftable symbols or
    raw provider slots. The remaining 311 registered smokes build and pass via
    CTest; excluded suites need source-model/provider-double rewrites before
    they can be restored as evidence.
  - Follow-up HUD typed-field cleanup replaced raw `FieldAt` runtime-state
    access in the save/load blocker path with recovered class fields:
    `HudUiElement::timer`, `HudUiBackgroundContainer::mouseState`,
    `HudUiBackground::capturedCompositeImage`, `HudUiBar::slideRangeX`,
    `HudUiTimerPanelFloat` sample fields, `HudUiPanel` text/style fields, and
    `HudUiListSelectorItem::entryIndex`. A small
    `HudUiListSelectorItemArrayHeader` now names the local allocation cookie
    used for list-selector item arrays. A subsequent owner-typing pass removed
    the generic `OwnerField` overlay helper by typing ZRD widget owners as
    `HudUiBackground *` and using BN-proven fields for UI origin, clipping,
    font styles, credits fade rate, and cursor state. Native x86 debug CTest
    passes 18/18, the focused source-shape scaffold guard reports zero
    findings, and the regenerated source-file map is current. Remaining HUD
    owner debt is the broader dispatch/source-owner and data/tier-S recovery;
    do not promote source-owner markers from this cleanup alone.
  - Current destructor pass moved `HudUiElement::~HudUiElement` inline in the
    class declaration while keeping a cpp provenance note for 0x4b47a0. This
    lets VC5 inline the base table reset into `HudUiWidget::~HudUiWidget`
    without manual vtable stores. Same-session VC5 evidence now passes for
    0x4b47a0, 0x4b3d50, and 0x4bfa20 with zero unmasked mismatches; 0x4b9760
    still fails on background-destructor EH state numbering,
    `ReleaseIfNotDefault` zero-store codegen, and MSVC array-destructor
    scheduling/argument order.
