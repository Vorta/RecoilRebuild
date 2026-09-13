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

The reviewed `recoil-logical-alias-group-v5` route represents shared-header
inline or implicit complete destructors. It separates the attached source
construct from its proven emitting translation units and retains one physical
authored gate. A target names one proved logical object witness; this is not an
assertion about the original linker winner. Every declared member emitted by
that TU must still exist in the fresh object as an independent ANY COMDAT with
identical complete bytes and relocation semantics. Only exact self-referencing
VC5 FPO associations are supported. Missing members, changed cleanup code,
unproved targets, duplicate source mirrors, and unknown physical artifacts fail
closed. Retail selector evidence and the paired same-object ICF/NOICF mechanism
proof remain mandatory; neither a selected witness nor a saved map accepts
linked placement or bytes.

An authored native implicit copy constructor or copy assignment has a separate,
narrow source-emission route. Its verification row sets `implicit_member_kind`
to `copy-constructor` or `copy-assignment`, names the exact VC5 decorated symbol,
and uses a `type-definition` `emission_anchor` for the owning class. The attached
class anchor must directly `emits .text` that function artifact. Its existing
resolved tracker source edge must name the exact emitting translation unit.
Authored classification, required presence, and both order gates remain enabled.

This source binding permits compilation; it does not establish an emission.
The verifier preprocesses the current translation unit with its actual VC5
includes and macro definitions, checks the class's compiler-reported source
origin, and rejects an explicit selected copy member, including one introduced
by a macro. It then requires the exact symbol in the same fresh TU's COD and
native ANY COMDAT, with valid extent, relocations, and associated COMDATs.
Missing definitions and ambiguous copy-like declarations fail closed. A
successful emission check establishes source provenance only; ordinary order,
call, relocation, and byte comparisons still apply. Enabling this route for a
production function requires the usual reviewed tracker source relationship.

For a read-only lifecycle diagnosis, `audit coff-lifecycle --object <obj>
--symbol <decorated-name> [--symbol ...] [--map <map>]` inventories complete
definitions, associations, inbound relocations, and map observations. Its output
is diagnostic only and does not establish freshness or accept tracker facts.

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
- selected retail-proven provider argument bits and their reaching-definition provenance;
- selected initializer dependencies on concrete constructor vptr writes and emitted dispatch targets;
- selected embedded-state dispatch dependencies on the concrete quit override and its live retail return semantics;
- selected background-thread dispatch dependencies on the complete object's constructor stamp, emitted update slot, and global receiver load;
- known caller cleanup;
- current direct Binary Ninja facts from the target-qualified loaded database.

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --build-root <fresh-root> --json --summary
```

Unresolved target, storage, provider, import, ICF/logical-alias identity, or
indirect provenance fails closed.

The private verifier is organized by evidence owner under
[`tools/_recoil/call_contract`](../../tools/_recoil/call_contract/README.md).
Its serial coordinator separates retail acquisition, candidate acquisition,
and comparison. Immutable retail facts create obligations; candidate facts
answer them with `proven`, `not-applicable`, `unresolved`, or `conflict` results.
Per-body `proof_results` explain these outcomes while the public result envelope,
slice projection, replay, and closeout contracts remain the same.

Instruction effects come from exact x86 bytes, including partial and implicit
register writes. Shared CFG reaching-definition checks bind IAT loads to the
register that reaches the transfer. Proof-map composition rejects contradictory
claims at the same location. Each side must establish its own receiver lineage;
comparison cannot copy the other side's receiver. Provider identity must be
independently bound by typed identity, ABI, and fresh native evidence rather
than inferred from an aligned call ordinal. Physical invocation contributions
retain their count, order, call/tail form, and direct/indirect dispatch through
helper and lifecycle projections.

When several registered regex rows identify the same physical address, the
unique live accepted authored-order target selects the governing row, not a
linked-order diagnostic registration. Different-address matches and duplicate
governing rows remain ambiguous. Selection does not bypass synchronization,
current order, exact physical identity, or caller COFF provenance checks.
HUD caller membership checks likewise require every named target exactly once,
without freezing the complete list of additional diagnostic registrations.

Before legacy call projections, the verifier rejects an unequal already-known
authored physical target at an aligned direct call site. This rejection-only
check requires matching zero-addend REL32 COFF and E8 listing evidence and
respects accepted physical/ICF identities. Historical constructor equivalence
cannot rewrite a known target mismatch into a passing contract. Inconclusive
prechecks still require complete ordinary extraction; they accept nothing.

Argument/dependency selection rules do not contain saved expected flag values
or table contents. Those facts are derived from the live retail body and
compared with the freshly compiled candidate. Unsupported bit operations,
conflicting CFG predecessors, missing constructor bodies, unbound table slots,
or unresolved target identities block the selected proof. Constructor listing
coverage permits only bounded post-return VC5 alignment, never additional code.
The save/load initializer obligation checks the complete relocated table extent
and selects deleting, activation, entry, and deactivation targets. Other
inherited/folded cells do not gain logical-alias or full-table byte acceptance.
Weak dispatch targets require the exact fresh COFF weak-external/default chain;
decorated-name substitution is not proof. The embedded quit-return obligation
does not accept a new physical/logical ICF identity. This is not a claim
that call contracts establish every ordinary data-flow or rendering behavior;
the later byte and final-image stages remain necessary.

The briefing update obligation follows entry `this` through a bounded normal
constructor CFG, requires the same explicit vptr stamp on every return, and
compares the generated three-cell table's update target with live retail.
The background-thread obligation also binds its actual slot-zero invocation to the
relocated global receiver load. Missing overrides, bypassed receiver loads,
ambiguous table relocations, and unsupported constructor instructions block
verification. This is a selected dispatch dependency, not acceptance of all
table cells or of arbitrary side effects inside constructor callees.

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
python tools/recoil.py progress call-contract close-live --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --max-workers 8 --apply --json
```

The closeout runs the complete current census from fresh output, forbids reuse,
requires zero divergence, queries the target-qualified canonical BN database
directly without repeating database preflight per slice, records each slice's
exact JSON-native expected-fact transcript, and records the current verifier
and expected-fact generations. The isolated slice verifiers use bounded
subprocess concurrency: eight slots by default, configurable from one through
21. Completion order is diagnostic; validation and storage always follow
retail slice order. After every slice passes and before tracker mutation,
closeout runs exactly one fresh canonical whole-program compile, COFF-alias
assembly, resource build, and link below the closeout root. This linkability
diagnostic must produce the executable and map, suppresses linked-order
evaluation and playground deployment, and accepts no byte, linked-order, or
final-image fact. It is never repeated per slice. The final closeout CAS is one
serial mutation and remains the only route to stage completion.

## Playground Build Safeguard (Not An Acceptance Stage)

### External gameplay diagnosis

For a user-requested Windows x86 reproduction, the bounded external debugger
is available through the workspace toolset:

```powershell
python tools/recoil.py diagnose gameplay-start --exe playground/Recoil-rebuild.exe --map <matching-linker-map> --output-dir build/diagnostics/<fresh-root>
```

The user or available Computer Use connection performs the menu actions. The
command launches only the named executable with its own directory as working
directory, records exceptions and periodic thread snapshots, and stops only
that exact process at the five-minute deadline. It injects no code or
breakpoints and accepts no reconstruction facts. Output contains raw x86
contexts, stack memory with explicitly heuristic symbol candidates, loaded
modules, and the last loading log line. Three one-second samples are taken
after a loading-log line has stayed unchanged for twenty seconds; log silence
alone is not classified as a hang. Existing game logs are copied before launch
and the new logs retained at completion.

Optional exact map symbols can be sampled with repeated `--watch-symbol`
arguments. `--timeout` is bounded to 1-300 seconds and `--snapshot-interval` to
1-30 seconds. An ambiguous/missing watched symbol, unavailable debugger API,
or unsupported host/target architecture fails explicitly. The tool requires
64-bit Windows Python and an x86 target. A diagnostic map is not acceptance
evidence or proof of correspondence to current production source.

The Windows structures/event lifecycle follow Microsoft's
[debug-event API](https://learn.microsoft.com/en-us/windows/win32/api/debugapi/nf-debugapi-waitfordebugevent)
and [WOW64 context layout](https://learn.microsoft.com/en-us/windows/win32/api/winnt/ns-winnt-wow64_context).

### Governed deployment

Before deployment, both canonical build modes also run the startup
regression guard against the same fresh object and linked image. Its present
scope includes the complete constant member-byte writes of the turret initializer
and selected unprojection, light-update, and model-rendering matrix-stack depth
on every reachable control-flow path, derived directly from immutable retail.
It detects missing/wrong initialization,
including the omitted active-trail flag, rejects unsupported initializer code,
and distinguishes branch-exclusive cleanup from sequential double pops despite
identical static call counts. The unprojection/light checks require reviewed
callee effects. Model-renderer checks cover local primitive invocations only,
not transitive callee effects. Unsupported control transfers, unresolved branches,
underflow, and object/linked byte or relocation target/addend disagreement block.
The call-contract verifier also enforces the selected path proof. It is not full
startup control-flow, whole-program call-contract, or runtime acceptance.
Failure reports `failure_stage: startup-contract` and leaves the playground exe
unchanged. No extra compile/link is performed for this guard during deployment.

The standalone non-deploying route is:

```powershell
python tools/recoil.py verify startup-contract --build-root build/live-validation/<fresh-root> --json
```

This performs one fresh canonical linkability build and then the same check.
The separate `--linkability-only` route itself still suppresses this guard and
all deployment, as required by call-contract closeout.

For broader read-only investigation of matrix-stack users in an existing
canonical build, use:

```powershell
python tools/recoil.py diagnose matrix-stack --build-dir <canonical-build> --output-dir build/diagnostics/<fresh-root>
```

The survey queries the open Recoil BN cross-reference census, authenticates its
instruction bytes against retail, and compares local primitive push/pop paths
with object/MAP-qualified candidate bodies. It reports matching local depths,
inspection candidates, and unresolved cases separately. Nonprimitive calls are
not analyzed transitively and branch predicates are unconstrained; a survey
flag is not itself a proven source defect, and a local match is not gameplay or
reconstruction acceptance. Bounded redisassembly uses `llvm-objdump` from PATH
when whole-section VC5 DUMPBIN loses synchronization after inline data.

For a requested playground test executable, use the explicit non-accepting mode:

```powershell
python tools/recoil.py verify final-build --playground-only --build-dir build/live-validation/playground/<fresh-root>
```

This mode requires an absent root, the canonical manifest/profile and tracker,
and a complete fresh compile, resource build, and link. It runs the same complete
authored linked-presence safeguard described below and deploys only if that
passes. A failed deployment is also a command failure. Partial builds, diagnostic
profiles, custom order targets, `--clean`, and `--linkability-only` cannot be
combined with it. It evaluates no linked order and accepts no order, byte, alias,
or final-image fact; those comparisons remain separate serial-stage work.
In particular, candidate ICF sharing is neither a retail alias proof nor a reason
to fabricate source solely to keep identical functions apart.

A normal canonical `verify final-build` checks required authored linked
presence across the complete accepted authored-order census before replacing
the playground executable. The current linked-order cursor does not limit that
presence check. Missing/stale target registrations, uncovered census members,
missing functions, and ambiguous matches block deployment and are reported as
`failure_stage: linked-presence`.

The accepted census owns membership: a compiler-lifecycle manifest role does
not remove a selected identity's presence obligation. Every census member must
still have required presence and an unambiguous linked selector.

The linked-presence guard checks selected symbolic identity presence only. It permits named
aliases to share an address and does not require retail RVAs, ordering, or body
bytes. It neither accepts a reconstruction stage nor proves runtime behavior.
The explicit `--linkability-only` diagnostic used by call-contract closeout
returns before this guard and before all playground deployment, preserving its
compile/resource/link-only contract.

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

Authored and linked byte scans obtain their fresh canonical artifacts through
the non-deploying `--linkability-only` build diagnostic. They require a successful
complete compile, alias, resource, and link result with deployment and linked-order
evaluation suppressed, then perform their own comparisons. The artifact producer
accepts no byte facts and never replaces the playground executable.

Expected relocation facts never come from candidate output:

```powershell
python tools/recoil.py audit relocation-expectations --at 0xNNNNNN --json
```

An explicit empty expectation is valid. A missing deterministic target identity
uses reviewed `progress relocation-target bind`. A genuine ambiguity alone
uses reviewed `progress relocation-exception set`. Both are dry-run-first.

A one-past-array comparison is a genuine per-operand ambiguity when the same
retail address is also the next object's start. Use `progress
relocation-exception set` for that operand, selecting the existing array's
`target_symbol_id` and object selector, with both `coff_addend` and
`resolved_target_addend` equal to the array's exact size. The narrow proof
requires known typed data bounds and an authenticated `CMP reg32, imm32`
operand at exactly `end_exclusive`. It rejects dereferences, other operations,
wrong addends and stale extents, and repeats these checks during live derivation.
Keep ordinary target lookup half-open and retain references to the adjacent
object. This review grants no function, owner or storage acceptance; the fresh
object and linked comparison must still prove the candidate operand.

Existing folded aliases need a site-specific exception review when their physical
address alone does not determine the logical callee. Adding an ordinary target
binding does not choose one alias from that group and can stale other reviewed
contexts by adding a global registration witness. Retract a mistaken binding of
a pre-existing symbol through `progress relocation-target retract
--target-symbol-id <id> --payload-json <json>` with the expected revision, first
`--dry-run --json`, then the reviewed `--apply --json`. Its exact payload fields
are `reviewed: true`, a nonempty `reason`, and `expected_binding` containing the
entire unchanged stored binding. It removes exactly that registration and
preserves the symbol, other bindings, ownership, providers and evidence.
Created-data bindings are outside this route. Retraction grants no acceptance;
follow the correction with fresh match verification.

A registered aggregate data slice resolves only within its retail field extent.
Its `object_offset` contributes to the expected COFF and resolved-target addend;
the containing object's base is not the field's address. Conflicting offsets,
nonzero function offsets, and incompatible provider bindings remain unresolved.

### Unused Assignments in Matching Source

The user authorized unused local assignments on 2026-09-11, including embedded
assignments that preserve an input in a temporary whose stored value is never
subsequently read. Such source can represent leftover captures from earlier
code. Unused storage alone is not a reason to reject an otherwise matching
C/C++ reconstruction or require renewed permission in similar future cases.

The approved example is `Player::UpdateFirstPersonCameraFromInput` at
`0x4059a0`, with both expressions inside the positive-speed branch:

```cpp
float unscaledMin, unscaledMax;
elevationMin = (unscaledMin = elevationMin) * elevationScale;
elevationMax = (unscaledMax = elevationMax) * elevationScale;
```

Document unused values and the byte-matching reason beside their declaration.
Compare complete current bodies and relocation semantics, and require the
normal linked presence/identity and normalized linked-body proof before a
match annotation or stage acceptance. The exact output supports the source
form's compiler compatibility; it does not prove the original variable names
or the authors' intended later use. This allowance does not change the rules
for raw assembly, unrelated dead functions, compiler flags or linker tricks.
Earlier camera audit/Pro statements rejecting this form solely for unused
assignments predate this explicit user authorization and are superseded on
that point; their compiler observations remain diagnostic evidence.

### Function Match Levels and Pro-Reviewed Fallback

`byte`, `instruction` and `commutative` are per-function levels, separate from owner tiers and
serial-stage acceptance. Mirror the current complete proof in the function's
attached canonical Doxygen comment with exactly one `@recoil-match byte`,
`@recoil-match instruction`, or `@recoil-match commutative`.
An unproved function has no such directive.
All levels require all relocation semantics, linked presence/identity and
normalized linked-body checks listed above. A masked object comparison alone
does not qualify for an annotation. Exact linked RVA is required at stage 5.

Always prefer exact bytes. The instruction alternative allows only general-purpose
register reassignment while retaining instruction operations, ordering,
boundaries, lengths, operand widths, constants, addressing and control-flow
targets. The pinned x86 decoder and paired register-bit dataflow prove value
equivalence through branches/loops, partial registers, implicit operands, flags,
stack operations and ABI boundaries. Unsupported effects fail closed. No opcode
replacement, instruction scheduling, changed stack layout, data/padding or
provider exception is implied.

Before using the instruction alternative, send ChatGPT Pro the current source/compiler
context, complete register differences, compiler evidence and credible failed
C/C++ variants. Require explicit compiler attribution and an engineering
judgment that no concrete credible source-faithful alternative remains untried.
Ask for the standalone `INSTRUCTION_MATCH_APPROVED` decision only if both hold;
a remaining required experiment, negative/ambiguous response, or transport
failure grants no fallback. Pro is advisory eligibility, never machine proof.

Register that specific reviewed decision, dry-run first:

```powershell
python tools/recoil.py progress match review-instruction --payload-file <review.json> --expected-revision <revision> --dry-run --json
python tools/recoil.py progress match review-instruction --payload-file <review.json> --expected-revision <revision> --apply --json
```

The payload contains exactly `symbol_id`, `reviewed: true`,
`decision: "compiler-register-allocation-only"`,
`no_remaining_credible_source_options: true`, `reason`, `attempts`,
`differences`, `source_context`, `prompt`, `answer`, `transcript`, and `receipt`.
The four artifact fields are repository-relative paths to the captured Pro
exchange. Source/compiler context and exact instruction differences must still
agree with the live comparison. Artifacts document review; they cannot replace
the current build. Refresh invalidates source-dependent match evidence on drift;
an exact-byte upgrade requires no additional Pro review.

The `commutative` alternative has a separate, narrow proof. Its initial support
is an exchange of the two binary32 memory factors between `FLD m32` and
`FMUL m32`. It retains each instruction's operation, position, length and
encoding form, ordered addition inputs/grouping, memory widths, output stores,
integer code and control flow. The verifier derives instruction/table boundaries
from retail, then follows paired load origins and x87 stack values through
balanced single-entry regions. No store, call or interior branch may intervene;
the sole permitted GPR write is the exact stack-argument load described below.
Every differing operand must belong to a specifically
proved load/multiply exchange. Unknown effects, mixed register reassignment,
changed embedded tables, padding, relocation targets or layout block.

This is a **conditional numerical match**. Its supported FP contract is owned
by `COMMUTATIVE_CONTRACT` in `tools/_recoil/lib/commutative_match.py`:

- finite binary32 values at each affected read in valid ordinary stable memory;
- the same fixed supported x87 control word, all exceptions masked, and enough
  free push slots at each region entry (the proof reports the number required);
- identical stored representations, including signed zero, plus equal integer
  state, control flow and ABI behavior;
- excluded transient live x87 operands, status, saved environment and dead physical registers cannot
  influence included observations through callers, callees or asynchronous inspection;
- normal completion through valid ordinary readable inputs and writable output;
  admitted stack arguments are initialized, live, readable four-byte slots with
  equal stable contents and unchanged addressing state; program-visible memory
  faults and debugging observations are excluded, and transparent paging or
  asynchronous services must not expose operand-read order or transient FP state;
- ordinary function entry and ABI call/return flow, without external interior
  entries or return-address manipulation.

Supported control words use an architected rounding mode and 24-, 53- or 64-bit
precision, never the reserved precision encoding. The proof preserves the opaque
incoming numerical stack; its capacity precondition is not inferred from the
region's maximum depth. Noninterference is a continuation obligation, not simply
permission to omit FP status fields from a comparison. Unresolved indirect calls
and jumps are rejected. Direct entries, including `LOOP`/`LOOPE`/`LOOPNE`/`JCXZ`,
come from the independent retail decoder. Status/environment observers and MMX
accesses that can expose physical x87 register state also block the initial proof.

An exact `FADDP` compares its actual two ordered operands and preserves the
separate histories of all other tracked slots. A pending unequal factor may
remain untouched until its own proved multiplication; it cannot be consumed
by an addition prematurely. `FADD m32` retains whole-stack equality.

A balanced region may also contain at most one byte-identical, unprefixed
`MOV reg32,[ESP+displacement]`, with a positive four-byte-aligned displacement,
no index or segment override, and a destination other than ESP or EBP. Complete
tracked x87 stacks must agree at the load. The destination cannot be a base or
index of any earlier or later floating-point read in either execution, including
exact memory additions. The terminal identical store may use that newly loaded
register. The proof records the exact instruction, width, destination, signed
displacement, unchanged ESP basis and implicit SS addressing. Integer state stays
equal through the common stack load; valid ABI argument memory is a reviewed
precondition, not inferred from the operand spelling. All other instruction and
control-flow restrictions remain. This is neither GPR reassignment nor a general
permission for intervening integer operations.

Proof/contract changes invalidate prior relaxed reviews. Renew the complete
caller/domain justification under the current contract and run fresh live proof;
old review eligibility must not be silently retained.

The kernel's `scope: normalized-function-body-only`, `accepts_function_match:
false`, `accepts_exact_bytes: false` and `pending_obligations` identify body
feedback. Its `exact` field means only equality of the supplied buffers; it
does not prove original object or image bytes. It takes no relocation catalog
or symbol identity to imply validation it does not perform. The enclosing live
verifier alone joins that body proof with current Pro eligibility, exact
relocation semantics, linked presence/identity and the corresponding linked
body from the same fresh build. Decoded instruction-span counts include
alignment instructions; embedded-table byte counts do not include that padding.

The live proof establishes equivalence under this contract; it does not prove
that arbitrary runtime matrices or external DLLs satisfy the assumptions.
NaNs/infinities, unmasked traps, volatile/MMIO/racing memory and FP diagnostic
observations are excluded. Load/multiply exception timing and saved environment
are architectural effects described by the [Intel x87 architecture manual](https://cdrdv2-public.intel.com/671436/253665-sdm-vol-1.pdf).
Do not infer runtime FP state solely from a CRT default. Record a concrete
caller/domain justification and retain the conditional contract in every report.

Send Pro the current source/compiler context, exact differences, credible failed
byte variants, proposed proof, FP contract and caller justification. Require
compiler operand-selection attribution, engineering exhaustion within the
governed constraints, and the standalone `COMMUTATIVE_MATCH_APPROVED` decision.
Negative/ambiguous advice and transport failures grant no eligibility. Register:

```powershell
python tools/recoil.py progress match review-commutative --payload-file <review.json> --expected-revision <revision> --dry-run --json
python tools/recoil.py progress match review-commutative --payload-file <review.json> --expected-revision <revision> --apply --json
```

Use the same exact payload fields as the instruction review, with
`decision: "compiler-commutative-operand-selection-only"`, plus `contract`
(the complete supported contract object) and a nonempty
`contract_justification`. The receipt must confirm submission, the answer must
contain the positive decision and occur in the transcript, and the source,
compiler context, exact differences, contract and proof version must remain
current. Pro grants fallback eligibility, never machine acceptance.

For a workspace census or an explicit function, use:

```powershell
python tools/recoil.py progress match refresh --all --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py progress match refresh --at <address> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Each invocation makes one complete fresh canonical non-deploying build,
classifies its selected functions, reports individual exclusions and records
only complete matches. It synchronizes annotations without changing source line
counts, preserves encoding/newlines and guards against intervening edits.
`--dry-run` still builds and previews; a later apply uses another fresh root.
This command does not advance any serial stage or owner tier. The stage 3/5
commands independently rebuild and accept their own eligible groups, recording
instruction and commutative proofs separately from exact-byte states. Mixed groups use their
weakest fully proved level; an unproved member blocks the group.

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
and raw linked-image bytes, or the approved instruction/commutative proof for
its permitted differences in authored bodies. It may advance only explicitly matched physical
groups before the first typed divergence.

## Stage 6: Final Typed Validation

The live catalog partitions file-backed and loaded RVA intervals and joins
accepted tracker facts for functions, variables, providers, resources, storage,
directories, padding, zero-fill, relocations, and overlay.

Coverage includes headers and section tables, linker-produced seams and
directories, import/export/provider boundaries, resources, initialized data,
BSS and other loaded zero-fill, base relocations, file/alignment padding, and
overlay bytes. Each file-backed or loaded-RVA interval has exactly one typed
owner; neither an unmapped gap nor two overlapping explanations may pass.

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

Every range must be covered exactly once. Gaps, overlaps, unknown extents,
ambiguous padding, missing providers, or unresolved entities block before the
unrestricted build.

Final comparison freshly re-proves accepted instruction and commutative matches
at their exact retail locations. Only the resulting proved differences may differ
in `.text` and the complete file; all other bytes and typed facts retain their
exact checks. Reports preserve exact-byte booleans and separately identify
instruction and commutative matches, retaining each commutative FP contract.
Passing with either relaxed level is not byte equality or tier S. A commutative
result remains conditional on its reviewed domain. Source annotations and old
reports never grant a final-image exemption.

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

When fresh retail layout evidence proves that existing standalone authored
data globals are fields of one native aggregate, use the reviewed
`progress data-artifact coalesce` command. Its exact snapshot payload must
enumerate all fields, padding, storage, the single owner, and target
registrations. Mixed code/data targets must preserve every non-data registration
fact exactly. Existing field source edges are archived as superseded physical
definitions; logical field views and the aggregate need fresh source topology.
Update the selected manifests to the reviewed aggregate before
the dry-run. The transaction preserves historical records and logical field
views, rejects unhandled references/overlaps, and invalidates call/byte evidence
without changing function order or accepting an owner/data/linkage gate.
It is not an extent-only shortcut or a provider-pooling alias operation.

The README contains only a static pointer to `progress next --json`. It is not
a second current-state authority and no tracker transaction updates it.

## Validation

Existing authored storage and owner acceptance use explicit scopes; they do
not advance the serial scheduler or change membership. For storage, run
`progress storage accept-live --storage <id> --dimension <dimension>` with an
absent `--build-root` below `build/live-validation`, `--expected-revision`, and
`--apply` (or diagnostic `--dry-run`). Repeat `--dimension` for the requested
subset of extent, object, relocation, order, link, raw, and zero-fill. Every
invocation builds all canonical sources and links once, compares registered
data definitions with immutable retail and the fresh linked image, and writes
only requested applicable dimensions that passed. Missing identities,
ambiguous ownership, unresolved extents, or unmodelled overlap block acceptance.

For an existing authored owner, first run
`progress owner review-context --owner <id> --gate <gate> --build-root <fresh-root> --json`
(repeat `--gate`, or use `--tier C|B|A|S` instead). This builds a current complete
comparison and returns an unreviewed JSON template. Review every primary entry,
the source closure, dependencies, and live comparison under the source-owner
scrutiny and tier skills. Fill the substantive observations and ALLOW rationale,
then set `reviewed` to true. Keep the generated context and comparison intact.
Use `progress owner accept-live` with matching gates, or
`progress owner promote-live` with the matching tier, plus `--payload-file`,
`--expected-revision`, and a new fresh build root. Dry-run and review first;
apply with another fresh root. Acceptance rebuilds and requires the reviewed
comparison to reproduce; the earlier build is never reused as acceptance.

Tier A reviews the complete current differences and grants no function match
annotation. Owner byte gates and tier S reject instruction and commutative matches.
Higher tiers require separately accepted boundary/source/data/linkage gates;
S additionally requires the owner byte gate and fresh provider comparisons.
Only the named gates or primary-entry tier promotions are written. Existing
higher entry tiers and accepted gates are retained only after rechecking their
stronger proof requirements in the same invocation. Owner invariants remain
mandatory. C and initial boundary review do not require dependent owners'
boundaries to be accepted first, so dependency cycles can be reviewed serially.

If an existing primary member lacks its default tier bookkeeping, use
`progress owner repair-entry-tiers` with `--payload-file`, the expected revision,
and dry-run first. The payload contains `reviewed: true`, a substantive `reason`,
and `current_owners` mapping exact owner ids to their complete current records.
This route adds only absent X records with no evidence; it preserves all
membership, gates and existing entries and accepts no reconstruction facts.

Pipeline reachability has two separate results. `current_task_reachable` checks
that the current scheduler task has the right public route, executable backend
and parser, revision arguments, and stage contract. A blocked task is reported
as blocked; a callable route does not imply that current source will pass it.
`completion_routes_complete` inventories the operations needed by all six
stages, the mandatory call closeout, authored storage, and existing authored
owner gates and tiers. It matches exact acceptance subjects and dimensions to
inspected command handlers, guards, and writers. Registration, replacement,
invalidation, downgrade, and provider-import registration cannot stand in for
positive acceptance of existing authored entities.

The combined audit's `passed` result requires both checks. Missing operations
remain explicit strict failures even when the current task has a valid route.
This is an audit of implemented command capabilities and required transitions;
actual reachability through source divergences and semantic prerequisites is
established only by fresh live verification. `doctor` runs this combined audit
once, after the other infrastructure checks. There is no separate
`audit pipeline-contracts` command.

After tool, docs, skill, or tracker work, use the canonical matrix in
`recoil-validation` rather than maintaining another copy here.

Run focused unit tests first. Preserve unrelated changes. Report exact commands,
first divergences, changed paths, remaining gaps, and the narrow next action.
