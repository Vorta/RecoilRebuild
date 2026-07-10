# Agent Launch Checklist

Compact launch reminder only. Root `AGENTS.md` is authoritative for evidence,
owner gates and tiers, source shape, provider boundaries, Binary Ninja,
ChatGPT Pro triggers, raw assembly, subagent authority, and the no-git rule.
`.agent/AGENTS.md` is a compatibility pointer.

## Choose The Workflow

- Address-led reconstruction or verification: use the preflight and scheduling
  commands below, then expand the address to its complete owner.
- Docs, tooling, skills, roles, or instruction cleanup: inspect only the
  assigned surfaces and run targeted checks. Do not select an address or
  require Binary Ninja unless the defect depends on current BN evidence.
- Workspace issue or agent-surface repair: the parent should spawn
  `recoil_tool_maintainer` by default. Run:

```powershell
python tools/recoil.py issue show <issue-id>
python tools/recoil.py audit agent-surface --strict
```

Agent-surface changes are governed by root `AGENTS.md`; they cannot weaken its
reconstruction rules or reopen parent-only ledgers and acceptance authority.

## Preflight

For BN-backed reconstruction:

```powershell
python tools/recoil.py doctor --quick --binja
```

Use plain `doctor --quick` only when BN is intentionally irrelevant. Check the
already-open target with the Binary Ninja bridge; never load, switch, or patch
a binary from an agent.

For non-address tool, docs, skill, role, or ledger maintenance, use
`python tools/recoil.py doctor --infrastructure-only` when broad infrastructure
health is useful. It skips reconstruction/source-evidence checks and never
replaces the full address-led preflight above.

For a known address:

```powershell
python tools/recoil.py status 0xNNNNNN
python tools/recoil.py owner show 0xNNNNNN
python tools/recoil.py owner relationships <owner-id-or-address> --json
python tools/recoil.py frontier 0xNNNNNN --depth 1 --lane binary
```

## No-Target Scheduling

Resume visible WIP before starting new work and compare both binaries:

```powershell
python tools/recoil.py audit groups --summary --wip-limit 4
python tools/recoil.py audit groups --binary messages --summary --wip-limit 4
python tools/recoil.py owner audit --strict
python tools/recoil.py owner next --lane binary
python tools/recoil.py owner next --binary messages --lane binary
python tools/recoil.py owner next --lane binary --json
python tools/recoil.py owner next --binary messages --lane binary --json
python tools/recoil.py audit sections --strict
python tools/recoil.py audit sections --pressure
```

Choose the first actionable active group, global work unit, or primary
source-shaped owner reported by current output. Auxiliary data packets belong
under their primary parent; orphan packets are parent-reconciliation blockers.
Recoil.exe and `messages.dll` workers may run together only when BN database
targets, sections, source paths, ledgers, and generated outputs do not overlap.
Route companion `Reconstructed` blockers to a BN reconstructor with target
binary `messages` when appropriate.

## Owner And Source-Block Essentials

Addresses are evidence anchors, not default work units. Use the complete
primary source-shaped owner for implementation and owner-scoped tier `S`.
Do not split a non-standalone owner because it is large. A ready owner may
advance while unrelated owner, data, or final-lane debt remains open.

Obtain the live source-block frontier from the catalog, never from onboarding
text:

```powershell
python tools/recoil.py audit source-blocks --list
python tools/recoil.py audit source-blocks --strict
```

Use `agent_source_path` for source-block work. Root `AGENTS.md` defines the
physical-block, original-style header/include, and generated-order rules. BN
function names and comments are provisional navigation labels. The
source-shape triggers include source-path literals, neighboring function order, class/table
evidence, and a generated VC5 order mismatch; route interpretation through the
full root policy.

Scheduling sections are parent-owned and do not prove source ownership:

```powershell
python tools/recoil.py section show <plan-group>
python tools/recoil.py section move <plan-group> <section-id> --reason "..." --dry-run
python tools/recoil.py owner show <owner-id>
python tools/recoil.py owner set-section <owner-id> <section-id> --evidence "..." --dry-run
```

## Parent Orchestration Loop

The parent schedules, integrates, validates, assigns BN scope, and owns owner
gates/tiers, workspace issues, and final claims. Parent source edits are limited
to small integration fixes after worker return or recorded delegation-impossible
exceptions. For production source implementation, spawn `recoil_source_worker`.

Use narrow roles as a pipeline:

- BN fact mapper: raw fact packet only.
- BN reconstructor: one non-overlapping already-open BN scope.
- Source-owner mapper/scrutinizer: read-only boundary packet or `ALLOW`/`BLOCK`.
- Provider/data classifier and scaffold auditor: narrow classification/audit.
- Source worker: one complete assigned owner and explicit write set.
- Verifier: assigned commands and ignored build/probe artifacts only.
- Tool maintainer: one assigned workspace/tool/docs/skill/role repair.

Minimum source-worker handoff fields: section, complete owner, anchors, allowed
and forbidden paths, evidence, expected source model, exact checks, and return
packet fields. Minimum verifier handoff fields: section, validation scope,
anchors, exact commands, evidence, forbidden paths, and return fields. Minimum
tool-maintainer handoff fields: issue or upgrade, actual/expected behavior,
allowed and forbidden paths, exact checks, and return fields. Tool maintainers
may edit only assigned governed paths and return issue-resolution candidates;
subagents never file, resolve, reopen, or directly edit workspace issues.

Before launching live source-worker or verifier blocks from markdown:

```powershell
python tools/recoil.py audit handoff --path .agent/IMPLEMENTATION_GROUPS.md --strict
```

The active `[agents].max_threads` value counts the parent and all children.
Never overlap owner boundaries, write paths, manifests, ledgers, generated
outputs, or BN state.

## Escalation Triggers

The source-discovery ChatGPT Pro policy in root `AGENTS.md` applies before any
new, changed, disputed, or acceptance-relevant owner/block/order conclusion.
Use `chatgpt-pro-line`; mechanical lookup of already-accepted durable facts and
raw BN fact packets that make no placement/ownership recommendation retain
their documented exemptions. Record the receipt or specific exemption.

For hard byte-match work, use `chatgpt-pro-line` at the triggers defined in
root `AGENTS.md` before repeated probes or raw assembly. The full C/C++-first,
evidence, docblock, and `.agent/RAW_ASSEMBLY_ALLOWLIST.txt` requirements remain
binding; this checklist does not restate or relax them.

Return rule conflicts or broken validation paths to the parent as issue
candidates. Do not hide them behind weaker evidence or a less faithful source
model.

## Quiet And Final Handoff

Quiet mode: do not send routine progress reports. Message only for required
input, a true blocker, worker handoff decision, validation failure, or final
result. Any unavoidable interim update must be one short sentence.

Agents must not run git commands or report version-control status. Before
ending multi-step address-led reconstruction:

```powershell
python tools/recoil.py handoff 0xNNNNNN --include-artifacts
```

State changed paths, checks and results, owner/gate/tier changes made only by
the parent, the durable-documentation decision, and remaining blockers.
