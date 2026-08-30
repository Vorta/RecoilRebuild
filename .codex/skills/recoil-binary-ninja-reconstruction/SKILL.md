---
name: recoil-binary-ninja-reconstruction
description: Safely improve names, types, prototypes, variables, comments, analysis, or saved state in the already-open Recoil Binary Ninja database.
---

# Recoil Binary Ninja Reconstruction

Use this only when the task authorizes Binary Ninja mutation. Perform the work directly in the current agent; there is no Binary Ninja role, handoff, or lease workflow.

## Before mutation

1. Read root `AGENTS.md` and identify the exact database and mutation scope.
2. Check the Binary Ninja bridge with `get_bridge_info` and `list_tools`.
3. Run `python tools/recoil.py doctor --quick --binja`.
4. Require the intended database to be already open. Never load, switch, or patch a binary through this workflow.

If the bridge or database is unavailable, stop and ask the user to open it.

## Evidence and authority

Assembly, xrefs, literals, layouts, imports, call sites, and retail bytes are primary. HLIL is supporting evidence and may hide register, FPU, cleanup, or tail-call details. Existing names and comments are provisional.

Correct only facts supported by direct evidence. A BN edit may improve navigation and analysis, but it never decides source ownership, provider classification, translation-unit placement, order or byte acceptance, or an owner tier.

## Mutation loop

- Inspect callees, types, globals, and call sites before changing a caller.
- Apply the narrowest justified name, type, prototype, calling convention, variable, global, comment, return-behavior, or function-property change.
- Reanalyze the affected scope and inspect propagation at representative callers and xrefs.
- Avoid speculative broad renames or types.
- Save the database only after the affected state is coherent.

Record durable reconstruction facts in the source or tracker through their governed commands; do not use BN comments as acceptance evidence.
