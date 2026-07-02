# Source File Layout Audit Notes

Binary Ninja remains authoritative for function bodies, calls, data, and
assembly. The generated `source_file_map.md` is only a navigation aid built from
current source docblocks and legacy comments; it can preserve stale source-path
claims when the implementation has been placed in the wrong file.

This audit is now the first Recoil.exe binary-lane scheduling surface for
physical source-file order. `owner next --lane binary` and
`audit backlog --lane binary` surface `work_unit=source-file-block-map` before
`final-repro`/`final-data-layout` while block-order or provisional physical-file
facts remain unreconciled with source owners. Final linked `.data` layout remains
a final validation lane unless block-order work exposes a direct data/object
ordering cause.

## `zError::ReportOldNoOp` Source-File Literals

Retail `zError::ReportOldNoOp` call sites pass compiler-emitted source-file
path literals. For Battlesport files these literals appear in alphabetical
source order in `.rdata` and provide a strong file-placement constraint. The
current evidence supports a translation-unit contribution model: the compiler
emitted each original source file as a physical block of `.text`, then the next
source file's functions followed. Agents must use this block order when
recovering source owners, before trusting semantic names or stale source-path
comments. For a known Recoil.exe physical block, the rebuilt VC5 object must
naturally emit the covered functions in the same order as the retail/reference
`.text` order seen in Binary Ninja before the owner is treated as byte-ready.
If generated order differs, treat it as a source-shape/include-shape blocker
until current evidence proves otherwise.

- `0x4da1e8` `"D:\Proj\Battlesport\ai_net.cpp"` is referenced by
  `AINet::LoadFromZrd` at `0x4030bb`, with the diagnostic call at `0x4030c5`.
- `0x4da32c` `"D:\Proj\Battlesport\Briefing.cpp"` is referenced by
  `Briefing::StartMissionBriefingThread` at `0x404238`, with the diagnostic
  call at `0x404242`.
- `0x4dc26c` `"D:\Proj\Battlesport\player.cpp"` is first referenced by
  `Player::ApplyMissionSaveData` at `0x41f20b`; the function starts at
  `0x41f1d0`.

This makes accepted `player.cpp` source-owner claims in the early
`0x401060..0x403870` AI/net neighborhood suspect unless independent current BN
evidence proves a helper, provider boundary, COMDAT placement exception, or a
different original source construct.

## Provisional No-Literal Physical File Names

No-literal authored ranges now carry `provisional_original_path` in
`tools/_recoil/config/source_file_blocks.json`. Agents should use these names as
compile-order placement labels for reconstruction and VC5 function-order work.
They are not literal-backed source-path gates, do not accept owner gates or
tiers, and do not override complete source-owner, header/COMDAT,
semantic-owner, or provider-boundary evidence. Provider and padding ranges keep
provider/padding labels only.

Agent-facing source-file block work uses `agent_source_path` for the placement
file. The tool derives it from `original_source_path`, then
`provisional_original_path`, then `source_path`, so compatibility `source_path`
can remain stale or shorthand without sending source workers to the wrong file.
The AINet reference block is literal-backed by
`D:\Proj\Battlesport\ai_net.cpp`; agents should place that block in
`src/Battlesport/ai_net.cpp`, not legacy `src/Battlesport/ainet.cpp`. Validate
continuity and placement metadata with
`python tools/recoil.py audit source-blocks --strict` after block-map edits.
Use `python tools/recoil.py audit source-blocks --list` to inspect the
flattened address-ordered rows, including provider rows, padding rows, `.cpp`
rows, and address-emitting `partial-header` rows. A `partial-header` row is
source-shape placement evidence only: reconstruct the emitted code in the row
`source_path` header and compile it through `included_in`/`physical_owner_path`,
but do not treat the row as proof of full header extent or accepted owner
gates/tiers. Declaration-only and type-only headers stay in
`source_shape_inputs` unless an emitted address range is known. Some no-literal
rows are `semantic-block` rows: they identify BN-proven semantic islands and
candidate source/header paths, but the original source file and physical host
are unresolved. Reconstruction agents must test source/header arrangements with
VC5 until the compiler emits the retail order naturally; do not resolve these
rows with `.inl` files, forced placement, or fake `.cpp` filenames.

### Special No-Literal Late Shelf `[0x4b2960,0x4c0d20)`

Treat `[0x4b2960,0x4c0d20)` as a known no-literal exception to the
literal-backed alphabetical folder model. It sits between literal-backed
`zwep_init.c [0x4ae380,0x4b2960)` and
`zinterp_parse.cpp [0x4c0d20,0x4c5a50)`, but the interior rows have no
source-path literal evidence. Current evidence supports physical sub-blocks and
semantic islands, not original translation-unit identity or global alphabetical
project order.

Use the current paths as provisional compile-order placement labels only:
`zgame_opt.c [0x4b2960,0x4b33f0)`, `zsys_cpu.cpp [0x4b33f0,0x4b3ce0)`,
`zui.cpp [0x4b3ce0,0x4bffe0)`, and
`zutl_zbd.cpp [0x4bffe0,0x4c0d20)`. The `zui.cpp` label is especially
provisional: focused BN review makes `src/GameZRecoil/zUI/zui.cpp` the current
physical placement label for the reusable widget/control/dialog/background
bodies, while finer `zUI/zui_*` splits and
`src/GameZRecoil/zWeather/zweather_fx.cpp` remain semantic filename hypotheses.
No current source-path literal, object-map evidence, or VC5 natural-order proof
accepts those exact filenames as physical rows.
Reconstruction agents must recover complete
source owners and local emitted function order inside this shelf. Do not force
semantic owners into globally alphabetical files, add `.inl` files, or use
linker/pragma ordering tricks to explain the placement. If current BN/VC5
evidence proves a different physical split or original path, update the
machine-readable source-block catalog and this audit before implementing
against the corrected model.

The active block catalog now covers the full BN `.text` retail range
`[0x401000,0x4cb9e8)`. Do not leave active gaps; provider, padding, and
no-literal authored prelude ranges need explicit physical blocks.

| Physical range | Provisional physical file name | Notes |
| --- | --- | --- |
| `[0x437e60,0x438980)` | `src/Battlesport/util.cpp` | Provisional physical host for mixed helper shelf: zClass recursive helpers, zVideo hotkey helper, Object3D model-ref queue, HUD message-box wrapper, zSaveGame helpers, and HUD sensor track-list append. BN xref sweep found no callers after `0x43ce80`; semantic subranges remain recorded in the source-block database. |
| `[0x438980,0x438990)` | `src/Battlesport/version.cpp` | Tiny `RecoilVersion::GetString` accessor. Current source-map evidence used `RecoilVersion.cpp`, but that filename would sort near `RecoilApp.cpp`; `version.cpp` fits the recovered alphabetical position between `util.cpp` and `weapon.cpp`. |
| `[0x438990,0x43ce80)` | `src/Battlesport/weapon.cpp` | Provisional physical host for pickup/airdrop helpers followed by player weapon banks, hardpoints, aim/fire pipeline, damage feedback, alt-gun/mines runtime, kill-verb callback, and mission weapon availability. BN xref sweep found no callers after `0x43ce80`; semantic subranges remain recorded in the source-block database. |
| `[0x43ce80,0x43cf90)` | `src/Battlesport/WinSock.cpp` | NetUi/zStr/GameNet transition immediately before WOL. |
| `[0x43cf90,0x442890)` | `src/Battlesport/WOL.cpp` | Westwood Online dialog/API/event-sink/config/progress/download cluster. BN order interleaves those class layers, so current evidence supports one provisional aggregate placement label rather than separate per-class physical `.cpp` rows. |
| `[0x4428b0,0x443730)` | `src/CZGameFrame/AppFrame.cpp` | Late RecoilApp/MFC/OLE/state-queue frame glue with 6 recorded semantic subranges; separate-folder hypothesis explains position after Battlesport WOL. |
| `[0x443730,0x443b70)` | `src/CZGameFrame/CZGameFrame.cpp` | CZGameFrame runtime/message-map/window methods with 8 recorded semantic subranges; includes one zVideo cached-client-rect helper island at `0x443a40`, called only by local `CZGameFrame::OnSize`/`OnMove` and not a proven emitted header row. |
| `[0x454360,0x4558f0)` | `src/GameZRecoil/zClass/cls_zbd.c` | Literal-backed ZBD block now includes the no-literal `0x454360 zClass::ResetCurrentZbdPath` prefix; no separate `cls_path.c` source file is proven. |
| `[0x470020,0x4706c0)` | `src/GameZRecoil/zInput/zin_mouse.cpp` | Bracketed/order-backed mouse runtime block; no local source-path literal, so this remains a provisional physical placement label. |
| `[0x4706c0,0x4719e0)` | `src/GameZRecoil/zInput/zinput.cpp` | Bracketed/order-backed core bind-map/context/overlay runtime; no local source-path literal, so owner gates must use the semantic zInput owners. |
| `[0x471e40,0x472670)` | `src/GameZRecoil/zInput/zin_joystick.cpp` | Bracketed/order-backed joystick tail. `0x472450..0x472490` routes semantically to force-feedback, and `0x472490..0x472670` is a shared DirectInput diagnostic helper needing owner-shape scrutiny before standalone/S use. |
| `[0x4a3930,0x4a3ea0)` | `src/GameZRecoil/zSound/zsnd_fade.cpp` | Bracketed/order-backed zSnd fade-list runtime; no local source-path literal and not part of the report-helper island. Refined subranges: lifecycle/global setup `[0x4a3930,0x4a3a80)`, dispatch/update `[0x4a3a80,0x4a3c20)`, active-list tick `[0x4a3c20,0x4a3d20)`, and cleanup/list cursor helpers `[0x4a3d20,0x4a3ea0)`. |
| `[0x4a3ea0,0x4a44c0)` | `src/GameZRecoil/zSound/zsnd_error.cpp` | Provisional invented no-literal report-helper shelf. Focused BN evidence shows `zSnd::ReportMciError`, `zSnd::ReportA3DError` plus jump table `[0x4a4248,0x4a432c)`, padding `[0x4a432c,0x4a4330)`, and `zSnd::ReportDirectSoundError`; no `zsnd_error.cpp` or `zsnd_report.cpp` source-path literal exists. Keep this as a practical physical placement label only until VC5 order tests prove a stronger original source/header shape. |
| `[0x4a53f0,0x4a5670)` | `src/GameZRecoil/zSound/zsnd.cpp` | Bracketed/order-backed zSndWaveData class island after `zsnd_grp.cpp`; no local source-path literal. Lowercase `zsnd.cpp` is a stronger provisional core-sound candidate than the invented `zsnd_wave.cpp`, but it is still not a source-path gate. Focused BN notes also found conflicting comments naming `zSound.cpp`, so neither spelling is accepted as original yet. Refined subranges: lifecycle `[0x4a53f0,0x4a5460)`, WAVE parsing `[0x4a5460,0x4a5540)`, and lazy load/reset/archive wrappers `[0x4a5540,0x4a5670)`. |
| `[0x4a5670,0x4a59d0)` | `src/GameZRecoil/zSys/zsys.cpp` | Conservative provisional no-literal mixed zSys shelf before the provider thunk. Semantic Time, RecoilApp, zSys, and zVideo subranges remain recorded in the source-block database; focused BN found no `zsys.cpp`, `zsys.h`, `Time.cpp`, `zLoc.*`, or `zVideo.cpp` literal, and `zsys.h` has no emitted address-row proof. Reconstruction agents must determine any stronger `.h`/`.cpp` split by VC5 order tests, update the block catalog if evidence proves it, and then reshape `src` to match the corrected model. |
| `[0x4a59d0,0x4a59e0)` | provider `KERNEL32_GetTickCount_ImportThunk` | Provider thunk between the two provisional zsys.cpp physical rows. |
| `[0x4a59e0,0x4a5c20)` | `src/GameZRecoil/zSys/zsys.cpp` | Conservative provisional no-literal zSys shelf continuation after the provider thunk. Semantic zSys drive-search and zLoc message/DLL helpers remain recorded as subranges; focused BN found no `zsys.cpp`, `zsys.h`, `zLoc.c`, or `zLoc.cpp` literal, and no separate drive/message physical split is accepted. Reconstruction agents must determine any stronger `.h`/`.cpp` split by VC5 order tests, update the block catalog if evidence proves it, and then reshape `src` to match the corrected model. |
| `[0x4a5c20,0x4a66e0)` | `src/GameZRecoil/zUtil/zutl_zar.cpp` | Literal-backed zUtil/ZAR block. The prefix includes zReader/ZRDR semantic helpers before the `zIndexArchive` core, but the physical source-path literal is `zutl_zar.cpp`. |
| `[0x4a66e0,0x4a69c0)` | `src/GameZRecoil/zVideo/zvid_main.c` | Provisional invented zVideo pre-buffer dispatch/window prelude before `zvid_buff.c`. Focused BN sweep found no `zvid_main.c`, `zVideo.cpp`, `zvid.h`, or `zvid_dd.c` source-path literal; the first `zvid_buff.c` helper starts at `0x4a69c0`, and the `zvid_buff.c` literal is not referenced until `0x4a6b26`, so do not merge the prelude into `zvid_buff.c`. Function-granular semantic routing is recorded in the block catalog. |
| `[0x4b2960,0x4b33f0)` | `src/GameZRecoil/zGame/zgame_opt.c` | Special no-literal late-shelf row: zGame options registry/runtime block with zSound/zSys/zVid helper exceptions. Provisional physical placement label only; `0x4b3380` also carries a conflicting BN comment naming `Battlesport/zgame_options.cpp`, so do not infer exact original filename or global alphabetical project order from this row. |
| `[0x4b33f0,0x4b3ce0)` | `src/GameZRecoil/zSys/zsys_cpu.cpp` | Special no-literal late-shelf row: CPU feature detection and benchmark resolver. This is the strongest practical no-literal physical label in the late shelf, but it still has no source-path literal and remains unresolved provenance. |
| `[0x4b3ce0,0x4bffe0)` | `src/GameZRecoil/zUI/zui.cpp` | Special no-literal late-shelf row: large HUD/UI physical shelf, including zTimedTask/HudLineClip/zVideoFxPass3/HudWeatherFx exceptions. `zUI/zui.cpp` is the current provisional physical placement label for the dominant widget/control/dialog/background subranges; finer `zUI/zui_*` splits and `zWeather/zweather_fx.cpp` remain semantic filename hypotheses, not accepted physical paths. |
| `[0x4bffe0,0x4c0d20)` | `src/GameZRecoil/zUtil/zutl_zbd.cpp` | Special no-literal late-shelf row: recursive ZBD/ZAR archive cluster; preserve CRT `tmpfile` provider exclusion. Provisional physical placement label only; filename follows the literal-backed zUtil `zutl_*` pattern proven by `zutl_zar.cpp`. |
| `[0x4c7f00,0x4c7fd0)` | `src/GameZRecoil/zImage/zimg_fonts.h` | Candidate address-emitting partial header contribution, included through `src/WinMain.cpp`; records `zImage_Font::BlitStringToActiveTarget` as a plausible selected header/COMDAT body rather than a split `zimg_fonts.cpp` block. BN found no `zimg_fonts.h` or local `.cpp` source-path literal for this late placement. |
| `[0x4c7fd0,0x4c81c0)` | `src/GameZRecoil/zVideo/zvid.h` | Candidate address-emitting partial header contribution, included through `src/WinMain.cpp`; records the zVideo palette/brightness helpers as plausible selected header/COMDAT bodies rather than a standalone late `zVideo.cpp` block. BN found no `zvid.h`, `zVideo.cpp`, or `WinMain.cpp` literal proving the host. |
| `[0x4c81c0,0x4c81d8)` | `src/WinMain.cpp` | Final authored WinMain wrapper before the MFC provider tail; root-level placement fits recovered compile-order reasoning. |

Current block evidence:

- The physical `ai_net.cpp` contribution starts at `0x401060` and runs through
  `AINet::FreeAll` at `0x403870`.
- The following `Briefing.cpp` contribution starts at `0x4038a0` and contains
  the later confirmed `Briefing::StartForMission` file-literal call at
  `0x404242`.
- `zMath::Vec3Normalize` at `0x402f60` is a semantic `zMath.h` inline helper;
  its COMDAT is physically emitted inside the probable `ai_net.cpp` block
  between `AINet::AiFinalizeMode2State1ForAllPlayers` at `0x402f10` and
  `AINet::LoadAllFromZrd` at `0x402fd0`. Such header/helper placement is an
  exception to source-owner naming, not a reason to discard the block model.
- Known order-relevant declaration-only/type-only header contributors remain
  recorded as `source_shape_inputs` on the owning physical block. Header
  contributors with known emitted address ranges are recorded as
  address-ordered `partial-header` rows in `mapped_blocks`; they participate in
  continuous coverage, but remain source-shape placement evidence rather than
  owner-gate/tier evidence.

## Current Repair State

The first durable repair is conservative:

- `0x403750` was split out of the later Player bootstrap owner into
  `battlesport_ai.ainet_peer_ring_build`; its source body now lives in
  `src/Battlesport/ai_net.cpp`, but owner-boundary scrutiny and byte drift still
  block acceptance.
- Early Player AI-mode owners rooted at `0x401060`, `0x402080`, `0x402250`,
  and `0x402be0` no longer have accepted boundary/source/linkage gates or tier
  `B`; their functional evidence remains useful as tier `C` evidence only.
- `battlesport_gameplay.player_ai_mode2_tuning_globals` keeps its data-byte
  evidence, but source/linkage ownership is blocked until the data packet is
  remapped or re-proven against the corrected owner.
- `legacy.battlesport_gameplay.subsystem_ai` metadata and source body for
  `0x403830` now route through `src/Battlesport/ai_net.cpp`; the broader owner
  gates remain blocked until the adjacent early AI/player placement audit is
  finished and positive gates are re-scrutinized.
- Latest VC5 `ainet_text_block_order` evidence after the 2026-07-01 Option A
  header-layering experiment shows the helper emits from the `ai_net.cpp`
  object at the intended retail slot: `AINet::AiFinalizeMode2State1ForAllPlayers`
  is `SECT39`, `zMath::Vec3Normalize` is `SECT3B`, and
  `AINet::LoadAllFromZrd` is `SECT3D`; `section_order_matches_manifest` is
  `True`. Source shape stays `zMath.h`: early headers now include
  `zMathTypes.h`/`zMathDecls.h`, while `ai_net.cpp` includes full `zMath.h`
  after `0x402f10`. The full block still fails byte verification; `0x402f60`
  has 95 mismatches after 4 relocation bytes and 5 trailing VC5 NOPs, BN size
  98 and VC5 size 112.
- This `0x402f60` repair is the model for header/COMDAT placement: keep the
  semantic helper in `src/GameZRecoil/zMath/zMath.h`, expose only early
  declarations/types through `zMathTypes.h`/`zMathDecls.h`, and include the full
  header at the retail emission point so VC5 naturally emits the retail order.
  Do not move semantic helpers into the wrong `.cpp` solely to force placement.
- The active source-block catalog now records address-emitting `ai_net.cpp`
  header contributors explicitly in the flattened block list:
  `src/Battlesport/ai_net.h` emits `[0x401060,0x402f60)`, included by
  `src/Battlesport/ai_net.cpp`; `src/GameZRecoil/zMath/zMath.h` emits
  `[0x402f60,0x402fd0)`, including `0x402f60`; and the `ai_net.cpp` body row
  resumes at `[0x402fd0,0x4038a0)`. Early `zMathTypes.h` and `zMathDecls.h`
  remain `source_shape_inputs` because they are declaration/type inputs, not
  known address-emitting rows.
- New `.inl` production reconstruction files are banned. Existing `.inl` files
  are legacy/provisional source-shape debt unless independently proven
  original: `src/GameZRecoil/zSys/zSys_probe_platform.inl`,
  `src/GameZRecoil/zSys/zSys_cpu_asm.inl`,
  `src/GameZRecoil/zSys/zSys_cpu_detect.inl`, and
  `src/GameZRecoil/zSys/zSys_cpu_get_class.inl`.

Current top-down Battlesport source-path literal queue:

- `ai_net.cpp`: literal `0x4da1e8`, referenced at `0x4030bb`; proven block
  `[0x401060,0x4038a0)`.
- `Briefing.cpp`: literal `0x4da32c`, referenced at `0x404238`; mapped block
  `[0x4038a0,0x404ca0)`, with HUD/zZbd/zInput semantic exceptions still
  requiring owner scrutiny before acceptance.
- `hud.cpp`: literal `0x4dadd8`, referenced at `0x4101a3` and `0x4141bb`;
  mapped block `[0x404ca0,0x415ab0)`, with semantic-owner exceptions.
- `map.cpp`: literal `0x4daf04`, referenced at `0x416922`; mapped block
  `[0x415ab0,0x417350)`.
- `mission.cpp`: literal `0x4db230`, referenced at `0x417fc2`, `0x4181b6`,
  `0x418209`, `0x4182ff`, `0x418395`, `0x419091`, and `0x419304`; mapped
  block `[0x417350,0x41cc10)`.
- `pickup.cpp`: literal `0x4dc190`, referenced at `0x41cd93`, `0x41d523`,
  and `0x41db80`.
- `player.cpp`: literal `0x4dc26c`, first referenced at `0x41f20b`; this is a
  later block boundary and must not justify `player.cpp` ownership for the
  earlier `ai_net.cpp` physical block.
- `RecoilApp.cpp`: literal `0x4dcb9c`, referenced at `0x42e620`.
- `turret.cpp`: literal `0x4dd19c`, referenced at `0x437b25`.

## `Class.c` zClass Node Allocation/Delete Local Block

BN source-path comments and xrefs to `0x4dd9e8`
`"D:\Proj\GameZRecoil\zClass\Class.c"` prove the local Class.c physical order:

- `0x4478c0` `zClass_Class::AllocNodeFromFreeList`
- `0x447980` `zClass_Class::DeleteNodeByType`
- `0x447a70` `zClass_Class::FreeNodeToFreeList`
- `0x447b60` `zClass_Class::TryFreeNode`
- `0x447bc0` `zClass_Class::FindNodeRecursiveByName`

`0x447a40..0x447a6f` is the compiler-emitted switch jump table inside
`DeleteNodeByType`, not a separate authored data owner. The semantic owner
boundary is still split: `DeleteNodeByType` belongs to
`engine.zclass.remove_child_delete_dispatch`, while `AllocNodeFromFreeList`,
`FreeNodeToFreeList`, and `TryFreeNode` belong to
`engine.zclass.node_free_and_deferred_work`. Physical adjacency across those
owners is expected and does not justify an owner remap.

Do not use the physical block order as a blanket semantic-owner rule. The early
range includes exceptions such as provider/MFC thunks, `zMath` helpers,
Briefing/HUD classes, and other source-shaped owners. Use the block order as a
source-owner blocker and audit signal, then confirm each function or owner with
current BN xrefs, callees, data, source comments, and source model evidence
before promoting gates.

## Current Top-Down Recoil.exe Physical Block Map

The following map consolidates the July 2026 BN source-path literal pass. Ranges
are half-open function-start ranges inferred from current `Recoil.bndb`
function order and literal xrefs. They are block-order evidence, not accepted
source-owner gates or tier evidence by themselves. Semantic owner names inside a
physical block can still be original header, COMDAT, provider, or included
helper emissions, but those exceptions must be proven before moving the
function into another source-file owner.

Durable machine-readable scheduling metadata lives in
`tools/_recoil/config/source_file_blocks.json`. The BN comment pass was assigned
to write comment anchors in `Recoil.bndb`; comments are navigation evidence only
and do not update `SOURCE_OWNERS`.

### Battlesport Blocks

| Source path | Physical range | Literal evidence | First / last BN function | Status |
| --- | --- | --- | --- | --- |
| `src/Battlesport/Recoil.cpp` | `[0x401000,0x401020)` | no literal; BN source comment `D:\Proj\Battlesport\Recoil.cpp` | `CAboutDlg::Constructor` / `CAboutDlg::Constructor` | mapped no-literal prelude |
| MFC provider prelude | `[0x401020,0x401060)` | no literal; provider wrappers | `MFC::NoOpVirtualOneArg` / `CWnd::EnableWindow` | provider boundary |
| `src/Battlesport/ai_net.cpp` | `[0x401060,0x4038a0)` | `0x4da1e8`, xref `0x4030bb` | `AINet::TickAiMode2TopLevel` / `AINet::FreeAll` | mapped with `ai_net.h`/`zMath.h` partial-header rows and 6 body subranges |
| `src/Battlesport/Briefing.cpp` | `[0x4038a0,0x404ca0)` | `0x4da32c`, xref `0x404238` | `HudUiBriefingObjectivePicture::DrawWithNoiseOverlay` / `Briefing::BuildObjectiveActionsForRuntime` | mapped, 11 semantic subranges |
| `src/Battlesport/hud.cpp` | `[0x404ca0,0x415ab0)` | `0x4dadd8`, xrefs `0x4101a3`, `0x4141bb` | `HudUiElement::Draw` / `zFMV_ActionBase::Destructor` | mapped refined |
| `src/Battlesport/map.cpp` | `[0x415ab0,0x417350)` | `0x4daf04`, xref `0x416922` | `HudSensorMapNode::Init` / `HudSensorTracker::SetObjectiveMarkerColorBlink` | mapped refined, 8 semantic subranges |
| `src/Battlesport/mission.cpp` | `[0x417350,0x41cc10)` | `0x4db230`, xrefs `0x417fc2`, `0x4181b6`, `0x418209`, `0x4182ff`, `0x418395`, `0x419091`, `0x419304` | `Mission::InitObjectives` / `CSpinButtonCtrl::ScalarDeletingDestructor` | mapped refined, 14 semantic subranges |
| `src/Battlesport/pickup.cpp` | `[0x41cc10,0x41ea90)` | `0x4dc190`, xrefs `0x41cd93`, `0x41d523`, `0x41db80` | `PickupSpawnList::Primary_Init` / `Pickup::SpawnAtCarrierNodeByName` | mapped |
| `src/Battlesport/player.cpp` | `[0x41ea90,0x42de10)` | `0x4dc26c`, xrefs `0x41f20b`, `0x41f870`, `0x42087a`, `0x420dc7`, `0x42155b`, `0x421722`, `0x42176f` | `Player::InitMasterCommonDataList` / `CRT::SafeVtableRelease` | mapped physical, tail scrutiny |
| `src/Battlesport/RecoilApp.cpp` | `[0x42de10,0x436630)` | `0x4dcb9c`, xref `0x42e620` | `RecoilApp::GetMessageMap` / `PartitionEntriesByPivot` | mapped physical, semantic scrutiny |
| `src/Battlesport/turret.cpp` | `[0x436630,0x437e60)` | `0x4dd19c`, xref `0x437b25` | `zTurret_Runtime::InitDefaults` / `zTurret_Runtime::FireWeaponCallback` | mapped, 7 semantic subranges |

The earlier HUD/map/mission overlaps are resolved by later BN source-block
mapping: `hud.cpp` ends at `0x415ab0`, `map.cpp` covers
`[0x415ab0,0x417350)`, and `mission.cpp` starts at `0x417350`. This is
non-overlap block-order evidence, not owner-gate acceptance; semantic
exceptions in these blocks still require owner scrutiny before byte work.

### GameZRecoil Blocks Through zClass/zFMV

| Source path | Physical range | Literal | First / last BN function | Status |
| --- | --- | --- | --- | --- |
| `src/GameZRecoil/zClass/cls_di.c` | `[0x443c50,0x4478c0)` | `0x4dd94c` | `zClass_cls_di::SetBreakOnFirstCandidate` / `zClass_cls_di::FrustumTestAndPick` | mapped |
| `src/GameZRecoil/zClass/Class.c` | `[0x4478c0,0x449ba0)` | `0x4dd9e8` | `zClass_Class::AllocNodeFromFreeList` / `zClass_Class::SetSingleParentFlagRecursive` | mapped |
| `src/GameZRecoil/zClass/Camera.c` | `[0x449ba0,0x44d990)` | `0x4ddd44` | `zClass_Camera::SetViewDistance` / `zVideo_sw::RenderFrame` | mapped refined, semantic exceptions |
| `src/GameZRecoil/zClass/Object3d.c` | `[0x44d990,0x44e630)` | `0x4ddeac` | `zClass_Node::PropagateTransformDirtyRecursive` / `zClass_Object3D::gwObject3DGetMatrixPtr` | mapped |
| `src/GameZRecoil/zClass/List.c` | `[0x44e630,0x44f7a0)` | `0x4ddfd0` | `zClass_TypeList::AllocLink` / `zClass_Class::gwNodeFindNextByName_Predicate` | mapped |
| `src/GameZRecoil/zClass/Window.c` | `[0x44f7a0,0x44fdd0)` | `0x4de110` | `zClass_Window::gwWindowNew` / `zClass_Window::gwWindowCloseClearPolygon` | mapped |
| `src/GameZRecoil/zClass/Display.c` | `[0x44fdd0,0x450030)` | `0x4de214` | `zClass_Display::gwDisplayInit` / `zClass_Display::gwDisplaySetBackgroundColor` | mapped |
| `src/GameZRecoil/zClass/cls_world.c` | `[0x450030,0x4518b0)` | `0x4de298` | `zClass_World::QueueAreaUpdate` / `zClass_World::ReadSettingsSection` | mapped |
| `src/GameZRecoil/zClass/cls_util.c` | `[0x4518b0,0x452920)` | `0x4de4d0` | `zClass::SetNodeArraySize` / `zClass_Node::AssignInt32ToDiRecursive` | mapped, tail conflict |
| `src/GameZRecoil/zClass/Switch.c` | `[0x452920,0x4529c0)` | `0x4dec88` | `zClass_Class::AddChildValidated` / `zClass_Class::RemoveChildValidated` | mapped physical, semantic conflict |
| `src/GameZRecoil/zClass/Sound.c` | `[0x4529c0,0x452fd0)` | `0x4decac` | `zClass_Sound::gwSoundNew` / `zClass_Sound::ComputeWorldTransform` | mapped |
| `src/GameZRecoil/zClass/Light.c` | `[0x452fd0,0x453b10)` | `0x4ded18` | `zClass_Light::gwLightNew` / `zClass_Light::gwLightSetSpecularColor` | mapped |
| `src/GameZRecoil/zClass/Animate.c` | `[0x453b10,0x453ee0)` | `0x4dedd0` | `zClass_Animate::DeleteNode` / `zClass_Animate::SampleTransform` | mapped |
| `src/GameZRecoil/zClass/Seq.c` | `[0x453ee0,0x454360)` | `0x4dedf8` | `zClass_Sequence::gwSequenceNew` / `zClass_Lod::SetTargetNodeAndRange` | mapped, Lod tail scrutiny |
| `src/GameZRecoil/zClass/cls_zbd.c` | `[0x454360,0x4558f0)` | `0x4dee1c` | `zClass::ResetCurrentZbdPath` / `GameZ_ZBD::ReloadDisplayInstancesRecursive` | mapped with no-literal ZBD-path reset prefix |
| `src/GameZRecoil/zDEClient/zdec_init.cpp` | `[0x4558f0,0x455ea0)` | `0x4df48c` | `zDEClient::LoadConfigResources` / `zDEClient::ShutdownGlobals` | mapped |
| `src/GameZRecoil/zDEClient/zdec_qsand.cpp` | `[0x455ea0,0x456ad0)` | `0x4df540` | `zDEClient_QSand::DestroyFeature` / `zDEClient_QSand::CreateFeature` | mapped |
| `src/GameZRecoil/zDEClient/zdec_crater.cpp` | `[0x456ad0,0x458af0)` | `0x4df604` | `zDEClient_Crater::DestroyFeature` / `zDEClient::GetCameraNode` | mapped with semantic exceptions |
| `src/GameZRecoil/zEffect/zeff_anim_run.c` | `[0x458af0,0x45e100)` | `0x4df674` | `zEffect::SetConditionalRefPos` / `zEffect::SetConditionalEffectLevel` | mapped |
| `src/GameZRecoil/zEffect/zeff_anim_init.c` | `[0x45e100,0x460020)` | `0x4df75c` | `zEffect_Anim::Init` / `zEffectAnim::GetRootNodeOrNull` | mapped |
| `src/GameZRecoil/zEffect/zeff_init.c` | `[0x460020,0x4603d0)` | `0x4df98c` | `zEffect::Init` / `zEffect::Reset` | mapped |
| `src/GameZRecoil/zEffect/zeff_anim_save.c` | `[0x4603d0,0x4622f0)` | `0x4dfa58` | `zEffect_Anim::ClearActivationRecords` / `zEffect::FindTemplateIndexByName` | mapped |
| `src/GameZRecoil/zError/zerr_old.c` | `[0x4622f0,0x462330)` | `0x4dfaf4` | `zError::EmitDebugBuffer` / `RecoilError::InitOutputContext` | mapped |
| `src/GameZRecoil/zFMV/fmv_main.cpp` | `[0x462330,0x4625e0)` | `0x4dfb28` | `zFMV_Playback::Constructor` / `zFMV_Playback::ReportMciError` | mapped |
| `src/GameZRecoil/zFMV/fmv_script.cpp` | `[0x4625e0,0x463d50)` | `0x4dfc1c` | `zFMV_Script::Init` / `zFMV_ActionPlayMci::End` | mapped |
| `src/GameZRecoil/zFMV/fmv_stream.cpp` | `[0x463d50,0x464670)` | `0x4dfc74` | `zFMV_Stream::Init` / `zFMV_Stream::FillAudioBuffer` | mapped |

### GameZRecoil Blocks From zGeometry To .text Tail

| Source path | Physical range | Literal evidence | First / last BN function | Status |
| --- | --- | --- | --- | --- |
| `src/GameZRecoil/zGeometry/zgeo_weiler.cpp` | `[0x464670,0x46a690)` | `0x4dff3c`, dense xrefs through `0x46a1f0` | `zGeometry_Weiler::GetInputContourAPointList` / `zGeometry_Bounds2D::OverlapsWithUnitMargin` | mapped, semantic conflicts |
| `src/GameZRecoil/zGeometry/zgeo_model.cpp` | `[0x46a690,0x46bd50)` | `0x4e034c`, xrefs through `0x46bb90` | `zGeometry_Model::FindOrCreateRandomDebugMaterial` / `zGeometry_Model::IsFullyInsideClipPolygonXY` | mapped, semantic conflicts |
| `src/GameZRecoil/zGeometry/zgeo_convexify.cpp` | `[0x46bd50,0x46d310)` | `0x4e0538`, xref in `zGeometry_Polygon::Convexify` | `zGeometry_TriangulateHole::TryAppendBridgeEdge` / `zGeometry_Polygon::TrySplitPointDwordOffsetsAtBestDiagonal` | mapped |
| `src/GameZRecoil/zImage/zimg_texture.cpp` | `[0x46d310,0x46efc0)` | `0x4e0740`, xrefs in texture directory read/write | `zImage::TexDirEntryToIndex` / `zVid_Image::ReadFromFile` | mapped physical, semantic conflicts |
| `src/GameZRecoil/zImage/zimg_fonts.cpp` | `[0x46efc0,0x46f300)` | `0x4e08a0`, xref in `zImage::FontsLoadFromPath` | `zImage_Font::GetByIndexOrDefault` / `zImage_Font::MeasureString` | mapped |
| `src/GameZRecoil/zInput/zin_kbd.cpp` | `[0x46f300,0x470020)` | `0x4e08cc`, keyboard xrefs | `zInput::Keyboard_InitDevice` / `zInput::Keyboard_InitDikToAsciiTable` | mapped |
| `src/GameZRecoil/zInput/zin_init.cpp` | `[0x4719e0,0x471e40)` | `0x4e0c9c`, xref in `zInput::Init` | `zInput::GlobalStateStaticInitAndRegisterAtExit` / `zInput::PollActiveDevices` | mapped refined, semantic subsystem crossings |
| `src/GameZRecoil/zMath/zmth_main.c` | `[0x472670,0x475c40)` | `0x4e0ed4`, xref in `zMath::CrtMatherrHandler` | `zMath::Vec3DeltaLengthSq` / `zMath::QuatFromRotationVector` | mapped physical with 10 semantic math subranges |
| `src/GameZRecoil/zModel/gmod_init.c` | `[0x475c40,0x4805b0)` | `0x4e0f28`, xrefs in display-instance setup | `zModel_Display::Init` / `zReader::FindGlobalStringPrefixIndex` | mapped, semantic conflicts |
| `src/GameZRecoil/zModel/gmod_matl.c` | `[0x4805b0,0x481530)` | `0x4e11a0`, material stream xrefs | `zModel_MatlSlot::IndexFromPtrOrMinus1` / `zRndr_GlobalStringTable::LoadDynamicEntriesFromPath` | mapped, semantic conflicts |
| `src/GameZRecoil/zModel/gmod_const.c` | `[0x481530,0x487a30)` | `0x4e13a0`, DI/model-const xrefs | `zModel_Const::GetVertexMergeEpsilon` / `zClass_cls_di::FilterRegionsAgainstHexahedronFaces` | mapped, semantic conflicts |
| `src/GameZRecoil/zModel/gmod_light.c` | `[0x487a30,0x489d00)` | `0x4e17f8`, light xrefs | `zModel_Light::BuildActiveLightList` / `zModel_Light::BuildAttr1Falloff` | mapped |
| `src/GameZRecoil/zNetwork/znet_dplay.cpp` | `[0x489d00,0x48c7d0)` | `0x4e1860`, DirectPlay xrefs | `zNetwork::InitSessionRuntime` / `zNetwork_DPlay::ReportError` | mapped |
| `src/GameZRecoil/zReader/zreader.cpp` | `[0x48c7d0,0x48d340)` | `0x4e2118`, xref in `zReader::ReadNode` | `zUtil::ZRDR_PreallocNodePool` / `zUtil_ZRDR::UnloadMountedArchives` | mapped |
| `src/GameZRecoil/zRender/zrndr_draw.c` | `[0x48d340,0x49f614)` | `0x4e2168`, late draw xrefs | `zVid::Noise_InitBuffers` / `zRndr::SpanShade16FromPal8SwitchVShift` | mapped, semantic conflicts |
| `src/GameZRecoil/zSound/zsnd_play.cpp` | `[0x49f614,0x4a10e0)` | effective path start `0x4e2208`; string base `0x4e2206` has no xrefs | `zSnd::TickWrapper` / `zSnd::SetFlag10PlaybackEnabled` | mapped physical with semantic slices; `0x49f614..0x49f620` is padding |
| `src/GameZRecoil/zSound/zsnd_parm.cpp` | `[0x4a10e0,0x4a12c0)` | `0x4e225c`, xref `0x4a11a9` | `zSndPlayHandle::SetFreqScaled` / `zSnd::GetActiveBackend` | mapped with 3 semantic parameter/accessor subranges |
| `src/GameZRecoil/zSound/zsnd_init.cpp` | `[0x4a12c0,0x4a2010)` | `0x4e2290`, init backend xrefs | `zSnd::PreInitializeRuntimeState` / `zSndBackend::Shutdown` | mapped |
| `src/GameZRecoil/zSound/zsnd_cd.cpp` | `[0x4a2010,0x4a2950)` | `0x4e23f0`, CD xrefs | `zSndCdTrackList::StaticInit` / `zSndCd::GetTrackCount` | mapped |
| `src/GameZRecoil/zSound/zsnd_3d.cpp` | `[0x4a2950,0x4a2ea0)` | effective path `0x4e2428`, xrefs `0x4a2e20` and `0x4a2e48` | `zSnd::UpdateListenerState` / `zSnd::SetSpeedOfSoundMps` | mapped-effective-literal; base string starts at `0x4e2426` with two prefix bytes |
| `src/GameZRecoil/zSound/zsnd_create.cpp` | `[0x4a2ea0,0x4a3930)` | `0x4e2450`, wave/back-end xrefs | `zSndSample::InitFromWaveData` / `zSndSample::Destroy` | mapped |
| `src/GameZRecoil/zSound/zsnd_grp.cpp` | `[0x4a44c0,0x4a53f0)` | `0x4e2df8`, group config xrefs | `zSndPendingList::FindByName` / `zSndGroup::QueueStreamRequestWithWorldPos` | mapped |
| `src/GameZRecoil/zSys/zsys.cpp` | `[0x4a5670,0x4a59d0)` | none | `Time::Reset` / `zVid::QueryCachedClientRectUpdateMaskIf3dfx` | provisional no-literal mixed zSys shelf; 4 semantic subranges |
| provider `KERNEL32_GetTickCount_ImportThunk` | `[0x4a59d0,0x4a59e0)` | import thunk | `KERNEL32_GetTickCount_ImportThunk` / same | provider boundary between provisional zsys.cpp rows |
| `src/GameZRecoil/zSys/zsys.cpp` | `[0x4a59e0,0x4a5c20)` | none | `zSys::FindFileOnDriveType` / `zLoc::GetMessageString` | provisional no-literal mixed zSys shelf continuation; 3 semantic subranges |
| `src/GameZRecoil/zUtil/zutl_zar.cpp` | `[0x4a5c20,0x4a66e0)` | `0x4e3010`, xref in `zIndexArchive::Init` | `zReader::FileExists` / `zIndexArchive::ReadFileByName` | mapped core with semantic zReader/ZRDR prefix |
| `src/GameZRecoil/zVideo/zvid_buff.c` | `[0x4a69c0,0x4a6b40)` | `0x4e3054`, xref `0x4a6b26` | `zVideo_buff::ClipCoordToRange` / `zVideo_buff::BltSourceToPrimaryClipped` | mapped after prelude gap with 2 semantic subranges |
| `src/GameZRecoil/zVideo/zvid_init.c` | `[0x4a6b40,0x4a7b40)` | `0x4e30a0`, xrefs in `zVideo::InitVideoSystem` | `zVideo::SetRendererTypeAndActivePath` / `zVideo::GetClearScreenBufferEnabled` | mapped, semantic exceptions at `0x4a6b60..0x4a6b80` |
| `src/GameZRecoil/zVideo/zvid_dd.c` | `[0x4a7b40,0x4a9ac0)` | `0x4e30e8`, 58 xrefs | `zVideo_dd::StartupEnumerateAndDefaultSelect` / `zVid::QueryTextureMemoryBytes` | mapped |
| `src/GameZRecoil/zVideo/zvid_ddd3d.c` | `[0x4a9ac0,0x4ae380)` | `0x4e3374`, 35 xrefs | `zVideo_dd3d::BeginSceneAndFlushPendingRenderStates` / `zVideo_dd::ReportError` | mapped core plus proven `zvid_dd.c` diagnostic tail |
| `src/GameZRecoil/zWeapon/zwep_init.c` | `[0x4ae380,0x4b2960)` | `0x4e45d8`, xrefs in `zWeapon::LoadOptCatalogFromPath` | `OptCatalog::BlendDirectionTowardTarget` / `OptCatalog_MineIterator::Next` | mapped broad physical block, semantically multi-owner |
| `src/GameZRecoil/zInterp/zinterp_parse.cpp` | `[0x4c0d20,0x4c5a50)` | `0x4e5654`, xrefs in `zInterp_Context::DispatchCoreCommand` | `zInterp_Context::Constructor` / `zInterp_Object3D::ScrollAlwaysTickAction` | mapped |

### Mapped No-Literal And Provider Sub-Blocks

These ranges have no local `zError::ReportOldNoOp` source-path literal. They
are still physical `.text` order facts from current BN function order and
neighboring literal-backed blocks. Treat source-path names here as provenance
hypotheses unless a later packet promotes the evidence.

| Physical range | Current classification | First / last BN function | Notes |
| --- | --- | --- | --- |
| `[0x437e60,0x438980)` | `src/Battlesport/util.cpp` provisional physical block | `zClass_Node::SetContextRecursive` / `HudUiMgrSensor::TrackList_Add` | Mixed utility shelf with 7 semantic subranges. No functions in this block have callers after `0x43ce80`; semantic ownership is retained in `tools/_recoil/config/source_file_blocks.json`. |
| `[0x438980,0x438990)` | `src/Battlesport/version.cpp` provisional physical block | `RecoilVersion::GetString` / same | App-shell version accessor between `util.cpp` and `weapon.cpp`. |
| `[0x438990,0x43ce80)` | `src/Battlesport/weapon.cpp` provisional physical block | `PickupAirdropSpawnRef::InitNodesFromCarrierNodeName` / `Player::Mines_ZAR_ReadEntryOrReset` | Weapon/combat shelf with 11 semantic subranges. No functions in this block have callers after `0x43ce80`; semantic ownership is retained in `tools/_recoil/config/source_file_blocks.json`. |
| `[0x43ce80,0x43cf90)` | `src/Battlesport/WinSock.cpp` provisional physical block | `NetUi::VerifyWinsock2OrPromptContinue` / `Net::FormatIpv4Address` | Six recorded subranges: NetUi Winsock prompt helper, padding, `0x43cf20` zStr CRT init-table stub, padding, Net IPv4 formatter, and padding before WOL. No WinSock/NetUi/Net source-path literal was found. |
| `[0x43cf90,0x43d130)` | WOL dialog head | `WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls` / `WestwoodOnlineUpgradeDialog::AppendStatusTextFmt` | Merge with discontiguous WOL dialog members for owner work. |
| `[0x43d130,0x43d650)` | WOL API | `WestwoodOnlineUpgradeApi::CreateInstanceAndLoadConfig` / `WestwoodOnlineUpgradeApi::Init` | Owner gates/tier state need parent scrutiny before acceptance claims. |
| `[0x43d650,0x43f610)` | WOL dialog continuation | `WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList` / `WestwoodOnlineUpgrade::TruncateStringAtFirstSpace` | Contains progress-dialog semantic exception at `0x43f440`. |
| `[0x43f610,0x441600)` | WOL API event sink | `WestwoodOnlineUpgradeApiEventSink::CreateInstance` / `WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult` | Contains MSVC EH jump artifacts at `0x43f682`/`0x43f688`. |
| `[0x441600,0x441620)` | WOL shared refcount helper | `WestwoodOnlineUpgradeRefCountAndLock::Init` / same | Shared WOL helper, not API-event-sink-only. |
| `[0x441620,0x4416f0)` | WOL API event-sink tail | `WestwoodOnlineUpgradeApiEventSink::Release` / `WestwoodOnlineUpgradeApiEventSink::Destructor` | Physically after shared helper; keep with event-sink owner. |
| `[0x4416f0,0x441750)` | WOL dialog profile getters | `WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName` / `WestwoodOnlineUpgradeDialog::GetSelectedProfileConnectString` | Discontiguous WOL dialog tail. |
| `[0x441750,0x442180)` | WOL config dialog | `WestwoodOnlineUpgradeConfigDialog::Constructor` / `WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus` | Contains `0x441a00` MFC provider thunk and class-coupled scalar deleting destructor. |
| `[0x442180,0x442220)` | WOL dialog profile setters | `WestwoodOnlineUpgradeDialog::SetSelectedProfilePlayerName` / `WestwoodOnlineUpgradeDialog::SetSelectedProfileConnectString` | Discontiguous dialog setters. |
| `[0x442220,0x4422a0)` | WOL progress dialog head | `WestwoodOnlineUpgradeProgressDialog::Constructor` / `WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt` | Class-coupled scalar deleting destructor present. |
| `[0x4422a0,0x442320)` | WOL download wrappers | `WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise` / `WestwoodOnlineUpgradeDownload::UnadviseAndRelease` | Authored COM integration wrappers, not provider. |
| `[0x442320,0x4425c0)` | WOL progress/dialog mixed | `WestwoodOnlineUpgradeProgressDialog::DlgProc` / `WestwoodOnlineUpgradeDialog::ShowDownloadReadyList` | Mixed progress dialog plus WOL dialog owner; `ShowDownloadReadyList` routes to dialog owner. |
| `[0x4425c0,0x442890)` | WOL download event sink | `WestwoodOnlineUpgradeDownloadEventSink::CreateInstance` / `MsvcEh::FuncletCleanup_4427F0` | `0x442632`, `0x442638`, and `0x442860` are compiler EH artifacts. |
| `[0x442890,0x4428b0)` | MFC provider getters | `MFC42::GetOrdinal4274Getter` / `COleControlModule::GetBaseMessageMap` | Provider boundary split into ordinal-import-pointer and COleControlModule message-map getter subranges. |
| `[0x4428b0,0x443730)` | AppFrame/RecoilApp late | `RecoilApp_MfcOleModule::Destructor` / `RecoilApp_StateQueueBlock::InitFromCursor` | Six recorded semantic subranges cover app-shell destruction, main-window/engine startup, module construction, Run/EH funclets, state-transition queue API, and queue storage helpers. |
| `[0x443730,0x443b70)` | CZGameFrame plus runtime/message-map | `CZGameFrame::CreateObject` / `CZGameFrame::OnAppIdleDispatchMessage` | Eight recorded semantic subranges; `0x443a40` is a zVideo cached-client-rect helper island, not a proven emitted header row; `0x443810` is class-coupled scalar deleting destructor glue. |
| `[0x443b70,0x443c50)` | MFC/GDI provider destructors | `CGdiObject::ScalarDeletingDestructor` / `CBitmap::Destructor` | Provider boundary split into CGdiObject and CBitmap destructor glue before `cls_di.c`. |
| `[0x470020,0x4706c0)` | likely `zInput/zin_mouse.cpp` | `zInput::Mouse_ApplyClientCursorPosToOS` / `zInput::Mouse_WaitForButtonPress` | Comment/order-backed; no local literal. |
| `[0x4706c0,0x4719e0)` | likely `zInput/zinput.cpp` | `zInput_BindMapContext::InitFromTemplate` / `zInput::PopBindMapContextOverlay` | Bind-map/context/current/overlay block. |
| `[0x471e40,0x472670)` | likely `zInput/zin_joystick.cpp` | `zInput::DI_InitJoystickDevice` / `zInput::DI_ReportError` | `0x472450` and `0x472480` physically sit in the joystick tail but semantically route to force-feedback (`zin_ff.cpp` in source/docblocks); `0x472490` is a shared DirectInput diagnostic helper whose standalone-vs-shared owner shape remains unresolved. |
| `[0x4a3930,0x4a3ea0)` | zSnd fade lists | `zSndFadeLists::Init` / `zSndFadeList::PopFrontCursor` | Bracketed no-literal `zsnd_fade.cpp` placement with 4 semantic subranges: lifecycle/global setup, dispatch/update, active-list ticking, and cleanup/list cursor helpers. Not an independent literal-backed source-file block. |
| `[0x4a3ea0,0x4a3ef0)` | zSnd MCI report helper | `zSnd::ReportMciError` / same | Strong semantic `zsnd_cd.cpp` exception physically inside the report shelf; all direct caller file literals are `zsnd_cd.cpp`, but that proves caller diagnostics rather than whole-shelf definition placement. |
| `[0x4a3ef0,0x4a44c0)` | zSnd A3D/DirectSound report helpers | `zSnd::ReportA3DError` / `zSnd::ReportDirectSoundError` | Proven address-backed helpers, but source provenance is conflicted. Current BN comments and adjacent diagnostic string/table layout favor `zsnd_create.cpp`; caller literals span `zsnd_play.cpp`, `zsnd_init.cpp`, `zsnd_create.cpp`, `zsnd_parm.cpp`, `zsnd_3d.cpp`, and others, proving call-site diagnostics rather than definition placement. The exact auxiliary split is A3D body `[0x4a3ef0,0x4a4248)`, A3D switch output `[0x4a4248,0x4a432c)`, padding `[0x4a432c,0x4a4330)`, then DirectSound body `[0x4a4330,0x4a44c0)`. |
| `[0x4a53f0,0x4a5670)` | zSndWaveData class island | `zSndWaveData::zSndWaveData` / `zSndWaveData::LoadAndParseFromIndexArchiveIfNeeded` | Strong semantic zSound class owner, but physical placement is not literal-backed and occurs after `zsnd_grp.cpp`; current provisional physical candidate is `zsnd.cpp`, not the invented `zsnd_wave.cpp`. BN comments conflict between `zSound.cpp` and `zsnd.cpp`, with no literal resolving either spelling. Refined subranges are lifecycle, WAVE parse, and lazy load/reset/archive wrapper phases. |
| `[0x4a5670,0x4a59d0)` | provisional `src/GameZRecoil/zSys/zsys.cpp` | `Time::Reset` / `zVid::QueryCachedClientRectUpdateMaskIf3dfx` | Collapsed mixed zSys shelf before provider thunk. Previous log-split and post-body emitted-header claims are rejected until VC5 order tests prove them; focused BN found no emitted `zsys.h` address evidence. |
| `[0x4a59d0,0x4a59e0)` | provider `KERNEL32_GetTickCount_ImportThunk` | `KERNEL32_GetTickCount_ImportThunk` / same | Provider import thunk. It may coincide with a source-shape boundary but is not authored source evidence. |
| `[0x4a59e0,0x4a5c20)` | provisional `src/GameZRecoil/zSys/zsys.cpp` | `zSys::FindFileOnDriveType` / `zLoc::GetMessageString` | Collapsed mixed zSys shelf continuation after provider thunk. Previous drive/message physical splits are semantic filename hypotheses only; focused BN found no `zLoc.*` source-path literal proving a physical split. |
| `[0x4a5c20,0x4a66e0)` | literal-backed `src/GameZRecoil/zUtil/zutl_zar.cpp` | `zReader::FileExists` / `zIndexArchive::ReadFileByName` | zReader/ZRDR semantic prefix from `0x4a5c20..0x4a6190`, then zIndexArchive/ZAR core through `0x4a66e0`; source-path literal in `zIndexArchive::Init` proves the physical block. |
| `[0x4a66e0,0x4a69c0)` | zVideo prelude | `zVideo::GetDisplayModeBpp` / `zVideo_dd::PrepareWindowForMode` | Mostly zVideo semantic work with `zVideo_dd3d`/`zVideo_dd` callback or window-helper exceptions. It precedes direct `zvid_buff.c` literal-backed island and is not `zvid_buff.c`, `zvid_init.c`, or `zvid_dd.c` block evidence. No `zvid_main.c`, `zVideo.cpp`, `zvid.h`, or `zvid_dd.c` literal was found in the focused BN sweep. The catalog records function-granular owner routing for mode-switch, surface accessors, clear dispatch, postprocess, surface-state thunk/accessors, adjust-surfaces, and DirectDraw window-helper slices. |
| `[0x4b2960,0x4b3ce0)` | zGame/zSnd/zSys option and CPU band | `zGame::Options_LoadFromRegistry` / `zSys::Sub64` | Owner-interleaved no-literal output. `engine.zgame.options_registry_option_list` is split across `[0x4b2960,0x4b2f50)` and `[0x4b3260,0x4b33f0)`; `[0x4b31b0,0x4b3260)` is mixed zSys/zSnd/zVid and cannot support one owner gate. |
| `[0x4b3ce0,0x4bd470)` | reusable UI class clusters | `HudUiWidget::ScalarDeletingDestructor` / `HudUiTextStack4::SetYDescending` | High-confidence semantic UI body under the provisional `zUI/zui.cpp` shelf, but not one accepted source owner. `zUI/zui_widgets.cpp` or `zUI/zui_controls.cpp` remain finer semantic hypotheses, not proven physical rows. Contains VC5 STL/vector COMDAT/provider exclusions at `0x4ba470`, `0x4ba4d0`, and `0x4ba510`; `0x4bcb48` is padding. |
| `[0x4bd470,0x4bf060)` | zTimedTask/HudLineClip/zVideo/HudWeatherFx | `zTimedTask::RemoveFromActiveList` / `zVideoFxPass3Config::Constructor` | Mixed owner islands and zMath helper exceptions. `0x4bd470..0x4bdb60` is a medium-high helper island, not a proven separate draw/clip `.cpp`. `render_video.zvideo_fxpass3_ui_local_config` is split into islands `[0x4bdb60,0x4bdc70)` and `[0x4bed30,0x4bf060)` around HudWeatherFx and must be scheduled as one semantic owner. That non-contiguous placement argues against an accepted contiguous `zvid_fxpass3.cpp` row until VC5 order tests prove one. `0x4bdc70..0x4bed30` is a high-confidence weather-FX class cluster, but `src/GameZRecoil/zWeather/zweather_fx.cpp` remains a semantic filename hypothesis only. |
| `[0x4bf060,0x4c0d20)` | UI message widgets plus zUtil/zutl_zbd recursive cluster | `HudUiMessageBoxDialog::Constructor` / `zZbdSectionHandlerList::SpliceThreeNodes` | `0x4bf060..0x4bffe0` is a high-confidence reusable UI/dialog/background tail inside the provisional `zUI/zui.cpp` shelf. BN caller-origin examples from earlier UI code (`0x4b463e -> 0x4bf840`, `0x4b95a6 -> 0x4bfc80`, `0x4ba2bd/0x4ba2ef -> 0x4bffb0`) support same-shelf later definitions rather than an accepted new `.cpp` row. `0x4bffe0..0x4c0d20` remains the zUtil/ZBD cluster. `0x4c06f0` is CRT `tmpfile` provider; sort cascade at `0x4c07d0` is template-expanded in this block. |
| `[0x4c5a50,0x4c5eb8)` | MFC42 import thunks | `Ordinal_MFC42_5265` / `CFrameWnd::OnActivate` | Provider import packet; no authored body. |
| `[0x4c5ec0,0x4c637c)` | VC5 CRT/compiler runtime | `MSVC_EH_ArrayDestructor` / `_controlfp` | Includes CRT startup `_start`; provider even though it calls authored `WinMain`. |
| `[0x4c63f0,0x4c7408)` | DirectInput SDK keyboard provider data | `c_dfDIKeyboard` data | Provider data in `.text`, not authored Recoil data. |
| `[0x4c7408,0x4c7410)` | linker padding | none | Padding between DirectInput provider data packets. |
| `[0x4c7410,0x4c7498)` | DirectInput SDK mouse provider data | `c_dfDIMouse` data | Provider data in `.text`, not authored Recoil data. |
| `[0x4c74a0,0x4c7ef8)` | DirectInput SDK joystick provider data | `c_dfDIJoystick` data | Provider data in `.text`, not authored Recoil data. |
| `[0x4c7f00,0x4c7fd0)` | candidate `zimg_fonts.h` partial-header contribution | `zImage_Font::BlitStringToActiveTarget` | Routes semantic work to the zImage font/blit owner, but physical placement is provisionally modeled as a body-bearing header included through `src/WinMain.cpp`; validate with VC5 order tests before accepting full header extent or owner gates. |
| `[0x4c7fd0,0x4c81c0)` | candidate `zvid.h` partial-header contribution | `zVideo::LoadPaletteFileAndApplyBrightness` / `zVideo::ApplyBrightnessToPaletteEntries` | Routes semantic work to zVideo palette/brightness owners, but physical placement is provisionally modeled as a body-bearing header included through `src/WinMain.cpp`; validate with VC5 order tests before accepting full header extent or owner gates. |
| `[0x4c81c0,0x4c81d8)` | authored WinMain tail island | `WinMain` | Routes to root-level `src/WinMain.cpp`; provider CRT startup calls this but does not own it. |
| `[0x4c81d8,0x4cb9e8)` | MFC/CRT and compiler EH tail | `MFC_ModuleState::SetMbcpData` / `__ehhandler_zInterp_Context_Destructor` | MFC module-state helpers followed by MSVC C++ EH unwind funclets and handlers. |

### Literal-Backed Blocks With Internal Owner Slices

These blocks have source-path literal evidence for the physical source-file
range, but internal semantic owners still differ from the physical `.cpp` or
`.c` file. Rebuild order must preserve the physical block while source work
uses complete owner units.

| Physical range | Internal placement fact | Routing consequence |
| --- | --- | --- |
| `Camera.c [0x449ba0,0x44d990)` | Physical first/last is `0x449ba0 zClass_Camera::SetViewDistance` through `0x44d600 zVideo_sw::RenderFrame`. Internal islands include zClass render traversal `[0x44af60,0x44c1b0)` routed to `engine.zclass.render_traversal_dispatch`, and the zVideo software frame orchestration tail `[0x44d600,0x44d990)`. | Do not hand off or verify `0x44d600` as Camera/zClass source. Preserve its physical Camera.c placement while routing semantic work to `render_video.zvideo_sw_frame_orchestration`; no current evidence proves a header/COMDAT explanation for this tail. |
| `cls_util.c [0x4518b0,0x452920)` | Xrefs to the `cls_util.c` literal stop at `zClass_cls_util::CopyNode`, but the physical block continues through `zClass_Node::AssignInt32ToDiRecursive`. Tail islands are BBox helpers `[0x4525d0,0x452770)`, `0x452770` Class.c-style `FindSubNodeByName`, predicate traversal `[0x4527f0,0x452850)`, `0x452860` material flag recursion, and node DI recursive updates `[0x4528a0,0x452920)`. | Treat cls_util as a physical-order block, not one semantic owner. BBox/Class/Object3D/List tail functions need owner-scoped routing before byte/source-path acceptance. |
| `List.c [0x44e630,0x44f7a0)` | Mostly coherent type-list/node-list operations. `0x44f630 zClass_List::RenderActiveCameras` is physically List.c by neighbor order and comments, has no local file-literal push, calls `zClass_TypeList::GetBucketHead`, then dispatches to `zVideo_sw::RenderFrame` or `zClass_Camera::RenderScene` depending on `g_zVideo_ActiveRendererPath`. | Keep `0x44f630` physically in List.c while routing semantic dependencies to the camera/render pass. Do not reclassify it to Camera solely from callees. |
| `Window.c [0x44f7a0,0x44fdd0)` | `0x44f870 zClass::RemoveChildChecked` pushes the Window.c source literal directly, then tail-jumps to `zClass_Class::RemoveChildGeneric` on valid parent/child. Its BN comment saying List.c is stale against the literal and physical Window.c block. | Treat as a Window.c-emitted zClass child-removal validation helper unless stronger header/inline evidence appears. Do not schedule it as a Window standalone owner. |
| `Switch.c [0x452920,0x4529c0)` | Literal xrefs are in both `0x452920 zClass_Class::AddChildValidated` and `0x452970 zClass_Class::RemoveChildValidated`; the row now has function layers `[0x452920,0x452970)` and `[0x452970,0x4529c0)`. Current source comments keep cleanup in Class.c, and BN has a stale `Class.c` comment on `0x452970`, but assembly/HLIL push the `Switch.c` literal. | Route through add/remove child dispatch owners, not a standalone Switch source owner. The physical Switch.c block still constrains COFF order. |
| `Seq.c [0x453ee0,0x454360)` | Literal xrefs are only in Sequence functions from `gwSequenceNew` through `Update`; the physical tail `[0x4542a0,0x454360)` is Lod classdata/add-remove behavior. | Sequence semantic ownership stops before the Lod tail. Lod functions route to `engine.zclass.lod_classdata_methods` plus add/remove dispatch dependencies while preserving the physical Seq.c tail. |
| `cls_zbd.c [0x454360,0x4558f0)` | The no-literal `0x454360 zClass::ResetCurrentZbdPath` is a one-function prefix immediately before `0x454370 GameZ_ZBD::NodePtrToIndex`, `0x4543a0 zClass::NodePtrToValidatedIndex`, and `0x4543d0 GameZ_ZBD::NodeIndexToPtr`; direct cls_zbd.c literal xrefs follow inside the same physical block. `0x4543a0` is semantically zClass, reads `g_zClass_NodeArray`, and is called by zEffect save/queue paths, but is physically embedded in the cls_zbd.c prefix next to ZBD node-index helpers. | Preserve cls_zbd.c physical placement for the prefix. No separate `cls_path.c` file is proven. Route semantic ownership/use to zClass/zEffect dependencies; do not move `0x454360` or `0x4543a0` out without header/COMDAT evidence. |
| `zdec_crater.cpp [0x456ad0,0x458af0)` | Literal xref evidence is in crater code, but the block splits into crater feature/event/build core `[0x456ad0,0x4575f0)` and zDEClient feature/list/map-tree/camera tail `[0x4575f0,0x458af0)`. The tail includes feature-system lifecycle `[0x457650,0x457750)`, feature display/list/apply/init-state `[0x457750,0x457d90)`, map-tree operations `[0x457d90,0x458a30)`, feature-entry storage `[0x458a30,0x458aa0)`, and camera accessors `[0x458aa0,0x458af0)`. | Do not rebuild or verify the whole physical block as one crater owner. Crater and zDEClient feature-map/list/camera owners must be scheduled separately while preserving the proven physical order. |
| `zeff_anim_run.c [0x458af0,0x45e100)` | One physical zEffect animation-run contribution alternates semantic names: conditional refs/reset/beam/event handlers `[0x458af0,0x45ae30)`, `zEffect_Anim` keyframe/evaluate/run `[0x45ae30,0x45b3b0)`, attach/detach/surface/emitter/conditional/screen/top-message handlers `[0x45b3b0,0x45cc00)`, sequence capture/restore with `0x45d000 zEffect::SetAnimDebugFrameTag` interleaved `[0x45cc00,0x45d6c0)`, runtime stop/reset/activation/setters `[0x45d6c0,0x45e0d0)`, and callback/effect-level tail `[0x45e0d0,0x45e100)`. | Current consolidated `zEffect.cpp` source comments are weaker than the literal-backed `zeff_anim_run.c` block. Route complete zEffect/zEffect_Anim/zEffectAnim owners while preserving this physical C-file order. |
| `zeff_anim_init.c [0x45e100,0x460020)` | Physical anim-init block includes zEffect world/resource setters `0x45e200`/`0x45e270`, sound/light/node lookup `[0x45e280,0x45e6d0)`, copied-root/clone/rebind/ZBD load `[0x45e6d0,0x45fd10)`, shutdown `[0x45fd10,0x45ff10)`, and lookup/async/root access `[0x45ff10,0x460020)`. | Treat zEffect setters here as semantic islands physically emitted in anim-init; do not collapse the block into consolidated `zEffect.cpp` byte work. |
| `zeff_init.c [0x460020,0x4603d0)` | zEffect init/shutdown/init-from-path/reset cluster split as `0x460020` init, `0x460060` shutdown-all, `0x460070` literal-bearing init-from-path, and `0x460330` reset. Direct literal xrefs are in `zEffect::InitFromPath`, with adjacent functions bounded by order. | Keep this as a distinct physical `zeff_init.c` contribution before save/load work; do not fold these bodies into consolidated `zEffect.cpp` solely because of semantic names. |
| `zeff_anim_save.c [0x4603d0,0x4622f0)` | Activation-record save/load `[0x4603d0,0x461970)`, activation queue commands `[0x461970,0x461eb0)`, `0x461eb0 zEffect_Anim::SetActivationDispatchContext`, and zEffect runtime/template helpers `[0x461ec0,0x4622f0)` are physically in the save block. | Runtime/template bodies currently described as `zEffect.cpp`/`eff_runtime.c` are semantic islands in physical `zeff_anim_save.c` unless later evidence proves a header/COMDAT path. |
| `zerr_old.c [0x4622f0,0x462330)` | Old error/debug bridge split as `0x4622f0 zError::EmitDebugBuffer` and `0x462310 RecoilError::InitOutputContext`. The `zerr_old.c` literal xref is in `EmitDebugBuffer`; the context initializer is included by neighbor order before the `fmv_main.cpp` boundary. | Preserve the physical `zerr_old.c` row while routing namespace-level ownership carefully; the `RecoilError::` semantic name does not prove a separate file. |
| `fmv_main.cpp [0x462330,0x4625e0)`, `fmv_script.cpp [0x4625e0,0x463d50)`, `fmv_stream.cpp [0x463d50,0x464670)` | BN literals split FMV into playback/MCI class bodies in `fmv_main.cpp`, script/action hierarchy in `fmv_script.cpp`, and stream decode/audio bodies in `fmv_stream.cpp`. `fmv_main.cpp` now has three recorded layers: playback constructor/destructor `[0x462330,0x462370)`, MCI open/play `[0x462370,0x4624f0)`, and stop/dest/error tail `[0x4624f0,0x4625e0)`. Current source comments often route playback and stream through `fmv_script.cpp`/`fmv.h`. `fmv_script.cpp` also contains class-owned scalar deleting destructors, while zFMV action base virtuals/destructor/update/run-timed appear earlier in the HUD physical block `[0x4159d0,0x415ab0)`. | Preserve the three physical FMV source blocks and route class hierarchy work across the out-of-range HUD zFMV action-base island. Do not treat class-owned deleting destructors as standalone authored owners or provider-primary work. |
| `ai_net.cpp [0x401060,0x4038a0)` | Physical AINet block has proven address-emitting `ai_net.h [0x401060,0x402f60)`, `zMath.h [0x402f60,0x402fd0)`, and 6 recorded `ai_net.cpp` body layers from ZRD load through teardown. `0x403750` is player-save-state-coupled but still physically in `ai_net.cpp`. | Preserve the detailed `ai_net.cpp` source-shape order. Do not move `0x403750` to `player.cpp`, do not create a new emitted `.h` row for `0x403620`, and do not reintroduce `.inl` source shape for `0x402f60`. |
| `hud.cpp [0x404ca0,0x415ab0)` | Physical HUD block with 21 recorded source-shape layers. The first source-path literal is inside `0x410160`, and another appears at `0x414180`; the earlier layers are inferred from BN assembly/order, not comments. | Do not treat physical `hud.cpp` as one semantic owner. Use the detailed `hud.cpp` source-shape layer table below and preserve physical HUD block order while routing semantic owners separately. |
| `map.cpp [0x415ab0,0x417350)` | Physical map block now has 8 recorded source-shape layers: HudSensorMapNode methods, HudRectI/HudGeom2D clipping helper island, projected-path drawing, tracker init/list maintenance, map file load, overlay/zoom/ref-count, projection/save-state marker drawing, and mission-map SFX/objective marker controls. | Use the detailed `map.cpp` source-shape layer table below. Do not move HudRectI/HudGeom2D helpers to `hud.cpp` or a new header row solely from semantic names/comments. |
| `pickup.cpp [0x41cc10,0x41ea90)` | Physical pickup block now has 10 recorded source-shape layers: subsystem bootstrap, init/resource registration, pickup-specific zClass_Node flag recursion, collection/effects/player grants, spawn-list lifecycle, pickup type lookup/Net-style slot predicate, ZRD spawn loading, respawn queue maintenance, ZAR callbacks, and network/drop helper tail. | Use the detailed `pickup.cpp` source-shape layer table below. Do not split zClass/Net-like helpers into separate `.h` or source rows without VC5 source-shape evidence. |
| `player.cpp [0x41ea90,0x42de10)` | Physical player block now has 19 recorded source-shape layers. Semantic/provider islands include HUD scalar/reset glue, zReader/checkpoint helpers, zClass recursive flags, zInput bind maps, cls_di/HUD helper activity, zMath midpoint, zCom, WOL API init, and provider SafeVtableRelease tail. | Use the detailed `player.cpp` source-shape layer table below. Player-owned work must preserve physical order while routing complete semantic owners separately when evidence supports them. |
| `RecoilApp.cpp [0x42de10,0x436630)` | Physical RecoilApp block now has 9 recorded source-shape layers: MFC app/static startup, zInput joystick/force-feedback exceptions, engine startup/shutdown, CZRecoilFrame MFC frame/menu/message-map layer, MFC/provider destructor glue, GameNet packet relay, save/load dialog file list, and save/load transition/sort/vector tail. This pass did not support a zWeapon split. | Use the detailed `RecoilApp.cpp` source-shape layer table below. Owner work must not collapse this block into one app-shell class or invent zWeapon/header ownership without stronger evidence. |
| `turret.cpp [0x436630,0x437e60)` | Physical turret block now has 7 recorded source-shape layers: private `zTurret_Runtime` init/load, per-frame/fire/damage mechanics, `zTurret_System` manager, tick callback wrappers, damage callback adapter, teardown, and fire-animation callback adapter. | Use the detailed `turret.cpp` source-shape layer table below. Model `zTurret_Runtime` as a private non-polymorphic runtime state object unless later BN evidence proves a vtable/interface shape; no emitted `turret.h` row is proven. |
| `WinSock.cpp [0x43ce80,0x43cf90)` | Provisional no-literal physical block now has 6 recorded source-shape layers: NetUi Winsock prompt helper, padding, `zStr.cpp` CRT init-table stub, padding, Net IPv4 formatter, and padding before WOL. | Use the detailed `WinSock.cpp` source-shape layer table below. Do not accept exact NetUi/Net/WinSock filenames from BN comments alone; `0x43cf90` is the WOL boundary. |
| `zgeo_weiler.cpp [0x464670,0x46a690)` and `zgeo_model.cpp [0x46a690,0x46bd50)` | Cross-emitted geometry islands: zgeo_weiler physically ends with zgeo_model-style helpers `0x469e50..0x469e90`, `0x46a130`, `0x46a5e0`, `0x46a620`; zgeo_model contains zgeo_weiler helpers `0x46a9c0`, `[0x46ab10,0x46ac80)`, zDEClient/ClipPatchOutput `[0x46ae40,0x46af40)`, and `0x46b650`. | Preserve both literal-backed physical blocks, but route Weiler/model/clip-patch/zDEClient semantic owners by complete owner boundaries. Many source-comment helper candidates are no-standalone and need BN caller scrutiny before acceptance. |
| `zimg_texture.cpp [0x46d310,0x46efc0)` | Semantic zVideo/zVid islands include texture/default image helpers `[0x46d5a0,0x46d780)`, `0x46d870`, texture-pack load helpers `[0x46d940,0x46dd30)`, `0x46df50`, palette/remap/image-resample `[0x46e4e0,0x46e9b0)`, and zVid_Image file I/O `[0x46ec00,0x46ef70]`. | Physical `zimg_texture.cpp` block is stable, but zVideo/zVid image and texture-pack owners must be scheduled independently; `zImage::CreateDefaultTextureRecord` remains no-standalone helper evidence, not an address-backed owner. |
| `zimg_fonts.cpp [0x46efc0,0x46f300)` | Literal-backed font block split into font index/default lookup `[0x46efc0,0x46efe0)`, literal-bearing `FontsLoadFromPath` `[0x46efe0,0x46f130)`, glyph-rect build `[0x46f130,0x46f210)`, transparent-column probe `[0x46f210,0x46f260)`, and measure-string tail `[0x46f260,0x46f300)`. | Preserve this as the normal `zimg_fonts.cpp` font load/layout contribution. The separate `0x4c7f00` blit body remains the candidate `zimg_fonts.h` contribution through `src/WinMain.cpp`, not a reason to split this physical block. |
| `gmod_init.c [0x475c40,0x4805b0)`, `gmod_matl.c [0x4805b0,0x481530)`, `gmod_const.c [0x481530,0x487a30)` | Major internal islands include gmod_init display/init `[0x475c40,0x476030)`, fog `[0x476040,0x4762f0)`, zClipAlt `0x4766a0/0x479f90/0x47a1d0`, zClipRect `[0x47a200,0x4803b0)`, `0x4804e0` zReader; gmod_matl material/gdi/zModel_Display islands through `0x481460`; gmod_const zDi/DiPool/gdi/zModel/zClass cls_di islands including `0x4826a0` zUtil and cls_di `[0x484960,0x4879c0)`. | These are literal-backed physical model blocks but heavily semantic-owner interleaved. Do not accept block-local byte/source gates until zModel/zClip/zReader/gdi/zClass/zUtil owners and no-standalone model helper candidates are reconciled. |
| `gmod_init.c [0x475c40,0x4805b0)` refined | Physical model-init block contains zModel display/init `[0x475c40,0x476030)`, render flags/setters `[0x476020,0x476110)`, zClipAlt rect/remap/projection helpers `0x476120`, `0x4766a0`, `0x479f90`, `0x47a1d0`, fog accessors `[0x476170,0x476300)`, `0x476480` zMath projection helper, zScene/zDi/zModel render path `[0x476700,0x478fc0)`, OptCatalog damage-mask helpers `[0x478fc0,0x479ce0)`, zVideo/zClipAlt projection `[0x479ce0,0x47a200)`, zClipRect cluster `[0x47a200,0x4804c0)`, and `0x4804e0 zReader::FindGlobalStringPrefixIndex`. | Current source comments route many of these to zModel_Display, gmod_light, zGeometry/zClipAlt, zGeometry/zClipRect, zReader lookup, gmod_scene, or zVideo. Physical gmod_init.c order wins until header/include/static-helper evidence proves otherwise. |
| `gmod_matl.c [0x4805b0,0x481530)` refined | Material slot/pool accessors `[0x4805b0,0x480600)`, material serialization/init/resize `[0x480600,0x480c40)`, material reset/compare/release/cycle/global-string release `[0x480c40,0x481460)`, and `zRndr_GlobalStringTable::LoadDynamicEntriesFromPath [0x481460,0x481530)` are physically gmod_matl.c. | Source map entries to zModel_Display, gdi.c, or zRndr global string table are semantic placement hints only. Preserve physical gmod_matl.c emission while routing complete material/global-string owners. |
| `gmod_const.c [0x481530,0x487a30)` refined | Epsilon accessors `[0x481530,0x481570)`, zDi pointer/index helpers `[0x481570,0x4815c0)`, DiPool stream read/write `[0x4815c0,0x482080)`, DiPool alloc/free `[0x482080,0x482160)`, zDi lifecycle/ref/flag helpers plus `0x4826a0 zUtil::StoreInt32` `[0x482160,0x482720)`, model-const geometry helpers `[0x482720,0x483610)`, zDi polygon/material helpers `[0x483610,0x484860)`, local gradient helper `[0x484860,0x484960)`, and zClass cls_di runtime island `[0x484960,0x487a30)` are physically gmod_const.c. | Do not schedule the zClass cls_di island, zUtil leaf, or zDi/DiPool helpers as isolated byte work without reconciling their complete source owners and gmod_const.c physical placement. |
| `gmod_light.c [0x487a30,0x489d00)` | Active-light list/build/test/set input `[0x487a30,0x488d60)` and light weights/distance/fog/falloff `[0x488d60,0x489d00)` form a comparatively coherent light block. | Earlier fog/render-current accessors currently mapped to gmod_light are physically in gmod_init.c, not this block. |
| `znet_dplay.cpp [0x489d00,0x48c7d0)` | `0x48bf10 zUtil::CopyDwordRange` is a VC5 STL/compiler-header COMDAT helper selected into this physical block. BN shows a generic `int32_t* __stdcall(srcBegin, srcEnd, dst)` dword-copy body `[0x48bf10,0x48bf3e)`, `retn 0xc`, no data refs/callees, and callers only from vector insertion paths across HUD, zInput, zNetwork, and zSound. Sibling helpers `0x40c190 zUtil::UninitializedFillDwordN` and `0x40c1c0 zUtil::CopySingleDword` match the same VC5 vector helper family. | Do not route `0x48bf10` to authored zUtil or zNetwork source ownership. Treat it as an address-backed provider/compiler-header COMDAT boundary; authored vector-using owners should depend on that provider helper rather than own it. |
| `znet_dplay.cpp [0x489d00,0x48c7d0)` refined | Internal islands include session runtime `[0x489d00,0x489f30)`, session/service/player list helpers `[0x489f30,0x48a0d0)`, DirectPlay provider/session enumeration/open/status `[0x48a0d0,0x48a980)`, local player/send/receive/pump `[0x48a980,0x48b3a0)`, DirectPlay enum callbacks `[0x48b3a0,0x48b730)`, COM/DirectPlay lifecycle `[0x48b730,0x48b820)`, player-color/record/status `[0x48b820,0x48be10)`, lobby/session helpers `[0x48be10,0x48bf10)`, provider COMDAT `0x48bf10`, dispatch-list helpers `[0x48bf40,0x48c250)`, and error-report tail `[0x48c250,0x48c7d0)`. | Early helpers lack direct local literal xrefs but are bracketed by znet physical order. DirectPlay provider APIs are dependencies; callback and dispatch helpers are authored zNetwork source-owner members, not standalone owners. |
| `zreader.cpp [0x48c7d0,0x48d340)` | ZRDR free-node pool helpers `[0x48c7d0,0x48c950)`, archive-list/list-node helpers `[0x48c950,0x48cc70)`, ZRDR init/search-path/shutdown `[0x48cc70,0x48cd40)`, zReader resolve/load/free/find/read/open `[0x48cd40,0x48d210)`, and archive mount/unload `[0x48d210,0x48d340)` are physically zreader.cpp. Only one direct source literal xref is late at `0x48d188`. | Current source comments split this range into zreader.cpp, zreader_load.cpp, and zreader_lookup.cpp; physical evidence favors one zreader.cpp emitted block. `core_util_archive.zutil_zrdr_free_pool_data` needs source-shaped parent reconciliation before byte work. |
| `zrndr_draw.c [0x48d340,0x49f614)` | Early zVideo/zVid noise/surface helpers `[0x48d340,0x48d420)`, overlay blend `[0x48d450,0x48d7a0)`, zVideo noise/fx/image blit `[0x48d910,0x48f560)`, zRndr core `[0x48fd80,0x49f180)`, and `0x490330 zFloat::Set255f` semantic exception. MMX/ESP-pivot rows are authored standalone raw-asm exceptions, not provider artifacts. | Preserve zRndr physical order while routing zVideo/zVid/zMath exceptions and zRndr no-standalone span/fog/texture helper candidates under their parent render owners. |
| `zsnd_play.cpp [0x49f614,0x4a10e0)` | `0x49f614..0x49f620` is NOP padding labeled `zSnd::TickWrapper`; authored code starts at `0x49f620`. The block then contains zSnd tick/fade, zSndSample, sample-set registry, and scalar playback helper slices. | Do not schedule `0x49f614` as a source owner. Preserve `zsnd_play.cpp` physical emission while routing zSndSample/sample-set/scalar helpers to complete zSound owners. |
| `zvid_init.c [0x4a6b40,0x4a7b40)` | `0x4a6b60` and `0x4a6b70` are tiny DD/DD3D pending-state setter semantic exceptions physically emitted in the init block. | Preserve placement through original-style declarations/includes; do not move semantic `zvid_dd.c`/`zvid_ddd3d.c` owners into the wrong source file merely for placement. |
| `zvid_ddd3d.c [0x4a9ac0,0x4ae380)` | Core DD3D code runs through `0x4ad680`; `0x4ad680` is address-backed `zVideo_dd3d::FloorPowerOfTwo`; `0x4ad6a0` is `zVideo_dd::ReportError`, a `zvid_dd.c` diagnostic helper physically emitted in this block tail. `[0x4ae1ec,0x4ae380)` is VC5 compiler-emitted switch machinery for `ReportError`: jump table `[0x4ae1ec,0x4ae274)`, byte lookup table `[0x4ae274,0x4ae2b1)`, 3-byte alignment `[0x4ae2b1,0x4ae2b4)`, jump tables `[0x4ae2b4,0x4ae348)` and `[0x4ae348,0x4ae380)`. | Rebuild `zVideo_dd::ReportError` as the `zvid_dd.c` diagnostic helper but preserve its physical tail placement and generated `.text` switch output. Do not model the table tail as authored arrays, provider data, callback tables, or primary data owners. |
| `zwep_init.c [0x4ae380,0x4b2960)` | BN evidence supports this as a real broad physical contribution block. Internal islands include OptCatalog runtime/process core `[0x4ae380,0x4af060)`, impact/aim/warning behavior `[0x4b0530,0x4b1090)`, zWeapon init/load `[0x4b1090,0x4b1d90)`, OptCatalog shutdown/trail/fx load `[0x4b1d90,0x4b2160)`, Light thermal pool plus PlayerTimedHit/HitSource `[0x4b2160,0x4b25a0)`, zClass damage callbacks `[0x4b25a0,0x4b26f0)`, and DamageFeedback/HitContext/MineIterator tail `[0x4b26f0,0x4b2960)`. | Do not collapse `zwep_init.c` into one semantic owner. Use the physical block as VC5 order evidence while scheduling complete linked OptCatalog/zWeapon/Light/player-hit/zClass/DamageFeedback/HitContext/MineIterator owners. |

#### `ai_net.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names and
comments were used only as navigation labels; placement evidence is the proven
`ai_net.cpp` source-path literal xref at `0x4030bb`, neighboring function
order, current BN assembly/xrefs, and the existing address-emitting
partial-header rows. Preserve the natural order `ai_net.h`, then `zMath.h`,
then `ai_net.cpp` body.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x401060,0x401180)` | `ai_net.h: mode2_top_level_dispatch_layer` | proven own-header contributor | AI mode-2 top-level dispatcher; switch over top-level AI states. |
| `[0x401180,0x401710)` | `ai_net.h: path_follow_cursor_branch_layer` | proven own-header contributor | Path follow, forward probe/auto-turn, path cursor advance, and branch selection. |
| `[0x401710,0x401b20)` | `ai_net.h: steering_substate_leaf_layer` | proven own-header contributor | Steering substate dispatcher plus basic move/turn, offset-target, and dynamic-offset steering leaves; includes tailcall forwarder at `0x401964`. |
| `[0x401b20,0x401f60)` | `ai_net.h: attack_pursuit_los_layer` | proven own-header contributor | Attack-pursuit transition: line-of-sight gate, buddy alert, enter-steering pursuit, and LOS raycast helpers. |
| `[0x401f60,0x402080)` | `ai_net.h: synthetic_path_rebuild_layer` | proven own-header contributor | Synthetic path-node rebuild when far from current node. |
| `[0x402080,0x402250)` | `ai_net.h: restore_turn_player_layer` | proven own-header contributor | Restore saved top-level state and turn-toward-player helpers. |
| `[0x402250,0x4026d0)` | `ai_net.h: altgun_window_lead_layer` | proven own-header contributor | Alternate-gun attack window, target acquisition, and lead-target solve. |
| `[0x4026d0,0x402b70)` | `ai_net.h: offset_dynamic_pursuit_layer` | proven own-header contributor | Offset and dynamic-offset pursuit target movement and steering. |
| `[0x402b70,0x402f10)` | `ai_net.h: timed_path_steering_layer` | proven own-header contributor | Timed path steering plus forward/reverse steer toward path nodes. |
| `[0x402f10,0x402f60)` | `ai_net.h: mode2_state_finalize_layer` | proven own-header contributor | Global finalize/reset of mode-2 state-1 AI players immediately before `zMath.h` `Vec3Normalize`. |
| `[0x402f60,0x402fd0)` | `src/GameZRecoil/zMath/zMath.h` | proven partial-header/header-COMDAT contributor | `0x402f60 zMath::Vec3Normalize` belongs to `zMath.h`; do not use `.inl` or move it into `ai_net.cpp`. |
| `[0x402fd0,0x403510)` | `ainet_zrd_load_parse_allocate_layer` | confirmed literal-bearing `ai_net.cpp` body | ZRD load/parse/allocation layer; contains the `ai_net.cpp` source-path literal xref at `0x4030bb`. |
| `[0x403510,0x403550)` | `ainet_lookup_helper_layer` | `ai_net.cpp` body/helper layer | Lookup helpers for AINet and node lists; exact static/member spelling remains source-shape work. |
| `[0x403550,0x4036f0)` | `ainet_neighbor_link_probe_fan_layer` | `ai_net.cpp` body with record/method helper semantics | Neighbor-link resolution and probe-fan construction; `AINetPathProbeFan::InitFromSegment` does not prove a new emitted header row. |
| `[0x4036f0,0x403750)` | `ainet_nearest_node_spatial_query_layer` | `ai_net.cpp` helper layer | Nearest-node query has external Player caller evidence, but that does not move ownership to `player.cpp`. |
| `[0x403750,0x4037c0)` | `ainet_player_peer_ring_helper_layer` | player-facing semantic tension inside physical `ai_net.cpp` | Walks player save-state data and is called by Player setup, but current physical block evidence keeps it in `ai_net.cpp` unless stronger source evidence appears. |
| `[0x4037c0,0x4038a0)` | `ainet_teardown_cleanup_layer` | `ai_net.cpp` body tail | AINet/node cleanup and global list teardown before `Briefing.cpp`. |

#### `Briefing.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names were
used only as navigation labels; placement evidence is physical order, assembly
shape, the `Briefing.cpp` literal xref at `0x404238`, and provider-shape
inspection for the STL/MFC-style glue. Do not promote these ranges to separate
`.h` rows or accepted source owners until VC5 source-shape experiments prove a
specific original header/source placement.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x4038a0,0x403c80)` | `briefing_objective_picture_runtime_ctor_layer` | probable Briefing/HUD briefing body | HUD briefing objective-picture and runtime constructors precede the literal-bearing body; keep physical Briefing placement. |
| `[0x403c80,0x403d70)` | `briefing_locator_hud_widget_layer` | HUD widget/locator semantic layer | HUD widget helpers are inside physical Briefing; do not reassign solely from HUD labels. |
| `[0x403d70,0x403db0)` | `briefing_shared_scalar_dtor_layer` | class-coupled scalar destructor glue | Treat as class-coupled glue, not standalone authored owners. |
| `[0x403db0,0x403e20)` | `briefing_list_destructor_comdat_layer` | provider/compiler-header COMDAT | `0x403db0` is VC5 STL `<xlist>` destructor COMDAT selected into Briefing. |
| `[0x403e20,0x403ed0)` | `briefing_hud_composite_destructor_layer` | HUD composite/fill-bitmap destructor layer | Preserve physical placement while routing complete HUD class owners separately. |
| `[0x403ed0,0x404140)` | `briefing_runtime_destructor_update_layer` | Briefing runtime destructor/update layer | Likely core briefing runtime layer before the zInput helper. |
| `[0x404140,0x404180)` | `briefing_zinput_wait_key_layer` | zInput semantic exception inside physical Briefing | Assembly polls keyboard and `Sleep`; source placement still requires source-shape proof before moving. |
| `[0x404180,0x404400)` | `briefing_thread_loop_layer` | confirmed literal-bearing Briefing thread/loop layer | Contains source-path literal xref `0x404238`; anchor for confirmed Briefing body. |
| `[0x404400,0x4045b0)` | `briefing_objective_action_index_builder_layer` | objective action builder layer | Keep with Briefing action-building source-shape work. |
| `[0x4045b0,0x404bd0)` | `briefing_action_queue_layer` | action queue/action tick layer | Action queue helpers and action tick bodies need owner-scoped routing. |
| `[0x404bd0,0x404ca0)` | `briefing_shutdown_transport_tail_layer` | shutdown/progress/build tail layer | Physical tail before `hud.cpp`; do not extend Briefing beyond `0x404ca0`. |

#### `hud.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names were
used only as navigation labels; the placement evidence is physical order,
assembly shape, data/xref shape, and the two `hud.cpp` literal xrefs. Do not
promote these ranges to separate `.h` rows or accepted source owners until VC5
source-shape experiments prove a specific original header/source placement.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x404ca0,0x404e80)` | `hud_ui_primitive_class_layer` | possible class-body/header-shaped layer | HUD UI primitive methods are table-visible, but exact header ownership is not proven. |
| `[0x404e80,0x406890)` | `hud_player_camera_helper_layer` | semantic exception inside physical `hud.cpp` | Camera/player helpers must preserve HUD-block placement until source-shape tests prove included-helper ownership. |
| `[0x406890,0x406a00)` | `hud_mfc_three_float_dialog_layer` | dialog helper layer | Route as UI/dialog support, not provider-primary work. |
| `[0x406a00,0x407130)` | `hud_cheat_dialog_and_string_helper_layer` | UI/helper layer | String helper plus cheat/dialog state need same physical-order scrutiny as surrounding HUD code. |
| `[0x407130,0x407190)` | `hud_small_stub_layer` | low-confidence helper/provider-adjacent layer | Recheck provider/source-shape before claiming authored HUD ownership. |
| `[0x407190,0x408a30)` | `hud_options_config_layer` | semantic exception / included-options candidate | Options/config/video/audio/input helpers are physically in `hud.cpp`; do not reassign solely from namespace-like labels. |
| `[0x408a30,0x409040)` | `hud_controls_dialog_layer` | UI dialog layer | Controls dialog/state layer ending with `0x409010 HudUiOptionSelectorWidget::EnableChildAtIndex`; BN showed the previous `0x409020` cut split that function. |
| `[0x409040,0x40a590)` | `hud_credits_panel_layer` | UI panel layer | Credits/panel layout layer. |
| `[0x40a590,0x40a5b0)` | `hud_panel_scalar_deleting_destructor_tail_layer` | UI panel destructor tail layer | Generic `HudUiPanel::ScalarDeletingDestructor` tail between credits panel and command dialog layers. |
| `[0x40a5b0,0x40c370)` | `hud_command_binding_layer` | UI dialog/container layer | Contains vector-like helper activity; `0x40c190..0x40c1f0` remains VC5 STL/xutility COMDAT candidate. |
| `[0x40c370,0x40c6e0)` | `hud_video_capability_probe_layer` | platform/video helper layer | Video capability probe layer, not enough to prove an engine source file. |
| `[0x40c6e0,0x40d1e0)` | `hud_options_dialog_layer` | UI dialog layer | Options panels/overlay state layer. |
| `[0x40d1e0,0x40d3b0)` | `hud_static_init_layer` | static-init/source-shape layer | Static singleton/window-class initialization and cleanup layer. |
| `[0x40d3b0,0x410160)` | `hud_core_manager_prelude_layer` | probable `hud.cpp` body | Core HUD manager/layout/message prelude before the first local literal. |
| `[0x410160,0x4136f0)` | `hud_confirmed_layout_runtime_layer` | confirmed `hud.cpp` body | Contains source-path literal xref `0x4101a3`; anchor for confirmed HUD layout/runtime body. |
| `[0x4136f0,0x414180)` | `hud_sensor_layout_parser_layer` | HUD helper/parser layer | Sensor, top/chat, layout-node parser, and message-layout helpers. |
| `[0x414180,0x414300)` | `hud_loading_checkpoint_layer` | confirmed/literal-bearing helper layer | Contains source-path literal xref `0x4141bb`. |
| `[0x414300,0x414670)` | `hud_gamenet_chat_score_layer` | semantic exception inside physical `hud.cpp` | GameNet/chat/score-style helpers remain physically HUD-blocked pending source-shape proof; BN showed the previous `0x414660` cut excluded `GameNet::EndChatComposeAndSendThunk`. |
| `[0x414670,0x414a60)` | `hud_triplet_menu_container_layer` | UI/container helper layer | Triplet/list-menu container helpers. |
| `[0x414a60,0x414b60)` | `hud_static_interp_wol_tail_layer` | semantic exception / static-init tail | zInterp/WOL-shaped tail must not be moved out solely from labels. |
| `[0x414b60,0x4159d0)` | `hud_main_menu_dialog_layer` | UI dialog/state layer | Main-menu, transition, and confirm-quit state layer. |
| `[0x4159d0,0x415ab0)` | `hud_fmv_action_tail_layer` | semantic exception tail | zFMV action-base tail is physically in `hud.cpp` until proven otherwise. |

#### `map.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names and
comments were used only as navigation labels; placement evidence is physical
order, assembly/callee shape, and the `map.cpp` literal xref at `0x416922`.
This pass did not prove new emitted `.h` rows.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x415ab0,0x415fb0)` | `map_sensor_node_methods_layer` | HudSensorMapNode data/node method cluster | Treat as map-node record/class-like methods physically emitted in `map.cpp`; no vtable/table-write evidence was observed. |
| `[0x415fb0,0x416480)` | `map_rect_geom_clip_helper_layer` | HudRectI/HudGeom2D helper island | Helper names suggest reusable HUD geometry, but inspected xrefs are local to the map draw pipeline; do not create a header row without stronger evidence. |
| `[0x416480,0x416650)` | `map_projected_path_draw_layer` | projected path draw continuation | Draw helper is coupled to map-node draw and tracker projection. |
| `[0x416650,0x4168d0)` | `map_tracker_init_bounds_list_layer` | HudSensorTracker init/list-maintenance layer | Tracker methods are physically in `map.cpp`; constructor/global setup outside this slice does not shift the physical block. |
| `[0x4168d0,0x416a30)` | `map_file_load_stream_layer` | confirmed literal-bearing map file loader | Contains source-path literal xref `0x416922`; loader uses VC5 EH/SEH allocation-loop shape. |
| `[0x416a30,0x416c90)` | `map_overlay_zoom_refcount_layer` | map overlay/zoom/ref-count layer | Overlay transitions, zoom, ref count, and map SFX calls stay in physical `map.cpp`. |
| `[0x416c90,0x417260)` | `map_projection_savestate_marker_layer` | projection and save-state marker layer | Preserve retail projection, marker draw, and raw float-bit distance approximation behavior. |
| `[0x417260,0x417350)` | `map_mission_path_sfx_objective_tail_layer` | mission-map tail layer | Mission map path/SFX/objective marker controls close the block before `mission.cpp`. |

#### `mission.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names were
used only as navigation labels; placement evidence is physical order, the seven
`mission.cpp` literal xrefs in the objective runtime layer, and sampled
assembly showing provider shape for the MFC scalar-deleting destructor islands.
Do not promote these ranges to separate `.h` rows or accepted source owners
until VC5 source-shape experiments prove a specific original header/source
placement.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x417350,0x417390)` | `mission_init_and_tracker_global_prelude_layer` | mission/HudSensorTracker static prelude | Mission init and tracker global setup start the physical block. |
| `[0x417390,0x417f90)` | `mission_sensor_tracker_lifecycle_layer` | HudSensorTracker mission lifecycle/resources layer | Lifecycle/resource functions precede the literal-bearing objective loader; keep physical mission placement. |
| `[0x417f90,0x419500)` | `mission_objective_runtime_layer` | confirmed literal-bearing objective/weather/start-animation runtime layer | Contains mission literal xrefs `0x417fc2`, `0x4181b6`, `0x418209`, `0x4182ff`, `0x418395`, `0x419091`, and `0x419304`. |
| `[0x419500,0x419aa0)` | `mission_mp_exit_dialog_layer` | multiplayer exit dialog/state layer | Route UI/state work separately from objective runtime while preserving order. |
| `[0x419aa0,0x41ada0)` | `mission_net_game_setup_panel_layer` | net-game setup UI/input/overlay layer | Net-game setup panel, numeric/clamped inputs, and overlay owner layer. |
| `[0x41ada0,0x41b8d0)` | `mission_net_session_browser_dialog_layer` | MFC net session browser dialog layer | Authored dialog layer with MFC dependencies. |
| `[0x41b8d0,0x41b950)` | `mission_mfc_control_dtor_provider_layer` | MFC scalar-deleting destructor provider layer | CBitmapButton/CCheckListBox/CComboBox/CEdit scalar-deleting destructor glue is provider-shaped. |
| `[0x41b950,0x41bd80)` | `mission_player_remote_destroyed_callbacks_layer` | Player remote/destroyed-state semantic exception | Player remote/destroyed callbacks are physically in mission.cpp but not accepted as mission source ownership. |
| `[0x41bd80,0x41c0c0)` | `mission_net_exit_panel_layer` | net exit panel UI layer | Net exit panel constructor, widgets, global show/tick/destroy. |
| `[0x41c0c0,0x41c270)` | `mission_ai_property_dialog_layer` | AI property/debug dialog semantic layer | Debug/property dialog island physically in mission.cpp. |
| `[0x41c270,0x41c6e0)` | `mission_new_game_panel_layer` | new-game panel and overlay state layer | New-game panel and overlay owner layer. |
| `[0x41c6e0,0x41c980)` | `mission_net_session_config_dialog_head_layer` | net session config dialog head | Dialog constructor/destructor/data exchange/message map head. |
| `[0x41c980,0x41cbf0)` | `mission_net_session_config_maps_layer` | multiplayer map registration/name/init/change layer | Map registration and config dialog map-name lifecycle. |
| `[0x41cbf0,0x41cc10)` | `mission_mfc_spin_provider_tail_layer` | MFC `CSpinButtonCtrl` scalar-deleting destructor provider tail | Provider-shaped tail closes mission.cpp before `pickup.cpp`. |

#### `pickup.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names and
comments were used only as navigation labels; placement evidence is physical
order, assembly/callee shape, and the `pickup.cpp` literal xrefs at `0x41cd93`,
`0x41d523`, and `0x41db80`. This pass did not prove new emitted `.h` rows.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x41cc10,0x41ccf0)` | `pickup_subsystem_globals_teardown_layer` | pickup subsystem bootstrap | Pickup subsystem data/list init and shutdown helpers start the physical block. |
| `[0x41ccf0,0x41ceb0)` | `pickup_init_resource_registration_layer` | confirmed literal-bearing init/resource layer | Contains the `pickup.cpp` literal xref at `0x41cd93`; keep resource and archive registration in physical pickup order. |
| `[0x41ceb0,0x41cf30)` | `pickup_zclass_node_flag_recursion_layer` | pickup-specific zClass_Node semantic exception | Recursive flag helpers look zClass-shaped but use pickup-specific flag masks; do not route to zClass source solely from names. |
| `[0x41cf30,0x41d8a0)` | `pickup_collection_effect_player_grant_layer` | pickup collection/effect body | Contains the `Pickup::ApplyEffect` literal xref at `0x41d523`. |
| `[0x41d8a0,0x41dd60)` | `pickup_spawn_list_creation_register_layer` | spawn-list lifecycle/body layer | Contains the `AssignBvolGroupAndId` literal xref at `0x41db80`; BVol/zClass calls are dependencies, not source-placement proof. |
| `[0x41dd60,0x41de70)` | `pickup_type_lookup_weapon_slot_predicate_layer` | mixed pickup lookup and Net-style predicate | Includes `Net::IsOptEntryActiveInAnySlot` semantics, but current evidence does not prove a Net source split. |
| `[0x41de70,0x41e240)` | `pickup_puppy_zrd_spawn_loader_layer` | ZRD spawn loader layer | Puppy/ZRD spawn loading and pickup type metadata lookup. |
| `[0x41e240,0x41e780)` | `pickup_spawn_list_respawn_queue_layer` | runtime spawn-list maintenance layer | Terrain/VTOL selection and respawn queue update remain physically pickup.cpp. |
| `[0x41e780,0x41e890)` | `pickup_zar_archive_callback_layer` | Pickup ZAR callback layer | Archive callbacks are registered by pickup initialization; keep callback order with the physical block. |
| `[0x41e890,0x41ea90)` | `pickup_network_copy_drop_meta_tail_layer` | network/drop helper tail | Network-copy reconciliation, spawn accessors, drop/meta helpers, and carrier-node spawn command close the block before `player.cpp`. |

#### `player.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names and
comments were used only as navigation labels; placement evidence is physical
order, assembly/callee/provider shape, and the `player.cpp` literal xrefs at
`0x41f20b`, `0x41f870`, `0x42087a`, `0x420dc7`, `0x42155b`, `0x421722`, and
`0x42176f`. This pass did not prove new emitted `.h` rows.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x41ea90,0x41ecd0)` | `player_static_lists_hud_singleton_layer` | static player/HUD setup with semantic glue | HUD scalar-deleting/reset glue appears in physical player.cpp and needs complete-owner routing before byte claims. |
| `[0x41ecd0,0x41f010)` | `player_recorded_node_flag_restore_layer` | recorded node flag restore globals | Preserve the early player runtime state order. |
| `[0x41f010,0x41fb80)` | `player_mission_save_zar_layer` | confirmed literal-bearing mission save/ZAR layer | Contains the `player.cpp` literal xrefs at `0x41f20b` and `0x41f870`. |
| `[0x41fb80,0x420be0)` | `player_mission_runtime_zrd_bootstrap_layer` | mission runtime and ZRD bootstrap layer | Keep vehicle/ZRD helper placement unresolved until source-shape tests prove a split. |
| `[0x420be0,0x420d10)` | `player_zreader_checkpoint_exception_layer` | zReader/checkpoint semantic exception | Non-player semantics are physically emitted inside the player block. |
| `[0x420d10,0x421d60)` | `player_state_binding_spawn_construction_layer` | confirmed player state/spawn layer | Contains literal xrefs at `0x420dc7`, `0x42155b`, `0x421722`, and `0x42176f`. |
| `[0x421d60,0x421e20)` | `player_zclass_recursive_flag_exception_layer` | zClass recursive flag semantic exception | Preserve physical placement until source-shape evidence proves header or engine-source ownership. |
| `[0x421e20,0x421ea0)` | `player_zreader_path_helper_exception_layer` | zReader path helper semantic exception | Path helper source placement remains unresolved. |
| `[0x421ea0,0x423460)` | `player_model_collision_hud_fx_layer` | player model/collision/HUD/FX layer | Player model data, support points, HUD refresh, small vectors, and FX UI are interleaved. |
| `[0x423460,0x425920)` | `player_contacts_pickups_checkpoint_layer` | contacts/pickups/checkpoint layer | HUD sensor and checkpoint helper labels require complete-owner scrutiny, not automatic source reassignment. |
| `[0x425920,0x426390)` | `player_gameplay_input_callbacks_layer` | gameplay input callback layer | Includes HUD hotkey semantics inside physical player.cpp. |
| `[0x426390,0x429f10)` | `player_manager_tick_vehicle_dynamics_layer` | player manager and vehicle dynamics body | Dense movement and vehicle dynamics layer. |
| `[0x429f10,0x42a9f0)` | `player_zinput_binding_exception_layer` | zInput binding/default-map semantic exception | Assembly shows vector/static init, CString, zLoc, bind maps, and VC5 helper activity; no `.h` row proven. |
| `[0x42a9f0,0x42b6e0)` | `player_hud_counters_master_transition_layer` | counters/transition/audio/camera layer | HUD counters, debug overlay, master-type transitions, audio, and camera probes. |
| `[0x42b6e0,0x42bf90)` | `player_pose_async_restart_layer` | pose/async/restart layer with semantic helpers | cls_di and HUD powerup SFX semantics remain source-shape questions inside the player block. |
| `[0x42bf90,0x42db50)` | `player_environment_surface_orientation_layer` | environment/surface/orientation layer | Includes `0x42d560` `zMath::Vec3Midpoint` semantic helper; do not claim zMath header ownership yet. |
| `[0x42db50,0x42dda0)` | `player_zcom_connection_exception_layer` | zCom COM helper semantic exception | zCom connection/interface helpers are physical player tail exceptions. |
| `[0x42dda0,0x42de00)` | `player_wol_upgrade_exception_layer` | WOL API init semantic exception | Westwood Online upgrade API init helper closes the semantic tail before provider glue. |
| `[0x42de00,0x42de10)` | `player_safe_vtable_release_provider_tail_layer` | compiler/provider tail | SafeVtableRelease is provider-shaped tail before `RecoilApp.cpp`. |

#### `RecoilApp.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names and
comments were used only as navigation labels; placement evidence is physical
order, assembly/callee/provider shape, and the `RecoilApp.cpp` literal xref at
`0x42e620`. This pass did not prove new emitted `.h` rows and did not support a
`zWeapon` split inside the physical `RecoilApp.cpp` block. The
`[0x4301e0,0x431b50)` CZRecoilFrame layer is semantic class-owner evidence
inside the literal-backed `RecoilApp.cpp` physical block: BN has the
`D:\Proj\Battlesport\RecoilApp.cpp` path literal at `0x4dcb9c` and no observed
`CZRecoilFrame.cpp` or `CZRecoilFrame.h` path literal. Keep reconstruction
agents routed to the semantic `CZRecoilFrame` owner/source files for class work,
but preserve the surrounding `RecoilApp.cpp` physical order for VC5 emission
unless later BN evidence proves a separate translation-unit, header/COMDAT, or
include-emission boundary.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x42de10,0x42e170)` | `recoilapp_mfc_app_static_startup_layer` | MFC app/static startup layer | Application object, MFC message map/runtime class startup, and static initialization start the block. |
| `[0x42e170,0x42e220)` | `recoilapp_zinput_joystick_enable_exception_layer` | zInput semantic exception | Joystick enable helper is physically emitted in RecoilApp.cpp; do not move it from labels alone. |
| `[0x42e220,0x42f9f0)` | `recoilapp_engine_startup_state_machine_layer` | confirmed app startup/state-machine layer | Contains the `RecoilApp.cpp` literal xref at `0x42e620`; includes ordered zUtil, HUD, and EH helper exceptions. |
| `[0x42f9f0,0x4301e0)` | `recoilapp_zinput_force_feedback_exception_layer` | zInput DirectInput force-feedback semantic exception | Force-feedback helpers are physically in RecoilApp.cpp pending source-shape proof. |
| `[0x4301e0,0x431b50)` | `recoilapp_czrecoilframe_mfc_frame_layer` | semantic CZRecoilFrame class/runtime/menu/message-map layer inside physical `RecoilApp.cpp` | Preserve `CZRecoilFrame::*` semantic names and route class/source-owner work to `src/Battlesport/CZRecoilFrame.cpp`/`.h`, but do not split this as a separate literal-backed `CZRecoilFrame.cpp` physical block on current BN evidence. |
| `[0x431b50,0x431bf0)` | `recoilapp_mfc_provider_destructor_glue_layer` | MFC/provider destructor glue | CObject/CMenu destructor glue is provider-shaped and adjacent to the frame layer. |
| `[0x431bf0,0x434660)` | `recoilapp_gamenet_packet_relay_layer` | GameNet packet and gameplay relay layer | zDEClient, Pickup, OptCatalog, and effect relay semantics are dependencies/semantic exceptions inside the physical block. |
| `[0x434660,0x435a30)` | `recoilapp_saveload_dialog_file_list_layer` | save/load dialog and file-list layer | Save/load UI, file records, and file-list behavior remain in the RecoilApp physical order. |
| `[0x435a30,0x436630)` | `recoilapp_saveload_transition_sort_tail_layer` | save/load transition and sort/vector tail | Transition state and VC5 vector/sort helper activity close the block before `turret.cpp`. |

#### `AppFrame.cpp` / `CZGameFrame.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names and
comments were used only as navigation labels; placement evidence is the
current BN function order, assembly/xrefs/data records, and the provider
boundaries around `[0x442890,0x443c50)`. No local source-path literal was found
for `AppFrame.cpp`, `CZGameFrame.cpp`, or `CZGameFrame.h`. Treat
`src/CZGameFrame/CZGameFrame.h` as a likely declaration/type source-shape
input for `CZGameFrame.cpp`, not as an address-emitting `partial-header` row,
until VC5 order evidence proves emitted header bodies.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x442890,0x4428a0)` | `provider:mfc/ordinal-import-pointer-getter` | MFC provider getter | Returns the MFC42 ordinal 4274 import pointer. Keep as provider boundary. |
| `[0x4428a0,0x4428b0)` | `provider:mfc/ole-control-module-message-map-getter` | MFC provider getter | Linked `COleControlModule` base message-map getter. Keep as provider boundary. |
| `[0x4428b0,0x4429d0)` | `AppFrame-mfc-ole-app-shell-dtor` | app-shell destructor layer | `RecoilApp_MfcOleModule` destructor/scalar deleting destructor; destroys deque-like state-queue storage and chains to MFC. |
| `[0x4429d0,0x442c70)` | `AppFrame-main-window-engine-startup-shutdown` | app/frame startup and shutdown layer | Main-window setup, skip-wait flags, engine init/shutdown, startup-state queueing. |
| `[0x442c70,0x442d00)` | `AppFrame-mfc-ole-module-constructor` | app-shell constructor layer | CWinApp base construction and RecoilApp state-queue storage initialization. |
| `[0x442d00,0x443140)` | `AppFrame-main-run-loop-and-eh-funclets` | Run loop plus MSVC EH funclets | `0x44300b..0x4430f3` are `RecoilApp::Run` authored catch bodies plus compiler jumps, not standalone owners. |
| `[0x443140,0x443650)` | `AppFrame-state-transition-queue-api` | state-transition queue API layer | Current-state lookup and switch/push/exit queue APIs; repeated queue-growth mechanics stay physically here. |
| `[0x443650,0x443730)` | `AppFrame-idle-dispatch-and-state-queue-storage-helpers` | dispatch and queue storage helper layer | Idle/message dispatch plus `RecoilApp_StateQueue` chunk-list helpers before the `CZGameFrame` block. |
| `[0x443730,0x4437d0)` | `CZGameFrame-runtime-class-message-map-accessors` | CZGameFrame runtime/message-map layer | MFC create-object, runtime-class, and message-map accessors. |
| `[0x4437d0,0x4438c0)` | `CZGameFrame-lifecycle-and-window-validity` | CZGameFrame lifecycle layer | Constructor, scalar deleting destructor, destructor, and window-validity helper; scalar deleting destructor remains class-coupled glue. |
| `[0x4438c0,0x443900)` | `CZGameFrame-title-and-close` | CZGameFrame title/close layer | Window-title CString construction and close handler. |
| `[0x443900,0x443a20)` | `CZGameFrame-paint-dc-bitmap-blit` | CZGameFrame paint layer | CPaintDC lifetime and bitmap blit/stretch handler; MFC/GDI providers remain dependencies. |
| `[0x443a20,0x443a40)` | `CZGameFrame-size-handler` | CZGameFrame size handler | Calls CFrameWnd `OnSize`, then the cached-client-rect helper at `0x443a40`. |
| `[0x443a40,0x443a50)` | `zVideo-cached-client-rect-helper-island` | zVideo semantic exception inside physical `CZGameFrame.cpp` | Called only by `CZGameFrame::OnSize` and `CZGameFrame::OnMove`; calls `zVid::QueryCachedClientRectUpdateMaskIf3dfx` and tail-jumps to `zVideo::UpdateCachedClientRectScreenCoords`. Do not promote to a separate `.h`/`.cpp` block without source-shape order proof. |
| `[0x443a50,0x443ae0)` | `CZGameFrame-move-create-destroy-handlers` | CZGameFrame window lifecycle handlers | Move/create/destroy handlers with bitmap, input, network, video, and sound dependencies. |
| `[0x443ae0,0x443b70)` | `CZGameFrame-activate-and-idle-forwarding` | CZGameFrame activation/idle forwarding layer | Activation/deactivation calls through `RecoilApp` and input/video dependencies; message-map idle forwarding calls `RecoilApp::OnIdleOrDispatch`. |
| `[0x443b70,0x443be0)` | `provider:mfc/gdi-object-destructor-glue` | MFC/GDI provider destructor glue | `CGdiObject` scalar deleting destructor/destructor glue. |
| `[0x443be0,0x443c50)` | `provider:mfc/bitmap-destructor-glue` | MFC/GDI provider destructor glue | `CBitmap` scalar deleting destructor/destructor glue before `cls_di.c`. |

#### `turret.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. Current BN function names and
comments were used only as navigation labels; placement evidence is physical
order, assembly/callee/global shape, and the `turret.cpp` literal xref at
`0x437b25`. This pass did not prove an emitted `turret.h` row.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x436630,0x436e00)` | `turret_runtime_init_zrd_load_layer` | private `zTurret_Runtime` init/ZRD load layer | Runtime object is allocated as `0x180` bytes, uses ECX-self methods, and has no vtable evidence. |
| `[0x436e00,0x437aa0)` | `turret_runtime_frame_fire_damage_layer` | `zTurret_Runtime` per-frame/fire/damage layer | Preserve x87/vector/matrix mechanics and field-heavy runtime state shape. |
| `[0x437aa0,0x437d40)` | `turret_system_globals_load_tick_layer` | `zTurret_System` manager layer | Static manager over BSS globals and runtime list; contains source-path literal xref at `0x437b25`. |
| `[0x437d40,0x437d60)` | `turret_tick_callback_wrapper_layer` | authored tick callback wrappers | Thin tailcall wrappers around `zClass_Class::gwNodeSetActionCallback`, not provider-owned functions. |
| `[0x437d60,0x437dc0)` | `turret_damage_callback_adapter_layer` | callback adapter | ABI-sensitive adapter forwards callback data into runtime damage logic. |
| `[0x437dc0,0x437e50)` | `turret_runtime_system_teardown_layer` | manager/runtime teardown layer | Frees runtime list, loaded reader tree, and callback node state. |
| `[0x437e50,0x437e60)` | `turret_fire_animation_done_callback_layer` | callback adapter tail | Fire-animation callback ignores ECX callback object and forwards EDX runtime user pointer to `FireWeapon`. |

#### `WinSock.cpp` Source-Shape Layer Detail

These layer names are reconstruction routing labels stored in
`tools/_recoil/config/source_file_blocks.json`. This is a provisional no-literal
physical block: no current BN source-path literal was found for `WinSock.cpp`,
`NetUi.cpp`, or `Net.cpp`. Current BN comments are navigation labels only, and
`0x43cf90` is the next WOL physical block boundary.

| Range | Layer label | Classification | Reconstruction consequence |
| --- | --- | --- | --- |
| `[0x43ce80,0x43cf1c)` | `winsock_netui_version_prompt_layer` | NetUi Winsock prompt helper | Authored out-of-line Net/UI helper, but exact source filename is not literal-backed. |
| `[0x43cf1c,0x43cf20)` | `winsock_post_netui_alignment_layer` | padding | NOP alignment between NetUi helper and CRT stub. |
| `[0x43cf20,0x43cf31)` | `winsock_zstr_crt_init_stub_exception_layer` | `zStr.cpp` CRT init-table stub exception | Data xref only from `g_CrtInitFn_zStr_CrtInitStub @ 0x4da094`; do not model as authored WinSock/Net body. |
| `[0x43cf31,0x43cf40)` | `winsock_post_zstr_alignment_layer` | padding | NOP alignment before the formatter. |
| `[0x43cf40,0x43cf8b)` | `winsock_ipv4_formatter_layer` | Net IPv4 dotted-quad formatter | Authored out-of-line network utility with WOL-side caller; no emitted header row is proven. |
| `[0x43cf8b,0x43cf90)` | `winsock_pre_wol_alignment_layer` | padding | WOL starts at `0x43cf90`; do not pull WOL dialog code into this block. |

### Exception And Gap Ledger

Full BN membership sweep status, 2026-07-01:

- Nine BN fact-mapper slices inspected the full Recoil.exe `.text` catalog with
  explicit `Recoil.bndb` selectors: 154 physical blocks covering
  `[0x401000,0x4cb9e8)`, 4,950 BN function starts, and zero unclassified
  function starts.
- The machine-readable block catalog has no active `.text` coverage holes.
  Provider/import/runtime packets, padding, generated switch tables, no-literal
  authored islands, and semantic-owner exceptions are all explicitly mapped.
- The remaining work is source-owner and provenance reconciliation inside
  mapped blocks, not discovery of additional physical block boundaries.

Known exception classes from this pass:

- `0x402f60` `zMath::Vec3Normalize` is an address-backed `zMath.h` header
  COMDAT physically emitted in the `ai_net.cpp` block; semantic source is
  `src/GameZRecoil/zMath/zMath.h`.
- `ai_net.cpp` now has a dedicated body-layer table after the proven
  `ai_net.h` and `zMath.h` partial-header rows. The player-facing
  `0x403750` peer-ring helper is still physically in `ai_net.cpp`; current BN
  evidence does not justify moving it to `player.cpp` or creating another
  emitted header row.
- `map.cpp` physically contains 8 recorded source-shape layers, including the
  HudRectI/HudGeom2D clipping helper island. Current BN evidence does not
  justify moving those helpers to `hud.cpp` or creating a new emitted header
  row.
- `turret.cpp` physically contains 7 recorded source-shape layers. The
  runtime object is a private non-polymorphic state object by current assembly
  evidence; no emitted `turret.h` row or vtable/interface shape is proven.
- `WinSock.cpp [0x43ce80,0x43cf90)` remains provisional/no-literal. It now has
  6 recorded subranges, including `0x43cf20` as a zStr CRT init-table stub
  exception, and the WOL boundary remains `0x43cf90`.
- `AppFrame.cpp [0x4428b0,0x443730)` and `CZGameFrame.cpp
  [0x443730,0x443b70)` remain provisional/no-literal physical blocks between
  WOL and `cls_di.c`. `CZGameFrame.h` is recorded only as a likely
  declaration/type source-shape input, and `0x443a40`
  `zVid::UpdateCachedClientRectIfUpdateMaskEnabled` is a semantic zVideo
  helper island inside the physical `CZGameFrame.cpp` block, not a proven
  emitted header row.
- `Briefing.cpp` physically contains 11 recorded source-shape layers,
  including HUD widget/locator helpers, class-coupled scalar destructors,
  `0x403db0` `MSVC_STL::ListDestructor_COMDAT`, and `0x404140`
  `zInput::WaitForAnyKeyPressWithTimeoutMs`. Use the dedicated Briefing layer
  table above before accepting source-owner placement.
- `mission.cpp` physically contains 14 recorded source-shape layers, including
  HudSensorTracker lifecycle/objective runtime, multiple HUD/MFC network dialog
  layers, MFC provider destructor islands, Player remote/destroyed callbacks,
  an AI property dialog island, and the final `CSpinButtonCtrl` provider tail.
  Use the dedicated mission layer table before accepting source-owner placement.
- `Camera.c` physically ends at `0x44d600 zVideo_sw::RenderFrame`, not
  `zClass_Camera::RenderScene`. The zClass render traversal island
  `[0x44af60,0x44c1b0)` routes to `engine.zclass.render_traversal_dispatch`,
  while `[0x44d600,0x44d990)` routes to
  `render_video.zvideo_sw_frame_orchestration`; neither should be accepted as
  Camera-owner byte work without preserving physical placement.
- `cls_util.c` contains BBox and zClass/Class/Object3D/List tail islands:
  `[0x4525d0,0x452770)` BBox bounding-sphere helpers and
  `[0x452770,0x452920)` split Class/predicate/material-flag/node-DI updates.
  The source literal xrefs stop at `CopyNode`, so the tail is a physical-order
  fact and an owner-routing blocker.
- `Switch.c [0x452920,0x4529c0)` is physical Switch.c by literal evidence,
  but the two functions route through add/remove child dispatch owners rather
  than a standalone switch owner.
- `Seq.c [0x453ee0,0x454360)` physically includes Lod classdata/add-remove
  tail `[0x4542a0,0x454360)`. Sequence semantic ownership stops before that
  tail.
- `0x454360 zClass::ResetCurrentZbdPath` is now treated as the no-literal
  prefix of the literal-backed `cls_zbd.c [0x454360,0x4558f0)` physical block.
  No separate `cls_path.c` file is proven; do not promote it as an isolated
  source-path claim or move it out without header/COMDAT evidence.
- `zdec_crater.cpp [0x456ad0,0x458af0)` splits into crater feature/event/build
  core `[0x456ad0,0x4575f0)` and zDEClient feature/list/map-tree/camera tail
  `[0x4575f0,0x458af0)`. The tail's `zdec_init.cpp` source comments are
  semantic evidence only; the physical block literal remains zdec_crater.cpp.
- `0x44f630 zClass_List::RenderActiveCameras` is physically inside List.c by
  neighbor order and BN comments, though it has no local source-path literal
  push. It semantically belongs with the camera scene render pass: it walks
  bucket `8`, reads `g_zVideo_ActiveRendererPath`, and calls either
  `zVideo_sw::RenderFrame` or `zClass_Camera::RenderScene`. Current
  `List_RenderActiveCameras.cpp` placement should be scrutinized before any
  byte/source-path claim.
- `0x44f870 zClass::RemoveChildChecked` is physically Window.c by direct
  Window.c literal push and surrounding block order, despite a stale BN/source
  comment saying List.c. It is a zClass child-removal validation helper that
  tail-jumps to `zClass_Class::RemoveChildGeneric`; do not schedule it as a
  Window standalone owner.
- `0x4543a0 zClass::NodePtrToValidatedIndex` is physically a cls_zbd.c prefix
  emission next to `GameZ_ZBD::NodePtrToIndex` / `NodeIndexToPtr`, while
  semantically serving zClass/ZBD validation and zEffect save/queue callers.
  Direct cls_zbd.c literal xrefs begin after this helper, so the physical
  placement is by neighbor-order evidence rather than a local literal push.
- No-standalone helper candidates in the zClass/zDEClient packet include the
  `cls_util.c` callback-storage cast helper observed in `0x451900`, BBox radius
  approximation shared by `0x4525d0` and `0x452650`, Switch validation shared by
  `0x452920` and `0x452970`, render-traversal cull/render fragments in
  `0x44af60`, `0x44b140`, `0x44b300`, `0x44b710`, `0x44b8c0`, `0x44bea0`, and
  `0x44bfb0`, Lod bit-level sqrt/range/alpha fragments in `0x44b8c0`, and
  zDEClient map-tree inline helpers such as nil tests, minimum/maximum,
  rotations, header reset/transplant/refresh, and lazy init observed across
  `0x457d90`, `0x457e80`, `0x457fe0`, `0x458510`, and `0x4585a0`.
- `hud.cpp`, `RecoilApp.cpp`, `player.cpp`, `zModel/gmod_*`, `zRender`, and
  `zImage/zimg_texture.cpp` each contain large semantic-owner mismatch fields.
  These mismatches are not proof of provider or header ownership by themselves;
  they are blockers for owner acceptance until source-shape evidence resolves
  whether the functions are headers, included helpers, COMDATs, or original
  same-translation-unit definitions.
- `0x4a59d0` is `KERNEL32_GetTickCount_ImportThunk` inside the mapped
  `[0x4a53f0,0x4a5c20)` no-literal mixed island and is provider/import, not
  authored code.
- `0x48bf10 zUtil::CopyDwordRange` is not authored zUtil/zNetwork source.
  Current BN evidence classifies it as a VC5 STL/compiler-header COMDAT helper
  selected into `znet_dplay.cpp`: generic dword copy body, `__stdcall ret 0xc`,
  no data refs/callees, and shared callers from HUD, zInput, zNetwork, and
  zSound vector insertion expansions. Sibling helpers `0x40c190` and
  `0x40c1c0` support the same provider/compiler family.
- `0x403db0 MSVC_STL::ListDestructor_COMDAT` is a Briefing.cpp physical
  exception and provider/compiler-header COMDAT by current BN evidence. It
  has zZbdSectionHandlerList semantic use through callers, but assembly/xrefs
  show the shared VC5 `<xlist>` destructor shape rather than authored Briefing
  or zZbd source ownership.
- `0x42d560 zMath::Vec3Midpoint` and `0x446ed0 BBox::ExpandToCorners` are
  address-backed semantic helper exceptions in `player.cpp` and `cls_di.c`,
  respectively. They are not no-standalone inline helpers by current evidence;
  source reconstruction must decide whether the original path was header,
  same-TU helper, or separate source owner before accepting source-path gates.
- `zmth_main.c [0x472670,0x475c40)` is the literal-backed physical zMath
  block. Current source comments/source map and some owner metadata still use
  `zMath.cpp`, `zmath_vec3.cpp`, `zmath_matrix.cpp`, or similar pseudo-file
  labels; those are stale for physical source-file ordering unless a separate
  header/COMDAT/include path is proven. Semantic zMath exceptions outside the
  block include `0x402f60`, `0x42d560`, `0x476480`, `0x490330`, and the later
  zMath Z-clip helpers in the mixed HUD/zTimedTask band.
- Focused single-row audit refined `zmth_main.c [0x472670,0x475c40)` into 10
  semantic math subranges without changing the physical file row: Vec3
  primitives `[0x472670,0x472d30)`, CRT matherr/literal anchor
  `[0x472d30,0x472ed0)`, matrix stack/current-load helpers
  `[0x472ed0,0x473370)`, affine matrix transforms `[0x473370,0x473e60)`,
  camera/local-TRS/Euler setup `[0x473e60,0x4743e0)`,
  projection setup and transform batches `[0x4743e0,0x474b20)`,
  project/unproject/angle helpers `[0x474b20,0x474e10)`,
  Euler/vector geometry helpers `[0x474e10,0x475210)`,
  collision/interpolant helpers `[0x475210,0x4757c0)`, and quaternion helpers
  `[0x4757c0,0x475c40)`. The `zmth_main.c` literal xref remains in
  `zMath::CrtMatherrHandler`; no `zMath.cpp` source-path literal was found.
- zMath no-standalone helper candidates observed in the physical block include
  `Subtract` and `Dot` in `0x4753e0`/`0x475210`, `Cross` in `0x4753e0`,
  `NegateFloatSignBit` in `0x473e60`, `Add`/`Scale`/`BuildUvOverZPlane` in
  `0x4753e0`, and `TransformBBoxCorner` in `0x474870`. Treat these as helper
  evidence under complete zMath owners, not standalone retail functions.
- `0x4a3ef0` `zSnd::ReportA3DError` and `0x4a4330`
  `zSnd::ReportDirectSoundError` are an unresolved source-provenance conflict:
  BN function comments say `zsnd_create.cpp`, while current owner/source map
  route them to `zsnd_init.cpp`. Treat them as address-backed diagnostic
  helpers pending owner scrutiny, not as evidence for either source-file block.
- `zsnd_3d.cpp [0x4a2950,0x4a2ea0)` is now mapped from the effective source
  path at `0x4e2428`: the base string object begins at `0x4e2426` with two
  prefix bytes, but BN xrefs `0x4a2e20` and `0x4a2e48` target the effective
  `D:\Proj\GameZRecoil\zSound\zsnd_3d.cpp` path inside
  `zSndPlayHandle::Update3D` diagnostics. Treat the row as stronger than a
  no-xref candidate, while preserving the broader zSound semantic-owner
  scrutiny for diagnostics and shared sound helpers.
- `zSnd::ReportMciError` is semantically `zsnd_cd.cpp` but physically in the
  no-literal report band `[0x4a3ea0,0x4a3ef0)`, after the literal-backed
  `zsnd_cd.cpp [0x4a2010,0x4a2950)` block. The adjacent A3D/DirectSound report
  helpers `[0x4a3ef0,0x4a44c0)` remain source-provenance blockers. A focused
  BN pass found no `zsnd_error.cpp` or `zsnd_report.cpp` literal; the catalog
  uses `zsnd_error.cpp` only as an invented physical reporter-shelf label.
- `0x4a3ea0 zSnd::ReportMciError` consumes caller-provided source file/line,
  calls `mciGetErrorStringA`, formats `%s(%d): MCIError [%s]\n`, and all direct
  callers pass the `zsnd_cd.cpp` literal. It is authored, not provider/import,
  but its no-literal physical placement after the fade-list block remains a
  header/static/late-emission caveat.
- `0x4a3ef0 zSnd::ReportA3DError` and
  `0x4a4330 zSnd::ReportDirectSoundError` consume caller-provided source
  file/line, call authored `zError::ReportOldNoOp`, and are best tied to
  `zsnd_create.cpp` by BN comments and adjacent diagnostic string/table layout.
  Their callers span `zsnd_play.cpp`, `zsnd_init.cpp`, `zsnd_create.cpp`, and
  for DirectSound also `zsnd_parm.cpp` and candidate `zsnd_3d.cpp`; this leaves
  external authored helper versus selected inline/header emission unresolved.
- Focused single-row audit refined three previously one-row zSound/zVideo
  blocks without changing their physical placement: `zsnd_parm.cpp
  [0x4a10e0,0x4a12c0)` splits into play-handle frequency/scale parameters,
  sample/event managed toggles, and active-backend preinit accessors;
  no-literal `zsnd_fade.cpp [0x4a3930,0x4a3ea0)` splits into fade lifecycle,
  dispatch/update, active-list tick, and cleanup/list cursor helpers;
  no-literal `zsnd.cpp [0x4a53f0,0x4a5670)` remains a compact zSndWaveData
  class island split into lifecycle, WAVE parser, and lazy load/reset/archive
  wrapper phases. `zvid_buff.c [0x4a69c0,0x4a6b40)` is literal-backed at
  `0x4a6b26` and splits into a tiny coordinate clip helper followed by the
  clipped blit body.
- Current BN does not prove an address-emitting `.h` contribution for the
  zSound report helpers. The three helpers have single retail addresses rather
  than per-translation-unit inline/header clones; any header role is currently
  declaration/macro context only.
- `0x4a4248..0x4a432c` is the jump table owned by
  `zSnd::ReportA3DError`, followed by NOP padding `[0x4a432c,0x4a4330)`. It is
  attached compiler switch data for the helper, not a primary data owner.
- Address-backed file-local/static predicate helpers in the zSound/zUtil band
  include `0x4a44e0`, `0x4a4c40`, `0x4a51e0`, `0x4a5220`, and `0x4a5da0`.
  They have retail addresses and are not no-standalone inline candidates unless
  later evidence proves header/COMDAT treatment.
- No fully inlined/no-standalone authored helpers were proven in
  `[0x4a3930,0x4a44c0)`, `[0x4a53f0,0x4a5c20)`, or
  `[0x4a66e0,0x4a69c0)`. Small zVideo surface/access/dispatch functions in
  the prelude have standalone retail addresses and remain address-backed
  functions unless later evidence proves a header/inline emission exception.
- `0x471de0` `zInput::PollActiveDevices` is physically inside the
  literal-backed `zin_init.cpp` block `[0x4719e0,0x471e40)`, making it the
  block's last BN function before the joystick tail starts.
- zInput no-standalone helper evidence in `[0x470020,0x472670)` is limited:
  likely class-body `zInput_BindMapContext` constructors appear as repeated
  allocation plus `InitFromTemplate` caller bodies at `0x4710a0` and
  `0x471860`; `IsUnsuspended(flags)` is repeated inside `0x471c60`,
  `0x471c70`, and `0x471c80`; joystick axis config and DI error-name mapping
  are repeated/inlined inside address-backed functions, not separate retail
  functions.
- `0x472450` and `0x472480` remain a source-path conflict: physical placement
  is the `zin_joystick.cpp` tail, while source map/docblocks call them
  `zin_ff.cpp`. `0x472490` should be scrutinized before accepting standalone
  owner shape because current callers are from keyboard/init paths, not the
  local joystick tail.
- `0x44300b..0x4430f3` is mixed: `0x44300b`, `0x443032`, and `0x4430cc`
  carry authored `RecoilApp::Run` fatal-dialog/catch policy, while `0x4430f3`
  is pure compiler epilogue. Do not classify the full range as provider.
- In `[0x437e60,0x443c50)`, no local source-path literal was found for the WOL,
  CZGameFrame, HudUi, zUtil, zVideo, NetUi, zStr, or RecoilVersion islands.
  These names are owner/comment/source-map provenance, not literal-backed
  translation-unit blocks.
- Likely no-standalone helpers in `[0x437e60,0x443c50)` are narrow: `ClampUnit`
  appears inside Object3D model-ref lerp code, and `PlayerFloatFromBits` /
  `PlayerClamp01` appear in player modal savegame SFX code. The WOL,
  RecoilApp, CZGameFrame, zVideo, zWeapon, pickup, and zUtil rows are
  address-backed semantic islands or provider/compiler artifacts, not proven
  fully inlined helpers.
- `0x4ba470`, `0x4ba4d0`, and `0x4ba510` are VC5 STL/template
  COMDAT/header provider boundaries inside the HUD-vector band, now named
  `MSVC_STL::PtrVector_FreeBufferAndReset_COMDAT`,
  `MSVC_STL::VectorVoidPtr_EraseRange_COMDAT`, and
  `MSVC_STL::VectorHudUiFlashPanelPtr_InsertN_COMDAT` in BN. `0x4bcb48` is
  padding.
- `0x49f614..0x49f620` is padding/fallthrough at the front of
  `zsnd_play.cpp`; BN now names it `padding_zsnd_play_pre_tick_0x49f614`.
  It is a block marker, not an authored standalone source owner.
- `0x4a6b60..0x4a6b80` are tiny `zvid_dd.c`/`zvid_ddd3d.c` state-setter
  semantic exceptions physically emitted inside `zvid_init.c`.
- `0x4ad6a0` `zVideo_dd::ReportError` is an address-backed `zvid_dd.c`
  diagnostic helper physically emitted at the tail of `zvid_ddd3d.c`.
  `0x4ae1ec..0x4ae380` are compiler switch/lookup tables for that function.
- `0x4b3020` `zSys::CheckMmxSupport` and `0x4bd800`
  `zMath::ClipLineSegmentPointToZ` are true standalone exceptions by current
  owner evidence. Other functions in `[0x4b2960,0x4c0d20)` belong to larger
  source owners even when their physical island is small.
- `0x4bdb60..0x4bdc70` and `0x4bed30..0x4bf060` are two physical islands of
  `render_video.zvideo_fxpass3_ui_local_config`; do not schedule or verify
  either island as a standalone owner.
- Focused BN fact mapping refined the late HUD/FxPass3 shelf without changing
  physical file names: `0x4bde20..0x4bde40`, `0x4be2c0..0x4be2e0`, and
  `0x4be850..0x4be870` are scalar deleting destructor glue tied to authored
  HUD weather vtables, and `0x4bee40..0x4bee80` is static-init/atexit glue tied
  to `g_zVideoFxPass3ConfigLocal`. These are compiler/glue semantic subranges
  inside the provisional `zUI/zui.cpp` physical shelf, not proof of
  address-emitting header rows or exact original filenames.
- Focused BN fact mapping also reviewed the `zUI/zui_*` and
  `zWeather/zweather_*` filename hypotheses for `[0x4b3ce0,0x4bffe0)`.
  Current evidence supports semantic routing only: most of
  `[0x4b3ce0,0x4bd470)` and `[0x4bf060,0x4bffe0)` behaves like reusable
  UI/widget/control/dialog/background code, making `zUI/zui_*` the preferred
  hypothesis for those authored bodies, while `[0x4bdc70,0x4bed30)` is a
  weather-Fx class cluster and should test first as
  `src/GameZRecoil/zWeather/zweather_fx.cpp`. No `zUI`, `zHud`, `zWeather`,
  `zhud`, or `zweather` source-path literal is known for the shelf. A naive
  physical split would also interleave the `render_video.zvideo_fxpass3_ui_local_config`
  owner on both sides of the weather owner, so keep `zUI/zui.cpp` as the
  provisional placement label until object/link/order evidence or a VC5 natural
  source-shape reproduction proves a better physical split.
- Caller-origin review for `[0x4b3ce0,0x4bffe0)` strengthens the single-shelf
  model but does not accept a physical filename: BN string searches found no
  `zUI`, `zHud`, `zWeather`, `zhud`, `zweather`, or `zui` source-path literal.
  The tail functions are reached by earlier UI code (`0x4b463e -> 0x4bf840`,
  `0x4b95a6 -> 0x4bfc80`, `0x4ba2bd/0x4ba2ef -> 0x4bffb0`), which fits later
  definitions in the same provisional `zUI/zui.cpp` shelf better than a proven
  new `.cpp` block. The provider/helper exceptions are also high-confidence:
  `0x4ba470..0x4ba4a0`, `0x4ba4d0..0x4ba510`, and `0x4ba510..0x4ba740` are
  VC5/STL/provider COMDAT rows, while `0x4bd470..0x4bdb60` is a
  zTimedTask/HudLineClip/zMath helper island and not a proven header or
  separate source file.
- `0x4c06f0` is CRT `tmpfile` provider inside the zUtil/zutl_zbd recursive cluster.
- `0x403db0` `MSVC_STL::ListDestructor_COMDAT` is a provider/compiler-header
  COMDAT physically emitted in `Briefing.cpp`; its zZbd semantic use is through
  callers, not authored zZbd source ownership.
- `0x4c5a50..0x4cb9e8` is no longer unattributed: it splits into provider
  MFC/import/CRT/DirectInput data, 8 bytes of DirectInput-provider padding at
  `[0x4c7408,0x4c7410)`, candidate zImage and zVideo partial-header
  contributions at `[0x4c7f00,0x4c7fd0)` and `[0x4c7fd0,0x4c81c0)`,
  the authored `WinMain` wrapper at `[0x4c81c0,0x4c81d8)`, and a final
  MSVC C++ EH funclet/handler tail.
- Focused BN fact mapping of the late zImage/zVideo candidates found semantic
  comments naming `.cpp` files, but no local source/header literal proving
  late physical placement or the `WinMain` host. Keep `[0x4c7f00,0x4c81c0)`
  as candidate `zimg_fonts.h`/`zvid.h` contributions included through
  `src/WinMain.cpp` until VC5 order tests prove or reject that source shape.

No currently known Recoil.exe `.text` range remains unattributed in the
machine-readable block catalog. The remaining open work is owner/source-shape
provenance and inline-helper scrutiny inside already mapped ranges. The
machine-readable unresolved-provenance list is now limited to
`[0x437e60,0x443c50)`, `[0x4a3ea0,0x4a44c0)`,
`[0x4a53f0,0x4a5c20)`, `[0x4a66e0,0x4a69c0)`,
`[0x4b2960,0x4c0d20)`, and `[0x4c7f00,0x4c81c0)`.
Other no-literal rows such
as zInput `[0x470020,0x472670)` and the zSound fade-list shelf
`[0x4a3930,0x4a3ea0)` are
bracketed/order-backed physical placement labels with explicit semantic
subranges, not unattributed gaps.
Literal-backed but source-metadata-conflicted blocks
also remain open for owner/source reconciliation, especially
`zmth_main.c [0x472670,0x475c40)`, zDEClient `zdec_qsand.cpp`/`zdec_crater.cpp`
tails, zEffect's `zeff_anim_*`/`zeff_init.c` split, and FMV's
`fmv_main.cpp`/`fmv_script.cpp`/`fmv_stream.cpp` split. These should be treated as mapped physical order facts
but not accepted source-path gates until their original TU/header/provider
provenance is proven.

For `[0x4b2960,0x4c0d20)`, current BN evidence supports exact physical
sub-blocks but not literal-proven `.cpp` translation-unit blocks. Treat it as a
special no-literal late/out-of-band shelf between literal-backed `zwep_init.c`
and `zinterp_parse.cpp`, not as proof that the global alphabetical folder model
continues through the band. Rebuild and verify complete source owners that may
be physically split or interleaved:
`engine.zgame.options_registry_option_list`, `engine.zgame.options_runtime_config`,
`engine.zsound.zsnd_init_backend_init_slice`,
`engine.zsys.cpu_feature_detection`, `engine.zsys.cpu_benchmark_resolver`,
the relevant HUD class/record owners, `hud_ui.ztimed_task_active_list_runtime`,
`hud_ui.hud_sensor_tracker_map_clip_geometry` with separate zMath owners,
`render_video.zvideo_fxpass3_ui_local_config` across both islands,
`ui.zhud.hud_weather_fx_class_family`, and the zZbd manager/section-handler
owners. Do not use the no-literal physical sub-blocks themselves as primary
tier `S` units unless current owner evidence proves a true standalone owner.

## Audit Procedure

Before accepting a source owner in this band or attempting byte matching for a
known source-file block:

1. Recover the physical source-file block from file-path literal xrefs,
   neighboring BN function order, and current source-block metadata before
   accepting semantic names.
2. Classify every semantic exception inside the block: headers, inline helpers,
   provider/compiler COMDATs, static helpers, compiler artifacts, padding, and
   source-owner islands.
3. Load any block `source_shape_inputs` and inspect any neighboring
   `partial-header` rows with `python tools/recoil.py audit source-blocks --list`.
   Declaration-only/type-only header contributors are required source-shape
   inputs for the block; emitted header contributors are address-ordered
   placement rows whose code belongs in the header `source_path` and compiles
   through `included_in`. Neither form is owner-gate/tier evidence by itself.
4. Check `python tools/recoil.py owner show <address-or-owner>` and
   `python tools/recoil.py owner relationships <address-or-owner> --json`.
5. Recreate the likely original `.h`/`.cpp` include and declaration layering:
   declaration-only/type-only/full-body headers, static/member/helper placement,
   and include timing that naturally causes VC5 to emit the retail order.
6. Reject forced placement. Do not move a helper into the wrong `.cpp`, add new
   `.inl` production files, use pragma/linker/order tricks, or treat artificial
   order matching as owner/tier evidence.
7. Verify VC5 function order with emitted function-order checking when
   available. If order differs, keep it as a source-shape/include-shape blocker.
8. Only after order matches naturally should remaining mismatches be treated as
   function-body, source-expression, data, or provider byte-diff problems.
9. Block or downgrade stale positive gates before byte verification if the
   current owner depends only on a stale source docblock or semantic BN name.
10. Move source bodies, headers, functional targets, and VC5 manifests only after
   the corrected source-shaped owner is proven.
