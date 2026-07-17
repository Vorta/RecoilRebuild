# Reconstruction Knowledge Index

This is the landing page for durable reconstruction facts that are broader than
one source comment. The current already-open Binary Ninja database is a
maintained analysis artifact and is authoritative for binary behavior, ABI,
layouts, globals, imports, tables, xrefs, function boundaries/order, and
assembly; BN function names and comments are provisional navigation labels. New
active-scope evidence may be corrected only by a parent-assigned bounded
`recoil_bn_reconstructor`; tool/docs maintenance is BN-read-only, and the
reconstructor may not load/switch/patch or decide owner/block/order/provider/tier
acceptance. `.agent/RECONSTRUCTION_PROGRESS.json` is the
only reconstruction-progress authority and must be accessed through `python
tools/recoil.py progress ...`.

## Current Authorities

- [`retail_executable_reproduction.md`](retail_executable_reproduction.md) -
  canonical order-primary pipeline (`authored-function-order`, then
  `full-function-order`) plus the independent retail-monotonic
  `authored-byte-match` lane; `linked-byte-match` waits for both lanes and is
  followed by `final-validation`. It defines row classifications, sole
  `progress next` scheduling, exact
  retail SHA-256 goal, evidence gates, and debt-free closeout contract.
- `agent_launch_checklist.md` - compact preflight and task-selection checklist
  for reconstruction agents.
- `compiler_linker_provenance.md` - compiler, linker, and verification-profile
  assumptions guarded by `python tools/recoil.py audit provenance --strict`.
- `data_owner_audit.md` - data-owner acceptance and the boundary between source
  data symbols/gates, physical storage contributions, PE output sections, and
  final-image evidence.
- [`final_executable_repro.md`](final_executable_repro.md) - Phase-5 VC5SP3
  Recoil.exe mechanics; linked-data and companion-DLL paths are diagnostics or
  independent validation, never peer schedulers.
- [`final_executable_repro_history.md`](final_executable_repro_history.md) -
  archived dated final-data experiments, rejected hypotheses, numeric
  snapshots, and provenance; use the live runbook and audit output for current
  state.
- `inlined_helpers.md` - compact ledger for likely original helpers and methods
  that were fully inlined by the retail compiler, with caller evidence and
  verification limits.
- `messages_dll.md` - companion `messages.dll` reconstruction scope, generated
  message-table source, lookup-table source, and validation commands.
- `original_classes.md` - compact policy and ledger for class, record,
  vtable/function-table, provider, and namespace-style subsystem boundaries.
- `owner_led_workflow.md` - unified-tracker owner operations, entry-tier
  acceptance, and derived-owner gate routing.
- `provider_abi_notes.md` - repo-local provider assumptions for VC5SP3, MFC42,
  legacy DirectX, imports, and runtime verification.
- `recoil_app_destructor_tier_s.md` - RecoilApp constructor/destructor EH and
  tier S verification notes for the app owner cleanup cluster.
- `source_file_layout_audit.md` - durable source-file placement constraints
  from BN source-path literal xrefs, compiler-emitted physical source-file
  block order, header/provider/COMDAT exceptions, and dated historical repair
  notes. It deliberately contains no live cursor.
- `visual_studio_mcp_workflow.md` - preferred Visual Studio MCP development
  workflow for generated `vs-x86` solution projects.
- `verified_patterns.md` - compact ledger of currently verified reusable source
  and verification shapes.
- `zfmv_script_cleanup_reset_verification.md` - zFMV cleanup/reset tier S
  verification notes for the script teardown dependency pair.
- `zsnd_error_verification.md` - zSound error-helper verification notes,
  including the `ReportA3DError` switch-table comparison limitation.
- `zsnd_cd_verification.md` - zSound CD helper verification notes, including the
  recovered playback-position triplet layout used by `ResetTrackState`.
- `zsnd_play_handle_verification.md` - zSound play-handle verification notes,
  including the `StopIfActive` backend-dispatch mismatch profile.
- `zsnd_sample_init_verification.md` - zSound wave-data initialization
  verification notes for A3D and DirectSound sample backends.
- `zsnd_sample_set_verification.md` - zSound sample-set registry verification
  notes, including the `FindByName` inline-`strcmp` mismatch profile.
- `zsnd_snapshot_verification.md` - zSound snapshot verification notes,
  including current `StopAllIfPlaying` and snapshot cluster byte-diff limits.

## Documentation Policy

Use this directory for durable facts that save future reconstruction time across
source files, subsystems, providers, or verification targets. Prefer source
comments for facts local to one function, class, layout, or call site.

When finishing reimplemented code, class/source-cluster work, or verification
evidence, make a documentation decision before handoff: add a compact durable
note here, add a local source comment, or state that no durable new
documentation was needed. Document facts that prevent rediscovery, not routine
progress.

Add or update a reconstruction document for cross-cutting facts such as
compiler/provider contracts, shared class layouts, repeated inlined helpers,
file formats, repeated Binary Ninja/toolchain limits, or source-file placement
evidence. Keep entries compact: name addresses and symbols when known, state
the evidence source, separate recovered facts from open limits, and avoid broad
progress notes or duplicated live tracker state.

## Agent Use

- With no explicit assignment, run `python tools/recoil.py progress next` before
  consulting any owner, block, semantic, work, final, or companion-binary view.
  Expand only that selected cursor into its physical block, semantic span, and
  complete source-shaped owner. For implementation placement, inspect the joined
  `progress show` view and `source_file_layout_audit.md`, then
  confirm current Binary Ninja source comments, source-path literal xrefs,
  physical source-file block order, and call-site evidence. New or touched
  functions need immediate provenance/Purpose docblocks. BN function names and
  comments are provisional navigation labels; current assembly, xrefs,
  source-path literals, function order, and provider/import evidence decide
  placement. The current production `src` tree is implementation state, not
  original-source authority. When a source-file
  block is known, the rebuilt VC5 COFF authored-body/authored-lifecycle-body rows must
  naturally retain retail relative order before authored-byte readiness. Exact
  complete linked population, RVAs, and seams remain the later full-order gate.
  A generated authored-order mismatch is a source-shape/include-shape blocker
  until proven otherwise. Model
  header/COMDAT helpers through recovered `.h`/`.cpp` ownership, header
  layering, and include timing instead of moving semantic helpers into the
  wrong `.cpp`. Known/order-relevant `source_shape_inputs` are mandatory
  reconstruction inputs once known, but remain attached to the physical `.cpp`
  block and are not owner-gate evidence by themselves. Emitted header
  contributors with known address ranges appear as partial-header physical-block
  records in the unified tracker; reconstruct those bodies in the header
  `source_path` and compile them through `included_in`, without
  treating the row as full-header or owner-gate/tier proof. Do not add `.inl` files for production reconstruction;
  existing `.inl` files need independent original-source proof. Use `progress
  show` only to expand the block selected by `progress next`; it is a joined
  entity view, not a second queue. Passing
  smokes, byte checks, ABI call-shape checks,
  or order diagnostics are evidence candidates, not owner-gate or tier proof.
  Do not reject a physical `.cpp` block because current production source is
  wrong, and do not classify provider-looking empty/no-op bodies as
  provider-owned before checking authored override evidence. Before simplifying
  repeated branch bodies into a shared source-level `goto` or common label,
  check `verified_patterns.md` for the VC5 duplicated-tail/tail-merge pattern;
  the retail CFG can be a compiler tail merge of duplicated original source,
  and a hand-written common tail can preserve behavior while breaking byte
  identity. Passing smokes, byte checks, or ABI call-shape checks are evidence
  candidates, not source-shape proof.
- For new agent handoff, start with `agent_launch_checklist.md`, then use
  `AGENTS.md` for the full workflow rules.
- For owner/data promotion, use `progress show`, `progress find`, and
  `progress audit`; do not treat one
  address as the accepted unit when BN proves a larger owner.
- For physical data/layout evidence, use `progress output-section show` and
  `progress storage show`. Unknown extents omit size/end; final-data/final-repro
  receipts and their imports are observed evidence only. They create no work
  unit, peer scheduler, or owner-action batch. Storage/section acceptance is an
  explicit dry-run-first operation, and final acceptance additionally requires
  every mandatory whole section and an exact final-repro receipt.
- For compiler or provider questions, check `provider_abi_notes.md` and
  `compiler_linker_provenance.md` before adding one-off flags or stand-ins.
- Before introducing or reshaping class, vtable, function-table, record, or
  namespace/module boundaries, check `original_classes.md` for the class/table
  gate and boundary ledger, then confirm against current Binary Ninja facts.
- Before duplicating a small repeated decompiled body across callers, check
  `inlined_helpers.md` and consider restoring a likely original inline helper or
  method with caller-based verification evidence.
- For temporary dependency closures, use a structured tracker work item; move
  durable facts here only when they save future reconstruction time.
