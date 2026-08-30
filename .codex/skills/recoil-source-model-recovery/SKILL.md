---
name: recoil-source-model-recovery
description: Recover source-faithful Recoil owners, classes, interfaces, source-file clusters, globals, tables, callbacks, helpers, and translation-unit placement from retail evidence.
---

# Recoil Source Model Recovery

Addresses are evidence keys, not default source units. Expand the target to the smallest higher-order construct supported by evidence: class/interface, source-file cluster, record/callback/table/global-object/static-member group, provider boundary, subsystem, or tightly coupled dependency group.

## Evidence order

Use retail assembly, xrefs, literals, layouts, function order, initialized data, imports, and governed VC5 output as primary evidence. Consult `docs/reconstruction/source_naming_conventions.md` and `support/engine_terminology/` before inventing production names or paths. Current `src/`, historical comments, smokes, and friendly BN names are implementation or navigation aids.

Prefer class/interface recovery when constructors, destructors, offset-zero table writes, `this` usage, inherited cleanup, or dispatch xrefs support it. Do not model original source as hand-authored vtable arrays, raw offsets, dispatch views, ABI scaffolds, provider shims, or helpers invented only to force output.

Recover natural declaration/body header layering and translation-unit contribution order. Do not use new `.inl` files, pragmas, linker-order tricks, duplicate definitions, post-link patching, wrong-file helpers, or production-source `goto` statements.

Ordinary literals, constants, and storage collections are auxiliary data attached to a primary owner unless direct evidence proves an authored construct. Unknown extents stay unknown.

## Ambiguity and Pro

Reason directly from BN, retail, source, and VC5 evidence first. Use the packaged ChatGPT Pro line only when at least two credible source-owner/block/order models remain, a materially disputed correction crosses an owner/provider/TU/physical-block boundary, raw inline assembly is proposed after credible C/C++ variants fail, or the user explicitly requests external critique. The current agent makes the Pro call directly and treats the result as advisory.

## Recording the result

State the proposed owner boundary, source placement, membership, artifact relationships, confidence, and unresolved alternatives. Update source docblocks and tracker relationships through governed commands. Manual positive owner/model changes are dry-run first. Source-model recovery alone does not accept provider status, order, bytes, gates, tier, or final-image coverage.
