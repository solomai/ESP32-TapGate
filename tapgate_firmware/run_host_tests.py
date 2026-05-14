import os
import shutil
import subprocess
import sys

# Color codes
COLOR_RED    = "\033[91m"
COLOR_GREEN  = "\033[92m"
COLOR_YELLOW = "\033[93m"
COLOR_RESET  = "\033[0m"

BUILD_DIR = "build_tests_host"
SRC_DIR = "tests_host"

# Step 1: Create build directory if not exists
if os.path.exists(BUILD_DIR):
    print(f"Removing old build directory: {BUILD_DIR}")
    shutil.rmtree(BUILD_DIR)
os.makedirs(BUILD_DIR, exist_ok=True)

# Step 2: Run CMake configure

print(f"{COLOR_YELLOW}Configuring CMake:{COLOR_RESET}")
res = subprocess.run(["cmake", f"../{SRC_DIR}"], cwd=BUILD_DIR)
if res.returncode != 0:
    print(f"{COLOR_RED}CMake configure failed{COLOR_RESET}")
    sys.exit(res.returncode)

# Step 3: Build

print(f"{COLOR_YELLOW}Building:{COLOR_RESET}")
res = subprocess.run(["cmake", "--build", "."], cwd=BUILD_DIR)
if res.returncode != 0:
    print(f"{COLOR_RED}Build failed{COLOR_RESET}")
    sys.exit(res.returncode)

# Step 4: Run tests

print(f"{COLOR_YELLOW}Running tests:{COLOR_RESET}")
# Use "--output-on-failure" instead "--verbose" to limit output to failed tests only
# Use "--stop-on-failure" to stop at first failure
res = subprocess.run(["ctest", "-C", "Debug", "--verbose"], cwd=BUILD_DIR, capture_output=True, text=True)
print(res.stdout)

# Step 5: Parse and summarize results
if "100% tests passed" in res.stdout:
    print(f"{COLOR_GREEN}All tests passed{COLOR_RESET}")
    sys.exit(0)
else:
    print(f"{COLOR_RED}Some tests failed{COLOR_RESET}")
    sys.exit(1)
