from __future__ import annotations

import argparse
from pathlib import Path
import sys

from _recoil.commands import vc5_build
from _recoil.lib.tooling import configure_stdio


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Compile and link current VC5SP3 source, compare one linked function-order "
            "target semantically, and stop before byte, resource, or PE validation."
        )
    )
    parser.add_argument("target", help="Registered VC5 linked-order target name")
    parser.add_argument("--scope", required=True, choices=("authored", "full"))
    parser.add_argument("--build-root", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, default=vc5_build.DEFAULT_MANIFEST)
    parser.add_argument("--progress", type=Path, default=vc5_build.DEFAULT_PROGRESS)
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Print the compile/link plan without running tools or comparing order.",
    )
    return parser


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = build_parser().parse_args(argv)
    if args.dry_run:
        try:
            config = vc5_build.with_explicit_build_dir(
                vc5_build.load_config(args.manifest),
                args.build_root,
            )
        except (OSError, ValueError) as exc:
            print(str(exc), file=sys.stderr)
            return 2
        print(f"VC5SP3 linked-order target: {args.target}")
        print(f"Order scope: {args.scope}")
        print(f"Build directory: {config.build_dir}")
        print("Plan: compile current configured sources, link one candidate, parse its MAP, compare the selected linked order, then stop.")
        print("Excluded: persisted order artifacts, PE/resource comparison, byte comparison, and raw whole-file comparison.")
        return 0
    forwarded = [
        "--manifest",
        str(args.manifest),
        "--build-dir",
        str(args.build_root),
        "--progress",
        str(args.progress),
        "--order-scope",
        args.scope,
        "--order-target",
        args.target,
        "--linked-order-only",
    ]
    forwarded.append("--clean")
    return vc5_build.main(forwarded)


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
