import importlib.util
import sys
import unittest
from pathlib import Path


def load_tests(loader, tests, pattern):
    suite = unittest.TestSuite()
    tests_dir = Path(__file__).resolve().parent
    for path in sorted(tests_dir.glob("recoil_*_tests.py")):
        module_name = f"_recoil_tool_discovery_{path.stem}"
        spec = importlib.util.spec_from_file_location(module_name, path)
        if spec is None or spec.loader is None:
            raise ImportError(f"cannot load test module: {path}")
        module = importlib.util.module_from_spec(spec)
        sys.modules[module_name] = module
        spec.loader.exec_module(module)
        suite.addTests(loader.loadTestsFromModule(module))
    return suite
