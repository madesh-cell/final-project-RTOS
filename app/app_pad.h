/**
 * @file    app_pad.h
 * @author  Madesh
 * @brief   Public interface for the Smart Pneumatic Support Pad application.
 * @version 0.3
 * @date    2026-10-03
 *
 * @note
 * - Two chambers, one pump, one 3/2 valve per chamber. One chamber inflates while the other
 * -   vents. The potentiometer sets the pump speed.
 * - All hardware is reached through the drivers and PIF_ function pointers supplied at init.
 * - The wiring of drivers and middleware happens only in main.c.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.3 2026-10-03 Madesh Two valves, fail-safe venting, pump speed by PWM.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef APP_PAD_H
#define APP_PAD_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdint.h>
#include <stdbool.h>
#include "gpio_if.h"
#include "time_if.h"
#include "uart_if.h"
#include "os_if.h"
#include "lib_fem.h"
#include "drv_hx710b.h"
#include "drv_pump.h"
#include "drv_valve.h"
#include "drv_pot.h"
#include "app_pad_config.h"

/* ── Public Types ────────────────────────────────────────────────────────── */
typedef enum
{
    APP_PAD_STATE_IDLE    = 0U,
    APP_PAD_STATE_RUNNING = 1U,
    APP_PAD_STATE_FAULT   = 2U,   /* start-up failure only: latched until reset or button */
    APP_PAD_STATE_HELD    = 3U    /* pump off, valves frozen, pressure kept               */
} APP_Pad_State_e;

typedef enum
{
    APP_PAD_PHASE_INFLATE_1 = 0U,
    APP_PAD_PHASE_INFLATE_2 = 1U
} APP_Pad_Phase_e;

typedef enum
{
    APP_PAD_STEP_PREFILL  = 0U,   /* FILL A: first group fills to full            */
    APP_PAD_STEP_INFLATE  = 1U,   /* REFILL: new group fills to full, old vents   */
    APP_PAD_STEP_HOLD     = 2U,   /* HOLD: 5 minutes, old group vents             */
    APP_PAD_STEP_TRANSFER = 3U    /* TRANSFER: both groups joined, pump off       */
} APP_Pad_Step_e;

typedef struct
{
    DRV_HX710B_Handle_t *sensor1;
    DRV_HX710B_Handle_t *sensor2;
    DRV_PUMP_Handle_t   *pump;
    DRV_VALVE_Handle_t  *valve1;
    DRV_VALVE_Handle_t  *valve2;
    DRV_POT_Handle_t    *pot;
    PIF_GPIO_SetFn       gpio_set;
    PIF_GPIO_ReadFn      gpio_read;
    PIF_TIME_GetMsFn     get_ms;
    PIF_UART_WriteFn     uart_write;
    PIF_OS_LockFn        os_lock;
    PIF_OS_VoidFn        os_unlock;
    PIF_OS_VoidFn        os_sched_lock;
    PIF_OS_VoidFn        os_sched_unlock;
    uint8_t              button_pin_id;
    uint8_t              led_pin_id;
} APP_Pad_Config_t;

/* ── Public Function Prototypes ──────────────────────────────────────────── */

/**
 * @name    APP_Pad_Init
 * @brief   Initialise the application: vent both chambers, then measure both sensor zero points.
 * @param   config Pointer to a valid APP_Pad_Config_t. All pointers must be set.
 * @return  true if the module is ready, false if the configuration is invalid or a sensor,
 *          valve or the pump failed (the failure is reported through LIB_FEM).
 * @note    Blocks for about 7 seconds: APP_PAD_VENT_SETTLE_MS for the chambers to empty, then
 *          the baseline samples. After a failure the module stays in FAULT with the pump off
 *          and both chambers venting, and only a reset clears it.
 */
bool APP_Pad_Init(const APP_Pad_Config_t *config);

/**
 * @name    APP_Pad_RunSensors
 * @brief   Task step: read both pressure sensors and the knob.
 * @note    Call from a task every APP_PAD_SENSOR_TASK_PERIOD_MS. The sensor bit-banging runs
 *          with task switching stopped, so no other task can stretch a clock pulse.
 */
void APP_Pad_RunSensors(void);

/**
 * @name    APP_Pad_RunButton
 * @brief   Task step: read and debounce the button and remember the press.
 * @note    Call from a task every APP_PAD_BUTTON_TASK_PERIOD_MS.
 */
void APP_Pad_RunButton(void);

/**
 * @name    APP_Pad_RunSafety
 * @brief   Task step: check for a lost sensor and for over-pressure.
 * @note    Call from the highest-priority task every APP_PAD_SAFETY_TASK_PERIOD_MS.
 */
void APP_Pad_RunSafety(void);

/**
 * @name    APP_Pad_RunControl
 * @brief   Task step: apply the button press, then run the current step of the cycle.
 * @note    Call from a task every APP_PAD_CONTROL_TASK_PERIOD_MS.
 */
void APP_Pad_RunControl(void);

/**
 * @name    APP_Pad_RunDisplay
 * @brief   Task step: update the LED blink code and print the status line.
 * @note    Call from the lowest-priority task every APP_PAD_DISPLAY_TASK_PERIOD_MS.
 */
void APP_Pad_RunDisplay(void);

/**
 * @name    APP_Pad_Run
 * @brief   Run one pass of all five task steps in order (no RTOS, or for the host test).
 * @note    Call repeatedly from the main loop. Does nothing until APP_Pad_Init has accepted
 *          the configuration.
 */
void APP_Pad_Run(void);

/**
 * @name    APP_Pad_GetState
 * @brief   Read the current application state.
 * @return  Current APP_Pad_State_e value.
 */
APP_Pad_State_e APP_Pad_GetState(void);

/**
 * @name    APP_Pad_GetPhase
 * @brief   Read which chamber is currently the inflating (or holding) one.
 * @return  Current APP_Pad_Phase_e value.
 */
APP_Pad_Phase_e APP_Pad_GetPhase(void);

/**
 * @name    APP_Pad_GetStep
 * @brief   Read the current step of the running cycle.
 * @return  Current APP_Pad_Step_e value. Only meaningful while RUNNING.
 */
APP_Pad_Step_e APP_Pad_GetStep(void);

/**
 * @name    APP_Pad_OnFemEvent
 * @brief   LIB_FEM callback that prints every reported event on the UART.
 * @param   event Event that was just recorded. Must not be NULL.
 * @note    Register this function in LIB_FEM_Config_t.on_event.
 */
void APP_Pad_OnFemEvent(const LIB_FEM_Event_t *event);

#endif /* APP_PAD_H */
