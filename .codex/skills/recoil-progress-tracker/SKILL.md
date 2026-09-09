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

Function match classification is distinct from serial stage acceptance. Use
`progress match refresh --all` or `--at <address>` with a fresh build root,
expected revision and dry-run/apply to record complete live `byte`/`instruction`
proofs and synchronize `@recoil-match` source mirrors. This does not move the
scheduler. Register specific Pro fallback eligibility through
`progress match review-instruction --payload-file ...` (dry-run, review, apply).
The executable runbook owns its exact payload. Old comparisons, annotations,
and Pro answers cannot replace a fresh build. Keep exact-byte dimensions false
for instruction matches; serial byte stages use the explicit approved
instruction alternative, retaining every relocation and placement obligation.

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

`close-live` runs each fresh no-reuse slice verifier in an isolated build root
with bounded subprocess concurrency (eight by default; `--max-workers` accepts
one through 21).

It validates and stores the transcripts in retail order. Only the read-only
scans overlap. It next runs exactly one fresh canonical
whole-program linkability diagnostic before one serial mutation. The diagnostic
compiles sources, aliases, and resources and produces an executable and map,
but suppresses linked-order evaluation, playground deployment, and all byte,
linked-order, or final-image acceptance. Never link per slice.

Use `verify call-contract` and `advance-live-call-contract` directly only for
the first divergent current slice or another focused one-slice diagnosis.

For manual semantic mutations—owner topology, provider/classification decisions, catalog exceptions, target bindings, positive gates, or tiers—run the command with `--dry-run`, review the complete diff, then repeat unchanged with `--apply` and the expected revision. Conservative downgrades use the governed downgrade route.

Named internal read-only data may use its stable object-name stem in a reviewed
relocation-target binding. The byte verifier proves the compiler's suffixed
static symbol against exact retail storage and the complete registered reader
population; do not invent a compiler ordinal for the expected identity.

For a native VC5 unwind-only EH parent, use
`progress relocation-target bind-native-eh --payload-file <reviewed-json> --expected-revision <revision> --dry-run --json`,
review its derived source/provider/retail/runtime context, then repeat with
`--apply`. The exact payload fields are `reviewed: true`, `source_symbol_id`,
`object_symbol`, `provider_owner_id`, `evidence_ids`, and `reason`. The handler
must retain its existing non-authored classification. If it has a primary owner,
that provider must be accepted and exclusive. A discovered packet lifecycle is
eligible when its provider boundary gate is accepted; retain that pending
lifecycle in the binding and require renewed review if it changes. This
dependency binding does not accept the packet owner's lifecycle. For an already classified generated
EH helper with unresolved ownership and no primary-owner relationship, use
`provider_owner_id: null` with existing parent-function evidence; ownership stays
unresolved and adding an owner later makes the binding stale. The command binds the native
handler role and the canonical runtime's absolute `__except_list` symbol;
it accepts no provider bytes, image storage, owner gates, or tiers. Fresh byte
verification requires the parent-associated COFF sections and the complete
linked packet relocation graph. Keep FS:[0] relocations; never invent a data
extent at address zero or use a generated label ordinal as expected identity.

For a compiler-generated implicit cleanup selected by native member-array
construction or destruction, use `progress relocation-target bind-native-array-cleanup` with
`--payload-file`, `--expected-revision`, and `--dry-run --json`, review, then
repeat with `--apply`. Its exact payload fields are `reviewed: true`,
`source_symbol_id`, `object_symbol`, `offset` (the relocation field), `owner_id`,
`evidence_ids`, and `reason`. Retail derives the cleanup/element/helper targets,
array count, stride, member offset, and helper callback ABI. Destruction also
requires one current reviewed construction dependency of the same parent class
and member array to derive its element identity; stale or ambiguous pairs block.
Current source must
retain the native array and implicit destructor hierarchy. Fresh verification
checks the cleanup's executable fragment, nested table relocation, linked symbol
resolution, and consistent physical mapping across checked references. This
binds a dependency to an existing generated lifecycle target; it does not create
an authored ICF group or accept an original alias census, source model, owner
gate, or tier. The original folded winner remains unresolved when unproved.

For a direct authored reference to an existing unowned non-authored MSVCRT import thunk, use
`progress relocation-target bind-native-import --payload-file <reviewed-json>
--expected-revision <revision> --dry-run --json`, review, then repeat with
`--apply`. The exact payload fields are `reviewed: true`, `source_symbol_id`,
`object_symbol`, `offset`, `target_symbol`, `evidence_ids`, and `reason`. The
proof derives the exact retail IAT/DLL/name, requires one matching canonical VC5
long import member and its code/name-table relocations, then checks the current
COFF reference and linked thunk/import. Unresolved ownership and the inventory
extent remain unchanged; no provider body, padding, storage, or gate is accepted.

To retract a superseded relocation exception, use
`progress relocation-exception remove` with the source id/address, the complete
stored exception as `--payload-json`, a `--reason`, and the expected revision.
Dry-run and review first. Removal changes only that exact exception; it retains
target/owner facts and accepts no replacement evidence. Bind the replacement
identity separately and require fresh live byte acceptance.

For a real function whose typed extent includes an unreferenced INT3 alignment
tail, use `progress symbol separate-tail-padding --payload-file ...` with an
exact `recoil-function-tail-padding-v1` snapshot, dry-run first. The route checks
current retail and BN instructions/references and preserves the function's
identity, block, owner and call sites. It accepts neither body bytes nor padding;
resume fresh serial byte acceptance after reviewing and applying the correction.

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

For positive existing-storage or owner acceptance, use the scoped live commands
and review-template procedure in
`docs/reconstruction/retail_executable_reproduction.md`. Registration and owner
replacement are not substitutes for these guarded acceptance operations.

Use `recoil-validation` after tracker-tool changes. A passing validation changes only the dimension named by that command.
