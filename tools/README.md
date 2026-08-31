# Recoil Tooling

`python tools/recoil.py` is the sole public tool entry point. Use
`python tools/recoil.py help`, `help <group>`, or `commands --json` for the
current command surface; backend modules are internal.

The serial scheduler is:

```powershell
python tools/recoil.py progress next --json
```

It returns one `recoil-current-task-v2` task with one stage, advisory scope,
optional diagnostic check, direct acceptance command when ready, blocker, and
the transaction/semantic/evidence-generation revisions. The machine-local
SQLite tracker is the only current-state authority. There is no README cache,
work allocator, role registry, reservation, handoff, or secondary progress
record.

For the current authored call-contract stage:

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --build-root <fresh-root> --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
python tools/recoil.py progress call-contract close-live --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

One target-qualified Binary Ninja preflight is required after opening or
switching the database, not per slice:

```powershell
python tools/recoil.py binja preflight --binary recoil --strict
```

Direct live acceptance rebuilds current source and compares typed retail facts.
Stored receipts, hashes, snapshots, timestamps, and raw whole-file equality are
not acceptance. Manual semantic mutations remain dry-run-first; self-validating
live acceptance normally uses direct `--apply`.

Core maintenance validation is one sequential, fail-fast aggregate after
focused tests:

```powershell
python tools/recoil.py doctor
```

Final typed validation is:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

Use the focused skill owning the operation and the canonical runbook at
`docs/reconstruction/retail_executable_reproduction.md`. Never hand-edit the
SQLite authorities or mutate reconstruction source/BN state as infrastructure
maintenance.
