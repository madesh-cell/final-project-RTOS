/**
 * @file    drv_hx710b.c
 * @author  Madesh
 * @brief   HX710B 24-bit pressure sensor driver (bit-banged 2-wire interface).
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - Clock pulses use PIF_TIME_DelayUsFn. The HX710B needs 0.2 us to 50 us per half period.
 * - Holding SCLK high for more than about 60 us would put the chip into power-down.
 * - No MISRA C deviations in this file.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "drv_hx710b.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>   /* NULL */

/* ── Private Macros ──────────────────────────────────────────────────────── */
#define DRV_HX710B_DATA_BITS               (24U)
#define DRV_HX710B_CLOCK_PULSES            (25U)    /* 24 data bits + 1 mode pulse (10 Hz input) */
#define DRV_HX710B_SIGN_BIT_MASK           (0x00800000U)
#define DRV_HX710B_VALUE_MASK              (0x007FFFFFU)
#define DRV_HX710B_CLOCK_HALF_PERIOD_US    (2U)

/* ── Private Function Prototypes ─────────────────────────────────────────── */
static bool DRV_HX710B_IsConfigValid(const DRV_HX710B_Config_t *config);
static uint32_t DRV_HX710B_ShiftInSample(const DRV_HX710B_Config_t *config);

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool DRV_HX710B_Init(DRV_HX710B_Handle_t *handle, const DRV_HX710B_Config_t *config)
{
    bool init_ok = false;

    if ((handle != NULL) && DRV_HX710B_IsConfigValid(config))
    {
        handle->config = *config;
        handle->config.gpio_set(handle->config.sclk_pin_id, false);
        handle->is_initialised = true;
        init_ok                = true;
    }

    return init_ok;
}


DRV_HX710B_Status_e DRV_HX710B_Poll(DRV_HX710B_Handle_t *handle, int32_t *raw_value)
{
    DRV_HX710B_Status_e status   = DRV_HX710B_STATUS_ERROR;
    uint32_t            raw_bits = 0U;

    if ((handle != NULL) && (raw_value != NULL) && handle->is_initialised)
    {
        /* DOUT idles HIGH and goes LOW when a new sample is ready. */
        if (handle->config.gpio_read(handle->config.dout_pin_id))
        {
            status = DRV_HX710B_STATUS_NOT_READY;
        }
        else
        {
            raw_bits = DRV_HX710B_ShiftInSample(&handle->config);

            /* Two's complement of a 24-bit value, built without implementation-defined casts. */
            *raw_value = (int32_t)(raw_bits & DRV_HX710B_VALUE_MASK) -
                         (int32_t)(raw_bits & DRV_HX710B_SIGN_BIT_MASK);
            status = DRV_HX710B_STATUS_OK;
        }
    }

    return status;
}

/* ── Private Function Definitions ────────────────────────────────────────── */

/**
 * @brief  Check that every function pointer in a configuration is set.
 * @param  config Configuration to check, may be NULL.
 * @return true if usable, false otherwise.
 */
static bool DRV_HX710B_IsConfigValid(const DRV_HX710B_Config_t *config)
{
    bool is_valid = false;

    if (config != NULL)
    {
        is_valid = (config->gpio_set != NULL) &&
                   (config->gpio_read != NULL) &&
                   (config->delay_us != NULL);
    }

    return is_valid;
}


/**
 * @brief  Clock one 24-bit sample out of the sensor, MSB first.
 * @param  config Valid sensor configuration.
 * @return The 24 raw bits in the low bits of the result.
 */
static uint32_t DRV_HX710B_ShiftInSample(const DRV_HX710B_Config_t *config)
{
    uint32_t raw_bits    = 0U;
    uint8_t  pulse_index = 0U;

    for (pulse_index = 0U; pulse_index < DRV_HX710B_CLOCK_PULSES; pulse_index++)
    {
        config->gpio_set(config->sclk_pin_id, true);
        config->delay_us(DRV_HX710B_CLOCK_HALF_PERIOD_US);

        if (pulse_index < DRV_HX710B_DATA_BITS)
        {
            raw_bits <<= 1U;

            if (config->gpio_read(config->dout_pin_id))
            {
                raw_bits |= 1U;
            }
        }

        config->gpio_set(config->sclk_pin_id, false);
        config->delay_us(DRV_HX710B_CLOCK_HALF_PERIOD_US);
    }

    return raw_bits;
}
