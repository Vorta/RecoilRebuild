#!/usr/bin/env python3
"""Join reconstruction_priority_list.md with RECOIL_PLAN.md markers."""

from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PRIORITY_PATH = ROOT / "temp" / "reconstruction_priority_list.md"
PLAN_PATH = ROOT / ".agent" / "RECOIL_PLAN.md"

ROW_RE = re.compile(
    r"\| (\d+) \| `(0x[0-9a-f]+)` \| `([^`]+)` \| (\d+) \|"
)


def bucket_for_rank(rank: int) -> str:
    if rank <= 660:
        return "P0"
    if rank <= 1231:
        return "P1"
    return "P2+"


def parse_plan(path: Path) -> dict[str, dict[str, str | bool]]:
    entries: dict[str, dict[str, str | bool]] = {}
    cur: str | None = None
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        m = re.match(r"^- (0x[0-9a-fA-F]+):", line)
        if m:
            cur = m.group(1).lower()
            entries[cur] = {
                "recon": False,
                "deps": "--",
                "impl": False,
                "verify": False,
                "impl_name": "",
                "impl_file": "",
            }
            continue
        if not cur:
            continue
        if "Reconstructed" in line and "(Name:" in line:
            entries[cur]["recon"] = "✅" in line or "☑️" in line
        if "Source dependencies satisfied" in line:
            if "\u2705" in line or "[OK]" in line:
                entries[cur]["deps"] = "OK"
            elif "\u2611" in line:
                entries[cur]["deps"] = "LIM"
            elif "\u2753" in line:
                entries[cur]["deps"] = "??"
        if "Reimplemented" in line and "(Name:" in line:
            entries[cur]["impl"] = "✅" in line
            name_m = re.search(r"Name: ([^)]+)", line)
            file_m = re.search(r"File: ([^)]+)", line)
            if name_m:
                entries[cur]["impl_name"] = name_m.group(1).strip()
            if file_m:
                entries[cur]["impl_file"] = file_m.group(1).strip()
        if "Binary-safe verified" in line:
            entries[cur]["verify"] = "✅" in line or "☑️" in line
    return entries


def parse_priority(path: Path) -> list[dict[str, object]]:
    rows: list[dict[str, object]] = []
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        m = ROW_RE.match(line)
        if not m:
            continue
        rank = int(m.group(1))
        rows.append(
            {
                "rank": rank,
                "bucket": bucket_for_rank(rank),
                "address": m.group(2).lower(),
                "name": m.group(3),
                "authored_callees": int(m.group(4)),
            }
        )
    return rows


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--bucket", choices=["P0", "P1", "P2+"], default="P0")
    parser.add_argument("--pending-impl", action="store_true")
    parser.add_argument("--pending-verify", action="store_true")
    parser.add_argument("--file-hint", default="", help="Substring filter on source hint/name")
    parser.add_argument("--limit", type=int, default=30)
    parser.add_argument("--summary", action="store_true")
    args = parser.parse_args()

    plan = parse_plan(PLAN_PATH)
    rows = parse_priority(PRIORITY_PATH)

    if args.summary:
        for bucket in ("P0", "P1", "P2+"):
            sub = [r for r in rows if r["bucket"] == bucket]
            impl_done = sum(1 for r in sub if plan.get(r["address"], {}).get("impl"))
            print(f"{bucket}: total={len(sub)} impl_done={impl_done} pending_impl={len(sub) - impl_done}")
        return 0

    shown = 0
    for row in rows:
        if row["bucket"] != args.bucket:
            continue
        pe = plan.get(row["address"], {})
        if args.pending_impl and pe.get("impl"):
            continue
        if args.pending_verify and not pe.get("impl"):
            continue
        if args.pending_verify and pe.get("verify"):
            continue
        hint = f"{row['name']}"
        if args.file_hint and args.file_hint.lower() not in hint.lower():
            continue
        def mark(ok: bool) -> str:
            return "OK" if ok else "--"

        deps = pe.get("deps", "--")
        print(
            f"{row['rank']:4d} {row['address']} {row['name']} "
            f"recon={mark(pe.get('recon'))} deps={deps} "
            f"impl={mark(pe.get('impl'))} verify={mark(pe.get('verify'))}"
        )
        shown += 1
        if shown >= args.limit:
            break
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
