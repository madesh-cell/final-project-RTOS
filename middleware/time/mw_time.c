/**
 * @file    mw_time.c
 * @author  Madesh
 * @brief   Time middleware - millisecond tick and short busy-wait delay.
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - MW_TIME_DelayUs is an uncalibrated busy-wait (Cortex-M0+ has no cycle counter).
 * -   It is accurate only to a factor of about 2, which the HX710B tolerates.
 * - No MISRA C deviations in this file.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "mw_time.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* ── Private Macros ──────────────────────────────────────────────────────── */
#define MW_TIME_LOOPS_PER_US    (2U)

/* ── Private Function Prototypes ─────────────────────────────────────────── */
static uint32_t MW_TIME_GetMs(void);
static void MW_TIME_DelayUs(uint32_t us);

/* ── Public Variables ────────────────────────────────────────────────────── */
const MW_TIME_If_t MW_TIME_If =
{
    .get_ms   = MW_TIME_GetMs,
    .delay_us = MW_TIME_DelayUs
};

/* ── Private Function Definitions ────────────────────────────────────────── */

/**
 * @brief  Read the HAL millisecond tick.
 * @return Milliseconds since start-up.
 */
static uint32_t MW_TIME_GetMs(void)
{
    return HAL_GetTick();
}


/**
 * @brief Busy-wait for roughly the requested number of microseconds.
 * @param us Requested delay in microseconds.
 */
static void MW_TIME_DelayUs(uint32_t us)
{
    volatile uint32_t loop_count = us * MW_TIME_LOOPS_PER_US;

    while (loop_count > 0U)
    {
        loop_count--;
    }
}
