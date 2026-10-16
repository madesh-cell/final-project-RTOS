/**
 * @file    uart_if.h
 * @author  Madesh
 * @brief   Platform interface - UART function pointer contracts (PIF_ layer).
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - This file is FROZEN after v0.0.1-alpha. Changes require Senior sign-off.
 * - Include path resolved by build system: -I platform_if/ (app repo)
 * -   or -I tests/stub_platform_if/ (CI host build).
 * - Contains typedefs only. No implementation, no extern, no logic macros.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef UART_IF_H
#define UART_IF_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Transmit bytes over the UART.
 * @param data Pointer to the data to transmit.
 * @param len  Number of bytes to transmit.
 * @return true on success, false on timeout or driver error.
 */
typedef bool (*PIF_UART_WriteFn)(const uint8_t *data, uint16_t len);

#endif /* UART_IF_H */
