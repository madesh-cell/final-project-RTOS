/**
 * @file    mw_os.h
 * @author  Madesh
 * @brief   Public interface for the MW_OS module (FreeRTOS locks behind PIF_ pointers).
 * @version 0.1
 * @date    2026-10-14
 *
 * @note
 * - Wraps the CMSIS-RTOS v2 mutex and scheduler lock. Only this module includes cmsis_os2.h.
 * - Call MW_OS_Init() once after osKernelInitialize(), before osKernelStart().
 *
 * @revision_history
 * - 0.1 2026-10-14 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef MW_OS_H
#define MW_OS_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "os_if.h"

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    PIF_OS_LockFn lock;          /**< Take the shared-data mutex (with a time limit).  */
    PIF_OS_VoidFn unlock;        /**< Release the shared-data mutex.                   */
    PIF_OS_VoidFn sched_lock;    /**< Stop task switching (for the sensor bit-banging). */
    PIF_OS_VoidFn sched_unlock;  /**< Allow task switching again.                      */
} MW_OS_If_t;

/* ── Public Variables ────────────────────────────────────────────────────── */
extern const MW_OS_If_t MW_OS_If;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    MW_OS_Init
 * @brief   Create the shared-data mutex (with priority inheritance).
 * @return  true on success, false if the mutex could not be created.
 */
bool MW_OS_Init(void);

#endif /* MW_OS_H */
