# Functional Verification Targets

Functional targets are tracked functional-verification evidence for reconstructed,
dependency-ready, implemented addresses. They support `Reimplemented [C]` only
when the source also satisfies the build/source contract, raw-offset,
original-source helper, and canonical source-trace requirements.

Each JSON target must name one original address, the production source file,
the native smoke tests that prove behavior, and the VC byte-attempt command when
available. The target id is the lowercase snake_case JSON filename stem, and the
manifest `name` field must match it exactly. Register or refresh only the
selected target through a revision-guarded dry-run:

```powershell
python tools/recoil.py progress verification-target sync --target <target-id> --expected-revision <revision> --dry-run --json
```

Review the result before repeating the exact command with `--apply`.
Functional-only targets list known tier `S` limits. Tier `B` requires accepted
source-owner/data/linkage gates. Targets whose current VC COFF byte comparison
already passed may leave `known_limits` empty and record `tier_s_evidence`
instead. Passing
`python tools/recoil.py verify functional <target-or-address>` supports tier
`C` only with the other source gates; tier `S` still requires accepted
VC/provider evidence.

This directory contains tracked, governed manifests. Edit them directly in the
canonical checkout only when the selected task or explicit maintenance request
authorizes the target change. A manifest or passing smoke accepts no owner,
gate, tier, order, byte, provider, storage, or final-image fact by itself.
Durable evidence belongs in source docblocks/comments, the progress tracker
through governed `python tools/recoil.py progress ...` commands,
`docs/reconstruction/`, or narrow subsystem docs.
