/**
 * @file    mw_os.c
 * @author  Madesh
 * @brief   MW_OS - FreeRTOS (CMSIS-RTOS v2) mutex and scheduler lock behind PIF_ pointers.
 * @version 0.1
 * @date    2026-10-14
 *
 * @note
 * - Before MW_OS_Init() or before the kernel runs, lock() returns true without locking:
 *   only one thread exists then, so there is nothing to protect.
 * - No MISRA C deviations in this file.
 *
 * @revision_history
 * - 0.1 2026-10-14 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "mw_os.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   /* NULL */
#include "cmsis_os2.h"

/* ── Private Macros ──────────────────────────────────────────────────────── */
#define MW_OS_LOCK_TIMEOUT_MS    (100U)

/* ── Private Variables ───────────────────────────────────────────────────── */
static osMutexId_t MW_OS_mutex = NULL;

/* ── Private Function Prototypes ─────────────────────────────────────────── */
static bool MW_OS_Lock(void);
static void MW_OS_Unlock(void);
static void MW_OS_SchedLock(void);
static void MW_OS_SchedUnlock(void);

/* ── Public Variables ────────────────────────────────────────────────────── */
const MW_OS_If_t MW_OS_If =
{
    .lock         = MW_OS_Lock,
    .unlock       = MW_OS_Unlock,
    .sched_lock   = MW_OS_SchedLock,
    .sched_unlock = MW_OS_SchedUnlock
};

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool MW_OS_Init(void)
{
    static const osMutexAttr_t mutex_attr =
    {
        .name      = "padMutex",
        .attr_bits = osMutexPrioInherit
    };

    MW_OS_mutex = osMutexNew(&mutex_attr);

    return (MW_OS_mutex != NULL);
}

/* ── Private Function Definitions ────────────────────────────────────────── */

/**
 * @brief  Take the shared-data mutex.
 * @return true if taken (or if no mutex exists yet), false on timeout.
 */
static bool MW_OS_Lock(void)
{
    bool taken = true;

    if ((MW_OS_mutex != NULL) && (osKernelGetState() == osKernelRunning))
    {
        taken = (osMutexAcquire(MW_OS_mutex, MW_OS_LOCK_TIMEOUT_MS) == osOK);
    }

    return taken;
}


/**
 * @brief Release the shared-data mutex.
 */
static void MW_OS_Unlock(void)
{
    if ((MW_OS_mutex != NULL) && (osKernelGetState() == osKernelRunning))
    {
        (void)osMutexRelease(MW_OS_mutex);
    }
}


/**
 * @brief Stop task switching. Interrupts still run, so the tick keeps counting.
 */
static void MW_OS_SchedLock(void)
{
    (void)osKernelLock();
}


/**
 * @brief Allow task switching again.
 */
static void MW_OS_SchedUnlock(void)
{
    (void)osKernelUnlock();
}
