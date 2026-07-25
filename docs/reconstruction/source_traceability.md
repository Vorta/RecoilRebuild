# Source-To-Artifact Traceability

## Purpose

Source traceability answers one narrow question: which legitimate source
construct directly defines, or causes VC5SP3 to emit, a catalogued retail
function or data artifact?

It does not answer whether the source is behaviorally correct, source-faithful,
in retail order, byte-matching, provider-correct, owner-complete, tier-ready, or
covered by final-image validation. Those remain independent live gates. The
unified reconstruction tracker owns the relationship graph; source comments are
strictly validated mirrors placed where a reviewer needs them.

## Canonical Source Form

Attach one docblock immediately to a complete function definition, data
definition, complete type, recognized source-generation region, or explicit
instantiation:

```cpp
/**
 * @recoil-anchor recoil:anchor:subsystem.source-construct.type
 * @recoil-artifact emits .text recoil:function:0xNNNNNN: VC5-generated helper.
 * @recoil-artifact emits .rdata recoil:data:0xNNNNNN: VC5-generated table.
 * Purpose: Describes the legitimate source construct and its emitted artifacts.
 */
```

The grammar is:

```text
@recoil-anchor recoil:anchor:<repository-unique-stable-id>
@recoil-artifact <defines|emits> <output-section> <artifact-id>: <description>
```

The anchor identifies the source construct and must remain stable across
ordinary file moves and symbol renames. The artifact id identifies a tracker
physical artifact or reviewed logical alias. The section is the exact final
output section, such as `.text`, `.data`, or `.rdata`. The description is for
humans and is not identity.

Use `defines` for the direct authored definition attached to the docblock. Use
`emits` when compiling the attached legitimate construct creates an additional
compiler/linker artifact: a destructor variant, thunk, exception helper, RTTI
record, vftable, constant pool item, or another catalogued function/data item.
One anchor may have many rows; every row names exactly one artifact.

An ordinary source construct that has no reviewed retail artifact relationship
does not need an anchor or an address-bearing docblock. Do not turn a function
name, variable name, or filename into a title-only block merely to satisfy a
mechanical inventory. Human prose belongs in a traced block only when it adds a
truthful source-level purpose or evidence statement; repeating the attached
construct's name is not documentation.

The same hygiene rule applies outside attached Doxygen blocks. Ordinary
multiline comments, contiguous `//` groups, macro/include-generation regions,
and detached inventory regions may not substitute standalone qualified or
unqualified symbols, friendly Binary Ninja labels, source paths,
symbol-plus-path titles, routing placeholders, or lifecycle-contribution rows
for documentation. Exact repeated semantic rows inside one comment are
redundant. Canonical `@recoil-*` directives, `Purpose:`/`Evidence:` prose,
substantive sentences, and their wrapped continuations remain valid; an inline
non-period label is not treated as a standalone documentation row.

## Attachment And Identity Rules

- A docblock must immediately precede or validly enclose its source construct.
  Detached file registries, stacked docblocks, preprocessor-address registries,
  and end-of-file lists are invalid.
- Pure declarations are not anchors. Do not create a fake function, wrapper,
  global, table, type, macro, or explicit instantiation to hold an address.
- A physical artifact id may appear on only one resolved source edge. When
  several source constructs are folded or pooled at one retail address, create
  distinct reviewed logical artifact ids and record their ICF/pooling group.
- Provider, import, CRT/MFC/DirectX/COM, or other external artifacts are
  `not-applicable` and have no production source edge.
- If ownership, attachment, section, extent, logical identity, or the emission
  cause is disputed, record an `unresolved` tracker state and omit the source
  claim. A plausible comment is not evidence.
- Legacy `Reimplements 0x...`, `Reimplements data 0x...`, `Emits 0x...`, grouped
  address lists, and address-only compiler/provider registries are invalid.

Raw-assembly approval is a separate policy dimension. A
`@recoil-raw-asm`/`@recoil-raw-consumer` region proves only that the exact
allowlisted source region may contain assembly; it neither creates nor replaces
a `defines`/`emits` relationship. Conversely, source traceability never grants
raw-assembly approval.

Registered VC5 order manifests also refer to physical retail artifacts
independently of source traceability. A generated function may be required in a
physical order target even when its source-emission cause remains unresolved.
Manifest path/name hints are diagnostic and must not be treated as source
ownership. Strict source-emission validation requires an exact reviewed
artifact relationship or an explicit unresolved/provider classification, not a
filename coincidence.

## Tracker Boundary

The tracker records a per-artifact source-trace state:

- `resolved`: exactly one reviewed source edge names its anchor, relationship,
  translation unit, and evidence;
- `unresolved`: no source edge, plus a narrow reason code describing the open
  identity/placement/emission question;
- `not-applicable`: no source edge because the artifact is provider-owned or
  otherwise not a production authored source artifact.

Logical aliases and their physical ICF/pooling groups are explicit graph
entities. They are not represented by repeating the same physical address on
multiple source constructs.

An obsolete legacy address that names an instruction inside a known physical
artifact is provenance debt, not a second artifact or a logical alias. Resolve
it append-only as an exact-existing or strict-interior legacy claim with the
containing artifact and evidence. Such a resolution creates no source edge,
catalog artifact, or acceptance fact.

Only the revision-guarded `progress source-trace` mutation path may change the
tracker graph. Review its dry-run before applying. Source comments must then
match the resulting graph under:

```powershell
python tools/recoil.py audit source-trace --path src --policy migrated --json
```

Migration/inventory mode may report legacy syntax while a bounded conversion is
in progress, but it never treats legacy comments as accepted graph edges.

## Governed Migration

For a repository-wide conversion, keep review, source mutation, and tracker
mutation as distinct steps. The migration command preserves the original source
encoding and newline style and fails unless the complete non-comment C/C++ token
stream remains identical.

```powershell
python tools/_recoil/commands/source_trace_migrate.py template --path src --progress .agent/RECONSTRUCTION_PROGRESS.json --output <template.json>
python tools/_recoil/commands/source_trace_migrate.py review-conservative --template <template.json> --progress .agent/RECONSTRUCTION_PROGRESS.json --parent-reviewed --output <reviewed.json>
python tools/_recoil/commands/source_trace_migrate.py batch-propose --progress .agent/RECONSTRUCTION_PROGRESS.json --payload-file <reviewed.json> --output <batch-dry-run.json>
python tools/_recoil/commands/source_trace_migrate.py tracker-payload --path src --progress .agent/RECONSTRUCTION_PROGRESS.json --migration-report <reviewed.json> --migration-template <template.json> --header-overrides <header-overrides.json> --output <tracker-payload.json>
python tools/_recoil/commands/source_trace_migrate.py batch-propose --progress .agent/RECONSTRUCTION_PROGRESS.json --payload-file <reviewed.json> --apply --output <batch-applied.json>
python tools/recoil.py progress source-trace replace-batch --expected-revision <revision> --payload-file <tracker-payload.json> --dry-run --json
python tools/recoil.py progress source-trace replace-batch --expected-revision <revision> --payload-file <tracker-payload.json> --apply --json
```

Generate the tracker payload before applying the source rewrite: reviewed
legacy rows are still available then, and canonical annotations anywhere under
the same top-level source root are included when deciding whether an occurrence
is cleanup-only. Header anchors require reviewed translation-unit overrides
because a header can be compiled into several consumers; an override may name
one translation unit for the whole header or map individual artifact ids.

The final migrated-policy audit must report zero legacy inventory and exact
agreement between canonical source edges and the tracker. The migration and
tracker commands are topology-only: they do not accept owner, source model,
provider, tier, function order, bytes, data extent, or final-image coverage.

Resolved legacy rows always become canonical directives, including their
reviewed artifact descriptions. For unresolved or `not-applicable` standalone
legacy rows, migration removes the complete semantic row when its description
is only a construct title, source path, or mechanical routing/lifecycle
placeholder; substantive prose remains after the legacy address marker is
removed. Reviewed occurrence-only redundant rows are deleted when standalone,
while an embedded marker inside substantive prose retains the surrounding
sentence. Empty touched comment blocks and excess blank separators are removed
without changing source encoding, newline style, or non-comment tokens.

## Data Coverage

Trace `.text`, `.data`, and `.rdata` data artifacts, including compiler-emitted
switch tables, RTTI, message maps, vftables, literals, and constants. Register
the physical identity and exact extent before adding a source mirror; do not
create a named C/C++ global when the legitimate source construct is a switch,
type, framework macro region, or literal expression.

The parent-only data commands keep physical catalog facts separate from source
topology:

```powershell
python tools/recoil.py progress data-extent register --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json
python tools/recoil.py progress data-artifact register --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json
python tools/recoil.py progress data-artifact logical-alias register-batch --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json
```

Review each dry-run, then repeat the same command with `--apply`. Extent
registration can only complete the bounds of an existing physical data row.
Artifact registration creates one exact physical data identity with an empty
source-edge set. Logical-alias registration is for independently reviewed
logical source occurrences that share a provider-owned pooled physical
realization; it never duplicates the physical artifact or gives that provider
row a production source edge.

For example, several legitimate `""` expressions may compile to one VC5
literal symbol and one physical byte range. The physical row is
provider/compiler-literal-pooling and `not-applicable`; each reviewed source
occurrence is a distinct logical-data alias that may receive one `emits` edge.
Address equality alone is not pooling evidence.

These commands and source edges do not prove data bytes, relocations, storage
contribution, owner data gates, function order, tier, or final typed coverage.
Those facts remain governed by the byte, data-owner, and final-image workflows.
