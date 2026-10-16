/**
 * @file    mw_uart.c
 * @author  Madesh
 * @brief   UART middleware - blocking transmit over the HAL.
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - MISRA C 11.8 deviation: HAL_UART_Transmit takes a non-const buffer in older HAL packages,
 * -   so the const qualifier is cast away for the call. The buffer is never modified.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "mw_uart.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ── Private Macros ──────────────────────────────────────────────────────── */
#define MW_UART_TX_TIMEOUT_MS    (100U)

/* ── Private Variables ───────────────────────────────────────────────────── */
static UART_HandleTypeDef *UART_handle = NULL;

/* ── Private Function Prototypes ─────────────────────────────────────────── */
static bool MW_UART_Write(const uint8_t *data, uint16_t len);

/* ── Public Variables ────────────────────────────────────────────────────── */
const MW_UART_If_t MW_UART_If =
{
    .write = MW_UART_Write
};

/* ── Public Function Definitions ─────────────────────────────────────────── */

bool MW_UART_Init(UART_HandleTypeDef *huart)
{
    bool init_ok = false;

    if (huart != NULL)
    {
        UART_handle = huart;
        init_ok     = true;
    }

    return init_ok;
}

/* ── Private Function Definitions ────────────────────────────────────────── */

/**
 * @brief  Transmit bytes over the attached UART.
 * @param  data Bytes to send. Must not be NULL.
 * @param  len  Number of bytes.
 * @return true on success, false on timeout, driver error or missing init.
 */
static bool MW_UART_Write(const uint8_t *data, uint16_t len)
{
    bool              write_ok   = false;
    HAL_StatusTypeDef hal_status = HAL_ERROR;

    if ((UART_handle != NULL) && (data != NULL))
    {
        hal_status = HAL_UART_Transmit(UART_handle, (uint8_t *)data, len, MW_UART_TX_TIMEOUT_MS);
        write_ok   = (hal_status == HAL_OK);
    }

    return write_ok;
}
