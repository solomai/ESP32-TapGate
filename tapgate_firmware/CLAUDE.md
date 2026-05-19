# TapGate Firmware

## Quick Reference
- **Role & tooling map**: `.claude/CLAUDE.md` — persona, activity → rules/skills/scripts/commands
- **Rules**: `.claude/rules/` — build, code-style, esp-idf, memory, testing, embedded-cpp
- **Docs**: `documents/` — developer documentation
- **Overview**: `README.md` — project setup and features

## Commit Gate
Every commit must pass: **host tests** + **clang-tidy**. Hardware tests required for releases only.

Commit style: `feat / fix / chore / refactor / test / docs`

## Scripts (Windows PowerShell)
| Script | Purpose |
|---|---|
| `.scripts/idf_build.ps1` | Build / flash / monitor / size via idf.py |
| `.scripts/run_host_tests.ps1` | Run host-side Unity tests (no hardware) |
| `.scripts/run_device_tests.ps1` | Run on-target Unity tests |
| `.scripts/_idf_env.ps1` | ESP-IDF v6 environment setup (sourced internally) |

## Skills (invoke with `/skill-name`)
| Skill | When to use |
|---|---|
| `/build` | Build firmware, report warnings + binary size |
| `/test` | Run host Unity tests, show failures with fix hints |
| `/sanitize` | ASan + UBSan host build |
| `/analyze` | Clang-tidy on changed files |
| `/review` | Code review on staged or specified files |
| `/security-review` | Security-focused review of branch changes |
| `/simplify` | Cleanup and dedup after changes |
