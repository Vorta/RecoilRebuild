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
- `provider_abi_notes.md` - repo-local provider assumptions for VC6, MFC42,
  legacy DirectX, imports, and runtime verification.
- `source_file_map.md` - generated original-source placement map from
  `Reimplements` provenance comments in `src/`.
- `README.md` - rules for when to add durable reconstruction notes.

## Legacy Notes

- `../../reconstruction/NOTES.md` is archival working evidence. Search it when
  investigating old Binary Ninja cleanup or naming rationale, but do not add new
  broad notes there.

## Agent Use

- For new implementation placement, check `source_file_map.md` first, then
  confirm current Binary Ninja source comments and call-site evidence.
- For new agent handoff, start with `agent_launch_checklist.md`, then use
  `AGENTS.md` for the full workflow rules.
- For compiler or provider questions, check `provider_abi_notes.md` and
  `compiler_linker_provenance.md` before adding one-off flags or stand-ins.
- For temporary dependency closures, use `.agent/IMPLEMENTATION_GROUPS.md`; move
  durable facts here only when they save future reconstruction time.
