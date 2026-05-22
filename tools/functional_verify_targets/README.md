# Functional Verification Targets

Functional targets are default-lane evidence for functions that are already
reconstructed, dependency-ready, and implemented. They prove behavior well
enough to mark `Functional-equivalent` and unblock callers in the functional
lane.

Each JSON target must name one original address, the production source file,
the native smoke tests that prove behavior, the VC byte-attempt command when
available, and known binary-safety limits. Passing `recoil_functional_verify.py`
supports a `Functional-equivalent` marker only; it does not satisfy
`Binary-safe`.
