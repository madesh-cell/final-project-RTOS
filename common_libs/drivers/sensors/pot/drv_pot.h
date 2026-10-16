/**
 * @file    drv_pot.h
 * @author  Madesh
 * @brief   Public interface for the potentiometer driver (position as permille).
 * @version 0.2
 * @date    2026-10-03
 *
 * @note
 * - Reads the wiper voltage through a PIF_ADC_ReadVoltageFn and scales it to 0..1000.
 * - All hardware access goes through PIF_ function pointers supplied at init.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.2 2026-10-03 Madesh Two-cuff alternating cycle, valves, potentiometer.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef DRV_POT_H
#define DRV_POT_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "adc_if.h"

/* ── Public Macros ───────────────────────────────────────────────────────── */
#define DRV_POT_PERMILLE_MAX    (1000U)

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    PIF_ADC_ReadVoltageFn adc_read;
    uint8_t               channel_id;
    uint16_t              full_scale_mv;
} DRV_POT_Config_t;

typedef struct
{
    DRV_POT_Config_t config;
    bool             is_initialised;
} DRV_POT_Handle_t;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    DRV_POT_Init
 * @brief   Prepare the potentiometer driver.
 * @param   handle Potentiometer instance to initialise. Must not be NULL.
 * @param   config Pointer to a valid DRV_POT_Config_t. adc_read must be set and
 *                 full_scale_mv must be greater than zero.
 * @return  true on success, false if an argument is invalid.
 */
bool DRV_POT_Init(DRV_POT_Handle_t *handle, const DRV_POT_Config_t *config);

/**
 * @name    DRV_POT_ReadPermille
 * @brief   Read the knob position as a value from 0 to 1000.
 * @param   handle   Initialised potentiometer instance.
 * @param   permille Receives the position. Must not be NULL.
 * @return  true on success, false if the ADC read failed or an argument is invalid.
 */
bool DRV_POT_ReadPermille(DRV_POT_Handle_t *handle, uint16_t *permille);

#endif /* DRV_POT_H */
