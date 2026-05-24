# Reconstruction Knowledge Index

This is the landing page for durable reconstruction facts that are broader than
one source comment. Binary Ninja and `.agent/RECOIL_PLAN.md` remain authoritative
for function identity, types, markers, and acceptance state.

## Current Ledgers

- `agent_launch_checklist.md` - compact preflight checklist for launching
  reconstruction agents, including Binary Ninja, native x86 shell, source-map,
  and temporary-state hygiene checks.
- `compiler_linker_provenance.md` - compiler, linker, and verification-profile
  assumptions guarded by `tools/recoil_provenance_audit.py --strict`.
- `inlined_helpers.md` - compact ledger for likely original helpers and methods
  that were fully inlined by the retail compiler, with caller evidence and
  verification limits.
- `original_class_candidates.md` - generated candidate map for likely original
  classes, records, vtables/function tables, and namespace-style subsystems.
- `provider_abi_notes.md` - repo-local provider assumptions for VC5SP3/VC6, MFC42,
  legacy DirectX, imports, and runtime verification.
- `source_file_map.md` - generated original-source placement map from
  `Reimplements` provenance comments in `src/`.
- `verified_patterns.md` - compact ledger of currently verified reusable source
  and verification shapes.
- `zsnd_error_verification.md` - zSound error-helper verification notes,
  including the `ReportA3DError` switch-table comparison limitation.
- `zsnd_cd_verification.md` - zSound CD helper verification notes, including the
  recovered playback-position triplet layout used by `ResetTrackState`.
- `zsnd_play_handle_verification.md` - zSound play-handle verification notes,
  including the `StopIfActive` backend-dispatch mismatch profile.
- `zsnd_sample_set_verification.md` - zSound sample-set registry verification
  notes, including the `FindByName` inline-`strcmp` mismatch profile.
- `README.md` - rules for when to add durable reconstruction notes.

## Archival Notes

- `docs/reconstruction/NOTES.md` is archival working evidence and source
  material for narrower durable docs. Do not add routine new findings there
  unless no narrower document or source comment can carry the fact.

## Agent Use

- For new implementation placement, check `source_file_map.md` first, then
  confirm current Binary Ninja source comments and call-site evidence.
- For new agent handoff, start with `agent_launch_checklist.md`, then use
  `AGENTS.md` for the full workflow rules.
- For compiler or provider questions, check `provider_abi_notes.md` and
  `compiler_linker_provenance.md` before adding one-off flags or stand-ins.
- Before introducing or reshaping class, vtable, function-table, record, or
  namespace/module boundaries, check `original_class_candidates.md` as advisory
  generated evidence, then confirm against current Binary Ninja facts.
- Before duplicating a small repeated decompiled body across callers, check
  `inlined_helpers.md` and consider restoring a likely original inline helper or
  method with caller-based verification evidence.
- For temporary dependency closures, use `.agent/IMPLEMENTATION_GROUPS.md`; move
  durable facts here only when they save future reconstruction time.
