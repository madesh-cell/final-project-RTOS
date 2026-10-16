/**
 * @file    os_if.h
 * @author  Madesh
 * @brief   Platform interface - RTOS lock function pointer contracts (PIF_ layer).
 * @version 0.1
 * @date    2026-10-14
 *
 * @note
 * - Contains typedefs only. No implementation, no extern, no logic macros.
 * - The application never includes an RTOS header; it only calls these pointers.
 *
 * @revision_history
 * - 0.1 2026-10-14 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef OS_IF_H
#define OS_IF_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Take the lock that protects the shared pad data.
 * @return true if the lock was taken, false if it timed out. Before the scheduler starts
 *         there is nothing to protect against, so it returns true.
 */
typedef bool (*PIF_OS_LockFn)(void);

/**
 * @brief Release the pad data lock, or end/begin a scheduler lock. Takes no arguments.
 */
typedef void (*PIF_OS_VoidFn)(void);

#endif /* OS_IF_H */
