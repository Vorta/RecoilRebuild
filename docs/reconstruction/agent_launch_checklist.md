# Agent Launch Checklist

Compact launch reminder for address-led reconstruction. It does not replace
`AGENTS.md`; Binary Ninja and `.agent/RECOIL_PLAN.md` remain authoritative for
function evidence and plan state.

`AGENTS.md` and `.agent/AGENTS.md` are local instruction surfaces.
`tools/` and `tests/tools/` are required local verification infrastructure.
Agents must not run git commands or report version-control state; private
inputs and generated state remain local workspace details.
Durable owner scopes live in `.agent/SOURCE_OWNERS.json` and must be inspected
or updated through `python tools/recoil.py owner ...`, not by hand.

## Preflight

From the workspace root:

```powershell
python tools/recoil.py doctor --quick --binja
```

Use plain `--quick` only when Binary Ninja is intentionally unavailable or
irrelevant. For documentation or tooling cleanup, inspect target files and run
targeted checks instead of selecting an address. For active implementation or
verification handoff, also run the address-specific doctor command from
`AGENTS.md`.
The quick doctor includes temporary group WIP hygiene and avoids any
version-control state checks.

For agent-facing command, doc, skill, or role drift:

```powershell
python tools/recoil.py audit agent-surface --strict
```

Agent-surface evolution is a governed tool-maintenance change. Allowed
agent-surface evolution write paths are `.codex/skills/recoil-*`,
`.codex/agents/*.toml`, `tools/recoil.py`, `tools/_recoil`, `tests/tools`, and
focused docs. Nontrivial updates need a direct user request or reproducible
process/tooling need, and the parent uses `recoil_tool_maintainer` by default
for implementation while keeping scheduler/integrator authority. Agent-surface
evolution must not introduce new marker criteria, must not weaken evidence or
provider boundaries, must not edit production source, must not change Binary
Ninja state, and must not mutate `.agent` ledgers unless separately authorized
by the existing workflow. Do not create broad/general reconstruction roles, fake
provider models, or overlapping subagent ownership.

For generated public status and local artifact inventory:

```powershell
python tools/recoil.py docs readme-progress --check
```

Run without `--check` only when intentionally refreshing the visitor-facing
`README.md` generated progress tables from the current Recoil.exe and
`messages.dll` plans.

For generated local artifact inventory and cleanup planning:

```powershell
python tools/recoil.py audit artifacts
```

`audit artifacts` is dry-run by default. Review the selected stale roots before
rerunning with `--delete`.

If a tool, instruction, environment, workspace setup, workspace rule, or
validation path blocks work or forces a workaround, use the `AGENTS.md` issue
commands and boundary rules. Do not file normal reconstruction backlog, stale
tests/code/manifests/markers, missing or unregistered smokes, owner/data
blockers, tier debt, or missing evidence as workspace issues. Do file or return
an issue candidate when the workspace rule itself blocks source-faithful code or
would force weaker evidence, marker criteria, source faithfulness, or provider
boundaries. The report must name the blocking rule surface, explain the
source-faithfulness impact, cite the BN/source evidence, describe why safe
alternatives do not work, and suggest the corrected rule wording or behavior
for user review. When a workspace issue is reported or a local tool upgrade is
requested, inspect the focused issue/tool surface, then spawn
`recoil_tool_maintainer` by default for the repair. Parent tool edits are
limited to small integration fixes after worker return, or cases where
delegation is impossible; record the exception before editing or in the final
report.

Address-led launch packet for a known anchor:

```powershell
python tools/recoil.py packet --address 0xNNNNNN
```

Use `--no-binja` only when intentional. Do not use address-led packets for
docs/tooling cleanup or other non-address work; inspect the relevant files and
run targeted checks instead.

## Task Selection

Resume active WIP before starting new work:

```powershell
python tools/recoil.py audit groups --summary --wip-limit 4
python tools/recoil.py audit groups --binary messages --summary --wip-limit 4
python tools/recoil.py owner audit --strict
python tools/recoil.py owner next --lane binary
python tools/recoil.py audit sections --strict
python tools/recoil.py audit sections --pressure
```

If active groups exist, choose the first actionable group in
`.agent/IMPLEMENTATION_GROUPS.md` or `.agent/IMPLEMENTATION_GROUPS_MESSAGES.md`,
then follow its anchor or next action with target-qualified status/frontier
checks. Compare the Recoil.exe and `messages.dll` queues instead of assuming the
default executable queue is the only open work. Skip a group only when current
BN, plan, or source evidence proves it stale, contradicted, completed, or
explicitly lower priority than another active group.

Use current plan state, not stale notes. Run `plan next --lane binary` only
when no active group exists for that target, active groups have been
refreshed/pruned or proven unactionable, or the user explicitly directs new
work:

```powershell
python tools/recoil.py plan next --lane binary
python tools/recoil.py plan next --binary messages --lane binary
python tools/recoil.py plan batch --lane binary
python tools/recoil.py plan batch --lane binary --spawnable-only
python tools/recoil.py plan batch --binary messages --lane binary --spawnable-only
python tools/recoil.py plan batch --lane binary --json
python tools/recoil.py plan batch --lane binary --handoff-template
python tools/recoil.py section show ui.zhud
python tools/recoil.py plan group app.recoil_app --lane binary
python tools/recoil.py owner show 0xNNNNNN
python tools/recoil.py status 0xNNNNNN
```

`plan next --lane binary` and `plan next --binary messages --lane binary` print
target-qualified `primary`, `secondary`, and `tertiary` ranked owner/work
scopes. `audit sections --pressure` summarizes scheduling risk and spawnable
capacity. `plan batch --lane binary` and
`plan batch --binary messages --lane binary --spawnable-only` print
section-isolated worker candidates for parallel scheduling; add
`--spawnable-only` for live handoffs that exclude pathless or parent-narrowing
blocks, and add `--json` or `--handoff-template` for machine-readable output or
a parent batch card.
The parent may schedule Recoil.exe and `messages.dll` workers in the same batch
only when BN database targets, sections, source paths, ledgers, and generated
outputs do not overlap.
If evidence shows a plan group is in the wrong scheduling section, inspect with
`section show`, then the parent validates `section move <plan-group>
<section-id> --reason "..." --dry-run` before applying the same command without
`--dry-run`. Workers should recommend section moves only; they must not edit the
section catalog.

Use `plan reclassify` for existing authored/provider entries. Use
`python tools/recoil.py plan add-provider-boundary ... --dry-run` only when BN
proves a provider/compiler/import boundary is absent from the plan. Do not
hand-edit provider marker blocks.

## Parent Orchestration Loop

The parent agent is an orchestrator, not the default implementer. After
selecting active WIP or binary-lane work, partition non-overlapping owner/source
scopes and spawn `recoil_source_worker` agents for source/test edits.

Use the role pipeline deliberately: the parent schedules and integrates;
workspace/BN fact/owner/provider/scaffold roles return evidence packets only;
`recoil_bn_reconstructor` performs one assigned BN-state slice; source workers
edit one assigned slice; tool maintainers fix one assigned workspace/tool
issue; verifier agents run targeted checks after the parent fixes the
validation scope.

Create a short parent batch card before any implementation or verification
handoff. It should record the task kind, active group or address, evidence
packets required and received, worker allocation, validation scope, and exit
criteria.

Source-worker handoffs must name the owner/source scope, selected section,
address or group anchor, allowed and forbidden paths, evidence inputs, expected
source model, and narrow validation commands. Do not assign overlapping
production source files, verification manifests, generated outputs, BN database
state, or `.agent` ledgers.

Minimum source-worker handoff fields:

- Owner/source scope.
- Section.
- Anchor addresses or group.
- Allowed write paths and forbidden paths.
- Evidence packet inputs.
- Expected source model.
- Exact validation commands.
- Return packet fields: changed files, evidence used, checks run, blockers,
  overlap warnings, and non-authoritative marker recommendations.

Minimum verifier handoff fields:

- Validation scope.
- Section.
- Anchor addresses or group.
- Exact commands.
- Evidence packet inputs.
- Forbidden paths.
- Return packet fields: exact command lines, pass/fail results, key output
  lines, failure category, and next narrow verification command.

Minimum tool-maintainer handoff fields:

- Workspace issue id or requested tool upgrade.
- Area and current behavior.
- Expected behavior.
- Allowed write paths and forbidden paths.
- Exact validation commands.
- Return packet fields: changed files, command results, validation gaps,
  blockers, and issue-resolution candidate text.

Minimum BN reconstructor handoff fields:

- Already-open binary target.
- Non-overlapping address/function, dependency cycle, owner-sized BN cluster, or
  type/global/table packet.
- Exact allowed BN state changes and forbidden BN actions.
- Evidence packet inputs.
- Reanalysis and save expectations.
- Return packet fields: inspected addresses/types/globals, exact BN changes,
  evidence used, propagation checked, reanalysis/save status, and blockers.

For `messages.dll`, unresolved `Reconstructed` blockers should normally be
assigned to `recoil_bn_reconstructor` with target binary `messages`.

Before launching live markdown handoff blocks, run:

```powershell
python tools/recoil.py audit handoff --path .agent/IMPLEMENTATION_GROUPS.md --strict
```

Workers may inspect BN and edit only assigned source/test files. Tool
maintainers may edit only assigned tool/docs/skill/role/test files. Only
`recoil_bn_reconstructor` may change BN names/types/comments or save BN, and
only inside its parent-assigned non-overlapping BN scope. Subagents must not
update plan markers, file workspace issues, run git commands, or select
follow-up work. They return changed paths, evidence or command results, checks,
blockers, and non-authoritative recommendations. The parent reviews changed
files, reruns checks, assigns and accepts BN/plan/issue work, and owns final
claims.

Parent source edits are limited to small integration/conflict fixes after worker
return, or cases where delegation is impossible; record the exception before
editing.

Quiet mode: do not send routine progress reports. Message the user only for
required user input, true blockers, worker handoff decisions, validation
failures, or final results. Any unavoidable interim update must be one short
sentence with no evidence dump or command output unless requested.

Normal binary-lane selection is owner-first after reconstruction/dependency
readiness: resolve owner structure and touched data before isolated behavior or
pure code/function tier `S` work. Data-entry `S` work follows the data entry's
local owner/data/byte gates. For the detailed owner/data gates, use
`owner_led_workflow.md` and `data_owner_audit.md`; this checklist only names
the launch commands.

Treat the address as an evidence anchor, not necessarily the implementation
unit. Expand to the proven owner boundary: class/interface, table-shaped
dispatch owner, provider boundary, source-file cluster, or dependency group.
Identify every affected address before editing source, BN state, plan markers,
VC manifests, or group notes.
Create or update the matching source-owner record before accepting owner/data
markers. The plan CLI rejects positive owner/data/tier-B+ updates unless the
linked owner gates prove the higher-order source/data scope.

If BN proves an authored class/interface/method cluster, restore that owner
before any `Reimplemented` tier or `Model: source-faithful`. Flattened functions
and production `VTable`/`FTable` scaffolds are not accepted reimplementations.

Before ending a multi-step reconstruction session:

```powershell
python tools/recoil.py handoff 0xNNNNNN --include-artifacts
```

## Local Groups

`.agent/IMPLEMENTATION_GROUPS.md` is temporary. If stale or contradicted by BN,
`.agent/RECOIL_PLAN.md`, or `recoil.py status`, refresh or prune it after
moving durable facts elsewhere. Active groups are the default no-address
startup queue; do not start unrelated new work while actionable WIP remains.

## Native Build Shell

Native builds/tests need an x86 MSVC environment. Prefer Visual Studio MCP for
already-open or safely generated `build/vs-x86/RecoilRebuildNative.slnx` work.

From normal PowerShell, use the wrapper:

```powershell
powershell -ExecutionPolicy Bypass -File cmake\recoil_native_x86_build.ps1 -Preset ninja-x86-debug
```

The wrapper loads `vcvarsall x86` and runs
`python tools/recoil.py env --native-x86`. A missing `kernel32.lib` is an
environment problem, not source evidence.

Use `python tools/recoil.py build msvc-x86 -- ...` for `ctest`, native smokes, and other
x86 MSVC commands. Do not call Visual Studio batch files under `Program Files`
directly. If `cl.exe`, `INCLUDE`, `LIB`, the x86 target, Windows SDK,
`support/sdk`, or `support/Recoil.exe` is missing, switch environments or ask
for the missing private input.

## Source Placement

Before creating or moving implementation files:

```powershell
python tools/recoil.py audit source-map --check docs/reconstruction/source_file_map.md
```

Use `source_file_map.md` plus current BN source comments and call-site evidence.
If `--check` reports stale, regenerate only when current source movement,
provenance docblocks, or legacy source comments explain the drift.
Regenerate explicitly:

```powershell
python tools/recoil.py audit source-map --update --output docs/reconstruction/source_file_map.md
```

For docblock checks, audit touched source files before marker work:

```powershell
python tools/recoil.py audit docblocks --path src/path/to/touched_file.cpp --summary --max 50
```

A broad `python tools/recoil.py audit docblocks --path src --summary --max 20`
run currently reports legacy backlog and should be treated as status debt, not
as a reason to block unrelated non-address workspace cleanup.

Check:

- `docs/reconstruction/verified_patterns.md` before destructor, thunk, vtable,
  provider, or small-accessor idioms.
- `docs/reconstruction/inlined_helpers.md` before duplicating repeated inlined
  caller bodies.
- `docs/reconstruction/original_classes.md` before class, inheritance, vtable,
  ftable, record, provider, namespace, or subsystem boundary edits.

For table dispatch, model the owner first. Do not use copied ftable/vtable
arrays, production `VTable`/`FTable` structs/globals, or raw slots as authored
source substitutes.

## Literals And Handoff Hygiene

Use decimal literals by default for ordinary counts, sizes, dimensions, enum
values, loop bounds, return codes, allocation sizes, tests, and gameplay/config
constants. Use hex only where hex grouping is evidence: addresses, bitmasks,
byte patterns, PE/RVA/file offsets, serialized wire values, or equivalent
contracts.

Before handoff, move durable facts into source comments or
`docs/reconstruction/`, prune completed group notes, and state the documentation
decision.
