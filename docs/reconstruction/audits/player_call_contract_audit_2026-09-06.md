# Player inline-context and node-flag audit

The generation-95 full-census diagnostic first exposes Player failures at
`0x41eb30`, `0x41eb90`, `0x41ecd0`, and `0x41efa0`. Current canonical
`player.cpp` used `/Ob0`, leaving placement allocation, vector insertion,
iterator accessors, and CString accessors as calls absent from retail.

The controlled existing-profile diagnostic
`player-inline-profile-diagnostic-01` changes only the inline-enabled
compiler profile, preserving canonical definitions and includes. It passes
the registered authored-order projection and matches the 603 instruction bytes
of `RecordNodeFlagsForRestore` outside COFF relocation fields. All eleven
retail calls are present; the `/Ob0` source had four. This supports selecting
the existing `/Ob1 /MD /GX /Gr` profile for this TU, not accepting an original
flag tuple or any provider/byte stage from the diagnostic alone.

Retail `RestoreRecordedNodeFlags` snapshots the node and all three flag fields
before its first conditional setter call. The original reconstruction read
later fields after earlier calls. Copying the 16-byte record at the top of the
iteration reproduces this behavior. `player-native-inline-snapshot-order-01`
passes all 165 authored-order entries and matches both `0x41ecd0` and
`0x41efa0` outside relocation fields. Its comparisons also match
`0x42a070`, `0x42a480`, `0x42a4a0`, `0x42a4b0`, and `0x42a4d0` outside
relocation fields; this is diagnostic, not linked/provider acceptance.

The old Binary Ninja function comment at `0x41efa0` incorrectly claimed that
the function clears the vector. Assembly has no end-pointer store or clear
call. The comment now describes the snapshot and end reload, explicitly
noting the absence of clearing. Analysis was refreshed with no warnings,
instructions were checked unchanged, and the existing database was saved.

The two overlay `Constructor()` wrappers remain incorrect after adding
`inline` to their separate C++ constructors: they still emit a null check and
call the derived constructor, while retail directly constructs the base and
writes the derived table. The extra wrapper layer needs source correction;
matching its one-call count does not establish call-target identity.

`player-native-constructor-order-01` removes those two wrappers and attaches
their address anchors to the natural C++ constructors at the same positions
in `player.cpp`. The existing initialization routines construct the same
global storage using placement new; no layout, table slot, storage extent,
or CRT registration changed. Both constructors now match outside relocation
fields, as do the two node-flag functions, and all 165 authored-order entries
still pass. Only their two registered candidate symbol selectors change from
named wrappers to VC5 constructor decorations.

The first inline-enabled live target scan aborts in the verifier, before
acceptance, with `NameError: _player_ob1_require_exact_caller_package`.
WSI-20260906-009 records this stale address-specific branch; its adjacent
`_player_ob1_require_int_vector_helpers` call also has no definition. The
generic exact no-op vector provider path already checks caller relocation
population, target/addend/opcode, unique external definition, exact return
body and padding, empty helper relocations, canonical header provenance,
COMDAT selection, and independent supplier identity. Removing the obsolete
integer-only branch lets integer and trivial-record elements use the same
proof. Synthetic positive/adversarial tests reproduce the crash before the
fix and pass afterward; the focused module has 60 passing tests. Verifier
generation 96 invalidates earlier candidate evidence conservatively; expected
fact schema 64 is unchanged because this repair derives no retail facts.

The constructor selector update was synchronized at transaction 5276, then
fresh `player-native-constructor-live-order-r5277-02` accepted all 165 order
identities at transaction 5277. This target requires its explicit
`--object-target` selector; the earlier attempt without it made no mutation.

The wider instruction diagnostic has 50 matching and 115 differing bodies.
An independently re-read historical `/Ob0` object had 46 matches. Six prior
matches are lost: `0x41eb00`, `0x41eb20`, `0x41eb60`, `0x41eb80`,
`0x41ec40`, and `0x41ec90`. All are singleton lifecycle wrappers. Therefore
the `/Ob1` mapping remains an active, incomplete source-context correction,
not an accepted final profile: it must recover those lifecycle sentinels.
Four initialization wrappers gain placement-new null checks; the two base
cleanup wrappers inline the header's empty destructor instead of tail-calling
retail `0x4b47a0`. Native global-object lifecycle and header/body placement
require investigation before changing the broader object/CRT model.

The complete generation-96 live Player diagnostic no longer crashes, but
reports 18 divergences, mostly obsolete address-specific projection rules
which require the old helper-heavy candidate even where the native body is
exact. WSI-20260906-009 is resolved after 139 kernel tests, seven doctor checks
and that complete live diagnostic. WSI-20260906-010 tracks retiring those
obsolete projections while retaining direct physical invocation comparison.
The cleanup removes 2,714 old lines and preserves the separate compiler,
provider, COFF, relocation and import identity paths. Verifier generation 97
conservatively invalidates prior candidate results; expected schema remains 64.
The extended physical-contract test rejects extra/missing/reordered rows,
call-versus-tail changes, direct-versus-indirect substitutions, wrong targets,
and mismatched known cleanup. Unknown retail cleanup remains unknown.

The complete generation-97 Player diagnostic (`call-contract-player-physical-calls-g97-01`)
finishes all 165 bodies and reports four divergences: the two overlay cleanup
tails, the top-panel direct/indirect call mismatch in `0x41fe90`, and the
uncatalogued pointer-vector `_Ucopy` identity in `0x42a070`. This is a diagnostic,
not call-contract acceptance. WSI-20260906-010 is resolved after that scan,
139 kernel tests and seven doctor checks.

The Pro lifecycle review (`2026-09-06T09-18-42-058Z-chatgpt-call`) recommends
isolating destructor visibility, then trying native global lifetimes without
their handwritten registration groups. A declaration-only base destructor
trial restores the cleanup tails but was reverted: the subsequent native
underwater global produces all four retail startup/cleanup bodies without
changing the shared inline destructor. Its implicit derived destructor has
the same seven-byte base-vptr leaf shape as retail `0x4b47a0`; logical target
identity and ICF selection are not established by matching that body.

`player-native-overlay-panel-globals-diagnostic-01` extends native lifetimes
to the projectile overlay and both message panels. All sixteen startup,
construction, registration and cleanup bodies match outside relocation
fields. The two node-flag bodies and `0x42a070` remain matching. Native panels
emit the direct `SetTextFmt` calls present in retail `0x41fe90`; that larger
caller still has other instruction mismatches. The native vector declaration
is now after the recording function, at its retail startup contribution.
Registered selectors still describe the removed handwritten functions, so
the current diagnostic does not pass registered order and is not accepted.

Direct retail CRT xrefs establish this player group at `0x4da058..0x4da07b`:
common list, modal list, underwater, projectile, sensor list, save-state list,
first panel, second panel, node-flag vector. Each of the four list initializers
contains a startup jump to an aligned local body which clears fields in the
order auxiliary, tail, head, count. Current handwritten functions omit that
startup jump, and their legacy manual CRT records precede the native entries.
The complete list/global model and registration order remain under review;
these diagnostics do not accept storage extent, aggregate aliases, lifecycle
classification, linked placement, or final CRT data.

The follow-up Pro review (`2026-09-06T09-44-49-184Z-chatgpt-call`) recommends
the existing sensor record as the first native-list experiment, then a
class/template differential for the common list. The transcript was verified
and the complete exchange echoed. `player-native-sensor-list-diagnostic-01`
emits a jump-only coordinator and the four absolute stores in retail order,
without destruction registration. The sensor definition moves from
`zui_widgets.cpp` to the Player startup position; only its definition moves,
not its operations. Complete direct xrefs establish the initializer roots,
their local store bodies, and each of the sixteen fields across the four lists.

The common-list class and state-only template experiments have identical
coordinator/body bytes, relocation types, targets and addends. This does not
recover an original template name. `CPlayerListState<Node>` remains a
provisional, explicitly documented state-only implementation of the repeated
16-byte layout. Existing insertion, traversal, unlinking and explicit deletion
operations are unchanged except for replacing split globals with direct members.
The sensor retains its existing distinct type; no shared node base or common
cleanup policy is inferred.

`player-all-native-startup-diagnostic-02` compiles all nine native globals in
the retail CRT sequence. `player-native-startup-selector-order-01` passes all
165 historical authored-order selectors after mapping native generated symbols.
These are pre-classification diagnostics. Current source removes all manual
Player CRT registrations and the overlay/panel storage unions and casts.
The three list aggregates and initialized save-list pointer pass the four-item
data comparison in `player-native-list-data-diagnostic-01`. The pointer's
DIR32 relocation targets `_g_PlayerSaveStateList` with zero addend; its masked
data comparison alone does not accept that target identity.

Tracker transactions 5278--5282 detach obsolete split-field source traces,
coalesce common/modal/save-state storage separately with complete logical
field history, then attach forty current native-global/lifecycle/field traces.
Coalescence preserves function order, explicitly accepts no owner/data/byte
gate, and conservatively invalidates dependent evidence. Field offsets are
0/4/8/12, each extent is four bytes, and each aggregate covers exactly sixteen
bytes with no internal padding.

Transaction 5283 corrects twenty former handwritten-body classifications to
`authored-lifecycle` / `compiler-generated-thunk`. Direct native VC5 emission,
the retail CRT-only root xrefs, and the full startup/cleanup graph establish
these as generated contributions of authored global objects, not deliberate
standalone C++ function definitions. Their actual constructors and gameplay
methods remain authored bodies. All twenty retain required presence and full
linked-order gates; no linked-byte or final-image requirement is removed.
The four list roots include local store bodies in BN, whereas VC5 emits
separate coordinator/store COMDATs: full linked verification must retain that
complete physical coverage, including intervening padding and jump targets.
Original template spelling, exact historical type/TU naming, destructor ICF
identity, and final linked CRT slots remain unaccepted.

The fresh live order invocation in `player-native-lifetime-live-order-r5285`
accepts all 145 remaining authored bodies at transaction 5285. Its selected
population excludes the twenty independently classified generated contributions,
not the constructors or gameplay methods. This accepts authored function order
only; whole-image generated-code presence and ordering remain later gates.

WSI-20260906-011 identifies a provider-catalog integration defect: the
candidate-only matcher ignored canonical-header v2 identities, while header
registration independently compiled only the primary name of a proposed
logical-alias census. Verifier generation 98 now re-proves every requested
specialization through an independently registered canonical-header recipe.
The candidate-only path retains exact live header provenance, complete COFF
extent and COMDAT selection, literal accepted type identity, and catalog-wide
unique physical identity. It deliberately rejects relocation-bearing aliases
until independent typed target proof is available; body similarity and caller
ordinal cannot introduce a provider name.

Transaction 5286 extends the existing `0x48bf10` catalog with the explicit
binding-group-pointer vector specialization. The dry run and apply each freshly
compile both integer and pointer recipes, both reproducing all 48 immutable
retail bytes with no relocations. Primary identity, extent, prior names and
provider owner are unchanged. The tool permits only this monotonic form of
existing-header catalog extension and preserves prior owner facts. The physical
emitter and original ICF winner remain unknown. Three synthetic adversarial
tests cover matching, every-alias proof, and guarded extension; the full compact
kernel passes 142 cases. This registration does not accept the Player caller's
call contract, authored bytes, or linked placement.

The generation-98 Player scan completes all 145 bodies with seven divergences.
The copy provider now resolves; the next uncatalogued specialization is `_Ufill`.
Transaction 5287 independently proves and adds that pointer specialization to
the existing `0x40c190` catalog, again matching all 48 retail bytes with no
relocations for both recipes. WSI-20260906-011 is resolved after the complete
scan, 142 kernel tests and seven infrastructure gates.

The other generation-98 results expose the obsolete top-message indirect-load
rule (WSI-20260906-012) and five real direct/virtual dispatch differences for
the native underwater object. Retail `0x428bb8` loads the object's table and
calls slot `0x60`; the transition callers use the same virtual interface.
Player initialization also uses that interface for both native overlays.
Calls now use their actual `HudUiElement` base-class interface, preserving
ordinary typed objects and native lifetimes; no new dispatch table or wrapper
is introduced. `player-native-base-dispatch-diagnostic-01` still passes all
145 authored-order selectors. Generation 99 removes the 193-line obsolete
panel-buffer bridge and its invocation, retaining physical call comparison.
The focused proof module passes 63 cases before the next complete Player scan.

The generation-99 scan reduces the remaining Player divergences to two:
panel `Invalidate` must also call through its base-class virtual interface,
and the pointer-vector `size` specialization lacks an explicit catalog name.
The panel source is corrected without changing `SetTextFmt`'s direct form.
Named casts used in the first diagnostic are rejected by workspace source
policy; they are replaced by the existing project cast spelling, and the
modern-C++ guard returns to zero findings. The complete corrected dispatch
source retains its 145/145 order diagnostic pass.

Transaction 5288 freshly proves the integer and pointer `size` recipes against
all 32 immutable bytes of `0x42a9d0` with no relocations, preserving its existing
provider identity and owner. Verifier generation 100 includes the new registered
recipe; expected schema remains unchanged. Inspection of every physical call in
the current binding-group insertion body finds no further unknown helper
specializations beyond copy, fill, size and the independently governed trivial
destroy family. No call-site count, ordinal or expected target supplies those
provider registrations.

`call-contract-player-native-g100-01` completes the fresh governed compile and
direct live retail census with **145/145 authored bodies passing and zero
divergences**. This focused target command is explicitly nonaccepting: it
establishes convergence, not original-slice acceptance or whole-stage closeout.
The native lifecycle/source corrections and provider names are retained for
the subsequent serial stage replay. Authored bytes and linked placement remain
unaccepted by this call-contract result.
