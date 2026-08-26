---
name: recoil-provider-boundary
description: Classify provider-owned boundaries in RecoilRebuild. Use when you must decide whether a function, table, thunk, import, CRT/MFC/DirectX/COM/compiler helper, runtime wrapper, message map, scaffold, or address entry is authored or provider-supplied, including Provider-boundary reclassification or missing provider-boundary address entries.
---

# Recoil Provider Boundary

## Global Text Pipeline

Classify provider/compiler/import/COMDAT contributions occupying the current
`progress next` interval without inventing functions, source owners, or stand-
ins. The six stages are `authored-function-order`, `authored-call-contract`,
`authored-byte-match`, `full-function-order`, `linked-byte-match`, and
`final-validation`; full order waits for every call-contract slice but not for
authored bytes. During `authored-function-order`, return both the canonical
`pipeline_class` (`authored`, `authored-lifecycle`, `non-authored`, or
`unresolved`) and the exact tracker-enumerated `authored_order_role`. Only
`authored-body` and `authored-lifecycle-body` gate authored order and authored
bytes. Explicit compiler-generated roles remain inventoried for full order but
do not gate the authored lanes. If the evidence cannot justify both fields,
leave the row unresolved. Proven non-authored rows are skipped by authored
order but retain their exact-position debt for `full-function-order`;
unresolved rows block. Later contributions are deferred until their cursor
unless required by the current `progress next` cursor.

## Core Rule

Classify the boundary before adding or changing source. Provider behavior is not authored Recoil game source. Do not create fake stand-ins, fake storage, copied provider tables, or local provider internals under `src/`.

Use root `AGENTS.md` as authority. Use Binary Ninja evidence,
`python tools/recoil.py progress show 0xNNNNNN`, and
`python tools/recoil.py progress owner relationships 0xNNNNNN` for current
context.

Keep provider/source ownership separate from physical storage contributions
and PE output-section placement. Unknown provider-data extents omit size/end.
Final-data/final-repro observations are linkage evidence and never prove
provider ownership or acceptance.

## Provider Candidates

Treat these as provider candidates until current evidence proves authored ownership:

- imports and import-address getter stubs
- CRT, MFC42, Win32, DirectX, COM, framework, and vendor runtime behavior
- compiler-generated C++ EH funclets, static initializers, destructor-unwind
  helpers, vector/scalar deleting destructor glue, atexit wrappers, and linker
  padding/fallthrough artifacts; classify authored constructors/destructors/
  static-lifetime bodies and class/object-lifetime-coupled compiler rows as
  `authored-lifecycle` for traversal unless evidence instead proves a wholly
  non-authored provider/runtime contribution
- provider-owned vtables, message maps, runtime classes, COM interfaces, DirectX interfaces, and function tables
- runtime wrappers whose body is only a dispatch/import/provider ABI bridge

Accepted `provider-boundary` owners are dependency-ready for authored callers. They do not carry authored source/data/linkage gates or `Reimplemented` tiers.
Pipeline class and owner class remain separate: `authored-lifecycle`
classification alone neither grants nor denies authored primary ownership or a
tier, and `non-authored` does not by itself accept a provider-boundary owner.

## Authored Candidates

Treat an item as authored only when current Binary Ninja evidence proves game/source ownership, such as:

- constructor/destructor ownership of object storage
- game-owned class/interface layout, or non-class record/callback/data ownership only when class evidence does not fit
- source-file/source-cluster evidence
- authored callbacks or wrappers with real game behavior around a provider call
- caller/callee contracts that require reconstructed game state, globals, or source model

For MFC/DirectX/COM-derived game classes, keep the game-owned derived class authored and the provider-owned base/interface/runtime behavior external.
Do not classify a tiny empty/no-op body as provider-owned solely because its
bytes match a provider base implementation. First check generated symbol,
derived-class declaration, vtable/message-map slot identity, source-file
function order, and VC5 natural-order evidence for an authored override.

Provider/data classification is exempt from the source-discovery ChatGPT Pro
policy only while it does not decide source owner, source block, function
order, or a header/provider/COMDAT placement exception. Before returning a new,
changed, disputed, or acceptance-relevant source-owner/source-block/
function-order determination from provider evidence, return a session-scoped
request id with request kind `source-discovery`, direct attachment roles and
paths, owner/address scope, prompt inputs, and requested conclusions, then
release the worker slot. Workers must never invoke `chatgpt-pro-line` or perform
a live upload. The parent broker checks attachment roles and paths against the
stated scope, uploads them, runs the session-global single-flight call, and
resumes subscribers with transcript and per-file upload results matched to the
request id. `source-discovery` and `hard-byte-raw-assembly` must never share a
prompt, attachment set, or call. Retain the parent-supplied request id and transcript and
treat the output as advisory only. Mechanical lookup of already-accepted
durable facts is exempt, and raw BN fact packets are exempt when they do not
recommend ownership or placement.
Use the complete source-discovery prompt schema in
`recoil-source-model-recovery`; provider classification adds only provider/
non-authored facts and routing.

## Table And Dispatch Gate

For every vtable, ftable, function table, message map, callback table, or indirect dispatch dependency, inspect before treating it as source-ready:

- constructor table writes and destructor resets
- table xrefs and object offset zero ownership
- slot owner, slot signature, and indirect callsites
- whether the table is compiler-generated C++, custom authored engine dispatch, data-driven callbacks, COM/provider data, or unresolved
- imports/provider boundaries reached through the table

Do not use copied Binary Ninja table arrays, production `VTable`/`FTable` structs, globals, slot arrays, or table factories as the implementation substitute for an authored class/interface or non-class callback/data owner.

## Source Modeling Rules

- Use real repo-local provider headers for known SDK, MFC, and DirectX types.
- Treat `support/sdk/DirectX6` as provider evidence and dependency headers. For
  MFC, official project/build paths select only
  `D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE/AFXWIN.H`; both
  `support/sdk/MFC42` and the Visual C++ 5.0 header tree are evidence only. MFC
  source is evidence for provider behavior, not production source to compile
  into Recoil. A matched alternate RTM `MFC42.LIB`/`MFCS42.LIB` pair may be an
  explicit diagnostic only, not provider acceptance.
- Keep target assumptions aligned with retail 1999 Windows x86: VC5SP3, MFC42-era shell behavior, DirectX 6-era APIs, Direct3D Immediate Mode, DirectSound/Input/Play, and Win32/COM provider ABIs.
- For unavailable vendor providers, leave the provider layout as a blocker until real provider headers/types or current provider ABI evidence can classify the boundary. Do not create local provider ABI shims.
- Do not add fake provider storage such as `MfcWndStorage`, fake MFC vtable markers, local DirectX vtable mirrors, fake CRT/MFC/COM storage, or raw provider slot arrays under `src/`.
- Do not introduce provider stand-ins, table-builder helpers, production `VTable`/`FTable` structs or globals, raw slot arrays, or `Make...Vtable`/`Make...FTable` factories as authored source. Authored callers that depend on non-original provider/access scaffolds are not reimplemented and must remain `Reimplemented [X]`/not done until proper source or provider evidence is recovered.
- Do not use authored reconstruction call-convention, thiscall, local virtual dispatch-view scaffolding, or hand-authored VTable/FTable source models as any `Reimplemented` tier endpoint when the real model is a member, virtual method, provider type, callback/data boundary, record, or subsystem.
- Do not add `struct ...Dispatch { virtual ... }`, `struct ...Virtual { virtual ... }`, or temporary ABI/source-shape scaffold views under production `src/` to call provider or authored table slots. Use real provider headers/types, classify the provider boundary, or leave the source blocked.
- Remove existing provider ABI shims when encountered. If removal is blocked by missing provider evidence, record the blocker and downgrade affected authored callers instead of tiering the scaffold.

## Owner Reclassification

Row classification and source-owner reclassification are separate parent-owned
routes. Provider/data classifiers return row recommendations through the
dry-run-first `progress symbol set-pipeline-class-batch` workflow, with both
`pipeline_class` and `authored_order_role`; they never route a row classification
through `progress owner` or apply it themselves: never route a row
classification through `progress owner`. When the evidence also changes the
owner model, return that as a separate complete proposal: exact current and
replacement owners, anchors, primary function/data memberships, dependencies,
retained evidence, and every owner retired by the batch. The parent first
inspects the current model:

```powershell
python tools/recoil.py progress show <owner-id-or-address>
python tools/recoil.py progress show <owner-id-or-address> --json
```

An existing authored/provider structural reclassification uses only the
registered dry-run-first replacement route:

```powershell
python tools/recoil.py progress owner replace-batch --payload-json '<recoil-owner-replace-batch-v2-object>' --expected-revision <revision> --dry-run --json
```

The exact payload must preserve unrelated gates, relationships, data/storage
ownership, and primary memberships. The parent reviews the proposal before
repeating it with `--apply`. This route does not supply positive gate/tier
evidence. Use `owner downgrade` only for conservative invalidation without
structural change; unsupported positive metadata/gate/tier changes are
workspace issues.

For a retail-proven named-function import whose IAT address has no target row,
the parent may dry-run the registered provider-target creation command:

```powershell
python tools/recoil.py progress provider-target register --address 0xNNNNNN --payload-json '{"reviewed":true,"dll":"PROVIDER.dll","import_name":"Function","object_symbol":"__imp_<exact-vc5-symbol>","owner_id":"recoil:owner:provider.example.function_import","owner_name":"Provider Function import","reason":"reviewed retail import and provider declaration"}' --expected-revision <revision> --dry-run --json
```

The command parses immutable `support/Recoil.exe`, requires one exact IAT
address/DLL/import-name identity, derives the four-byte IAT storage and
one-byte callable provider-function views, and atomically proposes current
evidence plus the accepted provider owner, function, data, storage, and exact
relationships. The reviewed `__imp_` COFF symbol must come from the applicable
VC5 provider declaration or equivalent provider evidence, never candidate
output. Review the JSON proposal, then repeat the unchanged command with
`--apply` against the same revision. The registered command is only for named
function IAT slots; ordinal imports and non-import provider boundaries remain
blocked when no other registered typed mutation exists.

After provider-target apply, bind each authored retail call/reference site
separately with the existing relocation-target command:

```powershell
python tools/recoil.py progress relocation-target bind --source-symbol-id <physical-symbol-id> --source-address 0xNNNNNN --payload-json '<reviewed-binding>' --expected-revision <next-revision> --dry-run --json
```

`provider-target register` creates no call-site binding, and
`relocation-target bind` creates no provider inventory. Keep the two review and
CAS steps distinct. If `owner replace-batch` cannot express a reviewed existing
authored/provider reclassification, return the exact proposal as a workspace
issue instead of inventing a command or hand-editing the ledger. Provider
boundaries carry no authored gates or `Reimplemented` tier.

Preserve a real functional target when one already documents the provider contract. Keep `File: external` unless current evidence proves a more specific local provider file.

## Verification And Reporting

Run `python tools/recoil.py guard provider --root src` and `python tools/recoil.py guard source-shape --root src` through the relevant doctor/CTest path when production source changed around provider or table surfaces. Also run `python tools\recoil.py guard original-symbol --root src --max 50` when provider/source work may have introduced or removed helper dependencies.

When reporting, state the address or owner boundary, current unified-tracker
classification and authored-order role, provider/authored evidence, unresolved
ownership questions, any reclassification command used, and whether source
changes avoided fake provider internals.
Subagents never clear or durably depend on `.devspace`; return material
semantic conclusions and direct evidence/transcript paths with their role and
scope.

