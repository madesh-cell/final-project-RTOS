/**
 * @file    app_pad_config.h
 * @author  Madesh
 * @brief   Tunable settings for the Smart Pneumatic Support Pad application.
 * @version 0.6
 * @date    2026-10-03
 *
 * @note
 * - Pressure scales were calibrated against a gauge (about +/-3 mmHg gauge accuracy).
 * - Each scale is valid for its own sensor module only; recalibrate if a module is replaced.
 * - Pressure values are in tenths of mmHg to avoid floating point.
 * - Bench values for a test load. They are not validated for use on a person.
 *
 * @revision_history
 * - 0.1 2026-10-01 Madesh Initial creation.
 * - 0.3 2026-10-03 Madesh Two valves, fail-safe venting, pump speed by PWM.
 * - 0.4 2026-10-09 Madesh 9-pillow test: groups A and B, 50 mmHg full, 5 min hold, air transfer.
 * - 0.5 2026-10-09 Madesh Hold and resume, long press, faults hold pressure, LED blink codes.
 * - 0.6 2026-10-14 Madesh RTOS tasks: periods, shared-data lock.
 *
 * @copyright Copyright (c) 2026 CIOET
 */

#ifndef APP_PAD_CONFIG_H
#define APP_PAD_CONFIG_H

/* ── Includes ────────────────────────────────────────────────────────────── */
#include <stdbool.h>

/* ── Public Macros ───────────────────────────────────────────────────────── */

/* Identification */
#define APP_PAD_NODE_ID                      (1U)
#define APP_PAD_CUFF_COUNT                   (2U)   /* group A = 1 (pillows 1-4), group B = 2 (pillows 5-9) */

/* Calibration: raw sensor counts per 1 mmHg. */
#define APP_PAD_COUNTS_PER_MMHG_SENSOR1      (80000)
#define APP_PAD_COUNTS_PER_MMHG_SENSOR2      (84000)
#define APP_PAD_TENTHS_PER_MMHG              (10)

/* Valve air paths. FA0520F: unpowered joins A to B, powered joins B to C.
 * Fail-safe layout: A = exhaust, B = chamber and sensor, C = pump line.
 * So an unpowered valve vents its chamber (power cut, reset, STOP, fault). */
#define APP_PAD_VALVE_VENTS_WHEN_ENERGISED   (false)

/* Pressure levels. Full is 50 mmHg. The group that is leaving only vents; it has no target. */
#define APP_PAD_MAX_TENTHS_MMHG              (500)     /* full: a filling group stops here      */
#define APP_PAD_SLOW_START_TENTHS_MMHG       (499)     /* no slow-down: knob speed up to full   */
#define APP_PAD_TOPUP_START_TENTHS_MMHG      (490)     /* the full group is topped up below     */
#define APP_PAD_TRANSFER_TARGET_TENTHS_MMHG  (150)     /* new group at 30% of full (15 mmHg)    */

/* Pump speed set by the potentiometer, in permille of full PWM duty. */
#define APP_PAD_SPEED_MIN_PERMILLE           (800U)    /* knob at minimum = 80% pump speed     */
#define APP_PAD_SPEED_MAX_PERMILLE           (1000U)

/* Safety limits and timing. */
#define APP_PAD_LIMIT_TENTHS_MMHG            (600)     /* any group at or above = fault (proposed) */
#define APP_PAD_HOLD_MS                      (120000U) /* each state is held 5 minutes             */
#define APP_PAD_FILL_TIMEOUT_MS              (150000U) /* one fill to full took too long           */
#define APP_PAD_TRANSFER_TIMEOUT_MS          (60000U)  /* new group never reached 30%              */
#define APP_PAD_SENSOR_TIMEOUT_MS            (1000U)   /* no new sample this long = fault          */

/* Start-up zero point. Both chambers vent first, so the baselines are taken at ambient. */
#define APP_PAD_VENT_SETTLE_MS               (5000U)
#define APP_PAD_BASELINE_SAMPLES             (8U)
#define APP_PAD_BASELINE_TIMEOUT_MS          (3000U)

/* Task periods (RTOS build). Each task wakes this often. */
#define APP_PAD_SENSOR_TASK_PERIOD_MS        (20U)     /* poll both pressure sensors          */
#define APP_PAD_BUTTON_TASK_PERIOD_MS        (10U)     /* read and debounce the button        */
#define APP_PAD_SAFETY_TASK_PERIOD_MS        (50U)     /* lost sensor and over-pressure check */
#define APP_PAD_CONTROL_TASK_PERIOD_MS       (20U)     /* fill / hold / transfer / refill     */
#define APP_PAD_DISPLAY_TASK_PERIOD_MS       (50U)     /* status line and LED blink codes     */

/* User interface. */
#define APP_PAD_POT_PERIOD_MS                (50U)
#define APP_PAD_BUTTON_DEBOUNCE_MS           (30U)
#define APP_PAD_LONG_PRESS_MS               (3000U)   /* hold the button this long = stop and vent */

/* Error LED: the fault code is shown as this many blinks, then a pause. */
#define APP_PAD_LED_BLINK_ON_MS              (200U)
#define APP_PAD_LED_BLINK_OFF_MS             (200U)
#define APP_PAD_LED_PAUSE_MS                 (1200U)
#define APP_PAD_STATUS_PERIOD_MS             (200U)
#define APP_PAD_TEXT_BUFFER_SIZE_BYTES       (112U)
#define APP_PAD_PERMILLE_PER_PERCENT         (10U)

#endif /* APP_PAD_CONFIG_H */
