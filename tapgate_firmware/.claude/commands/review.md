---
description: Run a full code review on staged or specified files using the cpp-reviewer agent. Pass a filename or leave empty for staged files.
---

Review C++ code for correctness, safety, style, and performance.

```bash
# If argument provided: review that file. Otherwise review staged files.
FILES="${1:-$(git diff --cached --name-only | grep -E '\.(cpp|hpp|h|cxx|cc)$')}"
echo "Reviewing: $FILES"
```

Invoke the `cpp-reviewer` subagent on each file. Output format:
```
## [CRITICAL] / [WARNING] / [SUGGESTION]
File: <file>:<line>
Issue: <description>
Fix: <concrete code fix>
```
