# Compiler And Linker Provenance

This ledger records the current compiler/linker assumptions used for source-faithful Recoil reconstruction. The machine-readable baseline is `tools/_recoil/config/compiler_linker_profiles.json`; `python tools/recoil.py audit provenance --strict` checks the active VC verification manifests against it.

## Role Of Each Build

- VC5SP3 `cl` 11.00.7022 is the only tier `S` compiler for authored production source.
- Current executable provenance points to VS97 SP3: VS97 SP3 `cvtres` produced 1 object and VS97 SP3 `link` 5.10.7303 linked the executable.
- Modern MSVC/CMake builds are smoke and guard builds only. They prove compile/link/test health, not tier `S` equivalence.
- Tier `S` acceptance normally requires relocation-masked COFF byte comparison. VC5 manifests can also compare generated `.data`/`.rdata` data symbols for data-gate evidence. Listings, normalized text diffs, and relocation identity reports are triage/review aids, not source-shape acceptance modes.

## Final Candidate Build Assumptions

The final candidate executable driver is `tools/_recoil/config/vc5_final_build.json`.

- Compiler environment: `D:\Recoil Project\Compiler\VC5SP3\vc5sp3-env.cmd`
- Compiler flags: `/nologo /TP /W3 /G5 /O2 /Ob0 /MD /GX /Gr /Zp4`
- Resource flags: `/r`
- Link flags: `/nologo /MACHINE:IX86 /SUBSYSTEM:WINDOWS /INCREMENTAL:NO /FIXED /BASE:0x400000`
- Runtime and provider inputs: VC5SP3 CRT/MFC import libraries, repo-local MFC42, repo-local DirectX 6 libraries, and the original PE facts in `.agent/REFERENCE_EXECUTABLE.json`.

These flags are reconstruction assumptions for the current final candidate driver, not proof that every original translation unit used this exact tuple. Per-function verification manifests must use documented VC5SP3 profiles only.

## Verification Profiles

Accepted per-target verification profiles are intentionally narrow:

- `vc5_o2_ob0_facs`: VC5SP3 object-byte verification profile for plain non-EH code.
- `vc5_o2_ob1_gx_facs`: VC5SP3 profile when one-level inlining and C++ EH, constructor, destructor, or member-call cleanup shape are relevant.
- `vc5_o2_ob1_facs`: VC5SP3 one-level inlining profile without C++ EH for plain leaf/helper code whose local evidence points to `/Ob1`.
- `vc5_o2_ob1_gx_uintptr_facs`: VC5SP3 one-level inlining profile with a `UINT_PTR` compatibility define for focused full-TU MFC frame targets whose production headers otherwise require a later SDK typedef.
- `vc5_o2_ob1_md_gx_facs`: VC5SP3 profile for functions where the original bytes use DLL CRT import-call forms such as `sprintf`.
- `vc5_o2_ob0_md_gx_fastcall_facs` and
  `vc5_o2_ob1_md_gx_fastcall_facs`: VC5SP3 dynamic-CRT profiles using the
  recovered `/Gr` project default, with `/Ob0` or `/Ob1` selected per
  translation-unit evidence. Explicit cdecl declarations remain cdecl; `/Gr`
  governs otherwise unannotated free and static functions.
- `vc5_o2_ob2_facs` and `vc5_o2_ob2_gx_facs`: VC5SP3 profiles for local evidence requiring aggressive inlining.
- `vc5_o2_oy_ob0_facs`: documented VC5SP3 profile with frame-pointer omission for targets whose evidence requires `/Oy`.
- `vc5_zsys_cpu_raw_asm`: documented exception for approved zSys CPU raw-assembly probes.
- `vc5_o2_ob0_md_zrndr_mmx_raw_asm_facs`: documented user-approved exception for the zRndr overlay RGB555/RGB565 MMX row leaves, enabled only with `RECOIL_ENABLE_ZRNDR_OVERLAY_MMX_RAW_ASM`. This permits narrow inline `__asm` MMX loops inside ordinary C++ functions only; it does not permit `__declspec(naked)`, `_emit`, `.asm`, raw byte emission, unrelated zRndr span/MMX families, provider shims, or future raw assembly.
- `vc5_o2_ob0_md_zrndr_span_mmx_raw_asm_facs`: documented user-approved exception for zRndr span callbacks where current BN proves authored MMX blocks: `0x49ea80`, `0x49ec20`, `0x49e400`, `0x49e560`, `0x49cbb0`, `0x49cea0`, `0x49da80`, and `0x49ddb0`. Enabled only with `RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM`. This permits inline `__asm` only for the necessary MMX blocks while C++ keeps the function shell, setup, scalar edge/tail logic, scratch-buffer preparation, and portable fallback. It does not permit whole-function raw assembly, `__declspec(naked)`, `_emit`, `.asm`, raw byte emission, provider shims, or non-MMX blocks.
- `vc5_o2_ob0_md_zrndr_esp_pivot_raw_asm_facs`: documented user-approved exception for the five zRndr ESP-pivot span leaves at `0x49b7e0`, `0x49bbf0`, `0x49e6c0`, `0x49edc0`, and `0x49f180`, enabled only with `RECOIL_ENABLE_ZRNDR_ESP_PIVOT_RAW_ASM`. This permits narrow inline `__asm` loops inside ordinary C++ functions only; it does not permit `__declspec(naked)`, `_emit`, `.asm`, raw byte emission, the user-approved span-MMX block family, provider shims, or future raw assembly.

New VC verification manifests should use `"compiler_profile": "<profile-name>"`.
Adding a new compiler environment or flag tuple requires updating this ledger
and `tools/_recoil/config/compiler_linker_profiles.json` with the reason and expected compiler
version. Do not silently add one-off flags to a VC manifest.

## Profile Sentinel Guards

Already byte-matched functions are compiler-profile sentinels. If a candidate
profile does not preserve those confirmed bytes for the same source block, that
profile is disqualified for the block; do not keep using it merely because a
later, still-unmatched function has a lower mismatch count. When a manifest has
`profile_guard.accepted_profiles`, that list is the enforced remaining valid
profile set for the manifest. `python tools/recoil.py verify vc5` rejects
explicit `--compiler-profile` use of profiles outside the accepted set or inside
`profile_guard.disqualified_profiles`, and skips them in `--profile-sweep` by
default. `--allow-disqualified-profile` is only for an explicit provenance
re-open with new sentinel evidence.

When a newly byte-matched function rules out another profile, update the same
manifest guard immediately: remove that profile from
`profile_guard.accepted_profiles`, add it to
`profile_guard.disqualified_profiles`, and record the sentinel address plus the
mismatch or compile-failure evidence. This keeps future byte-matching sweeps
restricted to the current viable profile set.

Current confirmed guards:

- `src/Battlesport/about.cpp`: the five-function
  `cabout_prelude_functions` sentinel (`0x401000`, `0x401020`,
  `0x401030`, `0x401040`, `0x401050`) byte-matches with
  `vc5_o2_ob0_md_facs`. The current accepted profile set is
  `vc5_o2_ob0_md_facs`, `vc5_o2_ob0_md_gx_gr_facs`,
  `vc5_o2_ob1_md_gx_facs`, `vc5_o2_ob1_md_facs`,
  `vc5_o2_ob2_md_facs`, `vc5_o2_ob2_md_gx_facs`, and
  `vc5_o2_oy_ob0_md_facs`. The 2026-07-04 sweep disqualified the non-`/MD`
  MFC-incompatible profiles, the explicit `/Oy-` profiles, and the `/O1`
  diagnostic profile for the About block.
- `src/Battlesport/ai_net.cpp`: `0x401060`
  `AINet::TickAiMode2TopLevel` is a byte-matched sentinel. The 2026-07-04
  sweep disqualified the non-`/MD` MFC-incompatible profiles, the `/Ob2`
  `/MD` profiles, and the `/O1` diagnostic profile for the ai_net block. The
  current accepted profile set is `vc5_o2_ob0_md_facs`,
  `vc5_o2_ob0_md_gx_gr_facs`, `vc5_o2_ob1_md_gx_facs`,
  `vc5_o2_ob1_md_facs`, `vc5_o2_oy_ob0_md_facs`,
  `vc5_o2_oyminus_ob0_md_zi_facs`, `vc5_o2_oyminus_ob0_md_z7_facs`,
  `vc5_o2_oyminus_ob0_md_gx_facs`, `vc5_o2_oyminus_ob0_md_facs`,
  `vc5_o2_oyminus_ob1_md_facs`, and
  `vc5_o2_oyminus_ob0_md_nog5_facs`. Prefer the manifest default and repair
  source/header/include shape unless new sentinels prove a different profile.

VC verification manifests may contain `functions`, `data_symbols`, or both.
`data_symbols` entries compare a BN data address and byte length against a VC5
COFF symbol with relocation masking, and write a relocation identity report for
pointer/symbol review. Passing data-symbol bytes can support initialized-data
evidence only after the normal source-owner and complete data-gate criteria are
met. The compared symbol must cover the complete touched authored data owner or
the complete initialized-global data set being accepted; a passing field-sized
slice inside a larger authored global is not enough for an accepted owner data gate.

## HUD Fastcall Compiland Investigation

The 2026-07-26 investigation of
`HudUiZrdScrollingText::LoadFromZrd` (`0x409570`) established a narrow
compiler-profile fact without establishing a production translation-unit
boundary:

- Retail `std::copy` providers at `0x40a170` and `0x40be60` are standard
  Microsoft x86 `__fastcall` functions. Their `YI` decorations and raw bodies
  agree on `ECX = first`, `EDX = last`, one stack destination, `EAX` return,
  and `ret 4`. The saved `Recoil.bndb` records the exact identities
  `?copy@std@@YIPAUHudUiPanelLayoutEntry@@PAU2@00@Z` and
  `?copy@std@@YIPAPAUHudCmdBindingEntry@@PAPAU2@00@Z`.
- A clean `vc5_o2_ob1_md_gx_fastcall_facs` diagnostic compile of the current
  flattened `src/Battlesport/hud.cpp` emits the exact `YI` layout-entry
  specialization, but `LoadFromZrd` still folds the empty-range copy out of
  `templateSpan.clear()` and calls only `_Destroy`. Retail instead retains
  `std::copy(old_end, old_end, begin)`, then `_Destroy(copy_result, old_end)`,
  and stores the returned end.
- Retail and candidate agree before that lowering on the `0x2d8` frame, EH
  shape, saved-register set, 16-byte span local, `0x2ac` entry local, three
  named-node queries, and control flow. A direct
  `erase(begin(), end())` diagnostic also failed to reproduce the retail pair.
  Further local reset-expression tuning is therefore not justified.
- Applying `/Gr` to the whole current `hud.cpp` creates 71 unique cross-TU
  link mismatches. Every affected identity has zero explicit arguments, so its
  retail assembly cannot distinguish `__cdecl` from `__fastcall`; those
  failures do not justify blanket annotations. A global `/Gr` diagnostic
  compiled 83 of 90 translation units, but sampled non-HUD retail fastcall
  sentinels were already explicitly annotated and therefore do not prove a
  project-wide default.
- The additive `vc5_o2_ob1_md_gx_cdecl_facs` diagnostic profile is the existing
  HUD profile with one explicit `/Gd` in both verification and reviewed
  final-build compile flags. It exists only to make mixed calling-convention
  experiments deterministic; no production source mapping uses it.
- An isolated, compile-closed `/Gr` diagnostic containing only the natural
  credits/scrolling-text owner declarations and definitions emitted the exact
  `YI` layout-entry specialization. `LoadFromZrd` nevertheless lowered the
  reset directly to `_Destroy(begin, old_end)` and stored `begin`. Reducing the
  current merged HUD translation unit therefore does not by itself recover the
  retail call-retention decision.
- A clean mixed-profile diagnostic created a PCH containing `<algorithm>` and
  `<vector>` under `/Gr`, then consumed it from an otherwise byte-identical HUD
  source snapshot under explicit `/Gd`. VC5 issued C4652, stating that the
  current command-line option overrides the PCH's fastcall option. The consumer
  emitted `YA` `HudCmdBindingEntry` `std::copy` and `std::transform` providers,
  emitted no `YI` layout-entry provider, and again lowered the reset directly
  to `_Destroy`. A `/Gr` standard-library PCH combined with a `/Gd` HUD
  consumer is therefore falsified; the independent command-binding provider
  sentinel failed before a link could add useful evidence.

Subsequent 2026-07-26 compiler-profile recovery supersedes the earlier
production-`/Gd` disposition. The governed final candidate now uses `/Gr` as
the project default, with `hud.cpp` on
`vc5_o2_ob1_md_gx_fastcall_facs`. The exact retail `YI` identities and raw
Microsoft x86 fastcall contracts of the two independent `std::copy` providers
establish that profile fact. Source declarations that already carry an explicit
convention retain it; the cdecl callback and variadic exceptions are represented
as actual cdecl declarations rather than casts. The final driver also validates
the zero-argument cross-TU ABI-equivalence rows recorded in
`vc5_final_build.json`. This profile recovery does not by itself establish the
source owner, physical translation-unit boundary, or exact nested-copy
retention at `0x409570`.

The remaining blocker is the unrecovered original TU/header/compiler context
that controlled VC5SP3 `/Ob1` retention of the nested copy call. A proposed
four-contribution object probe at natural owner boundaries—pre-HUD `/Gd`,
`HudUiCreditsPanel` `/Gr`, `RecoilStateCredits` `/Gd`, and post-HUD
`/Gd`—was preflighted and stopped before compilation. Although construct-
complete global-scope cuts are lexically possible, the post-HUD body consumes
TU-local option/video/audio constants and confirm-quit strings defined only in
the pre-HUD contribution and not declared by existing shared headers. Making
all four objects compile would therefore require scratch declarations,
duplicate definitions, a fifth shared contribution, or moving unrelated data.
The credits slice also contains unrelated folded logical definitions. Those
conditions violate the diagnostic's no-scaffolding and natural-owner stop
rules, so no scratch or production split was created. Retry requires new direct
evidence for the original shared declaration/data boundary or another
independently compilable original TU topology.

The bounded post-probe source census found no second natural
`HudUiPanelLayoutEntry` specialization context in the current source tree. The
only authored ODR-use family is the `LoadFromZrd` family itself:
`templateSpan.clear()`, the subsequent `templateSpan.insert(...)`, and the
outer `rows.insert(...)`. Other current occurrences provide declaration/type
visibility or are excluded from VC5 compilation. The `zui.cpp` and
`zui_widgets.cpp` translation units include `zhud_ui.h` but do not ODR-use this
specialization. The retained `HudCmdBindingEntry` provider is a different
pointer specialization with different helper fan-out and cannot be borrowed
without inventing or moving an authored construct.

Consequently, the reviewed `/Gr` baseline, isolated-owner, mixed-PCH,
direct-erase, and natural-source-context branches are exhausted. Production
keeps `templateSpan.clear()` and the recovered `/Gr` HUD mapping; the remaining
local mismatch is VC5SP3's elimination of the empty-range nested copy call in
the current merged header/TU context. New work requires direct evidence for an
original PCH/header split or another real layout-entry ODR-use. Explicit
template instantiations, forwarding wrappers, fake consumers, synthetic
special-member scaffolding, and unsupported translation-unit splits are not
evidence-backed resolutions.

ChatGPT Pro independently reviewed this disposition as advisory evidence in
room `agent:recoil-root:critic`; the final bounded pass was request/run
`2026-07-26T00-28-18-760Z-chatgpt-call`, with the session transcript written
under `.devspace/runs/<run-id>/transcript.md`. No files were uploaded. The
retail Binary Ninja facts and governed VC5SP3 results above remain the
authoritative evidence; the temporary Pro transcript is not required to
reproduce them.

## Required Checks

Run these before relying on compiler/linker provenance:

```powershell
python tools/recoil.py audit provenance --strict
python tools/recoil.py guard vc5-manifest
python tools/recoil.py verify vc5 --all --skip-bn-compare
```

`python tools/recoil.py doctor --quick` also runs the provenance audit.
