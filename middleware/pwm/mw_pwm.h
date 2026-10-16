/**
 * @file    mw_pwm.h
 * @author  Madesh
 * @brief   Public interface for the PWM middleware (STM32 HAL wrapper).
 * @version 0.3
 * @date    2026-10-03
 *
 * @note
 * - One channel: TIM3 channel 1 on PA6 (Arduino D12), drives TB6612FNG PWMA.
 * - Fit 10 kOhm from PWMA to GND so the pump stays off while the MCU is in reset.
 *
 * @revision_history
 * - 0.1 2026-10-03 Madesh Initial creation.
 * - 0.3 2026-10-03 Madesh Two valves, fail-safe venting, pump speed by PWM.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef MW_PWM_H
#define MW_PWM_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "pwm_if.h"

/* ── Public Macros ───────────────────────────────────────────────────────── */
#define MW_PWM_CH_PUMP          (0U)       /* PA6 / D12, TIM3_CH1 */
#define MW_PWM_CHANNEL_COUNT    (1U)
#define MW_PWM_FREQUENCY_HZ     (20000U)   /* above hearing; TB6612FNG accepts up to 100 kHz */
#define MW_PWM_PERMILLE_MAX     (1000U)

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    PIF_PWM_SetDutyFn set_duty;
} MW_PWM_If_t;

/* ── Public Variables ────────────────────────────────────────────────────── */
extern const MW_PWM_If_t MW_PWM_If;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    MW_PWM_Init
 * @brief   Configure PA6 as TIM3_CH1 and start TIM3 at MW_PWM_FREQUENCY_HZ with 0% duty.
 * @return  true on success, false if the timer clock gives fewer than 100 counts per period
 *          or a HAL call failed.
 * @note    Must be called before MW_PWM_If.set_duty is used. Needs the HAL TIM module
 *          enabled (CubeMX: PA6 = TIM3_CH1, TIM3 Channel1 = PWM Generation CH1).
 */
bool MW_PWM_Init(void);

#endif /* MW_PWM_H */
