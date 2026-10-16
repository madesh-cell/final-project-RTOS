/**
 * @file    adc_if.h
 * @author  Madesh
 * @brief   Platform interface - ADC function pointer contracts (PIF_ layer).
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - This file is FROZEN after v0.0.1-alpha. Changes require Senior sign-off.
 * - Include path resolved by build system: -I platform_if/ (app repo)
 * -   or -I tests/stub_platform_if/ (CI host build).
 * - Contains typedefs only. No implementation, no extern, no logic macros.
 * - Returns bool plus an out-parameter so a failed conversion can be reported.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef ADC_IF_H
#define ADC_IF_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Read one ADC channel as a voltage.
 * @param channel_id  Logical channel identifier defined by the middleware layer.
 * @param millivolts  Receives the measured voltage in millivolts.
 * @return true on success, false if the conversion failed or channel_id is invalid.
 */
typedef bool (*PIF_ADC_ReadVoltageFn)(uint8_t channel_id, uint16_t *millivolts);

#endif /* ADC_IF_H */
