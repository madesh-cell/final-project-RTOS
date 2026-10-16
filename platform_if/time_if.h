/**
 * @file    time_if.h
 * @author  Madesh
 * @brief   Platform interface - time function pointer contracts (PIF_ layer).
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - This file is FROZEN after v0.0.1-alpha. Changes require Senior sign-off.
 * - Include path resolved by build system: -I platform_if/ (app repo)
 * -   or -I tests/stub_platform_if/ (CI host build).
 * - Contains typedefs only. No implementation, no extern, no logic macros.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef TIME_IF_H
#define TIME_IF_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Read the free-running millisecond counter.
 * @return Milliseconds since start-up. Wraps after about 49 days.
 */
typedef uint32_t (*PIF_TIME_GetMsFn)(void);

/**
 * @brief Busy-wait for a short time.
 * @param us Requested delay in microseconds.
 */
typedef void (*PIF_TIME_DelayUsFn)(uint32_t us);

#endif /* TIME_IF_H */
