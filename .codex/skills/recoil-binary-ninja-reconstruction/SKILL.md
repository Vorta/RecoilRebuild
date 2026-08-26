---
name: recoil-binary-ninja-reconstruction
description: Reconstruct Recoil Binary Ninja state safely. Use when you are parent-assigned to improve names, prototypes, calling conventions, return behavior, types, variables, globals, comments, reanalysis, or saved BN database state in the already-open Recoil.bndb or messages.bndb, and need the original callee-first Binary Ninja reconstruction workflow.
---

# Recoil Binary Ninja Reconstruction

## Global Text Pipeline

When assigned a Recoil.exe frontier, return the complete contiguous function
inventory/boundaries and provider/padding/text-data caveats needed by
`python tools/recoil.py progress next`; do not select a later owner. Names
remain provisional. The six-stage scheduler keeps `authored-function-order`
and the independent `authored-byte-match` lane monotonic. After authored order,
`authored-call-contract` validates every current reviewed authored gating body.
Only when every slice is current does `full-function-order` restart at
`0x401000`, without waiting for authored bytes. `linked-byte-match` starts only
after full order and authored bytes both complete, followed by retail-identical
`final-validation`. Traversal inside each lane remains sequential in retail
order; the callee-first BN workflow below is dependency reconstruction inside
the parent-assigned scope, not a competing scheduler.

## Core Rule

Start from root `AGENTS.md`; this skill adds the write-capable Binary Ninja
reconstruction procedure and does not replace workspace authority, marker
criteria, owner gates, provider boundaries, or the ban on git commands. Binary
Ninja is a maintained analysis artifact. New evidence in the active scope lets
the parent assign a bounded correction without separate user approval or a
tracker mutation. The role's read-only sandbox applies to filesystem writes; it
does not make parent-assigned BN MCP state immutable.

Work only against the already-open `Recoil.bndb` or `messages.bndb` named by the
parent. Use target-qualified Binary Ninja bridge requests. Do not call
`select_binary`, switch targets, `load_binary`, or `patch_bytes`.
Do not decide source owner/block/order, provider acceptance, or tier state.

Before any BN mutation, acquire and report the exclusive writer
reservation/lease id for the named database. Exactly one writer may exist. No
reader or second writer may start or continue—even on a different function—
until this worker completes reanalysis, checks propagation in affected callers/
callees and views, saves the database, returns the packet, and releases the
writer lease. A new reader may then acquire a lease only against that stable
saved view.

Assembly and xrefs are primary evidence. HLIL, MLIL, and decompilation are
derivative views; if they contradict assembly, fix BN names, types, signatures,
variables, properties, or analysis first.

Use this skill for parent-assigned BN state reconstruction. For read-only facts,
use the BN fact role. For production source implementation, use source-worker
workflow after BN evidence is ready.

## Primary Objectives

Improve Binary Ninja so source reconstruction can rely on it:

- Recover meaningful names for functions, methods, globals, types, fields,
  tables, and vtable or callback slots.
- Correct calling conventions, return types, receiver and argument types, stack
  cleanup, return behavior, and propagated caller signatures.
- Replace raw offsets, pointer math, untyped locals, and `void *` artifacts with
  applied structs, classes, arrays, enums, and provider types when evidence
  supports them.
- Type allocation results and constructor/destructor flows so object ownership
  and cleanup are visible.
- Reconstruct vtables, ftables, callback tables, message tables, and touched
  `.rdata` or `.data` objects with source-shaped names and types.
- Leave durable BN comments only when they preserve evidence a future agent
  needs, such as source-file provenance, ABI caveats, table identity, or
  unresolved assembly constraints.

## Binary Ninja Workflow

Before editing, confirm the bridge and target:

```powershell
python tools/recoil.py binja preflight --binary recoil --strict
python tools/recoil.py binja preflight --binary messages --strict
```

Use the command matching the assigned binary. If the bridge is unavailable or
cannot address the requested target, ask the user for a bridge fix instead of
guessing or loading binaries yourself.

For the assigned scope:

1. Inspect existing names, comments, types, properties, callers, callees,
   imports, globals, xrefs, assembly, and decompilation.
2. Work type-first: return type, calling convention, receiver, arguments,
   touched globals/types, then body locals and field uses.
3. Reconstruct direct callees before callers when their ABI, return value,
   provider ownership, or data contract affects caller source shape.
4. Treat recursive or cyclic dependencies as one group; avoid accepting one side
   while the other still hides the real contract.
5. Do not use `void`, raw integers, raw pointers, or generic byte arrays to hide
   unresolved source contracts. Use partial types with explicit padding for
   known layout and unknown fields.
6. After significant edits, run reanalysis, reopen affected callers/callees, and
   save the database when the assigned batch is complete.

## Function Workflow

For each assigned function:

- Inspect direct callees first. Stop caller cleanup if a callee's signature,
  return behavior, provider classification, or table contract is still blocking
  propagation.
- Verify ABI from assembly: prologue, epilogue, `ret N`, stack cleanup,
  calling convention, arguments, return registers, x87 returns, hidden return
  buffers, and caller stack adjustments.
- Identify receiver and ownership evidence: constructor/destructor writes,
  offset-zero tables, inherited cleanup, globals, allocation sites, and table
  xrefs.
- Rename conservatively and uniquely. Use `self` for the receiver variable, not
  `this`.
- Correct the prototype before body cleanup. Then retype locals, stack slots,
  arguments, globals, and allocation results.
- Use stack overlays or equivalent BN typing for `esp_1` and split-stack
  artifacts when that reveals the true local or argument layout.
- Replace decompiler artifacts such as `__offset`, struct-pointer array
  indexing, raw `*(ptr + N)` field access, unresolved vtable offsets,
  `void *` casts, and `data_*` references after assembly confirms the contract.
- Scan decompilation after reanalysis for remaining artifacts, then inspect
  assembly again where decompilation still looks suspicious.
- Add or repair function declaration source-file comments only when current
  evidence supports the source-file identity.
- When source-path literal xrefs prove a physical source-file block, preserve
  neighboring function order and header/provider/COMDAT exception evidence;
  current semantic names do not override the block.
- BN reconstruction may correct assigned source-path literal facts, neighboring
  function order, names, comments, and caveats, but it must not claim a new or
  changed source-owner/source-block/function-order determination from ChatGPT
  Pro. If interpretation is needed, return Pro-ready evidence to the parent or
  source-owner mapper. ChatGPT Pro output is advisory evidence only and does
  not prove source ownership, physical-block tracker changes, owner gates,
  `Model: source-faithful`, or tier `S`.
- Route any interpretation to the complete source-discovery procedure in
  `recoil-source-model-recovery`; BN reconstruction returns facts only.

## Type, Naming, And Data Rules

Check existing types first and extend them rather than creating duplicate local
models. Prefer source-meaningful structs/classes with known fields, explicit
padding, and `unknown_NN` placeholders only where the bytes are truly unknown.

Use canonical SDK/provider types when evidence points to Windows, MFC, DirectX,
COM, CRT, GUID, HRESULT, HWND, HINSTANCE, Direct3D, DirectSound, DirectInput, or
similar provider-owned contracts. Preserve GUID identity by name and bytes.
Avoid casual `uint8_t` and `uint16_t`; prefer semantic char, bool, flags, enum,
word, or provider types when the assembly and xrefs support them.

Naming conventions:

- Functions and methods: `<Class>::<Name>` or `<Subsystem>::<Name>`.
- Globals: `g_<Subsystem>_<Name>` or `g_<Class>_<Name>`.
- Fields: source-meaningful names, or `unknown_NN` only for unresolved storage.
- Do not use `mcp_`, addresses, byte sizes, or temporary investigation notes in
  final BN names once identity is known.

When touching `.rdata` or `.data`, fix the object as well as the reference:

- Name and type complete vtables, ftables, callback tables, message lookup
  tables, string tables, GUIDs, constants, and initialized globals.
- Correct constants misrendered as pointers, pointers misrendered as integers,
  array extents, alignment, and slot order.
- Recheck xrefs after data typing because one fixed table can change many
  callers.

## Return Packet

Return a concise BN reconstruction packet:

- assigned binary and BN scope
- addresses, types, globals, and tables inspected
- exact BN changes made
- assembly/xref/data evidence and decompilation caveats
- reanalysis and save status
- caller/callee propagation checked
- blockers or parent follow-up

Do not report source edits, owner gate/tier changes, `.agent` ledger changes,
workspace issue mutations, support input changes, or git state because this
workflow does not own them.
Subagents never clear or durably depend on `.devspace`; return material
semantic conclusions and direct evidence/transcript paths with their role and
scope to the parent.
