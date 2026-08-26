---
name: recoil-address-handoff
description: Prepare RecoilRebuild address, source-group, or owner-led handoffs. Use when you need to start work on a function address, choose the next binary-lane target, inspect owner/status/frontier context, check Binary Ninja availability, identify blockers, avoid wrong functional-lane selection, or decide whether address-led reconstruction is appropriate.
---

# Recoil Address Handoff

Start from root `AGENTS.md`. This skill expands the sole scheduler's selected
cursor into a real compact work reservation; it does not create another queue.

## Select And Expand

With no explicit assignment, run only:

```powershell
python tools/recoil.py progress next --json
```

The returned Recoil.exe phase, primary cursor, parallel byte opportunities,
launchability, and tracker revision are authoritative. Tracker ids are ledger
and concurrency keys, not candidate identities. The six stages are
`authored-function-order`, `authored-call-contract`, `authored-byte-match`,
`full-function-order`, `linked-byte-match`, and `final-validation`. Every
call-contract slice must be current before full order starts; full order does
not wait for the independent authored-byte lane. Linked bytes wait for full
order and authored bytes. Deferred owner, functional, final-data/final-image,
and `messages.dll` views never become peer schedulers.

Legacy fallback selector spellings are deprecated aliases for the same full
authored-byte cursor and have no accepted-prefix prerequisite. Only subordinate
authored-object preparation is bounded by the semantically accepted authored-
order prefix. Ordinary deferred queues remain out of the primary handoff.

Use focused views only to expand the cursor:

```powershell
python tools/recoil.py progress show <selector> --json
python tools/recoil.py progress owner relationships <selector>
python tools/recoil.py progress work leases --json
```

An address is an evidence key. Expand source or byte repair to the complete
source-shaped owner and minimal dependency closure. Never split an owner merely
because it is large. An order target is different: it may cover one translation
unit and several explicit contiguous physical-block slices while its write
scope remains exactly the packet's source/header closure.

Use current Binary Ninja evidence only when owner/source-shape or ABI facts are
actually unresolved. Check the already-open bridge and `doctor --quick --binja`
before BN-backed work; never load, switch, or patch a binary. Registered order
execution, first-divergence interpretation, and edits to an already-reviewed
model need no BN or ChatGPT Pro pass.

## Claim A Real Packet

Before launch, the parent checks active leases. With no explicit target,
including a bare `Start`, it computes available child slots from effective
runtime capacity and atomically claims all compatible current lanes without
waiting for another user confirmation:

```powershell
python tools/recoil.py progress work leases --json
python tools/recoil.py progress work claim-current --lane all --max-packets <available-child-slots> --expected-revision <revision> --apply --json
python tools/recoil.py progress handoff --packet-id <packet-id> --json
```

Priority is fixed as the active primary lane (order or call contract), full
authored byte, then subordinate authored-object byte. A blocked primary does
not suppress compatible byte work; full authored byte wins over a newly
overlapping object packet. Resource conflicts and capacity skips are
tool-owned. Render every returned packet by id and launch its compatible
worker. Individual `--lane
<primary|authored|object>` claims remain supported for focused retries or
explicit assignments.

The claim command creates and reserves the compact mode-specific packet and
derives normalized resource claims behind `packet_id`. The handoff command is
only a renderer for that active reservation. It fails closed when there is no
reservation, no lease, no writable claim, an ineligible/stale cursor, or a
mutating worker validation command. It never fabricates a work item and never
hands a parent `progress ... --apply` command to a worker.

The parent uses the registered lane selector for a scheduler-returned parallel
authored-byte or subordinate object-byte opportunity and renders only the
corresponding real reservation. The full authored-byte opportunity has
priority over new overlapping object preparation. Active work is never
preempted; stale or overlapping claims fail closed. read/read may overlap;
read/write and write/write may not. Shared headers, manifests, provider inputs,
generated outputs, canonical build windows, live BN databases, the Pro browser
profile, and ledgers conflict even when owner/block ids differ.

## Packet Modes

An `order-edit-v1` packet contains its packet id, registered target, exact
writable source/header paths, isolated build root, objective, stop condition,
and one worker command:

```powershell
python tools/recoil.py verify vc5-order <target> --build-root <packet-root>
```

The worker edits the packet scope and repeats that command until PASS or a
concrete out-of-scope owner/block/header contradiction. The hot loop has no
Binary Ninja, byte validation, tracker mutation, artifact/evidence package,
candidate-content qualification, hard-byte evidence bundle, or routine Pro obligation. A target may
cover several explicit contiguous block slices; PASS is source feedback only.
An unresolved row anywhere in that interval blocks the whole physical-block
packet and live acceptance even if a resolved-subset raw diagnostic passes.

A `call-contract-edit-v1` packet contains its packet id, deterministic
retail-contiguous slice id and symbol/block/target scope, exact writable
source/header paths, isolated build root, objective, stop condition, and one
worker command:

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --progress .agent/RECONSTRUCTION_PROGRESS.sqlite3 --build-root <packet-root> --json
```

The worker repeats only that nonmutating command until PASS or the first exact
call target/form/dispatch/storage/slot/cleanup divergence or concrete scope
contradiction. The command may read retail and the already-open Binary Ninja
database. PASS is call-contract feedback only and accepts no order, byte,
owner, provider, gate, or tier state.

A byte-repair packet uses the complete owner and the canonical byte/tier skill
only when the selected lane requires it. Retail relocation expectations are
preflighted through:

```powershell
python tools/recoil.py audit relocation-expectations --json
```

They derive from immutable retail plus accepted typed identity/provider/alias
facts. Explicit empty expectations are valid. Ambiguity blocks before an
expensive build and routes to the narrow reviewed exception command; candidate
output never supplies expected facts.

If the retail operand is deterministic but its existing or exact known-extent
target identity is missing, route the parent to `progress relocation-target
bind` with dry-run-first review. Keep `progress relocation-exception set` for
genuine ambiguity; the two routes are not interchangeable.

ChatGPT Pro is escalation-only under `recoil-source-model-recovery` and
`recoil-tier-verification`: unresolved competing source models, a materially
disputed cross-boundary correction, raw-assembly escalation after credible C++
variants fail, or an explicit user request. Workers return a scoped request to
the parent when one of those triggers occurs; they do not invoke the Pro line
or upload attachments themselves.

## Parent Acceptance And Return

Workers never accept tracker state. The parent independently rebuilds current
source and accepts from that same invocation:

```powershell
python tools/recoil.py progress advance-live-order --target <target> --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-byte --lane <object|authored|linked> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Direct `--apply` is normal for these self-validating commands; optional
`--dry-run` is diagnostic only. Manual owner, block, provider, classification,
catalog-exception, and tier mutations remain dry-run-first.

The worker returns only packet id, outcome, changed paths, exact validation
result, first divergence, and any concrete scope contradiction. The parent
retains claim arbitration, integration, acceptance, issue reporting, and final
authority. Do not run git, clear `.devspace`, or make durable evidence depend
on a session-scratch path.
