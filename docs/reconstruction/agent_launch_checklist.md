# Agent Launch Checklist

Compact reminder only. Root `AGENTS.md` is authoritative; the executable
pipeline is [`retail_executable_reproduction.md`](retail_executable_reproduction.md).

## Select And Claim

With no explicit target:

```powershell
python tools/recoil.py progress next --json
python tools/recoil.py progress work leases --json
python tools/recoil.py progress work claim-current --lane all --max-packets <available-child-slots> --expected-revision <revision> --apply --json
```

`progress next` is the sole Recoil.exe scheduler. It keeps
`authored-function-order` is primary until complete, then the deterministic
`authored-call-contract` slices gate the restart of `full-function-order`
without waiting for the independent authored-byte lane.
`linked-byte-match` waits for both; `final-validation` follows. Owner, section,
functional, final-data, final-image, and ordinary `messages.dll` views are
deferred context, not peer queues.

A bare `Start` is enough: the root parent computes remaining child slots from
effective runtime capacity, applies the multi-lane claim without waiting for
more user confirmation, and launches every compatible returned packet. Fixed
priority is primary order, full authored byte, then subordinate authored-object
byte; here the primary packet category means the current primary lane, order or
call-contract. A blocked primary does not suppress compatible bytes; full
authored byte wins over overlapping new object work.
Conflicts and capacity skips are tool-owned. Render each real reservation by
packet id:

```powershell
python tools/recoil.py progress handoff --packet-id <packet-id> --json
```

Individual `--lane <primary|authored|object>` claims remain available for a
focused retry or explicit assignment.
The handoff exists only while that reservation and lease are active. It names
packet mode/id, target, covered blocks, writable paths, one non-mutating worker
command, objective, stop condition, and return fields. It fails closed for an
absent lease, empty write closure, or parent-only acceptance command.
`progress next` never allocates. A newly claimed tracked-write progress packet
uses the `native-git-v1` v4 contract and is handoff-visible only after its
baseline, packet branch, exact linked association, writable closure, and
physical external-root identity are complete. Terminal legacy v3 packets are
readable but cannot relaunch.

## Worker Loop

For `order-edit-v1`, run only the packet command:

```powershell
python tools/recoil.py verify vc5-order <target> --build-root <packet-root>
```

Edit the listed source/header closure and repeat until PASS or a concrete
out-of-scope contradiction. The registered target may cover several explicit
contiguous physical block slices. This loop does not need Binary Ninja, byte
evidence, Pro, artifact packaging, saved-content qualification, or tracker
mutation.

An unresolved row anywhere in the target interval blocks the whole block
packet and acceptance. A resolved-subset raw diagnostic PASS is useful feedback
only; it cannot launch or accept a partial block.

The worker returns packet id, changed paths, exact command/result, first
divergence, and any scope contradiction. The parent independently validates,
then closes or routes the packet.

For `call-contract-edit-v1`, run only the packet's nonmutating `verify
call-contract` command. The parent acceptance command performs its own single
fresh build and direct retail comparison and may advance only bodies that pass
in that invocation. Stored body results and worker output are nonaccepting.
Currency uses governed invalidation and the reviewed integer coordinates
`CALL_CONTRACT_VERIFIER_GENERATION = 11`,
`NORMALIZER_REGISTRY_GENERATION = 11`, and
`EXPECTED_FACT_SCHEMA_VERSION = 11`. Full-order transition additionally
requires the fresh complete no-reuse zero-divergence closeout.

A linked WSI-20260809-007 continuation is two claims, not an alternate
scheduler. Primary `claim-current` first creates a branchless
`call-contract-continuation-producer-v1`; the parent derives the exact route
descriptor from its fresh exhaustive verifier result. Only a later primary
claim creates the one-hop `call-contract-repair-continuation-edit-v2` child.
The child cannot accept evidence; fresh parent `advance-live-call-contract`
with a distinct parent-only acceptance packet is still required.

## Parent Acceptance

Use one fresh acceptance invocation:

```powershell
python tools/recoil.py progress advance-live-order --target <tracker-target-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress advance-live-byte --lane <object|authored|linked> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Direct `--apply` is normal because each command rebuilds, validates, derives
the safe semantic prefix, and performs one revision-guarded mutation from the
same result. `--dry-run` is an optional diagnostic, not a mandatory first build.
Reviewed owner topology/membership replacement uses dry-run-first `progress
owner replace-batch`; conservative gate/tier decreases use `progress owner
downgrade`; verification-target registration metadata uses `progress
verification-target sync`. Unsupported positive owner metadata, gate, or tier
changes route to a bounded `issue request`, never an improvised mutation.

For authored relocations, use `progress relocation-target bind` when retail
determines the operand but its existing or exact known-extent target identity
is missing. Use `progress relocation-exception set` only for genuine reviewed
ambiguity. Both are manual dry-run-first mutations; candidate output is never
expected truth.

## Assigned Entity Or Binary Ninja

For focused context:

```powershell
python tools/recoil.py progress show <id|address>
python tools/recoil.py progress find <query>
python tools/recoil.py progress owner relationships <id|address>
```

An address is evidence, not the default edit owner. Expand source work to the
complete proven owner and its assigned paths. Keep source owners, physical
blocks, semantic spans, storage, output sections, and final coverage distinct.

Use Binary Ninja only when current binary facts are required. Work in the
already-open maintained analysis artifact; never load, switch, or patch a
binary. Read-only use requires a stable saved view and no active writer.
Mutation requires one parent-assigned `recoil_bn_reconstructor` writer lease
through reanalysis, propagation checks, save, and return.

## Non-Address Maintenance

Do not select an address or require Binary Ninja. Use a bounded
`recoil_tool_maintainer` packet and validate with:

```powershell
python tools/recoil.py doctor --infrastructure-only
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py audit workflow-contracts --strict
python tools/recoil.py audit pipeline-reachability --strict
```

`agent-surface` is static alignment; the latter two exercise actual command and
expected-fact paths.

## Final Reminders

- Never treat persisted summaries, saved receipts, a worker PASS, or a raw
  whole-file delta as candidate acceptance.
- Never clear or durably depend on `.devspace`; keep material facts in their
  canonical source/doc/tracker destination.
- The orchestrator owns packet branches, linked worktrees, integration,
  retirement, and hygiene. A tracked-write issue or progress worker may stage only the exact writable closure
  and create exactly one nonaccepting packet-id commit with the
  packet id in its message; it never
  switches branches, modifies `master`, or removes a worktree/build root.
- Complete every fallible compiler, test, audit, and doctor check in the
  temporary integration worktree before fast-forwarding `master`. Afterwards,
  perform only deterministic Git, topology, tag, and physical-identity
  assertions.
- In a linked validation, load tracked source, tools, tests, policies, target
  manifests, and `.agent/REFERENCE_EXECUTABLE.json` from the executing
  worktree. Resolve machine-local `support/Recoil.exe` and live progress/issue
  SQLite inputs through the validated canonical control root; never copy or
  link them into the linked checkout.
- The authored-object byte cursor is subordinate evidence preparation, not a
  peer scheduler. Deprecated aliases have no accepted-prefix prerequisite.
- ChatGPT Pro is reserved for genuine competing source models, a material
  cross-boundary dispute, raw-assembly escalation, or an explicit user request.
