# Inlined Helper Recovery Ledger

Ledger for likely original helpers or class methods fully inlined by the retail
compiler, with no standalone executable address. Binary Ninja and caller
assembly remain authoritative.

Record only recurring helpers that clarify source/class structure or remove
duplicate recovered behavior. One-off cleanup belongs in source comments.

Bare `Observed in caller...` is insufficient. A recovered helper must state that
no standalone retail function exists and explain why the body looks like
original inline/static/member source, not a convenience wrapper.

Use VC5-era `inline`, `static inline`, ordinary `static`, member functions, or
class-body definitions. Do not use reconstruction inline marker macros.

## Entry Pattern

```text
## HelperOrClass::Method

Evidence:
- Caller addresses:
- Repeated instruction/source pattern:
- Likely original owner/source file:
- Why no standalone retail function is expected:

Restored source form:
- inline/static/member helper:
- Callers using it:

Verification notes:
- Native tests:
- VC byte or source-cluster attempt:
- Known tier `S` limits:

Open limits:
- ...
```

## Current Entries

## zInput_BindMapContext constructors

Evidence:
- Caller addresses: `0x471860` `zInput::BindMapContext_Push` and
  `0x4710a0` `zInput::BindMapSystem_Init`.
- Repeated instruction/source pattern: bind-map context allocations are followed
  by `zInput_BindMapContext::InitFromTemplate`; `0x471860` has VC5 EH
  allocation state around construction and the constructor body is recovered as
  the template initializer call.
- Likely original owner/source file: `GameZRecoil/zInput/zInput.h` with call
  sites in `GameZRecoil/zInput/zInput.cpp`.
- Why no standalone retail function is expected: BN has no separate retail
  constructor body for these context allocations; the observed construction
  work is inlined into the allocation callers.

Restored source form:
- class-body constructors on `zInput_BindMapContext`.
- Callers using it: bind-map system initialization and overlay push allocation.

Verification notes:
- Native tests: `zinput_bindmap_context_smoke` covers the overlay push path.
- VC byte/source-cluster attempt: `0x471860` under
  `vc5_o2_ob1_md_gx_facs` still has 124 unmasked mismatches; best observed
  profile `vc5_o2_ob0_md_gx_gr_facs` has 62 mismatches but calls the
  constructor symbol instead of the retail direct `InitFromTemplate` target.
- Known tier `S` limits: accepted only as recovered inline source shape until
  the remaining overlay-stack branch and constructor inlining drift is resolved.

Open limits:
- `0x471860` remains tier `B`; the plan blocker records the current byte drift.

## zRndrSpanDepthAtXByPartsLocal

Evidence:
- Caller addresses: `0x4907c0` `zRndr_SpanOcclusion::TestSpanDepthOrderPair`,
  `0x491dd0` `zRndr_SpanOcclusion::TestColumnVisibility`, and nearby recovered
  span-occlusion insertion/build helpers in `zRndr_Draw.cpp`.
- Repeated instruction/source pattern: callers evaluate
  `invDepth + (x - sampleXMin) * depthSlope` from `zRndr_SpanNode` fields before
  depth-order, trimming, or visibility comparisons.
- Likely original owner/source file: `GameZRecoil/zRndr/zRndr_Draw.cpp`.
- Why no standalone retail function is expected: BN shows the arithmetic inlined
  into the span-occlusion caller bodies and no standalone function target is
  identified for this expression helper.

Restored source form:
- `static float zRndrSpanDepthAtXByPartsLocal(...)` in the zRndr translation
  unit.
- Callers using it: span-occlusion depth-order and visibility helpers.

Verification notes:
- Native tests: span-occlusion insert, build-list, column-visibility,
  point-visibility, and rasterize smokes cover callers using the helper.
- VC byte/source-cluster attempt: no standalone helper byte target exists; tier
  `S` remains deferred to the coherent zRndr span-occlusion source-cluster pass.
- Known tier `S` limits: accepted only as recovered inline source shape, not a
  standalone tier marker.

Open limits:
- Some span-occlusion callers still carry BN limited reconstruction markers
  because x87 scratch and depth comparisons render verbosely in HLIL.

## zImage::CreateDefaultTextureRecord

Evidence:
- Caller address: `0x46d550` `zImage::InitTextureDirectory`.
- Instruction/source pattern: source-faithful helper recovered from this caller
  body, which routes the
  `DEFAULT_TEXTURE`/default-image contract through
  `g_zVideo_pfnCreateTextureRecord` and stores the returned texture record in
  `g_zImage_DefaultTextureRecord`.
- Likely original owner/source file: `GameZRecoil/zImage/zimg_texture.cpp`.
- No standalone retail function is expected: no standalone plan/BN function was
  identified for this helper in the inspected evidence; the behavior appears as
  caller-local setup around the zImage default texture globals and active
  zVideo texture callback.

Restored source form:
- namespace helper: `zImage::CreateDefaultTextureRecord()`.
- Callers using it: `zImage::InitTextureDirectory`.

Verification notes:
- Native tests: `zvideo_init_video_system` and
  `zimage_init_texture_directory` cover callers through focused smoke targets.
- VC byte/source-cluster attempt: no standalone helper byte target exists;
  `zImage::InitTextureDirectory` remains below tier `B` pending its owner/data
  audit, and `zVideo::InitVideoSystem` now uses its own direct default-image
  callback shape.
- Known tier `S` limits: accepted only as recovered helper source shape, not a
  standalone tier marker.

Open limits:
- `0x46d550` remains source-owner/data pending. `0x4a75f0` is tracked on its own
  owner-projection entry because BN shows a direct null-name call to the default image
  texture record callback.

## SaveLoadEntryCount

Evidence:
- Caller addresses: `0x434fb0` `HudUiSaveLoadDialog::DeleteSaveFile`,
  `0x435160` `HudUiSaveLoadNextButton::OnActivate`, and `0x4351b0`
  `HudUiSaveLoadPrevButton::OnActivate`.
- Repeated pattern: null `fileEntries.begin` returns zero; otherwise count is
  `fileEntries.end - fileEntries.begin` over `HudUiSaveLoadEntry` records.
- Likely owner/source cluster: save/load dialog code under
  `Battlesport/RecoilApp.cpp` / `HudUiSaveLoadDialog.cpp`.
- No standalone retail function is expected: BN shows the count expression
  inlined in every observed caller and no standalone call target.

Restored source form:
- `inline int SaveLoadEntryCount(const HudUiSaveLoadDialog *dialog)` in the
  anonymous namespace for the save/load dialog source cluster.
- Callers using it: `HudUiSaveLoadDialog::DeleteSaveFile`,
  `HudUiSaveLoadNextButton::OnActivate`, and
  `HudUiSaveLoadPrevButton::OnActivate`.

Verification notes:
- Native tests: save/load delete, next, and prev button smokes exercise callers
  through their class methods.
- VC byte/source-cluster attempt: no standalone helper byte target exists;
  caller tier `S` is deferred to the save/load dialog/button cluster.

Open limits:
- Accepted only as recovered inline source shape, not a tier `S` marker for any
  caller.
