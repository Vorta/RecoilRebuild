# Gameplay render-path comparison, 2026-09-05

This is an evidence note, not reconstruction acceptance or a runtime pass.
Retail assembly in the already-open `Recoil.bndb` and immutable
`support/Recoil.exe` are the reference. Current source and fresh governed VC5
output remain subject to the serial live gates.

## Matrix stack

The captured startup fault occurred inside `MatStackPushAndCloneParent` while
copying from a null parent matrix slot. An earlier extra pop can damage that
cursor without faulting at the pop itself.

| Retail body | Direct observation | Source correction |
| --- | --- | --- |
| `0x474c20`, unproject Z-buffer points | Identity and transformed paths each execute one pop and return. The three observed callers request one point. | Restore the separate local point scratch and mutually exclusive cleanup. |
| `0x453880`, light update | Identity position copy returns after its pop; the transformed/common path has the other pop. | Restore the early return instead of executing both pops. |
| `0x476cf0`, software model rendering | Point mode returns after its pop; polygon mode reaches a separate final pop. | Restore mode dispatch and exclusive cleanup. |
| `0x477b30`, hardware model rendering | The same point/polygon cleanup split; the lighting output locals do not overlap the active matrix. | Restore mode dispatch and separate matrix/lighting storage. |

The predeployment guard and selected call-contract checks now compare local
matrix-stack path effects, not just the static call population. The wider
`diagnose matrix-stack` command surveys primitive callers. Its unconstrained
branch exploration is diagnostic: a flag-correlated push/pop can produce an
infeasible path, and an unmodelled indirect jump remains unresolved. Calls
other than the selected stack primitives have zero *local* effect in the wider
survey; this is not an interprocedural balance proof.

## Scene traversal

Directly compared render traversals: camera `0x44ada0`, sound `0x44af60`, light
`0x44b140`, Object3D `0x44b300`, Animate `0x44b710`, and LOD `0x44b8c0`.

- Camera, sound, light, Object3D, and Animate test the model pointer at node
  offset `0x3c` before invoking `gModel_RenderFn`. Camera also applies the
  range-fade flag and scale before that call.
- For node flag `0x80`, result `0x20` is changed to zero, but clip-mask bit
  `0x20` is cleared regardless of whether that was the rejection result.
- Bounds-context activation belongs inside the actual bounds refresh; merely
  reusing cached bounds must not activate it. Matrix recomputation has its
  own activation path. Object3D's cached-matrix push skips that activation.
- Object3D's enabled HSE branch includes the frustum-grid tile condition.
  Its refresh path checks node flag `0x100` and returns, restoring alternate
  clipping when necessary, if the flag is absent (`0x44b3c5–0x44b3e2`).
- The LOD scale matrix must remain alive through recursive rendering and the
  corresponding pop, not merely through the conditional push statement.

## Hardware polygon submission

The material test at `0x4782e5` selects textured material bit `0x100`; its
clear branch goes directly to the no-UV lighting path at `0x478972`.
Within textured rendering, the lighting flags select the attribute path
(`0x478397`). Neither textured path falls through into no-UV submission.

Corrections recover distinct clipping counts, visibility, lighting flags,
lighting variation, and matrix storage. The lighting decisions use the
per-display-instance outputs, and preserve fog/light bit combinations. The
retail frame contains an `0x2000`-byte perspective-UV region below its other
locals; the reconstructed buffer now contains `0x400` two-float UV pairs.

Primary and alternate textured submissions receive perspective UVs. Alternate
clipping consumes the already projected/clipped polygon, remaps its coordinates,
and recomputes the UV output; it does not restart with unprojected original
vertices. Attribute propagation for newly clipped constant-shade vertices is
retained. The callback selection distinguishes the current normal array,
lighting flags, and attribute-array presence.

Both render-class submissions load alpha directly from retail `0x57d964`,
the current render alpha scale. They do not reinterpret a copied-vertices flag
as a float or substitute the material alpha byte. No-UV color submission takes
the material RGB at offset `4`. Depth bias uses the full signed draw-flags value;
immutable float `0x4d2a44` is `-1.0`, so the observed subtraction adds one.

## Subsequent world-grid crash

The next captured run reached a different fault: candidate `0x446228` in
`BuildFrustumGridTiles`, corresponding to the retail row/cell lookup at
`0x44c752–0x44c764`. The captured origin row was 17 and the clamped maximum
row was 16. The selected row pointer was `0xabababab`; dereferencing its
area-index member caused the access violation. This is evidence of the bad
grid lookup, not a repeat observation of the earlier null matrix copy.

Retail `WorldToGridCoordsClamped` (`0x450790`) clamps X inside the lower/upper
boundaries and treats Z as descending from `originZ` toward `worldMaxZ`.
The reconstruction incorrectly moved the lower X clamp outward and used
ascending min/max Z bounds expanded outward. Restore the inward clamps.
Retail's X threshold uses double `0.1`; its selected X/Z offsets use floats.
Immutable `0x4d2398` is float `-0.1`, `0x4d239c` is float `+0.1`, and
`0x4d23a0` is double `-0.1`: a subtraction of the negative constant must not
be reconstructed as a subtraction of positive `0.1`.

The companion `WorldToGridCoordsClampedEx` (`0x450650`) also had both Z tests
reversed. Retail clamps when `worldZ > originZ` or `worldZ <= worldMaxZ`,
not their opposite comparisons. The row-table allocation and cell stride in
`gwWorldSetVirtualAreaPartition` (`0x450c60`) agree with retail; the correction
does not resize that storage or add a bypass at the crashing dereference.

The fresh compiler listing was inspected for the corrected x87 comparisons,
branch directions, and clamp constant signs. Call population alone does not
detect this arithmetic/branch defect, and the matrix-depth startup guard does
not claim coverage of coordinate-clamp semantics.

## Limits

Order and call-contract comparisons constrain only their named dimensions.
They do not establish complete branching, arithmetic, storage, or runtime
equivalence. These corrections require another gameplay test; no successful
gameplay observation is recorded by this note. The broader matrix survey also
contains unresolved disassembly and predicate-correlation cases, which must
not be counted as confirmed defects or silently accepted as matches.
