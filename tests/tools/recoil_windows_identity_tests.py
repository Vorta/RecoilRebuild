from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.lib.progress import (  # noqa: E402
    ProgressError,
    authenticate_explicit_output_marker,
    explicit_output_marker_record,
)
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

    def test_output_root_marker_binds_allocation_and_directory_identity(self) -> None:
        allocation = {
            "schema": "recoil-explicit-output-root-owner-v3",
            "packet_id": "packet:1",
            "reservation_id": "packet:1:attempt:1",
            "normalized_output_root": "build/packet-1",
            "ownership_sidecar": "build/.packet-1.recoil-explicit-allocation-owner.json",
            "repository_root_identity": str(REPO_ROOT.resolve()),
            "tracker_identity": {"storage_kind": "sqlite", "user_version": 1},
            "issue_ledger_identity": {"ledger_version": 1},
            "operation_nonce": "allocation-operation-1",
        }
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary) / "output"
            root.mkdir()
            marker = explicit_output_marker_record(allocation, root)
            identity = authenticate_explicit_output_marker(marker, allocation, root)
            self.assertEqual(physical_identity(root, directory=True).file_id, identity["file_id"])
            copied_marker = dict(marker)
            old_root = Path(temporary) / "old-output"
            root.replace(old_root)
            root.mkdir()
            with self.assertRaisesRegex(ProgressError, "physically|physical"):
                authenticate_explicit_output_marker(copied_marker, allocation, root)


if __name__ == "__main__":
    unittest.main()
