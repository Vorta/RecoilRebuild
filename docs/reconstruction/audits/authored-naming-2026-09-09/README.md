# Authored Naming Audit — 2026-09-09

This is a dated naming review of the authored Recoil.exe source and its
navigation/binding names. It is not a tracker export or a new acceptance
authority. Live reconstruction state remains in the progress tracker through
`tools/recoil.py`. The audit does not claim that every retained or inferred
identifier is the original spelling. The companion messages.dll is excluded.

## Scope And Dispositions

| Inventory | Count | Renamed | Retained | Deferred | Excluded |
|---|---:|---:|---:|---:|---:|
| Authored physical functions | 3,540 | 848 | 2,689 | 3 | 0 |
| Recorded logical function aliases | 61 | 0 | 57 | 0 | 4 provider members |
| Source function definitions | 4,114 | 768 | 3,320 | 26 | 0 |
| Source class/record/type names | 678 | 54 | 621 | 3 | 0 |

These inventories overlap; their counts must not be added to infer a retail
function count. Source definitions include inline helpers, compatibility
wrappers, and provider-facing methods without separate authored retail bodies.
Physical-function names include synchronization of existing source names that
previously differed from Binary Ninja labels. The implementation changes 579
distinct identifier spellings in 104 source files, including declarations,
definitions, imports, and callers.

- [Physical function dispositions](physical-functions.tsv) cover every authored
  or authored-lifecycle physical row in the public tracker census used for this
  review.
- [Logical function dispositions](logical-functions.tsv) preserve the recorded
  folded identities and distinguish provider members.
- [Source function dispositions](source-functions.tsv) cover the lexical source
  definition inventory, including definitions without standalone retail bodies.
- [Source type dispositions](source-types.tsv) distinguish class/interface
  shapes from records and other declarations.
- [Identifier changes](identifier-changes.tsv) give the spelling substitutions
  and their naming rationale.

The census is complete for those inputs. Detailed retail role investigation is
concentrated on ambiguous or misleading names; a retained-name disposition
does not imply a new behavioral reconstruction of its entire function.

## Exact Retail Spelling

| Retail string | Named function | Evidence interpretation |
|---|---|---|
| `0x4dda0c` | `gwNodeNew` at `0x4478c0` | Diagnostic and containing allocator agree. |
| `0x4dda80` | `gwNodeSetActive` at `0x447c60` | Existing leaf spelling retained. |
| `0x4ddbcc` | `gwNodeUpdate` at `0x448cc0` | Existing leaf spelling retained. |
| `0x4ddc38` | `gwNodeBuildNodeToAncestorMatrix` at `0x449480` | Replaces the shortened placeholder leaf. |
| `0x4de064` | `_gwListDeleteANode` at `0x44f1d0` | Leading underscore is directly evidenced. |
| `0x4e050c`, `0x4e056c` | `convexify` at `0x46c760` | Lowercase retail spelling restored. |
| `0x4e1700` | `check_colinearity` at `0x482b40` | Caller `0x483650` invokes it at `0x4836ec` before reporting the reduced vertex count. |
| `0x4e2140` | `zRdrRead` at `0x48d080` | Existing source spelling retained; Binary Ninja synchronized. |

Exact leaf evidence establishes neither a namespace nor a class. Existing
source scopes remain reconstructed unless independently evidenced. The string
at `0x4ddb40` recommends `gwSequenceAddChild`; it does not establish that as
the name of the containing generic `AddChild` function. Underscores are not
categorically forbidden, as the two exact spellings above demonstrate.

## Inferred Role Corrections

`CZInterp` replaces `zInterp_Context`, following the documented GameZ class
default for the already reconstructed interpreter class. `CRecoilInterp`
replaces its application-specific derived class. Neither spelling is claimed
to survive in retail. The derived implementation remains in `hud.cpp`.

At `0x4c1020`, `RunStream(FILE*, int)` reads and dispatches multiple lines. At
`0x4c1090`, `RunLine(char*)` processes one line. The former `RunString` and
`RunStream` labels obscured that distinction. `HandleScrollDisable` names the
caller role of the old `DefaultDispatchHook`; the existing retail behavior is
preserved, including its questionable null output argument.

`CHudRadioGroupWidget` and `CHudRadioButtonWidget` replace the offset-derived
`HudUiZrdWidgetEx17C` names. The `Radio` ZRD child array, selected index, and
mutually exclusive item selection support those roles. `ShutdownShieldWidget`
and `ShutdownItems` replace misleading wrapper names ending in `_Stub` because
the wrappers call their contained widgets' shutdown methods.

Other substitutions retain existing descriptive roles and use the documented
family conventions. Numeric status ranges retain their endpoints with `To`,
for example `AppendConnectStatus301ETo3021`. Such names remain inferred.

## Deferred Class Spellings

Three proposed class renames were withdrawn as complete identifier changes:

| Retained | Proposed | Reason |
|---|---|---|
| `zFMV_Action` | `CZFMVAction` | Existing folded `End` and destructor bindings embed the current class spelling. |
| `zFMV_ActionBlur` | `CZFMVActionBlur` | Its destructor shares a governed folded-alias group. |
| `RecoilApp_MissionFmvState` | `CRecoilAppMissionFmvState` | `SetMissionId` participates in a governed folded-alias group. |

This preserves the existing alias source bindings without reopening their
proofs in a names-only change. It does not establish those retained spellings
as original. The audit also retains ambiguous source labels where another
cosmetic spelling would imply unsupported precision.

## Preservation And Validation

All changed source files preserve their line counts and literal tokens. No
layout, control flow, ABI, translation-unit placement, source filename, runtime
string, provider API, or stable source/retail identity is intentionally changed.
Source filename mentions in comments are preserved separately from identifier
renames. New tracker support updates existing display-name fields with exact
old-name and revision guards; it cannot change ownership, gates, tiers, paths,
or acceptance facts.

The final native diagnostic compiles all 87 configured translation units and
finds unchanged bodies for all 1,399 previously matched projections. This is a
regression diagnostic, not retail acceptance. Fresh governed order and complete
function-match results are recorded in the validation closeout below.

The naming pass exposed a raw-offset guard false positive in compact allocation
calls. The guard now distinguishes arithmetic inside call arguments from an
offset applied to a pointer, while overlapping scans retain nested raw casts.
The generic regression and all 250 proof-kernel tests pass. Workspace issue
`WSI-20260909-023` records that defect and its resolution.

The order resolver also validated complete contracts for disjoint intervals
while searching for an unconfigured current target. A necessary interval-overlap
prefilter now avoids that work; every overlapping candidate still receives all
contract checks, and ambiguous overlapping candidates still block. The generic
regression and complete kernel pass. Workspace issue `WSI-20260909-024` records
this performance defect. A fresh public next-task query with the prefilter took
13.34 seconds and returned the exact current target and object override.

Order replay also exposed four stale lifecycle labels in the `zsnd_create`
and `zsnd_grp` manifests. Their ordinary methods were already classified as
authored in the tracker before this naming pass. The four manifest classes and
order roles now agree with those existing facts; no tracker classification or
source behavior changed. The reviewed two-target synchronization invalidated
no additional blocks. Workspace issue `WSI-20260909-025` records the mismatch.

The first complete match run exposed old source spellings embedded in reviewed
relocation contexts. `progress relocation-target refresh-source-names` now
provides a narrow reviewed refresh for this case. It re-derives each current
source snapshot, permits only its object-symbol spelling to change, and checks
the complete target, retail, provider, and exception contexts. Exact source-name
and occurrence guards preserve the binding population; application reads back
the committed revision and every replacement. The applied refresh covers twenty
source spellings in 182 target bindings, five native EH bindings, and six
reviewed exceptions. Historical reasons, target names, identities, extents,
ownership, exception policy, and acceptance facts remain unchanged. Workspace
issue `WSI-20260909-026` records the missing refresh procedure.

ChatGPT Pro reviewed all 149 comment-normalized diff hunks across twelve loader
context files and renewed advisory instruction-match eligibility. The review
confirms the same ten compiler register-allocation residuals and no concrete
credible source-faithful byte-match option left in the supplied engineering
record. The final folded-class deferrals do not change that loader context.
Pro's approval is separate from the fresh machine proof.

Review artifacts:

- [Prompt](../../../../.devspace/runs/2026-09-09T18-35-26-340Z-chatgpt-call/prompt.md)
- [Answer](../../../../.devspace/runs/2026-09-09T18-35-26-340Z-chatgpt-call/assistant.md)
- [Transcript](../../../../.devspace/runs/2026-09-09T18-35-26-340Z-chatgpt-call/transcript.md)
- [Receipt](../../../../.devspace/runs/2026-09-09T18-35-26-340Z-chatgpt-call/receipt.json)

## Validation Closeout

All 53 fresh serial order acceptance operations passed, covering the 56 blocks
invalidated by the naming/binding synchronization. The final order acceptance
committed at revision 6021. A subsequent public `progress next --json` returned
the authored-call-contract stage, restoring the pre-change stage boundary.
The pass did not advance call-contract or byte stages.

- [Order closeout](../../../../build/diagnostics/authored-naming-20260909-01/order-closeout.json)
- [Next task after order](../../../../build/diagnostics/authored-naming-next-after-order.json)

The complete fresh function-match refresh checked 3,292 groups and committed at
revision 6023. It preserved the exact prior set of 937 byte matches and one
instruction match: no losses, downgrades, or compensating additions. The
remaining 2,354 groups are unmatched. The report's whole-census `passed: false`
reflects that unfinished reconstruction, not a naming regression.

`CZInterp::LoadPreparedScriptIndex` at `0x4c5550` remains instruction-matched:
496 bytes, eleven exact relocation semantics, ten register-encoding residuals,
and 170 paired instructions. Both object and normalized linked instruction
proofs pass; the renewed Pro review is current. This is not exact byte equality
or a final-image acceptance claim.

- [Fresh function-match report](../../../../build/live-validation/authored-naming-match-20260909-02/function-match-report.json)
- [Exact previous-set comparison](../../../../build/diagnostics/authored-naming-20260909-01/match-closeout.json)
- [Final proof-kernel run: 250 passed](../../../../build/diagnostics/authored-naming-kernel-complete.log)

The final workspace doctor passed all seven gates: agent surface, issue ledger,
progress tracker, live validation surface, source policy, function-match
annotations, and serial pipeline reachability. `git diff --check` also passed.
The final source invariant check confirms unchanged line counts and literal
tokens in all 104 changed source files. Binary Ninja names and propagated types
were verified and the database saved; no plugin change or restart is required.

- [Workspace doctor](../../../../build/diagnostics/authored-naming-doctor-complete.log)
- [Final source invariants](../../../../build/diagnostics/authored-naming-20260909-01/source-invariants-final.json)
- [Binary Ninja verification](../../../../build/diagnostics/authored-naming-20260909-01/bn-final-summary.json)
