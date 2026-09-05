# Retail .data audit — 2026-09-05

The whole-section inventory, reference screening, identified annotation
corrections, and their readback are complete. **This is not completion of the
original-source reconstruction or final typed .data coverage.** Original object
identities and extents that the evidence does not establish remain unresolved.
The final-image catalog still reports 3,098 .data annotations with unknown
storage extent. These notes are diagnostic findings and reversible edit notes,
not a second tracker or reconstruction acceptance.

## Scope and limits

Subsequent user-requested presentation correction: the four minimal partial
array views were replaced with plausible working capacities in BN: turret
pointers 128 (0x4f3fe8), triangle records 1,024 (0x53a750), matrix pointers 32
(0x566868), and matrix identity flags 32 (0x566950). Do not restore the old
one/two-element views from historical logs. Their comments now describe the
working capacity and its remaining evidentiary qualification. No source or
tracker acceptance changed. See the
[capacity restoration log](data_audit_2026-09-05_array_capacity_restoration.json).

The authenticated, already-open database was
`D:/Recoil Project/Decomp/Recoil.bndb`; no binary was loaded or switched.
Retail .data occupies `[0x4da000,0x779ac0)`. File-backed data occupies
`[0x4da000,0x4e5c00)` (48,128 bytes); the remaining 2,703,040 bytes are
loader zero-fill. Zero-fill alone establishes neither an array nor padding.

The audit obtained the initial 3,462 definitions and their xrefs, all 4,951
available function assembly listings without truncation or failed reads, and
the file-backed bytes. It screened direct/interior address operands,
initialized pointer candidates, gaps, string terminators, scalar floating-point
accesses, and non-character arrays. The literal review included 1,966 character
arrays. Pointer-looking words inside strings were not treated as pointers.
The 44 non-null CRT initializer slots were retained despite lacking ordinary
direct code xrefs.

This is a complete acquisition/reference-screening pass, not a proof that every
function was exhaustively interpreted or that indirect arithmetic can never
reach a currently unreferenced byte. Large-array bounds were checked against
producers/consumers where available. A runtime limit, known access prefix, or
adjacent symbol is not automatically the original allocation extent. Retained
BN names/types are not thereby accepted as original-source facts.

Inventory readback before the subsequent capacity restoration: **3,146 definitions, no overlapping declared extents**,
and 1,648 gaps totaling 31,905 bytes outside those declared extents. These are
BN analysis counts, not final-image typed coverage. Withdrawing unsupported
definitions and guessed extents necessarily exposes more gaps.

## Applied corrections

- Withdrew 315 unsupported BN definitions across the shelf and the rest of the
  section. At `0x4dc264`, withdrew the unsupported user symbol; BN retains an
  unnamed automatic pointer interpretation of the initialized word
  `0x4f3a78`. No retail bytes or production storage were deleted merely
  because a reference was absent.
- Replaced the false standalone ES string at `0x4e486c` with the complete
  PRIMITIVES key at `0x4e4864` (11 bytes including NUL). Retail `0x4ba0ff`
  supplies the complete key to the call at `0x4ba104`.
- Corrected CString at `0x4ed714` from zero-width void to four bytes using
  constructor/destructor/assignment evidence.
- Corrected two VC5 vector layouts: sound storage at `0x56b290` and feature
  storage at `0x539df0`. Both have byte allocator representation, alignment,
  and pointers at offsets 4/8/12. The latter replaces four misleading scalar
  globals. Production source already uses real std::vector objects.
- Corrected snow/rain accumulators `0x56bf68` and `0x56bf6c` to float.
  Production source already uses float.
- Corrected five NUL-terminated literal extents: objectives.zrd at
  `0x4dafa8` (15), DISABLE_SEL at `0x4e471c` (12), DISABLE_UNSEL at
  `0x4e4728` (14), TEXT at `0x4e4738` (5), and CHECKED at `0x4e4740` (8).
  Updated corresponding source declarations and three governed data targets
  from immutable retail terminators.
- Corrected CRT slot `0x4da080` to _PVFV. At `0x4c621b`, _start supplies
  `[0x4da000,0x4da0b4)` to _initterm.
- Corrected the MFC initializer interpretation: `0x4c81d8` is the local VC5
  AfxInitialize path with version 0x421, not a code-page setter. The separate
  _setmbcp call takes -3. Checked local provider source/header evidence.
- Corrected the briefing comment: progress at `0x4da24c` is consumed by
  signed FILD; assignment of 999 at `0x404b14` belongs to the missing-sample
  path, not successful playback initialization.
- Initially used partial access-prefix views for turret, triangulation and
  matrix stacks. These were subsequently replaced at the user's request with
  the plausible working capacities listed above, avoiding misleading [1]/[2]
  declarations while retaining explicit uncertainty in comments.

All mutations were read back and saved. Changed types were refreshed and
representative propagation checked. No retail bytes were patched.

## Comments

The section-base comment sweep retained 38 checked comments, narrowed/replaced
27, and removed 326 whose complete claims were not confirmed, in addition to
earlier shelf/initializer corrections. Removal means unconfirmed, not
necessarily proven false. Supplementary function work narrowed 43 function
comments and removed one; inline reviews retained seven, narrowed two, and
removed 28 unconfirmed comments. Historical source-file, capacity, decompiler,
and blanket acceptance claims were not retained as truth.

Two concrete misleading function claims found in the array follow-up were also
corrected: `0x49a920` tests x87 status AH bit 0x40, not an ordered greater-than
predicate; `0x4a7490` checks the -1 selection sentinel, not an upper device-index
bound. These observations did not authorize unrelated runtime rewrites.

This does not claim that every comment in the entire .text database was
audited. The JSON edit logs preserve exact removed text for rollback.

## Array evidence and remaining uncertainty

| Storage | Direct retail evidence | Still unproved |
| --- | --- | --- |
| Turrets 0x4f3fe8 | Pointer stride and count-controlled loops | Original capacity: neither candidate 128 nor proposed 129 follows from these accesses |
| Triangles 0x53a750 | Twelve-byte index records | Candidate 1,024-element allocation |
| Matrix stacks 0x566868 / 0x566950 | Initialized cursors at 0x4e0e88 / 0x4e0e84, four-byte steps, current/parent accesses in 0x472ef0 / 0x472f30 / 0x472f60 | Candidate 32-element allocations |
| Face scratch 0x57c2c4 | Twelve-byte vertices, runtime vertex count | Four is the current candidate prefix; a 64-element gap fit is not allocation proof |
| WOL browse records 0x4f5558 | 0x441040 copies 0x43 dwords: 268-byte stride | Candidate 1,024-record capacity |
| Map projection 0x4edc78 | 0x416480 uses twelve-byte points and appends the first point after runtime count | Candidate 1,024-point capacity |
| Light scratch 0x566a28 | Four-byte entries, 256-byte row stride in 0x488d60, separate light limit 64 | Complete allocation/model; values are not always squared distance |
| Software queues 0x57de80 / 0x5cb274 | Append limits 350, strides 900 / 1,164 | Source-owner/typed allocation acceptance |
| D3D queues 0x636518 / 0x6b7118 | Limits 256 / 384, strides 2,060 / 2,064 | Source-owner/typed allocation acceptance |
| Texture entries 0x53d79c | 0x46d42d limit 4,096, 36-byte transfer stride | Owner/full typed-data acceptance |
| Frustum rings 0x56ccc0 | Fifty count-field initializations, 724-byte stride | Full source model acceptance |
| Lens-flare samples 0x62ea04 | 0x49a830 rejects count >=650, 20-byte stride | Owner/typed extent acceptance |
| Visible sample pointers 0x631cd0 | Inputs restricted to first 64 queue records in 0x49a920 / 0x49a9c0 | Complete original allocation extent |
| Quad batch 0x63363c | 0x4acd0d limit 16, 128-byte stride | Owner/typed extent acceptance |
| Device records 0x633e44 | 0x4a942f limit 4, 0x6ec stride | Owner/typed extent acceptance |
| Impact queue 0x778970 | 0x4b0ac3 limit 64, 68-byte stride | Owner/typed extent acceptance |

Additional checks establish 20 initialized font pointers at 0x56179c, a 200-way
random-table index reduction at 0x575a80, five wildcard digits and pointers,
and seven CString objects ending exclusively at 0x4f32f4. The occluder append
path rejects count >=7; its vertex copy precedes the clamp of stored vertex
count to eight. Neither proves the existing eight-record allocation by itself.

Some apparent gap references have non-object explanations: 0x53898f is a
base-minus-one newline index; 0x4f32f4 is a one-past CString address; D3D operands
0x635d08 / 0x635d14 are biased indexed accesses into temporary vertices.
0x635d0c also has an independent context-global use and was not merged.
MFC's empty object address 0x56cc28 is real, but an arbitrary 64-byte bridge
preview was not accepted as its extent.

Unreferenced nonzero bytes remain in raw gaps, notably four AI words at
0x4da0d4, identity-like words near 0x4e0e44, and the numeric band near 0x4e5954.
Bit patterns do not establish independent source objects. No blanket
zero-gap-to-array or raw-gap-to-padding conversion was performed.

## Source and tracker consequences

Compiled-source changes are the five literal extents above. Additional source
comments identify unproved candidate capacities and zero-shadow objects
without asserting original ownership or deadness. Existing candidate turret,
triangle, matrix, and face-scratch allocations were not resized.

Ten reviewed, dry-run-first progress owner downgrade mutations withdrew
unsupported data/source/boundary/linkage gates as applicable: AI mode-2 words,
player auxiliary pointer, camera/list-filter/node/type-list zero shadows,
anonymous numeric cluster, turret storage, triangulation storage, and face
scratch. Transaction revisions 5211-5220 record exact changes. Face scratch
was already tier X and was not promoted. No order/call-contract/byte acceptance
was performed.

Fresh scheduler readback at transaction revision 5220, semantic revision 5217,
and evidence-generation revision 5217 selects authored-call-contract, first
slice 0x401000-0x408210, ready for replay. A passing audit or compiler diagnostic
does not replace that live replay and closeout.

## Validation

- Fresh verify vc5-order map_text_block_order_current_shape: **42/42 PASS**,
  root build/live-validation/data-audit-map-nul-20260905-01.
- Fresh verify vc5-order zui_4b3ce0_4bffe0_authored_order: **259/259 PASS**,
  root build/live-validation/data-audit-zui-nul-20260905-01.
- Fresh verify vc5 for the three changed literal-data targets with auto-chunk:
  **all selected data pass**, zero byte/relocation differences,
  root build/live-validation/data-audit-string-data-20260905-01.
- Eight focused tests in tests/tools/test_binja_resources.py pass for the
  evidence-reader correction. The final post-mutation infrastructure doctor
  run passed all seven gates, including the progress tracker and source policy.
- git diff --check passed after source/comment changes.
- Live audit final-image-catalog at revision 5220 **fails**: .data has 3,098
  unknown-storage-extent annotations and no accepted typed coverage for its
  full file/virtual range. Other sections also remain unresolved. Inventory
  coverage is not semantic acceptance. No final-image build was attempted
  through an incomplete catalog.

No complete call-contract replay, full link, playground deployment, or gameplay
test was performed as part of this audit. It does not establish that the
loading hang is fixed.

## Tools and rollback notes

WSI-20260905-006 is resolved: audit bn-data-evidence no longer silently swallows
failed/empty assembly reads or exhausted bridge budgets. It records
attempted/failed/unattempted reads and bounded errors, reporting partial
evidence instead of false support. The complete scan above was obtained
separately without raising or bypassing this command's bridge budget.

WSI-20260905-007 remains open: undefined-address declaration lookups can
serialize long MCP batches even after client cancellation. The installed
external bridge was not patched; inventory/gap inspection avoids those slow
lookups. The BN connection is currently usable.

Reversible edit logs, in application order (later entries supersede earlier
comments): [initial](data_audit_2026-09-05.json),
[follow-up](data_audit_2026-09-05_followup.json),
[literal/CString](data_audit_2026-09-05_continuation.json),
[shelf](data_audit_2026-09-05_shelf_corrections.json),
[weather](data_audit_2026-09-05_weather_types.json),
[other definitions](data_audit_2026-09-05_remaining_definitions.json),
[partial views](data_audit_2026-09-05_partial_views.json),
[feature vector](data_audit_2026-09-05_feature_vector.json),
[section comments](data_audit_2026-09-05_comment_review.json),
[function comments](data_audit_2026-09-05_function_comments.json),
[inline comments](data_audit_2026-09-05_inline_comments.json), and
[array follow-up](data_audit_2026-09-05_array_followup.json).
These logs are rollback history only and cannot substitute for live acceptance.
