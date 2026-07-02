# Reconstruction Knowledge Index

This is the landing page for durable reconstruction facts that are broader than
one source comment. Binary Ninja and `.agent/SOURCE_OWNERS.json` remain authoritative
for function identity, types, markers, and acceptance state.

## Current Ledgers

- `agent_launch_checklist.md` - compact preflight and task-selection checklist
  for reconstruction agents.
- `compiler_linker_provenance.md` - compiler, linker, and verification-profile
  assumptions guarded by `python tools/recoil.py audit provenance --strict`.
- `data_owner_audit.md` - complete data-owner acceptance rules and the current
  compact data-gate ledger.
- `final_executable_repro.md` - final VC5SP3 executable/DLL reproducibility
  lane, including `final-repro`, final-build, PE/resource comparison, and
  linked `.data` layout blockers.
- `inlined_helpers.md` - compact ledger for likely original helpers and methods
  that were fully inlined by the retail compiler, with caller evidence and
  verification limits.
- `messages_dll.md` - companion `messages.dll` reconstruction scope, generated
  message-table source, lookup-table source, and validation commands.
- `original_classes.md` - compact policy and ledger for class, record,
  vtable/function-table, provider, and namespace-style subsystem boundaries.
- `owner_led_workflow.md` - durable source-owner ledger commands and promotion
  gate routing.
- `provider_abi_notes.md` - repo-local provider assumptions for VC5SP3, MFC42,
  legacy DirectX, imports, and runtime verification.
- `recoil_app_destructor_tier_s.md` - RecoilApp constructor/destructor EH and
  tier S verification notes for the app owner cleanup cluster.
- `source_file_map.md` - generated original-source placement map from
  address-backed `Reimplements` provenance docblocks in `src/`, plus legacy
  line comments until touched source is converted.
- `source_file_layout_audit.md` - durable source-file placement constraints
  from BN source-path literal xrefs, including compiler-emitted source-file
  block order and the early Battlesport `ai_net.cpp`/`Briefing.cpp`/
  `player.cpp` owner repair notes.
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
progress notes or duplicated plan state.

## Agent Use

- For new implementation placement, check `source_file_map.md` first, then
  confirm current Binary Ninja source comments, source-path literal xrefs,
  physical source-file block order, and call-site evidence. New or touched
  functions need immediate provenance/Purpose docblocks. When a source-file
  block is known, the rebuilt VC5 COFF function order must naturally match the
  retail BN address order before byte readiness. A generated order mismatch is
  a source-shape/include-shape blocker until proven otherwise. Model
  header/COMDAT helpers through recovered `.h`/`.cpp` ownership, header
  layering, and include timing instead of moving semantic helpers into the
  wrong `.cpp`. Known/order-relevant `source_shape_inputs` are mandatory
  reconstruction inputs once known, but remain attached to the physical `.cpp`
  block and are not owner-gate evidence by themselves. Emitted header
  contributors with known address ranges appear as `partial-header` rows in
  `python tools/recoil.py audit source-blocks --list`; reconstruct those bodies
  in the header `source_path` and compile them through `included_in`, without
  treating the row as full-header or owner-gate/tier proof. Do not add `.inl` files for production reconstruction;
  existing `.inl` files need independent original-source proof.
- For new agent handoff, start with `agent_launch_checklist.md`, then use
  `AGENTS.md` for the full workflow rules.
- For owner/data promotion, inspect `.agent/SOURCE_OWNERS.json` through
  `python tools/recoil.py owner show/find/audit`; do not treat one address as
  the accepted unit when BN proves a larger owner.
- For compiler or provider questions, check `provider_abi_notes.md` and
  `compiler_linker_provenance.md` before adding one-off flags or stand-ins.
- Before introducing or reshaping class, vtable, function-table, record, or
  namespace/module boundaries, check `original_classes.md` for the class/table
  gate and boundary ledger, then confirm against current Binary Ninja facts.
- Before duplicating a small repeated decompiled body across callers, check
  `inlined_helpers.md` and consider restoring a likely original inline helper or
  method with caller-based verification evidence.
- For temporary dependency closures, use `.agent/IMPLEMENTATION_GROUPS.md`; move
  durable facts here only when they save future reconstruction time.
