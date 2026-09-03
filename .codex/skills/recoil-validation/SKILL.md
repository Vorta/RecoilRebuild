---
name: recoil-validation
description: Select and run proportionate Recoil checks after source, tracker, documentation, tool, build, or final-image changes and diagnose the first real failure.
---

# Recoil Validation

Validate the changed dimension directly in the canonical checkout. Use a fresh build root for live reconstruction acceptance. Do not treat a saved candidate, object hash, commit, timestamp, or raw whole-file equality as acceptance.

## Selection

- Tool, command, docs, skill, or policy changes: the smallest focused proof-kernel test that exercises the changed acceptance boundary, then the infrastructure aggregate. Do not add tests for ordinary reconstruction source work.
- VC5 compiler-driver changes: also run `verify vc5 --smoke` in a fresh
  `build/live-validation` root.
- Source/order changes: the registered `verify vc5-order` target, then the direct live order command when acceptance is authorized.
- Call contracts: `verify call-contract --slice ... --json --summary`, then direct live acceptance; complete the phase with `progress call-contract close-live`.
- Authored bytes: object feedback as useful, then direct authored-byte live acceptance.
- Linked bytes: direct linked-byte acceptance after strict prerequisites.
- Final candidate: PE manifest check, final-image catalog audit, and typed final-image validation.

Production source is accepted only through the registered VC5/order,
call-contract, byte, linked-image, and final typed comparisons. The retired
native/functional smoke lane is not an acceptance prerequisite and must not be
recreated. Source-policy guards run automatically on live acceptance and final
validation; use `audit source-policy` only for focused diagnosis.

Infrastructure gates:

```powershell
python tools/recoil.py doctor
```

`doctor` owns the sequential, fail-fast infrastructure matrix; it does not
compile or accept reconstruction source. Run an individual
underlying audit only to diagnose its reported failure, not in addition to a
passing doctor run. A workspace-wide maintenance pass runs doctor once after
focused tests; individual skills and runbooks should refer here instead of
copying its internal command list.

Final typed gates:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

Diagnose from the first divergence and its neighbors. Distinguish tool/environment failures from source or semantic mismatches. A pass accepts only the command's named dimension; never infer owner, provider, source model, gate, tier, storage, or final-image state from an unrelated pass.
