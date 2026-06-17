# Agent Launch Checklist

Compact launch reminder for address-led reconstruction. It does not replace
`AGENTS.md`; Binary Ninja and `.agent/RECOIL_PLAN.md` remain authoritative for
function evidence and plan state.

`AGENTS.md` and `.agent/AGENTS.md` are local ignored instruction surfaces.
`tools/` and `tests/tools/` are ignored but required local verification
infrastructure. Do not stage ignored local infrastructure, private inputs, or
generated state.

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
The quick doctor includes temporary group WIP hygiene and fails if checks
introduce new tracked dirty paths. Use `--allow-dirty-delta` only when a
tooling-maintenance task intentionally changes tracked files.

For agent-facing command, doc, skill, or role drift:

```powershell
python tools/recoil.py audit agent-surface --strict
```

For generated local artifact inventory and cleanup planning:

```powershell
python tools/recoil.py audit artifacts
```

`audit artifacts` is dry-run by default. Review the selected stale roots before
rerunning with `--delete`.

If a tool, instruction, environment, or workspace setup blocks work or forces a
workaround, use the `AGENTS.md` issue commands and boundary rules. Do not file
normal reconstruction backlog, stale tests/code/manifests/markers, missing or
unregistered smokes, owner/data blockers, tier debt, or missing evidence as
workspace issues.

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
python tools/recoil.py audit sections --strict
```

If active groups exist, choose the first actionable group in
`.agent/IMPLEMENTATION_GROUPS.md`, then follow its anchor or next action with
focused status/frontier checks. Skip a group only when current BN, plan, or
source evidence proves it stale, contradicted, completed, or explicitly lower
priority than another active group.

Use current plan state, not stale notes. Run `plan next --lane binary` only
when no active group exists, active groups have been refreshed/pruned or proven
unactionable, or the user explicitly directs new work:

```powershell
python tools/recoil.py plan next --lane binary
python tools/recoil.py plan batch --lane binary
python tools/recoil.py section show ui.zhud
python tools/recoil.py plan group app.recoil_app --lane binary
python tools/recoil.py status 0xNNNNNN
```

`plan next --lane binary` prints `primary`, `secondary`, and `tertiary`
ranked owner/work scopes. `plan batch --lane binary` prints section-isolated
worker candidates for parallel scheduling. The parent agent should use those
sections to schedule non-overlapping subagent handoffs after checking active WIP
and allowed/forbidden paths.
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
workspace/BN/owner/provider/scaffold roles return evidence packets only; source
workers edit one assigned slice; verifier agents run targeted checks after the
parent fixes the validation scope.

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

Before launching live markdown handoff blocks, run:

```powershell
python tools/recoil.py audit handoff --path .agent/IMPLEMENTATION_GROUPS.md --strict
```

Workers may inspect BN and edit only assigned source/test files. They must not
change BN names/types/comments, save BN, update plan markers, file workspace
issues, commit, or select follow-up work. They return changed paths, evidence,
checks, blockers, and non-authoritative marker recommendations. The parent
reviews diffs, reruns checks, performs BN/plan/issue/checkpoint work, and owns
final claims.

Parent source edits are limited to small integration/conflict fixes after worker
return, or cases where delegation is impossible; record the exception before
editing.

Quiet mode: do not send routine progress reports. Message the user only for
required user input, true blockers, worker handoff decisions, validation
failures, or final results. Any unavoidable interim update must be one short
sentence with no evidence dump or command output unless requested.

Normal binary-lane selection is owner-first after reconstruction/dependency
readiness: unresolved `Source owner` markers come before isolated
implementation or tier `C` behavior work, then `Data reimplemented`. Pure tier
`S` verification is globally deferred until every authored `Source owner`
marker is `✅` and every authored `Data reimplemented` marker is `✅` or `❎`,
unless the user explicitly directs tier `S` work.

For initialized table/data blockers, `python tools/recoil.py verify vc5
0xNNNNNN` may resolve a `data_symbols` manifest entry and emit
relocation-masked COFF data-byte evidence plus a relocation identity report.
Use this only after the table/data owner is classified; it does not replace
source-owner or source-shape review.

Treat the address as an evidence anchor, not necessarily the implementation
unit. Expand to the proven owner boundary: class/interface, table-shaped
dispatch owner, provider boundary, source-file cluster, or dependency group.
Identify every affected address before editing source, BN state, plan markers,
VC manifests, or group notes.

If BN proves an authored class/interface/method cluster, restore that owner
before any `Reimplemented` tier or `Model: source-faithful`. Flattened functions
and production `VTable`/`FTable` scaffolds are not accepted reimplementations.

Before ending a multi-step reconstruction session:

```powershell
python tools/recoil.py handoff 0xNNNNNN --include-artifacts
```

## Git And Groups

After verified qualifying source progress, create a local commit before final
handoff unless a blocking rule applies. Commit only completed batches with
tracked production source under `src/`. Do not commit plan-only, docs-only,
tools-only, manifest-only, or test-only changes. If a qualifying source commit
is being made and `.agent/RECOIL_PLAN.md` is dirty, stage it with that
checkpoint. Do not push. Do not use `git add .`. Never `git add -f` ignored
paths. Stage only the agent's related changes; do not stage ignored, private,
generated, runtime, or unrelated files. Final reports include the commit hash
and subject, or the exact no-commit blocker.

`.agent/IMPLEMENTATION_GROUPS.md` is temporary. If stale or contradicted by BN,
`.agent/RECOIL_PLAN.md`, or `recoil.py status`, refresh or prune it. Stage it
only with a qualifying source checkpoint. Active groups are the default
no-address startup queue; do not start unrelated new work while actionable WIP
remains.

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

For docblock checks, audit changed source files before marker work:

```powershell
python tools/recoil.py audit docblocks --changed --summary --max 50
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

Before handoff or a qualifying checkpoint, move durable facts into source
comments or `docs/reconstruction/`, prune completed group notes, and state the
documentation decision.
