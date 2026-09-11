# Raw assembly: unused-assignment exploration, 2026-09-11

The user requested a new C++/unused-assignment pass over every currently allowed raw assembly case, including previously committed exceptions. The starting allowlist contained **99 permission rows, 84 distinct region/consumer cases, and 55 region or file entries**. Duplicate permissions for `__asm`, `_emit`, and naked declarations are counted once per case. This audit covers that initial census, including the entry subsequently removed.

## Outcome

One assembly region was replaced: the discriminant calculation in `AINet::SolveAltGunLeadTargetPoint` at `0x4024a0`. Capturing both operands of its first product restores the retail x87 evaluation order. The six instructions in `[0x402624, 0x402635)` are now compiler-generated C++. The vector and square-root assembly regions elsewhere in that consumer remain.

```cpp
float unscaledQuadraticA, unscaledDistanceSquared; // Unused captures preserve the retail x87 evaluation order.
discriminant = (unscaledQuadraticA = quadraticA) *
    (unscaledDistanceSquared = dotProduct) +
    leadCoefficient.quadraticB * leadCoefficient.quadraticB;
```

The unused captures are supported by compiler output; their presence in the original source is a hypothesis. The production edit removes its two raw-source directives and its address-scoped allowlist permission. The allowlist consequently contains **98 rows and 83 distinct cases**.

## Experiment scope and limits

**302 successfully compiled complete-consumer comparisons cover 69 cases.** The other 15 cases were individually inspected: 14 hardware-instruction cases and one zero-code COFF alias. They are listed separately below; no failed or non-equivalent portable fallback is counted as a compiler experiment for them.

The C++ families include ordinary expressions, assignments inside left/right operands, both-operand captures, result captures, separate temporaries, pointer captures, reordered commutative operands, staged component stores, and selected double intermediates. Renderer variants use the native arithmetic/control-flow branch, then embedded input or index captures. LOS variants capture buffers at calls or conditions and capture scalar arguments. Sub64 variants use native 64-bit subtraction or explicit halves with input/result captures. The provider copy variants capture source, destination, or copied scalar fields. The CMOS benchmark variants retain its existing hardware-reading callees and reconstruct its RTC waits and arithmetic before applying clock/cycle captures.

These are bounded experiments, not a proof that every possible C++ spelling has been exhausted. No new instruction fallback, raw permission, compiler flag, pragma, provider identity, or source-owner claim was accepted. Existing pragma suppression was removed only in isolated normalize diagnostics to test the normal governed optimization profile.

Each comparison uses the complete emitted consumer body against immutable retail bytes and reports relocation-field-masked opcode differences. Candidate relocation fields are masked only to find useful candidates; this diagnostic does **not** prove relocation type, target, or addend. Only the subsequent fresh governed live comparison can establish a match. A zero-difference result with a different extent also fails. The table selects equal extent first, then the smallest diagnostic difference count; that ordering is a convenience, not a matching tier.

Diagnostic setup failures (macro semicolons, pointer-argument parentheses, CMOS placeholder substitution, and symbol lookup) were corrected and the affected cases covered successfully. The initial non-`v2` renderer fog/plain result selected the wrong conditional and is excluded. Some renderer exception branches are disabled in the canonical profile; their native alternatives were still examined, and are not claimed to match retail.

Artifacts: initial census and per-variant source/object/result files are under `build/live-validation/raw-unused-exploration-63/`; `inventory.json` retains all 99 starting rows. The drivers are `build/diagnostics/raw_cpp_exploration63.py`, `raw_cpp_followups64.py`, `raw_cpp_cmos66.py`, and `raw_cpp_ports69.py`. `hardware-inspection.json` records retail instruction observations. These are historical diagnostics, never reusable acceptance evidence.

## Complete case census

Region names below omit the common `recoil:raw-asm:` prefix. Each row corresponds to one initial allowlist case. The variant column identifies a directory containing `result.json` and the exact shadow source. Bytes are candidate/retail; differences exclude candidate relocation fields.

| Region or file | Consumer | Compiled variants | Best bytes; differences | Representative variant / assessment |
| --- | --- | ---: | --- | --- |
| `battlesport.ai-net.ainet-path-probe-fan.init-from-segment.path-width-store` | `0x403620` | 5 | 208/208; 4 | No match. `ai-width-input` |
| `battlesport.ai-net.forward-probe-add-world` | `0x401420` | 4 | 384/352; 303 | No match. `ai-forward-probe-add-world-plain` |
| `battlesport.ai-net.los-from-camera-target` | `0x401e50` | 4 | 272/272; 129 | No match. `los-camera-capture_buffer_at_call` |
| `battlesport.ai-net.los-from-local-player-fx-offset` | `0x401d50` | 4 | 256/256; 123 | No match. `los-fx-capture_buffer_at_call` |
| `battlesport.ai-net.path-cross-xz` | `0x401180` | 5 | 640/672; 251 | No match. `path-cross-xz-input` |
| `battlesport.ai-net.path-cross-xz` | `0x4026d0` | 5 | 464/496; 109 | No match. `path-cross-xz-both_inputs` |
| `battlesport.ai-net.path-cross-xz` | `0x4028c0` | 5 | 672/688; 135 | No match. `path-cross-xz-double_products` |
| `battlesport.ai-net.path-cross-xz` | `0x402be0` | 5 | 352/384; 126 | No match. `path-cross-xz-both_inputs` |
| `battlesport.ai-net.path-cross-xz` | `0x402d60` | 5 | 400/432; 144 | No match. `path-cross-xz-both_inputs` |
| `battlesport.ai-net.path-dot-xz` | `0x401180` | 5 | 656/672; 301 | No match. `path-dot-xz-input` |
| `battlesport.ai-net.path-dot-xz` | `0x4026d0` | 5 | 480/496; 158 | No match. `path-dot-xz-products` |
| `battlesport.ai-net.path-dot-xz` | `0x4028c0` | 5 | 672/688; 160 | No match. `path-dot-xz-products` |
| `battlesport.ai-net.path-dot-xz` | `0x402be0` | 5 | 368/384; 165 | No match. `path-dot-xz-products` |
| `battlesport.ai-net.path-dot-xz` | `0x402d60` | 5 | 400/432; 146 | No match. `path-dot-xz-products` |
| `battlesport.ai-net.path-probe-clamp-travel-vc5` | `0x403620` | 4 | 208/208; 114 | No match. `ai-clamp-capture_length` |
| `battlesport.ai-net.solve-alt-gun-lead.discriminant` | `0x4024a0` | 11 | 560/560; 0 | **C++ replacement; live proof below.** `ai-discriminant-capture_both_first_operands` |
| `battlesport.ai-net.solve-alt-gun-lead.fast-sqrt-estimate` | `0x4024a0` | 4 | 560/560; 124 | No match. `ai-fast-sqrt-input` |
| `battlesport.ai-net.solve-alt-gun-lead.vector-add` | `0x4024a0` | 4 | 544/560; 101 | No match. `ai-solve-alt-gun-lead.vector-add-plain` |
| `battlesport.ai-net.solve-alt-gun-lead.vector-add` | `0x4026d0` | 4 | 464/496; 293 | No match. `ai-solve-alt-gun-lead.vector-add-plain` |
| `battlesport.ai-net.solve-alt-gun-lead.vector-add` | `0x4028c0` | 4 | 656/688; 584 | No match. `ai-solve-alt-gun-lead.vector-add-input` |
| `battlesport.ai-net.solve-alt-gun-lead.vector-dot-xyz` | `0x4024a0` | 5 | 496/560; 284 | No match. `solve-alt-gun-lead.vector-dot-xyz-input` |
| `battlesport.ai-net.vector-subtract` | `0x401180` | 5 | 624/672; 460 | No match. `ai-vector-subtract-input-v2` |
| `battlesport.ai-net.vector-subtract` | `0x401580` | 5 | 288/288; 246 | No match. `ai-vector-subtract-product-v2` |
| `battlesport.ai-net.vector-subtract` | `0x401710` | 5 | 640/596; 496 | No match. `ai-vector-subtract-plain-v2` |
| `battlesport.ai-net.vector-subtract` | `0x401c60` | 5 | 240/240; 183 | No match. `ai-vector-subtract-plain-v2` |
| `battlesport.ai-net.vector-subtract` | `0x402090` | 5 | 224/224; 184 | No match. `ai-vector-subtract-input-v2` |
| `battlesport.ai-net.vector-subtract` | `0x402170` | 5 | 224/224; 184 | No match. `ai-vector-subtract-input-v2` |
| `battlesport.ai-net.vector-subtract` | `0x4024a0` | 5 | 512/560; 457 | No match. `ai-vector-subtract-plain-v2` |
| `battlesport.ai-net.vector-subtract` | `0x4026d0` | 5 | 432/496; 363 | No match. `ai-vector-subtract-plain-v2` |
| `battlesport.ai-net.vector-subtract` | `0x4028c0` | 5 | 656/688; 262 | No match. `ai-vector-subtract-input-v2` |
| `battlesport.ai-net.vector-subtract` | `0x402be0` | 5 | 352/384; 282 | No match. `ai-vector-subtract-product-v2` |
| `battlesport.ai-net.vector-subtract` | `0x402d60` | 5 | 400/432; 328 | No match. `ai-vector-subtract-product-v2` |
| `battlesport.ai-net.vector-subtract` | `0x403620` | 5 | 192/208; 159 | No match. `ai-vector-subtract-product-v2` |
| `gamezrecoil.zgame.check-cpu-signature-mask` | `0x4b3050` | 0 | Hardware inspection | Execute CPUID and mask the processor signature. |
| `gamezrecoil.zgame.has-cpuid-support-runtime-options` | `0x4b2fe0` | 0 | Hardware inspection | Toggle and read EFLAGS.ID to detect CPUID support. |
| `gamezrecoil.zgame.has-mmx-support` | `0x4b3020` | 0 | Hardware inspection | Execute CPUID and inspect its MMX feature bit. |
| `gamezrecoil.zgame.options-runtime-config.load-cpu-vendor-string` | `0x4b3160` | 0 | Hardware inspection | Obtain the vendor string from CPUID result registers. |
| `gamezrecoil.zmath.fast-exp-bits` | `0x405040` | 3 | 1504/1552; 860 | No match. `fast-exp-input` |
| `gamezrecoil.zmath.fast-exp-bits` | `0x4059a0` | 3 | 736/752; 433 | No match. `fast-exp-input` |
| `gamezrecoil.zmath.sin-cos` | `0x405040` | 3 | 1504/1552; 613 | No match. `sin-cos-input` |
| `gamezrecoil.zmath.vec3-normalize` | `0x402f60` | 8 | 96/98; 81 | No match. `normalize-capture_inputs-canonical-optimization` |
| `gamezrecoil.zmath.vector-add` | `0x405650` | 5 | 400/384; 323 | No match. `math-vector-add-plain` |
| `gamezrecoil.zmath.vector-add` | `0x4059a0` | 5 | 704/752; 186 | No match. `math-vector-add-pointer-captures` |
| `gamezrecoil.zmath.vector-add` | `0x405ee0` | 5 | 528/560; 427 | No match. `math-vector-add-plain` |
| `gamezrecoil.zmath.vector-direction` | `0x404e90` | 4 | 464/432; 340 | No match. `direction-capture_products` |
| `gamezrecoil.zmath.vector-direction` | `0x405040` | 4 | 1552/1552; 1240 | No match. `direction-capture_inputs` |
| `gamezrecoil.zmath.vector-direction` | `0x405650` | 4 | 384/384; 181 | No match. `direction-capture_inputs` |
| `gamezrecoil.zmath.vector-direction` | `0x405870` | 4 | 304/304; 266 | No match. `direction-double_intermediates` |
| `gamezrecoil.zmath.vector-direction` | `0x406110` | 4 | 832/736; 601 | No match. `direction-capture_inputs` |
| `gamezrecoil.zmath.vector-dot-xz` | `0x405040` | 5 | 1536/1552; 662 | No match. `vector-dot-xz-both_inputs-v2` |
| `gamezrecoil.zmath.vector-length-xz` | `0x405040` | 5 | 1552/1552; 892 | No match. `vector-length-xz-products-v2` |
| `gamezrecoil.zmath.vector-length-xz` | `0x4059a0` | 5 | 736/752; 464 | No match. `vector-length-xz-products-v2` |
| `gamezrecoil.zmath.vector-subtract` | `0x405ee0` | 5 | 528/560; 385 | No match. `math-vector-subtract-plain` |
| `gamezrecoil.zmath.vector-transform-direction` | `0x4059a0` | 4 | 768/752; 350 | No match. `transform-capture_inputs` |
| `gamezrecoil.zrender.fog-blend-span-555-mmx` | `0x49e560` | 3 | 288/352; 224 | No match. `render-fog-blend-span-555-mmx-index_capture-v2` |
| `gamezrecoil.zrender.fog-blend-span-565-mmx` | `0x49e400` | 3 | 288/352; 224 | No match. `render-fog-blend-span-565-mmx-index_capture-v2` |
| `gamezrecoil.zrender.overlay-blend-row-555-mmx` | `0x48d510` | 3 | 144/224; 125 | No match. `render-overlay-blend-row-555-mmx-index_capture-v2` |
| `gamezrecoil.zrender.overlay-blend-row-565-mmx` | `0x48d5f0` | 3 | 144/224; 126 | No match. `render-overlay-blend-row-565-mmx-index_capture-v2` |
| `gamezrecoil.zrender.span-alpha-blend-555-mmx-from-pal8-alpha8` | `0x49ddb0` | 3 | 288/816; 229 | No match. `render-span-alpha-blend-555-mmx-from-pal8-alpha8-index_capture-v2` |
| `gamezrecoil.zrender.span-alpha-blend-555-mmx-from-tex16-alpha8` | `0x49cea0` | 3 | 256/768; 199 | No match. `render-span-alpha-blend-555-mmx-from-tex16-alpha8-index_capture-v2` |
| `gamezrecoil.zrender.span-alpha-blend-565-mmx-from-pal8-alpha8` | `0x49da80` | 3 | 288/816; 230 | No match. `render-span-alpha-blend-565-mmx-from-pal8-alpha8-index_capture-v2` |
| `gamezrecoil.zrender.span-alpha-blend-565-mmx-from-tex16-alpha8` | `0x49cbb0` | 3 | 256/752; 199 | No match. `render-span-alpha-blend-565-mmx-from-tex16-alpha8-index_capture-v2` |
| `gamezrecoil.zrender.span-copy-16-from-pal8-switch-vshift` | `0x49edc0` | 3 | 848/960; 611 | No match. `render-span-copy-16-from-pal8-switch-vshift-index_capture-v2` |
| `gamezrecoil.zrender.span-copy-16-from-tex16` | `0x49ea80` | 3 | 432/416; 285 | No match. `render-span-copy-16-from-tex16-index_capture-v2` |
| `gamezrecoil.zrender.span-copy-16-from-tex16-explicit-vshift` | `0x49ec20` | 3 | 368/416; 261 | No match. `render-span-copy-16-from-tex16-explicit-vshift-index_capture-v2` |
| `gamezrecoil.zrender.span-copy-16-from-tex16-switch-vshift` | `0x49e6c0` | 3 | 768/896; 558 | No match. `render-span-copy-16-from-tex16-switch-vshift-index_capture-v2` |
| `gamezrecoil.zrender.span-masked-16-from-pal8-switch-vshift` | `0x49bbf0` | 3 | 992/1072; 753 | No match. `render-span-masked-16-from-pal8-switch-vshift-index_capture-v2` |
| `gamezrecoil.zrender.span-masked-16-from-tex16-switch-vshift` | `0x49b7e0` | 3 | 816/1040; 610 | No match. `render-span-masked-16-from-tex16-switch-vshift-index_capture-v2` |
| `gamezrecoil.zrender.span-shade-16-from-pal8-switch-vshift` | `0x49f180` | 3 | 1120/1172; 769 | No match. `render-span-shade-16-from-pal8-switch-vshift-index_capture-v2` |
| `gamezrecoil.zsys.cpu-benchmark-resolver.measure-cpu-mhz-cmos-rtc` | `0x4b3b50` | 3 | 336/336; 165 | No match. `cmos-control-v2-capture_cycles` |
| `gamezrecoil.zsys.cpu-benchmark-resolver.measure-cpu-mhz-rdtsc-qpc` | `0x4b38e0` | 0 | Hardware inspection | Read the timestamp counter with RDTSC inside the QPC benchmark. |
| `gamezrecoil.zsys.cpu-benchmark-resolver.measure-mhz-via-bsf-loop-qpc` | `0x4b37f0` | 0 | Hardware inspection | Time a deliberately repeated BSF instruction loop. Removing the dead BSF results removes the measured workload. |
| `gamezrecoil.zsys.detect-is-80286-by-eflags-hi-bits` | `0x4b35a0` | 0 | Hardware inspection | Write/read/restore the high bits of 16-bit FLAGS using PUSHF/POPF. |
| `gamezrecoil.zsys.detect-is-80386-by-ac-flag` | `0x4b35f0` | 0 | Hardware inspection | Toggle/read/restore EFLAGS.AC while preserving the probe stack sequence. |
| `gamezrecoil.zsys.detect-is-8086-by-eflags-hi-bits` | `0x4b3550` | 0 | Hardware inspection | Write/read/restore the high bits of 16-bit FLAGS using PUSHF/POPF. |
| `gamezrecoil.zsys.has-cpuid-support` | `0x4b33f0` | 0 | Hardware inspection | Toggle and read EFLAGS.ID to detect CPUID support. |
| `gamezrecoil.zsys.probe-div-zero-flag-behavior` | `0x4b3510` | 0 | Hardware inspection | Observe processor-dependent FLAGS after 16-bit DIV (0x5555 / 2); despite its provisional name, this is not division by zero. |
| `gamezrecoil.zsys.read-cmos-rtc-seconds-bcd` | `0x4b3b00` | 7 | 32/32; 27 | No match. `cmos-ports-capture_zero_and_result` |
| `gamezrecoil.zsys.read-cpuid-feature-flags` | `0x4b3480` | 0 | Hardware inspection | Execute CPUID leaves and extract feature flags. |
| `gamezrecoil.zsys.read-cpuid-vendor-and-family` | `0x4b3640` | 0 | Hardware inspection | Execute CPUID to obtain vendor and family values. |
| `gamezrecoil.zsys.read-tsc64` | `0x4b3b20` | 0 | Hardware inspection | Read both timestamp-counter halves with RDTSC. |
| `gamezrecoil.zsys.sub64` | `0x4b3ca0` | 4 | 48/64; 47 | No match. `sub64-capture_inputs` |
| `src/Battlesport/hud.cpp` | `0x40a170` | 4 | 80/112; 76 | No match. `provider-copy-capture_dest` |
| `tools/_recoil/compat/coff_aliases/hud_panel_provider_aliases.asm` | `coff-alias` | 0 | Linker inspection | Zero-section WeakExternal alias; no executable instructions or arithmetic to steer. |

## Hardware and linker boundaries

Unused assignments can affect how VC5 schedules or allocates registers for a C++ expression. They do not expose FLAGS, CPUID registers, or the timestamp counter to the C++ abstract machine. The 14 cases assessed without compiler variants require those observations in their existing instruction/call locations, or the specific BSF workload. Replacing them with constants, OS calls, unsupported modern intrinsics, or separately placed raw wrappers would not preserve their behavior and exact call/body contract. The BSF case additionally measures a specific repeated instruction, rather than a mathematical result. This assessment does not establish that all surrounding arithmetic in those bodies must forever remain raw.

VC5 does support `_inp` and `_outp` through `<conio.h>` and emits immediate-port IN/OUT instructions. The older source comment claiming VC5 cannot issue port I/O is too broad. Seven native port-I/O variants were compiled: ordinary calls, port/result captures, narrow results, unsigned-byte results, and split-word storage with or without captures. The shortest variants emit 16 bytes against 32 retail bytes; the two split-word forms reach 32 bytes but still have 27 differing bytes. The retained exception is justified here by the failed tested instruction/body shapes, not by an absence of C++ port intrinsics.

The COFF alias file contains only `.386`, `.model flat`, an `EXTERN`, an `ALIAS`, and `END`. It emits the exact assignment-provider alias and no runtime body. Its associated `std::copy` consumer at `0x40a170` was separately tested in four ordinary C++ forms; all emitted 80 bytes against 112 retail bytes. An unused assignment cannot replace a linker symbol relationship.

## Fresh production validation

Live command: `python tools/recoil.py progress match refresh --all --build-root build/live-validation/raw-unused-native-match-67 --expected-revision 6350 --apply --json`. Detailed result: `build/live-validation/raw-unused-native-match-67/function-match-report.json`.

The refresh committed revision **6351**. `0x4024a0` passes with its complete **560-byte** object body, all **seven relocation type/target/addend checks**, exact linked presence/identity, and relocation-normalized linked body bytes. Comparing every classification with the preceding live build found **no gained or lost matches**: **1,103 byte matches and one instruction match** remain current. The consumer was already byte-matched using assembly; the improvement is obtaining the same proof with C++ for this region.

This is function-body acceptance, not exact linked placement: the candidate consumer is at `0x402490`, while retail is at `0x4024a0`. The serial authored-byte stage and subsequent linked-order/final-image stages are not declared complete by this audit.

`python tools/recoil.py doctor` passed all seven gates after the refresh: agent surface, issue ledger, progress tracker, live validation surface, source policy, function match annotations, and serial pipeline reachability. Output is retained in `build/diagnostics/doctor-raw-unused70.log`. The scoped `git diff --check` also passed.
