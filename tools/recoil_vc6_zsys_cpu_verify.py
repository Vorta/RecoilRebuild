from __future__ import annotations

import sys

from recoil_vc6_verify import main as vc6_verify_main


def main(argv: list[str] | None = None) -> int:
    if argv is None:
        argv = sys.argv[1:]
    forwarded = ["zsys_cpu"]
    if argv:
        forwarded.extend(argv)
    return vc6_verify_main(forwarded)


if __name__ == "__main__":
    raise SystemExit(main())
