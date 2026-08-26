---
name: recoil-source-model-recovery
description: Recover source-faithful owner models in RecoilRebuild. Use when you need to model authored classes, interfaces, table-shaped dispatch evidence, source-file clusters, initialized globals, record subsystems, original inlined/static helpers, source-shape metadata, route unified tracker owner updates, or replace flattened/raw-offset/non-original-helper/ABI-scaffold code with the original higher-order source construct.
---

# Recoil Source Model Recovery

## Global Text Pipeline

`python tools/recoil.py progress next` selects the authoritative current cursor;
source-model recovery expands it to the complete source-shaped owner. Correct
owner/block/header/include shape needed for that cursor, but do not select a
later owner. The six-stage scheduler keeps `authored-function-order` and the
independent `authored-byte-match` lane monotonic. After authored order finishes,
deterministic `authored-call-contract` slices validate all reviewed authored
gating bodies before `full-function-order` restarts at `0x401000` without
waiting for authored bytes.
`linked-byte-match` starts only after full order and authored bytes both
complete, followed by complete typed `final-validation`. Retail traversal remains
sequential within each lane; that lane-local property does not serialize the
two independent lanes.

## Core Rule

Recover the owning source model before treating isolated functions as source-faithful. Function entries are bookkeeping; Binary Ninja evidence decides whether the real primary source-shaped unit is a class, interface, record subsystem, source-file cluster, authored callback/record/table/global object/static class-member group, provider boundary, subsystem, or strongly connected group. Ordinary initialized-global/literal/constant/storage groupings are auxiliary data packets linked upward to a primary owner unless evidence proves the original source had that exact authored data construct. Source recovery is class-first: when constructor/destructor ownership, object offset zero, `this` methods, inherited cleanup, or dispatch xrefs fit a class/interface, reconstruct that owner before considering a plain struct, record, callback/data, namespace, or subsystem model.

Default to the owner-sized unit. The primary source-shaped owner is the binary-lane work unit; the unified tracker is the acceptance ledger. Do not downscope because the coherent source model looks large, slow, multi-turn, or likely to touch many functions/files. Expected effort is not a blocker. Use an address as the anchor for evidence gathering, then reconstruct the class/source-cluster/record/callback/subsystem owner that the evidence proves unless the work is a true leaf, explicit user-directed repair, narrow blocker removal, or tooling cleanup that cannot safely be grouped. For tier `S`, do not split a non-standalone owner into source-file, data-packet, or single-function slices; remap the owner with evidence or schedule the complete owner.

Route durable owner-scope ledger work through `recoil-progress-tracker`.
Use `python tools/recoil.py progress show <address>` before accepting owner gates/tiers for an address, and record
new higher-order owners before source work proceeds beyond a true leaf.

Global binary-lane selection follows `python tools/recoil.py progress next`. At its
cursor, recover the owner model before writing or verifying an isolated
function, and recover touched-global data before byte verification. Tier `S`
verification is owner-scoped: the
complete linked primary source-shaped owner and its primary-owned, referenced,
touched, linked, and dependency data packets must be ready for the owner byte
gate, unless the user explicitly directs a narrower diagnostic. Data-packet
byte acceptance means the data dependency is byte-ready, not that the
parent/source-owner tier `S` gate is complete.
Use `progress show` to expand the global cursor; focused owner/work views are
deferred context, not permission to shrink the owner or skip the global prefix.
Keep source data symbols/owner data gates distinct from physical storage
contributions and PE output sections. A start address does not prove extent;
unknown extents omit size/end. Final-data/final-repro observations are
linkage evidence, not source-shape or owner acceptance.

Start from root `AGENTS.md`, current Binary Ninja evidence, and focused owner/status tools:

```powershell
python tools/recoil.py progress show 0xNNNNNN
python tools/recoil.py progress owner relationships 0xNNNNNN
```

Use `docs/reconstruction/original_classes.md` for orientation before reshaping
class/table/record/provider boundaries. Before choosing or renaming a
production-source identifier, class, module folder, or filename, consult
`docs/reconstruction/source_naming_conventions.md` and search
`support/engine_terminology/`. Recheck exact current Binary Ninja facts before
editing; naming defaults prove neither ownership nor placement.

Before creating or moving implementation files, run:

```powershell
python tools/recoil.py progress audit --scope blocks --strict
```

Use the joined tracker block/span view plus current BN source comments,
source-path literal xrefs, physical source-file block order, and call-site
evidence for placement. When `zError::ReportOldNoOp` or similar file-path
xrefs show that VC5 emitted whole translation-unit contribution blocks in
source-file order, use that block evidence before trusting stale source paths
or semantic function names. BN function names and comments are provisional
navigation labels; current assembly, xrefs, source-path literals,
provider/import evidence, and function order decide placement. Prove any
header/helper/provider/COMDAT exception with current BN evidence. For known
Recoil.exe physical blocks, generated VC5 COFF order must naturally match the
phase-appropriate retail projection before byte readiness: authored body roles
first, then the complete selected population during full order. A mismatch
inside the current projection is a source-shape/include-shape blocker until
current evidence proves otherwise. Alphabetical folder/source ordering is not a
whole-program assumption: it is binding only where source-path literals,
neighboring function order, or project/link evidence prove it. The no-literal
band `[0x4b2960,0x4c0d20)` is recorded as a special late/out-of-band physical
shelf; its `agent_source_path` rows are provisional compile-order placement
labels, not proof of original translation-unit identity. In that band, recover
the complete source owners and local emitted function order, and correct the
physical-block tracker plus durable audit if VC5/BN evidence proves a different
physical split. Check `docs/reconstruction/source_file_layout_audit.md` for
durable block-order constraints. For source-file-block map work units,
place code in `agent_source_path`; it is derived from `original_source_path`,
then `provisional_original_path`, then compatibility `source_path`. Known/order-relevant `source_shape_inputs` are
mandatory source-shape inputs once known, but they stay attached to the owning
physical `.cpp` block; do not schedule them as separate `.text` blocks or cite
them alone as owner-gate evidence. Address-emitting header contributors may
appear as partial-header physical-block records in `progress show`; reconstruct
their bodies in the row `source_path` header and compile
through `included_in`/`physical_owner_path`. A `partial-header` row is
source-shape placement evidence only: it does not prove full header extent or
accepted owner gates/tiers. Declaration-only/type-only headers remain
`source_shape_inputs` unless an emitted address range is known. Check `docs/reconstruction/verified_patterns.md`
before new destructors, thunks, vtable stubs, provider glue, or small
accessors, and `docs/reconstruction/inlined_helpers.md` before restoring
repeated fully inlined helpers.

The current production `src/` tree is implementation state, not
original-source authority. When a recovered source-file block
or generated VC5 function order contradicts current files, reshape `src` to the
source model that naturally emits the retail BN order: create, rename, split,
merge, or move `.cpp`/`.h` files within the assigned scope. If BN/VC5 evidence
proves a block row incorrect, the worker returns the exact correction proposal;
the parent dry-runs and applies the revision-bound physical-block tracker change
and returns a structured handoff before implementation against the corrected
model. Update `docs/reconstruction/source_file_layout_audit.md` through its
separately assigned write scope. Do not preserve stale `src` structure merely because it
currently compiles, emits plausible bodies, or byte-matches one isolated
function. A mismatch between current implementation and a proven block is often
evidence that the implementation/source-shape is wrong, not that the block
placement is wrong. Durable general lesson: a tiny provider-looking no-op may
still be an authored derived-class override when declaration, generated symbol,
message-map/vtable, and physical-order evidence prove the override; byte shape
alone never decides provider ownership.
Passing smokes, byte checks, or ABI call-shape checks are evidence candidates,
not source-shape proof.

During `authored-function-order`, every selected row must be classified under
the canonical four-way traversal model. `authored_order_role` then distinguishes
actual `authored-body`/`authored-lifecycle-body` gates from explicit
compiler-generated deleting variants, EH helpers, thunks, implicit cleanup-only
leaf variants, and ICF representatives. Only the two authored-body roles must
resolve exactly once in natural retail relative order; compiler-generated
roles stay inventoried and full-order-required but do not gate authored raw or
linked relative order or authored-byte traversal. Absent role metadata defaults
fail-closed from the historical pipeline class. This is the phase's expected
gating identity rule. Unresolved rows and
unclassified selected extras block. Exact selected linked
population, RVAs, and predecessor/successor seams for every class become the
`full-function-order` gate. At either raw-object/TU gate, inventory every
unlisted raw definition, but treat it as mechanically non-blocking because the
linker may discard or fold compiled COMDATs. An object pass proves no source
owner, tier, linked placement, byte identity, provider classification, or final acceptance.
Linker selection also does not excuse dead authored source, wrong declarations/
includes, or an artificial source model; those remain separate source-model
questions. For canonical MFC include
evidence, VC5SP3 does not support `/showIncludes`; use the fail-closed `/E`
preprocessor line-directive trace and require every observed `afx*.h`/`afx*.inl`
path to resolve only under
`D:/Recoil Project/Compiler/VC5SP3/VC/MFC/INCLUDE`.

Source-discovery ChatGPT Pro is an ambiguity escalation, not part of the normal
compile/edit loop. A parent-brokered pass is mandatory only when direct BN,
retail, source, and VC5 evidence still leaves at least two plausible source
owner/block/order models; a materially disputed tracker correction crosses a
translation-unit, owner, provider, or physical-block boundary; raw inline
assembly escalation is being considered under `recoil-tier-verification`; or
the user explicitly requests an external critique.

Registered `vc5-order` execution and first-divergence interpretation, rechecking
an accepted target against current source, editing source to satisfy an already
reviewed target model, deterministic catalog derivation, exact identity/provider
lookup, and tool-contract defects are exempt. Raw BN fact packets and mechanical
lookup of already-accepted durable facts are also exempt when they make no new
owner or placement recommendation. An ordinary mismatch is not a Pro trigger;
escalate only after the direct evidence establishes a genuine model ambiguity.

When Pro is triggered, a worker must never invoke `chatgpt-pro-line` or perform
a live upload. It returns a session-scoped `source-discovery` request id, binary
and owner/address scope, direct attachment roles/paths, prompt inputs, competing
hypotheses, and requested conclusions, then releases its slot. The parent
validates scope, serializes the upload/call, and resumes the same request id with
the transcript and per-file upload results.

The triggered source-discovery prompt must be self-contained: address/range, binary
target, current owner/block/order hypothesis, source-path literal xrefs,
neighboring BN function order, assembly/xrefs/calls/data facts, current
catalog/docs rows, proposed included/excluded functions/data, alternative
hypotheses, contradictions, stale-evidence risk, and requested answer shape.
Ask Pro to challenge the conclusion, identify missing evidence, and return
ranked hypotheses or an ALLOW/BLOCK-style critique. Keep the ChatGPT Pro
session request id and transcript, or a specific exemption reason, in the
worker packet.
ChatGPT Pro output is advisory evidence only; it does not prove source
ownership, physical-block tracker changes, owner gates, `Model: source-faithful`,
or tier `S`. Existing hard byte-match ChatGPT Pro rules remain separate.
`source-discovery` and `hard-byte-raw-assembly` are distinct request kinds and
must never share a prompt, attachment set, or call.
Hard byte/codegen escalation and raw-assembly acceptance are owned by
`recoil-tier-verification`; this skill supplies source-owner/source-block facts
and constraints without copying that policy.

## Boundary Classification

Classify the owner before implementation:

- Authored class/interface: constructor/destructor ownership, object layout, table at object offset zero, methods using `this`, and dispatch xrefs prove a C++ owner. This classification has priority whenever it fits the evidence.
- Authored record/callback/data construct: explicit records, callback tables,
  authored tables/global objects/static class-member groups, tags, globals, or
  nodes when a C++ class/interface model does not fit the evidence and current
  evidence proves this was the original source construct.
- Auxiliary data packet: grouping of globals, literals, constants, or storage
  ranges used as data prerequisite/evidence for a primary owner. Link upward to
  the parent; do not treat as a primary source-owner target or source-faithful
  model unless the exact authored data construct is proven.
- Record or namespace subsystem: explicit records, globals, tags, or data nodes without constructor-owned table identity.
- Data-driven callback table: callbacks are configured data, not an authored C++ class owner.
- Provider boundary: CRT, MFC, DirectX, COM, import, compiler, framework, or runtime-owned ABI behavior.
- Unresolved: evidence is incomplete or contradictory; improve Binary Ninja or keep a bounded temporary layout below source-faithful metadata.

Names such as `Namespace::Function` are not class evidence by themselves. Constructor/destructor ownership, object layout, table writes, and dispatch xrefs decide the boundary.

## Table And Dispatch Gate

Before reimplementing code that touches an `FTable`, `VTable`, `Vtbl`, `vptr`, `ftable`, `slots[n]`, indirect table call, constructor table write, or destructor table reset, inspect:

- object pointer identity and whether offset zero is a table pointer
- constructor writes that install the table
- destructor writes that reset base tables or tear down embedded bases
- table xrefs and indirect dispatch callsites needed by the caller
- slot order, slot targets, signatures, calling conventions, and cleanup shape
- whether the table is compiler C++, custom authored dispatch, COM/provider data, MFC/runtime metadata, data-driven callbacks, or unresolved

Do not mark caller dependencies source-ready until every table-shaped dependency has an owner classification. A copied Binary Ninja ftable/vtable array is not an authored source model, and production `VTable`/`FTable` structs, globals, slot arrays, or factories are forbidden as authored reimplementation endpoints.

Do not create local virtual dispatch views as an intermediate production model.
Types such as `struct ...Dispatch { virtual ... }` or
`struct ...Virtual { virtual ... }` are source-shape scaffolds unless current
Binary Ninja evidence proves that exact authored C++ class/interface owner. A
member-call ABI sequence or slot offset proves call shape, not source shape.
Do not recover authored dispatch as a production `FTable`/`VTable` type or
global. If class/interface evidence does not fit, recover the named
`struct`/record, callback/data record, namespace/source-file owner, provider
boundary, or subsystem that the evidence actually supports. When evidence is
unresolved, improve Binary Ninja or leave the source blocked instead of adding
production scaffolding.

## Source-Faithful Model Gate

Set or claim `Source owner ✅` or `Model: source-faithful` only when the source recreates the proven higher-order owner:

- layout and ABI-sensitive size/offset checks
- constructor/destructor behavior and base/member cleanup order
- typed methods, virtual declarations for proven C++ class/interface owners, or named record/callback/data operations when the class model does not fit
- dispatch contract and slot identities used by callers
- touched globals and initialized data shape where the owner depends on them
- linked auxiliary data packets, with byte evidence treated as dependency
  readiness rather than primary owner tier `S` completion
- source-file/source-cluster placement consistent with current evidence
- physical source-file block order from BN source-path literal xrefs and
  neighboring function order when that evidence exists; semantic names do not
  override a proven block, and header/COMDAT exceptions require recovered
  source shape that naturally emits in the proven physical order

Do not claim `Source owner ✅` or source-faithful model state for flattened helper bodies, copied table arrays, local virtual dispatch views, raw slot scaffolds, raw runtime-state offsets, anonymous byte/pointer layout access, forced `RECOIL_THISCALL`/explicit thiscall scaffolds, call-convention wrappers, reconstruction inline/noinline markers, unallowlisted or undocumented raw assembly, fake provider internals, placeholder global data, or an isolated function body that leaves its proven class/source-cluster/data owner unrecovered.

ChatGPT-Pro-confirmed raw inline assembly or an assembly macro is only approval
for a minimal address-scoped source-faithful inline-asm exception. It does not
prove source ownership, `Model: source-faithful`, owner gates, or tier `S`.
The exception still needs BN/VC5 evidence for the exact register, FPU, and
opcode role, a local docblock, and an address-scoped
`.agent/RAW_ASSEMBLY_ALLOWLIST.txt` row using `source-faithful-inline-asm` or a
narrower existing tag. Naked functions, `_emit`, `.asm` files, whole-function
assembly, raw stack shells, provider shims, and order tricks remain forbidden
except for pre-existing documented CPU-probe style exception classes.

If behavior or bytes match but the owner is flattened away, use the appropriate non-source-faithful metadata state until the owner is restored.

## Address-Backed And Inlined Helper Gate

Do not add unsupported reconstruction helpers to production source. Classify helpers into one of three categories:

For helpers physically emitted inside a different source-file block, prefer
recovered `.h`/`.cpp` ownership, declaration-only/type-only/full-body header
layering, and include timing when BN/source evidence proves a header/COMDAT
exception. Do not add `.inl` files for production reconstruction; existing
`.inl` files are legacy/provisional source-shape debt unless independently
proven original. Do not move a semantic helper into the block's `.cpp` only to
force VC5 placement; do not use pragma/linker/order tricks or artificial order
matching. Source shape and emitted COFF function order must both be justified by
the recovered original-style structure before source-faithful claims.

- Address-backed function: has current BN evidence for a standalone retail function and a canonical source-trace docblock whose artifact edge agrees with the tracker.
- Recovered inlined source helper: has no standalone retail address, but current evidence supports a likely original inline/static/member helper fully inlined into callers.
- Unsupported reconstruction helper: convenience wrapper, factory, adapter, readability helper, or scaffold without accepted original-source/provider evidence.

Recovered inlined helpers are valid source dependencies when the helper docblock or a narrow durable note states that no standalone retail function exists, names observed caller addresses, and explains the evidence basis: repeated instruction/source pattern, accessor idiom, constructor/destructor fragment, table dispatch wrapper, source-cluster ownership, class/member pattern, or matching VC5 caller/cluster bytes. Bare `Observed in caller...` wording is evidence context only; it is not enough by itself.

Treat reconstruction factories and adapters such as `Make...Vtable`, `Make...Vtbl`, `Make...FTable`, runtime table builders, hand-authored `VTable`/`FTable` structs or globals, fake no-op slots, member-address conversion helpers, provider stand-ins, local `...Dispatch`/`...Virtual` call-shape views, and copied table arrays as scaffold debt. Any authored caller that still uses them is not reimplemented and must remain `Reimplemented [X]`/not done until the original source construct, provider boundary, or recovered inlined helper is accepted.

## Inlined Helpers And Clusters

Treat fully inlined original helpers as reconstruction targets when repeated caller bodies, accessor idioms, constructor/destructor fragments, table dispatch wrappers, or class-method patterns support it.

Restore likely inline helpers as `inline` functions, `static inline` helpers, ordinary `static` helpers, member functions, or class-body definitions in the likely original source/header. Do not use reconstruction inline marker macros. Verify through callers or the smallest class/source cluster; do not create fake address entries or fake addresses for helpers with no standalone executable function.

Use an immediately preceding `/** ... */` docblock for restored helpers. State
that the helper has no standalone retail function, name the observed caller
addresses as evidence prose rather than artifact claims, include the evidence
basis, and include one `Purpose:` sentence. Do not invent a source-trace
artifact edge for a helper with no standalone retail artifact.

Source-model workers return exact proposed owner, relationship, physical-block,
gate, and structured-work-item mutations; they do not mutate the tracker. Only
the parent may dry-run and apply those mutations with an expected revision
through `recoil-progress-tracker`, then return the resulting structured handoff
before a source worker edits a multi-function closure, class/interface cluster,
shared type/global, source-file cluster, initialized-data set, subsystem pass,
or recursive group. If the necessary unit is large, the parent records the
active owner/group and the worker continues the owner pass rather than
shrinking it to one function. Keep work items temporary and move durable facts
into source comments or `docs/reconstruction/` when they will save future work.

## Source Shape Defaults

Root `AGENTS.md` owns the full source-shape and owner gate/tier criteria. Local defaults:

- Prefer BN-proven classes/interfaces first; otherwise use typed structs,
  fields, globals, provider types, callback/data records, namespaces, and
  original-era C/C++ spelling that the evidence supports.
- Use normal C++ member syntax and VC5-era `inline`/`static inline` forms when
  evidence supports them; avoid call-convention, inline/noinline, and attribute
  scaffolds unless a provider or ABI boundary requires a specific spelling.
- Replace raw offsets, unsupported helper factories, provider shims,
  source-shape scaffolds, and unallowlisted or undocumented raw assembly before any
  `Reimplemented` tier or `Model: source-faithful` claim. Temporary
  ABI/source-shape probes must stay outside production source and be deleted
  before handoff.
- Use `RecoilPtr32`/`RecoilFn32` only for serialized or
  verification-sensitive recovered layouts proven original, and use
  `RECOIL_STATIC_ASSERT` for recovered sizes, offsets, enum values, and
  ABI-sensitive constants.
- Return exact independently reviewed evidence only after the parent owner is
  source-faithfully reconstructed. When current evidence proves a different
  complete owner model, the parent uses the registered structural replacement
  route:

  ```powershell
  python tools/recoil.py progress owner replace-batch --payload-json '<recoil-owner-replace-batch-v2-object>' --expected-revision <revision> --dry-run --json
  python tools/recoil.py progress audit --scope owners --strict
  ```
- The replacement payload must guard exact current owners and contain the
  complete reviewed replacement records and memberships. The parent reviews
  the dry-run before repeating it with `--apply`. Use `owner downgrade` for
  conservative invalidation without structural change. Unsupported positive
  source-shape metadata, owner gate, or entry-tier changes are workspace issues;
  never teach or invent an add/link/set/batch owner operation.
- Use `recoil-provider-boundary` for plausible CRT/MFC/DirectX/COM/provider
  ownership and `recoil-tier-verification` before owner tier promotion.

## Reporting

Subagents never clear or durably depend on `.devspace`. Return material
semantic conclusions and direct evidence/transcript paths with their role and
scope so the parent can promote them before session-end cleanup.

When reporting source-model recovery, state the owner boundary, evidence for the classification, source model chosen, files or source group affected, table/global/inlined-helper facts, metadata state justified, remaining blockers, and validation or tier-verification follow-up.

