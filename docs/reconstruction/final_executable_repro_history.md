---
document_status: historical-process-record
operational_guidance: false
superseded_by: docs/reconstruction/final_executable_repro.md
---

# Final Executable Reproducibility History

> **Historical process record — not current operating guidance.**

This file is an archive of dated experiments and evidence. It is not the live
queue or current command surface. Use `final_executable_repro.md` and the live
current `verify final-image` / `audit final-data` output whenever they conflict with
historical wording below.

## Final Data Tail Evidence

Historical final-data observation from 2026-06-29: attribution in
`build/vc5-final/final_data_diff_virtual_tail_parent.json` showed the reference
`.data` tail `0x775d08..0x779ac0` was missing from the candidate image; the
corresponding candidate window `0x762d08..0x766ac0` was outside the candidate
`.data` section. The missing reference range begins inside
`g_zVideo_OverwriteQueueBase`, then continues through OptCatalog runtime,
context, warning, and queued-impact globals, `g_zWeapon_MaxTetherAltitude`,
`g_Player_LocalFxOffsetWorldPtr`, and CRT/common provider rows at
`0x779ab0..0x779abc`.

That observation's focused deltas were:

- `.data` RVA delta: `-0x13000`
- `.data` raw-size delta: `-0x600`
- `.data` virtual-size delta: `-0x3db8`
- `.data` zero-fill-tail delta: `-0x37b8`

These facts block final linked layout identity and affected owner byte gates,
but they do not by themselves disprove the accepted source-owner or data-owner
gates for the tail rows. The same audit produced no direct owner-action owners
or addresses; it only emitted diagnostic owner/address correlation rows.

Relink-only order probes under `build/vc5-final-order-probes/` tested the
remembered tail-order hypotheses without changing the canonical final-build
manifest. The current-regime probes showed:

- moving `OptCatalog.obj` after `zVideo.obj`: virtual delta worsened to
  `-0x3dc8`;
- moving `OptCatalog.obj` and `zWeapon.obj` after `zVideo.obj`: virtual delta
  worsened to `-0x3dc8`;
- moving `player.obj`, `OptCatalog.obj`, and `zWeapon.obj` after
  `zVideo.obj`: virtual delta worsened to `-0x3dc8`;
- moving `zSys.obj` before `zVideo.obj`: no change from `-0x3db8`;
- moving `upgrade_download.obj` and `zSys.obj` before `zVideo.obj`: no change
  from `-0x3db8`.
- moving `zinterp_parse.obj` after `zVideo.obj`: no change from `-0x3db8`;
- moving `zinterp_parse.obj` before `zVideo.obj`: no change from `-0x3db8`;
- moving `zinterp_parse.obj`, `OptCatalog.obj`, and `zWeapon.obj` after
  `zVideo.obj`: virtual delta worsened to `-0x3dc8`;
- moving `zinterp_parse.obj` after `ainet.obj`: virtual delta worsened to
  `-0x3dc8`.

Do not promote any of these probe orders into
`tools/_recoil/config/vc5_final_build.json`. Future final-data order probes
should record the exact object-order mutation beside each JSON artifact because
older probe artifacts capture numeric deltas but not always the source-list edit
that produced them.

`tools/vc5_verify_targets/optcatalog_runtime_callback_globals.json` accepts
both short object-local and full-path VC5 anonymous-namespace decorations for
`g_OptCatalogQueuedImpacts`. This prevents final-data diagnostics from treating
the candidate symbol at `0x558518` as missing when only the decorated map name
changed. The owner-scoped VC5 target verifies `0x778970` with zero unmasked
data-byte mismatches, but final linked `.data` layout still blocks byte-gate
acceptance until the global section deltas above are resolved.

Follow-up final-data audit on 2026-06-29 kept the same section deltas and
narrowed the likely cause away from simple source-size omissions in the largest
candidate BSS owners. The latest strict audit artifact is
`build/vc5-final/final_data_diff_bss_audit_parent.json`; the nonzero result is
still the expected blocked final-layout result, not a tool failure.

The raw-backed missing reference tail `0x4e5600..0x4e5c00` is a mixed
initialized-data window, not one owner. It contains mostly zInterp literals and
data, `g_zInterp_UnresolvedFloatDefaults` at `0x4e5954..0x4e5a4f`, provider or
compiler RTTI-like rows at `0x4e5b10..0x4e5b27`, a zVideo format string at
`0x4e5b28..0x4e5b48`, and the start of Player
`g_Player_AivParentDir` at `0x4e5b50`. A focused source audit of zInterp found
no source-faithful declaration, relocation, xref, copy site, or owner evidence
for adding `g_zInterp_UnresolvedFloatDefaults`; at that time adding a float
array there would only move bytes and remained rejected.

The zero-fill missing reference tail `0x775d08..0x779ac0` decomposes as the
tail of `g_zVideo_OverwriteQueueBase`, an 8-byte gap, OptCatalog runtime rows
including the `0x1100` queued-impact records, zWeapon/Player tail scalars, and
the CRT/common provider rows. Focused audits ruled out these large candidate BSS
contributors as the single `0x3db8` virtual-size cause:

- `zVideo.obj`, `zRndr.obj`, `WestwoodOnlineUpgradeApi.obj`, `zInput.obj`,
  `OptCatalog.obj`, `player.obj`, `zWeapon.obj`, `zSys.obj`, and
  `upgrade_download.obj` were already accounted for by earlier final-data
  evidence.
- `zimg_texture.obj` matches the reference-sized `0x24000`
  `g_zImage_TexDirEntries` and `0x50` font pointer table; its remaining small
  difference is padding/alignment scale, not `0x3db8`.
- `Camera.obj` matches the `0x8d68` frustum grid ring array; the remaining
  reference/candidate placement difference is only small unlabeled gap scale.
- `zModel_Display.obj` and `gmod_light.obj` match their large scratch arrays,
  active-light arrays, clip-attribute arrays, and material/light state sizes.
- `zhud_ui.obj` accounts for `g_HudUiMgr` as `0x7844`; the apparent `+0x4`
  public-symbol extent is padding, and its hidden `0x5` BSS contribution is
  consistent with the `HudUi::PlayPowerupSfx` local static pointer and guard.

Current next evidence should compare reference-side object/subsection or
owner-order contribution more directly. More broad relink order probes or
source padding tables are unlikely to be source-faithful without a new BN,
object, map, or tool fact tying the missing raw/virtual tails to an original
source declaration.

Tail manifest attribution was added to `audit final-data` on 2026-06-29. The
post-OptCatalog artifact
`build/vc5-final/final_data_diff_tail_manifest_after_optcatalog_parent.json`
kept the same section deltas but split manifest coverage from real attribution
gaps:

- raw tail: 44 BN rows, 5 unmatched rows. The substantive authored unmatched
  row remains `g_zInterp_UnresolvedFloatDefaults` at `0x4e5954..0x4e5a50`
  (`0xfc` bytes). The other unmatched raw rows are two unlabeled pointers before
  RTTI, the RTTI/type-info provider candidate at `0x4e5b10..0x4e5b28`, and one
  unlabeled pointer before `g_Player_AivParentDir`.
- virtual tail: 38 BN rows, 4 unmatched rows after OptCatalog manifest repair.
  The remaining unmatched virtual rows are CRT/common provider data:
  `__fdiv_adjust_snapshot`, `__matherr_installed`, `__onexitend`, and
  `__onexitbegin` at `0x779ab0..0x779ac0`.

Follow-up BN/provider packets on 2026-06-29 classified those unmatched tail
rows more tightly. `type_info_RttiTypeDescriptor` at `0x4e5b10..0x4e5b28` is
the existing VC5/MSVCRT `type_info` TypeDescriptor provider row
(`??_R0?AVtype_info@@@8` / `MSVCRT:ti_inst.obj` in the candidate map), with
BN xrefs through the `type_info` RTTI graph and vftable/destructor context. The
zero dwords at `0x4e5b08`, `0x4e5b0c`, and `0x4e5b4c` have no xrefs, owner-ledger
rows, relocations, or provider symbols; treat them as alignment/filler or
unresolved anonymous raw zero rows, not authored storage and not provider-boundary
candidates. The four virtual-tail rows at `0x779ab0..0x779ac0` are already
accepted VC5 CRT common provider data in the owner ledger. After this classification,
the remaining substantive authored raw-tail blocker is still only
`g_zInterp_UnresolvedFloatDefaults` at `0x4e5954..0x4e5a50`; it has no BN
xrefs, copy site, relocation, recovered source name, or source-faithful
declaration/model, so adding a 63-float source table was rejected under the
normal source-owner rule. The later user-approved exception below is the only
allowed non-source-faithful closure path for this row.

The same session saved BN cleanup for the filler/provider rows: `0x4e5b08` now
covers the eight zero bytes `0x4e5b08..0x4e5b0f` as
`rdata_alignment_before_type_info_RttiTypeDescriptor`, `0x4e5b4c` is
`rdata_tail_filler_after_zVideo_PaletteOpenFailedFormat`, and `0x4e5b10`
keeps its provider `type_info_RttiTypeDescriptor` identity with a VC5/MSVCRT RTTI
comment. These BN changes are reconstruction hygiene only; they do not alter
owner acceptance or the unresolved `0x4e5954` data-owner blocker.

`audit final-data` now emits `reference_tail_source_summary` for the raw and
virtual tails. The parent artifact
`build/vc5-final/final_data_diff_tail_source_summary_parent.json` keeps the
same section deltas but groups the raw-tail manifest-backed bytes as:

- `src/GameZRecoil/zInterp/zinterp_parse.cpp`: 37 manifest-backed items,
  `0x3df` bytes, target `zinterp_parser_runtime_data`;
- `src/GameZRecoil/zVideo/zVideo.cpp`: 1 manifest-backed item, `0x21` bytes,
  target `zvideo_palette_brightness_runtime`;
- `src/Battlesport/player.cpp`: 1 manifest-backed item, `0x104` bytes, target
  `player_mission_runtime_bootstrap_globals`;
- unmatched: 4 items, `0x120` bytes: `g_zInterp_UnresolvedFloatDefaults`
  (`0xfc`), the two filler/provider rows around `type_info`, and the filler
  dword after the zVideo palette format string.

This grouped summary strengthens the raw-tail accounting but still does not
prove source ownership for `g_zInterp_UnresolvedFloatDefaults`. Source-owner
review still rejects adding that `float[0x3f]`/candidate `zVec3[21]` table to
`zinterp_parse.cpp`, Player/camera, or zVideo without a new xref, relocation,
copy site, original source declaration, or object/subsection attribution.

`audit final-data` also now emits `candidate_boundary_packing`, a bounded
candidate-side map/link-response diagnostic for the `.data` map end, raw end,
BSS start, and virtual end. The canonical refreshed artifact
`build/vc5-final/final_data_diff_boundary_packing_parent.json` keeps the same
strict blocker (`rva=-0x13000`, `raw_size=-0x600`, `virtual_size=-0x3db8`,
`zero_fill_tail=-0x37b8`) and records that candidate `.data` still ends at map
offset `0xb4e0`, BSS starts at `0xb4e0`, and PE raw alignment extends to
`0xb600`. This confirms the missing retail raw tail is still backed by BSS in
the candidate rather than by a directly actionable owner command.

`audit final-data` emits `candidate_initialized_data_thresholds` for the same
candidate-side layout question. The field records the current initialized
`.data` end, BSS start when known, candidate raw/virtual ends, retail raw end,
candidate PE file alignment, and raw-aligned threshold rows with nearby
map-symbol and object/subsection context. Use it to identify which candidate
objects or provider members sit around the thresholds that would move the raw
end before assigning source work.

`audit final-data` now also emits `candidate_boundary_contribution_summary`,
a compact object/subsection view of the same candidate boundaries. The parent
artifact
`build/vc5-final/final_data_diff_boundary_contribution_parent.json` preserves
the strict deltas and shows the candidate `.data` end and BSS start coincide at
offset `0xb4e0` (`0x4d24e0`): `zVideo.obj` and provider
`MSVCRT:ti_inst.obj` sit immediately before the boundary, while `ainet.obj`
and `Briefing.obj` start at/after it. The candidate raw end is `0xb600`
(`0x4d2600`) inside `.bss`, near `Briefing.obj`,
`HudUiMessageBoxDialog.obj`, `RecoilApp_Late.obj`, and `GameNet.obj`
contributions. The candidate virtual end `0x29bd08` (`0x762d08`) follows
provider/common CRT/MFC tail symbols. Treat this as stronger evidence that the
next final-data work is object/subsection ordering and BSS extent accounting.
The unresolved zInterp numeric block is now handled only by the narrow
address-specific exception described below.

Follow-up diagnostic manifest
`build/vc5-final-order-probe-bss-front-after-zvideo/manifest_order_probe.json`
moved only the candidate BSS-front authored cluster
(`ainet.obj`, `Briefing.obj`, `HudUiMessageBoxDialog.obj`,
`RecoilApp_Late.obj`, and `GameNet.obj`) immediately after `zVideo.obj`,
leaving provider libraries in their canonical order. The probe linked with PE
and resource comparisons intentionally skipped, then strict final-data audit
remained blocked: `raw_size` stayed `-0x600`, while `virtual_size` worsened to
`-0x3dc8` and `zero_fill_tail` worsened to `-0x37c8`. The candidate `.data`
end/BSS start moved earlier from `0xb4e0` to `0xb4d0`; the boundary shifted
from `zVideo.obj`/`MSVCRT:ti_inst.obj` before `ainet.obj`/`Briefing.obj` to
`GameNet.obj`/`MSVCRT:ti_inst.obj` before `hud.obj`, but the PE raw end stayed
`0xb600`. Treat this probe as rejected diagnostic evidence: the visible
BSS-front authored cluster controls local boundary occupants, but it does not
increase initialized `.data` length or reduce the raw-size blocker.

A 2026-06-29 read-only object/subsection attribution pass over
`build/vc5-final/final_data_diff_coverage_parent.json`,
`build/vc5-final/final_data_owner_actions_coverage_parent.json`,
`build/vc5-final/Recoil.map`, `build/vc5-final/rsp/link.rsp`, and the
generated final-build objects confirms the current raw-alignment thresholds.
The candidate initialized `.data` end and BSS start still coincide at section
offset `0xb4e0` (`0x4d24e0`), while the PE raw end is `0xb600` (`0x4d2600`)
and the retail raw end is `0xbc00`. From this boundary, initialized data
before BSS would need to grow by at least `0x121` bytes to move the raw size at
all (`0xb800`), at least `0x321` bytes to reach `0xba00`, and at least
`0x521` bytes to reach the retail `0xbc00` raw size.

The same pass confirms that the retail raw-tail window `0x4e5600..0x4e5c00`
is explained as content by zInterp, zVideo, Player, and provider/filler rows,
but not as a source-faithful candidate-side implementation fix. The focused
zInterp raw-tail rows from
`g_zInterp_Err_NodeNoGraphicsDataForCycledTexture` through
`g_zInterp_Cmd_CameraSetHorizon` span `0x239` bytes and sum to `0x21e` bytes;
all zInterp raw-tail rows sum to `0x4db`, and the manifest-backed zInterp rows
excluding `g_zInterp_UnresolvedFloatDefaults` sum to `0x3df`. The unresolved
`g_zInterp_UnresolvedFloatDefaults` row is only `0xfc`, below even the `0x121`
threshold needed to move the candidate raw size from `0xb600` to `0xb800`.
The current `zinterp_parse.obj` already emits its initialized zInterp `.data`
much earlier in the candidate map, around offsets `0x52f0..0x6420`, and emits
no `0xfc` contribution matching the unresolved block.

Boundary object inspection also leaves no justified source-edit target. Immediately
before the candidate `.data` end are initialized `zVideo.obj` data, including
`_g_zVideo_PaletteOpenFailedFormat` near offset `0xb498`, and the provider
`MSVCRT:ti_inst.obj` type-info row near `0xb4c0`. The candidate BSS window
then starts with `ainet.obj`, `Briefing.obj`, `HudUiMessageBoxDialog.obj`,
`RecoilApp_Late.obj`, `GameNet.obj`, `hud.obj`, and later
`WestwoodOnlineUpgradeApi.obj`; these are BSS occupants around the missing raw
window, not proven initialized-data omissions. Use the generated
`candidate_initialized_data_thresholds` report to correlate final map offsets,
generated COFF section classes, and cumulative raw-alignment thresholds for the
initialized contributions that could legally move `data_end` from `0xb4e0`
past `0xb600`, `0xb800`, and `0xba00`. Do not assign a source worker until
that threshold evidence ties a row/subsection to original source or provider
provenance.

The final-build driver now supports an optional diagnostic `link_inputs` field
for ordered source/library link probes without changing the canonical manifest.
A parent-created probe manifest at
`build/vc5-final-order-probe/manifest_order_probe.json` placed
`zinterp_parse.obj`, `MSVCRT.LIB`, `zVideo.obj`, and `player.obj` at the tail
of the link response to test the retail-like raw-tail ordering hypothesis. The
probe linked and passed resource comparison, but strict final-data audit stayed
blocked: `raw_size` remained `-0x600`, while `virtual_size` changed from
`-0x3db8` to `-0x3dbc` and `zero_fill_tail` changed from `-0x37b8` to
`-0x37bc`. Treat this probe as rejected diagnostic evidence; do not move the
canonical final-build order from it.

A follow-up raw-tail VC5 symbol-section probe under
`build/vc5-raw-tail-manifest-probe` compiled
`zinterp_parser_runtime_data`, `zvideo_palette_brightness_runtime`, and
`player_mission_runtime_bootstrap_globals` with BN comparison intentionally
skipped. It showed the current generated COFF object classes for the raw-tail
candidate symbols:

- zInterp manifest literals around `0x4e55f4..0x4e5af8` emit as initialized
  `.data`, except the token delimiter entry which emits as `.rdata`, and the
  known zInterp scratch/runtime globals which emit as `.bss`.
- `g_zVideo_PaletteOpenFailedFormat` emits as initialized `.data`; the palette
  path/brightness/file/system palette buffers emit as `.bss`.
- `g_Player_AivParentDir` emits as `.bss`, while the nearby Player first-run
  flag and missing-AIV diagnostic literal emit as initialized `.data`.

This weakens further broad link-order probing: moving link inputs can reorder
already-emitted contributions, but it cannot make `g_Player_AivParentDir`
raw-backed initialized data. A scratch VC5SP3 section-placement probe under
`build/vc5-zero-init-section-probe` confirmed that VC5 places all tested
all-zero `char[0x104]` definitions in `.bss` for both C++ and C modes,
including `= {0}`, `= ""`, `= {'\0'}`, and `= "\0"`; only nonzero initializer
controls emitted as `.data`.

Do not change `g_Player_AivParentDir` just to force `.data` placement. Current
BN/source/owner evidence supports it as an authored zero-fill output buffer
written by `zReader::BuildResolvedParentDir`. Its retail address
`0x4e5b50..0x4e5c54` straddles the PE raw/zero-fill boundary, so the first
`0xb0` bytes being raw-backed does not prove initialized-source semantics. The
accepted Player mission-bootstrap owner and data gates remain valid; the
remaining authored raw-tail blocker is still the unresolved zInterp numeric
range `0x4e5954..0x4e5a50`, and the remaining whole-executable blocker is the
global final linked `.data` layout.

Focused 2026-06-29 source/BN/VC packets for `0x4e5954..0x4e5a50` found no new
implementation-grade owner evidence. BN reports no code or data xrefs to any
aligned float in the 63-float block; adjacent zInterp functions only reference
neighboring strings such as `AddChild`, the keyword validation formats, and
`ScrollAlways`. Source review found no command descriptor/default table, no
`zInterp_RuntimeBlob` member, and no camera/Player/zClass structure that owns
the exact 21 triples. A diagnostic VC5 compile of `zinterp_parser_runtime_data`
under `build/vc5-zinterp-unresolved-float-probe` showed the current
`zinterp_parse.obj` does not emit any named symbol, anonymous local, `0xfc`
section contribution, or gap corresponding to the block. The adjacent generated
COFF sections are separate known literals: keyword validation strings in `.data`
COMDAT sections, `AddChild` in a later `.data` COMDAT, and
`g_zInterp_ScrollAlwaysNodeName` in primary `.data`. Leave
`input_script_config.zinterp_anonymous_numeric_cluster_data` blocked; adding a
`float[63]` or `zVec3[21]` table would still be byte placement without a
source-faithful owner.

Follow-up final-data-layout packets on 2026-06-29 refreshed the canonical
artifacts `build/vc5-final/final_data_diff.json` and
`build/vc5-final/final_data_owner_actions.json` with
`candidate_initialized_data_thresholds`. The strict deltas remained unchanged:
`rva=-0x13000`, `raw_size=-0x600`, `virtual_size=-0x3db8`, and
`zero_fill_tail=-0x37b8`, with no direct tier-S owner, owner-address, or
affected-owner issue. Candidate initialized `.data` still ends at offset
`0xb4e0`, candidate raw end is `0xb600`, retail raw end is `0xbc00`, and the
next raw-aligned thresholds require `0x121`, `0x321`, and `0x521` more
initialized bytes from `data_end`. The BSS-side objects around those
thresholds (`ainet.obj`, `Briefing.obj`, `HudUiMessageBoxDialog.obj`,
`RecoilApp_Late.obj`, `GameNet.obj`, `hud.obj`, and later
`WestwoodOnlineUpgradeApi.obj`) remain boundary occupants, not evidence for
forcing initialized data. A focused Westwood owner check found its existing
source/data blockers unrelated to this raw-size gap and did not justify a
source edit. A focused zInterp placement check found the 37
manifest-backed raw-tail rows present in the current candidate object but
placed much earlier in candidate `.data` (`0x4cc340..0x4cd324`), while the
unresolved `0xfc` block at `0x4e5954..0x4e5a50` is absent from the candidate
object and still has no BN xrefs or owner proof. Adding only that block would
not cross the first raw-size threshold. The next safe action remains
source/data-owner or original object/map evidence for the `0xfc` float block,
or another initialized contribution that can move `data_end` past `0xb600`;
do not run another order-only probe unless it is expected to increase
`candidate_initialized_data_end_offset` beyond the current raw end.

Later on 2026-06-29, the Westwood API owner blockers were resolved as
ledger/model cleanup rather than final-data layout work. The stale standalone
`0x4f53e4` data row was retired because current BN/source evidence models it as
`WestwoodOnlineUpgradeApiInitState::eventSinkLiveCount` inside the canonical
`0x4f53d0` init-state aggregate. The three API GUIDs at `0x4d1838`,
`0x4d1848`, and `0x4d18d8` remain valid the unified tracker `.rdata` primary-data
links without legacy markdown rows, and owner-scoped VC5 verification selected and
byte-matched the Westwood data/GUID rows. The owner source/data gates and
`0x4f53d0` tier-B data row are now accepted, but the owner byte gate remains
deferred because functions `0x42dda0`, `0x43d130`, and `0x43d2e0` still have
function byte/codegen drift. This does not explain or repair the global
`raw_size=-0x600` or `zero_fill_tail=-0x37b8` final-data deltas.

A subsequent read-only source/BN/object pass on 2026-06-29 strengthened the
same blocked routing. The reference bytes for `0x4e5954..0x4e5a4f` come from
reference `.data` file offset `0xe3b54` and decode as 21 triples beginning
`(0, 2.2, 0.2)`, `(2, -2.2, 0.2)`,
`(-2, -2.2, -0.2)`, `(2, 2.2, -0.2)`, and `(-2, 0, 5)`, followed by sparse
zero/unit/`5`/`-5` triples. Raw-byte scans found no full `0xfc` hit in
`build/vc5-final/Recoil.exe` or any `build/vc5-final/obj/**/*.obj`, and no
object hit for the distinctive first `0x20` or `0x40` bytes. Map and LLVM
symbol inspection found no `UnresolvedFloatDefaults` or `FloatDefaults` symbol;
`zinterp_parse.obj` only exposes the known neighboring `g_zInterp_*` rows and
normal scratch/runtime globals. Binary Ninja section listing and PE extraction
place this range in `.data`; any packet line calling it `.rdata` is stale
against current section evidence. Keep
`input_script_config.zinterp_anonymous_numeric_cluster_data` blocked: the row is
absent from generated artifacts, but source search still found no exact
declaration, descriptor table, copy site, relocation, xref, or complete owner
model. If a future source model emits this missing block, it still needs at
least `0x25` additional initialized bytes or padding beyond the `0xfc` table to
move the current initialized data end past `0xb600`.

Another final-data pass on 2026-06-29 confirmed that the current raw-size
delta is not a zero-padding-only discrepancy. In the retail executable,
`.data` offset `0xb4e0..0xbc00` contains `0x720` bytes, `0x505` of them
nonzero, with first and last nonzero offsets `0xb4e0` and `0xbb47`. The first
printable runs are zInterp parser strings such as the tail of `FindNode`,
`FindNode %s: FAILED`, `DisplaySetClearColor`, `interp: DeleteTree (%s) --> NULL
NODE`, `CycleTextureSetSpeed`, and
`Node (%s) has no graphics data for cycled texture`. The candidate executable
has only `0x120` bytes present in the same offset window before its raw end,
and all of those bytes are zero. The refreshed strict final-data audit still
reports `rva=-0x13000`, `raw_size=-0x600`, `virtual_size=-0x3db8`, and
`zero_fill_tail=-0x37b8`, with candidate initialized `.data` ending exactly at
the `.bss` start `0xb4e0`.

The same packets narrowed the boundary routing. `zinterp_parse.obj` already
emits its candidate initialized/string data much earlier in the map, with no
`zinterp_parse.obj` symbols around `0xa800..0xc000`; the retail tail is
zInterp-heavy content whose candidate counterpart has moved out of the tail,
not content that can be explained by the nearby candidate BSS owners. The
boundary owners around candidate offsets `0xb4e0..0xb840` (`ainet.obj`,
`Briefing.obj`, `HudUiMessageBoxDialog.obj`, `RecoilApp_Late.obj`, `GameNet.obj`,
and neighboring `zVideo.obj`) are already source/data accepted or locally
data-symbol covered and are symptoms of placement drift, not source
implementation targets. The `MSVCRT:ti_inst.obj`
`??_R0?AVtype_info@@@8` symbol at candidate offset `0xb4c0` remains provider
RTTI/type_info data covered by the original-address provider row `0x4e5b10`;
the `HudUiMessageBoxDialog.obj` empty string literal at `0xb548` is a
compiler-generated literal from existing authored `SetTextFmt("")` call sites.
Neither justifies new owner rows, owner-ledger changes, or an authored source
editing. The next evidence-producing work is therefore zInterp source-owner
recovery plus COFF subsection/linker placement comparison for
`zinterp_parser_runtime_data`, not provider reclassification or boundary-owner
source implementation.

The canonical `audit final-data` report was refreshed later on 2026-06-29 so
default `--limit` display truncation no longer hides later raw-tail BN rows from
`reference_tail_source_summary` or manifest coverage accounting. The current
`build/vc5-final/final_data_diff.json` still reports the same strict deltas
(`rva=-0x13000`, `raw_size=-0x600`, `virtual_size=-0x3db8`,
`zero_fill_tail=-0x37b8`), but now explicitly lists four raw-tail
manifest-uncovered spans totaling `0x120` bytes: the unresolved
`g_zInterp_UnresolvedFloatDefaults` block at `0x4e5954..0x4e5a50`, the
`0x4e5b08..0x4e5b10` alignment row, the VC5/MSVCRT `type_info` row at
`0x4e5b10..0x4e5b28`, and the `0x4e5b4c..0x4e5b50` zVideo-adjacent filler
dword. The same report attributes the manifest-backed raw tail to 37 zInterp
rows (`0x3d3` overlap bytes), one zVideo row (`0x21` bytes), and the
`g_Player_AivParentDir` overlap (`0xb0` bytes). This is diagnostic only: the
`zinterp_parser_runtime_data` VC5 target passes for its 78 accepted data rows,
but the unresolved `0xfc` float block is absent from current
`zinterp_parse.obj`, has no xref/copy-site/relocation/source declaration, and
remains blocked as `input_script_config.zinterp_anonymous_numeric_cluster_data`
under the normal source-owner rule. The later user-approved one-row exception
allows only the emitted `data-equivalent-only` preservation table for
`0x4e5954..0x4e5a50`; it does not authorize a general raw `float[0x3f]` or
`zVec3[21]` source pattern. Outside that exception, the next useful
evidence remains source-owner, original object/map, copy-site, xref, or COFF
subsection attribution that proves a real source model, or another initialized
contribution that can move candidate `data_end` beyond the current raw boundary.

The read-only `audit bn-data-evidence` command was added the same day to make
this kind of data-owner blocker reproducible without changing BN state. Running
`python tools/recoil.py audit bn-data-evidence 0x4e5954 --size 0xfc
--constants float --nearby 0x60 --json --max-assembly-functions 120` confirms
the exact BN item `g_zInterp_UnresolvedFloatDefaults`, its 63 decoded floats,
and the adjacent zInterp string rows. Its assembly-text constant scan is only a
partial diagnostic and currently finds common constants such as `1` and `5` in
unrelated early functions; it is not source-owner proof. The command also
uses the read-only BN bridge `getXrefsTo` route for direct xrefs; current output
queries all 63 aligned float addresses from `0x4e5954..0x4e5a50` and returns no
direct code/data xref hits. Relocation enumeration remains unsupported by the
current read-only BN bridge surface. Use this command to preserve evidence
boundaries; promotion of `0x4e5954` is valid only through the approved
address-specific orphan initialized-data exception and its VC5 data-symbol
evidence.

`tools/vc5_verify_targets/optcatalog_free_trail_runtime_state_storage.json`
now covers the accepted OptCatalog tail globals
`g_OptCatalogRuntimeInstanceCount`, `g_OptCatalogRuntimeInstancePool`,
`g_OptCatalogFreeRuntimeInstanceList`, `g_OptCatalogNetworkOptionState`,
`g_OptCatalogPendingSpawnTargetCountPtr`,
`g_OptCatalogPendingSpawnTargetListPtr`, and `g_OptCatalogNextSpawnScale`.
Address-selected VC5 verification for those seven data rows passes with zero
unmasked data-byte mismatches. The full target still exits nonzero because the
pre-existing function row at `0x4b1f90` has a code byte mismatch; that does not
contradict the added data-symbol evidence.

`audit final-data` now also emits `candidate_threshold_attribution`, a
read-only COFF/map attribution companion to
`candidate_initialized_data_thresholds`. It records candidate initialized-data
end, BSS start, candidate/reference raw ends, raw-alignment thresholds,
bytes-needed-from-data-end math, nearby map symbols/objects, and explicit
limitations. The report is diagnostic only: it does not build, relink, generate
order probes, prove source ownership, or justify owner/data marker acceptance.

`audit final-data` now also emits `candidate_object_subsection_attribution` to
make the candidate-side object/subsection evidence explicit without treating it
as owner proof. The refreshed canonical artifact
`build/vc5-final/final_data_diff.json` still has
`candidate_initialized_data_end_offset=0xb4e0`, `bss_start_offset=0xb4e0`,
`candidate_raw_end_offset=0xb600`, `reference_raw_end_offset=0xbc00`, and
threshold byte counts `0x121`, `0x321`, and `0x521`. The new field shows
`zinterp_parse.obj` has `0x1012` initialized-data bytes and `0x8e0` BSS bytes,
and `zVideo.obj` has `0x1555` initialized-data bytes, so both satisfy the
threshold sizes arithmetically. That is placement attribution only:
read-only BN/source/provider packets still found no xrefs, relocation,
copy-site, original source declaration, object-local symbol, or accepted source
model for `g_zInterp_UnresolvedFloatDefaults` at `0x4e5954..0x4e5a50`. A
corrected parent byte-pattern probe using the reference `.data` raw pointer
`0xd8200` found no full `0xfc` match and no distinctive prefix match in any
linked candidate object; only isolated nonzero triples appear in unrelated
objects, which is not ownership evidence. Keep
`input_script_config.zinterp_anonymous_numeric_cluster_data` blocked and do not
schedule a source worker to add a raw float/vector table until a source-owner or
original object/map/subsection fact proves the original data model.

The later byte-composition summary for the same canonical final-data artifact
keeps that conclusion intact while making the raw-tail composition explicit.
The missing retail raw tail `0x4e5600..0x4e5c00` is file-backed and contains
`0x600` bytes: `0x40e` nonzero bytes and `0x1f2` zero bytes, with first/last
nonzero bytes at `0x4e5600` and `0x4e5b47`. Its candidate-corresponding window
`0x4d2600..0x4d2c00` is `zero-fill-bss`, so no file-backed candidate bytes are
available for that window. A fresh BN fact packet over `0x4e5900..0x4e5b60`
confirmed that `g_zInterp_UnresolvedFloatDefaults` remains only a
`float[0x3f]` data item with no base xref, no interior dword xrefs across
`0x4e5954..0x4e5a4c`, no pointer/relocation identity, and no source-owner
identity beyond the conservative BN comment. Adjacent zInterp strings are still
included only as neighborhood evidence; the RTTI/type_info, zVideo, and Player
neighbors are excluded by provider or source-cluster evidence. Under normal
rules this supports continued source-owner recovery, not source implementation
or marker promotion; the later exception below is the only approved closure path
without source-faithful owner recovery.

A historical review pass on 2026-06-29 reached the same boundary with
current BN, final-data JSON, source, object, and provider evidence. BN confirms
the `0xfc` item is `.data` `float[0x3f]` at `0x4e5954..0x4e5a50`, between
`g_zInterp_Cmd_AddChild` and `g_zInterp_FormatKeywordArgCount`, with no base or
interior xrefs and no observed loads from nearby `DispatchCoreCommand`,
`ValidateArgsAndNodeType`, or `RegisterScrollAlwaysNode` assembly. The adjacent
zInterp strings strengthen source-file neighborhood only; they do not prove the
float block's source model. The raw-tail RTTI/filler rows after the zInterp
strings were reclassified as non-authored context for scheduling purposes:
`0x4e5b10..0x4e5b28` is already an accepted VC5/MSVCRT `type_info`
provider-boundary row, while `0x4e5b08..0x4e5b10` and `0x4e5b4c..0x4e5b50` are
zero alignment/filler diagnostics with no xrefs or owner actions. The improved
`final_data_owner_actions.json` sample now exposes row-level diagnostic drift,
but its command batches remain empty (`direct_s_tier_issues=0`) and the sample
does not create an unblocked source-owner task. User-approved policy now allows
only `0x4e5954..0x4e5a50` to proceed as the one non-reusable
address-specific orphan initialized-data exception, with `data-equivalent-only`
modeling and the evidence requirements in `AGENTS.md`. This does not authorize
a general source-edit
implementation pattern or any future orphan data row.

The 2026-06-30 CRT-initializer recovery pass added source-backed VC5 `.CRT$XCU`
rows for the current authored/static-lifecycle owners already proven by BN:
`RecoilStateCredits`, `RecoilStateCheatCode`,
`HudUiNewGamePanelOverlayOwner`, `HudUiNetGameSetupOverlayOwner`, `HudUiMgr`,
`Mission::InitObjectives`, `HudUiTriplet`'s
`g_HudUiTripletWndClassName` CString lifecycle, and the
`HudUiSensorWindow` CWnd lifecycle. It also classified
`0x4c8214` / `0x4da004` as the provider-owned MFC static MBCP initializer
boundary, not authored Recoil source. After that pass, strict owner audit and
source guards were clean, and final-build still compiled, linked, and
resource-compared; PE comparison remains blocked by final reproducibility.

The refreshed final-data audit after those rows reports `.data`
`rva=-0xe000`, `raw_size=-0x600`, `virtual_size=-0x3dc8`, and
`zero_fill_tail=-0x37c8`, with candidate map `.CRT$XCU` size `0xe4` and
candidate data end/BSS start at offset `0xb4d0`. The added rows improved the
virtual/zero-fill deltas by `0x30` relative to the pre-pass baseline, but the
candidate constructor order is still not retail-shaped: current link order puts
`GameNet.obj` rows first, then `hud.obj`, `RecoilStateCredits.obj`, pickup,
player, main-menu, zSound, `mission.obj`, and finally `zhud_ui.obj` rows.
Retail order begins with the provider MFC row and early app/HUD/zInterp
startup rows before the GameNet/Pickup block. Treat the remaining blocker as
linked object/subsection order and provider-row reproduction evidence, not as
permission to add copied CRT tables, manual padding, or source rows without BN
owner evidence.

A focused 2026-06-30 follow-up removed the duplicate VC5 automatic initializer
row for `RecoilStateMainMenuTransition.obj` by changing
`g_RecoilState_MainMenuTransition` from a typed global object into an explicit
0x18-byte storage union with typed access at source use sites. The explicit
`s_MainMenuTransitionCrtInit` `.CRT$XCU` row remains, the
`recoil_state_main_menu_transition_global_data` VC5 data target now verifies the
storage symbol with zero mismatches, and the static-init functional smoke still
passes. This is retained as source-faithful local evidence for removing one
duplicate constructor row, but it did not resolve final layout: refreshed
`audit final-data --include-owners` reports `.data` `rva=-0xe000`,
`raw_size=-0x600`, `virtual_size=-0x3dd8`, and
`zero_fill_tail=-0x37d8`, with candidate data end/BSS start now at offset
`0xb4c0`. The `0x10` virtual/zero-fill regression confirms that remaining
work is still broader `.CRT$XCU`/object/BSS ordering evidence rather than a
local main-menu data-byte problem.

A second focused 2026-06-30 follow-up applied the same evidence pattern to
`RecoilStateCredits`: `g_RecoilStateCredits` now emits as an explicit
8-byte `RecoilStateCreditsStorage` object with typed source access, while the
explicit `s_RecoilStateCreditsCrtInit` `.CRT$XCU` row remains. The final map no
longer shows `RecoilStateCredits.obj` automatic `_$E` startup rows; it retains
the class methods, scalar-deleting destructor alias, vtable, explicit CRT row,
and storage symbol. The refreshed `recoil_state_credits_global_data` VC5 target
verifies the storage symbol with zero byte mismatches, and the static-init,
static-register, and register-at-exit functional targets still pass. The
current final-data audit reports `.data` `rva=-0xe000`, `raw_size=-0x600`,
`virtual_size=-0x3dd0`, and `zero_fill_tail=-0x37d0`, so removing the credits
duplicate row recovered `0x8` of virtual/zero-fill size relative to the
post-main-menu state but did not move the raw-size blocker or the
`0xb4c0` data/BSS boundary. Continue treating the remaining blocker as global
constructor/object ordering and BSS extent evidence.

The next 2026-06-30 HUD static-lifetime storage pass converted the five
`hud.cpp` typed singleton globals with explicit CRT rows into explicit storage:
`g_HudUiNewGamePanelOverlayOwner`, `g_HudUiOptionsPanelOverlayOwner`,
`g_RecoilState_ConfirmQuit`, `g_RecoilStateControls`, and
`g_RecoilStateCheatCode`. The candidate map no longer has automatic `_$E`
rows from `hud.obj`, and the explicit `s_BattlesportHudCrtInit_*` rows remain.
Updated VC5 data targets for new-game owner, options owner, confirm-quit state,
and cheat-code state pass with zero unmasked data-byte mismatches; controls
lifecycle functional coverage also still passes. This removes a source-backed
set of duplicate HUD constructor rows but does not resolve final layout:
refreshed final-data audit reports `.data` `rva=-0xe000`,
`raw_size=-0x600`, `virtual_size=-0x3dd8`, and
`zero_fill_tail=-0x37d8`, with candidate data end/BSS start at offset
`0xb4b0`. The result confirms the remaining blocker has moved to other
objects' automatic rows and global link/BSS ordering, not the repaired HUD
singleton storage itself.

A focused zInterp follow-up on 2026-06-30 removed the `zinterp_parse.obj`
automatic `_$E1`/`_$E2` rows for the process-wide interpreter object by
changing `g_zInterp_GlobalContext` from a typed automatic global into explicit
`zInterp_GlobalContextStorage` under the original symbol, with typed macro
access for source uses. `zInterp_GlobalContext::StaticInit` placement-constructs
the object in that storage before calling the recovered constructor, and the
explicit `s_zInterpCrtInit_GlobalContext` `.CRT$XCU` row remains the startup
entry. The final map now has no `zinterp_parse.obj` `_$E`/`??__E`/`??__F` rows,
retains `?s_zInterpCrtInit_GlobalContext@@3P6AXXZA`, and emits
`?g_zInterp_GlobalContext@@3TzInterp_GlobalContextStorage@@A`. The updated
`zinterp_context_family_data` VC5 target verifies all eight data rows with zero
unmasked mismatches, and the `zinterp_global_context_static_init` and
`zinterp_global_context_hooks` functional targets still pass. Final-build still
fails only at the whole-PE comparison stage, and refreshed final-data audit
remains blocked with `.data` `rva=-0xe000`, `raw_size=-0x600`,
`virtual_size=-0x3dd8`, and `zero_fill_tail=-0x37d8`, with candidate data
end/BSS start at offset `0xb4b0`. Remaining automatic startup rows are now
concentrated in `zhud_ui.obj`, `player.obj`, `RecoilApp_Late.obj`,
`CZRecoilFrame.obj`, and one `WestwoodOnlineUpgradeDialog.obj` row.

A subsequent 2026-06-30 `zhud_ui.obj` storage pass converted the explicit
HUD static-lifecycle globals from typed automatic objects into zero-initialized
storage wrappers while preserving typed source access and the explicit
`.CRT$XCU` rows. The repaired globals are `g_HudUiMgr`,
`g_HudLayoutHW`, `g_HudLayoutSW`, `g_HudCmdDialogState`,
`g_HudUiNetGameSetupOverlayOwner`, `g_HudUiTripletWndClassName`, and
`g_HudUiSensorWindow`. The final map now has no `zhud_ui.obj` `_$E`/`_$X`/
`??__E`/`??__F` rows; the explicit HUD CRT rows remain. The updated VC5 data
targets verify the HUD manager, layout vtables/singletons/strings, net-game
setup owner, and triplet window-class storage with zero unmasked data-byte
mismatches. `hud_cmd_dialog_state_lifecycle` now verifies all entries except
`0x40bc30 HudCmdDialogState::StaticInit`: explicit storage requires
source-faithful placement construction, which VC5 emits as a placement-new
null-check/call/return sequence instead of the retail compiler-generated
automatic global-constructor tail jump. Do not force that byte shape with raw
assembly, copied compiler thunks, or fake wrappers.

The focused HUD lifecycle functional targets still pass, source-shape and
original-symbol guards report zero findings, and final-build still compiles,
links, and passes resource comparison before the expected whole-PE comparison
failure. The refreshed final-data audit remains blocked with `.data`
`rva=-0xe000`, `raw_size=-0x600`, `virtual_size=-0x3e00`, and
`zero_fill_tail=-0x3800`, with candidate data end/BSS start at offset
`0xb490`. Remaining automatic startup rows are now concentrated in
`player.obj`, `RecoilApp_Late.obj`, `CZRecoilFrame.obj`, and one
`WestwoodOnlineUpgradeDialog.obj` row.

A follow-up 2026-06-30 `player.obj` storage pass converted the remaining typed
player static-lifecycle globals into explicit zero-initialized storage wrappers
under their original symbols while preserving typed source access and the
source-authored `.CRT$XCU` rows. The repaired globals are
`g_Player_UnderwaterFxPass3Ui`, `g_Player_State7FxPass3Ui`,
`g_Player_TopMsgPanel1`, and `g_Player_TopMsgPanel2`. A narrow integration
fix also added a conventional include guard to `src/GameZRecoil/Time/time.h`
after the focused player VC5 verification wrappers exposed a
`TimeRuntimeConfig` redefinition through their include shape; final-build
already compiled that header path, so this was validation-path hygiene rather
than new reconstruction evidence.

The final map now has no `player.obj` `_$E`/`_$X`/`??__E`/`??__F` rows; the
explicit Player CRT initializer rows remain. `player_underwater_fx_pass3_ui_global`,
`player_camera_state_globals`, and `player_bootstrap_globals` all pass VC5
data-symbol verification with zero unmasked mismatches after the guard fix.
Focused player lifecycle functional targets pass except for two validation
registry entries whose smoke functions existed in `tests/native/player_tests.cpp`
but were not yet exposed by `tests/native/smoke.cpp`; those are tracked as a
native-smoke registry repair, not as production source failures. Source-shape,
original-symbol, and docblock audits for `src/Battlesport/player.cpp` report
zero findings.

Final-build still compiles, links, and passes resource comparison before the
expected whole-PE comparison failure. The refreshed final-data audit remains
blocked with `.data` `rva=-0xf000`, `raw_size=-0x600`,
`virtual_size=-0x3e10`, and `zero_fill_tail=-0x3810`, with candidate data
end/BSS start at offset `0xb480`. Remaining automatic startup rows are now
concentrated in `RecoilApp_Late.obj`, `CZRecoilFrame.obj`, and one
`WestwoodOnlineUpgradeDialog.obj` row.

A later 2026-06-30 `RecoilApp_Late.obj` storage pass converted the two
remaining typed process globals in that object into explicit zero-initialized
storage while preserving typed source access. `g_RecoilApp` now uses
`RecoilAppStorage` with an explicit `s_RecoilAppCrtInit` row that
placement-constructs the application object and registers its destructor.
`g_RecoilStateSaveLoadTransition` now uses
`RecoilStateSaveLoadTransitionStorage`, and the explicit
`s_RecoilStateSaveLoadTransitionCrtInit` row points to
`RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit`. The final map no
longer has any `RecoilApp_Late.obj` `_$E`/`_$X`/`??__E`/`??__F` rows.

After parent manifest integration, `recoil_app_register_at_exit_late` verifies
the RecoilApp destructor, play-state constructor, RecoilApp constructor, and
the complete `g_RecoilApp` data symbol with zero unmasked mismatches.
`recoil_state_save_load_transition_singleton_data` verifies the complete
`g_RecoilStateSaveLoadTransition` storage with zero unmasked mismatches.
`recoil_state_save_load_transition_lifecycle` still passes the static-init,
register-at-exit, and at-exit helper bodies but fails the local constructor and
destructor byte checks (`0x435c80` has 16 unmasked mismatches and `0x435cc0`
has 67). Do not force those constructor/destructor bodies with raw assembly or
fake startup thunks; they remain a local byte-shape blocker.

Final-build still compiles, links, and passes resource comparison before the
expected whole-PE comparison failure. The refreshed final-data audit remains
blocked with `.data` `rva=-0xf000`, `raw_size=-0x600`,
`virtual_size=-0x3e10`, and `zero_fill_tail=-0x3810`, with candidate data
end/BSS start at offset `0xb480`. Remaining automatic startup rows are now
concentrated in `CZRecoilFrame.obj` and one `WestwoodOnlineUpgradeDialog.obj`
row.

A later 2026-06-30 cleanup removed the remaining automatic startup/helper rows
from the final map. `g_HudSensorTracker` now uses explicit zero-initialized
storage in `CZRecoilFrame.cpp` with typed source access through
`HudSensorTrackerStorage`; `czrecoilframe_hud_sensor_tracker_global` and
`czframe_mfc_metadata` both verify with zero unmasked VC5 data-symbol
mismatches, and the former `CZRecoilFrame.obj` automatic rows were proven to
belong to the HudSensorTracker global rather than to MFC metadata. The
function-local `pendingStatusText` CString in
`WestwoodOnlineUpgradeDialog::SubmitVisibleSessionRequestsAndStatusText` now
uses explicit guard/storage symbols and an explicit atexit destructor helper;
`westwood_online_upgrade_dialog_runtime_data` verifies all seven linked data
rows with zero unmasked mismatches. A final-map scan for `??__E`, `??__F`,
`_$E`, and `_$X` now returns no rows.

Final-build still compiles, links, and passes resource comparison before the
expected whole-PE comparison failure. The remaining blocker is no longer
automatic startup/helper emission; it is the final `.data` section
raw/virtual/BSS layout. The current final-data audit remains blocked with
`.data` `rva=-0xf000`, `raw_size=-0x600`, `virtual_size=-0x3e0c`, and
`zero_fill_tail=-0x380c`, with candidate data end/BSS start at offset
`0xb480`. The missing raw-backed tail maps into the zInterp parser data,
unresolved float defaults, zVideo palette format, provider RTTI, and
`g_Player_AivParentDir` window. The generated
`recoil_state_save_load_transition_lifecycle` native smoke now builds, and the
functional verifier reports VC byte evidence for all six lifecycle functions,
but the generated runtime smoke still fails/crashes; treat that as
validation-path drift, not as a final-data source blocker.

The refreshed strict final-data audit for the same candidate wrote
`build/vc5-final/final_data_diff.json` and confirms that `data_end` and
`bss_start` are both at segment offset `0xb480`, while the candidate raw end is
`0xb600` and the retail raw end is `0xbc00`. The active thresholds require the
initialized data end to advance past `0xb600`, `0xb800`, and finally `0xbc00`;
the matching-reference threshold is `0x581` bytes beyond current `data_end`
(`0x780` bytes to fill the target raw end). The boundary occupants after
`0xb480` are BSS-side objects (`ainet.obj`, `Briefing.obj`,
`HudUiMessageBoxDialog.obj`, `RecoilApp_Late.obj`, `GameNet.obj`, `hud.obj`,
and then `WestwoodOnlineUpgradeApi.obj`). `westwood_online_upgrade_session_browser_data`
still byte-verifies its large browse-record list and related counters with zero
unmasked mismatches; those globals are original BSS in retail and are not a
license to force initialized source data. The next source change needs new
owner/source evidence for an initialized contribution or a linker/layout fix
that genuinely moves `data_end`, not another zero-initialized storage tweak.

The refreshed `build/vc5-final/final_data_diff.json` selected COFF symbol
attribution gives the next durable placement facts without changing source
ownership. Selected matched COFF rows are:

- `_g_zInterp_UnresolvedFloatDefaults`: `zinterp_parse.obj` `.data` section
  order `2`.
- `_g_zVideo_PaletteOpenFailedFormat`: `zVideo.obj` `.data` section order `2`.
- `_g_zVideo_OverwriteQueueBase`: `zVideo.obj` `.bss` section order `1`.
- `_g_Player_LocalFxOffsetWorldPtr` and `_g_Player_AivParentDir`:
  `player.obj` `.bss` section order `3`.

Treat these rows as candidate map/COFF object-subsection placement evidence from
`audit final-data`, not as source-owner, provider-classification, or gate
acceptance evidence. They do not reopen the rejected broad zInterp tail,
source-order, `link_inputs`, or tail-order probe paths. The next final-data work
still needs a source-owner/provider fact or linker/layout fact that explains the
linked `.data`/`.bss` boundary shift source-faithfully.

A focused provider pass classified the remaining unmatched raw/virtual tail rows.
The real provider rows are now tracked as accepted provider boundaries:

- `provider.vc5_msvcrt_type_info_rtti_descriptor`: `0x4e5b10..0x4e5b28`,
  the VC5/MSVCRT `type_info` RTTI TypeDescriptor (`MSVCRT:ti_inst.obj`,
  `??_R0?AVtype_info@@@8`).
- `provider.vc5_crt_startup_common_tail_globals`: `0x779ab0..0x779ac0`,
  the VC5 CRT startup/common globals `__fdiv_adjust_snapshot`,
  `__matherr_installed`, `__onexitend`, and `__onexitbegin`.

The adjacent zero rows `0x4e5b08..0x4e5b10` and `0x4e5b4c..0x4e5b50` remain
filler/alignment diagnostics only. They are not authored data owners,
provider-boundary owners, or source-edit targets.
