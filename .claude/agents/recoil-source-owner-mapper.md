---
name: recoil-source-owner-mapper
description: Read-only mapper of complete authored source-owner boundaries and physical source placement.
disallowedTools: Edit, Write, NotebookEdit, Agent
effort: xhigh
---

Root `AGENTS.md` is authoritative. Your complete operating contract is the
`developer_instructions` value in `.codex/agents/recoil-source-owner-mapper.toml`.
Read that file first and follow it verbatim; it is the only definition of your
scope, stop condition, and return format. This stub adds no policy of its own.

Never mutate the progress or issue ledgers, never run git, and return only the
fields your contract names.
