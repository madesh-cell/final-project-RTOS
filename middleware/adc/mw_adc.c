/**
 * @file    mw_adc.c
 * @author  Madesh
 * @brief   ADC middleware - polled 12-bit conversions on ADC1 channel 0.
 * @version 0.2
 * @date    2026-10-03
 *
 * @note
 * - Each read averages several conversions to reduce noise.
 * - The ADC settings match the ones verified on the NUCLEO-G0B1RE with CubeIDE.
 * - No MISRA C deviations in this file.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.2 2026-10-03 Madesh Two-cuff alternating cycle, valves, potentiometer.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "mw_adc.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "main.h"

/* ── Private Macros ──────────────────────────────────────────────────────── */
#define MW_ADC_FULL_SCALE_COUNTS    (4095U)
#define MW_ADC_AVERAGE_SAMPLES      (8U)
#define MW_ADC_POLL_TIMEOUT_MS      (10U)

/* ── Private Variables ───────────────────────────────────────────────────── */
static ADC_HandleTypeDef ADC_handle;
static bool              ADC_is_ready = false;

/* ── Private Function Prototypes ─────────────────────────────────────────── */
static bool MW_ADC_ReadVoltage(uint8_t channel_id, uint16_t *millivolts);
static bool MW_ADC_ReadOnce(uint32_t *counts);

/* ── Public Variables ────────────────────────────────────────────────────── */
const MW_ADC_If_t MW_ADC_If =
{
    .read_voltage = MW_ADC_ReadVoltage
};

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool MW_ADC_Init(void)
{
    bool                   init_ok = false;
    GPIO_InitTypeDef       gpio    = {0};
    ADC_ChannelConfTypeDef channel = {0};

    __HAL_RCC_ADC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA0 as an analog input. */
    gpio.Pin  = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    ADC_handle.Instance                   = ADC1;
    ADC_handle.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV2;
    ADC_handle.Init.Resolution            = ADC_RESOLUTION_12B;
    ADC_handle.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    ADC_handle.Init.ScanConvMode          = ADC_SCAN_SEQ_FIXED;
    ADC_handle.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    ADC_handle.Init.LowPowerAutoWait      = DISABLE;
    ADC_handle.Init.LowPowerAutoPowerOff  = DISABLE;
    ADC_handle.Init.ContinuousConvMode    = DISABLE;
    ADC_handle.Init.NbrOfConversion       = 1;
    ADC_handle.Init.DiscontinuousConvMode = DISABLE;
    ADC_handle.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    ADC_handle.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    ADC_handle.Init.DMAContinuousRequests = DISABLE;
    ADC_handle.Init.Overrun               = ADC_OVR_DATA_PRESERVED;
    ADC_handle.Init.SamplingTimeCommon1   = ADC_SAMPLETIME_160CYCLES_5;
    ADC_handle.Init.SamplingTimeCommon2   = ADC_SAMPLETIME_1CYCLE_5;
    ADC_handle.Init.OversamplingMode      = DISABLE;
    ADC_handle.Init.TriggerFrequencyMode  = ADC_TRIGGER_FREQ_HIGH;

    if (HAL_ADC_Init(&ADC_handle) == HAL_OK)
    {
        channel.Channel      = ADC_CHANNEL_0;
        channel.Rank         = ADC_REGULAR_RANK_1;
        channel.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;

        if (HAL_ADC_ConfigChannel(&ADC_handle, &channel) == HAL_OK)
        {
            if (HAL_ADCEx_Calibration_Start(&ADC_handle) == HAL_OK)
            {
                ADC_is_ready = true;
                init_ok      = true;
            }
        }
    }

    return init_ok;
}

/* ── Private Function Definitions ────────────────────────────────────────── */

/**
 * @brief  Read one channel as a voltage, averaged over several conversions.
 * @param  channel_id Logical channel identifier (MW_ADC_CH_*).
 * @param  millivolts Receives the voltage in millivolts. Must not be NULL.
 * @return true on success, false on a HAL error, a bad channel or missing init.
 */
static bool MW_ADC_ReadVoltage(uint8_t channel_id, uint16_t *millivolts)
{
    bool     read_ok = false;
    bool     failed  = false;
    uint32_t sum     = 0U;
    uint32_t counts  = 0U;
    uint32_t average = 0U;
    uint8_t  index   = 0U;

    if (ADC_is_ready && (channel_id < MW_ADC_CHANNEL_COUNT) && (millivolts != NULL))
    {
        for (index = 0U; index < MW_ADC_AVERAGE_SAMPLES; index++)
        {
            if (MW_ADC_ReadOnce(&counts))
            {
                sum += counts;
            }
            else
            {
                failed = true;
            }
        }

        if (!failed)
        {
            average     = sum / MW_ADC_AVERAGE_SAMPLES;
            *millivolts = (uint16_t)((average * MW_ADC_VREF_MV) / MW_ADC_FULL_SCALE_COUNTS);
            read_ok     = true;
        }
    }

    return read_ok;
}


/**
 * @brief  Run one polled conversion on ADC1.
 * @param  counts Receives the raw 12-bit result. Must not be NULL.
 * @return true on success, false if any HAL call failed.
 */
static bool MW_ADC_ReadOnce(uint32_t *counts)
{
    bool read_ok = false;

    if (HAL_ADC_Start(&ADC_handle) == HAL_OK)
    {
        if (HAL_ADC_PollForConversion(&ADC_handle, MW_ADC_POLL_TIMEOUT_MS) == HAL_OK)
        {
            *counts = HAL_ADC_GetValue(&ADC_handle);
            read_ok = true;
        }

        if (HAL_ADC_Stop(&ADC_handle) != HAL_OK)
        {
            read_ok = false;
        }
    }

    return read_ok;
}
