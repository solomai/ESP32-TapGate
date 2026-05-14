/**
 * @file wait_dbg.h
 * @brief Startup debugger attach helper for ESP32/ESP-IDF projects.
 *
 * Provides wait_debugger_connection() — a startup barrier that holds execution
 * until a GDB session is attached via JTAG/OpenOCD, then breaks at a known
 * symbol so the developer can begin debugging from a clean state.
 *
 * Flow:
 *   1. If OpenOCD is not detected (esp_cpu_dbgr_is_attached() == false),
 *      logs a warning and returns immediately — no impact on normal boot.
 *   2. If OpenOCD is present, waits up to CONFIG_APP_WAIT_FOR_DEBUGGER_TIMEOUT
 *      seconds for GDB to set `dbg_ready = 1` via launch.json setupCommands.
 *   3. Once GDB signals ready, execution stops at dbg_resume_point() —
 *      a dedicated noinline symbol that GDB breaks on via `tbreak dbg_resume_point`.
 *   4. Press F5 in VSCode to resume normal execution.
 *   5. If GDB does not connect within the timeout, boot continues unimpeded.
 *
 * Requirements:
 *   - OpenOCD server must be running before firmware starts.
 *   - launch.json must include in setupCommands:
 *       { "text": "tbreak dbg_resume_point" },
 *       { "text": "set var dbg_ready = 1"   }
 *   - Kconfig: enable CONFIG_APP_WAIT_FOR_DEBUGGER and set timeout.
 *
 * Usage:
 *   #include "diag/wait_dbg.h"
 *   ...
 *   wait_debugger_connection();  // place at the top of app_main()
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/**
* @brief Wait for GDB session and break at a known symbol.
*
* Blocks execution until GDB sets @c dbg_ready or timeout expires.
* No-op if OpenOCD is not detected.
*/
void wait_debugger_connection(void);

/**
* @brief GDB breakpoint landing site.
*
* Do not call directly. GDB breaks here via @c tbreak dbg_resume_point
* issued from launch.json setupCommands.
*/
void dbg_resume_point(void);

#ifdef __cplusplus
}
#endif // __cplusplus