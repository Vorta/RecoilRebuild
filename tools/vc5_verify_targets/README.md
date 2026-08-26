# VC Verification Targets

This directory contains local VC object-byte verification manifests. The path
and tool names are historical, but verification is VC5SP3-only for authored
production functions. New targets must use documented VC5SP3 profiles unless
the entry is an accepted provider/import boundary where no authored compiler
comparison applies.

Each authored production target should compile the real implementation through
`source_from` so verification covers the source under `src/`. Manifest-local
copies of production bodies and generated project-header shadows are rejected.
Canonical source-trace requirements are governed by the strict and migrated
policies below; ordinary VC validation does not require an address-bearing
legacy provenance comment. Passing VC byte evidence does not override
raw-offset, non-original helper, touched-global data, or source-shape metadata
gates.

Normal `functions` rows for non-authored diagnostics may use `provenance` to
record `provider-boundary` or `compiler-emitted-noncovering` classification
without inventing a source edge or retaining a local legacy address marker.
Any supplied canonical claim is still validated and is rejected for an
unresolved or not-applicable row.

Canonical source policy uses the shared source-trace grammar. Attach one
Doxygen block directly to the complete authored function/data definition or to
the legitimate construct that causes VC5 to emit a generated artifact:

```cpp
/**
 * @recoil-anchor recoil:anchor:hud.briefing-runtime.type
 * @recoil-artifact emits .text recoil:function:0x403d90: VC5 scalar deleting destructor.
 * @recoil-artifact emits .rdata recoil:data:0x4cd388: VC5-generated vftable.
 */
struct HudUiBriefingRuntime {
    // ...
};
```

The directive grammar is exact and lowercase:

```text
@recoil-anchor recoil:anchor:<stable-id>
@recoil-artifact <defines|emits> <exact-output-section> <artifact-id>: <description>
```

Use `defines` for a directly attached authored function or data definition and
`emits` for compiler-generated rows. Function artifacts name `.text`; data
artifacts name their exact `.data` or `.rdata` section. The artifact id is the
exact physical tracker id, or a reviewed logical id when the manifest row has a
`logical_identity_key`. A canonical directive must be unique in the recursive
source closure and must pass the shared attachment/parser checks.

Rows with an explicit compiler-generated `authored_order_role` may retain
`emission_anchor` while they migrate. It is a semantic, causal cross-check for
the current source shape:

`emission_anchor` is a semantic, causal contract for the current source shape.
Its `path` records where that contract is attached; it does not claim an
original header, a historical source-file block, the translation unit that
physically instantiated the contribution, or any owner/order/tier acceptance.

```json
{
  "address": "0x403d90",
  "symbol": "??_GHudUiBriefingRuntime@@UAEPAXI@Z",
  "name": "HudUiBriefingRuntime scalar deleting destructor",
  "pipeline_class": "authored-lifecycle",
  "authored_order_role": "compiler-generated-deleting-variant",
  "emission_anchor": {
    "path": "src/Battlesport/briefing.h",
    "kind": "type-definition",
    "name": "HudUiBriefingRuntime"
  }
}
```

When supplied, the named construct must resolve exactly once in its
repository-relative `path`, the path must be reachable from the recursive
quoted-include closure, and the canonical artifact must attach to that exact
construct. `function_address_refs` preserve the top-level metadata.

Only whitespace may separate the Doxygen block from the construct. Detached
registries, `//` comments, ordinary `/* */` comments, comments attached to an
`#include`, wrong relations, wrong binary/logical ids, and wrong sections do
not qualify.

Anchor kinds are constrained by role: deleting variants use
`type-definition`; EH helpers and implicit cleanup rows use
`function-definition` or `data-definition`; thunks use `function-definition`
or `type-definition`; ICF representatives use `function-definition`. A type
anchor is a complete class/struct definition, and a data anchor is a
non-`extern` definition. In particular, a scalar deleting destructor anchors
to the class definition whose virtual lifetime model causes VC5 to emit it,
not to an arbitrary `delete` expression or `operator delete(this)` statement.

Default validation does not require legacy `Reimplements 0x...`,
`Reimplements data 0x...`, or `Emits 0x...` comments. Canonical directives are
validated when present, while required-versus-omitted source edges are enforced
only by `--strict-source-traceability` for resolved authored VC rows or by the
repository migrated audit. The historical `--strict-source-emissions` option
keeps its narrower compatibility contract: compiler-generated authored-order
rows must have valid source-anchored `emission_anchor` metadata. An old attached
`Emits` spelling may remain temporarily and is validated when supplied, but it
is not required. Order-only verification continues to use that narrow emission
anchor check.

Use the explicit `--strict-source-traceability` option with either `verify vc5`
or `guard vc5-manifest` to enforce canonical relationships for resolved
authored VC rows. An authored function uses `defines`; an explicit
compiler-generated function uses `emits`. Data directives present in a VC
source closure are checked for exact id, data section, direct attachment, and
`defines`; the repository-wide
`audit source-trace --policy migrated` command remains authoritative for
whether a data artifact is resolved/direct, compiler-emitted, unresolved, or
not-applicable. Unresolved/provider rows do not acquire a canonical edge merely
because a VC manifest names them, and an inline canonical claim for such a
function fails.

Any supplied malformed, unreachable, ambiguous, detached, duplicated,
mismatched, wrong-section, or wrong-id canonical directive is an error.
Neither mode changes tracker state or qualifies owner/order/byte/tier
acceptance. The legacy provider and generic
`compiler-emitted-noncovering` markers remain compatibility-only when the row
does not carry an explicit compiler-generated role.

Targets may also declare generated data under `data_symbols` when a BN data
symbol needs comparison against a VC5 COFF `.data` or `.rdata` symbol:

```json
"data_symbols": [
  {
    "address": "0x4cd388",
    "symbol": "??_7HudUiTransitionTextPanel@@6B@",
    "name": "g_HudUiTransitionTextPanel_FTable",
    "bn_name": "g_HudUiTransitionTextPanel_FTable",
    "byte_length": 148
  }
]
```

Data-symbol comparison masks COFF relocation fields and writes a relocation
identity report for pointer/symbol review beside the byte diff. Zero unmasked
data-byte mismatches are tool evidence only; owner tier promotion still requires
the normal source-owner, source-shape, and data-gate review.

For source-file blocks proven by Binary Ninja source-path literal xrefs or
neighboring function order, a manifest may opt into `check_function_order`
generated order checking:

```json
"check_function_order": true
```

When enabled, `python tools/recoil.py verify vc5 <target>` compares the manifest
function list order, which should be the retail BN address order, against the
generated VC5 COFF section/value order. This detects source-shape or include
placement drift before byte mismatches are treated as the only remaining
blocker. A passing function-order check does not prove byte identity or owner
gate acceptance by itself.

Order manifests can classify each expected row for the authored-first pipeline:

```json
{
  "address": "0x401000",
  "symbol": "??0CAboutDlg@@QAE@PAVCWnd@@@Z",
  "name": "CAboutDlg::CAboutDlg",
  "pipeline_class": "authored-lifecycle",
  "required_presence": true,
  "full_order_gate": true
}
```

When every top-level `functions` row in one target has the same class, the
manifest may state that class once without silently relying on the unresolved
default:

```json
"function_defaults": {
  "pipeline_class": "authored"
}
```

`function_defaults` currently accepts only `pipeline_class`. Individual rows
override it. Translation-unit and linked-interval entries may reuse those
fully parsed top-level identities with an ordered `function_address_refs`
list. A ref must resolve exactly once in the same manifest; refs cannot be
combined with an inline `functions` list and never infer a symbol or class.

`pipeline_class` is one of `authored`, `authored-lifecycle`, `non-authored`, or
`unresolved`. `required_presence` controls whether the identity must resolve;
it defaults to true. `full_order_gate` controls whether the row participates in
the complete selected-order gate and defaults to the row's presence policy.
Every `translation_unit_function_order` or `linked_function_intervals` entry
selects `"order_scope": "authored"` or `"order_scope": "full"`.
Authored scope still enforces required identities but compares relative order
only for `authored` and `authored-lifecycle` rows. It therefore exposes
non-authored placement without letting a provider/compiler COMDAT transposition
block the authored source-order checkpoint. Full scope checks every enabled
authored and non-authored row and remains the exact linked interval/seam gate.

Known selected candidate-only non-authored contributions can be classified
without fabricating retail addresses. Nest them beside `functions` in the
relevant translation-unit or linked-interval entry:

```json
"candidate_only_extras": [
  {
    "symbol": "??_GCAboutDlg@@UAEPAXI@Z",
    "name": "CAboutDlg scalar deleting destructor",
    "pipeline_class": "non-authored"
  }
]
```

Each candidate-only row uses exactly one of `symbol` or `symbol_regex`, has no
retail `address`, and is inventory/classification evidence rather than a retail
order identity. Omit the array when no specific candidate-only classification
is needed. Do not copy ordinary raw-only extras into this array: unlisted raw
definitions remain part of the generated inventory and are mechanically
nonblocking, while `candidate_only_extras` records a specifically classified
candidate selected contribution.

For cross-translation-unit source-block provenance diagnostics, a manifest may
opt into a compile-only order check across multiple separately compiled
production sources:

When one physical translation unit needs a reviewed compile profile that
differs from the target-wide default, use an exact `source_compile_profiles`
map. Keys are normalized repository-relative source paths; globs, unknown
sources, normalized duplicates, and unknown or compiler-environment-mismatched
profiles are rejected.

```json
"compiler_profile": "vc5_o2_ob0_md_gx_facs",
"source_compile_profiles": {
  "src/Battlesport/hud.cpp": "vc5_o2_ob0_md_gx_fastcall_facs"
}
```

The map applies independently to the primary `source_from` and every
`translation_unit_function_order[].source_from`. Effective profile names,
flags, and compile commands are resolved live for each run.
`compile_context_from` inherits and validates
the referenced final-build manifest's map; a target must not shadow that map
locally. Final-context verification uses each profile's reviewed
`final_build_compile_flags`, then appends the final manifest defines and the
reporting-only `/FAcs` switch. A command-line `--compiler-profile` or profile
sweep is deliberately target-wide and suppresses per-source selections for
that diagnostic run.

```json
"check_translation_unit_function_order": true,
"translation_unit_function_order": [
  {
    "source_from": "src/GameZRecoil/zImage.cpp",
    "functions": [
      {
        "address": "0x4b0000",
        "symbol": "?ImageBody@@YAXXZ",
        "name": "ImageBody"
      },
      {
        "address": "0x4b0010",
        "listing_label_regex": "\\$L100",
        "name": "ImageBody::CompilerLocalCleanup",
        "provenance": "compiler-emitted-noncovering"
      }
    ]
  },
  {
    "source_from": "src/WinMain.cpp",
    "functions": [
      {
        "address": "0x4b0100",
        "symbol": "?WinMainTail@@YAXXZ",
        "name": "WinMainTail"
      }
    ]
  }
]
```

Run this diagnostic with `--skip-bn-compare`; the tool rejects the manifest in
normal BN/byte-comparison mode. Each entry is copied from production
`source_from`, compiled with the manifest compiler profile/flags/includes, and
checked in the declared translation-unit order. The report prints each expected
function in manifest retail order with its generated object index and COFF
section/value, and fails when an expected identity is missing, ambiguous,
duplicated, resolves to the same contribution as another expected row, or
breaks relative order within the declared object sequence. Every other raw
defined-code contribution is inventoried with its aliases and COMDAT/weak
metadata but is informational and does not block. Exact selected population is
enforced only by `linked_function_intervals`, where an unlisted linked address
group or wrong seam remains a failure. The retired
`exact_defined_function_set` field is rejected with a migration error.

A row may use `listing_label_regex` instead of a COFF
`symbol`/`symbol_regex` only in `translation_unit_function_order` diagnostics
when the generated `.cod` listing has an anonymous/local compiler label that has
no COFF symbol. The tool resolves the label to the enclosing COFF code PROC and
local listing offset so it can be placed among COFF symbol rows for order
diagnostics; such rows are not byte-covering functions or COFF symbols. Under
strict traceability, every resolved authored function requires a directly
attached canonical `@recoil-artifact defines .text ...` relationship in the
production source or included project headers. Provider, unresolved, and other
not-applicable rows receive no source edge. Explicit resolved
compiler-generated roles instead require `@recoil-artifact emits ...`; the
narrow strict-emissions compatibility policy requires only their validated
`emission_anchor` metadata. Their role takes precedence over direct-definition
provenance.
Manifest-local generated project-header shadows remain rejected. This is
diagnostic/process evidence only: it does not replace BN comparison, byte
identity, owner/data/linkage gates, or source-file order review.

Final-build manifests can consume exact linked-image intervals from a VC5
verification target:

```json
{
"name": "map_mission_order",
"description": "Exact linked map-to-mission function interval.",
"linked_function_intervals": [
  {
    "name": "map_to_mission",
    "predecessor": {
      "address": "0x415aa0",
      "symbol": "?MapPredecessor@@YAXXZ",
      "name": "MapPredecessor"
    },
    "functions": [
      {
        "address": "0x415ab0",
        "symbol": "?FirstMapFunction@@YAXXZ",
        "name": "FirstMapFunction"
      }
    ],
    "successor": {
      "address": "0x417350",
      "symbol_regex": "\\?MissionInit.+",
      "name": "MissionInit"
    }
  }
]
}
```

For an interval beginning at the start of a linked section, replace
`predecessor` with a real section-boundary sentinel instead of inventing a fake
predecessor function:

```json
"predecessor_section_boundary": {
  "section": ".text",
  "address": "0x401000"
}
```

The first listed function must resolve exactly at that section boundary. An
interval must provide exactly one of `predecessor` or
`predecessor_section_boundary`.

A linked-only target needs only `name`, `description`, and
`linked_function_intervals`. It does not need `source_from`, `source_filename`,
compiler profile/flags, include directories, or source-file lists. Linked-only
targets participate in manifest discovery and final-link `--order-target`
loading, but are excluded from `verify vc5 --source-from`, `verify vc5 --all`,
address coverage, ordinary VC5 compilation, manifest-local source debt checks,
and compiler-profile usage counts. A target that also has normal
`functions`, `data_symbols`, or `translation_unit_function_order` remains a
mixed compile target and retains the normal production-source policy.

Run one or more targets after the normal final link:

```powershell
python tools/recoil.py verify final-build -- --no-pe-compare --no-resource-compare --order-target map_mission_order
```

`--order-target` is repeatable and names a VC5 verification manifest. The final
build parses both `Publics by Value` and `Static symbols` from the map generated
by that same invocation, groups aliases at one linked address, retains
`Lib:Object` providers, and requires the predecessor, every function row, and
the successor to be the exact selected function sequence. Missing, duplicate,
reordered, or unlisted selected functions fail. The live result exposes
provider, RVA, alias, disposition, and first-divergence fields directly.
`--compile-only` is rejected when an order target is requested. Linked-order
success is order/process evidence only and does not prove byte identity,
source ownership, source shape, owner gates, or owner tier acceptance.

This directory is local ignored verification state. Agents may create or update
targets here for current evidence, but must not stage or commit them. Durable
evidence belongs in source docblocks/comments, `.agent/RECONSTRUCTION_PROGRESS.sqlite3`
through `python tools/recoil.py progress ...`, `docs/reconstruction/`, or narrow
subsystem docs.
