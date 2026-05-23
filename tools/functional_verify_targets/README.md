# Functional Verification Targets

Functional targets are default-lane evidence for functions that are already
reconstructed, dependency-ready, and implemented. They prove behavior well
enough to mark `Functional-equivalent` and unblock callers in the functional
lane.

Each JSON target must name one original address, the production source file,
the native smoke tests that prove behavior, and the VC byte-attempt command when
available. Functional-only targets list known binary-safety limits. Targets whose
current VC COFF byte comparison already passed may leave `known_limits` empty
and record `binary_safe_evidence` instead. Passing `recoil_functional_verify.py`
supports a `Functional-equivalent` marker; `Binary-safe` still requires the
separate VC evidence named in the target.
