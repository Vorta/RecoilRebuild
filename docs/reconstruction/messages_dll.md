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

## Plan Ledger

The companion address ledger is `.agent/RECOIL_MESSAGES_PLAN.md`. Use the same
plan/status/frontier tools as the main executable, but pass `--binary messages`
so the tools resolve the messages plan path:

```powershell
python tools/recoil.py plan find --binary messages ZLocGetID
python tools/recoil.py plan show --binary messages 0x10001010
python tools/recoil.py status --binary messages 0x10001010 --lane binary
python tools/recoil.py frontier --binary messages 0x10001010 --depth 1 --lane binary
```

Temporary multi-entry or owner-sized WIP belongs in
`.agent/IMPLEMENTATION_GROUPS_MESSAGES.md`, not in the main executable
implementation-groups file. Keep it empty when no companion DLL group is
active, and validate it with:

```powershell
python tools/recoil.py audit groups --binary messages --summary --wip-limit 4
```

Seed or refresh the companion ledger from the already-open `messages.bndb`
through the target-qualified bridge:

```powershell
python tools/recoil.py plan seed-binary --binary messages --dry-run
python tools/recoil.py plan seed-binary --binary messages --apply
```

The seed command inventories every current BN function whose start falls inside
`.text` and every current BN data variable whose root falls inside `.data`.
Function entries start as authored not-done rows unless they match only a
provider/runtime hint; data entries record the current `.data` shape but keep
source-owner and reimplementation markers unresolved. Provider hints are seed
hints only, not accepted provider-boundary evidence.

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
python tools/recoil.py guard original-symbol --root src/Messages --plan .agent/RECOIL_MESSAGES_PLAN.md --max 20
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

As of this note, the resource payload matches retail, the DLL links, and full PE
comparison still reports expected non-accepted differences: export RVA, section
placement/size, timestamp/hash, and import order. Those are active
reconstruction/byte-equivalence work, not accepted tier `S` evidence.
