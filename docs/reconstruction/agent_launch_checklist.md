# Agent Launch Checklist

Use this compact checklist before assigning address-led reconstruction work to a
new agent. It does not replace `AGENTS.md`; Binary Ninja and
`.agent/RECOIL_PLAN.md` remain authoritative for function evidence and plan
state.

## Required preflight

Run the process-health check from the workspace root:

```powershell
python tools/recoil_doctor.py --quick --binja
```

Use plain `--quick` only when Binary Ninja is intentionally unavailable or the
task does not depend on current Binary Ninja evidence. For documentation-only or
tooling-inspection work, inspect the relevant files and run targeted checks
instead of selecting a plan address. For active implementation or verification
handoff, also run the address-specific doctor command described in `AGENTS.md`.

For an address-led reconstruction launch packet, use:

```powershell
python tools/recoil_task_packet.py
```

Use `--address 0xNNNNNN` for a specific function or `--no-binja` only when the
task intentionally does not need Binary Ninja. Do not use this packet for
documentation-only or tooling-inspection cleanup, because it selects an address.

## Task selection

Choose the next address from current plan state, not from stale working notes:

```powershell
python tools/recoil_plan_cli.py next
python tools/recoil_status.py 0xNNNNNN
```

If a task expands into a multi-function closure, identify every affected address
before editing source, Binary Ninja state, plan markers, VC verification
manifests, or group notes.

Generate a handoff report before ending a multi-step reconstruction session:

```powershell
python tools/recoil_handoff.py 0xNNNNNN --include-artifacts
```

After finishing a function or class reimplementation step, create a focused
local git commit. A coherent multi-function batch may use one commit. Do not
push. Stage only the agent's own related changes, do not use `git add .`, and
do not stage private inputs, generated artifacts, ignored runtime state, or
unrelated user changes.

Treat `.agent/IMPLEMENTATION_GROUPS.md` as temporary context only. If it
disagrees with `.agent/RECOIL_PLAN.md`, Binary Ninja, or `recoil_status.py`,
trust the current plan/Binary Ninja evidence and refresh or prune the group note.

## Native build shell

Native CMake configure/build/test work needs an x86 MSVC developer environment.
For generated Visual Studio solution work, prefer Visual Studio MCP when
`build/vs-x86/RecoilRebuildNative.slnx` is open or can be opened without
disrupting a user solution. Use it for solution navigation, builds, Error List
triage, output panes, symbol outlines, open-buffer checks, and focused
debugging. See `docs/reconstruction/visual_studio_mcp_workflow.md`.

Before running the native build presets from a new shell, verify that shell:

```powershell
python tools/recoil_env_check.py --native-x86
```

From normal PowerShell, run native commands through the checked-in helper:

```powershell
python tools/recoil_msvc_x86_run.py -- cmake --preset ninja-x86-debug
```

Use this helper for Ninja presets, CMake configure commands, direct `ctest`,
native smoke runs with command-line arguments, and other command-line work that
MCP cannot express. Do not call Visual Studio batch files under `Program Files`
directly.

If this fails because `cl.exe`, `INCLUDE`, `LIB`, the x86 compiler target, the
Windows SDK, `support/sdk`, or `support/Recoil.exe` are unavailable, switch to a
proper x86 developer shell or ask the user for the missing private input. Do not
treat a normal PowerShell failure as source evidence.

## Source placement

Before creating or moving implementation files, check:

```powershell
python tools/recoil_source_file_map.py --check docs/reconstruction/source_file_map.md
```

Use `docs/reconstruction/source_file_map.md` as placement guidance, then confirm
with current Binary Ninja source comments and call-site evidence. Regenerate the
map only when provenance comments in `src/` changed.

Check `docs/reconstruction/verified_patterns.md` before introducing a new
destructor, thunk, vtable stub, provider glue, or small-accessor idiom.

Check `docs/reconstruction/inlined_helpers.md` before duplicating a small
repeated caller body. If Binary Ninja has no standalone function but multiple
callers show the same helper-like code, restore the likely original inline
helper or method and verify it through those callers or the smallest affected
class/source cluster.

Check `docs/reconstruction/original_class_candidates.md` before introducing or
reshaping classes, inheritance, vtables, function tables, records, or
namespace/module boundaries. Treat it as advisory generated evidence and confirm
against current Binary Ninja facts.

## Source literals

Use decimal numeric literals by default in authored C/C++ for ordinary counts,
sizes, dimensions, enum values, loop bounds, return codes, allocation sizes, test
expectations, and gameplay/config constants. Keep hexadecimal for addresses,
bitmasks, byte patterns, PE/RVA/file offsets, serialized wire values, and other
contracts where hexadecimal grouping carries evidence.

## Working state hygiene

Keep `.agent/IMPLEMENTATION_GROUPS.md` for short-lived dependency closures.
Move durable cross-file facts into source comments or narrow notes under
`docs/reconstruction/`, and prune completed group notes before handoff.
Before final handoff or a required git checkpoint, state the documentation
decision: durable facts were captured in source/docs, or no durable new
documentation was needed.
