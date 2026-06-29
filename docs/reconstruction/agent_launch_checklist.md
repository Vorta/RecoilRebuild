# Agent Launch Checklist

Compact launch reminder for Recoil reconstruction agents. It does not replace
root `AGENTS.md`; root instructions remain the full workflow authority for
evidence gates, owner gate/tier criteria, subagent boundaries, issue-ledger scope, and
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
They must not introduce new owner gate/tier criteria, must not weaken evidence or
provider boundaries, must not edit production source, must not change Binary
Ninja state, and must not mutate `.agent` ledgers unless the existing workflow
separately authorizes that exact mutation.

Workspace issues are only for agent tooling, workspace setup, instruction,
environment, or validation-path defects. Normal reconstruction backlog remains
in the owner-led reconstruction and verification workflow. When a workspace
issue or local tool upgrade is assigned, inspect the focused surface, then spawn
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

Choose the first actionable source-file block-map work unit, source-owner work
unit, final-lane work unit, or active
group in `.agent/IMPLEMENTATION_GROUPS.md` or
`.agent/IMPLEMENTATION_GROUPS_MESSAGES.md`. Skip a unit only when current BN,
owner-ledger, or source evidence proves it stale, contradicted, completed, or
explicitly lower priority than another active unit.

The primary source-shaped owner is the default binary-lane work unit; address
rows are evidence anchors. Primary owners are original-source-shaped units such
as classes/interfaces, source-file clusters, subsystems, authored
callback/record/table/global object/static class-member groups, provider
boundaries, or true standalone leaves. Ordinary global/literal/constant/storage
groupings are auxiliary data packets: link them upward to a primary owner and
use them as data prerequisites/evidence packets unless current evidence proves
the original source had that exact authored data construct. Use address-led
inspection only after active groups/source-owner units have been refreshed or
proven unactionable, or when the user explicitly directs address-led work:

```powershell
python tools/recoil.py owner next --lane binary
python tools/recoil.py owner next --binary messages --lane binary
python tools/recoil.py owner next --lane binary --json
python tools/recoil.py owner next --binary messages --lane binary --json
python tools/recoil.py owner show 0xNNNNNN
python tools/recoil.py status 0xNNNNNN
```

`owner next --lane binary` can print non-ledger work units. For Recoil.exe,
`work_unit=source-file-block-map` is the first priority while physical
translation-unit boundaries and header/COMDAT/provider exceptions are still
being recovered. It uses `docs/reconstruction/source_file_layout_audit.md` and
`tools/_recoil/config/source_file_blocks.json` to continue top-down from the
proven `ai_net.cpp` block before final-data layout or owner-local byte work.

`owner next --lane binary` can also print global final-lane work units:
`work_unit=final-repro` for final executable reproducibility and
`work_unit=final-data-layout` for linked `.data` layout drift. Neither is a
SOURCE_OWNERS record. They block final executable acceptance and directly
affected owner/data byte gates only, not unrelated source-owner tier `S` work.
Human output labels them with `work_unit_scope=global-final-lane` to distinguish
them from `work_unit_scope=owner-local-tier-s-candidate` source-owner entries.
Queue output should surface auxiliary data packets beneath their primary
source-shaped owner. Orphan data packets are parent-reconciliation blockers,
not standalone primary source-owner targets.

`owner next --lane binary` and
`owner next --binary messages --lane binary` print target-qualified primary,
secondary, and tertiary scopes. Use `owner next --binary messages --lane binary --json`
plus `audit sections --strict` when choosing companion-DLL worker candidates.

Do not split a non-standalone source-owner work unit into source-file slices.
Tier `S` is owner-scoped: schedule or verify the complete linked primary
source-shaped owner only when that owner plus its primary-owned, referenced,
touched, linked, and dependency data packets are ready for the byte gate.
Data-packet byte acceptance means the data dependency is byte-ready; it does
not by itself complete the parent/source-owner tier `S` gate. Unrelated
owner/data debt, including orphan data such as `0x4e5954`, does not block
unrelated owner-scoped tier `S`. This is a per-owner pipeline, not a
whole-program sequence of all functional work first, all owner linkage second,
and all byte work last. A ready owner-local tier `S` candidate may proceed
while unrelated owners or global final-lane work units remain open.
The source-file block map is different: when current BN source-path literal
evidence proves incomplete physical block boundaries in the same executable
range, recover those boundaries and exceptions before using semantic owner names
or final-data layout as the scheduling driver.
Schedule Recoil.exe and `messages.dll` workers together only when BN database
targets, sections, source paths, ledgers, and generated outputs do not overlap.
If evidence shows an owner or group belongs in another scheduling section,
inspect with `section show`, dry-run `section move`, validate, then apply
through the section command. `messages.dll` `Reconstructed` blockers should
normally be assigned to `recoil_bn_reconstructor` with target binary `messages`.

Known address launch packet:

```powershell
python tools/recoil.py packet --address 0xNNNNNN
```

Use `--no-binja` only when intentional. Do not use address-led packets for
docs/tooling cleanup.

## Parent Orchestration Loop

The parent schedules, integrates, validates, owns BN scope assignment, owner
gates/tiers, workspace issues, and final claims. Parent must not perform production
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
Subagents must not update owner gates/tiers, file workspace issues, run git
commands, or select follow-up work.

Quiet mode: do not send routine progress reports. Message the user only for
required input, true blockers, worker handoff decisions, validation failures, or
final results. Any unavoidable interim update must be one short sentence with
no evidence dump or command output unless requested.

## Owner Gates And Tiers

Treat an address as an evidence anchor. Expand to the proven owner boundary:
class/interface, table-shaped dispatch owner, provider boundary, source-file
cluster, initialized-global data set, subsystem, dependency group, or true
standalone leaf.
Prefer the primary source-shaped owner when source-owner acceptance or tier `S`
is in scope. Treat initialized-global/literal groupings as auxiliary data
packets unless evidence proves a real original authored data construct such as
a callback table, authored record/table/global object, or static class-member
group.

Before accepting positive owner/data/tier-B+ gates or
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

Use `source_file_map.md` plus current BN source comments, source-path literal
xrefs, neighboring function order, and call-site evidence. When
`zError::ReportOldNoOp` file-path literals or similar source-path xrefs show
that VC5 emitted whole translation-unit contribution blocks in source-file
order, use that physical block evidence before trusting stale source paths or
semantic function names. A function can still be a header/helper/provider
exception inside the physical block, but the exception must be proven with
current BN evidence. The byte-matching goal includes matching generated VC5 COFF
function order to the retail Binary Ninja address order inside the source-file
block, so source workers must shape declarations, static/helper placement, and
header or `.inl` inclusion order accordingly. Do not relocate semantic helpers
into the wrong `.cpp` to force placement; model proven header/COMDAT helpers as
original-style includes. Treat generated map entries as stale when current BN
file-literal evidence contradicts the recorded source path.
Regenerate only when current source movement, provenance docblocks, or legacy
source comments explain the drift:

```powershell
python tools/recoil.py audit source-map --update --output docs/reconstruction/source_file_map.md
```

For touched source files before owner gate/tier work:

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
