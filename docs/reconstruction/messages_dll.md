# messages.dll Reconstruction

`support/messages.dll` is treated as an authored companion binary for Recoil,
not as a provider DLL. The current reference facts are captured in
`.agent/REFERENCE_MESSAGES_DLL.json` and should be verified with:

```powershell
python tools/recoil.py verify pe --reference support/messages.dll --manifest .agent/REFERENCE_MESSAGES_DLL.json --verify
```

Binary Ninja evidence for this binary comes from the already-open
`messages.bndb`. Use the target-aware preflight before address-led DLL work:

```powershell
python tools/recoil.py binja preflight --binary messages --strict
```

The bridge supports target-qualified requests, so agents should pass
`--binary messages` instead of switching Binary Ninja tabs. Do not call
`select_binary` directly from an agent.

## Unified Progress State

Companion owners, symbols, verification targets, work items, blockers, and
semantic observations are binary-qualified entities in
`.agent/RECONSTRUCTION_PROGRESS.json`. Inspect them with joined selectors:

```powershell
python tools/recoil.py progress find ZLocGetID
python tools/recoil.py progress show messages:0x10001010
python tools/recoil.py progress owner relationships messages:0x10001010
python tools/recoil.py progress audit --scope all --strict
```

Companion work items remain structured, binary-qualified state in the same
tracker. They never create a second no-target scheduler: ordinary `messages.dll`
work remains deferred unless explicitly requested or required by the active
Recoil.exe cursor.

## Current Source Shape

The reconstructed source lives under `src/Messages/`:

- `messages.mc` is generated from the retail RT_MESSAGETABLE payload.
- `messages_lookup.inc` is generated from the retail `ZLocGetID` lookup table at
  `0x10006030..0x10007798`.
- `messages.c` implements the exported `ZLocGetID` lookup over the generated
  rows.
- `messages.def` exports `ZLocGetID` at ordinal 1.

Regenerate the message-table and lookup sources from the reference DLL with:

```powershell
python tools/recoil.py build resource --reference support/messages.dll --manifest .agent/MESSAGES_RESOURCE_MANIFEST.json --no-rc --messages-mc src/Messages/messages.mc --messages-lookup src/Messages/messages_lookup.inc
```

Run focused source-surface checks for the companion source with:

```powershell
python tools/recoil.py guard original-symbol --root src/Messages --max 20
python tools/recoil.py audit docblocks --path src/Messages --summary --max 20
python tools/recoil.py doctor --binary messages --quick
```

Known retail facts from the current generator/tests:

- image base `0x10000000`
- entry point RVA `0x1190`
- one export: `ZLocGetID`, ordinal 1, retail RVA `0x1010`
- imports only `KERNEL32.dll`
- one RT_MESSAGETABLE resource, language `0x409`, size 24176 bytes
- 749 message-table entries and 749 lookup rows
- lookup/message IDs range from `0x1` (`MSG_BACK`) through `0x3046`
  (`MSG_WOL_RESTART_REQUIRED`)

## VC5 Build

The VC5SP3 companion build manifest is
`tools/_recoil/config/vc5_messages_build.json`.

Dry-run the command graph:

```powershell
python tools/recoil.py verify final-build --manifest tools/_recoil/config/vc5_messages_build.json --dry-run
```

Compile MC/RC/object inputs without linking:

```powershell
python tools/recoil.py verify final-build --manifest tools/_recoil/config/vc5_messages_build.json --compile-only --clean
```

Validate the compiled message-table resource:

```powershell
python tools/recoil.py build resource --reference support/messages.dll --compare-res build/vc5-messages/messages.res
```

Build and compare resources while intentionally skipping PE byte comparison:

```powershell
python tools/recoil.py verify final-build --manifest tools/_recoil/config/vc5_messages_build.json --no-pe-compare
```

Obtain live companion status from current commands rather than this note:

```powershell
python tools/recoil.py doctor --binary messages --quick
python tools/recoil.py progress show messages:0x10001010
python tools/recoil.py verify final-build --manifest tools/_recoil/config/vc5_messages_build.json --dry-run
```

For acceptance evidence, run the non-dry-run build and resource comparison
above. Any reported PE/export/import/code difference remains active
reconstruction debt and cannot justify authored tier `S`.
