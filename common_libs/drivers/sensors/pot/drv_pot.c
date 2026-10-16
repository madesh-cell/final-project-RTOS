/**
 * @file    drv_pot.c
 * @author  Madesh
 * @brief   Potentiometer driver - wiper voltage to permille.
 * @version 0.2
 * @date    2026-10-03
 *
 * @note
 * - No MISRA C deviations in this file.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.2 2026-10-03 Madesh Two-cuff alternating cycle, valves, potentiometer.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "drv_pot.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   /* NULL */

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool DRV_POT_Init(DRV_POT_Handle_t *handle, const DRV_POT_Config_t *config)
{
    bool init_ok = false;

    if ((handle != NULL) && (config != NULL) &&
        (config->adc_read != NULL) && (config->full_scale_mv > 0U))
    {
        handle->config         = *config;
        handle->is_initialised = true;
        init_ok                = true;
    }

    return init_ok;
}


bool DRV_POT_ReadPermille(DRV_POT_Handle_t *handle, uint16_t *permille)
{
    bool     read_ok    = false;
    uint16_t millivolts = 0U;
    uint32_t scaled     = 0U;

    if ((handle != NULL) && (permille != NULL) && handle->is_initialised)
    {
        if (handle->config.adc_read(handle->config.channel_id, &millivolts))
        {
            scaled = ((uint32_t)millivolts * DRV_POT_PERMILLE_MAX) /
                     (uint32_t)handle->config.full_scale_mv;

            if (scaled > DRV_POT_PERMILLE_MAX)
            {
                scaled = DRV_POT_PERMILLE_MAX;
            }

            *permille = (uint16_t)scaled;
            read_ok   = true;
        }
    }

    return read_ok;
}
