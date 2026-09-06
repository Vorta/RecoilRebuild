# Prepared-script index call-contract diagnosis

This is nonaccepting diagnosis of the later full-census divergence at
`zInterp_Context::LoadPreparedScriptIndex` (`0x4c5550`). It does not bypass
the earlier mission-constructor blocker or advance the serial scheduler.

## Direct retail and current compiler evidence

Current target-qualified Binary Ninja assembly has twelve calls, in order:
search-path creation, resolved open, header `fread`, header-failure `fclose`,
count `fread`, version/count-failure `fclose`, `realloc`, table `fread`,
table-failure `fclose`, `_stat`, stale-entry `fclose`, and `free`.
The three `fread` sites use the IAT value held in EDI. The header is eight bytes,
the format is magic `0x08971119` and version seven, and records are `0x80` bytes.
The loop compares stored time at entry offset `0x78` against `st_mtime`.

The generation-89 full-replay COD has ten calls. VC5 merges the first two
cleanup sites with a later failure path; this is an actual emitted call-count
and order difference, not an unidentified IAT or a verifier spelling issue.
The current source's short-circuit loop also lets the compiler branch directly
to stale-entry cleanup, whereas retail rejoins before testing freshness.

Failure behavior must not be modernized during reconstruction: retail frees
the allocated table after timestamp validation fails, but its earlier failed
table-read path closes the stream without freeing that allocation. The old BN
comment's blanket claim that every failure frees the table is too broad.

## Rejected diagnostics

- `zinterp-prepared-read-validation-order-01` separates table acquisition and
  timestamp validation with an `entriesRead` flag. Authored order remains
  42/42, but the body emits eleven calls and still displaces a cleanup site.
  The source experiment was reverted completely.
- `zinterp-global-optimization-diagnostic-01` changes only `/Og-` in a
  registered nonproduction profile. It emits twelve calls but loses retail's
  register-held `fread` calls and puts the final `fclose` after `free`, unlike
  retail. It is rejected as the resolution. Production flags are unchanged.
- `zinterp-freshness-loop-exit-order-01` moves the freshness test from the
  loop condition into an explicit structured break. Order remains 42/42,
  but the compiler still merges cleanup calls and bypasses the common
  post-loop freshness test. This source differential was also reverted.

No call contract, byte, owner, provider, data, or linked-image fact is accepted
by these diagnostics.

## Native context lifetime and header correction

Retail `0x4c0d20` is a C++ constructor with native VC5 list construction and
exception cleanup, not an ordinary method manually allocating a list head.
The byte at context `+0xb0` comes from allocator storage; it is not the first
character of the search path. The sentinel and size occupy `+0xb4` and `+0xb8`.
The destructor `0x4c0e50`, clear in `0x4c0f70`, insertion in `0x4c58e0`, and
traversal in `0x4c5a00` corroborate `std::list<zClass_NodePartial *>`.

Production now uses that native member and real nonvirtual constructor and
destructor. The existing three virtual dispatch slots are unchanged. The HUD
constructor call at `0x414ab0` passes prepared filename first and search path
second; both declarations and caller now reproduce that order. The old source
had reversed both sides, hiding the ABI mismatch from behavior alone.

Retail also clears context `+0x84..+0x8b` as one eight-byte record, matching
the header read by `0x4c5550`. A POD header and ordinary `memset` recover that
instruction sequence. VC5's `T()` POD spelling did not initialize its temporary
in the diagnostic and was rejected; an inline two-store clear did not recover
the sequence either. Neither experiment remains in production.

The governed `zinterp-native-header-memset-ob1-diagnostic-01` build preserves
the 42-body order and reproduces the constructor's 0x128-byte instruction
sequence outside relocation operands. Native destruction has the nine retail
call offsets `2d,40,50,60,70,7d,8a,bb,e7`. The existing `/Ob1` profile is selected
for this translation unit on that evidence. These observations do not accept
relocation identity, linked bytes, an owner tier, or the complete target.
The loader still emits ten calls, so its earlier blocker remains open.

Additional small source corrections reproduce retail's `and edi,eax` loop
accumulation (`RunString`, `0x4c1020`), positive-branch-first command getter
(`0x4c5510`), byte-width false return (`DefaultDispatchHook`, `0x4c58c0`), and
uninitialized output slot passed to `gwNodeGetUserData` in the scrolling action
(`0x4c59e0`). The two direct callers of `0x4c58c0` discard its result; retail
clears AL only, so its former full-EAX `int` return was incorrect. The native
`bool` declaration reproduces that ABI. The scrolling output slot is filled
by its callee, as in retail; no new initialization is emitted before the call.

Fresh `zinterp-dispatch-return-width-byte-feedback-01` reports zero unmasked
differences on fifteen of the forty-two functions. Other functions still fail,
and this diagnostic does not accept the authored-byte stage. The fresh scoped
generation-91 call check before the return-type correction passed 41/42,
with only `0x4c5550` divergent. HUD's scoped check found only the native
constructor identity awaiting fresh accepted order after manifest sync.

## Further direct byte reconstruction

The `zinterp-builtin-variable-switch-byte-feedback-01` diagnostic reaches
38/42 bodies with zero differences outside relocation fields. This is not
authored-byte acceptance: exact relocation semantics, linked presence and
identity, and the serial call-contract prerequisites remain required.

The recovered source includes bounded token access, live table-pointer
iteration, positive guard structure, native scalar-type switches, and the
retail text-line and macro-expansion loops. In particular:

- `ExpandMacroRefs` (`0x4c1250`) appends the trailing text with `strcat`.
  The conditional final `strncpy` at `0x4c1390..0x4c13a4` writes its local
  macro-name buffer, not the shared output buffer. The previous reconstruction
  attributed that destination incorrectly.
- `HandleBuiltinCommand` (`0x4c1c50`) uses the bounded current token for each
  keyword comparison. Its integer-variable branch calls `ParseIntToken` and
  stores the result; the float branch calls `ParseFloatToken` and stores it;
  the string branch logs `Can't modify string variables (yet!)`. The previous
  source omitted the integer assignment and put an integer-parser call in
  the string branch. Its invalid-operation path also contained a log call
  absent from that retail path. The reconstructed native switch now matches
  all 989 instruction bytes outside relocation operands.
- `ValidateArgsAndNodeType` (`0x4c5820`) returns `bool` in AL and uses an
  unsigned argument-count comparison. `RegisterScrollAlwaysNode`
  (`0x4c58e0`) likewise returns `bool`, and its final parameter is a boolean.
  The native list insertion and these widths reproduce both bodies outside
  relocation fields. Their two decorated manifest symbols were synchronized
  at transaction revision 5261, conservatively invalidating the prior order.

Retail `DispatchCoreCommand` (`0x4c20a0`) begins with a first-character switch,
with its compiler-generated target table at `0x4c53ec` and byte map at
`0x4c543c`. The source now uses a native switch, including the `I` no-op and
the shared `M`/`m` branch. No hand-authored jump tables were introduced.
Shared x/y/z operands and the two-element vertex-exclusion array `{0, 1}`
recover its 40-byte frame and entry initialization. Assembly at `0x4c4236`
passes the address of that array to `BuildBlendVertsFromConnectivity`;
the old source passed null. It also used a local display-instance pointer
where retail stores and reloads the existing global display-instance pointer.
The callee receives exclusion count zero, so restoring the array is an
argument/source-fidelity correction, not evidence of an observed runtime fix.

`zinterp-shared-command-operands-byte-feedback-01` retains the 38 matched
bodies; the four outstanding bodies are tokenization (`0x4c13c0`, three
register-byte differences), script-file execution (`0x4c1500`), core dispatch
(`0x4c20a0`), and index loading (`0x4c5550`). The core dispatcher still has
substantial differences; its matching entry does not qualify the whole body.
Some independent VC5 register allocation changed across source variants;
only the fresh whole-target diagnostic count is reported here.

Additional rejected loader variants moved magic handling into a native
switch, wrapped the open-stream path in a positive guard, or extended the
allocated entry-table lifetime across the magic guard. None reproduced the
twelve-call retail sequence. All three source experiments were reverted.

The subsequent file-restore/camera diagnostic reduces `RunScriptFile` to
13 unmasked differences by passing the restored `currentScriptFile` to
`fseek`, as retail does. Its remaining differences are register allocation.
The FOV arm now reads both float tokens before multiplying by the existing
double-precision degree conversion constant. This reproduces the retail FPU
sequence; the former arm multiplied each value immediately by a float literal.

## Native switch proof infrastructure

Fresh live order at revision 5262 passes all 42 bodies. The generation-92
call diagnostic then rejects the native core switch during exact-IAT lineage
extraction. WSI-20260906-006 records the infrastructure gap: complete local
table edges were retained only in the classification view, and the primary
byte-classifier guard includes two flag-preserving stack stores between CMP
and JA. Generic exact COFF switch recovery now proves either a direct indexed
table or a byte classifier, verifies all relevant bytes/relocations/ranges,
rejects guard bypass, and publishes complete successors to the ordinary CFG.
Unknown edge sets remain unresolved. Parameterized generic tests exercise
both forms with and without scheduled stores and retain rejection of table
addends, guard bypass, and flag-clobbering instructions.

Generation 93 gets past that CFG failure, exposing an obsolete interpreter
camera validation profile. It requires EDI for the comparison call even though
the candidate and retail both now use EBX, and separately pins historical
stack-byte patterns. WSI-20260906-007 records its removal together with the
dormant prepared-loader tail-fold projection. The latter reused candidate row
nine three times to turn ten physical calls into twelve logical rows. The
earlier raw-count guard already rejected that body, and still does; the
projection must not return. The removed family and constants are recoverable
from workspace history. Generation 94 uses ordinary exact physical-call
comparison without these interpreter-specific paths.

The complete compact proof kernel passes 133 tests after the change. This
tool result does not accept the unresolved loader, dispatcher bytes, or the
authored-byte stage.

The fresh generation-94 scoped call scan passes 41/42 bodies, including the
native dispatcher. Only the loader remains divergent. Doctor passes all seven
checks. Issues WSI-20260906-006 and -007 were resolved at workspace-issue
revisions 3942 and 3943 respectively; these resolutions accept tool repairs,
not reconstruction bytes.

## Indexed loader and advisory review

The retained loader now indexes `entries[entryIndex]` rather than advancing a
separate cursor alias. VC5 strength-reduces that indexed loop and restores the
EBX table base before the common post-loop freshness test, as retail does.
The older loop-exit observation above is superseded. This also recovers the
retail ESI/EDI/EBX/EBP allocation up through the first read, but the loader
still has ten calls and 337 unmasked differences.

ChatGPT Pro reviewed the source/header, normalized retail/candidate assembly,
audit, and compiler profiles in run `2026-09-06T06-46-51-607Z-chatgpt-call`.
All six uploads completed and transcript verification passed. Its advice
favored moving table rejection before timestamp validation and then testing
the association between stale cleanup and the open-failure return. Neither
test recovered the missing calls: `zinterp-prepared-pointer-validation-join-pro-01`
and `zinterp-prepared-open-continuation-pro-02` each emit ten calls, use EBP
instead of EDI for `fread`, and have 350 unmasked differences. Both experiments
were reverted to the indexed-loop model. Pro is advisory, not acceptance.
The second and third retail inline closing blocks span 34 bytes each; an
earlier prompt incorrectly described all three as 33-byte blocks.

The effective generated compiler command explicitly uses `/Ob1`, with no
`CL` or `_CL_` environment override present. The selected compiler directory
contains CL 11.00.7022, C1XX.DLL 11.00.7303, and C2.EXE 11.00.7303. File-version
inspection alone is not a live loaded-module trace. The historical `/Og-`
experiment used `/Ob0` before the native lifetime correction and is not a
single-flag differential against today's `/Ob1` production baseline.

## Additional interpreter corrections

`zinterp-cycle-texture-error-path-byte-feedback-01` reaches 39/42 bodies with
zero differences outside relocation fields. Tokenization is the additional
match: a direct newline branch that terminates the separator in either arm
recovers its remaining three register bytes without assembly. A local-bool
experiment in `RunScriptFile` adds byte-width handling absent from retail and
was reverted; that function retains its 13 register-allocation differences.

The dispatcher comparison also establishes:

- `CycleTextureSetOn` reports an error on a nonzero setter result, not zero.
  Retail callee `0x484250` returns zero on success and minus one for a null
  display instance. The old caller's condition was reversed. It also cached a
  node where retail reloads the context member across calls.
- Camera and world-fog getters receive uninitialized output variables filled
  by the callee. Extra pre-call zero stores were removed.
- Light and object rotation parse all three degree values first and convert
  them at the rotation call. The previous source converted each token
  immediately. The current shared operands reproduce their retail FPU
  sequences outside relocations.
- Several scalar, range, and world-coordinate commands reuse the shared
  x/y/z operands. Their spill/reload behavior differs from directly nesting
  the parser call in a setter argument.
- Retail's comparison lengths are not uniformly the literal length:
  `CountCameraNodes` uses 14, `LightSetSaturated` 19,
  `Object3DSetTextureWorldTexturesPerMeter` 37, `SEQSetPause` 12, and
  `SetPerspectiveInverseZTolerance` 20. Those values are restored directly.
- `FreeNode` dispatches on the found node without the added null guard. Its
  class deletion targets include retail-folded equivalent functions; friendly
  BN names alone do not distinguish those ICF identities.

The dispatcher remains byte-divergent. Its raw object extent includes native
switch tables, so that extent is not an executable-instruction-length claim.
No authored-byte group, owner tier, or playground image is accepted here.

## Native scrolling operands and current rechecks

The display-instance fields at offsets `0x24` and `0x28` are both floating-point
scroll rates, not a float plus an integer axis selector. Retail `0x4760d0`
stores the two argument words, `0x478fc0` passes the address of the first field
to `0x4791c0`, and that updater loads both with FLD/FCOMP and multiplies them by
the frame delta before updating texture coordinates. The existing model
instance view already represented these fields as float U/V rates.

`zDiPartial`, the model setter and clone, and `RegisterScrollAlwaysNode` now
use that native pair consistently. The interpreter no longer bit-casts the
second float through a union to satisfy an invented integer parameter. Layout
assertions still prove offsets `0x24` and `0x28`; no extent changed. The three
registered manifests were synchronized at revisions 5263, 5264, and 5266.
Fresh live order passes the 68-body model block at revision 5265 and the
42-body interpreter block at revision 5267. The subsequent generation-94
model call-contract diagnostic passes all 68 bodies. These are not byte-stage
acceptances.

The dispatcher now also returns immediately for `Object3DGetTranslate` with
no current node, matching retail, rather than logging invented zero
coordinates. With a node present it uses getter-filled shared coordinates and
reloads the node for the subsequent log. Shared color operands and explicit
terminal error returns in command families recover additional retail source
and control-flow structure without introducing helper calls.

The latest `zinterp-object-position-query-byte-feedback-01` diagnostic has
38/42 bodies with zero differences outside relocation fields. In addition to
the three persistent divergences, `OpenPreparedScriptStream` now differs in
register allocation and spilling even though its source was unchanged by
these edits. Earlier 39/42 diagnostics remain historical observations, not a
claim about the current translation unit. The raw loader still has ten calls
against twelve in retail, and the dispatcher remains byte-divergent.

## Object command pointer and polygon record corrections

All five current xrefs to `0x56c780` are in the dispatcher. Stores at
`0x4c3db5`, `0x4c3f39`, and `0x4c4122` copy the node user-data pointer, and
loads at `0x4c3f44` and `0x4c412c` supply that pointer to zDi setters after
parsing an argument. This is not integer argument storage. Source and the
registered data manifest now name it `g_zInterp_Object3DCommandDi` with type
`zDiPartial *`; the existing stable source anchor is retained. The name is
descriptive, not a recovered original spelling. The data target was
synchronized at revision 5268 without accepting a data gate.

Binary Ninja now has the pointer type and descriptive name at this address.
The dispatcher was explicitly reanalyzed, the reads/writes and downstream
calls inspected, and `Recoil.bndb` saved. The former integer interpretation
in the source docblock was removed. The four source commands now keep parsed
integer values in ordinary locals/arguments rather than writing this pointer
slot; the three material commands stage the pointer as retail does.

Retail polygon construction also disproves three source record offsets:
`pointCount` is at `0x28`, `uvCount` at `0xa4`, and `polygonMaterial` at
`0x1c4`. The source had those roles rotated. The existing BN sparse record
already has the correct count/material offsets; it needed no mutation.
The source record and its static assertions are corrected together, preserving
its `0x1d8` total size. UV and vertex handlers now retain an index, not an
element pointer, and reload the runtime record after each parsing call.

Both scroll-disable calls (`0x4c4009` and `0x4c40d8`) supply the context in
ECX and the node on the stack. There are no data references to `0x4c58c0`.
`DefaultDispatchHook` is therefore modeled as an ordinary context member,
not a static stdcall callback. Its body remains byte-identical outside
relocations and the native member declaration recovers both caller ECX moves.

`zinterp-scroll-disable-native-member-byte-feedback-01` has 39/42 bodies
matching outside relocation fields; the open-prepared-stream allocation is
again a match. The remaining bodies are the runner (13 register bytes), core
dispatcher, and prepared-index loader. This diagnostic is not byte acceptance.

`RunScriptFile` subsequently converges in
`zinterp-run-file-common-return-byte-feedback-01`: placing its return after
the parent-frame restore/clear alternatives recovers all 238 instruction
bytes outside relocation fields. Forty of the block's 42 bodies now match
that diagnostic dimension. The scoped S command case cleanup preserves those
results. An explicit local integer comparison result also reproduces the
retail small-polygon predicate's NEG/SBB/INC and stack store; a shared return
at the end of the LOD case recovers its retail FPU/store scheduling and the
following material/polygon handlers' register allocation. No raw assembly or
compiler-profile change was used.

A signed local count experiment in the prepared loader produces identical
diagnostic output and was reverted; it does not resolve the ten-versus-twelve
physical calls. The loader remains unresolved. The native scroll-disable
member manifest was synchronized at revision 5269. Its BN prototype and
unsupported callback comment were also corrected; both caller argument flows
were rechecked after reanalysis and the database saved.

## Current interpreter and HUD rechecks

Fresh live interpreter order passes all 42 identities at revision 5270.
`zinterp-object-command-di-data-01` reports all selected data rows matching
outside relocation fields, including the corrected pointer at `0x56c780`.
`call-contract-zinterp-native-object-di-g94-02` passes 41/42 bodies and rejects
only the loader's ten actual calls against twelve retail calls.

The full HUD recheck exposes WSI-20260906-008: the new generic COFF decoder
collapsed repeated switch destinations, while a case-map consumer correctly
required the ordered physical entries. Generation 95 retains table order and
multiplicity; CFG consumers derive unique successors separately. Four generic
repeated-case variants reproduce the defect before the fix. Focused evidence
tests pass 58/58, the complete proof kernel 137/137, and doctor 7/7.
`call-contract-hud-ordered-switch-map-g95-01` freshly passes all 406 HUD bodies,
including the former global interpreter constructor identity divergence.
The workspace issue is resolved at issue revision 3945. These scoped checks
do not replace whole-stage replay or closeout.

The advisory exchange at
`.devspace/runs/2026-09-06T07-50-32-392Z-chatgpt-call/transcript.md` successfully
uploaded eight attachments and passed transcript integrity verification. It
identified an overlooked diagnostic immediate in `Object3DAddChild`: retail
pushes `0x3f3`, not the source's `0x26f`. The independently checked immediate
is corrected. The baseline diagnostic preserves forty matching bodies and
reduces dispatcher unmasked differences from 3385 to 3383; it does not resolve
the one-byte instruction-extent difference.

The direct callee/xref review finds only one caller of `0x483f80`. Its exit
leaves the updated flags in EAX incidentally, and its caller does not consume
a return value. This is insufficient evidence for changing the current void
declaration or adding a scale setter solely to influence register allocation.

The `/Og-` investigation now retains `/Ob1`. Its first override omitted the
build-context defines, so that run is not a clean single-option comparison.
The corrected `zinterp-corrected-line-ogminus-ob1-context-diagnostic-02`
command retains every canonical define and include option and adds only
`/Og-` after `/O2`. All 42 bodies diverge, including the four-byte error
increment becoming a 26-byte framed body. The loader has twelve calls but
uses direct IAT reads and puts the table-failure close last rather than in
retail order. This strongly rejects that setting for production; the added
profile is diagnostic only and no production TU selects it.

An unchanged canonical repeat again reports forty matching bodies and the same
two residuals. Live process inspection observes CL `11.00.7022` loading
`VC5SP3/VC/BIN/c1xx.dll` `11.00.7303` and launching the adjacent `c2.exe`
`11.00.7303`. Both compiler components match the local ENU SP3 distribution
copies. This is an installation/provenance diagnostic, not reconstruction
acceptance or independent authentication of the distribution media.
