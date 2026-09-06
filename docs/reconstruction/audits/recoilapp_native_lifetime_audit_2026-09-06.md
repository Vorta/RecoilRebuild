# RecoilApp native lifetime and call-contract audit — 2026-09-06

## Scope and evidence

This audit covers the application/frame and save/load UI contribution in
`src/Battlesport/RecoilApp.cpp`, with the existing `CZGameFrame` title interface.
Westwood Online behavior remains deferred. Current retail assembly in the
authenticated, already-open Recoil database and fresh governed VC5 output are
the comparison authorities; old names and comments are navigation only.

## Native title and coordinate corrections

The title interface returns `CString` by value, not a pointer to explicitly
constructed caller storage. Both native implementations reproduce their retail
instruction bodies outside COFF relocation fields: `CZRecoilFrame` at
`0x4306f0` and `CZGameFrame` at `0x4438c0`. The frame constructor now uses normal
temporary lifetime and a local formatted title, and calls the existing MFC
menu interface directly. No wrapper or manual destructor controls that lifetime.

The remote-player label position body at `0x432d60` has three physical floating
conversion calls, with distinct doubled-coordinate and ordinary-coordinate
branches. Removing unsupported zero initialization of its output vector and
recovering those branches reproduces the retail instruction body outside
relocation fields.

Fresh live authored order accepted the 218-body RecoilApp sequence at tracker
revision 5290 and the 18-body base-frame sequence at revision 5292. These order
acceptances do not accept bytes, linked layout, or the whole source owner.

The frame constructor at `0x430250` still has object-byte differences. Among
them, the four `GetSystemMetrics` arguments are evaluated in a different order;
retail calls use 4, 15, 33, 32. The remaining constructor differences have not
been waived. The retained WOL registry behavior has not been changed.

## Verifier correction (WSI-20260906-013)

Verifier generation 101 removes four obsolete candidate projections that could
rewrite the physical call census: leave-network virtual-to-direct shutdown,
duplicated branch conversion, reordered/deleted constructor lifetime calls,
and panel-destructor replacement with an array helper. Actual calls must now
agree without those transformations. Independent stack-cleanup and provider
identity proofs are retained.

The focused generic proof kernel passes all 63 cases, including adversarial
missing/extra/reordered calls, call versus tail transfer, direct versus indirect
dispatch, target identity, and cleanup. This is infrastructure validation, not
acceptance of application source.

## Application singleton investigation

Retail `0x42f9d0` calls `RecoilApp::ShutdownEngine` directly with the singleton
address `0x4f3ca8`. The current source still wraps that object in aligned byte
storage and obtains a reference through a cast. Retail startup independently
shows a native lifecycle pattern: `0x42de20` invokes initialization at
`0x42de30`, then registration at `0x42de40`; initialization tail-calls the real
constructor at `0x42dfa0`, and registered cleanup `0x42de50` tail-calls the real
destructor at `0x42de60`. Recovering native storage is under investigation;
these observations alone do not accept storage or final-image facts.

The native global definition is now retained. Fresh
`recoilapp-native-singleton-order-01` reproduces all four lifecycle instruction
bodies and `0x42f9d0` outside relocation fields. The latter now emits the exact
direct shutdown call naturally. The global has 0x228 bytes of zero-initialized
storage; its old cast macro, raw storage union, explicit initializer and atexit
functions, and manual application CRT row are removed. The separate save/load
singleton remains unchanged. Native lifecycle selectors are included as
non-gating authored-lifecycle rows for eventual full-order coverage.

Tracker transaction 5294 attaches the application data and four emitted
functions to the real singleton source anchor without accepting data extent,
owner, bytes, or tiers. Transaction 5296 freshly accepts all 218 authored-order
bodies with this source and lifecycle inventory. Transaction 5297 updates the
existing data target to the native class symbol.

Generation 102 also removes the obsolete shifted-position exception for the
frame's texture-pack getter. The fresh application census passes 217/218 bodies;
its sole divergence is the distinct renderer-count entrypoint described below.
The focused infrastructure module still passes all 63 cases.

## Renderer-count boundary follow-up

Retail directly calls `0x4a7470` from `0x43056e` and `0x4b3220`; the only
incoming code edge to its backend `0x4a9910` is the jump at `0x4a7470`. The
parallel DirectDraw count entrypoint `0x4a7480` is called from `0x43173c`, and
its jump is the sole code reference to backend `0x4a9900`. There are no data
references to these four entrypoints in the current BN census. Both public
entries are exactly five instruction bytes plus eleven NOP bytes. Both cached
leaves are distinct six-byte loads/returns with ten NOP bytes. The shared
hardware count query at `0x4b3220` uses a signed positive comparison.

The existing `non-authored/compiler-generated-thunk` classification for
`0x4a7470` has no evidence IDs and unresolved ownership/disposition. Its old BN
comment is not proof. The neighboring wrapper's old comment also claims a
source file using only a BN comment and cannot establish original ownership.

A ChatGPT Pro review (2026-09-06 11:15 UTC; five verified uploads and verified
transcript) favors an authored public/backend API pair while requiring direct
checks against the incremental-link hypothesis. Advice is not acceptance.

An independent two-TU VC5SP3 mechanism experiment uses ordinary argument-free
forwarding and backend functions, not Recoil source or a reconstruction test
target. Its COFF already contains the forwarder's E9 and a REL32 at +1. Clean
non-incremental linking retains that body with eleven NOP bytes. Incremental
linking instead creates an additional packed five-byte `@ILT` jump table at the
front of `.text`, redirects callers and the authored forwarder through it, and
surrounds contributions with INT3 reserve padding. A size-changing incremental
relink moves the backend and retargets the ILT entry, leaving the old backend
range as INT3; it does not turn the old backend into the isolated NOP-aligned
entry pattern observed in retail. The first immediate same-second relink did
not update the image; recompiling in a later clock tick confirmed the actual
size-changing relink. This experiment distinguishes compiler-emitted forwarding
from the proposed incremental mechanism; it cannot recover original identifier
spelling or exact historical source-owner boundaries.

The experiment lives under `build/advisory/vc5_forwarding_mechanism*` and its
fresh output root `build/live-validation/vc5-forwarding-mechanism-01`. No
diagnostic executable was launched or deployed.

The public accessor is now a normal out-of-line C++ definition in the adjacent
video API group. Its two callers no longer bypass it. The cached backend stays
unchanged and retains its separate physical identity. Transaction 5298 corrects
only the unsupported pipeline classification to authored/authored-body; 5299
registers the 40-body order census and invalidates its old order evidence; 5300
attaches the source definition. Historical spelling and exact owner boundaries
remain unaccepted. The source comments no longer present old BN comments as
original-file evidence.

Fresh live order at transaction 5302 accepts all 40 selected bodies. The
preflight first exposed a pre-existing disagreement for shutdown callback
`0x4a7520`: its manifest said authored-lifecycle but the tracker lacked that
role. Direct assembly at `0x4a75c1` pushes it immediately before the `atexit`
call, and its body tail-calls teardown at `0x4a91b0`; transaction 5301 aligns
the tracker with that explicit authored lifecycle role, without changing code.

The new accessor emits E9 plus one zero-addend REL32 at +1 to the cached getter,
followed by eleven NOPs. Its executable instruction bytes match outside that
relocation. The diagnostic with NOP trimming disabled reported eleven missing
BN bytes because assembly extraction returns only the five-byte instruction;
it did not compare retail padding. The ordinary instruction comparison passes.
No full-range padding or relocation-semantic byte acceptance is inferred.

Fresh generation-104 convergence checks pass all 218 application bodies and
all 20 video-option bodies, with no caller divergence or source changes during
validation. In particular, both renderer-count callers now reference the real
public API without target rewriting. These focused checks are nonaccepting;
the serial whole-stage replay and closeout remain required.

## Base-frame inline context

The default `/Ob0` compilation incorrectly retains bitmap-constructor and
handle-accessor calls absent from retail. Selecting the existing `/Ob1 /GX`
profile for `CZGameFrame.cpp` preserves 18/18 authored order and reproduces the
entire `0x443900` paint-handler instruction body outside relocations. The
constructor's three calls then agree in count, but it still has extra exception
handling state. Marking the constructor itself inline was tested and rejected:
VC5 removes the separately required constructor and inlines it into the factory.
The ordinary non-inline constructor is restored. Its byte mismatch remains
open; the successful paint comparison does not accept the whole frame owner.

Generation 103 removes the remaining renderer-count target rewrite, obsolete
frame ordinal shift, and frame constructor/destructor/paint call projections.
The old paint expected projection also inserted calls absent from retail; the
expected-fact generation advances with its removal. The focused 63-case proof
kernel passes. Fresh `call-contract-czgameframe-native-g103-01` passes all 18
actual frame call contracts with no divergences and no acceptance claim.

No authored-byte acceptance or playground deployment has been performed by
this audit.

## Message-box probe and loop-domain diagnostics

The native `HudUi::ShowMessageBox` at `0x438350` already matches every
instruction byte outside relocations. WSI-20260906-018 records the descriptor
error that nevertheless rejected it: candidate offsets defaulted to MOV at
+0 and CALL at +5, while both retail and current source use +0x0e and +0x1a
after EH setup. Generation 106 supplies those directly checked coordinates;
the frame size remains 0xb214 and all helper-body and relocation guards remain.
Fresh `call-contract-util-probe-g106-01` passes all four util bodies.

WSI-20260906-019 records a domain mismatch in the early raw count for Run at
`0x442d00`: 22 parent calls are compared against the whole COFF contribution's
28 calls, including six calls in the three separately catalogued catches.
Generation 107 invokes the existing exact associative-xdata/RTTI/COFF/CFG
funclet proof before the rejection-only raw census. The full candidate
inventory remains untouched and the later exact funclet partition still runs.
Generic tests retain rejection of extra parent calls and malformed indices.
Generation 108 also reconciles the existing Run profile with the independently
proved physical 13-entry forward switch map; its default edge remains a
separate conditional branch, not a fabricated fourteenth table entry. Fresh
`call-contract-appframe-case-map-g108-01` passes all 17 target bodies. The
65-case focused proof kernel and all seven infrastructure gates pass.
WSI-20260906-019 is resolved; this is not a six-call waiver or byte acceptance.

Separate source/byte observations for Run remain open: retail checks the
active-loop flag at +0xd0 before processing transitions or updating the state,
whereas current source checks it only at the final wait. Retail state suspend
and resume calls do not have the current source's extra null guards. The
conditional native-deque experiment is not selected by the production profile;
the current queue wrapper/layout also remains a source-model obligation. These
are not accepted or repaired merely by a parent/catch call-count partition.

The active-loop correction is now implemented directly from retail assembly:
the +0xd0 flag guards queue processing and state updates; the inactive arm only
peeks/waits. Suspend and resume use the retail unguarded receivers. Queued state
pointers are reloaded from the queue item across callbacks rather than cached
before those callbacks. No queue layout, owner or provider model changes here.
Fresh `appframe-active-loop-order-01` passes all 17 order identities.

The generation-122 current-source call scan
`call-contract-appframe-active-loop-g122-02` rejects an obsolete candidate-size
gate before ordinary comparison. WSI-20260906-035 records the retired
CException::Delete proof's fixed 0x430 extent and the later receiver projections
that require the old null guards and cached pointer. The new COMDAT is 0x410
including catch contributions and switch data; it is not instruction-exact.
The existing associative-EH catch partition already derives current extents
and remains required. The source correction is not live call or byte acceptance.

Generation 125/expected schema 70 removes the retired zero-effect Delete
profile and all five old ordinal-specific receiver projections, including
their fixed candidate COMDAT/switch profile. The current-coordinate EH
partition remains mandatory. All 92 focused call-contract cases and seven
infrastructure checks pass. The fresh
`call-contract-appframe-live-lineage-g125-01` scan now compares the actual 22
parent calls; its first divergence is ordinal 7's equivalent nested receiver
renderings, not an old candidate-size gate. No unresolved receiver is accepted
by removing these adapters.

Generation 126/expected schema 71 extends the rendering-only normalizer to
nested proven this/field/load expressions. Positive additions are combined
only within the same dereference level; wrong roots, different offsets, extra
or missing loads, malformed expressions and overflowing sums remain distinct.
The 92 focused call-contract cases pass. The fresh generation-126 scan clears
ordinal 7 and exposes ordinal 8's nullable call-result rendering.

Generation 127/expected schema 72 preserves the actual member-sourced SIB
index instead of an EAX/EDX name, and removes the old indexed-register marker
exceptions. It also fixes WSI-20260906-037: CFG propagation now includes later
arrivals through the queried call's own backedges. A generic regression had
proved that the former early stop silently omitted a changed next-iteration
receiver. Current COFF direct operands and converged ECX/vptr values supply a
targetless call-result proof; a nullable marker is never erased by fiat.

Generation 128/expected schema 73 recognizes the listing's trailing four-byte
DD data directives when checking those COFF operand proofs. DD had otherwise
been read as a fifth hex-looking token. Every supplied code/data row still
agrees with COFF bytes; existing independent switch-relocation and census
checks retain responsibility for noninstruction ranges. The 95 focused cases
pass. Fresh `call-contract-appframe-data-directive-g128-01` passes all 17
current-source call contracts with no divergences. No older candidate profile
is restored and no call or byte stage acceptance is inferred from this scan.

WSI-20260906-035 and WSI-20260906-037 are resolved at issue revisions 3999
and 4000 respectively. All seven generation-128 infrastructure checks pass.
Target synchronization at r5325 explicitly invalidates the prior AppFrame
order evidence. The provider-only message-map prelude has no default object
target, so live revalidation supplies the same exact registered AppFrame
target explicitly. Fresh `live-order-appframe-active-loop-r5325-02` passes
all 13 source-policy checks and all 17 authored order identities, accepting
the two covered blocks at r5326. Provider classification, full linked order,
source-owner provenance and byte facts are unchanged.
