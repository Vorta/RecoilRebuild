# Agent Launch Checklist

Compact reminder only. Root `AGENTS.md` is authoritative; the canonical global
pipeline is [`retail_executable_reproduction.md`](retail_executable_reproduction.md).

## No Explicit Target

```powershell
python tools/recoil.py progress next
python tools/recoil.py progress next --json
python tools/recoil.py progress audit --scope all --strict
python tools/recoil.py doctor --quick --binja
```

`progress next` is the sole Recoil.exe scheduler. It follows the canonical
order-primary sequence `authored-function-order` -> `full-function-order`, with
an independent retail-monotonic authored-byte lane. `linked-byte-match` starts
only after full order and authored bytes are both complete, followed by
`final-validation`. Expand the primary cursor into the complete physical block,
semantic span, and source-shaped owner. If it returns
`parallel_authored_byte_cursor`, that row is bounded secondary-lane work only:
it does not move the primary cursor or phase, and it must not overlap the active
order block, owner, writable paths, or mutable build step. Use `progress handoff
--authored-byte --json`; the old fallback cursor/flag are deprecated aliases.
If `parallel_authored_object_byte_cursor` is returned, use `progress handoff
--authored-object-byte --json`. That packet prepares only one exact object body
inside the accepted authored-order prefix, uses a packet-unique
`build/vc5-authored-object-byte/...` root, and never runs a whole-project/final
build. Version 1 advances address-at-a-time; owner-bundle production and
`--through` acceptance are intentionally deferred.
Owner/work/section/final
and ordinary `messages.dll` views are deferred context unless the cursor records
them as required dependencies.

## Assigned Entity

```powershell
python tools/recoil.py progress show <binary-qualified-id|owner-id|address|block-id|semantic-id>
python tools/recoil.py progress find <query>
python tools/recoil.py progress owner relationships 0xNNNNNN
python tools/recoil.py doctor --quick --binja
```

Binary Ninja is a maintained analysis artifact. Work only in the already-open
target and never load, switch, or patch a binary. New active-scope evidence lets
the parent assign a bounded `recoil_bn_reconstructor` correction without another
user approval or tracker mutation. Only that role may edit, reanalyze, and save;
its read-only filesystem sandbox does not make assigned BN MCP state immutable.
It requires one parent-assigned exclusive writer lease for that live database,
held through reanalysis and save; no reader or second writer may use the same
database until the lease is released. It may not decide owner/block/order/
provider/tier acceptance. Address is
traversal evidence; implementation and owner acceptance
use the complete proven source-shaped owner. Keep physical blocks, semantic
spans, source owners, data symbols, owner data gates, physical storage
contributions, PE output sections, work items, linked-image state, and evidence
freshness distinct. Unknown extents have no guessed size/end.

## Non-Address Maintenance

Inspect only assigned surfaces. Do not run `progress next` or require Binary
Ninja unless the defect depends on current binary facts. Use
`python tools/recoil.py doctor --infrastructure-only` when broad infrastructure
health is useful. Tool/docs/skill/role upgrades normally go through a
parent-assigned `recoil_tool_maintainer`.

## Handoff Essentials

Generate handoffs from structured work items with:

```powershell
python tools/recoil.py progress handoff --json
```

The handoff names target binary, global phase, cursor/range, physical block,
semantic span, complete owner, work item, first unresolved item, exact next
command, exit gate, role, exact allowed and forbidden paths, ordered validation
commands, non-overlap statement, evidence, and required return fields. It fails
closed when any required envelope field is missing, broad, overlapping, or
mutation-bearing. Source workers own hard-byte artifact production/upload;
verifiers validate the supplied synchronized triplet and call order mismatches
`order-gate failure`. A source-owner mapper that needs ChatGPT Pro returns a
complete `needs_pro` bundle and releases its worker slot. The parent broker
obtains one receipt for the scoped request and supplies it when the mapper
resumes; workers do not wait on or compete for the global browser lock.

Only the parent mutates the unified tracker, always dry-run first and with the
reviewed revision on apply. Agents never run git commands or report
version-control state. Subagents never clear or durably depend on `.devspace`;
the parent waits for all consumers, promotes material evidence, runs strict
checks, and clears `.devspace` as the final workspace action.

`audit final-data` and `audit final-repro` emit tracker-bound observed evidence
only. Their import commands accept nothing; they are not work units, owner
action generators, or peer schedulers. Use explicit `progress accept storage`
and `progress accept section` review, and require mandatory whole sections plus
an exact final-repro receipt before `progress accept final`.
