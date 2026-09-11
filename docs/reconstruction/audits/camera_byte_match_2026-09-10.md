# Camera byte reconstruction — 2026-09-10

The active authored-byte cursor is `Player::UpdateChaseCameraFromInput`
(`0x405040`). This audit records evidence and experiments; it does not accept
bytes, source ownership, storage, or a stage.

## Accepted predecessor

`Player::TickActiveCameraState` (`0x404e90`) passed a fresh complete function
match classification and live authored-byte acceptance. The successful source
uses the reviewed shared vector-direction primitive and an empty, out-of-line
camera-state-5 handler. The latter is a provisional logical alias of retail
`0x4076f0`; its original spelling, original TU, and physical ICF winner remain
unknown. Exact linked placement is a later gate.

## Chase-camera semantic corrections

Retail skips elevation writes when there is no relevant input, compares
absolute vertical speed against double `11.0`, and does not initialize the
mouse snapshot or fully written output vectors to zero. Its positive elevation
limit is the lesser of the configured swing and base clearance, whereas its
negative limit uses the original swing. Its heading dot uses the local steering
vector copied before the interpolation call. The yaw-free branch copies a full
direction vector; the yaw branch writes only the needed X/Z components. The
two Track checks reread the current modal pointer chain; the final Sub check
uses the pointer captured at entry.

At `0x4052c8`, the epsilon operand is float `-0.0001`; the numerator at
`0x4052d6` is float **-1**, not +1. The existing negative sum expression is
mathematically consistent with these retail values. Advice premised on a
positive numerator does not justify changing that sign.

## Camera parameter aggregate

Retail `0x4051f5` copies state bytes `[0x52c,0x538)` as a three-component
aggregate. Independently, initialization at `[0x4211fc,0x42122e)` copies common
data `[0x350,0x35c)` into that exact state vector. Source now represents both
as `zVec3 cameraBackOffset`, with X=side, Y=height, and Z=distance. The name is
inferred. Native assignment replaces the initializer's three scalar copies;
other readers and configuration writes use vector components. No overlay,
raw-offset cast, size change, or original-name claim is involved. A fresh
registered Player authored-order check passed all 145 selected functions under
`build/live-validation/byte-chase-player-vector-order-01`.

## Math experiments still under review

All builds use canonical VC5SP3 11.00.7022 with
`/TP /W3 /MD /G5 /O2 /Ob1 /GX /Gr /Zp4 /FAcs`.

* Pointer punning and a split integer addition did not recover the exponential
  approximation's local store/reload sequence.
* A pure C++ inline exponential helper was inlined but folded the two
  multiplications. An output-lvalue macro preserves both multiplications,
  yet still omits the retail integer store/reload.
* Explicit double yaw, reversed sine/cosine declaration order, and a pure C++
  inline two-output helper all omit the retail range check and `fsincos` path.
* Explicit vector pointers, float intermediates, and pure C++ inline XZ
  magnitude/dot helpers still omit the retail pointer captures and float
  store/reloads.

The XZ magnitude motif appears at `0x403660`, `0x40368e`, `0x40521c`,
`0x405a76`, `0x474d43`, `0x474db8`, `0x474e2d`, and `0x474e7b`.
The independent `zMathVec3RotateX` body at `0x474ec0` contains the same
double-magnitude check and fixed-register combined sine/cosine island.
The first-person camera also contains the same exponential integer
store/reload/add/float-store sequence. These observations support investigating
shared primitives; they do not by themselves authorize additional assembly.

Fresh experiment roots are `build/live-validation/byte-chase-405040-vNN`;
source and COFF assembly diagnostics are
`build/diagnostics/stages-byte-chase-vNN.json`. Version 14 also captures the
complete math header. The first Pro model review is recorded under
`.devspace/runs/2026-09-10T01-44-44-950Z-chatgpt-call` and explicitly required
more C++ experiments before any new raw-math exception.

## Relocation work

Complete authenticated BN reference lists and data declarations are recorded in
`build/diagnostics/stages-byte-chase-retail-constants.json`. The camera literal
pool contains four-byte floats and eight-byte doubles. Original temporary
symbol names are unknown. Governed physical-target exceptions require exact
retail contents, extents, operand sites, and local static read-only VC5 data
witnesses. Descriptive tracker names do not introduce production globals or
accept storage/linkage gates. The `_ftol` dependency uses the native MSVCRT
import-member proof, retaining the thunk's unresolved ownership.

## Follow-up primitive review and decoder correction

Pro's review in `.devspace/runs/2026-09-10T02-07-21-076Z-chatgpt-call`
approved the one-consumer XZ length and dot primitives and the four minimal
integer bridges for the exponential approximation. Version 15 applies those
regions, retaining compiler-generated arithmetic, conversions and control flow.
The trig primitive remained blocked pending an explicit C++ range/precision
experiment. Version 16 performs that experiment with the exact retail double
`9.22e18`: VC5 removes the float rounding stores on both arms and emits separate
`fsin`/`fcos` operations. Its 1552-byte extent equals retail by coincidence;
the frame is `0xa4` rather than `0x88`, with substantial remaining differences.
Three additional BN-classified trig consumers use the same exact threshold and
fixed-register `fsincos` island, with caller-dependent fallback stores. Their
evidence and the compiler result were sent for follow-up Pro review.

The live version-14 check exposed WSI-20260910-003: the retail decoder required
nonempty padding between the last RET and its trailing switch tables. The
HUD resolution selector at `0x40cd30` legitimately places its first table
immediately after RET. The correction permits that exact adjacency while
retaining the unique return boundary, exact bounds, decoded target boundaries,
and complete trailing-island proof. Generic adjacent remapped/direct table
coverage and all 250 proof-kernel cases pass. A fresh version-16 whole-program
build and authored-byte comparison passes that decoding step and reports the
camera's actual object-body mismatch. No byte acceptance was inferred.

Doctor passes its first five infrastructure gates; its annotation gate requires
a fresh match refresh after the verifier dependency changed. The stale prior
annotations must be synchronized by live proof, not manually asserted current.

Pro follow-up `.devspace/runs/2026-09-10T02-35-24-409Z-chatgpt-call`
approves only the camera normal-arm trig island [0x405390,0x40539f).
The compiler evidence resolves the earlier float-output precision objection:
VC5 scalar-replaces inlined C++ fallback outputs, while explicit asm stores
force rounding. The exact helper spelling and historical header stay inferred.
Version 19 applies that approved inline helper; no surrounding code is raw.

The inline trig helper now emits the exact retail normal/fallback/rotation
operation sequence after passing the player yaw directly (v20). Restricting
heading temporaries to their logical scope reduces the frame from 0x9c to 0x90
(v21), still above retail 0x88. A broader position-calculation scope (v22) did
not reduce it and changed FPU scheduling; that experiment was reverted.
Explicit double comparison operands are being tested against the remaining
retail elevation-clamp operations. Diagnostic operation alignment is not an
instruction-match proof and does not authorize register masking.

Version 25 recovers both clamp comparisons with double-valued upper/lower
limits and float storage of the maximum swing, preserving the retail
load/compare/status/branch sequence. Native copying of the returned angle
vector (v26) recovers retail component-load order. The extra frame extent and
remaining interpolation FPU scheduling are still ordinary C++ recovery work.

The public relocation audit at revision 6092 derives all 56 expectations with
no unresolved identities. Reordering the initial scalar declarations to
max-yaw-rate, camera-zone, inverse-range recovers retail read order (v31).
Diagnostic checks compare ordinary symbols, reviewed symbol regexes and
static literal contents in sequence; they do not substitute for complete
relocation type/addend, physical-reader or linked-body proofs.

Version 35 expresses the already reviewed three-instruction exponential
bridge in an inline float-returning helper, retaining C++ expression, scaled
conversion, integer store and float return. All four expansions keep both
retail multiplications. The caller now emits the retail heading FSUBP without
the extra post-conversion FPU pop that remained with the output macro.
Native vector reuse (v34) shrank the frame to 0x84 but added alias-related
reloads and was rejected; distinct vector scopes are being tested instead.

A later signature investigation finds that all seven retail callers of
0x474d10 supply an EAX-computed result buffer and consume returned EAX via
native three-component copies. The callee computes a local aggregate before
copying it to [EBP+8] and returning that buffer with RET4. Existing canonical
VC5 output for the by-value Subtract helper independently confirms that VC5
fastcall vector returns preserve ECX/EDX inputs and use a stack result pointer.
The explicit-output-pointer versus by-value interface correction is under
required Pro review before acceptance across the affected TUs. The complete
BN callee/xref/caller evidence is in
`build/diagnostics/stages-byte-chase-angles-abi-evidence.json`.


Pro review `.devspace/runs/2026-09-10T03-15-34-476Z-chatgpt-call`
approves the native zVec3 value-return correction, retaining the two existing
const-pointer inputs, owner, source placement and inferred spelling. The
pointer-versus-reference spelling of those inputs remains unresolved. The
implementation, declaration, all seven callers and the registered decorated
selector are synchronized. Fresh VC5 output confirms the predicted symbol,
ECX/EDX inputs, stack hidden result pointer, EAX result and RET4. The callee
arithmetic body remains unmatched. Math authored order passes all 68 bodies;
the fresh 405040 relocation audit resolves all 56 references with the reviewed
signature. Camera v39 recovers retail aggregate-return argument setup naturally.
Remaining frame and FPU scheduling differences are not instruction-match eligible.


The remaining C++ experiments retain distinct diagnostics rather than match
claims. V41's explicit distance complement/product removes that interpolation
schedule difference. V44 reuses the camera adjustment scalar, as supported by
retail's shared yaw/distance slot, and reaches the retail 0x88 frame. V47 reuses
the local steering/focus vector, matching every retail vector displacement.
V53 uses a double-valued vertical blend complement initialized by the float
subtraction, recovering the complete retail x87 operation sequence with the
same float literal reference. The residual body differences are scalar and
saved-pointer stack slot displacements. Stack reassignment is not eligible for
the register-only instruction alternative. No 405040 match is asserted.

Negative diagnostics: moving final vectors to a new block shrinks the frame
and changes scheduling (v45); inline value-return versions of the existing
XZ islands add FPU exchanges without fixing slots (v46, reverted); reordering
owner-pointer declarations changes the entry operations and global read order
(v48, reverted); local identifier spellings do not change code (v50); reusing
the yaw-rate scalar for speed moves vector slots incorrectly (v51, reverted).
The four affected non-HUD order checks pass: math 68, class 45, light 17,
effect 75. Full live function classification is running against v53.


Full refresh06 (revision6092->6093) classifies937 exact matches and2355 unmatched.
It loses four prior exact bodies (4026d0,404d90,404dd0,475910), gains42b520,
and removes4c5550's stale instruction annotation. The loader still passes the
same170-instruction paired dataflow proof with the same ten register residuals;
its source/compiler review context must be renewed after header work stabilizes.
No removed annotation was manually restored. Existing source declarations can
affect other VC5 bodies, so local camera diagnostics do not prove preservation.
Both HUD center getters recover exact retail object bytes by spelling the
half-dimension first in the addition. AI4026d0 recovers the entire relocation-
masked object after swapping the two rotation-z terms and using compound
x/z scaling; complete relocation/linked proof is pending. Quaternion475910
still differs in12 nonrelocation bytes: paired product loads/multiplies inx/y/z.
Five grouping/operand/precision/intermediate diagnostics do not fix it; its
original production expression was restored. No new asm is proposed.

Pro `.devspace/runs/2026-09-10T03-42-39-513Z-chatgpt-call` identifies stable
scalar/pointer color classes in the38 camera stack residuals and recommends
bounded qualifier, actual-read-order, local-reference, SinCos-output-reference,
and distinct outer-vector experiments. It does not authorize stack relaxation.
The current reused focus vector is a storage model, not a proved historical
conceptual variable; prefer distinct source roles if VC5 naturally coalesces
them, otherwise use a neutral inferred scratch name. V54 pointer top-level
const removal has no code effect and is restored. V55 tests cached scalar
lexical declaration order against actual retail global read order.


All proposed declaration distinctions are now measured: local common/player
references are byte-neutral (v56/v57), SinCos output references are byte-neutral
(v58), and the textual macro delays the two output-address LEAs (v59, rejected).
Distinct outer-scope steering/focus vectors enlarge the frame to0x94 (v60,
rejected). V53's source shape is restored, with the reused vector renamed
cameraScratch to avoid implying recovered historical conceptual identity.
The scalar-read-order variant v55 swaps the first two global identities and is
rejected. Six quaternion regression variants remain negative, including local
references; its baseline expression remains production pending further work.


V61 resolves all38 residual stack displacements through a source distinction:
keep the cached cameraZoneInvRange constant and derive a separate constant
verticalCameraZoneInvRange for the elevation input. Retail reuses their physical
slot, but the front end now sees their distinct lifetimes, rotating the three
scalar colors and the two pointer spills into exactly the retail homes. Every
instruction and displacement agrees in the diagnostic; all56 references retain
their expected sequence. The later register-keyword probe v62 was already
launched when this result arrived and is discarded; v61 is restored. Complete
fresh governed object/relocation/linked acceptance remains mandatory.

The first v61 live attempt stopped before building because FastExp/SinCos
docblocks lacked the source-provenance guard's recognized helper evidence.
The existing retail expansion evidence is now stated explicitly, preserving
comment line counts; the guard reports zero violations. A separate fresh call
check exposed WSI-20260910-004: the v2 reviewed provisional camera alias was
incorrectly required in the older five-member authored-order registration.
The call verifier now preserves that exact order membership while validating
the additional provisional call identity through its current independent review.
All member identity, owner, evidence, target synchronization and extra-row
checks remain required. No original definition position is accepted for the
extension. The focused rejection checks and all 250 kernel cases pass.
Verifier/expected-fact generations advance together; a fresh full call replay
is running. Doctor reaches stale function-match annotations, which require
fresh classification under the new generation before acceptance is current.

Loader Pro review `.devspace/runs/2026-09-10T04-03-40-338Z-chatgpt-call`
explicitly renews compiler-only register fallback eligibility for the same ten
residuals after the angle-helper header correction. The reviewed mutation is
recorded at revision6094; this is eligibility, not fresh machine acceptance.

Replay05 completes the full 3292-body census but stops serial acceptance at
157 passing bodies in the first slice (revision6095). Its nine divergences are
seven callers of the native aggregate-return angle helper whose tracker target
registration still named the old signature, and two HUD literal adapters that
incorrectly fixed candidate pool offsets. The math registration is synchronized
at revision6096, conservatively invalidating its order proof; fresh order
acceptance is required. WSI-20260910-005 tracks the literal-adapter correction:
retain exact compiler-local metadata, numeric contents, unique selection and
the complete caller relocation checks, while removing candidate section ordinal
and pool-offset assumptions. Both generation coordinates advance. All 250 kernel
cases pass, including malformed literal rejection and placement independence.

Quaternion Pro follow-up is recorded under
`.devspace/runs/2026-09-10T04-20-57-428Z-chatgpt-call`. It suggests per-component
scalar captures without changing retail sequential alias behavior, and expressly
does not authorize an instruction fallback. Fresh variants07 and08 (reused float
and distinct const float W captures) shrink the body to176 and alter its DAG;
variant09 (distinct double W captures) retains192 bytes but changes many x87
operations. All are rejected and the baseline expression is restored. The one
indexed retail caller at45b03f uses separate input/output buffers, but this does
not authorize removing the body's alias behavior. No quaternion annotation is
restored without a fresh exact proof.

Math order is freshly accepted at revision6097 (all68 bodies). The corrected
first call slice and HUD slice [40e140,415b40] each pass all160 bodies with zero
divergences in fresh roots `call-angle-signature-first-slice-02` and
`call-hud-literals-slice-02`. WSI-20260910-004 and005 are resolved through the
issue ledger at4141 and4142. Replay06 is now running the full census under
verifier158/expected100; no current stage completion is claimed before its
required fresh closeout. Doctor still requires a fresh match refresh to renew
annotations after the verifier changes.

Replay06 passes all3292 bodies across21 original slices with zero divergences
and records revisions6097->6118 (semantic/evidence6115). The returned mandatory
fresh closeout is running in `call-contract-closeout/all-r6119`, with eight
read-only verifier subprocesses and one subsequent canonical whole-program
compile/resource/link diagnostic. No byte or deployment acceptance is inferred
from replay or that diagnostic.

Closeout02 passes the fresh isolated scan of all3292 bodies/21 slices and its
complete canonical compile/resource/link. The serial commit records revision6119
(semantic/evidence6116). Authored call contracts are current. Play-test deployment,
linked-order, byte and final-image acceptance remain explicitly suppressed in
the linkability diagnostic. Full live match refresh07 is now running in its
fresh root to reclassify current source and renew annotation mirrors before
serial authored-byte acceptance resumes.

Refresh07 completed at revision6120 with937 exact byte matches andone reviewed
instruction match. The first fresh serial byte recheck exposed three earlier
functions whose focused compiler results did not hold in the canonical context:
AI steering4026d0 and HUD center getters404d90/404dd0. Diagnostic compiles now
clone the emitted canonical response without changing optimization flags,
defines or include order. Moving the AI offset-distance declaration after the
three vector declarations restores all496 non-relocation bytes; expressing each
getter as a conditional return restores all64 bytes. Reordered products,
compound assignments, double casts and scalar captures were rejected. Fresh
complete refresh08 independently proves these fixes and records revision6121:
940 exact byte matches, one reviewed instruction match,2351 unmatched functions.
The live tool restores allthree annotations. The call-contract stage remains
current; no byte-stage completion is claimed.

The remaining405040 failure is physical literal identity, despite exact1552
non-relocation bytes and56 relocation fields. The current giant HUD translation
unit pools camera constants with live laterHUD functions that read distinct
retail objects. The verifier correctly rejects this population conflict.
The first13 camera constants span4ccaa0..4ccadc;4ccadc is float3.0, correcting
an erroneous0.001 label in the first Pro prompt. The correction was explicitly
submitted and acknowledged before further model review.

Pro storage review and numeric correction are recorded respectively in
`.devspace/runs/2026-09-10T05-47-37-484Z-chatgpt-call` and
`.devspace/runs/2026-09-10T05-56-52-591Z-chatgpt-call`. Pro supports gathering
physical-boundary evidence and a shadow split, with no permanent move yet.
Fresh indexed xrefs confirm the camera reader partition and no camera reference
to either survivinghud.cpp orplayer.cpp filename string. Read-only assembly
covers the complete neighboring function census404e60..406890. The last camera
function returns at40688d before the next dialog handler406890. The404e80
legacy error no-op's original winning provider remains unresolved.

Controlled VC5 experiments under canonical optimization and linker pooling
options show five ordinary literals shared bytwo functions inoneTU, separate
copies across twoTUs, and also separate named namespace static-const objects
withinoneTU. Therefore named storage remains a real alternative. The candidate
camera temporaries are ordinary.rdata, notCOMDAT. A diagnostic-only split of
the unchanged existing Player camera namespace and HUD remainder compiles both
TUs; the camera's1552 non-relocation bytes stay exact and all extra liveHUD
literal readers disappear. Its first13 constants naturally reproduce retail
relative offsets, order, widths and contents. Three extra1.0 camera readers
remain in currently unmatched normalization implementations; retail usesfld1
on those paths. No productionfile, owner, boundary, provider, storage identity
or target has been moved or reclassified. The second Pro review receives these
facts, controlTU reader sets and the remaining named-storage alternative.

The second Pro review at
`.devspace/runs/2026-09-10T06-09-51-722Z-chatgpt-call` supports extracting exactly
the Player camera definitions404e90..406890 into an inferred implementation TU.
It keeps the semantic owners and logical identities, leaves404e80 and the MFC
remainder inHUD, and explicitly leaves the historical camera filename unknown.
The shared inline-math literal order and exact unchanged shadow body/pool prefix
make a separate TU stronger than an invented named-constant family. Doctor08
passes every infrastructure gate after the live refresh.

WSI-20260910-006 (issue ledger4143) records the missing guarded partial-file
source-path maintenance operation. Whole-prefix relocation requires the old
source file to be absent; owner replacement forbids retained-owner path edits;
physical-block replacement preserves prior semantic paths. A narrowly scoped
`progress source-path extract` route is being implemented and tested before
production extraction. It preserves the retail block grid and owner identities,
guards complete current records, retracts only unresolved broad filename
inference, and invalidates dependent evidence. No reconstruction tracker
mutation or production camera-file extraction has yet been made.

Subsequent execution completed the guarded source extraction at revision6124,
after target synchronization and the explicit state5 source trace repair.
The production camera definitions now reside in `src/Battlesport/camera.cpp`;
the original filename remains unresolved, and the existing semantic owners and
retail physical block grid are preserved. The registered order diagnostic and
fresh direct acceptance pass all404 rows at revision6125. The latter explicitly
names the aggregate object target because no original filename is accepted.

The extraction conservatively revokes affected owner source/linkage and call
evidence. Call replay currently stops before candidate proof: the reviewed
winner-unknown ICF group at4076f0 requires accepted source/linkage gates for its
camera state5 logical alias owner. Those gates have not been reinstated. This
supersedes the earlier statement that call-contract completion was current;
the earlier successful closeout remains historical evidence only.

WSI-20260910-007 records two scalar-mutation bookkeeping defects exposed by
the fresh camera owner-context build: thirteen primary data relationships lack
their initial X-tier entries, and rebinding existing compiler literals loses
creation-time owner context. The mutation implementation now initializes unknown
entry records and rederives exact existing primary-owner bindings. Its focused
and full250-case proof-kernel runs pass. Historical entry repair is prepared
but not yet applied. Public dry-run/review/apply operations refreshed all13
camera scalar exception contexts through revision6151; normalized fields remain
identical except for current source registration IDs. This does not accept
those data or owner gates.

The earlier Pro review at06:52 allowed the unchanged direction primitive at
405650,405870 and406110, and those scoped consumers were implemented. The review
withheld a proposed vector-add primitive pending two actual inline C++ helper
experiments. Both have since failed to reproduce the retail scheduling, but no
vector-add assembly or allowlist entry has been added. The subsequent07:05 Pro
call was interrupted without a captured answer; it grants no new decision.

The user's subsequent instruction requires a new independent Pro thread for
every new raw-assembly case. The review inventory contains nine entries across
five functions: direction at404e90; direction, XZ length, XZ dot, integer
FastExp bridge and SinCos at405040; and direction at405650,405870 and406110.
Each receives its own new thread with complete current source, exact retail
assembly, governed compiler context and failed C++ variants. Prior approvals
do not substitute for these fresh reviews. Results remain pending here until
captured and evaluated.

A fresh current camera compile in `camera-independent-raw-review-01` reproduces
all non-relocation body bytes for404e90 (432),405040 (1552), and405870 (304).
The405650 and406110 enclosing bodies still diverge. These are diagnostic
comparisons only: they do not establish relocation identity, linked body proof,
owner acceptance or stage completion, and no match annotation is inferred.

The first independent review, for404e90 direction, is captured under
`.devspace/runs/2026-09-10T07-49-56-778Z-chatgpt-call`. Submission is confirmed;
the receipt verifies GPT-5.6 Sol with Pro power after Latest offered only Extra
High. Pro withholds approval pending an exact-order C++ normalization probe in
the current extracted TU. It otherwise supports the implementation's behavior
and strongly favors an original inlined assembly primitive, while distinguishing
that from unproved macro syntax, identifier and header ownership.

Fresh current-TU experiments in `camera-independent-direction-cpp-01` address
that gap: float deltas/float inverse produces464 bytes; float deltas/double
inverse and double deltas/double inverse each produce480, versus retail432.
All use the retail squared-length order (z*z+y*y)+x*x and retain broad frame,
pointer-home and x87 differences. The additionally requested split square-root
and integer1 reciprocal in `camera-independent-direction-cpp-02` also produces
480 bytes and346 differing non-relocation offsets. These results and independent
retail math bodies were sent as a follow-up in the same404e90-specific thread;
the decision remains pending. The primitive's comment was corrected without
changing source line count: x87 intermediates still round according to the
ambient control word, and macro syntax/header naming are reconstruction choices.

Separate current-TU FastExp counterexperiments in
`camera-independent-fast-exp-cpp-01` try a volatile integer with pointer punning,
a volatile integer with a union result, and a volatile read of a normal integer.
All produce the same1520-byte body versus retail1552. The integer reload survives,
but register scheduling changes, one result store disappears into a pushed
argument, and a multiplication pair combines. These diagnostic counterexamples
are included in that entry's independent review packet; no volatile production
code or new raw primitive was introduced.

The404e90 follow-up at
`.devspace/runs/2026-09-10T08-07-59-121Z-chatgpt-call` explicitly allows only
the71-byte island[404f18,404f5f). The submitted current-TU exact-order and
split-integer-reciprocal tests resolve its initial blocker; independent retail
examples support a shared inlined assembly primitive. Pro identifies no remaining
credible ordinary-C++ variant that should block this exception, while retaining
unresolved macro spelling, syntax and header ownership. The receipt confirms
submission and GPT-5.6 Sol/Pro selection. The allowlist now records the independent
review and preceding hold; this accepts no whole-function or owner facts.
One of nine entry reviews is settled. The405040 direction case has started in
its own fresh thread, with no inherited approval from404e90.

The first405040 direction attempt at08:20 failed model-menu cleanup before
submission. Its subsequent attempt was interrupted without a receipt or saved
conversation URL. After the user updated the Pro plugin to1.1.4, canonical
recovery reached an empty home page; no answer from that interrupted attempt
is used. A fresh independent call at
`.devspace/runs/2026-09-10T09-13-56-419Z-chatgpt-call` verifies and enters the
complete228414-character packet in6.6 seconds and captures a confirmed Pro answer.

That review allows exactly[405582,4055c9) in405040. It independently evaluates
the current consumer's C++ fallback experiment, which keeps1552 bytes but changes
the frame, pointer homes and x87 schedule. It identifies no credible remaining
C++ alternative that should block the exception. The allowlist records this
new review and uses current operand names: cameraScratch minus cameraPos into
cameraDirNext. The header comment now distinguishes this implementation's
once-only pointer captures from the unknown historical generic argument
evaluation contract, preserving source line count. No executable code changed.
Two of nine reviews are settled; the XZ-length entry is the next independent case.

Fresh exact-consumer portable-direction probes in
`camera-independent-direction-consumers-cpp-01` replace one invocation at a
time, leaving every other primitive unchanged. Their extents are1552 at405040,
400 at405650,304 at405870 and848 at406110; every candidate differs from retail.
The current-TU XZ-length and XZ-dot C++ macro probes, and their ordinary inline
function forms in `camera-independent-math-cpp-01` and02, also fail. The inline
forms emit the same differing operations as the macros, with no helper call
remaining. The current-TU C++ SinCos normal arm produces1520 bytes, and plain
C++ FastExp produces1504, versus retail1552. These are diagnostic counterexamples,
not new production implementations or matching acceptance.

The independent XZ-length review at
`.devspace/runs/2026-09-10T09-31-41-454Z-chatgpt-call` withholds approval pending
vector-first fastcall helpers with an output reference and output pointer.
Both are compiled in `camera-independent-math-cpp-03`; both naturally inline
and emit exactly the same candidate instructions as the earlier C++ macro and
returned-float helper. The mandatory pointer reload and float result boundary
remain absent. The corresponding output-interface dot-product probes likewise
emit the same divergent sequence as their earlier C++ forms.

The XZ-length follow-up at
`.devspace/runs/2026-09-10T09-41-09-429Z-chatgpt-call` explicitly allows only
[405219,40522d) in405040 and identifies no remaining credible source-faithful
C++ variant that should block it. The allowlist records both the initial hold
and its resolution. The source comment now states the exact consumer and range,
with unchanged line count. Three of nine reviews are settled. Dot product has
started in its own independent thread with all four current-TU interface probes
and direct immutable-retail disassembly of the independent AI island.

The independent XZ-dot review at
`.devspace/runs/2026-09-10T09-47-18-368Z-chatgpt-call` allows only
[40532d,405342) in405040. Submission is confirmed and the receipt verifies
GPT-5.6 Sol/Pro. Four current-TU C++ interfaces converge on the same1536-byte
candidate, losing the pointer homes and binary32 result boundary before fabs.
An independent AI island repeats the exact21-byte pattern. Pro favors historical
inline assembly but does not consider its provenance proved; no materially
distinct credible C++ variant remains. The header comment and allowlist now
record that scope without changing source line counts. Four of nine reviews
are settled; no whole-function matching or tracker acceptance follows.

The independent FastExp review at
`.devspace/runs/2026-09-10T10-01-11-225Z-chatgpt-call` allows only the four
MOV/ADD/MOV bridges in405040. Submission is confirmed with GPT-5.6 Sol/Pro.
The redundant integer home/reload and explicit result home survive at all four
retail sites, while current C++ variants remove or reschedule them. The preceding
compiler stores remain ordinary interaction dependencies outside the allowlist.
Pro finds no credible C++ blocker; an unsigned-union spelling is suggested as a
nonblocking sanity probe. Historical inline assembly is strongly supported but
original spelling, namespace, header and function-versus-macro form are unresolved.
The header comment is narrowed with unchanged line count. Five of nine reviews
are settled; this accepts no standalone helper or whole-function facts.

The suggested unsigned-union FastExp probe was subsequently compiled in fresh
`camera-independent-fast-exp-cpp-02`, using the exact code from the review.
It produces1504 bytes and860 differing non-relocation offsets, matching the
earlier ordinary C++ helper's diagnostic extent and difference count. The four
retail bridges are not recovered; production code remains unchanged.

The independent SinCos review at
`.devspace/runs/2026-09-10T10-12-12-889Z-chatgpt-call` allows exactly the six
instructions in[405390,40539f), with this consumer's distinct local outputs.
Submission is confirmed and Pro selection verified. Current-TU ordinary C++
produces1520 bytes, separate FSIN/FCOS and different output homes and lifetimes.
Independent retail functions repeat the fixed EBX/EDX roles and compiler-owned
preservation. No credible C++ blocker remains; an outputs-first reference spelling
is a nonblocking completeness probe. The source comment now includes the
unordered-input arm and distinct-output precondition, preserving line counts.
The fallback remains separate C++ expressions with VC5-controlled precision,
not a claimed robust large-argument reduction. Six of nine reviews are settled.

The nonblocking outputs-first reference SinCos probe was compiled in fresh
`camera-independent-sin-cos-reference-01`. It retains the threshold and both
C++ arms while changing only the helper interface and its matching call. It
produces1520 bytes and622 differing non-relocation offsets, with no FSINCOS.
Production remains unchanged. Comment-only edits preserve source line counts
and executable tokens relative to the submitted dot-review header snapshot.

The independent405650 direction review at
`.devspace/runs/2026-09-10T10-19-57-776Z-chatgpt-call` allows only
[405725,40576c). Submission is confirmed with GPT-5.6 Sol/Pro. It identifies
an important current diagnostic limit: the first three MOVs have reversed
source/destination pointer-home offsets, while the x87 suffix[40572e,40576c)
matches. This is raw-assembly eligibility only, not an instruction-match
classification, a complete71-byte match, or whole-function acceptance. The
suggested reversed capture declaration order requires a fresh experiment.
No credible pure-C++ blocker remains; a return-by-value helper is suggested
as a nonblocking forensic probe. Seven of nine reviews are settled.

Fresh `camera-independent-direction-review-options-01` tests both suggestions.
Reversing the shared raw macro's pointer declarations leaves405650's homes
unchanged and introduces12,15 and4 non-relocation differences in previously
matching404e90,405040 and405870 respectively. It is not applied. The native
return-by-value C++ helper fully compiles but produces416 bytes versus384 with
338 differing offsets. This adds failed source counterevidence; it does not
resolve the remaining405650 frame layout or enclosing-body mismatch.

The same native by-value C++ interface was tested at the two remaining exact
consumers in `camera-independent-direction-review-options-02`. It fully inlines
without helper relocations at both sites, producing336 bytes at405870 and912
at406110 versus retail304 and736. Both remain divergent. The406110 result and
complete generated code were added to its still-unsubmitted independent prompt.

The independent405870 direction review at
`.devspace/runs/2026-09-10T10-28-54-593Z-chatgpt-call` allows exactly
[4058e6,40592d). Submission is confirmed with GPT-5.6 Sol/Pro. The current
diagnostic304-byte object body matches outside its four relocations, and the
71-byte island itself has no relocations. The exact-consumer C++ probe retains
304 bytes but changes the frame, pointer homes, constant relocation and x87
schedule. Pro finds no credible C++ blocker; its optional by-value interface
probe was already compiled in review-options02 and remains divergent at336
bytes. The preceding pointer homes, FPU pop and copy store remain compiler-owned.
Eight of nine reviews are settled. No whole-function live proof is inferred.

The independent406110 direction review at
`.devspace/runs/2026-09-10T10-36-42-898Z-chatgpt-call` allows only
[406380,4063c7). Submission is confirmed with GPT-5.6 Sol/Pro. The current
768-byte raw candidate still has unmatched frame/capture details and at least
one different pointer-home displacement; neither the whole body nor the entire
71-byte range is accepted. Current pointer-body and native by-value C++ probes
produce848 and912 bytes with the wrong local x87 lowering. No credible C++
blocker remains; an output-reference interface is an optional sanity probe.
Exact-object aliasing is supported, while arbitrary partial overlap is not
claimed as a recovered source contract. The camera-Y store before the island
and cameraAdjusted assignment after it remain compiler-owned.

All nine new entries now have separate independent Pro conversations and
explicit address-scoped decisions. This completes the requested review sweep,
not authored-call-contract or authored-byte-match. The apparent phrase
"instruction-level reconstruction" in two advisory answers describes raw-asm
eligibility only; no @recoil-match instruction fallback is registered from it.
Current executable source remains unchanged during the sweep; header comment
edits preserve line counts. All required C++ follow-ups and additional optional
interface probes are recorded above. The unresolved frame/home differences
remain ordinary reconstruction work requiring live machine comparison.

The final optional output-reference direction helper fully inlines at406110
in `camera-independent-direction-review-options-03`, but produces896 bytes
versus736 with the same material x87-lowering failures. It is not applied.
The review transport inventory verifies nine distinct conversations with
confirmed submissions, verified Pro power and complete response/transcript
artifacts. This inventory accepts no reconstruction facts.

Source policy initially caught two wording regressions from the provenance
cleanup: the direction comment no longer named raw assembly/failed VC5 C++
evidence, and FastExp/SinCos no longer identified the inferred inline helper
model. Those descriptions now explicitly state the failed-compiler rationale
and inferred original inline model while retaining unknown spelling/header/form.
No original text is claimed. Source line counts and executable tokens remain
unchanged. `stages-independent-raw-review-source-policy03.log` passes all14
source-policy checks; this is not a live function-match refresh.

The public owner-entry repair applied at revision6152, adding exactly thirteen
X data-entry records with empty evidence and accepting no reconstruction facts.
The associated historical boundary/data gate downgrade dry-run passes owner
invariants and preserves unrelated facts. These old aggregate acceptances did
not cover the expanded primary-data population; source/linkage were already
pending. A separate independent Pro review of call-only ICF eligibility has
started with these unresolved owner obligations stated explicitly.

The conservative boundary/data gate downgrade subsequently applied at r6153.
Source/linkage remain pending. Four native `_ftol` import-binding source
registration snapshots were refreshed through the guarded public route at
r6154-r6157. Each dry-run proved that only the two `registration_ids` paths
changed from `hud.cpp` to `camera.cpp`; all retail, operand, target, canonical
import-member and evidence facts remained identical. The fresh relocation
expectation audit for 405040 has no unresolved entries and retains all thirteen
reviewed scalar exceptions. WSI-20260910-008 tracks the missing refresh route.

The independent call-only ICF review completed in
`.devspace/runs/2026-09-10T10-55-18-613Z-chatgpt-call/assistant.md` with confirmed
submission and verified Pro mode. It supports exact site-bound logical-call
eligibility while rejecting a blanket aggregate-owner-gate bypass. The original
five order aliases retain their existing strict checks. WSI-20260910-009 tracks
this missing typed policy route.

`progress call-contract bind-icf-extension` now registers a versioned exact
contract for physical4076f0, the provisional Player camera alias, caller404e90,
call404fa2 and operand404fa3. It applied at r6158 after dry-run review and generic
negative tests. The extension is excluded from unrestricted object/logical/group
maps and every order population. Source checks require an exclusive resolved
edge, canonical production TU, unique attached anchor/artifact and compatible
signature. Live verification requires the exact local external COFF definition,
call relocation, return behavior, caller instruction/operand shape and local
control-flow target semantics. This conservative first version grants no
function-byte acceptance; changed caller shapes require a separate ABI proof.
No owner gate, tier, provider, original spelling/TU, winner or byte fact is
accepted by registration. The ordinary full call census and fresh closeout
remain mandatory. The focused proof kernel passes164 cases; the whole-stage
replay is being run against current production source.

The first replay attempt stopped during BN prefetch with retryable HTTP409
`view_changed`; strict preflight still authenticated the same Recoil database.
The second attempt passed that point but was interrupted by the user before
completion. Neither attempt accepted call facts.

The concrete implementation review was recovered through the Pro read route at
`.devspace/runs/2026-09-10T11-45-24-551Z-chatgpt-read-current/assistant.md`.
It BLOCKED the initial implementation: raw contract inventory could bypass the
complete group-validation path; caller/ABI expectations needed independent
binding; and the inspected callee record needed an explicit same-object,
same-symbol, same-section join to the call relocation. Source qualification and
zero-cleanup proof also needed tightening. This did not reject the scoped policy.

The corrected code returns call-only entries solely from complete group
validation and rejects every orphan or misattached inventory occurrence. Caller
identity now binds to its accepted, synchronized order target, and the exact
review's ABI/unknown/withheld dimensions are checked. Current COFF records carry
object path, symbol index, section, value and type; duplicate external definitions
are rejected. Whole-program closeout also requires exactly one callback
definition in its canonical TU. Source anchors/artifacts share an exact qualified
construct. A forward CFG proof rejects unknown post-call stack effects rather
than treating unknown cleanup as zero.

Fresh diagnostic slice01 exposed two implementation details: the compiler's
source spelling names its governed `_tu_order` copy, and raw operand decoding
also lists non-address constants and internal branch displacements. Current
source mapping now requires the exact stable verifier/parent source observation.
Retail-derived filtering leaves constants and internal branch bytes exact.
No field is selected for masking from candidate output.

Fresh `stages-camera-call-only-slice-02` passes all160 bodies with zero
divergences and unchanged source. `stages-call-only-pro-fixes-full-kernel02.log`
passes250 generic tests. `stages-call-only-doctor02.log` passes the first five
infrastructure checks, then stops on709 stale function-match annotations; their
fresh match refresh remains outstanding. WSI006/007/008 are resolved with their
scoped repair evidence. WSI009 remains open pending the concrete Pro re-review
and live stage progression. No call stage acceptance or byte acceptance is
claimed from the focused slice result.

The existing dispatch relocation exception at404e90 offset275 also retained its
old HUD source registration. Its public remove/set dry-runs showed exact
old/new equality outside `source_binding.registration_ids`, with only
`hud.cpp` becoming `camera.cpp`. Removal at r6159 and re-registration at r6160
preserved its target4076f0, logical camera-state5 identity, evidence and
original-name/TU/winner uncertainty. Fresh
`stages-camera-dispatch-relocations-after-refresh-r6160.json` reports no
unresolved relocation expectations. This refresh accepts no body bytes.

The second concrete Pro critique at
`.devspace/runs/2026-09-10T12-06-35-387Z-chatgpt-call/assistant.md` confirmed the
validated-index, exact COFF join, source mapping, ABI/value-flow and cleanup
corrections. It narrowly BLOCKED caller-target synchronization and competing
regex-name claims. Current and stored caller row views are now explicitly
compared, with exact current target metadata and symbol-to-target membership.
Only dictionary-equivalent recognized mirror views are allowed; missing, extra
or conflicting rows fail. Competing literal and regex claims now check the
caller's exact semantic classification, rejecting regex ambiguity and provider
conflicts. Order scopes may retain their distinct order-gate flags.

`stages-call-only-caller-fixes-full-kernel03.log` passes250 generic tests.
Fresh `stages-camera-call-only-slice-03` again passes all160 bodies with zero
divergences and unchanged source. A focused follow-up Pro review is submitted
in `stages-icf-call-only-implementation-review03.md`. No call-stage acceptance
is claimed yet. Doctor03 still stops at709 stale function-match annotations,
so fresh full match classification is being run separately from stage replay.

Pro's final implementation response
`.devspace/runs/2026-09-10T12-26-26-587Z-chatgpt-call/assistant.md` ALLOWs the
corrected call-only prerequisite. It explicitly confirms the current caller
authority, competing-name conflict checks, exact local definition and absence
of global identity leakage. The optional suggestions are not acceptance gaps.
Fresh full-stage replay03 is now running; Pro advice accepts no reconstruction.

Full match refresh10 completed all3292 functions and applied at r6161:
944byte and2348unmatched. It removed the old loader instruction annotation
because the captured global compiler-profile map now includes camera.cpp.
Direct context comparison finds no changed loader/include file or other
context field; the loader's own profile is unchanged. Its496-byte,
170-instruction, ten-register-difference proof and11 relocation checks still
pass, but renewed Pro eligibility and a fresh linked instruction proof remain
required. That narrow renewal is submitted separately. Doctor04 passes all
seven infrastructure checks after the annotation refresh.

The first refresh10 unmatched function is405040. Its1552-byte object body and
normalized linked body are equal; four references at504/697/973/1103 fail the
complete reader-population proof for scalar4ccab8. Read-only BN xrefs identify
four chase-camera readers and one first-person-camera reader at405aa3 in4059a0.
Current first-person source still uses its own combined exponential expression;
the chase path uses the previously reviewed FastExp primitive. No source change
or new raw-assembly use has been made on this basis while call replay runs.

The loader renewal response
`.devspace/runs/2026-09-10T12-37-45-725Z-chatgpt-call/assistant.md` explicitly
confirms compiler GPR-only attribution, no remaining credible source option,
and `INSTRUCTION_MATCH_APPROVED`. Registration and fresh live proof are pending
until the active serial replay finishes, avoiding concurrent tracker mutation.

Isolated first-person C++ experiments in
`camera-first-person-exp-cpp-01` retain no reader of the retail4ccab8 scalar:
baseline720bytes, split-float temporary736bytes, ordinary pure-C++ inline
FastExp720bytes, versus retail752bytes. The latter does not add raw assembly.
All still have broad opcode differences; these are counterexamples only, not
source or byte acceptance. Canonical source stayed unchanged throughout replay.

Replay03 completed its full3292-body,96-target proof in1048806ms and recorded
three complete160-body slices plus145 passing bodies in the first divergent
slice,625 total, advancing r6161 to r6165. Its first divergence is4174f0.
All59 full-census failures have the same cross-TU callee-source prerequisite:
`dispatch.py` required the physical block's historical `original_source_path`
to equal the current defining TU. The reviewed extraction intentionally left
that original path null. Current HUD source edges, contribution and block
implementation paths remain consistent. WSI009 is resolved at issue r4150;
this distinct defect is WSI010, reported at issue r4151.

The current-source join is now factored and tested independently, retaining
every prior current contribution/edge/path requirement while excluding the
historical filename from call authority. Current target synchronization,
authored order, exact external COFF identity and call relocations remain in
the surrounding proof. Generic negative source-join cases pass; the full
kernel again passes250. Generations advanced conservatively, so call evidence
must be refreshed. A fresh formerly divergent slice is running.

Loader profile-only review renewal applied at r6166 after reviewed dry-run.
Its fresh complete instruction proof and annotation restoration are pending
the next full match refresh under the corrected verifier generation.

Fresh `stages-cross-tu-source-slice-01` passes all160 bodies in223546ms,
including all15 formerly blocked bodies in that slice. Full match refresh11
then completed and applied at r6167:944byte,1instruction,2347unmatched.
The loader annotation is restored only after its renewed review and complete
new object/relocation/linked-instruction proof.

Additional first-person scalar diagnostics: signed and unsigned union helpers
each remain720bytes with no4ccab8 reader. An isolated use of the existing
minimal-assembly FastExp produces704bytes and restores that reader. Its
MOV/ADD/MOV primitive uses a compiler-chosen input home at ebp-0xc instead of
retail ebp-0x10; the destination matches ebp-0x8. The full function remains
broadly unmatched. A new independent Pro thread `raw-review-4059a0-fast-exp`
is reviewing only the proposed11-byte [405ab1,405abc) bridge. No production
call or allowlist entry has been added. C++ retains arithmetic, native _ftol
and its preceding store; this proposal cannot authorize other math primitives.

The independent first-person review at
`.devspace/runs/2026-09-10T13-12-50-076Z-chatgpt-call/assistant.md` returns
ALLOW eligibility only for [405ab1,405abc). It accepts neither the current
stack-home difference nor the whole function, and excludes all surrounding
arithmetic/_ftol/store/x87 operations, other primitives, frame forcing and
body/data/owner/order/tier acceptance. No production or allowlist change is
applied while replay04 runs. The earlier nine-entry review sweep remains
complete; this is a separately reviewed proposed tenth case.

Doctor after the cross-TU source fix passes all seven infrastructure checks.
Replay04 now runs all21 slices with the current163/105 verifier generations.
Read-only camera audits at4059a0/405ee0/406110 identify missing typed target
bindings, not stale source registrations: scalar identities and the native
_ftol edge still require their governed byte-phase binding/proof procedures.

Replay04 completed with all 3,292 bodies passing across all 21 slices and no
divergence. Every slice was recorded serially, advancing transaction revision
6167 to 6188; semantic and evidence-generation revisions are both 6185.
The result is `closeout-ready`, not call-stage completion. WSI010 is resolved
at issue revision 4152 with this full replay and the focused/kernel checks.
The returned mandatory closeout is running below the fresh
`call-contract-closeout/all-r6189` root with eight verification workers,
followed by its one canonical whole-program linkability diagnostic.

The fresh closeout passed and applied at r6189 (semantic/evidence revisions
6186). All 21 isolated slices passed all 3,292 bodies; the scan took 745799ms.
Its canonical whole-program compile, aliases, resources and link passed with
deployment, linked-order, byte and final-image acceptance suppressed. The
scheduler now selects authored-byte-match at physical block 0x404ca0.

The separately reviewed first-person FastExp call is now applied, with its
address-scoped tenth allowlist entry. The helper comment preserves its line
count and code. No fragment or function match follows from the Pro decision;
fresh object, reader-population, relocation and linked verification is pending.

The fresh full match refresh14 passed and applied at r6190: 945 byte matches,
one approved instruction match, and 2,346 unmatched functions. The first-person
FastExp change restores the complete scalar reader population and 0x405040 now
passes object, relocation and normalized linked-body proof. Its docblock lacks
a spare line-preserving annotation slot, so the refresh leaves the annotation
excluded rather than shifting source lines. Earlier refresh12/13 runs were
terminated before acceptance to complete the raw-consumer declaration while
preserving the function docblock's line count; neither is evidence.

The first divergence is now 0x405650, object offset 12. Six isolated C++ vector
addition variants fail. A proposed 35-byte raw addition at [405674,405697)
produces all 384 nonrelocation object bytes equal, including the direction
primitive's formerly differing pointer homes. These are diagnostic results only.
A new Pro thread `raw-review-405650-vector-add` receives the complete function,
fresh retail assembly, canonical flags, all six failed variants, the proposal,
and the identical existing AI helper for source-sharing scrutiny. No vector-add
source/allowlist change is applied pending this independent review.

Refresh15 applied at r6191 with the same 945 byte / one instruction matches and
added the chase-camera byte annotation after a line-preserving signature/docblock
adjustment. An attempted live byte advance stopped in source policy before
building: the shortened FastExp comment no longer matched the original-inline
provenance pattern. Restoring that wording passes the focused original-symbol
guard with zero violations. That stopped advance accepted nothing.

The independent vector-add review returned ALLOW eligibility only in
`.devspace/runs/2026-09-10T14-08-52-544Z-chatgpt-call/assistant.md`.
It supports a neutral inferred math-header primitive and explicitly defers AI
wrapper consolidation and all other consumer approvals. Its two additional
exact-capture C++ controls were run: float locals produce 400 bytes and direct
stores 384 bytes, both with 324 nonrelocation differences. The approved macro
is now applied only to 0x405650 with an eleventh independently reviewed allowlist
entry. Fresh full match refresh16 is running. Pro's requested final placement
check remains a later serial-stage obligation; this work grants no linked RVA
acceptance.

The next neighbor, 0x4057d0, differs in twelve operand bytes: each x87 addition
loads the target offset before the world position. Nine alternative C++ forms
were examined, including reversed addends and explicit precision. Ordinary
float locals initialized from world position, then incremented by the offset,
produce the complete 160-byte object body outside relocation fields. This needs
no raw assembly or instruction fallback. The production edit and fresh complete
proof are pending until refresh16 finishes.

Refresh16 completed and applied at r6192: 946 byte matches, one instruction
match. The full 0x405650 production body, relocation semantics and normalized
linked bytes pass, and its byte annotation is recorded. The first remaining
divergence is 0x4057d0 at operand byte 9. Its ordinary float-local compound-add
fix is now applied and full match refresh17 is running. The top-down docblock
and signature adjustment preserves its body start while reserving a separate
annotation line and prose separator.

Refresh17 applied at r6193 with 947 byte matches and one instruction match.
The 0x4057d0 C++ correction passes full object, relocation and normalized linked
proof and receives its byte annotation. The first divergence is now 0x4059a0,
whose seven unresolved operand sites require five scalar records and one native
_ftol import binding. Three scalar targets (4ccae0/4ccae4/4ccae8) are absent from
the tracker; fresh complete indexed BN xrefs show one first-person reader each
and no data references. `stages-first-person-new-scalar-xrefs.json` records that
census. Candidate-independent mutation requests are prepared but not applied.
Live authored-byte advancement is running below `authored/404ca0-r6194`.

The live authored-byte advance completed and applied at r6194, recording
18 newly passing groups before the first-person relocation-expectation
divergence at 0x4059a0. Its source-policy prerequisite passed all 14 checks,
including 728 valid match annotations and zero findings. The nonzero command
exit reports the remaining divergence; the passing prefix was accepted.

The user requested a new playground executable. The explicit non-accepting
playground-only build is running below `playground/camera-fixes-20260910-01`;
deployment remains pending its canonical compile/resource/link and complete
authored linked-presence safeguard. No executable existed at the destination
when this request was received.

For the next first-person dependency, eight current-TU C++ length controls fail.
A shadow use of the existing XZ-length macro reproduces its 20-byte retail
island [405a73,405a87), including symbolic homes, at a shifted candidate offset.
The full function remains 704 versus 752 bytes and broadly unmatched. A new
independent Pro thread `raw-review-4059a0-length-xz` is reviewing only that
proposed consumer; no production length call or twelfth allowlist entry exists.

The requested playground build succeeded and deployed
`playground/Recoil-rebuild.exe` (1,242,624 bytes, 2026-09-10 16:41:53 +02:00).
The fresh canonical compile, COFF aliases, resources and link passed. The
required-authored-linked-presence safeguard passed all 3,292 selectors/bodies
with zero divergences. This is a play-test build only and accepts no order,
bytes, final image or runtime behavior. Source remains at the three newly
verified camera fixes; the proposed first-person length change is not included.

The independent first-person length review returned ALLOW in
`.devspace/runs/2026-09-10T14-39-00-988Z-chatgpt-call/assistant.md`.
Its approved 20-byte consumer is now applied, with the twelfth address-scoped
allowlist entry. All twelve additions have confirmed Pro submissions and
distinct conversation URLs; the transport inventory passes with no pending
review. This does not establish a complete first-person function match.

Five scalar dependencies were dry-run, reviewed and applied at r6195 through
r6199. The three new four-byte data records remain pending camera auxiliaries;
their names are navigation labels, with no original identifier, owner gate,
tier, storage-byte or linked-placement acceptance. The native MSVCRT _ftol
dependency was applied at r6200 after reviewing its canonical import member
and current first-person call-contract evidence r6168:000091. An initial
request incorrectly cited camera-owner evidence; the command rejected it
without mutation, and the corrected request passed a new dry-run.

Seven C++ matrix-transform controls fail in the current consumer. A shadow
83-byte symbolic transform reproduces the local retail sequence and homes,
but the full function remains unmatched. The new independent thread
`raw-review-4059a0-transform` is reviewing that case. Six additional vector-add
C++ controls also fail; that separate consumer has not yet been reviewed or
applied. Composed shadow diagnostics combine the proposals only to diagnose
remaining native C++ behavior; they grant no raw eligibility or acceptance.

The first transform review returned BLOCK in
`.devspace/runs/2026-09-10T15-01-14-903Z-chatgpt-call/assistant.md`:
its boundary/ABI/x87/alias audit supports the proposal, but it requests staged
zVec3 member updates before raw eligibility. All three requested direct,
captured-pointer and inline-helper forms fail in fresh controls; four additional
grouped-product controls also fail. The same thread is reviewing those results.
No transform raw source or allowlist entry is applied.

The first-person relocation audit at r6200 has zero unresolved expectations.
Full match refresh18 is running against the applied length change and bindings.
Composed diagnostic source can reproduce all 752 nonrelocation bytes using the
two still-unaccepted math proposals, a local camera-elevation comparison, a
reused camera point and embedded temporary assignments in the limit products.
The last form remains under source-shape scrutiny; ordinary separate
assignments and declaration-order changes retain four operand differences.

Read-only scrutiny of the adjacent 0x405c90 finds stale current-state caching
across calls and overbroad restore-previous behavior in the current source.
Reading the state at retail's actual use sites, reloading the global save
state for the update callbacks, and restoring the prior state only in the
projectile/clear-screen branches reproduces its full 560 nonrelocation bytes
in C++. Its relocation audit has no unresolved expectations. The production
correction is pending completion of the in-flight match refresh.

Full refresh18 completed at r6201 with 947 byte / one instruction match and no annotation edits.
The second matrix review at
`.devspace/runs/2026-09-10T15-13-12-098Z-chatgpt-call/assistant.md`
discharged the staged-member gap but requested two bounded scalar-sink forms.
Both exact requested inline Set(x,y,z) and Make(x,y,z) forms fail at 704 bytes,
with no out-of-line helper calls. The same independent thread is reviewing
that final evidence; raw eligibility remains withheld.

The ordinary C++ 0x405c90 correction was applied, then scoped live match
refresh19 passed its full 560-byte object, relocation and normalized linked
proof at r6202. It receives a byte annotation. That scoped invocation removed
four other camera annotations not re-proved in its current TU context; the
next full refresh will classify and mirror the whole current population.

Read-only neighbor analysis of 0x405ee0 finds no retail zero-initialization of
the temporary vectors, segment array or probe output buffer. Removing those
initializations in an isolated diagnostic reduces 624 bytes to 512 versus
retail 560. Its vector-add and subtract islands still need separate source
work/reviews; no production change or raw proposal is applied. The one missing
relocation target is a four-byte float -2 at 0x4ccb04, used by FSUB at 0x405ff3.

The third transform response explicitly ALLOWs only the 83-byte island:
`.devspace/runs/2026-09-10T15-22-49-463Z-chatgpt-call/assistant.md`.
Its two earlier procedural blocks are superseded after sixteen C++ controls.
The inferred typed matrix/vector helper and thirteenth address-scoped entry
were applied. All thirteen entries have separate confirmed Pro conversations.

The user corrected the numeric structure-field addressing in the raw source.
VC5 accepts named member operands such as `[ebx]zVec3.z` and
`[ebx]zMat4x3.xz`. All 56 field operands in the math header were changed,
including the pre-existing Vec3Normalize helper and zero-offset fields.
No handwritten field displacements remain in that header, and no newly added
raw numeric register-offset expressions remain in the source diff. A fresh
before/after camera compile preserves all nineteen emitted function bodies and
relocation records exactly, with identical source/header line counts.
`typed-asm-fields-01/after/comparison.json` records that diagnostic.
Full live match refresh20 is running at expected r6202.

The independent addition review initially received the earlier prepared
289,931-character snapshot because a UTF-8 decoding error prevented rebuilding
its prompt. That submission is not treated as a current-context approval.
A corrected same-thread follow-up is prepared with the applied matrix review,
fresh eight-form addition controls, named-member source, exact before/after
proof and the user's prohibition. A new current-header composed diagnostic
retains four product-operand differences with a clean cached comparison, or
zero nonrelocation differences with the disputed embedded product assignments.
Neither the addition consumer nor those native product assignments is applied.

Full refresh20 completed at r6203: 948 byte matches and one approved instruction
match. Four eligible camera annotations were restored. The first doctor run
overlapped that refresh and saw stale annotations; the subsequent settled-state
doctor passed all seven gates. The typed-field change therefore preserves the
current complete matching census as well as the nineteen-body diagnostic.

The first addition response at
`.devspace/runs/2026-09-10T15-31-59-317Z-chatgpt-call/assistant.md` grants only
the 35-byte add island and explicitly rejects the dead embedded scalar
assignments. Those assignments will not be applied. It accepts the clean
cached comparison and scratch-vector reuse as provisional source choices and
requests true caller-output FastExp and a two-bound scaling helper as bounded
alternatives. All three fresh controls (each separately and their combination)
retain the same four operand differences at 752 bytes in
`first-person-exp-output-01`. No extra helper call remains.

The same independent addition thread now has a verified current-context
follow-up with the actual named-member header, the user's prohibition, all
eight addition forms, and those exact requested scalar controls. Its prompt is
`build/diagnostics/stages-first-person-add-raw-review03.md`; review02 was a
prepared intermediate and was not submitted. Current production remains
unchanged pending that response, with thirteen independently reviewed raw
entries and no first-person match annotation. The four scalar differences
are x87 memory-operand order, outside the GPR-only instruction relaxation.

The final first-person addition response explicitly ALLOWs the named-member
island at `.devspace/runs/2026-09-10T15-49-37-708Z-chatgpt-call/assistant.md`.
It rejects the embedded scalar assignments and requests context/census evidence
instead of further artificial expression variants. The fourteenth raw entry,
its consumer annotation, the clean cached comparison and camera-point reuse
were applied. Fourteen new entries now have fourteen independent confirmed
Pro threads. Refresh21 completed at r6204 with the same 948 byte / one
instruction census. First-person is still 752 bytes with four scalar operand
differences and has no match annotation. A diagnostic of that fresh canonical
object and linked image proves the 35-byte island and preceding 18 compiler-owned
setup bytes exactly, with no island relocations. See
`stages-first-person-add-linked-island21.json`; this does not accept the body.

The requested TU-context hypothesis was tested without undoing the reviewed
camera split. The current complete camera unit and a counterfactual insertion
back into the pre-extraction HUD position both retain exactly four differences
under identical governed flags (`first-person-tu-context-01`). The old broad
HUD ownership comments in BN are superseded navigation labels, not a reason to
reverse the independently reviewed literal-pool boundary. Grouped real local
declarations, inline-math source forms and explicit float-bit representation
controls also fail to resolve those four bytes; none is applied.

A bounded retail opcode census (`stages-retail-bound-products01.json`) finds
49 candidate paired multiplication sequences. BN confirms the closest other
stack-local bound-first examples at 0x424650 and 0x472b00 are contiguous vector
component scaling, not evidence of a two-bound type or helper for this caller.
Indexed xrefs to 0x4f3710/14 contain only first-person reads and initialization
stores in 0x41fe90; there are no indexed data refs. Captured BN assembly excerpts
live in `stages-census-bn-0x424270.json`, `-0x472a10.json` and `-0x41fe90.json`.
The initialization capture is paginated but includes all six relevant stores/
reads identified by the complete xref query. No scalar source model is accepted.

The second requested playground refresh compiled/linked and passed presence/
startup safeguards, but the background game process prevented atomic replacement.
After the user closed it, the third fresh build deployed successfully:
`build/live-validation/playground/camera-add-fixes-20260910-03`, 1,242,624 bytes,
2026-09-10 18:13:26 local. All 3,292 authored bodies are present. This includes
the first-person addition and camera-state fixes; no gameplay or reconstruction
acceptance is implied by deployment.

The user now authorizes ordinary file uploads to Pro; subsequent calls use a
short prompt plus an explicit source-mapped attachment. The next independent
addition case, 0x405ee0/[0x405f76,0x405f99), is pending with
`stages-offset-camera-add-raw-review01.md` and uploaded
`stages-offset-camera-raw-context01.md`. The subtract prompt is prepared but
not submitted; it must receive its own new thread. No source/allowlist entry
for either offset-probe island is applied. Eight ordinary C++ families were
tested in both initial and corrected-native contexts. A shared true inline
primitive (no standalone COFF helper symbols) plus ordinary native corrections
matches all 560 nonrelocation bytes. It also preserves the previous third-person
384-byte body and the first-person 752-byte/four-difference result. The pure
inline helper source form, previous-consumer review requirements, and prohibition
on whole-retail-function assembly are explicitly submitted for challenge.

Two further ordinary C++ corrections are applied: 0x406470 removes initialization
of callee-owned output buffers/variables absent in retail; 0x406730 uses a
countdown for-loop while preserving its existing typed candidate traversal.
Fresh diagnostic bodies match 160 and 112 bytes respectively. Full match
refresh22 is running at expected r6204 before adding either match annotation.
These two changes are newer than the deployed playground executable.

Refresh22 completed successfully at r6205: 950 byte matches and one reviewed
instruction match. Both new camera bodies passed complete object/relocation and
normalized linked-body checks. Their annotations were excluded because the
existing docblocks had no line-preserving insertion position with a prose
separator; this was an annotation-placement limitation, not a failed body proof.

The independent offset-add review initially returned BLOCK
(`.devspace/runs/2026-09-10T16-13-34-223Z-chatgpt-call/assistant.md`). Its requested
exact-signature ordinary-C++ controls and isolated add/subtract controls are now
complete in `offset-camera-isolation-03`. Native helpers with the exact proposed
left/right/destination formal order do not match. The add-only raw diagnostic
retains the 35-byte island at relative 0x96, but two compiler-owned parameter-home
displacements differ when subtraction is native. The reciprocal subtract-only
control also has different homes. The combined raw control remains 560 bytes
with zero nonrelocation differences and no emitted helper definition, call,
relocation or COMDAT. Follow-up02 supplies every source/body/relocation and
explicit COFF inventory, accurately reports the failed isolation result, and
asks whether eligibility may be conditional on separately reviewed subtraction
and exact fresh combined production proof. No offset-probe exception is applied.

Three more ordinary C++ corrections are applied for live verification:
0x406510 defers the saved variant fallback load until its fallback paths and
caches the chosen result after selecting it (256 diagnostic bytes exact);
0x4067a0 follows retail endpoint-copy order, caches the root/player position,
removes absent output initialization and returns a common result (240 exact);
0x406610 restores nested visibility decisions, root-node capture and the signed
vertical-ray scalar, and captures the camera pointer before assigning it
(288 exact). The last local capture resolves the EAX/ECX difference without
raw assembly, a new helper, or instruction fallback.

Reviewed scalar dependency mutations r6206 and r6207 identify retail .rdata
0x4ccaf8 (0.200000003f) and 0x4ccb10 (-50.0f). Both have one complete indexed
BN reader census, exact four-byte retail contents and compiler-temporary witness
requirements; no owner gate, tier or linked placement is accepted. Failed
initial dry runs (function-scoped instead of owner-scoped evidence, then a
concrete weather symbol instead of its registered regex selector) committed
nothing; corrected requests were reviewed before applying.

Nine camera docblocks now leave a line-preserving annotation position by
condensing their existing purpose prose, with no line-count change. Full
refresh23 is running at expected r6207 to prove the current source and mirror
only complete matches. These three native fixes are also newer than the
deployed playground build03. First-person 0x4059a0 remains four native x87
operand bytes different and has no match annotation.

Refresh23 completed at r6208 with 951 byte matches and one instruction match.
The sub-camera focus correction passed; four camera annotations were mirrored.
The anchor variant path still used the wrong global save-state binding and the
retail decoder omitted three register-relative image displacements. The source
now uses `g_CurrentPlayerSaveState`, matching retail 0x4f36a8. Existing data
identity 0x4f3ab0 received its evidenced four-byte extent at r6211, without a new
owner, storage contribution, source relationship or acceptance claim.

WSI-20260910-011 tracks the decoder omission. The decoder now identifies ModRM
and SIB disp32 image operands, including x87 and two-byte opcodes, while rejecting
ordinary member/frame displacements through the immutable image-range filter.
The existing generic proof-kernel cases exercise these forms. Refresh24 at
r6210 exposed duplicate enumeration of a mapped-switch operand; the switch-map
proof now upgrades its unique decoded site rather than adding a duplicate and
fails closed on ambiguity. The focused module passes all 21 tests. Refresh24's
temporary loss of the DispatchCoreCommand annotation is not a source regression.

The weather registration still selected its old cdecl symbol in one legacy
target. An attempted second scalar exception could not fix that population
error. The obsolete concrete exception was removed at r6212 and both manifests
were synchronized at r6213. The stale shared exception was explicitly removed
at r6214 and the same retail scalar/operand rebound against both registrations
at r6215, after reviewing each dry run. No target identity or scalar extent
changed. Full refresh25 and the infrastructure aggregate are running against
this corrected source, registration and decoder context.

The two offset-probe raw islands now have independent conditional eligibility:
ADD at `.devspace/runs/2026-09-10T16-29-34-363Z-chatgpt-call/assistant.md`, and
SUB at `.devspace/runs/2026-09-10T16-53-02-058Z-22d84340-22c0-4e59-ac47-b1e1d0c7b05b-chatgpt-read-current/assistant.md`.
The SUB prompt was confirmed once in the new room by the receipt under
`2026-09-10T16-44-30-051Z-7411ab73-5005-4cb9-85b6-94aaef9aac00-chatgpt-call`.
The plugin bound a transient WEB URL; the alias was rebound to its permanent
conversation URL and the completed response read without resubmitting.
Both verdicts require the combined source context, separate new-thread reviews
of the two earlier ADD consumer migrations, and fresh complete production proof.
Neither permits isolated application, numeric member/frame offsets or scalar
raw code. No offset-probe source or allowlist entry has been applied.

The proposed actual shared `zMath` header model was compiled in
`camera-vector-production-model-02`. It preserves the complete 384-byte
third-person body and first-person's exact four differing offsets, while the
offset-probe body becomes 560 diagnostic bytes with zero differences. No helper
definition, call, relocation or COMDAT survives; local scalar ordinals were
checked by their contents for this diagnostic only. The complete baseline and
proposed closures, responses and COFF evidence are uploaded as
`stages-vector-migration-context01.md`. The independent new 0x405650 migration
review is running; the separate 0x4059a0 prompt is prepared and not yet sent.

Refresh25 completed at r6216 with 953 byte proofs and one instruction proof.
Both 0x406510 and 0x406610 gained annotations and 0x4c20a0 was restored. Doctor03
ran before that refresh completed and correctly rejected stale annotations;
doctor04, after fresh proof, passes all seven gates. WSI-20260910-011 is resolved
at issue revision4154. The decoder repair has no outstanding validation failure.

The independent 0x405650 migration review returned conditional ALLOW at
`.devspace/runs/2026-09-10T17-10-25-318Z-f3e6f9e9-8ebb-4952-99e2-81e8a5b0bcc0-chatgpt-call/assistant.md`.
It confirms the unchanged 384-byte function, simple argument evaluation and
distinct destination, and requires complete actual-production object, relocation,
linked-body and whole-program no-helper checks. It grants no other consumer or
interval. The independent 0x4059a0 migration review is now running in its own new
room with the same complete source/evidence upload. Production migration is
still withheld pending that review and the combined fresh proof.

Side-probe diagnostics at 0x406110 now reach the exact 736-byte extent with
32 remaining bytes different. The first 488 bytes match outside relocation
fields in the best proposed context. Ordinary C++ capture of the negative scale,
an early result declaration and an ordinary inline vector-scale expression
resolve the native arithmetic/prologue differences. The diagnostic additionally
requires two unapproved ADD expansions and migration of the previously reviewed
direction macro to a real inline function; none of these source changes or raw
permissions has been applied. The remaining differences concern later native
argument preparation and register choices, including changed instruction order,
and therefore are not an instruction-match claim.

The independent first-person migration review returned conditional ALLOW at
`.devspace/runs/2026-09-10T17-19-37-153Z-c279b114-509a-46de-8286-16f978e3712f-chatgpt-call/assistant.md`.
It verifies the destination-equals-left case, argument-role conversion and
unchanged complete 752-byte diagnostic function. All four independent review
prerequisites now permit the combined source candidate. The old ADD macro was
replaced with typed `zMath::Vec3Add`/`Vec3Subtract`, the offset-probe native fixes
were applied, and the sixteen current allowlist additions have sixteen distinct
confirmed Pro rooms. The recovered SUB answer retains both its original
confirmed submission receipt and completed read receipt. No review substitutes
for a machine proof.

The -2.0f offset-probe scalar binding was reviewed and applied at r6217. Its
complete indexed BN census contains only 0x405ff3. Fresh full refresh26 completed
at r6218 with 955 byte matches and one instruction match, with no lost matches.
The offset-probe body passes both registered complete identities, including
typed relocation semantics and linked normalization, and gained its annotation.
`zMathQuatMultiply` at 0x475910 also gained a complete byte proof; no body source
was changed there. The first-person function remains 752 bytes with precisely
the same four native operand differences and no annotation.

`stages-camera-vector-production-proof26.json` scans all 89 canonical objects
and the final map and finds no ADD/SUB helper symbol, call, relocation or COMDAT.
All reviewed raw intervals outside the still-unmatched side-probe consumer match
their retail and linked bytes. The three affected complete bodies are separately
diagnosed in both object and linked output: third-person384/zero, first-person
752/four, and offset-probe560/zero. Those diagnostics claim no function or stage
acceptance beyond the live matcher.

The post-migration call closeout initially stopped before creating its fresh
root because the helper comments lacked explicit raw-assembly rationale, then
because they did not identify their inferred original inline role and retail
sites. Both docblocks now include those facts while retaining their line counts
and keeping spelling/header ownership unresolved. The source-policy guards
correctly exposed incomplete documentation; no proof rule was relaxed. Final
source-policy verification and the required fresh closeout are being retried.

The changed quaternion result is a concrete compiler-context observation:
refresh25 had twelve x87 operand bytes different, whereas refresh26 is exact.
Two shadow controls replacing the genuine direction macro with inline function
forms change quaternion output again (192 bytes to176), even though that TU has
no direction calls. Both controls retain the first-person752/four result. They
are diagnostics only; no direction migration or extra raw permission is applied.
Register-storage hints on live first-person floats also leave all four bytes.
A new Pro source/compiler investigation is running with the complete current
source, exact current/retail disassembly, compiler response and failed recipes;
it explicitly cannot authorize scalar raw code or an instruction relaxation.

The helper documentation corrections passed the source-policy guards. The next
close-live invocation correctly rejected mutation because the serial stage is
already authored-byte-match; the completed authored-call-contract stage cannot
be reopened through that command. No stage was rolled back or weakened. A
read-only regression instead invokes the public live verifier for each of the
original 21 slice selectors, deriving current facts anew, with eight bounded
verification subprocesses and no tracker mutation or per-slice linking.

Comment-only changes preserve the normalized source text used by instruction
review context, but do invalidate live function evidence through source file
dependency state. Accordingly a fresh complete refresh27 was run after the
docblocks changed. It committed r6219 with the same 955 byte matches and one
instruction match and no annotation edits. Doctor08 then passed all seven gates,
including the 737 current source annotations. No further production edit has
been applied after this refresh.

The read-only 21-slice regression04 did not fully pass: several slices were
blocked by Binary Ninja bridge HTTP409 view_changed responses and others by an
unresolved containing-function lookup. These are verifier/environment failures,
not established call divergences. Passing slices retain their diagnostic scope
only. A fresh database identity preflight passed, and failed slice18 is being
rerun in isolation to distinguish transient concurrency from a reproducible
lookup problem. A separate fresh canonical playground build is running; it
does not grant stage or final-image acceptance.

The native-products Pro response at
`.devspace/runs/2026-09-10T17-38-18-043Z-60651d23-8857-4c18-9bb0-a9ad499bc885-chatgpt-call/assistant.md`
requested bounded declaration-order and fixed-name controls, and rejected dead
assignments, invented scalar aggregates, further helper-signature permutations,
and arbitrary context perturbations. All six requested baseline/control forms
have now compiled: every first-person result remains 752 bytes/four differences,
and every chase result remains 1552 bytes/exact. Integer, unsigned and union
FastExp result representations likewise do not help; in-place integer mutation
regresses both functions. None has been applied to production.

Installed CL.EXE equals the preserved base VC5 media component directly, and
installed C1XX.DLL and C2.EXE equal the preserved SP3 ENU components directly.
Their differing version labels are therefore expected installation-overlay
provenance, not evidence of an accidental mixed toolchain. C2 has no COFF symbols
and names c2.dbg in its MISC debug record. No local compiler DBG was found; the
exact Microsoft symbol-server DBG index returned404 for each of c2.dbg,
c2.db_ and file.ptr. These component checks are diagnostics, not reconstruction
acceptance.

An adjacent native local/local FLD/FMUL census of current exact bodies found six
pairs in three functions after conservative raw-interval exclusions. Two AI
functions use address-taken vector components, and the chase function uses
vector components with a captured scalar. This is a limited syntactic census,
not proof of all compiler multiplication idioms. The completed probe results,
these source bodies, compiler component evidence and context observations were
uploaded in a follow-up to the same native-model Pro thread. The requested next
step is a bounded investigation of the unsymbolized compiler's operand choice;
no scalar-assembly or instruction fallback is requested or implied.

The complete fresh playground compile/resource/link finished successfully and
deployed `playground/Recoil-rebuild.exe` at 20:09:53 local time, 1,242,624 bytes,
from `build/live-validation/playground/camera-vector-fixes-20260910-04`.
The startup-contract diagnostic passed. No gameplay run or final-image
acceptance is claimed.

Isolated call-contract slice18 passed. Reduced-concurrency retries05 passed
slices20/21 but repeatedly encountered the same bridge view-change failures
elsewhere; no complete regression pass is claimed. The user has been asked
whether another task is mutating or reanalyzing the open database. Database
identity remains stable and status reports idle at the observed instant, but
its revision advances between requests. No plugin or database changes have been
made by this continuation, and no restart has been attempted.

Existing diagnostic compiler capture and hardware tracing infrastructure was
reused solely for the camera. Fresh captured full-TU inputs reproduce the
current752/four result and the rejected dead-assignment752/zero oracle. Tracing
the real C2 encoder preserves each output body's bytes and relocation records.
The current first load at relative0x11c is d945f8, followed by d84dfc; the oracle
loads d945fc and multiplies d84df8, with an intervening zero-length internal
instruction. Its operand record also differs. These observations locate a real
compiler-IR difference but do not identify its semantic cause or justify the
rejected source. A preliminary allocation-ordinal watch was not stable across
changed trace environments (the oracle watch did not arm), so its addresses and
writes are not accepted as tracing the desired record's origin.

The follow-up Pro response at
`.devspace/runs/2026-09-10T18-08-54-026Z-ffec66e9-0805-4ab3-bdb9-7b68dba09e08-chatgpt-call/assistant.md`
confirms there is no remaining credible finite source-form experiment. Its
leading compiler hypothesis is destination-affine store-back recognition,
with side-effect/alias flags and x87 costs as alternatives; symbol ordering is
now secondary. It recommends a bounded compiler provenance investigation, with
at most camera baseline/oracle, AI control and quaternion before/after traces,
and explicitly grants no scalar assembly, x87 instruction relaxation, invented
aggregate/helper, compiler-context knob, annotation or acceptance. Even a
compiler-mechanism proof would require new primary source evidence to choose
among historical source explanations.

The recurring BN read-instability issue was reviewed and recorded as
WSI-20260910-012 at issue revision4155. The reconstruction scheduler remains
r6219, authored-byte-match at0x4059a0; its current semantic/evidence revisions
are6216/6216. The earlier completed call-contract stage is retained.

The bounded compiler trace now correlates the unique encoded first-person FLD
with the predecessor inserted by C2's x87 load-lowering routine. In baseline,
the requesting source IR node has operation0x46 and calls through C2 address
0x462cad; the oracle uses operation0x138 and calls through0x47aef2. Both invoke
the load routine0x45f434. The input node's previous pointer changes to the
unique eventual FLD record, directly associating lowering with encoding.
`stages-camera-native-load-lowering04.json` contains both complete excerpts.
The baseline loads the scale value; the oracle loads the bound from its extra
assignment-related IR node. The tracer preserves each body's bytes and
relocations. This is a difference in the intermediate operation graph, not
merely final register numbering. It does not establish the precise earlier
optimization rule or the historical source that generated retail. No invented
source workaround is accepted. Further work requires tracing the earlier IR
transformation or obtaining stronger original-source/compiler evidence.

Across regression04, isolated slice18 and fresh retries05, thirteen complete
slices comprising2,012 bodies pass. Eight slices (4,5,9,11,15,16,17,19) remain
blocked by the bridge. This combination is a read-only regression inventory,
not a new whole-stage closeout or acceptance. The latest executable remains the
20:09:53 deployment; subsequent work changed diagnostics and this audit only.

Final doctor09 passes all seven infrastructure gates after the issue report and
diagnostic/audit updates. No reconstruction source, match annotation, compiler
component, BN plugin or open database was changed by the compiler tracing work.

The user confirmed that another task was reanalyzing Recoil.bndb, then stopped
that task. The resumed strict database preflight passes, and repeated status
observations retain view revision1095621. Fresh reduced-concurrency retries06
pass all eight previously blocked slices, comprising1,280 bodies, with no
bridge errors. Together with regression04, isolated slice18 and retries05,
the read-only original-slice inventory now covers all3,292 authored bodies.
Each passing slice rebuilt and compared current source in its own invocation;
this combined inventory is not a new single-run closeout or stage acceptance.
WSI-20260910-012 was resolved dry-run/review/apply at issue revision4156.
No plugin change, database mutation or restart was needed. Reconstruction
progress remains r6219, authored-byte-match at0x4059a0.

Further bounded compiler observations preserve the complete camera body's
bytes and relocation tuples for each diagnostic baseline/oracle. Prelower05
shows the immediate optimization at0x4623d1 returns0 without changing the
selected operand. Multiply-origin06 verifies its watched list-head address
against the eventual encoded instruction. Descriptor-verified09 verifies the
selected operand before lowering recycles its storage, then correlates the
same source instruction with the final FMUL. The descriptor's first observed
scale value is already present in its clone source; no bound-to-scale mutation
of that descriptor has been established. Copy-source10 ties the clone to an
earlier propagation pass replacing a scale temporary in the multiply's input
list. It does not prove initial IR construction or the historical source.

Multiply-list11 and operand-sort12 locate an actual reordering operation.
The exact instruction later encoded as the first-person FMUL at relative0x11f
enters C2's linked-list sort at0x407313 with input IDs0x14278 (bound),0x14283
(scale). The returned list is0x14283,0x14278. The comparator at0x408d58 reads
the DWORD at operand+0xc and orders the list by descending unsigned value,
as confirmed by the merge logic at0x409306. Later propagation replaces the
scale ID with0x1497b without changing its position; a second sort retains
scale before bound. These internal numeric fields are called IDs for this
diagnostic; their original compiler type/name is not yet established.

The rejected assignment oracle has a different operation graph. Its first
multiply sort retains0x143ba (assignment result),0x14283 (scale); its second
sort reverses0x14340 (assignment result),0x1497a (scale). The extra
assignment-related operation still loads the bound before the multiply is
lowered. Thus its matching emitted order is not evidence that sorting simply
chose the opposite two local-variable IDs. No dead assignment, fabricated
storage, scalar assembly, x87 instruction relaxation or match annotation is
accepted. The exact before/after excerpts and comparator/merge disassembly are
in `build/diagnostics/stages-camera-native-operand-sort12.json`. A direct
follow-up to the existing native-model Pro room asks it to challenge this
interpretation and identify whether any bounded source-faithful step follows.

A separate bounded side-probe local-output diagnostic remains negative.
Swapping the two ordinary output declarations leaves the reviewed shadow
context at736bytes/32 differences. Reusing the native scale helper in its
positive branch regresses it to720bytes/346 differences, with or without the
declaration swap. Nothing from these side-probe experiments was applied to
production or added to the raw-assembly allowlist. The latest playground
executable remains the20:09:53 deployment.

Operand-key13 and symbol-key-origin14 trace the sort key's producer. For the
observed kind-2 operand/symbol-kind-4 leaves, C2 computes
`0x10000 | ((symbolNumber ^ (symbolNumber >> 16)) & 0xffff)`.
The bound's symbol number0x14279 produces key0x14278; the original scale's
0x14282 produces0x14283. The propagated scale's number0x1497a produces0x1497b.
A hardware watch verifies the propagated scale's actual symbol record before
lowering, ties it to the encoded FMUL, and records its number being written at
C2 0x44ac28. That value is returned by0x40d5d8, which reads and increments
global0x491050. Its two directly observed initialization sites read compiler
streams. The allocator and fold are concrete compiler facts; the historical
source context behind the retail number ordering remains unknown. No stream,
counter, compiler instruction or production source was changed. The diagnostic
identity checks and disassembly are recorded in
`build/diagnostics/stages-camera-native-key-origin14.json`.

Resume doctor10 passes all seven infrastructure gates after the resolved BN
issue and audit updates. No new reconstruction acceptance is claimed.

The confirmed Pro follow-up completed at
`.devspace/runs/2026-09-10T18-59-15-527Z-2fbbe33c-fab7-45e0-9abe-9ab347a2b638-chatgpt-call/assistant.md`.
It confirms the first product's descending-key sort as the immediate cause and
withdraws the earlier direct destination-affinity explanation. It requests a
mirrored second-product identity trace and allows one natural branch-defined
bounds experiment if key provenance supports creation/value numbering. It
grants no assembly, x87 relaxation, source-context steering or acceptance.
The subsequently derived folded-symbol-number/counter observations above were
not part of that submitted Pro prompt; no Pro endorsement of those later
observations is claimed.

Second-product15 freshly traces the maximum-bound multiply through the sort,
load insertion and final FLD/FMUL encoding. Its first input list changes from
maximum0x1427b/scale0x14283 to scale0x14283/maximum0x1427b. Its later list retains
scale0x1497b before maximum0x1427b. The load routine inserts the exact FLD record
subsequently encoded at0x125, and its source is the exact FMUL record encoded
at0x128. Both captured baseline/oracle outputs retain their original complete
bodies and relocations. The same sorting mechanism therefore accounts for both
baseline products. The derived sort reports distinguish the pre-call comparator
pointer from post-call EDX; a returned value2 is not a changed comparator.

The single Pro-proposed branch-defined source model was compiled in fresh
diagnostic root `camera-branch-defined-bounds-20260910-16`. Every bound assignment
is live and occurs in one of the two speed-condition arms, without extra base
locals or aliases. It fails: first-person remains752bytes but changes from four
to269 nonrelocation byte differences. The exact chase body remains1552bytes.
No production edit was made. Pro's specified failure stopping rule applies:
end source-form probing rather than manufacturing newer bound values or tuning
unrelated compiler context. Authored-byte-match remains blocked at0x4059a0
pending stronger primary evidence for the original source/header/value context.


The next complete canonical match refresh, `camera-vector-match-28`, committed
r6221 with 957 byte matches and one instruction match. It proves both HUD
functions at 0x406cf0 and 0x407100 after their obsolete secondary-target
cdecl decorations were corrected to the existing fastcall symbols and the
registration synchronized at r6220. WSI-20260910-013 was resolved at issue
revision4158. The serial first divergence remains 0x4059a0, 752 bytes with
four native x87 operand differences; classification does not advance the stage.

The independent scalar raw review has a confirmed submission and completed
answer in `.devspace/runs/2026-09-10T21-43-43-376Z-466b6177-6fe7-446e-a631-8bdd22356885-chatgpt-call/`.
The first transport attempt failed composer verification before submission;
the retry used a short prompt and uploaded the full request as a file. Pro
rejected the 15-byte nonempty-exit assembly model because it cannot establish
compiler ownership of the maximum in ST0. The balanced six-instruction
candidate also improperly stores/rounds that maximum and produced 752/431.
Pro permitted one diagnostic of the nine-byte minimum-only island, leaving
the maximum as C++. `camera-minimum-island-review-20260910-20` produced752/355,
with chase1552/exact. Thus this raw path is closed pending new primary source
or TU evidence. No scalar assembly, allowlist entry, or match annotation was
added. The unused-assignment exact oracle remains ineligible.

The six nearby MFC dialog handlers expose deterministic ordinal imports that
the native-import verifier previously supported only for named MSVCRT calls.
The bounded extension proves MFC42.LIB long import members against immutable
retail: both .idata$4 and .idata$5 must contain the exact high-bit ordinal word,
with no lookup relocation or name section, and the code must relocate to the
exact imported-address symbol with the MFC42 descriptor dependency. Existing
named MSVCRT proof output remains unchanged; unrelated DLLs and import forms
remain rejected. Retail UpdateData is ordinal6334 and OnOK is ordinal4853,
each independently confirmed in the canonical VC5SP3 library. This registers
only source-call dependencies and accepts no provider owner, bytes or padding.
The focused COFF proof kernel passed21tests; doctor11 passed all seven gates.
The changed module is outside both governed call-contract component sets;
no call-contract proof semantics or generation was changed.


Fourteen reviewed native MFC call bindings were committed serially at
r6222-r6235 for all eight dialog handlers, including CWnd::Default ordinal2379
at retail0x4c5b82. Its protected canonical declaration is `?Default@CWnd@@IAEJXZ`;
the initial public-decoration diagnostic was correctly rejected by the import
proof. Current retail-only relocation audits pass for OnKillFocusValue0 and
SetGameControlOptions. The latter now derives the registered aggregate member
addend60 instead of incorrectly treating that member as the containing object.

WSI-20260910-014 records29 obsolete secondary function registrations across
21 current-block functions. Only the exact failed secondary symbols were changed;
primary source and ABI remain unchanged. A no-target synchronization dry run
included unrelated registration drift, so it was not applied. Every affected
target was instead dry-run individually and reviewed: no new targets, no order
invalidations, and no provider or source acceptance. Three target snapshots
already matched their corrected manifests. The remaining eighteen are being
synchronized serially.

WSI-20260910-015 records the dropped aggregate `object_offset` in relocation
identity derivation. The correction retains the retail field interval while
adding its registered object offset to both exact and interior references.
It rejects conflicting offsets, negative/noninteger offsets, nonzero function
offsets, and incompatible extra identity provenance. Generic tests cover both
valid aggregate/base overlap and ambiguous conflicting-base identities.
The focused COFF module passes21tests. Doctor12 passed its first five gates
but reported737 stale function annotations after the semantic binding changes;
those mirrors require the planned complete live match refresh, not manual
annotation edits or acceptance from the retail-only audits.


Complete refresh29 committedr6254: 1018 byte matches plus one instruction match,
61 newly matched groups, zero lost matches. All eight MFC dialog handlers pass;
the aggregate offset correction clears many option accessors. WSI-20260910-015
was resolved at issue4161 on that fresh proof. The call-contract stage remained
current and the scheduler still selected authored-byte at0x4059a0.

The newly unblocked option accessors exposed74 further obsolete secondary
symbol occurrences over28functions, including three getters whose body fixes
are being tested. They were corrected against current primary definitions.
The40bf00 destructor secondary target also incorrectly used zui.cpp; its exact
current definition is in hud.cpp. It now has an explicit hud.cpp TU entry in
that target. No production definition moved, and the dry runs invalidate no
accepted order facts. Unsupported inventory-only schema spellings were rejected
before mutation and replaced with the existing ordinary TU-entry form. The eleven
changed target snapshots were then synchronized at r6255-r6265; two already
matched. WSI-20260910-014 remains open until the next complete proof.

Retail407e80,407eb0 and407ef0 use logical right shifts. The signed C++ flag
expressions emitted arithmetic shifts. Both an unsigned flag-type diagnostic
and a scoped unsigned-value diagnostic produce exact16/16/32-byte bodies.
Only the latter was applied: three explicit unsigned interpretations at the
bit-extraction expressions, with no public signature, storage type or header
change. Source line counts are preserved; full live acceptance is pending.

Retail407680 reloads the parent rule-list and selected node after the condition
call. The current source cached both pointers across that call, producing96bytes
instead of112. The bounded direct-member reload diagnostic produces112/zero
nonrelocation differences. Those named-node accesses were applied, preserving
source line count and the existing three calls. Full live proof is pending.

The side-camera independent review at
`.devspace/runs/2026-09-10T22-08-55-128Z-59e7cfc7-59c1-47eb-9d33-3203cd2429d6-chatgpt-call/assistant.md`
confirmed three allocation/scheduling clusters and prohibited raw call-preparation
or instruction fallback. Its one established-primitive diagnostic regressed to
768/339; no part was applied. Primary follow-up evidence includes the complete
160-byte matching sibling406470 and its live match29 relocation/linked proof,
full Select/BuildPick callee assembly, and all seven indexed callers of each
callee. Both xref censuses have complete BN coverage. The sibling demonstrates
retail's late ECX scene load and scalar output-home ordering without an aggregate.
The follow-up also corrects Pro's mistaken inference that current reconstruction
mangled names were original retail names: authored retail names/prototypes remain
provisional. The follow-up review is pending in the same room.

### Subsequent live proof and ordinary HUD source corrections

The side-camera follow-up completed in
`.devspace/runs/2026-09-10T22-33-59-910Z-f2fbe49c-211c-4825-b632-68e17d8d0a9d-chatgpt-call/assistant.md`.
It withdrew the claim that reconstruction COFF names establish original retail
prototypes. The complete caller census, callee output writes and exact sibling
strengthen the direct-call/separate-scalar model but select no additional source
change that removes the remaining scheduling and stack-coloring differences.
The 736/32 shadow remains diagnostic only. No new assembly, allowlist entry or
instruction-match approval was applied.

Explicit hardware/software branches reproduce the retail stores and loads for
effects, object LOD, texture memory, graphics flags, HUD visibility and HUD type.
The effects setter uses a switch. The LOD setter uses a default clip-distance
local before the camera lookup and one common call after its switch; VC5
duplicates that call into the retail branch paths. These are ordinary C++ source
changes, with no raw offsets or new assembly. Four additional secondary targets
were individually dry-run/reviewed and synchronized at r6266-r6269; their17
corrected occurrences retain the actual no-argument fastcall definitions.

Complete refresh30 committed r6270 with1058 byte matches and one instruction
match:40 gains and zero losses relative to refresh29. All14 newly changed option
and profile bodies passed full relocation and normalized linked-body proof.
The corrected40bf00 secondary destructor registration also passed. WSI-20260910-014
was resolved at issue revision4162. These matches do not advance the serial
stage: the first-person camera still has its four x87 memory-leaf differences,
and198 functions in the current physical group remain unmatched.

The first focused call-contract retry encountered a BN view-change before any
compilation. Strict preflight authenticated the current Recoil database again.
Retry36 then passed all160 bodies of slice0x401000-0x408220 with no divergences
and no source drift. This was verification only, not a replacement stage closeout.

Six further ordinary C++ changes are applied pending the next complete proof:
the camera-section setter removes absent retail FOV-output initializers;
the getter uses a null-initialized return value and conditional assignment;
the HUD-visibility setter uses separate hardware/software branches;
controls OnDeactivate reloads m_dialog before disabling it;
controls OnResume reloads m_dialog across calls and passes window queries
directly to the final surface operation; and SetPlayerName uses strcpy for the
fitting path and reloads option-buffer fields after strncpy. Isolated full-TU
diagnostics reproduce their96/32/32/144/80/112-byte bodies respectively, with
zero nonrelocation differences. No annotations were manually added from these
diagnostics.

Refresh31 committed r6274 with1065 byte matches and one instruction match:
seven gains and zero losses. The six pending C++ fixes passed complete proof,
as did ReadScalarValueAsInt after its reviewed native _ftol tail binding.
Three native _ftol references were added at r6271-r6273; the other two callers
still have additional literal/data dependencies. Both affected call-contract
slices passed (320 bodies total) in retries37a/37b. Doctor13 passed all seven
infrastructure gates. A fresh canonical playground-only build deployed
`playground/Recoil-rebuild.exe` at2026-09-11 01:06 local time,1242624 bytes,
from `build/live-validation/playground/hud-native-fixes-20260911-05`.
No gameplay outcome was inferred from that deployment.

Subsequent diagnostic source work found an exact64-byte scoreboard comparator:
expressing the player-key tie break as A > B rather than B < A gives the retail
register allocation. Key reassignment, tie-first, separate tie locals and a
conditional expression retain three differences; no instruction fallback or
Pro eligibility review was needed. The successful comparison spelling is
applied. Credits UpdateAll changes its1.0f comparison to1.0 to match retail's
qword floating comparison (144/zero). The layout-delay helper uses its nonzero
branch first (32/zero). Direct global stack accesses match both top-message
and chat dispatchers (32/zero each). These five changes await the next fresh
complete proof; no match tags were manually added.

Native delete alone, direct-global positive-branch delete, and local-pointer
delete all retain six differences in DestroySensorWindow. The differing null
check/register-copy schedule is not an instruction match. Those variants are
diagnostic only and are not applied. The current group's full-TU diagnostic
inventory found101 of195 inspected unmatched HUD bodies equal outside candidate
relocations; those results do not prove relocation identity or linked bodies.

The HUD-type toggle contained a behavior error: standard was returned unchanged,
and unrecognized types were changed to perspective. Retail toggles standard and
perspective and re-reads other values unchanged. The corrected C++ switch matches
all48 bytes. CanSaveGame's combined player-state/block condition also matches48
bytes. Both are applied pending live refresh32.

ScreenToWorld's combined Y expression generated subtraction of a negative half
constant. That is an algebraically valid compiler transformation, not an observed
sign error. A source-level scaledY intermediate gives retail's positive shared
half constant and final addition, reproducing80bytes exactly. Reordering addition
alone and removing redundant casts did not help; changing the surrounding branch
did not help. Only the scaledY form was applied. Its four-byte physical half
dependency was created and reviewed at r6275 under the existing function owner,
with both immutable operand sites, complete BN reader census and a static VC5
temporary witness requirement. Original literal/name/ordinal and storage/linkage
provenance remain unresolved; the governed creation resets prior accepted owner
gates to pending rather than silently accepting the enlarged owner.

After the three initial _ftol bindings,36 further exact references across13
authored parents were individually dry-run, reviewed against immutable retail
and the canonical long import member, then applied serially at r6276-r6311.
All retain the existing unowned non-authored thunk and its unresolved body/padding
and storage. The credits binary64 unit constant at4cd348..4cd350 was likewise
created at r6312 for the existing credits owner (both readers already belong to
that owner), then its UpdateAll reference was bound at r6313. Its only indexed
readers are409380 and409470, with no data references. Owner gates remain pending,
and fresh function/relocation/linked proof is required.

Retail412c10 explicitly returns1 on every path. Changing the existing
HudLayoutBase::LoadTypeIFromZarRoot declaration from void to int and returning1
matches its80-byte body. The corresponding source/header correction is applied;
existing registration selectors already cover the corrected decorated return
type. No original method spelling is inferred from those selectors. A timer-write
callback experiment copying elapsedSeconds to a float local improves32bytes from
21 to8 differences, but remains diagnostic only; the residual is an integer-copy
versus x87-transfer difference. The volume widget40cc30 presents a related x87
value-transfer/source-accessor uncertainty and has an independent Pro review in
progress. No new inline accessor or raw assembly has been applied.

Refresh32 and focused call-contract slices2/3 are running from fresh roots after
these changes. The latest deployed executable remains the earlier fully built
playground05 image; subsequent source changes are not yet in that executable.


### Refresh 32, shared fill-widget accessor, and verifier repair

Full current-source refresh32 committed r6314 with1072 byte and1 instruction
matches, seven gains and no losses from refresh31. New matches:409470,40d220,
40e910,413600,4138d0,4138f0,414b90. Camera4059a0 remains752 bytes with its first
x87 operand difference at+286; no camera assembly or instruction fallback was added.
The fresh second call slice passed160/160. The third exposed a verifier defect:
the zero-float compiler-local bridge pinned COFF section4 offset374, while fresh
VC5 had the same selected static zero at39c. WSI-20260910-016 records the defect.
The fix validates the relocation-selected local scalar by its temporary spelling,
read-only section, valid defined coordinates, storage/type/aux metadata and exact
bytes. It retains relocation type, zero addend, operand mask, uniqueness and
external-population exclusions. Generic adversarial tests pass:144 cases in the
focused call-contract evidence module. Verifier generation164 requires all call
contracts to be replayed; the full serial replay is running. Doctor14 passed its
first five gates and correctly rejected stale match annotations after this
verifier change; a fresh match refresh and doctor rerun remain necessary.

The independent Pro accessor review completed with a valid submission receipt:
.devspace/runs/2026-09-10T23-20-06-113Z-ab0513b0-d6de-4786-a07b-8752e49ee79d-chatgpt-call/assistant.md.
It allowed one bounded inline GetNormalizedValue experiment on HudUiFillBitmap,
used by both sound and music readers. The complete current indexed BN field census
(fill-widget-value-xrefs39.json) contains those two reads and three writes, with
duplicate typed records at two writes; it is complete for current indexed
analysis, not proof of every untyped possible access. The full-TU experiment
hud-volume-value-accessor-20260911-01/getter yields exact48-byte sound and music
bodies and no emitted GetNormalizedValue symbol. The40cc10..40cd30 neighborhood
has unchanged bodies except the intended sound fix. Three other currently
unmatched bodies change code allocation:EnsureHudLoaded,UpdateObjectiveDirtyRect,
and SortRange; they are not silently accepted. Shared getter and both uses are
now production changes, pending a fresh whole-program proof and loss check.
The name is explicitly inferred; no original spelling or owner tier is asserted.

ScreenToWorld now has exact opcode bytes, but its physical half-literal witness
fails because the candidate pools additional readers from the same TU. The
verifier correctly refuses an unproved reader universe; no proof relaxation or
invented constant declaration was used to conceal it.


The compiler-local repair belongs to both governed component sets, so the final
coordinates are verifier164/expected-facts106. The initial164/105 replay was
stopped before any acceptance commit. The refreshed dry-run plans the full3292
body census under both corrected coordinates. Refresh33 was stopped before
acceptance to repair two joined source lines in the source-transform output;
refresh34 is the fresh canonical proof for the final formatted source.

Additional native C++ fixes:40c9e0 and40ca40 use mutable graphics flags, update
them in the checkbox branches, then call the common setter (and perspective
span selector). Full-TU diagnostic graphics-toggle-flags-20260911-01 is exact64
bytes for each; direct setter expressions did not match and were not adopted.
Music SyncFromOptions40ccc0 now leaves the two output locals uninitialized as
retail does; music-volume-locals-20260911-01 is exact64 bytes. The callee can
return failure without writing outputs, and retail ignores that return. This
reconstructs the retail path; it is not a new robustness guarantee for mixer
failure. All three changes await the final live full-match and call proofs.


Refresh34 committed r6315 with1071 byte/1 instruction: sound40cc30 gained, but
AINet4026d0 and FMV462ee0 lost exactness through header-driven x87 allocation.
The full-program negative result overrides the narrow getter success. Reverted
the accessor and both uses completely; do not accept this source experiment or
move/qualify/rename it to steer allocation. Its diagnostic evidence is retained.

Call40c passed the repaired first zero-local check, then exposed the duplicate
fixed-object-coordinate check later in the same bridge. The completed repair
also removes section ordinal/offset from the exact named-scalar check and the
final two-scalar population check. Exact semantic name selection, private suffix
pattern, relocation population/type/addends/masks, two-object census, exact
bytes, static storage and read-only section remain required. Generation165/107
owns the completed repair. Focused generic scalar tests cover temporary and
named-private storage witnesses independently of object placement. Refresh35
and fresh call41c are running for the final source and verifier; no call-contract
replay or closeout has yet been accepted under the new generations.


Refresh35 committed r6316:1072 byte/1 instruction matches, exactly the same
matched set as refresh32. Both getter-induced losses are repaired and sound
remains unmatched. The three additional scalar/branch fixes have diagnostic
body equality but still require callee/storage identities before classification.

The next call failure was a retired target schema lookup in EnableTopAndChatStacks.
WSI-20260910-017 records it. The verifier now checks the exact current ordered
registered_addresses population against canonical typed function rows; it fails
on missing, duplicate, reversed, extra, wrong-kind, wrong-binary or wrong-address
entries. Existing source/manifest/function-registration checks remain exact.
Focused evidence module:145 tests pass. Fresh hud-source-call42c verifies the
full160-body third slice with zero divergences and no source drift under final
generations166/108. No completed stage is inferred from that slice pass.

Reviewed secondary registration corrections cover28 stale symbol occurrences
for13 already modeled functions (hud-secondary-name-fixes42-plan.json). This
includes native constructors/destructors, virtual callback qualifiers and the
already recovered HudUiBar argument. Two destructor registrations also now
select their actual existing zui_widgets.cpp definition. No production TU or
source owner changed. Full-order and authored-order secondary files retain
exact addresses and gate flags; edits do not accept any order result. Sync42
committed r6317 for existing targets only, with no new target, block or symbol
acceptance. Two already present but unregistered secondary manifest files were
corrected on disk without enrolling them in tracker authority.

The complete21-slice/3292-body call replay is running under166/108 in
call-contract/401000-408220-r6318-replay-001. Required fresh closeout, subsequent
match refresh and doctor remain outstanding.


The completed focused third-slice check validated both tool repairs, and
WSI-20260910-016/-017 are resolved at issue-ledger r4166. The complete compact
proof kernel passed250 tests in24.78s. New scalar and target-population
adversarial cases were consolidated into the existing identity test group,
retaining all assertions and the mandated250-case cap.

While the full replay remains source-frozen, diagnostic-only full-TU probes
found three additional exact source candidates:RecoilStateControls destructor
408d90 (clear m_dialog inside the nonnull deletion branch), kill-message414330
(nonnull entry branch first), and main-menu-transition constructor415170
(assign its four retail initialized fields in the constructor body and leave
the saved half-resolution field to activation, matching retail's absent store).
All are exact outside relocation fields in
hud-lifetime-message-shapes-20260911-01; none has yet been applied or accepted.
The timer UpdateHMS parameter-reuse cleanup still has19 opcode differences,
including MOV versus x87 field copy, and remains unadopted diagnostic evidence.


Three more diagnostic source candidates in video-caps-objective-20260911-01
match all opcode bytes:40c370(880),4117f0(192),411a20(160). Video capabilities
currently use enum values1,2,3,4,5, but retail writes100h,200h,300h,500h,600h.
The sole indexed consumer RecoilApp::InitInstance compares against600h at42e796
(video-capability-consumer-retail43.json), independently corroborating the enum
correction. The two objective fixes move the fill-animation enabled reset after
the integer conversion/subtraction, and clear auto-hide delay on every remaining
non-chat phase path instead of only phase1. They remain shadow-only until the
active whole-census replay finishes. Applying the six pending native candidates
will require fresh closeout and whole-program byte/loss checks.


The cumulative HUD experiment hud-six-source-fixes-20260911-01 still matches
all six selected bodies. Its full HUD opcode inventory changes only those six
previously unmatched functions (hud-six-source-fixes43-collateral.json).
A separate full zui_widgets.cpp experiment replaces the fill-widget setter's
two memcpy calls and temporary integer with normalizedValue=value. It preserves
all32 retail body bytes outside relocation operands at4ba3c0, and changes no
function's instruction bytes anywhere in that TU (fill-value-assignment44-collateral.json).
This seventh, source-faithful cleanup is also held until the replay completes.
The runtime source/header inputs have remained stable throughout the replay.

The complete replay subsequently passed all 3,292 bodies in 21 slices with no
divergences under generations 166/108, committing transaction revision 6338
(semantic/evidence generation revisions 6335/6335). The seven tested native
source fixes above were then applied using guarded replacements. The required
fresh closeout is running in call-contract-closeout/all-r6339 with eight
read-only slice verifiers and one subsequent canonical whole-program
compile/resource/linkability diagnostic. No byte-stage completion is inferred
from replay or the diagnostic opcode comparisons. Fresh full match refresh,
collateral match comparison and doctor are still required after closeout.

Closeout passed all 21 fresh slices / 3,292 bodies and its required canonical
resource/linkability build, committing r6339 (semantic/evidence 6336/6336).
The scheduler returned to authored-byte-match, cursor 0x4059a0. Exact report:
build/diagnostics/call-closeout-generations166-108.json.

Reviewed stable-name relocation bindings then registered the two music-volume
binary32 constants at 4ce224 and 4ce228, committing r6340/r6341. Immutable
retail operands, four-byte float declarations and complete current indexed
reader censuses support these separate objects; each has one reader in the
existing options owner. The source's existing named internal constants use
their stable stems, not VC5 $S suffixes. No original spelling, storage/linkage
acceptance or owner gate is asserted. Fresh named-static reader and linked
proofs remain mandatory. See music-binding45-*-dry/apply.json.

Three further native fixes were applied after closeout: CanLoadGame414b60
uses the nonnull path first; IsLocalPlayerFirstEntry40ea60 uses entries.empty()
instead of manually reproducing vector count arithmetic; timer Update40ed20
selects the complete scaled time-step product in its conditional expression.
The combined three-native-fixes-20260911-01 diagnostic is exact48/80/96 bytes
outside relocation operands. It also changes code allocation in three
currently unmatched HUD bodies (EnsureHudLoaded, UpdateObjectiveDirtyRect and
SortRange); no acceptance is inferred for them. Full current-source refresh36
is running from r6341 for these changes, the seven preceding fixes, updated
secondary identities and generations166/108. No new raw assembly was added.

One additional shadow-only diagnostic, insertion-predicate-20260911-01,
matches InsertPivotIntoSortedPrefix414930 at80 bytes by testing CompareSortKey
directly instead of converting its result through the local bool wrapper.
It remains unapplied while refresh36 runs.

Refresh36 committed r6342:1,087 byte and1 instruction matches, 15 gains and
zero losses against refresh35. Gains:40ba30,40bab0,40bb00,40bb50,40c9e0,
40ca40,40caa0,40cbb0,40cc80,40ccc0,40cd00,40ea60,40ed20,414b60,4b7290.
Both stable music constant bindings passed complete byte/reader/linked proofs.
Doctor15 passed all seven infrastructure gates. Fresh third-slice call45c
passed160 bodies with zero divergences and no source drift. The direct
CompareSortKey insertion predicate was then applied, and refresh37 is running.

HUD manager extent investigation46 finds an independent lower and upper bound:
StaticInit40d410 passes4e5ed0 as this to constructor40d7e0; that constructor
preserves this in ESI and constructs its tail member at+7704. Its dword store
at40d99f writes tail+13c, i.e. manager+7840 through+7844. Independent CString
initializer40d1f0 passes4ed714 (manager+7844) to the MFC default constructor.
These retail instructions support the manager interval[4e5ed0,4ed714), rather
than deriving it from the candidate union or BN's current type size. Captures:
manager_static_init46,manager_ctor_head46,manager_ctor_tail46,
triplet_string_ctor46.json under build/diagnostics.

Registration is held: the complete current data-row inventory also contains
standalone physical identities4ea654 (timerPanel, offset4784) and4ed4e0
(statsList, offset7610), both with unknown extents and independent storage
contribution ids. Their identities must be reconciled as aggregate fields
before introducing an overlapping physical interval. The existing coalescence
route requires standalone fields, one owner, full coverage and data-only
targets; it cannot be assumed to cover this existing aggregate plus mixed
function/data target case. No extent, field alias, owner gate or storage
acceptance has been applied for this investigation.

Refresh37 completed comparison but rejected mixed CRLF/LF source input before
acceptance. The one-line insertion edit was normalized back to CRLF without
changing line counts. Fresh refresh38 then committed r6343:1,088 byte and1
instruction matches, exactly one gain (414930) and zero losses against36.
The source-level camera comparison still has precisely four operand-byte
differences at+286,+289,+295,+298 (camera-current46.json): retail loads each
bound first and multiplies by the scale, while VC5 loads the scale first and
multiplies by the bound. No GPR-only fallback or scalar assembly is eligible.

The complete bounded manager-data context and existing coalescence command were
prepared for a new Pro invocation using alias hud-manager-data-identity-review;
submission/response receipt validation remains pending. The prompt explicitly
asks for criticism of the extent proof, necessary member-vs-global evidence,
and a guarded existing-aggregate migration design; it grants no data or owner
acceptance. A supplementary diagnostic linear text scan found no direct memory
operands with4784/7610 displacement; this absence is not a complete decoder or
member-ownership proof. Playground06 is building the accumulated source fixes.

Playground06 completed and deployed playground/Recoil-rebuild.exe at local
2026-09-11 03:05:06 (1,242,624 bytes). Compilation, resources and link passed;
the game was not launched or play-tested. Direct authored acceptance46 then
rebuilt current source in authored/4059a0-r6344, re-proved the existing prefix,
and diverged at4059a0. It committed no groups and left revision6343 unchanged.
The authored-byte stage is not complete.

Pro review46 completed with confirmed submission, verified GPT-5.6 Sol/Pro,
one successful context upload, and verified transcript integrity. Exact run:
.devspace/runs/2026-09-11T01-00-42-789Z-67ae5877-8927-4fb9-89ef-69d5438c217d-chatgpt-call/.
Pro supports the constructed interval only under an explicit non-overlap
premise; it does not establish original sizeof/global declaration or prove
the two pointers were source members. It recommends a separate neutral
existing-aggregate/interior-identity migration, preserving semantic owners,
unknown source membership, history and parent storage order. The current
coalescence route must retain its existing refusal guards.

The review also corrected the prompt's inaccurate description of current
manifests as data_slices: the timer and local-player stats targets still
declare standalone data_symbols. The SetScaleAndRebuild manifest is now
function-only, while its stored association to the stats physical row remains.
Existing object_offset support is independently demonstrated by
zopt_current_hw_mode_hud_globals.json; that support does not repair these HUD
registrations or prove the proposed semantic model by itself. No migration,
neutral-view schema change or target correction was applied here.

Additional retail evidence47 strengthens the non-overlap premise: the CString
has its own CRT initializer at40d1e0 (slot4da01c), its own atexit registration
at40d200 to40d210, and the latter passes4ed714 to the MFC CString destructor.
The manager initializer occupies slot4da028 and registers its distinct
destructor at40d420. The default CString thunk4c5ba0 jumps through4cc3c0,
MFC42 ordinal540. Timer GetSeconds40ed10 and stats wrapper4143a0 load exactly
four bytes at the respective interior addresses and dereference those values,
establishing pointer access widths, not original member declarations.
Captures are timer-pointer-access47,stats-pointer-access47,
triplet-string-init47,triplet-string-atexit47,triplet-string-dtor47,
cstring-default-thunk47 and manager-atexit47.json under build/diagnostics.

Continuation48 applied one native source correction: resolution selector
SyncFromOptions40cd30 now follows retail's accelerated-then-software branches,
uses the existing video-mode enum cases in selector order, and does not fall
through to a second mode read for an unrecognized software mode. The complete
336-byte shadow object contribution matches outside relocation operands.
Timer save callback40fb90 experiments confirm a missing local-copy source
shape, but ordinary float initialization/assignment/const variants still use
integer copies where retail uses FLD/FSTP. Those variants remain unapplied.
No raw assembly or instruction fallback was added.

WSI-20260911-001 records a reproduced byte-verifier composition defect: a
same-COMDAT switch label was correctly normalized to its containing function,
then rejected by a registered selector because the raw label spelling differed
from that function. The correction re-proves local containment and addend in
the identical freshly loaded COFF object before composing the selector proof.
Cross-object, cross-section, non-label, non-COMDAT, out-of-body, wrong-function,
wrong-kind, truncated-operand and changed-addend cases continue to fail.
The generic regression failed before the fix; all250 proof-kernel cases pass
afterward. No call-contract or expected-fact implementation was changed;
live_byte_verify.py is an explicit match-proof dependency. Doctor48 stopped
at stale match annotations as expected pending the fresh complete match scan.

Pro follow-up48 confirmed the narrower current-candidate binding correction
without accepting a physical storage model. Receipt and transcript integrity
passed; submission confirmed, GPT-5.6 Sol/Pro verified, no new attachment:
.devspace/runs/2026-09-11T01-28-29-065Z-82ce53ff-ed83-4b25-86ba-d5200a27650b-chatgpt-call/.
The existing timer data-only target now locates both four-byte pointer access
slices in the actual zui_widgets.cpp candidate definition, at offsets4784 and
7610 from the g_HudUiMgr COFF symbol. Stats function4143a0 remains in its
unchanged hud.cpp function target; its data entry moved to the data-only
target. The already function-only SetScaleAndRebuild target was synchronized.
This avoids pretending that an auxiliary source_files entry changes the
primary definition source understood by the current byte binding loader.

Dry-run and reviewed apply committed r6345. Exact before/after census checks
confirmed all matched owner, storage and section records unchanged, all three
physical extents still unknown with no size/end, and no new parent target
association. Only the stats row's stale reverse target associations changed.
These are candidate symbol-plus-addend bindings, not historical COFF spelling,
member declarations, physical extent, allocation, or owner/storage acceptance.
Pro's requested exact linked addresses remain a later separate gate under
AGENTS; authored-byte proof still requires linked identity and normalized body.

An independent registration correction committed r6344 first: the two Recoil
interpreter literals now bind to their existing HUD-TU static definitions in
recoil_interp_data; the interpreter-family target retains its other data and
uses the current CRecoilInterp global object's actual decorated type name.
No production declaration or ownership changed. The obsolete registrations
and exact mutation outputs are retained under build/diagnostics/*48*.
Fresh full match refresh49 is running from r6345; no result is accepted here.

Refresh49 completed and committed r6346:1,101 byte matches plus1 instruction,
13 gains and zero losses from refresh38. The gains are40cd30,40ce80,40eab0,
40eae0,40eca0,40ecc0,40ece0,40ed10,414390,4143a0,4143b0,4143c0,414ab0.
Both switch functions passed complete relocation and normalized linked-body
proof after the verifier correction. The HUD pointer accessors and interpreter
constructor passed with corrected candidate data identities, without accepting
physical data extents or owner gates.

The existing hud_layout_typed_globals_data manifest already declares TYPEI at
4dae0c and TYPEII at4dae14 in their current zui_widgets.cpp definition TU.
Their tracker associations still pointed only at the now-vtable-only
hud_layout_class_data target. After direct TYPEI bytes and complete indexed
reader inspection, dry-run/review/apply synchronized those two existing
manifests at r6347; no file or physical extent changed. Refresh50 committed
r6348:1,102 byte matches plus1 instruction, one additional gain412c10 and no
losses. The remaining2,189 authored bodies have no current matching proof.
The serial first divergence is still4059a0. Neither refresh advanced the stage.

Doctor50 passed all seven infrastructure gates after refreshed proof restored
annotation currentness. WSI-20260911-001 was resolved through reviewed dry-run
and apply at issue revision4168. Call replay50 correctly refused because the
authored-call-contract phase is already complete; no reconstruction mutation
occurred from that attempt. A fresh second-slice call check and explicit
playground-only build are running for the final source state. These checks do
not replace the pending direct authored-byte stage acceptance.

Final batch validation50/51: the fresh second call-contract slice
[408230,40e070] passed all160 bodies with zero divergences and no source drift.
Playground-only build50 compiled/resources/linked successfully, passed authored
linked presence and deployed playground/Recoil-rebuild.exe (1,242,624 bytes,
local2026-09-11 03:53:16). It was not launched or play-tested.

Direct authored-byte acceptance51 freshly rebuilt current production source,
passed all14 source-policy checks, and re-proved90 groups before failing at
4059a0. No groups were committed, no mutation was planned, revision6348 stayed
unchanged. A diagnostic read of that same fresh camera object independently
confirms extent752 and precisely four nonrelocation differences at offsets286,
289,295,298: retail loads the minimum/maximum bound then multiplies by the
scale; candidate loads the scale then multiplies by the bound. This is not
general-purpose register reassignment and is outside the instruction fallback.
The existing Pro-reviewed native/source and narrowly permitted raw-minimum
experiments leave no supported next variant without new primary evidence for
an authentic source/TU construct. No scalar assembly, fake local/dead source,
new exception, match annotation, or stage acceptance was introduced to hide it.

Current result: authored-call-contract remains complete; authored-byte-match
remains incomplete at4059a0. Fresh complete function evidence is1,102 byte plus
1 instruction of3,292 authored bodies, with2,189 unmatched. The14 gains from
refresh38 through50 and the deployable executable are valid partial progress,
not completion of the requested authored-byte stage. Final artifacts:
build/diagnostics/authored-byte-current51.json,
build/diagnostics/camera-current51.json,
build/diagnostics/hud-resolution-call50.json,
build/diagnostics/stages-playground-hud-bindings50.log,
build/diagnostics/doctor50.log,
build/diagnostics/proof-kernel-validation48.log.
