from __future__ import annotations

import argparse
from dataclasses import dataclass
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Any


REPO_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = REPO_ROOT / "tools" / "vc6_final_build.json"
DEFAULT_VC6_ROOT = REPO_ROOT.parent / "Compiler" / "VC6"


@dataclass(frozen=True)
class FinalBuildConfig:
    name: str
    description: str
    vc6_env: Path
    build_dir: Path
    output_exe: str
    output_map: str
    resource_script: Path
    resource_output: str
    reference_exe: Path
    reference_manifest: Path
    defines: tuple[str, ...]
    include_dirs: tuple[Path, ...]
    lib_dirs: tuple[Path, ...]
    compile_flags: tuple[str, ...]
    resource_flags: tuple[str, ...]
    link_flags: tuple[str, ...]
    libs: tuple[str, ...]
    sources: tuple[Path, ...]
    manifest_path: Path


@dataclass(frozen=True)
class BuildPaths:
    build_dir: Path
    obj_dir: Path
    logs_dir: Path
    rsp_dir: Path
    exe_path: Path
    map_path: Path
    resource_path: Path
    summary_path: Path


@dataclass(frozen=True)
class CommandSpec:
    name: str
    command: str
    cwd: Path
    stdout_log: Path
    stderr_log: Path


@dataclass(frozen=True)
class CommandResult:
    name: str
    returncode: int
    stdout_log: Path
    stderr_log: Path


def require_string(data: dict[str, Any], key: str, *, manifest_path: Path) -> str:
    value = data.get(key)
    if not isinstance(value, str) or not value:
        raise ValueError(f"{manifest_path}: expected non-empty string field '{key}'")
    return value


def require_string_list(data: dict[str, Any], key: str, *, manifest_path: Path) -> tuple[str, ...]:
    value = data.get(key, [])
    if not isinstance(value, list) or any(not isinstance(item, str) or not item for item in value):
        raise ValueError(f"{manifest_path}: expected non-empty string list field '{key}'")
    return tuple(value)


def expand_config_path(path_text: str | Path) -> str:
    text = str(path_text)
    vc6_root = os.environ.get("RECOIL_VC6_ROOT", str(DEFAULT_VC6_ROOT))
    text = text.replace("${RECOIL_VC6_ROOT}", vc6_root)
    text = text.replace("%RECOIL_VC6_ROOT%", vc6_root)
    return os.path.expandvars(text)


def repo_path(path_text: str | Path) -> Path:
    path = Path(expand_config_path(path_text))
    if path.is_absolute():
        return path
    return REPO_ROOT / path


def load_config(path: str | Path = DEFAULT_MANIFEST) -> FinalBuildConfig:
    manifest_path = Path(path)
    with manifest_path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError(f"{manifest_path}: manifest root must be an object")

    return FinalBuildConfig(
        name=require_string(data, "name", manifest_path=manifest_path),
        description=require_string(data, "description", manifest_path=manifest_path),
        vc6_env=repo_path(require_string(data, "vc6_env", manifest_path=manifest_path)),
        build_dir=repo_path(require_string(data, "build_dir", manifest_path=manifest_path)),
        output_exe=require_string(data, "output_exe", manifest_path=manifest_path),
        output_map=require_string(data, "output_map", manifest_path=manifest_path),
        resource_script=repo_path(require_string(data, "resource_script", manifest_path=manifest_path)),
        resource_output=require_string(data, "resource_output", manifest_path=manifest_path),
        reference_exe=repo_path(require_string(data, "reference_exe", manifest_path=manifest_path)),
        reference_manifest=repo_path(require_string(data, "reference_manifest", manifest_path=manifest_path)),
        defines=require_string_list(data, "defines", manifest_path=manifest_path),
        include_dirs=tuple(repo_path(item) for item in require_string_list(data, "include_dirs", manifest_path=manifest_path)),
        lib_dirs=tuple(repo_path(item) for item in require_string_list(data, "lib_dirs", manifest_path=manifest_path)),
        compile_flags=require_string_list(data, "compile_flags", manifest_path=manifest_path),
        resource_flags=require_string_list(data, "resource_flags", manifest_path=manifest_path),
        link_flags=require_string_list(data, "link_flags", manifest_path=manifest_path),
        libs=require_string_list(data, "libs", manifest_path=manifest_path),
        sources=tuple(repo_path(item) for item in require_string_list(data, "sources", manifest_path=manifest_path)),
        manifest_path=manifest_path,
    )


def quote_cmd_arg(value: str | Path) -> str:
    text = str(value)
    if '"' in text:
        raise ValueError(f"Command argument contains an unsupported quote: {text}")
    return f'"{text}"'


def response_line(value: str | Path) -> str:
    return quote_cmd_arg(value) if any(char.isspace() for char in str(value)) else str(value)


def safe_object_stem(source: Path) -> Path:
    try:
        rel = source.relative_to(REPO_ROOT)
    except ValueError:
        rel = Path(source.name)
    return rel.with_suffix(".obj")


def object_path(config: FinalBuildConfig, paths: BuildPaths, source: Path) -> Path:
    return paths.obj_dir / safe_object_stem(source)


def display_path(path: Path) -> str:
    try:
        return str(path.relative_to(REPO_ROOT)).replace("\\", "/")
    except ValueError:
        return str(path).replace("\\", "/")


def build_paths(config: FinalBuildConfig) -> BuildPaths:
    build_dir = config.build_dir
    return BuildPaths(
        build_dir=build_dir,
        obj_dir=build_dir / "obj",
        logs_dir=build_dir / "logs",
        rsp_dir=build_dir / "rsp",
        exe_path=build_dir / config.output_exe,
        map_path=build_dir / config.output_map,
        resource_path=build_dir / config.resource_output,
        summary_path=build_dir / "summary.json",
    )


def ensure_inputs_exist(config: FinalBuildConfig) -> list[str]:
    missing: list[str] = []
    for path in (config.vc6_env, config.resource_script, config.reference_exe, config.reference_manifest):
        if not path.exists():
            missing.append(str(path))
    for path in config.include_dirs:
        if not path.exists():
            missing.append(str(path))
    for path in config.lib_dirs:
        if not path.exists():
            missing.append(str(path))
    for path in config.sources:
        if not path.exists():
            missing.append(str(path))
    for lib in config.libs:
        if not any((lib_dir / lib).exists() for lib_dir in config.lib_dirs):
            missing.append(f"{lib} in {', '.join(str(path) for path in config.lib_dirs)}")
    return missing


def write_response(path: Path, args: list[str]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("\n".join(args) + "\n", encoding="ascii")


def compile_response_args(config: FinalBuildConfig, source: Path, obj_path: Path) -> list[str]:
    args = list(config.compile_flags)
    args.extend(f"/D{define}" for define in config.defines)
    args.extend(f"/I{path}" for path in config.include_dirs)
    args.append(f"/Fo{obj_path}")
    args.append("/c")
    args.append(str(source))
    return [response_line(arg) for arg in args]


def resource_response_args(config: FinalBuildConfig, paths: BuildPaths) -> list[str]:
    args = list(config.resource_flags)
    args.extend(f"/d{define}" for define in config.defines)
    args.extend(f"/i{path}" for path in config.include_dirs)
    args.append(f"/fo{paths.resource_path}")
    args.append(str(config.resource_script))
    return [response_line(arg) for arg in args]


def link_response_args(config: FinalBuildConfig, paths: BuildPaths, objects: list[Path]) -> list[str]:
    args = list(config.link_flags)
    args.append(f"/OUT:{paths.exe_path}")
    args.append(f"/MAP:{paths.map_path}")
    args.extend(f"/LIBPATH:{path}" for path in config.lib_dirs)
    args.extend(str(path) for path in objects)
    args.append(str(paths.resource_path))
    args.extend(config.libs)
    return [response_line(arg) for arg in args]


def make_compile_command(config: FinalBuildConfig, paths: BuildPaths, source: Path) -> tuple[CommandSpec, Path]:
    obj = object_path(config, paths, source)
    obj.parent.mkdir(parents=True, exist_ok=True)
    rsp = paths.rsp_dir / "compile" / safe_object_stem(source).with_suffix(".rsp")
    write_response(rsp, compile_response_args(config, source, obj))
    name = "compile:" + display_path(source)
    stem = str(safe_object_stem(source)).replace("\\", "__").replace("/", "__").replace(":", "")
    return (
        CommandSpec(
            name=name,
            command=f"call {quote_cmd_arg(config.vc6_env)} && cl @{quote_cmd_arg(rsp)}",
            cwd=REPO_ROOT,
            stdout_log=paths.logs_dir / f"{stem}.out.log",
            stderr_log=paths.logs_dir / f"{stem}.err.log",
        ),
        obj,
    )


def make_resource_command(config: FinalBuildConfig, paths: BuildPaths) -> CommandSpec:
    rsp = paths.rsp_dir / "resource.rsp"
    args = resource_response_args(config, paths)
    write_response(rsp, args)
    return CommandSpec(
        name="resource",
        command=f"call {quote_cmd_arg(config.vc6_env)} && rc {' '.join(args)}",
        cwd=REPO_ROOT,
        stdout_log=paths.logs_dir / "resource.out.log",
        stderr_log=paths.logs_dir / "resource.err.log",
    )


def make_link_command(config: FinalBuildConfig, paths: BuildPaths, objects: list[Path]) -> CommandSpec:
    rsp = paths.rsp_dir / "link.rsp"
    write_response(rsp, link_response_args(config, paths, objects))
    return CommandSpec(
        name="link",
        command=f"call {quote_cmd_arg(config.vc6_env)} && link @{quote_cmd_arg(rsp)}",
        cwd=REPO_ROOT,
        stdout_log=paths.logs_dir / "link.out.log",
        stderr_log=paths.logs_dir / "link.err.log",
    )


def make_script_command(name: str, command: str, paths: BuildPaths) -> CommandSpec:
    log_stem = name.replace(":", "_")
    return CommandSpec(
        name=name,
        command=command,
        cwd=REPO_ROOT,
        stdout_log=paths.logs_dir / f"{log_stem}.out.log",
        stderr_log=paths.logs_dir / f"{log_stem}.err.log",
    )


def run_command(spec: CommandSpec) -> CommandResult:
    spec.stdout_log.parent.mkdir(parents=True, exist_ok=True)
    spec.stderr_log.parent.mkdir(parents=True, exist_ok=True)
    script_path = spec.stdout_log.parent / f"_{spec.name.replace(':', '_').replace('/', '_')}.cmd"
    script_path.write_text("@echo off\r\n" + spec.command + "\r\n", encoding="ascii")
    creation_flags = 0
    if sys.platform == "win32":
        creation_flags = subprocess.CREATE_NO_WINDOW  # type: ignore[attr-defined]
    comspec = os.environ.get("ComSpec", r"C:\Windows\System32\cmd.exe")
    with spec.stdout_log.open("w", encoding="utf-8", errors="replace") as stdout, spec.stderr_log.open(
        "w", encoding="utf-8", errors="replace"
    ) as stderr:
        completed = subprocess.run(
            [comspec, "/d", "/c", str(script_path)],
            cwd=str(spec.cwd),
            stdout=stdout,
            stderr=stderr,
            text=True,
            creationflags=creation_flags,
        )
    return CommandResult(
        name=spec.name,
        returncode=completed.returncode,
        stdout_log=spec.stdout_log,
        stderr_log=spec.stderr_log,
    )


def read_log_tail(path: Path, max_lines: int = 8) -> list[str]:
    if not path.exists():
        return []
    lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    return lines[-max_lines:]


def print_result(result: CommandResult) -> None:
    status = "OK" if result.returncode == 0 else f"FAIL({result.returncode})"
    print(f"{status}: {result.name}")
    if result.returncode != 0:
        for path in (result.stdout_log, result.stderr_log):
            tail = read_log_tail(path)
            if tail:
                print(f"  {path}:")
                for line in tail:
                    print(f"    {line}")


def write_summary(paths: BuildPaths, results: list[CommandResult], *, dry_run: bool) -> None:
    data = {
        "dry_run": dry_run,
        "results": [
            {
                "name": result.name,
                "returncode": result.returncode,
                "stdout_log": str(result.stdout_log.relative_to(REPO_ROOT)),
                "stderr_log": str(result.stderr_log.relative_to(REPO_ROOT)),
            }
            for result in results
        ],
    }
    paths.summary_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def print_dry_run(commands: list[CommandSpec]) -> None:
    for spec in commands:
        print(f"{spec.name}:")
        print(f"  cwd: {spec.cwd}")
        print(f"  cmd: {spec.command}")
        print(f"  stdout: {spec.stdout_log}")
        print(f"  stderr: {spec.stderr_log}")


def run_python_check(name: str, args: list[str], paths: BuildPaths) -> CommandResult:
    command = " ".join([quote_cmd_arg(sys.executable), *[quote_cmd_arg(arg) for arg in args]])
    return run_command(make_script_command(name, command, paths))


def run_build(
    config: FinalBuildConfig,
    *,
    clean: bool,
    dry_run: bool,
    compile_only: bool,
    keep_going: bool,
    no_pe_compare: bool,
    no_resource_compare: bool,
) -> int:
    paths = build_paths(config)
    if clean and paths.build_dir.exists() and not dry_run:
        shutil.rmtree(paths.build_dir)
    paths.obj_dir.mkdir(parents=True, exist_ok=True)
    paths.logs_dir.mkdir(parents=True, exist_ok=True)
    paths.rsp_dir.mkdir(parents=True, exist_ok=True)

    missing = ensure_inputs_exist(config)
    if missing:
        print("Missing build inputs:", file=sys.stderr)
        for item in missing:
            print(f"- {item}", file=sys.stderr)
        return 2

    compile_commands: list[CommandSpec] = []
    objects: list[Path] = []
    for source in config.sources:
        command, obj = make_compile_command(config, paths, source)
        compile_commands.append(command)
        objects.append(obj)
    resource_command = make_resource_command(config, paths)
    link_command = make_link_command(config, paths, objects)
    all_commands = compile_commands + [resource_command]
    if not compile_only:
        all_commands.append(link_command)

    if dry_run:
        print(f"VC6 final build manifest: {config.manifest_path}")
        print(f"Build directory: {paths.build_dir}")
        print_dry_run(all_commands)
        write_summary(paths, [], dry_run=True)
        return 0

    results: list[CommandResult] = []
    failed = False
    for command in compile_commands:
        result = run_command(command)
        results.append(result)
        print_result(result)
        if result.returncode != 0:
            failed = True
            if not keep_going:
                write_summary(paths, results, dry_run=False)
                return result.returncode

    resource_result = run_command(resource_command)
    results.append(resource_result)
    print_result(resource_result)
    if resource_result.returncode != 0:
        failed = True
        if not keep_going:
            write_summary(paths, results, dry_run=False)
            return resource_result.returncode

    if compile_only:
        write_summary(paths, results, dry_run=False)
        return 1 if failed else 0

    if failed:
        print("Skipping link because compile/resource failures remain.", file=sys.stderr)
        write_summary(paths, results, dry_run=False)
        return 1

    link_result = run_command(link_command)
    results.append(link_result)
    print_result(link_result)
    if link_result.returncode != 0:
        write_summary(paths, results, dry_run=False)
        return link_result.returncode

    if not no_resource_compare:
        resource_compare = run_python_check(
            "resource_compare",
            [
                "tools/recoil_resource_extract.py",
                "--compare-res",
                str(paths.resource_path),
            ],
            paths,
        )
        results.append(resource_compare)
        print_result(resource_compare)
        if resource_compare.returncode != 0:
            write_summary(paths, results, dry_run=False)
            return resource_compare.returncode

    if not no_pe_compare:
        pe_compare = run_python_check(
            "pe_compare",
            [
                "tools/recoil_pe_reference.py",
                "--reference",
                str(config.reference_exe),
                "--manifest",
                str(config.reference_manifest),
                "--candidate",
                str(paths.exe_path),
            ],
            paths,
        )
        results.append(pe_compare)
        print_result(pe_compare)
        if pe_compare.returncode != 0:
            write_summary(paths, results, dry_run=False)
            return pe_compare.returncode

    write_summary(paths, results, dry_run=False)
    print(f"Candidate executable: {paths.exe_path}")
    print(f"Map file: {paths.map_path}")
    return 0


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Build a VC6-linked candidate Recoil.exe outside the modern CMake smoke build."
    )
    parser.add_argument("--manifest", default=str(DEFAULT_MANIFEST))
    parser.add_argument("--dry-run", action="store_true", help="Generate response files and print commands only.")
    parser.add_argument("--compile-only", action="store_true", help="Compile objects and resources, then stop.")
    parser.add_argument("--keep-going", action="store_true", help="Keep compiling after source failures.")
    parser.add_argument("--clean", action="store_true", help="Remove build/vc6-final before building.")
    parser.add_argument("--no-pe-compare", action="store_true", help="Skip final PE comparison after link.")
    parser.add_argument("--no-resource-compare", action="store_true", help="Skip compiled resource comparison.")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        config = load_config(args.manifest)
        return run_build(
            config,
            clean=args.clean,
            dry_run=args.dry_run,
            compile_only=args.compile_only,
            keep_going=args.keep_going,
            no_pe_compare=args.no_pe_compare,
            no_resource_compare=args.no_resource_compare,
        )
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
