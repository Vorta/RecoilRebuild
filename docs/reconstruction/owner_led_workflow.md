# Source-Owner Mechanics

This document explains owner membership, gates, entry tiers, and safe mutation.
It is not a scheduler. With no explicit target, begin only with:

```powershell
python tools/recoil.py progress next
```

Then expand that cursor into its complete physical block, semantic span, and
source-shaped owner. Other owner/work/section/final and companion-binary views
are deferred context and never outrank the global pipeline.

## Owner Unit

Function addresses are evidence anchors. The accepted authored unit is the
proven original source construct: a class/interface, source-file cluster,
subsystem, callback/record/table/global object/static class-member group, or a
true standalone leaf. Literal, constant, global, or storage-range packets are
auxiliary data prerequisites unless evidence proves that exact authored
construct. Link auxiliary packets to their primary source-shaped owner and
treat orphan packets as parent-reconciliation blockers.

`.agent/RECONSTRUCTION_PROGRESS.json` is the only owner/gate/entry-tier and
reconstruction-progress authority. Never hand-edit it. Inspect joined state:

```powershell
python tools/recoil.py progress show <owner-id-or-address>
python tools/recoil.py progress find <query>
python tools/recoil.py progress audit --scope owners --strict
```

`progress show` must distinguish anchors, primary function/data membership,
owner dependencies, physical blocks, semantic spans, work items, owner linkage,
linked-image identity, entry-local bytes, owner byte gate, and global prefixes.

Only the parent mutates owner state. Preview each nontrivial operation, review
its revision and cross-entity effects, then apply the identical command:

```powershell
python tools/recoil.py progress owner <operation> ... --dry-run
python tools/recoil.py progress owner <operation> ... --apply --expected-revision <revision>
python tools/recoil.py progress audit --scope all --strict
```

## Gate And Tier Meaning

- `boundary`: complete owner extent is proven.
- `source`: the original higher-order source model is implemented.
- `data`: all touched authored `.data`, `.rdata`, and BSS facts are complete.
- `functional`: current owner/target behavior evidence exists.
- `linkage`: typed owner membership and dependency relationships are correct.
- `byte`: complete owner-scoped byte/provider ABI evidence is accepted.

Every primary function/data entry stores a tier and evidence. The owner tier is
derived from the entry floor and applicable gates; it is never set directly.
`C` covers behavior, `B` adds accepted owner/data/linkage shape, `A` adds
reviewed near-identity evidence, and `S` requires exact byte/provider ABI
evidence for every primary entry plus the owner byte gate. Auxiliary data `S`
means that dependency is byte-ready, not that its parent owner is complete.

Owner gates are local acceptance properties. They do not authorize out-of-phase
scheduling, later-owner work ahead of `progress next`, or byte work outside the
scheduler-returned primary or `parallel_authored_byte_cursor`. The deprecated
fallback cursor is only a compatibility alias for that full authored-byte
cursor and has no accepted-prefix prerequisite. Only subordinate
`parallel_authored_object_byte_cursor` preparation is limited to the current
semantically accepted authored-order prefix. Order validation and acceptance
rerun the configured object/TU and linked comparisons directly; they do not
depend on stored file identity. Byte and final validation likewise compare
current compiled outputs directly; they never silently prove source shape. All
positive boundary/source/data/tier-B-or-better claims require the scrutiny
workflow in root `AGENTS.md`.

Tracker mutations use the reviewed monotonic revision as their sole
concurrency guard. Durable evidence records semantic scope, direct comparison
results, and any material evidence paths without binding validation to file
identity.

The address-specific `0x4e5954..0x4e5a50` orphan-data exception described in
root `AGENTS.md` remains non-reusable.
