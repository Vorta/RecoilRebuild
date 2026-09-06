# Weapon math call targets

Retail `0x4ae660` calls `_CIasin` at `0x4ae8ba` and `0x4ae918`;
`0x4aee40` calls it at `0x4aeef5`. The floating-point input is loaded
directly and the result is stored as a float. The previous source used
`asinf`, whose VC5 `MATH.H` inline wrapper is emitted out of line under
the governed `/Ob0` profile. That adds a distinct physical call target.

The three expressions now use `(float)asin((double)value)`, matching the
canonical header's conversion semantics while allowing VC5's intrinsic
selection to emit direct `__CIasin` calls. No inline assembly, new helper,
compiler-profile change, or provider alias is involved.

The fresh `zwep-native-asin-order-01` build passes all 66 authored order
rows. Its direct helper call offsets are +0x250/+0x2a9 and +0xab. The
fourth math caller and its `_CIacos` call remain unchanged.

Generation 109 removes the verifier's wrapper-to-helper identity rewrite
(WSI-20260906-021). It retains exact live COD/COFF direct-call opcode,
relocation, undefined-symbol, and independently derived retail provider
checks. A reintroduced wrapper fails the candidate population proof.

Removing the wrapper also renumbered the runtime loop's COFF section and
undefined `__chkstk` symbol: section 65, symbol 289, with the same 0x14c0
body, `mov eax,0x1f6c` at +0 and direct probe at +5. Generation 110 records
those candidate-only coordinates; retail setup remains separately checked.
Fresh `call-contract-zwep-native-asin-g110-01` passes all 66 call contracts.

The focused proof kernel passes 65 cases. These are scoped diagnostic
passes, not whole-census acceptance, authored-byte equality, provider ABI
acceptance, or a new playground build.
