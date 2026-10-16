/**
 * @file    drv_valve.h
 * @author  Madesh
 * @brief   Public interface for the on/off 3-way solenoid valve driver.
 * @version 0.2
 * @date    2026-10-03
 *
 * @note
 * - The valve coil is switched by an external MOSFET stage (gate HIGH = energised).
 * - What energised means for the air path is decided by the application, not here.
 * - All hardware access goes through PIF_ function pointers supplied at init.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.2 2026-10-03 Madesh Two-cuff alternating cycle, valves, potentiometer.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef DRV_VALVE_H
#define DRV_VALVE_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "gpio_if.h"

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    PIF_GPIO_SetFn gpio_set;
    uint8_t        pin_id;
} DRV_VALVE_Config_t;

typedef struct
{
    DRV_VALVE_Config_t config;
    bool               is_initialised;
    bool               is_energised;
} DRV_VALVE_Handle_t;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    DRV_VALVE_Init
 * @brief   Prepare the valve driver and make sure the coil is de-energised.
 * @param   handle Valve instance to initialise. Must not be NULL.
 * @param   config Pointer to a valid DRV_VALVE_Config_t. gpio_set must not be NULL.
 * @return  true on success, false if an argument is invalid.
 */
bool DRV_VALVE_Init(DRV_VALVE_Handle_t *handle, const DRV_VALVE_Config_t *config);

/**
 * @name    DRV_VALVE_SetEnergised
 * @brief   Energise or de-energise the valve coil.
 * @param   handle    Initialised valve instance.
 * @param   energised true to power the coil, false to remove power.
 * @return  true on success, false if the handle is invalid or not initialised.
 */
bool DRV_VALVE_SetEnergised(DRV_VALVE_Handle_t *handle, bool energised);

/**
 * @name    DRV_VALVE_IsEnergised
 * @brief   Read the last commanded coil state.
 * @param   handle Initialised valve instance.
 * @return  true if the coil is commanded on, false if off or the handle is invalid.
 */
bool DRV_VALVE_IsEnergised(const DRV_VALVE_Handle_t *handle);

#endif /* DRV_VALVE_H */
