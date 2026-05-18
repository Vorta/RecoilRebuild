from __future__ import annotations

import sys
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_frontier import extract_call_tokens, is_indirect_call_token  # noqa: E402


class RecoilFrontierTests(unittest.TestCase):
    def test_indirect_vtable_call_is_not_treated_as_named_symbol(self) -> None:
        tokens = extract_call_tokens(
            "00408f69  ff 50 04        call    dword [eax+0x4]",
            "",
        )

        self.assertEqual(["dword [eax+0x4]"], tokens)
        self.assertTrue(is_indirect_call_token(tokens[0]))

    def test_direct_named_call_is_not_indirect(self) -> None:
        self.assertFalse(is_indirect_call_token("zOpt::GetWindowSection"))


if __name__ == "__main__":
    unittest.main()
