# Functional Verification Targets

Functional targets are functional-lane behavior evidence for owner child
addresses that are reconstructed, dependency-ready, and implemented. They
support `Reimplemented [C]` only when the source also satisfies the build/source
contract, raw-offset, original-source helper, and canonical source-trace
requirements.

Each JSON target must name one original address, the production source file,
the native smoke tests that prove behavior, and the VC byte-attempt command when
available. The target id is the lowercase snake_case JSON filename stem, and the
manifest `name` field must match it exactly. When durable owner tracking needs
the target id, record it in the unified tracker's child-address metadata `target`
field through `python tools/recoil.py progress owner set-address-meta`; do not use the
source symbol as the target id. Functional-only targets list known tier `S`
limits. Tier `B` requires accepted source-owner/data/linkage gates. Targets whose
current VC COFF byte comparison already passed may leave `known_limits` empty
and record `tier_s_evidence` instead. Passing
`python tools/recoil.py verify functional <target-or-address>` supports tier
`C` only with the other source gates; tier `S` still requires accepted
VC/provider evidence.

This directory is local ignored verification state. Agents may create or update
targets here for current evidence, but must not run git commands or report
version-control state. Durable evidence belongs in source docblocks/comments,
`.agent/RECONSTRUCTION_PROGRESS.sqlite3` through `python tools/recoil.py progress ...`,
`docs/reconstruction/`, or narrow subsystem docs.
