# Agent Launch Checklist

Compact launch reminder for Recoil reconstruction agents. It does not replace
root `AGENTS.md`; root instructions remain the full workflow authority for
evidence gates, marker criteria, subagent boundaries, issue-ledger scope, and
the ban on git commands.

`.agent/AGENTS.md` is a compatibility pointer only. Durable source-owner scopes
live in `.agent/SOURCE_OWNERS.json` and must be inspected or updated through
`python tools/recoil.py owner ...`, not by hand.

## Preflight

For normal reconstruction with Binary Ninja evidence expected:

```powershell
python tools/recoil.py doctor --quick --binja
```

Use plain `--quick` only when Binary Ninja is intentionally irrelevant. For
documentation, tooling, skill, role, or instruction cleanup, inspect the target
files and run targeted checks instead of selecting an address.

Agent-facing command, doc, skill, or role drift:

```powershell
python tools/recoil.py audit agent-surface --strict
```

Agent-surface evolution is a governed tool-maintenance change. Allowed
agent-surface evolution write paths are `.codex/skills/recoil-*`,
`.codex/agents/*.toml`, `tools/recoil.py`, `tools/_recoil`, `tests/tools`, and
focused docs. Nontrivial updates need a direct user request or reproducible
process/tooling need, and the parent uses `recoil_tool_maintainer` by default.
They must not introduce new marker criteria, must not weaken evidence or
provider boundaries, must not edit production source, must not change Binary
Ninja state, and must not mutate `.agent` ledgers unless the existing workflow
separately authorizes that exact mutation.

Workspace issues are only for agent tooling, workspace setup, instruction,
environment, or validation-path defects. Normal reconstruction backlog remains
in the owner/plan/verification workflow. When a workspace issue or local tool
upgrade is assigned, inspect the focused surface, then spawn
`recoil_tool_maintainer` by default for the repair.

## Task Selection

Resume active WIP across both binary targets before starting new work:

```powershell
python tools/recoil.py audit groups --summary --wip-limit 4
python tools/recoil.py audit groups --binary messages --summary --wip-limit 4
python tools/recoil.py owner audit --strict
python tools/recoil.py owner next --lane binary
python tools/recoil.py owner next --binary messages --lane binary
python tools/recoil.py audit sections --strict
python tools/recoil.py audit sections --pressure
```

Choose the first actionable global work unit, source-owner work unit, or active
group in `.agent/IMPLEMENTATION_GROUPS.md` or
`.agent/IMPLEMENTATION_GROUPS_MESSAGES.md`. Skip a unit only when current BN,
plan, or source evidence proves it stale, contradicted, completed, or
explicitly lower priority than another active unit.

The source owner is the default binary-lane work unit; address rows are
evidence anchors. Use address-led plan fallback only after active
groups/source-owner units have been refreshed or proven unactionable, or when
the user explicitly directs address-led work:

```powershell
python tools/recoil.py plan next --lane binary
python tools/recoil.py plan next --binary messages --lane binary
python tools/recoil.py plan batch --lane binary
python tools/recoil.py plan batch --lane binary --spawnable-only
python tools/recoil.py plan batch --binary messages --lane binary --spawnable-only
python tools/recoil.py plan batch --lane binary --json
python tools/recoil.py plan batch --lane binary --handoff-template
python tools/recoil.py owner show 0xNNNNNN
python tools/recoil.py status 0xNNNNNN
```

`owner next --lane binary` can print global non-ledger work units:
`work_unit=final-repro` for final executable reproducibility and
`work_unit=final-data-layout` for linked `.data` layout drift. Neither is a
SOURCE_OWNERS record.

`plan next --lane binary` and
`plan next --binary messages --lane binary` print target-qualified primary,
secondary, and tertiary scopes. `plan batch --binary messages --lane binary
--spawnable-only` selects companion-DLL worker candidates.

Do not split a non-standalone source-owner work unit into source-file slices.
Schedule Recoil.exe and `messages.dll` workers together only when BN database
targets, sections, source paths, ledgers, and generated outputs do not overlap.
If evidence shows a plan group belongs in another scheduling section, inspect
with `section show`, dry-run `section move`, validate, then apply through the
section command. `messages.dll` `Reconstructed` blockers should normally be
assigned to `recoil_bn_reconstructor` with target binary `messages`.

Known address launch packet:

```powershell
python tools/recoil.py packet --address 0xNNNNNN
```

Use `--no-binja` only when intentional. Do not use address-led packets for
docs/tooling cleanup.

## Parent Orchestration Loop

The parent schedules, integrates, validates, owns BN scope assignment, plan
markers, workspace issues, and final claims. Parent must not perform production
source implementation by default; after focused context gathering, spawn
`recoil_source_worker` agents for non-overlapping source/test edits. Parent
source edits are limited to small integration/conflict fixes after worker
return, or cases where delegation is impossible; record the exception before
editing.

Use roles as a pipeline:

- Evidence roles return narrow packets only.
- `recoil_bn_reconstructor` performs one assigned BN-state slice.
- `recoil_source_worker` edits one assigned source/test slice.
- `recoil_source_owner_scrutinizer` challenges proposed positive owner/data,
  tier-B+, or `Model: source-faithful` acceptance.
- `recoil_tool_maintainer` fixes one assigned workspace/tool/docs/skill/role
  issue.
- `recoil_verifier` runs targeted checks after the parent fixes the scope.

Minimum handoff fields:

- Source worker: complete source-owner work unit, section, anchors or group,
  allowed and forbidden paths, evidence inputs, expected source model, exact
  validation commands, and return packet fields.
- Verifier: validation scope, section, anchors or group, exact commands,
  evidence inputs, forbidden paths, and return packet fields.
- Minimum tool-maintainer handoff fields: workspace issue id or requested tool
  upgrade, area/current
  behavior, expected behavior, allowed and forbidden paths, exact validation
  commands, and return packet fields.
- BN reconstructor: already-open binary target, non-overlapping BN scope,
  allowed BN changes, forbidden BN actions, evidence inputs, reanalysis/save
  expectations, and return packet fields.

Before launching live markdown handoff blocks:

```powershell
python tools/recoil.py audit handoff --path .agent/IMPLEMENTATION_GROUPS.md --strict
```

Tool maintainers may edit only assigned tool/docs/skill/role/test files.
Subagents must not update plan markers, file workspace issues, run git
commands, or select follow-up work.

Quiet mode: do not send routine progress reports. Message the user only for
required input, true blockers, worker handoff decisions, validation failures, or
final results. Any unavoidable interim update must be one short sentence with
no evidence dump or command output unless requested.

## Owner And Marker Gates

Treat an address as an evidence anchor. Expand to the proven owner boundary:
class/interface, table-shaped dispatch owner, provider boundary, source-file
cluster, initialized-global data set, subsystem, dependency group, or true
standalone leaf.

Before accepting positive owner/data/tier-B+ markers or
`Model: source-faithful`, inspect the owner ledger and run scrutiny:

```powershell
python tools/recoil.py owner relationships <owner-id-or-address> --json
python tools/recoil.py owner audit-acceptance <owner-id-or-address> --strict --json
python tools/recoil.py owner audit-membership <owner-id-or-address> --strict
```

Use `owner audit-membership` when current BN evidence is relevant. A mechanical
pass is not enough: disprove shortcut ownership such as address slices,
anchor-only links, arbitrary source-file slices, folded/shared bodies, split
lifecycle pairs, and test/ABI/byte-only evidence.

For detailed rules, use:

- `owner_led_workflow.md` for source-owner ledger mechanics.
- `data_owner_audit.md` for full data-owner acceptance.
- `final_executable_repro.md` for final executable and final linked-data gates.
- `original_classes.md` for class/table/source-shape boundaries.

## Source Placement

Before creating or moving implementation files:

```powershell
python tools/recoil.py audit source-map --check docs/reconstruction/source_file_map.md
```

Use `source_file_map.md` plus current BN source comments and call-site evidence.
Regenerate only when current source movement, provenance docblocks, or legacy
source comments explain the drift:

```powershell
python tools/recoil.py audit source-map --update --output docs/reconstruction/source_file_map.md
```

For touched source files before marker work:

```powershell
python tools/recoil.py audit docblocks --path src/path/to/touched_file.cpp --summary --max 50
```

Broad `audit docblocks --path src` output is legacy backlog unless the task
assigns a source-docblock cleanup.

## Native Build Shell

Native builds/tests need an x86 MSVC environment. From normal PowerShell, use:

```powershell
powershell -ExecutionPolicy Bypass -File cmake\recoil_native_x86_build.ps1 -Preset ninja-x86-debug
python tools/recoil.py build msvc-x86 -- ctest --preset ninja-x86-debug
```

The wrapper loads `vcvarsall x86` and runs
`python tools/recoil.py env --native-x86`. A missing `kernel32.lib` is an
environment problem, not source evidence. Do not call Visual Studio batch files
under `Program Files` directly.

## Handoff Hygiene

Before ending multi-step reconstruction work:

```powershell
python tools/recoil.py handoff 0xNNNNNN --include-artifacts
```

Move durable facts into source comments or `docs/reconstruction/`, prune
completed temporary group notes only after durable facts are moved, and state
the documentation decision.
