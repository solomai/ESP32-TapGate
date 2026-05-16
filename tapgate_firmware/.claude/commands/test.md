---
description: Run host-side unit tests (Unity). Shows failing test name, output, and suggests a fix.
---

Run the host-side unit test suite. No hardware required.

```bash
python run_host_tests.py 2>&1
```

The script cleans `build_tests_host/`, configures CMake against `tests_host/`, builds, and runs `ctest`.

To run manually step by step:
```bash
cmake -B build_tests_host -DCMAKE_BUILD_TYPE=Debug tests_host/ 2>&1
cmake --build build_tests_host 2>&1
ctest -C Debug --verbose --test-dir build_tests_host 2>&1
```

If tests fail: show the failing test name, full stdout, and suggest a concrete fix.

To run a specific test:
```bash
ctest --test-dir build_tests_host -R MyTest --verbose
```

For IDF on-target Unity tests (requires connected device):
```bash
python run_device_tests.py
python run_device_tests.py test_comp_crc32
python run_device_tests.py "test_comp*"
```
