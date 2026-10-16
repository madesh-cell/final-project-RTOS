/**
 * @file    mw_pwm.c
 * @author  Madesh
 * @brief   PWM middleware - TIM3 channel 1 on PA6 through the STM32 HAL.
 * @version 0.4
 * @date    2026-10-04
 *
 * @note
 * - The period is computed from the real timer clock, so the frequency is right whatever
 * -   system clock CubeMX chose.
 * - MISRA deviation: HAL register macros (TIM3, RCC->CFGR) use casts from integers to
 * -   pointers inside the vendor headers (Rule 11.4). Same deviation as the other MW_ files.
 *
 * @revision_history
 * - 0.1 2026-10-03 Madesh Initial creation.
 * - 0.3 2026-10-03 Madesh Two valves, fail-safe venting, pump speed by PWM.
 * - 0.4 2026-10-04 Madesh Accept a 16 MHz timer clock (no PLL) for 20 kHz PWM.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "mw_pwm.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* ── Private Macros ──────────────────────────────────────────────────────── */
#define MW_PWM_MIN_PERIOD_COUNTS    (100U)   /* at least 100 duty steps per period */

/* ── Private Variables ───────────────────────────────────────────────────── */
static TIM_HandleTypeDef PWM_handle;
static uint32_t          PWM_period_counts = 0U;
static bool              PWM_is_ready      = false;

/* ── Private Function Prototypes ─────────────────────────────────────────── */
static bool     MW_PWM_SetDuty(uint8_t channel_id, uint16_t duty_permille);
static uint32_t MW_PWM_GetTimerClockHz(void);

/* ── Public Variables ────────────────────────────────────────────────────── */
const MW_PWM_If_t MW_PWM_If =
{
    .set_duty = MW_PWM_SetDuty
};

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool MW_PWM_Init(void)
{
    bool               init_ok  = false;
    uint32_t           clock_hz = MW_PWM_GetTimerClockHz();
    GPIO_InitTypeDef   gpio     = {0};
    TIM_OC_InitTypeDef channel  = {0};

    __HAL_RCC_TIM3_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA6 as TIM3_CH1 (alternate function 1). */
    gpio.Pin       = GPIO_PIN_6;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF1_TIM3;
    HAL_GPIO_Init(GPIOA, &gpio);

    PWM_period_counts = clock_hz / MW_PWM_FREQUENCY_HZ;

    /* 16 MHz gives 800 counts per period at 20 kHz, which is plenty of duty resolution. */
    if (PWM_period_counts >= MW_PWM_MIN_PERIOD_COUNTS)
    {
        PWM_handle.Instance               = TIM3;
        PWM_handle.Init.Prescaler         = 0U;
        PWM_handle.Init.CounterMode       = TIM_COUNTERMODE_UP;
        PWM_handle.Init.Period            = PWM_period_counts - 1U;
        PWM_handle.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
        PWM_handle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;

        if (HAL_TIM_PWM_Init(&PWM_handle) == HAL_OK)
        {
            channel.OCMode     = TIM_OCMODE_PWM1;
            channel.Pulse      = 0U;
            channel.OCPolarity = TIM_OCPOLARITY_HIGH;
            channel.OCFastMode = TIM_OCFAST_DISABLE;

            if (HAL_TIM_PWM_ConfigChannel(&PWM_handle, &channel, TIM_CHANNEL_1) == HAL_OK)
            {
                if (HAL_TIM_PWM_Start(&PWM_handle, TIM_CHANNEL_1) == HAL_OK)
                {
                    PWM_is_ready = true;
                    init_ok      = true;
                }
            }
        }
    }

    return init_ok;
}

/* ── Private Function Definitions ────────────────────────────────────────── */

/**
 * @brief  Set the duty cycle of one PWM channel.
 * @param  channel_id    Logical channel identifier (MW_PWM_CH_*).
 * @param  duty_permille Duty cycle, 0 to MW_PWM_PERMILLE_MAX.
 * @return true on success, false on a bad argument or missing init.
 */
static bool MW_PWM_SetDuty(uint8_t channel_id, uint16_t duty_permille)
{
    bool     set_ok  = false;
    uint32_t compare = 0U;

    if (PWM_is_ready && (channel_id < MW_PWM_CHANNEL_COUNT) &&
        (duty_permille <= MW_PWM_PERMILLE_MAX))
    {
        compare = (PWM_period_counts * (uint32_t)duty_permille) / MW_PWM_PERMILLE_MAX;
        __HAL_TIM_SET_COMPARE(&PWM_handle, TIM_CHANNEL_1, compare);
        set_ok = true;
    }

    return set_ok;
}


/**
 * @brief  Clock that drives TIM3.
 * @return Timer clock in Hz.
 * @note   On the STM32G0 the timer clock is twice PCLK when the APB prescaler is not 1.
 */
static uint32_t MW_PWM_GetTimerClockHz(void)
{
    uint32_t clock_hz = HAL_RCC_GetPCLK1Freq();

    if ((RCC->CFGR & RCC_CFGR_PPRE) != 0U)
    {
        clock_hz *= 2U;
    }

    return clock_hz;
}
