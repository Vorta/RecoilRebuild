"""Separate a reviewed, unreferenced INT3 alignment tail from a real Recoil function."""
from __future__ import annotations

import argparse
from copy import deepcopy
import json
from pathlib import Path
import re
import sys

from capstone import Cs, CS_ARCH_X86, CS_MODE_32

from _recoil.lib.binja import BinaryNinjaBridge, BridgeError
from _recoil.lib.pe import parse_pe_headers, rva_to_offset
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH, ProgressDocument, ProgressError, ProgressStore, address_value
from _recoil.lib.reference_images import reference_image
from _recoil.lib.tooling import REPO_ROOT, configure_stdio


def _same(left, right):
    return json.dumps(left, sort_keys=True) == json.dumps(right, sort_keys=True)


def plan_separation(document, payload):
    fields = {"schema", "reviewed", "reason", "symbol_id", "current_symbol", "body_end_exclusive", "evidence_ids"}
    if set(payload) != fields or payload["schema"] != "recoil-function-tail-padding-v1":
        raise ProgressError("invalid function-tail-padding payload")
    if payload["reviewed"] is not True or not isinstance(payload["reason"], str) or not payload["reason"].strip():
        raise ProgressError("tail separation requires reviewed=true and a reason")
    symbol_id = payload["symbol_id"]
    symbol = document.collection("symbols").get(symbol_id)
    if not isinstance(symbol, dict) or not _same(symbol, payload["current_symbol"]):
        raise ProgressError("complete function snapshot is stale")
    if (symbol.get("binary") != "recoil" or symbol.get("kind") != "function"
            or symbol.get("pipeline_class") not in {"authored", "authored-lifecycle"}
            or symbol.get("extent_state") != "known"
            or symbol.get("output_section_id") != "recoil:section:.text"):
        raise ProgressError("tail separation requires one known authored .text function")
    if symbol.get("logical_aliases") or symbol.get("icf_address_group") or symbol.get("storage_contribution_ids"):
        raise ProgressError("tail separation does not handle aliases or registered storage")
    if symbol.get("accepted_byte_facts") or symbol.get("tail_padding_separation"):
        raise ProgressError("function already has accepted bytes or a tail separation")
    for name, state in symbol.get("binary_state", {}).items():
        if name != "call_contract" and isinstance(state, dict) and state.get("disposition") == "accepted":
            raise ProgressError("tail separation refuses accepted byte or linked state")
    start = address_value(symbol["address"])
    end = address_value(symbol["end_exclusive"])
    body_end = address_value(payload["body_end_exclusive"])
    if (symbol_id != f"recoil:function:0x{start:x}" or symbol.get("size") != end-start
            or not start < body_end < end or end % 16 or not 1 <= end-body_end <= 15):
        raise ProgressError("tail must be at most 15 bytes ending at the next 16-byte alignment")
    block = document.collection("physical_blocks").get(symbol.get("physical_block_id"), {})
    if (symbol_id not in block.get("contribution_ids", [])
            or not address_value(block["start"]) <= start < end <= address_value(block["end_exclusive"])):
        raise ProgressError("function is not contained in its retained physical block")
    allowed_evidence = set(symbol.get("evidence_ids", [])) | set(block.get("evidence_ids", []))
    evidence = payload["evidence_ids"]
    if (not isinstance(evidence, list) or not evidence or len(evidence) != len(set(evidence))
            or not set(evidence) <= allowed_evidence
            or any(item not in document.collection("evidence") for item in evidence)):
        raise ProgressError("tail separation requires existing function/block evidence")
    for other_id, other in document.collection("symbols").items():
        if other_id == symbol_id or not isinstance(other, dict) or other.get("binary") != "recoil":
            continue
        if other.get("address") and other.get("end_exclusive"):
            if address_value(other["address"]) < end and start < address_value(other["end_exclusive"]):
                raise ProgressError("function overlaps another typed symbol")
    proposed = deepcopy(document.data)
    revised = proposed["symbols"][symbol_id]
    revised["end_exclusive"] = f"0x{body_end:x}"
    revised["size"] = body_end-start
    revised["tail_padding_separation"] = dict(
        start=f"0x{body_end:x}", end_exclusive=f"0x{end:x}", byte_value=0xCC,
        reason=payload["reason"], evidence_ids=evidence, acceptance="unaccepted",
    )
    return proposed, (start, body_end, end)


def prove_tail(retail, start, body_end, end, assembly):
    if len(retail) != end-start or retail[body_end-start:] != b"\xcc" * (end-body_end):
        raise ProgressError("retail tail is not exact INT3 alignment bytes")
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_32).disasm(retail[:body_end-start], start))
    if (not instructions or instructions[-1].address + instructions[-1].size != body_end
            or instructions[-1].mnemonic != "ret"):
        raise ProgressError("retail body must decode completely and end in RET")
    expected = [(ins.address, bytes(ins.bytes)) for ins in instructions]
    observed = []
    for line in assembly.splitlines():
        match = re.match(r"^([0-9a-fA-F]{8,16})\s+((?:[0-9a-fA-F]{2} )*[0-9a-fA-F]{2})\s+\S", line)
        if match:
            observed.append((int(match[1], 16), bytes.fromhex(match[2])))
    if sorted(observed) != expected:
        raise ProgressError("complete BN instruction population disagrees with the retail body boundary")
    for ins in instructions:
        if ins.mnemonic.startswith("j") or ins.mnemonic.startswith("loop"):
            try:
                target = int(ins.op_str, 0)
            except ValueError as exc:
                raise ProgressError("indirect branch leaves the tail boundary unresolved") from exc
            if body_end <= target < end:
                raise ProgressError("a body branch targets the proposed padding")


def prove_live(reference, interval, bridge):
    start, body_end, end = interval
    spec = reference_image("recoil")

    def status():
        row = bridge.get_json("status")
        if (not row.get("loaded") or Path(row.get("filename", "")).resolve() != Path(spec.bndb_path).resolve()
                or row.get("platform") != spec.platform or row.get("arch") != spec.arch
                or row.get("analysis", {}).get("state") != "AnalysisState.IdleState"
                or not row.get("view_identity") or not row.get("session_id")
                or type(row.get("view_revision")) is not int or row["view_revision"] < 0):
            raise ProgressError("expected idle Recoil database is unavailable")
        return tuple(row.get(key) for key in ("session_id", "view_identity", "view_revision"))

    before = status()
    image = reference.read_bytes()
    headers = parse_pe_headers(image, source=str(reference))
    first = rva_to_offset(start-headers.image_base, headers.sections)
    last = rva_to_offset(end-1-headers.image_base, headers.sections)
    if first is None or last != first + end-start-1 or last >= len(image):
        raise ProgressError("retail function is not wholly file-backed")
    section = [s for s in headers.sections if s.virtual_address <= start-headers.image_base
               and end-headers.image_base <= s.virtual_address+s.raw_size]
    if len(section) != 1 or section[0].name != ".text":
        raise ProgressError("retail function is not contained in .text")
    prove_tail(image[first:last+1], start, body_end, end, bridge.assembly(f"0x{start:x}"))
    for address in range(body_end, end):
        at = bridge.get_json("functionAt", address=f"0x{address:x}")
        refs = bridge.get_json("getXrefsTo", address=f"0x{address:x}", limit=-1)
        if address_value(str(at.get("address", ""))) != address or at.get("functions") != []:
            raise ProgressError("BN assigns proposed padding to a function")
        coverage = refs.get("coverage", {})
        if (address_value(str(refs.get("address", ""))) != address
                or coverage.get("complete") is not True or coverage.get("errors")
                or refs.get("partial") or refs.get("truncated") or refs.get("has_more")
                or refs.get("total") != 0 or refs.get("code_references") != [] or refs.get("data_references") != []):
            raise ProgressError("proposed padding has indexed references or incomplete BN coverage")
    if status() != before or reference.read_bytes() != image:
        raise ProgressError("retail or BN changed during tail verification")
    return dict(body_size=body_end-start, tail_size=end-body_end,
                retail_tail_hex=image[first+body_end-start:last+1].hex(),
                bn_identity=list(before), indexed_tail_references=0)


def main(argv=None):
    configure_stdio()
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--payload-file", type=Path, required=True)
    parser.add_argument("--progress", type=Path, default=DEFAULT_PROGRESS_PATH)
    parser.add_argument("--expected-revision", type=int, required=True)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--dry-run", action="store_true")
    mode.add_argument("--apply", action="store_true")
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args(argv)
    try:
        path = args.payload_file.resolve()
        if not path.is_relative_to((REPO_ROOT / "build").resolve()):
            raise ProgressError("reviewed payload must be under workspace build/")
        payload = json.loads(path.read_text(encoding="utf-8-sig"))
        store = ProgressStore(args.progress)
        document = store.load()
        if document.revision != args.expected_revision:
            raise ProgressError(f"tracker revision changed: expected {args.expected_revision}, found {document.revision}")
        proposed, interval = plan_separation(document, payload)
        spec = reference_image("recoil")
        proof = prove_live(REPO_ROOT / spec.reference_path, interval, BinaryNinjaBridge(binary=spec.bndb_path))
        commit = store.commit(proposed, expected_revision=args.expected_revision, apply=args.apply)
        print(json.dumps(dict(kind="function-tail-padding-separation", symbol_id=payload["symbol_id"],
            before=payload["current_symbol"], after=proposed["symbols"][payload["symbol_id"]],
            proof=proof, retained_call_contract="no instruction or call site removed",
            accepted_bytes=False, accepted_padding=False, commit=commit.to_dict()), indent=2))
    except (OSError, ValueError, KeyError, TypeError, ProgressError, BridgeError) as exc:
        print(f"function tail separation error: {exc}", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
