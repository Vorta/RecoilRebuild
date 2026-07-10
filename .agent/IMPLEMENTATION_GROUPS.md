# Implementation Groups

Use this tracked file for temporary dependency-group notes during
reconstruction. `.agent/SOURCE_OWNERS.json` is the durable owner-scope ledger;
this file lists only active multi-function, source-readiness, owner, or data
groups currently being coordinated. Pure tier `S` verification groups are active only after
`tier_s_priority_ready=true` or explicit user direction. Active groups are the
default no-address startup queue: new agents should resume actionable WIP here
before selecting new work with
`python tools/recoil.py owner next --lane binary`. Keep the header and template
available even when no groups are active.

## Rules

- Create or update a group before editing when a task touches more than one
  function or a shared type/global/vtable.
- Create or update the matching source-owner record with `python tools/recoil.py
  owner ...` before accepting owner source/data gates or tier `B`/`A`/`S`
  owner status. This file is not source-owner evidence.
- When launching without a user-specified address or source group, inspect
  active groups first and resume the first actionable one. Start unrelated new
  work only when active groups are absent, stale, contradicted, completed, or
  explicitly deprioritized by the user.
- Keep groups scoped. Prefer one class, one source file cluster, one recursive
  cycle, or one call-chain frontier.
- Do not claim address or owner completion from this file alone. SOURCE_OWNERS
  gates and tiers still require current source/build/Binary Ninja evidence.
- Keep notes concise and temporary. Move durable facts into source comments,
  Binary Ninja comments, tests, `docs/reconstruction/`, or narrow subsystem
  docs before pruning.
- Verification-only queues that no longer carry source, owner, or data blockers
  should not live in this active working file while global owner/data blockers
  remain. Use `.agent/SOURCE_OWNERS.json`, `python tools/recoil.py status
  0xNNNNNN`, VC verification manifests, and `python tools/recoil.py audit
  backlog --lane binary --include-deferred-verify` for deferred verification
  state.
- Normal binary-lane planning prioritizes owner structure blockers before
  isolated implementation/behavior work and prioritizes global owner/data
  blockers before verify-only tier `S` work. Active verify-only groups should
  condense or move out of this file while any authored owner source/data gate
  remains blocked or pending.
- Recompute verification scope with `python tools/recoil.py status 0xNNNNNN`
  or `python tools/recoil.py frontier 0xNNNNNN --depth 1` after source blockers
  clear.
- Use `python tools/recoil.py audit groups --summary --wip-limit 4` to check
  for stale, completed, or overgrown groups.
- Use `python tools/recoil.py audit handoff --path .agent/IMPLEMENTATION_GROUPS.md
  --strict` before launching workers from live handoff blocks.

## Active Group Template

```text
### Group: short descriptive name

- Anchor: 0xNNNNNN Name
- Owner id:
- Section:
- Queue: ready owner/data work / blocked pending evidence or policy / shared blocker / deferred verify-only debt
- Reason: dependency closure / class cluster / recursive cycle / shared ABI layout / source file cluster
- Source blockers:
  - 0xNNNNNN Name
- Next action:
  - python tools/recoil.py status 0xNNNNNN
```

For explicit user-directed verify-only work, add both canonical lines below;
omit both for ordinary groups:

```text
- Scheduling exception: explicit-user-direction
- Scheduling exception evidence: YYYY-MM-DD user-direction summary
```

## Source Worker Handoff Template

```text
### Parent batch card: short batch name

- Task kind: active WIP / address-led owner-data work / validation handoff
- Active group or address:
- Evidence packets required:
- Evidence packets received:
- Worker allocation:
- Validation scope:
- Exit criteria:

### Source-worker handoff: short scope name

- Section:
- Owner/source scope:
- Owner id:
- Anchor addresses/groups:
- Allowed write paths:
- Forbidden paths:
- Evidence inputs:
  - BN fact packet:
  - source-owner packet:
  - provider/data packet:
  - scaffold audit packet:
  - workspace/librarian packet:
- Expected source model:
- Validation commands:
- Return packet:
  - changed files
  - evidence used and caveats
  - commands run with pass/fail
  - blockers and overlap warnings
  - non-authoritative owner gate/tier recommendations only
```

## Verifier Handoff Template

```text
### Verifier handoff: short scope name

- Section:
- Validation scope:
- Anchor addresses/groups:
- Exact commands:
- Evidence inputs:
  - source worker packet:
  - BN fact packet:
  - provider/data packet:
- Forbidden paths:
- Return packet:
  - exact command lines
  - pass/fail results
  - key output lines
  - failure category
  - next narrow verification command
```

## Active Groups

Active queue sections:

- The former zVideo renderer dispatch/global owner audit has been pruned
  because `audit groups --summary --wip-limit 4` classified it as verify-only
  debt after the 0x42e330, 0x48ff70, 0x4a75f0, and zRndr span-occlusion data
  blockers were promoted to tier B.
- Deferred verify-only debt stays in the plan and VC manifests while
  `tier_s_priority_ready=false`; do not recreate an active WIP group for pure
  code/function tier S work unless the user explicitly directs tier S work.

### Group: ai_net.cpp address-order byte closure

- Anchor: 0x401060 AINet::TickAiMode2TopLevel
- Owner id: battlesport_gameplay.player_ai_mode2_top_level_steering; linked block owners engine.zmath.vec3_normalize_header_helper, battlesport_ai.ainet_find_by_net_id_lookup, battlesport_ai.ainet_peer_ring_build, and legacy.battlesport_gameplay.subsystem_ai
- Section: battlesport.ai
- Queue: active WIP under explicit user direction
- Scheduling exception: explicit-user-direction
- Scheduling exception evidence: User directed all `ai_net.cpp` functions to byte-match one-by-one without skipping, in retail address order, on 2026-07-09.
- Reason: complete literal-backed physical source-file block diagnostic over `[0x401060,0x4038a0)` with natural `ai_net.h` -> `zmth.h` -> `ai_net.cpp` VC5 contribution order
- Source blockers:
  - 0x401710, 0x401970, 0x401d50, 0x401e50, 0x401f60, 0x402090, 0x402170, 0x402250, 0x4024a0, 0x4026d0, 0x4028c0, and 0x403620 have current unmasked VC5 byte drift.
  - 0x401580 is source-shaped and zero-mismatch as of 2026-07-10: ordinary C++ emits the prefix, negative path, pre-tested reverse-edge scan, direct calls, merge, and probe-fan binding; only the recurring grouped-x87 vector subtraction remains narrowly allowlisted assembly. The result is 283 retail instruction bytes, a 288-byte VC5 extent, five NOPs, a 16-byte frame, 12 relocation-masked bytes, exact compiler shell, and natural order.
  - Its three direct REL32 fields are +0x2d to ordinary member `AINetNode::Free()` and +0x5b/+0xc3 to ordinary class-static `AINet::AiChooseNextPathBranchIndex`; 0x4016a0 and 0x4037c0 also remain zero-mismatch under those ordinary source spellings. This is bounded address/block byte evidence and does not accept the larger owner boundary/source/data/linkage/byte gates or tier S.
  - Durable VC5 loop-ordering evidence and the reusable hard-byte differential ChatGPT Pro prompt pattern are recorded inline in `docs/reconstruction/verified_patterns.md`; retain only the zero-byte checkpoint here while this ordered block group remains active.
  - Wrapper/alias, indirect-call, `_emit`, linker-alias, naked-function, scan/whole-function assembly, `.asm`, and linker/order tricks remain forbidden. The exact whole-body diagnostic macros, synthetic scratch overlay, and VC5-only diagnostic helper/free spellings were removed after the C++/minimal-x87 shape matched.
  - BN reconstruction confirmed `0x401964..0x401970` is twelve unreferenced padding NOPs after the 0x401710 switch table; the database and durable source-layout audit now record the former forwarder label as an analysis artifact.
  - The separate exact `src/Battlesport/ai_net.h` 0x403620 allowlist row is present and guard-validated; 0x403620 remains byte-unmatched and has not been reached by the ordered cursor.
- Ordered cursor: 0x401060, 0x401180, 0x401420, 0x401580, 0x4016a0, 0x401710, padding checkpoint 0x401964, 0x401970, 0x401a40, 0x401ab0, 0x401b20, 0x401c00, 0x401c60, 0x401d50, 0x401e50, 0x401f60, 0x402080, 0x402090, 0x402170, 0x402250, 0x4024a0, 0x4026d0, 0x4028c0, 0x402b70, 0x402be0, 0x402d60, 0x402f10, 0x402f60, 0x402fd0, 0x402ff0, 0x403040, 0x403510, 0x403530, 0x403550, 0x403620, 0x4036f0, 0x403750, 0x4037c0, 0x403800, 0x403830, 0x403870.
- Frozen profile/order: `vc5_o2_ob0_md_facs`; `check_function_order=true`; current 28 zero-mismatch rows are sentinels; `src/GameZRecoil/zMath/zmth.h` remains read-only.
- Next action:
  - Preserve the source-shaped zero-byte 0x401580 checkpoint and its ordinary 0x4016a0/0x4037c0 dependencies. The next ordered unmatched body is 0x401710; no source work at that address was started in this session.

### Parent batch card: ai_net.cpp address-order byte closure

- Task kind: active WIP
- Active group or address: `ai_net.cpp address-order byte closure`, `[0x401060,0x4038a0)`
- Evidence packets required: assigned Recoil.bndb reconstruction; per-failure ChatGPT Pro hard-byte transcript/receipt; source-worker closure; independent verifier
- Evidence packets received: saved BN reconstruction packet; ten ChatGPT Pro passes on 2026-07-09/10 established the recurring grouped-x87 macro boundary, retail register/stack-coloring constraints, bounded ordinary-C++ cleanup, and the differential CFG hypothesis ladder whose first probe failed unchanged and whose second probe, the pre-tested `while`, matched; the final differential-review content is recorded inline in `docs/reconstruction/verified_patterns.md` under transcript-content SHA-256 `353CA299FAED535572709FDCBACA2A39B4EBED4BEE586CA9C8056A52BFDFCD21`; mechanical source-block, owner, section, functional, data, and VC5 baseline audits
- Worker allocation: one BN reconstructor first, then one source worker, then one read-only verifier; no parallel writers because source/header/manifest order and Binary Ninja state are shared
- Validation scope: all 40 `ainet_text_block_order` functions, four linked data targets, all address-bound functional targets, guards/docblocks, and native x86 CTest
- Exit criteria: 40/40 zero unmasked mismatches under the frozen profile/order; no sentinel regression; 0x401964 resolved explicitly; functional/data/native checks pass; no unsupported owner-tier claim

### Source-worker handoff: ai_net.cpp ordered byte closure

- Section: battlesport.ai
- Owner/source scope: explicit narrower physical `ai_net.cpp` source-file block diagnostic `[0x401060,0x4038a0)` across the linked accepted owners named by the active group; this handoff does not accept owner gates or tier S
- Owner id: battlesport_gameplay.player_ai_mode2_top_level_steering
- Anchor addresses/groups: `ai_net.cpp address-order byte closure`; exact ordered cursor recorded above
- Allowed write paths: `src/Battlesport/ai_net.cpp`; `src/Battlesport/ai_net.h`; `tests/native/player_tests.cpp` only if the existing smoke lacks a required branch/ABI assertion
- Forbidden paths: every other production source/header; `src/GameZRecoil/zMath/zmth.h`; `.agent/RAW_ASSEMBLY_ALLOWLIST.txt` and `tools/vc5_verify_targets/ainet_text_block_order.json` (parent integration paths after the worker reports final generated symbols); `.agent/SOURCE_OWNERS.json`; `.agent/IMPLEMENTATION_GROUPS.md`; reconstruction-section/source-block catalogs; unrelated tools and verification manifests; unrelated tests; docs; support inputs; Binary Ninja state; workspace issues; git state
- Evidence inputs:
  - BN fact packet: parent-assigned current Recoil.bndb packet for `[0x401060,0x4038a0)`, especially 0x401964 padding, prototypes/calling conventions, tables, xrefs, and retail assembly
  - source-owner packet: `owner show/relationships` for the linked owners; accepted durable source-block routing to `src/Battlesport/ai_net.cpp`; no new owner determination or promotion
  - provider/data packet: four currently zero-mismatch data targets and all linked dependency owners remain frozen
  - scaffold audit packet: current source-shape, modern-C++, raw-offset, raw-image, and raw-assembly guards; the existing exact 0x401580 allowlist row covers only `AINET_VECTOR_SUBTRACT` and must not be broadened back to the scan, calls, shell, or whole body
  - workspace/librarian packet: `docs/reconstruction/source_file_layout_audit.md`, `verified_patterns.md`, and `inlined_helpers.md`; existing source-discovery Pro receipts above
- Expected source model: VC5-era C/C++ in the literal-backed `ai_net.cpp` physical block, naturally emitted through the current `ai_net.h` body reinclusion and `zmth.h` include timing. At 0x401580 ordinary C++ uses the pre-tested reverse-edge scan, ordinary class-static 0x4016a0 helper, ordinary member 0x4037c0 free, and only the recurring grouped-x87 subtraction macro; no `.inl`, order/linker pragma, wrapper, duplicate emitted body, indirect call, `_emit`, scan/whole-function assembly, `.asm`, naked function, provider shim, or source-shape/tier claim.
- Current 0x401580 result: zero mismatches with the source-shaped pre-tested `while`, exact 16-byte frame, 283/288-byte body/extent, five NOPs, natural order, and direct relocations +0x2d/+0x5b/+0xc3. Preserve this checkpoint; the expanded seven-case native smoke passes.
- Validation commands: `python tools/recoil.py verify vc5 0xADDRESS`; `python tools/recoil.py verify functional 0xADDRESS`; `python tools/recoil.py verify vc5 ainet_text_block_order`; `python tools/recoil.py guard raw-assembly --root src --allowlist .agent/RAW_ASSEMBLY_ALLOWLIST.txt`; `python tools/recoil.py guard source-shape --root src`; `python tools/recoil.py guard modern-cpp --root src --summary`; `python tools/recoil.py audit docblocks --path src/Battlesport/ai_net.cpp --summary --max 50`; `python tools/recoil.py audit docblocks --path src/Battlesport/ai_net.h --summary --max 50`
- Return packet:
  - changed files and exact per-address cursor results in ascending order
  - BN/source/VC5 evidence used and caveats; helper textual-user/emitted-function impact graph before shared edits
  - ChatGPT Pro receipt/transcript for every hard-byte failing address before its first new source variant; raw-assembly exception evidence and exact allowlist rows
  - exact commands run with pass/fail, mismatch/size/order/sentinel results, and functional status
  - blockers and overlap warnings; stop at the first unresolved address and do not skip it
  - non-authoritative owner gate/tier recommendations only; do not mutate owner or section ledgers

### Verifier handoff: ai_net.cpp complete physical block

- Section: battlesport.ai
- Validation scope: read-only independent verification of the final `[0x401060,0x4038a0)` source-file block, 40 functions, linked data, functional behavior, guards, native x86 build/tests, BN padding fact, and retained hard-byte evidence
- Anchor addresses/groups: `ai_net.cpp address-order byte closure`; all cursor addresses recorded above
- Exact commands: `python tools/recoil.py verify vc5 ainet_text_block_order`; focused `python tools/recoil.py verify vc5 0xADDRESS` and `python tools/recoil.py verify functional 0xADDRESS` for every cursor row; `python tools/recoil.py verify vc5 ainet_path_probe_half_width_scale_data`; `python tools/recoil.py verify vc5 player_ai_mode2_tuning_globals_data`; `python tools/recoil.py verify vc5 ainet_zrd_string_token_block_data`; `python tools/recoil.py verify vc5 ainet_node_free`; `python tools/recoil.py guard raw-assembly --root src --allowlist .agent/RAW_ASSEMBLY_ALLOWLIST.txt`; `python tools/recoil.py guard vc5-manifest`; `python tools/recoil.py guard source-shape --root src`; `python tools/recoil.py guard modern-cpp --root src --summary`; targeted docblock audits for both source files; `python tools/recoil.py audit provenance --strict`; `python tools/recoil.py audit source-blocks --strict`; `python tools/recoil.py audit source-map --check docs/reconstruction/source_file_map.md`; `python tools/recoil.py owner audit --strict`; native x86 configure/build and `python tools/recoil.py build msvc-x86 -- ctest --preset ninja-x86-debug`
- Evidence inputs:
  - source worker packet: ordered per-address results, changed files, full-target artifacts, functional results, and sentinel matrix
  - BN fact packet: saved Recoil.bndb packet with 0x401964 boundary/xref/table classification and any corrected prototypes
  - provider/data packet: final four data-target reports and immutable PE manifest verification
- Forbidden paths: all tracked files, Binary Ninja state, support inputs, owner/section/workspace-issue ledgers, and git state
- Return packet:
  - exact command lines and pass/fail results
  - 40-row mismatch/order/trim summary and key output lines
  - failure category, including any missing Pro/raw-assembly evidence or unrelated pre-existing broad guard debt
  - next narrow verification command; no owner gate/tier acceptance

### Group: Briefing.cpp byte cleanup

- Anchor: 0x403930 HudUiBriefingRuntime::Constructor
- Owner id: legacy.battlesport_gameplay.class_huduibriefingruntime
- Section: battlesport_gameplay
- Queue: active WIP under explicit user direction
- Scheduling exception: explicit-user-direction
- Scheduling exception evidence: User directed continued Briefing.cpp byte cleanup on 2026-07-09.
- Reason: physical `Briefing.cpp` byte cleanup after source-block/order diagnostics passed
- Source blockers:
  - 0x403930 HudUiBriefingRuntime::Constructor byte drift
  - 0x403ed0 HudUiBriefingRuntime::~HudUiBriefingRuntime byte drift
  - 0x404070 HudUiBriefingRuntime::Update byte drift
  - 0x403e20 HudUiCompositePanel::Destructor physical Briefing exception byte drift
- Verified clean rows:
  - 0x404400 HudUiBriefingRuntime::BuildObjectiveActionsFromIndex
  - 0x404aa0 BriefingActionPlaySample::Tick
  - 0x404bd0 Briefing::StopAndShutdownThread
- Next action:
  - Continue with focused `recoil_source_worker` handoffs on `0x403930`, `0x403e20`, `0x403ed0`, and `0x404070`; each hard byte-match pass must retain ChatGPT Pro receipt/transcript evidence before repeated probes or any raw assembly exception.
