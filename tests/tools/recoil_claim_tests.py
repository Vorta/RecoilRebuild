from __future__ import annotations

from datetime import UTC, datetime, timedelta
from pathlib import Path
import subprocess
import sys
import tempfile
import textwrap
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))
CLI = REPO_ROOT / "tools" / "recoil_claim.py"

from recoil_claim import (  # noqa: E402
    Claim,
    ClaimError,
    claim_addresses,
    claim_path,
    claim_to_json,
    first_unclaimed_unfinished,
    load_claim,
    prune_stale_claims,
    read_claim,
    release_addresses,
)
from recoil_plan import PlanDocument  # noqa: E402


PLAN_TEXT = textwrap.dedent(
    """\
    # Test Plan

    ## M01 Test

    - 0x401000:
      - [✅] Reconstructed (Name: Claimed)
      - [✅] Source dependencies satisfied
      - [✅] Reimplemented (Name: Claimed File: src/Claimed.cpp)
      - [❌] Binary-safe verified

    - 0x401020:
      - [❌] Reconstructed (Name: NextFree)
      - [❌] Source dependencies satisfied
      - [❌] Reimplemented (Name: pending File: pending)
      - [❌] Binary-safe verified
    """
)


class RecoilClaimTests(unittest.TestCase):
    def test_claim_uses_normalized_address_filename(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            claims_dir = Path(tmp)

            claims = claim_addresses(
                ["401000"],
                owner="agent-a",
                ttl_hours=1,
                reason="test",
                claims_dir=claims_dir,
            )

            self.assertEqual(["0x401000"], [claim.address for claim in claims])
            self.assertTrue((claims_dir / "0x401000.json").exists())
            loaded = load_claim(claims_dir / "0x401000.json")
            self.assertEqual("agent-a", loaded.owner)
            self.assertEqual("test", loaded.reason)

    def test_duplicate_claim_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            claims_dir = Path(tmp)
            claim_addresses(["0x401000"], owner="agent-a", ttl_hours=1, reason="", claims_dir=claims_dir)

            with self.assertRaises(ClaimError):
                claim_addresses(["0x401000"], owner="agent-b", ttl_hours=1, reason="", claims_dir=claims_dir)

            self.assertEqual("agent-a", read_claim(claims_dir, "0x401000").owner)  # type: ignore[union-attr]

    def test_multi_address_claim_rolls_back_on_conflict(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            claims_dir = Path(tmp)
            claim_addresses(["0x401000"], owner="agent-a", ttl_hours=1, reason="", claims_dir=claims_dir)

            with self.assertRaises(ClaimError):
                claim_addresses(
                    ["0x401020", "0x401000"],
                    owner="agent-b",
                    ttl_hours=1,
                    reason="group",
                    claims_dir=claims_dir,
                )

            self.assertFalse((claims_dir / "0x401020.json").exists())
            self.assertEqual("agent-a", read_claim(claims_dir, "0x401000").owner)  # type: ignore[union-attr]

    def test_release_requires_matching_token(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            claims_dir = Path(tmp)
            claim = claim_addresses(
                ["0x401000"],
                owner="agent-a",
                ttl_hours=1,
                reason="",
                claims_dir=claims_dir,
            )[0]

            with self.assertRaises(ClaimError):
                release_addresses(["0x401000"], token="wrong", claims_dir=claims_dir)

            released = release_addresses(["0x401000"], token=claim.token, claims_dir=claims_dir)
            self.assertEqual(["0x401000"], [item.address for item in released])
            self.assertFalse((claims_dir / "0x401000.json").exists())

    def test_multi_release_validates_before_unlinking(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            claims_dir = Path(tmp)
            first = claim_addresses(
                ["0x401000"],
                owner="agent-a",
                ttl_hours=1,
                reason="",
                claims_dir=claims_dir,
            )[0]
            claim_addresses(["0x401020"], owner="agent-b", ttl_hours=1, reason="", claims_dir=claims_dir)

            with self.assertRaises(ClaimError):
                release_addresses(["0x401000", "0x401020"], token=first.token, claims_dir=claims_dir)

            self.assertTrue((claims_dir / "0x401000.json").exists())
            self.assertTrue((claims_dir / "0x401020.json").exists())

    def test_prune_stale_removes_expired_claims(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            claims_dir = Path(tmp)
            expired = Claim(
                address="0x401000",
                owner="agent-a",
                token="token",
                pid=1,
                host="host",
                cwd=str(REPO_ROOT),
                created_at_utc=datetime.now(UTC) - timedelta(hours=2),
                expires_at_utc=datetime.now(UTC) - timedelta(hours=1),
                reason="expired",
            )
            claim_path(claims_dir, expired.address).write_text(claim_to_json(expired), encoding="utf-8")

            pruned = prune_stale_claims(claims_dir)

            self.assertEqual(["0x401000"], [claim.address for claim in pruned])
            self.assertFalse((claims_dir / "0x401000.json").exists())

    def test_first_unclaimed_unfinished_skips_claimed_entries(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            plan = root / "RECOIL_PLAN.md"
            claims_dir = root / "claims"
            plan.write_text(PLAN_TEXT, encoding="utf-8")
            claim_addresses(["0x401000"], owner="agent-a", ttl_hours=1, reason="", claims_dir=claims_dir)

            entry = first_unclaimed_unfinished(PlanDocument.load(plan), claims_dir)

            self.assertIsNotNone(entry)
            self.assertEqual("0x401020", entry.address)

    def test_cli_next_can_claim_first_unclaimed_entry(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            plan = root / "RECOIL_PLAN.md"
            claims_dir = root / "claims"
            plan.write_text(PLAN_TEXT, encoding="utf-8")

            result = subprocess.run(
                [
                    sys.executable,
                    str(CLI),
                    "--claims-dir",
                    str(claims_dir),
                    "next",
                    "--plan",
                    str(plan),
                    "--claim",
                    "--owner",
                    "agent-a",
                    "--reason",
                    "smoke",
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("0x401000", result.stdout)
            self.assertIn("claim:", result.stdout)
            self.assertEqual("agent-a", read_claim(claims_dir, "0x401000").owner)  # type: ignore[union-attr]


if __name__ == "__main__":
    unittest.main()
