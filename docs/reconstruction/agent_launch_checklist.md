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
`authored-function-order` -> `authored-byte-match` -> `full-function-order` ->
`linked-byte-match` -> `final-validation` sequence. Expand its primary cursor
into the complete physical block, semantic span, and source-shaped owner. If it
returns `fallback_authored_byte_cursor`, that authored/lifecycle row is bounded
fallback work only: it does not move the primary cursor or phase. Owner/work/section/final
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
It may not decide owner/block/order/provider/tier acceptance. Address is
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
`order-gate failure`.

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
