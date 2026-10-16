/**
 * @file    mw_uart.h
 * @author  Madesh
 * @brief   Public interface for the UART middleware (STM32 HAL wrapper).
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - Implements the PIF_UART_ function pointer types on the ST-LINK virtual COM port.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef MW_UART_H
#define MW_UART_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "uart_if.h"
#include "main.h"

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef struct
{
    PIF_UART_WriteFn write;
} MW_UART_If_t;

/* ── Public Variables ────────────────────────────────────────────────────── */
extern const MW_UART_If_t MW_UART_If;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    MW_UART_Init
 * @brief   Attach the middleware to an initialised HAL UART handle.
 * @param   huart Handle created by the CubeMX-generated init code. Must not be NULL.
 * @return  true on success, false if huart is NULL.
 */
bool MW_UART_Init(UART_HandleTypeDef *huart);

#endif /* MW_UART_H */
