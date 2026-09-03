# Source Naming And GameZ Layout Conventions

Canonical guidance for choosing source identifiers, class prefixes, GameZ
module paths, filenames, and unresolved GameZ compilation-order hypotheses.
Use this before inventing or renaming a production-source construct.

This guide supplies naming defaults, not source-model proof. Binary Ninja,
`support/Recoil.exe`, source-path literals, current retail function/data order,
the unified tracker, and governed VC5SP3 output remain authoritative for Recoil
identity, ownership, placement, order, provider classification, and acceptance.
The current `src/` tree is implementation state, not original-source authority.

## Evidence Order

Choose the first spelling or path supported by the following evidence order:

1. Exact Recoil evidence: source-path literal, runtime class name, mangled/type
   spelling, table identity, or corroborated Binary Ninja fact.
2. A directly evidenced Recoil class, namespace, subsystem, or source-file
   family whose spelling pattern covers the new construct.
3. The pinned same-engine SOCOM catalog under
   `support/engine_terminology/`, especially high-confidence terms recovered
   from both binaries.
4. Corroborating morphology from the comparative GameZ reconstruction tree.
5. The defaults below when no exact or family spelling survives.

Use these labels when describing the basis for a naming decision:

| Label | Meaning |
|---|---|
| `Recoil-confirmed` | Direct Recoil evidence supports the exact spelling or path. |
| `Same-engine default` | Same-engine terminology or a repeated family supports the default, but Recoil does not expose the exact spelling. |
| `Placement heuristic` | A pattern predicts a likely path or order; it proves neither. |

Stronger evidence replaces a weaker default. A friendly Binary Ninja name,
current source spelling, or aesthetically consistent name is not evidence by
itself. Naming evidence never proves that a construct is a class, determines
its source owner, or accepts source shape, order, provider status, or tier.

## Required Naming Procedure

Before adding or renaming a class, type, namespace, subsystem, folder, or file:

1. Classify the source construct from current Recoil construction,
   destruction, table-write, `this`-use, layout, and dispatch evidence. Prove
   class/interface ownership before applying a class prefix.
2. Search Recoil Binary Ninja facts and source-path/name literals.
3. Search `support/engine_terminology/cpp_names.tsv`,
   `source_files.tsv`, and `identifier_terms.tsv` before inventing a spelling.
4. Identify the evidence label above and use the strongest available spelling
   or family.
5. Apply a default below only when the exact spelling remains unknown.
6. Check the registered VC5 order target and neighboring retail evidence
   before treating a path or order hypothesis as physical placement.

If evidence remains contradictory, keep the name or placement unresolved.
Do not manufacture a class, namespace, translation unit, or tracker claim
merely to satisfy a naming pattern.

## Authored C++ Class Prefixes

Once current evidence proves an authored C++ class/interface but does not
reveal its exact name, use this prospective default:

| Situation | Default |
|---|---|
| Unqualified GameZ class | `CZ<Name>` |
| GameZ class already inside a lowercase-`z` namespace | `z…::C<Name>` |
| Non-GameZ or MFC-style authored class | `C<Name>` |

Examples of the same-engine families include `CZAnimArg`, `CZWeapon`,
`zdb::CModel`, and `zdb::CWorld`. The SOCOM catalog also contains exact
unqualified `C…` terms such as `CGame` and `CBody`; an exact matching
same-engine term outranks the fallback `CZ…` form and must be evaluated against
Recoil evidence rather than mechanically rewritten.

The prefix default is strict only after class/interface ownership is proven.
It does not apply to:

- POD or serialized records
- callback/data structures and authored tables
- namespace or global-state subsystems
- provider, CRT, MFC, DirectX, COM, or compiler-owned types
- temporary layouts, partial records, or unresolved source models

Preserve exact contrary Recoil spellings. `RecoilApp`, `HudUiElement`, and
`zFMV_Action` are examples of evidenced Recoil families that must not be
renamed merely to make them start with `C` or `CZ`. Provider spellings come
from the corresponding provider headers and ABI, not this convention.

## GameZ Module Folders And Filenames

When a proven GameZ owner lacks an exact original path, default its module
folder to:

```text
GameZRecoil/z<Subsystem>
```

The `z` is lowercase and the subsystem component begins with a capital, as in
`zSound`, `zSystem`, `zUI`, and `zVideo`. Exact Recoil source-path evidence
always wins, including any casing or non-`z` exception.

Choose filenames in this order:

1. Preserve an exact Recoil filename or source-path literal.
2. Preserve an exact high-confidence same-engine filename from
   `source_files.tsv` when Recoil evidence does not contradict it.
3. Reuse the nearest evidenced family stem inside the same module.
4. Only then form an era-consistent default from that family.

Typical public umbrella headers include `zsnd.h`, `zsys.h`, `zgame.h`, and
`zvid.h`. Implementation families commonly use a lowercase stem plus a topic,
such as `zsnd_csnd.cpp`, `zsys_memory.cpp`, and `zvid_draw.cpp`. Other modules
use historically shorter stems such as `game_main.cpp`, `seal_ai.cpp`, or
`node_world.cpp`. Preserve the evidenced stem; do not regularize every file to
the folder name.

Do not assume:

- every implementation file starts with `z`
- every `.cpp` has a same-basename header
- every module uses the same abbreviation rule
- a modern `.hpp`, new `.inl`, or one-class-per-file layout is original
- the current production directory is proof of the historical path

### Comparative GameZ Evidence

A read-only comparison on 2026-07-27 found:

- all 38 inspected GameZ module folders used the `z<Subsystem>` form
- the tree contained 203 `.cpp` and 70 `.h` files
- 80 of the 98 pinned SOCOM source basenames matched it exactly, including case
- repeated umbrella-header and module-stem families matched the catalog

That comparison is corroborating reverse-engineering evidence, not original
source and not a durable dependency on the external project remaining
available. Folders that the comparative project itself identifies as
non-original do not establish a historical GameZ convention.

## Alphabetical GameZ Order Heuristic

When direct Recoil placement evidence is absent, use case-insensitive
Windows/VC5-style lexical order of likely original sibling GameZ folder or
source names as the default prediction. For example, a comparable `zSound` or
`zSnd` block is expected before `zSystem` or `zSys`.

This is a `Placement heuristic`, not an acceptance rule:

- apply it only among comparable unresolved GameZ sibling modules or
  translation units
- investigate a mismatch before moving code
- prefer source-path literals, neighboring retail function order, project or
  object evidence, and current Binary Ninja facts
- require the registered VC5 order target to establish generated order
- let direct Recoil and governed VC5 evidence override the prediction

Do not infer global alphabetical link order across application, GameZ,
provider, CRT, or compiler boundaries. The no-literal shelf
`[0x4b2960,0x4c0d20)` is a known global exception: its internal
`zGame -> zSys -> zUI -> zUtil` sequence is alphabetic, but the shelf itself is
out of global alphabetical position.

## Source Model And Placement

An address is an evidence key, not a default source unit. Recover the strongest
supported owner: class/interface, translation-unit cluster, record/table/
callback group, global object or static-member group, provider boundary,
subsystem, or strongly connected dependency group.

Prefer class recovery when constructors/destructors, offset-zero table writes,
stable `this` use, inherited cleanup, or dispatch xrefs agree. Do not encode an
authored class as a hand-written vtable, slot array, raw-offset view, ABI shim,
or collection of address-named helpers. Scalar deleting destructors and other
compiler lifecycle artifacts remain distinct from the authored destructor
body; provider-supplied variants remain provider facts.

Retail source-path literals outrank inferred layout. When literals are absent,
combine contiguous function/data order, xrefs, include and template emission,
VC5 object order, tables, and neighboring owners. Record an exact historical
path as `provisional_original_path` only while its evidence remains
provisional; `agent_source_path` names the current implementation location and
never proves the original path. A partial header or semantic block may span
several physical contributions, but each attached artifact must still name its
actual source construct and section.

Generated inline/template/COMDAT bodies belong with the source construct or
provider that caused their emission. Validate complete invocation leaves,
relocations, selection, provenance, and call-free topology before projecting a
compiler-local helper. VC5 tail merging may explain shared physical tails, but
does not merge source owners or authorize wrong-file helper placement.

## Source Trace Attachments

Attach durable trace rows directly to the owning declaration, definition,
table, or global object:

```cpp
/**
 * @recoil-anchor recoil:anchor:<stable-source-construct-id>
 * @recoil-artifact defines .text recoil:function:0xNNNNNN: Primary authored body.
 * @recoil-artifact emits .rdata recoil:data:0xNNNNNN: VC5-generated table.
 * Purpose: Explains why this source construct emits the artifact.
 */
```

Use `defines` for a primary authored definition and `emits` for an auxiliary
artifact caused by it. Data may be a primary source owner or an auxiliary
emission; decide from source semantics, linkage, xrefs, layout, and generation
cause rather than its section alone. Comments mirror tracker topology and
accept nothing by themselves. Contradictory ownership, placement, section,
extent, alias, or emission cause stays unresolved.

## Adding A Convention

Add a future convention here only when it saves repeated reconstruction work.
State:

- the construct and scope it covers
- its evidence label
- direct Recoil evidence or pinned same-engine examples
- the default spelling or path
- known exceptions and what evidence overrides it

Do not turn speculative observations, current-source cleanup preferences, or
live tracker state into naming policy.
