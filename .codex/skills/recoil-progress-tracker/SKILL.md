---
name: recoil-progress-tracker
description: Inspect and safely update the serial Recoil reconstruction tracker, its current task, relationships, gates, order, call-contract, byte, and final-image state.
---

# Recoil Progress Tracker

`.agent/RECONSTRUCTION_PROGRESS.sqlite3` is the sole reconstruction-progress authority. Never edit it with SQLite tools or application code. Use `python tools/recoil.py progress ...` only.

Schema versions are owned by tool migrations. Its revision vector contains
exactly:

- `transaction_revision`
- `semantic_revision`
- `evidence_generation_revision`

There is no scheduler revision, work-item collection, packet, claim, lease, reservation, lane allocator, or generated-current cache.

## Current task

With no explicit target, run:

```powershell
python tools/recoil.py progress next --json
```

It returns one `recoil-current-task-v2` object with one task, check command,
optional serial `stage_runner_command`, and—when ready—one direct acceptance
command. Work on that task in the canonical checkout. Do not allocate or hand
it off.

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

For authored call contracts, the normal serial whole-stage route is:

```powershell
python tools/recoil.py progress call-contract replay-live --dry-run --json
python tools/recoil.py progress call-contract replay-live --apply --json
```

Dry-run plans the complete original-slice census without building, querying
Binary Ninja, or mutating. Apply performs one invocation-local full-census
proof: it discovers source once, builds each unique target and separate
definition TU once, shares one COD index and target-qualified Binary Ninja fact
cache, then projects the proof back onto the immutable original slices. Those
projections use the same per-body evidence shape and serial semantic/evidence
CAS as direct slice acceptance. Replay revalidates already-current predecessor
slices, stops after committing only the passing bodies in the first divergent
current slice, leaves later slices untouched, and returns but never runs the
mandatory `close-live` command. Its fresh `-replay-NNN` sibling root never
consumes the scheduler-selected direct root; an interrupted root is inert and a
later invocation selects a new sibling.

`close-live` runs one fresh no-reuse complete-census scan and then exactly one
fresh canonical whole-program linkability diagnostic before mutation. The
diagnostic compiles sources, aliases, and resources and produces an executable
and map, but suppresses linked-order evaluation, playground deployment, and all
byte, linked-order, or final-image acceptance. Never link per slice.

Use `verify call-contract` and `advance-live-call-contract` directly only for
the first divergent current slice or another focused one-slice diagnosis.

For manual semantic mutations—owner topology, provider/classification decisions, catalog exceptions, target bindings, positive gates, or tiers—run the command with `--dry-run`, review the complete diff, then repeat unchanged with `--apply` and the expected revision. Conservative downgrades use the governed downgrade route.

For a reviewed current implementation-path move, edit the repository paths,
synchronize every affected verification target first, then use the exact-match
relocation route rather than composing owner/block/source-trace mutations:

```powershell
python tools/recoil.py progress source-path relocate --payload-file build/reviewed-source-path-relocation.json --expected-revision <revision> --dry-run --json
python tools/recoil.py progress source-path relocate --payload-file build/reviewed-source-path-relocation.json --expected-revision <revision> --apply --json
```

The v1 payload must enumerate the complete matching physical-block,
semantic-span, owner, artifact, and pre-synchronized verification-target id
sets. This route changes only current implementation paths, retains historical
provenance, and conservatively invalidates dependent order, call-contract, and
byte facts.

Run `python tools/recoil.py progress audit --scope pipeline --strict --json` after tracker-tool changes. A passing validation changes only the dimension named by that command.
