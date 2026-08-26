#!/usr/bin/env python3
"""Prove the VC5 selective-inlining profile for the zDEClient vector growth path."""

from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys


REPO_ROOT = Path(__file__).resolve().parents[4]
SOURCE = Path(__file__).with_name("zdeclient_vector_profile.cpp")
DEFAULT_VC5_ROOT = Path("D:/Recoil Project/Compiler/VC5SP3")
DEFAULT_OUTPUT = REPO_ROOT / "build/vc5-probes/zdeclient-vector-profile"
COMPILER_VERSION = "Microsoft (R) 32-bit C/C++ Optimizing Compiler Version 11.00.7022"

APPEND_SYMBOL = "?AppendFeatureEntryProbe@@YIHHPBX@Z"
PUSH_BACK_SYMBOL = (
    "?push_back@?$vector@UFeatureEntry@@V?$allocator@UFeatureEntry@@@std@@@std@@"
    "QAEXABUFeatureEntry@@@Z"
)
UCOPY_SYMBOL = (
    "?_Ucopy@?$vector@UFeatureEntry@@V?$allocator@UFeatureEntry@@@std@@@std@@"
    "IAEPAUFeatureEntry@@PBU3@0PAU3@@Z"
)
UFILL_SYMBOL = (
    "?_Ufill@?$vector@UFeatureEntry@@V?$allocator@UFeatureEntry@@@std@@@std@@"
    "IAEXPAUFeatureEntry@@IABU3@@Z"
)
DESTROY_SYMBOL = (
    "?_Destroy@?$vector@UFeatureEntry@@V?$allocator@UFeatureEntry@@@std@@@std@@"
    "IAEXPAUFeatureEntry@@0@Z"
)
OPERATOR_NEW_SYMBOL = "??2@YAPAXI@Z"
OPERATOR_DELETE_SYMBOL = "??3@YAXPAX@Z"

OB0_CALLS = (PUSH_BACK_SYMBOL,)
OB1_CALLS = (
    OPERATOR_NEW_SYMBOL,
    UFILL_SYMBOL,
    UCOPY_SYMBOL,
    DESTROY_SYMBOL,
    OPERATOR_DELETE_SYMBOL,
    UCOPY_SYMBOL,
    UFILL_SYMBOL,
    UCOPY_SYMBOL,
)


def _proc_body(listing: str, symbol: str) -> str:
    starts = [match.start() for match in re.finditer(re.escape(symbol) + r" PROC NEAR", listing)]
    if len(starts) != 1:
        raise RuntimeError(f"{symbol}: expected one PROC body, found {len(starts)}")
    end_marker = symbol + " ENDP"
    end = listing.find(end_marker, starts[0])
    if end < 0:
        raise RuntimeError(f"{symbol}: missing ENDP")
    return listing[starts[0] : end + len(end_marker)]


def _calls(body: str) -> tuple[str, ...]:
    return tuple(re.findall(r"\bcall\s+([^\s;]+)", body))


def _compile(
    *,
    compiler: Path,
    environment: dict[str, str],
    output_dir: Path,
    inline_flag: str,
) -> tuple[Path, Path, Path]:
    tag = inline_flag.removeprefix("/").lower()
    listing = output_dir / f"{tag}.cod"
    obj = output_dir / f"{tag}.obj"
    log = output_dir / f"{tag}.compile.log"
    command = [
        str(compiler),
        "/nologo",
        "/TP",
        "/W3",
        "/G5",
        "/O2",
        inline_flag,
        "/MD",
        "/GX",
        "/Gr",
        "/Zp4",
        "/FAcs",
        f"/Fa{listing}",
        f"/Fo{obj}",
        "/c",
        str(SOURCE),
    ]
    completed = subprocess.run(
        command,
        cwd=output_dir,
        env=environment,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    log.write_text(
        "command: " + subprocess.list2cmdline(command) + "\n"
        + "returncode: " + str(completed.returncode) + "\n"
        + "stdout:\n" + completed.stdout
        + "\nstderr:\n" + completed.stderr,
        encoding="utf-8",
    )
    if completed.returncode != 0:
        raise RuntimeError(f"{inline_flag}: VC5 compile failed; see {log}")
    if not listing.is_file() or not obj.is_file():
        raise RuntimeError(f"{inline_flag}: compiler omitted the expected listing or object")
    return listing, obj, log


def run_probe(*, vc5_root: Path, output_dir: Path) -> dict[str, object]:
    compiler = vc5_root / "VC/BIN/cl.exe"
    shared_bin = vc5_root / "SHAREDIDE/BIN"
    redist = vc5_root / "VC/REDIST"
    include = vc5_root / "VC/INCLUDE"
    if not compiler.is_file() or not include.is_dir():
        raise RuntimeError(f"VC5SP3 compiler/include tree is unavailable under {vc5_root}")
    if not SOURCE.is_file():
        raise RuntimeError(f"probe source is unavailable: {SOURCE}")

    output_dir.mkdir(parents=True, exist_ok=True)
    environment = dict(os.environ)
    environment["PATH"] = os.pathsep.join(
        (str(compiler.parent), str(shared_bin), str(redist), environment.get("PATH", ""))
    )
    environment["INCLUDE"] = str(include)

    version = subprocess.run(
        [str(compiler)],
        cwd=output_dir,
        env=environment,
        capture_output=True,
        text=True,
        encoding="utf-8",
        errors="replace",
        check=False,
    )
    version_text = version.stdout + version.stderr
    if COMPILER_VERSION not in version_text:
        raise RuntimeError("unexpected compiler version: " + version_text.strip())

    compiled: dict[str, dict[str, object]] = {}
    for inline_flag, expected_calls in (("/Ob0", OB0_CALLS), ("/Ob1", OB1_CALLS)):
        listing_path, obj_path, log_path = _compile(
            compiler=compiler,
            environment=environment,
            output_dir=output_dir,
            inline_flag=inline_flag,
        )
        listing = listing_path.read_text(encoding="utf-8", errors="replace")
        append_body = _proc_body(listing, APPEND_SYMBOL)
        calls = _calls(append_body)
        if calls != expected_calls:
            raise RuntimeError(
                f"{inline_flag}: call sequence drifted; expected {expected_calls!r}, got {calls!r}"
            )
        compiled[inline_flag] = {
            "calls": list(calls),
            "listing": listing_path.resolve().as_posix(),
            "object": obj_path.resolve().as_posix(),
            "compile_log": log_path.resolve().as_posix(),
        }

    ob1_listing = Path(str(compiled["/Ob1"]["listing"])).read_text(
        encoding="utf-8", errors="replace"
    )
    ob1_append = _proc_body(ob1_listing, APPEND_SYMBOL)
    growth_start = ob1_append.find("call\t " + OPERATOR_NEW_SYMBOL)
    growth_fill = ob1_append.find("call\t " + UFILL_SYMBOL, growth_start)
    if growth_start < 0 or growth_fill < 0:
        raise RuntimeError("/Ob1: cannot isolate the growth-path first _Ucopy region")
    inlined_first_ucopy = ob1_append[growth_start:growth_fill]
    if not all(
        token in inlined_first_ucopy
        for token in ("rep movsd", "add\t eax, 52", "add\t ebp, 52")
    ):
        raise RuntimeError("/Ob1: first _Ucopy is not the expected inline 52-byte copy loop")

    helper_returns: dict[str, str] = {}
    for symbol, expected_ret in (
        (UCOPY_SYMBOL, "12"),
        (UFILL_SYMBOL, "12"),
        (DESTROY_SYMBOL, "8"),
    ):
        body = _proc_body(ob1_listing, symbol)
        if not re.search(rf"\bret\s+{expected_ret}\b", body):
            raise RuntimeError(f"/Ob1: {symbol} does not return with ret {expected_ret}")
        helper_returns[symbol] = expected_ret

    result: dict[str, object] = {
        "kind": "zdeclient-vector-profile-probe",
        "contract_version": 1,
        "passed": True,
        "compiler_version": COMPILER_VERSION,
        "source": SOURCE.resolve().as_posix(),
        "common_profile": ["/nologo", "/TP", "/W3", "/G5", "/O2", "/MD", "/GX", "/Gr", "/Zp4", "/FAcs"],
        "profiles": compiled,
        "ob1_growth_path": {
            "first_ucopy": "inlined-52-byte-rep-movsd-loop",
            "direct_call_prefix": list(OB1_CALLS[:5]),
            "helper_stack_cleanup_bytes": helper_returns,
        },
        "decision": (
            "/Ob0 cannot express the retail vector-growth body; /Ob1 naturally emits "
            "the inline first _Ucopy followed by _Ufill, empty-range _Ucopy, _Destroy, "
            "and operator delete."
        ),
    }
    result_path = output_dir / "result.json"
    result["result_path"] = result_path.resolve().as_posix()
    result_path.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    return result


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--vc5-root", type=Path, default=DEFAULT_VC5_ROOT)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args(argv)
    try:
        result = run_probe(
            vc5_root=args.vc5_root.resolve(),
            output_dir=args.output_dir.resolve(),
        )
    except (OSError, RuntimeError) as exc:
        print(json.dumps({"passed": False, "error": str(exc)}, indent=2))
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
