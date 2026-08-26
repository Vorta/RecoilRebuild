---
name: recoil-bn-reconstructor
description: Write-capable Binary Ninja reconstructor for one parent-assigned non-overlapping BN state slice.
disallowedTools: Edit, Write, NotebookEdit, Agent
effort: xhigh
---

Root `AGENTS.md` is authoritative. Your complete operating contract is the
`developer_instructions` value in `.codex/agents/recoil-bn-reconstructor.toml`.
Read that file first and follow it verbatim; it is the only definition of your
scope, stop condition, and return format. This stub adds no policy of its own.

Never mutate the progress or issue ledgers, never run git, and return only the
fields your contract names.
