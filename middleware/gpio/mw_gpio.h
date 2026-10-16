/**
 * @file    mw_gpio.h
 * @author  Madesh
 * @brief   Public interface for the GPIO middleware (STM32 HAL wrapper).
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - Implements the PIF_GPIO_ function pointer types using the STM32 HAL.
 * - Logical pin identifiers map to physical pins in mw_gpio.c.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef MW_GPIO_H
#define MW_GPIO_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "gpio_if.h"

/* ── Public Macros ───────────────────────────────────────────────────────── */
#define MW_GPIO_PIN_PUMP          (0U)    /* PB4  -> TB6612FNG AIN1        */
#define MW_GPIO_PIN_BUTTON        (1U)    /* PB5  -> START/STOP, active LOW */
#define MW_GPIO_PIN_S1_DOUT       (2U)    /* PA9  -> HX710B sensor 1 DOUT   */
#define MW_GPIO_PIN_S1_SCLK       (3U)    /* PC7  -> HX710B sensor 1 SCK    */
#define MW_GPIO_PIN_S2_DOUT       (4U)    /* PB0  -> HX710B sensor 2 DOUT   */
#define MW_GPIO_PIN_S2_SCLK       (5U)    /* PA7  -> HX710B sensor 2 SCK    */
#define MW_GPIO_PIN_LED_GREEN     (6U)    /* PA5  -> on-board green LED     */
#define MW_GPIO_PIN_VALVE1        (7U)    /* PB14 -> valve 1 MOSFET gate    */
#define MW_GPIO_PIN_VALVE2        (8U)    /* PA8  -> valve 2 MOSFET gate    */
#define MW_GPIO_PIN_COUNT         (9U)

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    PIF_GPIO_SetFn  set;
    PIF_GPIO_ReadFn read;
} MW_GPIO_If_t;

/* ── Public Variables ────────────────────────────────────────────────────── */
extern const MW_GPIO_If_t MW_GPIO_If;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    MW_GPIO_Init
 * @brief   Configure the pins that CubeMX does not set up (sensor 2 and the valve gates).
 * @note    Must be called after MX_GPIO_Init(). Safe to call even if CubeMX also sets them.
 */
void MW_GPIO_Init(void);

/**
 * @name    MW_GPIO_GetErrorCount
 * @brief   Read how many calls used an invalid pin identifier.
 * @return  Number of invalid-pin calls since start-up.
 */
uint32_t MW_GPIO_GetErrorCount(void);

#endif /* MW_GPIO_H */
