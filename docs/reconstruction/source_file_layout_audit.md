# Source File Layout Audit Notes

Binary Ninja remains authoritative for function bodies, calls, data, and
assembly. The generated `source_file_map.md` is only a navigation aid built from
current source docblocks and legacy comments; it can preserve stale source-path
claims when the implementation has been placed in the wrong file.

This audit is now the first Recoil.exe binary-lane scheduling surface while
source-file block boundaries remain incomplete. `owner next --lane binary` and
`audit backlog --lane binary` surface `work_unit=source-file-block-map` before
`final-repro`/`final-data-layout`; final linked `.data` layout remains a final
validation lane unless block-order work exposes a direct data/object ordering
cause.

## `zError::ReportOldNoOp` Source-File Literals

Retail `zError::ReportOldNoOp` call sites pass compiler-emitted source-file
path literals. For Battlesport files these literals appear in alphabetical
source order in `.rdata` and provide a strong file-placement constraint. The
current evidence supports a translation-unit contribution model: the compiler
emitted each original source file as a physical block of `.text`, then the next
source file's functions followed. Agents should use this block order when
recovering source owners, before trusting semantic names or stale source-path
comments. The reconstruction objective for a known block is not only behavior
equivalence: the rebuilt VC5 object should emit the covered functions in the
same order as the retail/reference `.text` order seen in Binary Ninja before
the owner is treated as byte-ready.

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

Current block evidence:

- The physical `ai_net.cpp` contribution starts at `0x401060` and runs through
  `AINet::FreeAll` at `0x403870`.
- The following `Briefing.cpp` contribution starts at `0x4038a0` and contains
  the later confirmed `Briefing::StartForMission` file-literal call at
  `0x404242`.
- `zMath::Vec3Normalize` at `0x402f60` is a semantic `zMath` inline helper
  declared by the common zMath header and defined by a later included zMath
  `.inl`; its COMDAT is physically emitted inside the probable `ai_net.cpp`
  block between `AINet::AiFinalizeMode2State1ForAllPlayers` at `0x402f10`
  and `AINet::LoadAllFromZrd` at `0x402fd0`. Such header/helper placement is
  an exception to source-owner naming, not a reason to discard the block model.

## Current Repair State

The first durable repair is conservative:

- `0x403750` was split out of the later Player bootstrap owner into
  `battlesport_ai.ainet_peer_ring_build`; its source body now lives in
  `src/Battlesport/ainet.cpp`, but owner-boundary scrutiny and byte drift still
  block acceptance.
- Early Player AI-mode owners rooted at `0x401060`, `0x402080`, `0x402250`,
  and `0x402be0` no longer have accepted boundary/source/linkage gates or tier
  `B`; their functional evidence remains useful as tier `C` evidence only.
- `battlesport_gameplay.player_ai_mode2_tuning_globals` keeps its data-byte
  evidence, but source/linkage ownership is blocked until the data packet is
  remapped or re-proven against the corrected owner.
- `legacy.battlesport_gameplay.subsystem_ai` metadata and source body for
  `0x403830` now route through `src/Battlesport/ainet.cpp`; the broader owner
  gates remain blocked until the adjacent early AI/player placement audit is
  finished and positive gates are re-scrutinized.
- Latest VC5 `ainet_text_block_order` evidence includes the
  `zMath_vec3_normalize.inl` `0x402f60` COMDAT in the emitted block order
  and the covered COFF section order now matches the retail-order manifest,
  but the full block still fails byte verification: `13/40` functions
  byte-match. `0x4016a0` and `0x403830` byte-match; `0x401a40` and
  `0x401ab0` each have 7 register-allocation mismatches; `0x402f60` has 89
  mismatches after 4 relocation bytes and 10 trailing VC5 NOPs with a local
  VC5 frame-pointer pragma matching the retail EBP frame but not the saved
  `ebx`/`esi`/`edi` prologue or direct x/y/z square schedule; `0x403750`
  now byte-matches in the block-order diagnostic.

Current top-down Battlesport source-path literal queue:

- `ai_net.cpp`: literal `0x4da1e8`, referenced at `0x4030bb`; proven block
  `0x401060..0x403870`.
- `Briefing.cpp`: literal `0x4da32c`, referenced at `0x404238`; next block
  starts at `0x4038a0` and still needs a durable end boundary plus exception
  classification.
- `hud.cpp`: literal `0x4dadd8`, referenced at `0x4101a3` and `0x4141bb`;
  audit after `Briefing.cpp`, with expected zHud/HUD semantic-owner exceptions.
- `map.cpp`: literal `0x4daf04`, referenced at `0x416922`.
- `mission.cpp`: literal `0x4db230`, referenced at `0x417fc2`, `0x4181b6`,
  `0x418209`, `0x4182ff`, `0x418395`, `0x419091`, and `0x419304`.
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

## Audit Procedure

Before accepting a source owner in this band or attempting byte matching for a
known source-file block:

1. Inspect `zError::ReportOldNoOp` xrefs and xrefs to the relevant file-path
   literal strings.
2. Enumerate neighboring functions with Binary Ninja and identify the physical
   source-file block boundaries before accepting semantic names.
3. Check `python tools/recoil.py owner show <address-or-owner>` and
   `python tools/recoil.py owner relationships <address-or-owner> --json`.
4. Classify helper/header/provider/COMDAT exceptions explicitly, especially
   when a function's semantic owner differs from its physical emitted block.
5. Shape source declarations, static helpers, member definitions, and
   header/`.inl` include points so VC5 emits functions in the retail BN order.
   Do not move a helper into the block's `.cpp` unless current evidence proves
   that was the original authored source shape.
6. Use a VC5 manifest with emitted function-order checking, when available, to
   compare generated COFF section order against the retail-order manifest before
   treating byte mismatches as the only remaining blocker.
7. Block or downgrade stale positive gates before byte verification if the
   current owner depends only on a stale source docblock or semantic BN name.
8. Move source bodies, headers, functional targets, and VC5 manifests only
   after the corrected source-shaped owner is proven.
