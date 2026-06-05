# Original Class And Table Boundary Guide

This guide is the class-boundary policy for reconstruction agents. Binary Ninja
and `.agent/RECOIL_PLAN.md` remain authoritative for current function identity,
layout, xrefs, source readiness, and acceptance markers.

Use this document before introducing, reshaping, or reimplementing anything that
looks like a class, vtable, ftable, callback/data table, provider table, or
namespace-style subsystem.

## Core Rule

Tables are ABI evidence, not the source design. Source recovery is class-first.

When Binary Ninja shows vtable or function-table dispatch, first recover the
owning source boundary. If constructor/destructor ownership, object offset zero,
`this` methods, inherited cleanup, and dispatch xrefs fit an authored C++
class/interface, reconstruct that class/interface first. Do not copy a
decompiled ftable/vtable array as the main implementation of authored code, and
do not use raw slot arrays as the source substitute. Only when the class model
does not fit should agents model the proven `struct`/record, provider boundary,
callback/data system, namespace/source-file owner, global-data set, or subsystem.
Use the table layout only as ABI evidence.

When current evidence proves an authored class/interface or method cluster,
recreating that owner is required before any `Reimplemented` tier can be
accepted. A flattened function body, copied table array, hand-authored
`VTable`/`FTable`, or raw slot/offset scaffold can preserve behavior or byte
evidence, but it is not a reimplementation for code that originally belonged to
the owner.

Raw `slots[n]` dispatch is acceptable only when current evidence proves one of
these cases:

- a COM, DirectX, MFC, import, or provider-owned ABI table
- a data-driven callback table that is not an authored C++ class
- non-authored verification/test code that is not production source

Local virtual dispatch views are not source models. Do not introduce
`struct ...Dispatch { virtual ... }`, `struct ...Virtual { virtual ... }`, or
similar call-shape-only declarations unless current Binary Ninja evidence proves
the original owner was that C++ class/interface. A compiler-generated
member-call sequence can prove ABI shape, but it does not prove the source had a
local dispatch wrapper. If evidence points away from a class, recover the typed
callback/data record or owning subsystem that the evidence actually supports.
Do not create production `FTable`/`VTable` types, globals, or factories for
authored game dispatch. If evidence points to a provider table, classify the
provider boundary. If evidence is incomplete, keep the source blocked and
improve Binary Ninja instead of adding a production scaffold.

Temporary ABI/source-shape scaffolds are allowed only as scratch probes outside
committed production source and durable evidence. They must be removed before
handoff and must never be staged, committed, cited in plan markers, or used to
claim `Model: source-faithful`.

## Class-Promotion Gate

Before reimplementing code that touches an `FTable`, `VTable`, `Vtbl`, `vptr`,
`ftable`, `slots[n]`, indirect table call, constructor table write, or destructor
table reset, classify the table owner from current Binary Ninja evidence.

Required evidence checks:

- identify the object pointer and whether offset `0` is a table pointer
- inspect constructor writes that install the table
- inspect destructor writes that restore a base table or tear down embedded bases
- inspect table xrefs and all indirect dispatch callsites needed by the caller
- identify slot order, target functions, calling conventions, and cleanup shape
- decide whether the table is compiler-generated C++, COM/provider data,
  MFC/runtime metadata, data-driven callbacks, or unresolved
- update `.agent/IMPLEMENTATION_GROUPS.md` before editing a multi-function class,
  table, layout, provider, or source-file cluster

Do not mark `Source dependencies satisfied` for a caller until this ownership
classification is known for every table dispatch used by that caller. For
authored dispatch, the caller remains source-blocked until the owner is modeled
as a class/interface when evidence fits, or as the proven non-class
record/callback/data subsystem when it does not; a copied Binary Ninja
ftable/vtable array is not enough.

Do not set any `Reimplemented` tier, `Source owner ✅`, or
`Model: source-faithful` for an entry or source group that flattens a proven
authored class/table owner into isolated functions. Restore the higher-order
source construct first, including the layout, methods, constructor/destructor
behavior, and dispatch contract needed by the verified frontier.

## Boundary Decisions

Use the first matching classification that is supported by current evidence.

| Evidence | Source model |
|---|---|
| Constructor installs a table at object offset `0`, methods use `this`, destructor restores or destroys base state | Authored class/interface. Implement class/layout/method cluster before isolated callers. |
| Offset `0` table exists, but compiler C++ virtuals do not fit after constructor/destructor/xref review | Do not create a production VTable/FTable model. Implement the proven `struct`/record, callback/data record, namespace/source-file owner, global-data set, or subsystem with named fields and functions. |
| Table is COM/MFC/DirectX/import/runtime-owned | Provider boundary. Do not author fake provider internals; model only the game-owned wrapper or derived class. |
| Functions operate on explicit records, globals, tags, or data nodes without constructor-owned table identity | Record or namespace subsystem. Do not promote to a C++ class by name alone. |
| Repeated caller bodies look like an inlined helper or method and no standalone executable function exists | Restore a likely inline helper/member and verify through callers or the smallest class/source cluster. |
| Table-like evidence is incomplete or contradictory | Keep a bounded record/padding layout, document the blocker, and improve Binary Ninja before source promotion. |

Names such as `Namespace::Function` are not class evidence by themselves.
Function-pointer tables are not enough by themselves. Constructor/destructor
ownership, object layout, and dispatch xrefs decide the boundary.

## Canonical Family Ledger

The entries below are orientation only. Recheck the exact address, type, layout,
and table slots in Binary Ninja before editing source.

### Authored Class Or Interface Families

| Family | Boundary guidance |
|---|---|
| `RecoilApp` | Authored MFC app shell with embedded app-state objects and a state queue. Keep MFC base behavior provider-owned. |
| `RecoilApp_IState` and derived states | Authored state interface. Model state objects and virtual-style lifecycle methods instead of flattening into app switches. |
| `CZRecoilFrame`, `CZGameFrame`, `CAboutDlg` | Authored MFC-derived classes over provider-owned MFC bases, runtime classes, and message maps. |
| Net session and Westwood Online dialogs/event sinks | Authored dialog/sink classes where constructors and vtables prove ownership; imported WOL/MFC/COM providers stay external. |
| HUD/UI core | Class-first authored UI hierarchy around `HudUiElement`, widgets, panels, containers, backgrounds, dialogs, text inputs, and variants when constructor/destructor/table evidence supports it. Existing explicit FTable models are source-shape debt; do not add more copied slot arrays or production FTable owners. |
| HUD app/dialog state wrappers | Authored app-state wrappers around HUD dialogs/controllers. Keep state lifecycle and dialog ownership explicit. |
| HUD briefing runtime/actions | Mixed authored class, record, and callback/data system. Classify runtime, action queues, and callback data before editing; do not model authored dispatch as production FTables. |
| `zFMV_Action` hierarchy and `zFMV_Script` | Authored action classes/records. Preserve action ownership, script list order, and destructor behavior. |
| `zInterp_Context` and global context | Authored interpreter context with table evidence and global singleton lifetime. |
| `zVideoFxPass3*`, HUD weather/pass-3 UI elements | Authored overlay classes or records when constructor/table evidence is present. Classify first; do not reimplement as production FTable objects. |

### Record, Data-System, Or Namespace Boundaries

| Boundary | Guidance |
|---|---|
| `RecoilApp_StateQueue` | Non-polymorphic deque-like record embedded in `RecoilApp`; implement as an original-era helper record. |
| zClass scene node system | Data-driven node/class system over `zClass_Node`, `classType`, and `classData`; not ordinary C++ inheritance. |
| `Player` | Large subsystem over records/globals. Do not promote the whole namespace to a C++ class unless a constructor-owned table proves one. Isolated Player UI/pass-3 evidence must still be class-first; do not model it as production FTables. |
| `zVideo`, `zVid`, `zRndr` | Static subsystems plus provider/COM objects and records. Keep DirectDraw/D3D interfaces provider-owned. |
| `zInput` | Static subsystem/global input state plus bind records unless table ownership proves a class. |
| `zSnd` | Static subsystem/provider mix. DirectSound/A3D tables are provider ABI; samples, banks, handles, and snapshots are authored records/classes only where evidence supports it. |
| `Pickup`, `OptCatalog`, `Mission`, `HudSensorTracker`, `GameNet`, `Net`, `NetUi` | Subsystems and records unless constructor-owned table evidence proves a narrower class. |
| `zMath`, `zGeometry`, `zReader`, `zUtil`, archive/ZBD helpers | Utility/data subsystems. Do not add virtual classes by namespace name. |

### Provider Boundaries

Keep these provider-owned unless current evidence proves an authored game wrapper
or derived class:

- MFC base classes, runtime classes, message maps, and thunks
- CRT/compiler helpers, scalar/vector deleting destructor helpers, and imports
- DirectDraw, Direct3D Immediate Mode, DirectSound, DirectInput, DirectPlay, A3D,
  and COM interface vtables
- Westwood Online imported API/provider interfaces

Provider tables must come from real provider headers or provider-boundary
classification. Do not add fake provider table structs under production `src/`.

## Implementation Pattern

For table-shaped authored evidence, the default implementation unit is the
smallest coherent class/source cluster:

- layout declaration with `RECOIL_STATIC_ASSERT` size/offset checks
- virtual/member declarations only for proven C++ class/interface owners, or
  named record/callback/data fields only when the class model does not fit
- constructor and destructor table/base behavior
- direct methods and dispatch behavior needed by the caller frontier
- focused tests or functional verification for reachable behavior
- VC byte/provider verification when doing a tier `S` class/table pass

The same class/source cluster is the minimum source-shape unit for
`Source owner ✅` and `Model: source-faithful` when current evidence proves the
function belonged to that owner. Record, namespace, provider, or data-driven
callback models remain valid only when the boundary evidence supports them
instead of an authored class.

If clean original-era member or virtual spelling fails binary verification, keep
the readable source shape when behavior is proven and document the mismatch. Use
provider-header dispatch only for the specific provider callsites where evidence
requires it. Do not replace a proven class or unresolved table with a local
virtual dispatch view or production FTable/VTable to make the compiler emit a
convenient call shape.

## Updating This Guide

Update this file only for durable class-boundary facts that will save future
agents time. Keep updates compact:

- name the class/table/subsystem and key original addresses or symbols
- state the evidence source: current Binary Ninja xrefs/types/table writes,
  source comments, tests, or VC/provider verification
- separate recovered boundary facts from open limits
- avoid generated inventories, long member tables, duplicated plan marker rows,
  and broad progress logs
