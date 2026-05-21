# Agent Launch Checklist

Use this compact checklist before assigning reconstruction work to a new agent.
`AGENTS.md`, Binary Ninja, and `.agent/RECOIL_PLAN.md` remain authoritative.

## Required preflight

Run the process-health check from the workspace root:

```powershell
python tools/recoil_doctor.py --quick --binja
```

Use plain `--quick` only when Binary Ninja is intentionally unavailable for a
non-Binary-Ninja task. For active implementation or verification handoff, also
run the address-specific doctor command described in `AGENTS.md`.

## Task selection

Choose and claim the next address from current plan state, not from stale
working notes:

```powershell
python tools/recoil_claim.py next --owner <name> --claim
python tools/recoil_status.py 0xNNNNNN
```

Use `python tools/recoil_plan_cli.py next` only for read-only navigation when
no agent should claim work yet. If a task expands into a multi-function closure,
claim every affected address before editing source, Binary Ninja state, plan
markers, VC6 manifests, or group notes. Release claims at handoff, or report
the owner, token, and addresses if the next session should continue the work.

Treat `.agent/IMPLEMENTATION_GROUPS.md` as temporary context only. If it
disagrees with `.agent/RECOIL_PLAN.md`, Binary Ninja, or `recoil_status.py`,
trust the current plan/Binary Ninja evidence and refresh or prune the group note.

## Native build shell

Native CMake configure/build/test work needs an x86 MSVC developer environment.
Before running the native build presets from a new shell, verify that shell:

```powershell
python tools/recoil_env_check.py --native-x86
```

From normal PowerShell, run native commands through the checked-in helper:

```powershell
python tools/recoil_msvc_x86_run.py -- cmake --preset ninja-x86-debug
```

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

## Working state hygiene

Keep `.agent/IMPLEMENTATION_GROUPS.md` for short-lived dependency closures.
Move durable cross-file facts into source comments or narrow notes under
`docs/reconstruction/`, and prune completed group notes before handoff.
