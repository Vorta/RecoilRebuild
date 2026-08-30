# Direct Work Checklist

Use this checklist at the start and end of a Recoil reconstruction session.

## Start

1. Read `AGENTS.md` and the owning `recoil-*` skill.
2. Inspect the working tree and preserve unrelated changes.
3. Run:

   ```powershell
   python tools/recoil.py progress next --json
   ```

4. Work only the returned serial stage/task unless the user selected an
   explicit target.
5. Use a fresh absent root below `build/live-validation` for every
   compiler-backed check.
6. If Binary Ninja is required, check:

   ```powershell
   python tools/recoil.py doctor --quick --binja
   ```

7. Invoke ChatGPT Pro directly only under the ambiguity triggers in
   `AGENTS.md`.

## Work

- Edit the canonical checkout directly.
- Run the task’s nonmutating check while iterating.
- Run only its direct acceptance command when current source passes.
- Keep manual semantic mutations dry-run-first.
- Do not use candidate output as expected truth.
- Do not hand-edit either SQLite database.

## Finish

1. Run focused unit/source/tool validation.
2. Run the relevant infrastructure checks:

   ```powershell
   python tools/recoil.py audit agent-surface --strict
   python tools/recoil.py audit pipeline-contracts --strict
   python tools/recoil.py audit pipeline-reachability --strict
   python tools/recoil.py progress audit --scope pipeline --strict
   python tools/recoil.py issue audit --strict
   ```

3. Refresh the public README projection only when required:

   ```powershell
   python tools/recoil.py docs readme-progress
   ```

4. Report the outcome, changed paths, exact validation commands, first
   remaining divergence, and narrow next task.
