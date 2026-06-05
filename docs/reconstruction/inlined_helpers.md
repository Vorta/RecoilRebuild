# Inlined Helper Recovery Ledger

Use this ledger for likely original helpers or class methods that the retail
compiler fully inlined, leaving no standalone executable function address.
Binary Ninja and caller assembly remain authoritative.

Record only helpers that recur across callers, clarify class/source structure,
or materially reduce duplicate recovered behavior/source patterns. Local
one-off cleanup belongs in source comments beside the caller instead.

Bare `Observed in caller...` wording is not sufficient provenance. A recovered
helper should say that no standalone retail function exists and record why the
inlined body looks like original inline/static/member source rather than a local
reconstruction convenience wrapper.
Use VC5-era `inline`, `static inline`, ordinary `static`, member functions, or
class-body definitions for restored helpers; do not use reconstruction inline
marker macros.

## Entry Pattern

```text
## HelperOrClass::Method

Evidence:
- Caller addresses:
- Repeated instruction/source pattern:
- Likely original owner/source file:
- Why no standalone retail function is expected:

Restored source form:
- inline/static/member helper:
- Callers using it:

Verification notes:
- Native tests:
- VC byte or source-cluster attempt:
- Known tier `S` limits:

Open limits:
- ...
```

## Current Entries

No standalone inlined-helper entries have been promoted yet. When adding the
first entry, keep it compact and include the caller addresses that prove the
helper-like body was duplicated by compiler inlining.
