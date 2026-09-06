# DirectPlay call-contract audit — 2026-09-06

This is non-WOL reconstruction work restoring truthful call-contract
prerequisites before authored-byte acceptance. No gameplay, byte-stage,
complete owner, or final-image acceptance is claimed.

## Direct retail evidence and source corrections

Fresh assembly in the authenticated Recoil database identifies the service
provider list as the VC5 native vector: `0x489d00` allocates sixteen bytes,
copies the empty allocator byte, and clears pointers at offsets 4/8/12;
`0x489e10` destroys its storage; `0x489fa0` clears the range after releasing
entries; `0x48a0d0` obtains the pointer-count size. The insertion callback
`0x48b3a0` calls the canonical vector construction/fill/copy/destroy helpers,
including the independently registered dword-range helper at `0x48bf10`.

The previous reconstruction used a fabricated four-field structure and a
handwritten grow/copy helper. The source now uses
`std::vector<zNetworkDPlayServiceProviderInfo *>`, native allocation/deletion,
clear, size, indexing, and `push_back`. Its sole cross-TU consumer is the
mission session-browser dialog. The changed return-type decoration was read
from fresh governed VC5 output and synchronized at revision 5242; no retail
target identity was substituted. Fresh live order passes mission 120/120
(5243) and DirectPlay 54/54 (5244).

Retail `0x48afe0` initializes a result local to zero and routes switch cases
to a common return, setting minus one on fatal disconnection. The source
now uses that structured common-return form. The first governed output still
merged case 0x102 and 0x103 dispatches into one physical call. Moving packet
locals into the four individual case scopes restores both physical calls;
the body now emits all twelve retail invocations. Native provider-record
construction also restores the insertion callback's thirteenth `size()` call
and its guarded allocation/EH structure. Native player-record construction
and complete body equality remain additional source/byte obligations.

## Verifier defect

WSI-20260905-029 records a legacy projection that copied the eleventh actual
invocation to manufacture a twelfth row, plus candidate indirect-call/switch
adapters requiring obsolete body lengths, local labels, and instruction
offsets. The rejection-only raw CALL census already rejects the shared-call
source mismatch before those adapters can run. Generation 84 removes all
three functions and their dependent invocation/merge code. Calls now reach
ordinary live extraction without the fabricated row or candidate snapshots.
`call-contract-network-native-vector-g84-01` passed 51/54 with two actual
call-count differences and the dispatcher storage failure. Following the
packet-local/provider-record corrections, `network-native-record-order-01`
passes 54/54 order. Its g84 call comparison reports three extraction/identity
blockers rather than the former raw-count mismatches.

## Canonical provider and generic CFG correction

Retail `0x42a9d0` tests the vector begin pointer at +4, returns zero for null,
otherwise subtracts it from the end pointer at +8 and divides by four. The
canonical VC5 `<vector>` `vector<int>::size() const` probe matches all 32 bytes,
with no relocation masks. Registration at revision 5245 records only this
canonical provider identity; the physical emitter and retail ICF winner remain
unknown. WSI-20260906-001 fixes a registration guard that rejected detached
non-authored rows solely because of a legacy `primary-owned` label. Actual
owner references and overlaps still reject registration; generic negative
tests exercise symbol-only and address-only owner claims.

Generations 87/88 add an address-independent proof for the contiguous
CMP/JA/XOR/byte-classifier/table-JMP lowering. Live COFF bytes, exact DIR32
symbol indices, complete bounded classifier values and all local table
targets establish the CFG. Bounds-check bypasses, wrong symbols, malformed
tables and unknown paths fail closed. Backward targets remain in the complete
CFG; they are not silently discarded from forward-only dataflow. A generic
callback-load proof requires every CFG path and every loop visit to retain the
same exact zero-addend global load before the register call. These changes
publish no retail expectation from candidate bytes.

`call-contract-network-classifier-g87-01` aborted on a misplaced keyword
parameter and accepted nothing; generation 88 corrects its integration.
`call-contract-network-classifier-g88-01` passes 53/54 authored bodies. The sole
remaining failure is `0x48c200`, unresolved candidate storage at offset `0x30`.
The focused proof kernel passes 99 tests. No stage acceptance follows.

## Remaining native-list data identity

Direct retail constructor `0x48bfb0` copies an allocator byte to `0x56add8`,
allocates a circular 12-byte sentinel node, stores its pointer at `0x56addc`,
and clears count at `0x56ade0`. Fresh native-list construction has the same
instruction sequence outside relocation fields. The 74 executable bytes of
the dispatcher likewise match outside its A1 operand: retail references
`0x56addc`, while COFF uses `_g_zNetwork_DispatchHandlerList` with addend four.
The existing tracker has three separate unsized physical field rows, and BN
still presents those fields separately. The governed base-identity binding
dry-run from the constructor's operand at offset eight rejects the unknown
target extent and makes no mutation. Do not invent a one-byte whole-object
extent or overlay a 12-byte physical aggregate on the existing field rows.
The ambiguity-driven Pro critique at
`.devspace/runs/2026-09-06T00-21-44-472Z-chatgpt-call/transcript.md` recommends
validating then coalescing the complete physical object while preserving
non-owning field views. This is advice, not evidence. The canonical VC5 LIST
header independently declares allocator, `_Head`, `_Size` in that order;
constructor and mutation xrefs corroborate the fields. No xrefs were found at
the three alignment bytes `0x56add9` through `0x56addb`, or at `0x56ade4`.
The latter has no BN declaration; its surrounding zeroes do not establish a
larger array. The 12-byte heap allocation is node size, not proof of the
global object's extent. At that point no BN data changes had been made.

## Reviewed aggregate correction

WSI-20260906-002 records the missing governed coalescence route. The new
`progress data-artifact coalesce` command checks an exact current snapshot,
one primary owner, the complete field/storage/target census, explicit field
and padding coverage, section containment, and unhandled current references.
It does not accept data bytes, source shape, linkage, or an owner tier.
Generic proof-kernel tests exercise stale state, missing padding, overlap,
foreign owners, extra physical rows, omitted targets, and stale base bindings.
The complete compact kernel passes 126 tests.

The reviewed dry-run was applied at tracker revision 5246. Physical
`0x56add8` now represents the native 12-byte aggregate; the allocator byte,
sentinel pointer, and count survive as logical-data views at offsets 0, 4,
and 8. The old physical field/storage rows are archived in
`recoil:evidence:r5246:000001`. Both data-only manifests and their target
registrations now describe the same aggregate. The owner's affected gates
are pending, and dependent call/byte evidence is conservatively invalidated;
all physical function-order records are preserved unchanged. This correction
does not claim authored-byte progress.

The production header already asserts `sizeof(zNetworkDispatchHandlerList)
== 12`; the canonical VC5 LIST layout and fresh constructor output corroborate
that exact extent independently of the heap-node allocation. Binary Ninja
now has one `zNetworkDispatchHandlerList` declaration at `0x56add8`, with
natural padding between the allocator byte and sentinel. Attempts to undefine
the interior user variables initially reported auto-analysis variables still
present; defining the enclosing aggregate superseded them. Subsequent reads
resolve both interior addresses to the aggregate at offsets 4 and 8, and
refreshed constructor/dispatcher HLIL accesses its named fields. Assembly
operands are unchanged. The database was saved after propagation checks.
An unsupported exact original filename and the allocator-as-application-flag
wording were removed from the constructor comment; the iterator comment now
includes the required INC in its inequality explanation.

The immutable-retail relocation-target binding was separately reviewed and
applied at revision 5247: constructor operand offset 8 binds the actual
`_g_zNetwork_DispatchHandlerList` symbol to the existing aggregate base.
The three unused old split-layout macros were removed from `znet.h`; they
referred to members absent from the native `std::list`. A fresh DirectPlay
call comparison is required to establish its effect on dispatcher storage.
`call-contract-network-aggregate-g89-01` now passes all 54 authored bodies,
including `0x48c200`, with no caller divergences and unchanged source during
validation. This is a nonaccepting scoped comparison; replay and closeout
remain required for whole-stage acceptance.

`network-aggregate-cleanup-order-01` separately passes 54/54 authored order.
`network-native-aggregate-data-01` compares the complete native 12-byte symbol
with retail: zero unmasked differences, zero relocation bytes, BN size 12,
COFF size 12. This data-only diagnostic does not accept linked storage or an
owner gate. WSI-20260905-029 is resolved at issue revision 3932 and
WSI-20260906-002 at 3933. The workspace doctor passes all seven checks; the
latest focused transaction suite passes 18 tests, including conservative
rejection of a stale field-base reference from another storage contribution.

## Later generic integration regressions

Fresh generation-104 comparison passes 52/54 bodies. The unchanged dispatcher
is rejected because a complete COFF table with a backward case was merged into
the forward-only invocation walk. WSI-20260906-016 records this integration
error. Generation 105 separates the complete, ordered case map from the linear
walk's forward-only map. A table with any backward case terminates forward
provenance as a whole, while bounded CFG callback proof retains every case.
It does not admit a backward edge into a one-pass merge or discard individual
predecessors. Generic tests cover the composed maps, preserved repeated cases,
reordered/conflicting tables, malformed targets, and callback-load bypasses.

The second rejection is in service-provider insertion: the actual canonical
aliases of the Ufill helper are a proper subset of the separately accepted
catalogue, not conflicting identities. WSI-20260906-017 records the erroneous
required-population check. Generation 105 checks every present alias against
its exact catalogued identity without demanding absent aliases. The current
helper's independent canonical-header, body, and relocation proof is unchanged;
unknown names and wrong identities still reject. The focused proof module
passes 64 tests. Fresh `call-contract-network-cfg-catalog-g105-01` passes all
54 authored call contracts with no divergence and unchanged source during
validation. All seven doctor gates pass. Issues 016 and 017 are resolved at
issue revisions 3963 and 3964. The scoped comparison does not accept a stage.
