from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.lib.windows_identity import (  # noqa: E402
    StableReadHandle,
    WindowsIdentityError,
    physical_identity,
    require_same_physical_object,
)


class WindowsIdentityTests(unittest.TestCase):
    def test_stable_read_handle_reads_one_physical_file(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "retail.exe"
            path.write_bytes(b"MZfixture")
            before = physical_identity(path)
            with StableReadHandle(path) as stable:
                self.assertEqual(b"MZfixture", stable.read())
                self.assertTrue(before.same_physical_object(stable.identity))
                self.assertEqual(path.resolve(), Path(stable.identity.canonical_path))
            after = physical_identity(path)
        require_same_physical_object(before, after, context="test retail")

    def test_replaced_file_is_rejected_even_at_the_same_path(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "tool.exe"
            replacement = Path(temporary) / "replacement.exe"
            path.write_bytes(b"first")
            expected = physical_identity(path)
            replacement.write_bytes(b"first")
            path.unlink()
            replacement.replace(path)
            observed = physical_identity(path)
            with self.assertRaises(WindowsIdentityError):
                require_same_physical_object(expected, observed, context="toolchain")

if __name__ == "__main__":
    unittest.main()
