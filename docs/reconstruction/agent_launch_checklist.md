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
python tools/recoil_doctor.py --quick --binja
```

Use plain `--quick` only when Binary Ninja is intentionally unavailable or
irrelevant. For documentation or tooling cleanup, inspect target files and run
targeted checks instead of selecting an address. For active implementation or
verification handoff, also run the address-specific doctor command from
`AGENTS.md`.

Address-led launch packet for a known anchor:

```powershell
python tools/recoil_task_packet.py --address 0xNNNNNN
```

Use `--no-binja` only when intentional. Do not use the task packet for
docs/tooling cleanup or other non-address work; inspect the relevant files and
run targeted checks instead.

## Task Selection

Use current plan state, not stale notes:

```powershell
python tools/recoil_plan_cli.py next --lane binary
python tools/recoil_plan_cli.py group app.recoil_app --lane binary
python tools/recoil_status.py 0xNNNNNN
```

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
python tools/recoil_handoff.py 0xNNNNNN --include-artifacts
```

## Git And Groups

Commit only completed batches with tracked production source under `src/`. Do
not commit plan-only, docs-only, tools-only, manifest-only, or test-only
changes. Do not push. Do not use `git add .`. Never `git add -f` ignored paths.
Stage only the agent's related changes; do not stage ignored, private,
generated, runtime, or unrelated files.

`.agent/IMPLEMENTATION_GROUPS.md` is temporary. If stale or contradicted by BN,
`.agent/RECOIL_PLAN.md`, or `recoil_status.py`, refresh or prune it. Stage it
only with a qualifying source checkpoint.

## Native Build Shell

Native builds/tests need an x86 MSVC environment. Prefer Visual Studio MCP for
already-open or safely generated `build/vs-x86/RecoilRebuildNative.slnx` work.

From normal PowerShell, use the wrapper:

```powershell
powershell -ExecutionPolicy Bypass -File cmake\recoil_native_x86_build.ps1 -Preset ninja-x86-debug
```

The wrapper loads `vcvarsall x86` and runs
`python tools/recoil_env_check.py --native-x86`. A missing `kernel32.lib` is an
environment problem, not source evidence.

Use `tools/recoil_msvc_x86_run.py -- ...` for `ctest`, native smokes, and other
x86 MSVC commands. Do not call Visual Studio batch files under `Program Files`
directly. If `cl.exe`, `INCLUDE`, `LIB`, the x86 target, Windows SDK,
`support/sdk`, or `support/Recoil.exe` is missing, switch environments or ask
for the missing private input.

## Source Placement

Before creating or moving implementation files:

```powershell
python tools/recoil_source_file_map.py --check docs/reconstruction/source_file_map.md
```

Use `source_file_map.md` plus current BN source comments and call-site evidence.
Regenerate the map only when provenance docblocks or legacy source comments
changed.

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
