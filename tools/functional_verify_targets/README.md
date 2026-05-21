# Functional Verification Targets

Functional targets are optional progress-lane evidence for functions that are
already reconstructed, dependency-ready, implemented, and blocked only by a
reviewed VC byte-verification mismatch.

Each JSON target must name one original address, the production source file,
the native smoke tests that prove behavior, the VC byte-attempt command, and
known binary-safety limits. Passing `recoil_functional_verify.py` supports a
`Functional-equivalent accepted` marker only; it does not satisfy
`Binary-safe verified`.
