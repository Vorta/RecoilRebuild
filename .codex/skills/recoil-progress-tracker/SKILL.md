---
name: recoil-progress-tracker
description: Inspect and safely update the unified RecoilRebuild reconstruction tracker. Use when you need the current cursor, joined owner/block/semantic/work context, relationships and gates, function-order/linkage/byte state, progress audits, blocker routing, or a parent-owned revision-guarded mutation in `.agent/RECONSTRUCTION_PROGRESS.sqlite3`.
---

# Recoil Progress Tracker

Start from root `AGENTS.md`. `.agent/RECONSTRUCTION_PROGRESS.sqlite3` is the
only progress authority. Never hand-edit it. It is distinct from the workspace
issue authority `.agent/WORKSPACE_ISSUES.sqlite3`. Progress transactions retain
one monotonic transaction revision and separate semantic, evidence-generation,
and scheduler revision domains; the issue database keeps its own independent
monotonic revision. Claims and leases advance scheduler concurrency,
call-contract acceptance guards semantic plus evidence-generation concurrency,
and issue changes belong only to the issue ledger. Tracker ids and revision
coordinates identify ledger state and concurrency only; none qualifies a
candidate. Routine no-mutation checks compare revisions, schema/user version,
relevant row counts, and `PRAGMA integrity_check`. After the paired cutover,
runtime and rollback are SQLite-only; no JSON backend, mirror, or export exists.

## Navigate

With no explicit target, run:

```powershell
python tools/recoil.py progress next --json
```

Then use focused joined views:

```powershell
python tools/recoil.py progress status --json
python tools/recoil.py progress show <selector> --json
python tools/recoil.py progress find <query>
python tools/recoil.py progress owner relationships <selector>
python tools/recoil.py progress work leases --json
python tools/recoil.py progress audit --strict
```

Order, object-byte, authored-byte, linked-byte, owner, tier, storage, output
section, and final-image state remain distinct.

## Claim And Render Work

With no explicit target, including a bare `Start`, only the parent computes
available child slots from effective runtime capacity and atomically claims all
compatible current scheduler lanes without waiting for another user
confirmation:

```powershell
python tools/recoil.py progress work claim-current --lane all --max-packets <available-child-slots> --expected-scheduler-revision <scheduler-revision> --apply --json
python tools/recoil.py progress handoff --packet-id <packet-id> --json
```

The fixed priority is the active primary lane (order or call contract), full
authored byte, then subordinate authored-object byte. A blocked primary does
not suppress compatible bytes; full authored byte wins over overlapping new
object work. The tool owns resource-conflict and capacity skips. Render and
launch each returned packet. Individual
`--lane <primary|authored|object>` claims remain supported.

Claims, leases, and work close are guarded by the scheduler revision domain. A
lease change must not stale accepted semantic evidence or force a convergence
generation carry-forward. Source, compiler-profile, verifier, and linker
call-contract obligations may become distinct packets for the same target when
their normalized writable resource claims do not conflict. Target identity is
not a mutex by itself: read/read may overlap, while read/write and write/write
overlap remains blocked.

`claim-current` creates and reserves the real compact mode-specific packet and
derives its normalized resource claims. `progress handoff --packet-id
<packet-id> --json` only renders that active reservation; it fails for no
reservation, absent lease, empty write claims, or a mutating worker command. It
must never synthesize a work item or expose a parent `--apply` command as worker
validation. A tracked-write handoff reauthenticates and returns the exact
branch, opaque baseline commit, linked `worktree_root`, authenticated external
build root, and bounded worker Git permissions. Launch the worker with its
current directory set to that returned `worktree_root`. For `order-edit-v1`, the worker command is only the registered
`verify vc5-order` loop. For `call-contract-edit-v1`, it is only the registered
nonmutating `verify call-contract` loop for the complete deterministic slice.
Byte/BN/Pro/evidence-package obligations are not added to either packet. An
unresolved row inside an order target interval blocks the complete block packet
and live acceptance even if a resolved-subset raw diagnostic passes.

When the current authored-call-contract convergence generation is
`retail-blocked`, the same `claim-current` route derives bounded
`call-contract-retail-fact-v1` packets only from that generation's strict
`retail_blocker_descriptors`. Each packet is retail-monotonic, contains at most
eight exact caller blockers, names `recoil_bn_fact_mapper`, and reserves one
packet-specific lane write plus read access to the exact target, covered blocks,
tracker, and `Recoil.bndb`. Its compact handoff truthfully has no writable
source/header paths. The lane reservation is the required write claim; every
database/content claim is read-only. The sole worker command is the nonmutating
target-qualified Binary Ninja preflight. The packet gathers raw facts only,
accepts no call contract or other state, supplies no identity guess, and never
uses candidate output as retail expected truth. Source repair descriptors keep
priority; dependent-owner and dependent-header blocker modes retain their
separate explicit parent routes and are not converted into retail fact packets.

## Live Validation And Acceptance

Worker feedback is read-only:

```powershell
python tools/recoil.py verify vc5-order <target> --build-root <isolated-root>
python tools/recoil.py verify call-contract --slice <slice-id> --build-root <isolated-root> --json
python tools/recoil.py verify authored-object-byte
python tools/recoil.py verify authored-byte
python tools/recoil.py verify linked-byte
```

The parent accepts only through a fresh self-validating invocation:

```powershell
python tools/recoil.py progress advance-live-order --target <target> --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --packet-id <packet-id> --build-root <packet-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
python tools/recoil.py progress advance-live-byte --lane <object|authored|linked> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Each command builds and validates once, derives the safe blocks/groups from
that invocation, and CAS-mutates from the same in-memory semantic result.
Revision drift, build conflict/failure, or divergence causes no unsafe
mutation. Direct `--apply` is normal; `--dry-run` is optional diagnostic only.
No artifact, observation receipt, or persisted content summary is imported.

The reviewed one-time migration initialization used an exact 3,380 authored
gating-body census. That number is migration evidence, not a permanent live
invariant: current qualifying population derives dynamically from reviewed
`pipeline_class` and `authored_order_role` state. The sole scheduler inserts
`authored-call-contract` after authored order and before full order. Its
deterministic, retail-monotonic slices of at most 160 bodies are cursor windows,
not atomic evidence units. In one parent invocation, an active packet and its
physical output root are authenticated, current source is freshly compiled,
expected rows are read directly from retail and the governed Binary Ninja
saved view, and each body is compared directly. Only passing bodies advance;
unrelated divergent bodies remain pending and the cursor stays on the first
slice containing one.

No stored body result, worker result, saved candidate, object, receipt, or
prior scan substitutes for the fresh comparison. Currency is maintained by
governed source/tool/manifest mutation and explicit conservative invalidation.
The reviewed implementation coordinates are currently
`CALL_CONTRACT_VERIFIER_GENERATION = 12`,
`NORMALIZER_REGISTRY_GENERATION = 12`, and
`EXPECTED_FACT_SCHEMA_VERSION = 12`. A verifier component edit invalidates all
current call-contract evidence. A normalizer registry edit invalidates every
user, or all evidence when the exact user set cannot be proven. These integer
schema coordinates do not supply retail expected truth. Each passing body
accepts only per-symbol `call_contract`.

Compatible byte lanes remain independent. Full order stays blocked until every
body is current and the parent completes one fresh, no-reuse, zero-divergence
complete-TU closeout scan:

```powershell
python tools/recoil.py progress call-contract prepare-live-convergence --closeout --build-root <fresh-root> --jobs <n> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

Incremental scans and worker receipts remain nonaccepting. The one-time
initialization route is a reviewed dry-run-first parent mutation:

```powershell
python tools/recoil.py progress call-contract initialize --expected-revision <revision> --dry-run --json
python tools/recoil.py progress call-contract initialize --expected-revision <revision> --apply --json
```

The migration guard required complete authored order, zero accepted full-order
blocks, the reviewed 3,380-body census, and no partial prior initialization.
Normal live operation must not reimpose that historical count after reviewed
classification changes.

## Manual Semantic Mutations

Every reviewed semantic change uses an exact registered route and remains
dry-run-first:

```powershell
python tools/recoil.py progress owner replace-batch --payload-json '<recoil-owner-replace-batch-v2-object>' --expected-revision <revision> --dry-run --json
python tools/recoil.py progress owner downgrade --payload-json '<recoil-owner-downgrade-v1-object>' --expected-revision <revision> --dry-run --json
python tools/recoil.py progress verification-target sync --target <target-id> --expected-revision <revision> --dry-run --json
python tools/recoil.py progress relocation-exception set --source-symbol-id <physical-symbol-id> --source-address 0xNNNNNN --payload-json '<json-object>' --expected-revision <revision> --dry-run --json
python tools/recoil.py progress relocation-target bind --source-symbol-id <physical-symbol-id> --source-address 0xNNNNNN --payload-json '<reviewed-binding>' --expected-revision <revision> --dry-run --json
python tools/recoil.py progress storage register-authored-data --payload-file <recoil-authored-data-storage-register-v1.json> --expected-revision <revision> --dry-run --json
python tools/recoil.py progress symbol set-logical-alias-group --payload-json '<recoil-logical-alias-group-v4-object>' --expected-revision <revision> --dry-run --json
```

Use `owner replace-batch` only for a complete reviewed structural replacement
guarded by exact current and replacement owner records. It is not a positive
gate/tier/metadata setter. If a required positive owner metadata, gate, or tier
mutation has no registered command, file a workspace issue rather than
inventing a generic owner operation.

Use `owner downgrade` when live evidence invalidates previously accepted owner
gates or primary-entry tiers without changing ownership. Its reviewed payload
guards the exact current owner, selected gate states, and selected primary-entry
tiers; it permits only conservative gate-state and strict tier downgrades,
creates current evidence, and preserves unrelated tracker state. Review the
dry-run, then repeat with `--apply`.

Use `relocation-target bind` when immutable retail gives the operand but its
reviewed existing or exact known-extent target identity is missing. Use
`relocation-exception set` only for genuine ambiguity. Neither route derives
expected facts from candidate output.

Use `storage register-authored-data` only after the exact known-extent authored
data symbol and its unique primary-data owner relationship already exist. Its
reviewed payload guards both current rows, canonical ids, section containment,
current absence, and non-overlap. It creates only the pending data-symbol
storage contribution and reciprocal symbol link; it creates no source edge and
changes no owner gate/tier or order/byte/provider/link/final-image acceptance.
Review the dry-run, then repeat the unchanged payload and revision with
`--apply`.

Use `symbol set-logical-alias-group` v4 only for reviewed authored bodies that
the linker coalesces. The physical function remains the sole authored
order/byte/call gate and retains exactly one address-exclusive primary owner.
Each logical member is non-gating but remains authored, has a distinct owner,
one exclusive resolved `defines` source edge with an attached live-validated
production-source mirror, and immutable-retail call-site or vtable selectors.
`original_name_status` may be `recovered` or `provisional`; neither the
descriptive name nor the exact decorated candidate object symbol supplies
retail expected truth. The v4 evidence contract deliberately separates truth
roles: retail assembly/xrefs/vtables supply candidate-independent expected
identity and selection; fresh current-source VC5 OBJ/MAP diagnostics only
corroborate the source/link mechanism. The latter must prove at least two
distinct eligible COMDAT definitions with explicit body relocation partitions,
exact inbound caller/vtable relocation bindings for every retail selector,
distinct `vc5sp3_ref_noicf` addresses, one folded `vc5sp3_ref_icf` address, and
a base-implementation negative control whose own object report demonstrates a
fold-relevant COMDAT/length/bytes/relocation/associative-section difference. Candidate
addresses and outputs never become retail expected truth. Review the complete
dry-run before `--apply`; the route changes no owner gate or tier.

Legacy progress mutations still use `--expected-revision
<transaction-revision>`. Call-contract acceptance and expected-fact/closeout
mutations require both `--expected-semantic-revision` and
`--expected-evidence-generation-revision`; claims, leases, and work close use
`--expected-scheduler-revision`. Do not mix the domain pair with the legacy
transaction guard or substitute one domain's token for another. A temporary
legacy fallback is conservatively global and does not provide independent-
domain behavior.

The linked-worktree adapter is `native-git-v1` for newly claimed tracked-write
progress reconstruction packets. `progress next` remains query-only;
`progress work claim-current` alone creates the v4 reservation and allocation
journal. Handoff requires the exact opaque baseline, packet branch, linked
worktree association, normalized nonempty writable closure, and physically
authenticated external build root. Terminal legacy v3 records stay readable
but cannot relaunch. Read-only and generated-output-only producers may remain
branchless. Integration and retirement use the shared worktree primitives
under progress authority without changing issue authority behavior.
