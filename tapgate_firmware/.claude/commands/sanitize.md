---
description: Build and run host tests with AddressSanitizer + UBSan. Reports memory errors or undefined behavior.
---

Build and run the host-side unit test suite with AddressSanitizer and UBSan enabled.

**Note:** ASan/UBSan cannot run on ESP32 target hardware. Sanitizers apply to host-side unit tests only. For on-target memory issues, use JTAG + OpenOCD or the IDF heap tracing feature.

```bash
cmake -B test/build-asan -G Ninja \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
      -DCMAKE_BUILD_TYPE=Debug test/ 2>&1
cmake --build test/build-asan -j$(nproc) 2>&1
ASAN_OPTIONS=detect_leaks=1 ctest --test-dir test/build-asan --output-on-failure -j$(nproc) 2>&1
```

Report all ASan/UBSan findings with:
- File path and line number
- Type of error (heap-use-after-free, stack-buffer-overflow, signed-integer-overflow, etc.)
- Call stack
- Suggested fix

For on-target heap diagnostics (requires device):
```bash
# Enable in sdkconfig.defaults:
# CONFIG_HEAP_TRACING=y
# CONFIG_HEAP_TRACING_STANDALONE=y
# Then in code: heap_trace_start(HEAP_TRACE_LEAKS); ... heap_trace_stop(); heap_trace_dump();
```
