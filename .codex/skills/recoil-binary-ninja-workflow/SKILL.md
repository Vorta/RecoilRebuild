---
name: recoil-binary-ninja-workflow
description: Inspect the already-open Recoil Binary Ninja database read-only for assembly, xrefs, layouts, globals, imports, function order, and current analysis facts.
---

# Recoil Binary Ninja Read-Only Workflow

Read root `AGENTS.md`, check `get_bridge_info` and `list_tools`, then run:

```powershell
python tools/recoil.py doctor --quick --binja
```

Require the correct database to be already open. Never load, switch, patch, save, rename, retype, or otherwise mutate Binary Ninja in this workflow.

Start from the exact address, owner, global, range, or question. Inspect assembly and xrefs first; use HLIL only as a convenience view. Check callers and callees when interpreting ABI, cleanup, return values, virtual dispatch, callbacks, tail calls, FPU behavior, or global access.

Return concrete facts with addresses and the remaining ambiguity. Treat names and comments as navigation labels unless corroborated. BN evidence can support a source or tracker decision, but does not itself accept ownership, placement, provider status, order, bytes, or tier.
