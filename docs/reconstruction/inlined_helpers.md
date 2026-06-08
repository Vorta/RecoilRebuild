# Inlined Helper Recovery Ledger

Ledger for likely original helpers or class methods fully inlined by the retail
compiler, with no standalone executable address. Binary Ninja and caller
assembly remain authoritative.

Record only recurring helpers that clarify source/class structure or remove
duplicate recovered behavior. One-off cleanup belongs in source comments.

Bare `Observed in caller...` is insufficient. A recovered helper must state that
no standalone retail function exists and explain why the body looks like
original inline/static/member source, not a convenience wrapper.

Use VC5-era `inline`, `static inline`, ordinary `static`, member functions, or
class-body definitions. Do not use reconstruction inline marker macros.

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

## SaveLoadEntryCount

Evidence:
- Caller addresses: `0x434fb0` `HudUiSaveLoadDialog::DeleteSaveFile`,
  `0x435160` `HudUiSaveLoadNextButton::OnActivate`, and `0x4351b0`
  `HudUiSaveLoadPrevButton::OnActivate`.
- Repeated pattern: null `fileEntries.begin` returns zero; otherwise count is
  `fileEntries.end - fileEntries.begin` over `HudUiSaveLoadEntry` records.
- Likely owner/source cluster: save/load dialog code under
  `Battlesport/RecoilApp.cpp` / `HudUiSaveLoadDialog.cpp`.
- No standalone retail function is expected: BN shows the count expression
  inlined in every observed caller and no standalone call target.

Restored source form:
- `inline int SaveLoadEntryCount(const HudUiSaveLoadDialog *dialog)` in the
  anonymous namespace for the save/load dialog source cluster.
- Callers using it: `HudUiSaveLoadDialog::DeleteSaveFile`,
  `HudUiSaveLoadNextButton::OnActivate`, and
  `HudUiSaveLoadPrevButton::OnActivate`.

Verification notes:
- Native tests: save/load delete, next, and prev button smokes exercise callers
  through their class methods.
- VC byte/source-cluster attempt: no standalone helper byte target exists;
  caller tier `S` is deferred to the save/load dialog/button cluster.

Open limits:
- Accepted only as recovered inline source shape, not a tier `S` marker for any
  caller.
