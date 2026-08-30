---
name: recoil-progress-tracker
description: Inspect and safely update the serial Recoil reconstruction tracker, its current task, relationships, gates, order, call-contract, byte, and final-image state.
---

# Recoil Progress Tracker

`.agent/RECONSTRUCTION_PROGRESS.sqlite3` is the sole reconstruction-progress authority. Never edit it with SQLite tools or application code. Use `python tools/recoil.py progress ...` only.

The tracker is schema 6/user version 3. Its revision vector contains exactly:

- `transaction_revision`
- `semantic_revision`
- `evidence_generation_revision`

There is no scheduler revision, work-item collection, packet, claim, lease, reservation, lane allocator, or generated-current cache.

## Current task

With no explicit target, run:

```powershell
python tools/recoil.py progress next --json
```

It returns one `recoil-current-task-v2` object with one task, check command, and—when ready—one direct acceptance command. Work on that task in the canonical checkout. Do not allocate or hand it off.

Strict stage order is:

1. authored function order
2. authored call contracts
3. authored bytes
4. full linked function order
5. linked bytes
6. final validation

## Mutation rules

Live self-validating commands rebuild current source and apply their own CAS-guarded result:

```powershell
python tools/recoil.py progress advance-live-order --target <id> --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-semantic-revision <semantic> --expected-evidence-generation-revision <evidence> --apply --json
python tools/recoil.py progress call-contract close-live --build-root <fresh-root> --expected-semantic-revision <semantic> --expected-evidence-generation-revision <evidence> --apply --json
python tools/recoil.py progress advance-live-authored-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-linked-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
```

For manual semantic mutations—owner topology, provider/classification decisions, catalog exceptions, target bindings, positive gates, or tiers—run the command with `--dry-run`, review the complete diff, then repeat unchanged with `--apply` and the expected revision. Conservative downgrades use the governed downgrade route.

Run `python tools/recoil.py progress audit --scope pipeline --strict --json` after tracker-tool changes. A passing validation changes only the dimension named by that command.
