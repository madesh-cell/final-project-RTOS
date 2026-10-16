/**
 * @file    mw_adc.h
 * @author  Madesh
 * @brief   Public interface for the ADC middleware (STM32 HAL wrapper).
 * @version 0.2
 * @date    2026-10-03
 *
 * @note
 * - Implements PIF_ADC_ReadVoltageFn on ADC1. Channel 0 is PA0 (A0), the potentiometer.
 * - MW_ADC_Init sets up the ADC itself, so CubeMX does not have to.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.2 2026-10-03 Madesh Two-cuff alternating cycle, valves, potentiometer.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef MW_ADC_H
#define MW_ADC_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "adc_if.h"

/* ── Public Macros ───────────────────────────────────────────────────────── */
#define MW_ADC_CH_POT         (0U)    /* PA0 / A0 */
#define MW_ADC_CHANNEL_COUNT  (1U)
#define MW_ADC_VREF_MV        (3300U)

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    PIF_ADC_ReadVoltageFn read_voltage;
} MW_ADC_If_t;

/* ── Public Variables ────────────────────────────────────────────────────── */
extern const MW_ADC_If_t MW_ADC_If;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    MW_ADC_Init
 * @brief   Configure PA0 as an analog input and set up and calibrate ADC1.
 * @return  true on success, false if any HAL call failed.
 * @note    Must be called before MW_ADC_If.read_voltage is used.
 */
bool MW_ADC_Init(void);

#endif /* MW_ADC_H */
