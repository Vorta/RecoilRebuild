# Retail Executable Reproduction

This is the canonical operational runbook for rebuilding the 1999 Windows x86
Recoil executable from source with VC5SP3.

## Acceptance Model

The goal is source-faithful C/C++ whose unrestricted build reproduces every
catalogued typed semantic fact in `support/Recoil.exe`.

Acceptance always uses current source and fresh governed output. A candidate
hash, source hash, object hash, saved executable, receipt, snapshot, or raw
whole-file match cannot qualify work. The linker COFF timestamp and raw file
delta are diagnostic only.

Expected truth comes from:

1. immutable retail bytes and PE structure;
2. current assembly, xrefs, imports, layouts, literals, tables, and function
   boundaries in the already-open `Recoil.bndb`;
3. reviewed tracker identity/provider/alias facts;
4. current source and fresh VC5SP3 output only as the candidate.

## Current Task

With no explicit target:

```powershell
python tools/recoil.py progress next --json
```

The result is one serial task. Work it directly in the canonical checkout.
There is no secondary queue or allocation step.

The stages are:

1. authored function order;
2. authored call contracts;
3. authored bytes;
4. full linked function order;
5. linked bytes;
6. final typed validation.

Do not move to full order until authored bytes are complete.

## Stage 1: Authored Function Order

Retail traversal begins at `0x401000`. Each selected row must be reviewed as
`authored`, `authored-lifecycle`, `non-authored`, or `unresolved`.
Only physical `authored-body` and `authored-lifecycle-body` identities gate
the authored order.

Run the registered target:

```powershell
python tools/recoil.py verify vc5-order <target> --build-root <fresh-root>
```

The verifier reports the first missing, duplicated, unexpected, or reordered
identity and its neighbors. Edit only the source/header closure implicated by
the current target. Recover declaration order, source-file placement, header
layering, include timing, and structured source that naturally produces the
retail order.

Do not use pragmas, linker-order manipulation, duplicate definitions,
wrong-file helpers, post-link patching, or source-level `goto`.

One unresolved row inside the target interval blocks acceptance of the complete
covered physical slice. Raw definitions omitted from the target are diagnostics
because the linker may discard or fold them; full linked order later requires
the exact linked population.

Fresh acceptance:

```powershell
python tools/recoil.py progress advance-live-order --target <target-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

The same invocation builds, validates, derives safe complete slices, and
CAS-updates only order state.

## Stage 2: Authored Call Contracts

The live census is derived from reviewed authored-order classifications, not a
fixed historical body count. Slices are deterministic retail-contiguous windows
of at most 160 bodies.

The direct verifier compares:

- exact invocation count and order;
- call versus tail-call form;
- direct versus indirect dispatch;
- authored/self/provider/IAT target identity;
- virtual/interface slot or callback storage;
- known caller cleanup;
- current direct Binary Ninja facts from the target-qualified loaded database.

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --build-root <fresh-root> --json
```

Unresolved target, storage, provider, import, ICF/logical-alias identity, or
indirect provenance fails closed.

Fresh acceptance:

```powershell
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

Only bodies passing in that invocation advance. An unrelated divergent body
stays pending and keeps the cursor on its first slice.

The normal authored-call-contract route is the serial replay command; it
replaces ad hoc shell loops across the complete original-slice census:

```powershell
python tools/recoil.py progress call-contract replay-live --dry-run --json
python tools/recoil.py progress call-contract replay-live --apply --json
```

Dry-run plans the complete original-slice census and fresh replay sibling root
without building, querying Binary Ninja, or mutating. Apply loads the serial
task once, creates an exclusive `-replay-NNN` sibling without consuming the
scheduler-selected direct root, and performs one invocation-local complete
proof. Repository/source discovery, each unique target build, each separate
definition-TU build, the COD index, and target-qualified immutable Binary Ninja
facts are shared across the census. The proof is projected back into the exact
original slices and committed serially through the same per-body evidence and
semantic/evidence CAS path as direct acceptance. Already-current predecessor
slices are revalidated. The first divergent current slice commits only its
passing non-current bodies and stops; later slices remain untouched. Interrupted
roots are inert, and a resumed invocation uses a new sibling. Replay neither
supplies expected truth nor performs the mandatory closeout. After the last
slice passes, it returns the scheduler-selected `close-live` command without
running it.

After all bodies are current:

```powershell
python tools/recoil.py progress call-contract close-live --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

The closeout runs the complete current census from fresh output, forbids reuse,
requires zero divergence, queries the target-qualified canonical BN database
directly without repeating database preflight per slice, records each slice's
exact expected-fact transcript, and records generation 34/30/31 currency. It is
the only route to stage completion.

## Stage 3: Authored Bytes

The public authored acceptance command is:

```powershell
python tools/recoil.py progress advance-live-authored-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Authored equality requires:

- exact object body bytes outside relocation fields;
- exact relocation type;
- immutable-retail-derived target/provider/alias identity;
- exact relocation addend semantics;
- linked presence and symbolic identity;
- relocation-normalized linked body bytes.

Expected relocation facts never come from candidate output:

```powershell
python tools/recoil.py audit relocation-expectations --at 0xNNNNNN --json
```

An explicit empty expectation is valid. A missing deterministic target identity
uses reviewed `progress relocation-target bind`. A genuine ambiguity alone
uses reviewed `progress relocation-exception set`. Both are dry-run-first.

## Stage 4: Full Function Order

Full order begins only after authored call-contract closeout and authored byte
completion. Restart at `0x401000`.

Require exact:

- selected linked physical address groups;
- identities and RVAs;
- relative order;
- provider/compiler-generated members;
- predecessor/successor seams;
- padding intervals.

Use the current task’s registered target and the same order validation/acceptance
commands as stage 1. Passing raw object order is feedback only; acceptance is
the linked full-order result.

## Stage 5: Linked Bytes

```powershell
python tools/recoil.py verify linked-byte --at 0xNNNNNN
python tools/recoil.py progress advance-live-linked-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Linked validation requires exact linked RVA, resolved operands, target identity,
and raw linked-image bytes. It may advance only explicitly matched physical
groups before the first typed divergence.

## Stage 6: Final Typed Validation

The live catalog partitions file-backed and loaded RVA intervals and joins
accepted tracker facts for functions, variables, providers, resources, storage,
directories, padding, zero-fill, relocations, and overlay.

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

Every range must be covered exactly once. Gaps, overlaps, unknown extents,
ambiguous padding, missing providers, or unresolved entities block before the
unrestricted build.

## Source Shape

Addresses identify evidence, not source units. Expand work to the proven
source-level construct: class/interface, TU cluster, table/callback group,
record subsystem, global object/static-member group, provider boundary, or
strongly connected dependency group.

Prefer class recovery when construction/destruction, offset-0 table writes,
`this` use, inherited cleanup, or dispatch xrefs support it. Do not model
authored classes as raw offsets, slot arrays, synthetic vtables, or provider
shims.

Consult `docs/reconstruction/source_naming_conventions.md` and
`support/engine_terminology/` before inventing production names and paths.
The current `src/` layout is not evidence of original placement.

Attach canonical `@recoil-anchor` and `@recoil-artifact` rows to the actual
source construct. A comment mirrors traceability; it does not accept an owner,
model, provider, tier, order, byte, section, or final fact.

## Provider And Tier Separation

Provider/import/external artifacts are not authored source. Use
`recoil-provider-boundary` to classify CRT, MFC, DirectX, COM, compiler, and
runtime boundaries.

Authored owner tiers progress from C to B to A to S. The owner tier is the floor
of all primary entries and required gates. Passing behavior, one function’s
bytes, order, or data equality never promotes a complete owner implicitly.

Use `recoil-tier-verification` before promotion and
`recoil-source-owner-scrutiny` for positive owner/data/linkage/tier-B+
acceptance.

## Raw Assembly Exception

Raw assembly is considered only after credible source-faithful VC5 C/C++
variants fail and a triggered ChatGPT Pro pass advises that it is necessary.
Require exact BN/VC5 opcode/register/FPU evidence, a nearby purpose docblock,
and an address-scoped allowlist entry. Keep the block minimal.

Naked functions, `_emit`, standalone assembly files, whole-function assembly,
raw stack shells, provider shims, and order tricks are forbidden outside
documented CPU-probe exceptions.

## ChatGPT Pro

Invoke the persistent Pro line directly only when evidence leaves competing
source models, a disputed correction crosses an ownership/TU/provider/block
boundary, raw assembly is proposed after credible failures, or the user asks
for external critique.

Pro is advisory. Preserve the competing models and primary evidence in the
prompt, ask it to challenge the interpretation, and return to direct retail/BN/
VC5 evidence for every actual acceptance.

## Binary Ninja

Once after Binary Ninja is opened, reopened, or switched for the current
working session:

```powershell
python tools/recoil.py binja preflight --binary recoil --strict
```

If the bridge is unavailable, ask the user to open the correct database. Do not
load or switch binaries. Reuse the preflight result while the database and
connection remain unchanged; never repeat it per slice. Read-only inspection
never saves. Reconstruction edits only explicitly selected analysis facts,
runs propagation checks, and saves before returning to source work.

## Persistence And Recovery

The two live SQLite databases are the only runtime authorities. Never hand-edit
them. Semantic mutations use CAS and fresh evidence. Issue mutations use their
independent monotonic revision.

The README contains only a static pointer to `progress next --json`. It is not
a second current-state authority and no tracker transaction updates it.

## Validation

After tool, docs, skill, or tracker work, use the canonical matrix in
`recoil-validation` rather than maintaining another copy here.

Run focused unit tests first. Preserve unrelated changes. Report exact commands,
first divergences, changed paths, remaining gaps, and the narrow next action.
