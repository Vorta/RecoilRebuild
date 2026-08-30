---
name: recoil-validation
description: Select and run proportionate Recoil checks after source, tracker, documentation, tool, build, or final-image changes and diagnose the first real failure.
---

# Recoil Validation

Validate the changed dimension directly in the canonical checkout. Use a fresh build root for live reconstruction acceptance. Do not treat a saved candidate, object hash, commit, timestamp, or raw whole-file equality as acceptance.

## Selection

- Tool, command, docs, skill, or policy changes: focused unit tests plus infrastructure audits.
- Source/order changes: the registered `verify vc5-order` target, then the direct live order command when acceptance is authorized.
- Call contracts: `verify call-contract --slice ... --json`, then direct live acceptance; complete the phase with `progress call-contract close-live`.
- Authored bytes: object feedback as useful, then direct authored-byte live acceptance.
- Linked bytes: direct linked-byte acceptance after strict prerequisites.
- Functional behavior: registered functional target and native smoke where required.
- Final candidate: PE manifest check, final-image catalog audit, and typed final-image validation.

Infrastructure gates:

```powershell
python tools/recoil.py doctor --infrastructure-only
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py audit pipeline-contracts --strict
python tools/recoil.py audit pipeline-reachability --strict
```

Final typed gates:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

Diagnose from the first divergence and its neighbors. Distinguish tool/environment failures from source or semantic mismatches. A pass accepts only the command's named dimension; never infer owner, provider, source model, gate, tier, storage, or final-image state from an unrelated pass.
