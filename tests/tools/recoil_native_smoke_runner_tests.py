from __future__ import annotations

import contextlib
import importlib.util
import io
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest import mock


REPO_ROOT = Path(__file__).resolve().parents[2]
RUNNER_PATH = REPO_ROOT / "tests" / "native" / "run_native_smokes.py"
RUNNER_SPEC = importlib.util.spec_from_file_location(
    "recoil_native_smoke_runner",
    RUNNER_PATH,
)
assert RUNNER_SPEC is not None
assert RUNNER_SPEC.loader is not None
RUNNER = importlib.util.module_from_spec(RUNNER_SPEC)
RUNNER_SPEC.loader.exec_module(RUNNER)


class NativeSmokeRunnerTests(unittest.TestCase):
    def test_each_smoke_has_a_finite_timeout(self) -> None:
        with tempfile.TemporaryDirectory() as temporary_directory:
            temporary_path = Path(temporary_directory)
            smoke_cpp = temporary_path / "smoke.cpp"
            executable = temporary_path / "recoil_native_smoke.exe"
            smoke_cpp.write_text(
                '{"volume_regression_smoke", volume_regression_smoke},\n',
                encoding="utf-8",
            )

            expired = subprocess.TimeoutExpired(
                cmd=[str(executable), "volume_regression_smoke"],
                timeout=0.25,
                output=b"partial output\n",
            )
            output = io.StringIO()
            with mock.patch.object(RUNNER.subprocess, "run", side_effect=expired) as run:
                with contextlib.redirect_stdout(output):
                    result = RUNNER.main(
                        [
                            str(executable),
                            "--smoke-cpp",
                            str(smoke_cpp),
                            "--timeout-seconds",
                            "0.25",
                        ]
                    )

            self.assertEqual(1, result)
            self.assertEqual(0.25, run.call_args.kwargs["timeout"])
            self.assertIn(
                "[FAIL] volume_regression_smoke: timeout after 0.25 seconds",
                output.getvalue(),
            )
            self.assertIn("partial output", output.getvalue())


if __name__ == "__main__":
    unittest.main()
