/**
 * @file    mw_gpio.c
 * @author  Madesh
 * @brief   GPIO middleware - maps logical pin identifiers to STM32 HAL pins.
 * @version 0.1
 * @date    2026-10-01
 *
 * @note
 * - The PIF_GPIO_ function pointer types return void/bool, so an invalid pin id cannot be
 * -   returned to the caller. It is counted instead; see MW_GPIO_GetErrorCount().
 * - No MISRA C deviations in this file.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

/* ── Includes ────────────────────────────────────────────────────────────── */
#include "mw_gpio.h"
#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/* ── Private Types ───────────────────────────────────────────────────────── */
typedef struct
{
    GPIO_TypeDef *port;
    uint16_t      pin;
} MW_GPIO_PinMap_t;

/* ── Private Variables ───────────────────────────────────────────────────── */
static const MW_GPIO_PinMap_t GPIO_pin_map[MW_GPIO_PIN_COUNT] =
{
    [MW_GPIO_PIN_PUMP]      = { GPIOB,               GPIO_PIN_4        },
    [MW_GPIO_PIN_BUTTON]    = { GPIOB,               GPIO_PIN_5        },
    [MW_GPIO_PIN_S1_DOUT]   = { GPIOA,               GPIO_PIN_9        },
    [MW_GPIO_PIN_S1_SCLK]   = { GPIOC,               GPIO_PIN_7        },
    [MW_GPIO_PIN_S2_DOUT]   = { GPIOB,               GPIO_PIN_0        },
    [MW_GPIO_PIN_S2_SCLK]   = { GPIOA,               GPIO_PIN_7        },
    [MW_GPIO_PIN_LED_GREEN] = { LED_GREEN_GPIO_Port, LED_GREEN_Pin     },
    [MW_GPIO_PIN_VALVE1]    = { GPIOB,               GPIO_PIN_14       },
    [MW_GPIO_PIN_VALVE2]    = { GPIOA,               GPIO_PIN_8        }
};

static uint32_t GPIO_error_count = 0U;

/* ── Private Function Prototypes ─────────────────────────────────────────── */
static void MW_GPIO_Set(uint8_t pin_id, bool state);
static bool MW_GPIO_Read(uint8_t pin_id);

/* ── Public Variables ────────────────────────────────────────────────────── */
const MW_GPIO_If_t MW_GPIO_If =
{
    .set  = MW_GPIO_Set,
    .read = MW_GPIO_Read
};

/* ── Public Function Definitions ─────────────────────────────────────────── */

void MW_GPIO_Init(void)
{
    GPIO_InitTypeDef init = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Sensor 2 clock (PA7): output, idle LOW so the HX710B is never put into power-down. */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
    init.Pin   = GPIO_PIN_7;
    init.Mode  = GPIO_MODE_OUTPUT_PP;
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &init);

    /* Sensor 2 data (PB0): input driven by the HX710B. */
    init.Pin  = GPIO_PIN_0;
    init.Mode = GPIO_MODE_INPUT;
    init.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &init);

    /* Valve gates (PB14, PA8): drive LOW first so both valves start de-energised (vent). */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
    init.Pin   = GPIO_PIN_14;
    init.Mode  = GPIO_MODE_OUTPUT_PP;
    init.Pull  = GPIO_NOPULL;
    init.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &init);

    init.Pin = GPIO_PIN_8;
    HAL_GPIO_Init(GPIOA, &init);
}


uint32_t MW_GPIO_GetErrorCount(void)
{
    return GPIO_error_count;
}

/* ── Private Function Definitions ────────────────────────────────────────── */

/**
 * @brief Drive a mapped output pin.
 * @param pin_id Logical pin identifier (MW_GPIO_PIN_*).
 * @param state  true drives HIGH, false drives LOW.
 */
static void MW_GPIO_Set(uint8_t pin_id, bool state)
{
    if (pin_id < MW_GPIO_PIN_COUNT)
    {
        HAL_GPIO_WritePin(GPIO_pin_map[pin_id].port,
                          GPIO_pin_map[pin_id].pin,
                          state ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
    else
    {
        GPIO_error_count++;
    }
}


/**
 * @brief  Read a mapped input pin.
 * @param  pin_id Logical pin identifier (MW_GPIO_PIN_*).
 * @return true if the pin reads HIGH, false if LOW or the pin id is invalid.
 */
static bool MW_GPIO_Read(uint8_t pin_id)
{
    bool is_high = false;

    if (pin_id < MW_GPIO_PIN_COUNT)
    {
        is_high = (HAL_GPIO_ReadPin(GPIO_pin_map[pin_id].port,
                                    GPIO_pin_map[pin_id].pin) == GPIO_PIN_SET);
    }
    else
    {
        GPIO_error_count++;
    }

    return is_high;
}
