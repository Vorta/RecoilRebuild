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

## SaveLoadEntryCount

Evidence:
- Caller addresses: `0x434fb0` `HudUiSaveLoadDialog::DeleteSaveFile`,
  `0x435160` `HudUiSaveLoadNextButton::OnActivate`, and `0x4351b0`
  `HudUiSaveLoadPrevButton::OnActivate`.
- Repeated instruction/source pattern: each caller checks whether
  `fileEntries.begin` is null, returns zero when it is null, otherwise computes
  `(fileEntries.end - fileEntries.begin)` for `HudUiSaveLoadEntry` records.
- Likely original owner/source file: save/load dialog source cluster under
  `Battlesport/RecoilApp.cpp` / `HudUiSaveLoadDialog.cpp`.
- Why no standalone retail function is expected: Binary Ninja shows the count
  expression inlined at every observed caller and no standalone call target for
  this helper.

Restored source form:
- `inline int SaveLoadEntryCount(const HudUiSaveLoadDialog *dialog)` in the
  anonymous namespace for the save/load dialog source cluster.
- Callers using it: `HudUiSaveLoadDialog::DeleteSaveFile`,
  `HudUiSaveLoadNextButton::OnActivate`, and
  `HudUiSaveLoadPrevButton::OnActivate`.

Verification notes:
- Native tests: save/load delete, next, and prev button functional smokes
  exercise callers through their class methods.
- VC byte or source-cluster attempt: no standalone helper byte target exists;
  caller tier `S` remains deferred to the save/load dialog/button cluster.

Open limits:
- The helper is accepted as recovered inline source shape only; it is not a
  tier `S` byte marker for any caller.
