/**
 * @file    gpio_if.h
 * @author  Madesh
 * @brief   Platform interface - GPIO function pointer contracts (PIF_ layer).
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

#ifndef GPIO_IF_H
#define GPIO_IF_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Drive a GPIO output pin.
 * @param pin_id Logical pin identifier defined by the middleware layer.
 * @param state  true drives the pin HIGH, false drives it LOW.
 */
typedef void (*PIF_GPIO_SetFn)(uint8_t pin_id, bool state);

/**
 * @brief Read a GPIO input pin.
 * @param pin_id Logical pin identifier defined by the middleware layer.
 * @return true if the pin reads HIGH, false if it reads LOW or pin_id is invalid.
 */
typedef bool (*PIF_GPIO_ReadFn)(uint8_t pin_id);

#endif /* GPIO_IF_H */
