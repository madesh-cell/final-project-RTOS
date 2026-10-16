/**
 * @file    mw_time.h
 * @author  Madesh
 * @brief   Public interface for the time middleware (STM32 HAL wrapper).
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - Implements the PIF_TIME_ function pointer types.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef MW_TIME_H
#define MW_TIME_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "time_if.h"

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    PIF_TIME_GetMsFn   get_ms;
    PIF_TIME_DelayUsFn delay_us;
} MW_TIME_If_t;

/* ── Public Variables ────────────────────────────────────────────────────── */
extern const MW_TIME_If_t MW_TIME_If;

#endif /* MW_TIME_H */
