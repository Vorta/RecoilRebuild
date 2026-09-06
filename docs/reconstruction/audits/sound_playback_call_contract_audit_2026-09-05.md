# Sound playback call-contract audit — 2026-09-05

This records diagnostic source and verifier work for the authored-byte
prerequisite. It accepts no owner, model, call contract, byte group, linked
image or gameplay result. The playground executable was not updated.

The authenticated open Recoil database provides direct assembly for
`zSndSample::PlayOnDirectSound` at `0x49fbb0`. Retail acquires a play handle,
falls back to the sample's embedded handle at `0x44`, and uses that handle's
backend-buffer member at `0x8`. Its ten calls are acquisition, GetStatus,
Restore, 3D update, mute query, two distinct volume-setting arms, position,
Play, and error reporting. COM slots are `0x24`, `0x50`, `0x3c`, `0x3c`,
`0x34`, and `0x30`, in that order. There is no marker-refresh helper call.

Source corrections restore separate muted/unmuted volume branches and load
the buffer after the mute query. The reconstructed zero initialization of the
GetStatus output was absent from retail and was removed. The shared marker
helper still emitted a call even when marked inline under the canonical VC5
profile. Its loop and last-voice stores were restored directly in the A3D and
DirectSound playback bodies, and the now-unused helper was removed.

`zsnd-volume-branches-order-01`, `zsnd-natural-marker-inline-order-01`, and
`zsnd-playback-marker-loops-order-01` each pass 43/43 authored order. The last
candidate has exactly the ten DirectSound calls and ordered COM slots listed
above. This listing observation is not whole-body or slice acceptance.

The generation-68 sound slice stopped at unresolved receiver storage because
the existing adapter required historical whole-caller offsets and a combined
volume call plus marker helper. WSI-20260905-017 tracks this verifier defect.
The replacement uses exact candidate listing/COFF agreement, registered
zero-addend E8/REL32 call identities, and bounded CFG propagation of the
factory-result/embedded-fallback join. It preserves targetless COM dispatch.
Retail must independently produce the same receiver lineage before storage
names are unified. No call is added, removed, reordered or assigned a static
COM target by this proof.

A generic regression verifies the join, malformed or missing COFF identities,
register clobbering and the distinction between zero-filled object operands
and actual linked target addresses. Generation 69 exposed a further loss of
receiver provenance after a loop: a partial branch join was stringified inside
the dataflow transfer and could not converge monotonically across a backedge.
The generation-70 transfer retains individual path expressions until the final
result is published. A generic factory/fallback-and-loop regression passes,
including rejection when the loop clobbers the receiver register.

The fresh 47-body target diagnostic at
`call-contract-sound-target-cfg-g70-01` passes the DirectSound caller. Four
other callers still diverge; this is not target, slice or stage acceptance.
Historical candidate-offset and call-folding code was removed, not refreshed
to match the new output.

## A3D and mute-state corrections

Retail A3D playback at `0x49fa60` calls vtable slot `0xd0` twice, at
`0x49fabf` and `0x49faea`. The bundled Aureal SDK's `IA3dSource` declares
`SetRenderMode` at that slot; `SetTransformMode` is at `0xf4`. Source incorrectly
called the latter. The old A3D projection explicitly rewrote those slots to
`0xd0` and removed three helper calls. WSI-20260905-018 records this acceptance
defect. That projection and the historical A3D receiver-offset profile were
removed. The shared exact CFG/COFF receiver proof preserves the actual slot.
Source now calls `SetRenderMode`.

The generation-70 diagnostic correctly reports the remaining `FloatToBits`
helper call as an extra invocation instead of concealing it. There was no
independent evidence for the two bit-conversion helper abstractions. Removing
their unsupported provenance claims caused the source-policy gate to reject
them. They were subsequently removed, with bit-preserving `memcpy` operations
placed directly in the three retail-backed caller sites.

Retail mute-state application at `0x4a0670` emits the A3D arm before the
DirectSound arm and has two separate DirectSound volume-setting sites. Source
now uses that backend arm order and separate mute/unmute calls, with receiver
loads after the mute query. Fresh `zsnd-inline-gain-storage-order-01` passes
43/43 authored order. Fresh
`call-contract-sound-target-g70-natural-source-01` passes 45/47 bodies,
including both playback paths and mute-state application. Only the two
wave-data deletion callers remain divergent. All seven infrastructure gates
pass. WSI-20260905-017 and WSI-20260905-018 were resolved on that scoped
evidence; no whole-slice or stage acceptance was claimed.

## Native wave-data destructor and compiler context

Retail `0x4a5440` calls Reset and conditionally frees the duplicated path.
The two heap callers invoke it directly before operator delete; the source
also contains a stack wave-data lifetime in `zsnd_create.cpp`. The unsupported
inline native-destructor-to-`Destructor()` forwarding layer was removed and
the existing body is now the ordinary out-of-line C++ destructor under the
same source anchor and physical lifecycle identity. The three affected target
registrations were synchronized through the governed dry-run/apply route.
Fresh order diagnostics pass 6/6 for the wave-data group and 43/43 for its
playback callers. `zsnd-native-wave-destructor-bytes-01` shows zero unmasked
COFF body differences at `0x4a5440`; this alone does not accept relocation
semantics, linked bytes, an owner tier, or the authored-byte phase.

WSI-20260905-019 tracks the two removed verifier projections that expanded a
candidate scalar-deleting-destructor invocation into two retail calls. Their
historical caller snapshots and helper-expansion code were removed in
generation 71, not adapted to the new source.

Important compiler-context correction: the canonical generated sound command
uses `/Ob0`, not `/Ob1`. An early hypothesis and the initial Pro prompt
incorrectly called that baseline `/Ob1`; the actual generated `_run_vc5_verify.cmd`
and final-build configuration supersede that claim. `/Ob1 /Gr` already emits
the direct destructor/delete pair. `/Ob2 /Gr` is not required for this pair.
The controlled registered `/Ob2 /Gr` diagnostic remains nonaccepting and has
no production TU mapping. Both `/Ob1 /Gr` and `/Ob2 /Gr` diagnostics preserve
the selected 43-body order. The focused archive-load caller has 74 unmasked
byte differences under either profile; its flag-update/control-flow source
still differs from retail. A subsequent controlled candidate maps only
`zsnd_play.cpp` to `/Ob1 /Gr`; no production TU uses the `/Ob2 /Gr` diagnostic.
Fresh live order accepted the six-body destructor group at revision 5229 and
the 43-body playback group at revision 5231. This is order evidence only.
The `PlaySimple` and snapshot `NewNode` byte sentinels both have zero unmasked
differences. The registry sentinel initially failed to compile because its
manifest omitted the existing Aureal SDK include path (WSI-20260905-022).

Direct diagnostic comparison of the archive-load `/Ob1 /Gr` and `/Ob2 /Gr`
COFF bodies finds identical 224-byte bodies, not merely equal mismatch counts.
Their relocation tuples are not literally identical: the generated EH label
is `$L43185` versus `$L43528`. Other relocation offsets/types/names agree.
No semantic equivalence of those differently named EH targets is accepted by
this observation. The Pro follow-up corrected the original `/Ob0` baseline
and advised the minimal TU-local `/Ob1` experiment with independent sentinels;
that advice is not acceptance or proof of original destructor spelling.

## Independent census and compiler-lifecycle review

The next call diagnostic blocked before compilation: the tracker still
classified `0x4a0800`, `0x4a0810`, `0x4a0830`, and `0x4a0840` as authored,
whereas the existing order manifest classified all four as non-authored.
Live order accepted the manifest subset without rejecting the disagreement.
WSI-20260905-021 tracks that tool defect. Generic regressions cover both
directions of classification disagreement and explicit role disagreement;
the order contract now fails closed instead of silently preferring one source.

Fresh BN assembly and xrefs independently support the existing compiler-helper
classification, rather than the old tracker navigation labels:

- `0x4a0800` has no code callers and one data reference, startup slot `0x4da0a4`.
  It calls `0x4a0810` and tail-jumps to `0x4a0830`.
- `0x4a0810` is called only by that coordinator. It copies the empty allocator
  byte to `0x56b290` and clears the vector's three pointers at offsets 4/8/12.
- `0x4a0830` is reached only from the coordinator and registers `0x4a0840`
  with `atexit`.
- `0x4a0840` is referenced only by that registration. It deletes the begin
  pointer and clears all three vector pointers; it does not destroy samples.

VC5 `INCLUDE/VECTOR` lines 34-35 and 52-55 implement the corresponding default
construction and destruction. Current source has one ordinary global vector,
not four authored wrapper bodies; fresh governed output emits `$E6/$E3/$E5/$E4`.
The reviewed correction is limited to these four pipeline classifications and
roles. It does not accept an owner, provider target, bytes, linkage, or their
legacy owner relationships. All four remain required full-order rows.

Generation 73 adds a rejection-only raw `CALL` population check before legacy
projections and inline-absence handling. One candidate helper cannot stand for
multiple retail calls. Tail calls still require ordinary control-flow and
target extraction. WSI-20260905-020 separately fixes stale register provenance
after `BTS/BTR/BTC`; `BT` remains read-only.

The four compiler-helper classifications were separately applied at revision
5232. After synchronizing the repaired registry diagnostic (5233), explicit
sound-order invalidation (5234) and fresh live reacceptance (5235) passed all
43 authored identities under the stricter conflict guard. The strict pipeline
audit passes. The four helpers remain selected non-authored full-order rows;
their old primary-owner relationships were not promoted or otherwise accepted.

The repaired registry diagnostic now compiles and reports zero unmasked
differences for both `GetCount` and `GetByIndex`, plus zero data differences
for the registry. WSI-20260905-021 and WSI-20260905-022 are resolved on their
scoped regression/live evidence.

Generation 74 removes five registry projections which erased actual `begin`,
`end`, `size`, indexing, or `clear` calls/tails (WSI-20260905-023). The fresh
`call-contract-sound-ob1-g74-01` diagnostic passes 41/43 authored bodies,
including all five registry callers and both wave-data deletion callers.
It still blocks the snapshot receiver and `RegistryAddEntry` provider helper.

Generation 75 uses the existing fresh, stable verifier/parent TU header
observations as the bounded source-file search set for deferred COMDATs.
VC5's no-op vector destructor has a numbered `VECTOR` source row but no
header `File` marker under `/Ob1`. Exact numbered rows must identify one
source file; no candidate body or receipt becomes provider expected truth.
The header pool is read once per TU without an additional compiler pass.
Generic regressions cover missing/duplicate/stale TU observations and ambiguous
or changed source rows. The focused proof kernel passes 92 tests.
`call-contract-sound-ob1-g75-01` gets past that provider-origin check, then
rejects the obsolete `push_back` projection (WSI-20260905-024/025).

Generation 76 removes that three-to-eleven-call expansion and the three
snapshot projections that rewrote constructor/helper calls into retail
provider, direct or virtual calls. The raw invocation populations and targets
now remain subject to ordinary extraction and comparison. The generation-76
diagnostic passes 41/43 bodies. It rejects a snapshot receiver lineage and a
malformed provider source-trace record; no complete slice, closeout,
authored-byte, or gameplay acceptance is implied by these focused checks.

Generation 77 repairs the provider registration producer and permits reviewed,
exact-current replacement of a malformed source-trace record. Replacement
content must still satisfy the complete schema. The five existing canonical
header providers at `0x40bdf0`, `0x40c190`, `0x40c1c0`, `0x4233b0`, and
`0x423400` had explicit provider-boundary status but lacked `source_edges`.
The reviewed batch at revision 5236 adds explicit empty lists. It does not
change their providers, owners, physical-emitter uncertainty, or byte facts.
The focused kernel passes 94 tests and all seven doctor checks pass.
WSI-20260905-026 records the producer/repair-route defect.

The generation-77 fresh sound comparison again passes 41/43. Its expanded
diagnostic shows the snapshot index is already a proven
`bounded-stack-counter+0x18`; the receiver remains rooted in the sample
returned by `GetSampleAt`, through the array at `+0x84` and interface at `+8`.
The old stack-vptr grammar rejects that exact four-load path. The provider
comparison advances past the repaired trace and rejects the canonical
`_Construct<int>` catalog name as a competing identity for a freshly proved
pointer-copy COMDAT.

Generation 78 recognizes only the exact known-object/array/proven-index/
element/interface grammar without removing any field, scale, or root identity.
It also distinguishes explicit canonical provider catalog names from inferred
collisions after the existing independent header/body/relocation proof.
Unknown indices, stack-rooted objects, unsupported scales, malformed catalog
names, and unlisted identity collisions remain blocked. The generation-78
comparison reaches the exact snapshot call at ordinal 9, but its expected
storage still uses a generic historical dynamic label; it also exposes an
additional literal registered provider name for the same physical copy helper.

Generation 79 removes the snapshot's four historical retail receiver-label
overrides. Ordinary extraction now preserves the complete primary and duplicate
voice paths on both sides, and the snapshot passes directly. The collision
check also recognizes literal, explicitly non-authored registered names at the
same physical address; inferred names, regex guesses, and conflicting physical
identities are not admitted. The fresh sound comparison passes 42/43 bodies.

The remaining registry-add caller reaches unresolved provider `0x48bf10`.
Fresh BN assembly shows a call-free three-stack-argument dword construction
loop, including the conditional destination store, source/destination strides,
advanced-destination return, and `RET 12`. Sixteen direct call xrefs span HUD,
input, network, and sound; no data references are reported. Canonical VC5
`VECTOR` `_Ucopy` and allocator construction reproduce all 48 bytes, with an
explicit empty relocation set, under the new governed
`vc5-vector-int-ucopy-ob1-v1` recipe. Dry-run scrutiny permits only this exact
provider boundary and supplied source identity. Fresh apply at revision 5237
registers it; the physical emitter and retail ICF winner remain unknown.
No authored byte group, selected linked population, or final-image fact is
accepted by that provider registration. The compact focused kernel passes
97 tests. The fresh `call-contract-sound-ob1-g81-01` comparison passes all
43 authored bodies with zero divergences, using ordinary extraction after
the obsolete registry and snapshot projections were removed. Doctor passes
all seven infrastructure checks. WSI-20260905-019/023/024/025/026 are resolved
on their scoped proof-kernel and live diagnostic evidence. This is not serial
slice acceptance, whole-stage closeout, authored-byte acceptance, or a gameplay
test.
