#!/usr/bin/env python3
"""
ESP32 device test runner.
Reads serial port from .vscode/settings.json automatically.
Builds, flashes and runs each test group sequentially,
collects and prints a summary.

Usage:
    python run_device_tests.py                  # run all tests
    python run_device_tests.py test_diagnostics # run one group
    python run_device_tests.py test_comp*       # run by mask
"""

import fnmatch
import json
import os
import re
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional

import serial

# ── Config ────────────────────────────────────────────────────────────────────

REPO_ROOT       = Path(__file__).parent.resolve()
TESTS_DIR       = REPO_ROOT / "tests"
VSCODE_SETTINGS = REPO_ROOT / ".vscode" / "settings.json"

BAUD_RATE      = 115200
TEST_TIMEOUT_S = 60

# ── Colors ────────────────────────────────────────────────────────────────────

COLOR_RED    = "\033[91m"
COLOR_GREEN  = "\033[92m"
COLOR_YELLOW = "\033[93m"
COLOR_CYAN   = "\033[96m"
COLOR_RESET  = "\033[0m"

# ── Data ──────────────────────────────────────────────────────────────────────

@dataclass
class TestResult:
    name:    str
    passed:  int = 0
    failed:  int = 0
    ignored: int = 0
    errors:  List[str] = field(default_factory=list)


# ── Helpers ───────────────────────────────────────────────────────────────────

def get_idf_python() -> str:
    idf_python = os.environ.get("IDF_PYTHON_ENV_PATH")
    if idf_python:
        candidate = Path(idf_python) / "Scripts" / "python.exe"
        if candidate.exists():
            return str(candidate)
    return sys.executable


def get_idf_py() -> Path:
    idf_path = os.environ.get("IDF_PATH")
    if not idf_path:
        sys.exit(f"{COLOR_RED}[ERROR] IDF_PATH not set. Open ESP-IDF terminal before running.{COLOR_RESET}")

    idf_py = Path(idf_path) / "tools" / "idf.py"
    if not idf_py.exists():
        sys.exit(f"{COLOR_RED}[ERROR] idf.py not found at: {idf_py}{COLOR_RESET}")

    return idf_py


def get_idf_env() -> dict:
    env = os.environ.copy()
    if "IDF_PATH" not in env:
        sys.exit(f"{COLOR_RED}[ERROR] IDF_PATH not set. Open ESP-IDF terminal before running.{COLOR_RESET}")
    return env


def get_serial_port() -> str:
    if not VSCODE_SETTINGS.exists():
        sys.exit(f"{COLOR_RED}[ERROR] {VSCODE_SETTINGS} not found.\n"
                 f"        Add idf.portWin (Windows) or idf.port (Linux/Mac) there.{COLOR_RESET}")

    with open(VSCODE_SETTINGS, encoding="utf-8") as f:
        raw = f.read()

    raw = re.sub(r'//[^\n]*', '', raw)
    raw = re.sub(r',\s*([}\]])', r'\1', raw)

    try:
        settings = json.loads(raw)
    except json.JSONDecodeError as e:
        sys.exit(f"{COLOR_RED}[ERROR] Failed to parse {VSCODE_SETTINGS}: {e}{COLOR_RESET}")

    port_key = "idf.portWin" if sys.platform == "win32" else "idf.port"
    port = settings.get(port_key) or settings.get("idf.port")

    if not port:
        sys.exit(f"{COLOR_RED}[ERROR] Port key '{port_key}' not found in {VSCODE_SETTINGS}{COLOR_RESET}")

    print(f"[INFO] Using serial port: {port}")
    return port


def run_cmd(cmd: List[str], cwd: Path, env: dict) -> int:
    print(f"{COLOR_YELLOW}\n>>> {' '.join(str(c) for c in cmd)}{COLOR_RESET}")
    return subprocess.run(cmd, cwd=str(cwd), env=env, shell=False).returncode


def discover_test_groups() -> List[Path]:
    if not TESTS_DIR.exists():
        sys.exit(f"{COLOR_RED}[ERROR] Tests directory not found: {TESTS_DIR}{COLOR_RESET}")

    groups = sorted([
        d for d in TESTS_DIR.iterdir()
        if d.is_dir() and (d / "CMakeLists.txt").exists()
    ])

    if not groups:
        sys.exit(f"{COLOR_RED}[ERROR] No test groups found in {TESTS_DIR}{COLOR_RESET}")

    return groups


def filter_test_groups(groups: List[Path], pattern: Optional[str]) -> List[Path]:
    """Filter groups by name using fnmatch pattern. None = return all."""
    if pattern is None:
        return groups

    matched = [g for g in groups if fnmatch.fnmatch(g.name, pattern)]

    if not matched:
        available = ", ".join(g.name for g in groups)
        sys.exit(
            f"{COLOR_RED}[ERROR] No test groups match pattern '{pattern}'.\n"
            f"        Available groups: {available}{COLOR_RESET}"
        )

    return matched


def build_and_flash(test_dir: Path, port: str, idf_py: Path,
                    python: str, env: dict) -> bool:
    print(f"\n{'='*60}")
    print(f"{COLOR_CYAN}[BUILD] {test_dir.name}{COLOR_RESET}")
    print(f"{'='*60}")

    build_dir = REPO_ROOT / "build_tests" / test_dir.name

    if run_cmd([python, str(idf_py), "-B", str(build_dir), "build"], test_dir, env) != 0:
        print(f"{COLOR_RED}[FAIL] Build failed: {test_dir.name}{COLOR_RESET}")
        return False

    print(f"\n{COLOR_CYAN}[FLASH] {test_dir.name}{COLOR_RESET}")
    if run_cmd([python, str(idf_py), "-B", str(build_dir), "-p", port, "flash"], test_dir, env) != 0:
        print(f"{COLOR_RED}[FAIL] Flash failed: {test_dir.name}{COLOR_RESET}")
        return False

    return True


def collect_results(port: str, group_name: str) -> TestResult:
    result = TestResult(name=group_name)

    print(f"\n{COLOR_CYAN}[MONITOR] Waiting for test output on {port} ...{COLOR_RESET}")

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
    except serial.SerialException as e:
        result.errors.append(f"Serial open error: {e}")
        return result

    ser.setRTS(True)
    time.sleep(0.1)
    ser.setRTS(False)
    time.sleep(0.1)
    ser.reset_input_buffer()

    deadline   = time.time() + TEST_TIMEOUT_S
    summary_re = re.compile(r"(\d+) Tests (\d+) Failures (\d+) Ignored")
    pass_re    = re.compile(r":PASS$")
    fail_re    = re.compile(r"FAIL:")

    while time.time() < deadline:
        try:
            raw = ser.readline()
        except serial.SerialException as e:
            result.errors.append(str(e))
            break

        if not raw:
            continue

        line = raw.decode("utf-8", errors="replace").rstrip()

        if fail_re.search(line):
            print(f"  {COLOR_RED}{line}{COLOR_RESET}")
            result.errors.append(line)
        elif pass_re.search(line):
            print(f"  {COLOR_GREEN}{line}{COLOR_RESET}")
        else:
            print(f"  {line}")

        m = summary_re.search(line)
        if m:
            total          = int(m.group(1))
            failed         = int(m.group(2))
            result.ignored = int(m.group(3))
            result.failed  = failed
            result.passed  = total - failed - result.ignored
            break
    else:
        result.errors.append(f"Timeout after {TEST_TIMEOUT_S}s — no Unity summary received")

    ser.close()
    return result


def print_summary(results: List[TestResult]) -> None:
    print(f"\n{'='*60}")
    print("TEST SUMMARY")
    print(f"{'='*60}")

    total_passed  = 0
    total_failed  = 0
    total_ignored = 0

    for r in results:
        if r.failed == 0 and not r.errors:
            status_str = f"{COLOR_GREEN}PASS{COLOR_RESET}"
        else:
            status_str = f"{COLOR_RED}FAIL{COLOR_RESET}"

        print(f"  [{status_str}] {r.name:<30} "
              f"passed={r.passed}  failed={r.failed}  ignored={r.ignored}")
        for e in r.errors:
            print(f"  {COLOR_RED}     !! {e}{COLOR_RESET}")

        total_passed  += r.passed
        total_failed  += r.failed
        total_ignored += r.ignored

    print(f"{'─'*60}")
    print(f"  TOTAL  passed={total_passed}  failed={total_failed}  ignored={total_ignored}")
    print(f"{'='*60}\n")

    if total_failed > 0 or any(r.errors for r in results):
        print(f"{COLOR_RED}Some tests failed{COLOR_RESET}")
        sys.exit(1)
    else:
        print(f"{COLOR_GREEN}All tests passed{COLOR_RESET}")


# ── Main ──────────────────────────────────────────────────────────────────────

def main() -> None:
    # Optional positional argument: filter pattern (supports * mask).
    # PowerShell and cmd.exe do NOT expand globs, so sys.argv[1] always
    # arrives as the literal string the user typed (e.g. "test_sys*").
    pattern = sys.argv[1] if len(sys.argv) > 1 else None

    idf_py = get_idf_py()
    python  = get_idf_python()
    env     = get_idf_env()
    port    = get_serial_port()
    groups  = discover_test_groups()
    groups  = filter_test_groups(groups, pattern)

    print(f"[INFO] IDF_PATH : {env['IDF_PATH']}")
    print(f"[INFO] Python   : {python}")
    print(f"[INFO] idf.py   : {idf_py}")
    if pattern:
        print(f"[INFO] Filter   : {pattern}")
    print(f"[INFO] Running {len(groups)} test group(s): "
          f"{', '.join(g.name for g in groups)}")

    results: List[TestResult] = []

    for group in groups:
        if not build_and_flash(group, port, idf_py, python, env):
            results.append(TestResult(
                name=group.name,
                errors=["Build or flash failed"]
            ))
            continue

        result = collect_results(port, group.name)
        results.append(result)

    print_summary(results)


if __name__ == "__main__":
    main()