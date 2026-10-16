/**
 * @file    drv_pump.h
 * @author  Madesh
 * @brief   Public interface for the air pump driver (enable pin plus PWM speed).
 * @version 0.3
 * @date    2026-10-03
 *
 * @note
 * - Made for a TB6612FNG channel: enable on AIN1 (AIN2 to GND), speed on PWMA.
 * - Speed 0 sets the duty to 0 and then drops the enable, so two signals stop the pump.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.3 2026-10-03 Madesh Two valves, fail-safe venting, pump speed by PWM.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef DRV_PUMP_H
#define DRV_PUMP_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "gpio_if.h"
#include "pwm_if.h"

/* ── Public Macros ───────────────────────────────────────────────────────── */
#define DRV_PUMP_SPEED_MAX_PERMILLE    (1000U)

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    PIF_GPIO_SetFn    gpio_set;
    uint8_t           enable_pin_id;
    PIF_PWM_SetDutyFn pwm_set;
    uint8_t           pwm_channel_id;
} DRV_PUMP_Config_t;

typedef struct
{
    DRV_PUMP_Config_t config;
    uint16_t          speed_permille;
    bool              is_initialised;
} DRV_PUMP_Handle_t;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    DRV_PUMP_Init
 * @brief   Prepare the pump driver and make sure the pump is off.
 * @param   handle Pump instance to initialise. Must not be NULL.
 * @param   config Pointer to a valid DRV_PUMP_Config_t. gpio_set and pwm_set must not be NULL.
 * @return  true on success, false if an argument is invalid or the PWM could not be zeroed.
 */
bool DRV_PUMP_Init(DRV_PUMP_Handle_t *handle, const DRV_PUMP_Config_t *config);

/**
 * @name    DRV_PUMP_SetSpeed
 * @brief   Run the pump at a given speed, or stop it.
 * @param   handle         Initialised pump instance.
 * @param   speed_permille 0 stops the pump, 1 to DRV_PUMP_SPEED_MAX_PERMILLE sets the PWM duty.
 * @return  true on success, false if an argument is invalid or the PWM write failed.
 * @note    On a failed PWM write the enable is dropped, so the pump is stopped either way.
 */
bool DRV_PUMP_SetSpeed(DRV_PUMP_Handle_t *handle, uint16_t speed_permille);

/**
 * @name    DRV_PUMP_GetSpeed
 * @brief   Read the last commanded speed.
 * @param   handle Initialised pump instance.
 * @return  Speed in permille, 0 if stopped or the handle is invalid.
 */
uint16_t DRV_PUMP_GetSpeed(const DRV_PUMP_Handle_t *handle);

#endif /* DRV_PUMP_H */
