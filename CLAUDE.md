@AGENTS.md

## Claude Code Harness Notes

The imported root `AGENTS.md` above is the authority for mission, stages,
acceptance, evidence ranking, scheduling, and every reconstruction rule. This
section only maps that policy onto the Claude Code harness. It adds no
reconstruction policy and never overrides `AGENTS.md`.

### Canonical Procedure Skills

`.claude/skills/recoil-*/SKILL.md` are pointers, not procedures. Each canonical
body lives at `.codex/skills/<skill>/SKILL.md`; read that file and follow it
verbatim. `python tools/recoil.py audit agent-surface --strict` fails when a
pointer, its description, or its canonical target drifts.

### Role Names

`AGENTS.md` names parent-assigned roles with underscores. Claude subagent types
use hyphens, and the definitions live in `.claude/agents/`:

| AGENTS.md role | Claude subagent type |
| --- | --- |
| `recoil_bn_fact_mapper` | `recoil-bn-fact-mapper` |
| `recoil_bn_reconstructor` | `recoil-bn-reconstructor` |
| `recoil_provider_data_classifier` | `recoil-provider-data-classifier` |
| `recoil_scaffold_auditor` | `recoil-scaffold-auditor` |
| `recoil_source_owner_mapper` | `recoil-source-owner-mapper` |
| `recoil_source_owner_scrutinizer` | `recoil-source-owner-scrutinizer` |
| `recoil_source_worker` | `recoil-source-worker` |
| `recoil_tool_maintainer` | `recoil-tool-maintainer` |
| `recoil_verifier` | `recoil-verifier` |
| `recoil_workspace_librarian` | `recoil-workspace-librarian` |

Each role stub carries the equivalent of its canonical `sandbox_mode` as a tool
restriction and defers to the `developer_instructions` value in its
`.codex/agents/<role>.toml`. No role may spawn a further subagent; the parent
owns scheduling, integration, acceptance, and every ledger mutation.

### Harness Facts

- PowerShell is the primary shell. Every `python tools/recoil.py` command in
  `AGENTS.md` and the canonical skills runs as written.
- Binary Ninja is reached through the `binaryninja` MCP server. Check
  `get_bridge_info` and `list_tools`, then run
  `python tools/recoil.py doctor --quick --binja`. Never load, switch, or patch
  a binary; ask the user to open the correct database instead.
- ChatGPT Pro escalation uses the user-level `chatgpt-pro-line` skill. Only the
  parent may invoke it, and only when a section 5 trigger applies.
- This file is harness mapping, not durable evidence. Durable facts belong in
  source comments, focused reconstruction docs, or the tracker.

### Enforced Gates

`.claude/settings.json` denies edits to `.agent/RECONSTRUCTION_PROGRESS.sqlite3`,
`.agent/WORKSPACE_ISSUES.sqlite3`, and everything under `support/`. Those denials
are mechanical rather than advisory: mutate the tracker only through
`python tools/recoil.py progress ...`, and treat retail input as immutable.

Rules the harness cannot enforce, which remain equally binding:

- The parent/tool orchestrator owns packet branches, linked worktrees, build
  roots, review, integration, retirement, and hygiene. A worker may create only
  one closure-exact nonaccepting packet-id commit in its handed-off worktree.
  Commit existence never accepts reconstruction semantics. Destructive Git
  operations in the primary worktree remain prohibited.
- Never write a progress narrative, and never duplicate live tracker values into
  documentation or source.
- Never clear `.devspace`, and never let a durable fact depend on a concrete
  `.devspace` path.
