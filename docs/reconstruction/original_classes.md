# Original Class And Table Boundary Guide

This guide is the class-boundary policy for reconstruction agents. Binary Ninja
and `.agent/RECOIL_PLAN.md` remain authoritative for current function identity,
layout, xrefs, source readiness, and acceptance markers.

Use this document before introducing, reshaping, or reimplementing anything that
looks like a class, vtable, ftable, callback table, provider table, or
namespace-style subsystem.

## Core Rule

Tables are ABI evidence, not the source design.

When Binary Ninja shows vtable or function-table dispatch, first recover the
owning source boundary. Do not copy a decompiled ftable/vtable array as the main
implementation of an authored class or custom table object. Model the class,
interface, typed custom function-table object, provider boundary, callback table,
or data system that owns the table, then use the table layout only as ABI
evidence.

Raw `slots[n]` dispatch is acceptable only when current evidence proves one of
these cases:

- a custom engine table where original-era C++ virtual spelling is not supported
  by the evidence
- a COM, DirectX, MFC, import, or provider-owned ABI table
- a data-driven callback table that is not an authored C++ class
- a documented temporary blocker after clean member/virtual spelling was tried
  and failed source or byte verification

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
- decide whether the table is compiler-generated C++, custom authored engine
  dispatch, COM/provider data, MFC/runtime metadata, or data-driven callbacks
- update `.agent/IMPLEMENTATION_GROUPS.md` before editing a multi-function class,
  table, layout, provider, or source-file cluster

Do not mark `Source dependencies satisfied` for a caller until this ownership
classification is known for every table dispatch used by that caller. For an
authored table owner, the caller remains source-blocked until the owner is
modeled as a class/interface or typed custom table object; a copied Binary Ninja
ftable/vtable array is not enough.

## Boundary Decisions

Use the first matching classification that is supported by current evidence.

| Evidence | Source model |
|---|---|
| Constructor installs a table at object offset `0`, methods use `this`, destructor restores or destroys base state | Authored class/interface. Implement class/layout/method cluster before isolated callers. |
| Offset `0` table exists, but dispatch is a hand-built engine table rather than compiler C++ virtuals | Authored custom function-table object. Implement the typed owner and named methods/table fields; use raw slots only where necessary. |
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
| HUD/UI core | Custom authored function-table hierarchy rooted around `HudUiElement`, widgets, panels, containers, backgrounds, dialogs, text inputs, and variants. Reconstruct the object hierarchy and typed tables before adding more copied slot arrays. |
| HUD app/dialog state wrappers | Authored app-state wrappers around HUD dialogs/controllers. Keep state lifecycle and dialog ownership explicit. |
| HUD briefing runtime/actions | Mixed authored class/custom-table system. Classify runtime, action queues, and callback tables before editing. |
| `zFMV_Action` hierarchy and `zFMV_Script` | Authored action classes/records. Preserve action ownership, script list order, and destructor behavior. |
| `zInterp_Context` and global context | Authored interpreter context with table evidence and global singleton lifetime. |
| `zVideoFxPass3*`, HUD weather/pass-3 UI elements | Authored overlay/function-table objects when constructor/table evidence is present. |

### Record, Data-System, Or Namespace Boundaries

| Boundary | Guidance |
|---|---|
| `RecoilApp_StateQueue` | Non-polymorphic deque-like record embedded in `RecoilApp`; implement as an original-era helper record. |
| zClass scene node system | Data-driven node/class system over `zClass_Node`, `classType`, and `classData`; not ordinary C++ inheritance. |
| `Player` | Large subsystem over records/globals. Do not promote the whole namespace to a C++ class unless a constructor-owned table proves one. Isolated Player UI/pass-3 table objects may still be authored table objects. |
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

Provider tables can be represented with typed ABI structs where needed for
callsites and tests, but they are not authored game classes.

## Implementation Pattern

For a table-shaped authored object, the default implementation unit is the
smallest coherent class/source cluster:

- layout declaration with `RECOIL_STATIC_ASSERT` size/offset checks
- typed table or virtual/member declarations that match the recovered call shape
- constructor and destructor table/base behavior
- direct methods and table-dispatched methods needed by the caller frontier
- focused tests or functional verification for reachable behavior
- VC byte/provider verification when doing a binary-safe class/table pass

If clean original-era member or virtual spelling fails binary verification, keep
the readable source shape when behavior is proven and document the mismatch. Use
raw slot dispatch only for the specific callsites where evidence requires it.

## Updating This Guide

Update this file only for durable class-boundary facts that will save future
agents time. Keep updates compact:

- name the class/table/subsystem and key original addresses or symbols
- state the evidence source: current Binary Ninja xrefs/types/table writes,
  source comments, tests, or VC/provider verification
- separate recovered boundary facts from open limits
- avoid generated inventories, long member tables, duplicated plan marker rows,
  and broad progress logs
