/**
 * @file    drv_hx710b.h
 * @author  Madesh
 * @brief   Public interface for the HX710B 24-bit pressure sensor driver.
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - Several sensors can be used at once: each one has its own DRV_HX710B_Handle_t.
 * - All hardware access goes through PIF_ function pointers supplied at init.
 * - Poll is non-blocking: it returns DRV_HX710B_STATUS_NOT_READY until a sample exists.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef DRV_HX710B_H
#define DRV_HX710B_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "gpio_if.h"
#include "time_if.h"

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef enum
{
    DRV_HX710B_STATUS_OK        = 0U,
    DRV_HX710B_STATUS_NOT_READY = 1U,
    DRV_HX710B_STATUS_ERROR     = 2U
} DRV_HX710B_Status_e;

typedef struct
{
    PIF_GPIO_SetFn     gpio_set;
    PIF_GPIO_ReadFn    gpio_read;
    PIF_TIME_DelayUsFn delay_us;
    uint8_t            dout_pin_id;
    uint8_t            sclk_pin_id;
} DRV_HX710B_Config_t;

typedef struct
{
    DRV_HX710B_Config_t config;
    bool                is_initialised;
} DRV_HX710B_Handle_t;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    DRV_HX710B_Init
 * @brief   Prepare one HX710B sensor and hold its clock line low.
 * @param   handle Sensor instance to initialise. Must not be NULL.
 * @param   config Pointer to a valid DRV_HX710B_Config_t. All function pointers must be set.
 * @return  true on success, false if an argument is invalid.
 * @note    Does not wait for the sensor. Use DRV_HX710B_Poll to detect a missing sensor.
 */
bool DRV_HX710B_Init(DRV_HX710B_Handle_t *handle, const DRV_HX710B_Config_t *config);

/**
 * @name    DRV_HX710B_Poll
 * @brief   Read one sample if the sensor has one ready (10 Hz).
 * @param   handle    Initialised sensor instance.
 * @param   raw_value Receives the signed 24-bit reading when OK is returned.
 * @return  OK if a sample was read, NOT_READY if none is available yet, ERROR on bad arguments.
 * @note    Takes about 100 us when a sample is read. Not ISR-safe.
 */
DRV_HX710B_Status_e DRV_HX710B_Poll(DRV_HX710B_Handle_t *handle, int32_t *raw_value);

#endif /* DRV_HX710B_H */
