from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
REFERENCE_EXE = REPO_ROOT / "support" / "Recoil.exe"
REFERENCE_MESSAGES_DLL = REPO_ROOT / "support" / "messages.dll"
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.resource_extract import (  # noqa: E402
    bitmap_file_from_dib,
    extract_resources,
    make_messages_lookup_inc,
    make_messages_mc,
    make_res_file,
    message_table_resource,
    parse_message_table,
    parse_res_payloads,
    parse_resources,
    parse_zloc_lookup_rows,
    resource_type_label,
)


class RecoilResourceExtractTests(unittest.TestCase):
    def test_support_recoil_exe_resource_facts(self) -> None:
        if not REFERENCE_EXE.exists():
            self.skipTest("support/Recoil.exe is a local-only reference input")

        _rsrc_rva, _rsrc_size, entries, data = parse_resources(REFERENCE_EXE)
        counts: dict[int | str, int] = {}
        for entry in entries:
            counts[entry.type_id] = counts.get(entry.type_id, 0) + 1

        self.assertEqual(1, counts[2])
        self.assertEqual(1, counts[3])
        self.assertEqual(1, counts[4])
        self.assertEqual(17, counts[5])
        self.assertEqual(1, counts[6])
        self.assertEqual(1, counts[14])
        self.assertEqual(1, counts[16])

        gamebmp = next(entry for entry in entries if entry.type_id == 2 and entry.name == "GAMEBMP")
        self.assertEqual(308266, gamebmp.size)
        self.assertEqual(
            "BM",
            bitmap_file_from_dib(data[gamebmp.file_offset : gamebmp.file_offset + gamebmp.size])[
                :2
            ].decode("ascii"),
        )

    def test_extract_resources_writes_manifest_rc_and_images(self) -> None:
        if not REFERENCE_EXE.exists():
            self.skipTest("support/Recoil.exe is a local-only reference input")

        with tempfile.TemporaryDirectory() as temp_root_raw:
            temp_root = Path(temp_root_raw)
            raw_dir = temp_root / "raw"
            img_dir = temp_root / "img"
            manifest_path = temp_root / "RESOURCE_MANIFEST.json"
            rc_path = temp_root / "Recoil.rc"
            header_path = temp_root / "Resource.h"

            entries = extract_resources(
                REFERENCE_EXE,
                None,
                img_dir,
                manifest_path,
                None,
                rc_path,
                header_path,
            )

            self.assertEqual(23, len(entries))
            self.assertFalse(raw_dir.exists())
            self.assertTrue((img_dir / "GAMEBMP.bmp").exists())
            self.assertTrue((img_dir / "RECOIL.ico").exists())
            rc_text = rc_path.read_text(encoding="utf-8")
            self.assertIn('GAMEBMP BITMAP "', rc_text)
            self.assertIn('151 ICON "', rc_text)
            self.assertIn("MYMENU MENU", rc_text)
            self.assertIn("103 DIALOG 20, 40, 155, 126", rc_text)
            self.assertIn("STRINGTABLE", rc_text)
            self.assertIn("1 VERSIONINFO", rc_text)
            self.assertIn("#define IDS_GAME_FILE_FILTER 200", header_path.read_text(encoding="utf-8"))

            manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
            self.assertEqual("0x37a000", manifest["resource_directory"]["rva"])
            self.assertEqual("bitmap", resource_type_label(manifest["entries"][0]["type_id"]))

    def test_generated_res_parser_reads_all_payloads(self) -> None:
        if not REFERENCE_EXE.exists():
            self.skipTest("support/Recoil.exe is a local-only reference input")

        _rsrc_rva, _rsrc_size, entries, data = parse_resources(REFERENCE_EXE)
        with tempfile.TemporaryDirectory() as temp_root_raw:
            res_path = Path(temp_root_raw) / "Recoil.res"
            res_path.write_bytes(make_res_file(entries, data))
            payloads = parse_res_payloads(res_path)

        self.assertEqual(23, len(payloads))
        self.assertTrue(any(payload.type_id == 4 and payload.name == "MYMENU" for payload in payloads))

    def test_support_messages_dll_message_table_and_lookup_facts(self) -> None:
        if not REFERENCE_MESSAGES_DLL.exists():
            self.skipTest("support/messages.dll is a local-only reference input")

        _rsrc_rva, _rsrc_size, entries, data = parse_resources(REFERENCE_MESSAGES_DLL)
        message_entry, payload = message_table_resource(entries, data)
        messages = parse_message_table(payload)
        lookup_rows = parse_zloc_lookup_rows(REFERENCE_MESSAGES_DLL)

        self.assertEqual(11, message_entry.type_id)
        self.assertEqual(24176, message_entry.size)
        self.assertEqual(749, len(messages))
        self.assertEqual(749, len(lookup_rows))
        self.assertEqual(0x1, messages[0].message_id)
        self.assertEqual("MSG_BACK", lookup_rows[0].name)
        self.assertEqual(0x3046, messages[-1].message_id)
        self.assertEqual("MSG_WOL_RESTART_REQUIRED", lookup_rows[-1].name)
        self.assertEqual(
            {entry.message_id for entry in messages},
            {row.message_id for row in lookup_rows},
        )

    def test_messages_dll_generators_write_mc_and_lookup_include(self) -> None:
        if not REFERENCE_MESSAGES_DLL.exists():
            self.skipTest("support/messages.dll is a local-only reference input")

        _rsrc_rva, _rsrc_size, entries, data = parse_resources(REFERENCE_MESSAGES_DLL)
        _message_entry, payload = message_table_resource(entries, data)
        messages = parse_message_table(payload)
        lookup_rows = parse_zloc_lookup_rows(REFERENCE_MESSAGES_DLL)

        mc_text = make_messages_mc(REFERENCE_MESSAGES_DLL, lookup_rows, messages)
        lookup_text = make_messages_lookup_inc(REFERENCE_MESSAGES_DLL, lookup_rows)

        self.assertIn("MessageId=0x1", mc_text)
        self.assertIn("SymbolicName=MSG_BACK", mc_text)
        self.assertIn("Language=English", mc_text)
        self.assertIn("MessageId=0x3046", mc_text)
        self.assertNotIn("\0", mc_text)
        self.assertIn('static char g_MessageName_MSG_BACK[] = "MSG_BACK";', lookup_text)
        self.assertIn("#define MESSAGES_LOOKUP_ROW_COUNT 749", lookup_text)


if __name__ == "__main__":
    unittest.main()
