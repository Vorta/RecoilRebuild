from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

import recoil  # noqa: E402


class RecoilCliParserSmokeTests(unittest.TestCase):
    def test_every_registered_backend_accepts_help(self) -> None:
        failures: list[str] = []
        for spec in recoil.COMMAND_SPECS:
            command = recoil.build_command(spec, ["--help"])
            try:
                completed = subprocess.run(
                    command,
                    cwd=REPO_ROOT,
                    env=recoil.internal_command_env(),
                    capture_output=True,
                    text=True,
                    encoding="utf-8",
                    errors="replace",
                    timeout=30,
                    check=False,
                )
            except subprocess.TimeoutExpired:
                failures.append(f"{spec.name}: timed out while parsing --help")
                continue
            if completed.returncode != 0:
                detail = (completed.stderr or completed.stdout).strip()
                failures.append(f"{spec.name}: rc={completed.returncode}: {detail}")
        self.assertEqual([], failures, "\n\n".join(failures))


if __name__ == "__main__":
    unittest.main()
