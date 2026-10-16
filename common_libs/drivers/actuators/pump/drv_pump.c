/**
 * @file    drv_pump.c
 * @author  Madesh
 * @brief   Air pump driver - enable pin plus PWM speed through PIF_ function pointers.
 * @version 0.3
 * @date    2026-10-03
 *
 * @note
 * - No hardware access: everything goes through the function pointers in the configuration.
 * - No MISRA C deviations in this file.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.3 2026-10-03 Madesh Two valves, fail-safe venting, pump speed by PWM.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "drv_pump.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   /* NULL */

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool DRV_PUMP_Init(DRV_PUMP_Handle_t *handle, const DRV_PUMP_Config_t *config)
{
    bool init_ok = false;

    if ((handle != NULL) && (config != NULL) && (config->gpio_set != NULL) &&
        (config->pwm_set != NULL))
    {
        handle->config = *config;
        handle->config.gpio_set(handle->config.enable_pin_id, false);
        handle->speed_permille = 0U;

        if (handle->config.pwm_set(handle->config.pwm_channel_id, 0U))
        {
            handle->is_initialised = true;
            init_ok                = true;
        }
    }

    return init_ok;
}


bool DRV_PUMP_SetSpeed(DRV_PUMP_Handle_t *handle, uint16_t speed_permille)
{
    bool set_ok = false;

    if ((handle != NULL) && handle->is_initialised &&
        (speed_permille <= DRV_PUMP_SPEED_MAX_PERMILLE))
    {
        if (handle->config.pwm_set(handle->config.pwm_channel_id, speed_permille))
        {
            handle->config.gpio_set(handle->config.enable_pin_id, (speed_permille > 0U));
            handle->speed_permille = speed_permille;
            set_ok                 = true;
        }
        else
        {
            /* The speed is unknown now: make sure the pump is stopped. */
            handle->config.gpio_set(handle->config.enable_pin_id, false);
            handle->speed_permille = 0U;
        }
    }

    return set_ok;
}


uint16_t DRV_PUMP_GetSpeed(const DRV_PUMP_Handle_t *handle)
{
    uint16_t speed = 0U;

    if ((handle != NULL) && handle->is_initialised)
    {
        speed = handle->speed_permille;
    }

    return speed;
}
