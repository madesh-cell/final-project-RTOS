/**
 * @file    drv_valve.c
 * @author  Madesh
 * @brief   On/off 3-way solenoid valve driver.
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
#include "drv_valve.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   /* NULL */

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool DRV_VALVE_Init(DRV_VALVE_Handle_t *handle, const DRV_VALVE_Config_t *config)
{
    bool init_ok = false;

    if ((handle != NULL) && (config != NULL) && (config->gpio_set != NULL))
    {
        handle->config = *config;
        handle->config.gpio_set(handle->config.pin_id, false);
        handle->is_energised   = false;
        handle->is_initialised = true;
        init_ok                = true;
    }

    return init_ok;
}


bool DRV_VALVE_SetEnergised(DRV_VALVE_Handle_t *handle, bool energised)
{
    bool set_ok = false;

    if ((handle != NULL) && handle->is_initialised)
    {
        handle->config.gpio_set(handle->config.pin_id, energised);
        handle->is_energised = energised;
        set_ok               = true;
    }

    return set_ok;
}


bool DRV_VALVE_IsEnergised(const DRV_VALVE_Handle_t *handle)
{
    bool is_on = false;

    if ((handle != NULL) && handle->is_initialised)
    {
        is_on = handle->is_energised;
    }

    return is_on;
}
