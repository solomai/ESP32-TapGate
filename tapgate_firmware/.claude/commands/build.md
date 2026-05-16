---
description: Build ESP-IDF firmware with idf.py. Reports build status, warnings, and binary size.
---

Build the ESP-IDF firmware. Runs `idf.py build` and reports results.

```bash
idf.py build 2>&1
```

After a successful build, report:
- Binary sizes: app.bin, bootloader.bin, partition-table.bin
- Any compiler warnings (treat as errors in CI)
- Build time

If the build fails:
- Show the first error with file path and line number
- Identify whether it is a CMake configure error, compile error, or link error
- Suggest a fix

For a full clean rebuild:
```bash
idf.py fullclean && idf.py build
```

To check firmware size breakdown:
```bash
idf.py size-components
```
