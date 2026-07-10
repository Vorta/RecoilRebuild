# Retail Executable Reproduction

## Goal

The workspace is complete only when source-faithful VC5SP3 C/C++ compiles and
links into a file whose complete SHA-256 equals immutable
`support/Recoil.exe`. Matching behavior, selected functions, normalized bytes,
or PE shape is intermediate evidence, not completion. Post-link patching is
forbidden.

Original retail COFF objects are unavailable. Prove the executable-bearing
facts that can be observed: the exact function/contribution inventory and
order, object bytes and relocations, linked addresses and bytes, provider
selection, and final executable identity. Do not claim equality of unavailable
COFF metadata.

## Authoritative Sequence

Run `python tools/recoil.py progress next` before any other no-target view.
`.agent/RECONSTRUCTION_PROGRESS.json` is the only reconstruction-progress
authority. It stores distinct linked physical-block, semantic-span, symbol,
source-owner/gate/tier, output-section, physical-storage-contribution,
verification-target, work-item, blocker, and hashed-evidence entities. Binary
Ninja remains binary authority. Data symbols, owner data gates, physical
storage, PE sections, and final-image acceptance never imply one another.
Unknown extents retain their start address and `extent_state=unknown` but omit
size and end; a fabricated one-byte extent is forbidden.

### 1. `function-order`

Start at `0x401000` and advance monotonically through retail `.text` ending at
`0x4cb9e8`. For every physical block, compile the reconstructed translation
unit with VC5SP3 and require its exact defined-function set and natural order:
no extra, missing, duplicate, unresolved alias, or reordered definition. Then
require an exact linked interval bounded by the preceding/succeeding retail
sentinels. The first block uses
`predecessor_section_boundary: {"section":".text","address":"0x401000"}`
instead of inventing a predecessor function.

Provider, compiler, COMDAT, padding, and data contributions inside `.text` must
retain their retail positions and classifications; padding/data are not fake
functions. Correct source owners, physical blocks, header layering, helper
placement, linkage, and include timing whenever order shows that existing
source shape is wrong. Never force order with `.inl`, pragma/linker tricks,
wrong-file helpers, fake wrappers, or duplicate bodies. New or disputed
owner/block/order conclusions continue to require the root ChatGPT Pro policy.

Stop at the first divergence. Work elsewhere is allowed only when recorded as
the required compile/link dependency of that cursor. If the cursor is genuinely
external-blocked after safe dependency work is exhausted, `progress next` may
permit only the earliest byte-unmatched function in the contiguous
order-accepted prefix; this fallback never promotes the phase.

### 2. `linked-byte-match`

After every order row is accepted, restart at `0x401000`. Traverse functions by
retail address while editing and accepting the complete source-shaped owner.
Each function needs one synchronized receipt proving exact extent and object
bytes, relocation types/symbol-provider identities/addends, its retail RVA in
the current map, all linked call/reference targets, and exact linked-image bytes
including resolved relocation fields. Accept only the current address or a
contiguous bundle beginning there. Owner `Reimplemented [S]` remains governed
by its complete unified-tracker owner/data gates. Entry-local byte evidence
does not imply owner `S` or advance the global prefix.

### 3. `final-validation`

Run one unrestricted synchronized build:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py verify final-build
python tools/recoil.py audit final-data --strict --json-out build/vc5-final/final_data_diff.json
python tools/recoil.py progress evidence import-final-data --report build/vc5-final/final_data_diff.json --expected-revision <revision> --dry-run
python tools/recoil.py audit final-repro --strict --output build/vc5-final/final_repro.json
python tools/recoil.py progress evidence import-final-repro --report build/vc5-final/final_repro.json --expected-revision <revision> --dry-run
python tools/recoil.py progress audit --scope pipeline --strict
```

Receipt imports record hash-bound observations only. Review normalized state
with `progress output-section show` and `progress storage show`; accept each
storage/section dimension explicitly with dry-run-first `progress accept
storage` and `progress accept section`. Final acceptance requires every order
and byte row, every required order target, every mandatory whole output
section, resource comparison, PE/data/import/provider/address validation, an
exact final-repro receipt, and candidate size/SHA-256 matching retail. `--no-pe-compare`,
`--no-resource-compare`, skipped required order checks, normalized comparison,
or a stale map/candidate makes the run diagnostic-only.

## Commands and Handoffs

Use:

```powershell
python tools/recoil.py progress status
python tools/recoil.py progress next --json
python tools/recoil.py progress audit --scope all --strict
```

Only the parent mutates the unified tracker, dry-run first and against the
reviewed revision, through narrow `progress accept`, `progress blocker`,
`progress owner`, `progress block`, `progress semantic`, `progress work`, or
`progress evidence` operations.
Receipts must name the retail reference hash, VC5SP3 profile, source/manifest/
object/map/candidate hashes, exact gates, command, scope, and conclusion; never
store a concrete `.devspace` dependency. Evidence generation/import never promotes
owner gates, tiers, provider classification, global prefixes, or `Model:`
metadata.

A source-worker/verifier handoff names the global phase and cursor, physical
block, complete owner, allowed files, first divergence, exact command, required
receipt, and exit gate. Reports include expected/actual definition counts,
extra/missing/reordered rows, linked seam/address/byte results, and the first
unresolved address. Ordinary owner, final-data/final-repro, functional, and
`messages.dll` queues are `deferred_by_pipeline_phase` unless required by the
current Recoil.exe cursor.
Final-data and final-repro are evidence producers, not work units, queues, or
peer schedulers; they never generate owner-action batches.

## Debt-Free Session Close

Before final response, fix every regression introduced by the session; remove
temporary probes and failed variants; promote conclusions and receipt hashes;
leave exactly one truthful cursor/blocker with an exact next command; update or
close structured work items; run touched checks and strict workspace/progress/
agent audits; and
wait for workers/Pro consumers. A source blocker is legitimate backlog only
when its address/range, evidence, failed command/result, accepted checkpoint,
and next command are durable. Parent-only `.devspace` cleanup is the last
workspace action. Do not finalize with stale work items/cursors, hidden failures,
false claims, unregistered evidence, or any other session-introduced debt.
