/**
 * @file    pwm_if.h
 * @author  Madesh
 * @brief   Platform interface - PWM function pointer contract (PIF_ layer).
 * @version 0.3
 * @date    2026-10-03
 *
 * @note
 * - This file is FROZEN after v0.0.1-alpha. Changes require Senior sign-off.
 * - New in 0.3: needed for pump speed control. Requires Senior sign-off before release.
 * - Contains typedefs only. No implementation, no extern, no logic macros.
 *
 * @revision_history
 * - 0.1 2026-10-03 Madesh Initial creation.
 * - 0.3 2026-10-03 Madesh Two valves, fail-safe venting, pump speed by PWM.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef PWM_IF_H
#define PWM_IF_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Set the duty cycle of one PWM output.
 * @param channel_id     Logical channel identifier defined by the middleware layer.
 * @param duty_permille  Duty cycle in tenths of a percent, 0 (always low) to 1000 (always high).
 * @return true on success, false if channel_id or duty_permille is invalid or the output is
 *         not ready.
 */
typedef bool (*PIF_PWM_SetDutyFn)(uint8_t channel_id, uint16_t duty_permille);

#endif /* PWM_IF_H */
