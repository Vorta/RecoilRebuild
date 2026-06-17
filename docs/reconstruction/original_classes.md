# Original Class And Table Boundary Guide

Compact class/table boundary policy. Binary Ninja and `.agent/RECOIL_PLAN.md`
remain authoritative for identity, layout, xrefs, source readiness, and markers.

Use before introducing, reshaping, or reimplementing a class, vtable, ftable,
callback/data table, provider table, or namespace-style subsystem.

## Core Rule

Tables are ABI evidence, not source design. Source recovery is class-first.

When BN shows vtable/function-table dispatch, recover the owner before the
caller. If constructor/destructor ownership, object offset zero, `this` methods,
inherited cleanup, and dispatch xrefs fit authored C++ class/interface source,
reconstruct that class/interface. Do not copy decompiled ftable/vtable arrays or
use raw slot arrays as authored implementation.

If the class model does not fit, model the proven `struct`/record, provider
boundary, callback/data system, namespace/source-file owner, global-data set, or
subsystem. Use table layout only as ABI evidence.

A proven authored class/interface/method cluster must be recreated before any
`Reimplemented` tier. Flattened functions, copied table arrays, hand-authored
`VTable`/`FTable`, and raw slot/offset scaffolds are not accepted
reimplementations.

Raw `slots[n]` dispatch is allowed only for:

- COM, DirectX, MFC, import, or provider ABI tables
- data-driven callback tables that are not authored C++ classes
- non-production verification/test code

Local virtual dispatch views are not source models unless BN proves that exact
C++ owner. Do not add production `FTable`/`VTable` types, globals, or factories
for authored game dispatch. Provider tables require provider-boundary
classification and real provider headers. Incomplete evidence blocks source;
improve BN instead of adding production scaffolds.

Temporary ABI/source-shape scaffolds are scratch only, outside production source
and durable evidence. Remove them before handoff. Never cite them in plan
markers or use them for `Model: source-faithful`.

## Class-Promotion Gate

Before source work that touches an `FTable`, `VTable`, `Vtbl`, `vptr`, `ftable`,
`slots[n]`, indirect table call, constructor table write, or destructor table
reset, classify ownership from current BN evidence:

- identify the object pointer and whether offset `0` is a table pointer
- inspect constructor table writes
- inspect destructor table restores and base teardown
- inspect table xrefs and required indirect dispatch callsites
- identify slot order, target functions, calling conventions, and cleanup shape
- classify as compiler C++, COM/provider, MFC/runtime metadata, data callbacks,
  or unresolved
- update `.agent/IMPLEMENTATION_GROUPS.md` before multi-function
  class/table/layout/provider/source-file edits

Do not mark `Source dependencies satisfied` until every table dispatch used by
the caller has classified ownership. Do not set a `Reimplemented` tier, `Source
owner ✅`, or `Model: source-faithful` for a flattened proven owner. Restore the
layout, methods, constructor/destructor behavior, and dispatch contract first.

## Boundary Decisions

Use the first classification supported by current evidence.

| Evidence | Source model |
|---|---|
| Constructor installs offset-`0` table; methods use `this`; destructor restores/destroys base state | Authored class/interface; implement layout and method cluster before isolated callers. |
| Offset-`0` table exists, but compiler C++ virtuals do not fit | No production VTable/FTable model; implement proven record, callback/data owner, namespace/source-file owner, global-data set, or subsystem. |
| Table is COM/MFC/DirectX/import/runtime-owned | Provider boundary; do not author fake provider internals. Model only game-owned wrappers/derived classes. |
| Functions operate on explicit records, globals, tags, or data nodes without constructor-owned table identity | Record or namespace subsystem; do not promote by name alone. |
| Repeated caller bodies look like an inlined helper/method and no standalone function exists | Restore likely inline helper/member; verify through callers or the smallest class/source cluster. |
| Table evidence is incomplete or contradictory | Keep bounded record/padding, document blocker, and improve BN before source promotion. |

Names like `Namespace::Function` and function-pointer tables are not enough.
Constructor/destructor ownership, object layout, and dispatch xrefs decide.

## Canonical Family Ledger

Orientation only. Recheck addresses, types, layouts, and slots in BN before
editing source.

### Authored Class Or Interface Families

| Family | Boundary guidance |
|---|---|
| `RecoilApp` | Authored MFC app shell with embedded app states and state queue; MFC base stays provider-owned. |
| `RecoilApp_IState` and derived states | Authored state interface; model lifecycle methods instead of flattening into app switches. |
| `CZRecoilFrame`, `CZGameFrame`, `CAboutDlg` | Authored MFC-derived classes over provider-owned MFC bases, runtime classes, and message maps. |
| Net session and Westwood Online dialogs/event sinks | Authored dialog/sink classes when constructors/vtables prove ownership; WOL/MFC/COM providers stay external. |
| HUD/UI core | Class-first UI hierarchy around `HudUiElement`, widgets, panels, containers, backgrounds, dialogs, text inputs, and variants when constructor/destructor/table evidence supports it. Existing explicit FTables are source-shape debt; add no copied slots or production FTable owners. |
| HudCmd bind-button classes | `HudCmdBindButtonBase` and the command/key/joy/mouse bind-button subclasses are authored HUD UI classes, not copied FTable data. BN `HudCmdBindButton_FTable` is 136 bytes and its final slot at offset `0x84` targets `0x40ba90` `HudCmdBindButtonBase::OnSelectionChangedRefresh`, so that method must remain virtual in source. Scalar-deleting wrappers `0x40b0a0`, `0x40b0c0`, `0x40b0e0`, `0x40b100`, and `0x40b120` are compiler provider-boundaries. Generated VC5 vtables `0x4cda80`, `0x4cd8e8`, `0x4cd860`, `0x4cd7d8`, `0x4cd750`, and `0x4cd6c8` match BN with zero unmasked data-byte mismatches under `vc5_o2_ob1_md_gx_facs`; real destructor bodies `0x40a940`-`0x40ad00` remain below tier S due isolated EH cleanup scheduling drift. |
| HUD app/dialog state wrappers | Authored app-state wrappers around HUD dialogs/controllers; keep lifecycle and dialog ownership explicit. |
| HUD briefing runtime/actions | Mixed class, record, and callback/data system; classify runtime, action queues, and callback data before editing. |
| `zFMV_Action` hierarchy and `zFMV_Script` | Authored action classes/records; preserve ownership, script list order, and destructor behavior. |
| `zInterp_Context` and global context | Authored interpreter context with table evidence and global singleton lifetime. |
| `zVideoFxPass3*`, HUD weather/pass-3 UI elements | Authored overlay classes or records when constructor/table evidence exists; classify first. |

### Record, Data-System, Or Namespace Boundaries

| Boundary | Guidance |
|---|---|
| `RecoilApp_StateQueue` | Non-polymorphic deque-like record embedded in `RecoilApp`; implement as original-era helper record. |
| zClass scene node system | Data-driven node/class system over `zClass_Node`, `classType`, and `classData`; not ordinary C++ inheritance. |
| `Player` | Large record/global subsystem. Do not promote the whole namespace to C++ class without constructor-owned table evidence. Isolated UI/pass-3 evidence remains class-first; no production FTables. |
| `zVideo`, `zVid`, `zRndr` | Static subsystems plus provider/COM objects and records; DirectDraw/D3D interfaces stay provider-owned. |
| `zInput` | Static subsystem/global input state plus bind records unless table ownership proves a class. |
| `zSnd` | Static subsystem/provider mix. DirectSound/A3D tables are provider ABI; samples, banks, handles, and snapshots are authored records/classes only when evidence supports it. |
| `Pickup`, `OptCatalog`, `Mission`, `HudSensorTracker`, `GameNet`, `Net`, `NetUi` | Subsystems/records unless constructor-owned table evidence proves a narrower class. |
| `zMath`, `zGeometry`, `zReader`, `zUtil`, archive/ZBD helpers | Utility/data subsystems; do not add virtual classes by namespace name. |

### Provider Boundaries

Keep provider-owned unless current evidence proves an authored wrapper or
derived class:

- MFC base classes, runtime classes, message maps, and thunks
- CRT/compiler helpers, scalar/vector deleting destructor helpers, and imports
- DirectDraw, Direct3D Immediate Mode, DirectSound, DirectInput, DirectPlay,
  A3D, and COM interface vtables
- Westwood Online imported API/provider interfaces

Provider tables must come from real provider headers or `Provider-boundary`
classification. Do not add fake provider table structs under production `src/`.

## Implementation Pattern

For table-shaped authored evidence, implement the smallest coherent
class/source cluster:

- layout declaration with `RECOIL_STATIC_ASSERT` size/offset checks
- virtual/member declarations only for proven C++ owners
- named record/callback/data fields when class source does not fit
- constructor/destructor table and base behavior
- direct methods and dispatch needed by the frontier
- focused tests or functional verification
- VC byte/provider verification for tier `S` class/table passes

That cluster is the minimum source-shape unit for `Source owner ✅` and
`Model: source-faithful` when evidence proves ownership. Record, namespace,
provider, and data-callback models are valid only when evidence supports them
instead of authored class source.

If clean original-era member/virtual spelling fails binary verification but
behavior is proven, keep the readable source shape and document the mismatch.
Use provider-header dispatch only where evidence requires it. Do not replace a
proven class or unresolved table with local dispatch, `FTable`, or `VTable`
scaffolding to force convenient codegen.

## Updating This Guide

Update only durable boundary facts that save future work:

- name class/table/subsystem and key addresses or symbols
- state evidence source: BN xrefs/types/table writes, source comments, tests, or
  VC/provider verification
- separate recovered facts from open limits
- avoid inventories, long member tables, duplicated plan rows, and progress logs
